/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstoff
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
#include <sstream>
#include <time.h>

#include <librevenge/librevenge.h>

#include "libstaroffice/libstaroffice.hxx"
#include "libstaroffice_internal.hxx"

#include "STOFFCell.hxx"
#include "STOFFFont.hxx"
//#include "STOFFGraphicEncoder.hxx"
//#include "STOFFGraphicStyle.hxx"
//#include "STOFFGraphicShape.hxx"
#include "STOFFInputStream.hxx"
#include "STOFFList.hxx"
#include "STOFFParagraph.hxx"
#include "STOFFParser.hxx"
#include "STOFFPosition.hxx"
#include "STOFFSection.hxx"
#include "STOFFSubDocument.hxx"
#include "STOFFTable.hxx"

#include "STOFFGraphicListener.hxx"

//! Internal and low level namespace to define the states of STOFFGraphicListener
namespace STOFFGraphicListenerInternal
{
/** the global graphic state of STOFFGraphicListener */
struct GraphicState {
  //! constructor
  explicit GraphicState(std::vector<STOFFPageSpan> const &pageList) :
    m_pageList(pageList), m_metaData(),
    m_isDocumentStarted(false), m_isPageSpanOpened(false), m_isMasterPageSpanOpened(false), m_isAtLeastOnePageOpened(false),
    m_isHeaderFooterOpened(false), m_isHeaderFooterRegionOpened(false), m_pageSpan(), m_sentListMarkers(), m_subDocuments()
  {
  }
  //! destructor
  ~GraphicState()
  {
  }
  //! the pages definition
  std::vector<STOFFPageSpan> m_pageList;
  //! the document meta data
  librevenge::RVNGPropertyList m_metaData;
  /** a flag to know if the document is open */
  bool m_isDocumentStarted;
  //! true if a page is open
  bool m_isPageSpanOpened;
  //! true if a masterpage is open
  bool m_isMasterPageSpanOpened;
  /** true if the first page has been open */
  bool m_isAtLeastOnePageOpened;
  /** a flag to know if the header footer is started */
  bool m_isHeaderFooterOpened;
  bool m_isHeaderFooterRegionOpened /** a flag to know if the header footer region is started */;
  ///! the current page span
  STOFFPageSpan m_pageSpan;
  /// the list of marker corresponding to sent list
  std::vector<int> m_sentListMarkers;
  //! the list of actual subdocument
  std::vector<STOFFSubDocumentPtr> m_subDocuments;
};

/** the state of a STOFFGraphicListener */
struct State {
  //! constructor
  State();
  //! destructor
  ~State() { }

//! returns true if we are in a text zone, ie. either in a textbox or a table cell
  bool isInTextZone() const
  {
    return m_inNote || m_isTextBoxOpened || m_isTableCellOpened;
  }
  //! the origin position
  STOFFVec2f m_origin;
  //! a buffer to stored the text
  librevenge::RVNGString m_textBuffer;

  //! the font
  STOFFFont m_font;
  //! the paragraph
  STOFFParagraph m_paragraph;
  //! the list of list
  shared_ptr<STOFFList> m_list;

  //! a flag to know if openFrame was called
  bool m_isFrameOpened;
  //! the frame position
  STOFFPosition m_framePosition;
  //! the frame style
  STOFFGraphicStyle m_frameStyle;

  //! a flag to know if we are in a textbox
  bool m_isTextBoxOpened;
  //! a flag to know if openGroup was called
  bool m_isGroupOpened;
  //! a flag to know if openLayer was called
  bool m_isLayerOpened;
  bool m_isSpanOpened;
  bool m_isParagraphOpened;
  bool m_isListElementOpened;

  std::vector<bool> m_listOrderedLevels; //! a stack used to know what is open

  bool m_isTableOpened;
  bool m_isTableRowOpened;
  bool m_isTableColumnOpened;
  bool m_isTableCellOpened;

  unsigned m_currentPage;
  int m_numPagesRemainingInSpan;
  int m_currentPageNumber;

  bool m_inLink;
  bool m_inNote;
  bool m_inSubDocument;
  libstoff::SubDocumentType m_subDocumentType;

private:
  State(const State &);
  State &operator=(const State &);
};

State::State() : m_origin(0,0),
  m_textBuffer(""), m_font()/* default time 12 */, m_paragraph(), m_list(),
  m_isFrameOpened(false), m_framePosition(), m_frameStyle(), m_isTextBoxOpened(false),
  m_isGroupOpened(false), m_isLayerOpened(false),
  m_isSpanOpened(false), m_isParagraphOpened(false), m_isListElementOpened(false),
  m_listOrderedLevels(),
  m_isTableOpened(false), m_isTableRowOpened(false), m_isTableColumnOpened(false),
  m_isTableCellOpened(false),
  m_currentPage(0), m_numPagesRemainingInSpan(0), m_currentPageNumber(1),
  m_inLink(false), m_inNote(false), m_inSubDocument(false), m_subDocumentType(libstoff::DOC_NONE)
{
}
}

