// ======================================================================
/*!
 * \file Polyline.cpp
 * \brief Implementation details for Polyline class
 */
// ======================================================================

#include "Polyline.h"
#include <newbase/NFmiValueString.h>

#include <iostream>
#include <sstream>

using namespace std;

// ======================================================================
//				HIDDEN INTERNAL FUNCTIONS
// ======================================================================

namespace
{
//! The number identifying the region within the rectangle
const int central_quadrant = 4;

//! Test the position of given point with respect to a rectangle.
int quadrant(double x, double y, double x1, double y1, double x2, double y2, double margin)
{
  int value = central_quadrant;
  if (x < x1 - margin)
    value--;
  else if (x > x2 + margin)
    value++;
  if (y < y1 - margin)
    value -= 3;
  else if (y > y2 + margin)
    value += 3;
  return value;
}

//! Test whether two rectangles intersect
bool intersects(
    double x1, double y1, double x2, double y2, double X1, double Y1, double X2, double Y2)
{
  bool xoutside = (x1 > X2 || x2 < X1);
  bool youtside = (y1 > Y2 || y2 < Y1);
  return (!xoutside && !youtside);
}

}  // anonymous namespace

// ======================================================================
//				METHOD IMPLEMENTATIONS
// ======================================================================

// ----------------------------------------------------------------------
/*!
 * Return a string representation of the polyline using the given
 * strings for path movement operations.
 *
 * \todo Remove dependency on NFmiValueString once g++ supports
 *       setprecision better. Full accuracy is needed.
 */
// ----------------------------------------------------------------------

string Polyline::path(const string &moveto, const string &lineto, const string &closepath) const
{
  ostringstream out;
  unsigned int n = size() - 1;
  bool isclosed = (size() > 1 && itsPoints[0] == itsPoints[n]);
  for (unsigned int i = 0; i <= n; i++)
  {
    if (isclosed && i == n && !closepath.empty())
      out << closepath;
    else
    {
      out << itsPoints[i].x() << ' ' << itsPoints[i].y() << ' ' << (i == 0 ? moveto : lineto);
    }
    out << endl;
  }
  return out.str();
}

// ----------------------------------------------------------------------
/*!
 * Clip the polyline against the given rectangle. The result will contain
 * only points in the original polyline, no intersections are ever calculated.
 */
// ----------------------------------------------------------------------

void Polyline::clip(double theX1, double theY1, double theX2, double theY2, double margin)
{
  if (empty()) return;

  DataType newpts;

  int last_quadrant = 0;
  int next_quadrant = 0;
  int this_quadrant = 0;

  // Initialize the new bounding box
  double minx = itsPoints[0].x();
  double miny = itsPoints[0].y();
  double maxx = itsPoints[0].x();
  double maxy = itsPoints[0].y();

  next_quadrant = quadrant(itsPoints[0].x(), itsPoints[0].y(), theX1, theY1, theX2, theY2, margin);

  for (unsigned int i = 0; i < size(); i++)
  {
    last_quadrant = this_quadrant;
    this_quadrant = next_quadrant;
    if (i < size() - 1)
      next_quadrant =
          quadrant(itsPoints[i + 1].x(), itsPoints[i + 1].y(), theX1, theY1, theX2, theY2, margin);

    bool pushit = true;

    if (i == 0)
      pushit = true;
    else if (this_quadrant == central_quadrant)
      pushit = true;
    else if (this_quadrant != next_quadrant)
      pushit = true;
    else if (this_quadrant != last_quadrant)
      pushit = true;
    else if (i == size() - 1)
      pushit = true;
    else
      pushit = false;

    if (pushit)
    {
      newpts.push_back(itsPoints[i]);

      // Update bounding box if point was accepted

      minx = min(minx, itsPoints[i].x());
      miny = min(miny, itsPoints[i].y());
      maxx = max(maxx, itsPoints[i].x());
      maxy = max(maxy, itsPoints[i].y());
    }
  }

  // If the bounding boxes don't intersect, output nothing, otherwise
  // replace old points with clipped ones

  if (newpts.size() <= 1 ||
      !intersects(
          minx, miny, maxx, maxy, theX1 - margin, theY1 - margin, theX2 + margin, theY2 + margin))
    itsPoints.clear();
  else
    itsPoints = newpts;
}

// ======================================================================
