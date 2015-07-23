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
    STOFFOLEParser oleParser("NIMP");
    oleParser.parse(getInput());
    // create the asciiFile
    checkHeader(0L);
    ok = false;
    ascii().addPos(getInput()->tell());
    ascii().addNote("_");
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
