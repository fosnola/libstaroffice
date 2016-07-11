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

#include "STOFFCell.hxx"
#include "STOFFOLEParser.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFParagraph.hxx"
#include "STOFFTable.hxx"
#include "STOFFTextListener.hxx"

#include "SWFieldManager.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarGraphicStruct.hxx"
#include "StarItemPool.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectModel.hxx"
#include "StarObjectSpreadsheet.hxx"
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
  GraphZone(shared_ptr<STOFFOLEParser> oleParser) : Zone(), m_oleParser(oleParser), m_attributeList(), m_limitList(), m_contour()
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const */*pool*/, StarObject &object) const;
  //! the ole parser
  shared_ptr<STOFFOLEParser> m_oleParser;
  //! the graph name, the fltName, the replace text
  librevenge::RVNGString m_names[3];
  //! the attributes list
  std::vector<shared_ptr<StarAttribute> > m_attributeList;
  //! the attributes limit
  std::vector<STOFFVec2i> m_limitList;
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
//! Internal: a textZone of StarObjectTextInteral
struct TextZone : public Zone {
  //! constructor
  TextZone() : Zone(), m_text(), m_textSourcePosition(), m_styleName(""), m_charAttributeList(), m_charLimitList(), m_format()
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
  std::vector<shared_ptr<StarAttribute> > m_charAttributeList;
  //! the character limit
  std::vector<STOFFVec2i> m_charLimitList;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
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

  std::set<size_t> modPosSet;
  size_t numFonts=m_charAttributeList.size();
  if (m_charLimitList.size()!=numFonts) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: the number of fonts seems bad\n"));
    if (m_charLimitList.size()<numFonts)
      numFonts=m_charLimitList.size();
  }
  modPosSet.insert(0);
  for (size_t i=0; i<numFonts; ++i) {
    if (m_charLimitList[i][0]>0)
      modPosSet.insert(size_t(m_charLimitList[i][0]));
    if (m_charLimitList[i][1]>0)
      modPosSet.insert(size_t(m_charLimitList[i][1]));
  }
  std::set<size_t>::const_iterator posSetIt=modPosSet.begin();
  for (size_t c=0; c<m_text.size(); ++c) {
    bool fontChange=false;
    size_t srcPos=c<m_textSourcePosition.size() ? m_textSourcePosition[c] : 10000;
    while (posSetIt!=modPosSet.end() && *posSetIt <= srcPos) {
      ++posSetIt;
      fontChange=true;
    }
    shared_ptr<StarAttribute> footnote;
    if (fontChange) {
      STOFFFont font(mainFont);
      for (size_t f=0; f<numFonts; ++f) {
        if ((m_charLimitList[f][0]>=0 && m_charLimitList[f][0]>int(srcPos)) ||
            (m_charLimitList[f][1]>=0 && m_charLimitList[f][1]<=int(srcPos)))
          continue;
        if (!m_charAttributeList[f])
          continue;
        m_charAttributeList[f]->addTo(font, pool);
        if (!footnote && font.m_footnote)
          footnote=m_charAttributeList[f];
        if (c==0)
          m_charAttributeList[f]->addTo(para, pool);
      }
      listener->setFont(font);
      if (c==0)
        listener->setParagraph(para);
    }
    if (footnote)
      footnote->send(listener, pool, object);
    else if (m_text[c]==0x9)
      listener->insertTab();
    else if (m_text[c]==0xa)
      listener->insertEOL(true);
    else
      listener->insertUnicode(m_text[c]);
  }
  return true;
}

struct TableLine;
//! small structure used to store a table box
struct TableBox {
  //! constructor
  TableBox() : m_formatId(0), m_numLines(0), m_content(), m_lineList(), m_format()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const;
  //! the format
  int m_formatId;
  //! the number of lines
  int m_numLines;
  //! the content
  shared_ptr<Content> m_content;
  //! a list of line
  std::vector<shared_ptr<TableLine> > m_lineList;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

bool TableBox::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TableBox::send: call without listener\n"));
    return false;
  }
  if (!m_lineList.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TableBox::send: sending box with line list is not implemented\n"));
  }
  if (m_content)
    m_content->send(listener, pool, object);
  return true;
}

//! small structure used to store a table line
struct TableLine {
  //! constructor
  TableLine() : m_formatId(0), m_numBoxes(0), m_boxList(), m_format()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object, int row) const;
  //! the format
  int m_formatId;
  //! the number of boxes
  int m_numBoxes;
  //! a list of box
  std::vector<shared_ptr<TableBox> > m_boxList;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

