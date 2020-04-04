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
#include <memory>
#include <sstream>

#include <librevenge/librevenge.h>

#include "StarZone.hxx"

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarGraphicStruct.hxx"
#include "StarObject.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectDraw.hxx"
#include "StarObjectMath.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarObjectText.hxx"
#include "StarItemPool.hxx"
#include "STOFFGraphicEncoder.hxx"
#include "STOFFGraphicListener.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFSpreadsheetEncoder.hxx"
#include "STOFFSpreadsheetListener.hxx"

#include "StarFileManager.hxx"

/** Internal: the structures of a StarFileManager */
namespace StarFileManagerInternal
{
////////////////////////////////////////
//! Internal: a structure use to read SfxMultiRecord zone of a StarFileManager
struct SfxMultiRecord {
  //! constructor
  explicit SfxMultiRecord(StarZone &zone)
    : m_zone(zone)
    , m_zoneType(0)
    , m_zoneOpened(false)
    , m_headerType(0)
    , m_headerVersion(0)
    , m_headerTag(0)
    , m_actualRecord(0)
    , m_numRecord(0)
    , m_contentSize(0)
    , m_startPos(0)
    , m_endPos(0)
    , m_offsetList()
    , m_extra("")
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
    if (m_zoneType==static_cast<unsigned char>(0xff)) {
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
      for (unsigned int off : r.m_offsetList) {
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
  unsigned char m_zoneType;
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
StarFileManager::StarFileManager()
  : m_state(new StarFileManagerInternal::State)
{
}

StarFileManager::~StarFileManager()
{
}

bool StarFileManager::readOLEDirectory(std::shared_ptr<STOFFOLEParser> oleParser, std::shared_ptr<STOFFOLEParser::OleDirectory> ole, STOFFEmbeddedObject &image, std::shared_ptr<StarObject> &res)
{
  image=STOFFEmbeddedObject();
  if (!oleParser || !ole || ole->m_inUse) {
    STOFF_DEBUG_MSG(("StarFileManager::readOLEDirectory: can not read an ole\n"));
    return false;
  }
  ole->m_inUse=true;
  StarObject object(nullptr, oleParser, ole); // do we need password here ?
  if (object.getDocumentKind()==STOFFDocument::STOFF_K_CHART) {
    auto chart=std::make_shared<StarObjectChart>(object, false);
    ole->m_parsed=true;
    if (chart->parse())
      res=chart;
  }
  else if (object.getDocumentKind()==STOFFDocument::STOFF_K_DRAW) {
    StarObjectDraw draw(object, false);
    ole->m_parsed=true;
    if (draw.parse()) {
      STOFFGraphicEncoder graphicEncoder;
      std::vector<STOFFPageSpan> pageList;
      int numPages;
      if (!draw.updatePageSpans(pageList, numPages)) {
        STOFFPageSpan ps;
        ps.m_pageSpan=1;
        pageList.push_back(ps);
      }
      STOFFGraphicListenerPtr graphicListener(new STOFFGraphicListener(STOFFListManagerPtr(), pageList, &graphicEncoder));
      graphicListener->startDocument();
      draw.sendPages(graphicListener);
      graphicListener->endDocument();
      graphicEncoder.getBinaryResult(image);
    }
  }
  else if (object.getDocumentKind()==STOFFDocument::STOFF_K_MATH) {
    auto math=std::make_shared<StarObjectMath>(object, false);
    ole->m_parsed=true;
    if (math->parse())
      res=math;
  }
  else if (object.getDocumentKind()==STOFFDocument::STOFF_K_SPREADSHEET) {
    StarObjectSpreadsheet spreadsheet(object, false);
    ole->m_parsed=true;
    if (spreadsheet.parse()) {
      STOFFSpreadsheetEncoder spreadsheetEncoder;
      std::vector<STOFFPageSpan> pageList;
      int numPages;
      if (!spreadsheet.updatePageSpans(pageList, numPages)) {
        STOFFPageSpan ps;
        ps.m_pageSpan=1;
        pageList.push_back(ps);
      }
      STOFFListManagerPtr listManager;
      STOFFSpreadsheetListenerPtr spreadsheetListener(new STOFFSpreadsheetListener(listManager, pageList, &spreadsheetEncoder));
      spreadsheetListener->startDocument();
      spreadsheet.send(spreadsheetListener);
      spreadsheetListener->endDocument();
      spreadsheetEncoder.getBinaryResult(image);
    }
  }
  else if (object.getDocumentKind()==STOFFDocument::STOFF_K_TEXT) {
    auto text=std::make_shared<StarObjectText>(object, false);
    ole->m_parsed=true;
    if (text->parse())
      res=text;
  }
  else {
    ole->m_parsed=true;
    // Ole-Object has persist elements, so...
    if (ole->m_hasCompObj) object.parse();
    STOFFOLEParser::OleDirectory &direct=*ole;
    std::vector<std::string> unparsedOLEs=direct.getUnparsedOles();
    size_t numUnparsed = unparsedOLEs.size();
    for (size_t i = 0; i < numUnparsed; i++) {
      std::string const &name = unparsedOLEs[i];
      STOFFInputStreamPtr stream = ole->m_input->getSubStreamByName(name.c_str());
      if (!stream.get()) {
        STOFF_DEBUG_MSG(("StarFileManager::readOLEDirectory: error: can not find OLE part: \"%s\"\n", name.c_str()));
        continue;
      }

      std::string::size_type pos = name.find_last_of('/');
      std::string base;
      if (pos == std::string::npos) base = name;
      else
        base = name.substr(pos+1);
      stream->setReadInverted(true);
      if (base=="SfxStyleSheets") {
        object.readSfxStyleSheets(stream,name);
        continue;
      }

      if (base=="StarImageDocument" || base=="StarImageDocument 4.0") {
        librevenge::RVNGBinaryData data;
        if (readImageDocument(stream,data,name) && !data.empty())
          image.add(data);
        continue;
      }
      // other
      if (base=="Ole-Object") {
        librevenge::RVNGBinaryData data;
        if (readOleObject(stream,data,name))
          image.add(data,"object/ole");
        continue;
      }
      libstoff::DebugFile asciiFile(stream);
      asciiFile.open(name);

      bool ok=false;
      if (base=="OutPlace Object")
        ok=readOutPlaceObject(stream, asciiFile);
      if (!ok) {
        libstoff::DebugStream f;
        if (base=="Standard") // can be Standard or STANDARD
          f << "Entries(STANDARD):";
        else
          f << "Entries(" << base << "):";
        asciiFile.addPos(0);
        asciiFile.addNote(f.str().c_str());
      }
      asciiFile.reset();
    }
  }
  // finally look if some content have image
  for (auto &content : ole->m_contentList) {
    librevenge::RVNGBinaryData data;
    std::string type;
    if (content.getImageData(data,type))
      image.add(data, type);
  }
  ole->m_inUse=false;
  return !image.isEmpty();
}

void StarFileManager::checkUnparsed(STOFFInputStreamPtr input, std::shared_ptr<STOFFOLEParser> oleParser, char const *password)
{
  if (!input || !oleParser) {
    STOFF_DEBUG_MSG(("StarFileManager::readOLEDirectory: can not find the input/ole parser\n"));
    return;
  }
  auto listDir=oleParser->getDirectoryList();
  for (auto &dir : listDir) {
    if (!dir || dir->m_parsed) continue;
    dir->m_parsed=true;
    StarObject object(password, oleParser, dir);
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_CHART) {
      StarObjectChart chart(object, false);
      chart.parse();
      continue;
    }
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_DRAW) {
      StarObjectDraw draw(object, false);
      draw.parse();
      continue;
    }
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_MATH) {
      StarObjectMath math(object, false);
      math.parse();
      continue;
    }
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_SPREADSHEET) {
      StarObjectSpreadsheet spreadsheet(object, false);
      spreadsheet.parse();
      continue;
    }
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_TEXT) {
      StarObjectText text(object, false);
      text.parse();
      continue;
    }
    // Ole-Object has persist elements, so...
    if (dir->m_hasCompObj) object.parse();
    auto unparsedOLEs=dir->getUnparsedOles();
    for (auto const &name : unparsedOLEs) {
      auto ole = input->getSubStreamByName(name.c_str());
      if (!ole.get()) {
        STOFF_DEBUG_MSG(("SDWParser::createZones: error: can not find OLE part: \"%s\"\n", name.c_str()));
        continue;
      }

      auto pos = name.find_last_of('/');
      std::string base;
      if (pos == std::string::npos) base = name;
      else base = name.substr(pos+1);
      ole->setReadInverted(true);
      if (base=="SfxStyleSheets") {
        object.readSfxStyleSheets(ole,name);
        continue;
      }

      if (base=="StarImageDocument" || base=="StarImageDocument 4.0") {
        librevenge::RVNGBinaryData data;
        readImageDocument(ole,data,name);
        continue;
      }
      if (base.compare(0,3,"Pic")==0) {
        librevenge::RVNGBinaryData data;
        std::string type;
        readEmbeddedPicture(ole,data,type,name);
        continue;
      }
      // other
      if (base=="Ole-Object") {
        librevenge::RVNGBinaryData data;
        readOleObject(ole,data,name);
        continue;
      }
      libstoff::DebugFile asciiFile(ole);
      asciiFile.open(name);

      bool ok=false;
      if (base=="OutPlace Object")
        ok=readOutPlaceObject(ole, asciiFile);
      if (!ok) {
        libstoff::DebugStream f;
        if (base=="Standard") // can be Standard or STANDARD
          f << "Entries(STANDARD):";
        else
          f << "Entries(" << base << "):";
        asciiFile.addPos(0);
        asciiFile.addNote(f.str().c_str());
      }
      asciiFile.reset();
    }
  }
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

