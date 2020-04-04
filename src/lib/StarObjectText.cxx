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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFList.hxx"
#include "STOFFOLEParser.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFParagraph.hxx"
#include "STOFFSection.hxx"
#include "STOFFSubDocument.hxx"
#include "STOFFTextListener.hxx"

#include "SWFieldManager.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarObject.hxx"
#include "StarFileManager.hxx"
#include "StarGraphicStruct.hxx"
#include "StarItemPool.hxx"
#include "StarLayout.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectMath.hxx"
#include "StarObjectModel.hxx"
#include "StarObjectNumericRuler.hxx"
#include "StarObjectPageStyle.hxx"
#include "StarObjectSpreadsheet.hxx"
#include "StarState.hxx"
#include "StarTable.hxx"
#include "StarWriterStruct.hxx"
#include "StarZone.hxx"

#include "StarObjectText.hxx"

/** Internal: the structures of a StarObjectText */
namespace StarObjectTextInternal
{
////////////////////////////////////////
//! Internal: the subdocument of a StarObjectSpreadsheet
class SubDocument final : public STOFFSubDocument
{
public:
  explicit SubDocument(StarObjectTextInternal::Content const &content, StarState &state) :
    STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry()), m_content(content), m_state(state) {}

  //! destructor
  ~SubDocument() final {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const final
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    auto const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (&m_content != &sDoc->m_content) return true;
    return false;
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;

protected:
  //! the content
  StarObjectTextInternal::Content const &m_content;
  //! the state
  StarState &m_state;
private:
  SubDocument(SubDocument const &);
  SubDocument &operator=(SubDocument const &);
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::SubDocument::parse: no listener\n"));
    return;
  }
  m_content.send(listener, m_state);
}

////////////////////////////////////////
// Zone, Content function
Zone::~Zone()
{
}

Content::~Content()
{
}

void Content::inventoryPages(StarState &state) const
{
  if (!state.m_global->m_pool) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Content::inventoryPages: can not find the pool\n"));
    return;
  }
  for (auto &z : m_zoneList) {
    if (z)
      z->inventoryPage(state);
    if (state.m_global->m_pageNameList.empty())
      state.m_global->m_pageNameList.push_back("");
  }
}

////////////////////////////////////////
//! Internal: a formatZone of StarObjectTextInteral
struct FormatZone final : public Zone {
  //! constructor
  explicit FormatZone(std::shared_ptr<StarFormatManagerInternal::FormatDef> &format)
    : Zone()
    , m_format(format)
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! the format
  std::shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

bool FormatZone::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::FormatZone::send: call without listener\n"));
    return false;
  }
  if (!m_format) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::FormatZone::send: can not find the format\n"));
    return false;
  }
  StarState cState(*state.m_global);
  return m_format->send(listener, cState);
}

////////////////////////////////////////
//! Internal: a graphZone of StarObjectTextInteral
struct GraphZone final : public Zone {
  //! constructor
  explicit GraphZone(std::shared_ptr<STOFFOLEParser> &oleParser)
    : Zone()
    , m_oleParser(oleParser)
    , m_attributeList()
    , m_contour()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! the ole parser
  std::shared_ptr<STOFFOLEParser> m_oleParser;
  //! the graph name, the fltName, the replace text
  librevenge::RVNGString m_names[3];
  //! the attributes list
  std::vector<StarWriterStruct::Attribute> m_attributeList;
  //! the contour(useme)
  StarGraphicStruct::StarPolygon m_contour;
};

bool GraphZone::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::GraphZone::send: call without listener\n"));
    return false;
  }
  if (m_names[0].empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::GraphZone::send: can not find the graph name\n"));
    return false;
  }
  STOFFEmbeddedObject localPicture;
  if (!m_oleParser || !StarFileManager::readEmbeddedPicture(m_oleParser, m_names[0].cstr(), localPicture) || localPicture.isEmpty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal: sorry, can not find object %s\n", m_names[0].cstr()));
    return false;
  }
  STOFFGraphicStyle style=state.m_graphic;
  state.m_frame.addTo(style.m_propertyList);
  listener->insertPicture(state.m_frame, localPicture, style);
  return true;
}

////////////////////////////////////////
//! Internal: a OLEZone of StarObjectTextInteral
struct OLEZone final : public Zone {
  //! constructor
  OLEZone()
    : Zone()
    , m_name("")
    , m_replaceText("")
    , m_oleParser()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! the OLE name
  librevenge::RVNGString m_name;
  //! the replacement text
  librevenge::RVNGString m_replaceText;
  //! the ole parser
  std::shared_ptr<STOFFOLEParser> m_oleParser;
};

bool OLEZone::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::OLEZone::send: call without listener\n"));
    return false;
  }
  if (m_name.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::OLEZone::send: call without object name\n"));
    return false;
  }
  if (!m_oleParser) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::OLEZone::send: call without ole parser\n"));
    return false;
  }
  STOFFEmbeddedObject localPicture;
  std::shared_ptr<StarObject> localObj;
  auto dir=m_oleParser->getDirectory(m_name.cstr());
  STOFFGraphicStyle style=state.m_graphic;
  state.m_frame.addTo(style.m_propertyList);
  if (!dir || !StarFileManager::readOLEDirectory(m_oleParser, dir, localPicture, localObj) || localPicture.isEmpty()) {
    if (!localObj) {
      STOFF_DEBUG_MSG(("StarObjectTextInternal::OLEZone::send: sorry, can not find object %s\n", m_name.cstr()));
      return false;
    }
    auto chart=std::dynamic_pointer_cast<StarObjectChart>(localObj);
    if (chart && chart->send(listener, state.m_frame, style))
      return true;
    auto math=std::dynamic_pointer_cast<StarObjectMath>(localObj);
    if (math && math->send(listener, state.m_frame, style))
      return true;
    if (std::dynamic_pointer_cast<StarObjectText>(localObj)) {
      STOFF_DEBUG_MSG(("StarObjectTextInternal::OLEZone::send: sorry, unsure how to send a text object %s\n", m_name.cstr()));
    }
    else {
      STOFF_DEBUG_MSG(("StarObjectTextInternal::OLEZone::send: sorry, find unexpected object for %s\n", m_name.cstr()));
    }
    return false;
  }
  listener->insertPicture(state.m_frame, localPicture, style);

  return true;
}