bool TableLine::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object, int row) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TableLine::send: call without listener\n"));
    return false;
  }
  listener->openTableRow(0, librevenge::RVNG_POINT);
  for (size_t i=0; i<m_boxList.size(); ++i) {
    STOFFVec2i pos(int(i), row);
    if (!m_boxList[i])
      listener->addEmptyTableCell(pos);

    STOFFCellStyle cellStyle;
    cellStyle.m_format=unsigned(m_boxList[i]->m_formatId);
    STOFFCell cell;
    cell.setPosition(pos);
    cell.setCellStyle(cellStyle);
    object.getFormatManager()->updateNumberingProperties(cell);
    listener->openTableCell(cell);
    m_boxList[i]->send(listener, pool, object);
    listener->closeTableCell();
  }
  listener->closeTableRow();
  return true;
}

//! Internal: a table of StarObjectTextInteral
struct Table : public Zone {
  //! constructor
  Table() : Zone(), m_headerRepeated(false), m_numBoxes(0), m_chgMode(0), m_lineList()
  {
  }
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const;

  //! flag to know if the header is repeated
  bool m_headerRepeated;
  //! the number of boxes
  int m_numBoxes;
  //! the change mode
  int m_chgMode;
  //! the list of line
  std::vector<shared_ptr<TableLine> > m_lineList;
};

bool Table::send(STOFFListenerPtr listener, StarItemPool const *pool, StarObject &object) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Table::send: call without listener\n"));
    return false;
  }
  STOFFTable table;
  // first find the number of columns
  size_t numCol=0;
  for (size_t i=0; i<m_lineList.size(); ++i) {
    if (m_lineList[i] && m_lineList[i]->m_boxList.size()>numCol)
      numCol=m_lineList[i]->m_boxList.size();
  }
  table.setColsSize(std::vector<float>(numCol,40)); // changeme
  listener->openTable(table);
  for (size_t i=0; i<m_lineList.size(); ++i) {
    if (m_lineList[i])
      m_lineList[i]->send(listener, pool, object, int(i));
  }
  listener->closeTable();
  return true;
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
  readSWBookmarkList(zone);
  readSWRedlineList(zone);
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
        // readSWPageDef will check that we can read the data, ....
        if (!readSWPageDef(zone))
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

bool StarObjectText::readSWPageDef(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='p' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3page.cxx InPageDesc
  libstoff::DebugStream f;
  f << "Entries(SWPageDef)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  int val=int(input->readULong(2));
  librevenge::RVNGString poolName;
  if (!zone.getPoolName(val, poolName)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSwPageDef: can not find a pool name\n"));
    f << "###nId=" << val << ",";
  }
  else if (!poolName.empty())
    f << poolName.cstr() << ",";
  val=int(input->readULong(2));
  if (val) f << "nFollow=" << val << ",";
  val=int(input->readULong(2));
  if (val) f << "nPoolId2=" << val << ",";
  val=int(input->readULong(1));
  if (val) f << "nNumType=" << val << ",";
  val=int(input->readULong(2));
  if (val) f << "nUseOn=" << val << ",";
  if (zone.isCompatibleWith(0x16,0x22, 0x101)) {
    val=int(input->readULong(2));
    if (val!=0xffff) f << "regCollIdx=" << val << ",";
  }
  zone.closeFlagZone();

  long lastPos=zone.getRecordLastPosition();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  while (input->tell() < lastPos) {
    pos=input->tell();
    int rType=input->peek();
    std::vector<shared_ptr<StarAttribute> > attributeList;
    std::vector<STOFFVec2i> limitsList;
    if (rType=='S' && readSWAttributeList(zone, *this, attributeList, limitsList))
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    f.str("");
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f << "SWPageDef[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case '1': // foot info
    case '2': { // page foot info
      f << (type=='1' ? "footInfo" : "pageFootInfo") << ",";
      val=int(input->readLong(4));
      if (val) f << "height=" << val << ",";
      val=int(input->readLong(4));
      if (val) f << "topDist=" << val << ",";
      val=int(input->readLong(4));
      if (val) f << "bottomDist=" << val << ",";
      val=int(input->readLong(2));
      if (val) f << "adjust=" << val << ",";
      f << "width=" << input->readLong(4) << "/" << input->readLong(4) << ",";
      val=int(input->readLong(2));
      if (val) f << "penWidth=" << val << ",";
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSwPageDef: can not read a color\n"));
        f << "###color,";
      }
      else if (!col.isBlack())
        f << col << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSwPageDef: find unknown type\n"));
      f << "###type,";
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWPageDef");
  }
  zone.closeSWRecord('p', "SWPageDef");
  return true;
}

