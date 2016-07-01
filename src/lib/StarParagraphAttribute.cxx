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

//! a left, right, ... attribute
class StarPAttributeLRSpace : public StarAttribute
{
public:
  //! constructor
  StarPAttributeLRSpace(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_textLeft(0), m_autoFirst(true)
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

void StarPAttributeLRSpace::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_LR_SPACE || m_type==ATTR_EE_PARA_OUTLLR_SPACE) {
    if (m_propMargins[0]==100)
      para.m_propertyList.insert("fo:margin-left", libstoff::convertMiniMToPoint(m_margins[0]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-left", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      para.m_propertyList.insert("fo:margin-right", libstoff::convertMiniMToPoint(m_margins[1]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-right", double(m_propMargins[1])/100., librevenge::RVNG_PERCENT);
    if (m_propMargins[2]==100)
      para.m_propertyList.insert("fo:text-indent", libstoff::convertMiniMToPoint(m_margins[2]), librevenge::RVNG_POINT);
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

void StarPAttributeULSpace::addTo(STOFFParagraph &para, StarItemPool const */*pool*/, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_UL_SPACE) {
    if (m_propMargins[0]==100)
      para.m_propertyList.insert("fo:margin-top", libstoff::convertMiniMToPoint(m_margins[0]), librevenge::RVNG_POINT);
    else
      para.m_propertyList.insert("fo:margin-top", double(m_propMargins[0])/100., librevenge::RVNG_PERCENT);

    if (m_propMargins[1]==100)
      para.m_propertyList.insert("fo:margin-bottom", libstoff::convertMiniMToPoint(m_margins[1]), librevenge::RVNG_POINT);
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
  printData(f);
  m_adjust=int(input->readULong(1));
  if (vers>=1) m_flags=int(input->readULong(1));
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

  map[StarAttribute::ATTR_PARA_ADJUST]=shared_ptr<StarAttribute>(new StarPAttributeAdjust(StarAttribute::ATTR_PARA_ADJUST,"parAtrAdjust"));
  map[StarAttribute::ATTR_FRM_LR_SPACE]=shared_ptr<StarAttribute>(new StarPAttributeLRSpace(StarAttribute::ATTR_FRM_LR_SPACE,"lrSpace"));
  map[StarAttribute::ATTR_FRM_UL_SPACE]=shared_ptr<StarAttribute>(new StarPAttributeULSpace(StarAttribute::ATTR_FRM_UL_SPACE,"ulSpace"));
  map[StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE]=shared_ptr<StarAttribute>(new StarPAttributeLRSpace(StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE,"eeOutLrSpace"));

  // seems safe to ignore
  addAttributeBool(map,StarAttribute::ATTR_PARA_SCRIPTSPACE,"para[scriptSpace]",false); // script type, use by ui
  std::stringstream s;
  for (int type=StarAttribute::ATTR_PARA_DUMMY5; type<=StarAttribute::ATTR_PARA_DUMMY8; ++type) {
    s.str("");
    s << "paraDummy" << type-StarAttribute::ATTR_PARA_DUMMY5+5;
    addAttributeBool(map,StarAttribute::Type(type), s.str(), false);
  }

  // can we retrieve the following attribute ?
  addAttributeBool(map,StarAttribute::ATTR_PARA_REGISTER,"para[register]",false);
  addAttributeBool(map,StarAttribute::ATTR_PARA_FORBIDDEN_RULES,"para[forbiddenRules]",true); // If the forbidden characters rules are to be applied or not.

  // TODO
  addAttributeUInt(map, StarAttribute::ATTR_EE_PARA_BULLETSTATE,"para[bullet,state]",2,0);
  addAttributeUInt(map, StarAttribute::ATTR_EE_PARA_OUTLLEVEL,"para[outlevel]",2,0);
  addAttributeBool(map, StarAttribute::ATTR_EE_PARA_ASIANCJKSPACING,"para[asianCJKSpacing]",false);
  addAttributeVoid(map, StarAttribute::ATTR_EE_PARA_XMLATTRIBS, "para[xmlAttrib]");
  addAttributeVoid(map, StarAttribute::ATTR_EE_FEATURE_TAB, "feature[tab]");
  addAttributeVoid(map, StarAttribute::ATTR_EE_FEATURE_LINEBR, "feature[linebr]");
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
