// ======================================================================
/*!
 * \file
 * \brief Implementation of namespace BoundingBox
 */
// ======================================================================

#include "BoundingBox.h"
#include "NFmiArea.h"

namespace
{
  // ----------------------------------------------------------------------
  /*!
   * \brief Update bounding box based on given point
   *
   * \param thePoint The point to add to the bounding box
   * \param theMinLon The variable in which to store the minimum longitude
   * \param theMinLat The variable in which to store the minimum latitude
   * \param theMaxLon The variable in which to store the maximum longitude
   * \param theMaxLat The variable in which to store the maximum latitude
   */
  // ----------------------------------------------------------------------
  
  void UpdateBBox(const NFmiPoint & thePoint,
				  double & theMinLon, double & theMinLat,
				  double & theMaxLon, double & theMaxLat)
  {
	theMinLon = std::min(theMinLon,thePoint.X());
	theMinLat = std::min(theMinLat,thePoint.Y());
	theMaxLon = std::max(theMaxLon,thePoint.X());
	theMaxLat = std::max(theMaxLat,thePoint.Y());
  }
  
} // namespace anonymous

namespace BoundingBox
{

  // ----------------------------------------------------------------------
  /*!
   * \brief Find geographic bounding box for given area
   *
   * The bounding box is found by traversing the edges of the area
   * and converting the coordinates to geographic ones for extrema
   * calculations.
   *
   * \param theArea The area
   * \param theMinLon The variable in which to store the minimum longitude
   * \param theMinLat The variable in which to store the minimum latitude
   * \param theMaxLon The variable in which to store the maximum longitude
   * \param theMaxLat The variable in which to store the maximum latitude
   */
  // ----------------------------------------------------------------------
  
  void FindBBox(const NFmiArea & theArea,
				double & theMinLon, double & theMinLat,
				double & theMaxLon, double & theMaxLat)
  {
  // Good initial values are obtained from the corners
	
	theMinLon = theArea.TopLeftLatLon().X();
	theMinLat = theArea.TopLeftLatLon().Y();
	theMaxLon = theMinLon;
	theMaxLat = theMinLat;
	
	const unsigned int divisions = 500;
	
	// Go through the top edge
	
	for(unsigned int i=0; i<=divisions; i++)
	  {
		NFmiPoint xy(theArea.Left()+theArea.Width()*i/divisions,
					 theArea.Top());
		NFmiPoint latlon(theArea.ToLatLon(xy));
		UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	  }
	
	// Go through the bottom edge
	
	for(unsigned int i=0; i<=divisions; i++)
	  {
		NFmiPoint xy(theArea.Left()+theArea.Width()*i/divisions,
					 theArea.Bottom());
		NFmiPoint latlon(theArea.ToLatLon(xy));
		UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	  }
	
	// Go through the left edge
	
	for(unsigned int i=0; i<=divisions; i++)
	  {
		NFmiPoint xy(theArea.Left(),
					 theArea.Bottom()+theArea.Height()*i/divisions);
		NFmiPoint latlon(theArea.ToLatLon(xy));
		UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	  }
	
	// Go through the top edge
	
	for(unsigned int i=0; i<=divisions; i++)
	  {
		NFmiPoint xy(theArea.Left(),
					 theArea.Bottom()+theArea.Height()*i/divisions);
		NFmiPoint latlon(theArea.ToLatLon(xy));
		UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	  }
  }

} // namespace BoundingBox
