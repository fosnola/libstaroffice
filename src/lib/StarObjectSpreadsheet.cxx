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
#include <set>
#include <sstream>

#include <librevenge/librevenge.h>

#include "StarAttribute.hxx"
#include "StarCellFormula.hxx"
#include "StarEncryption.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObjectDraw.hxx"
#include "StarObjectModel.hxx"
#include "StarObjectSmallText.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "STOFFCell.hxx"
#include "STOFFCellStyle.hxx"
#include "STOFFFont.hxx"
#include "STOFFGraphicStyle.hxx"
#include "STOFFOLEParser.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFSubDocument.hxx"
#include "STOFFSpreadsheetListener.hxx"
#include "STOFFTable.hxx"

#include "StarFormatManager.hxx"

#include "StarObjectSpreadsheet.hxx"

/** Internal: the structures of a StarObjectSpreadsheet */
namespace StarObjectSpreadsheetInternal
{
////////////////////////////////////////
//! Internal: a structure use to read ScMultiRecord zone of a StarObjectSpreadsheet
struct ScMultiRecord {
  //! constructor
  explicit ScMultiRecord(StarZone &zone)
    : m_zone(zone)
    , m_zoneOpened(false)
    , m_actualRecord(0)
    , m_numRecord(0)
    , m_startPos(0)
    , m_endPos(0)
    , m_endContentPos(0)
    , m_endRecordPos(0)
    , m_offsetList()
    , m_extra("")
  {
  }
  //! destructor
  ~ScMultiRecord()
  {
    if (m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::~ScMultiRecord: INTERNAL PROBLEM\n"));
      close("Entries(BADScMultiRecord):###");
    }
  }
  //! try to open a zone
  bool open()
  {
    if (m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord: oops a record has been opened\n"));
      return false;
    }
    m_actualRecord=m_numRecord=0;
    m_startPos=m_endPos=m_endContentPos=m_endRecordPos=0;
    m_offsetList.clear();

    STOFFInputStreamPtr input=m_zone.input();
    long pos=input->tell();
    long lastPos=m_zone.getRecordLevel() ? m_zone.getRecordLastPosition() : input->size();
    if (!m_zone.openSCRecord()) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    m_zoneOpened=true;
    m_startPos=input->tell();
    m_endPos=m_zone.getRecordLastPosition();
    // sc_rechead.cxx ScMultipleReadHeader::ScMultipleReadHeader
    if (m_endPos+6>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::open: oops the zone seems too short\n"));
      m_extra="###zoneShort,";
      return false;
    }
    input->seek(m_endPos, librevenge::RVNG_SEEK_SET);
    uint16_t id;
    uint32_t tableLen;
    *input>>id >> tableLen;
    m_endRecordPos=input->tell()+long(tableLen);
    if (id!=0x4200 || m_endRecordPos > lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::open: oops can not find the size data\n"));
      m_extra="###zoneShort,";
      m_endRecordPos=0;
      return false;
    }
    m_numRecord=tableLen/4;
    for (uint32_t i=0; i<m_numRecord; ++i)
      m_offsetList.push_back(static_cast<uint32_t>(input->readULong(4)));
    input->seek(m_startPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  //! try to close a zone
  void close(std::string const &wh)
  try
  {
    if (!m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::close: can not find any opened zone\n"));
      return;
    }
    if (m_endContentPos>0) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::close: argh, the current content is not closed, let close it\n"));
      closeContent(wh);
    }

    m_zoneOpened=false;
    STOFFInputStreamPtr input=m_zone.input();
    if (input->tell()<m_endPos && input->tell()+4>=m_endPos) { // small diff is possible
      m_zone.ascii().addDelimiter(input->tell(),'|');
      input->seek(m_zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()==m_endPos)
      input->seek(m_zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    m_zone.closeSCRecord(wh);
    if (m_endRecordPos>0)
      input->seek(m_endRecordPos, librevenge::RVNG_SEEK_SET);
  }
  catch (...)
  {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::close: catch an exception...\n"));
    m_zoneOpened=false;
  }
  //! returns true if a content is opened
  bool isContentOpened() const
  {
    return m_endContentPos>0;
  }
  //! try to go to the new content positon
  bool openContent(std::string const &wh)
  {
    if (m_endContentPos>0) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::openContent: argh, the current content is not closed, let close it\n"));
      closeContent(wh);
    }
    STOFFInputStreamPtr input=m_zone.input();
    if (m_actualRecord >= m_numRecord || m_actualRecord >= uint32_t(m_offsetList.size()) ||
        input->tell()+long(m_offsetList[size_t(m_actualRecord)])>m_endPos)
      return false;
    // ScMultipleReadHeader::StartEntry
    m_endContentPos=input->tell()+long(m_offsetList[size_t(m_actualRecord)]);
    ++m_actualRecord;
    return true;
  }
  //! try to go to the new content positon
  bool closeContent(std::string const &wh)
  {
    if (m_endContentPos<=0) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::closeContent: no content opened\n"));
      return false;
    }
    STOFFInputStreamPtr input=m_zone.input();
    if (input->tell()<m_endContentPos && input->tell()+4>=m_endContentPos) { // small diff is possible ?
      m_zone.ascii().addDelimiter(input->tell(),'|');
      input->seek(m_endContentPos, librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()!=m_endContentPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::getContent: find extra data\n"));
      m_zone.ascii().addPos(input->tell());
      libstoff::DebugStream f;
      f << wh << ":###extra";
      m_zone.ascii().addNote(f.str().c_str());
      input->seek(m_endContentPos, librevenge::RVNG_SEEK_SET);
    }
    m_endContentPos=0;
    return true;
  }
  //! returns the last content position
  long getContentLastPosition() const
  {
    if (m_endContentPos<=0) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::ScMultiRecord::getContentLastPosition: no content opened\n"));
      return m_endPos;
    }
    return m_endContentPos;
  }

  //! basic operator<< ; print header data
  friend std::ostream &operator<<(std::ostream &o, ScMultiRecord const &r)
  {
    if (!r.m_zoneOpened) {
      o << r.m_extra;
      return o;
    }
    if (r.m_numRecord) o << "num[record]=" << r.m_numRecord << ",";
    if (!r.m_offsetList.empty()) {
      o << "offset=[";
      for (unsigned int i : r.m_offsetList) o << i << ",";
      o << "],";
    }
    o << r.m_extra;
    return o;
  }
protected:
  //! the main zone
  StarZone &m_zone;
  //! true if a SfxRecord has been opened
  bool m_zoneOpened;
  //! the actual record
  uint32_t m_actualRecord;
  //! the number of record
  uint32_t m_numRecord;
  //! the start of data position
  long m_startPos;
  //! the end of data position
  long m_endPos;
  //! the end of the content position
  long m_endContentPos;
  //! the end of the record position
  long m_endRecordPos;
  //! the list of offset
  std::vector<uint32_t> m_offsetList;
  //! extra data
  std::string m_extra;
private:
  ScMultiRecord(ScMultiRecord const &orig);
  ScMultiRecord &operator=(ScMultiRecord const &orig);
};

//! Internal: the cell of a StarObjectSpreadsheet
class Cell : public STOFFCell
{
public:
  //! constructor
  explicit Cell(STOFFVec2i pos=STOFFVec2i(0,0))
    : STOFFCell()
    , m_content()
    , m_textZone()
    , m_hasNote(false)
  {
    setPosition(pos);
  }
  //! destructor
  ~Cell() override;
  //! the cell content
  STOFFCellContent m_content;
  //! the text zone(if set)
  std::shared_ptr<StarObjectSmallText> m_textZone;
  //! flag to know if the cell has some note
  bool m_hasNote;
  //! the notes text, date, author
  librevenge::RVNGString m_notes[3];
};

Cell::~Cell()
{
}
////////////////////////////////////////
//! Internal: structure used to store a row of a StarObjectSpreadsheet
class RowContent
{
public:
  //! constructor
  RowContent()
    : m_colToCellMap()
    , m_colToAttributeMap()
  {
  }
  //! try to compress the item list and create a attribute list
  void compressItemList()
  {
    std::map<STOFFVec2i,std::shared_ptr<StarAttribute> > oldMap=m_colToAttributeMap;
    std::map<STOFFVec2i,std::shared_ptr<StarAttribute> >::const_iterator it=oldMap.begin();
    m_colToAttributeMap.clear();

    std::shared_ptr<StarAttribute> actAttribute;
    STOFFVec2i actPos(0,-1);
    while (it!=oldMap.end()) {
      // first check for not filled row
      std::shared_ptr<StarAttribute> newAttrib=it->second;
      if (it->first[0]!=actPos[1]+1 || newAttrib.get()!=actAttribute.get()) {
        if (actAttribute)
          m_colToAttributeMap[actPos]=actAttribute;
        actAttribute=newAttrib;
        actPos=STOFFVec2i(it->first[0], it->first[0]-1);
      }
      actPos[1]=it->first[1];
      ++it;
    }
    if (actAttribute)
      m_colToAttributeMap[actPos]=actAttribute;
  }
  //! map col -> cell
  std::map<int, std::shared_ptr<Cell> > m_colToCellMap;
  //! map col -> attribute
  std::map<STOFFVec2i, std::shared_ptr<StarAttribute> > m_colToAttributeMap;
};

////////////////////////////////////////
//! Internal: a table of a StarObjectSpreadsheet
class Table : public STOFFTable
{
public:
  //! constructor
  Table(int loadingVers, int maxRow)
    : m_loadingVersion(loadingVers)
    , m_name("")
    , m_pageStyle("")
    , m_maxRow(maxRow)
    , m_colWidthList()
    , m_rowHeightMap()
    , m_rowToRowContentMap()
    , m_badCell()
  {
  }
  //! destructor
  ~Table() override;
  //! returns the load version
  int getLoadingVersion() const
  {
    return m_loadingVersion;
  }
  //! returns the maximum number of columns
  static int getMaxCols()
  {
    return 255;
  }
  //! returns the maximum number of row
  int getMaxRows() const
  {
    return m_maxRow;
  }
  //! returns the col width dimension in inch
  std::vector<float> getColumnWidths(std::vector<int> &repeated) const
  {
    std::vector<float> widths;
    repeated.clear();
    size_t actPos=0;
    int lastWidth=-1;
    for (size_t i=0; i<=m_colWidthList.size(); ++i) {
      if ((i==m_colWidthList.size() || lastWidth!=m_colWidthList[i]) && actPos<i) {
        widths.push_back(float(lastWidth)/1440.f);
        repeated.push_back(int(i-actPos));
        actPos=i;
      }
      if (i==m_colWidthList.size()) break;
      lastWidth=m_colWidthList[i];
    }
    return widths;
  }
  //! returns the row size in point
  float getRowHeight(int row) const
  {
    auto rIt=m_rowHeightMap.lower_bound(STOFFVec2i(-1,row));
    if (rIt!=m_rowHeightMap.end() && rIt->first[0]<=row && rIt->first[1]>=row)
      return float(rIt->second)/20.f;
    return 12.f;
  }
  //! returns a row content
  RowContent *getRow(int row)
  {
    auto it=m_rowToRowContentMap.lower_bound(STOFFVec2i(-1,row));
    if (it!=m_rowToRowContentMap.end() && it->first[0]<=row && row<=it->first[1])
      return &it->second;
    return nullptr;
  }
  //! create a block of rows(if not created)
  void updateRowsBlocks(STOFFVec2i const &rows)
  {
    auto it=m_rowToRowContentMap.lower_bound(STOFFVec2i(-1,rows[0]));
    if (it==m_rowToRowContentMap.end()) {
      m_rowToRowContentMap[rows]=RowContent();
      return;
    }
    if (it->first[0]<rows[0]) { // we must split this set of rows
      STOFFVec2i oldRows=it->first;
      RowContent const &content=it->second;
      m_rowToRowContentMap[STOFFVec2i(oldRows[0],rows[0]-1)]=content;
      m_rowToRowContentMap[STOFFVec2i(rows[0],oldRows[1])]=content;
      m_rowToRowContentMap.erase(oldRows);
    }
    int actRow=rows[0];
    while (actRow<=rows[1]) {
      it=m_rowToRowContentMap.lower_bound(STOFFVec2i(-1,actRow));
      if (it==m_rowToRowContentMap.end()) {
        m_rowToRowContentMap[STOFFVec2i(actRow,rows[1])]=RowContent();
        break;
      }
      if (it->first[0]!=actRow) {
        if (it->first[0]-1<=rows[1]) {
          m_rowToRowContentMap[STOFFVec2i(actRow,it->first[0]-1)]=RowContent();
          actRow=it->first[0];
          continue;
        }
        m_rowToRowContentMap[STOFFVec2i(actRow,rows[1])]=RowContent();
        break;
      }
      if (it->first[1]<=rows[1]) {
        actRow=it->first[1]+1;
        continue;
      }
      // we must split the last row
      STOFFVec2i oldRows=it->first;
      RowContent const &content=it->second;
      m_rowToRowContentMap[STOFFVec2i(oldRows[0],rows[1])]=content;
      m_rowToRowContentMap[STOFFVec2i(rows[1]+1,oldRows[1])]=content;
      m_rowToRowContentMap.erase(oldRows);
      break;
    }
  }
  //! returns a cell corresponding to a position
  Cell &getCell(STOFFVec2i const &pos)
  {
    if (pos[1]<0 || pos[1]>getMaxRows() || pos[0]<0 || pos[0]>getMaxCols()) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::Table::getCell: the position is bad (%d,%d)\n", pos[0], pos[1]));
      return m_badCell;
    }
    updateRowsBlocks(STOFFVec2i(pos[1],pos[1]));
    RowContent *row=getRow(pos[1]);
    if (row->m_colToCellMap.find(pos[0]) == row->m_colToCellMap.end() || !row->m_colToCellMap.find(pos[0])->second) {
      std::shared_ptr<Cell> newCell(new Cell(pos));
      row->m_colToCellMap.insert(std::map<int, std::shared_ptr<Cell> >::value_type(pos[0],newCell));
      return *newCell;
    }
    return *(row->m_colToCellMap.find(pos[0])->second);
  }