////////////////////////////////////////
//! Internal: a sectionZone of StarObjectTextInteral
struct SectionZone final : public Zone {
  //! constructor
  SectionZone()
    : Zone()
    , m_name("")
    , m_condition("")
    , m_linkName("")
    , m_type(0)
    , m_flags(0)
    , m_format()
    , m_content()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! the section name
  librevenge::RVNGString m_name;
  //! the section condition
  librevenge::RVNGString m_condition;
  //! the section link name
  librevenge::RVNGString m_linkName;
  //! the section type
  int m_type;
  //! the section flag
  int m_flags;
  //! the format
  std::shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
  //! the content
  std::shared_ptr<Content> m_content;
};

bool SectionZone::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::SectionZone::send: call without listener\n"));
    return false;
  }
  // checkme: do we need to create a new section here
  if (m_content)
    m_content->send(listener, state);
  else {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::SectionZone::send: call without content\n"));
  }
  return true;
}

////////////////////////////////////////
//! Internal: a textZone of StarObjectTextInteral
struct TextZone final : public Zone {
  //! constructor
  TextZone()
    : Zone()
    , m_text()
    , m_textSourcePosition()
    , m_styleName("")
    , m_level(-1)
    , m_charAttributeList()
    , m_formatList()
    , m_list()
    , m_markList()
  {
  }
  //! try to inventory the different pages
  void inventoryPage(StarState &state) const final;
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! the text
  std::vector<uint32_t> m_text;
  //! the text initial position
  std::vector<size_t> m_textSourcePosition;
  //! the style name
  librevenge::RVNGString m_styleName;
  //! the level -1=none, 200: keep in list with no bullet
  int m_level;
  //! the character item list
  std::vector<StarWriterStruct::Attribute> m_charAttributeList;
  //! the format
  std::vector<std::shared_ptr<StarFormatManagerInternal::FormatDef> > m_formatList;
  //! the list (if defined)
  std::shared_ptr<STOFFList> m_list;
  //! the mark
  std::vector<StarWriterStruct::Mark> m_markList;
};

void TextZone::inventoryPage(StarState &state) const
{
  size_t numPages=state.m_global->m_pageNameList.size();
  if (state.m_styleName!=m_styleName) {
    state.reinitializeLineData();
    state.m_styleName=m_styleName;
    if (state.m_global->m_pool && !m_styleName.empty()) { // checkme
      auto const *style=state.m_global->m_pool->findStyleWithFamily(m_styleName, StarItemStyle::F_Paragraph);
      if (style) {
        for (auto it : style->m_itemSet.m_whichToItemMap) {
          if (it.second && it.second->m_attribute)
            it.second->m_attribute->addTo(state);
        }
      }
      else {
        STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::inventoryPage: can not find style %s\n", m_styleName.cstr()));
      }
    }
  }
  StarState lineState(state);
  for (auto const &attrib : m_charAttributeList) {
    if ((attrib.m_position[1]<0 && attrib.m_position[0]>0) || attrib.m_position[0]>0)
      continue;
    if (!attrib.m_attribute)
      continue;
    attrib.m_attribute->addTo(lineState);
  }
  if (lineState.m_global->m_pageNameList.size()!=state.m_global->m_pageNameList.size()) {
    state.m_global->m_pageName=lineState.m_global->m_pageName;
    state.m_global->m_pageNameList.push_back(state.m_global->m_pageName);
  }
  else if (lineState.m_global->m_pageNameList.size()==numPages && lineState.m_break==4)
    state.m_global->m_pageNameList.push_back("");
}

