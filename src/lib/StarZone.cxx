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

#include "StarEncryption.hxx"

#include "StarZone.hxx"

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarZone::StarZone(STOFFInputStreamPtr const &inputStream, std::string const &ascName, std::string const &zoneName, char const *password)
  : m_input(inputStream)
  , m_ascii(inputStream)
  , m_version(0)
  , m_documentVersion(0)
  , m_headerVersionStack()
  , m_encoding(StarEncoding::E_DONTKNOW)
  , m_guiType(0)
  , m_encryption()
  , m_asciiName(ascName)
  , m_zoneName(zoneName)
  , m_typeStack()
  , m_positionStack()
  , m_beginToEndMap()
  , m_flagEndZone()
  , m_poolList()
{
  if (password)
    m_encryption.reset(new StarEncryption(password));
}

StarZone::~StarZone()
{
  m_ascii.reset();
}

void StarZone::setInput(STOFFInputStreamPtr ip)
{
  m_input=ip;
  m_ascii.setStream(ip);
}

bool StarZone::readString(std::vector<uint32_t> &string, std::vector<size_t> &srcPositions, int encoding, bool chckEncryption) const
{
  auto sSz=int(m_input->readULong(2));
  string.clear();
  srcPositions.clear();
  if (!sSz) return true;
  unsigned long numRead;
  uint8_t const *data=m_input->read(size_t(sSz), numRead);
  if (!data || numRead!=static_cast<unsigned long>(sSz)) {
    STOFF_DEBUG_MSG(("StarZone::readString: the sSz seems bad\n"));
    return false;
  }
  std::vector<uint8_t> buffer;
  buffer.resize(size_t(sSz));
  std::memcpy(&buffer[0], data, size_t(sSz));
  if (chckEncryption && m_encryption)
    m_encryption->decode(buffer);
  auto encod=m_encoding;
  if (encoding>=1) encod=StarEncoding::getEncodingForId(encoding);
  return StarEncoding::convert(buffer, encod, string, srcPositions);
}

bool StarZone::readStringsPool()
{
  long pos=m_input->tell();
  unsigned char type;
  if (m_input->peek()!='!' || !openSWRecord(type)) {
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
    auto n=int(m_input->readULong(2));
    if (n>=256) {
      m_input->seek(-1, librevenge::RVNG_SEEK_CUR);
      encoding=int(m_input->readULong(1));
      f << "encoding=" << encoding << ",";
      n=int(m_input->readULong(2));
    }
    f << "n=" << n << ",";
    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());
    std::vector<uint32_t> string;
    for (int i=0; i<n; ++i) {
      pos=m_input->tell();
      f.str("");
      f << "SWPoolList-" << i << ":";
      if (!readString(string, encoding) || m_input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarZone::readStringsPool: can not read a string\n"));
        m_input->seek(pos, librevenge::RVNG_SEEK_SET);
        break;
      }
      m_poolList.push_back(libstoff::getString(string));
      f << m_poolList.back().cstr() << ",";
      m_ascii.addPos(pos);
      m_ascii.addNote(f.str().c_str());
    }
  }
  else {
    encoding=int(m_input->readULong(1));
    f << "encoding=" << encoding << ",";
    auto n=int(m_input->readULong(2));
    f << "n=" << n << ",";
    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());

    std::vector<uint32_t> string;
    for (int i=0; i<n; ++i) { // checkme
      pos=m_input->tell();
      f.str("");
      f << "SWPoolList-" << i << ":";
      auto nId=int(m_input->readULong(2));
      f << "nId=" << nId << ",";
      if (!readString(string, encoding) || m_input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarZone::readStringsPool: can not read a string\n"));
        m_input->seek(pos, librevenge::RVNG_SEEK_SET);
        break;
      }
      m_poolList.push_back(libstoff::getString(string));
      f << m_poolList.back().cstr() << ",";
      m_ascii.addPos(pos);
      m_ascii.addNote(f.str().c_str());
    }
  }
  closeSWRecord(type, "SWPoolList");
  return true;
}

