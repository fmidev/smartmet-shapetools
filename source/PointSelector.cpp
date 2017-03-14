// ======================================================================
/*!
 * \file
 * \brief Implementation of class PointSelector
 */
// ======================================================================
/*!
 * \brief Class PointSelector
 *
 * Basic usage:
 * \code
 * NFmiArea area = NFmiAreaFactory::Create("stereographic,...");
 * PointSelector selector(area);
 * selector.minDistance(20);
 *
 * selector.add(1,224400,24,60);	// Espoo
 * selector.add(2,574700,25,60);	// Helsinki
 * selector.add(3,122900,23,67);	// Oulu
 * selector.add(4,199200,23,62);	// Tampere
 * selector.add(5,176600,22,60);	// Turku
 * selector.add(6,186000,25,61);	// Vantaa
 *
 * for(PointSelector::const_iterator it = selector.begin();
 *     it != selector.end();
 *     ++it)
 * {
 *   cout << *it << endl;
 * }
 * \endcode
 *
 * The output order would be
 * \code
 * 2	// Helsinki
 * 1	// Espoo
 * 4	// Tampere
 * 6	// Vantaa
 * 5	// Turku
 * 3	// Oulu
 * \endcode
 * with possibly some points left out. The first candidates
 * for removal would be Espoo and Vantaa since they are very
 * close to Helsinki. Whether or not the points would be removed
 * would depend on the details of the projection.
 *
 */
// ======================================================================


#include "PointSelector.h"
#include <newbase/NFmiArea.h>
#include <newbase/NFmiNearTree.h>
#include <map>
#include <stdexcept>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief Data holder for a point
 */
// ----------------------------------------------------------------------

struct PointData
{
  int id;
  double x;
  double y;

  PointData(int theID, double theX, double theY)
	: id(theID), x(theX), y(theY)
  { }

};

// ----------------------------------------------------------------------
/*!
 * \brief Implementation hiding pimple
 */
// ----------------------------------------------------------------------

class PointSelector::Pimple
{
public:
  double itsMinDistance;
  double itsX1;
  double itsY1;
  double itsX2;
  double itsY2;
private:
  typedef multimap<double,PointData,greater<double> > Candidates;
  const NFmiArea & itsArea;
  const bool itsNegateFlag;
  mutable bool itsReduced;
  mutable Candidates itsCandidates;
  mutable IndexList itsResults;
  void reduce() const;
public:
  Pimple(const NFmiArea & theArea, bool theNegateFlag);
  bool add(int theID, double theValue, double theLon, double theLat);
  bool empty() const;
  size_type size() const;
  const_iterator begin() const;
  const_iterator end() const;

}; // class PointSelector::Pimple

// ----------------------------------------------------------------------
/*!
 * \brief Pimple constructor
 *
 * \param theArea The reference projection area
 * \param theNegateFlag True, if shape field values are to be negated
 */
// ----------------------------------------------------------------------

PointSelector::Pimple::Pimple(const NFmiArea & theArea, bool theNegateFlag)
  : itsMinDistance(10)
  , itsX1(theArea.Left())
  , itsY1(theArea.Top())
  , itsX2(theArea.Right())
  , itsY2(theArea.Bottom())
  , itsArea(theArea)
  , itsNegateFlag(theNegateFlag)
  , itsReduced(true)		// empty selector is in reduced state
  , itsCandidates()
  , itsResults()
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Pimple empty() test
 */
// ----------------------------------------------------------------------

bool PointSelector::Pimple::empty() const
{
  reduce();
  return itsResults.empty();
}

// ----------------------------------------------------------------------
/*!
 * \brief Pimple size() method
 */
// ----------------------------------------------------------------------

PointSelector::size_type PointSelector::Pimple::size() const
{
  reduce();
  return itsResults.size();
}

// ----------------------------------------------------------------------
/*!
 * \brief Pimple begin const_iterator
 */
// ----------------------------------------------------------------------

PointSelector::const_iterator PointSelector::Pimple::begin() const
{
  reduce();
  return itsResults.begin();
}

// ----------------------------------------------------------------------
/*!
 * \brief Pimple end const_iterator
 */
// ----------------------------------------------------------------------

PointSelector::const_iterator PointSelector::Pimple::end() const
{
  reduce();
  return itsResults.end();
}

// ----------------------------------------------------------------------
/*!
 * \brief Add a new candidate point
 *
 * \param theID The unique ID of the point
 * \param theValue The value used for sorting the points
 * \param theLon The longitude
 * \param theLat The latitude
 * \return True, if the point was accepted as a candidate
 */
