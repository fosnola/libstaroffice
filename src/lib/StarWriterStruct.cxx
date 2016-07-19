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

#include "StarWriterStruct.hxx"

#include "StarFormatManager.hxx"
#include "StarObject.hxx"
#include "StarZone.hxx"

#include "STOFFDebug.hxx"

namespace StarWriterStruct
{
////////////////////////////////////////////////////////////
//  Bookmark
////////////////////////////////////////////////////////////
bool Bookmark::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='B' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Bookmark::read: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InBookmark
  f << "Entries(StarBookmark)[" << type << "-" << zone.getRecordLevel() << "]:";
  std::vector<uint32_t> text;
  bool ok=true;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      ok=false;
      STOFF_DEBUG_MSG(("StarWriterStruct::Bookmark::read: can not read a name\n"));
      f << "###short";
      break;
    }
    else if (i==0)
      m_shortName=libstoff::getString(text);
    else
      m_name=libstoff::getString(text);
  }
  if (ok) {
    zone.openFlagZone();
    m_offset=int(input->readULong(2));
    m_key=int(input->readULong(2));
    m_modifier=int(input->readULong(2));
    zone.closeFlagZone();
  }
  if (ok && input->tell()<zone.getRecordLastPosition()) {
    for (int i=0; i<4; ++i) { // start[aMac:aLib],end[aMac:Alib]
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarWriterStruct::Bookmark::read: can not read macro name\n"));
        f << "###macro";
        break;
      }
      else
        m_macroNames[i]=libstoff::getString(text);
    }
  }

  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarBookmark");
  return true;
}

bool Bookmark::readList(StarZone &zone, std::vector<Bookmark> &markList)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='a') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Bookmark::readList: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InBookmarks
  f << "Entries(StarBookmark)[list-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    Bookmark bookmark;
    if (!bookmark.read(zone)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    markList.push_back(bookmark);
  }

  zone.closeSWRecord(type, "StarBookmark");
  return true;
}

std::ostream &operator<<(std::ostream &o, Bookmark const &mark)
{
  if (!mark.m_shortName.empty()) o << "shortName=" << mark.m_shortName.cstr() << ",";
  if (!mark.m_name.empty()) o << "name=" << mark.m_name.cstr() << ",";
  if (mark.m_offset) o << "offset=" << mark.m_offset << ",";
  if (mark.m_key) o << "key=" << mark.m_key << ",";
  if (mark.m_modifier) o << "modifier=" << mark.m_modifier << ",";
  for (int i=0; i<4; i+=2) {
    if (mark.m_macroNames[i].empty() && mark.m_macroNames[i+1].empty()) continue;
    o << "macro[" << (i==0 ? "start" : "end") << "]=" << mark.m_macroNames[i].cstr() << ":" << mark.m_macroNames[i+1].cstr() << ",";
  }
  return o;
}

////////////////////////////////////////////////////////////
//  Layout
////////////////////////////////////////////////////////////
bool Layout::read(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='U' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::read: can not read a layout\n"));
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(StarLayout)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  *input>>m_version;
  f << "vers=" << std::hex << m_version << std::dec << ",";
  if (input->tell()!=zone.getFlagLastPosition()) // exists when m_version<0x201?
    f << "f1=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos) {
    pos=input->tell();
    bool done=false;
    switch (input->peek()) {
    case 0xd2:
      done=readD2(zone, object);
      break;
    case 0xd7:
      done=readD7(zone, object);
      break;
    default:
      break;
    }
    if (done && input->tell()!=pos) continue;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "StarLayout[###" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeSWRecord(type, "StarLayout");
  }

  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::read: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout:###");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord('U', "StarLayout");
  return true;
}

