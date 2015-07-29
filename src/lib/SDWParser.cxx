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

    if (!ok) {
      libstoff::DebugStream f;
      if (base=="Star Framework Config File")
        f << "Entries(StarFrameworkConfigFile):";
      else if (base=="Standard") // can be Standard or STANDARD
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
    if (pos+10+37>endDataPos || !input->readCompressedLong(id)) {
      STOFF_DEBUG_MSG(("SDWParser::readPersists: data %d seems bad\n", i));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    if (id!=i+1) f << "id=" << id << ",";
    for (int j=0; j<2; ++j) { // f1=[2f|30|33]82 : another compressed long + ?
      val=(int) input->readULong(2);
      if (val) f << "f" << j+1 << "=" << std::hex << val << std::dec << ",";
    }
    val=(int) input->readULong(1); // always 0
    if (val) f << "f3=" << val << ",";
    int decalSz=(int) input->readULong(1);
    int textSz=(int) input->readULong(2);
    bool oddHeader=false;
    if (textSz==0) {
      f << "#odd,";
      oddHeader=true;
      textSz=(int) input->readULong(2);
    }
    long endZPos=input->tell()+textSz+decalSz+36+(oddHeader ? -2 : 0);
    if (endZPos>endDataPos) {
      STOFF_DEBUG_MSG(("SDWParser::readPersists: data %d seems bad\n", i));
      f << "###textSz=" << textSz << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
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
  readSWNumberFormatterList(zone);

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
  if ((zone.isCompatibleWith(0x16) && !zone.isCompatibleWith(0x22)) || zone.isCompatibleWith(0x101)) {
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
  // sw_sw3sectn.cxx: InContent
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
  for (int i=0; i<nNodes; ++i) {
    if (input->tell()>=lastPos) break;
    pos=input->tell();
    int cType=input->peek();
    bool done=false;
    switch (cType) {
    case 'T':
      done=readSWTextZone(zone);
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
    STOFF_DEBUG_MSG(("SDWParser::readSWContent: find unexpected type\n"));
    f << "###";
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

bool SDWParser::readSWFormatDef(SWZone &zone, char kind)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!=kind || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3fmts.cxx inFormat
  libstoff::DebugStream f;
  f << "Entries(SWFormatDef)[" << zone.getRecordLevel() << "]:";
  int flags=zone.openFlagZone();
  f << "nDerived=" << input->readULong(2) << ",";
  f << "nPoolId=" << input->readULong(2) << ",";
  int stringId=0xFFFF;
  if (flags&0x10) {
    stringId=(int) input->readULong(2);
    if (stringId!=0xffff)
      f << "nStringId=" << stringId << ",";
  }
  if (flags&0x20)
    f << "nObjRef=" << input->readULong(4) << ",";
  int moreFlags=0;
  if (flags&(zone.isCompatibleWith(0x0201) ? 0x80:0x40)) {
    moreFlags=(int) input->readULong(1);
    f << "flags[more]=" << std::hex << moreFlags << std::dec << ",";
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  bool readName;
  if (zone.isCompatibleWith(0x0201))
    readName=(moreFlags&0x20);
  else
    readName=(stringId==0xffff);
  librevenge::RVNGString string;
  if (readName) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWFormatDef: can not read the name\n"));
      f << "###name";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeRecord(kind);
      return true;
    }
    else if (!string.empty())
      f << string.cstr() << ",";
  }
  else if (!zone.getPoolName(stringId, string))
    f << "###nPoolId=" << stringId << ",";
  else if (!string.empty())
    f << string.cstr() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  SWAttributeManager attributeManager;
  while (input->tell()<lastPos) {
    pos=(int) input->tell();
    int rType=input->peek();
    if (rType=='S' && attributeManager.readAttributeList(zone, *this))
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWFormatDef: find extra data\n"));
      ascFile.addPos(pos);
      ascFile.addNote("SWFormatDef:###");
      break;
    }
    f.str("");
    f << "SWFormatDef[" << type << "-" << zone.getRecordLevel() << "]:";
    STOFF_DEBUG_MSG(("SDWParser::readSwFormatDef: find unexpected type\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }

  zone.closeRecord(kind);
  return true;
}

bool SDWParser::readSWNumberFormat(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='n' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWNumbFormat)[" << zone.getRecordLevel() << "]:";
  // sw_sw3num.cxx Sw3IoImp::InNumFmt
  librevenge::RVNGString string;
  for (int i=0; i<4; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormat: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeRecord('n');
      return true;
    }
    if (string.empty()) continue;
    static char const *(wh[])= {"prefix", "postfix", "fontname", "fontstyle"};
    f << wh[i] << "=" << string.cstr() << ",";
  }
  f << "nFmt=" << input->readULong(2) << ",";
  f << "eType=" << input->readULong(1) << ",";
  f << "cBullet=" << input->readULong(1) << ",";
  f << "nUpperLevel=" << input->readULong(1) << ","; // int or bool
  f << "nStart=" << input->readULong(2) << ",";
  f << "eNumAdjust=" << input->readULong(1) << ",";
  f << "nLSpace=" << input->readLong(4) << ",";
  f << "nFirstLineOffset=" << input->readLong(4) << ",";
  f << "eFamily=" << input->readULong(1) << ",";
  f << "ePitch=" << input->readULong(1) << ",";
  f << "eCharSet=" << input->readULong(1) << ",";
  int cFlags=zone.openFlagZone();
  zone.closeFlagZone();
  if (cFlags&0xF0) f << "cFlags=" << (cFlags>>4) << ",";
  if (zone.isCompatibleWith(0x17) && !zone.isCompatibleWith(0x22))
    f << "nFmt=" << input->readULong(2) << ",";
  if ((zone.isCompatibleWith(0x17) && !zone.isCompatibleWith(0x22)) ||
      (zone.isCompatibleWith(0x101) && !zone.isCompatibleWith(0x201))) {
    f << "bRelSpace=" << input->readULong(1) << ",";
    f << "nRelLSpace=" << input->readLong(4) << ",";
  }
  if ((zone.isCompatibleWith(0x17) && !zone.isCompatibleWith(0x22)) ||
      zone.isCompatibleWith(0x101)) {
    f << "nTextOffset=" << input->readULong(2) << ",";
    if (input->tell()<zone.getRecordLastPosition()) {
      f << "width=" << input->readLong(4) << ",";
      f << "height=" << input->readLong(4) << ",";
      int cF=(int) input->readULong(1);
      if (cF&1) f << "nVer[brush]=" << input->readULong(2) << ",";
      if (cF&2) f << "nVer[vertOrient]=" << input->readULong(2) << ",";
    }
  }
  if (input->tell()!=zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SDWParser::readSwNumberFormat: find extra data\n"));
    f << "###extra:";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeRecord('n');
  return true;
}

bool SDWParser::readSWNumberFormatterList(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='q' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWNumberFormatter)[" << zone.getRecordLevel() << "]:";
  long dataSz=(int) input->readULong(4);
  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+dataSz+6>lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: data size seems bad\n"));
    f << "###dataSz";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  // svt_numhead::ImpSvNumMultipleReadHeader
  long actPos=input->tell();
  long endDataPos=input->tell()+dataSz;
  f.str("");
  f << "SWNumberFormatter-T:";
  input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
  int val=(int) input->readULong(2);
  if (val) f << "nId=" << val << ",";
  long nSizeTableLen=(long) input->readULong(4);
  f << "nTableSize=" << nSizeTableLen << ","; // copy in pMemStream
  std::vector<long> fieldSize;
  if (input->tell()+nSizeTableLen>lastPos) {
    STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: the table length seems bad\n"));
    f << "###";
    nSizeTableLen=lastPos-input->tell();
  }
  f << "listSize=[";
  for (long i=0; i<nSizeTableLen/4; ++i) {
    fieldSize.push_back((long) input->readULong(4));
    f << std::hex << fieldSize.back() << std::dec << ",";
  }
  f << "],";
  if (nSizeTableLen) ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(endDataPos);
  ascFile.addNote(f.str().c_str());

  // svt_zforList::Load
  input->seek(actPos, librevenge::RVNG_SEEK_SET);
  f.str("");
  f << "SWNumberFormatter:";
  int nVers=(int) input->readULong(2);
  f << "vers=" << nVers << ",";
  val=(int) input->readULong(2);
  if (val) f << "nSysOnStore=" << std::hex << val << std::dec << ",";
  val=(int) input->readULong(2);
  if (val) f << "eLge=" << val << ",";
  ascFile.addPos(actPos);
  ascFile.addNote(f.str().c_str());

  unsigned long nPos=(unsigned long) input->readULong(4);
  size_t n=0;
  while (nPos!=0xffffffff) {
    pos=input->tell();
    if (pos==endDataPos) break;
    if (pos>endDataPos) {
      STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: data size seems bad\n"));
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }

    f.str("");
    f << "SWNumberFormatter-A" << n << ":";

    input->seek(2, librevenge::RVNG_SEEK_CUR);
    val=(int) input->readULong(2);
    if (val) f << "eLge=" << val << ",";

    if (n>=fieldSize.size()||input->tell()+fieldSize[n]>endDataPos) {
      STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: can not find end of field\n"));
      f << "###unknownN";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endFieldPos=input->tell()+fieldSize[n++];

    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: can not read the format string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    f << text.cstr() << ",";
    val=(int) input->readULong(2);
    if (val) f << "eType=" << val << ",";
    for (int i=0; i<2; ++i) { // checkme
      double res;
      bool isNan;
      if (!input->readDouble8(res, isNan)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: can not read a double\n"));
        f << "##limit" << i << ",";
      }
      else if (res<0||res>0)
        f << "limit" << i << "=" << res << ",";
    }
    for (int i=0; i<2; ++i) {
      val=(int) input->readULong(2);
      if (val) f << "nOp" << i << "=" << val << ",";
    }
    val=(int) input->readULong(1);
    if (val) f << "bStandart=" << val << ",";
    val=(int) input->readULong(1);
    if (val) f << "bIsUsed=" << val << ",";

    bool ok=true;
    for (int i=0; i<4; ++i) {
      // ImpSvNumFor::Load
      f << "numFor" << i << "=[";
      int N=(int) input->readULong(2);
      if (input->tell()+4*N>endFieldPos) break;
      for (int c=0; c<N; ++c) {
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: can not read the format string\n"));
          f << "###SvNumFor" << c;
          ok=false;
          break;
        }
        f << text.cstr() << ":" << input->readULong(2) << ",";
      }
      if (!ok) break;
      val=(int) input->readLong(2);
      if (val) f << "eScannedType=" << val << ",";
      val=(int) input->readULong(1);
      if (val) f << "bThousand=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nThousand=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nCntPre=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nCntPost=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nCntExp=" << val << ",";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: can not read the color name\n"));
        f << "###colorname";
        ok=false;
        break;
      }
      if (!text.empty())
        f << text.cstr() << ",";
      f << "],";
    }
    if (input->tell()>endFieldPos) {
      STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: we read too much\n"));
      f << "###pos";
      ok=false;
    }
    if (ok && input->tell()!=endFieldPos && !zone.readString(text)) {
      STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormatterList: can not read the comment\n"));
      f << "###comment";
      ok=false;
    }
    else {
      if (!text.empty())
        f << "comment=" << text.cstr() << ",";
    }

    if (ok && input->tell()!=endFieldPos) {
      val=(int) input->readULong(2);
      if (val) f << "nNewStandardDefined=" << val << ",";
    }

    if (input->tell()!=endFieldPos) {
      // now there can still be a list of currency version....
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("SDWParser::readSWNumberFormat: find extra data\n"));
        first=false;
      }
      ascFile.addDelimiter(input->tell(),'|');
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (endFieldPos==endDataPos) break;
    input->seek(endFieldPos, librevenge::RVNG_SEEK_SET);
    nPos=(unsigned long) input->readULong(4);
  }
  zone.closeRecord(type);
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
  if (zone.isCompatibleWith(0x0201)) {
    cFlags=(int) zone.openFlagZone();
    val=(int) input->readULong(2);
    if (val) f << "nStringId=" << val << ",";
    if (cFlags&0x10) {
      nPoolId=(int) input->readULong(2);
      librevenge::RVNGString poolName;
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
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
  if (zone.isCompatibleWith(0x0201))
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
  for (int i=0; i<nKnownFormat; ++i) {
    pos=input->tell();
    if (readSWNumberFormat(zone)) continue;
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
    if (val==200 && zone.isCompatibleWith(0xf) && !zone.isCompatibleWith(0x101) &&
        input->tell() < zone.getFlagLastPosition())
      val=(int) input->readULong(1);
    if (val)
      f << "nLevel=" << val << ",";
  }
  if ((zone.isCompatibleWith(0x19) && !zone.isCompatibleWith(0x22))||zone.isCompatibleWith(0x101))
    f << "nCondColl=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  librevenge::RVNGString text;
  if (!zone.readString(text)) {
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
  while (input->tell()<lastPos) {
    pos=input->tell();

    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'A':
      done=attributeManager.readAttribute(zone, *this);
      break;
    // case 'K': done=readNodeMark(zone); break;
    case 'R':
      done=readSWNumRule(zone,'R');
      break;
    case 'S':
      done=attributeManager.readAttributeList(zone, *this);
      break;
    case 'l': // related to link
    case 'o': // format: safe to ignore
      done=readSWFormatDef(zone,char(rType));
      break;
    // case 'v': done=readSWNodeRedLine(zone); break;
    // case '3': done=readSWNodeNum(zone); break;
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
    f << "SWText[" << type << "]:";
    // todo 'w' wrong type here
    STOFF_DEBUG_MSG(("SDWParser::readSWTextZone: find unexpected type\n"));
    f << "###";
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

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool SDWParser::readWriterDocument(STOFFInputStreamPtr input, std::string const &name)
{
  SWZone zone(input, name, "WriterDocument");
  if (!zone.readHeader()) {
    STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: can not read the header\n"));
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  while (!input->isEnd()) {
    long pos=input->tell();
    int rType=input->peek();
    if (rType=='N' && readSWContent(zone)) continue;
    if (rType=='!' && zone.readStringsPool()) continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    char type;
    if (!zone.openRecord(type) || !zone.closeRecord(type)) {
      STOFF_DEBUG_MSG(("SDWParser::readWriterDocument: find extra data\n"));
      ascFile.addPos(pos);
      ascFile.addNote("WriterDocument:##extra");
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "WriterDocument[" << type << "]:";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
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
