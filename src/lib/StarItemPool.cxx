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

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <sstream>

#include <librevenge/librevenge.h>

#include "StarAttribute.hxx"
#include "StarFileManager.hxx"
#include "StarObject.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"
#include "STOFFGraphicStyle.hxx"
#include "STOFFListener.hxx"
#include "STOFFParagraph.hxx"

#include "StarItemPool.hxx"

/** Internal: the structures of a StarItemPool */
namespace StarItemPoolInternal
{
////////////////////////////////////////
//! Internal: a structure use to read SfxMultiRecord zone of a StarItemPool
struct SfxMultiRecord {
  //! constructor
  SfxMultiRecord()
    : m_zone(nullptr)
    , m_zoneType(0)
    , m_zoneOpened(false)
    , m_headerType(0)
    , m_headerVersion(0)
    , m_headerTag(0)
    , m_actualRecord(0)
    , m_numRecord(0)
    , m_contentSize(0)
    , m_startPos(0)
    , m_endPos(0)
    , m_offsetList()
    , m_extra("")
  {
  }
  //! returns true if the record is opened
  bool isOpened() const
  {
    return m_zoneOpened;
  }
  //! returns the number of record
  uint16_t getNumRecords() const
  {
    return m_zoneOpened ? m_numRecord : 0;
  }
  //! returns the header tag or -1
  int getHeaderTag() const
  {
    return !m_zoneOpened ? -1 : int(m_headerTag);
  }
  //! try to open a zone
  bool open(StarZone &zone)
  {
    if (m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord: oops a record has been opened\n"));
      return false;
    }
    m_actualRecord=m_numRecord=0;
    m_headerType=m_headerVersion=0;
    m_headerTag=0;
    m_contentSize=0;
    m_offsetList.clear();
    m_zone=&zone;
    STOFFInputStreamPtr input=m_zone->input();
    long pos=input->tell();
    if (!m_zone->openSfxRecord(m_zoneType)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    if (m_zoneType==static_cast<unsigned char>(0xff)) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord: oops end header\n"));
      m_extra="###emptyZone,";
      return true; /* empty zone*/
    }
    if (m_zoneType!=0) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord: find unknown header\n"));
      m_extra="###badZoneType,";
      return true;
    }

    m_zoneOpened=true;
    m_endPos=m_zone->getRecordLastPosition();
    // filerec.cxx: SfxSingleRecordReader::FindHeader_Impl
    if (input->tell()+10>m_endPos) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::open: oops the zone seems too short\n"));
      m_extra="###zoneShort,";
      return true;
    }
    *input >> m_headerType >> m_headerVersion >> m_headerTag;
    // filerec.cxx: SfxMultiRecordReader::ReadHeader_Impl
    *input >> m_numRecord >> m_contentSize;
    m_startPos=input->tell();
    std::stringstream s;
    if (m_headerType==2) {
      // fixed size
      if (m_startPos+long(m_numRecord)*long(m_contentSize) > m_endPos) {
        STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::open: oops the number of record seems bad\n"));
        s << "##numRecord=" << m_numRecord << ",";
        if (m_contentSize && m_endPos>m_startPos)
          m_numRecord=uint16_t((m_endPos-m_startPos)/long(m_contentSize));
        else
          m_numRecord=0;
      }
      m_extra=s.str();
      return true;
    }

    long debOffsetList=((m_headerType==3 || m_headerType==7) ? m_startPos : 0) + long(m_contentSize);
    if (debOffsetList<m_startPos || debOffsetList+4*m_numRecord > m_endPos) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::open: can not find the version map offset\n"));
      s << "###contentCount";
      m_numRecord=0;
      m_extra=s.str();
      return true;
    }
    m_endPos=debOffsetList;
    input->seek(debOffsetList, librevenge::RVNG_SEEK_SET);
    for (uint16_t i=0; i<m_numRecord; ++i) {
      uint32_t offset;
      *input >> offset;
      m_offsetList.push_back(offset);
    }
    input->seek(m_startPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  //! try to close a zone
  void close(std::string const &wh)
  {
    if (!m_zone) return;
    if (!m_zoneOpened) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::close: can not find any opened zone\n"));
      return;
    }
    m_zoneOpened=false;
    STOFFInputStreamPtr input=m_zone->input();
    if (input->tell()<m_endPos && input->tell()+4>=m_endPos) { // small diff is possible
      m_zone->ascii().addDelimiter(input->tell(),'|');
      input->seek(m_zone->getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()==m_endPos)
      input->seek(m_zone->getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    m_zone->closeSfxRecord(m_zoneType, wh);
    m_zone=nullptr;
  }
  //! try to go to the new content positon
  bool getNewContent(std::string const &wh, int &id)
  {
    if (!m_zone) return false;
    // SfxMultiRecordReader::GetContent
    long newPos=getLastContentPosition();
    if (newPos>=m_endPos) return false;
    STOFFInputStreamPtr input=m_zone->input();
    id=m_actualRecord++;
    if (input->tell()<newPos && input->tell()+4>=newPos) { // small diff is possible
      m_zone->ascii().addDelimiter(input->tell(),'|');
      input->seek(newPos, librevenge::RVNG_SEEK_SET);
    }
    else if (input->tell()!=newPos) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::getNewContent: find extra data\n"));
      m_zone->ascii().addPos(input->tell());
      libstoff::DebugStream f;
      f << wh << ":###extra";
      m_zone->ascii().addNote(f.str().c_str());
      input->seek(newPos, librevenge::RVNG_SEEK_SET);
    }
    if (m_headerType==7) // mixtags?
      input->seek(2, librevenge::RVNG_SEEK_CUR);
    else if (m_headerType==8) // relocate
      id=int(input->readULong(2));
    return true;
  }
  //! returns the last content position
  long getLastContentPosition() const
  {
    if (m_actualRecord >= m_numRecord) return m_endPos;
    if (m_headerType==2) return m_startPos+m_actualRecord*long(m_contentSize);
    if (m_actualRecord >= uint16_t(m_offsetList.size())) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::SfxMultiRecord::getLastContentPosition: argh, find unexpected index\n"));
      return m_endPos;
    }
    const long pos = m_startPos+long(m_offsetList[size_t(m_actualRecord)]>>8)-14;
    return m_zone->input()->checkPosition(pos) ? pos : m_endPos;
  }

  //! basic operator<< ; print header data
  friend std::ostream &operator<<(std::ostream &o, SfxMultiRecord const &r)
  {
    if (!r.m_zoneOpened) {
      o << r.m_extra;
      return o;
    }
    if (r.m_headerType) o << "type=" << int(r.m_headerType) << ",";
    if (r.m_headerVersion) o << "version=" << int(r.m_headerVersion) << ",";
    if (r.m_headerTag) o << "tag=" << r.m_headerTag << ",";
    if (r.m_numRecord) o << "num[record]=" << r.m_numRecord << ",";
    if (r.m_contentSize) o << "content[size/pos]=" << r.m_contentSize << ",";
    if (!r.m_offsetList.empty()) {
      o << "offset=[";
      for (auto const &off : r.m_offsetList) {
        if (off&0xff)
          o << (off>>8) << ":" << (off&0xff) << ",";
        else
          o << (off>>8) << ",";
      }
      o << "],";
    }
    o << r.m_extra;
    return o;
  }
protected:
  //! the main zone
  StarZone *m_zone;
  //! the zone type
  unsigned char m_zoneType;
  //! true if a SfxRecord has been opened
  bool m_zoneOpened;
  //! the record type
  uint8_t m_headerType;
  //! the header version
  uint8_t m_headerVersion;
  //! the header tag
  uint16_t m_headerTag;
  //! the actual record
  uint16_t m_actualRecord;
  //! the number of record
  uint16_t m_numRecord;
  //! the record/content/pos size
  uint32_t m_contentSize;
  //! the start of data position
  long m_startPos;
  //! the end of data position
  long m_endPos;
  //! the list of (offset + type)
  std::vector<uint32_t> m_offsetList;
  //! extra data
  std::string m_extra;
private:
  SfxMultiRecord(SfxMultiRecord const &orig) = delete;
  SfxMultiRecord &operator=(SfxMultiRecord const &orig) = delete;
};

//! small struct used to keep a list of version
struct Version {
  //! constructor
  Version(int vers, int start, std::vector<int> const &list)
    : m_version(vers)
    , m_start(start)
    , m_list(list)
    , m_invertListMap()
  {
    for (size_t i=0; i<m_list.size(); ++i)
      m_invertListMap[list[i]]=int(i);
  }
  //! the version number
  int m_version;
  //! int the start value
  int m_start;
  //! the list of value
  std::vector<int> m_list;
  //! a map offset to which
  std::map<int,int> m_invertListMap;
};

//! internal: list of attribute corresponding to a slot id
struct Values {
  //! constructor
  Values()
    : m_default()
    , m_idValueMap()
  {
  }
  //! the default values
  std::shared_ptr<StarAttribute> m_default;
  //! the list of attribute
  std::map<int,std::shared_ptr<StarAttribute> > m_idValueMap;
};

////////////////////////////////////////
//! Internal: a style of a StarItemPool
struct StyleId {
  //! constructor
  StyleId(librevenge::RVNGString const &name, int family)
    : m_name(name)
    , m_family(family)
  {
  }
  //! operator==
  bool operator==(StyleId const &other) const
  {
    return m_family==other.m_family && m_name==other.m_name;
  }
  //! operator!=
  bool operator!=(StyleId const &other) const
  {
    return !operator==(other);
  }
  //! operator<
  bool operator<(StyleId const &other) const
  {
    if (m_name<other.m_name) return true;
    if (m_name>other.m_name) return false;
    return m_family<other.m_family;
  }
  //! the name
  librevenge::RVNGString m_name;
  //! the family
  int m_family;
};

