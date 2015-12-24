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

#include "STOFFPageSpan.hxx"
#include "STOFFSubDocument.hxx"

#include "StarAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarLanguage.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarZone.hxx"

#include "StarPageAttribute.hxx"

namespace StarPageAttribute
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
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeBool(*this));
  }
  //! add to a page
  // virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const;

protected:
  //! copy constructor
  StarPAttributeBool(StarPAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

//! a character color attribute
class StarPAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarPAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeColor(*this));
  }
  //! add to a page
  // virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarPAttributeColor(StarPAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

//! a character integer attribute
class StarPAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarPAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a page
  // virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const;
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

//! a character unsigned integer attribute
class StarPAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarPAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeUInt(*this));
  }
  //! add to a page
  // virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarPAttributeUInt(StarPAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

//! a void attribute
class StarPAttributeVoid : public StarAttributeVoid
{
public:
  //! constructor
  StarPAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! add to a page
  // virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const;
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

//! add a bool attribute
void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeBool(type,debugName, defValue));
}
//! add a color attribute
void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeColor(type,debugName, defValue));
}
//! add a int attribute
void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeVoid(type,debugName));
}

}

namespace StarPageAttribute
{
////////////////////////////////////////
//! Internal: the subdocument of a StarObjectSpreadsheet
class SubDocument : public STOFFSubDocument
{
public:
  SubDocument(shared_ptr<StarObjectSmallText> text) :
    STOFFSubDocument(0, STOFFInputStreamPtr(), STOFFEntry()), m_smallText(text) {}

  //! destructor
  virtual ~SubDocument() {}

  //! operator!=
  virtual bool operator!=(STOFFSubDocument const &doc) const
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    SubDocument const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_smallText.get() != sDoc->m_smallText.get()) return true;
    return false;
  }

  //! operator!==
  virtual bool operator==(STOFFSubDocument const &doc) const
  {
    return !operator!=(doc);
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type);

protected:
  //! the note text
  shared_ptr<StarObjectSmallText> m_smallText;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (m_smallText)
    m_smallText->send(listener);
}

// ------------------------------------------------------------
//! a page header/footer attribute
class StarPAttributePageHF : public StarAttribute
{
public:
  //! constructor
  StarPAttributePageHF(Type type, std::string const &debugName) : StarAttribute(type, debugName)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributePageHF(*this));
  }
  //! add to a page
  virtual void addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const;
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=*,";
  }

protected:
  //! copy constructor
  StarPAttributePageHF(StarPAttributePageHF const &orig) : StarAttribute(orig)
  {
    for (int i=0; i<3; ++i) m_zones[i]=orig.m_zones[i];
  }
  //! the left/middle/right zones
  shared_ptr<StarObjectSmallText> m_zones[3];
};

