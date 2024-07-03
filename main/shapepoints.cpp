// ======================================================================
/*!
 * \file
 * \brief Implementation of program shapepoints
 */
// ======================================================================
/*!
 * \page shapepoints shapepoints
 *
 * \brief Choose evenly spaced points from given shapefile.
 *
 * Usage:
 * \code
 * shapepoints [options] <inputshape> <outputshape>
 * \endcode
 *
 * The options are:
 * <dl>
 * <dt>-h</dt>
 * <dd>Print usage information</dd>
 * <dt>-f [fieldname]</dt>
 * <dd>The field name of the numeric field used for sorting the points (default
 * is TYPE)</dd>
 * <dt>-n</dt>
 * <dd>Negate the fields to obtain ascending sort</dd>
 * <dt>-p [projection]</dt>
 * <dd>Projection specification translating latlon to XY-coordinates (no
 * default)</dd>
 * <dt>-d [distance]</dt>
 * <dd>The required minimum distance between the points in pixels (default
 * 10)</dd>
 * <dt>-D [distance]</dt>
 * <dd>The required minimum distance from the border (default 0)</dd>
 * </dl>
 *
 */
// ======================================================================

#include "PointSelector.h"
#include <memory>
#include <imagine/NFmiEsriPoint.h>
#include <imagine/NFmiEsriShape.h>
#include <newbase/NFmiArea.h>
#include <newbase/NFmiAreaFactory.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiSettings.h>
#include <memory>
#include <string>

using namespace std;
using namespace Imagine;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage info
 */
// ----------------------------------------------------------------------

void usage()
{
  cout << "Usage: shapepoints [options] <inputshape> <outputshape>" << endl
       << endl
       << "Choose evenly spaced points from the input shape." << endl
       << endl
       << "Options:" << endl
       << endl
       << "\t-h\t\tPrint this help information" << endl
       << "\t-v\t\tVerbose mode" << endl
       << "\t-d [dist]\tMinimum distance between points (10)" << endl
       << "\t-D [dist]\tMinimum distance to border (0)" << endl
       << "\t-p [desc]\tProjection description" << endl
       << "\t-f [name]\tData field used for sorting the points (TYPE)" << endl
       << "\t-n\t\tNegate the field value to obtain ascending sort" << endl
       << endl
       << "Typical usage:" << endl
       << endl
       << "\tAREA=stereographic,25:6,51.3,49,70.2:400,-1" << endl
       << "\tshapepoints -p $AREA -d 20 -n ESRI/europe/places myplaces" << endl
       << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  bool verbose;
  bool negate;
  double mindistance;
  double minborderdistance;
  string projection;
  string fieldname;
  string inputshape;
  string outputshape;

  Options()
      : verbose(false),
        negate(false),
        mindistance(10),
        minborderdistance(0),
        projection(),
        fieldname("TYPE"),
        inputshape(),
        outputshape()
  {
  }
};

//! The sole instance of command line options

Options options;

// ----------------------------------------------------------------------
/*!
 * \brief Parse command line options
 *
 * \return True, if execution is to be continued normally
 */
// ----------------------------------------------------------------------

bool parse_command_line(int argc, const char *argv[])
{
  using namespace NFmiStringTools;

  NFmiCmdLine cmdline(argc, argv, "vhnd!D!f!p!");

  if (cmdline.Status().IsError())
    throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.isOption('h'))
  {
    usage();
    return false;
  }

  if (cmdline.NumberofParameters() != 2)
    throw runtime_error("Two command line arguments are expected");

  options.inputshape = cmdline.Parameter(1);
  options.outputshape = cmdline.Parameter(2);

  if (cmdline.isOption('v'))
    options.verbose = true;
  if (cmdline.isOption('d'))
    options.mindistance = Convert<double>(cmdline.OptionValue('d'));
  if (cmdline.isOption('D'))
    options.minborderdistance = Convert<double>(cmdline.OptionValue('D'));
  if (cmdline.isOption('p'))
    options.projection = cmdline.OptionValue('p');
  if (cmdline.isOption('f'))
    options.fieldname = cmdline.OptionValue('f');
  if (cmdline.isOption('n'))
    options.negate = true;

  // Final checks

  if (options.projection.empty())
    throw runtime_error("Must specify some projection with option -p");

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Feed shape points into the selector
 *
 * \param theSelector The selector
 * \param theShape The shape to feed
 */