bool StarZone::checkEncryption(uint32_t date, uint32_t time, std::vector<uint8_t> const &passwd)
{
  if ((!date && !time) || passwd.empty())
    return true;
  if (m_encryption && m_encryption->checkPassword(date,time,passwd))
    return true;
  if (!m_encryption) m_encryption.reset(new StarEncryption);

  if (!m_encryption->guessPassword(date,time,passwd) || !m_encryption->checkPassword(date,time,passwd)) {
    STOFF_DEBUG_MSG(("StarZone::readZoneHeader: can not find the password\n"));
    throw libstoff::WrongPasswordException();
  }
  STOFF_DEBUG_MSG(("StarZone::readZoneHeader: find a potential password, let continue\n"));
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

  auto val=int(m_input->readULong(2));
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
    auto c=char(m_input->readULong(1));
    static char const expected[]= {'S','W',char(0),'H', 'D', 'R', char(0)};
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
      f << "##version=" << int(c) << ",";
    }
    else {
      m_version=int(c-'0');
      f << "version=" << m_version << ",";
    }
  }
  auto hSz=int(m_input->readULong(1));
  if (hSz<0x2e || !m_input->checkPosition(8+hSz)) {
    STOFF_DEBUG_MSG(("StarZone::readSWHeader: the header seems bad\n"));
    f << "###hSz";
    m_ascii.addPos(0);
    m_ascii.addNote(f.str().c_str());

    return false;
  }
  m_documentVersion=int(m_input->readULong(2));
  f << "docVersion=" << std::hex << m_documentVersion << std::dec << ",";
  auto fFlags=int(m_input->readULong(2));
  bool hasPasswd=false;
  if (fFlags&2) f << "hasBlockName,";
  if (fFlags&8) {
    hasPasswd=true;
    f << "hasPasswd,";
  }
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
  auto dFlags=long(m_input->readULong(4));
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
  long recPos=int(m_input->readULong(4));
  if (recPos) f << "recPos=" << std::hex << val << std::dec << ",";
  m_input->seek(6, librevenge::RVNG_SEEK_CUR); // dummy
  val=int(m_input->readULong(1));
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
  val=int(m_input->readULong(1));
  if (val) f << "compVers=" << val << ",";
  std::vector<uint8_t> passwd;
  for (int i=0; i<16; ++i) passwd.push_back(static_cast<uint8_t>(m_input->readULong(1)));
  val=int(m_input->readULong(1));
  if (val) f << "charSet[encoding]=" << val << ",";
  m_encoding=StarEncoding::getEncodingForId(val);
  val=int(m_input->readULong(1));
  if (val) f << "f0=" << val << ",";
  uint32_t date, time;
  *m_input >> date >> time;
  f << "date=" << date << ",";
  f << "time=" << time << ",";
  if (hasPasswd)
    checkEncryption(date,time,passwd);
  else
    m_encryption.reset();
  if (hSz==0x2e +64 && (fFlags&2)) {
    std::string string("");
    for (int i=0; i<64; ++i) {
      auto c=char(m_input->readULong(1));
      if (!c) break;
      string+=c;
    }
    f << string;
    m_input->seek(8+0x2e +64, librevenge::RVNG_SEEK_SET);
  }
  if (m_input->tell()!=8+hSz) {
    m_ascii.addDelimiter(m_input->tell(),'|');
    STOFF_DEBUG_MSG(("StarZone::readSWHeader: find extra data\n"));
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
// sdrheader: open/close
////////////////////////////////////////////////////////////
bool StarZone::openSCHHeader()
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+6)) return false;
  // schiocmp.cxx: SchIOHeader::SchIOHeader
  auto len=long(m_input->readULong(4));
  m_headerVersionStack.push(int(m_input->readULong(2)));
  long endPos=pos+len;
  if (len<6 || !m_input->checkPosition(endPos)) {
    m_headerVersionStack.pop();
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openSCHHeader: argh endPosition is not in the current group\n"));
    m_headerVersionStack.pop();
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  m_typeStack.push('@');
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::closeSCHHeader(std::string const &debugName)
{
  if (!m_headerVersionStack.empty()) m_headerVersionStack.pop();
  return closeRecord('@', debugName);
}

bool StarZone::openVersionCompatHeader()
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+6)) return false;
  // vcompat.cxx: VersionCompat::VersionCompat
  m_headerVersionStack.push(int(m_input->readULong(2)));
  auto len=long(m_input->readULong(4));
  long endPos=pos+6+len;
  if (len<0 || !m_input->checkPosition(endPos)) {
    m_headerVersionStack.pop();
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openVersionCompatHeader: argh endPosition is not in the current group\n"));
    m_headerVersionStack.pop();
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  m_typeStack.push('*');
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::closeVersionCompatHeader(std::string const &debugName)
{
  if (!m_headerVersionStack.empty()) m_headerVersionStack.pop();
  return closeRecord('*', debugName);
}

bool StarZone::openSDRHeader(std::string &magic)
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+4)) return false;
  // svdio.cxx: SdrIOHeader::Read
  magic="";
  for (int i=0; i<4; ++i) magic+=char(m_input->readULong(1));
  // special case: ok to have only magic if ...
  if (magic=="DrXX") {
    m_typeStack.push('_');
    m_positionStack.push(m_input->tell());
    return true;
  }
  m_headerVersionStack.push(int(m_input->readULong(2)));
  auto len=long(m_input->readULong(4));
  long endPos=pos+len;
  if (len<10 || magic.compare(0,2,"Dr")!=0 || !m_input->checkPosition(endPos)) {
    m_headerVersionStack.pop();
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openSDRHeader: argh endPosition is not in the current group\n"));
    m_headerVersionStack.pop();
    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  m_typeStack.push('_');
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::closeSDRHeader(std::string const &debugName)
{
  if (!m_headerVersionStack.empty()) m_headerVersionStack.pop();
  return closeRecord('_', debugName);
}

////////////////////////////////////////////////////////////
// record: open/close, read size
////////////////////////////////////////////////////////////
bool StarZone::openDummyRecord()
{
  m_typeStack.push('@');
  if (!m_positionStack.empty())
    m_positionStack.push(m_positionStack.top());
  else
    m_positionStack.push(m_input->size());
  return true;
}

bool StarZone::openRecord()
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+4)) return false;
  unsigned long sz=m_input->readULong(4);
  long endPos=0;

  m_flagEndZone=0;
  if (sz<4) {
    STOFF_DEBUG_MSG(("StarZone::openRecord: size can be less than 4\n"));
    return false;
  }
  endPos=pos+long(sz);
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
  m_typeStack.push(' ');
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::openSCRecord()
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+4)) return false;
  unsigned long sz=m_input->readULong(4);
  long endPos=0;

  m_flagEndZone=0;
  endPos=pos+4+long(sz);
  // check the position is in the file
  if (endPos && !m_input->checkPosition(endPos)) {
    STOFF_DEBUG_MSG(("StarZone::openSCRecord: endPosition is bad\n"));
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openSCRecord: argh endPosition is not in the current group\n"));
    return false;
  }
  m_typeStack.push('_');
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::openSWRecord(unsigned char &type)
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+4)) return false;
  unsigned long val=m_input->readULong(4);
  type=static_cast<unsigned char>(val&0xff);
  if (!type) {
    STOFF_DEBUG_MSG(("StarZone::openSWRecord: type can not be null\n"));
    return false;
  }
  unsigned long sz=(val>>8);
  long endPos=0;

  m_flagEndZone=0;
  if (sz==0xffffff && isCompatibleWith(0x0209)) {
    if (m_beginToEndMap.find(pos)!=m_beginToEndMap.end())
      endPos=m_beginToEndMap.find(pos)->second;
    else {
      STOFF_DEBUG_MSG(("StarZone::openSWRecord: can not find size for a zone, we may have some problem\n"));
    }
  }
  else {
    if (sz<4) {
      STOFF_DEBUG_MSG(("StarZone::openSWRecord: size can be less than 4\n"));
      return false;
    }
    endPos=pos+long(sz);
  }
  // check the position is in the file
  if (endPos && !m_input->checkPosition(endPos)) {
    STOFF_DEBUG_MSG(("StarZone::openSWRecord: endPosition is bad\n"));
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openSWRecord: argh endPosition is not in the current group\n"));
    return false;
  }
  m_typeStack.push(type);
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::openSfxRecord(unsigned char &type)
{
  long pos=m_input->tell();
  if (!m_input->checkPosition(pos+4)) return false;
  // filerec.cxx SfxMiniRecordReader::SfxMiniRecordReader
  unsigned long val=m_input->readULong(4);
  type=static_cast<unsigned char>(val&0xff);
  // checkme: can type be null
  unsigned long sz=(val>>8);
  long endPos=0;

  m_flagEndZone=0;
  endPos=pos+4+long(sz);
  // check the position is in the file
  if (endPos && !m_input->checkPosition(endPos)) {
    STOFF_DEBUG_MSG(("StarZone::openSfxRecord: endPosition is bad\n"));
    return false;
  }
  // check the position ends in the current group (if a group is open)
  if (!m_positionStack.empty() && endPos>m_positionStack.top() && m_positionStack.top()) {
    STOFF_DEBUG_MSG(("StarZone::openSfxRecord: argh endPosition is not in the current group\n"));
    return false;
  }
  m_typeStack.push(type);
  m_positionStack.push(endPos);
  return true;
}

