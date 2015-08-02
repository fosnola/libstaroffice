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

#include "SWAttributeManager.hxx"
#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"
#include "SWZone.hxx"

#include "SDWParser.hxx"

/** Internal: the structures of a SDWParser */
namespace SDWParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDWParser
struct State {
  //! constructor
  State() : m_actPage(0), m_numPages(0)
  {
  }

  int m_actPage /** the actual page */, m_numPages /** the number of page of the final document */;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDWParser::SDWParser(STOFFInputStreamPtr input, STOFFHeader *header) :
  STOFFTextParser(input, header), m_state()
{
  init();
}

SDWParser::~SDWParser()
{
}

void SDWParser::init()
{
  setAsciiName("main-1");

  m_state.reset(new SDWParserInternal::State);
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
void SDWParser::parse(librevenge::RVNGTextInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(0L))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    checkHeader(0L);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDWParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  if (!ok) throw(libstoff::ParseException());
}


bool SDWParser::createZones()
{
  STOFFOLEParser oleParser("NIMP");
  oleParser.parse(getInput());

  // send the final data
  std::vector<std::string> unparsedOLEs=oleParser.getUnparsedOLEZones();
  size_t numUnparsed = unparsedOLEs.size();
  for (size_t i = 0; i < numUnparsed; i++) {
    std::string const &name = unparsedOLEs[i];
    STOFFInputStreamPtr ole = getInput()->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("SDWParser::createZones: error: can not find OLE part: \"%s\"\n", name.c_str()));
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
    if (base=="SwNumRules") {
      readSwNumRuleList(ole, name);
      continue;
    }
    if (base=="SwPageStyleSheets") {
      readSwPageStyleSheets(ole,name);
      continue;
    }
    if (base=="StarWriterDocument") {
      readWriterDocument(ole,name);
      continue;
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    bool ok=false;
    if (base=="persist elements")
      ok=readPersists(ole, asciiFile);
    else if (base=="SfxDocumentInfo")
      ok=readDocumentInformation(ole, asciiFile);
    else if (base=="SfxWindows")
      ok=readSfxWindows(ole, asciiFile);
    else if (base=="Star Framework Config File")
      ok=readFrameworkConfigFile(ole, asciiFile);

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
  return false;
}

////////////////////////////////////////////////////////////
// create the document
////////////////////////////////////////////////////////////
void SDWParser::createDocument(librevenge::RVNGTextInterface *documentInterface)
{
  if (!documentInterface) return;
  STOFF_DEBUG_MSG(("SDWParser::createDocument: not implemented exist\n"));
  return;
}


////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////
bool SDWParser::readPersists(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(Persists):";
  // persist.cxx: SvPersist::LoadContent
  if (input->size()<21 || input->readLong(1)!=2) {
    STOFF_DEBUG_MSG(("SDWParser::readPersists: data seems bad\n"));
    f << "###";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  int hasElt=(int) input->readLong(1);
  if (hasElt==1) {
    if (input->size()<29) {
      STOFF_DEBUG_MSG(("SDWParser::readPersists: flag hasData, but zone seems too short\n"));
      f << "###";
      hasElt=0;
    }
    f << "hasData,";
  }
  else if (hasElt) {
    STOFF_DEBUG_MSG(("SDWParser::readPersists: flag hasData seems bad\n"));
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
      STOFF_DEBUG_MSG(("SDWParser::readPersists: data size seems bad\n"));
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
      STOFF_DEBUG_MSG(("SDWParser::readPersists: data %d seems bad\n", i));
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
      STOFF_DEBUG_MSG(("SDWParser::readPersists: dSz seems bad\n"));
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
      STOFF_DEBUG_MSG(("SDWParser::readPersists: data %d seems bad\n", i));
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

bool SDWParser::readDocumentInformation(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
{
  // see sfx2_docinf.cxx
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(DocInfo):";
  int sSz=(int) input->readULong(2);
  if (2+sSz>input->size()) {
    STOFF_DEBUG_MSG(("SDWParser::readDocumentInformation: header seems bad\n"));
    f << "###sSz=" << sSz << ",";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  std::string text("");
  for (int i=0; i<sSz; ++i) text+=(char) input->readULong(1);
  if (text!="SfxDocumentInfo") {
    STOFF_DEBUG_MSG(("SDWParser::readDocumentInformation: header seems bad\n"));
    f << "###text=" << text << ",";
    ascii.addPos(0);
    ascii.addNote(f.str().c_str());
    return true;
  }
  int val=(int) input->readULong(2);
  if (val)
    f << "vers=" << val << ",";
  val=(int) input->readULong(1);
  if (val==1)
    f << "passwd,";
  else if (val)
    f << "passwd=" << val << ",";
  f << "charSet=" << input->readULong(2) << ",";
  for (int i=0; i<2; ++i) {
    val=(int) input->readULong(1);
    if (!val) continue;
    f << (i==0 ? "graph[portable]" : "template");
    if (val==1)
      f <<",";
    else if (val)
      f << "=" << val << ",";
  }
  ascii.addPos(0);
  ascii.addNote(f.str().c_str());

  for (int i=0; i<17; ++i) {
    long pos=input->tell();
    f.str("");
    f << "DocInfo-A" << i << ":";
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
        expectedSz+=0x10000;
      else
        expectedSz=2+dSz;
    }
    if (pos+expectedSz+(i<3 ? 8 : 0)>input->size()) {
      STOFF_DEBUG_MSG(("SDWParser::readDocumentInformation: can not read string %d\n", i));
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
      f << "date=" << input->readULong(4) << ",";
      f << "time=" << input->readULong(4) << ",";
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  long pos=input->tell();
  f.str("");
  f << "DocInfo-B:";
  if (pos+8>input->size()) {
    STOFF_DEBUG_MSG(("SDWParser::readDocumentInformation: last zone seems too short\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return true;
  }
  f << "date=" << input->readULong(4) << ","; // YYYYMMDD
  f << "time=" << input->readULong(4) << ","; // HHMMSS00
  if (!input->isEnd()) // [MailAddr], lTime, ...
    ascii.addDelimiter(input->tell(),'|');
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  return true;
}

bool SDWParser::readFrameworkConfigFile(STOFFInputStreamPtr input, libstoff::DebugFile &asciiFile)
{
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugStream f;
  f << "Entries(StarFrameworkConfig):";
  // see sfx2_cfgimex SfxConfigManagerImExport_Impl::Import
  std::string header("");
  for (int i=0; i<26; ++i) header+=(char) input->readULong(1);
  if (!input->checkPosition(33)||header!="Star Framework Config File") {
    STOFF_DEBUG_MSG(("SDWParser::readFrameworkConfigFile: the header seems bad\n"));
    f << "###" << header;
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    return true;
  }
  int val=(int) input->readULong(1); // cCtrlZ
  if (val!=26) f << "f0=" << val << ",";
  val=(int) input->readULong(2); // 26: normal version
  if (val!=26) f << "vers=" << input->readULong(2) << ",";
  long pos=(long) input->readULong(4);
  if (!input->checkPosition(pos+2)) {
    STOFF_DEBUG_MSG(("SDWParser::readFrameworkConfigFile: dir pos is bad\n"));
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
  int N=(int) input->readULong(2);
  f << "N=" << N << ",";
  for (int i=0; i<N; ++i) {
    f << "item" << i << "=[";
    f << "nType=" << input->readULong(2) << ",";
    long lPos=input->readLong(4);
    if (lPos!=-1) {
      f << "lPos=" << input->readLong(4) << ",";
      if (!input->checkPosition(lPos)) {
        STOFF_DEBUG_MSG(("SDWParser::readFrameworkConfigFile: a item position seems bad\n"));
        f << "###";
      }
      else {
        static bool first=true;
        if (first) {
          STOFF_DEBUG_MSG(("SDWParser::readFrameworkConfigFile: Ohhh find some item\n"));
          first=true;
        }
        asciiFile.addPos(lPos);
        asciiFile.addNote("StarFrameworkConfig[Item]:###");
        // see SfxConfigManagerImExport_Impl::ImportItem, if needed
      }
    }
    f << "lLength=" << input->readLong(4) << ",";
    int strSz=(int) input->readULong(2);
    if (!input->checkPosition(input->tell()+strSz)) {
      STOFF_DEBUG_MSG(("SDWParser::readFrameworkConfigFile: a item seems bad\n"));
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

bool SDWParser::readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii)
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
    STOFF_DEBUG_MSG(("SDWParser::readSfxWindows: find extra data\n"));
    ascii.addPos(input->tell());
    ascii.addNote("SfWindows:extra###");
  }
  return true;
}

bool SDWParser::readSwNumRuleList(STOFFInputStreamPtr input, std::string const &name)
{
  SWZone zone(input, name, "SWNumRuleList");
  if (!zone.readHeader()) {
    STOFF_DEBUG_MSG(("SDWParser::readSwNumRuleList: can not read the header\n"));
    return false;
  }
  zone.readStringsPool();
  // sw_sw3num.cxx::Sw3IoImp::InNumRules
  libstoff::DebugFile &ascFile=zone.ascii();
  while (!input->isEnd()) {
    long pos=input->tell();
    int rType=input->peek();
    if ((rType=='0' || rType=='R') && readSWNumRule(zone, char(rType)))
      continue;
    char type;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWNumRuleList[" << type << "]:";
    bool done=false;
    switch (type) {
    case '+': { // extra outline
      zone.openFlagZone();
      int N=(int) input->readULong(1);
      f << "N=" << N << ",";
      zone.closeFlagZone();
      if (input->tell()+3*N>zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("SDWParser::readSwNumRuleList: nExtraOutline seems bad\n"));
        f << "###,";
        break;
      }
      f << "levSpace=[";
      for (int i=0; i<N; ++i)
        f << input->readULong(1) << ":" << input->readULong(2) << ",";
      f << "],";
      break;
    }
    case '?':
      STOFF_DEBUG_MSG(("SDWParser::readSwNumRuleList: reading inHugeRecord(TEST_HUGE_DOCS) is not implemented\n"));
      break;
    case 'Z':
      done=true;
      break;
    default:
      STOFF_DEBUG_MSG(("SDWParser::readSwNumRuleList: find unimplemented type\n"));
      f << "###type,";
      break;
    }
    if (!zone.closeRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (done)
      break;
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDWParser::readSwNumRuleList: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWNumRuleList:###extra");
  }
  return true;
}

bool SDWParser::readSwPageStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  SWZone zone(input, name, "SWPageStyleSheets");
  if (!zone.readHeader()) {
    STOFF_DEBUG_MSG(("SDWParser::readSwPageStyleSheets: can not read the header\n"));
    return false;
  }
  zone.readStringsPool();
  SWFieldManager fieldManager;
  while (fieldManager.readField(zone,'Y'))
    ;
  readSWBookmarkList(zone);
  readSWRedlineList(zone);
  SWFormatManager formatManager;
  formatManager.readSWNumberFormatterList(zone);

  // sw_sw3page.cxx Sw3IoImp::InPageDesc
  libstoff::DebugFile &ascFile=zone.ascii();
  while (!input->isEnd()) {
    long pos=input->tell();
    char type;
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWPageStyleSheets[" << type << "]:";
    bool done=false;
    switch (type) {
    case 'P': {
      zone.openFlagZone();
      int N=(int) input->readULong(2);
      f << "N=" << N << ",";
      zone.closeFlagZone();
      for (int i=0; i<N; ++i) {
        if (!readSWPageDef(zone))
          break;
      }
      break;
    }
    case 'Z':
      done=true;
      break;
    default:
      STOFF_DEBUG_MSG(("SDWParser::readSwPageStyleSheets: find unknown data\n"));
      f << "###";
      break;
    }
    if (!zone.closeRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (done)
      break;
  }

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDWParser::readSwPageStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWPageStyleSheets:##extra");
  }

  return true;
}

bool SDWParser::readSWPageDef(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='p' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3page.cxx InPageDesc
  libstoff::DebugStream f;
  f << "Entries(SWPageDef)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  int val=(int) input->readULong(2);
  librevenge::RVNGString poolName;
  if (!zone.getPoolName(val, poolName)) {
    STOFF_DEBUG_MSG(("SDWParser::readSwPageDef: can not find a pool name\n"));
    f << "###nId=" << val << ",";
  }
  else if (!poolName.empty())
    f << poolName.cstr() << ",";
  val=(int) input->readULong(2);
  if (val) f << "nFollow=" << val << ",";
  val=(int) input->readULong(2);
  if (val) f << "nPoolId2=" << val << ",";
  val=(int) input->readULong(1);
  if (val) f << "nNumType=" << val << ",";
  val=(int) input->readULong(2);
  if (val) f << "nUseOn=" << val << ",";
  if (zone.isCompatibleWith(0x16,0x22, 0x101)) {
    val=(int) input->readULong(2);
    if (val!=0xffff) f << "regCollIdx=" << val << ",";
  }
  zone.closeFlagZone();

  long lastPos=zone.getRecordLastPosition();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  SWAttributeManager attributeManager;
  while (input->tell() < lastPos) {
    pos=input->tell();
    int rType=input->peek();
    if (rType=='S' && attributeManager.readAttributeList(zone, *this))
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    f.str("");
    if (!zone.openRecord(type)) {
      STOFF_DEBUG_MSG(("SDWParser::readSwPageDef: find extra data\n"));
      ascFile.addPos(pos);
      ascFile.addNote("SWPageDef:###extra");
      break;
    }
    f << "SWPageDef[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case '1': // foot info
    case '2': { // page foot info
      f << (type=='1' ? "footInfo" : "pageFootInfo") << ",";
      val=(int) input->readLong(4);
      if (val) f << "height=" << val << ",";
      val=(int) input->readLong(4);
      if (val) f << "topDist=" << val << ",";
      val=(int) input->readLong(4);
      if (val) f << "bottomDist=" << val << ",";
      val=(int) input->readLong(2);
      if (val) f << "adjust=" << val << ",";
      f << "width=" << input->readLong(4) << "/" << input->readLong(4) << ",";
      val=(int) input->readLong(2);
      if (val) f << "penWidth=" << val << ",";
      STOFFColor col;
      if (!input->readColor(col)) {
        STOFF_DEBUG_MSG(("SDWParser::readSwPageDef: can not read a color\n"));
        f << "###color,";
      }
      else if (!col.isBlack())
        f << col << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDWParser::readSwPageDef: find unknown type\n"));
      f << "###type,";
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  zone.closeRecord('p');
  return true;
}

bool SDWParser::readSWBookmarkList(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='a' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // sw_sw3misc.cxx InBookmarks
  libstoff::DebugStream f;
  f << "Entries(SWBookmark)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    f.str("");
    f << "SWBookmark:";
    if (input->peek()!='B' || !zone.openRecord(type)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWBookmarkList: find extra data\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }

    librevenge::RVNGString text("");
    bool ok=true;
    if (!zone.readString(text)) {
      ok=false;
      STOFF_DEBUG_MSG(("SDWParser::readSWBookmarkList: can not read shortName\n"));
      f << "###short";
    }
    else
      f << text.cstr();
    if (ok && !zone.readString(text)) {
      ok=false;
      STOFF_DEBUG_MSG(("SDWParser::readSWBookmarkList: can not read name\n"));
      f << "###";
    }
    else
      f << text.cstr();
    if (ok) {
      zone.openFlagZone();
      int val=(int) input->readULong(2);
      if (val) f << "nOffset=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nKey=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nMod=" << val << ",";
      zone.closeFlagZone();

      // we may still have some macros: start:aMac, aLib, end:aMac, aLib
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }

  zone.closeRecord('a');
  return true;
}

bool SDWParser::readSWContent(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='N' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3sectn.cxx: InContents
  libstoff::DebugStream f;
  f << "Entries(SWContent)[" << zone.getRecordLevel() << "]:";
  if (zone.isCompatibleWith(5))
    zone.openFlagZone();
  int nNodes;
  if (zone.isCompatibleWith(0x201))
    nNodes=(int) input->readULong(4);
  else {
    if (zone.isCompatibleWith(5))
      f << "sectId=" << input->readULong(2) << ",";
    nNodes=(int) input->readULong(2);
  }
  f << "N=" << nNodes << ",";
  if (zone.isCompatibleWith(5))
    zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  SWFormatManager formatManager;
  for (int i=0; i<nNodes; ++i) {
    if (input->tell()>=lastPos) break;
    pos=input->tell();
    int cType=input->peek();
    bool done=false;
    switch (cType) {
    case 'E':
      done=readSWTable(zone);
      break;
    case 'G':
      done=readSWGraphNode(zone);
      break;
    case 'I':
      done=readSWSection(zone);
      break;
    case 'O':
      done=readSWOLENode(zone);
      break;
    case 'T':
      done=readSWTextZone(zone);
      break;
    case 'l': // related to link
    case 'o': // format: safe to ignore
      done=formatManager.readSWFormatDef(zone,char(cType),*this);
      break;
    case 'v':
      done=readSWNodeRedline(zone);
      break;
    default:
      break;
    }
    if (done) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWContent[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (cType) {
    case 'i':
      // sw_sw3node.cxx InRepTxtNode
      f << "repTxtNode,";
      f << "rep=" << input->readULong(4) << ",";
      break;
    default:
      STOFF_DEBUG_MSG(("SDWParser::readSWContent: find unexpected type\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWContent: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWContent:###extra");
  }
  zone.closeRecord('N');
  return true;
}

bool SDWParser::readSWDBName(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='D' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx: InDBName
  libstoff::DebugStream f;
  f << "Entries(SWDBName)[" << zone.getRecordLevel() << "]:";
  librevenge::RVNGString text("");
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("SDWParser::readSWDBName: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord('D');
    return true;
  }
  if (!text.empty())
    f << "sStr=" << text.cstr() << ",";
  if (zone.isCompatibleWith(0xf,0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWDBName: can not read a SQL string\n"));
      f << "###SQL";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('D');
      return true;
    }
    if (!text.empty())
      f << "sSQL=" << text.cstr() << ",";
  }
  if (zone.isCompatibleWith(0x11,0x22)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWDBName: can not read a table name string\n"));
      f << "###tableName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('D');
      return true;
    }
    if (!text.empty())
      f << "sTableName=" << text.cstr() << ",";
  }
  if (zone.isCompatibleWith(0x12,0x22, 0x101)) {
    int nCount=(int) input->readULong(2);
    f << "nCount=" << nCount << ",";
    if (nCount>0 && zone.isCompatibleWith(0x28)) {
      f << "dbData=[";
      for (int i=0; i<nCount; ++i) {
        if (input->tell()>=zone.getRecordLastPosition()) {
          STOFF_DEBUG_MSG(("SDWParser::readSWDBName: can not read a DBData\n"));
          f << "###";
          break;
        }
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("SDWParser::readSWDBName: can not read a table name string\n"));
          f << "###dbDataName";
          break;
        }
        f << text.cstr() << ":";
        f << input->readULong(4) << "<->"; // selStart
        f << input->readULong(4) << ","; // selEnd
      }
      f << "],";
    }
  }
  if (input->tell()<zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SDWParser::readSWDBName: find extra data\n"));
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('D');
  return true;
}

bool SDWParser::readSWDictionary(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='j' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InDictionary
  libstoff::DebugStream f;
  f << "Entries(SWDictionary)[" << zone.getRecordLevel() << "]:";
  long lastPos=zone.getRecordLastPosition();
  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    f << "[";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWDictionary: can not read a string\n"));
      f << "###string,";
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    if (!string.empty())
      f << string.cstr() << ",";
    f << "nLanguage=" << input->readULong(2) << ",";
    f << "nCount=" << input->readULong(2) << ",";
    f << "c=" << input->readULong(1) << ",";
    f << "],";
  }
  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWDictionary: find extra data\n"));
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeRecord('j');
  return true;
}

bool SDWParser::readSWEndNoteInfo(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='4' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx: InEndNoteInfo
  libstoff::DebugStream f;
  f << "Entries(SWEndNoteInfo)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  f << "eType=" << input->readULong(1) << ",";
  f << "nPageId=" << input->readULong(2) << ",";
  f << "nCollIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0xc))
    f << "nFtnOffset=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x203))
    f << "nChrIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x216) && (fl&0x10))
    f << "nAnchorChrIdx=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (zone.isCompatibleWith(0x203)) {
    librevenge::RVNGString text("");
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWEndNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeRecord('4');
        return true;
      }
      if (!text.empty())
        f << (i==0 ? "prefix":"suffix") << "=" << text.cstr() << ",";
    }
  }
  if (input->tell()<zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SDWParser::readSWEndNoteInfo: find extra data\n"));
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('4');
  return true;
}

bool SDWParser::readSWFootNoteInfo(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='1' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx: InFtnInfo and InFntInfo40
  libstoff::DebugStream f;
  f << "Entries(SWFootNoteInfo)[" << zone.getRecordLevel() << "]:";
  bool old=!zone.isCompatibleWith(0x201);
  librevenge::RVNGString text("");
  if (old) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWFootNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeRecord('1');
      }
      if (!text.empty())
        f << (i==0 ? "quoVadis":"ergoSum") << "=" << text.cstr() << ",";
    }
  }
  int fl=zone.openFlagZone();

  if (old) {
    f << "ePos=" << input->readULong(1) << ",";
    f << "eNum=" << input->readULong(1) << ",";
  }
  f << "eType=" << input->readULong(1) << ",";
  f << "nPageId=" << input->readULong(2) << ",";
  f << "nCollIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0xc))
    f << "nFtnOffset=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x203))
    f << "nChrIdx=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x216) && (fl&0x10))
    f << "nAnchorChrIdx=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (zone.isCompatibleWith(0x203)) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWFootNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeRecord('1');
        return true;
      }
      if (!text.empty())
        f << (i==0 ? "prefix":"suffix") << "=" << text.cstr() << ",";
    }
  }

  if (!old) {
    zone.openFlagZone();
    f << "ePos=" << input->readULong(1) << ",";
    f << "eNum=" << input->readULong(1) << ",";
    zone.closeFlagZone();
    for (int i=0; i<2; ++i) {
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWFootNoteInfo: can not read a string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeRecord('1');
        return true;
      }
      if (!text.empty())
        f << (i==0 ? "quoVadis":"ergoSum") << "=" << text.cstr() << ",";
    }
  }
  if (input->tell()<zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SDWParser::readSWFootNoteInfo: find extra data\n"));
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('1');
  return true;
}

bool SDWParser::readSWGraphNode(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='G' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InGrfNode
  libstoff::DebugStream f;
  f << "Entries(SWGraphNode)[" << zone.getRecordLevel() << "]:";

  librevenge::RVNGString text;
  int fl=zone.openFlagZone();
  if (fl&0x10) f << "link,";
  if (fl&0x20) f << "empty,";
  if (fl&0x40) f << "serverMap,";
  zone.closeFlagZone();
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWGraphNode: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('G');
      return true;
    }
    if (!text.empty())
      f << (i==0 ? "grfName" : "fltName") << "=" << text.cstr() << ",";
  }
  if (zone.isCompatibleWith(0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWGraphNode: can not read a objName\n"));
      f << "###textRepl";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('G');
      return true;
    }
    if (!text.empty())
      f << "textRepl=" << text.cstr() << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  SWAttributeManager attributeManager;
  while (input->tell() < lastPos) {
    pos=input->tell();
    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'S':
      done=attributeManager.readAttributeList(zone, *this);
      break;
    case 'X':
      done=readSWImageMap(zone);
      break;
    default:
      break;
    }
    if (done)
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWGraphNode[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case 'k': {
      // sw_sw3nodes.cxx InContour
      int polyFl=zone.openFlagZone();
      zone.closeFlagZone();
      if (polyFl&0x10) {
        // poly2.cxx operator>>
        int numPoly=(int) input->readULong(2);
        for (int i=0; i<numPoly; ++i) {
          f << "poly" << i << "=[";
          // poly.cxx operator>>
          int numPoints=(int) input->readULong(2);
          if (input->tell()+8*numPoints>lastPos) {
            STOFF_DEBUG_MSG(("SDWParser::readSWGraphNode: can not read a polygon\n"));
            f << "###poly";
            break;
          }
          for (int p=0; p<numPoints; ++p)
            f << input->readLong(4) << "x" << input->readLong(4) << ",";
          f << "],";
        }
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDWParser::readSWGraphNode: find unexpected type\n"));
      f << "###";
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  if (input->tell()!=lastPos) {
    ascFile.addDelimiter(input->tell(),'|');
    STOFF_DEBUG_MSG(("SDWParser::readSWGraphNode: extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWGraphNode:###extra");
  }

  zone.closeRecord('G');
  return true;
}

bool SDWParser::readSWImageMap(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='X' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugStream f;
  f << "Entries(SWImageMap)[" << zone.getRecordLevel() << "]:";
  // sw_sw3nodes.cxx InImageMap
  int flag=zone.openFlagZone();
  if (flag&0xF0) f << "fl=" << flag << ",";
  zone.closeFlagZone();
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("SDWParser::readSWImageMap: can not read url\n"));
    f << "###url";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    zone.closeRecord('X');
    return true;
  }
  if (!string.empty())
    f << "url=" << string.cstr() << ",";
  if (zone.isCompatibleWith(0x11,0x22, 0x101)) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWImageMap: can not read string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        zone.closeRecord('X');
        return true;
      }
      if (string.empty()) continue;
      f << (i==0 ? "target" : "dummy") << "=" << string.cstr() << ",";
    }
  }
  if (flag&0x20) {
    // svt_imap.cxx: ImageMap::Read
    std::string cMagic("");
    for (int i=0; i<6; ++i) cMagic+=(char) input->readULong(1);
    if (cMagic!="SDIMAP") {
      STOFF_DEBUG_MSG(("SDWParser::readSWImageMap: cMagic is bad\n"));
      f << "###cMagic=" << cMagic << ",";
    }
    else {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDWParser::readSWImageMap: can not read string\n"));
          f << "###string";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());

          zone.closeRecord('X');
          return true;
        }
        if (!string.empty())
          f << (i==0 ? "target" : i==1 ? "dummy1" : "dummy2") << "=" << string.cstr() << ",";
        if (i==1)
          f << "nCount=" << input->readULong(2) << ",";
      }
      if (input->tell()<zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("SDWParser::readSWImageMap: find imapCompat data, not implemented\n"));
        // svt_imap3.cxx IMapCompat::IMapCompat
        ascFile.addPos(input->tell());
        ascFile.addNote("SWImageMap:###IMapCompat");
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      }
    }
  }
  if (input->tell()!=zone.getRecordLastPosition()) {
    ascFile.addDelimiter(input->tell(),'|');
    STOFF_DEBUG_MSG(("SDWParser::readSWImageMap: extra data\n"));
    f << "###extra:";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('X');
  return true;
}

