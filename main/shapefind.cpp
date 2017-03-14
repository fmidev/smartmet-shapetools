// ======================================================================
/*!
 * \file shapefind
 * \brief A program to find the features closest to the given point
 */
// ======================================================================
/*
 * \page shapefind shapefind
 *
 * shapefind takes as input a single shapefile containing arbitrary
 * shapedata and a coordinate to search for. The program prints out
 * the elements closest to the given point. If the data is polygonal,
 * the polygon containing the point is printed out.
 *
 * The program was originally created for finding the road closest
 * to the given coordinate.
 *
 */
// ======================================================================

#include <newbase/NFmiArea.h>
#include <newbase/NFmiAreaFactory.h>
#include <newbase/NFmiGeoTools.h>
#include <newbase/NFmiPoint.h>
#include <newbase/NFmiPreProcessor.h>
#include <newbase/NFmiStringTools.h>
#include <imagine/NFmiEsriPoint.h>
#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriPolyLine.h>
#include <imagine/NFmiEsriShape.h>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <stdexcept>
#include <string>
#include <memory>

using namespace std;
using namespace Imagine;

// ----------------------------------------------------------------------
/*!
 * \brief Command line options
 */
// ----------------------------------------------------------------------

struct Options {
  string shapefile;
  string condition;
  string attributes;
  string uniqueattribute;
  string projection;
  string coordinatefile;
  float latitude;
  float longitude;
  float searchradius;
  unsigned int maxcount;
  string delimiter;
  bool verbose;

  Options();
};

// ----------------------------------------------------------------------
/*!
 * \brief Default options
 */
// ----------------------------------------------------------------------

Options::Options()
    : shapefile(), condition(), attributes(), uniqueattribute(),
      projection("latlon"), coordinatefile(), latitude(), longitude(),
      searchradius(10) // kilometers
      ,
      maxcount(1), delimiter("\t"), verbose(false) {}

// ----------------------------------------------------------------------
/*!
 * \brief Global access to options
 */
// ----------------------------------------------------------------------

Options options;

// Search projection and the projected search coordinate

NFmiAreaFactory::return_type projection;

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 */
// ----------------------------------------------------------------------

bool parse_options(int argc, char *argv[]) {
  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "print out help message")(
      "verbose,v", po::bool_switch(&options.verbose),
      "set verbose mode on")("version,V", "display version number")(
      "attributes,a", po::value(&options.attributes),
      "shapefile attributes to be printed (comma separated)")(
      "unique,u", po::value(&options.uniqueattribute),
      "force a specific attribute to have unique values")(
      "shapefile,s", po::value(&options.shapefile),
      "shapefile (without suffix)")("condition,c",
                                    po::value(&options.condition),
                                    "search condition (for example CLASS>2)")(
      "lat,y", po::value(&options.latitude),
      "latitude of the searched coordinate")(
      "lon,x", po::value(&options.longitude),
      "longitude of the searched coordinate")(
      "coordinatefile,l", po::value(&options.coordinatefile),
      "file containing lines of form name,lon,lat for processing multiple "
      "coordinates simultaneously")(
      "radius,r", po::value(&options.searchradius),
      "maximum search radius")("maxcount,n", po::value(&options.maxcount),
                               "maximum number of search results")(
      "delimiter,d", po::value(&options.delimiter),
      "delimiter string for output columns")(
      "projection,p", po::value(&options.projection),
      "projection definition (default=latlon)");

  po::positional_options_description p;
  p.add("shapefile", 1);

  po::variables_map opt;
  po::store(
      po::command_line_parser(argc, argv).options(desc).positional(p).run(),
      opt);

  po::notify(opt);

  if (opt.count("version") != 0) {
    cout << "shapefind v1.0 (" << __DATE__ << ' ' << __TIME__ << ')' << endl;
  }

  if (opt.count("help")) {
    cout << "Usage: shapefind [options] shapefile" << endl
         << endl
         << "shapefind finds the element closest to the given point" << endl
         << endl
         << desc << endl
         << "Note that if the input data consists of polygons, the options"
         << endl
         << "-u, -r and -n are meaningless since the program finds the first"
         << endl
         << "polygon containing the input coordinate." << endl
         << endl;
    return false;
  }

  if (opt.count("shapefile") == 0)
    throw runtime_error("shapefile name not specified");

  if ((opt.count("lon") > 0 || opt.count("lat") > 0) &&
      (opt.count("coordinatefile") > 0))
    throw runtime_error("-l and -x,-y options are mutually exclusive");

  // Sanity checks

  if (options.latitude < -90 || options.latitude > 90)
    throw runtime_error("Search latitude outside -90...90");

  if (options.longitude < -180 || options.longitude > 180)
    throw runtime_error("Search longitude outside -180...180");

  if (options.searchradius < 0)
    throw runtime_error("Search radius cannot be negative");

  if (options.maxcount <= 0)
    throw runtime_error("maxcount must be nonnegative");

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Read file of form name,lon,lat
 */
