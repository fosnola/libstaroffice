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

#include "StarZone.hxx"

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarZone::StarZone(STOFFInputStreamPtr inputStream, std::string const &ascName, std::string const &zoneName) :
  m_input(inputStream), m_ascii(inputStream), m_version(0), m_documentVersion(0), m_encoding(0), m_asciiName(ascName), m_zoneName(zoneName),
  m_typeStack(), m_positionStack(), m_beginToEndMap(), m_flagEndZone(), m_poolList()
{
}

StarZone::~StarZone()
{
  m_ascii.reset();
}

bool StarZone::readString(librevenge::RVNGString &string, int /*encoding*/) const
{
  int sSz=(int) m_input->readULong(2);
  string="";
  if (!m_input->checkPosition(m_input->tell()+sSz)) {
    STOFF_DEBUG_MSG(("StarZone::readString: the sSz seems bad\n"));
    return false;
  }
  // fixme use encoding
  for (int c=0; c<sSz; ++c) string.append((char) m_input->readULong(1));
  return true;
}

bool StarZone::readStringsPool()
{
  long pos=m_input->tell();
  char type;
  if (m_input->peek()!='!' || !openRecord(type)) {
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWPoolList)[" << getRecordLevel() << "]:";
  // sw_sw3misc.xx: InStringPool sw_sw3imp.cxx: LoadOld and Load
  int encoding=m_encoding;
  m_poolList.clear();
  long lastPos=getRecordLastPosition();
  if (!isCompatibleWith(3)) {
    int n=(int) m_input->readULong(2);
    if (n>=256) {
      m_input->seek(-1, librevenge::RVNG_SEEK_CUR);
      encoding=(int) m_input->readULong(1);
      f << "encoding=" << encoding << ",";
      n=(int) m_input->readULong(2);
    }
    f << "n=" << n << ",";
    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());
    librevenge::RVNGString string;
    for (int i=0; i<n; ++i) {
      pos=m_input->tell();
      f.str("");
      f << "SWPoolList-" << i << ":";
      if (!readString(string, encoding) || m_input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarZone::readStringsPool: can not read a string\n"));
        m_input->seek(pos, librevenge::RVNG_SEEK_SET);
        break;
      }
      m_poolList.push_back(string);
      f << string.cstr() << ",";
      m_ascii.addPos(pos);
      m_ascii.addNote(f.str().c_str());
    }
  }
  else {
    encoding=(int) m_input->readULong(1);
    f << "encoding=" << encoding << ",";
    int n=(int) m_input->readULong(2);
    f << "n=" << n << ",";
    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());

    librevenge::RVNGString string;
    for (int i=0; i<n; ++i) { // checkme
      pos=m_input->tell();
      f.str("");
      f << "SWPoolList-" << i << ":";
      int nId=(int) m_input->readULong(2);
      f << "nId=" << nId << ",";
      if (!readString(string, encoding)) {
        STOFF_DEBUG_MSG(("StarZone::readStringsPool: can not read a string\n"));
        m_input->seek(pos, librevenge::RVNG_SEEK_SET);
        break;
      }
      m_poolList.push_back(string);
      f << string.cstr() << ",";
      m_ascii.addPos(pos);
      m_ascii.addNote(f.str().c_str());
    }
  }
  closeRecord(type, "SWPoolList");
  m_ascii.addPos(pos);
  m_ascii.addNote(f.str().c_str());
  return true;
}

