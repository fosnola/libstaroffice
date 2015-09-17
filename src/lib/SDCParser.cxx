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
#include "StarDocument.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarZone.hxx"
#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"

#include "SDCParser.hxx"

/** Internal: the structures of a SDCParser */
namespace SDCParserInternal
{
////////////////////////////////////////
//! Internal: a structure use to read ScMultiRecord zone of a SDCParser
struct ScMultiRecord {
  //! constructor
  ScMultiRecord(StarZone &zone) : m_zone(zone), m_zoneOpened(false), m_actualRecord(0), m_numRecord(0),
    m_startPos(0), m_endPos(0), m_endContentPos(0), m_endRecordPos(0), m_offsetList(), m_extra("")
  {
  }
  //! destructor
  ~ScMultiRecord()
  {
    if (m_zoneOpened)
      close("Entries(BADScMultiRecord):###");
  }
  //! try to open a zone
  bool open()
  {
    if (m_zoneOpened) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord: oops a record has been opened\n"));
      return false;
    }
    m_actualRecord=m_numRecord=0;
    m_startPos=m_endPos=m_endContentPos=m_endRecordPos=0;
    m_offsetList.clear();

    STOFFInputStreamPtr input=m_zone.input();
    long pos=input->tell();
    long lastPos=m_zone.getRecordLevel() ? m_zone.getRecordLastPosition() : input->size();
    if (!m_zone.openSCRecord()) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    m_zoneOpened=true;
    m_startPos=input->tell();
    m_endPos=m_zone.getRecordLastPosition();
    // sc_rechead.cxx ScMultipleReadHeader::ScMultipleReadHeader
    if (m_endPos+6>lastPos) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::open: oops the zone seems too short\n"));
      m_extra="###zoneShort,";
      return false;
    }
    input->seek(m_endPos, librevenge::RVNG_SEEK_SET);
    uint16_t id;
    uint32_t tableLen;
    *input>>id >> tableLen;
    m_endRecordPos=input->tell()+long(tableLen);
    if (id!=0x4200 || m_endRecordPos > lastPos) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::open: oops can not find the size data\n"));
      m_extra="###zoneShort,";
      m_endRecordPos=0;
      return false;
    }
    m_numRecord=tableLen/4;
    for (uint32_t i=0; i<m_numRecord; ++i)
      m_offsetList.push_back((uint32_t) input->readULong(4));
    input->seek(m_startPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  //! try to close a zone
  void close(std::string const &wh)
  {
    if (!m_zoneOpened) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::close: can not find any opened zone\n"));
      return;
    }
    if (m_endContentPos>0) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::close: argh, the current content is not closed, let close it\n"));
      closeContent(wh);
    }

    m_zoneOpened=false;
    STOFFInputStreamPtr input=m_zone.input();
    if (input->tell()<m_endPos && input->tell()+4>=m_endPos) { // small diff is possible
      m_zone.ascii().addDelimiter(input->tell(),'|');
      input->seek(m_zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()==m_endPos)
      input->seek(m_zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    m_zone.closeSCRecord(wh);
    if (m_endRecordPos>0)
      input->seek(m_endRecordPos, librevenge::RVNG_SEEK_SET);
  }
  //! returns true if a content is opened
  bool isContentOpened() const
  {
    return m_endContentPos>0;
  }
  //! try to go to the new content positon
  bool openContent(std::string const &wh)
  {
    if (m_endContentPos>0) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::openContent: argh, the current content is not closed, let close it\n"));
      closeContent(wh);
    }
    STOFFInputStreamPtr input=m_zone.input();
    if (m_actualRecord >= m_numRecord || m_actualRecord >= uint32_t(m_offsetList.size()) ||
        input->tell()+long(m_offsetList[size_t(m_actualRecord)])>m_endPos)
      return false;
    // ScMultipleReadHeader::StartEntry
    m_endContentPos=input->tell()+long(m_offsetList[size_t(m_actualRecord)]);
    ++m_actualRecord;
    return true;
  }
  //! try to go to the new content positon
  bool closeContent(std::string const &wh)
  {
    if (m_endContentPos<=0) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::closeContent: no content opened\n"));
      return false;
    }
    STOFFInputStreamPtr input=m_zone.input();
    if (input->tell()<m_endContentPos && input->tell()+4>=m_endContentPos) { // small diff is possible ?
      m_zone.ascii().addDelimiter(input->tell(),'|');
      input->seek(m_endContentPos, librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()!=m_endContentPos) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::getContent: find extra data\n"));
      m_zone.ascii().addPos(input->tell());
      libstoff::DebugStream f;
      f << wh << ":###extra";
      m_zone.ascii().addNote(f.str().c_str());
      input->seek(m_endContentPos, librevenge::RVNG_SEEK_SET);
    }
    m_endContentPos=0;
    return true;
  }
  //! returns the last content position
  long getContentLastPosition() const
  {
    if (m_endContentPos<=0) {
      STOFF_DEBUG_MSG(("SDCParserInternal::ScMultiRecord::getContentLastPosition: no content opened\n"));
      return m_endPos;
    }
    return m_endContentPos;
  }

  //! basic operator<< ; print header data
  friend std::ostream &operator<<(std::ostream &o, ScMultiRecord const &r)
  {
    if (!r.m_zoneOpened) {
      o << r.m_extra;
      return o;
    }
    if (r.m_numRecord) o << "num[record]=" << r.m_numRecord << ",";
    if (!r.m_offsetList.empty()) {
      o << "offset=[";
      for (size_t i=0; i<r.m_offsetList.size(); ++i) o << r.m_offsetList[i] << ",";
      o << "],";
    }
    o << r.m_extra;
    return o;
  }
protected:
  //! the main zone
  StarZone &m_zone;
  //! true if a SfxRecord has been opened
  bool m_zoneOpened;
  //! the actual record
  uint32_t m_actualRecord;
  //! the number of record
  uint32_t m_numRecord;
  //! the start of data position
  long m_startPos;
  //! the end of data position
  long m_endPos;
  //! the end of the content position
  long m_endContentPos;
  //! the end of the record position
  long m_endRecordPos;
  //! the list of offset
  std::vector<uint32_t> m_offsetList;
  //! extra data
  std::string m_extra;
private:
  ScMultiRecord(ScMultiRecord const &orig);
  ScMultiRecord &operator=(ScMultiRecord const &orig);
};

////////////////////////////////////////
//! Internal: a table of a SDCParser
class Table
{
public:
  //! constructor
  Table(int loadingVers, int maxRow) : m_loadingVersion(loadingVers), m_maxRow(maxRow)
  {
  }
  //! returns the load version
  int getLoadingVersion() const
  {
    return m_loadingVersion;
  }
  //! returns the maximum number of columns
  static int getMaxCols()
  {
    return 255;
  }
  //! returns the maximum number of row
  int getMaxRows() const
  {
    return m_maxRow;
  }

  //! the loading version
  int m_loadingVersion;
  //! the maximum number of row
  int m_maxRow;
};

////////////////////////////////////////
//! Internal: the state of a SDCParser
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
SDCParser::SDCParser() : m_state(new SDCParserInternal::State)
{
}

