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
class StarZone;

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

  //! the page style
  bool readSwPageStyleSheets(STOFFInputStreamPtr input, std::string const &fileName);
  //! the rulers?
  bool readSwNumRuleList(STOFFInputStreamPtr input, std::string const &fileName);

  //! the main zone
  bool readWriterDocument(STOFFInputStreamPtr input, std::string const &fileName);

protected:
  //! try a list of bookmark field : 'a'
  bool readSWBookmarkList(StarZone &zone);
  //! try to read some content : 'N'
  bool readSWContent(StarZone &zone);
  //! try to read a DBName zone : 'D'
  bool readSWDBName(StarZone &zone);
  //! try to read a dictionary table : 'j'
  bool readSWDictionary(StarZone &zone);
  //! try to read a endnode node : '4'
  bool readSWEndNoteInfo(StarZone &zone);
  //! try to read a footnode node : '1'
  bool readSWFootNoteInfo(StarZone &zone);
  //! try to read a OLE node : 'g'
  bool readSWGraphNode(StarZone &zone);
  //! try to read a image map zone : 'X'
  bool readSWImageMap(StarZone &zone);
  //! try to read a layout information zone : 'Y'
  bool readSWLayoutInfo(StarZone &zone);
  //! try to read a layout subinformation zone : 0xd2 or 0xd7
  bool readSWLayoutSub(StarZone &zone);
  //! try to read a macro table : 'M' (list of 'm')
  bool readSWMacroTable(StarZone &zone);
  //! try to read a node redline : 'v'
  bool readSWNodeRedline(StarZone &zone);
  //! a simple rule : '0' or 'R'
  bool readSWNumRule(StarZone &zone, char kind);
  //! try to read a OLE node : 'O'
  bool readSWOLENode(StarZone &zone);
  //! try to read a list of page style : 'p'
  bool readSWPageDef(StarZone &zone);
  //! try to read a list of redline : 'V' (list of 'R' list of 'D')
  bool readSWRedlineList(StarZone &zone);
  //! try to read a section : 'I'
  bool readSWSection(StarZone &zone);
  //! try to read a table : 'E'
  bool readSWTable(StarZone &zone);
  //! try to read a table box : 't'
  bool readSWTableBox(StarZone &zone);
  //! try to read a table line : 'L'
  bool readSWTableLine(StarZone &zone);
  //! try to read some content : 'T'
  bool readSWTextZone(StarZone &zone);
  //! try to read a list of TOX : 'u' ( list of 'x')
  bool readSWTOXList(StarZone &zone);
  //! try to read a list of TOX51 : 'y' ( list of 'x')
  bool readSWTOX51List(StarZone &zone);
  //
  // data
  //

  //! the state
  shared_ptr<SDWParserInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
