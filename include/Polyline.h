// ======================================================================
/*!
 * \file Polyline.h
 * \brief Contains declaration of a sequence of points.
 */
// ======================================================================
/*!
 * \class Polyline
 * 
 * Operations on the polyline include clipping it and returning a string
 * representation of the polyline with user specified moveto, lineto and
 * closepath operations.
 *
 * Note that a polygon is simply a special case of a polyline in which
 * the first and last points are equal. The type is detected automatically
 * for example when calling the path method.
 *
 */
// ======================================================================

#ifndef POLYLINE_H
#define POLYLINE_H

#include "Point.h"
#include <string>
#include <vector>

//! A simple polyline container

class Polyline
{
public:

  //! Default destructor
  ~Polyline(void) { }

  //! Default constructor
  Polyline(void) { }

  //! Return the size of the polyline
  unsigned int size(void) const { return itsPoints.size(); }

  //! Test whether the polyline is empty
  bool empty(void) const { return itsPoints.empty(); }

  //! Clear the polyline
  void clear(void) { itsPoints.clear(); }

  //! Add a point to the polyline
  void add(double x, double y) { itsPoints.push_back(Point(x,y)); }

  //! Add a point to the polyline
  void add(const Point & thePoint) { itsPoints.push_back(thePoint); }

  //! Clip the polyline with the given rectangle
  void clip(double x1, double y1, double x2, double y2, double margin=0);

  //! Clip the polyline with the given bounding box
  void clip(const Point & lowleft, const Point & topright, double margin=0)
  { clip(lowleft.x(),lowleft.y(),topright.x(),topright.y(),margin); }

  //! Return a string representation of the polyline
  std::string path(const std::string & moveto,
				   const std::string & lineto,
				   const std::string & closepath="") const;

private:

  typedef std::vector<Point> DataType;

  //! The actual data container
  DataType itsPoints;

}; // class Polyline

#endif // POLYLINE_H

// ======================================================================
