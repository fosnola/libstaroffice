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

/** \file STOFFCell.cxx
 * Implements STOFFCell (cell content and format)
 */
#include <time.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFFont.hxx"
#include "STOFFListener.hxx"

#include "STOFFCell.hxx"

////////////////////////////////////////////////////////////
// STOFFCell::Format
////////////////////////////////////////////////////////////
STOFFCell::Format::~Format()
{
}

std::string STOFFCell::Format::getValueType() const
{
  switch (m_format) {
  case F_NUMBER:
    if (m_numberFormat==F_NUMBER_CURRENCY) return "currency";
    if (m_numberFormat==F_NUMBER_PERCENT) return "percent";
    if (m_numberFormat==F_NUMBER_SCIENTIFIC) return "scientific";
    return "float";
  case F_BOOLEAN:
    return "boolean";
  case F_DATE:
  case F_DATETIME:
    return "date";
  case F_TIME:
    return "time";
  case F_TEXT:
  case F_UNKNOWN:
  default:
    break;
  }
  return "float";
}

bool STOFFCell::Format::convertDTFormat(std::string const &dtFormat, librevenge::RVNGPropertyListVector &propVect)
{
  propVect.clear();
  size_t len=dtFormat.size();
  std::string text("");
  librevenge::RVNGPropertyList list;
  for (size_t c=0; c < len; ++c) {
    if (dtFormat[c]!='%' || c+1==len) {
      text+=dtFormat[c];
      continue;
    }
    char ch=dtFormat[++c];
    if (ch=='%') {
      text += '%';
      continue;
    }
    if (!text.empty()) {
      list.clear();
      list.insert("librevenge:value-type", "text");
      list.insert("librevenge:text", text.c_str());
      propVect.append(list);
      text.clear();
    }
    list.clear();
    switch (ch) {
    case 'Y':
      list.insert("number:style", "long");
      STOFF_FALLTHROUGH;
    case 'y':
      list.insert("librevenge:value-type", "year");
      propVect.append(list);
      break;
    case 'B':
      list.insert("number:style", "long");
      STOFF_FALLTHROUGH;
    case 'b':
    case 'h':
      list.insert("librevenge:value-type", "month");
      list.insert("number:textual", true);
      propVect.append(list);
      break;
    case 'm':
      list.insert("librevenge:value-type", "month");
      propVect.append(list);
      break;
    case 'e':
      list.insert("number:style", "long");
      STOFF_FALLTHROUGH;
    case 'd':
      list.insert("librevenge:value-type", "day");
      propVect.append(list);
      break;
    case 'A':
      list.insert("number:style", "long");
      STOFF_FALLTHROUGH;
    case 'a':
      list.insert("librevenge:value-type", "day-of-week");
      propVect.append(list);
      break;

    case 'H':
      list.insert("number:style", "long");
      STOFF_FALLTHROUGH;
    case 'I':
      list.insert("librevenge:value-type", "hours");
      propVect.append(list);
      break;
    case 'M':
      list.insert("librevenge:value-type", "minutes");
      list.insert("number:style", "long");
      propVect.append(list);
      break;
    case 'S':
      list.insert("librevenge:value-type", "seconds");
      list.insert("number:style", "long");
      propVect.append(list);
      break;
    case 's': // extension for second + milli second...
      list.insert("librevenge:value-type", "seconds");
      list.insert("number:decimal-places", 2);
      list.insert("number:style", "long");
      propVect.append(list);
      break;
    case 'p':
      list.insert("librevenge:value-type", "text");
      list.insert("librevenge:text", " ");
      propVect.append(list);
      list.clear();
      list.insert("librevenge:value-type", "am-pm");
      propVect.append(list);
      break;
    case 'Q': // extension for quarter
      list.insert("librevenge:value-type", "quarter");
      propVect.append(list);
      break;
    default:
      STOFF_DEBUG_MSG(("STOFFCell::Format::convertDTFormat: find unimplement command %c(ignored)\n", ch));
    }
  }
  if (!text.empty()) {
    list.clear();
    list.insert("librevenge:value-type", "text");
    list.insert("librevenge:text", text.c_str());
    propVect.append(list);
  }
  return propVect.count()!=0;
}

