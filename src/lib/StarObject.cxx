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
#include "StarFormatManager.hxx"

#include "StarObject.hxx"

/** Internal: the structures of a StarObject */
namespace StarObjectInternal
{
//! the state of a StarObject
struct State {
  //! constructor
  State()
    : m_poolList()
    , m_attributeManager(new StarAttributeManager)
    , m_formatManager(new StarFormatManager)
  {
  }
  //! copy constructor
  State(State const &) = default;
  //! the list of pool
  std::vector<std::shared_ptr<StarItemPool> > m_poolList;
  //! the attribute manager
  std::shared_ptr<StarAttributeManager> m_attributeManager;
  //! the format manager
  std::shared_ptr<StarFormatManager> m_formatManager;
  //! the list of user name
  librevenge::RVNGString m_userMetaNames[4];
private:
  State operator=(State const &) = delete;
};
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObject::StarObject(char const *passwd, std::shared_ptr<STOFFOLEParser> &oleParser, std::shared_ptr<STOFFOLEParser::OleDirectory> &directory)
  : m_password(passwd)
  , m_oleParser(oleParser)
  , m_directory(directory)
  , m_state(new StarObjectInternal::State())
  , m_metaData()
{
}

StarObject::StarObject(StarObject const &orig, bool duplicateState)
  : m_password(orig.m_password)
  , m_oleParser(orig.m_oleParser)
  , m_directory(orig.m_directory)
  , m_state()
  , m_metaData(orig.m_metaData)
{
  if (duplicateState)
    m_state.reset(new StarObjectInternal::State(*orig.m_state));
  else
    m_state.reset(new StarObjectInternal::State);
}

StarObject::~StarObject()
{
}

void StarObject::cleanPools()
{
  for (auto &p : m_state->m_poolList) {
    if (p)
      p->clean();
  }
  m_state->m_poolList.clear();
}

STOFFDocument::Kind StarObject::getDocumentKind() const
{
  return m_directory ? m_directory->m_kind : STOFFDocument::STOFF_K_UNKNOWN;
}

std::shared_ptr<StarAttributeManager> StarObject::getAttributeManager()
{
  return m_state->m_attributeManager;
}

std::shared_ptr<StarFormatManager> StarObject::getFormatManager()
{
  return m_state->m_formatManager;
}

librevenge::RVNGString StarObject::getUserNameMetaData(int i) const
{
  if (i>=0 && i<=3) {
    if (!m_state->m_userMetaNames[i].empty())
      return m_state->m_userMetaNames[i];
  }
  STOFF_DEBUG_MSG(("StarObject::parse: can not find user meta data %d\n", i));
  librevenge::RVNGString res;
  res.sprintf("Info%d", i);
  return res;
}

std::shared_ptr<StarItemPool> StarObject::getNewItemPool(StarItemPool::Type type)
{
  std::shared_ptr<StarItemPool> pool(new StarItemPool(*this, type));
  m_state->m_poolList.push_back(pool);
  return pool;
}

std::shared_ptr<StarItemPool> StarObject::getCurrentPool(bool onlyInside)
{
  for (size_t i=m_state->m_poolList.size(); i>0;) {
    auto pool=m_state->m_poolList[--i];
    if (pool && !pool->isSecondaryPool() && (!onlyInside || pool->isInside()))
      return pool;
  }
  return std::shared_ptr<StarItemPool>();
}

std::shared_ptr<StarItemPool> StarObject::findItemPool(StarItemPool::Type type, bool isInside)
{
  for (size_t i=m_state->m_poolList.size(); i>0;) {
    auto pool=m_state->m_poolList[--i];
    if (!pool || pool->getType()!=type) continue;
    if (isInside && !pool->isInside()) continue;
    return pool;
  }
  return std::shared_ptr<StarItemPool>();
}

bool StarObject::parse()
{
  if (!m_directory) {
    STOFF_DEBUG_MSG(("StarObject::parse: can not find directory\n"));
    return false;
  }
  if (!m_directory->m_hasCompObj) {
    STOFF_DEBUG_MSG(("StarObject::parse: called with unknown document\n"));
  }
  for (auto &content : m_directory->m_contentList) {
    if (content.isParsed()) continue;
    auto name = content.getOleName();
    auto const &base = content.getBaseName();
    STOFFInputStreamPtr ole;
    if (m_directory->m_input)
      ole = m_directory->m_input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObject::createZones: error: can not find OLE part: \"%s\"\n", name.c_str()));
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
    if (base=="SfxDocumentInfo") {
      content.setParsed(true);
      readSfxDocumentInformation(ole, name);
      continue;
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    bool ok=false;
    if (base=="SfxWindows")
      ok=readSfxWindows(ole, asciiFile);
    else if (base=="Star Framework Config File")
      ok=readStarFrameworkConfigFile(ole, asciiFile);
    content.setParsed(ok);
  }

