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
#include "STOFFSection.hxx"
#include "STOFFSubDocument.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarState.hxx"
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
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;

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
  //! destructor
  ~StarPAttributeColor();
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeColor(*this));
  }
  //! add to a page
  // virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
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
  //! add to a page
  // virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
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
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeUInt(*this));
  }
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
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
  //! destructor
  ~StarPAttributeVoid();
  //! add to a page
  // virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
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

//! a list of item attribute of StarAttributeInternal
class StarPAttributeItemSet : public StarAttributeItemSet
{
public:
  //! constructor
  StarPAttributeItemSet(Type type, std::string const &debugName, std::vector<STOFFVec2i> const &limits) :
    StarAttributeItemSet(type, debugName, limits)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeItemSet(*this));
  }
  //! add to a pageSpan
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &done) const;

protected:
  //! copy constructor
  explicit StarPAttributeItemSet(StarAttributeItemSet const &orig) : StarAttributeItemSet(orig)
  {
  }
};

//! an Vec2i attribute
class StarPAttributeVec2i : public StarAttributeVec2i
{
public:
  //! constructor
  StarPAttributeVec2i(Type type, std::string const &debugName, int intSize, STOFFVec2i value=STOFFVec2i(0,0)) : StarAttributeVec2i(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeVec2i(*this));
  }
  //! add to a pageSpan
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
protected:
  //! copy constructor
  explicit StarPAttributeVec2i(StarAttributeVec2i const &orig) : StarAttributeVec2i(orig)
  {
  }
};

void StarPAttributeBool::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_SC_PAGE_HORCENTER) {
    if (state.m_pageZone==0) {
      librevenge::RVNGString act("");
      if (state.m_page.m_propertiesList[0]["style:table-centering"])
        act=state.m_page.m_propertiesList[0]["style:table-centering"]->getStr();
      if (m_value)
        state.m_page.m_propertiesList[0].insert("style:table-centering", (act=="both"||act=="vertical") ? "both" : "horizontal");
      else
        state.m_page.m_propertiesList[0].insert("style:table-centering", (act=="both"||act=="vertical") ? "vertical" : "none");
    }
  }
  else if (m_type==ATTR_SC_PAGE_VERCENTER) {
    if (state.m_pageZone==0) {
      librevenge::RVNGString act("");
      if (state.m_page.m_propertiesList[0]["style:table-centering"])
        act=state.m_page.m_propertiesList[0]["style:table-centering"]->getStr();
      if (m_value)
        state.m_page.m_propertiesList[0].insert("style:table-centering", (act=="both"||act=="horizontal") ? "both" : "vertical");
      else
        state.m_page.m_propertiesList[0].insert("style:table-centering", (act=="both"||act=="horizontal") ? "horizontal" : "none");
    }
  }
  else if (m_type==ATTR_SC_PAGE_HEADERS || m_type==ATTR_SC_PAGE_NOTES ||
           m_type==ATTR_SC_PAGE_GRID || m_type==ATTR_SC_PAGE_FORMULAS ||
           m_type==ATTR_SC_PAGE_NULLVALS) {
    if (state.m_pageZone==0 && m_value) {
      librevenge::RVNGString wh
      (m_type==ATTR_SC_PAGE_HEADERS ? "headers" :
       m_type==ATTR_SC_PAGE_NOTES ? "annotations" :
       m_type==ATTR_SC_PAGE_GRID ? "grid" :
       m_type==ATTR_SC_PAGE_FORMULAS ? "formulas" : "zero-values");
      if (state.m_page.m_propertiesList[0]["style:print"]) {
        librevenge::RVNGString res(state.m_page.m_propertiesList[0]["style:print"]->getStr());
        res.append(" ");
        res.append(wh);
        state.m_page.m_propertiesList[0].insert("style:print", res);
      }
      else
        state.m_page.m_propertiesList[0].insert("style:print", wh);
    }
  }
  else if (m_type==ATTR_SC_PAGE_TOPDOWN) {
    if (state.m_pageZone==0)
      state.m_page.m_propertiesList[0].insert("style:print-page-order", m_value ? "ttb" : "ltr");
  }
}

void StarPAttributeUInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_SC_PAGE_SCALE) {
    if (state.m_pageZone==0) {
      if (m_value)
        state.m_page.m_propertiesList[0].insert("style:scale-to", double(m_value)/100., librevenge::RVNG_PERCENT);
      else if (state.m_page.m_propertiesList[0]["style:scale-to"])
        state.m_page.m_propertiesList[0].remove("style:scale-to");
    }
  }
  else if (m_type==ATTR_SC_PAGE_SCALETOPAGES) {
    if (state.m_pageZone==0) {
      if (m_value)
        state.m_page.m_propertiesList[0].insert("style:scale-to-pages", int(m_value));
      else if (state.m_page.m_propertiesList[0]["style:scale-to-pages"])
        state.m_page.m_propertiesList[0].remove("style:scale-to-pages");
    }
  }
  else if (m_type==ATTR_SC_PAGE_FIRSTPAGENO) {
    if (state.m_pageZone==0)
      state.m_page.m_propertiesList[0].insert("style:first-page-number", int(m_value));
  }
}

void StarPAttributeVec2i::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type!=ATTR_SC_PAGE_SIZE)
    return;
  if (state.m_pageZone==0) {
    state.m_page.m_propertiesList[0].insert("fo:page-width", double(m_value[0])/1440., librevenge::RVNG_INCH);
    state.m_page.m_propertiesList[0].insert("fo:page-height", double(m_value[1])/1440., librevenge::RVNG_INCH);
  }
  else if (state.m_pageZone==1 || state.m_pageZone==2)
    state.m_page.m_propertiesList[state.m_pageZone].insert("fo:min-height", double(m_value[1])/1440., librevenge::RVNG_INCH);
}

void StarPAttributeItemSet::addTo(StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarPAttributeItemSet::addTo: find a cycle\n"));
    return;
  }
  if (m_type!=ATTR_SC_PAGE_HEADERSET && m_type!=ATTR_SC_PAGE_FOOTERSET)
    return;
  STOFFPageSpan::ZoneType prevZone=state.m_pageZone;
  state.m_pageZone=m_type==ATTR_SC_PAGE_HEADERSET ? STOFFPageSpan::Header : STOFFPageSpan::Footer;
  StarAttributeItemSet::addTo(state, done);
  state.m_pageZone=prevZone;
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

namespace StarPageAttribute
{
////////////////////////////////////////
//! Internal: the subdocument of a StarObjectSpreadsheet
class SubDocument : public STOFFSubDocument
{
public:
  explicit SubDocument(shared_ptr<StarObjectSmallText> text) :
    STOFFSubDocument(0, STOFFInputStreamPtr(), STOFFEntry()), m_smallText(text), m_format(), m_object(0), m_pool(0) {}
  SubDocument(shared_ptr<StarFormatManagerInternal::FormatDef> format, StarItemPool const *pool, StarObject *object) :
    STOFFSubDocument(0, STOFFInputStreamPtr(), STOFFEntry()), m_smallText(), m_format(format), m_object(object), m_pool(pool) {}

  //! destructor
  virtual ~SubDocument() {}

  //! operator!=
  virtual bool operator!=(STOFFSubDocument const &doc) const
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    SubDocument const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_smallText.get() != sDoc->m_smallText.get()) return true;
    if (m_format.get() != sDoc->m_format.get()) return true;
    if (m_object != sDoc->m_object) return true;
    if (m_pool != sDoc->m_pool) return true;
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
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  //! the original object
  StarObject *m_object;
  //! the pool
  StarItemPool const *m_pool;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (m_smallText)
    m_smallText->send(listener);
  else if (m_format && m_object) {
    StarState state(m_pool, *m_object); // check me, set relUnit
    m_format->send(listener, state);
  }
}

