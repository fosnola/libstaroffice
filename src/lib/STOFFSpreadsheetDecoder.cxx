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

#include <librevenge/librevenge.h>
#include <libstaroffice/libstaroffice.hxx>

#include "libstaroffice_internal.hxx"

#include "STOFFDebug.hxx"

#include "STOFFSpreadsheetDecoder.hxx"

void STOFFSpreadsheetDecoder::insertElement(const char *psName)
{
  if (!m_output) return;
  if (!psName || !*psName) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetDecoder::insertElement: called without any name\n"));
    return;
  }

  bool ok=true;
  switch (psName[0]) {
  case 'C':
    if (strcmp(psName,"CloseChart")==0)
      m_output->closeChart();
    else if (strcmp(psName,"CloseChartPlotArea")==0)
      m_output->closeChartPlotArea();
    else if (strcmp(psName,"CloseChartSerie")==0)
      m_output->closeChartSerie();
    else if (strcmp(psName,"CloseChartTextObject")==0)
      m_output->closeChartTextObject();
    else if (strcmp(psName,"CloseComment")==0)
      m_output->closeComment();
    else if (strcmp(psName,"CloseFooter")==0)
      m_output->closeFooter();
    else if (strcmp(psName,"CloseFootnote")==0)
      m_output->closeFootnote();
    else if (strcmp(psName,"CloseFrame")==0)
      m_output->closeFrame();
    else if (strcmp(psName,"CloseGroup")==0)
      m_output->closeGroup();
    else if (strcmp(psName,"CloseHeader")==0)
      m_output->closeHeader();
    else if (strcmp(psName,"CloseLink")==0)
      m_output->closeLink();
    else if (strcmp(psName,"CloseListElement")==0)
      m_output->closeListElement();
    else if (strcmp(psName,"CloseOrderedListLevel")==0)
      m_output->closeOrderedListLevel();
    else if (strcmp(psName,"ClosePageSpan")==0)
      m_output->closePageSpan();
    else if (strcmp(psName,"CloseParagraph")==0)
      m_output->closeParagraph();
    else if (strcmp(psName,"CloseSection")==0)
      m_output->closeSection();
    else if (strcmp(psName,"CloseSheet")==0)
      m_output->closeSheet();
    else if (strcmp(psName,"CloseSheetCell")==0)
      m_output->closeSheetCell();
    else if (strcmp(psName,"CloseSheetRow")==0)
      m_output->closeSheetRow();
    else if (strcmp(psName,"CloseSpan")==0)
      m_output->closeSpan();
    else if (strcmp(psName,"CloseTableCell")==0)
      m_output->closeTableCell();
    else if (strcmp(psName,"CloseTableRow")==0)
      m_output->closeTableRow();
    else if (strcmp(psName,"CloseTextBox")==0)
      m_output->closeTextBox();
    else if (strcmp(psName,"CloseUnorderedListLevel")==0)
      m_output->closeUnorderedListLevel();
    else
      ok=false;
    break;
  case 'E':
    if (strcmp(psName,"EndDocument")==0)
      m_output->endDocument();
    else
      ok=false;
    break;
  case 'I':
    if (strcmp(psName,"InsertTab")==0)
      m_output->insertTab();
    else if (strcmp(psName,"InsertSpace")==0)
      m_output->insertSpace();
    else if (strcmp(psName,"InsertLineBreak")==0)
      m_output->insertLineBreak();
    else
      ok=false;
    break;
  default:
    ok=false;
    break;
  }
  if (!ok) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetDecoder::insertElement: called with unexpected name %s\n", psName));
  }
}

