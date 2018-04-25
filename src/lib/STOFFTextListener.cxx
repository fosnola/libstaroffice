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

/** \file STOFFTextListener.cxx
 * Implements STOFFTextListener: the libstoff word processor listener
 *
 * \note this class is the only class which does the interface with
 * the librevenge::RVNGTextInterface
 */

#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include <time.h>

#include <librevenge/librevenge.h>

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
#include "STOFFParser.hxx"
#include "STOFFPosition.hxx"
#include "STOFFSection.hxx"
#include "STOFFSubDocument.hxx"
#include "STOFFTable.hxx"

#include "STOFFTextListener.hxx"

//! Internal and low level namespace to define the states of STOFFTextListener
namespace STOFFTextListenerInternal
{
//! a enum to define basic break bit
enum { PageBreakBit=0x1, ColumnBreakBit=0x2 };
//! a class to store the document state of a STOFFTextListener
struct TextState {
  //! constructor
  explicit TextState(std::vector<STOFFPageSpan> const &pageList)
    : m_pageList(pageList)
    , m_pageSpan()
    , m_metaData()
    , m_footNoteNumber(0)
    , m_endNoteNumber(0)
    , m_smallPictureNumber(0)
    , m_isDocumentStarted(false)
    , m_isHeaderFooterOpened(false)
    , m_isHeaderFooterRegionOpened(false)
    , m_sentListMarkers()
    , m_subDocuments()

    , m_definedFontStyleSet()
    , m_definedGraphicStyleSet()
    , m_definedParagraphStyleSet()
  {
  }
  //! destructor
  ~TextState()
  {
  }

  //! the pages definition
  std::vector<STOFFPageSpan> m_pageList;
  //! the current page span
  STOFFPageSpan m_pageSpan;
  //! the document meta data
  librevenge::RVNGPropertyList m_metaData;

  int m_footNoteNumber /** footnote number*/, m_endNoteNumber /** endnote number*/;

  int m_smallPictureNumber /** number of small picture */;
  bool m_isDocumentStarted /** a flag to know if the document is open */, m_isHeaderFooterOpened /** a flag to know if the header footer is started */;
  bool m_isHeaderFooterRegionOpened /** a flag to know if the header footer region is started */;
  /// the list of marker corresponding to sent list
  std::vector<int> m_sentListMarkers;
  std::vector<STOFFSubDocumentPtr> m_subDocuments; /** list of document actually open */
  //! the set of defined font style
  std::set<librevenge::RVNGString> m_definedFontStyleSet;
  //! the set of defined graphic style
  std::set<librevenge::RVNGString> m_definedGraphicStyleSet;
  //! the set of defined paragraph style
  std::set<librevenge::RVNGString> m_definedParagraphStyleSet;

private:
  TextState(const TextState &);
  TextState &operator=(const TextState &);
};

/** the state of a STOFFTextListener */
struct State {
  //! constructor
  State();
  //! destructor
  ~State() { }

  //! a buffer to stored the text
  librevenge::RVNGString m_textBuffer;
  //! the number of tabs to add
  int m_numDeferredTabs;

  //! the font
  STOFFFont m_font;
  //! the paragraph
  STOFFParagraph m_paragraph;
  //! a sequence of bit used to know if we need page/column break
  int m_paragraphNeedBreak;

  std::shared_ptr<STOFFList> m_list;

  bool m_isPageSpanOpened;
  bool m_isSectionOpened;
  bool m_isFrameOpened;
  bool m_isPageSpanBreakDeferred;
  bool m_isHeaderFooterWithoutParagraph;
  //! a flag to know if openGroup was called
  bool m_isGroupOpened;

  bool m_isSpanOpened;
  bool m_isParagraphOpened;
  bool m_isListElementOpened;

  bool m_firstParagraphInPageSpan;

  bool m_isTableOpened;
  bool m_isTableRowOpened;
  bool m_isTableColumnOpened;
  bool m_isTableCellOpened;

  unsigned m_currentPage;
  int m_numPagesRemainingInSpan;
  int m_currentPageNumber;

  bool m_sectionAttributesChanged;
  //! the section
  STOFFSection m_section;

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
  , m_paragraphNeedBreak(0)

  , m_list()

  , m_isPageSpanOpened(false)
  , m_isSectionOpened(false)
  , m_isFrameOpened(false)
  , m_isPageSpanBreakDeferred(false)
  , m_isHeaderFooterWithoutParagraph(false)
  , m_isGroupOpened(false)

  , m_isSpanOpened(false)
  , m_isParagraphOpened(false)
  , m_isListElementOpened(false)

  , m_firstParagraphInPageSpan(true)

  , m_isTableOpened(false)
  , m_isTableRowOpened(false)
  , m_isTableColumnOpened(false)
  , m_isTableCellOpened(false)

  , m_currentPage(0)
  , m_numPagesRemainingInSpan(0)
  , m_currentPageNumber(1)

  , m_sectionAttributesChanged(false)
  , m_section()

  , m_listOrderedLevels()