  //! the loading version
  int m_loadingVersion;
  //! the table name
  librevenge::RVNGString m_name;
  //! the page style name
  librevenge::RVNGString m_pageStyle;
  //! the maximum number of row
  int m_maxRow;
  //! the columns width
  std::vector<int> m_colWidthList;
  //! the rows heights in TWIP
  std::map<STOFFVec2i, int> m_rowHeightMap;
  //! map (min row, max row) -> rowContent
  std::map<STOFFVec2i, RowContent> m_rowToRowContentMap;
  //! a cell uses to return an empty cell
  Cell m_badCell;
};

Table::~Table()
{
}

////////////////////////////////////////
//! Internal: the state of a StarObjectSpreadsheet
struct State {
  //! constructor
  State()
    : m_model()
    , m_tableList()
    , m_sheetNames()
    , m_pageStyle("")
  {
  }
  //! the model
  std::shared_ptr<StarObjectModel> m_model;
  //! the actual table
  std::vector<std::shared_ptr<Table> > m_tableList;
  //! the sheet names
  std::vector<librevenge::RVNGString> m_sheetNames;
  //! the main page style
  librevenge::RVNGString m_pageStyle;
};

////////////////////////////////////////
//! Internal: the subdocument of a StarObjectSpreadsheet
class SubDocument final : public STOFFSubDocument
{
public:
  explicit SubDocument(librevenge::RVNGString const &text) :
    STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry()), m_text(text) {}

  //! destructor
  ~SubDocument() final {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const override
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    auto const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_text != sDoc->m_text) return true;
    return false;
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;

protected:
  //! the note text
  librevenge::RVNGString m_text;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (!m_text.empty())
    listener->insertUnicodeString(m_text);
  else
    listener->insertChar(' ');
}

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectSpreadsheet::StarObjectSpreadsheet(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_spreadsheetState(new StarObjectSpreadsheetInternal::State)
{
}

StarObjectSpreadsheet::~StarObjectSpreadsheet()
{
  cleanPools();
}

////////////////////////////////////////////////////////////
//
// send data
//
////////////////////////////////////////////////////////////
bool StarObjectSpreadsheet::updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages)
{
  if (m_spreadsheetState->m_tableList.empty()) return false;
  numPages=int(m_spreadsheetState->m_tableList.size());

  librevenge::RVNGString styleName("");
  int nPages=0;
  auto pool=const_cast<StarObjectSpreadsheet *>(this)->findItemPool(StarItemPool::T_SpreadsheetPool, false);
  StarState state(pool.get(), *this);
  for (size_t i=0; i<=m_spreadsheetState->m_tableList.size(); ++i) {
    bool isEnd=(i==m_spreadsheetState->m_tableList.size());
    if (!isEnd && m_spreadsheetState->m_tableList[i] && m_spreadsheetState->m_tableList[i]->m_pageStyle==styleName) {
      ++nPages;
      continue;
    }
    if (nPages) {
      auto const *style=(pool&&!styleName.empty()) ? pool->findStyleWithFamily(styleName, StarItemStyle::F_Page) : nullptr;
      if (!style && pool && !m_spreadsheetState->m_pageStyle.empty())
        style=pool->findStyleWithFamily(m_spreadsheetState->m_pageStyle, StarItemStyle::F_Page);
      state.m_global->m_page=STOFFPageSpan();
      state.m_global->m_page.m_pageSpan=nPages;
      if (style) {
        for (auto it : style->m_itemSet.m_whichToItemMap) {
          if (it.second && it.second->m_attribute)
            it.second->m_attribute->addTo(state);
        }
#if 0
        std::cerr << style->m_itemSet.printChild() << "\n";
#endif
      }
      pageSpan.push_back(state.m_global->m_page);
    }
    if (isEnd) break;
    styleName=m_spreadsheetState->m_tableList[i] ? m_spreadsheetState->m_tableList[i]->m_pageStyle : "";
    nPages=1;
  }
  return true;
}

bool StarObjectSpreadsheet::send(STOFFSpreadsheetListenerPtr listener)
{
  if (m_spreadsheetState->m_tableList.empty() || !listener) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::send: can not find the table\n"));
    return false;
  }
  // first creates the list of sheet names
  m_spreadsheetState->m_sheetNames.clear();
  for (auto const &t : m_spreadsheetState->m_tableList) {
    if (!t)
      m_spreadsheetState->m_sheetNames.push_back("");
    else
      m_spreadsheetState->m_sheetNames.push_back(t->m_name);
  }

  for (size_t t=0; t<m_spreadsheetState->m_tableList.size(); ++t) {
    if (t) listener->insertBreak(STOFFListener::PageBreak);
    if (!m_spreadsheetState->m_tableList[t]) continue;
    StarObjectSpreadsheetInternal::Table &sheet=*m_spreadsheetState->m_tableList[t];
    std::vector<int> repeated;
    std::vector<float> widths=sheet.getColumnWidths(repeated);
    listener->openSheet(widths, librevenge::RVNG_INCH, repeated, sheet.m_name);
    if (m_spreadsheetState->m_model)
      m_spreadsheetState->m_model->sendPage(int(t), listener);

    /* create a set to know which row needed to be send, each value of
       the set corresponding to a position where the rows change
       excepted the last position */
    std::set<int> newRowSet;
    for (auto it : sheet.m_rowToRowContentMap) {
      STOFFVec2i const &rows=it.first;
      newRowSet.insert(rows[0]);
      newRowSet.insert(rows[1]+1);
    }
    for (auto it : sheet.m_rowHeightMap) {
      STOFFVec2i const &rows=it.first;
      newRowSet.insert(rows[0]);
      newRowSet.insert(rows[1]+1);
    }

    for (auto it=newRowSet.begin(); it!=newRowSet.end();) {
      int row=*(it++);
      if (row<0) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::sendSpreadsheet: find a negative row %d\n", row));
        continue;
      }
      if (it==newRowSet.end())
        break;
      listener->openSheetRow(sheet.getRowHeight(row), librevenge::RVNG_POINT, *it-row);
      sendRow(int(t), row, listener);
      listener->closeSheetRow();
    }
    listener->closeSheet();
  }

  return true;
}

bool StarObjectSpreadsheet::sendRow(int table, int row, STOFFSpreadsheetListenerPtr listener)
{
  if (!listener || table<0 || table>=int(m_spreadsheetState->m_tableList.size()) || !m_spreadsheetState->m_tableList[size_t(table)]) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::send: can not find the table %d\n", table));
    return false;
  }
  auto &sheet=*m_spreadsheetState->m_tableList[size_t(table)];
  auto *rowC=sheet.getRow(row);
  if (!rowC) return true;
  rowC->compressItemList();

  // we need to go through the row style list and the cell list in parallel
  bool checkStyle=false;
  int actStyleCol=0;
  std::map<STOFFVec2i, std::shared_ptr<StarAttribute> >::const_iterator sIt;
  if (!rowC->m_colToAttributeMap.empty()) {
    checkStyle=true;
    sIt=rowC->m_colToAttributeMap.begin();
    actStyleCol=sIt->first[0];
  }
  bool checkCell=false;
  std::map<int, std::shared_ptr<StarObjectSpreadsheetInternal::Cell> >::iterator cIt;
  if (!rowC->m_colToCellMap.empty()) {
    checkCell=true;
    cIt=rowC->m_colToCellMap.begin();
  }

  StarObjectSpreadsheetInternal::Cell emptyCell;
  while (checkStyle || checkCell) {
    int newCol=checkCell ? cIt->first : -1;
    if (checkStyle && sIt->first[1] < actStyleCol) {
      ++sIt;
      checkStyle=sIt!=rowC->m_colToAttributeMap.end();
      actStyleCol=checkStyle ? sIt->first[0] : -1;
    }
    if (checkStyle && (!checkCell || actStyleCol<newCol)) {
      emptyCell.setPosition(STOFFVec2i(actStyleCol, row));
      int numRepeated=(checkCell && newCol<=sIt->first[1]) ? newCol-actStyleCol : sIt->first[1]-actStyleCol+1;
      sendCell(emptyCell, sIt->second ? sIt->second.get() : nullptr, table, numRepeated, listener);
      actStyleCol += numRepeated;
      continue;
    }
    if (!checkCell)
      break;
    if (checkStyle && newCol==actStyleCol) {
      sendCell(cIt->second ? *cIt->second : emptyCell, sIt->second ? sIt->second.get() : nullptr, table, 1, listener);
      ++actStyleCol;
    }
    else
      sendCell(cIt->second ? *cIt->second : emptyCell, nullptr, table, 1, listener);
    ++cIt;
    checkCell=cIt!=rowC->m_colToCellMap.end();
  }
  return true;
}

