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

#ifndef STOFF_SUB_DOCUMENT_HXX
#define STOFF_SUB_DOCUMENT_HXX

#include "libstaroffice_internal.hxx"
#include "STOFFEntry.hxx"

/** abstract class used to store a subdocument (with a comparison function) */
class STOFFSubDocument
{
public:
  //! constructor from parser, input stream and zone in the input
  STOFFSubDocument(STOFFParser *pars, STOFFInputStreamPtr ip, STOFFEntry const &z);
  //! copy constructor
  explicit STOFFSubDocument(STOFFSubDocument const &doc);
  //! copy operator
  STOFFSubDocument &operator=(STOFFSubDocument const &doc);
  //! virtual destructor
  virtual ~STOFFSubDocument();

  //! comparison operator!=
  virtual bool operator!=(STOFFSubDocument const &doc) const;
  //! comparison operator==
  bool operator==(STOFFSubDocument const &doc) const
  {
    return !operator!=(doc);
  }
  //! comparison operator!=
  bool operator!=(std::shared_ptr<STOFFSubDocument> const &doc) const;
  //! comparison operator==
  bool operator==(std::shared_ptr<STOFFSubDocument> const &doc) const
  {
    return !operator!=(doc);
  }

  /** virtual parse function
   *
   * this function is called to parse the subdocument */
  virtual void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType subDocumentType) = 0;

protected:
  //! the main zone parser
  STOFFParser *m_parser;
  //! the input
  std::shared_ptr<STOFFInputStream> m_input;
  //! if valid the zone to parse
  STOFFEntry m_zone;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
