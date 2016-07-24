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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFOLEParser.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFParagraph.hxx"
#include "STOFFSection.hxx"
#include "STOFFTextListener.hxx"

#include "SWFieldManager.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarGraphicStruct.hxx"
#include "StarItemPool.hxx"
#include "StarLayout.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectModel.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarTable.hxx"
#include "StarWriterStruct.hxx"
#include "StarZone.hxx"

#include "StarObjectText.hxx"

/** Internal: the structures of a StarObjectText */
namespace StarObjectTextInternal
{
////////////////////////////////////////
// Zone, Content function
Zone::~Zone()
{
}

Content::~Content()
{
}

bool Content::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Content::send: call without listener\n"));
    return false;
  }
  for (size_t t=0; t<m_zoneList.size(); ++t) {
    if (m_zoneList[t])
      m_zoneList[t]->send(listener, pool, object);
    if (t+1!=m_zoneList.size())
      listener->insertEOL();
  }
  return true;
}

////////////////////////////////////////
//! Internal: a formatZone of StarObjectTextInteral
struct FormatZone : public Zone {
  //! constructor
  FormatZone(shared_ptr<StarFormatManagerInternal::FormatDef> format) : Zone(), m_format(format)
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

bool FormatZone::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::FormatZone::send: call without listener\n"));
    return false;
  }
  if (!m_format) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::FormatZone::send: can not find the format\n"));
    return false;
  }
  return m_format->send(listener, pool, object);
}

////////////////////////////////////////
//! Internal: a graphZone of StarObjectTextInteral
struct GraphZone : public Zone {
  //! constructor
  GraphZone(shared_ptr<STOFFOLEParser> oleParser) : Zone(), m_oleParser(oleParser), m_attributeList(), m_contour()
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const */*pool*/, StarObject &object) const;
  //! the ole parser
  shared_ptr<STOFFOLEParser> m_oleParser;
  //! the graph name, the fltName, the replace text
  librevenge::RVNGString m_names[3];
  //! the attributes list
  std::vector<StarWriterStruct::Attribute> m_attributeList;
  //! the contour
  StarGraphicStruct::StarPolygon m_contour;
};

bool GraphZone::send(STOFFListenerPtr listener, StarItemPool const */*pool*/, StarObject &/*object*/) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::GraphZone::send: call without listener\n"));
    return false;
  }
  if (m_names[0].empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::GraphZone::send: can not find the graph name\n"));
    return false;
  }
  STOFFEmbeddedObject localPicture;
  if (!m_oleParser || !StarFileManager::readEmbeddedPicture(m_oleParser, m_names[0].cstr(), localPicture) || localPicture.isEmpty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal: sorry, can not find object %s\n", m_names[0].cstr()));
    return false;
  }
  STOFFPosition position;
  position.setAnchor(STOFFPosition::Paragraph);
  //position.setOrigin(STOFFVec2i(0,0), librevenge::RVNG_POINT);
  position.setSize(STOFFVec2i(100,100), librevenge::RVNG_POINT);
  STOFFGraphicStyle style;
  //updateStyle(style, object, listener);
  listener->insertPicture(position, localPicture, style);
  return true;
}

////////////////////////////////////////
//! Internal: a sectionZone of StarObjectTextInteral
struct SectionZone : public Zone {
  //! constructor
  SectionZone() : Zone(), m_name(""), m_condition(""), m_linkName(""), m_type(0), m_flags(0), m_format(), m_content()
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const;
  //! the section name
  librevenge::RVNGString m_name;
  //! the section condition
  librevenge::RVNGString m_condition;
  //! the section link name
  librevenge::RVNGString m_linkName;
  //! the section type
  int m_type;
  //! the section flag
  int m_flags;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  //! the content
  shared_ptr<Content> m_content;
};

bool SectionZone::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::FormatZone::send: call without listener\n"));
    return false;
  }
  STOFFSection section;
  if (!m_name.empty())
    section.m_propertyList.insert("text:name", m_name);
  if (listener->isSectionOpened())
    listener->closeSection();
  if (listener->canOpenSectionAddBreak())
    listener->openSection(section);
  if (m_content)
    m_content->send(listener, pool, object);
  else {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::FormatZone::send: call without content\n"));
  }
  return true;
}

////////////////////////////////////////
//! Internal: a textZone of StarObjectTextInteral
struct TextZone : public Zone {
  //! constructor
  TextZone() : Zone(), m_text(), m_textSourcePosition(), m_styleName(""), m_charAttributeList(), m_format(), m_markList()
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const;
  //! the text
  std::vector<uint32_t> m_text;
  //! the text initial position
  std::vector<size_t> m_textSourcePosition;
  //! the style name
  librevenge::RVNGString m_styleName;
  //! the character item list
  std::vector<StarWriterStruct::Attribute> m_charAttributeList;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  //! the mark
  std::vector<StarWriterStruct::Mark> m_markList;
};