bool StarObjectText::readSWAttribute(StarZone &zone, StarObject &doc, shared_ptr<StarAttribute> &attribute, STOFFVec2i &limits)
{
  attribute.reset();
  limits=STOFFVec2i(-1,-1);
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='A' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // sw_sw3fmts.cxx InAttr
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[SW-" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  uint16_t nWhich, nVers, nBegin=0xFFFF, nEnd=0xFFFF;
  *input >> nWhich >> nVers;
  if (fl&0x10) *input >> nBegin;
  if (fl&0x20) *input >> nEnd;

  int which=int(nWhich);
  if (which>0x6001 && zone.getDocumentVersion()!=0x0219) // bug correction 0x95500
    which+=15;
  if (which>=0x1000 && which<=0x1024) which+=-0x1000+int(StarAttribute::ATTR_CHR_CASEMAP);
  else if (which>=0x2000 && which<=0x2009) which+=-0x2000+int(StarAttribute::ATTR_TXT_INETFMT);
  else if (which>=0x3000 && which<=0x3006) which+=-0x3000+int(StarAttribute::ATTR_TXT_FIELD);
  else if (which>=0x4000 && which<=0x4013) which+=-0x4000+int(StarAttribute::ATTR_PARA_LINESPACING);
  else if (which>=0x5000 && which<=0x5022) which+=-0x5000+int(StarAttribute::ATTR_FRM_FILL_ORDER);
  else if (which>=0x6000 && which<=0x6013) which+=-0x6000+int(StarAttribute::ATTR_GRF_MIRRORGRF);
  else {
    STOFF_DEBUG_MSG(("StarObjectText::readSWAttribute: find unexpected which value\n"));
    which=-1;
    f << "###";
  }
  f << "wh=" << which << "[" << std::hex << nWhich << std::dec << "],";
  if (nVers) f << "nVers=" << nVers << ",";
  if (nBegin!=0xFFFF) {
    limits[0]=int(nBegin);
    f << "nBgin=" << nBegin << ",";
  }
  if (nEnd!=0xFFFF) {
    limits[1]=int(nEnd);
    f << "nEnd=" << nEnd << ",";
  }
  zone.closeFlagZone();

  if (which<=0 || !doc.getAttributeManager() ||
      !(attribute=doc.getAttributeManager()->readAttribute(zone, which, int(nVers), zone.getRecordLastPosition(), doc)))
    f << "###";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('A', "StarAttribute");
  return true;
}

bool StarObjectText::readSWAttributeList
(StarZone &zone, StarObject &doc, std::vector<shared_ptr<StarAttribute> > &attributeList, std::vector<STOFFVec2i> &limitsList)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='S' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[SWList-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell() < zone.getRecordLastPosition()) { // normally only 2
    pos=input->tell();
    shared_ptr<StarAttribute> attribute;
    STOFFVec2i limit;
    if (readSWAttribute(zone, doc, attribute, limit) && input->tell()>pos) {
      if (attribute) {
        attributeList.push_back(attribute);
        limitsList.push_back(limit);
      }
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('S', "StarAttribute");
  return true;
}

bool StarObjectText::readSWBookmarkList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='a' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // sw_sw3misc.cxx InBookmarks
  libstoff::DebugStream f;
  f << "Entries(SWBookmark)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    f.str("");
    f << "SWBookmark:";
    if (input->peek()!='B' || !zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }

    std::vector<uint32_t> text;
    bool ok=true;
    if (!zone.readString(text)) {
      ok=false;
      STOFF_DEBUG_MSG(("StarObjectText::readSWBookmarkList: can not read shortName\n"));
      f << "###short";
    }
    else
      f << libstoff::getString(text).cstr();
    if (ok && !zone.readString(text)) {
      ok=false;
      STOFF_DEBUG_MSG(("StarObjectText::readSWBookmarkList: can not read name\n"));
      f << "###";
    }
    else
      f << libstoff::getString(text).cstr();
    if (ok) {
      zone.openFlagZone();
      int val=int(input->readULong(2));
      if (val) f << "nOffset=" << val << ",";
      val=int(input->readULong(2));
      if (val) f << "nKey=" << val << ",";
      val=int(input->readULong(2));
      if (val) f << "nMod=" << val << ",";
      zone.closeFlagZone();
    }
    if (ok && input->tell()<zone.getRecordLastPosition()) {
      for (int i=0; i<4; ++i) { // start[aMac:aLib],end[aMac:Alib]
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("StarObjectText::readSWBookmarkList: can not read macro name\n"));
          f << "###macro";
          break;
        }
        else if (!text.empty())
          f << "macro" << i << "=" << libstoff::getString(text).cstr();
      }
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWBookmark");
  }

  zone.closeSWRecord('a', "SWBookmark");
  return true;
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
      shared_ptr<StarObjectTextInternal::Table> table;
      done=readSWTable(zone, table);
      if (done && table)
        content->m_zoneList.push_back(table);
      break;
    }
    case 'G': {
      shared_ptr<StarObjectTextInternal::GraphZone> graph;
      done=readSWGraphNode(zone, graph);
      if (done && graph)
        content->m_zoneList.push_back(graph);
      break;
    }
    case 'I':
      done=readSWSection(zone);
      break;
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
    case 'v':
      done=readSWNodeRedline(zone);
      break;
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