bool SDWParser::readSWJobSetUp(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='J' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InJobSetup
  libstoff::DebugStream f;
  f << "Entries(SWJobSetUp)[" << zone.getRecordLevel() << "]:";
  zone.openFlagZone();
  zone.closeFlagZone();
  // sfx2_printer.cxx: SfxPrinter::Create
  // jobset.cxx: JobSetup operator>>
  long lastPos=zone.getRecordLastPosition();
  int len=(int) input->readULong(2);
  f << "nLen=" << len << ",";
  if (len==0) {
    if (input->tell()<lastPos) {
      f << "###extra,";
      STOFF_DEBUG_MSG(("SDWParser::readSWJobSetUp: find extra data\n"));
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord('J');
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

    ascFile.addPos(input->tell());
    ascFile.addNote("SWJobSetUp:driverData");
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
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (driverDataLen && input->tell()+driverDataLen<=lastPos) {
      ascFile.addPos(input->tell());
      ascFile.addNote("SWJobSetUp:driverData");
      input->seek(driverDataLen, librevenge::RVNG_SEEK_CUR);
    }
    else if (driverDataLen)
      ok=false;
    if (ok) {
      pos=input->tell();
      f.str("");
      f << "SWJobSetUp[values]:";
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
        if (ok) {
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
        }
      }
      else if (input->tell()<lastPos) {
        ascFile.addPos(input->tell());
        ascFile.addNote("SWJobSetUp:driverData");
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      }
    }
  }
  else
    ok=false;

  if (!ok || input->tell()<lastPos) {
    f << "###extra,";
    STOFF_DEBUG_MSG(("SDWParser::readSWJobSetUp: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  zone.closeRecord('J');
  return true;
}

bool SDWParser::readSWLayoutInfo(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='U' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // find no code, let improvise
  libstoff::DebugStream f;
  f << "Entries(SWLayoutInfo)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  f << "f0=" << input->readULong(2) << ",";
  if (input->tell()!=zone.getFlagLastPosition()) // maybe
    f << "f1=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (readSWLayoutSub(zone)) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWLayoutInfo[" << std::hex << int((unsigned char)type) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
    break;
  }

  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWLayoutInfo: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWLayoutInfo:###");
  }
  zone.closeRecord('U');
  return true;
}

bool SDWParser::readSWLayoutSub(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  int rType=input->peek();
  if ((rType!=0xd2&&rType!=0xd7) || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // find no code, let improvise
  libstoff::DebugStream f;
  f << "Entries(SWLayoutSub)[" << std::hex << rType << std::dec << "-" << zone.getRecordLevel() << "]:";
  int const expectedSz=rType==0xd2 ? 11 : 9;
  long lastPos=zone.getRecordLastPosition();
  int val=(int) input->readULong(1);
  if (val!=0x11) f << "f0=" << val << ",";
  val=(int) input->readULong(1);
  if (val!=0xaf) f << "f1=" << val << ",";
  val=(int) input->readULong(1); // small value 1-1f
  if (val) f << "f2=" << val << ",";
  val=(int) input->readULong(1);
  if (val) f << "f3=" << std::hex << val << std::dec << ",";
  if (input->tell()+(val&0xf)+expectedSz>lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWLayoutSub: the zone seems too short\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(char(0xd2));
    return true;
  }
  ascFile.addDelimiter(input->tell(),'|');
  input->seek(input->tell()+(val&0xf)+expectedSz, librevenge::RVNG_SEEK_SET);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos) {
    pos=input->tell();
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWLayoutSub[" << std::hex << int((unsigned char)type) << std::dec << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }

  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWLayoutSub: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWLayoutSub:###");
  }
  zone.closeRecord(char(rType));
  return true;
}

bool SDWParser::readSWMacroTable(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='M' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InMacroTable
  libstoff::DebugStream f;
  f << "Entries(SWMacroTable)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='m' || !zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f << "SWMacroTable:";
    f << "key=" << input->readULong(2) << ",";
    bool ok=true;
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWMacroTable: can not read a string\n"));
        f << "###,";
        ok=false;
        break;
      }
      if (!string.empty())
        f << string.cstr() << ",";
    }
    if (ok && zone.isCompatibleWith(0x102))
      f << "scriptType=" << input->readULong(2) << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWMacroTable: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWMacroTable:###extra");
  }
  zone.closeRecord('M');
  return true;
}

