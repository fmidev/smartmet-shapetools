// ======================================================================
/*!
 * \file
 * \brief Convert a shapefile to a GrADS map data file
 */
// ======================================================================
/*!
 * \page shape2grads shape2grads
 *
 * shape2grads takes as input a shapefile, the desired data type
 * number and produces a map data in GrADS format.
 *
 * Usage:
 * \code
 * shape2grads <n> <shapefile>
 * \endcode
 *
 * Since GrADS files can be concatenated, shape2grads can
 * be used as follows
 * \code
 * shape2grads 1 suomi > skandinavia.grads
 * shape2grads 1 ruotsi >> skandinavia.grads
 * shape2grads 1 norja >> skandinavia.grads
 * shape2grads 1 tanska >> skandinavia.grads
 * \endcode
 *
 */
// ----------------------------------------------------------------------

// imagine
#include "NFmiGeoShape.h"
#include "NFmiPath.h"

// newbase
#include "NFmiCmdLine.h"
#include "NFmiStringTools.h"

// system
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief Print coodinate as 3-byte integer
 */
// ----------------------------------------------------------------------

void print_double(double theValue)
{
  int value = static_cast<int>(theValue*1e4+0.5);

  cout << static_cast<unsigned char>( (value >> 16) & 0xFF);
  cout << static_cast<unsigned char>( (value >> 8) & 0xFF);
  cout << static_cast<unsigned char>( (value >> 0) & 0xFF);
}

// ----------------------------------------------------------------------
/*!
 * \brief Print longitude
 */
// ----------------------------------------------------------------------

void print_lon(double theLon)
{
  // Make sure value is in range [0-360[
  while(theLon < 0)
	theLon += 360;
  while(theLon >= 360)
	theLon -= 360;
  print_double(theLon);
}

// ----------------------------------------------------------------------
/*!
 * \brief Print latitude
 */
// ----------------------------------------------------------------------

void print_lat(double theLat)
{
  // Make sure value is in range [0-180[
  while(theLat < 0)
	theLat += 180;
  while(theLat >= 180)
	theLat -= 180;
  print_double(theLat);
}

// ----------------------------------------------------------------------
/*!
 * \brief Print out a sequence of lineto commands
 *
 * Each record header consists of
 *
 *  - Byte 0: 1
 *  - Byte 1: The type of the record
 *  - Byte 2: Number of points in the record
 *  - Bytes 3-N: Pairs of lon-lat values
 */
// ----------------------------------------------------------------------

void tograds(int theLevel, const vector<NFmiPoint> & thePoints)
{
  if(thePoints.empty())
	return;

  unsigned int pos1=0;
  while(pos1 < thePoints.size())
	{
	  unsigned int pos2 = min(pos1+255,thePoints.size()-1);
	  if(pos1==pos2)
		break;

	  // Byte 0:
	  cout << static_cast<unsigned char>(1);
	  // Byte 1:
	  cout << static_cast<unsigned char>(theLevel);
	  // Byte 2:
	  cout << static_cast<unsigned char>(pos2-pos1+1);
	  // The coordinates
	  for(unsigned int i=pos1; i<=pos2; i++)
		{
		  print_lon(thePoints[i].X());
		  print_lat(thePoints[i].Y());
		}
	  // Note! no +1, we start from where we left off!
	  pos1 = pos2;
	}
}

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
  const string shapefile = cmdline.Parameter(2);

  if(level<0 || level>255)
	throw runtime_error("The level parameter must be in range 0-255");

  if(shapefile.empty())
	throw runtime_error("The name of the shapefile is empty");

  // Read the shapefile (only SHP, DBF is not needed)

  Imagine::NFmiGeoShape shp(shapefile, Imagine::kFmiGeoShapeEsri);
  Imagine::NFmiPath path = shp.Path();

  // Output the data

  vector<NFmiPoint> buffer;

  for(Imagine::NFmiPathData::const_iterator it = path.Elements().begin();
	  it != path.Elements().end();
	  ++it)
	{
	  switch((*it).Oper())
		{
		case Imagine::kFmiMoveTo:
		  tograds(level,buffer);
		  buffer.clear();
		  break;
		case Imagine::kFmiLineTo:
		  buffer.push_back(NFmiPoint((*it).X(),(*it).Y()));
		  break;
		case Imagine::kFmiGhostLineTo:
		case Imagine::kFmiConicTo:
		case Imagine::kFmiCubicTo:
		  throw runtime_error("The shapefile contains Bezier curve segments");
		}
	}

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
	  cerr << "Error: shape2grads failed due to" << endl
		   << "--> " << e.what() << endl;
	}
  catch(...)
	{
	  cerr << "Error: shape2grads failed due to an unknown exception" << endl;
	}
  return 1;
}
