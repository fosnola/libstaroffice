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

// ----------------- STOFFPageSpan ------------------------
STOFFPageSpan::STOFFPageSpan() :
  m_formLength(11.0), m_formWidth(8.5), m_formOrientation(STOFFPageSpan::PORTRAIT),
  m_name(""), m_masterName(""),
  m_backgroundColor(STOFFColor::white()),
  m_pageNumber(-1),
  m_occurenceHeaderMap(), m_occurenceFooterMap(),
  m_pageSpan(1)
{
  for (int i = 0; i < 4; i++) m_margins[i] = 1.0;
}

STOFFPageSpan::~STOFFPageSpan()
{
}

void STOFFPageSpan::addHeader(librevenge::RVNGString const &occurence, STOFFSubDocumentPtr document)
{
  if (!document) {
    if (m_occurenceHeaderMap.find(occurence)!=m_occurenceHeaderMap.end())
      m_occurenceHeaderMap.erase(occurence);
    return;
  }
  m_occurenceHeaderMap[occurence]=document;
}

void STOFFPageSpan::addFooter(librevenge::RVNGString const &occurence, STOFFSubDocumentPtr document)
{
  if (!document) {
    if (m_occurenceFooterMap.find(occurence)!=m_occurenceFooterMap.end())
      m_occurenceFooterMap.erase(occurence);
    return;
  }
  m_occurenceFooterMap[occurence]=document;
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

  std::map<librevenge::RVNGString,STOFFSubDocumentPtr>::const_iterator it;
  for (it=m_occurenceHeaderMap.begin(); it!=m_occurenceHeaderMap.end(); ++it) {
    if (it->first.empty()) continue;
    librevenge::RVNGPropertyList propList;
    propList.insert("librevenge:occurrence", it->first);
    listener->insertHeader(it->second,propList);
  }
  for (it=m_occurenceFooterMap.begin(); it!=m_occurenceFooterMap.end(); ++it) {
    if (it->first.empty()) continue;
    librevenge::RVNGPropertyList propList;
    propList.insert("librevenge:occurrence", it->first);
    listener->insertFooter(it->second,propList);
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

  for (int step=0; step<2; ++step) {
    std::map<librevenge::RVNGString,STOFFSubDocumentPtr> const &map1=
      step==0 ? m_occurenceHeaderMap : m_occurenceFooterMap;
    std::map<librevenge::RVNGString,STOFFSubDocumentPtr> const &map2=
      step==0 ? page2->m_occurenceHeaderMap : page2->m_occurenceFooterMap;
    if (map1.size()!=map2.size())
      return false;
    std::map<librevenge::RVNGString,STOFFSubDocumentPtr>::const_iterator it1, it2;
    for (it1=map1.begin(); it1!=map1.end(); ++it1) {
      it2=map2.find(it1->first);
      if (it2==map2.end()) return false;
      if (it1->second) {
        if (!it2->second || *it1->second!=*it2->second)
          return false;
      }
      else if (it2->second)
        return false;
    }
  }
  STOFF_DEBUG_MSG(("WordPerfect: STOFFPageSpan == comparison finished, found no differences\n"));

  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
