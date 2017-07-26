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
class STOFFSpreadsheetEncoder : public librevenge::RVNGSpreadsheetInterface
{
public:
  /// constructor
  STOFFSpreadsheetEncoder();
  /// destructor
  ~STOFFSpreadsheetEncoder();
  /// return the final spreadsheet
  bool getBinaryResult(STOFFEmbeddedObject &object);

  void setDocumentMetaData(const librevenge::RVNGPropertyList &propList);

  void startDocument(const librevenge::RVNGPropertyList &propList);
  void endDocument();

  void definePageStyle(const librevenge::RVNGPropertyList &propList);
  void defineEmbeddedFont(const librevenge::RVNGPropertyList &propList);

  void openPageSpan(const librevenge::RVNGPropertyList &propList);
  void closePageSpan();

  void openHeader(const librevenge::RVNGPropertyList &propList);
  void closeHeader();

  void openFooter(const librevenge::RVNGPropertyList &propList);
  void closeFooter();

  void defineSheetNumberingStyle(const librevenge::RVNGPropertyList &propList);
  void openSheet(const librevenge::RVNGPropertyList &propList);
  void closeSheet();
  void openSheetRow(const librevenge::RVNGPropertyList &propList);
  void closeSheetRow();
  void openSheetCell(const librevenge::RVNGPropertyList &propList);
  void closeSheetCell();

  void defineChartStyle(const librevenge::RVNGPropertyList &propList);

  void openChart(const librevenge::RVNGPropertyList &propList);
  void closeChart();

  void openChartTextObject(const librevenge::RVNGPropertyList &propList);
  void closeChartTextObject();

  void openChartPlotArea(const librevenge::RVNGPropertyList &propList);
  void closeChartPlotArea();
  void insertChartAxis(const librevenge::RVNGPropertyList &axis);
  void openChartSerie(const librevenge::RVNGPropertyList &series);
  void closeChartSerie();

  void defineParagraphStyle(const librevenge::RVNGPropertyList &propList);

  void openParagraph(const librevenge::RVNGPropertyList &propList);
  void closeParagraph();

  void defineCharacterStyle(const librevenge::RVNGPropertyList &propList);

  void openSpan(const librevenge::RVNGPropertyList &propList);
  void closeSpan();
  void openLink(const librevenge::RVNGPropertyList &propList);
  void closeLink();

  void defineSectionStyle(const librevenge::RVNGPropertyList &propList);

  void openSection(const librevenge::RVNGPropertyList &propList);
  void closeSection();

  void insertTab();
  void insertSpace();
  void insertText(const librevenge::RVNGString &text);
  void insertLineBreak();

  void insertField(const librevenge::RVNGPropertyList &propList);

  void openOrderedListLevel(const librevenge::RVNGPropertyList &propList);
  void openUnorderedListLevel(const librevenge::RVNGPropertyList &propList);
  void closeOrderedListLevel();
  void closeUnorderedListLevel();
  void openListElement(const librevenge::RVNGPropertyList &propList);
  void closeListElement();

  void openFootnote(const librevenge::RVNGPropertyList &propList);
  void closeFootnote();

  void openComment(const librevenge::RVNGPropertyList &propList);
  void closeComment();

  void openFrame(const librevenge::RVNGPropertyList &propList);
  void closeFrame();
  void insertBinaryObject(const librevenge::RVNGPropertyList &propList);

  //
  // specific text/table
  //

  void openTextBox(const librevenge::RVNGPropertyList &propList);
  void closeTextBox();

  void openTable(const librevenge::RVNGPropertyList &propList);
  void closeTable();
  void openTableRow(const librevenge::RVNGPropertyList &propList);
  void closeTableRow();
  void openTableCell(const librevenge::RVNGPropertyList &propList);
  void closeTableCell();
  void insertCoveredTableCell(const librevenge::RVNGPropertyList &propList);

  //
  // simple Graphic
  //

  void openGroup(const librevenge::RVNGPropertyList &propList);
  void closeGroup();

  void defineGraphicStyle(const librevenge::RVNGPropertyList &propList);

  void drawRectangle(const librevenge::RVNGPropertyList &propList);
  void drawEllipse(const librevenge::RVNGPropertyList &propList);
  void drawPolygon(const librevenge::RVNGPropertyList &propList);
  void drawPolyline(const librevenge::RVNGPropertyList &propList);
  void drawPath(const librevenge::RVNGPropertyList &propList);
  void drawConnector(const ::librevenge::RVNGPropertyList &propList);

  //
  // Equation
  //

  void insertEquation(const librevenge::RVNGPropertyList &propList);

protected:
  //! the actual state
  std::shared_ptr<STOFFSpreadsheetEncoderInternal::State> m_state;
};

#endif

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
