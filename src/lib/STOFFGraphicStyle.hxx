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

#ifndef STOFF_GRAPHIC_STYLE
#  define STOFF_GRAPHIC_STYLE

#include <ostream>
#include <string>
#include <vector>

#include "libstaroffice_internal.hxx"

//! Class to store a graphic style
class STOFFGraphicStyle
{
public:
  /** constructor */
  STOFFGraphicStyle();
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList) const;
  /** check if the line style is set and the fill style is set.
      If not set the line style, to black color and the fill style to none */
  static void checkForDefault(librevenge::RVNGPropertyList &propList);
  //! check if the padding margins are set, if not set them to 0
  static void checkForPadding(librevenge::RVNGPropertyList &propList);
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, STOFFGraphicStyle const &graphicStyle);
  //! operator==
  bool operator==(STOFFGraphicStyle const &graphicStyle) const;
  //! operator!=
  bool operator!=(STOFFGraphicStyle const &graphicStyle) const
  {
    return !operator==(graphicStyle);
  }
  /** the property list */
  librevenge::RVNGPropertyList m_propertyList;
  //! true if background attribute is set
  bool m_hasBackground;
  //! the protection: move, size, printable
  bool m_protections[3];
};


#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
