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

#include <sstream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFFont.hxx"

// font function
STOFFFont::~STOFFFont()
{
}

std::ostream &operator<<(std::ostream &o, STOFFFont const &font)
{
  o << font.m_propertyList.getPropString().cstr() << ",";
  if (!font.m_shadowColor.isBlack()) o << "shadow[color]=" << font.m_shadowColor << ",";
  if (font.m_hyphen) o << "hyphen,";
  if (font.m_softHyphen) o << "hyphen[soft],";
  if (font.m_hardBlank) o << "hard[blank],";
  if (font.m_lineBreak) o << "line[break],";
  return o;
}

int STOFFFont::cmp(STOFFFont const &font) const
{
  if (m_propertyList.getPropString() < font.m_propertyList.getPropString())
    return -1;
  if (m_propertyList.getPropString() > font.m_propertyList.getPropString())
    return 1;
  if (m_shadowColor < font.m_shadowColor)
    return -1;
  if (m_shadowColor > font.m_shadowColor)
    return 1;
  if (m_hyphen != font.m_hyphen)
    return m_hyphen ? 1 : -1;
  if (m_softHyphen != font.m_softHyphen)
    return m_softHyphen ? 1 : -1;
  if (m_hardBlank != font.m_hardBlank)
    return m_hardBlank ? 1 : -1;
  if (m_tab != font.m_tab)
    return m_tab ? 1 : -1;
  if (m_lineBreak != font.m_lineBreak)
    return m_lineBreak ? 1 : -1;
  return 0;
}

void STOFFFont::addTo(librevenge::RVNGPropertyList &pList) const
{
  librevenge::RVNGPropertyList::Iter i(m_propertyList);
  for (i.rewind(); i.next();) {
    if (i.child()) {
      STOFF_DEBUG_MSG(("STOFFFont::addTo: find unexpected property child\n"));
      pList.insert(i.key(), *i.child());
      continue;
    }
    pList.insert(i.key(), i()->clone());
  }
  if (!m_shadowColor.isBlack() && pList["fo:text-shadow"] && pList["fo:text-shadow"]->getStr()!="none") {
    // TODO: rewrite this code
    std::string what(pList["fo:text-shadow"]->getStr().cstr());
    if (what.empty() || what.find('#')!=std::string::npos) {
      STOFF_DEBUG_MSG(("STOFFFont::addTo: adding complex shadow color is not implemented\n"));
    }
    else {
      std::stringstream s;
      s << what << " " << m_shadowColor.str();
      pList.insert("fo:text-shadow", s.str().c_str());
    }
  }
}

void STOFFFont::checkForDefault(librevenge::RVNGPropertyList &propList)
{
  if (propList["librevenge:parent-display-name"]) return;
  if (!propList["style:font-name"])
    propList.insert("style:font-name", "Times");
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
