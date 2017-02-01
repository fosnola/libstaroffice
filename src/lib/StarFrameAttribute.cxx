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

#include "StarAttribute.hxx"
#include "StarGraphicStruct.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "StarFrameAttribute.hxx"

namespace StarFrameAttribute
{
//! a character bool attribute
class StarFAttributeBool : public StarAttributeBool
{
public:
  //! constructor
  StarFAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeBool(*this));
  }
  //! add to a frame style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;

protected:
  //! copy constructor
  StarFAttributeBool(StarFAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

//! a character color attribute
class StarFAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarFAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! destructor
  ~StarFAttributeColor();
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeColor(*this));
  }
  //! add to a frame style
  //virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
protected:
  //! copy constructor
  StarFAttributeColor(StarFAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

StarFAttributeColor::~StarFAttributeColor()
{
}
//! a character integer attribute
class StarFAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarFAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a frame style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarFAttributeInt(StarFAttributeInt const &orig) : StarAttributeInt(orig)
  {
  }
};

//! a character unsigned integer attribute
class StarFAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarFAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object)
  {
    if (m_type==ATTR_FRM_BREAK) {
      // SvxFmtBreakItem
      STOFFInputStreamPtr input=zone.input();
      long pos=input->tell();
      libstoff::DebugFile &ascFile=zone.ascii();
      libstoff::DebugStream f;
      m_value=static_cast<unsigned int>(input->readULong(1));
      f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
      if (vers==0) input->seek(1, librevenge::RVNG_SEEK_CUR); // dummy
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return input->tell()<=endPos;
    }
    return StarAttributeUInt::read(zone, vers, endPos, object);
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeUInt(*this));
  }
  //! add to a frame style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
protected:
  //! copy constructor
  StarFAttributeUInt(StarFAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

//! a void attribute
class StarFAttributeVoid : public StarAttributeVoid
{
public:
  //! constructor
  StarFAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! destructor
  ~StarFAttributeVoid();
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeVoid(*this));
  }
  //! add to a frame style
  //virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
protected:
  //! copy constructor
  StarFAttributeVoid(StarFAttributeVoid const &orig) : StarAttributeVoid(orig)
  {
  }
};

StarFAttributeVoid::~StarFAttributeVoid()
{
}

void StarFAttributeBool::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_LAYOUT_SPLIT)
    state.m_cell.m_propertyList.insert("style:may-break-between-rows", m_value);
}

void StarFAttributeInt::addTo(StarState &/*state*/, std::set<StarAttribute const *> &/*done*/) const
{
}

void StarFAttributeUInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_BREAK) {
    if (m_value>0 && m_value<=6) state.m_break=int(m_value);
    else if (m_value) {
      STOFF_DEBUG_MSG(("StarPAttributeUInt::addTo: unknown break value %d\n", int(m_value)));
    }
  }
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarFAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarFAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarFAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarFAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=shared_ptr<StarAttribute>(new StarFAttributeVoid(type,debugName));
}

}

