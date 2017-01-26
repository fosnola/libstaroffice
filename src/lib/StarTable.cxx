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

#include <map>
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
  TableBox() : m_formatId(0), m_numLines(0), m_content(), m_lineList(), m_format()
  {
  }
  //! try to read the data
  bool read(StarZone &zone, StarObjectText &object);
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarState &state) const;
  //! the format
  int m_formatId;
  //! the number of lines
  int m_numLines;
  //! the content
  shared_ptr<StarObjectTextInternal::Content> m_content;
  //! a list of line
  std::vector<shared_ptr<TableLine> > m_lineList;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

//! small structure used to store a table line
struct TableLine {
  //! constructor
  TableLine() : m_formatId(0), m_numBoxes(0), m_boxList(), m_format()
  {
  }
  //! try to read the data
  bool read(StarZone &zone, StarObjectText &object);
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarState &state, int row) const;
  //! the format
  int m_formatId;
  //! the number of boxes
  int m_numBoxes;
  //! a list of box
  std::vector<shared_ptr<TableBox> > m_boxList;
  //! the format
  shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

bool TableBox::read(StarZone &zone, StarObjectText &object)
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
  f << "Entries(StarTable)[box-" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20 || !zone.isCompatibleWith(0x201)) {
    m_formatId=int(input->readULong(2));
    librevenge::RVNGString format;
    if (!zone.getPoolName(m_formatId, format))
      f << "###format=" << m_formatId << ",";
    else
      f << format.cstr() << ",";
  }
  if (fl&0x10) {
    m_numLines=int(input->readULong(2));
    if (m_numLines)
      f << "numLines=" << m_numLines << ",";
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->peek()=='f')
    object.getFormatManager()->readSWFormatDef(zone,'f', m_format, object);
  if (input->peek()=='N')
    object.readSWContent(zone, m_content);
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    shared_ptr<TableLine> line(new TableLine);
    if (line->read(zone, object) && input->tell()<=lastPos) {
      m_lineList.push_back(line);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('t', "StarTable");
  return true;
}

bool TableBox::send(STOFFListenerPtr listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TableBox::send: call without listener\n"));
    return false;
  }
  if (!m_lineList.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TableBox::send: sending box with line list is not implemented\n"));
  }
  if (m_content)
    m_content->send(listener, state);
  return true;
}

bool TableLine::read(StarZone &zone, StarObjectText &object)
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
  libstoff::DebugStream f;
  f << "Entries(StarTable)[line-" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20 || !zone.isCompatibleWith(0x201)) {
    m_formatId=int(input->readULong(2));
    librevenge::RVNGString format;
    if (!zone.getPoolName(m_formatId, format))
      f << "###format=" << m_formatId << ",";
    else
      f << format.cstr() << ",";
  }
  m_numBoxes=int(input->readULong(2));
  if (m_numBoxes)
    f << "nBoxes=" << m_numBoxes << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (input->peek()=='f')
    object.getFormatManager()->readSWFormatDef(zone,'f',m_format, object);

  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    shared_ptr<TableBox> box(new TableBox);
    if (box->read(zone, object) && input->tell()<=lastPos) {
      m_boxList.push_back(box);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('L', "StarTable");
  return true;
}

bool TableLine::send(STOFFListenerPtr listener, StarState &state, int row) const
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
    state.m_global->m_object.getFormatManager()->updateNumberingProperties(cell);
    listener->openTableCell(cell);
    StarState cState(state);
    m_boxList[i]->send(listener, state);
    listener->closeTableCell();
  }
  listener->closeTableRow();
  return true;
}

}

////////////////////////////////////////////////////////////
// StarTable
////////////////////////////////////////////////////////////
StarTable::~StarTable()
{
}


bool StarTable::read(StarZone &zone, StarObjectText &object)
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
  // TODO storeme
  shared_ptr<StarFormatManagerInternal::FormatDef> format;
  if (input->peek()=='f') object.getFormatManager()->readSWFormatDef(zone, 'f', format, object);
  if (input->peek()=='Y') {
    SWFieldManager fieldManager;
    fieldManager.readField(zone,'Y');
  }
  while (input->tell()<lastPos && input->peek()=='v') {
    pos=input->tell();
    // TODO storeme
    StarWriterStruct::NodeRedline redline;
    if (redline.read(zone))
      continue;
    STOFF_DEBUG_MSG(("StarTable::read: can not read a red line\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWTableDef:###redline");
    zone.closeSWRecord('E',"StarTable");
    return true;
  }

  while (input->tell()<lastPos && input->peek()=='L') {
    pos=input->tell();
    shared_ptr<StarTableInternal::TableLine> line(new StarTableInternal::TableLine);
    if (line->read(zone, object)) {
      m_lineList.push_back(line);
      continue;
    }
    STOFF_DEBUG_MSG(("StarTable::read: can not read a table line\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeSWRecord('E',"StarTable");
  return true;
}

bool StarTable::send(STOFFListenerPtr listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarTableInternal::Table::send: call without listener\n"));
    return false;
  }
  STOFFTable table;
  // first find the number of columns
  size_t numCol=0;
  for (size_t i=0; i<m_lineList.size(); ++i) {
    if (m_lineList[i] && m_lineList[i]->m_boxList.size()>numCol)
      numCol=m_lineList[i]->m_boxList.size();
  }
  librevenge::RVNGPropertyListVector columns;
  for (size_t c = 0; c < numCol; ++c) {
    librevenge::RVNGPropertyList column;
    float w=40;
    column.insert("style:column-width", double(w), librevenge::RVNG_POINT);
    columns.append(column);
  }
  table.setColsSize(columns);
  listener->openTable(table);
  for (size_t i=0; i<m_lineList.size(); ++i) {
    if (!m_lineList[i]) continue;
    StarState cState(state);
    m_lineList[i]->send(listener, cState, int(i));
  }
  listener->closeTable();
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