bool Layout::readD0(StarZone &zone, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!=0xd0 || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: can not read a layout\n"));
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  f << "StarLayout[D0-" << zone.getRecordLevel() << "]:";
  ascFile.addDelimiter(input->tell(),'|');
  int type1=int(input->readULong(1));
  if (type1) f << "f0=" << std::hex << type1 << std::dec << ","; // 13|37|3f+1|b7
  int addSize1=(type1&4) ? 2 : 0;
  int val=int(input->readULong(2));
  if (val!=0x8f) f << "f1=" << val << ","; // 8f|af
  int type2=int(input->readULong(1));
  f << "f2=" << std::hex << type2 << std::dec << ",";
  int addSize2=0;
  bool hasN=true;
  switch (type2) {
  case 5:
    addSize2=21;
    break;
  case 0x15:
    addSize2=23;
    break;
  case 0x20:
    addSize2=11;
    hasN=false;
    break;
  case 0x25:
    addSize2=24;
    break;
  default:
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: unknown type2=%d\n", type2));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(char(0xd0), "StarLayout");
    return true;
  }
  if (input->tell()+addSize1+addSize2+(hasN ? 1 : 0)>lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: the zone seems too short\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(char(0xd0), "StarLayout");
    return true;
  }
  if (addSize1+addSize2) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(addSize1+addSize2, librevenge::RVNG_SEEK_CUR);
  }

  int N1=0;
  if (hasN) {
    ascFile.addDelimiter(input->tell(),'|');
    // condition seems ok when version<=3 or version>=10, unsure when 3<version<0x10 :-~
    if (m_version>=0xa) { // SWG_SHORTFIELDS
      N1=int(input->readULong(1));
      if (!N1) N1=int(input->readULong(2));
    }
    else
      N1=int(input->readULong(2));
    f << "N=" << N1 << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  bool ok=true;
  for (int i=0; i<N1; ++i) {
    pos=input->tell();
    int cType=input->peek();
    if ((cType&0xc0)!=0xc0 || !zone.openSWRecord(type)) {
      STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: oops can not find a A-data\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
    // find type c2|c3|c6|c8
    f.str("");
    f << "StarLayout[#" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeSWRecord(type, "StarLayout");
  }

  pos=input->tell();
  if (ok && pos+(m_version>=0x201 ? 3:4)<=lastPos) {
    // vers<=0101 && vers>=201 ok, other?
    int N2=int(input->readULong(m_version>=0x201 ? 1 : 2));
    if (input->tell()+5*N2>lastPos)
      input->seek(pos, librevenge::RVNG_SEEK_SET);
    else {
      f.str("");
      f << "StarLayout-B:N=" << N2 << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      for (int i=0; i<N2; ++i) {
        pos=input->tell();
        if (pos+5>lastPos) {
          STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: oops can not find a B-data\n"));
          break;
        }
        f.str("");
        f << "StarLayout-B" << i << ":";
        f << "id=" << input->readULong(2) << ",";
        int cType=input->peek();
        if ((cType&0xc0)!=0xc0 || !zone.openSWRecord(type)) {
          input->seek(pos, librevenge::RVNG_SEEK_SET);
          STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: oops can not find a B-data\n"));
          break;
        }
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        // find type c1|c4|cc
        pos=input->tell();
        f.str("");
        f << "StarLayout-B[#" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        zone.closeSWRecord(type, "StarLayout");
      }
    }
  }
  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD0: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout[D0]:##extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord(char(0xd0), "StarLayout");
  return true;
}

bool Layout::readD2(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!=0xd2 || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD2: can not read a layout\n"));
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  f << "StarLayout[D2-" << zone.getRecordLevel() << "]:";
  int val=int(input->readULong(2));
  if (val!=0xaf11) f << "f0=" << std::hex << val << std::dec << ",";
  val=int(input->readULong(1)); // small value 1-1f
  if (val) f << "f1=" << val << ",";
  int fl=zone.openFlagZone();
  if (fl&0xF0) f << "flag=" << (fl>>4) << ",";
  if (zone.getFlagLastPosition()!=input->tell()) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(zone.getFlagLastPosition(), librevenge::RVNG_SEEK_SET);
  }
  zone.closeFlagZone();

  if (input->tell()+11>lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD2: the zone seems too short\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(char(0xd2), "StarLayout");
    return true;
  }
  ascFile.addDelimiter(input->tell(),'|');
  input->seek(input->tell()+11, librevenge::RVNG_SEEK_SET);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos) {
    pos=input->tell();
    bool done=false;
    switch (input->peek()) {
    case 'l': {
      shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=object.getFormatManager()->readSWFormatDef(zone,'l', format, object);
      break;
    }
    case 0xd0:
      done=readD0(zone, object);
      break;
    default:
      break;
    }
    if (done && input->tell()!=pos) continue;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "StarLayout[###" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeSWRecord(type, "StarLayout");
  }

  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD2: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout:###");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord(char(0xd2), "StarLayout");
  return true;
}