  , m_inSubDocument(false)
  , m_isNote(false)
  , m_inLink(false)
  , m_subDocumentType(libstoff::DOC_NONE)
{
}
}

STOFFTextListener::STOFFTextListener(STOFFListManagerPtr &listManager, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGTextInterface *documentInterface)
  : STOFFListener(listManager)
  , m_ds(new STOFFTextListenerInternal::TextState(pageList))
  , m_ps(new STOFFTextListenerInternal::State)
  , m_psStack()
  , m_documentInterface(documentInterface)
{
}

STOFFTextListener::~STOFFTextListener()
{
}

void STOFFTextListener::defineStyle(STOFFFont const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedFontStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  librevenge::RVNGPropertyList pList(style.m_propertyList);
  STOFFFont::checkForDefault(pList);
  m_documentInterface->defineCharacterStyle(pList);
}

void STOFFTextListener::defineStyle(STOFFGraphicStyle const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedGraphicStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  librevenge::RVNGPropertyList pList(style.m_propertyList);
  STOFFGraphicStyle::checkForDefault(pList);
  STOFFGraphicStyle::checkForPadding(pList);
  m_documentInterface->defineGraphicStyle(pList);
}

void STOFFTextListener::defineStyle(STOFFParagraph const &style)
{
  if (style.m_propertyList["style:display-name"])
    m_ds->m_definedParagraphStyleSet.insert(style.m_propertyList["style:display-name"]->getStr());
  m_documentInterface->defineParagraphStyle(style.m_propertyList);
}

bool STOFFTextListener::isFontStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedFontStyleSet.find(name)!=m_ds->m_definedFontStyleSet.end();
}

bool STOFFTextListener::isGraphicStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedGraphicStyleSet.find(name)!=m_ds->m_definedGraphicStyleSet.end();
}

bool STOFFTextListener::isParagraphStyleDefined(librevenge::RVNGString const &name) const
{
  return m_ds->m_definedParagraphStyleSet.find(name)!=m_ds->m_definedParagraphStyleSet.end();
}

///////////////////
// text data
///////////////////
void STOFFTextListener::insertChar(uint8_t character)
{
  if (character >= 0x80) {
    STOFFTextListener::insertUnicode(character);
    return;
  }
  _flushDeferredTabs();
  if (!m_ps->m_isSpanOpened) _openSpan();
  m_ps->m_textBuffer.append(char(character));
}

void STOFFTextListener::insertUnicode(uint32_t val)
{
  // undef character, we skip it
  if (val == 0xfffd) return;
  if (val<0x20 && val!=0x9 && val!=0xa && val!=0xd) {
    static int numErrors=0;
    if (++numErrors<10) {
      STOFF_DEBUG_MSG(("STOFFTextListener::insertUnicode: find odd char %x\n", static_cast<unsigned int>(val)));
    }
    return;
  }

  _flushDeferredTabs();
  if (!m_ps->m_isSpanOpened) _openSpan();
  libstoff::appendUnicode(val, m_ps->m_textBuffer);
}

void STOFFTextListener::insertUnicodeString(librevenge::RVNGString const &str)
{
  _flushDeferredTabs();
  if (!m_ps->m_isSpanOpened) _openSpan();
  m_ps->m_textBuffer.append(str);
}

