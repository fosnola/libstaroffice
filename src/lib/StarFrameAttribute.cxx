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
class StarFAttributeBool final : public StarAttributeBool
{
public:
  //! constructor
  StarFAttributeBool(Type type, std::string const &debugName, bool value)
    : StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeBool(*this));
  }
  //! add to a frame style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;

protected:
  //! copy constructor
  StarFAttributeBool(StarFAttributeBool const &) = default;
};

//! a character color attribute
class StarFAttributeColor final : public StarAttributeColor
{
public:
  //! constructor
  StarFAttributeColor(Type type, std::string const &debugName, STOFFColor const &value)
    : StarAttributeColor(type, debugName, value)
  {
  }
  //! destructor
  ~StarFAttributeColor() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeColor(*this));
  }
  //! add to a frame style
  // void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarFAttributeColor(StarFAttributeColor const &) = default;
};

StarFAttributeColor::~StarFAttributeColor()
{
}
//! a character integer attribute
class StarFAttributeInt final : public StarAttributeInt
{
public:
  //! constructor
  StarFAttributeInt(Type type, std::string const &debugName, int intSize, int value)
    : StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a frame style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarFAttributeInt(StarFAttributeInt const &) = default;
};

//! a character unsigned integer attribute
class StarFAttributeUInt final : public StarAttributeUInt
{
public:
  //! constructor
  StarFAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value)
    : StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final
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
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeUInt(*this));
  }
  //! add to a frame style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarFAttributeUInt(StarFAttributeUInt const &) = default;
};

//! a void attribute
class StarFAttributeVoid final : public StarAttributeVoid
{
public:
  //! constructor
  StarFAttributeVoid(Type type, std::string const &debugName)
    : StarAttributeVoid(type, debugName)
  {
  }
  //! destructor
  ~StarFAttributeVoid() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeVoid(*this));
  }
  //! add to a frame style
  // void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarFAttributeVoid(StarFAttributeVoid const &) = default;
};

StarFAttributeVoid::~StarFAttributeVoid()
{
}

void StarFAttributeBool::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_LAYOUT_SPLIT)
    state.m_frame.m_propertyList.insert("style:may-break-between-rows", m_value);
}

void StarFAttributeInt::addTo(StarState &/*state*/, std::set<StarAttribute const *> &/*done*/) const
{
}

void StarFAttributeUInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_BREAK) {
    if (m_value>0 && m_value<=6) state.m_break=int(m_value);
    else if (m_value) {
      STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeUInt::addTo: unknown break value %d\n", int(m_value)));
    }
  }
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarFAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarFAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarFAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarFAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarFAttributeVoid(type,debugName));
}

}