// ----------------------------------------------------------------------

typedef map<string, NFmiPoint> LocationList;

LocationList read_locationlist(const string &theFile) {
  // this code was copied from qdpoint.cpp

  LocationList ret;

  const bool strip_pound = true;
  NFmiPreProcessor processor(strip_pound);
  if (!processor.ReadAndStripFile(theFile))
    throw runtime_error("Unable to preprocess " + theFile);

  string text = processor.GetString();
  istringstream input(text);
  string line;
  while (getline(input, line)) {
    if (line.empty())
      continue;
    vector<string> parts = NFmiStringTools::Split(line, ",");
    if (parts.size() != 3)
      cerr << "Warning: Invalid line '" + line + "' in file '" + theFile + "'"
           << endl;
    else {
      try {
        string name = parts[0];
        double lon = NFmiStringTools::Convert<double>(parts[1]);
        double lat = NFmiStringTools::Convert<double>(parts[2]);
        ret.insert(make_pair(name, NFmiPoint(lon, lat)));
      } catch (...) {
        cerr << "Error while reading '" + theFile + "'";
        throw;
      }
    }
  }
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Parse comparison operation
 */
// ----------------------------------------------------------------------

void parse_condition(string &theVariable, string &theComparison,
                     string &theValue) {
  if (options.condition.empty())
    return;

  // Possible operations

  list<string> comparisons;
  comparisons.push_back("==");
  comparisons.push_back("<=");
  comparisons.push_back(">=");
  comparisons.push_back("<>");
  comparisons.push_back("<");
  comparisons.push_back(">");
  comparisons.push_back("=");

  // Try to split the condition

  for (list<string>::const_iterator it = comparisons.begin();
       it != comparisons.end(); ++it) {
    string::size_type pos = options.condition.find(*it);
    if (pos != string::npos) {
      theComparison = *it;
      theVariable = options.condition.substr(0, pos);
      theValue = options.condition.substr(pos + it->size());
      break;
    }
  }

  if (theComparison.empty())
    throw runtime_error("Unable to parse search condition");
}

// ----------------------------------------------------------------------
/*!
 * \brief Test the search condition
 */
// ----------------------------------------------------------------------

bool condition_satisfied(const NFmiEsriElement &theElement,
                         const std::string &theVariable,
                         const std::string &theComparison,
                         const std::string &theValue) {
  if (theComparison.empty())
    return true;

  NFmiEsriAttributeType atype = theElement.GetType(theVariable);

  if (atype == kFmiEsriString) {
    if (theComparison == "=" || theComparison == "==")
      return (theElement.GetString(theVariable) == theValue);
    else if (theComparison == "<>")
      return (theElement.GetString(theVariable) != theValue);
    else if (theComparison == "<")
      return (theElement.GetString(theVariable) < theValue);
    else if (theComparison == ">")
      return (theElement.GetString(theVariable) > theValue);
    else if (theComparison == "<=")
      return (theElement.GetString(theVariable) <= theValue);
    else if (theComparison == ">=")
      return (theElement.GetString(theVariable) >= theValue);
  } else if (atype == kFmiEsriInteger) {
    double value = boost::lexical_cast<double>(theValue);
    if (theComparison == "=" || theComparison == "==")
      return (theElement.GetInteger(theVariable) == value);
    else if (theComparison == "<>")
      return (theElement.GetInteger(theVariable) != value);
    else if (theComparison == "<")
      return (theElement.GetInteger(theVariable) < value);
    else if (theComparison == ">")
      return (theElement.GetInteger(theVariable) > value);
    else if (theComparison == "<=")
      return (theElement.GetInteger(theVariable) <= value);
    else if (theComparison == ">=")
      return (theElement.GetInteger(theVariable) >= value);
  } else if (atype == kFmiEsriDouble) {
    double value = boost::lexical_cast<double>(theValue);
    if (theComparison == "=" || theComparison == "==")
      return (theElement.GetDouble(theVariable) == value);
    else if (theComparison == "<>")
      return (theElement.GetDouble(theVariable) != value);
    else if (theComparison == "<")
      return (theElement.GetDouble(theVariable) < value);
    else if (theComparison == ">")
      return (theElement.GetDouble(theVariable) > value);
    else if (theComparison == "<=")
      return (theElement.GetDouble(theVariable) <= value);
    else if (theComparison == ">=")
      return (theElement.GetDouble(theVariable) >= value);
  }

  // Should never reach this point
  assert(false);
  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Establish projection for distance calculations
 */
// ----------------------------------------------------------------------

void establish_projection() {
  if (options.projection == "latlon")
    return;

  projection = NFmiAreaFactory::Create(options.projection);
}

// ----------------------------------------------------------------------
/*!
 * \brief Make sure an attribute is selected for output
 */
// ----------------------------------------------------------------------

void establish_attribute(const NFmiEsriShape &theShape) {
  const NFmiEsriShape::attributes_type &attributes = theShape.Attributes();

  if (attributes.size() == 0)
    throw runtime_error("shapefile does not contain any attributes");

  // If no attribute option was given, we accept it as long as
  // the shape contains exactly one attribute

  if (options.attributes.empty()) {
    if (attributes.size() > 1) {
      ostringstream out;
      out << "shapefile contains multiple attributes, choose one: ";
      int pos = 0;
      for (NFmiEsriShape::attributes_type::const_iterator
               at = attributes.begin();
           at != attributes.end(); ++at, ++pos) {
        if (pos > 0)
          out << ",";
        out << (*at)->Name();
      }

      throw runtime_error(out.str());
    }
    options.attributes = (*attributes.begin())->Name();
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Establish that all elements of shape are of unique type
 */
// ----------------------------------------------------------------------

NFmiEsriElementType establish_type(const NFmiEsriShape &theShape) {
  if (theShape.Elements().size() == 0)
    throw runtime_error("The shape is empty!");

  NFmiEsriElementType type = kFmiEsriNull;
  bool found_one = false;
  bool multiple_types = false;

  NFmiEsriShape::const_iterator it = theShape.Elements().begin();
  for (; it != theShape.Elements().end(); ++it) {

    if (*it == NULL)
      continue;

    switch ((*it)->Type()) {
    case kFmiEsriNull:
      break;
    case kFmiEsriMultiPatch:
      throw runtime_error("multipatch elements are not supported");
    case kFmiEsriPoint:
    case kFmiEsriPointM:
    case kFmiEsriPointZ:
    case kFmiEsriMultiPoint:
    case kFmiEsriMultiPointM:
    case kFmiEsriMultiPointZ: {
      found_one = true;
      if (type != kFmiEsriNull && type != kFmiEsriPoint)
        multiple_types = true;
      else
        type = kFmiEsriPoint;
      break;
    }
    case kFmiEsriPolyLine:
    case kFmiEsriPolyLineM:
    case kFmiEsriPolyLineZ: {
      found_one = true;
      if (type != kFmiEsriNull && type != kFmiEsriPolyLine)
        multiple_types = true;
      else
        type = kFmiEsriPolyLine;
      break;
    }
    case kFmiEsriPolygon:
    case kFmiEsriPolygonM:
    case kFmiEsriPolygonZ: {
      found_one = true;
      if (type != kFmiEsriNull && type != kFmiEsriPolygon)
        multiple_types = true;
      else
        type = kFmiEsriPolygon;
      break;
    }
    }

    if (multiple_types)
      throw runtime_error("Shape contains elements of different types");
  }

  if (!found_one)
    throw runtime_error("Shape contains only null elements");

  return type;
}

// ----------------------------------------------------------------------
/*!
 * \brief Latlon distance calculator
 */
// ----------------------------------------------------------------------

float latlon_point_distance(double theX1, double theY1, double theX2,
                            double theY2) {
  float m = NFmiGeoTools::GeoDistance(theX1, theY1, theX2, theY2);
  return m / 1000;
}

// ----------------------------------------------------------------------
/*!
 * \brief WorldXY Distance calculator
 */
// ----------------------------------------------------------------------

float world_point_distance(double theX1, double theY1, double theX2,
                           double theY2) {
  float m = NFmiGeoTools::Distance(theX1, theY1, theX2, theY2);
  return m / 1000;
}

// ----------------------------------------------------------------------
/*!
 * \brief Calculate isosegment midpoint
 */
// ----------------------------------------------------------------------

void isosegment_center(double theX1, double theY1, double theX2, double theY2,
                       double &theX, double &theY) {
  double lat1 = FmiRad(theY1);
  double lat2 = FmiRad(theY2);

  double dlon = FmiRad(theX2 - theX1);

  double Bx = cos(lat2) * cos(dlon);
  double By = cos(lat2) * sin(dlon);

  theX = theX1 + atan2(By, cos(lat1) + Bx);
  theY = atan2(sin(lat1) + sin(lat2),
               sqrt((cos(lat1) + Bx) * (cos(lat1) + Bx) + By * By));
}

// ----------------------------------------------------------------------
/*!
 * \brief Isosegment distance calculator
 */
// ----------------------------------------------------------------------

float isosegment_distance(double theX, double theY, double theX1, double theY1,
                          double theX2, double theY2) {
  float m1 = NFmiGeoTools::GeoDistance(theX, theY, theX1, theY1);

  // Isosegment midpoint

  double xc, yc;
  isosegment_center(theX1, theY1, theX2, theY2, xc, yc);

  float m2 = NFmiGeoTools::GeoDistance(theX, theY, theX2, theY2);

  float m = NFmiGeoTools::GeoDistance(theX, theY, xc, yc);

  if (m > m1 && m > m2)
    return min(m1, m2);

  // Recursion should happen pretty seldom

  double x1 = theX1;
  double y1 = theY1;
  double x2 = theX2;
  double y2 = theY2;

  while (m < m1 && m < m2) {
    if (m1 < m2) {
      x2 = xc;
      y2 = yc;
      m2 = m;
    } else {
      x1 = xc;
      y1 = yc;
      m1 = m;
    }

    isosegment_center(x1, y1, x2, y2, xc, yc);
    m = NFmiGeoTools::GeoDistance(theX, theY, xc, yc);
  }

  return m;
}

// ----------------------------------------------------------------------
/*!
 * \brief Latlon line distance calculator
 */
// ----------------------------------------------------------------------

float latlon_line_distance(double theX, double theY, double theX1, double theY1,
                           double theX2, double theY2) {
  float m = isosegment_distance(theX, theY, theX1, theY1, theX2, theY2);
  return m / 1000;
}

// ----------------------------------------------------------------------
/*!
 * \brief WorldXY line distance calculator
 */
// ----------------------------------------------------------------------

float world_line_distance(double theX, double theY, double theX1, double theY1,
                          double theX2, double theY2) {
  float m = NFmiGeoTools::DistanceFromLineSegment(theX, theY, theX1, theY1,
                                                  theX2, theY2);
  return m / 1000;
}

// ----------------------------------------------------------------------
/*!
 * \brief Establish the attribute type of the given attribute
 */
// ----------------------------------------------------------------------

NFmiEsriAttributeType establish_attribute_type(const NFmiEsriShape &theShape,
                                               const string &theName) {
  const Imagine::NFmiEsriShape::attributes_type &attributes =
      theShape.Attributes();

  for (Imagine::NFmiEsriShape::attributes_type::const_iterator ait =
           attributes.begin();
       ait != attributes.end(); ++ait) {
    if ((*ait)->Name() == theName)
      return (*ait)->Type();
  }
  throw runtime_error("No attribute named '" + theName + "' in the shape");
}

// ----------------------------------------------------------------------
/*!
 * \brief Print the desired attribute values
 */
// ----------------------------------------------------------------------

void print_attributes(const NFmiEsriElement &theElement) {
  vector<string> attribs = NFmiStringTools::Split(options.attributes, ",");

  for (unsigned int i = 0; i < attribs.size(); i++) {
    if (i > 0)
      cout << options.delimiter;

    switch (theElement.GetType(attribs[i])) {
    case kFmiEsriString:
      cout << theElement.GetString(attribs[i]);
      break;
    case kFmiEsriInteger:
      cout << theElement.GetInteger(attribs[i]);
      break;
    case kFmiEsriDouble:
      cout << theElement.GetDouble(attribs[i]);
      break;
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Remove indices which would contain duplicates
 */
// ----------------------------------------------------------------------

void filter_out_duplicates(const NFmiEsriShape &theShape,
                           multimap<float, int> &theData) {
  if (options.uniqueattribute.empty())
    return;

  multimap<float, int> newdata;

  set<string> unique_strings;
  set<int> unique_ints;
  set<double> unique_doubles;

  NFmiEsriAttributeType atype =
      establish_attribute_type(theShape, options.uniqueattribute);

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  for (multimap<float, int>::const_iterator it = theData.begin();
       it != theData.end(); ++it) {
    int pos = it->second;
    const NFmiEsriElement *elem =
        static_cast<const NFmiEsriElement *>(elements[pos]);

    switch (atype) {
    case kFmiEsriString: {
      string value = elem->GetString(options.uniqueattribute);
      if (unique_strings.find(value) == unique_strings.end()) {
        newdata.insert(*it);
        unique_strings.insert(value);
      }
      break;
    }
    case kFmiEsriInteger: {
      int value = elem->GetInteger(options.uniqueattribute);
      if (unique_ints.find(value) == unique_ints.end()) {
        newdata.insert(*it);
        unique_ints.insert(value);
      }
      break;
    }
    case kFmiEsriDouble: {
      double value = elem->GetDouble(options.uniqueattribute);
      if (unique_doubles.find(value) == unique_doubles.end()) {
        newdata.insert(*it);
        unique_doubles.insert(value);
      }
      break;
    }
    }
  }

  theData = newdata;
}

// ----------------------------------------------------------------------
/*!
 * \brief Find nearest points from point shapefile
 *
 * Algorithm:
 *
 * 1. scan through all points
 * 2. calculate the distance
 * 3. if inside search radius, insert into distance sorted map
 * 4. if map size increases above maxcount, drop last point in the map
 * 5. print out the results
 */
// ----------------------------------------------------------------------

void find_nearest_points(const NFmiEsriShape &theShape,
                         const NFmiPoint &theLatLon,
                         const std::string &theName = "") {
  // Projected coordinate if needed

  NFmiPoint worldxy;

  if (options.projection != "latlon")
    worldxy = projection->LatLonToWorldXY(theLatLon);

  // Search results in sorted order

  typedef multimap<float, int> DistanceMap;
  DistanceMap distance_map;

  // Search conditions

  string cond_variable, cond_comparison, cond_value;
  parse_condition(cond_variable, cond_comparison, cond_value);

  // Sort nearest elements

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  for (NFmiEsriShape::elements_type::size_type i = 0; i < elements.size();
       i++) {
    if (elements[i] == 0) // null element?
      continue;

    const NFmiEsriPoint *elem = static_cast<const NFmiEsriPoint *>(elements[i]);

    float dist;
    if (options.projection == "latlon")
      dist = latlon_point_distance(elem->X(), elem->Y(), theLatLon.X(),
                                   theLatLon.Y());
    else
      dist =
          world_point_distance(elem->X(), elem->Y(), worldxy.X(), worldxy.Y());

    if (dist <= options.searchradius) {
      if (condition_satisfied(*elem, cond_variable, cond_comparison,
                              cond_value)) {
        distance_map.insert(DistanceMap::value_type(dist, i));
      }
    }
  }

  // Print the results. Note that we print only the shortest distance for
  // each unique attribute, if so requested.

  filter_out_duplicates(theShape, distance_map);

  // Print the results

  unsigned int num = 0;
  for (DistanceMap::const_iterator it = distance_map.begin();
       it != distance_map.end(); ++it) {
    if (num >= options.maxcount)
      break;

    int pos = it->second;
    const NFmiEsriPoint *elem =
        static_cast<const NFmiEsriPoint *>(elements[pos]);

    double x = elem->X();
    double y = elem->Y();

    if (options.projection != "latlon") {
      NFmiPoint p = projection->WorldXYToLatLon(NFmiPoint(x, y));
      x = p.X();
      y = p.Y();
    }

    if (!theName.empty())
      cout << theName << options.delimiter;

    cout << ++num << options.delimiter << it->first << options.delimiter << x
         << options.delimiter << y << options.delimiter;

    print_attributes(*elem);
    cout << endl;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Find nearest lines from polyline shapefile
 */
// ----------------------------------------------------------------------

void find_nearest_lines(const NFmiEsriShape &theShape,
                        const NFmiPoint &theLatLon,
                        const std::string &theName = "") {
  // Projected coordinate if needed

  NFmiPoint worldxy;

  if (options.projection != "latlon")
    worldxy = projection->LatLonToWorldXY(theLatLon);

  // Search results in sorted order

  typedef multimap<float, int> DistanceMap;
  DistanceMap distance_map;

  // Search conditions

  string cond_variable, cond_comparison, cond_value;
  parse_condition(cond_variable, cond_comparison, cond_value);

  // Sort nearest elements

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  for (NFmiEsriShape::elements_type::size_type i = 0; i < elements.size();
       i++) {
    if (elements[i] == 0) // null element?
      continue;

    const NFmiEsriPolyLine *elem =
        static_cast<const NFmiEsriPolyLine *>(elements[i]);

    float mindist = -1;

    for (int part = 0; part < elem->NumParts(); part++) {
      int i1, i2;
      i1 = elem->Parts()[part]; // start of part
      if (part + 1 == elem->NumParts())
        i2 = elem->NumPoints() - 1; // end of part
      else
        i2 = elem->Parts()[part + 1] - 1; // end of part

      if (i2 >= i1) {
        for (int ii = i1 + 1; ii <= i2; ii++) {
          float dist;
          if (options.projection == "latlon")
            dist = latlon_line_distance(
                theLatLon.X(), theLatLon.Y(), elem->Points()[ii - 1].X(),
                elem->Points()[ii - 1].Y(), elem->Points()[ii].X(),
                elem->Points()[ii].Y());
          else
            dist = world_line_distance(
                worldxy.X(), worldxy.Y(), elem->Points()[ii - 1].X(),
                elem->Points()[ii - 1].Y(), elem->Points()[ii].X(),
                elem->Points()[ii].Y());
          if (mindist < 0 || dist < mindist)
            mindist = dist;
        }
      }
    }

    if (mindist <= options.searchradius) {
      if (condition_satisfied(*elem, cond_variable, cond_comparison,
                              cond_value)) {
        distance_map.insert(DistanceMap::value_type(mindist, i));
      }
    }
  }

  // Print the results. Note that we print only the shortest distance for
  // each unique attribute, if so requested.

  filter_out_duplicates(theShape, distance_map);

  unsigned int num = 0;
  for (DistanceMap::const_iterator it = distance_map.begin();
       it != distance_map.end(); ++it) {
    if (num >= options.maxcount)
      break;

    int pos = it->second;
    const NFmiEsriElement *elem =
        static_cast<const NFmiEsriElement *>(elements[pos]);

    if (!theName.empty())
      cout << theName << options.delimiter;

    cout << ++num << options.delimiter << it->first << options.delimiter;
    print_attributes(*elem);
    cout << endl;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if the given point is inside the polygon
 *
 * Copied from NFmiSvgPath::IsInside
 */
// ----------------------------------------------------------------------

bool is_inside(const NFmiEsriPolygon &thePoly, double theX, double theY) {
  int counter = 0;

  for (int part = 0; part < thePoly.NumParts(); part++) {
    int i1 = thePoly.Parts()[part]; // start of part
    int i2;
    if (part + 1 == thePoly.NumParts())
      i2 = thePoly.NumPoints() - 1; // end of part
    else
      i2 = thePoly.Parts()[part + 1] - 1; // end of part

    if (i2 >= i1) {
      double x1 = thePoly.Points()[i1].X();
      double y1 = thePoly.Points()[i1].Y();

      for (int i = i1 + 1; i <= i2; i++) {
        double x2 = thePoly.Points()[i].X();
        double y2 = thePoly.Points()[i].Y();
        if (theY > std::min(y1, y2) && theY <= std::max(y1, y2) &&
            theX <= std::max(x1, x2) && y1 != y2) {
          const double xinters = (theY - y1) * (x2 - x1) / (y2 - y1) + x1;
          if (x1 == x2 || theX <= xinters)
            counter++;
        }
        x1 = x2;
        y1 = y2;
      }
    }
  }

  return (counter % 2 != 0);
}

// ----------------------------------------------------------------------
/*!
 * \brief Find polygon surrounding point from polygon shapefile
 */
// ----------------------------------------------------------------------

void find_enclosing_polygons(const NFmiEsriShape &theShape,
                             const NFmiPoint &theLatLon,
                             const std::string &theName = "") {
  // Projected coordinate if needed

  NFmiPoint worldxy;

  if (options.projection != "latlon")
    worldxy = projection->LatLonToWorldXY(theLatLon);

  // Search conditions

  string cond_variable, cond_comparison, cond_value;
  parse_condition(cond_variable, cond_comparison, cond_value);

  // Find the first match

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  NFmiEsriShape::elements_type::size_type i;
  for (i = 0; i < elements.size(); i++) {
    if (elements[i] == 0) // null element?
      continue;

    const NFmiEsriPolygon *elem =
        static_cast<const NFmiEsriPolygon *>(elements[i]);

    bool enclosed = false;

    if (condition_satisfied(*elem, cond_variable, cond_comparison,
                            cond_value)) {
      if (options.projection == "latlon")
        enclosed = is_inside(*elem, theLatLon.X(), theLatLon.Y());
      else
        enclosed = is_inside(*elem, worldxy.X(), worldxy.Y());
    }

    if (enclosed)
      break;
  }

  // Print the results.

  if (i < elements.size()) {
    const NFmiEsriPolygon *elem =
        static_cast<const NFmiEsriPolygon *>(elements[i]);

    if (!theName.empty())
      cout << theName << options.delimiter;

    print_attributes(*elem);
    cout << endl;
  } else {
    // No match, print name,-,-,-,-,....
    if (!theName.empty())
      cout << theName << options.delimiter << "-";

    vector<string> attribs = NFmiStringTools::Split(options.attributes, ",");
    for (unsigned j = 0; j < attribs.size(); j++) {
      if (!theName.empty() || j > 0)
        cout << options.delimiter;
      cout << "-";
    }
    cout << endl;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief main progran
 */
// ----------------------------------------------------------------------

int domain(int argc, char *argv[]) {
  if (!parse_options(argc, argv))
    return 0;

  // Read the shape

  NFmiEsriShape shape;
  if (!shape.Read(options.shapefile, true))
    throw runtime_error("Failed to read '" + options.shapefile + "'");

  // Validate search attribute

  establish_attribute(shape);

  // Establish projection for distance calculations

  establish_projection();

  // Establish what to do.
  //
  // Polygons: find polygon surrounding the point
  // Polylines: find nearest polyline
  // Points: find nearest point

  NFmiEsriElementType type = establish_type(shape);

  if (options.coordinatefile.empty()) {
    NFmiPoint latlon(options.longitude, options.latitude);

    if (type == kFmiEsriPoint)
      find_nearest_points(shape, latlon);
    else if (type == kFmiEsriPolyLine)
      find_nearest_lines(shape, latlon);
    else if (type == kFmiEsriPolygon)
      find_enclosing_polygons(shape, latlon);
    else
      throw runtime_error("Internal error while deciding shape type");
  } else {
    LocationList places = read_locationlist(options.coordinatefile);

    for (LocationList::const_iterator it = places.begin(); it != places.end();
         ++it) {
      string name = it->first;
      NFmiPoint latlon = it->second;

      if (type == kFmiEsriPoint)
        find_nearest_points(shape, latlon, name);
      else if (type == kFmiEsriPolyLine)
        find_nearest_lines(shape, latlon, name);
      else if (type == kFmiEsriPolygon)
        find_enclosing_polygons(shape, latlon, name);
      else
        throw runtime_error("Internal error while deciding shape type");
    }
  }

  return 1;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, char *argv[]) {
  try {
    return domain(argc, argv);
  } catch (exception &e) {
    cerr << "Error: " << e.what() << endl;
  } catch (...) {
    cerr << "Error: An unknown exception occurred" << endl;
  }
  return 1;
}
