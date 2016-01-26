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

#include <iostream>

#include "STOFFCellStyle.hxx"
#include "STOFFFont.hxx"
#include "STOFFGraphicStyle.hxx"

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarGraphicStruct.hxx"
#include "StarItemPool.hxx"
#include "StarLanguage.hxx"
#include "StarObject.hxx"
#include "StarZone.hxx"

#include "StarGraphicAttribute.hxx"

namespace StarGraphicAttribute
{
//! a character bool attribute
class StarGAttributeBool : public StarAttributeBool
{
public:
  //! constructor
  StarGAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeBool(*this));
  }
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;

protected:
  //! copy constructor
  StarGAttributeBool(StarGAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

//! a character color attribute
class StarGAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarGAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeColor(*this));
  }
  //! add to a graphic style
  //virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarGAttributeColor(StarGAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

//! a character integer attribute
class StarGAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarGAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarGAttributeInt(StarGAttributeInt const &orig) : StarAttributeInt(orig)
  {
  }
};

//! a character unsigned integer attribute
class StarGAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarGAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeUInt(*this));
  }
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarGAttributeUInt(StarGAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

//! a void attribute
class StarGAttributeVoid : public StarAttributeVoid
{
public:
  //! constructor
  StarGAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeVoid(*this));
  }
  //! add to a graphic style
  //virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarGAttributeVoid(StarGAttributeVoid const &orig) : StarAttributeVoid(orig)
  {
  }
};

//! a list of item attribute of StarAttributeInternal
class StarGAttributeItemSet : public StarAttributeItemSet
{
public:
  //! constructor
  StarGAttributeItemSet(Type type, std::string const &debugName, std::vector<STOFFVec2i> const &limits) :
    StarAttributeItemSet(type, debugName, limits)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeItemSet(*this));
  }

protected:
  //! copy constructor
  explicit StarGAttributeItemSet(StarAttributeItemSet const &orig) : StarAttributeItemSet(orig)
  {
  }
};

void StarGAttributeBool::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_LINESTARTCENTER)
    graphic.m_propertyList.insert("draw:marker-start-center", m_value);
  else if (m_type==XATTR_LINEENDCENTER)
    graphic.m_propertyList.insert("draw:marker-end-center", m_value);
  else if (m_type==XATTR_FILLBMP_TILE && m_value)
    graphic.m_propertyList.insert("style:repeat", "repeat");
  else if (m_type==XATTR_FILLBMP_STRETCH && m_value)
    graphic.m_propertyList.insert("style:repeat", "stretch");
  else if (m_type==XATTR_FILLBACKGROUND)
    graphic.m_hasBackground=m_value;
  // TODO: XATTR_FILLBMP_SIZELOG
}

void StarGAttributeInt::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_LINEWIDTH)
    graphic.m_propertyList.insert("svg:stroke-width", double(m_value)/2540.,librevenge::RVNG_INCH);
  else if (m_type==XATTR_LINESTARTWIDTH)
    graphic.m_propertyList.insert("draw:marker-start-width", double(m_value)/2540.,librevenge::RVNG_INCH);
  else if (m_type==XATTR_LINEENDWIDTH)
    graphic.m_propertyList.insert("draw:marker-end-width", double(m_value)/2540.,librevenge::RVNG_INCH);
  else if (m_type==XATTR_FILLBMP_SIZEX)
    graphic.m_propertyList.insert("draw:fill-image-width", double(m_value)/2540.,librevenge::RVNG_INCH);
  else if (m_type==XATTR_FILLBMP_SIZEY)
    graphic.m_propertyList.insert("draw:fill-image-height", double(m_value)/2540.,librevenge::RVNG_INCH);
}

