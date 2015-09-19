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

#ifndef STOFF_HEADER_H
#define STOFF_HEADER_H
/** \file STOFFHeader.hxx
 * Defines STOFFHeader (document's type, version, kind)
 */

#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include <libstaroffice/libstaroffice.hxx>
#include "STOFFInputStream.hxx"

/** \brief a function used by STOFFDocument to store the version of document
 *
 * This class is responsible for finding a list of potential formats
 * corresponding to a file, this list will latter be checked by
 * calling the corresponding parser's function checkHeader via
 * STOFFDocument.
 *
 * This class also allows to store the document type, king and version.
 */
class STOFFHeader
{
public:
  typedef enum STOFFDocument::Kind Kind;

  /** constructor given the input

      \param version the file version
      \param kind the document kind (default word processing document)
  */
  STOFFHeader(int version=0, STOFFDocument::Kind kind = STOFFDocument::STOFF_K_TEXT);
  //! destructor
  virtual ~STOFFHeader();

  /** tests the input file and returns a header if the file looks like a STOFF document ( trying first to use the resource parsed if it exists )

  \note this check phase can only be partial ; ie. we only test the first bytes of the file and/or the existence of some oles. This explains that STOFFDocument implements a more complete test to recognize the difference Mac Files which share the same type of header...
  */
  static std::vector<STOFFHeader> constructHeader(STOFFInputStreamPtr input);

  //! resets the data
  void reset(int vers, Kind kind = STOFFDocument::STOFF_K_TEXT)
  {
    m_version = vers;
    m_docKind = kind;
  }

  //! returns the major version
  int getVersion() const
  {
    return m_version;
  }
  //! sets the major version
  void setVersion(int version)
  {
    m_version=version;
  }

  //! returns the document kind
  Kind getKind() const
  {
    return m_docKind;
  }
  //! sets the document kind
  void setKind(Kind kind)
  {
    m_docKind = kind;
  }
  //! returns true if the file is encypted
  bool isEncrypted() const
  {
    return m_isEncrypted;
  }
  //! set the encryption mode
  void setEncrypted(bool encrypted)
  {
    m_isEncrypted=encrypted;
  }
private:
  /** the document version */
  int m_version;
  /** the document kind */
  Kind m_docKind;
  /** flag to know if the file is encrypted */
  bool m_isEncrypted;
};

#endif /* STOFFHEADER_H */
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