SDCParser::~SDCParser()
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
bool SDCParser::readCalcDocument(STOFFInputStreamPtr input, std::string const &name, StarDocument &document)
try
{
  StarZone zone(input, name, "SWCalcDocument", document.getPassword()); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCCalcDocument):";
  // sc_docsh.cxx: ScDocShell::Load then sc_documen2.cxx ScDocument::Load
  uint16_t nId;
  *input>>nId;
  if ((nId>>8)!=0x42) {
    /* if the zone has a password, we can retrieve it knowing that nId must begin by 0x42

       TODO: we must also check if the user has given a password and
       if the user mask does correspond to the real mask.
    */
    input=StarEncryption::decodeStream(input, StarEncryption::getMaskToDecodeStream(uint8_t(nId>>8), 0x42));
    if (input) {
      zone.setInput(input);
      input->seek(0, librevenge::RVNG_SEEK_SET);
      *input>>nId;
    }
  }
  if ((nId!=0x4220 && nId!=0x422d)||!zone.openSCRecord()) {
    STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read the document id\n"));
    f << "###";
    ascFile.addPos(0);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  int version=0, maxRow=8191;
  while (!input->isEnd() && input->tell()<lastPos) {
    long pos=input->tell();
    uint16_t subId;
    *input>>subId;
    f.str("");
    f << "SCCalcDocument[" << std::hex << subId << std::dec << "]:";
    bool ok=false;
    switch (subId) {
    case 0x4222: {
      f << "table,";
      SDCParserInternal::Table table(version, maxRow);
      ok=readSCTable(zone, table, document);
      break;
    }
    case 0x4224: {
      f << "rangeName,";
      // sc_rangenam.cxx ScRangeName::Load
      SDCParserInternal::ScMultiRecord scRecord(zone);
      ok=scRecord.open();
      if (!ok) break;
      uint16_t count, sharedMaxIndex, dummy;
      if (version >= 3)
        *input >> sharedMaxIndex >> count;
      else
        *input >> sharedMaxIndex >> dummy >> count;
      f << "index[sharedMax]=" << sharedMaxIndex << ";";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      for (int i=0; i<int(count); ++i)  {
        pos=input->tell();
        f.str("");
        f << "Entries(SCRange):";
        if (!scRecord.openContent("SCCalcDocument")) {
          f << "###";
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument:can not find some content\n"));
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          break;
        }
        // sc_rangenam.cxx ScRangeData::ScRangeData
        long endDataPos=scRecord.getContentLastPosition();
        librevenge::RVNGString string;
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
          f << "###string";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          continue;
        }
        f << string.cstr() << ",";
        if (version >= 3) {
          uint32_t nPos;
          uint16_t rangeType, index;
          uint8_t nData;
          *input >> nPos >> rangeType >> index >> nData;
          int row=int(nPos&0xFFFF);
          int col=int((nPos>>16)&0xFF);
          int table=int((nPos>>24)&0xFF);
          f << "pos=" << row << "x" << col;
          if (table) f << "x" << table;
          f << ",";
          f << "range[type]=" << rangeType << ",";
          f << "index=" << index << ",";
          if (nData&0xf) input->seek((nData&0xf), librevenge::RVNG_SEEK_CUR);
          if (!readSCFormula(zone, STOFFVec2i(row,col), version, endDataPos) || input->tell()>endDataPos)
            f << "###";
        }
        else {
          uint16_t row, col, table, tokLen, rangeType, index;
          *input >> col >> row >> table >> rangeType >> index >> tokLen;
          f << "pos=" << row << "x" << col;
          if (table) f << "x" << table;
          f << ",";
          f << "range[type]=" << rangeType << ",";
          f << "index=" << index << ",";
          if (tokLen && (!readSCFormula3(zone, STOFFVec2i(row,col), version, endDataPos) || input->tell()>endDataPos))
            f << "###";
        }
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        scRecord.closeContent("SCCalcDocument");
      }
      scRecord.close("SCCalcDocument");
      pos=input->tell();
      break;
    }
    case 0x4225:
    case 0x4226:
    case 0x4227:
    case 0x422e:
    case 0x422f:
    case 0x4230:
    case 0x4231:
    case 0x4234:
    case 0x4239: {
      std::string what(subId==0x4225 ? "dbCollect" : subId==0x4226 ? "dbPivot" :
                       subId==0x4227 ? "chartCol" : subId==0x422e ? "ddeLinks" :
                       subId==0x422f ? "areaLinks" : subId==0x4230 ? "condFormats" :
                       subId==0x4231 ? "validation" : subId==0x4234 ? "detOp" : "dpCollection");
      f << what << ",";
      // sc_dbcolect.cxx ScDBCollection::Load and ...
      SDCParserInternal::ScMultiRecord scRecord(zone);
      ok=scRecord.open();
      if (!ok) break;

      long endDataPos=zone.getRecordLastPosition();
      if (subId==0x4239 && input->readLong(4)!=6) {
        f << "###version";
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument:find unknown version\n"));
        input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
        scRecord.close("SCCalcDocument");
        break;
      }
      uint16_t count;
      *input >> count;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      bool isOk=true;
      librevenge::RVNGString string;
      for (int i=0; i<int(count); ++i)  {
        pos=input->tell();
        f.str("");
        f << "SCCalcDocument:" << what << ",";
        if (!scRecord.openContent("SCCalcDocument")) {
          f << "###";
          isOk=false;
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument:can not find some content\n"));
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          break;
        }
        bool parsed=false, addDebugFile=false;
        long endData=scRecord.getContentLastPosition();
        switch (subId) {
        case 0x4225:
          parsed=readSCDBData(zone, version, endData);
          break;
        case 0x4226:
          parsed=readSCDBPivot(zone, version, endData);
          break;
        case 0x4227: {
          parsed=addDebugFile=true;
          uint16_t nTab, nCol1, nRow1, nCol2, nRow2;
          *input >> nTab >> nCol1 >> nRow1 >> nCol2 >> nRow2;
          f << "dim=" << nCol1 << "x" << nRow1 << "<->" << nCol2 << "x" << nRow2 << "[" << nTab << "],";
          if (!zone.readString(string) || input->tell()>endData) {
            STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
            f << "###string";
          }
          else {
            if (!string.empty()) f << string.cstr() << ",";
            bool bColHeaders, bRowHeaders;
            *input >> bColHeaders >> bRowHeaders;
            if (bColHeaders) f << "col[headers],";
            if (bRowHeaders) f << "row[headers],";
          }
          break;
        }
        case 0x422e: {
          parsed=addDebugFile=true;
          for (int j=0; j<3; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
              f << "###string";
              parsed=false;
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "appl" : j==1 ? "topic" : "item") << "=" << string.cstr() << ",";
          }
          if (!parsed)
            break;
          bool hasValue;
          *input>>hasValue;
          if (hasValue && !readSCMatrix(zone, version, endData)) {
            STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a matrix\n"));
            f << "###matrix";
            parsed=false;
            break;
          }
          if (input->tell()>endData) f << "mode=" << input->readULong(1) << ",";
          break;
        }
        case 0x422f: { // checkme
          parsed=addDebugFile=true;
          for (int j=0; j<3; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
              f << "###string";
              parsed=false;
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "file" : j==1 ? "filter" : "source") << "=" << string.cstr() << ",";
          }
          if (!parsed)
            break;
          f << "range=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
          if (input->tell()<endData) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
              f << "###string";
              parsed=false;
              break;
            }
            else if (!string.empty())
              f << "options=" << string.cstr() << ",";
          }
          break;
        }
        case 0x4230: {
          addDebugFile=parsed=true;
          // sc_conditio.cxx ScConditionalFormat::ScConditionalFormat
          uint32_t key;
          uint16_t entryCount;
          *input >> key >> entryCount;
          f << "key=" << key << ",";
          if (!entryCount) break;
          scRecord.closeContent("SCCalcDocument");
          f << "entries=[";
          for (int e=0; e<int(entryCount); ++e) {
            f << "[";
            if (!scRecord.openContent("SCCalcDocument")) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument:can not find some content\n"));
              f << "###";
              isOk=parsed=false;
              break;
            }
            if (!zone.readString(string)) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument:can not read a string\n"));
              isOk=parsed=false;
              f << "###string,";
              break;
            }
            f << "],";
            scRecord.closeContent("SCCalcDocument");
          }
          f << "],";
          break;
        }
        case 0x4231: {
          addDebugFile=parsed=true;
          // sc_validat.cxx
          uint32_t key;
          uint16_t mode;
          bool showInput;
          *input >> key >> mode >> showInput;
          f << "key=" << key << ",";
          f << "mode=" << mode << ",";
          if (showInput) f << "show[input],";
          for (int j=0; j<2; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
              parsed=false;
              f << "###string";
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "title" : "message") << "=" << string.cstr() << ",";
          }
          if (!parsed) break;
          bool showError;
          *input >> showError;
          if (showError) f << "show[error],";
          for (int j=0; j<2; ++j) {
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
              parsed=false;
              f << "###string";
              break;
            }
            if (string.empty()) continue;
            f << (j==0 ? "error[title]" : "error[message]") << "=" << string.cstr() << ",";
          }
          if (!parsed) break;
          f << "style[error]=" << input->readULong(2) << ",";
          break;
        }
        case 0x4234: {
          addDebugFile=parsed=true;
          // sc_detdata.cxx
          f << "range=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
          f << "op=" << input->readULong(2) << ",";
          break;
        }
        case 0x4239: {
          addDebugFile=parsed=true;
          // sc_dpobject.cxx ScDPObject::LoadNew
          uint8_t dType;
          *input >> dType;
          switch (dType) {
          case 0:
            f << "range=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
            parsed=readSCQueryParam(zone, version, endData);
            break;
          case 1:
            for (int j=0; j<2; ++j) {
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (string.empty()) continue;
              f << (j==0 ? "name" : "object") << "=" << string.cstr() << ",";
            }
            if (!parsed) break;
            f << "type=" << input->readULong(2) << ",";
            if (input->readULong(1)) f << "native,";
            break;
          case 2:
            for (int j=0; j<5; ++j) {
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (string.empty()) continue;
              static char const*(wh[])= {"serviceName","source","name","user","pass"};
              f << wh[j] << "=" << string.cstr() << ",";
            }
            break;
          default:
            STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: unexpected sub type\n"));
            f << "###subType=" << int(dType) << ",";
            parsed=false;
            break;
          }
          if (!parsed) break;
          f << "out[range]=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
          // ScDPSaveData::Load
          uint32_t nDim;
          *input >> nDim;
          for (uint32_t j=0; j<nDim; ++j) {
            f << "dim" << j << "=[";
            if (!zone.readString(string) || input->tell()>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
              parsed=false;
              f << "###string";
              break;
            }
            if (!string.empty()) f << string.cstr() << ",";
            bool isDataLayout, dupFlag, subTotalDef;
            uint16_t orientation, function, showEmptyMode, subTotalCount, extra;
            int32_t hierarchy;
            *input >> isDataLayout >> dupFlag >> orientation >> function >> hierarchy
                   >> showEmptyMode >> subTotalDef >> subTotalCount;
            if (isDataLayout) f << "isDataLayout,";
            if (dupFlag) f << "dupFlag,";
            if (orientation) f << "orientation=" << orientation << ",";
            if (function) f << "function=" << function << ",";
            if (hierarchy) f << "hierarchy=" << hierarchy << ",";
            if (showEmptyMode) f << "showEmptyMode=" << showEmptyMode << ",";
            if (subTotalDef) f << "subTotalDef,";
            if (input->tell()+2*long(subTotalCount)>endData) {
              STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read function list\n"));
              parsed=false;
              f << "###totalFuncs";
              break;
            }
            f << "funcs=[";
            for (int k=0; k<int(subTotalCount); ++k) f << input->readULong(2) << ",";
            f << "],";
            *input >> extra;
            if (extra) input->seek(extra, librevenge::RVNG_SEEK_CUR);
            uint32_t nMember;
            *input >> nMember;
            for (uint32_t k=0; k<nMember; ++k) {
              f << "member" << k << "[";
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (!string.empty()) f << string.cstr() << ",";
              uint16_t visibleMode, showDetailMode;
              *input >> visibleMode >> showDetailMode >> extra;
              if (visibleMode) f << "visibleMode=" << visibleMode << ",";
              if (showDetailMode) f << "showDetailMode=" << showDetailMode << ",";
              if (extra) input->seek(extra, librevenge::RVNG_SEEK_CUR);
              f << "],";
            }
            if (!parsed) break;
            f << "],";
          }
          if (!parsed) break;
          uint16_t colGrandMode, rowGrandMode, ignoreEmptyMode, repeatEmptyMode, extra;
          *input >> colGrandMode >> rowGrandMode >> ignoreEmptyMode >> repeatEmptyMode >> extra;
          f << "grandMode=" << colGrandMode << "x" << rowGrandMode << ",";
          if (ignoreEmptyMode) f << "ignoreEmptyMode=" << ignoreEmptyMode << ",";
          if (repeatEmptyMode) f << "repeatEmptyMode=" << repeatEmptyMode << ",";
          if (extra) input->seek(extra, librevenge::RVNG_SEEK_CUR);
          //
          if (input->tell()<endData) {
            for (int j=0; j<2; ++j) {
              if (!zone.readString(string) || input->tell()>endData) {
                STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
                parsed=false;
                f << "###string";
                break;
              }
              if (string.empty()) continue;
              f << (j==0 ? "tableName" : "tableTab") << "=" << string.cstr() << ",";
            }
          }
          break;
        }
        default:
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: unexpected type\n"));
          f << "###type,";
          addDebugFile=parsed=true;
          input->seek(endData, librevenge::RVNG_SEEK_SET);
          break;
        }
        if (!parsed) {
          addDebugFile=true;
          f << "###";
          input->seek(endData, librevenge::RVNG_SEEK_SET);
        }
        if (addDebugFile) {
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
        }
        if (scRecord.isContentOpened()) scRecord.closeContent("SCCalcDocument");
        if (!isOk) break;
      }
      if (isOk && subId==0x4225 && input->tell()<endDataPos) {
        pos=input->tell();
        f.str("");
        f << "SCCalcDocument:dbCollect, nEntry=" << input->readULong(2) << ",";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
      }
      scRecord.close("SCCalcDocument");
      pos=input->tell();
      break;
    }
    default:
      break;
    }
    if (ok) {
      if (pos!=input->tell()) {
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
      }
      continue;
    }
    input->seek(pos+2, librevenge::RVNG_SEEK_SET);
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not open the zone record\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endPos=zone.getRecordLastPosition();
    uint16_t vers;
    librevenge::RVNGString string;
    switch (subId) {
    case 0x4221: {
      f << "docFlags,";
      *input>>vers;
      version=(int) vers;
      f << "vers=" << vers << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "pageStyle=" << string.cstr() << ",";
      f << "protected=" << input->readULong(1) << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "passwd=" << string.cstr() << ","; // the uncrypted table password, safe to ignore
      if (input->tell()<endPos) f << "language=" << input->readULong(2) << ",";
      if (input->tell()<endPos) f << "autoCalc=" << input->readULong(1) << ",";
      if (input->tell()<endPos) f << "visibleTab=" << input->readULong(2) << ",";
      if (input->tell()<endPos) {
        *input>>vers;
        version=(int) vers;
        f << "vers=" << vers << ",";
      }
      if (input->tell()<endPos) {
        uint16_t nMaxRow;
        *input>>nMaxRow;
        maxRow=(int) nMaxRow;
        if (nMaxRow!=8191) f << "maxRow=" << nMaxRow << ",";
      }
      break;
    }
    case 0x4223: {
      // sc_documen9.cxx ScDocument::LoadDrawLayer, sc_drwlayer.cxx ScDrawLayer::Load
      f << "drawing,";
      while (input->tell()<endPos) {
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        pos=input->tell();
        *input >> nId;
        f.str("");
        f << "SCCalcDocument[drawing-" << std::hex << nId << std::dec << "]:";
        if (!zone.openSCRecord()) {
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not open a record\n"));
          f << "###record";
          break;
        }
        switch (nId) {
        case 0x4260: {
          f << "pool,";
          shared_ptr<StarItemPool> pool=document.getNewItemPool(StarItemPool::T_XOutdevPool);
          pool->addSecondaryPool(document.getNewItemPool(StarItemPool::T_EditEnginePool));
          if (!pool->read(zone))
            input->seek(pos, librevenge::RVNG_SEEK_SET);
          break;
        }
        case 0x4261:
          f << "sdrModel,";
          if (!readSdrModel(zone,document))
            input->seek(pos, librevenge::RVNG_SEEK_SET);
          break;
        default:
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: find unexpected type\n"));
          f << "###unknown";
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
          break;
        }
        zone.closeSCRecord("SCCalcDocument");
      }
      break;
    }
    case 0x4228: {
      f << "numFormats,";
      SWFormatManager formatManager;
      if (!formatManager.readNumberFormatter(zone))
        f << "###";
      break;
    }
    case 0x4229: {
      f << "docOptions,vers=" << version << ",";
      // sc_docoptio.cxx void ScDocOptions::Load
      bool bIsIgnoreCase, bIsIter=false;
      uint16_t nIterCount, nPrecStandardFormat, nDay, nMonth, nYear;
      double fIterEps;
      *input >> bIsIgnoreCase;
      if (version>=2) {
        uint8_t val;
        *input >> val;
        if (val!=0 && val!=1)
          input->seek(-1, librevenge::RVNG_SEEK_CUR);
        else
          bIsIter=(val!=0);
      }
      *input >> nIterCount;
      *input >> fIterEps >> nPrecStandardFormat >> nDay >> nMonth >> nYear;
      if (bIsIgnoreCase) f << "ignore[case],";
      if (bIsIter) f << "isIter,";
      if (nIterCount) f << "iterCount=" << nIterCount << ",";
      if (nPrecStandardFormat) f << "precStandartFormat=" << nPrecStandardFormat << ",";
      if (fIterEps<0||fIterEps>0) f << "iter[eps]=" << fIterEps << ",";
      f << "date=" << nMonth << "/" << nDay << "/" << nYear << ",";
      if (input->tell()+1==endPos)
        f << "unkn=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "tabs[distance]=" << input->readULong(2) << ",";
      if (input->tell()<endPos)
        f << "calc[asShown]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "match[wholeCell]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "autoSpell=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "lookUpColRowNames=" << input->readULong(1) << ",";
      if (input->tell()<endPos) {
        uint16_t nYear2000;
        *input >> nYear2000;
        if (input->tell()<endPos)
          *input >> nYear2000;
        else
          nYear2000 = uint16_t(nYear2000+1901);
        f << "year[2000]=" << nYear2000 << ",";
      }
      break;
    }
    case 0x422a: {
      f << "viewOptions,";
      // sc_viewopti.cxx operator>>(... ScViewOptions)
      for (int i=0; i<=9; ++i) {
        bool val;
        *input >> val;
        if (val) f << "opt" << i << ",";
      }
      for (int i=0; i<3; ++i) {
        uint8_t type;
        *input >> type;
        if (type) f << "type" << i << "=" << int(type) << ",";
      }
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a color\n"));
        f << "###color";
        break;
      }
      f << "col=" << col << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "name=" << string.cstr() << ",";
      if (input->tell()<endPos)
        f << "opt[helplines]=" << input->readULong(1) << ",";
      if (input->tell()<endPos) {
        // ScGridOptions operator >>
        uint32_t drawX, drawY, divisionX, divisionY, snapX, snapY;
        *input >> drawX >> drawY >> divisionX >> divisionY >> snapX >> snapY;
        f << "grid[draw]=" << drawX << "x" << drawY << ",";
        f << "grid[division]=" << divisionX << "x" << divisionY << ",";
        f << "grid[snap]=" << snapX << "x" << snapY << ",";
        bool useSnap, synchronize, visibleGrid, equalGrid;
        *input >> useSnap >> synchronize >> visibleGrid >> equalGrid;
        if (useSnap) f << "grid[useSnap],";
        if (synchronize) f << "grid[synchronize],";
        if (visibleGrid) f << "grid[visible],";
        if (equalGrid) f << "grid[equal],";
      }
      if (input->tell()<endPos)
        f << "hideAutoSpell=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[anchor]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[pageBreak]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[solidHandles]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[clipMarks]=" << input->readULong(1) << ",";
      if (input->tell()<endPos)
        f << "opt[bigHandles]=" << input->readULong(1) << ",";
      break;
    }
    case 0x422b: {
      f << "printer,";
      StarFileManager fileManager;
      if (!fileManager.readJobSetUp(zone)) break;
      break;
    }
    case 0x422c:
      f << "charset,";
      input->seek(1, librevenge::RVNG_SEEK_CUR); // GUI, dummy
      f << "set=" << input->readULong(1) << ",";
      break;
    case 0x4232:
    case 0x4233: {
      f << (subId==4232 ? "colNameRange" : "rowNameRange") << ",";
      // sc_rangelst.cxx ScRangePairList::Load(
      uint32_t n;
      *input >> n;
      if (!n) break;
      f << "ranges=[";
      if (version<0x12) {
        if (input->tell()+8*long(n) > endPos) {
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read some ranges\n"));
          f << "###ranges";
          break;
        }
        for (uint32_t i=0; i<n; ++i)
          f << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
      }
      else {
        if (input->tell()+16*long(n) > endPos) {
          STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read some ranges\n"));
          f << "###ranges";
          break;
        }
        for (uint32_t i=0; i<n; ++i)
          f << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ":"
            << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
      }
      f << "],";
      break;
    }
    case 0x4235: {
      f << "consolidateParam,";
      uint8_t funct;
      uint16_t nCol, nRow, nTab, nCount;
      bool byCol, byRow, byReference;
      *input >> nCol >> nRow >> nTab >> byCol >> byRow >> byReference >> funct >> nCount;
      f << "cell=" << nCol << "x" << nRow << "x" << nTab << ",";
      if (byCol) f << "byCol,";
      if (byRow) f << "byRow,";
      if (byReference) f << "byReference,";
      f << "funct=" << funct << ",";
      if (!nCount) break;
      if (input->tell()+10*long(nCount) > endPos) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read some area\n"));
        f << "###area";
        break;
      }
      for (int i=0; i<int(nCount); ++i) {
        uint16_t nCol2, nRow2;
        *input >> nTab >> nCol >> nRow >> nCol2 >> nRow2;
        f << nCol << "x" << nRow << "<->" << nCol2 << "x" << nRow2 << "[" << nTab << "],";
      }
      break;
    }
    case 0x4236: {
      f << "changeTrack,";
      uint16_t loadVers;
      *input >> loadVers;
      f << "load[version]=" << loadVers << ",";
      if (loadVers&0xff00) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: unknown version\n"));
        f << "###version";
        break;
      }
      // sc_chgtrack.cxx ScChangeTrack::Load
      if (!readSCChangeTrack(zone,int(loadVers),endPos))
        f << "###";
      break;
    }
    case 0x4237: {
      f << "changeViewSetting,";
      // sc_chgviset.cxx ScChangeViewSettings::Load
      bool bShowIt, bIsDate, bIsAuthor, bEveryoneButMe, bIsRange;
      uint8_t dateMode;
      uint32_t date1, time1, date2, time2;
      *input >> bShowIt >> bIsDate >> dateMode >> date1 >> time1 >> date2 >> time2 >> bIsAuthor >> bEveryoneButMe;
      if (bShowIt) f << "show,";
      if (bIsDate) f << "isDate,";
      f << "mode[date]=" << int(dateMode) << ",";
      f << "firstDate=" << date1 << ",";
      f << "firstTime=" << time1 << ",";
      f << "lastDate=" << date2 << ",";
      f << "lastTime=" << time2 << ",";
      if (bIsAuthor) f << "isAuthor,";
      if (bEveryoneButMe) f << "everyoneButMe,";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not read a string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << "author=" << string.cstr() << ",";
      *input >> bIsRange;
      if (bIsRange) f << "isRange,";
      if (!zone.openSCRecord()) {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: can not open the range list record\n"));
        f << "###";
        break;
      }
      uint32_t nRange;
      *input >> nRange;
      if (input->tell()+8*long(nRange)<=zone.getRecordLastPosition()) {
        f << "ranges=[";
        for (uint32_t j=0; j<nRange; ++j)
          f << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
        f << "],";
      }
      else {
        STOFF_DEBUG_MSG(("SDCParser::readCalcDocument: bad num rangen"));
        f << "###nRange=" << nRange << ",";
      }
      zone.closeSCRecord("SCCalcDocument");
      if (input->tell()<lastPos) {
        bool bShowAccepted, bShowRejected;
        *input >> bShowAccepted >> bShowRejected;
        if (bShowAccepted) f << "show[accepted],";
        if (bShowRejected) f << "show[rejected],";
      }
      if (input->tell()<lastPos) {
        bool isComment;
        *input >> isComment;
        if (isComment) f << "isComment,";
      }
      break;
    }
    case 0x4238:
      f << "linkUp[mode]=" << input->readULong(1) << ",";
      break;
    default:
      f << "##";
      input->seek(endPos, librevenge::RVNG_SEEK_SET);
    }
    if (pos!=endPos) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    zone.closeSCRecord("SCCalcDocument");
  }
  zone.closeSCRecord("SCCalcDocument");
  return true;
}
catch (...)
{
  return false;
}

