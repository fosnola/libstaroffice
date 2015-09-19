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

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"
#include "STOFFListener.hxx"
#include "STOFFParagraph.hxx"
#include "STOFFSubDocument.hxx"

#include "STOFFPageSpan.hxx"

/** Internal: the structures of a STOFFPageSpan */
namespace STOFFPageSpanInternal
{
////////////////////////////////////////
//! Internal: the subdocument of a STOFFParser
class SubDocument : public STOFFSubDocument
{
public:
  //! constructor
  SubDocument(STOFFHeaderFooter const &headerFooter) :
    STOFFSubDocument(0, STOFFInputStreamPtr(), STOFFEntry()), m_headerFooter(headerFooter) {}

  //! destructor
  virtual ~SubDocument() {}

  //! operator!=
  virtual bool operator!=(STOFFSubDocument const &doc) const
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    SubDocument const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_headerFooter != sDoc->m_headerFooter) return true;
    return false;
  }

  //! operator!==
  virtual bool operator==(STOFFSubDocument const &doc) const
  {
    return !operator!=(doc);
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type);

protected:
  //! the header footer
  STOFFHeaderFooter const &m_headerFooter;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("STOFFPageSpanInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (m_headerFooter.m_pageNumberPosition >= STOFFHeaderFooter::TopLeft &&
      m_headerFooter.m_pageNumberPosition <= STOFFHeaderFooter::TopRight)
    m_headerFooter.insertPageNumberParagraph(listener.get());
  if (m_headerFooter.m_subDocument)
    m_headerFooter.m_subDocument->parse(listener, type);
  if (m_headerFooter.m_pageNumberPosition >= STOFFHeaderFooter::BottomLeft &&
      m_headerFooter.m_pageNumberPosition <= STOFFHeaderFooter::BottomRight)
    m_headerFooter.insertPageNumberParagraph(listener.get());
}
}

// ----------------- STOFFHeaderFooter ------------------------
STOFFHeaderFooter::STOFFHeaderFooter(STOFFHeaderFooter::Type const type, STOFFHeaderFooter::Occurrence const occurrence) :
  m_type(type), m_occurrence(occurrence), m_height(0),
  m_pageNumberPosition(STOFFHeaderFooter::None), m_pageNumberType(libstoff::ARABIC),
  m_pageNumberFont("Geneva",12), m_subDocument()
{
}

STOFFHeaderFooter::~STOFFHeaderFooter()
{
}

bool STOFFHeaderFooter::operator==(STOFFHeaderFooter const &hf) const
{
  if (&hf==this) return true;
  if (m_type != hf.m_type)
    return false;
  if (m_type == UNDEF)
    return true;
  if (m_occurrence != hf.m_occurrence)
    return false;
  if (m_height < hf.m_height || m_height > hf.m_height)
    return false;
  if (m_pageNumberPosition != hf.m_pageNumberPosition ||
      m_pageNumberType != hf.m_pageNumberType ||
      m_pageNumberFont != hf.m_pageNumberFont)
    return false;
  if (!m_subDocument)
    return !hf.m_subDocument;
  if (*m_subDocument.get() != hf.m_subDocument)
    return false;
  return true;
}

// send data to the listener
void STOFFHeaderFooter::send(STOFFListener *listener) const
{
  if (m_type == UNDEF)
    return;
  if (!listener) {
    STOFF_DEBUG_MSG(("STOFFHeaderFooter::send: called without listener\n"));
    return;
  }
  librevenge::RVNGPropertyList propList;
  switch (m_occurrence) {
  case ODD:
    propList.insert("librevenge:occurrence", "odd");
    break;
  case EVEN:
    propList.insert("librevenge:occurrence", "even");
    break;
  case ALL:
    propList.insert("librevenge:occurrence", "all");
    break;
  case NEVER:
  default:
    break;
  }
  if (m_pageNumberPosition!=None) {
    shared_ptr<STOFFPageSpanInternal::SubDocument> doc
    (new STOFFPageSpanInternal::SubDocument(*this));
    if (m_type == HEADER)
      listener->insertHeader(doc,propList);
    else
      listener->insertFooter(doc,propList);
    return;
  }
  if (m_type == HEADER)
    listener->insertHeader(m_subDocument,propList);
  else
    listener->insertFooter(m_subDocument,propList);
}

void STOFFHeaderFooter::insertPageNumberParagraph(STOFFListener *listener) const
{
  STOFFParagraph para;
  para.m_justify = STOFFParagraph::JustificationCenter;
  switch (m_pageNumberPosition) {
  case TopLeft:
  case BottomLeft:
    para.m_justify = STOFFParagraph::JustificationLeft;
    break;
  case TopRight:
  case BottomRight:
    para.m_justify = STOFFParagraph::JustificationRight;
    break;
  case TopCenter:
  case BottomCenter:
  case None:
    break;
  default:
    STOFF_DEBUG_MSG(("STOFFHeaderFooter::insertPageNumberParagraph: unexpected value\n"));
    break;
  }
  listener->setParagraph(para);
  listener->setFont(m_pageNumberFont);
  if (listener->isParagraphOpened())
    listener->insertEOL();

  STOFFField field(STOFFField::PageNumber);
  field.m_numberingType=m_pageNumberType;
  listener->insertField(field);
}