bool TextZone::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: call without listener\n"));
    return false;
  }

  std::map<int, shared_ptr<StarItem> >::const_iterator it;
  STOFFFont mainFont;
  STOFFParagraph para;
  if (pool && !m_styleName.empty()) { // checkme
    StarItemStyle const *style=pool->findStyleWithFamily(m_styleName, StarItemStyle::F_Paragraph);
    if (style) {
#if 0
      bool done=false;
      if (!style->m_names[0].empty()) {
        if (listener) pool->defineParagraphStyle(listener, style->m_names[0]);
        para.m_propertyList.insert("librevenge:parent-display-name", style->m_names[0]);
        done=true;
      }
#endif
      for (it=style->m_itemSet.m_whichToItemMap.begin(); it!=style->m_itemSet.m_whichToItemMap.end(); ++it) {
        if (it->second && it->second->m_attribute) {
          it->second->m_attribute->addTo(mainFont, pool);
#if 0
          if (!done)
#endif
            it->second->m_attribute->addTo(para, pool);
        }
      }
#if 0
      std::cerr << "Para:" << style->m_itemSet.printChild() << "\n";
#endif
    }
  }
  listener->setFont(mainFont);
  listener->setParagraph(para);
  if (m_format)
    m_format->send(listener, pool, object);
  if (!m_markList.empty()) {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: sorry mark are not implemented\n"));
      first=false;
    }
  }
  if (mainFont.m_footnote) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a footnote in mainFont\n"));
    mainFont.m_footnote=false;
  }
  if (mainFont.m_field) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a field in mainFont\n"));
    mainFont.m_field.reset();
  }
  if (!mainFont.m_link.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a link in mainFont\n"));
    mainFont.m_link.clear();
  }
  if (!mainFont.m_refMark.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a refMark in mainFont\n"));
    mainFont.m_refMark.clear();
  }
  std::set<size_t> modPosSet;
  size_t numFonts=m_charAttributeList.size();
  modPosSet.insert(0);
  for (size_t i=0; i<numFonts; ++i) {
    if (m_charAttributeList[i].m_position[0]>0)
      modPosSet.insert(size_t(m_charAttributeList[i].m_position[0]));
    if (m_charAttributeList[i].m_position[1]>0)
      modPosSet.insert(size_t(m_charAttributeList[i].m_position[1]));
  }
  std::set<size_t>::const_iterator posSetIt=modPosSet.begin();
  int endLinkPos=-1, endRefMarkPos=-1;
  librevenge::RVNGString refMarkString;
  for (size_t c=0; c<m_text.size(); ++c) {
    bool fontChange=false;
    size_t srcPos=c<m_textSourcePosition.size() ? m_textSourcePosition[c] : 10000;
    while (posSetIt!=modPosSet.end() && *posSetIt <= srcPos) {
      ++posSetIt;
      fontChange=true;
    }
    shared_ptr<StarAttribute> footnote;
    shared_ptr<SWFieldManagerInternal::Field> field;
    librevenge::RVNGString linkString;
    bool startRefMark=false;

    if (fontChange) {
      STOFFFont font(mainFont);
      for (size_t f=0; f<numFonts; ++f) {
        StarWriterStruct::Attribute const &attrib=m_charAttributeList[f];
        if ((attrib.m_position[1]<0 && attrib.m_position[0]>=0 && attrib.m_position[0]!=int(srcPos)) ||
            (attrib.m_position[0]>=0 && attrib.m_position[0]>int(srcPos)) ||
            (attrib.m_position[1]>=0 && attrib.m_position[1]<=int(srcPos)))
          continue;
        if (!attrib.m_attribute)
          continue;
        attrib.m_attribute->addTo(font, pool);
        if (!footnote && font.m_footnote)
          footnote=attrib.m_attribute;
        if (c==0)
          attrib.m_attribute->addTo(para, pool);
        if (font.m_field) {
          if (int(srcPos)==attrib.m_position[0])
            field=font.m_field;
          font.m_field.reset();
        }
        if (!font.m_link.empty()) {
          if (endLinkPos<0) {
            linkString=font.m_link;
            endLinkPos=int(attrib.m_position[1]<0 ? attrib.m_position[0] : attrib.m_position[1]);
          }
          font.m_link.clear();
        }
        if (!font.m_refMark.empty()) {
          if (endRefMarkPos<0) {
            refMarkString=font.m_refMark;
            endRefMarkPos=int(attrib.m_position[1]<0 ? attrib.m_position[0] : attrib.m_position[1]);
            startRefMark=true;
          }
          else {
            STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: multiple refmark is not implemented\n"));
          }
          font.m_refMark.clear();
        }
      }
      listener->setFont(font);
      if (c==0)
        listener->setParagraph(para);
      static bool first=true;
      if (font.m_content) {
        first=false;
        STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find unexpected content zone\n"));
      }
    }
    if (!linkString.empty()) {
      STOFFLink link;
      link.m_HRef=linkString.cstr();
      listener->openLink(link);
    }
    if (!refMarkString.empty() && startRefMark) {
      STOFFField cField;
      cField.m_propertyList.insert("librevenge:field-type", "text:reference-mark-start");
      cField.m_propertyList.insert("text:name", refMarkString);
      listener->insertField(cField);
    }
    if (endLinkPos>=0 && int(c)==endLinkPos) {
      listener->closeLink();
      endLinkPos=-1;
    }
    if (endRefMarkPos>=0 && int(c)==endRefMarkPos) {
      STOFFField cField;
      cField.m_propertyList.insert("librevenge:field-type", "text:reference-mark-end");
      cField.m_propertyList.insert("text:name", refMarkString);
      listener->insertField(cField);
      endRefMarkPos=-1;
    }
    if (footnote)
      footnote->send(listener, pool, object);
    else if (field)
      field->send(listener, pool, object);
    else if (m_text[c]==0x9)
      listener->insertTab();
    else if (m_text[c]==0xa)
      listener->insertEOL(true);
    else
      listener->insertUnicode(m_text[c]);
  }
  if (endLinkPos>=0) // check that not link is opened
    listener->closeLink();
  if (endRefMarkPos>=0) { // check that not refMark is opened
    STOFFField cField;
    cField.m_propertyList.insert("librevenge:field-type", "text:reference-mark-end");
    cField.m_propertyList.insert("text:name", refMarkString);
    listener->insertField(cField);
  }
  return true;
}