//! a frame + columns
class StarPAttributeColumns : public StarAttribute
{
protected:
  //! a column
  struct Column {
    //! constructor
    Column() : m_wishWidth(0)
    {
      for (int i=0; i<4; ++i) m_margins[i]=0;
    }
    //! debug function to print the data
    void printData(libstoff::DebugStream &o) const
    {
      if (m_wishWidth) o << "wish[width]=" << m_wishWidth << ",";
      for (int i=0; i<4; ++i) {
        if (!m_margins[i]) continue;
        char const *(wh[])= {"left", "top", "right", "bottom" };
        o << wh[i] << "=" << m_margins[i] << ",";
      }
    }
    bool addTo(librevenge::RVNGPropertyList &propList) const
    {
      if (m_margins[0])
        propList.insert("fo:start-indent", double(m_margins[0])*0.05, librevenge::RVNG_POINT);
      if (m_margins[1])
        propList.insert("fo:end-indent", double(m_margins[1])*0.05, librevenge::RVNG_POINT);
      if (m_wishWidth) // must be in twip
        propList.insert("style:rel-width", double(m_wishWidth)*0.05*20, librevenge::RVNG_TWIP);
      return true;
    }

    //! the wish width
    int m_wishWidth;
    //! the left, top, right bottom margins
    int m_margins[4];
  };
public:
  //! constructor
  StarPAttributeColumns(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_lineAdj(0), m_ortho(true), m_lineHeight(0), m_gutterWidth(0), m_wishWidth(0),
    m_penStyle(0), m_penWidth(0), m_penColor(STOFFColor::black()), m_columnList()
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeColumns(*this));
  }
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (m_lineAdj) o << "line[adj]=" << m_lineAdj << ",";
    if (!m_ortho) o << "ortho*,";
    if (m_lineHeight) o << "line[height]=" << m_lineHeight << ",";
    if (m_gutterWidth) o << "gutter[width]=" << m_gutterWidth << ",";
    if (m_wishWidth) o << "wish[width]=" << m_wishWidth << ",";
    if (m_penStyle) o << "pen[style]=" << m_penStyle << ",";
    if (m_penWidth) o << "pen[width]=" << m_penWidth << ",";
    if (!m_penColor.isBlack()) o << "pen[color]=" << m_penColor << ",";
    if (!m_columnList.empty()) {
      o << "columns=[";
      for (size_t i=0; i<m_columnList.size(); ++i) {
        o << "[";
        m_columnList[i].printData(o);
        o << "],";
      }
      o << "],";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarPAttributeColumns(StarPAttributeColumns const &orig) :
    StarAttribute(orig), m_lineAdj(orig.m_lineAdj), m_ortho(orig.m_ortho), m_lineHeight(orig.m_lineHeight),
    m_gutterWidth(orig.m_gutterWidth), m_wishWidth(orig.m_wishWidth), m_penStyle(orig.m_penStyle), m_penWidth(orig.m_penWidth),
    m_penColor(orig.m_penColor), m_columnList(orig.m_columnList)
  {
  }

  //! the lineAdj
  int m_lineAdj;
  //! ortho flag
  bool m_ortho;
  //! the line height
  int m_lineHeight;
  //! the gutter width
  int m_gutterWidth;
  //! the wish width
  int m_wishWidth;
  //! the pen style
  int m_penStyle;
  //! the pen width
  int m_penWidth;
  //! the pen color
  STOFFColor m_penColor;
  //! the column list
  std::vector<Column> m_columnList;
};

//! a frame header/footer attribute (used to define header/footer in a sdw file)
class StarPAttributeFrameHF : public StarAttribute
{
public:
  //! constructor
  StarPAttributeFrameHF(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_active(true), m_format(), m_object(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeFrameHF(*this));
  }
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName;
    if (!m_active)
      o << "*,";
    o << ",";
  }

protected:
  //! copy constructor
  StarPAttributeFrameHF(StarPAttributeFrameHF const &orig) : StarAttribute(orig), m_active(orig.m_active), m_format(orig.m_format), m_object(orig.m_object)
  {
  }
  //! active flag
  bool m_active;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  //! the original object
  StarObject *m_object;
};

// ------------------------------------------------------------
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
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (!m_name.empty()) o << m_name.cstr();
    if (m_pageType>=0 && m_pageType<8) {
      char const *(wh[])= {"charUpper", "charLower", "romanUpper", "romanLower", "arabic", "none", "specialChar", "pageDesc"};
      o << "number=" << wh[m_pageType] << ",";
    }
    else
      o << "number[type]=" << m_pageType << ",";
    if (m_landscape) o << "landscape,";
    if (m_used>=0 && m_used<6) {
      char const *(wh[])= {"left", "right", "all", "mirror", "header[share]", "footer[share]"};
      o << wh[m_used] << ",";
    }
    else
      o << "used=" << m_used << ",";
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