// ----------------------------------------------------------------------

void select_points(PointSelector &theSelector, const NFmiEsriShape &theShape)
{
  // Establish the correct attribute

  const NFmiEsriAttributeName *name = theShape.AttributeName(options.fieldname);
  if (name == 0)
    throw runtime_error("The input shape does not have a field named '" + options.fieldname + "'");

  const NFmiEsriAttributeType atype = name->Type();

  if (atype != kFmiEsriInteger && atype != kFmiEsriDouble)
    throw runtime_error("The input shape field named '" + options.fieldname + "' is not numeric");

  // Start processing the points

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  int candidates = 0;
  for (NFmiEsriShape::elements_type::size_type i = 0; i < elements.size(); i++)
  {
    // Ignore empty elements
    if (elements[i] == 0)
      continue;

    // Ignore invalid element types
    if (elements[i]->Type() != kFmiEsriPoint)
      continue;

    const NFmiEsriPoint *elem = static_cast<const NFmiEsriPoint *>(elements[i]);
    const int id = i;
    const double lon = elem->X();
    const double lat = elem->Y();
    const double value = (atype == kFmiEsriInteger ? elements[i]->GetInteger(options.fieldname)
                                                   : elements[i]->GetDouble(options.fieldname));

    if (theSelector.add(id, value, lon, lat))
      ++candidates;
  }

  if (options.verbose)
  {
    cout << "Accepted " << candidates << " candidates out of " << elements.size()
         << " points in the input shape" << endl;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Create output shape from input shape and selector
 *
 * \param theSelector The selector
 * \param theShape The input shape
 */
// ----------------------------------------------------------------------

void create_shape(const PointSelector &theSelector, const NFmiEsriShape &theShape)
{
  // The output shape

  NFmiEsriShape shape(theShape.Type());

  // Copy the attribute type information

  for (NFmiEsriShape::attributes_type::const_iterator ait = theShape.Attributes().begin();
       ait != theShape.Attributes().end();
       ++ait)
  {
    shape.Add(new NFmiEsriAttributeName(**ait));
  }

  // Then copy the desired elements in the order of importance

  if (options.verbose)
    cout << "Selected " << theSelector.size() << " points" << endl;

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  for (PointSelector::const_iterator it = theSelector.begin(); it != theSelector.end(); ++it)
  {
    NFmiEsriElement *tmp = elements[*it]->Clone();
    shape.Add(tmp);
  }

  // And finally save the shape

  if (options.verbose)
    cout << "Saving shapefile '" << options.outputshape << "'" << endl;

  shape.Write(options.outputshape);
}

// ----------------------------------------------------------------------
/*!
 * \brief The main algorithm
 *
 * Algorithm:
 *
 *  -# Read and verify command line options
 *  -# Read the input shapefile
 *  -# Sort the points based on the desired numeric field
 *  -# While there are points remaining
 *    -# If the next point is too close to one chosen earlier, discard it
 *    -# Add the point to the set of accepted points
 *  -# Save the accepted points as a shapefile
 */
// ----------------------------------------------------------------------

int domain(int argc, const char *argv[])
{
  // Init global settings to make sure shapepath is initialized

  NFmiSettings::Init();

  // Parse command line options

  if (!parse_command_line(argc, argv))
    return 0;

  // Read the shape

  if (options.verbose)
    cout << "Reading shapefile '" << options.inputshape << "'" << endl;

  NFmiEsriShape inputshape;
  if (!inputshape.Read(options.inputshape, true))
    throw runtime_error("Failed to read shape '" + options.inputshape + "'");

  if (inputshape.Type() != kFmiEsriPoint)
    throw runtime_error("Input shape must contain plain point data");

  // Setup the selector

  std::shared_ptr<NFmiArea> area = NFmiAreaFactory::Create(options.projection);
  PointSelector selector(*area, options.negate);
  selector.minDistance(options.mindistance);
  selector.boundingBox(area->Left() + options.minborderdistance,
                       area->Top() + options.minborderdistance,
                       area->Right() - options.minborderdistance,
                       area->Bottom() - options.minborderdistance);

  // Feed the shape into point selector

  select_points(selector, inputshape);

  // Create output shape from selected points and save it

  create_shape(selector, inputshape);

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
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
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
  catch (...)
  {
    cerr << "Error: Caught an unknown exception" << endl;
    return 1;
  }
}