STOFFGraphicListener::STOFFGraphicListener(STOFFParserState &parserState, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGDrawingInterface *documentInterface) : STOFFListener(),
  m_ds(new STOFFGraphicListenerInternal::GraphicState(pageList)), m_ps(new STOFFGraphicListenerInternal::State),
  m_psStack(), m_parserState(parserState), m_documentInterface(documentInterface)
{
}

STOFFGraphicListener::~STOFFGraphicListener()
{
}

///////////////////
// text data
///////////////////
void STOFFGraphicListener::insertChar(uint8_t character)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertCharacter: called outside a text zone\n"));
    return;
  }
  if (character >= 0x80) {
    STOFFGraphicListener::insertUnicode(character);
    return;
  }
  if (!m_ps->m_isSpanOpened) _openSpan();
  m_ps->m_textBuffer.append((char) character);
}

void STOFFGraphicListener::insertUnicode(uint32_t val)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertUnicode: called outside a text zone\n"));
    return;
  }
  // undef character, we skip it
  if (val == 0xfffd) return;

  if (!m_ps->m_isSpanOpened) _openSpan();
  libstoff::appendUnicode(val, m_ps->m_textBuffer);
}

void STOFFGraphicListener::insertUnicodeString(librevenge::RVNGString const &str)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertUnicodeString: called outside a text zone\n"));
    return;
  }
  if (!m_ps->m_isSpanOpened) _openSpan();
  m_ps->m_textBuffer.append(str);
}

void STOFFGraphicListener::insertEOL(bool soft)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertEOL: called outside a text zone\n"));
    return;
  }
  if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
    _openSpan();
  if (soft) {
    _flushText();
    m_documentInterface->insertLineBreak();
  }
  else if (m_ps->m_isParagraphOpened)
    _closeParagraph();
}

void STOFFGraphicListener::insertTab()
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertTab: called outside a text zone\n"));
    return;
  }
  if (!m_ps->m_isSpanOpened) _openSpan();
  _flushText();
  m_documentInterface->insertTab();
}

///////////////////
// font/paragraph function
///////////////////
void STOFFGraphicListener::setFont(STOFFFont const &font)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::setFont: called outside a text zone\n"));
    return;
  }
  if (font == m_ps->m_font) return;
  _closeSpan();
  m_ps->m_font = font;
}

STOFFFont const &STOFFGraphicListener::getFont() const
{
  return m_ps->m_font;
}

bool STOFFGraphicListener::isParagraphOpened() const
{
  return m_ps->m_isParagraphOpened;
}

void STOFFGraphicListener::setParagraph(STOFFParagraph const &para)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::setParagraph: called outside a text zone\n"));
    return;
  }
  if (para==m_ps->m_paragraph) return;

  m_ps->m_paragraph=para;
}

STOFFParagraph const &STOFFGraphicListener::getParagraph() const
{
  return m_ps->m_paragraph;
}

///////////////////
// field/link :
///////////////////
void STOFFGraphicListener::insertField(STOFFField const &field)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::setParagraph: called outside a text zone\n"));
    return;
  }
  librevenge::RVNGPropertyList propList;
  if (field.addTo(propList)) {
    _flushText();
    _openSpan();
    m_documentInterface->insertField(propList);
    return;
  }
  librevenge::RVNGString text=field.getString();
  if (!text.empty())
    STOFFGraphicListener::insertUnicodeString(text);
  else {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertField: must not be called with type=%d\n", int(field.m_type)));
  }
}

void STOFFGraphicListener::openLink(STOFFLink const &link)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openLink: called outside a textbox\n"));
    return;
  }
  if (m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openLink: called inside a link\n"));
    return;
  }
  if (!m_ps->m_isSpanOpened) _openSpan();
  librevenge::RVNGPropertyList propList;
  link.addTo(propList);
  m_documentInterface->openLink(propList);
  _pushParsingState();
  m_ps->m_inLink=true;
// we do not want any close open paragraph in a link
  m_ps->m_isParagraphOpened=true;
}

void STOFFGraphicListener::closeLink()
{
  if (!m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeLink: closed outside a link\n"));
    return;
  }
  m_documentInterface->closeLink();
  _popParsingState();
}

///////////////////
// document
///////////////////
void STOFFGraphicListener::startDocument()
{
  if (m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::startDocument: the document is already started\n"));
    return;
  }
  m_ds->m_isDocumentStarted=true;
  m_documentInterface->startDocument(librevenge::RVNGPropertyList());
  m_documentInterface->setDocumentMetaData(m_ds->m_metaData);
}

void STOFFGraphicListener::endDocument(bool /*delayed*/)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::endDocument: the document is not started\n"));
    return;
  }
  if (!m_ds->m_isAtLeastOnePageOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::endDocument: no data have been send\n"));
    _openPageSpan();
  }
  if (m_ds->m_isPageSpanOpened)
    _closePageSpan(m_ds->m_isMasterPageSpanOpened);
  m_documentInterface->endDocument();
  m_ds->m_isDocumentStarted=false;
  *m_ds=STOFFGraphicListenerInternal::GraphicState(std::vector<STOFFPageSpan>());
}

