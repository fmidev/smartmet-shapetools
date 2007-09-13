// ======================================================================
/*!
 * \file shape2ps.cpp
 * \brief A program to render shapefiles into PostScript
 */
// ======================================================================
/*!
 * \page shape2ps shape2ps
 *
 * shape2ps takes as input a single filename containing rendering
 * directives. All rendering directive lines are by default considered
 * to be plain PostScript, and are output as is. A few special tokens
 * will read and output shapefile data, set projections and so on.
 * Note that the special tokens must be the first ones on the line,
 * otherwise they won't be recognized.
 *
 * A very simple example file would be
 *
 * \code
 * projection stereographic,20,90,60:6,51.3,49,70.2:400,400
 * body
 * 3 setlinewidth
 * 1 1 0 setrgbcolor
 * shape moveto lineto closepath suomi stroke
 * gshhs moveto lineto closepath gshhs_f.c stroke
 * \endcode
 *
 * Atleast one of coordinate ranges given in the projection is expected to
 * differ from 0-1. If only one differs, the other is calculated to be 0-X,
 * where X is calculated so that aspect ratio is preserved in the set
 * projection.
 *
 * To render querydata contours one can use code like
 * \code
 * area 10 NFmiLatLonArea ...
 * body
 * querydata /data/pal/querydata/pal/skandinavia/pinta
 * parameter Temperature
 * level 750
 * timemode utc
 * time +1 12
 * newpath
 * bezier approximate 10
 * contourcommands moveto lineto curveto closepath
 * labelcommand show
 * contourline 10 stroke
 * contourline 15 stroke
 * contourline 20 stroke
 * contourline 25 stroke
 * contourline 30 stroke
 * contourfill -10 10 fill
 * contourlabels 10 30 5
 * \endcode
 *
 * The list of special commands is
 * - projection ... to set the projection
 * - area ... to set the projection as an object
 * - projectioncenter <lon> <lat> <scale>
 * - body to indicate the start of the PostScript body
 * - shape <moveto> <lineto> <closepath> <shapefile> to render a shapefile
 * - gshhs <moveto> <lineto> <closepath> <gshhsfile> to render a shoreline
 * - {moveto} {lineto} exec <shapefile> to execute given commands for vertices
 * - {} qdexec <querydata> to execute given commands for grid points
 * - project <x> <y> to output projected x and y
 * - location <place> to output projected x and y
 * - system .... to execute the remaining line in the shell
 * - querydata <name> Set the active querydata
 * - parameter <name> Set the active querydata parameter
 * - level <levelvalue> Set the active querydata level
 * - timemode <local|utc>
 * - time <days> <hour>
 * - smoother <name> <factor> <radius>
 * - contourcommands <moveto> <lineto> <curveto> <closepath>
 * - contourline <value>
 * - contourfill <lolimit> <hilimit>
 * - bezier none
 * - bezier cardinal <0-1>
 * - bezier approximate <maxerror>
 * - bezier tight <maxerror>
 */
// ======================================================================


// internal
#include "Polyline.h"
#include "imagine/NFmiCardinalBezierFit.h"
#include "imagine/NFmiApproximateBezierFit.h"
#include "imagine/NFmiTightBezierFit.h"
#include "imagine/NFmiContourTree.h"
#include "imagine/NFmiGeoShape.h"
#include "imagine/NFmiGshhsTools.h"
#include "imagine/NFmiPath.h"
#include "newbase/NFmiArea.h"
#include "newbase/NFmiAreaFactory.h"
#include "newbase/NFmiAreaTools.h"
#include "newbase/NFmiCmdLine.h"
#include "newbase/NFmiEnumConverter.h"
#include "newbase/NFmiFileSystem.h"
#include "newbase/NFmiLocationFinder.h"
#include "newbase/NFmiParameterName.h"
#include "newbase/NFmiPreProcessor.h"
#include "newbase/NFmiSettings.h"
#include "newbase/NFmiSaveBaseFactory.h"
#include "newbase/NFmiSmoother.h"
#include "newbase/NFmiStreamQueryData.h"
#include "newbase/NFmiValueString.h"
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <memory>
#include <sstream>

#include <boost/lexical_cast.hpp>

using namespace boost;
using namespace std;

// Clamp PostScript path elements to within this range
const double clamp_limit = 10000;

struct BezierSettings
{
  BezierSettings(const string & theName, const string theMode, double theSmoothness, double theMaxError)
	: name(theName), mode(theMode), smoothness(theSmoothness), maxerror(theMaxError)
  { }

  bool operator==(const BezierSettings & theOther) const
  {
	return (mode == theOther.mode &&
			smoothness == theOther.smoothness &&
			maxerror == theOther.maxerror);
  }

  bool operator<(const BezierSettings & theOther) const
  {
	if(mode != theOther.mode) return (mode < theOther.mode);
	if(smoothness != theOther.smoothness) return (smoothness < theOther.smoothness);
	return (maxerror < theOther.maxerror);
  }