////////////////////////////////////////
//! Internal: the state of a StarItemPool
struct State {
  //! constructor
  State(StarObject &document, StarItemPool::Type type)
    : m_document(document)
    , m_type(StarItemPool::T_Unknown)
    , m_majorVersion(0)
    , m_minorVersion(0)
    , m_loadingVersion(-1)
    , m_name("")
    , m_relativeUnit(0)
    , m_isSecondaryPool(false)
    , m_secondaryPool()
    , m_currentVersion(0)
    , m_verStart(0)
    , m_verEnd(0)
    , m_versionList()
    , m_idToAttributeList()
    , m_slotIdToValuesMap()
    , m_styleIdToStyleMap()
    , m_simplifyNameToStyleNameMap()
    , m_idToDefaultMap()
    , m_delayedItemList()
  {
    init(type);
  }
  //! initialize a pool
  void init(StarItemPool::Type type);
  //! clean the state
  void clean()
  {
    if (m_secondaryPool) m_secondaryPool->clean();
    m_versionList.clear();
    m_idToAttributeList.clear();
    m_slotIdToValuesMap.clear();
    m_styleIdToStyleMap.clear();
    m_simplifyNameToStyleNameMap.clear();
    m_idToDefaultMap.clear();
    m_delayedItemList.clear();
  }
  //! set the pool name
  void setPoolName(librevenge::RVNGString const &name)
  {
    m_name=name;
    auto type=StarItemPool::T_Unknown;
    if (m_name=="EditEngineItemPool")
      type=StarItemPool::T_EditEnginePool;
    else if (m_name=="SchItemPool")
      type=StarItemPool::T_ChartPool;
    else if (m_name=="ScDocumentPool")
      type=StarItemPool::T_SpreadsheetPool;
    else if (m_name=="SWG")
      type=StarItemPool::T_WriterPool;
    else if (m_name=="XOutdevItemPool")
      type=StarItemPool::T_XOutdevPool;
    else if (m_name=="VCControls")
      type=StarItemPool::T_VCControlPool;
    else {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::State::setPoolName: find unknown pool type %s\n", name.cstr()));
    }
    init(type);
  }
  //! returns true if the value is in expected range
  int isInRange(int which) const
  {
    if (which>=m_verStart&&which<=m_verEnd) return true;
    if (m_secondaryPool) return m_secondaryPool->m_state->isInRange(which);
    return false;
  }
  //! add a new version map
  void addVersionMap(uint16_t nVers, uint16_t nStart, std::vector<int> const &list)
  {
    // SfxItemPool::SetVersionMap
    if (nVers<=m_currentVersion)
      return;
    m_versionList.push_back(Version(int(nVers), int(nStart), list));
    m_currentVersion=nVers;
    Version const &vers=m_versionList.back();
    if (vers.m_invertListMap.empty()) return;
    m_verStart=std::min(m_verStart,vers.m_invertListMap.begin()->first);
    m_verEnd=std::max(m_verEnd,(--vers.m_invertListMap.end())->first);
  }
  //! try to return ???
  int getWhich(int nFileWhich) const
  {
    // polio.cxx: SfxItemPool::GetNewWhich
    if (nFileWhich<m_verStart||nFileWhich>m_verEnd) {
      if (m_secondaryPool)
        return m_secondaryPool->m_state->getWhich(nFileWhich);
      STOFF_DEBUG_MSG(("StarItemPoolInternal::State::getWhich: can not find a conversion for which=%d\n", nFileWhich));
      return 0;
    }
    if (m_loadingVersion>m_currentVersion) {
      for (size_t i=m_versionList.size(); i>0;) {
        Version const &vers=m_versionList[--i];
        if (vers.m_version<=m_currentVersion)
          break;
        if (vers.m_invertListMap.find(nFileWhich)==vers.m_invertListMap.end())
          return 0;
        nFileWhich=vers.m_start+vers.m_invertListMap.find(nFileWhich)->second;
      }
    }
    else if (m_loadingVersion<m_currentVersion) {
      for (auto const &vers : m_versionList) {
        if (vers.m_version<=m_loadingVersion)
          continue;
        if (nFileWhich<vers.m_start || nFileWhich>=vers.m_start+int(vers.m_list.size())) {
          STOFF_DEBUG_MSG(("StarItemPoolInternal::State::getWhich: argh nFileWhich=%d is not in good range\n", nFileWhich));
          break;
        }
        else
          nFileWhich=vers.m_list[size_t(nFileWhich-vers.m_start)];
      }
    }
    return nFileWhich;
  }
  //! returns the state corresponding to which
  State *getPoolStateFor(int which)
  {
    if (which>=m_verStart&&which<=m_verEnd) return this;
    if (m_secondaryPool) return m_secondaryPool->m_state->getPoolStateFor(which);
    return nullptr;
  }
  //! returns a pointer to the values data
  Values *getValues(int id, bool create=false)
  {
    if (m_slotIdToValuesMap.find(id)!=m_slotIdToValuesMap.end())
      return &m_slotIdToValuesMap.find(id)->second;
    if (!create)
      return nullptr;
    m_slotIdToValuesMap[id]=Values();
    return &m_slotIdToValuesMap.find(id)->second;
  }
  //! try to return a default attribute corresponding to which
  std::shared_ptr<StarAttribute> getDefaultAttribute(int which)
  {
    if (m_idToDefaultMap.find(which)!=m_idToDefaultMap.end() && m_idToDefaultMap.find(which)->second)
      return m_idToDefaultMap.find(which)->second;
    std::shared_ptr<StarAttribute> res;
    auto *state=getPoolStateFor(which);
    if (!state || which<state->m_verStart || which>=state->m_verStart+int(state->m_idToAttributeList.size()) ||
        !state->m_document.getAttributeManager()) {
      STOFF_DEBUG_MSG(("StarItemPoolInternal::State::getDefaultAttribute: find unknown attribute\n"));
      res=StarAttributeManager::getDummyAttribute();
    }
    else
      res=m_document.getAttributeManager()->getDefaultAttribute(state->m_idToAttributeList[size_t(which-state->m_verStart)]);
    m_idToDefaultMap[which]=res;
    return res;
  }
  //! the document
  StarObject &m_document;
  //! the document type
  StarItemPool::Type m_type;
  //! the majorVersion
  int m_majorVersion;
  //! the minorVersion
  int m_minorVersion;
  //! the loading version
  int m_loadingVersion;
  //! the name
  librevenge::RVNGString m_name;
  //! the relative unit
  double m_relativeUnit;
  //! a flag to know if a pool is a secondary pool
  bool m_isSecondaryPool;
  //! the secondary pool
  std::shared_ptr<StarItemPool> m_secondaryPool;
  //! the current version
  int m_currentVersion;
  //! the minimum version
  int m_verStart;
  //! the maximum version
  int m_verEnd;
  //! the list of version
  std::vector<Version> m_versionList;
  //! list whichId to attribute list
  std::vector<int> m_idToAttributeList;
  //! a map slot to the attribute list
  std::map<int, Values> m_slotIdToValuesMap;
  //! the set of style
  std::map<StyleId,StarItemStyle> m_styleIdToStyleMap;
  //! map simplify style name to style name
  std::map<librevenge::RVNGString, librevenge::RVNGString> m_simplifyNameToStyleNameMap;
  //! map of created default attribute
  std::map<int,std::shared_ptr<StarAttribute> > m_idToDefaultMap;
  //! list of item which need to be read
  std::vector<std::shared_ptr<StarItem> > m_delayedItemList;
private:
  State(State const &orig) = delete;
  State operator=(State const &orig) = delete;
};

