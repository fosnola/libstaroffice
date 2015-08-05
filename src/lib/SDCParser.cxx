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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFOLEParser.hxx"

#include "StarFileManager.hxx"
#include "SWAttributeManager.hxx"
#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"
#include "StarZone.hxx"

#include "SDCParser.hxx"

/** Internal: the structures of a SDCParser */
namespace SDCParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDCParser
struct State {
  //! constructor
  State()
  {
  }
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDCParser::SDCParser() : m_state(new SDCParserInternal::State)
{
}

SDCParser::~SDCParser()
{
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool SDCParser::readChartDocument(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SWChartDocument");
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCChartDocument):";
  uint32_t nSize, n;
  uint16_t nVersion;
  *input >> nSize; // svdio.cxx: SdrDownCompat::openSubRecord
  *input >> nVersion; // schiocmp.cxx: SchIOCompat::SchIOCompat
  f << "vers=" << nVersion << ",";
  // sch_chartdoc.cxx: operator>> ChartModel
  *input >> n;
  if (n>1) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: bad version\n"));
    ascFile.addPos(0);
    f << "###n=" << n << ",";
    ascFile.addNote(f.str().c_str());
  }
  long pos=input->tell();
  if (n==1) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(pos+74, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());

  input->seek((long) nSize, librevenge::RVNG_SEEK_SET);
  pos=input->tell();
  f.str("");
  f << "SCChartDocument:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
#if 0
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWChartDocument:##extra");
  }
#endif
  return true;
}
////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////



// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
