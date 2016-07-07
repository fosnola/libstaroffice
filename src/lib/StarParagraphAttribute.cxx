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
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarZone.hxx"

#include "StarParagraphAttribute.hxx"

namespace StarParagraphAttribute
{
//! a character bool attribute
class StarPAttributeBool : public StarAttributeBool
{
public:
  //! constructor
  StarPAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! destructor
  ~StarPAttributeBool();
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeBool(*this));
  }
  //! add to a para
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;

protected:
  //! copy constructor
  StarPAttributeBool(StarPAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

StarPAttributeBool::~StarPAttributeBool()
{
}

//! a character color attribute
class StarPAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarPAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! destructor
  ~StarPAttributeColor();
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeColor(*this));
  }
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
protected:
  //! copy constructor
  StarPAttributeColor(StarPAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

StarPAttributeColor::~StarPAttributeColor()
{
}
//! a character integer attribute
class StarPAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarPAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! destructor
  ~StarPAttributeInt();
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarPAttributeInt(StarPAttributeInt const &orig) : StarAttributeInt(orig)
  {
  }
};

StarPAttributeInt::~StarPAttributeInt()
{
}

//! a character unsigned integer attribute
class StarPAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarPAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! destructor
  ~StarPAttributeUInt();
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeUInt(*this));
  }
  //! add to a para
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
protected:
  //! copy constructor
  StarPAttributeUInt(StarPAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

StarPAttributeUInt::~StarPAttributeUInt()
{
}
//! a void attribute
class StarPAttributeVoid : public StarAttributeVoid
{
public:
  //! constructor
  StarPAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! destructor
  ~StarPAttributeVoid();
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeVoid(*this));
  }
protected:
  //! copy constructor
  StarPAttributeVoid(StarPAttributeVoid const &orig) : StarAttributeVoid(orig)
  {
  }
};

StarPAttributeVoid::~StarPAttributeVoid()
{
}

void StarPAttributeBool::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_SPLIT)
    para.m_propertyList.insert("fo:keep-together", m_value ? "auto" : "always");
  else if (m_type==ATTR_PARA_HANGINGPUNCTUATION)
    para.m_propertyList.insert("fo:style:punctuation-wrap", m_value ? "hanging" : "simple");
  else if (m_type==ATTR_PARA_SNAPTOGRID)
    para.m_propertyList.insert("style:snap-to-layout-grid", m_value);
  else if (m_type==ATTR_PARA_CONNECT_BORDER)
    para.m_propertyList.insert("style:join-border", m_value);
  else if (m_type==ATTR_EE_PARA_ASIANCJKSPACING)
    para.m_propertyList.insert("style:font-independent-line-spacing", !m_value);
}

void StarPAttributeUInt::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_ORPHANS)
    para.m_propertyList.insert("fo:orphans", int(m_value));
  else if (m_type==ATTR_PARA_WIDOWS)
    para.m_propertyList.insert("fo:widows", int(m_value));
  else if (m_type==ATTR_PARA_VERTALIGN) {
    if (m_value<=4) {
      char const *(wh[])= {"auto", "baseline", "top", "middle", "bottom"};
      para.m_propertyList.insert("style:vertical-align", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarPAttributeUInt::addTo: unknown vertical align %d\n", int(m_value)));
    }
  }
  else if (m_type==ATTR_EE_PARA_BULLETSTATE)
    para.m_bulletVisible=m_value!=0;
  else if (m_type==ATTR_EE_PARA_OUTLLEVEL)
    para.m_propertyList.insert("text:outline-level", int(m_value));
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeVoid(type,debugName));
}

}