bool TextZone::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: call without listener\n"));
    return false;
  }

  if (m_list) state.m_global->m_list=listener->getListManager()->addList(m_list);
  size_t numPages=state.m_global->m_pageNameList.size();
  if (state.m_styleName!=m_styleName) {
    state.reinitializeLineData();
    state.m_paragraph=STOFFParagraph();
    state.m_styleName=m_styleName;
    if (state.m_global->m_pool && !m_styleName.empty()) { // checkme
      StarItemStyle const *style=state.m_global->m_pool->findStyleWithFamily(m_styleName, StarItemStyle::F_Paragraph);
      if (style) {
        if (style->m_outlineLevel>=0 && style->m_outlineLevel<20) {
          state.m_paragraph.m_outline=true;
          state.m_paragraph.m_listLevelIndex=style->m_outlineLevel+1;
        }
        for (auto it : style->m_itemSet.m_whichToItemMap) {
          if (it.second && it.second->m_attribute)
            it.second->m_attribute->addTo(state);
        }
#if 0
        std::cerr << "Para[" << m_styleName.cstr() << "]:" << style->m_itemSet.printChild() << "\n";
#endif
      }
      else {
        STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: can not find style %s\n", m_styleName.cstr()));
      }
    }
  }
  STOFFFont mainFont=state.m_font;
  listener->setFont(mainFont);
  if (!m_markList.empty()) {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: sorry mark are not implemented\n"));
      first=false;
    }
  }
  if (state.m_flyCnt || state.m_footnote || state.m_field) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a data in mainFont\n"));
  }
  if (!state.m_link.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a link in mainFont\n"));
  }
  if (!state.m_refMark.empty()) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find a refMark in mainFont\n"));
  }
  std::set<size_t> modPosSet;
  modPosSet.insert(0);
  for (auto const &attrib : m_charAttributeList) {
    if (attrib.m_position[0]>0)
      modPosSet.insert(size_t(attrib.m_position[0]));
    if (attrib.m_position[1]>0)
      modPosSet.insert(size_t(attrib.m_position[1]));
  }
  std::multimap<size_t, std::shared_ptr<StarFormatManagerInternal::FormatDef> > posToFormat;
  for (auto const &c : m_formatList) {
    if (!c) continue;
    // either a paragraph anchor or a "at char" anchor
    StarState fState(state);
    c->updateState(fState);
    size_t lastIndex=m_textSourcePosition.empty() ? 0 : m_textSourcePosition.back();
    size_t cPos=fState.m_frame.m_anchorIndex<0 ? 0 : fState.m_frame.m_anchorIndex>int(lastIndex) ? lastIndex :
                size_t(fState.m_frame.m_anchorIndex);
    posToFormat.insert(std::multimap<size_t, std::shared_ptr<StarFormatManagerInternal::FormatDef> >::value_type(cPos, c));
  }
  auto posSetIt=modPosSet.begin();
  int endLinkPos=-1, endRefMarkPos=-1;
  librevenge::RVNGString refMarkString;
  StarState lineState(state);
  state.m_break=0;
  bool newPage=false;
  for (size_t c=0; c<= m_text.size(); ++c) {
    bool fontChange=false;
    size_t srcPos=c<m_textSourcePosition.size() ? m_textSourcePosition[c] : m_textSourcePosition.empty() ? 0 : 10000;
    while (posSetIt!=modPosSet.end() && *posSetIt <= srcPos) {
      ++posSetIt;
      fontChange=true;
    }
    std::shared_ptr<StarAttribute> flyCnt;
    std::shared_ptr<StarAttribute> footnote;
    std::shared_ptr<SWFieldManagerInternal::Field> field;
    librevenge::RVNGString linkString;
    bool startRefMark=false;
    bool softHyphen=false;
    bool hardBlank=false;
    if (fontChange) {
      lineState.reinitializeLineData();
      lineState.m_font=mainFont;
      STOFFFont &font=lineState.m_font;
      for (auto const &attrib : m_charAttributeList) {
        if ((attrib.m_position[1]<0 && attrib.m_position[0]>=0 && attrib.m_position[0]!=int(srcPos)) ||
            (attrib.m_position[0]>=0 && attrib.m_position[0]>int(srcPos)) ||
            (attrib.m_position[1]>=0 && attrib.m_position[1]<=int(srcPos)))
          continue;
        if (!attrib.m_attribute)
          continue;
        attrib.m_attribute->addTo(lineState);
        if (!footnote && lineState.m_footnote)
          footnote=attrib.m_attribute;
        if (!flyCnt && lineState.m_flyCnt)
          flyCnt=attrib.m_attribute;
        if (c==0) {
          switch (lineState.m_break) {
          case 0:
            break;
          case 1:
            listener->insertBreak(STOFFListener::ColumnBreak);
            break;
          case 4:
            newPage=true;
            listener->insertBreak(STOFFListener::PageBreak);
            if (state.m_global->m_pageNameList.size()==numPages)
              state.m_global->m_pageNameList.push_back("");
            break;
          default: {
            static bool first=true;
            if (first) {
              first=false;
              STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: unexpected break\n"));
            }
            break;
          }
          }
          lineState.m_break=0;
        }
        if (lineState.m_field) {
          if (int(srcPos)==attrib.m_position[0])
            field=lineState.m_field;
          lineState.m_field.reset();
        }
        if (!lineState.m_link.empty()) {
          if (endLinkPos<0) {
            linkString=lineState.m_link;
            endLinkPos=int(attrib.m_position[1]<0 ? attrib.m_position[0] : attrib.m_position[1]);
          }
          lineState.m_link.clear();
        }
        if (!lineState.m_refMark.empty()) {
          if (endRefMarkPos<0) {
            refMarkString=lineState.m_refMark;
            endRefMarkPos=int(attrib.m_position[1]<0 ? attrib.m_position[0] : attrib.m_position[1]);
            startRefMark=true;
          }
          else {
            STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: multiple refmark is not implemented\n"));
          }
          lineState.m_refMark.clear();
        }
      }
      softHyphen=font.m_softHyphen;
      hardBlank=font.m_hardBlank;
      listener->setFont(font);
      if (c==0) {
        int level=m_level;
        if (level==200) {
          level=state.m_global->m_listLevel;
          lineState.m_paragraph.m_bulletVisible=true; // useMe
        }
        if (!lineState.m_paragraph.m_outline) {
          if (level>=0 && (!state.m_global->m_list || state.m_global->m_list->numLevels()<=int(level))) {
            STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: oops can not find the list\n"));
            level=-1;
          }
          if (level>=0) {
            lineState.m_paragraph.m_listLevel = state.m_global->m_list->getLevel(level);
            lineState.m_paragraph.m_listId = state.m_global->m_list->getId();
            lineState.m_paragraph.m_listLevelIndex = level+1;
          }
          else
            lineState.m_paragraph.m_listLevelIndex=0;
        }
        listener->setParagraph(lineState.m_paragraph);
      }
      static bool first=true;
      if (first && lineState.m_content) {
        first=false;
        STOFF_DEBUG_MSG(("StarObjectTextInternal::TextZone::send: find unexpected content zone\n"));
      }
    }
    if (c==0 && !newPage && numPages && numPages!=lineState.m_global->m_pageNameList.size())
      listener->insertBreak(STOFFListener::SoftPageBreak);
    for (auto it=posToFormat.lower_bound(srcPos); it!=posToFormat.upper_bound(srcPos); ++it) {
      StarState cState(*lineState.m_global);
      it->second->send(listener, cState);
    }

    if (!linkString.empty()) {
      STOFFLink link;
      link.m_HRef=linkString.cstr();
      listener->openLink(link);
    }
    if (!refMarkString.empty() && startRefMark) {
      STOFFField cField;
      cField.m_propertyList.insert("librevenge:field-type", "text:reference-mark-start");
      cField.m_propertyList.insert("text:name", refMarkString);
      listener->insertField(cField);
    }
    if (endLinkPos>=0 && int(c)==endLinkPos) {
      listener->closeLink();
      endLinkPos=-1;
    }
    if (endRefMarkPos>=0 && int(c)==endRefMarkPos) {
      STOFFField cField;
      cField.m_propertyList.insert("librevenge:field-type", "text:reference-mark-end");
      cField.m_propertyList.insert("text:name", refMarkString);
      listener->insertField(cField);
      endRefMarkPos=-1;
    }
    if (footnote) {
      StarState cState(*state.m_global);
      footnote->send(listener, cState);
    }
    else if (flyCnt) {
      StarState cState(*state.m_global);
      flyCnt->send(listener, cState);
    }
    else if (field) {
      StarState cState(*state.m_global);
      field->send(listener, cState);
    }
    else if (c==m_text.size())
      break;
    else if (hardBlank)
      listener->insertUnicode(0xa0);
    else if (softHyphen)
      listener->insertUnicode(0xad);
    else if (m_text[c]==0x9)
      listener->insertTab();
    else if (m_text[c]==0xa)
      listener->insertEOL(true);
    else
      listener->insertUnicode(m_text[c]);
  }
  if (endLinkPos>=0) // check that not link is opened
    listener->closeLink();
  if (endRefMarkPos>=0) { // check that not refMark is opened
    STOFFField cField;
    cField.m_propertyList.insert("librevenge:field-type", "text:reference-mark-end");
    cField.m_propertyList.insert("text:name", refMarkString);
    listener->insertField(cField);
  }
  return true;
}

