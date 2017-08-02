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

/** \file STOFFHeader.cxx
 * Implements STOFFHeader (document's type, version, kind)
 */

#include <string.h>
#include <iostream>

#include "libstaroffice_internal.hxx"

#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

#include "STOFFHeader.hxx"

STOFFHeader::STOFFHeader(int vers, STOFFDocument::Kind kind)
  : m_version(vers)
  , m_docKind(kind)
  , m_isEncrypted(false)
{
}

STOFFHeader::~STOFFHeader()
{
}

/**
 * So far, we have identified
 */
std::vector<STOFFHeader> STOFFHeader::constructHeader(STOFFInputStreamPtr input)
{
  std::vector<STOFFHeader> res;
  if (!input) return res;

  // ----------- now check data fork ------------
  if (!input->hasDataFork() || input->size() < 8)
    return res;
  if (!input->isStructured()) {
    if (input->readULong(4)==0x53474133) {// SGA3
      STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star graphic document\n"));
      res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_GRAPHIC));
    }
    return res;
  }

  if (input->getSubStreamByName("StarCalcDocument")) {
    STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star calc document\n"));
    res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_SPREADSHEET));
  }
  if (input->getSubStreamByName("StarChartDocument")) {
    STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star chart document, unimplemented\n"));
    res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_CHART));
    return res;
  }
  if (input->getSubStreamByName("StarDrawDocument") ||
      input->getSubStreamByName("StarDrawDocument3")) {
    STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star draw document\n"));
    res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_DRAW));
  }
  if (input->getSubStreamByName("StarImageDocument") || input->getSubStreamByName("StarImageDocument 4.0")) {
    STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star image document, unimplemented\n"));
    res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_BITMAP));
    return res;
  }
  if (input->getSubStreamByName("StarMathDocument")) {
    STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star math document, unimplemented\n"));
    res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_MATH));
    return res;
  }
  if (input->getSubStreamByName("StarWriterDocument")) {
    STOFF_DEBUG_MSG(("STOFFHeader::constructHeader: find a star writer document\n"));
    res.push_back(STOFFHeader(1, STOFFDocument::STOFF_K_TEXT));
  }
  return res;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
