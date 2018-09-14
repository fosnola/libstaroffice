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
#include "StarBitmap.hxx"
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
  State()
    : m_nameToListMap()
    , m_simplifyNameToListMap()
  {
  }
  //! a map list name to list
  std::map<librevenge::RVNGString, std::shared_ptr<STOFFList> > m_nameToListMap;
  //! a map list simpl name to list
  std::map<librevenge::RVNGString, std::shared_ptr<STOFFList> > m_simplifyNameToListMap;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectNumericRuler::StarObjectNumericRuler(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_numericRulerState(new StarObjectNumericRulerInternal::State)
{
}

StarObjectNumericRuler::~StarObjectNumericRuler()
{
}

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
std::shared_ptr<STOFFList> StarObjectNumericRuler::getList(librevenge::RVNGString const &name) const
{
  if (name.empty())
    return std::shared_ptr<STOFFList>();
  if (m_numericRulerState->m_nameToListMap.find(name)!=m_numericRulerState->m_nameToListMap.end())
    return m_numericRulerState->m_nameToListMap.find(name)->second;
  auto simpName=libstoff::simplifyString(name);
  if (m_numericRulerState->m_simplifyNameToListMap.find(simpName)!=m_numericRulerState->m_simplifyNameToListMap.end())
    return m_numericRulerState->m_simplifyNameToListMap.find(simpName)->second;
  STOFF_DEBUG_MSG(("StarObjectNumericRuler::getList: can not find list with name %s\n", name.cstr()));
  return std::shared_ptr<STOFFList>();
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
      std::shared_ptr<STOFFList> list;
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
        auto simpName=libstoff::simplifyString(list->m_name);
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
    unsigned char type;
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
      auto N=int(input->readULong(1));
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
  unsigned char type;
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
    static char const *wh[]= {"prefix", "postfix", "fontname", "fontstyle"};
    f << wh[i] << "=" << libstoff::getString(string).cstr() << ",";
    if (i==0) level.m_propertyList.insert("style:num-prefix",libstoff::getString(string));
    else if (i==1) level.m_propertyList.insert("style:num-suffix",libstoff::getString(string));
    else if (i==2) fontName=libstoff::getString(string);
  }
  auto format=int(input->readULong(2));
  auto eType=int(input->readULong(1));
  auto cBullet=uint8_t(input->readULong(1));
  int maxLevel=0;
  if (zone.isCompatibleWith(0x201))
    maxLevel=int(input->readULong(1));
  else {
    bool hasLevel;
    *input >> hasLevel;
    if (hasLevel) maxLevel=10;
  }
  level.m_startValue=int(input->readULong(2));
  auto adjust=int(input->readULong(1));
  unsigned long leftSpace=input->readULong(4);
  long firstLineOffset=input->readLong(4);
  auto fontFamily=int(input->readULong(1));
  auto fontPitch=int(input->readULong(1));
  auto charSet=int(input->readULong(1));
  bool isSymbolFont=false;
  if (!fontName.empty()) {
    // todo find where other font characteristic are stored
    level.m_font.reset(new STOFFFont);
    STOFFFont &font=*level.m_font;
    font.m_propertyList.insert("style:font-name", fontName);
    if (fontName=="StarBats" || fontName=="StarMath") {
      isSymbolFont=true;
      font.m_propertyList.insert("style:font-charset","x-symbol");
    }
  }
  if (format!=0xFFFF) f << "nFmt=" << format << ",";
  if (eType<=4 || eType==9 || eType==10) {
    char const *wh[]= {"A", "a", "I", "i", "1", "", "", "", "", "A"/*UPPER_LETTER_N*/, "a"/*LOWER_LETTER_N*/};
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
    auto encoding=(charSet==0 && isSymbolFont) ? StarEncoding::E_SYMBOL : StarEncoding::getEncodingForId(charSet);
    StarEncoding::convert(buffer, encoding, res, positions);
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
  if (adjust>=0 && adjust<=5) {
    char const *wh[]= {"left", "right", "justify", "center", "justify"/*block line*/, "end"};
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
    auto charTextDistance=int(input->readULong(2));
    if (charTextDistance) {
      level.m_propertyList.insert("text:min-label-distance", 0.05*double(charTextDistance), librevenge::RVNG_POINT);
      f << "nTextOffset=" << charTextDistance << ",";
    }
    if (input->tell()<zone.getRecordLastPosition() && eType==8) {
      f << "width=" << input->readLong(4) << ",";
      f << "height=" << input->readLong(4) << ",";
      auto cF=int(input->readULong(1));
      if (cF&1) f << "nVer[brush]=" << input->readULong(2) << ",";
      if (cF&2) f << "nVer[vertOrient]=" << input->readULong(2) << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('n', "StarNumericList");
  return true;
}

bool StarObjectNumericRuler::readList(StarZone &zone, std::shared_ptr<STOFFList> &list)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
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
    auto cFlags=int(zone.openFlagZone());
    auto nStringId=int(input->readULong(2));
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
  if (val==0) f << "outline,";  // OUTLINE_RULE, NUM_RULE
  else if (val!=1) {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readList: unknown type\n"));
    f << "##eType=" << val << ",";
  }
  list->m_name=name;
  STOFFListLevel defLevel;
  defLevel.m_type=STOFFListLevel::NUMBER;
  defLevel.m_propertyList.insert("style:num-format", "1");
  for (int i=0; i<10; ++i) list->set(i+1, defLevel);
  if (zone.isCompatibleWith(0x201))
    zone.closeFlagZone();
  auto nFormat=int(input->readULong(1));
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

bool StarObjectNumericRuler::readAttributeLevel(StarZone &zone, int vers, long lastPos, STOFFListLevel &level)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:paraBullet,";

  level=STOFFListLevel();
  // svx_bulitem.cxx SvxBulletItem::SvxBulletItem
  uint16_t style;
  *input >> style;
  uint16_t charSet=0;
  bool isSymbolFont=false;
  int alignement=-1;
  if (style==128) {
    f << "bitmap,";
    StarBitmap bitmap;
    librevenge::RVNGBinaryData data;
    std::string dType;
    if (!bitmap.readBitmap(zone, true, lastPos, data, dType)) {
      f << "###BM,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
  }
  else {
    f << "type=" << style << ",";
    // SvxBulletItem::CreateFont
    f << "font=[";
    level.m_font.reset(new STOFFFont);
    auto &font=*level.m_font;

    STOFFColor col;
    if (!input->readColor(col)) {
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: can not read a color\n"));
      f << "###color,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    else {
      font.m_propertyList.insert("fo:color", col.str().c_str());
      if (!col.isBlack())
        f << "color=" << col << ",";
    }
    uint16_t family, pitch, align, weight, underline, strikeOut, italic;
    *input >> family >> charSet >> pitch >> align >> weight >> underline >> strikeOut >> italic;
    alignement=int(align);
    if (family) f << "family=" << family << ",";
    if (charSet) f << "charSet=" << charSet << ",";
    if (pitch) f << "pitch=" << pitch << ",";
    if (weight) f << "weight=" << weight << ",";
    if (underline) f << "underline=" << underline << ",";
    if (strikeOut) f << "strikeOut=" << strikeOut << ",";
    if (italic) f << "italic=" << italic << ",";
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: can not read a name\n"));
      f << "###fontName,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (!text.empty()) {
      auto name=libstoff::getString(text);
      font.m_propertyList.insert("style:font-name", name);
      f << name.cstr() << ",";
      if (name=="StarBats" || name=="StarMath") {
        font.m_propertyList.insert("style:font-charset","x-symbol");
        isSymbolFont=true;
      }
    }
    if (vers==1) f << "size=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    bool outline, shadow, transparent;
    *input >> outline >>  shadow >>  transparent;
    if (outline) f << "outline,";
    if (shadow) f << "shadow,";
    if (transparent) f << "transparent,";
    f << "],";
    //
    // time to update the font
    //
    font.m_propertyList.insert("style:font-pitch", pitch==1 ? "fixed" : "variable");
    if (weight==5)
      font.m_propertyList.insert("fo:font-weight", "normal");
    else if (weight==8)
      font.m_propertyList.insert("fo:font-weight", "bold");
    else if (weight>=1 && weight<=9)
      font.m_propertyList.insert("fo:font-weight", weight*100, librevenge::RVNG_GENERIC);
    else // 10: WEIGHT_BLACK
      font.m_propertyList.insert("fo:font-weight", "normal");
    switch (underline) {
    case 0: // none
    case 4: // unknown
      font.m_propertyList.insert("style:text-underline-type", "none");
      break;
    case 1: // single
    case 2: // double
      font.m_propertyList.insert("style:text-underline-type", underline==1 ? "single" : "double");
      font.m_propertyList.insert("style:text-underline-style", "solid");
      break;
    case 3: // dot
    case 5: // dash
    case 6: // long-dash
    case 7: // dash-dot
    case 8: // dash-dot-dot
      font.m_propertyList.insert("style:text-underline-type", "single");
      font.m_propertyList.insert("style:text-underline-style", underline==3 ? "dotted" : underline==5 ? "dash" :
                                 underline==6 ? "long-dash" : underline==7 ? "dot-dash" : "dot-dot-dash");
      break;
    case 9: // small wave
    case 10: // wave
    case 11: // double wave
      font.m_propertyList.insert("style:text-underline-type", underline==11 ? "single" : "double");
      font.m_propertyList.insert("style:text-underline-style", "wave");
      if (underline==9)
        font.m_propertyList.insert("style:text-underline-width", "thin");
      break;
    case 12: // bold
    case 13: // bold dot
    case 14: // bold dash
    case 15: // bold long-dash
    case 16: // bold dash-dot
    case 17: // bold dash-dot-dot
    case 18: { // bold wave
      char const *wh[]= {"solid", "dotted", "dash", "long-dash", "dot-dash", "dot-dot-dash", "wave"};
      font.m_propertyList.insert("style:text-underline-type", "single");
      font.m_propertyList.insert("style:text-underline-style", wh[underline-12]);
      font.m_propertyList.insert("style:text-underline-width", "thick");
      break;
    }
    default:
      font.m_propertyList.insert("style:text-underline-type", "none");
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: find unknown underline enum=%d\n", underline));
      break;
    }
    switch (strikeOut) {
    case 0: // none
      font.m_propertyList.insert("style:text-line-through-type", "none");
      break;
    case 1: // single
    case 2: // double
      font.m_propertyList.insert("style:text-line-through-type", strikeOut==1 ? "single" : "double");
      font.m_propertyList.insert("style:text-line-through-style", "solid");
      break;
    case 3: // dontknow
      break;
    case 4: // bold
      font.m_propertyList.insert("style:text-line-through-type", "single");
      font.m_propertyList.insert("style:text-line-through-style", "solid");
      font.m_propertyList.insert("style:text-line-through-width", "thick");
      break;
    case 5: // slash
    case 6: // X
      font.m_propertyList.insert("style:text-line-through-type", "single");
      font.m_propertyList.insert("style:text-line-through-style", "solid");
      font.m_propertyList.insert("style:text-line-through-text", strikeOut==5 ? "/" : "X");
      break;
    default:
      font.m_propertyList.insert("style:text-line-through-type", "none");
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: find unknown crossedout enum=%d\n", strikeOut));
      break;
    }
    if (italic==1)
      font.m_propertyList.insert("fo:font-style", "oblique");
    else if (italic==2)
      font.m_propertyList.insert("fo:font-style", "italic");
    else if (italic) {
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: find unknown posture enum=%d\n", italic));
    }
    font.m_propertyList.insert("style:text-outline", outline);
    font.m_propertyList.insert("fo:text-shadow", shadow ? "1pt 1pt" : "none");
  }
  auto width=int(input->readLong(4));
  auto start=int(input->readULong(2));
  auto justify=int(input->readULong(1));
  auto symbol=int(input->readULong(1));
  auto scale=int(input->readULong(2));
  if (width) f << "width=" << width << ",";
  if (start) f << "start=" << start << ",";
  if (justify) f << "justify=" << justify << ",";
  if (symbol) f << "symbol=" << symbol << ",";
  if (scale) f << "scale=" << scale << ",";

  if (style<=4) {
    char const *wh[]= {"A", "a", "I", "i", "1"};
    level.m_propertyList.insert("style:num-format", wh[style]);
    level.m_type=STOFFListLevel::NUMBER;
  }
  else if (style==5 || (style==6 && symbol==0)) {
    level.m_propertyList.insert("text:bullet-char", " ");
    level.m_type=STOFFListLevel::NONE;
  }
  else if (style==6) {
    level.m_type=STOFFListLevel::BULLET;
    std::vector<uint8_t> buffer(1, uint8_t(symbol));
    std::vector<uint32_t> res;
    std::vector<size_t> positions;
    auto encoding=(charSet==0 && isSymbolFont) ? StarEncoding::E_SYMBOL : StarEncoding::getEncodingForId(charSet);
    StarEncoding::convert(buffer, encoding, res, positions);
    level.m_propertyList.insert("text:bullet-char", libstoff::getString(res));
  }
  else {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: unimplemented style %d\n", style));
    // BITMAP
    level.m_type=STOFFListLevel::BULLET;
    librevenge::RVNGString bullet;
    libstoff::appendUnicode(0x2022, bullet); // checkme
    level.m_propertyList.insert("text:bullet-char", bullet);
  }
  if (alignement>=0 && alignement<6) {
    char const *wh[]= {"left", "right", "justify", "center", "justify"/*block line*/, "end"};
    level.m_propertyList.insert("fo:text-align", wh[alignement]);
  }
  else if (alignement>=0) {
    STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: unknown align %d\n", alignement));
  }

  for (int i=0; i<2; ++i) {
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectNumericRuler::readAttributeLevel: can not read a name\n"));
      f << "###text,";
      break;
    }
    else if (text.empty())
      continue;
    f << (i==0 ? "prefix" : "suffix") << "=" << libstoff::getString(text).cstr() << ",";
    if (i==0) level.m_propertyList.insert("style:num-prefix",libstoff::getString(text));
    else if (i==1) level.m_propertyList.insert("style:num-suffix",libstoff::getString(text));
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=lastPos;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