namespace StarFrameAttribute
{
//! a anchor attribute
class StarFAttributeAnchor final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeAnchor(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_anchor(-1)
    , m_index(-1)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeAnchor(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a cell/graphic/paragraph style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_anchor>=0) o << "anchor=" << m_anchor << ",";
    if (m_index>=0) o << "index=" << m_index << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarFAttributeAnchor(StarFAttributeAnchor const &) = default;
  //! the anchor
  int m_anchor;
  //! the index (page?)
  int m_index;
};

//! a border attribute
class StarFAttributeBorder final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeBorder(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_distance(0)
  {
    for (int &distance : m_distances) distance=0;
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeBorder(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a cell/graphic/paragraph style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_distance) o << "dist=" << m_distance << ",";
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        o << "border" << i << "=[" << m_borders[i] << "],";
    }
    bool hasDistances=false;
    for (int distance : m_distances) {
      if (distance) {
        hasDistances=true;
        break;
      }
    }
    if (hasDistances) {
      o << "dists=[";
      for (int distance : m_distances)
        o << distance << ",";
      o << "],";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarFAttributeBorder(StarFAttributeBorder const &) = default;
  //! the distance
  int m_distance;
  //! the border list: top, left, right, bottom
  STOFFBorderLine m_borders[4];
  //! the padding distance: top, left, right, bottom
  int m_distances[4];
};

//! a brush attribute
class StarFAttributeBrush final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeBrush(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_brush()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeBrush(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a cell/font/graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[" << m_brush << "],";
  }

protected:
  //! copy constructor
  StarFAttributeBrush(StarFAttributeBrush const &) = default;
  //! the brush
  StarGraphicStruct::StarBrush m_brush;
};

//! a frameSize attribute
class StarFAttributeFrameSize final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeFrameSize(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_frmType(0)
    , m_width(0)
    , m_height(0)
    , m_percent(0,0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeFrameSize(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a page
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
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
  StarFAttributeFrameSize(StarFAttributeFrameSize const &) = default;
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
class StarFAttributeLineNumbering final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeLineNumbering(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_start(-1)
    , m_countLines(false)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeLineNumbering(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName;
    if (m_countLines)
      o << "=" << m_start << ",";
    else
      o << "*,";
  }
protected:
  //! copy constructor
  StarFAttributeLineNumbering(StarFAttributeLineNumbering const &) = default;
  //! the name value
  long m_start;
  //! the countLines flag
  bool m_countLines;
};

//! a left, right, ... attribute
class StarFAttributeLRSpace final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeLRSpace(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_textLeft(0)
    , m_autoFirst(false)
  {
    for (int &margin : m_margins) margin=0;
    for (int &propMargin : m_propMargins) propMargin=100;
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeLRSpace(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a paragraph/page
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    for (int i=0; i<3; ++i) {
      char const *wh[]= {"left", "right", "firstLine"};
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
  StarFAttributeLRSpace(StarFAttributeLRSpace const &) = default;
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
class StarFAttributeOrientation final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeOrientation(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_position(0)
    , m_orient(0)
    , m_relat(1)
    , m_posToggle(false)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeOrientation(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a cell style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
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
  StarFAttributeOrientation(StarFAttributeOrientation const &) = default;
  //! the position in twip
  int32_t m_position;
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
class StarFAttributeShadow final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeShadow(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_location(0)
    , m_width(0)
    , m_transparency(0)
    , m_color(STOFFColor::white())
    , m_fillColor(STOFFColor::white())
    , m_style(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeShadow(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a cell/graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
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
  StarFAttributeShadow(StarFAttributeShadow const &) = default;
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

//! a surround attribute
class StarFAttributeSurround final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeSurround(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_surround(2)
  {
    for (auto &b : m_bools) b=false;
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeSurround(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a cell/graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_surround) o << "surround=" << m_surround << ",";
    for (int i=0; i<4; ++i) {
      if (!m_bools[i]) continue;
      char const *wh[] = {"ideal", "anchor[only]", "contour", "outside"};
      o << wh[i] << ",";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarFAttributeSurround(StarFAttributeSurround const &) = default;
  //! the main value: NONE, THROUGH, PARALLEL, IDEAL, LEFT, RIGHT, END
  int m_surround;
  //! the ideal,anchorOnly, contour, outside
  bool m_bools[4];
};

//! a top, bottom, ... attribute
class StarFAttributeULSpace final : public StarAttribute
{
public:
  //! constructor
  StarFAttributeULSpace(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
  {
    for (int &margin : m_margins) margin=0;
    for (int &propMargin : m_propMargins) propMargin=100;
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarFAttributeULSpace(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a page/paragraph
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    for (int i=0; i<2; ++i) {
      char const *wh[]= {"top", "bottom" };
      if (m_propMargins[i]!=100)
        o << wh[i] << "=" << m_propMargins[i] << "%,";
      else if (m_margins[i])
        o << wh[i] << "=" << m_margins[i] << ",";
    }
    o << "],";
  }
protected:
  //! copy constructor
  StarFAttributeULSpace(StarFAttributeULSpace const &) = default;
  //! the margins: top, bottom
  int m_margins[2];
  //! the prop margins: top, bottom
  int m_propMargins[2];
};

void StarFAttributeAnchor::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  /* FLY_AT_PARA, FLY_AS_CHAR, FLY_AT_PAGE, FLY_AT_FLY, FLY_AT_CHAR
     RND_STD_HEADER, RND_STD_FOOTER, RND_STD_HEADERL, RND_STD_HEADERR,
     RND_STD_FOOTERL, RND_STD_FOOTERR,
     RND_DRAW_OBJECT   */
  STOFFPosition::AnchorTo const wh[]= {STOFFPosition::Paragraph, STOFFPosition::CharBaseLine,  STOFFPosition::Page, STOFFPosition::Frame, STOFFPosition::Char };
  if (m_anchor>=0 && m_anchor < int(STOFF_N_ELEMENTS(wh))) {
    state.m_frame.m_position.setAnchor(wh[m_anchor]);
    char const *defaultXRel[] = {"paragraph", "paragraph", "page", "frame", "paragraph" };
    char const *defaultYRel[] = {"paragraph", "baseline", "page", "frame", "paragraph" };
    char const *defaultYPos[] = {nullptr, "bottom", nullptr, nullptr, "from-top"};
    if (!state.m_frame.m_propertyList["style:horizontal-rel"] && defaultXRel[m_anchor])
      state.m_frame.m_propertyList.insert("style:horizontal-rel",  defaultXRel[m_anchor]);
    if (!state.m_frame.m_propertyList["style:vertical-rel"] && defaultYRel[m_anchor])
      state.m_frame.m_propertyList.insert("style:vertical-rel",  defaultYRel[m_anchor]);
    if (!state.m_frame.m_propertyList["style:vertical-pos"] && defaultYPos[m_anchor])
      state.m_frame.m_propertyList.insert("style:vertical-pos",  defaultYPos[m_anchor]);

    if (m_anchor==2) { // page
      if (m_index>=0)
        state.m_frame.m_propertyList.insert("text:anchor-page-number", m_index);
    }
    else if (m_anchor==4) // at char
      state.m_frame.m_anchorIndex = m_index;
  }
  else if (m_anchor>=0) {
    STOFF_DEBUG_MSG(("StarFrameAttributeInternal::StarFAttributeAnchor::addTo: unsure how to send anchor=%d\n", m_anchor));
  }
}

void StarFAttributeBorder::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  char const *wh[] = {"top", "left", "right", "bottom"};
  if (m_type==ATTR_FRM_BOX) {
    // checkme what is m_distance

    // graphic
    for (int i=0; i<4; ++i) {
      if (!m_borders[i].isEmpty())
        m_borders[i].addTo(state.m_graphic.m_propertyList, wh[i]);
    }
    for (int i=0; i<4; ++i)
      state.m_graphic.m_propertyList.insert((std::string("padding-")+wh[i]).c_str(), state.convertInPoint(m_distances[i]), librevenge::RVNG_POINT);

    // paragraph
    for (int i=0; i<4; ++i)
      m_borders[i].addTo(state.m_paragraph.m_propertyList, wh[i]);
    // SW table
    for (int i=0; i<4; ++i)
      m_borders[i].addTo(state.m_cell.m_propertyList, wh[i]);
  }
  else if (m_type==ATTR_SC_BORDER) {
    // checkme what is m_distance?
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
        STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBrush::addTo: can not set a font background\n"));
      }
    }
  }
  // graphic|para
  else if (m_type==ATTR_FRM_BACKGROUND) {
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
        STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBrush::addTo: can not set a cell background\n"));
        state.m_cell.m_propertyList.insert("fo:background-color", "transparent");
      }
    }
    // Frame
    if (!m_brush.isEmpty()) {
      STOFFColor color;
      if (m_brush.getColor(color))
        state.m_frame.m_propertyList.insert("fo:background-color", color.str().c_str());
    }
  }
  // cell
  else if (m_type == ATTR_SC_BACKGROUND) {
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
    STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBrush::addTo: can not set a cell background\n"));
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
      STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBrush::addTo: can not set a cell background\n"));
    }
#endif
  }
}

void StarFAttributeFrameSize::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_FRM_SIZE) {
    if (m_width>0) {
      state.m_frame.m_position.setSize(STOFFVec2f(float(m_width)*0.05f, state.m_frame.m_position.m_size[1]));
      state.m_global->m_page.m_propertiesList[0].insert("fo:page-width", double(state.m_frame.m_position.m_size[0]), librevenge::RVNG_POINT);
    }
    if (m_height>0) {
      state.m_frame.m_position.setSize(STOFFVec2f(state.m_frame.m_position.m_size[0], float(m_height)*0.05f));
      state.m_global->m_page.m_propertiesList[0].insert("fo:page-height", double(state.m_frame.m_position.m_size[1]), librevenge::RVNG_POINT);
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
  // frame
  if (m_type==ATTR_FRM_LR_SPACE) {
    if (m_propMargins[0]==100)
      state.m_frame.m_propertyList.insert("fo:margin-left", state.m_global->m_relativeUnit*double(m_textLeft), librevenge::RVNG_POINT);
    else
      state.m_frame.m_propertyList.insert("fo:margin-left", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      state.m_frame.m_propertyList.insert("fo:margin-right", state.m_global->m_relativeUnit*double(m_margins[1]), librevenge::RVNG_POINT);
    else
      state.m_frame.m_propertyList.insert("fo:margin-right", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
  // page
  if (m_type==ATTR_FRM_LR_SPACE && state.m_global->m_pageZone<=2) {
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
    // NONE,RIGHT,CENTER, LEFT,INSIDE,OUTSIDE,FULL, LEFT_AND_WIDTH
    char const *wh[]= {nullptr, "right", "center", "left", "inside", "outside", nullptr, "from-left"};
    if (m_orient>=0 && m_orient<=7 && wh[m_orient])
      state.m_frame.m_propertyList.insert("style:horizontal-pos", wh[m_orient]);

    switch (m_orient) {
    case 0: // default
      break;
    case 1:
    case 2:
    case 3: { // CHECKME
      char const *align[]= {"start", "center", "left"};
      state.m_frame.m_propertyList.insert("fo:text-align", align[m_orient-1]);
      break;
    }
    case 6:
      state.m_frame.m_propertyList.insert("fo:text-align", "justify");
      break;
    default: // TODO
      break;
    }
    // the relative position, FRAME, PRTAREA, CHAR, PG_LEFT, PG_RIGTH, FRM_LEFT, FRM_RIGHT, PG_FRAME, PG_PTRAREA
    char const *relat[]= { "frame", nullptr, "char", "page-start-margin", "page-left-margin", "frame-start-margin",
                           "frame-end-margin", "page", "page"
                         };
    if (m_relat>=0 && m_relat<=8 && relat[m_relat])
      state.m_frame.m_propertyList.insert("style:horizontal-rel", relat[m_relat]);
    if (m_position)
      state.m_frame.m_position.m_propertyList.insert("svg:x", double(m_position)*0.05, librevenge::RVNG_POINT);
  }
  else if (m_type==StarAttribute::ATTR_FRM_VERT_ORIENT) {
    // NONE,TOP,CENTER,BOTTOM,CHAR_TOP,CHAR_CENTER,CHAR_BOTTOM,LINE_TOP,LINE_CENTER,LINE_BOTTOM
    if (m_orient>0 && m_orient<=9) {
      switch (m_orient%3) {
      case 0: // BOTTOM, CHAR_BOTTOM, LINE_BOTTOM
        state.m_frame.m_propertyList.insert("style:vertical-pos", "bottom");
        break;
      case 1: // TOP, CHAR_TOP, LINE_TOP
        state.m_frame.m_propertyList.insert("style:vertical-pos", m_position ? "from-top" : "top");
        break;
      case 2: // CENTER, CHAR_CENTER, LINE_CENTER
        state.m_frame.m_propertyList.insert("style:vertical-pos", "middle");
        break;
      default: // nothing
        break;
      }
      switch ((m_orient-1)/3) {
      case 1:
        state.m_frame.m_propertyList.insert("style:vertical-rel", "char");
        break;
      case 2:
        state.m_frame.m_propertyList.insert("style:vertical-rel", "line");
        break;
      default:
        break;
      }
    }
    // the relative position, FRAME, PRTAREA, CHAR, PG_LEFT, PG_RIGTH, FRM_LEFT, FRM_RIGHT, PG_FRAME, PG_PTRAREA
    switch (m_relat) {
    case 0: // FRAME
    case 5: // FRM_LEFT
    case 6: // FRM_RIGHT
      state.m_frame.m_propertyList.insert("style:vertical-rel", "frame");
      break;
    case 2:
      state.m_frame.m_propertyList.insert("style:vertical-rel", "char");
      break;
    case 3: // PG_LEFT
    case 4: // PG_RIGH
    case 7: // PG_FRAME
    case 8: // PG_PTRAREA
      state.m_frame.m_propertyList.insert("style:vertical-rel", "page");
      break;
    default:
      break;
    }
    if (m_position)
      state.m_frame.m_position.m_propertyList.insert("svg:y", double(m_position)*0.05, librevenge::RVNG_POINT);
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
    state.m_graphic.m_propertyList.insert("draw:shadow-offset-x", ((m_location%2)?-1:1)*state.convertInPoint(m_width), librevenge::RVNG_POINT);
    state.m_graphic.m_propertyList.insert("draw:shadow-offset-y", (m_location<=2?-1:1)*state.convertInPoint(m_width), librevenge::RVNG_POINT);
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

void StarFAttributeSurround::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  int surround = m_surround; // NONE, THROUGH, PARALLEL, IDEAL, LEFT, RIGHT, END
  if (m_bools[0] && surround>1) surround=3;
  if (surround>=0 && surround<=5) {
    char const *wh[]= {"none", "run-through", "parallel", "dynamic", "left", "right"};
    state.m_frame.m_propertyList.insert("style:wrap", wh[surround]);
  }
  // checkme what is bAnchorOnly : m_bools[1]
  state.m_frame.m_propertyList.insert("style:wrap-countour", m_bools[2]);
  state.m_frame.m_propertyList.insert("style:wrap-contour-mode", m_bools[3] ? "outside" : "full");
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
  if (m_type==ATTR_FRM_UL_SPACE && state.m_global->m_pageZone<=2) {
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

bool StarFAttributeAnchor::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_anchor=int(input->readULong(1));
  bool ok=true;
  if (nVers<1)
    m_index=int(input->readULong(2));
  else {
    unsigned long nId;
    if (!input->readCompressedULong(nId)) {
      STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBorder::read: can not read nId\n"));
      f << "###nId,";
      ok=false;
    }
    else
      m_index=int(nId);
  }
  if (!ok) {
    if (input->tell()+8 > endPos) {
      STOFF_DEBUG_MSG(("StarFrameAttribute::StarFAttributeBorder::read: try to use endPos\n"));
      ok=true;
      input->seek(endPos, librevenge::RVNG_SEEK_SET);
    }
    else
      f << "###extra,";
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
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
      for (int &distance : m_distances) distance=int(input->readULong(2));
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
    for (int &i : dim) i=int(input->readULong(1));
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
    auto marker=long(input->readULong(4));
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
  // if (m_type==StarAttribute::ATTR_FRM_VERT_ORIENT && m_orient==0 && vers==0)
  // m_relat=0;
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

bool StarFAttributeSurround::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_surround = int(input->readULong(1));
  if (vers<5) // ideal
    *input >> m_bools[0];
  if (vers>1) // anchor only
    *input >> m_bools[1];
  if (vers>2) // continuous only
    *input >> m_bools[2];
  if (vers>3) // outside
    *input >> m_bools[2];
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
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
void addInitTo(std::map<int, std::shared_ptr<StarAttribute> > &map)
{
  addAttributeBool(map, StarAttribute::ATTR_FRM_LAYOUT_SPLIT,"layout[split]", true);
  addAttributeUInt(map, StarAttribute::ATTR_FRM_BREAK,"para[break]",1,0);

  map[StarAttribute::ATTR_FRM_LINENUMBER]=std::shared_ptr<StarAttribute>(new StarFAttributeLineNumbering(StarAttribute::ATTR_FRM_LINENUMBER,"lineNumbering"));
  map[StarAttribute::ATTR_FRM_LR_SPACE]=std::shared_ptr<StarAttribute>(new StarFAttributeLRSpace(StarAttribute::ATTR_FRM_LR_SPACE,"lrSpace"));
  map[StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE]=std::shared_ptr<StarAttribute>(new StarFAttributeLRSpace(StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE,"eeOutLrSpace"));
  map[StarAttribute::ATTR_FRM_UL_SPACE]=std::shared_ptr<StarAttribute>(new StarFAttributeULSpace(StarAttribute::ATTR_FRM_UL_SPACE,"ulSpace"));
  map[StarAttribute::ATTR_FRM_HORI_ORIENT]=std::shared_ptr<StarAttribute>(new StarFAttributeOrientation(StarAttribute::ATTR_FRM_HORI_ORIENT,"horiOrient"));
  map[StarAttribute::ATTR_FRM_VERT_ORIENT]=std::shared_ptr<StarAttribute>(new StarFAttributeOrientation(StarAttribute::ATTR_FRM_VERT_ORIENT,"vertOrient"));
  map[StarAttribute::ATTR_FRM_FRM_SIZE]=std::shared_ptr<StarAttribute>(new StarFAttributeFrameSize(StarAttribute::ATTR_FRM_FRM_SIZE,"frmSize"));
  map[StarAttribute::ATTR_FRM_ANCHOR]=std::shared_ptr<StarAttribute>(new StarFAttributeAnchor(StarAttribute::ATTR_FRM_ANCHOR,"frmAnchor"));
  map[StarAttribute::ATTR_FRM_BOX]=std::shared_ptr<StarAttribute>(new StarFAttributeBorder(StarAttribute::ATTR_FRM_BOX,"box"));
  map[StarAttribute::ATTR_SC_BORDER]=std::shared_ptr<StarAttribute>(new StarFAttributeBorder(StarAttribute::ATTR_SC_BORDER,"scBorder"));
  map[StarAttribute::ATTR_FRM_SHADOW]=std::shared_ptr<StarAttribute>(new StarFAttributeShadow(StarAttribute::ATTR_FRM_SHADOW,"shadow"));
  map[StarAttribute::ATTR_SC_SHADOW]=std::shared_ptr<StarAttribute>(new StarFAttributeShadow(StarAttribute::ATTR_SC_SHADOW,"shadow"));
  map[StarAttribute::ATTR_FRM_SURROUND]=std::shared_ptr<StarAttribute>(new StarFAttributeSurround(StarAttribute::ATTR_FRM_SURROUND,"surround"));
  map[StarAttribute::ATTR_CHR_BACKGROUND]=std::shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_CHR_BACKGROUND,"chrBackground"));
  map[StarAttribute::ATTR_FRM_BACKGROUND]=std::shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_FRM_BACKGROUND,"frmBackground"));
  map[StarAttribute::ATTR_SC_BACKGROUND]=std::shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_SC_BACKGROUND,"scBackground"));
  map[StarAttribute::ATTR_SCH_SYMBOL_BRUSH]=std::shared_ptr<StarAttribute>(new StarFAttributeBrush(StarAttribute::ATTR_SCH_SYMBOL_BRUSH,"symbold[brush]"));
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
