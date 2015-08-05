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
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