///////////////////
// document
///////////////////
void STOFFGraphicListener::setDocumentLanguage(std::string locale)
{
  if (!locale.length()) return;
  m_ds->m_metaData.insert("librevenge:language", locale.c_str());
}

void STOFFGraphicListener::setDocumentMetaData(const librevenge::RVNGPropertyList &list)
{
  librevenge::RVNGPropertyList::Iter i(list);
  for (i.rewind(); i.next();)
    m_ds->m_metaData.insert(i.key(), i()->getStr());
}

bool STOFFGraphicListener::isDocumentStarted() const
{
  return m_ds->m_isDocumentStarted;
}

bool STOFFGraphicListener::canWriteText() const
{
  return m_ds->m_isPageSpanOpened && m_ps->isInTextZone();
}

///////////////////
// page
///////////////////
bool STOFFGraphicListener::isPageSpanOpened() const
{
  return m_ds->m_isPageSpanOpened;
}

STOFFPageSpan const &STOFFGraphicListener::getPageSpan()
{
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  return m_ds->m_pageSpan;
}

bool STOFFGraphicListener::openMasterPage(STOFFPageSpan &masterPage)
{
  if (m_ds->m_isMasterPageSpanOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openMasterPage: a master page is already opened\n"));
    return false;
  }
  if (!m_ds->m_isDocumentStarted)
    startDocument();
  if (m_ds->m_isPageSpanOpened)
    _closePageSpan();

  librevenge::RVNGPropertyList propList;
  masterPage.getPageProperty(propList);

  m_documentInterface->startMasterPage(propList);
  m_ds->m_isPageSpanOpened = m_ds->m_isMasterPageSpanOpened = true;

  // checkme: can we send some header/footer if some exists
  return true;
}

void STOFFGraphicListener::_openPageSpan(bool sendHeaderFooters)
{
  if (m_ds->m_isPageSpanOpened)
    return;

  if (!m_ds->m_isDocumentStarted)
    startDocument();

  if (m_ds->m_pageList.size()==0) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_openPageSpan: can not find any page\n"));
    throw libstoff::ParseException();
  }
  m_ds->m_isAtLeastOnePageOpened=true;
  unsigned actPage = 0;
  std::vector<STOFFPageSpan>::iterator it = m_ds->m_pageList.begin();
  m_ps->m_currentPage++;
  while (true) {
    actPage+=(unsigned)it->m_pageSpan;
    if (actPage >= m_ps->m_currentPage) break;
    if (++it == m_ds->m_pageList.end()) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::_openPageSpan: can not find current page, use the previous one\n"));
      --it;
      break;
    }
  }

  STOFFPageSpan &currentPage = *it;
  librevenge::RVNGPropertyList propList;
  currentPage.getPageProperty(propList);
  propList.insert("librevenge:is-last-page-span", ++it == m_ds->m_pageList.end());
  // now add data for embedded graph
  propList.insert("svg:x",m_ps->m_origin.x(), librevenge::RVNG_POINT);
  propList.insert("svg:y",m_ps->m_origin.y(), librevenge::RVNG_POINT);
  propList.insert("librevenge:enforce-frame",true);

  if (!m_ds->m_isPageSpanOpened)
    m_documentInterface->startPage(propList);
  m_ds->m_isPageSpanOpened = true;
  m_ds->m_pageSpan = currentPage;

  // we insert the header footer
  if (sendHeaderFooters)
    currentPage.sendHeaderFooters(this);

  m_ps->m_numPagesRemainingInSpan = (currentPage.m_pageSpan - 1);
}

void STOFFGraphicListener::_closePageSpan(bool masterPage)
{
  if (!m_ds->m_isPageSpanOpened)
    return;

  if (masterPage && !m_ds->m_isMasterPageSpanOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::endDocument:no master page are opened\n"));
    return;
  }
  if (!masterPage && m_ds->m_isMasterPageSpanOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::endDocument:a master page are opened\n"));
    return;
  }
  if (m_ps->m_inSubDocument) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::endDocument: we are in a sub document\n"));
    _endSubDocument();
    _popParsingState();
  }
  if (m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_closePageSpan: we are in a table zone\n"));
    closeTable();
  }
  if (m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_closePageSpan: we are in a text zone\n"));
    if (m_ps->m_isParagraphOpened)
      _closeParagraph();
    m_ps->m_paragraph.m_listLevelIndex = 0;
    _changeList(); // flush the list exterior
  }
  m_ds->m_isPageSpanOpened = m_ds->m_isMasterPageSpanOpened = false;
  if (masterPage)
    m_documentInterface->endMasterPage();
  else
    m_documentInterface->endPage();
}

