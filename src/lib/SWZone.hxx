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
 * SWZone zone of StarOffice document
 *
 */
#ifndef SW_ZONE
#  define SW_ZONE

#include <vector>
#include <stack>

#include "STOFFDebug.hxx"

/** \brief a SW zone in a StarOffice file
 *
 *
 *
 */
class SWZone
{
public:
  //! constructor
  SWZone(STOFFInputStreamPtr input, std::string const &ascName, std::string const &zoneName);
  //! destructor
  virtual ~SWZone();
  //! read the zone header
  bool readHeader();

  //! returns the StarOffice version: 3-5
  int getVersion() const
  {
    return m_version;
  }
  //! returns the StarOffice document version
  int getDocumentVersion() const
  {
    return m_documentVersion;
  }
  //! checks if the document is compatible with vers
  int isCompatibleWith(int vers) const
  {
    return m_documentVersion>=vers;
  }
  //! checks if the document is compatible with vers1 and not vers2
  int isCompatibleWith(int vers1, int vers2) const
  {
    return m_documentVersion>=vers1 && m_documentVersion<vers2;
  }
  //! checks if the document is compatible with vers1 and not vers2 or vers3
  int isCompatibleWith(int vers1, int vers2, int vers3) const
  {
    return (m_documentVersion>=vers1 && m_documentVersion<vers2) ||
           m_documentVersion>=vers3;
  }
  //! checks if the document is compatible with vers1 and not vers2 or vers3 and not vers4
  int isCompatibleWith(int vers1, int vers2, int vers3, int vers4) const
  {
    return (m_documentVersion>=vers1 && m_documentVersion<vers2) ||
           (m_documentVersion>=vers3 && m_documentVersion<vers4);
  }

  //! try to open a record
  bool openRecord(char &type);
  //! try to close a record
  bool closeRecord(char type);
  //! returns the record level
  int getRecordLevel() const
  {
    return (int) m_positionStack.size();
  }
  //! returns the actual record last position
  long getRecordLastPosition() const
  {
    if (m_positionStack.empty()) {
      STOFF_DEBUG_MSG(("SWZone::getRecordLastPosition: can not find last position\n"));
      return 0;
    }
    return m_positionStack.top();
  }

  //! try to open a cflag zone
  unsigned char openFlagZone();
  //! close the cflag zone
  void closeFlagZone();
  //! returns the flag last position
  long getFlagLastPosition() const
  {
    return m_flagEndZone;
  }

  //! try to read an unicode string
  bool readString(librevenge::RVNGString &string, int encoding=-1) const;
  //! try to read a pool of strings
  bool readStringsPool();
  //! try to return a pool name
  bool getPoolName(int poolId, librevenge::RVNGString &res) const
  {
    res="";
    if (poolId>=0 && poolId<(int) m_poolList.size()) {
      res=m_poolList[size_t(poolId)];
      return true;
    }
    if (poolId==0xFFF0) return true;
    STOFF_DEBUG_MSG(("SWZone::getPoolName: can not find pool name for %d\n", poolId));
    return false;
  }
  //! return the zone input
  STOFFInputStreamPtr input()
  {
    return m_input;
  }
  //! returns the ascii file
  libstoff::DebugFile &ascii()
  {
    return m_ascii;
  }
  //! return the zone name
  std::string const &name() const
  {
    return m_zoneName;
  }
protected:
  //
  // low level
  //

  //! try to read the record sizes
  bool readRecordSizes(long pos);

  //
  // data
  //

  //! the input stream
  STOFFInputStreamPtr m_input;
  //! the ascii zone
  libstoff::DebugFile m_ascii;
  //! the zone version
  int m_version;
  //! the document version
  int m_documentVersion;
  //! the zone encoding
  int m_encoding;
  //! the file ascii name
  std::string m_asciiName;
  //! the zone name
  std::string m_zoneName;

  //! the type stack
  std::stack<char> m_typeStack;
  //! the position stack
  std::stack<long> m_positionStack;
  //! other position to end position zone
  std::map<long, long> m_beginToEndMap;
  //! end of a cflags zone
  long m_flagEndZone;

  //! the pool name list
  std::vector<librevenge::RVNGString> m_poolList;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