//! a page descriptor attribute
class StarPAttributePageDesc : public StarAttribute
{
public:
  //! constructor
  StarPAttributePageDesc(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_auto(false), m_offset(0), m_name("")
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributePageDesc(*this));
  }
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << "=[";
    if (!m_auto) o << "auto[no],";
    if (m_offset) o << "offset=" << m_offset << ",";
    if (!m_name.empty()) o << "name=" << m_name.cstr() << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarPAttributePageDesc(StarPAttributePageDesc const &orig) : StarAttribute(orig), m_auto(orig.m_auto), m_offset(orig.m_offset), m_name(orig.m_name)
  {
  }
  //! the auto flag
  bool m_auto;
  //! the offset
  int m_offset;
  //! the page name
  librevenge::RVNGString m_name;
};

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
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
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
  virtual void printData(libstoff::DebugStream &o) const
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
  virtual void printData(libstoff::DebugStream &o) const
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
  //! add to a page
  virtual void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const;
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
  void printData(libstoff::DebugStream &o) const
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
  explicit StarPAttributeViewMode(StarPAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

void StarPAttributeColumns::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_FRM_COL) {
    if (!m_columnList.empty()) {
      librevenge::RVNGPropertyListVector columns;
      for (size_t c=0; c < m_columnList.size(); c++) {
        librevenge::RVNGPropertyList propList;
        if (m_columnList[c].addTo(propList))
          columns.append(propList);
      }
      state.m_section.m_propertyList.insert("style:columns", columns);
    }
  }
}

void StarPAttributePage::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type!=ATTR_SC_PAGE || state.m_pageZone!=STOFFPageSpan::Page)
    return;
  // todo: check correctly m_used
  if (m_used<0||m_used>=4) return;
  if (!m_name.empty()) state.m_page.m_propertiesList[0].insert("draw:name", m_name);
  state.m_page.m_propertiesList[0].insert("style:print-orientation", m_landscape ? "landscape" : "portrait");
  if (m_pageType>=0 && m_pageType<6) {
    char const *(wh[])= {"a", "A", "i", "I", "1", ""};
    state.m_page.m_propertiesList[0].insert("style:num-format", wh[m_pageType]);
  }
}

void StarPAttributeFrameHF::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (!m_active || !m_format)
    return;
  if (m_type==ATTR_FRM_HEADER || m_type==ATTR_FRM_FOOTER) {
    STOFFHeaderFooter hf;
    hf.m_subDocument[3].reset(new SubDocument(m_format, state.m_pool, &state.m_object));
    state.m_page.addHeaderFooter(m_type==ATTR_FRM_HEADER, "all", hf);
  }
  else {
    STOFF_DEBUG_MSG(("StarPAttributeFrameHF::addTo: unknown type\n"));
  }
}

void StarPAttributePageDesc::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (!m_name.empty()) {
    state.m_pageName=m_name;
    state.m_pageNameList.push_back(m_name);
  }
}

void StarPAttributePageHF::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
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
  state.m_page.addHeaderFooter(isHeader,wh, hf);
}

