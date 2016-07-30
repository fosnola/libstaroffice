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

#include <librevenge/librevenge.h>

#include "StarState.hxx"

#include "SWFieldManager.hxx"

StarState::StarState(StarState const &orig) :
  m_pool(orig.m_pool), m_object(orig.m_object), m_styleName(orig.m_styleName),
  m_page(orig.m_page), m_pageName(orig.m_pageName), m_pageNameList(orig.m_pageNameList), m_pageZone(orig.m_pageZone), m_pageOccurence(orig.m_pageOccurence),
  m_break(orig.m_break),
  m_cell(orig.m_cell),
  m_graphic(orig.m_graphic), m_paragraph(orig.m_paragraph),
  m_font(orig.m_font), m_content(orig.m_content), m_footnote(orig.m_footnote), m_link(orig.m_link), m_refMark(orig.m_refMark), m_field(orig.m_field),
  m_relativeUnit(orig.m_relativeUnit)
{
}

StarState::~StarState()
{
}

void StarState::reinitializeLineData()
{
  m_break=0;
  m_font=STOFFFont();
  m_content=m_footnote=false;
  m_link=m_refMark="";
  m_field.reset();
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