bool SDWParser::readSWNodeRedline(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='v' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3redln.cxx InNodeRedLine
  libstoff::DebugStream f;
  f << "Entries(SWNodeRedline)[" << zone.getRecordLevel() << "]:";
  int cFlag=zone.openFlagZone();
  if (cFlag&0xf0) f << "flag=" << (cFlag>>4) << ",";
  f << "nId=" << input->readULong(2) << ",";
  f << "nNodeOf=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (input->tell()!=zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SDWParser::readSWNodeRedline: find extra data\n"));
    f << "###extra";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('v');
  return true;
}

bool SDWParser::readSWNumRule(SWZone &zone, char cKind)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!=cKind || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3num.cxx::Sw3IoImp::InNumRule
  libstoff::DebugStream f;
  f << "Entries(SWNumRuleDef)[" << cKind << "-" << zone.getRecordLevel() << "]:";
  int cFlags=0x20, nPoolId=-1;
  int val;
  if (zone.isCompatibleWith(0x201)) {
    cFlags=(int) zone.openFlagZone();
    val=(int) input->readULong(2);
    if (val) f << "nStringId=" << val << ",";
    if (cFlags&0x10) {
      nPoolId=(int) input->readULong(2);
      librevenge::RVNGString poolName;
      if (!zone.getPoolName(nPoolId, poolName))
        f << "###nPoolId=" << nPoolId << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
      val=(int) input->readULong(2);
      if (val) f << "poolHelpId=" << val << ",";
      val=(int) input->readULong(1);
      if (val) f << "poolHelpFileId=" << val << ",";
    }
  }
  val=(int) input->readULong(1);
  if (val) f << "eType=" << val << ",";
  if (zone.isCompatibleWith(0x201))
    zone.closeFlagZone();
  int nFormat=(int) input->readULong(1);
  long lastPos=zone.getRecordLastPosition();
  f << "nFormat=" << nFormat << ",";
  if (input->tell()+nFormat>lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSwNumRule: nFormat seems bad\n"));
    f << "###,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(cKind);
    return true;
  }
  f << "lvl=[";
  for (int i=0; i<nFormat; ++i) f  << input->readULong(1) << ",";
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  int nKnownFormat=nFormat>10 ? 10 : nFormat;
  SWFormatManager formatManager;
  for (int i=0; i<nKnownFormat; ++i) {
    pos=input->tell();
    if (formatManager.readSWNumberFormat(zone)) continue;
    STOFF_DEBUG_MSG(("SDWParser::readSwNumRule: can not read a format\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSwNumRule: find extra data for %d zones\n", type));
    f.str("");
    f << "SWNumRuleDef[R]:###extra:";
    ascFile.addPos(input->tell());
    ascFile.addNote(f.str().c_str());
  }

  zone.closeRecord(cKind);
  return true;
}