bool StarObjectText::readSWDBName(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='D' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx: InDBName
  libstoff::DebugStream f;
  f << "Entries(SWDBName)[" << zone.getRecordLevel() << "]:";
  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWDBName: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord('D', "SWDBName");
    return true;
  }
  if (!text.empty())
    f << "sStr=" << libstoff::getString(text).cstr() << ",";
  if (zone.isCompatibleWith(0xf,0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWDBName: can not read a SQL string\n"));
      f << "###SQL";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('D', "SWDBName");
      return true;
    }
    if (!text.empty())
      f << "sSQL=" << libstoff::getString(text).cstr() << ",";
  }
  if (zone.isCompatibleWith(0x11,0x22)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWDBName: can not read a table name string\n"));
      f << "###tableName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('D', "SWDBName");
      return true;
    }
    if (!text.empty())
      f << "sTableName=" << libstoff::getString(text).cstr() << ",";
  }
  if (zone.isCompatibleWith(0x12,0x22, 0x101)) {
    int nCount=int(input->readULong(2));
    f << "nCount=" << nCount << ",";
    if (nCount>0 && zone.isCompatibleWith(0x28)) {
      f << "dbData=[";
      for (int i=0; i<nCount; ++i) {
        if (input->tell()>=zone.getRecordLastPosition()) {
          STOFF_DEBUG_MSG(("StarObjectText::readSWDBName: can not read a DBData\n"));
          f << "###";
          break;
        }
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("StarObjectText::readSWDBName: can not read a table name string\n"));
          f << "###dbDataName";
          break;
        }
        f << libstoff::getString(text).cstr() << ":";
        f << input->readULong(4) << "<->"; // selStart
        f << input->readULong(4) << ","; // selEnd
      }
      f << "],";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('D', "SWDBName");
  return true;
}

bool StarObjectText::readSWDictionary(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='j' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InDictionary
  libstoff::DebugStream f;
  f << "Entries(SWDictionary)[" << zone.getRecordLevel() << "]:";
  long lastPos=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    f << "[";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWDictionary: can not read a string\n"));
      f << "###string,";
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    if (!string.empty())
      f << libstoff::getString(string).cstr() << ",";
    f << "nLanguage=" << input->readULong(2) << ",";
    f << "nCount=" << input->readULong(2) << ",";
    f << "c=" << input->readULong(1) << ",";
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSWRecord('j', "SWDictionary");
  return true;
}

bool StarObjectText::readSWEndNoteInfo(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='4' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx: InEndNoteInfo
  libstoff::DebugStream f;
  f << "Entries(SWEndNoteInfo)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  f << "eType=" << input->readULong(1) << ",";
  f << "nPageId=" << input->readULong(2) << ",";
  f << "nCollIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0xc))
    f << "nFtnOffset=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x203))
    f << "nChrIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x216) && (fl&0x10))
    f << "nAnchorChrIdx=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (zone.isCompatibleWith(0x203)) {
    std::vector<uint32_t> text;
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWEndNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord('4', "SWEndNoteInfo");
        return true;
      }
      if (!text.empty())
        f << (i==0 ? "prefix":"suffix") << "=" << libstoff::getString(text).cstr() << ",";
    }
  }
  if (input->tell()<zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWEndNoteInfo: find extra data\n"));
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('4', "SWEndNoteInfo");
  return true;
}

bool StarObjectText::readSWFootNoteInfo(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='1' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx: InFtnInfo and InFntInfo40
  libstoff::DebugStream f;
  f << "Entries(SWFootNoteInfo)[" << zone.getRecordLevel() << "]:";
  bool old=!zone.isCompatibleWith(0x201);
  std::vector<uint32_t> text;
  if (old) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWFootNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord('1', "SWFootNoteInfo");
      }
      if (!text.empty())
        f << (i==0 ? "quoVadis":"ergoSum") << "=" << libstoff::getString(text).cstr() << ",";
    }
  }
  int fl=zone.openFlagZone();

  if (old) {
    f << "ePos=" << input->readULong(1) << ",";
    f << "eNum=" << input->readULong(1) << ",";
  }
  f << "eType=" << input->readULong(1) << ",";
  f << "nPageId=" << input->readULong(2) << ",";
  f << "nCollIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0xc))
    f << "nFtnOffset=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x203))
    f << "nChrIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x216) && (fl&0x10))
    f << "nAnchorChrIdx=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (zone.isCompatibleWith(0x203)) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWFootNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord('1', "SWFootNoteInfo");
        return true;
      }
      if (!text.empty())
        f << (i==0 ? "prefix":"suffix") << "=" << libstoff::getString(text).cstr() << ",";
    }
  }

  if (!old) {
    zone.openFlagZone();
    f << "ePos=" << input->readULong(1) << ",";
    f << "eNum=" << input->readULong(1) << ",";
    zone.closeFlagZone();
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWFootNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord('1', "SWFootNoteInfo");
        return true;
      }
      if (!text.empty())
        f << (i==0 ? "quoVadis":"ergoSum") << "=" << libstoff::getString(text).cstr() << ",";
    }
  }
  if (input->tell()<zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWFootNoteInfo: find extra data\n"));
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('1', "SWFootNoteInfo");
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
      done=readSWAttributeList(zone, *this, graphZone->m_attributeList, graphZone->m_limitList);
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

bool StarObjectText::readSWLayoutInfo(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='U' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // find no code, let improvise
  libstoff::DebugStream f;
  f << "Entries(SWLayoutInfo)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  f << "f0=" << input->readULong(2) << ",";
  if (input->tell()!=zone.getFlagLastPosition()) // maybe
    f << "f1=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (readSWLayoutSub(zone)) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWLayoutInfo[" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWLayoutInfo");
    break;
  }

  zone.closeSWRecord('U', "SWLayoutInfo");
  return true;
}