void STOFFTextListener::insertEOL(bool soft)
{
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

void STOFFTextListener::insertTab()
{
  if (!m_ps->m_isParagraphOpened) {
    m_ps->m_numDeferredTabs++;
    return;
  }
  if (m_ps->m_isSpanOpened) _flushText();
  m_ps->m_numDeferredTabs++;
  _flushDeferredTabs();
}

void STOFFTextListener::insertBreak(STOFFTextListener::BreakType breakType)
{
  switch (breakType) {
  case ColumnBreak:
    if (!m_ps->m_isPageSpanOpened && !m_ps->m_inSubDocument)
      _openSpan();
    if (m_ps->m_isParagraphOpened)
      _closeParagraph();
    m_ps->m_paragraphNeedBreak |= STOFFTextListenerInternal::ColumnBreakBit;
    break;
  case PageBreak:
    if (!m_ps->m_isPageSpanOpened && !m_ps->m_inSubDocument)
      _openSpan();
    if (m_ps->m_isParagraphOpened)
      _closeParagraph();
    m_ps->m_paragraphNeedBreak |= STOFFTextListenerInternal::PageBreakBit;
    break;
  case SoftPageBreak:
#if !defined(__clang__)
  default:
#endif
    break;
  }

  if (m_ps->m_inSubDocument)
    return;

  switch (breakType) {
  case PageBreak:
  case SoftPageBreak:
    if (m_ps->m_numPagesRemainingInSpan > 0)
      m_ps->m_numPagesRemainingInSpan--;
    else {
      if (!m_ps->m_isTableOpened && !m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened)
        _closePageSpan();
      else
        m_ps->m_isPageSpanBreakDeferred = true;
    }
    m_ps->m_currentPageNumber++;
    break;
  case ColumnBreak:
#if !defined(__clang__)
  default:
#endif
    break;
  }
}

void STOFFTextListener::_insertBreakIfNecessary(librevenge::RVNGPropertyList &propList)
{
  if (!m_ps->m_paragraphNeedBreak)
    return;

  if ((m_ps->m_paragraphNeedBreak&STOFFTextListenerInternal::PageBreakBit) ||
      m_ps->m_section.numColumns() <= 1) {
    if (m_ps->m_inSubDocument) {
      STOFF_DEBUG_MSG(("STOFFTextListener::_insertBreakIfNecessary: can not add page break in subdocument\n"));
    }
    else
      propList.insert("fo:break-before", "page");
  }
  else if (m_ps->m_paragraphNeedBreak&STOFFTextListenerInternal::ColumnBreakBit)
    propList.insert("fo:break-before", "column");
  m_ps->m_paragraphNeedBreak=0;
}

///////////////////
// font/paragraph function
///////////////////
void STOFFTextListener::setFont(STOFFFont const &font)
{
  if (font == m_ps->m_font) return;

  _closeSpan();
  m_ps->m_font = font;
}

STOFFFont const &STOFFTextListener::getFont() const
{
  return m_ps->m_font;
}

bool STOFFTextListener::isParagraphOpened() const
{
  return m_ps->m_isParagraphOpened;
}

void STOFFTextListener::setParagraph(STOFFParagraph const &para)
{
  if (para==m_ps->m_paragraph) return;

  m_ps->m_paragraph=para;
  if (m_ps->m_paragraph.m_listLevelIndex>20) {
    STOFF_DEBUG_MSG(("STOFFTextListener::setParagraph: the level index seems bad, resets it to 10\n"));
    m_ps->m_paragraph.m_listLevelIndex=10;
  }
}

STOFFParagraph const &STOFFTextListener::getParagraph() const
{
  return m_ps->m_paragraph;
}

///////////////////
// field/link :
///////////////////
void STOFFTextListener::insertField(STOFFField const &field)
{
  librevenge::RVNGPropertyList propList;
  field.addTo(propList);
  _flushDeferredTabs();
  _flushText();
  _openSpan();
  m_documentInterface->insertField(propList);
}

void STOFFTextListener::openLink(STOFFLink const &link)
{
  if (m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openLink: a link is already opened\n"));
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

void STOFFTextListener::closeLink()
{
  if (!m_ps->m_inLink) {
    STOFF_DEBUG_MSG(("STOFFTextListener:closeLink: can not close a link\n"));
    return;
  }
  if (m_ps->m_isSpanOpened) _closeSpan();
  m_documentInterface->closeLink();
  _popParsingState();
}

///////////////////
// document
///////////////////
void STOFFTextListener::setDocumentMetaData(librevenge::RVNGPropertyList const &meta)
{
  librevenge::RVNGPropertyList::Iter i(meta);
  for (i.rewind(); i.next();)
    m_ds->m_metaData.insert(i.key(), i()->getStr());
}

void STOFFTextListener::setDocumentLanguage(std::string locale)
{
  if (!locale.length()) return;
  m_ds->m_metaData.insert("librevenge:language", locale.c_str());
}

bool STOFFTextListener::isDocumentStarted() const
{
  return m_ds->m_isDocumentStarted;
}

void STOFFTextListener::startDocument()
{
  if (m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFTextListener::startDocument: the document is already started\n"));
    return;
  }

  m_documentInterface->startDocument(librevenge::RVNGPropertyList());
  m_ds->m_isDocumentStarted = true;

  m_documentInterface->setDocumentMetaData(m_ds->m_metaData);
}

void STOFFTextListener::endDocument(bool sendDelayedSubDoc)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFTextListener::endDocument: the document is not started\n"));
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
  _closeSection();
  _closePageSpan();
  m_documentInterface->endDocument();
  m_ds->m_isDocumentStarted = false;
}

///////////////////
// page
///////////////////
bool STOFFTextListener::isPageSpanOpened() const
{
  return m_ps->m_isPageSpanOpened;
}

STOFFPageSpan const &STOFFTextListener::getPageSpan()
{
  if (!m_ps->m_isPageSpanOpened)
    _openPageSpan();
  return m_ds->m_pageSpan;
}


void STOFFTextListener::_openPageSpan(bool sendHeaderFooters)
{
  if (m_ps->m_isPageSpanOpened)
    return;

  if (!m_ds->m_isDocumentStarted)
    startDocument();

  if (m_ds->m_pageList.size()==0) {
    STOFF_DEBUG_MSG(("STOFFTextListener::_openPageSpan: can not find any page\n"));
    throw libstoff::ParseException();
  }
  unsigned actPage = 0;
  auto it = m_ds->m_pageList.begin();
  ++m_ps->m_currentPage;
  while (true) {
    actPage+=static_cast<unsigned>(it->m_pageSpan);
    if (actPage>=m_ps->m_currentPage) break;
    if (++it == m_ds->m_pageList.end()) {
      STOFF_DEBUG_MSG(("STOFFTextListener::_openPageSpan: can not find current page, use last page\n"));
      --it;
      break;
    }
  }
  auto &currentPage = *it;

  librevenge::RVNGPropertyList propList;
  currentPage.getPageProperty(propList);
  propList.insert("librevenge:is-last-page-span", ++it == m_ds->m_pageList.end());

  if (!m_ps->m_isPageSpanOpened)
    m_documentInterface->openPageSpan(propList);

  m_ps->m_isPageSpanOpened = true;
  m_ds->m_pageSpan = currentPage;

  // we insert the header footer
  if (sendHeaderFooters)
    currentPage.sendHeaderFooters(this);

  // first paragraph in span (necessary for resetting page number)
  m_ps->m_firstParagraphInPageSpan = true;
  m_ps->m_numPagesRemainingInSpan = (currentPage.m_pageSpan - 1);

  // finally update the section
  m_ps->m_section=currentPage.m_section;
}

void STOFFTextListener::_closePageSpan()
{
  if (!m_ps->m_isPageSpanOpened)
    return;

  if (m_ps->m_isSectionOpened)
    _closeSection();

  m_documentInterface->closePageSpan();
  m_ps->m_isPageSpanOpened = m_ps->m_isPageSpanBreakDeferred = false;
}

///////////////////
// header/footer
///////////////////
bool STOFFTextListener::isHeaderFooterOpened() const
{
  return m_ds->m_isHeaderFooterOpened;
}

bool STOFFTextListener::openHeader(librevenge::RVNGPropertyList const &extras)
{
  if (m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openHeader: Oops a header/footer is already opened\n"));
    return false;
  }
  m_ds->m_isHeaderFooterOpened=true;
  librevenge::RVNGPropertyList propList(extras);
  m_documentInterface->openHeader(propList);
  return true;
}

bool STOFFTextListener::insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &/*which*/)
{
  if (!m_ds->m_isHeaderFooterOpened || m_ds->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  m_ds->m_isHeaderFooterRegionOpened=true;
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  m_ds->m_isHeaderFooterRegionOpened=false;
  return true;
}

bool STOFFTextListener::closeHeader()
{
  if (!m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeHeader: Oops no opened header/footer\n"));
    return false;
  }
  m_documentInterface->closeHeader();
  m_ds->m_isHeaderFooterOpened=false;
  return true;
}

bool STOFFTextListener::openFooter(librevenge::RVNGPropertyList const &extras)
{
  if (m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openFooter: Oops a header/footer is already opened\n"));
    return false;
  }
  m_ds->m_isHeaderFooterOpened=true;
  librevenge::RVNGPropertyList propList(extras);
  m_documentInterface->openFooter(propList);
  return true;
}

bool STOFFTextListener::insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &/*which*/)
{
  if (!m_ds->m_isHeaderFooterOpened || m_ds->m_isHeaderFooterRegionOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::insertHeaderRegion: Oops can not open a new region\n"));
    return false;
  }
  m_ds->m_isHeaderFooterRegionOpened=true;
  handleSubDocument(subDocument, libstoff::DOC_HEADER_FOOTER_REGION);
  m_ds->m_isHeaderFooterRegionOpened=false;
  return true;
}

bool STOFFTextListener::closeFooter()
{
  if (!m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeFooter: Oops no opened header/footer\n"));
    return false;
  }
  m_documentInterface->closeFooter();
  m_ds->m_isHeaderFooterOpened=false;
  return true;
}

///////////////////
// section
///////////////////
bool STOFFTextListener::isSectionOpened() const
{
  return m_ps->m_isSectionOpened;
}

STOFFSection const &STOFFTextListener::getSection() const
{
  return m_ps->m_section;
}

bool STOFFTextListener::canOpenSectionAddBreak() const
{
  return !m_ps->m_isTableOpened && (!m_ps->m_inSubDocument || m_ps->m_subDocumentType == libstoff::DOC_TEXT_BOX);
}

bool STOFFTextListener::openSection(STOFFSection const &section)
{
  if (m_ps->m_isSectionOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openSection: a section is already opened\n"));
    return false;
  }

  if (m_ps->m_isTableOpened || (m_ps->m_inSubDocument && m_ps->m_subDocumentType != libstoff::DOC_TEXT_BOX)) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openSection: impossible to open a section\n"));
    return false;
  }
  m_ps->m_section=section;
  _openSection();
  return true;
}

