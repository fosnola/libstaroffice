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

#include "STOFFList.hxx"

#include "StarItemPool.hxx"
#include "StarObjectModel.hxx"
#include "StarObjectNumericRuler.hxx"

#include "SWFieldManager.hxx"

StarState::GlobalState::~GlobalState()
{
}

StarState::StarState(StarItemPool const *pool, StarObject &object)
  : m_global(new StarState::GlobalState(pool, object, pool ? pool->getRelativeUnit() : 0.05))
  , m_styleName("")
  , m_break(0)
  , m_cell()
  , m_frame()
  , m_graphic()
  , m_paragraph()
  , m_font()
  , m_content(false)
  , m_flyCnt(false)
  , m_footnote(false)
  , m_headerFooter(false)
  , m_link("")
  , m_refMark("")
  , m_field()
{
}

StarState::StarState(StarState::GlobalState const &global)
  : m_global(new StarState::GlobalState(global.m_pool, global.m_object, global.m_relativeUnit))
  , m_styleName("")
  , m_break(0)
  , m_cell()
  , m_frame()
  , m_graphic()
  , m_paragraph()
  , m_font()
  , m_content(false)
  , m_flyCnt(false)
  , m_footnote(false)
  , m_headerFooter(false)
  , m_link("")
  , m_refMark("")
  , m_field()
{
  m_global->m_objectModel=global.m_objectModel;
}
StarState::~StarState()
{
}

void StarState::reinitializeLineData()
{
  m_font=STOFFFont();
  m_content=m_flyCnt=m_footnote=false;
  m_link=m_refMark="";
  m_field.reset();
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