void StarGAttributeUInt::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_LINESTYLE) {
    if (m_value<=2) {
      char const *(wh[])= {"none", "solid", "dash"};
      graphic.m_propertyList.insert("draw:stroke", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown line style %d\n", int(m_value)));
    }
  }
  else if (m_type==XATTR_FILLSTYLE) {
    if (m_value<=4) {
      char const *(wh[])= {"none", "solid", "gradient", "hatch", "bitmap"};
      graphic.m_propertyList.insert("draw:fill", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown fill style %d\n", int(m_value)));
    }
  }
  else if (m_type==XATTR_LINETRANSPARENCE)
    graphic.m_propertyList.insert("svg:stroke-opacity", 1-double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_FILLTRANSPARENCE)
    graphic.m_propertyList.insert("draw:opacity", 1-double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_LINEJOINT) {
    if (m_value<=4) {
      char const *(wh[])= {"none", "middle", "bevel", "miter", "round"};
      graphic.m_propertyList.insert("draw:stroke-linejoin", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown line join %d\n", int(m_value)));
    }
  }
  else if (m_type==XATTR_FILLBMP_POS) {
    if (m_value<9) {
      char const *(wh[])= {"top-left", "top", "top-right", "left", "center", "right", "bottom-left", "bottom", "bottom-right"};
      graphic.m_propertyList.insert("draw:fill-image-ref-point", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown bmp position %d\n", int(m_value)));
    }
  }
  // CHECKME: from here never seen
  else if (m_type==XATTR_GRADIENTSTEPCOUNT)
    graphic.m_propertyList.insert("draw:gradient-step-count", m_value, librevenge::RVNG_GENERIC);
  else if (m_type==XATTR_FILLBMP_POSOFFSETX)
    graphic.m_propertyList.insert("draw:fill-image-ref-point-x", double(m_value)/100, librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_FILLBMP_POSOFFSETY)
    graphic.m_propertyList.insert("draw:fill-image-ref-point-y", double(m_value)/100, librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_FILLBMP_TILEOFFSETX || XATTR_FILLBMP_TILEOFFSETY) {
    std::stringstream s;
    s << m_value << "% " << (m_type==XATTR_FILLBMP_TILEOFFSETX ? "horizontal" : "vertical");
    graphic.m_propertyList.insert("draw:tile-repeat-offset", s.str().c_str());
  }
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeVoid(type,debugName));
}

}

namespace StarGraphicAttribute
{
// ------------------------------------------------------------
//! a border attribute
class StarGAttributeBorder : public StarAttribute
{
public:
  //! constructor
  StarGAttributeBorder(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_distance(0)
  {
    for (int i=0; i<4; ++i) m_distances[i]=0;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeBorder(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_distance) o << "dist=" << m_distance << ",";
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        o << "border" << i << "=[" << m_borders[i] << "],";
    }
    bool hasDistances=false;
    for (int i=0; i<4; ++i) {
      if (m_distances[i]) {
        hasDistances=true;
        break;
      }
    }
    if (hasDistances) {
      o << "dists=[";
      for (int i=0; i<4; ++i)
        o << m_distances[i] << ",";
      o << "],";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeBorder(StarGAttributeBorder const &orig) : StarAttribute(orig), m_distance(orig.m_distance)
  {
    for (int i=0; i<4; ++i) {
      m_borders[i]=orig.m_borders[i];
      m_distances[i]=orig.m_distances[i];
    }
  }
  //! the distance
  int m_distance;
  //! the border list: top, left, right, bottom
  STOFFBorderLine m_borders[4];
  //! the padding distance: top, left, right, bottom
  int m_distances[4];
};

//! a box info attribute
class StarGAttributeBoxInfo : public StarAttribute
{
public:
  //! constructor
  StarGAttributeBoxInfo(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_distance(0), m_borderList(), m_flags(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeBoxInfo(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_distance) o << "dist=" << m_distance << ",";
    for (size_t i=0; i<m_borderList.size(); ++i) {
      if (!m_borderList[i].isEmpty())
        o << "border" << i << "=[" << m_borderList[i] << "],";
    }
    if (m_flags&1) o << "set[table],";
    if (m_flags&2) o << "set[dist],";
    if (m_flags&4) o << "set[minDist],";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeBoxInfo(StarGAttributeBoxInfo const &orig) : StarAttribute(orig), m_distance(orig.m_distance), m_borderList(orig.m_borderList), m_flags(orig.m_flags)
  {
  }
  //! the distance
  int m_distance;
  //! the boxInfo list: top, left, right, bottom
  std::vector<STOFFBorderLine> m_borderList;
  //! some flags: setTable, setDist, setMinDist
  int m_flags;
};

//! a brush attribute
class StarGAttributeBrush : public StarAttribute
{
public:
  //! constructor
  StarGAttributeBrush(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_brush()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeBrush(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFFont &font, StarItemPool const */*pool*/) const;
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[" << m_brush << "],";
  }

protected:
  //! copy constructor
  StarGAttributeBrush(StarGAttributeBrush const &orig) : StarAttribute(orig), m_brush(orig.m_brush)
  {
  }
  //! the brush
  StarGraphicStruct::StarBrush m_brush;
};

//! a named attribute
class StarGAttributeNamed : public StarAttribute
{
public:
  //! constructor
  StarGAttributeNamed(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_named(), m_namedId(0)
  {
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (!m_named.empty())
      o << m_named.cstr() << ",";
    if (m_namedId>=0)
      o << "id=" << m_namedId << ",";
    o << "]";
  }

protected:
  //! copy constructor
  StarGAttributeNamed(StarGAttributeNamed const &orig) : StarAttribute(orig), m_named(orig.m_named), m_namedId(orig.m_namedId)
  {
  }
  //! the named
  librevenge::RVNGString m_named;
  //! the name id
  int m_namedId;
};

//! a arrow's named attribute
class StarGAttributeNamedArrow : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedArrow(Type type, std::string const &debugName) : StarGAttributeNamed(type, debugName), m_polygon()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeNamedArrow(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarGAttributeNamed::print(o);
    o << ":[" << m_polygon << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedArrow(StarGAttributeNamedArrow const &orig) : StarGAttributeNamed(orig), m_polygon(orig.m_polygon)
  {
  }
  //! the polygon
  StarGraphicStruct::StarPolygon m_polygon;
};

//! a bitmap's named attribute
class StarGAttributeNamedBitmap : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedBitmap(Type type, std::string const &debugName) :
    StarGAttributeNamed(type, debugName), m_bitmap()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeNamedBitmap(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarGAttributeNamed::print(o);
    if (m_bitmap.isEmpty()) o << "empty";
    o << ",";
  }

protected:
  //! copy constructor
  StarGAttributeNamedBitmap(StarGAttributeNamedBitmap const &orig) : StarGAttributeNamed(orig), m_bitmap(orig.m_bitmap)
  {
  }
  //! the bitmap
  STOFFEmbeddedObject m_bitmap;
};

//! a color's named attribute
class StarGAttributeNamedColor : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedColor(Type type, std::string const &debugName, STOFFColor const &defColor) :
    StarGAttributeNamed(type, debugName), m_color(defColor)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeNamedColor(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFFont &font, StarItemPool const */*pool*/) const;
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarGAttributeNamed::print(o);
    o << ":[" << m_color << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedColor(StarGAttributeNamedColor const &orig) : StarGAttributeNamed(orig), m_color(orig.m_color)
  {
  }
  //! the color
  STOFFColor m_color;
};

//! a dash's named attribute
class StarGAttributeNamedDash : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedDash(Type type, std::string const &debugName) :
    StarGAttributeNamed(type, debugName), m_dashStyle(0), m_distance(20)
  {
    for (int i=0; i<2; ++i) m_numbers[i]=1;
    for (int i=0; i<2; ++i) m_lengths[i]=20;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeNamedDash(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarGAttributeNamed::print(o);
    o << ":[";
    o << "style=" << m_dashStyle << ",";
    for (int i=0; i<2; ++i) {
      if (m_numbers[i])
        o << (i==0 ? "dots" : "dashs") << "=" << m_numbers[i] << ":" << m_lengths[i] << ",";
    }
    if (m_distance) o << "distance=" << m_distance << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedDash(StarGAttributeNamedDash const &orig) : StarGAttributeNamed(orig), m_dashStyle(orig.m_dashStyle), m_distance(orig.m_distance)
  {
    for (int i=0; i<2; ++i) m_numbers[i]=orig.m_numbers[i];
    for (int i=0; i<2; ++i) m_lengths[i]=orig.m_lengths[i];
  }
  //! the style:  XDASH_RECT, XDASH_ROUND, XDASH_RECTRELATIVE, XDASH_ROUNDRELATIVE
  int m_dashStyle;
  //! the number of dot/dash
  int m_numbers[2];
  //! the lengths of dot/dash
  int m_lengths[2];
  //! the distance
  int m_distance;
};

//! a gradient's named attribute
class StarGAttributeNamedGradient : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedGradient(Type type, std::string const &debugName) : StarGAttributeNamed(type, debugName), m_gradientType(0), m_enable(true), m_angle(0), m_border(0), m_step(0)
  {
    for (int i=0; i<2; ++i) {
      m_offsets[i]=50;
      m_intensities[i]=100;
    }
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeNamedGradient(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarGAttributeNamed::print(o);
    o << ":[";
    o << "type=" << m_gradientType << ",";
    if (m_angle) o << "angle=" << m_angle << ",";
    if (m_border) o << "border=" << m_border << ",";
    if (m_step) o << "step=" << m_step << ",";
    o << m_colors[0] << "->" << m_colors[1] << ",";
    o << "offset=" << m_offsets[0] << "->" << m_offsets[1] << ",";
    o << "intensity=" << m_intensities[0] << "->" << m_intensities[1] << ",";
    if (!m_enable) o << "disable,";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedGradient(StarGAttributeNamedGradient const &orig) : StarGAttributeNamed(orig), m_gradientType(orig.m_gradientType), m_enable(orig.m_enable), m_angle(orig.m_angle), m_border(orig.m_border), m_step(orig.m_step)
  {
    for (int i=0; i<2; ++i) {
      m_colors[i]=orig.m_colors[i];
      m_offsets[i]=orig.m_offsets[i];
      m_intensities[i]=orig.m_intensities[i];
    }
  }
  //! the gradient type
  int m_gradientType;
  //! a flag to know if the gradient is enable
  bool m_enable;
  //! the angle
  int m_angle;
  //! the border
  int m_border;
  //! the step
  int m_step;
  //! the colors
  STOFFColor m_colors[2];
  //! the x offsets
  int m_offsets[2];
  //! the intensities
  int m_intensities[2];
};

//! a hatch's named attribute
class StarGAttributeNamedHatch : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedHatch(Type type, std::string const &debugName) : StarGAttributeNamed(type, debugName), m_hatchType(0), m_color(STOFFColor::black()), m_distance(0), m_angle(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeNamedHatch(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarGAttributeNamed::print(o);
    o << ":[";
    o << "type=" << m_hatchType << ",";
    o << m_color << ",";
    if (m_distance) o << "distance=" << m_distance << ",";
    if (m_angle) o << "angle=" << m_angle << ",";
  }

protected:
  //! copy constructor
  StarGAttributeNamedHatch(StarGAttributeNamedHatch const &orig) : StarGAttributeNamed(orig), m_hatchType(orig.m_hatchType), m_color(orig.m_color), m_distance(orig.m_distance), m_angle(orig.m_angle)
  {
  }
  //! the type
  int m_hatchType;
  //! the color
  STOFFColor m_color;
  //! the distance
  int m_distance;
  //! the angle
  int m_angle;
};

//! a shadow attribute
class StarGAttributeShadow : public StarAttribute
{
public:
  //! constructor
  StarGAttributeShadow(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_location(0), m_width(0), m_transparency(0), m_color(STOFFColor::white()),
    m_fillColor(STOFFColor::white()), m_style(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarGAttributeShadow(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_location) o << "loc=" << m_location << ",";
    if (m_width) o << "width=" << m_width << ",";
    if (m_transparency) o << "transparency=" << m_transparency << ",";
    if (!m_color.isWhite()) o << "col=" << m_color << ",";
    if (!m_fillColor.isWhite()) o << "col[fill]=" << m_fillColor << ",";
    if (m_style) o << "style=" << m_style << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeShadow(StarGAttributeShadow const &orig) :
    StarAttribute(orig), m_location(orig.m_location), m_width(orig.m_width), m_transparency(orig.m_transparency), m_color(orig.m_color),
    m_fillColor(orig.m_fillColor), m_style(orig.m_style)
  {
  }
  //! the location 0-4
  int m_location;
  //! the width in twip
  int m_width;
  //! the trans?
  int m_transparency;
  //! the color
  STOFFColor m_color;
  //! the fill color
  STOFFColor m_fillColor;
  //! the style
  int m_style;
};

void StarGAttributeBorder::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type==ATTR_SC_BORDER) {
    // checkme what is m_distance?
    char const * (wh[])= {"top", "left", "right", "bottom"};
    for (int i=0; i<4; ++i)
      m_borders[i].addTo(cell.m_propertyList, wh[i]);
  }
}

void StarGAttributeBorder::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==ATTR_FRM_BOX) {
    // checkme what is m_distance?
    char const * (wh[])= {"top", "left", "right", "bottom"};
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        m_borders[i].addTo(graphic.m_propertyList, wh[i]);
    }
    for (int i=0; i<4; ++i)
      graphic.m_propertyList.insert((std::string("padding-")+wh[i]).c_str(), float(m_distances[i])/20.f, librevenge::RVNG_POINT);
  }
}

void StarGAttributeBrush::addTo(STOFFFont &font, StarItemPool const */*pool*/) const
{
  if (m_type != ATTR_CHR_BACKGROUND)
    return;
  if (m_brush.isEmpty()) {
    font.m_propertyList.insert("fo:background-color", "transparent");
    return;
  }
  STOFFColor color;
  if (m_brush.getColor(color)) {
    font.m_propertyList.insert("fo:background-color", color.str().c_str());
    return;
  }
  else {
    font.m_propertyList.insert("fo:background-color", "transparent");
    STOFF_DEBUG_MSG(("StarGAttributeBrush::addTo: can not set a font background\n"));
  }
}

void StarGAttributeBrush::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type != ATTR_SC_BACKGROUND)
    return;
  if (m_brush.isEmpty()) {
    cell.m_propertyList.insert("fo:background-color", "transparent");
    return;
  }
  STOFFColor color;
#if 1
  if (m_brush.getColor(color)) {
    cell.m_propertyList.insert("fo:background-color", color.str().c_str());
    return;
  }
  STOFF_DEBUG_MSG(("StarGAttributeBrush::addTo: can not set a cell background\n"));
  cell.m_propertyList.insert("fo:background-color", "transparent");
#else
  /* checkme, is it possible to use style:background-image here ?
     Can not create any working ods file with bitmap cell's background...
   */
  if (m_brush.hasUniqueColor() && m_brush.getColor(color)) {
    cell.m_propertyList.insert("fo:background-color", color.str().c_str());
    return;
  }
  STOFFEmbeddedObject object;
  STOFFVec2i size;
  if (m_brush.getPattern(object, size) && object.m_dataList.size()) {
    librevenge::RVNGPropertyList backgroundList;
    backgroundList.insert("librevenge:bitmap", object.m_dataList[0].getBase64Data());
    backgroundList.insert("xlink:type", "simple");
    backgroundList.insert("xlink:show", "embed");
    backgroundList.insert("xlink:actuate", "onLoad");
    backgroundList.insert("style:filter-name", object.m_typeList.empty() ? "image/pict" : object.m_typeList[0].c_str());
    if (m_brush.m_transparency>0 && m_brush.m_transparency<=255)
      backgroundList.insert("draw:opacity", 1.-double(m_brush.m_transparency)/255., librevenge::RVNG_PERCENT);
    if (m_brush.m_position>=1 && m_brush.m_position<=9) {
      int xPos=(m_brush.m_position-1)%3, yPos=(m_brush.m_position-1)%3;
      backgroundList.insert("style:position",
                            (std::string(xPos==0 ? "left " : xPos==1 ? "center " : "right ")+
                             std::string(yPos==0 ? "top" : yPos==1 ? "center" : "bottom")).c_str());
    }
    librevenge::RVNGPropertyListVector backgroundVector;
    backgroundVector.append(backgroundList);
    cell.m_propertyList.insert("librevenge:background-image", backgroundVector);
  }
  else {
    STOFF_DEBUG_MSG(("StarGAttributeBrush::addTo: can not set a cell background\n"));
  }
#endif
}

void StarGAttributeBrush::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type!=ATTR_FRM_BACKGROUND)
    return;
  if (m_brush.m_transparency>0 && m_brush.m_transparency<=255)
    graphic.m_propertyList.insert("draw:opacity", 1.-double(m_brush.m_transparency)/255., librevenge::RVNG_PERCENT);
  else
    graphic.m_propertyList.insert("draw:opacity",1., librevenge::RVNG_PERCENT);
  if (m_brush.isEmpty()) {
    graphic.m_propertyList.insert("draw:fill", "none");
    return;
  }
  STOFFColor color;
  if (m_brush.hasUniqueColor() && m_brush.getColor(color)) {
    graphic.m_propertyList.insert("draw:fill", "solid");
    graphic.m_propertyList.insert("draw:fill-color", color.str().c_str());
    return;
  }
  STOFFEmbeddedObject object;
  STOFFVec2i size;
  if (m_brush.getPattern(object, size) && object.m_dataList.size()) {
    graphic.m_propertyList.insert("draw:fill", "bitmap");
    graphic.m_propertyList.insert("draw:fill-image", object.m_dataList[0].getBase64Data());
    graphic.m_propertyList.insert("draw:fill-image-width", size[0], librevenge::RVNG_POINT);
    graphic.m_propertyList.insert("draw:fill-image-height", size[1], librevenge::RVNG_POINT);
    graphic.m_propertyList.insert("draw:fill-image-ref-point-x",0, librevenge::RVNG_POINT);
    graphic.m_propertyList.insert("draw:fill-image-ref-point-y",0, librevenge::RVNG_POINT);
    graphic.m_propertyList.insert("librevenge:mime-type", object.m_typeList.empty() ? "image/pict" : object.m_typeList[0].c_str());
  }
  else
    graphic.m_propertyList.insert("draw:fill", "none");
}

