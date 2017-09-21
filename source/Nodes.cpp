// ======================================================================
/*!
 * \file Nodes.cpp
 * \brief Implementation details for Nodes class
 */
// ======================================================================
#ifdef _MSC_VER
#pragma warning(disable : 4786)  // poistaa n kpl VC++ kääntäjän varoitusta
                                 // (liian pitkä nimi >255 merkkiä joka johtuu
                                 // 'puretuista' STL-template nimistä)
#endif

// internal
#include "Nodes.h"
#include "Point.h"
// external
// system

using namespace std;

// ======================================================================
//			HIDDEN INTERNAL FUNCTIONS
// ======================================================================

// ======================================================================
//			METHOD IMPLEMENTATIONS
// ======================================================================

// ----------------------------------------------------------------------
/*!
 * Add a new numbered point to the container. If the point already
 * exists in the set, nothing actually happens. The number of the
 * added point is returned. If the point was already in the container,
 * the number returned is the number assigned previously to the point.
 */
// ----------------------------------------------------------------------

long Nodes::add(const Point &pt, long theId)
{
  unsigned long idx =
      itsData.insert(make_pair(pt, make_pair(itsData.size() + 1, theId))).first->second.first;
  if (idx > itsOrderedData.size()) itsOrderedData.push_back(pt);
  return idx;
}

// ----------------------------------------------------------------------
/*!
 * Return the unique ordinal assigned to the given point, or 0 if
 * the point has not been added into Nodes.
 */
// ----------------------------------------------------------------------

unsigned long Nodes::number(const Point &pt) const
{
  DataType::const_iterator pos = itsData.find(pt);
  if (pos == itsData.end())
    return 0;
  else
    return pos->second.first;
}

// ----------------------------------------------------------------------
/*!
 * Return the id number assigned to the given point, or 0 if
 * the point has not been added into Nodes.
 */
// ----------------------------------------------------------------------

long Nodes::id(const Point &pt) const
{
  DataType::const_iterator pos = itsData.find(pt);
  if (pos == itsData.end())
    return 0;
  else
    return pos->second.second;
}

// ----------------------------------------------------------------------
/*!
 * Return the Point with the given ordinal number.
 */
// ----------------------------------------------------------------------

Point Nodes::point(long ordinal) const
{
  if (ordinal <= 0 || static_cast<unsigned long>(ordinal) > itsOrderedData.size())
    return Point(0, 0);
  else
    return itsOrderedData[ordinal - 1];
}

// ======================================================================
