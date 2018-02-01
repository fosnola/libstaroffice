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

#include "STOFFParagraph.hxx"
#include "STOFFPageSpan.hxx"

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarFormatManager.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectNumericRuler.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "StarParagraphAttribute.hxx"

namespace StarParagraphAttribute
{
//! a character bool attribute
class StarPAttributeBool final : public StarAttributeBool
{
public:
  //! constructor
  StarPAttributeBool(Type type, std::string const &debugName, bool value)
    : StarAttributeBool(type, debugName, value)
  {
  }
  //! destructor
  ~StarPAttributeBool() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeBool(*this));
  }
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;

protected:
  //! copy constructor
  StarPAttributeBool(StarPAttributeBool const &) = default;
};

StarPAttributeBool::~StarPAttributeBool()
{
}

//! a character color attribute
class StarPAttributeColor final : public StarAttributeColor
{
public:
  //! constructor
  StarPAttributeColor(Type type, std::string const &debugName, STOFFColor const &value)
    : StarAttributeColor(type, debugName, value)
  {
  }
  //! destructor
  ~StarPAttributeColor() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeColor(*this));
  }
  //! add to a para
  // void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarPAttributeColor(StarPAttributeColor const &) = default;
};

StarPAttributeColor::~StarPAttributeColor()
{
}
//! a character integer attribute
class StarPAttributeInt final : public StarAttributeInt
{
public:
  //! constructor
  StarPAttributeInt(Type type, std::string const &debugName, int intSize, int value)
    : StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! destructor
  ~StarPAttributeInt() final;
  //! add to a para
  // void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarPAttributeInt(StarPAttributeInt const &) = default;
};

StarPAttributeInt::~StarPAttributeInt()
{
}

//! a character unsigned integer attribute
class StarPAttributeUInt final : public StarAttributeUInt
{
public:
  //! constructor
  StarPAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value)
    : StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! destructor
  ~StarPAttributeUInt() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeUInt(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final
  {
    STOFFInputStreamPtr input=zone.input();
    long pos=input->tell();
    if (pos+2==endPos && m_intSize==1 && (m_type==ATTR_PARA_WIDOWS || m_type==ATTR_PARA_ORPHANS)) {
      // unsure, sometimes, I found an extra byte
      libstoff::DebugFile &ascFile=zone.ascii();
      libstoff::DebugStream f;
      m_value=static_cast<unsigned int>(input->readULong(1));
      f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
      auto tmp=int(input->readULong(1));
      if (tmp) f << "#unkn=" << tmp << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return input->tell()<=endPos;
    }
    return StarAttributeUInt::read(zone, vers, endPos, object);
  }
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarPAttributeUInt(StarPAttributeUInt const &) = default;
};

StarPAttributeUInt::~StarPAttributeUInt()
{
}
//! a void attribute
class StarPAttributeVoid final : public StarAttributeVoid
{
public:
  //! constructor
  StarPAttributeVoid(Type type, std::string const &debugName)
    : StarAttributeVoid(type, debugName)
  {
  }
  //! destructor
  ~StarPAttributeVoid() final;
  //! add to a para
  // void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeVoid(*this));
  }
protected:
  //! copy constructor
  StarPAttributeVoid(StarPAttributeVoid const &) = default;
};

StarPAttributeVoid::~StarPAttributeVoid()
{
}

void StarPAttributeBool::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_SPLIT)
    state.m_paragraph.m_propertyList.insert("fo:keep-together", m_value ? "auto" : "always");
  else if (m_type==ATTR_PARA_HANGINGPUNCTUATION)
    state.m_paragraph.m_propertyList.insert("style:punctuation-wrap", m_value ? "hanging" : "simple");
  else if (m_type==ATTR_PARA_SNAPTOGRID)
    state.m_paragraph.m_propertyList.insert("style:snap-to-layout-grid", m_value);
  else if (m_type==ATTR_PARA_CONNECT_BORDER)
    state.m_paragraph.m_propertyList.insert("style:join-border", m_value);
  else if (m_type==ATTR_EE_PARA_ASIANCJKSPACING)
    state.m_paragraph.m_propertyList.insert("style:font-independent-line-spacing", !m_value);
}

void StarPAttributeUInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_ORPHANS)
    state.m_paragraph.m_propertyList.insert("fo:orphans", int(m_value));
  else if (m_type==ATTR_PARA_WIDOWS)
    state.m_paragraph.m_propertyList.insert("fo:widows", int(m_value));
  else if (m_type==ATTR_PARA_VERTALIGN) {
    if (m_value<=4) {
      char const *wh[]= {"auto", "baseline", "top", "middle", "bottom"};
      state.m_paragraph.m_propertyList.insert("style:vertical-align", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarPAttributeUInt::addTo: unknown vertical align %d\n", int(m_value)));
    }
  }
  else if (m_type==ATTR_EE_PARA_BULLETSTATE)
    state.m_paragraph.m_bulletVisible=m_value!=0;
  else if (m_type==ATTR_EE_PARA_OUTLLEVEL)
    state.m_paragraph.m_listLevelIndex=int(m_value);
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarPAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarPAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarPAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarPAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarPAttributeVoid(type,debugName));
}

}

namespace StarParagraphAttribute
{
// ------------------------------------------------------------
//! a adjust attribute
class StarPAttributeAdjust final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeAdjust(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_adjust(0)
    , m_flags(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeAdjust(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_adjust) o << "adjust=" << m_adjust << ",";
    if (m_flags) o << "flags=" << std::hex << m_flags << std::dec << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeAdjust(StarPAttributeAdjust const &) = default;
  //! the adjust value
  int m_adjust;
  //! the flags
  int m_flags;
};

// ------------------------------------------------------------
//! a numeric bullet attribute
class StarPAttributeBulletNumeric final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeBulletNumeric(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_numType(0)
    , m_numLevels(0)
    , m_flags(0)
    , m_continuous(true)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeBulletNumeric(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_numType) o << "type=" << m_numType << ",";
    if (m_numLevels) o << "numLevels=" << m_numLevels << ",";
    if (m_flags) o << "flags=" << std::hex << m_flags << std::dec << ",";
    if (!m_continuous) o << "continuous*,";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeBulletNumeric(StarPAttributeBulletNumeric const &) = default;
  //! the type
  int m_numType;
  //! the numLevels
  int m_numLevels;
  //! the flags
  int m_flags;
  //! a continuous flag
  bool m_continuous;
};

// ------------------------------------------------------------
//! a simple bullet attribute
class StarPAttributeBulletSimple final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeBulletSimple(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_level()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeBulletSimple(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName;
  }
protected:
  //! copy constructor
  StarPAttributeBulletSimple(StarPAttributeBulletSimple const &) = default;
  //! the level
  STOFFListLevel m_level;
};

//! a drop attribute
class StarPAttributeDrop final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeDrop(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_numFormats(0)
    , m_numLines(0)
    , m_numChars(0)
    , m_numDistances(0)
    , m_whole(false)
    , m_numX(0)
    , m_numY(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeDrop(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_numFormats!=0xFFFF) o << "num[formats]=" << m_numFormats << ",";
    if (m_numLines) o << "num[lines]=" << m_numLines << ",";
    if (m_numChars) o << "num[chars]=" << m_numChars << ",";
    if (m_numDistances) o << "num[distances]=" << m_numDistances << ",";
    if (m_whole) o << "whole,";
    if (m_numX) o << "numX=" << m_numX << ",";
    if (m_numY) o << "numY=" << m_numY << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeDrop(StarPAttributeDrop const &) = default;
  //! the number of format
  int m_numFormats;
  //! the number of lines
  int m_numLines;
  //! the number of chars
  int m_numChars;
  //! the number of distances
  int m_numDistances;
  //! flag to know if whole
  bool m_whole;
  //! the number of x
  int m_numX;
  //! the number of y
  int m_numY;
};

//! a hyphen attribute
class StarPAttributeHyphen final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeHyphen(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_hyphenZone(0)
    , m_pageEnd(0)
    , m_minLead(0)
    , m_minTail(0)
    , m_maxHyphen(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeHyphen(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_hyphenZone) o << "hyphenZone=" << m_hyphenZone << ",";
    if (m_pageEnd) o << "pageEnd,";
    if (m_minLead) o << "lead[min]=" << m_minLead << ",";
    if (m_minTail) o << "tail[min]=" << m_minTail << ",";
    if (m_maxHyphen) o << "hyphen[max]=" << m_maxHyphen << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeHyphen(StarPAttributeHyphen const &) = default;
  //! the hyphen value
  int m_hyphenZone;
  //! the page end flag
  bool m_pageEnd;
  //! the min lead
  int m_minLead;
  //! the min tail
  int m_minTail;
  //! the max hyphen
  int m_maxHyphen;
};

//! a line spacing attribute
class StarPAttributeLineSpacing final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeLineSpacing(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_propLineSpace(100)
    , m_interLineSpace(0)
    , m_lineHeight(200)
    , m_lineSpaceRule(0)
    , m_interLineSpaceRule(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeLineSpacing(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_interLineSpace) o << "lineSpace=" << m_interLineSpace;
    if (m_propLineSpace!=100) o << "propLineSpace=" << m_propLineSpace << ",";
    if (m_lineHeight) o << "height=" << m_lineHeight << ",";
    if (m_lineSpaceRule) o << "lineSpaceRule=" << m_lineSpaceRule << ",";
    if (m_interLineSpaceRule) o << "interLineSpaceRule=" << m_interLineSpaceRule << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeLineSpacing(StarPAttributeLineSpacing const &) = default;
  //! the prop lineSpacing
  int m_propLineSpace;
  //! the line spacing
  int m_interLineSpace;
  //! the height
  int m_lineHeight;
  //! the line spacing rule: SvxLineSpace
  int m_lineSpaceRule;
  //! the inter line spacing rule: SvxInterLineSpace
  int m_interLineSpaceRule;
};

//! a numRule attribute
class StarPAttributeNumericRuler final : public StarAttribute
{
public:
  //! constructor
  StarPAttributeNumericRuler(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_name("")
    , m_poolId(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeNumericRuler(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (!m_name.empty()) o << m_name.cstr() << ",";
    if (m_poolId) o << "poolId=" << m_poolId << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeNumericRuler(StarPAttributeNumericRuler const &) = default;
  //! the name value
  librevenge::RVNGString m_name;
  //! the poolId
  int m_poolId;
};

//! a tabStop attribute
class StarPAttributeTabStop final : public StarAttribute
{
public:
  //! a tabs structure
  struct TabStop {
    //! constructor
    TabStop()
      : m_pos(0)
      , m_type(0)
      , m_decimal(44)
      , m_fill(32)
    {
    }
    //! debug function to print the data
    void printData(libstoff::DebugStream &o) const
    {
      o << "pos=" << m_pos;
      if (m_type>=0 && m_type<5) {
        char const *wh[]= {"L", "R", "D", "C", "Def"};
        o << wh[m_type];
      }
      else
        o << "[##type=" << m_type << "]";
      if (m_decimal!=44) o << "[decimal=" << m_decimal << "]";
      if (m_fill!=32) o << "[fill=" << m_fill << "]";
      o << ",";
    }
    //! the position
    int m_pos;
    //! the type: SvxTabAdjust: left, right, decimal, center, default
    int m_type;
    //! the decimal char
    int m_decimal;
    //! the fill char
    int m_fill;
  };
  //! constructor
  StarPAttributeTabStop(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_tabList()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarPAttributeTabStop(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a para
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName << "=[";
    for (auto const &t : m_tabList)
      t.printData(o);
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeTabStop(StarPAttributeTabStop const &) = default;
  //! the tabStop list
  std::vector<TabStop> m_tabList;
};

void StarPAttributeAdjust::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_ADJUST) {
    switch (m_adjust) {
    case 0:
      state.m_paragraph.m_propertyList.insert("fo:text-align", "left");
      break;
    case 1:
      state.m_paragraph.m_propertyList.insert("fo:text-align", "right");
      break;
    case 2: // block
      state.m_paragraph.m_propertyList.insert("fo:text-align", "justify");
      state.m_paragraph.m_propertyList.insert("fo:text-align-last", "default");
      break;
    case 3:
      state.m_paragraph.m_propertyList.insert("fo:text-align", "center");
      break;
    case 4: // blockline
      state.m_paragraph.m_propertyList.insert("fo:text-align", "justify");
      break;
    case 5:
      state.m_paragraph.m_propertyList.insert("fo:text-align", "end");
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeAdjust::addTo: unknown adjust %d\n", int(m_adjust)));
    }
  }
}

void StarPAttributeBulletNumeric::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_EE_PARA_NUMBULLET) {
    STOFFListLevel level;
    if (m_numType<=4) {
      char const *wh[]= {"A", "a", "I", "i", "1"};
      level.m_propertyList.insert("style:num-format", wh[m_numType]);
      level.m_type=STOFFListLevel::NUMBER;
    }
    else {
      STOFF_DEBUG_MSG(("StarPAttributeBulletNumeric::addTo: unknown type=%d\n", m_numType));
      level.m_type=STOFFListLevel::BULLET;
      librevenge::RVNGString bullet;
      libstoff::appendUnicode(0x2022, bullet); // checkme
      level.m_propertyList.insert("text:bullet-char", bullet);
    }
    state.m_paragraph.m_listLevel=level;
  }
}

void StarPAttributeBulletSimple::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_EE_PARA_BULLET)
    state.m_paragraph.m_listLevel=m_level;
}

void StarPAttributeDrop::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_DROP) {
    librevenge::RVNGPropertyList cap;
    cap.insert("style:distance", state.m_global->m_relativeUnit*double(m_numDistances), librevenge::RVNG_POINT);
    cap.insert("style:length", m_numChars);
    cap.insert("style:lines", m_numLines);
    librevenge::RVNGPropertyListVector capVector;
    capVector.append(cap);
    state.m_paragraph.m_propertyList.insert("style:drop-cap", capVector);
  }
}

void StarPAttributeHyphen::addTo(StarState &/*state*/, std::set<StarAttribute const *> &/*done*/) const
{
}

void StarPAttributeLineSpacing::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_LINESPACING) {
    // svx_paraitem.cxx SvxLineSpacingItem::QueryValue
    if (m_interLineSpaceRule==0)
      state.m_paragraph.m_propertyList.insert("fo:line-height", "normal");
    switch (m_lineSpaceRule) {
    case 0: // will be set later
      break;
    case 1:
      state.m_paragraph.m_propertyList.insert("fo:line-height", state.m_global->m_relativeUnit*double(m_lineHeight), librevenge::RVNG_POINT);
      return;
    case 2:
      state.m_paragraph.m_propertyList.insert("fo:line-height-at-least", state.m_global->m_relativeUnit*double(m_lineHeight), librevenge::RVNG_POINT);
      return;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeLineSpacing::addTo: unknown rule %d\n", int(m_lineSpaceRule)));
    }
    switch (m_interLineSpaceRule) {
    case 0: // off
      state.m_paragraph.m_propertyList.insert("fo:line-height", 1., librevenge::RVNG_PERCENT);
      break;
    case 1: // Prop
      state.m_paragraph.m_propertyList.insert("fo:line-height", double(m_propLineSpace)/100., librevenge::RVNG_PERCENT);
      break;
    case 2: // Fix
      state.m_paragraph.m_propertyList.insert("fo:line-height", state.m_global->m_relativeUnit*double(m_interLineSpace), librevenge::RVNG_POINT);
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeLineSpacing::addTo: unknown inter linse spacing rule %d\n", int(m_interLineSpaceRule)));
    }
  }
}