void State::init(StarItemPool::Type type)
{
  if (type==m_type) return;
  if (m_type!=StarItemPool::T_Unknown) {
    STOFF_DEBUG_MSG(("StarItemPoolInternal::State::init: arghhhh, change pool type\n"));
  }
  m_type=type;
  // reset internal data
  m_verStart=m_verEnd=m_currentVersion=0;
  m_versionList.clear();
  m_idToAttributeList.clear();

  // to do VCControls
  switch (type) {
  case StarItemPool::T_ChartPool: {
    // sch_itempool.cxx SchItemPool::SchItemPool
    m_verStart=1; // SCHATTR_START
    m_verEnd=100; // SCHATTR_NONPERSISTENT_START
    // svx_eerdll.cxx GlobalEditData::GetDefItems
    static int const what[]= {
      StarAttribute::ATTR_SCH_DATADESCR_DESCR, StarAttribute::ATTR_SCH_DATADESCR_SHOW_SYM, StarAttribute::ATTR_SCH_LEGEND_POS, StarAttribute::ATTR_SCH_TEXT_ORIENT,
      StarAttribute::ATTR_SCH_TEXT_ORDER, StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MIN, StarAttribute::ATTR_SCH_Y_AXIS_MIN, StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MAX,
      StarAttribute::ATTR_SCH_Y_AXIS_MAX, StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN, StarAttribute::ATTR_SCH_Y_AXIS_STEP_MAIN, StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_HELP,
      StarAttribute::ATTR_SCH_Y_AXIS_STEP_HELP, StarAttribute::ATTR_SCH_Y_AXIS_LOGARITHM, StarAttribute::ATTR_SCH_Y_AXIS_AUTO_ORIGIN, StarAttribute::ATTR_SCH_Y_AXIS_ORIGIN,
      StarAttribute::ATTR_SCH_X_AXIS_AUTO_MIN, StarAttribute::ATTR_SCH_X_AXIS_MIN, StarAttribute::ATTR_SCH_X_AXIS_AUTO_MAX, StarAttribute::ATTR_SCH_X_AXIS_MAX,

      StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_MAIN, StarAttribute::ATTR_SCH_X_AXIS_STEP_MAIN, StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_HELP, StarAttribute::ATTR_SCH_X_AXIS_STEP_HELP,
      StarAttribute::ATTR_SCH_X_AXIS_LOGARITHM, StarAttribute::ATTR_SCH_X_AXIS_AUTO_ORIGIN, StarAttribute::ATTR_SCH_X_AXIS_ORIGIN, StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MIN,
      StarAttribute::ATTR_SCH_Z_AXIS_MIN, StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MAX, StarAttribute::ATTR_SCH_Z_AXIS_MAX, StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN,
      StarAttribute::ATTR_SCH_Z_AXIS_STEP_MAIN, StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_HELP, StarAttribute::ATTR_SCH_Z_AXIS_STEP_HELP, StarAttribute::ATTR_SCH_Z_AXIS_LOGARITHM,
      StarAttribute::ATTR_SCH_Z_AXIS_AUTO_ORIGIN, StarAttribute::ATTR_SCH_Z_AXIS_ORIGIN, StarAttribute::ATTR_SCH_AXISTYPE, StarAttribute::ATTR_SCH_DUMMY0,

      StarAttribute::ATTR_SCH_DUMMY1, StarAttribute::ATTR_SCH_DUMMY2, StarAttribute::ATTR_SCH_DUMMY3, StarAttribute::ATTR_SCH_DUMMY_END,
      StarAttribute::ATTR_SCH_STAT_AVERAGE, StarAttribute::ATTR_SCH_STAT_KIND_ERROR, StarAttribute::ATTR_SCH_STAT_PERCENT, StarAttribute::ATTR_SCH_STAT_BIGERROR,
      StarAttribute::ATTR_SCH_STAT_CONSTPLUS, StarAttribute::ATTR_SCH_STAT_CONSTMINUS, StarAttribute::ATTR_SCH_STAT_REGRESSTYPE, StarAttribute::ATTR_SCH_STAT_INDICATE,
      StarAttribute::ATTR_SCH_TEXT_DEGREES, StarAttribute::ATTR_SCH_TEXT_OVERLAP, StarAttribute::ATTR_SCH_TEXT_DUMMY0, StarAttribute::ATTR_SCH_TEXT_DUMMY1,
      StarAttribute::ATTR_SCH_TEXT_DUMMY2, StarAttribute::ATTR_SCH_TEXT_DUMMY3, StarAttribute::ATTR_SCH_STYLE_DEEP, StarAttribute::ATTR_SCH_STYLE_3D,

      StarAttribute::ATTR_SCH_STYLE_VERTICAL, StarAttribute::ATTR_SCH_STYLE_BASETYPE, StarAttribute::ATTR_SCH_STYLE_LINES, StarAttribute::ATTR_SCH_STYLE_PERCENT,
      StarAttribute::ATTR_SCH_STYLE_STACKED, StarAttribute::ATTR_SCH_STYLE_SPLINES, StarAttribute::ATTR_SCH_STYLE_SYMBOL, StarAttribute::ATTR_SCH_STYLE_SHAPE,
      StarAttribute::ATTR_SCH_AXIS, StarAttribute::ATTR_SCH_AXIS_AUTO_MIN, StarAttribute::ATTR_SCH_AXIS_MIN, StarAttribute::ATTR_SCH_AXIS_AUTO_MAX,
      StarAttribute::ATTR_SCH_AXIS_MAX, StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_MAIN, StarAttribute::ATTR_SCH_AXIS_STEP_MAIN, StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_HELP,
      StarAttribute::ATTR_SCH_AXIS_STEP_HELP, StarAttribute::ATTR_SCH_AXIS_LOGARITHM, StarAttribute::ATTR_SCH_AXIS_AUTO_ORIGIN, StarAttribute::ATTR_SCH_AXIS_ORIGIN,

      StarAttribute::ATTR_SCH_AXIS_TICKS, StarAttribute::ATTR_SCH_AXIS_NUMFMT, StarAttribute::ATTR_SCH_AXIS_NUMFMTPERCENT, StarAttribute::ATTR_SCH_AXIS_SHOWAXIS,
      StarAttribute::ATTR_SCH_AXIS_SHOWDESCR, StarAttribute::ATTR_SCH_AXIS_SHOWMAINGRID, StarAttribute::ATTR_SCH_AXIS_SHOWHELPGRID, StarAttribute::ATTR_SCH_AXIS_TOPDOWN,
      StarAttribute::ATTR_SCH_AXIS_HELPTICKS, StarAttribute::ATTR_SCH_AXIS_DUMMY0, StarAttribute::ATTR_SCH_AXIS_DUMMY1, StarAttribute::ATTR_SCH_AXIS_DUMMY2,
      StarAttribute::ATTR_SCH_AXIS_DUMMY3, StarAttribute::ATTR_SCH_BAR_OVERLAP, StarAttribute::ATTR_SCH_BAR_GAPWIDTH, StarAttribute::ATTR_SCH_SYMBOL_BRUSH,
      StarAttribute::ATTR_SCH_STOCK_VOLUME, StarAttribute::ATTR_SCH_STOCK_UPDOWN, StarAttribute::ATTR_SCH_SYMBOL_SIZE, StarAttribute::ATTR_SCH_USER_DEFINED_ATTR
    };

    for (auto wh : what)
      m_idToAttributeList.push_back(wh);
    break;
  }
  case StarItemPool::T_EditEnginePool: {
    m_verStart=3989;
    m_verEnd=4037;
    // svx_editdoc.cxx
    std::vector<int> list;
    for (int i = 0; i <= 14; ++i) list.push_back(3999+i);
    for (int i = 15; i <= 17; ++i) list.push_back(3999+i+3);
    addVersionMap(1, 3999, list);

    list.clear();
    for (int i = 0; i <= 17; ++i) list.push_back(3999+i);
    for (int i=18; i<=20; ++i)  list.push_back(3999+i+1);
    addVersionMap(2, 3999, list);

    list.clear();
    for (int i = 0; i <= 10; ++i) list.push_back(3997+i);
    for (int i=11; i<=23; ++i)  list.push_back(3997+i+1);
    addVersionMap(3, 3997, list);

    list.clear();
    for (int i = 0; i <= 24; ++i) list.push_back(3994+i);
    for (int i=25; i<=28; ++i)  list.push_back(3994+i+15);
    addVersionMap(4, 3994, list);
    // svx_eerdll.cxx GlobalEditData::GetDefItems
    static int const what[]= {
      StarAttribute::ATTR_SC_WRITINGDIR, StarAttribute::ATTR_EE_PARA_XMLATTRIBS, StarAttribute::ATTR_PARA_HANGINGPUNCTUATION, StarAttribute::ATTR_PARA_FORBIDDEN_RULES,
      StarAttribute::ATTR_EE_PARA_ASIANCJKSPACING, StarAttribute::ATTR_EE_PARA_NUMBULLET, StarAttribute::ATTR_SC_HYPHENATE, StarAttribute::ATTR_EE_PARA_BULLETSTATE,
      StarAttribute::ATTR_EE_PARA_OUTLLR_SPACE, StarAttribute::ATTR_EE_PARA_OUTLLEVEL, StarAttribute::ATTR_EE_PARA_BULLET, StarAttribute::ATTR_FRM_LR_SPACE,
      StarAttribute::ATTR_FRM_UL_SPACE, StarAttribute::ATTR_PARA_LINESPACING, StarAttribute::ATTR_PARA_ADJUST, StarAttribute::ATTR_PARA_TABSTOP,
      StarAttribute::ATTR_CHR_COLOR, StarAttribute::ATTR_CHR_FONT, StarAttribute::ATTR_CHR_FONTSIZE, StarAttribute::ATTR_EE_CHR_SCALEW,

      StarAttribute::ATTR_CHR_WEIGHT, StarAttribute::ATTR_CHR_UNDERLINE, StarAttribute::ATTR_CHR_CROSSEDOUT, StarAttribute::ATTR_CHR_POSTURE,
      StarAttribute::ATTR_CHR_CONTOUR, StarAttribute::ATTR_CHR_SHADOWED, StarAttribute::ATTR_CHR_ESCAPEMENT, StarAttribute::ATTR_CHR_AUTOKERN,
      StarAttribute::ATTR_CHR_KERNING, StarAttribute::ATTR_CHR_WORDLINEMODE, StarAttribute::ATTR_CHR_LANGUAGE, StarAttribute::ATTR_CHR_CJK_LANGUAGE,
      StarAttribute::ATTR_CHR_CTL_LANGUAGE, StarAttribute::ATTR_CHR_CJK_FONT, StarAttribute::ATTR_CHR_CTL_FONT, StarAttribute::ATTR_CHR_CJK_FONTSIZE,
      StarAttribute::ATTR_CHR_CTL_FONTSIZE, StarAttribute::ATTR_CHR_CJK_WEIGHT, StarAttribute::ATTR_CHR_CTL_WEIGHT, StarAttribute::ATTR_CHR_CJK_POSTURE,

      StarAttribute::ATTR_CHR_CTL_POSTURE, StarAttribute::ATTR_CHR_EMPHASIS_MARK, StarAttribute::ATTR_CHR_RELIEF, StarAttribute::ATTR_EE_CHR_RUBI_DUMMY,
      StarAttribute::ATTR_EE_CHR_XMLATTRIBS, StarAttribute::ATTR_EE_FEATURE_TAB, StarAttribute::ATTR_EE_FEATURE_LINEBR, StarAttribute::ATTR_CHR_CHARSETCOLOR,
      StarAttribute::ATTR_EE_FEATURE_FIELD
    };
    for (auto wh : what)
      m_idToAttributeList.push_back(wh);
    break;
  }
  case StarItemPool::T_SpreadsheetPool: {
    // sc_docpool.cxx
    m_verStart=100; // ATTR_STARTINDEX
    m_verEnd=183; // ATTR_ENDINDEX

    std::vector<int> list;
    for (int i = 0; i <= 17; i++) list.push_back(100+i);
    for (int i = 18; i <= 57; i++) list.push_back(100+i+1);
    addVersionMap(1, 100, list);

    list.clear();
    for (int i = 0; i <= 23; i++) list.push_back(100+i);
    for (int i = 24; i <= 58; i++) list.push_back(100+i+2);
    addVersionMap(2, 100, list);

    list.clear();
    for (int i = 0; i <= 10; i++) list.push_back(100+i);
    for (int i = 11; i <= 60; i++) list.push_back(100+i+1);
    addVersionMap(3, 100, list);

    list.clear();
    for (int i = 0; i <= 13; i++) list.push_back(100+i);
    for (int i = 14; i <= 61; i++) list.push_back(100+i+2);
    addVersionMap(4, 100, list);

    list.clear();
    for (int i = 0; i <= 9; i++) list.push_back(100+i);
    for (int i = 10; i <= 63; i++) list.push_back(100+i+12);
    addVersionMap(5, 100, list);

    list.clear();
    for (int i = 0; i <= 21; i++) list.push_back(100+i);
    for (int i = 22; i <= 75; i++) list.push_back(100+i+3);
    addVersionMap(6, 100, list);

    list.clear();
    for (int i = 0; i <= 21; i++) list.push_back(100+i);
    for (int i = 22; i <= 78; i++) list.push_back(100+i+3);
    addVersionMap(7, 100, list);

    list.clear();
    for (int i = 0; i <= 33; i++) list.push_back(100+i);
    for (int i = 34; i <= 81; i++) list.push_back(100+i+1);
    addVersionMap(8, 100, list);

    list.clear();
    for (int i = 0; i <= 34; i++) list.push_back(100+i);
    for (int i = 35; i <= 82; i++) list.push_back(100+i+1);
    addVersionMap(9, 100, list);
    static int const what[]= {
      StarAttribute::ATTR_CHR_FONT, StarAttribute::ATTR_CHR_FONTSIZE, StarAttribute::ATTR_CHR_WEIGHT, StarAttribute::ATTR_CHR_POSTURE,
      StarAttribute::ATTR_CHR_UNDERLINE, StarAttribute::ATTR_CHR_CROSSEDOUT, StarAttribute::ATTR_CHR_CONTOUR, StarAttribute::ATTR_CHR_SHADOWED,
      StarAttribute::ATTR_CHR_COLOR, StarAttribute::ATTR_CHR_LANGUAGE, StarAttribute::ATTR_CHR_CJK_FONT, StarAttribute::ATTR_CHR_CJK_FONTSIZE,
      StarAttribute::ATTR_CHR_CJK_WEIGHT, StarAttribute::ATTR_CHR_CJK_POSTURE, StarAttribute::ATTR_CHR_CJK_LANGUAGE, StarAttribute::ATTR_CHR_CTL_FONT,
      StarAttribute::ATTR_CHR_CTL_FONTSIZE, StarAttribute::ATTR_CHR_CTL_WEIGHT, StarAttribute::ATTR_CHR_CTL_POSTURE, StarAttribute::ATTR_CHR_CTL_LANGUAGE,

      StarAttribute::ATTR_CHR_EMPHASIS_MARK, StarAttribute::ATTR_SC_USERDEF, StarAttribute::ATTR_CHR_WORDLINEMODE, StarAttribute::ATTR_CHR_RELIEF,
      StarAttribute::ATTR_SC_HYPHENATE, StarAttribute::ATTR_PARA_SCRIPTSPACE, StarAttribute::ATTR_PARA_HANGINGPUNCTUATION, StarAttribute::ATTR_PARA_FORBIDDEN_RULES,
      StarAttribute::ATTR_SC_HORJUSTIFY, StarAttribute::ATTR_SC_INDENT, StarAttribute::ATTR_SC_VERJUSTIFY, StarAttribute::ATTR_SC_ORIENTATION,
      StarAttribute::ATTR_SC_ROTATE_VALUE, StarAttribute::ATTR_SC_ROTATE_MODE, StarAttribute::ATTR_SC_VERTICAL_ASIAN, StarAttribute::ATTR_SC_WRITINGDIR,
      StarAttribute::ATTR_SC_LINEBREAK, StarAttribute::ATTR_SC_MARGIN, StarAttribute::ATTR_SC_MERGE, StarAttribute::ATTR_SC_MERGE_FLAG,

      StarAttribute::ATTR_SC_VALUE_FORMAT, StarAttribute::ATTR_SC_LANGUAGE_FORMAT, StarAttribute::ATTR_SC_BACKGROUND, StarAttribute::ATTR_SC_PROTECTION,
      StarAttribute::ATTR_SC_BORDER, StarAttribute::ATTR_SC_BORDER_INNER, StarAttribute::ATTR_SC_SHADOW, StarAttribute::ATTR_SC_VALIDDATA,
      StarAttribute::ATTR_SC_CONDITIONAL, StarAttribute::ATTR_SC_PATTERN, StarAttribute::ATTR_FRM_LR_SPACE, StarAttribute::ATTR_FRM_UL_SPACE,
      StarAttribute::ATTR_SC_PAGE, StarAttribute::ATTR_SC_PAGE_PAPERTRAY, StarAttribute::ATTR_FRM_PAPER_BIN, StarAttribute::ATTR_SC_PAGE_SIZE,
      StarAttribute::ATTR_SC_PAGE_MAXSIZE, StarAttribute::ATTR_SC_PAGE_HORCENTER, StarAttribute::ATTR_SC_PAGE_VERCENTER, StarAttribute::ATTR_SC_PAGE_ON,

      StarAttribute::ATTR_SC_PAGE_DYNAMIC, StarAttribute::ATTR_SC_PAGE_SHARED, StarAttribute::ATTR_SC_PAGE_NOTES, StarAttribute::ATTR_SC_PAGE_GRID,
      StarAttribute::ATTR_SC_PAGE_HEADERS, StarAttribute::ATTR_SC_PAGE_CHARTS, StarAttribute::ATTR_SC_PAGE_OBJECTS, StarAttribute::ATTR_SC_PAGE_DRAWINGS,
      StarAttribute::ATTR_SC_PAGE_TOPDOWN, StarAttribute::ATTR_SC_PAGE_SCALE, StarAttribute::ATTR_SC_PAGE_SCALETOPAGES, StarAttribute::ATTR_SC_PAGE_FIRSTPAGENO,
      StarAttribute::ATTR_SC_PAGE_PRINTAREA, StarAttribute::ATTR_SC_PAGE_REPEATROW, StarAttribute::ATTR_SC_PAGE_REPEATCOL, StarAttribute::ATTR_SC_PAGE_PRINTTABLES,
      StarAttribute::ATTR_SC_PAGE_HEADERLEFT, StarAttribute::ATTR_SC_PAGE_FOOTERLEFT, StarAttribute::ATTR_SC_PAGE_HEADERRIGHT, StarAttribute::ATTR_SC_PAGE_FOOTERRIGHT,
      StarAttribute::ATTR_SC_PAGE_HEADERSET, StarAttribute::ATTR_SC_PAGE_FOOTERSET, StarAttribute::ATTR_SC_PAGE_FORMULAS, StarAttribute::ATTR_SC_PAGE_NULLVALS
    };
    for (auto wh : what)
      m_idToAttributeList.push_back(wh);
    break;
  }
  case StarItemPool::T_VCControlPool: // never seens with data, so
    break;
  case StarItemPool::T_WriterPool: {
    // SwAttrPool::SwAttrPool set default map
    m_verStart=1; //POOLATTR_BEGIN
    m_verEnd=130; //POOLATTR_END-1
    for (int i=StarAttribute::ATTR_CHR_CASEMAP; i<=StarAttribute::ATTR_BOX_VALUE; ++i)
      m_idToAttributeList.push_back(i);
    std::vector<int> list;
    // sw_swatrset.cxx SwAttrPool::SwAttrPool and sw_init.cxx pVersionMap1
    for (int i = 1; i <= 17; i++) list.push_back(i);
    for (int i = 18; i <= 27; i++) list.push_back(i+5);
    for (int i = 28; i <= 35; i++) list.push_back(i+7);
    for (int i = 36; i <= 58; i++) list.push_back(i+10);
    for (int i = 59; i <= 60; i++) list.push_back(i+12);
    addVersionMap(1, 1, list);
    list.clear();
    for (int i = 1; i <= 70; i++) list.push_back(i);
    for (int i = 71; i <= 75; i++) list.push_back(i+10);
    addVersionMap(2, 1, list);
    list.clear();
    for (int i = 1; i <= 21; i++) list.push_back(i);
    for (int i = 22; i <= 27; i++) list.push_back(i+15);
    for (int i = 28; i <= 82; i++) list.push_back(i+20);
    for (int i = 83; i <= 86; i++) list.push_back(i+35);
    addVersionMap(3, 1, list);
    list.clear();
    for (int i = 1; i <= 65; i++) list.push_back(i);
    for (int i = 66; i <= 121; i++) list.push_back(i+9);
    addVersionMap(4, 1, list);
    break;
  }
  case StarItemPool::T_XOutdevPool: {
    // svx_xpool.cxx XOutdevItemPool::Ctor and svx_svdattr.cxx SdrItemPool::Ctor
    m_verStart=1000;
    m_verEnd=1333;
    std::vector<int> list;
    for (int i=1000; i<=1021; ++i) list.push_back(i);
    for (int i=1022; i<=1039; ++i) list.push_back(i+13);
    addVersionMap(1, 1000, list);

    list.clear();
    for (int i=1000; i<=1009; ++i) list.push_back(i);
    for (int i=1010; i<=1015; ++i) list.push_back(i+7);
    for (int i=1016; i<=1039; ++i) list.push_back(i+14);
    for (int i=1040; i<=1050; ++i) list.push_back(i+22);
    for (int i=1051; i<=1056; ++i) list.push_back(i+27);
    for (int i=1057; i<=1065; ++i) list.push_back(i+52);
    addVersionMap(2, 1000, list);

    list.clear();
    for (int i=1000; i<=1029; ++i) list.push_back(i);
    for (int i=1030; i<=1123; ++i) list.push_back(i+17);
    addVersionMap(3, 1000, list);

    list.clear();
    for (int i=1000; i<=1126; ++i) list.push_back(i);
    for (int i=1127; i<=1140; ++i) list.push_back(i+45);
    addVersionMap(4, 1000, list);

    static int const what[]= {
      StarAttribute::XATTR_LINESTYLE, StarAttribute::XATTR_LINEDASH, StarAttribute::XATTR_LINEWIDTH, StarAttribute::XATTR_LINECOLOR,
      StarAttribute::XATTR_LINESTART, StarAttribute::XATTR_LINEEND, StarAttribute::XATTR_LINESTARTWIDTH, StarAttribute::XATTR_LINEENDWIDTH,
      StarAttribute::XATTR_LINESTARTCENTER, StarAttribute::XATTR_LINEENDCENTER, StarAttribute::XATTR_LINETRANSPARENCE, StarAttribute::XATTR_LINEJOINT,
      StarAttribute::XATTR_LINERESERVED2, StarAttribute::XATTR_LINERESERVED3, StarAttribute::XATTR_LINERESERVED4,StarAttribute::XATTR_LINERESERVED5,
      StarAttribute::XATTR_LINERESERVED_LAST, StarAttribute::XATTR_SET_LINE, StarAttribute::XATTR_FILLSTYLE, StarAttribute::XATTR_FILLCOLOR,

      StarAttribute::XATTR_FILLGRADIENT, StarAttribute::XATTR_FILLHATCH, StarAttribute::XATTR_FILLBITMAP, StarAttribute::XATTR_FILLTRANSPARENCE,
      StarAttribute::XATTR_GRADIENTSTEPCOUNT, StarAttribute::XATTR_FILLBMP_TILE, StarAttribute::XATTR_FILLBMP_POS, StarAttribute::XATTR_FILLBMP_SIZEX,
      StarAttribute::XATTR_FILLBMP_SIZEY, StarAttribute::XATTR_FILLFLOATTRANSPARENCE, StarAttribute::XATTR_FILLRESERVED2, StarAttribute::XATTR_FILLBMP_SIZELOG,
      StarAttribute::XATTR_FILLBMP_TILEOFFSETX, StarAttribute::XATTR_FILLBMP_TILEOFFSETY, StarAttribute::XATTR_FILLBMP_STRETCH, StarAttribute::XATTR_FILLRESERVED3,
      StarAttribute::XATTR_FILLRESERVED4, StarAttribute::XATTR_FILLRESERVED5, StarAttribute::XATTR_FILLRESERVED6, StarAttribute::XATTR_FILLRESERVED7,

      StarAttribute::XATTR_FILLRESERVED8, StarAttribute::XATTR_FILLBMP_POSOFFSETX, StarAttribute::XATTR_FILLBMP_POSOFFSETY, StarAttribute::XATTR_FILLBACKGROUND,
      StarAttribute::XATTR_FILLRESERVED10, StarAttribute::XATTR_FILLRESERVED11, StarAttribute::XATTR_FILLRESERVED_LAST, StarAttribute::XATTR_SET_FILL,
      StarAttribute::XATTR_FORMTXTSTYLE, StarAttribute::XATTR_FORMTXTADJUST, StarAttribute::XATTR_FORMTXTDISTANCE, StarAttribute::XATTR_FORMTXTSTART,
      StarAttribute::XATTR_FORMTXTMIRROR, StarAttribute::XATTR_FORMTXTOUTLINE, StarAttribute::XATTR_FORMTXTSHADOW, StarAttribute::XATTR_FORMTXTSHDWCOLOR,
      StarAttribute::XATTR_FORMTXTSHDWXVAL, StarAttribute::XATTR_FORMTXTSHDWYVAL, StarAttribute::XATTR_FORMTXTSTDFORM, StarAttribute::XATTR_FORMTXTHIDEFORM,

      StarAttribute::XATTR_FORMTXTSHDWTRANSP, StarAttribute::XATTR_FTRESERVED2, StarAttribute::XATTR_FTRESERVED3, StarAttribute::XATTR_FTRESERVED4,
      StarAttribute::XATTR_FTRESERVED5, StarAttribute::XATTR_FTRESERVED_LAST, StarAttribute::XATTR_SET_TEXT,

      // SDR 1067...
      StarAttribute::SDRATTR_SHADOW, StarAttribute::SDRATTR_SHADOWCOLOR, StarAttribute::SDRATTR_SHADOWXDIST, StarAttribute::SDRATTR_SHADOWYDIST,
      StarAttribute::SDRATTR_SHADOWTRANSPARENCE, StarAttribute::SDRATTR_SHADOW3D, StarAttribute::SDRATTR_SHADOWPERSP, StarAttribute::SDRATTR_SHADOWRESERVE1,
      StarAttribute::SDRATTR_SHADOWRESERVE2, StarAttribute::SDRATTR_SHADOWRESERVE3, StarAttribute::SDRATTR_SHADOWRESERVE4, StarAttribute::SDRATTR_SHADOWRESERVE5,
      StarAttribute::SDRATTR_SET_SHADOW, StarAttribute::SDRATTR_CAPTIONTYPE, StarAttribute::SDRATTR_CAPTIONFIXEDANGLE, StarAttribute::SDRATTR_CAPTIONANGLE,
      StarAttribute::SDRATTR_CAPTIONGAP, StarAttribute::SDRATTR_CAPTIONESCDIR, StarAttribute::SDRATTR_CAPTIONESCISREL, StarAttribute::SDRATTR_CAPTIONESCREL,

      StarAttribute::SDRATTR_CAPTIONESCABS, StarAttribute::SDRATTR_CAPTIONLINELEN, StarAttribute::SDRATTR_CAPTIONFITLINELEN, StarAttribute::SDRATTR_CAPTIONRESERVE1,
      StarAttribute::SDRATTR_CAPTIONRESERVE2, StarAttribute::SDRATTR_CAPTIONRESERVE3, StarAttribute::SDRATTR_CAPTIONRESERVE4, StarAttribute::SDRATTR_CAPTIONRESERVE5,
      StarAttribute::SDRATTR_SET_CAPTION, StarAttribute::SDRATTR_SET_OUTLINER, StarAttribute::SDRATTR_ECKENRADIUS, StarAttribute::SDRATTR_TEXT_MINFRAMEHEIGHT,
      StarAttribute::SDRATTR_TEXT_AUTOGROWHEIGHT, StarAttribute::SDRATTR_TEXT_FITTOSIZE, StarAttribute::SDRATTR_TEXT_LEFTDIST, StarAttribute::SDRATTR_TEXT_RIGHTDIST,
      StarAttribute::SDRATTR_TEXT_UPPERDIST, StarAttribute::SDRATTR_TEXT_LOWERDIST, StarAttribute::SDRATTR_TEXT_VERTADJUST, StarAttribute::SDRATTR_TEXT_MAXFRAMEHEIGHT,

      StarAttribute::SDRATTR_TEXT_MINFRAMEWIDTH, StarAttribute::SDRATTR_TEXT_MAXFRAMEWIDTH, StarAttribute::SDRATTR_TEXT_AUTOGROWWIDTH, StarAttribute::SDRATTR_TEXT_HORZADJUST,
      StarAttribute::SDRATTR_TEXT_ANIKIND, StarAttribute::SDRATTR_TEXT_ANIDIRECTION, StarAttribute::SDRATTR_TEXT_ANISTARTINSIDE, StarAttribute::SDRATTR_TEXT_ANISTOPINSIDE,
      StarAttribute::SDRATTR_TEXT_ANICOUNT, StarAttribute::SDRATTR_TEXT_ANIDELAY, StarAttribute::SDRATTR_TEXT_ANIAMOUNT, StarAttribute::SDRATTR_TEXT_CONTOURFRAME,
      StarAttribute::SDRATTR_AUTOSHAPE_ADJUSTMENT, StarAttribute::SDRATTR_XMLATTRIBUTES, StarAttribute::SDRATTR_RESERVE15, StarAttribute::SDRATTR_RESERVE16,
      StarAttribute::SDRATTR_RESERVE17, StarAttribute::SDRATTR_RESERVE18, StarAttribute::SDRATTR_RESERVE19, StarAttribute::SDRATTR_SET_MISC,

      StarAttribute::SDRATTR_EDGEKIND, StarAttribute::SDRATTR_EDGENODE1HORZDIST, StarAttribute::SDRATTR_EDGENODE1VERTDIST, StarAttribute::SDRATTR_EDGENODE2HORZDIST,
      StarAttribute::SDRATTR_EDGENODE2VERTDIST, StarAttribute::SDRATTR_EDGENODE1GLUEDIST, StarAttribute::SDRATTR_EDGENODE2GLUEDIST, StarAttribute::SDRATTR_EDGELINEDELTAANZ,
      StarAttribute::SDRATTR_EDGELINE1DELTA, StarAttribute::SDRATTR_EDGELINE2DELTA, StarAttribute::SDRATTR_EDGELINE3DELTA, StarAttribute::SDRATTR_EDGERESERVE02,
      StarAttribute::SDRATTR_EDGERESERVE03, StarAttribute::SDRATTR_EDGERESERVE04, StarAttribute::SDRATTR_EDGERESERVE05, StarAttribute::SDRATTR_EDGERESERVE06,
      StarAttribute::SDRATTR_EDGERESERVE07, StarAttribute::SDRATTR_EDGERESERVE08, StarAttribute::SDRATTR_EDGERESERVE09, StarAttribute::SDRATTR_SET_EDGE,

      StarAttribute::SDRATTR_MEASUREKIND, StarAttribute::SDRATTR_MEASURETEXTHPOS, StarAttribute::SDRATTR_MEASURETEXTVPOS, StarAttribute::SDRATTR_MEASURELINEDIST,
      StarAttribute::SDRATTR_MEASUREHELPLINEOVERHANG, StarAttribute::SDRATTR_MEASUREHELPLINEDIST, StarAttribute::SDRATTR_MEASUREHELPLINE1LEN, StarAttribute::SDRATTR_MEASUREHELPLINE2LEN,
      StarAttribute::SDRATTR_MEASUREBELOWREFEDGE, StarAttribute::SDRATTR_MEASURETEXTROTA90, StarAttribute::SDRATTR_MEASURETEXTUPSIDEDOWN, StarAttribute::SDRATTR_MEASUREOVERHANG,
      StarAttribute::SDRATTR_MEASUREUNIT, StarAttribute::SDRATTR_MEASURESCALE, StarAttribute::SDRATTR_MEASURESHOWUNIT, StarAttribute::SDRATTR_MEASUREFORMATSTRING,
      StarAttribute::SDRATTR_MEASURETEXTAUTOANGLE, StarAttribute::SDRATTR_MEASURETEXTAUTOANGLEVIEW, StarAttribute::SDRATTR_MEASURETEXTISFIXEDANGLE, StarAttribute::SDRATTR_MEASURETEXTFIXEDANGLE,
      // 1167
      StarAttribute::SDRATTR_MEASUREDECIMALPLACES, StarAttribute::SDRATTR_MEASURERESERVE05, StarAttribute::SDRATTR_MEASURERESERVE06, StarAttribute::SDRATTR_MEASURERESERVE07,
      StarAttribute::SDRATTR_SET_MEASURE, StarAttribute::SDRATTR_CIRCKIND, StarAttribute::SDRATTR_CIRCSTARTANGLE, StarAttribute::SDRATTR_CIRCENDANGLE,
      StarAttribute::SDRATTR_CIRCRESERVE0, StarAttribute::SDRATTR_CIRCRESERVE1, StarAttribute::SDRATTR_CIRCRESERVE2, StarAttribute::SDRATTR_CIRCRESERVE3,
      StarAttribute::SDRATTR_SET_CIRC, StarAttribute::SDRATTR_OBJMOVEPROTECT, StarAttribute::SDRATTR_OBJSIZEPROTECT, StarAttribute::SDRATTR_OBJPRINTABLE,
      StarAttribute::SDRATTR_LAYERID, StarAttribute::SDRATTR_LAYERNAME, StarAttribute::SDRATTR_OBJECTNAME, StarAttribute::SDRATTR_ALLPOSITIONX,

      StarAttribute::SDRATTR_ALLPOSITIONY, StarAttribute::SDRATTR_ALLSIZEWIDTH, StarAttribute::SDRATTR_ALLSIZEHEIGHT, StarAttribute::SDRATTR_ONEPOSITIONX,
      StarAttribute::SDRATTR_ONEPOSITIONY, StarAttribute::SDRATTR_ONESIZEWIDTH, StarAttribute::SDRATTR_ONESIZEHEIGHT, StarAttribute::SDRATTR_LOGICSIZEWIDTH,
      StarAttribute::SDRATTR_LOGICSIZEHEIGHT, StarAttribute::SDRATTR_ROTATEANGLE, StarAttribute::SDRATTR_SHEARANGLE, StarAttribute::SDRATTR_MOVEX,
      StarAttribute::SDRATTR_MOVEY, StarAttribute::SDRATTR_RESIZEXONE, StarAttribute::SDRATTR_RESIZEYONE, StarAttribute::SDRATTR_ROTATEONE,
      StarAttribute::SDRATTR_HORZSHEARONE, StarAttribute::SDRATTR_VERTSHEARONE, StarAttribute::SDRATTR_RESIZEXALL, StarAttribute::SDRATTR_RESIZEYALL,

      StarAttribute::SDRATTR_ROTATEALL, StarAttribute::SDRATTR_HORZSHEARALL, StarAttribute::SDRATTR_VERTSHEARALL, StarAttribute::SDRATTR_TRANSFORMREF1X,
      StarAttribute::SDRATTR_TRANSFORMREF1Y, StarAttribute::SDRATTR_TRANSFORMREF2X, StarAttribute::SDRATTR_TRANSFORMREF2Y, StarAttribute::SDRATTR_TEXTDIRECTION,
      StarAttribute::SDRATTR_NOTPERSISTRESERVE2, StarAttribute::SDRATTR_NOTPERSISTRESERVE3, StarAttribute::SDRATTR_NOTPERSISTRESERVE4, StarAttribute::SDRATTR_NOTPERSISTRESERVE5,
      StarAttribute::SDRATTR_NOTPERSISTRESERVE6, StarAttribute::SDRATTR_NOTPERSISTRESERVE7, StarAttribute::SDRATTR_NOTPERSISTRESERVE8, StarAttribute::SDRATTR_NOTPERSISTRESERVE9,
      StarAttribute::SDRATTR_NOTPERSISTRESERVE10, StarAttribute::SDRATTR_NOTPERSISTRESERVE11, StarAttribute::SDRATTR_NOTPERSISTRESERVE12, StarAttribute::SDRATTR_NOTPERSISTRESERVE13,

      StarAttribute::SDRATTR_NOTPERSISTRESERVE14, StarAttribute::SDRATTR_NOTPERSISTRESERVE15, StarAttribute::SDRATTR_GRAFRED, StarAttribute::SDRATTR_GRAFGREEN,
      StarAttribute::SDRATTR_GRAFBLUE, StarAttribute::SDRATTR_GRAFLUMINANCE, StarAttribute::SDRATTR_GRAFCONTRAST, StarAttribute::SDRATTR_GRAFGAMMA,
      StarAttribute::SDRATTR_GRAFTRANSPARENCE, StarAttribute::SDRATTR_GRAFINVERT, StarAttribute::SDRATTR_GRAFMODE, StarAttribute::SDRATTR_GRAFCROP,
      StarAttribute::SDRATTR_GRAFRESERVE3, StarAttribute::SDRATTR_GRAFRESERVE4, StarAttribute::SDRATTR_GRAFRESERVE5, StarAttribute::SDRATTR_GRAFRESERVE6,
      StarAttribute::SDRATTR_SET_GRAF, StarAttribute::SDRATTR_3DOBJ_PERCENT_DIAGONAL, StarAttribute::SDRATTR_3DOBJ_BACKSCALE, StarAttribute::SDRATTR_3DOBJ_DEPTH,

      StarAttribute::SDRATTR_3DOBJ_HORZ_SEGS, StarAttribute::SDRATTR_3DOBJ_VERT_SEGS, StarAttribute::SDRATTR_3DOBJ_END_ANGLE, StarAttribute::SDRATTR_3DOBJ_DOUBLE_SIDED,
      StarAttribute::SDRATTR_3DOBJ_NORMALS_KIND, StarAttribute::SDRATTR_3DOBJ_NORMALS_INVERT, StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_X, StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_Y,
      StarAttribute::SDRATTR_3DOBJ_SHADOW_3D, StarAttribute::SDRATTR_3DOBJ_MAT_COLOR, StarAttribute::SDRATTR_3DOBJ_MAT_EMISSION, StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR,
      StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY, StarAttribute::SDRATTR_3DOBJ_TEXTURE_KIND, StarAttribute::SDRATTR_3DOBJ_TEXTURE_MODE, StarAttribute::SDRATTR_3DOBJ_TEXTURE_FILTER,
      StarAttribute::SDRATTR_3DOBJ_SMOOTH_NORMALS, StarAttribute::SDRATTR_3DOBJ_SMOOTH_LIDS, StarAttribute::SDRATTR_3DOBJ_CHARACTER_MODE, StarAttribute::SDRATTR_3DOBJ_CLOSE_FRONT,
      //1267
      StarAttribute::SDRATTR_3DOBJ_CLOSE_BACK, StarAttribute::SDRATTR_3DOBJ_RESERVED_06, StarAttribute::SDRATTR_3DOBJ_RESERVED_07, StarAttribute::SDRATTR_3DOBJ_RESERVED_08,
      StarAttribute::SDRATTR_3DOBJ_RESERVED_09, StarAttribute::SDRATTR_3DOBJ_RESERVED_10, StarAttribute::SDRATTR_3DOBJ_RESERVED_11, StarAttribute::SDRATTR_3DOBJ_RESERVED_12,
      StarAttribute::SDRATTR_3DOBJ_RESERVED_13, StarAttribute::SDRATTR_3DOBJ_RESERVED_14, StarAttribute::SDRATTR_3DOBJ_RESERVED_15, StarAttribute::SDRATTR_3DOBJ_RESERVED_16,
      StarAttribute::SDRATTR_3DOBJ_RESERVED_17, StarAttribute::SDRATTR_3DOBJ_RESERVED_18, StarAttribute::SDRATTR_3DOBJ_RESERVED_19, StarAttribute::SDRATTR_3DOBJ_RESERVED_20,
      StarAttribute::SDRATTR_3DSCENE_PERSPECTIVE, StarAttribute::SDRATTR_3DSCENE_DISTANCE, StarAttribute::SDRATTR_3DSCENE_FOCAL_LENGTH, StarAttribute::SDRATTR_3DSCENE_TWO_SIDED_LIGHTING,

      StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1, StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_2, StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_3, StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_4,
      StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_5, StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_6, StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_7, StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_8,
      StarAttribute::SDRATTR_3DSCENE_AMBIENTCOLOR, StarAttribute::SDRATTR_3DSCENE_LIGHTON_1, StarAttribute::SDRATTR_3DSCENE_LIGHTON_2, StarAttribute::SDRATTR_3DSCENE_LIGHTON_3,
      StarAttribute::SDRATTR_3DSCENE_LIGHTON_4, StarAttribute::SDRATTR_3DSCENE_LIGHTON_5, StarAttribute::SDRATTR_3DSCENE_LIGHTON_6, StarAttribute::SDRATTR_3DSCENE_LIGHTON_7,
      StarAttribute::SDRATTR_3DSCENE_LIGHTON_8, StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1, StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_2, StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_3,

      StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_4, StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_5, StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_6, StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_7,
      StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_8, StarAttribute::SDRATTR_3DSCENE_SHADOW_SLANT, StarAttribute::SDRATTR_3DSCENE_SHADE_MODE, StarAttribute::SDRATTR_3DSCENE_RESERVED_01,
      StarAttribute::SDRATTR_3DSCENE_RESERVED_02, StarAttribute::SDRATTR_3DSCENE_RESERVED_03, StarAttribute::SDRATTR_3DSCENE_RESERVED_04, StarAttribute::SDRATTR_3DSCENE_RESERVED_05,
      StarAttribute::SDRATTR_3DSCENE_RESERVED_06, StarAttribute::SDRATTR_3DSCENE_RESERVED_07, StarAttribute::SDRATTR_3DSCENE_RESERVED_08, StarAttribute::SDRATTR_3DSCENE_RESERVED_09,
      StarAttribute::SDRATTR_3DSCENE_RESERVED_10, StarAttribute::SDRATTR_3DSCENE_RESERVED_11, StarAttribute::SDRATTR_3DSCENE_RESERVED_12, StarAttribute::SDRATTR_3DSCENE_RESERVED_13,

      StarAttribute::SDRATTR_3DSCENE_RESERVED_14, StarAttribute::SDRATTR_3DSCENE_RESERVED_15, StarAttribute::SDRATTR_3DSCENE_RESERVED_16, StarAttribute::SDRATTR_3DSCENE_RESERVED_17,
      StarAttribute::SDRATTR_3DSCENE_RESERVED_18, StarAttribute::SDRATTR_3DSCENE_RESERVED_19, StarAttribute::SDRATTR_3DSCENE_RESERVED_20 /* 1333*/
    };
    for (auto wh : what)
      m_idToAttributeList.push_back(wh);
    break;
  }
  case StarItemPool::T_Unknown:
  default:
    break;
  }
}
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarItemPool::StarItemPool(StarObject &doc, StarItemPool::Type type)
  : m_isInside(false)
  , m_state(new StarItemPoolInternal::State(doc, type))
{
}

