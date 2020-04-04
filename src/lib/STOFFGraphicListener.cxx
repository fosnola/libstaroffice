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

#include <time.h>

#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>

#include <librevenge/librevenge.h>

#include "libstaroffice/libstaroffice.hxx"
#include "libstaroffice_internal.hxx"

#include "STOFFCell.hxx"
#include "STOFFFont.hxx"
#include "STOFFFrameStyle.hxx"
#include "STOFFGraphicStyle.hxx"
#include "STOFFGraphicShape.hxx"
#include "STOFFInputStream.hxx"
#include "STOFFList.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFParagraph.hxx"
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
  explicit GraphicState(std::vector<STOFFPageSpan> const &pageList)
    : m_pageList(pageList)
    , m_metaData()
    , m_isDocumentStarted(false)
    , m_isPageSpanOpened(false)
    , m_isMasterPageSpanOpened(false)
    , m_isAtLeastOnePageOpened(false)
    , m_isHeaderFooterOpened(false)
    , m_isHeaderFooterRegionOpened(false)
    , m_pageSpan()
    , m_sentListMarkers()
    , m_subDocuments()
    , m_definedFontStyleSet()
    , m_definedGraphicStyleSet()
    , m_definedParagraphStyleSet()
    , m_section()
  {
  }
  GraphicState(GraphicState const &)=default;
  GraphicState(GraphicState &&)=default;
  GraphicState &operator=(GraphicState const &)=default;
  GraphicState &operator=(GraphicState &&)=default;
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
  //! the set of defined font style
  std::set<librevenge::RVNGString> m_definedFontStyleSet;
  //! the set of defined graphic style
  std::set<librevenge::RVNGString> m_definedGraphicStyleSet;
  //! the set of defined paragraph style
  std::set<librevenge::RVNGString> m_definedParagraphStyleSet;
  //! am empty section
  STOFFSection m_section;
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
    return m_inNote || m_inLink || m_isTextBoxOpened || m_isTableCellOpened;
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
  std::shared_ptr<STOFFList> m_list;

  //! a flag to know if openFrame was called
  bool m_isFrameOpened;
  //! the frame position
  STOFFFrameStyle m_framePosition;
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

State::State()
  : m_origin(0,0)
  , m_textBuffer("")
  , m_font()/* default time 12 */
  , m_paragraph()
  , m_list()
  , m_isFrameOpened(false)
  , m_framePosition()
  , m_frameStyle()
  , m_isTextBoxOpened(false)
  , m_isGroupOpened(false)
  , m_isLayerOpened(false)
  , m_isSpanOpened(false)
  , m_isParagraphOpened(false)
  , m_isListElementOpened(false)
  , m_listOrderedLevels()
  , m_isTableOpened(false)
  , m_isTableRowOpened(false)
  , m_isTableColumnOpened(false)
  , m_isTableCellOpened(false)
  , m_currentPage(0)
  , m_numPagesRemainingInSpan(0)
  , m_currentPageNumber(1)
  , m_inLink(false)
  , m_inNote(false)
  , m_inSubDocument(false)
  , m_subDocumentType(libstoff::DOC_NONE)
{
}
}

STOFFGraphicListener::STOFFGraphicListener(STOFFListManagerPtr listManager, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGDrawingInterface *drawingInterface)
  : STOFFListener(listManager)
  , m_ds(new STOFFGraphicListenerInternal::GraphicState(pageList))
  , m_ps(new STOFFGraphicListenerInternal::State)
  , m_psStack()
  , m_drawingInterface(drawingInterface)
  , m_presentationInterface(nullptr)
{
}

STOFFGraphicListener::STOFFGraphicListener(STOFFListManagerPtr listManager, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGPresentationInterface *presentationInterface)
  : STOFFListener(listManager)
  , m_ds(new STOFFGraphicListenerInternal::GraphicState(pageList))
  , m_ps(new STOFFGraphicListenerInternal::State)
  , m_psStack()
  , m_drawingInterface(nullptr)
  , m_presentationInterface(presentationInterface)
{
}

STOFFGraphicListener::~STOFFGraphicListener()
{
}

void STOFFGraphicListener::defineStyle(STOFFFont const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedFontStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  librevenge::RVNGPropertyList pList(style.m_propertyList);
  STOFFFont::checkForDefault(pList);
  if (m_drawingInterface)
    m_drawingInterface->defineCharacterStyle(pList);
  else
    m_presentationInterface->defineCharacterStyle(pList);
}