///////////////////
// paragraph
///////////////////
void STOFFGraphicListener::_openParagraph()
{
  if (m_ps->m_inNote || (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened))
    return;
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_openParagraph: called outsize a text zone\n"));
    return;
  }
  if (m_ps->m_isParagraphOpened || m_ps->m_isListElementOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_openParagraph: a paragraph (or a list) is already opened"));
    return;
  }

  librevenge::RVNGPropertyList propList;
  m_ps->m_paragraph.addTo(propList);
  m_documentInterface->openParagraph(propList);

  _resetParagraphState();
}

void STOFFGraphicListener::_closeParagraph()
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_closeParagraph: called outsize a text zone\n"));
    return;
  }
  if (m_ps->m_inLink) return;
  if (m_ps->m_isListElementOpened) {
    _closeListElement();
    return;
  }

  if (m_ps->m_isParagraphOpened) {
    if (m_ps->m_isSpanOpened)
      _closeSpan();
    m_documentInterface->closeParagraph();
  }

  m_ps->m_isParagraphOpened = false;
  m_ps->m_paragraph.m_listLevelIndex = 0;
}

void STOFFGraphicListener::_resetParagraphState(const bool isListElement)
{
  m_ps->m_isListElementOpened = isListElement;
  m_ps->m_isParagraphOpened = true;
}

///////////////////
// list
///////////////////
void STOFFGraphicListener::_openListElement()
{
  if (m_ps->m_inNote || (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened))
    return;
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_openListElement: called outsize a text zone\n"));
    return;
  }
  if (m_ps->m_isParagraphOpened || m_ps->m_isListElementOpened)
    return;

  librevenge::RVNGPropertyList propList;
  m_ps->m_paragraph.addTo(propList);

  // check if we must change the start value
  int startValue=m_ps->m_paragraph.m_listStartValue;
  if (startValue > 0 && m_ps->m_list && m_ps->m_list->getStartValueForNextElement() != startValue) {
    propList.insert("text:start-value", startValue);
    m_ps->m_list->setStartValueForNextElement(startValue);
  }

  if (m_ps->m_list) m_ps->m_list->openElement();
  m_documentInterface->openListElement(propList);
  _resetParagraphState(true);
}

void STOFFGraphicListener::_closeListElement()
{
  if (m_ps->m_isListElementOpened) {
    if (m_ps->m_isSpanOpened)
      _closeSpan();

    if (m_ps->m_list) m_ps->m_list->closeElement();
    m_documentInterface->closeListElement();
  }

  m_ps->m_isListElementOpened = m_ps->m_isParagraphOpened = false;
}

int STOFFGraphicListener::_getListId() const
{
  size_t newLevel= (size_t) m_ps->m_paragraph.m_listLevelIndex;
  if (newLevel == 0) return -1;
  int newListId = m_ps->m_paragraph.m_listId;
  if (newListId > 0) return newListId;
  static bool first = true;
  if (first) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_getListId: the list id is not set, try to find a new one\n"));
    first = false;
  }
  shared_ptr<STOFFList> list=m_parserState.m_listManager->getNewList
                             (m_ps->m_list, int(newLevel), m_ps->m_paragraph.m_listLevel);
  if (!list) return -1;
  return list->getId();
}

void STOFFGraphicListener::_changeList()
{
  if (m_ps->m_inNote || !m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_changeList: called outsize a text zone\n"));
    return;
  }
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  size_t actualLevel = m_ps->m_listOrderedLevels.size();
  size_t newLevel= (size_t) m_ps->m_paragraph.m_listLevelIndex;
  int newListId = newLevel>0 ? _getListId() : -1;
  bool changeList = newLevel &&
                    (m_ps->m_list && m_ps->m_list->getId()!=newListId);
  size_t minLevel = changeList ? 0 : newLevel;
  while (actualLevel > minLevel) {
    if (m_ps->m_listOrderedLevels[--actualLevel])
      m_documentInterface->closeOrderedListLevel();
    else
      m_documentInterface->closeUnorderedListLevel();
  }

  if (newLevel) {
    shared_ptr<STOFFList> theList;

    theList=m_parserState.m_listManager->getList(newListId);
    if (!theList) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::_changeList: can not find any list\n"));
      m_ps->m_listOrderedLevels.resize(actualLevel);
      return;
    }
    m_parserState.m_listManager->needToSend(newListId, m_ds->m_sentListMarkers);
    m_ps->m_list = theList;
    m_ps->m_list->setLevel((int)newLevel);
  }

  m_ps->m_listOrderedLevels.resize(newLevel, false);
  if (actualLevel == newLevel) return;

  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:list-id", m_ps->m_list->getId());
  for (size_t i=actualLevel+1; i<= newLevel; i++) {
    bool ordered = m_ps->m_list->isNumeric(int(i));
    m_ps->m_listOrderedLevels[i-1] = ordered;

    librevenge::RVNGPropertyList level;
    m_ps->m_list->addTo(int(i), level);
    if (ordered)
      m_documentInterface->openOrderedListLevel(level);
    else
      m_documentInterface->openUnorderedListLevel(level);
  }
}

