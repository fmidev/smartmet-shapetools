// ======================================================================
/*!
 * \file
 * \brief Bounding box tools
 */
// ======================================================================

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

class NFmiArea;

namespace BoundingBox
{

  void FindBBox(const NFmiArea & theArea,
				double & theMinLon,
				double & theMinLat,
				double & theMaxLon,
				double & theMaxLat);

} // namespace BoundingBox

#endif // BOUNDINGBOX_H

// ======================================================================