bool SDCParser::readChartDocument(STOFFInputStreamPtr input, std::string const &name, StarDocument &document)
try
{
  StarZone zone(input, name, "SWChartDocument", document.getPassword()); // checkme
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCChartDocument):";
  // sch_docshell.cxx: SchChartDocShell::Load
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not find header zone\n"));
    f << "###";
    ascFile.addPos(0);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  uint32_t n;
  uint16_t nVersion;
  *input >> nVersion; // schiocmp.cxx: SchIOCompat::SchIOCompat
  f << "vers=" << nVersion << ",";
  // sch_chartdoc.cxx: operator>> ChartModel
  *input >> n;
  if (n>1) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: bad version\n"));
    f << "###n=" << n << ",";
  }
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  long pos=input->tell(), lastPos= zone.getRecordLastPosition();
  shared_ptr<StarItemPool> pool=document.getNewItemPool(StarItemPool::T_VCControlPool);
  if (!pool || !pool->read(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);

  pos=input->tell();
  StarFileManager fileManager;
  if (pos!=lastPos && !fileManager.readJobSetUp(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);

  zone.closeRecord("SCChartDocument");

  if (input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find end after first zone\n"));
    return true;
  }

  pos=input->tell();
  if (!readSdrModel(zone, document)) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not find the SdrModel\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SCChartDocument:###SdrModel");
    return true;
  }

  pos=input->tell();
  if (input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find no attribute\n"));
    return true;
  }
  f.str("");
  f << "SCChartDocument[attributes]:";
  if (!zone.openSCHHeader()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not open SCHAttributes\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  f << "vers=" << zone.getHeaderVersion() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  // chtmode2.cxx ChartModel::LoadAttributes
  if (!readSCHAttributes(zone, document)) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not open the attributes zone\n"));
  }
  zone.closeSCHHeader("SCChartDocumentAttr");

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SCChartDocument:##extra");
  }

  return true;
}
catch (...)
{
  return false;
}