bool SDWParser::readSWOLENode(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='O' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InOLENode
  libstoff::DebugStream f;
  f << "Entries(SWOLENode)[" << zone.getRecordLevel() << "]:";

  librevenge::RVNGString text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("SDWParser::readSWOLENode: can not read a objName\n"));
    f << "###objName";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord('O');
    return true;
  }
  if (!text.empty())
    f << "objName=" << text.cstr() << ",";
  if (zone.isCompatibleWith(0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWOLENode: can not read a objName\n"));
      f << "###textRepl";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('O');
      return true;
    }
    if (!text.empty())
      f << "textRepl=" << text.cstr() << ",";
  }
  if (input->tell()!=zone.getRecordLastPosition()) {
    ascFile.addDelimiter(input->tell(),'|');
    STOFF_DEBUG_MSG(("SDWParser::readSWOLENode: extra data\n"));
    f << "###extra:";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('O');
  return true;
}

bool SDWParser::readSWRedlineList(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='V' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3redline.cxx inRedlines
  libstoff::DebugStream f;
  f << "Entries(SWRedline)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<zone.getRecordLastPosition()) {
    pos=input->tell();
    f.str("");
    f << "SWRedline:";
    if (input->peek()!='R' || !zone.openRecord(type)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWRedlineList: find extra data\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    zone.openFlagZone();
    int N=(int) input->readULong(2);
    zone.closeFlagZone();
    f << "N=" << N << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (int i=0; i<N; ++i) {
      pos=input->tell();
      f.str("");
      f << "SWRedline-" << i << ":";
      if (input->peek()!='D' || !zone.openRecord(type)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWRedlineList: can not read a redline\n"));
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        break;
      }

      zone.openFlagZone();
      int val=(int) input->readULong(1);
      if (val) f << "cType=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "stringId=" << val << ",";
      zone.closeFlagZone();

      f << "date=" << input->readULong(4) << ",";
      f << "time=" << input->readULong(4) << ",";
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWRedlineList: can not read the comment\n"));
        f << "###comment";
      }
      else if (!text.empty())
        f << text.cstr();
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('D');
    }
    zone.closeRecord('R');
  }
  zone.closeRecord('V');
  return true;
}

