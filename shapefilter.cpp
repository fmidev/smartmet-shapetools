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

// newbase
#include "NFmiCmdLine.h"
#include "NFmiSettings.h"
// imagine
#include "NFmiCounter.h"
#include "NFmiEsriPoint.h"
#include "NFmiEsriPolygon.h"
#include "NFmiEsriPolyLine.h"
#include "NFmiEsriShape.h"
#include "NFmiEdge.h"
#include "NFmiEdgeTree.h"
#include "NFmiPath.h"
// system
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
  bool verbose;
  bool filter_odd_count;
  bool filter_even_count;
};

//! Global instance of command line options
static OptionsList options;

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
	   << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 */
// ----------------------------------------------------------------------

void parse_command_line(int argc, const char * argv[])
{
  // Establish the defaults

  options.verbose = false;
  options.input_shape = "";
  options.output_shape = "";
  options.filter_odd_count = false;
  options.filter_even_count = false;

  // Parse
  
  NFmiCmdLine cmdline(argc,argv,"oehv");

  if(cmdline.Status().IsError())
	throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if(cmdline.isOption('h'))
	{
	  usage();
	  exit(0);
	}

  if(cmdline.NumberofParameters() != 2)
	throw runtime_error("Two command line parameters are expected");

  options.input_shape = cmdline.Parameter(1);
  options.output_shape = cmdline.Parameter(2);

  if(cmdline.isOption('v'))
	options.verbose = true;

  if(cmdline.isOption('o'))
	options.filter_odd_count = true;

  if(cmdline.isOption('e'))
	options.filter_even_count = true;

  if(options.filter_even_count && options.filter_odd_count)
	throw runtime_error("Cannot use options -o and -e simultaneously");

  if(options.input_shape == options.output_shape)
	throw runtime_error("Input and output names are equal");

}

// ----------------------------------------------------------------------
/*!
 * \brief Count the edges in the given shape
 */
// ----------------------------------------------------------------------

void count_edges(const NFmiEsriShape & theShape, NFmiCounter<NFmiEdge> & theCounts)
{
  if(options.verbose)
	cout << "Counting edges in shape..." << endl;

  NFmiEsriShape::const_iterator it = theShape.Elements().begin();

  for( ; it != theShape.Elements().end(); ++it)
	{
	  if(*it == NULL)
		continue;

	  switch((*it)->Type())
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
			const NFmiEsriPolyLine * elem = static_cast<const NFmiEsriPolyLine *>(*it);
			for(int part=0; part<elem->NumParts(); part++)
                {
                  int i1,i2;
                  i1 = elem->Parts()[part];     // start of part
                  if(part+1 == elem->NumParts())
                    i2 = elem->NumPoints()-1;       // end of part
                  else
                    i2 = elem->Parts()[part+1]-1;       // end of part
				  
                  if(i2>=i1)
                    {
					  double x1 = elem->Points()[i1].X();
					  double y1 = elem->Points()[i1].Y();

                      for(int i=i1+1; i<=i2; i++)
						{
						  double x2 = elem->Points()[i].X();
						  double y2 = elem->Points()[i].Y();
						  theCounts.Add(NFmiEdge(x1,y1,x2,y2,true));
						  x1 = x2;
						  y1 = y2;
						}
                    }
                }
			break;
		  }
		case kFmiEsriPolygon:
		case kFmiEsriPolygonM:
		case kFmiEsriPolygonZ:
		  {
			const NFmiEsriPolygon * elem = static_cast<const NFmiEsriPolygon *>(*it);
			for(int part=0; part<elem->NumParts(); part++)
                {
                  int i1,i2;
                  i1 = elem->Parts()[part];     // start of part
                  if(part+1 == elem->NumParts())
                    i2 = elem->NumPoints()-1;       // end of part
                  else
                    i2 = elem->Parts()[part+1]-1;       // end of part
				  
                  if(i2>=i1)
                    {
					  double x1 = elem->Points()[i1].X();
					  double y1 = elem->Points()[i1].Y();

                      for(int i=i1+1; i<=i2; i++)
						{
						  double x2 = elem->Points()[i].X();
						  double y2 = elem->Points()[i].Y();
						  theCounts.Add(NFmiEdge(x1,y1,x2,y2,true));
						  x1 = x2;
						  y1 = y2;
						}
                    }
                }
			break;
		  }
		}
	}
}

// ----------------------------------------------------------------------
/*!
 * \brief Insert the given path into the given shape as polylines
 */
// ----------------------------------------------------------------------