bool StarObjectSpreadsheet::sendCell(StarObjectSpreadsheetInternal::Cell &cell, StarAttribute *attrib, int table, int numRepeated, STOFFSpreadsheetListenerPtr listener)
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::send: can not find the listener\n"));
    return false;
  }
  if (attrib) {
    auto pool=findItemPool(StarItemPool::T_SpreadsheetPool, false);
    StarState state(pool.get(), *this);
    attrib->addTo(state);
    cell.setFont(state.m_font);
    cell.setCellStyle(state.m_cell);
    // checkme: we need the pool here
    getFormatManager()->updateNumberingProperties(cell);
  }
  if (!cell.m_content.m_formula.empty())
    StarCellFormula::updateFormula(cell.m_content, m_spreadsheetState->m_sheetNames, table);

  listener->openSheetCell(cell, cell.m_content, numRepeated);
  if (cell.m_content.m_contentType==STOFFCellContent::C_TEXT_BASIC)
    listener->insertUnicodeList(cell.m_content.m_text);
  else if (cell.m_content.m_contentType==STOFFCellContent::C_TEXT && cell.m_textZone)
    cell.m_textZone->send(listener);
  if (cell.m_hasNote) {
    std::shared_ptr<STOFFSubDocument> subDoc(new StarObjectSpreadsheetInternal::SubDocument(cell.m_notes[0]));
    listener->insertComment(subDoc, cell.m_notes[2], cell.m_notes[1]);
  }
  listener->closeSheetCell();
  return true;
}

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool StarObjectSpreadsheet::parse()
{
  if (!getOLEDirectory() || !getOLEDirectory()->m_input) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::parser: error, incomplete document\n"));
    return false;
  }
  auto &directory=*getOLEDirectory();
  StarObject::parse();
  auto unparsedOLEs=directory.getUnparsedOles();
  STOFFInputStreamPtr input=directory.m_input;
  StarFileManager fileManager;

  STOFFInputStreamPtr mainOle; // let store the StarCalcDocument to read it in last position
  std::string mainName;
  for (auto const &name : unparsedOLEs) {
    STOFFInputStreamPtr ole = input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::parse: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    auto pos = name.find_last_of('/');
    std::string base;
    if (pos == std::string::npos) base = name;
    else base = name.substr(pos+1);
    ole->setReadInverted(true);
    if (base=="SfxStyleSheets") {
      readSfxStyleSheets(ole,name);
      continue;
    }
    if (base=="StarCalcDocument") {
      mainOle=ole;
      mainName=name;
      continue;
    }
    if (base!="BasicManager2") {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::parse: find unexpected ole %s\n", name.c_str()));
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
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::parser: can not find the main calc document\n"));
    return false;
  }
  else
    readCalcDocument(mainOle,mainName);
  return true;
}