StarItemPool::~StarItemPool()
{
}

void StarItemPool::setRelativeUnit(double relUnit)
{
  m_state->m_relativeUnit=relUnit;
}

double StarItemPool::getRelativeUnit() const
{
  if (m_state->m_relativeUnit>0)
    return m_state->m_relativeUnit;
  return m_state->m_type==StarItemPool::T_XOutdevPool || m_state->m_type==StarItemPool::T_EditEnginePool ? 0.028346457 : 0.05;
}
void StarItemPool::clean()
{
  m_state->clean();
}

bool StarItemPool::isSecondaryPool() const
{
  return m_state->m_isSecondaryPool;
}

void StarItemPool::addSecondaryPool(std::shared_ptr<StarItemPool> secondary)
{
  if (!secondary) {
    STOFF_DEBUG_MSG(("StarItemPool::addSecondaryPool: called without pool\n"));
    return;
  }
  secondary->m_state->m_isSecondaryPool=true;
  if (m_state->m_secondaryPool)
    m_state->m_secondaryPool->addSecondaryPool(secondary);
  else
    m_state->m_secondaryPool=secondary;
}

int StarItemPool::getVersion() const
{
  return m_state->m_majorVersion;
}

StarItemPool::Type StarItemPool::getType() const
{
  return m_state->m_type;
}

