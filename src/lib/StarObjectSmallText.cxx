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
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFFont.hxx"
#include "STOFFListener.hxx"
#include "STOFFParagraph.hxx"

#include "StarAttribute.hxx"
#include "StarObject.hxx"
#include "StarItemPool.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "SWFieldManager.hxx"

#include "StarObjectSmallText.hxx"

/** Internal: the structures of a StarObjectSmallText */
namespace StarObjectSmallTextInternal
{
////////////////////////////////////////
//! Internal: a paragraph of StarObjectSmallText
struct Paragraph {
  //! constructor
  Paragraph()
    : m_text()
    , m_textSourcePosition()
    , m_styleName()
    , m_family(0)
    , m_itemSet()
    , m_charItemList()
    , m_charLimitList()
  {
  }
  //! try to send the data to a listener
  bool send(STOFFListenerPtr &listener, StarState &mainState, StarState &editState, int level=-1) const;
  //! the text
  std::vector<uint32_t> m_text;
  //! the text initial position
  std::vector<size_t> m_textSourcePosition;
  //! the style name
  librevenge::RVNGString m_styleName;
  //! the family
  int m_family;
  //! the main item list
  StarItemSet m_itemSet;
  //! the character item list
  std::vector<std::shared_ptr<StarItem> > m_charItemList;
  //! the character limit
  std::vector<STOFFVec2i> m_charLimitList;
};

bool Paragraph::send(STOFFListenerPtr &listener, StarState &mainState, StarState &editState, int level) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("StarObjectSmallTextInternal::Paragraph::send: call without listener\n"));
    return false;
  }

  mainState.m_break=0;
  mainState.m_paragraph=STOFFParagraph();
  if (mainState.m_global->m_pool && !m_styleName.empty()) { // checkme
    auto const *style=mainState.m_global->m_pool->findStyleWithFamily(m_styleName, StarItemStyle::F_Paragraph);
    if (style) {
#if 0
      bool done=false;
      if (!style->m_names[0].empty()) {
        if (listener) mainPool->defineParagraphStyle(listener, style->m_names[0], mainState.m_object);
        para.m_propertyList.insert("librevenge:parent-display-name", style->m_names[0]);
        done=true;
      }
#endif
      for (auto it : style->m_itemSet.m_whichToItemMap) {
        if (it.second && it.second->m_attribute)
          it.second->m_attribute->addTo(mainState);
      }
#if 0
      std::cerr << "Para:" << style->m_itemSet.printChild() << "\n";
#endif
    }
  }
  editState.m_paragraph=mainState.m_paragraph;
  if (level>=0) editState.m_paragraph.m_listLevelIndex=level;
  editState.m_font=mainState.m_font;
  for (auto it : m_itemSet.m_whichToItemMap) {
    if (!it.second || !it.second->m_attribute) continue;
    it.second->m_attribute->addTo(editState);
  }
#if 0
  std::cerr << "ItemSet:" << m_itemSet.printChild() << "\n";
