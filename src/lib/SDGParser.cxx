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

#include "SWFieldManager.hxx"
#include "StarAttribute.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectDraw.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarObjectText.hxx"
#include "StarZone.hxx"

#include "SDGParser.hxx"

/** Internal: the structures of a SDGParser */
namespace SDGParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDGParser
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
SDGParser::SDGParser(STOFFInputStreamPtr input, STOFFHeader *header) :
  STOFFGraphicParser(input, header), m_password(0), m_state(new SDGParserInternal::State)
{
}

SDGParser::~SDGParser()
{
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
void SDGParser::parse(librevenge::RVNGDrawingInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(0L))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    ascii().setStream(getInput());
    ascii().open("main-1");

    checkHeader(0L);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      // todo
      ok=false;
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDGParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  resetGraphicListener();
  if (!ok) throw(libstoff::ParseException());
}


bool SDGParser::createZones()
{
  STOFFInputStreamPtr input=getInput();
  if (!input)
    return false;
  STOFF_DEBUG_MSG(("SDGParser::createZones: sorry, not implemented\n"));
  return false;
}

////////////////////////////////////////////////////////////
// create the document and send data
////////////////////////////////////////////////////////////
void SDGParser::createDocument(librevenge::RVNGDrawingInterface *documentInterface)
{
  if (!documentInterface) return;

  std::vector<STOFFPageSpan> pageList;
  STOFFPageSpan ps(getPageSpan());
  ps.m_pageSpan=1;
  pageList.push_back(ps);
  m_state->m_numPages = 1;
  STOFFGraphicListenerPtr listen(new STOFFGraphicListener(*getParserState(), pageList, documentInterface));
  setGraphicListener(listen);

  listen->startDocument();
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
bool SDGParser::checkHeader(STOFFHeader *header, bool /*strict*/)
{
  *m_state = SDGParserInternal::State();

  STOFFInputStreamPtr input = getInput();
  if (!input || !input->hasDataFork() || input->isStructured())
    return false;
  input->seek(0, librevenge::RVNG_SEEK_SET);
  if (input->readULong(4)!=0x53474133) // SGA3
    return false;
  ascii().addPos(0);
  ascii().addNote("Entries(SGA3):");
  input->seek(0, librevenge::RVNG_SEEK_SET);
  if (header)
    header->reset(1, STOFFDocument::STOFF_K_GRAPHIC);
#ifdef DEBUG
  return true;
#else
  return false;
#endif
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