void StarGAttributeNamedArrow::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_LINESTART || m_type==XATTR_LINEEND) {
    char const *pathName=m_type==XATTR_LINESTART ? "draw:marker-start-path" : "draw:marker-end-path";
    char const *viewboxName=m_type==XATTR_LINESTART ? "draw:marker-start-viewbox" : "draw:marker-end-viewbox";
    if (m_polygon.empty()) {
      if (graphic.m_propertyList[pathName]) graphic.m_propertyList.remove(pathName);
      if (graphic.m_propertyList[viewboxName]) graphic.m_propertyList.remove(viewboxName);
    }
    else {
      librevenge::RVNGString path, viewbox;
      if (m_polygon.convert(path, viewbox)) {
        graphic.m_propertyList.insert(pathName, path);
        graphic.m_propertyList.insert(viewboxName, viewbox);
      }
    }
  }
}

void StarGAttributeNamedBitmap::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_FILLBITMAP) {
    if (!m_bitmap.isEmpty())
      m_bitmap.addAsFillImageTo(graphic.m_propertyList);
    else {
      STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::addTo: can not find the bitmap\n"));
    }
  }
}

void StarGAttributeNamedColor::addTo(STOFFFont &font, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_FORMTXTSHDWCOLOR)
    font.m_shadowColor=m_color;
}

