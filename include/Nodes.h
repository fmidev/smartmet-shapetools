// ======================================================================
/*!
 * \file Nodes.h
 * \brief Declaration of a node container.
 */
// ======================================================================
/*!
 * \class Nodes
 *
 * Nodes is a container for unique Points, which are assigned unique ordinals
 * ranging from 1 to N, the number of unique Points, as well as an unique
 * number based on the polyline or polygon containing the Point. The latter
 * number is decided externally during construction, the former during addition
 * into the container.
 *
 * The class provides O(log N) access to the ordinal and id of the given point
 * and linear time access to the point with the given ordinal.
 */
// ======================================================================

#ifndef NODES_H
#define NODES_H

// internal
#include "Point.h"
// external
// system
#include <map>
#include <vector>

//! Nodes is a collection of uniquely numbered points
class Nodes
{
 public:
  //! Destructor
  ~Nodes(void) {}

  //! Default constructor
  Nodes(void) {}

  //! Add a point, returning the ordinal of the point
  long add(const Point &pt, long theId = 0);

  //! Return the ordinal of the given point
  unsigned long number(const Point &pt) const;

  //! Return the id assigned to the given point
  long id(const Point &pt) const;

  //! Return the point with the given ordinal
  Point point(long ordinal) const;

  //! The container holds a map of Points with values number,id
  typedef std::map<Point, std::pair<unsigned long, long>> DataType;

  //! Return the data
  const DataType &data(void) const { return itsData; }

 private:
  //! Copy constructor is disabled
  Nodes(const Nodes &theNodes);

  //! Assignment is disabled
  Nodes &operator=(const Nodes &theNodes);

  //! The actual data
  DataType itsData;

  //! An extra ordinal sorted container
  std::vector<Point> itsOrderedData;

};  // class Nodes

#endif  // NODES_H

// ======================================================================