#endif
  STOFFFont mainFont=editState.m_font; // save font
  listener->setFont(mainFont);
  listener->setParagraph(editState.m_paragraph);

  std::set<size_t> modPosSet;
  size_t numFonts=m_charItemList.size();
  if (m_charLimitList.size()!=numFonts) {
    STOFF_DEBUG_MSG(("StarObjectSmallTextInternal::Paragraph::send: the number of fonts seems bad\n"));
    if (m_charLimitList.size()<numFonts)
      numFonts=m_charLimitList.size();
  }
  for (size_t i=0; i<numFonts; ++i) {
    modPosSet.insert(size_t(m_charLimitList[i][0]));
    modPosSet.insert(size_t(m_charLimitList[i][1]));
  }
  auto posSetIt=modPosSet.begin();
  for (size_t c=0; c<=m_text.size(); ++c) {
    bool fontChange=false;
    size_t srcPos=c<m_textSourcePosition.size() ? m_textSourcePosition[c] : m_textSourcePosition.empty() ? 0 : 10000;
    while (posSetIt!=modPosSet.end() && *posSetIt <= srcPos) {
      ++posSetIt;
      fontChange=true;
    }
    std::shared_ptr<SWFieldManagerInternal::Field> field;
    if (fontChange) {
      STOFFFont &font=editState.m_font;
      editState.reinitializeLineData();
      font=mainFont;
      for (size_t f=0; f<numFonts; ++f) {
        if (m_charLimitList[f][0]>int(srcPos) || m_charLimitList[f][1]<=int(srcPos))
          continue;
        if (!m_charItemList[f] || !m_charItemList[f]->m_attribute)
          continue;
        m_charItemList[f]->m_attribute->addTo(editState);
        if (editState.m_field) {
          if (int(srcPos)==m_charLimitList[f][0])
            field=editState.m_field;
          editState.m_field.reset();
        }
      }
      static bool first=true;
      if (first && (editState.m_content || editState.m_flyCnt || editState.m_footnote || !editState.m_link.empty() || !editState.m_refMark.empty())) {
        STOFF_DEBUG_MSG(("StarObjectSmallTextInternal::Paragraph::send: sorry, sending content/field/flyCnt/footnote/refMark/link is not implemented\n"));
        first=false;
      }
      listener->setFont(font);
      if (font.m_lineBreak) {
        listener->insertEOL(true);
        continue;
      }
      if (font.m_tab) {
        listener->insertTab();
        continue;
      }
    }
    if (field) {
      StarState cState(*editState.m_global);
      field->send(listener, cState);
    }
    else if (c==m_text.size())
      break;
    else if (m_text[c]==0x9)
      listener->insertTab();
    else if (m_text[c]==0xa)
      listener->insertEOL(true);
    else
      listener->insertUnicode(m_text[c]);
  }
  return true;
}

////////////////////////////////////////
//! Internal: the state of a StarObjectSmallText
struct State {
  //! constructor
  State()
    : m_paragraphList()
  {
  }

  //! the paragraphs
  std::vector<Paragraph> m_paragraphList;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectSmallText::StarObjectSmallText(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_textState(new StarObjectSmallTextInternal::State)
{
}

StarObjectSmallText::~StarObjectSmallText()
{
}

bool StarObjectSmallText::send(std::shared_ptr<STOFFListener> listener, int level)
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("StarObjectSmallText::send: call without listener\n"));
    return false;
  }
  // fixme: this works almost alway, but ...
  auto editPool=findItemPool(StarItemPool::T_EditEnginePool, false);
  auto mainPool=findItemPool(StarItemPool::T_XOutdevPool, false);
  StarState mainState(mainPool.get(), *this);
  StarState editState(editPool.get(), *this);
  for (size_t p=0; p<m_textState->m_paragraphList.size(); ++p) {
    m_textState->m_paragraphList[p].send(listener, mainState, editState, level);
    if (p+1!=m_textState->m_paragraphList.size())
      listener->insertEOL();
  }
  return true;
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////