bool SDCParser::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name, StarDocument &document)
{
  StarZone zone(input, name, "SfxStyleSheets", document.getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  shared_ptr<StarItemPool> mainPool;

  if (document.getDocumentKind()==STOFFDocument::STOFF_K_SPREADSHEET) {
    // sc_docsh.cxx ScDocShell::LoadCalc and sc_document.cxx: ScDocument::LoadPool
    long pos=0;
    libstoff::DebugStream f;
    uint16_t nId;
    *input >> nId;
    f << "Entries(SfxStyleSheets):id=" << std::hex << nId << std::dec << ",calc,";
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not open main zone\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    long lastPos=zone.getRecordLastPosition();
    while (input->tell()+6 < lastPos) {
      pos=input->tell();
      f.str("");
      *input >> nId;
      f << "SfxStyleSheets-1:id=" << std::hex << nId << std::dec << ",";
      if (!zone.openSCRecord()) {
        STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not open second zone\n"));
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        break;
      }
      switch (nId) {
      case 0x4211:
      case 0x4214: {
        f << (nId==0x411 ? "pool" : "pool[edit]") << ",";
        shared_ptr<StarItemPool> pool=document.getNewItemPool(nId==0x4211 ? StarItemPool::T_SpreadsheetPool : StarItemPool::T_EditEnginePool);
        if (pool && pool->read(zone)) {
          if (nId==0x4214 || !mainPool) mainPool=pool;
        }
        else {
          STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not readPoolZone\n"));
          f << "###";
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
          break;
        }
        break;
      }
      case 0x4212:
        f << "style[pool],";
        if (!StarItemPool::readStyle(zone, mainPool, document)) {
          STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not readStylePool\n"));
          f << "###";
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
          break;
        }
        break;
      case 0x422c: {
        uint8_t cSet, cGUI;
        *input >> cGUI >> cSet;
        f << "charset=" << int(cSet) << ",";
        if (cGUI) f << "gui=" << int(cGUI) << ",";
        break;
      }
      default:
        STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: find unexpected tag\n"));
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        f << "###";
        break;
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCRecord("SfxStyleSheets");
    }
    zone.closeSCRecord("SfxStyleSheets");

    if (!input->isEnd()) {
      STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: find extra data\n"));
      ascFile.addPos(input->tell());
      ascFile.addNote("SfxStyleSheets:###extra");
    }
    return true;
  }

  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  shared_ptr<StarItemPool> pool;
  if (document.getDocumentKind()==STOFFDocument::STOFF_K_DRAW) {
    pool=document.getNewItemPool(StarItemPool::T_XOutdevPool);
    pool->addSecondaryPool(document.getNewItemPool(StarItemPool::T_EditEnginePool));
  }
  else if (document.getDocumentKind()==STOFFDocument::STOFF_K_CHART) {
    pool=document.getNewItemPool(StarItemPool::T_XOutdevPool);
    pool->addSecondaryPool(document.getNewItemPool(StarItemPool::T_EditEnginePool));
    pool->addSecondaryPool(document.getNewItemPool(StarItemPool::T_ChartPool));
  }
  else if (document.getDocumentKind()==STOFFDocument::STOFF_K_TEXT)
    pool=document.getNewItemPool(StarItemPool::T_WriterPool);
  mainPool=pool;
  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=document.getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: create extra pool for %d of type %d\n",
                         (int) document.getDocumentKind(), (int) pool->getType()));
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
  if (!StarItemPool::readStyle(zone, mainPool, document))
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: find extra data\n"));
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
bool SDCParser::readSCTable(StarZone &zone, SDCParserInternal::Table &table, StarDocument &doc)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // sc_table2.cxx ScTable::Load
  if (!zone.openSCRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(SCTable)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    uint16_t id;
    *input>>id;
    f.str("");
    f << "SCTable[" << std::hex << id << std::dec << "]:";
    if (id==0x4240) {
      SDCParserInternal::ScMultiRecord scRecord(zone);
      f << "columns,";
      if (!scRecord.open()) {
        input->seek(pos,librevenge::RVNG_SEEK_SET);
        STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not find the column header \n"));
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        break;
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      int nCol=0;
      long endDataPos=zone.getRecordLastPosition();
      while (input->tell()<endDataPos) {
        if (table.getLoadingVersion()>=6) {
          pos=input->tell();
          nCol=(int) input->readULong(1);
          f.str("");
          f << "SCTable:C" << nCol << ",";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
        }
        else if (nCol>table.getMaxCols())
          break;
        pos=input->tell();
        if (!scRecord.openContent("SCTable")) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not open a column \n"));
          ascFile.addPos(pos);
          ascFile.addNote("SCTable-C###");
          break;
        }
        if (!readSCColumn(zone,table,doc, nCol, scRecord.getContentLastPosition())) {
          ascFile.addPos(pos);
          ascFile.addNote("SCTable-C###");
          input->seek(scRecord.getContentLastPosition(), librevenge::RVNG_SEEK_SET);
        }
        scRecord.closeContent("SCTable");
        ++nCol;
      }
      scRecord.close("SCTable");
      continue;
    }
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not open the zone record\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endDataPos=zone.getRecordLastPosition();
    switch (id) {
    case 0x4241: {
      f << "dim,";
      f << "col[width]=[";
      uint16_t rep;
      bool ok=true;
      for (int i=0; i<=table.getMaxCols();) {
        uint16_t val;
        *input>>rep >> val;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << val << "x" << rep << ",";
        else if (rep==1)
          f << val << ",";
        i+=int(rep);
      }
      f << "],col[flag]=[";
      for (int i=0; i<=table.getMaxCols();) {
        uint8_t flags;
        *input>>rep >> flags;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << int(flags) << "x" << (rep+1) << ",";
        else if (rep)
          f << int(flags) << ",";
        i+=int(rep);
      }
      f << "],row[height]=[";
      for (int i=0; i<=table.getMaxRows();) {
        uint16_t val;
        *input>>rep >> val;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << val << "x" << (rep+1) << ",";
        else if (rep==1)
          f << val << ",";
        i+=int(rep);
      }
      f << "],row[flag]=[";
      for (int i=0; i<=table.getMaxRows();) {
        uint8_t flags;
        *input>>rep >> flags;
        if (input->tell()>endDataPos) {
          ok=false;
          break;
        }
        if (rep>1)
          f << int(flags) << "x" << (rep+1) << ",";
        else if (rep==1)
          f << int(flags) << ",";
        i+=int(rep);
      }
      f << "],";
      if (!ok) {
        STOFF_DEBUG_MSG(("SDCParser::readSCTable: somethings went bad\n"));
        f << "###bound";
        break;
      }
      break;
    }
    case 0x4242: {
      f << "tabOptions,";
      bool ok=true;
      bool bVal;
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not read a string\n"));
          f << "###string" << i;
          ok=false;
          break;
        }
        if (!string.empty()) {
          static char const *(wh[])= {"name", "comment", "pass"};
          f << wh[i] << "=" << string.cstr() << ",";
        }
        if (i==2) break;
        *input>>bVal;
        if (bVal) f << (i==0 ? "scenario," : "protected") << ",";
      }
      if (!ok) break;
      *input>> bVal;
      if (bVal) {
        f << "outlineTable,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        for (int i=0; i<2; ++i) {
          pos=input->tell();
          if (!readSCOutlineArray(zone)) {
            STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not open the zone record\n"));
            ok=false;
            input->seek(pos,librevenge::RVNG_SEEK_SET);
            break;
          }
        }
        f.str("");
        f << "SCTable[tabOptionsB]:";
        pos=input->tell();
        if (!ok) {
          f << "###outline,";
          break;
        }
      }
      if (input->tell()<endDataPos) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not read a string\n"));
          f << "###pageStyle";
          break;
        }
        if (!string.empty())
          f << "pageStyle=" << string.cstr() << ",";
      }
      if (input->tell()<endDataPos) {
        *input >> bVal;
        if (bVal) f << "oneRange=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
        for (int i=0; i<2; ++i)
          f << "range" << i << "=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
      }
      if (input->tell()<endDataPos)
        f << "isVisible=" << input->readULong(1);
      if (input->tell()<endDataPos) {
        uint16_t nCount;
        *input>>nCount;
        if (input->tell()+8*long(nCount)>endDataPos) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: bad nCount\n"));
          f << "###nCount=" << nCount << ",";
          break;
        }
        f << "ranges=[";
        for (int i=0; i<int(nCount); ++i)
          f << "range" << i << "=" << std::hex << input->readULong(4) << "<->" << input->readULong(4) << std::dec << ",";
        f << "],";
      }
      if (input->tell()<endDataPos) {
        STOFFColor color;
        if (!input->readColor(color)) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not read a color\n"));
          f << "###color";
          break;
        }
        f << "scenario[color]=" << color << ",";
        f << "scenario[flags]=" << input->readULong(2) << ",";
        f << "scenario[active]=" << input->readULong(1) << ",";
      }
      break;
    }
    case 0x4243: {
      f << "tabLink,";
      f << "mode=" << input->readULong(1) << ",";
      bool ok=true;
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not read a string\n"));
          f << "###string" << i;
          ok=false;
          break;
        }
        if (string.empty()) continue;
        static char const *(wh[])= {"doc", "flt", "tab"};
        f << "link[" << wh[i] << "]=" << string.cstr() << ",";
      }
      if (!ok) break;
      if (input->tell()<endDataPos)
        f << "bRelUrl=" << input->readULong(1) << ",";
      if (input->tell()<endDataPos) {
        if (!zone.readString(string) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("SDCParser::readSCTable: can not read a string\n"));
          f << "###opt";
          break;
        }
        if (string.empty()) continue;
        f << "link[opt]=" << string.cstr() << ",";
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDCParser::readSCTable: find unexpected type\n"));
      f << "###";
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCRecord("SCTable");
  }
  zone.closeSCRecord("SCTable");
  return true;
}

