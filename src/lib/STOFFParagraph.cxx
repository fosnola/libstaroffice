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

#include <map>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFList.hxx"

#include "STOFFParagraph.hxx"

////////////////////////////////////////////////////////////
// tabstop
////////////////////////////////////////////////////////////
void STOFFTabStop::addTo(librevenge::RVNGPropertyListVector &propList, double decalX) const
{
  librevenge::RVNGPropertyList tab;

  // type
  switch (m_alignment) {
  case RIGHT:
    tab.insert("style:type", "right");
    break;
  case CENTER:
    tab.insert("style:type", "center");
    break;
  case DECIMAL:
    tab.insert("style:type", "char");
    if (m_decimalCharacter) {
      librevenge::RVNGString sDecimal;
      libstoff::appendUnicode(m_decimalCharacter, sDecimal);
      tab.insert("style:char", sDecimal);
    }
    else
      tab.insert("style:char", ".");
    break;
  case LEFT:
  case BAR: // BAR is not handled in OO
  default:
    break;
  }

  // leader character
  if (m_leaderCharacter != 0x0000) {
    librevenge::RVNGString sLeader;
    libstoff::appendUnicode(m_leaderCharacter, sLeader);
    tab.insert("style:leader-text", sLeader);
    tab.insert("style:leader-style", "solid");
  }

  // position
  double position = m_position+decalX;
  if (position < 0.00005 && position > -0.00005)
    position = 0.0;
  tab.insert("style:position", position, librevenge::RVNG_INCH);

  propList.append(tab);
}

int STOFFTabStop::cmp(STOFFTabStop const &tabs) const
{
  if (m_position < tabs.m_position) return -1;
  if (m_position > tabs.m_position) return 1;
  if (m_alignment < tabs.m_alignment) return -1;
  if (m_alignment > tabs.m_alignment) return 1;
  if (m_leaderCharacter < tabs.m_leaderCharacter) return -1;
  if (m_leaderCharacter > tabs.m_leaderCharacter) return 1;
  if (m_decimalCharacter < tabs.m_decimalCharacter) return -1;
  if (m_decimalCharacter > tabs.m_decimalCharacter) return 1;
  return 0;
}

// operator<<
std::ostream &operator<<(std::ostream &o, STOFFTabStop const &tab)
{
  o << tab.m_position;

  switch (tab.m_alignment) {
  case STOFFTabStop::LEFT:
    o << "L";
    break;
  case STOFFTabStop::CENTER:
    o << "C";
    break;
  case STOFFTabStop::RIGHT:
    o << "R";
    break;
  case STOFFTabStop::DECIMAL:
    o << ":decimal";
    break;
  case STOFFTabStop::BAR:
    o << ":bar";
    break;
  default:
    o << ":#type=" << int(tab.m_alignment);
    break;
  }
  if (tab.m_leaderCharacter != '\0')
    o << ":sep='"<< (char) tab.m_leaderCharacter << "'";
  if (tab.m_decimalCharacter && tab.m_decimalCharacter != '.')
    o << ":dec='" << (char) tab.m_decimalCharacter << "'";
  return o;
}

////////////////////////////////////////////////////////////
// paragraph
////////////////////////////////////////////////////////////
STOFFParagraph::STOFFParagraph() : m_marginsUnit(librevenge::RVNG_INCH), m_spacingsInterlineUnit(librevenge::RVNG_PERCENT), m_spacingsInterlineType(Fixed),
  m_tabs(), m_tabsRelativeToLeftMargin(false), m_justify(JustificationLeft), m_breakStatus(0),
  m_listLevelIndex(0), m_listId(-1), m_listStartValue(-1), m_listLevel(), m_backgroundColor(STOFFColor::white()),
  m_borders(), m_styleName(""), m_extra("")
{
  for (int i = 0; i < 3; i++) m_margins[i] = m_spacings[i] = 0.0;
  m_spacings[0] = 1.0; // interline normal
  for (int i = 0; i < 3; i++) {
    m_margins[i].setSet(false);
    m_spacings[i].setSet(false);
  }
}

STOFFParagraph::~STOFFParagraph()
{
}

