// ======================================================================
/*!
 * \file triangle2shape.cpp
 * \brief A program to convert PSLG to shapefiles
 */
// ======================================================================
/*!
 * \page triangle2shape triangle2shape
 *
 * triangle2shape takes as input a name indentifying a set of PSLG files
 * (.node and .poly) and builds closed polygons from them, which are
 * then output as ESRI shapefiles.
 *
 * Usage: triangle2shape [arealimit] [pslg] [shape]
 *
 * The program will generate shape.shp and shape.dbf. The
 * last one is empty.
 *
 */
// ======================================================================

#include "Point.h"
#include "Polygon.h"
#include <imagine/NFmiEdgeTree.h>
#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriShape.h>
#include <macgyver/StringConversion.h>
#include <fstream>
#include <cmath>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
// Read in the nodes
// ----------------------------------------------------------------------

bool read_nodes(const string& inname, vector<Point>& nodes)
{
  string filename = inname + ".node";
  ifstream in(filename.c_str());
  if (!in)
  {
    cerr << "Error: Could not open " << filename << " for reading" << endl;
    return false;
  }
  // First row: number_of_nodes dimension number_of_attributes
  // number_of_boundary_markers

  long number_of_nodes = 0;
  long dummy = 0;
  in >> number_of_nodes >> dummy >> dummy >> dummy;
  nodes.resize(number_of_nodes + 1);  // nodes are numbered starting from 1
  for (long i = 1; i <= number_of_nodes && !in.fail(); i++)
  {
    long node = 0;
    double x = NAN;
    double y = NAN;
    in >> node >> x >> y;
    nodes[i] = Point(x, y);
  }
  if (in.fail())
  {
    cerr << "Error: Error reading " << filename << endl;
    return false;
  }
  in.close();
  return true;
}

// ----------------------------------------------------------------------
// Read the edges
// ----------------------------------------------------------------------

bool read_edges(const string& inname, const vector<Point>& nodes, Imagine::NFmiEdgeTree& edges)
{
  string filename = inname + ".poly";
  ifstream in(filename.c_str());
  if (!in)
  {
    cerr << "Error: Could not open " << filename << " for reading" << endl;
    return false;
  }
  // First row: number_of_nodes dimension number_of_attributes
  // number_of_boundary_markers
  long number_of_nodes = 0;
  long dummy = 0;
  in >> number_of_nodes >> dummy >> dummy >> dummy;
  if (number_of_nodes != 0)
  {
    cerr << "Error: .poly file also containing nodes is not supported" << endl;
    return false;
  }
  // Second row: number_of_edges number_of_boundary_markers
  long number_of_edges = 0;
  in >> number_of_edges >> dummy;
  for (long i = 1; i <= number_of_edges && !in.fail(); i++)
  {
    long edge = 0;
    long idx1 = 0;
    long idx2 = 0;
    in >> edge >> idx1 >> idx2;
    cout << edge << '\t' << idx1 << '\t' << idx2 << endl;

    if (edge != i)
    {
      cerr << "Error: Edges must be numbered sequentially starting from 1 in "
              "file "
           << filename << endl;
      return false;
    }
    Imagine::NFmiEdge tmp(
        nodes[idx1].x(), nodes[idx1].y(), nodes[idx2].x(), nodes[idx2].y(), true, false);
    if (edge == 85)
      cout << "Edge 85!" << endl;
    edges.Add(tmp);
  }

  if (in.fail())
  {
    cerr << "Error: Error reading " << filename << endl;
    return false;
  }
  in.close();
  return true;
}

// ----------------------------------------------------------------------
// Add the polygon to the shape if it is large enough
// ----------------------------------------------------------------------

void add_polygon(Imagine::NFmiEsriShape& shape, const Polygon& poly, double arealimit)
{
  if (arealimit <= 0 || poly.geoarea() >= arealimit)
  {
    // Convert to NFmiEsriPolygon
    auto* p = new Imagine::NFmiEsriPolygon();
    auto b = poly.data().begin();
    auto e = poly.data().end();
    for (auto it = b; it != e; ++it)
      p->Add(Imagine::NFmiEsriPoint(it->x(), it->y()));
    // And add to output
    shape.Add(p);
  }
}

// ----------------------------------------------------------------------
// The main program
// ----------------------------------------------------------------------
int main(int argc, char *argv[])
{
  // Read the command line arguments
  if (argc != 4)
  {
    cerr << "Usage: " << argv[0] << " [arealimit] [pslg] [shape]" << endl;
    return 1;
  }

  double arealimit = Fmi::stod(argv[1]);
  string inname = argv[2];
  string shapename = argv[3];

  // Read in the nodes

  vector<Point> nodes;
  if (!read_nodes(inname, nodes))
    return 1;

  // Read the edges

  Imagine::NFmiEdgeTree edges;
  if (!read_edges(inname, nodes, edges))
    return 1;

  // Build a path from the edges

  Imagine::NFmiPath path = edges.Path();

  // Convert individual closed segments into polygons
  // As soon as a polygon becomes closed, we output
  // it as an ESRI shape

  {
    Imagine::NFmiEsriShape shape;
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
        add_polygon(shape, poly, arealimit);
        poly.clear();
      }

      poly.add(Point((*iter).X(), (*iter).Y()));
      ++iter;
    }

    if (!shape.Write(shapename))
    {
      cerr << "Error while saving the shapefiles" << endl;
      return 1;
    }
  }

  return 0;
}

// ======================================================================
