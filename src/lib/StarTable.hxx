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
 * StarTable to read/parse table in sdw file
 *
 */
#ifndef STAR_TABLE
#  define STAR_TABLE

#include <ostream>
#include <vector>

#include "libstaroffice_internal.hxx"

class StarObject;
class StarObjectText;
class StarState;
class StarZone;
namespace StarTableInternal
{
class Table;
}

/** \brief class to store a table in a sdw file
 */
class StarTable
{
public:
  //! the constructor
  StarTable();
  //! the destructor
  ~StarTable();
  //! try to read the data
  bool read(StarZone &zone, StarObjectText &object);
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarState &state) const;

  //! the table
  std::shared_ptr<StarTableInternal::Table> m_table;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
