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

/** \file STOFFSpreadsheetListener.cxx
 * Implements STOFFSpreadsheetListener: the libstaroffice spreadsheet processor listener
 *
 * \note this class is the only class which does the interface with
 * the librevenge::RVNGSpreadsheetInterface
 */

#define TEST_SEND_HEADERFOOTER_REGION 0

#include <time.h>

#include <cmath>
#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFCell.hxx"
#include "STOFFChart.hxx"
#include "STOFFFont.hxx"
#include "STOFFFrameStyle.hxx"
#include "STOFFGraphicStyle.hxx"
#include "STOFFGraphicShape.hxx"
#include "STOFFInputStream.hxx"
#include "STOFFList.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFParagraph.hxx"
#include "STOFFParser.hxx"
#include "STOFFPosition.hxx"
#include "STOFFSection.hxx"
#include "STOFFSubDocument.hxx"
#include "STOFFTable.hxx"

#include "STOFFSpreadsheetListener.hxx"

//! Internal and low level namespace to define the states of STOFFSpreadsheetListener
namespace STOFFSpreadsheetListenerInternal
{
//! a enum to define basic break bit
enum { PageBreakBit=0x1, ColumnBreakBit=0x2 };
//! a class to store the document state of a STOFFSpreadsheetListener
struct DocumentState {
  //! constructor
  explicit DocumentState(std::vector<STOFFPageSpan> const &pageList)
    : m_pageList(pageList)
    , m_pageSpan()
    , m_metaData()
    , m_footNoteNumber(0)
    , m_smallPictureNumber(0)
    , m_isDocumentStarted(false)
    , m_isSheetOpened(false)
    , m_isSheetRowOpened(false)
    , m_sentListMarkers()
    , m_numberingIdMap()
    , m_subDocuments()
    , m_definedFontStyleSet()
    , m_definedGraphicStyleSet()
    , m_definedParagraphStyleSet()
    , m_section()
  {
  }
  //! destructor
  ~DocumentState()
  {
  }

  //! the pages definition
  std::vector<STOFFPageSpan> m_pageList;
  //! the current page span
  STOFFPageSpan m_pageSpan;
  //! the document meta data
  librevenge::RVNGPropertyList m_metaData;

  int m_footNoteNumber /** footnote number*/;

  int m_smallPictureNumber /** number of small picture */;
  bool m_isDocumentStarted /** a flag to know if the document is open */;
  bool m_isSheetOpened /** a flag to know if a sheet is open */;
  bool m_isSheetRowOpened /** a flag to know if a row is open */;
  /// the list of marker corresponding to sent list
  std::vector<int> m_sentListMarkers;
  /** a map cell's format to id */
  std::map<librevenge::RVNGString,int> m_numberingIdMap;
  std::vector<STOFFSubDocumentPtr> m_subDocuments; /** list of document actually open */
  //! the set of defined font style
  std::set<librevenge::RVNGString> m_definedFontStyleSet;
  //! the set of defined graphic style
  std::set<librevenge::RVNGString> m_definedGraphicStyleSet;
  //! the set of defined paragraph style
  std::set<librevenge::RVNGString> m_definedParagraphStyleSet;
  //! an empty section
  STOFFSection m_section;
private:
  DocumentState(const DocumentState &);
  DocumentState &operator=(const DocumentState &);
};

/** the state of a STOFFSpreadsheetListener */
struct State {
  //! constructor
  State();
  //! destructor
  ~State() { }
  //! returns true if we are in a text zone
  bool canWriteText() const
  {
    if (m_isSheetCellOpened || m_isHeaderFooterRegionOpened) return true;
    return m_isTextboxOpened || m_isTableCellOpened || m_isNote;
  }

  //! a buffer to stored the text
  librevenge::RVNGString m_textBuffer;
  //! the number of tabs to add
  int m_numDeferredTabs;

  //! the font
  STOFFFont m_font;
  //! the paragraph
  STOFFParagraph m_paragraph;

  std::shared_ptr<STOFFList> m_list;

  bool m_isPageSpanOpened;
  bool m_isHeaderFooterOpened /** a flag to know if the header footer is started */;
  bool m_isHeaderFooterRegionOpened /** a flag to know if the header footer region is started */;
  bool m_isFrameOpened;
  bool m_isTextboxOpened;

  bool m_isHeaderFooterWithoutParagraph;

  bool m_isSpanOpened;
  bool m_isParagraphOpened;
  bool m_isListElementOpened;

  bool m_isSheetColumnOpened;
  bool m_isSheetCellOpened;

  bool m_isTableOpened;
  bool m_isTableRowOpened;
  bool m_isTableColumnOpened;
  bool m_isTableCellOpened;

  unsigned m_currentPage;
  int m_numPagesRemainingInSpan;
  int m_currentPageNumber;

  std::vector<bool> m_listOrderedLevels; //! a stack used to know what is open

  bool m_inSubDocument;

  bool m_isNote;
  bool m_inLink;
  libstoff::SubDocumentType m_subDocumentType;

private:
  State(const State &);
  State &operator=(const State &);
};

State::State()
  : m_textBuffer("")
  , m_numDeferredTabs(0)

  , m_font()

  , m_paragraph()

  , m_list()

  , m_isPageSpanOpened(false)
  , m_isHeaderFooterOpened(false)
  , m_isHeaderFooterRegionOpened(false)
  , m_isFrameOpened(false)
  , m_isTextboxOpened(false)
  , m_isHeaderFooterWithoutParagraph(false)

  , m_isSpanOpened(false)
  , m_isParagraphOpened(false)
  , m_isListElementOpened(false)

  , m_isSheetColumnOpened(false)
  , m_isSheetCellOpened(false)

  , m_isTableOpened(false)
  , m_isTableRowOpened(false)
  , m_isTableColumnOpened(false)
  , m_isTableCellOpened(false)

  , m_currentPage(0)
  , m_numPagesRemainingInSpan(0)
  , m_currentPageNumber(1)

  , m_listOrderedLevels()

  , m_inSubDocument(false)
  , m_isNote(false)
  , m_inLink(false)
  , m_subDocumentType(libstoff::DOC_NONE)
{
}
}

STOFFSpreadsheetListener::STOFFSpreadsheetListener(STOFFListManagerPtr &listManager, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGSpreadsheetInterface *documentInterface)
  : STOFFListener(listManager)
  , m_ds(new STOFFSpreadsheetListenerInternal::DocumentState(pageList))
  , m_ps(new STOFFSpreadsheetListenerInternal::State)
  , m_psStack()
  , m_documentInterface(documentInterface)
{
}

STOFFSpreadsheetListener::~STOFFSpreadsheetListener()
{
}

void STOFFSpreadsheetListener::defineStyle(STOFFFont const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedFontStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  librevenge::RVNGPropertyList pList(style.m_propertyList);
  STOFFFont::checkForDefault(pList);
  m_documentInterface->defineCharacterStyle(pList);
}

void STOFFSpreadsheetListener::defineStyle(STOFFGraphicStyle const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedGraphicStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  librevenge::RVNGPropertyList pList(style.m_propertyList);
  STOFFGraphicStyle::checkForDefault(pList);
  STOFFGraphicStyle::checkForPadding(pList);
  m_documentInterface->defineGraphicStyle(pList);
}

void STOFFSpreadsheetListener::defineStyle(STOFFParagraph const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedParagraphStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  m_documentInterface->defineParagraphStyle(style.m_propertyList);
}

bool STOFFSpreadsheetListener::isFontStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedFontStyleSet.find(name)!=m_ds->m_definedFontStyleSet.end();
}

bool STOFFSpreadsheetListener::isGraphicStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedGraphicStyleSet.find(name)!=m_ds->m_definedGraphicStyleSet.end();
}

bool STOFFSpreadsheetListener::isParagraphStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedParagraphStyleSet.find(name)!=m_ds->m_definedParagraphStyleSet.end();
}

///////////////////
// text data
///////////////////
bool STOFFSpreadsheetListener::canWriteText() const
{
  return m_ps->canWriteText();
}

void STOFFSpreadsheetListener::insertChar(uint8_t character)
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertChar: called outside a text zone\n"));
    return;
  }
  if (character >= 0x80) {
    STOFFSpreadsheetListener::insertUnicode(character);
    return;
  }
  _flushDeferredTabs();
  if (!m_ps->m_isSpanOpened) _openSpan();
  m_ps->m_textBuffer.append(char(character));
}

void STOFFSpreadsheetListener::insertUnicode(uint32_t val)
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertUnicode: called outside a text zone\n"));
    return;
  }

  // undef character, we skip it
  if (val == 0xfffd) return;
  if (val<0x20 && val!=0x9 && val!=0xa && val!=0xd) {
    static int numErrors=0;
    if (++numErrors<10) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertUnicode: find odd char %x\n", static_cast<unsigned int>(val)));
    }
    return;
  }

  _flushDeferredTabs();
  if (!m_ps->m_isSpanOpened) _openSpan();
  libstoff::appendUnicode(val, m_ps->m_textBuffer);
}

void STOFFSpreadsheetListener::insertUnicodeString(librevenge::RVNGString const &str)
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertUnicodeString: called outside a text zone\n"));
    return;
  }

  _flushDeferredTabs();
  if (!m_ps->m_isSpanOpened) _openSpan();
  m_ps->m_textBuffer.append(str);
}

void STOFFSpreadsheetListener::insertEOL(bool soft)
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertEOL: called outside a text zone\n"));
    return;
  }

  if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
    _openSpan();
  _flushDeferredTabs();

  if (soft) {
    if (m_ps->m_isSpanOpened)
      _flushText();
    m_documentInterface->insertLineBreak();
  }
  else if (m_ps->m_isParagraphOpened)
    _closeParagraph();
}

void STOFFSpreadsheetListener::insertTab()
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertTab: called outside a text zone\n"));
    return;
  }

  if (!m_ps->m_isParagraphOpened) {
    m_ps->m_numDeferredTabs++;
    return;
  }
  if (m_ps->m_isSpanOpened) _flushText();
  m_ps->m_numDeferredTabs++;
  _flushDeferredTabs();
}

void STOFFSpreadsheetListener::insertBreak(STOFFSpreadsheetListener::BreakType type)
{
  if (type!=PageBreak) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertBreak: make not sense\n"));
    return;
  }
  if (!m_ps->m_isPageSpanOpened && !m_ps->m_inSubDocument)
    _openSpan();
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();
  if (m_ps->m_numPagesRemainingInSpan > 0)
    m_ps->m_numPagesRemainingInSpan--;
  else {
    if (!m_ds->m_isSheetOpened && !m_ps->m_isTableOpened && !m_ps->m_inSubDocument)
      _closePageSpan();
    else {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertBreak: oops, call inside a sheet, table, ...\n"));
    }
  }
  m_ps->m_currentPageNumber++;
}

///////////////////
// font/paragraph function
///////////////////
void STOFFSpreadsheetListener::setFont(STOFFFont const &font)
{
  if (font == m_ps->m_font) return;
  _closeSpan();
  m_ps->m_font = font;
}

STOFFFont const &STOFFSpreadsheetListener::getFont() const
{
  return m_ps->m_font;
}

bool STOFFSpreadsheetListener::isParagraphOpened() const
{
  return m_ps->m_isParagraphOpened;
}

void STOFFSpreadsheetListener::setParagraph(STOFFParagraph const &para)
{
  if (para==m_ps->m_paragraph) return;

  m_ps->m_paragraph=para;
  if (m_ps->m_paragraph.m_listLevelIndex>20) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::setParagraph: the level index seems bad, resets it to 10\n"));
    m_ps->m_paragraph.m_listLevelIndex=10;
  }
}

STOFFParagraph const &STOFFSpreadsheetListener::getParagraph() const
{
  return m_ps->m_paragraph;
}

///////////////////
// field/link :
///////////////////
void STOFFSpreadsheetListener::insertField(STOFFField const &field)
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertField: called outside a text zone\n"));
    return;
  }

  librevenge::RVNGPropertyList propList;
  field.addTo(propList);
  _flushDeferredTabs();
  _flushText();
  _openSpan();
  m_documentInterface->insertField(propList);
}

void STOFFSpreadsheetListener::openLink(STOFFLink const &link)
{
  if (!m_ps->canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openLink: called outside a text zone\n"));
    return;
  }
  if (m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener:openLink: a link is already opened\n"));
    return;
  }
  _flushDeferredTabs();
  _flushText();
  if (!m_ps->m_isSpanOpened) _openSpan();
  librevenge::RVNGPropertyList propList;
  link.addTo(propList);
  m_documentInterface->openLink(propList);
  _pushParsingState();
  m_ps->m_inLink=true;
// we do not want any close open paragraph in a link
  m_ps->m_isParagraphOpened=true;
}

void STOFFSpreadsheetListener::closeLink()
{
  if (!m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener:closeLink: can not close a link\n"));
    return;
  }
  if (m_ps->m_isSpanOpened) _closeSpan();
  m_documentInterface->closeLink();
  _popParsingState();
}

///////////////////
// document
///////////////////
void STOFFSpreadsheetListener::setDocumentLanguage(std::string locale)
{
  if (!locale.length()) return;
  m_ds->m_metaData.insert("librevenge:language", locale.c_str());
}

void STOFFSpreadsheetListener::setDocumentMetaData(const librevenge::RVNGPropertyList &list)
{
  librevenge::RVNGPropertyList::Iter i(list);
  for (i.rewind(); i.next();)
    m_ds->m_metaData.insert(i.key(), i()->getStr());
}

bool STOFFSpreadsheetListener::isDocumentStarted() const
{
  return m_ds->m_isDocumentStarted;
}

void STOFFSpreadsheetListener::startDocument()
{
  if (m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::startDocument: the document is already started\n"));
    return;
  }

  m_documentInterface->startDocument(librevenge::RVNGPropertyList());
  m_ds->m_isDocumentStarted = true;

  m_documentInterface->setDocumentMetaData(m_ds->m_metaData);
}

void STOFFSpreadsheetListener::endDocument(bool sendDelayedSubDoc)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::endDocument: the document is not started\n"));
    return;
  }

  if (!m_ps->m_isPageSpanOpened) {
    // we must call by hand openPageSpan to avoid sending any header/footer documents
    if (!sendDelayedSubDoc) _openPageSpan(false);
    _openSpan();
  }

  if (m_ps->m_isTableOpened)
    closeTable();
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  m_ps->m_paragraph.m_listLevelIndex = 0;
  _changeList(); // flush the list exterior

  // close the document nice and tight
  if (m_ds->m_isSheetOpened)
    closeSheet();

  _closePageSpan();
  m_documentInterface->endDocument();
  m_ds->m_isDocumentStarted = false;
}

///////////////////
// page
///////////////////
bool STOFFSpreadsheetListener::isPageSpanOpened() const
{
  return m_ps->m_isPageSpanOpened;
}

STOFFPageSpan const &STOFFSpreadsheetListener::getPageSpan()
{
  if (!m_ps->m_isPageSpanOpened)
    _openPageSpan();
  return m_ds->m_pageSpan;
}


void STOFFSpreadsheetListener::_openPageSpan(bool sendHeaderFooters)
{
  if (m_ps->m_isPageSpanOpened)
    return;

  if (!m_ds->m_isDocumentStarted)
    startDocument();

  if (m_ds->m_pageList.size()==0) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::_openPageSpan: can not find any page\n"));
    throw libstoff::ParseException();
  }
  unsigned actPage = 0;
  auto it = m_ds->m_pageList.begin();
  ++m_ps->m_currentPage;
  while (true) {
    actPage+=unsigned(it->m_pageSpan);
    if (actPage >= m_ps->m_currentPage)
      break;
    if (++it == m_ds->m_pageList.end()) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::_openPageSpan: can not find current page, use the previous one\n"));
      --it;
      break;
    }
  }
  STOFFPageSpan &currentPage = *it;

  librevenge::RVNGPropertyList propList;
  currentPage.getPageProperty(propList);
  propList.insert("librevenge:is-last-page-span", ++it==m_ds->m_pageList.end());

  if (!m_ps->m_isPageSpanOpened)
    m_documentInterface->openPageSpan(propList);

  m_ps->m_isPageSpanOpened = true;
  m_ds->m_pageSpan = currentPage;

  // we insert the header footer
  if (sendHeaderFooters)
    currentPage.sendHeaderFooters(this);

  m_ps->m_numPagesRemainingInSpan = (currentPage.m_pageSpan - 1);
}

void STOFFSpreadsheetListener::_closePageSpan()
{
  if (!m_ps->m_isPageSpanOpened)
    return;

  m_documentInterface->closePageSpan();
  m_ps->m_isPageSpanOpened = false;
}

///////////////////
// header/footer
///////////////////
bool STOFFSpreadsheetListener::isHeaderFooterOpened() const
{
  return m_ps->m_isHeaderFooterOpened;
}

bool STOFFSpreadsheetListener::openHeader(librevenge::RVNGPropertyList const &extras)
{
  if (m_ps->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openHeader: Oops a header/footer is already opened\n"));
    return false;
  }
  _pushParsingState();
  m_ps->m_isHeaderFooterOpened=true;
  m_documentInterface->openHeader(extras);
  return true;
}