//! Internal: a table of StarObjectTextInteral
struct Table final : public Zone {
  //! constructor
  Table()
    : Zone()
    , m_table()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! the table
  std::shared_ptr<StarTable> m_table;
};

bool Table::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Table::send: call without listener\n"));
    return false;
  }
  if (!m_table) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Table::send: can not find the table\n"));
    return false;
  }
  return m_table->send(listener, state);
}

bool Content::send(STOFFListenerPtr &listener, StarState &state, bool isFlyer) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectTextInternal::Content::send: call without listener\n"));
    return false;
  }
  if (isFlyer) {
    // check if we need to create a textbox
    for (size_t t=0; t<m_zoneList.size(); ++t) {
      if (!std::dynamic_pointer_cast<TextZone>(m_zoneList[t]))
        continue;
      StarState cState(state.m_global);
      auto subDoc = std::make_shared<SubDocument>(*this, cState);
      STOFFGraphicStyle style=cState.m_graphic;
      state.m_frame.addTo(style.m_propertyList);
      listener->insertTextBox(state.m_frame, subDoc, style);
      return true;
    }
  }
  StarState cState(state.m_global);
  cState.m_frame=state.m_frame;
  for (size_t t=0; t<m_zoneList.size(); ++t) {
    if (m_zoneList[t])
      m_zoneList[t]->send(listener, cState);
    if (t+1!=m_zoneList.size())
      listener->insertEOL();
  }
  return true;
}

