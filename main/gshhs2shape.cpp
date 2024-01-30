// ======================================================================
/*!
 * \file
 * \brief Convert a GSHHS file to a shape file
 */
// ======================================================================
/*!
 * \page gshhs2shape gshhs2shape
 *
 * gshhs2shape takes as input a GSHHS shoreline database,
 * and produces an ESRI shape file
 *
 * Usage:
 * \code
 * gshhs2shape <gshhsfile> <shapename>
 * \endcode
 *
 */
// ----------------------------------------------------------------------

#include <imagine/NFmiEsriPoint.h>
#include <imagine/NFmiEsriPolyLine.h>
#include <imagine/NFmiEsriShape.h>
#include <imagine/NFmiGshhsTools.h>
#include <imagine/NFmiPath.h>
#include <newbase/NFmiCmdLine.h>
#include <stdexcept>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief The main driver
 */
// ----------------------------------------------------------------------

int domain(int argc, const char *argv[])
{
  // Process the command line

  NFmiCmdLine cmdline(argc, argv, "");

  if (cmdline.Status().IsError())
    throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if (cmdline.NumberofParameters() != 2)
    throw runtime_error("Expecting two command line arguments");

  const string gshhsfile = cmdline.Parameter(1);
  const string shapename = cmdline.Parameter(2);

  if (gshhsfile.empty())
    throw runtime_error("The name of the gshhsfile is empty");

  if (shapename.empty())
    throw runtime_error("The name of the shape is empty");

  // Read the GSHHS data

  Imagine::NFmiPath path(Imagine::NFmiGshhsTools::ReadPath(gshhsfile, -180, -90, +180, +90));

  Imagine::NFmiEsriShape shp(Imagine::kFmiEsriPolyLine);

  // Output the data

  Imagine::NFmiEsriPolyLine *line = 0;

  for (Imagine::NFmiPathData::const_iterator it = path.Elements().begin();
       it != path.Elements().end();
       ++it)
  {
    switch ((*it).Oper())
    {
      case Imagine::kFmiMoveTo:
        if (line != 0)
          shp.Add(line);
        line = new Imagine::NFmiEsriPolyLine;
        if (line == 0)
          throw runtime_error("Failed to allocate a new NFmiEsriPolyLine");
      // fall through
      case Imagine::kFmiLineTo:
        if (line == 0)
          throw runtime_error("Internal error - a lineto before a moveto");
        line->Add(Imagine::NFmiEsriPoint((*it).X(), (*it).Y()));
        break;
      case Imagine::kFmiGhostLineTo:
      case Imagine::kFmiConicTo:
      case Imagine::kFmiCubicTo:
        throw runtime_error("The shapefile contains Bezier curve segments");
    }
  }
  if (line != 0)
    shp.Add(line);

  string filename = shapename + ".shp";
  if (!shp.WriteSHP(filename))
    throw runtime_error("Failed to write '" + filename + "'");

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
  catch (runtime_error &e)
  {
    cerr << "Error: gshhs2shape failed due to" << endl << "--> " << e.what() << endl;
  }
  catch (...)
  {
    cerr << "Error: gshhs2shape failed due to an unknown exception" << endl;
  }
  return 1;
}