bool Layout::readD7(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!=0xd7 || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD7: can not read a layout\n"));
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  f << "StarLayout[D7-" << zone.getRecordLevel() << "]:";
  int val=int(input->readULong(2));
  if (val!=0xaf11) f << "f0=" << std::hex << val << std::dec << ",";
  val=int(input->readULong(1)); // small value 1-1f
  if (val) f << "f1=" << val << ",";
  int fl=zone.openFlagZone();
  if (fl&0xF0) f << "flag=" << (fl>>4) << ",";
  if (zone.getFlagLastPosition()!=input->tell()) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(zone.getFlagLastPosition(), librevenge::RVNG_SEEK_SET);
  }
  zone.closeFlagZone();

  if (input->tell()+9>lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD7: the zone seems too short\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(char(0xd7), "StarLayout");
    return true;
  }
  ascFile.addDelimiter(input->tell(),'|');
  input->seek(input->tell()+9, librevenge::RVNG_SEEK_SET);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos) {
    pos=input->tell();
    bool done=false;
    switch (input->peek()) {
    case 'l': {
      shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=object.getFormatManager()->readSWFormatDef(zone,'l', format, object);
      break;
    }
    case 0xd0:
      done=readD0(zone, object);
      break;
    default:
      break;
    }
    if (done && input->tell()!=pos) continue;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "StarLayout[###" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeSWRecord(type, "StarLayout");
  }

  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarWriterStruct::Layout::readD7: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout:###");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord(char(0xd7), "StarLayout");
  return true;
}

////////////////////////////////////////////////////////////
//  Macro
////////////////////////////////////////////////////////////
bool Macro::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='m' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Macro::read: can not read a macro\n"));
    return false;
  }
  // sw_sw3misc.cxx InMacroTable
  f << "Entries(StarMacro)[" << zone.getRecordLevel() << "]:";
  m_key=int(input->readULong(2));
  bool ok=true;
  for (int i=0; i<2; ++i) {
    std::vector<uint32_t> string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWMacroTable: can not read a string\n"));
      f << "###name,";
      ok=false;
      break;
    }
    m_names[i]=libstoff::getString(string);
  }
  if (ok && zone.isCompatibleWith(0x102))
    m_scriptType=int(input->readULong(2));
  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarMacro");
  return true;
}

bool Macro::readList(StarZone &zone, std::vector<Macro> &macroList)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='u') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Macro::readList: can not read a macro\n"));
    return false;
  }
  // sw_sw3misc.cxx InMacroTable
  f << "Entries(StarMacro)[list-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    Macro macro;
    if (!macro.read(zone)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    macroList.push_back(macro);
  }

  zone.closeSWRecord(type, "StarMacro");
  return true;
}

std::ostream &operator<<(std::ostream &o, Macro const &macro)
{
  if (macro.m_key) o << "key=" << macro.m_key << ",";
  for (int i=0; i<2; ++i) {
    if (!macro.m_names[i].empty())
      o << "name" << i << "=" << macro.m_names[i].cstr() << ",";
  }
  if (macro.m_scriptType) o << "type[script]=" << macro.m_scriptType << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  Mark
////////////////////////////////////////////////////////////
bool Mark::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='K' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Mark::read: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InNodeMark
  f << "Entries(StarMark)[" << type << "-" << zone.getRecordLevel() << "]:";
  m_type=int(input->readULong(1));
  m_id=int(input->readULong(2));
  m_offset=int(input->readULong(2));
  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarMark");
  return true;
}