bool STOFFSpreadsheetListener::insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which)
{
  if (!m_ps->m_isHeaderFooterOpened || m_ps->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge::region", which);
#if TEST_SEND_HEADERFOOTER_REGION
  m_documentInterface->openHeader(propList);
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  m_documentInterface->closeHeader();
#else
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
#endif
  return true;
}

bool STOFFSpreadsheetListener::closeHeader()
{
  if (!m_ps->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeHeader: Oops no opened header/footer\n"));
    return false;
  }
  _popParsingState();
  m_documentInterface->closeHeader();
  return true;
}

bool STOFFSpreadsheetListener::openFooter(librevenge::RVNGPropertyList const &extras)
{
  if (m_ps->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFooter: Oops a header/footer is already opened\n"));
    return false;
  }
  _pushParsingState();
  m_ps->m_isHeaderFooterOpened=true;
  m_documentInterface->openFooter(extras);
  return true;
}

bool STOFFSpreadsheetListener::insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which)
{
  if (!m_ps->m_isHeaderFooterOpened || m_ps->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertFooterRegion: Oops can not open a new region\n"));
    return false;
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge::region", which);
#if TEST_SEND_HEADERFOOTER_REGION
  m_documentInterface->openFooter(propList);
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  m_documentInterface->closeFooter();
#else
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
#endif
  return true;
}

bool STOFFSpreadsheetListener::closeFooter()
{
  if (!m_ps->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeFooter: Oops no opened header/footer\n"));
    return false;
  }
  _popParsingState();
  m_documentInterface->closeFooter();
  return true;
}

///////////////////
// section
///////////////////
STOFFSection const &STOFFSpreadsheetListener::getSection() const
{
  STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::getSection: make no sense\n"));
  return m_ds->m_section;
}

bool STOFFSpreadsheetListener::openSection(STOFFSection const &)
{
  STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSection: make no sense\n"));
  return false;
}

bool STOFFSpreadsheetListener::closeSection()
{
  STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeSection: make no sense\n"));
  return false;
}

///////////////////
// paragraph
///////////////////
void STOFFSpreadsheetListener::_openParagraph()
{
  if (!m_ps->canWriteText())
    return;

  if (m_ps->m_isParagraphOpened || m_ps->m_isListElementOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::_openParagraph: a paragraph (or a list) is already opened"));
    return;
  }

  librevenge::RVNGPropertyList propList;
  m_ps->m_paragraph.addTo(propList);
  if (!m_ps->m_isParagraphOpened)
    m_documentInterface->openParagraph(propList);

  _resetParagraphState();
}

void STOFFSpreadsheetListener::_closeParagraph()
{
  // we can not close a paragraph in a link
  if (m_ps->m_inLink)
    return;
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

void STOFFSpreadsheetListener::_resetParagraphState(const bool isListElement)
{
  m_ps->m_isListElementOpened = isListElement;
  m_ps->m_isParagraphOpened = true;
  m_ps->m_isHeaderFooterWithoutParagraph = false;
}

///////////////////
// list
///////////////////
void STOFFSpreadsheetListener::_openListElement()
{
  if (!m_ps->canWriteText())
    return;

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

void STOFFSpreadsheetListener::_closeListElement()
{
  if (m_ps->m_isListElementOpened) {
    if (m_ps->m_isSpanOpened)
      _closeSpan();

    if (m_ps->m_list) m_ps->m_list->closeElement();
    m_documentInterface->closeListElement();
  }

  m_ps->m_isListElementOpened = m_ps->m_isParagraphOpened = false;
}

int STOFFSpreadsheetListener::_getListId() const
{
  auto newLevel= size_t(m_ps->m_paragraph.m_listLevelIndex);
  if (newLevel == 0) return -1;
  int newListId = m_ps->m_paragraph.m_listId;
  if (newListId > 0) return newListId;
  static bool first = true;
  if (first) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::_getListId: the list id is not set, try to find a new one\n"));
    first = false;
  }
  auto list=m_listManager->getNewList(m_ps->m_list, int(newLevel), m_ps->m_paragraph.m_listLevel);
  if (!list) return -1;
  return list->getId();
}

void STOFFSpreadsheetListener::_changeList()
{
  if (!m_ps->canWriteText())
    return;

  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  size_t actualLevel = m_ps->m_listOrderedLevels.size();
  auto newLevel= size_t(m_ps->m_paragraph.m_listLevelIndex);
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
    std::shared_ptr<STOFFList> theList;

    theList=m_listManager->getList(newListId);
    if (!theList) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::_changeList: can not find any list\n"));
      m_ps->m_listOrderedLevels.resize(actualLevel);
      return;
    }
    m_listManager->needToSend(newListId, m_ds->m_sentListMarkers);
    m_ps->m_list = theList;
    m_ps->m_list->setLevel(int(newLevel));
  }

  m_ps->m_listOrderedLevels.resize(newLevel, false);
  if (actualLevel == newLevel) return;

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
void STOFFSpreadsheetListener::_openSpan()
{
  if (m_ps->m_isSpanOpened || !m_ps->canWriteText())
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

  m_documentInterface->openSpan(propList);

  m_ps->m_isSpanOpened = true;
}

void STOFFSpreadsheetListener::_closeSpan()
{
  // better not to close a link...
  if (!m_ps->m_isSpanOpened)
    return;

  _flushText();
  m_documentInterface->closeSpan();
  m_ps->m_isSpanOpened = false;
}

///////////////////
// text (send data)
///////////////////
void STOFFSpreadsheetListener::_flushDeferredTabs()
{
  if (m_ps->m_numDeferredTabs == 0 || !m_ps->canWriteText())
    return;
  if (!m_ps->m_isSpanOpened) _openSpan();
  for (; m_ps->m_numDeferredTabs > 0; m_ps->m_numDeferredTabs--)
    m_documentInterface->insertTab();
  return;
}

void STOFFSpreadsheetListener::_flushText()
{
  if (m_ps->m_textBuffer.len() == 0  || !m_ps->canWriteText()) return;

  // when some many ' ' follows each other, call insertSpace
  librevenge::RVNGString tmpText;
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
// Note/Comment/picture/textbox
///////////////////
void STOFFSpreadsheetListener::insertNote(STOFFNote const &note, STOFFSubDocumentPtr &subDocument)
{
  if (m_ps->m_isNote) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertNote try to insert a note recursively (ignored)\n"));
    return;
  }
  if (!canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertNote called outside a text zone (ignored)\n"));
    return;
  }
  m_ps->m_isNote = true;
  if (m_ps->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertNote try to insert a note in a header/footer\n"));
    /** Must not happen excepted in corrupted document, so we do the minimum.
    	Note that we have no choice, either we begin by closing the paragraph,
    	... or we reprogram handleSubDocument.
    */
    if (m_ps->m_isParagraphOpened)
      _closeParagraph();
    int prevListLevel = m_ps->m_paragraph.m_listLevelIndex;
    m_ps->m_paragraph.m_listLevelIndex = 0;
    _changeList(); // flush the list exterior
    handleSubDocument(subDocument, libstoff::DOC_NOTE);
    m_ps->m_paragraph.m_listLevelIndex = prevListLevel;
  }
  else {
    if (!m_ps->m_isParagraphOpened)
      _openParagraph();
    else {
      _flushText();
      _closeSpan();
    }

    librevenge::RVNGPropertyList propList;
    if (note.m_label.len())
      propList.insert("text:label", librevenge::RVNGPropertyFactory::newStringProp(note.m_label));
    if (note.m_type == STOFFNote::FootNote) {
      if (note.m_number >= 0)
        m_ds->m_footNoteNumber = note.m_number;
      else
        m_ds->m_footNoteNumber++;
      propList.insert("librevenge:number", m_ds->m_footNoteNumber);
      m_documentInterface->openFootnote(propList);
      handleSubDocument(subDocument, libstoff::DOC_NOTE);
      m_documentInterface->closeFootnote();
    }
    else {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertNote try to insert a unexpected note\n"));
    }
  }
  m_ps->m_isNote = false;
}

void STOFFSpreadsheetListener::insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &creator, librevenge::RVNGString const &date)
{
  if (m_ps->m_isNote) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertComment try to insert a comment in a note (ignored)\n"));
    return;
  }
  if (!canWriteText()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertComment called outside a text zone (ignored)\n"));
    return;
  }

  if (!m_ps->m_isSheetCellOpened) {
    if (!m_ps->m_isParagraphOpened)
      _openParagraph();
    else {
      _flushText();
      _closeSpan();
    }
  }
  else if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  librevenge::RVNGPropertyList propList;
  if (!creator.empty())  propList.insert("dc:creator", creator);
  if (!date.empty())  propList.insert("meta:date-string", date);
  m_documentInterface->openComment(propList);

  m_ps->m_isNote = true;
  handleSubDocument(subDocument, libstoff::DOC_COMMENT_ANNOTATION);

  m_documentInterface->closeComment();
  m_ps->m_isNote = false;
}

