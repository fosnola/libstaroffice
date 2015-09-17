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
 * Parser to convert sdc StarOffice document
 *
 */
#ifndef SDC_PARSER
#  define SDC_PARSER

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

#include "STOFFParser.hxx"

namespace SDCParserInternal
{
class Table;

struct State;
}

class StarAttribute;
class StarDocument;
class StarZone;

/** \brief the main class to read a StarOffice sdc file
 *
 *
 *
 */
class SDCParser
{
public:
  //! constructor
  SDCParser();
  //! destructor
  virtual ~SDCParser();

  //! try to read a chart zone: StarCalcDocument .sdc
  bool readCalcDocument(STOFFInputStreamPtr input, std::string const &fileName, StarDocument &document);
  //! try to read a spreadshet style zone: SfxStyleSheets
  bool readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &fileName, StarDocument &document);

protected:
  //
  // data
  //
  //! try to read a SCTable
  bool readSCTable(StarZone &zone, SDCParserInternal::Table &table, StarDocument &doc);
  //! try to read a SCColumn
  bool readSCColumn(StarZone &zone, SDCParserInternal::Table &table, StarDocument &doc, int column, long lastPos);
  //! try to read a list of data
  bool readSCData(StarZone &zone, SDCParserInternal::Table &table, StarDocument &doc, int column);

  //! try to read a change trak
  bool readSCChangeTrack(StarZone &zone, int version, long lastPos);
  //! try to read a dbData
  bool readSCDBData(StarZone &zone, int version, long lastPos);
  //! try to read a dbPivot
  bool readSCDBPivot(StarZone &zone, int version, long lastPos);
  //! try to read a formula
  bool readSCFormula(StarZone &zone, STOFFVec2i const &pos, int version, long lastPos);
  //! try to read a formula(v3)
  bool readSCFormula3(StarZone &zone, STOFFVec2i const &pos, int version, long lastPos);
  //! try to read a matrix
  bool readSCMatrix(StarZone &zone, int version, long lastPos);
  //! try to read a query param
  bool readSCQueryParam(StarZone &zone, int version, long lastPos);
  //! try to read a token in a formula
  bool readSCTokenInFormula(StarZone &zone, STOFFVec2i const &pos, int version, long lastPos, libstoff::DebugStream &f);
  //! try to read a token in a formula (v3)
  bool readSCTokenInFormula3(StarZone &zone, STOFFVec2i const &pos, bool &endData, long lastPos, libstoff::DebugStream &f);
  //! try to read a SCOutlineArray
  bool readSCOutlineArray(StarZone &zone);

  //! the state
  shared_ptr<SDCParserInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
