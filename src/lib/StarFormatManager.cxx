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
#include "STOFFCellStyle.hxx"

#include "StarAttribute.hxx"
#include "StarFileManager.hxx"
#include "StarGraphicStruct.hxx"
#include "StarLanguage.hxx"
#include "StarObject.hxx"
#include "StarObjectModel.hxx"
#include "StarState.hxx"
#include "StarWriterStruct.hxx"
#include "StarZone.hxx"

#include "StarFormatManager.hxx"

/** Internal: the structures of a StarFormatManager */
namespace StarFormatManagerInternal
{
//------------------------------------------------------------
// implements formatDef function
//------------------------------------------------------------
FormatDef::~FormatDef()
{
}

void FormatDef::updateState(StarState &state) const
{
  for (auto const &attr : m_attributeList) {
    if (!attr.m_attribute)
      continue;
    attr.m_attribute->addTo(state);
  }
}

bool FormatDef::send(STOFFListenerPtr listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarFormatManagerInternal::FormatDef::send: call without listener\n"));
    return false;
  }
  std::shared_ptr<StarAttribute> attrib;
  for (auto const &attr : m_attributeList) {
    if (!attr.m_attribute)
      continue;
    attr.m_attribute->addTo(state);
    if (!state.m_content) continue;
    if (!attrib)
      attrib=attr.m_attribute;
    else {
      STOFF_DEBUG_MSG(("StarFormatManagerInternal::FormatDef::send: oops find already a content attribute\n"));
    }
    state.m_content=false;
  }
  if (attrib)
    attrib->send(listener, state);
  else if (state.m_global->m_objectModel && m_values[2]>=0)
    state.m_global->m_objectModel->sendObject(m_values[2], listener, state);
  else {
    STOFF_DEBUG_MSG(("StarFormatManagerInternal::FormatDef::send: can not find any data to send %d\n", m_values[2]));
  }
  return true;
}

void FormatDef::printData(libstoff::DebugStream &o) const
{
  for (int i=0; i<2; ++i) {
    if (!m_names[i].empty())
      o << (i==0 ? "poolName" : "readName") << "=" << m_names[i].cstr() << ",";
  }
  if (m_values[0]) o << "derived=" << m_values[0] << ",";
  if (m_values[1]!=0xFFFF) o << "poolId=" << m_values[1] << ",";
  if (m_values[2]>=0) o << "objRef=" << m_values[2] << ",";
}

