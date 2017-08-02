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
#ifndef STOFF_GRAPHIC_SHAPE
#  define STOFF_GRAPHIC_SHAPE
#  include <ostream>
#  include <string>
#  include <vector>

#  include "librevenge/librevenge.h"
#  include "libstaroffice_internal.hxx"

class STOFFGraphicStyle;

/** a structure used to define a picture shape */
class STOFFGraphicShape
{
public:
  //! an enum used to define the interface command
  enum Command { C_Connector, C_Ellipse, C_Path, C_Polygon, C_Polyline, C_Rectangle, C_Unknown };
  //! constructor
  STOFFGraphicShape()
    : m_command(C_Unknown)
    , m_bdbox()
    , m_propertyList()
    , m_extra("")
  {
  }
  //! virtual destructor
  virtual ~STOFFGraphicShape();
  //! add shape to property list
  void addTo(librevenge::RVNGPropertyList &list) const;
  //! a print operator
  friend std::ostream &operator<<(std::ostream &o, STOFFGraphicShape const &sh);
  //! the command
  Command m_command;
  //! the shape bdbox in twip
  STOFFBox2f m_bdbox;
  //! the property
  librevenge::RVNGPropertyList m_propertyList;
  //! extra data
  std::string m_extra;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
