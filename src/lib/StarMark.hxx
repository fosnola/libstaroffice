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
 * StarMark to read/parse some basic mark in StarOffice documents
 *
 */
#ifndef STAR_MARK
#  define STAR_MARK

#include <ostream>
#include <vector>

#include "libstaroffice_internal.hxx"

class StarZone;
/** \brief class to store a mark in a text zone
 */
class StarMark
{
public:
  //! constructor
  StarMark() : m_type(-1), m_id(-1), m_offset(-1)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, StarMark const &mark);
  //! try to read a mark
  bool read(StarZone &zone);
  //! the type:  2: bookmark-start, 3:bookmark-end
  int m_type;
  //! the id
  int m_id;
  //! the offset
  int m_offset;
};

/** \brief class to store a bookmark
 */
class StarBookmark
{
public:
  //! the constructor
  StarBookmark() : m_shortName(""), m_name(""), m_offset(0), m_key(0), m_modifier(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, StarBookmark const &mark);
  //! try to read a mark
  bool read(StarZone &zone);
  //! try to read a list of bookmark
  static bool readList(StarZone &zone, std::vector<StarBookmark> &markList);
  //! the shortname
  librevenge::RVNGString m_shortName;
  //! the name
  librevenge::RVNGString m_name;
  //! the offset
  int m_offset;
  //! the key
  int m_key;
  //! the modifier
  int m_modifier;
  //! the macros names
  librevenge::RVNGString m_macroNames[4];
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