bool STOFFTextListener::closeSection()
{
  if (!m_ps->m_isSectionOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeSection: no section are already opened\n"));
    return false;
  }

  if (m_ps->m_isTableOpened || (m_ps->m_inSubDocument && m_ps->m_subDocumentType != libstoff::DOC_TEXT_BOX)) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeSection: impossible to close a section\n"));
    return false;
  }
  _closeSection();
  return true;
}

void STOFFTextListener::_openSection()
{
  if (m_ps->m_isSectionOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::_openSection: a section is already opened\n"));
    return;
  }

  if (!m_ps->m_isPageSpanOpened)
    _openPageSpan();

  librevenge::RVNGPropertyList propList;
  m_ps->m_section.addTo(propList);
  m_documentInterface->openSection(propList);

  m_ps->m_sectionAttributesChanged = false;
  m_ps->m_isSectionOpened = true;
}

void STOFFTextListener::_closeSection()
{
  if (!m_ps->m_isSectionOpened ||m_ps->m_isTableOpened)
    return;

  if (m_ps->m_isParagraphOpened)
    _closeParagraph();
  m_ps->m_paragraph.m_listLevelIndex=0;
  _changeList();

  m_documentInterface->closeSection();

  m_ps->m_section = STOFFSection();
  m_ps->m_sectionAttributesChanged = false;
  m_ps->m_isSectionOpened = false;
}