// ----------------- STOFFPageSpan ------------------------
STOFFPageSpan::STOFFPageSpan() :
  m_formLength(11.0), m_formWidth(8.5), m_formOrientation(STOFFPageSpan::PORTRAIT),
  m_name(""), m_masterName(""),
  m_backgroundColor(STOFFColor::white()),
  m_pageNumber(-1),
  m_headerFooterList(),
  m_pageSpan(1)
{
  for (int i = 0; i < 4; i++) m_margins[i] = 1.0;
}

STOFFPageSpan::~STOFFPageSpan()
{
}

void STOFFPageSpan::setHeaderFooter(STOFFHeaderFooter const &hF)
{
  STOFFHeaderFooter::Type const type=hF.m_type;
  switch (hF.m_occurrence) {
  case STOFFHeaderFooter::NEVER:
    removeHeaderFooter(type, STOFFHeaderFooter::ALL);
  case STOFFHeaderFooter::ALL:
    removeHeaderFooter(type, STOFFHeaderFooter::ODD);
    removeHeaderFooter(type, STOFFHeaderFooter::EVEN);
    break;
  case STOFFHeaderFooter::ODD:
    removeHeaderFooter(type, STOFFHeaderFooter::ALL);
    break;
  case STOFFHeaderFooter::EVEN:
    removeHeaderFooter(type, STOFFHeaderFooter::ALL);
    break;
  default:
    break;
  }
  int pos = getHeaderFooterPosition(hF.m_type, hF.m_occurrence);
  if (pos != -1)
    m_headerFooterList[size_t(pos)]=hF;

  bool containsHFLeft = containsHeaderFooter(type, STOFFHeaderFooter::ODD);
  bool containsHFRight = containsHeaderFooter(type, STOFFHeaderFooter::EVEN);

  if (containsHFLeft && !containsHFRight) {
    STOFF_DEBUG_MSG(("Inserting dummy header right\n"));
    STOFFHeaderFooter dummy(type, STOFFHeaderFooter::EVEN);
    pos = getHeaderFooterPosition(type, STOFFHeaderFooter::EVEN);
    if (pos != -1)
      m_headerFooterList[size_t(pos)]=STOFFHeaderFooter(type, STOFFHeaderFooter::EVEN);
  }
  else if (!containsHFLeft && containsHFRight) {
    STOFF_DEBUG_MSG(("Inserting dummy header left\n"));
    pos = getHeaderFooterPosition(type, STOFFHeaderFooter::ODD);
    if (pos != -1)
      m_headerFooterList[size_t(pos)]=STOFFHeaderFooter(type, STOFFHeaderFooter::ODD);
  }
}

void STOFFPageSpan::checkMargins()
{
  if (m_margins[libstoff::Left]+m_margins[libstoff::Right] > 0.95*m_formWidth) {
    STOFF_DEBUG_MSG(("STOFFPageSpan::checkMargins: left/right margins seems bad\n"));
    m_margins[libstoff::Left] = m_margins[libstoff::Right] = 0.05*m_formWidth;
  }
  if (m_margins[libstoff::Top]+m_margins[libstoff::Bottom] > 0.95*m_formLength) {
    STOFF_DEBUG_MSG(("STOFFPageSpan::checkMargins: top/bottom margins seems bad\n"));
    m_margins[libstoff::Top] = m_margins[libstoff::Bottom] = 0.05*m_formLength;
  }
}

void STOFFPageSpan::sendHeaderFooters(STOFFListener *listener) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("STOFFPageSpan::sendHeaderFooters: no listener\n"));
    return;
  }

  for (size_t i = 0; i < m_headerFooterList.size(); i++) {
    STOFFHeaderFooter const &hf = m_headerFooterList[i];
    if (!hf.isDefined()) continue;
    hf.send(listener);
  }
}

void STOFFPageSpan::sendHeaderFooters(STOFFListener *listener, STOFFHeaderFooter::Occurrence occurrence) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("STOFFPageSpan::sendHeaderFooters: no listener\n"));
    return;
  }

  for (size_t i = 0; i < m_headerFooterList.size(); i++) {
    STOFFHeaderFooter const &hf = m_headerFooterList[i];
    if (!hf.isDefined()) continue;
    if (hf.m_occurrence==occurrence || hf.m_occurrence==STOFFHeaderFooter::ALL)
      hf.send(listener);
  }
}

