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

#include "STOFFGraphicListener.hxx"
#include "STOFFOLEParser.hxx"

#include "StarBitmap.hxx"
#include "StarZone.hxx"

#include "SDGParser.hxx"

/** Internal: the structures of a SDGParser */
namespace SDGParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDGParser
struct State {
  //! constructor
  State() : m_imagesList(), m_sizesList()
  {
  }

  //! the list of image
  std::vector<STOFFEmbeddedObject> m_imagesList;
  //! the list of size
  std::vector<STOFFVec2i> m_sizesList;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDGParser::SDGParser(STOFFInputStreamPtr input, STOFFHeader *header) :
  STOFFGraphicParser(input, header), m_password(0), m_state(new SDGParserInternal::State)
{
}

SDGParser::~SDGParser()
{
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
void SDGParser::parse(librevenge::RVNGDrawingInterface *docInterface)
{
  if (!getInput().get() || !checkHeader(0L))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    checkHeader(0L);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      STOFFListenerPtr listener=getGraphicListener();
      if (listener) {
        // CHECKME: better if we have the bitmap size
        STOFFPosition position;
        position.setOrigin(STOFFVec2f(20,20), librevenge::RVNG_POINT);
        position.setSize(STOFFVec2f(400,400), librevenge::RVNG_POINT);
        position.m_propertyList.insert("text:anchor-type", "page");
        STOFFGraphicStyle style;
        style.m_propertyList.insert("draw:stroke", "none");
        style.m_propertyList.insert("draw:fill", "none");
        bool useSize=m_state->m_sizesList.size()==m_state->m_imagesList.size();
        for (size_t i=0; i<m_state->m_imagesList.size(); ++i) {
          if (i) listener->insertBreak(STOFFListener::PageBreak);
          if (m_state->m_imagesList[i].isEmpty())
            continue;
          if (useSize)
            position.setSize(m_state->m_sizesList[i], librevenge::RVNG_POINT);
          listener->insertPicture(position, m_state->m_imagesList[i], style);
        }
      }
    }
    ascii().reset();
  }
  catch (...) {
    STOFF_DEBUG_MSG(("SDGParser::parse: exception catched when parsing\n"));
    ok = false;
  }

  resetGraphicListener();
  if (!ok) throw(libstoff::ParseException());
}


bool SDGParser::createZones()
{
  STOFFInputStreamPtr input=getInput();
  if (!input)
    return false;
  StarZone zone(input, "SDGDoc", "SDGDocument", m_password); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open("main-1");

  ascFile.addPos(0);
  ascFile.addNote("FileHeader");

  libstoff::DebugStream f;
  input->seek(0, librevenge::RVNG_SEEK_SET);
  long pos=input->tell();
  while (readSGA3(zone))
    pos=input->tell();
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  pos=input->tell();
  ascFile.addPos(pos);
  ascFile.addNote("SGA3:##extra");
  return !m_state->m_imagesList.empty();
}