void STOFFSpreadsheetListener::insertTextBox
(STOFFFrameStyle const &frame, STOFFSubDocumentPtr subDocument, STOFFGraphicStyle const &frameStyle)
{
  if (!m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertTextBox insert a textbox outside a sheet is not implemented\n"));
    return;
  }
  if (!openFrame(frame, frameStyle)) return;

  librevenge::RVNGPropertyList propList;
  if (frameStyle.m_propertyList["librevenge:next-frame-name"])
    propList.insert("librevenge:next-frame-name",frameStyle.m_propertyList["librevenge:next-frame-name"]->clone());
  STOFFGraphicStyle::checkForPadding(propList);
  m_documentInterface->openTextBox(propList);
  handleSubDocument(subDocument, libstoff::DOC_TEXT_BOX);
  m_documentInterface->closeTextBox();

  closeFrame();
}

void STOFFSpreadsheetListener::insertPicture(STOFFFrameStyle const &frame, STOFFEmbeddedObject const &picture, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertPicture insert a picture outside a sheet is not implemented\n"));
    return;
  }
  if (!openFrame(frame, style)) return;

  librevenge::RVNGPropertyList propList;
  if (picture.addTo(propList))
    m_documentInterface->insertBinaryObject(propList);

  closeFrame();
}

void STOFFSpreadsheetListener::insertEquation(STOFFFrameStyle const &frame, librevenge::RVNGString const &equation,
    STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertEquation insert an equation outside a sheet is not implemented\n"));
    return;
  }
  if (equation.empty()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertEquation: oops the equation is empty\n"));
    return;
  }
  if (!openFrame(frame, style)) return;

  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:mime-type", "application/mathml+xml");
  propList.insert("librevenge:data", equation);
  m_documentInterface->insertEquation(propList);
  closeFrame();
}

void STOFFSpreadsheetListener::insertShape(STOFFFrameStyle const &frame, STOFFGraphicShape const &shape, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertShape insert a picture outside a sheet is not implemented\n"));
    return;
  }
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
  else if (shape.m_propertyList["table:end-cell-address"] && !m_ds->m_isSheetRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertShape: must be called inside a cell\n"));
  }

  librevenge::RVNGPropertyList shapeProp, styleProp;
  frame.addTo(shapeProp);
  shape.addTo(shapeProp);
  style.addTo(styleProp);
  STOFFGraphicStyle::checkForDefault(styleProp);

  m_documentInterface->defineGraphicStyle(styleProp);
  switch (shape.m_command) {
  case STOFFGraphicShape::C_Connector:
    m_documentInterface->drawConnector(shapeProp);
    break;
  case STOFFGraphicShape::C_Ellipse:
    m_documentInterface->drawEllipse(shapeProp);
    break;
  case STOFFGraphicShape::C_Path:
    m_documentInterface->drawPath(shapeProp);
    break;
  case STOFFGraphicShape::C_Polyline:
    m_documentInterface->drawPolyline(shapeProp);
    break;
  case STOFFGraphicShape::C_Polygon:
    m_documentInterface->drawPolygon(shapeProp);
    break;
  case STOFFGraphicShape::C_Rectangle:
    m_documentInterface->drawRectangle(shapeProp);
    break;
  case STOFFGraphicShape::C_Unknown:
    break;
  default:
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertShape: unexpected shape\n"));
    break;
  }
}
///////////////////
// frame
///////////////////
bool STOFFSpreadsheetListener::openGroup(STOFFFrameStyle const &/*frame*/)
{
  STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openGroup is not implemented\n"));
  return false;
}

