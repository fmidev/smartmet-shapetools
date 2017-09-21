// ======================================================================
/*!
 * \file
 * \brief Implementation of namespace GradsTools
 */
// ======================================================================

#include "GradsTools.h"
#include <newbase/NFmiPoint.h>

#include <iostream>

using namespace std;

namespace GradsTools
{
// ----------------------------------------------------------------------
/*!
 * \brief Print coodinate as 3-byte integer
 */
// ----------------------------------------------------------------------

void print_double(ostream &out, double theValue)
{
  int value = static_cast<int>(theValue * 1e4 + 0.5);
  out << static_cast<unsigned char>((value >> 16) & 0xFF);
  out << static_cast<unsigned char>((value >> 8) & 0xFF);
  out << static_cast<unsigned char>((value >> 0) & 0xFF);
}

// ----------------------------------------------------------------------
/*!
 * \brief Print longitude
 */
// ----------------------------------------------------------------------

void print_lon(ostream &out, double theLon)
{
  // Make sure value is in range [0-360[
  if (theLon < 0) theLon += 360;
  print_double(out, theLon);
}

// ----------------------------------------------------------------------
/*!
 * \brief Print latitude
 */
// ----------------------------------------------------------------------

void print_lat(ostream &out, double theLat)
{
  // Make sure value is in shifted range [0-180[
  print_double(out, theLat + 90);
}

// ----------------------------------------------------------------------
/*!
 * \brief Print out a sequence of lineto commands
 *
 * Each record header consists of
 *
 *  - Byte 0: 1
 *  - Byte 1: The type of the record
 *  - Byte 2: Number of points in the record
 *  - Bytes 3-N: Pairs of lon-lat values
 */
// ----------------------------------------------------------------------

void print_line(ostream &out, int theLevel, const vector<NFmiPoint> &thePoints)
{
  if (thePoints.empty()) return;

  typedef vector<NFmiPoint>::size_type size_type;

  size_type pos1 = 0;
  while (pos1 < thePoints.size())
  {
    size_type pos2 = min(pos1 + 254, thePoints.size() - 1);
    if (pos1 == pos2) break;

    // Must not cross the meridian during one segment

    const double x = thePoints[pos1].X();
    for (size_type i = pos1 + 1; i <= pos2; i++)
    {
      if ((x < 0 && thePoints[i].X() >= 0) || (x >= 0 && thePoints[i].X() < 0))
      {
        pos2 = max(pos1, i - 1);
        break;
      }
    }

    // Byte 0:
    out << static_cast<unsigned char>(1);
    // Byte 1:
    out << static_cast<unsigned char>(theLevel);
    // Byte 2:
    out << static_cast<unsigned char>(pos2 - pos1 + 1);
    // The coordinates
    for (unsigned int i = pos1; i <= pos2; i++)
    {
      print_lon(out, thePoints[i].X());
      print_lat(out, thePoints[i].Y());
    }
    // Note! no +1, we start from where we left off,
    // unless we crossed the meridian with one single point!
    if (pos2 != pos1)
      pos1 = pos2;
    else
      pos1++;
  }

  return;
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a 3-byte integer from the input stream
 */
// ----------------------------------------------------------------------

int read_int(istream &in)
{
  unsigned char ch1, ch2, ch3;
  in >> noskipws >> ch1 >> ch2 >> ch3;
  return ((ch1 << 16) | (ch2 << 8) | ch3);
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a longitude from the input stream
 */
// ----------------------------------------------------------------------

double read_lon(istream &in)
{
  int value = read_int(in);
  double lon = (value / 1e4);
  if (lon >= 180) lon -= 360;
  return lon;
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a latitude from the input stream
 */
// ----------------------------------------------------------------------

double read_lat(istream &in)
{
  int value = read_int(in);
  double lat = value / 1e4 - 90;
  return lat;
}

// ----------------------------------------------------------------------
/*!
 * \brief Read a length from the input stream
 */
// ----------------------------------------------------------------------

unsigned int read_length(istream &in)
{
  unsigned char ch1, ch2, ch3, ch4;
  in >> noskipws >> ch1 >> ch2 >> ch3 >> ch4;
  return ((ch1 << 24) | (ch2 << 16) | (ch3 << 8) | ch4);
}

}  // namespace GradsTools

// ======================================================================