//------------------------------------------------------------
//! a struct use to store number formatter of a StarFormatManager
struct NumberFormatter {
  //! struct use to store small format item
  struct FormatItem {
    //! constructor
    FormatItem()
      : m_text("")
      , m_type(0)
    {
    }
    //! try to update the cell's formating
    bool updateNumberingProperties(librevenge::RVNGPropertyListVector &vect) const;

    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, FormatItem const &item)
    {
      if (!item.m_text.empty()) o << item.m_text.cstr();
      o << ":";
      if (item.m_type<0 && item.m_type>=-21) {
        char const *wh[]= {"", "string", "del", "blank", "star", "digit",
                           "decimSep", "thSep", "exp", "frac", "empty",
                           "fracBlank", "currency", "currDel", "currext",
                           "calendar", "caldel", "dateSep", "timeSep",
                           "time100secSep", "percent", "frac_fdiv"
                          };
        o << wh[-item.m_type];
      }
      else if (item.m_type>0 && item.m_type<50) {
        char const *wh[]= {
          "", "E", "AMPM", "AP", "MI", "MMI", "M", "MM", // exp, 2a/p, 2min, 2month
          "MMM", "MMMM", "H", "HH", "S", "SS", "Q", "QQ", // 2month, 2hours, 2sec, 2quarter
          "D", "DD", "DDD", "DDDD", "YY", "YYYY", "NN", "NNNN", //4 day, 2 year, 2 day of week
          "CCC", "GENERAL", "NNN", "WW", "MMMMM", "UNUSED4", "QUARTER", "TRUE", // currency, blank, day of week, week, month[first letter], ...,  quarter, true
          "FALSE", "BOOLEAN", "COLOR", "BLACK", "BLUE", "GREEN", "YELLOW", "WHITE", // false, bool, + colors
          "AAA", "AAAA", "EC", "EEC", "G", "GG", "GGG", "R", // 2 jap DDD, 2 gregorian year, 3 gentoo era, EE
          "RR", "THAI_T"
        };
        o << wh[item.m_type];
      }
      else if (item.m_type) o << ":" << item.m_type;
      return o;
    }
    //! the string
    librevenge::RVNGString m_text;
    //! the type
    int m_type;
  };
  //! struct use to store different local format
  struct Format {
    //! constructor
    Format()
      : m_itemList()
      , m_type(0)
      , m_hasThousandSep(false)
      , m_prefix(0)
      , m_postfix(0)
      , m_exponential(0)
      , m_thousand(0)
      , m_colorName("")
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Format const &form)
    {
      if (!form.m_type) return o;
      o << getTypeString(form.m_type) << ",";
      if (form.m_prefix) o << "prefix[digits]=" << form.m_prefix << ",";
      if (form.m_postfix) o << "postfix[digits]=" << form.m_postfix << ",";
      if (form.m_exponential) o << "exponential[digits]=" << form.m_exponential << ",";
      if (form.m_hasThousandSep) o << "thousand[digits]=" << form.m_thousand << ",";
      if (!form.m_itemList.empty()) {
        o << "item=[";
        for (const auto &i : form.m_itemList)
          o << i << ",";
        o << "],";
      }
      if (!form.m_colorName.empty()) o << "color[name]=" << form.m_colorName.cstr() << ",";
      return o;
    }
    //! try to update the cell's formating
    bool updateNumberingProperties(STOFFCell &cell, std::string const &language, std::string const &country) const;
    //! the item list
    std::vector<FormatItem> m_itemList;
    //! the format type
    int m_type;
    //! a flag to know if we need to add a thousand separator
    bool m_hasThousandSep;
    //! the prefix digits
    int m_prefix;
    //! the postfix digits
    int m_postfix;
    //! the exponential digits
    int m_exponential;
    //! the number of thousand digits
    int m_thousand;
    //! the color name
    librevenge::RVNGString m_colorName;
  };
  //! constructor
  NumberFormatter()
    : m_format("")
    , m_language(0)
    , m_type(0)
    , m_isStandart(false)
    , m_isUsed(false)
    , m_extra("")
  {
    for (int i=0; i<2; ++i) {
      m_limits[i]=0;
      m_limitsOp[i]=0;
    }
  }
  //! returns a string corresponding to a format type
  static std::string getTypeString(int typeId)
  {
    std::stringstream s;
    if (typeId&1) s << "user,";
    if (typeId&2) s << "date";
    if (typeId&4) s << "time";
    if (typeId&8) s << "currency";
    if (typeId&0x10) s << "number";
    if (typeId&0x20) s << "scientific";
    if (typeId&0x40) s << "fraction";
    if (typeId&0x80) s << "percent";
    if (typeId&0x100) s << "text";
    if (typeId&0x400) s << "logical";
    if (typeId&0x800) s << "unused";
    if (typeId&0xF200) s << "#form=" << std::hex << (typeId&0xFa00) << std::dec;
    return s.str();
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, NumberFormatter const &form)
  {
    if (!form.m_format.empty()) o << form.m_format.cstr() << ",";
    if (form.m_language) {
      std::string lang, country;
      if (StarLanguage::getLanguageId(form.m_language, lang, country))
        o << lang << "_" << country << ",";
      else
        o << "#langId=" << form.m_language << ",";
    }
    o << getTypeString(form.m_type) << ",";
    if (form.m_isStandart) o << "standart,";
    if (!form.m_isUsed) o << "unused,";
    for (int i=0; i<2; ++i) {
      if (!form.m_limitsOp[i]) continue;
      if (form.m_limitsOp[i]>0 && form.m_limitsOp[i]<=6) {
        char const *wh[]= {"none", "=", "<>", "<", "<=", ">", ">="};
        o << "lim" << i << "=[" << wh[form.m_limitsOp[i]] << form.m_limits[i] << "],";
      }
      else {
        STOFF_DEBUG_MSG(("StarFormatManagerInternal::NumberFormatter::operator<< unknown limit op\n"));
        o << "lim" << i << "=###" << form.m_limitsOp[i] << ":" << form.m_limits[i] << ",";
      }
    }
    for (int i=0; i<4; ++i) {
      if ((form.m_subFormats[i].m_type&0xF7FF)==0) continue;
      o << "format" << i << "=[" << form.m_subFormats[i] << "],";
    }
    o << form.m_extra;
    return o;
  }
  //! try to update the cell's formating
  bool updateNumberingProperties(STOFFCell &cell) const
  {
    std::string lang(""), country("");
    if (m_language) StarLanguage::getLanguageId(m_language, lang, country);
    return m_subFormats[0].updateNumberingProperties(cell, lang, country);
  }
  //! the format
  librevenge::RVNGString m_format;
  //! the format language
  int m_language;
  //! the format type
  uint16_t m_type;
  //! a flag to know if the format is standart
  bool m_isStandart;
  //! a flag to know if this format is used
  bool m_isUsed;
  //! the limits
  double m_limits[2];
  //! the limits operator
  int m_limitsOp[2];
  //! the list of sub format
  Format m_subFormats[4];
  //! extra data
  std::string m_extra;
};

