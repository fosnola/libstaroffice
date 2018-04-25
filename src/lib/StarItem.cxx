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

#include "StarAttribute.hxx"

#include "StarItem.hxx"

bool StarItemSet::add(std::shared_ptr<StarItem> item)
{
  if (!item) return false;
  if (m_whichToItemMap.find(item->m_which)!=m_whichToItemMap.end()) {
    STOFF_DEBUG_MSG(("StarItemSet::add: oops, item %d already exists\n", item->m_which));
  }
  m_whichToItemMap[item->m_which]=item;
  return true;
}

std::string StarItemSet::printChild() const
{
  if (m_whichToItemMap.empty()) return "";
  libstoff::DebugStream o;
  o << "Attrib=[";
  for (auto const &it : m_whichToItemMap) {
    if (!it.second || !it.second->m_attribute) {
      o << "_,";
      continue;
    }
    it.second->m_attribute->printData(o);
    o << ",";
  }
  o << "],";
  return o.str();
}

std::ostream &operator<<(std::ostream &o, StarItemStyle const &style)
{
  for (int i=0; i<4; ++i) {
    if (style.m_names[i].empty()) continue;
    static char const *wh[]= {"name","parent","follow","help"};
    o << wh[i] << "=" << style.m_names[i].cstr() << ",";
  }
  switch (style.m_family&0xff) {
  case 0:
    break;
  case 1:
    o << "char[family],";
    break;
  case 2:
    o << "para[family],";
    break;
  case 4:
    o << "frame[family],";
    break;
  case 8:
    o << "page[family],";
    break;
  case 0x10:
    o << "pseudo[family],";
    break;
  case 0xFE:
    o << "*[family],";
    break;
  default:
    STOFF_DEBUG_MSG(("StarItemPoolInternal::StarItemStyle::operator<< unexpected family\n"));
    o << "###family=" << std::hex << (style.m_family&0xff) << std::dec << ",";
    break;
  }
  if (style.m_family&0xFF00) // find 0xaf
    o << "#family[high]=" << std::hex << (style.m_family>>8) << std::dec << ",";
  if (style.m_mask) o << "mask=" << std::hex << style.m_mask << std::dec << ",";
  if (style.m_helpId) o << "help[id]=" << style.m_helpId << ",";
#if 0
  o << style.m_itemSet.printChild();
#endif
  return o;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
