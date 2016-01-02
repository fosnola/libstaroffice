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

#include "StarAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarLanguage.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarZone.hxx"

#include "StarCellAttribute.hxx"

namespace StarCellAttribute
{
//! a character bool attribute
class StarCAttributeBool : public StarAttributeBool
{
public:
  //! constructor
  StarCAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeBool(*this));
  }
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;

protected:
  //! copy constructor
  StarCAttributeBool(StarCAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

//! a character color attribute
class StarCAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarCAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeColor(*this));
  }
  //! add to a cell style
  //virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarCAttributeColor(StarCAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

//! a character integer attribute
class StarCAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarCAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarCAttributeInt(StarCAttributeInt const &orig) : StarAttributeInt(orig)
  {
  }
};

//! a character unsigned integer attribute
class StarCAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarCAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeUInt(*this));
  }
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarCAttributeUInt(StarCAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

//! a void attribute
class StarCAttributeVoid : public StarAttributeVoid
{
public:
  //! constructor
  StarCAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeVoid(*this));
  }
  //! add to a cell style
  //virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarCAttributeVoid(StarCAttributeVoid const &orig) : StarAttributeVoid(orig)
  {
  }
};

void StarCAttributeBool::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type==ATTR_SC_LINEBREAK)
    cell.m_propertyList.insert("fo:wrap-option",m_value ? "wrap" : "no-wrap");
}

void StarCAttributeInt::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type==ATTR_SC_ROTATE_VALUE) {
    int prevRot=cell.m_propertyList["style:rotation-angle"] ? cell.m_propertyList["style:rotation-angle"]->getInt() : 0;
    cell.m_propertyList.insert("style:rotation-angle",prevRot+m_value);
  }
}

void StarCAttributeUInt::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type==ATTR_SC_VALUE_FORMAT)
    cell.m_format=(unsigned) m_value;
  else if (m_type==ATTR_SC_HORJUSTIFY) {
    cell.m_propertyList.insert("style:repeat-content", false);
    switch (m_value) {
    case 0: // standard
      cell.m_propertyList.insert("style:text-align-source", "value-type");
      if (cell.m_propertyList["fo:text-align"]) cell.m_propertyList.remove("fo:text-align");
      break;
    case 1: // left
    case 2: // center
    case 3: // right
      cell.m_propertyList.insert("style:text-align-source", "fix");
      cell.m_propertyList.insert("fo:text-align", m_value==1 ? "first" : m_value==2 ? "center" : "end");
      break;
    case 4: // block
      cell.m_propertyList.insert("style:text-align-source", "fix");
      cell.m_propertyList.insert("fo:text-align", "justify");
      break;
    case 5: // repeat
      cell.m_propertyList.insert("style:repeat-content", true);
      break;
    default:
      cell.m_propertyList.insert("style:text-align-source", "value-type");
      if (cell.m_propertyList["fo:text-align"]) cell.m_propertyList.remove("fo:text-align");
      STOFF_DEBUG_MSG(("StarCellAttribute::StarCAttributeUInt::addTo: find unknown horizontal enum=%d\n", int(m_value)));
      break;
    }
  }
  else if (m_type==ATTR_SC_VERJUSTIFY || m_type==ATTR_SC_ROTATE_MODE) {
    switch (m_value) {
    case 0: // standard
      if (m_type==ATTR_SC_VERJUSTIFY)
        cell.m_propertyList.insert("style:vertical-align", "automatic");
      else
        cell.m_propertyList.insert("style:rotation-align", "none");
      break;
    case 1: // top
    case 2: // center
    case 3: // bottom
      cell.m_propertyList.insert(m_type==ATTR_SC_VERJUSTIFY ? "style:vertical-align" : "style:rotation-align",
                                 m_value==1 ? "top" : m_value==2 ? "middle" : "bottom");
      break;
    default:
      if (m_type==ATTR_SC_VERJUSTIFY)
        cell.m_propertyList.insert("style:vertical-align", "automatic");
      else
        cell.m_propertyList.insert("style:rotation-align", "none");
      if (m_type==ATTR_SC_VERJUSTIFY && m_value==4) // block ?
        break;
      STOFF_DEBUG_MSG(("StarCellAttribute::StarCAttributeUInt::addTo: find unknown vertical/rotateMode enum=%d\n", int(m_value)));
      break;
    }
  }
  else if (m_type==ATTR_SC_ORIENTATION) {
    if (cell.m_propertyList["style:direction"]) cell.m_propertyList.remove("style:direction");
    // fixme: we must also revert the rotation angle, but ...
    switch (m_value) {
    case 0: // standard
      break;
    case 1: // topbottom
    case 2: { // bottomtop
      int prevRot=cell.m_propertyList["style:rotation-angle"] ? cell.m_propertyList["style:rotation-angle"]->getInt() : 0;
      cell.m_propertyList.insert("style:rotation-angle",prevRot+(m_value==1 ? 270 : 90));
      break;
    }
    case 3: // stacked
      cell.m_propertyList.insert("style:direction","ttb");
      break;
    default:
      STOFF_DEBUG_MSG(("StarCellAttribute::StarCAttributeUInt::addTo: find unknown orientation enum=%d\n", int(m_value)));
      break;
    }
  }
  else if (m_type==ATTR_SC_WRITINGDIR) {
    if (m_value<=4) {
      char const *(wh[])= {"lr-tb", "rl-tb", "tb-rl", "tb-lr", "page"};
      cell.m_propertyList.insert("style:writing-mode", wh[m_value]);
    }
    else {
      cell.m_propertyList.insert("style:writing-mode", "page");
      STOFF_DEBUG_MSG(("StarCellAttribute::StarCAttributeUInt::addTo: find unknown writing dir enum=%d\n", int(m_value)));
    }
  }
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeVoid(type,debugName));
}

}