void STOFFGraphicListener::defineStyle(STOFFGraphicStyle const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedGraphicStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  librevenge::RVNGPropertyList pList(style.m_propertyList);
  STOFFGraphicStyle::checkForDefault(pList);
  STOFFGraphicStyle::checkForPadding(pList);
  if (m_drawingInterface)
    m_drawingInterface->setStyle(pList);
  else
    m_presentationInterface->setStyle(pList);
}

void STOFFGraphicListener::defineStyle(STOFFParagraph const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedParagraphStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  if (m_drawingInterface)
    m_drawingInterface->defineParagraphStyle(style.m_propertyList);
  else
    m_presentationInterface->defineParagraphStyle(style.m_propertyList);
}

bool STOFFGraphicListener::isFontStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedFontStyleSet.find(name)!=m_ds->m_definedFontStyleSet.end();
}

bool STOFFGraphicListener::isGraphicStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedGraphicStyleSet.find(name)!=m_ds->m_definedGraphicStyleSet.end();
}

bool STOFFGraphicListener::isParagraphStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedParagraphStyleSet.find(name)!=m_ds->m_definedParagraphStyleSet.end();
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
  m_ps->m_textBuffer.append(char(character));
}

void STOFFGraphicListener::insertUnicode(uint32_t val)
{
  if (!m_ps->isInTextZone()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertUnicode: called outside a text zone\n"));
    return;
  }
  // undef character, we skip it
  if (val == 0xfffd) return;
  if (val<0x20 && val!=0x9 && val!=0xa && val!=0xd) {
    static int numErrors=0;
    if (++numErrors<10) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::insertUnicode: find odd char %x\n", static_cast<unsigned int>(val)));
    }
    return;
  }
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
    if (m_drawingInterface)
      m_drawingInterface->insertLineBreak();
    else
      m_presentationInterface->insertLineBreak();
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
  if (m_drawingInterface)
    m_drawingInterface->insertTab();
  else
    m_presentationInterface->insertTab();
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
  if (m_ps->m_paragraph.m_listLevelIndex>20) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::setParagraph: the level index seems bad, resets it to 10\n"));
    m_ps->m_paragraph.m_listLevelIndex=10;
  }
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
  field.addTo(propList);
  _flushText();
  _openSpan();
  if (m_drawingInterface)
    m_drawingInterface->insertField(propList);
  else
    m_presentationInterface->insertField(propList);
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
  _flushText();
  if (!m_ps->m_isSpanOpened) _openSpan();
  librevenge::RVNGPropertyList propList;
  link.addTo(propList);
  if (m_drawingInterface)
    m_drawingInterface->openLink(propList);
  else
    m_presentationInterface->openLink(propList);
  _pushParsingState();
  m_ps->m_inLink=true;
  // we do not want any close open paragraph in a link
  m_ps->m_isParagraphOpened=true;
  m_ps->m_isSpanOpened=true;
}

