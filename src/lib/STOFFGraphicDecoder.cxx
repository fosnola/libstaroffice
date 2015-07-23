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

#include "STOFFGraphicDecoder.hxx"

void STOFFGraphicDecoder::insertElement(const char *psName)
{
  if (!m_output) return;
  int len=psName ? int(strlen(psName)) : 0;
  if (!len) {
    STOFF_DEBUG_MSG(("STOFFGraphicDecoder::insertElement: called without name\n"));
    return;
  }

  bool ok=true;
  switch (psName[0]) {
  case 'C':
    if (len>=6 && strncmp(psName,"Close",5)==0) {
      psName+=5;
      if (strcmp(psName,"Group")==0)
        m_output->closeGroup();
      else if (strcmp(psName,"Link")==0)
        m_output->closeLink();
      else if (strcmp(psName,"ListElement")==0)
        m_output->closeListElement();
      else if (strcmp(psName,"OrderedListLevel")==0)
        m_output->closeOrderedListLevel();
      else if (strcmp(psName,"Paragraph")==0)
        m_output->closeParagraph();
      else if (strcmp(psName,"Span")==0)
        m_output->closeSpan();
      else if (strcmp(psName,"TableCell")==0)
        m_output->closeTableCell();
      else if (strcmp(psName,"TableRow")==0)
        m_output->closeTableRow();
      else if (strcmp(psName,"UnorderedListLevel")==0)
        m_output->closeUnorderedListLevel();
      else
        ok=false;
    }
    else
      ok=false;
    break;
  case 'E':
    if (len>=4 && strncmp(psName,"End",3)==0) {
      psName+=3;
      if (strcmp(psName,"Document")==0)
        m_output->endDocument();
      else if (strcmp(psName,"EmbeddedGraphics")==0)
        m_output->endEmbeddedGraphics();
      else if (strcmp(psName,"Layer")==0)
        m_output->endLayer();
      else if (strcmp(psName,"MasterPage")==0)
        m_output->endMasterPage();
      else if (strcmp(psName,"Page")==0)
        m_output->endPage();
      else if (strcmp(psName,"TableObject")==0)
        m_output->endTableObject();
      else if (strcmp(psName,"TextObject")==0)
        m_output->endTextObject();
      else
        ok=false;
    }
    else
      ok=false;
    break;
  case 'I':
    if (len>=7 && strncmp(psName,"Insert",6)==0) {
      psName+=6;
      if (strcmp(psName,"LineBreak")==0)
        m_output->insertLineBreak();
      else if (strcmp(psName,"Space")==0)
        m_output->insertSpace();
      else if (strcmp(psName,"Tab")==0)
        m_output->insertTab();
      else
        ok=false;
    }
    else
      ok=false;
    break;
  default:
    ok=false;
    break;
  }
  if (!ok) {
    STOFF_DEBUG_MSG(("STOFFGraphicDecoder::insertElement: called with unexpected name %s\n", psName));
  }
}

void STOFFGraphicDecoder::insertElement(const char *psName, const librevenge::RVNGPropertyList &propList)
{
  if (!m_output) return;
  int len=psName ? int(strlen(psName)) : 0;
  if (!len) {
    STOFF_DEBUG_MSG(("STOFFGraphicDecoder::insertElement: called without any name\n"));
    return;
  }

  bool ok=true;
  switch (psName[0]) {
  case 'D':
    if (len>=7 && strncmp(psName,"Define",6)==0) {
      psName+=6;
      if (strcmp(psName,"CharacterStyle")==0)
        m_output->defineCharacterStyle(propList);
      else if (strcmp(psName,"EmbeddedFont")==0)
        m_output->defineEmbeddedFont(propList);
      else if (strcmp(psName,"ParagraphStyle")==0)
        m_output->defineParagraphStyle(propList);
      else
        ok=false;
    }
    else if (len>=5 && strncmp(psName,"Draw",4)==0) {
      psName+=4;
      if (strcmp(psName,"Connector")==0)
        m_output->drawConnector(propList);
      else if (strcmp(psName,"Ellipse")==0)
        m_output->drawEllipse(propList);
      else if (strcmp(psName,"GraphicObject")==0)
        m_output->drawGraphicObject(propList);
      else if (strcmp(psName,"Path")==0)
        m_output->drawPath(propList);
      else if (strcmp(psName,"Polygon")==0)
        m_output->drawPolygon(propList);
      else if (strcmp(psName,"Polyline")==0)
        m_output->drawPolyline(propList);
      else if (strcmp(psName,"Rectangle")==0)
        m_output->drawRectangle(propList);
      else
        ok=false;
    }
    else
      ok=false;
    break;
  case 'I':
    if (len>=7 && strncmp(psName,"Insert",6)==0) {
      psName+=6;
      if (strcmp(psName,"CoveredTableCell")==0)
        m_output->insertCoveredTableCell(propList);
      else if (strcmp(psName,"Field")==0)
        m_output->insertField(propList);
      else
        ok=false;
    }
    else
      ok=false;
    break;
  case 'O':
    if (len>=5 && strncmp(psName,"Open",4)==0) {
      psName+=4;
      if (strcmp(psName,"Group")==0)
        m_output->openGroup(propList);
      else if (strcmp(psName,"Link")==0)
        m_output->openLink(propList);
      else if (strcmp(psName,"ListElement")==0)
        m_output->openListElement(propList);
      else if (strcmp(psName,"OrderedListLevel")==0)
        m_output->openOrderedListLevel(propList);
      else if (strcmp(psName,"Paragraph")==0)
        m_output->openParagraph(propList);
      else if (strcmp(psName,"Span")==0)
        m_output->openSpan(propList);
      else if (strcmp(psName,"TableCell")==0)
        m_output->openTableCell(propList);
      else if (strcmp(psName,"TableRow")==0)
        m_output->openTableRow(propList);
      else if (strcmp(psName,"UnorderedListLevel")==0)
        m_output->openUnorderedListLevel(propList);
      else
        ok=false;
    }
    else
      ok=false;
    break;
  case 'S':
    if (len>=4 && strncmp(psName,"Set",3)==0) {
      psName+=3;
      if (strcmp(psName,"DocumentMetaData")==0)
        m_output->setDocumentMetaData(propList);
      else if (strcmp(psName,"Style")==0)
        m_output->setStyle(propList);
      else
        ok=false;
    }
    else if (len>=6 && strncmp(psName,"Start",5)==0) {
      psName+=5;
      if (strcmp(psName,"Document")==0)
        m_output->startDocument(propList);
      else if (strcmp(psName,"EmbeddedGraphics")==0)
        m_output->startEmbeddedGraphics(propList);
      else if (strcmp(psName,"Layer")==0)
        m_output->startLayer(propList);
      else if (strcmp(psName,"MasterPage")==0)
        m_output->startMasterPage(propList);
      else if (strcmp(psName,"Page")==0)
        m_output->startPage(propList);
      else if (strcmp(psName,"TableObject")==0)
        m_output->startTableObject(propList);
      else if (strcmp(psName,"TextObject")==0)
        m_output->startTextObject(propList);
      else
        ok=false;
    }
    else
      ok=false;
    break;
  default:
    ok=false;
    break;
  }
  if (!ok) {
    STOFF_DEBUG_MSG(("STOFFGraphicDecoder::insertElement: called with unexpected name %s\n", psName));
  }
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
