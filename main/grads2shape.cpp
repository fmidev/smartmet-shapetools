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

#include "GradsTools.h"
#include <imagine/NFmiEsriShape.h>
#include <imagine/NFmiEsriPoint.h>
#include <imagine/NFmiEsriPolyLine.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiCmdLine.h>
#include <newbase/NFmiStringTools.h>
#include <fstream>
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
			  double lon = GradsTools::read_lon(in);
			  double lat = GradsTools::read_lat(in);
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
		  double lon1 = GradsTools::read_lon(in);
		  double lat1 = GradsTools::read_lat(in);
		  double lon2 = GradsTools::read_lon(in);
		  double lat2 = GradsTools::read_lat(in);
		  
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