///////////////////
// span
///////////////////
void STOFFGraphicListener::_openSpan()
{
  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
    return;
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_openSpan: called outsize a text zone\n"));
    return;
  }
  if (m_ps->m_isSpanOpened)
    return;

  if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened) {
    _changeList();
    if (m_ps->m_paragraph.m_listLevelIndex == 0)
      _openParagraph();
    else
      _openListElement();
  }

  librevenge::RVNGPropertyList propList;
  m_documentInterface->openSpan(propList);

  m_ps->m_isSpanOpened = true;
}

void STOFFGraphicListener::_closeSpan()
{
  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
    return;
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_closeSpan: called outsize a text zone\n"));
    return;
  }
  if (!m_ps->m_isSpanOpened)
    return;

  _flushText();
  m_documentInterface->closeSpan();
  m_ps->m_isSpanOpened = false;
}

///////////////////
// text (send data)
///////////////////
void STOFFGraphicListener::_flushText()
{
  if (m_ps->m_textBuffer.len() == 0) return;

  // when some many ' ' follows each other, call insertSpace
  librevenge::RVNGString tmpText("");
  int numConsecutiveSpaces = 0;
  librevenge::RVNGString::Iter i(m_ps->m_textBuffer);
  for (i.rewind(); i.next();) {
    if (*(i()) == 0x20) // this test is compatible with unicode format
      numConsecutiveSpaces++;
    else
      numConsecutiveSpaces = 0;

    if (numConsecutiveSpaces > 1) {
      if (tmpText.len() > 0) {
        m_documentInterface->insertText(tmpText);
        tmpText.clear();
      }
      m_documentInterface->insertSpace();
    }
    else
      tmpText.append(i());
  }
  m_documentInterface->insertText(tmpText);
  m_ps->m_textBuffer.clear();
}

///////////////////
// header/footer
///////////////////
bool STOFFGraphicListener::isHeaderFooterOpened() const
{
  return m_ds->m_isHeaderFooterOpened;
}

bool STOFFGraphicListener::openHeader(librevenge::RVNGPropertyList const &extras)
{
  if (m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openHeader: Oops a header/footer is already opened\n"));
    return false;
  }

  // we do not have any header interface, so mimick it by creating a textbox
  STOFFPosition pos(STOFFVec2f(20,20), STOFFVec2f(-20,-10), librevenge::RVNG_POINT); // fixme
  pos.m_anchorTo=STOFFPosition::Page;
  if (!openFrame(pos))
    return false;
  m_ds->m_isHeaderFooterOpened=true;
  librevenge::RVNGPropertyList propList(extras);
  _handleFrameParameters(propList, pos, STOFFGraphicStyle());

  m_documentInterface->startTextObject(propList);
  return true;
}

bool STOFFGraphicListener::insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &/*which*/)
{
  if (!m_ds->m_isHeaderFooterOpened || m_ds->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  handleSubDocument(STOFFVec2f(20,20), subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  return true;
}

bool STOFFGraphicListener::closeHeader()
{
  if (!m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeHeader: Oops no opened header/footer\n"));
    return false;
  }
  m_documentInterface->endTextObject();
  closeFrame();
  m_ds->m_isHeaderFooterOpened=false;
  return true;
}

bool STOFFGraphicListener::openFooter(librevenge::RVNGPropertyList const &extras)
{
  if (m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openFooter: Oops a header/footer is already opened\n"));
    return false;
  }

  // we do not have any footer interface, so mimick it by creating a textbox
  STOFFPosition pos(STOFFVec2f(700,20), STOFFVec2f(-20,-10), librevenge::RVNG_POINT); // fixme: ypos
  pos.m_anchorTo=STOFFPosition::Page;
  if (!openFrame(pos))
    return false;
  m_ds->m_isHeaderFooterOpened=true;
  librevenge::RVNGPropertyList propList(extras);
  _handleFrameParameters(propList, pos, STOFFGraphicStyle());

  m_documentInterface->startTextObject(propList);
  return true;
}

bool STOFFGraphicListener::insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &/*which*/)
{
  if (!m_ds->m_isHeaderFooterOpened || m_ds->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  handleSubDocument(STOFFVec2f(700,20), subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  return true;
}

bool STOFFGraphicListener::closeFooter()
{
  if (!m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeFooter: Oops no opened header/footer\n"));
    return false;
  }
  m_documentInterface->endTextObject();
  closeFrame();
  m_ds->m_isHeaderFooterOpened=false;
  return true;
}

///////////////////
// section
///////////////////
STOFFSection const &STOFFGraphicListener::getSection() const
{
  STOFF_DEBUG_MSG(("STOFFGraphicListener::getSection: must not be called\n"));
  static STOFFSection s_section;
  return s_section;
}

bool STOFFGraphicListener::openSection(STOFFSection const &)
{
  STOFF_DEBUG_MSG(("STOFFGraphicListener::openSection: must not be called\n"));
  return false;
}

void STOFFGraphicListener::insertBreak(BreakType breakType)
{
  if (m_ps->m_inSubDocument)
    return;

  switch (breakType) {
  case ColumnBreak:
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertBreak: must not be called on column\n"));
    break;
  case SoftPageBreak:
  case PageBreak:
    if (m_ds->m_isMasterPageSpanOpened) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::insertBreak: can not insert a page break in master page definition\n"));
      break;
    }
    if (!m_ds->m_isPageSpanOpened)
      _openPageSpan();
    _closePageSpan();
    break;
  default:
    break;
  }
}

///////////////////
// note/comment ( we inserted them as text between -- -- )
///////////////////
void STOFFGraphicListener::insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &/*creator*/, librevenge::RVNGString const &/*date*/)
{
  if (!canWriteText() || m_ps->m_inNote) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertComment try to insert recursively or outside a text zone\n"));
    return;
  }
  // first check that a paragraph is already open
  if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
    _openParagraph();
  insertChar(' ');
  insertUnicode(0x2014); // -
  insertChar(' ');
  handleSubDocument(subDocument, libstoff::DOC_COMMENT_ANNOTATION);
  insertChar(' ');
  insertUnicode(0x2014); // -
  insertChar(' ');
}