///////////////////
// paragraph
///////////////////
void STOFFTextListener::_openParagraph()
{
  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
    return;

  if (m_ps->m_isParagraphOpened || m_ps->m_isListElementOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::_openParagraph: a paragraph (or a list) is already opened"));
    return;
  }
  if (!m_ps->m_isTableOpened && (!m_ps->m_inSubDocument || m_ps->m_subDocumentType == libstoff::DOC_TEXT_BOX)) {
    if (m_ps->m_sectionAttributesChanged)
      _closeSection();

    if (!m_ps->m_isSectionOpened)
      _openSection();
  }

  librevenge::RVNGPropertyList propList;
  _appendParagraphProperties(propList);
  if (m_ps->m_paragraph.m_outline && m_ps->m_paragraph.m_listLevelIndex>0)
    propList.insert("text:outline-level", m_ps->m_paragraph.m_listLevelIndex);
  if (!m_ps->m_isParagraphOpened)
    m_documentInterface->openParagraph(propList);

  _resetParagraphState();
  m_ps->m_firstParagraphInPageSpan = false;
}

void STOFFTextListener::_closeParagraph()
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

  if (!m_ps->m_isTableOpened && m_ps->m_isPageSpanBreakDeferred && !m_ps->m_inSubDocument)
    _closePageSpan();
}

void STOFFTextListener::_resetParagraphState(const bool isListElement)
{
  m_ps->m_paragraphNeedBreak = 0;
  m_ps->m_isListElementOpened = isListElement;
  m_ps->m_isParagraphOpened = true;
  m_ps->m_isHeaderFooterWithoutParagraph = false;
}

void STOFFTextListener::_appendParagraphProperties(librevenge::RVNGPropertyList &propList, const bool /*isListElement*/)
{
  m_ps->m_paragraph.addTo(propList);

  if (!m_ps->m_inSubDocument && m_ps->m_firstParagraphInPageSpan && m_ds->m_pageSpan.m_pageNumber >= 0)
    propList.insert("style:page-number", m_ds->m_pageSpan.m_pageNumber);

  _insertBreakIfNecessary(propList);
}

