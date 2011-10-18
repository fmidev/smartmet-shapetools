// ======================================================================
/*!
 * \file Polygon.cpp
 * \brief Implementations of Polygon class methods.
 */
// ======================================================================

#include "Polygon.h"
#include <iostream>
#include <cstdlib>

using namespace std;

// ======================================================================
/*!
 * Force closure on the polygon.
 */
// ======================================================================

void Polygon::close(void) const
{
  if(!itsData.empty())
	if(itsData[0] != itsData[itsData.size()-1])
	  itsData.push_back(itsData[0]);
}

// ----------------------------------------------------------------------
/*!
 * Calculate the polygon area. If the polygon is empty, returns zero.
 * The formula is the very well known: 0.5*abs(sum(x[i]*y[i+1]-x[i+1]*y[i]))
 * with the condition that the last point is equal to the first one.
 */
// ----------------------------------------------------------------------

double Polygon::area(void) const
{
  close();
  if(itsData.size()<=2)
	return 0.0;
  double sum = 0;
  for(unsigned int i=0; i<itsData.size()-1; i++)
	sum += itsData[i].x()*itsData[i+1].y()-itsData[i+1].x()*itsData[i].y();
  return abs(0.5*sum);
}

// ----------------------------------------------------------------------
/*!
 * Calculate the cartographic area of the polygon.
 *
 * The formula is the very well known: 0.5*abs(sum(x[i]*y[i+1]-x[i+1]*y[i]))
 * with the condition that the last point is equal to the first one.
 *
 * All (lat/lon) coordinates are projected to the Lambert cylindrical
 * equal are projection before applying the formula. Extra care
 * is taken while crossing the 180 degree meridian, and when the polygon
 * encloses one of the cartographic poles.
 */
// ----------------------------------------------------------------------

double Polygon::geoarea(void) const
{
  // The last point must be the same as the first point, make sure it is
  close();

  // Safety against strange input
  if(itsData.size()<=2)
	return 0.0;

  // Angle constants for projection
  const double kpi = 3.14159265358979323/180;
  const double k90 = kpi*90;
  const double k360 = kpi*360;

  double sum = 0; // The accumulated area so far

  double dx1 = 0; // X-offsets for longitudes are multiples
  double dx2 = 0; // of 360 (in radians)

  double x1 = 0;	// the previous point is not defined yet
  double y1 = 0;

  for(unsigned int i=0; i<itsData.size(); i++)
	{
	  // Establish new current projected point
	  double x2 = kpi*itsData[i].x();
	  double y2 = sin(kpi*itsData[i].y());

	  // If we are at the second point, we can start calculating
	  if(i>0)
		{
		  // Update dx2 if the 180 meridian is crossed
		  if(x1<-k90 && x2>k90)
			dx2 -= k360;
		  else if(x1>k90 && x2<-k90)
			dx2 += k360;
		  
		  // Update area sum
		  sum += (x1+dx1)*y2-(x2+dx2)*y1;

		}
	  
	  // Current point becomes new previous point
	  dx1 = dx2;
	  x1 = x2;
	  y1 = y2;

	}

  // Fix things if we went around the pole by adding path
  // xn,yn -> xn,pole
  // xn,pole -> x1,pole
  // x1,pole -> x1,y1
  // where pole is +-90 depending on which pole we're nearest to

  if(dx2!=0)
	{
	  double x2 = x1;
	  double y2 = sin(y1<0 ? -k90 : k90);
	  sum += (x1+dx1)*y2-(x2+dx2)*y1;
	  x1 = x2, y1 = y2, dx1=dx2;
	  x2 = kpi*itsData[0].x();
	  sum += (x1+dx1)*y2-x2*y1;
	  x1 = x2, y1 = y2;
	  y2 = sin(kpi*itsData[0].y());
	  sum += x1*y2-x2*y1;
	}
  
  // And finally calculate the correct scaled result in square kilometers

  const double R = 6371.220;
  return R*R*abs(0.5*sum);

}

// ----------------------------------------------------------------------
/*!
 * Test whether the given point is inside the polygon. The algorithm
 * was easily found with google, and was simplied a bit by replacing
 * a sequence of if's with a single with many && conditions.
 *
 * Similar code can be found from NFmiSvgPath in newbase.
 */
// ----------------------------------------------------------------------