bool StarObjectText::readSWLayoutSub(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  int rType=input->peek();
  if ((rType!=0xd2&&rType!=0xd7) || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // find no code, let improvise
  libstoff::DebugStream f;
  f << "Entries(SWLayoutSub)[" << std::hex << rType << std::dec << "-" << zone.getRecordLevel() << "]:";
  int const expectedSz=rType==0xd2 ? 11 : 9;
  long lastPos=zone.getRecordLastPosition();
  int val=int(input->readULong(1));
  if (val!=0x11) f << "f0=" << val << ",";
  val=int(input->readULong(1));
  if (val!=0xaf) f << "f1=" << val << ",";
  val=int(input->readULong(1)); // small value 1-1f
  if (val) f << "f2=" << val << ",";
  val=int(input->readULong(1));
  if (val) f << "f3=" << std::hex << val << std::dec << ",";
  if (input->tell()+(val&0xf)+expectedSz>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWLayoutSub: the zone seems too short\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(char(0xd2), "SWLayoutSub");
    return true;
  }
  ascFile.addDelimiter(input->tell(),'|');
  input->seek(input->tell()+(val&0xf)+expectedSz, librevenge::RVNG_SEEK_SET);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos) {
    pos=input->tell();
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWLayoutSub[" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeSWRecord(type, "SWLayoutSub");
  }

  zone.closeSWRecord(char(rType), "SWLayoutSub");
  return true;
}

bool StarObjectText::readSWMacroTable(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='M' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InMacroTable
  libstoff::DebugStream f;
  f << "Entries(SWMacroTable)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='m' || !zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f << "SWMacroTable:";
    f << "key=" << input->readULong(2) << ",";
    bool ok=true;
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWMacroTable: can not read a string\n"));
        f << "###,";
        ok=false;
        break;
      }
      if (!string.empty())
        f << libstoff::getString(string).cstr() << ",";
    }
    if (ok && zone.isCompatibleWith(0x102))
      f << "scriptType=" << input->readULong(2) << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWMacroTable");
  }
  zone.closeSWRecord('M', "SWMacroTable");
  return true;
}

bool StarObjectText::readSWNodeRedline(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='v' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3redln.cxx InNodeRedLine
  libstoff::DebugStream f;
  f << "Entries(SWNodeRedline)[" << zone.getRecordLevel() << "]:";
  int cFlag=zone.openFlagZone();
  if (cFlag&0xf0) f << "flag=" << (cFlag>>4) << ",";
  f << "nId=" << input->readULong(2) << ",";
  f << "nNodeOf=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('v', "SWNodeRedline");
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

bool StarObjectText::readSWRedlineList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='V' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3redline.cxx inRedlines
  libstoff::DebugStream f;
  f << "Entries(SWRedline)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    f.str("");
    f << "SWRedline:";
    if (input->peek()!='R' || !zone.openSWRecord(type)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWRedlineList: find extra data\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    zone.openFlagZone();
    int N=int(input->readULong(2));
    zone.closeFlagZone();
    f << "N=" << N << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (int i=0; i<N; ++i) {
      pos=input->tell();
      f.str("");
      f << "SWRedline-" << i << ":";
      if (input->peek()!='D' || !zone.openSWRecord(type)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWRedlineList: can not read a redline\n"));
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        break;
      }

      zone.openFlagZone();
      int val=int(input->readULong(1));
      if (val) f << "cType=" << val << ",";
      val=int(input->readULong(2));
      if (val) f << "stringId=" << val << ",";
      zone.closeFlagZone();

      f << "date=" << input->readULong(4) << ",";
      f << "time=" << input->readULong(4) << ",";
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWRedlineList: can not read the comment\n"));
        f << "###comment";
      }
      else if (!text.empty())
        f << libstoff::getString(text).cstr();
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('D', "SWRedline");
    }
    zone.closeSWRecord('R', "SWRedline");
  }
  zone.closeSWRecord('V', "SWRedline");
  return true;
}

