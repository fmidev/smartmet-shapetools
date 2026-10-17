// ======================================================================
/*!
 * \file shape2triangle.cpp
 * \brief A program to convert shapefiles into PSLG format
 */
// ======================================================================
/*!
 * \page shape2triangle shape2triangle
 *
 * shap2triangle taks as input a filename identifying a set of ESRI
 * shapefiles, and outputs respective PSLG files to be used with
 * the Delaunay triangulation package by Jonathan R. Shewchuk.
 *
 * Usage: shape2triangle [arealimit] [shape] [outname]
 *
 * The program will generate outname.node and outname.poly files.
 * Any polygon smaller than the given area limit is not output.
 */
// ======================================================================

#include "Nodes.h"
#include "Polygon.h"
#include <macgyver/StringConversion.h>
#include <imagine/NFmiGeoShape.h>
#include <imagine/NFmiPath.h>
#include <newbase/NFmiArea.h>
#include <newbase/NFmiFileSystem.h>
#include <newbase/NFmiValueString.h>
// system
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
// Create a vector of polygons large enough
// ----------------------------------------------------------------------

vector<Polygon> collect_polygons(Imagine::NFmiGeoShape& geo, double arealimit)
{
  vector<Polygon> polygons;
  Imagine::NFmiPath path = geo.Path();
  Polygon poly;
  const auto begin = path.Elements().begin();
  const auto end = path.Elements().end();

  for (Imagine::NFmiPathData::const_iterator iter = begin; iter != end;)
  {
    bool doflush = false;
    // Huom! Jostain syyst‰ g++ k‰‰nt‰‰ v‰‰rin (iter++==end), pakko tehd‰ n‰in
    if ((*iter).Oper() == Imagine::kFmiMoveTo)
      doflush = true;
    else if (++iter == end)
    {
      --iter;
      poly.add(Point((*iter).X(), (*iter).Y()));
      doflush = true;
    }
    else
      --iter;

    if (doflush && !poly.empty())
    {
      if (arealimit <= 0 || poly.geoarea() >= arealimit)
        polygons.push_back(poly);
      poly.clear();
    }

    poly.add(Point((*iter).X(), (*iter).Y()));
    ++iter;
  }
  return polygons;
}

// ----------------------------------------------------------------------
// Create a table of unique nodes from the accepted polygons
// ----------------------------------------------------------------------

void collect_nodes(const vector<Polygon>& polygons, Nodes& nodes)
{
  const auto begin = polygons.begin();
  const auto end = polygons.end();
  unsigned long idx = 0;
  for (vector<Polygon>::const_iterator iter = begin; iter != end; ++iter)
  {
    ++idx;
    const auto pbegin = iter->data().begin();
    const auto pend = iter->data().end();
    for (Polygon::DataType::const_iterator piter = pbegin; piter != pend; ++piter)
      static_cast<void>(nodes.add(*piter, idx));
  }
}

// ----------------------------------------------------------------------
// Output a file containing all the nodes
// ----------------------------------------------------------------------

bool write_nodes(const string& outname, const Nodes& nodes)
{
  string nodefile = outname + ".node";
  cout << "Writing " << nodefile << endl;
  ofstream out(nodefile.c_str());
  if (!out)
  {
    cerr << "Error: Could not open " << nodefile << " for writing" << endl;
    return false;
  }

  // PSLG syntax has numbers for each point, but triangle seems to assume
  // the lines have been sorted.

  map<long, Point> sortednodes;
  const Nodes::DataType& data = nodes.data();
  {
    const auto begin = data.begin();
    const auto end = data.end();
    for (Nodes::DataType::const_iterator iter = begin; iter != end; ++iter)
      sortednodes.insert(make_pair(iter->second.first, iter->first));
  }
  // #points dimension #attributes #boundarymarkers
  out << data.size() << " 2 1 0" << endl;
  {
    const auto begin = sortednodes.begin();
    const auto end = sortednodes.end();
    for (map<long, Point>::const_iterator iter = begin; iter != end; ++iter)
      out << iter->first << '\t' << NFmiValueString(iter->second.x()).CharPtr() << '\t'
          << NFmiValueString(iter->second.y()).CharPtr() << '\t' << nodes.id(iter->second) << endl;
  }
  out.close();
  return true;
}

