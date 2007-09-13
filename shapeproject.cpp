// ======================================================================
/*!
 * \file
 * \brief Implementation of shapeproject for reprojecting shape data
 */
// ======================================================================
/*!
 * \page shapeproject shapeproject
 *
 * shapeproject ohjelma lukee annetun SHP-tiedoston ja muuntaa
 * sen toiseen projektioon. Sekä input- että output-projektio
 * annetaan komentorivioptioina. Kummankin default-arvo on latlon.
 *
 * Komentorivisyntaksi:
 * \code
 * shapeproject [options] <inputshape> <outputshape>
 * \endcode
 *
 * Mahdolliset optiot ovat
 *
 *   - -h              Tulostaa käyttöohjeet
 *   - -i <inputproj>  Input-datan projektio
 *   - -o <outputproj> Output-datan projektio
 *
 * Projektio määritellään newbasen NFmiAreaFactory työkalun speksien
 * mukaisesti. Koordinaatteja käsitellään maailmankoordinaatiston
 * yksiköissä (eli metreissä). Poikkeus on "latlon" projektio,
 * jota käsitellään asteina.
 *
 * Esimerkiksi Tiehallinnon tiestöt voi muuntaa YKJ-koordinaateista
 * latlon-koordinaateiksi seuraavasti:
 * \code
 * shapeproject -i ykj tiet_ykj tiet_latlon
 * \endcode
 */
// ======================================================================

#include "newbase/NFmiArea.h"
#include "newbase/NFmiAreaFactory.h"
#include "newbase/NFmiCmdLine.h"
#include "imagine/NFmiEsriPoint.h"
#include "imagine/NFmiEsriProjector.h"
#include "imagine/NFmiEsriShape.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;
using namespace Imagine;

// ----------------------------------------------------------------------
/*!
 * \brief Options holder
 */
// ----------------------------------------------------------------------

struct Options
{
  string inputprojection;
  string outputprojection;
  string inputfile;
  string outputfile;

  Options()
	: inputprojection("latlon")
	, outputprojection("latlon")
	, inputfile()
	, outputfile()
  { }

};

// ----------------------------------------------------------------------
/*!
 * \brief Global instance of the parsed command line options
 */
// ----------------------------------------------------------------------

Options options;

// ----------------------------------------------------------------------
/*!
 * \brief Print usage
 */
// ----------------------------------------------------------------------

void usage()
{
  cout << "Usage: shapeproject [options] inputshape outputshape" << endl
	   << endl
	   << "shapeproject projects SHP-data to a different coordinate system." << endl
	   << endl
	   << "The available options are:" << endl
	   << endl
	   << "\t-h\t\tprint this help information" << endl
	   << "\t-i [proj]\tthe input projection (default: latlon)" << endl
	   << "\t-o [proj]\tthe output projection (default: latlon)" << endl
	   << endl;
}


// ----------------------------------------------------------------------
/*!
 * \brief Parse the command line
 *
 * \return False, if execution is to be stopped
 */
// ----------------------------------------------------------------------

bool parse_command_line(int argc, const char * argv[])
{
  NFmiCmdLine cmdline(argc,argv,"hi!o!");

  if(cmdline.Status().IsError())
	throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  // help-option must be checked first

  if(cmdline.isOption('h'))
	{
	  usage();
	  return false;
	}

  // then the required parameters

  if(cmdline.NumberofParameters() != 2)
	throw runtime_error("Incorrect number of command line parameters");

  options.inputfile  = cmdline.Parameter(1);
  options.outputfile = cmdline.Parameter(2);

  // options

  if(cmdline.isOption('i'))
	options.inputprojection = cmdline.OptionValue('i');

  if(cmdline.isOption('o'))
	options.outputprojection = cmdline.OptionValue('o');

  // validity checks

  if(options.inputprojection == options.outputprojection)
	throw("Input and output projections are equal, nothing to do");

  return true;

}

// ----------------------------------------------------------------------
/*!
 * \brief Class for reprojecting ESRI data
 */
// ----------------------------------------------------------------------

class MyProjector : public NFmiEsriProjector
{
private:

  const NFmiArea & itsInputArea;
  const NFmiArea & itsOutputArea;

public:

  MyProjector(const NFmiArea & theInputArea,
			  const NFmiArea & theOutputArea)
	: itsInputArea(theInputArea)
	, itsOutputArea(theOutputArea)
  { }

  virtual NFmiEsriPoint operator()(const NFmiEsriPoint & thePoint) const
  {
	const NFmiPoint p(thePoint.X(),thePoint.Y());
	if(options.inputprojection == "latlon")
	  {
		const NFmiPoint q = itsOutputArea.LatLonToWorldXY(p);
		return NFmiEsriPoint(q.X(),q.Y());
	  }
	else if(options.outputprojection == "latlon")
	  {
		const NFmiPoint latlon = itsInputArea.WorldXYToLatLon(p);
		return NFmiEsriPoint(latlon.X(),latlon.Y());
	  }
	else
	  {
		const NFmiPoint latlon = itsInputArea.WorldXYToLatLon(p);
		const NFmiPoint q = itsOutputArea.LatLonToWorldXY(latlon);
		return NFmiEsriPoint(q.X(),q.Y());
	  }
  }

  virtual void SetBox(const NFmiEsriBox & theBox) const
  {
	// We do nothing, no idea how this should even work
  }

};

// ----------------------------------------------------------------------
/*!
 * \brief The main program without error trapping
 */
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  // Parse the command line options
  if(!parse_command_line(argc,argv))
	return 0;

  // Create the projections

  auto_ptr<NFmiArea> inarea = NFmiAreaFactory::Create(options.inputprojection);
  auto_ptr<NFmiArea> outarea = NFmiAreaFactory::Create(options.outputprojection);
  // Read the shape data

  NFmiEsriShape shape;
  if(!shape.Read(options.inputfile))
	throw runtime_error("Failed to read shape '"+options.inputfile+"'");

  // Project it
  MyProjector projector(*inarea,*outarea);
  shape.Project(projector);

  // And save the results
  shape.Write(options.outputfile);

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
	  return domain(argc,argv);
	}
  catch(exception & e)
	{
	  cout << "Error: " << e.what() << endl;
	  return 1;
	}
  catch(...)
	{
	  cout << "Error: Caught an unknown exception" << endl;
	  return 1;
	}
}
