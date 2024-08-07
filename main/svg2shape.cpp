// ======================================================================
/*!
 * \file svg2shape.cpp
 * \brief A program to convert files generated by shape2svg back to shapes
 *
 * Command line: svg2shape -o foobar -f AREA *.svg
 */
// ======================================================================

#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriShape.h>
#include <newbase/NFmiSvgPath.h>

#include <filesystem>
#include <boost/program_options.hpp>

#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

// ----------------------------------------------------------------------
/*!
 * \brief Command line options
 */
// ----------------------------------------------------------------------

struct Options
{
  Options();
  std::vector<std::string> infiles;
  std::string shapename;  // -o --shape
  std::string fieldname;  // -f --field
};

Options::Options() : infiles(), shapename("out"), fieldname("NAME") {}

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
      "version,V",
      "display version number")("infiles",
                                po::value<std::vector<std::string>>(&options.infiles),
                                "input files")("shape,o",
                                               po::value(&options.shapename),
                                               "shapefile name ('out')")("field,f",
                                                                         po::value(
                                                                             &options.shapename),
                                                                         "field name for the paths "
                                                                         "('NAME')");

  po::positional_options_description p;
  p.add("infiles", -1);

  po::variables_map opt;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), opt);

  po::notify(opt);

  if (opt.count("version") != 0)
  {
    std::cout << "svg2shape v1.0 (" << __DATE__ << ' ' << __TIME__ << ')' << std::endl;
  }

  if (opt.count("help"))
  {
    std::cout << "Usage: svg2shape [options] [infiles]" << std::endl
              << std::endl
              << "svg2shape converts SVG paths generated by shape2svg back to a shape" << std::endl
              << std::endl
              << desc << std::endl;
    return false;
  }

  if (options.infiles.empty())
    throw std::runtime_error("No input files given");

  return true;
}

// ----------------------------------------------------------------------
/*!
 * \brief Read the SVG files
 */
// ----------------------------------------------------------------------

std::map<std::string, NFmiSvgPath> read_files()
{
  std::map<std::string, NFmiSvgPath> paths;

  for (const std::string &s : options.infiles)
  {
    NFmiSvgPath svg;
    std::ifstream in(s.c_str());
    if (!in)
      throw std::runtime_error("Failed to open '" + s + "' for reading");
    svg.Read(in);
    in.close();

    if (svg.empty())
      throw std::runtime_error("File '" + s + "' contained no SVG path data");

    std::filesystem::path p(s);
    std::string stem = p.stem().string();
    paths[stem] = svg;
  }

  return paths;
}

// ----------------------------------------------------------------------
/*!
 * \brief Estabish max name length
 */
// ----------------------------------------------------------------------

std::size_t max_name_length(const std::map<std::string, NFmiSvgPath> &paths)
{
  std::size_t max_length = 0;
  typedef std::map<std::string, NFmiSvgPath>::value_type value_type;
  for (const value_type &v : paths)
  {
    max_length = std::max(max_length, v.first.size());
  }
  return max_length;
}

// ----------------------------------------------------------------------
/*!
 * \brief Make the shape file
 */
// ----------------------------------------------------------------------

void make_shape(const std::map<std::string, NFmiSvgPath> &paths)
{
  using namespace Imagine;

  NFmiEsriShape shape(kFmiEsriPolygon);

  NFmiEsriAttributeName *attribute =
      new NFmiEsriAttributeName(options.fieldname, std::string(), max_name_length(paths));
  shape.Add(attribute);

  typedef std::map<std::string, NFmiSvgPath>::value_type value_type;

  typedef NFmiSvgPath::const_iterator const_iterator;

  for (const value_type &v : paths)
  {
    const std::string &name = v.first;
    const NFmiSvgPath &svg = v.second;

    NFmiEsriPolygon *polygon = new NFmiEsriPolygon;
    NFmiEsriAttribute attr(name, attribute);
    polygon->Add(attr);

    double firstX = 0;
    double firstY = 0;

    const_iterator end = svg.end();
    for (const_iterator it = svg.begin(); it != end; ++it)
    {
      if (it->itsType == NFmiSvgPath::kElementMoveto)
      {
        firstX = it->itsX;
        firstY = it->itsY;
        polygon->AddPart(NFmiEsriPoint(it->itsX, it->itsY));
      }
      else if (it->itsType == NFmiSvgPath::kElementClosePath)
      {
        polygon->Add(NFmiEsriPoint(firstX, firstY));
      }
      else
      {
        polygon->Add(NFmiEsriPoint(it->itsX, it->itsY));
      }
    }

    shape.Add(polygon);
  }

  shape.Write(options.shapename);
}

// ----------------------------------------------------------------------
/*!
 * \brief The main program
 */
// ----------------------------------------------------------------------

int run(int argc, char *argv[])
{
  if (!parse_options(argc, argv))
    return 0;

  // Read the SVG files

  std::map<std::string, NFmiSvgPath> paths = read_files();

  // Make the shape

  make_shape(paths);

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