//! a page attribute
class StarPAttributePage : public StarAttribute
{
public:
  //! constructor
  StarPAttributePage(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_name(""), m_pageType(0), m_landscape(false), m_used(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributePage(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (!m_name.empty()) o << m_name.cstr();
    if (m_pageType) o << "type=" << m_pageType << ",";
    if (m_landscape) o << "landscape,";
    if (m_used) o << "used=" << m_used << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarPAttributePage(StarPAttributePage const &orig) : StarAttribute(orig), m_name(orig.m_name), m_pageType(orig.m_pageType), m_landscape(orig.m_landscape), m_used(orig.m_used)
  {
  }
  //! the name
  librevenge::RVNGString m_name;
  //! the type
  int m_pageType;
  //! true if landscape
  bool m_landscape;
  //! the number of use
  int m_used;
};

//! a print attribute
class StarPAttributePrint : public StarAttribute
{
public:
  //! constructor
  StarPAttributePrint(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_tableList()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributePrint(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    for (size_t i=0; i<m_tableList.size(); ++i)
      o << m_tableList[i];
    o << "],";
  }

protected:
  //! copy constructor
  StarPAttributePrint(StarPAttributePrint const &orig) : StarAttribute(orig), m_tableList(orig.m_tableList)
  {
  }
  //! the list of table to print
  std::vector<int> m_tableList;
};

//! a rangeItem attribute
class StarPAttributeRangeItem : public StarAttribute
{
public:
  //! constructor
  StarPAttributeRangeItem(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_table(-1), m_range()
  {
    for (int i=0; i<2; ++i) m_flags[i]=false;
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeRangeItem(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    o << "range=" << m_range << ",";
    if (m_table>255) o << "all[table],";
    else if (m_table>=0) o << "table=" << m_table << ",";
    for (int i=0; i<2; ++i) {
      if (m_flags[i]) o << "flag" << i << ",";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarPAttributeRangeItem(StarPAttributeRangeItem const &orig) : StarAttribute(orig), m_table(orig.m_table), m_range(orig.m_range)
  {
    for (int i=0; i<2; ++i) m_flags[i]=orig.m_flags[i];
  }
  //! the table(v0)
  int m_table;
  //! the range
  STOFFBox2i m_range;
  //! two flags
  bool m_flags[2];
};

//! a character unsigned integer attribute
class StarPAttributeViewMode : public StarAttributeUInt
{
public:
  //! constructor
  StarPAttributeViewMode(Type type, std::string const &debugName) :
    StarAttributeUInt(type, debugName, 2, 0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeViewMode(*this));
  }
  //! try to read a field
  bool read(StarZone &zone, int vers, long endPos, StarObject &object)
  {
    if (vers==0) {
      m_value=0; // show
      return true;
    }
    return StarAttributeUInt::read(zone, vers, endPos, object);
  }
  //! print data
  void print(libstoff::DebugStream &o) const
  {
    o << m_debugName;
    switch (m_value) {
    case 0: // show
      break;
    case 1:
      o << "=hide";
      break;
    default:
      STOFF_DEBUG_MSG(("StarPAttribute::StarPAttributeViewMode::print: find unknown enum=%d\n", int(m_value)));
      o << "=###" << m_value;
      break;
    }
    o << ",";
  }

protected:
  //! copy constructor
  StarPAttributeViewMode(StarPAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

void StarPAttributePageHF::addTo(STOFFPageSpan &page, StarItemPool const */*pool*/) const
{
  bool isHeader=m_type==ATTR_SC_PAGE_HEADERLEFT || m_type==ATTR_SC_PAGE_HEADERRIGHT;
  if (!isHeader && m_type!=ATTR_SC_PAGE_FOOTERLEFT && m_type!=ATTR_SC_PAGE_FOOTERRIGHT)
    return;
  STOFFHeaderFooter hf;
  bool hasData=false;
  for (int i=0; i<3; ++i) {
    if (!m_zones[i]) continue;
    hasData=true;
    hf.m_subDocument[i].reset(new SubDocument(m_zones[i]));
  }
  if (!hasData) return;
  std::string wh(m_type==ATTR_SC_PAGE_HEADERLEFT || m_type==ATTR_SC_PAGE_FOOTERLEFT ? "left" : "right");
  page.addHeaderFooter(isHeader,wh, hf);
}

bool StarPAttributePage::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // svx_pageitem.cxx SvxPageItem::Create
  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarPAttributePage::read: can not read a name\n"));
    f << "###name,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (!text.empty())
    m_name=libstoff::getString(text);
  m_pageType=(int) input->readULong(1);
  *input >> m_landscape;
  m_used=(int) input->readULong(2);
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributePageHF::read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  for (int i=0; i<3; ++i) {
    long actPos=input->tell();
    shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(object, true));
    if (!smallText->read(zone, endPos) || input->tell()>endPos) {
      f << "###editTextObject";
      input->seek(actPos, librevenge::RVNG_SEEK_SET);
      ok=false;
    }
    m_zones[i]=smallText;
  }
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarPAttributePrint::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  uint16_t n;
  *input >> n;
  if (!n||input->tell()+2*int(n)>endPos) {
    STOFF_DEBUG_MSG(("StarPellAttribute::StarPAttributePrint::read: the number seems bad\n"));
    f << "###n=" << n << ",";
    ok=false;
  }
  for (int i=0; i<int(n); ++i)
    m_tableList.push_back((int) input->readULong(2));
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarPAttributeRangeItem::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  // algItem ScRangeItem::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  int dim[4];
  if (vers==0) {
    m_table=(int) input->readULong(2);
    for (int i=0; i<4; ++i) dim[i]=(int) input->readULong(2);
  }
  else {
    for (int i=0; i<4; ++i) dim[i]=(int) input->readULong(2);
    if (vers>=2) {
      *input>>m_flags[0];
      if (input->tell()+1==endPos) // checkme
        *input>>m_flags[1];
    }
  }
  m_range=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}
}

namespace StarPageAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  // TODO
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_PAPERTRAY,"page[papertray]",2,0);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_HORCENTER,"page[horizontal,center]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_VERCENTER,"page[vertical,center]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_ON,"page[on]", true);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_DYNAMIC,"page[dynamic]", true);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_SHARED,"page[shared]", true);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_NOTES,"page[notes]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_GRID,"page[grid]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_HEADERS,"page[headers]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_TOPDOWN,"page[topdown]", true);
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_SCALE,"page[scale]",2,100);
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_SCALETOPAGES,"page[scaleToPage]",2,1);
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_FIRSTPAGENO,"page[first,pageNo]",2,1);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_FORMULAS,"page[formulas]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_NULLVALS,"page[nullvals]", true);

  map[StarAttribute::ATTR_SC_PAGE]=shared_ptr<StarAttribute>(new StarPAttributePage(StarAttribute::ATTR_SC_PAGE, "page"));
  map[StarAttribute::ATTR_SC_PAGE_CHARTS]=shared_ptr<StarAttribute>(new StarPAttributeViewMode(StarAttribute::ATTR_SC_PAGE_CHARTS, "page[charts]"));
  map[StarAttribute::ATTR_SC_PAGE_OBJECTS]=shared_ptr<StarAttribute>(new StarPAttributeViewMode(StarAttribute::ATTR_SC_PAGE_OBJECTS, "page[objects]"));
  map[StarAttribute::ATTR_SC_PAGE_DRAWINGS]=shared_ptr<StarAttribute>(new StarPAttributeViewMode(StarAttribute::ATTR_SC_PAGE_DRAWINGS, "page[drawings]"));
  map[StarAttribute::ATTR_SC_PAGE_PRINTTABLES]=shared_ptr<StarAttribute>(new StarPAttributePrint(StarAttribute::ATTR_SC_PAGE_PRINTTABLES, "page[printtables]"));
  map[StarAttribute::ATTR_SC_PAGE_HEADERLEFT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_HEADERLEFT, "header[left]"));
  map[StarAttribute::ATTR_SC_PAGE_HEADERRIGHT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_HEADERRIGHT, "header[right]"));
  map[StarAttribute::ATTR_SC_PAGE_FOOTERLEFT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_FOOTERLEFT, "footer[left]"));
  map[StarAttribute::ATTR_SC_PAGE_FOOTERRIGHT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_FOOTERRIGHT, "footer[right]"));
  map[StarAttribute::ATTR_SC_PAGE_SIZE]=shared_ptr<StarAttribute>(new StarAttributeVec2i(StarAttribute::ATTR_SC_PAGE_SIZE, "page[size]", 4));
  map[StarAttribute::ATTR_SC_PAGE_MAXSIZE]=shared_ptr<StarAttribute>(new StarAttributeVec2i(StarAttribute::ATTR_SC_PAGE_MAXSIZE, "page[maxsize]", 4));
  map[StarAttribute::ATTR_SC_PAGE_PRINTAREA]=shared_ptr<StarAttribute>(new StarPAttributeRangeItem(StarAttribute::ATTR_SC_PAGE_PRINTAREA, "page[printArea]"));
  map[StarAttribute::ATTR_SC_PAGE_REPEATCOL]=shared_ptr<StarAttribute>(new StarPAttributeRangeItem(StarAttribute::ATTR_SC_PAGE_REPEATCOL, "page[repeatCol]"));
  map[StarAttribute::ATTR_SC_PAGE_REPEATROW]=shared_ptr<StarAttribute>(new StarPAttributeRangeItem(StarAttribute::ATTR_SC_PAGE_REPEATROW, "page[repeatRow]"));
  std::vector<STOFFVec2i> limits;
  limits.push_back(STOFFVec2i(142,142)); // BACKGROUND
  limits.push_back(STOFFVec2i(144,146)); // BORDER->SHADOW
  limits.push_back(STOFFVec2i(150,151)); // LRSPACE->ULSPACE
  limits.push_back(STOFFVec2i(155,155)); // PAGESIZE
  limits.push_back(STOFFVec2i(159,161)); // ON -> SHARED
  map[StarAttribute::ATTR_SC_PAGE_HEADERSET]=shared_ptr<StarAttribute>(new StarAttributeItemSet(StarAttribute::ATTR_SC_PAGE_HEADERSET, "setPageHeader", limits));
  map[StarAttribute::ATTR_SC_PAGE_FOOTERSET]=shared_ptr<StarAttribute>(new StarAttributeItemSet(StarAttribute::ATTR_SC_PAGE_FOOTERSET, "setPageFooter", limits));
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