bool StarZone::readSWHeader()
{
  m_ascii.open(m_asciiName);

  libstoff::DebugStream f;
  f << "Entries(" << m_zoneName << "):";

  // sw_sw3doc.cxx: Sw3IoImp::InHeader
  if (m_input->size()<0x36) {
    STOFF_DEBUG_MSG(("StarZone::readSWHeader: the zone is too short\n"));
    f << "###";
    m_ascii.addPos(0);
    m_ascii.addNote(f.str().c_str());

    return false;
  }
  m_input->seek(0, librevenge::RVNG_SEEK_SET);

  int val=(int) m_input->readULong(2);
  if (val==0x5357)
    m_input->setReadInverted(!m_input->readInverted());
  else if (val!=0x5753) {
    STOFF_DEBUG_MSG(("StarZone::readSWHeader: can not set the endian\n"));
    f << "###";
    m_ascii.addPos(0);
    m_ascii.addNote(f.str().c_str());

    return false;
  }

  m_input->seek(0, librevenge::RVNG_SEEK_SET);
  for (int i=0; i<7; ++i) {
    char c=(char) m_input->readULong(1);
    static char const(expected[])= {'S','W',(char)0,'H', 'D', 'R', (char)0};
    if (c==expected[i]) continue;
    if (i!=2) {
      STOFF_DEBUG_MSG(("StarZone::readSWHeader: can not read the header\n"));
      f << "###";
      m_ascii.addPos(0);
      m_ascii.addNote(f.str().c_str());

      return false;
    }
    if (c<'3' || c>'5') {
      STOFF_DEBUG_MSG(("StarZone::readZoneHeader: find unexpected version number\n"));
      f << "##version=" << (int) c << ",";
    }
    else {
      m_version=int(c-'0');
      f << "version=" << m_version << ",";
    }
  }
  int hSz=(int) m_input->readULong(1);
  if (hSz<0x2e || !m_input->checkPosition(8+hSz)) {
    STOFF_DEBUG_MSG(("StarZone::readSWHeader: the header seems bad\n"));
    f << "###hSz";
    m_ascii.addPos(0);
    m_ascii.addNote(f.str().c_str());

    return false;
  }
  m_documentVersion=(int) m_input->readULong(2);
  f << "docVersion=" << std::hex << m_documentVersion << std::dec << ",";
  int fFlags=(int) m_input->readULong(2);
  if (fFlags&2) f << "hasBlockName,";
  if (fFlags&8) f << "hasPasswd,";
  if (fFlags&0x100) f << "hasPGNums,";
  if (fFlags&0x8000) {
    f << "#badFile,";
    STOFF_DEBUG_MSG(("StarZone::readSWHeader: bad file is set\n"));

    m_ascii.addPos(0);
    m_ascii.addNote(f.str().c_str());

    return false;
  }
  fFlags&=0x7EF5;
  if (fFlags) f << "flags=" << std::hex << fFlags << std::dec << ",";
  long dFlags=(long) m_input->readULong(4);
  if (dFlags&1) f << "browse[mode],";
  if (dFlags&2) f << "browse[mode2],";
  if (dFlags&4) f << "html[mode],";
  if (dFlags&8) f << "show[header],";
  if (dFlags&0x10) f << "show[footer],";
  if (dFlags&0x20) f << "global[doc],";
  if (dFlags&0x40) f << "hasSections,";
  if (dFlags&0x80) f << "isLabel,";
  dFlags&=0xffffff00;
  if (dFlags) f << "dFlags=" << std::hex << dFlags << std::dec << ",";
  long recPos=(int) m_input->readULong(4);
  if (recPos) f << "recPos=" << std::hex << val << std::dec << ",";
  m_input->seek(6, librevenge::RVNG_SEEK_CUR); // dummy
  val=(int) m_input->readULong(1);
  if (val&1)
    f << "redline[on],";
  if (val&2)
    f << "redline[ignore],";
  if (val&0x10)
    f << "redline[showInsert],";
  if (val&0x20)
    f << "redline[showDelete],";
  val&=0xCC;
  if (val) f << "redline=" << std::hex << val << std::dec << ",";
  val=(int) m_input->readULong(1);
  if (val) f << "compVers=" << val << ",";
  m_input->seek(16, librevenge::RVNG_SEEK_CUR); // checkme
  m_encoding=(int) m_input->readULong(1);
  if (val) f << "charSet[encoding]=" << m_encoding << ",";
  val=(int) m_input->readULong(1);
  if (val) f << "f0=" << val << ",";
  f << "date=" << m_input->readULong(4) << ",";
  f << "time=" << m_input->readULong(4) << ",";
  if (hSz==0x2e +64 && (fFlags&2)) {
    std::string string("");
    for (int i=0; i<64; ++i) {
      char c=(char) m_input->readULong(1);
      if (!c) break;
      string+=c;
    }
    f << string;
    m_input->seek(8+0x2e +64, librevenge::RVNG_SEEK_SET);
  }
  if (m_input->tell()!=8+hSz) {
    m_ascii.addDelimiter(m_input->tell(),'|');
    STOFF_DEBUG_MSG(("StarZone::readZoneHeader: find extra data\n"));
    f << "###extra";
  }
  m_ascii.addPos(0);
  m_ascii.addNote(f.str().c_str());
  m_input->seek(8+hSz, librevenge::RVNG_SEEK_SET);
  if (recPos && isCompatibleWith(int('%')))
    return readRecordSizes(recPos);

  return true;
}