void STOFFSpreadsheetListener::closeGroup()
{
}

bool STOFFSpreadsheetListener::openFrame(STOFFFrameStyle const &frame, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isSheetOpened || m_ds->m_isSheetRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame insert a frame outside a sheet is not implemented\n"));
    return false;
  }
  if (m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: called but a frame is already opened\n"));
    return false;
  }
  auto finalFrame=frame;
  switch (finalFrame.m_position.m_anchorTo) {
  case STOFFPosition::Page:
    break;
  case STOFFPosition::Paragraph:
    if (m_ps->m_isParagraphOpened)
      _flushText();
    else
      _openParagraph();
    break;
  case STOFFPosition::Unknown:
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: UNKNOWN position, insert as char position\n"));
    STOFF_FALLTHROUGH;
  case STOFFPosition::CharBaseLine:
  case STOFFPosition::Char:
    if (m_ps->m_isSpanOpened)
      _flushText();
    else
      _openSpan();
    break;
  case STOFFPosition::Frame:
    if (!m_ds->m_subDocuments.size()) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: can not determine the frame\n"));
      return false;
    }
    if (m_ps->m_subDocumentType==libstoff::DOC_HEADER_FOOTER_REGION) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: called with Frame position in header footer, switch to paragraph\n"));
      if (m_ps->m_isParagraphOpened)
        _flushText();
      else
        _openParagraph();
      finalFrame.m_position.m_anchorTo=STOFFPosition::Paragraph;
    }
    break;
  case STOFFPosition::Cell:
    if (!m_ps->m_isSheetCellOpened) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: called with Cell position not in a sheet cell\n"));
      return false;
    }
    if (!finalFrame.m_position.m_propertyList["table:end-cell-address"]) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: can not find the cell name\n"));
      return false;
    }
    break;
  default:
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openFrame: can not determine the anchor\n"));
    return false;
  }

  librevenge::RVNGPropertyList propList(style.m_propertyList);
  if (!propList["draw:fill"])
    propList.insert("draw:fill","none");
  _handleFrameParameters(propList, finalFrame);
  m_documentInterface->openFrame(propList);

  m_ps->m_isFrameOpened = true;
  return true;
}

void STOFFSpreadsheetListener::closeFrame()
{
  if (!m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeFrame: called but no frame is already opened\n"));
    return;
  }
  m_documentInterface->closeFrame();
  m_ps->m_isFrameOpened = false;
}

void STOFFSpreadsheetListener::_handleFrameParameters
(librevenge::RVNGPropertyList &propList, STOFFFrameStyle const &frame)
{
  frame.addTo(propList);
  if (propList["text:anchor-page-number"])
    propList.remove("text:anchor-page-number");
}

///////////////////
// subdocument
///////////////////
void STOFFSpreadsheetListener::handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType)
{
  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = subDocumentType;

  m_ps->m_isPageSpanOpened = true;
  m_ps->m_list.reset();

  switch (subDocumentType) {
  case libstoff::DOC_TEXT_BOX:
    m_ps->m_isTextboxOpened = true;
    break;
  case libstoff::DOC_HEADER_FOOTER_REGION:
    m_ps->m_isHeaderFooterRegionOpened=true;
    m_ps->m_isHeaderFooterWithoutParagraph = true;
    break;
  case libstoff::DOC_CHART_ZONE:
    m_ps->m_isTextboxOpened = true;
    break;
  case libstoff::DOC_NONE:
  case libstoff::DOC_CHART:
  case libstoff::DOC_NOTE:
  case libstoff::DOC_SHEET:
  case libstoff::DOC_TABLE:
  case libstoff::DOC_COMMENT_ANNOTATION:
  case libstoff::DOC_GRAPHIC_GROUP:
  default:
    break;
  }

  // Check whether the document is calling itself
  bool sendDoc = true;
  for (auto const &subDoc : m_ds->m_subDocuments) {
    if (!subDocument)
      break;
    if (!subDoc) continue;
    if (*subDocument == *subDoc) {
      STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::handleSubDocument: recursif call, stop...\n"));
      sendDoc = false;
      break;
    }
  }
  if (sendDoc) {
    if (subDocument) {
      m_ds->m_subDocuments.push_back(subDocument);
      std::shared_ptr<STOFFListener> listen(this, STOFF_shared_ptr_noop_deleter<STOFFSpreadsheetListener>());
      try {
        subDocument->parse(listen, subDocumentType);
      }
      catch (...) {
        STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::handleSubDocument exception catched \n"));
      }
      m_ds->m_subDocuments.pop_back();
    }
    if (m_ps->m_isHeaderFooterWithoutParagraph)
      _openSpan();
  }

  _endSubDocument();
  _popParsingState();
}

bool STOFFSpreadsheetListener::isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const
{
  if (!m_ps->m_inSubDocument)
    return false;
  subdocType = m_ps->m_subDocumentType;
  return true;
}

void STOFFSpreadsheetListener::_startSubDocument()
{
  m_ds->m_isDocumentStarted = true;
  m_ps->m_inSubDocument = true;
}

void STOFFSpreadsheetListener::_endSubDocument()
{
  if (m_ps->m_isTableOpened)
    closeTable();
  if (m_ps->m_isSpanOpened)
    _closeSpan();
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  m_ps->m_paragraph.m_listLevelIndex=0;
  _changeList(); // flush the list exterior
}

