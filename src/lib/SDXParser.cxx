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
#include "SWFieldManager.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectDraw.hxx"
#include "StarObjectMath.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarObjectText.hxx"
#include "StarZone.hxx"

#include "SDXParser.hxx"

/** Internal: the structures of a SDXParser */
namespace SDXParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDXParser
struct State {
  //! constructor
  State()
    : m_actPage(0)
    , m_numPages(0)
  {
  }

  int m_actPage /** the actual page */, m_numPages /** the number of page of the final document */;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDXParser::SDXParser(STOFFInputStreamPtr &input, STOFFHeader *header)
  : STOFFTextParser(input, header)
  , m_password(nullptr)
  , m_oleParser()
  , m_state()
{
  init();
}

SDXParser::~SDXParser()
{
}

void SDXParser::init()
{
  setAsciiName("main-1");

  m_state.reset(new SDXParserInternal::State);
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
void SDXParser::parse(librevenge::RVNGTextInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(nullptr))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    // create the asciiFile
    checkHeader(nullptr);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDXParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  if (!ok) throw(libstoff::ParseException());
}


bool SDXParser::createZones()
{
  m_oleParser.reset(new STOFFOLEParser);
  m_oleParser->parse(getInput());

  // send the final data
  auto listDir=m_oleParser->getDirectoryList();
  for (auto &dir : listDir) {
    if (!dir || dir->m_parsed) continue;
    dir->m_parsed=true;
    StarObject object(m_password, m_oleParser, dir);
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
    STOFFOLEParser::OleDirectory &direct=*dir;
    auto unparsedOLEs=direct.getUnparsedOles();
    StarFileManager fileManager;
    for (auto const &name : unparsedOLEs) {
      STOFFInputStreamPtr ole = getInput()->getSubStreamByName(name.c_str());
      if (!ole.get()) {
        STOFF_DEBUG_MSG(("SDXParser::createZones: error: can not find OLE part: \"%s\"\n", name.c_str()));
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
        fileManager.readImageDocument(ole,data,name);
        continue;
      }
      if (base.compare(0,3,"Pic")==0) {
        librevenge::RVNGBinaryData data;
        std::string type;
        fileManager.readEmbeddedPicture(ole,data,type,name);
        continue;
      }
      // other
      if (base=="Ole-Object") {
        librevenge::RVNGBinaryData data;
        fileManager.readOleObject(ole,data,name);
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
  return false;
}

////////////////////////////////////////////////////////////
// create the document
////////////////////////////////////////////////////////////
void SDXParser::createDocument(librevenge::RVNGTextInterface *documentInterface)
{
  if (!documentInterface) return;
  STOFF_DEBUG_MSG(("SDXParser::createDocument: not implemented, exit\n"));
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
bool SDXParser::checkHeader(STOFFHeader * /*header*/, bool /*strict*/)
{
  *m_state = SDXParserInternal::State();

  STOFFInputStreamPtr input = getInput();
  if (!input || !input->hasDataFork() || !input->isStructured())
    return false;
  //if (header)
  //header->reset(1);
#ifndef DEBUG
  return false;
#else
  return true;
#endif
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
