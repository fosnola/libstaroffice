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

#ifndef STOFF_PARAGRAPH
#  define STOFF_PARAGRAPH

#include <iostream>
#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFList.hxx"

//! class to store the paragraph properties
class STOFFParagraph
{
public:
  //! constructor
  STOFFParagraph()
    : m_propertyList()
    , m_outline(false)
    , m_bulletVisible(false)
    , m_listLevelIndex(0)
    , m_listId(-1)
    , m_listStartValue(-1)
    , m_listLevel()
  {
  }
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList) const;
  //! operator==
  bool operator==(STOFFParagraph const &p) const;
  //! operator!=
  bool operator!=(STOFFParagraph const &p) const
  {
    return !operator==(p);
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, STOFFParagraph const &para);
  //! the properties
  librevenge::RVNGPropertyList m_propertyList;
  /// flag to know if this is a outline level
  bool m_outline;
  /// flag to know if the bullet is visible
  bool m_bulletVisible;
  /** the actual level index */
  int m_listLevelIndex;
  /** the list id (if know ) */
  int m_listId;
  /** the list start value (if set ) */
  int m_listStartValue;
  /** the actual level */
  STOFFListLevel m_listLevel;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
