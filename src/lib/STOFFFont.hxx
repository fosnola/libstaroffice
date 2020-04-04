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

#ifndef STOFF_FONT
#  define STOFF_FONT

#include <string>
#include <vector>

#include "libstaroffice_internal.hxx"

//! Class to store font
class STOFFFont
{
public:
  /** constructor */
  STOFFFont()
    : m_propertyList()
    , m_shadowColor(STOFFColor::black())
    , m_hyphen(false)
    , m_softHyphen(false)
    , m_hardBlank(false)
    , m_tab(false)
    , m_lineBreak(false)
  {
  }
  STOFFFont(STOFFFont const &)=default;
  STOFFFont(STOFFFont &&)=default;
  STOFFFont &operator=(STOFFFont const &)=default;
  STOFFFont &operator=(STOFFFont &&)=default;
  /** destructor */
  ~STOFFFont();
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList) const;
  /** check if the font name defined. If not set Times*/
  static void checkForDefault(librevenge::RVNGPropertyList &propList);

  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, STOFFFont const &font);
  //! a comparison function
  int cmp(STOFFFont const &font) const;
  //! operator==
  bool operator==(STOFFFont const &font) const
  {
    return cmp(font)==0;
  }
  //! operator!=
  bool operator!=(STOFFFont const &font) const
  {
    return cmp(font)!=0;
  }
  //! operator<
  bool operator<(STOFFFont const &font) const
  {
    return cmp(font)<0;
  }
  /** the property list */
  librevenge::RVNGPropertyList m_propertyList;
  //! the shadow color
  STOFFColor m_shadowColor;
  /** hyphen */
  bool m_hyphen;
  /** soft hyphen */
  bool m_softHyphen;
  /** hard blank */
  bool m_hardBlank;
  /** tab */
  bool m_tab;
  /** line break */
  bool m_lineBreak;
};


#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