std::shared_ptr<StarItem> StarItemPool::createItem(int which, int surrogateId, bool localId)
{
  std::shared_ptr<StarItem> res(new StarItem(which));
  res->m_surrogateId=surrogateId;
  res->m_localId=localId;
  m_state->m_delayedItemList.push_back(res);
  return res;
}

std::shared_ptr<StarAttribute> StarItemPool::readAttribute(StarZone &zone, int which, int vers, long endPos)
{
  if (m_state->m_currentVersion!=m_state->m_loadingVersion)
    which=m_state->getWhich(which);

  auto *state=m_state->getPoolStateFor(which);
  if (!state || which<state->m_verStart || which>=state->m_verStart+int(state->m_idToAttributeList.size()) ||
      !state->m_document.getAttributeManager()) {
    STOFFInputStreamPtr input=zone.input();
    long pos=input->tell();
    libstoff::DebugFile &ascii=zone.ascii();
    libstoff::DebugStream f;
    STOFF_DEBUG_MSG(("StarItemPool::readAttribute: find unknown attribute\n"));
    f << "Entries(StarAttribute)["<< zone.getRecordLevel() << "]:###wh=" << which;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    return StarAttributeManager::getDummyAttribute();
  }
  zone.openDummyRecord();
  auto attribute=state->m_document.getAttributeManager()->readAttribute
                 (zone, state->m_idToAttributeList[size_t(which-state->m_verStart)], vers, endPos, state->m_document);
  zone.closeDummyRecord();
  return attribute;
}

bool StarItemPool::read(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();

  if (pos+18>endPos)
    return false;
  uint16_t tag;
  uint8_t nMajorVers;
  *input >> tag >> nMajorVers;
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if ((tag!=0x1111 && tag!=0xbbbb) || nMajorVers<1 || nMajorVers>2)
    return false;
  StarItemPool *master=nullptr, *pool=this;
  // update the inside flag to true
  while (pool) {
    pool->m_isInside=true;
    pool=pool->m_state->m_secondaryPool.get();
  }
  bool ok=false;
  // read the pools
  pool=this;
  while (input->tell()<endPos) {
    if ((nMajorVers==2 && !pool->readV2(zone, master)) ||
        (nMajorVers==1 && !pool->readV1(zone, master)))
      break;
    ok=true;
    master=pool;
    pool=pool->m_state->m_secondaryPool.get();
    if (!pool) break;
  }
  // reset the inside flag to false
  pool=this;
  while (pool) {
    pool->m_isInside=false;
    pool=pool->m_state->m_secondaryPool.get();
  }
  // update the delayed item
  pool=this;
  while (pool) {
    for (auto it : pool->m_state->m_delayedItemList)
      loadSurrogate(*it);
    pool->m_state->m_delayedItemList.clear();
    pool=pool->m_state->m_secondaryPool.get();
  }
  return ok;
}

std::shared_ptr<StarItem> StarItemPool::readItem(StarZone &zone, bool isDirect, long endPos)
{
  // polio.cxx SfxItemPool::LoadItem
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarItem)["<< zone.getRecordLevel() << "]:";
  long pos=input->tell();
  if (pos+4>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readItem: the zone seems too short\n"));
    return std::shared_ptr<StarItem>();
  }
  uint16_t nWhich, nSlot;
  *input>>nWhich >> nSlot;
  f << "which=" << nWhich << ",";
  if (nSlot) f << "slot=" << nSlot << ",";
  std::shared_ptr<StarItem> pItem;
  if (!m_state->isInRange(nWhich)) {
    uint16_t nSurro;
    *input >> nSurro;
    bool ok=true;
    if (nSurro==0xffff) {
      uint16_t nVersion, nLength;
      *input >> nVersion >> nLength;
      f << "nVersion=" << nVersion << ",";
      if (input->tell()+long(nLength)>endPos) {
        f << "###length=" << nLength << ",";
        STOFF_DEBUG_MSG(("StarItemPool::readItem: find bad surro length\n"));
        ok=false;
      }
      else if (nLength) {
        long endAttrPos=input->tell()+long(nLength);
        ascii.addDelimiter(input->tell(),'|');
        input->seek(endAttrPos, librevenge::RVNG_SEEK_SET);
      }
    }
    else if (nSurro)
      f << "surro=" << nSurro << ",";
    if (ok && input->tell()>endPos) {
      f << "###,";
      STOFF_DEBUG_MSG(("StarItemPool::readItem: find bad position\n"));
    }
    else if (ok)
      pItem.reset(new StarItem(StarAttributeManager::getDummyAttribute(), nWhich));
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return pItem;
  }

  if (!isDirect) {
    if (nWhich)
      pItem=loadSurrogate(zone, nWhich, true, f);
    else {
      input->seek(2, librevenge::RVNG_SEEK_CUR);
      pItem.reset(new StarItem(StarAttributeManager::getDummyAttribute(), nWhich));
    }
  }
  if (isDirect || (nWhich && !pItem)) {
    uint16_t nVersion;
    uint32_t nLength;
    *input >> nVersion >> nLength;
    if (nVersion) f << "vers=" << nVersion << ",";
    if (input->tell()+long(nLength)>endPos) {
      f << "###length=" << nLength << ",";
      STOFF_DEBUG_MSG(("StarItemPool::readItem: find bad item\n"));
    }
    else if (nLength) {
      long endAttrPos=input->tell()+long(nLength);
      auto attribute=readAttribute(zone, int(nWhich), int(nVersion), endAttrPos);
      pItem.reset(new StarItem(attribute, nWhich));
      if (!attribute) {
        STOFF_DEBUG_MSG(("StarItemPool::readItem: can not read an attribute\n"));
        f << "###";
      }
      if (input->tell()!=endAttrPos) {
        ascii.addDelimiter(input->tell(),'|');
        input->seek(endAttrPos, librevenge::RVNG_SEEK_SET);
      }
    }
    if (pItem && input->tell()>endPos) {
      f << "###,";
      STOFF_DEBUG_MSG(("StarItemPool::readItem: find bad position\n"));
      pItem.reset();
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return pItem;
  }
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  return pItem;
}

