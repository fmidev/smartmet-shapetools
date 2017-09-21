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
    if (!shape.Read(shapefile, true)) throw std::runtime_error("Failed to read " + shapefile);

    cout << "<shapefile filename=\"" << shapefile << "\">" << endl;

    const Imagine::NFmiEsriShape::attributes_type &attributes = shape.Attributes();

    {
      cout << "<attributelist>" << endl;
      for (Imagine::NFmiEsriShape::attributes_type::const_iterator ait = attributes.begin();
           ait != attributes.end();
           ++ait)
        cout << " <attribute name=\"" << (*ait)->Name() << '"' << " type=\""
             << static_cast<int>((*ait)->Type()) << '"' << "/>" << endl;
      cout << "</attributelist>" << endl;
    }

    int shapenumber = -1;
    for (Imagine::NFmiEsriShape::const_iterator it = shape.Elements().begin();
         it != shape.Elements().end();
         ++it)
    {
      ++shapenumber;

      if (*it == 0) continue;

      cout << "<shape id=\"" << shapenumber << '"' << " type=\"" << static_cast<int>((*it)->Type())
           << '"';

      for (Imagine::NFmiEsriShape::attributes_type::const_iterator ait = attributes.begin();
           ait != attributes.end();
           ++ait)
      {
        cout << ' ' << (*ait)->Name() << "=\"";
        switch ((*ait)->Type())
        {
          case Imagine::kFmiEsriString:
            cout << (*it)->GetString((*ait)->Name());
            break;
          case Imagine::kFmiEsriInteger:
            cout << (*it)->GetInteger((*ait)->Name());
            break;
          case Imagine::kFmiEsriDouble:
            cout << (*it)->GetDouble((*ait)->Name());
            break;
        }
        cout << '"';
      }
      cout << '>' << endl;

      switch ((*it)->Type())
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
          cout << "M " << x << ' ' << y << endl;
          break;
        }
        case Imagine::kFmiEsriMultiPoint:
        case Imagine::kFmiEsriMultiPointM:
        case Imagine::kFmiEsriMultiPointZ:
        {
          const Imagine::NFmiEsriMultiPoint *elem =
              static_cast<const Imagine::NFmiEsriMultiPoint *>(*it);
          for (int i = 0; i < elem->NumPoints(); i++)
          {
            const float x = elem->Points()[i].X();
            const float y = elem->Points()[i].Y();
            if (i > 0) cout << ' ';
            cout << "M " << x << ' ' << y;
          }
          cout << endl;
          break;
        }
        case Imagine::kFmiEsriPolyLine:
        case Imagine::kFmiEsriPolyLineM:
        case Imagine::kFmiEsriPolyLineZ:
        {
          const Imagine::NFmiEsriPolyLine *elem =
              static_cast<const Imagine::NFmiEsriPolyLine *>(*it);
          for (int part = 0; part < elem->NumParts(); part++)
          {
            int i1, i2;
            i1 = elem->Parts()[part];  // start of part
            if (part + 1 == elem->NumParts())
              i2 = elem->NumPoints() - 1;  // end of part
            else
              i2 = elem->Parts()[part + 1] - 1;  // end of part

            if (i2 >= i1)
            {
              if (part > 0) cout << endl;
              cout << "M " << elem->Points()[i1].X() << ' ' << elem->Points()[i1].Y();
              for (int i = i1 + 1; i <= i2; i++)
                cout << " L " << elem->Points()[i].X() << ' ' << elem->Points()[i].Y();
              cout << endl;
            }
          }
          break;
        }
        case Imagine::kFmiEsriPolygon:
        case Imagine::kFmiEsriPolygonM:
        case Imagine::kFmiEsriPolygonZ:
        {
          const Imagine::NFmiEsriPolygon *elem = static_cast<const Imagine::NFmiEsriPolygon *>(*it);
          for (int part = 0; part < elem->NumParts(); part++)
          {
            int i1, i2;
            i1 = elem->Parts()[part];  // start of part
            if (part + 1 == elem->NumParts())
              i2 = elem->NumPoints() - 1;  // end of part
            else
              i2 = elem->Parts()[part + 1] - 1;  // end of part

            if (i2 >= i1)
            {
              if (part > 0) cout << endl;
              cout << "M " << elem->Points()[i1].X() << ' ' << elem->Points()[i1].Y();
              for (int i = i1 + 1; i <= i2; i++)
                cout << " L " << elem->Points()[i].X() << ' ' << elem->Points()[i].Y();
              cout << " Z" << endl;
            }
          }
          break;
        }
      }
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