void STOFFGraphicListener::insertNote(STOFFNote const &, STOFFSubDocumentPtr &subDocument)
{
  if (!canWriteText() || m_ps->m_inNote) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertNote try to insert recursively or outside a text zone\n"));
    return;
  }
  // first check that a paragraph is already open
  if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
    _openParagraph();
  insertChar(' ');
  insertUnicode(0x2014); // -
  insertChar(' ');
  handleSubDocument(subDocument, libstoff::DOC_NOTE);
  insertChar(' ');
  insertUnicode(0x2014); // -
  insertChar(' ');
}

///////////////////
// picture/textbox
///////////////////
void STOFFGraphicListener::insertGroup(STOFFBox2f const &bdbox, STOFFSubDocumentPtr subDocument)
{
  if (!m_ds->m_isDocumentStarted || m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertGroup: can not insert a group\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  handleSubDocument(bdbox[0], subDocument, libstoff::DOC_GRAPHIC_GROUP);
}

///////////////////
// table
///////////////////
void STOFFGraphicListener::insertTable(STOFFPosition const &pos, STOFFTable &table, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted || m_ps->m_inSubDocument) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertTable insert a table in a subdocument is not implemented\n"));
    return;
  }
  if (!openFrame(pos, style)) return;

  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = libstoff::DOC_TABLE;

  shared_ptr<STOFFListener> listen(this, STOFF_shared_ptr_noop_deleter<STOFFGraphicListener>());
  try {
    table.sendTable(listen);
  }
  catch (...) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertTable exception catched \n"));
  }
  _endSubDocument();
  _popParsingState();

  closeFrame();
}

void STOFFGraphicListener::openTable(STOFFTable const &table)
{
  if (!m_ps->m_isFrameOpened) {
    if (m_ps->m_isTextBoxOpened) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::openTable: must not be called inside a textbox\n"));
      STOFFPosition pos(m_ps->m_origin, STOFFVec2f(400,100), librevenge::RVNG_POINT);
      pos.m_anchorTo=STOFFPosition::Page;
      openTable(pos, table);
      return;
    }
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTable: called outside openFrame\n"));
    return;
  }
  openTable(m_ps->m_framePosition, table);
}

void STOFFGraphicListener::openTable(STOFFPosition const &pos, STOFFTable const &table)
{
  if (!m_ps->m_isFrameOpened || m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTable: no frame is already open...\n"));
    return;
  }

  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  librevenge::RVNGPropertyList propList;
  // default value: which can be redefined by table
  propList.insert("table:align", "left");
  if (m_ps->m_paragraph.m_propertyList["fo:margin-left"])
    propList.insert("fo:margin-left", m_ps->m_paragraph.m_propertyList["fo:margin-left"]->clone());
  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = libstoff::DOC_TABLE;

  _handleFrameParameters(propList, pos, STOFFGraphicStyle());
  table.addTablePropertiesTo(propList);
  m_documentInterface->startTableObject(propList);
  m_ps->m_isTableOpened = true;
}

void STOFFGraphicListener::closeTable()
{
  if (!m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeTable: called with m_isTableOpened=false\n"));
    return;
  }

  m_ps->m_isTableOpened = false;
  _endSubDocument();
  m_documentInterface->endTableObject();

  _popParsingState();
}

void STOFFGraphicListener::openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow)
{
  if (m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTableRow: called with m_isTableRowOpened=true\n"));
    return;
  }
  if (!m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTableRow: called with m_isTableOpened=false\n"));
    return;
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:is-header-row", headerRow);

  if (h > 0)
    propList.insert("style:row-height", h, unit);
  else if (h < 0)
    propList.insert("style:min-row-height", -h, unit);
  m_documentInterface->openTableRow(propList);
  m_ps->m_isTableRowOpened = true;
}

void STOFFGraphicListener::closeTableRow()
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeTableRow: called with m_isTableRowOpened=false\n"));
    return;
  }
  m_ps->m_isTableRowOpened = false;
  m_documentInterface->closeTableRow();
}