bool SDWParser::readSWSection(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='I' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3sectn.cxx: InSection
  libstoff::DebugStream f;
  f << "Entries(SWSection)[" << zone.getRecordLevel() << "]:";

  librevenge::RVNGString text;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWSection: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('I');
      return true;
    }
    if (text.empty()) continue;
    f << (i==0 ? "name" : "cond") << "=" << text.cstr() << ",";
  }
  int fl=zone.openFlagZone();
  if (fl&0x10) f << "hidden,";
  if (fl&0x20) f << "protect,";
  if (fl&0x40) f << "condHidden,";
  if (fl&0x40) f << "connectFlag,";
  f << "nType=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  if (zone.isCompatibleWith(0xd)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWSection: can not read a linkName\n"));
      f << "###linkName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord('I');
      return true;
    }
    else if (!text.empty())
      f << "linkName=" << text.cstr() << ",";
  }
  if (input->tell()!=zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SDWParser::readSWSection: find extra data\n"));
    f << "###extra";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('I');
  return true;
}

bool SDWParser::readSWTable(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='E' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTable
  libstoff::DebugStream f;
  f << "Entries(SWTableDef)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20) f << "headerRepeat,";
  f << "nBoxes=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x5,0x201))
    f << "nTblIdDummy=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x103))
    f << "cChgMode=" << input->readULong(1) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  SWFormatManager formatManager;
  if (input->peek()=='f') formatManager.readSWFormatDef(zone, 'f', *this);
  if (input->peek()=='Y') {
    SWFieldManager fieldManager;
    fieldManager.readField(zone,'Y');
  }
  while (input->tell()<lastPos && input->peek()=='v') {
    pos=input->tell();
    if (readSWNodeRedline(zone))
      continue;
    STOFF_DEBUG_MSG(("SDWParser::readSWTable: can not read a red line\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWTableDef:###redline");
    zone.closeRecord('E');
    return true;
  }

  while (input->tell()<lastPos && input->peek()=='L') {
    pos=input->tell();
    if (readSWTableLine(zone))
      continue;
    pos=input->tell();
    STOFF_DEBUG_MSG(("SDWParser::readSWTable: can not read a table line\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTable: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWTableDef:###extra");
  }
  zone.closeRecord('E');
  return true;
}

bool SDWParser::readSWTableBox(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='t' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTableBox
  libstoff::DebugStream f;
  f << "Entries(SWTableBox)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20 || !zone.isCompatibleWith(0x201))
    f << "fmtId=" << input->readULong(2) << ",";
  if (fl&0x10)
    f << "numLines=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  SWFormatManager formatManager;
  if (input->peek()=='f') formatManager.readSWFormatDef(zone,'f',*this);
  if (input->peek()=='N') readSWContent(zone);
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (readSWTableLine(zone)) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTableBox: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWTableBox:###extra");
  }
  zone.closeRecord('t');
  return true;
}

bool SDWParser::readSWTableLine(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='L' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3table.cxx: InTableLine
  libstoff::DebugStream f;
  f << "Entries(SWTableLine)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x20 || !zone.isCompatibleWith(0x201))
    f << "fmtId=" << input->readULong(2) << ",";
  f << "nBoxes=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  SWFormatManager formatManager;
  if (input->peek()=='f')
    formatManager.readSWFormatDef(zone,'f',*this);

  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (readSWTableBox(zone))
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTableLine: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWTableLine:###extra");
  }
  zone.closeRecord('L');
  return true;
}

