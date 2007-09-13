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
#include "newbase/NFmiValueString.h"
#include "newbase/NFmiFileSystem.h"
#include "newbase/NFmiArea.h"
#include "newbase/NFmiValueString.h"
#include "imagine/NFmiPath.h"
#include "imagine/NFmiGeoShape.h"
// system
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>

using namespace std;


// ----------------------------------------------------------------------
// The main program
// ----------------------------------------------------------------------
int main(int argc, const char * argv[])
{
  // Read the command line arguments
  if(argc!=4)
	{
	  cerr << "Usage: " << argv[0] << " [arealimit] [shape] [outname]" << endl;
	  return 1;
	}
  double arealimit = atof(argv[1]);
  string shapefile = argv[2];
  string outname = argv[3];

  // Read the shapefile
  cout << "Reading shapefile " << shapefile << endl;
  Imagine::NFmiGeoShape geo(shapefile,Imagine::kFmiGeoShapeEsri);

  // Create a vector of polygons
  cout << "Collecting polygons large enough" << endl;
  vector<Polygon> polygons;
  {
	Imagine::NFmiPath path = geo.Path();
	Polygon poly;
	const Imagine::NFmiPathData::const_iterator begin = path.Elements().begin();
	const Imagine::NFmiPathData::const_iterator end = path.Elements().end();
	
	for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; )
	  {
		bool doflush = false;
		// Huom! Jostain syyst‰ g++ k‰‰nt‰‰ v‰‰rin (iter++==end), pakko tehd‰ n‰in
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
			  polygons.push_back(poly);
			poly.clear();
		  }

		poly.add(Point((*iter).X(),(*iter).Y()));
		++iter;
	  }
  }
  cout << "Found " << polygons.size() << " large enough polygons" << endl;
  
  // Create a table of unique nodes from the accepted polygons

  cout << "Calculating unique nodes" << endl;
  Nodes nodes;
  {
	const vector<Polygon>::const_iterator begin = polygons.begin();
	const vector<Polygon>::const_iterator end = polygons.end();
	unsigned long idx=0;
	for(vector<Polygon>::const_iterator iter=begin; iter!=end; ++iter)
	  {
		++idx;
		const Polygon::DataType::const_iterator pbegin = iter->data().begin();
		const Polygon::DataType::const_iterator pend = iter->data().end();
		for(Polygon::DataType::const_iterator piter=pbegin; piter!=pend; ++piter)
		  static_cast<void>(nodes.add(*piter,idx));
	  }
  }
  cout << "Counted " << nodes.data().size() << " nodes" << endl;

  // Output a file containing all the nodes
  {
	string nodefile = outname + ".node";
	cout << "Writing " << nodefile << endl;
	ofstream out(nodefile.c_str());
	if(!out)
	  {
		cerr << "Error: Could not open " << nodefile << " for writing" << endl;
		return 1;
	  }

	// PSLG syntax has numbers for each point, but triangle seems to assume
	// the lines have been sorted.

	map<long,Point> sortednodes;
	const Nodes::DataType & data = nodes.data();
	{
	  const Nodes::DataType::const_iterator begin = data.begin();
	  const Nodes::DataType::const_iterator end = data.end();
	  for(Nodes::DataType::const_iterator iter=begin; iter!=end; ++iter)
		sortednodes.insert(make_pair(iter->second.first,iter->first));
	}
	// #points dimension #attributes #boundarymarkers
	out << data.size() << " 2 1 0" << endl;
	{
	  const map<long,Point>::const_iterator begin = sortednodes.begin();
	  const map<long,Point>::const_iterator end = sortednodes.end();
	  for(map<long,Point>::const_iterator iter=begin; iter!=end; ++iter)
		out << iter->first << '\t'
			<< NFmiValueString(iter->second.x()).CharPtr() << '\t'
			<< NFmiValueString(iter->second.y()).CharPtr() << '\t'
			<< nodes.id(iter->second) << endl;
	}
	out.close();
  }

  // Output a file containing all the edges
  {
	string polyfile = outname + ".poly";
	cout << "Writing " << polyfile << endl;
	ofstream out(polyfile.c_str());
	if(!out)
	  {
		cerr << "Error: Could not open " << polyfile << " for writing" << endl;
		return 1;
	  }

	long number_of_edges=0;
	for(unsigned int pass=1; pass<=2; pass++)
	  {
		long edge = 0;
		if(pass==2)
		  {
			out << "0 2 0 0" << endl; // no nodes in .poly
			out << number_of_edges << " 0" << endl;
		  }

		const vector<Polygon>::const_iterator begin = polygons.begin();
		const vector<Polygon>::const_iterator end = polygons.end();
		for(vector<Polygon>::const_iterator iter=begin; iter!=end; ++iter)
		  {
			Point previous_point(0,0);
			const Polygon::DataType::const_iterator pbegin = iter->data().begin();
			const Polygon::DataType::const_iterator pend = iter->data().end();
			for(Polygon::DataType::const_iterator piter=pbegin; piter!=pend; ++piter)
			  {
				if(piter!=pbegin)
				  {
					if(pass==1)
					  number_of_edges++;
					else
					  {
						out << ++edge << '\t'
							<< nodes.number(previous_point) << '\t'
							<< nodes.number(*piter) << endl;
					  }
				  }
				if(pass==2)
				  previous_point = *piter;
			  }
		  }
	  }

	// No holes
	out << "0" << endl;

	// For each polygon we output a single point inside the polygon.
	// The attribute is the ordinal of the polygon itself. This
	// is sufficient to uniquely identify all triangles in the
	// subsequent triangulation as belonging to some original
	// polygon, provided no polygon encloses another one.

	{
	  cout << "Finding an inside point for " << polygons.size() << " polygons" << endl;
	  out << polygons.size() << endl;
	  const vector<Polygon>::const_iterator begin = polygons.begin();
	  const vector<Polygon>::const_iterator end = polygons.end();
	  long poly = 0;
	  for(vector<Polygon>::const_iterator iter=begin; iter!=end; ++iter)
		{
		  ++poly;
		  Point pt = iter->someInsidePoint();
		  out << poly << '\t'
			  << NFmiValueString(pt.x()).CharPtr() << '\t'
			  << NFmiValueString(pt.y()).CharPtr() << '\t'
			  << poly << endl;
		}
	}

	out.close();
  }

  cout << "Done" << endl;

}
