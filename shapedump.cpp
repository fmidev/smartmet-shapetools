// ======================================================================
/*!
 * \file
 * \brief A program to dump shapefile contents
 */
// ======================================================================
/*!
 * \page shapedump shapedump
 *
 * shapedump takes as input a single filename indicating a set
 * of ESRI shapefiles and dumps the coordinates to the screen.
 *
 * For example
 * \code
 * > shapedump suomi > suomi.dat
 * \endcode
 */
// ======================================================================
#ifdef _MSC_VER
#pragma warning(disable : 4786) // poistaa n kpl VC++ kääntäjän varoitusta (liian pitkä nimi >255 merkkiä joka johtuu 'puretuista' STL-template nimistä)
#endif

#include "NFmiGeoShape.h"
#include "NFmiPath.h"
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

int main(int argc, char * argv[])
{

  // Command line arguments
  if(argc!=2)
	{
	  cerr << "Usage: shapedump [shapefile]" << endl;
	  return 1;
	}
  string shapefile = argv[1];

  int shapenum = 0;
  int linenum = 0;
  try
	{
	  NFmiGeoShape geo(shapefile,kFmiGeoShapeEsri);
	  NFmiPath path = geo.Path();

	  const NFmiPathData::const_iterator begin = path.Elements().begin();
	  const NFmiPathData::const_iterator end = path.Elements().end();

	  for(NFmiPathData::const_iterator it=begin; it!=end; ++it)
		{
		  if(it->Oper()==kFmiMoveTo)
			{
			  shapenum++;
			  linenum = 0;
			}
		  linenum++;
		  cout << shapenum << '\t'
			   << linenum << '\t'
			   << std::setiosflags(std::ios::fixed)
			   << it->X() << '\t'
			   << it->Y() << endl;
		}
	  
	}
  catch(exception & e)
	{
	  cerr << "Error: shape2dump failed" << endl
		   << " --> " << e.what() << endl;
	  return 1;
	}
  return 0;
}
