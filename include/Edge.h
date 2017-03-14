// ======================================================================
/*!
 * \file Edge.h
 * \brief Contains declaration of Edge class
 */
// ======================================================================
/*!
 * \class Edge
 *
 * An egde is a pair of indices which uniquely identify some point.
 * The direction of the edge is immaterial.
 */
// ======================================================================

#ifndef EDGE_H
#define EDGE_H

//! An edge is a pair of unordered integer indices.
class Edge {
public:
  //! Destructor
  ~Edge(void) {}

  //! Constructor from the given indices
  Edge(long idx1, long idx2)
      : itsIndex1(idx1 < idx2 ? idx1 : idx2),
        itsIndex2(idx1 > idx2 ? idx1 : idx2) {}

  //! Equality comparison
  bool operator==(const Edge &edge) const {
    return (itsIndex1 == edge.itsIndex1 && itsIndex2 == edge.itsIndex2);
  }

  //! Lexical less-than comparison
  bool operator<(const Edge &edge) const {
    return ((itsIndex1 != edge.itsIndex1) ? (itsIndex1 < edge.itsIndex1)
                                          : (itsIndex2 < edge.itsIndex2));
  }

private:
  //! Default constructor is disabled
  Edge(void);

  //! The smaller of the indices
  long itsIndex1;

  //! The larger of the indices
  long itsIndex2;

}; // class Edge

#endif // EDGE_H

// ======================================================================
