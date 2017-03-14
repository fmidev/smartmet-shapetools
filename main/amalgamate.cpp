// ======================================================================
/*!
 * \file amalgamate.cpp
 * \brief A program to amalgamate PSLG files
 */
// ======================================================================
/*!
 * \page amalgamate amalgamate
 *
 * triangle2shape takes as input a name indentifying a set of PSLG files
 * (.node .poly and .ele), a limiting distance for the edges of the
 * triangles, and outputs a new set of PSLG files.
 *
 * Usage: amalgamate [lengthlimit] [arealimit] [inputname] [outputname]
 *
 * One may wish to use inputname.1 as outputname so that the
 * triangle visualization program can be used to visualize input
 * and output simultaneously.
 *
 * The program outputs .node and .poly files only. The respective
 * new .ele file can of course be created with triangle -cpI outputname,
 * if necessary.
 *
 * If outputname is -debug, the .ele file will be overwritten by
 * a new one containing the triangles accepted for the amalgamation,
 * the .node and .poly files will remain the same.
 */
// ======================================================================

#include "Nodes.h"
#include "Edges.h"
#include "Polygon.h"
#include <imagine/NFmiEdgeTree.h>
#include <newbase/NFmiValueString.h>
#include <fstream>
#include <string>
#include <set>

using namespace std;

