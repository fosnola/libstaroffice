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
 * FormatManager to read/parse SW StarOffice format
 *
 */
#ifndef SW_FORMATMANAGER
#  define SW_FORMATMANAGER

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

namespace SWFormatManagerInternal
{
struct State;
}

class StarZone;

/** \brief the main class to read/.. a StarOffice sdw format
 *
 *
 *
 */
class SWFormatManager
{
public:
  //! constructor
  SWFormatManager();
  //! destructor
  virtual ~SWFormatManager();

  //! try to read a format zone : 'f' or 'l' or 'o' or 'r' or 's'(in TOCX)
  bool readSWFormatDef(StarZone &zone, char kind, SDWParser &manager);
  //! try to read a number format zone : 'n'
  bool readSWNumberFormat(StarZone &zone);
  //! try to read a number formatter type : 'q'
  bool readSWNumberFormatterList(StarZone &zone);
  //! try to read a fly frame list : 'F' (list of 'l' or 'o')
  bool readSWFlyFrameList(StarZone &zone, SDWParser &manager);
  //! try to read a format pattern LCL : 'P' (list of 'D') (child of a TOXs)
  bool readSWPatternLCL(StarZone &zone);

  //! try to read number formatter type
  bool readNumberFormatter(StarZone &zone);

  //
  // data
  //
private:
  //! the state
  shared_ptr<SWFormatManagerInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
