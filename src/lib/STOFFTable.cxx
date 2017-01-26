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
//! a comparaison structure used retrieve the rows and the columns
struct Compare {
  explicit Compare(int dim) : m_coord(dim) {}
  //! small structure to define a cell point
  struct Point {
    Point(int wh, STOFFCell const *cell, int cellId) : m_which(wh), m_cell(cell), m_cellId(cellId) {}
    float getPos(int coord) const
    {
      if (m_which)
        return m_cell->bdBox().max()[coord];
      return m_cell->bdBox().min()[coord];
    }
    /** returns the cells size */
    float getSize(int coord) const
    {
      return m_cell->bdBox().size()[coord];
    }
    /** the position of the point in the cell (0: LT, 1: RB) */
    int m_which;
    /** the cell */
    STOFFCell const *m_cell;
    //! the cell id ( used by compare)
    int m_cellId;
  };

  //! comparaison function
  bool operator()(Point const &c1, Point const &c2) const
  {
    float diffF = c1.getPos(m_coord)-c2.getPos(m_coord);
    if (diffF < 0) return true;
    if (diffF > 0) return false;
    int diff = c2.m_which - c1.m_which;
    if (diff) return (diff < 0);
    diffF = c1.m_cell->bdBox().size()[m_coord]
            - c2.m_cell->bdBox().size()[m_coord];
    if (diffF < 0) return true;
    if (diffF > 0) return false;
    return c1.m_cellId < c2.m_cellId;
  }

  //! the coord to compare
  int m_coord;
};
}

////////////////////////////////////////////////////////////
// STOFFTable
////////////////////////////////////////////////////////////

// destructor, ...
STOFFTable::~STOFFTable()
{
}

shared_ptr<STOFFCell> STOFFTable::get(int id)
{
  if (id < 0 || id >= int(m_cellsList.size())) {
    STOFF_DEBUG_MSG(("STOFFTable::get: cell %d does not exists\n",id));
    return shared_ptr<STOFFCell>();
  }
  return m_cellsList[size_t(id)];
}

void STOFFTable::addTablePropertiesTo(librevenge::RVNGPropertyList &propList) const
{
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
  if (mergeBorders())
    propList.insert("table:border-model","collapsing");

  if (!m_colsSize.empty())
    propList.insert("librevenge:table-columns", m_colsSize);
  if (m_tableWidth>0)
    propList.insert("style:width", double(m_tableWidth), librevenge::RVNG_POINT);
}

////////////////////////////////////////////////////////////
// try to send the table
bool STOFFTable::sendAsText(STOFFListenerPtr listener)
{
  if (!listener) return true;

  size_t nCells = m_cellsList.size();
  for (size_t i = 0; i < nCells; i++) {
    if (!m_cellsList[i]) continue;
    m_cellsList[i]->sendContent(listener, *this);
    listener->insertEOL();
  }
  return true;
}