  return true;
}

bool StarObject::readItemSet(StarZone &zone, std::vector<STOFFVec2i> const &/*limits*/, long lastPos,
                             StarItemSet &itemSet, StarItemPool *pool, bool isDirect)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  itemSet.m_whichToItemMap.clear();
  f << "Entries(StarItem):pool,";
  // itemset.cxx: SfxItemSet::Load (ncount)
  uint16_t n;
  *input >> n;
  f << "N=" << n << ",";
  if (!pool) {
    if (input->tell()+6*n > lastPos) {
      STOFF_DEBUG_MSG(("StarObject::readItemSet: can not read a SfxItemSet\n"));
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
          STOFF_DEBUG_MSG(("StarObject::readItemSet: reading a SfxItem is not implemented without pool\n"));
          first=false;
        }
        f << "##noPool,";
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
    auto item=pool->readItem(zone, isDirect, lastPos);
    if (item && input->tell()<=lastPos) {
      itemSet.add(item);
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    ascFile.addPos(pos);
    ascFile.addNote("StarItem:pool,###extra");
    break;
  }
  return true;
}

bool StarObject::readPersistElements(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "PersistsElement", m_password);
  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(name);
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(Persists):";
  // persist.cxx: SvPersist::LoadContent
  if (input->size()<21 || input->readLong(1)!=2) {
    STOFF_DEBUG_MSG(("StarObject::readPersistElements: data seems bad\n"));
    f << "###";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  auto hasElt=int(input->readLong(1));
  if (hasElt==1) {
    if (input->size()<29) {
      STOFF_DEBUG_MSG(("StarObject::readPersistElements: flag hasData, but zone seems too short\n"));
      f << "###";
      hasElt=0;
    }
    f << "hasData,";
  }
  else if (hasElt) {
    STOFF_DEBUG_MSG(("StarObject::readPersistElements: flag hasData seems bad\n"));
    f << "#hasData=" << hasElt << ",";
    hasElt=0;
  }
  int val;
  int N=0;
  long endDataPos=0;
  if (hasElt) {
    val=int(input->readULong(1)); // always 80?
    if (val!=0x80) f << "#f0=" << std::hex << val << std::dec << ",";
    auto dSz=long(input->readULong(4));
    N=int(input->readULong(4));
    f << "dSz=" << dSz << ",N=" << N << ",";
    if (!dSz || 7+dSz+18>input->size()) {
      STOFF_DEBUG_MSG(("StarObject::readPersistElements: data size seems bad\n"));
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
    STOFF_DEBUG_MSG(("StarObject::readPersistElements: data %d seems bad\n", i));
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
  for (int &i : dim) i=int(input->readLong(4));
  f << "dim=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]), STOFFVec2i(dim[2],dim[3])) << ",";
  val=int(input->readLong(2)); // 0|9
  if (val) f << "f0=" << val << ",";
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  return true;
}

bool StarObject::readPersistData(StarZone &zone, long lastPos)
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
    STOFF_DEBUG_MSG(("StarObject::readPersistData: find unexpected header\n"));
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
      STOFF_DEBUG_MSG(("StarObject::readPersistData: can not open main zone\n"));
      if (ok) zone.closeSCRecord("PersistData");
      f << "###,";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      return false;
    }
    lastPos=zone.getRecordLastPosition();
  }
  if (hdr&0x40) {
    // app.cxx OfficeApplication::Init, or SV_DECL_PERSIST1
    switch (classId) {
    // case 1 SvxFieldData:: or SvInfoObject:: or SvClassElement::
    case 2: { // embobj.cxx SvEmbeddedInfoObject::Load
      long actPos=input->tell();
      // SvInfoObject::Load
      uint8_t vers;
      *input>>vers;
      f << "objData,";
      if (vers) f << "vers=" << int(vers) << ","; // 0 or 1
      bool objOk=true;
      for (int i=0; i<2; ++i) {
        std::vector<uint32_t> text;
        if (!zone.readString(text)||input->tell()+16>=lastPos) {
          input->seek(actPos, librevenge::RVNG_SEEK_SET);
          f << "##stringId" << i << ",";
          objOk=false;
          break;
        }
        f << libstoff::getString(text).cstr() << ",";
      }
      if (!objOk) break;
      // SvGlobalName::operator<<
      int val;
      for (int i=0; i<3; ++i) {
        val=int(input->readULong(i==0 ? 4 : 2));
        if (val)
          f << "data" << i << "=" << std::hex << val << std::dec << ",";
      }
      f << "data3=[";
      for (int i=0; i<8; ++i) {
        val=int (input->readULong(1));
        if (val)
          f<< std::hex << val << std::dec << ",";
        else
          f << "_,";
      }
      f << "],";
      if (vers>0) {
        val=int (input->readULong(1));
        if (val) f << "deleted,";
      }
      if (input->readULong(1)!=2 || input->tell()+17>lastPos) {
        STOFF_DEBUG_MSG(("StarObject::readPersistData: can not find the object info\n"));
        input->seek(actPos, librevenge::RVNG_SEEK_SET);
        f << "##badInfo" << ",";
        break;
      }
      val=int (input->readULong(1));
      if (val)
        f << "isLink,";
      f << "rect=[";
      for (int i=0; i<4; ++i) f << input->readLong(4) << ",";
      f << "],";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObject::readPersistData: unknown class id\n"));
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

bool StarObject::readSfxDocumentInformation(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxDocInfo", nullptr); // no password
  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(name);
  input->seek(0, librevenge::RVNG_SEEK_SET);

  libstoff::DebugStream f;
  f << "Entries(SfxDocInfo):";

  // see sfx2_docinf.cxx
  auto sSz=int(input->readULong(2));
  if (2+sSz>input->size()) {
    STOFF_DEBUG_MSG(("StarObject::readSfxDocumentInformation: header seems bad\n"));
    f << "###sSz=" << sSz << ",";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  std::string text("");
  for (int i=0; i<sSz; ++i) text+=char(input->readULong(1));
  if (text!="SfxDocumentInfo") {
    STOFF_DEBUG_MSG(("StarObject::readSfxDocumentInformation: header seems bad\n"));
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
  auto encoding=StarEncoding::getEncodingForId(nUS);
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());

  librevenge::RVNGString prevAttrib;
  for (int i=0; i<17; ++i) {
    long pos=input->tell();
    f.str("");
    f << "SfxDocInfo-A" << i << ":";
    auto dSz=int(input->readULong(2));
    int expectedSz= i < 3 ? 33 : i < 5 ? 65 : i==5 ? 257 : i==6 ? 129 : i<15 ? 21 : 2;
    static char const *wh[]= {
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
      STOFF_DEBUG_MSG(("StarObject::readSfxDocumentInformation: can not read string %d\n", i));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      return true;
    }
    std::vector<uint8_t> string;
    for (int c=0; c<dSz; ++c) string.push_back(static_cast<uint8_t>(input->readULong(1)));
    std::vector<uint32_t> finalString;
    std::vector<size_t> srcPositions;
    if (StarEncoding::convert(string, encoding, finalString, srcPositions)) {
      auto attrib=libstoff::getString(finalString);
      f << attrib.cstr() << ",";
      static char const *attribNames[] = {
        "meta:initial-creator", "dc:creator", "", "dc:title", "dc:subject", "dc:description"/*comment*/, "meta:keywords",
        "", "user", "", "user", "", "user", "", "user",
        "librevenge:template-name", "librevenge:template-filename"
      };
      if ((i%2)==1 && i>=7 && i<=13)
        m_state->m_userMetaNames[(i-7)/2]=attrib;
      if (attrib.empty() || std::string(attribNames[i]).empty())
        prevAttrib=attrib;
      else if (std::string(attribNames[i])=="user") {
        if (!prevAttrib.empty()) {
          librevenge::RVNGString userMeta("librevenge:");
          userMeta.append(prevAttrib);
          m_metaData.insert(userMeta.cstr(), attrib);
        }
      }
      else
        m_metaData.insert(attribNames[i], attrib);
    }
    else {
      STOFF_DEBUG_MSG(("StarObject::readSfxDocumentInformation: can not convert a string\n"));
      f << "###string,";
      prevAttrib.clear();
    }
    input->seek(pos+expectedSz, librevenge::RVNG_SEEK_SET);
    if (i<3) {
      uint32_t date, time;
      *input >> date >> time;
      f << "date=" << date << ", time=" << time << ",";
      std::string dateTime;
      if (date && libstoff::convertToDateTime(date,time, dateTime)) {
        static char const *attribNames[]= { "meta:creation-date", "dc:date", "meta:print-date" };
        m_metaData.insert(attribNames[i], dateTime.c_str());
      }
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  long pos=input->tell();
  f.str("");
  f << "SfxDocInfo-B:";
  if (pos+8>input->size()) {
    STOFF_DEBUG_MSG(("StarObject::readSfxDocumentInformation: last zone seems too short\n"));
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
        sSz=int(input->readULong(2));
        if (input->tell()+sSz+2>input->size())
          break;
        for (int c=0; c<sSz; ++c) text+=char(input->readULong(1));
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

bool StarObject::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  std::shared_ptr<StarItemPool> pool;
  if (getDocumentKind()==STOFFDocument::STOFF_K_DRAW || getDocumentKind()==STOFFDocument::STOFF_K_PRESENTATION) {
    pool=getNewItemPool(StarItemPool::T_XOutdevPool);
    pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));
  }
  auto mainPool=pool;
  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("StarObject::readSfxStyleSheets: create extra pool for %d of type %d\n",
                         int(getDocumentKind()), int(pool->getType())));
      }
      if (!mainPool) mainPool=pool;
      pool.reset();
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->isEnd()) return true;
  long pos=input->tell();
  if (!mainPool || !mainPool->readStyles(zone, *this)) {
    STOFF_DEBUG_MSG(("StarObject::readSfxStyleSheets: can not read a style pool\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObject::readSfxStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SfxStyleSheets):###extra");
  }
  return true;
}

bool StarObject::readStarFrameworkConfigFile(STOFFInputStreamPtr input, libstoff::DebugFile &asciiFile)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(StarFrameworkConfig):";
  // see sfx2_cfgimex SfxConfigManagerImExport_Impl::Import
  std::string header("");
  for (int i=0; i<26; ++i) header+=char(input->readULong(1));
  if (!input->checkPosition(33)||header!="Star Framework Config File") {
    STOFF_DEBUG_MSG(("StarObject::readStarFrameworkConfigFile: the header seems bad\n"));
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
  auto pos=long(lDirPos);
  if (!input->checkPosition(pos+2)) {
    STOFF_DEBUG_MSG(("StarObject::readStarFrameworkConfigFile: dir pos is bad\n"));
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
    if (input->isEnd()) {
      STOFF_DEBUG_MSG(("StarObject::readStarFrameworkConfigFile: oops, end of file\n"));
      f << "###";
      break;
    }
    f << "item" << i << "=[";
    uint16_t nType;
    int32_t lPos, lLength;
    *input >> nType >> lPos >> lLength;
    if (nType) f << "nType=" << nType << ",";
    if (lPos!=-1) {
      long actPos=input->tell();
      STOFFEntry entry;
      entry.setId(int(nType));
      entry.setBegin(lPos);
      entry.setLength(lLength);
      readStarFrameworkConfigItem(entry, input, asciiFile);
      input->seek(actPos, librevenge::RVNG_SEEK_SET);
      if (lLength) f << "len=" << lLength << ",";
    }
    auto strSz=int(input->readULong(2));
    if (!input->checkPosition(input->tell()+strSz)) {
      STOFF_DEBUG_MSG(("StarObject::readStarFrameworkConfigFile: a item seems bad\n"));
      f << "###item,";
      break;
    }
    std::string name("");
    for (int c=0; c<strSz; ++c) name+=char(input->readULong(1));
    f << name << ",";
    f << "],";
  }
  asciiFile.addPos(pos);
  asciiFile.addNote(f.str().c_str());

  return true;
}

bool StarObject::readStarFrameworkConfigItem(STOFFEntry &entry, STOFFInputStreamPtr input, libstoff::DebugFile &asciiFile)
{
  libstoff::DebugStream f;
  f << "StarFrameworkConfig[Item]:";
  // see sfx2_cfgimex SfxConfigManagerImExport_Impl::ImportItem
  if (!entry.valid() || !input->checkPosition(long(entry.end()))) {
    STOFF_DEBUG_MSG(("StarObject::readStarFrameworkConfigFile: a item position seems bad\n"));
    f << "###";
    asciiFile.addPos(entry.begin());
    asciiFile.addNote(f.str().c_str());

    return true;
  }
  input->seek(entry.begin(), librevenge::RVNG_SEEK_SET);
  uint16_t nType;
  *input >> nType;
  f << "type=" << nType << ",";
  if (nType!=uint16_t(entry.id()) &&
      !(1294 <= nType && nType <= 1301 && 1294 <= entry.id() && entry.id() <= 1301)) {
    STOFF_DEBUG_MSG(("StarObject::readStarFrameworkConfigFile: find unexpected type\n"));
    f << "###";
    asciiFile.addPos(entry.begin());
    asciiFile.addNote(f.str().c_str());

    return true;
  }
  f << "#";
  // readme, some toolbar, ...
  if (input->tell()!=entry.length())
    asciiFile.addDelimiter(input->tell(),'|');

  asciiFile.addPos(entry.begin());
  asciiFile.addNote(f.str().c_str());
  return true;
}

bool StarObject::readSfxPreview(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxPreview", m_password);
  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(name);
  input->seek(0, librevenge::RVNG_SEEK_SET);
  StarFileManager fileManager;
  if (!fileManager.readSVGDI(zone)) {
    STOFF_DEBUG_MSG(("StarObject::readSfxPreview: can not find the first image\n"));
    input->seek(0, librevenge::RVNG_SEEK_SET);
  }
  if (input->isEnd()) return true;

  long pos=input->tell();
  libstoff::DebugStream f;
  STOFF_DEBUG_MSG(("StarObject::readSfxPreview: find extra data\n"));
  f << "Entries(SfxPreview):###extra";

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  return true;
}

bool StarObject::readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
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
    auto dSz=int(input->readULong(2));
    if (!input->checkPosition(pos+2+dSz)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SfWindows:";
    std::string text("");
    for (int i=0; i<dSz; ++i) text+=char(input->readULong(1));
    f << text;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObject::readSfxWindows: find extra data\n"));
    ascii.addPos(input->tell());
    ascii.addNote("SfWindows:extra###");
  }
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
