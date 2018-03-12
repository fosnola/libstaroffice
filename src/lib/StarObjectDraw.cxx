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

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarEncryption.hxx"
#include "StarGraphicStruct.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObjectModel.hxx"
#include "StarObjectSmallGraphic.hxx"
#include "StarZone.hxx"

#include "StarObjectDraw.hxx"

/** Internal: the structures of a StarObjectDraw */
namespace StarObjectDrawInternal
{
////////////////////////////////////////
//! Internal: the state of a StarObjectDraw
struct State {
  //! constructor
  State()
    : m_model()
    , m_numPages()
  {
  }
  //! the model
  std::shared_ptr<StarObjectModel> m_model;
  //! the list of pages number
  int m_numPages;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectDraw::StarObjectDraw(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_drawState(new StarObjectDrawInternal::State)
{
}

StarObjectDraw::~StarObjectDraw()
{
  cleanPools();
}

////////////////////////////////////////////////////////////
//
// send data
//
////////////////////////////////////////////////////////////

bool StarObjectDraw::updatePageSpans(std::vector<STOFFPageSpan> &pageSpan, int &numPages) const
{
  if (!m_drawState->m_model)
    return false;

  if (!m_drawState->m_model->updatePageSpans(pageSpan, numPages))
    return false;
  m_drawState->m_numPages=numPages;
  return numPages>0;
}

bool StarObjectDraw::sendMasterPages(STOFFGraphicListenerPtr listener)
{
  if (!m_drawState->m_model)
    return false;
  return m_drawState->m_model->sendMasterPages(listener);
}

bool StarObjectDraw::sendPages(STOFFGraphicListenerPtr listener)
{
  if (!m_drawState->m_model)
    return false;
  return m_drawState->m_model->sendPages(listener);
}

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool StarObjectDraw::parse()
{
  if (!getOLEDirectory() || !getOLEDirectory()->m_input) {
    STOFF_DEBUG_MSG(("StarObjectDraw::parser: error, incomplete document\n"));
    return false;
  }
  auto &directory=*getOLEDirectory();
  StarObject::parse();
  auto unparsedOLEs=directory.getUnparsedOles();
  STOFFInputStreamPtr input=directory.m_input;

  STOFFInputStreamPtr mainOle; // let store the StarDrawDocument to read it in last position
  std::string mainName;
  for (auto const &name : unparsedOLEs) {
    STOFFInputStreamPtr ole = input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::parse: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    auto pos = name.find_last_of('/');
    std::string base;
    if (pos == std::string::npos) base = name;
    else if (pos == 0) base = name.substr(1);
    else base = name.substr(pos+1);
    ole->setReadInverted(true);
    if (base=="StarDrawDocument" || base=="StarDrawDocument3") {
      if (mainOle) {
        STOFF_DEBUG_MSG(("StarObjectDraw::parse: find unexpected second draw ole %s\n", name.c_str()));
      }
      else {
        mainOle=ole;
        mainName=name;
        continue;
      }
    }
    if (base=="SfxStyleSheets") {
      readSfxStyleSheets(ole,name);
      continue;
    }
    if (base!="BasicManager2") {
      STOFF_DEBUG_MSG(("StarObjectDraw::parse: find unexpected ole %s\n", name.c_str()));
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    libstoff::DebugStream f;
    f << "Entries(" << base << "):";
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    asciiFile.reset();
  }

  if (!mainOle) {
    STOFF_DEBUG_MSG(("StarObjectDraw::parser: can not find the main draw document\n"));
    return false;
  }
  else
    readDrawDocument(mainOle,mainName);

  return true;
}

bool StarObjectDraw::readDrawDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SCDrawDocument", getPassword()); // checkme: do we need to pass the password
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);
  auto firstByte=static_cast<uint8_t>(input->readULong(1));
  if (firstByte!=0x44) { // D
    /* if the zone has a password, we can retrieve it knowing that the first byte must be 0x44

       TODO: we must also check if the user has given a password and
       if the user mask does correspond to the real mask.
    */
    input=StarEncryption::decodeStream(input, StarEncryption::getMaskToDecodeStream(firstByte, 0x44));
    if (!input) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument: decryption failed\n"));
      return false;
    }
    zone.setInput(input);
  }
  input->seek(0, librevenge::RVNG_SEEK_SET);
  std::shared_ptr<StarObjectModel> model(new StarObjectModel(*this, true));
  if (!model->read(zone)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument: can not read the main zone\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SCDrawDocument):###");
    return false;
  }
  m_drawState->m_model=model;
#if 0
  std::cerr << *model << "\n";
#endif
  long pos=input->tell();
  if (!readPresentationData(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readDrawDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SCDrawDocument):###extra");
  }
  return true;
}
catch (...)
{
  return false;
}

