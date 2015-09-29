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

bool STOFFCell::Format::getNumberingProperties(librevenge::RVNGPropertyList &propList) const
{
  librevenge::RVNGPropertyListVector pVect;
  switch (m_format) {
  case F_BOOLEAN:
    propList.insert("librevenge:value-type", "boolean");
    break;
  case F_NUMBER:
    if (m_digits>0)
      propList.insert("number:decimal-places", m_digits);
    if (m_thousandHasSeparator)
      propList.insert("number:grouping", true);
    switch (m_numberFormat) {
    case F_NUMBER_GENERIC:
      propList.insert("librevenge:value-type", "number");
      propList.remove("number:decimal-places");
      break;
    case F_NUMBER_SCIENTIFIC:
      propList.insert("librevenge:value-type", "scientific");
      break;
    case F_NUMBER_PERCENT:
      propList.insert("librevenge:value-type", "percentage");
      break;
    case F_NUMBER_DECIMAL:
      propList.insert("librevenge:value-type", "number");
      if (m_integerDigits>=0) {
        propList.insert("number:min-integer-digits", m_integerDigits+1);
        propList.insert("number:decimal-places", 0);
      }
      break;
    case F_NUMBER_FRACTION:
      propList.insert("librevenge:value-type", "fraction");
      propList.insert("number:min-integer-digits", 0);
      propList.insert("number:min-numerator-digits", m_numeratorDigits>0 ? m_numeratorDigits : 1);
      propList.insert("number:min-denominator-digits", m_denominatorDigits>0 ? m_denominatorDigits : 1);
      propList.remove("number:decimal-places");
      break;
    case F_NUMBER_CURRENCY: {
      propList.clear();
      propList.insert("librevenge:value-type", "currency");
      librevenge::RVNGPropertyList list;
      list.insert("librevenge:value-type", "currency-symbol");
      list.insert("number:language","en");
      list.insert("number:country","US");
      list.insert("librevenge:currency",m_currencySymbol.c_str());
      pVect.append(list);

      list.clear();
      list.insert("librevenge:value-type", "number");
      if (m_digits>-1000)
        list.insert("number:decimal-places", m_digits);
      pVect.append(list);
      break;
    }
    case F_NUMBER_UNKNOWN:
    default:
      return false;
    }
    break;
  case F_DATE:
    propList.insert("librevenge:value-type", "date");
    propList.insert("number:automatic-order", "true");
    if (!convertDTFormat(m_DTFormat.empty() ? "%m/%d/%Y" : m_DTFormat, pVect))
      return false;
    break;
  case F_TIME:
    propList.insert("librevenge:value-type", "time");
    propList.insert("number:automatic-order", "true");
    if (!convertDTFormat(m_DTFormat.empty() ? "%H:%M:%S" : m_DTFormat, pVect))
      return false;
    break;
  case F_TEXT:
  case F_UNKNOWN:
  default:
    return false;
  }
  if (pVect.count())
    propList.insert("librevenge:format", pVect);
  return true;
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
    case 'y':
      list.insert("librevenge:value-type", "year");
      propVect.append(list);
      break;
    case 'B':
      list.insert("number:style", "long");
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
    // fall-through intended
    case 'd':
      list.insert("librevenge:value-type", "day");
      propVect.append(list);
      break;
    case 'A':
      list.insert("number:style", "long");
    case 'a':
      list.insert("librevenge:value-type", "day-of-week");
      propVect.append(list);
      break;

    case 'H':
      list.insert("number:style", "long");
    // fall-through intended
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
    case 'p':
      list.insert("librevenge:value-type", "text");
      list.insert("librevenge:text", " ");
      propVect.append(list);
      list.clear();
      list.insert("librevenge:value-type", "am-pm");
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
      o << "[money=" << format.m_currencySymbol << "]";
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
    if (format.m_thousandHasSeparator)
      o << "[thousandSep]";
    if (format.m_parenthesesForNegative)
      o << "[parenthesis<0]";
    break;
  case STOFFCell::F_DATE:
    o << "date[" << format.m_DTFormat << "]";
    break;
  case STOFFCell::F_TIME:
    o << "time[" << format.m_DTFormat << "]";
    break;
  case STOFFCell::F_UNKNOWN:
  default:
    break; // default
  }
  o << ",";

  if (format.m_digits != -1) o << "digits=" << format.m_digits << ",";
  if (format.m_integerDigits != -1) o << "digits[min]=" << format.m_integerDigits << ",";
  if (format.m_numeratorDigits != -1) o << "digits[num]=" << format.m_numeratorDigits << ",";
  if (format.m_denominatorDigits != -1) o << "digits[den]=" << format.m_denominatorDigits << ",";
  return o;
}

