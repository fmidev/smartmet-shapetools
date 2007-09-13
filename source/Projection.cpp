// ======================================================================
/*!
 * \file
 * \brief Implementation of the Projection class
 */
// ======================================================================

#include "Projection.h"

#include "newbase/NFmiEquidistArea.h"
#include "newbase/NFmiGlobals.h"
#include "newbase/NFmiGnomonicArea.h"
#include "newbase/NFmiLatLonArea.h"
#include "newbase/NFmiMercatorArea.h"
#include "newbase/NFmiPoint.h"
#include "newbase/NFmiStereographicArea.h"
#include "newbase/NFmiYKJArea.h"

#include <stdexcept>

using namespace std;
// ----------------------------------------------------------------------
/*!
 * The implementation hiding pimple for class Projection
 */
// ----------------------------------------------------------------------

//! The implementation hiding pimple for class Projection
struct ProjectionPimple
{
  ProjectionPimple(void)
	: itsType("")
	, itsCentralLatitude(kFloatMissing)
	, itsCentralLongitude(kFloatMissing)
	, itsTrueLatitude(kFloatMissing)
	, itsBottomLeft(NFmiPoint(kFloatMissing,kFloatMissing))
	, itsTopRight(NFmiPoint(kFloatMissing,kFloatMissing))
	, itsCenter(NFmiPoint(kFloatMissing,kFloatMissing))
	, itsScale(kFloatMissing)
	, itsWidth(-1)
	, itsHeight(-1)
	, itsOrigin(0,0)
  { }

  std::string itsType;
  float itsCentralLatitude;
  float itsCentralLongitude;
  float itsTrueLatitude;
  NFmiPoint itsBottomLeft;
  NFmiPoint itsTopRight;
  NFmiPoint itsCenter;
  float itsScale;
  float itsWidth;
  float itsHeight;
  NFmiPoint itsOrigin;

};

// ----------------------------------------------------------------------
/*!
 * The destructor does nothing special.
 */
// ----------------------------------------------------------------------

Projection::~Projection(void)
{
}

// ----------------------------------------------------------------------
/*!
 * The void constructor merely initializes the implementation pimple
 */
// ----------------------------------------------------------------------

Projection::Projection(void)
{
#ifdef _MSC_VER
  itsPimple = auto_ptr<ProjectionPimple>(new ProjectionPimple());
#else
  itsPimple.reset(new ProjectionPimple());
#endif
}
// ----------------------------------------------------------------------
/*!
 * The copy constructor
 *
 * \param theProjection The object to copy
 */
// ----------------------------------------------------------------------

Projection::Projection(const Projection & theProjection)
{
#ifdef _MSC_VER
  itsPimple = auto_ptr<ProjectionPimple>(new ProjectionPimple(*theProjection.itsPimple));
#else
  itsPimple.reset(new ProjectionPimple(*theProjection.itsPimple));
#endif
}

// ----------------------------------------------------------------------
/*!
 * Assignment operator
 *
 * \param theProjection The projection to copy
 * \return This
 */
// ----------------------------------------------------------------------

Projection & Projection::operator=(const Projection & theProjection)
{
  if(this != &theProjection)
	{
#ifdef _MSC_VER
		itsPimple = auto_ptr<ProjectionPimple>(new ProjectionPimple(*theProjection.itsPimple));
#else
		itsPimple.reset(new ProjectionPimple(*theProjection.itsPimple));
#endif
	}
  return *this;
}

// ----------------------------------------------------------------------
/*!
 * Set the type of the projection.
 *
 * \param theType The name of the projection
 */
// ----------------------------------------------------------------------

void Projection::type(const std::string & theType)
{
  itsPimple->itsType = theType;
}

// ----------------------------------------------------------------------
/*!
 * Set the central longitude of the projection.
 *
 * \param theLongitude The longitude
 */
// ----------------------------------------------------------------------

void Projection::centralLongitude(float theLongitude)
{
  itsPimple->itsCentralLongitude = theLongitude;
}

// ----------------------------------------------------------------------
/*!
 * Set the central latitude of the projection.
 *
 * \param theLatitude The latitude
 */
// ----------------------------------------------------------------------

void Projection::centralLatitude(float theLatitude)
{
  itsPimple->itsCentralLatitude = theLatitude;
}

// ----------------------------------------------------------------------
/*!
 * Set the true latitude of the projection.
 *
 * \param theLatitude The latitude
 */
// ----------------------------------------------------------------------

void Projection::trueLatitude(float theLatitude)
{
  itsPimple->itsTrueLatitude = theLatitude;
}

// ----------------------------------------------------------------------
/*!
 * Set the bottom left coordinates of the projection
 *
 * \param theLon The longitude
 * \param theLat The latitude
 */
// ----------------------------------------------------------------------

void Projection::bottomLeft(float theLon, float theLat)
{
  itsPimple->itsBottomLeft = NFmiPoint(theLon, theLat);
}

// ----------------------------------------------------------------------
/*!
 * Set the top right coordinates of the projection
 *
 * \param theLon The longitude
 * \param theLat The latitude
 */
// ----------------------------------------------------------------------