bool StarObjectSpreadsheet::readCalcDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWCalcDocument", getPassword()); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCCalcDocument):";
  // sc_docsh.cxx: ScDocShell::Load then sc_documen2.cxx ScDocument::Load
  uint16_t nId;
  *input>>nId;
  if ((nId>>8)!=0x42) {
    /* if the zone has a password, we can retrieve it knowing that nId must begin by 0x42

       TODO: we must also check if the user has given a password and
       if the user mask does correspond to the real mask.
    */
    input=StarEncryption::decodeStream(input, StarEncryption::getMaskToDecodeStream(uint8_t(nId>>8), 0x42));
    if (input) {
      zone.setInput(input);
      input->seek(0, librevenge::RVNG_SEEK_SET);
      *input>>nId;
    }
  }
  if ((nId!=0x4220 && nId!=0x422d) || !zone.openSCRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read the document id\n"));
    f << "###";
    ascFile.addPos(0);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  int version=0, maxRow=8191;
  while (!input->isEnd() && input->tell()<lastPos) {
    long pos=input->tell();
    uint16_t subId;
    *input>>subId;
    f.str("");
    f << "SCCalcDocument[" << std::hex << subId << std::dec << "]:";
    bool ok=false;
    switch (subId) {
    case 0x4222: {
      f << "table,";
      std::shared_ptr<StarObjectSpreadsheetInternal::Table> table;
      table.reset(new StarObjectSpreadsheetInternal::Table(version, maxRow));
      m_spreadsheetState->m_tableList.push_back(table);
      ok=readSCTable(zone, *table);
      break;
    }
    case 0x4224: {
      f << "rangeName,";
      // sc_rangenam.cxx ScRangeName::Load
      StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
      ok=scRecord.open();
      if (!ok) break;
      uint16_t count, sharedMaxIndex;
      if (version >= 3)
        *input >> sharedMaxIndex >> count;
      else {
        uint16_t dummy;
        *input >> sharedMaxIndex >> dummy >> count;
      }
      f << "index[sharedMax]=" << sharedMaxIndex << ";";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      for (int i=0; i<int(count); ++i)  {
        pos=input->tell();
        f.str("");
        f << "Entries(SCRange):";
        if (!scRecord.openContent("SCCalcDocument")) {
          f << "###";
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument:can not find some content\n"));
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          break;
        }
        // sc_rangenam.cxx ScRangeData::ScRangeData
        long endDataPos=scRecord.getContentLastPosition();
        std::vector<uint32_t> string;
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
          f << "###string";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          continue;
        }
        f << libstoff::getString(string).cstr() << ",";
        StarObjectSpreadsheetInternal::Cell cell;
        if (version >= 3) {
          uint32_t nPos;
          uint16_t rangeType, index;
          uint8_t nData;
          *input >> nPos >> rangeType >> index >> nData;
          auto row=int(nPos&0xFFFF);
          auto col=int((nPos>>16)&0xFF);
          auto table=int((nPos>>24)&0xFF);
          f << "pos=" << row << "x" << col;
          if (table) f << "x" << table;
          f << ",";
          f << "range[type]=" << rangeType << ",";
          f << "index=" << index << ",";
          if (nData&0xf) input->seek((nData&0xf), librevenge::RVNG_SEEK_CUR);
          if (!StarCellFormula::readSCFormula(zone, cell.m_content, version, endDataPos) || input->tell()>endDataPos)
            f << "###";
        }
        else {
          uint16_t row, col, table, tokLen, rangeType, index;
          *input >> col >> row >> table >> rangeType >> index >> tokLen;
          f << "pos=" << row << "x" << col;
          if (table) f << "x" << table;
          f << ",";
          f << "range[type]=" << rangeType << ",";
          f << "index=" << index << ",";
          if (tokLen &&
              (!StarCellFormula::readSCFormula3(zone, cell.m_content, version, endDataPos) || input->tell()>endDataPos))
            f << "###";
        }
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        scRecord.closeContent("SCCalcDocument");
      }
      scRecord.close("SCCalcDocument");
      pos=input->tell();
      break;
    }
    case 0x4225:
    case 0x4226:
    case 0x4227:
    case 0x422e:
    case 0x422f:
    case 0x4230:
    case 0x4231:
    case 0x4234:
    case 0x4239: {
      std::string what(subId==0x4225 ? "dbCollect" : subId==0x4226 ? "dbPivot" :
                       subId==0x4227 ? "chartCol" : subId==0x422e ? "ddeLinks" :
                       subId==0x422f ? "areaLinks" : subId==0x4230 ? "condFormats" :
                       subId==0x4231 ? "validation" : subId==0x4234 ? "detOp" : "dpCollection");
      f << what << ",";
      // sc_dbcolect.cxx ScDBCollection::Load and ...
      StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
      ok=scRecord.open();
      if (!ok) break;

      long endDataPos=zone.getRecordLastPosition();
      if (subId==0x4239 && input->readLong(4)!=6) {
        f << "###version";
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument:find unknown version\n"));
        input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
        scRecord.close("SCCalcDocument");
        break;
      }
      uint16_t count;
      *input >> count;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      bool isOk=true;
      std::vector<uint32_t> string;
      for (int i=0; i<int(count); ++i)  {
        pos=input->tell();
        f.str("");
        f << "SCCalcDocument:" << what << ",";
        if (!scRecord.openContent("SCCalcDocument")) {
          f << "###";
          isOk=false;
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument:can not find some content\n"));
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          break;
        }
        bool parsed=false, addDebugFile=false;
        long endData=scRecord.getContentLastPosition();
        switch (subId) {
        case 0x4225:
          parsed=readSCDBData(zone, version, endData);
          break;
        case 0x4226:
          parsed=readSCDBPivot(zone, version, endData);
          break;
        case 0x4227: {
          f << "chart,";
          parsed=addDebugFile=true;
          uint16_t nTab, nCol1, nRow1, nCol2, nRow2;
          *input >> nTab >> nCol1 >> nRow1 >> nCol2 >> nRow2;
          f << "dim=" << nCol1 << "x" << nRow1 << "<->" << nCol2 << "x" << nRow2 << "[" << nTab << "],";
          if (!zone.readString(string) || input->tell()>endData) {
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
            f << "###string";
          }
          else {
            if (!string.empty()) f << libstoff::getString(string).cstr() << ",";
            bool bColHeaders, bRowHeaders;
            *input >> bColHeaders >> bRowHeaders;
            if (bColHeaders) f << "col[headers],";
            if (bRowHeaders) f << "row[headers],";
          }
          break;
        }
        case 0x422e: {
          parsed=addDebugFile=true;
          for (int j=0; j<3; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
              f << "###string";
              parsed=false;
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "appl" : j==1 ? "topic" : "item") << "=" << libstoff::getString(string).cstr() << ",";
          }
          if (!parsed)
            break;
          bool hasValue;
          *input>>hasValue;
          if (hasValue && !readSCMatrix(zone, version, endData)) {
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a matrix\n"));
            f << "###matrix";
            parsed=false;
            break;
          }
          if (input->tell()>endData) f << "mode=" << input->readULong(1) << ",";
          break;
        }
        case 0x422f: { // checkme
          parsed=addDebugFile=true;
          for (int j=0; j<3; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
              f << "###string";
              parsed=false;
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "file" : j==1 ? "filter" : "source") << "=" << libstoff::getString(string).cstr() << ",";
          }
          if (!parsed)
            break;
          f << "range=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
          if (input->tell()<endData) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
              f << "###string";
              parsed=false;
              break;
            }
            else if (!string.empty())
              f << "options=" << libstoff::getString(string).cstr() << ",";
          }
          break;
        }
        case 0x4230: {
          addDebugFile=parsed=true;
          // sc_conditio.cxx ScConditionalFormat::ScConditionalFormat
          uint32_t key;
          uint16_t entryCount;
          *input >> key >> entryCount;
          f << "key=" << key << ",";
          if (!entryCount) break;
          scRecord.closeContent("SCCalcDocument");
          f << "entries=[";
          for (int e=0; e<int(entryCount); ++e) {
            f << "[";
            if (!scRecord.openContent("SCCalcDocument")) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument:can not find some content\n"));
              f << "###";
              isOk=parsed=false;
              break;
            }
            if (!zone.readString(string)) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument:can not read a string\n"));
              isOk=parsed=false;
              f << "###string,";
              break;
            }
            f << "],";
            scRecord.closeContent("SCCalcDocument");
          }
          f << "],";
          break;
        }
        case 0x4231: {
          addDebugFile=parsed=true;
          // sc_validat.cxx
          uint32_t key;
          uint16_t mode;
          bool showInput;
          *input >> key >> mode >> showInput;
          f << "key=" << key << ",";
          f << "mode=" << mode << ",";
          if (showInput) f << "show[input],";
          for (int j=0; j<2; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
              parsed=false;
              f << "###string";
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "title" : "message") << "=" << libstoff::getString(string).cstr() << ",";
          }
          if (!parsed) break;
          bool showError;
          *input >> showError;
          if (showError) f << "show[error],";
          for (int j=0; j<2; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
              parsed=false;
              f << "###string";
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "error[title]" : "error[message]") << "=" << libstoff::getString(string).cstr() << ",";
          }
          if (!parsed) break;
          f << "style[error]=" << input->readULong(2) << ",";
          break;
        }
        case 0x4234: {
          addDebugFile=parsed=true;
          // sc_detdata.cxx
          f << "range=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
          f << "op=" << input->readULong(2) << ",";
          break;
        }
        case 0x4239: {
          addDebugFile=parsed=true;
          // sc_dpobject.cxx ScDPObject::LoadNew
          uint8_t dType;
          *input >> dType;
          switch (dType) {
          case 0:
            f << "range=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
            parsed=readSCQueryParam(zone, version, endData);
            break;
          case 1:
            for (int j=0; j<2; ++j) {
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (string.empty()) continue;
              f << (j==0 ? "name" : "object") << "=" << libstoff::getString(string).cstr() << ",";
            }
            if (!parsed) break;
            f << "type=" << input->readULong(2) << ",";
            if (input->readULong(1)) f << "native,";
            break;
          case 2:
            for (int j=0; j<5; ++j) {
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (string.empty()) continue;
              static char const *wh[]= {"serviceName","source","name","user","pass"};
              f << wh[j] << "=" << libstoff::getString(string).cstr() << ",";
            }
            break;
          default:
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: unexpected sub type\n"));
            f << "###subType=" << int(dType) << ",";
            parsed=false;
            break;
          }
          if (!parsed) break;
          f << "out[range]=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
          // ScDPSaveData::Load
          uint32_t nDim;
          *input >> nDim;
          for (uint32_t j=0; j<nDim; ++j) {
            f << "dim" << j << "=[";
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
              parsed=false;
              f << "###string";
              break;
            }
            if (!string.empty()) f << libstoff::getString(string).cstr() << ",";
            bool isDataLayout, dupFlag, subTotalDef;
            uint16_t orientation, function, showEmptyMode, subTotalCount, extra;
            int32_t hierarchy;
            *input >> isDataLayout >> dupFlag >> orientation >> function >> hierarchy
                   >> showEmptyMode >> subTotalDef >> subTotalCount;
            if (isDataLayout) f << "isDataLayout,";
            if (dupFlag) f << "dupFlag,";
            if (orientation) f << "orientation=" << orientation << ",";
            if (function) f << "function=" << function << ",";
            if (hierarchy) f << "hierarchy=" << hierarchy << ",";
            if (showEmptyMode) f << "showEmptyMode=" << showEmptyMode << ",";
            if (subTotalDef) f << "subTotalDef,";
            if (input->tell()+2*long(subTotalCount)>endData) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read function list\n"));
              parsed=false;
              f << "###totalFuncs";
              break;
            }
            f << "funcs=[";
            for (int k=0; k<int(subTotalCount); ++k) f << input->readULong(2) << ",";
            f << "],";
            *input >> extra;
            if (extra) input->seek(extra, librevenge::RVNG_SEEK_CUR);
            uint32_t nMember;
            *input >> nMember;
            for (uint32_t k=0; k<nMember; ++k) {
              f << "member" << k << "[";
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (!string.empty()) f << libstoff::getString(string).cstr() << ",";
              uint16_t visibleMode, showDetailMode;
              *input >> visibleMode >> showDetailMode >> extra;
              if (visibleMode) f << "visibleMode=" << visibleMode << ",";
              if (showDetailMode) f << "showDetailMode=" << showDetailMode << ",";
              if (extra) input->seek(extra, librevenge::RVNG_SEEK_CUR);
              f << "],";
            }
            if (!parsed) break;
            f << "],";
          }
          if (!parsed) break;
          uint16_t colGrandMode, rowGrandMode, ignoreEmptyMode, repeatEmptyMode, extra;
          *input >> colGrandMode >> rowGrandMode >> ignoreEmptyMode >> repeatEmptyMode >> extra;
          f << "grandMode=" << colGrandMode << "x" << rowGrandMode << ",";
          if (ignoreEmptyMode) f << "ignoreEmptyMode=" << ignoreEmptyMode << ",";
          if (repeatEmptyMode) f << "repeatEmptyMode=" << repeatEmptyMode << ",";
          if (extra) input->seek(extra, librevenge::RVNG_SEEK_CUR);
          //
          if (input->tell()<endData) {
            for (int j=0; j<2; ++j) {
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (string.empty()) continue;
              f << (j==0 ? "tableName" : "tableTab") << "=" << libstoff::getString(string).cstr() << ",";
            }
          }
          break;
        }
        default:
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: unexpected type\n"));
          f << "###type,";
          addDebugFile=parsed=true;
          input->seek(endData, librevenge::RVNG_SEEK_SET);
          break;
        }
        if (!parsed) {
          addDebugFile=true;
          f << "###";
          input->seek(endData, librevenge::RVNG_SEEK_SET);
        }
        if (addDebugFile) {
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
        }
        if (scRecord.isContentOpened()) scRecord.closeContent("SCCalcDocument");
        if (!isOk) break;
      }
      if (isOk && subId==0x4225 && input->tell()<endDataPos) {
        pos=input->tell();
        f.str("");
        f << "SCCalcDocument:dbCollect, nEntry=" << input->readULong(2) << ",";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
      }
      scRecord.close("SCCalcDocument");
      pos=input->tell();
      break;
    }
    default:
      break;
    }
    if (ok) {
      if (pos!=input->tell()) {
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
      }
      continue;
    }
    input->seek(pos+2, librevenge::RVNG_SEEK_SET);
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not open the zone record\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endPos=zone.getRecordLastPosition();
    std::vector<uint32_t> string;
    switch (subId) {
    case 0x4221: {
      f << "docFlags,";
      uint16_t vers;
      *input>>vers;
      version=int(vers);
      f << "vers=" << vers << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty()) {
        m_spreadsheetState->m_pageStyle=libstoff::getString(string);
        f << "pageStyle=" << m_spreadsheetState->m_pageStyle.cstr() << ",";
      }
      f << "protected=" << input->readULong(1) << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "passwd=" << libstoff::getString(string).cstr() << ","; // the uncrypted table password, safe to ignore
      if (input->tell()<endPos) f << "language=" << input->readULong(2) << ",";
      if (input->tell()<endPos) f << "autoCalc=" << input->readULong(1) << ",";
      if (input->tell()<endPos) f << "visibleTab=" << input->readULong(2) << ",";
      if (input->tell()<endPos) {
        *input>>vers;
        version=int(vers);
        f << "vers=" << vers << ",";
      }
      if (input->tell()<endPos) {
        uint16_t nMaxRow;
        *input>>nMaxRow;
        maxRow=int(nMaxRow);
        if (nMaxRow!=8191) f << "maxRow=" << nMaxRow << ",";
      }
      break;
    }
    case 0x4223: {
      // sc_documen9.cxx ScDocument::LoadDrawLayer, sc_drwlayer.cxx ScDrawLayer::Load
      f << "drawing,";
      while (input->tell()<endPos) {
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        pos=input->tell();
        *input >> nId;
        f.str("");
        f << "SCCalcDocument[drawing-" << std::hex << nId << std::dec << "]:";
        if (!zone.openSCRecord()) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not open a record\n"));
          f << "###record";
          break;
        }
        switch (nId) {
        case 0x4260: {
          f << "pool,";
          auto pool=getNewItemPool(StarItemPool::T_XOutdevPool);
          pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));
          if (!pool->read(zone))
            input->seek(pos, librevenge::RVNG_SEEK_SET);
          break;
        }
        case 0x4261: {
          f << "sdrModel,";
          std::shared_ptr<StarObjectModel> model(new StarObjectModel(*this, true));
          if (model->read(zone)) {
            if (m_spreadsheetState->m_model) {
              STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: the model is already set\n"));
              f << "###model";
            }
            else
              m_spreadsheetState->m_model=model;
          }
          else
            input->seek(pos, librevenge::RVNG_SEEK_SET);
          break;
        }
        default:
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: find unexpected type\n"));
          f << "###unknown";
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
          break;
        }
        zone.closeSCRecord("SCCalcDocument");
      }
      break;
    }
    case 0x4228: {
      f << "numFormats,";
      if (!getFormatManager()->readNumberFormatter(zone))
        f << "###";
      break;
    }
    case 0x4229: {
      f << "docOptions,vers=" << version << ",";
      // sc_docoptio.cxx void ScDocOptions::Load
      bool bIsIgnoreCase, bIsIter=false;
      uint16_t nIterCount, nPrecStandardFormat, nDay, nMonth, nYear;
      double fIterEps;
      *input >> bIsIgnoreCase;
      if (version>=2) {
        uint8_t val;
        *input >> val;
        if (val!=0 && val!=1)
          input->seek(-1, librevenge::RVNG_SEEK_CUR);
        else
          bIsIter=(val!=0);
      }
      *input >> nIterCount;
      *input >> fIterEps >> nPrecStandardFormat >> nDay >> nMonth >> nYear;
      if (bIsIgnoreCase) f << "ignore[case],";
      if (bIsIter) f << "isIter,";
      if (nIterCount) f << "iterCount=" << nIterCount << ",";
      if (nPrecStandardFormat) f << "precStandartFormat=" << nPrecStandardFormat << ",";
      if (fIterEps<0||fIterEps>0) f << "iter[eps]=" << fIterEps << ",";
      f << "date=" << nMonth << "/" << nDay << "/" << nYear << ",";
      if (input->tell()+1==endPos)
        f << "unkn=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "tabs[distance]=" << input->readULong(2) << ",";
      if (input->tell()<endPos)
        f << "calc[asShown]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "match[wholeCell]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "autoSpell=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "lookUpColRowNames=" << input->readULong(1) << ",";
      if (input->tell()<endPos) {
        uint16_t nYear2000;
        *input >> nYear2000;
        if (input->tell()<endPos)
          *input >> nYear2000;
        else
          nYear2000 = uint16_t(nYear2000+1901);
        f << "year[2000]=" << nYear2000 << ",";
      }
      break;
    }
    case 0x422a: {
      f << "viewOptions,";
      // sc_viewopti.cxx operator>>(... ScViewOptions)
      for (int i=0; i<=9; ++i) {
        bool val;
        *input >> val;
        if (val) f << "opt" << i << ",";
      }
      for (int i=0; i<3; ++i) {
        uint8_t type;
        *input >> type;
        if (type) f << "type" << i << "=" << int(type) << ",";
      }
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a color\n"));
        f << "###color";
        break;
      }
      f << "col=" << col << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "name=" << libstoff::getString(string).cstr() << ",";
      if (input->tell()<endPos)
        f << "opt[helplines]=" << input->readULong(1) << ",";
      if (input->tell()<endPos) {
        // ScGridOptions operator >>
        uint32_t drawX, drawY, divisionX, divisionY, snapX, snapY;
        *input >> drawX >> drawY >> divisionX >> divisionY >> snapX >> snapY;
        f << "grid[draw]=" << drawX << "x" << drawY << ",";
        f << "grid[division]=" << divisionX << "x" << divisionY << ",";
        f << "grid[snap]=" << snapX << "x" << snapY << ",";
        bool useSnap, synchronize, visibleGrid, equalGrid;
        *input >> useSnap >> synchronize >> visibleGrid >> equalGrid;
        if (useSnap) f << "grid[useSnap],";
        if (synchronize) f << "grid[synchronize],";
        if (visibleGrid) f << "grid[visible],";
        if (equalGrid) f << "grid[equal],";
      }
      if (input->tell()<endPos)
        f << "hideAutoSpell=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[anchor]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[pageBreak]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[solidHandles]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[clipMarks]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[bigHandles]=" << input->readULong(1) << ",";
      break;
    }
    case 0x422b: {
      f << "printer,";
      StarFileManager fileManager;
      if (!fileManager.readJobSetUp(zone, false))
        break;
      // TODO store the page size
      break;
    }
    case 0x422c: {
      f << "charset,";
      auto guiType=int(input->readULong(1));
      f << "gui[type]=" << guiType << ",";
      zone.setGuiType(guiType);
      auto charSet=int(input->readULong(1));
      f << "set=" << charSet << ",";
      if (StarEncoding::getEncodingForId(charSet)!=StarEncoding::E_DONTKNOW)
        zone.setEncoding(StarEncoding::getEncodingForId(charSet));
      else
        f << "###,";
      break;
    }
    case 0x4232:
    case 0x4233: {
      f << (subId==4232 ? "colNameRange" : "rowNameRange") << ",";
      // sc_rangelst.cxx ScRangePairList::Load(
      uint32_t n;
      *input >> n;
      if (!n) break;
      f << "ranges=[";
      if (version<0x12) {
        if (uint32_t(endPos-input->tell())/8<n || input->tell()+8*long(n) > endPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read some ranges\n"));
          f << "###ranges";
          break;
        }
        for (uint32_t i=0; i<n; ++i)
          f << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
      }
      else {
        if (input->tell()+16*long(n) > endPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read some ranges\n"));
          f << "###ranges";
          break;
        }
        for (uint32_t i=0; i<n; ++i)
          f << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ":"
            << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
      }
      f << "],";
      break;
    }
    case 0x4235: {
      f << "consolidateParam,";
      uint8_t funct;
      uint16_t nCol, nRow, nTab, nCount;
      bool byCol, byRow, byReference;
      *input >> nCol >> nRow >> nTab >> byCol >> byRow >> byReference >> funct >> nCount;
      f << "cell=" << nCol << "x" << nRow << "x" << nTab << ",";
      if (byCol) f << "byCol,";
      if (byRow) f << "byRow,";
      if (byReference) f << "byReference,";
      f << "funct=" << funct << ",";
      if (!nCount) break;
      if (input->tell()+10*long(nCount) > endPos) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read some area\n"));
        f << "###area";
        break;
      }
      for (int i=0; i<int(nCount); ++i) {
        uint16_t nCol2, nRow2;
        *input >> nTab >> nCol >> nRow >> nCol2 >> nRow2;
        f << nCol << "x" << nRow << "<->" << nCol2 << "x" << nRow2 << "[" << nTab << "],";
      }
      break;
    }
    case 0x4236: {
      f << "changeTrack,";
      uint16_t loadVers;
      *input >> loadVers;
      f << "load[version]=" << loadVers << ",";
      if (loadVers&0xff00) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: unknown version\n"));
        f << "###version";
        break;
      }
      // sc_chgtrack.cxx ScChangeTrack::Load
      if (!readSCChangeTrack(zone,int(loadVers),endPos))
        f << "###";
      break;
    }
    case 0x4237: {
      f << "changeViewSetting,";
      // sc_chgviset.cxx ScChangeViewSettings::Load
      bool bShowIt, bIsDate, bIsAuthor, bEveryoneButMe, bIsRange;
      uint8_t dateMode;
      uint32_t date1, time1, date2, time2;
      *input >> bShowIt >> bIsDate >> dateMode >> date1 >> time1 >> date2 >> time2 >> bIsAuthor >> bEveryoneButMe;
      if (bShowIt) f << "show,";
      if (bIsDate) f << "isDate,";
      f << "mode[date]=" << int(dateMode) << ",";
      f << "firstDate=" << date1 << ",";
      f << "firstTime=" << time1 << ",";
      f << "lastDate=" << date2 << ",";
      f << "lastTime=" << time2 << ",";
      if (bIsAuthor) f << "isAuthor,";
      if (bEveryoneButMe) f << "everyoneButMe,";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "author=" << libstoff::getString(string).cstr() << ",";
      *input >> bIsRange;
      if (bIsRange) f << "isRange,";
      if (!zone.openSCRecord()) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: can not open the range list record\n"));
        f << "###";
        break;
      }
      uint32_t nRange;
      *input >> nRange;
      if (uint32_t(zone.getRecordLastPosition()-input->tell())<nRange ||
          static_cast<unsigned long>(input->tell())+8*static_cast<unsigned long>(nRange)<=static_cast<unsigned long>(zone.getRecordLastPosition())) {
        f << "ranges=[";
        for (uint32_t j=0; j<nRange; ++j)
          f << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
        f << "],";
      }
      else {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readCalcDocument: bad num rangen"));
        f << "###nRange=" << nRange << ",";
      }
      zone.closeSCRecord("SCCalcDocument");
      if (input->tell()<lastPos) {
        bool bShowAccepted, bShowRejected;
        *input >> bShowAccepted >> bShowRejected;
        if (bShowAccepted) f << "show[accepted],";
        if (bShowRejected) f << "show[rejected],";
      }
      if (input->tell()<lastPos) {
        bool isComment;
        *input >> isComment;
        if (isComment) f << "isComment,";
      }
      break;
    }
    case 0x4238:
      f << "linkUp[mode]=" << input->readULong(1) << ",";
      break;
    default:
      f << "##";
      input->seek(endPos, librevenge::RVNG_SEEK_SET);
    }
    if (pos!=endPos) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    zone.closeSCRecord("SCCalcDocument");
  }
  zone.closeSCRecord("SCCalcDocument");
  return true;
}
catch (...)
{
  return false;
}

