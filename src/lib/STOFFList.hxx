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

#ifndef STOFF_LIST_H
#  define STOFF_LIST_H

#include <iostream>

#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

class STOFFFont;

/** small structure to keep information about a list level */
struct STOFFListLevel {
  /** the type of the level */
  enum Type { DEFAULT, NONE, BULLET, NUMBER };

  /** basic constructor */
  STOFFListLevel()
    : m_type(NONE)
    , m_propertyList()
    , m_font()
    , m_startValue(0)
  {
  }
  STOFFListLevel(STOFFListLevel const &)=default;
  STOFFListLevel(STOFFListLevel &&)=default;
  STOFFListLevel &operator=(STOFFListLevel const &)=default;
  STOFFListLevel &operator=(STOFFListLevel &&)=default;
  /** destructor */
  ~STOFFListLevel();

  /** returns true if the level type was not set */
  bool isDefault() const
  {
    return m_type==DEFAULT;
  }
  /** returns true if the list is decimal, alpha or roman */
  bool isNumeric() const
  {
    return m_type==NUMBER;
  }
  //! operator==
  bool operator==(STOFFListLevel const &levl) const
  {
    return cmp(levl)==0;
  }
  //! operator!=
  bool operator!=(STOFFListLevel const &levl) const
  {
    return !operator==(levl);
  }
  /** add the information of this level in the propList */
  void addTo(librevenge::RVNGPropertyList &propList) const;

  /** returns the start value (if set) or 1 */
  int getStartValue() const
  {
    return m_startValue <= 0 ? 1 : m_startValue;
  }

  /** comparison function ( compare all values excepted m_startValues */
  int cmp(STOFFListLevel const &levl) const;
  /** the type of the level */
  Type m_type;
  //! the propertyList
  librevenge::RVNGPropertyList m_propertyList;
  /// the font
  std::shared_ptr<STOFFFont> m_font;
  /** the actual value (if this is an ordered level ) */
  int m_startValue;
};

/** a small structure used to store the informations about a list */
class STOFFList
{
public:
  /** default constructor */
  explicit STOFFList(bool outline)
    : m_outline(outline)
    , m_name(""), m_levels()
    , m_actLevel(-1)
    , m_actualIndices()
    , m_nextIndices()
    , m_modifyMarker(1)
  {
    m_id[0] = m_id[1] = -1;
  }

  /** returns the list id */
  int getId() const
  {
    return m_id[0];
  }

  /** returns the actual modify marker */
  int getMarker() const
  {
    return m_modifyMarker;
  }
  /** resize the number of level of the list (keeping only n level) */
  void resize(int levl);
  /** returns true if we can add a new level in the list without changing is meaning */
  bool isCompatibleWith(int levl, STOFFListLevel const &level) const;
  /** returns true if the list is compatible with the defined level of new list */
  bool isCompatibleWith(STOFFList const &newList) const;
  /** update the indices, the actual level from newList */
  void updateIndicesFrom(STOFFList const &list);

  /** swap the list id

  \note a cheat because writerperfect imposes to get a new id if the level 1 changes
  */
  void swapId() const
  {
    int tmp = m_id[0];
    m_id[0] = m_id[1];
    m_id[1] = tmp;
  }

  /** set the list id */
  void setId(int newId) const;

  /** returns a level if it exists */
  STOFFListLevel getLevel(int levl) const
  {
    if (levl >= 0 && levl < int(m_levels.size()))
      return m_levels[size_t(levl)];
    STOFF_DEBUG_MSG(("STOFFList::getLevel: can not find level %d\n", levl));
    return STOFFListLevel();
  }
  /** returns the number of level */
  int numLevels() const
  {
    return int(m_levels.size());
  }
  /** sets a level */
  void set(int levl, STOFFListLevel const &level);

  /** set the list level */
  void setLevel(int levl) const;
  /** open the list element */
  void openElement() const;
  /** close the list element */
  void closeElement() const {}
  /** returns the startvalue corresponding to the actual level ( or -1 for an unknown/unordered list) */
  int getStartValueForNextElement() const;
  /** set the startvalue corresponding to the actual level*/
  void setStartValueForNextElement(int value);

  /** returns true is a level is numeric */
  bool isNumeric(int levl) const;

  /// retrieve the list level property
  bool addTo(int level, librevenge::RVNGPropertyList &pList) const;

  /// flag to know if the list is a outline list
  bool m_outline;
  /// the list name
  librevenge::RVNGString m_name;
protected:
  //! the different levels
  std::vector<STOFFListLevel> m_levels;

  //! the actual levels
  mutable int m_actLevel;
  mutable std::vector<int> m_actualIndices, m_nextIndices;
  //! the identificator ( actual and auxilliar )
  mutable int m_id[2];
  //! a modification marker ( can be used to check if a list has been send to a interface )
  mutable int m_modifyMarker;
};

/** a manager which manages the lists, keeps the different kind of lists, to assure the unicity of each list */
class STOFFListManager
{
public:
  //! the constructor
  STOFFListManager()
    : m_listList()
    , m_sendIdMarkerList() { }
  //! the destructor
  ~STOFFListManager() { }
  /** check if a list need to be send/resend to the interface */
  bool needToSend(int index, std::vector<int> &idMarkerList) const;
  //! returns a list with given index ( if found )
  std::shared_ptr<STOFFList> getList(int index) const;
  //! returns a new list corresponding to a list where we have a new level
  std::shared_ptr<STOFFList> getNewList(std::shared_ptr<STOFFList> actList, int levl, STOFFListLevel const &level);
  //! add a new list
  std::shared_ptr<STOFFList> addList(std::shared_ptr<STOFFList> actList);
protected:
  //! the list of created list
  std::vector<STOFFList> m_listList;
  //! the list of send list to interface
  mutable std::vector<int> m_sendIdMarkerList;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
