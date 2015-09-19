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
#include <iostream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFList.hxx"

////////////////////////////////////////////////////////////
// list level functions
////////////////////////////////////////////////////////////
void STOFFListLevel::addTo(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("text:min-label-width", m_labelWidth, librevenge::RVNG_INCH);
  propList.insert("text:space-before", m_labelBeforeSpace, librevenge::RVNG_INCH);
  if (m_labelAfterSpace > 0)
    propList.insert("text:min-label-distance", m_labelAfterSpace, librevenge::RVNG_INCH);
  if (m_numBeforeLabels)
    propList.insert("text:display-levels", m_numBeforeLabels+1);
  switch (m_alignment) {
  case LEFT:
    break;
  case CENTER:
    propList.insert("fo:text-align", "center");
    break;
  case RIGHT:
    propList.insert("fo:text-align", "end");
    break;
  default:
    break;
  }

  switch (m_type) {
  case NONE:
    propList.insert("text:bullet-char", " ");
    break;
  case BULLET:
    if (m_bullet.len())
      propList.insert("text:bullet-char", m_bullet.cstr());
    else {
      STOFF_DEBUG_MSG(("STOFFListLevel::addTo: the bullet char is not defined\n"));
      propList.insert("text:bullet-char", "*");
    }
    break;
  case LABEL:
    if (m_label.len())
      propList.insert("style:num-suffix", librevenge::RVNGPropertyFactory::newStringProp(m_label));
    propList.insert("style:num-format", "");
    break;
  case DECIMAL:
  case LOWER_ALPHA:
  case UPPER_ALPHA:
  case LOWER_ROMAN:
  case UPPER_ROMAN:
    if (m_prefix.len())
      propList.insert("style:num-prefix",librevenge::RVNGPropertyFactory::newStringProp(m_prefix));
    if (m_suffix.len())
      propList.insert("style:num-suffix", librevenge::RVNGPropertyFactory::newStringProp(m_suffix));
    if (m_type==DECIMAL) propList.insert("style:num-format", "1");
    else if (m_type==LOWER_ALPHA) propList.insert("style:num-format", "a");
    else if (m_type==UPPER_ALPHA) propList.insert("style:num-format", "A");
    else if (m_type==LOWER_ROMAN) propList.insert("style:num-format", "i");
    else propList.insert("style:num-format", "I");
    propList.insert("text:start-value", getStartValue());
    break;
  case DEFAULT:
  default:
    STOFF_DEBUG_MSG(("STOFFListLevel::addTo: the level type is not set\n"));
  }
}

int STOFFListLevel::cmp(STOFFListLevel const &levl) const
{
  int diff = int(m_type)-int(levl.m_type);
  if (diff) return diff;
  double fDiff = m_labelBeforeSpace-levl.m_labelBeforeSpace;
  if (fDiff < 0.0) return -1;
  if (fDiff > 0.0) return 1;
  fDiff = m_labelWidth-levl.m_labelWidth;
  if (fDiff < 0.0) return -1;
  if (fDiff > 0.0) return 1;
  diff = int(m_alignment)-levl.m_alignment;
  if (diff) return diff;
  fDiff = m_labelAfterSpace-levl.m_labelAfterSpace;
  if (fDiff < 0.0) return -1;
  if (fDiff > 0.0) return 1;
  diff = m_numBeforeLabels-levl.m_numBeforeLabels;
  if (diff) return diff;
  diff = strcmp(m_label.cstr(),levl.m_label.cstr());
  if (diff) return diff;
  diff = strcmp(m_prefix.cstr(),levl.m_prefix.cstr());
  if (diff) return diff;
  diff = strcmp(m_suffix.cstr(),levl.m_suffix.cstr());
  if (diff) return diff;
  diff = strcmp(m_bullet.cstr(),levl.m_bullet.cstr());
  if (diff) return diff;
  return 0;
}