bool StarItemPool::loadSurrogate(StarItem &item)
{
  if (item.m_attribute || !item.m_which)
    return false;

  if ((item.m_which<m_state->m_verStart||item.m_which>m_state->m_verEnd)&&m_state->m_secondaryPool)
    return m_state->m_secondaryPool->loadSurrogate(item);
  int aWhich=(item.m_localId && m_state->m_currentVersion!=m_state->m_loadingVersion) ?
             m_state->getWhich(item.m_which) : item.m_which;
  auto *values=m_state->getValues(aWhich);
  if (item.m_surrogateId==0xfffe) {
    // default
    if (!values || !values->m_default)
      item.m_attribute=m_state->getDefaultAttribute(aWhich);
    else
      item.m_attribute=values->m_default;
    return true;
  }
  if (!values || values->m_idValueMap.find(item.m_surrogateId)==values->m_idValueMap.end()) {
    STOFF_DEBUG_MSG(("StarItemPool::loadSurrogate: can not find the attribute array for %d[%d]\n", aWhich, item.m_surrogateId));
    item.m_attribute=m_state->getDefaultAttribute(aWhich);
    return true;
  }
  item.m_attribute=values->m_idValueMap.find(item.m_surrogateId)->second;

  return true;
}

std::shared_ptr<StarItem> StarItemPool::loadSurrogate(StarZone &zone, uint16_t &nWhich, bool localId, libstoff::DebugStream &f)
{
  if ((nWhich<m_state->m_verStart||nWhich>m_state->m_verEnd)&&m_state->m_secondaryPool)
    return m_state->m_secondaryPool->loadSurrogate(zone, nWhich, localId, f);
  // polio.cxx SfxItemPool::LoadSurrogate
  uint16_t nSurrog;
  *zone.input()>>nSurrog;
  if (nSurrog==0xffff) {
    f << "direct,";
    return std::shared_ptr<StarItem>();
  }
  if (nSurrog==0xfff0) {
    f << "null,";
    nWhich=0;
    return std::shared_ptr<StarItem>();
  }
  if (m_state->m_loadingVersion<0) // the pool is not read, so we wait
    return createItem(int(nWhich), int(nSurrog), localId);
  std::shared_ptr<StarItem> res(new StarItem(int(nWhich)));
  int aWhich=(localId && m_state->m_currentVersion!=m_state->m_loadingVersion) ?
             m_state->getWhich(nWhich) : nWhich;
  StarItemPoolInternal::Values *values=m_state->getValues(aWhich);
  if (nSurrog==0xfffe) {
    f << "default,";
    if (!values || !values->m_default) {
      if (!isInside() && m_state->m_document.getAttributeManager())
        res->m_attribute=m_state->getDefaultAttribute(aWhich);
      else // we must wait that the pool is read
        return createItem(int(nWhich), int(nSurrog), localId);
    }
    else
      res->m_attribute=values->m_default;
    return res;
  }
  f << "surrog=" << nSurrog << ",";
  if (!values || values->m_idValueMap.find(int(nSurrog))==values->m_idValueMap.end()) {
    if (isInside()) {
      // ok, we must wait that the pool is read
      return createItem(int(nWhich), int(nSurrog), localId);
    }
    STOFF_DEBUG_MSG(("StarItemPool::loadSurrogate: can not find the attribute array for %d[%d]\n", aWhich, int(nSurrog)));
    f << "###notFind,";
    res->m_attribute=m_state->getDefaultAttribute(aWhich);
    return res;
  }
  res->m_attribute=values->m_idValueMap.find(int(nSurrog))->second;
  return res;
}