  string name;
  string mode;
  double smoothness;
  double maxerror;
};

// ----------------------------------------------------------------------
/*!
 * \brief Global replace within a string
 *
 * \param theString The string in which to replace
 * \param theMatch The original string
 * \param theReplacement The replacement string
 */
// ----------------------------------------------------------------------

void replace(string & theString,
			 const string & theMatch,
			 const string & theReplacement)
{
  string::size_type pos;

  while( (pos = theString.find(theMatch)) != string::npos)
	{
	  theString.replace(pos,theMatch.size(),theReplacement);
	}
}


// ----------------------------------------------------------------------
/*!
 * \brief Create unique name for a contour
 *
 * \param theIndex The unique ordinal of the contour
 * \return The unique name
 */
// ----------------------------------------------------------------------

string ContourName(unsigned long theIndex)
{
  return ("% shape2ps: path " + lexical_cast<string>(theIndex) + " place holder");
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert local time to UTC time using current TZ
 *
 * \param theLocalTime The local time
 * \return The UTC time
 *
 * \note This was stolen from textgen library TimeTools namespace
 */
// ----------------------------------------------------------------------

NFmiTime toutctime(const NFmiTime & theLocalTime)
{
  ::tm tlocal;
  tlocal.tm_sec   = theLocalTime.GetSec();
  tlocal.tm_min   = theLocalTime.GetMin();
  tlocal.tm_hour  = theLocalTime.GetHour();
  tlocal.tm_mday  = theLocalTime.GetDay();
  tlocal.tm_mon   = theLocalTime.GetMonth()-1;
  tlocal.tm_year  = theLocalTime.GetYear()-1900;
  tlocal.tm_wday  = -1;
  tlocal.tm_yday  = -1;
  tlocal.tm_isdst = -1;
  
  ::time_t tsec = mktime(&tlocal);
  
  ::tm * tutc = ::gmtime(&tsec);
  
  NFmiTime out(tutc->tm_year + 1900,
			   tutc->tm_mon + 1,
			   tutc->tm_mday,
			   tutc->tm_hour,
			   tutc->tm_min,
			   tutc->tm_sec);
  
  return out;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert path to PostScript path
 */
// ----------------------------------------------------------------------

string pathtostring(const Imagine::NFmiPath & thePath,
					const NFmiArea & theArea,
					double theClipMargin,
					const string & theMoveto,
					const string & theLineto,
					const string & theClosepath = "")
{
  const Imagine::NFmiPathData::const_iterator begin = thePath.Elements().begin();
  const Imagine::NFmiPathData::const_iterator end = thePath.Elements().end();
  
  string out;
  
  Polyline polyline;
  for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; )
	{
	  double X = (*iter).X();
	  double Y = theArea.Bottom()-((*iter).Y()-theArea.Top());
	  
	  X = std::min(X,clamp_limit);
	  Y = std::min(Y,clamp_limit);
	  X = std::max(X,-clamp_limit);
	  Y = std::max(Y,-clamp_limit);

	  if((*iter).Oper()==Imagine::kFmiMoveTo ||
		 (*iter).Oper()==Imagine::kFmiLineTo ||
		 (*iter).Oper()==Imagine::kFmiGhostLineTo )
		polyline.add(X,Y);
	  else
		throw runtime_error("Only moveto and lineto commands are supported in paths");
	  
	  // Advance to next point. If end or moveto, flush previous polyline out
	  ++iter;
	  if(!polyline.empty() && (iter==end || (*iter).Oper()==Imagine::kFmiMoveTo))
		{
		  polyline.clip(theArea.Left(), theArea.Top(),
						theArea.Right(), theArea.Bottom(),
						theClipMargin);
		  if(!polyline.empty())
			
			out += polyline.path(theMoveto,theLineto,theClosepath);
		  polyline.clear();
		}
	}
  
  return out;
}

// ----------------------------------------------------------------------
/*!
 * \brief Convert path to PostScript path
 */
// ----------------------------------------------------------------------

string pathtostring(const Imagine::NFmiPath & thePath,
					const NFmiArea & theArea,
					double theClipMargin,
					const string & theMoveto,
					const string & theLineto,
					const string & theCurveto,
					const string & theClosepath)
{

  Imagine::NFmiPath path = thePath.Clip(theArea.Left(), theArea.Top(),
										theArea.Right(), theArea.Bottom(),
										theClipMargin);

  const Imagine::NFmiPathData::const_iterator begin = path.Elements().begin();
  const Imagine::NFmiPathData::const_iterator end = path.Elements().end();
  
  ostringstream out;

  unsigned int cubic_count = 0;

  for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; ++iter)
	{
	  double X = iter->X();
	  double Y = theArea.Bottom()-(iter->Y()-theArea.Top());

	  X = std::min(X,clamp_limit);
	  Y = std::min(Y,clamp_limit);
	  X = std::max(X,-clamp_limit);
	  Y = std::max(Y,-clamp_limit);

	  out << X
		  << ' '
		  << Y
		  << ' ';

	  switch(iter->Oper())
		{
		case Imagine::kFmiMoveTo:
		  out << theMoveto << endl;
		  cubic_count = 0;
		  break;
		case Imagine::kFmiLineTo:
		case Imagine::kFmiGhostLineTo:
		  out << theLineto << endl;
		  cubic_count = 0;
		  break;
		case Imagine::kFmiCubicTo:
		  ++cubic_count;
		  if(cubic_count % 3 == 0)
			out << theCurveto << endl;
		  break;
		case Imagine::kFmiConicTo:
		  throw runtime_error("Conic segments not supported");
		}
	}

  return out.str();
}

