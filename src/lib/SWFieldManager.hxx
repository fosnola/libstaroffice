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
 * FieldManager to read/parse SW StarOffice field
 *
 */
#ifndef SW_FIELDMANAGER
#  define SW_FIELDMANAGER

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

class StarState;

namespace SWFieldManagerInternal
{
////////////////////////////////////////
//! Internal: a field
struct Field {
  //! constructor
  Field()
    : m_type(-1)
    , m_subType(-1)
    , m_format(-1)
    , m_name("")
    , m_content("")
    , m_textValue("")
    , m_doubleValue(0)
    , m_level(0)
  {
  }
  //! destructor
  virtual ~Field();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Field const &field)
  {
    field.print(o);
    return o;
  }
  //! add to send the zone data
  virtual bool send(STOFFListenerPtr &listener, StarState &state) const;
  //! print a field
  virtual void print(std::ostream &o) const;
  //! the field type
  int m_type;
  //! the subtype
  int m_subType;
  //! the field format
  int m_format;
  //! the name
  librevenge::RVNGString m_name;
  //! the content
  librevenge::RVNGString m_content;
  //! the value text
  librevenge::RVNGString m_textValue;
  //! double
  double m_doubleValue;
  //! the chapter level
  int m_level;
protected:
  //! copy constructor
  Field(const Field &) = default;
};

struct State;
}

class StarZone;

/** \brief the main class to read/.. a StarOffice sdw field
 *
 *
 *
 */
class SWFieldManager
{
public:
  //! constructor
  SWFieldManager();
  //! destructor
  virtual ~SWFieldManager();


  //! try to read a field type
  std::shared_ptr<SWFieldManagerInternal::Field> readField(StarZone &zone, unsigned char cKind='_');
  //! try to read a persist field type
  std::shared_ptr<SWFieldManagerInternal::Field> readPersistField(StarZone &zone, long lastPos);

  //
  // data
  //
private:
  //! the state
  std::shared_ptr<SWFieldManagerInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