void STOFFGraphicListener::closeLink()
{
  if (!m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeLink: closed outside a link\n"));
    return;
  }
  _flushText();
  if (m_drawingInterface)
    m_drawingInterface->closeLink();
  else
    m_presentationInterface->closeLink();
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
  if (m_drawingInterface) {
    m_drawingInterface->startDocument(librevenge::RVNGPropertyList());
    m_drawingInterface->setDocumentMetaData(m_ds->m_metaData);
  }
  else {
    m_presentationInterface->startDocument(librevenge::RVNGPropertyList());
    m_presentationInterface->setDocumentMetaData(m_ds->m_metaData);
  }
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
  if (m_drawingInterface)
    m_drawingInterface->endDocument();
  else
    m_presentationInterface->endDocument();
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

  if (m_drawingInterface)
    m_drawingInterface->startMasterPage(propList);
  else
    m_presentationInterface->startMasterSlide(propList);
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
  auto it = m_ds->m_pageList.begin();
  m_ps->m_currentPage++;
  while (true) {
    actPage+=unsigned(it->m_pageSpan);
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
  propList.insert("svg:x",double(m_ps->m_origin.x()), librevenge::RVNG_POINT);
  propList.insert("svg:y",double(m_ps->m_origin.y()), librevenge::RVNG_POINT);
  propList.insert("librevenge:enforce-frame",true);

  if (!m_ds->m_isPageSpanOpened) {
    if (m_drawingInterface)
      m_drawingInterface->startPage(propList);
    else
      m_presentationInterface->startSlide(propList);
  }
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
  if (m_drawingInterface) {
    if (masterPage)
      m_drawingInterface->endMasterPage();
    else
      m_drawingInterface->endPage();
  }
  else {
    if (masterPage)
      m_presentationInterface->endMasterSlide();
    else
      m_presentationInterface->endSlide();
  }
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
  if (m_drawingInterface)
    m_drawingInterface->openParagraph(propList);
  else
    m_presentationInterface->openParagraph(propList);

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
    if (m_drawingInterface)
      m_drawingInterface->closeParagraph();
    else
      m_presentationInterface->closeParagraph();
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
  if (m_drawingInterface)
    m_drawingInterface->openListElement(propList);
  else
    m_presentationInterface->openListElement(propList);
  _resetParagraphState(true);
}

void STOFFGraphicListener::_closeListElement()
{
  if (m_ps->m_isListElementOpened) {
    if (m_ps->m_isSpanOpened)
      _closeSpan();

    if (m_ps->m_list) m_ps->m_list->closeElement();
    if (m_drawingInterface)
      m_drawingInterface->closeListElement();
    else
      m_presentationInterface->closeListElement();
  }

  m_ps->m_isListElementOpened = m_ps->m_isParagraphOpened = false;
}

int STOFFGraphicListener::_getListId() const
{
  auto newLevel= size_t(m_ps->m_paragraph.m_listLevelIndex);
  if (newLevel == 0) return -1;
  int newListId = m_ps->m_paragraph.m_listId;
  if (newListId > 0) return newListId;
  static bool first = true;
  if (first) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::_getListId: the list id is not set, try to find a new one\n"));
    first = false;
  }
  auto list=m_listManager->getNewList(m_ps->m_list, int(newLevel), m_ps->m_paragraph.m_listLevel);
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
  auto newLevel= size_t(m_ps->m_paragraph.m_listLevelIndex);
  int newListId = newLevel>0 ? _getListId() : -1;
  bool changeList = newLevel &&
                    (m_ps->m_list && m_ps->m_list->getId()!=newListId);
  size_t minLevel = changeList ? 0 : newLevel;
  while (actualLevel > minLevel) {
    if (m_drawingInterface) {
      if (m_ps->m_listOrderedLevels[--actualLevel])
        m_drawingInterface->closeOrderedListLevel();
      else
        m_drawingInterface->closeUnorderedListLevel();
    }
    else {
      if (m_ps->m_listOrderedLevels[--actualLevel])
        m_presentationInterface->closeOrderedListLevel();
      else
        m_presentationInterface->closeUnorderedListLevel();
    }
  }

  if (newLevel) {
    std::shared_ptr<STOFFList> theList;

    theList=m_listManager->getList(newListId);
    if (!theList) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::_changeList: can not find any list\n"));
      m_ps->m_listOrderedLevels.resize(actualLevel);
      return;
    }
    m_listManager->needToSend(newListId, m_ds->m_sentListMarkers);
    m_ps->m_list = theList;
    m_ps->m_list->setLevel(int(newLevel));
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
    if (m_drawingInterface) {
      if (ordered)
        m_drawingInterface->openOrderedListLevel(level);
      else
        m_drawingInterface->openUnorderedListLevel(level);
    }
    else {
      if (ordered)
        m_presentationInterface->openOrderedListLevel(level);
      else
        m_presentationInterface->openUnorderedListLevel(level);
    }
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
  m_ps->m_font.addTo(propList);
  STOFFFont::checkForDefault(propList);

  if (m_drawingInterface)
    m_drawingInterface->openSpan(propList);
  else
    m_presentationInterface->openSpan(propList);

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
  if (m_drawingInterface)
    m_drawingInterface->closeSpan();
  else
    m_presentationInterface->closeSpan();
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
        if (m_drawingInterface)
          m_drawingInterface->insertText(tmpText);
        else
          m_presentationInterface->insertText(tmpText);
        tmpText.clear();
      }
      if (m_drawingInterface)
        m_drawingInterface->insertSpace();
      else
        m_presentationInterface->insertSpace();
    }
    else
      tmpText.append(i());
  }
  if (m_drawingInterface)
    m_drawingInterface->insertText(tmpText);
  else
    m_presentationInterface->insertText(tmpText);
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

  // normally graphic document does not have header/footer, so do the minimum
  STOFF_DEBUG_MSG(("STOFFGraphicListener::openHeader: Oops I am called\n"));

  // we do not have any header interface, so mimick it by creating a textbox
  STOFFFrameStyle frame;
  auto &pos=frame.m_position;
  pos.setOrigin(STOFFVec2f(20,20)); // fixme
  pos.setSize(STOFFVec2f(-20,-10));
  pos.m_anchorTo=STOFFPosition::Page;
  if (!openFrame(frame))
    return false;
  m_ds->m_isHeaderFooterOpened=true;
  librevenge::RVNGPropertyList propList(extras);
  _handleFrameParameters(propList, frame, STOFFGraphicStyle());

  if (m_drawingInterface)
    m_drawingInterface->startTextObject(propList);
  else
    m_presentationInterface->startTextObject(propList);
  return true;
}