////////////////////////////////////////
//! Internal: the state of a StarObjectText
struct State {
  //! constructor
  State()
    : m_numPages(0)
    , m_numGraphicPages(0)
    , m_mainContent()
    , m_flyList()
    , m_numericRuler()
    , m_pageStyle()
    , m_model()
  {
  }
  //! the number of pages
  int m_numPages;
  //! the graphic number of pages
  int m_numGraphicPages;
  //! the main content
  std::shared_ptr<Content> m_mainContent;
  //! the list of fly zone
  std::vector<std::shared_ptr<StarFormatManagerInternal::FormatDef> > m_flyList;
  //! the numeric ruler
  std::shared_ptr<StarObjectNumericRuler> m_numericRuler;
  //! the page style
  std::shared_ptr<StarObjectPageStyle> m_pageStyle;
  //! the drawing model
  std::shared_ptr<StarObjectModel> m_model;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectText::StarObjectText(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_textState(new StarObjectTextInternal::State)
{
}

StarObjectText::~StarObjectText()
{
  cleanPools();
}

////////////////////////////////////////////////////////////
// send the data
////////////////////////////////////////////////////////////
bool StarObjectText::updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages)
{
  numPages=0;

  auto pool=findItemPool(StarItemPool::T_WriterPool, false);
  StarState state(pool.get(), *this);
  state.m_global->m_objectModel=m_textState->m_model;
  if (m_textState->m_mainContent)
    m_textState->m_mainContent->inventoryPages(state);
  if (m_textState->m_pageStyle)
    m_textState->m_pageStyle->updatePageSpans(state.m_global->m_pageNameList, pageSpan, numPages);
  else {
    numPages=1000;
    STOFFPageSpan ps;
    ps.m_pageSpan=numPages;
    pageSpan.clear();
    pageSpan.push_back(ps);
  }
  m_textState->m_numPages=numPages;
  if (m_textState->m_model) {
    std::vector<STOFFPageSpan> modelPageSpan;
    m_textState->m_model->updatePageSpans(modelPageSpan, m_textState->m_numGraphicPages);
  }
  return numPages>0;
}

bool StarObjectText::sendPages(STOFFTextListenerPtr &listener)
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectText::sendPages: can not find the listener\n"));
    return false;
  }
  if (!m_textState->m_mainContent) {
    STOFF_DEBUG_MSG(("StarObjectText::sendPages: can not find any content\n"));
    return true;
  }
  /*
  if (m_textState->m_model) {
    // send the graphics which are in the model
    for (int i=0; i<=m_textState->m_numGraphicPages; ++i)
      m_textState->m_model->sendPage(i, listener);
  }
  */
  auto pool=findItemPool(StarItemPool::T_WriterPool, false);
  // then send the frames relative to the page
  for (auto fly : m_textState->m_flyList) {
    if (!fly) continue;
    StarState state(pool.get(), *this);
    state.m_global->m_objectModel=m_textState->m_model;
    fly->send(listener, state);
  }
  // finally send the text content
  StarState state(pool.get(), *this);
  state.m_global->m_objectModel=m_textState->m_model;
  state.m_global->m_numericRuler=m_textState->m_numericRuler;
  STOFFListenerPtr basicListener(listener);
  m_textState->m_mainContent->send(basicListener, state);
  return true;
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
bool StarObjectText::parse()
{
  if (!getOLEDirectory() || !getOLEDirectory()->m_input) {
    STOFF_DEBUG_MSG(("StarObjectText::parser: error, incomplete document\n"));
    return false;
  }
  auto &directory=*getOLEDirectory();
  StarObject::parse();
  auto unparsedOLEs=directory.getUnparsedOles();
  STOFFInputStreamPtr input=directory.m_input;
  StarFileManager fileManager;
  STOFFInputStreamPtr mainOle; // let store the StarWriterDocument to read it in last position
  std::string mainName;
  for (auto const &name : unparsedOLEs) {
    STOFFInputStreamPtr ole = input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObjectText::parse: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    auto pos = name.find_last_of('/');
    std::string base;
    if (pos == std::string::npos) base = name;
    else if (pos == 0) base = name.substr(1);
    else
      base = name.substr(pos+1);
    ole->setReadInverted(true);
    if (base=="SwNumRules") {
      try {
        StarZone zone(ole, name, "StarNumericList", getPassword());
        std::shared_ptr<StarObjectNumericRuler> numericRuler(new StarObjectNumericRuler(*this,true));
        if (numericRuler->read(zone))
          m_textState->m_numericRuler=numericRuler;
      }
      catch (...) {
      }
      continue;
    }
    if (base=="SwPageStyleSheets") {
      try {
        StarZone zone(ole, name, "StarPageStyleSheets", getPassword());
        std::shared_ptr<StarObjectPageStyle> pageStyle(new StarObjectPageStyle(*this,true));
        if (pageStyle->read(zone))
          m_textState->m_pageStyle=pageStyle;
      }
      catch (...) {
      }
      continue;
    }

    if (base=="DrawingLayer") {
      readDrawingLayer(ole,name);
      continue;
    }
    if (base=="SfxStyleSheets") {
      readSfxStyleSheets(ole,name);
      continue;
    }
    if (base=="StarWriterDocument") {
      mainOle=ole;
      mainName=name;
      continue;
    }
    if (base!="BasicManager2") {
      STOFF_DEBUG_MSG(("StarObjectText::parse: find unexpected ole %s\n", name.c_str()));
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    libstoff::DebugStream f;
    f << "Entries(" << base << "):";
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    asciiFile.reset();
  }
  if (!mainOle) {
    STOFF_DEBUG_MSG(("StarObjectText::parser: can not find the main writer document\n"));
    return false;
  }
  readWriterDocument(mainOle,mainName);
  return true;
}

bool StarObjectText::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  if (getDocumentKind()!=STOFFDocument::STOFF_K_TEXT) {
    STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: called with unexpected document\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SfxStyleSheets)");
    return false;
  }
  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  auto pool=getNewItemPool(StarItemPool::T_WriterPool);
  auto mainPool=pool;
  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: create extra pool of type %d\n", int(pool->getType())));
      }
      if (!mainPool) mainPool=pool;
      pool.reset();
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->isEnd()) return true;
  long pos=input->tell();
  if (!mainPool || !mainPool->readStyles(zone, *this)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: can not read a style pool\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  mainPool->updateStyles();
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readSfxStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SfxStyleSheets):###extra");
  }
  return true;
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////
bool StarObjectText::readSWContent(StarZone &zone, std::shared_ptr<StarObjectTextInternal::Content> &content)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='N' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3sectn.cxx: InContents
  if (content) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWContent: oops, the content zone is already created\n"));
  }
  else
    content.reset(new StarObjectTextInternal::Content);
  libstoff::DebugStream f;
  f << "Entries(SWContent)[" << zone.getRecordLevel() << "]:";
  if (zone.isCompatibleWith(5))
    zone.openFlagZone();
  int nNodes;
  if (zone.isCompatibleWith(0x201))
    nNodes=int(input->readULong(4));
  else {
    if (zone.isCompatibleWith(5)) {
      auto id=int(input->readULong(2));
      // dummy sectid, so probably ok if we do not find the pool name
      if (id>=zone.getNumPoolNames() || !zone.getPoolName(id, content->m_sectionName))
        f << "#sectId=" << id << ",";
      else
        f << "section[name]=" << content->m_sectionName.cstr() << ",";
    }
    nNodes=int(input->readULong(2));
  }
  f << "N=" << nNodes << ",";
  if (zone.isCompatibleWith(5))
    zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<nNodes; ++i) {
    if (input->tell()>=lastPos) break;
    pos=input->tell();
    int cType=input->peek();
    bool done=false;
    switch (cType) {
    case 'E': {
      std::shared_ptr<StarTable> table(new StarTable);
      done=table->read(zone, *this);
      if (done) {
        std::shared_ptr<StarObjectTextInternal::Table> tableZone(new StarObjectTextInternal::Table);
        tableZone->m_table=table;
        content->m_zoneList.push_back(tableZone);
      }
      break;
    }
    case 'G': {
      std::shared_ptr<StarObjectTextInternal::GraphZone> graph;
      done=readSWGraphNode(zone, graph);
      if (done && graph)
        content->m_zoneList.push_back(graph);
      break;
    }
    case 'I': {
      std::shared_ptr<StarObjectTextInternal::SectionZone> section;
      done=readSWSection(zone, section);
      if (done && section)
        content->m_zoneList.push_back(section);
      break;
    }
    case 'O': {
      std::shared_ptr<StarObjectTextInternal::OLEZone> ole;
      done=readSWOLENode(zone, ole);
      if (done && ole)
        content->m_zoneList.push_back(ole);
      break;
    }
    case 'T': {
      std::shared_ptr<StarObjectTextInternal::TextZone> text;
      done=readSWTextZone(zone, text);
      if (done && text)
        content->m_zoneList.push_back(text);
      break;
    }
    case 'l': // related to link
    case 'o': { // format: safe to ignore
      std::shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=getFormatManager()->readSWFormatDef(zone,static_cast<unsigned char>(cType),format, *this);
      if (done && format) {
        std::shared_ptr<StarObjectTextInternal::FormatZone> formatZone;
        formatZone.reset(new StarObjectTextInternal::FormatZone(format));
        content->m_zoneList.push_back(formatZone);
      }
      break;
    }
    case 'v': {
      StarWriterStruct::NodeRedline redline;
      done=redline.read(zone);
      break;
    }
    default:
      break;
    }
    if (done) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWContent[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (cType) {
    case 'i':
      // sw_sw3node.cxx InRepTxtNode
      f << "repTxtNode,";
      f << "rep=" << input->readULong(4) << ",";
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSWContent: find unexpected type\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWContent");
  }
  zone.closeSWRecord('N', "SWContent");
  return true;
}

bool StarObjectText::readSWGraphNode(StarZone &zone, std::shared_ptr<StarObjectTextInternal::GraphZone> &graphZone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='G' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InGrfNode
  libstoff::DebugStream f;
  f << "Entries(SWGraphNode)[" << zone.getRecordLevel() << "]:";
  graphZone.reset(new StarObjectTextInternal::GraphZone(m_oleParser));
  std::vector<uint32_t> text;
  int fl=zone.openFlagZone();
  if (fl&0x10) f << "link,";
  if (fl&0x20) f << "empty,";
  if (fl&0x40) f << "serverMap,";
  zone.closeFlagZone();
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('G', "SWGraphNode");
      return true;
    }
    if (!text.empty()) {
      graphZone->m_names[i]=libstoff::getString(text);
      f << (i==0 ? "grfName" : "fltName") << "=" << graphZone->m_names[i].cstr() << ",";
    }
  }
  if (zone.isCompatibleWith(0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: can not read a objName\n"));
      f << "###textRepl";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('G', "SWGraphNode");
      return true;
    }
    if (!text.empty()) {
      graphZone->m_names[2]=libstoff::getString(text);
      f << "textRepl=" << graphZone->m_names[2].cstr() << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  long lastPos=zone.getRecordLastPosition();
  while (input->tell() < lastPos) {
    pos=input->tell();
    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'S':
      done=StarWriterStruct::Attribute::readList(zone, graphZone->m_attributeList, *this);
      break;
    case 'X': // store me
      done=readSWImageMap(zone);
      break;
    default:
      break;
    }
    if (done)
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWGraphNode[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case 'k': {
      // sw_sw3nodes.cxx InContour
      int polyFl=zone.openFlagZone();
      zone.closeFlagZone();
      if (polyFl&0x10) {
        // poly2.cxx operator>>
        auto numPoly=int(input->readULong(2));
        for (int i=0; i<numPoly; ++i) {
          f << "poly" << i << "=[";
          // poly.cxx operator>>
          auto numPoints=int(input->readULong(2));
          if (input->tell()+8*numPoints>lastPos) {
            STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: can not read a polygon\n"));
            f << "###poly";
            break;
          }
          for (int p=0; p<numPoints; ++p) {
            int dim[2];
            for (int &j : dim) j=int(input->readLong(4));
            graphZone->m_contour.m_points.push_back(StarGraphicStruct::StarPolygon::Point(STOFFVec2i(dim[0],dim[1])));
            f << STOFFVec2i(dim[0],dim[1]) << ",";
          }
          f << "],";
        }
      }
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSWGraphNode: find unexpected type\n"));
      f << "###";
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWGraphNode");
  }

  zone.closeSWRecord('G', "SWGraphNode");
  return true;
}

bool StarObjectText::readSWImageMap(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='X' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugStream f;
  f << "Entries(SWImageMap)[" << zone.getRecordLevel() << "]:";
  // sw_sw3nodes.cxx InImageMap
  int flag=zone.openFlagZone();
  if (flag&0xF0) f << "fl=" << flag << ",";
  zone.closeFlagZone();
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: can not read url\n"));
    f << "###url";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    zone.closeSWRecord('X', "SWImageMap");
    return true;
  }
  if (!string.empty())
    f << "url=" << libstoff::getString(string).cstr() << ",";
  if (zone.isCompatibleWith(0x11,0x22, 0x101)) {
    for (int i=0; i<2; ++i) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: can not read string\n"));
        f << "###string";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        zone.closeSWRecord('X', "SWImageMap");
        return true;
      }
      if (string.empty()) continue;
      f << (i==0 ? "target" : "dummy") << "=" << libstoff::getString(string).cstr() << ",";
    }
  }
  if (flag&0x20) {
    // svt_imap.cxx: ImageMap::Read
    std::string cMagic("");
    for (int i=0; i<6; ++i) cMagic+=char(input->readULong(1));
    if (cMagic!="SDIMAP") {
      STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: cMagic is bad\n"));
      f << "###cMagic=" << cMagic << ",";
    }
    else {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      for (int i=0; i<3; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: can not read string\n"));
          f << "###string";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());

          zone.closeSWRecord('X', "SWImageMap");
          return true;
        }
        if (!string.empty())
          f << (i==0 ? "target" : i==1 ? "dummy1" : "dummy2") << "=" << libstoff::getString(string).cstr() << ",";
        if (i==1)
          f << "nCount=" << input->readULong(2) << ",";
      }
      if (input->tell()<zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWImageMap: find imapCompat data, not implemented\n"));
        // svt_imap3.cxx IMapCompat::IMapCompat
        ascFile.addPos(input->tell());
        ascFile.addNote("SWImageMap:###IMapCompat");
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      }
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('X', "SWImageMap");
  return true;
}