std::ostream &operator<<(std::ostream &o, Mark const &mark)
{
  o << "type=" << mark.m_type << ",";
  o << "id=" << mark.m_id << ",";
  o << "offset=" << mark.m_offset << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  NodeRedline
////////////////////////////////////////////////////////////
bool NodeRedline::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='v' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::NodeRedline::read: can not read a nodeRedline\n"));
    return false;
  }
  // sw_sw3misc.cxx InNodeNodeRedline
  f << "Entries(StarNodeRedline)[" << type << "-" << zone.getRecordLevel() << "]:";
  m_flags=zone.openFlagZone();
  m_id=int(input->readULong(2));
  m_offset=int(input->readULong(2));
  zone.closeFlagZone();
  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarNodeRedline");
  return true;
}

std::ostream &operator<<(std::ostream &o, NodeRedline const &nodeRedline)
{
  o << "id=" << nodeRedline.m_id << ",";
  o << "offset=" << nodeRedline.m_offset << ",";
  if (nodeRedline.m_flags)
    o << "flags=" << std::hex << nodeRedline.m_flags << std::dec << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  NoteInfo
////////////////////////////////////////////////////////////
bool NoteInfo::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!=(m_isFootnote ? '1' : '4') || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::NoteInfo::read: can not read a noteInfo\n"));
    return false;
  }
  f << "Entries(StarNoteInfo)[" << (m_isFootnote ? "footnote" : "endnote") << "-" << zone.getRecordLevel() << "]:";
  // sw_sw3num.cxx: InFtnInfo and InFntInfo40 InEndNoteInfo
  bool oldFootnote=m_isFootnote && !zone.isCompatibleWith(0x201);
  std::vector<uint32_t> text;
  if (oldFootnote) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarWriterStruct::NoteInfo::read: can not read a string\n"));
        f << *this << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord(type, "StarNoteInfo");
      }
      m_strings[i+2]=libstoff::getString(text);
    }
  }
  int fl=zone.openFlagZone();

  if (oldFootnote) {
    m_posType=int(input->readULong(1));
    m_numType=int(input->readULong(1));
  }
  m_type=int(input->readULong(1));
  for (int i=0; i<2; ++i)
    m_idx[i]=int(input->readULong(2));
  if (zone.isCompatibleWith(0xc))
    m_ftnOffset=int(input->readULong(2));
  if (zone.isCompatibleWith(0x203))
    m_idx[2]=int(input->readULong(2));
  if (zone.isCompatibleWith(0x216) && (fl&0x10))
    m_idx[3]=int(input->readULong(2));
  zone.closeFlagZone();

  if (zone.isCompatibleWith(0x203)) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarWriterStruct::NoteInfo::read: can not read a string\n"));
        f << *this << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord(type, "StarNoteInfo");
        return true;
      }
      m_strings[i]=libstoff::getString(text);
    }
  }

  if (m_isFootnote && !oldFootnote) {
    zone.openFlagZone();
    m_posType=int(input->readULong(1));
    m_numType=int(input->readULong(1));
    zone.closeFlagZone();
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarWriterStruct::NoteInfo::read: can not read a string\n"));
        f << *this << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeSWRecord(type, "StarNoteInfo");
        return true;
      }
      m_strings[i+2]=libstoff::getString(text);
    }
  }

  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarNoteInfo");
  return true;
}