int STOFFCell::Format::compare(STOFFCell::Format const &cell) const
{
  if (m_format<cell.m_format) return 1;
  if (m_format>cell.m_format) return -1;
  if (m_numberFormat<cell.m_numberFormat) return 1;
  if (m_numberFormat>cell.m_numberFormat) return -1;
  if (m_digits<cell.m_digits) return 1;
  if (m_digits>cell.m_digits) return -1;
  if (m_integerDigits<cell.m_integerDigits) return 1;
  if (m_integerDigits>cell.m_integerDigits) return -1;
  if (m_numeratorDigits<cell.m_numeratorDigits) return 1;
  if (m_numeratorDigits>cell.m_numeratorDigits) return -1;
  if (m_denominatorDigits<cell.m_denominatorDigits) return 1;
  if (m_denominatorDigits>cell.m_denominatorDigits) return -1;
  if (m_thousandHasSeparator!=cell.m_thousandHasSeparator) return m_thousandHasSeparator ? -1:1;
  if (m_parenthesesForNegative!=cell.m_parenthesesForNegative) return m_parenthesesForNegative ? -1:1;
  if (m_DTFormat<cell.m_DTFormat) return 1;
  if (m_DTFormat>cell.m_DTFormat) return -1;
  if (m_currencySymbol<cell.m_currencySymbol) return 1;
  if (m_currencySymbol>cell.m_currencySymbol) return -1;
  return 0;
}
////////////////////////////////////////////////////////////
// STOFFCell
////////////////////////////////////////////////////////////
void STOFFCell::addTo(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("librevenge:column", position()[0]);
  propList.insert("librevenge:row", position()[1]);

  propList.insert("table:number-columns-spanned", numSpannedCells()[0]);
  propList.insert("table:number-rows-spanned", numSpannedCells()[1]);

  if (m_fontSet)
    m_font.addTo(propList);
  for (size_t c = 0; c < m_bordersList.size(); c++) {
    switch (c) {
    case libstoff::Left:
      m_bordersList[c].addTo(propList, "left");
      break;
    case libstoff::Right:
      m_bordersList[c].addTo(propList, "right");
      break;
    case libstoff::Top:
      m_bordersList[c].addTo(propList, "top");
      break;
    case libstoff::Bottom:
      m_bordersList[c].addTo(propList, "bottom");
      break;
    default:
      STOFF_DEBUG_MSG(("STOFFCell::addTo: can not send %d border\n",int(c)));
      break;
    }
  }
  if (!backgroundColor().isWhite())
    propList.insert("fo:background-color", backgroundColor().str().c_str());
  if (isProtected())
    propList.insert("style:cell-protect","protected");
  // alignment
  switch (hAlignment()) {
  case HALIGN_LEFT:
    propList.insert("fo:text-align", "first");
    propList.insert("style:text-align-source", "fix");
    break;
  case HALIGN_CENTER:
    propList.insert("fo:text-align", "center");
    propList.insert("style:text-align-source", "fix");
    break;
  case HALIGN_RIGHT:
    propList.insert("fo:text-align", "end");
    propList.insert("style:text-align-source", "fix");
    break;
  case HALIGN_DEFAULT:
    break; // default
  case HALIGN_FULL:
  default:
    STOFF_DEBUG_MSG(("STOFFCell::addTo: called with unknown halign=%d\n", hAlignment()));
  }
  // no padding
  propList.insert("fo:padding", 0, librevenge::RVNG_POINT);
  // alignment
  switch (vAlignment()) {
  case VALIGN_TOP:
    propList.insert("style:vertical-align", "top");
    break;
  case VALIGN_CENTER:
    propList.insert("style:vertical-align", "middle");
    break;
  case VALIGN_BOTTOM:
    propList.insert("style:vertical-align", "bottom");
    break;
  case VALIGN_DEFAULT:
    break; // default
  default:
    STOFF_DEBUG_MSG(("STOFFCell::addTo: called with unknown valign=%d\n", vAlignment()));
  }
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
  f << "[.";
  if (absolute[1]) f << "$";
  int col = pos[0];
  if (col > 26) f << char('A'+col/26);
  f << char('A'+(col%26));
  if (absolute[0]) f << "$";
  f << pos[1]+1 << ']';
  return f.str();
}