bool StarFileManager::readEmbeddedPicture(std::shared_ptr<STOFFOLEParser> oleParser, std::string const &fileName, STOFFEmbeddedObject &image)
{
  if (!oleParser) {
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: called without OLE parser\n"));
    return false;
  }
  auto dir=oleParser->getDirectory("EmbeddedPictures");
  if (!dir || !dir->m_input || !dir->m_input->isStructured()) {
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: can not find the embedded picture directory\n"));
    return false;
  }
  std::string name("EmbeddedPictures/");
  name+=fileName;
  auto ole= dir->m_input->getSubStreamByName(name.c_str());
  if (!ole) {
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: can not find the picture %s\n", name.c_str()));
    return false;
  }
  librevenge::RVNGBinaryData data;
  std::string type;
  if (!readEmbeddedPicture(ole,data,type,name))
    return false;
  image.add(data, type);
  return true;
}

bool StarFileManager::readEmbeddedPicture(STOFFInputStreamPtr input, librevenge::RVNGBinaryData &data, std::string &dataType, std::string const &fileName)
try
{
  // see this Ole with classic bitmap format
  StarZone zone(input, fileName, "EmbeddedPicture", nullptr);

  libstoff::DebugFile &ascii=zone.ascii();
  ascii.open(fileName);
  data.clear();
  dataType="";
  // impgraph.cxx: ImpGraphic::ImplReadEmbedded

  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(EmbeddedPicture):";
  uint32_t nId;
  int32_t nLen;

  *input>>nId;
  if (nId==0x35465247 || nId==0x47524635) {
    if (nId==0x47524635)
      input->setReadInverted(!input->readInverted());
    if (!zone.openVersionCompatHeader()) {
      f << "###";
      STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: can not open the header\n"));
      ascii.addPos(0);
      ascii.addNote(f.str().c_str());
      return false;
    }
    int32_t nType;
    int32_t sizeX, sizeY;
    *input >> nType >> nLen >> sizeX>> sizeY;
    if (nType)
      f << "type=" << nType << ",";
    f << "size=" << sizeX << "x" << sizeY << ",";
    // mapmod.cxx: operator>>(..., ImplMapMode& )
    if (!zone.openVersionCompatHeader()) {
      f << "###";
      STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: can not open the mapmod header\n"));
    }
    else {
      uint16_t nMapMode;
      int32_t nOffsX, nOffsY, nScaleNumX, nScaleDenomX, nScaleNumY, nScaleDenomY;
      bool mbSimple;
      *input >> nMapMode >> nOffsX>> nOffsY;
      if (nMapMode) f << "mapMode=" << nMapMode << ",";
      if (nOffsX || nOffsY)
        f << "offs=" << nOffsX << "x" << nOffsY << ",";
      *input >> nScaleNumX >> nScaleDenomX >> nScaleNumY >> nScaleDenomY >> mbSimple;
      f << "scaleX=" << nScaleNumX << "/" << nScaleDenomX << ",";
      f << "scaleY=" << nScaleNumY << "/" << nScaleDenomY << ",";
      if (mbSimple) f << "simple,";
      zone.closeVersionCompatHeader("StarFileManager");
    }
    zone.closeVersionCompatHeader("StarFileManager");
  }
  else {
    if (nId>0x100) {
      input->seek(0, librevenge::RVNG_SEEK_SET);
      input->setReadInverted(!input->readInverted());
      *input>>nId;
    }
    if (nId)
      f << "type=" << nId << ",";
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
  if (nLen<10 || input->size()!=input->tell()+nLen) {
    f << "###";
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: the length seems bad\n"));
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return false;
  }
  long pictPos=input->tell();
  auto header=int(input->readULong(2));
  input->seek(pictPos, librevenge::RVNG_SEEK_SET);
  std::string extension("pict");
  if (header==0x4142 || header==0x4d42) {
    dataType="image/bm";
    extension="bm";
#ifdef DEBUG_WITH_FILES
    StarBitmap bitmap;
    bitmap.readBitmap(zone, true, input->size(), data, dataType);
#endif
  }
  else if (header==0x5653) {
#ifdef DEBUG_WITH_FILES
    readSVGDI(zone);
#endif
    dataType="image/svg";
    extension="svgdi";
  }
  else if (header==0xcdd7) {
    dataType="image/wmf";
    extension="wmf";
  }
  else if (header==0x414e) {
    bool nat5=input->readULong(4)==0x3554414e;
    input->seek(pictPos, librevenge::RVNG_SEEK_SET);
    StarGraphicStruct::StarGraphic graphic;
    if (nat5 && graphic.read(zone,input->size())) {
      ascii.addPos(0);
      ascii.addNote(f.str().c_str());
      dataType=graphic.m_object.m_typeList.empty() ? "image/pict" : graphic.m_object.m_typeList[0];
      if (!graphic.m_object.m_dataList.empty())
        data=graphic.m_object.m_dataList[0];
      return true;
    }
    dataType="image/pict";
    f << "###unknown";
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: find unknown format\n"));
  }
  else if (header==0x4356) { // VLCMTF
    dataType="image/pict";
    extension="pict";
  }
  else {
    dataType="image/pict";
    f << "###unknown";
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: find unknown format\n"));
  }
  f << extension << ",";
  if (input->tell()==pictPos) ascii.addDelimiter(input->tell(),'|');
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());
  ascii.skipZone(pictPos+4, input->size());

  input->seek(pictPos, librevenge::RVNG_SEEK_SET);
  if (!input->readEndDataBlock(data)) {
    data.clear();
    STOFF_DEBUG_MSG(("StarFileManager::readEmbeddedPicture: can not read image content\n"));
    return true;
  }
#ifdef DEBUG_WITH_FILES
  libstoff::Debug::dumpFile(data, (fileName+"."+extension).c_str());
#endif
  return true;
}
catch (...)
{
  return false;
}

