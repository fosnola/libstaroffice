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

#include <cmath>
#include <map>
#include <set>
#include <string>

#include <librevenge/librevenge.h>

#include "StarTable.hxx"

#include "StarFormatManager.hxx"
#include "StarObjectText.hxx"
#include "StarState.hxx"
#include "StarWriterStruct.hxx"
#include "StarZone.hxx"

#include "STOFFCell.hxx"
#include "STOFFCellStyle.hxx"
#include "STOFFDebug.hxx"
#include "STOFFListener.hxx"
#include "STOFFTable.hxx"

#include "SWFieldManager.hxx"

/** brief namespace used to define some table structure */
namespace StarTableInternal
{
struct TableLine;
//! small structure used to store a table box
struct TableBox {
  //! constructor
  TableBox()
    : m_position()
    , m_formatId(0xFFFF)
    , m_numLines(0)
    , m_content()
    , m_lineList()
    , m_format()
    , m_cellStyle()
    , m_xDimension(0,0)
  {
  }
  //! try to read the data
  bool read(Table &table, StarZone &zone, StarObjectText &object, STOFFBox2i &cPos);
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarState &state) const;
  //! update the position to correspond to cover the m_position[0],right/bottom
  void updatePosition(Table &table, StarState const &state, float xOrigin, STOFFVec2i const &RBpos=STOFFVec2i(-1,-1));
  //! the position
  STOFFBox2i m_position;
  //! the format
  int m_formatId;
  //! the number of lines
  int m_numLines;
  //! the content
  std::shared_ptr<StarObjectTextInternal::Content> m_content;
  //! a list of line
  std::vector<std::shared_ptr<TableLine> > m_lineList;
  //! the format
  std::shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  /// the cell style
  STOFFCellStyle m_cellStyle;
  //! the x position
  STOFFVec2f m_xDimension;
};

//! small structure used to store a table line
struct TableLine {
  //! constructor
  TableLine()
    : m_position()
    , m_formatId(0xFFFF)
    , m_numBoxes(0)
    , m_boxList()
    , m_format()
  {
  }
  //! try to read the data
  bool read(Table &table, StarZone &zone, StarObjectText &object, STOFFBox2i &cPos);
  //! update the position to correspond to cover the m_position[0],right/bottom
  void updatePosition(Table &table, StarState const &state, float xOrigin, STOFFVec2i const &RBpos=STOFFVec2i(-1,-1));
  //! the position
  STOFFBox2i m_position;
  //! the format
  int m_formatId;
  //! the number of boxes
  int m_numBoxes;
  //! a list of box
  std::vector<std::shared_ptr<TableBox> > m_boxList;
  //! the format
  std::shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

/** \brief class to store a table data in a sdw file
 */
class Table
{
public:
  //! the constructor
  Table()
    : m_headerRepeated(false)
    , m_numBoxes(0)
    , m_chgMode(0)
    , m_dimension()
    , m_minColWidth(1000000)
    , m_format()
    , m_formatList()
    , m_lineList()
    , m_xPositionSet()
    , m_columnWidthList()
    , m_rowToBoxMap()
  {
  }
  //! try use the xdimension to compute the final col positions
  void updateColumnsPosition();
  //! try to read the data
  bool read(StarZone &zone, StarObjectText &object);
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarState &state);

  //! flag to know if the header is repeated
  bool m_headerRepeated;
  //! the number of boxes
  int m_numBoxes;
  //! the change mode
  int m_chgMode;
  //! the dimension
  STOFFVec2i m_dimension;
  //! the minimal col width
  float m_minColWidth;
  //! the table format
  std::shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  //! map format id to format def
  std::vector<std::shared_ptr<StarFormatManagerInternal::FormatDef> > m_formatList;
  //! the list of line
  std::vector<std::shared_ptr<StarTableInternal::TableLine> > m_lineList;
  //! the list of x position
  std::set<float> m_xPositionSet;
  //! the column width
  std::vector<float> m_columnWidthList;
  //! the list of row to box
  std::map<int, std::vector<StarTableInternal::TableBox *> > m_rowToBoxMap;
};

