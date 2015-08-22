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

#include "StarAttribute.hxx"
#include "StarDocument.hxx"
#include "StarItemPool.hxx"

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
      if (m_startPos+long(m_numRecord)*long(m_contentSize) > m_endPos) {
        STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::open: oops the number of record seems bad\n"));
        s << "##numRecord=" << m_numRecord << ",";
        if (m_contentSize && m_endPos>m_startPos)
          m_numRecord=uint16_t((m_endPos-m_startPos)/long(m_contentSize));
        else
          m_numRecord=0;
      }
      m_extra=s.str();
      return true;
    }

    long debOffsetList=((m_headerType==3 || m_headerType==7) ? m_startPos : 0) + long(m_contentSize);
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
  bool getNewContent(std::string const &wh)
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
      libstoff::DebugStream f;
      f << wh << ":###extra";
      m_zone.ascii().addNote(f.str().c_str());
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
    if (m_headerType==2) return m_startPos+m_actualRecord*long(m_contentSize);
    if (m_actualRecord >= uint16_t(m_offsetList.size())) {
      STOFF_DEBUG_MSG(("StarFileManagerInternal::SfxMultiRecord::getLastContentPosition: argh, find unexpected index\n"));
      return m_endPos;
    }
    return m_startPos+long(m_offsetList[size_t(m_actualRecord)]>>8)-14;
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
bool StarFileManager::readFont(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  // font.cxx:  operator>>( ..., Impl_Font& )
  libstoff::DebugStream f;
  f << "Entries(StarFont)[" << zone.getRecordLevel() << "]:";
  if (!zone.openVersionCompatHeader()) {
    STOFF_DEBUG_MSG(("StarFileManager::readFont: can not read the header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<2; ++i) {
    librevenge::RVNGString string;
    if (!zone.readString(string)||input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readFont: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeVersionCompatHeader("StarFont");
      return true;
    }
    if (!string.empty()) f << (i==0 ? "name" : "style") << "=" << string.cstr() << ",";
  }
  f << "size=" << input->readLong(4) << "x" << input->readLong(4) << ",";
  uint16_t eCharSet, eFamily, ePitch, eWeight, eUnderline, eStrikeOut, eItalic, eLanguage, eWidthType;
  int16_t orientation;
  *input >> eCharSet >> eFamily >> ePitch >> eWeight >> eUnderline >> eStrikeOut
         >> eItalic >> eLanguage >> eWidthType >> orientation;
  if (eCharSet) f << "eCharSet=" << eCharSet << ",";
  if (eFamily) f << "eFamily=" << eFamily << ",";
  if (ePitch) f << "ePitch=" << ePitch << ",";
  if (eWeight) f << "eWeight=" << eWeight << ",";
  if (eUnderline) f << "eUnderline=" << eUnderline << ",";
  if (eStrikeOut) f << "eStrikeOut=" << eStrikeOut << ",";
  if (eItalic) f << "eItalic=" << eItalic << ",";
  if (eLanguage) f << "eLanguage=" << eLanguage << ",";
  if (eWidthType) f << "eWidthType=" << eWidthType << ",";
  if (orientation) f << "orientation=" << orientation << ",";
  bool wordline, outline, shadow;
  uint8_t kerning;
  *input >> wordline >> outline >> shadow >> kerning;
  if (wordline) f << "wordline,";
  if (outline) f << "outline,";
  if (shadow) f << "shadow,";
  if (kerning) f << "kerning=" << (int) kerning << ",";
  if (zone.getHeaderVersion() >= 2) {
    bool vertical;
    int8_t relief;
    uint16_t cjkLanguage, emphasisMark;
    *input >> relief >> cjkLanguage >> vertical >> emphasisMark;
    if (relief) f << "relief=" << int(relief) << ",";
    if (cjkLanguage) f << "cjkLanguage=" << cjkLanguage << ",";
    if (vertical) f << "vertical,";
    if (emphasisMark) f << "emphasisMark=" << emphasisMark << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeVersionCompatHeader("StarFont");
  return true;
}

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

bool StarFileManager::readEditTextObject(StarZone &zone, long lastPos, StarDocument &doc)
{
  // svx_editobj.cxx EditTextObject::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  f << "Entries(EditTextObject):";
  uint16_t nWhich;
  uint32_t nStructSz;
  *input >> nWhich >> nStructSz;
  f << "structSz=" << nStructSz << ",";
  f << "nWhich=" << nWhich << ",";
  if ((nWhich != 0x22 && nWhich !=0x31) || pos+6+long(nStructSz)>lastPos) {
    STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: bad which/structSz\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  lastPos=pos+6+long(nStructSz);
  // BinTextObject::CreateData or BinTextObject::CreateData300
  uint16_t version=0;
  bool ownPool=true;
  if (nWhich==0x31) {
    *input>>version >> ownPool;
    if (version) f << "vers=" << version << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  shared_ptr<StarItemPool> pool;
  if (!ownPool) pool=doc.findItemPool(StarItemPool::T_EditEnginePool, true); // checkme
  if (!pool) pool=doc.getNewItemPool(StarItemPool::T_EditEnginePool);
  if (ownPool && !pool->read(zone)) {
    STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: can not read a pool\n"));
    ascFile.addPos(pos);
    ascFile.addNote("EditTextObject:###pool");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  pos=input->tell();
  f.str("");
  f << "EditTextObject:";
  uint16_t charSet=0;
  uint32_t nPara;
  if (nWhich==0x31) {
    uint16_t numPara;
    *input>>charSet >> numPara;
    nPara=numPara;
  }
  else
    *input>> nPara;
  if (charSet)  f << "char[set]=" << charSet << ",";
  if (nPara) f << "nPara=" << nPara << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int i=0; i<int(nPara); ++i) {
    pos=input->tell();
    f.str("");
    f << "EditTextObject[para" << i << "]:";
    for (int j=0; j<2; ++j) {
      librevenge::RVNGString text;
      if (!zone.readString(text) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: can not read a strings\n"));
        f << "###strings,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
        return true;
      }
      else if (!text.empty())
        f << (j==0 ? "name" : "style") << "=" << text.cstr() << ",";
    }
    uint16_t styleFamily;
    *input >> styleFamily;
    if (styleFamily) f << "styleFam=" << styleFamily << ",";
    std::vector<STOFFVec2i> limits;
    limits.push_back(STOFFVec2i(3989, 4033)); // EE_PARA_START 4033: EE_CHAR_END
    if (!doc.readItemSet(zone, limits, lastPos, pool.get(), false)) {
      STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: can not read item list\n"));
      f << "###item list,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "EditTextObjectA[para" << i << "]:";
    uint32_t nAttr;
    if (nWhich==0x22)
      *input>>nAttr;
    else {
      uint16_t nAttr16;
      *input>>nAttr16;
      nAttr=nAttr16;
    }
    if (input->tell()+long(nAttr)*8 > lastPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: can not read attrib list\n"));
      f << "###attrib list,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    f << "attrib=[";
    for (int j=0; j<int(nAttr); ++j) { // checkme, probably bad
      uint16_t which, start, end, surrogate;
      *input >> which;
      if (nWhich==0x22) *input >> surrogate;
      *input >> start >> end;
      if (nWhich!=0x22) *input >> surrogate;
      f << "wh=" << which << ":";
      f << "pos=" << start << "x" << end << ",";
      f << "surrogate=" << surrogate << ",";
    }
    f << "],";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }

  pos=input->tell();
  f.str("");
  f << "EditTextObject:";
  if (nWhich==0x22) {
    uint16_t marker;
    *input >> marker;
    if (marker==0x9999) {
      *input >> charSet;
      if (charSet)  f << "char[set]=" << charSet << ",";
    }
  }
  if (version>=400)
    f << "tmpMetric=" << input->readULong(2) << ",";
  if (version>=600) {
    f << "userType=" << input->readULong(2) << ",";
    f << "objSettings=" << input->readULong(4) << ",";
  }
  if (version>=601)
    f << "vertical=" << input->readULong(1) << ",";
  if (version>=602) {
    f << "scriptType=" << input->readULong(2) << ",";
    bool unicodeString;
    *input >> unicodeString;
    f << "strings=[";
    for (int i=0; unicodeString && i<int(nPara); ++i) {
      for (int s=0; s<2; ++s) {
        librevenge::RVNGString text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: can not read a strings\n"));
          f << "###strings,";
          break;
        }
        if (text.empty())
          f << "_,";
        else
          f << text.cstr() << ",";
      }
    }
    f << "],";
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarFileManager::readEditTextObject: find extra data\n"));
    f << "###";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