std::ostream &operator<<(std::ostream &o, STOFFCell::Format const &format)
{
  switch (format.m_format) {
  case STOFFCell::F_BOOLEAN:
    o << "boolean";
    break;
  case STOFFCell::F_TEXT:
    o << "text";
    break;
  case STOFFCell::F_NUMBER:
    o << "number";
    switch (format.m_numberFormat) {
    case STOFFCell::F_NUMBER_GENERIC:
      break;
    case STOFFCell::F_NUMBER_DECIMAL:
      o << "[decimal]";
      break;
    case STOFFCell::F_NUMBER_SCIENTIFIC:
      o << "[exp]";
      break;
    case STOFFCell::F_NUMBER_PERCENT:
      o << "[percent]";
      break;
    case STOFFCell::F_NUMBER_CURRENCY:
      o << "[money]";
      break;
    case STOFFCell::F_NUMBER_FRACTION:
      o << "[fraction]";
      break;
    case STOFFCell::F_NUMBER_UNKNOWN:
    default:
      STOFF_DEBUG_MSG(("STOFFCell::operator<<(Format): find unexpected type\n"));
      o << "###format,";
      break;
    }
    break;
  case STOFFCell::F_DATE:
    o << "date";
    break;
  case STOFFCell::F_DATETIME:
    o << "date+time";
    break;
  case STOFFCell::F_TIME:
    o << "time";
    break;
  case STOFFCell::F_UNKNOWN:
  default:
    break; // default
  }
  o << ",";

  return o;
}

////////////////////////////////////////////////////////////
// STOFFCell
////////////////////////////////////////////////////////////
void STOFFCell::addTo(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("librevenge:column", position()[0]);
  propList.insert("librevenge:row", position()[1]);

  m_font.addTo(propList);
  m_cellStyle.addTo(propList);
}

std::string STOFFCell::getColumnName(int col)
{
  std::stringstream f;
  f << "[.";
  if (col > 26) f << char('A'+col/26);
  f << char('A'+(col%26));
  f << "]";
  return f.str();
}

std::string STOFFCell::getCellName(STOFFVec2i const &pos, STOFFVec2b const &absolute)
{
  std::stringstream f;
  f << "[." << libstoff::getCellName(pos, absolute) << ']';
  return f.str();
}

std::ostream &operator<<(std::ostream &o, STOFFCell const &cell)
{
  o << STOFFCell::getCellName(cell.m_position, STOFFVec2b(false,false)) << ":";
  if (cell.m_bdBox.size()[0]>0 || cell.m_bdBox.size()[1]>0)
    o << "bdBox=" << cell.m_bdBox << ",";
  if (cell.m_bdSize[0]>0 || cell.m_bdSize[1]>0)
    o << "bdSize=" << cell.m_bdSize << ",";
  o << cell.m_format;
  return o;
}

// send data to listener
bool STOFFCell::send(STOFFListenerPtr listener, STOFFTable &table)
{
  if (!listener) return true;
  listener->openTableCell(*this);
  bool ok=sendContent(listener, table);
  listener->closeTableCell();
  return ok;
}

bool STOFFCell::sendContent(STOFFListenerPtr, STOFFTable &)
{
  STOFF_DEBUG_MSG(("STOFFCell::sendContent: must not be called!!!\n"));
  return false;
}