bool SDWParser::readSWTextZone(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='T' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InTxtNode
  libstoff::DebugStream f;
  f << "Entries(SWText)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  f << "nColl=" << input->readULong(2) << ",";
  int val;
  if (fl&0x10 && !zone.isCompatibleWith(0x201)) {
    val=(int) input->readULong(1);
    if (val==200 && zone.isCompatibleWith(0xf,0x101) && input->tell() < zone.getFlagLastPosition())
      val=(int) input->readULong(1);
    if (val)
      f << "nLevel=" << val << ",";
  }
  if (zone.isCompatibleWith(0x19,0x22, 0x101))
    f << "nCondColl=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  librevenge::RVNGString text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTextZone: can not read main text\n"));
    f << "###text";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord('T');
    return true;
  }
  else if (!text.empty())
    f << text.cstr();

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  SWAttributeManager attributeManager;
  SWFormatManager formatManager;
  while (input->tell()<lastPos) {
    pos=input->tell();

    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'A':
      done=attributeManager.readAttribute(zone, *this);
      break;
    case 'R':
      done=readSWNumRule(zone,'R');
      break;
    case 'S':
      done=attributeManager.readAttributeList(zone, *this);
      break;
    case 'l': // related to link
    case 'o': // format: safe to ignore
      done=formatManager.readSWFormatDef(zone,char(rType), *this);
      break;
    case 'v':
      done=readSWNodeRedline(zone);
      break;
    default:
      break;
    }
    if (done)
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWText[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case '3': {
      // sw_sw3num InNodeNum
      f << "nodeNum,";
      int cFlag=zone.openFlagZone();
      int nLevel=(int) input->readULong(1);
      if (nLevel!=201)
        f << "nLevel=" << input->readULong(1) << ",";
      if (cFlag&0x20) f << "nSetValue=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      if (nLevel!=201) {
        int N=int(lastPos-input->tell())/2;
        f << "level=[";
        for (int i=0; i<N; ++i)
          f << input->readULong(2) << ",";
        f << "],";
      }
      break;
    }
    case 'K':
      // sw_sw3misc.cxx InNodeMark
      f << "nodeMark,";
      f << "cType=" << input->readULong(1) << ",";
      f << "nId=" << input->readULong(2) << ",";
      f << "nOff=" << input->readULong(2) << ",";
      break;
    case 'w': { // wrong list
      // sw_sw3nodes.cxx in text node
      f << "wrongList,";
      int cFlag=zone.openFlagZone();
      if (cFlag&0xf0) f << "flag=" << (cFlag>>4) << ",";
      f << "nBeginInv=" << input->readULong(2) << ",";
      f << "nEndInc=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      int N =(int) input->readULong(2);
      if (input->tell()+4*N>lastPos) {
        STOFF_DEBUG_MSG(("SDWParser::readSWTextZone: find bad count\n"));
        f << "###N=" << N << ",";
        break;
      }
      f << "nWrong=[";
      for (int i=0; i<N; ++i) f << input->readULong(4) << ",";
      f << "],";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SDWParser::readSWTextZone: find unexpected type\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTextZone: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWText:###extra");
  }
  zone.closeRecord('T');
  return true;
}