///////////////////
// list
///////////////////
void STOFFTextListener::_openListElement()
{
  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
    return;

  if (m_ps->m_isParagraphOpened || m_ps->m_isListElementOpened)
    return;

  if (!m_ps->m_isTableOpened && (!m_ps->m_inSubDocument || m_ps->m_subDocumentType == libstoff::DOC_TEXT_BOX)) {
    if (m_ps->m_sectionAttributesChanged)
      _closeSection();

    if (!m_ps->m_isSectionOpened)
      _openSection();
  }

  librevenge::RVNGPropertyList propList;
  _appendParagraphProperties(propList, true);
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

void STOFFTextListener::_closeListElement()
{
  if (m_ps->m_isListElementOpened) {
    if (m_ps->m_isSpanOpened)
      _closeSpan();

    if (m_ps->m_list) m_ps->m_list->closeElement();
    m_documentInterface->closeListElement();
  }

  m_ps->m_isListElementOpened = m_ps->m_isParagraphOpened = false;

  if (!m_ps->m_isTableOpened && m_ps->m_isPageSpanBreakDeferred && !m_ps->m_inSubDocument)
    _closePageSpan();
}

int STOFFTextListener::_getListId() const
{
  auto newLevel= size_t(m_ps->m_paragraph.m_listLevelIndex);
  if (newLevel == 0) return -1;
  int newListId = m_ps->m_paragraph.m_listId;
  if (newListId > 0) return newListId;
  static bool first = true;
  if (first) {
    STOFF_DEBUG_MSG(("STOFFTextListener::_getListId: the list id is not set, try to find a new one\n"));
    first = false;
  }
  auto list=m_listManager->getNewList(m_ps->m_list, int(newLevel), m_ps->m_paragraph.m_listLevel);
  if (!list) return -1;
  return list->getId();
}

void STOFFTextListener::_changeList()
{
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  size_t actualLevel = m_ps->m_listOrderedLevels.size();
  auto newLevel= size_t(m_ps->m_paragraph.m_listLevelIndex > 0 && !m_ps->m_paragraph.m_outline ? m_ps->m_paragraph.m_listLevelIndex : 0);
  if (newLevel>100) {
    STOFF_DEBUG_MSG(("STOFFTextListener::_changeList: find level=%d, set it to 100\n", static_cast<int>(newLevel)));
    newLevel=100;
  }
  if (!m_ps->m_isSectionOpened && newLevel && !m_ps->m_isTableOpened &&
      (!m_ps->m_inSubDocument || m_ps->m_subDocumentType == libstoff::DOC_TEXT_BOX))
    _openSection();

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
      STOFF_DEBUG_MSG(("STOFFTextListener::_changeList: can not find any list\n"));
      m_ps->m_listOrderedLevels.resize(actualLevel);
      return;
    }
    m_listManager->needToSend(newListId, m_ds->m_sentListMarkers);
    m_ps->m_list = theList;
    m_ps->m_list->setLevel(static_cast<int>(newLevel));
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
void STOFFTextListener::_openSpan()
{
  if (m_ps->m_isSpanOpened)
    return;

  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened)
    return;

  if (!m_ps->m_isParagraphOpened && !m_ps->m_isListElementOpened) {
    _changeList();
    if (m_ps->m_paragraph.m_listLevelIndex == 0 || m_ps->m_paragraph.m_outline)
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

void STOFFTextListener::_closeSpan()
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
void STOFFTextListener::_flushDeferredTabs()
{
  if (m_ps->m_numDeferredTabs == 0) return;
  if (!m_ps->m_isSpanOpened) _openSpan();
  for (; m_ps->m_numDeferredTabs > 0; m_ps->m_numDeferredTabs--)
    m_documentInterface->insertTab();
}

void STOFFTextListener::_flushText()
{
  if (m_ps->m_textBuffer.len() == 0) return;

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
void STOFFTextListener::insertNote(STOFFNote const &note, STOFFSubDocumentPtr &subDocument)
{
  if (m_ps->m_isNote) {
    STOFF_DEBUG_MSG(("STOFFTextListener::insertNote try to insert a note recursively (ignored)\n"));
    return;
  }

  m_ps->m_isNote = true;
  if (m_ds->m_isHeaderFooterOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::insertNote try to insert a note in a header/footer\n"));
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
      if (note.m_number >= 0)
        m_ds->m_endNoteNumber = note.m_number;
      else
        m_ds->m_endNoteNumber++;
      propList.insert("librevenge:number", m_ds->m_endNoteNumber);
      m_documentInterface->openEndnote(propList);
      handleSubDocument(subDocument, libstoff::DOC_NOTE);
      m_documentInterface->closeEndnote();
    }
  }
  m_ps->m_isNote = false;
}

void STOFFTextListener::insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &creator, librevenge::RVNGString const &date)
{
  if (m_ps->m_isNote) {
    STOFF_DEBUG_MSG(("STOFFTextListener::insertComment try to insert a note recursively (ignored)\n"));
    return;
  }

  if (!m_ps->m_isParagraphOpened)
    _openParagraph();
  else {
    _flushText();
    _closeSpan();
  }

  librevenge::RVNGPropertyList propList;
  if (!creator.empty())  propList.insert("dc:creator", creator);
  if (!date.empty())  propList.insert("meta:date-string", date);
  m_documentInterface->openComment(propList);

  m_ps->m_isNote = true;
  handleSubDocument(subDocument, libstoff::DOC_COMMENT_ANNOTATION);

  m_documentInterface->closeComment();
  m_ps->m_isNote = false;
}

void STOFFTextListener::insertTextBox
(STOFFFrameStyle const &frame, STOFFSubDocumentPtr subDocument, STOFFGraphicStyle const &frameStyle)
{
  if (!openFrame(frame, frameStyle)) return;

  librevenge::RVNGPropertyList propList;
  if (frameStyle.m_propertyList["librevenge:next-frame-name"])
    propList.insert("librevenge:next-frame-name",frameStyle.m_propertyList["librevenge:next-frame-name"]->getStr());
  STOFFGraphicStyle::checkForPadding(propList);
  m_documentInterface->openTextBox(propList);
  handleSubDocument(subDocument, libstoff::DOC_TEXT_BOX);
  m_documentInterface->closeTextBox();

  closeFrame();
}

void STOFFTextListener::insertShape(STOFFFrameStyle const &frame, STOFFGraphicShape const &shape, STOFFGraphicStyle const &style)
{
  if (!m_ps->m_isPageSpanOpened)
    _openPageSpan();
  // now check that the anchor is coherent with the actual state
  switch (frame.m_position.m_anchorTo) {
  case STOFFPosition::Page:
    if (m_ps->m_isParagraphOpened)
      _closeParagraph();
    break;
  case STOFFPosition::Paragraph:
    if (m_ps->m_isParagraphOpened)
      _flushText();
    else
      _openParagraph();
    break;
  case STOFFPosition::Unknown:
#if !defined(__clang__)
  default:
#endif
    STOFF_DEBUG_MSG(("STOFFTextListener::insertShape: UNKNOWN position, insert as char position\n"));
    STOFF_FALLTHROUGH;
  case STOFFPosition::CharBaseLine:
  case STOFFPosition::Char:
    if (m_ps->m_isSpanOpened)
      _flushText();
    else
      _openSpan();
    break;
  case STOFFPosition::Cell:
  case STOFFPosition::Frame:
    break;
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
#if !defined(__clang__)
  default:
    STOFF_DEBUG_MSG(("STOFFTextListener::insertShape: unexpected shape\n"));
    break;
#endif
  }
}

void STOFFTextListener::insertPicture(STOFFFrameStyle const &frame, STOFFEmbeddedObject const &picture, STOFFGraphicStyle const &style)
{
  if (!openFrame(frame, style)) return;

  librevenge::RVNGPropertyList propList;
  if (picture.addTo(propList))
    m_documentInterface->insertBinaryObject(propList);
  closeFrame();
}

void STOFFTextListener::insertEquation(STOFFFrameStyle const &frame, librevenge::RVNGString const &equation,
                                       STOFFGraphicStyle const &style)
{
  if (equation.empty()) {
    STOFF_DEBUG_MSG(("STOFFTextListener::insertEquation: oops the equation is empty\n"));
    return;
  }
  if (!openFrame(frame, style)) return;

  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:mime-type", "application/mathml+xml");
  propList.insert("librevenge:data", equation);
  m_documentInterface->insertEquation(propList);
  closeFrame();
}

///////////////////
// frame
///////////////////
bool STOFFTextListener::openFrame(STOFFFrameStyle const &frame, STOFFGraphicStyle const &style)
{
  if (m_ps->m_isTableOpened && !m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: called in table but cell is not opened\n"));
    return false;
  }
  if (m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: called but a frame is already opened\n"));
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
    STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: UNKNOWN position, insert as char position\n"));
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
      STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: can not determine the frame\n"));
      return false;
    }
    if (m_ps->m_subDocumentType==libstoff::DOC_HEADER_FOOTER_REGION) {
      STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: called with Frame position in header footer, switch to paragraph\n"));
      if (m_ps->m_isParagraphOpened)
        _flushText();
      else
        _openParagraph();
      finalFrame.m_position.m_anchorTo=STOFFPosition::Paragraph;
    }
    break;
  case STOFFPosition::Cell:
    if (!m_ps->m_isTableCellOpened) {
      STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: called with Cell position not in a table cell\n"));
      return false;
    }
    if (!finalFrame.m_position.m_propertyList["table:end-cell-address"]) {
      STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: can not find the cell name\n"));
      return false;
    }
    break;
#if !defined(__clang__)
  default:
    STOFF_DEBUG_MSG(("STOFFTextListener::openFrame: can not determine the anchor\n"));
    return false;
#endif
  }

  librevenge::RVNGPropertyList propList;
  _handleFrameParameters(propList, finalFrame, style);
  m_documentInterface->openFrame(propList);

  m_ps->m_isFrameOpened = true;
  return true;
}

