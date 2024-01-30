// ======================================================================
/*!
 * \file Polygon.h
 * \brief Declaration of a Polygon container
 */
// ======================================================================
/*!
 * \class Polygon
 *
 * A class to hold a simple non-self intersecting polygon as defined
 * by a sequence of points.
 *
 * The basic methods of the class include testing Point insidedness
 * and actually finding a Point inside the Polygon itself.
 */
// ======================================================================

#ifndef POLYGON_H
#define POLYGON_H

// internal
#include "Point.h"
// external
// system
#include <vector>

//! A simple polygon container

class Polygon
{
 public:
  //! Destructor
  ~Polygon() {}

  //! Default constructor
  Polygon() {}

  //! Adding a new point to the polygon
  void add(const Point &pt) { itsData.push_back(pt); }

  //! Test if the polygon is empty
  bool empty() const { return itsData.empty(); }

  //! Empty the polygon
  void clear() { itsData.clear(); }

  //! Calculate the area of the polygon
  double area() const;

  //! Calculate the cartographic area of the polygon
  double geoarea() const;

  //! Test whether the given point is inside the polygon
  bool isInside(const Point &thePoint) const;

  //! Find some point inside the polygon
  Point someInsidePoint() const;

  //! The actual type of the contained data
  typedef std::vector<Point> DataType;

  //! Return the data itself

  const DataType &data() const { return itsData; }

 private:
  //! Close the polygon by making sure the last point is equal to the first
  //! point
  void close() const;

  //! The actual data is mutable, since we want close to be const
  mutable DataType itsData;

};  // class Polygon

#endif  // class Polygon

// ======================================================================