bool SDWParser::readSWTOXList(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='u' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InTOXs
  libstoff::DebugStream f;
  f << "Entries(SWTOXList)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='x' || !zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    long lastRecordPos=zone.getRecordLastPosition();
    f << "SWTOXList:";
    int fl=zone.openFlagZone();
    if (fl&0xf0)
      f << "fl=" << (fl>>4) << ",";
    f << "nType=" << input->readULong(2) << ",";
    f << "nCreateType=" << input->readULong(2) << ",";
    f << "nCaptionDisplay=" << input->readULong(2) << ",";
    f << "nStrIdx=" << input->readULong(2) << ",";
    f << "nData=" << input->readULong(2) << ",";
    f << "cFormFlags=" << input->readULong(1) << ",";
    zone.closeFlagZone();
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWTOXList: can not read aName\n"));
      f << "###aName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord(type);
      continue;
    }
    if (!string.empty())
      f << "aName=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWTOXList: can not read aTitle\n"));
      f << "###aTitle";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord(type);
      continue;
    }
    if (!string.empty())
      f << "aTitle=" << string.cstr() << ",";
    if (zone.isCompatibleWith(0x215)) {
      f << "nOLEOptions=" << input->readULong(2) << ",";
      f << "nMainStyleIdx=" << input->readULong(2) << ",";
    }
    else {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWTOXList: can not read aDummy\n"));
        f << "###aDummy";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeRecord(type);
        continue;
      }
      if (!string.empty())
        f << "aDummy=" << string.cstr() << ",";
    }

    int N=(int) input->readULong(1);
    f << "nPatterns=" << N << ",";
    bool ok=true;
    SWFormatManager formatManager;
    for (int i=0; i<N; ++i) {
      if (formatManager.readSWPatternLCL(zone))
        continue;
      ok=false;
      f << "###pat";
      break;
    }
    if (!ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord(type);
      continue;
    }
    N=(int) input->readULong(1);
    f << "nTmpl=" << N << ",";
    f << "tmpl[strId]=[";
    for (int i=0; i<N; ++i)
      f << input->readULong(2) << ",";
    f << "],";
    N=(int) input->readULong(1);
    f << "nStyle=" << N << ",";
    f << "style=[";
    for (int i=0; i<N; ++i) {
      f << "[";
      f << "level=" << input->readULong(1) << ",";
      int nCount=(int) input->readULong(2);
      f << "nCount=" << nCount << ",";
      if (input->tell()+2*nCount>lastRecordPos) {
        STOFF_DEBUG_MSG(("SDWParser::readSWTOXList: can not read some string id\n"));
        f << "###styleId";
        ok=false;
        break;
      }
      librevenge::RVNGString poolName;
      for (int j=0; j<nCount; ++j) {
        int val=(int) input->readULong(2);
        if (!zone.getPoolName(val, poolName))
          f << "###nPoolId=" << val << ",";
        else
          f << poolName.cstr() << ",";
      }
      f << "],";
    }
    f << "],";
    if (!ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord(type);
      continue;
    }
    fl=zone.openFlagZone();
    f << "nSectStrIdx=" << input->readULong(2) << ",";
    if (fl&0x10) f << "nTitleLen=" << input->readULong(4) << ",";
    zone.closeFlagZone();

    if (input->tell()!=lastRecordPos) {
      STOFF_DEBUG_MSG(("SDWParser::readSWTOXList: find extra data\n"));
      f << "###extra,";
      ascFile.addDelimiter(input->tell(),'|');
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTOXList: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWTOXList:###extra");
  }
  zone.closeRecord('u');
  return true;
}