//! Internal: a table of StarObjectTextInteral
struct Table : public Zone {
  //! constructor
  Table() : Zone(), m_table()
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const;
  //! the table
  shared_ptr<StarTable> m_table;
};

bool Table::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Table::send: call without listener\n"));
    return false;
  }
  if (!m_table) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Table::send: can not find the table\n"));
    return false;
  }
  return m_table->send(listener, pool, object);
}

////////////////////////////////////////
//! Internal: the state of a StarObjectText
struct State {
  //! constructor
  State() : m_numPages(0), m_mainContent(), m_model()
  {
  }
  //! the number of pages
  int m_numPages;
  //! the main content
  shared_ptr<Content> m_mainContent;
  //! the drawing model
  shared_ptr<StarObjectModel> m_model;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectText::StarObjectText(StarObject const &orig, bool duplicateState) : StarObject(orig, duplicateState), m_textState(new StarObjectTextInternal::State)
{
}

StarObjectText::~StarObjectText()
{
  cleanPools();
}

////////////////////////////////////////////////////////////
// send the data
////////////////////////////////////////////////////////////
bool StarObjectText::updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages) const
{
  STOFF_DEBUG_MSG(("StarObjectText::updatePageSpans: not implemented\n"));
  numPages=0;
  if (m_textState->m_model) {
    std::vector<STOFFPageSpan> modelPageSpan;
    m_textState->m_model->updatePageSpans(modelPageSpan, numPages);
  }
  if (numPages<=0)
    numPages=1;
  STOFFPageSpan ps;
  ps.m_pageSpan=numPages;
  pageSpan.clear();
  pageSpan.push_back(ps);
  m_textState->m_numPages=numPages;
  return numPages>0;
}

bool StarObjectText::sendPages(STOFFTextListenerPtr listener)
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectText::sendPages: can not find the listener\n"));
    return false;
  }
  if (!m_textState->m_mainContent) {
    STOFF_DEBUG_MSG(("StarObjectText::sendPages: can not find any content\n"));
    return true;
  }
  if (m_textState->m_model) {
    for (int i=0; i<m_textState->m_numPages; ++i)
      m_textState->m_model->sendPage(i, listener);
  }
  shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_WriterPool, false);
  m_textState->m_mainContent->send(listener, pool.get(), *this);
  return true;
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
bool StarObjectText::parse()
{
  if (!getOLEDirectory() || !getOLEDirectory()->m_input) {
    STOFF_DEBUG_MSG(("StarObjectText::parser: error, incomplete document\n"));
    return false;
  }
  STOFFOLEParser::OleDirectory &directory=*getOLEDirectory();
  StarObject::parse();
  std::vector<std::string> unparsedOLEs=directory.getUnparsedOles();
  size_t numUnparsed = unparsedOLEs.size();
  STOFFInputStreamPtr input=directory.m_input;
  StarFileManager fileManager;
  STOFFInputStreamPtr mainOle; // let store the StarWriterDocument to read it in last position
  std::string mainName;
  for (size_t i = 0; i < numUnparsed; i++) {
    std::string const &name = unparsedOLEs[i];
    STOFFInputStreamPtr ole = input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObjectText::parse: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    std::string::size_type pos = name.find_last_of('/');
    std::string base;
    if (pos == std::string::npos) base = name;
    else if (pos == 0) base = name.substr(1);
    else
      base = name.substr(pos+1);
    ole->setReadInverted(true);
    if (base=="SwNumRules") {
      readSwNumRuleList(ole, name);
      continue;
    }
    if (base=="SwPageStyleSheets") {
      readSwPageStyleSheets(ole,name);
      continue;
    }

    if (base=="DrawingLayer") {
      readDrawingLayer(ole,name);
      continue;
    }
    if (base=="SfxStyleSheets") {
      readSfxStyleSheets(ole,name);
      continue;
    }
    if (base=="StarWriterDocument") {
      mainOle=ole;
      mainName=name;
      continue;
    }
    if (base!="BasicManager2") {
      STOFF_DEBUG_MSG(("StarObjectText::parse: find unexpected ole %s\n", name.c_str()));
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    libstoff::DebugStream f;
    f << "Entries(" << base << "):";
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    asciiFile.reset();
  }
  if (!mainOle) {
    STOFF_DEBUG_MSG(("StarObjectText::parser: can not find the main writer document\n"));
    return false;
  }
  else
    readWriterDocument(mainOle,mainName);
  return true;
}

bool StarObjectText::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  if (getDocumentKind()!=STOFFDocument::STOFF_K_TEXT) {
    STOFF_DEBUG_MSG(("StarObjectChart::readSfxStyleSheets: called with unexpected document\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SfxStyleSheets)");
    return false;
  }
  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  shared_ptr<StarItemPool> pool=getNewItemPool(StarItemPool::T_WriterPool);
  shared_ptr<StarItemPool> mainPool=pool;
  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: create extra pool of type %d\n", int(pool->getType())));
      }
      if (!mainPool) mainPool=pool;
      pool.reset();
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->isEnd()) return true;
  long pos=input->tell();
  if (!mainPool || !mainPool->readStyles(zone, *this)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: can not read a style pool\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SfxStyleSheets):###extra");
  }
  return true;
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////
bool StarObjectText::readSwNumRuleList(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWNumRuleList", getPassword());
  if (!zone.readSWHeader()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSwNumRuleList: can not read the header\n"));
    return false;
  }
  zone.readStringsPool();
  // sw_sw3num.cxx::Sw3IoImp::InNumRules
  libstoff::DebugFile &ascFile=zone.ascii();
  while (!input->isEnd()) {
    long pos=input->tell();
    int rType=input->peek();
    if ((rType=='0' || rType=='R') && readSWNumRule(zone, char(rType)))
      continue;
    char type;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWNumRuleList[" << type << "]:";
    bool done=false;
    switch (type) {
    case '+': { // extra outline
      zone.openFlagZone();
      int N=int(input->readULong(1));
      f << "N=" << N << ",";
      zone.closeFlagZone();
      if (input->tell()+3*N>zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("StarObjectText::readSwNumRuleList: nExtraOutline seems bad\n"));
        f << "###,";
        break;
      }
      f << "levSpace=[";
      for (int i=0; i<N; ++i)
        f << input->readULong(1) << ":" << input->readULong(2) << ",";
      f << "],";
      break;
    }
    case '?':
      STOFF_DEBUG_MSG(("StarObjectText::readSwNumRuleList: reading inHugeRecord(TEST_HUGE_DOCS) is not implemented\n"));
      break;
    case 'Z':
      done=true;
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSwNumRuleList: find unimplemented type\n"));
      f << "###type,";
      break;
    }
    if (!zone.closeSWRecord(type, "SWNumRuleList")) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (done)
      break;
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSwNumRuleList: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWNumRuleList:###extra");
  }
  return true;
}
catch (...)
{
  return false;
}