void STOFFTextListener::closeFrame()
{
  if (!m_ps->m_isFrameOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeFrame: called but no frame is already opened\n"));
    return;
  }
  m_documentInterface->closeFrame();
  m_ps->m_isFrameOpened = false;
}

bool STOFFTextListener::openGroup(STOFFFrameStyle const &frame)
{
  if (!m_ds->m_isDocumentStarted) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openGroup: the document is not started\n"));
    return false;
  }
  if (m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openGroup: called in table or in a text zone\n"));
    return false;
  }

  // now check that the anchor is coherent with the actual state
  switch (frame.m_position.m_anchorTo) {
  case STOFFPosition::Page:
    break;
  case STOFFPosition::Paragraph:
    if (m_ps->m_isParagraphOpened)
      _flushText();
    else
      _openParagraph();
    break;
  case STOFFPosition::Unknown:
#if !defined(__clang__)
  default:
#endif
    STOFF_DEBUG_MSG(("STOFFTextListener::openGroup: UNKNOWN position, insert as char position\n"));
    STOFF_FALLTHROUGH;
  case STOFFPosition::CharBaseLine:
  case STOFFPosition::Char:
    if (m_ps->m_isSpanOpened)
      _flushText();
    else
      _openSpan();
    break;
  case STOFFPosition::Frame:
  case STOFFPosition::Cell:
    break;
  }

  _pushParsingState();
  _startSubDocument();
  m_ps->m_isGroupOpened = true;

  librevenge::RVNGPropertyList propList;
  frame.addTo(propList);
  m_documentInterface->openGroup(propList);

  return true;
}

void  STOFFTextListener::closeGroup()
{
  if (!m_ps->m_isGroupOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeGroup: called but no group is already opened\n"));
    return;
  }
  _endSubDocument();
  _popParsingState();
  m_documentInterface->closeGroup();
}

void STOFFTextListener::_handleFrameParameters
(librevenge::RVNGPropertyList &propList, STOFFFrameStyle const &frame, STOFFGraphicStyle const &style)
{
  if (!m_ds->m_isDocumentStarted)
    return;
  frame.addTo(propList);
  style.addTo(propList);
}