void StarPAttributeViewMode::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_SC_PAGE_CHARTS || m_type==ATTR_SC_PAGE_OBJECTS || m_type==ATTR_SC_PAGE_DRAWINGS) {
    if (state.m_pageZone==0 && m_value==0) {
      librevenge::RVNGString wh
      (m_type==ATTR_SC_PAGE_CHARTS ? "charts" :
       m_type==ATTR_SC_PAGE_OBJECTS ? "objects" : "drawings");
      if (state.m_page.m_propertiesList[0]["style:print"]) {
        librevenge::RVNGString res(state.m_page.m_propertiesList[0]["style:print"]->getStr());
        res.append(" ");
        res.append(wh);
        state.m_page.m_propertiesList[0].insert("style:print", res);
      }
      else
        state.m_page.m_propertiesList[0].insert("style:print", wh);
    }
  }
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
  m_pageType=int(input->readULong(1));
  *input >> m_landscape;
  m_used=int(input->readULong(2));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeColumns::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // sw_sw3attr.cxx SwFmtCol::Create
  m_lineAdj=int(input->readULong(1));
  *input >> m_ortho;
  m_lineHeight=int(input->readULong(1));
  m_gutterWidth=int(input->readULong(2));
  m_wishWidth=int(input->readULong(2));
  m_penStyle=int(input->readULong(1));
  m_penWidth=int(input->readULong(2));
  uint8_t color[3];
  for (int i=0; i<3; ++i) color[i]=uint8_t(input->readULong(2)>>8);
  m_penColor=STOFFColor(color[0],color[1],color[2]);
  int nCol=int(input->readULong(2));
  f << "N=" << nCol << ",";
  if (m_wishWidth==0)
    nCol=0;
  if (input->tell()+10*nCol>endPos) {
    STOFF_DEBUG_MSG(("StarPAttributeColumns::read: nCol is bad\n"));
    f << "###N,";
    nCol=0;
  }
  for (int i=0; i<nCol; ++i) {
    StarPAttributeColumns::Column col;
    col.m_wishWidth=int(input->readULong(2));
    for (int d=0; d<4; ++d) col.m_margins[d]=int(input->readULong(2));
    m_columnList.push_back(col);
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributeFrameHF::read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_object=&object;
  *input >> m_active;
  if (input->tell()<endPos) {
    shared_ptr<StarFormatManagerInternal::FormatDef> format;
    if (object.getFormatManager()->readSWFormatDef(zone,'r',format, object))
      m_format=format;
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarPAttributePageDesc::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // sw_sw3npool.cxx SwFmtPageDesc::Create
  if (nVers<1)
    *input >> m_auto;
  if (nVers< 2)
    m_offset=int(input->readULong(2));
  else {
    unsigned long nOff;
    if (!input->readCompressedULong(nOff)) {
      STOFF_DEBUG_MSG(("StarPAttributePageDesc::read: can not read nOff\n"));
      f << "###nOff,";
      printData(f);
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    m_offset=int(nOff);
  }
  int id=int(input->readULong(2));
  if (id!=0xFFFF && !zone.getPoolName(id, m_name)) {
    STOFF_DEBUG_MSG(("StarPAttributePageDesc::read: can not find the style name\n"));
    f << "###id=" << id << ",";
  }
  printData(f);
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
  printData(f);
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
    m_tableList.push_back(int(input->readULong(2)));
  printData(f);
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
    m_table=int(input->readULong(2));
    for (int i=0; i<4; ++i) dim[i]=int(input->readULong(2));
  }
  else {
    for (int i=0; i<4; ++i) dim[i]=int(input->readULong(2));
    if (vers>=2) {
      *input>>m_flags[0];
      if (input->tell()+1==endPos) // checkme
        *input>>m_flags[1];
    }
  }
  m_range=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}
}