void STOFFGraphicListener::addEmptyTableCell(STOFFVec2i const &pos, STOFFVec2i span)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::addEmptyTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::addEmptyTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:column", pos[0]);
  propList.insert("librevenge:row", pos[1]);
  propList.insert("table:number-columns-spanned", span[0]);
  propList.insert("table:number-rows-spanned", span[1]);
  m_documentInterface->openTableCell(propList);
  m_documentInterface->closeTableCell();
}

void STOFFGraphicListener::openTableCell(STOFFCell const &cell)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }

  librevenge::RVNGPropertyList propList;
  cell.addTo(propList);
  m_ps->m_isTableCellOpened = true;
  m_documentInterface->openTableCell(propList);
}

void STOFFGraphicListener::closeTableCell()
{
  if (!m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeTableCell: called with m_isTableCellOpened=false\n"));
    return;
  }

  _closeParagraph();
  m_ps->m_paragraph.m_listLevelIndex=0;
  _changeList(); // flush the list exterior

  m_documentInterface->closeTableCell();
  m_ps->m_isTableCellOpened = false;
}

///////////////////
// frame/group
///////////////////
bool STOFFGraphicListener::openFrame(STOFFPosition const &pos, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openFrame: the document is not started\n"));
    return false;
  }
  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openFrame: called in table but cell is not opened\n"));
    return false;
  }
  if (m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openFrame: called but a frame is already opened\n"));
    return false;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  m_ps->m_isFrameOpened = true;
  m_ps->m_framePosition=pos;
  m_ps->m_frameStyle=style;
  return true;
}

void STOFFGraphicListener::closeFrame()
{
  if (!m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeFrame: called but no frame is already opened\n"));
    return;
  }
  m_ps->m_isFrameOpened = false;
}

bool  STOFFGraphicListener::openGroup(STOFFPosition const &pos)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openGroup: the document is not started\n"));
    return false;
  }
  if (m_ps->m_isTableOpened || m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openGroup: called in table or in a text zone\n"));
    return false;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();

  librevenge::RVNGPropertyList propList;
  _handleFrameParameters(propList, pos, STOFFGraphicStyle());

  _pushParsingState();
  _startSubDocument();
  m_ps->m_isGroupOpened = true;

  m_documentInterface->openGroup(propList);

  return true;
}

void  STOFFGraphicListener::closeGroup()
{
  if (!m_ps->m_isGroupOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeGroup: called but no group is already opened\n"));
    return;
  }
  _endSubDocument();
  _popParsingState();
  m_documentInterface->closeGroup();
}

bool STOFFGraphicListener::openLayer(librevenge::RVNGString const &layerName)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openLayer: the document is not started\n"));
    return false;
  }
  if (m_ps->m_isTableOpened || m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openLayer: called in table or in a text zone\n"));
    return false;
  }
  if (m_ps->m_isLayerOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openLayer: called but layer is already opened\n"));
    return false;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();

  _pushParsingState();
  _startSubDocument();
  m_ps->m_isLayerOpened = true;

  librevenge::RVNGPropertyList propList;
  propList.insert("draw:layer", layerName);
  m_documentInterface->startLayer(propList);
  return true;
}

void  STOFFGraphicListener::closeLayer()
{
  if (!m_ps->m_isLayerOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeLayer: called but no layer is already opened\n"));
    return;
  }
  m_documentInterface->endLayer();
  _endSubDocument();
  _popParsingState();
}