bool StarObjectDraw::readPresentationData(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCDrawPres):";
  // sd_drawdoc.cxx: operator>>(SdDrawDocument)
  int vers=zone.getHeaderVersion();
  long lastPos=zone.getRecordLastPosition();
  f << "vers=" << vers << ",";
  input->seek(1, librevenge::RVNG_SEEK_CUR); // dummy
  for (int i=0; i<5; ++i) {
    bool bVal;
    *input>>bVal;
    if (!bVal) continue;
    char const *wh[]= {"pres[all]", "pres[end]", "pres[manual]", "mouse[visible]", "mouse[asPen]"};
    f << wh[i] << ",";
  }
  auto firstPage=int(input->readULong(4));
  if (firstPage!=1) f << "firstPage=" << firstPage << ",";

  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: the zone is too short\n"));
    f<<"###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SCDrawPres");
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  f.str("");
  f << "SCDrawPres:";
  pos=input->tell();
  if (vers>=1) {
    bool hasFrameView;
    *input>>hasFrameView;
    if (hasFrameView && (!readSdrFrameView(zone) || input->tell()>lastPos)) {
      f<<"###frameView";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      zone.closeSCHHeader("SCDrawPres");
      return true;
    }
  }

  pos=input->tell();
  if (vers>=2) { // we need to set the pool here
    StarFileManager fileManager;
    if (!fileManager.readJobSetUp(zone, true)) {
      f<<"###setUp";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      zone.closeSCHHeader("SCDrawPres");
      return true;
    }
  }
  pos=input->tell();
  bool ok=true;
  if (vers>=3) {
    f << "language=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=4) {
    auto n=int(input->readULong(4));
    f << "num[frameView]=" << n << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SCDrawPres:";
    for (int i=0; i<n; ++i) {
      if (!readSdrFrameView(zone) || input->tell()>lastPos) {
        ok=false;
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        break;
      }
    }
    if (ok) pos=input->tell();
  }
  if (ok&&vers>=5) {
    bool startWithNav;
    *input >> startWithNav;
    if (startWithNav) f << "start[withNav],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=6) {
    bool presLockedPage;
    *input >> presLockedPage;
    if (presLockedPage) f << "lock[pages],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=7) {
    bool presOnTop;
    *input >> presOnTop;
    if (presOnTop) f << "pres[onTop],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=8) {
    bool onlineSpell, hideSpell;
    *input >> onlineSpell >> hideSpell;
    if (onlineSpell) f << "spell[online],";
    if (hideSpell) f << "spell[hide],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=9) {
    bool fullScreen;
    *input >> fullScreen;
    if (fullScreen) f << "pres[fullScreen],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=10) {
    std::vector<uint32_t> string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: can not read presPage\n"));
      ok=false;
      f << "###presPage,";
    }
    else if (!string.empty())
      f << libstoff::getString(string).cstr() << ",";
  }
  if (ok&&vers>=11) {
    bool animOk;
    *input >> animOk;
    if (animOk) f << "anim[allowed],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=12) {
    auto docType=int(input->readULong(2));
    if (docType==0) f << "impress,";
    else if (docType==1) f << "draw,";
    else {
      STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: unknown doc type\n"));
      f << "###docType=" << docType << ",";
    }
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=14) {
    bool custShow;
    *input>>custShow;
    if (custShow) f << "customShow,";
    auto nCustShow=int(input->readULong(4));
    if (nCustShow) {
      f << "n[custShow]=" << nCustShow << ",";
      pos=input->tell();
      f.str("");
      f << "SCDrawPres:";
      for (int i=0; i<nCustShow; ++i) {
        if (!readSdrCustomShow(zone) || input->tell()>lastPos) {
          ok=false;
          input->seek(pos, librevenge::RVNG_SEEK_SET);
          break;
        }
      }
      if (ok) pos=input->tell();
    }
  }
  if (ok&&vers>=15) {
    f << "pageNum[type]=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=17) {
    f << "sec[pause]=" << input->readULong(4) << ",";
    bool showLogo;
    *input>>showLogo;
    if (showLogo) f << "show[logo],";
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readPresentationData: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SCDrawPres:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSCHHeader("SCDrawPres");
  return true;
}