bool Polygon::isInside(const Point & pt) const
{
  close();

  if(itsData.size() <= 2)
	return false;

  // Saattaa nopeuttaa koodia, riippuu kääntäjästä:
  const double x = pt.x();
  const double y = pt.y();

  // Edellinen ja nykyinen piste
  double x1=0, y1=0, x2=0, y2=0;

  bool inside = false;

  const DataType::const_iterator begin = itsData.begin();
  const DataType::const_iterator end = itsData.end();
  for(DataType::const_iterator iter=begin; iter!=end; ++iter)
	{
	  x2 = iter->x();
	  y2 = iter->y();
	  if(iter!=begin)
		{
		  if(y > min(y1,y2) &&
			 y <= max(y1,y2) &&
			 x <= max(x1,x2) &&
			 y1!=y2 &&
			 (x1==x2 || x<(y-y1)*(x2-x1)/(y2-y1)+x1))
			inside = !inside;
		}
	  x1 = x2;
	  y1 = y2;
	}
  return inside;
}

// ----------------------------------------------------------------------
/*!
 * Find some point inside the given polygon. This is done by
 * examining the triangle formed by two consecutive edges at a time.
 * If the examined part of the polygon is convex, chance is that a random
 * point inside the triangle is also inside the polygon. If a test shows
 * this to not be the case, we try again at the next pair of consecutive
 * edges. The process repeats at the start of the polygon until a point
 * inside the polygon is found, or a set maximum number of iterations
 * is reached, in which case the code exits due to a fatal failure.
 * A more robust algorithm would guarantee finding a point, but alas
 * we cannot. In practise the algorithm seems to work exceedingly
 * well, since typical polygons have well behaved convex parts in them.
 *
 * To pick a point from within vertices p1,p2,p3 we can use
 * p = p1 + a1*(p2-p1)+(1-a1)*a2*(p3-p1)
 * This will however concentrate the points near p2. However, since
 * we cannot know whether the triangle is fully inside the polygon,
 * we can actually choose p2 to be the middle point of the 2 edges
 * known to belong to the triangle. In case the triangle is not
 * fully inside the polygon, this bias will aid in finding a point
 * more likely to be inside the polygon.
 *
 * Also, it may happen that 3 consecutive points are almost in a line.
 * In such cases numerical stability comes into play. To prevent this,
 * we require a reasonable shape index from each triangle being tested.
 * In the worst case this may prevent a point to be found, but this is
 * a small price to pay for strange effects caused by marking a point
 * as being inside within some marging close to rounding error.
 * The easiest way to test the shape is to require that 
 */
// ----------------------------------------------------------------------

Point Polygon::someInsidePoint(void) const
{
  close();
  if(itsData.size()<3)
	return Point(0,0);

  const long max_iterations = 10000;
  long iterations = 0;

  double shapelimit = 10;

  while(true)
	{
	  for(unsigned int i=0; i<itsData.size()-2; i++)
		{
		  if(++iterations > max_iterations)
			{
			  cerr << "Error: Could not find a point inside polygon" << endl;
			  exit(1);
			}

		  const double x1 = itsData[i].x();
		  const double y1 = itsData[i].y();
		  const double x2 = itsData[i+1].x();
		  const double y2 = itsData[i+1].y();
		  const double x3 = itsData[i+2].x();
		  const double y3 = itsData[i+2].y();

		  // lengths of the edges
		  const double a = sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
		  const double b = sqrt((x2-x3)*(x2-x3)+(y2-y3)*(y2-y3));
		  const double c = sqrt((x1-x3)*(x1-x3)+(y1-y3)*(y1-y3));
		  // perimeter
		  const double L = a+b+c;
		  // semiperimeter
		  const double s = 0.5*L;
		  // area
		  const double A = sqrt(s*(s-a)*(s-b)*(s-c));
		  // shape index
		  const double shape = L/sqrt(A);
		  
		  // if shape is too distorted, skip the triangle. To prevent
		  // too strict shape requirements, we gradually loosen the
		  // shape requirement
		  shapelimit *= 1.01;
		  if(shape>shapelimit)
			continue;

		  // We limit the scales to 0.2-0.8 to guarantee numerical stability
		  // For example a1=0.999x is known to have caused trouble

		  double a1 = 0.2+(0.8-0.2)*rand()/(RAND_MAX+1.0);
		  double a2 = 0.2+(0.8-0.2)*rand()/(RAND_MAX+1.0);

		  const double x = x1 + a1*(x2-x1)+(1-a1)*a2*(x3-x1);
		  const double y = y1 + a1*(y2-y1)+(1-a1)*a2*(y3-y1);

		  Point tmp(x,y);

		  if(isInside(tmp))
			return tmp;
		}
	}
}

// ======================================================================


