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
#include <set>
#include <sstream>

#include <librevenge/librevenge.h>

#include "StarObjectPageStyle.hxx"

#include "STOFFListener.hxx"
#include "STOFFPageSpan.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarState.hxx"
#include "StarWriterStruct.hxx"
#include "StarZone.hxx"
#include "SWFieldManager.hxx"

/** Internal: the structures of a StarObjectPageStyle */
namespace StarObjectPageStyleInternal
{
/** \brief structure to store a endnote/footnote page description
 */
struct NoteDesc {
public:
  //! constructor
  explicit NoteDesc(bool isFootnote)
    : m_isFootnote(isFootnote)
    , m_adjust(0)
    , m_penWidth(0)
    , m_color(STOFFColor::black())
  {
    for (float &distance : m_distances) distance=0;
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, NoteDesc const &desc);
  //! try to read a noteDesc: '1' or '2'
  bool read(StarZone &zone);
  //! a flag to know if this corresponds to a footnote or a endnote
  bool m_isFootnote;
  //! the width/height/topDist/bottomDist
  float m_distances[4];
  //! the adjust
  int m_adjust;
  //! the pen's width
  int m_penWidth;
  //! the color
  STOFFColor m_color;
};

bool NoteDesc::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  if (input->peek()!=(m_isFootnote ? '1' : '2') || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::NoteDesc::read: can not read a noteDesc\n"));
    return false;
  }
  // sw_sw3page.cxx InPageFtnInfo
  f << "Entries(StarNoteDesc)[" << zone.getRecordLevel() << "]:";
  for (int i=1; i<4; ++i) m_distances[i]=float(input->readLong(4));
  m_adjust=int(input->readLong(2));
  long dim[2];
  for (long &i : dim) i=long(input->readLong(4));
  if (!dim[1]) {
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::NoteDesc::read: can not find the width deminator\n"));
  }
  else
    m_distances[0]=float(dim[0])/float(dim[1]);
  m_penWidth=int(input->readLong(2));
  if (!input->readColor(m_color)) {
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::NoteDesc::read: can not read a color\n"));
    f << "###color,";
  }
  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarNoteDesc");
  return true;
}

std::ostream &operator<<(std::ostream &o, NoteDesc const &desc)
{
  if (desc.m_isFootnote)
    o << "footnote,";
  else
    o << "endnote,";
  for (int i=0; i<4; ++i) {
    if (desc.m_distances[i]>=0 && desc.m_distances[i]<=0) continue;
    char const *wh[]= {"width", "height", "top[dist]", "bottom[dist]"};
    o << wh[i] << "=" << desc.m_distances[i] << ",";
  }
  if (desc.m_adjust) o << "adjust=" << desc.m_adjust << ",";
  if (desc.m_penWidth) o << "penWidth=" << desc.m_penWidth << ",";
  if (!desc.m_color.isBlack()) o << "color=" << desc.m_color << ",";
  return o;
}

/** \brief structure to store a page description
 */
struct PageDesc {
public:
  //! constructor
  explicit PageDesc()
    : m_name("")
    , m_follow("")
    , m_landscape(false)
    , m_poolId(0)
    , m_numType(0)
    , m_usedOn(3)
    , m_regCollIdx(0xFFFF)
  {
  }
  //! destructor
  ~PageDesc()
  {
  }
  //! update pagespan properties
  void updatePageSpan(StarState &state) const;
  /** try to update the section*/
  bool updateState(StarState &state) const;
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, PageDesc const &desc);
  //! try to read a pageDesc: 'p'
  bool read(StarZone &zone, StarObject &object);
  //! the name
  librevenge::RVNGString m_name;
  //! the follow
  librevenge::RVNGString m_follow;
  //! is the page a lanscape
  bool m_landscape;
  //! the poolId
  int m_poolId;
  //! the number's type
  int m_numType;
  //! the page where is it used ?
  int m_usedOn;
  //! the coll idx
  int m_regCollIdx;
  //! the foot and page foot desc
  std::shared_ptr<NoteDesc> m_noteDesc[2];
  //! the master and left attributes lists
  std::vector<StarWriterStruct::Attribute> m_attributes[2];
};