bool StarZone::closeRecord(unsigned char type, std::string const &debugName)
{
  m_flagEndZone=0;
  while (!m_typeStack.empty()) {
    unsigned char typ=m_typeStack.top();
    long pos=m_positionStack.top();

    m_typeStack.pop();
    m_positionStack.pop();
    if (typ!=type) continue;
    if (!pos || type=='@')
      return true;
    long actPos=m_input->tell();
    if (actPos!=pos) {
      if (actPos>pos) {
        STOFF_DEBUG_MSG(("StarZone::closeRecord: oops, we read to much data\n"));
      }
      else if (actPos<pos) {
        STOFF_DEBUG_MSG(("StarZone::closeRecord: oops, some data have been ignored\n"));
      }
      libstoff::DebugStream f;
      f << debugName << ":###extra";
      m_ascii.addPos(actPos);
      m_ascii.addNote(f.str().c_str());
    }

    m_input->seek(pos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  STOFF_DEBUG_MSG(("StarZone::closeRecord: oops, can not find type %d\n", int(type)));
  return false;
}

unsigned char StarZone::openFlagZone()
{
  auto cFlags=static_cast<unsigned char>(m_input->readULong(1));
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
  unsigned char type;
  bool ok=openSWRecord(type);
  if (!ok || type!='%') {
    STOFF_DEBUG_MSG(("StarZone::readRecordSizes: can not open the record(recsize)\n"));
    f << "###extra";
    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());

    m_input->seek(oldPos, librevenge::RVNG_SEEK_SET);
    return ok || (oldPos!=pos);
  }

  openFlagZone();
  auto nCount=int(m_input->readULong(4));
  f << "N=" << nCount << ",";
  closeFlagZone();

  if (nCount<0 || (getRecordLastPosition()-m_input->tell())/8<nCount || !m_input->checkPosition(m_input->tell()+8*nCount)) {
    STOFF_DEBUG_MSG(("StarZone::readRecordSizes: endCPos seems bad\n"));
    f << "###badN,";

    m_ascii.addPos(pos);
    m_ascii.addNote(f.str().c_str());
    closeSWRecord('%',m_zoneName);
    if (oldPos!=pos)
      m_input->seek(oldPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  f << "pos:size=[";
  for (int i=0; i<nCount; ++i) {
    auto cPos=long(m_input->readULong(4));
    auto sz=long(m_input->readULong(4));
    m_beginToEndMap[cPos]=cPos+sz;
    f << std::hex << cPos << "<->" << cPos+sz << std::dec << ",";
  }
  f << "],";

  closeSWRecord('%',m_zoneName);
  if (oldPos!=pos)
    m_input->seek(oldPos, librevenge::RVNG_SEEK_SET);

  m_ascii.addPos(pos);
  m_ascii.addNote(f.str().c_str());
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