void TableBox::updatePosition(Table &table, StarState const &state, float xOrigin, STOFFVec2i const &RBpos)
{
  for (int i=0; i<2; ++i) {
    if (RBpos[i]>=0 && m_position[1][i]<RBpos[i])
      m_position.max()[i]=RBpos[i];
  }
  StarState cState(state);
  if (m_formatId!=0xFFFF && !m_format) {
    if (m_formatId>=0 && m_formatId<int(table.m_formatList.size()))
      m_format=table.m_formatList[size_t(m_formatId)];
    else {
      STOFF_DEBUG_MSG(("StarTableInternal::TableBox::updatePosition: oops, can not find format %d\n", m_formatId));
    }
  }
  if (m_format) {
    cState.m_frame.m_position.m_size=STOFFVec2i(0,0);
    m_format->updateState(cState);
    if (cState.m_frame.m_position.m_size[0]<=0) {
      if (m_lineList.empty()) {
        static bool first=true;
        if (first) {
          STOFF_DEBUG_MSG(("StarTableInternal::TableBox::updatePosition: oops, can not find some box witdh\n"));
          first=false;
        }
        table.m_minColWidth=0;
      }
    }
    else {
      m_xDimension=STOFFVec2f(xOrigin, xOrigin+cState.m_frame.m_position.m_size[0]);
      table.m_xPositionSet.insert(xOrigin+cState.m_frame.m_position.m_size[0]);
      if (cState.m_frame.m_position.m_size[0] < table.m_minColWidth)
        table.m_minColWidth=cState.m_frame.m_position.m_size[0];
    }
  }
  else if (m_lineList.empty()) {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarTableInternal::TableBox::updatePosition: oops, can not find some box witdh\n"));
      first=false;
    }
    table.m_minColWidth=0;
  }
  if (m_lineList.empty()) {
    int row=m_position.min()[1];
    if (table.m_rowToBoxMap.find(row)==table.m_rowToBoxMap.end())
      table.m_rowToBoxMap[row]=std::vector<StarTableInternal::TableBox *>();
    table.m_rowToBoxMap.find(row)->second.push_back(this);
    m_cellStyle=cState.m_cell;
    cState.m_frame.addTo(m_cellStyle.m_propertyList);
    return;
  }
  for (size_t i=0; i< m_lineList.size(); ++i) {
    if (!m_lineList[i]) continue;
    int nextRow=(RBpos[1]>=0 && i+1==m_lineList.size()) ? RBpos[1] : m_lineList[i]->m_position.max()[1];
    m_lineList[i]->updatePosition(table, cState, xOrigin, STOFFVec2i(RBpos[0], nextRow));
  }
}

bool TableBox::read(Table &table, StarZone &zone, StarObjectText &object, STOFFBox2i &cPos)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='t' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTableBox
  libstoff::DebugStream f;
  f << "Entries(StarTable)[box-" << zone.getRecordLevel() << "]:" << cPos[0] << ",";
  int fl=zone.openFlagZone();
  librevenge::RVNGString formatName;
  if ((fl&0x20) || !zone.isCompatibleWith(0x201)) {
    m_formatId=int(input->readULong(2));
    if ((fl&0x20)==0) {
      if (!zone.getPoolName(m_formatId, formatName))
        f << "###format=" << m_formatId << ",";
      else
        f << formatName.cstr() << ",";
      m_formatId=0xFFFF; // CHANGEME
    }
    else if (m_formatId!=0xFFFF)
      f << "format[id]=" << m_formatId << ",";
  }
  if (fl&0x10) {
    m_numLines=int(input->readULong(2));
    if (m_numLines)
      f << "numLines=" << m_numLines << ",";
  }
  bool saveFormat=false;
  if (fl&0x40) {
    f << "save[format],";
    saveFormat=true;
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->peek()=='f') {
    object.getFormatManager()->readSWFormatDef(zone,'f', m_format, object);
    if (saveFormat)
      table.m_formatList.push_back(m_format);
    else if (!formatName.empty())
      object.getFormatManager()->storeSWFormatDef(formatName, m_format);
  }
  else if (!formatName.empty())
    m_format=object.getFormatManager()->getSWFormatDef(formatName);
  if (input->peek()=='N')
    object.readSWContent(zone, m_content);
  long lastPos=zone.getRecordLastPosition();
  STOFFBox2i boxCPos(cPos);
  STOFFVec2i maxPos=cPos[0]+STOFFVec2i(1,1);
  while (input->tell()<lastPos) {
    pos=input->tell();
    std::shared_ptr<TableLine> line(new TableLine);
    boxCPos.min()[0]=cPos.min()[0];
    boxCPos.max()[0]=cPos.max()[0];
    if (line->read(table, zone, object, boxCPos) && input->tell()<=lastPos) {
      for (int i=0; i<2; ++i) {
        if (boxCPos[0][i]>maxPos[i])
          maxPos[i]=boxCPos[0][i];
      }
      m_lineList.push_back(line);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  m_position=STOFFBox2i(cPos.min(), maxPos);
  for (int i=0; i<2; ++i) {
    if (maxPos[i]>cPos[1][i]) {
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarTableInternal::TableBox::read: the dim %d number seems bad: %d>%d\n", i, maxPos[i], cPos[1][i]));
      }
      m_position.max()[i]=cPos.max()[i];
    }
    else
      cPos.min()[i]=maxPos[i];
  }
  zone.closeSWRecord('t', "StarTable");
  return true;
}