///////////////////
// subdocument
///////////////////
void STOFFTextListener::handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType)
{
  _pushParsingState();
  _startSubDocument();
  m_ps->m_subDocumentType = subDocumentType;

  m_ps->m_isPageSpanOpened = true;
  m_ps->m_list.reset();

  switch (subDocumentType) {
  case libstoff::DOC_TEXT_BOX:
    m_ps->m_sectionAttributesChanged = true;
    break;
  case libstoff::DOC_HEADER_FOOTER_REGION:
    m_ds->m_isHeaderFooterRegionOpened = true;
    m_ps->m_isHeaderFooterWithoutParagraph = true;
    break;
  case libstoff::DOC_NONE:
  case libstoff::DOC_CHART:
  case libstoff::DOC_CHART_ZONE:
  case libstoff::DOC_NOTE:
  case libstoff::DOC_SHEET:
  case libstoff::DOC_TABLE:
  case libstoff::DOC_COMMENT_ANNOTATION:
  case libstoff::DOC_GRAPHIC_GROUP:
#if !defined(__clang__)
  default:
#endif
    break;
  }

  // Check whether the document is calling itself
  bool sendDoc = true;
  for (auto const &doc : m_ds->m_subDocuments) {
    if (!subDocument)
      break;
    if (!doc)
      continue;
    if (*subDocument == *doc) {
      STOFF_DEBUG_MSG(("STOFFTextListener::handleSubDocument: recursif call, stop...\n"));
      sendDoc = false;
      break;
    }
  }
  if (sendDoc) {
    if (subDocument) {
      m_ds->m_subDocuments.push_back(subDocument);
      std::shared_ptr<STOFFListener> listen(this, STOFF_shared_ptr_noop_deleter<STOFFTextListener>());
      try {
        subDocument->parse(listen, subDocumentType);
      }
      catch (...) {
        STOFF_DEBUG_MSG(("Works: STOFFTextListener::handleSubDocument exception catched \n"));
      }
      m_ds->m_subDocuments.pop_back();
    }
    if (m_ps->m_isHeaderFooterWithoutParagraph)
      _openSpan();
  }

  switch (m_ps->m_subDocumentType) {
  case libstoff::DOC_TEXT_BOX:
    _closeSection();
    break;
  case libstoff::DOC_HEADER_FOOTER_REGION:
    m_ds->m_isHeaderFooterRegionOpened = false;
  case libstoff::DOC_NONE:
  case libstoff::DOC_CHART:
  case libstoff::DOC_CHART_ZONE:
  case libstoff::DOC_NOTE:
  case libstoff::DOC_SHEET:
  case libstoff::DOC_TABLE:
  case libstoff::DOC_COMMENT_ANNOTATION:
  case libstoff::DOC_GRAPHIC_GROUP:
#if !defined(__clang__)
  default:
#endif
    break;
  }
  _endSubDocument();
  _popParsingState();
}

bool STOFFTextListener::isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const
{
  if (!m_ps->m_inSubDocument)
    return false;
  subdocType = m_ps->m_subDocumentType;
  return true;
}

void STOFFTextListener::_startSubDocument()
{
  m_ds->m_isDocumentStarted = true;
  m_ps->m_inSubDocument = true;
}

void STOFFTextListener::_endSubDocument()
{
  if (m_ps->m_isTableOpened)
    closeTable();
  if (m_ps->m_isParagraphOpened)
    _closeParagraph();

  m_ps->m_paragraph.m_listLevelIndex=0;
  _changeList(); // flush the list exterior
}

///////////////////
// table
///////////////////
void STOFFTextListener::openTable(STOFFTable const &table)
{
  if (m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openTable: called with m_isTableOpened=true\n"));
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

void STOFFTextListener::closeTable()
{
  if (!m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeTable: called with m_isTableOpened=false\n"));
    return;
  }

  m_ps->m_isTableOpened = false;
  _endSubDocument();
  m_documentInterface->closeTable();

  _popParsingState();
}

void STOFFTextListener::openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow)
{
  if (m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openTableRow: called with m_isTableRowOpened=true\n"));
    return;
  }
  if (!m_ps->m_isTableOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openTableRow: called with m_isTableOpened=false\n"));
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

void STOFFTextListener::closeTableRow()
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openTableRow: called with m_isTableRowOpened=false\n"));
    return;
  }
  m_ps->m_isTableRowOpened = false;
  m_documentInterface->closeTableRow();
}

void STOFFTextListener::addCoveredTableCell(STOFFVec2i const &pos)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::addCoveredTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::addCoveredTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }
  librevenge::RVNGPropertyList propList;
  propList.insert("librevenge:column", pos[0]);
  propList.insert("librevenge:row", pos[1]);
  m_documentInterface->insertCoveredTableCell(propList);
}

void STOFFTextListener::addEmptyTableCell(STOFFVec2i const &pos, STOFFVec2i span)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::addEmptyTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::addEmptyTableCell: called with m_isTableCellOpened=true\n"));
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

void STOFFTextListener::openTableCell(STOFFCell const &cell)
{
  if (!m_ps->m_isTableRowOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openTableCell: called with m_isTableRowOpened=false\n"));
    return;
  }
  if (m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::openTableCell: called with m_isTableCellOpened=true\n"));
    closeTableCell();
  }

  librevenge::RVNGPropertyList propList;
  cell.addTo(propList);
  m_ps->m_isTableCellOpened = true;
  m_documentInterface->openTableCell(propList);
}

void STOFFTextListener::closeTableCell()
{
  if (!m_ps->m_isTableCellOpened) {
    STOFF_DEBUG_MSG(("STOFFTextListener::closeTableCell: called with m_isTableCellOpened=false\n"));
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
std::shared_ptr<STOFFTextListenerInternal::State> STOFFTextListener::_pushParsingState()
{
  auto actual = m_ps;
  m_psStack.push_back(actual);
  m_ps.reset(new STOFFTextListenerInternal::State);

  m_ps->m_isNote = actual->m_isNote;

  return actual;
}

void STOFFTextListener::_popParsingState()
{
  if (m_psStack.size()==0) {
    STOFF_DEBUG_MSG(("STOFFTextListener::_popParsingState: psStack is empty()\n"));
    throw libstoff::ParseException();
  }
  m_ps = m_psStack.back();
  m_psStack.pop_back();
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
