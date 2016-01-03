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

#include "STOFFOLEParser.hxx"

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarEncryption.hxx"
#include "StarGraphicStruct.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectSmallText.hxx"
#include "StarZone.hxx"

#include "StarObjectDraw.hxx"

/** Internal: the structures of a StarObjectDraw */
namespace StarObjectDrawInternal
{
////////////////////////////////////////
//! Internal: the state of a StarObjectDraw
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
StarObjectDraw::StarObjectDraw(StarObject const &orig, bool duplicateState) : StarObject::StarObject(orig, duplicateState), m_drawState(new StarObjectDrawInternal::State)
{
}

StarObjectDraw::~StarObjectDraw()
{
  cleanPools();
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool StarObjectDraw::parse()
{
  if (!getOLEDirectory() || !getOLEDirectory()->m_input) {
    STOFF_DEBUG_MSG(("StarObjectDraw::parser: error, incomplete document\n"));
    return false;
  }
  STOFFOLEParser::OleDirectory &directory=*getOLEDirectory();
  StarObject::parse();
  std::vector<std::string> unparsedOLEs=directory.getUnparsedOles();
  size_t numUnparsed = unparsedOLEs.size();
  STOFFInputStreamPtr input=directory.m_input;

  STOFFInputStreamPtr mainOle; // let store the StarDrawDocument to read it in last position
  std::string mainName;
  for (size_t i = 0; i < numUnparsed; i++) {
    std::string const &name = unparsedOLEs[i];
    STOFFInputStreamPtr ole = input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::parse: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    std::string::size_type pos = name.find_last_of('/');
    std::string dir(""), base;
    if (pos == std::string::npos) base = name;
    else if (pos == 0) base = name.substr(1);
    else base = name.substr(pos+1);
    ole->setReadInverted(true);
    if (base=="StarDrawDocument" || base=="StarDrawDocument3") {
      if (mainOle) {
        STOFF_DEBUG_MSG(("StarObjectDraw::parse: find unexpected second draw ole %s\n", name.c_str()));
      }
      else {
        mainOle=ole;
        mainName=name;
        continue;
      }
    }
    if (base=="SfxStyleSheets") {
      readSfxStyleSheets(ole,name);
      continue;
    }
    if (base!="BasicManager2") {
      STOFF_DEBUG_MSG(("StarObjectDraw::parse: find unexpected ole %s\n", name.c_str()));
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    libstoff::DebugStream f;
    f << "Entries(" << base << "):";
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    asciiFile.reset();
  }

  if (!mainOle) {
    STOFF_DEBUG_MSG(("StarObjectDraw::parser: can not find the main draw document\n"));
    return false;
  }
  else
    readDrawDocument(mainOle,mainName);

  return true;
}

bool StarObjectDraw::readDrawDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SCDrawDocument", getPassword()); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  if (!readSdrModel(zone, *this)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument: can not read the main zone\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SCDrawDocument):###");
    return false;
  }
  long pos=input->tell();
  if (!readPresentationData(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SCDrawDocument):###extra");
  }
  return true;
}
catch (...)
{
  return false;
}

bool StarObjectDraw::readPresentationData(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCDrawPres):";
  // sd_drawdoc.xx: operator>>(SdDrawDocument)
  int vers=zone.getHeaderVersion();
  long lastPos=zone.getRecordLastPosition();
  f << "vers=" << vers << ",";
  input->seek(1, librevenge::RVNG_SEEK_CUR); // dummy
  for (int i=0; i<5; ++i) {
    bool bVal;
    *input>>bVal;
    if (!bVal) continue;
    char const *(wh[])= {"pres[all]", "pres[end]", "pres[manual]", "mouse[visible]", "mouse[asPen]"};
    f << wh[i] << ",";
  }
  int firstPage=(int) input->readULong(4);
  if (firstPage!=1) f << "firstPage=" << firstPage << ",";

  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: the zone is too short\n"));
    f<<"###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SCDrawPres");
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  f.str("");
  f << "SCDrawPres:";
  pos=input->tell();
  if (vers>=1) {
    bool hasFrameView;
    *input>>hasFrameView;
    if (hasFrameView && (!readSdrFrameView(zone) || input->tell()>lastPos)) {
      f<<"###frameView";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      zone.closeSCHHeader("SCDrawPres");
      return true;
    }
  }

  pos=input->tell();
  if (vers>=2) { // we need to set the pool here
    StarFileManager fileManager;
    if (!fileManager.readJobSetUp(zone, true)) {
      f<<"###setUp";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      zone.closeSCHHeader("SCDrawPres");
      return true;
    }
  }
  pos=input->tell();
  bool ok=true;
  if (vers>=3) {
    f << "language=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=4) {
    int n=(int) input->readULong(4);
    f << "num[frameView]=" << n << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SCDrawPres:";
    for (int i=0; i<n; ++i) {
      if (!readSdrFrameView(zone) || input->tell()>lastPos) {
        ok=false;
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        break;
      }
    }
    if (ok) pos=input->tell();
  }
  if (ok&&vers>=5) {
    bool startWithNav;
    *input >> startWithNav;
    if (startWithNav) f << "start[withNav],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=6) {
    bool presLockedPage;
    *input >> presLockedPage;
    if (presLockedPage) f << "lock[pages],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=7) {
    bool presOnTop;
    *input >> presOnTop;
    if (presOnTop) f << "pres[onTop],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=8) {
    bool onlineSpell, hideSpell;
    *input >> onlineSpell >> hideSpell;
    if (onlineSpell) f << "spell[online],";
    if (hideSpell) f << "spell[hide],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=9) {
    bool fullScreen;
    *input >> fullScreen;
    if (fullScreen) f << "pres[fullScreen],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=10) {
    std::vector<uint32_t> string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: can not read presPage\n"));
      ok=false;
      f << "###presPage,";
    }
    else if (!string.empty())
      f << libstoff::getString(string).cstr() << ",";
  }
  if (ok&&vers>=11) {
    bool animOk;
    *input >> animOk;
    if (animOk) f << "anim[allowed],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=12) {
    int docType=(int) input->readULong(2);
    if (docType==0) f << "impress,";
    else if (docType==1) f << "draw,";
    else {
      STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: unknown doc type\n"));
      f << "###docType=" << docType << ",";
    }
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=14) {
    bool custShow;
    *input>>custShow;
    if (custShow) f << "customShow,";
    int nCustShow=(int) input->readULong(4);
    if (nCustShow) {
      f << "n[custShow]=" << nCustShow << ",";
      pos=input->tell();
      f.str("");
      f << "SCDrawPres:";
      for (int i=0; i<nCustShow; ++i) {
        if (!readSdrCustomShow(zone) || input->tell()>lastPos) {
          ok=false;
          input->seek(pos, librevenge::RVNG_SEEK_SET);
          break;
        }
      }
      if (ok) pos=input->tell();
    }
  }
  if (ok&&vers>=15) {
    f << "pageNum[type]=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=17) {
    f << "sec[pause]=" << input->readULong(4) << ",";
    bool showLogo;
    *input>>showLogo;
    if (showLogo) f << "show[logo],";
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SCDrawPres:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSCHHeader("SCDrawPres");
  return true;
}

bool StarObjectDraw::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  if (getDocumentKind()!=STOFFDocument::STOFF_K_DRAW) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: called with unexpected document\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SfxStyleSheets)");
    return false;
  }
  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  shared_ptr<StarItemPool> pool=getNewItemPool(StarItemPool::T_XOutdevPool);
  pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));
  shared_ptr<StarItemPool> mainPool=pool;
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
        STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: create extra pool of type %d\n", (int) pool->getType()));
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
    STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: can not read a style pool\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SfxStyleSheets):###extra");
  }
  return true;
}

////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////

bool StarObjectDraw::readSdrLayer(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrLy") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrLayerDef)[" << zone.getRecordLevel() << "]:";
  // svdlayer.cxx operator>>(..., SdrLayer &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrLayer: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLy") {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrLayer: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }

  uint8_t layerId;
  *input>>layerId;
  if (layerId) f << "layerId=" << int(layerId) << ",";
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrLayer: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrLayerDef");
    return true;
  }
  f << libstoff::getString(string).cstr() << ",";
  if (version>=1) {
    uint16_t nType;
    *input>>nType;
    if (nType==0) f << "userLayer,";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrLayerDef");
  return true;
}

