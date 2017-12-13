/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstaroffice
* Version: MPL 2.0 / LGPLv2+
*
* The contents of this file are subject to the Mozilla Public License Version
* 2.0 (the "License"); you may not use this file except in compliance with
* the License or as specified alternatively below. You may obtain a copy of
* the License at http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* Major Contributor(s):
* Copyright (C) 2002 William Lachance (wrlach@gmail.com)
* Copyright (C) 2002,2004 Marc Maurer (uwog@uwog.net)
* Copyright (C) 2004-2006 Fridrich Strba (fridrich.strba@bluewin.ch)
* Copyright (C) 2006, 2007 Andrew Ziem
* Copyright (C) 2011, 2012 Alonso Laurent (alonso@loria.fr)
*
*
* All Rights Reserved.
*
* For minor contributions see the git repository.
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU Lesser General Public License Version 2 or later (the "LGPLv2+"),
* in which case the provisions of the LGPLv2+ are applicable
* instead of those above.
*/

#ifndef STAR_GRAPHIC_STRUCT
#  define STAR_GRAPHIC_STRUCT

#include <ostream>
#include <string>
#include <vector>

#include "libstaroffice_internal.hxx"

class StarBitmap;
class StarObject;
class StarZone;

//! a name use to define basic StarOffice graphic structure
namespace StarGraphicStruct
{
//! Class to store a brush
class StarBrush
{
public:
  //! constructor
  StarBrush()
    : m_transparency(0)
    , m_color(STOFFColor::white())
    , m_fillColor(STOFFColor::white())
    , m_style(0)
    , m_position(0)
    , m_linkName("")
    , m_filterName("")
    , m_extra("")
  {
  }
  //! returns true if the brush is empty
  bool isEmpty() const
  {
    return m_style<=0 || m_style>=11 || m_transparency>=255;
  }
  //! returns true is the brush has unique color
  bool hasUniqueColor() const
  {
    return m_style==1;
  }
  //! try to return a color corresponding to the brush
  bool getColor(STOFFColor &color) const;
  //! try to return a pattern corresponding to the brush
  bool getPattern(STOFFEmbeddedObject &object, STOFFVec2i &sz) const;
  //! try to read a brush
  bool read(StarZone &zone, int nVers, long endPos, StarObject &document);
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, StarBrush const &brush);
  //! the transparency
  int m_transparency;
  //! the color
  STOFFColor m_color;
  //! the fill color
  STOFFColor m_fillColor;
  /** the brush style(pattern): BRUSH_NULL, BRUSH_SOLID, BRUSH_HORZ, BRUSH_VERT, BRUSH_CROSS,
      BRUSH_DIAGCROSS, BRUSH_UPDIAG, BRUSH_DOWNDIAG, BRUSH_25, BRUSH_50, BRUSH_75, BRUSH_BITMAP */
  int m_style;
  //! the position(none, lt, mt, rt, lm, mm, rm, lb, mb, rb, area, tiled)
  int m_position;
  //! the link name
  librevenge::RVNGString m_linkName;
  //! the filter name
  librevenge::RVNGString m_filterName;
  //! extra data
  std::string m_extra;
};

//! Class to store a graphic
class StarGraphic
{
public:
  //! constructor
  StarGraphic()
    : m_object()
    , m_bitmap()
  {
  }
  //! try to read a graphic, give lastPos if you are sure of the end graphic position
  bool read(StarZone &zone, long lastPos=-1);
  //! the embedded object
  STOFFEmbeddedObject m_object;
  //! the bitmap
  std::shared_ptr<StarBitmap> m_bitmap;
};

//! Class to store a polygon
class StarPolygon
{
public:
  //! a structure to keep a point and a flag
  struct Point {
    //! constructor
    explicit Point(STOFFVec2i point=STOFFVec2i(), int flag=0)
      : m_point(point)
      , m_flags(flag)
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Point const &point)
    {
      o << point.m_point;
      switch (point.m_flags) {
      case 0:
        break;
      case 1: // smooth
        o << ":s";
        break;
      case 2: // control
        o << ":c";
        break;
      case 3: // symetric
        o << ":S";
        break;
      default:
        STOFF_DEBUG_MSG(("StarObjectSmallGraphicStruct::StarPolygon::Point::operator<< unexpected flag\n"));
        o << ":[##" << point.m_flags << "]";
      }
      return o;
    }
    //! the point
    STOFFVec2i m_point;
    //! the flags
    int m_flags;
  };
  //! contructor
  StarPolygon()
    : m_points()
  {
  }
  //! check if a polygon has special point
  bool hasSpecialPoints() const
  {
    for (auto const &pt : m_points) {
      if (pt.m_flags)
        return true;
    }
    return false;
  }
  //! returns true if the polygon is empty
  bool empty() const
  {
    return m_points.empty();
  }
  //! returns true if the polygon is empty
  size_t size() const
  {
    return m_points.size();
  }
  //! add the polygon to a path
  void addToPath(librevenge::RVNGPropertyListVector &path, bool isClosed, double relUnit, STOFFVec2f const &decal) const;
  //! convert a path in a string and update the bdbox
  bool convert(librevenge::RVNGString &path, librevenge::RVNGString &viewbox, double relUnit, STOFFVec2f const &decal) const;
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, StarPolygon const &poly);
  //! the list of points
  std::vector<Point> m_points;
};

}

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