///////////////////
// sheet
///////////////////
void STOFFSpreadsheetListener::openSheet(std::vector<float> const &colWidth, librevenge::RVNGUnit unit, std::vector<int> const &repeatColWidthNumber, librevenge::RVNGString const &name)
{
  if (m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheet: called with m_isSheetOpened=true\n"));
    return;
  }
  if (!m_ps->m_isPageSpanOpened)
    _openPageSpan();
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = libstoff::DOC_SHEET;
  m_ps->m_isPageSpanOpened = true;

  librevenge::RVNGPropertyList propList;
  librevenge::RVNGPropertyListVector columns;
  size_t nCols = colWidth.size();
  bool useRepeated=repeatColWidthNumber.size()==nCols;
  if (!useRepeated&&!repeatColWidthNumber.empty()) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheet: repeatColWidthNumber seems bad\n"));
  }
  for (size_t c = 0; c < nCols; c++) {
    librevenge::RVNGPropertyList column;
    column.insert("style:column-width", double(colWidth[c]), unit);
    if (useRepeated && repeatColWidthNumber[c]>1)
      column.insert("table:number-columns-repeated", repeatColWidthNumber[c]);
    columns.append(column);
  }
  propList.insert("librevenge:columns", columns);
  if (!name.empty())
    propList.insert("librevenge:sheet-name", name);
  m_documentInterface->openSheet(propList);
  m_ds->m_isSheetOpened = true;
}

void STOFFSpreadsheetListener::closeSheet()
{
  if (!m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeSheet: called with m_isSheetOpened=false\n"));
    return;
  }

  m_ds->m_isSheetOpened = false;
  m_documentInterface->closeSheet();
  _endSubDocument();
  _popParsingState();
}

void STOFFSpreadsheetListener::openSheetRow(float h, librevenge::RVNGUnit unit, int numRepeated)
{
  if (m_ds->m_isSheetRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheetRow: called with m_isSheetRowOpened=true\n"));
    return;
  }
  if (!m_ds->m_isSheetOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheetRow: called with m_isSheetOpened=false\n"));
    return;
  }
  librevenge::RVNGPropertyList propList;
  if (h > 0)
    propList.insert("style:row-height", double(h), unit);
  else if (h < 0)
    propList.insert("style:min-row-height", double(-h), unit);
  if (numRepeated>1)
    propList.insert("table:number-rows-repeated", numRepeated);
  m_documentInterface->openSheetRow(propList);
  m_ds->m_isSheetRowOpened = true;
}

void STOFFSpreadsheetListener::closeSheetRow()
{
  if (!m_ds->m_isSheetRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheetRow: called with m_isSheetRowOpened=false\n"));
    return;
  }
  m_ds->m_isSheetRowOpened = false;
  m_documentInterface->closeSheetRow();
}

void STOFFSpreadsheetListener::openSheetCell(STOFFCell const &cell, STOFFCellContent const &content, int numRepeated)
{
  if (!m_ds->m_isSheetRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheetCell: called with m_isSheetRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isSheetCellOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openSheetCell: called with m_isSheetCellOpened=true\n"));
    closeSheetCell();
  }

  librevenge::RVNGPropertyList propList;
  cell.addTo(propList);
  if (numRepeated>1)
    propList.insert("table:number-columns-repeated", numRepeated);
  STOFFCell::Format const &format=cell.getFormat();
  if (!format.hasBasicFormat()) {
    int numberingId=-1;
    librevenge::RVNGPropertyList const &numberingStyle=cell.getNumberingStyle();
    librevenge::RVNGString hashKey = numberingStyle.getPropString();
    std::stringstream name;
    if (m_ds->m_numberingIdMap.find(hashKey)!=m_ds->m_numberingIdMap.end()) {
      numberingId=m_ds->m_numberingIdMap.find(hashKey)->second;
      name << "Numbering" << numberingId;
    }
    else if (!numberingStyle.empty()) {
      numberingId=int(m_ds->m_numberingIdMap.size());
      name << "Numbering" << numberingId;

      librevenge::RVNGPropertyList numList(numberingStyle);
      numList.insert("librevenge:name", name.str().c_str());
      m_documentInterface->defineSheetNumberingStyle(numList);
      m_ds->m_numberingIdMap[hashKey]=numberingId;
    }
    if (numberingId>=0)
      propList.insert("librevenge:numbering-name", name.str().c_str());
  }
  // formula
  if (content.m_formula.size()) {
    librevenge::RVNGPropertyListVector formulaVect;
    for (auto const &f : content.m_formula) {
      if (f.m_type!=STOFFCellContent::FormulaInstruction::F_None)
        formulaVect.append(f.getPropertyList());
    }
    propList.insert("librevenge:formula", formulaVect);
  }
  bool hasFormula=!content.m_formula.empty();
  if (content.isValueSet() || hasFormula) {
    bool hasValue=content.isValueSet();
    if (hasFormula && (content.m_value >= 0 && content.m_value <= 0))
      hasValue=false;
    switch (format.m_format) {
    case STOFFCell::F_TEXT:
      if (!hasValue) break;
      propList.insert("librevenge:value-type", format.getValueType().c_str());
      propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
      break;
    case STOFFCell::F_NUMBER:
      propList.insert("librevenge:value-type", format.getValueType().c_str());
      if (!hasValue) break;
      propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
      break;
    case STOFFCell::F_BOOLEAN:
      propList.insert("librevenge:value-type", "boolean");
      if (!hasValue) break;
      propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
      break;
    case STOFFCell::F_DATE:
    case STOFFCell::F_DATETIME: {
      propList.insert("librevenge:value-type", "date");
      if (!hasValue) break;
      int Y=0, M=0, D=0;
      if (!STOFFCellContent::double2Date(content.m_value, Y, M, D)) break;
      propList.insert("librevenge:year", Y);
      propList.insert("librevenge:month", M);
      propList.insert("librevenge:day", D);
      if (format.m_format==STOFFCell::F_DATE)
        break;
    }
    STOFF_FALLTHROUGH;
    case STOFFCell::F_TIME: {
      if (format.m_format==STOFFCell::F_TIME)
        propList.insert("librevenge:value-type", "time");
      if (!hasValue) break;
      int H=0, M=0, S=0;
      if (!STOFFCellContent::double2Time(std::fmod(content.m_value,1.),H,M,S))
        break;
      propList.insert("librevenge:hours", H);
      propList.insert("librevenge:minutes", M);
      propList.insert("librevenge:seconds", S);
      break;
    }
    case STOFFCell::F_UNKNOWN:
      if (!hasValue) break;
      propList.insert("librevenge:value-type", format.getValueType().c_str());
      propList.insert("librevenge:value", content.m_value, librevenge::RVNG_GENERIC);
      break;
    default:
      break;
    }
  }

  _pushParsingState();
  m_ps->m_isSheetCellOpened = true;
  m_documentInterface->openSheetCell(propList);
}