bool STOFFGraphicListener::insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &/*which*/)
{
  if (!m_ds->m_isHeaderFooterOpened || m_ds->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  return true;
}

bool STOFFGraphicListener::closeHeader()
{
  if (!m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeHeader: Oops no opened header/footer\n"));
    return false;
  }
  if (m_drawingInterface)
    m_drawingInterface->endTextObject();
  else
    m_presentationInterface->endTextObject();
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

  // normally graphic document does not have header/footer, so do the minimum
  STOFF_DEBUG_MSG(("STOFFGraphicListener::openFooter: Oops I am called\n"));

  // we do not have any footer interface, so mimick it by creating a textbox
  STOFFFrameStyle frame;
  auto &pos=frame.m_position;
  pos.setOrigin(STOFFVec2f(20,700)); // fixme: ypos
  pos.setSize(STOFFVec2f(-20,-10));
  pos.m_anchorTo=STOFFPosition::Page;
  if (!openFrame(frame))
    return false;
  m_ds->m_isHeaderFooterOpened=true;
  librevenge::RVNGPropertyList propList(extras);
  _handleFrameParameters(propList, frame, STOFFGraphicStyle());

  if (m_drawingInterface)
    m_drawingInterface->startTextObject(propList);
  else
    m_presentationInterface->startTextObject(propList);
  return true;
}

bool STOFFGraphicListener::insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &/*which*/)
{
  if (!m_ds->m_isHeaderFooterOpened || m_ds->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  return true;
}

bool STOFFGraphicListener::closeFooter()
{
  if (!m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeFooter: Oops no opened header/footer\n"));
    return false;
  }
  if (m_drawingInterface)
    m_drawingInterface->endTextObject();
  else
    m_presentationInterface->endTextObject();
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
  return m_ds->m_section;
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
void STOFFGraphicListener::insertPicture(STOFFFrameStyle const &frame, STOFFEmbeddedObject const &picture, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertPicture: the document is not started\n"));
    return;
  }
  if (m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertPicture: a frame is already open\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  librevenge::RVNGPropertyList list;
  style.addTo(list);
  if (m_drawingInterface)
    m_drawingInterface->setStyle(list);
  else
    m_presentationInterface->setStyle(list);
  list.clear();
  _handleFrameParameters(list, frame, style);
  if (picture.addTo(list)) {
    if (m_drawingInterface)
      m_drawingInterface->drawGraphicObject(list);
    else
      m_presentationInterface->drawGraphicObject(list);
  }
}

void STOFFGraphicListener::insertEquation(STOFFFrameStyle const &frame, librevenge::RVNGString const &equation,
    STOFFGraphicStyle const &style)
{
  if (equation.empty()) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertEquation: oops the equation is empty\n"));
    return;
  }
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertEquation: the document is not started\n"));
    return;
  }
  if (m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertEquation: a frame is already open\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  librevenge::RVNGPropertyList list;
  style.addTo(list);
  if (m_drawingInterface)
    m_drawingInterface->setStyle(list);
  else
    m_presentationInterface->setStyle(list);
  list.clear();
  _handleFrameParameters(list, frame, style);
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:mime-type", "application/mathml+xml");
  propList.insert("office:binary-data", equation);
  if (m_drawingInterface)
    m_drawingInterface->drawGraphicObject(list);
  else
    m_presentationInterface->drawGraphicObject(list);
}

void STOFFGraphicListener::insertShape(STOFFFrameStyle const &frame, STOFFGraphicShape const &shape, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertShape: the document is not started\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  // now check that the anchor is coherent with the actual state
  if (frame.m_position.m_anchorTo==STOFFPosition::Paragraph) {
    if (m_ps->m_isParagraphOpened)
      _flushText();
    else
      _openParagraph();
  }
  else if (frame.m_position.m_anchorTo==STOFFPosition::Char || frame.m_position.m_anchorTo==STOFFPosition::CharBaseLine) {
    if (m_ps->m_isSpanOpened)
      _flushText();
    else
      _openSpan();
  }

  librevenge::RVNGPropertyList shapeProp, styleProp;
  frame.addTo(shapeProp);
  shape.addTo(shapeProp);
  style.addTo(styleProp);
  STOFFGraphicStyle::checkForDefault(styleProp);
  if (m_drawingInterface) {
    m_drawingInterface->setStyle(styleProp);
    switch (shape.m_command) {
    case STOFFGraphicShape::C_Connector:
      m_drawingInterface->drawConnector(shapeProp);
      break;
    case STOFFGraphicShape::C_Ellipse:
      m_drawingInterface->drawEllipse(shapeProp);
      break;
    case STOFFGraphicShape::C_Path:
      m_drawingInterface->drawPath(shapeProp);
      break;
    case STOFFGraphicShape::C_Polyline:
      m_drawingInterface->drawPolyline(shapeProp);
      break;
    case STOFFGraphicShape::C_Polygon:
      m_drawingInterface->drawPolygon(shapeProp);
      break;
    case STOFFGraphicShape::C_Rectangle:
      m_drawingInterface->drawRectangle(shapeProp);
      break;
    case STOFFGraphicShape::C_Unknown:
      break;
    default:
      STOFF_DEBUG_MSG(("STOFFGraphicListener::insertShape: unexpected shape\n"));
      break;
    }
  }
  else {
    m_presentationInterface->setStyle(styleProp);
    switch (shape.m_command) {
    case STOFFGraphicShape::C_Connector:
      m_presentationInterface->drawConnector(shapeProp);
      break;
    case STOFFGraphicShape::C_Ellipse:
      m_presentationInterface->drawEllipse(shapeProp);
      break;
    case STOFFGraphicShape::C_Path:
      m_presentationInterface->drawPath(shapeProp);
      break;
    case STOFFGraphicShape::C_Polyline:
      m_presentationInterface->drawPolyline(shapeProp);
      break;
    case STOFFGraphicShape::C_Polygon:
      m_presentationInterface->drawPolygon(shapeProp);
      break;
    case STOFFGraphicShape::C_Rectangle:
      m_presentationInterface->drawRectangle(shapeProp);
      break;
    case STOFFGraphicShape::C_Unknown:
      break;
    default:
      STOFF_DEBUG_MSG(("STOFFGraphicListener::insertShape: unexpected shape\n"));
      break;
    }
  }
}

void STOFFGraphicListener::insertTextBox
(STOFFFrameStyle const &frame, STOFFSubDocumentPtr subDocument, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertTextBox: the document is not started\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  if (m_ps->m_isTextBoxOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::insertTextBox: can not insert a textbox in a textbox\n"));
    handleSubDocument(subDocument, libstoff::DOC_TEXT_BOX);
    return;
  }
  if (!openFrame(frame))
    return;
  librevenge::RVNGPropertyList propList;
  _handleFrameParameters(propList, frame, style);
  STOFFGraphicStyle::checkForPadding(propList);
  if (m_drawingInterface)
    m_drawingInterface->startTextObject(propList);
  else
    m_presentationInterface->startTextObject(propList);
  handleSubDocument(subDocument, libstoff::DOC_TEXT_BOX);
  if (m_drawingInterface)
    m_drawingInterface->endTextObject();
  else
    m_presentationInterface->endTextObject();
  closeFrame();
}

///////////////////
// table
///////////////////
void STOFFGraphicListener::openTable(STOFFTable const &table)
{
  if (!m_ps->m_isFrameOpened) {
    if (m_ps->m_isTextBoxOpened) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::openTable: must not be called inside a textbox\n"));
      STOFFFrameStyle frame;
      auto &pos=frame.m_position;
      pos.setOrigin(m_ps->m_origin);
      pos.setSize(STOFFVec2f(400,100));
      pos.m_anchorTo=STOFFPosition::Page;
      openTable(frame, table);
      return;
    }
    STOFF_DEBUG_MSG(("STOFFGraphicListener::openTable: called outside openFrame\n"));
    return;
  }
  openTable(m_ps->m_framePosition, table);
}