std::ostream &operator<<(std::ostream &o, NoteInfo const &info)
{
  o << (info.m_isFootnote ? "footnote" : "endnote") << ",";
  if (info.m_type) o << "type=" << info.m_type << ",";
  for (int i=0; i<4; ++i) {
    if (info.m_idx[i]==0xFFFF) continue;
    char const *(wh[])= {"pageId", "collIdx", "charIdx", "anchorCharIdx"};
    o << wh[i] << "=" << info.m_idx[i] << ",";
  }
  if (info.m_ftnOffset) o << "ftnOffset=" << info.m_ftnOffset << ",";
  for (int i=0; i<4; ++i) {
    if (info.m_strings[i].empty()) continue;
    char const *(wh[])= {"prefix", "suffix", "quoValis", "ergoSum"};
    o << wh[i] << "=" << info.m_strings[i].cstr() << ",";
  }
  if (info.m_posType) o << "type[pos]=" << info.m_posType << ",";
  if (info.m_numType) o << "type[number]=" << info.m_numType << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  Redline
////////////////////////////////////////////////////////////
bool Redline::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='D' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Redline::read: can not read a redline\n"));
    return false;
  }
  f << "Entries(StarRedline)[" << zone.getRecordLevel() << "]:";
  // sw_sw3redline.cxx inRedline
  zone.openFlagZone();
  m_type=int(input->readULong(1));
  m_stringId=int(input->readULong(2));
  zone.closeFlagZone();
  m_date=long(input->readULong(4));
  m_time=long(input->readULong(4));
  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWRedlineList: can not read the comment\n"));
    f << "###comment";
  }
  else
    m_comment=libstoff::getString(text);
  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarRedline");
  return true;
}

bool Redline::readList(StarZone &zone, std::vector<Redline> &redlineList)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='R') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Redline::readList: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InRedlines
  f << "Entries(StarRedline)[list-" << zone.getRecordLevel() << "]:";
  zone.openFlagZone();
  int N=int(input->readULong(2));
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int i=0; i<N; ++i) {
    pos=input->tell();
    Redline redline;
    if (!redline.read(zone)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    redlineList.push_back(redline);
  }

  zone.closeSWRecord(type, "StarRedline");
  return true;
}

bool Redline::readListList(StarZone &zone, std::vector<std::vector<Redline> > &redlineListList)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='V') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::Redline::readListList: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InRedlines
  f << "Entries(StarRedline)[listList-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell() < zone.getRecordLastPosition()) {
    pos=input->tell();
    std::vector<Redline> redlineList;
    if (!readList(zone, redlineList)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    redlineListList.push_back(redlineList);
  }

  zone.closeSWRecord(type, "StarRedline");
  return true;
}