////////////////////////////////////////////////////////////
// create the document and send data
////////////////////////////////////////////////////////////
void SDGParser::createDocument(librevenge::RVNGDrawingInterface *documentInterface)
{
  if (!documentInterface) return;

  int numImages=(int) m_state->m_imagesList.size();
  std::vector<STOFFPageSpan> pageList;
  STOFFPageSpan ps(getPageSpan());
  ps.m_pageSpan=numImages ? numImages : 1;
  pageList.push_back(ps);
  STOFFGraphicListenerPtr listen(new STOFFGraphicListener(*getParserState(), pageList, documentInterface));
  setGraphicListener(listen);

  listen->startDocument();
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////
bool SDGParser::readSGA3(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  if (!input || input->isEnd())
    return false;
  long pos=input->tell();
  // look for 53474133
  bool findHeader=false;
  while (true) {
    if (!input->checkPosition(input->tell()+10)) {
      findHeader=false;
      break;
    }
    long val=(int) input->readULong(4);
    if (val==0x33414753) {
      findHeader=true;
      break;
    }
    if ((val>>8)==0x414753)
      input->seek(-3, librevenge::RVNG_SEEK_CUR);
    else if ((val>>16)==0x4753)
      input->seek(-2, librevenge::RVNG_SEEK_CUR);
    else if ((val>>24)==0x47)
      input->seek(-1, librevenge::RVNG_SEEK_CUR);
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SGA3):";
  if (!findHeader) {
    STOFF_DEBUG_MSG(("SDGParser::readSGA3: can not find header\n"));
    f << "###header";
  }
  else if (input->tell()!=pos+4) {
    STOFF_DEBUG_MSG(("SDGParser::readSGA3: find unknown header\n"));
    ascFile.addPos(pos);
    ascFile.addNote("Entries(SGA3):###unknown");
    pos=input->tell()-4;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (findHeader)
    readBitmap(zone);
  return findHeader;
}

bool SDGParser::readBitmap(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  if (!input)
    return false;
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  if (!input->checkPosition(pos+7)) return false;
  f << "Entries(SGA3):";
  for (int i=0; i<3; ++i) { // f0=4, f1=5|6, f2=1|2|5
    int val=(int) input->readULong(2);
    int const expected[]= {4,5,1};
    if (val!=expected[i]) f << "f" << i << "=" << val << ",";
  }
  int val;
  for (int step=0; step<2; ++step) {
    int type=(int) input->readULong(1);
    if (type>2) {
      input->seek(-1, librevenge::RVNG_SEEK_CUR);
      break;
    }
    f << "type=" << type << ",";
    if (type) {
      StarBitmap bitmap;
      librevenge::RVNGBinaryData data;
      std::string bType;
      val=(int) input->readULong(2);
      input->seek(-2, librevenge::RVNG_SEEK_CUR);
      if (val!=0x4D42 || !bitmap.readBitmap(zone, true, input->size(), data, bType)) {
        STOFF_DEBUG_MSG(("SDGParser::readBitmap: sorry, can not read a bitmap\n"));
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      else if (step==0 && bitmap.getData(data, bType)) {
        m_state->m_imagesList.push_back(STOFFEmbeddedObject(data, bType));
        m_state->m_sizesList.push_back(bitmap.getBitmapSize());
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      f.str("");
      f << "SGA3:";
    }

    val=(int) input->readULong(2);
    bool findText=false;
    if (val==0x1962) {
      f << "empty,";
      for (int i=0; i<3; ++i) {
        val=(int) input->readULong(2);
        int const(expected[])= {0x2509, 0x201, 0xacb2};
        if (val!=expected[i])
          f << "f" << i << "=" << std::hex << val << std::dec << ",";
      }
    }
    else {
      input->seek(-2, librevenge::RVNG_SEEK_CUR);
      for (int i=0; i<2; ++i) {
        std::vector<uint32_t> text;
        val=(int) input->readULong(2);
        if (val==0x5300 || (type==0 && val>=0x80)) {
          input->seek(-2, librevenge::RVNG_SEEK_CUR);
          break;
        }
        if (val>=0x80) {
          f << "val" << i << "=" << std::hex << val << std::dec << ",";
          continue;
        }
        input->seek(-2, librevenge::RVNG_SEEK_CUR);
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("SDGParser::readBitmap: sorry, can not read a text zone\n"));
          input->seek(pos, librevenge::RVNG_SEEK_SET);
          f << "###";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          return false;
        }
        else if (!text.empty())
          f << "text" << i << "=" << libstoff::getString(text).cstr() << ",";
        findText=true;
      }
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    pos=input->tell();
    f.str("");
    f << "SGA3:";
    if (findText) break;
  }
  if (input->checkPosition(input->tell()+2)) {
    long actPos=input->tell();
    val=(int) input->readULong(2);
    if (val==0x4753 || val==0x5300)
      input->seek(actPos, librevenge::RVNG_SEEK_SET);
    else {
      f << "val=" << std::hex << val << std::dec << ",";
      val=(int) input->readULong(2);
      if (val==0x4753)
        input->seek(actPos+2, librevenge::RVNG_SEEK_SET);
      else
        input->seek(actPos+12, librevenge::RVNG_SEEK_SET);
    }
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  return true;
}
////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// read the header
////////////////////////////////////////////////////////////
bool SDGParser::checkHeader(STOFFHeader *header, bool /*strict*/)
{
  *m_state = SDGParserInternal::State();

  STOFFInputStreamPtr input = getInput();
  input->setReadInverted(true);
  if (!input || !input->hasDataFork() || input->isStructured() || input->size()<30)
    return false;
  input->seek(0, librevenge::RVNG_SEEK_SET);
  if (input->readULong(4)!=0x33414753) // SGA3
    return false;
  if (header)
    header->reset(1, STOFFDocument::STOFF_K_GRAPHIC);
#ifdef DEBUG
  return true;
#else
  return false;
#endif
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
