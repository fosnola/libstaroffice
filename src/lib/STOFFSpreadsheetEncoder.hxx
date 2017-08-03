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

#ifndef STOFF_SPREADSHEET_ENCODER_HXX
#define STOFF_SPREADSHEET_ENCODER_HXX

#include <librevenge/librevenge.h>
#include "libstaroffice_internal.hxx"

class STOFFPropertyHandlerEncoder;

namespace STOFFSpreadsheetEncoderInternal
{
struct State;
}
/** main class used to define store librevenge::RVNGSpreadsheetInterface
    lists of command in a librevenge::RVNGBinaryData. \see STOFFSpreadsheetDecoder
    can be used to decode back the spreadsheet...

	\note as this class implements the functions librevenge::RVNGSpreadsheetInterface,
  the documentation is not duplicated..
*/
class STOFFSpreadsheetEncoder final : public librevenge::RVNGSpreadsheetInterface
{
public:
  /// constructor
  STOFFSpreadsheetEncoder();
  /// destructor
  ~STOFFSpreadsheetEncoder() final;
  /// return the final spreadsheet
  bool getBinaryResult(STOFFEmbeddedObject &object);

  void setDocumentMetaData(const librevenge::RVNGPropertyList &propList) final;

  void startDocument(const librevenge::RVNGPropertyList &propList) final;
  void endDocument() final;

  void definePageStyle(const librevenge::RVNGPropertyList &propList) final;
  void defineEmbeddedFont(const librevenge::RVNGPropertyList &propList) final;

  void openPageSpan(const librevenge::RVNGPropertyList &propList) final;
  void closePageSpan() final;

  void openHeader(const librevenge::RVNGPropertyList &propList) final;
  void closeHeader() final;

  void openFooter(const librevenge::RVNGPropertyList &propList) final;
  void closeFooter() final;

  void defineSheetNumberingStyle(const librevenge::RVNGPropertyList &propList) final;
  void openSheet(const librevenge::RVNGPropertyList &propList) final;
  void closeSheet() final;
  void openSheetRow(const librevenge::RVNGPropertyList &propList) final;
  void closeSheetRow() final;
  void openSheetCell(const librevenge::RVNGPropertyList &propList) final;
  void closeSheetCell() final;

  void defineChartStyle(const librevenge::RVNGPropertyList &propList) final;

  void openChart(const librevenge::RVNGPropertyList &propList) final;
  void closeChart() final;

  void openChartTextObject(const librevenge::RVNGPropertyList &propList) final;
  void closeChartTextObject() final;

  void openChartPlotArea(const librevenge::RVNGPropertyList &propList) final;
  void closeChartPlotArea() final;
  void insertChartAxis(const librevenge::RVNGPropertyList &axis) final;
  void openChartSerie(const librevenge::RVNGPropertyList &series) final;
  void closeChartSerie() final;

  void defineParagraphStyle(const librevenge::RVNGPropertyList &propList) final;

  void openParagraph(const librevenge::RVNGPropertyList &propList) final;
  void closeParagraph() final;

  void defineCharacterStyle(const librevenge::RVNGPropertyList &propList) final;

  void openSpan(const librevenge::RVNGPropertyList &propList) final;
  void closeSpan() final;
  void openLink(const librevenge::RVNGPropertyList &propList) final;
  void closeLink() final;

  void defineSectionStyle(const librevenge::RVNGPropertyList &propList) final;

  void openSection(const librevenge::RVNGPropertyList &propList) final;
  void closeSection() final;

  void insertTab() final;
  void insertSpace() final;
  void insertText(const librevenge::RVNGString &text) final;
  void insertLineBreak() final;

  void insertField(const librevenge::RVNGPropertyList &propList) final;

  void openOrderedListLevel(const librevenge::RVNGPropertyList &propList) final;
  void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList) final;
  void closeOrderedListLevel() final;
  void closeUnorderedListLevel() final;
  void openListElement(const librevenge::RVNGPropertyList &propList) final;
  void closeListElement() final;

  void openFootnote(const librevenge::RVNGPropertyList &propList) final;
  void closeFootnote() final;

  void openComment(const librevenge::RVNGPropertyList &propList) final;
  void closeComment() final;

  void openFrame(const librevenge::RVNGPropertyList &propList) final;
  void closeFrame() final;
  void insertBinaryObject(const librevenge::RVNGPropertyList &propList) final;

  //
  // specific text/table
  //

  void openTextBox(const librevenge::RVNGPropertyList &propList) final;
  void closeTextBox() final;

  void openTable(const librevenge::RVNGPropertyList &propList) final;
  void closeTable() final;
  void openTableRow(const librevenge::RVNGPropertyList &propList) final;
  void closeTableRow() final;
  void openTableCell(const librevenge::RVNGPropertyList &propList) final;
  void closeTableCell() final;
  void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList) final;

  //
  // simple Graphic
  //

  void openGroup(const librevenge::RVNGPropertyList &propList) final;
  void closeGroup() final;

  void defineGraphicStyle(const librevenge::RVNGPropertyList &propList) final;

  void drawRectangle(const librevenge::RVNGPropertyList &propList) final;
  void drawEllipse(const librevenge::RVNGPropertyList &propList) final;
  void drawPolygon(const librevenge::RVNGPropertyList &propList) final;
  void drawPolyline(const librevenge::RVNGPropertyList &propList) final;
  void drawPath(const librevenge::RVNGPropertyList &propList) final;
  void drawConnector(const ::librevenge::RVNGPropertyList &propList) final;

  //
  // Equation
  //

  void insertEquation(const librevenge::RVNGPropertyList &propList) final;

protected:
  //! the actual state
  std::shared_ptr<STOFFSpreadsheetEncoderInternal::State> m_state;
};

#endif

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
