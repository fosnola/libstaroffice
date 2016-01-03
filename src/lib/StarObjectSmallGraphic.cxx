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
#include "StarBitmap.hxx"
#include "StarGraphicStruct.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarItemPool.hxx"
#include "StarZone.hxx"

#include "StarObjectSmallGraphic.hxx"

/** Internal: the structures of a StarObjectSmallGraphic */
namespace StarObjectSmallGraphicInternal
{
////////////////////////////////////////
//! Internal: the state of a StarObjectSmallGraphic
struct State {
  //! constructor
  State() : m_graphic()
  {
  }

  //! the graphic
  std::vector<uint32_t> m_graphic;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectSmallGraphic::StarObjectSmallGraphic(StarObject const &orig, bool duplicateState) : StarObject(orig, duplicateState), m_graphicState(new StarObjectSmallGraphicInternal::State)
{
}

StarObjectSmallGraphic::~StarObjectSmallGraphic()
{
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////

bool StarObjectSmallGraphic::readSdrObject(StarZone &zone)
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
    if (!readSVDRObject(zone, (int) identifier)) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: can not read SVDr object\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: read SVDr object, find extra data\n"));
        first=false;
      }
      f << "##";
    }
  }
  else if (magic=="SCHU") {
    if (!readSCHUObject(zone, (int) identifier)) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: can not read SCHU object\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: read Schu object, find extra data\n"));
        first=false;
      }
      f << "##";
    }
  }
  else if (magic=="FM01") { // FmFormInventor
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: read FM01 object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: find unknown magic\n"));
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

////////////////////////////////////////////////////////////
//  SVDR
////////////////////////////////////////////////////////////
bool StarObjectSmallGraphic::readSVDRObject(StarZone &zone, int identifier)
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
    ok=readSVDRObjectGroup(zone);
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
    ok=readSVDRObjectPath(zone, identifier);
    break;
  case 4: // circle
  case 5: // sector
  case 6: // arc
  case 7: // cut
    ok=readSVDRObjectCircle(zone, identifier);
    break;
  case 3: // rect
  case 16: // text
  case 17: // textextended
  case 20: // title text
  case 21: // outline text
    ok=readSVDRObjectRect(zone, identifier);
    break;
  case 24: // edge
    ok=readSVDRObjectEdge(zone);
    break;
  case 22: // graph
    ok=readSVDRObjectGraph(zone);
    break;
  case 23: // ole
  case 31: // frame
    ok=readSVDRObjectOLE(zone, identifier);
    break;
  case 25: // caption
    ok=readSVDRObjectCaption(zone);
    break;
  case 28: { // page
    ok=readSVDRObjectHeader(zone);
    if (!ok) break;
    pos=input->tell();
    if (!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not open page record\n"));
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
    ok=readSVDRObjectMeasure(zone);
    break;
  case 32: { // uno
    ok=readSVDRObjectRect(zone, identifier);
    pos=input->tell();
    if (!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not open uno record\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
    f << "SVDR[uno]:";
    // + SdrUnoObj::ReadData (checkme)
    std::vector<uint32_t> string;
    if (input->tell()!=zone.getRecordLastPosition() && (!zone.readString(string) || input->tell()>zone.getRecordLastPosition())) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: can not read uno string\n"));
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
    ok=readSVDRObjectHeader(zone);
    break;
  }
  pos=input->tell();
  if (!ok) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not read some zone\n"));
    ascFile.addPos(pos);
    ascFile.addNote("Entries(SVDR):###");
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  if (input->tell()==endPos)
    return true;
  static bool first=true;
  if (first) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: find unexpected data\n"));
  }
  if (identifier<=0 || identifier>32) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: unknown identifier\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectAttrib(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!readSVDRObjectHeader(zone)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:attrib,";

  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  long lastPos=zone.getRecordLastPosition();
  shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool)
    pool=getNewItemPool(StarItemPool::T_VCControlPool);
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: can not read the sheet style name\n"));
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: find extra data\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectCaption(StarZone &zone)
{
  if (!readSVDRObjectRect(zone, 25))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:caption,";
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectCaption: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  // svx_svdocapt.cxx SdrCaptionObj::ReadData
  bool ok=true;
  uint16_t n;
  *input >> n;
  if (input->tell()+8*n>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectCaption: the number of point seems bad\n"));
    f << "###n=" << n << ",";
    ok=false;
    n=0;
  }
  f << "poly=[";
  for (int pt=0; pt<int(n); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
  f << "],";
  if (ok) {
    shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=getNewItemPool(StarItemPool::T_XOutdevPool);
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

bool StarObjectSmallGraphic::readSVDRObjectCircle(StarZone &zone, int id)
{
  if (!readSVDRObjectRect(zone, id))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  f << (id==4 ? "circle" : id==5 ? "sector" : id==6 ? "arc" : "cut") << ",";
  // svx_svdocirc SdrCircObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectCircle: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  if (id!=4)
    f << "angles=" << float(input->readLong(4))/100.f << "x" << float(input->readLong(4))/100.f << ",";
  if (input->tell()!=lastPos) {
    shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=getNewItemPool(StarItemPool::T_XOutdevPool);
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

bool StarObjectSmallGraphic::readSVDRObjectEdge(StarZone &zone)
{
  if (!readSVDRObjectText(zone))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:edge,";
  // svx_svdoedge SdrEdgeObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  bool ok=true;
  if (vers<2) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: unexpected version\n"));
    f << "##badVers,";
    ok=false;
  }

  bool openRec=false;
  if (ok && vers>=11) {
    openRec=zone.openRecord();
    if (!openRec) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: can not edgeTrack record\n"));
      f << "###record";
      ok=false;
    }
  }
  if (ok) {
    uint16_t n;
    *input >> n;
    if (input->tell()+9*n>zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: the number of point seems bad\n"));
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
    shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=getNewItemPool(StarItemPool::T_XOutdevPool);
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: SdrEdgeInfoRec seems too short\n"));
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: find extra data\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectHeader(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SVDR)[" << zone.getRecordLevel() << "]:header,";

  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: can not open record\n"));
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: oops read to much data\n"));
    f << "###bad,";
    ok=false;
  }
  if (ok && vers<11) {
    // poly.cxx operator>>(Polygon) : test compression here
    uint16_t n;
    *input >> n;
    if (input->tell()+8*n>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: the number of point seems bad\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: can not find the gluePoints record\n"));
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
      if (!readSDRUserDataList(zone, vers>=11)) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: can not find the data list record\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectGraph(StarZone &zone)
{
  if (!readSVDRObjectRect(zone, 22))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:graph,";
  // SdrGrafObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not open record\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read the file name\n"));
        f << "###fileName";
        ok=false;
      }
      else if (!string.empty())
        f << "fileName=" << libstoff::getString(string).cstr() << ",";
    }
    if (ok && vers>=9) {
      std::vector<uint32_t> string;
      if (!zone.readString(string) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read the filter name\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not open graphic record\n"));
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
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read a string\n"));
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
      shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_XOutdevPool, false);
      if (!pool)
        pool=getNewItemPool(StarItemPool::T_XOutdevPool);
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraphic: find extra data\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectGroup(StarZone &zone)
{
  if (!readSVDRObjectHeader(zone))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:group,";
  // svx_svdogrp SdrObjGroup::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  std::vector<uint32_t> string;
  bool ok=true;
  if (!zone.readString(string) || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read the sheet style name\n"));
    ok=false;
  }
  else if (!string.empty())
    f << libstoff::getString(string).cstr() << ",";
  if (ok) {
    f << "bRefPoint=" << input->readULong(1) << ",";
    f << "refPoint=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    if (input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: the zone seems too short\n"));
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
    if (!readSdrObject(zone)) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read an object\n"));
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: find extra data\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectMeasure(StarZone &zone)
{
  if (!readSVDRObjectText(zone))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:measure,";
  // svx_svdomeas SdrMeasureObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectMeasure: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<2; ++i)
    f << "pt" << i << "=" << input->readLong(4) << "x" << input->readLong(4) << ",";
  bool overwritten;
  *input >> overwritten;
  if (overwritten) f << "overwritten[text],";
  shared_ptr<StarItemPool> pool=findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool)
    pool=getNewItemPool(StarItemPool::T_XOutdevPool);
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

bool StarObjectSmallGraphic::readSVDRObjectOLE(StarZone &zone, int id)
{
  if (!readSVDRObjectRect(zone, id))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:" << (id==23 ? "ole2" : "frame") << ",";
  // svx_svdoole2 SdrOle2Obj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  bool ok=true;
  for (int i=0; i<2; ++i) {
    std::vector<uint32_t> string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: can not read a string\n"));
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: find extra data\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectPath(StarZone &zone, int id)
{
  if (!readSVDRObjectText(zone))
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: can not open record\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: the number of point seems bad\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: can not open zone record\n"));
        ok=false;
      }
    }
    int nPoly=ok ? (int) input->readULong(2) : 0;
    for (int poly=0; poly<nPoly; ++poly) {
      uint16_t n;
      *input >> n;
      if (input->tell()+9*n>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: the number of point seems bad\n"));
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
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: find extra data\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectRect(StarZone &zone, int id)
{
  if (!readSVDRObjectText(zone))
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectRect: can not open record\n"));
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

bool StarObjectSmallGraphic::readSVDRObjectText(StarZone &zone)
{
  if (!readSVDRObjectAttrib(zone))
    return false;

  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:textZone,";
  // svx_svdotext SdrTextObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectText: can not open record\n"));
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectText: can not open paraObject record\n"));
      paraObjectValid=ok=false;
      f << "##paraObject";
    }
    else if (!readSDROutlinerParaObject(zone)) {
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectText: find extra data\n"));
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

bool StarObjectSmallGraphic::readSDRObjectConnection(StarZone &zone)
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: can not read object surrogate\n"));
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrObjConn:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrObjConn");
  return true;
}

bool StarObjectSmallGraphic::readSDRObjectSurrogate(StarZone &zone)
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: unexpected num bytes\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: num child is bas\n"));
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

bool StarObjectSmallGraphic::readSDROutlinerParaObject(StarZone &zone)
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not check the version\n"));
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
      shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(*this, true));
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
            STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not check the bitmpa\n"));
            ok=false;
          }
        }
        else {
          STOFFColor color;
          if (!input->readColor(color)) {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not find a color\n"));
            f << "###aColor,";
            ok=false;
          }
          else {
            f << "col=" << color << ",";
            input->seek(16, librevenge::RVNG_SEEK_CUR);
          }
          std::vector<uint32_t> string;
          if (ok && (!zone.readString(string) || input->tell()>lastPos)) {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not find string\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: we read too much data\n"));
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
    shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(*this, true)); // checkme, we must use the text edit pool here
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

bool StarObjectSmallGraphic::readSDRGluePoint(StarZone &zone)
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

bool StarObjectSmallGraphic::readSDRGluePointList(StarZone &zone)
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
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRGluePointList: can not find a glue point\n"));
    }
  }
  zone.closeRecord("SdrGluePoint");
  return true;
}

bool StarObjectSmallGraphic::readSDRUserData(StarZone &zone, bool inRecord)
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
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: the zone seems too short\n"));
  }
  else {
    std::string type("");
    for (int i=0; i<4; ++i) type+=(char) input->readULong(1);
    int id=(int) input->readULong(2);
    f << type << ",id=" << id << ",";
    if (type=="SCHU") {
      if (!readSCHUObject(zone, id)) {
        f << "##";
        if (!inRecord) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: can not determine end size\n"));
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
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: reading data is not implemented\n"));
      }
      if (!inRecord && nSz<0) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: can not determine end size\n"));
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

bool StarObjectSmallGraphic::readSDRUserDataList(StarZone &zone, bool inRecord)
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
    if (!readSDRUserData(zone, inRecord)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserDataList: can not find a glue point\n"));
    }
  }
  if (inRecord) zone.closeRecord("SdrUserData");
  return true;
}

////////////////////////////////////////////////////////////
// SCHU object
////////////////////////////////////////////////////////////
bool StarObjectSmallGraphic::readSCHUObject(StarZone &zone, int identifier)
{
  if (identifier==1)
    return readSVDRObjectGroup(zone);
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // long endPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCHU):";
  if (identifier<=0 || identifier>7) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSCHUObject: find unknown identifier\n"));
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