bool StarObjectText::readSwPageStyleSheets(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWPageStyleSheets", getPassword());
  if (!zone.readSWHeader()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSwPageStyleSheets: can not read the header\n"));
    return false;
  }
  zone.readStringsPool();
  SWFieldManager fieldManager;
  while (fieldManager.readField(zone,'Y'))
    ;
  std::vector<StarWriterStruct::Bookmark> markList;
  StarWriterStruct::Bookmark::readList(zone, markList);
  std::vector<std::vector<StarWriterStruct::Redline> > redlineListList;
  StarWriterStruct::Redline::readListList(zone, redlineListList);
  getFormatManager()->readSWNumberFormatterList(zone);
  // sw_sw3page.cxx Sw3IoImp::InPageDesc
  libstoff::DebugFile &ascFile=zone.ascii();
  while (!input->isEnd()) {
    long pos=input->tell();
    char type;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWPageStyleSheets[" << type << "]:";
    bool done=false;
    switch (type) {
    case 'P': {
      zone.openFlagZone();
      int N=int(input->readULong(2));
      f << "N=" << N << ",";
      zone.closeFlagZone();
      for (int i=0; i<N; ++i) {
        StarWriterStruct::PageDesc desc;
        // read will check that we can read the data, ....
        if (!desc.read(zone, *this))
          break;
      }
      break;
    }
    case 'Z':
      done=true;
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSwPageStyleSheets: find unknown data\n"));
      f << "###";
      break;
    }
    if (!zone.closeSWRecord(type, "SWPageStyleSheets")) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (done)
      break;
  }

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSwPageStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWPageStyleSheets:##extra");
  }

  return true;
}
catch (...)
{
  return false;
}

