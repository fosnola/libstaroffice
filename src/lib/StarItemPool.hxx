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

class StarAttribute;
class StarItemPool;

/** \brief class to store an item: ie. an attribute whose reading is
    potentially retarded
 */
struct StarItem {
  //! constructor
  StarItem() : m_attribute(), m_pool(0), m_which(0), m_surrogateId(0), m_localId(false)
  {
  }
  //! constructor from attribute
  explicit StarItem(shared_ptr<StarAttribute> attribute) : m_attribute(attribute), m_pool(0), m_which(0), m_surrogateId(0), m_localId(false)
  {
  }
  /** the attribute if loaded */
  shared_ptr<StarAttribute> m_attribute;
  //! the item pool
  StarItemPool *m_pool;
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

namespace StarItemPoolInternal
{
struct State;
}

class StarAttribute;
class StarObject;
class StarZone;

/** \brief the main class to read/.. some basic StarOffice SfxItemItemPool itemPools
 *
 *
 *
 */
class StarItemPool
{
public:
  friend struct StarItemPoolInternal::State;
  //! the known item pool
  enum Type { T_ChartPool, T_EditEnginePool, T_SpreadsheetPool, T_VCControlPool, T_WriterPool, T_XOutdevPool, T_Unknown };
  //! constructor
  StarItemPool(StarObject &document, Type type);
  //! destructor
  virtual ~StarItemPool();

  //! add a secondary pool
  void addSecondaryPool(shared_ptr<StarItemPool> secondary);
  //! returns true if a pool is a secondary pool
  bool isSecondaryPool() const;
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
  static bool readStyle(StarZone &zone, shared_ptr<StarItemPool> pool, StarObject &doc);

  //! try to read an attribute
  shared_ptr<StarAttribute> readAttribute(StarZone &zone, int which, int vers, long endPos);
  //! read a item
  shared_ptr<StarItem> readItem(StarZone &zone, bool isDirect, long endPos);
  //! try to load a surrogate
  shared_ptr<StarItem> loadSurrogate(StarZone &zone, uint16_t &nWhich, bool localId, libstoff::DebugStream &f);
  //! try to load a surrogate
  bool loadSurrogate(StarItem &item);

protected:
  //! try to read a "ItemPool" zone (version 1)
  bool readV1(StarZone &zone, StarItemPool *master);
  //! try to read a "ItemPool" zone (version 2)
  bool readV2(StarZone &zone, StarItemPool *master);

  //! create an item for futher reading
  shared_ptr<StarItem> createItem(int which, int surrogateId, bool localId);

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
