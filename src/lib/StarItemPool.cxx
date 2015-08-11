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

#include "StarFileManager.hxx"
#include "StarZone.hxx"

#include "StarItemPool.hxx"

/** Internal: the structures of a StarItemPool */
namespace StarItemPoolInternal
{
////////////////////////////////////////
//! Internal: a structure use to read SfxMultiRecord zone of a StarItemPool
struct SfxMultiRecord {
  //! constructor
  SfxMultiRecord() : m_zone(0), m_zoneType(0), m_zoneOpened(false), m_headerType(0), m_headerVersion(0), m_headerTag(0),
    m_actualRecord(0), m_numRecord(0), m_contentSize(0),
    m_startPos(0), m_endPos(0), m_offsetList(), m_extra("")
  {
  }
  //! returns true if the record is opened
  bool isOpened() const
  {
    return m_zoneOpened;
  }
  //! returns the number of record
  uint16_t getNumRecords() const
  {
    return m_zoneOpened ? m_numRecord : 0;
  }
  //! returns the header tag or -1
  int getHeaderTag() const
  {
    return !m_zoneOpened ? -1 : int(m_headerTag);
  }
  //! try to open a zone
  bool open(StarZone &zone)
  {
    if (m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord: oops a record has been opened\n"));
      return false;
    }
    m_actualRecord=m_numRecord=0;
    m_headerType=m_headerVersion=0;
    m_headerTag=0;
    m_contentSize=0;
    m_offsetList.clear();
    m_zone=&zone;
    STOFFInputStreamPtr input=m_zone->input();
    long pos=input->tell();
    if (!m_zone->openSfxRecord(m_zoneType)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    if (m_zoneType==char(0xff)) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord: oops end header\n"));
      m_extra="###emptyZone,";
      return true; /* empty zone*/
    }
    if (m_zoneType!=0) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord: find unknown header\n"));
      m_extra="###badZoneType,";
      return true;
    }

    m_zoneOpened=true;
    m_endPos=m_zone->getRecordLastPosition();
    // filerec.cxx: SfxSingleRecordReader::FindHeader_Impl
    if (input->tell()+10>m_endPos) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::open: oops the zone seems too short\n"));
      m_extra="###zoneShort,";
      return true;
    }
    *input >> m_headerType >> m_headerVersion >> m_headerTag;
    // filerec.cxx: SfxMultiRecordReader::ReadHeader_Impl
    *input >> m_numRecord >> m_contentSize;
    m_startPos=input->tell();
    std::stringstream s;
    if (m_headerType==2) {
      // fixed size
      if (m_startPos+m_numRecord*m_contentSize > m_endPos) {
        STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::open: oops the number of record seems bad\n"));
        s << "##numRecord=" << m_numRecord << ",";
        if (m_contentSize && m_endPos>m_startPos)
          m_numRecord=uint16_t((m_endPos-m_startPos)/m_contentSize);
        else
          m_numRecord=0;
      }
      m_extra=s.str();
      return true;
    }