bool StarObjectText::readSWJobSetUp(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='J' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugStream f;
  zone.openFlagZone();
  zone.closeFlagZone();
  if (input->tell()==zone.getRecordLastPosition()) // empty
    f << "Entries(JobSetUp)[" << zone.getRecordLevel() << "]:";
  else {
    f << "JobSetUp[container-" << zone.getRecordLevel() << "]:";
    StarFileManager fileManager;
    fileManager.readJobSetUp(zone, false);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSWRecord(type, "JobSetUp[container]");
  return true;
}

bool StarObjectText::readSWOLENode(StarZone &zone, std::shared_ptr<StarObjectTextInternal::OLEZone> &ole)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='O' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InOLENode
  libstoff::DebugStream f;
  f << "Entries(SWOLENode)[" << zone.getRecordLevel() << "]:";

  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWOLENode: can not read a objName\n"));
    f << "###objName";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord('O', "SWOLENode");
    return true;
  }
  ole.reset(new StarObjectTextInternal::OLEZone);
  ole->m_oleParser=m_oleParser;
  if (!text.empty()) {
    ole->m_name=libstoff::getString(text);
    f << "objName=" << ole->m_name.cstr() << ",";
  }
  if (zone.isCompatibleWith(0x101)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWOLENode: can not read a objName\n"));
      f << "###textRepl";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('O', "SWOLENode");
      return true;
    }
    if (!text.empty()) {
      ole->m_replaceText=libstoff::getString(text);
      f << "textRepl=" << ole->m_replaceText.cstr() << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('O', "SWOLENode");
  return true;
}

bool StarObjectText::readSWSection(StarZone &zone, std::shared_ptr<StarObjectTextInternal::SectionZone> &section)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='I' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3sectn.cxx: InSection
  libstoff::DebugStream f;
  f << "Entries(SWSection)[" << zone.getRecordLevel() << "]:";
  section.reset(new StarObjectTextInternal::SectionZone);
  std::vector<uint32_t> text;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWSection: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('I', "SWSection");
      return true;
    }
    if (text.empty()) continue;
    if (i==0)
      section->m_name=libstoff::getString(text);
    else
      section->m_condition=libstoff::getString(text);
    f << (i==0 ? "name" : "cond") << "=" << libstoff::getString(text).cstr() << ",";
  }
  int fl=section->m_flags=zone.openFlagZone();
  if (fl&0x10) f << "hidden,";
  if (fl&0x20) f << "protect,";
  if (fl&0x40) f << "condHidden,";
  if (fl&0x80) f << "connectFlag,";
  section->m_type=int(input->readULong(2));
  if (section->m_type) f << "nType=" << section->m_type << ",";
  zone.closeFlagZone();
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    long actPos=input->tell();
    int c=input->peek();
    bool done=false;
    switch (c) { // checkme: unsure which data can be found here
    case 's':
      done=getFormatManager()->readSWFormatDef(zone,'s', section->m_format, *this);
      break;
    case 'N':
      done=readSWContent(zone, section->m_content);
      break;
    default:
      break;
    }
    if (!done || input->tell()<=actPos || input->tell()>lastPos) {
      input->seek(actPos, librevenge::RVNG_SEEK_SET);
      break;
    }
  }
  if (zone.isCompatibleWith(0xd)) {
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarObjectText::readSWSection: can not read a linkName\n"));
      f << "###linkName";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSWRecord('I', "SWSection");
      return true;
    }
    else if (!text.empty()) {
      section->m_linkName=libstoff::getString(text);
      f << "linkName=" << section->m_linkName.cstr() << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord('I', "SWSection");
  return true;
}