////////////////////////////////////////////////////////////
// record: open/close, read size
////////////////////////////////////////////////////////////
bool StarZone::openRecord(char &type)
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos)) return false;
  unsigned long val=m_input->readULong(4);
  type=(char)(val&0xff);
  if (!type) {
    STOFF_DEBUG_MSG(("StarZone::openRecord: type can not be null\n"));
    return false;
  }
  unsigned long sz=(val>>8);
  long endPos=0;

  m_flagEndZone=0;
  if (sz==0xffffff && isCompatibleWith(0x0209)) {
    if (m_beginToEndMap.find(pos)!=m_beginToEndMap.end())
      endPos=m_beginToEndMap.find(pos)->second;
    else {
      STOFF_DEBUG_MSG(("StarZone::openRecord: can not find size for a zone, we may have some problem\n"));
    }
  }
  else {
    if (sz<4) {
      STOFF_DEBUG_MSG(("StarZone::openRecord: size can be less than 4\n"));
      return false;
    }
    endPos=pos+(long)sz;
  }
  // check the position is in the file
  if (endPos && !m_input->checkPosition(endPos)) {
    STOFF_DEBUG_MSG(("StarZone::openRecord: endPosition is bad\n"));
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openRecord: argh endPosition is not in the current group\n"));
    return false;
  }
  m_typeStack.push(type);
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::closeRecord(char type, std::string const &debugName)
{
  m_flagEndZone=0;
  while (!m_typeStack.empty()) {
    char typ=m_typeStack.top();
    long pos=m_positionStack.top();

    m_typeStack.pop();
    m_positionStack.pop();
    if (typ!=type) continue;
    if (!pos)
      return true;
    long actPos=m_input->tell();
    if (actPos!=pos) {
      if (actPos>pos)
        STOFF_DEBUG_MSG(("StarZone::closeRecord: oops, we read to much data\n"));
      else if (actPos<pos)
        STOFF_DEBUG_MSG(("StarZone::closeRecord: oops, some data have been ignored\n"));
      libstoff::DebugStream f;
      f << debugName << ":###extra";
      m_ascii.addPos(actPos);
      m_ascii.addNote(f.str().c_str());
    }

    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  STOFF_DEBUG_MSG(("StarZone::closeRecord: oops, can not find type %d\n", (int) type));
  return false;
}

unsigned char StarZone::openFlagZone()
{
  unsigned char cFlags=(unsigned char) m_input->readULong(1);
  m_flagEndZone=m_input->tell()+long(cFlags&0xf);
  return cFlags;
}

void StarZone::closeFlagZone()
{
  if (!m_flagEndZone) {
    STOFF_DEBUG_MSG(("StarZone::closeFlagZone: oops, can not find end position\n"));
    return;
  }
  if (m_flagEndZone<m_input->tell()) {
    STOFF_DEBUG_MSG(("StarZone::closeFlagZone: oops, we have read too much data\n"));
    m_ascii.addPos(m_input->tell());
    m_ascii.addNote("Entries(BadFlagZone):###");
  }
  else if (m_flagEndZone>m_input->tell()) {
    STOFF_DEBUG_MSG(("StarZone::closeFlagZone: oops, we do not have read all data\n"));
    m_ascii.addPos(m_input->tell());
    m_ascii.addNote("Entries(BadFlagZone):#");
  }
  m_input->seek(m_flagEndZone, librevenge::RVNG_SEEK_SET);
}

bool StarZone::readRecordSizes(long pos)
{
  if (!pos || !isCompatibleWith('%'))
    return true;
  // read the position:  sw_sw3imp.cxx: Sw3IoImp::InRecSizes
  long oldPos=m_input->tell();
  if (oldPos!=pos)
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << m_zoneName << "[RecSize]:";
  char type;
  bool ok=openRecord(type);
  if (!ok || type!='%') {
    STOFF_DEBUG_MSG(("StarZone::readRecordSizes: can not open the record(recsize)\n"));
    f << "###extra";
    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());

    m_input->seek(oldPos, librevenge::RVNG_SEEK_SET);
    return ok || (oldPos!=pos);
  }

  openFlagZone();
  int nCount=(int) m_input->readULong(4);
  f << "N=" << nCount << ",";
  closeFlagZone();

  if (!m_input->checkPosition(m_input->tell()+8*nCount)) {
    STOFF_DEBUG_MSG(("StarZone::readRecordSizes: endCPos seems bad\n"));
    f << "###badN,";

    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());
    closeRecord('%',m_zoneName);
    if (oldPos!=pos)
      m_input->seek(oldPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  f << "pos:size=[";
  for (int i=0; i<nCount; ++i) {
    long cPos=(long) m_input->readULong(4);
    long sz=(long) m_input->readULong(4);
    m_beginToEndMap[cPos]=cPos+sz;
    f << std::hex << cPos << "<->" << cPos+sz << std::dec << ",";
  }
  f << "],";

  closeRecord('%',m_zoneName);
  if (oldPos!=pos)
    m_input->seek(oldPos, librevenge::RVNG_SEEK_SET);

  m_ascii.addPos(pos);
  m_ascii.addNote(f.str().c_str());
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