bool StarObjectDraw::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  if (getDocumentKind()!=STOFFDocument::STOFF_K_DRAW) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: called with unexpected document\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SfxStyleSheets)");
    return false;
  }
  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  auto pool=getNewItemPool(StarItemPool::T_XOutdevPool);
  pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));
  auto mainPool=pool;
  while (!input->isEnd()) {
    // REMOVEME: remove this loop, when creation of secondary pool is checked
    long pos=input->tell();
    bool extraPool=false;
    if (!pool) {
      extraPool=true;
      pool=getNewItemPool(StarItemPool::T_Unknown);
    }
    if (pool && pool->read(zone)) {
      if (extraPool) {
        STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: create extra pool of type %d\n", int(pool->getType())));
      }
      if (!mainPool) mainPool=pool;
      pool.reset();
      continue;
    }
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->isEnd()) return true;
  long pos=input->tell();
  if (!mainPool || !mainPool->readStyles(zone, *this)) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: can not read a style pool\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSfxStyleSheets: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("Entries(SfxStyleSheets):###extra");
  }
  return true;
}

////////////////////////////////////////////////////////////
//
// Low level
//
////////////////////////////////////////////////////////////

bool StarObjectDraw::readSdrCustomShow(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrCustomShow)[" << zone.getRecordLevel() << "]:";
  int vers=zone.getHeaderVersion();
  f << "vers=" << vers << ",";

  long lastPos=zone.getRecordLastPosition();
  std::vector<uint32_t> string;
  if (!zone.readString(string) || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrCustomShow: can not read the name\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SdrCustomShow");
    return true;
  }
  f << libstoff::getString(string).cstr() << ",";
  auto n=long(input->readULong(4));
  f << "N=" << n << ",";
  if (n<0 || (lastPos-input->tell())/2<n || input->tell()+2*n>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrCustomShow: the number of page seems bad\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SdrCustomShow");
    return true;
  }
  f << "pages=[";
  for (long p=0; p<n; ++p)
    f << input->readULong(2) << ",";
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrCustomShow: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrCustomShow:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSCHHeader("SdrCustomShow");
  return true;
}

bool StarObjectDraw::readSdrHelpLine(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrHl" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrHelpLine)[" << zone.getRecordLevel() << "]:set,";
  //svx_svdhlpln.cxx operator>>(SdrHelpLine)
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  long lastPos=zone.getRecordLastPosition();
  auto val=int(input->readULong(2));
  if (val) f << "kind=" << val << ",";
  int dim[2];
  for (int &i : dim) i=int(input->readLong(4));
  f << "pos=" << STOFFVec2i(dim[0],dim[1]) << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrHelpLine: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrHelpLine:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrHelpLine");
  return true;
}

bool StarObjectDraw::readSdrHelpLineSet(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrHL" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrHelpLine)[" << zone.getRecordLevel() << "]:set,";
  //svx_svdhlpln.cxx operator>>(SdrHelpLineList)
  long lastPos=zone.getRecordLastPosition();
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  auto n=int(input->readULong(2));
  if (n) f << "N=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int i=0; i<n; ++i) {
    pos=input->tell();
    if (!readSdrHelpLine(zone) || input->tell()>lastPos) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrHelpLineSet: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrHelpLine:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrHelpLine");
  return true;
}

