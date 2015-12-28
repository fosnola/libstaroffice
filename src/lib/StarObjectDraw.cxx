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
#include "StarEncryption.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
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
StarObjectDraw::StarObjectDraw(StarObject const &orig, bool duplicateState) : StarObject::StarObject(orig, duplicateState), m_state(new StarObjectDrawInternal::State)
{
}

StarObjectDraw::~StarObjectDraw()
{
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
  StarFileManager fileManager;
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
    else {
      dir = name.substr(0,pos);
      base = name.substr(pos+1);
    }
    ole->setReadInverted(true);
    if (base=="StarDrawDocument") {
      readDrawDocument(ole,name);
      continue;
    }
    if (base=="StarDrawDocument3") {
      readDrawDocument3(ole,name);
      continue;
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
  return true;
}

bool StarObjectDraw::readDrawDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWDrawDocument", getPassword()); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCDrawDocument):";
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  if (0 && !input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWDrawDocument:###extra");
  }
  return true;
}
catch (...)
{
  return false;
}

bool StarObjectDraw::readDrawDocument3(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWDrawDocument3", getPassword()); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCDraw3Document):";
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  if (0 && !input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument3: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWDraw3Document:###extra");
  }
  return true;
}
catch (...)
{
  return false;
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
  // in DrawingLayer, find also 0500000001 and 060000000000
  zone.closeSDRHeader("SdrModel");
  return true;
}

bool StarObjectDraw::readSdrObject(StarZone &zone)
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
  // svdobject.cxx SdrObjFactory::MakeNewObject
  pos=input->tell();
  f.str("");
  f << "SdrObject:";
  magic="";
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  uint16_t identifier;
  *input>>identifier;
  f << magic << ", ident=" << std::hex << identifier << std::dec << ",";
  if (magic=="SVDr") {
    static bool first=true;
    if (first) { // see SdrObjFactory::MakeNewObject
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: read SVDr object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else if (magic=="SCHU") {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrObject: read SCHU object is not implemented\n"));
      first=false;
    }
    f << "##";
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
  // svdpage.cxx SdrPageDef::ReadData
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
    if (readSdrObject(zone)) continue;

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
      if (!readSdrObject(zone)) {
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
      else {
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

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
