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
 * StarLanguage to read/parse some layout in StarOffice documents
 *
 */
#ifndef STAR_LAYOUT
#  define STAR_LAYOUT

#include <vector>

#include "libstaroffice_internal.hxx"
#include "STOFFDebug.hxx"

class StarObject;
class StarZone;
/** \brief structure to parse a layout in a text zone (very incomplete)
 */
struct StarLayout {
public:
  //! constructor
  StarLayout()
    : m_version(0)
  {
  }
  //! try to read a layout: 'U'
  bool read(StarZone &zone, StarObject &object);
protected:
  //! try to read a sub zone: 'c1' or 'cc', 'cd'
  bool readC1(StarZone &zone, StarObject &object);
  //! try to read a sub zone: 'c2', 'c3', 'c6', 'c8', 'c9', 'ce', 'd2', 'd3', 'd7', 'e3' or 'f2'
  bool readC2(StarZone &zone, StarObject &object);
  //! try to read a sub zone: 'c4' or 'c7'
  bool readC4(StarZone &zone, StarObject &object);
  //! try to read a sub zone: 'd0'
  bool readD0(StarZone &zone, StarObject &object);
  //! try to read a sub zone: 'd8'
  bool readD8(StarZone &zone, StarObject &object);

  //! try to read a child of a zone
  bool readChild(StarZone &zone, StarObject &object);
  //! try to read a block header
  bool readHeader(StarZone &zone, libstoff::DebugStream &f, int &type, int valueMode=1) const;
  //! try to read a small data block
  bool readDataBlock(StarZone &zone, libstoff::DebugStream &f) const;
  /** try to read a positive number of 1|2 bytes depending on the version:
      - if m_version<vers, read 2 bytes
      - if not, read 1 byte and if result=0, read 2 bytes
   */
  int readNumber(STOFFInputStreamPtr input, int vers) const;
  //! the version
  uint16_t m_version;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
