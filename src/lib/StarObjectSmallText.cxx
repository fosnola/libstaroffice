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

#include "STOFFListener.hxx"
#include "STOFFOLEParser.hxx"

#include "StarAttribute.hxx"
#include "StarObject.hxx"
#include "StarItemPool.hxx"
#include "StarZone.hxx"

#include "StarObjectSmallText.hxx"

/** Internal: the structures of a StarObjectSmallText */
namespace StarObjectSmallTextInternal
{
////////////////////////////////////////
//! Internal: the state of a StarObjectSmallText
struct State {
  //! constructor
  State() : m_text()
  {
  }

  //! the text
  std::vector<uint32_t> m_text;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectSmallText::StarObjectSmallText(StarObject const &orig, bool duplicateState) : StarObject(orig, duplicateState), m_state(new StarObjectSmallTextInternal::State)
{
}

StarObjectSmallText::~StarObjectSmallText()
{
}

bool StarObjectSmallText::send(shared_ptr<STOFFListener> listener)
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("StarObjectSmallText::send: call without listener\n"));
    return false;
  }
  listener->insertUnicodeList(m_state->m_text);
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
    if (version) f << "vers=" << version << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  shared_ptr<StarItemPool> pool;
  if (!ownPool) pool=findItemPool(StarItemPool::T_EditEnginePool, true); // checkme
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

  for (int i=0; i<int(nPara); ++i) {
    pos=input->tell();
    f.str("");
    f << "EditTextObject[para" << i << "]:";
    for (int j=0; j<2; ++j) {
      std::vector<uint32_t> text;
      if (!zone.readString(text, int(charSet)) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read a strings\n"));
        f << "###strings,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        input->seek(lastPos, librevenge::RVNG_SEEK_SET);
        return true;
      }
      else if (!text.empty())
        f << (j==0 ? "name" : "style") << "=" << libstoff::getString(text).cstr() << ",";
      if (j==0)
        m_state->m_text=text;
    }
    uint16_t styleFamily;
    *input >> styleFamily;
    if (styleFamily) f << "styleFam=" << styleFamily << ",";
    std::vector<STOFFVec2i> limits;
    limits.push_back(STOFFVec2i(3989, 4033)); // EE_PARA_START 4033: EE_CHAR_END
    if (!readItemSet(zone, limits, lastPos, pool.get(), false)) {
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
    if (input->tell()+long(nAttr)*8 > lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallText::read: can not read attrib list\n"));
      f << "###attrib list,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    f << "attrib=[";
    for (int j=0; j<int(nAttr); ++j) { // checkme, probably bad
      uint16_t which, start, end, surrogate;
      *input >> which;
      if (nWhich==0x22) *input >> surrogate;
      *input >> start >> end;
      if (nWhich!=0x22) *input >> surrogate;
      f << "wh=" << which << ":";
      f << "pos=" << start << "x" << end << ",";
      f << "surrogate=" << surrogate << ",";
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