void StarGAttributeNamedColor::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_LINECOLOR)
    graphic.m_propertyList.insert("svg:stroke-color", m_color.str().c_str());
  else if (m_type==XATTR_FILLCOLOR)
    graphic.m_propertyList.insert("draw:fill-color", m_color.str().c_str());
  else if (m_type==SDRATTR_SHADOWCOLOR)
    graphic.m_propertyList.insert("draw:shadow-color", m_color.str().c_str());
}

void StarGAttributeNamedDash::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_LINEDASH) {
    graphic.m_propertyList.insert("draw:dots1", m_numbers[0]);
    graphic.m_propertyList.insert("draw:dots1-length", double(m_lengths[0])/2540., librevenge::RVNG_INCH);
    graphic.m_propertyList.insert("draw:dots2", m_numbers[1]);
    graphic.m_propertyList.insert("draw:dots2-length", double(m_lengths[1])/2540., librevenge::RVNG_INCH);
    graphic.m_propertyList.insert("draw:distance", double(m_distance)/2540., librevenge::RVNG_INCH);;
  }
}

void StarGAttributeNamedGradient::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_FILLGRADIENT) {
    // TODO XATTR_FILLFLOATTRANSPARENCE
    if (!m_enable)
      return;
    if (m_gradientType>=0 && m_gradientType<=5) {
      char const *(wh[])= {"linear", "axial", "radial", "ellipsoid", "square", "rectangle"};
      graphic.m_propertyList.insert("draw:style", wh[m_gradientType]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeNamedGradient::addTo: unknown hash type %d\n", m_gradientType));
    }
    graphic.m_propertyList.insert("draw:angle", double(m_angle)/10, librevenge::RVNG_GENERIC);
    graphic.m_propertyList.insert("draw:border", double(m_border)/100, librevenge::RVNG_PERCENT);
    graphic.m_propertyList.insert("draw:start-color", m_colors[0].str().c_str());
    graphic.m_propertyList.insert("librevenge:start-opacity", double(m_intensities[0])/100, librevenge::RVNG_PERCENT);
    graphic.m_propertyList.insert("draw:end-color", m_colors[1].str().c_str());
    graphic.m_propertyList.insert("librevenge:end-opacity", double(m_intensities[1])/100, librevenge::RVNG_PERCENT);
    graphic.m_propertyList.insert("svg:cx", double(m_offsets[0])/100, librevenge::RVNG_PERCENT);
    graphic.m_propertyList.insert("svg:cy", double(m_offsets[1])/100, librevenge::RVNG_PERCENT);
    // m_step ?
  }
}