void PageDesc::updatePageSpan(StarState &state) const
{
  updateState(state);
  auto &page=state.m_global->m_page;
  if (m_landscape && page.m_propertiesList[0]["fo:page-height"] && page.m_propertiesList[0]["fo:page-width"] &&
      page.m_propertiesList[0]["fo:page-height"]->getInt() > page.m_propertiesList[0]["fo:page-width"]->getInt()) {
    // we must inverse fo:page-height and fo:page-width
    auto height=page.m_propertiesList[0]["fo:page-height"]->getStr();
    page.m_propertiesList[0].insert("fo:page-height", page.m_propertiesList[0]["fo:page-width"]);
    page.m_propertiesList[0].insert("fo:page-width", height);
    page.m_propertiesList[0].insert("style:print-orientation", "landscape");
  }
}

bool PageDesc::updateState(StarState &state) const
{
  for (const auto &attribute : m_attributes) {
    for (auto &attr : attribute) {
      if (attr.m_attribute)
        attr.m_attribute->addTo(state);
    }
  }
  return true;
}

bool PageDesc::read(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  if (input->peek()!='p' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::PageDesc::read: can not read a pageDesc\n"));
    return false;
  }
  // sw_sw3page.cxx InPageDesc
  f << "Entries(StarPageDesc)[" << type << "-" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0x10) m_landscape=true;
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  auto val=int(input->readULong(2));
  if (!zone.getPoolName(val, m_name)) {
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::PageDesc::read: can not find a pool name\n"));
    f << "###nameId=" << val << ",";
  }
  val=int(input->readULong(2));
  if (!zone.getPoolName(val, m_follow)) {
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::PageDesc::read: can not find a pool name\n"));
    f << "###followId=" << val << ",";
  }
  m_poolId=int(input->readULong(2));
  m_numType=int(input->readULong(1));
  m_usedOn=int(input->readULong(2));
  if (zone.isCompatibleWith(0x16,0x22, 0x101))
    m_regCollIdx=int(input->readULong(2));
  zone.closeFlagZone();
  f << *this;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  int whichAttrib=0;
  while (input->tell() < lastPos) {
    pos=input->tell();
    int rType=input->peek();
    bool done=false;
    switch (rType) {
    case 'S': {
      std::vector<StarWriterStruct::Attribute> attributeList;
      done=StarWriterStruct::Attribute::readList(zone, attributeList, object);
      if (!done) break;
      if (whichAttrib<2) // first master, second left
        m_attributes[whichAttrib++]=attributeList;
      else {
        STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::PageDesc::read: unexpected attribute list\n"));
      }
      break;
    }
    case '1': // foot info
    case '2': { // page foot info
      std::shared_ptr<NoteDesc> desc(new NoteDesc(rType=='1'));
      done=desc->read(zone);
      if (done)
        m_noteDesc[rType=='1' ? 0 : 1]=desc;
      break;
    }
    default:
      break;
    }
    if (done) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    f.str("");
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f << "StarPageDesc[" << type << "-" << zone.getRecordLevel() << "]:";
    STOFF_DEBUG_MSG(("StarObjectPageStyleInternal::PageDesc::read: find unknown type\n"));
    f << "###type,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarPageDesc");
  }
  zone.closeSWRecord('p', "StarPageDesc");
  return true;
}

std::ostream &operator<<(std::ostream &o, PageDesc const &desc)
{
  o << desc.m_name.cstr() << ",";
  if (!desc.m_follow.empty()) o << "follow=" << desc.m_follow.cstr() << ",";
  if (desc.m_landscape) o << "landscape,";
  if (desc.m_poolId) o << "poolId=" << desc.m_poolId << ",";
  if (desc.m_numType) o << "numType=" << desc.m_numType << ",";
  switch (desc.m_usedOn&3) {
  case 1:
    o << "left,";
    break;
  case 2:
    o << "right,";
    break;
  case 3:
    o << "all,";
    break;
  case 0: // internal
  default:
    break;
  }
  if (desc.m_usedOn&0x40) o << "header[share],";
  if (desc.m_usedOn&0x80) o << "footer[share],";
  if (desc.m_usedOn&0x100) o << "first[share],";
  if (desc.m_usedOn&0xFE3C) o << "usedOn=" << std::hex << (desc.m_usedOn&0xFE3C) << std::dec << ",";
  if (desc.m_regCollIdx!=0xFFFF) o << "regCollIdx=" << desc.m_regCollIdx << ",";
  return o;
}

