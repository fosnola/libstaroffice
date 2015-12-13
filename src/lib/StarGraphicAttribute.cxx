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

#include "STOFFCellStyle.hxx"
#include "STOFFGraphicStyle.hxx"

#include "StarAttribute.hxx"
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
  //virtual void addTo(STOFFGraphicStyle &graphic) const;

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
  //virtual void addTo(STOFFGraphicStyle &graphic) const;
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
  // virtual void addTo(STOFFGraphicStyle &graphic) const;
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
  //virtual void addTo(STOFFGraphicStyle &graphic) const;
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
  //virtual void addTo(STOFFGraphicStyle &graphic) const;
protected:
  //! copy constructor
  StarGAttributeVoid(StarGAttributeVoid const &orig) : StarAttributeVoid(orig)
  {
  }
};

//! add a bool attribute
void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeBool(type,debugName, defValue));
}
//! add a color attribute
void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeColor(type,debugName, defValue));
}
//! add a int attribute
void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarGAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
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
  virtual void addTo(STOFFCellStyle &cell) const;
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic) const;
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
  virtual void addTo(STOFFCellStyle &cell) const;
  //! add to a graphic style
  virtual void addTo(STOFFGraphicStyle &graphic) const;
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

void StarGAttributeBorder::addTo(STOFFCellStyle &cell) const
{
  if (m_type==ATTR_SC_BORDER) {
    // checkme what is m_distance?
    char const * (wh[])= {"top", "left", "right", "bottom"};
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        m_borders[i].addTo(cell.m_propertyList, wh[i]);
    }
  }
}

void StarGAttributeBorder::addTo(STOFFGraphicStyle &graphic) const
{
  if (m_type==ATTR_FRM_BOX) {
    // checkme what is m_distance?
    char const * (wh[])= {"top", "left", "right", "bottom"};
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        m_borders[i].addTo(graphic.m_propertyList, wh[i]);
    }
    for (int i=0; i<4; ++i) {
      if (m_distances[i]==0)
        continue;
      graphic.m_propertyList.insert((std::string("padding-")+wh[i]).c_str(), float(m_distances[i])/20.f, librevenge::RVNG_POINT);
    }
  }
}

void StarGAttributeShadow::addTo(STOFFCellStyle &cell) const
{
  if (m_width<=0 || m_location<=0 || m_location>4 || m_transparency<0 || m_transparency>=100) return;
  std::stringstream s;
  s << m_color.str().c_str() << " "
    << ((m_location%2)?-1:1)*double(m_width)/20. << "pt "
    << (m_location<=2?-1:1)*double(m_width)/20. << "pt";
  cell.m_propertyList.insert("style:shadow", s.str().c_str());
}

void StarGAttributeShadow::addTo(STOFFGraphicStyle &graphic) const
{
  if (m_width<=0 || m_location<=0 || m_location>4 || m_transparency<0 || m_transparency>=100) return;
  graphic.m_propertyList.insert("draw:shadow", "visible");
  graphic.m_propertyList.insert("draw:shadow-color", m_color.str().c_str());
  graphic.m_propertyList.insert("draw:shadow-opacity", 1.f-float(m_transparency)/100.f, librevenge::RVNG_PERCENT);
  graphic.m_propertyList.insert("draw:shadow-offset-x", ((m_location%2)?-1:1)*double(m_width)/20., librevenge::RVNG_POINT);
  graphic.m_propertyList.insert("draw:shadow-offset-y", (m_location<=2?-1:1)*double(m_width)/20., librevenge::RVNG_POINT);
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


}

namespace StarGraphicAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  map[StarAttribute::ATTR_FRM_BOX]=shared_ptr<StarAttribute>(new StarGAttributeBorder(StarAttribute::ATTR_FRM_BOX,"box"));
  map[StarAttribute::ATTR_SC_BORDER]=shared_ptr<StarAttribute>(new StarGAttributeBorder(StarAttribute::ATTR_SC_BORDER,"scBorder"));
  map[StarAttribute::ATTR_FRM_SHADOW]=shared_ptr<StarAttribute>(new StarGAttributeShadow(StarAttribute::ATTR_FRM_SHADOW,"shadow"));
  map[StarAttribute::ATTR_SC_SHADOW]=shared_ptr<StarAttribute>(new StarGAttributeShadow(StarAttribute::ATTR_SC_SHADOW,"shadow"));
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
