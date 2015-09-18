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
 * Parser to convert a text zone/OLE in a StarOffice document
 *
 */
#ifndef STAR_OBJECT_TEXT
#  define STAR_OBJECT_TEXT

#include <vector>

#include "libstaroffice_internal.hxx"
#include "STOFFDebug.hxx"

namespace StarObjectTextInternal
{
struct State;
}

class StarAttributeManager;
class StarObject;
class StarZone;

/** \brief the main class to read a StarOffice sdw file
 *
 *
 *
 */
class StarObjectText
{
  friend class StarAttributeManager;
public:
  //! constructor
  StarObjectText(shared_ptr<StarObject> document);
  //! destructor
  virtual ~StarObjectText();

  // try to parse all zone
  bool parse();

  //! try to read a image map zone : 'X'
  static bool readSWImageMap(StarZone &zone);
  //! try to read a attribute list: 'S'
  static bool readSWAttributeList(StarZone &zone, StarObject &doc);

protected:
  //
  // low level
  //

  //! the page style
  bool readSwPageStyleSheets(STOFFInputStreamPtr input, std::string const &fileName, StarObject &doc);
  //! try to read a text style zone: SfxStyleSheets
  bool readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &fileName);
  //! the rulers?
  bool readSwNumRuleList(STOFFInputStreamPtr input, std::string const &fileName, StarObject &doc);
  //! the main zone
  bool readWriterDocument(STOFFInputStreamPtr input, std::string const &fileName, StarObject &doc);

  //! the drawing layers ?
  bool readDrawingLayer(STOFFInputStreamPtr input, std::string const &fileName, StarObject &doc);

protected:
  //! try to read an attribute: 'A'
  static bool readSWAttribute(StarZone &zone, StarObject &doc);
  //! try a list of bookmark field : 'a'
  bool readSWBookmarkList(StarZone &zone);
  //! try to read some content : 'N'
  bool readSWContent(StarZone &zone, StarObject &doc);
  //! try to read a DBName zone : 'D'
  bool readSWDBName(StarZone &zone);
  //! try to read a dictionary table : 'j'
  bool readSWDictionary(StarZone &zone);
  //! try to read a endnode node : '4'
  bool readSWEndNoteInfo(StarZone &zone);
  //! try to read a footnode node : '1'
  bool readSWFootNoteInfo(StarZone &zone);
  //! try to read a OLE node : 'g'
  bool readSWGraphNode(StarZone &zone, StarObject &doc);
  //! try to read a SW zone setup : 'J'
  bool readSWJobSetUp(StarZone &zone);
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
  bool readSWPageDef(StarZone &zone, StarObject &doc);
  //! try to read a list of redline : 'V' (list of 'R' list of 'D')
  bool readSWRedlineList(StarZone &zone);
  //! try to read a section : 'I'
  bool readSWSection(StarZone &zone);
  //! try to read a table : 'E'
  bool readSWTable(StarZone &zone, StarObject &doc);
  //! try to read a table box : 't'
  bool readSWTableBox(StarZone &zone, StarObject &doc);
  //! try to read a table line : 'L'
  bool readSWTableLine(StarZone &zone, StarObject &doc);
  //! try to read some content : 'T'
  bool readSWTextZone(StarZone &zone, StarObject &doc);
  //! try to read a list of TOX : 'u' ( list of 'x')
  bool readSWTOXList(StarZone &zone, StarObject &doc);
  //! try to read a list of TOX51 : 'y' ( list of 'x')
  bool readSWTOX51List(StarZone &zone);
  //
  // data
  //

  //! the ole document
  shared_ptr<StarObject> m_document;
  //! the state
  shared_ptr<StarObjectTextInternal::State> m_state;
private:
  StarObjectText(StarObjectText const &orig);
  StarObjectText &operator=(StarObjectText const &orig);
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
