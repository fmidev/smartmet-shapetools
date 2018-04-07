// ======================================================================
/*!
 * \file shapepack.cpp
 * \brief A program to convert a shapefile into a packed attribute table
 */
// ======================================================================
/*!
 * \page shapepack shapepack
 *
 * shapepack takes as input a single file shapefile containing latlon
 * polygonal data, the name of the desired attribute, and converts the
 * shape into packed 2D table. The default bounding box for the data
 * is the world from -180 to 180 longitude, -90 to 90 latitude, but
 * it can be altered via the command line.
 *
 * The program was originally created for getting a quick method to
 * determine the timezone of any given coordinate on the planet.
 * The selected attribute was the name of the timezone at the coordinate.
 * For this reason the 2D table is always indexed so that latitude
 * is the inner index in the actual binary encoded table.
 *
 * The format of the shapepacked file is as follows
 * \code
 * SHAPEPACK
 * width_in_pixels height_in_pixels
 * lon1 lat1 lon2 lat2
 * number_of_attributes
 * attribute1
 * attribute2
 * ...
 * attributeN
 * table2d
 * \endcode
 * The header part is in plain ASCII, the 2D table is in the following
 * binary format:
 * \code
 * uint32 number_of_entries
 * uint32 position1 (always 0)
 * uint16 attribute_index_1
 * uint32 position2
 * uint16 attribute_index_2
 * ...
 * uint32 positionN (always width*height)
 * uint16 0
 * \endcode
 * The indices start at one. Zero return values are used to indicate
 * missing data. The final zero is output only for programming convenience.
 *
 * The position for a particular latlon coordinate is calculated as
 * follows:
 * \code
 * ypos = (lat-lat1)/(lat2-lat1)*(height-1)
 * xpos = (lon-lon1)/(lon2-lon1)*(width-1)
 * pos = ypos+xpos*height
 * \endcode
 * ypos and xpos are rounded to nearest integers before the final
 * integer position is calculated.
 *
 */
// ======================================================================

#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriShape.h>
#include <imagine/NFmiFillMap.h>
#include <imagine/NFmiImage.h>

#include <macgyver/WorldTimeZones.h>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

extern "C"
{
#include <stdint.h>
}

using namespace std;
using namespace Imagine;

// ----------------------------------------------------------------------
/*!
 * \brief Command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  string shapefile;
  string packfile;
  string pngfile;
  string zonefile;
  int width;
  int height;
  float lon1;
  float lat1;
  float lon2;
  float lat2;
  string attribute;
  bool verbose;
  bool accurate;

  Options()
      : shapefile(),
        packfile(),
        pngfile(),
        zonefile(),
        width(-1),
        height(-1),
        lon1(-180),
        lat1(-90),
        lon2(180),
        lat2(90),
        attribute(),
        verbose(false),
        accurate(false)
  {
  }
};

Options options;

// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 */
// ----------------------------------------------------------------------

bool parse_options(int argc, char *argv[])
{
  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "print out help message")(
      "verbose,v", po::bool_switch(&options.verbose), "set verbose mode on")(
      "version,V", "display version number")(
      "attribute,a", po::value(&options.attribute), "shapefile attribute name")(
      "shapefile,s", po::value(&options.shapefile), "shapefile (without suffix)")(
      "output,o", po::value(&options.packfile), "output filename")(
      "pngfile,p", po::value(&options.pngfile), "optional image filename")(
      "zonefile,z",
      po::value(&options.zonefile),
      "optional shapepack with which to combine the information with")(
      "width,W", po::value(&options.width), "width of rendered image")(
      "height,H", po::value(&options.height), "height of rendered image")(
      "lon1", po::value(&options.lon1), "bottom left longitude (default is -180)")(
      "lat1", po::value(&options.lat1), "bottom left latitude (default is -90)")(
      "lon2", po::value(&options.lon2), "top right longitude (default is 180)")(
      "lat2", po::value(&options.lat2), "top right latitude (default is 90)")(
      "accurate,A", po::bool_switch(&options.accurate));

  po::positional_options_description p;
  p.add("shapefile", 1);
  p.add("output", 1);

  po::variables_map opt;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), opt);

  po::notify(opt);

  if (opt.count("version") != 0)
  {
    cout << "shapepack v1.0 (" << __DATE__ << ' ' << __TIME__ << ')' << endl;
  }

  if (opt.count("help"))
  {
    cout << "Usage: shapepack [options] shapefile outfile" << endl
         << endl
         << "shapepack picks an attribute from a polygonal shapefile" << endl
         << "and converts the data into a fast access 2D table." << endl
         << endl
         << desc << endl;
    return false;
  }

  if (opt.count("shapefile") == 0) throw runtime_error("shapefile name not specified");

  if (opt.count("output") == 0) throw runtime_error("output name not specified");

  // Check bounding box

  if (options.lon1 >= options.lon2 || options.lat1 >= options.lat2)
    throw runtime_error("Stupid bounding box, fix it");

  // Preserve image aspect if one size attribute is missing

  if (options.width < 0 && options.height < 0)
    throw runtime_error("Either image width or height must be specified");

  if (options.width < 0 && options.height > 0)
  {
    options.width = static_cast<int>(options.height * (options.lon2 - options.lon1) /
                                     (options.lat2 - options.lat1));
  }

  if (options.height < 0 && options.width > 0)
  {
    options.height = static_cast<int>(options.width * (options.lat2 - options.lat1) /
                                      (options.lon2 - options.lon1));
  }

  if (options.height == 0 || options.width == 0)
    throw runtime_error("Try a larger image, this is pointless");

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Get attribute value as string
 */
