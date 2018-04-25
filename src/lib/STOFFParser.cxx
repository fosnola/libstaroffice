/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libmwaw
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

#include "libstaroffice_internal.hxx"

#include "STOFFList.hxx"
#include "STOFFGraphicListener.hxx"
#include "STOFFSpreadsheetListener.hxx"
#include "STOFFTextListener.hxx"

#include "STOFFParser.hxx"

STOFFParserState::STOFFParserState(STOFFParserState::Type type, STOFFInputStreamPtr &input, STOFFHeader *header)
  : m_type(type)
  , m_kind(STOFFDocument::STOFF_K_TEXT)
  , m_version(0)
  , m_input(input)
  , m_header(header)
  , m_pageSpan()
  , m_listManager()
  , m_graphicListener()
  , m_spreadsheetListener()
  , m_textListener()
  , m_asciiFile(input)
{
  if (header) {
    m_version=header->getVersion();
    m_kind=header->getKind();
  }
  m_listManager.reset(new STOFFListManager);
}

STOFFParserState::~STOFFParserState()
{
}

STOFFParser::STOFFParser(STOFFParserState::Type type, STOFFInputStreamPtr input, STOFFHeader *header)
  : m_parserState()
  , m_asciiName("")
{
  m_parserState.reset(new STOFFParserState(type, input, header));
}

STOFFParser::~STOFFParser()
{
}

void STOFFParser::setGraphicListener(STOFFGraphicListenerPtr &listener)
{
  m_parserState->m_graphicListener=listener;
}

void STOFFParser::resetGraphicListener()
{
  if (getGraphicListener()) getGraphicListener()->endDocument();
  m_parserState->m_graphicListener.reset();
}

void STOFFParser::setSpreadsheetListener(STOFFSpreadsheetListenerPtr &listener)
{
  m_parserState->m_spreadsheetListener=listener;
}

void STOFFParser::resetSpreadsheetListener()
{
  if (getSpreadsheetListener()) getSpreadsheetListener()->endDocument();
  m_parserState->m_spreadsheetListener.reset();
}

void STOFFParser::setTextListener(STOFFTextListenerPtr &listener)
{
  m_parserState->m_textListener=listener;
}

void STOFFParser::resetTextListener()
{
  if (getTextListener()) getTextListener()->endDocument();
  m_parserState->m_textListener.reset();
}

STOFFGraphicParser::~STOFFGraphicParser()
{
}

void STOFFGraphicParser::parse(librevenge::RVNGDrawingInterface * /*documentInterface*/)
{
  STOFF_DEBUG_MSG(("STOFFGraphicParser::parse[drawingInterface]: not implemented\n"));
}

void STOFFGraphicParser::parse(librevenge::RVNGPresentationInterface * /*documentInterface*/)
{
  STOFF_DEBUG_MSG(("STOFFGraphicParser::parse[drawing/Interface]: not implemented\n"));
}

STOFFSpreadsheetParser::~STOFFSpreadsheetParser()
{
}

STOFFTextParser::~STOFFTextParser()
{
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