bool TableBox::send(STOFFListenerPtr listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarTableInternal::TableBox::send: call without listener\n"));
    return false;
  }
  STOFFCellStyle cellStyle(m_cellStyle);
  cellStyle.m_numberCellSpanned=m_position.size();
  STOFFCell cell;
  cell.setPosition(m_position.min());
  cell.setCellStyle(cellStyle);

  state.m_global->m_object.getFormatManager()->updateNumberingProperties(cell);
  listener->openTableCell(cell);
  StarState cState(state);
  if (m_content)
    m_content->send(listener, state);
  listener->closeTableCell();
  return true;
}

void TableLine::updatePosition(Table &table, StarState const &state, float xOrigin, STOFFVec2i const &RBpos)
{
  for (int i=0; i<2; ++i) {
    if (RBpos[i]>=0 && m_position[1][i]<RBpos[i])
      m_position.max()[i]=RBpos[i];
  }
  StarState cState(state);
  if (m_formatId!=0xFFFF && !m_format) {
    if (m_formatId>=0 && m_formatId<int(table.m_formatList.size()))
      m_format=table.m_formatList[size_t(m_formatId)];
    else {
      STOFF_DEBUG_MSG(("StarTableInternal::TableBox::updatePosition: oops, can not find format %d\n", m_formatId));
    }
  }
  if (m_format)
    m_format->updateState(cState);
  for (size_t i=0; i< m_boxList.size(); ++i) {
    if (!m_boxList[i]) continue;
    int nextCol=(RBpos[0]>=0 && i+1==m_boxList.size()) ? RBpos[0] : m_boxList[i]->m_position.max()[0];
    m_boxList[i]->updatePosition(table, cState, xOrigin, STOFFVec2i(nextCol, RBpos[1]));
    xOrigin=m_boxList[i]->m_xDimension[1];
  }
}

bool TableLine::read(Table &table, StarZone &zone, StarObjectText &object, STOFFBox2i &cPos)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='L' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTableLine
  libstoff::DebugStream f;
  f << "Entries(StarTable)[line-" << zone.getRecordLevel() << "]:R" << cPos[0][1] << ",";
  int fl=zone.openFlagZone();
  librevenge::RVNGString formatName;
  if ((fl&0x20) || !zone.isCompatibleWith(0x201)) {
    m_formatId=int(input->readULong(2));
    if ((fl&0x20)==0) {
      if (!zone.getPoolName(m_formatId, formatName))
        f << "###format=" << m_formatId << ",";
      else
        f << formatName.cstr() << ",";
      m_formatId=0xFFFF; // CHANGEME
    }
    else if (m_formatId!=0xFFFF)
      f << "format[id]=" << m_formatId << ",";
  }
  bool saveFormat=false;
  if (fl&0x40) {
    saveFormat=true;
    f << "save[format],";
  }
  m_numBoxes=int(input->readULong(2));
  if (m_numBoxes)
    f << "nBoxes=" << m_numBoxes << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (input->peek()=='f') {
    object.getFormatManager()->readSWFormatDef(zone,'f',m_format, object);
    if (saveFormat)
      table.m_formatList.push_back(m_format);
    else if (!formatName.empty())
      object.getFormatManager()->storeSWFormatDef(formatName, m_format);
  }
  else if (!formatName.empty())
    m_format=object.getFormatManager()->getSWFormatDef(formatName);
  long lastPos=zone.getRecordLastPosition();
  STOFFVec2i maxPos=cPos[0]+STOFFVec2i(1,1);
  STOFFBox2i boxCPos(cPos);
  while (input->tell()<lastPos) {
    pos=input->tell();
    std::shared_ptr<TableBox> box(new TableBox);
    boxCPos.min()[1]=cPos.min()[1];
    boxCPos.max()[1]=cPos.max()[1];
    if (box->read(table, zone, object, boxCPos) && input->tell()<=lastPos) {
      for (int i=0; i<2; ++i) {
        if (boxCPos[0][i]>maxPos[i])
          maxPos[i]=boxCPos[0][i];
      }
      m_boxList.push_back(box);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  m_position=STOFFBox2i(cPos.min(), maxPos);
  for (int i=0; i<2; ++i) {
    if (maxPos[i]>cPos[1][i]) {
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarTableInternal::TableLine::read: the dim %d number seems bad: %d>%d\n", i, maxPos[i], cPos[1][i]));
      }
      m_position.max()[i]=cPos.max()[i];
    }
    else
      cPos.min()[i]=maxPos[i];
  }
  zone.closeSWRecord('L', "StarTable");
  return true;
}