namespace StarFrameAttribute
{
//! a border attribute
class StarFAttributeBorder : public StarAttribute
{
public:
  //! constructor
  StarFAttributeBorder(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_distance(0)
  {
    for (int i=0; i<4; ++i) m_distances[i]=0;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeBorder(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell/graphic/paragraph style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
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
  StarFAttributeBorder(StarFAttributeBorder const &orig) : StarAttribute(orig), m_distance(orig.m_distance)
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

//! a brush attribute
class StarFAttributeBrush : public StarAttribute
{
public:
  //! constructor
  StarFAttributeBrush(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_brush()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeBrush(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell/font/graphic style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[" << m_brush << "],";
  }

protected:
  //! copy constructor
  StarFAttributeBrush(StarFAttributeBrush const &orig) : StarAttribute(orig), m_brush(orig.m_brush)
  {
  }
  //! the brush
  StarGraphicStruct::StarBrush m_brush;
};

//! a frameSize attribute
class StarFAttributeFrameSize : public StarAttribute
{
public:
  //! constructor
  StarFAttributeFrameSize(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_frmType(0), m_width(0), m_height(0), m_percent(0,0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeFrameSize(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  // //! add to a graphic style
  // virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_frmType) // 0: var, 1:fixed, 2:min
      o << "type=" << m_frmType << ",";
    if (m_width)
      o << "width=" << m_width << ",";
    if (m_height)
      o << "height=" << m_height << ",";
    if (m_percent[0]>0 || m_percent[1]>0)
      o << "size[%]=" << m_percent << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarFAttributeFrameSize(StarFAttributeFrameSize const &orig) : StarAttribute(orig), m_frmType(orig.m_frmType), m_width(orig.m_width), m_height(orig.m_height), m_percent(orig.m_percent)
  {
  }
  //! the type
  int m_frmType;
  //! the width
  long m_width;
  //! the height
  long m_height;
  //! the percent value
  STOFFVec2i m_percent;
};

//! a line numbering attribute
class StarFAttributeLineNumbering : public StarAttribute
{
public:
  //! constructor
  StarFAttributeLineNumbering(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_start(-1), m_countLines(false)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeLineNumbering(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a para
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName;
    if (m_countLines)
      o << "=" << m_start << ",";
    else
      o << "*,";
  }
protected:
  //! copy constructor
  StarFAttributeLineNumbering(StarFAttributeLineNumbering const &orig) : StarAttribute(orig), m_start(orig.m_start), m_countLines(orig.m_countLines)
  {
  }
  //! the name value
  long m_start;
  //! the countLines flag
  bool m_countLines;
};

//! a left, right, ... attribute
class StarFAttributeLRSpace : public StarAttribute
{
public:
  //! constructor
  StarFAttributeLRSpace(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_textLeft(0), m_autoFirst(false)
  {
    for (int i=0; i<3; ++i) m_margins[i]=0;
    for (int i=0; i<3; ++i) m_propMargins[i]=100;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeLRSpace(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a paragraph/page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    for (int i=0; i<3; ++i) {
      char const *(wh[])= {"left", "right", "firstLine"};
      if (m_propMargins[i]!=100)
        o << wh[i] << "=" << m_propMargins[i] << "%,";
      else if (m_margins[i])
        o << wh[i] << "=" << m_margins[i] << ",";
    }
    if (m_textLeft) o << "text[left]=" << m_textLeft << ",";
    if (!m_autoFirst) o << "autoFirst=no,";
    o << "],";
  }
protected:
  //! copy constructor
  StarFAttributeLRSpace(StarFAttributeLRSpace const &orig) : StarAttribute(orig), m_textLeft(orig.m_textLeft), m_autoFirst(orig.m_autoFirst)
  {
    for (int i=0; i<3; ++i) m_margins[i]=orig.m_margins[i];
    for (int i=0; i<3; ++i) m_propMargins[i]=orig.m_propMargins[i];
  }
  //! the margins: left, right, firstline
  int m_margins[3];
  //! the prop margins: left, right, firstline
  int m_propMargins[3];
  //! the text left
  int m_textLeft;
  //! a bool
  bool m_autoFirst;
};

//! a orientation attribute
class StarFAttributeOrientation : public StarAttribute
{
public:
  //! constructor
  StarFAttributeOrientation(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_position(0), m_orient(0), m_relat(1), m_posToggle(false)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeOrientation(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_position) o << "pos=" << m_position << ",";
    if (m_orient) o << "orient=" << m_orient << ",";
    if (m_relat) o << "relat=" << m_relat << ","; // PRTAREA,
    if (m_posToggle) o << "posToggle,";
    o << "],";
  }

protected:
  //! copy constructor
  StarFAttributeOrientation(StarFAttributeOrientation const &orig) : StarAttribute(orig), m_position(orig.m_position), m_orient(orig.m_orient), m_relat(orig.m_relat), m_posToggle(orig.m_posToggle)
  {
  }
  //! the position in twip
  uint32_t m_position;
  /** the orientation:
      -horizontal:NONE,RIGHT,CENTER, LEFT,INSIDE,OUTSIDE,FULL, LEFT_AND_WIDTH
      -vertical:NONE,TOP,CENTER,BOTTOM,CHAR_TOP,CHAR_CENTER,CHAR_BOTTOM,LINE_TOP,LINE_CENTER,LINE_BOTTOM
   */
  int m_orient;
  //! the relative position, FRAME, PRTAREA, CHAR, PG_LEFT, PG_RIGTH, FRM_LEFT, FRM_RIGHT, PG_FRAME, PG_PTRAREA
  int m_relat;
  //! a posToggle flag
  bool m_posToggle;
};

//! a shadow attribute
class StarFAttributeShadow : public StarAttribute
{
public:
  //! constructor
  StarFAttributeShadow(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_location(0), m_width(0), m_transparency(0), m_color(STOFFColor::white()),
    m_fillColor(STOFFColor::white()), m_style(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeShadow(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell/graphic style
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
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
  StarFAttributeShadow(StarFAttributeShadow const &orig) :
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

//! a top, bottom, ... attribute
class StarFAttributeULSpace : public StarAttribute
{
public:
  //! constructor
  StarFAttributeULSpace(Type type, std::string const &debugName) : StarAttribute(type, debugName)
  {
    for (int i=0; i<2; ++i) m_margins[i]=0;
    for (int i=0; i<2; ++i) m_propMargins[i]=100;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarFAttributeULSpace(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a page/paragraph
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    for (int i=0; i<2; ++i) {
      char const *(wh[])= {"top", "bottom" };
      if (m_propMargins[i]!=100)
        o << wh[i] << "=" << m_propMargins[i] << "%,";
      else if (m_margins[i])
        o << wh[i] << "=" << m_margins[i] << ",";
    }
    o << "],";
  }
protected:
  //! copy constructor
  StarFAttributeULSpace(StarFAttributeULSpace const &orig) : StarAttribute(orig)
  {
    for (int i=0; i<2; ++i) m_margins[i]=orig.m_margins[i];
    for (int i=0; i<2; ++i) m_propMargins[i]=orig.m_propMargins[i];
  }
  //! the margins: top, bottom
  int m_margins[2];
  //! the prop margins: top, bottom
  int m_propMargins[2];
};

void StarFAttributeBorder::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_BOX) {
    // checkme what is m_distance

    // graphic
    char const * (wh[])= {"top", "left", "right", "bottom"};
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        m_borders[i].addTo(state.m_graphic.m_propertyList, wh[i]);
    }
    for (int i=0; i<4; ++i)
      state.m_graphic.m_propertyList.insert((std::string("padding-")+wh[i]).c_str(), libstoff::convertMiniMToPoint(m_distances[i]), librevenge::RVNG_POINT);

    // paragraph
    for (int i=0; i<4; ++i)
      m_borders[i].addTo(state.m_paragraph.m_propertyList, wh[i]);
    // SW table
    for (int i=0; i<4; ++i)
      m_borders[i].addTo(state.m_cell.m_propertyList, wh[i]);
  }
  else if (m_type==ATTR_SC_BORDER) {
    // checkme what is m_distance?
    char const * (wh[])= {"top", "left", "right", "bottom"};
    for (int i=0; i<4; ++i)
      m_borders[i].addTo(state.m_cell.m_propertyList, wh[i]);
  }
}

void StarFAttributeBrush::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  // font
  if (m_type == ATTR_CHR_BACKGROUND) {
    if (m_brush.isEmpty())
      state.m_font.m_propertyList.insert("fo:background-color", "transparent");
    else {
      STOFFColor color;
      if (m_brush.getColor(color))
        state.m_font.m_propertyList.insert("fo:background-color", color.str().c_str());
      else {
        state.m_font.m_propertyList.insert("fo:background-color", "transparent");
        STOFF_DEBUG_MSG(("StarFAttributeBrush::addTo: can not set a font background\n"));
      }
    }
  }
  // graphic|para
  if (m_type==ATTR_FRM_BACKGROUND) {
    if (m_brush.m_transparency>0 && m_brush.m_transparency<=255)
      state.m_graphic.m_propertyList.insert("draw:opacity", 1.-double(m_brush.m_transparency)/255., librevenge::RVNG_PERCENT);
    else
      state.m_graphic.m_propertyList.insert("draw:opacity",1., librevenge::RVNG_PERCENT);
    state.m_paragraph.m_propertyList.insert("fo:background-color", "transparent");
    // graphic
    if (m_brush.isEmpty())
      state.m_graphic.m_propertyList.insert("draw:fill", "none");
    else {
      STOFFColor color;
      if (m_brush.hasUniqueColor() && m_brush.getColor(color)) {
        state.m_graphic.m_propertyList.insert("draw:fill", "solid");
        state.m_graphic.m_propertyList.insert("draw:fill-color", color.str().c_str());

        // para
        state.m_paragraph.m_propertyList.insert("fo:background-color", color.str().c_str());
      }
      else {
        STOFFEmbeddedObject object;
        STOFFVec2i size;
        if (m_brush.getPattern(object, size) && object.m_dataList.size()) {
          state.m_graphic.m_propertyList.insert("draw:fill", "bitmap");
          state.m_graphic.m_propertyList.insert("draw:fill-image", object.m_dataList[0].getBase64Data());
          state.m_graphic.m_propertyList.insert("draw:fill-image-width", size[0], librevenge::RVNG_POINT);
          state.m_graphic.m_propertyList.insert("draw:fill-image-height", size[1], librevenge::RVNG_POINT);
          state.m_graphic.m_propertyList.insert("draw:fill-image-ref-point-x",0, librevenge::RVNG_POINT);
          state.m_graphic.m_propertyList.insert("draw:fill-image-ref-point-y",0, librevenge::RVNG_POINT);
          state.m_graphic.m_propertyList.insert("librevenge:mime-type", object.m_typeList.empty() ? "image/pict" : object.m_typeList[0].c_str());
        }
        else
          state.m_graphic.m_propertyList.insert("draw:fill", "none");
      }
    }
    // SW table
    if (m_brush.isEmpty())
      state.m_cell.m_propertyList.insert("fo:background-color", "transparent");
    else {
      STOFFColor color;
      if (m_brush.getColor(color))
        state.m_cell.m_propertyList.insert("fo:background-color", color.str().c_str());
      else {
        STOFF_DEBUG_MSG(("StarFAttributeBrush::addTo: can not set a cell background\n"));
        state.m_cell.m_propertyList.insert("fo:background-color", "transparent");
      }
    }
  }
  // cell
  if (m_type == ATTR_SC_BACKGROUND) {
    if (m_brush.isEmpty()) {
      state.m_cell.m_propertyList.insert("fo:background-color", "transparent");
      return;
    }

    STOFFColor color;
#if 1
    if (m_brush.getColor(color)) {
      state.m_cell.m_propertyList.insert("fo:background-color", color.str().c_str());
      return;
    }
    STOFF_DEBUG_MSG(("StarFAttributeBrush::addTo: can not set a cell background\n"));
    state.m_cell.m_propertyList.insert("fo:background-color", "transparent");
#else
    /* checkme, is it possible to use style:background-image here ?
       Can not create any working ods file with bitmap cell's background...
    */
    if (m_brush.hasUniqueColor() && m_brush.getColor(color)) {
      state.m_cell.m_propertyList.insert("fo:background-color", color.str().c_str());
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
      state.m_cell.m_propertyList.insert("librevenge:background-image", backgroundVector);
    }
    else {
      STOFF_DEBUG_MSG(("StarFAttributeBrush::addTo: can not set a cell background\n"));
    }
#endif
  }
}

void StarFAttributeFrameSize::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_FRM_SIZE) {
    if (m_width>0) {
      state.m_frameSize[0]=float(m_width)*0.05f;
      state.m_global->m_page.m_propertiesList[0].insert("fo:page-width", double(state.m_frameSize[0]), librevenge::RVNG_POINT);
    }
    if (m_height>0) {
      state.m_frameSize[1]=float(m_height)*0.05f;
      state.m_global->m_page.m_propertiesList[0].insert("fo:page-height", double(state.m_frameSize[1]), librevenge::RVNG_POINT);
    }
  }
}

void StarFAttributeLineNumbering::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_LINENUMBER) {
    if (m_start>=0 && m_countLines) {
      state.m_paragraph.m_propertyList.insert("text:number-lines", true);
      state.m_paragraph.m_propertyList.insert("text:line-number", m_start==0 ? 1 : int(m_start));
    }
  }
}

void StarFAttributeLRSpace::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  // paragraph
  if (m_type==ATTR_FRM_LR_SPACE || m_type==ATTR_EE_PARA_OUTLLR_SPACE) {
    if (m_propMargins[0]==100) // unsure if/when we need to use textLeft/margins[0] here
      state.m_paragraph.m_propertyList.insert("fo:margin-left", state.m_global->m_relativeUnit*double(m_textLeft), librevenge::RVNG_POINT);
    else
      state.m_paragraph.m_propertyList.insert("fo:margin-left", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      state.m_paragraph.m_propertyList.insert("fo:margin-right", state.m_global->m_relativeUnit*double(m_margins[1]), librevenge::RVNG_POINT);
    else
      state.m_paragraph.m_propertyList.insert("fo:margin-right", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
    if (m_propMargins[2]==100)
      state.m_paragraph.m_propertyList.insert("fo:text-indent", state.m_global->m_relativeUnit*double(m_margins[2]), librevenge::RVNG_POINT);
    else
      state.m_paragraph.m_propertyList.insert("fo:text-indent", double(m_propMargins[2])/100., librevenge::RVNG_PERCENT);
    // m_textLeft: ok to ignore ?
    state.m_paragraph.m_propertyList.insert("style:auto-text-indent", m_autoFirst);
  }
  // page
  if (m_type==ATTR_FRM_LR_SPACE && state.m_global->m_pageZone>=0 && state.m_global->m_pageZone<=2) {
    if (m_propMargins[0]==100)
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-left", double(m_margins[0])*0.05, librevenge::RVNG_POINT);
    else
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-left", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-right", double(m_margins[1])*0.05, librevenge::RVNG_POINT);
    else
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-right", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
}

void StarFAttributeOrientation::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==StarAttribute::ATTR_FRM_HORI_ORIENT) {
    switch (m_orient) {
    case 0: // default
      break;
    case 1:
    case 2:
    case 3: { // CHECKME
      char const *(wh[])= {"start", "center", "left"};
      state.m_cell.m_propertyList.insert("fo:text-align", wh[m_orient-1]);
      break;
    }
    default: // TODO
      break;
    }
  }
  else if (m_type==StarAttribute::ATTR_FRM_VERT_ORIENT) {
    switch (m_orient) {
    case 0: // default
      break;
    case 1:
    case 2:
    case 3: {
      char const *(wh[])= {"top", "middle", "bottom"};
      state.m_cell.m_propertyList.insert("style:vertical-align", wh[m_orient-1]);
      break;
    }
    default: // TODO
      break;
    }
  }
}

void StarFAttributeShadow::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  // graphic
  if (m_width<=0 || m_location<=0 || m_location>4 || m_transparency<0 || m_transparency>=255)
    state.m_graphic.m_propertyList.insert("draw:shadow", "hidden");
  else {
    state.m_graphic.m_propertyList.insert("draw:shadow", "visible");
    state.m_graphic.m_propertyList.insert("draw:shadow-color", m_color.str().c_str());
    state.m_graphic.m_propertyList.insert("draw:shadow-opacity", 1.-double(m_transparency)/255., librevenge::RVNG_PERCENT);
    state.m_graphic.m_propertyList.insert("draw:shadow-offset-x", ((m_location%2)?-1:1)*libstoff::convertMiniMToPoint(m_width), librevenge::RVNG_POINT);
    state.m_graphic.m_propertyList.insert("draw:shadow-offset-y", (m_location<=2?-1:1)*libstoff::convertMiniMToPoint(m_width), librevenge::RVNG_POINT);
  }
  // cell
  if (m_width<=0 || m_location<=0 || m_location>4 || m_transparency<0 || m_transparency>=100)
    state.m_cell.m_propertyList.insert("style:shadow", "none");
  else {
    std::stringstream s;
    s << m_color.str().c_str() << " "
      << ((m_location%2)?-1:1)*double(m_width)/20. << "pt "
      << (m_location<=2?-1:1)*double(m_width)/20. << "pt";
    state.m_cell.m_propertyList.insert("style:shadow", s.str().c_str());
  }
}

void StarFAttributeULSpace::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  // paragraph
  if (m_type==ATTR_FRM_UL_SPACE) {
    if (m_propMargins[0]==100)
      state.m_paragraph.m_propertyList.insert("fo:margin-top", state.m_global->m_relativeUnit*double(m_margins[0]), librevenge::RVNG_POINT);
    else
      state.m_paragraph.m_propertyList.insert("fo:margin-top", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      state.m_paragraph.m_propertyList.insert("fo:margin-bottom", state.m_global->m_relativeUnit*double(m_margins[1]), librevenge::RVNG_POINT);
    else
      state.m_paragraph.m_propertyList.insert("fo:margin-bottom", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
  // page
  if (m_type==ATTR_FRM_UL_SPACE && state.m_global->m_pageZone>=0 && state.m_global->m_pageZone<=2) {
    if (m_propMargins[0]==100)
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-top", double(m_margins[0])*0.05, librevenge::RVNG_POINT);
    else
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-top", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-bottom", double(m_margins[1])*0.05, librevenge::RVNG_POINT);
    else
      state.m_global->m_page.m_propertiesList[state.m_global->m_pageZone].insert("fo:margin-bottom", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
}

bool StarFAttributeBorder::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_distance=int(input->readULong(2));
  int cLine=0;
  bool ok=true;
  while (input->tell()<endPos) {
    cLine=int(input->readULong(1));
    if (cLine>3) break;
    STOFFBorderLine border;
    if (!input->readColor(border.m_color)) {
      STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBorder::read: can not find a box's color\n"));
      f << "###color,";
      ok=false;
      break;
    }
    border.m_outWidth=int(input->readULong(2));
    border.m_inWidth=int(input->readULong(2));
    border.m_distance=int(input->readULong(2));
    m_borders[cLine]=border;
  }
  if (ok && nVers>=1 && cLine&0x10) {
    if (input->tell()+8 > endPos) {
      STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBorder::read: can not find the box's borders\n"));
      f << "###distances,";
    }
    else {
      for (int i=0; i<4; ++i) m_distances[i]=int(input->readULong(2));
    }
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarFAttributeBrush::read(StarZone &zone, int nVers, long endPos, StarObject &object)
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
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarFAttributeFrameSize::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // sw_sw3attr.cxx SwFmtFrmSize::Create
  m_frmType=int(input->readULong(1));
  m_width=int(input->readLong(4));
  m_height=int(input->readLong(4));
  if (nVers>1) {
    int dim[2];
    for (int i=0; i<2; ++i) dim[i]=int(input->readULong(1));
    m_percent=STOFFVec2i(dim[0],dim[1]);
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarFAttributeLineNumbering::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // sw_sw3attr.cxx SwFmtLineNumber::Create
  m_start=long(input->readULong(4));
  *input >> m_countLines;

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarFAttributeLRSpace::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // svx_frmitems.cxx: SvxLRSpaceItem::Create
  for (int i=0; i<3; ++i) {
    if (i<2)
      m_margins[i]=int(input->readULong(2));
    else
      m_margins[i]=int(input->readLong(2));
    m_propMargins[i]=int(input->readULong(vers>=1 ? 2 : 1));
  }
  if (vers>=2)
    m_textLeft=int(input->readLong(2));
  uint8_t autofirst=0;
  if (vers>=3) {
    *input >> autofirst;
    m_autoFirst=(autofirst&1);
    long marker=long(input->readULong(4));
    if (marker==0x599401FE) {
      m_margins[2]=int(input->readLong(2));
      if (m_margins[2]<0)
        m_margins[0]+=m_margins[2];
    }
    else
      input->seek(-4, librevenge::RVNG_SEEK_CUR);
  }
  if (vers>=4 && (autofirst&0x80)) { // negative margin
    int32_t nMargin;
    *input >> nMargin;
    m_margins[0]=nMargin;
    *input >> nMargin;
    m_margins[1]=nMargin;
  }
  //m_textLeft=(m_margins[2]>=0) ? m_margins[0] : m_margins[0]-m_margins[2];
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarFAttributeOrientation::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  // sw_sw3attr.cxx SwFmtVertOrient::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  *input >> m_position;
  m_orient=int(input->readULong(1));
  m_relat=int(input->readULong(1));
  if (m_type==StarAttribute::ATTR_FRM_HORI_ORIENT && vers>=1)
    *input>>m_posToggle;

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarFAttributeShadow::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  bool ok=true;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_location=int(input->readULong(1));
  m_width=int(input->readULong(2));
  m_transparency=int(input->readULong(1));
  if (!input->readColor(m_color)) {
    STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeShadow::read: can not find a fill color\n"));
    f << "###color,";
    ok=false;
  }
  if (ok && !input->readColor(m_fillColor)) {
    STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeShadow::read: can not find a fill color\n"));
    f << "###fillcolor,";
    ok=false;
  }
  if (ok) m_style=int(input->readULong(1));

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarFAttributeULSpace::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  for (int i=0; i<2; ++i) {
    m_margins[i]=int(input->readULong(2));
    m_propMargins[i]=int(input->readULong(vers>=1 ? 2 : 1));
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}
}

namespace StarFrameAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  addAttributeBool(map, StarAttribute::ATTR_FRM_LAYOUT_SPLIT,"layout[split]", true);
  addAttributeUInt(map, StarAttribute::ATTR_FRM_BREAK,"para[break]",1,0);

  map[StarAttribute::ATTR_FRM_LINENUMBER]=shared_ptr<StarAttribute>(new StarFAttributeLineNumbering(StarAttribute::ATTR_FRM_LINENUMBER,"lineNumbering"));
  map[StarAttribute::ATTR_FRM_LR_SPACE]=shared_ptr<StarAttribute>(new StarFAttributeLRSpace(StarAttribute::ATTR_FRM_LR_SPACE,"lrSpace"));
  map[StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE]=shared_ptr<StarAttribute>(new StarFAttributeLRSpace(StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE,"eeOutLrSpace"));
  map[StarAttribute::ATTR_FRM_UL_SPACE]=shared_ptr<StarAttribute>(new StarFAttributeULSpace(StarAttribute::ATTR_FRM_UL_SPACE,"ulSpace"));
  map[StarAttribute::ATTR_FRM_HORI_ORIENT]=shared_ptr<StarAttribute>(new StarFAttributeOrientation(StarAttribute::ATTR_FRM_HORI_ORIENT,"horiOrient"));
  map[StarAttribute::ATTR_FRM_VERT_ORIENT]=shared_ptr<StarAttribute>(new StarFAttributeOrientation(StarAttribute::ATTR_FRM_VERT_ORIENT,"vertOrient"));
  map[StarAttribute::ATTR_FRM_FRM_SIZE]=shared_ptr<StarAttribute>(new StarFAttributeFrameSize(StarAttribute::ATTR_FRM_FRM_SIZE,"frmSize"));
  map[StarAttribute::ATTR_FRM_BOX]=shared_ptr<StarAttribute>(new StarFAttributeBorder(StarAttribute::ATTR_FRM_BOX,"box"));
  map[StarAttribute::ATTR_SC_BORDER]=shared_ptr<StarAttribute>(new StarFAttributeBorder(StarAttribute::ATTR_SC_BORDER,"scBorder"));
  map[StarAttribute::ATTR_FRM_SHADOW]=shared_ptr<StarAttribute>(new StarFAttributeShadow(StarAttribute::ATTR_FRM_SHADOW,"shadow"));
  map[StarAttribute::ATTR_SC_SHADOW]=shared_ptr<StarAttribute>(new StarFAttributeShadow(StarAttribute::ATTR_SC_SHADOW,"shadow"));
  map[StarAttribute::ATTR_CHR_BACKGROUND]=shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_CHR_BACKGROUND,"chrBackground"));
  map[StarAttribute::ATTR_FRM_BACKGROUND]=shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_FRM_BACKGROUND,"frmBackground"));
  map[StarAttribute::ATTR_SC_BACKGROUND]=shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_SC_BACKGROUND,"scBackground"));
  map[StarAttribute::ATTR_SCH_SYMBOL_BRUSH]=shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_SCH_SYMBOL_BRUSH,"symbold[brush]"));
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
