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

#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"
#include "StarAttribute.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectDraw.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarObjectText.hxx"
#include "StarZone.hxx"
#include "STOFFSpreadsheetListener.hxx"

#include "SDCParser.hxx"

/** Internal: the structures of a SDCParser */
namespace SDCParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDCParser
struct State {
  //! constructor
  State() : m_actPage(0), m_numPages(0), m_mainSpreadsheet()
  {
  }

  int m_actPage /** the actual page */, m_numPages /** the number of page of the final document */;
  shared_ptr<StarObjectSpreadsheet> m_mainSpreadsheet;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDCParser::SDCParser(STOFFInputStreamPtr input, STOFFHeader *header) :
  STOFFSpreadsheetParser(input, header), m_password(0), m_oleParser(), m_state(new SDCParserInternal::State)
{
}

SDCParser::~SDCParser()
{
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
void SDCParser::parse(librevenge::RVNGSpreadsheetInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(0L))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    checkHeader(0L);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      sendSpreadsheet();
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDCParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  resetSpreadsheetListener();
  if (!ok) throw(libstoff::ParseException());
}


bool SDCParser::createZones()
{
  m_oleParser.reset(new STOFFOLEParser);
  m_oleParser->parse(getInput());

  // send the final data
  std::vector<shared_ptr<STOFFOLEParser::OleDirectory> > listDir=m_oleParser->getDirectoryList();
  for (size_t d=0; d<listDir.size(); ++d) {
    if (!listDir[d]) continue;
    StarObject object(m_password, listDir[d]);
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
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_SPREADSHEET) {
      shared_ptr<StarObjectSpreadsheet> spreadsheet(new StarObjectSpreadsheet(object, false));
      spreadsheet->parse();
      if (listDir[d]->m_dir.empty())
        m_state->m_mainSpreadsheet=spreadsheet;
      continue;
    }
    if (object.getDocumentKind()==STOFFDocument::STOFF_K_TEXT) {
      StarObjectText text(object, false);
      text.parse();
      continue;
    }
    // Ole-Object has persist elements, so...
    if (listDir[d]->m_hasCompObj) object.parse();
    STOFFOLEParser::OleDirectory &direct=*listDir[d];
    std::vector<std::string> unparsedOLEs=direct.getUnparsedOles();
    size_t numUnparsed = unparsedOLEs.size();
    StarFileManager fileManager;
    for (size_t i = 0; i < numUnparsed; i++) {
      std::string const &name = unparsedOLEs[i];
      STOFFInputStreamPtr ole = getInput()->getSubStreamByName(name.c_str());
      if (!ole.get()) {
        STOFF_DEBUG_MSG(("SDCParser::createZones: error: can not find OLE part: \"%s\"\n", name.c_str()));
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
      if (base=="SfxStyleSheets") {
        object.readSfxStyleSheets(ole,name);
        continue;
      }

      if (base=="StarImageDocument" || base=="StarImageDocument 4.0") {
        librevenge::RVNGBinaryData data;
        fileManager.readImageDocument(ole,data,name);
        continue;
      }
      if (base=="StarMathDocument") {
        fileManager.readMathDocument(ole,name,object);
        continue;
      }
      if (base.compare(0,3,"Pic")==0) {
        librevenge::RVNGBinaryData data;
        std::string type;
        fileManager.readEmbeddedPicture(ole,data,type,name,object);
        continue;
      }
      // other
      if (base=="Ole-Object") {
        fileManager.readOleObject(ole,name);
        continue;
      }
      libstoff::DebugFile asciiFile(ole);
      asciiFile.open(name);

      bool ok=false;
      if (base=="OutPlace Object")
        ok=fileManager.readOutPlaceObject(ole, asciiFile);
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
  return m_state->m_mainSpreadsheet;
}

////////////////////////////////////////////////////////////
// create the document and send data
////////////////////////////////////////////////////////////
void SDCParser::createDocument(librevenge::RVNGSpreadsheetInterface *documentInterface)
{
  if (!documentInterface) return;
  m_state->m_numPages = 1;
  STOFFPageSpan ps(getPageSpan());
  ps.setPageSpan(1);
  std::vector<STOFFPageSpan> pageList(1,ps);
  //
  STOFFSpreadsheetListenerPtr listen(new STOFFSpreadsheetListener(*getParserState(), pageList, documentInterface));
  setSpreadsheetListener(listen);
  listen->startDocument();
}

bool SDCParser::sendSpreadsheet()
{
  STOFFSpreadsheetListenerPtr listener=getSpreadsheetListener();
  if (!listener || !m_state->m_mainSpreadsheet) {
    STOFF_DEBUG_MSG(("SDCParser::sendSpreadsheet: can not find the main spreadsheet\n"));
    return false;
  }
  return m_state->m_mainSpreadsheet->send(listener);
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool SDCParser::checkHeader(STOFFHeader *header, bool /*strict*/)
{
  *m_state = SDCParserInternal::State();

  STOFFInputStreamPtr input = getInput();
  if (!input || !input->hasDataFork() || !input->isStructured())
    return false;
  STOFFInputStreamPtr calcInput=input->getSubStreamByName("StarCalcDocument");
  if (!calcInput)
    return false;
  if (header) {
    header->reset(1, STOFFDocument::STOFF_K_SPREADSHEET);
    calcInput->seek(1, librevenge::RVNG_SEEK_SET);
    header->setEncrypted(input->readULong(1)!=0x42);
  }
  return true;
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