bool StarObjectText::readSWContent(StarZone &zone, shared_ptr<StarObjectTextInternal::Content> &content)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='N' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3sectn.cxx: InContents
  if (content) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWContent: oops, the content zone is already created\n"));
  }
  else
    content.reset(new StarObjectTextInternal::Content);
  libstoff::DebugStream f;
  f << "Entries(SWContent)[" << zone.getRecordLevel() << "]:";
  if (zone.isCompatibleWith(5))
    zone.openFlagZone();
  int nNodes;
  if (zone.isCompatibleWith(0x201))
    nNodes=int(input->readULong(4));
  else {
    if (zone.isCompatibleWith(5))
      f << "sectId=" << input->readULong(2) << ",";
    nNodes=int(input->readULong(2));
  }
  f << "N=" << nNodes << ",";
  if (zone.isCompatibleWith(5))
    zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<nNodes; ++i) {
    if (input->tell()>=lastPos) break;
    pos=input->tell();
    int cType=input->peek();
    bool done=false;
    switch (cType) {
    case 'E': {
      shared_ptr<StarTable> table(new StarTable);
      done=table->read(zone, *this);
      if (done) {
        shared_ptr<StarObjectTextInternal::Table> tableZone(new StarObjectTextInternal::Table);
        tableZone->m_table=table;
        content->m_zoneList.push_back(tableZone);
      }
      break;
    }
    case 'G': {
      shared_ptr<StarObjectTextInternal::GraphZone> graph;
      done=readSWGraphNode(zone, graph);
      if (done && graph)
        content->m_zoneList.push_back(graph);
      break;
    }
    case 'I': {
      shared_ptr<StarObjectTextInternal::SectionZone> section;
      done=readSWSection(zone, section);
      if (done && section)
        content->m_zoneList.push_back(section);
      break;
    }
    case 'O':
      done=readSWOLENode(zone);
      break;
    case 'T': {
      shared_ptr<StarObjectTextInternal::TextZone> text;
      done=readSWTextZone(zone, text);
      if (done && text)
        content->m_zoneList.push_back(text);
      break;
    }
    case 'l': // related to link
    case 'o': { // format: safe to ignore
      shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=getFormatManager()->readSWFormatDef(zone,char(cType),format, *this);
      if (done && format) {
        shared_ptr<StarObjectTextInternal::FormatZone> formatZone;
        formatZone.reset(new StarObjectTextInternal::FormatZone(format));
        content->m_zoneList.push_back(formatZone);
      }
      break;
    }
    case 'v': {
      StarWriterStruct::NodeRedline redline;
      done=redline.read(zone);
      break;
    }
    default:
      break;
    }
    if (done) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWContent[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (cType) {
    case 'i':
      // sw_sw3node.cxx InRepTxtNode
      f << "repTxtNode,";
      f << "rep=" << input->readULong(4) << ",";
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSWContent: find unexpected type\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWContent");
  }
  zone.closeSWRecord('N', "SWContent");
  return true;
}

bool StarObjectText::readSWGraphNode(StarZone &zone, shared_ptr<StarObjectTextInternal::GraphZone> &graphZone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='G' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InGrfNode
  libstoff::DebugStream f;
  f << "Entries(SWGraphNode)[" << zone.getRecordLevel() << "]:";
  graphZone.reset(new StarObjectTextInternal::GraphZone(m_oleParser));
  std::vector<uint32_t> text;
  int fl=zone.openFlagZone();
  if (fl&0x10) f << "link,";
  if (fl&0x20) f << "empty,";
  if (fl&0x40) f << "serverMap,";
  zone.closeFlagZone();
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('G', "SWGraphNode");
      return true;
    }
    if (!text.empty()) {
      graphZone->m_names[i]=libstoff::getString(text);
      f << (i==0 ? "grfName" : "fltName") << "=" << graphZone->m_names[i].cstr() << ",";
    }
  }
  if (zone.isCompatibleWith(0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: can not read a objName\n"));
      f << "###textRepl";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('G', "SWGraphNode");
      return true;
    }
    if (!text.empty()) {
      graphZone->m_names[2]=libstoff::getString(text);
      f << "textRepl=" << graphZone->m_names[2].cstr() << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  while (input->tell() < lastPos) {
    pos=input->tell();
    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'S':
      done=StarWriterStruct::Attribute::readList(zone, graphZone->m_attributeList, *this);
      break;
    case 'X': // store me
      done=readSWImageMap(zone);
      break;
    default:
      break;
    }
    if (done)
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWGraphNode[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case 'k': {
      // sw_sw3nodes.cxx InContour
      int polyFl=zone.openFlagZone();
      zone.closeFlagZone();
      if (polyFl&0x10) {
        // poly2.cxx operator>>
        int numPoly=int(input->readULong(2));
        for (int i=0; i<numPoly; ++i) {
          f << "poly" << i << "=[";
          // poly.cxx operator>>
          int numPoints=int(input->readULong(2));
          if (input->tell()+8*numPoints>lastPos) {
            STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: can not read a polygon\n"));
            f << "###poly";
            break;
          }
          for (int p=0; p<numPoints; ++p) {
            int dim[2];
            for (int j=0; j<2; ++j) dim[j]=int(input->readLong(4));
            graphZone->m_contour.m_points.push_back(StarGraphicStruct::StarPolygon::Point(STOFFVec2i(dim[0],dim[1])));
            f << STOFFVec2i(dim[0],dim[1]) << ",";
          }
          f << "],";
        }
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: find unexpected type\n"));
      f << "###";
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWGraphNode");
  }

  zone.closeSWRecord('G', "SWGraphNode");
  return true;
}

bool StarObjectText::readSWImageMap(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='X' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugStream f;
  f << "Entries(SWImageMap)[" << zone.getRecordLevel() << "]:";
  // sw_sw3nodes.cxx InImageMap
  int flag=zone.openFlagZone();
  if (flag&0xF0) f << "fl=" << flag << ",";
  zone.closeFlagZone();
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: can not read url\n"));
    f << "###url";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    zone.closeSWRecord('X', "SWImageMap");
    return true;
  }
  if (!string.empty())
    f << "url=" << libstoff::getString(string).cstr() << ",";
  if (zone.isCompatibleWith(0x11,0x22, 0x101)) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: can not read string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        zone.closeSWRecord('X', "SWImageMap");
        return true;
      }
      if (string.empty()) continue;
      f << (i==0 ? "target" : "dummy") << "=" << libstoff::getString(string).cstr() << ",";
    }
  }
  if (flag&0x20) {
    // svt_imap.cxx: ImageMap::Read
    std::string cMagic("");
    for (int i=0; i<6; ++i) cMagic+=char(input->readULong(1));
    if (cMagic!="SDIMAP") {
      STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: cMagic is bad\n"));
      f << "###cMagic=" << cMagic << ",";
    }
    else {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: can not read string\n"));
          f << "###string";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());

          zone.closeSWRecord('X', "SWImageMap");
          return true;
        }
        if (!string.empty())
          f << (i==0 ? "target" : i==1 ? "dummy1" : "dummy2") << "=" << libstoff::getString(string).cstr() << ",";
        if (i==1)
          f << "nCount=" << input->readULong(2) << ",";
      }
      if (input->tell()<zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: find imapCompat data, not implemented\n"));
        // svt_imap3.cxx IMapCompat::IMapCompat
        ascFile.addPos(input->tell());
        ascFile.addNote("SWImageMap:###IMapCompat");
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      }
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('X', "SWImageMap");
  return true;
}

