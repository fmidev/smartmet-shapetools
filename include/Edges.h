// ======================================================================
/*!
 * \file Edges.h
 * \brief Contains declaration of Edges
 */
// ======================================================================
/*!
 * \class Edges
 *
 * Edges is a set of unique edges.
 */
// ======================================================================

#ifndef EDGES_H
#define EDGES_H

#include "Edge.h"
#include <set>

//! Edges maintains a set of unique edges
class Edges
{
public:
  //! Destructor
  ~Edges(void) { }

  //! Default constructor
  Edges(void) { }

  //! The type of the contained data
  typedef std::set<Edge> DataType;

  //! Adding a new edge to the set returns true if the edge is new
  bool add(const Edge & edge)
  { return itsData.insert(edge).second; }

  //! Testing if the given edge is in the set
  bool contains(const Edge & edge) const
  { return (itsData.find(edge) != itsData.end()); }

private:

  //! The actual data
  DataType itsData;

}; // class Edges

#endif // EDGES_H

// ======================================================================
