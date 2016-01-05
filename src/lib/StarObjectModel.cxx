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

#include "STOFFListener.hxx"
#include "STOFFOLEParser.hxx"

#include "StarAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallGraphic.hxx"
#include "StarZone.hxx"

#include "StarObjectModel.hxx"

/** Internal: the structures of a StarObjectModel */
namespace StarObjectModelInternal
{
////////////////////////////////////////
//! Internal: class used to store a layer and its data
class Layer
{
public:
  //! constructor
  Layer() : m_name(""), m_id(0), m_type(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Layer const &layer)
  {
    o << "id=" << layer.m_id << ",";
    if (!layer.m_name.empty()) o << layer.m_name.cstr() << ",";
    if (layer.m_type==0) o << "user,";
    return o;
  }
  //! the layer name
  librevenge::RVNGString m_name;
  //! the layer id
  int m_id;
  //! the layer type
  int m_type;
};

////////////////////////////////////////
//! Internal: class used to store a page and its data
class Page
{
public:
  ////////////////////////////////////////
  //! Internal: class used to store a page descriptor
  struct Descriptor {
    //! constructor
    Descriptor() : m_pageNumber(1), m_layerList()
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Descriptor const &desc)
    {
      o << "pageNum=" << desc.m_pageNumber << ",";
      o << "aVisLayer=[";
      for (size_t i=0; i<desc.m_layerList.size(); ++i) {
        int c=desc.m_layerList[i];
        if (c==255)
          o << "*,";
        else if (c)
          o << c << ",";
        else
          o << "_,";
      }
      o << "],";
      return o;
    }
    //! the page number
    int m_pageNumber;
    //! a list of layer
    std::vector<int> m_layerList;
  };
  //! constructor
  Page() : m_masterPage(false), m_size(0,0), m_masterPageDescList(), m_layer(), m_objectList(), m_background()
  {
    for (int i=0; i<4; ++i) m_borders[i]=0;
  }
  //! a flag to know if the page is a master page
  bool m_masterPage;
  //! the page size
  STOFFVec2i m_size;
  //! the border size: left, up, right, bottom
  int m_borders[4];
  //! the list of master page descriptor
  std::vector<Descriptor> m_masterPageDescList;
  //! the layer
  Layer m_layer;
  //! the list of object
  std::vector<shared_ptr<StarObjectSmallGraphic> > m_objectList;
  //! the background object
  shared_ptr<StarObjectSmallGraphic> m_background;
};

////////////////////////////////////////
//! Internal: the state of a StarObjectModel
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
StarObjectModel::StarObjectModel(StarObject const &orig, bool duplicateState) : StarObject(orig, duplicateState), m_modelState(new StarObjectModelInternal::State)
{
}