// ----------------------------------------------------------------------
/*!
 * \brief Set queryinfo level
 *
 * A negative level value implies the first level in the data
 *
 * \param theInfo The queryinfo
 * \param theLevel The level value
 */
// ----------------------------------------------------------------------

void set_level(NFmiFastQueryInfo & theInfo, int theLevel)
{
  if(theLevel < 0)
	theInfo.FirstLevel();
  else
	{
	  for(theInfo.ResetLevel(); theInfo.NextLevel(); )
		if(theInfo.Level()->LevelValue() == static_cast<unsigned int>(theLevel))
		  return;
	  throw runtime_error("Level value "+NFmiStringTools::Convert(theLevel)+" is not available");
	}
}

// ----------------------------------------------------------------------
// The main driver
// ----------------------------------------------------------------------

int domain(int argc, const char * argv[])
{
  bool verbose = false;

  NFmiCmdLine cmdline(argc,argv,"v");

  if(cmdline.NumberofParameters() != 1)
	throw runtime_error("Usage: shape2ps [options] <filename>");

  if(cmdline.Status().IsError())
	throw runtime_error(cmdline.Status().ErrorLog().CharPtr());

  if(cmdline.isOption('v'))
    verbose = true;

  string scriptfile = cmdline.Parameter(1);

  // Open the script file

  const bool strip_pound = false;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include","","");
  processor.SetDefine("#define");
  if(!processor.ReadAndStripFile(scriptfile))
	throw runtime_error("Error: Could not open '"+scriptfile+"' for reading");

  string text = processor.GetString();
  istringstream script(text);

  // The area specification is not given yet
  auto_ptr<NFmiArea> theArea;

  // The querydata is not given yet
  string theQueryDataName;
  NFmiStreamQueryData theQueryData;

  // The querydata parameter is not given yet
  string theParameterName;
  FmiParameterName theParameter = kFmiBadParameter;

  // The level is not given yet - use first level
  int theLevel = -1;

  // The time mode is by default local time
  bool theLocalTimeMode = true;

  // The time offset has not been given yet
  string theTimeOrigin = "now";
  int theDay = -1;
  int theHour = -1;

  // Contouring movement command names

  string theMovetoCommand = "moveto";
  string theLinetoCommand = "lineto";
  string theCurvetoCommand = "curveto";
  string theClosepathCommand = "closepath";
  
  // Bezier smoothing
  string theBezierMode = "none";
  double theBezierSmoothness = 0.5;
  double theBezierMaxError = 1.0;

  // Smoothing

  string theSmoother = "None";
  double theSmootherRadius = 10;
  int theSmootherFactor = 5;

  // The calculated contours before Bezier fitting, and set of all
  // different combinatins of Bezier parameters used in the script

  typedef list<pair<BezierSettings,Imagine::NFmiPath> > Contours;
  Contours theContours;
  set<BezierSettings> theContourSettings;

  // No clipping margin given yet
  double theClipMargin = 0.0;

  // Not in the body yet
  bool body = false;

  // Prepare the location finder for "location" token

  string coordfile = NFmiSettings::Optional<string>("qdpoint::coordinates_file","default.txt");
  string coordpath = NFmiSettings::Optional<string>("qdpoint::coordinates_path",".");

  NFmiLocationFinder locfinder;
  locfinder.AddFile(NFmiFileSystem::FileComplete(coordfile,coordpath), false);

  // We try to cache the matrices for best speed.
  // Some tokens will invalidate the matrices


  auto_ptr<NFmiDataMatrix<float> > values;
  auto_ptr<NFmiDataMatrix<NFmiPoint> > coords;

  // Do the deed
  string token;
  ostringstream buffer;

  while(script >> token)
	{

	  // ------------------------------------------------------------
	  // Handle script comments
	  // ------------------------------------------------------------
	  if(token == "#")
		script.ignore(1000000,'\n');

	  // ------------------------------------------------------------
	  // Handle PostScript comments
	  // ------------------------------------------------------------
	  else if(token == "%")
		{
		  getline(script,token);
		  buffer << '%' << token << endl;
		}

	  // ------------------------------------------------------------
	  // Handle the clipmargin command
	  // ------------------------------------------------------------
	  else if(token == "clipmargin")
		script >> theClipMargin;

	  // ------------------------------------------------------------
	  // Handle the area command
	  // ------------------------------------------------------------
	  else if(token == "area")
		{
		  cerr << "Warning: The area command is deprecated, use projection command instead" << endl;

		  // Invalidate coordinate matrix
		  coords.reset(0);

		  if(theArea.get())
			throw runtime_error("Area given twice");

		  unsigned long classID;
		  string className;
		  script >> classID >> className;
		  theArea.reset(static_cast<NFmiArea *>(CreateSaveBase(classID)));
		  if(!theArea.get())
			throw runtime_error("Unrecognized area in the script");

		  script >> *theArea;

		  // Now handle XY limits

		  double x1 = theArea->Left();
		  double x2 = theArea->Right();
		  double y1 = theArea->Top();
		  double y2 = theArea->Bottom();

		  if(x2-x1==1 && y2-y1==1)
			throw runtime_error("Error: No decent XY-area given in projection");
		  // Recalculate x-range from y-range if necessary
		  if(x2-x1==1)
			{
			  x1 = 0;
			  x2 = (y2-y1)*theArea->WorldXYAspectRatio();
			  theArea->SetXYArea(NFmiRect(x1,y2,x2,y1));
			}
		  // Recalculate y-range from x-range if necessary
		  if(y2-y1==1)
			{
			  y1 = 0;
			  y2 = (x2-x1)/theArea->WorldXYAspectRatio();
			  theArea->SetXYArea(NFmiRect(x1,y2,x2,y1));
			}
		}

	  // ------------------------------------------------------------
	  // Handle the projectioncenter command
	  // ------------------------------------------------------------

	  else if(token == "projectioncenter")
		{
		  cerr << "Warning: The projectioncenter command is deprecated, use projection command instead" << endl;

		  coords.reset(0);

		  if(!theArea.get())
			throw runtime_error("projectioncenter must be used after a projection has been specified");

		  float lon, lat, scale;
		  script >> lon >> lat >> scale;

		  NFmiPoint center(lon,lat);
		  double x1 = theArea->Left();
		  double x2 = theArea->Right();
		  double y1 = theArea->Top();
		  double y2 = theArea->Bottom();

		  // Projektiolaskuja varten
		  theArea.reset(theArea->NewArea(center,center));

		  NFmiPoint c = theArea->LatLonToWorldXY(center);

		  NFmiPoint bl(c.X()-scale*1000*(x2-x1), c.Y()-scale*1000*(y2-y1));
		  NFmiPoint tr(c.X()+scale*1000*(x2-x1), c.Y()+scale*1000*(y2-y1));
		  NFmiPoint bottomleft = theArea->WorldXYToLatLon(bl);
		  NFmiPoint topright = theArea->WorldXYToLatLon(tr);
		  theArea.reset(theArea->NewArea(bottomleft, topright));

		  if(verbose)
			{
			  cerr << "Calculated new area to be" << endl
				   << *theArea
				   << endl;
			}

		}

	  // ------------------------------------------------------------
	  // Handle the projection command
	  // ------------------------------------------------------------

	  else if(token == "projection")
		{
		  // Invalidate coordinate matrix
		  coords.reset(0);

		  if(theArea.get())
			throw runtime_error("Projection given twice");

		  string specs;
		  script >> specs;
		  theArea.reset(NFmiAreaFactory::Create(specs).release());

		  // Now handle XY limits

		  double x1 = theArea->Left();
		  double x2 = theArea->Right();
		  double y1 = theArea->Top();
		  double y2 = theArea->Bottom();

		  if(x2-x1==1 && y2-y1==1)
			throw runtime_error("Error: No decent XY-area given in projection");
		  // Recalculate x-range from y-range if necessary
		  if(x2-x1==1)
			{
			  x1 = 0;
			  x2 = (y2-y1)*theArea->WorldXYAspectRatio();
			  theArea->SetXYArea(NFmiRect(x1,y2,x2,y1));
			}
		  // Recalculate y-range from x-range if necessary
		  if(y2-y1==1)
			{
			  y1 = 0;
			  y2 = (x2-x1)/theArea->WorldXYAspectRatio();
			  theArea->SetXYArea(NFmiRect(x1,y2,x2,y1));
			}

		  if(verbose)
			{
			  cerr << "The new projection is" << endl
				   << *theArea
				   << endl;
			}
		}
	  
	  // ------------------------------------------------------------
	  // Handle the body command
	  // ------------------------------------------------------------
	  else if(token == "body")
		{
		  if(body)
			throw runtime_error("body command given twice in script");

		  if(!theArea.get())
			throw runtime_error("No area specified before body");
		  
		  body = true;

		  // Output the header, then the buffer, then the beginning of body

		  string tmp = buffer.str();
		  buffer.str("");

		  buffer << "%!PS-Adobe-3.0 EPSF-3.0" << endl
				 << "%%Creator: shape2ps" << endl
				 << "%%Pages: 1" << endl
				 << "%%BoundingBox: "
				 << static_cast<int>(theArea->Left()) << " "
				 << static_cast<int>(theArea->Top()) << " "
				 << static_cast<int>(theArea->Right()) << " "
				 << static_cast<int>(theArea->Bottom()) << endl
				 << "%%EndComments" << endl
				 << "%%BeginProcSet: shape2ps" << endl
				 << "save /mysave exch def" << endl
				 << "/mydict 1000 dict def" << endl
				 << "mydict begin" << endl
				 << "/e2{2 index exec}def" << endl
				 << "/e3{3 index exec}def" << endl
				 << tmp << endl
				 << "end" << endl
				 << "%%EndProcSet" << endl
				 << "%%EndProlog" << endl
				 << "%%Page: 1 1" << endl
				 << "%%BeginPageSetup" << endl
				 << "mydict begin" << endl
				 << "%%EndPageSetup" << endl;

		}

	  // ------------------------------------------------------------
	  // Handle a project command
	  // ------------------------------------------------------------

	  else if(token == "project")
		{
		  if(!theArea.get())
			throw runtime_error("Using project before area");

		  double x,y;
		  script >> x >> y;
		  NFmiPoint pt = theArea->ToXY(NFmiPoint(x,y));
		  buffer << static_cast<char*>(NFmiValueString(pt.X()))
				 << ' '
				 << static_cast<char*>(NFmiValueString(theArea->Bottom()-(pt.Y()-theArea->Top())))
				 << ' ';
		}

	  // ------------------------------------------------------------
	  // Handle a location command
	  // ------------------------------------------------------------

	  else if(token == "location")
		{
		  if(!theArea.get())
			throw runtime_error("Using location before area");

		  string placename;
		  script >> placename;

		  NFmiPoint lonlat = locfinder.Find(placename);
		  if(locfinder.LastSearchFailed())
			throw runtime_error("Location "+placename+" is not in the database");

		  NFmiPoint pt = theArea->ToXY(lonlat);
		  buffer << static_cast<char*>(NFmiValueString(pt.X()))
				 << ' '
				 << static_cast<char*>(NFmiValueString(theArea->Bottom()-(pt.Y()-theArea->Top())))
				 << ' ';
		}

	  // ------------------------------------------------------------
	  // Handle a system command
	  // ------------------------------------------------------------

	  else if(token == "system")
		{
		  if(!body)
			throw runtime_error("system command does not work in the header");

		  getline(script,token);
		  buffer << "% " << token << endl;
		  system(token.c_str());
		}

	  // ------------------------------------------------------------
	  // Handle the shape and exec commands
	  // ------------------------------------------------------------
	  else if(token == "shape" || token=="exec")
		{
		  if(!body)
			throw runtime_error("Cannot have "+token+" command in header");
		  
		  string moveto, lineto, closepath;
		  if(token == "shape")
			script >> moveto >> lineto >> closepath;

		  string shapefile;
		  script >> shapefile;

		  buffer << "% ";
		  buffer << token;
		  buffer << ' ';
		  buffer << shapefile;
		  buffer << endl;
			

		  // Read the shape, project and get as path
		  try
			{
			  Imagine::NFmiGeoShape geo(shapefile,Imagine::kFmiGeoShapeEsri);
			  geo.ProjectXY(*theArea);
			  Imagine::NFmiPath path = geo.Path();

			  if(token=="shape")
				buffer << pathtostring(path,
									   *theArea,
									   theClipMargin,
									   moveto,lineto,closepath);
			  else
				buffer << pathtostring(path,
									   *theArea,
									   theClipMargin,
									   "e3","e2");
			  if(token == "exec")
				buffer << "pop pop" << endl;
			  
			}
		  catch(std::exception & e)
			{
			  if(token!="shape")
				throw e;
			  string msg = "Failed at command shape ";
			  msg += moveto + ' ';
			  msg += lineto + ' ';
			  msg += closepath + ' ';
			  msg += shapefile;
			  msg += " : ";
			  msg += e.what();
			  throw runtime_error(msg);
			}
		  
		}

	  // ------------------------------------------------------------
	  // Handle the qdexec command
	  // ------------------------------------------------------------

	  else if(token == "qdexec")
		{
		  if(!body)
			throw runtime_error("Cannot have "+token+" command in header");

		  if(!theArea.get())
			throw runtime_error("Using qdexec before projection specified");

		  string queryfile;
		  script >> queryfile;
		  buffer << "% " << token << ' ' << queryfile << endl;

		  NFmiStreamQueryData qd;
		  qd.SafeReadLatestData(queryfile);
		  NFmiFastQueryInfo * qi = qd.QueryInfoIter();
		  qi->First();

		  for(qi->ResetLocation(); qi->NextLocation(); )
			{
			  const NFmiPoint lonlat = qi->LatLon();
			  const NFmiPoint pt = theArea->ToXY(lonlat);
			  buffer << static_cast<char*>(NFmiValueString(pt.X()))
					 << ' '
					 << static_cast<char*>(NFmiValueString(theArea->Bottom()-(pt.Y()-theArea->Top())))
					 << " e2" << endl;
			}
		  buffer << "pop" << endl;
		}

	  // ------------------------------------------------------------
	  // Handle the gshhs command
	  // ------------------------------------------------------------

	  else if(token == "gshhs")
		{
		  if(!body)
			throw runtime_error("Cannot have "+token+" command in header");
		  
		  string moveto, lineto, closepath;
		  script >> moveto >> lineto >> closepath;

		  string gshhsfile;
		  script >> gshhsfile;

		  buffer << "% "
				 << token
				 << ' '
				 << gshhsfile
				 << endl;
			
		  // Read the gshhs, project and get as path
		  try
			{
			  double minlon, minlat, maxlon, maxlat;
			  NFmiAreaTools::LatLonBoundingBox(*theArea,minlon,minlat,maxlon,maxlat);

			  Imagine::NFmiPath path = Imagine::NFmiGshhsTools::ReadPath(gshhsfile,
																		 minlon,minlat,
																		 maxlon,maxlat);
			  
			  path.Project(theArea.get());

			  buffer << pathtostring(path,*theArea,theClipMargin,moveto,lineto,closepath);
			  
			}
		  catch(std::exception & e)
			{
			  string msg = "Failed at command gshhs ";
			  msg += moveto + ' ';
			  msg += lineto + ' ';
			  msg += closepath + ' ';
			  msg += gshhsfile;
			  msg += " due to ";
			  msg += e.what();
			  throw runtime_error(e.what());
			}
		  
		}

	  // ------------------------------------------------------------
	  // Handle the querydata <filename> command
	  // ------------------------------------------------------------

	  else if(token == "querydata")
		{
		  coords.reset(0);
		  values.reset(0);
		  script >> theQueryDataName;
		  if(!theQueryData.SafeReadLatestData(theQueryDataName))
			throw runtime_error("Failed to read querydata from "+theQueryDataName);
		}

	  // ------------------------------------------------------------
	  // Handle the parameter <name> command
	  // ----------------------------------------------------------------------

	  else if(token == "parameter")
		{
		  values.reset(0);
		  script >> theParameterName;
		  NFmiEnumConverter converter;
		  theParameter = FmiParameterName(converter.ToEnum(theParameterName));
		  if(theParameter == kFmiBadParameter)
			throw runtime_error("Parameter name "+theParameterName+" is not recognized by newbase");
		}

	  // ------------------------------------------------------------
	  // Handle the level <levelvalue> command
	  // ----------------------------------------------------------------------

	  else if(token == "level")
		{
		  values.reset(0);
		  script >> theLevel;
		}

	  // ------------------------------------------------------------
	  // Handle the timemode <local|utc> command
	  // ------------------------------------------------------------

	  else if(token == "timemode")
		{
		  values.reset(0);
		  string name;
		  script >> name;
		  if(name == "local")
			theLocalTimeMode = true;
		  else if(name == "utc")
			theLocalTimeMode = false;
		  else
			throw runtime_error("Unrecognized time mode "+name+", the name must be 'local' or 'utc'");
		}
  
	  // ------------------------------------------------------------
	  // Handle the time <day> <hour> command
	  // ------------------------------------------------------------

	  else if(token == "time")
		{
		  values.reset(0);
		  script >> theTimeOrigin >> theDay >> theHour;
		  if(theTimeOrigin != "now" &&
			 theTimeOrigin != "origintime" &&
			 theTimeOrigin != "firsttime")
			throw runtime_error("Time mode "+theTimeOrigin+" is not recognized");
		  if(theDay < 0)
			throw runtime_error("First argument of time-command must be nonnegative");
		  if(theHour<0 || theHour>24)
			throw runtime_error("Second argument of time-command must be in range 0-23");
		}

	  // ------------------------------------------------------------
	  // Handle the bezier command
	  // ------------------------------------------------------------

	  else if(token == "bezier")
		{
		  script >> theBezierMode;
		  if(theBezierMode == "none")
			;
		  else if(theBezierMode == "cardinal")
			script >> theBezierSmoothness;
		  else if(theBezierMode == "approximate")
			script >> theBezierMaxError;
		  else if(theBezierMode == "tight")
			script >> theBezierMaxError;
		  else
			throw runtime_error("Bezier mode "+theBezierMode+" is not recognized");
		}

	  // ------------------------------------------------------------
	  // Handle the smoother command
	  // ------------------------------------------------------------

	  else if(token == "smoother")
		{
		  values.reset(0);
		  script >> theSmoother;
		  if(theSmoother != "None")
			script >> theSmootherFactor >> theSmootherRadius;
		  if(NFmiSmoother::SmootherValue(theSmoother) == NFmiSmoother::kFmiSmootherMissing)
			throw runtime_error("Smoother mode "+theSmoother+" is not recognized");
		}
	  
	  // ------------------------------------------------------------
	  // Handle the contourcommands <moveto> <lineto>  <curveto> <closepath> command
	  // ------------------------------------------------------------

	  else if(token == "contourcommands")
		{
		  script >> theMovetoCommand
				 >> theLinetoCommand
				 >> theCurvetoCommand
				 >> theClosepathCommand;
		}

	  // ----------------------------------------------------------------------
	  // Handle the windarrows <dx> <dy> command
	  // ----------------------------------------------------------------------

	  else if(token == "windarrows")
		{
		  if(!body)
			throw runtime_error(token+" command is not allowed in the header");

		  int dx,dy;
		  script >> dx >> dy;

		  NFmiFastQueryInfo * q = theQueryData.QueryInfoIter();
		  if(q == 0)
			throw runtime_error("querydata must be specified before using any windarrows commands");
		  if(!q->Param(kFmiWindDirection))
			throw runtime_error("parameter WindDirection is not available in "+theQueryDataName);
		  
		  if(theDay < 0 || theHour<0)
			throw runtime_error("time must be specified before using any contouring commands");

		  // Try to set the proper level on
		  set_level(*q,theLevel);

		  // Try to set the proper time on

		  NFmiTime t;
		  t.SetMin(0);
		  t.SetSec(0);
		  if(theTimeOrigin == "now")
			{
			  t.ChangeByDays(theDay);
			  t.SetHour(theHour);
			}
		  if(theTimeOrigin == "origintime")
			{
			  t = q->OriginTime();
			  t.ChangeByDays(theDay);
			  t.ChangeByHours(theHour);
			}
		  else if(theTimeOrigin == "firsttime")
			{
			  q->FirstTime();
			  t = q->ValidTime();
			  t.ChangeByDays(theDay);
			  t.ChangeByHours(theHour);
			}

		  // Get the data to be contoured
		  
		  if(coords.get() == 0)
			{
			  coords.reset(new NFmiDataMatrix<NFmiPoint>);
			  q->LocationsXY(*coords,*theArea);
			}

		  values.reset();
		  if(values.get() == 0)
			{
			  values.reset(new NFmiDataMatrix<float>);
			  q->Values(*values,t);
			}

		  // Loop through the data and render arrows

		  for(unsigned int j=0; j<values->NY(); j += dy)
			for(unsigned int i=0; i<values->NX(); i += dx)
			  {
				float wdir = (*values)[i][j];
				NFmiPoint xy = (*coords)[i][j];

				double x = xy.X();
				double y = theArea->Bottom()-(xy.Y()-theArea->Top());

				if(x > theArea->Left() && x < theArea->Right() ||
				   y > theArea->Top() && y < theArea->Bottom())
				  {
					buffer << wdir << ' ' << x << ' ' << y << ' ' << " windarrow" << endl;
				  }

			  }
		}


	  // ------------------------------------------------------------
	  // Handle the contourline <value> command
	  // Handle the contourfill <lolimit> <hilimit> command
	  // ------------------------------------------------------------

	  else if(token == "contourline" || token == "contourfill")
		{
		  if(!body)
			throw runtime_error(token+" command is not allowed in the header");
		  
		  NFmiFastQueryInfo * q = theQueryData.QueryInfoIter();
		  if(q == 0)
			throw runtime_error("querydata must be specified before using any contouring commands");
		  
		  if(theParameter == kFmiBadParameter)
			throw runtime_error("parameter must be specified before using any contouring commands");
		  
		  if(!q->Param(theParameter))
			throw runtime_error("parameter "+theParameterName+" is not available in "+theQueryDataName);

		  // Try to set the proper level on
		  set_level(*q,theLevel);

		  if(theDay < 0 || theHour<0)
			throw runtime_error("time must be specified before using any contouring commands");

		  // Try to set the proper time on

		  NFmiTime t;
		  t.SetMin(0);
		  t.SetSec(0);
		  if(theTimeOrigin == "now")
			{
			  t.ChangeByDays(theDay);
			  t.SetHour(theHour);
			}
		  if(theTimeOrigin == "origintime")
			{
			  t = q->OriginTime();
			  t.ChangeByDays(theDay);
			  t.ChangeByHours(theHour);
			}
		  else if(theTimeOrigin == "firsttime")
			{
			  q->FirstTime();
			  t = q->ValidTime();
			  t.ChangeByDays(theDay);
			  t.ChangeByHours(theHour);
			}

		  if(theLocalTimeMode)
			t = toutctime(t);

		  cerr << "Time = " << t << endl;

		  float lolimit, hilimit;
		  if(token == "contourline")
			{
			  script >> lolimit;
			  hilimit = kFloatMissing;
			}
		  else
			{
			  script >> lolimit >> hilimit;
			  if(lolimit!=kFloatMissing &&
				 hilimit!=kFloatMissing &&
				 lolimit >= hilimit)
				throw runtime_error("contourfill first argument must be smaller than second argument");
			}

		  // Get the data to be contoured

		  if(coords.get() == 0)
			{
			  coords.reset(new NFmiDataMatrix<NFmiPoint>);
			  q->LocationsXY(*coords,*theArea);
			}

		  if(values.get() == 0)
			{
			  values.reset(new NFmiDataMatrix<float>);
			  q->Values(*values,t);

			  if(theSmoother != "None")
				{
				  NFmiSmoother smoother(theSmoother,theSmootherFactor,theSmootherRadius);
				  *values = smoother.Smoothen(*coords,*values);
				}
			}

		  Imagine::NFmiContourTree tree(lolimit,hilimit);
		  if(token == "contourline")
			tree.LinesOnly(true);

		  tree.Contour(*coords,*values,Imagine::NFmiContourTree::kFmiContourLinear);

		  Imagine::NFmiPath path = tree.Path();

		  // We don't bother to store non-smoothed contours at all
		  if(theBezierMode == "none")
			{
			  buffer << pathtostring(path,
									 *theArea,
									 theClipMargin,
									 theMovetoCommand,
									 theLinetoCommand,
									 theCurvetoCommand,
									 theClosepathCommand);
			}
		  else
			{
			  const string name = ContourName(theContours.size()+1);
			  BezierSettings bset(name,theBezierMode, theBezierSmoothness, theBezierMaxError);
			  theContourSettings.insert(bset);
			  theContours.push_back(make_pair(bset,path));
			  buffer << name << endl;
			}

		}

	  // ------------------------------------------------------------
	  // Handle a regular PostScript token line
	  // ------------------------------------------------------------

	  else
		{
		  buffer << token;
		  getline(script,token);
		  buffer << token << endl;
		}

	}

  // The script finished

  if(!body)
	{
	  cerr << "Error: There was no body in the script" << endl;
	  return 1;
	}

  // Fill in the contours

  string output = buffer.str();

  if(!theContours.empty())
	{
	  for(set<BezierSettings>::const_iterator sit = theContourSettings.begin();
		  sit != theContourSettings.end();
		  ++sit)
		{
		  list<string> names;
		  Imagine::NFmiBezierTools::NFmiPaths paths;
		  for(Contours::const_iterator it = theContours.begin();
			  it != theContours.end();
			  ++it)
			{
			  if(*sit == it->first)
				{
				  paths.push_back(it->second);
				  names.push_back(it->first.name);
				}
			}

		  Imagine::NFmiBezierTools::NFmiPaths outpaths;
		  if(sit->mode == "cardinal")
			outpaths = Imagine::NFmiCardinalBezierFit::Fit(paths,sit->smoothness);
		  else if(sit->mode == "approximate")
			outpaths = Imagine::NFmiApproximateBezierFit::Fit(paths,sit->maxerror);
		  else if(sit->mode == "tight")
			outpaths = Imagine::NFmiTightBezierFit::Fit(paths,sit->maxerror);
		  else
			throw runtime_error("Unknown Bezier mode "+sit->mode+" while fitting contours");

		  list<string>::const_iterator nit = names.begin();
		  Imagine::NFmiBezierTools::NFmiPaths::const_iterator it = outpaths.begin();
		  for( ; nit!=names.end() && it!=outpaths.end(); ++nit, ++it)
			{
			  const string name = *nit;
			  const string path = pathtostring(*it,
											   *theArea,
											   theClipMargin,
											   theMovetoCommand,
											   theLinetoCommand,
											   theCurvetoCommand,
											   theClosepathCommand);
			  replace(output,name,path);
			  
			}


		}
	}

  cout << output
	   << "end" << endl
	   << "%%Trailer" << endl
	   << "mysave restore" << endl
	   << "%%EOF" << endl;

  return 0;
}

// ----------------------------------------------------------------------
// Main program.
// ----------------------------------------------------------------------

int main(int argc, const char* argv[])
{
  try
	{
	  return domain(argc, argv);
	}
  catch(runtime_error & e)
	{
	  cerr << "Error: shape2ps failed due to" << endl
		   << "--> " << e.what() << endl;
	  return 1;
	}
  catch(...)
	{
	  cerr << "Error: shape2ps failed due to an unknown exception" << endl;
	  return 1;
	}
}

// ======================================================================