void StarPAttributeNumericRuler::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_name.empty() || !state.m_global->m_numericRuler)
    return;
  state.m_global->m_list=state.m_global->m_numericRuler->getList(m_name);
}

void StarPAttributeTabStop::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  librevenge::RVNGPropertyListVector tabs;
  for (auto const &tabStop : m_tabList) {
    librevenge::RVNGPropertyList tab;
    switch (tabStop.m_type) {
    case 0:
      tab.insert("style:type", "left");
      break;
    case 1:
      tab.insert("style:type", "right");
      break;
    case 2:
      tab.insert("style:type", "center");
      break;
    case 3:
      tab.insert("style:type", "char");
      if (tabStop.m_decimal) {
        librevenge::RVNGString sDecimal;
        libstoff::appendUnicode(uint32_t(tabStop.m_decimal), sDecimal);
        tab.insert("style:char", sDecimal);
      }
      break;
    case 4: // default: checkme: can we set "style:tab-stop-distance" here
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeTabStop::addTo: unknown attrib %d\n", int(tabStop.m_type)));
    }
    if (tabStop.m_fill) {
      librevenge::RVNGString sFill;
      libstoff::appendUnicode(uint32_t(tabStop.m_fill), sFill);
      tab.insert("style:leader-text", sFill);
      tab.insert("style:leader-style", "solid");
    }
    tab.insert("style:position", state.m_global->m_relativeUnit*double(tabStop.m_pos), librevenge::RVNG_POINT);
    tabs.append(tab);
  }
  state.m_paragraph.m_propertyList.insert("style:tab-stops", tabs);
}