std::ostream &operator<<(std::ostream &o, Redline const &redline)
{
  if (redline.m_type) o << "type=" << redline.m_type << ",";
  if (redline.m_stringId) o << "stringId=" << redline.m_stringId << ",";
  if (redline.m_date) o << "date=" << redline.m_date << ",";
  if (redline.m_time) o << "time=" << redline.m_time << ",";
  if (!redline.m_comment.empty()) o << "comment=" << redline.m_comment.cstr() << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  TOX
////////////////////////////////////////////////////////////
TOX::~TOX()
{
}

bool TOX::read(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='x' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::TOX::read::read: can not read a nodeRedline\n"));
    return false;
  }
  long lastRecordPos=zone.getRecordLastPosition();
  // sw_sw3misc.cxx: InTOX
  f << "Entries(StarTOX)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0)
    f << "fl=" << (fl>>4) << ",";
  m_type=int(input->readULong(2));
  m_createType=int(input->readULong(2));
  m_captionDisplay=int(input->readULong(2));
  for (int i=0; i<2; ++i) m_stringIds[i]=int(input->readULong(2));
  m_data=int(input->readULong(2));
  m_formFlags=int(input->readULong(1));
  zone.closeFlagZone();
  std::vector<uint32_t> string;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarWriterStruct::TOX::read: can not read aName\n"));
      f << *this;
      f << "###aName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "StarTOX");
      return true;
    }
    if (i==0)
      m_name=libstoff::getString(string);
    else
      m_title=libstoff::getString(string);
  }
  if (zone.isCompatibleWith(0x215)) {
    m_OLEOptions=int(input->readULong(2));
    m_styleId=int(input->readULong(2));
  }
  else {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarWriterStruct::TOX::read: can not read aDummy\n"));
      f << "###aDummy";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "StarTOX");
      return true;;
    }
    if (!string.empty())
      f << "aDummy=" << libstoff::getString(string).cstr() << ",";
  }

  int N=int(input->readULong(1));
  f << "nPatterns=" << N << ",";
  bool ok=true;
  for (int i=0; i<N; ++i) { // storeme
    if (object.getFormatManager()->readSWPatternLCL(zone))
      continue;
    ok=false;
    f << "###pat";
    break;
  }
  if (!ok) {
    f << *this;
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarTOX");
    return true;
  }
  N=int(input->readULong(1));
  for (int i=0; i<N; ++i)
    m_stringIdList.push_back(int(input->readULong(2)));
  N=int(input->readULong(1));
  for (int i=0; i<N; ++i) {
    Style style;
    style.m_level=int(input->readULong(1));
    int nCount=int(input->readULong(2));
    f << "nCount=" << nCount << ",";
    if (input->tell()+2*nCount>lastRecordPos) {
      STOFF_DEBUG_MSG(("StarWriterStruct::TOX::read: can not read some string id\n"));
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
        style.m_names.push_back(poolName);
    }
  }
  if (!ok) {
    f << *this;
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarTOX");
    return true;
  }
  fl=zone.openFlagZone();
  m_stringIds[2]=int(input->readULong(2));
  m_titleLength=long(input->readULong(4));
  zone.closeFlagZone();

  if ((fl&0x10)) {
    while (input->tell()<lastRecordPos && input->peek()=='s') {
      shared_ptr<StarFormatManagerInternal::FormatDef> format;
      if (!object.getFormatManager()->readSWFormatDef(zone,'s', format, object)) {
        STOFF_DEBUG_MSG(("StarWriterStruct::TOX::read: can not read some format\n"));
        f << "###format,";
        break;
      }
      m_formatList.push_back(format);
    }
  }

  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarTOX");
  return true;
}

bool TOX::readList(StarZone &zone, std::vector<TOX> &toxList, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='u') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::TOX::readList: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InTOXs
  f << "Entries(StarTOX)[list-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    TOX tox;
    if (!tox.read(zone, object)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    toxList.push_back(tox);
  }

  zone.closeSWRecord(type, "StarTOX");
  return true;
}

