// ======================================================================
/*!
 * \file
 * \brief Interface of namespace GradsTools
 */
// ======================================================================

#ifndef GRADSTOOLS_H
#define GRADSTOOLS_H

#include <iosfwd>
#include <vector>

class NFmiPoint;

namespace GradsTools {

void print_double(std::ostream &out, double theValue);
void print_lon(std::ostream &out, double theLon);
void print_lat(std::ostream &out, double theLat);
void print_line(std::ostream &out, int theLevel,
                const std::vector<NFmiPoint> &thePoints);

int read_int(std::istream &in);
double read_lon(std::istream &in);
double read_lat(std::istream &in);
unsigned int read_length(std::istream &in);

} // namespace GradsTools

#endif // GRADSTOOLS_H

// ======================================================================
