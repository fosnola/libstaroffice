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
 * Parser to convert StarOffice's draw
 *
 */
#ifndef STAR_OBJECT_DRAW
#  define STAR_OBJECT_DRAW

#include <vector>
#include <string>

#include "libstaroffice_internal.hxx"

#include "StarObject.hxx"

namespace StarObjectDrawInternal
{
struct State;
}

class StarObject;
class StarZone;

/** \brief the main class to read a StarOffice draw
 *
 * \note currently, it contains only static function :-~
 *
 */
class StarObjectDraw : public StarObject
{
public:
  //! constructor
  StarObjectDraw(StarObject const &orig, bool duplicateState);
  //! destructor
  virtual ~StarObjectDraw();
  //! try to parse the current object
  bool parse();
  //! try to read a SdrModel zone: "DrMd"
  static bool readSdrModel(StarZone &zone, StarObject &doc);

protected:
  //! try to read a spreadsheet zone: StarDrawDocument .sdd
  bool readDrawDocument(STOFFInputStreamPtr input, std::string const &fileName);
  //! try to read a draw style zone: SfxStyleSheets
  bool readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &fileName);

  //! try to read a SdrLayer zone: "DrLy'
  static bool readSdrLayer(StarZone &zone);
  //! try to read a SdrLayerSet zone: "DrLS'
  static bool readSdrLayerSet(StarZone &zone);
  //! try to read a object zone: "DrOb'
  static bool readSdrObject(StarZone &zone);
  //! try to read a Page/MasterPage zone: "DrPg'
  static bool readSdrPage(StarZone &zone, StarObject &doc);
  //! try to read a Master Page descriptor zone: "DrMP'
  static bool readSdrMPageDesc(StarZone &zone);
  //! try to read a list of Master Page zone: "DrML'
  static bool readSdrMPageDescList(StarZone &zone);

  //! try to read a zone which appear at end of a zone: "DrPg'
  static bool readSdrPageUnknownZone1(StarZone &zone, long lastPos);
protected:
  //
  // data
  //

  //! the state
  shared_ptr<StarObjectDrawInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
