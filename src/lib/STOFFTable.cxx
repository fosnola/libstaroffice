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

  size_t nCols = m_colsSize.size();
  float tableWidth = 0;
  librevenge::RVNGPropertyListVector columns;
  for (size_t c = 0; c < nCols; ++c) {
    librevenge::RVNGPropertyList column;
    column.insert("style:column-width", double(m_colsSize[c]), librevenge::RVNG_POINT);
    columns.append(column);
    tableWidth += m_colsSize[c];
  }
  propList.insert("librevenge:table-columns", columns);
  if (tableWidth>0)
    propList.insert("style:width", double(tableWidth), librevenge::RVNG_POINT);
}

////////////////////////////////////////////////////////////
// build the table structure
bool STOFFTable::buildStructures()
{
  if (m_setData&CellPositionBit)
    return true;
  if ((m_setData&BoxBit)==0) {
    STOFF_DEBUG_MSG(("STOFFTable::buildStructures: can not reconstruct cellule position if their boxes are not set\n"));
    return false;
  }

  size_t nCells = m_cellsList.size();
  std::vector<float> listPositions[2];
  for (int dim = 0; dim < 2; dim++) {
    STOFFTableInternal::Compare compareFunction(dim);
    std::set<STOFFTableInternal::Compare::Point,
        STOFFTableInternal::Compare> set(compareFunction);
    for (size_t c = 0; c < nCells; ++c) {
      set.insert(STOFFTableInternal::Compare::Point(0, m_cellsList[c].get(), int(c)));
      set.insert(STOFFTableInternal::Compare::Point(1, m_cellsList[c].get(), int(c)));
    }

    std::vector<float> positions;
    std::set<STOFFTableInternal::Compare::Point,
        STOFFTableInternal::Compare>::iterator it = set.begin();
    float maxPosiblePos=0;
    int actCell = -1;
    for (; it != set.end(); ++it) {
      float pos = it->getPos(dim);
      if (actCell < 0 || pos > maxPosiblePos) {
        actCell++;
        positions.push_back(pos);
        maxPosiblePos = float(pos+2.0f); // 2 pixel ok
      }
      if (it->m_which == 0 && it->getPos(dim)-2.0f < maxPosiblePos)
        maxPosiblePos = float((it->getPos(dim)+pos)/2.f);
    }
    listPositions[dim] = positions;
  }
  for (size_t c = 0; c < nCells; ++c) {
    int cellPos[2], spanCell[2];
    for (int dim = 0; dim < 2; dim++) {
      float pt[2] = { m_cellsList[c]->bdBox().min()[dim],
                      m_cellsList[c]->bdBox().max()[dim]
                    };
      std::vector<float> &pos = listPositions[dim];
      size_t numPos = pos.size();
      size_t i = 0;
      while (i+1 < numPos && pos[i+1] < pt[0])
        i++;
      if (i+1 < numPos && (pos[i]+pos[i+1])/2 < pt[0])
        i++;
      if (i+1 > numPos) {
        STOFF_DEBUG_MSG(("STOFFTable::buildStructures: impossible to find cell position !!!\n"));
        return false;
      }
      cellPos[dim] = int(i);
      while (i+1 < numPos && pos[i+1] < pt[1])
        i++;
      if (i+1 < numPos && (pos[i]+pos[i+1])/2 < pt[1])
        i++;
      spanCell[dim] = int(i)-cellPos[dim];
      if (spanCell[dim]==0 &&
          (m_cellsList[c]->bdBox().size()[dim] < 0 || m_cellsList[c]->bdBox().size()[dim] > 0)) {
        STOFF_DEBUG_MSG(("STOFFTable::buildStructures: impossible to find span number !!!\n"));
        return false;
      }
      if (spanCell[dim] > 1 &&
          pos[size_t(cellPos[dim])]+2.0f > pos[size_t(cellPos[dim]+1)]) {
        spanCell[dim]--;
        cellPos[dim]++;
      }
    }
    m_cellsList[c]->setPosition(STOFFVec2i(cellPos[0], cellPos[1]));
    m_cellsList[c]->getCellStyle().m_numberCellSpanned=STOFFVec2i(spanCell[0], spanCell[1]);
  }
  m_setData |= CellPositionBit;
  // finally update the row/col size
  for (int dim = 0; dim < 2; dim++) {
    std::vector<float> const &pos = listPositions[dim];
    size_t numPos = pos.size();
    if (!numPos) continue;
    std::vector<float> &res = (dim==0) ? m_colsSize : m_rowsSize;
    res.resize(numPos-1);
    for (size_t i = 0; i < numPos-1; i++)
      res[i] = pos[i+1]-pos[i];
  }
  m_setData |= TableDimBit;
  return true;
}


