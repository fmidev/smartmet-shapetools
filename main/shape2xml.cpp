// ======================================================================
/*!
 * \file
 * \brief A program to dump shapefile contents in XML form
 */
// ======================================================================
/*!
 * \page shape2xml shape2xml
 *
 * shape2xml takes as input a single filename indicating a set
 * of ESRI shapefiles and dumps the map information to the screen.
 *
 * For example
 * \code
 * > shape2xml suomi > suomi.xml
 * \endcode
 */
// ======================================================================

#ifdef _MSC_VER
#pragma warning(disable : 4786)  // STL name length warnings off
#endif

#include <imagine/NFmiEsriMultiPoint.h>
#include <imagine/NFmiEsriPolyLine.h>
#include <imagine/NFmiEsriPolygon.h>
#include <imagine/NFmiEsriShape.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;

// ----------------------------------------------------------------------
/*!
 * \brief Print the attributes of the element
 */
// ----------------------------------------------------------------------

void print_attributes(const Imagine::NFmiEsriElement& theElement,
                      const Imagine::NFmiEsriShape::attributes_type& attributes)
{
  for (auto* attribute : attributes)
  {
    cout << ' ' << attribute->Name() << "=\"";
    switch (attribute->Type())
    {
      case Imagine::kFmiEsriString:
        cout << theElement.GetString(attribute->Name());
        break;
      case Imagine::kFmiEsriInteger:
        cout << theElement.GetInteger(attribute->Name());
        break;
      case Imagine::kFmiEsriDouble:
        cout << theElement.GetDouble(attribute->Name());
        break;
      case Imagine::kFmiEsriDate:
        cout << theElement.GetDate(attribute->Name()).ToStr(kYYYYMMDD);
        break;
    }
    cout << '"';
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Print the parts of a polyline or polygon, each part terminated by theEnd
 */
// ----------------------------------------------------------------------

template <typename T>
void print_parts(const T& theElement, const char* theEnd)
{
  const auto* elem = &theElement;
  for (int part = 0; part < elem->NumParts(); part++)
  {
    int i1 = 0;
    int i2 = 0;
    i1 = elem->Parts()[part];  // start of part
    if (part + 1 == elem->NumParts())
      i2 = elem->NumPoints() - 1;  // end of part
    else
      i2 = elem->Parts()[part + 1] - 1;  // end of part

    if (i2 >= i1)
    {
      if (part > 0)
        cout << endl;
      cout << "M " << elem->Points()[i1].X() << ' ' << elem->Points()[i1].Y();
      for (int i = i1 + 1; i <= i2; i++)
        cout << " L " << elem->Points()[i].X() << ' ' << elem->Points()[i].Y();
      cout << theEnd << endl;
    }
  }
}

// ----------------------------------------------------------------------
/*!
 * \brief Print the path of the element
 */
// ----------------------------------------------------------------------

void print_element_path(const Imagine::NFmiEsriElement& theElement)
{
  switch (theElement.Type())
  {
    case Imagine::kFmiEsriNull:
    case Imagine::kFmiEsriMultiPatch:
      break;
    case Imagine::kFmiEsriPoint:
    case Imagine::kFmiEsriPointM:
    case Imagine::kFmiEsriPointZ:
    {
      const float x = theElement.X();
      const float y = theElement.Y();
      cout << "M " << x << ' ' << y << endl;
      break;
    }
    case Imagine::kFmiEsriMultiPoint:
    case Imagine::kFmiEsriMultiPointM:
    case Imagine::kFmiEsriMultiPointZ:
    {
      const auto* elem = static_cast<const Imagine::NFmiEsriMultiPoint*>(&theElement);
      for (int i = 0; i < elem->NumPoints(); i++)
      {
        const float x = elem->Points()[i].X();
        const float y = elem->Points()[i].Y();
        if (i > 0)
          cout << ' ';
        cout << "M " << x << ' ' << y;
      }
      cout << endl;
      break;
    }
    case Imagine::kFmiEsriPolyLine:
    case Imagine::kFmiEsriPolyLineM:
    case Imagine::kFmiEsriPolyLineZ:
    {
      print_parts(*static_cast<const Imagine::NFmiEsriPolyLine*>(&theElement), "");
      break;
    }
    case Imagine::kFmiEsriPolygon:
    case Imagine::kFmiEsriPolygonM:
    case Imagine::kFmiEsriPolygonZ:
    {
      print_parts(*static_cast<const Imagine::NFmiEsriPolygon*>(&theElement), " Z");
      break;
    }
  }
}

int main(int argc, char *argv[])
{
  // Command line arguments
  if (argc != 2)
  {
    cerr << "Usage: shape2xml [shapefile]" << endl;
    return 1;
  }
  string shapefile = argv[1];

  try
  {
    Imagine::NFmiEsriShape shape;
    if (!shape.Read(shapefile, true))
      throw std::runtime_error("Failed to read " + shapefile);

    cout << "<shapefile filename=\"" << shapefile << "\">" << endl;

    const Imagine::NFmiEsriShape::attributes_type &attributes = shape.Attributes();

    {
      cout << "<attributelist>" << endl;
      for (auto* attribute : attributes)
        cout << " <attribute name=\"" << attribute->Name() << '"' << " type=\""
             << static_cast<int>(attribute->Type()) << '"' << "/>" << endl;
      cout << "</attributelist>" << endl;
    }

    int shapenumber = -1;
    for (auto* it : shape.Elements())
    {
      ++shapenumber;

      if (it == nullptr)
        continue;

      cout << "<shape id=\"" << shapenumber << '"' << " type=\"" << static_cast<int>(it->Type())
           << '"';

      print_attributes(*it, attributes);
      cout << '>' << endl;

      print_element_path(*it);
      cout << "</shape>" << endl;
    }

    cout << "</shapefile>" << endl;
  }
  catch (const std::exception &e)
  {
    cerr << "Error: shape2xml failed" << endl << " --> " << e.what() << endl;
    return 1;
  }
  return 0;
}
