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

#ifndef STOFF_ENTRY_H
#define STOFF_ENTRY_H

#include <ostream>
#include <string>

/** \brief  basic class to store an entry in a file
 * This contained :
 * - its begin and end positions
 * - its type, its name and an identificator
 * - a flag used to know if the file is or not parsed
 */
class STOFFEntry
{
public:
  //!constructor
  STOFFEntry()
    : m_begin(-1)
    , m_length(-1)
    , m_type("")
    , m_name("")
    , m_id(-1)
    , m_parsed(false)
    , m_extra("") {}

  virtual ~STOFFEntry();

  //! sets the begin offset
  void setBegin(long off)
  {
    m_begin = off;
  }
  //! sets the zone size
  void setLength(long l)
  {
    m_length = l;
  }
  //! sets the end offset
  void setEnd(long off)
  {
    m_length = off-m_begin;
  }

  //! returns the begin offset
  long begin() const
  {
    return m_begin;
  }
  //! returns the end offset
  long end() const
  {
    return m_begin+m_length;
  }
  //! returns the length of the zone
  long length() const
  {
    return m_length;
  }

  //! returns true if the zone length is positive
  bool valid() const
  {
    return m_begin >= 0 && m_length > 0;
  }

  //! basic operator==
  bool operator==(const STOFFEntry &a) const
  {
    if (m_begin != a.m_begin) return false;
    if (m_length != a.m_length) return false;
    if (m_id != a. m_id) return false;
    if (m_type != a.m_type) return false;
    if (m_name != a.m_name) return false;
    return true;
  }
  //! basic operator!=
  bool operator!=(const STOFFEntry &a) const
  {
    return !operator==(a);
  }

  //! a flag to know if the entry was parsed or not
  bool isParsed() const
  {
    return m_parsed;
  }
  //! sets the flag m_parsed to true or false
  void setParsed(bool ok=true) const
  {
    m_parsed = ok;
  }

  //! sets the type of the entry: BTEP,FDPP, BTEC, FDPC, PLC , TEXT, ...
  void setType(std::string const &newType)
  {
    m_type=newType;
  }
  //! returns the type of the entry
  std::string const &type() const
  {
    return m_type;
  }
  //! returns true if the type entry == \a type
  bool hasType(std::string const &typ) const
  {
    return m_type == typ;
  }

  //! sets the name of the entry
  void setName(std::string const &nam)
  {
    m_name=nam;
  }
  //! name of the entry
  std::string const &name() const
  {
    return m_name;
  }
  //! checks if the entry name is equal to \a name
  bool hasName(std::string const &nam) const
  {
    return m_name == nam;
  }

  /** \brief returns the id */
  int id() const
  {
    return m_id;
  }
  //! sets the id
  void setId(int newId)
  {
    m_id = newId;
  }

  //! retrieves the extra string
  std::string const &extra() const
  {
    return m_extra;
  }
  //! sets the extra string
  void setExtra(std::string const &s)
  {
    m_extra = s;
  }

  friend std::ostream &operator<< (std::ostream &o, STOFFEntry const &ent)
  {
    o << ent.m_type;
    if (ent.m_name.length()) o << "|" << ent.m_name;
    if (ent.m_id >= 0) o << "[" << ent.m_id << "]";
    if (ent.m_extra.length()) o << "[" << ent.m_extra << "]";
    return o;
  }

protected:
  long m_begin /** the begin of the entry.*/, m_length /** the size of the entry*/;

  //! the entry type
  std::string m_type;
  //! the name
  std::string m_name;
  //! an identificator
  int m_id;
  //! a bool to store if the entry is or not parsed
  mutable bool m_parsed;
  //! an extra string
  std::string m_extra;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
