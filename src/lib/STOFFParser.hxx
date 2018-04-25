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

#ifndef STOFF_PARSER_H
#define STOFF_PARSER_H

#include <ostream>
#include <string>
#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFInputStream.hxx"

#include "STOFFEntry.hxx"
#include "STOFFHeader.hxx"
#include "STOFFPageSpan.hxx"

/** a class to define the parser state */
class STOFFParserState
{
public:
  //! the parser state type
  enum Type { Graphic, Presentation, Spreadsheet, Text };
  //! Constructor
  STOFFParserState(Type type, STOFFInputStreamPtr &input, STOFFHeader *header);
  //! destructor
  ~STOFFParserState();
  //! the state type
  Type m_type;
  //! the document kind
  STOFFDocument::Kind m_kind;
  //! the actual version
  int m_version;
  //! the input
  STOFFInputStreamPtr m_input;
  //! the header
  STOFFHeader *m_header;
  //! the actual document size
  STOFFPageSpan m_pageSpan;

  //! the list manager
  STOFFListManagerPtr m_listManager;
  //! the graphic listener
  STOFFGraphicListenerPtr m_graphicListener;
  //! the spreadsheet listener
  STOFFSpreadsheetListenerPtr m_spreadsheetListener;
  //! the text listener
  STOFFTextListenerPtr m_textListener;

  //! the debug file
  libstoff::DebugFile m_asciiFile;

private:
  STOFFParserState(STOFFParserState const &orig);
  STOFFParserState &operator=(STOFFParserState const &orig);
};

/** virtual class which defines the ancestor of all main zone parser */
class STOFFParser
{
public:
  //! virtual destructor
  virtual ~STOFFParser();
  //! virtual function used to check if the document header is correct (or not)
  virtual bool checkHeader(STOFFHeader *header, bool strict=false) = 0;

  //! returns the works version
  int version() const
  {
    return m_parserState->m_version;
  }
  //! returns the parser state
  STOFFParserStatePtr getParserState()
  {
    return m_parserState;
  }
  //! returns the header
  STOFFHeader *getHeader()
  {
    return m_parserState->m_header;
  }
  //! returns the actual input
  STOFFInputStreamPtr &getInput()
  {
    return m_parserState->m_input;
  }
  //! returns the actual page dimension
  STOFFPageSpan const &getPageSpan() const
  {
    return m_parserState->m_pageSpan;
  }
  //! returns the actual page dimension
  STOFFPageSpan &getPageSpan()
  {
    return m_parserState->m_pageSpan;
  }
  //! returns the graphic listener
  STOFFGraphicListenerPtr &getGraphicListener()
  {
    return m_parserState->m_graphicListener;
  }
  //! returns the spreadsheet listener
  STOFFSpreadsheetListenerPtr &getSpreadsheetListener()
  {
    return m_parserState->m_spreadsheetListener;
  }
  //! returns the text listener
  STOFFTextListenerPtr &getTextListener()
  {
    return m_parserState->m_textListener;
  }
  //! a DebugFile used to write what we recognize when we parse the document
  libstoff::DebugFile &ascii()
  {
    return m_parserState->m_asciiFile;
  }
protected:
  //! constructor (protected)
  STOFFParser(STOFFParserState::Type type, STOFFInputStreamPtr input, STOFFHeader *header);
  //! constructor using a state
  explicit STOFFParser(STOFFParserStatePtr &state) : m_parserState(state), m_asciiName("") { }

  //! sets the document's version
  void setVersion(int vers)
  {
    m_parserState->m_version = vers;
  }
  //! sets the graphic listener
  void setGraphicListener(STOFFGraphicListenerPtr &listener);
  //! resets the graphic listener
  void resetGraphicListener();
  //! sets the spreadsheet listener
  void setSpreadsheetListener(STOFFSpreadsheetListenerPtr &listener);
  //! resets the spreadsheet listener
  void resetSpreadsheetListener();
  //! sets the text listener
  void setTextListener(STOFFTextListenerPtr &listener);
  //! resets the text listener
  void resetTextListener();
  //! Debugging: change the default ascii file
  void setAsciiName(char const *name)
  {
    m_asciiName = name;
  }
  //! return the ascii file name
  std::string const &asciiName() const
  {
    return m_asciiName;
  }

private:
  //! private copy constructor: forbidden
  STOFFParser(const STOFFParser &);
  //! private operator=: forbidden
  STOFFParser &operator=(const STOFFParser &);

  //! the parser state
  STOFFParserStatePtr m_parserState;
  //! the debug file name
  std::string m_asciiName;
};

/** virtual class which defines the ancestor of all text zone parser */
class STOFFTextParser : public STOFFParser
{
public:
  //! virtual function used to parse the input
  virtual void parse(librevenge::RVNGTextInterface *documentInterface) = 0;
protected:
  //! constructor (protected)
  STOFFTextParser(STOFFInputStreamPtr &input, STOFFHeader *header) : STOFFParser(STOFFParserState::Text, input, header) {}
  //! constructor using a state
  explicit STOFFTextParser(STOFFParserStatePtr &state) : STOFFParser(state) {}
  //! destructor
  ~STOFFTextParser() override;
};

/** virtual class which defines the ancestor of all graphic/presentation zone parser */
class STOFFGraphicParser : public STOFFParser
{
public:
  //! virtual function used to parse the input
  virtual void parse(librevenge::RVNGDrawingInterface *documentInterface);
  //! virtual function used to parse the input
  virtual void parse(librevenge::RVNGPresentationInterface *documentInterface);
protected:
  //! constructor (protected)
  STOFFGraphicParser(STOFFInputStreamPtr &input, STOFFHeader *header) : STOFFParser(STOFFParserState::Graphic, input, header) {}
  //! constructor using a state
  explicit STOFFGraphicParser(STOFFParserStatePtr &state) : STOFFParser(state) {}
  //! destructor
  ~STOFFGraphicParser() override;
};

/** virtual class which defines the ancestor of all spreadsheet zone parser */
class STOFFSpreadsheetParser : public STOFFParser
{
public:
  //! virtual function used to parse the input
  virtual void parse(librevenge::RVNGSpreadsheetInterface *documentInterface) = 0;
protected:
  //! constructor (protected)
  STOFFSpreadsheetParser(STOFFInputStreamPtr &input, STOFFHeader *header) : STOFFParser(STOFFParserState::Spreadsheet, input, header) {}
  //! constructor using a state
  explicit STOFFSpreadsheetParser(STOFFParserStatePtr &state) : STOFFParser(state) {}
  //! destructor
  ~STOFFSpreadsheetParser() override;
};

#endif /* STOFFPARSER_H */
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