bool StarObjectText::readSWJobSetUp(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='J' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugStream f;
  zone.openFlagZone();
  zone.closeFlagZone();
  if (input->tell()==zone.getRecordLastPosition()) // empty
    f << "Entries(JobSetUp)[" << zone.getRecordLevel() << "]:";
  else {
    f << "JobSetUp[container-" << zone.getRecordLevel() << "]:";
    StarFileManager fileManager;
    fileManager.readJobSetUp(zone, false);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSWRecord(type, "JobSetUp[container]");
  return true;
}
bool StarObjectText::readSWNumRule(StarZone &zone, char cKind)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!=cKind || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx::Sw3IoImp::InNumRule
  libstoff::DebugStream f;
  f << "Entries(SWNumRuleDef)[" << cKind << "-" << zone.getRecordLevel() << "]:";
  int val;
  if (zone.isCompatibleWith(0x201)) {
    int cFlags=int(zone.openFlagZone());
    int nStringId=int(input->readULong(2));
    librevenge::RVNGString poolName;
    if (nStringId==0xFFFF)
      ;
    else if (!zone.getPoolName(nStringId, poolName))
      f << "###nStringId=" << nStringId << ",";
    else if (!poolName.empty())
      f << poolName.cstr() << ",";
    if (cFlags&0x10) {
      int nPoolId=int(input->readULong(2));
      f << "PoolId=" << nPoolId << ",";
      val=int(input->readULong(2));
      if (val) f << "poolHelpId=" << val << ",";
      val=int(input->readULong(1));
      if (val) f << "poolHelpFileId=" << val << ",";
    }
  }
  val=int(input->readULong(1));
  if (val) f << "eType=" << val << ",";
  if (zone.isCompatibleWith(0x201))
    zone.closeFlagZone();
  int nFormat=int(input->readULong(1));
  long lastPos=zone.getRecordLastPosition();
  f << "nFormat=" << nFormat << ",";
  if (input->tell()+nFormat>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectText::readSwNumRule: nFormat seems bad\n"));
    f << "###,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(cKind, "SWNumRuleDef");
    return true;
  }
  f << "lvl=[";
  for (int i=0; i<nFormat; ++i) f  << input->readULong(1) << ",";
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  int nKnownFormat=nFormat>10 ? 10 : nFormat;
  for (int i=0; i<nKnownFormat; ++i) {
    pos=input->tell();
    if (getFormatManager()->readSWNumberFormat(zone)) continue;
    STOFF_DEBUG_MSG(("StarObjectText::readSwNumRule: can not read a format\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSWRecord(cKind, "SWNumRuleDef");
  return true;
}

bool StarObjectText::readSWOLENode(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='O' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InOLENode
  libstoff::DebugStream f;
  f << "Entries(SWOLENode)[" << zone.getRecordLevel() << "]:";

  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWOLENode: can not read a objName\n"));
    f << "###objName";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord('O', "SWOLENode");
    return true;
  }
  if (!text.empty())
    f << "objName=" << libstoff::getString(text).cstr() << ",";
  if (zone.isCompatibleWith(0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWOLENode: can not read a objName\n"));
      f << "###textRepl";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('O', "SWOLENode");
      return true;
    }
    if (!text.empty())
      f << "textRepl=" << libstoff::getString(text).cstr() << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('O', "SWOLENode");
  return true;
}

bool StarObjectText::readSWSection(StarZone &zone, shared_ptr<StarObjectTextInternal::SectionZone> &section)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='I' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3sectn.cxx: InSection
  libstoff::DebugStream f;
  f << "Entries(SWSection)[" << zone.getRecordLevel() << "]:";
  section.reset(new StarObjectTextInternal::SectionZone);
  std::vector<uint32_t> text;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWSection: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('I', "SWSection");
      return true;
    }
    if (text.empty()) continue;
    if (i==0)
      section->m_name=libstoff::getString(text);
    else
      section->m_condition=libstoff::getString(text);
    f << (i==0 ? "name" : "cond") << "=" << libstoff::getString(text).cstr() << ",";
  }
  int fl=section->m_flags=zone.openFlagZone();
  if (fl&0x10) f << "hidden,";
  if (fl&0x20) f << "protect,";
  if (fl&0x40) f << "condHidden,";
  if (fl&0x40) f << "connectFlag,";
  section->m_type=int(input->readULong(2));
  if (section->m_type) f << "nType=" << section->m_type << ",";
  zone.closeFlagZone();
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    long actPos=input->tell();
    int c=input->peek();
    bool done=false;
    switch (c) { // checkme: unsure which data can be found here
    case 's':
      done=getFormatManager()->readSWFormatDef(zone,'s', section->m_format, *this);
      break;
    case 'N':
      done=readSWContent(zone, section->m_content);
      break;
    default:
      break;
    }
    if (!done || input->tell()<=actPos || input->tell()>lastPos) {
      input->seek(actPos, librevenge::RVNG_SEEK_SET);
      break;
    }
  }
  if (zone.isCompatibleWith(0xd)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWSection: can not read a linkName\n"));
      f << "###linkName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('I', "SWSection");
      return true;
    }
    else if (!text.empty()) {
      section->m_linkName=libstoff::getString(text);
      f << "linkName=" << section->m_linkName.cstr() << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('I', "SWSection");
  return true;
}