int STOFFParagraph::cmp(STOFFParagraph const &para) const
{
  for (int i = 0; i < 3; i++) {
    if (*(m_margins[i]) < *(para.m_margins[i])) return -1;
    if (*(m_margins[i]) > *(para.m_margins[i])) return 1;
    if (*(m_spacings[i]) < *(para.m_spacings[i])) return -1;
    if (*(m_spacings[i]) > *(para.m_spacings[i])) return 1;
  }
  if (*m_justify < *para.m_justify) return -1;
  if (*m_justify > *para.m_justify) return -1;
  if (*m_marginsUnit < *para.m_marginsUnit) return -1;
  if (*m_marginsUnit > *para.m_marginsUnit) return -1;
  if (*m_spacingsInterlineUnit < *para.m_spacingsInterlineUnit) return -1;
  if (*m_spacingsInterlineUnit > *para.m_spacingsInterlineUnit) return -1;
  if (*m_spacingsInterlineType < *para.m_spacingsInterlineType) return -1;
  if (*m_spacingsInterlineType > *para.m_spacingsInterlineType) return -1;
  if (*m_tabsRelativeToLeftMargin < *para.m_tabsRelativeToLeftMargin) return -1;
  if (*m_tabsRelativeToLeftMargin > *para.m_tabsRelativeToLeftMargin) return -1;

  if (m_tabs->size() < para.m_tabs->size()) return -1;
  if (m_tabs->size() > para.m_tabs->size()) return -1;

  for (size_t i=0; i < m_tabs->size(); i++) {
    int diff=(*m_tabs)[i].cmp((*para.m_tabs)[i]);
    if (diff) return diff;
  }
  if (*m_breakStatus < *para.m_breakStatus) return -1;
  if (*m_breakStatus > *para.m_breakStatus) return -1;
  if (*m_listLevelIndex < *para.m_listLevelIndex) return -1;
  if (*m_listLevelIndex > *para.m_listLevelIndex) return -1;
  if (*m_listId < *para.m_listId) return -1;
  if (*m_listId > *para.m_listId) return -1;
  if (*m_listStartValue < *para.m_listStartValue) return -1;
  if (*m_listStartValue > *para.m_listStartValue) return -1;
  int diff=m_listLevel->cmp(*para.m_listLevel);
  if (diff) return diff;
  if (*m_backgroundColor < *para.m_backgroundColor) return -1;
  if (*m_backgroundColor > *para.m_backgroundColor) return -1;

  if (m_borders.size() < para.m_borders.size()) return -1;
  if (m_borders.size() > para.m_borders.size()) return 1;
  for (size_t i=0; i < m_borders.size(); i++) {
    if (m_borders[i].isSet() != para.m_borders[i].isSet())
      return m_borders[i].isSet() ? 1 : -1;
    diff = m_borders[i]->compare(*(para.m_borders[i]));
    if (diff) return diff;
  }
  if (m_styleName<para.m_styleName) return -1;
  if (m_styleName>para.m_styleName) return 1;
  return 0;
}

void STOFFParagraph::insert(STOFFParagraph const &para)
{
  for (int i = 0; i < 3; i++) {
    m_margins[i].insert(para.m_margins[i]);
    m_spacings[i].insert(para.m_spacings[i]);
  }
  m_marginsUnit.insert(para.m_marginsUnit);
  m_spacingsInterlineUnit.insert(para.m_spacingsInterlineUnit);
  m_spacingsInterlineType.insert(para.m_spacingsInterlineType);
  if (para.m_tabs.isSet() && m_tabs.isSet()) {
    std::map<double, STOFFTabStop> all;
    for (size_t t=0; t <m_tabs->size(); ++t)
      all[(*m_tabs)[t].m_position]=(*m_tabs)[t];
    for (size_t t=0; t <para.m_tabs->size(); ++t)
      all[(*para.m_tabs)[t].m_position]=(*para.m_tabs)[t];

    m_tabs->resize(0);
    std::map<double, STOFFTabStop>::const_iterator it=all.begin();
    for (; it!=all.end(); ++it)
      m_tabs->push_back(it->second);
  }
  else if (para.m_tabs.isSet())
    m_tabs=para.m_tabs;
  m_tabsRelativeToLeftMargin.insert(para.m_tabsRelativeToLeftMargin);
  m_justify.insert(para.m_justify);
  m_breakStatus.insert(para.m_breakStatus);
  m_listLevelIndex.insert(para.m_listLevelIndex);
  m_listId.insert(para.m_listId);
  m_listStartValue.insert(m_listStartValue);
  m_listLevel.insert(para.m_listLevel);
  m_backgroundColor.insert(para.m_backgroundColor);
  if (m_borders.size() < para.m_borders.size())
    m_borders.resize(para.m_borders.size());
  for (size_t i = 0; i < para.m_borders.size(); i++)
    m_borders[i].insert(para.m_borders[i]);
  m_styleName=para.m_styleName; // checkme
  m_extra += para.m_extra;
}

