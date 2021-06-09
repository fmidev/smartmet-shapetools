// ======================================================================
/*!
 * \file
 * \brief Implementation of lights2shape program
 */
// ======================================================================

#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <imagine/NFmiContourTree.h>
#include <imagine/NFmiDataHints.h>
#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriShape.h>
#include <imagine/NFmiPath.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiDataMatrix.h>
#include <newbase/NFmiFileString.h>
#include <newbase/NFmiLatLonArea.h>
#include <newbase/NFmiSettings.h>
#include <fstream>
#include <iomanip>
#include <set>
#include <string>

using namespace std;
using namespace Imagine;

// from 2000.ver1.lights.hdr - the maximum latitude
const double maxlatitude = 75;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage information
 */
// ----------------------------------------------------------------------

void usage()
{
  cout << "Usage: lights2shape [options] [shapename]" << endl
       << endl
       << "Available options are:" << endl
       << endl
       << "   -h\t\tHelp" << endl
       << "   -v\t\tVerbose mode" << endl
       << "   -b [x1,y1,x2,y2]\tThe bounding box to extract" << endl
       << "   -l [l1,l2,l3...]\tThe intensity levels to extract" << endl
       << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Container for global variables
 */
// ----------------------------------------------------------------------

struct GlobalsList
{
  bool verbose;           // verbose mode flag
  string shapename;       // output shape
  double x1, y1, x2, y2;  // the bounding box
  set<int> levels;        // the desired levels

  NFmiDataMatrix<float> values;  // the data values
};

static GlobalsList globals;

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 */
// ----------------------------------------------------------------------

void parse_command_line(int argc, const char *argv[])
{
  // suitable defaults for Scandinavia (?)
  globals.verbose = false;
  globals.levels = NFmiStringTools::Split<set<int>>("32");  // average default
  globals.x1 = 6;
  globals.y1 = 51;
  globals.x2 = 49;
  globals.y2 = 71;

  // Begin parsing

  NFmiCmdLine cmdline(argc, argv, "hvb!l!");

  if (cmdline.Status().IsError())
    throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h'))
  {
    usage();
    exit(0);
  }

  if (cmdline.NumberofParameters() != 1)
    throw runtime_error("One command line parameter is expected");

  globals.shapename = cmdline.Parameter(1);

  if (cmdline.isOption('v'))
    globals.verbose = true;

  if (cmdline.isOption('b'))
  {
    const vector<double> values = NFmiStringTools::Split<vector<double>>(cmdline.OptionValue('b'));
    if (values.size() != 4)
      throw runtime_error("The bounding box must consist of 4 numbers");
    globals.x1 = values[0];
    globals.y1 = values[1];
    globals.x2 = values[2];
    globals.y2 = values[3];
    if (globals.x1 >= globals.x2 || globals.y1 >= globals.y2)
      throw runtime_error("Bounding box is empty");
    if (globals.x1 < -180 || globals.x1 > 180 || globals.x2 < -180 || globals.x2 > 180 ||
        globals.y1 < -90 || globals.y1 > 90 || globals.y2 < -90 || globals.y2 > 90)
      throw runtime_error("Bounding box exceeds geographic coordinate limits");

    // Now fixate the limits - the lights do not cover poles
    // The number 75 is specified in the data header
    globals.y1 = std::min(globals.y1, maxlatitude);
    globals.y2 = std::min(globals.y2, maxlatitude);
    globals.y1 = std::max(globals.y1, -maxlatitude);
    globals.y2 = std::max(globals.y2, -maxlatitude);
  }

  if (cmdline.isOption('l'))
  {
    globals.levels.clear();
    globals.levels = NFmiStringTools::Split<set<int>>(cmdline.OptionValue('l'));
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Read lights data withing bounding box
 */
// ----------------------------------------------------------------------

void read_lights()
{
  if (globals.verbose)
    cout << "Reading the lights data..." << endl;

  const string filename = NFmiSettings::Require<string>("rasters::lights");

  // The data size is 43201x16201 with start coordinates at -180,75
  // The grid resolution is 30x30 seconds. Data is 8 bit.

  const double ratio = 60 * 2;  // 60 minutes and a half

  // Grid bounds in integer coordinates - file ordering

  const int columns = 43201;
  // const int rows = 16201;

  const int i1 = static_cast<int>(std::floor((globals.x1 + 180) * ratio));
  const int i2 = static_cast<int>(std::ceil((globals.x2 + 180) * ratio));
  const int j1 = static_cast<int>(std::floor((maxlatitude - globals.y2) * ratio));
  const int j2 = static_cast<int>(std::ceil((maxlatitude - globals.y1) * ratio));

  // The actual bounding box extracted:

  globals.x1 = i1 / ratio - 180;
  globals.x2 = i2 / ratio - 180;
  globals.y1 = maxlatitude - j2 / ratio;
  globals.y2 = maxlatitude - j1 / ratio;

  // The data is pixel centered so we must adjust by hald grid spacing
  // to get coordinates right.

  globals.x1 += 1 / ratio / 2;
  globals.x2 += 1 / ratio / 2;
  globals.y1 -= 1 / ratio / 2;
  globals.y2 -= 1 / ratio / 2;

  if (globals.verbose)
    cout << "The grid to be extracted is " << i2 - i1 + 1 << 'x' << j2 - j1 + 1 << '+' << i1 << '+'
         << j1 << endl;

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

  const int skip = (j1 * columns + i1);

  if (globals.verbose)
    cout << "Skipping first " << skip << " bytes..." << endl;
  filter.ignore(skip);

  if (globals.verbose)
    cout << "Reading desired subgrid..." << endl;
  for (unsigned int j = 0; j < globals.values.NY(); j++)
  {
    // skip to next row if not the first row
    if (j > 0)
      filter.ignore(columns - globals.values.NX());

    unsigned char ch;
    for (unsigned int i = 0; i < globals.values.NX(); i++)
    {
      filter >> noskipws >> ch;

      globals.values[i][j] = ch;
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Insert the given path into the given shape a a polygon
 */
// ----------------------------------------------------------------------

void path_to_shape(const NFmiPath &thePath,
                   NFmiEsriShape &theShape,
                   const NFmiEsriAttribute theAttribute)
{
  const NFmiPathData::const_iterator begin = thePath.Elements().begin();
  const NFmiPathData::const_iterator end = thePath.Elements().end();

  NFmiEsriPolygon *polygon = 0;

  for (NFmiPathData::const_iterator it = begin; it != end;)
  {
    bool doflush = false;

    if ((*it).Oper() == kFmiMoveTo)
      doflush = true;
    else if (++it == end)
    {
      --it;
      if (polygon == 0)
        polygon = new NFmiEsriPolygon;
      polygon->Add(NFmiEsriPoint((*it).X(), (*it).Y()));
      doflush = true;
    }
    else
      --it;

    if (doflush)
    {
      if (polygon != 0)
      {
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

void create_shape()
{
  if (globals.verbose)
    cout << "Contouring the lights data..." << endl;

  NFmiLatLonArea area(NFmiPoint(globals.x1, globals.y1),
                      NFmiPoint(globals.x2, globals.y2),
                      NFmiPoint(0, 0),
                      NFmiPoint(globals.values.NX(), globals.values.NY()));

  NFmiDataHints hints(globals.values);

  // Initialize the shape

  NFmiEsriShape shape(kFmiEsriPolygon);
  NFmiEsriAttributeName *attribute = new NFmiEsriAttributeName("INTENSITY", kFmiEsriInteger, 4, 0);
  shape.Add(attribute);

  for (set<int>::const_iterator it = globals.levels.begin(); it != globals.levels.end(); ++it)
  {
    if (globals.verbose)
      cout << "  intensity " << *it << "..." << endl;

    NFmiContourTree tree(*it, kFloatMissing);
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

int domain(int argc, const char *argv[])
{
  // Initialize global settings

  NFmiSettings::Init();

  // Parse command line options

  parse_command_line(argc, argv);

  // Read the topography data

  read_lights();

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

int main(int argc, const char *argv[])
{
  try
  {
    return domain(argc, argv);
  }
  catch (const exception &e)
  {
    cerr << "Error: Caught an exception:" << endl << "--> " << e.what() << endl << endl;
    return 1;
  }
  catch (...)
  {
    cerr << "Error: Caught an unknown exception" << endl;
  }
}
