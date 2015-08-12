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
 * class to parse/store the main ole document
 *
 */
#ifndef STAR_DOCUMENT
#  define STAR_DOCUMENT

#include <vector>
#include <stack>
#include <libstaroffice/STOFFDocument.hxx>

#include "STOFFDebug.hxx"
#include "STOFFOLEParser.hxx"

namespace StarDocumentInternal
{
struct State;
}

class StarItemPool;

class SDWParser;
/** \brief a main ole in a StarOffice file
 *
 *
 *
 */
class StarDocument
{
public:
  //! constructor
  StarDocument(STOFFInputStreamPtr input, shared_ptr<STOFFOLEParser> oleParser, shared_ptr<STOFFOLEParser::OleDirectory> directory, SDWParser *parser);
  //! destructor
  virtual ~StarDocument();

  //
  // basic
  //

  //! try to parse data
  bool parse();
  //! returns a new item pool for this document
  shared_ptr<StarItemPool> getNewItemPool();
  //! returns the document kind
  STOFFDocument::Kind getDocumentKind() const;
  //! returns a SDWParser(REMOVEME)
  SDWParser *getSDWParser();

protected:
  //!  the "persist elements" small ole: the list of object
  bool readPersistElements(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! try to read the document information : "SfxDocumentInformation"
  bool readSfxDocumentInformation(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! try to read the windows information : "SfxWindows"
  bool readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! try to read the "Star Framework Config File"
  bool readStarFrameworkConfigFile(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);

  //
  // data
  //

  //! the input
  STOFFInputStreamPtr m_input;
  //! the main ole parser
  shared_ptr<STOFFOLEParser> m_oleParser;
  //! the directory
  shared_ptr<STOFFOLEParser::OleDirectory> m_directory;

  //! the state
  shared_ptr<StarDocumentInternal::State> m_state;

private:
  StarDocument(StarDocument const &orig);
  StarDocument &operator=(StarDocument const &orig);
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
