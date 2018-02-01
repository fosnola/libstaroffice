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

// ----------------- STOFFHeaderFooter ------------------------
bool STOFFHeaderFooter::operator==(STOFFHeaderFooter const &hF) const
{
  for (int step=0; step<4; ++step) {
    if (m_subDocument[step]) {
      if (!hF.m_subDocument[step] || *m_subDocument[step]!=hF.m_subDocument[step])
        return false;
    }
    else if (hF.m_subDocument[step])
      return false;
  }
  return true;
}

void STOFFHeaderFooter::send(STOFFListener *listener, bool isHeader) const
{
  librevenge::RVNGPropertyList propList;
  if (!listener) {
    STOFF_DEBUG_MSG(("STOFFHeaderFooter::send: called without listener\n"));
    return;
  }
  for (int i=0; i<4; ++i) {
    char const *wh[]= {"region-left", "region-center", "region-right", ""};
    if (!m_subDocument[i]) continue;
    if (isHeader)
      listener->insertHeaderRegion(m_subDocument[i], wh[i]);
    else
      listener->insertFooterRegion(m_subDocument[i], wh[i]);
  }
}

// ----------------- STOFFPageSpan ------------------------
STOFFPageSpan::STOFFPageSpan()
  : m_pageSpan(1)
  , m_section()
  , m_pageNumber(-1)
{
  m_propertiesList[0].insert("fo:page-height", 11., librevenge::RVNG_INCH);
  m_propertiesList[0].insert("fo:page-width", 8.5, librevenge::RVNG_INCH);
  m_propertiesList[0].insert("style:print-orientation", "portrait");
}

STOFFPageSpan::~STOFFPageSpan()
{
}

void STOFFPageSpan::addHeaderFooter(bool header, std::string const &occurrence, STOFFHeaderFooter const &hf)
{
  m_occurrenceHFMap[header ? 0 : 1][occurrence]=hf;
}

void STOFFPageSpan::sendHeaderFooters(STOFFListener *listener) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("STOFFPageSpan::sendHeaderFooters: no listener\n"));
    return;
  }

  for (int i=0; i<2; ++i) {
    for (auto &hf : m_occurrenceHFMap[i]) {
      std::string const &occurrence=hf.first;
      if (occurrence.empty()) continue;
      librevenge::RVNGPropertyList propList;
      propList.insert("librevenge:occurrence", occurrence.c_str());
      if (i==0)
        listener->openHeader(propList);
      else
        listener->openFooter(propList);
      hf.second.send(listener, i==0);
      if (i==0)
        listener->closeHeader();
      else
        listener->closeFooter();
    }
  }
}

void STOFFPageSpan::getPageProperty(librevenge::RVNGPropertyList &propList) const
{
  propList=m_propertiesList[0];
  propList.insert("librevenge:num-pages", m_pageSpan);
  for (int i=1; i<3; ++i) {
    if (m_propertiesList[i].empty())
      continue;
    librevenge::RVNGPropertyListVector vect;
    vect.append(m_propertiesList[i]);
    propList.insert(i==1 ? "librevenge:header" : "librevenge:footer", vect);
  }
}


bool STOFFPageSpan::operator==(std::shared_ptr<STOFFPageSpan> const &page2) const
{
  if (!page2) return false;
  if (page2.get() == this) return true;

  for (int i=0; i<3; ++i) {
    if (m_propertiesList[i].getPropString() != page2->m_propertiesList[i].getPropString())
      return false;
  }
  for (int i=0; i<2; ++i) {
    if (m_occurrenceHFMap[i].size()!=page2->m_occurrenceHFMap[i].size())
      return false;
    std::map<std::string,STOFFHeaderFooter>::const_iterator it1, it2;
    for (auto &hf : m_occurrenceHFMap[i]) {
      it2=page2->m_occurrenceHFMap[i].find(hf.first);
      if (it2==page2->m_occurrenceHFMap[i].end() || hf.second!=it2->second) return false;
    }
  }
  STOFF_DEBUG_MSG(("WordPerfect: STOFFPageSpan == comparison finished, found no differences\n"));

  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