    long debOffsetList=((m_headerType==3 || m_headerType==7) ? m_startPos : 0) + m_contentSize;
    if (debOffsetList<m_startPos || debOffsetList+4*m_numRecord > m_endPos) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::open: can not find the version map offset\n"));
      s << "###contentCount";
      m_numRecord=0;
      m_extra=s.str();
      return true;
    }
    m_endPos=debOffsetList;
    input->seek(debOffsetList, librevenge::RVNG_SEEK_SET);
    for (uint16_t i=0; i<m_numRecord; ++i) {
      uint32_t offset;
      *input >> offset;
      m_offsetList.push_back(offset);
    }
    input->seek(m_startPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  //! try to close a zone
  void close(std::string const &wh)
  {
    if (!m_zone) return;
    if (!m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::close: can not find any opened zone\n"));
      return;
    }
    m_zoneOpened=false;
    STOFFInputStreamPtr input=m_zone->input();
    if (input->tell()<m_endPos && input->tell()+4>=m_endPos) { // small diff is possible
      m_zone->ascii().addDelimiter(input->tell(),'|');
      input->seek(m_zone->getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()==m_endPos)
      input->seek(m_zone->getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    m_zone->closeSfxRecord(m_zoneType, wh);
    m_zone=0;
  }
  //! try to go to the new content positon
  bool getNewContent(std::string const &wh)
  {
    if (!m_zone) return false;
    // SfxMultiRecordReader::GetContent
    long newPos=getLastContentPosition();
    if (newPos>=m_endPos) return false;
    STOFFInputStreamPtr input=m_zone->input();
    ++m_actualRecord;
    if (input->tell()<newPos && input->tell()+4>=newPos) { // small diff is possible
      m_zone->ascii().addDelimiter(input->tell(),'|');
      input->seek(newPos, librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()!=newPos) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::getNewContent: find extra data\n"));
      m_zone->ascii().addPos(input->tell());
      libstoff::DebugStream f;
      f << wh << ":###extra";
      m_zone->ascii().addNote(f.str().c_str());
      input->seek(newPos, librevenge::RVNG_SEEK_SET);
    }
    if (m_headerType==7 || m_headerType==8) {
      // TODO: readtag
      input->seek(2, librevenge::RVNG_SEEK_CUR);
    }
    return true;
  }
  //! returns the last content position
  long getLastContentPosition() const
  {
    if (m_actualRecord >= m_numRecord) return m_endPos;
    if (m_headerType==2) return m_startPos+m_actualRecord*m_contentSize;
    if (m_actualRecord >= uint16_t(m_offsetList.size())) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::getLastContentPosition: argh, find unexpected index\n"));
      return m_endPos;
    }
    return m_startPos+(m_offsetList[size_t(m_actualRecord)]>>8)-14;
  }

  //! basic operator<< ; print header data
  friend std::ostream &operator<<(std::ostream &o, SfxMultiRecord const &r)
  {
    if (!r.m_zoneOpened) {
      o << r.m_extra;
      return o;
    }
    if (r.m_headerType) o << "type=" << int(r.m_headerType) << ",";
    if (r.m_headerVersion) o << "version=" << int(r.m_headerVersion) << ",";
    if (r.m_headerTag) o << "tag=" << r.m_headerTag << ",";
    if (r.m_numRecord) o << "num[record]=" << r.m_numRecord << ",";
    if (r.m_contentSize) o << "content[size/pos]=" << r.m_contentSize << ",";
    if (!r.m_offsetList.empty()) {
      o << "offset=[";
      for (size_t i=0; i<r.m_offsetList.size(); ++i) {
        uint32_t off=r.m_offsetList[i];
        if (off&0xff)
          o << (off>>8) << ":" << (off&0xff) << ",";
        else
          o << (off>>8) << ",";
      }
      o << "],";
    }
    o << r.m_extra;
    return o;
  }
protected:
  //! the main zone
  StarZone *m_zone;
  //! the zone type
  char m_zoneType;
  //! true if a SfxRecord has been opened
  bool m_zoneOpened;
  //! the record type
  uint8_t m_headerType;
  //! the header version
  uint8_t m_headerVersion;
  //! the header tag
  uint16_t m_headerTag;
  //! the actual record
  uint16_t m_actualRecord;
  //! the number of record
  uint16_t m_numRecord;
  //! the record/content/pos size
  uint32_t m_contentSize;
  //! the start of data position
  long m_startPos;
  //! the end of data position
  long m_endPos;
  //! the list of (offset + type)
  std::vector<uint32_t> m_offsetList;
  //! extra data
  std::string m_extra;
private:
  SfxMultiRecord(SfxMultiRecord const &orig);
  SfxMultiRecord &operator=(SfxMultiRecord const &orig);
};

//! small struct used to keep a list of version
struct Version {
  //! constructor
  Version(int vers, int start, std::vector<int> const &list) :
    m_version(vers), m_start(start), m_list(list), m_invertListMap()
  {
    for (size_t i=0; i<m_list.size(); ++i)
      m_invertListMap[list[i]]=int(i);
  }
  //! the version number
  int m_version;
  //! int the start value
  int m_start;
  //! the list of value
  std::vector<int> m_list;
  //! a map offset to which
  std::map<int,int> m_invertListMap;
};

////////////////////////////////////////
//! Internal: the state of a StarItemPool
struct State {
  //! constructor
  State() : m_majorVersion(0), m_minorVersion(0), m_loadingVersion(0), m_name(""),
    m_currentVersion(0), m_verStart(100/*ATTR_STARTINDEX*/), m_verEnd(183/*ATTR_ENDINDEX*/), m_versionList()
  {
  }
  //! returns true if the value is in expected range
  int isInRange(int which) const
  {
    return which>=m_verStart&&which<=m_verEnd;
  }
  //! add a new version map
  void addVersionMap(uint16_t nVers, uint16_t nStart, std::vector<int> const &list)
  {
    // SfxItemPool::SetVersionMap
    m_versionList.push_back(Version(int(nVers), int(nStart), list));
    if (nVers<=m_currentVersion) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::State::addVersionMap: arghh the version are not sorted\n"));
    }
    m_currentVersion=nVers;
    Version const &vers=m_versionList.back();
    if (vers.m_invertListMap.empty()) return;
    int min=vers.m_invertListMap.begin()->first;
    if (m_verStart>min) m_verStart=min;
    int max=(--vers.m_invertListMap.end())->first;
    if (m_verEnd<max) m_verEnd=max;
  }
  //! try to return ???
  int getWhich(int nFileWhich) const
  {
    // polio.cxx: SfxItemPool::GetNewWhich
    if (nFileWhich<m_verStart||nFileWhich>m_verEnd) {
      // to do implement recursif method
      STOFF_DEBUG_MSG(("StarItemPoolInternal::State::GetWhich: recursive method is not implemented\n"));
      return 0;
    }
    if (m_loadingVersion>m_currentVersion) {
      for (size_t i=m_versionList.size(); i>0;) {
        Version const &vers=m_versionList[--i];
        if (vers.m_version<=m_currentVersion)
          break;
        if (vers.m_invertListMap.find(nFileWhich)==vers.m_invertListMap.end())
          return 0;
        nFileWhich=vers.m_start+vers.m_invertListMap.find(nFileWhich)->second;
      }
    }
    else if (m_loadingVersion<m_currentVersion) {
      for (size_t i=0; i<m_versionList.size(); ++i) {
        Version const &vers=m_versionList[i];
        if (vers.m_version<=m_loadingVersion)
          continue;
        if (nFileWhich<vers.m_start || nFileWhich>=vers.m_start+(int) vers.m_list.size()) {
          STOFF_DEBUG_MSG(("StarItemPoolInternal::State::GetWhich: argh nFileWhich is not in good range\n"));
          break;
        }
        else
          nFileWhich=vers.m_list[size_t(nFileWhich-vers.m_start)];
      }
    }
    return nFileWhich;
  }
  //! the majorVersion
  int m_majorVersion;
  //! the minorVersion
  int m_minorVersion;
  //! the loading version
  int m_loadingVersion;
  //! the name
  librevenge::RVNGString m_name;
  //! the current version
  int m_currentVersion;
  //! the minimum version
  int m_verStart;
  //! the maximum version
  int m_verEnd;
  //! the list of version
  std::vector<Version> m_versionList;
private:
  State(State const &orig);
  State operator=(State const &orig);
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarItemPool::StarItemPool() : m_state(new StarItemPoolInternal::State)
{
}

StarItemPool::~StarItemPool()
{
}

int StarItemPool::getVersion() const
{
  return m_state->m_majorVersion;
}

bool StarItemPool::read(StarZone &zone)
{
  // reinit all
  m_state.reset(new StarItemPoolInternal::State);

  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();
  libstoff::DebugStream f;
  f << "Entries(PoolDef)["<< zone.getRecordLevel() << "]:";
  long pos=input->tell();
  if (pos+18>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::read: the zone seems too short\n"));
    return false;
  }
  uint16_t tag;
  // poolio.cxx SfxItemPool::Load
  uint8_t nMajorVers, nMinorVers;
  *input >> tag >> nMajorVers >> nMinorVers;
  if (tag==0x1111)
    f << "v4,";
  else if (tag==0xbbbb)
    f << "v5,";
  else {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  if (nMajorVers==1) {
    m_state->m_majorVersion=1;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return readV1(zone);
  }
  if (nMajorVers!=2) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  *input>>tag;
  if (tag!=0xffff) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  m_state->m_majorVersion=2;
  m_state->m_minorVersion=int(nMinorVers);
  f << "version=2,";
  if (m_state->m_minorVersion) f << "vers[minor]=" << m_state->m_minorVersion << ",";
  input->seek(4, librevenge::RVNG_SEEK_CUR); // 0,0

  char type; // always 1
  if (input->peek()!=1 || !zone.openSfxRecord(type)) {
    STOFF_DEBUG_MSG(("StarItemPool::read: can not open the sfx record\n"));
    m_state->m_majorVersion=0;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  endPos=zone.getRecordLastPosition();
  if (input->tell()==endPos) {
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  // string part
  char type1;
  if (input->peek()!=16 || !zone.openSfxRecord(type1)) {
    STOFF_DEBUG_MSG(("StarItemPool::read: can not open the string sfx record\n"));
    f << "###openString";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  int16_t val;
  *input >> val;
  if (val) {
    f << "loadingVersion=" << val << ",";
    m_state->m_loadingVersion=val;
  }
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarItemPool::read: can not read the name\n"));
    f << "###name";
  }
  if (!string.empty()) {
    m_state->m_name=string;
    f << "name[ext]=" << string.cstr() << ",";
  }
  zone.closeSfxRecord(type1, "PoolDef");

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "PoolDef[versMap]:";
  StarItemPoolInternal::SfxMultiRecord mRecord;
  if (!mRecord.open(zone) || mRecord.getHeaderTag()!=0x20) {
    STOFF_DEBUG_MSG(("StarItemPool::read: can not open the versionMap sfx record\n"));
    f << "###openVersionMap";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    if (mRecord.isOpened()) mRecord.close("PoolDef");
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  f << mRecord;
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  int n=0;
  while (mRecord.getNewContent("PoolDef")) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[versMap-" << n++ << "]:";
    uint16_t nVers, nStart, nEnd;
    *input >> nVers >> nStart >> nEnd;
    f << "vers=" << nVers << "," << nStart << "<->" << nEnd << ",";
    if (nStart>nEnd || input->tell()+2*(nEnd-nStart+1) > mRecord.getLastContentPosition()) {
      STOFF_DEBUG_MSG(("StarItemPool::read: can not find start|end pos\n"));
      f << "###badStartEnd";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    f << "pos=[";
    std::vector<int> listData;
    for (uint16_t i=nStart; i<=nEnd; ++i) {
      uint16_t nPos;
      *input>>nPos;
      listData.push_back(int(nPos));
      f << nPos << ",";
    }
    f << "],";
    if (!listData.empty()) m_state->addVersionMap(nVers,nStart,listData);

    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  mRecord.close("PoolDef");

  for (int step=0; step<2; ++step) {
    std::string wh(step==0 ? "attrib" : "default");
    pos=input->tell();
    f.str("");
    f << "PoolDef[" << wh << "]:";
    if (!mRecord.open(zone)) {
      STOFF_DEBUG_MSG(("StarItemPool::read: can not open the sfx record\n"));
      f << "###" << wh;
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type, "PoolDef");
      return true;
    }
    f << mRecord;

    if (mRecord.getHeaderTag()!=(step==0 ? 0x30:0x50)) {
      STOFF_DEBUG_MSG(("StarItemPool::read: can not find the pool which tag\n"));
      f << "###tag";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      mRecord.close("PoolDef");
      continue;
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    n=0;
    while (mRecord.getNewContent("PoolDef")) {
      pos=input->tell();
      f.str("");
      f << "PoolDef[" << wh << "-" << n++ << "]:";
      uint16_t nWhich, nVersion, nCount=1;
      *input >> nWhich >> nVersion;
      if (step==0) *input >> nCount;
      int which=nWhich;
      if (m_state->m_currentVersion!=m_state->m_loadingVersion) which=m_state->getWhich(which);
      if (!m_state->isInRange(which)) {
        if (which<100)
          f << "##";
        else {
          STOFF_DEBUG_MSG(("StarItemPool::read: the which value seems bad\n"));
          f << "###";
        }
      }
      f << "wh=" << which << ", vers=" << nVersion << ", count=" << nCount << ",";
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarItemPool::read: reading attribute is not implemented\n"));
        first=false;
      }
      f << "##";
      input->seek(mRecord.getLastContentPosition(), librevenge::RVNG_SEEK_SET);
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
    }
    mRecord.close("PoolDef");
  }

  zone.closeSfxRecord(type, "PoolDef");
  return true;
}

bool StarItemPool::readV1(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();
  libstoff::DebugStream f;
  f << "Entries(PoolDef)[" << zone.getRecordLevel() << "]:";
  long pos=input->tell();
  if (pos+18>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: the zone seems too short\n"));
    return false;
  }
  uint16_t tag;
  // poolio.cxx SfxItemPool::Load
  uint8_t nMajorVers, nMinorVers;
  *input >> tag >> nMajorVers >> nMinorVers;
  if (tag==0x1111)
    f << "v4,";
  else if (tag==0xbbbb)
    f << "v5,";
  else {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  if (nMajorVers!=1) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  f << "version=" << int(nMajorVers) << ",vers[minor]=" << int(nMinorVers) << ",";
  if (nMinorVers>=3) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: find a minor version >= 3\n"));
    f << "###";
  }

  // SfxItemPool::Load1_Impl
  if (nMinorVers>=2) {
    int16_t nLoadingVersion;
    *input >> nLoadingVersion;
    if (nLoadingVersion) f << "vers[loading]=" << nLoadingVersion << ",";
  }
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: can not read the name\n"));
    f << "###name";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  if (!string.empty())
    f << "name[ext]=" << string.cstr() << ",";
  uint32_t attribSize;
  *input>>attribSize;
  long attribPos=input->tell();
  if (attribPos+attribSize+10>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: attribSize is bad\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  else if (attribSize) {
    f << "attr[sz]=" << attribSize << ",";
    input->seek(attribSize, librevenge::RVNG_SEEK_CUR);
  }
  else {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: attribSize\n"));
    f << "###attrib[sz]==0,";
  }

  *input >> tag;
  if (tag!=0x3333) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find tag size\n"));
    f << "###tag=" << std::hex << tag << std::dec;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  uint32_t tableSize;
  *input>>tableSize;
  long tablePos=input->tell();
  long beginEndPos=tablePos+tableSize;
  if (beginEndPos+4>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: tableSize is bad\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  else if (tableSize)
    f << "table[sz]=" << tableSize << ",";
  else if (nMinorVers>=3) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: tableSize is null for version 3\n"));
    f << "###table[sz]==0,";
  }
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  long endTablePos=tablePos+tableSize;
  if (nMinorVers>=3 && tableSize>=4) { // CHECKME: never seens
    input->seek(tablePos+tableSize-4, librevenge::RVNG_SEEK_SET);
    pos=(long) input->readULong(4);
    endTablePos-=4;
    if (pos<tablePos || pos>=endTablePos) {
      STOFF_DEBUG_MSG(("StarItemPool::readV1: arrgh can not find versionmap position\n"));
    }
    else {
      long lastVersMapPos=endTablePos;
      endTablePos=pos;
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      f.str("");
      f << "PoolDef[versionMap]:";
      uint16_t nVerCount;
      *input >> nVerCount;
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      for (int i=0; i<int(nVerCount); ++i) {
        pos=input->tell();
        f.str("");
        f << "PoolDef[versionMap" << i << "]:";
        uint16_t nVers, nStart, nEnd;
        *input >> nVers >> nStart >> nEnd;
        f << "vers=" << nVers << "," << nStart << "-" << nEnd << ",";
        int N=int(nEnd)+1-int(nStart);
        if (N<0 || pos+6+2*N>lastVersMapPos) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: arrgh nBytes is bad\n"));
          f << "###";
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
          break;
        }
        f << "val=[";
        for (int j=0; j<N; ++j) {
          uint16_t val;
          *input >> val;
          if (val) f << val << ",";
          else
            f << "_,";
        }
        f << "],";
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
      }
    }
  }

  // now read the table
  input->seek(tablePos, librevenge::RVNG_SEEK_SET);
  pos=input->tell();
  f.str("");
  f << "PoolDef[table]:";
  std::vector<uint32_t> sizeAttr;
  f << "size=[";
  while (input->tell()+4<=endTablePos) {
    uint32_t sz;
    *input>>sz;
    sizeAttr.push_back(sz);
    f << sz << ",";
  }
  f << "],";
  ascii.addPos(tablePos-6);
  ascii.addNote(f.str().c_str());

  // now read the attribute
  input->seek(attribPos, librevenge::RVNG_SEEK_SET);
  pos=input->tell();
  long endDataPos=attribPos+attribSize;
  f.str("");
  f << "PoolDef[attrib]:";
  bool ok=false;
  if (attribSize!=0)  {
    *input >> tag;
    if (tag!=0x2222) {
      STOFF_DEBUG_MSG(("StarItemPool::readV1: tag is bad \n"));
      f << "###tag=" << std::hex << tag << std::dec;
    }
    else
      ok=true;
  }
  ascii.addPos(attribPos-4);
  ascii.addNote(f.str().c_str());
  size_t n=0;
  while (ok) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[attrib" << n << "]:";
    if (pos+2 > endDataPos) {
      ok=false;

      STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find last attrib\n"));
      f << "###noLast,";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    uint16_t nWhich;
    *input >> nWhich;
    if (nWhich==0) {
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }

    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarItemPool::readV1: reading attribute is not implemented\n"));
      first=false;
    }
    uint16_t nSlot, nVersion, nCount;
    *input >> nSlot >> nVersion >> nCount;
    f << "wh=" << nWhich << "[" << std::hex << nSlot << std::dec << "], vers=" << nVersion << ", count=" << nCount << ",";
    f << "sz=[";
    long actPos=input->tell();
    for (int i=0; i<nCount; ++i) {
      if (n >= sizeAttr.size() || actPos+sizeAttr[n]>endDataPos) {
        ok=false;

        STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find attrib size\n"));
        f << "###badSize,";
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
        break;
      }
      if (sizeAttr[n]) {
        ascii.addDelimiter(actPos,'|');
        f << sizeAttr[n] << ",";
        actPos+=sizeAttr[n];
      }
      else
        f << "_,";
      ++n;
    }
    if (!ok) break;
    f << "],";
    input->seek(actPos, librevenge::RVNG_SEEK_SET);
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  if (ok) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[default]:";
    if (nMinorVers>0) {
      *input>>tag;
      if (tag!=0x4444) {
        STOFF_DEBUG_MSG(("StarItemPool::readV1: default tag is bad \n"));
        f << "###tag=" << std::hex << tag << std::dec;
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
        ok=false;
      }
    }
  }
  if (ok) {
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  while (ok) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[default" << n << "]:";
    if (pos+2 > endDataPos) {
      ok=false;

      STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find last attrib\n"));
      f << "###noLast,";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    uint16_t nWhich;
    *input >> nWhich;
    if (nWhich==0) {
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    uint16_t nSlot, nVersion;
    *input >> nSlot >> nVersion;
    f << "wh=" << nWhich << "[" << std::hex << nSlot << std::dec << "], vers=" << nVersion << ",";
    if (n >= sizeAttr.size() || sizeAttr[n]<6 || pos+sizeAttr[n]>endDataPos) {
      ok=false;

      STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find attrib size\n"));
      f << "###badSize,";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    if (sizeAttr[n]>6) ascii.addDelimiter(input->tell(),'|');
    input->seek(pos+sizeAttr[n++], librevenge::RVNG_SEEK_SET);
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  input->seek(beginEndPos, librevenge::RVNG_SEEK_SET);
  pos=input->tell();
  f.str("");
  f << "PoolDef[end]:";
  uint16_t tag1;
  *input >> tag >> tag1;
  if (tag!=0xeeee || tag1!=0xeeee) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find end tag\n"));
    f << "###tag=" << std::hex << tag << std::dec;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  return true;
}

bool StarItemPool::readStyle(StarZone &zone, int poolVersion)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long pos=input->tell();
  libstoff::DebugStream f;
  f << "Entries(SfxStylePool)[" << zone.getRecordLevel() << "]:pool[vers]=" << poolVersion << ",";
  if (poolVersion!=1 && poolVersion!=2) {
    STOFF_DEBUG_MSG(("StarItemPool::readStyle: reading version 1 is not implemented\n"));
    return false;
  }
  char type;
  uint16_t charSet, nCount;

  bool helpIdSize32=true, ok=true;
  StarItemPoolInternal::SfxMultiRecord mRecord;
  if (poolVersion==1) {
    // style.cxx SfxStyleSheetBasePool::Load1_Impl
    *input >> charSet;
    if (charSet==50) {
      f << "v50,";
      *input >> charSet;
    }
    else
      helpIdSize32=false;
    if (charSet) f << "char[set]="<< charSet << ",";
    *input >> nCount;
    f << "n=" << nCount << ",";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  else {
    // style.cxx SfxStyleSheetBasePool::Load(vers==2)
    if (input->peek()!=3 || !zone.openSfxRecord(type)) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not open the first zone\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }

    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SfxStylePool[header]:";
    char type1;
    if (!zone.openSfxRecord(type1)) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not open the header zone\n"));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type, "SfxStylePool");
      return false;
    }
    uint8_t headerType, headerVersion;
    uint16_t headerTag;
    *input >> headerType >> headerVersion >> headerTag;
    if (headerTag!=0x10) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not find header tag\n"));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type1, "SfxStylePool");
      zone.closeSfxRecord(type, "SfxStylePool");
    }
    if (headerVersion) f << "vers=" << int(headerVersion) << ",";
    if (headerType) f << "type=" << int(headerType) << ",";
    *input >> charSet;
    if (charSet) f << "char[set]="<< charSet << ",";
    zone.closeSfxRecord(type1, "SfxStylePool");
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SfxStylePool[styles]:";
    if (!mRecord.open(zone)) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not open the versionMap sfx record\n"));
      f << "###openVersionMap";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type, "PoolDef");
      return true;
    }
    f << mRecord;

    if (mRecord.getHeaderTag()!=0x20) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not find the version map tag\n"));
      f << "###tag";
      ok=false;
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    nCount=mRecord.getNumRecords();
  }
  long lastPos=zone.getRecordLevel() ? zone.getRecordLastPosition() : input->size();
  for (int i=0; ok && i<int(nCount); ++i) {
    pos=input->tell();
    if (poolVersion==2) {
      if (!mRecord.getNewContent("SfxStylePool"))
        break;
      lastPos=mRecord.getLastContentPosition();
    }
    f.str("");
    f << "SfxStylePool[data" << i << "]:";
    bool readOk=true;
    librevenge::RVNGString text;
    for (int j=0; j<3; ++j) {
      if (!zone.readString(text) || input->tell()>=lastPos) {
        STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not find a string\n"));
        f << "###string";
        readOk=false;
        break;
      }
      if (text.empty()) continue;
      static char const *(wh[])= {"name","parent","follow"};
      f << wh[j] << "=" << text.cstr() << ",";
    }
    if (!readOk) {
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    uint16_t nFamily, nMask, nItemCount, nVer;
    *input >> nFamily >> nMask;
    if (nFamily) f << "family=" << nFamily << ",";
    if (nMask) f << "mask=" << nMask << ",";
    if (!zone.readString(text) || input->tell()>=lastPos) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: can not find helpFile\n"));
      f << "###helpFile";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    if (!text.empty()) f << "help[file]=" << text.cstr() << ",";
    uint32_t nHelpId, nSize;
    if (helpIdSize32)
      *input>>nHelpId;
    else {
      uint16_t tmp;
      *input >> tmp;
      nHelpId=tmp;
    }
    if (nHelpId) f << "help[id]=" << nHelpId << ",";
    *input >> nItemCount;
    if (nItemCount) {
      f << "item[count]=" << nItemCount << ",";
      input->seek(-2, librevenge::RVNG_SEEK_CUR);
      if (!StarFileManager::readSfxItemList(zone)) {
        f << "###itemList";
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
        if (poolVersion==1) return true;
        continue;
      }
      ascii.addDelimiter(input->tell(),'|');
    }
    *input>>nVer>>nSize;
    if (nVer) f << "version=" << nVer << ",";
    if (input->tell()+nSize>lastPos || (poolVersion==2 && input->tell()+nSize+4<lastPos)) {
      // be strict while readSfxItemList is not sure
      STOFF_DEBUG_MSG(("StarItemPool::readStyle: something is bad\n"));
      f << "###nSize=" << nSize << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    if (nSize) {
      f << "#size=" << nSize << ",";
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarItemPool::readStyle: loading a sheet is not implemented\n"));
        first=false;
      }
      // normally SfxStyleSheetBase::Load but no code
      libstoff::DebugStream f2;
      f2 << "Entries(SfxBaseSheet):sz=" << nSize << ",##";
      ascii.addPos(input->tell()-4);
      ascii.addNote(f2.str().c_str());
      ascii.addDelimiter(input->tell(),'|');
      input->seek(nSize, librevenge::RVNG_SEEK_CUR);
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  if (poolVersion==2) {
    mRecord.close("SfxStylePool");
    zone.closeSfxRecord(type, "SfxStylePool");
  }
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