void StarGAttributeNamedHatch::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_type==XATTR_FILLHATCH) {
    if (m_hatchType>=0 && m_hatchType<3) {
      char const *(wh[])= {"single", "double", "triple"};
      graphic.m_propertyList.insert("draw:style", wh[m_hatchType]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeNamedHatch::addTo: unknown hash type %d\n", m_hatchType));
    }
    graphic.m_propertyList.insert("draw:color", m_color.str().c_str());
    graphic.m_propertyList.insert("draw:distance", double(m_distance)/2540.,librevenge::RVNG_INCH);
    if (m_angle) graphic.m_propertyList.insert("draw:rotation", double(m_angle)/10, librevenge::RVNG_GENERIC);
  }
}

void StarGAttributeShadow::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_width<=0 || m_location<=0 || m_location>4 || m_transparency<0 || m_transparency>=100) {
    cell.m_propertyList.insert("style:shadow", "none");
    return;
  }
  std::stringstream s;
  s << m_color.str().c_str() << " "
    << ((m_location%2)?-1:1)*double(m_width)/20. << "pt "
    << (m_location<=2?-1:1)*double(m_width)/20. << "pt";
  cell.m_propertyList.insert("style:shadow", s.str().c_str());
}

void StarGAttributeShadow::addTo(STOFFGraphicStyle &graphic, StarItemPool const */*pool*/) const
{
  if (m_width<=0 || m_location<=0 || m_location>4 || m_transparency<0 || m_transparency>=255) {
    graphic.m_propertyList.insert("draw:shadow", "hidden");
    return;
  }
  graphic.m_propertyList.insert("draw:shadow", "visible");
  graphic.m_propertyList.insert("draw:shadow-color", m_color.str().c_str());
  graphic.m_propertyList.insert("draw:shadow-opacity", 1.f-float(m_transparency)/255.f, librevenge::RVNG_PERCENT);
  graphic.m_propertyList.insert("draw:shadow-offset-x", ((m_location%2)?-1:1)*double(m_width)/20., librevenge::RVNG_POINT);
  graphic.m_propertyList.insert("draw:shadow-offset-y", (m_location<=2?-1:1)*double(m_width)/20., librevenge::RVNG_POINT);
}