bool NumberFormatter::FormatItem::updateNumberingProperties(librevenge::RVNGPropertyListVector &vect) const
{
  librevenge::RVNGPropertyList list;
  switch (m_type) {
  case -3: {
    if (m_text.empty()) break;
    auto fChar=int(m_text.cstr()[0]);
    if (fChar>=32) {
      static int cCharWidths[ 128-32 ] = {
        1,1,1,2,2,3,2,1,1,1,1,2,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,1,1,2,2,2,2,
        3,2,2,2,2,2,2,3,2,1,2,2,2,3,3,3,
        2,3,2,2,2,2,2,3,2,2,2,1,1,1,2,2,
        1,2,2,2,2,2,1,2,2,1,1,2,1,3,2,2,
        2,2,1,2,1,2,2,2,2,2,2,1,1,1,2,1
      };
      int n=fChar<128 ? cCharWidths[fChar-32] : 2;
      std::string s("");
      for (int i=0; i<n; ++i) s+=' ';
      list.insert("librevenge:value-type", "text");
      list.insert("librevenge:text", s.c_str());
    }
    break;
  }
  case -4: { // star
    librevenge::RVNGString s("*");
    s.append(m_text);
    list.insert("librevenge:value-type", "text");
    list.insert("librevenge:text", s);
    break;
  }
  case -5: // digits todo
  case -15: // calendar
    break;
  case -1: // string
  case -12: // currency
  case -17: // dateSep
  case -18: // timeSep
  case -19: // time100Sep
    if (m_text.empty()) break;
    list.insert("librevenge:value-type", "text");
    list.insert("librevenge:text", m_text);
    break;
  case 2: // ampm
  case 3: // ap
    list.insert("librevenge:value-type", "am-pm");
    break;
  case 4: // mi
  case 5: // mmi
    if (m_type==5)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "minutes");
    break;
  case 6: // m
  case 7: // mm
    if (m_type==7)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "month");
    break;
  case 8: // mmm
  case 9: // mmmm
  case 28: // mmmmm fixme only one letter
    if (m_type==9)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "month");
    list.insert("number:textual", true);
    break;
  case 10: // h
  case 11: // hh
    if (m_type==11)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "hours");
    break;
  case 12: // s
  case 13: // ss
    if (m_type==13)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "seconds");
    break;
  case 14: // q
  case 15: // qq
    if (m_type==15)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "quarter");
    break;
  case 16: // d
  case 17: // dd
    if (m_type==17)
      list.insert("number:style", "long");
    list.insert("librevenge:value-type", "day");
    break;
  case 18: // ddd
  case 26: // nnn
  case 41: // aaa
    list.insert("number:style", "long");
    STOFF_FALLTHROUGH;
  case 19: // dddd
  case 22: // nn
  case 40: // aa
    list.insert("librevenge:value-type", "day-of-week");
    break;
  case 23: // nnnn
    list.insert("number:style", "long");
    list.insert("librevenge:value-type", "day-of-week");
    vect.append(list);
    list.clear();
    list.insert("librevenge:value-type", "text");
    list.insert("librevenge:text", " ");
    break;
  case 20: // yy
  case 42: // ec
    list.insert("librevenge:value-type", "year");
    break;
  case 21: // yyyy
  case 43: // eec
  case 47: // r
    list.insert("number:style", "long");
    list.insert("librevenge:value-type", "year");
    break;
  case 27: // ww
    list.insert("librevenge:value-type", "week-of-year");
    break;
  case 44: // g
  case 45: // gg
  case 46: // ggg
    if (m_type==46) list.insert("number:style", "long");
    list.insert("librevenge:value-type", "era");
    break;
  case 48: // rr
    list.insert("number:style", "long");
    list.insert("librevenge:value-type", "year");
    vect.append(list);
    list.clear();
    list.insert("librevenge:value-type", "text");
    list.insert("librevenge:text", " ");
    vect.append(list);
    list.clear();
    list.insert("number:style", "long");
    list.insert("librevenge:value-type", "era");
    break;
  default:
    STOFF_DEBUG_MSG(("StarFormatManagerInternal::NumberFormatter::Format::updateNumberingProperties: find unexpected type %d\n", m_type));
    return false;
  }
  if (!list.empty())
    vect.append(list);
  return true;
}

