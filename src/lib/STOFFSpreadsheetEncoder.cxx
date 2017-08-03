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

#include "STOFFSpreadsheetEncoder.hxx"

//! a name space used to define internal data of STOFFSpreadsheetEncoder
namespace STOFFSpreadsheetEncoderInternal
{
//! the state of a STOFFSpreadsheetEncoder
struct State {
  //! constructor
  State()
    : m_encoder()
  {
  }
  //! the encoder
  STOFFPropertyHandlerEncoder m_encoder;
};

}

STOFFSpreadsheetEncoder::STOFFSpreadsheetEncoder()
  : librevenge::RVNGSpreadsheetInterface()
  , m_state(new STOFFSpreadsheetEncoderInternal::State)
{
}

STOFFSpreadsheetEncoder::~STOFFSpreadsheetEncoder()
{
}

bool STOFFSpreadsheetEncoder::getBinaryResult(STOFFEmbeddedObject &object)
{
  librevenge::RVNGBinaryData data;
  if (!m_state->m_encoder.getData(data))
    return false;
  object=STOFFEmbeddedObject(data, "image/stoff-ods");
  return true;
}

void STOFFSpreadsheetEncoder::setDocumentMetaData(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("SetDocumentMetaData", list);
}

void STOFFSpreadsheetEncoder::startDocument(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("StartDocument", list);
}

void STOFFSpreadsheetEncoder::endDocument()
{
  m_state->m_encoder.insertElement("EndDocument");
}

//
// page
//
void STOFFSpreadsheetEncoder::definePageStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefinePageStyle", list);
}

void STOFFSpreadsheetEncoder::defineEmbeddedFont(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineEmbeddedFont", list);
}

void STOFFSpreadsheetEncoder::openPageSpan(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenPageSpan", list);
}
void STOFFSpreadsheetEncoder::closePageSpan()
{
  m_state->m_encoder.insertElement("ClosePageSpan");
}

void STOFFSpreadsheetEncoder::openHeader(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenHeader", list);
}
void STOFFSpreadsheetEncoder::closeHeader()
{
  m_state->m_encoder.insertElement("CloseHeader");
}

void STOFFSpreadsheetEncoder::openFooter(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenFooter", list);
}
void STOFFSpreadsheetEncoder::closeFooter()
{
  m_state->m_encoder.insertElement("CloseFooter");
}

//
// spreadsheet
//
void STOFFSpreadsheetEncoder::defineSheetNumberingStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineSheetNumberingStyle", list);
}
void STOFFSpreadsheetEncoder::openSheet(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenSheet", list);
}
void STOFFSpreadsheetEncoder::closeSheet()
{
  m_state->m_encoder.insertElement("CloseSheet");
}
void STOFFSpreadsheetEncoder::openSheetRow(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenSheetRow", list);
}

void STOFFSpreadsheetEncoder::closeSheetRow()
{
  m_state->m_encoder.insertElement("CloseSheetRow");
}

void STOFFSpreadsheetEncoder::openSheetCell(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenSheetCell", list);
}

void STOFFSpreadsheetEncoder::closeSheetCell()
{
  m_state->m_encoder.insertElement("CloseSheetCell");
}

//
// chart
//

void STOFFSpreadsheetEncoder::defineChartStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineChartStyle", list);
}

void STOFFSpreadsheetEncoder::openChart(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenChart", list);
}

void STOFFSpreadsheetEncoder::closeChart()
{
  m_state->m_encoder.insertElement("CloseChart");
}

void STOFFSpreadsheetEncoder::openChartTextObject(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenChartTextObject", list);
}

void STOFFSpreadsheetEncoder::closeChartTextObject()
{
  m_state->m_encoder.insertElement("CloseChartTextObject");
}

void STOFFSpreadsheetEncoder::openChartPlotArea(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenChartPlotArea", list);
}

void STOFFSpreadsheetEncoder::closeChartPlotArea()
{
  m_state->m_encoder.insertElement("CloseChartPlotArea");
}

void STOFFSpreadsheetEncoder::insertChartAxis(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertChartAxis", list);
}

void STOFFSpreadsheetEncoder::openChartSerie(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenChartSerie", list);
}

void STOFFSpreadsheetEncoder::closeChartSerie()
{
  m_state->m_encoder.insertElement("CloseChartSerie");
}


//
// para styles + character styles + link
//
void STOFFSpreadsheetEncoder::defineParagraphStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineParagraphStyle", list);
}

void STOFFSpreadsheetEncoder::openParagraph(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenParagraph", list);
}

void STOFFSpreadsheetEncoder::closeParagraph()
{
  m_state->m_encoder.insertElement("CloseParagraph");
}

void STOFFSpreadsheetEncoder::defineCharacterStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineCharacterStyle", list);
}

void STOFFSpreadsheetEncoder::openSpan(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenSpan", list);
}

void STOFFSpreadsheetEncoder::closeSpan()
{
  m_state->m_encoder.insertElement("CloseSpan");
}

void STOFFSpreadsheetEncoder::openLink(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenLink", list);
}

