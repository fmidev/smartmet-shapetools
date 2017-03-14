// ======================================================================
/*!
 * \file
 * \brief Implementation of etopo2shape program
 */
// ======================================================================

#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiDataMatrix.h>
#include <newbase/NFmiFileString.h>
#include <newbase/NFmiLatLonArea.h>
#include <newbase/NFmiSettings.h>
#include <imagine/NFmiContourTree.h>
#include <imagine/NFmiDataHints.h>
#include <imagine/NFmiEsriShape.h>
#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiPath.h>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <fstream>
#include <iomanip>
#include <set>
#include <string>

using namespace std;
using namespace Imagine;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage information
 */
// ----------------------------------------------------------------------

void usage() {
  cout << "Usage: etopo2shape [options] [shapename]" << endl
       << endl
       << "Available options are:" << endl
       << endl
       << "   -h\t\tHelp" << endl
       << "   -v\t\tVerbose mode" << endl
       << "   -b [x1,y1,x2,y2]\tThe bounding box to extract" << endl
       << "   -l [h1,h2,h3...]\tThe heights to extract" << endl
       << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Container for global variables
 */
// ----------------------------------------------------------------------

struct GlobalsList {
  bool verbose;          // verbose mode flag
  string shapename;      // output shape
  double x1, y1, x2, y2; // the bounding box
  set<int> heights;      // the desired heights

  NFmiDataMatrix<float> values; // the data values
};

static GlobalsList globals;

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 */
// ----------------------------------------------------------------------

void parse_command_line(int argc, const char *argv[]) {
  // suitable defaults for Scandinavia (?)
  globals.verbose = false;
  globals.heights =
      NFmiStringTools::Split<set<int>>("100,200,300,500,700,1000");
  globals.x1 = 6;
  globals.y1 = 51;
  globals.x2 = 49;
  globals.y2 = 71;

  // Begin parsing

  NFmiCmdLine cmdline(argc, argv, "hvb!l!");

  if (cmdline.Status().IsError())
    throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h')) {
    usage();
    exit(0);
  }

  if (cmdline.NumberofParameters() != 1)
    throw runtime_error("One command line parameter is expected");

  globals.shapename = cmdline.Parameter(1);

  if (cmdline.isOption('v'))
    globals.verbose = true;

  if (cmdline.isOption('b')) {
    const vector<double> values =
        NFmiStringTools::Split<vector<double>>(cmdline.OptionValue('b'));
    if (values.size() != 4)
      throw runtime_error("The bounding box must consist of 4 numbers");
    globals.x1 = values[0];
    globals.y1 = values[1];
    globals.x2 = values[2];
    globals.y2 = values[3];
    if (globals.x1 >= globals.x2 || globals.y1 >= globals.y2)
      throw runtime_error("Bounding box is empty");
    if (globals.x1 < -180 || globals.x1 > 180 || globals.x2 < -180 ||
        globals.x2 > 180 || globals.y1 < -90 || globals.y1 > 90 ||
        globals.y2 < -90 || globals.y2 > 90)
      throw runtime_error("Bounding box exceeds geographic coordinate limits");
  }

  if (cmdline.isOption('l')) {
    globals.heights.clear();
    globals.heights =
        NFmiStringTools::Split<set<int>>(cmdline.OptionValue('l'));
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Read ETOPO2 data withing bounding box
 */
// ----------------------------------------------------------------------

void read_etopo2() {
  if (globals.verbose)
    cout << "Reading the topography data..." << endl;

  const string filename = NFmiSettings::Require<string>("rasters::etopo2");

  // The data resolution is 2 minutes, which translates into
  // a grid of size 10801x5401 with 16 bit elements. The data
  // starts at 89"58 N, 180 W. The byte order is big endian.

  // Grid bounds in integer coordinates - file ordering

  const int columns = 10801;
  // const int rows = 5401;

  const int i1 = static_cast<int>(std::floor((globals.x1 + 180) * 30));
  const int i2 = static_cast<int>(std::ceil((globals.x2 + 180) * 30));
  const int j1 = static_cast<int>(std::floor((90 - globals.y2) * 30));
  const int j2 = static_cast<int>(std::ceil((90 - globals.y1) * 30));

  // The actual bounding box extracted:

  globals.x1 = i1 / 30.0 - 180;
  globals.x2 = i2 / 30.0 - 180;
  globals.y1 = 90 - j2 / 30.0;
  globals.y2 = 90 - j1 / 30.0;

  // The data is pixel centered so we must adjust by hald grid spacing
  // to get coordinates right. However, it to match Scandinavian
  // coastlines better with ESRI data, we shift a full grid spacing
  // in Y-direction. The difference may be due to a different
  // globe approximation.

  globals.x1 += 1 / 30.0 / 2.0;
  globals.x2 += 1 / 30.0 / 2.0;
  globals.y1 -= 1 / 30.0;
  globals.y2 -= 1 / 30.0;

  if (globals.verbose)
    cout << "The grid to be extracted is " << i2 - i1 + 1 << 'x' << j2 - j1 + 1
         << '+' << i1 << '+' << j1 << endl;

  // Initialize the height matrix

  globals.values.Resize(i2 - i1 + 1, j2 - j1 + 1, 0);

  // Start reading

  ifstream in(filename.c_str(), ios::in | ios::binary);
  if (!in)
    throw runtime_error("Failed to open '" + filename + "' for reading");

  const NFmiFileString tmpfilename(filename);
  const string suffix = tmpfilename.Extension().CharPtr();

  using namespace boost;
  using namespace boost::iostreams;
  filtering_stream<input> filter;
  if (suffix == "gz")
    filter.push(gzip_decompressor());
  else if (suffix == "bz2")
    filter.push(bzip2_decompressor());
  filter.push(in);

  // Skip to the first correct data element

  const int skip = 2 * (j1 * columns + i1);

  if (globals.verbose)
    cout << "Skipping first " << skip << " bytes..." << endl;
  filter.ignore(skip);

  if (globals.verbose)
    cout << "Reading desired subgrid..." << endl;
  for (unsigned int j = 0; j < globals.values.NY(); j++) {
    // skip to next row if not the first row
    if (j > 0)
      filter.ignore(2 * (columns - globals.values.NX()));

    unsigned char ch1, ch2;
    short height;
    for (unsigned int i = 0; i < globals.values.NX(); i++) {
      filter >> noskipws >> ch1 >> ch2;
      height = (ch1 << 8) | ch2;
      globals.values[i][j] = height;
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Insert the given path into the given shape a a polygon
 */
// ----------------------------------------------------------------------

void path_to_shape(const NFmiPath &thePath, NFmiEsriShape &theShape,
                   const NFmiEsriAttribute theAttribute) {
  const NFmiPathData::const_iterator begin = thePath.Elements().begin();
  const NFmiPathData::const_iterator end = thePath.Elements().end();

  NFmiEsriPolygon *polygon = 0;

  for (NFmiPathData::const_iterator it = begin; it != end;) {
    bool doflush = false;

    if ((*it).Oper() == kFmiMoveTo)
      doflush = true;
    else if (++it == end) {
      --it;
      if (polygon == 0)
        polygon = new NFmiEsriPolygon;
      polygon->Add(NFmiEsriPoint((*it).X(), (*it).Y()));
      doflush = true;
    } else
      --it;

    if (doflush) {
      if (polygon != 0) {
        polygon->Add(theAttribute);
        theShape.Add(polygon);
      }
      polygon = new NFmiEsriPolygon;
    }

    polygon->Add(NFmiEsriPoint((*it).X(), (*it).Y()));
    ++it;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create the shape
 */
// ----------------------------------------------------------------------

void create_shape() {
  if (globals.verbose)
    cout << "Contouring the topography data..." << endl;

  NFmiLatLonArea area(NFmiPoint(globals.x1, globals.y1),
                      NFmiPoint(globals.x2, globals.y2), NFmiPoint(0, 0),
                      NFmiPoint(globals.values.NX(), globals.values.NY()));

  NFmiDataHints hints(globals.values);

  // Initialize the shape

  NFmiEsriShape shape(kFmiEsriPolygon);
  NFmiEsriAttributeName *attribute =
      new NFmiEsriAttributeName("HEIGHT", kFmiEsriInteger, 6, 0);
  shape.Add(attribute);

  for (set<int>::const_iterator it = globals.heights.begin();
       it != globals.heights.end(); ++it) {
    if (globals.verbose)
      cout << "  height " << *it << "..." << endl;

    float lolimit = kFloatMissing;
    float hilimit = kFloatMissing;
    if (*it >= 0)
      lolimit = *it;
    else
      hilimit = *it;

    NFmiContourTree tree(lolimit, hilimit);
    tree.SubTriangleMode(false);
    tree.Contour(globals.values, hints, NFmiContourTree::kFmiContourLinear);

    NFmiPath path = tree.Path();
    path.InvProject(&area);

    NFmiEsriAttribute attrvalue(*it, attribute);
    path_to_shape(path, shape, attrvalue);
  }

  if (globals.verbose)
    cout << "Writing result..." << endl;

  shape.Write(globals.shapename);
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program without error trapping
 */
// ----------------------------------------------------------------------

int domain(int argc, const char *argv[]) {
  // Initialize global settings

  NFmiSettings::Init();

  // Parse command line options

  parse_command_line(argc, argv);

  // Read the topography data

  read_etopo2();

  // Contour and write the shape

  create_shape();

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 *
 * The main program is only an error trapping driver for domain
 */
// ----------------------------------------------------------------------

int main(int argc, const char *argv[]) {
  try {
    return domain(argc, argv);
  } catch (const exception &e) {
    cerr << "Error: Caught an exception:" << endl
         << "--> " << e.what() << endl
         << endl;
    return 1;
  } catch (...) {
    cerr << "Error: Caught an unknown exception" << endl;
  }
}
