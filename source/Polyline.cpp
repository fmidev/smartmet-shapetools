// ======================================================================
/*!
 * \file Polyline.cpp
 * \brief Implementation details for Polyline class
 */
// ======================================================================

#include "Polyline.h"
#include "NFmiValueString.h"

using namespace std;

// ======================================================================
//				HIDDEN INTERNAL FUNCTIONS
// ======================================================================

namespace
{
  //! The number identifying the region within the rectangle
  const int central_quadrant = 4;

  //! Test the position of given point with respect to a rectangle.
  int quadrant(double x, double y,
			   double x1, double y1,
			   double x2, double y2,
			   double margin)
  {
	int value = central_quadrant;
	if(x<x1-margin)
	  value--;
	else if(x>x2+margin)
	  value++;
	if(y<y1-margin)
	  value -= 3;
	else if(y>y2+margin)
	  value += 3;
	return value;
  }
  
  //! Test whether two rectangles intersect
  bool intersects(double x1, double y1, double x2, double y2,
				  double X1, double Y1, double X2, double Y2)
  {
	bool xoutside = (x1>X2 || x2<X1);
	bool youtside = (y1>Y2 || y2<Y1);
	return (!xoutside && !youtside);
  }

}; // anonymous namespace

// ======================================================================
//				METHOD IMPLEMENTATIONS
// ======================================================================

// ----------------------------------------------------------------------
/*!
 * Return a string representation of the polyline using the given
 * strings for path movement operations.
 *
 * \todo Remove dependency on NFmiValueString once g++ supports
 *       setprecision better. Full accuracy is needed.
 */
// ----------------------------------------------------------------------

string Polyline::path(const string & moveto,
					  const string & lineto,
					  const string & closepath) const
{
  string output = "";
  unsigned int n = size()-1;
  bool isclosed = (size()>1 && itsPoints[0]==itsPoints[n]);
  for(unsigned int i=0; i<=n; i++)
	{
	  if(isclosed && i==n && !closepath.empty())
		output += closepath;
	  else
		{
		  output += NFmiValueString(itsPoints[i].x());
		  output += ' ';
		  output += NFmiValueString(itsPoints[i].y());
		  output += ' ';
		  output += (i==0 ? moveto : lineto);
		}
	  output += '\n';
	}
  return output;
}

// ----------------------------------------------------------------------
/*!
 * Clip the polyline against the given rectangle. The result will contain
 * only points in the original polyline, no intersections are ever calculated.
 */
// ----------------------------------------------------------------------

void Polyline::clip(double x1, double y1, double x2, double y2, double margin)
{
  if(empty())
	return;

  DataType newpts;

  int last_quadrant = 0;
  int this_quadrant = 0;

  // Initialize the new bounding box
  double minx = itsPoints[0].x();
  double miny = itsPoints[0].y();
  double maxx = itsPoints[0].x();
  double maxy = itsPoints[0].y();

  for(unsigned int i=0; i<size(); i++)
	{
	  bool pushed = true;
	  if(i==0)
		newpts.push_back(itsPoints[i]);
	  else
		{
		  this_quadrant = quadrant(itsPoints[i].x(),itsPoints[i].y(),x1,y1,x2,y2,margin);
		  if(this_quadrant==central_quadrant)
			newpts.push_back(itsPoints[i]);
		  else if(this_quadrant != last_quadrant)
			newpts.push_back(itsPoints[i]);
		  else
			pushed = false;
		  last_quadrant = this_quadrant;
		}
	  // Update bounding box if point was accepted
	  if(pushed)
		{
		  minx = min(minx,itsPoints[i].x());
		  miny = min(miny,itsPoints[i].y());
		  maxx = max(maxx,itsPoints[i].x());
		  maxy = max(maxy,itsPoints[i].y());
		}
	}
  
  // If the bounding boxes don't intersect, output nothing, otherwise
  // replace old points with clipped ones

  if(newpts.size()<=1 ||
	 !intersects(minx,miny,maxx,maxy,x1-margin,y1-margin,x2+margin,y2+margin))
	itsPoints.clear();
  else
	itsPoints = newpts;
}

// ======================================================================

