// ======================================================================
/*!
 * \file
 * \brief Interface of class PointSelector
 */
// ======================================================================

#ifndef POINTSELECTOR_H
#define POINTSELECTOR_H

#include <memory>
#include <list>

class NFmiArea;

class PointSelector
{
 public:
  typedef std::list<int> IndexList;
  typedef IndexList::const_iterator const_iterator;
  typedef IndexList::size_type size_type;

 public:
  ~PointSelector();
  PointSelector(const NFmiArea &theArea, bool theNegateFlag = false);

  void minDistance(double theDistance);
  void boundingBox(double theX1, double theY1, double theX2, double theY2);

  bool add(int theID, double theValue, double theLon, double theLat);

  bool empty() const;
  size_type size() const;

  const_iterator begin() const;
  const_iterator end() const;

 private:
  class Pimple;
  std::shared_ptr<Pimple> itsPimple;

  PointSelector();
  PointSelector(const PointSelector &theSelector);
  PointSelector &operator=(const PointSelector &theSelector);

};  // class PointSelector

#endif  // POINTSELECTOR_H

// ======================================================================