std::ostream &operator<<(std::ostream &o, STOFFListLevel const &level)
{
  o << "ListLevel[";
  switch (level.m_type) {
  case STOFFListLevel::BULLET:
    o << "bullet='" << level.m_bullet.cstr() <<"'";
    break;
  case STOFFListLevel::LABEL:
    o << "text='" << level.m_label.cstr() << "'";
    break;
  case STOFFListLevel::DECIMAL:
    o << "decimal";
    break;
  case STOFFListLevel::LOWER_ALPHA:
    o << "alpha";
    break;
  case STOFFListLevel::UPPER_ALPHA:
    o << "ALPHA";
    break;
  case STOFFListLevel::LOWER_ROMAN:
    o << "roman";
    break;
  case STOFFListLevel::UPPER_ROMAN:
    o << "ROMAN";
    break;
  case STOFFListLevel::NONE:
    break;
  case STOFFListLevel::DEFAULT:
  default:
    o << "####type";
  }
  switch (level.m_alignment) {
  case STOFFListLevel::LEFT:
    break;
  case STOFFListLevel::CENTER:
    o << ",center";
    break;
  case STOFFListLevel::RIGHT:
    o << ",right";
    break;
  default:
    o << "###align=" << int(level.m_alignment) << ",";
  }
  if (level.m_type != STOFFListLevel::BULLET && level.m_startValue)
    o << ",startVal= " << level.m_startValue;
  if (level.m_prefix.len()) o << ", prefix='" << level.m_prefix.cstr()<<"'";
  if (level.m_suffix.len()) o << ", suffix='" << level.m_suffix.cstr()<<"'";
  if (level.m_labelBeforeSpace < 0 || level.m_labelBeforeSpace > 0) o << ", indent=" << level.m_labelBeforeSpace;
  if (level.m_labelWidth < 0 || level.m_labelWidth > 0) o << ", width=" << level.m_labelWidth;
  if (level.m_labelAfterSpace > 0) o << ", labelTextW=" << level.m_labelAfterSpace;
  if (level.m_numBeforeLabels) o << ", show=" << level.m_numBeforeLabels << "[before]";
  o << "]";
  if (level.m_extra.length()) o << ", " << level.m_extra;
  return o;
}

////////////////////////////////////////////////////////////
// list functions
////////////////////////////////////////////////////////////
void STOFFList::resize(int level)
{
  if (level < 0) {
    STOFF_DEBUG_MSG(("STOFFList::resize: level %d can not be negatif\n",level));
    return;
  }
  if (level == int(m_levels.size()))
    return;
  m_levels.resize(size_t(level));
  m_actualIndices.resize(size_t(level), 0);
  m_nextIndices.resize(size_t(level), 1);
  if (m_actLevel >= level)
    m_actLevel=level-1;
  m_modifyMarker++;
}

void STOFFList::updateIndicesFrom(STOFFList const &list)
{
  size_t maxLevel=list.m_levels.size();
  if (maxLevel>m_levels.size())
    maxLevel=m_levels.size();
  for (size_t level=0 ; level < maxLevel; level++) {
    m_actualIndices[size_t(level)]=m_levels[size_t(level)].getStartValue()-1;
    m_nextIndices[level]=list.m_nextIndices[level];
  }
  m_modifyMarker++;
}

bool STOFFList::isCompatibleWith(int levl, STOFFListLevel const &level) const
{
  if (levl < 1) {
    STOFF_DEBUG_MSG(("STOFFList::isCompatibleWith: level %d must be positive\n",levl));
    return false;
  }

  return levl > int(m_levels.size()) ||
         level.cmp(m_levels[size_t(levl-1)])==0;
}

bool STOFFList::isCompatibleWith(STOFFList const &newList) const
{
  size_t maxLevel=newList.m_levels.size();
  if (maxLevel>m_levels.size())
    maxLevel=m_levels.size();
  for (size_t level=0 ; level < maxLevel; level++) {
    if (m_levels[level].cmp(newList.m_levels[level])!=0)
      return false;
  }
  return true;
}

void STOFFList::setId(int newId) const
{
  m_id[0] = newId;
  m_id[1] = newId+1;
}

bool STOFFList::addTo(int level, librevenge::RVNGPropertyList &pList) const
{
  if (level <= 0 || level > int(m_levels.size()) ||
      m_levels[size_t(level-1)].isDefault()) {
    STOFF_DEBUG_MSG(("STOFFList::addTo: level %d is not defined\n",level));
    return false;
  }

  if (getId()==-1) {
    STOFF_DEBUG_MSG(("STOFFList::addTo: the list id is not set\n"));
    static int falseId = 1000;
    setId(falseId+=2);
  }
  pList.insert("librevenge:list-id", getId());
  pList.insert("librevenge:level", level);
  m_levels[size_t(level-1)].addTo(pList);
  return true;
}

void STOFFList::set(int levl, STOFFListLevel const &level)
{
  if (levl < 1) {
    STOFF_DEBUG_MSG(("STOFFList::set: can not set level %d\n",levl));
    return;
  }
  if (levl > int(m_levels.size())) resize(levl);
  int needReplace = m_levels[size_t(levl-1)].cmp(level) != 0 ||
                    (level.m_startValue && m_nextIndices[size_t(levl-1)] !=level.getStartValue());
  if (level.m_startValue > 0 || level.m_type != m_levels[size_t(levl-1)].m_type) {
    m_nextIndices[size_t(levl-1)]=level.getStartValue();
    m_modifyMarker++;
  }
  if (needReplace) {
    m_levels[size_t(levl-1)] = level;
    m_modifyMarker++;
  }
}

