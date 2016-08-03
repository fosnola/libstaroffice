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
#include <map>
#include <sstream>

#include <librevenge/librevenge.h>

#include "StarObjectNumericRuler.hxx"

#include "STOFFListener.hxx"
#include "STOFFPageSpan.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarState.hxx"
#include "StarWriterStruct.hxx"
#include "StarZone.hxx"
#include "SWFieldManager.hxx"

/** Internal: the structures of a StarObjectNumericRuler */
namespace StarObjectNumericRulerInternal
{
////////////////////////////////////////
//! Internal: the state of a StarObjectNumericRuler
struct State {
  //! constructor
  State() : m_nameToListMap(), m_simplifyNameToListMap()
  {
  }
  //! a map list name to list
  std::map<librevenge::RVNGString, shared_ptr<STOFFList> > m_nameToListMap;
  //! a map list simpl name to list
  std::map<librevenge::RVNGString, shared_ptr<STOFFList> > m_simplifyNameToListMap;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectNumericRuler::StarObjectNumericRuler(StarObject const &orig, bool duplicateState) : StarObject(orig, duplicateState), m_numericRulerState(new StarObjectNumericRulerInternal::State)
{
}

StarObjectNumericRuler::~StarObjectNumericRuler()
{
}

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
shared_ptr<STOFFList> StarObjectNumericRuler::getList(librevenge::RVNGString const &name) const
{
  if (name.empty())
    return shared_ptr<STOFFList>();
  if (m_numericRulerState->m_nameToListMap.find(name)!=m_numericRulerState->m_nameToListMap.end())
    return m_numericRulerState->m_nameToListMap.find(name)->second;
  librevenge::RVNGString simpName=libstoff::simplifyString(name);
  if (m_numericRulerState->m_simplifyNameToListMap.find(simpName)!=m_numericRulerState->m_simplifyNameToListMap.end())
    return m_numericRulerState->m_simplifyNameToListMap.find(simpName)->second;
  STOFF_DEBUG_MSG(("StarObjectNumericRuler::getList: can not find list with name %s\n", name.cstr()));
  return shared_ptr<STOFFList>();
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
bool StarObjectNumericRuler::read(StarZone &zone)
try
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  if (!zone.readSWHeader()) {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: can not read the header\n"));
    return false;
  }
  zone.readStringsPool();
  // sw_sw3num.cxx::Sw3IoImp::InNumRules
  while (!input->isEnd()) {
    long pos=input->tell();
    bool done=false;
    switch (input->peek()) {
    case '0':
    case 'R': {
      shared_ptr<STOFFList> list;
      done=StarObjectNumericRuler::readList(zone,list);
      if (!done || !list) break;
      if (list->m_name.empty()) {
        STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: can not find the list name\n"));
        break;
      }
      if (m_numericRulerState->m_nameToListMap.find(list->m_name)!=m_numericRulerState->m_nameToListMap.end()) {
        STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: oops a list with name %s already exists\n", list->m_name.cstr()));
      }
      else {
        m_numericRulerState->m_nameToListMap[list->m_name]=list;
        librevenge::RVNGString simpName=libstoff::simplifyString(list->m_name);
        if (m_numericRulerState->m_simplifyNameToListMap.find(simpName)==m_numericRulerState->m_simplifyNameToListMap.end())
          m_numericRulerState->m_simplifyNameToListMap[simpName]=list;
      }
      break;
    }
    default:
      break;
    }
    if (done && input->tell()>pos)
      continue;
    char type;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "StarNumericList[list-" << type << "]:";
    done=false;
    switch (type) {
    case '+': { // extra outline
      zone.openFlagZone();
      int N=int(input->readULong(1));
      f << "N=" << N << ",";
      zone.closeFlagZone();
      if (input->tell()+3*N>zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: nExtraOutline seems bad\n"));
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
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: reading inHugeRecord(TEST_HUGE_DOCS) is not implemented\n"));
      break;
    case 'Z':
      done=true;
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: find unimplemented type\n"));
      f << "###type,";
      break;
    }
    if (!zone.closeSWRecord(type, "StarNumericList")) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (done)
      break;
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::read: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("StarNumericList:###extra");
  }
  return true;
}
catch (...)
{
  return false;
}

bool StarObjectNumericRuler::readLevel(StarZone &zone, STOFFListLevel &level)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='n' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(StarNumericList)[level-" << zone.getRecordLevel() << "]:";
  // sw_sw3num.cxx Sw3IoImp::InNumFmt
  level=STOFFListLevel();
  std::vector<uint32_t> string;
  librevenge::RVNGString fontName;
  for (int i=0; i<4; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readLevel: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSWRecord('n', "StarNumericList");
      return true;
    }
    if (string.empty()) continue;
    static char const *(wh[])= {"prefix", "postfix", "fontname", "fontstyle"};
    f << wh[i] << "=" << libstoff::getString(string).cstr() << ",";
    if (i==0) level.m_propertyList.insert("style:num-prefix",libstoff::getString(string));
    else if (i==1) level.m_propertyList.insert("style:num-suffix",libstoff::getString(string));
    else if (i==2) fontName=libstoff::getString(string);
  }
  f << "nFmt=" << input->readULong(2) << ",";
  int eType=int(input->readULong(1));
  uint8_t cBullet=uint8_t(input->readULong(1));
  int maxLevel=0;
  if (zone.isCompatibleWith(0x201))
    maxLevel=int(input->readULong(1));
  else {
    bool hasLevel;
    *input >> hasLevel;
    if (hasLevel) maxLevel=10;
  }
  level.m_startValue=int(input->readULong(2));
  int adjust=int(input->readULong(1));
  unsigned long leftSpace=input->readULong(4);
  long firstLineOffset=input->readLong(4);
  int fontFamily=int(input->readULong(1));
  int fontPitch=int(input->readULong(1));
  int charSet=int(input->readULong(1));
  if (eType<=4 || eType==9 || eType==10) {
    char const *(wh[])= {"A", "a", "I", "i", "1", "", "", "", "", "A"/*UPPER_LETTER_N*/, "a"/*LOWER_LETTER_N*/};
    level.m_propertyList.insert("style:num-format", wh[eType]);
    level.m_type=STOFFListLevel::NUMBER;
    f << "type=" << wh[eType] << ",";
  }
  else if (eType==5) {
    level.m_propertyList.insert("text:bullet-char", " ");
    level.m_type=STOFFListLevel::NONE;
    f << "none,";
  }
  else if (eType==6) {
    level.m_type=STOFFListLevel::BULLET;
    std::vector<uint8_t> buffer(1, cBullet);
    std::vector<uint32_t> res;
    std::vector<size_t> positions;
    // checkme if fontname is StarBats or StarMath, this does not works very well...
    StarEncoding::convert(buffer, StarEncoding::getEncodingForId(charSet), res, positions);
    level.m_propertyList.insert("text:bullet-char", libstoff::getString(res));
    f << "bullet=" << libstoff::getString(res).cstr() << ",";
  }
  else {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readLevel: unimplemented format\n"));
    // PAGEDESC, BITMAP
    level.m_type=STOFFListLevel::BULLET;
    librevenge::RVNGString bullet;
    libstoff::appendUnicode(0x2022, bullet); // checkme
    level.m_propertyList.insert("text:bullet-char", bullet);
    f << "#eType=" << eType << "," << "cBullet=" << cBullet << ",";
  }
  if (maxLevel) f << "nUpperLevel=" << maxLevel << ",";
  if (level.m_startValue) f << "start=" << level.m_startValue << ",";
  if (adjust<=6) {
    char const *(wh[])= {"left", "right", "justify", "center", "justify"/*block line*/, "end"};
    level.m_propertyList.insert("fo:text-align", wh[adjust]);
    f << wh[adjust] << ",";
  }
  else {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readLevel: unknown adjust\n"));
    f << "#adjust=" << adjust << ",";
  }

  if (leftSpace || firstLineOffset) { // checkme
    f << "left[offset]=" << leftSpace << ",";
    level.m_propertyList.insert("fo:margin-left", 0.05*double(long(leftSpace)+(firstLineOffset<0 ? -firstLineOffset: 0)), librevenge::RVNG_POINT);
  }
  if (firstLineOffset) {
    level.m_propertyList.insert("fo:text-indent", 0.05*double(firstLineOffset), librevenge::RVNG_POINT);
    f << "firstLine[offset]=" << firstLineOffset << ",";
  }
  if (fontFamily) f << "family[font]=" << fontFamily << ",";
  if (fontPitch) f << "pitch[font]=" << fontPitch << ",";
  if (charSet) f << "char[set]=" << charSet << ",";

  int cFlags=zone.openFlagZone();
  zone.closeFlagZone();
  if (cFlags&0xF0) f << "cFlags=" << (cFlags>>4) << ",";
  if (zone.isCompatibleWith(0x17, 0x22)) // the character format
    f << "nFmt=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x17,0x22, 0x101,0x201)) { // ignored by old LibreOffice
    f << "bRelSpace=" << input->readULong(1) << ",";
    f << "nRelLSpace=" << input->readLong(4) << ",";
  }
  if (zone.isCompatibleWith(0x17,0x22, 0x101)) {
    int charTextDistance=int(input->readULong(2));
    if (charTextDistance) {
      level.m_propertyList.insert("text:min-label-distance", 0.05*double(charTextDistance), librevenge::RVNG_POINT);
      f << "nTextOffset=" << charTextDistance << ",";
    }
    if (input->tell()<zone.getRecordLastPosition() && eType==8) {
      f << "width=" << input->readLong(4) << ",";
      f << "height=" << input->readLong(4) << ",";
      int cF=int(input->readULong(1));
      if (cF&1) f << "nVer[brush]=" << input->readULong(2) << ",";
      if (cF&2) f << "nVer[vertOrient]=" << input->readULong(2) << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('n', "StarNumericList");
  return true;
}

bool StarObjectNumericRuler::readList(StarZone &zone, shared_ptr<STOFFList> &list)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if ((input->peek()!='0' && input->peek()!='R') || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx::Sw3IoImp::InNumRule
  libstoff::DebugStream f;
  f << "Entries(StarNumericList)[" << zone.getRecordLevel() << "]:";
  int val;
  librevenge::RVNGString name;
  if (zone.isCompatibleWith(0x201)) {
    int cFlags=int(zone.openFlagZone());
    int nStringId=int(input->readULong(2));
    if (nStringId==0xFFFF)
      ;
    else if (!zone.getPoolName(nStringId, name))
      f << "###name=" << nStringId << ",";
    else if (!name.empty())
      f << name.cstr() << ",";
    if (cFlags&0x10) {
      val=int(input->readULong(2));     // 3<<11+1<<10+(num1,num2,...,num5,bul1,...,bul5)
      if (val!=0xFFFF) f << "frmtId=" << val << ",";
      val=int(input->readULong(2));
      if (val) f << "poolHelpId=" << val << ",";
      val=int(input->readULong(1));
      if (val) f << "poolHelpFileId=" << val << ",";
    }
    if (cFlags&0x20) f << "invalid,";
    if (cFlags&0x40) f << "continuous,";
    if ((cFlags&0x80) && zone.isCompatibleWith(0x211)) f << "absolute[spaces],";
  }
  val=int(input->readULong(1));
  // initialise list to default
  list.reset(new STOFFList(val==0));
  list->m_name=name;
  STOFFListLevel defLevel;
  defLevel.m_type=STOFFListLevel::NUMBER;
  defLevel.m_propertyList.insert("style:num-format", "1");
  for (int i=0; i<10; ++i) list->set(i+1, defLevel);
  if (val==0) f << "outline,";  // OUTLINE_RULE, NUM_RULE
  else if (val!=1) {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readList: unknown type\n"));
    f << "##eType=" << val << ",";
  }
  if (zone.isCompatibleWith(0x201))
    zone.closeFlagZone();
  int nFormat=int(input->readULong(1));
  long lastPos=zone.getRecordLastPosition();
  f << "nFormat=" << nFormat << ",";
  if (input->tell()+nFormat>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readList: nFormat seems bad\n"));
    f << "###,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarNumericList");
    return true;
  }
  std::vector<int> levels;
  f << "lvl=[";
  for (int i=0; i<nFormat; ++i) {
    levels.push_back(int(input->readULong(1)));
    f  << levels.back() << ",";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  int nKnownFormat=nFormat>10 ? 10 : nFormat;
  for (int i=0; i<nKnownFormat; ++i) {
    pos=input->tell();
    STOFFListLevel level;
    if (StarObjectNumericRuler::readLevel(zone, level)) {
      list->set(levels[size_t(i)]+1, level);
      continue;
    }
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readList: can not read a format\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSWRecord(type, "StarNumericList");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
