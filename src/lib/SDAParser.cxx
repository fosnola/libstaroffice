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

#include "STOFFGraphicListener.hxx"
#include "STOFFOLEParser.hxx"

#include "StarFileManager.hxx"
#include "StarObjectDraw.hxx"
#include "StarZone.hxx"

#include "SDAParser.hxx"

/** Internal: the structures of a SDAParser */
namespace SDAParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDAParser
struct State {
  //! constructor
  State()
    : m_actPage(0)
    , m_numPages(0)
    , m_mainGraphic()
  {
  }

  int m_actPage /** the actual page */, m_numPages /** the number of page of the final document */;
  std::shared_ptr<StarObjectDraw> m_mainGraphic;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDAParser::SDAParser(STOFFInputStreamPtr &input, STOFFHeader *header)
  : STOFFGraphicParser(input, header)
  , m_password(nullptr)
  , m_oleParser()
  , m_state(new SDAParserInternal::State)
{
}

SDAParser::~SDAParser()
{
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
void SDAParser::parse(librevenge::RVNGDrawingInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(nullptr))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    checkHeader(nullptr);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      if (m_state->m_mainGraphic)
        m_state->m_mainGraphic->sendPages(getGraphicListener());
#ifdef DEBUG
      StarFileManager::checkUnparsed(getInput(), m_oleParser, m_password);
#endif
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDAParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  resetGraphicListener();
  if (!ok) throw(libstoff::ParseException());
}

void SDAParser::parse(librevenge::RVNGPresentationInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(nullptr))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    checkHeader(nullptr);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      if (m_state->m_mainGraphic)
        m_state->m_mainGraphic->sendPages(getGraphicListener());
#ifdef DEBUG
      StarFileManager::checkUnparsed(getInput(), m_oleParser, m_password);
#endif
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDAParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  resetGraphicListener();
  if (!ok) throw(libstoff::ParseException());
}

bool SDAParser::createZones()
{
  m_oleParser.reset(new STOFFOLEParser);
  m_oleParser->parse(getInput());

  auto mainOle=m_oleParser->getDirectory("/");
  if (!mainOle) {
    STOFF_DEBUG_MSG(("SDAParser::parse: can not find the main ole\n"));
    return false;
  }
  mainOle->m_parsed=true;
  StarObject mainObject(m_password, m_oleParser, mainOle);
  if (mainObject.getDocumentKind()!=STOFFDocument::STOFF_K_DRAW) {
    STOFF_DEBUG_MSG(("SDAParser::createZones: can not find the main graphic\n"));
    return false;
  }
  m_state->m_mainGraphic.reset(new StarObjectDraw(mainObject, false));
  return m_state->m_mainGraphic->parse();
}

////////////////////////////////////////////////////////////
// create the document and send data
////////////////////////////////////////////////////////////
void SDAParser::createDocument(librevenge::RVNGDrawingInterface *documentInterface)
{
  if (!documentInterface) return;

  std::vector<STOFFPageSpan> pageList;
  if (!m_state->m_mainGraphic || !m_state->m_mainGraphic->updatePageSpans(pageList, m_state->m_numPages)) {
    STOFFPageSpan ps(getPageSpan());
    ps.m_pageSpan=1;
    pageList.push_back(ps);
    m_state->m_numPages = 1;
  }
  STOFFGraphicListenerPtr listen(new STOFFGraphicListener(getParserState()->m_listManager, pageList, documentInterface));
  setGraphicListener(listen);
  if (m_state->m_mainGraphic)
    listen->setDocumentMetaData(m_state->m_mainGraphic->getMetaData());

  listen->startDocument();
  if (m_state->m_mainGraphic)
    m_state->m_mainGraphic->sendMasterPages(listen);
}

void SDAParser::createDocument(librevenge::RVNGPresentationInterface *documentInterface)
{
  if (!documentInterface) return;

  std::vector<STOFFPageSpan> pageList;
  if (!m_state->m_mainGraphic || !m_state->m_mainGraphic->updatePageSpans(pageList, m_state->m_numPages)) {
    STOFFPageSpan ps(getPageSpan());
    ps.m_pageSpan=1;
    pageList.push_back(ps);
    m_state->m_numPages = 1;
  }
  STOFFGraphicListenerPtr listen(new STOFFGraphicListener(getParserState()->m_listManager, pageList, documentInterface));
  setGraphicListener(listen);
  if (m_state->m_mainGraphic)
    listen->setDocumentMetaData(m_state->m_mainGraphic->getMetaData());

  listen->startDocument();
  if (m_state->m_mainGraphic)
    m_state->m_mainGraphic->sendMasterPages(listen);
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
bool SDAParser::checkHeader(STOFFHeader *header, bool /*strict*/)
{
  *m_state = SDAParserInternal::State();

  STOFFInputStreamPtr input = getInput();
  if (!input || !input->hasDataFork() || !input->isStructured())
    return false;
  bool isPres=false;
  STOFFInputStreamPtr drawInput=input->getSubStreamByName("StarDrawDocument");
  if (!drawInput) {
    drawInput=input->getSubStreamByName("StarDrawDocument3");
    if (!drawInput)
      return false;
    STOFFOLEParser oleParser;
    std::string clipName;
    isPres=oleParser.getCompObjName(input, clipName) && clipName.substr(0,11)=="StarImpress";
  }
  if (header) {
    header->reset(1, isPres ? STOFFDocument::STOFF_K_PRESENTATION : STOFFDocument::STOFF_K_DRAW);
    drawInput->seek(0, librevenge::RVNG_SEEK_SET);
    header->setEncrypted(drawInput->readULong(2)!=0x7244);
  }
  return true;
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