void STOFFSpreadsheetEncoder::closeLink()
{
  m_state->m_encoder.insertElement("CloseLink");
}

//
// section + add basic char
//
void STOFFSpreadsheetEncoder::defineSectionStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineSectionStyle", list);
}

void STOFFSpreadsheetEncoder::openSection(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenSection", list);
}

void STOFFSpreadsheetEncoder::closeSection()
{
  m_state->m_encoder.insertElement("CloseSection");
}

void STOFFSpreadsheetEncoder::insertTab()
{
  m_state->m_encoder.insertElement("InsertTab");
}

void STOFFSpreadsheetEncoder::insertSpace()
{
  m_state->m_encoder.insertElement("InsertSpace");
}

void STOFFSpreadsheetEncoder::insertText(const librevenge::RVNGString &text)
{
  m_state->m_encoder.characters(text);
}

void STOFFSpreadsheetEncoder::insertLineBreak()
{
  m_state->m_encoder.insertElement("InsertLineBreak");
}

void STOFFSpreadsheetEncoder::insertField(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertField", list);
}

//
// list
//
void STOFFSpreadsheetEncoder::openOrderedListLevel(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenOrderedListLevel", list);
}

void STOFFSpreadsheetEncoder::openUnorderedListLevel(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenUnorderedListLevel", list);
}

void STOFFSpreadsheetEncoder::closeOrderedListLevel()
{
  m_state->m_encoder.insertElement("CloseOrderedListLevel");
}

void STOFFSpreadsheetEncoder::closeUnorderedListLevel()
{
  m_state->m_encoder.insertElement("CloseOrderedListLevel");
}

void STOFFSpreadsheetEncoder::openListElement(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenListElement", list);
}

void STOFFSpreadsheetEncoder::closeListElement()
{
  m_state->m_encoder.insertElement("CloseListElement");
}

//
// footnote, comment, frame
//

void STOFFSpreadsheetEncoder::openFootnote(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenFootnote", list);
}

void STOFFSpreadsheetEncoder::closeFootnote()
{
  m_state->m_encoder.insertElement("CloseFootnote");
}

void STOFFSpreadsheetEncoder::openComment(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenComment", list);
}
void STOFFSpreadsheetEncoder::closeComment()
{
  m_state->m_encoder.insertElement("CloseComment");
}

void STOFFSpreadsheetEncoder::openFrame(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenFrame", list);
}
void STOFFSpreadsheetEncoder::closeFrame()
{
  m_state->m_encoder.insertElement("CloseFrame");
}
void STOFFSpreadsheetEncoder::insertBinaryObject(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertBinaryObject", list);
}

//
// specific text/table
//
void STOFFSpreadsheetEncoder::openTextBox(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenTextBox", list);
}

void STOFFSpreadsheetEncoder::closeTextBox()
{
  m_state->m_encoder.insertElement("CloseTextBox");
}

void STOFFSpreadsheetEncoder::openTable(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenTable", list);
}
void STOFFSpreadsheetEncoder::closeTable()
{
  m_state->m_encoder.insertElement("CloseTable");
}
void STOFFSpreadsheetEncoder::openTableRow(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenTableRow", list);
}

void STOFFSpreadsheetEncoder::closeTableRow()
{
  m_state->m_encoder.insertElement("CloseTableRow");
}

void STOFFSpreadsheetEncoder::openTableCell(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenTableCell", list);
}

void STOFFSpreadsheetEncoder::closeTableCell()
{
  m_state->m_encoder.insertElement("CloseTableCell");
}

void STOFFSpreadsheetEncoder::insertCoveredTableCell(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertCoveredTableCell", list);
}

//
// simple Graphic
//

void STOFFSpreadsheetEncoder::openGroup(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("OpenGroup", list);
}

void STOFFSpreadsheetEncoder::closeGroup()
{
  m_state->m_encoder.insertElement("CloseGroup");
}

void STOFFSpreadsheetEncoder::defineGraphicStyle(const librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DefineGraphicStyle", list);
}

void STOFFSpreadsheetEncoder::drawRectangle(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawRectangle", list);
}

void STOFFSpreadsheetEncoder::drawEllipse(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawEllipse", list);
}

void STOFFSpreadsheetEncoder::drawPolygon(const ::librevenge::RVNGPropertyList &vertices)
{
  m_state->m_encoder.insertElement("DrawPolygon", vertices);
}

void STOFFSpreadsheetEncoder::drawPolyline(const ::librevenge::RVNGPropertyList &vertices)
{
  m_state->m_encoder.insertElement("DrawPolyline", vertices);
}

void STOFFSpreadsheetEncoder::drawPath(const ::librevenge::RVNGPropertyList &path)
{
  m_state->m_encoder.insertElement("DrawPath", path);
}

void STOFFSpreadsheetEncoder::drawConnector(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("DrawConnector", list);
}

//
// equation
//
void STOFFSpreadsheetEncoder::insertEquation(const ::librevenge::RVNGPropertyList &list)
{
  m_state->m_encoder.insertElement("InsertEquation", list);
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
