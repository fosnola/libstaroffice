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
 * class to parse/store an object corresponding to an OLE directory
 *
 */
#ifndef STAR_OBJECT
#  define STAR_OBJECT

#include <vector>
#include <stack>
#include <libstaroffice/STOFFDocument.hxx>

#include "STOFFDebug.hxx"
#include "STOFFOLEParser.hxx"

#include "StarItemPool.hxx"

namespace StarObjectInternal
{
struct State;
}

class StarAttributeManager;
class StarFormatManager;
class StarItemSet;

/** \brief an object corresponding to an OLE directory
 *
 *
 *
 */
class StarObject
{
public:
  //! constructor
  StarObject(char const *passwd, std::shared_ptr<STOFFOLEParser> &oleParser, std::shared_ptr<STOFFOLEParser::OleDirectory> &directory);
  //! destructor
  virtual ~StarObject();

  //
  // basic
  //

  //! try to parse data
  bool parse();
  //! returns the document kind
  STOFFDocument::Kind getDocumentKind() const;
  //! returns the document password (the password given by the user)
  char const *getPassword() const
  {
    return m_password;
  }
  //! returns the object directory
  std::shared_ptr<STOFFOLEParser::OleDirectory> getOLEDirectory()
  {
    return m_directory;
  }
  //! returns the attribute manager
  std::shared_ptr<StarAttributeManager> getAttributeManager();
  //! returns the format manager
  std::shared_ptr<StarFormatManager> getFormatManager();
  //! returns the meta data (filled by readSfxDocumentInformation)
  librevenge::RVNGPropertyList const &getMetaData() const
  {
    return m_metaData;
  }
  //! returns the ith user meta data
  librevenge::RVNGString getUserNameMetaData(int i) const;
  // the document pool
  //! clean each pool
  void cleanPools();
  //! returns a new item pool for this document
  std::shared_ptr<StarItemPool> getNewItemPool(StarItemPool::Type type);
  /** check if a pool corresponding to a given type is opened if so returned it.
      \note if isInside is set only look for inside pool
   */
  std::shared_ptr<StarItemPool> findItemPool(StarItemPool::Type type, bool isInside);
  //! returns the current all/inside pool
  std::shared_ptr<StarItemPool> getCurrentPool(bool onlyInside=true);

  //! try to read persist data
  bool readPersistData(StarZone &zone, long endPos);
  //! try to read a spreadshet style zone: SfxStyleSheets
  bool readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name);
  //! try to read a list of item
  bool readItemSet(StarZone &zone, std::vector<STOFFVec2i> const &limits, long endPos,
                   StarItemSet &itemSet, StarItemPool *pool=nullptr, bool isDirect=false);
protected:
  //!  the "persist elements" small ole: the list of object
  bool readPersistElements(STOFFInputStreamPtr input, std::string const &name);
  //! try to read the document information : "SfxDocumentInformation"
  bool readSfxDocumentInformation(STOFFInputStreamPtr input, std::string const &name);
  //! try to read the preview : "SfxPreview"
  bool readSfxPreview(STOFFInputStreamPtr input, std::string const &name);
  //! try to read the windows information : "SfxWindows"
  bool readSfxWindows(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! try to read the "Star Framework Config File"
  bool readStarFrameworkConfigFile(STOFFInputStreamPtr input, libstoff::DebugFile &ascii);
  //! try to read an item in a "Star Framework Config File"
  bool readStarFrameworkConfigItem(STOFFEntry &entry, STOFFInputStreamPtr input, libstoff::DebugFile &ascii);

  //
  // data
  //
protected:
  //! copy constructor
  StarObject(StarObject const &orig, bool duplicateState);
  //! the document password
  char const *m_password;
  //! the ole parser
  std::shared_ptr<STOFFOLEParser> m_oleParser;
  //! the directory
  std::shared_ptr<STOFFOLEParser::OleDirectory> m_directory;

  //! the state
  std::shared_ptr<StarObjectInternal::State> m_state;
  //! the meta data
  librevenge::RVNGPropertyList m_metaData;

private:
  StarObject(StarObject const &orig) = delete;
  StarObject &operator=(StarObject const &orig) = delete;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
