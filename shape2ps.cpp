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
 * area 10 NFmiLatLonArea ...
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
 * timemode utc
 * time +1 12
 * newpath
 * bezier approximate 10
 * contourcommands moveto lineto curveto closepath
 * contourline 10 stroke
 * contourline 15 stroke
 * contourline 20 stroke
 * contourline 25 stroke
 * contourline 30 stroke
 * contourfill -10 10 fill
 * \endcode
 *
 * The list of special commands is
 * - area ... to set the projection
 * - body to indicate the start of the PostScript body
 * - shape <moveto> <lineto> <closepath> <shapefile> to render a shapefile
 * - gshhs <moveto> <lineto> <closepath> <gshhsfile> to render a shoreline
 * - {moveto} {lineto} exec <shapefile> to execute given commands for vertices
 * - project <x> <y> to output projected x and y
 * - location <place> to output projected x and y
 * - system .... to execute the remaining line in the shell
 * - projectioncenter <lon> <lat> <scale>
 * - querydata <name> Set the active querydata
 * - parameter <name> Set the active querydata parameter
 * - timemode <local|utc>
 * - time <days> <hour>
 * - contourcommands <moveto> <lineto> <curveto> <closepath>
 * - contourline <value>
 * - contourfill <lolimit> <hilimit>
 * - bezier none
 * - bezier cardinal <0-1>
 * - bezier approximate <maxerror>
 */
// ======================================================================


// internal
#include "Polyline.h"
// imagine
#include "NFmiCardinalBezierFit.h"
#include "NFmiApproximateBezierFit.h"
#include "NFmiContourTree.h"
#include "NFmiGeoShape.h"
#include "NFmiGshhsTools.h"
// newbase
#include "NFmiArea.h"
#include "NFmiCmdLine.h"
#include "NFmiEnumConverter.h"
#include "NFmiFileSystem.h"
#include "NFmiLocationFinder.h"
#include "NFmiParameterName.h"
#include "NFmiPath.h"
#include "NFmiPreProcessor.h"
#include "NFmiSettings.h"
#include "NFmiStreamQueryData.h"
#include "NFmiValueString.h"
// system
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <memory>

#ifdef OLDGCC
  #include <strstream>
#else
  #include <sstream>
#endif

// For reading a projection specification
extern void * CreateSaveBase(unsigned long classID);

using namespace std;

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
 * \brief Update bounding box based on given point
 *
 * \param thePoint The point to add to the bounding box
 * \param theMinLon The variable in which to store the minimum longitude
 * \param theMinLat The variable in which to store the minimum latitude
 * \param theMaxLon The variable in which to store the maximum longitude
 * \param theMaxLat The variable in which to store the maximum latitude
 */
// ----------------------------------------------------------------------

void UpdateBBox(const NFmiPoint & thePoint,
				double & theMinLon, double & theMinLat,
				double & theMaxLon, double & theMaxLat)
{
  theMinLon = std::min(theMinLon,thePoint.X());
  theMinLat = std::min(theMinLat,thePoint.Y());
  theMaxLon = std::max(theMaxLon,thePoint.X());
  theMaxLat = std::max(theMaxLat,thePoint.Y());
}

// ----------------------------------------------------------------------
/*!
 * \brief Find geographic bounding box for given area
 *
 * The bounding box is found by traversing the edges of the area
 * and converting the coordinates to geographic ones for extrema
 * calculations.
 *
 * \param theArea The area
 * \param theMinLon The variable in which to store the minimum longitude
 * \param theMinLat The variable in which to store the minimum latitude
 * \param theMaxLon The variable in which to store the maximum longitude
 * \param theMaxLat The variable in which to store the maximum latitude
 */
// ----------------------------------------------------------------------