bool StarObjectText::readSWSection(StarZone &zone)
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
    f << (i==0 ? "name" : "cond") << "=" << libstoff::getString(text).cstr() << ",";
  }
  int fl=zone.openFlagZone();
  if (fl&0x10) f << "hidden,";
  if (fl&0x20) f << "protect,";
  if (fl&0x40) f << "condHidden,";
  if (fl&0x40) f << "connectFlag,";
  f << "nType=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  if (zone.isCompatibleWith(0xd)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWSection: can not read a linkName\n"));
      f << "###linkName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('I', "SWSection");
      return true;
    }
    else if (!text.empty())
      f << "linkName=" << libstoff::getString(text).cstr() << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('I', "SWSection");
  return true;
}

bool StarObjectText::readSWTable(StarZone &zone, shared_ptr<StarObjectTextInternal::Table> &table)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='E' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTable
  libstoff::DebugStream f;
  f << "Entries(SWTableDef)[" << zone.getRecordLevel() << "]:";
  table.reset(new StarObjectTextInternal::Table);
  int fl=zone.openFlagZone();
  if (fl&0x20) {
    table->m_headerRepeated=true;
    f << "headerRepeat,";
  }
  table->m_numBoxes=int(input->readULong(2));
  if (table->m_numBoxes)
    f << "nBoxes=" << table->m_numBoxes << ",";
  if (zone.isCompatibleWith(0x5,0x201))
    f << "nTblIdDummy=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x103)) {
    table->m_chgMode=int(input->readULong(1));
    if (table->m_chgMode)
      f << "cChgMode=" << table->m_chgMode << ",";
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  // TODO storeme
  shared_ptr<StarFormatManagerInternal::FormatDef> format;
  if (input->peek()=='f') getFormatManager()->readSWFormatDef(zone, 'f', format, *this);
  if (input->peek()=='Y') {
    SWFieldManager fieldManager;
    fieldManager.readField(zone,'Y');
  }
  while (input->tell()<lastPos && input->peek()=='v') {
    pos=input->tell();
    // TODO storeme
    if (readSWNodeRedline(zone))
      continue;
    STOFF_DEBUG_MSG(("StarObjectText::readSWTable: can not read a red line\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWTableDef:###redline");
    zone.closeSWRecord('E',"SWTableDef");
    return true;
  }

  while (input->tell()<lastPos && input->peek()=='L') {
    pos=input->tell();
    shared_ptr<StarObjectTextInternal::TableLine> line;
    if (readSWTableLine(zone, line)) {
      table->m_lineList.push_back(line);
      continue;
    }
    STOFF_DEBUG_MSG(("StarObjectText::readSWTable: can not read a table line\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('E',"SWTableDef");
  return true;
}

bool StarObjectText::readSWTableBox(StarZone &zone, shared_ptr<StarObjectTextInternal::TableBox> &box)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='t' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTableBox
  libstoff::DebugStream f;
  f << "Entries(SWTableBox)[" << zone.getRecordLevel() << "]:";
  box.reset(new StarObjectTextInternal::TableBox);
  int fl=zone.openFlagZone();
  if (fl&0x20 || !zone.isCompatibleWith(0x201)) {
    box->m_formatId=int(input->readULong(2));
    librevenge::RVNGString format;
    if (!zone.getPoolName(box->m_formatId, format))
      f << "###format=" << box->m_formatId << ",";
    else
      f << format.cstr() << ",";
  }
  if (fl&0x10) {
    box->m_numLines=int(input->readULong(2));
    if (box->m_numLines)
      f << "numLines=" << box->m_numLines << ",";
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->peek()=='f')
    getFormatManager()->readSWFormatDef(zone,'f', box->m_format, *this);
  if (input->peek()=='N')
    readSWContent(zone, box->m_content);
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    shared_ptr<StarObjectTextInternal::TableLine> line;
    if (readSWTableLine(zone, line) && input->tell()<=lastPos) {
      box->m_lineList.push_back(line);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('t', "SWTableBox");
  return true;
}

bool StarObjectText::readSWTableLine(StarZone &zone, shared_ptr<StarObjectTextInternal::TableLine> &line)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='L' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTableLine
  line.reset(new StarObjectTextInternal::TableLine);
  libstoff::DebugStream f;
  f << "Entries(SWTableLine)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20 || !zone.isCompatibleWith(0x201)) {
    line->m_formatId=int(input->readULong(2));
    librevenge::RVNGString format;
    if (!zone.getPoolName(line->m_formatId, format))
      f << "###format=" << line->m_formatId << ",";
    else
      f << format.cstr() << ",";
  }
  line->m_numBoxes=int(input->readULong(2));
  if (line->m_numBoxes)
    f << "nBoxes=" << line->m_numBoxes << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (input->peek()=='f')
    getFormatManager()->readSWFormatDef(zone,'f',line->m_format, *this);

  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    shared_ptr<StarObjectTextInternal::TableBox> box;
    if (readSWTableBox(zone, box) && input->tell()<=lastPos) {
      line->m_boxList.push_back(box);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('L', "SWTableLine");
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
      shared_ptr<StarAttribute> attribute;
      STOFFVec2i limit;
      done=readSWAttribute(zone, *this, attribute, limit);
      if (done && attribute) {
        textZone->m_charAttributeList.push_back(attribute);
        textZone->m_charLimitList.push_back(limit);
      }
      break;
    }
    case 'R':
      done=readSWNumRule(zone,'R');
      break;
    case 'S':
      done=readSWAttributeList(zone, *this, textZone->m_charAttributeList, textZone->m_charLimitList);
      break;
    case 'l': // related to link
      done=getFormatManager()->readSWFormatDef(zone,'l', textZone->m_format, *this);
      break;
    case 'o': { // format: safe to ignore
      shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=getFormatManager()->readSWFormatDef(zone,'o', format, *this);
      break;
    }
    case 'v':
      done=readSWNodeRedline(zone);
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
    case 'K':
      // sw_sw3misc.cxx InNodeMark
      f << "nodeMark,";
      f << "cType=" << input->readULong(1) << ",";
      f << "nId=" << input->readULong(2) << ",";
      f << "nOff=" << input->readULong(2) << ",";
      break;
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

bool StarObjectText::readSWTOXList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='u' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InTOXs
  libstoff::DebugStream f;
  f << "Entries(SWTOXList)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    int rType=input->peek();
    if (rType!='x' || !zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    long lastRecordPos=zone.getRecordLastPosition();
    f << "SWTOXList:";
    int fl=zone.openFlagZone();
    if (fl&0xf0)
      f << "fl=" << (fl>>4) << ",";
    f << "nType=" << input->readULong(2) << ",";
    f << "nCreateType=" << input->readULong(2) << ",";
    f << "nCaptionDisplay=" << input->readULong(2) << ",";
    f << "nStrIdx=" << input->readULong(2) << ",";
    f << "nSeqStrIdx=" << input->readULong(2) << ",";
    f << "nData=" << input->readULong(2) << ",";
    f << "cFormFlags=" << input->readULong(1) << ",";
    zone.closeFlagZone();
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWTOXList: can not read aName\n"));
      f << "###aName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "SWTOXList");
      continue;
    }
    if (!string.empty())
      f << "aName=" << libstoff::getString(string).cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWTOXList: can not read aTitle\n"));
      f << "###aTitle";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "SWTOXList");
      continue;
    }
    if (!string.empty())
      f << "aTitle=" << libstoff::getString(string).cstr() << ",";
    if (zone.isCompatibleWith(0x215)) {
      f << "nOLEOptions=" << input->readULong(2) << ",";
      f << "nMainStyleIdx=" << input->readULong(2) << ",";
    }
    else {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWTOXList: can not read aDummy\n"));
        f << "###aDummy";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord(type, "SWTOXList");
        continue;
      }
      if (!string.empty())
        f << "aDummy=" << libstoff::getString(string).cstr() << ",";
    }

    int N=int(input->readULong(1));
    f << "nPatterns=" << N << ",";
    bool ok=true;
    for (int i=0; i<N; ++i) {
      if (getFormatManager()->readSWPatternLCL(zone))
        continue;
      ok=false;
      f << "###pat";
      break;
    }
    if (!ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "SWTOXList");
      continue;
    }
    N=int(input->readULong(1));
    f << "nTmpl=" << N << ",";
    f << "tmpl[strId]=[";
    for (int i=0; i<N; ++i)
      f << input->readULong(2) << ",";
    f << "],";
    N=int(input->readULong(1));
    f << "nStyle=" << N << ",";
    f << "style=[";
    for (int i=0; i<N; ++i) {
      f << "[";
      f << "level=" << input->readULong(1) << ",";
      int nCount=int(input->readULong(2));
      f << "nCount=" << nCount << ",";
      if (input->tell()+2*nCount>lastRecordPos) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWTOXList: can not read some string id\n"));
        f << "###styleId";
        ok=false;
        break;
      }
      librevenge::RVNGString poolName;
      for (int j=0; j<nCount; ++j) {
        int val=int(input->readULong(2));
        if (!zone.getPoolName(val, poolName))
          f << "###nPoolId=" << val << ",";
        else
          f << poolName.cstr() << ",";
      }
      f << "],";
    }
    f << "],";
    if (!ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "SWTOXList");
      continue;
    }
    fl=zone.openFlagZone();
    f << "nSectStrIdx=" << input->readULong(2) << ",";
    if (fl&0x10) f << "nTitleLen=" << input->readULong(4) << ",";
    zone.closeFlagZone();

    if ((fl&0x10)) {
      while (input->tell()<zone.getRecordLastPosition() && input->peek()=='s') {
        shared_ptr<StarFormatManagerInternal::FormatDef> format;
        if (!getFormatManager()->readSWFormatDef(zone,'s', format, *this)) {
          STOFF_DEBUG_MSG(("StarObjectText::readSWTOXList: can not read some format\n"));
          f << "###format,";
          break;
        }
      }
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWTOXList");
  }
  zone.closeSWRecord('u', "SWTOXList");
  return true;
}