void STOFFGraphicListener::openTable(STOFFFrameStyle const &frame, STOFFTable const &table)
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

  _handleFrameParameters(propList, frame, STOFFGraphicStyle());
  table.addTablePropertiesTo(propList);
  if (m_drawingInterface)
    m_drawingInterface->startTableObject(propList);
  else
    m_presentationInterface->startTableObject(propList);
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
  if (m_drawingInterface)
    m_drawingInterface->endTableObject();
  else
    m_presentationInterface->endTableObject();

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
    propList.insert("style:row-height", double(h), unit);
  else if (h < 0)
    propList.insert("style:min-row-height", double(-h), unit);
  if (m_drawingInterface)
    m_drawingInterface->openTableRow(propList);
  else
    m_presentationInterface->openTableRow(propList);
  m_ps->m_isTableRowOpened = true;
}

void STOFFGraphicListener::closeTableRow()
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeTableRow: called with m_isTableRowOpened=false\n"));
    return;
  }
  m_ps->m_isTableRowOpened = false;
  if (m_drawingInterface)
    m_drawingInterface->closeTableRow();
  else
    m_presentationInterface->closeTableRow();
}

void STOFFGraphicListener::addCoveredTableCell(STOFFVec2i const &pos)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::addCoveredTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::addCoveredTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:column", pos[0]);
  propList.insert("librevenge:row", pos[1]);
  if (m_drawingInterface)
    m_drawingInterface->insertCoveredTableCell(propList);
  else
    m_presentationInterface->insertCoveredTableCell(propList);
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
  if (m_drawingInterface) {
    m_drawingInterface->openTableCell(propList);
    m_drawingInterface->closeTableCell();
  }
  else {
    m_presentationInterface->openTableCell(propList);
    m_presentationInterface->closeTableCell();
  }
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
  if (m_drawingInterface)
    m_drawingInterface->openTableCell(propList);
  else
    m_presentationInterface->openTableCell(propList);
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

  if (m_drawingInterface)
    m_drawingInterface->closeTableCell();
  else
    m_presentationInterface->closeTableCell();
  m_ps->m_isTableCellOpened = false;
}