bool StarObjectDraw::readSdrLayerSet(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrLS") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrLayerSet)[" << zone.getRecordLevel() << "]:";
  // svdlayer.cxx operator>>(..., SdrLayerSet &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrLayerSet: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLS") {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrLayerSet: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  for (int i=0; i<2; ++i) { // checkme: set of 32 bytes ?
    f << (i==0 ? "member" : "exclude") << "=[";
    for (int j=0; j<32; ++j) {
      int c=(int) input->readULong(1);
      if (c)
        f << c << ",";
      else
        f << ",";
    }
    f << "],";
  }
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrLayerSet: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrLayerSet");
    return true;
  }
  f << libstoff::getString(string).cstr() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrLayerSet");
  return true;
}

bool StarObjectDraw::readSdrModel(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrMd") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrModel)[" << zone.getRecordLevel() << "]:";
  // svdmodel.cxx operator>>(..., SdrModel &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrMd" || version>17) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();

  f.str("");
  f << "SdrModelA[" << zone.getRecordLevel() << "]:";
  // SdrModel::ReadData
  if (!zone.openRecord()) { // SdrModelA1
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: can not open downCompat\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrModel");
    return true;
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f.str("");
  f << "SdrModelA[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  if (version>=11) {
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    f << "joeM";
    if (magic!="JoeM" || !zone.openRecord()) { // SdrModelA2
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: unexpected magic2 data\n"));
      f << "###magic" << magic;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrModelA1");
      zone.closeSDRHeader("SdrModel");
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SdrModelInfo[" << zone.getRecordLevel() << "]:";
    if (!zone.openRecord()) { // SdrModelInfo
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: can not read model info\n"));
      f << "###";
      ok=false;
    }
    else {
      // operator>>(..., SdrModelInfo&)
      uint32_t date, time;
      uint8_t enc;
      *input >> date >> time >> enc;
      f << "date=" << date << ",time=" << time << ",encoding=" << int(enc) << ",";
      input->seek(3, librevenge::RVNG_SEEK_CUR); // creationGUI, creationCPU, creationSys
      *input >> date >> time >> enc;
      f << "modDate=" << date << ", modTime=" << time << ",encoding[mod]=" << int(enc) << ",";
      input->seek(3, librevenge::RVNG_SEEK_CUR); // writeGUI, writeCPU, writeSys
      *input >> date >> time >> enc;
      f << "lastReadDate=" << date << ", lastReadTime=" << time << ",encoding[lastRead]=" << int(enc) << ",";
      input->seek(3, librevenge::RVNG_SEEK_CUR); // lastReadGUI, lastReadCPU, lastReadSys
      *input >> date >> time;
      f << "lastPrintDate=" << date << ", lastPrintTime=" << time << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeRecord("SdrModelInfo");
      pos=input->tell();
      f.str("");
      f << "SdrModelStats[" << zone.getRecordLevel() << "]:";
      if (!zone.openRecord()) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: can not read model statistic\n"));
        f << "###";
        ok=false;
      }
      else {
        zone.closeRecord("SdrModelStat");
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        pos=input->tell();
        f.str("");
        f << "SdrModelFormatCompat[" << zone.getRecordLevel() << "]:";
      }
    }
    if (ok&&!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: can not read model format compat\n"));
      f << "###";
      ok=false;
    }
    else if (ok) {
      if (input->tell()+6<=zone.getRecordLastPosition()) {
        int16_t nTmp; // find 0
        *input >> nTmp;
        if (nTmp) f << "unk=" << nTmp << ",";
      }
      if (input->tell()+4<=zone.getRecordLastPosition()) {
        f << "nStreamCompressed=" << input->readULong(2) << ",";
        f << "nStreamNumberFormat=" << input->readULong(2) << ",";
      }
      zone.closeRecord("SdrModelFormatCompat");
    }

    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    pos=input->tell();
    f.str("");
    f << "SdrModelA[" << zone.getRecordLevel() << "]-data:";
  }
  if (ok) {
    int32_t nNum, nDen;
    uint16_t nTmp;
    *input>> nNum >> nDen >> nTmp;
    f << "objectUnit=" << nNum << "/" << nDen << ",";
    if (nTmp) f << "eObjUnit=" << nTmp << ",";
    *input>>nTmp;
    if (nTmp==1) f << "pageNotValid,";
    input->seek(2, librevenge::RVNG_SEEK_CUR); // ???, reserved

    if (version<1) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: reading model format for version 0 is not implemented\n"));
      f << "###version0";
      ok=false;
    }
    else {
      if (version<11) f << "nCharSet=" << input->readLong(2) << ",";
      std::vector<uint32_t> string;
      for (int i=0; i<6; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: can not read a string\n"));
          f << "###string";
          ok=false;
          break;
        }
        if (string.empty()) continue;
        static char const *(wh[])= {"cTableName", "dashName", "lineEndName", "hashName", "gradientName", "bitmapName"};
        f << wh[i] << "=" << libstoff::getString(string).cstr() << ",";
      }
    }
  }
  if (ok && version>=12 && input->tell()<zone.getRecordLastPosition()) {
    int32_t nNum, nDen;
    uint16_t nTmp;
    *input>> nNum >> nDen >> nTmp;
    f << "UIUnit=" << nNum << "/" << nDen << ",";
    if (nTmp) f << "eUIUnit=" << nTmp << ",";
  }
  if (ok && version>=13 && input->tell()<zone.getRecordLastPosition()) {
    int32_t nNum1, nNum2;
    *input>> nNum1 >> nNum2;
    if (nNum1) f << "nDefTextHgt=" << nNum1 << ",";
    if (nNum2) f << "nDefTab=" << nNum2 << ",";
  }
  if (ok && version>=14 && input->tell()<zone.getRecordLastPosition()) {
    uint16_t pNum;
    *input>> pNum;
    if (pNum) f << "nStartPageNum=" << pNum << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (version>=11) {
    zone.closeRecord("SdrModelA2");
    ok=true;
  }

  while (ok) {
    pos=input->tell();
    if (pos+4>zone.getRecordLastPosition())
      break;
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrLy" && readSdrLayer(zone)) continue;
    if (magic=="DrLS" && readSdrLayerSet(zone)) continue;
    if ((magic=="DrPg" || magic=="DrMP") && readSdrPage(zone, doc)) continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSDRHeader(magic)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SdrModel[" << magic << "-" << zone.getRecordLevel() << "]:";
    if (magic!="DrXX") {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrModel: find unexpected child\n"));
      f << "###type";
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrModel");
  }

  zone.closeRecord("SdrModelA1");
  // in DrawingLayer, find also 0500000001 and 060000000[01]00
  pos=input->tell();
  if (pos+4<=zone.getRecordLastPosition()) {
    long sz=(long) input->readULong(4);
    if (pos+sz==zone.getRecordLastPosition()) {
      ascFile.addPos(pos);
      ascFile.addNote("SdrModel:#extra");
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    else
      input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrModel");
  return true;
}

bool StarObjectDraw::readSdrObject(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrOb" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrObject)[" << zone.getRecordLevel() << "]:";
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  if (lastPos==input->tell()) {
    zone.closeSDRHeader("SdrObject");
    return true;
  }
  // svdobj.cxx SdrObjFactory::MakeNewObject
  pos=input->tell();
  f.str("");
  f << "SdrObject:";
  magic="";
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  uint16_t identifier;
  *input>>identifier;
  f << magic << ", ident=" << std::hex << identifier << std::dec << ",";
  if (magic=="SVDr") {
    if (!readSVDRObject(zone, doc, (int) identifier)) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: can not read SVDr object\n"));
      f << "###";
    }
    else {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      if (pos==lastPos) {
        zone.closeSDRHeader("SdrObject");
        return true;
      }
      f.str("");
      f << "SVDR:##extra";
      static bool first=true;
      if (first) { // see SdrObjFactory::MakeNewObject
        STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: read SVDr object, find extra data\n"));
        first=false;
      }
      f << "##";
    }
  }
  else if (magic=="SCHU") {
    if (!readSCHUObject(zone, (int) identifier, doc)) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: can not read SCHU object\n"));
      f << "###";
    }
    else {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      if (pos==lastPos) {
        zone.closeSDRHeader("SdrObject");
        return true;
      }
      f.str("");
      f << "SdrObject:##extra";
      static bool first=true;
      if (first) { // see SdrObjFactory::MakeNewObject
        STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: read Schu object, find extra data\n"));
        first=false;
      }
      f << "##";
    }
  }
  else if (magic=="FM01") { // FmFormInventor
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: read FM01 object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: find unknown magic\n"));
    f << "###";
  }
  if (pos!=input->tell())
    ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeSDRHeader("SdrObject");
  return true;
}

bool StarObjectDraw::readSdrPage(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrPg" && magic!="DrMP") return false;

  // svdpage.cxx operator>>
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrPageDef)[" << zone.getRecordLevel() << "]:";
  if (magic=="DrMP") f << "master,";
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrPg" && magic!="DrMP") {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();

  f.str("");
  f << "SdrPageDefA[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx SdrPage::ReadData
  bool hasExtraData=false;
  for (int tr=0; tr<2; ++tr) {
    long lastPos=zone.getRecordLastPosition();
    if (!zone.openRecord()) { // SdrPageDefA1
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: can not open downCompat\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSDRHeader("SdrPageDef");
      return true;
    }
    if (tr==1 || zone.getRecordLastPosition()==lastPos)
      break;
    // unsure, how we can have some dim and pool here but seems frequent in DrawingLayer
    uint16_t n;
    *input >> n;
    bool ok=false;
    if (n>1) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: bad version\n"));
      f << "###n=" << n << ",";
      ok=false;
    }
    else
      hasExtraData=true;
    if (n==1 && input->tell()+8<=zone.getRecordLastPosition())  {
      f << "dim=[";
      for (int i=0; i<4; ++i) f << input->readLong(2) << ((i%2) ? "," : "x");
      f << "],";
      *input >> n;
      ok=true;
    }
    if (ok && n>1 && input->tell()+2<=zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: bad version2\n"));
      f << "###n2=" << n << ",";
      ok=false;
    }
    if (ok && n==1) {
      long actPos=input->tell();
      shared_ptr<StarItemPool> pool=doc.getNewItemPool(StarItemPool::T_VCControlPool);
      if (!pool || !pool->read(zone))
        input->seek(actPos, librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    zone.closeRecord("SdrPageDef");
    pos=input->tell();
    f.str("");
    f << "SdrPageDefA[" << zone.getRecordLevel() << "]:";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f.str("");
  f << "SdrPageDefA[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  if (version>=11) {
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    f << "joeM";
    if (magic!="JoeM" || !zone.openRecord()) { // SdrPageDefA2
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: unexpected magic2 data\n"));
      f << "###magic" << magic;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrPageDefA1");
      zone.closeSDRHeader("SdrPageDef");
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SdrPageDefData[" << zone.getRecordLevel() << "]:";
  }
  if (ok) {
    int32_t nWdt, nHgt, nBordLft, nBordUp, nBordRgt, nBordLwr;
    uint16_t n;
    *input >> nWdt >> nHgt >> nBordLft >> nBordUp >> nBordRgt >> nBordLwr >> n;
    f << "dim=" << nWdt << "x" << nHgt << ",";
    f << "border=[" << nBordLft << "x" << nBordUp << "-" << nBordRgt << "x" << nBordLwr << "],";
    if (n) f << "n=" << n << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (version>=11) {
    zone.closeRecord("SdrPageDefData");
    ok=true;
  }

  long lastPos=zone.getRecordLastPosition();
  while (ok) {
    pos=input->tell();
    if (pos+4>lastPos)
      break;
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrLy" && readSdrLayer(zone)) continue;
    if (magic=="DrLS" && readSdrLayerSet(zone)) continue;
    if (magic=="DrMP" && readSdrMPageDesc(zone)) continue;
    if (magic=="DrML" && readSdrMPageDescList(zone)) continue;
    break;
  }
  if (ok && version==0) {
    pos=input->tell();
    f.str("");
    f << "SdrPageDef[masterPage]:";
    uint16_t n;
    *input >> n;
    if (pos+2+2*n>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: n is bad\n"));
      f << "###n=" << n << ",";
      ok=false;
    }
    else {
      f << "pages=[";
      for (int i=0; i<int(n); ++i) f << input->readULong(2) << ",";
      f << "],";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  // SdrObjList::Load
  while (ok) {
    pos=input->tell();
    if (pos+4>lastPos)
      break;
    if (readSdrObject(zone, doc)) continue;

    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrXX" && zone.openSDRHeader(magic)) {
      f.str("");
      f << "SdrPageDef[DrXX-" << zone.getRecordLevel() << "]:";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSDRHeader("SdrPageDef");
      break;
    }
    ok=false;
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: can not find end object zone\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (ok && version>=16) {
    pos=input->tell();
    f.str("");
    f << "SdrPageDef[background-" << zone.getRecordLevel() << "]:";
    bool background;
    *input>>background;
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (background) {
      if (!readSdrObject(zone, doc)) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: can not find backgroound object\n"));
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        ok=false;
      }
    }
  }
  zone.closeRecord("SdrPageDefA1");
  if (ok && hasExtraData) {
    lastPos=zone.getRecordLastPosition();
    int n=0;;
    while (input->tell()<lastPos) {
      pos=input->tell();
      if (!zone.openRecord()) break;
      f.str("");
      f << "SdrPageDefB" << ++n << "[" << zone.getRecordLevel() << "]:";
      if (n==1) {
        std::vector<uint32_t> string;
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: can not find read a string\n"));
          f << "###string";
        }
        else // Controls
          f << libstoff::getString(string).cstr() << ",";
        // then if vers>={13|14|15|16}  671905111105671901000800000000000000
      }
      else if (n==2 && pos+12==zone.getRecordLastPosition()) {
        f << "empty,";
        for (int i=0; i<4; ++i) { // always 0
          int val=(int) input->readLong(2);
          if (val) f << "f" << i << "=" << val << ",";
        }
      }
      else if (!readSdrPageUnknownZone1(zone, zone.getRecordLastPosition())) {
        input->seek(pos+4, librevenge::RVNG_SEEK_SET);
        STOFF_DEBUG_MSG(("StarObjectDraw::readSdrPage: find some unknown zone\n"));
        f << "###unknown";
      }
      // for version>=16?, find another zone: either an empty zone or a zone which contains many string...
      if (zone.getRecordLastPosition()!=input->tell()) {
        ascFile.addDelimiter(input->tell(),'|');
        f << "#";
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrPageDefB");
    }
  }

  zone.closeSDRHeader("SdrPageDef");
  return true;
}

bool StarObjectDraw::readSdrPageUnknownZone1(StarZone &zone, long lastPos)
{
  // checkme: probably as draw pres: schheader: first a schheader, then 3 bool
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell()-4;
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrPageUnkn1)[" << zone.getRecordLevel() << "]:";
  if (pos+28>lastPos) return false;
  int val=(int) input->readULong(2);
  if (val!=3 && val!=7)
    return false;
  f << "f0=" << val << ",";
  for (int i=0; i<3; ++i) { // 1
    val=(int) input->readULong(1);
    if (val!=1) f << "f" << i+1 << "=" << val << ",";
  }
  for (int i=0; i<5; ++i) { // g0=14|15|19, g1=1, g3=0-10,
    val=(int) input->readLong(2);
    if (val) f << "g" << i << "=" << val << ",";
  }
  val=(int) input->readLong(1); // always 1
  if (val!=1) f << "f6=" << val << ",";
  for (int i=0; i<3; ++i) { // g5=1|1e
    val=(int) input->readLong(2);
    if (val) f << "g" << i+5 << "=" << val << ",";
  }
  std::vector<uint32_t> string;
  if (!zone.readString(string) || input->tell()>lastPos)
    return false;
  f << libstoff::getString(string).cstr() << ",";
  int n=(int) input->readULong(4);
  if (input->tell()+8*n>lastPos)
    return false;
  f << "unk=[";
  for (int i=0; i<n; ++i) {
    f << "[";
    for (int j=0; j<4; ++j) {
      val=(int) input->readLong(2);
      if (val)
        f << val << ",";
      else
        f << "_,";
    }
    f << "],";
  }
  f << "],";
  int n1=int(lastPos-input->tell())/2;
  f << "unkn1=[";
  for (int i=0; i<n1; ++i) {
    val=(int) input->readLong(2);
    if (val)
      f << val << ",";
    else
      f << "_,";
  }
  f << "],";
  if (lastPos>input->tell()) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectDraw::readSdrMPageDesc(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrMD") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrMPageDesc)[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx operator>>(..., SdrMaterPageDescriptor &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrMPageDesc: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  uint16_t nPageNum;
  *input >> nPageNum;
  f << "pageNum=" << nPageNum << ",";
  f << "aVisLayer=[";
  for (int i=0; i<32; ++i) {
    int c=(int) input->readULong(1);
    if (c)
      f << c << ",";
    else
      f << ",";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrMPageDesc");
  return true;
}

bool StarObjectDraw::readSdrMPageDescList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrML") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrMPageList)[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx operator>>(..., SdrMaterPageDescListriptor &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrMPageDescList: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  uint16_t n;
  *input>>n;
  f << magic << ",nVers=" << version << ",N=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<n; ++i) {
    pos=input->tell();
    if (pos<lastPos && readSdrMPageDesc(zone)) continue;
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrMPageDescList: can not find a page desc\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SdrMPageList:###pageDesc");
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSDRHeader("SdrMPageList");
  return true;
}

bool StarObjectDraw::readSdrCustomShow(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrCustomShow)[" << zone.getRecordLevel() << "]:";
  int vers=zone.getHeaderVersion();
  f << "vers=" << vers << ",";

  long lastPos=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  if (!zone.readString(string) || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrCustomShow: can not read the name\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SdrCustomShow");
    return true;
  }
  f << libstoff::getString(string).cstr() << ",";
  long n=(int) input->readULong(4);
  f << "N=" << n << ",";
  if (input->tell()+2*n>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrCustomShow: the number of page seems bad\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SdrCustomShow");
    return true;
  }
  f << "pages=[";
  for (long p=0; p<n; ++p)
    f << input->readULong(2) << ",";
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrCustomShow: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrCustomShow:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSCHHeader("SdrCustomShow");
  return true;
}

bool StarObjectDraw::readSdrHelpLine(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrHl" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrHelpLine)[" << zone.getRecordLevel() << "]:set,";
  //svx_svdhlpln.cxx operator>>(SdrHelpLine)
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  long lastPos=zone.getRecordLastPosition();
  int val=(int) input->readULong(2);
  if (val) f << "kind=" << val << ",";
  int dim[2];
  for (int i=0; i<2; ++i) dim[i]=(int) input->readLong(4);
  f << "pos=" << STOFFVec2i(dim[0],dim[1]) << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrHelpLine: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrHelpLine:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrHelpLine");
  return true;
}

bool StarObjectDraw::readSdrHelpLineSet(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrHL" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrHelpLine)[" << zone.getRecordLevel() << "]:set,";
  //svx_svdhlpln.cxx operator>>(SdrHelpLineList)
  long lastPos=zone.getRecordLastPosition();
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  int n=int(input->readULong(2));
  if (n) f << "N=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int i=0; i<n; ++i) {
    pos=input->tell();
    if (!readSdrHelpLine(zone) || input->tell()>lastPos) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrHelpLineSet: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrHelpLine:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrHelpLine");
  return true;
}

bool StarObjectDraw::readSdrFrameView(StarZone &zone)
{
  //sd_frmview.cxx operator>>(FrameView)
  if (!readSdrView(zone))
    return false;

  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrFrameView)[" << zone.getRecordLevel() << "]:";
  int vers=zone.getHeaderVersion();
  f << "vers=" << vers << ",";

  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+1+3*32>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrFrameView: the zone seems too short\n"));
    f << "###short";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SdrFrameView");
  }
  if (input->readLong(1)) f << "ruler,";
  for (int i=0; i<3; ++i) {
    char const *(wh[])= {"visible","printtable","helpLine"};
    f << "layers[" << wh[i] << "]=[";
    int prevFl=-1, numSame=0;
    for (int j=0; j<=32; ++j) {
      int fl=j==32 ? -1 : (int) input->readULong(1);
      if (fl==prevFl) {
        ++numSame;
        continue;
      }
      if (numSame) {
        if (prevFl==0)
          f << "_";
        else if (prevFl==255)
          f << "*";
        else
          f << std::hex << prevFl << std::dec;
        if (numSame>1)
          f << "x" << numSame;
        f << ",";
      }
      prevFl=fl;
      numSame=1;
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  bool ok=true;
  if (!readSdrHelpLineSet(zone)) { // standart line
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    ok=false;
  }
  if (ok&&vers>=1) {
    // notes and handout line
    for (int i=0; i<2; ++i) {
      if (readSdrHelpLineSet(zone) && input->tell()<=lastPos)
        continue;
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
  }
  if (ok) pos=input->tell();
  f.str("");
  f << "SdrFrameView:";
  if (ok&&vers>=2) {
    bool noColors, noAttribs;
    *input>>noColors >> noAttribs;
    if (noColors) f << "no[color],";
    if (noAttribs) f << "no[attrib],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=3) {
    int dim[4];
    for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
    f << "vis[area]=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
    f << "page[kind]=" << input->readULong(4) << ",";
    f << "page[select]=" << input->readULong(2) << ",";
    f << "page[editMode]=" << input->readULong(4) << ",";
    bool layerMode;
    *input>>layerMode;
    if (layerMode) f << "layer[mode],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=4) {
    bool quickEdit;
    *input>>quickEdit;
    if (quickEdit) f << "edit[quick],";
  }
  if (ok&&vers>=5) {
    bool dragWithCopy;
    *input>>dragWithCopy;
    if (dragWithCopy) f << "drag[withCopy],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=6)
    f << "slides[byRow]="<< input->readULong(2) << ",";
  if (ok&&vers>=7) {
    bool bigHandles, clickTextEdit, clickChangeRot;
    *input >> bigHandles >> clickTextEdit >> clickChangeRot;
    if (bigHandles) f << "handles[big],";
    if (clickTextEdit) f << "click[textEdit],";
    if (clickChangeRot) f << "click[changeRotation],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=8) {
    f << "notes[editMode]=" << input->readULong(4) << ",";
    f << "notes[handoutMode]=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=9) {
    f << "draw[mode]=" << input->readULong(4) << ",";
    f << "draw[preview,mode]=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=10) {
    bool inPageMode, inMasterPageMode;
    *input >> inPageMode >> inMasterPageMode;
    if (inPageMode) f << "showPreview[pageMode],";
    if (inMasterPageMode) f << "showPreview[masterPageMode],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=11) {
    bool inOutlineMode;
    *input >> inOutlineMode;
    if (inOutlineMode) f << "showPreview[outlineMode],";
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrFrameView: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrFrameView:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSCHHeader("SdrFrameView");
  return true;
}

bool StarObjectDraw::readSdrView(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrVw" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrView)[" << zone.getRecordLevel() << "]:";
  //svx_svdview.cxx operator>>(SdrView)
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  int n=0;
  while (input->tell()<lastPos) {
    // svx_svdio.cxx: SdrNamedSubRecord::SdrNamedSubRecord
    pos=input->tell();
    if (!zone.openRecord()) break;
    f.str("");
    f << "SdrViewA" << ++n << "[" << zone.getRecordLevel() << "]:";
    if (pos+10>zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrView: sub named record seems too short\n"));
      f << "###short";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrView");
      continue;
    }
    std::string inventor("");
    for (int i=0; i<4; ++i) inventor+=(char) input->readULong(1);
    f << inventor << "-" << input->readULong(2) << ",";
    if (input->tell()!=zone.getRecordLastPosition()) {
      ascFile.addDelimiter(input->tell(),'|');
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord("SdrView");
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrView: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrView:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrView");
  return true;
}

////////////////////////////////////////////////////////////
//  object
////////////////////////////////////////////////////////////
bool StarObjectDraw::readSVDRObject(StarZone &zone, StarObject &doc, int identifier)
{
  STOFFInputStreamPtr input=zone.input();
  long pos;

  long endPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  bool ok=true;
  char const *(wh[])= {"none", "group", "line", "rect", "circle",
                       "sector", "arc", "ccut", "poly", "polyline",
                       "pathline", "pathfill", "freeline", "freefill", "splineline",
                       "splinefill", "text", "textextended", "fittext", "fitalltext",
                       "titletext", "outlinetext", "graf", "ole2", "edge",
                       "caption", "pathpoly", "pathline", "page", "measure",
                       "dummy","frame", "uno"
                      };
  switch (identifier) {
  case 1: // group
    ok=readSVDRObjectGroup(zone, doc);
    break;
  case 2: // line
  case 8: // poly
  case 9: // polyline
  case 10: // pathline
  case 11: // pathfill
  case 12: // freeline
  case 13: // freefill
  case 26: // pathpoly
  case 27: // pathline
    ok=readSVDRObjectPath(zone, doc, identifier);
    break;
  case 4: // circle
  case 5: // sector
  case 6: // arc
  case 7: // cut
    ok=readSVDRObjectCircle(zone, doc, identifier);
    break;
  case 3: // rect
  case 16: // text
  case 17: // textextended
  case 20: // title text
  case 21: // outline text
    ok=readSVDRObjectRect(zone, doc, identifier);
    break;
  case 24: // edge
    ok=readSVDRObjectEdge(zone, doc);
    break;
  case 22: // graph
    ok=readSVDRObjectGraph(zone, doc);
    break;
  case 23: // ole
  case 31: // frame
    ok=readSVDRObjectOLE(zone, doc, identifier);
    break;
  case 25: // caption
    ok=readSVDRObjectCaption(zone, doc);
    break;
  case 28: { // page
    ok=readSVDRObjectHeader(zone, doc);
    if (!ok) break;
    pos=input->tell();
    if (!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObject: can not open page record\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
    f << "SVDR[page]:page=" << input->readULong(2) << ",";
    ok=input->tell()<=zone.getRecordLastPosition();
    if (!ok)
      f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord("SVDR");
    break;
  }
  case 29: // measure
    ok=readSVDRObjectMeasure(zone, doc);
    break;
  case 32: { // uno
    ok=readSVDRObjectRect(zone, doc, identifier);
    pos=input->tell();
    if (!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObject: can not open uno record\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
    f << "SVDR[uno]:";
    // + SdrUnoObj::ReadData (checkme)
    std::vector<uint32_t> string;
    if (input->tell()!=zone.getRecordLastPosition() && (!zone.readString(string) || input->tell()>zone.getRecordLastPosition())) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectAttrib: can not read uno string\n"));
      f << "###uno";
      ok=false;
    }
    else if (!string.empty())
      f << libstoff::getString(string).cstr() << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord("SVDR");
    break;
  }
  default:
    ok=readSVDRObjectHeader(zone, doc);
    break;
  }
  pos=input->tell();
  if (!ok) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObject: can not read some zone\n"));
    ascFile.addPos(pos);
    ascFile.addNote("Entries(SVDR):###");
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  if (input->tell()==endPos)
    return true;
  static bool first=true;
  if (first) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObject: find unexpected data\n"));
  }
  if (identifier<=0 || identifier>32) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObject: unknown identifier\n"));
    ascFile.addPos(pos);
    ascFile.addNote("Entries(SVDR):###");
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    return true;
  }

  while (input->tell()<endPos) {
    pos=input->tell();
    f.str("");
    f << "SVDR:" << wh[identifier] << ",###unknown,";
    if (!zone.openRecord())
      return true;
    long lastPos=zone.getRecordLastPosition();
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeRecord("SVDR");
  }
  return true;
}

bool StarObjectDraw::readSVDRObjectAttrib(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!readSVDRObjectHeader(zone, doc)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:attrib,";

  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectAttrib: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  long lastPos=zone.getRecordLastPosition();
  shared_ptr<StarItemPool> pool=doc.findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool)
    pool=doc.getNewItemPool(StarItemPool::T_VCControlPool);
  int vers=zone.getHeaderVersion();
  // svx_svdoattr: SdrAttrObj:ReadData
  bool ok=true;
  for (int i=0; i<6; ++i) {
    if (vers<11) input->seek(2, librevenge::RVNG_SEEK_CUR);
    uint16_t const(what[])= {1017/*XATTRSET_LINE*/, 1047/*XATTRSET_FILL*/, 1066/*XATTRSET_TEXT*/, 1079/*SDRATTRSET_SHADOW*/,
                             1096 /*SDRATTRSET_OUTLINER*/, 1126 /*SDRATTRSET_MISC*/
                            };
    uint16_t nWhich=what[i];
    shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos) {
      f << "###";
      ok=false;
      break;
    }
    else if (item->m_attribute)
      item->m_attribute->print(f);
    if (vers<5 && i==3) break;
    if (vers<6 && i==4) break;
  }
  std::vector<uint32_t> string;
  if (ok && (!zone.readString(string) || input->tell()>lastPos)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectAttrib: can not read the sheet style name\n"));
    ok=false;
  }
  else if (!string.empty()) {
    f << libstoff::getString(string).cstr() << ",";
    f << "eFamily=" << input->readULong(2) << ",";
    if (vers>0 && vers<11) // in this case, we must convert the style name
      f << "charSet=" << input->readULong(2) << ",";
  }
  if (ok && vers==9 && input->tell()+2==lastPos) // probably a charset even when string.empty()
    f << "#charSet?=" << input->readULong(2) << ",";
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectAttrib: find extra data\n"));
      f << "###extra,vers=" << vers;
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectDraw::readSVDRObjectCaption(StarZone &zone, StarObject &doc)
{
  if (!readSVDRObjectRect(zone, doc, 25))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:caption,";
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectCaption: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  // svx_svdocapt.cxx SdrCaptionObj::ReadData
  bool ok=true;
  uint16_t n;
  *input >> n;
  if (input->tell()+8*n>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectCaption: the number of point seems bad\n"));
    f << "###n=" << n << ",";
    ok=false;
    n=0;
  }
  f << "poly=[";
  for (int pt=0; pt<int(n); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
  f << "],";
  if (ok) {
    shared_ptr<StarItemPool> pool=doc.findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=doc.getNewItemPool(StarItemPool::T_XOutdevPool);
    uint16_t nWhich=1195; // SDRATTRSET_CAPTION
    shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos) {
      f << "###";
    }
    else if (item->m_attribute)
      item->m_attribute->print(f);
  }
  if (!ok) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectCircle(StarZone &zone, StarObject &doc, int id)
{
  if (!readSVDRObjectRect(zone, doc, id))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  f << (id==4 ? "circle" : id==5 ? "sector" : id==6 ? "arc" : "cut") << ",";
  // svx_svdocirc SdrCircObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectCircle: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  if (id!=4)
    f << "angles=" << float(input->readLong(4))/100.f << "x" << float(input->readLong(4))/100.f << ",";
  if (input->tell()!=lastPos) {
    shared_ptr<StarItemPool> pool=doc.findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=doc.getNewItemPool(StarItemPool::T_XOutdevPool);
    uint16_t nWhich=1179; // SDRATTRSET_CIRC
    shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos) {
      f << "###";
    }
    else if (item->m_attribute)
      item->m_attribute->print(f);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectEdge(StarZone &zone, StarObject &doc)
{
  if (!readSVDRObjectText(zone, doc))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:edge,";
  // svx_svdoedge SdrEdgeObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectEdge: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  bool ok=true;
  if (vers<2) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectEdge: unexpected version\n"));
    f << "##badVers,";
    ok=false;
  }

  bool openRec=false;
  if (ok && vers>=11) {
    openRec=zone.openRecord();
    if (!openRec) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectEdge: can not edgeTrack record\n"));
      f << "###record";
      ok=false;
    }
  }
  if (ok) {
    uint16_t n;
    *input >> n;
    if (input->tell()+9*n>zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectEdge: the number of point seems bad\n"));
      f << "###n=" << n << ",";
      ok=false;
    }
    else {
      f << "poly=[";
      for (int pt=0; pt<int(n); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "],";
      f << "polyFlag=[";
      for (int pt=0; pt<int(n); ++pt) f << input->readULong(1) << ",";
      f << "],";
    }
  }
  if (openRec) {
    if (!ok) input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeRecord("SVDR");
  }
  if (ok && input->tell()<lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    f.str("");
    f << "SVDR[edgeB]:";
    pos=input->tell();

    for (int i=0; i<2; ++i) {
      if (!readSDRObjectConnection(zone)) {
        f << "##connector,";
        ok=false;
        break;
      }
      pos=input->tell();
    }
  }
  if (ok && input->tell()<lastPos) {
    shared_ptr<StarItemPool> pool=doc.findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=doc.getNewItemPool(StarItemPool::T_XOutdevPool);
    uint16_t nWhich=1146; // SDRATTRSET_EDGE
    shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos) {
      f << "###";
    }
    else if (item->m_attribute)
      item->m_attribute->print(f);
  }
  if (ok && input->tell()<lastPos) {
    // svx_svdoedge.cxx SdrEdgeInfoRec operator>>
    if (input->tell()+5*8+2*4+3*2+1>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectEdge: SdrEdgeInfoRec seems too short\n"));
      ok=false;
    }
    else {
      f << "infoRec=[";
      for (int i=0; i<5; ++i)
        f << "pt" << i << "=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "angles=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      for (int i=0; i<3; ++i) f << "n" << i << "=" << input->readULong(2) << ",";
      f << "orthoForm=" << input->readULong(1) << ",";
      f << "],";
    }
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectEdge: find extra data\n"));
      f << "###extra,vers=" << vers;
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectHeader(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SVDR)[" << zone.getRecordLevel() << "]:header,";

  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectHeader: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  // svx_svdobj: SdrObject::ReadData
  int dim[4];    // gen.cxx operator>>(Rect) : test compression here
  for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
  f << "bdbox=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
  f << "layer[id]=" << input->readULong(2) << ",";
  for (int i=0; i<2; ++i) dim[i]=(int) input->readLong(4);
  if (dim[0] || dim[1])
    f << "anchor=" << STOFFVec2i(dim[0],dim[1]) << ",";
  bool movProt, sizProt, noPrint, markProt, emptyObj;
  *input >> movProt >> sizProt >> noPrint >> markProt >> emptyObj;
  if (movProt) f << "move[protected],";
  if (sizProt) f << "size[protected],";
  if (noPrint) f << "print[no],";
  if (markProt) f << "mark[protected],";
  if (emptyObj) f << "empty,";
  if (vers>=4) {
    bool notVisibleAsMaster;
    *input >> notVisibleAsMaster;
    if (notVisibleAsMaster) f << "notVisible[asMaster],";
  }
  bool ok=true;
  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectHeader: oops read to much data\n"));
    f << "###bad,";
    ok=false;
  }
  if (ok && vers<11) {
    // poly.cxx operator>>(Polygon) : test compression here
    uint16_t n;
    *input >> n;
    if (input->tell()+8*n>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectHeader: the number of point seems bad\n"));
      f << "###n=" << n << ",";
      ok=false;
      n=0;
    }
    f << "poly=[";
    for (int pt=0; pt<int(n); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
    f << "],";
  }
  if (ok && vers>=11) {
    bool bTmp;
    *input >> bTmp;
    if (bTmp) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
      f.str("");
      f << "SVDR[headerB]:";
      if (!readSDRGluePointList(zone)) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectHeader: can not find the gluePoints record\n"));
        f << "###gluePoint";
        ok=false;
      }
      else
        pos=input->tell();
    }
  }
  if (ok) {
    bool readUser=true;
    if (vers>=11) *input >> readUser;
    if (readUser) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
      f.str("");
      f << "SVDR[headerC]:";
      if (!readSDRUserDataList(zone, doc, vers>=11)) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectHeader: can not find the data list record\n"));
        f << "###dataList";
      }
      else
        pos=input->tell();
    }
  }

  if (input->tell()!=pos) {
    if (input->tell()!=lastPos)
      ascFile.addDelimiter(input->tell(),'|');
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectDraw::readSVDRObjectGraph(StarZone &zone, StarObject &doc)
{
  if (!readSVDRObjectRect(zone, doc, 22))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:graph,";
  // SdrGrafObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  bool ok=true;
  if (vers<11) {
    // ReadDataTilV10
    StarGraphicStruct::StarGraphic graphic;
    if (!graphic.read(zone) || input->tell()>lastPos) {
      f << "###graphic";
      ok=false;
    }
    if (ok && vers>=6) {
      int dim[4];
      for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
      f << "rect=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
    }
    if (ok && vers>=8) {
      std::vector<uint32_t> string;
      if (!zone.readString(string) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not read the file name\n"));
        f << "###fileName";
        ok=false;
      }
      else if (!string.empty())
        f << "fileName=" << libstoff::getString(string).cstr() << ",";
    }
    if (ok && vers>=9) {
      std::vector<uint32_t> string;
      if (!zone.readString(string) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not read the filter name\n"));
        f << "###filter";
        ok=false;
      }
      else if (!string.empty())
        f << "filterName=" << libstoff::getString(string).cstr() << ",";
    }
  }
  else {
    bool hasGraphic;
    *input >> hasGraphic;
    if (hasGraphic) {
      if (!zone.openRecord()) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not open graphic record\n"));
        f << "###graphRecord";
        ok=false;
      }
      else {
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        StarGraphicStruct::StarGraphic graphic;
        if (!graphic.read(zone, zone.getRecordLastPosition()) || input->tell()>zone.getRecordLastPosition()) {
          ascFile.addPos(pos);
          ascFile.addNote("SVDR[graph]:##graphic");
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        }
        pos=input->tell();
        f.str("");
        f << "SVDR[graph]:";
        zone.closeRecord("SVDR");
      }
    }
    if (ok) {
      int dim[4];
      for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
      f << "rect=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
      bool mirrored;
      *input >> mirrored;
      if (mirrored) f << "mirrored,";
      for (int i=0; i<3; ++i) {
        std::vector<uint32_t> string;
        if (!zone.readString(string) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not read a string\n"));
          f << "###string";
          ok=false;
          break;
        }
        char const *(wh[])= {"name", "filename", "filtername"};
        if (!string.empty())
          f << wh[i] << "=" << libstoff::getString(string).cstr() << ",";
      }
    }
    if (ok) {
      bool hasGraphicLink;
      *input >> hasGraphicLink;
      if (hasGraphicLink) f << "hasGraphicLink,";
    }
    if (ok && input->tell()<lastPos) {
      shared_ptr<StarItemPool> pool=doc.findItemPool(StarItemPool::T_XOutdevPool, false);
      if (!pool)
        pool=doc.getNewItemPool(StarItemPool::T_XOutdevPool);
      uint16_t nWhich=1243; // SDRATTRSET_GRAF
      shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
      if (!item || input->tell()>lastPos) {
        f << "###";
      }
      else if (item->m_attribute)
        item->m_attribute->print(f);
    }
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGraphic: find extra data\n"));
      f << "###extra";
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectDraw::readSVDRObjectGroup(StarZone &zone, StarObject &doc)
{
  if (!readSVDRObjectHeader(zone, doc))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:group,";
  // svx_svdogrp SdrObjGroup::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  std::vector<uint32_t> string;
  bool ok=true;
  if (!zone.readString(string) || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not read the sheet style name\n"));
    ok=false;
  }
  else if (!string.empty())
    f << libstoff::getString(string).cstr() << ",";
  if (ok) {
    f << "bRefPoint=" << input->readULong(1) << ",";
    f << "refPoint=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    if (input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: the zone seems too short\n"));
      f << "###short";
    }
  }
  while (ok && input->tell()+4<lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    f.str("");
    f << "SVDR:";
    pos=input->tell();
    // check magic
    std::string magic("");
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(-4, librevenge::RVNG_SEEK_CUR);
    if (magic=="DrXX" && zone.openSDRHeader(magic)) {
      ascFile.addPos(pos);
      ascFile.addNote("SVDR:DrXX");
      zone.closeSDRHeader("SVDR");
      pos=input->tell();
      break;
    }
    if (magic!="DrOb")
      break;
    if (!readSdrObject(zone, doc)) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: can not read an object\n"));
      f << "###object";
      ok=false;
      break;
    }
  }
  if (ok && vers>=2) {
    f << "drehWink=" << input->readLong(4) << ",";
    f << "shearWink=" << input->readLong(4) << ",";
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectGroup: find extra data\n"));
      f << "###extra";
    }
    if (input->tell()!=pos)
      ascFile.addDelimiter(input->tell(),'|');
  }
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectMeasure(StarZone &zone, StarObject &doc)
{
  if (!readSVDRObjectText(zone, doc))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:measure,";
  // svx_svdomeas SdrMeasureObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectMeasure: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<2; ++i)
    f << "pt" << i << "=" << input->readLong(4) << "x" << input->readLong(4) << ",";
  bool overwritten;
  *input >> overwritten;
  if (overwritten) f << "overwritten[text],";
  shared_ptr<StarItemPool> pool=doc.findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool)
    pool=doc.getNewItemPool(StarItemPool::T_XOutdevPool);
  uint16_t nWhich=1171; // SDRATTRSET_MEASURE
  shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
  if (!item || input->tell()>lastPos) {
    f << "###";
  }
  else if (item->m_attribute)
    item->m_attribute->print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectOLE(StarZone &zone, StarObject &doc, int id)
{
  if (!readSVDRObjectRect(zone, doc, id))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:" << (id==23 ? "ole2" : "frame") << ",";
  // svx_svdoole2 SdrOle2Obj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectOLE: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  bool ok=true;
  for (int i=0; i<2; ++i) {
    std::vector<uint32_t> string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectOLE: can not read a string\n"));
      f << "###string";
      ok=false;
      break;
    }
    if (!string.empty())
      f << (i==0 ? "persistName" : "progName") << "=" << libstoff::getString(string).cstr() << ",";
  }
  if (ok) {
    bool objValid, hasGraphic;
    *input >> objValid >> hasGraphic;
    if (objValid) f << "obj[refValid],";
    if (hasGraphic) {
      StarGraphicStruct::StarGraphic graphic;
      if (!graphic.read(zone, lastPos) || input->tell()>lastPos) {
        // TODO: we can recover here the unknown graphic
        f << "###graphic";
        ok=false;
      }
    }
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectOLE: find extra data\n"));
      f << "###extra";
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectPath(StarZone &zone, StarObject &doc, int id)
{
  if (!readSVDRObjectText(zone, doc))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  int vers=zone.getHeaderVersion();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  f << (id==2 ? "line" : id==8 ? "poly" : id==9 ? "polyline" : id==10 ? "pathline" :
        id==11 ? "pathfill" : id==12 ? "freeline" : id==13 ? "freefill" : id==26 ? "pathpoly" : "pathline") << ",";
  // svx_svdopath SdrPathObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectPath: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  bool ok=true;
  if (vers<=6 && (id==2 || id==8 || id==9)) {
    int nPoly=id==2 ? 2 : id==8 ? 1 : (int) input->readULong(2);
    for (int poly=0; poly<nPoly; ++poly) {
      uint16_t n;
      *input >> n;
      if (input->tell()+8*n>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectHeader: the number of point seems bad\n"));
        f << "###n=" << n << ",";
        ok=false;
        break;
      }
      f << "poly" << poly <<"=[";
      for (int pt=0; pt<int(n); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "],";
    }
  }
  else {
    bool recOpened=false;
    if (vers>=11) {
      recOpened=zone.openRecord();
      if (!recOpened) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectPath: can not open zone record\n"));
        ok=false;
      }
    }
    int nPoly=ok ? (int) input->readULong(2) : 0;
    for (int poly=0; poly<nPoly; ++poly) {
      uint16_t n;
      *input >> n;
      if (input->tell()+9*n>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectPath: the number of point seems bad\n"));
        f << "###n=" << n << ",";
        ok=false;
        break;
      }
      f << "poly" << poly <<"=[";
      for (int pt=0; pt<int(n); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "],";
      f << "fl" << poly << "=[";
      for (int pt=0; pt<int(n); ++pt) f << input->readULong(1) << ",";
      f << "],";
    }
    if (recOpened) {
      if (input->tell()!=zone.getRecordLastPosition()) {
        if (ok) {
          f << "##";
          STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectPath: find extra data\n"));
        }
        ascFile.addDelimiter(input->tell(),'|');
      }
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      zone.closeRecord("SVDR");
    }
    ok=false;
  }
  if (!ok) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectDraw::readSVDRObjectRect(StarZone &zone, StarObject &doc, int id)
{
  if (!readSVDRObjectText(zone, doc))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  int vers=zone.getHeaderVersion();
  if (vers<3 && (id==16 || id==17 || id==20 || id==21))
    return true;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:rectZone,";
  // svx_svdorect.cxx SdrRectObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectRect: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  if (vers<=5) {
    f << "nEckRag=" << input->readLong(4) << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectDraw::readSVDRObjectText(StarZone &zone, StarObject &doc)
{
  if (!readSVDRObjectAttrib(zone, doc))
    return false;

  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:textZone,";
  // svx_svdotext SdrTextObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectText: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  f << "textKind=" << input->readULong(1) << ",";
  int dim[4];
  for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
  f << "rect=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
  f << "drehWink=" << input->readLong(4) << ",";
  f << "shearWink=" << input->readLong(4) << ",";
  bool paraObjectValid;
  *input >> paraObjectValid;
  bool ok=input->tell()<=lastPos;
  if (paraObjectValid) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SVDR:textB";
    if (vers>=11 && !zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectText: can not open paraObject record\n"));
      paraObjectValid=ok=false;
      f << "##paraObject";
    }
    else if (!readSDROutlinerParaObject(zone, doc)) {
      ok=false;
      f << "##paraObject";
    }
    else
      pos=input->tell();
    if (paraObjectValid && vers>=11) {
      zone.closeRecord("SdrParaObject");
      ok=true;
    }
  }
  if (ok && vers>=10) {
    bool hasBound;
    *input >> hasBound;
    if (hasBound) {
      for (int i=0; i<4; ++i) dim[i]=(int) input->readLong(4);
      f << "bound=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
    }
    ok=input->tell()<=lastPos;
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSVDRObjectText: find extra data\n"));
      f << "###extra, vers=" << vers;
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectDraw::readSDRObjectConnection(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrCn" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(SdrObjConn)[" << zone.getRecordLevel() << "]:";
  // svx_svdoedge.cxx SdrObjConnection::Read
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  if (!readSDRObjectSurrogate(zone)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObjectConnection: can not read object surrogate\n"));
    f << "###surrogate";
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrObjConn:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSDRHeader("SdrObjConn");
    return true;
  }
  f << "condId=" << input->readULong(2) << ",";
  f << "dist=" << input->readLong(4) << "x" << input->readLong(4) << ",";
  for (int i=0; i<6; ++i) {
    bool val;
    *input>>val;
    char const *(wh[])= {"bestConn", "bestVertex", "xDistOvr", "yDistOvr", "autoVertex", "autoCorner"};
    if (val)
      f << wh[i] << ",";
  }
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObjectConnection: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrObjConn:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrObjConn");
  return true;
}

bool StarObjectDraw::readSDRObjectSurrogate(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrObjSurr):";
  // svx_svdsuro.cxx SdrObjSurrogate::ImpRead
  int id=(int) input->readULong(1);
  f << "id=" << id << ",";
  bool ok=true;
  if (id) {
    int eid=id&0x1f;
    int nBytes=1+(id>>6);
    if (nBytes==3) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObjectConnection: unexpected num bytes\n"));
      f << "###nBytes,";
      ok=false;
    }
    if (ok)
      f << "val=" << input->readULong(nBytes) << ",";
    if (ok && eid>=0x10 && eid<=0x1a)
      f << "page=" << input->readULong(2) << ",";
    if (ok && id&0x20) {
      int grpLevel=(int) input->readULong(2);
      f << "nChild=" << grpLevel << ",";
      if (input->tell()+nBytes*grpLevel>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObjectConnection: num child is bas\n"));
        f << "###";
        ok=false;
      }
      else {
        f << "child=[";
        for (int i=0; i<grpLevel; ++i)
          f << input->readULong(nBytes) << ",";
        f << "],";
      }
    }
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=lastPos;
}

bool StarObjectDraw::readSDROutlinerParaObject(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrParaObject):";
  // svx_outlobj.cxx OutlinerParaObject::Create
  long N=(long) input->readULong(4);
  f << "N=" << N << ",";
  long syncRef=(long) input->readULong(4);
  int vers=0;
  if (syncRef == 0x12345678)
    vers = 1;
  else if (syncRef == 0x22345678)
    vers = 2;
  else if (syncRef == 0x32345678)
    vers = 3;
  else if (syncRef == 0x42345678)
    vers = 4;
  else {
    f << "##syncRef,";
    STOFF_DEBUG_MSG(("StarObjectDraw::readSDROutlinerParaObject: can not check the version\n"));
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return N==0;
  }
  f << "version=" << vers << ",";
  if (vers<=3) {
    for (long i=0; i<N; ++i) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
      f.str("");
      f << "SdrParaObject:";
      shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(doc, true));
      if (!smallText->read(zone, lastPos) || input->tell()>lastPos) {
        f << "###editTextObject";
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      pos=input->tell();
      f << "sync=" << input->readULong(4) << ",";
      f << "depth=" << input->readULong(2) << ",";
      bool ok=true;
      if (vers==1) {
        int flags=(int) input->readULong(2);
        if (flags&1) {
          StarBitmap bitmap;
          librevenge::RVNGBinaryData data;
          std::string dType;
          if (!bitmap.readBitmap(zone, true, lastPos, data, dType)) {
            STOFF_DEBUG_MSG(("StarObjectDraw::readSDROutlinerParaObject: can not check the bitmpa\n"));
            ok=false;
          }
        }
        else {
          STOFFColor color;
          if (!input->readColor(color)) {
            STOFF_DEBUG_MSG(("StarObjectDraw::readSDROutlinerParaObject: can not find a color\n"));
            f << "###aColor,";
            ok=false;
          }
          else {
            f << "col=" << color << ",";
            input->seek(16, librevenge::RVNG_SEEK_CUR);
          }
          std::vector<uint32_t> string;
          if (ok && (!zone.readString(string) || input->tell()>lastPos)) {
            STOFF_DEBUG_MSG(("StarObjectDraw::readSDROutlinerParaObject: can not find string\n"));
            f << "###string,";
            ok=false;
          }
          else if (!string.empty())
            f << "col[name]=" << libstoff::getString(string).cstr() << ",";
          if (ok)
            input->seek(12, librevenge::RVNG_SEEK_CUR);
        }
        input->seek(8, librevenge::RVNG_SEEK_CUR); // 2 long dummy
      }

      if (input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSDROutlinerParaObject: we read too much data\n"));
        f << "###bad,";
        ok=false;
      }
      if (!ok) {
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (i+1!=N) f << "sync=" << input->readULong(4) << ",";
    }
    if (vers==3) {
      bool isEditDoc;
      *input >> isEditDoc;
      if (isEditDoc) f << "isEditDoc,";
    }
  }
  else {
    pos=input->tell();
    f.str("");
    f << "SdrParaObject:";
    shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(doc, true)); // checkme, we must use the text edit pool here
    if (!smallText->read(zone, lastPos) || input->tell()+N*2>lastPos) {
      f << "###editTextObject";
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    pos=input->tell();
    f << "depth=[";
    for (long i=0; i<N; ++i) f << input->readULong(2) << ",";
    f << "],";
    bool isEditDoc;
    *input >> isEditDoc;
    if (isEditDoc) f << "isEditDoc,";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectDraw::readSDRGluePoint(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrGluePoint):";
  // svx_svdglue_drawdoc.xx: operator>>(SdrGluePoint)
  int dim[2];
  for (int i=0; i<2; ++i) dim[i]=(int) input->readULong(2);
  f << "dim=" << STOFFVec2i(dim[0],dim[1]) << ",";
  f << "escDir=" << input->readULong(2) << ",";
  f << "id=" << input->readULong(2) << ",";
  f << "align=" << input->readULong(2) << ",";
  bool noPercent;
  *input >> noPercent;
  if (noPercent) f << "noPercent,";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SdrGluePoint");
  return true;
}

bool StarObjectDraw::readSDRGluePointList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrGluePoint)[list]:";
  // svx_svdglue_drawdoc.xx: operator>>(SdrGluePointList)
  int n=(int) input->readULong(2);
  f << "n=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  for (int i=0; i<n; ++i) {
    pos=input->tell();
    if (!readSDRGluePoint(zone)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      STOFF_DEBUG_MSG(("StarObjectDraw::readSDRGluePointList: can not find a glue point\n"));
    }
  }
  zone.closeRecord("SdrGluePoint");
  return true;
}

bool StarObjectDraw::readSDRUserData(StarZone &zone, StarObject &doc, bool inRecord)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (inRecord && !zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrUserData):";
  // svx_svdobj.xx: SdrObject::ReadData
  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+6>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSDRUserData: the zone seems too short\n"));
  }
  else {
    std::string type("");
    for (int i=0; i<4; ++i) type+=(char) input->readULong(1);
    int id=(int) input->readULong(2);
    f << type << ",id=" << id << ",";
    if (type=="SCHU") {
      if (!readSCHUObject(zone, id, doc)) {
        f << "##";
        if (!inRecord) {
          STOFF_DEBUG_MSG(("StarObjectDraw::readSDRUserData: can not determine end size\n"));
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          return false;
        }
      }
      else if (!inRecord)
        lastPos=input->tell();
    }
    else {
      int nSz=-1;
      if (type=="SDUD" && id==1 && input->tell()+6<=lastPos) {
        int val=(int) input->readULong(2); // 0
        if (val) f << "f0=" << val << ",";
        int sz=(int) input->readULong(4);
        if (sz>=4 && input->tell()+sz-4<=lastPos)
          nSz=sz-4;
        else
          f << "##sz=" << nSz << ",";
      }
      f << "#";
      static bool first=true;
      if (first) {
        first=false;
        STOFF_DEBUG_MSG(("StarObjectDraw::readSDRUserData: reading data is not implemented\n"));
      }
      if (!inRecord && nSz<0) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSDRUserData: can not determine end size\n"));
        f << "##";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      else if (!inRecord)
        lastPos=input->tell()+nSz;
    }
  }
  if (input->tell()!=lastPos) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (inRecord)
    zone.closeRecord("SdrUserData");
  return true;
}

bool StarObjectDraw::readSDRUserDataList(StarZone &zone, StarObject &doc, bool inRecord)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (inRecord && !zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrUserData)[list]:";
  // svx_svdglue_drawdoc.xx: operator>>(SdrUserDataList)
  int n=(int) input->readULong(2);
  f << "n=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  for (int i=0; i<n; ++i) {
    pos=input->tell();
    if (!readSDRUserData(zone, doc, inRecord)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      STOFF_DEBUG_MSG(("StarObjectDraw::readSDRUserDataList: can not find a glue point\n"));
    }
  }
  if (inRecord) zone.closeRecord("SdrUserData");
  return true;
}

