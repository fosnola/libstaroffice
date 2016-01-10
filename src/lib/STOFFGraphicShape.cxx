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

/* This header contains code specific to a pict mac file
 */
#include <string.h>

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFGraphicStyle.hxx"

#include "STOFFGraphicShape.hxx"

////////////////////////////////////////////////////////////
// STOFFGraphicShape
////////////////////////////////////////////////////////////
std::ostream &operator<<(std::ostream &o, STOFFGraphicShape const &sh)
{
  o << "box=" << sh.m_bdBox << ",";
  switch (sh.m_command) {
  case STOFFGraphicShape::C_Ellipse:
    o << "ellipse,";
    break;
  case STOFFGraphicShape::C_Polyline:
    o << "polyline,";
    break;
  case STOFFGraphicShape::C_Rectangle:
    o << "rect,";
    break;
  case STOFFGraphicShape::C_Path:
    o << "path,";
    break;
  case STOFFGraphicShape::C_Polygon:
    o << "polygons,";
    break;
  default:
  case STOFFGraphicShape::C_Unknown:
    o << "undef,";
    break;
    break;
  }
  o << "[" << sh.m_propertyList.getPropString().cstr() << "],";
  o << sh.m_extra;
  return o;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:

