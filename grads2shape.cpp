// ======================================================================
/*!
 * \file
 * \brief Convert a GrADS map data file to an ESRI shapefile
 */
// ======================================================================
/*!
 * \page grads2shape grads2shape
 *
 * grads2shape takes as input a GrADS map data file and produces
 * an ESRI shape file (polyline)
 *
 * Usage:
 * \code
 * grads2shape <mapfile> <shape>
 * \endcode
 */
// ----------------------------------------------------------------------

// imagine
#include "NFmiEsriShape.h"
#include "NFmiEsriPoint.h"
#include "NFmiEsriPolyLine.h"
#include "NFmiPoint.h"

// newbase
#include "NFmiCmdLine.h"
#include "NFmiStringTools.h"

// system
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;


// ----------------------------------------------------------------------
/*!
 * \brief Read a 3-byte integer from the input stream
 */
// ----------------------------------------------------------------------

int read_int(istream & in)
{
  unsigned char ch1, ch2, ch3;
  in >> noskipws >> ch1 >> ch2 >> ch3;
  return ((ch1<<16) | (ch2<<8) | ch3);
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a longitude from the input stream
 */
// ----------------------------------------------------------------------

double read_lon(istream & in)
{
  int value = read_int(in);
  double lon = (value/1e4);
  if(lon >= 180) lon -= 360;
  return lon;
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a latitude from the input stream
 */
// ----------------------------------------------------------------------

double read_lat(istream & in)
{
  int value = read_int(in);
  double lat = value/1e4 - 90;
  return lat;
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

  const string gradsfile = cmdline.Parameter(1);
  const string shapename = cmdline.Parameter(2);

  if(gradsfile.empty())
	throw runtime_error("The name of the GrADS file is empty");

  if(shapename.empty())
	throw runtime_error("The name of the shape is empty");

  // Read the GrADS file into a shape

  Imagine::NFmiEsriShape shp(Imagine::kFmiEsriPolyLine);

  ifstream in(gradsfile.c_str(), ios::in|ios::binary);
  if(!in)
	throw runtime_error("Failed to open '"+gradsfile+"' for reading");

  unsigned char record_type;
  unsigned char record_level;
  unsigned char record_points;

  int counter = 0;
  while(in >> noskipws >> record_type >> record_level >> record_points)
	{
	  if(record_type == 1)
		{
		  Imagine::NFmiEsriPolyLine * line =
			new Imagine::NFmiEsriPolyLine(counter);
		  for(unsigned int i=0; i<record_points; i++)
			{
			  double lon = read_lon(in);
			  double lat = read_lat(in);
			  line->Add(Imagine::NFmiEsriPoint(lon,lat));
			}
		  shp.Add(line);
		  ++counter;
		}
	  else if(record_type == 2)
		{
		  unsigned char ch1,ch2,ch3,ch4;
		  in >> noskipws >> ch1 >> ch2 >> ch3 >> ch4;
		  int length = ( (ch1<<24) | (ch2<<16) | (ch3<<8) | ch4);
		  double lon1 = read_lon(in);
		  double lat1 = read_lat(in);
		  double lon2 = read_lon(in);
		  double lat2 = read_lat(in);
		  
		  cout << "Skipping skip record with bbox: "
			   << lon1 << ',' << lat1 << " ... "
			   << lon2 << ',' << lat2
			   << " and length " << length << endl;
		}
	  else
		throw runtime_error("Record type "+NFmiStringTools::Convert(static_cast<int>(record_type))+" is unknown");

	}

  string filename = shapename + ".shp";
  if(!shp.WriteSHP(filename))
	throw runtime_error("Failed to write '"+filename+"'");

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
	  cerr << "Error: grads2shape failed due to" << endl
		   << "--> " << e.what() << endl;
	}
  catch(...)
	{
	  cerr << "Error: grads2shape failed due to an unknown exception" << endl;
	}
  return 1;
}