// ----------------------------------------------------------------------
// Output the edges of the polygons
// ----------------------------------------------------------------------

void write_edges(ostream& out, const vector<Polygon>& polygons, const Nodes& nodes)
{
  long number_of_edges = 0;
  for (unsigned int pass = 1; pass <= 2; pass++)
  {
    long edge = 0;
    if (pass == 2)
    {
      out << "0 2 0 0" << endl;  // no nodes in .poly
      out << number_of_edges << " 0" << endl;
    }

    const auto begin = polygons.begin();
    const auto end = polygons.end();
    for (vector<Polygon>::const_iterator iter = begin; iter != end; ++iter)
    {
      Point previous_point(0, 0);
      const auto pbegin = iter->data().begin();
      const auto pend = iter->data().end();
      for (Polygon::DataType::const_iterator piter = pbegin; piter != pend; ++piter)
      {
        if (piter != pbegin)
        {
          if (pass == 1)
            number_of_edges++;
          else
          {
            out << ++edge << '\t' << nodes.number(previous_point) << '\t' << nodes.number(*piter)
                << endl;
          }
        }
        if (pass == 2)
          previous_point = *piter;
      }
    }
  }
}

// ----------------------------------------------------------------------
// For each polygon we output a single point inside the polygon.
// The attribute is the ordinal of the polygon itself. This
// is sufficient to uniquely identify all triangles in the
// subsequent triangulation as belonging to some original
// polygon, provided no polygon encloses another one.
// ----------------------------------------------------------------------

void write_inside_points(ostream& out, const vector<Polygon>& polygons)
{
  cout << "Finding an inside point for " << polygons.size() << " polygons" << endl;
  out << polygons.size() << endl;
  const auto begin = polygons.begin();
  const auto end = polygons.end();
  long poly = 0;
  for (vector<Polygon>::const_iterator iter = begin; iter != end; ++iter)
  {
    ++poly;
    Point pt = iter->someInsidePoint();
    out << poly << '\t' << NFmiValueString(pt.x()).CharPtr() << '\t'
        << NFmiValueString(pt.y()).CharPtr() << '\t' << poly << endl;
  }
}

// ----------------------------------------------------------------------
// Output a file containing all the edges
// ----------------------------------------------------------------------

bool write_poly(const string& outname, const vector<Polygon>& polygons, const Nodes& nodes)
{
  string polyfile = outname + ".poly";
  cout << "Writing " << polyfile << endl;
  ofstream out(polyfile.c_str());
  if (!out)
  {
    cerr << "Error: Could not open " << polyfile << " for writing" << endl;
    return false;
  }

  write_edges(out, polygons, nodes);

  // No holes
  out << "0" << endl;

  // For each polygon we output a single point inside the polygon.

  write_inside_points(out, polygons);

  out.close();
  return true;
}

// ----------------------------------------------------------------------
// The main program
// ----------------------------------------------------------------------
int main(int argc, const char* argv[])
{
  // Read the command line arguments
  if (argc != 4)
  {
    cerr << "Usage: " << argv[0] << " [arealimit] [shape] [outname]" << endl;
    return 1;
  }
  double arealimit = Fmi::stod(argv[1]);
  string shapefile = argv[2];
  string outname = argv[3];

  // Read the shapefile
  cout << "Reading shapefile " << shapefile << endl;
  Imagine::NFmiGeoShape geo(shapefile, Imagine::kFmiGeoShapeEsri);

  // Create a vector of polygons
  cout << "Collecting polygons large enough" << endl;
  vector<Polygon> polygons = collect_polygons(geo, arealimit);
  cout << "Found " << polygons.size() << " large enough polygons" << endl;

  // Create a table of unique nodes from the accepted polygons

  cout << "Calculating unique nodes" << endl;
  Nodes nodes;
  collect_nodes(polygons, nodes);
  cout << "Counted " << nodes.data().size() << " nodes" << endl;

  // Output a file containing all the nodes
  if (!write_nodes(outname, nodes))
    return 1;

  // Output a file containing all the edges
  if (!write_poly(outname, polygons, nodes))
    return 1;

  cout << "Done" << endl;
}