std::ostream &operator<<(std::ostream &o, TOX const &tox)
{
  if (tox.m_type) o << "type=" << tox.m_type << ",";
  if (tox.m_createType) o << "type[create]=" << tox.m_createType << ",";
  if (tox.m_captionDisplay) o << "captionDisplay=" << tox.m_captionDisplay << ",";
  for (int i=0; i<3; ++i) {
    if (tox.m_stringIds[i]==0xFFFF) continue;
    char const *(wh[])= {"stringId", "seqStringId", "sectStringId"};
    o << wh[i] << "=" << tox.m_stringIds[i] << ",";
  }
  if (tox.m_styleId!=0xFFFF) o << "styleId=" << tox.m_styleId << ",";
  if (tox.m_data) o << "data=" << tox.m_data << ",";
  if (tox.m_formFlags) o << "formFlags=" << std::hex << tox.m_formFlags << std::dec << ",";
  if (!tox.m_title.empty()) o << "title=" << tox.m_title.cstr() << ",";
  if (!tox.m_name.empty()) o << "name=" << tox.m_name.cstr() << ",";
  if (tox.m_OLEOptions) o << "OLEOptions=" << tox.m_OLEOptions << ",";
  if (!tox.m_stringIdList.empty()) {
    o << "stringIdList=[";
    for (size_t i=0; i<tox.m_stringIdList.size(); ++i) {
      if (tox.m_stringIdList[i]==0xFFFF)
        o << "_,";
      else
        o << tox.m_stringIdList[i] << ",";
    }
    o << "],";
  }
  if (!tox.m_styleList.empty()) {
    o << "styleList=[";
    for (size_t i=0; i<tox.m_styleList.size(); ++i)
      o << "[" << tox.m_styleList[i] << "],";
    o << "],";
  }
  if (tox.m_titleLength) o << "titleLength=" << tox.m_titleLength << ",";
  if (!tox.m_formatList.empty()) o << "nFormat=" << tox.m_formatList.size() << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  TOX51
////////////////////////////////////////////////////////////
TOX51::~TOX51()
{
}

bool TOX51::read(StarZone &zone, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='x' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::TOX51::read: can not read a tox51\n"));
    return false;
  }
  // sw_sw3misc.cxx InNodeTOX51
  f << "Entries(StarTox51)[" << type << "-" << zone.getRecordLevel() << "]:";
  std::vector<uint32_t> string;
  if (zone.isCompatibleWith(0x201)) {
    int strId=int(input->readULong(2));
    if (strId!=0xFFFF && !zone.getPoolName(strId, m_typeName))
      f << "###nPoolId=" << strId << ",";
  }
  else {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarWriterStruct::TOX51::read: can not read typeName\n"));
      f << *this << "###typeName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord(type, "StarTox51");
      return true;
    }
    m_typeName=libstoff::getString(string);
  }
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarWriterStruct::TOX51::read: can not read aTitle\n"));
    f << *this << "###aTitle";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarTox51");
    return true;
  }
  m_title=libstoff::getString(string);
  int fl=zone.openFlagZone();
  m_createType=int(input->readULong(2));
  m_type=int(input->readULong(1));
  if (zone.isCompatibleWith(0x213) && (fl&0x10))
    m_firstTabPos=int(input->readULong(2));

  int N=int(input->readULong(1));
  bool ok=true;
  for (int i=0; i<N; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarWriterStruct::TOX51::read: can not read a pattern name\n"));
      f << "###pat";
      ok=false;
      break;
    }
    m_patternList.push_back(libstoff::getString(string));
  }
  if (!ok) {
    f << *this;
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarTox51");
    return true;
  }
  N=int(input->readULong(1));
  for (int i=0; i<N; ++i)
    m_stringIdList.push_back(int(input->readULong(2)));
  m_infLevel=int(input->readULong(2));

  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarTox51");
  return true;
}


bool TOX51::readList(StarZone &zone, std::vector<TOX51> &toxList, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='y') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarWriterStruct::TOX51::readList: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InTOX51s
  f << "Entries(StarTox51)[list-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    TOX51 tox;
    if (!tox.read(zone, object)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    toxList.push_back(tox);
  }

  zone.closeSWRecord(type, "StarTox51");
  return true;
}

std::ostream &operator<<(std::ostream &o, TOX51 const &tox)
{
  if (!tox.m_typeName.empty()) o << "type[name]=" << tox.m_typeName.cstr() << ",";
  if (tox.m_type) o << "type=" << tox.m_type << ",";
  if (tox.m_createType) o << "type[create]=" << tox.m_createType << ",";
  if (tox.m_firstTabPos) o << "firstTabPos=" << tox.m_firstTabPos << ",";
  if (!tox.m_title.empty()) o << "title=" << tox.m_title.cstr() << ",";
  if (!tox.m_patternList.empty()) {
    o << "patternList=[";
    for (size_t i=0; i<tox.m_patternList.size(); ++i)
      o << tox.m_patternList[i].cstr() << ",";
    o << "],";
  }
  if (!tox.m_stringIdList.empty()) {
    o << "stringIdList=[";
    for (size_t i=0; i<tox.m_stringIdList.size(); ++i) {
      if (tox.m_stringIdList[i]==0xFFFF)
        o << "_,";
      else
        o << tox.m_stringIdList[i] << ",";
    }
    o << "],";
  }
  if (tox.m_infLevel) o << "infLevel=" << tox.m_infLevel << ",";
  return o;
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