bool STOFFParagraph::hasBorders() const
{
  for (size_t i = 0; i < m_borders.size() && i < 4; i++) {
    if (!m_borders[i].isSet())
      continue;
    if (!m_borders[i]->isEmpty())
      return true;
  }
  return false;
}

bool STOFFParagraph::hasDifferentBorders() const
{
  if (!hasBorders()) return false;
  if (m_borders.size() < 4) return true;
  for (size_t i = 1; i < m_borders.size(); i++) {
    if (m_borders[i].isSet() != m_borders[0].isSet())
      return true;
    if (*(m_borders[i]) != *(m_borders[0]))
      return true;
  }
  return false;
}

double STOFFParagraph::getMarginsWidth() const
{
  double factor = (double) libstoff::getScaleFactor(*m_marginsUnit, librevenge::RVNG_INCH);
  return factor*(*(m_margins[1])+*(m_margins[2]));
}

void STOFFParagraph::addTo(librevenge::RVNGPropertyList &propList, bool inTable) const
{
  switch (*m_justify) {
  case JustificationLeft:
    // doesn't require a paragraph prop - it is the default
    propList.insert("fo:text-align", "left");
    break;
  case JustificationCenter:
    propList.insert("fo:text-align", "center");
    break;
  case JustificationRight:
    propList.insert("fo:text-align", "end");
    break;
  case JustificationFull:
    propList.insert("fo:text-align", "justify");
    break;
  case JustificationFullAllLines:
    propList.insert("fo:text-align", "justify");
    propList.insert("fo:text-align-last", "justify");
    break;
  default:
    break;
  }
  if (!inTable) {
    propList.insert("fo:margin-left", *m_margins[1], *m_marginsUnit);
    propList.insert("fo:text-indent", *m_margins[0], *m_marginsUnit);
    propList.insert("fo:margin-right", *m_margins[2], *m_marginsUnit);
    if (!m_styleName.empty())
      propList.insert("style:display-name", m_styleName.c_str());
    if (!m_backgroundColor->isWhite())
      propList.insert("fo:background-color", m_backgroundColor->str().c_str());
    if (hasBorders()) {
      bool setAll = !hasDifferentBorders();
      for (size_t w = 0; w < m_borders.size() && w < 4; w++) {
        if (w && setAll)
          break;
        if (!m_borders[w].isSet())
          continue;
        STOFFBorder const &border = *(m_borders[w]);
        if (border.isEmpty())
          continue;
        if (setAll) {
          border.addTo(propList);
          break;
        }
        switch (w) {
        case libstoff::Left:
          border.addTo(propList,"left");
          break;
        case libstoff::Right:
          border.addTo(propList,"right");
          break;
        case libstoff::Top:
          border.addTo(propList,"top");
          break;
        case libstoff::Bottom:
          border.addTo(propList,"bottom");
          break;
        default:
          STOFF_DEBUG_MSG(("STOFFParagraph::addTo: can not send %d border\n",int(w)));
          break;
        }
      }
    }
  }
  propList.insert("fo:margin-top", *(m_spacings[1]), librevenge::RVNG_INCH);
  propList.insert("fo:margin-bottom", *(m_spacings[2]), librevenge::RVNG_INCH);
  switch (*m_spacingsInterlineType) {
  case Fixed:
    propList.insert("fo:line-height", *(m_spacings[0]), *m_spacingsInterlineUnit);
    break;
  case AtLeast:
    if (*(m_spacings[0]) <= 0 && *(m_spacings[0]) >= 0)
      break;
    if (*(m_spacings[0]) < 0) {
      static bool first = true;
      if (first) {
        STOFF_DEBUG_MSG(("STOFFParagraph::addTo: interline spacing seems bad\n"));
        first = false;
      }
    }
    else if (*m_spacingsInterlineUnit != librevenge::RVNG_PERCENT)
      propList.insert("style:line-height-at-least", *(m_spacings[0]), *m_spacingsInterlineUnit);
    else {
      propList.insert("style:line-height-at-least", *(m_spacings[0])*12.0, librevenge::RVNG_POINT);
      static bool first = true;
      if (first) {
        first = false;
        STOFF_DEBUG_MSG(("STOFFParagraph::addTo: assume height=12 to set line spacing at least with percent type\n"));
      }
    }
    break;
  default:
    STOFF_DEBUG_MSG(("STOFFParagraph::addTo: can not set line spacing type: %d\n",int(*m_spacingsInterlineType)));
    break;
  }
  if (*m_breakStatus & NoBreakBit)
    propList.insert("fo:keep-together", "always");
  if (*m_breakStatus & NoBreakWithNextBit)
    propList.insert("fo:keep-with-next", "always");

  if (!m_tabs->empty()) {
    double decalX=0;
    librevenge::RVNGPropertyListVector tabs;
    if (!*m_tabsRelativeToLeftMargin) {
      // tabs are absolute, we must remove left margin
      double factor = (double) libstoff::getScaleFactor(*m_marginsUnit, librevenge::RVNG_INCH);
      decalX -= m_margins[1].get()*factor;
    }

    for (size_t i=0; i<m_tabs->size(); i++)
      m_tabs.get()[i].addTo(tabs, decalX);
    propList.insert("style:tab-stops", tabs);
  }
}