bool StarGAttributeBorder::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_distance=(int) input->readULong(2);
  int cLine=0;
  bool ok=true;
  while (input->tell()<endPos) {
    cLine=(int) input->readULong(1);
    if (cLine>3) break;
    STOFFBorderLine border;
    if (!input->readColor(border.m_color)) {
      STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeBorder::read: can not find a box's color\n"));
      f << "###color,";
      ok=false;
      break;
    }
    border.m_outWidth=(int) input->readULong(2);
    border.m_inWidth=(int) input->readULong(2);
    border.m_distance=(int) input->readULong(2);
    m_borders[cLine]=border;
  }
  if (ok && nVers>=1 && cLine&0x10) {
    if (input->tell()+8 > endPos) {
      STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeBorder::read: can not find the box's borders\n"));
      f << "###distances,";
    }
    else {
      for (int i=0; i<4; ++i) m_distances[i]=(int) input->readULong(2);
    }
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeBoxInfo::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  // frmitem.cxx SvxBoxInfoItem::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_flags=(int) input->readULong(1);
  m_distance=(int) input->readULong(2);
  bool ok=true;
  while (input->tell()<endPos) {
    int cLine=(int) input->readULong(1);
    if (cLine>1) break;
    STOFFBorderLine border;
    if (!input->readColor(border.m_color)) {
      STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeBoxInfo::read: can not find a box's color\n"));
      f << "###color,";
      ok=false;
      break;
    }
    border.m_outWidth=(int) input->readULong(2);
    border.m_inWidth=(int) input->readULong(2);
    border.m_distance=(int) input->readULong(2);
    m_borderList.push_back(border);
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeBrush::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  bool transparent=false;
  *input>>transparent;
  bool ok=m_brush.read(zone, nVers, endPos, object);
  if (!ok)
    f << "###brush,";
  if (transparent)
    m_brush.m_transparency=255;
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamed::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamed::read: can not read a string\n"));
    return false;
  }
  m_named=libstoff::getString(text);
  m_namedId=int(input->readLong(4));
  return input->tell()<=endPos;
}