void Projection::topRight(float theLon, float theLat)
{
  itsPimple->itsTopRight = NFmiPoint(theLon, theLat);
}

// ----------------------------------------------------------------------
/*!
 * Set the center coordinates of the projection
 *
 * \param theLon The longitude
 * \param theLat The latitude
 */
// ----------------------------------------------------------------------

void Projection::center(float theLon, float theLat)
{
  itsPimple->itsCenter = NFmiPoint(theLon, theLat);
}

// ----------------------------------------------------------------------
/*!
 * Set the scale of the projection (when center is also specified).
 *
 * \param theScale the scale
 */
// ----------------------------------------------------------------------

void Projection::scale(float theScale)
{
  itsPimple->itsScale = theScale;
}

// ----------------------------------------------------------------------
/*!
 * Set the width of the projection. -1 implies automatic calculation.
 *
 * \param theWidth the width
 */
// ----------------------------------------------------------------------

void Projection::width(float theWidth)
{
  itsPimple->itsWidth = theWidth;
}

// ----------------------------------------------------------------------
/*!
 * Set the height of the projection. -1 implies automatic calculation.
 *
 * \param theHeight the height
 */
// ----------------------------------------------------------------------

void Projection::height(float theHeight)
{
  itsPimple->itsHeight = theHeight;
}

void Projection::origin(float theLon, float theLat)
{
  itsPimple->itsOrigin = NFmiPoint(theLon, theLat);
}

// ----------------------------------------------------------------------
/*!
 * The projection service provided by the class.
 *
 * This method projects and creates the wanted area
 *
 * \return The projected area
 */
// ----------------------------------------------------------------------

NFmiArea* Projection::createArea(void) const
{

  if(itsPimple->itsWidth < 0 && itsPimple->itsHeight < 0)
	  throw std::runtime_error("Must specify atleast one of width/height");

  // Create the required projection

  NFmiArea * area = 0;

  // Special handling for the center

  bool has_center = (itsPimple->itsCenter.X()!=kFloatMissing &&
					 itsPimple->itsCenter.Y()!=kFloatMissing);

  NFmiPoint bottomleft = has_center ? itsPimple->itsCenter : itsPimple->itsBottomLeft;
  NFmiPoint topright = has_center ? itsPimple->itsCenter : itsPimple->itsTopRight;

  NFmiPoint topleftxy = NFmiPoint(0,0);
  NFmiPoint bottomrightxy = NFmiPoint(1,1);

  if(itsPimple->itsType == string("latlon"))
	area = new NFmiLatLonArea(bottomleft, topright,
							 topleftxy, bottomrightxy);

  else if(itsPimple->itsType == "ykj")
	area = new NFmiYKJArea(bottomleft, topright,
							   topleftxy, bottomrightxy);

  else if(itsPimple->itsType == "mercator")
	area = new NFmiMercatorArea(bottomleft, topright,
									topleftxy, bottomrightxy);

  else if(itsPimple->itsType == string("stereographic"))
	area = new NFmiStereographicArea(bottomleft, topright,
										 itsPimple->itsCentralLongitude,
										 topleftxy, bottomrightxy,
										 itsPimple->itsCentralLatitude,
										 itsPimple->itsTrueLatitude);
  
  else if(itsPimple->itsType == "gnomonic")
	area = new NFmiGnomonicArea(bottomleft, topright,
									itsPimple->itsCentralLongitude,
									topleftxy, bottomrightxy,
									itsPimple->itsCentralLatitude,
									itsPimple->itsTrueLatitude);

  else if(itsPimple->itsType == "equidist")
	area = new NFmiEquidistArea(bottomleft, topright,
									itsPimple->itsCentralLongitude,
									topleftxy, bottomrightxy,
									itsPimple->itsCentralLatitude);
  else
	{
	  std::string msg = "Unrecognized projection type ";
	  msg += itsPimple->itsType;
	  msg += " in Projection::project()";
	  throw std::runtime_error(msg);
	}

  // Recalculate topleft and bottom right if center was set
  if(has_center)
	{
	  const float scale = 1000*itsPimple->itsScale;
	  const float width = itsPimple->itsWidth;
	  const float height = itsPimple->itsHeight;
	  const NFmiPoint c = area->LatLonToWorldXY(itsPimple->itsCenter);
	  const NFmiPoint bl(c.X()-scale*width, c.Y()-scale*height);
	  const NFmiPoint tr(c.X()+scale*width, c.Y()+scale*height);
	  
	  const NFmiPoint BL = area->WorldXYToLatLon(bl);
	  const NFmiPoint TR = area->WorldXYToLatLon(tr);

	  NFmiArea *tmp = area->NewArea(BL,TR);
	  delete area;
	  area = tmp;
	}
  else
	{
		float w = itsPimple->itsWidth;
		float h = itsPimple->itsHeight;

	    if(w<0)	w = h * area->WorldXYAspectRatio();
	    if(h<0)	h = w / area->WorldXYAspectRatio();

	    area->SetXYArea(NFmiRect(itsPimple->itsOrigin.X(), h, w, itsPimple->itsOrigin.Y()));
  }

  return area;

}

// ======================================================================
