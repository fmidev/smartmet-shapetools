// ======================================================================
/*!
 * \file
 * \brief Convert a shapefile to a GrADS map data file
 */
// ======================================================================
/*!
 * \page gshhs2grads gshhs2grads
 *
 * gshhs2grads takes as input a GSHHS shoreline database,
 * the desired data type number and produces a map data in GrADS format.
 *
 * Usage:
 * \code
 * gshhs2grads <n> <gshhsfile>
 * \endcode
 *
 */
// ----------------------------------------------------------------------

// imagine
#include "NFmiGshhsTools.h"
#include "NFmiPath.h"

// newbase
#include "NFmiCmdLine.h"
#include "NFmiStringTools.h"

// self
#include "GradsTools.h"

// system
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief The main driver
 */
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  // Process the command line

  NFmiCmdLine cmdline(argc,argv,"");

  if(cmdline.Status().IsError())
	throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if(cmdline.NumberofParameters() != 2)
	throw runtime_error("Expecting two command line arguments");

  const int level = NFmiStringTools::Convert<int>(cmdline.Parameter(1));
  const string gshhsfile = cmdline.Parameter(2);

  if(level<0 || level>255)
	throw runtime_error("The level parameter must be in range 0-255");

  if(gshhsfile.empty())
	throw runtime_error("The name of the gshhsfile is empty");

  // Read the GSHHS data

  Imagine::NFmiPath path(Imagine::NFmiGshhsTools::ReadPath(gshhsfile,
														   -180,
														   -90,
														   +180,
														   +90));

  // Output the data

  vector<NFmiPoint> buffer;

  for(Imagine::NFmiPathData::const_iterator it = path.Elements().begin();
	  it != path.Elements().end();
	  ++it)
	{
	  switch((*it).Oper())
		{
		case Imagine::kFmiMoveTo:
		  GradsTools::print_line(cout,level,buffer);
		  buffer.clear();
		  // fallthrough
		case Imagine::kFmiLineTo:
		  buffer.push_back(NFmiPoint((*it).X(),(*it).Y()));
		  break;
		case Imagine::kFmiGhostLineTo:
		case Imagine::kFmiConicTo:
		case Imagine::kFmiCubicTo:
		  throw runtime_error("The shapefile contains Bezier curve segments");
		}
	}
  GradsTools::print_line(cout,level,buffer);

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  try
	{
	  return domain(argc,argv);
	}
  catch(runtime_error & e)
	{
	  cerr << "Error: gshhs2grads failed due to" << endl
		   << "--> " << e.what() << endl;
	}
  catch(...)
	{
	  cerr << "Error: gshhs2grads failed due to an unknown exception" << endl;
	}
  return 1;
}
