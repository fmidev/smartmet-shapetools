// ======================================================================
/*!
 * \file Point.h
 * \brief Declaration of a point suitable for ordered containers.
 */
// ======================================================================
/*!
 * \class Point
 *
 * newbase NFmiPoint is not suitable for ordered containers, which
 * are a must in GIS applications. This is a substitute with the
 * proper operations defined. Also, construction from a NFmiPoint
 * is enabled to allow for direct substitution.
 */
// ======================================================================

#ifndef POINT_H
#define POINT_H

#include "NFmiPoint.h"

//! A simple point class

class Point
{
public:
  //! Destructor
  ~Point(void) { }
  //! Constructor
  Point(double theX=0, double theY=0) : itsX(theX), itsY(theY) { }
  //! Copy constructor
  Point(const Point & pt) : itsX(pt.itsX), itsY(pt.itsY) { }
  //! Construction from NFmiPoint
  Point(const NFmiPoint & pt) : itsX(pt.X()), itsY(pt.Y()) { }
  //! Assignment operator
  Point & operator=(const Point & pt)
  { if(this!=&pt) { itsX=pt.itsX; itsY=pt.itsY; } return *this; }
  //! Return x-coordinate
  double x(void) const { return itsX; }
  //! Return y-coordinate
  double y(void) const { return itsY; }
  //! Set x-coordinate
  void x(double value) { itsX = value; }
  //! Set y-coordinate
  void y(double value) { itsY = value; }
  //! Equality comparison
  bool operator==(const Point & pt) const
  { return (itsX==pt.itsX && itsY==pt.itsY); }
  //! Inequality comparison
  bool operator!=(const Point & pt) const
  { return (itsX!=pt.itsX || itsY!=pt.itsY); }
  //! Lexical comparison
  bool operator<(const Point & pt) const
  { return (itsX!=pt.itsX ? itsX<pt.itsX : itsY<pt.itsY); }
  //! Euclidian distance between two points
  double distance(const Point & pt) const
  { return sqrt((x()-pt.x())*(x()-pt.x())+(y()-pt.y())*(y()-pt.y())); }
  //! Cartographic distance between two points
  double geodistance(const Point & pt) const;

private:

  double itsX;		//!< The x-coordinate
  double itsY;		//!< The y-coordinate

}; // class Point

#endif // POINT_H

// ======================================================================

 