void STOFFCell::setBorders(int wh, STOFFBorder const &border)
{
  int const allBits = libstoff::LeftBit|libstoff::RightBit|libstoff::TopBit|libstoff::BottomBit|libstoff::HMiddleBit|libstoff::VMiddleBit;
  if (wh & (~allBits)) {
    STOFF_DEBUG_MSG(("STOFFCell::setBorders: unknown borders\n"));
    return;
  }
  size_t numData = 4;
  if (wh & (libstoff::HMiddleBit|libstoff::VMiddleBit))
    numData=6;
  if (m_bordersList.size() < numData) {
    STOFFBorder emptyBorder;
    emptyBorder.m_style = STOFFBorder::None;
    m_bordersList.resize(numData, emptyBorder);
  }
  if (wh & libstoff::LeftBit) m_bordersList[libstoff::Left] = border;
  if (wh & libstoff::RightBit) m_bordersList[libstoff::Right] = border;
  if (wh & libstoff::TopBit) m_bordersList[libstoff::Top] = border;
  if (wh & libstoff::BottomBit) m_bordersList[libstoff::Bottom] = border;
  if (wh & libstoff::HMiddleBit) m_bordersList[libstoff::HMiddle] = border;
  if (wh & libstoff::VMiddleBit) m_bordersList[libstoff::VMiddle] = border;
}

