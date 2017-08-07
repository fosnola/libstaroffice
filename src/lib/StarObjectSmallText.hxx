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
 * Parser to convert a small text zone/OLE in a StarOffice document
 *
 */
#ifndef STAR_OBJECT_SMALL_TEXT
#  define STAR_OBJECT_SMALL_TEXT

#include <vector>

#include "libstaroffice_internal.hxx"
#include "StarObject.hxx"

namespace StarObjectSmallTextInternal
{
struct State;
}

class StarZone;

/** \brief the main class to read a small StarOffice text zone
 *
 *
 *
 */
class StarObjectSmallText : public StarObject
{
public:
  //! constructor
  StarObjectSmallText(StarObject const &orig, bool duplicateState);
  //! destructor
  ~StarObjectSmallText() override;

  //! try to read a small text object
  bool read(StarZone &zone, long lastPos);

  //! try to send a small text zone
  bool send(std::shared_ptr<STOFFListener> listener, int level=-1);
protected:
  //
  // low level
  //

protected:
  //
  // data
  //

  //! the state
  std::shared_ptr<StarObjectSmallTextInternal::State> m_textState;
private:
  StarObjectSmallText &operator=(StarObjectSmallText const &orig) = delete;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
