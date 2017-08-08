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
 * Parser to convert a SwNumRules in a StarOffice document
 *
 */
#ifndef STAR_OBJECT_NUMERICRULER
#  define STAR_OBJECT_NUMERICRULER

#include <ostream>
#include <vector>

#include "libstaroffice_internal.hxx"
#include "StarObject.hxx"

namespace StarObjectNumericRulerInternal
{
struct State;
}

class StarState;
class StarZone;

struct STOFFListLevel;
class STOFFList;

/** \brief the main class to read a SwNumRules zone
 *
 *
 *
 */
class StarObjectNumericRuler : public StarObject
{
public:
  //! constructor
  StarObjectNumericRuler(StarObject const &orig, bool duplicateState);
  //! destructor
  ~StarObjectNumericRuler() override;
  //! try to read a number format zone : 'n'
  static bool readLevel(StarZone &zone, STOFFListLevel &level);
  //! try to read a attribute format : ATTR_EE_PARA_BULLET
  static bool readAttributeLevel(StarZone &zone, int vers, long lastPos, STOFFListLevel &level);
  //! try to read a list : '0' or 'R'
  static bool readList(StarZone &zone, std::shared_ptr<STOFFList> &list);
  //! try to read a SwNumRules zone
  bool read(StarZone &zone);
  //! try to return the list corresponding to a name
  std::shared_ptr<STOFFList> getList(librevenge::RVNGString const &name) const;
  //
  // low level
  //

protected:
  //
  // data
  //

  //! the state
  std::shared_ptr<StarObjectNumericRulerInternal::State> m_numericRulerState;
private:
  StarObjectNumericRuler &operator=(StarObjectNumericRuler const &orig) = delete;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