////////////////////////////////////////////////////////////
// SCHU object
////////////////////////////////////////////////////////////
bool StarObjectDraw::readSCHUObject(StarZone &zone, int identifier, StarObject &doc)
{
  if (identifier==1)
    return readSVDRObjectGroup(zone, doc);
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // long endPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCHU):";
  if (identifier<=0 || identifier>7) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSCHUObject: find unknown identifier\n"));
    f << "###id=" << identifier << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return false;
  }
  char const(*wh[])= {"none", "group", "objectId", "objectAdjustId", "dataRowId", "dataPointId", "lightfactorId", "axisId"};
  f << wh[identifier] << ",";
  // sch_objfac.xx : SchObjFactory::MakeUserData
  int vers=(int) input->readULong(2);
  f << "vers=" << vers << ",";
  switch (identifier) {
  case 2:
    f << "id=" << input->readULong(2) << ",";
    break;
  case 3:
    f << "adjust=" << input->readULong(2) << ",";
    if (vers>=1)
      f << "orient=" << input->readULong(2) << ",";
    break;
  case 4:
    f << "row=" << input->readLong(2) << ",";
    break;
  case 5:
    f << "col=" << input->readLong(2) << ",";
    f << "row=" << input->readLong(2) << ",";
    break;
  case 6: {
    double val;
    *input >> val;
    f << "factor=" << val << ",";
    break;
  }
  case 7:
    f << "id=" << input->readULong(2) << ",";
    break;
  default:
    f << "##";
    break;
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
