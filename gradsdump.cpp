// ======================================================================
/*!
 * \file
 * \brief Dump GrADS map data to standard output
 */
// ======================================================================
/*!
 * \page gradsdump gradsdump
 *
 * gradsdump takes as input a GrADS map data file and display
 * its contents on standard output
 *
 * Usage:
 * \code
 * gradsdump <mapfile>
 * \endcode
 */
// ----------------------------------------------------------------------

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

  if(cmdline.NumberofParameters() != 1)
	throw runtime_error("Expecting one command line argument");

  const string gradsfile = cmdline.Parameter(1);

  if(gradsfile.empty())
	throw runtime_error("The name of the GrADS file is empty");

  // Dump the GrADS file

  ifstream in(gradsfile.c_str(), ios::in|ios::binary);
  if(!in)
	throw runtime_error("Failed to open '"+gradsfile+"' for reading");

  unsigned char record_type;
  unsigned char record_level;
  unsigned char record_points;

  while(in >> noskipws >> record_type >> record_level >> record_points)
	{
	  cout << "# Record type = " << static_cast<int>(record_type) << endl;
	  if(record_type == 1)
		{
		  cout << "# Record level = "
			   << static_cast<int>(record_level)
			   << endl
			   << "# Record size = "
			   << static_cast<int>(record_points)
			   << endl;

		  for(unsigned int i=0; i<record_points; i++)
			{
			  double lon = read_lon(in);
			  double lat = read_lat(in);
			  cout << lon << '\t' << lat << endl;
			}
		}
	  else if(record_type == 2)
		{
		  cout << "# Record start level = "
			   << static_cast<int>(record_level)
			   << endl
			   << "# Record end level = "
			   << static_cast<int>(record_points)
			   << endl;

		  unsigned char ch1,ch2,ch3,ch4;
		  in >> noskipws >> ch1 >> ch2 >> ch3 >> ch4;
		  int length = ( (ch1<<24) | (ch2<<16) | (ch3<<8) | ch4);
		  double lon1 = read_lon(in);
		  double lat1 = read_lat(in);
		  double lon2 = read_lon(in);
		  double lat2 = read_lat(in);
		  
		  cout << "# Record size = " << length << endl
			   << "# BBox bottom left = " << lon1 << ' ' << lat1 << endl
			   << "# BBox top right = " << lon2 << ' ' << lat2 << endl;
		}
	  else
		throw runtime_error("Record type "+NFmiStringTools::Convert(static_cast<int>(record_type))+" is unknown");

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
	  cerr << "Error: gradsdump failed due to" << endl
		   << "--> " << e.what() << endl;
	}
  catch(...)
	{
	  cerr << "Error: gradsdump failed due to an unknown exception" << endl;
	}
  return 1;
}