StarObjectModel::~StarObjectModel()
{
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
bool StarObjectModel::read(StarZone &zone)
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
    STOFF_DEBUG_MSG(("StarObjectModel::read: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrMd" || version>17) {
    STOFF_DEBUG_MSG(("StarObjectModel::read: unexpected magic data\n"));
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
    STOFF_DEBUG_MSG(("StarObjectModel::read: can not open downCompat\n"));
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
      STOFF_DEBUG_MSG(("StarObjectModel::read: unexpected magic2 data\n"));
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
      STOFF_DEBUG_MSG(("StarObjectModel::read: can not read model info\n"));
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
        STOFF_DEBUG_MSG(("StarObjectModel::read: can not read model statistic\n"));
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
      STOFF_DEBUG_MSG(("StarObjectModel::read: can not read model format compat\n"));
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
      STOFF_DEBUG_MSG(("StarObjectModel::read: reading model format for version 0 is not implemented\n"));
      f << "###version0";
      ok=false;
    }
    else {
      if (version<11) f << "nCharSet=" << input->readLong(2) << ",";
      std::vector<uint32_t> string;
      for (int i=0; i<6; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectModel::read: can not read a string\n"));
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
    if (magic=="DrLy") {
      StarObjectModelInternal::Layer layer;
      if (readSdrLayer(zone, layer))
        continue;
    }
    if (magic=="DrLS" && readSdrLayerSet(zone)) continue;
    if ((magic=="DrPg" || magic=="DrMP") && readSdrPage(zone)) continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSDRHeader(magic)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SdrModel[" << magic << "-" << zone.getRecordLevel() << "]:";
    if (magic!="DrXX") {
      STOFF_DEBUG_MSG(("StarObjectModel::read: find unexpected child\n"));
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

bool StarObjectModel::readSdrLayer(StarZone &zone, StarObjectModelInternal::Layer &layer)
{
  layer=StarObjectModelInternal::Layer();
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrLayer: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLy") {
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrLayer: unexpected magic data\n"));
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrLayer: can not read a string\n"));
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

bool StarObjectModel::readSdrLayerSet(StarZone &zone)
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrLayerSet: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLS") {
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrLayerSet: unexpected magic data\n"));
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrLayerSet: can not read a string\n"));
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

bool StarObjectModel::readSdrPage(StarZone &zone)
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrPg" && magic!="DrMP") {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();

  shared_ptr<StarObjectModelInternal::Page> page(new StarObjectModelInternal::Page);
  page->m_masterPage=magic=="DrMP";
  f.str("");
  f << "SdrPageDefA[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx SdrPage::ReadData
  bool hasExtraData=false;
  for (int tr=0; tr<2; ++tr) {
    /* sometimes there is one or two block which appears before the
       SdrPage::ReadData data, so try to find them*/
    long lastPos=zone.getRecordLastPosition();
    if (!zone.openRecord()) { // SdrPageDefA1
      STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: can not open downCompat\n"));
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
      STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: bad version\n"));
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
      STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: bad version2\n"));
      f << "###n2=" << n << ",";
      ok=false;
    }
    if (ok && n==1) {
      long actPos=input->tell();
      shared_ptr<StarItemPool> pool=getNewItemPool(StarItemPool::T_VCControlPool);
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
      STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: unexpected magic2 data\n"));
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
    int32_t nWdt, nHgt;
    int32_t borders[4];
    uint16_t n;
    *input >> nWdt >> nHgt >> borders[0] >> borders[1] >> borders[2] >> borders[3] >> n;
    f << "dim=" << nWdt << "x" << nHgt << ",";
    f << "border=[" << borders[0] << "x" << borders[1] << "-" << borders[2] << "x" << borders[3] << "],";
    page->m_size=STOFFVec2i(int(nWdt), int(nHgt));
    for (int i=0; i<4; ++i) page->m_borders[i]=(int) borders[i];
    if (n) f << "n=" << n << ","; // link to page name?
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (version>=11) {
    zone.closeRecord("SdrPageDefData");
    ok=true;
  }

  long lastPos=zone.getRecordLastPosition();
  bool layerSet=false;
  while (ok) {
    pos=input->tell();
    if (pos+4>lastPos)
      break;
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrLy" && readSdrLayer(zone, page->m_layer)) continue;
    if (magic=="DrLS" && readSdrLayerSet(zone)) continue;
    if (magic=="DrMP" && readSdrMPageDesc(zone, *page)) continue;
    if (magic=="DrML" && readSdrMPageDescList(zone, *page)) continue;
    break;
  }
  if (ok && version==0) {
    pos=input->tell();
    f.str("");
    f << "SdrPageDef[masterPage]:";
    uint16_t n;
    *input >> n;
    if (pos+2+2*n>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: n is bad\n"));
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
    shared_ptr<StarObjectSmallGraphic> smallGraphic(new StarObjectSmallGraphic(*this, true));
    if (smallGraphic->readSdrObject(zone)) {
      page->m_objectList.push_back(smallGraphic);
      continue;
    }

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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: can not find end object zone\n"));
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
      shared_ptr<StarObjectSmallGraphic> smallGraphic(new StarObjectSmallGraphic(*this, true));
      if (smallGraphic->readSdrObject(zone))
        page->m_background=smallGraphic;
      else {
        STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: can not find backgroound object\n"));
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
          STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: can not find read a string\n"));
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
        STOFF_DEBUG_MSG(("StarObjectModel::readSdrPage: find some unknown zone\n"));
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

bool StarObjectModel::readSdrPageUnknownZone1(StarZone &zone, long lastPos)
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

bool StarObjectModel::readSdrMPageDesc(StarZone &zone, StarObjectModelInternal::Page &page)
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrMPageDesc: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  StarObjectModelInternal::Page::Descriptor desc;
  desc.m_pageNumber=(int) input->readULong(2);
  for (int i=0; i<32; ++i)
    desc.m_layerList.push_back((int) input->readULong(1));
  f << desc;
  page.m_masterPageDescList.push_back(desc);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrMPageDesc");
  return true;
}

bool StarObjectModel::readSdrMPageDescList(StarZone &zone, StarObjectModelInternal::Page &page)
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
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrMPageDescList: can not open the SDR Header\n"));
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
    if (pos<lastPos && readSdrMPageDesc(zone, page)) continue;
    STOFF_DEBUG_MSG(("StarObjectModel::readSdrMPageDescList: can not find a page desc\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SdrMPageList:###pageDesc");
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSDRHeader("SdrMPageList");
  return true;
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