bool StarObjectSmallText::read(StarZone &zone, long lastPos)
{
  // svx_editobj.cxx EditTextObject::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  f << "Entries(EditTextObject):";
  uint16_t nWhich;
  uint32_t nStructSz;
  *input >> nWhich >> nStructSz;
  f << "structSz=" << nStructSz << ",";
  f << "nWhich=" << nWhich << ",";
  if ((nWhich != 0x22 && nWhich !=0x31) || pos+6+long(nStructSz)>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallText::read: bad which/structSz\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  lastPos=pos+6+long(nStructSz);
  // BinTextObject::CreateData or BinTextObject::CreateData300
  uint16_t version=0;
  bool ownPool=true;
  if (nWhich==0x31) {
    *input>>version >> ownPool;
    if (!ownPool) f << "mainPool,";
    if (version) f << "vers=" << version << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  std::shared_ptr<StarItemPool> pool;
  if (!ownPool) {
    pool=findItemPool(StarItemPool::T_EditEnginePool, false);
    if (!pool) {
      f << "###noPool,";
      STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not find the main pool\n"));
    }
  }
  if (!pool) pool=getNewItemPool(StarItemPool::T_EditEnginePool);
  if (ownPool && !pool->read(zone)) {
    STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read a pool\n"));
    ascFile.addPos(pos);
    ascFile.addNote("EditTextObject:###pool");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  pos=input->tell();
  f.str("");
  f << "EditTextObject:";
  uint16_t charSet=0;
  uint32_t nPara;
  if (nWhich==0x31) {
    uint16_t numPara;
    *input>>charSet >> numPara;
    nPara=numPara;
  }
  else
    *input>> nPara;
  if (charSet)  f << "char[set]=" << charSet << ",";
  if (nPara) f << "nPara=" << nPara << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (size_t i=0; i<size_t(nPara); ++i) {
    if (i>=m_textState->m_paragraphList.size())
      m_textState->m_paragraphList.resize(i+10>size_t(nPara) ? size_t(nPara) : i+10);
    auto &para=m_textState->m_paragraphList[i];
    pos=input->tell();
    f.str("");
    f << "EditTextObject[para" << i << "]:";
    for (int j=0; j<2; ++j) {
      std::vector<uint32_t> text;
      std::vector<size_t> positions;
      if (!zone.readString(text, positions, int(charSet)) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read a strings\n"));
        f << "###strings,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
        return true;
      }
      else if (!text.empty())
        f << (j==0 ? "text" : "style") << "=" << libstoff::getString(text).cstr() << ",";
      if (j==0) {
        para.m_text=text;
        para.m_textSourcePosition=positions;
      }
      else
        para.m_styleName=libstoff::getString(text);
    }
    uint16_t styleFamily;
    *input >> styleFamily;
    if (styleFamily) f << "styleFam=" << styleFamily << ",";
    para.m_family=int(styleFamily);
    std::vector<STOFFVec2i> limits;
    limits.push_back(STOFFVec2i(3989, 4033)); // EE_PARA_START 4033: EE_CHAR_END
    if (!readItemSet(zone, limits, lastPos, para.m_itemSet, pool.get(), false) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read item list\n"));
      f << "###item list,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "EditTextObjectA[para" << i << "]:";
    uint32_t nAttr;
    if (nWhich==0x22)
      *input>>nAttr;
    else {
      uint16_t nAttr16;
      *input>>nAttr16;
      nAttr=nAttr16;
    }
    if (lastPos-input->tell()<long(nAttr)*8 || input->tell()+long(nAttr)*8 > lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read attrib list\n"));
      f << "###attrib list,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    f << "attrib=[";
    for (int j=0; j<int(nAttr); ++j) { // checkme, probably bad
      uint16_t which, start, end;
      *input >> which;
      std::shared_ptr<StarItem> item=pool->loadSurrogate(zone, which, true, f);
      if (!item) {
        STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not find an item\n"));
        f << "###attrib,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
        return true;
      }
      *input >> start >> end;
      f << "wh=" << which << ":";
      f << "pos=" << start << "x" << end;
      para.m_charItemList.push_back(item);
      para.m_charLimitList.push_back(STOFFVec2i(int(start), int(end)));
      if (nWhich==0x31 && which==4036) // checkme: we must not convert this character...
        f << "notConv,";
      else if (item->m_attribute) {
        f << "[";
        item->m_attribute->printData(f);
        f << "]";
      }
      f << ",";
    }
    f << "],";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }

  pos=input->tell();
  f.str("");
  f << "EditTextObject:";
  if (nWhich==0x22) {
    uint16_t marker;
    *input >> marker;
    if (marker==0x9999) {
      *input >> charSet;
      if (charSet)  f << "char[set]=" << charSet << ",";
    }
  }
  if (version>=400)
    f << "tmpMetric=" << input->readULong(2) << ",";
  if (version>=600) {
    f << "userType=" << input->readULong(2) << ",";
    f << "objSettings=" << input->readULong(4) << ",";
  }
  if (version>=601)
    f << "vertical=" << input->readULong(1) << ",";
  if (version>=602) {
    f << "scriptType=" << input->readULong(2) << ",";
    bool unicodeString;
    *input >> unicodeString;
    f << "strings=[";
    for (int i=0; unicodeString && i<int(nPara); ++i) {
      for (int s=0; s<2; ++s) {
        std::vector<uint32_t> text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read a strings\n"));
          f << "###strings,";
          break;
        }
        if (text.empty())
          f << "_,";
        else
          f << libstoff::getString(text).cstr() << ",";
      }
    }
    f << "],";
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallText::read: find extra data\n"));
    f << "###";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