void STOFFSpreadsheetDecoder::insertElement(const char *psName, const librevenge::RVNGPropertyList &propList)
{
  if (!m_output) return;
  if (!psName || !*psName) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetDecoder::insertElement: called without any name\n"));
    return;
  }

  bool ok=true;
  switch (psName[0]) {
  case 'D':
    if (strcmp(psName,"DefineCharacterStyle")==0)
      m_output->defineCharacterStyle(propList);
    else if (strcmp(psName,"DefineChartStyle")==0)
      m_output->defineChartStyle(propList);
    else if (strcmp(psName,"DefineEmbeddedFont")==0)
      m_output->defineEmbeddedFont(propList);
    else if (strcmp(psName,"DefineGraphicStyle")==0)
      m_output->defineGraphicStyle(propList);
    else if (strcmp(psName,"DefinePageStyle")==0)
      m_output->definePageStyle(propList);
    else if (strcmp(psName,"DefineParagraphStyle")==0)
      m_output->defineParagraphStyle(propList);
    else if (strcmp(psName,"DefineSectionStyle")==0)
      m_output->defineSectionStyle(propList);
    else if (strcmp(psName,"DefineSheetNumberingStyle")==0)
      m_output->defineSheetNumberingStyle(propList);

    else if (strcmp(psName,"DrawConnector")==0)
      m_output->drawConnector(propList);
    else if (strcmp(psName,"DrawEllipse")==0)
      m_output->drawEllipse(propList);
    else if (strcmp(psName,"DrawPath")==0)
      m_output->drawPath(propList);
    else if (strcmp(psName,"DrawPolygon")==0)
      m_output->drawPolygon(propList);
    else if (strcmp(psName,"DrawPolyline")==0)
      m_output->drawPolyline(propList);
    else if (strcmp(psName,"DrawRectangle")==0)
      m_output->drawRectangle(propList);
    else
      ok=false;
    break;
  case 'I':
    if (strcmp(psName,"InsertBinaryObject")==0)
      m_output->insertBinaryObject(propList);
    else if (strcmp(psName,"InsertChartAxis")==0)
      m_output->insertChartAxis(propList);
    else if (strcmp(psName,"InsertCoveredTableCell")==0)
      m_output->insertCoveredTableCell(propList);
    else if (strcmp(psName,"InsertEquation")==0)
      m_output->insertEquation(propList);
    else if (strcmp(psName,"InsertField")==0)
      m_output->insertField(propList);
    else
      ok=false;
    break;
  case 'O':
    if (strcmp(psName,"OpenChart")==0)
      m_output->openChart(propList);
    else if (strcmp(psName,"OpenChartPlotArea")==0)
      m_output->openChartPlotArea(propList);
    else if (strcmp(psName,"OpenChartSerie")==0)
      m_output->openChartSerie(propList);
    else if (strcmp(psName,"OpenChartTextObject")==0)
      m_output->openChartTextObject(propList);
    else if (strcmp(psName,"OpenComment")==0)
      m_output->openComment(propList);
    else if (strcmp(psName,"OpenFooter")==0)
      m_output->openFooter(propList);
    else if (strcmp(psName,"OpenFootnote")==0)
      m_output->openFootnote(propList);
    else if (strcmp(psName,"OpenFrame")==0)
      m_output->openFrame(propList);
    else if (strcmp(psName,"OpenGroup")==0)
      m_output->openGroup(propList);
    else if (strcmp(psName,"OpenHeader")==0)
      m_output->openHeader(propList);
    else if (strcmp(psName,"OpenLink")==0)
      m_output->openLink(propList);
    else if (strcmp(psName,"OpenListElement")==0)
      m_output->openListElement(propList);
    else if (strcmp(psName,"OpenOrderedListLevel")==0)
      m_output->openOrderedListLevel(propList);
    else if (strcmp(psName,"OpenPageSpan")==0)
      m_output->openPageSpan(propList);
    else if (strcmp(psName,"OpenParagraph")==0)
      m_output->openParagraph(propList);
    else if (strcmp(psName,"OpenSheet")==0)
      m_output->openSheet(propList);
    else if (strcmp(psName,"OpenSection")==0)
      m_output->openSection(propList);
    else if (strcmp(psName,"OpenSheetCell")==0)
      m_output->openSheetCell(propList);
    else if (strcmp(psName,"OpenSheetRow")==0)
      m_output->openSheetRow(propList);
    else if (strcmp(psName,"OpenSpan")==0)
      m_output->openSpan(propList);
    else if (strcmp(psName,"OpenTableCell")==0)
      m_output->openTableCell(propList);
    else if (strcmp(psName,"OpenTableRow")==0)
      m_output->openTableRow(propList);
    else if (strcmp(psName,"OpenTextBox")==0)
      m_output->openTextBox(propList);
    else if (strcmp(psName,"OpenUnorderedListLevel")==0)
      m_output->openUnorderedListLevel(propList);
    else
      ok=false;
    break;
  case 'S':
    if (strcmp(psName,"SetDocumentMetaData")==0)
      m_output->setDocumentMetaData(propList);
    else if (strcmp(psName,"StartDocument")==0)
      m_output->startDocument(propList);

    else
      ok=false;
    break;
  default:
    ok=false;
    break;
  }
  if (!ok) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetDecoder::insertElement: called with unexpected name %s\n", psName));
  }
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