// ----------------------------------------------------------------------
// The main program
// ----------------------------------------------------------------------
int main(int argc, char *argv[]) {
  // Read the command line arguments
  if (argc != 5) {
    cerr << "Usage: " << argv[0]
         << " [lengthlimit] [arealimit] [input] [output]" << endl;
    return 1;
  }

  double lengthlimit = atof(argv[1]);
  double arealimit = atof(argv[2]);
  string inname = argv[3];
  string outname = argv[4];

  bool debug = (outname == "-debug");
  cout << "debug = " << debug << endl;

  // Read in the nodes

  Nodes inodes;
  {
    string filename = inname + ".node";
    ifstream in(filename.c_str());
    if (!in) {
      cerr << "Error: Could not open " << filename << " for reading" << endl;
      return 1;
    }
    // First row: #nodes dimension #attributes #boundary_markers

    long number_of_nodes, number_of_attributes, dummy;
    in >> number_of_nodes >> dummy >> number_of_attributes >> dummy;
    if (number_of_attributes != 1) {
      cerr << "Error: " << filename
           << " must contain exactly one attribute field" << endl;
      return 1;
    }

    cout << "Reading " << number_of_nodes << " nodes from " << filename << endl;
    for (long i = 1; i <= number_of_nodes && !in.fail(); i++) {
      long node, id;
      double x, y;
      in >> node >> x >> y >> id;
      static_cast<void>(inodes.add(Point(x, y), id));
    }
    if (in.fail()) {
      cerr << "Error: Error reading " << filename << endl;
      return 1;
    }
    in.close();
  }

  // Read the edges

  Edges constraints;
  {
    string filename = inname + ".poly";
    ifstream in(filename.c_str());
    if (!in) {
      cerr << "Error: Could not open " << filename << " for reading" << endl;
      return 1;
    }
    // First row: number_of_nodes dimension number_of_attributes
    // number_of_boundary_markers
    long number_of_nodes, dummy;
    in >> number_of_nodes >> dummy >> dummy >> dummy;
    if (number_of_nodes != 0) {
      cerr << "Error: .poly file also containing nodes is not supported"
           << endl;
      return 1;
    }
    // Second row: number_of_edges number_of_boundary_markers
    long number_of_edges;
    in >> number_of_edges >> dummy;
    cout << "Reading " << number_of_edges << " edges from " << filename << endl;
    for (long i = 1; i <= number_of_edges && !in.fail(); i++) {
      long edge, idx1, idx2;
      in >> edge >> idx1 >> idx2;
      if (edge != i) {
        cerr << "Error: Edges must be numbered sequentially starting from 1 in "
                "file "
             << filename << endl;
        return 1;
      }
      constraints.add(Edge(idx1, idx2));
    }

    if (in.fail()) {
      cerr << "Error: Error reading " << filename << endl;
      return 1;
    }
    in.close();
  }

  // Read the .ele file and build an edge tree obeying the area constraints

  Imagine::NFmiEdgeTree edges;
  {
    string filename = inname + ".ele";
    ifstream in(filename.c_str());
    if (!in) {
      cerr << "Error: Could not open " << filename << " for reading" << endl;
      return 1;
    }
    // First row: #triangles #points #attributes

    long number_of_triangles, number_of_points, dummy;
    in >> number_of_triangles >> number_of_points >> dummy;
    if (number_of_points != 3) {
      cerr << "Error: " << filename << " must have 3 points per line only"
           << endl;
      return 1;
    }
    cout << "Reading " << number_of_triangles << " triangles from " << filename
         << endl;

    string debug_buffer;
    long debug_triangles = 0;

    for (long i = 1; i <= number_of_triangles && !in.fail(); i++) {
      long edge, idx1, idx2, idx3, region;
      in >> edge >> idx1 >> idx2 >> idx3 >> region;

      bool triangle_ok = true;

      // If all are from the same polygon, output triangle

      const Point &pt1 = inodes.point(idx1);
      const Point &pt2 = inodes.point(idx2);
      const Point &pt3 = inodes.point(idx3);

      if (region == 0) {
        triangle_ok &= (pt1.geodistance(pt2) <= lengthlimit);
        triangle_ok &= (pt2.geodistance(pt3) <= lengthlimit);
        triangle_ok &= (pt3.geodistance(pt1) <= lengthlimit);
      }

      if (triangle_ok) {
        edges.Add(
            Imagine::NFmiEdge(pt1.x(), pt1.y(), pt2.x(), pt2.y(), true, false));
        edges.Add(
            Imagine::NFmiEdge(pt2.x(), pt2.y(), pt3.x(), pt3.y(), true, false));
        edges.Add(
            Imagine::NFmiEdge(pt3.x(), pt3.y(), pt1.x(), pt1.y(), true, false));

        if (debug) {
          ++debug_triangles;
          debug_buffer += static_cast<char *>(NFmiValueString(debug_triangles));
          debug_buffer += '\t';
          debug_buffer += static_cast<char *>(NFmiValueString(idx1));
          debug_buffer += '\t';
          debug_buffer += static_cast<char *>(NFmiValueString(idx2));
          debug_buffer += '\t';
          debug_buffer += static_cast<char *>(NFmiValueString(idx3));
          debug_buffer += '\t';
          debug_buffer += static_cast<char *>(NFmiValueString(region));
          debug_buffer += '\n';
        }
      }
    }
    if (in.fail()) {
      cerr << "Error: Error reading " << filename << endl;
      return 1;
    }
    in.close();

    if (debug) {
      cout << "Writing " << filename << endl;
      ofstream out(filename.c_str());
      out << debug_triangles << " 3 1" << endl << debug_buffer << endl;
      out.close();
    }
  }

  // Build a path from the remaining edges

  cout << "Building a path" << endl;
  Imagine::NFmiPath path = edges.Path();

  // Preserve all big enough polygons in the path

  cout << "Collecting polygons large enough" << endl;
  vector<Polygon> polygons;
  {
    Polygon poly;
    const Imagine::NFmiPathData::const_iterator begin = path.Elements().begin();
    const Imagine::NFmiPathData::const_iterator end = path.Elements().end();

    for (Imagine::NFmiPathData::const_iterator iter = begin; iter != end;) {
      bool doflush = false;
      // Huom! Jostain syyst‰ g++ k‰‰nt‰‰ v‰‰rin (iter++==end), pakko
      // tehd‰ n‰in
      if ((*iter).Oper() == Imagine::kFmiMoveTo)
        doflush = true;
      else if (++iter == end) {
        --iter;
        poly.add(Point((*iter).X(), (*iter).Y()));
        doflush = true;
      } else
        --iter;

      if (doflush && !poly.empty()) {
        if (arealimit <= 0 || poly.geoarea() >= arealimit)
          polygons.push_back(poly);
        poly.clear();
      }

      poly.add(Point((*iter).X(), (*iter).Y()));
      ++iter;
    }
  }
  cout << "Found " << polygons.size() << " large enough polygons" << endl;

  // Establish all the nodes in the path and assign numbers to them

  cout << "Calculating unique nodes" << endl;
  Nodes nodes;
  {
    const vector<Polygon>::const_iterator begin = polygons.begin();
    const vector<Polygon>::const_iterator end = polygons.end();
    unsigned long idx = 0;
    for (vector<Polygon>::const_iterator iter = begin; iter != end; ++iter) {
      ++idx;
      const Polygon::DataType::const_iterator pbegin = iter->data().begin();
      const Polygon::DataType::const_iterator pend = iter->data().end();
      for (Polygon::DataType::const_iterator piter = pbegin; piter != pend;
           ++piter)
        static_cast<void>(nodes.add(*piter, idx));
    }
  }
  cout << "Counted " << nodes.data().size() << " nodes" << endl;

  // Output .node

  if (!debug) {
    string nodefile = outname + ".node";
    ofstream out(nodefile.c_str());
    if (!out) {
      cerr << "Error: Could not open " << nodefile << " for writing" << endl;
      return 1;
    }

    // PSLG syntax has numbers for each point, but triangle seems to assume
    // the lines have been sorted.

    map<unsigned long, Point> sortednodes;
    const Nodes::DataType &data = nodes.data();
    {
      const Nodes::DataType::const_iterator begin = data.begin();
      const Nodes::DataType::const_iterator end = data.end();
      for (Nodes::DataType::const_iterator iter = begin; iter != end; ++iter)
        sortednodes.insert(make_pair(iter->second.first, iter->first));
    }
    cout << "Writing " << nodefile << " with " << data.size() << " nodes"
         << endl;
    out << data.size() << " 2 0 0" << endl;
    {
      const map<unsigned long, Point>::const_iterator begin =
          sortednodes.begin();
      const map<unsigned long, Point>::const_iterator end = sortednodes.end();
      for (map<unsigned long, Point>::const_iterator iter = begin; iter != end;
           ++iter)
        out << iter->first << '\t' << iter->second.x() << '\t'
            << iter->second.y() << endl;
    }
    out.close();
  }

  // Output .poly

  if (!debug) {
    string polyfile = outname + ".poly";
    cout << "Writing " << polyfile << endl;
    ofstream out(polyfile.c_str());
    if (!out) {
      cerr << "Error: Could not open " << polyfile << " for writing" << endl;
      return 1;
    }

    long number_of_edges = 0;
    for (unsigned int pass = 1; pass <= 2; pass++) {
      long edge = 0;
      if (pass == 2) {
        out << "0 2 0 0" << endl; // no nodes in .poly
        out << number_of_edges << " 0" << endl;
      }

      const vector<Polygon>::const_iterator begin = polygons.begin();
      const vector<Polygon>::const_iterator end = polygons.end();
      for (vector<Polygon>::const_iterator iter = begin; iter != end; ++iter) {
        Point previous_point(0, 0);
        const Polygon::DataType::const_iterator pbegin = iter->data().begin();
        const Polygon::DataType::const_iterator pend = iter->data().end();
        for (Polygon::DataType::const_iterator piter = pbegin; piter != pend;
             ++piter) {
          if (piter != pbegin) {
            if (pass == 1)
              number_of_edges++;
            else {
              out << ++edge << '\t' << nodes.number(previous_point) << '\t'
                  << nodes.number(*piter) << endl;
            }
          }
          if (pass == 2)
            previous_point = *piter;
        }
      }
    }

    // No holes
    out << "0" << endl;
    out.close();
  }

  cout << "Done" << endl;

  return 0;
}
