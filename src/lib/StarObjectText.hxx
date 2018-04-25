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
 * Parser to convert a text zone/OLE in a StarOffice document
 *
 */
#ifndef STAR_OBJECT_TEXT
#  define STAR_OBJECT_TEXT

#include <vector>

#include "libstaroffice_internal.hxx"
#include "StarObject.hxx"

class StarState;

namespace StarObjectTextInternal
{
//! Internal: a basic sone of StarObjectTextInternal
struct Zone {
  //! constructor
  Zone()
  {
  }
  //! destructor
  virtual ~Zone();
  //! try to send the data to a listener
  virtual bool send(STOFFListenerPtr &listener, StarState &state) const=0;
  //! try to inventory the different pages
  virtual void inventoryPage(StarState &/*state*/) const
  {
  }
};

//! Internal: a set of zone
struct Content {
  //! constructor
  Content()
    : m_sectionName("")
    , m_zoneList()
  {
  }
  //! destructor
  ~Content();
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state, bool isFlyer=false) const;
  //! try to inventory the different pages
  void inventoryPages(StarState &state) const;
  //! the section name
  librevenge::RVNGString m_sectionName;
  //! the list of text zone
  std::vector<std::shared_ptr<Zone> > m_zoneList;
};

struct GraphZone;
struct OLEZone;
struct SectionZone;
struct TextZone;
struct State;
}

class StarZone;

/** \brief the main class to read a StarOffice sdw file
 *
 *
 *
 */
class StarObjectText final : public StarObject
{
public:
  //! constructor
  StarObjectText(StarObject const &orig, bool duplicateState);
  //! destructor
  ~StarObjectText() final;

  // try to parse all zone
  bool parse();

  /** try to update the page span (to create draw document)*/
  bool updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages);
  //! try to send the different page
  bool sendPages(STOFFTextListenerPtr &listener);

  //! try to read a image map zone : 'X'
  static bool readSWImageMap(StarZone &zone);

  //! try to read some content : 'N'
  bool readSWContent(StarZone &zone, std::shared_ptr<StarObjectTextInternal::Content> &content);
protected:
  //
  // low level
  //

  //! try to read a text style zone: SfxStyleSheets
  bool readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &fileName);
  //! the main zone
  bool readWriterDocument(STOFFInputStreamPtr input, std::string const &fileName);

  //! the drawing layers ?
  bool readDrawingLayer(STOFFInputStreamPtr input, std::string const &fileName);

protected:
  //! try to read a OLE node : 'g'
  bool readSWGraphNode(StarZone &zone, std::shared_ptr<StarObjectTextInternal::GraphZone> &graphZone);
  //! try to read a SW zone setup : 'J'
  bool readSWJobSetUp(StarZone &zone);
  //! try to read a OLE node : 'O'
  bool readSWOLENode(StarZone &zone, std::shared_ptr<StarObjectTextInternal::OLEZone> &ole);
  //! try to read a section : 'I'
  bool readSWSection(StarZone &zone, std::shared_ptr<StarObjectTextInternal::SectionZone> &section);
  //! try to read some content : 'T'
  bool readSWTextZone(StarZone &zone, std::shared_ptr<StarObjectTextInternal::TextZone> &textZone);
  //
  // data
  //

  //! the state
  std::shared_ptr<StarObjectTextInternal::State> m_textState;
private:
  StarObjectText &operator=(StarObjectText const &orig) = delete;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
