// ======================================================================
/*!
 * \file
 * \brief Implementation of the compositealpha program
 */
// ======================================================================
/*!
 * \page compositealpha compositealpha
 *
 */
// ======================================================================

#include "newbase/NFmiStringTools.h"
#include "imagine/NFmiColorTools.h"
#include "imagine/NFmiImage.h"

#include <iomanip>
#include <iostream>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * The main program
 */
// ----------------------------------------------------------------------

int main(int argc, const char *argv[])
{
  
  // Read the arguments
  if(argc != 4)
    {
      cerr << "Error: Expecting three filenames as input" << endl;
      return 1;
    }
  
  string infile = argv[1];
  string maskfile = argv[2];
  string outfile = argv[3];

  const string suffix = NFmiStringTools::Suffix(outfile);

  if(suffix.empty())
	{
	  cerr << "Error: No suffix in output filename" << endl;
	  return 1;
	}


  Imagine::NFmiImage inputimage(infile);
  Imagine::NFmiImage maskimage(maskfile);

  if(inputimage.Width() != maskimage.Width() ||
	 inputimage.Height() != maskimage.Height())
	{
	  cerr << "Error: Image sizes differ" << endl;
	  return 1;
	}

  const unsigned int nx = inputimage.Width();
  const unsigned int ny = inputimage.Height();

  Imagine::NFmiImage outputimage(nx,ny);

  cout.setf(std::ios::hex,std::ios::basefield);
  for(unsigned int j=0; j<ny; j++)
	for(unsigned int i=0; i<nx; i++)
	  {

		Imagine::NFmiColorTools::Color c = inputimage(i,j);
		unsigned int intensity = Imagine::NFmiColorTools::Intensity(maskimage(i,j));
		unsigned int alpha = intensity*Imagine::NFmiColorTools::MaxAlpha/Imagine::NFmiColorTools::MaxRGB;

		c = Imagine::NFmiColorTools::ReplaceAlpha(inputimage(i,j),alpha);
		c = Imagine::NFmiColorTools::Simplify(c,-1,false);
		outputimage(i,j) = c;
	  }

  outputimage.SaveAlpha(true);
  outputimage.WantPalette(true);

  outputimage.Write(outfile,suffix);

  return 0;
}

// ======================================================================