bool StarObjectText::readSWTextZone(StarZone &zone, shared_ptr<StarObjectTextInternal::TextZone> &textZone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='T' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InTxtNode
  libstoff::DebugStream f;
  f << "Entries(SWText)[" << zone.getRecordLevel() << "]:";
  textZone.reset(new StarObjectTextInternal::TextZone);
  int fl=zone.openFlagZone();
  int poolId=int(input->readULong(2));
  librevenge::RVNGString poolName;
  if (!zone.getPoolName(poolId, poolName))
    f << "###nPoolId=" << poolId << ",";
  else
    f << poolName.cstr() << ",";
  if (fl&0x10 && !zone.isCompatibleWith(0x201)) {
    int val=int(input->readULong(1));
    if (val==200 && zone.isCompatibleWith(0xf,0x101) && input->tell() < zone.getFlagLastPosition())
      val=int(input->readULong(1));
    if (val)
      f << "nLevel=" << val << ",";
  }
  if (zone.isCompatibleWith(0x19,0x22, 0x101))
    f << "nCondColl=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (!zone.readString(textZone->m_text, textZone->m_textSourcePosition, -1, true)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWTextZone: can not read main text\n"));
    f << "###text";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord('T', "SWText");
    return true;
  }
  else if (!textZone->m_text.empty())
    f << libstoff::getString(textZone->m_text).cstr();

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  std::vector<shared_ptr<StarAttribute> > attributeList;
  std::vector<STOFFVec2i> limitsList;
  while (input->tell()<lastPos) {
    pos=input->tell();

    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'A': {
      StarWriterStruct::Attribute attrib;
      done=attrib.read(zone, *this);
      if (done)
        textZone->m_charAttributeList.push_back(attrib);
      break;
    }
    case 'K': {
      StarWriterStruct::Mark mark;
      done=mark.read(zone);
      if (done) textZone->m_markList.push_back(mark);
      break;
    }
    case 'R':
      done=readSWNumRule(zone,'R');
      break;
    case 'S':
      done=StarWriterStruct::Attribute::readList(zone, textZone->m_charAttributeList, *this);
      break;
    case 'l': // related to link
      done=getFormatManager()->readSWFormatDef(zone,'l', textZone->m_format, *this);
      break;
    case 'o': { // format: safe to ignore
      shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=getFormatManager()->readSWFormatDef(zone,'o', format, *this);
      break;
    }
    case 'v': {
      StarWriterStruct::NodeRedline redline;
      done=redline.read(zone);
      break;
    }
    default:
      break;
    }
    if (done)
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWText[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case '3': {
      // sw_sw3num InNodeNum
      f << "nodeNum,";
      int cFlag=zone.openFlagZone();
      int nLevel=int(input->readULong(1));
      if (nLevel!=201)
        f << "nLevel=" << nLevel<< ",";
      if (cFlag&0x20) f << "nSetValue=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      if (nLevel!=201) {
        int N=int(zone.getRecordLastPosition()-input->tell())/2;
        f << "level=[";
        for (int i=0; i<N; ++i)
          f << input->readULong(2) << ",";
        f << "],";
      }
      break;
    }
    case 'w': { // wrong list
      // sw_sw3nodes.cxx in text node
      f << "wrongList,";
      int cFlag=zone.openFlagZone();
      if (cFlag&0xf0) f << "flag=" << (cFlag>>4) << ",";
      f << "nBeginInv=" << input->readULong(2) << ",";
      f << "nEndInc=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      int N =int(input->readULong(2));
      if (input->tell()+4*N>zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWTextZone: find bad count\n"));
        f << "###N=" << N << ",";
        break;
      }
      f << "nWrong=[";
      for (int i=0; i<N; ++i) f << input->readULong(4) << ",";
      f << "],";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSWTextZone: find unexpected type\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWText");
  }
  zone.closeSWRecord('T', "SWText");
  return true;
}

////////////////////////////////////////////////////////////
// drawing layer
////////////////////////////////////////////////////////////
bool StarObjectText::readDrawingLayer(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "DrawingLayer", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);
  // sw_sw3imp.cxx Sw3IoImp::LoadDrawingLayer

  // create this pool from the main's SWG pool
  shared_ptr<StarItemPool> pool=getNewItemPool(StarItemPool::T_XOutdevPool);
  pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));

  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: create extra pool for %d of type %d\n",
                         int(getDocumentKind()), int(pool->getType())));
      }
      pool.reset();
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  long pos=input->tell();
  shared_ptr<StarObjectModel> model(new StarObjectModel(*this, true));
  if (!model->read(zone)) {
    STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: can not read the drawing model\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(DrawingLayer):###extra");
    return true;
  }
  if (m_textState->m_model) {
    STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: oops the drawing model is already defined\n"));
  }
  else
    m_textState->m_model=model;
  if (input->isEnd()) return true;
  pos=input->tell();
  uint16_t nSign;
  *input >> nSign;
  libstoff::DebugStream f;
  f << "Entries(DrawingLayer):";
  bool ok=true;
  if (nSign!=0x444D && nSign!=0) // 0 seems ok if followed by 0
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  else {
    uint16_t n;
    *input >> n;
    if (pos+4+4*long(n)>input->size()) {
      STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: bad n frame\n"));
      f << "###pos";
      ok=false;
    }
    else {
      f << "framePos=[";
      for (uint16_t i=0; i<n; ++i) f << input->readULong(4) << ",";
      f << "],";
    }
  }
  if (ok && input->tell()+4==input->size())
    f << "num[hidden]=" << input->readULong(4) << ",";
  if (ok && !input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: find extra data\n"));
    f << "###extra";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}
