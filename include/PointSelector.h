// ======================================================================
/*!
 * \file
 * \brief Interface of class PointSelector
 */
// ======================================================================

#ifndef POINTSELECTOR_H
#define POINTSELECTOR_H

#include "boost/shared_ptr.hpp"
#include <list>

class NFmiArea;

class PointSelector
{
public:

  typedef std::list<int> IndexList;
  typedef IndexList::const_iterator const_iterator;

public:

  ~PointSelector();
  PointSelector(const NFmiArea & theArea);

  void minDistance(double theDistance);
  void boundingBox(double theX1, double theY1, double theX2, double theY2);

  bool add(int theID, double theValue, double theLon, double theLat);
  
  const_iterator begin() const;
  const_iterator end() const;
  
private:

  class Pimple;
  boost::shared_ptr<Pimple> itsPimple;

  PointSelector();
  PointSelector(const PointSelector & theSelector);
  PointSelector & operator=(const PointSelector & theSelector);

}; // class PointSelector

#endif // POINTSELECTOR_H

// ======================================================================
