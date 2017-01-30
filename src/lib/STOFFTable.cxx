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

/*
 * Structure to store and construct a table from an unstructured list
 * of cell
 *
 */

#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFCell.hxx"
#include "STOFFListener.hxx"
#include "STOFFPosition.hxx"

#include "STOFFTable.hxx"

/** Internal: the structures of a STOFFTable */
namespace STOFFTableInternal
{
}

////////////////////////////////////////////////////////////
// STOFFTable
////////////////////////////////////////////////////////////

// destructor, ...
STOFFTable::~STOFFTable()
{
}

void STOFFTable::addTablePropertiesTo(librevenge::RVNGPropertyList &pList) const
{
  if (!m_propertyList["table:border-model"])
    pList.insert("table:border-model","collapsing");
  librevenge::RVNGPropertyList::Iter i(m_propertyList);
  for (i.rewind(); i.next();) {
    if (i.child()) {
      pList.insert(i.key(), *i.child());
      continue;
    }
    pList.insert(i.key(), i()->clone());
  }
#if 0
  switch (m_alignment) {
  case Paragraph:
    break;
  case Left:
    propList.insert("table:align", "left");
    propList.insert("fo:margin-left", double(m_leftMargin), librevenge::RVNG_POINT);
    break;
  case Center:
    propList.insert("table:align", "center");
    break;
  case Right:
    propList.insert("table:align", "right");
    propList.insert("fo:margin-right", double(m_rightMargin), librevenge::RVNG_POINT);
    break;
  default:
    break;
  }
#endif
}