bool NumberFormatter::Format::updateNumberingProperties(STOFFCell &cell, std::string const &language, std::string const &country) const
{
  if (m_type==0 || m_type==0x800) return false;
  auto &propList=cell.getNumberingStyle();
  librevenge::RVNGPropertyListVector pVect;
  STOFFCell::Format format=cell.getFormat();
  bool done=true;
  if (m_type&6) {
    format.m_format=(m_type&6)==6 ? STOFFCell::F_DATETIME : (m_type&6)==2 ? STOFFCell::F_DATE : STOFFCell::F_TIME;
    for (auto const &item : m_itemList) {
      if (!item.updateNumberingProperties(pVect))
        return false;
    }
    propList.insert("librevenge:value-type", (m_type&6)==4 ? "time" : "date");
    propList.insert("librevenge:format", pVect);
  }
  else if (m_type&8) {
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_CURRENCY;
    librevenge::RVNGString currency("");
    // look for currency
    int nString=0;
    for (auto const &item : m_itemList) {
      if (item.m_type!=-1) continue;
      currency=item.m_text;
      ++nString;
    }
    if (nString!=1 || currency.empty()) currency="$";
    propList.insert("librevenge:value-type", "currency");
    librevenge::RVNGPropertyList list;
    list.insert("librevenge:value-type", "currency-symbol");
    list.insert("number:language",language.empty() ? "en" : language.c_str());
    list.insert("number:country",country.empty() ? "US" : country.c_str());
    list.insert("librevenge:currency",currency);
    pVect.append(list);
    list.clear();
    list.insert("librevenge:value-type", "number");
    list.insert("number:decimal-places", m_postfix);
    if (m_prefix) list.insert("number:min-integer-digits", m_prefix);
    if (m_hasThousandSep) list.insert("number:grouping", true);
    pVect.append(list);
    propList.insert("librevenge:format", pVect);
  }
  else if (m_type&0x10) {
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_DECIMAL;
    propList.insert("librevenge:value-type", "number");
    propList.insert("number:decimal-places", m_postfix);
    int prefix=m_prefix;
    if (m_hasThousandSep) {
      // if set, prefix seems to contains leading+3 characters
      propList.insert("number:grouping", true);
      prefix-=3;
    }
    if (prefix>0) propList.insert("number:min-integer-digits", prefix);
  }
  else if (m_type&0x20) {
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_SCIENTIFIC;
    propList.insert("librevenge:value-type", "scientific");
    propList.insert("number:decimal-places", m_postfix);
    if (m_prefix) propList.insert("number:min-integer-digits", m_prefix);
    if (m_exponential) propList.insert("number:min-exponent-digits", m_exponential);
    if (m_hasThousandSep) propList.insert("number:grouping", true);
  }
  else if (m_type&0x40) {
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_FRACTION;
    propList.insert("librevenge:value-type", "fraction");
    propList.insert("number:min-numerator-digits", m_postfix ? m_postfix : 1);
    propList.insert("number:min-denominator-digits", m_exponential ? m_exponential : 1);
    if (m_prefix) propList.insert("number:min-integer-digits", m_prefix);
    if (m_hasThousandSep) propList.insert("number:grouping", true);
  }
  else if (m_type&0x80) {
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_PERCENT;
    propList.insert("librevenge:value-type", "percent");
    propList.insert("number:decimal-places", m_postfix);
    if (m_prefix) propList.insert("number:min-integer-digits", m_prefix);
    if (m_hasThousandSep) propList.insert("number:grouping", true);
  }
  else if (m_type&0x100) {
    format.m_format=STOFFCell::F_TEXT;
    done = false;
  }
  else if (m_type&0x400) {
    format.m_format=STOFFCell::F_BOOLEAN;
    propList.insert("librevenge:value-type", "boolean");
  }
  else
    return false;
  cell.setFormat(format);
  return done;
}

////////////////////////////////////////
//! Internal: the state of a StarFormatManager
struct State {
  //! constructor
  State()
    : m_idNumberFormatMap()
    , m_nameToFormatDefMap()
  {
  }
  //! a map id to number format
  std::map<unsigned, NumberFormatter> m_idNumberFormatMap;
  //! a map name to format definition
  std::map<librevenge::RVNGString, std::shared_ptr<StarFormatManagerInternal::FormatDef> > m_nameToFormatDefMap;
};
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarFormatManager::StarFormatManager()
  : m_state(new StarFormatManagerInternal::State)
{
}

StarFormatManager::~StarFormatManager()
{
}

void StarFormatManager::storeSWFormatDef(librevenge::RVNGString const &name, std::shared_ptr<StarFormatManagerInternal::FormatDef> &format)
{
  if (m_state->m_nameToFormatDefMap.find(name)!=m_state->m_nameToFormatDefMap.end()) {
    STOFF_DEBUG_MSG(("StarFormatManager::getSWFormatDef: a format with name %s is already defined\n", name.cstr()));
  }
  else
    m_state->m_nameToFormatDefMap[name]=format;
}

