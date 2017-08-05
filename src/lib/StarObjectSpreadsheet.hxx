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
 * Parser to read StarOffice spreadsheet zones
 *
 */
#ifndef STAR_OBJECT_SPREADSHEET
#  define STAR_OBJECT_SPREADSHEET

#include <vector>

#include "libstaroffice_internal.hxx"
#include "StarObject.hxx"

namespace StarObjectSpreadsheetInternal
{
class Cell;
class Table;

struct State;
}

class StarAttribute;
class StarZone;
class STOFFPageSpan;

/** \brief the main class to read a StarOffice sdc file
 *
 *
 *
 */
class StarObjectSpreadsheet final : public StarObject
{
public:
  //! constructor
  StarObjectSpreadsheet(StarObject const &orig, bool duplicateState);
  //! destructor
  ~StarObjectSpreadsheet() final;
  //! try to parse the current object
  bool parse();
  //! try to send the spreadsheet
  bool send(STOFFSpreadsheetListenerPtr listener);
  /** try to send a spreadsheet row.

   \note this function does not call openSheetRow,closeSheetRow */
  bool sendRow(int table, int row, STOFFSpreadsheetListenerPtr listener);
  /** try to send a cell */
  bool sendCell(StarObjectSpreadsheetInternal::Cell &cell, StarAttribute *attrib, int table, int numRepeated, STOFFSpreadsheetListenerPtr listener);
  /** try to update the page span */
  bool updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages);
protected:
  //
  // data
  //
  //! try to read a spreadsheet zone: StarCalcDocument .sdc
  bool readCalcDocument(STOFFInputStreamPtr input, std::string const &fileName);
  //! try to read a spreadshet style zone: SfxStyleSheets
  bool readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &fileName);

  //! try to read a SCTable
  bool readSCTable(StarZone &zone, StarObjectSpreadsheetInternal::Table &table);
  //! try to read a SCColumn
  bool readSCColumn(StarZone &zone, StarObjectSpreadsheetInternal::Table &table, int column, long lastPos);
  //! try to read a list of data
  bool readSCData(StarZone &zone, StarObjectSpreadsheetInternal::Table &table, int column);

  //! try to read a change trak
  bool readSCChangeTrack(StarZone &zone, int version, long lastPos);
  //! try to read a dbData
  bool readSCDBData(StarZone &zone, int version, long lastPos);
  //! try to read a dbPivot
  bool readSCDBPivot(StarZone &zone, int version, long lastPos);
  //! try to read a matrix
  bool readSCMatrix(StarZone &zone, int version, long lastPos);
  //! try to read a query param
  bool readSCQueryParam(StarZone &zone, int version, long lastPos);
  //! try to read a SCOutlineArray
  bool readSCOutlineArray(StarZone &zone);

  //! the state
  std::shared_ptr<StarObjectSpreadsheetInternal::State> m_spreadsheetState;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