void Table::updateColumnsPosition()
{
  if (m_minColWidth<=0 || m_xPositionSet.empty()) return;
  std::set<float> xPositionSet, xRoundedPositionSet;
  float minColWidth = m_minColWidth>10 ? 10 : m_minColWidth;
  float f=2.f/minColWidth;
  for (auto const &pos : m_xPositionSet) {
    // todo replace float(int(f*(*it)+0.5f)) by std::round(f*(*it)) when possible
    float xRounded=0.5f*float(int(f*pos+0.5f))*minColWidth;
    if (xRoundedPositionSet.find(xRounded)!=xRoundedPositionSet.end())
      continue;
    xPositionSet.insert(pos);
    xRoundedPositionSet.insert(xRounded);
  }
  int col=0;
  std::map<float,int> xPosToColMap;
  for (auto const &pos : xRoundedPositionSet)
    xPosToColMap[pos]=col++;
  float prevPos=0;
  for (auto const &pos : xPositionSet) {
    if (pos<=prevPos) continue;
    m_columnWidthList.push_back(pos-prevPos);
    prevPos=pos;
  }
  auto maxCol=int(xPosToColMap.size()-1);
  for (auto &it : m_rowToBoxMap) {
    auto &boxes=it.second;
    int actCol=0;
    for (auto *box : boxes) {
      if (!box)
        continue;
      for (int c=0; c<2; ++c) {
        float x=box->m_xDimension[c];
        auto mIt=xPosToColMap.lower_bound(x);
        int newCol=0;
        if (mIt==xPosToColMap.end())
          newCol=maxCol;
        else
          newCol=(x<mIt->first-0.5f*minColWidth) ? mIt->second-1 : mIt->second;
        if (newCol<0) newCol=0;
        if (newCol<actCol) {
          STOFF_DEBUG_MSG(("Table::updateColumnsPosition: can not compute a col find %d<%d\n", newCol, actCol));
          break;
        }
        if (c==0) {
          if (newCol!=box->m_position.min()[0])
            box->m_position.min()[0]=newCol;
        }
        else {
          if (newCol!=box->m_position.max()[0])
            box->m_position.max()[0]=newCol;
        }
        actCol=newCol;
      }
    }
  }
}

