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
#include "imagine/NFmiEdgeTree.h"
#include "imagine/NFmiEsriShape.h"
#include "imagine/NFmiEsriPolygon.h"
#include <iostream>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
// The main program
// ----------------------------------------------------------------------
int main(int argc, char * argv[])
{
  // Read the command line arguments
  if(argc!=4)
	{
	  cerr << "Usage: " << argv[0] << " [arealimit] [pslg] [shape]" << endl;
	  return 1;
	}

  double arealimit = atof(argv[1]);
  string inname = argv[2];
  string shapename = argv[3];

  // Read in the nodes

  vector<Point> nodes;
  {
	string filename = inname + ".node";
	ifstream in(filename.c_str());
	if(!in)
	  {
		cerr << "Error: Could not open " << filename << " for reading" << endl;
		return 1;
	  }
	// First row: number_of_nodes dimension number_of_attributes number_of_boundary_markers

	long number_of_nodes, dummy;
	in >> number_of_nodes >> dummy >> dummy >> dummy;
	nodes.reserve(number_of_nodes);
	for(long i=1; i<=number_of_nodes  && !in.fail(); i++)
	  {
		long node;
		double x,y;
		in >> node >> x >> y;
		nodes[i] = Point(x,y);
	  }
	if(in.fail())
	  {
		cerr << "Error: Error reading " << filename << endl;
		return 1;
	  }
	in.close();
  }

  // Read the edges

  Imagine::NFmiEdgeTree edges;
  {
	string filename = inname + ".poly";
	ifstream in(filename.c_str());
	if(!in)
	  {
		cerr << "Error: Could not open " << filename << " for reading" << endl;
		return 1;
	  }
	// First row: number_of_nodes dimension number_of_attributes number_of_boundary_markers
	long number_of_nodes, dummy;
	in >> number_of_nodes >> dummy >> dummy >> dummy;
	if(number_of_nodes!=0)
	  {
		cerr << "Error: .poly file also containing nodes is not supported" << endl;
		return 1;
	  }
	// Second row: number_of_edges number_of_boundary_markers
	long number_of_edges;
	in >> number_of_edges >> dummy;
	for(long i=1; i<=number_of_edges && !in.fail(); i++)
	  {
		long edge, idx1, idx2;
		in >> edge >> idx1 >> idx2;
		cout << edge << '\t'
			 << idx1 << '\t'
			 << idx2 << endl;

		if(edge != i)
		  {
			cerr << "Error: Edges must be numbered sequentially starting from 1 in file "
				 << filename << endl;
			return 1;
		  }
		Imagine::NFmiEdge tmp(nodes[idx1].x(),nodes[idx1].y(),
							  nodes[idx2].x(),nodes[idx2].y(),true);
		if(edge==85)
		  cout << "Edge 85!" << endl;
		edges.Add(tmp);
	  }

	if(in.fail())
	  {
		cerr << "Error: Error reading " << filename << endl;
		return 1;
	  }
	in.close();
  }

  // Build a path from the edges

  Imagine::NFmiPath path = edges.Path();

  // Convert individual closed segments into polygons
  // As soon as a polygon becomes closed, we output
  // it as an ESRI shape
  
  {
	Imagine::NFmiEsriShape shape;
	Polygon poly;

	const Imagine::NFmiPathData::const_iterator begin = path.Elements().begin();
	const Imagine::NFmiPathData::const_iterator end = path.Elements().end();

	for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; )
	  {
		bool doflush = false;
		// Huom! Jostain syystä g++ kääntää väärin (iter++==end), pakko tehdä näin
		if((*iter).Oper()==Imagine::kFmiMoveTo)
		  doflush = true;
		else if(++iter==end)
		  {
			--iter;
			poly.add(Point((*iter).X(),(*iter).Y()));
			doflush = true;
		  }
		else
		  --iter;
		
		if(doflush && !poly.empty())
		  {
			if(arealimit<=0 || poly.geoarea() >= arealimit)
			  {
				// Convert to NFmiEsriPolygon
				Imagine::NFmiEsriPolygon * p = new Imagine::NFmiEsriPolygon();
				Polygon::DataType::const_iterator b = poly.data().begin();
				Polygon::DataType::const_iterator e = poly.data().end();
				for(Polygon::DataType::const_iterator it = b; it!=e; ++it)
				  p->Add(Imagine::NFmiEsriPoint(it->x(),it->y()));
				// And add to output
				shape.Add(p);
			  }
			poly.clear();
		  }
		
		poly.add(Point((*iter).X(),(*iter).Y()));
		++iter;
	  }	
	
	if(!shape.Write(shapename))
	  {
		cerr << "Error while saving the shapefiles" << endl;
		return 1;
	  }
  }

  return 0;

}

// ======================================================================