std::shared_ptr<StarFormatManagerInternal::FormatDef> StarFormatManager::getSWFormatDef(librevenge::RVNGString const &name) const
{
  if (m_state->m_nameToFormatDefMap.find(name)!=m_state->m_nameToFormatDefMap.end())
    return m_state->m_nameToFormatDefMap.find(name)->second;
  STOFF_DEBUG_MSG(("StarFormatManager::getSWFormatDef: can not find any format corresponding to %s\n", name.cstr()));
  return std::shared_ptr<StarFormatManagerInternal::FormatDef>();
}
bool StarFormatManager::readSWFormatDef(StarZone &zone, unsigned char kind, std::shared_ptr<StarFormatManagerInternal::FormatDef> &format, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!=kind || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();

  // sw_sw3fmts.cxx InFormat
  libstoff::DebugStream f;
  f << "Entries(SWFormatDef)[" << kind << "-" << zone.getRecordLevel() << "]:";
  format.reset(new StarFormatManagerInternal::FormatDef);
  if (input->tell()==lastPos) { // unsure, but look like it can be empty
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    zone.closeSWRecord(kind, "SWFormatDef");
    return true;
  }
  int flags=zone.openFlagZone();
  for (int i=0; i<2; ++i) format->m_values[i]=int(input->readULong(2));
  int stringId=0xFFFF;
  if (flags&0x10) {
    stringId=int(input->readULong(2));
    if (stringId!=0xffff)
      f << "nStringId=" << stringId << ",";
  }
  if (flags&0x20)
    format->m_values[2]=int(input->readULong(4));
  int moreFlags=0;
  if (flags&(zone.isCompatibleWith(0x201) ? 0x80:0x40)) {
    moreFlags=int(input->readULong(1));
    f << "flags[more]=" << std::hex << moreFlags << std::dec << ",";
  }
  zone.closeFlagZone();

  bool readName;
  if (zone.isCompatibleWith(0x201))
    readName=(moreFlags&0x20);
  else
    readName=(stringId==0xffff);
  std::vector<uint32_t> string;
  if (readName) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarFormatManager::readSWFormatDef: can not read the name\n"));
      f << "###name";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSWRecord(kind, "SWFormatDef");
      return true;
    }
    else if (!string.empty()) {
      format->m_names[1]=libstoff::getString(string);
    }
  }
  else if (stringId!=0xffff) {
    if (!zone.getPoolName(stringId, format->m_names[0]))
      f << "###nPoolId=" << stringId << ",";
  }
  format->printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos) {
    pos=long(input->tell());
    int rType=input->peek();
    if (rType=='S' && StarWriterStruct::Attribute::readList(zone, format->m_attributeList, doc))
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type))
      break;
    f.str("");
    f << "SWFormatDef[" << type << "-" << zone.getRecordLevel() << "]:";
    STOFF_DEBUG_MSG(("StarFormatManager::readSwFormatDef: find unexpected type\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWFormatDef");
  }

  zone.closeSWRecord(kind, "SWFormatDef");
  return true;
}

bool StarFormatManager::readSWNumberFormatterList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='q' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  if (input->tell()==zone.getRecordLastPosition()) // empty
    f << "Entries(NumberFormatter)[" << zone.getRecordLevel() << "]:";
  else {
    f << "NumberFormatter[container-" << zone.getRecordLevel() << "]:";
    readNumberFormatter(zone);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSWRecord(type, "NumberFormatter[container]");
  return true;
}