void STOFFGraphicListener::_handleFrameParameters(librevenge::RVNGPropertyList &list, STOFFPosition const &pos, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted)
    return;

  librevenge::RVNGUnit unit = pos.unit();
  float pointFactor = pos.getInvUnitScale(librevenge::RVNG_POINT);
  // first compute the origin ( in given unit and in point)
  //STOFFVec2f origin = pos.origin()-pointFactor*m_ps->m_origin;
  STOFFVec2f originPt = (1.f/pointFactor)*pos.origin()-m_ps->m_origin;
  STOFFVec2f size = pos.size();
  style.addTo(list);

  list.insert("svg:x",originPt[0], librevenge::RVNG_POINT);
  list.insert("svg:y",originPt[1], librevenge::RVNG_POINT);
  if (size.x()>0)
    list.insert("svg:width",size.x(), unit);
  else if (size.x()<0)
    list.insert("fo:min-width",-size.x(), unit);
  if (size.y()>0)
    list.insert("svg:height",size.y(), unit);
  else if (size.y()<0)
    list.insert("fo:min-height",-size.y(), unit);
  if (pos.order() > 0)
    list.insert("draw:z-index", pos.order());
  if (pos.naturalSize().x() > 4*pointFactor && pos.naturalSize().y() > 4*pointFactor) {
    list.insert("librevenge:naturalWidth", pos.naturalSize().x(), pos.unit());
    list.insert("librevenge:naturalHeight", pos.naturalSize().y(), pos.unit());
  }
  STOFFVec2f TLClip = (1.f/pointFactor)*pos.leftTopClipping();
  STOFFVec2f RBClip = (1.f/pointFactor)*pos.rightBottomClipping();
  if (TLClip[0] > 0 || TLClip[1] > 0 || RBClip[0] > 0 || RBClip[1] > 0) {
    // in ODF1.2 we need to separate the value with ,
    std::stringstream s;
    s << "rect(" << TLClip[1] << "pt " << RBClip[0] << "pt "
      <<  RBClip[1] << "pt " << TLClip[0] << "pt)";
    list.insert("fo:clip", s.str().c_str());
  }

  if (pos.m_wrapping ==  STOFFPosition::WDynamic)
    list.insert("style:wrap", "dynamic");
  else if (pos.m_wrapping ==  STOFFPosition::WBackground) {
    list.insert("style:wrap", "run-through");
    list.insert("style:run-through", "background");
  }
  else if (pos.m_wrapping ==  STOFFPosition::WForeground) {
    list.insert("style:wrap", "run-through");
    list.insert("style:run-through", "foreground");
  }
  else if (pos.m_wrapping ==  STOFFPosition::WParallel) {
    list.insert("style:wrap", "parallel");
    list.insert("style:run-through", "foreground");
  }
  else if (pos.m_wrapping ==  STOFFPosition::WRunThrough)
    list.insert("style:wrap", "run-through");
  else
    list.insert("style:wrap", "none");
  STOFF_DEBUG_MSG(("STOFFGraphicListener::_handleFrameParameters: not fully implemented\n"));
}

///////////////////
// subdocument
///////////////////
void STOFFGraphicListener::handleSubDocument(STOFFVec2f const &orig, STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::handleSubDocument: the graphic is not started\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  STOFFVec2f actOrigin=m_ps->m_origin;
  _pushParsingState();
  m_ps->m_origin=actOrigin-orig;
  _startSubDocument();
  m_ps->m_subDocumentType = subDocumentType;

  m_ps->m_list.reset();
  if (subDocumentType==libstoff::DOC_TEXT_BOX)
    m_ps->m_isTextBoxOpened=true;
  else if (subDocumentType==libstoff::DOC_HEADER_FOOTER_REGION) {
    m_ds->m_isHeaderFooterRegionOpened=true;
    m_ps->m_isTextBoxOpened=true;
  }
  else if (subDocumentType==libstoff::DOC_COMMENT_ANNOTATION || subDocumentType==libstoff::DOC_NOTE)
    m_ps->m_inNote=true;
  // Check whether the document is calling itself
  bool sendDoc = true;
  for (size_t i = 0; i < m_ds->m_subDocuments.size(); i++) {
    if (!subDocument)
      break;
    if (subDocument == m_ds->m_subDocuments[i]) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::handleSubDocument: recursif call, stop...\n"));
      sendDoc = false;
      break;
    }
  }
  if (sendDoc) {
    if (subDocument) {
      m_ds->m_subDocuments.push_back(subDocument);
      shared_ptr<STOFFListener> listen(this, STOFF_shared_ptr_noop_deleter<STOFFListener>());
      try {
        subDocument->parse(listen, subDocumentType);
      }
      catch (...) {
        STOFF_DEBUG_MSG(("Works: STOFFGraphicListener::handleSubDocument exception catched \n"));
      }
      m_ds->m_subDocuments.pop_back();
    }
  }

  _endSubDocument();
  _popParsingState();

  if (subDocumentType==libstoff::DOC_HEADER_FOOTER_REGION)
    m_ds->m_isHeaderFooterRegionOpened = false;
}

bool STOFFGraphicListener::isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const
{
  if (!m_ds->m_isDocumentStarted || !m_ps->m_inSubDocument)
    return false;
  subdocType = m_ps->m_subDocumentType;
  return true;
}

void STOFFGraphicListener::_startSubDocument()
{
  if (!m_ds->m_isDocumentStarted) return;
  m_ps->m_inSubDocument = true;
}

void STOFFGraphicListener::_endSubDocument()
{
  if (!m_ds->m_isDocumentStarted) return;
  if (m_ps->m_isTableOpened)
    closeTable();
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();
  if (m_ps->isInTextZone()) {
    m_ps->m_paragraph.m_listLevelIndex=0;
    _changeList(); // flush the list exterior
  }
}

///////////////////
// others
///////////////////

// ---------- state stack ------------------
shared_ptr<STOFFGraphicListenerInternal::State> STOFFGraphicListener::_pushParsingState()
{
  shared_ptr<STOFFGraphicListenerInternal::State> actual = m_ps;
  m_psStack.push_back(actual);
  m_ps.reset(new STOFFGraphicListenerInternal::State);
  return actual;
}

void STOFFGraphicListener::_popParsingState()
{
  if (m_psStack.size()==0) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_popParsingState: psStack is empty()\n"));
    throw libstoff::ParseException();
  }
  m_ps = m_psStack.back();
  m_psStack.pop_back();
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