bool StarObjectSpreadsheet::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  std::shared_ptr<StarItemPool> mainPool;
  // sc_docsh.cxx ScDocShell::LoadCalc and sc_document.cxx: ScDocument::LoadPool
  long pos=0;
  libstoff::DebugStream f;
  uint16_t nId;
  *input >> nId;
  f << "Entries(SfxStyleSheets):id=" << std::hex << nId << std::dec << ",calc,";
  if (getDocumentKind()!=STOFFDocument::STOFF_K_SPREADSHEET || !zone.openSCRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSfxStyleSheets: can not open main zone\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  auto oldEncoding=zone.getEncoding();
  while (input->tell()+6 < lastPos) {
    pos=input->tell();
    f.str("");
    *input >> nId;
    f << "SfxStyleSheets-1:id=" << std::hex << nId << std::dec << ",";
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSfxStyleSheets: can not open second zone\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    switch (nId) {
    case 0x4211:
    case 0x4214: {
      f << (nId==0x4211 ? "pool" : "pool[edit]") << ",";
      auto pool=getNewItemPool(nId==0x4211 ? StarItemPool::T_SpreadsheetPool : StarItemPool::T_EditEnginePool);
      if (pool && pool->read(zone)) {
        if (nId==0x4214 || !mainPool) mainPool=pool;
      }
      else {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSfxStyleSheets: can not readPoolZone\n"));
        f << "###";
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        break;
      }
      break;
    }
    case 0x4212: {
      f << "style[pool],";
      if (!mainPool || !mainPool->readStyles(zone, *this)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSfxStyleSheets: can not readStylePool\n"));
        f << "###";
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      }
      break;
    }
    case 0x422c: {
      uint8_t cSet, cGUI;
      *input >> cGUI >> cSet;
      if (cSet) {
        f << "charset=" << int(cSet) << ",";
        if (StarEncoding::getEncodingForId(cSet)!=StarEncoding::E_DONTKNOW)
          zone.setEncoding(StarEncoding::getEncodingForId(cSet));
      }
      if (cGUI) {
        zone.setGuiType(cGUI);
        f << "gui=" << int(cGUI) << ",";
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSfxStyleSheets: find unexpected tag\n"));
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      f << "###";
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCRecord("SfxStyleSheets");
  }
  zone.closeSCRecord("SfxStyleSheets");
  zone.setEncoding(oldEncoding);

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSfxStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SfxStyleSheets:###extra");
  }
  if (mainPool) mainPool->updateStyles();
  return true;
}

////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////
bool StarObjectSpreadsheet::readSCTable(StarZone &zone, StarObjectSpreadsheetInternal::Table &table)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // sc_table2.cxx ScTable::Load
  if (!zone.openSCRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(SCTable)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    uint16_t id;
    *input>>id;
    f.str("");
    f << "SCTable[" << std::hex << id << std::dec << "]:";
    if (id==0x4240) {
      StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
      f << "columns,";
      if (!scRecord.open()) {
        input->seek(pos,librevenge::RVNG_SEEK_SET);
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not find the column header \n"));
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        break;
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      int nCol=0;
      long endDataPos=zone.getRecordLastPosition();
      while (input->tell()<endDataPos) {
        if (table.getLoadingVersion()>=6) {
          pos=input->tell();
          nCol=int(input->readULong(1));
          f.str("");
          f << "SCTable:C" << nCol << ",";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
        }
        else if (nCol>table.getMaxCols())
          break;
        pos=input->tell();
        if (!scRecord.openContent("SCTable")) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not open a column \n"));
          ascFile.addPos(pos);
          ascFile.addNote("SCTable-C###");
          break;
        }
        if (!readSCColumn(zone,table, nCol, scRecord.getContentLastPosition())) {
          ascFile.addPos(pos);
          ascFile.addNote("SCTable-C###");
          input->seek(scRecord.getContentLastPosition(), librevenge::RVNG_SEEK_SET);
        }
        scRecord.closeContent("SCTable");
        ++nCol;
      }
      scRecord.close("SCTable");
      continue;
    }
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not open the zone record\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endDataPos=zone.getRecordLastPosition();
    switch (id) {
    case 0x4241: { // SCID_COLROWFLAGS
      f << "dim,";
      f << "col[width]=[";
      uint16_t rep;
      bool ok=true;
      for (int i=0; i<=table.getMaxCols();) {
        uint16_t val;
        *input>>rep >> val;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << val << "x" << rep << ",";
        else if (rep==1)
          f << val << ",";
        for (int j=0; j<int(rep); ++j) {
          if (i+j>int(rep)+table.getMaxCols())
            break;
          table.m_colWidthList.push_back(int(val));
        }
        i+=int(rep);
      }
      f << "],col[flag]=[";
      for (int i=0; i<=table.getMaxCols();) {
        uint8_t flags;
        *input>>rep >> flags;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << int(flags) << "x" << (rep+1) << ",";
        else if (rep)
          f << int(flags) << ",";
        i+=int(rep);
      }
      f << "],row[height]=[";
      for (int i=0; i<=table.getMaxRows();) {
        uint16_t val;
        *input>>rep >> val;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << val << "x" << (rep+1) << ",";
        else if (rep==1)
          f << val << ",";
        if (rep>0) {
          table.m_rowHeightMap[STOFFVec2i(i,i+rep-1)]=int(val);
          i+=int(rep);
        }
      }
      f << "],row[flag]=[";
      for (int i=0; i<=table.getMaxRows();) {
        uint8_t flags;
        *input>>rep >> flags;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << int(flags) << "x" << (rep+1) << ",";
        else if (rep==1)
          f << int(flags) << ",";
        i+=int(rep);
      }
      f << "],";
      if (!ok) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: somethings went bad\n"));
        f << "###bound";
        break;
      }
      break;
    }
    case 0x4242: { // SCID_TABOPTIONS
      f << "tabOptions,";
      bool ok=true;
      bool bVal;
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not read a string\n"));
          f << "###string" << i;
          ok=false;
          break;
        }
        if (!string.empty()) {
          static char const *wh[]= {"name", "comment", "pass"};
          f << wh[i] << "=" << libstoff::getString(string).cstr() << ",";
          if (i==0 && !m_spreadsheetState->m_tableList.empty() && m_spreadsheetState->m_tableList.back())
            m_spreadsheetState->m_tableList.back()->m_name=libstoff::getString(string);
        }
        if (i==2) break;
        *input>>bVal;
        if (bVal) f << (i==0 ? "scenario," : "protected") << ",";
      }
      if (!ok) break;
      *input>> bVal;
      if (bVal) {
        f << "outlineTable,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        for (int i=0; i<2; ++i) {
          pos=input->tell();
          if (!readSCOutlineArray(zone)) {
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not open the zone record\n"));
            ok=false;
            input->seek(pos,librevenge::RVNG_SEEK_SET);
            break;
          }
        }
        f.str("");
        f << "SCTable[tabOptionsB]:";
        pos=input->tell();
        if (!ok) {
          f << "###outline,";
          break;
        }
      }
      if (input->tell()<endDataPos) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not read a string\n"));
          f << "###pageStyle";
          break;
        }
        table.m_pageStyle=libstoff::getString(string);
        if (!string.empty())
          f << "pageStyle=" << table.m_pageStyle.cstr() << ",";
      }
      if (input->tell()<endDataPos) {
        for (int i=0; i<3; ++i) {
          *input >> bVal;
          if (!bVal) continue;
          f << "range" << i << "=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
        }
      }
      if (input->tell()<endDataPos)
        f << "isVisible=" << input->readULong(1);
      if (input->tell()<endDataPos) {
        uint16_t nCount;
        *input>>nCount;
        if (input->tell()+8*long(nCount)>endDataPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: bad nCount\n"));
          f << "###nCount=" << nCount << ",";
          break;
        }
        f << "ranges=[";
        for (int i=0; i<int(nCount); ++i)
          f << "range" << i << "=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
        f << "],";
      }
      if (input->tell()<endDataPos) {
        STOFFColor color;
        if (!input->readColor(color)) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not read a color\n"));
          f << "###color";
          break;
        }
        f << "scenario[color]=" << color << ",";
        f << "scenario[flags]=" << input->readULong(2) << ",";
        f << "scenario[active]=" << input->readULong(1) << ",";
      }
      break;
    }
    case 0x4243: {
      f << "tabLink,";
      f << "mode=" << input->readULong(1) << ",";
      bool ok=true;
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not read a string\n"));
          f << "###string" << i;
          ok=false;
          break;
        }
        if (string.empty()) continue;
        static char const *wh[]= {"doc", "flt", "tab"};
        f << "link[" << wh[i] << "]=" << libstoff::getString(string).cstr() << ",";
      }
      if (!ok) break;
      if (input->tell()<endDataPos)
        f << "bRelUrl=" << input->readULong(1) << ",";
      if (input->tell()<endDataPos) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: can not read a string\n"));
          f << "###opt";
          break;
        }
        if (string.empty()) continue;
        f << "link[opt]=" << libstoff::getString(string).cstr() << ",";
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCTable: find unexpected type\n"));
      f << "###";
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCRecord("SCTable");
  }
  zone.closeSCRecord("SCTable");
  return true;
}

