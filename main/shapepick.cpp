// ======================================================================
/*!
 * \file shapepick.cpp
 * \brief A program to pick a shapepack value for a coordinate
 */
// ======================================================================
/*!
 * \page shapepick shapepick
 *
 * shapepick takes as input latlon coordinates and a shapepack
 * and prints out the shapepack attribute for the coordinate.
 */
// ======================================================================

#include <macgyver/WorldTimeZones.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

// ----------------------------------------------------------------------
/*!
 * \brief Command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  std::string infile;
  double lon;
  double lat;

  Options() : infile(), lon(-1), lat(-1) {}
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
  desc.add_options()("help,h", "print out help message")("version,V", "display version number")(
      "lon", po::value(&options.lon), "longitude")("lat", po::value(&options.lat), "latitude")(
      "infile,i", po::value(&options.infile), "input shapepack");

  po::positional_options_description p;
  p.add("infile", 1);
  p.add("lon", 1);
  p.add("lat", 1);

  po::variables_map opt;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), opt);

  po::notify(opt);

  if (opt.count("version") != 0)
  {
    std::cout << "shapepick v1.0 (" << __DATE__ << ' ' << __TIME__ << ')' << std::endl;
  }

  if (opt.count("help"))
  {
    std::cout << "Usage: shapepick [options] shapepack lon lat" << std::endl
              << std::endl
              << "shapepick picks an attribute from the given shapepack" << std::endl
              << std::endl
              << desc << std::endl;
    return false;
  }

  if (opt.count("infile") == 0) throw std::runtime_error("shapepick name not specified");

  if (opt.count("lon") == 0) throw std::runtime_error("longitude not specified");

  if (opt.count("lat") == 0) throw std::runtime_error("latitude not specified");

  // Check bounding box

  if (options.lon < -180 || options.lon > 180)
    throw std::runtime_error("Longitude option out of bounds");

  if (options.lat < -90 || options.lat > 90)
    throw std::runtime_error("Latitude option out of bounds");

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int run(int argc, char *argv[])
{
  if (!parse_options(argc, argv)) return 0;

  Fmi::WorldTimeZones shape(options.infile);

  std::cout << shape.zone_name(options.lon, options.lat) << std::endl;

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
    return run(argc, argv);
  }
  catch (std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Error: An unknown exception occurred" << std::endl;
  }
  return 1;
}
