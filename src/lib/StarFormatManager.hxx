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
 * FormatManager to read/parse StarOffice format
 *
 */
#ifndef STAR_FORMATMANAGER
#  define STAR_FORMATMANAGER

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

#include "StarWriterStruct.hxx"

class StarState;
class StarObject;

namespace StarFormatManagerInternal
{
struct FormatDef {
  //! constructor
  FormatDef() : m_attributeList()
  {
    for (int &value : m_values) value=0;
    m_values[2]=-1; // no objRef
  }
  //! destructor
  ~FormatDef();
  //! try to update the state
  void updateState(StarState &state) const;
  //! try to send the data to a listener
  bool send(STOFFListenerPtr listener, StarState &state) const;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const;
  //! the pool name, the read name
  librevenge::RVNGString m_names[2];
  //! the attributes list
  std::vector<StarWriterStruct::Attribute> m_attributeList;
  //! nDerived, nPoolId, nObjRef
  int m_values[3];
};

struct State;
}

class StarZone;
class StarObject;

class STOFFCell;

/** \brief the main class to read/.. a StarOffice sdw format
 *
 *
 *
 */
class StarFormatManager
{
public:
  //! constructor
  StarFormatManager();
  //! destructor
  virtual ~StarFormatManager();

  //! try to read a format zone : 'f' or 'l' or 'o' or 'r' or 's'(in TOCX)
  bool readSWFormatDef(StarZone &zone, unsigned char kind, std::shared_ptr<StarFormatManagerInternal::FormatDef> &format, StarObject &doc);
  //! store a named format zone
  void storeSWFormatDef(librevenge::RVNGString const &name, std::shared_ptr<StarFormatManagerInternal::FormatDef> &format);
  //! try to return a named format zone(if possible)
  std::shared_ptr<StarFormatManagerInternal::FormatDef> getSWFormatDef(librevenge::RVNGString const &name) const;
  //! try to read a number formatter type : 'q'
  bool readSWNumberFormatterList(StarZone &zone);
  //! try to read a fly frame list : 'F' (list of 'l' or 'o')
  bool readSWFlyFrameList(StarZone &zone, StarObject &doc, std::vector<std::shared_ptr<StarFormatManagerInternal::FormatDef> > &listFormats);
  //! try to read a format pattern LCL : 'P' (list of 'D') (child of a TOXs)
  bool readSWPatternLCL(StarZone &zone);

  //! try to read a number format (find in attribute)
  bool readNumberFormat(StarZone &zone, long endPos, StarObject &doc);
  //! try to read number formatter type
  bool readNumberFormatter(StarZone &zone);

  //! try to update the cell's data
  void updateNumberingProperties(STOFFCell &cell) const;

  //
  // data
  //
private:
  //! the state
  std::shared_ptr<StarFormatManagerInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