std::ostream &operator<<(std::ostream &o, STOFFParagraph const &pp)
{
  if (!pp.m_styleName.empty())
    o << "style=\"" << pp.m_styleName << "\",";
  if (pp.m_margins[0].get()<0||pp.m_margins[0].get()>0)
    o << "textIndent=" << pp.m_margins[0].get() << ",";
  if (pp.m_margins[1].get()<0||pp.m_margins[1].get()>0)
    o << "leftMarg=" << pp.m_margins[1].get() << ",";
  if (pp.m_margins[2].get()<0||pp.m_margins[2].get()>0)
    o << "rightMarg=" << pp.m_margins[2].get() << ",";

  if (pp.m_spacingsInterlineUnit.get()==librevenge::RVNG_PERCENT) {
    if (pp.m_spacings[0].get() < 1.0 || pp.m_spacings[0].get() > 1.0) {
      o << "interLineSpacing=" << pp.m_spacings[0].get() << "%";
      if (pp.m_spacingsInterlineType.get()==STOFFParagraph::AtLeast)
        o << "[atLeast]";
      o << ",";
    }
  }
  else if (pp.m_spacings[0].get() > 0.0) {
    o << "interLineSpacing=" << pp.m_spacings[0].get();
    if (pp.m_spacingsInterlineType.get()==STOFFParagraph::AtLeast)
      o << "[atLeast]";
    o << ",";
  }
  if (pp.m_spacings[1].get()<0||pp.m_spacings[1].get()>0)
    o << "befSpacing=" << pp.m_spacings[1].get() << ",";
  if (pp.m_spacings[2].get()<0||pp.m_spacings[2].get()>0)
    o << "aftSpacing=" << pp.m_spacings[2].get() << ",";

  if (pp.m_breakStatus.get() & STOFFParagraph::NoBreakBit) o << "dontbreak,";
  if (pp.m_breakStatus.get() & STOFFParagraph::NoBreakWithNextBit) o << "dontbreakafter,";

  switch (pp.m_justify.get()) {
  case STOFFParagraph::JustificationLeft:
    break;
  case STOFFParagraph::JustificationCenter:
    o << "just=centered, ";
    break;
  case STOFFParagraph::JustificationRight:
    o << "just=right, ";
    break;
  case STOFFParagraph::JustificationFull:
    o << "just=full, ";
    break;
  case STOFFParagraph::JustificationFullAllLines:
    o << "just=fullAllLines, ";
    break;
  default:
    o << "just=" << pp.m_justify.get() << ", ";
    break;
  }

  if (pp.m_tabs->size()) {
    o << "tabs=(";
    for (size_t i = 0; i < pp.m_tabs->size(); i++)
      o << pp.m_tabs.get()[i] << ",";
    o << "),";
  }
  if (!pp.m_backgroundColor.get().isWhite())
    o << "backgroundColor=" << pp.m_backgroundColor.get() << ",";
  if (*pp.m_listId >= 0) o << "listId=" << *pp.m_listId << ",";
  if (pp.m_listLevelIndex.get() >= 1)
    o << pp.m_listLevel.get() << ":" << pp.m_listLevelIndex.get() <<",";

  for (size_t i = 0; i < pp.m_borders.size(); i++) {
    if (!pp.m_borders[i].isSet())
      continue;
    STOFFBorder const &border = pp.m_borders[i].get();
    if (border.isEmpty())
      continue;
    o << "bord";
    if (i < 6) {
      static char const *wh[] = { "L", "R", "T", "B", "MiddleH", "MiddleV" };
      o << wh[i];
    }
    else o << "[#wh=" << i << "]";
    o << "=" << border << ",";
  }

  if (!pp.m_extra.empty()) o << "extras=(" << pp.m_extra << ")";
  return o;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