bool StarItemPool::readV2(StarZone &zone, StarItemPool *master)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();
  libstoff::DebugStream f;
  f << "Entries(PoolDef)["<< zone.getRecordLevel() << "]:";
  long pos=input->tell();
  if (master) {
    f << "secondary,";
    m_state->m_majorVersion=2;
    m_state->m_minorVersion=master->m_state->m_minorVersion;
  }
  else {
    if (pos+18>endPos) {
      STOFF_DEBUG_MSG(("StarItemPool::readV2: the zone seems too short\n"));
      return false;
    }
    uint16_t tag;
    // poolio.cxx SfxItemPool::Load
    uint8_t nMajorVers, nMinorVers;
    *input >> tag >> nMajorVers >> nMinorVers;
    if (tag==0x1111)
      f << "v4,";
    else if (tag==0xbbbb)
      f << "v5,";
    else {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }

    if (nMajorVers!=2) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }
    *input>>tag;
    if (tag!=0xffff) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }

    m_state->m_majorVersion=2;
    m_state->m_minorVersion=int(nMinorVers);
    f << "version=2,";
    if (m_state->m_minorVersion) f << "vers[minor]=" << m_state->m_minorVersion << ",";
    input->seek(4, librevenge::RVNG_SEEK_CUR); // 0,0
  }
  unsigned char type; // always 1
  if (input->peek()!=1 || !zone.openSfxRecord(type)) {
    STOFF_DEBUG_MSG(("StarItemPool::readV2: can not open the sfx record\n"));
    m_state->m_majorVersion=0;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  endPos=zone.getRecordLastPosition();
  if (input->tell()==endPos) {
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  // string part
  unsigned char type1;
  if (input->peek()!=16 || !zone.openSfxRecord(type1)) {
    STOFF_DEBUG_MSG(("StarItemPool::readV2: can not open the string sfx record\n"));
    f << "###openString";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  int16_t val;
  *input >> val;
  if (val)
    f << "loadingVersion=" << val << ",";
  m_state->m_loadingVersion=val;
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarItemPool::readV2: can not readV2 the name\n"));
    f << "###name";
  }
  if (!string.empty()) {
    m_state->setPoolName(libstoff::getString(string));
    f << "name[ext]=" << libstoff::getString(string).cstr() << ",";
  }
  zone.closeSfxRecord(type1, "PoolDef");

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "PoolDef[versMap]:";
  StarItemPoolInternal::SfxMultiRecord mRecord;
  if (!mRecord.open(zone) || mRecord.getHeaderTag()!=0x20) {
    STOFF_DEBUG_MSG(("StarItemPool::readV2: can not open the versionMap sfx record\n"));
    f << "###openVersionMap";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    if (mRecord.isOpened()) mRecord.close("PoolDef");
    zone.closeSfxRecord(type, "PoolDef");
    return true;
  }
  f << mRecord;
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  int id;
  while (mRecord.getNewContent("PoolDef", id)) {
    pos=input->tell();
    f.str("");
    f << "PoolDef[versMap-" << id << "]:";
    uint16_t nVers, nStart, nEnd;
    *input >> nVers >> nStart >> nEnd;
    f << "vers=" << nVers << "," << nStart << "<->" << nEnd << ",";
    if (nStart>nEnd || input->tell()+2*(nEnd-nStart+1) > mRecord.getLastContentPosition()) {
      STOFF_DEBUG_MSG(("StarItemPool::readV2: can not find start|end pos\n"));
      f << "###badStartEnd";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      break;
    }
    f << "pos=[";
    std::vector<int> listData;
    for (auto i=int(nStart); i<=int(nEnd); ++i) {
      uint16_t nPos;
      *input>>nPos;
      listData.push_back(int(nPos));
      f << nPos << ",";
    }
    f << "],";
    if (!listData.empty()) m_state->addVersionMap(nVers,nStart,listData);

    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  mRecord.close("PoolDef");

  for (int step=0; step<2; ++step) {
    std::string wh(step==0 ? "attrib" : "default");
    pos=input->tell();
    f.str("");
    f << "PoolDef[" << wh << "]:";
    if (!mRecord.open(zone)) {
      STOFF_DEBUG_MSG(("StarItemPool::readV2: can not open the sfx record\n"));
      f << "###" << wh;
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type, "PoolDef");
      return true;
    }
    f << mRecord;

    if (mRecord.getHeaderTag()!=(step==0 ? 0x30:0x50)) {
      STOFF_DEBUG_MSG(("StarItemPool::readV2: can not find the pool which tag\n"));
      f << "###tag";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      mRecord.close("PoolDef");
      continue;
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    while (mRecord.getNewContent("PoolDef", id)) {
      pos=input->tell();
      f.str("");
      f << "PoolDef[" << wh << "-" << id << "]:";
      uint16_t nWhich, nVersion, nCount=1;
      *input >> nWhich >> nVersion;
      if (step==0) *input >> nCount;
      int which=nWhich;
      if (!m_state->isInRange(which)) {
        STOFF_DEBUG_MSG(("StarItemPool::readV2: the which value seems bad\n"));
        f << "###";
      }
      f << "wh=" << which << ",vers=" << nVersion << ",";
      if (step==0) f << "count=" << nCount << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      pos=input->tell();
      std::shared_ptr<StarAttribute> attribute;
      int aWhich=m_state->m_currentVersion!=m_state->m_loadingVersion ? m_state->getWhich(which) : which;
      StarItemPoolInternal::Values *values=m_state->getValues(aWhich, true);
      if (step==0) {
        if (!values->m_idValueMap.empty()) {
          STOFF_DEBUG_MSG(("StarItemPool::readV2: oops, there is already some attributes in values\n"));
        }
        StarItemPoolInternal::SfxMultiRecord mRecord1;
        f.str("");
        f << "Entries(StarAttribute):inPool,";
        if (!mRecord1.open(zone)) {
          STOFF_DEBUG_MSG(("StarItemPool::readV2: can not open record1\n"));
          f << "###record1";
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
        }
        else {
          f << mRecord1;
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
          while (mRecord1.getNewContent("StarAttribute", id)) {
            pos=input->tell();
            f.str("");
            f << "StarAttribute:inPool,wh=" <<  which << "[" << id << "],";
            uint16_t nRef;
            *input>>nRef;
            f << "ref=" << nRef << ",";
            attribute=readAttribute(zone, which, int(nVersion), mRecord1.getLastContentPosition());
            if (!attribute)
              f << "###";
            else if (input->tell()!=mRecord1.getLastContentPosition()) {
              STOFF_DEBUG_MSG(("StarItemPool::readV2: find extra attrib data\n"));
              f << "###extra";
            }
            if (values->m_idValueMap.find(id)!=values->m_idValueMap.end()) {
              STOFF_DEBUG_MSG(("StarItemPool::readV2: find dupplicated attrib data in %d\n", id));
              f << "###id";
            }
            else
              values->m_idValueMap[id]=attribute;
            input->seek(mRecord1.getLastContentPosition(), librevenge::RVNG_SEEK_SET);
            ascii.addPos(pos);
            ascii.addNote(f.str().c_str());
          }
          mRecord1.close("StarAttribute");
        }
      }
      else {
        if (values->m_default) {
          STOFF_DEBUG_MSG(("StarItemPool::readV2: the default slot %d is already created\n", aWhich));
        }
        attribute=readAttribute(zone, which, int(nVersion), mRecord.getLastContentPosition());
        if (!attribute) {
          f.str("");
          f << "Entries(StarAttribute)[" <<  which << "]:";
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
        }
        else {
          values->m_default=attribute;
          if (input->tell()!=mRecord.getLastContentPosition()) {
            STOFF_DEBUG_MSG(("StarItemPool::readV2: find extra attrib data\n"));
            ascii.addPos(pos);
            ascii.addNote("extra###");
          }
        }
      }
      input->seek(mRecord.getLastContentPosition(), librevenge::RVNG_SEEK_SET);
    }
    mRecord.close("PoolDef");
  }

  zone.closeSfxRecord(type, "PoolDef");
  return true;
}

bool StarItemPool::readV1(StarZone &zone, StarItemPool * /*master*/)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long endPos=zone.getRecordLevel()>0 ?  zone.getRecordLastPosition() : input->size();
  libstoff::DebugStream f;
  f << "Entries(PoolDef)[" << zone.getRecordLevel() << "]:";
  long pos=input->tell();
  if (pos+18>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: the zone seems too short\n"));
    return false;
  }
  uint16_t tag;
  // poolio.cxx SfxItemPool::Load1_Impl
  uint8_t nMajorVers, nMinorVers;
  *input >> tag >> nMajorVers >> nMinorVers;
  if (tag==0x1111)
    f << "v4,";
  else if (tag==0xbbbb)
    f << "v5,";
  else {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  if (nMajorVers!=1) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  m_state->m_majorVersion=1;
  f << "version=" << int(nMajorVers) << ",vers[minor]=" << int(nMinorVers) << ",";
  if (nMinorVers>=3) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: find a minor version >= 3\n"));
    f << "###";
  }

  // SfxItemPool::Load1_Impl
  if (nMinorVers>=2) {
    int16_t nLoadingVersion;
    *input >> nLoadingVersion;
    m_state->m_loadingVersion=int(nLoadingVersion);
    if (nLoadingVersion) f << "vers[loading]=" << nLoadingVersion << ",";
  }
  else
    m_state->m_loadingVersion=0;
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: can not read the name\n"));
    f << "###name";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  if (!string.empty()) {
    m_state->setPoolName(libstoff::getString(string));
    f << "name[ext]=" << libstoff::getString(string).cstr() << ",";
  }
  uint32_t attribSize;
  *input>>attribSize;
  long attribPos=input->tell();
  if (attribPos+long(attribSize)+10>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: attribSize is bad\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  else if (attribSize) {
    f << "attr[sz]=" << attribSize << ",";
    input->seek(long(attribSize), librevenge::RVNG_SEEK_CUR);
  }
  else {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: attribSize\n"));
    f << "###attrib[sz]==0,";
  }

  *input >> tag;
  if (tag!=0x3333) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find tag size\n"));
    f << "###tag=" << std::hex << tag << std::dec;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  uint32_t tableSize;
  *input>>tableSize;
  long tablePos=input->tell();
  long beginEndPos=tablePos+long(tableSize);
  if (beginEndPos+4>endPos) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: tableSize is bad\n"));
    f << "###";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  else if (tableSize)
    f << "table[sz]=" << tableSize << ",";
  else if (nMinorVers>=3) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: tableSize is null for version 3\n"));
    f << "###table[sz]==0,";
  }
  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());

  long endTablePos=tablePos+long(tableSize);
  if (nMinorVers>=3 && tableSize>=4) { // CHECKME: never seens
    input->seek(endTablePos-4, librevenge::RVNG_SEEK_SET);
    pos=long(input->readULong(4));
    endTablePos-=4;
    if (pos<tablePos || pos>=endTablePos) {
      STOFF_DEBUG_MSG(("StarItemPool::readV1: arrgh can not find versionmap position\n"));
    }
    else {
      long lastVersMapPos=endTablePos;
      endTablePos=pos;
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      f.str("");
      f << "PoolDef[versionMap]:";
      uint16_t nVerCount;
      *input >> nVerCount;
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      for (int i=0; i<int(nVerCount); ++i) {
        pos=input->tell();
        f.str("");
        f << "PoolDef[versionMap" << i << "]:";
        uint16_t nVers, nStart, nEnd;
        *input >> nVers >> nStart >> nEnd;
        f << "vers=" << nVers << "," << nStart << "-" << nEnd << ",";
        int N=int(nEnd)+1-int(nStart);
        if (N<0 || pos+6+2*N>lastVersMapPos) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: arrgh nBytes is bad\n"));
          f << "###";
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
          break;
        }
        f << "val=[";
        for (int j=0; j<N; ++j) {
          uint16_t val;
          *input >> val;
          if (val) f << val << ",";
          else
            f << "_,";
        }
        f << "],";
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
      }
    }
  }

  // now read the table
  input->seek(tablePos, librevenge::RVNG_SEEK_SET);
  f.str("");
  f << "PoolDef[table]:";
  std::vector<uint32_t> sizeAttr;
  f << "size=[";
  while (input->tell()+4<=endTablePos) {
    uint32_t sz;
    *input>>sz;
    sizeAttr.push_back(sz);
    f << sz << ",";
  }
  f << "],";
  ascii.addPos(tablePos-6);
  ascii.addNote(f.str().c_str());

  // now read the attribute and default
  long endDataPos=attribPos+long(attribSize);
  size_t n=0;
  for (int step=0; step<2; ++step) {
    std::string const what(step==0 ? "attrib" : "default");
    f.str("");
    f << "PoolDef[" << what << "]:";
    if (step==1) {
      pos=input->tell();
      if (nMinorVers>0) {
        *input>>tag;
        if (tag!=0x4444) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: default tag is bad \n"));
          f << "###tag=" << std::hex << tag << std::dec;
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
          break;
        }
      }
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
    }
    else {
      input->seek(attribPos, librevenge::RVNG_SEEK_SET);
      if (attribSize!=0)  {
        *input >> tag;
        if (tag!=0x2222) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: tag is bad \n"));
          f << "###tag=" << std::hex << tag << std::dec;
          break;
        }
      }
      ascii.addPos(attribPos-4);
      ascii.addNote(f.str().c_str());
      if (attribSize==0) continue;
    }
    bool ok=true;
    while (ok) {
      pos=input->tell();
      f.str("");
      f << "PoolDef[" << what << "-" << n << "]:";
      if (pos+2 > endDataPos) {
        ok=false;
        STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find some attrib\n"));
        f << "###noLast,";
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
        break;
      }
      uint16_t nWhich;
      *input >> nWhich;
      if (nWhich==0) {
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
        break;
      }

      uint16_t nSlot, nVersion, nCount=1;
      *input >> nSlot >> nVersion;
      if (step==0) *input >> nCount;
      int which=nWhich;
      // checkme: sometimes, we must use slot
      if (!m_state->isInRange(which)) {
        STOFF_DEBUG_MSG(("StarItemPool::readV1: the which value seems bad\n"));
        f << "###";
      }
      f << "wh=" << which << "[" << std::hex << nSlot << std::dec << "], vers=" << nVersion << ", count=" << nCount << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      int aWhich=m_state->m_currentVersion!=m_state->m_loadingVersion ? m_state->getWhich(which) : which;
      auto *values=m_state->getValues(aWhich, true);
      if (step==0 && nCount) {
        if (!values->m_idValueMap.empty()) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: the slot %d is already created\n", aWhich));
        }
      }
      else if (step==1) {
        if (values->m_default) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: the default slot %d is already created\n", aWhich));
        }
      }
      for (int i=0; i<nCount; ++i) {
        long debAttPos=(step==0 ? input->tell() : pos);
        pos=input->tell();
        f.str("");
        f << "StarAttribute:inPool,wh=" <<  which << "->" << aWhich << "[id=" << i << "],";
        if (n >= sizeAttr.size() || debAttPos+long(sizeAttr[n])>endDataPos ||
            input->tell()>debAttPos+long(sizeAttr[n])) {
          ok=false;

          STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find attrib size\n"));
          f << "###badSize,";
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
          break;
        }
        long endAttPos=debAttPos+long(sizeAttr[n]);
        std::shared_ptr<StarAttribute> attribute;
        if (input->tell()==endAttPos) {
          ascii.addPos(pos);
          ascii.addNote(f.str().c_str());
          if (step==0)
            values->m_idValueMap[i]=attribute;
          continue;
        }

        uint16_t nRef=1;
        if (step==0) {
          *input>>nRef;
          f << "ref=" << nRef << ",";
        }
        if (nRef) {
          attribute=readAttribute(zone, which, int(nVersion), debAttPos+long(sizeAttr[n]));
          if (!attribute)
            f << "###";
        }
        if (step==0)
          values->m_idValueMap[i]=attribute;
        else
          values->m_default=attribute;
        if (input->tell()!=debAttPos+long(sizeAttr[n])) {
          STOFF_DEBUG_MSG(("StarItemPool::readV1: find extra attrib data\n"));
          f << "###extra,";
          if (input->tell()!=pos)
            ascii.addDelimiter(input->tell(),'|');
          input->seek(debAttPos+long(sizeAttr[n]), librevenge::RVNG_SEEK_SET);
        }
        ascii.addPos(pos);
        ascii.addNote(f.str().c_str());
        ++n;
      }
    }
    if (!ok) break;
  }

  input->seek(beginEndPos, librevenge::RVNG_SEEK_SET);
  pos=input->tell();
  f.str("");
  f << "PoolDef[end]:";
  uint16_t tag1;
  *input >> tag >> tag1;
  if (tag!=0xeeee || tag1!=0xeeee) {
    STOFF_DEBUG_MSG(("StarItemPool::readV1: can not find end tag\n"));
    f << "###tag=" << std::hex << tag << std::dec;
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  return true;
}

bool StarItemPool::readStyles(StarZone &zone, StarObject &doc)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascii=zone.ascii();
  long pos=input->tell();
  libstoff::DebugStream f;
  int poolVersion=input->peek()==3 ? 2 : 1;
  f << "Entries(SfxStylePool)[" << zone.getRecordLevel() << "]:pool[vers]=" << poolVersion << ",";
  unsigned char type=3; // to make clang analyzer happy
  uint16_t charSet=0, nCount;

  bool helpIdSize32=true, ok=true;
  StarItemPoolInternal::SfxMultiRecord mRecord;
  if (poolVersion==1) {
    // style.cxx SfxStyleSheetBasePool::Load1_Impl
    *input >> charSet;
    if (charSet==50) {
      f << "v50,";
      *input >> charSet;
    }
    else
      helpIdSize32=false;
    if (charSet) f << "char[set]="<< charSet << ",";
    *input >> nCount;
    f << "n=" << nCount << ",";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }
  else {
    // style.cxx SfxStyleSheetBasePool::Load(vers==2)
    if (input->peek()!=3 || !zone.openSfxRecord(type)) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not open the first zone\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      return false;
    }

    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SfxStylePool[header]:";
    unsigned char type1;
    if (!zone.openSfxRecord(type1)) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not open the header zone\n"));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type, "SfxStylePool");
      return false;
    }
    uint8_t headerType, headerVersion;
    uint16_t headerTag;
    *input >> headerType >> headerVersion >> headerTag;
    if (headerTag!=0x10) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not find header tag\n"));
      f << "###";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type1, "SfxStylePool");
      zone.closeSfxRecord(type, "SfxStylePool");
    }
    if (headerVersion) f << "vers=" << int(headerVersion) << ",";
    if (headerType) f << "type=" << int(headerType) << ",";
    *input >> charSet;
    if (charSet) f << "char[set]="<< charSet << ",";
    zone.closeSfxRecord(type1, "SfxStylePool");
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SfxStylePool[styles]:";
    if (!mRecord.open(zone)) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not open the versionMap sfx record\n"));
      f << "###openVersionMap";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      zone.closeSfxRecord(type, "PoolDef");
      return true;
    }
    f << mRecord;

    if (mRecord.getHeaderTag()!=0x20) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not find the version map tag\n"));
      f << "###tag";
      ok=false;
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());

    nCount=mRecord.getNumRecords();
  }
  long lastPos=zone.getRecordLevel() ? zone.getRecordLastPosition() : input->size();
  for (int i=0; ok && i<int(nCount); ++i) {
    pos=input->tell();
    if (poolVersion==2) {
      int id=0;
      if (!mRecord.getNewContent("SfxStylePool", id))
        break;
      lastPos=mRecord.getLastContentPosition();
    }
    f.str("");
    f << "SfxStylePool[data" << i << "]:";
    bool readOk=true;
    std::vector<uint32_t> text;
    StarItemStyle style;
    for (int j=0; j<3; ++j) {
      if (!zone.readString(text, charSet) || input->tell()>=lastPos) {
        STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not find a string\n"));
        f << style << "###string" << j;
        readOk=false;
        break;
      }
      style.m_names[j]=libstoff::getString(text);
    }
    if (!readOk) {
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    style.m_family=int(input->readULong(2));
    style.m_mask=int(input->readULong(2));
    if (!zone.readString(text, charSet) || input->tell()>=lastPos) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not find helpFile\n"));
      f << style << "###helpFile";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    style.m_names[3]=libstoff::getString(text);
    style.m_helpId=unsigned(input->readULong(helpIdSize32 ? 4 : 2));
    std::vector<STOFFVec2i> limits; // unknown
    if (!doc.readItemSet(zone, limits, lastPos, style.m_itemSet, this, false)) {
      f << style << "###itemList";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    StarItemPoolInternal::StyleId styleId(style.m_names[0], style.m_family);
    if (m_state->m_styleIdToStyleMap.find(styleId)!=m_state->m_styleIdToStyleMap.end()) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: style %s-%d\n", style.m_names[0].cstr(), style.m_family));
    }
    else
      m_state->m_styleIdToStyleMap[styleId]=style;
    f << style;
    ascii.addDelimiter(input->tell(),'|');
    uint16_t nVer;
    uint32_t nSize;
    *input>>nVer>>nSize;
    if (nVer) f << "version=" << nVer << ",";
    if (input->tell()+long(nSize)>lastPos) {
      STOFF_DEBUG_MSG(("StarItemPool::readStyles: something is bad\n"));
      f << "###nSize=" << nSize << ",";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      if (poolVersion==1) return true;
      continue;
    }
    if (nSize>=3 && m_state->m_type==StarItemPool::T_WriterPool) {
      // sw3style.cxx SwStyleSheet::Load
      long dataPos=input->tell(), endDataPos=dataPos+long(nSize);
      libstoff::DebugStream f2;
      f2 << "Entries(SwStyleSheet):sz=" << nSize << ",";
      auto nId=int(input->readULong(2));
      if (nId) f2 << "id=" << nId << ",";
      auto level=int(input->readULong(1));
      if (level!=201) {
        // checkme: sometimes is more complicated
        if (m_state->m_styleIdToStyleMap.find(styleId)!=m_state->m_styleIdToStyleMap.end())
          m_state->m_styleIdToStyleMap.find(styleId)->second.m_outlineLevel=level;
        f2 << "level=" << level << ",";
      }
      bool dataOk=true;
      if (1<=nVer) {
        /*if (nVer==1 && style.m_family==2 && nId==1)
          nId=(1<<11)+1; // RES_POOLCOLL_TEXT*/
        dataOk=input->tell()+2<=endDataPos;
        int val=dataOk ? int(input->readULong(2)) : 0;
        if (val==1) {
          f2 << "condCol,";
          dataOk=input->tell()+2<=endDataPos;
          int N=dataOk ? int(input->readULong(2)) : 0;
          if (N>255) N=255;
          for (int n=0; dataOk && n<N; ++n) {
            if (!zone.readString(text, charSet) || input->tell()+4>endDataPos) {
              dataOk=false;
              break;
            }
            f2 << "[" << libstoff::getString(text).cstr();
            auto cond=int(input->readULong(4));
            if (cond) f2 << "cond=" << std::hex << cond << std::dec << ",";
            if (cond & 0x8000) {
              if (!zone.readString(text, charSet) || input->tell()>endDataPos) {
                dataOk=false;
                break;
              }
              f2 << libstoff::getString(text).cstr() << ",";
            }
            else if (input->tell()+4<=endDataPos)
              f2 << "subCond=" << std::hex << input->readULong(4) << std::dec << ",";
            else
              dataOk=false;
            f2 << "]";
          }
        }
        int cFlag=0;
        if (dataOk && 4<=nVer) {
          dataOk=input->tell()+1<=endDataPos;
          cFlag=dataOk ? int(input->readULong(1)) : 0;
          if (cFlag) f2 << "cFlag=" << std::hex << cFlag << std::dec << ",";
        }
        if (nVer>=6 && (cFlag&2)) {
          dataOk=input->tell()+4<=endDataPos;
          long len=dataOk ? long(input->readULong(4)) : 0;
          dataOk=input->tell()+len<=endDataPos;
          if (dataOk && len>=2 && doc.getAttributeManager()) {
            auto nAttrVer=int(input->readULong(2));
            auto attrib=doc.getAttributeManager()->readAttribute
                        (zone, StarAttribute::ATTR_FRM_LR_SPACE, nAttrVer, endDataPos, doc);
            f2 << "LR,";
            if (!attrib || input->tell()>endDataPos) {
              STOFF_DEBUG_MSG(("StarItemPool::readStyles: reading relative LR is not implemented\n"));
              f2 << "###";
              dataOk=false;
            }
          }
          else if (len)
            dataOk=false;
        }
      }
      if (!dataOk) {
        f2 << "###";
        STOFF_DEBUG_MSG(("StarItemPool::readStyles: can not read some final data\n"));
      }
      if (input->tell()!=endDataPos) {
        if (!dataOk) {
          STOFF_DEBUG_MSG(("StarItemPool::readStyles:  find extra final data\n"));
          f << "###extra,";
        }
        ascii.addDelimiter(input->tell(),'|');
      }
      ascii.addPos(dataPos-4);
      ascii.addNote(f2.str().c_str());
      input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
    }
    else if (nSize) {
      f << "#size=" << nSize << ",";
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarItemPool::readStyles: loading the base sheet data is not implemented\n"));
        first=false;
      }
      libstoff::DebugStream f2;
      f2 << "Entries(SfxBaseSheet):sz=" << nSize << ",###unknown";
      ascii.addPos(input->tell()-4);
      ascii.addNote(f2.str().c_str());
      ascii.addDelimiter(input->tell(),'|');
      input->seek(long(nSize), librevenge::RVNG_SEEK_CUR);
    }
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
  }

  if (poolVersion==2) {
    mRecord.close("SfxStylePool");
    zone.closeSfxRecord(type, "SfxStylePool");
  }
  return true;
}