void STOFFList::setLevel(int levl) const
{
  if (levl < 1 || levl > int(m_levels.size())) {
    STOFF_DEBUG_MSG(("STOFFList::setLevel: can not set level %d\n",levl));
    return;
  }

  if (levl < int(m_levels.size()))
    m_actualIndices[size_t(levl)]=
      (m_nextIndices[size_t(levl)]=m_levels[size_t(levl)].getStartValue())-1;

  m_actLevel=levl-1;
}

void STOFFList::setStartValueForNextElement(int value)
{
  if (m_actLevel < 0 || m_actLevel >= int(m_levels.size())) {
    STOFF_DEBUG_MSG(("STOFFList::setStartValueForNextElement: can not find level %d\n",m_actLevel));
    return;
  }
  if (m_nextIndices[size_t(m_actLevel)]==value)
    return;
  m_nextIndices[size_t(m_actLevel)]=value;
  m_modifyMarker++;
}

int STOFFList::getStartValueForNextElement() const
{
  if (m_actLevel < 0 || m_actLevel >= int(m_levels.size())) {
    STOFF_DEBUG_MSG(("STOFFList::getStartValueForNextElement: can not find level %d\n",m_actLevel));
    return -1;
  }
  if (!m_levels[size_t(m_actLevel)].isNumeric())
    return -1;
  return m_nextIndices[size_t(m_actLevel)];
}

void STOFFList::openElement() const
{
  if (m_actLevel < 0 || m_actLevel >= int(m_levels.size())) {
    STOFF_DEBUG_MSG(("STOFFList::openElement: can not set level %d\n",m_actLevel));
    return;
  }
  if (m_levels[size_t(m_actLevel)].isNumeric())
    m_actualIndices[size_t(m_actLevel)]=m_nextIndices[size_t(m_actLevel)]++;
}

bool STOFFList::isNumeric(int levl) const
{
  if (levl < 1 || levl > int(m_levels.size())) {
    STOFF_DEBUG_MSG(("STOFFList::isNumeric: the level does not exist\n"));
    return false;
  }

  return m_levels[size_t(levl-1)].isNumeric();
}

////////////////////////////////////////////////////////////
// list manager function
////////////////////////////////////////////////////////////
bool STOFFListManager::needToSend(int index, std::vector<int> &idMarkerList) const
{
  if (index <= 0) return false;
  if (index >= int(idMarkerList.size()))
    idMarkerList.resize(size_t(index)+1,0);
  size_t mainId=size_t(index-1)/2;
  if (mainId >= m_listList.size()) {
    STOFF_DEBUG_MSG(("STOFFListManager::needToSend: can not find list %d\n", index));
    return false;
  }
  STOFFList const &list=m_listList[mainId];
  if (idMarkerList[size_t(index)] == list.getMarker())
    return false;
  idMarkerList[size_t(index)]=list.getMarker();
  if (list.getId()!=index) list.swapId();
  return true;
}

shared_ptr<STOFFList> STOFFListManager::getList(int index) const
{
  shared_ptr<STOFFList> res;
  if (index <= 0) return res;
  size_t mainId=size_t(index-1)/2;
  if (mainId < m_listList.size()) {
    res.reset(new STOFFList(m_listList[mainId]));
    if (res->getId()!=index)
      res->swapId();
  }
  return res;
}

shared_ptr<STOFFList> STOFFListManager::getNewList(shared_ptr<STOFFList> actList, int levl, STOFFListLevel const &level)
{
  if (actList && actList->getId()>=0 && actList->isCompatibleWith(levl, level)) {
    actList->set(levl, level);
    int index=actList->getId();
    size_t mainId=size_t(index-1)/2;
    if (mainId < m_listList.size() && m_listList[mainId].numLevels() < levl)
      m_listList[mainId].set(levl, level);
    return actList;
  }
  STOFFList res;
  if (actList) {
    res = *actList;
    res.resize(levl);
  }
  size_t numList=m_listList.size();
  res.setId(int(2*numList+1));
  res.set(levl, level);
  for (size_t l=0; l < numList; l++) {
    if (!m_listList[l].isCompatibleWith(res))
      continue;
    if (m_listList[l].numLevels() < levl)
      m_listList[l].set(levl, level);
    shared_ptr<STOFFList> copy(new STOFFList(m_listList[l]));
    copy->updateIndicesFrom(res);
    return copy;
  }
  m_listList.push_back(res);
  return shared_ptr<STOFFList>(new STOFFList(res));
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