bool StarFormatManager::readNumberFormat(StarZone &zone, long lastPos, StarObject &doc)
{
  // svx_numitem.cxx SvxNumberFormat::SvxNumberFormat
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  libstoff::DebugStream f;
  f << "Entries(StarNumberFormat)[" << zone.getRecordLevel() << "]:";
  if (pos+26>lastPos) {
    STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormat: the zone seems too short\n"));
    f << "###size";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  uint16_t nVers, numType, eNumAdjust, nInclUpperLevels, nStart, cBullet;
  *input >> nVers >> numType >> eNumAdjust >> nInclUpperLevels >> nStart >> cBullet;
  f << "vers=" << nVers << ",";
  if (numType) f << "numberingType=" << numType << ",";
  if (eNumAdjust) f << "eNumAdjust=" << eNumAdjust << ",";
  if (nInclUpperLevels) f << "nInclUpperLevels=" << nInclUpperLevels << ",";
  if (nStart) f << "nStart=" << nStart << ",";
  if (cBullet) f << "cBullet=" << cBullet << ",";

  int16_t firstLineOffs, absLSpace, lSpace, nCharTextDist;
  *input >> firstLineOffs >> absLSpace >> lSpace >> nCharTextDist;
  if (firstLineOffs) f << "firstLineOffs=" << firstLineOffs << ",";
  if (absLSpace) f << "absLSpace=" << absLSpace << ",";
  if (lSpace) f << "lSpace=" << lSpace << ",";
  if (nCharTextDist) f << "nCharTextDist=" << nCharTextDist << ",";

  for (int i=0; i<3; ++i) {
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormat: can not read the format string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (!text.empty())
      f << (i==0 ? "prefix" : i==1 ? "suffix" : "style[name]") << "=" << libstoff::getString(text).cstr() << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "StarNumberFormat:";
  uint16_t tmp;
  *input >> tmp;
  if (tmp) {
    StarGraphicStruct::StarBrush brush;
    if (!brush.read(zone, 1, lastPos, doc)) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormat: can not read a brush\n"));
      f << brush << "###brush";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    f << brush;
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "StarNumberFormat:";
  }
  uint16_t eVertOrient;
  *input >> eVertOrient >> tmp;
  if (eVertOrient) f << "vertOrient=" << eVertOrient << ",";
  if (tmp) {
    StarFileManager fileManager;
    if (!fileManager.readFont(zone) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormat: can not read a font\n"));
      f << "###font";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "StarNumberFormat:";
  }
  f << "size=" << input->readLong(4) << "x" << input->readLong(4) << ",";
  STOFFColor col;
  if (!input->readColor(col)) {
    STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormat: can not read a color\n"));
    f << "###color";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  *input >> tmp;
  if (tmp) f << "bullet[resSize]=" << tmp << ",";
  *input >> tmp;
  if (tmp) f << "show[symbol],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  return true;
}

bool StarFormatManager::readNumberFormatter(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  libstoff::DebugStream f;
  f << "Entries(NumberFormatter)[" << zone.getRecordLevel() << "]:";
  long dataSz=int(input->readULong(4));
  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+dataSz+6>lastPos) {
    STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: data size seems bad\n"));
    f << "###dataSz";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  // svt_numhead::ImpSvNumMultipleReadHeader
  long actPos=input->tell();
  long endDataPos=input->tell()+dataSz;
  f.str("");
  f << "NumberFormatter-T:";
  input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
  auto val=int(input->readULong(2));
  if (val) f << "nId=" << val << ",";
  auto nSizeTableLen=long(input->readULong(4));
  f << "nTableSize=" << nSizeTableLen << ","; // copy in pMemStream
  long lastZonePos=input->tell()+nSizeTableLen;
  std::vector<long> fieldSize;
  if (lastZonePos>lastPos) {
    STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: the table length seems bad\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }

  f << "listSize=[";
  for (long i=0; i<nSizeTableLen/4; ++i) {
    fieldSize.push_back(long(input->readULong(4)));
    f << std::hex << fieldSize.back() << std::dec << ",";
  }
  f << "],";
  ascFile.addPos(endDataPos);
  ascFile.addNote(f.str().c_str());

  // svt_zforList::Load
  input->seek(actPos, librevenge::RVNG_SEEK_SET);
  f.str("");
  f << "NumberFormatter:";
  auto nVers=int(input->readULong(2));
  f << "vers=" << nVers << ",";
  val=int(input->readULong(2));
  if (val) f << "nSysOnStore=" << std::hex << val << std::dec << ",";
  val=int(input->readULong(2));
  if (val) f << "eLge=" << val << ",";
  ascFile.addPos(actPos);
  ascFile.addNote(f.str().c_str());

  auto id=static_cast<unsigned long>(input->readULong(4));
  size_t n=0;
  while (id!=0xffffffff) {
    pos=input->tell();
    if (pos==endDataPos) break;
    if (pos>endDataPos) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: data size seems bad\n"));
      break;
    }

    f.str("");
    StarFormatManagerInternal::NumberFormatter form;
    input->seek(2, librevenge::RVNG_SEEK_CUR);
    form.m_language=int(input->readULong(2));

    if (n>=fieldSize.size()||input->tell()+fieldSize[n]>endDataPos) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: can not find end of field\n"));
      form.m_extra="###unknownN";
      f.str("");
      f << "NumberFormatter-A" << id << ":" << form;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endFieldPos=input->tell()+fieldSize[n++];

    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: can not read the format string\n"));
      form.m_extra="###format";
      f.str("");
      f << "NumberFormatter-A" << id << ":" << form;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    form.m_format=libstoff::getString(text).cstr();
    *input>>form.m_type;
    for (int i=0; i<2; ++i) {
      bool isNan;
      if (!input->readDoubleReverted8(form.m_limits[i], isNan)) {
        STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: can not read a double\n"));
        f << "##limit" << i << ",";
      }
    }
    for (int &i : form.m_limitsOp)
      i=int(input->readULong(2));
    *input >> form.m_isStandart;
    *input >> form.m_isUsed;

    bool ok=true;
    for (auto &subFormat : form.m_subFormats) {
      // ImpSvNumFor::Load
      StarFormatManagerInternal::NumberFormatter::Format subForm;
      auto N=int(input->readULong(2));
      if (input->tell()+4*N>endFieldPos) break;
      for (int c=0; c<N; ++c) {
        StarFormatManagerInternal::NumberFormatter::FormatItem item;
        if (!zone.readString(text) || input->tell()>endFieldPos) {
          STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: can not read the format string\n"));
          f << "###SvNumFor" << c << "[" << subForm << "]";
          ok=false;
          break;
        }
        item.m_text=libstoff::getString(text).cstr();
        item.m_type=int(input->readLong(2));
        subForm.m_itemList.push_back(item);
      }
      if (!ok) break;
      subForm.m_type=int(input->readULong(2));
      *input>>subForm.m_hasThousandSep;
      subForm.m_thousand=int(input->readULong(2));
      subForm.m_prefix=int(input->readULong(2));
      subForm.m_postfix=int(input->readULong(2));
      subForm.m_exponential=int(input->readULong(2));
      if (!zone.readString(text) || input->tell()>endFieldPos) {
        STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: can not read the color name\n"));
        f << "###colorname";
        ok=false;
      }
      else
        subForm.m_colorName=libstoff::getString(text).cstr();

      if (!ok) {
        f << "###[" << subForm << "],";
        break;
      }
      subFormat=subForm;
    }
    form.m_extra=f.str();
    if (ok && m_state->m_idNumberFormatMap.find(unsigned(id))==m_state->m_idNumberFormatMap.end())
      m_state->m_idNumberFormatMap[unsigned(id)]=form;
    else if (ok) {
      // FIXME: can happen in StarChartDocument which can have multible number formatter zones
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: format %d already exist...\n", int(id)));
        first=false;
      }
    }

    f.str("");
    f << "NumberFormatter-A" << id << ":" << form;
    if (ok && input->tell()!=endFieldPos && !zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarFormatManager::readNumberFormatter: can not read the comment\n"));
      f << "###comment";
      ok=false;
    }
    else {
      if (!text.empty())
        f << "comment=" << libstoff::getString(text).cstr() << ",";
    }

    if (ok && input->tell()!=endFieldPos) {
      val=int(input->readULong(2));
      if (val) f << "nNewStandardDefined=" << val << ",";
    }

    if (input->tell()!=endFieldPos) {
      // now there can still be a list of currency version....
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarFormatManager::readSWNumberFormat: find extra data\n"));
        first=false;
      }
      ascFile.addDelimiter(input->tell(),'|');
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (endFieldPos+4>endDataPos) {
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
      break;
    }
    input->seek(endFieldPos, librevenge::RVNG_SEEK_SET);
    id=static_cast<unsigned long>(input->readULong(4));
  }

  if (input->tell()+4>=endDataPos)
    input->seek(lastZonePos, librevenge::RVNG_SEEK_SET);
  return true;
}

bool StarFormatManager::readSWFlyFrameList(StarZone &zone, StarObject &doc, std::vector<std::shared_ptr<StarFormatManagerInternal::FormatDef> > &listFormats)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='F' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWFlyFrames)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  // sw_sw3fmts.cxx sub part of Sw3IoImp::InFlyFrames
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    int rType=input->peek();
    std::shared_ptr<StarFormatManagerInternal::FormatDef> format;
    if ((rType=='o' || rType=='l') && readSWFormatDef(zone, static_cast<unsigned char>(rType), format, doc)) {
      if (format) listFormats.push_back(format);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSWRecord('F', "SWFlyFrames");
  return true;
}

bool StarFormatManager::readSWPatternLCL(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='P' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWPatternLCL)[" << zone.getRecordLevel() << "]:";
  // sw_sw3misc.cxx sub part of Sw3IoImp::InTOXs
  long lastPos=zone.getRecordLastPosition();
  zone.openFlagZone();
  f << "nLevel=" << input->readULong(1) << ",";
  f << "nTokens=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  std::vector<uint32_t> string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='D' || !zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWPatternLCL[token-" << zone.getRecordLevel() << "]:";
    zone.openFlagZone();
    auto nType=int(input->readULong(2));
    f << "nType=" << nType << ",";
    f << "nStrIdx=" << input->readULong(2) << ",";
    zone.closeFlagZone();
    switch (nType) {
    case 0:
      f << "tknEntryNo,";
      break;
    case 1:
      f << "tknEntryText,";
      break;
    case 3:
      f << "tknEntryTabStop,";
      f << "nTabStopPos=" << input->readLong(4) << ",";
      f << "nTabAlign=" << input->readULong(2) << ",";
      if (zone.isCompatibleWith(0x217))
        f << "fillChar=" << input->readULong(1) << ",";
      break;
    case 2:
      if (input->tell()==zone.getRecordLastPosition()) {
        f << "tknEntry,";
        break;
      }
      // #92986# some older versions created a wrong content index entry definition
      STOFF_FALLTHROUGH;
    case 4: {
      f << "tknText,";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarFormatManager::readSWPatternLCL: can not read a string\n"));
        f << "###type,";
        break;
      }
      f << libstoff::getString(string).cstr() << ",";
      break;
    }
    case 5:
      f << "tknPageNums,";
      break;
    case 6:
      f << "tknChapterInfo,";
      f << "format=" << input->readULong(1);
      break;
    case 7:
      f << "tknLinkStart,";
      break;
    case 8:
      f << "tknLinkEnd,";
      break;
    case 9:
      f << "tknAutority,";
      f << "field=" << input->readULong(2) << ",";
      break;
    case 10: // end of token
      break;
    default:
      STOFF_DEBUG_MSG(("StarFormatManager::readSWPatternLCL: find unknown token\n"));
      f << "###type,";
      break;
    }
    zone.closeSWRecord('D', "SWPatternLCL");
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  zone.closeSWRecord('P', "SWPatternLCL");
  return true;
}