void StarItemPool::updateStyles()
{
  std::set<StarItemPoolInternal::StyleId> done, toDo;
  std::multimap<StarItemPoolInternal::StyleId, StarItemPoolInternal::StyleId> childMap;
  std::map<int, std::shared_ptr<StarItem> >::const_iterator iIt;
  for (auto it : m_state->m_styleIdToStyleMap) {
    if (it.second.m_names[1].empty())
      toDo.insert(it.first);
    else {
      StarItemPoolInternal::StyleId parentId(it.second.m_names[1], it.first.m_family);
      // reset the parent name, as it must no longer be used when creating a styles...
      it.second.m_names[1].clear();
      if (m_state->m_styleIdToStyleMap.find(parentId)==m_state->m_styleIdToStyleMap.end()) {
        STOFF_DEBUG_MSG(("StarItemPool::updateStyles: can not find style %s-%d\n", parentId.m_name.cstr(), parentId.m_family));
      }
      else
        childMap.insert(std::multimap<StarItemPoolInternal::StyleId, StarItemPoolInternal::StyleId>::value_type(parentId, it.first));
    }
  }
  while (!toDo.empty()) {
    auto styleId=*toDo.begin();
    toDo.erase(styleId);
    if (done.find(styleId)!=done.end()) {
      STOFF_DEBUG_MSG(("StarItemPool::updateStyles: find loop for style %s-%d\n", styleId.m_name.cstr(), styleId.m_family));
      continue;
    }
    done.insert(styleId);
    auto it=m_state->m_styleIdToStyleMap.find(styleId);
    if (it==m_state->m_styleIdToStyleMap.end()) {
      STOFF_DEBUG_MSG(("StarItemPool::updateStyles: can not find style %s-%d\n", styleId.m_name.cstr(), styleId.m_family));
      continue;
    }
    StarItemSet const &parentItemSet=it->second.m_itemSet;
    auto cIt=childMap.lower_bound(styleId);
    while (cIt!=childMap.end() && cIt->first==styleId) {
      auto childId=cIt++->second;
      it=m_state->m_styleIdToStyleMap.find(childId);
      if (it==m_state->m_styleIdToStyleMap.end() || done.find(childId)!=done.end()) {
        STOFF_DEBUG_MSG(("StarItemPool::updateStyles: pb with style %s-%d\n", childId.m_name.cstr(), childId.m_family));
        continue;
      }
      toDo.insert(childId);
      auto &childItemSet=it->second.m_itemSet;
      for (iIt=parentItemSet.m_whichToItemMap.begin(); iIt!=parentItemSet.m_whichToItemMap.end(); ++iIt) {
        if (!iIt->second || childItemSet.m_whichToItemMap.find(iIt->first)!=childItemSet.m_whichToItemMap.end())
          continue;
        childItemSet.m_whichToItemMap[iIt->first]=iIt->second;
      }
    }
  }
  if (done.size()!=m_state->m_styleIdToStyleMap.size()) {
    STOFF_DEBUG_MSG(("StarItemPool::updateStyles: convert only %d of %d styles\n", int(done.size()), int(m_state->m_styleIdToStyleMap.size())));
  }
  /* Sometimes, the attribute encoding when reading style name seems
     bad (even in the lastest versions of LibreOffice which read sdc
     files), so create a map to try to retrieve the real style name
     from a bad encoded style name...
   */
  std::set<librevenge::RVNGString> dupplicatedSimpName;
  for (auto it : m_state->m_styleIdToStyleMap) {
    if (it.second.m_names[0].empty()) continue;
    auto simpName=libstoff::simplifyString(it.second.m_names[0]);
    if (it.second.m_names[0]==simpName || dupplicatedSimpName.find(simpName)!=dupplicatedSimpName.end()) continue;
    if (m_state->m_simplifyNameToStyleNameMap.find(simpName)==m_state->m_simplifyNameToStyleNameMap.end())
      m_state->m_simplifyNameToStyleNameMap[simpName]=it.second.m_names[0];
    else if (m_state->m_simplifyNameToStyleNameMap.find(simpName)->second!=it.second.m_names[0]) {
      dupplicatedSimpName.insert(simpName);
      m_state->m_simplifyNameToStyleNameMap.erase(simpName);
    }
  }
}

StarItemStyle const *StarItemPool::findStyleWithFamily(librevenge::RVNGString const &style, int family) const
{
  if (style.empty())
    return nullptr;
  for (int step=0; step<2; ++step) {
    librevenge::RVNGString name(style);
    if (step==1) {
      // hack: try to retrieve the original style, ...
      librevenge::RVNGString simpName=libstoff::simplifyString(style);
      if (m_state->m_simplifyNameToStyleNameMap.find(simpName)==m_state->m_simplifyNameToStyleNameMap.end())
        break;
      name=m_state->m_simplifyNameToStyleNameMap.find(simpName)->second;
    }
    StarItemPoolInternal::StyleId styleId(name, 0);
    auto it=m_state->m_styleIdToStyleMap.lower_bound(styleId);
    while (it!=m_state->m_styleIdToStyleMap.end() && it->first.m_name==name) {
      if ((it->first.m_family&family)==family)
        return &it->second;
      ++it;
    }
  }
  STOFF_DEBUG_MSG(("StarItemPool::findStyleWithFamily: can not find with style %s-%d\n", style.cstr(), family));
  return nullptr;
}

void StarItemPool::defineGraphicStyle(STOFFListenerPtr listener, librevenge::RVNGString const &styleName, StarObject &object, std::set<librevenge::RVNGString> &done) const
{
  if (styleName.empty() || done.find(styleName)!=done.end())
    return;
  done.insert(styleName);
  if (listener->isGraphicStyleDefined(styleName))
    return;
  if (!listener) {
    STOFF_DEBUG_MSG(("StarItemPool::defineGraphicStyle: can not find the listener"));
    return;
  }
  auto const *style=findStyleWithFamily(styleName, StarItemStyle::F_Paragraph);
  if (!style) {
    STOFF_DEBUG_MSG(("StarItemPool::defineGraphicStyle: can not find graphic style with name %s", styleName.cstr()));
    return;
  }
  StarState state(this, object);
  state.m_frame.addTo(state.m_graphic.m_propertyList);
  state.m_graphic.m_propertyList.insert("style:display-name", styleName);
  if (!style->m_names[1].empty()) {
    if (done.find(style->m_names[1])!=done.end()) {
      STOFF_DEBUG_MSG(("StarItemPool::defineGraphicStyle: oops find a look with %s", style->m_names[1].cstr()));
    }
    else {
      defineGraphicStyle(listener, style->m_names[1], object, done);
      state.m_graphic.m_propertyList.insert("librevenge:parent-display-name", style->m_names[1]);
    }
  }
  for (auto it : style->m_itemSet.m_whichToItemMap) {
    if (it.second && it.second->m_attribute)
      it.second->m_attribute->addTo(state);
  }
  listener->defineStyle(state.m_graphic);
}

void StarItemPool::defineParagraphStyle(STOFFListenerPtr listener, librevenge::RVNGString const &styleName, StarObject &object, std::set<librevenge::RVNGString> &done) const
{
  if (styleName.empty() || done.find(styleName)!=done.end())
    return;
  done.insert(styleName);
  if (listener->isParagraphStyleDefined(styleName))
    return;
  if (!listener) {
    STOFF_DEBUG_MSG(("StarItemPool::defineParagraphStyle: can not find the listener"));
    return;
  }
  auto const *style=findStyleWithFamily(styleName, StarItemStyle::F_Paragraph);
  if (!style) {
    STOFF_DEBUG_MSG(("StarItemPool::defineParagraphStyle: can not find paragraph style with name %s", styleName.cstr()));
    return;
  }
  StarState state(this, object);
  if (style->m_outlineLevel>=0 && style->m_outlineLevel<20) {
    state.m_paragraph.m_outline=true;
    state.m_paragraph.m_listLevelIndex=style->m_outlineLevel+1;
  }
  state.m_paragraph.m_propertyList.insert("style:display-name", styleName);
  if (!style->m_names[1].empty()) {
    if (done.find(style->m_names[1])!=done.end()) {
      STOFF_DEBUG_MSG(("StarItemPool::defineParagraphStyle: oops find a look with %s", style->m_names[1].cstr()));
    }
    else {
      defineParagraphStyle(listener, style->m_names[1], object, done);
      state.m_paragraph.m_propertyList.insert("librevenge:parent-display-name", style->m_names[1]);
    }
  }
  for (auto it : style->m_itemSet.m_whichToItemMap) {
    if (it.second && it.second->m_attribute)
      it.second->m_attribute->addTo(state);
  }
  listener->defineStyle(state.m_paragraph);
}

void StarItemPool::updateUsingStyles(StarItemSet &itemSet) const
{
  auto const *style=findStyleWithFamily(itemSet.m_style, itemSet.m_family);
  if (!style) return;
  auto const &parentItemSet=style->m_itemSet;
  for (auto it : parentItemSet.m_whichToItemMap) {
    if (!it.second || itemSet.m_whichToItemMap.find(it.first)!=itemSet.m_whichToItemMap.end())
      continue;
    itemSet.m_whichToItemMap[it.first]=it.second;
  }
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
