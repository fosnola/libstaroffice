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
#include "STOFFSpreadsheetListener.hxx"

#include "StarFileManager.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarZone.hxx"

#include "SDCParser.hxx"

/** Internal: the structures of a SDCParser */
namespace SDCParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDCParser
struct State {
  //! constructor
  State()
    : m_actPage(0)
    , m_numPages(0)
    , m_mainSpreadsheet()
  {
  }

  int m_actPage /** the actual page */, m_numPages /** the number of page of the final document */;
  std::shared_ptr<StarObjectSpreadsheet> m_mainSpreadsheet;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDCParser::SDCParser(STOFFInputStreamPtr &input, STOFFHeader *header)
  : STOFFSpreadsheetParser(input, header)
  , m_password(nullptr)
  , m_oleParser()
  , m_state(new SDCParserInternal::State)
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
  if (!getInput().get() || !checkHeader(nullptr))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    checkHeader(nullptr);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      sendSpreadsheet();
#ifdef DEBUG
      StarFileManager::checkUnparsed(getInput(), m_oleParser, m_password);
#endif
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

  auto mainOle=m_oleParser->getDirectory("/");
  if (!mainOle) {
    STOFF_DEBUG_MSG(("SDCParser::createZones: can not find the main ole\n"));
    return false;
  }
  mainOle->m_parsed=true;
  StarObject mainObject(m_password, m_oleParser, mainOle);
  if (mainObject.getDocumentKind()!=STOFFDocument::STOFF_K_SPREADSHEET) {
    STOFF_DEBUG_MSG(("SDCParser::createZones: can not find the main spreadsheet\n"));
    return false;
  }
  m_state->m_mainSpreadsheet.reset(new StarObjectSpreadsheet(mainObject, false));
  m_state->m_mainSpreadsheet->parse();
  return true;
}

////////////////////////////////////////////////////////////
// create the document and send data
////////////////////////////////////////////////////////////
void SDCParser::createDocument(librevenge::RVNGSpreadsheetInterface *documentInterface)
{
  if (!documentInterface) return;
  std::vector<STOFFPageSpan> pageList;
  if (!m_state->m_mainSpreadsheet || !m_state->m_mainSpreadsheet->updatePageSpans(pageList, m_state->m_numPages)) {
    STOFFPageSpan ps(getPageSpan());
    ps.m_pageSpan=1;
    pageList.push_back(ps);
    m_state->m_numPages = 1;
  }
  //
  STOFFSpreadsheetListenerPtr listen(new STOFFSpreadsheetListener(getParserState()->m_listManager, pageList, documentInterface));
  setSpreadsheetListener(listen);
  if (m_state->m_mainSpreadsheet)
    listen->setDocumentMetaData(m_state->m_mainSpreadsheet->getMetaData());
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