bool StarObjectText::readSWTextZone(StarZone &zone, std::shared_ptr<StarObjectTextInternal::TextZone> &textZone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  if (input->peek()!='T' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3nodes.cxx: InTxtNode
  libstoff::DebugStream f;
  f << "Entries(SWText)[" << zone.getRecordLevel() << "]:";
  textZone.reset(new StarObjectTextInternal::TextZone);
  int fl=zone.openFlagZone();
  auto poolId=int(input->readULong(2));
  if (!zone.getPoolName(poolId, textZone->m_styleName))
    f << "###nPoolId=" << poolId << ",";
  else
    f << textZone->m_styleName.cstr() << ",";
  if (fl&0x10 && !zone.isCompatibleWith(0x201)) {
    auto val=int(input->readULong(1));
    if (val==200 && zone.isCompatibleWith(0xf,0x101) && input->tell() < zone.getFlagLastPosition())
      val=int(input->readULong(1));
    if (val!=200) {
      textZone->m_level=val&0x1f;
      f << "nLevel=" << val << ",";
    }
  }
  if (zone.isCompatibleWith(0x19,0x22, 0x101))
    f << "nCondColl=" << input->readULong(2) << ",";
  zone.closeFlagZone();

  if (!zone.readString(textZone->m_text, textZone->m_textSourcePosition, -1, true)) {
    STOFF_DEBUG_MSG(("StarObjectText::readSWTextZone: can not read main text\n"));
    f << "###text";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord('T', "SWText");
    return true;
  }
  else if (!textZone->m_text.empty())
    f << libstoff::getString(textZone->m_text).cstr();

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();

    bool done=false;
    int rType=input->peek();

    switch (rType) {
    case 'A': {
      StarWriterStruct::Attribute attrib;
      done=attrib.read(zone, *this);
      if (done)
        textZone->m_charAttributeList.push_back(attrib);
      break;
    }
    case 'K': {
      StarWriterStruct::Mark mark;
      done=mark.read(zone);
      if (done) textZone->m_markList.push_back(mark);
      break;
    }
    case '0':
    case 'R': {
      std::shared_ptr<STOFFList> list;
      done=StarObjectNumericRuler::readList(zone,list);
      if (done && list) textZone->m_list=list;
      break;
    }
    case 'S':
      done=StarWriterStruct::Attribute::readList(zone, textZone->m_charAttributeList, *this);
      break;
    case 'l': { // related to link
      std::shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=getFormatManager()->readSWFormatDef(zone,'l', format, *this);
      if (done && format)
        textZone->m_formatList.push_back(format);
      break;
    }
    case 'o': { //
      std::shared_ptr<StarFormatManagerInternal::FormatDef> format;
      done=getFormatManager()->readSWFormatDef(zone,'o', format, *this);
      if (done && format)
        textZone->m_formatList.push_back(format);
      break;
    }
    case 'v': {
      StarWriterStruct::NodeRedline redline;
      done=redline.read(zone);
      break;
    }
    default:
      break;
    }
    if (done)
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWText[" << type << "-" << zone.getRecordLevel() << "]:";
    switch (type) {
    case '3': {
      // sw_sw3num InNodeNum
      f << "nodeNum,";
      int cFlag=zone.openFlagZone();
      auto nLevel=int(input->readULong(1));
      if (nLevel!=201) {
        int level=(nLevel&0x1f);
        if (level>=10) {
          level=9;
          if (nLevel&0x20) level |= 0x20;
        }
        textZone->m_level=level;
        f << "nLevel=" << level<< ",";
      }
      if (cFlag&0x20) f << "nSetValue=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      if (nLevel!=201) {
        int N=int(zone.getRecordLastPosition()-input->tell())/2;
        f << "level=[";
        for (int i=0; i<N; ++i)
          f << input->readULong(2) << ",";
        f << "],";
      }
      break;
    }
    case 'w': { // wrong list
      // sw_sw3nodes.cxx in text node
      f << "wrongList,";
      int cFlag=zone.openFlagZone();
      if (cFlag&0xf0) f << "flag=" << (cFlag>>4) << ",";
      f << "nBeginInv=" << input->readULong(2) << ",";
      f << "nEndInc=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      auto N =int(input->readULong(2));
      if (input->tell()+4*N>zone.getRecordLastPosition()) {
        STOFF_DEBUG_MSG(("StarObjectText::readSWTextZone: find bad count\n"));
        f << "###N=" << N << ",";
        break;
      }
      f << "nWrong=[";
      for (int i=0; i<N; ++i) f << input->readULong(4) << ",";
      f << "],";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readSWTextZone: find unexpected type\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWText");
  }
  zone.closeSWRecord('T', "SWText");
  return true;
}

////////////////////////////////////////////////////////////
// drawing layer
////////////////////////////////////////////////////////////
bool StarObjectText::readDrawingLayer(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "DrawingLayer", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);
  // sw_sw3imp.cxx Sw3IoImp::LoadDrawingLayer

  // create this pool from the main's SWG pool
  auto pool=getNewItemPool(StarItemPool::T_XOutdevPool);
  pool->setRelativeUnit(0.05);
  pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));

  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: create extra pool for %d of type %d\n",
                         int(getDocumentKind()), int(pool->getType())));
      }
      pool.reset();
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  long pos=input->tell();
  std::shared_ptr<StarObjectModel> model(new StarObjectModel(*this, true));
  if (!model->read(zone)) {
    STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: can not read the drawing model\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(DrawingLayer):###extra");
    return true;
  }
  if (m_textState->m_model) {
    STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: oops the drawing model is already defined\n"));
  }
  else
    m_textState->m_model=model;
  if (input->isEnd()) return true;
  pos=input->tell();
  uint16_t nSign;
  *input >> nSign;
  libstoff::DebugStream f;
  f << "Entries(DrawingLayer):";
  bool ok=true;
  std::set<long> ids;
  if (nSign!=0x444D && nSign!=0) // 0 seems ok if followed by 0
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  else {
    uint16_t n;
    *input >> n;
    if (pos+4+4*long(n)>input->size()) {
      STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: bad n frame\n"));
      f << "###pos";
      ok=false;
    }
    else {
      f << "framePos=[";
      for (uint16_t i=0; i<n; ++i) {
        auto id=long(input->readULong(4));
        ids.insert(id);
        f << id << ",";
      }
      f << "],";
    }
  }
  if (ok && input->tell()+4==input->size())
    f << "num[hidden]=" << input->readULong(4) << ",";
  if (ok && !input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readDrawingLayer: find extra data\n"));
    f << "###extra";
  }
  model->updateObjectIds(ids);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}