bool STOFFTable::buildPosToCellId()
{
  if (m_setData&TablePosToCellBit)
    return true;
  if ((m_setData&CellPositionBit)==0) {
    STOFF_DEBUG_MSG(("STOFFTable::buildPosToCellId: can not reconstruct cellule position if their boxes are not set\n"));
    return false;
  }
  m_posToCellId.resize(0);

  size_t nCells = m_cellsList.size();
  m_numRows=(m_setData&TableDimBit) ? m_rowsSize.size() : 0;
  m_numCols=(m_setData&TableDimBit) ? m_colsSize.size() : 0;
  if ((m_setData&TableDimBit)==0) {
    // m_numCols, m_numRows is not updated, we must compute it
    m_numCols = 0;
    m_numRows = 0;
    for (size_t c = 0; c < nCells; ++c) {
      if (!m_cellsList[c]) continue;
      STOFFVec2i const &lastPos=m_cellsList[c]->position() +
                                m_cellsList[c]->getCellStyle().m_numberCellSpanned;
      if (lastPos[0]>int(m_numCols)) m_numCols=size_t(lastPos[0]);
      if (lastPos[1]>int(m_numRows)) m_numRows=size_t(lastPos[1]);
    }
  }
  if (!m_numCols || !m_numRows)
    return false;
  m_posToCellId.resize(m_numCols*m_numRows, -1);
  for (size_t c = 0; c < nCells; ++c) {
    if (!m_cellsList[c]) continue;
    STOFFVec2i const &pos=m_cellsList[c]->position();
    STOFFVec2i lastPos=pos+m_cellsList[c]->getCellStyle().m_numberCellSpanned;
    for (int x = pos[0]; x < lastPos[0]; x++) {
      for (int y = pos[1]; y < lastPos[1]; y++) {
        int tablePos = getCellIdPos(x,y);
        if (tablePos<0) {
          STOFF_DEBUG_MSG(("STOFFTable::buildPosToCellId: the position is bad!!!\n"));
          return false;
        }
        if (m_posToCellId[size_t(tablePos)] != -1) {
          STOFF_DEBUG_MSG(("STOFFTable::buildPosToCellId: cells is used!!!\n"));
          return false;
        }
        if (x == pos[0] && y == pos[1])
          m_posToCellId[size_t(tablePos)] = int(c);
        else
          m_posToCellId[size_t(tablePos)] = -2;
      }
    }
  }
  m_setData |= TablePosToCellBit;
  return true;
}