void StarFormatManager::updateNumberingProperties(STOFFCell &cell) const
{
  auto const &style=cell.getCellStyle();
  auto &propList=cell.getNumberingStyle();
  auto format=cell.getFormat();
  librevenge::RVNGPropertyListVector pVect;
  if (style.m_format &&
      m_state->m_idNumberFormatMap.find(unsigned(style.m_format))!=m_state->m_idNumberFormatMap.end() &&
      m_state->m_idNumberFormatMap.find(unsigned(style.m_format))->second.updateNumberingProperties(cell))
    return;
  // CHECKME: style.m_format/1000 is the language format,
  int formatId=(style.m_format%1000);
  switch (formatId) {
  case 0: // standart
    break;
  case 1: // decimal
  case 2:
  case 3:
  case 4:
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_DECIMAL;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "number");
    propList.insert("number:decimal-places", (style.m_format&1) ? 0 : 2);
    if (formatId>=3) propList.insert("number:grouping", true);
    return;
  case 10:
  case 11:
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_PERCENT;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "percentage");
    propList.insert("number:decimal-places", (style.m_format&1) ? 2 : 0);
    return;
  case 20:
  case 21:
  case 22: // red negative
  case 23: { // red negative
    // fixme implement red negative + non us currency
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_CURRENCY;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "currency");
    librevenge::RVNGPropertyList list;
    list.insert("librevenge:value-type", "currency-symbol");
    list.insert("number:language","en");
    list.insert("number:country","US");
    list.insert("librevenge:currency","$");
    pVect.append(list);
    list.clear();
    list.insert("librevenge:value-type", "number");
    list.insert("number:decimal-places", (style.m_format&1) ? 2 : 0);
    list.insert("number:grouping", true);
    pVect.append(list);
    propList.insert("librevenge:format", pVect);
    return;
  }
  case 30:
  case 31:
  case 32:
  case 33:
  case 34:
  case 35:
  case 36: {
    format.m_format=STOFFCell::F_DATE;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "date");
    propList.insert("number:automatic-order", "true");
    char const *wh[]= {"%m/%d/%y", "%a %d/%b %y", "%m/%y", "%b %d", "%B",
                       "%Q Quarter %y", "%m/%d/%Y"
                      };
    if (format.convertDTFormat(wh[formatId-30], pVect))
      propList.insert("librevenge:format", pVect);
    return;
  }
  case 40:
  case 41:
  case 42:
  case 43:
  case 44:
  case 45: {
    format.m_format=STOFFCell::F_TIME;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "time");
    propList.insert("number:automatic-order", "true");
    char const *wh[]= {"%H:%M", "%H:%M:%S", "%I:%M %p","%I:%M:%S %p", "%H:%M:%S", "%M:%s" };
    if (format.convertDTFormat(wh[formatId-40], pVect))
      propList.insert("librevenge:format", pVect);
    return;
  }
  case 50:
    format.m_format=STOFFCell::F_DATETIME;
    cell.setFormat(format);
    if (format.convertDTFormat("%m/%d/%Y %H:%M", pVect))
      propList.insert("librevenge:format", pVect);
    return;
  case 60:
  case 61:
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_SCIENTIFIC;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "scientific");
    propList.insert("number:decimal-places", 2);
    propList.insert("number:min-exponent-digits", formatId==60 ? 3 : 2);
    return;
  case 70:
  case 71:
    format.m_format=STOFFCell::F_NUMBER;
    format.m_numberFormat=STOFFCell::F_NUMBER_FRACTION;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "fraction");
    propList.insert("number:min-numerator-digits", formatId==70 ? 1 : 2);
    propList.insert("number:min-denominator-digits", formatId==70 ? 1 : 2);
    return;
  case 99:
    format.m_format=STOFFCell::F_BOOLEAN;
    cell.setFormat(format);
    propList.insert("librevenge:value-type", "boolean");
    return;
  case 100: // text
  default:
    break;
  }

  switch (format.m_format) {
  case STOFFCell::F_BOOLEAN:
    propList.insert("librevenge:value-type", "boolean");
    break;
  case STOFFCell::F_NUMBER:
    propList.insert("librevenge:value-type", "number");
    format.m_numberFormat=STOFFCell::F_NUMBER_GENERIC;
    cell.setFormat(format);
    break;
  case STOFFCell::F_DATE:
    propList.insert("librevenge:value-type", "date");
    propList.insert("number:automatic-order", "true");
    if (!format.convertDTFormat("%m/%d/%Y", pVect))
      return;
    break;
  case STOFFCell::F_DATETIME:
    propList.insert("librevenge:value-type", "date");
    propList.insert("number:automatic-order", "true");
    if (!format.convertDTFormat("%m/%d/%Y %H:%M", pVect))
      return;
    break;
  case STOFFCell::F_TIME:
    propList.insert("librevenge:value-type", "time");
    propList.insert("number:automatic-order", "true");
    if (!format.convertDTFormat("%H:%M:%S", pVect))
      return;
    break;
  case STOFFCell::F_TEXT:
  case STOFFCell::F_UNKNOWN:
  default:
    return;
  }
  if (pVect.count())
    propList.insert("librevenge:format", pVect);
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