// ----------------------------------------------------------------------

bool PointSelector::Pimple::add(int theID,
								double theValue,
								double theLon,
								double theLat)
{
  // Convert to image points
  NFmiPoint xy = itsArea.ToXY(NFmiPoint(theLon,theLat));
  if(xy.X() < itsX1 || xy.X() > itsX2 || xy.Y() < itsY1 || xy.Y() > itsY2)
	return false;

  // Invalidate the results

  itsReduced = false;
  itsResults.clear();

  // Add the point to the candidates
  
  PointData pd(theID,xy.X(),xy.Y());
  if(itsNegateFlag)
	itsCandidates.insert(make_pair(-theValue,pd));
  else
	itsCandidates.insert(make_pair(theValue,pd));

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Reduce the results
 */
// ----------------------------------------------------------------------

void PointSelector::Pimple::reduce() const
{
  if(itsReduced)
	return;

  // Go through all the candidate points in descending order

  itsResults.clear();

  // This is used for speeding up the distance calculations
  NFmiNearTree<NFmiPoint> ntree;
  NFmiPoint nearest;

  for(Candidates::const_iterator it = itsCandidates.begin();
	  it != itsCandidates.end();
	  ++it)
	{
	  const NFmiPoint xy(it->second.x,it->second.y);
	  if(ntree.NearestPoint(nearest,xy,itsMinDistance))
		{
		  // There was an earlier point too close - discard the candidate
		}
	  else
		{
		  // Accept the candidate
		  itsResults.push_back(it->second.id);
		  ntree.Insert(xy);
		}
	}

  itsReduced = true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Destructor
 */
// ----------------------------------------------------------------------

PointSelector::~PointSelector()
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Constructor
 *
 * \param theArea The relevant projection area
 * \param theNegateFlag True, if ascending sort is desired
 */
// ----------------------------------------------------------------------

PointSelector::PointSelector(const NFmiArea & theArea, bool theNegateFlag)
  : itsPimple(new Pimple(theArea,theNegateFlag))
{
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the minimum pixel distance between points
 *
 * Throws if the distance is negative
 *
 * \param theDistance The minimum distance
 */
// ----------------------------------------------------------------------

void PointSelector::minDistance(double theDistance)
{
  if(theDistance < 0)
	throw runtime_error("PointSelector::minDistance must be nonnegative");
  itsPimple->itsMinDistance = theDistance;
}

// ----------------------------------------------------------------------
/*!
 * \brief Set the clipping bounding box
 *
 * The constructor initializes the default bounding box based on
 * the projection XY-coordinate limits, so usually this method
 * is not needed unless some extra clipping marging is required.
 *
 * \param theX1 The minimum allowed X-coordinate
 * \param theY1 The minimum allowed Y-coordinate
 * \param theX2 The largest allowed X-coordinate
 * \param theY2 The largest allowed Y-coordinate
 */
// ----------------------------------------------------------------------

void PointSelector::boundingBox(double theX1, double theY1,
								double theX2, double theY2)
{
  itsPimple->itsX1 = theX1;
  itsPimple->itsY1 = theY1;
  itsPimple->itsX2 = theX2;
  itsPimple->itsY2 = theY2;
}

// ----------------------------------------------------------------------
/*!
 * \brief Add a new candidate point to be processed later
 *
 * \param theID The unique ID for the point
 * \param theValue The value for sorting
 * \param theLon The longitude
 * \param theLat The latitude
 * \return True, if the point is within the bounding box and is
 *         thus accepted for later processing
 */
// ----------------------------------------------------------------------

bool PointSelector::add(int theID,
						double theValue,
						double theLon,
						double theLat)
{
  return itsPimple->add(theID, theValue, theLon, theLat);
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if the number of selected points is zero
 */
// ----------------------------------------------------------------------

bool PointSelector::empty() const
{
  return itsPimple->empty();
}

// ----------------------------------------------------------------------
/*!
 * \brief Return the number of selected points
 */
// ----------------------------------------------------------------------

PointSelector::size_type PointSelector::size() const
{
  return itsPimple->size();
}

// ----------------------------------------------------------------------
/*!
 * \brief The begin const_iterator
 *
 * \return The begin const_iterator
 */
// ----------------------------------------------------------------------

PointSelector::const_iterator PointSelector::begin() const
{
  return itsPimple->begin();
}

// ----------------------------------------------------------------------
/*!
 * \brief The end const_iterator
 *
 * \return The end const_iterator
 */
// ----------------------------------------------------------------------

PointSelector::const_iterator PointSelector::end() const
{
  return itsPimple->end();
}

// ======================================================================
