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

#ifndef STOFF_TABLE
#  define STOFF_TABLE

#include <iostream>
#include <vector>

#include "libstaroffice_internal.hxx"

#include "STOFFCell.hxx"

/** a class used to recreate the table structure using cell informations, .... */
class STOFFTable
{
public:
  //! an enum used to indicate what the list of entries which are filled
  enum DataSet {
    CellPositionBit=1, BoxBit=2, SizeBit=4, TableDimBit=8, TablePosToCellBit=0x10
  };
  /** an enum do define the table alignment.

  \note Paragraph means using the default alignment, left page alignment and use left margin */
  enum Alignment {
    Paragraph, Left, Center, Right
  };
  //! the constructor
  explicit STOFFTable(uint32_t givenData=BoxBit) :
    m_givenData(givenData), m_setData(givenData), m_mergeBorders(true), m_cellsList(),
    m_numRows(0), m_numCols(0), m_rowsSize(), m_colsSize(), m_alignment(Paragraph), m_leftMargin(0), m_rightMargin(0),
    m_posToCellId() {}

  //! the destructor
  virtual ~STOFFTable();

  //! add a new cells
  void add(shared_ptr<STOFFCell> cell)
  {
    if (!cell) {
      STOFF_DEBUG_MSG(("STOFFTable::add: must be called with a cell\n"));
      return;
    }
    m_cellsList.push_back(cell);
  }
  //! returns true if we need to merge borders
  bool mergeBorders() const
  {
    return m_mergeBorders;
  }
  //! sets the merge borders' value
  bool setMergeBorders(bool val)
  {
    return m_mergeBorders=val;
  }
  /** defines the current alignment
      \note: leftMargin,rightMargin are given in Points */
  void setAlignment(Alignment align, float leftMargin=0, float rightMargin=0)
  {
    m_alignment = align;
    m_leftMargin = leftMargin;
    m_rightMargin = rightMargin;
  }
  //! returns the number of cell
  int numCells() const
  {
    return int(m_cellsList.size());
  }
  /** returns the row size if defined (in point) */
  std::vector<float> const &getRowsSize() const
  {
    return m_rowsSize;
  }
  /** define the row size (in point) */
  void setRowsSize(std::vector<float> const &rSize)
  {
    m_rowsSize=rSize;
  }
  /** returns the columns size if defined (in point) */
  std::vector<float> const &getColsSize() const
  {
    return m_colsSize;
  }
  /** define the columns size (in point) */
  void setColsSize(std::vector<float> const &cSize)
  {
    m_colsSize=cSize;
  }

  //! returns the i^th cell
  shared_ptr<STOFFCell> get(int id);

  /** try to build the table structures */
  bool updateTable();

  /** try to send the table

  Note: either send the table ( and returns true ) or do nothing.
   */
  bool sendTable(STOFFListenerPtr listener);

  /** try to send the table as basic text */
  bool sendAsText(STOFFListenerPtr listener);

  // interface with the content listener

  //! adds the table properties to propList
  void addTablePropertiesTo(librevenge::RVNGPropertyList &propList) const;

protected:
  //! convert a cell position in a posToCellId's position
  int getCellIdPos(int col, int row) const
  {
    if (col<0||col>=int(m_numCols))
      return -1;
    if (row<0||row>=int(m_numRows))
      return -1;
    return col*int(m_numRows)+row;
  }
  //! create the correspondance list, ...
  bool buildStructures();
  /** compute the rows and the cells size */
  bool buildDims();
  /** a function which fills to posToCellId vector using the cell position */
  bool buildPosToCellId();

protected:
  /** a int to indicate what data are given in entries*/
  uint32_t m_givenData;
  /** a int to indicate what data are been reconstruct*/
  uint32_t m_setData;
  /** do we need to merge cell borders ( default yes) */
  bool m_mergeBorders;
  /** the list of cells */
  std::vector<shared_ptr<STOFFCell> > m_cellsList;
  /** the number of rows ( set by buildPosToCellId ) */
  size_t m_numRows;
  /** the number of cols ( set by buildPosToCellId ) */
  size_t m_numCols;
  /** the final row  size (in point) */
  std::vector<float> m_rowsSize;
  /** the final col size (in point) */
  std::vector<float> m_colsSize;
  /** the table alignment */
  Alignment m_alignment;
  /** the left margin in point */
  float m_leftMargin;
  /** the right margin in point */
  float m_rightMargin;

  /** a vector used to store an id corresponding to each cell */
  std::vector<int> m_posToCellId;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
