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
 * Parser to read StarOffice formula in cells
 *
 */
#ifndef STAR_CELL_FORMULA
#  define STAR_CELL_FORMULA

#include <vector>

#include "libstaroffice_internal.hxx"
#include "STOFFCell.hxx"

class StarZone;

namespace StarCellFormulaInternal
{
struct Token;
}
/** \brief the main class to read a cell formula
 *
 *
 *
 */
class StarCellFormula
{
public:
  //! constructor
  StarCellFormula() {}
  //! destructor
  ~StarCellFormula() {}
  //! try to read a formula
  static bool readSCFormula(StarZone &zone, STOFFCellContent &content, int version, long lastPos);
  //! try to read a formula(v3)
  static bool readSCFormula3(StarZone &zone, STOFFCellContent &content, int version, long lastPos);
  //! update the different formula(knowing the list of sheet names and the cell's sheetId)
  static void updateFormula(STOFFCellContent &content, std::vector<librevenge::RVNGString> const &sheetNames, int cellSheetId);
protected:
  //
  // data
  //
  //! try to read a token in a formula
  static bool readSCToken(StarZone &zone, StarCellFormulaInternal::Token &token, int version, long lastPos);
  //! try to read a token in a formula (v3)
  static bool readSCToken3(StarZone &zone, StarCellFormulaInternal::Token &token, bool &endData, long lastPos);
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