namespace StarParagraphAttribute
{
// ------------------------------------------------------------
//! a adjust attribute
class StarPAttributeAdjust : public StarAttribute
{
public:
  //! constructor
  StarPAttributeAdjust(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_adjust(0), m_flags(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeAdjust(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_adjust) o << "adjust=" << m_adjust << ",";
    if (m_flags) o << "flags=" << std::hex << m_flags << std::dec << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeAdjust(StarPAttributeAdjust const &orig) : StarAttribute(orig), m_adjust(orig.m_adjust), m_flags(orig.m_flags)
  {
  }
  //! the adjust value
  int m_adjust;
  //! the flags
  int m_flags;
};

//! a drop attribute
class StarPAttributeDrop : public StarAttribute
{
public:
  //! constructor
  StarPAttributeDrop(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_numFormats(0), m_numLines(0), m_numChars(0), m_numDistances(0), m_whole(false), m_numX(0), m_numY(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeDrop(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_numFormats!=0xFFFF) o << "num[formats]=" << m_numFormats << ",";
    if (m_numLines) o << "num[lines]=" << m_numLines << ",";
    if (m_numChars) o << "num[chars]=" << m_numChars << ",";
    if (m_numDistances) o << "num[distances]=" << m_numChars << ",";
    if (m_whole) o << "whole,";
    if (m_numX) o << "numX=" << m_numX << ",";
    if (m_numY) o << "numY=" << m_numY << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeDrop(StarPAttributeDrop const &orig) : StarAttribute(orig), m_numFormats(orig.m_numFormats), m_numLines(orig.m_numLines), m_numChars(orig.m_numChars), m_numDistances(orig.m_numDistances), m_whole(orig.m_whole), m_numX(orig.m_numX), m_numY(orig.m_numY)
  {
  }
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
class StarPAttributeHyphen : public StarAttribute
{
public:
  //! constructor
  StarPAttributeHyphen(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_hyphenZone(0), m_pageEnd(0), m_minLead(0), m_minTail(0), m_maxHyphen(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeHyphen(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
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
  StarPAttributeHyphen(StarPAttributeHyphen const &orig) : StarAttribute(orig), m_hyphenZone(orig.m_hyphenZone), m_pageEnd(orig.m_pageEnd), m_minLead(orig.m_minLead), m_minTail(orig.m_minTail), m_maxHyphen(orig.m_maxHyphen)
  {
  }
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
class StarPAttributeLineSpacing : public StarAttribute
{
public:
  //! constructor
  StarPAttributeLineSpacing(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_propLineSpace(100), m_lineSpace(0), m_height(200), m_lineSpaceRule(0), m_interLineSpaceRule(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeLineSpacing(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_lineSpace) o << "lineSpace=" << m_lineSpace << "[" << m_propLineSpace << "],";
    if (m_height) o << "height=" << m_height << ",";
    if (m_lineSpaceRule) o << "lineSpaceRule=" << m_lineSpaceRule << ",";
    if (m_interLineSpaceRule) o << "interLineSpaceRule=" << m_interLineSpaceRule << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeLineSpacing(StarPAttributeLineSpacing const &orig) : StarAttribute(orig), m_propLineSpace(orig.m_propLineSpace), m_lineSpace(orig.m_lineSpace), m_height(orig.m_height), m_lineSpaceRule(orig.m_lineSpaceRule), m_interLineSpaceRule(orig.m_interLineSpaceRule)
  {
  }
  //! the prop lineSpacing
  int m_propLineSpace;
  //! the line spacing
  int m_lineSpace;
  //! the height
  int m_height;
  //! the line spacing rule: SvxLineSpace
  int m_lineSpaceRule;
  //! the inter line spacing rule: SvxInterLineSpace
  int m_interLineSpaceRule;
};

//! a left, right, ... attribute
class StarPAttributeLRSpace : public StarAttribute
{
public:
  //! constructor
  StarPAttributeLRSpace(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_textLeft(0), m_autoFirst(false)
  {
    for (int i=0; i<3; ++i) m_margins[i]=0;
    for (int i=0; i<3; ++i) m_propMargins[i]=100;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeLRSpace(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a paragraph
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! add to a page
  virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
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
  StarPAttributeLRSpace(StarPAttributeLRSpace const &orig) : StarAttribute(orig), m_textLeft(orig.m_textLeft), m_autoFirst(orig.m_autoFirst)
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

//! a numRule attribute
class StarPAttributeNumRule : public StarAttribute
{
public:
  //! constructor
  StarPAttributeNumRule(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_name(""), m_poolId(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeNumRule(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (!m_name.empty()) o << m_name.cstr() << ",";
    if (m_poolId) o << "poolId=" << m_poolId << ",";
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeNumRule(StarPAttributeNumRule const &orig) : StarAttribute(orig), m_name(orig.m_name), m_poolId(orig.m_poolId)
  {
  }
  //! the name value
  librevenge::RVNGString m_name;
  //! the poolId
  int m_poolId;
};

//! a tabStop attribute
class StarPAttributeTabStop : public StarAttribute
{
public:
  //! a tabs structure
  struct TabStop {
    //! constructor
    TabStop() : m_pos(0), m_type(0), m_decimal(44), m_fill(32)
    {
    }
    //! debug function to print the data
    void printData(libstoff::DebugStream &o) const
    {
      o << "pos=" << m_pos;
      if (m_type>=0 && m_type<5) {
        char const *(wh[])= {"L", "R", "D", "C", "Def"};
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
  StarPAttributeTabStop(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_tabList()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeTabStop(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    for (size_t t=0; t<m_tabList.size(); ++t)
      m_tabList[t].printData(o);
    o << "],";
  }
protected:
  //! copy constructor
  StarPAttributeTabStop(StarPAttributeTabStop const &orig) : StarAttribute(orig), m_tabList(orig.m_tabList)
  {
  }
  //! the tabStop list
  std::vector<TabStop> m_tabList;
};

//! a top, bottom, ... attribute
class StarPAttributeULSpace : public StarAttribute
{
public:
  //! constructor
  StarPAttributeULSpace(Type type, std::string const &debugName) : StarAttribute(type, debugName)
  {
    for (int i=0; i<2; ++i) m_margins[i]=0;
    for (int i=0; i<2; ++i) m_propMargins[i]=100;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeULSpace(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  // ! add to a paragraph
  virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
  //! add to a page
  virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const;
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
  StarPAttributeULSpace(StarPAttributeULSpace const &orig) : StarAttribute(orig)
  {
    for (int i=0; i<2; ++i) m_margins[i]=orig.m_margins[i];
    for (int i=0; i<2; ++i) m_propMargins[i]=orig.m_propMargins[i];
  }
  //! the margins: top, bottom
  int m_margins[2];
  //! the prop margins: top, bottom
  int m_propMargins[2];
};

void StarPAttributeAdjust::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_ADJUST) {
    switch (m_adjust) {
    case 0:
      para.m_propertyList.insert("fo:text-align", "left");
      break;
    case 1:
      para.m_propertyList.insert("fo:text-align", "right");
      break;
    case 2: // block
      para.m_propertyList.insert("fo:text-align", "justify");
      para.m_propertyList.insert("fo:text-align-last", "justify");
      break;
    case 3:
      para.m_propertyList.insert("fo:text-align", "center");
      break;
    case 4: // blockline
      para.m_propertyList.insert("fo:text-align", "justify");
      break;
    case 5:
      para.m_propertyList.insert("fo:text-align", "end");
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeAdjust::addTo: unknown adjust %d\n", int(m_adjust)));
    }
  }
}

void StarPAttributeDrop::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_DROP) {
    librevenge::RVNGPropertyList cap;
    cap.insert("style:distance", para.m_relativeUnit*double(m_numDistances), librevenge::RVNG_POINT);
    cap.insert("style:length", m_numChars);
    cap.insert("style:lines", m_numLines);
    librevenge::RVNGPropertyListVector capVector;
    capVector.append(cap);
    para.m_propertyList.insert("style:drop-cap", capVector);
  }
}

void StarPAttributeHyphen::addTo(STOFFParagraph &/*para*/, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
}

void StarPAttributeLineSpacing::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_PARA_LINESPACING) {
    if (m_interLineSpaceRule==0)
      para.m_propertyList.insert("fo:line-height", "normal");
    switch (m_lineSpaceRule) {
    case 0:
      break;
    case 1:
      para.m_propertyList.insert("fo:line-height", para.m_relativeUnit*double(m_height), librevenge::RVNG_POINT);
      break;
    case 2:
      para.m_propertyList.insert("fo:line-height-at-least", para.m_relativeUnit*double(m_height), librevenge::RVNG_POINT);
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeLineSpacing::addTo: unknown rule %d\n", int(m_lineSpaceRule)));
    }
    switch (m_interLineSpaceRule) {
    case 0: // auto
      break;
    case 1: // Prop
      if (m_lineSpace)
        para.m_propertyList.insert("style:line-spacing", double(m_lineSpace<50 ? 50 : m_lineSpace)/100., librevenge::RVNG_PERCENT);
      break;
    case 2: // Fix
      para.m_propertyList.insert("style:line-spacing", para.m_relativeUnit*double(m_lineSpace), librevenge::RVNG_POINT);
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttributeLineSpacing::addTo: unknown inter linse spacing rule %d\n", int(m_interLineSpaceRule)));
    }
  }
}

void StarPAttributeLRSpace::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_LR_SPACE || m_type==ATTR_EE_PARA_OUTLLR_SPACE) {
    if (m_propMargins[0]==100)
      para.m_propertyList.insert("fo:margin-left", para.m_relativeUnit*double(m_margins[0]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-left", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      para.m_propertyList.insert("fo:margin-right", para.m_relativeUnit*double(m_margins[1]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-right", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
    if (m_propMargins[2]==100)
      para.m_propertyList.insert("fo:text-indent", para.m_relativeUnit*double(m_margins[2]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:text-indent", double(m_propMargins[2])/100., librevenge::RVNG_PERCENT);
    // m_textLeft: ok to ignore ?
    para.m_propertyList.insert("style:auto-text-indent", m_autoFirst);
  }
}

void StarPAttributeLRSpace::addTo(STOFFPageSpan &page, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type!=ATTR_FRM_LR_SPACE) return;
  if (page.m_actualZone>=0 && page.m_actualZone<=2) {
    if (m_propMargins[0]==100)
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-left", libstoff::convertMiniMToPoint(m_margins[0]), librevenge::RVNG_POINT);
    else
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-left", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-right", libstoff::convertMiniMToPoint(m_margins[1]), librevenge::RVNG_POINT);
    else
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-right", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
}

void StarPAttributeNumRule::addTo(STOFFParagraph &/*para*/, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
}

void StarPAttributeTabStop::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  librevenge::RVNGPropertyListVector tabs;
  for (size_t t=0; t<m_tabList.size(); ++t) {
    StarPAttributeTabStop::TabStop const &tabStop=m_tabList[t];
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
    case 4: // default
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
    tab.insert("style:position", para.m_relativeUnit*double(tabStop.m_pos), librevenge::RVNG_POINT);
    tabs.append(tab);
  }
  para.m_propertyList.insert("style:tab-stops", tabs);
}

void StarPAttributeULSpace::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_UL_SPACE) {
    if (m_propMargins[0]==100)
      para.m_propertyList.insert("fo:margin-top", para.m_relativeUnit*double(m_margins[0]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-top", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      para.m_propertyList.insert("fo:margin-bottom", para.m_relativeUnit*double(m_margins[1]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-bottom", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
}

void StarPAttributeULSpace::addTo(STOFFPageSpan &page, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type!=ATTR_FRM_UL_SPACE) return;
  if (page.m_actualZone>=0 && page.m_actualZone<=2) {
    if (m_propMargins[0]==100)
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-top", libstoff::convertMiniMToPoint(m_margins[0]), librevenge::RVNG_POINT);
    else
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-top", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-bottom", libstoff::convertMiniMToPoint(m_margins[1]), librevenge::RVNG_POINT);
    else
      page.m_propertiesList[page.m_actualZone].insert("fo:margin-bottom", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
  }
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
  m_propLineSpace=int(input->readLong(1));
  m_lineSpace=int(input->readLong(2));
  m_height=int(input->readULong(2));
  m_lineSpaceRule=int(input->readULong(1));
  m_interLineSpaceRule=int(input->readULong(1));

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeLRSpace::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
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
      m_textLeft+=m_margins[2];
    }
    else
      input->seek(-4, librevenge::RVNG_SEEK_CUR);
  }
  if (vers>=4 && (autofirst&0x80)) { // negative margin
    int32_t nMargin;
    *input >> nMargin;
    m_margins[0]=nMargin;
    m_textLeft=m_margins[2]>=0 ? nMargin : nMargin-m_margins[2];
    *input >> nMargin;
    m_margins[1]=nMargin;
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeNumRule::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  std::vector<uint32_t> string;
  if (!zone.readString(string) || input->tell()>endPos) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the sTmp\n"));
    f << "###sTmp,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_name=libstoff::getString(string);
  if (vers>0)
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
  int N=int(input->readULong(1));
  if (input->tell()+7*N>endPos) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: N is too big\n"));
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

bool StarPAttributeULSpace::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
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

namespace StarParagraphAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
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

  map[StarAttribute::ATTR_PARA_ADJUST]=shared_ptr<StarAttribute>(new StarPAttributeAdjust(StarAttribute::ATTR_PARA_ADJUST,"parAtrAdjust"));
  map[StarAttribute::ATTR_FRM_LR_SPACE]=shared_ptr<StarAttribute>(new StarPAttributeLRSpace(StarAttribute::ATTR_FRM_LR_SPACE,"lrSpace"));
  map[StarAttribute::ATTR_FRM_UL_SPACE]=shared_ptr<StarAttribute>(new StarPAttributeULSpace(StarAttribute::ATTR_FRM_UL_SPACE,"ulSpace"));
  map[StarAttribute::ATTR_PARA_LINESPACING]=shared_ptr<StarAttribute>(new StarPAttributeLineSpacing(StarAttribute::ATTR_PARA_LINESPACING,"parAtrLinespacing"));
  map[StarAttribute::ATTR_PARA_TABSTOP]=shared_ptr<StarAttribute>(new StarPAttributeTabStop(StarAttribute::ATTR_PARA_TABSTOP,"parAtrTabStop"));
  map[StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE]=shared_ptr<StarAttribute>(new StarPAttributeLRSpace(StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE,"eeOutLrSpace"));
  map[StarAttribute::ATTR_PARA_DROP]=shared_ptr<StarAttribute>(new StarPAttributeDrop(StarAttribute::ATTR_PARA_DROP,"parAtrDrop"));

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
  map[StarAttribute::ATTR_PARA_HYPHENZONE]=shared_ptr<StarAttribute>(new StarPAttributeHyphen(StarAttribute::ATTR_PARA_HYPHENZONE,"parAtrHyphenZone"));
  map[StarAttribute::ATTR_PARA_NUMRULE]=shared_ptr<StarAttribute>(new StarPAttributeNumRule(StarAttribute::ATTR_PARA_NUMRULE,"parAtrNumRule"));

  // TODO
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