void FindBBox(const NFmiArea & theArea,
			  double & theMinLon, double & theMinLat,
			  double & theMaxLon, double & theMaxLat)
{
  // Good initial values are obtained from the corners

  theMinLon = theArea.TopLeftLatLon().X();
  theMinLat = theArea.TopLeftLatLon().Y();
  theMaxLon = theMinLon;
  theMaxLat = theMinLat;

  const unsigned int divisions = 100;

  // Go through the top edge

  for(unsigned int i=0; i<=divisions; i++)
	{
	  NFmiPoint xy(theArea.Left()+theArea.Width()*i/divisions,
				   theArea.Top());
	  NFmiPoint latlon(theArea.ToLatLon(xy));
	  UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	}

  // Go through the bottom edge

  for(unsigned int i=0; i<=divisions; i++)
	{
	  NFmiPoint xy(theArea.Left()+theArea.Width()*i/divisions,
				   theArea.Bottom());
	  NFmiPoint latlon(theArea.ToLatLon(xy));
	  UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	}
  
  // Go through the left edge

  for(unsigned int i=0; i<=divisions; i++)
	{
	  NFmiPoint xy(theArea.Left(),
				   theArea.Bottom()+theArea.Height()*i/divisions);
	  NFmiPoint latlon(theArea.ToLatLon(xy));
	  UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	}

  // Go through the top edge

  for(unsigned int i=0; i<=divisions; i++)
	{
	  NFmiPoint xy(theArea.Left(),
				   theArea.Bottom()+theArea.Height()*i/divisions);
	  NFmiPoint latlon(theArea.ToLatLon(xy));
	  UpdateBBox(latlon,theMinLon,theMinLat,theMaxLon,theMaxLat);
	}
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
					const string & theMoveto,
					const string & theLineto,
					const string & theCurveto,
					const string & theClosepath)
{
  const Imagine::NFmiPathData::const_iterator begin = thePath.Elements().begin();
  const Imagine::NFmiPathData::const_iterator end = thePath.Elements().end();
  
  string out;

  unsigned int cubic_count = 0;

  for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; ++iter)
	{
	  const string x = NFmiValueString(iter->X()).CharPtr();
	  const string y = NFmiValueString(iter->Y()).CharPtr();

	  out += (x + ' ' + y + ' ');

	  switch(iter->Oper())
		{
		case Imagine::kFmiMoveTo:
		  out += (theMoveto + '\n');
		  cubic_count = 0;
		  break;
		case Imagine::kFmiLineTo:
		case Imagine::kFmiGhostLineTo:
		  out += (theLineto + '\n');
		  cubic_count = 0;
		  break;
		case Imagine::kFmiCubicTo:
		  ++cubic_count;
		  if(cubic_count % 3 == 0)
			out += (theCurveto + '\n');
		  break;
		case Imagine::kFmiConicTo:
		  throw runtime_error("Conic segments not supported");
		}
	}

  return out;
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

  const bool strip_pound = true;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include","","");
  if(!processor.ReadAndStripFile(scriptfile))
	throw runtime_error("Error: Could not open '"+scriptfile+"' for reading");

  string text = processor.GetString();
#ifdef OLDGCC
  istrstream script(text.c_str());
#else
  istringstream script(text);