bool SDCParser::readSCColumn(StarZone &zone, SDCParserInternal::Table &table, StarDocument &doc,
                             int column, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // sc_column2.cxx ScColumn::Load
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCColumn)-C" << column << "[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    uint16_t id;
    *input>>id;
    f.str("");
    f << "SCColumn[" << std::hex << id << std::dec << "]:";
    if (id==0x4250 && readSCData(zone,table,doc,column)) {
      f << "data,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      continue;
    }
    input->seek(pos+2, librevenge::RVNG_SEEK_SET);
    bool ok=zone.openSCRecord();
    if (!ok || zone.getRecordLastPosition()>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCColumn:can not open record\n"));
      f << "###";
      if (ok) zone.closeSCRecord("SCColumn");
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endDataPos=zone.getRecordLastPosition();
    switch (id) {
    case 0x4251: {
      f << "notes,";
      uint16_t nCount;
      *input >> nCount;
      f << "n=" << nCount << ",";
      for (int i=0; i<nCount; ++i) {
        f << "note" << i << "[pos=" << input->readULong(2) << ",";
        // sc_cell.cxx ScBaseCell::LoadNotes, ScPostIt operator>>
        for (int j=0; j<3; ++j) {
          if (!zone.readString(string)||input->tell()>endDataPos) {
            STOFF_DEBUG_MSG(("SDCParser::readSCColumn:can not read a string\n"));
            f << "###string" << j;
            ok=false;
            break;
          }
          if (string.empty()) continue;
          static char const *(wh[])= {"note","date","author"};
          f << wh[j] << "=" << string.cstr() << ",";
        }
        if (!ok) break;
        f << "],";
      }
      break;
    }
    case 0x4252: {
      f << "attrib,";
      // sc_attarray.cxx ScAttrArray::Load
      uint16_t nCount;
      *input >> nCount;
      f << "n=" << nCount << ",";
      shared_ptr<StarItemPool> pool=doc.getNewItemPool(StarItemPool::T_SpreadsheetPool); // FIXME
      f << "attrib=[";
      for (int i=0; i<nCount; ++i) {
        f << input->readULong(2) << ":";
        uint16_t nWhich=149; // ATTR_PATTERN
        if (!pool->loadSurrogate(zone, nWhich, f) || input->tell()>endDataPos) {
          STOFF_DEBUG_MSG(("SDCParser::readSCColumn:can not read a attrib\n"));
          f << "###attrib";
          break;
        }
      }
      f << "],";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDCParser::readSCColumn: find unexpected type\n"));
      f << "###";
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCRecord("SCColumn");
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  return true;
}

bool SDCParser::readSCData(StarZone &zone, SDCParserInternal::Table &table, StarDocument &doc, int column)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // sc_column2.cxx ScColumn::LoadData
  SDCParserInternal::ScMultiRecord scRecord(zone);
  if (!scRecord.open()) {
    input->seek(pos,librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCData)[C" << column << "-" << zone.getRecordLevel() << "]:" << scRecord;
  int count=(int) input->readULong(2);
  f << "count=" << count << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  int const version=table.getLoadingVersion();
  for (int i=0; i<count; ++i) {
    pos=input->tell();
    f.str("");
    f << "SCData-" << i << ":";
    if (input->tell()+4>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCData:can not read some data\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    int row=(int) input->readULong(2);
    f << "row=" << row << ",";
    uint8_t what;
    *input>>what;
    bool ok=true;
    switch (what) {
    case 1: { // value
      // sc_cell2.cxx
      f << "value,";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      double value; // checkme
      *input >> value;
      f << "val=" << value << ",";
      break;
    }
    case 2:
    case 6: {
      // sc_cell2.cxx
      f << (what==2 ? "string" : "symbol") << ",";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDCParser::readSCData: can not open some text \n"));
        f << "###text";
        ok=false;
        break;
      }
      f << "val=" << text.cstr() << ",";
      break;
    }
    case 3: {
      // sc_cell.cxx
      f << "formula,";
      if (!scRecord.openContent("SCData")) {
        STOFF_DEBUG_MSG(("SDCParser::readSCData: can not open a formula \n"));
        f << "###formula";
        ok=false;
        break;
      }
      long endDataPos=scRecord.getContentLastPosition();
      if (version>=8) {
        int cData=int(input->readULong(1));
        if ((cData&0x10) && (cData&0xf)>=4) {
          f << "format=" <<input->readULong(4)<<",";
          cData-=4;
        }
        if (cData&0xf) input->seek((cData&0xf), librevenge::RVNG_SEEK_CUR);
        uint8_t cFlags;
        *input >> cFlags;
        f << "formatType=" << input->readLong(2) << ",";
        if (cFlags&8) {
          double ergValue;
          *input >> ergValue;
          f << "ergValue=" << ergValue << ",";
        }
        if (cFlags&0x10) {
          librevenge::RVNGString text;
          if (!zone.readString(text)) {
            STOFF_DEBUG_MSG(("SDCParser::readSCData: can not open some text\n"));
            f << "###text";
            ascFile.addDelimiter(input->tell(),'|');
            input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
            scRecord.closeContent("SCData");
            break;
          }
          else if (!text.empty())
            f << "val=" << text.cstr() << ",";
        }
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        pos=input->tell();
        f.str("");
        f << "SCData[formula]:";

        if (!readSCFormula(zone, STOFFVec2i(row,column), version, endDataPos) || input->tell()>endDataPos) {
          f << "###";
          scRecord.closeContent("SCData");
          ascFile.addDelimiter(input->tell(),'|');
          input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
          break;
        }
        pos=input->tell();
        if ((cFlags&3)==1 && input->tell()<endDataPos)
          f << "cols=" << input->readULong(2) << ",rows=" << input->readULong(2) << ",";
      }
      else {
        if (version>=2) input->seek(2, librevenge::RVNG_SEEK_CUR);
        f << "matrix[flags]=" << input->readULong(1) << ",";
        uint16_t codeLen;
        *input>>codeLen;
        if (codeLen && (!readSCFormula3(zone, STOFFVec2i(row,column), version, endDataPos) || input->tell()>endDataPos))
          f << "###";
      }
      if (input->tell()!=endDataPos) {
        f << "##";
        ascFile.addDelimiter(input->tell(),'|');
        input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
      }
      scRecord.closeContent("SCData");
      break;
    }
    case 4: {
      // sc_cell2.cxx
      f << "note";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      break;
    }
    case 5: {
      // sc_cell2.cxx
      f << "edit";
      if (version>=7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      StarFileManager fileManager;
      if (!fileManager.readEditTextObject(zone, lastPos, doc) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("SDCParser::readSCData: can not open some edit text \n"));
        f << "###edit";
        ok=false;
        break;
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDCParser::readSCData: find unexpected type\n"));
      f << "###type=" << what;
      ok=false;
      break;
    }
    if (!ok || pos!=input->tell()) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    if (!ok) break;
  }
  scRecord.close("SCData");
  return true;
}

// sc_chgtrack.cxx ScChangeActionContent::ScChangeActionContent

bool SDCParser::readSCChangeTrack(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCChangeTrack)[" << zone.getRecordLevel() << "]:";
  // sc_chgtrack.cxx ScChangeTrack::Load

  if (!zone.openSCRecord()) {
    STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not find string collection\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###strings";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  bool bDup;
  uint16_t count, limit, delta;
  *input >> bDup >> count >> limit >> delta;
  if (bDup) f << "bDups,";
  if (limit) f << "limit=" << limit << ",";
  if (delta) f << "delta=" << delta << ",";
  long endData=zone.getRecordLastPosition();
  librevenge::RVNGString string;
  for (int16_t i=0; i<count; ++i) {
    if (!zone.readString(string) || input->tell() > endData) {
      STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not open some string\n"));
      f << "###string";
      ascFile.addDelimiter(input->tell(),'|');
      input->seek(endData, librevenge::RVNG_SEEK_SET);
      break;
    }
    else if (!string.empty())
      f << "string" << i << "=" << string.cstr() << ",";
  }
  zone.closeSCRecord("SCChangeTrack");

  uint32_t nCount, nActionMax, nLastAction, nGeneratedCount;
  *input >> nCount >> nActionMax >> nLastAction >> nGeneratedCount;
  if (nCount) f << "count=" << nCount << ",";
  if (nActionMax) f << "actionMax=" << nActionMax << ",";
  if (nLastAction) f << "lastAction=" << nLastAction << ",";

  for (int s=0; s<2; ++s) {
    if (s==1) {
      pos=input->tell();
      f.str("");
      f << "Entries(SCChangeTrack)[A]:";
    }
    SDCParserInternal::ScMultiRecord scRecord(zone);
    if (!scRecord.open()) {
      STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not find the action list\n"));
      ascFile.addDelimiter(input->tell(),'|');
      f << "###actions";
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (uint32_t i=0; i<(s==0 ? nGeneratedCount:nCount); ++i) {
      f.str("");
      f << "Entries(SCChangeTrack)[A" << i << "]:";
      if (!scRecord.openContent("SCChangeTrack")) {
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack:can not read an action\n"));
        break;
      }
      endData=scRecord.getContentLastPosition();

      uint8_t type;
      *input >> type;
      uint32_t col, row, tab, date, time, action, rejAction, state;
      *input >> col >> row >> tab >> date >> time >> action >> rejAction >> state;
      f << "cell=" << col << "x" << row << "[" << tab << "],";
      f << "date=" << date << ",";
      f << "time=" << time << ",";
      if (action) f << "action=" << action << ",";
      if (rejAction) f << "rejAction=" << rejAction << ",";
      if (state) f << "state=" << state << ",";
      if (!zone.readString(string) || input->tell() > endData) {
        STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not open some string\n"));
        f << "###string";
        ascFile.addDelimiter(input->tell(),'|');
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        scRecord.closeContent("SCChangeTrack");
        continue;
      }
      if (!string.empty())
        f << "comment" << i << "=" << string.cstr() << ",";
      if (s==0 && type!=8) {
        f << "###type";
        STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack:the type seems bad\n"));
      }
      switch (type) {
      case 1:
      case 2:
      case 3:
      case 9:
        f << "insert[" << (type==1 ? "cols" : type==2 ? "rows" : type==9 ? "tab" : "reject") << "],";
        break;
      case 4:
      case 5:
      case 6: {
        f << "delete[" << (type==4 ? "cols" : type==5 ? "rows" : "tab") << "],";
        uint32_t pCutOff;
        uint16_t cutOff, dx, dy;
        *input >> pCutOff >> cutOff >> dx >> dy;
        if (pCutOff) f << "pCutOff=" << pCutOff << ",";
        if (cutOff) f << "cutOff=" << cutOff << ",";
        f << "delta=" << dx << "x" << dy << ",";
        break;
      }
      case 7:
        f << "move,";
        *input >> col >> row >> tab;
        f << "fromCell=" << col << "x" << row << "[" << tab << "],";
        break;
      case 8: {
        bool ok=true;
        for (int j=0; j<2; ++j) {
          if (!zone.readString(string) || input->tell() > endData) {
            STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not open some string\n"));
            f << "###string";
            ok=false;
            break;
          }
          if (string.empty()) continue;
          f << (j==0 ? "oldValue" : "newValue") << "=" << string.cstr() << ",";
        }
        if (!ok) break;
        uint32_t oldContent, newContent;
        *input >> oldContent >> newContent;
        if (oldContent) f << "oldContent=" << oldContent << ",";
        if (newContent) f << "newContent=" << newContent << ",";
        SDCParserInternal::ScMultiRecord scRecord2(zone);
        if (!scRecord2.open()) {
          STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not find the cell action\n"));
          f << "###cells";
          break;
        }
        STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: reading cell action is not implemented\n"));
        f << "###cells,";
        // fixme: call readSCData with only one cell
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        scRecord2.close("SCChangeTrack");
        break;
      }
      default:
        STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack:unknown type\n"));
        f << "###type=" << (int) type << ",";
        break;
      }
      scRecord.closeContent("SCChangeTrack");
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    scRecord.close("SCChangeTrack");
  }

  pos=input->tell();
  f.str("");
  f << "Entries(SCChangeTrack)[L]:";
  SDCParserInternal::ScMultiRecord scRecord(zone);
  if (!scRecord.open()) {
    STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: can not find the action list\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###link";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  while (scRecord.openContent("SCChangeTrack")) {
    pos=input->tell();
    f.str("");
    f << "Entries(SCChangeTrack)[L]:###";
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: reading the action links is not implemented\n"));
      first=false;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(scRecord.getContentLastPosition(), librevenge::RVNG_SEEK_SET);
    scRecord.closeContent("SCChangeTrack");
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDCParser::readSCChangeTrack: find extra data\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;

}

bool SDCParser::readSCDBData(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCDBData)[" << zone.getRecordLevel() << "]:";
  // sc_dbcolect.cxx ScDBData::Load
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some text\n"));
    f << "###name";
    ascFile.addDelimiter(input->tell(),'|');
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  f << "name=" << string.cstr() << ",";
  uint16_t nTable, nStartCol, nStartRow, nEndCol, nEndRow;
  *input >> nTable >> nStartCol >> nStartRow >> nEndCol >> nEndRow;
  if (nTable) f << "table=" << nTable << ",";
  f << "dim=" << nStartCol << "x" << nStartRow << "<->" << nEndCol << "x" << nEndRow << ",";
  bool bByRow, bHasHeader, bSortCaseSens, bIncludePattern, bSortInplace;
  *input >> bByRow >> bHasHeader >> bSortCaseSens >> bIncludePattern >> bSortInplace;
  if (bByRow) f << "byRow,";
  if (bHasHeader) f << "hasHeader,";
  if (bSortCaseSens) f << "sortCaseSens,";
  if (bIncludePattern) f << "includePattern,";
  if (bSortInplace) f << "sortInPlace,";
  uint16_t nSortDestTab, nSortDestCol, nSortDestRow;
  *input >> nSortDestTab >> nSortDestCol >> nSortDestRow;
  f << "dest[sort]=" << nSortDestCol << "x" << nSortDestCol << "x" << nSortDestTab << ",";
  bool bQueryInplace, bQueryCaseSen, bQueryRegExp, bQueryDuplicate;
  *input >> bQueryInplace >> bQueryCaseSen >> bQueryRegExp >> bQueryDuplicate;
  if (bQueryInplace) f << "query[inPlace],";
  if (bQueryCaseSen) f << "query[caseSen],";
  if (bQueryRegExp) f << "query[regExp],";
  if (bQueryDuplicate) f << "query[duplicate],";
  uint16_t nQueryDestTab, nQueryDestCol, nQueryDestRow;
  *input >> nQueryDestTab >> nQueryDestCol >> nQueryDestRow;
  f << "dest[query]=" << nQueryDestCol << "x" << nQueryDestCol << "x" << nQueryDestTab << ",";
  bool bSubRemoveOnly, bSubReplace, bSubPagebreak, bSubCaseSens, bSubDoSort,
       bSubAscending, bSubIncludePattern, bSubUserDef;
  bool bDBImport, bDBNative;
  *input >> bSubRemoveOnly >> bSubReplace >> bSubPagebreak >> bSubCaseSens
         >> bSubDoSort >> bSubAscending >> bSubIncludePattern >> bSubUserDef
         >> bDBImport;
  if (bSubRemoveOnly) f << "subRemoveOnly,";
  if (bSubReplace) f << "subReplace,";
  if (bSubPagebreak) f << "subPagebreak,";
  if (bSubCaseSens) f << "subCaseSens,";
  if (bSubDoSort) f << "subDoSort,";
  if (bSubAscending) f << "subAscending,";
  if (bSubIncludePattern) f << "subIncludePattern,";
  if (bSubUserDef) f << "subUserDef,";
  if (bDBImport) f << "dbImport,";
  for (int i=0; i<2; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some text\n"));
      f << "###name";
      ascFile.addDelimiter(input->tell(),'|');
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (!string.empty())
      f << (i==0 ? "dbName" : "dbStatement") << "=" << string.cstr() << ",";
  }
  *input >> bDBNative;
  if (bDBNative) f << "dbNative,";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f.str("");
  f << "SCDBData:";
  if (input->tell()+3*4>lastPos) {
    STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read sort data\n"));
    f << "###name";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  for (int i=0; i<3; ++i) {
    bool doSort, doAscend;
    uint16_t sortField;
    *input>>doSort>>sortField >> doAscend;
    if (doSort) f << "sort" << i << "=" << sortField << "[" << int(doAscend) << "],";
  }
  for (int i=0; i<8; ++i) {
    bool doQuery, queryByString;
    uint16_t queryField;
    uint8_t queryOp, queryConnect;
    double val;
    *input >> doQuery >> queryField >> queryOp >> queryByString;
    if (!zone.readString(string)||input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some text\n"));
      f << "###name";
      ascFile.addDelimiter(input->tell(),'|');
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    *input>>val >> queryConnect;
    if (!doQuery) continue;
    f << "query" << i << "=[";
    f << string.cstr() << ",";
    f << "field=" << queryField << ",";
    f << "op=" << (int)queryOp << ",";
    if (queryByString) f << "byString,";
    if (!string.empty()) f << string.cstr() << ",";
    if (val<0 || val>0) f << "val=" << val << ",";
    f << "connect=" << (int)queryConnect << ",";
    f << "],";
  }
  for (int i=0; i<3; ++i) {
    bool doSubTotal;
    uint16_t field, count;
    *input >> doSubTotal >> field >> count;
    if (input->tell() + 3*long(count) > lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some subTotal\n"));
      f << "###subTotal";
      ascFile.addDelimiter(input->tell(),'|');
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (!doSubTotal && !count) continue;
    f << "subTotal" << i << "=[";
    f << "field=" << field  << ",[";
    for (int c=0; c<int(count); ++c)
      f << input->readULong(2) << ":" << input->readULong(1) << ",";
    f << "],";
  }
  if (input->tell()<lastPos)
    f << "index=" << input->readULong(2) << ",";
  if (input->tell()<lastPos)
    f << "dbSelect=" << input->readULong(1) << ",";
  if (input->tell()<lastPos)
    f << "dbSQL=" << input->readULong(1) << ",";
  if (input->tell()<lastPos) {
    f << "subUserIndex=" << input->readULong(2) << ",";
    f << "sortUserDef=" << input->readULong(1) << ",";
    f << "sortUserIndex=" << input->readULong(2) << ",";
  }
  if (input->tell()<lastPos) {
    f << "doSize=" << input->readULong(1) << ",";
    f << "keepFmt=" << input->readULong(1) << ",";
  }
  if (input->tell()<lastPos)
    f << "stripData=" << input->readULong(1) << ",";
  if (input->tell()<lastPos)
    f << "dbType=" << input->readULong(1) << ",";
  if (input->tell()<lastPos) {
    bool isAdvanced;
    *input >> isAdvanced;
    if (isAdvanced)
      f << "advSource=" << std::hex << input->readULong(4) << "x" << input->readULong(4) << std::dec << ",";
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDCParser::readSCDBData: find extra data\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool SDCParser::readSCDBPivot(StarZone &zone, int version, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCDBPivot)[" << zone.getRecordLevel() << "]:";
  // sc_pivot.cxx ScPivot::Load
  bool hasHeader;
  *input >> hasHeader;
  if (hasHeader) f << "hasHeader,";
  for (int i=0; i<2; ++i) {
    uint16_t col1, row1, col2, row2, table;
    *input >> col1 >> row1 >> col2 >> row2 >> table;
    f << (i==0 ? "src":"dest") << "="
      << col1 << "x" << row1 << "<->" << col2 << "x" << row2 << "[" << table << "],";
  }
  for (int i=0; i<3; ++i) {
    uint16_t nCount;
    *input >> nCount;
    if (input->tell()+6*int(nCount) >lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some fields\n"));
      f << "###fields";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }

    if (!nCount) continue;
    f << (i==0 ? "col" : i==1 ? "row" : "data") << "=[";
    for (int c=0; c<int(nCount); ++c) {
      if (version >= 7) {
        uint8_t unkn;
        *input>>unkn;
        if (unkn&0xf) input->seek((unkn&0xf), librevenge::RVNG_SEEK_CUR);
      }
      f << "[";
      f << "col=" << input->readLong(2) << ",";
      f << "mask=" << input->readULong(2) << ",";
      f << "count=" << input->readULong(2) << ",";
      f << "],";
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f << "SCDBPivot:";
  if (!readSCQueryParam(zone, version, lastPos)) {
    f << "###query";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  pos=input->tell();
  bool bIgnoreEmpty, bDectectCat;
  *input >> bIgnoreEmpty >> bDectectCat;
  if (bIgnoreEmpty) f << "ignore[empty],";
  if (bDectectCat) f << "detect[cat],";
  if (input->tell()<lastPos) {
    bool makeTotalCol, makeTotalRow;
    *input >> makeTotalCol >> makeTotalRow;
    if (makeTotalCol) f << "make[totalCol],";
    if (makeTotalRow) f << "make[totalRow],";
  }
  if (input->tell()<lastPos) {
    librevenge::RVNGString string;
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some text\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!string.empty())
        f << (i==0 ? "name": "tag") << "=" << string.cstr() << ",";
    }
    uint16_t count;
    *input >> count;
    for (int i=0; i<int(count); ++i) {
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some text\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!string.empty())
        f << "colName" << i << "=" << string.cstr() << ",";
    }
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDCParser::readSCDBPivot: find extra data\n"));
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool SDCParser::readSCFormula(StarZone &zone, STOFFVec2i const &cell, int version, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCFormula)[" << zone.getRecordLevel() << "]:";
  uint8_t fFlags;
  *input>>fFlags;
  if (fFlags&0xf) input->seek((fFlags&0xf), librevenge::RVNG_SEEK_CUR);
  f << "cMode=" << input->readULong(1) << ","; // if (version<0x201) old mode
  if (fFlags&0x10) f << "nRefs=" << input->readLong(2) << ",";
  if (fFlags&0x20) f << "nErrors=" << input->readULong(2) << ",";
  if (fFlags&0x40) { // token
    uint16_t nLen;
    *input>>nLen;
    f << "formula=[";
    for (int tok=0; tok<nLen; ++tok) {
      if (!readSCTokenInFormula(zone, cell, version, lastPos, f) || input->tell()>lastPos) {
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
    }
    f << "],";
  }
  if (fFlags&0x80) {
    uint16_t nRPN;
    *input >> nRPN;
    f << "rpn=[";
    for (int rpn=0; rpn<int(nRPN); ++rpn) {
      uint8_t b1;
      *input >> b1;
      if (b1==0xff) {
        if (!readSCTokenInFormula(zone, cell, version, lastPos, f)) {
          f << "###";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          return false;
        }
      }
      else if (b1&0x40)
        f << "[Index" << ((b1&0x3f) & (input->readULong(1)<<6)) << "]";
      else
        f << "[Index" << int(b1) << "]";
      if (input->tell()>lastPos) {
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool SDCParser::readSCMatrix(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCMatrix)[" << zone.getRecordLevel() << "]:";
  // sc_scmatrix.cxx ScMatrix::ScMatrix
  uint16_t nCol, nRow;
  *input >> nCol >> nRow;
  f << "dim=" << nCol << "x" << nRow << ",";
  int nCell=int(nCol)*int(nRow);
  bool ok=true;
  f << "values=[";
  for (int i=0; i<nCell; ++i) {
    uint8_t type;
    *input>>type;
    if ((i%nCol)==0) f << "[";
    switch (type) {
    case 0:
      f << "_,";
      break;
    case 1: {
      double val;
      *input>>val;
      f << val << ",";
      break;
    }
    case 2: {
      librevenge::RVNGString string;
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDCParser::readSCMatrix: can not read a string\n"));
        f << "###string";
        ok=false;
        break;
      }
      f << string.cstr() << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDCParser::readSCMatrix: find unexpected type\n"));
      f << "###type=" << int(type) << ",";
      ok=false;
      break;
    }
    if (!ok) break;
    if (input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCMatrix: the zone is too short\n"));
      f << "###short,";
      ok=false;
      break;
    }
    if ((i%nCol)==0) f << "],";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=lastPos;
}
bool SDCParser::readSCQueryParam(StarZone &zone, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  if (!zone.openSCRecord()) return false;
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCQueryParam)[" << zone.getRecordLevel() << "]:";
  // sc_global2.cxx ScQueryParam::Load
  uint16_t nCol1, nRow1, nCol2, nRow2, nDestTab, nDestCol, nDestRow;
  bool bHasHeader, bInplace, bCaseSens, bRegExp, bDuplicate, bByRow;
  *input >> nCol1 >> nRow1 >> nCol2 >> nRow2 >> nDestTab >> nDestCol >> nDestRow;
  f << "dim=" << nCol1 << "x" << nRow1 << "<->" << nCol2 << "x" << nRow2 << ",";
  f << "dest=" << nDestCol << "x" << nDestRow << "x" << nDestTab << ",";
  *input >> bHasHeader >> bInplace >> bCaseSens >> bRegExp >> bDuplicate >> bByRow;
  if (bHasHeader) f << "hasHeader,";
  if (bInplace) f << "inPlace,";
  if (bCaseSens) f << "caseSens,";
  if (bRegExp) f << "regExp,";
  if (bDuplicate) f << "duplicate,";
  if (bByRow) f << "byRow,";
  librevenge::RVNGString string;
  for (int i=0; i<8; ++i) {
    bool doQuery, queryByString;
    uint8_t op, connect;
    uint16_t nField;
    double val;
    *input >> doQuery >> queryByString >> op >> connect >> nField >> val;
    if (!zone.readString(string)||input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCDBData: can not read some text\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCRecord("SCQueryParam");
      return false;
    }
    if (!doQuery) continue;
    f << "query" << i << "=[";
    if (queryByString) f << "byString,";
    f << "op=" << int(op) << ",";
    f << "connect=" << int(connect) << ",";
    f << "field=" << nField << ",";
    f << "val=" << val << ",";
    if (!string.empty()) f << string.cstr() << ",";
    f << "],";
  }
  zone.closeSCRecord("SCQueryParam");
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool SDCParser::readSCFormula3(StarZone &zone, STOFFVec2i const &cell, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCFormula)[" << zone.getRecordLevel() << "]:";
  for (int tok=0; tok<512; ++tok) {
    bool endData;
    if (!readSCTokenInFormula3(zone, cell, endData, lastPos, f) || input->tell()>lastPos) {
      f << "###";
      break;
    }
    if (endData) break;
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool SDCParser::readSCTokenInFormula(StarZone &zone, STOFFVec2i const &/*cell*/, int /*vers*/, long lastPos, libstoff::DebugStream &f)
{
  STOFFInputStreamPtr input=zone.input();
  // sc_token.cxx ScRawToken::Load
  uint16_t nOp;
  uint8_t type;
  *input >> nOp >> type;
  bool ok=true;
  switch (type) {
  case 0: {
    bool val;
    *input>>val;
    f << "[" << val << "]";
    break;
  }
  case 1: {
    double val;
    *input>>val;
    f << "[" << val << "]";
    break;
  }
  case 2: // string
  case 8: // external
  default: { // ?
    if (type==8) f << "[cByte=" << input->readULong(1) << "]";
    uint8_t nBytes;
    *input >> nBytes;
    if (input->tell()+int(nBytes)>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCTokenInFormula: can not read text zone\n"));
      f << "###text";
      ok=false;
      break;
    }
    librevenge::RVNGString text;
    for (int i=0; i<int(nBytes); ++i) text.append((char) input->readULong(1));
    f << "[" << text.cstr() << "]";
    break;
  }
  case 3:
  case 4: {
    int16_t nCol, nRow, nTab;
    uint8_t nByte;
    f << "[";
    *input >> nCol >> nRow >> nTab >> nByte;
    f << nRow << "x" << nCol;
    if (nTab) f << "x" << nTab;
    if (nByte) f << ":" << int(nByte); // vers<10 diff
    if (type==4) {
      *input >> nCol >> nRow >> nTab >> nByte;
      f << "<->" << nRow << "x" << nCol;
      if (nTab) f << "x" << nTab;
      if (nByte) f << ":" << int(nByte); // vers<10 diff
    }
    f << "]";
    break;
  }
  case 6:
    f << "[index" << input->readULong(2) << "]";
    break;
  case 7: {
    uint8_t nByte;
    *input >> nByte;
    if (input->tell()+2*int(nByte)>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCTokenInFormula: can not read the jump\n"));
      f << "###jump";
      ok=false;
      break;
    }
    f << "[J" << (int) nByte << ",";
    for (int i=0; i<(int) nByte; ++i) f << input->readLong(2) << ",";
    f << "]";
    break;
  }
  case 0x70:
    f << "[missing]";
    break;
  case 0x71:
    f << "[error]";
    break;
  }
  return ok && input->tell()<=lastPos;
}

bool SDCParser::readSCTokenInFormula3(StarZone &zone, STOFFVec2i const &/*cell*/, bool &endData, long lastPos, libstoff::DebugStream &f)
{
  endData=false;
  STOFFInputStreamPtr input=zone.input();
  // sc_token.cxx ScRawToken::Load30
  uint16_t nOp;
  *input >> nOp;
  bool ok=true;
  switch (nOp) {
  case 0: {
    uint8_t type;
    *input >> type;
    switch (type) {
    case 0: {
      bool val;
      *input>>val;
      f << val;
      break;
    }
    case 1: {
      double val;
      *input>>val;
      f << val << ",";
      break;
    }
    case 2: {
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDCParser::readSCTokenInFormula3: can not read text zone\n"));
        f << "###text";
        ok=false;
        break;
      }
      f << "\"" << text.cstr() << "\"";
      break;
    }
    case 3: {
      int16_t nCol, nRow, nTab;
      uint8_t relCol, relRow, relTab, oldFlag;
      *input >> nCol >> nRow >> nTab >> relCol >> relRow >> relTab >> oldFlag;
      f << nRow << "x" << nCol;
      if (nTab) f << "x" << nTab;
      f << "[" << int(relCol) << "," << int(relRow) << "," << int(relTab) << "," << int(oldFlag) << "]";
      break;
    }
    case 4: {
      int16_t nCol1, nRow1, nTab1, nCol2, nRow2, nTab2;
      uint8_t relCol1, relRow1, relTab1, oldFlag1, relCol2, relRow2, relTab2, oldFlag2;
      *input >> nCol1 >> nRow1 >> nTab1 >> nCol2 >> nRow2 >> nTab2
             >> relCol1 >> relRow1 >> relTab1 >> relCol2 >> relRow2 >> relTab2
             >> oldFlag1 >> oldFlag2;
      f << nRow1 << "x" << nCol1;
      if (nTab1) f << "x" << nTab1;
      f << "[" << int(relCol1) << "," << int(relRow1) << "," << int(relTab1) << "," << int(oldFlag1) << "]";
      f << ":" << nRow2 << "x" << nCol2;
      if (nTab2) f << "x" << nTab2;
      f << "[" << int(relCol2) << "," << int(relRow2) << "," << int(relTab2) << "," << int(oldFlag2) << "]";
      break;
    }
    default:
      f << "##type=" << int(type) << ",";
      ok=false;
      break;
    }
    break;
  }
  case 2: // stop
    endData=true;
    break;
  case 3: { // external
    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDCParser::readSCTokenInFormula3: can not read external zone\n"));
      f << "###external";
      ok=false;
      break;
    }
    f << "\"" << text.cstr() << "\"";
    break;
  }
  case 4: { // name
    uint16_t index;
    *input >> index;
    f << "I[" << index << "]";
    break;
  }
  case 5: // jump 3
    f << "if(";
    break;
  case 6: // jump=maxjumpcount
    f << "choose(";
    break;
  default:
    f << "f" << nOp << ",";
    break;
  }
  return ok && input->tell()<=lastPos;
}

bool SDCParser::readSCOutlineArray(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  SDCParserInternal::ScMultiRecord scRecord(zone);
  if (!scRecord.open()) {
    input->seek(pos,librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCOutlineArray)[" << zone.getRecordLevel() << "]:";
  int depth=(int) input->readULong(2);
  f << "depth=" << depth << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int lev=0; lev<depth; ++lev) {
    pos=input->tell();
    f.str("");
    f << "SCOutlineArray-" << lev << ",";
    int count=(int) input->readULong(2);
    f << "entries=[";
    for (int i=0; i<count; ++i) {
      if (!scRecord.openContent("SCOutlineArray")) {
        f << "###";
        STOFF_DEBUG_MSG(("SDCParser::readSCOutlineArray:can not find some content\n"));
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        scRecord.close("SCOutlineArray");
        return true;
      }
      f << "[start=" << input->readULong(2) << ",sz=" << input->readULong(2) << ",";
      bool hidden, visible;
      *input >> hidden >> visible;
      if (hidden) f << "hidden,";
      if (visible) f << "visible,";
      f << "],";
      scRecord.closeContent("SCOutlineArray");
    }
    f << "],";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

  }
  scRecord.close("SCOutlineArray");
  return true;
}

bool SDCParser::readSCHAttributes(StarZone &zone, StarDocument &doc)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // chtmode2.cxx ChartModel::LoadAttributes
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(SCHAttributes)[" << zone.getRecordLevel() << "]:";
  int version=zone.getHeaderVersion();
  f << "vers=" << version << ",";
  double lightX, lightY, lightZ;
  int16_t nInt16;
  bool bNoMore=version<=7;
  *input >> lightX >> lightY >> lightZ;
  f << "light=" << lightX << "x" << lightY << "x" << lightZ << ",";
  if (version>=3) {
    *input>>nInt16;
    switch (nInt16) {
    case 0: // none
      bNoMore=true;
      break;
    case 1: // memchart
    case 2: // dynchart
    case 3: // memchartplus
      f << (nInt16==1 ? "memchart" : nInt16==2 ? "dynchart" : "memchartplus") << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      if (!readSCHMemChart(zone)) {
        STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not open memchart\n"));
        ascFile.addPos(pos);
        ascFile.addNote("SCHAttributes:###memchart");
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
      pos=input->tell();
      f.str("");
      f << "SCHAttributes[" << zone.getRecordLevel() << "]:";
      break;
    default:
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: find unknown chart id\n"));
      f << "###chartId=" << nInt16 << ",";
      bNoMore=true;
      break;
    }
  }
  if (version >= 5) {
    bool bIsCopied;
    *input>>bIsCopied;
    if (bIsCopied) f << "isCopied,";
  }
  if (version>=8) {
    double fMinData;
    *input>>fMinData;
    if (fMinData<0||fMinData>0) f << "fMinData=" << fMinData << ",";
  }
  *input>>nInt16;
  if (nInt16) f << "eChartStyle=" << nInt16 << ",";
  f << "linePoints=[";
  for (int i=0; i<9; ++i) {
    *input>>nInt16;
    if (nInt16) f << nInt16 << ",";
    else f << "_,";
  }
  f << "],";
  f << "rowColors=[";
  for (int i=0; i<12; ++i) {
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a color\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    f << color << ",";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "SCHAttributesA:";
  int32_t nGap, nOverlap, nMarkLen, nPieHeight;
  *input >> nGap >> nOverlap >> nMarkLen;
  if (nGap) f << "nGap=" << nGap << ",";
  if (nOverlap) f << "nOverlap=" << nOverlap << ",";
  if (nMarkLen!=100) f << "##nMarklen=" << nMarkLen << ",";
  int32_t dim[4];
  *input >> dim[0] >> dim[1] >> dim[2] >> dim[3] >> nPieHeight;
  f << "rect=" << dim[0] << "x" << dim[1] << "<->" << dim[2] << "x" << dim[3] << ",";
  if (nPieHeight) f << "height[pie]=" << nPieHeight << ",";
  if (version>=6) {
    *input>>nInt16;
    if (nInt16<0||input->tell()+4*nInt16>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read the number of pie segment\n"));
      f << "###n=" << nInt16 << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    f << "pieSegments=[";
    for (int i=0; i<int(nInt16); ++i) {
      int32_t nPieOffset;
      *input >> nPieOffset;
      if (nPieOffset) f << nPieOffset << ",";
      else
        f << "_,";
    }
    f << "],";
  }
  int16_t nXAngle, nYAngle, nZAngle, nCharSet;
  *input >> nXAngle >> nYAngle >> nZAngle >> nCharSet;
  f << "angles=" << nXAngle << "x" << nYAngle << "x" << nZAngle << ",";
  if (nCharSet) f << "charSet=" << nCharSet << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "SCHAttributesB:";
  for (int i=0; i<5; ++i) {
    bool bShow;
    librevenge::RVNGString string;
    *input>>bShow;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a string\n"));
      f << "###string,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    static char const *(wh[])= {"mainTitle", "subTitle", "xAxisTitle", "yAxisTitle", "zAxisTitle" };
    f << wh[i] << "=" << string.cstr();
    if (bShow) f << ":show";
    f << ",";
  }
  for (int i=0; i<3; ++i) {
    for (int j=0; j<4; ++j) {
      bool bShow;
      *input>>bShow;
      if (!bShow) continue;
      static char const *(wh[])= {"Axis", "GridMain", "GridHelp", "Descr"};
      f << "show" << char('X'+i) << wh[j] << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  shared_ptr<StarItemPool> pool=doc.getCurrentPool();
  if (!pool) {
    // CHANGEME
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a pool, create a false one\n"));
      first=false;
    }
    pool=doc.getNewItemPool(StarItemPool::T_Unknown);
  }
  static STOFFVec2i const(titleLimitsVec[])= {
    STOFFVec2i(4,4) /*SCHATTR_TEXT_ORIENT*/, STOFFVec2i(53,53) /*SCHATTR_TEXT_ORIENT*/,
    STOFFVec2i(100,100)/*SCHATTR_USER_DEFINED_ATTR*/, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
    STOFFVec2i(1018,1046) /*XATTR_FILL_FIRST, XATTR_FILL_LAST*/, STOFFVec2i(1067,1078) /*XATTR_SHADOW_FIRST, XATTR_SHADOW_LAST*/,
    STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/
  };
  std::vector<STOFFVec2i> titleLimits;
  for (int i=0; i<7; ++i) titleLimits.push_back(titleLimitsVec[i]);
  static STOFFVec2i const(allAxisLimitsVec[])= {
    STOFFVec2i(4,4) /*SCHATTR_TEXT_ORIENT*/, STOFFVec2i(53,54) /*SCHATTR_TEXT_ORIENT, SCHATTR_TEXT_OVERLAP*/,
    STOFFVec2i(85,85) /* SCHATTR_AXIS_SHOWDESCR */, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
    STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/, STOFFVec2i(30587,30587) /* SID_TEXTBREAK*/
  };
  std::vector<STOFFVec2i> allAxisLimits;
  for (int i=0; i<6; ++i) allAxisLimits.push_back(allAxisLimitsVec[i]);
  static STOFFVec2i const(compatAxisLimitsVec[])= {
    STOFFVec2i(4,39) /*SCHATTR_TEXT_START, SCHATTR_AXISTYPE*/, STOFFVec2i(53,54)/*SCHATTR_TEXT_DEGREES,SCHATTR_TEXT_OVERLAP*/,
    STOFFVec2i(70,95) /*SCHATTR_AXIS_START,SCHATTR_AXIS_END*/, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
    STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/, STOFFVec2i(30587,30587) /* SID_TEXTBREAK*/,
    STOFFVec2i(10585,10585) /*SID_ATTR_NUMBERFORMAT_VALUE*/
  };
  std::vector<STOFFVec2i> compatAxisLimits;
  for (int i=0; i<7; ++i) compatAxisLimits.push_back(compatAxisLimitsVec[i]);
  std::vector<STOFFVec2i> gridLimits;
  gridLimits.push_back(STOFFVec2i(1000,1016)); //XATTR_LINE_FIRST, XATTR_LINE_LAST
  gridLimits.push_back(STOFFVec2i(100,100)); // SCHATTR_USER_DEFINED_ATTR
  std::vector<STOFFVec2i> diagramAreaLimits=gridLimits;
  diagramAreaLimits.push_back(STOFFVec2i(1018,1046)); //XATTR_FILL_FIRST, XATTR_FILL_LAST
  std::vector<STOFFVec2i> legendLimits=diagramAreaLimits;
  legendLimits.push_back(STOFFVec2i(1067,1078)); //XATTR_SHADOW_FIRST, XATTR_SHADOW_LAST
  legendLimits.push_back(STOFFVec2i(3,3)); // SCHATTR_LEGEND_END
  legendLimits.push_back(STOFFVec2i(3994,4037)); // EE_ITEMS_START, EE_ITEMS_END

  for (int i=0; i<10+11; ++i) {
    if (!doc.readItemSet(zone, i<6 ? titleLimits : i==6 ? allAxisLimits : i<10 ? compatAxisLimits :
                         i<17 ? gridLimits : i < 20 ? diagramAreaLimits : legendLimits, lastPos, pool.get(), false) ||
        input->tell()>lastPos) {
      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
  }

  std::vector<STOFFVec2i> rowLimits(1, STOFFVec2i(0,0)); // CHART_ROW_WHICHPAIRS
  int nLoop = version<4 ? 2 : 3;
  for (int loop=0; loop<nLoop; ++loop) {
    pos=input->tell();
    f.str("");
    f << "SCHAttributesC:";
    *input>>nInt16;
    f << "n=" << nInt16 << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (int i=0; i<int(nInt16); ++i) {
      if (!doc.readItemSet(zone,rowLimits,lastPos, pool.get(), false) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
    }
  }

  pos=input->tell();
  f.str("");
  f << "SCHAttributesD:";
  int16_t nDataDesc;
  bool bShowSym, bSwitchData;
  *input>>nDataDesc >> bShowSym >> bSwitchData;
  if (nDataDesc) f << "dataDesc=" << nDataDesc << ",";
  if (bShowSym) f << "showSym,";
  if (bSwitchData) f << "switchData,";
  if (version==0) {
    bool bDataPercent;
    *input>>bDataPercent;
    if (bDataPercent) f << "dataPercent,";
  }
  uint32_t nValFormat, nValPercentFormat, nDescrFormat, nDescrPercentFormat;
  *input >> nValFormat >> nValPercentFormat >> nDescrFormat >> nDescrPercentFormat;
  f << "valFormat=" << nValFormat << "[" << nValPercentFormat << "],";
  f << "descrFormat=" << nDescrFormat << "[" << nDescrPercentFormat << "],";
  for (int i=0; i<3; ++i) {
    // chaxis.cxx: ChartAxis::LoadMemberCompat
    f << "chart" << char('X'+i) << "=[";
    double mfMin, mfMax, mfStep, mfStepHelp, mfOrigin;
    *input>>mfMin>> mfMax>> mfStep>> mfStepHelp>> mfOrigin;
    f << "minMax=" << mfMin << ":" << mfMax << ",";
    f << "step=" << mfStep << "[" << mfStepHelp << "],";
    if (mfOrigin<0||mfOrigin>0) f << "origin=" << mfOrigin << ",";
    f << "],";
  }
  double fMaxData;
  *input >> fMaxData;
  if (fMaxData < 0 || fMaxData>0) f << "fMaxData="  << fMaxData << ",";
  uint32_t moreData=0;
  if (!bNoMore) {
    *input >> moreData;
    f << "moreData=" << moreData << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (moreData>1) {
    pos=input->tell();
    f.str("");
    f << "SCHAttributes[moreData]:";

    for (int i=0; i<4; ++i) {
      librevenge::RVNGString string;
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a string\n"));
        f << "###string,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
      if (!string.empty()) f << "someData" << i << "=" << string.cstr() << ",";
    }
    if (moreData>=3) {
      double fSpotIntensity;
      *input>>fSpotIntensity;
      if (fSpotIntensity<0||fSpotIntensity>0) f << "spotIntensity=" << fSpotIntensity << ",";
    }
    if (moreData>=4) {
      bool bShowAverage, bDummy;
      int16_t errorKind, eIndicate;
      double indicate[4];
      *input >> bShowAverage >> errorKind >> bDummy >> eIndicate;
      if (bShowAverage) f << "show[average],";
      if (errorKind) f << "errorKind=" << errorKind << ",";
      if (eIndicate) f << "eIndicate=" << eIndicate << ",";
      *input >> indicate[0] >> indicate[1] >> indicate[2] >> indicate[3];
      f << "indicate=[" << indicate[0] << "," << indicate[1] << "," << indicate[2] << "," << indicate[3] << "],";
    }
    if (moreData>=5) {
      *input >> nInt16;
      if (nInt16) f << "eRegression=" << nInt16 << ",";
    }
    if (moreData>=6) {
      int32_t splineDepth, granularity;
      *input >> splineDepth >> granularity;
      if (splineDepth) f << "splineDepth=" << splineDepth << ",";
      if (granularity) f << "granularity=" << granularity << ",";
    }
    if (moreData>=7) {
      bool legendVisible;
      *input >> legendVisible;
      if (legendVisible) f << "legendVisible,";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (moreData>=8) {
      for (int loop=0; loop<3; ++loop) {
        pos=input->tell();
        f.str("");
        f << "SCHAttributes[attrList]:";
        *input>>nInt16;
        f << "n=" << nInt16 << ",";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        for (int i=0; i<int(nInt16); ++i) {
          if (!doc.readItemSet(zone, gridLimits, lastPos, pool.get(), false) ||
              input->tell()>lastPos) {
            zone.closeSCHHeader("SCHAttributes");
            return true;
          }
        }
      }

      pos=input->tell();
      f.str("");
      f << "SCHAttributes[moreDataB]:";
    }
    if (moreData>=9 && input->tell()<lastPos) {
      double fAmbientIntensity;
      *input>>fAmbientIntensity;
      if (fAmbientIntensity<0||fAmbientIntensity>0) f << "ambientIntensity=" << fAmbientIntensity << ",";
    }
    if (moreData>=10 && input->tell()<lastPos) {
      bool textScalable;
      *input>>textScalable;
      if (textScalable) f << "textScalable,";
    }
    if (moreData>=11 && input->tell()<lastPos) {
      int32_t sizeX, sizeY;
      *input >> sizeX >> sizeY;
      f << "initSize=" << sizeX << "x" << sizeY << ",";
    }
    if (moreData >= 12 && input->tell()<lastPos) {
      for (int i=0; i<4; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        if (i<3)
          f << "format" << char('X'+i) << "TextInMultiLine,";
        else
          f << "formatLegentTextInMultiLine,";
      }
    }
    if (moreData>=13 && input->tell()<lastPos) {
      for (int i=0; i<3; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        f << char('X'+i) << "AxisTextMaxNumLines=" << nInt16 << ",";
      }
      int32_t widthFirstXAxisText, widthLastXAxisText;
      *input >> widthFirstXAxisText >> widthLastXAxisText;
      if (widthFirstXAxisText) f << "widthFirstXAxisText=" << widthFirstXAxisText << ",";
      if (widthLastXAxisText) f << "widthLastXAxisText=" << widthLastXAxisText << ",";
      for (int i=0; i<2; ++i) {
        int32_t posX, posY;
        *input >> posX >> posY;
        if (posX!=-1 || posY!=-1) f << (i==0 ? "title" : "subTitle") << "TopCenter=" << posX << "x" << posY << ",";
      }
      int32_t diagramRectangle[4];
      *input >> diagramRectangle[0] >> diagramRectangle[1] >> diagramRectangle[2] >> diagramRectangle[3];
      f << "diagRect=" << diagramRectangle[0] << "x" << diagramRectangle[1] << "<->" << diagramRectangle[2] << "x" << diagramRectangle[3] << ",";
      for (int i=0; i<4; ++i) {
        int32_t posX, posY;
        *input >> posX >> posY;
        if (posX==-1 && posY==-1) continue;
        if (i==0)
          f << "legendTopLeft=" << posX << "x" << posY << ",";
        else
          f << "title" << char('X'+i-1) << "AxisPos=" << posX << "x" << posY << ",";
      }
      for (int i=0; i<7; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        static char const *wh[]= {"useRelPosForChartGroup","adjMargForLegend","adjMargForTitle","adjMargForSubTitle",
                                  "adjMargForXAxisTitle","adjMargForYAxisTitle","adjMargForZAxisTitle"
                                 };
        f << wh[i] << ",";
      }
    }
    if (moreData>=14 && input->tell()<lastPos) {
      for (int i=0; i<2; ++i) {
        STOFFColor col;
        if (!input->readColor(col)) {
          STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a color\n"));
          f << "###color,";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());

          zone.closeSCHHeader("SCHAttributes");
          return true;
        }
        if (!col.isWhite())
          f << (i==0 ? "spotColor" : "ambientColor") << "=" << col << ",";
      }
      for (int i=0; i<7; ++i) {
        bool hasMoved;
        *input>>hasMoved;
        if (!hasMoved) continue;
        static char const *wh[]= {"diagram", "title", "subTitle", "legend", "XAxis", "YAxis", "ZAxis"};
        f << wh[i] << "HasMoved,";
      }
    }
    if (moreData>=15 && input->tell()<lastPos) {
      for (int i=0; i<3; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        f << "adjust" << char('X'+i) << "AxesTitle=" << nInt16 << ",";
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
    }

    if (moreData>=16 && input->tell()<lastPos) {
      SWFormatManager formatManager;
      for (int i=0; i<3; ++i) {
        pos=input->tell();
        if (!formatManager.readNumberFormatter(zone)) {
          ascFile.addPos(pos);
          ascFile.addNote("SCHAttributes[format]:####");
          STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a format zone\n"));
          zone.closeSCHHeader("SCHAttributes");
          return true;
        }
      }
      pos=input->tell();
    }
    if (pos!=input->tell()) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
  }

  pos=input->tell();
  if (pos>=lastPos) {
    zone.closeSCHHeader("SCHAttributes");
    return true;
  }
  f.str("");
  f << "SCHAttributesE:";

  if (version>=9) {
    int16_t barPercentWidth;
    int32_t defaultColSet, numLineColChart, nXLastNumFmt, nYLastNumFmt, nBLastNumFmt;
    *input >> barPercentWidth >> defaultColSet >> numLineColChart >> nXLastNumFmt >> nYLastNumFmt >> nBLastNumFmt;
    if (barPercentWidth) f << "barPercentWidth=" << barPercentWidth << ",";
    if (defaultColSet) f << "defaultColSet=" << defaultColSet << ",";
    if (numLineColChart) f << "numLineColChart=" << numLineColChart << ",";
    if (nXLastNumFmt!=-1 || nYLastNumFmt!=-1 || nBLastNumFmt!=-1)
      f << "lastNumFmt=[" << nXLastNumFmt << "," << nYLastNumFmt << "," << nBLastNumFmt << "],";
    input->seek(4, librevenge::RVNG_SEEK_CUR); // dummy -1
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (version>=11 && input->tell()<lastPos) {
    for (int loop=0; loop<3; ++loop) { // the StockXXXAttr
      if (!doc.readItemSet(zone,rowLimits,lastPos, pool.get(), false) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
    }

    pos=input->tell();
  }

  if (version>=12) {
    static STOFFVec2i const(axisLimitsVec[])= {
      STOFFVec2i(4,6) /*SCHATTR_TEXT_START, SCHATTR_TEXT_END*/, STOFFVec2i(39,39) /*SCHATTR_AXISTYPE*/,
      STOFFVec2i(53,54)/*SCHATTR_TEXT_DEGREES,SCHATTR_TEXT_OVERLAP*/, STOFFVec2i(70,95) /*SCHATTR_AXIS_START,SCHATTR_AXIS_END*/,
      STOFFVec2i(100,100) /* SCHATTR_USER_DEFINED_ATTR */, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
      STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/, STOFFVec2i(11432,11432) /* SID_ATTR_NUMBERFORMAT_VALUE */,
      STOFFVec2i(30587,30587) /* SID_TEXTBREAK*/, STOFFVec2i(10585,10585) /*SID_ATTR_NUMBERFORMAT_VALUE*/
    };
    std::vector<STOFFVec2i> axisLimits;
    for (int i=0; i<8; ++i) axisLimits.push_back(axisLimitsVec[i]);
    while (input->tell()<lastPos) {
      pos=input->tell();
      int32_t nAxisId;
      *input >> nAxisId;
      f.str("");
      f << "SCHAttributesE:id=" << nAxisId << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      if (nAxisId==-1) break;
      if (!doc.readItemSet(zone,axisLimits,lastPos, pool.get(), false) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
    }
  }

  pos=input->tell();
  f.str("");
  f << "SCHAttributesF:";
  if (version>=14 && input->tell()<lastPos) {
    uint32_t nGap1, nOverlap1, nGap2, nOverlap2;
    *input >> nGap1 >> nOverlap1 >> nGap2 >> nOverlap2;
    if (nGap1) f << "nGap1=" << nGap1 << ",";
    if (nOverlap1) f << "nOverlap1=" << nOverlap1 << ",";
    if (nGap2) f << "nGap2=" << nGap2 << ",";
    if (nOverlap2) f << "nOverlap2=" << nOverlap2 << ",";
  }
  if (version>=15 && input->tell()<lastPos) {
    bool bDiagramHasBeenMoved;
    *input >> bDiagramHasBeenMoved;
    if (bDiagramHasBeenMoved) f << "diagram[movedOrRemoved],";
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  zone.closeSCHHeader("SCHAttributes");
  return true;
}

bool SDCParser::readSCHMemChart(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // memchrt.cxx: operator>> (... SchMemChart&)
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCHMemChart)[" << zone.getRecordLevel() << "]:";
  int version=zone.getHeaderVersion();
  f << "vers=" << version << ",";
  uint16_t nCol, nRow;
  *input >> nCol >> nRow;
  f << "nCol=" << nCol << ",nRow=" << nRow << ",";
  if (input->tell()+8*long(nCol)*long(nRow)>lastPos) {
    STOFF_DEBUG_MSG(("SDCParser::readSCHMemChart: dim seems bad\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCHHeader("SCHMemChart");
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (nCol && nRow) {
    for (int c=0; c < int(nCol); ++c) {
      pos=input->tell();
      f.str("");
      f << "SCHMemChart-col" << c << ":val=[";
      for (int r=0; r < int(nRow); ++r) {
        double val;
        *input>>val;
        f << val << ",";
      }
      f << "],";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
  }
  pos=input->tell();
  f.str("");
  f << "SCHMemChart-A:";
  int16_t charSet, nDataType;
  *input>>charSet;
  if (charSet) f << "charSet=" << charSet << ",";
  for (int i=0; i<5+int(nCol)+int(nRow); ++i) {
    librevenge::RVNGString string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHMemChart: can not read a title\n"));
      f << "###title";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCHHeader("SCHMemChart");
      return true;
    }
    if (string.empty()) continue;
    if (i<5) {
      static char const *(wh[])= {"mainTitle","subTitle","xAxisTitle","yAxisTitle","zAxisTitle"};
      f << wh[i] << "=" << string.cstr() << ",";
    }
    else if (i<5+int(nCol))
      f << "colTitle" << i-5 << "=" << string.cstr() << ",";
    else
      f << "rowTitle" << i-5-int(nCol) << "=" << string.cstr() << ",";
  }
  *input >> nDataType;
  if (nDataType) f << "dataType=" << nDataType << ",";
  if (version>=1) {
    int32_t val;
    f << "colTable=[";
    for (int i=0; i<int(nCol); ++i) {
      *input >> val;
      if (val)
        f << val << ",";
      else
        f << "_,";
    }
    f << "],";
    f << "rowTable=[";
    for (int i=0; i<int(nRow); ++i) {
      *input >> val;
      if (val)
        f << val << ",";
      else
        f << "_,";
    }
    f << "],";
  }
  if (version>=2) {
    int32_t nTranslated;
    *input >> nTranslated;
    if (nTranslated) f << "translated=" << nTranslated << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSCHHeader("SCHMemChart");
  return true;
}

bool SDCParser::readSdrLayer(StarZone &zone)
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayer: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLy") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayer: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }

  uint8_t layerId;
  *input>>layerId;
  if (layerId) f << "layerId=" << int(layerId) << ",";
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayer: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrLayerDef");
    return true;
  }
  f << string.cstr() << ",";
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

bool SDCParser::readSdrLayerSet(StarZone &zone)
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayerSet: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLS") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayerSet: unexpected magic data\n"));
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
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayerSet: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrLayerSet");
    return true;
  }
  f << string.cstr() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrLayerSet");
  return true;
}

bool SDCParser::readSdrModel(StarZone &zone, StarDocument &doc)
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrMd" || version>17) {
    STOFF_DEBUG_MSG(("SDCParser::readSdrModel: unexpected magic data\n"));
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not open downCompat\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: unexpected magic2 data\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read model info\n"));
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
        STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read model statistic\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read model format compat\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: reading model format for version 0 is not implemented\n"));
      f << "###version0";
      ok=false;
    }
    else {
      if (version<11) f << "nCharSet=" << input->readLong(2) << ",";
      librevenge::RVNGString string;
      for (int i=0; i<6; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read a string\n"));
          f << "###string";
          ok=false;
          break;
        }
        if (string.empty()) continue;
        static char const *(wh[])= {"cTableName", "dashName", "lineEndName", "hashName", "gradientName", "bitmapName"};
        f << wh[i] << "=" << string.cstr() << ",";
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: find unexpected child\n"));
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

bool SDCParser::readSdrObject(StarZone &zone)
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrObject: read SVDr object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else if (magic=="SCHU") {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("SDCParser::readSdrObject: read SCHU object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else if (magic=="FM01") { // FmFormInventor
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("SDCParser::readSdrObject: read FM01 object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else {
    STOFF_DEBUG_MSG(("SDCParser::readSdrObject: find unknown magic\n"));
    f << "###";
  }
  ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeSDRHeader("SdrObject");
  return true;
}

bool SDCParser::readSdrPage(StarZone &zone, StarDocument &doc)
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrPg" && magic!="DrMP") {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: unexpected magic data\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not open downCompat\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: bad version\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: bad version2\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: unexpected magic2 data\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: n is bad\n"));
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not find end object zone\n"));
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
        STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not find backgroound object\n"));
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
        librevenge::RVNGString string;
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not find read a string\n"));
          f << "###string";
        }
        else // Controls
          f << string.cstr() << ",";
        // then if vers>={13|14|15|16}  671905111105671901000800000000000000
      }
      else {
        STOFF_DEBUG_MSG(("SDCParser::readSdrPage: find some unknown zone\n"));
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

bool SDCParser::readSdrMPageDesc(StarZone &zone)
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDesc: can not open the SDR Header\n"));
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

bool SDCParser::readSdrMPageDescList(StarZone &zone)
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDescList: can not open the SDR Header\n"));
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
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDescList: can not find a page desc\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SdrMPageList:###pageDesc");
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSDRHeader("SdrMPageList");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