bool StarFileManager::readOleObject(STOFFInputStreamPtr input, librevenge::RVNGBinaryData &data, std::string const &fileName)
{
  // see this Ole only once with PaintBrush data ?
  libstoff::DebugFile ascii(input);
  ascii.open(fileName);
  ascii.skipZone(0, input->size());

  input->seek(0, librevenge::RVNG_SEEK_SET);
  if (!input->readEndDataBlock(data)) {
    STOFF_DEBUG_MSG(("StarFileManager::readOleObject: can not read image content\n"));
    data.clear();
    return false;
  }
#ifdef DEBUG_WITH_FILES
  libstoff::Debug::dumpFile(data, (fileName+".ole").c_str());
#endif
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
    std::vector<uint32_t> string;
    if (!zone.readString(string)||input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readFont: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeVersionCompatHeader("StarFont");
      return true;
    }
    if (!string.empty()) f << (i==0 ? "name" : "style") << "=" << libstoff::getString(string).cstr() << ",";
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
  if (kerning) f << "kerning=" << int(kerning) << ",";
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
  if (zone.getHeaderVersion() >= 3) {
    int16_t overline;
    *input>>overline;
    if (overline) f << "overline=" << overline << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeVersionCompatHeader("StarFont");
  return true;
}

bool StarFileManager::readJobSetUp(StarZone &zone, bool useJobLen)
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
  auto len=int(input->readULong(2));
  f << "nLen=" << len << ",";
  if (len==0) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  if (useJobLen) {
    if (pos+len>lastPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readJobSetUp: the jobs len seems bad\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    else
      lastPos=pos+len;
  }
  bool ok=false;
  int nSystem=0;
  if (input->tell()+2+64+3*32<=lastPos) {
    nSystem=int(input->readULong(2));
    f << "system=" << nSystem << ",";
    for (int i=0; i<4; ++i) {
      long actPos=input->tell();
      int const length=(i==0 ? 64 : 32);
      std::string name("");
      for (int c=0; c<length; ++c) {
        auto ch=char(input->readULong(1));
        if (ch==0) break;
        name+=ch;
      }
      if (!name.empty()) {
        static char const *wh[]= { "printerName", "deviceName", "portName", "driverName" };
        f << wh[i] << "=" << name << ",";
      }
      input->seek(actPos+length, librevenge::RVNG_SEEK_SET);
    }
    ok=true;
  }
  if (ok && nSystem<0xfffe) {
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
    driverDataLen=int(input->readULong(4));
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
        std::vector<uint32_t> text;
        while (input->tell()<lastPos) {
          for (int i=0; i<2; ++i) {
            if (!zone.readString(text)) {
              f << "###string,";
              ok=false;
              break;
            }
            f << libstoff::getString(text).cstr() << (i==0 ? ':' : ',');
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
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  return true;
}

bool StarFileManager::readSVGDI(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  long lastPos=zone.getRecordLevel()==0 ? input->size() : zone.getRecordLastPosition();
  // cvtsvm.cxx: SVMConverter::ImplConvertFromSVM1
  libstoff::DebugStream f;
  f << "Entries(ImageSVGDI)[" << zone.getRecordLevel() << "]:";
  std::string code;
  for (int i=0; i<5; ++i) code+=char(input->readULong(1));
  if (code!="SVGDI") {
    input->seek(0, librevenge::RVNG_SEEK_SET);
    return false;
  }
  uint16_t sz;
  int16_t version;
  int32_t width, height;
  *input >> sz >> version >> width >> height;
  long endPos=pos+5+sz;
  if (version!=200 || sz<42 || endPos>lastPos) {
    STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: find unknown version\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  f << "size=" << width << "x" << height << ",";
  // map mode
  int16_t unit;
  int32_t orgX, orgY, nXNum, nXDenom, nYNum, nYDenom;
  *input >> unit >> orgX >> orgY >> nXNum >> nXDenom >> nYNum >> nYDenom;
  if (unit) f << "unit=" << unit << ",";
  f << "orig=" << orgX << "x" << orgY << ",";
  f << "x=" << nXNum << "/" << nXDenom << ",";
  f << "y=" << nYNum << "/" << nYDenom << ",";

  int32_t nActions;
  *input >> nActions;
  f << "actions=" << nActions << ",";
  if (input->tell()!=endPos)
    ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  input->seek(endPos, librevenge::RVNG_SEEK_SET);
  uint32_t nUnicodeCommentActionNumber=0;
  for (int32_t i=0; i<nActions; ++i) {
    pos=input->tell();
    f.str("");
    f << "ImageSVGDI[" << i << "]:";
    int16_t type;
    int32_t nActionSize;
    *input>>type>>nActionSize;
    long endDataPos=pos+2+nActionSize;
    if (nActionSize<4 || (lastPos-pos-2)<nActionSize || endDataPos>lastPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad size\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    unsigned char col[3];
    STOFFColor color;
    int32_t nTmp, nTmp1;
    std::vector<uint32_t> text;
    switch (type) {
    case 1:
      f << "pixel=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      for (unsigned char &c : col) c=static_cast<unsigned char>(input->readULong(2)>>8);
      f << "col=" << STOFFColor(col[0],col[1],col[2]) << ",";
      break;
    case 2:
      f << "point=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 3:
      f << "line=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 4:
      f << "rect=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      *input >> nTmp >> nTmp1;
      if (nTmp || nTmp1) f << "round=" << nTmp << "x" << nTmp1 << ",";
      break;
    case 5:
      f << "ellipse=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 6:
    case 7:
      f << (type==6 ? "arc" : "pie")<< "=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "pt1=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "pt2=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 8:
    case 9:
      f << (type==8 ? "rect[invert]" : "rect[highlight") << "="
        << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 10:
    case 11:
      f << (type==10 ? "polyline" : "polygon") << ",";
      *input >> nTmp;
      if (nTmp<0 || nTmp>(endDataPos-input->tell())/8) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad number of points\n"));
        f << "###nPts=" << nTmp << ",";
        break;
      }
      f << "pts=[";
      for (int pt=0; pt<int(nTmp); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "],";
      break;
    case 12:
    case 1024:
    case 1025:
    case 1030:
      f << (type==12 ? "polypoly" : type==1024 ? "transparent[comment]" :
            type==1025 ? "hatch[comment]" : "gradient[comment]") << ",";
      *input >> nTmp;
      for (int poly=0; poly<int(nTmp) && !input->isEnd(); ++poly) {
        *input >> nTmp1;
        if (nTmp1<0 || nTmp1>(endDataPos-input->tell())/8) {
          STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad number of points\n"));
          f << "###poly[nPts=" << nTmp1 << "],";
          break;
        }
        f << "poly" << poly << "=[";
        for (int pt=0; pt<int(nTmp1); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
        f << "],";
      }
      if (type==1024) {
        f << "nTrans=" << input->readULong(2) << ",";
        f << "nComment=" << input->readULong(4) << ",";
      }
      if (type==1025) {
        f << "style=" << input->readULong(2) << ",";
        for (unsigned char &c : col) c=static_cast<unsigned char>(input->readULong(2)>>8);
        f << "col=" << STOFFColor(col[0],col[1],col[2]) << ",";
        f << "distance=" << input->readLong(4) << ",";
        f << "nComment=" << input->readULong(4) << ",";
      }
      if (type==1030) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: reading gradient is not implemented\n"));
        f << "###gradient+following";
        input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
      }
      break;
    case 13:
    case 15: {
      f << (type==13 ? "text" : "stretch") << ",";
      f << "pos=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      int32_t nIndex, nLen;
      *input>>nIndex>>nLen >> nTmp;
      if (nIndex) f << "index=" << nIndex << ",";
      if (nLen) f << "len=" << nLen << ",";
      if (type==15) f << "nWidth=" << input->readLong(4) << ",";
      if (nTmp<0 || input->tell()+nTmp>endDataPos) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad string\n"));
        f << "###string";
        break;
      }
      text.clear();
      for (int c=0; c<int(nTmp); ++c) text.push_back(static_cast<uint32_t>(input->readULong(1)));
      input->seek(1, librevenge::RVNG_SEEK_CUR);
      f << libstoff::getString(text).cstr() << ",";
      if (nUnicodeCommentActionNumber!=static_cast<uint32_t>(i)) break;
      uint16_t type1;
      uint32_t len;
      *input >> type1 >> len;
      if (long(len)<4 || input->tell()+long(len)>endDataPos) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad string\n"));
        f << "###unicode";
        break;
      }
      if (type1==1032) {
        text.clear();
        int nUnicode=int(len-4)/2;
        for (int c=0; c<nUnicode; ++c) text.push_back(static_cast<uint32_t>(input->readULong(2)));
        f << libstoff::getString(text).cstr() << ",";
      }
      else {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: unknown data\n"));
        f << "###unknown";
        input->seek(long(len)-4, librevenge::RVNG_SEEK_CUR);
      }
      break;
    }
    case 14: {
      f << "text[array],";
      f << "pos=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      int32_t nIndex, nLen, nAryLen;
      *input>>nIndex>>nLen >> nTmp >> nAryLen;
      if (nTmp<0 || nAryLen<0 || nAryLen>(endDataPos-input->tell()-nTmp)/4) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad string\n"));
        f << "###string";
        break;
      }
      text.clear();
      for (int c=0; c<int(nTmp); ++c) text.push_back(static_cast<uint32_t>(input->readULong(1)));
      input->seek(1, librevenge::RVNG_SEEK_CUR);
      f << libstoff::getString(text).cstr() << ",";
      f << "ary=[";
      for (int ary=0; ary<int(nAryLen); ++ary) f << input->readLong(4) << ",";
      f << "],";
      if (nUnicodeCommentActionNumber!=static_cast<uint32_t>(i)) break;
      uint16_t type1;
      uint32_t len;
      *input >> type1 >> len;
      if (long(len)<4 || input->tell()+long(len)>endDataPos) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad string\n"));
        f << "###unicode";
        break;
      }
      if (type1==1032) {
        text.clear();
        int nUnicode=int(len-4)/2;
        for (int c=0; c<nUnicode; ++c) text.push_back(static_cast<uint32_t>(input->readULong(2)));
        f << libstoff::getString(text).cstr() << ",";
      }
      else {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: unknown data\n"));
        f << "###unknown";
        input->seek(long(len)-4, librevenge::RVNG_SEEK_CUR);
      }

      break;
    }
    case 16:
      STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: reading icon is not implemented\n"));
      f << "###icon";
      break;
    case 17:
    case 18:
    case 32: {
      f << (type==17 ? "bitmap" : type==18 ? "bitmap[scale]" : "bitmap[scale2]");
      f << "pos=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      if (type>=17) f << "scale=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      if (type==32) {
        f << "pos2=" << input->readLong(4) << "x" << input->readLong(4) << ",";
        f << "scale2=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      }
      StarBitmap bitmap;
      librevenge::RVNGBinaryData data;
      std::string dataType;
      if (!bitmap.readBitmap(zone, false, endDataPos, data, dataType))
        f << "###bitmap,";
      break;
    }
    case 19:
      f << "pen,";
      for (unsigned char &c : col) c=static_cast<unsigned char>(input->readULong(2)>>8);
      color=STOFFColor(col[0],col[1],col[2]);
      if (!color.isBlack()) f << "col=" << color << ",";
      f << "penWidth=" << input->readULong(4) << ",";
      f << "penStyle=" << input->readULong(2) << ",";
      break;
    case 20: {
      f << "font,";
      for (int c=0; c<2; ++c) {
        for (unsigned char &j : col) j=static_cast<unsigned char>(input->readULong(2)>>8);
        color=STOFFColor(col[0],col[1],col[2]);
        if ((c==1&&!color.isWhite()) || (c==0&&!color.isBlack()))
          f << (c==0 ? "col" : "col[fill]") << "=" << color << ",";
      }
      long actPos=input->tell();
      if (actPos+62>endDataPos) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: the zone seems too short\n"));
        f << "###short";
        break;
      }
      std::string name("");
      for (int c=0; c<32; ++c) {
        auto ch=char(input->readULong(1));
        if (!ch) break;
        name+=ch;
      }
      f << name << ",";
      input->seek(actPos+32, librevenge::RVNG_SEEK_SET);
      f << "size=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      int16_t nCharSet, nFamily, nPitch, nAlign, nWeight, nUnderline, nStrikeout, nCharOrient, nLineOrient;
      bool bItalic, bOutline, bShadow, bTransparent;
      *input >> nCharSet >> nFamily >> nPitch >> nAlign >> nWeight >> nUnderline >> nStrikeout >> nCharOrient >> nLineOrient;
      if (nCharSet) f << "char[set]=" << nCharSet << ",";
      if (nFamily) f << "family=" << nFamily << ",";
      if (nPitch) f << "pitch=" << nPitch << ",";
      if (nAlign) f << "align=" << nAlign << ",";
      if (nWeight) f << "weight=" << nWeight << ",";
      if (nUnderline) f << "underline=" << nUnderline << ",";
      if (nStrikeout) f << "strikeout=" << nStrikeout << ",";
      if (nCharOrient) f << "charOrient=" << nCharOrient << ",";
      if (nLineOrient) f << "lineOrient=" << nLineOrient << ",";
      *input >> bItalic >> bOutline >> bShadow >> bTransparent;
      if (bItalic) f << "italic,";
      if (bOutline) f << "outline,";
      if (bShadow) f << "shadow,";
      if (bTransparent) f << "transparent,";
      break;
    }
    case 21: // unsure
    case 22:
      f << (type==21 ? "brush[back]" : "brush[fill]") << ",";
      for (unsigned char &j : col) j=static_cast<unsigned char>(input->readULong(2)>>8);
      f << STOFFColor(col[0],col[1],col[2]) << ",";
      input->seek(6, librevenge::RVNG_SEEK_CUR); // unknown
      f << "style=" << input->readLong(2) << ",";
      input->seek(2, librevenge::RVNG_SEEK_CUR); // unknown
      break;
    case 23:
      f << "map[mode],";
      *input >> unit >> orgX >> orgY >> nXNum >> nXDenom >> nYNum >> nYDenom;
      if (unit) f << "unit=" << unit << ",";
      f << "orig=" << orgX << "x" << orgY << ",";
      f << "x=" << nXNum << "/" << nXDenom << ",";
      f << "y=" << nYNum << "/" << nYDenom << ",";
      break;
    case 24: {
      f << "clip[region],";
      int16_t clipType, bIntersect;
      *input >> clipType >> bIntersect;
      f << "rect=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      if (bIntersect) f << "intersect,";
      switch (clipType) {
      case 0:
        break;
      case 1:
        f << "rect2=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
          << input->readLong(4) << "x" << input->readLong(4) << ",";
        break;
      case 2:
        *input >> nTmp;
        if (nTmp<0 || nTmp>(endDataPos-input->tell())/8) {
          STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad number of points\n"));
          f << "###nPts=" << nTmp << ",";
          break;
        }
        f << "poly=[";
        for (int pt=0; pt<int(nTmp); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
        f << "],";
        break;
      case 3:
        *input >> nTmp;
        for (int poly=0; poly<int(nTmp) && !input->isEnd(); ++poly) {
          *input >> nTmp1;
          if (nTmp1<0 || nTmp1>(endDataPos-input->tell())/8) {
            STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: bad number of points\n"));
            f << "###poly[nPts=" << nTmp1 << "],";
            break;
          }
          f << "poly" << poly << "=[";
          for (int pt=0; pt<int(nTmp1); ++pt) f << input->readLong(4) << "x" << input->readLong(4) << ",";
          f << "],";
        }
        break;
      default:
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: find unknown clip type\n"));
        f << "###type=" << clipType << ",";
        break;
      }
      break;
    }
    case 25:
      f << "raster=" << input->readULong(2) << ","; // 1 invert, 4,5: xor other paint
      break;
    case 26:
      f << "push,";
      break;
    case 27:
      f << "pop,";
      break;
    case 28:
      f << "clip[move]=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 29:
      f << "clip[rect]=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      break;
    case 30: // checkme
    case 1029: {
      f << (type==30 ? "mtf" : "floatComment") << ",";;
      // gdimtf.cxx operator>>(... GDIMetaFile )
      std::string name("");
      for (int c=0; c<6; ++c) name+=char(input->readULong(1));
      if (name!="VCLMTF") {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: find unexpected header\n"));
        f << "###name=" << name << ",";
        break;
      }
      if (!zone.openVersionCompatHeader()) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: can not open compat header\n"));
        f << "###compat,";
        break;
      }
      f << "compress=" << input->readULong(4) << ",";
      // map mode
      *input >> unit >> orgX >> orgY >> nXNum >> nXDenom >> nYNum >> nYDenom;
      f << "map=[";
      if (unit) f << "unit=" << unit << ",";
      f << "orig=" << orgX << "x" << orgY << ",";
      f << "x=" << nXNum << "/" << nXDenom << ",";
      f << "y=" << nYNum << "/" << nYDenom << ",";
      f << "],";
      f << "size=" << input->readULong(4) << ",";
      uint32_t nCount;
      *input >> nCount;
      if (nCount) f << "nCount=" << nCount << ",";
      if (input->tell()!=zone.getRecordLastPosition()) {
        // for (int act=0; act<nCount; ++act) MetaAction::ReadMetaAction();
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: reading mtf action zones is not implemented\n"));
        ascFile.addPos(input->tell());
        ascFile.addNote("ImageSVGDI:###listMeta");
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      }
      zone.closeVersionCompatHeader("ImageSVGDI");
      if (type!=30) {
        f << "orig=" << input->readLong(4) << "x" << input->readLong(4) << ",";
        f << "sz=" << input->readULong(4) << ",";
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: reading gradient is not implemented\n"));
        f << "###gradient+following";
        input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
      }
      break;
    }
    case 33: {
      f << "gradient,";
      f << "rect=" << input->readLong(4) << "x" << input->readLong(4) << "<->"
        << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "style=" << input->readULong(2) << ",";
      for (int c=0; c<2; ++c) {
        for (unsigned char &j : col) j=static_cast<unsigned char>(input->readULong(2)>>8);
        color=STOFFColor(col[0],col[1],col[2]);
        f << "col" << c << "=" << color << ",";
      }
      f << "angle=" << input->readLong(2) << ",";
      f << "border=" << input->readLong(2) << ",";
      f << "offs=" << input->readLong(2) << "x" << input->readLong(2) << ",";
      f << "intensity=" << input->readLong(2) << "<->" << input->readLong(2) << ",";
      break;
    }
    case 1026:
      f << "refpoint[comment],";
      f << "pt=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "set=" << input->readULong(1) << ",";
      f << "nComments=" << input->readULong(4) << ",";
      break;
    case 1027:
      f << "textline[color,comment],";
      for (unsigned char &j : col) j=static_cast<unsigned char>(input->readULong(2)>>8);
      f << "col=" << STOFFColor(col[0],col[1],col[2]) << ",";
      f << "set=" << input->readULong(1) << ",";
      f << "nComments=" << input->readULong(4) << ",";
      break;
    case 1028:
      f << "textline[comment],";
      f << "pt=" << input->readLong(4) << "x" << input->readLong(4) << ",";
      f << "width=" << input->readLong(4) << ",";
      f << "strikeOut=" << input->readULong(4) << ",";
      f << "underline=" << input->readULong(4) << ",";
      f << "nComments=" << input->readULong(4) << ",";
      break;
    case 1031: {
      f << "comment[comment],";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: can not read the text\n"));
        f << "###text,";
        break;
      }
      f << libstoff::getString(text).cstr() << ",";
      f << "value=" << input->readULong(4) << ",";
      long size=input->readLong(4);
      if (size<0 || input->tell()+size+4>endDataPos) {
        STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: size seems bad\n"));
        f << "###size=" << size << ",";
        break;
      }
      if (size) {
        f << "###unknown,";
        ascFile.addDelimiter(input->tell(),'|');
        input->seek(size, librevenge::RVNG_SEEK_CUR);
      }
      f << "nComments=" << input->readULong(4) << ",";
      break;
    }
    case 1032:
      f << "unicode[next],";
      nUnicodeCommentActionNumber=uint32_t(i)+1;
      break;
    default:
      STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: find unimplement type\n"));
      f << "###type=" << type << ",";
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
      break;
    }
    if (input->tell()!=endDataPos) {
      STOFF_DEBUG_MSG(("StarFileManager::readSVGDI: find extra data\n"));
      f << "###extra,";
      ascFile.addDelimiter(input->tell(),'|');
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
  }

  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
