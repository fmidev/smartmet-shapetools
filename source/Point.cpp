// ======================================================================
/*!
 * \file Point.cpp
 * \brief Definition of Point class methods.
 */
// ======================================================================

#include "Point.h"

// Pirun g++-3.0.4 on buginen, min ei toimi
// #include <algorithm>
template <typename T>
inline const T & min(const T & a, const T & b)
{
  if(b<a) return b;
  return a;
}

// ----------------------------------------------------------------------
/*!
 * Calculate the cartographic distance between two points. The distance
 * is calculated using the Haversine formula.
 */
// ----------------------------------------------------------------------

double Point::geodistance(const Point & pt) const
{
  const double kpi = 3.14159265358979323848/180;
  const double R = 6371.220;

  const double x1 = kpi*x();
  const double y1 = kpi*y();
  const double x2 = kpi*pt.x();
  const double y2 = kpi*pt.y();

  const double sindx = sin((x2-x1)/2);
  const double sindy = sin((y2-y1)/2);
  const double a = sindy*sindy + cos(y1)*cos(y2)*sindx*sindx;
  const double c = 2*asin(min(1.0,sqrt(a)));
  return R*c;
}

// ======================================================================