namespace StarCellAttribute
{
//! a margins attribute
class StarCAttributeMargins : public StarAttribute
{
public:
  //! constructor
  StarCAttributeMargins(Type type, std::string const &debugName) : StarAttribute(type, debugName)
  {
    for (int i=0; i<4; ++i) m_margins[i]=20;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeMargins(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    for (int i=0; i<4; ++i) {
      char const *(wh[])= {"top", "left", "right", "bottom"};
      if (m_margins[i]) o << "margin[" << wh[i] << "]=" << m_margins[i] << ",";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarCAttributeMargins(StarCAttributeMargins const &orig) : StarAttribute(orig)
  {
    for (int i=0; i<4; ++i) m_margins[i]=orig.m_margins[i];
  }
  //! the top, left, right, bottom margins
  int m_margins[4];
};

//! a merge attribute
class StarCAttributeMerge : public StarAttribute
{
public:
  //! constructor
  StarCAttributeMerge(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_span(1,1)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeMerge(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName;
    if (m_span!=STOFFVec2i(1,1)) o << "=" << m_span;
    o << ",";
  }

protected:
  //! copy constructor
  StarCAttributeMerge(StarCAttributeMerge const &orig) : StarAttribute(orig), m_span(orig.m_span)
  {
  }
  //! the span
  STOFFVec2i m_span;
};

//! Pattern attribute of StarAttributeInternal
class StarCAttributePattern : public StarAttributeItemSet
{
public:
  //! constructor
  StarCAttributePattern() :
    StarAttributeItemSet(StarAttribute::ATTR_SC_PATTERN, "ScPattern", std::vector<STOFFVec2i>()), m_style("")
  {
    m_limits.push_back(STOFFVec2i(100,148));
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributePattern(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
  {
    STOFFInputStreamPtr input=zone.input();
    long pos=input->tell();
    libstoff::DebugFile &ascFile=zone.ascii();
    libstoff::DebugStream f;
    f << "StarAttribute[" << zone.getRecordLevel() << "]:" << m_debugName << ",";
    // sc_patattr.cxx ScPatternAttr::Create
    bool hasStyle;
    *input>>hasStyle;
    if (hasStyle) {
      f << "hasStyle,";
      std::vector<uint32_t> text;
      if (!zone.readString(text) || text.size()>1000 || input->tell()>endPos) {
        STOFF_DEBUG_MSG(("StarCellAttribute::StarCAttributePattern::read: can not read a name\n"));
        f << "###name,";
        return false;
      }
      m_itemSet.m_style=libstoff::getString(text);
      if (!text.empty())
        f << "name=" << m_itemSet.m_style.cstr() << ",";
      m_itemSet.m_family=(int) input->readULong(2);
    }
    bool ok=object.readItemSet(zone, m_limits, endPos, m_itemSet, object.getCurrentPool(false).get());
    if (!ok) f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return ok && input->tell()<=endPos;
  }
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    StarAttributeItemSet::print(o);
    if (m_style.empty())
      return;
    o << "style=" << m_style.cstr() << ",";
  }
protected:
  //! copy constructor
  StarCAttributePattern(StarCAttributePattern const &orig) : StarAttributeItemSet(orig), m_style(orig.m_style)
  {
  }
  //! the style
  librevenge::RVNGString m_style;
};

//! a protection attribute
class StarCAttributeProtection : public StarAttribute
{
public:
  //! constructor
  StarCAttributeProtection(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_protected(false), m_hiddenFormula(false), m_hiddenCell(false), m_doNotPrint(false)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeProtection(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a cell style
  virtual void addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const;
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_protected) o << "protected,";
    if (m_hiddenFormula) o << "hiddenFormula,";
    if (m_hiddenCell) o << "hiddenCell,";
    if (m_doNotPrint) o << "doNotPrint,";
    o << "],";
  }

protected:
  //! copy constructor
  StarCAttributeProtection(StarCAttributeProtection const &orig) : StarAttribute(orig), m_protected(orig.m_protected), m_hiddenFormula(orig.m_hiddenFormula), m_hiddenCell(orig.m_hiddenCell), m_doNotPrint(orig.m_doNotPrint)
  {
  }
  //! the cell is protected
  bool m_protected;
  //! the formula is hidden
  bool m_hiddenFormula;
  //! the cell is hidden and protected
  bool m_hiddenCell;
  //! do not print the formula
  bool m_doNotPrint;
};

void StarCAttributeMargins::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type!=ATTR_SC_MARGIN)
    return;
  for (int i=0; i<4; ++i) {
    char const *(wh[])= {"top", "left", "right", "bottom"};
    cell.m_propertyList.insert((std::string("fo:padding-")+wh[i]).c_str(), float(m_margins[i])/20.f, librevenge::RVNG_POINT);
  }
}

void StarCAttributeMerge::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type!=ATTR_SC_MERGE)
    return;
  cell.m_numberCellSpanned=STOFFVec2i(1,1);
  if (m_span==STOFFVec2i(0,0)) // checkme
    return;
  if (m_span[0]<=0 || m_span[1]<=0) {
    STOFF_DEBUG_MSG(("StarCellAttribute::StarCAttributeMerge::addTo: the span value %dx%d seems bad\n", m_span[0], m_span[1]));
  }
  else
    cell.m_numberCellSpanned=m_span;
}

void StarCAttributeProtection::addTo(STOFFCellStyle &cell, StarItemPool const */*pool*/) const
{
  if (m_type!=ATTR_SC_PROTECTION)
    return;
  if (m_hiddenCell)
    cell.m_propertyList.insert("style:cell-protect","hidden-and-protected");
  else if (m_protected || m_hiddenFormula)
    cell.m_propertyList.insert
    ("style:cell-protect", m_protected ? (m_hiddenFormula ? "hidden-and-protected" : "protected") : "formula-hidden");
  else
    cell.m_propertyList.insert("style:cell-protect","none");
  cell.m_propertyList.insert("style:print-content", m_doNotPrint);
  // hiddenCell ?
}

bool StarCAttributeMargins::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  // algItem SvxMarginItem::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  for (int i=0; i<4; ++i)
    m_margins[i]=(int) input->readLong(2);
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeMerge::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  // sc_attrib.cxx ScMergeAttr::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  int span[2];
  for (int i=0; i<2; ++i)
    span[i]=(int) input->readLong(2);
  m_span=STOFFVec2i(span[0], span[1]);
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeProtection::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  *input >> m_protected;
  *input >> m_hiddenFormula;
  *input >> m_hiddenCell;
  *input >> m_doNotPrint;
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

}

namespace StarCellAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  // --- sc --- sc_docpool.cxx
  addAttributeVoid(map, StarAttribute::ATTR_SC_USERDEF, "sc[userDef]");
  addAttributeUInt(map, StarAttribute::ATTR_SC_VALUE_FORMAT,"format[value]",4,0);
  addAttributeUInt(map, StarAttribute::ATTR_SC_HORJUSTIFY,"justify[horz]",2,0); // standart
  addAttributeUInt(map, StarAttribute::ATTR_SC_VERJUSTIFY,"justify[vert]",2,0); // standart
  addAttributeBool(map, StarAttribute::ATTR_SC_LINEBREAK,"lineBreak", false);
  addAttributeUInt(map, StarAttribute::ATTR_SC_ORIENTATION,"orientation",2,0); // standard
  addAttributeInt(map, StarAttribute::ATTR_SC_ROTATE_VALUE,"rotate[value]",4,0);
  addAttributeUInt(map, StarAttribute::ATTR_SC_ROTATE_MODE,"rotate[mode]",2,0); // normal
  addAttributeUInt(map, StarAttribute::ATTR_SC_WRITINGDIR,"writing[dir]",2,4); // frame environment