////////////////////////////////////////////////////////////
// STOFFCellContent
////////////////////////////////////////////////////////////
bool STOFFCellContent::double2Date(double val, int &Y, int &M, int &D)
{
  /* first convert the date in long*/
  auto numDaysSinceOrigin=long(val+(val>0 ? -2 : -3)+0.4);
  // checkme: do we need to check before for isNan(val) ?
  if (numDaysSinceOrigin<-10000*365 || numDaysSinceOrigin>10000*365) {
    /* normally, we can expect documents to contain date between 1900
       and 2100. So even if such a date can make sense, storing it as
       a number of days is clearly abnormal */
    STOFF_DEBUG_MSG(("STOFFCellContent::double2Date: using a double to represent the date %ld seems odd\n", numDaysSinceOrigin));
    Y=1900;
    M=D=1;
    return false;
  }
  // find the century
  int century=19;
  while (numDaysSinceOrigin>=36500+24) {
    long numDaysInCentury=36500+24+((century%4)?0:1);
    if (numDaysSinceOrigin<numDaysInCentury) break;
    numDaysSinceOrigin-=numDaysInCentury;
    ++century;
  }
  while (numDaysSinceOrigin<0) {
    --century;
    numDaysSinceOrigin+=36500+24+((century%4)?0:1);
  }
  // now compute the year
  Y=int(numDaysSinceOrigin/365);
  long numDaysToEndY1=Y*365+(Y>0 ? (Y-1)/4+((century%4)?0:1) : 0);
  if (numDaysToEndY1>numDaysSinceOrigin) {
    --Y;
    numDaysToEndY1=Y*365+(Y>0 ? (Y-1)/4+((century%4)?0:1) : 0);
  }
  // finish to compute the date
  auto numDaysFrom1Jan=int(numDaysSinceOrigin-numDaysToEndY1);
  Y+=century*100;
  bool isLeap=(Y%4)==0 && ((Y%400)==0 || (Y%100)!=0);

  for (M=0; M<12; ++M) {
    static const int days[2][12] = {
      { 0,31,59,90,120,151,181,212,243,273,304,334},
      { 0,31,60,91,121,152,182,213,244,274,305,335}
    };
    if (M<11 && days[isLeap ? 1 : 0][M+1]<=numDaysFrom1Jan) continue;
    D=(numDaysFrom1Jan-days[isLeap ? 1 : 0][M++])+1;
    break;
  }
  return true;
}

bool STOFFCellContent::date2Double(int Y, int M, int D, double &val)
{
  --M;
  --D;
  if (M>11) {
    Y += M/12;
    M %= 12;
  }
  else if (M<0) {
    int yDiff = (-M + 11)/12;
    Y -= yDiff;
    M+=12*yDiff;
  }
  // sanity check
  if (M<0||M>11) {
    STOFF_DEBUG_MSG(("STOFFCellContent::date2Double: something is bad\n"));
    return false;
  }
  bool isLeap=(Y%4)==0 && ((Y%400)==0 || (Y%100)!=0);
  int32_t const daysFrom0=365*Y+(Y/400)-(Y/100)+(Y/4);
  int32_t const daysFrom1900=365*1900+(1900/400)-(1900/100)+(1900/4);
  static const int32_t days[2][12] = {
    { 0,31,59,90,120,151,181,212,243,273,304,334},
    { 0,31,60,91,121,152,182,213,244,274,305,335}
  };
  int32_t daysFrom1Jan=days[isLeap ? 1 : 0][M] + D;
  val=double(daysFrom0-daysFrom1900+daysFrom1Jan);
  return true;
}

bool STOFFCellContent::double2Time(double val, int &H, int &M, int &S)
{
  if (val < 0.0 || val > 1.0) return false;
  double time = 24.*3600.*val+0.5;
  H=int(time/3600.);
  time -= H*3600.;
  M=int(time/60.);
  time -= M*60.;
  S=int(time);
  return true;
}

std::ostream &operator<<(std::ostream &o, STOFFCellContent const &content)
{
  switch (content.m_contentType) {
  case STOFFCellContent::C_NONE:
    break;
  case STOFFCellContent::C_TEXT:
    o << ",text";
    break;
  case STOFFCellContent::C_TEXT_BASIC:
    o << ",text=\"" << libstoff::getString(content.m_text).cstr() << "\"";
    break;
  case STOFFCellContent::C_NUMBER:
    o << ",val="<< content.m_value;
    break;
  case STOFFCellContent::C_FORMULA:
    o << ",formula=";
    for (const auto &l : content.m_formula)
      o << l;
    if (content.isValueSet()) o << "[" << content.m_value << "]";
    break;
  case STOFFCellContent::C_UNKNOWN:
    break;
  default:
    o << "###unknown type,";
    break;
  }
  return o;
}