#endif

  // The area specification is not given yet
  auto_ptr<NFmiArea> theArea;

  // The querydata is not given yet
  string theQueryDataName;
  NFmiStreamQueryData theQueryData;

  // The querydata parameter is not given yet
  string theParameterName;
  FmiParameterName theParameter = kFmiBadParameter;

  // The time mode is by default local time
  bool theLocalTimeMode = true;

  // The time offset has not been given yet
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

  // No clipping margin given yet
  double theClipMargin = 0.0;

  // Not in the body yet
  bool body = false;

  // Prepare the location finder for "location" token

  string coordfile = NFmiSettings::instance().value("qdpoint::coordinates_file","default.txt");
  string coordpath = NFmiSettings::instance().value("qdpoint::coordinates_path",".");

  NFmiLocationFinder locfinder;
  locfinder.AddFile(FileComplete(coordfile,coordpath), false);

  // Do the deed
  string token;
  string buffer;
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
		  buffer += '%' + token + '\n';
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
			  cerr << "Calculated new projection corners" << endl
				   << "  BottomLeft = " << bottomleft
				   << "  TopRight = " << topright;
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

		  cout << "%!PS-Adobe-3.0 EPSF-3.0" << endl
			   << "%%Creator: shape2ps" << endl
			   << "%%Pages: 1" << endl
			   << "%%BoundingBox: "
			   << theArea->Left() << " "
			   << theArea->Top() << " "
			   << theArea->Right() << " "
			   << theArea->Bottom() << endl
			   << "%%EndComments" << endl
			   << "%%BeginProcSet: shape2ps" << endl
			   << "save /mysave exch def" << endl
			   << "/mydict 1000 dict def" << endl
			   << "mydict begin" << endl
			   << "/e2{2 index exec}def" << endl
			   << "/e3{3 index exec}def" << endl
			   << buffer << endl
			   << "end" << endl
			   << "%%EndProcSet" << endl
			   << "%%EndProlog" << endl
			   << "%%Page: 1 1" << endl
			   << "%%BeginPageSetup" << endl
			   << "mydict begin" << endl
			   << "%%EndPageSetup" << endl;

		  buffer = "";

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
		  buffer += static_cast<char*>(NFmiValueString(pt.X()));
		  buffer += ' ';
		  buffer += static_cast<char*>(NFmiValueString(theArea->Bottom()-(pt.Y()-theArea->Top())));
		  buffer += ' ';
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
		  buffer += static_cast<char*>(NFmiValueString(pt.X()));
		  buffer += ' ';
		  buffer += static_cast<char*>(NFmiValueString(theArea->Bottom()-(pt.Y()-theArea->Top())));
		  buffer += ' ';
		}

	  // ------------------------------------------------------------
	  // Handle a system command
	  // ------------------------------------------------------------

	  else if(token == "system")
		{
		  if(!body)
			throw runtime_error("system command does not work in the header");

		  getline(script,token);
		  cout << "% " << token << endl;
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

		  buffer += "% ";
		  buffer += token;
		  buffer += ' ';
		  buffer += shapefile;
		  buffer += '\n';
			

		  // Read the shape, project and get as path
		  try
			{
			  Imagine::NFmiGeoShape geo(shapefile,Imagine::kFmiGeoShapeEsri);
			  geo.ProjectXY(*theArea);
			  Imagine::NFmiPath path = geo.Path();

			  if(token=="shape")
				buffer += pathtostring(path,
									   *theArea,
									   theClipMargin,
									   moveto,lineto,closepath);
			  else
				buffer += pathtostring(path,
									   *theArea,
									   theClipMargin,
									   "e3","e2");
			  if(token == "exec")
				buffer += "pop pop\n";
			  
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

		  buffer += "% ";
		  buffer += token;
		  buffer += ' ';
		  buffer += gshhsfile;
		  buffer += '\n';
			
		  // Read the gshhs, project and get as path
		  try
			{
			  double minlon, minlat, maxlon, maxlat;
			  FindBBox(*theArea,minlon,minlat,maxlon,maxlat);

			  Imagine::NFmiPath path = Imagine::NFmiGshhsTools::ReadPath(gshhsfile,
																		 minlon,minlat,
																		 maxlon,maxlat);
			  
			  path.Project(theArea.get());

			  buffer += pathtostring(path,*theArea,theClipMargin,moveto,lineto,closepath);
			  
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
		  script >> theQueryDataName;
		  if(!theQueryData.ReadLatestData(theQueryDataName))
			throw runtime_error("Failed to read querydata from "+theQueryDataName);
		}

	  // ------------------------------------------------------------
	  // Handle the parameter <name> command
	  // ----------------------------------------------------------------------

	  else if(token == "parameter")
		{
		  script >> theParameterName;
		  NFmiEnumConverter converter;
		  theParameter = FmiParameterName(converter.ToEnum(theParameterName));
		  if(theParameter == kFmiBadParameter)
			throw runtime_error("Parameter name "+theParameterName+" is not recognized by newbase");
		}

	  // ------------------------------------------------------------
	  // Handle the timemode <local|utc> command
	  // ------------------------------------------------------------

	  else if(token == "timemode")
		{
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
		  script >> theDay >> theHour;
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
		  else
			throw runtime_error("Bezier mode "+theBezierMode+" is not recognized");
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

		  if(theDay < 0 || theHour<0)
			throw runtime_error("time must be specified before using any contouring commands");

		  // Try to set the proper time on

		  NFmiTime t;
		  t.ChangeByDays(theDay);
		  t.SetMin(0);
		  t.SetSec(0);
		  t.SetHour(theHour);
		  if(theLocalTimeMode)
			t = toutctime(t);

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

		  NFmiDataMatrix<float> values;
		  NFmiDataMatrix<NFmiPoint> coords;

		  q->Values(values,t);
		  q->LocationsXY(coords,*theArea);

		  Imagine::NFmiContourTree tree(lolimit,hilimit);
		  if(token == "contourline")
			tree.LinesOnly(true);

		  tree.Contour(coords,values,Imagine::NFmiContourTree::kFmiContourLinear);

		  Imagine::NFmiPath path = tree.Path();

		  if(theBezierMode == "cardinal")
			path = Imagine::NFmiCardinalBezierFit::Fit(path,theBezierSmoothness);
		  else if(theBezierMode == "approximate")
			path = Imagine::NFmiApproximateBezierFit::Fit(path,theBezierMaxError);

		  buffer += pathtostring(path,
								 theMovetoCommand,
								 theLinetoCommand,
								 theCurvetoCommand,
								 theClosepathCommand);
		  
		}

	  // ------------------------------------------------------------
	  // Handle a regular PostScript token line
	  // ------------------------------------------------------------

	  else
		{
		  buffer += token;
		  getline(script,token);
		  buffer += token + '\n';
		}

	  // If in body, there is no need to cache anymore
	  if(body && !buffer.empty())
		{
		  cout << buffer;
		  buffer = "";
		}

	}

  // The script finished

  if(!body)
	{
	  cerr << "Error: There was no body in the script" << endl;
	  return 1;
	}

  cout << "end" << endl
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
  catch(const runtime_error & e)
	{
	  cerr << "Error: shape2ps failed due to" << endl
		   << "--> " << e.what() << endl;
	  return 1;
	}
}

// ======================================================================
