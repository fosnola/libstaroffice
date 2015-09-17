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

#include "StarAttribute.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarZone.hxx"

#include "StarDocument.hxx"

/** Internal: the structures of a StarDocument */
namespace StarDocumentInternal
{
//! the state of a StarDocument
struct State {
  //! constructor
  State(shared_ptr<StarAttributeManager> attributeManager) :
    m_poolList(), m_attributeManager(attributeManager), m_sdwParser(0)
  {
  }
  //! the list of pool
  std::vector<shared_ptr<StarItemPool> > m_poolList;
  //! the attribute manager
  shared_ptr<StarAttributeManager> m_attributeManager;
  //! the sdw parser REMOVEME
  SDWParser *m_sdwParser;
private:
  State(State const &orig);
  State operator=(State const &orig);
};
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarDocument::StarDocument(STOFFInputStreamPtr input, char const *passwd,
                           shared_ptr<STOFFOLEParser> oleParser, shared_ptr<STOFFOLEParser::OleDirectory> directory,
                           shared_ptr<StarAttributeManager> attributeManager, SDWParser *sdwParser) :
  m_input(input), m_password(passwd), m_oleParser(oleParser), m_directory(directory), m_state(new StarDocumentInternal::State(attributeManager))
{
  m_state->m_sdwParser=sdwParser;
}

StarDocument::~StarDocument()
{
}

STOFFDocument::Kind StarDocument::getDocumentKind() const
{
  return m_directory ? m_directory->m_kind : STOFFDocument::STOFF_K_UNKNOWN;
}

shared_ptr<StarAttributeManager> StarDocument::getAttributeManager()
{
  return m_state->m_attributeManager;
}

shared_ptr<StarItemPool> StarDocument::getNewItemPool(StarItemPool::Type type)
{
  shared_ptr<StarItemPool> pool(new StarItemPool(*this, type));
  m_state->m_poolList.push_back(pool);
  return pool;
}

shared_ptr<StarItemPool> StarDocument::getCurrentPool()
{
  for (size_t i=m_state->m_poolList.size(); i>0;) {
    shared_ptr<StarItemPool> pool=m_state->m_poolList[--i];
    if (pool && pool->isInside()) return pool;
  }
  return shared_ptr<StarItemPool>();
}

shared_ptr<StarItemPool> StarDocument::findItemPool(StarItemPool::Type type, bool isInside)
{
  for (size_t i=m_state->m_poolList.size(); i>0;) {
    shared_ptr<StarItemPool> pool=m_state->m_poolList[--i];
    if (!pool || pool->getType()!=type) continue;
    if (isInside && !pool->isInside()) continue;
    return pool;
  }
  return shared_ptr<StarItemPool>();
}

SDWParser *StarDocument::getSDWParser()
{
  return m_state->m_sdwParser;
}

bool StarDocument::parse()
{
  if (!m_oleParser || !m_directory) {
    STOFF_DEBUG_MSG(("StarDocument::readPersistElements: can not find directory\n"));
    return false;
  }
  if (!m_directory->m_hasCompObj) {
    STOFF_DEBUG_MSG(("StarDocument::readPersistElements: called with unknown document\n"));
  }
  for (size_t i = 0; i < m_directory->m_contentList.size(); ++i) {
    STOFFOLEParser::OleContent &content=m_directory->m_contentList[i];
    if (content.isParsed()) continue;
    std::string name = content.getOleName();
    std::string const &base = content.getBaseName();
    STOFFInputStreamPtr ole = m_input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarDocument::createZones: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    ole->setReadInverted(true);
    if (base=="VCPool") {
      content.setParsed(true);
      StarZone zone(ole, name, "VCPool", m_password);
      zone.ascii().open(name);
      ole->seek(0, librevenge::RVNG_SEEK_SET);
      getNewItemPool(StarItemPool::T_VCControlPool)->read(zone);
      continue;
    }
    if (base=="persist elements") {
      content.setParsed(true);
      readPersistElements(ole, name);
      continue;
    }
    if (base=="SfxPreview") {
      content.setParsed(true);
      readSfxPreview(ole, name);
      continue;
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    bool ok=false;
    if (base=="SfxDocumentInfo")
      ok=readSfxDocumentInformation(ole, asciiFile);
    else if (base=="SfxWindows")
      ok=readSfxWindows(ole, asciiFile);
    else if (base=="Star Framework Config File")
      ok=readStarFrameworkConfigFile(ole, asciiFile);
    content.setParsed(ok);
  }

  return true;
}

bool StarDocument::readItemSet(StarZone &zone, std::vector<STOFFVec2i> const &/*limits*/, long lastPos, StarItemPool *pool, bool isDirect)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  f << "Entries(StarItem):pool,";
  // itemset.cxx: SfxItemSet::Load (ncount)
  uint16_t n;
  *input >> n;
  f << "N=" << n << ",";
  if (!pool) {
    if (input->tell()+6*n > lastPos) {
      STOFF_DEBUG_MSG(("StarDocument::readItemSet: can not read a SfxItemSet\n"));
      f << "###,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      return false;
    }
    if (n) {
      if (lastPos!=pos+2+6*n) {
        // TODO poolio.cxx SfxItemPool::LoadItem
        static bool first=true;
        if (first) {
          STOFF_DEBUG_MSG(("StarDocument::readItemSet: reading a SfxItem is not implemented without pool\n"));
          first=false;
        }
        f << "##noPool";
      }
      else
        f << "#";
      input->seek(pos+2+6*n, librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  for (int i=0; i<int(n); ++i) {
    pos=input->tell();
    if (pool->readItem(zone, isDirect, lastPos)) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    ascFile.addPos(pos);
    ascFile.addNote("StarItem:pool,###extra");
    break;
  }
  return true;
}

bool StarDocument::readPersistElements(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "PersistsElement", m_password);
  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(name);
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(Persists):";
  // persist.cxx: SvPersist::LoadContent
  if (input->size()<21 || input->readLong(1)!=2) {
    STOFF_DEBUG_MSG(("StarDocument::readPersistElements: data seems bad\n"));
    f << "###";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  int hasElt=(int) input->readLong(1);
  if (hasElt==1) {
    if (input->size()<29) {
      STOFF_DEBUG_MSG(("StarDocument::readPersistElements: flag hasData, but zone seems too short\n"));
      f << "###";
      hasElt=0;
    }
    f << "hasData,";
  }
  else if (hasElt) {
    STOFF_DEBUG_MSG(("StarDocument::readPersistElements: flag hasData seems bad\n"));
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
      STOFF_DEBUG_MSG(("StarDocument::readPersistElements: data size seems bad\n"));
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
    if (readPersistData(zone, endDataPos))
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    f.str("");
    f << "Persists-A" << i << ":";
    STOFF_DEBUG_MSG(("StarDocument::readPersistElements: data %d seems bad\n", i));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    break;
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

bool StarDocument::readPersistData(StarZone &zone, long lastPos)
{
  // pstm.cxx SvPersistStream::ReadObj
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascii=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(PersistData)["<< zone.getRecordLevel() << "]:";
  // SvPersistStream::ReadId
  uint8_t hdr;
  *input >> hdr;
  long id=0, classId=0;
  bool ok=true;
  if (hdr&0x80) // nId=0
    ;
  else {
    if ((hdr&0xf)==0) {
      if ((hdr&0x20) || !(hdr&0x40))
        ok=input->readCompressedLong(id);
    }
    else if (hdr&0x10)
      ok=input->readCompressedLong(id);
    if (hdr&0x60)
      ok=input->readCompressedLong(classId);
  }
  if (id) f << "id=" << id << ",";
  if (classId) f << "id[class]=" << classId << ",";
  if (!ok || !hdr || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarDocument::readPersistData: find unexpected header\n"));
    f << "###header";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return false;
  }
  if (hdr&0x80 || (hdr&0x40)==0) {
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return true;
  }
  if (hdr&0x20) {
    ok=zone.openSCRecord();
    if (!ok || zone.getRecordLastPosition()>lastPos) {
      STOFF_DEBUG_MSG(("StarDocument::readPersistData: can not open main zone\n"));
      if (ok) zone.closeSCRecord("PersistData");
      f << "###,";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      return false;
    }
    lastPos=zone.getRecordLastPosition();
  }
  if (hdr&0x40) {
    // app.cxx OfficeApplication::Init
    switch (classId) {
    case 2: {
      long actPos=input->tell();
      uint8_t val;
      *input>>val;
      librevenge::RVNGString text;
      if (!zone.readString(text)||input->tell()>=lastPos) {
        input->seek(actPos, librevenge::RVNG_SEEK_SET);
        f << "##classId";
        break;
      }
      f << "objData,";
      if (val) f << "f0=" << int(val) << ","; // 0 or 1
      f << text.cstr() << ",";
      break;
    }
    case 3: { // flditem.cxx:void SvxURLField::Load
      f << "urlData,";
      uint16_t format;
      *input >> format;
      if (format) f << "format=" << format << ",";
      for (int i=0; i<2; ++i) {
        librevenge::RVNGString text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarDocument::readPersistData: can not read a string\n"));
          f << "##string";
          break;
        }
        else if (!text.empty())
          f << (i==0 ? "url" : "representation") << "=" << text.cstr() << ",";
      }
      if (input->tell()==lastPos)
        break;
      uint32_t nFrameMarker;
      *input>>nFrameMarker;
      uint16_t val;
      switch (nFrameMarker) {
      case 0x21981357:
        *input>>val;
        if (val) f << "char[set]=" << val << ",";
        break;
      case 0x21981358:
        for (int i=0; i<2; ++i) {
          *input>>val;
          if (val) f << "f" << i << "=" << val << ",";
        }
        break;
      default:
        input->seek(-4, librevenge::RVNG_SEEK_CUR);
        break;
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarDocument::readPersistData: unknown class id\n"));
      f << "##classId";
      break;
    }
  }
  if (input->tell()!=lastPos)
    ascii.addDelimiter(input->tell(),'|');
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  if (hdr&0x20)
    zone.closeSCRecord("PersistData");

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  return true;
}

bool StarDocument::readSfxDocumentInformation(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);

  libstoff::DebugStream f;
  f << "Entries(SfxDocInfo):";

  // see sfx2_docinf.cxx
  int sSz=(int) input->readULong(2);
  if (2+sSz>input->size()) {
    STOFF_DEBUG_MSG(("StarDocument::readSfxDocumentInformation: header seems bad\n"));
    f << "###sSz=" << sSz << ",";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  std::string text("");
  for (int i=0; i<sSz; ++i) text+=(char) input->readULong(1);
  if (text!="SfxDocumentInfo") {
    STOFF_DEBUG_MSG(("StarDocument::readSfxDocumentInformation: header seems bad\n"));
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
  if (bPasswd) f << "passwd,"; // the password does not seems to be kept/used in this block
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
      STOFF_DEBUG_MSG(("StarDocument::readSfxDocumentInformation: can not read string %d\n", i));
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
    STOFF_DEBUG_MSG(("StarDocument::readSfxDocumentInformation: last zone seems too short\n"));
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

bool StarDocument::readStarFrameworkConfigFile(STOFFInputStreamPtr input, libstoff::DebugFile &asciiFile)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(StarFrameworkConfig):";
  // see sfx2_cfgimex SfxConfigManagerImExport_Impl::Import
  std::string header("");
  for (int i=0; i<26; ++i) header+=(char) input->readULong(1);
  if (!input->checkPosition(33)||header!="Star Framework Config File") {
    STOFF_DEBUG_MSG(("StarDocument::readStarFrameworkConfigFile: the header seems bad\n"));
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
    STOFF_DEBUG_MSG(("StarDocument::readStarFrameworkConfigFile: dir pos is bad\n"));
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
        STOFF_DEBUG_MSG(("StarDocument::readStarFrameworkConfigFile: a item position seems bad\n"));
        f << "###";
      }
      else {
        static bool first=true;
        if (first) {
          STOFF_DEBUG_MSG(("StarDocument::readStarFrameworkConfigFile: Ohhh find some item\n"));
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
      STOFF_DEBUG_MSG(("StarDocument::readStarFrameworkConfigFile: a item seems bad\n"));
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

bool StarDocument::readSfxPreview(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxPreview", m_password);
  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(name);
  input->seek(0, librevenge::RVNG_SEEK_SET);
  StarFileManager fileManager;
  if (!fileManager.readSVGDI(zone)) {
    STOFF_DEBUG_MSG(("StarDocument::readSfxPreview: can not find the first image\n"));
    input->seek(0, librevenge::RVNG_SEEK_SET);
  }
  if (input->isEnd()) return true;

  long pos=input->tell();
  libstoff::DebugStream f;
  STOFF_DEBUG_MSG(("StarDocument::readSfxPreview: find extra data\n"));
  f << "Entries(SfxPreview):###extra";

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  return true;
}

bool StarDocument::readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
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
    STOFF_DEBUG_MSG(("StarDocument::readSfxWindows: find extra data\n"));
    ascii.addPos(input->tell());
    ascii.addNote("SfWindows:extra###");
  }
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
