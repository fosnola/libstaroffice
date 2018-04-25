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

#include "STOFFFrameStyle.hxx"
#include "STOFFGraphicListener.hxx"
#include "STOFFOLEParser.hxx"
#include "STOFFSubDocument.hxx"

#include "StarBitmap.hxx"
#include "StarZone.hxx"

#include "SDGParser.hxx"

/** Internal: the structures of a SDGParser */
namespace SDGParserInternal
{

////////////////////////////////////////
//! Internal: small class use to store an image content in a SDGParser
class Image
{
public:
  //! constructor
  Image()
    : m_object()
    , m_size()
    , m_link()
  {
  }
  //! the object
  STOFFEmbeddedObject m_object;
  //! the bitmap size
  STOFFVec2i m_size;
  //! the link name
  librevenge::RVNGString m_link;
};

////////////////////////////////////////
//! Internal: the state of a SDGParser
struct State {
  //! constructor
  State()
    : m_imagesList()
  {
  }

  //! the list of image
  std::vector<Image> m_imagesList;
};

////////////////////////////////////////
//! Internal: the subdocument of a SDGParser
class SubDocument final : public STOFFSubDocument
{
public:
  explicit SubDocument(librevenge::RVNGString const &text)
    : STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry())
    , m_text(text) {}

  //! destructor
  ~SubDocument() final {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const final
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    auto const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_text != sDoc->m_text) return true;
    return false;
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;

protected:
  //! the text
  librevenge::RVNGString m_text;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (m_text.empty())
    listener->insertChar(' ');
  else
    listener->insertUnicodeString(m_text);
}
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDGParser::SDGParser(STOFFInputStreamPtr &input, STOFFHeader *header)
  : STOFFGraphicParser(input, header)
  , m_password(nullptr)
  , m_state(new SDGParserInternal::State)
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
  if (!getInput().get() || !checkHeader(nullptr))  throw(libstoff::ParseException());
  bool ok = true;
  try {
    checkHeader(nullptr);
    ok = createZones();
    if (ok) {
      createDocument(docInterface);
      STOFFListenerPtr listener=getGraphicListener();
      if (listener) {
        STOFFFrameStyle frame;
        auto &position=frame.m_position;
        position.setAnchor(STOFFPosition::Page);
        STOFFGraphicStyle style;
        style.m_propertyList.insert("draw:stroke", "none");
        style.m_propertyList.insert("draw:fill", "none");
        bool first=true;
        for (auto const &image : m_state->m_imagesList) {
          if (image.m_object.isEmpty())
            continue;
          if (!first)
            listener->insertBreak(STOFFListener::PageBreak);
          else
            first=false;
          position.setOrigin(STOFFVec2f(20,20));
          STOFFVec2f size=(image.m_size[0]>0 && image.m_size[1]>0) ? STOFFVec2f(image.m_size) : STOFFVec2f(400,400);
          position.setSize(size);
          listener->insertPicture(frame, image.m_object, style);
          if (!image.m_link.empty()) {
            std::shared_ptr<SDGParserInternal::SubDocument> doc(new SDGParserInternal::SubDocument(image.m_link));
            position.setOrigin(STOFFVec2f(20,30+size[1]));
            position.setSize(STOFFVec2f(600,200));
            listener->insertTextBox(frame, doc, style);
          }
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

  input->seek(0, librevenge::RVNG_SEEK_SET);
  long pos=input->tell();
  while (!input->isEnd() && readSGA3(zone))
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

  auto numImages=int(m_state->m_imagesList.size());
  std::vector<STOFFPageSpan> pageList;
  STOFFPageSpan ps(getPageSpan());
  ps.m_pageSpan=numImages ? numImages : 1;
  pageList.push_back(ps);
  STOFFGraphicListenerPtr listen(new STOFFGraphicListener(getParserState()->m_listManager, pageList, documentInterface));
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
    long val=int(input->readULong(4));
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
    STOFF_DEBUG_MSG(("SDGParser::readSGA3: can not find the header\n"));
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
    auto val=int(input->readULong(2));
    int const expected[]= {4,5,1};
    if (val!=expected[i]) f << "f" << i << "=" << val << ",";
  }
  SDGParserInternal::Image image;
  int val;
  for (int step=0; step<2; ++step) {
    auto type=int(input->readULong(1));
    if (type>2) {
      input->seek(-1, librevenge::RVNG_SEEK_CUR);
      break;
    }
    f << "type=" << type << ",";
    if (type) {
      StarBitmap bitmap;
      librevenge::RVNGBinaryData data;
      std::string bType;
      val=int(input->readULong(2));
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
        image.m_object.add(data, bType);
        image.m_size=bitmap.getBitmapSize();
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      f.str("");
      f << "SGA3:";
    }

    val=int(input->readULong(2));
    bool findText=false;
    if (val==0x1962) {
      f << "empty,";
      for (int i=0; i<3; ++i) {
        val=int(input->readULong(2));
        int const expected[]= {0x2509, 0x201, 0xacb2};
        if (val!=expected[i])
          f << "f" << i << "=" << std::hex << val << std::dec << ",";
      }
    }
    else {
      input->seek(-2, librevenge::RVNG_SEEK_CUR);
      for (int i=0; i<2; ++i) {
        std::vector<uint32_t> text;
        val=int(input->readULong(2));
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
        else if (!text.empty()) {
          if (i==0)
            image.m_link=libstoff::getString(text);
          f << "text" << i << "=" << libstoff::getString(text).cstr() << ",";
        }
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
  if (!image.m_object.isEmpty())
    m_state->m_imagesList.push_back(image);
  if (input->checkPosition(input->tell()+2)) {
    long actPos=input->tell();
    val=int(input->readULong(2));
    if (val==0x4753 || val==0x5300)
      input->seek(actPos, librevenge::RVNG_SEEK_SET);
    else {
      f << "val=" << std::hex << val << std::dec << ",";
      val=int(input->readULong(2));
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
  return true;
}


// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