bool StarObjectSpreadsheet::readSCColumn(StarZone &zone, StarObjectSpreadsheetInternal::Table &table,
    int column, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // sc_column2.cxx ScColumn::Load
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCColumn)-C" << column << "[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    uint16_t id;
    *input>>id;
    f.str("");
    f << "SCColumn[" << std::hex << id << std::dec << "]:";
    if (id==0x4250 && readSCData(zone,table,column)) {
      f << "data,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      continue;
    }
    input->seek(pos+2, librevenge::RVNG_SEEK_SET);
    bool ok=zone.openSCRecord();
    if (!ok || zone.getRecordLastPosition()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCColumn:can not open record\n"));
      f << "###";
      if (ok) zone.closeSCRecord("SCColumn");
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endDataPos=zone.getRecordLastPosition();
    switch (id) {
    case 0x4251: {
      f << "notes,";
      uint16_t nCount;
      *input >> nCount;
      f << "n=" << nCount << ",";
      for (int i=0; i<nCount; ++i) {
        auto row=int(input->readULong(2));
        f << "note" << i << "[R" << row << ",";
        auto &cell=table.getCell(STOFFVec2i(column, row));
        // sc_cell.cxx ScBaseCell::LoadNotes, ScPostIt operator>>
        for (int j=0; j<3; ++j) {
          if (!zone.readString(string)||input->tell()>endDataPos) {
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCColumn:can not read a string\n"));
            f << "###string" << j;
            ok=false;
            break;
          }
          if (string.empty()) continue;
          static char const *wh[]= {"note","date","author"};
          cell.m_notes[j]=libstoff::getString(string);
          f << wh[j] << "=" << cell.m_notes[j].cstr()  << ",";
        }
        if (!ok) break;
        cell.m_hasNote=true;
        f << "],";
      }
      break;
    }
    case 0x4252: {
      f << "attrib,";
      // sc_attarray.cxx ScAttrArray::Load
      uint16_t nCount;
      *input >> nCount;
      f << "n=" << nCount << ",";
      std::shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_SpreadsheetPool, false);
      if (!pool) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCColumn:can not read the spreadsheet pool, create a new one\n"));
        pool=getNewItemPool(StarItemPool::T_SpreadsheetPool);
      }
      f << "attrib=[";
#if 0
      std::cerr << "Attrib\n";
#endif
      for (int i=0,row=0; i<nCount; ++i) {
        auto newRow=int(input->readULong(2));
        f << newRow << ":";
        uint16_t nWhich=149;//StarAttribute::ATTR_SC_PATTERN-3;
        auto item=pool->loadSurrogate(zone, nWhich, false, f);
        if (!item || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCColumn:can not read a attrib\n"));
          f << "###attrib";
          break;
        }
        if (!item->m_attribute) {
          row=newRow+1;
          continue;
        }
#if 0
        libstoff::DebugStream f2;
        std::set<StarAttribute const *> done;
        item->m_attribute->print(f2, done);
        std::cerr << "\tC" << column << "x" << STOFFVec2i(row, newRow) << ":" << f2.str().c_str() << "[" << item->m_attribute.get() << "]\n";
#endif
        if (newRow>=row) {
          table.updateRowsBlocks(STOFFVec2i(row, newRow));
          auto it= table.m_rowToRowContentMap.lower_bound(STOFFVec2i(-1,row));
          while (it!=table.m_rowToRowContentMap.end() && it->first[1]<=newRow) {
            it->second.m_colToAttributeMap[STOFFVec2i(column, column)]=item->m_attribute;
            ++it;
          }
          row=newRow+1;
        }
      }
      f << "],";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCColumn: find unexpected type\n"));
      f << "###";
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCRecord("SCColumn");
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  return true;
}

bool StarObjectSpreadsheet::readSCData(StarZone &zone, StarObjectSpreadsheetInternal::Table &table, int column)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // sc_column2.cxx ScColumn::LoadData
  StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
  if (!scRecord.open()) {
    input->seek(pos,librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCData)[C" << column << "-" << zone.getRecordLevel() << "]:" << scRecord;
  auto count=int(input->readULong(2));
  f << "count=" << count << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  int const version=table.getLoadingVersion();
  for (int i=0; i<count; ++i) {
    pos=input->tell();
    f.str("");
    f << "SCData-" << i << ":";
    if (input->tell()+4>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCData:can not read some data\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    auto row=int(input->readULong(2));
    f << "row=" << row << ",";
    uint8_t what;
    *input>>what;
    bool ok=true;
    auto &cell=table.getCell(STOFFVec2i(column, row));
    STOFFCell::Format format=cell.getFormat();
    switch (what) {
    case 1: { // value
      // sc_cell2.cxx
      f << "value,";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      double value;
      *input >> value;
      format.m_format=STOFFCell::F_NUMBER;
      cell.m_content.m_contentType=STOFFCellContent::C_NUMBER;
      cell.m_content.setValue(value);
      f << "val=" << value << ",";
      break;
    }
    case 2:
    case 6: {
      // sc_cell2.cxx
      f << (what==2 ? "string" : "symbol") << ",";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCData: can not open some text \n"));
        f << "###text";
        ok=false;
        break;
      }
      // checkme: never seems what==6, so unsure...
      format.m_format=STOFFCell::F_TEXT;
      cell.m_content.m_contentType=STOFFCellContent::C_TEXT_BASIC;
      cell.m_content.m_text=text;
      f << "val=" << libstoff::getString(text).cstr() << ",";
      break;
    }
    case 3: { // TODO
      // sc_cell.cxx
      f << "formula,";
      if (!scRecord.openContent("SCData")) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCData: can not open a formula \n"));
        f << "###formula";
        ok=false;
        break;
      }
      long endDataPos=scRecord.getContentLastPosition();
      if (version>=8) {
        auto cData=int(input->readULong(1));
        if ((cData&0x10) && (cData&0xf)>=4) {
          f << "format=" <<input->readULong(4)<<",";
          cData-=4;
        }
        if (cData&0xf) input->seek((cData&0xf), librevenge::RVNG_SEEK_CUR);
        uint8_t cFlags;
        *input >> cFlags;
        f << "formatType=" << input->readLong(2) << ",";
        if (cFlags&8) {
          double ergValue;
          *input >> ergValue;
          format.m_format=STOFFCell::F_NUMBER;
          cell.m_content.m_contentType=STOFFCellContent::C_NUMBER;
          cell.m_content.setValue(ergValue);
          f << "ergValue=" << ergValue << ",";
        }
        if (cFlags&0x10) {
          std::vector<uint32_t> text;
          if (!zone.readString(text)) {
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCData: can not open some text\n"));
            f << "###text";
            ascFile.addDelimiter(input->tell(),'|');
            input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
            scRecord.closeContent("SCData");
            break;
          }
          else if (!text.empty()) {
            format.m_format=STOFFCell::F_TEXT;
            cell.m_content.m_contentType=STOFFCellContent::C_TEXT_BASIC;
            cell.m_content.m_text=text;
            f << "val=" << libstoff::getString(text).cstr() << ",";
          }
        }
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        pos=input->tell();
        f.str("");
        f << "SCData[formula]:";

        if (!StarCellFormula::readSCFormula(zone, cell.m_content, version, endDataPos) || input->tell()>endDataPos) {
          f << "###";
          scRecord.closeContent("SCData");
          ascFile.addDelimiter(input->tell(),'|');
          input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
          break;
        }
        pos=input->tell();
        if ((cFlags&3)==1 && input->tell()<endDataPos)
          f << "cols=" << input->readULong(2) << ",rows=" << input->readULong(2) << ",";
      }
      else {
        if (version>=2) input->seek(2, librevenge::RVNG_SEEK_CUR);
        f << "matrix[flags]=" << input->readULong(1) << ",";
        uint16_t codeLen;
        *input>>codeLen;
        if (codeLen && (!StarCellFormula::readSCFormula3(zone, cell.m_content, version, endDataPos) || input->tell()>endDataPos))
          f << "###";
      }
      if (input->tell()!=endDataPos) {
        f << "##";
        ascFile.addDelimiter(input->tell(),'|');
        input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
      }
      scRecord.closeContent("SCData");
      break;
    }
    case 4: {
      // sc_cell2.cxx
      f << "note";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      break;
    }
    case 5: { // TODO
      // sc_cell2.cxx
      f << "edit";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      std::shared_ptr<StarObjectSmallText> textZone(new StarObjectSmallText(*this, true));
      if (!textZone->read(zone, lastPos) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCData: can not open some edit text \n"));
        f << "###edit";
        ok=false;
        break;
      }
      format.m_format=STOFFCell::F_TEXT;
      cell.m_content.m_contentType=STOFFCellContent::C_TEXT;
      cell.m_textZone=textZone;
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCData: find unexpected type\n"));
      f << "###type=" << what;
      ok=false;
      break;
    }
    cell.setFormat(format);

    if (!ok || pos!=input->tell()) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    if (!ok) break;
  }
  scRecord.close("SCData");
  return true;
}

// sc_chgtrack.cxx ScChangeActionContent::ScChangeActionContent

bool StarObjectSpreadsheet::readSCChangeTrack(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCChangeTrack)[" << zone.getRecordLevel() << "]:";
  // sc_chgtrack.cxx ScChangeTrack::Load

  if (!zone.openSCRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not find string collection\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###strings";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  bool bDup;
  uint16_t count, limit, delta;
  *input >> bDup >> count >> limit >> delta;
  if (bDup) f << "bDups,";
  if (limit) f << "limit=" << limit << ",";
  if (delta) f << "delta=" << delta << ",";
  long endData=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  for (uint16_t i=0; i<count; ++i) {
    if (!zone.readString(string) || input->tell() > endData) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not open some string\n"));
      f << "###string";
      ascFile.addDelimiter(input->tell(),'|');
      input->seek(endData, librevenge::RVNG_SEEK_SET);
      break;
    }
    else if (!string.empty())
      f << "string" << i << "=" << libstoff::getString(string).cstr() << ",";
  }
  zone.closeSCRecord("SCChangeTrack");

  uint32_t nCount, nActionMax, nLastAction, nGeneratedCount;
  *input >> nCount >> nActionMax >> nLastAction >> nGeneratedCount;
  if (nCount) f << "count=" << nCount << ",";
  if (nActionMax) f << "actionMax=" << nActionMax << ",";
  if (nLastAction) f << "lastAction=" << nLastAction << ",";

  for (int s=0; s<2; ++s) {
    if (s==1) {
      pos=input->tell();
      f.str("");
      f << "Entries(SCChangeTrack)[A]:";
    }
    StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
    if (!scRecord.open()) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not find the action list\n"));
      ascFile.addDelimiter(input->tell(),'|');
      f << "###actions";
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (uint32_t i=0; i<(s==0 ? nGeneratedCount:nCount); ++i) {
      f.str("");
      f << "Entries(SCChangeTrack)[A" << i << "]:";
      if (!scRecord.openContent("SCChangeTrack")) {
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack:can not read an action\n"));
        break;
      }
      endData=scRecord.getContentLastPosition();

      uint8_t type;
      *input >> type;
      uint32_t col, row, tab, date, time, action, rejAction, state;
      *input >> col >> row >> tab >> date >> time >> action >> rejAction >> state;
      f << "cell=" << col << "x" << row << "[" << tab << "],";
      f << "date=" << date << ",";
      f << "time=" << time << ",";
      if (action) f << "action=" << action << ",";
      if (rejAction) f << "rejAction=" << rejAction << ",";
      if (state) f << "state=" << state << ",";
      if (!zone.readString(string) || input->tell() > endData) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not open some string\n"));
        f << "###string";
        ascFile.addDelimiter(input->tell(),'|');
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        scRecord.closeContent("SCChangeTrack");
        continue;
      }
      if (!string.empty())
        f << "comment" << i << "=" << libstoff::getString(string).cstr() << ",";
      if (s==0 && type!=8) {
        f << "###type";
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack:the type seems bad\n"));
      }
      switch (type) {
      case 1:
      case 2:
      case 3:
      case 9:
        f << "insert[" << (type==1 ? "cols" : type==2 ? "rows" : type==9 ? "tab" : "reject") << "],";
        break;
      case 4:
      case 5:
      case 6: {
        f << "delete[" << (type==4 ? "cols" : type==5 ? "rows" : "tab") << "],";
        uint32_t pCutOff;
        uint16_t cutOff, dx, dy;
        *input >> pCutOff >> cutOff >> dx >> dy;
        if (pCutOff) f << "pCutOff=" << pCutOff << ",";
        if (cutOff) f << "cutOff=" << cutOff << ",";
        f << "delta=" << dx << "x" << dy << ",";
        break;
      }
      case 7:
        f << "move,";
        *input >> col >> row >> tab;
        f << "fromCell=" << col << "x" << row << "[" << tab << "],";
        break;
      case 8: {
        bool ok=true;
        for (int j=0; j<2; ++j) {
          if (!zone.readString(string) || input->tell() > endData) {
            STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not open some string\n"));
            f << "###string";
            ok=false;
            break;
          }
          if (string.empty()) continue;
          f << (j==0 ? "oldValue" : "newValue") << "=" << libstoff::getString(string).cstr() << ",";
        }
        if (!ok) break;
        uint32_t oldContent, newContent;
        *input >> oldContent >> newContent;
        if (oldContent) f << "oldContent=" << oldContent << ",";
        if (newContent) f << "newContent=" << newContent << ",";
        StarObjectSpreadsheetInternal::ScMultiRecord scRecord2(zone);
        if (!scRecord2.open()) {
          STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not find the cell action\n"));
          f << "###cells";
          break;
        }
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: reading cell action is not implemented\n"));
        f << "###cells,";
        // fixme: call readSCData with only one cell
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        scRecord2.close("SCChangeTrack");
        break;
      }
      default:
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack:unknown type\n"));
        f << "###type=" << int(type) << ",";
        break;
      }
      scRecord.closeContent("SCChangeTrack");
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    scRecord.close("SCChangeTrack");
  }

  pos=input->tell();
  f.str("");
  f << "Entries(SCChangeTrack)[L]:";
  StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
  if (!scRecord.open()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: can not find the action list\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###link";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  while (scRecord.openContent("SCChangeTrack")) {
    pos=input->tell();
    f.str("");
    f << "Entries(SCChangeTrack)[L]:###";
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: reading the action links is not implemented\n"));
      first=false;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(scRecord.getContentLastPosition(), librevenge::RVNG_SEEK_SET);
    scRecord.closeContent("SCChangeTrack");
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCChangeTrack: find extra data\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;

}

bool StarObjectSpreadsheet::readSCDBData(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCDBData)[" << zone.getRecordLevel() << "]:";
  // sc_dbcolect.cxx ScDBData::Load
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some text\n"));
    f << "###name";
    ascFile.addDelimiter(input->tell(),'|');
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  f << "name=" << libstoff::getString(string).cstr() << ",";
  uint16_t nTable, nStartCol, nStartRow, nEndCol, nEndRow;
  *input >> nTable >> nStartCol >> nStartRow >> nEndCol >> nEndRow;
  if (nTable) f << "table=" << nTable << ",";
  f << "dim=" << nStartCol << "x" << nStartRow << "<->" << nEndCol << "x" << nEndRow << ",";
  bool bByRow, bHasHeader, bSortCaseSens, bIncludePattern, bSortInplace;
  *input >> bByRow >> bHasHeader >> bSortCaseSens >> bIncludePattern >> bSortInplace;
  if (bByRow) f << "byRow,";
  if (bHasHeader) f << "hasHeader,";
  if (bSortCaseSens) f << "sortCaseSens,";
  if (bIncludePattern) f << "includePattern,";
  if (bSortInplace) f << "sortInPlace,";
  uint16_t nSortDestTab, nSortDestCol, nSortDestRow;
  *input >> nSortDestTab >> nSortDestCol >> nSortDestRow;
  f << "dest[sort]=" << nSortDestCol << "x" << nSortDestCol << "x" << nSortDestTab << ",";
  bool bQueryInplace, bQueryCaseSen, bQueryRegExp, bQueryDuplicate;
  *input >> bQueryInplace >> bQueryCaseSen >> bQueryRegExp >> bQueryDuplicate;
  if (bQueryInplace) f << "query[inPlace],";
  if (bQueryCaseSen) f << "query[caseSen],";
  if (bQueryRegExp) f << "query[regExp],";
  if (bQueryDuplicate) f << "query[duplicate],";
  uint16_t nQueryDestTab, nQueryDestCol, nQueryDestRow;
  *input >> nQueryDestTab >> nQueryDestCol >> nQueryDestRow;
  f << "dest[query]=" << nQueryDestCol << "x" << nQueryDestCol << "x" << nQueryDestTab << ",";
  bool bSubRemoveOnly, bSubReplace, bSubPagebreak, bSubCaseSens, bSubDoSort,
       bSubAscending, bSubIncludePattern, bSubUserDef;
  bool bDBImport, bDBNative;
  *input >> bSubRemoveOnly >> bSubReplace >> bSubPagebreak >> bSubCaseSens
         >> bSubDoSort >> bSubAscending >> bSubIncludePattern >> bSubUserDef
         >> bDBImport;
  if (bSubRemoveOnly) f << "subRemoveOnly,";
  if (bSubReplace) f << "subReplace,";
  if (bSubPagebreak) f << "subPagebreak,";
  if (bSubCaseSens) f << "subCaseSens,";
  if (bSubDoSort) f << "subDoSort,";
  if (bSubAscending) f << "subAscending,";
  if (bSubIncludePattern) f << "subIncludePattern,";
  if (bSubUserDef) f << "subUserDef,";
  if (bDBImport) f << "dbImport,";
  for (int i=0; i<2; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some text\n"));
      f << "###name";
      ascFile.addDelimiter(input->tell(),'|');
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (!string.empty())
      f << (i==0 ? "dbName" : "dbStatement") << "=" << libstoff::getString(string).cstr() << ",";
  }
  *input >> bDBNative;
  if (bDBNative) f << "dbNative,";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f.str("");
  f << "SCDBData:";
  if (input->tell()+3*4>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read sort data\n"));
    f << "###name";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  for (int i=0; i<3; ++i) {
    bool doSort, doAscend;
    uint16_t sortField;
    *input>>doSort>>sortField >> doAscend;
    if (doSort) f << "sort" << i << "=" << sortField << "[" << int(doAscend) << "],";
  }
  for (int i=0; i<8; ++i) {
    bool doQuery, queryByString;
    uint16_t queryField;
    uint8_t queryOp, queryConnect;
    double val;
    *input >> doQuery >> queryField >> queryOp >> queryByString;
    if (!zone.readString(string)||input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some text\n"));
      f << "###name";
      ascFile.addDelimiter(input->tell(),'|');
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    *input>>val >> queryConnect;
    if (!doQuery) continue;
    f << "query" << i << "=[";
    f << libstoff::getString(string).cstr() << ",";
    f << "field=" << queryField << ",";
    f << "op=" << int(queryOp) << ",";
    if (queryByString) f << "byString,";
    if (!string.empty()) f << libstoff::getString(string).cstr() << ",";
    if (val<0 || val>0) f << "val=" << val << ",";
    f << "connect=" << int(queryConnect) << ",";
    f << "],";
  }
  for (int i=0; i<3; ++i) {
    bool doSubTotal;
    uint16_t field, count;
    *input >> doSubTotal >> field >> count;
    if (input->tell() + 3*long(count) > lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some subTotal\n"));
      f << "###subTotal";
      ascFile.addDelimiter(input->tell(),'|');
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (!doSubTotal && !count) continue;
    f << "subTotal" << i << "=[";
    f << "field=" << field  << ",[";
    for (int c=0; c<int(count); ++c)
      f << input->readULong(2) << ":" << input->readULong(1) << ",";
    f << "],";
  }
  if (input->tell()<lastPos)
    f << "index=" << input->readULong(2) << ",";
  if (input->tell()<lastPos)
    f << "dbSelect=" << input->readULong(1) << ",";
  if (input->tell()<lastPos)
    f << "dbSQL=" << input->readULong(1) << ",";
  if (input->tell()<lastPos) {
    f << "subUserIndex=" << input->readULong(2) << ",";
    f << "sortUserDef=" << input->readULong(1) << ",";
    f << "sortUserIndex=" << input->readULong(2) << ",";
  }
  if (input->tell()<lastPos) {
    f << "doSize=" << input->readULong(1) << ",";
    f << "keepFmt=" << input->readULong(1) << ",";
  }
  if (input->tell()<lastPos)
    f << "stripData=" << input->readULong(1) << ",";
  if (input->tell()<lastPos)
    f << "dbType=" << input->readULong(1) << ",";
  if (input->tell()<lastPos) {
    bool isAdvanced;
    *input >> isAdvanced;
    if (isAdvanced)
      f << "advSource=" << std::hex << input->readULong(4) << "x" << input->readULong(4) << std::dec << ",";
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: find extra data\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectSpreadsheet::readSCDBPivot(StarZone &zone, int version, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCDBPivot)[" << zone.getRecordLevel() << "]:";
  // sc_pivot.cxx ScPivot::Load
  bool hasHeader;
  *input >> hasHeader;
  if (hasHeader) f << "hasHeader,";
  for (int i=0; i<2; ++i) {
    uint16_t col1, row1, col2, row2, table;
    *input >> col1 >> row1 >> col2 >> row2 >> table;
    f << (i==0 ? "src":"dest") << "="
      << col1 << "x" << row1 << "<->" << col2 << "x" << row2 << "[" << table << "],";
  }
  for (int i=0; i<3; ++i) {
    uint16_t nCount;
    *input >> nCount;
    if (input->tell()+6*int(nCount) >lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some fields\n"));
      f << "###fields";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }

    if (!nCount) continue;
    f << (i==0 ? "col" : i==1 ? "row" : "data") << "=[";
    for (int c=0; c<int(nCount); ++c) {
      if (version >= 7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      f << "[";
      f << "col=" << input->readLong(2) << ",";
      f << "mask=" << input->readULong(2) << ",";
      f << "count=" << input->readULong(2) << ",";
      f << "],";
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f << "SCDBPivot:";
  if (!readSCQueryParam(zone, version, lastPos)) {
    f << "###query";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  pos=input->tell();
  bool bIgnoreEmpty, bDectectCat;
  *input >> bIgnoreEmpty >> bDectectCat;
  if (bIgnoreEmpty) f << "ignore[empty],";
  if (bDectectCat) f << "detect[cat],";
  if (input->tell()<lastPos) {
    bool makeTotalCol, makeTotalRow;
    *input >> makeTotalCol >> makeTotalRow;
    if (makeTotalCol) f << "make[totalCol],";
    if (makeTotalRow) f << "make[totalRow],";
  }
  if (input->tell()<lastPos) {
    std::vector<uint32_t> string;
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some text\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!string.empty())
        f << (i==0 ? "name": "tag") << "=" << libstoff::getString(string).cstr() << ",";
    }
    uint16_t count;
    *input >> count;
    for (int i=0; i<int(count); ++i) {
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some text\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!string.empty())
        f << "colName" << i << "=" << libstoff::getString(string).cstr() << ",";
    }
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBPivot: find extra data\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectSpreadsheet::readSCMatrix(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCMatrix)[" << zone.getRecordLevel() << "]:";
  // sc_scmatrix.cxx ScMatrix::ScMatrix
  uint16_t nCol, nRow;
  *input >> nCol >> nRow;
  f << "dim=" << nCol << "x" << nRow << ",";
  int nCell=int(nCol)*int(nRow);
  bool ok=true;
  f << "values=[";
  for (int i=0; i<nCell; ++i) {
    uint8_t type;
    *input>>type;
    if ((i%nCol)==0) f << "[";
    switch (type) {
    case 0:
      f << "_,";
      break;
    case 1: {
      double val;
      *input>>val;
      f << val << ",";
      break;
    }
    case 2: {
      std::vector<uint32_t> string;
      if (!zone.readString(string) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCMatrix: can not read a string\n"));
        f << "###string";
        ok=false;
        break;
      }
      f << libstoff::getString(string).cstr() << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCMatrix: find unexpected type\n"));
      f << "###type=" << int(type) << ",";
      ok=false;
      break;
    }
    if (!ok) break;
    if (input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCMatrix: the zone is too short\n"));
      f << "###short,";
      ok=false;
      break;
    }
    if ((i%nCol)==0) f << "],";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=lastPos;
}
bool StarObjectSpreadsheet::readSCQueryParam(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  if (!zone.openSCRecord()) return false;
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCQueryParam)[" << zone.getRecordLevel() << "]:";
  // sc_global2.cxx ScQueryParam::Load
  uint16_t nCol1, nRow1, nCol2, nRow2, nDestTab, nDestCol, nDestRow;
  bool bHasHeader, bInplace, bCaseSens, bRegExp, bDuplicate, bByRow;
  *input >> nCol1 >> nRow1 >> nCol2 >> nRow2 >> nDestTab >> nDestCol >> nDestRow;
  f << "dim=" << nCol1 << "x" << nRow1 << "<->" << nCol2 << "x" << nRow2 << ",";
  f << "dest=" << nDestCol << "x" << nDestRow << "x" << nDestTab << ",";
  *input >> bHasHeader >> bInplace >> bCaseSens >> bRegExp >> bDuplicate >> bByRow;
  if (bHasHeader) f << "hasHeader,";
  if (bInplace) f << "inPlace,";
  if (bCaseSens) f << "caseSens,";
  if (bRegExp) f << "regExp,";
  if (bDuplicate) f << "duplicate,";
  if (bByRow) f << "byRow,";
  std::vector<uint32_t> string;
  for (int i=0; i<8; ++i) {
    bool doQuery, queryByString;
    uint8_t op, connect;
    uint16_t nField;
    double val;
    *input >> doQuery >> queryByString >> op >> connect >> nField >> val;
    if (!zone.readString(string)||input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCDBData: can not read some text\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCRecord("SCQueryParam");
      return false;
    }
    if (!doQuery) continue;
    f << "query" << i << "=[";
    if (queryByString) f << "byString,";
    f << "op=" << int(op) << ",";
    f << "connect=" << int(connect) << ",";
    f << "field=" << nField << ",";
    f << "val=" << val << ",";
    if (!string.empty()) f << libstoff::getString(string).cstr() << ",";
    f << "],";
  }
  zone.closeSCRecord("SCQueryParam");
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectSpreadsheet::readSCOutlineArray(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  StarObjectSpreadsheetInternal::ScMultiRecord scRecord(zone);
  if (!scRecord.open()) {
    input->seek(pos,librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCOutlineArray)[" << zone.getRecordLevel() << "]:";
  auto depth=int(input->readULong(2));
  f << "depth=" << depth << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int lev=0; lev<depth; ++lev) {
    pos=input->tell();
    f.str("");
    f << "SCOutlineArray-" << lev << ",";
    if (pos>zone.getRecordLastPosition()) {
      f << "###";
      STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCOutlineArray:can not find some content\n"));
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      scRecord.close("SCOutlineArray");
      return true;
    }
    auto count=int(input->readULong(2));
    f << "entries=[";
    for (int i=0; i<count; ++i) {
      if (!scRecord.openContent("SCOutlineArray")) {
        f << "###";
        STOFF_DEBUG_MSG(("StarObjectSpreadsheet::readSCOutlineArray:can not find some content(II)\n"));
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        scRecord.close("SCOutlineArray");
        return true;
      }
      f << "[start=" << input->readULong(2) << ",sz=" << input->readULong(2) << ",";
      bool hidden, visible;
      *input >> hidden >> visible;
      if (hidden) f << "hidden,";
      if (visible) f << "visible,";
      f << "],";
      scRecord.closeContent("SCOutlineArray");
    }
    f << "],";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  scRecord.close("SCOutlineArray");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