bool SDWParser::readSWTOX51List(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='y' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3misc.cxx: InTOX51s
  libstoff::DebugStream f;
  f << "Entries(SWTOX51List)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='x' || !zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f << "SWTOX51List:";
    if (zone.isCompatibleWith(0x201)) {
      int strId=(int) input->readULong(2);
      if (strId!=0xFFFF && !zone.getPoolName(strId, string))
        f << "###nPoolId=" << strId << ",";
      else if (strId!=0xFFFF && !string.empty())
        f << string.cstr() << ",";
    }
    else {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWTOX51List: can not read typeName\n"));
        f << "###typeName";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        zone.closeRecord(type);
        continue;
      }
      if (!string.empty())
        f << "typeName=" << string.cstr() << ",";
    }
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWTOX51List: can not read aTitle\n"));
      f << "###aTitle";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord(type);
      continue;
    }
    if (!string.empty())
      f << "aTitle=" << string.cstr() << ",";
    int fl=zone.openFlagZone();
    f << "nCreateType=" << input->readLong(2) << ",";
    f << "nType=" << input->readULong(1) << ",";
    if (zone.isCompatibleWith(0x213) && (fl&0x10))
      f << "firstTabPos=" << input->readULong(2) << ",";

    int N=(int) input->readULong(1);
    f << "nPat=" << N << ",";
    f << "pat=[";
    bool ok=true;
    for (int i=0; i<N; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWTOX51List: can not read a pattern name\n"));
        f << "###pat";
        ok=false;
        break;
      }
      if (!string.empty())
        f << string.cstr() << ",";
    }
    f << "],";
    if (!ok) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord(type);
      continue;
    }
    N=(int) input->readULong(1);
    f << "nTmpl=" << N << ",";
    f << "tmpl[strId]=[";
    for (int i=0; i<N; ++i)
      f << input->readULong(2) << ",";
    f << "],";

    f << "nInf=" << input->readULong(2) << ",";
    if (input->tell()!=zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("SDWParser::readSWTOX51List: find extra data\n"));
      f << "###extra,";
      ascFile.addDelimiter(input->tell(),'|');
    }

    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }
  if (input->tell()<lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWTOX51List: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWTOX51List:###extra");
  }
  zone.closeRecord('y');
  return true;
}

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool SDWParser::readWriterDocument(STOFFInputStreamPtr input, std::string const &name)
{
  SWZone zone(input, name, "SWWriterDocument");
  if (!zone.readHeader()) {
    STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: can not read the header\n"));
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  // sw_sw3doc.cxx Sw3IoImp::LoadDocContents
  SWFieldManager fieldManager;
  SWFormatManager formatManager;
  while (!input->isEnd()) {
    long pos=input->tell();
    int rType=input->peek();
    bool done=false;
    switch (rType) {
    case '!':
      done=zone.readStringsPool();
      break;
    case 'R':
    case '0': // Outline
      done=readSWNumRule(zone,char(rType));
      break;
    case '1':
      done=readSWFootNoteInfo(zone);
      break;
    case '4':
      done=readSWEndNoteInfo(zone);
      break;
    case 'D':
      done=readSWDBName(zone);
      break;
    case 'F':
      done=formatManager.readSWFlyFrameList(zone, *this);
      break;
    case 'J':
      done=readSWJobSetUp(zone);
      break;
    case 'M':
      done=readSWMacroTable(zone);
      break;
    case 'N':
      done=readSWContent(zone);
      break;
    case 'U': // layout info, no code, ignored by LibreOffice
      done=readSWLayoutInfo(zone);
      break;
    case 'V':
      done=readSWRedlineList(zone);
      break;
    case 'Y':
      done=fieldManager.readField(zone,'Y');
      break;

    case 'a':
      done=readSWBookmarkList(zone);
      break;
    case 'j':
      done=readSWDictionary(zone);
      break;
    case 'q':
      done=formatManager.readSWNumberFormatterList(zone);
      break;
    case 'u':
      done=readSWTOXList(zone);
      break;
    case 'y':
      done=readSWTOX51List(zone);
      break;
    default:
      break;
    }
    if (done)
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    char type;
    if (!zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWWriterDocument[" << type << "]:";
    long lastPos=zone.getRecordLastPosition();
    bool endZone=false;
    librevenge::RVNGString string;
    switch (type) {
    case '$': // unknown, seems to store an object name
      f << "dollarZone,";
      if (input->tell()+7>lastPos) {
        STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: zone seems to short\n"));
        break;
      }
      for (int i=0; i<5; ++i) { // f0=f1=1
        int val=(int) input->readULong(1);
        if (val) f << "f" << i << "=" << val << ",";
      }
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: can not read main string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << string.cstr();
      break;
    case '5':
      // sw_sw3misc.cxx InLineNumberInfo
      f << "linenumberInfo=" << input->readULong(1) << ",";
      f << "nPos=" << input->readULong(1) << ",";
      f << "nChrIdx=" << input->readULong(2) << ",";
      f << "nPosFromLeft=" << input->readULong(2) << ",";
      f << "nCountBy=" << input->readULong(2) << ",";
      f << "nDividerCountBy=" << input->readULong(2) << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: can not read sDivider string\n"));
        f << "###sDivider";
        break;
      }
      else if (!string.empty())
        f << string.cstr();
      break;
    case '6':
      // sw_sw3misc.cxx InDocDummies
      f << "docDummies,";
      f << "n1=" << input->readULong(4) << ",";
      f << "n2=" << input->readULong(4) << ",";
      f << "n3=" << input->readULong(1) << ",";
      f << "n4=" << input->readULong(1) << ",";
      for (int i=0; i<2; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: can not read a string\n"));
          f << "###string";
          break;
        }
        else if (!string.empty())
          f << (i==0 ? "sAutoMarkURL" : "s2") << "=" << string.cstr() << ",";
      }
      break;
    case '7': { // config, ignored by LibreOffice, and find no code
      f << "config,";
      int fl=(int) zone.openFlagZone();
      if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
      f << "f0=" << input->readULong(1) << ","; // 1
      for (int i=0; i<5; ++i) // e,1,5,1,5
        f << "f" << i+1 << "=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      break;
    }
    case '8':
      // sw_sw3misc.cxx InPagePreviewPrintData
      f << "pagePreviewPrintData,";
      f << "cFlags=" << input->readULong(1) << ",";
      f << "nRow=" << input->readULong(2) << ",";
      f << "nCol=" << input->readULong(2) << ",";
      f << "nLeftSpace=" << input->readULong(2) << ",";
      f << "nRightSpace=" << input->readULong(2) << ",";
      f << "nTopSpace=" << input->readULong(2) << ",";
      f << "nBottomSpace=" << input->readULong(2) << ",";
      f << "nHorzSpace=" << input->readULong(2) << ",";
      f << "nVertSpace=" << input->readULong(2) << ",";
      break;
    case 'd':
      // sw_sw3misc.cxx: InDocStat
      f << "docStats,";
      f << "nTbl=" << input->readULong(2) << ",";
      f << "nGraf=" << input->readULong(2) << ",";
      f << "nOLE=" << input->readULong(2) << ",";
      if (zone.isCompatibleWith(0x201)) {
        f << "nPage=" << input->readULong(4) << ",";
        f << "nPara=" << input->readULong(4) << ",";
      }
      else {
        f << "nPage=" << input->readULong(2) << ",";
        f << "nPara=" << input->readULong(2) << ",";
      }
      f << "nWord=" << input->readULong(4) << ",";
      f << "nChar=" << input->readULong(4) << ",";
      break;
    case 'C': { // ignored by LibreOffice
      std::string comment("");
      while (lastPos && input->tell()<lastPos) comment+=(char) input->readULong(1);
      f << "comment=" << comment << ",";
      break;
    }
    case 'P': // password
      // sw_sw3misc.cxx: InPasswd
      f << "passwd,";
      if (zone.isCompatibleWith(0x6)) {
        f << "cType=" << input->readULong(1) << ",";
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: can not read passwd string\n"));
          f << "###passwd";
        }
        else
          f << "cryptedPasswd=" << string.cstr() << ",";
      }
      break;
    case 'Z':
      endZone=true;
      break;
    default:
      STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: find unexpected type\n"));
      f << "###type,";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
    if (endZone)
      break;
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWWriterDocument:##extra");
  }
  return true;
}
////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool SDWParser::checkHeader(STOFFHeader *header, bool /*strict*/)
{
  *m_state = SDWParserInternal::State();

  STOFFInputStreamPtr input = getInput();
  if (!input || !input->hasDataFork() || !input->isStructured())
    return false;

  if (header)
    header->reset(1);

  return true;
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