void STOFFPageSpan::getPageProperty(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("librevenge:num-pages", getPageSpan());

  if (hasPageName())
    propList.insert("draw:name", getPageName());
  if (hasMasterPageName())
    propList.insert("librevenge:master-page-name", getMasterPageName());
  propList.insert("fo:page-height", getFormLength(), librevenge::RVNG_INCH);
  propList.insert("fo:page-width", getFormWidth(), librevenge::RVNG_INCH);
  if (getFormOrientation() == LANDSCAPE)
    propList.insert("style:print-orientation", "landscape");
  else
    propList.insert("style:print-orientation", "portrait");
  propList.insert("fo:margin-left", getMarginLeft(), librevenge::RVNG_INCH);
  propList.insert("fo:margin-right", getMarginRight(), librevenge::RVNG_INCH);
  propList.insert("fo:margin-top", getMarginTop(), librevenge::RVNG_INCH);
  propList.insert("fo:margin-bottom", getMarginBottom(), librevenge::RVNG_INCH);
  if (!m_backgroundColor.isWhite())
    propList.insert("fo:background-color", m_backgroundColor.str().c_str());
}


bool STOFFPageSpan::operator==(shared_ptr<STOFFPageSpan> const &page2) const
{
  if (!page2) return false;
  if (page2.get() == this) return true;
  if (m_formLength < page2->m_formLength || m_formLength > page2->m_formLength ||
      m_formWidth < page2->m_formWidth || m_formWidth > page2->m_formWidth ||
      m_formOrientation != page2->m_formOrientation)
    return false;
  if (getMarginLeft() < page2->getMarginLeft() || getMarginLeft() > page2->getMarginLeft() ||
      getMarginRight() < page2->getMarginRight() || getMarginRight() > page2->getMarginRight() ||
      getMarginTop() < page2->getMarginTop() || getMarginTop() > page2->getMarginTop() ||
      getMarginBottom() < page2->getMarginBottom() || getMarginBottom() > page2->getMarginBottom())
    return false;
  if (getPageName() != page2->getPageName() || getMasterPageName() != page2->getMasterPageName() ||
      backgroundColor() != page2->backgroundColor())
    return false;

  if (getPageNumber() != page2->getPageNumber())
    return false;

  size_t numHF = m_headerFooterList.size();
  size_t numHF2 = page2->m_headerFooterList.size();
  for (size_t i = numHF; i < numHF2; i++) {
    if (page2->m_headerFooterList[i].isDefined())
      return false;
  }
  for (size_t i = numHF2; i < numHF; i++) {
    if (m_headerFooterList[i].isDefined())
      return false;
  }
  if (numHF2 < numHF) numHF = numHF2;
  for (size_t i = 0; i < numHF; i++) {
    if (m_headerFooterList[i] != page2->m_headerFooterList[i])
      return false;
  }
  STOFF_DEBUG_MSG(("WordPerfect: STOFFPageSpan == comparison finished, found no differences\n"));

  return true;
}

// -------------- manage header footer list ------------------
void STOFFPageSpan::removeHeaderFooter(STOFFHeaderFooter::Type type, STOFFHeaderFooter::Occurrence occurrence)
{
  int pos = getHeaderFooterPosition(type, occurrence);
  if (pos == -1) return;
  m_headerFooterList[size_t(pos)]=STOFFHeaderFooter();
}

bool STOFFPageSpan::containsHeaderFooter(STOFFHeaderFooter::Type type, STOFFHeaderFooter::Occurrence occurrence)
{
  int pos = getHeaderFooterPosition(type, occurrence);
  if (pos == -1 || !m_headerFooterList[size_t(pos)].isDefined()) return false;
  return true;
}

int STOFFPageSpan::getHeaderFooterPosition(STOFFHeaderFooter::Type type, STOFFHeaderFooter::Occurrence occurrence)
{
  int typePos = 0, occurrencePos = 0;
  switch (type) {
  case STOFFHeaderFooter::HEADER:
    typePos = 0;
    break;
  case STOFFHeaderFooter::FOOTER:
    typePos = 1;
    break;
  case STOFFHeaderFooter::UNDEF:
  default:
    STOFF_DEBUG_MSG(("STOFFPageSpan::getVectorPosition: unknown type\n"));
    return -1;
  }
  switch (occurrence) {
  case STOFFHeaderFooter::ALL:
    occurrencePos = 0;
    break;
  case STOFFHeaderFooter::ODD:
    occurrencePos = 1;
    break;
  case STOFFHeaderFooter::EVEN:
    occurrencePos = 2;
    break;
  case STOFFHeaderFooter::NEVER:
  default:
    STOFF_DEBUG_MSG(("STOFFPageSpan::getVectorPosition: unknown occurrence\n"));
    return -1;
  }
  size_t res = size_t(typePos*3+occurrencePos);
  if (res >= m_headerFooterList.size())
    m_headerFooterList.resize(res+1);
  return int(res);
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
