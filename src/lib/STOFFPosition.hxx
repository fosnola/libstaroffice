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

#ifndef STOFF_POSITION_H
#define STOFF_POSITION_H

#include <ostream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

/** Class to define the position of an object (textbox, picture, ..) in the document
 *
 * Stores the page, object position, object size, anchor, wrapping, ...
 */
class STOFFPosition
{
public:
  //! a list of enum used to defined the anchor
  enum AnchorTo { Char, CharBaseLine, Frame, Paragraph, Page, Unknown };

public:
  //! constructor
  STOFFPosition() : m_anchorTo(Unknown), m_propertyList() {}
  //! destructor
  virtual ~STOFFPosition() {}
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList) const
  {
    librevenge::RVNGPropertyList::Iter i(m_propertyList);
    for (i.rewind(); i.next();) {
      if (i.child()) {
        STOFF_DEBUG_MSG(("STOFFPosition::addTo: find unexpected property child\n"));
        propList.insert(i.key(), *i.child());
        continue;
      }
      propList.insert(i.key(), i()->clone());
    }
  }
  //! utility function to set a origin
  void setOrigin(STOFFVec2f const &origin, librevenge::RVNGUnit unit)
  {
    m_propertyList.insert("svg:x",origin[0], unit);
    m_propertyList.insert("svg:y",origin[1], unit);
  }
  //! utility function to set a size
  void setSize(STOFFVec2f const &size, librevenge::RVNGUnit unit)
  {
    if (size.x()>0)
      m_propertyList.insert("svg:width",size.x(), unit);
    else if (size.x()<0)
      m_propertyList.insert("fo:min-width",-size.x(), unit);
    if (size.y()>0)
      m_propertyList.insert("svg:height",size.y(), unit);
    else if (size.y()<0)
      m_propertyList.insert("fo:min-height",-size.y(), unit);
  }
  //! operator<<
  friend  std::ostream &operator<<(std::ostream &o, STOFFPosition const &pos)
  {
    o << "prop=[" << pos.m_propertyList.getPropString().cstr() << "],";
    return o;
  }
  //! basic operator==
  bool operator==(STOFFPosition const &f) const
  {
    return m_anchorTo==f.m_anchorTo && m_propertyList.getPropString()==f.m_propertyList.getPropString();
  }
  //! basic operator!=
  bool operator!=(STOFFPosition const &f) const
  {
    return !operator==(f);
  }

  //! anchor position
  AnchorTo m_anchorTo;
  //! the property list
  librevenge::RVNGPropertyList m_propertyList;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
