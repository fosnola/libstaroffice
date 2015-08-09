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

#include "SWAttributeManager.hxx"
#include "StarFileManager.hxx"

/** Internal: the structures of a StarFileManager */
namespace StarFileManagerInternal
{
////////////////////////////////////////
//! Internal: a structure use to read SfxMultiRecord zone of a StarFileManager
struct SfxMultiRecord {
  //! constructor
  SfxMultiRecord(StarZone &zone) : m_zone(zone), m_zoneType(0), m_zoneOpened(false), m_headerType(0), m_headerVersion(0), m_headerTag(0),
    m_actualRecord(0), m_numRecord(0), m_contentSize(0),
    m_startPos(0), m_endPos(0), m_offsetList(), m_extra("")
  {
  }
  //! try to open a zone
  bool open()
  {
    if (m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord: oops a record has been opened\n"));
      return false;
    }
    m_actualRecord=m_numRecord=0;
    m_headerType=m_headerVersion=0;
    m_headerTag=0;
    m_contentSize=0;
    m_offsetList.clear();

    STOFFInputStreamPtr input=m_zone.input();
    long pos=input->tell();
    if (!m_zone.openSfxRecord(m_zoneType)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    if (m_zoneType==char(0xff)) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord: oops end header\n"));
      m_extra="###emptyZone,";
      return true; /* empty zone*/
    }
    if (m_zoneType!=0) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord: find unknown header\n"));
      m_extra="###badZoneType,";
      return true;
    }