// ---------- MWAWContentListener::FormulaInstruction ------------------
librevenge::RVNGPropertyList STOFFCellContent::FormulaInstruction::getPropertyList() const
{
  librevenge::RVNGPropertyList pList;
  switch (m_type) {
  case F_None:
    break;
  case F_Operator:
    pList.insert("librevenge:type","librevenge-operator");
    pList.insert("librevenge:operator",m_content);
    break;
  case F_Function:
    pList.insert("librevenge:type","librevenge-function");
    pList.insert("librevenge:function",m_content);
    break;
  case F_Text:
    pList.insert("librevenge:type","librevenge-text");
    pList.insert("librevenge:text",m_content);
    break;
  case F_Double:
    pList.insert("librevenge:type","librevenge-number");
    pList.insert("librevenge:number",m_doubleValue, librevenge::RVNG_GENERIC);
    break;
  case F_Long:
    pList.insert("librevenge:type","librevenge-number");
    pList.insert("librevenge:number",double(m_longValue), librevenge::RVNG_GENERIC);
    break;
  case F_Cell:
    pList.insert("librevenge:type","librevenge-cell");
    pList.insert("librevenge:column",m_position[0][0], librevenge::RVNG_GENERIC);
    pList.insert("librevenge:row",m_position[0][1], librevenge::RVNG_GENERIC);
    pList.insert("librevenge:column-absolute",!m_positionRelative[0][0]);
    pList.insert("librevenge:row-absolute",!m_positionRelative[0][1]);
    if (!m_sheet.empty())
      pList.insert("librevenge:sheet",m_sheet);
    break;
  case F_CellList:
    pList.insert("librevenge:type","librevenge-cells");
    pList.insert("librevenge:start-column",m_position[0][0], librevenge::RVNG_GENERIC);
    pList.insert("librevenge:start-row",m_position[0][1], librevenge::RVNG_GENERIC);
    pList.insert("librevenge:start-column-absolute",!m_positionRelative[0][0]);
    pList.insert("librevenge:start-row-absolute",!m_positionRelative[0][1]);
    pList.insert("librevenge:end-column",m_position[1][0], librevenge::RVNG_GENERIC);
    pList.insert("librevenge:end-row",m_position[1][1], librevenge::RVNG_GENERIC);
    pList.insert("librevenge:end-column-absolute",!m_positionRelative[1][0]);
    pList.insert("librevenge:end-row-absolute",!m_positionRelative[1][1]);
    if (!m_sheet.empty())
      pList.insert("librevenge:sheet-name",m_sheet.cstr());
    break;
  case F_Index: {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("STOFFCellContent::FormulaInstruction::getPropertyList: impossible to send index data\n"));
      first=false;
    }
    break;
  }
  default:
    STOFF_DEBUG_MSG(("STOFFCellContent::FormulaInstruction::getPropertyList: unexpected type\n"));
  }
  return pList;
}

std::ostream &operator<<(std::ostream &o, STOFFCellContent::FormulaInstruction const &inst)
{
  if (inst.m_type==STOFFCellContent::FormulaInstruction::F_Double)
    o << inst.m_doubleValue;
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_Long)
    o << inst.m_longValue;
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_Index)
    o << "[Idx" << inst.m_longValue << "]";
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_Cell) {
    if (!inst.m_sheet.empty()) o << inst.m_sheet.cstr();
    else if (inst.m_sheetId>=0) {
      if (!inst.m_sheetIdRelative) o << "$";
      o << "S" << inst.m_sheetId;
      o << ":";
    }
    o << libstoff::getCellName(inst.m_position[0],inst.m_positionRelative[0]);
  }
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_CellList) {
    if (!inst.m_sheet.empty()) o << inst.m_sheet.cstr() << ":";
    else if (inst.m_sheetId>=0) {
      if (inst.m_sheetIdRelative) o << "$";
      o << "S" << inst.m_sheetId;
      o << ":";
    }
    for (int l=0; l<2; ++l) {
      o << libstoff::getCellName(inst.m_position[l],inst.m_positionRelative[l]);
      if (l==0) o << ":";
    }
  }
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_Text)
    o << "\"" << inst.m_content.cstr() << "\"";
  else if (inst.m_type!=STOFFCellContent::FormulaInstruction::F_None)
    o << inst.m_content.cstr();
  if (!inst.m_extra.empty())
    o << "[" << inst.m_extra << "]";
  return o;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