  map[StarAttribute::ATTR_SC_PATTERN]=shared_ptr<StarAttribute>(new StarCAttributePattern());
  map[StarAttribute::ATTR_SC_MARGIN]=shared_ptr<StarAttribute>(new StarCAttributeMargins(StarAttribute::ATTR_SC_MARGIN,"scMargins"));
  map[StarAttribute::ATTR_SC_MERGE]=shared_ptr<StarAttribute>(new StarCAttributeMerge(StarAttribute::ATTR_SC_MERGE,"scMerge"));
  map[StarAttribute::ATTR_SC_PROTECTION]=shared_ptr<StarAttribute>(new StarCAttributeProtection(StarAttribute::ATTR_SC_PROTECTION,"scProtection"));

  // TOUSE
  addAttributeUInt(map, StarAttribute::ATTR_SC_INDENT,"indent",2,0);
  addAttributeBool(map, StarAttribute::ATTR_SC_VERTICAL_ASIAN,"vertical[asian]", false);
  addAttributeInt(map, StarAttribute::ATTR_SC_MERGE_FLAG,"merge[flag]",2,0);
  addAttributeUInt(map, StarAttribute::ATTR_SC_VALIDDATA,"data[valid]",4,0);
  addAttributeUInt(map, StarAttribute::ATTR_SC_CONDITIONAL,"conditional",4,0);

}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
