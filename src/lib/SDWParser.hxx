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
 * Parser to convert sdw StarOffice document
 *
 */
#ifndef SDW_PARSER
#  define SDW_PARSER

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

#include "STOFFParser.hxx"

namespace SDWParserInternal
{
struct State;
}

class SWAttributeManager;
class SWZone;

/** \brief the main class to read a StarOffice sdw file
 *
 *
 *
 */
class SDWParser : public STOFFTextParser
{
  friend class SWAttributeManager;
public:
  //! constructor
  SDWParser(STOFFInputStreamPtr input, STOFFHeader *header);
  //! destructor
  virtual ~SDWParser();

  //! checks if the document header is correct (or not)
  bool checkHeader(STOFFHeader *header, bool strict=false);

  // the main parse function
  void parse(librevenge::RVNGTextInterface *documentInterface);

protected:
  //! inits all internal variables
  void init();

  //! creates the listener which will be associated to the document
  void createDocument(librevenge::RVNGTextInterface *documentInterface);

  //! parses the different OLE, ...
  bool createZones();

  //
  // low level
  //

  //!  the "persist elements" small structure: the list of object
  static bool readPersists(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! the document information
  static bool readDocumentInformation(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! the windows information
  static bool readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);

  //! the page style
  bool readSwPageStyleSheets(STOFFInputStreamPtr input, std::string const &fileName);
  //! the rulers?
  bool readSwNumRuleList(STOFFInputStreamPtr input, std::string const &fileName);

  //! the main zone
  bool readWriterDocument(STOFFInputStreamPtr input, std::string const &fileName);

protected:
  //! try a list of bookmark field : 'a'
  bool readSWBookmarkList(SWZone &zone);
  //! try to read some content : 'N'
  bool readSWContent(SWZone &zone);
  //! try to read a format zone : 'l' or 'o' or 'r'
  bool readSWFormatDef(SWZone &zone, char kind);
  //! try to read a number format zone : 'n'
  bool readSWNumberFormat(SWZone &zone);
  //! try to read a number formatter type : 'q'
  bool readSWNumberFormatterList(SWZone &zone);
  //! a simple rule : '0' or 'R'
  bool readSWNumRule(SWZone &zone, char kind);
  //! try to read a list of page style : 'p'
  bool readSWPageDef(SWZone &zone);
  //! try to read a list of redline : 'V' (list of 'R' list of 'D')
  bool readSWRedlineList(SWZone &zone);
  //! try to read some content : 'T'
  bool readSWTextZone(SWZone &zone);

  //
  // data
  //

  //! the state
  shared_ptr<SDWParserInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
