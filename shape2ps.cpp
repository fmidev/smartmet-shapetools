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
 * The list of special commands is
 * - area ... to set the projection
 * - body to indicate the start of the PostScript body
 * - shape <moveto> <lineto> <shapefile> to render a shapefile
 * - {moveto} {lineto} exec <shapefile> to execute given commands for a shape
 * - project <x> <y> to output projected x and y
 * - location <place> to output projected x and y
 * - system .... to execute the remaining line in the shell
 * - projectionscenter <lon> <lat> <scale>
 */
// ======================================================================


// internal
#include "Polyline.h"
// imagine
#include "NFmiGeoShape.h"
#include "NFmiGshhsTools.h"
// newbase
#include "NFmiArea.h"
#include "NFmiCmdLine.h"
#include "NFmiFileSystem.h"
#include "NFmiLocationFinder.h"
#include "NFmiPath.h"
#include "NFmiPreProcessor.h"
#include "NFmiSettings.h"
#include "NFmiValueString.h"
// system
#include <cstdlib>
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
// The main program
// ----------------------------------------------------------------------
int main(int argc, const char * argv[])
{
  bool verbose = false;

  NFmiCmdLine cmdline(argc,argv,"v");

  if(cmdline.NumberofParameters() != 1)
	{
	  cerr << "Usage: " << argv[0] << " [options] [filename]" << endl;
	  return 1;
	}

  if(cmdline.Status().IsError())
	{
	  cerr << "Error: Invalid command line" << endl
		   << "\t --> " << cmdline.Status().ErrorLog().CharPtr() << endl;
	  return 1;
	}

  if(cmdline.isOption('v'))
    verbose = true;

  string scriptfile = cmdline.Parameter(1);

  // Open the script file

  const bool strip_pound = true;
  NFmiPreProcessor processor(strip_pound);
  processor.SetIncluding("include","","");
  if(!processor.ReadAndStripFile(scriptfile))
	{
	  cerr << "Error: Could not open '"
		   << scriptfile
		   << "' for reading"
		   << endl;
	  return 1;
	}
  string text = processor.GetString();
#ifdef OLDGCC
  istrstream script(text.c_str());
#else
  istringstream script(text);
#endif

  // The area specification is not given yet
  auto_ptr<NFmiArea> theArea;

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
			{
			  cerr << "Error: Area already given" << endl;
			  return 1;
			}
		  unsigned long classID;
		  string className;
		  script >> classID >> className;
		  theArea.reset(static_cast<NFmiArea *>(CreateSaveBase(classID)));
		  if(!theArea.get())
			{
			  cerr << "Error: Unrecognized area in the script:" << endl
				   << classID << " " << className << endl;
			  return 1;
			}
		  script >> *theArea;

		  // Now handle XY limits

		  double x1 = theArea->Left();
		  double x2 = theArea->Right();
		  double y1 = theArea->Top();
		  double y2 = theArea->Bottom();

		  if(x2-x1==1 && y2-y1==1)
			{
			  cerr << "Error: No decent XY-area given in projection" << endl;
			  return 1;
			}
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
			{
			  cerr << "projectioncenter must be used after a projection has been specified" << endl;
			  return 1;
			}

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
			{
			  cerr << "Error: body command given twice in script" << endl;
			  return 1;
			}
		  if(!theArea.get())
			{
			  cerr << "Error: No area specified before body" << endl;
			  return 1;
			}

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
			{
			  cerr << "Error: Using project before area" << endl;
			  return 1;
			}
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
			{
			  cerr << "Error: Using location before area" << endl;
			  return 1;
			}
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
			{
			  cerr << "Error: system does not work in the header" << endl;
			  return 1;
			}
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
			{
			  cerr << "Error: Cannot have " << token << " command in header" << endl;
			  return 1;
			}
		  
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
			  
			  const Imagine::NFmiPathData::const_iterator begin = path.Elements().begin();
			  const Imagine::NFmiPathData::const_iterator end = path.Elements().end();
			  
			  Polyline polyline;
			  for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; )
				{
				  double X = (*iter).X();
				  double Y = theArea->Bottom()-((*iter).Y()-theArea->Top());
				  
				  if((*iter).Oper()==Imagine::kFmiMoveTo || (*iter).Oper()==Imagine::kFmiLineTo)
					polyline.add(X,Y);
				  else
					{
					  cerr << "Error: Only moveto and lineto are supported in shapes" << endl;
					  return 1;
					}
				  
				  // Advance to next point. If end or moveto, flush previous polyline out
				  ++iter;
				  if(!polyline.empty() && (iter==end || (*iter).Oper()==Imagine::kFmiMoveTo))
					{
					  polyline.clip(theArea->Left(), theArea->Top(),
									theArea->Right(), theArea->Bottom(),
									theClipMargin);
					  if(!polyline.empty())
						if(token=="shape")
						  buffer += polyline.path(moveto,lineto,closepath);
						else
						  buffer += polyline.path("e3","e2");
					  polyline.clear();
					}
				}
			  
			  if(token == "exec")
				buffer += "pop pop\n";
			  
			}
		  catch(std::exception & e)
			{
			  cerr << "Error: shape2ps failed" << endl;
			  if(token == "shape")
				cerr << "at command shape "
					 << moveto << ' '
					 << lineto << ' '
					 << closepath << ' '
					 << shapefile << endl;
			  cerr << " --> " << e.what() << endl;
			  return 1;
			}
		  
		}

	  // ------------------------------------------------------------
	  // Handle the gshhs command
	  // ------------------------------------------------------------

	  else if(token == "gshhs")
		{
		  if(!body)
			{
			  cerr << "Error: Cannot have " << token << " command in header" << endl;
			  return 1;
			}
		  
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
			  
			  const Imagine::NFmiPathData::const_iterator begin = path.Elements().begin();
			  const Imagine::NFmiPathData::const_iterator end = path.Elements().end();
			  
			  Polyline polyline;
			  for(Imagine::NFmiPathData::const_iterator iter=begin; iter!=end; )
				{
				  double X = (*iter).X();
				  double Y = theArea->Bottom()-((*iter).Y()-theArea->Top());
				  
				  if((*iter).Oper()==Imagine::kFmiMoveTo || (*iter).Oper()==Imagine::kFmiLineTo)
					polyline.add(X,Y);
				  else
					{
					  cerr << "Error: Only moveto and lineto are supported in shapes" << endl;
					  return 1;
					}
				  
				  // Advance to next point. If end or moveto, flush previous polyline out
				  ++iter;
				  if(!polyline.empty() && (iter==end || (*iter).Oper()==Imagine::kFmiMoveTo))
					{
					  polyline.clip(theArea->Left(), theArea->Top(),
									theArea->Right(), theArea->Bottom(),
									theClipMargin);
					  if(!polyline.empty())
						buffer += polyline.path(moveto,lineto,closepath);
					  polyline.clear();
					}
				}
			  
			}
		  catch(std::exception & e)
			{
			  cerr << "Error: shape2ps failed at command gshhs "
				   << moveto << ' '
				   << lineto << ' '
				   << closepath << ' '
				   << gshhsfile << endl;
			  cerr << " --> " << e.what() << endl;
			  return 1;
			}
		  
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
}

// ======================================================================