bool Table::read(StarZone &zone, StarObjectText &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='E' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTable
  libstoff::DebugStream f;
  f << "Entries(StarTable)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20) {
    m_headerRepeated=true;
    f << "headerRepeat,";
  }
  m_numBoxes=int(input->readULong(2));
  if (m_numBoxes)
    f << "nBoxes=" << m_numBoxes << ",";
  if (zone.isCompatibleWith(0x5,0x201))
    f << "nTblIdDummy=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x103)) {
    m_chgMode=int(input->readULong(1));
    if (m_chgMode)
      f << "cChgMode=" << m_chgMode << ",";
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  if (input->peek()=='f') object.getFormatManager()->readSWFormatDef(zone, 'f', m_format, object);
  if (input->peek()=='Y') {
    // TODO storeme
    SWFieldManager fieldManager;
    fieldManager.readField(zone,'Y');
  }
  while (input->tell()<lastPos && input->peek()=='v') {
    pos=input->tell();
    // TODO storeme
    StarWriterStruct::NodeRedline redline;
    if (redline.read(zone))
      continue;
    STOFF_DEBUG_MSG(("StarTableInternal::Table::read: can not read a red line\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWTableDef:###redline");
    zone.closeSWRecord('E',"StarTable");
    return true;
  }

  STOFFBox2i cPos(STOFFVec2i(0,0),STOFFVec2i(200,200));
  int maxCol=0;
  while (input->tell()<lastPos && input->peek()=='L') {
    pos=input->tell();
    std::shared_ptr<StarTableInternal::TableLine> line(new StarTableInternal::TableLine);
    cPos.min()[0]=0;
    cPos.max()[0]=200;
    if (line->read(*this, zone, object, cPos)) {
      if (cPos.min()[0]>maxCol) maxCol=cPos.min()[0];
      m_lineList.push_back(line);
      continue;
    }
    STOFF_DEBUG_MSG(("StarTableInternal::Table::read: can not read a table line\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  m_dimension=STOFFVec2i(maxCol,cPos.min()[1]);
  zone.closeSWRecord('E',"StarTable");
  return true;
}

bool StarTableInternal::Table::send(STOFFListenerPtr listener, StarState &state)
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarTableInternal::Table::send: call without listener\n"));
    return false;
  }
  float percentMaxValue=0;
  StarState cState(state.m_global);
  STOFFTable table;
  if (m_format) {
    m_format->updateState(cState);
    table.m_propertyList=cState.m_cell.m_propertyList;
    cState.m_frame.addTo(table.m_propertyList);

    m_minColWidth=cState.m_frame.m_position.m_size[0];
    // checkme sometime the width is 65535/20, ie. bigger than the page width...
    if (cState.m_frame.m_position.m_size[0]>=float(int(65535/20))) {
      percentMaxValue=cState.m_frame.m_position.m_size[0];
      table.m_propertyList.insert("style:width", 1., librevenge::RVNG_PERCENT);
    }
    else if (cState.m_frame.m_position.m_size[0]>0)
      table.m_propertyList.insert("style:width", double(cState.m_frame.m_position.m_size[0]), librevenge::RVNG_POINT);
  }
  m_xPositionSet.insert(0);
  for (auto line : m_lineList) {
    if (!line) continue;
    line->updatePosition(*this, cState, 0, STOFFVec2i(m_dimension[0],line->m_position.max()[1]));
  }
  updateColumnsPosition();
  // first find the number of columns
  librevenge::RVNGPropertyListVector columns;
  if (!m_columnWidthList.empty()) {
    if (percentMaxValue>0) {
      float factor=1/percentMaxValue;
      for (auto const &c : m_columnWidthList) {
        librevenge::RVNGPropertyList column;
        column.insert("style:column-width", factor*c, librevenge::RVNG_PERCENT);
        columns.append(column);
      }
    }
    else {
      for (auto const &c : m_columnWidthList) {
        librevenge::RVNGPropertyList column;
        column.insert("style:column-width", c, librevenge::RVNG_POINT);
        columns.append(column);
      }
    }
  }
  else {
    for (int c = 0; c < m_dimension[0]; ++c) {
      librevenge::RVNGPropertyList column;
      column.insert("style:column-width", 40, librevenge::RVNG_POINT);
      columns.append(column);
    }
  }
  table.m_propertyList.insert("librevenge:table-columns", columns);
  listener->openTable(table);
  int row=-1;
  for (auto it : m_rowToBoxMap) {
    while (++row<it.first) {
      listener->openTableRow(0, librevenge::RVNG_POINT);
      listener->closeTableRow();
    }
    listener->openTableRow(0, librevenge::RVNG_POINT);
    std::vector<StarTableInternal::TableBox *> const &line=it.second;
    int col=-1;
    for (auto const *box : line) {
      if (!box) continue;
      while (++col<box->m_position.min()[0])
        listener->addCoveredTableCell(STOFFVec2i(col,row));
      box->send(listener, state);
    }
    listener->closeTableRow();
  }
  listener->closeTable();
  return true;
}
}

////////////////////////////////////////////////////////////
// StarTable
////////////////////////////////////////////////////////////
StarTable::StarTable() : m_table(new StarTableInternal::Table)
{
}

StarTable::~StarTable()
{
}

bool StarTable::read(StarZone &zone, StarObjectText &object)
{
  return m_table->read(zone, object);
}

bool StarTable::send(STOFFListenerPtr listener, StarState &state) const
{
  return m_table->send(listener, state);
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
