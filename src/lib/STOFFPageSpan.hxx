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

#ifndef STOFFPAGESPAN_H
#define STOFFPAGESPAN_H

#include <map>
#include <vector>

#include "libstaroffice_internal.hxx"

#include "STOFFFont.hxx"
#include "STOFFSection.hxx"

class STOFFListener;

/** a class which stores the header/footer */
class STOFFHeaderFooter
{
public:
  //! the different type
  enum Type { R_Left=0, R_Center, R_Right, R_All };
  //! constructor
  STOFFHeaderFooter()
  {
  }
  //! destructor
  ~STOFFHeaderFooter()
  {
  }
  //! operator==
  bool operator==(STOFFHeaderFooter const &headerFooter) const;
  //! operator!=
  bool operator!=(STOFFHeaderFooter const &headerFooter) const
  {
    return !operator==(headerFooter);
  }
  /** send to header to the listener */
  void send(STOFFListener *listener, bool header) const;
  //! the document data: left, center, right, all
  STOFFSubDocumentPtr m_subDocument[4];
};

/** A class which defines the page properties */
class STOFFPageSpan
{
public:
  /** the zone type */
  enum ZoneType { Page=0, Header, Footer };
public:
  //! constructor
  STOFFPageSpan();
  STOFFPageSpan(STOFFPageSpan const &)=default;
  STOFFPageSpan(STOFFPageSpan &&)=default;
  STOFFPageSpan &operator=(STOFFPageSpan const &)=default;
  STOFFPageSpan &operator=(STOFFPageSpan &&)=default;
  //! destructor
  ~STOFFPageSpan();

  //! add a header on some page: occurrence must be odd|right, even|left, first, last or all
  void addHeaderFooter(bool header, std::string const &occurrence, STOFFHeaderFooter const &hf);
  //! operator==
  bool operator==(std::shared_ptr<STOFFPageSpan> const &pageSpan) const;
  //! operator!=
  bool operator!=(std::shared_ptr<STOFFPageSpan> const &pageSpan) const
  {
    return !operator==(pageSpan);
  }

  // interface with STOFFListener

  //! add the page properties in pList
  void getPageProperty(librevenge::RVNGPropertyList &pList) const;
  //! send the page's headers/footers if some exists
  void sendHeaderFooters(STOFFListener *listener) const;

  //! the number of page
  int m_pageSpan;
  //! the document, header and footer property list
  librevenge::RVNGPropertyList m_propertiesList[3];
  //! the map occurrence to header/footer document
  std::map<std::string, STOFFHeaderFooter> m_occurrenceHFMap[2];
  //! the main section
  STOFFSection m_section;
  //! the page number ( or -1)
  int m_pageNumber;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