bool StarGAttributeNamedArrow::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeArrowNamed::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  bool ok=true;
  if (m_namedId<0) {
    uint32_t nPoints;
    *input >> nPoints;
    if (input->tell()+12*long(nPoints)>endPos) {
      STOFF_DEBUG_MSG(("StarGAttributeArrowNamed::read: bad num point\n"));
      f << "###nPoints=" << nPoints << ",";
      ok=false;
      nPoints=0;
    }
    f << "pts=[";
    m_polygon.m_points.resize(size_t(nPoints));
    for (size_t i=0; i<size_t(nPoints); ++i) {
      int dim[2];
      for (int j=0; j<2; ++j) dim[j]=(int) input->readLong(4);
      m_polygon.m_points[i].m_point=STOFFVec2i(dim[0],dim[1]);
      m_polygon.m_points[i].m_flags=(int) input->readULong(4);
    }
    f << "],";
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamedBitmap::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  bool ok=true;
  if (m_namedId<0) {
    if (nVers==1) {
      int16_t style, type;
      *input >> style >> type;
      if (style) f << "style=" << style << ",";
      if (type) f << "type=" << type << ",";
      if (type==1) {
        if (input->tell()+128+2>endPos) {
          STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: the zone seems too short\n"));
          f << "###short,";
          ok=false;
        }
        else {
          f << "val=[";
          uint32_t values[32];
          for (int i=0; i<32; ++i) {
            *input>>values[i];
            if (values[i]) f << std::hex << values[i] << std::dec << ",";
            else f << "_,";
          }
          f << "],";
          STOFFColor colors[2];
          for (int i=0; i<2; ++i) {
            if (!input->readColor(colors[i])) {
              STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: can not read a color\n"));
              f << "###color,";
              ok=false;
              break;
            }
            f << "col" << i << "=" << colors[i] << ",";
          }
          if (ok) {
            StarBitmap bitmap(values, colors);
            librevenge::RVNGBinaryData data;
            std::string dType;
            if (bitmap.getData(data, dType))
              m_bitmap.add(data,dType);
          }
        }
      }
      else if (type!=2 && type!=0) {
        STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: unexpected type\n"));
        f << "###type,";
        ok=false;
      }
      if (type!=0 || !ok) {
        print(f);
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return ok && input->tell()<=endPos;
      }
    }
    StarBitmap bitmap;
    librevenge::RVNGBinaryData data;
    std::string dType;
    if (bitmap.readBitmap(zone, true, endPos, data, dType))
      m_bitmap.add(data,dType);
    else
      ok=false;
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamedColor::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedColor::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  bool ok=true;
  if (m_namedId<0) {
    if (!input->readColor(m_color)) {
      STOFF_DEBUG_MSG(("StarGAttributeNamedColor::read: can not read a color\n"));
      f << "###color,";
      ok=false;
    }
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamedDash::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedDash::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (m_namedId<0) {
    m_dashStyle=(int) input->readULong(4);
    for (int i=0; i<2; ++i) {
      m_numbers[i]=(int) input->readULong(2);
      m_lengths[i]=(int) input->readULong(4);
    }
    m_distance=(int) input->readULong(4);
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarGAttributeNamedGradient::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedGradient::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (m_namedId<0) {
    m_gradientType=(int) input->readULong(2);
    uint16_t red, green, blue, red2, green2, blue2;
    *input >> red >> green >> blue >> red2 >> green2 >> blue2;
    m_colors[0]=STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8));
    m_colors[1]=STOFFColor(uint8_t(red2>>8),uint8_t(green2>>8),uint8_t(blue2>>8));
    m_angle=(int) input->readULong(4);
    m_border=(int) input->readULong(2);
    for (int i=0; i<2; ++i) m_offsets[i]=(int) input->readULong(2);
    for (int i=0; i<2; ++i) m_intensities[i]=(int) input->readULong(2);
    if (nVers>=1) m_step=(int) input->readULong(2);
    if (m_type==XATTR_FILLFLOATTRANSPARENCE) *input >> m_enable;
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarGAttributeNamedHatch::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedHatch::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (m_namedId<0) {
    m_hatchType=(int) input->readULong(2);
    uint16_t red, green, blue;
    *input >> red >> green >> blue;
    m_color=STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8));
    m_distance=(int) input->readLong(4);
    m_angle=(int) input->readLong(4);
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarGAttributeShadow::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  bool ok=true;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_location=(int) input->readULong(1);
  m_width=(int) input->readULong(2);
  m_transparency=(int) input->readULong(1);
  if (!input->readColor(m_color)) {
    STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeShadow::read: can not find a fill color\n"));
    f << "###color,";
    ok=false;
  }
  if (ok && !input->readColor(m_fillColor)) {
    STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeShadow::read: can not find a fill color\n"));
    f << "###fillcolor,";
    ok=false;
  }
  if (ok) m_style=(int) input->readULong(1);

  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

}

namespace StarGraphicAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  // --- xattr --- svx_xpool.cxx
  addAttributeUInt(map, StarAttribute::XATTR_LINESTYLE,"line[style]",2,1); // solid
  addAttributeInt(map, StarAttribute::XATTR_LINEWIDTH, "line[width]",4, 0); // metric
  addAttributeInt(map, StarAttribute::XATTR_LINESTARTWIDTH, "line[start,width]",4, 200); // metric
  addAttributeInt(map, StarAttribute::XATTR_LINEENDWIDTH, "line[end,width]",4, 200); // metric
  addAttributeBool(map, StarAttribute::XATTR_LINESTARTCENTER,"line[startCenter]", false);
  addAttributeBool(map, StarAttribute::XATTR_LINEENDCENTER,"line[endCenter]", false);
  addAttributeUInt(map, StarAttribute::XATTR_LINETRANSPARENCE,"line[transparence]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_LINEJOINT,"line[joint]",2,4); // use arc
  addAttributeUInt(map, StarAttribute::XATTR_FILLSTYLE,"fill[style]",2,1); // solid
  addAttributeUInt(map, StarAttribute::XATTR_FILLTRANSPARENCE,"fill[transparence]",2,0);
  addAttributeInt(map, StarAttribute::XATTR_FILLBMP_SIZEX, "fill[bmp,sizeX]",4, 0); // metric
  addAttributeInt(map, StarAttribute::XATTR_FILLBMP_SIZEY, "fill[bmp,sizeY]",4, 0); // metric
  addAttributeBool(map, StarAttribute::XATTR_FILLBMP_TILE,"fill[bmp,tile]", true);
  addAttributeBool(map, StarAttribute::XATTR_FILLBMP_STRETCH,"fill[bmp,stretch]", true);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_POS,"fill[bmp,pos]",2,4); // middle-middle
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_POSOFFSETX,"fill[bmp,pos,offX]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_POSOFFSETY,"fill[bmp,pos,offY]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_TILEOFFSETX,"fill[bmp,tile,offX]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_TILEOFFSETY,"fill[bmp,tile,offY]",2,0);
  addAttributeBool(map, StarAttribute::XATTR_FILLBACKGROUND,"fill[background]", false);
  addAttributeUInt(map, StarAttribute::XATTR_GRADIENTSTEPCOUNT,"gradient[stepCount]",2,0);

  map[StarAttribute::XATTR_LINECOLOR]=shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedColor(StarAttribute::XATTR_LINECOLOR,"line[color]",STOFFColor::black()));
  map[StarAttribute::XATTR_LINESTART]=shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedArrow(StarAttribute::XATTR_LINESTART,"line[start]"));
  map[StarAttribute::XATTR_LINEEND]=shared_ptr<StarAttribute>
                                    (new StarGAttributeNamedArrow(StarAttribute::XATTR_LINEEND,"line[end]"));
  map[StarAttribute::XATTR_LINEDASH]=shared_ptr<StarAttribute>
                                     (new StarGAttributeNamedDash(StarAttribute::XATTR_LINEDASH,"line[dash]"));
  map[StarAttribute::XATTR_FILLCOLOR]=shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedColor(StarAttribute::XATTR_FILLCOLOR,"fill[color]",STOFFColor::white()));
  map[StarAttribute::XATTR_FILLGRADIENT]=shared_ptr<StarAttribute>
                                         (new StarGAttributeNamedGradient(StarAttribute::XATTR_FILLGRADIENT,"gradient[fill]"));
  map[StarAttribute::XATTR_FILLHATCH]=shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedHatch(StarAttribute::XATTR_FILLHATCH,"hatch[fill]"));
  map[StarAttribute::XATTR_FILLBITMAP]=shared_ptr<StarAttribute>
                                       (new StarGAttributeNamedBitmap(StarAttribute::XATTR_FILLBITMAP,"bitmap[fill]"));
  map[StarAttribute::XATTR_FORMTXTSHDWCOLOR]=shared_ptr<StarAttribute>
      (new StarGAttributeNamedColor(StarAttribute::XATTR_FORMTXTSHDWCOLOR,"shadow[form,color]",STOFFColor::black()));
  map[StarAttribute::SDRATTR_SHADOWCOLOR]=shared_ptr<StarAttribute>
                                          (new StarGAttributeNamedColor(StarAttribute::SDRATTR_SHADOWCOLOR,"sdrShadow[color]",STOFFColor::black()));
  std::vector<STOFFVec2i> limits;
  limits.resize(1);
  limits[0]=STOFFVec2i(1000,1016);
  map[StarAttribute::XATTR_SET_LINE]=shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::XATTR_SET_LINE,"setLine",limits));
  limits[0]=STOFFVec2i(1018,1046);
  map[StarAttribute::XATTR_SET_FILL]=shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::XATTR_SET_FILL,"setFill",limits));
  limits[0]=STOFFVec2i(1048,1064);
  map[StarAttribute::XATTR_SET_TEXT]=shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::XATTR_SET_TEXT,"setText",limits));

  for (int type=StarAttribute::XATTR_LINERESERVED2; type<=StarAttribute::XATTR_LINERESERVED_LAST; ++type) {
    std::stringstream s;
    s << "line[reserved" << type-StarAttribute::XATTR_LINERESERVED2+2 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::XATTR_FILLRESERVED2; type<=StarAttribute::XATTR_FILLRESERVED_LAST; ++type) {
    std::stringstream s;
    s << "fill[reserved" << type-StarAttribute::XATTR_FILLRESERVED2+2 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }

  // to do
  addAttributeBool(map, StarAttribute::XATTR_FILLBMP_SIZELOG,"fill[bmp,sizeLog]", true);
  map[StarAttribute::XATTR_FILLFLOATTRANSPARENCE]=shared_ptr<StarAttribute>
      (new StarGAttributeNamedGradient(StarAttribute::XATTR_FILLFLOATTRANSPARENCE,"gradient[trans,fill]"));


  map[StarAttribute::ATTR_FRM_BOX]=shared_ptr<StarAttribute>(new StarGAttributeBorder(StarAttribute::ATTR_FRM_BOX,"box"));
  map[StarAttribute::ATTR_SC_BORDER]=shared_ptr<StarAttribute>(new StarGAttributeBorder(StarAttribute::ATTR_SC_BORDER,"scBorder"));
  map[StarAttribute::ATTR_CHR_BACKGROUND]=shared_ptr<StarAttribute>(new StarGAttributeBrush(StarAttribute::ATTR_CHR_BACKGROUND,"chrBackground"));
  map[StarAttribute::ATTR_FRM_BACKGROUND]=shared_ptr<StarAttribute>(new StarGAttributeBrush(StarAttribute::ATTR_FRM_BACKGROUND,"frmBackground"));
  map[StarAttribute::ATTR_SC_BACKGROUND]=shared_ptr<StarAttribute>(new StarGAttributeBrush(StarAttribute::ATTR_SC_BACKGROUND,"scBackground"));
  map[StarAttribute::ATTR_SCH_SYMBOL_BRUSH]=shared_ptr<StarAttribute>(new StarGAttributeBrush(StarAttribute::ATTR_SCH_SYMBOL_BRUSH,"symbold[brush]"));
  map[StarAttribute::ATTR_FRM_SHADOW]=shared_ptr<StarAttribute>(new StarGAttributeShadow(StarAttribute::ATTR_FRM_SHADOW,"shadow"));
  map[StarAttribute::ATTR_SC_SHADOW]=shared_ptr<StarAttribute>(new StarGAttributeShadow(StarAttribute::ATTR_SC_SHADOW,"shadow"));

  // TOUSE
  map[StarAttribute::ATTR_SC_BORDER_INNER]=shared_ptr<StarAttribute>(new StarGAttributeBoxInfo(StarAttribute::ATTR_SC_BORDER_INNER,"scBorder[inner]"));

}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