bool StarObjectDraw::readSdrFrameView(StarZone &zone)
{
  //sd_frmview.cxx operator>>(FrameView)
  if (!readSdrView(zone))
    return false;

  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrFrameView)[" << zone.getRecordLevel() << "]:";
  int vers=zone.getHeaderVersion();
  f << "vers=" << vers << ",";

  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+1+3*32>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrFrameView: the zone seems too short\n"));
    f << "###short";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSCHHeader("SdrFrameView");
  }
  if (input->readLong(1)) f << "ruler,";
  for (int i=0; i<3; ++i) {
    char const *wh[]= {"visible","printtable","helpLine"};
    f << "layers[" << wh[i] << "]=[";
    int prevFl=-1, numSame=0;
    for (int j=0; j<=32; ++j) {
      int fl=j==32 ? -1 : int(input->readULong(1));
      if (fl==prevFl) {
        ++numSame;
        continue;
      }
      if (numSame) {
        if (prevFl==0)
          f << "_";
        else if (prevFl==255)
          f << "*";
        else
          f << std::hex << prevFl << std::dec;
        if (numSame>1)
          f << "x" << numSame;
        f << ",";
      }
      prevFl=fl;
      numSame=1;
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  bool ok=true;
  if (!readSdrHelpLineSet(zone)) { // standart line
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    ok=false;
  }
  if (ok&&vers>=1) {
    // notes and handout line
    for (int i=0; i<2; ++i) {
      if (readSdrHelpLineSet(zone) && input->tell()<=lastPos)
        continue;
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
  }
  if (ok) pos=input->tell();
  f.str("");
  f << "SdrFrameView:";
  if (ok&&vers>=2) {
    bool noColors, noAttribs;
    *input>>noColors >> noAttribs;
    if (noColors) f << "no[color],";
    if (noAttribs) f << "no[attrib],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=3) {
    int dim[4];
    for (int &i : dim) i=int(input->readLong(4));
    f << "vis[area]=" << STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3])) << ",";
    f << "page[kind]=" << input->readULong(4) << ",";
    f << "page[select]=" << input->readULong(2) << ",";
    f << "page[editMode]=" << input->readULong(4) << ",";
    bool layerMode;
    *input>>layerMode;
    if (layerMode) f << "layer[mode],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=4) {
    bool quickEdit;
    *input>>quickEdit;
    if (quickEdit) f << "edit[quick],";
  }
  if (ok&&vers>=5) {
    bool dragWithCopy;
    *input>>dragWithCopy;
    if (dragWithCopy) f << "drag[withCopy],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=6)
    f << "slides[byRow]="<< input->readULong(2) << ",";
  if (ok&&vers>=7) {
    bool bigHandles, clickTextEdit, clickChangeRot;
    *input >> bigHandles >> clickTextEdit >> clickChangeRot;
    if (bigHandles) f << "handles[big],";
    if (clickTextEdit) f << "click[textEdit],";
    if (clickChangeRot) f << "click[changeRotation],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=8) {
    f << "notes[editMode]=" << input->readULong(4) << ",";
    f << "notes[handoutMode]=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=9) {
    f << "draw[mode]=" << input->readULong(4) << ",";
    f << "draw[preview,mode]=" << input->readULong(4) << ",";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=10) {
    bool inPageMode, inMasterPageMode;
    *input >> inPageMode >> inMasterPageMode;
    if (inPageMode) f << "showPreview[pageMode],";
    if (inMasterPageMode) f << "showPreview[masterPageMode],";
    ok=input->tell()<=lastPos;
  }
  if (ok&&vers>=11) {
    bool inOutlineMode;
    *input >> inOutlineMode;
    if (inOutlineMode) f << "showPreview[outlineMode],";
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }

  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrFrameView: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrFrameView:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSCHHeader("SdrFrameView");
  return true;
}

bool StarObjectDraw::readSdrView(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrVw" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrView)[" << zone.getRecordLevel() << "]:";
  //svx_svdview.cxx operator>>(SdrView)
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  int n=0;
  while (input->tell()<lastPos) {
    // svx_svdio.cxx: SdrNamedSubRecord::SdrNamedSubRecord
    pos=input->tell();
    if (!zone.openRecord()) break;
    f.str("");
    f << "SdrViewA" << ++n << "[" << zone.getRecordLevel() << "]:";
    if (pos+10>zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("StarObjectDraw::readSdrView: sub named record seems too short\n"));
      f << "###short";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrView");
      continue;
    }
    std::string inventor("");
    for (int i=0; i<4; ++i) inventor+=char(input->readULong(1));
    f << inventor << "-" << input->readULong(2) << ",";
    if (input->tell()!=zone.getRecordLastPosition()) {
      ascFile.addDelimiter(input->tell(),'|');
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord("SdrView");
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectDraw::readSdrView: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrView:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrView");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