    m_zoneOpened=true;
    m_endPos=m_zone.getRecordLastPosition();
    // filerec.cxx: SfxSingleRecordReader::FindHeader_Impl
    if (input->tell()+10>m_endPos) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::open: oops the zone seems too short\n"));
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
        STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::open: oops the number of record seems bad\n"));
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
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::open: can not find the version map offset\n"));
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
    if (!m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::close: can not find any opened zone\n"));
      return;
    }
    m_zoneOpened=false;
    STOFFInputStreamPtr input=m_zone.input();
    if (input->tell()<m_endPos && input->tell()+4>=m_endPos) { // small diff is possible
      m_zone.ascii().addDelimiter(input->tell(),'|');
      input->seek(m_zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()==m_endPos)
      input->seek(m_zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    m_zone.closeSfxRecord(m_zoneType, wh);
  }
  //! returns the header tag or -1
  int getHeaderTag() const
  {
    return !m_zoneOpened ? -1 : int(m_headerTag);
  }
  //! try to go to the new content positon
  bool getNewContent()
  {
    // SfxMultiRecordReader::GetContent
    long newPos=getLastContentPosition();
    if (newPos>=m_endPos) return false;
    STOFFInputStreamPtr input=m_zone.input();
    ++m_actualRecord;
    if (input->tell()<newPos && input->tell()+4>=newPos) { // small diff is possible
      m_zone.ascii().addDelimiter(input->tell(),'|');
      input->seek(newPos, librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()!=newPos) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::getNewContent: find extra data\n"));
      m_zone.ascii().addPos(input->tell());
      m_zone.ascii().addNote("Entries(BadPos):###extra");
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
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::getLastContentPosition: argh, find unexpected index\n"));
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
  StarZone &m_zone;
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

////////////////////////////////////////
//! Internal: the state of a StarFileManager
struct State {
  //! constructor
  State()
  {
  }
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarFileManager::StarFileManager() : m_state(new StarFileManagerInternal::State)
{
}

StarFileManager::~StarFileManager()
{
}

bool StarFileManager::readPersistElements(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(Persists):";
  // persist.cxx: SvPersist::LoadContent
  if (input->size()<21 || input->readLong(1)!=2) {
    STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: data seems bad\n"));
    f << "###";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  int hasElt=(int) input->readLong(1);
  if (hasElt==1) {
    if (input->size()<29) {
      STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: flag hasData, but zone seems too short\n"));
      f << "###";
      hasElt=0;
    }
    f << "hasData,";
  }
  else if (hasElt) {
    STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: flag hasData seems bad\n"));
    f << "#hasData=" << hasElt << ",";
    hasElt=0;
  }
  int val;
  int N=0;
  long endDataPos=0;
  if (hasElt) {
    val=(int) input->readULong(1); // always 80?
    if (val!=0x80) f << "#f0=" << std::hex << val << std::dec << ",";
    long dSz=(int) input->readULong(4);
    N=(int) input->readULong(4);
    f << "dSz=" << dSz << ",N=" << N << ",";
    if (!dSz || 7+dSz+18>input->size()) {
      STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: data size seems bad\n"));
      f << "###dSz";
      dSz=0;
      N=0;
    }
    endDataPos=7+dSz;
  }
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());
  for (int i=0; i<N; ++i) {
    long pos=input->tell();
    f.str("");
    f << "Persists-A" << i << ":";
    val=(int) input->readULong(1); // 60|70
    if (val) f << "f0=" << std::hex << val << std::dec << ",";
    long id;
    if (pos+10>endDataPos || !input->readCompressedLong(id)) {
      STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: data %d seems bad\n", i));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    if (id!=i+1) f << "id=" << id << ",";
    val=(int) input->readULong(1); // another compressed long ?
    if (val) f << "f1=" << val << ",";
    long dSz=(long) input->readULong(4);
    long endZPos=input->tell()+dSz;
    if (dSz<16 || endZPos>endDataPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: dSz seems bad\n"));
      f << "###dSz=" << dSz << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    int decalSz=(int) input->readULong(1);
    if (decalSz) f << "decal=" << decalSz << ",";
    int textSz=(int) input->readULong(2);
    bool oddHeader=false;
    if (textSz==0) {
      f << "#odd,";
      oddHeader=true;
      textSz=(int) input->readULong(2);
    }
    if (input->tell()+textSz>endZPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readPersistElements: data %d seems bad\n", i));
      f << "###textSz=" << textSz << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      input->seek(endZPos, librevenge::RVNG_SEEK_SET);
      continue;
    }
    std::string text("");
    for (int c=0; c<textSz; ++c) text += (char) input->readULong(1);
    f << text << ",";
    if (!oddHeader) { // always 0?
      val=(int) input->readLong(2);
      if (val) f << "f2=" << val << ",";
    }
    ascii.addDelimiter(input->tell(),'|');
    input->seek(endZPos, librevenge::RVNG_SEEK_SET);
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  input->seek(-18, librevenge::RVNG_SEEK_END);
  long pos=input->tell();
  f.str("");
  f << "Persists-B:";
  int dim[4];
  for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
  f << "dim=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]), STOFFVec2i(dim[2],dim[3])) << ",";
  val=(int) input->readLong(2); // 0|9
  if (val) f << "f0=" << val << ",";
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  return true;
}

bool StarFileManager::readSfxDocumentInformation(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);

  libstoff::DebugStream f;
  f << "Entries(SfxDocInfo):";

  // see sfx2_docinf.cxx
  int sSz=(int) input->readULong(2);
  if (2+sSz>input->size()) {
    STOFF_DEBUG_MSG(("StarFileManager::readSfxDocumentInformation: header seems bad\n"));
    f << "###sSz=" << sSz << ",";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  std::string text("");
  for (int i=0; i<sSz; ++i) text+=(char) input->readULong(1);
  if (text!="SfxDocumentInfo") {
    STOFF_DEBUG_MSG(("StarFileManager::readSfxDocumentInformation: header seems bad\n"));
    f << "###text=" << text << ",";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  // FileHeader::FileHeader, SfxDocumentInfo::Load
  uint16_t nVersion, nUS;
  bool bPasswd, bPGraphic, bQTemplate;
  *input >> nVersion >> bPasswd >> nUS >> bPGraphic >> bQTemplate;
  if (nVersion) f << "vers=" << std::hex << nVersion << std::dec << ",";
  if (nUS) f << "encoding=" << nUS << ","; // need to load encoding here
  if (bPasswd) f << "passwd,";
  if (bPGraphic) f << "portableGraphic,";
  if (bQTemplate) f << "queryTemplate,";
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());

  for (int i=0; i<17; ++i) {
    long pos=input->tell();
    f.str("");
    f << "SfxDocInfo-A" << i << ":";
    int dSz=(int) input->readULong(2);
    int expectedSz= i < 3 ? 33 : i < 5 ? 65 : i==5 ? 257 : i==6 ? 129 : i<15 ? 21 : 2;
    static char const *(wh[])= {
      "time[creation]","time[mod]","time[print]","title","subject","comment","keyword",
      "user0[name]", "user0[data]","user1[name]", "user1[data]","user2[name]", "user2[data]","user3[name]", "user3[data]",
      "template[name]", "template[filename]"
    };
    f << wh[i] << ",";
    if (dSz+2>expectedSz) {
      if (i<15)
        expectedSz+=0x10000; // rare but can happen, probably due to a bug when calling SkipRep with a negative value
      else
        expectedSz=2+dSz;
    }
    if (pos+expectedSz+(i<3 ? 8 : 0)>input->size()) {
      STOFF_DEBUG_MSG(("StarFileManager::readSfxDocumentInformation: can not read string %d\n", i));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      return true;
    }
    text="";
    for (int c=0; c<dSz; ++c) text+=(char) input->readULong(1);
    f << text << ",";
    input->seek(pos+expectedSz, librevenge::RVNG_SEEK_SET);
    if (i<3) {
      uint32_t date, time;
      *input >> date >> time;
      f << "date=" << date << ", time=" << time << ",";
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  long pos=input->tell();
  f.str("");
  f << "SfxDocInfo-B:";
  if (pos+8>input->size()) {
    STOFF_DEBUG_MSG(("StarFileManager::readSfxDocumentInformation: last zone seems too short\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return true;
  }
  uint32_t date, time;
  *input >> date >> time;
  f << "date=" << date << ", time=" << time << ",";
  // the following depend on the file version, so let try to get the mail address and stop
  if (input->tell()+6 <= input->size()) {  // [MailAddr], lTime, ...
    uint16_t nMailAdr;
    *input >> nMailAdr;
    if (nMailAdr && nMailAdr<20 && input->tell()+4*nMailAdr<input->tell()) {
      f << "mailAdr=[";
      for (int i=0; i<int(nMailAdr); ++i) {
        sSz=(int) input->readULong(2);
        if (input->tell()+sSz+2>input->size())
          break;
        for (int c=0; c<sSz; ++c) text+=(char) input->readULong(1);
        f << text.c_str() << ",";
        input->seek(2, librevenge::RVNG_SEEK_CUR); // flag dummy
      }
      f << "],";
    }
  }
  if (!input->isEnd()) ascii.addDelimiter(input->tell(),'|');
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  return true;
}

bool StarFileManager::readStarFrameworkConfigFile(STOFFInputStreamPtr input, libstoff::DebugFile &asciiFile)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(StarFrameworkConfig):";
  // see sfx2_cfgimex SfxConfigManagerImExport_Impl::Import
  std::string header("");
  for (int i=0; i<26; ++i) header+=(char) input->readULong(1);
  if (!input->checkPosition(33)||header!="Star Framework Config File") {
    STOFF_DEBUG_MSG(("StarFileManager::readStarFrameworkConfigFile: the header seems bad\n"));
    f << "###" << header;
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    return true;
  }
  uint8_t cC;
  uint16_t fileVersion;
  int32_t lDirPos;
  *input >> cC >> fileVersion >> lDirPos;
  if (cC!=26) f << "c=" << cC << ",";
  if (fileVersion!=26) f << "vers=" << fileVersion << ",";
  long pos=long(lDirPos);
  if (!input->checkPosition(pos+2)) {
    STOFF_DEBUG_MSG(("StarFileManager::readStarFrameworkConfigFile: dir pos is bad\n"));
    f << "###dirPos" << pos << ",";
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    return true;
  }
  if (input->tell()!=pos) asciiFile.addDelimiter(input->tell(),'|');
  asciiFile.addPos(0);
  asciiFile.addNote(f.str().c_str());

  input->seek(pos, librevenge::RVNG_SEEK_SET);
  f.str("");
  f << "StarFrameworkConfig:";
  uint16_t N;
  *input >> N;
  f << "N=" << N << ",";
  for (uint16_t i=0; i<N; ++i) {
    f << "item" << i << "=[";
    uint16_t nType;
    int32_t lPos, lLength;
    *input >> nType >> lPos >> lLength;
    if (nType) f << "nType=" << nType << ",";
    if (lPos!=-1) {
      if (!input->checkPosition(long(lPos))) {
        STOFF_DEBUG_MSG(("StarFileManager::readStarFrameworkConfigFile: a item position seems bad\n"));
        f << "###";
      }
      else {
        static bool first=true;
        if (first) {
          STOFF_DEBUG_MSG(("StarFileManager::readStarFrameworkConfigFile: Ohhh find some item\n"));
          first=true;
        }
        asciiFile.addPos(long(lPos));
        asciiFile.addNote("StarFrameworkConfig[Item]:###");
        // see SfxConfigManagerImExport_Impl::ImportItem, if needed
      }
      if (lLength) f << "len=" << lLength << ",";
    }
    int strSz=(int) input->readULong(2);
    if (!input->checkPosition(input->tell()+strSz)) {
      STOFF_DEBUG_MSG(("StarFileManager::readStarFrameworkConfigFile: a item seems bad\n"));
      f << "###item,";
      break;
    }
    std::string name("");
    for (int c=0; c<strSz; ++c) name+=(char) input->readULong(1);
    f << name << ",";
    f << "],";
  }
  asciiFile.addPos(pos);
  asciiFile.addNote(f.str().c_str());

  return true;
}

bool StarFileManager::readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(SfWindows):";
  // see sc_docsh.cxx
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());
  while (!input->isEnd()) {
    long pos=input->tell();
    if (!input->checkPosition(pos+2))
      break;
    int dSz=(int) input->readULong(2);
    if (!input->checkPosition(pos+2+dSz)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SfWindows:";
    std::string text("");
    for (int i=0; i<dSz; ++i) text+=(char) input->readULong(1);
    f << text;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarFileManager::readSfxWindows: find extra data\n"));
    ascii.addPos(input->tell());
    ascii.addNote("SfWindows:extra###");
  }
  return true;
}

bool StarFileManager::readImageDocument(STOFFInputStreamPtr input, librevenge::RVNGBinaryData &data, std::string const &fileName)
{
  // see this Ole with classic bitmap format
  libstoff::DebugFile ascii(input);
  ascii.open(fileName);
  ascii.skipZone(0, input->size());

  input->seek(0, librevenge::RVNG_SEEK_SET);
  data.clear();
  if (!input->readEndDataBlock(data)) {
    STOFF_DEBUG_MSG(("StarFileManager::readImageDocument: can not read image content\n"));
    return false;
  }
#ifdef DEBUG_WITH_FILES
  libstoff::Debug::dumpFile(data, (fileName+".ppm").c_str());
#endif
  return true;
}

bool StarFileManager::readEmbeddedPicture(STOFFInputStreamPtr input, librevenge::RVNGBinaryData &data, std::string const &fileName)
{
  // see this Ole with classic bitmap format
  libstoff::DebugFile ascii(input);
  ascii.open(fileName);
  data.clear();
  // impgraph.cxx: ImpGraphic::ImplReadEmbedded

  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(EmbeddedPicture):";
  uint32_t nId;
  int32_t nLen;

  *input>>nId;
  if (nId==0x35465347 || nId==0x47534635) { // CHECKME: order
    int32_t nType;
    uint16_t nMapMode;
    int32_t nOffsX, nOffsY, nScaleNumX, nScaleDenomX, nScaleNumY, nScaleDenomY;
    bool mbSimple;
    *input >> nType >> nLen;
    if (nType) f << "type=" << nType << ",";
    // mapmod.cxx: operator>>(..., ImplMapMode& )
    *input >> nMapMode >> nOffsX>> nOffsY;
    if (nMapMode) f << "mapMode=" << nMapMode << ",";
    *input >> nScaleNumX >> nScaleDenomX >> nScaleNumY >> nScaleDenomY >> mbSimple;
    f << "scaleX=" << nScaleNumX << "/" << nScaleDenomX << ",";
    f << "scaleY=" << nScaleNumY << "/" << nScaleDenomY << ",";
    if (mbSimple) f << "simple,";
  }
  else {
    if (nId>0x100) {
      input->seek(0, librevenge::RVNG_SEEK_SET);
      input->setReadInverted(!input->readInverted());
      *input>>nId;
    }
    if (nId) f << "type=" << nId << ",";
    int32_t nWidth, nHeight, nMapMode, nScaleNumX, nScaleDenomX, nScaleNumY, nScaleDenomY, nOffsX, nOffsY;
    *input >> nLen >> nWidth >> nHeight >> nMapMode;
    f << "size=" << nWidth << "x" << nHeight << ",";
    if (nMapMode) f << "mapMode=" << nMapMode << ",";
    *input >> nScaleNumX >> nScaleDenomX >> nScaleNumY >> nScaleDenomY;
    f << "scaleX=" << nScaleNumX << "/" << nScaleDenomX << ",";
    f << "scaleY=" << nScaleNumY << "/" << nScaleDenomY << ",";
    *input >> nOffsX >> nOffsY;
    f << "offset=" << nOffsX << "x" << nOffsY << ",";
  }
  ascii.addDelimiter(input->tell(),'|');
  if (nLen<10 || input->size()!=input->tell()+nLen) {
    f << "###";
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: the length seems bad\n"));
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return false;
  }
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());
  ascii.skipZone(input->tell()+4, input->size());
  // CHECKME: compressed, SVGD
  if (!input->readEndDataBlock(data)) {
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: can not read image content\n"));
    return true;
  }
#ifdef DEBUG_WITH_FILES
  libstoff::Debug::dumpFile(data, (fileName+".pict").c_str());
#endif
  return true;
}

bool StarFileManager::readOleObject(STOFFInputStreamPtr input, std::string const &fileName)
{
  // see this Ole only once with PaintBrush data ?
  libstoff::DebugFile ascii(input);
  ascii.open(fileName);
  ascii.skipZone(0, input->size());

#ifdef DEBUG_WITH_FILES
  input->seek(0, librevenge::RVNG_SEEK_SET);
  librevenge::RVNGBinaryData data;
  if (!input->readEndDataBlock(data)) {
    STOFF_DEBUG_MSG(("StarFileManager::readOleObject: can not read image content\n"));
    return false;
  }
  libstoff::Debug::dumpFile(data, (fileName+".ole").c_str());
#endif
  return true;
}

bool StarFileManager::readPool(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();
  libstoff::DebugStream f;
  f << "Entries(PoolDef):";
  long pos=input->tell();
  if (pos+18>endPos) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: the zone seems too short\n"));
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
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return readPoolV1(zone);
  }
  if (nMajorVers!=2) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  f << "version=" << int(nMajorVers) << ",";
  if (nMinorVers) f << "vers[minor]=" << int(nMinorVers) << ",";
  *input>>tag;
  if (tag!=0xffff) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  input->seek(4, librevenge::RVNG_SEEK_CUR); // 0,0

  char type; // always 1?
  if (!zone.openSfxRecord(type)) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not open the sfx record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  if (type!=1) f << "type=" << int(type) << ",";

  endPos=zone.getRecordLastPosition();
  if (input->tell()==endPos) {
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  // string part
  char type1;
  if (!zone.openSfxRecord(type1)) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not open the string sfx record\n"));
    f << "###openString";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  if (type!=16) f << "type1=" << int(type1) << ",";
  int16_t val;
  *input >> val;
  if (val) f << "loadingVersion=" << val << ",";
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not read the name\n"));
    f << "###name";
  }
  if (!string.empty())
    f << "name[ext]=" << string.cstr() << ",";
  zone.closeSfxRecord(type1, "PoolDef");

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "PoolDef[versMap]:";
  StarFileManagerInternal::SfxMultiRecord mRecord(zone);
  if (!mRecord.open()) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not open the versionMap sfx record\n"));
    f << "###openVersionMap";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  f << mRecord;
  bool ok=true;
  if (mRecord.getHeaderTag()!=0x20) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not find the version map tag\n"));
    f << "###tag";
    ok=false;
  }
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  int n=0;
  while (ok && mRecord.getNewContent()) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[versMap-" << n++ << "]:";
    uint16_t nVers, nStart, nEnd;
    *input >> nVers >> nStart >> nEnd;
    f << "vers=" << nVers << "," << nStart << "<->" << nEnd << ",";
    if (nStart>nEnd || input->tell()+2*(nEnd-nStart+1) > mRecord.getLastContentPosition()) {
      STOFF_DEBUG_MSG(("StarFileManager::readPool: can not find start|end pos\n"));
      f << "###badStartEnd";
      ok=false;
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    f << "pos=[";
    for (uint16_t i=nStart; i<=nEnd; ++i) {
      uint16_t nPos;
      *input>>nPos;
      f << nPos << ",";
    }
    f << "],";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  mRecord.close("PoolDef");

  pos=input->tell();
  f.str("");
  f << "PoolDef[attrib]:";
  if (!mRecord.open()) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not open the attrib sfx record\n"));
    f << "###attrib";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  f << mRecord;
  ok=true;
  if (mRecord.getHeaderTag()!=0x30) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not find the pool which tag\n"));
    f << "###tag";
    ok=false;
  }
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  n=0;
  while (ok && mRecord.getNewContent()) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[attrib-" << n++ << "]:";
    uint16_t nWhich, nVersion, nCount;
    *input >> nWhich >> nVersion >> nCount;
    f << "wh=" << nWhich << ", vers=" << nVersion << ", count=" << nCount << ",";
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: reading attribute is not implemented\n"));
      first=false;
    }
    f << "##";
    input->seek(mRecord.getLastContentPosition(), librevenge::RVNG_SEEK_SET);
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  mRecord.close("PoolDef");

  pos=input->tell();
  f.str("");
  f << "PoolDef[default]:";
  if (!mRecord.open()) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not open the default sfx record\n"));
    f << "###default";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  f << mRecord;
  ok=true;
  if (mRecord.getHeaderTag()!=0x50) {
    STOFF_DEBUG_MSG(("StarFileManager::readPool: can not find the pool which tag\n"));
    f << "###tag";
    ok=false;
  }
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  n=0;
  while (ok && mRecord.getNewContent()) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[default-" << n++ << "]:";
    uint16_t nWhich, nVersion;
    *input >> nWhich >> nVersion;
    f << "wh=" << nWhich << ", vers=" << nVersion << ",";
    f << "##";
    input->seek(mRecord.getLastContentPosition(), librevenge::RVNG_SEEK_SET);
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  mRecord.close("PoolDef");

  zone.closeSfxRecord(type, "PoolDef");
  return true;
}

bool StarFileManager::readPoolV1(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();
  libstoff::DebugStream f;
  f << "Entries(PoolDef):";
  long pos=input->tell();
  if (pos+18>endPos) {
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: the zone seems too short\n"));
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
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: find a minor version >= 3\n"));
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
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not read the name\n"));
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
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: attribSize is bad\n"));
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
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: attribSize\n"));
    f << "###attrib[sz]==0,";
  }

  *input >> tag;
  if (tag!=0x3333) {
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not find tag size\n"));
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
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: tableSize is bad\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  else if (tableSize)
    f << "table[sz]=" << tableSize << ",";
  else if (nMinorVers>=3) {
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: tableSize is null for version 3\n"));
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
      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: arrgh can not find versionmap position\n"));
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
          STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: arrgh nBytes is bad\n"));
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
      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: tag is bad \n"));
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

      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not find last attrib\n"));
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
      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: reading attribute is not implemented\n"));
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

        STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not find attrib size\n"));
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
        STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: default tag is bad \n"));
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

      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not find last attrib\n"));
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

      STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not find attrib size\n"));
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
    STOFF_DEBUG_MSG(("StarFileManager::readPoolV1: can not find end tag\n"));
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

bool StarFileManager::readMathDocument(STOFFInputStreamPtr input, std::string const &fileName)
{
  StarZone zone(input, fileName, "MathDocument"); // use encoding RTL_TEXTENCODING_MS_1252
  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(fileName);

  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(SMMathDocument):";
  // starmath_document.cxx Try3x
  uint32_t lIdent, lVersion;
  *input >> lIdent;
  if (lIdent==0x534d3330 || lIdent==0x30333034) {
    input->setReadInverted(input->readInverted());
    input->seek(0, librevenge::RVNG_SEEK_SET);
    *input >> lIdent;
  }
  if (lIdent!=0x30334d53L && lIdent!=0x34303330L) {
    STOFF_DEBUG_MSG(("StarFileManager::readMathDocument: find unexpected header\n"));
    f << "###header";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  *input >> lVersion;
  f << "vers=" << std::hex << lVersion << std::dec << ",";
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());
  librevenge::RVNGString text;
  while (!input->isEnd()) {
    long pos=input->tell();
    int8_t cTag;
    *input>>cTag;
    f.str("");
    f << "SMMathDocument[" << char(cTag) << "]:";
    bool done=true, isEnd=false;;
    switch (cTag) {
    case 0:
      isEnd=true;
      break;
    case 'T':
      if (!zone.readString(text)) {
        done=false;
        break;
      }
      f << text.cstr();
      break;
    case 'D':
      for (int i=0; i<4; ++i) {
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("StarFileManager::readMathDocument: can not read a string\n"));
          f << "###string" << i << ",";
          done=false;
          break;
        }
        if (!text.empty()) f << "str" << i << "=" << text.cstr() << ",";
        if (i==1 || i==2) {
          uint32_t date, time;
          *input >> date >> time;
          f << "date" << i << "=" << date << ",";
          f << "time" << i << "=" << time << ",";
        }
      }
      break;
    case 'F': {
      // starmath_format.cxx operator>>
      uint16_t n, vLeftSpace, vRightSpace, vTopSpace, vDist, vSize;
      *input>>n>>vLeftSpace>>vRightSpace;
      f << "baseHeight=" << (n&0xff) << ",";
      if (n&0x100) f << "isTextMode,";
      if (n&0x200) f << "bScaleNormalBracket,";
      if (vLeftSpace) f << "vLeftSpace=" << vLeftSpace << ",";
      if (vRightSpace) f << "vRightSpace=" << vRightSpace << ",";
      f << "size=[";
      for (int i=0; i<=4; ++i) {
        *input>>vSize;
        f << vSize << ",";
      }
      f << "],";
      *input>>vTopSpace;
      if (vTopSpace) f << "vTopSpace=" << vTopSpace << ",";
      f << "fonts=[";
      for (int i=0; i<=6; ++i) {
        // starmath_utility.cxx operator>>(..., SmFace)
        uint32_t nData1, nData2, nData3, nData4;
        f << "[";
        if (input->isEnd() || !zone.readString(text)) {
          STOFF_DEBUG_MSG(("StarFileManager::readMathDocument: can not read a font string\n"));
          f << "###string,";
          done=false;
          break;
        }
        f << text.cstr() << ",";
        *input >> nData1 >> nData2 >> nData3 >> nData4;
        if (nData1) f << "familly=" << nData1 << ",";
        if (nData2) f << "encoding=" << nData2 << ",";
        if (nData3) f << "weight=" << nData3 << ",";
        if (nData4) f << "bold=" << nData4 << ",";
        f << "],";
      }
      f << "],";
      if (!done) break;
      if (input->tell()+21*2 > input->size()) {
        STOFF_DEBUG_MSG(("StarFileManager::readMathDocument: zone is too short\n"));
        f << "###short,";
        done=false;
        break;
      }
      f << "vDist=[";
      for (int i=0; i<=18; ++i) {
        *input >> vDist;
        if (vDist)
          f << vDist << ",";
        else
          f << "_,";
      }
      f << "],";
      *input >> n >> vDist;
      f << "vers=" << (n>>8) << ",";
      f << "eHorAlign=" << (n&0xFF) << ",";
      if (vDist) f << "bottomSpace=" << vDist << ",";
      break;
    }
    case 'S': {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarFileManager::readMathDocument: can not read a string\n"));
        f << "###string,";
        done=false;
        break;
      }
      f << text.cstr() << ",";
      uint16_t n;
      *input>>n;
      if (n) f << "n=" << n << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarFileManager::readMathDocument: find unexpected type\n"));
      f << "###type,";
      done=false;
      break;
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    if (!done) {
      ascii.addDelimiter(input->tell(),'|');
      break;
    }
    if (isEnd)
      break;
  }
  return true;
}

bool StarFileManager::readOutPlaceObject(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(OutPlaceObject):";
  // see outplace.cxx SvOutPlaceObject::SaveCompleted
  if (input->size()<7) {
    STOFF_DEBUG_MSG(("StarFileManager::readOutPlaceObject: file is too short\n"));
    f << "###";
  }
  else {
    uint16_t len;
    uint32_t dwAspect;
    bool setExtent;
    *input>>len >> dwAspect >> setExtent;
    if (len!=7) f << "length=" << len << ",";
    if (dwAspect) f << "dwAspect=" << dwAspect << ",";
    if (setExtent) f << setExtent << ",";
    if (!input->isEnd()) {
      STOFF_DEBUG_MSG(("StarFileManager::readOutPlaceObject: find extra data\n"));
      f << "###extra";
      ascii.addDelimiter(input->tell(),'|');
    }
  }
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());
  return true;
}

////////////////////////////////////////////////////////////
// small zone
////////////////////////////////////////////////////////////
bool StarFileManager::readJobSetUp(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  // sw_sw3misc.cxx: InJobSetup
  libstoff::DebugStream f;
  f << "Entries(JobSetUp)[" << zone.getRecordLevel() << "]:";
  // sfx2_printer.cxx: SfxPrinter::Create
  // jobset.cxx: JobSetup operator>>
  long lastPos=zone.getRecordLastPosition();
  int len=(int) input->readULong(2);
  f << "nLen=" << len << ",";
  if (len==0) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  bool ok=false;
  int nSystem=0;
  if (input->tell()+2+64+3*32<=lastPos) {
    nSystem=(int) input->readULong(2);
    f << "system=" << nSystem << ",";
    for (int i=0; i<4; ++i) {
      long actPos=input->tell();
      int const length=(i==0 ? 64 : 32);
      std::string name("");
      for (int c=0; c<length; ++c) {
        char ch=(char)input->readULong(1);
        if (ch==0) break;
        name+=ch;
      }
      if (!name.empty()) {
        static char const *(wh[])= { "printerName", "deviceName", "portName", "driverName" };
        f << wh[i] << "=" << name << ",";
      }
      input->seek(actPos+length, librevenge::RVNG_SEEK_SET);
    }
    ok=true;
  }
  if (ok && nSystem<0xffffe) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "JobSetUp:driverData,";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  else if (ok && input->tell()+22<=lastPos) {
    int driverDataLen=0;
    f << "nSize2=" << input->readULong(2) << ",";
    f << "nSystem2=" << input->readULong(2) << ",";
    driverDataLen=(int) input->readULong(4);
    f << "nOrientation=" << input->readULong(2) << ",";
    f << "nPaperBin=" << input->readULong(2) << ",";
    f << "nPaperFormat=" << input->readULong(2) << ",";
    f << "nPaperWidth=" << input->readULong(4) << ",";
    f << "nPaperHeight=" << input->readULong(4) << ",";

    if (driverDataLen && input->tell()+driverDataLen<=lastPos) {
      ascFile.addPos(input->tell());
      ascFile.addNote("JobSetUp:driverData");
      input->seek(driverDataLen, librevenge::RVNG_SEEK_CUR);
    }
    else if (driverDataLen)
      ok=false;
    if (ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      f.str("");
      f << "JobSetUp[values]:";
      if (nSystem==0xfffe) {
        librevenge::RVNGString text;
        while (input->tell()<lastPos) {
          for (int i=0; i<2; ++i) {
            if (!zone.readString(text)) {
              f << "###string,";
              ok=false;
              break;
            }
            f << text.cstr() << (i==0 ? ':' : ',');
          }
          if (!ok)
            break;
        }
      }
      else if (input->tell()<lastPos) {
        ascFile.addPos(input->tell());
        ascFile.addNote("JobSetUp:driverData");
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      }
    }
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarFileManager::readSfxItemList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  f << "Entries(SfxItemList):";
  // itemset.cxx: SfxItemSet::Load (ncount)
  uint16_t n;
  *input >> n;
  f << "N=" << n << ",";
  if (input->tell()+6*n > zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("StarFileManager::readSfxItemList: can not read a SfxItemList\n"));
    f << "###,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return false;
  }
  if (n) {
    // TODO poolio.cxx SfxItemPool::LoadItem
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarFileManager::readSfxItemList: reading a SfxItem is not implemented\n"));
      first=false;
    }
    f << "##";
    input->seek(pos+2+6*n, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