catch (...)
{
  return false;
}

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool StarObjectText::readWriterDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWWriterDocument", getPassword());
  if (!zone.readSWHeader()) {
    STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read the header\n"));
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  // sw_sw3doc.cxx Sw3IoImp::LoadDocContents
  SWFieldManager fieldManager;
  StarFileManager fileManager;
  while (!input->isEnd()) {
    long pos=input->tell();
    int rType=input->peek();
    bool done=false;
    switch (rType) {
    case '!':
      done=zone.readStringsPool();
      break;
    case 'R':
    case '0': { // Outline
      std::shared_ptr<STOFFList> list;
      done=StarObjectNumericRuler::readList(zone,list);
      break;
    }
    case '1': {
      StarWriterStruct::NoteInfo info(true);
      done=info.read(zone);
      break;
    }
    case '4': {
      StarWriterStruct::NoteInfo info(false);
      done=info.read(zone);
      break;
    }
    case '8': {
      StarWriterStruct::PrintData info;
      done=info.read(zone);
      break;
    }
    case 'D': {
      StarWriterStruct::DatabaseName dbase;
      done=dbase.read(zone);
      break;
    }
    case 'F':
      done=getFormatManager()->readSWFlyFrameList(zone, *this, m_textState->m_flyList);
      break;
    case 'J':
      done=readSWJobSetUp(zone);
      break;
    case 'M': {
      std::vector<StarWriterStruct::Macro> macroList;
      done=StarWriterStruct::Macro::readList(zone, macroList);
      break;
    }
    case 'N':
      done=readSWContent(zone, m_textState->m_mainContent);
      break;
    case 'U': { // layout info, no code, ignored by LibreOffice
      StarLayout layout;
      done=layout.read(zone, *this);
      break;
    }
    case 'V': {
      std::vector<std::vector<StarWriterStruct::Redline> > redlineListList;
      done=StarWriterStruct::Redline::readListList(zone, redlineListList);
      break;
    }
    case 'Y':
      done=bool(fieldManager.readField(zone,'Y'));
      break;
    case 'a': {
      std::vector<StarWriterStruct::Bookmark> markList;
      done=StarWriterStruct::Bookmark::readList(zone, markList);
      break;
    }
    case 'd': {
      StarWriterStruct::DocStats stats;
      done=stats.read(zone);
      break;
    }
    case 'j': {
      StarWriterStruct::Dictionary dico;
      done=dico.read(zone);
      break;
    }
    case 'q':
      done=getFormatManager()->readSWNumberFormatterList(zone);
      break;
    case 'u': {
      std::vector<StarWriterStruct::TOX> toxList;
      done=StarWriterStruct::TOX::readList(zone, toxList, *this);
      break;
    }
    case 'y': {
      std::vector<StarWriterStruct::TOX51> toxList;
      done=StarWriterStruct::TOX51::readList(zone, toxList, *this);
      break;
    }
    default:
      break;
    }
    if (done)
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    unsigned char type;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "SWWriterDocument[" << type << "]:";
    long lastPos=zone.getRecordLastPosition();
    bool endZone=false;
    std::vector<uint32_t> string;
    switch (type) {
    case '$': // unknown, seems to store an object name
      f << "dollarZone,";
      if (input->tell()+7>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: zone seems to short\n"));
        break;
      }
      for (int i=0; i<5; ++i) { // f0=f1=1
        auto val=int(input->readULong(1));
        if (val) f << "f" << i << "=" << val << ",";
      }
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read main string\n"));
        f << "###string";
        break;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr();
      break;
    case '5': {
      // sw_sw3misc.cxx InLineNumberInfo
      int fl=zone.openFlagZone();
      if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
      f << "linenumberInfo=" << input->readULong(1) << ",";
      f << "nPos=" << input->readULong(1) << ",";
      f << "nChrIdx=" << input->readULong(2) << ",";
      f << "nPosFromLeft=" << input->readULong(2) << ",";
      f << "nCountBy=" << input->readULong(2) << ",";
      f << "nDividerCountBy=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read sDivider string\n"));
        f << "###sDivider";
        break;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr();
      break;
    }
    case '6':
      // sw_sw3misc.cxx InDocDummies
      f << "docDummies,";
      f << "n1=" << input->readULong(4) << ",";
      f << "n2=" << input->readULong(4) << ",";
      f << "n3=" << input->readULong(1) << ",";
      f << "n4=" << input->readULong(1) << ",";
      for (int i=0; i<2; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read a string\n"));
          f << "###string";
          break;
        }
        else if (!string.empty())
          f << (i==0 ? "sAutoMarkURL" : "s2") << "=" << libstoff::getString(string).cstr() << ",";
      }
      break;
    case '7': { // config, ignored by LibreOffice, and find no code
      f << "config,";
      auto fl=int(zone.openFlagZone());
      if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
      f << "f0=" << input->readULong(1) << ","; // 1
      for (int i=0; i<5; ++i) // e,1,5,1,5
        f << "f" << i+1 << "=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      break;
    }
    case 'C': { // ignored by LibreOffice
      std::string comment("");
      while (lastPos && input->tell()<lastPos) comment+=char(input->readULong(1));
      f << "comment=" << comment << ",";
      break;
    }
    case 'P': // password
      // sw_sw3misc.cxx: InPasswd
      f << "passwd,";
      if (zone.isCompatibleWith(0x6)) {
        f << "cType=" << input->readULong(1) << ",";
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: can not read passwd string\n"));
          f << "###passwd";
        }
        else
          f << "cryptedPasswd=" << libstoff::getString(string).cstr() << ",";
      }
      break;
    case 'Z':
      endZone=true;
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: find unexpected type\n"));
      f << "###type,";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "SWWriterDocument");
    if (endZone)
      break;
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectText::readWriterDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SWWriterDocument:##extra");
  }
  return true;
}
catch (...)
{
  return false;
}
////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