namespace StarPageAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_HORCENTER,"page[horizontal,center]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_VERCENTER,"page[vertical,center]", false);
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_SCALE,"page[scale]",2,100);
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_SCALETOPAGES,"page[scaleToPage]",2,1);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_HEADERS,"page[headers]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_NOTES,"page[notes]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_GRID,"page[grid]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_FORMULAS,"page[formulas]", false);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_NULLVALS,"page[nullvals]", true);
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_FIRSTPAGENO,"page[first,pageNo]",2,1);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_TOPDOWN,"page[topdown]", true);

  map[StarAttribute::ATTR_FRM_PAGEDESC]=shared_ptr<StarAttribute>(new StarPAttributePageDesc(StarAttribute::ATTR_FRM_PAGEDESC, "pageDesc"));
  map[StarAttribute::ATTR_FRM_HEADER]=shared_ptr<StarAttribute>(new StarPAttributeFrameHF(StarAttribute::ATTR_FRM_HEADER, "header"));
  map[StarAttribute::ATTR_FRM_FOOTER]=shared_ptr<StarAttribute>(new StarPAttributeFrameHF(StarAttribute::ATTR_FRM_FOOTER, "footer"));
  map[StarAttribute::ATTR_FRM_COL]=shared_ptr<StarAttribute>(new StarPAttributeColumns(StarAttribute::ATTR_FRM_COL, "col"));
  map[StarAttribute::ATTR_SC_PAGE_SIZE]=shared_ptr<StarAttribute>(new StarPAttributeVec2i(StarAttribute::ATTR_SC_PAGE_SIZE, "page[size]", 4));

  map[StarAttribute::ATTR_SC_PAGE]=shared_ptr<StarAttribute>(new StarPAttributePage(StarAttribute::ATTR_SC_PAGE, "page"));
  map[StarAttribute::ATTR_SC_PAGE_HEADERLEFT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_HEADERLEFT, "header[left]"));
  map[StarAttribute::ATTR_SC_PAGE_HEADERRIGHT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_HEADERRIGHT, "header[right]"));
  map[StarAttribute::ATTR_SC_PAGE_FOOTERLEFT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_FOOTERLEFT, "footer[left]"));
  map[StarAttribute::ATTR_SC_PAGE_FOOTERRIGHT]=shared_ptr<StarAttribute>(new StarPAttributePageHF(StarAttribute::ATTR_SC_PAGE_FOOTERRIGHT, "footer[right]"));
  map[StarAttribute::ATTR_SC_PAGE_CHARTS]=shared_ptr<StarAttribute>(new StarPAttributeViewMode(StarAttribute::ATTR_SC_PAGE_CHARTS, "page[charts]"));
  map[StarAttribute::ATTR_SC_PAGE_OBJECTS]=shared_ptr<StarAttribute>(new StarPAttributeViewMode(StarAttribute::ATTR_SC_PAGE_OBJECTS, "page[objects]"));
  map[StarAttribute::ATTR_SC_PAGE_DRAWINGS]=shared_ptr<StarAttribute>(new StarPAttributeViewMode(StarAttribute::ATTR_SC_PAGE_DRAWINGS, "page[drawings]"));
  std::vector<STOFFVec2i> limits;
  limits.push_back(STOFFVec2i(142,142)); // BACKGROUND
  limits.push_back(STOFFVec2i(144,146)); // BORDER->SHADOW
  limits.push_back(STOFFVec2i(150,151)); // LRSPACE->ULSPACE
  limits.push_back(STOFFVec2i(155,155)); // PAGESIZE
  limits.push_back(STOFFVec2i(159,161)); // ON -> SHARED
  map[StarAttribute::ATTR_SC_PAGE_HEADERSET]=shared_ptr<StarAttribute>(new StarPAttributeItemSet(StarAttribute::ATTR_SC_PAGE_HEADERSET, "setPageHeader", limits));
  map[StarAttribute::ATTR_SC_PAGE_FOOTERSET]=shared_ptr<StarAttribute>(new StarPAttributeItemSet(StarAttribute::ATTR_SC_PAGE_FOOTERSET, "setPageFooter", limits));

  // TODO
  addAttributeUInt(map, StarAttribute::ATTR_SC_PAGE_PAPERTRAY,"page[papertray]",2,0);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_ON,"page[on]", true);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_DYNAMIC,"page[dynamic]", true);
  addAttributeBool(map, StarAttribute::ATTR_SC_PAGE_SHARED,"page[shared]", true);

  map[StarAttribute::ATTR_SC_PAGE_PRINTTABLES]=shared_ptr<StarAttribute>(new StarPAttributePrint(StarAttribute::ATTR_SC_PAGE_PRINTTABLES, "page[printtables]"));
  map[StarAttribute::ATTR_SC_PAGE_MAXSIZE]=shared_ptr<StarAttribute>(new StarPAttributeVec2i(StarAttribute::ATTR_SC_PAGE_MAXSIZE, "page[maxsize]", 4));
  map[StarAttribute::ATTR_SC_PAGE_PRINTAREA]=shared_ptr<StarAttribute>(new StarPAttributeRangeItem(StarAttribute::ATTR_SC_PAGE_PRINTAREA, "page[printArea]"));
  map[StarAttribute::ATTR_SC_PAGE_REPEATCOL]=shared_ptr<StarAttribute>(new StarPAttributeRangeItem(StarAttribute::ATTR_SC_PAGE_REPEATCOL, "page[repeatCol]"));
  map[StarAttribute::ATTR_SC_PAGE_REPEATROW]=shared_ptr<StarAttribute>(new StarPAttributeRangeItem(StarAttribute::ATTR_SC_PAGE_REPEATROW, "page[repeatRow]"));
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