// ----------------------------------------------------------------------

string get_attribute_value(const NFmiEsriElement &theElement, const NFmiEsriAttributeName &theName)
{
  string name = theName.Name();
  switch (theName.Type())
  {
    case kFmiEsriString:
      return theElement.GetString(name);
    case kFmiEsriInteger:
      return boost::lexical_cast<string>(theElement.GetInteger(name));
    case kFmiEsriDouble:
      return boost::lexical_cast<string>(theElement.GetDouble(name));
    default:
      throw runtime_error("Unknown attribute value type");
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Find the given attribute name
 */
// ----------------------------------------------------------------------

NFmiEsriShape::attributes_type::const_iterator find_attribute(
    const NFmiEsriShape::attributes_type &theAttributes, const std::string &theName)
{
  NFmiEsriShape::attributes_type::const_iterator it = theAttributes.begin();
  for (; it != theAttributes.end(); ++it)
  {
    if ((*it)->Name() == theName) break;
  }
  if (it == theAttributes.end())
    throw runtime_error("No attribute named '" + theName + "' in the shape");
  return it;
}

// ----------------------------------------------------------------------
/*!
 * \brief Find all unique attribute values for the given attribute
 */
// ----------------------------------------------------------------------

set<string> find_unique_attributes(const NFmiEsriShape &theShape, const string &theAttribute)
{
  const NFmiEsriShape::attributes_type &attributes = theShape.Attributes();

  NFmiEsriShape::attributes_type::const_iterator at = find_attribute(attributes, theAttribute);

  set<string> values;

  for (NFmiEsriShape::const_iterator it = theShape.Elements().begin();
       it != theShape.Elements().end();
       ++it)
  {
    if (*it == NULL) continue;
    string value = get_attribute_value(**it, **at);
    values.insert(value);
  }

  return values;
}

// ----------------------------------------------------------------------
/*!
 * \brief Verbose printout of unique attribute values
 */
// ----------------------------------------------------------------------

void print_uniques(const set<string> &theValues)
{
  cout << "There were " << theValues.size() << " unique values in the shape:" << endl;
  int pos = 1;
  for (set<string>::const_iterator it = theValues.begin(); it != theValues.end(); ++it, ++pos)
  {
    cout << pos << ' ' << *it << endl;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Map set of strings to integer indices
 */
// ----------------------------------------------------------------------

map<string, int> make_attribute_map(const set<string> &theSet)
{
  map<string, int> ret;
  int i = 1;
  for (set<string>::const_iterator it = theSet.begin(); it != theSet.end(); ++it)
  {
    ret.insert(map<string, int>::value_type(*it, i++));
  }
  return ret;
}

// ----------------------------------------------------------------------
/*!
 * \brief Pixel scaling
 */
// ----------------------------------------------------------------------

inline float xpixel(float x) { return (x + 180) / 360 * options.width; }

inline float ypixel(float y) { return (y + 90) / 180 * options.height; }

inline float lonpixel(int x) { return (360.0 * x / options.width - 180); }

inline float latpixel(int y) { return (180.0 * y / options.height - 90); }

// ----------------------------------------------------------------------
/*!
 * \brief Insert polygon to fillmap
 */
// ----------------------------------------------------------------------

void polygon_to_fillmap(NFmiFillMap &theMap, const NFmiEsriElement *theElement)
{
  switch (theElement->Type())
  {
    case kFmiEsriPolygon:
    case kFmiEsriPolygonM:
    case kFmiEsriPolygonZ:
    {
      const NFmiEsriPolygon *elem = static_cast<const NFmiEsriPolygon *>(theElement);
      for (int part = 0; part < elem->NumParts(); part++)
      {
        int i1, i2;
        i1 = elem->Parts()[part];  // start of part
        if (part + 1 == elem->NumParts())
          i2 = elem->NumPoints() - 1;  // end of part
        else
          i2 = elem->Parts()[part + 1] - 1;  // end of part

        for (int i = i1 + 1; i <= i2; i++)
          theMap.Add(xpixel(elem->Points()[i - 1].X()),
                     ypixel(elem->Points()[i - 1].Y()),
                     xpixel(elem->Points()[i].X()),
                     ypixel(elem->Points()[i].Y()));
      }
      break;
    }
    default:
      break;
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Render the image to be compressed
 */
// ----------------------------------------------------------------------

void render_image(NFmiImage &theImage,
                  const NFmiEsriShape &theShape,
                  const map<string, int> &theValues)
{
  if (options.verbose)
  {
    cout << "Rendering " << theImage.Width() << "x" << theImage.Height()
         << " size image, this may take a while" << endl;
  }

  // Attribute information

  const NFmiEsriShape::attributes_type &attributes = theShape.Attributes();

  NFmiEsriShape::attributes_type::const_iterator at = find_attribute(attributes, options.attribute);

  for (NFmiEsriShape::const_iterator it = theShape.Elements().begin();
       it != theShape.Elements().end();
       ++it)
  {
    if (*it == NULL) continue;

    const string value = get_attribute_value(**it, **at);
    NFmiColorTools::Color color = theValues.find(value)->second;

    NFmiFillMap fillmap;
    polygon_to_fillmap(fillmap, *it);
    fillmap.Fill(theImage, color, NFmiColorTools::kFmiColorCopy);
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if the given point is inside the polygon
 *
 * Copied from shapefind.cpp
 */
// ----------------------------------------------------------------------

bool is_inside(const NFmiEsriPolygon &thePoly, double theX, double theY)
{
  int counter = 0;

  for (int part = 0; part < thePoly.NumParts(); part++)
  {
    int i1 = thePoly.Parts()[part];  // start of part
    int i2;
    if (part + 1 == thePoly.NumParts())
      i2 = thePoly.NumPoints() - 1;  // end of part
    else
      i2 = thePoly.Parts()[part + 1] - 1;  // end of part

    if (i2 >= i1)
    {
      double x1 = thePoly.Points()[i1].X();
      double y1 = thePoly.Points()[i1].Y();

      for (int i = i1 + 1; i <= i2; i++)
      {
        double x2 = thePoly.Points()[i].X();
        double y2 = thePoly.Points()[i].Y();
        if (theY > std::min(y1, y2) && theY <= std::max(y1, y2) && theX <= std::max(x1, x2) &&
            y1 != y2)
        {
          const double xinters = (theY - y1) * (x2 - x1) / (y2 - y1) + x1;
          if (x1 == x2 || theX <= xinters) counter++;
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

string find_enclosing_polygon(const NFmiEsriShape &theShape, float theLon, float theLat)
{
  // Find the first match

  const NFmiEsriShape::elements_type &elements = theShape.Elements();

  NFmiEsriShape::elements_type::size_type i;
  for (i = 0; i < elements.size(); i++)
  {
    if (elements[i] == 0)  // null element?
      continue;

    const NFmiEsriPolygon *elem = static_cast<const NFmiEsriPolygon *>(elements[i]);

    bool enclosed = is_inside(*elem, theLon, theLat);

    if (enclosed) break;
  }

  // Print the results.

  if (i < elements.size())
  {
    const NFmiEsriPolygon *elem = static_cast<const NFmiEsriPolygon *>(elements[i]);
    const NFmiEsriShape::attributes_type &attributes = theShape.Attributes();
    NFmiEsriShape::attributes_type::const_iterator at =
        find_attribute(attributes, options.attribute);
    const string value = get_attribute_value(*elem, **at);
    return value;
  }
  else
    return "";
}

// ----------------------------------------------------------------------
/*!
 * \brief Test if pixel color is the same if the coordinate is inside image
 *
 * We intentionally return true for pixels outside the image.
 */
// ----------------------------------------------------------------------

inline bool same_color(const NFmiImage &theImage, int i, int j, NFmiColorTools::Color c)
{
  return (i < 0 || j < 0 || i >= theImage.Width() || j >= theImage.Height() || theImage(i, j) == c);
}

// ----------------------------------------------------------------------
/*!
 * \brief Check if pixel is surrounded by indentical pixels or not
 */
// ----------------------------------------------------------------------

inline bool is_boundary_pixel(const NFmiImage &theImage, int i, int j)
{
  const NFmiColorTools::Color color = theImage(i, j);

  return !(same_color(theImage, i - 1, j - 1, color) && same_color(theImage, i + 1, j - 1, color) &&
           same_color(theImage, i, j - 1, color) && same_color(theImage, i - 1, j, color) &&
           same_color(theImage, i + 1, j, color) && same_color(theImage, i - 1, j + 1, color) &&
           same_color(theImage, i, j + 1, color) && same_color(theImage, i + 1, j + 1, color));
}

// ----------------------------------------------------------------------
/*!
 * \brief Check accuracy of image near borders
 */
// ----------------------------------------------------------------------

void refine_image(NFmiImage &theImage,
                  const NFmiEsriShape &theShape,
                  const map<string, int> &theValues)
{
  int checks = 0;
  int changes = 0;
  int pixels = 0;

  int percentage = 0;
  int perstep = 1;
  int total_pixels = theImage.Width() * theImage.Height();

  if (options.verbose) cout << "Validating border areas..." << endl;

  for (int i = 0; i < theImage.Width(); i++)
    for (int j = 0; j < theImage.Height(); j++)
    {
      ++pixels;
      if (pixels * 100.0 / total_pixels - percentage > perstep)
      {
        percentage += perstep;
        if (options.verbose) cout << "\t" << percentage << "%" << endl;
      }

      if (is_boundary_pixel(theImage, i, j))
      {
        ++checks;

        string tz = find_enclosing_polygon(theShape, lonpixel(i), latpixel(j));

        if (!tz.empty())
        {
          int idx = theValues.find(tz)->second;
          if (theImage(i, j) != idx)
          {
            if (options.verbose)
              cout << "Changed " << i << "," << j << " value from " << theImage(i, j) << " to "
                   << idx << " (" << tz << ")" << endl;
            ++changes;
            theImage(i, j) = idx;
          }
        }
      }
    }

  if (options.verbose)
    cout << "Total checks:  " << checks << endl << "Total changes: " << changes << endl;
}

// ----------------------------------------------------------------------
/*!
 * \brief Output uint32 binary representation
 */
// ----------------------------------------------------------------------

void output_uint32(std::ostream &out, uint32_t theValue)
{
  uint32_t value = theValue;
  out.write(reinterpret_cast<char *>(&value), sizeof(uint32_t));
}

// ----------------------------------------------------------------------
/*!
 * \brief Output uint16 binary representation
 */
// ----------------------------------------------------------------------

void output_uint16(std::ostream &out, uint16_t theValue)
{
  uint16_t value = theValue;
  out.write(reinterpret_cast<char *>(&value), sizeof(uint16_t));
}

// ----------------------------------------------------------------------
/*!
 * \brief Compress the image
 */
// ----------------------------------------------------------------------

string compress_image(const NFmiImage &theImage, const map<string, int> &theMap)
{
  // Header
  ostringstream out;
  out << "SHAPEPACK\n"
      << theImage.Width() << ' ' << theImage.Height() << '\n'
      << options.lon1 << ' ' << options.lat1 << ' ' << options.lon2 << ' ' << options.lat2 << '\n';

  // Attribute index

  out << theMap.size() << '\n';

  for (map<string, int>::const_iterator it = theMap.begin(); it != theMap.end(); ++it)
  {
    out << it->first << '\n';
  }

  // Raw data

  ostringstream table;
  int startpos = 0;
  int entries = 0;
  int lastcolor = theImage(0, 0);
  int duplicates = 0;

#if 1
  std::cout << "Unique values:" << std::endl;
  std::set<int> uniques;
  int last = -1000;
  for (int i = 0; i < theImage.Width(); i++)
    for (int j = 0; j < theImage.Height(); j++)
    {
      if (theImage(i, j) != last) uniques.insert(theImage(i, j));
      last = theImage(i, j);
    }
  BOOST_FOREACH (int c, uniques)
    std::cout << c << std::endl;
#endif

  if (options.verbose) std::cout << "Compressing the image data" << std::endl;

  for (int i = 0; i < theImage.Width(); i++)
    for (int j = 0; j < theImage.Height(); j++)
    {
      if (theImage(i, j) == lastcolor)
        ++duplicates;
      else
      {
        ++entries;
        output_uint32(table, startpos);
        output_uint16(table, lastcolor);
        startpos = j + i * theImage.Height();
        lastcolor = theImage(i, j);
      }
    }
  // Flush remaining line
  if (duplicates > 0)
  {
    output_uint32(table, startpos);
    output_uint16(table, lastcolor);
  }
  // And terminate table
  output_uint32(table, theImage.Width() * theImage.Height());
  output_uint16(table, 0);

  // Final output

  output_uint32(out, entries);
  out << table.str();

  return out.str();
}

// ----------------------------------------------------------------------
/*!
 * \brief Render a shapepack onto an image
 */
// ----------------------------------------------------------------------

void render_shapepack(NFmiImage &img,
                      const Fmi::WorldTimeZones &zones,
                      const map<string, int> &attmap)
{
  if (options.verbose) std::cout << "Rendering background shapepack" << std::endl;

  for (int i = 0; i < img.Width(); i++)
    for (int j = 0; j < img.Height(); j++)
    {
      try
      {
        std::string tz = zones.zone_name(lonpixel(i), latpixel(j));
        map<string, int>::const_iterator it = attmap.find(tz);
        if (it == attmap.end())
        {
          cerr << "Failed to find index for timezone " << tz << " at coordinate " << i << "," << j
               << " at lonlat " << lonpixel(i) << "," << latpixel(j) << endl;
        }
        else
          img(i, j) = attmap.find(tz)->second;
      }
      catch (...)
      {
      }
    }
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 *
 * -# read the shapefile
 * -# verify chosen attribute is valid
 * -# list all unique attribute values
 * -# render the polygons using attribute indices as values
 * -# compress the image
 * -# output the image
 */
// ----------------------------------------------------------------------

int domain(int argc, char *argv[])
{
  if (!parse_options(argc, argv)) return 0;

  // Read the shape

  NFmiEsriShape shape;
  if (!shape.Read(options.shapefile, true))
    throw runtime_error("Failed to read '" + options.shapefile + "'");

  const NFmiEsriShape::attributes_type &attributes = shape.Attributes();

  if (attributes.size() == 0) throw runtime_error("shapefile does not contain any attributes");

  // If no attribute option was given, we accept it as long as
  // the shape contains exactly one attribute

  if (options.attribute.empty())
  {
    if (attributes.size() > 1)
    {
      ostringstream out;
      out << "shapefile contains multiple attributes, choose one: ";
      int pos = 0;
      for (NFmiEsriShape::attributes_type::const_iterator at = attributes.begin();
           at != attributes.end();
           ++at, ++pos)
      {
        if (pos > 0) out << ",";
        out << (*at)->Name();
      }

      throw runtime_error(out.str());
    }
    options.attribute = (*attributes.begin())->Name();
  }

  // Help informatio

  boost::shared_ptr<Fmi::WorldTimeZones> zones;
  if (!options.zonefile.empty()) zones.reset(new Fmi::WorldTimeZones(options.zonefile));

  // Unique attributes

  set<string> uniques = find_unique_attributes(shape, options.attribute);

  if (options.verbose) print_uniques(uniques);

  if (zones)
  {
    BOOST_FOREACH (const string &z, zones->zones())
      uniques.insert(z);
  }

  map<string, int> attmap = make_attribute_map(uniques);

  if (options.verbose) print_uniques(uniques);

  // Render the image

  NFmiImage img(options.width, options.height, -1);
  if (zones) render_shapepack(img, *zones, attmap);

  render_image(img, shape, attmap);

  if (options.accurate) refine_image(img, shape, attmap);

  if (!options.pngfile.empty()) img.WritePng(options.pngfile);

  // Compress the data

  string data = compress_image(img, attmap);

  ofstream out(options.packfile.c_str());
  if (!out) throw runtime_error("Could not open '" + options.packfile + "' for writing");
  out << data;
  out.close();

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, char *argv[])
{
  try
  {
    return domain(argc, argv);
  }
  catch (exception &e)
  {
    cerr << "Error: " << e.what() << endl;
  }
  catch (...)
  {
    cerr << "Error: An unknown exception occurred" << endl;
  }
  return 1;
}
