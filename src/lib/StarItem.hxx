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
 * StarPool to store some StarOffice items
 *
 */
#ifndef STAR_ITEM_HXX
#  define STAR_ITEM_HXX

#include <map>
#include <vector>

#include <libstaroffice/STOFFDocument.hxx>

#include "STOFFEntry.hxx"

class StarAttribute;

/** \brief class to store an item: ie. an attribute whose reading is
    potentially retarded
 */
class StarItem
{
public:
  //! constructor
  explicit StarItem(int which) : m_attribute(), m_which(which), m_surrogateId(0), m_localId(false)
  {
  }
  //! constructor from attribute
  StarItem(shared_ptr<StarAttribute> attribute, int which) : m_attribute(attribute), m_which(which), m_surrogateId(0), m_localId(false)
  {
  }
  /** the attribute if loaded */
  shared_ptr<StarAttribute> m_attribute;
  //! the which id
  int m_which;
  //! the surrogate id
  int m_surrogateId;
  //! true if which is local
  bool m_localId;
private:
  StarItem(StarItem const &);
  StarItem &operator=(StarItem const &);
};

/** \brief class to store a list of item
 */
class StarItemSet
{
public:
  //! constructor
  StarItemSet() : m_style(""), m_whichToItemMap()
  {
  }
  //! return true if the set is empty
  bool empty() const
  {
    return m_whichToItemMap.empty();
  }
  //! try to add a item
  bool add(shared_ptr<StarItem> item);
  //! item set name
  librevenge::RVNGString m_style;
  //! the list of item
  std::map<int, shared_ptr<StarItem> > m_whichToItemMap;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
