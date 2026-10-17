// ======================================================================
/*!
 * \file
 * \brief A program to filter shapefiles
 */
// ======================================================================
/*!
 * \page shapefilter shapefilter
 *
 * shapefilter takes as input an ESRI shapefile and outputs
 * a new ESRI shapefile based on the command line options.
 *
 */
// ======================================================================

#include <imagine/NFmiCounter.h>
#include <imagine/NFmiEdge.h>
#include <imagine/NFmiEdgeTree.h>
#include <imagine/NFmiEsriPoint.h>
#include <imagine/NFmiEsriPolyLine.h>
#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriShape.h>
#include <imagine/NFmiEsriTools.h>
#include <imagine/NFmiPath.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiSettings.h>
#include <string>

using namespace std;
using namespace Imagine;

// ----------------------------------------------------------------------
/*!
 * \brief Container for command line options
 */
// ----------------------------------------------------------------------

struct OptionsList
{
  string input_shape;
  string output_shape;
  string filter_field;
  string filter_boundingbox;
  bool verbose = false;
  bool filter_odd_count = false;
  bool filter_even_count = false;
};

namespace
{
//! Global instance of command line options
OptionsList options;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage information
 */
// ----------------------------------------------------------------------

void usage()
{
  cout << "Usage: shapefilter [options] [inputshape] [outputshape]" << endl
       << endl
       << "Available options are:" << endl
       << "   -e\tKeep only even numbered edges (national borders etc)" << endl
       << "   -o\tKeep only odd numbered egdes (coastlines etc)" << endl
       << "   -f [name=value]\tKeep only elements with required field value" << endl
       << "   -b [x1,y1,x2,y2]\tBounding box for elements to be kept" << endl
       << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 */
// ----------------------------------------------------------------------

void parse_command_line(int argc, const char *argv[])
{
  // Establish the defaults

  options.verbose = false;
  options.input_shape = "";
  options.output_shape = "";
  options.filter_boundingbox = "";
  options.filter_field = "";
  options.filter_odd_count = false;
  options.filter_even_count = false;

  // Parse

  NFmiCmdLine cmdline(argc, argv, "oehvf!b!");

  if (cmdline.Status().IsError())
    throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h'))
  {
    usage();
    // NOLINTNEXTLINE(concurrency-mt-unsafe): single-threaded command line tool
    exit(0);
  }

  if (cmdline.NumberofParameters() != 2)
    throw runtime_error("Two command line parameters are expected");

  options.input_shape = cmdline.Parameter(1);
  options.output_shape = cmdline.Parameter(2);

  unsigned int filtercount = 0;

  if (cmdline.isOption('v'))
    options.verbose = true;

  if (cmdline.isOption('o'))
  {
    ++filtercount;
    options.filter_odd_count = true;
  }

  if (cmdline.isOption('e'))
  {
    ++filtercount;
    options.filter_even_count = true;
  }

  if (cmdline.isOption('f'))
  {
    ++filtercount;
    options.filter_field = cmdline.OptionValue('f');
  }

  if (cmdline.isOption('b'))
  {
    ++filtercount;
    options.filter_boundingbox = cmdline.OptionValue('b');
  }

  if (filtercount > 1)
    throw runtime_error("Only one filtering method allowed at a time");

  if (options.input_shape == options.output_shape)
    throw runtime_error("Input and output names are equal");
}

// ----------------------------------------------------------------------
/*!
 * \brief Count the edges of a polyline or a polygon
 */
// ----------------------------------------------------------------------

template <typename T>
void count_element_edges(const T& theElement, NFmiCounter<NFmiEdge>& theCounts)
{
  const auto* elem = &theElement;
  for (int part = 0; part < elem->NumParts(); part++)
  {
    int i1 = 0;
    int i2 = 0;
    i1 = elem->Parts()[part];  // start of part
    if (part + 1 == elem->NumParts())
      i2 = elem->NumPoints() - 1;  // end of part
    else
      i2 = elem->Parts()[part + 1] - 1;  // end of part

    if (i2 >= i1)
    {
      double x1 = elem->Points()[i1].X();
      double y1 = elem->Points()[i1].Y();

      for (int i = i1 + 1; i <= i2; i++)
      {
        double x2 = elem->Points()[i].X();
        double y2 = elem->Points()[i].Y();
        theCounts.Add(NFmiEdge(x1, y1, x2, y2, true, false));
        x1 = x2;
        y1 = y2;
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Count the edges in the given shape
 */
// ----------------------------------------------------------------------

void count_edges(const NFmiEsriShape &theShape, NFmiCounter<NFmiEdge> &theCounts)
{
  if (options.verbose)
    cout << "Counting edges in shape..." << endl;

  auto it = theShape.Elements().begin();

  for (; it != theShape.Elements().end(); ++it)
  {
    if (*it == nullptr)
      continue;

    switch ((*it)->Type())
    {
      case kFmiEsriNull:
      case kFmiEsriPoint:
      case kFmiEsriPointM:
      case kFmiEsriPointZ:
      case kFmiEsriMultiPoint:
      case kFmiEsriMultiPointM:
      case kFmiEsriMultiPointZ:
      case kFmiEsriMultiPatch:
        break;
      case kFmiEsriPolyLine:
      case kFmiEsriPolyLineM:
      case kFmiEsriPolyLineZ:
      {
        count_element_edges(*static_cast<const NFmiEsriPolyLine*>(*it), theCounts);
        break;
      }
      case kFmiEsriPolygon:
      case kFmiEsriPolygonM:
      case kFmiEsriPolygonZ:
      {
        count_element_edges(*static_cast<const NFmiEsriPolygon*>(*it), theCounts);
        break;
      }
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Add a point to the polygon or polyline being built, creating it if necessary
 */
// ----------------------------------------------------------------------

void add_path_point(NFmiEsriElementType etype,
                    NFmiEsriPolygon*& polygon,
                    NFmiEsriPolyLine*& polyline,
                    double x,
                    double y)
{
  if (etype == kFmiEsriPolygon)
  {
    if (polygon == nullptr)
      polygon = new NFmiEsriPolygon;
    polygon->Add(NFmiEsriPoint(x, y));
  }
  else
  {
    if (polyline == nullptr)
      polyline = new NFmiEsriPolyLine;
    polyline->Add(NFmiEsriPoint(x, y));
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Add the polygon or polyline being built to the shape and start a new one
 */
// ----------------------------------------------------------------------

void flush_path_element(NFmiEsriElementType etype,
                        NFmiEsriShape& theShape,
                        NFmiEsriPolygon*& polygon,
                        NFmiEsriPolyLine*& polyline)
{
  if (etype == kFmiEsriPolygon)
  {
    if (polygon != nullptr)
      theShape.Add(polygon);
    polygon = new NFmiEsriPolygon;
  }
  else
  {
    if (polyline != nullptr)
      theShape.Add(polyline);
    polyline = new NFmiEsriPolyLine;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Insert the given path into the given shape as polylines
 */
// ----------------------------------------------------------------------

void path_to_shape(const NFmiPath &thePath, NFmiEsriShape &theShape)
{
  if (options.verbose)
    cout << "Converting path to shape..." << endl;

  const NFmiEsriElementType etype = theShape.Type();

  const auto begin = thePath.Elements().begin();
  const auto end = thePath.Elements().end();

  NFmiEsriPolyLine* polyline = nullptr;
  NFmiEsriPolygon* polygon = nullptr;

  for (NFmiPathData::const_iterator it = begin; it != end;)
  {
    bool doflush = false;

    if ((*it).Oper() == kFmiMoveTo)
      doflush = true;
    else if (++it == end)
    {
      --it;
      add_path_point(etype, polygon, polyline, (*it).X(), (*it).Y());
      doflush = true;
    }
    else
      --it;

    if (doflush)
      flush_path_element(etype, theShape, polygon, polyline);

    if (etype == kFmiEsriPolygon)
      polygon->Add(NFmiEsriPoint((*it).X(), (*it).Y()));
    else
      polyline->Add(NFmiEsriPoint((*it).X(), (*it).Y()));
    ++it;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape based on even occurrance count
 */
// ----------------------------------------------------------------------

const NFmiEsriShape *filter_even_count(const NFmiEsriShape &theShape)
{
  if (options.verbose)
    cout << "Filtering even numbered edges..." << endl;

  // Count the edges

  NFmiCounter<NFmiEdge> counts;
  count_edges(theShape, counts);

  // Build a path from even numbered edges

  NFmiEdgeTree tree;
  for (const auto& count : counts)
  {
    if (count.second % 2 == 0)
      tree.Add(count.first);
  }

  if (options.verbose)
    cout << "Converting surviving edges to a path..." << endl;
  NFmiPath path = tree.Path();

  // Convert the tree to a shape

  auto* shape = new NFmiEsriShape(kFmiEsriPolyLine);
  path_to_shape(path, *shape);

  return shape;
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape based on odd occurrance count
 */
// ----------------------------------------------------------------------

const NFmiEsriShape *filter_odd_count(const NFmiEsriShape &theShape)
{
  if (options.verbose)
    cout << "Filtering odd numbered edges..." << endl;

  // Count the edges

  NFmiCounter<NFmiEdge> counts;
  count_edges(theShape, counts);

  // Build a path from odd numbered edges

  NFmiEdgeTree tree;
  for (const auto& count : counts)
  {
    if (count.second % 2 != 0)
      tree.Add(count.first);
  }

  if (options.verbose)
    cout << "Converting surviving edges to a path..." << endl;
  NFmiPath path = tree.Path();

  // Convert the tree to a shape
  // Note that polygons will remain polygons, otherwise we will
  // assume polyline output

  const NFmiEsriElementType oldtype = theShape.Type();
  NFmiEsriElementType newtype = oldtype;
  if (oldtype != kFmiEsriPolygon)
    newtype = kFmiEsriPolyLine;

  auto* shape = new NFmiEsriShape(newtype);
  path_to_shape(path, *shape);

  return shape;
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape based on a field value
 */
// ----------------------------------------------------------------------

const NFmiEsriShape *filter_field(const NFmiEsriShape &theShape)
{
  if (options.verbose)
    cout << "Filtering based on field value..." << endl;

  return NFmiEsriTools::filter(theShape, options.filter_field);
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape based on a bounding box
 */
// ----------------------------------------------------------------------

const NFmiEsriShape *filter_boundingbox(const NFmiEsriShape &theShape)
{
  if (options.verbose)
    cout << "Filtering based on bounding box..." << endl;

  // Parse the bounding box

  const auto values = NFmiStringTools::Split<vector<double>>(options.filter_boundingbox);
  if (values.size() != 4)
    throw runtime_error("Bounding box must consist of 4 values");

  const double x1 = values[0];
  const double y1 = values[1];
  const double x2 = values[2];
  const double y2 = values[3];

  // Check bounding box validity

  if (x1 >= x2 || y1 >= y2)
    throw runtime_error("Bounding box is empty");

  if (x1 < -180 || x1 > 180 || x2 < -180 || x2 > 180 || y1 < -90 || y1 > 90 || y2 < -90 || y2 > 90)
    throw runtime_error("The bounding box exceeds geographic coordinate limits");

  // Start the result shape

  auto* shape = new NFmiEsriShape(theShape.Type());

  // Copy the attribute type information
  for (auto* ait : theShape.Attributes())
  {
    shape->Add(new NFmiEsriAttributeName(*ait));
  }

  // Copy the elements which overlap the bounding box

  for (auto* it : theShape.Elements())
  {
    // Skip empty elements
    if (it == nullptr)
      continue;

    // Establish bounding box of the element
    NFmiEsriBox box;
    it->Update(box);

    // Establish whether element is outside the desired box
    bool outside = false;
    outside |= (box.Xmin() > x2);
    outside |= (box.Xmax() < x1);
    outside |= (box.Ymin() > y2);
    outside |= (box.Ymax() < y1);

    // If elements overlaps bbox, add it to the result
    if (!outside)
    {
      NFmiEsriElement* tmp = it->Clone();
      shape->Add(tmp);
    }
  }

  return shape;
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape
 */
// ----------------------------------------------------------------------

const NFmiEsriShape *filter_shape(const NFmiEsriShape &theShape)
{
  if (!options.filter_field.empty())
    return filter_field(theShape);
  if (options.filter_even_count)
    return filter_even_count(theShape);
  if (options.filter_odd_count)
    return filter_odd_count(theShape);
  if (!options.filter_boundingbox.empty())
    return filter_boundingbox(theShape);

  return &theShape;
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

  // Read the command line arguments

  parse_command_line(argc, argv);

  // Read the shapefile

  if (options.verbose)
    cout << "Reading input shapefile '" + options.input_shape + "'" << endl;

  NFmiEsriShape inputshape;
  if (!inputshape.Read(options.input_shape, true))
    throw runtime_error("Failed to read shape '" + options.input_shape + "'");

  // Process the shapefile

  if (options.verbose)
    cout << "Filtering..." << endl;
  const NFmiEsriShape *outputshape = filter_shape(inputshape);

  // And write it

  if (options.verbose)
    cout << "Writing output shapefile '" + options.output_shape + "'" << endl;
  outputshape->Write(options.output_shape);

  return 0;
}
}  // namespace

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
