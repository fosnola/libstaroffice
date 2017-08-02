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
* Copyright (C) 2006 Ariya Hidayat (ariya@kde.org)
* Copyright (C) 2004 Marc Oude Kotte (marc@solcon.nl)
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

#include <string.h>

#include <map>
#include <sstream>
#include <string>

#include <librevenge/librevenge.h>
#include <libstaroffice/libstaroffice.hxx>

#include "libstaroffice_internal.hxx"

#include "STOFFPropertyHandler.hxx"

#include "STOFFGraphicEncoder.hxx"

//! a name space used to define internal data of STOFFGraphicEncoder
namespace STOFFGraphicEncoderInternal
{
//! the state of a STOFFGraphicEncoder
struct State {
  //! constructor
  State() : m_encoder()
  {
  }
  //! the encoder
  STOFFPropertyHandlerEncoder m_encoder;
};

}

STOFFGraphicEncoder::STOFFGraphicEncoder()
  : librevenge::RVNGDrawingInterface()
  , m_state(new STOFFGraphicEncoderInternal::State)
{
}

STOFFGraphicEncoder::~STOFFGraphicEncoder()
{
}

bool STOFFGraphicEncoder::getBinaryResult(STOFFEmbeddedObject &result)
{
  librevenge::RVNGBinaryData data;
  if (!m_state->m_encoder.getData(data))
    return false;
  result=STOFFEmbeddedObject(data, "image/stoff-odg");
  return true;
}

void STOFFGraphicEncoder::startDocument(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartDocument", list);
}

void STOFFGraphicEncoder::endDocument()
{
  m_state->m_encoder.insertElement("EndDocument");
}

void STOFFGraphicEncoder::setDocumentMetaData(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("SetDocumentMetaData", list);
}

void STOFFGraphicEncoder::defineEmbeddedFont(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineEmbeddedFont", list);
}

void STOFFGraphicEncoder::startPage(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartPage", list);
}

void STOFFGraphicEncoder::endPage()
{
  m_state->m_encoder.insertElement("EndPage");
}

void STOFFGraphicEncoder::startMasterPage(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartMasterPage", list);
}

void STOFFGraphicEncoder::endMasterPage()
{
  m_state->m_encoder.insertElement("EndMasterPage");
}

void STOFFGraphicEncoder::setStyle(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("SetStyle", list);
}

void STOFFGraphicEncoder::startLayer(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartLayer", list);
}

void STOFFGraphicEncoder::endLayer()
{
  m_state->m_encoder.insertElement("EndLayer");
}

void STOFFGraphicEncoder::startEmbeddedGraphics(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartEmbeddedGraphics", list);
}

void STOFFGraphicEncoder::endEmbeddedGraphics()
{
  m_state->m_encoder.insertElement("StartEmbeddedGraphics");
}

void STOFFGraphicEncoder::openGroup(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenGroup", list);
}

void STOFFGraphicEncoder::closeGroup()
{
  m_state->m_encoder.insertElement("CloseGroup");
}

void STOFFGraphicEncoder::drawRectangle(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawRectangle", list);
}

void STOFFGraphicEncoder::drawEllipse(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawEllipse", list);
}

void STOFFGraphicEncoder::drawPolygon(const ::librevenge::RVNGPropertyList &vertices)
{
  m_state->m_encoder.insertElement("DrawPolygon", vertices);
}

void STOFFGraphicEncoder::drawPolyline(const ::librevenge::RVNGPropertyList &vertices)
{
  m_state->m_encoder.insertElement("DrawPolyline", vertices);
}

void STOFFGraphicEncoder::drawPath(const ::librevenge::RVNGPropertyList &path)
{
  m_state->m_encoder.insertElement("DrawPath", path);
}

void STOFFGraphicEncoder::drawConnector(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawConnector", list);
}

void STOFFGraphicEncoder::drawGraphicObject(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawGraphicObject", list);
}

void STOFFGraphicEncoder::startTextObject(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartTextObject", list);
}

void STOFFGraphicEncoder::endTextObject()
{
  m_state->m_encoder.insertElement("EndTextObject");
}

void STOFFGraphicEncoder::startTableObject(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartTableObject", list);
}

void STOFFGraphicEncoder::endTableObject()
{
  m_state->m_encoder.insertElement("EndTableObject");
}

void STOFFGraphicEncoder::openTableRow(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenTableRow", list);
}

void STOFFGraphicEncoder::closeTableRow()
{
  m_state->m_encoder.insertElement("CloseTableRow");
}

void STOFFGraphicEncoder::openTableCell(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenTableCell", list);
}

void STOFFGraphicEncoder::closeTableCell()
{
  m_state->m_encoder.insertElement("CloseTableCell");
}

void STOFFGraphicEncoder::insertCoveredTableCell(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertCoveredTableCell", list);
}

void STOFFGraphicEncoder::insertTab()
{
  m_state->m_encoder.insertElement("InsertTab");
}

void STOFFGraphicEncoder::insertSpace()
{
  m_state->m_encoder.insertElement("InsertSpace");
}

void STOFFGraphicEncoder::insertText(const librevenge::RVNGString &text)
{
  m_state->m_encoder.characters(text);
}

void STOFFGraphicEncoder::insertLineBreak()
{
  m_state->m_encoder.insertElement("InsertLineBreak");
}

void STOFFGraphicEncoder::insertField(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertField", list);
}

void STOFFGraphicEncoder::openLink(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenLink", list);
}

void STOFFGraphicEncoder::closeLink()
{
  m_state->m_encoder.insertElement("CloseLink");
}

void STOFFGraphicEncoder::openOrderedListLevel(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenOrderedListLevel", list);
}

void STOFFGraphicEncoder::openUnorderedListLevel(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenUnorderedListLevel", list);
}

void STOFFGraphicEncoder::closeOrderedListLevel()
{
  m_state->m_encoder.insertElement("CloseOrderedListLevel");
}

void STOFFGraphicEncoder::closeUnorderedListLevel()
{
  m_state->m_encoder.insertElement("CloseOrderedListLevel");
}

void STOFFGraphicEncoder::openListElement(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenListElement", list);
}

void STOFFGraphicEncoder::closeListElement()
{
  m_state->m_encoder.insertElement("CloseListElement");
}

void STOFFGraphicEncoder::defineParagraphStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineParagraphStyle", list);
}

void STOFFGraphicEncoder::openParagraph(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenParagraph", list);
}

void STOFFGraphicEncoder::closeParagraph()
{
  m_state->m_encoder.insertElement("CloseParagraph");
}

void STOFFGraphicEncoder::defineCharacterStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineCharacterStyle", list);
}

void STOFFGraphicEncoder::openSpan(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenSpan", list);
}

void STOFFGraphicEncoder::closeSpan()
{
  m_state->m_encoder.insertElement("CloseSpan");
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