catch (...)
{
  return false;
}

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool StarObjectText::readWriterDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWWriterDocument", getPassword());
  if (!zone.readSWHeader()) {
    STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read the header\n"));
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  // sw_sw3doc.cxx Sw3IoImp::LoadDocContents
  SWFieldManager fieldManager;
  StarFileManager fileManager;
  while (!input->isEnd()) {
    long pos=input->tell();
    int rType=input->peek();
    bool done=false;
    switch (rType) {
    case '!':
      done=zone.readStringsPool();
      break;
    case 'R':
    case '0': // Outline
      done=readSWNumRule(zone,char(rType));
      break;
    case '1': {
      StarWriterStruct::NoteInfo info(true);
      done=info.read(zone);
      break;
    }
    case '4': {
      StarWriterStruct::NoteInfo info(false);
      done=info.read(zone);
      break;
    }
    case '8': {
      StarWriterStruct::PrintData info;
      done=info.read(zone);
      break;
    }
    case 'D': {
      StarWriterStruct::DatabaseName dbase;
      done=dbase.read(zone);
      break;
    }
    case 'F':
      done=getFormatManager()->readSWFlyFrameList(zone, *this);
      break;
    case 'J':
      done=readSWJobSetUp(zone);
      break;
    case 'M': {
      std::vector<StarWriterStruct::Macro> macroList;
      done=StarWriterStruct::Macro::readList(zone, macroList);
      break;
    }
    case 'N':
      done=readSWContent(zone, m_textState->m_mainContent);
      break;
    case 'U': { // layout info, no code, ignored by LibreOffice
      StarLayout layout;
      done=layout.read(zone, *this);
      break;
    }
    case 'V': {
      std::vector<std::vector<StarWriterStruct::Redline> > redlineListList;
      done=StarWriterStruct::Redline::readListList(zone, redlineListList);
      break;
    }
    case 'Y':
      done=fieldManager.readField(zone,'Y');
      break;
    case 'a': {
      std::vector<StarWriterStruct::Bookmark> markList;
      done=StarWriterStruct::Bookmark::readList(zone, markList);
      break;
    }
    case 'd': {
      StarWriterStruct::DocStats stats;
      done=stats.read(zone);
      break;
    }
    case 'j': {
      StarWriterStruct::Dictionary dico;
      done=dico.read(zone);
      break;
    }
    case 'q':
      done=getFormatManager()->readSWNumberFormatterList(zone);
      break;
    case 'u': {
      std::vector<StarWriterStruct::TOX> toxList;
      done=StarWriterStruct::TOX::readList(zone, toxList, *this);
      break;
    }
    case 'y': {
      std::vector<StarWriterStruct::TOX51> toxList;
      done=StarWriterStruct::TOX51::readList(zone, toxList, *this);
      break;
    }
    default:
      break;
    }
    if (done)
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    char type;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWWriterDocument[" << type << "]:";
    long lastPos=zone.getRecordLastPosition();
    bool endZone=false;
    std::vector<uint32_t> string;
    switch (type) {
    case '$': // unknown, seems to store an object name
      f << "dollarZone,";
      if (input->tell()+7>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: zone seems to short\n"));
        break;
      }
      for (int i=0; i<5; ++i) { // f0=f1=1
        int val=int(input->readULong(1));
        if (val) f << "f" << i << "=" << val << ",";
      }
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read main string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr();
      break;
    case '5': {
      // sw_sw3misc.cxx InLineNumberInfo
      int fl=zone.openFlagZone();
      if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
      f << "linenumberInfo=" << input->readULong(1) << ",";
      f << "nPos=" << input->readULong(1) << ",";
      f << "nChrIdx=" << input->readULong(2) << ",";
      f << "nPosFromLeft=" << input->readULong(2) << ",";
      f << "nCountBy=" << input->readULong(2) << ",";
      f << "nDividerCountBy=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read sDivider string\n"));
        f << "###sDivider";
        break;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr();
      break;
    }
    case '6':
      // sw_sw3misc.cxx InDocDummies
      f << "docDummies,";
      f << "n1=" << input->readULong(4) << ",";
      f << "n2=" << input->readULong(4) << ",";
      f << "n3=" << input->readULong(1) << ",";
      f << "n4=" << input->readULong(1) << ",";
      for (int i=0; i<2; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read a string\n"));
          f << "###string";
          break;
        }
        else if (!string.empty())
          f << (i==0 ? "sAutoMarkURL" : "s2") << "=" << libstoff::getString(string).cstr() << ",";
      }
      break;
    case '7': { // config, ignored by LibreOffice, and find no code
      f << "config,";
      int fl=int(zone.openFlagZone());
      if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
      f << "f0=" << input->readULong(1) << ","; // 1
      for (int i=0; i<5; ++i) // e,1,5,1,5
        f << "f" << i+1 << "=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      break;
    }
    case 'C': { // ignored by LibreOffice
      std::string comment("");
      while (lastPos && input->tell()<lastPos) comment+=char(input->readULong(1));
      f << "comment=" << comment << ",";
      break;
    }
    case 'P': // password
      // sw_sw3misc.cxx: InPasswd
      f << "passwd,";
      if (zone.isCompatibleWith(0x6)) {
        f << "cType=" << input->readULong(1) << ",";
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read passwd string\n"));
          f << "###passwd";
        }
        else
          f << "cryptedPasswd=" << libstoff::getString(string).cstr() << ",";
      }
      break;
    case 'Z':
      endZone=true;
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: find unexpected type\n"));
      f << "###type,";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWWriterDocument");
    if (endZone)
      break;
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWWriterDocument:##extra");
  }
  return true;
}
catch (...)
{
  return false;
}
////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