void STOFFSpreadsheetListener::closeSheetCell()
{
  if (!m_ps->m_isSheetCellOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeSheetCell: called with m_isSheetCellOpened=false\n"));
    return;
  }

  _closeParagraph();

  m_ps->m_isSheetCellOpened = false;
  m_documentInterface->closeSheetCell();
  _popParsingState();
}

///////////////////
// chart
///////////////////
void STOFFSpreadsheetListener::insertChart
(STOFFFrameStyle const &frame, STOFFChart &chart, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isSheetOpened || m_ds->m_isSheetRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertChart outside a chart in a sheet is not implemented\n"));
    return;
  }
  if (!openFrame(frame, style)) return;

  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = libstoff::DOC_CHART;

  std::shared_ptr<STOFFSpreadsheetListener> listen(this, STOFF_shared_ptr_noop_deleter<STOFFSpreadsheetListener>());
  try {
    chart.sendChart(listen, m_documentInterface);
  }
  catch (...) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::insertChart exception catched \n"));
  }
  _endSubDocument();
  _popParsingState();

  closeFrame();
}

void STOFFSpreadsheetListener::openTable(STOFFTable const &table)
{
  if (m_ps->m_isFrameOpened || m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openTable: no frame is already open...\n"));
    return;
  }

  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  // default value: which can be redefined by table
  librevenge::RVNGPropertyList propList;
  propList.insert("table:align", "left");
  if (m_ps->m_paragraph.m_propertyList["fo:margin-left"])
    propList.insert("fo:margin-left", m_ps->m_paragraph.m_propertyList["fo:margin-left"]->clone());

  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = libstoff::DOC_TABLE;

  table.addTablePropertiesTo(propList);
  m_documentInterface->openTable(propList);
  m_ps->m_isTableOpened = true;
}

void STOFFSpreadsheetListener::closeTable()
{
  if (!m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeTable: called with m_isTableOpened=false\n"));
    return;
  }

  m_ps->m_isTableOpened = false;
  _endSubDocument();
  m_documentInterface->closeTable();

  _popParsingState();
}

void STOFFSpreadsheetListener::openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow)
{
  if (m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openTableRow: called with m_isTableRowOpened=true\n"));
    return;
  }
  if (!m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openTableRow: called with m_isTableOpened=false\n"));
    return;
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:is-header-row", headerRow);

  if (h > 0)
    propList.insert("style:row-height", double(h), unit);
  else if (h < 0)
    propList.insert("style:min-row-height", double(-h), unit);
  m_documentInterface->openTableRow(propList);
  m_ps->m_isTableRowOpened = true;
}

void STOFFSpreadsheetListener::closeTableRow()
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openTableRow: called with m_isTableRowOpened=false\n"));
    return;
  }
  m_ps->m_isTableRowOpened = false;
  m_documentInterface->closeTableRow();
}

void STOFFSpreadsheetListener::addCoveredTableCell(STOFFVec2i const &pos)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::addCoveredTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::addCoveredTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:column", pos[0]);
  propList.insert("librevenge:row", pos[1]);
  m_documentInterface->insertCoveredTableCell(propList);
}

void STOFFSpreadsheetListener::addEmptyTableCell(STOFFVec2i const &pos, STOFFVec2i span)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::addEmptyTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::addEmptyTableCell: called with m_isTableCellOpened=true\n"));
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

void STOFFSpreadsheetListener::openTableCell(STOFFCell const &cell)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::openTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }

  librevenge::RVNGPropertyList propList;
  cell.addTo(propList);
  m_ps->m_isTableCellOpened = true;
  m_documentInterface->openTableCell(propList);
}

void STOFFSpreadsheetListener::closeTableCell()
{
  if (!m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::closeTableCell: called with m_isTableCellOpened=false\n"));
    return;
  }

  _closeParagraph();
  m_ps->m_paragraph.m_listLevelIndex=0;
  _changeList(); // flush the list exterior

  m_ps->m_isTableCellOpened = false;
  m_documentInterface->closeTableCell();
}

///////////////////
// others
///////////////////

// ---------- state stack ------------------
std::shared_ptr<STOFFSpreadsheetListenerInternal::State> STOFFSpreadsheetListener::_pushParsingState()
{
  auto actual = m_ps;
  m_psStack.push_back(actual);
  m_ps.reset(new STOFFSpreadsheetListenerInternal::State);

  m_ps->m_isNote = actual->m_isNote;

  return actual;
}

void STOFFSpreadsheetListener::_popParsingState()
{
  if (m_psStack.size()==0) {
    STOFF_DEBUG_MSG(("STOFFSpreadsheetListener::_popParsingState: psStack is empty()\n"));
    throw libstoff::ParseException();
  }
  m_ps = m_psStack.back();
  m_psStack.pop_back();
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