bool StarPAttributeAdjust::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_adjust=int(input->readULong(1));
  if (vers>=1) m_flags=int(input->readULong(1));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeBulletNumeric::read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // svx_numitem.cxx SvxNumRule::SvxNumRule
  uint16_t version;
  *input >> version;
  m_numLevels=int(input->readULong(2));
  m_flags=int(input->readULong(2));
  m_continuous=(input->readULong(2)!=0);
  m_numType=int(input->readULong(2));
  f << "set=[";
  for (int i=0; i<10; ++i) {
    uint16_t nSet;
    *input>>nSet;
    if (nSet) {
      f << nSet << ",";
      if (!object.getFormatManager()->readNumberFormat(zone, endPos, object) || input->tell()>endPos) {
        f << "###";
        break;
      }
    }
    else
      f << "_,";
  }
  f << "],";
  if (version>=2)
    m_flags=int(input->readULong(2));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeBulletSimple::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  // svx_bulitem.cxx SvxBulletItem::SvxBulletItem
  return StarObjectNumericRuler::readAttributeLevel(zone, vers, endPos, m_level);
}

bool StarPAttributeDrop::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_numFormats=int(input->readULong(2));
  m_numLines=int(input->readULong(2));
  m_numChars=int(input->readULong(2));
  m_numDistances=int(input->readULong(2));
  if (vers>=1)
    *input >> m_whole;
  else {
    m_numX=int(input->readULong(2));
    m_numY=int(input->readULong(2));
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeHyphen::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_hyphenZone=int(input->readLong(1));
  *input >> m_pageEnd;
  m_minLead=int(input->readLong(1));
  m_minTail=int(input->readLong(1));
  m_maxHyphen=int(input->readLong(1));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeLineSpacing::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_propLineSpace=int(input->readULong(1));
  m_interLineSpace=int(input->readLong(2));
  m_lineHeight=int(input->readULong(2));
  m_lineSpaceRule=int(input->readULong(1));
  m_interLineSpaceRule=int(input->readULong(1));

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeNumericRuler::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // sw_sw3attr.cxx SwNumRuleItem::Create
  std::vector<uint32_t> string;
  if (!zone.readString(string) || input->tell()>endPos) {
    STOFF_DEBUG_MSG(("StarPAttributeNumericRuler::read: can not find the sTmp\n"));
    f << "###sTmp,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_name=libstoff::getString(string);
  if (vers>0)
    // 3<<11+1<<10+(num1,num2,...,num5,bul1,...,bul5)
    m_poolId=int(input->readULong(2));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeTabStop::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  auto N=int(input->readULong(1));
  if (input->tell()+7*N>endPos) {
    STOFF_DEBUG_MSG(("StarPAttributeTabStop::read: N is too big\n"));
    f << "###N=" << N << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_tabList.resize(size_t(N));
  for (size_t i=0; i<size_t(N); ++i) {
    StarPAttributeTabStop::TabStop &tab=m_tabList[i];
    tab.m_pos=int(input->readLong(4));
    tab.m_type=int(input->readULong(1));
    tab.m_decimal=int(input->readULong(1));
    tab.m_fill=int(input->readULong(1));
  }
  f << "],";
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

}

namespace StarParagraphAttribute
{
void addInitTo(std::map<int, std::shared_ptr<StarAttribute> > &map)
{
  addAttributeBool(map,StarAttribute::ATTR_PARA_SPLIT,"para[split]",true);
  addAttributeUInt(map,StarAttribute::ATTR_PARA_WIDOWS,"para[widows]",1,0); // numlines
  addAttributeUInt(map,StarAttribute::ATTR_PARA_ORPHANS,"para[orphans]",1,0); // numlines
  addAttributeBool(map,StarAttribute::ATTR_PARA_HANGINGPUNCTUATION,"para[hangingPunctuation]",true);
  addAttributeUInt(map,StarAttribute::ATTR_PARA_VERTALIGN,"para[vert,align]",2,0);
  addAttributeBool(map,StarAttribute::ATTR_PARA_SNAPTOGRID,"para[snapToGrid]",true);
  addAttributeBool(map,StarAttribute::ATTR_PARA_CONNECT_BORDER,"para[connectBorder]",true);
  addAttributeUInt(map, StarAttribute::ATTR_EE_PARA_BULLETSTATE,"para[bullet,state]",2,0);
  addAttributeUInt(map, StarAttribute::ATTR_EE_PARA_OUTLLEVEL,"para[outlevel]",2,0);
  addAttributeBool(map, StarAttribute::ATTR_EE_PARA_ASIANCJKSPACING,"para[asianCJKSpacing]",false);

  map[StarAttribute::ATTR_PARA_ADJUST]=std::shared_ptr<StarAttribute>(new StarPAttributeAdjust(StarAttribute::ATTR_PARA_ADJUST,"parAtrAdjust"));
  map[StarAttribute::ATTR_PARA_LINESPACING]=std::shared_ptr<StarAttribute>(new StarPAttributeLineSpacing(StarAttribute::ATTR_PARA_LINESPACING,"parAtrLinespacing"));
  map[StarAttribute::ATTR_PARA_TABSTOP]=std::shared_ptr<StarAttribute>(new StarPAttributeTabStop(StarAttribute::ATTR_PARA_TABSTOP,"parAtrTabStop"));
  map[StarAttribute::ATTR_EE_PARA_NUMBULLET]=std::shared_ptr<StarAttribute>(new StarPAttributeBulletNumeric(StarAttribute::ATTR_EE_PARA_NUMBULLET,"eeParaNumBullet"));
  map[StarAttribute::ATTR_EE_PARA_BULLET]=std::shared_ptr<StarAttribute>(new StarPAttributeBulletSimple(StarAttribute::ATTR_EE_PARA_BULLET,"paraBullet"));
  map[StarAttribute::ATTR_PARA_DROP]=std::shared_ptr<StarAttribute>(new StarPAttributeDrop(StarAttribute::ATTR_PARA_DROP,"parAtrDrop"));

  // seems safe to ignore
  addAttributeBool(map,StarAttribute::ATTR_PARA_SCRIPTSPACE,"para[scriptSpace]",false); // script type, use by ui
  addAttributeVoid(map, StarAttribute::ATTR_EE_PARA_XMLATTRIBS, "para[xmlAttrib]");
  std::stringstream s;
  for (int type=StarAttribute::ATTR_PARA_DUMMY5; type<=StarAttribute::ATTR_PARA_DUMMY8; ++type) {
    s.str("");
    s << "paraDummy" << type-StarAttribute::ATTR_PARA_DUMMY5+5;
    addAttributeBool(map,StarAttribute::Type(type), s.str(), false);
  }

  // can we retrieve the following attribute ?
  addAttributeBool(map,StarAttribute::ATTR_PARA_REGISTER,"para[register]",false);
  addAttributeBool(map,StarAttribute::ATTR_PARA_FORBIDDEN_RULES,"para[forbiddenRules]",true); // If the forbidden characters rules are to be applied or not.
  map[StarAttribute::ATTR_PARA_HYPHENZONE]=std::shared_ptr<StarAttribute>(new StarPAttributeHyphen(StarAttribute::ATTR_PARA_HYPHENZONE,"parAtrHyphenZone"));
  map[StarAttribute::ATTR_PARA_NUMRULE]=std::shared_ptr<StarAttribute>(new StarPAttributeNumericRuler(StarAttribute::ATTR_PARA_NUMRULE,"parAtrNumRule"));

  // TODO
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
