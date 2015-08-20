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
 * StarPool to read/parse some StarOffice pools
 *
 */
#ifndef STAR_ITEM_POOL_HXX
#  define STAR_ITEM_POOL_HXX

#include <vector>

#include <libstaroffice/STOFFDocument.hxx>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

namespace StarItemPoolInternal
{
struct State;
}

class StarDocument;
class StarZone;

/** \brief the main class to read/.. some basic StarOffice SfxItemItemPool itemPools
 *
 *
 *
 */
class StarItemPool
{
public:
  //! the known item pool
  enum Type { T_ChartPool, T_EditEnginePool, T_SpreadsheetPool, T_VCControlPool, T_WriterPool, T_XOutdevPool, T_Unknown };
  //! constructor
  StarItemPool(StarDocument &document, Type type);
  //! destructor
  virtual ~StarItemPool();

  //! try to read a "ItemPool" zone
  bool read(StarZone &zone);
  //! returns the pool version
  int getVersion() const;
  //! returns the pool type
  Type getType() const;
  //! returns true if we are reading the pool
  bool isInside() const
  {
    return m_isInside;
  }
  //! try to read a "StyleItemPool" zone
  static bool readStyle(StarZone &zone, int itemPoolVersion);

  //! try to read an attribute
  bool readAttribute(StarZone &zone, int which, int vers, long endPos);
  //! read a item
  bool readItem(StarZone &zone, bool isDirect, long endPos);
protected:
  //! try to read a "ItemPool" zone (version 1)
  bool readV1(StarZone &zone);
  //! try to read a "ItemPool" zone (version 2)
  bool readV2(StarZone &zone);

  //
  // data
  //
private:
  //! true if the pool is open
  bool m_isInside;
  //! the state
  shared_ptr<StarItemPoolInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