///////////////////
// frame/group
///////////////////
bool STOFFGraphicListener::openFrame(STOFFFrameStyle const &frame, STOFFGraphicStyle const &style)
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
  m_ps->m_framePosition=frame;
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

bool STOFFGraphicListener::openGroup(STOFFFrameStyle const &frame)
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

  _pushParsingState();
  _startSubDocument();
  m_ps->m_isGroupOpened = true;

  librevenge::RVNGPropertyList propList;
  frame.addTo(propList);
  if (m_drawingInterface)
    m_drawingInterface->openGroup(propList);
  else
    m_presentationInterface->openGroup(propList);

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
  if (m_drawingInterface)
    m_drawingInterface->closeGroup();
  else
    m_presentationInterface->closeGroup();
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
  if (m_drawingInterface)
    m_drawingInterface->startLayer(propList);
  else
    m_presentationInterface->startLayer(propList);
  return true;
}

void  STOFFGraphicListener::closeLayer()
{
  if (!m_ps->m_isLayerOpened) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::closeLayer: called but no layer is already opened\n"));
    return;
  }
  if (m_drawingInterface)
    m_drawingInterface->endLayer();
  else
    m_presentationInterface->endLayer();
  _endSubDocument();
  _popParsingState();
}

void STOFFGraphicListener::_handleFrameParameters(librevenge::RVNGPropertyList &list, STOFFFrameStyle const &frame, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted)
    return;
  frame.addTo(list);
  style.addTo(list);
  if (list["text:anchor-page-number"])
    list.remove("text:anchor-page-number");
}

///////////////////
// subdocument
///////////////////
void STOFFGraphicListener::handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFGraphicListener::handleSubDocument: the graphic is not started\n"));
    return;
  }
  if (!m_ds->m_isPageSpanOpened)
    _openPageSpan();
  _pushParsingState();
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
  for (auto const &doc : m_ds->m_subDocuments) {
    if (!subDocument)
      break;
    if (!doc) continue;
    if (*subDocument == *doc) {
      STOFF_DEBUG_MSG(("STOFFGraphicListener::handleSubDocument: recursif call, stop...\n"));
      sendDoc = false;
      break;
    }
  }
  if (sendDoc) {
    if (subDocument) {
      m_ds->m_subDocuments.push_back(subDocument);
      std::shared_ptr<STOFFListener> listen(this, STOFF_shared_ptr_noop_deleter<STOFFListener>());
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
std::shared_ptr<STOFFGraphicListenerInternal::State> STOFFGraphicListener::_pushParsingState()
{
  std::shared_ptr<STOFFGraphicListenerInternal::State> actual = m_ps;
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
