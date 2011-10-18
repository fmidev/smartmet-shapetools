// ======================================================================
/*!
 * \file
 * \brief A program to dump shapefile contents in qdtext SVG form
 */
// ======================================================================
/*!
 * \page shape2svg shape2svg
 *
 * shape2svg takes as input a single filename indicating a set
 * of ESRI shapefiles and dumps the map information to files
 * named based on the given attribute
 *
 * For example
 * \code
 * > shape2xml -f NAME -d /tmp suomi
 * \endcode
 */
// ======================================================================

#ifdef _MSC_VER
#pragma warning(disable : 4786) // STL name length warnings off
#endif

#include "newbase/NFmiCmdLine.h"
#include "imagine/NFmiEsriShape.h"
#include "imagine/NFmiEsriMultiPoint.h"
#include "imagine/NFmiEsriPolyLine.h"
#include "imagine/NFmiEsriPolygon.h"
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>

using namespace std;

struct Options
{
  string infile;
  string outdir;
  string fieldname;

  Options()
	: infile()
	, outdir(".")
	, fieldname("NAME")
  { }
};

Options options;

void usage()
{
  cout << "Usage: shape2svg [options] shapename" << endl
	   << endl
	   << "shape2svg dumps the shape into files in SVG path data form, which" << endl
	   << "used for example by the qdtext program." << endl
	   << endl
	   << "The available options are:" << endl
	   << endl
	   << "\t-f <name>\tThe name of the attribute from which the filename is generated" << endl
	   << "\t\t\t(default = NAME)" << endl
	   << "\t-d <dir>\tThe output directory (default = .)" << endl
	   << endl;
}


int run(int argc, const char * argv[])
{
  // Parse command line

  NFmiCmdLine cmdline(argc,argv,"hd!f!");

  if(cmdline.Status().IsError())
	throw runtime_error("Invalid command line");

  if(cmdline.isOption('h'))
	{
	  usage();
	  return 0;
	}

  if(cmdline.NumberofParameters() != 1)
	throw runtime_error("Shape file name not given as argument");

  options.infile = cmdline.Parameter(1);

  if(cmdline.isOption('f'))
	options.fieldname = cmdline.OptionValue('f');

  if(cmdline.isOption('d'))
	options.outdir = cmdline.OptionValue('d');


  Imagine::NFmiEsriShape shape;
  if(!shape.Read(options.infile,true))
	throw std::runtime_error("Failed to read "+options.infile);

  const Imagine::NFmiEsriShape::attributes_type & attributes = shape.Attributes();

  int shapenumber = -1;
  for(Imagine::NFmiEsriShape::const_iterator it = shape.Elements().begin();
	  it != shape.Elements().end();
	  ++it) 
	{
	  ++shapenumber;

	  if(*it == 0)
		continue;

	  string name;
	  for(Imagine::NFmiEsriShape::attributes_type::const_iterator ait = attributes.begin();
		  ait != attributes.end() && name.empty();
		  ++ait)
		{
		  if( (*ait)->Name() != options.fieldname)
			continue;

		  switch((*ait)->Type())
			{
			case Imagine::kFmiEsriString:
			  name = (*it)->GetString((*ait)->Name());
			  break;
			default:
			  throw runtime_error("Attribute "+options.fieldname+" must be of type string");
			}
		}

	  if(name.empty())
		throw runtime_error("The shape does not contain a field named "+options.fieldname);


	  string outfile = options.outdir + "/" + name + ".svg";

	  ofstream out(outfile.c_str());
	  if(!out)
		throw runtime_error("Failed to open '"+outfile+"' for writing");

	  out << '"';

	  switch( (*it)->Type())
		{
		case Imagine::kFmiEsriNull:
		case Imagine::kFmiEsriMultiPatch:
		  break;
		case Imagine::kFmiEsriPoint:
		case Imagine::kFmiEsriPointM:
		case Imagine::kFmiEsriPointZ:
		  {
			const float x = (*it)->X();
			const float y = (*it)->Y();
			out << "M " << x << ' ' << y << endl;
			break;
		  }
		case Imagine::kFmiEsriMultiPoint:
		case Imagine::kFmiEsriMultiPointM:
		case Imagine::kFmiEsriMultiPointZ:
		  {
			const Imagine::NFmiEsriMultiPoint * elem = static_cast<const Imagine::NFmiEsriMultiPoint *>(*it);
			for(int i=0; i<elem->NumPoints(); i++)
			  {
				const float x = elem->Points()[i].X();
				const float y = elem->Points()[i].Y();
				if(i>0)
				  out << ' ';
				out << "M " << x << ' ' << y;
			  }
			out << endl;
			break;
		  }
		case Imagine::kFmiEsriPolyLine:
		case Imagine::kFmiEsriPolyLineM:
		case Imagine::kFmiEsriPolyLineZ:
		  {
			const Imagine::NFmiEsriPolyLine * elem = static_cast<const Imagine::NFmiEsriPolyLine *>(*it);
			for(int part=0; part<elem->NumParts(); part++)
			  {
				int i1,i2;
				i1 = elem->Parts()[part];       // start of part
				if(part+1 == elem->NumParts())
				  i2 = elem->NumPoints()-1;     // end of part
				else
				  i2 = elem->Parts()[part+1]-1;     // end of part
				
				if(i2>=i1)
				  {
					if(part>0)
					  out << endl;
					out << "M "
						<< elem->Points()[i1].X()
						<< ' '
						<< elem->Points()[i1].Y();
					for(int i=i1+1; i<=i2; i++)
					  out << " L "
						  << elem->Points()[i].X()
						  << ' '
						  << elem->Points()[i].Y();
					out << endl;
				  }
			  }
			break;
		  }
		case Imagine::kFmiEsriPolygon:
		case Imagine::kFmiEsriPolygonM:
		case Imagine::kFmiEsriPolygonZ:
		  {
			const Imagine::NFmiEsriPolygon * elem = static_cast<const Imagine::NFmiEsriPolygon *>(*it);
			for(int part=0; part<elem->NumParts(); part++)
			  {
				int i1,i2;
				i1 = elem->Parts()[part];       // start of part
				if(part+1 == elem->NumParts())
				  i2 = elem->NumPoints()-1;     // end of part
				else
				  i2 = elem->Parts()[part+1]-1; // end of part
				
				if(i2>=i1)
				  {
					if(part>0)
					  out << endl;
					out << "M "
						<< elem->Points()[i1].X()
						<< ' '
						<< elem->Points()[i1].Y();
					for(int i=i1+1; i<=i2; i++)
					  out << " L "
						  << elem->Points()[i].X()
						  << ' '
						  << elem->Points()[i].Y();
					out << " Z" << endl;
				  }
			  }
			break;
		  }
		}

	  out << '"' << endl;
	  out.close();
	}

  return 0;
}

// ----------------------------------------------------------------------
/*!
 * \brief Main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char * argv[])
{
  try
	{
	  return run(argc,argv);
	}
  catch(std::exception & e)
	{
	  std::cerr << "Error: " << e.what() << std::endl;
	  return 1;
	}
  catch(...)
	{
	  std::cerr << "Error: Caught an unknown exception" << std::endl;
	  return 1;
	}
}