////////////////////////////////////////
//! Internal: the state of a StarObjectPageStyle
struct State {
  //! constructor
  State()
    : m_pageList()
    , m_nameToPageIdMap()
    , m_simplifyNameToPageIdMap()
  {
  }
  //! list of pages
  std::vector<PageDesc> m_pageList;
  //! map name to id
  std::map<librevenge::RVNGString, size_t> m_nameToPageIdMap;
  //! map simplify name to id
  std::map<librevenge::RVNGString, size_t> m_simplifyNameToPageIdMap;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectPageStyle::StarObjectPageStyle(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_pageStyleState(new StarObjectPageStyleInternal::State)
{
}

StarObjectPageStyle::~StarObjectPageStyle()
{
}

////////////////////////////////////////////////////////////
// send data
////////////////////////////////////////////////////////////
bool StarObjectPageStyle::updatePageSpan(librevenge::RVNGString const &name, StarState &state)
{
  auto &ps=state.m_global->m_page;
  ps=STOFFPageSpan();
  size_t id=0;
  if (m_pageStyleState->m_nameToPageIdMap.find(name) != m_pageStyleState->m_nameToPageIdMap.end())
    id=m_pageStyleState->m_nameToPageIdMap.find(name)->second;
  else {
    auto simpName=libstoff::simplifyString(name);
    if (m_pageStyleState->m_simplifyNameToPageIdMap.find(simpName)!=m_pageStyleState->m_simplifyNameToPageIdMap.end())
      id=m_pageStyleState->m_simplifyNameToPageIdMap.find(simpName)->second;
    else if (!name.empty()) {
      STOFF_DEBUG_MSG(("StarObjectPageStyle::updatePageSpan: can not find page %s\n", name.cstr()));
    }
  }
  if (id>=m_pageStyleState->m_pageList.size())
    return false;
  size_t listIds[3];
  std::string listOccurence[3];
  std::set<librevenge::RVNGString> seen;
  int numPages=0;
  for (int i=0; i<3; ++i) {
    auto const &page=m_pageStyleState->m_pageList[id];
    listIds[i]=id;
    numPages=i+1;
    if ((page.m_usedOn&3)==3) {
      if (i==1) listOccurence[0]="first";
      listOccurence[i]="all";
      break;
    }
    listOccurence[i]=(page.m_usedOn&1) ? "left" : "right";
    seen.insert(page.m_name);
    auto const &follow=page.m_follow;
    if (follow.empty() || seen.find(follow)!=seen.end())
      break;
    if (m_pageStyleState->m_nameToPageIdMap.find(follow) != m_pageStyleState->m_nameToPageIdMap.end())
      id=m_pageStyleState->m_nameToPageIdMap.find(follow)->second;
    else {
      auto simpName=libstoff::simplifyString(follow);
      if (m_pageStyleState->m_simplifyNameToPageIdMap.find(simpName)!=m_pageStyleState->m_simplifyNameToPageIdMap.end())
        id=m_pageStyleState->m_simplifyNameToPageIdMap.find(simpName)->second;
      else {
        STOFF_DEBUG_MSG(("StarObjectPageStyle::updatePageSpan: can not find page %s\n", follow.cstr()));
        break;
      }
    }
    if (id>=m_pageStyleState->m_pageList.size())
      break;
  }
  if (numPages==3) listOccurence[0]="first";
  for (int p=numPages-1; p>=0; --p) { // invert so that section correspond to the first page
    ps.m_section=STOFFSection();
    state.m_global->m_pageOccurence=listOccurence[p];
    m_pageStyleState->m_pageList[listIds[p]].updatePageSpan(state);
  }
  return true;
}

bool StarObjectPageStyle::updatePageSpans
(std::vector<librevenge::RVNGString> const &listNames, std::vector<STOFFPageSpan> &pageSpan, int &number)
{
  librevenge::RVNGString lastPageName("");
  int numPage=0;
  number=0;
  auto pool=findItemPool(StarItemPool::T_WriterPool, false);
  StarState state(pool.get(), *this);
  auto &ps=state.m_global->m_page;
  for (size_t i=0; i<=listNames.size(); ++i) {
    bool newPage=(i==listNames.size()) || (lastPageName!="" && listNames[i]!="" && lastPageName!=listNames[i]);
    if (!newPage) {
      if (lastPageName.empty()) lastPageName=listNames[i];
      ++numPage;
      continue;
    }
    if (i==listNames.size())
      numPage=10000; // be sure to allow enough page
    if (numPage) {
      updatePageSpan(lastPageName, state);
      ps.m_pageSpan=numPage;
      pageSpan.push_back(ps);
      number+=numPage;
    }
    if (i==listNames.size()) break;
    numPage=1;
    lastPageName=listNames[i];
  }
  return number!=0;
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////
bool StarObjectPageStyle::read(StarZone &zone)
try
{
  STOFFInputStreamPtr input=zone.input();
  if (!zone.readSWHeader()) {
    STOFF_DEBUG_MSG(("StarObjectPageStyle::read: can not read the header\n"));
    return false;
  }
  zone.readStringsPool();
  SWFieldManager fieldManager;
  while (fieldManager.readField(zone,'Y'))
    ;
  std::vector<StarWriterStruct::Bookmark> markList;
  StarWriterStruct::Bookmark::readList(zone, markList);
  std::vector<std::vector<StarWriterStruct::Redline> > redlineListList;
  StarWriterStruct::Redline::readListList(zone, redlineListList);
  getFormatManager()->readSWNumberFormatterList(zone);
  // sw_sw3page.cxx Sw3IoImp::InPageDesc
  libstoff::DebugFile &ascFile=zone.ascii();
  while (!input->isEnd()) {
    long pos=input->tell();
    unsigned char type;
    if (!zone.openSWRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    libstoff::DebugStream f;
    f << "StarPageStyleSheets[" << type << "]:";
    bool done=false;
    switch (type) {
    case 'P': {
      zone.openFlagZone();
      auto N=int(input->readULong(2));
      f << "N=" << N << ",";
      zone.closeFlagZone();
      for (int i=0; i<N; ++i) {
        StarObjectPageStyleInternal::PageDesc desc;
        // read will check that we can read the data, ....
        if (!desc.read(zone, *this))
          break;
        if (m_pageStyleState->m_nameToPageIdMap.find(desc.m_name)!=m_pageStyleState->m_nameToPageIdMap.end()) {
          STOFF_DEBUG_MSG(("StarObjectPageStyle::read: oops page with name=%s already exists\n", desc.m_name.cstr()));
        }
        else {
          m_pageStyleState->m_nameToPageIdMap[desc.m_name]=m_pageStyleState->m_pageList.size();
          auto simpName=libstoff::simplifyString(desc.m_name);
          if (m_pageStyleState->m_simplifyNameToPageIdMap.find(simpName)==m_pageStyleState->m_simplifyNameToPageIdMap.end())
            m_pageStyleState->m_simplifyNameToPageIdMap[simpName]=m_pageStyleState->m_pageList.size();
        }
        m_pageStyleState->m_pageList.push_back(desc);
      }
      break;
    }
    case 'Z':
      done=true;
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectPageStyle::read: find unknown data\n"));
      f << "###";
      break;
    }
    if (!zone.closeSWRecord(type, "StarPageStyleSheets")) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (done)
      break;
  }

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectPageStyle::read: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("StarPageStyleSheets:##extra");
  }

  return true;
}
catch (...)
{
  return false;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