std::ostream &operator<<(std::ostream &o, STOFFCell const &cell)
{
  o << STOFFCell::getCellName(cell.m_position, STOFFVec2b(false,false)) << ":";
  if (cell.numSpannedCells()[0] != 1 || cell.numSpannedCells()[1] != 1)
    o << "span=[" << cell.numSpannedCells()[0] << "," << cell.numSpannedCells()[1] << "],";

  if (cell.m_protected) o << "protected,";
  if (cell.m_bdBox.size()[0]>0 || cell.m_bdBox.size()[1]>0)
    o << "bdBox=" << cell.m_bdBox << ",";
  if (cell.m_bdSize[0]>0 || cell.m_bdSize[1]>0)
    o << "bdSize=" << cell.m_bdSize << ",";
  o << cell.m_format;
  if (cell.m_fontSet) o << "hasFont,";
  switch (cell.m_hAlign) {
  case STOFFCell::HALIGN_LEFT:
    o << "left,";
    break;
  case STOFFCell::HALIGN_CENTER:
    o << "centered,";
    break;
  case STOFFCell::HALIGN_RIGHT:
    o << "right,";
    break;
  case STOFFCell::HALIGN_FULL:
    o << "full,";
    break;
  case STOFFCell::HALIGN_DEFAULT:
  default:
    break; // default
  }
  switch (cell.m_vAlign) {
  case STOFFCell::VALIGN_TOP:
    o << "top,";
    break;
  case STOFFCell::VALIGN_CENTER:
    o << "centered[y],";
    break;
  case STOFFCell::VALIGN_BOTTOM:
    o << "bottom,";
    break;
  case STOFFCell::VALIGN_DEFAULT:
  default:
    break; // default
  }

  if (!cell.m_backgroundColor.isWhite())
    o << "backColor=" << cell.m_backgroundColor << ",";
  for (size_t i = 0; i < cell.m_bordersList.size(); i++) {
    if (cell.m_bordersList[i].m_style == STOFFBorder::None)
      continue;
    o << "bord";
    if (i < 6) {
      static char const *wh[] = { "L", "R", "T", "B", "MiddleH", "MiddleV" };
      o << wh[i];
    }
    else o << "[#wh=" << i << "]";
    o << "=" << cell.m_bordersList[i] << ",";
  }
  switch (cell.m_extraLine) {
  case STOFFCell::E_None:
    break;
  case STOFFCell::E_Line1:
    o << "line[TL->RB],";
    break;
  case STOFFCell::E_Line2:
    o << "line[BL->RT],";
    break;
  case STOFFCell::E_Cross:
    o << "line[cross],";
    break;
  default:
    break;
  }
  if (cell.m_extraLine!=STOFFCell::E_None)
    o << cell.m_extraLineType << ",";
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
  long numDaysSinceOrigin=long(val+0.4);
  // checkme: do we need to check before for isNan(val) ?
  if (numDaysSinceOrigin<-10000*365 || numDaysSinceOrigin>10000*365) {
    /* normally, we can expect documents to contain date between 1904
       and 2004. So even if such a date can make sense, storing it as
       a number of days is clearly abnormal */
    STOFF_DEBUG_MSG(("STOFFCellContent::double2Date: using a double to represent the date %ld seems odd\n", numDaysSinceOrigin));
    Y=1904;
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
  int numDaysFrom1Jan=int(numDaysSinceOrigin-numDaysToEndY1);
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

bool STOFFCellContent::double2String(double val, STOFFCell::Format const &format, std::string &str)
{
  std::stringstream s;
  switch (format.m_format) {
  case STOFFCell::F_BOOLEAN:
    if (val<0 || val >0) s << "true";
    else s << "false";
    break;
  case STOFFCell::F_NUMBER:
    if (format.m_digits>=0 && format.m_numberFormat!=STOFFCell::F_NUMBER_GENERIC)
      s << std::setprecision(format.m_digits);
    switch (format.m_numberFormat) {
    case STOFFCell::F_NUMBER_CURRENCY:
      s << std::fixed << val << "$";
      break;
    case STOFFCell::F_NUMBER_DECIMAL:
      s << val;
      break;
    case STOFFCell::F_NUMBER_SCIENTIFIC:
      s << std::scientific << val;
      break;
    case STOFFCell::F_NUMBER_PERCENT:
      s << std::fixed << 100*val << "%";
      break;
    case STOFFCell::F_NUMBER_FRACTION:
    case STOFFCell::F_NUMBER_GENERIC:
    case STOFFCell::F_NUMBER_UNKNOWN:
    default:
      s << val;
      break;
    }
    break;
  case STOFFCell::F_DATE: {
    int Y, M, D;
    if (!double2Date(val, Y, M, D)) return false;
    struct tm time;
    time.tm_sec=time.tm_min=time.tm_hour=0;
    time.tm_mday=D;
    time.tm_mon=M;
    time.tm_year=Y;
    time.tm_wday=time.tm_yday=time.tm_isdst=-1;
#if HAVE_STRUCT_TM_TM_ZONE
    time.tm_zone=0;
#endif
    char buf[256];
    if (mktime(&time)==-1 ||
        !strftime(buf, 256, format.m_DTFormat.empty() ? "%m/%d/%y" : format.m_DTFormat.c_str(), &time))
      return false;
    s << buf;
    break;
  }
  case STOFFCell::F_TIME: {
    if (val<0 || val>=1)
      val=std::fmod(val,1.);
    int H, M, S;
    if (!double2Time(val, H, M, S)) return false;
    struct tm time;
    time.tm_sec=S;
    time.tm_min=M;
    time.tm_hour=H;
    time.tm_mday=time.tm_mon=1;
    time.tm_year=100;
    time.tm_wday=time.tm_yday=time.tm_isdst=-1;
#if HAVE_STRUCT_TM_TM_ZONE
    time.tm_zone=0;
#endif
    char buf[256];
    if (mktime(&time)==-1 ||
        !strftime(buf, 256, format.m_DTFormat.empty() ? "%H:%M:%S" : format.m_DTFormat.c_str(), &time))
      return false;
    s << buf;
    break;
  }
  case STOFFCell::F_TEXT:
  case STOFFCell::F_UNKNOWN:
  default:
    STOFF_DEBUG_MSG(("STOFFCellContent::double2String: called with bad format\n"));
    return false;
  }
  str=s.str();
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
    for (size_t l=0; l < content.m_formula.size(); ++l)
      o << content.m_formula[l];
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
    if (!inst.m_positionRelative[0][0]) o << "$";
    if (inst.m_position[0][0]<0) o << "C" << inst.m_position[0][0];
    else {
      if (inst.m_position[0][0]>=26) o << (char)(inst.m_position[0][0]/26-1 + 'A');
      o << (char)(inst.m_position[0][0]%26+'A');
    }
    if (!inst.m_positionRelative[0][1]) o << "$";
    if (inst.m_position[0][1]<0) o << "R" << inst.m_position[0][1];
    else o << inst.m_position[0][1];
  }
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_CellList) {
    if (!inst.m_sheet.empty()) o << inst.m_sheet.cstr() << ":";
    else if (inst.m_sheetId>=0) {
      if (inst.m_sheetIdRelative) o << "$";
      o << "S" << inst.m_sheetId;
      o << ":";
    }
    for (int l=0; l<2; ++l) {
      if (!inst.m_positionRelative[l][0]) o << "$";
      if (inst.m_position[l][0]<0) o << "C" << inst.m_position[l][0];
      else {
        if (inst.m_position[l][0]>=26) o << (char)(inst.m_position[l][0]/26-1 + 'A');
        o << (char)(inst.m_position[l][0]%26+'A');
      }
      if (!inst.m_positionRelative[l][1]) o << "$";
      if (inst.m_position[l][1]<0) o << "R" << inst.m_position[l][1];
      else o << inst.m_position[l][1];
      if (l==0) o << ":";
    }
  }
  else if (inst.m_type==STOFFCellContent::FormulaInstruction::F_Text)
    o << "\"" << inst.m_content.cstr() << "\"";
  else
    o << inst.m_content.cstr();
  if (!inst.m_extra.empty())
    o << "[" << inst.m_extra << "]";
  return o;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
