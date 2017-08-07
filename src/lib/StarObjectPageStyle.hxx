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
 * Parser to convert a SwPageStyleSheets in a StarOffice document
 *
 */
#ifndef STAR_OBJECT_PAGESTYLE
#  define STAR_OBJECT_PAGESTYLE

#include <ostream>
#include <vector>

#include "libstaroffice_internal.hxx"
#include "StarObject.hxx"

namespace StarObjectPageStyleInternal
{
struct PageDesc;

struct State;
}

class StarState;
class StarZone;

/** \brief the main class to read a SwPageStyleSheets zone
 *
 *
 *
 */
class StarObjectPageStyle : public StarObject
{
public:
  //! constructor
  StarObjectPageStyle(StarObject const &orig, bool duplicateState);
  //! destructor
  ~StarObjectPageStyle() override;
  //! try to read a PageStyle zone
  bool read(StarZone &zone);

  /** try to update the page span (to create text document)*/
  bool updatePageSpans(std::vector<librevenge::RVNGString> const &listNames, std::vector<STOFFPageSpan> &pageSpan, int &numPages);
protected:
  /// try to update the pagespan
  bool updatePageSpan(librevenge::RVNGString const &name, StarState &state);
  //
  // low level
  //

protected:
  //
  // data
  //

  //! the state
  std::shared_ptr<StarObjectPageStyleInternal::State> m_pageStyleState;
private:
  StarObjectPageStyle &operator=(StarObjectPageStyle const &orig) = delete;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