void path_to_shape(const NFmiPath & thePath, NFmiEsriShape & theShape)
{
  cout << "Converting path to shape..." << endl;

  const NFmiPathData::const_iterator begin = thePath.Elements().begin();
  const NFmiPathData::const_iterator end   = thePath.Elements().end();

  NFmiEsriPolyLine * polyline = 0;
  
  for(NFmiPathData::const_iterator it = begin; it != end; )
	{
	  bool doflush = false;
	  
	  if((*it).Oper()==kFmiMoveTo)
		doflush = true;
	  else if(++it==end)
		{
		  --it;
		  if(polyline == 0) polyline = new NFmiEsriPolyLine;
		  polyline->Add(NFmiEsriPoint((*it).X(),(*it).Y()));
		  doflush = true;
		}
	  else
		--it;

	  if(doflush)
		{
		  if(polyline != 0) theShape.Add(polyline);
		  polyline = new NFmiEsriPolyLine;
		}

	  polyline->Add(NFmiEsriPoint((*it).X(),(*it).Y()));
	  ++it;
	}
  
}				   

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape based on even occurrance count
 */
// ----------------------------------------------------------------------

const NFmiEsriShape * filter_even_count(const NFmiEsriShape & theShape)
{
  if(options.verbose)
	cout << "Filtering even numbered edges..." << endl;

  // Count the edges

  NFmiCounter<NFmiEdge> counts;
  count_edges(theShape,counts);

  // Build a path from even numbered edges

  NFmiEdgeTree tree;
  for(NFmiCounter<NFmiEdge>::const_iterator it = counts.begin();
	  it != counts.end();
	  ++it)
	{
	  if(it->second % 2 == 0)
		tree.Add(it->first);
	}

  if(options.verbose)
	cout << "Converting surviving edges to a path..." << endl;
  NFmiPath path = tree.Path();

  // Convert the tree to a shape

  NFmiEsriShape * shape = new NFmiEsriShape(kFmiEsriPolyLine);
  path_to_shape(path,*shape);

  return shape;
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape based on odd occurrance count
 */
// ----------------------------------------------------------------------

const NFmiEsriShape * filter_odd_count(const NFmiEsriShape & theShape)
{
  if(options.verbose)
	cout << "Filtering odd numbered edges..." << endl;

  // Count the edges

  NFmiCounter<NFmiEdge> counts;
  count_edges(theShape,counts);

  // Build a path from odd numbered edges

  NFmiEdgeTree tree;
  for(NFmiCounter<NFmiEdge>::const_iterator it = counts.begin();
	  it != counts.end();
	  ++it)
	{
	  if(it->second % 2 != 0)
		tree.Add(it->first);
	}

  if(options.verbose)
	cout << "Converting surviving edges to a path..." << endl;
  NFmiPath path = tree.Path();

  // Convert the tree to a shape
  // Note that polygons will remain polygons, otherwise we will
  // assume polyline output

  const NFmiEsriElementType oldtype = theShape.Type();
  NFmiEsriElementType newtype = oldtype;
  if(oldtype != kFmiEsriPolygon)
	newtype = kFmiEsriPolyLine;

  NFmiEsriShape * shape = new NFmiEsriShape(newtype);
  path_to_shape(path,*shape);

  return shape;
}

// ----------------------------------------------------------------------
/*!
 * \brief Filter the shape
 */
// ----------------------------------------------------------------------

const NFmiEsriShape * filter_shape(const NFmiEsriShape & theShape)
{
  if(options.filter_even_count)
	return filter_even_count(theShape);
  if(options.filter_odd_count)
  	return filter_odd_count(theShape);

  return &theShape;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program without error trapping
 */
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  // Initialize global settings

  NFmiSettings::Init();
  
  // Read the command line arguments

  parse_command_line(argc,argv);

  // Read the shapefile

  if(options.verbose)
	cout << "Reading input shapefile '"+options.input_shape+"'" << endl;

  NFmiEsriShape inputshape;
  if(!inputshape.Read(options.input_shape,true))
	throw runtime_error("Failed to read shape '"+options.input_shape+"'");

  // Process the shapefile

  if(options.verbose)
	cout << "Filtering..." << endl;
  const NFmiEsriShape * outputshape = filter_shape(inputshape);

  // And write it

  if(options.verbose)
	cout << "Writing output shapefile '"+options.output_shape+"'" << endl;
  outputshape->Write(options.output_shape);

  return 0;

}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 *
 * The main program is only an error trapping driver for domain
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  try
	{
	  return domain(argc,argv);
	}
  catch(const exception & e)
	{
	  cerr << "Error: Caught an exception:" << endl
		   << "--> " << e.what() << endl
		   << endl;
	  return 1;
	}
  catch(...)
	{
	  cerr << "Error: Caught an unknown exception" << endl;
	}
}