bool StarObjectText::readSWTOX51List(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='y' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InTOX51s
  libstoff::DebugStream f;
  f << "Entries(SWTOX51List)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='x' || !zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f << "SWTOX51List:";
    if (zone.isCompatibleWith(0x201)) {
      int strId=int(input->readULong(2));
      librevenge::RVNGString poolName;
      if (strId!=0xFFFF && !zone.getPoolName(strId, poolName))
        f << "###nPoolId=" << strId << ",";
      else if (strId!=0xFFFF && !poolName.empty())
        f << poolName.cstr() << ",";
    }
    else {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWTOX51List: can not read typeName\n"));
        f << "###typeName";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord(type, "SWTOX51List");
        continue;
      }
      if (!string.empty())
        f << "typeName=" << libstoff::getString(string).cstr() << ",";
    }
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWTOX51List: can not read aTitle\n"));
      f << "###aTitle";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "SWTOX51List");
      continue;
    }
    if (!string.empty())
      f << "aTitle=" << libstoff::getString(string).cstr() << ",";
    int fl=zone.openFlagZone();
    f << "nCreateType=" << input->readLong(2) << ",";
    f << "nType=" << input->readULong(1) << ",";
    if (zone.isCompatibleWith(0x213) && (fl&0x10))
      f << "firstTabPos=" << input->readULong(2) << ",";

    int N=int(input->readULong(1));
    f << "nPat=" << N << ",";
    f << "pat=[";
    bool ok=true;
    for (int i=0; i<N; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWTOX51List: can not read a pattern name\n"));
        f << "###pat";
        ok=false;
        break;
      }
      if (!string.empty())
        f << libstoff::getString(string).cstr() << ",";
    }
    f << "],";
    if (!ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "SWTOX51List");
      continue;
    }
    N=int(input->readULong(1));
    f << "nTmpl=" << N << ",";
    f << "tmpl[strId]=[";
    for (int i=0; i<N; ++i)
      f << input->readULong(2) << ",";
    f << "],";

    f << "nInf=" << input->readULong(2) << ",";

    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWTOX51List");
  }
  zone.closeSWRecord('y', "SWTOX51List");
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
    case '1':
      done=readSWFootNoteInfo(zone);
      break;
    case '4':
      done=readSWEndNoteInfo(zone);
      break;
    case 'D':
      done=readSWDBName(zone);
      break;
    case 'F':
      done=getFormatManager()->readSWFlyFrameList(zone, *this);
      break;
    case 'J':
      done=readSWJobSetUp(zone);
      break;
    case 'M':
      done=readSWMacroTable(zone);
      break;
    case 'N':
      done=readSWContent(zone, m_textState->m_mainContent);
      break;
    case 'U': // layout info, no code, ignored by LibreOffice
      done=readSWLayoutInfo(zone);
      break;
    case 'V':
      done=readSWRedlineList(zone);
      break;
    case 'Y':
      done=fieldManager.readField(zone,'Y');
      break;

    case 'a':
      done=readSWBookmarkList(zone);
      break;
    case 'j':
      done=readSWDictionary(zone);
      break;
    case 'q':
      done=getFormatManager()->readSWNumberFormatterList(zone);
      break;
    case 'u':
      done=readSWTOXList(zone);
      break;
    case 'y':
      done=readSWTOX51List(zone);
      break;
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
    case '8':
      // sw_sw3misc.cxx InPagePreviewPrintData
      f << "pagePreviewPrintData,";
      f << "cFlags=" << input->readULong(1) << ",";
      f << "nRow=" << input->readULong(2) << ",";
      f << "nCol=" << input->readULong(2) << ",";
      f << "nLeftSpace=" << input->readULong(2) << ",";
      f << "nRightSpace=" << input->readULong(2) << ",";
      f << "nTopSpace=" << input->readULong(2) << ",";
      f << "nBottomSpace=" << input->readULong(2) << ",";
      f << "nHorzSpace=" << input->readULong(2) << ",";
      f << "nVertSpace=" << input->readULong(2) << ",";
      break;
    case 'd':
      // sw_sw3misc.cxx: InDocStat
      f << "docStats,";
      f << "nTbl=" << input->readULong(2) << ",";
      f << "nGraf=" << input->readULong(2) << ",";
      f << "nOLE=" << input->readULong(2) << ",";
      if (zone.isCompatibleWith(0x201)) {
        f << "nPage=" << input->readULong(4) << ",";
        f << "nPara=" << input->readULong(4) << ",";
      }
      else {
        f << "nPage=" << input->readULong(2) << ",";
        f << "nPara=" << input->readULong(2) << ",";
      }
      f << "nWord=" << input->readULong(4) << ",";
      f << "nChar=" << input->readULong(4) << ",";
      f << "nModified=" << input->readULong(1) << ",";
      break;
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
