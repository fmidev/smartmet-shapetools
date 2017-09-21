// ======================================================================
/*!
 * \file
 * \brief Interface of the Projection class
 */
// ======================================================================
/*!
 * \class FMI::RadContour::Projection
 *
 * The responsibility of this class is to provide projection services
 * for NFmiPath objects onto the given image.
 *
 * In detail, this class checks the given image dimensions, modifies
 * the internally stored projection accordingly, and then projects
 * the given path so that the area specified in the projection
 * matches exactly the boundaries of the image.
 *
 * The alternative way to implement this would have been to require
 * the user to explicitly create the projection with knowledge of
 * the image size to be used. This is not always convenient in
 * scripted environments due to the execution order of things.
 * Hence this solution, which will automatically check the image
 * dimensions at the precise moment when it is needed - right
 * before rendering a path to the image.
 *
 * Note that it is not possible to create a projection using the
 * center and scale only without knowing the width and height also.
 * Hence we use an approach which stores the projection specification
 * in name only, and then we construct the actual projection on demand
 * when the width and height are known for certain.
 *
 * The recognized projection types are
 *   - latlon
 *   - stereographic
 *   - ykj
 *   - equidist
 *   - mercator
 *   - gnomonic

 */
// ----------------------------------------------------------------------

#ifndef PROJECTION_H
#define PROJECTION_H

#include <memory>
#include <string>
class NFmiArea;

struct ProjectionPimple;

//! Define a projection
class Projection
{
 public:
  ~Projection(void);
  Projection(void);
  Projection(const Projection &theProjection);
  Projection &operator=(const Projection &theProjection);

  void type(const std::string &theType);
  void centralLatitude(float theLatitude);
  void centralLongitude(float theLongitude);
  void trueLatitude(float theLongitude);

  void bottomLeft(float theLon, float theLat);
  void topRight(float theLon, float theLat);

  void center(float theLon, float theLat);
  void scale(float theScale);

  void width(float theWidth);
  void height(float theHeight);
  void origin(float theLon, float theLat);

  NFmiArea *createArea(void) const;

 private:
  std::auto_ptr<ProjectionPimple> itsPimple;

};  // class Projection

#endif  // PROJECTION_H

// ======================================================================