bool STOFFTable::buildDims()
{
  if (m_setData&TableDimBit)
    return true;
  if ((m_setData&CellPositionBit)==0)
    return false;
  if ((m_setData&BoxBit)==0 && (m_setData&SizeBit)==0) {
    STOFF_DEBUG_MSG(("STOFFTable::buildDims: not enough information to reconstruct dimension\n"));
    return false;
  }

  if (m_numRows==0 || m_numCols==0) {
    STOFF_DEBUG_MSG(("STOFFTable::buildDims: can not compute the number of columns/row\n"));
    return false;
  }

  std::vector<float> colLimit(m_numCols+1,0);
  std::vector<bool> isFixed(m_numCols+1, (m_setData&BoxBit));
  for (int c = 0; c < int(m_numCols); ++c) {
    for (int r = 0; r < int(m_numRows); ++r) {
      int cPos = getCellIdPos(c, r);
      if (cPos<0 || m_posToCellId[size_t(cPos)]<0) continue;
      shared_ptr<STOFFCell> cell=m_cellsList[size_t(m_posToCellId[size_t(cPos)])];
      if (!cell) continue;
      STOFFVec2i const &pos=cell->position();
      STOFFVec2i lastPos=pos+cell->getCellStyle().m_numberCellSpanned;
      if (m_setData&BoxBit) {
        colLimit[size_t(pos[0])] = cell->bdBox()[0][0];
        colLimit[size_t(lastPos[0])] = cell->bdBox()[1][0];
      }
      else if (cell->bdSize()[0]>=0) {
        colLimit[size_t(lastPos[0])] = colLimit[size_t(pos[0])]+cell->bdSize()[0];
        isFixed[size_t(lastPos[0])]=true;
      }
      else if (!isFixed[size_t(lastPos[0])])
        colLimit[size_t(lastPos[0])] = colLimit[size_t(pos[0])]-cell->bdSize()[0];
    }
    if (colLimit[size_t(c)+1]<=colLimit[size_t(c)]) {
      STOFF_DEBUG_MSG(("STOFFTable::buildDims: oops can not find the size of col %d\n", c));
      return false;
    }
  }
  m_colsSize.resize(m_numCols);
  for (size_t c = 0; c < m_numCols; ++c) {
    if (isFixed[c+1])
      m_colsSize[c]=colLimit[c+1]-colLimit[c];
    else
      m_colsSize[c]=colLimit[c]-colLimit[c+1];
  }

  std::vector<float> rowLimit(m_numRows+1,0);
  isFixed.resize(0);
  isFixed.resize(m_numRows+1,(m_setData&BoxBit));
  for (int r = 0; r < int(m_numRows); ++r) {
    for (int c = 0; c < int(m_numCols); ++c) {
      int cPos = getCellIdPos(c, r);
      if (cPos<0 || m_posToCellId[size_t(cPos)]<0) continue;
      shared_ptr<STOFFCell> cell=m_cellsList[size_t(m_posToCellId[size_t(cPos)])];
      if (!cell) continue;
      STOFFVec2i const &pos=cell->position();
      STOFFVec2i lastPos=pos+cell->getCellStyle().m_numberCellSpanned;
      if (m_setData&BoxBit) {
        rowLimit[size_t(pos[1])] = cell->bdBox()[0][1];
        rowLimit[size_t(lastPos[1])] = cell->bdBox()[1][1];
      }
      else if (cell->bdSize()[1]>=0) {
        rowLimit[size_t(lastPos[1])] = rowLimit[size_t(pos[1])]+cell->bdSize()[1];
        isFixed[size_t(lastPos[1])]=true;
      }
      else if (!isFixed[size_t(lastPos[1])])
        rowLimit[size_t(lastPos[1])] = rowLimit[size_t(pos[1])]-cell->bdSize()[1];
    }
    if (rowLimit[size_t(r)+1]<=rowLimit[size_t(r)]) {
      STOFF_DEBUG_MSG(("STOFFTable::buildDims: oops can not find the size of row %d\n", r));
      return false;
    }
  }
  m_rowsSize.resize(m_numRows);
  for (size_t r = 0; r < m_numRows; ++r) {
    if (isFixed[r+1])
      m_rowsSize[r]=rowLimit[r+1]-rowLimit[r];
    else
      m_rowsSize[r]=rowLimit[r]-rowLimit[r+1];
  }
  m_setData |= TableDimBit;
  return true;
}

////////////////////////////////////////////////////////////
// try to send the table
bool STOFFTable::updateTable()
{
  if ((m_setData&CellPositionBit)==0 && !buildStructures())
    return false;
  if ((m_setData&TablePosToCellBit)==0 && !buildPosToCellId())
    return false;
  if (!m_numCols || !m_numRows)
    return false;
  if ((m_givenData&TableDimBit)==0 && !buildDims())
    return false;
  return true;
}

bool STOFFTable::sendTable(STOFFListenerPtr listener)
{
  if (!updateTable())
    return false;
  if (!listener)
    return true;
  listener->openTable(*this);
  for (size_t r = 0; r < m_numRows; ++r) {
    listener->openTableRow(m_rowsSize[r], librevenge::RVNG_POINT);
    for (size_t c = 0; c < m_numCols; ++c) {
      int tablePos = getCellIdPos(int(c), int(r));
      if (tablePos<0)
        continue;
      int id = m_posToCellId[size_t(tablePos)];
      if (id == -1)
        listener->addEmptyTableCell(STOFFVec2i(int(c), int(r)));
      if (id < 0) continue;
      m_cellsList[size_t(id)]->send(listener, *this);
    }
    listener->closeTableRow();
  }
  listener->closeTable();

  return true;
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
