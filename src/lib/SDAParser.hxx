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
 * Parser to convert sda StarOffice document
 *
 */
#ifndef SDA_PARSER
#  define SDA_PARSER

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

#include "STOFFParser.hxx"

namespace SDAParserInternal
{
struct State;
}

class StarAttributeManager;
class StarObject;
class StarZone;
class STOFFOLEParser;
class StarFormatManager;
/** \brief the main class to read a StarOffice sda/sdd file
 *
 *
 *
 */
class SDAParser final : public STOFFGraphicParser
{
public:
  //! constructor
  SDAParser(STOFFInputStreamPtr &input, STOFFHeader *header);
  //! destructor
  ~SDAParser() final;
  //! set the document password
  void setDocumentPassword(char const *passwd)
  {
    m_password=passwd;
  }
  //! checks if the document header is correct (or not)
  bool checkHeader(STOFFHeader *header, bool strict=false) override;

  // the main parse function
  void parse(librevenge::RVNGDrawingInterface *documentInterface) final;
  // the main presentation parse function
  void parse(librevenge::RVNGPresentationInterface *documentInterface) final;

protected:
  //! creates the listener which will be associated to the document
  void createDocument(librevenge::RVNGDrawingInterface *documentInterface);
  //! creates the listener which will be associated to the document
  void createDocument(librevenge::RVNGPresentationInterface *documentInterface);

  //! parses the different OLE, ...
  bool createZones();

  //
  // low level
  //

  //
  // data
  //

  //! the password
  char const *m_password;
  //! the ole parser
  std::shared_ptr<STOFFOLEParser> m_oleParser;
  //! the state
  std::shared_ptr<SDAParserInternal::State> m_state;
private:
  SDAParser(SDAParser const &orig) = delete;
  SDAParser &operator=(SDAParser const &orig) = delete;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
