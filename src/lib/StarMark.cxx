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

#include "StarMark.hxx"

#include "STOFFDebug.hxx"
#include "StarZone.hxx"

////////////////////////////////////////////////////////////
//  StarMark
////////////////////////////////////////////////////////////
bool StarMark::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='K' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarMark::read: can not read a mark\n"));
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

std::ostream &operator<<(std::ostream &o, StarMark const &mark)
{
  o << "type=" << mark.m_type << ",";
  o << "id=" << mark.m_id << ",";
  o << "offset=" << mark.m_offset << ",";
  return o;
}

////////////////////////////////////////////////////////////
//  StarBookmark
////////////////////////////////////////////////////////////
bool StarBookmark::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='B' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarBookmark::read: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InBookmark
  f << "Entries(StarBookmark)[" << type << "-" << zone.getRecordLevel() << "]:";
  std::vector<uint32_t> text;
  bool ok=true;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      ok=false;
      STOFF_DEBUG_MSG(("StarBookmark::read: can not read a name\n"));
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
        STOFF_DEBUG_MSG(("StarBookmark::read: can not read macro name\n"));
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

bool StarBookmark::readList(StarZone &zone, std::vector<StarBookmark> &markList)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  char type;
  if (input->peek()!='a') return false;
  if (!zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarBookmark::readList: can not read a mark\n"));
    return false;
  }
  // sw_sw3misc.cxx InBookmarks
  f << "Entries(StarBookmark)[list-" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    StarBookmark bookmark;
    if (!bookmark.read(zone)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    markList.push_back(bookmark);
  }

  zone.closeSWRecord(type, "StarBookmark");
  return true;
}

std::ostream &operator<<(std::ostream &o, StarBookmark const &mark)
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
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
