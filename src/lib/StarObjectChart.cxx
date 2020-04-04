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
#include <memory>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFChart.hxx"
#include "STOFFFrameStyle.hxx"
#include "STOFFGraphicEncoder.hxx"
#include "STOFFGraphicListener.hxx"
#include "STOFFOLEParser.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFSpreadsheetListener.hxx"

#include "StarAttribute.hxx"
#include "StarEncryption.hxx"
#include "StarFileManager.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectDraw.hxx"
#include "StarObjectModel.hxx"
#include "StarZone.hxx"
#include "SWFieldManager.hxx"
#include "StarFormatManager.hxx"

#include "StarObjectChart.hxx"

/** Internal: the structures of a StarObjectChart */
namespace StarObjectChartInternal
{
////////////////////////////////////////
//! the chart of a StarObjectChart
class Chart final : public STOFFChart
{
public:
  //! constructor
  Chart() : STOFFChart()
  {
  }
protected:
  //! send the zone content (called when the zone is of text type)
  void sendContent(TextZone const &zone, STOFFListenerPtr &listener) const final;
};

void Chart::sendContent(Chart::TextZone const &/*zone*/, STOFFListenerPtr &listener) const
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectChartInternal::Chart::sendContent: can not find the listener\n"));
    return;
  }
  STOFF_DEBUG_MSG(("StarObjectChartInternal::Chart::sendContent: sorry, not implemented\n"));
}

////////////////////////////////////////
//! Internal: the state of a StarObjectChart
struct State {
  //! constructor
  State()
    : m_chart()
    , m_model()
  {
  }

  //! the chart
  std::shared_ptr<Chart> m_chart;
  //! the model
  std::shared_ptr<StarObjectModel> m_model;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectChart::StarObjectChart(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_chartState(new StarObjectChartInternal::State)
{
}

StarObjectChart::~StarObjectChart()
{
  cleanPools();
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////

bool StarObjectChart::send(STOFFListenerPtr listener, STOFFFrameStyle const &frame, STOFFGraphicStyle const &style)
{
  auto sheetListener=std::dynamic_pointer_cast<STOFFSpreadsheetListener>(listener);
  if (sheetListener && m_chartState->m_chart) {
    sheetListener->insertChart(frame, *m_chartState->m_chart, style);
    return true;
  }
  if (!listener || !m_chartState->m_model) {
    STOFF_DEBUG_MSG(("StarObjectChart::send: oops, can not create the graphic representation\n"));
    return false;
  }

  STOFFGraphicEncoder graphicEncoder;
  std::vector<STOFFPageSpan> pageList;
  int numPages;
  if (!m_chartState->m_model->updatePageSpans(pageList, numPages, true))
    pageList.push_back(STOFFPageSpan());
  STOFFGraphicListenerPtr graphicListener(new STOFFGraphicListener(STOFFListManagerPtr(), pageList, &graphicEncoder));
  graphicListener->startDocument();
  m_chartState->m_model->sendPage(0,graphicListener);
  graphicListener->endDocument();
  STOFFEmbeddedObject image;
  graphicEncoder.getBinaryResult(image);
  listener->insertPicture(frame, image, style);
  return true;
}

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool StarObjectChart::parse()
{
  if (!getOLEDirectory() || !getOLEDirectory()->m_input) {
    STOFF_DEBUG_MSG(("StarObjectChart::parser: error, incomplete document\n"));
    return false;
  }
  auto &directory=*getOLEDirectory();
  StarObject::parse();
  auto unparsedOLEs=directory.getUnparsedOles();
  STOFFInputStreamPtr input=directory.m_input;
  StarFileManager fileManager;
  for (auto const &name : unparsedOLEs) {
    STOFFInputStreamPtr ole = input->getSubStreamByName(name.c_str());
    if (!ole.get()) {
      STOFF_DEBUG_MSG(("StarObjectChart::parse: error: can not find OLE part: \"%s\"\n", name.c_str()));
      continue;
    }

    auto pos = name.find_last_of('/');
    std::string base;
    if (pos == std::string::npos) base = name;
    else base = name.substr(pos+1);
    ole->setReadInverted(true);
    if (base=="SfxStyleSheets") {
      readSfxStyleSheets(ole,name);
      continue;
    }
    if (base=="StarChartDocument") {
      readChartDocument(ole,name);
      continue;
    }
    if (base!="BasicManager2") {
      STOFF_DEBUG_MSG(("StarObjectChart::parse: find unexpected ole %s\n", name.c_str()));
    }
    libstoff::DebugFile asciiFile(ole);
    asciiFile.open(name);

    libstoff::DebugStream f;
    f << "Entries(" << base << "):";
    asciiFile.addPos(0);
    asciiFile.addNote(f.str().c_str());
    asciiFile.reset();
  }
  return true;
}

bool StarObjectChart::readChartDocument(STOFFInputStreamPtr input, std::string const &name)
try
{
  StarZone zone(input, name, "SWChartDocument", getPassword()); // checkme
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCChartDocument):";
  // sch_docshell.cxx: SchChartDocShell::Load
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: can not find header zone\n"));
    f << "###";
    ascFile.addPos(0);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  uint32_t n;
  uint16_t nVersion;
  *input >> nVersion; // schiocmp.cxx: SchIOCompat::SchIOCompat
  f << "vers=" << nVersion << ",";
  // sch_chartdoc.cxx: operator>> ChartModel
  *input >> n;
  if (n>1) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: bad version\n"));
    f << "###n=" << n << ",";
  }
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  long pos=input->tell(), lastPos= zone.getRecordLastPosition();
  auto pool=getNewItemPool(StarItemPool::T_VCControlPool);
  if (!pool || !pool->read(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);

  pos=input->tell();
  StarFileManager fileManager;
  if (pos!=lastPos && !fileManager.readJobSetUp(zone, false))
    input->seek(pos, librevenge::RVNG_SEEK_SET);

  zone.closeRecord("SCChartDocument");

  if (input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: find end after first zone\n"));
    return true;
  }

  pos=input->tell();
  auto model=std::make_shared<StarObjectModel>(*this, true);
  if (!model->read(zone)) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: can not find the SdrModel\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SCChartDocument:###SdrModel");
    return true;
  }
  m_chartState->m_model=model;
  pos=input->tell();
  if (input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: find no attribute\n"));
    return true;
  }
  f.str("");
  f << "SCChartDocument[attributes]:";
  if (!zone.openSCHHeader()) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: can not open SCHAttributes\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  f << "vers=" << zone.getHeaderVersion() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  // chtmode2.cxx ChartModel::LoadAttributes
  if (!readSCHAttributes(zone)) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: can not open the attributes zone\n"));
  }
  zone.closeSCHHeader("SCChartDocumentAttr");

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectChart::readChartDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SCChartDocument:##extra");
  }

  return true;
}
catch (...)
{
  return false;
}

bool StarObjectChart::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name)
{
  StarZone zone(input, name, "SfxStyleSheets", getPassword());
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);
  if (getDocumentKind()!=STOFFDocument::STOFF_K_CHART) {
    STOFF_DEBUG_MSG(("StarObjectChart::readSfxStyleSheets: called with unexpected document\n"));
    ascFile.addPos(0);
    ascFile.addNote("Entries(SfxStyleSheets)");
    return false;
  }

  // sd_sdbinfilter.cxx SdBINFilter::Import: one pool followed by a pool style
  // chart sch_docshell.cxx SchChartDocShell::Load
  auto pool=getNewItemPool(StarItemPool::T_XOutdevPool);
  pool->addSecondaryPool(getNewItemPool(StarItemPool::T_EditEnginePool));
  pool->addSecondaryPool(getNewItemPool(StarItemPool::T_ChartPool));

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
        STOFF_DEBUG_MSG(("StarObjectChart::readSfxStyleSheets: create extra pool of type %d\n", int(pool->getType())));
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
    STOFF_DEBUG_MSG(("StarObjectChart::readSfxStyleSheets: can not read a style pool\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  }
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("StarObjectChart::readSfxStyleSheets: find extra data\n"));
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
bool StarObjectChart::readSCHAttributes(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // chtmode2.cxx ChartModel::LoadAttributes
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(SCHAttributes)[" << zone.getRecordLevel() << "]:";
  int version=zone.getHeaderVersion();
  f << "vers=" << version << ",";
  double lightX, lightY, lightZ;
  int16_t nInt16;
  bool bNoMore=version<=7;
  *input >> lightX >> lightY >> lightZ;
  f << "light=" << lightX << "x" << lightY << "x" << lightZ << ",";
  if (version>=3) {
    *input>>nInt16;
    switch (nInt16) {
    case 0: // none
      bNoMore=true;
      break;
    case 1: // memchart
    case 2: // dynchart
    case 3: // memchartplus
      f << (nInt16==1 ? "memchart" : nInt16==2 ? "dynchart" : "memchartplus") << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      pos=input->tell();
      if (!readSCHMemChart(zone)) {
        STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not open memchart\n"));
        ascFile.addPos(pos);
        ascFile.addNote("SCHAttributes:###memchart");
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
      pos=input->tell();
      f.str("");
      f << "SCHAttributes[" << zone.getRecordLevel() << "]:";
      break;
    default:
      STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: find unknown chart id\n"));
      f << "###chartId=" << nInt16 << ",";
      bNoMore=true;
      break;
    }
  }
  if (version >= 5) {
    bool bIsCopied;
    *input>>bIsCopied;
    if (bIsCopied) f << "isCopied,";
  }
  if (version>=8) {
    double fMinData;
    *input>>fMinData;
    if (fMinData<0||fMinData>0) f << "fMinData=" << fMinData << ",";
  }
  *input>>nInt16;
  if (nInt16) f << "eChartStyle=" << nInt16 << ",";
  f << "linePoints=[";
  for (int i=0; i<9; ++i) {
    *input>>nInt16;
    if (nInt16) f << nInt16 << ",";
    else f << "_,";
  }
  f << "],";
  f << "rowColors=[";
  for (int i=0; i<12; ++i) {
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a color\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    f << color << ",";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "SCHAttributesA:";
  int32_t nGap, nOverlap, nMarkLen, nPieHeight;
  *input >> nGap >> nOverlap >> nMarkLen;
  if (nGap) f << "nGap=" << nGap << ",";
  if (nOverlap) f << "nOverlap=" << nOverlap << ",";
  if (nMarkLen!=100) f << "##nMarklen=" << nMarkLen << ",";
  int32_t dim[4];
  *input >> dim[0] >> dim[1] >> dim[2] >> dim[3] >> nPieHeight;
  f << "rect=" << dim[0] << "x" << dim[1] << "<->" << dim[2] << "x" << dim[3] << ",";
  if (nPieHeight) f << "height[pie]=" << nPieHeight << ",";
  if (version>=6) {
    *input>>nInt16;
    if (nInt16<0||input->tell()+4*nInt16>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read the number of pie segment\n"));
      f << "###n=" << nInt16 << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    f << "pieSegments=[";
    for (int i=0; i<int(nInt16); ++i) {
      int32_t nPieOffset;
      *input >> nPieOffset;
      if (nPieOffset) f << nPieOffset << ",";
      else
        f << "_,";
    }
    f << "],";
  }
  int16_t nXAngle, nYAngle, nZAngle, nCharSet;
  *input >> nXAngle >> nYAngle >> nZAngle >> nCharSet;
  f << "angles=" << nXAngle << "x" << nYAngle << "x" << nZAngle << ",";
  if (nCharSet) f << "charSet=" << nCharSet << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  f.str("");
  f << "SCHAttributesB:";
  for (int i=0; i<5; ++i) {
    bool bShow;
    std::vector<uint32_t> string;
    *input>>bShow;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a string\n"));
      f << "###string,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    static char const *wh[]= {"mainTitle", "subTitle", "xAxisTitle", "yAxisTitle", "zAxisTitle" };
    f << wh[i] << "=" << libstoff::getString(string).cstr();
    if (bShow) f << ":show";
    f << ",";
  }
  for (int i=0; i<3; ++i) {
    for (int j=0; j<4; ++j) {
      bool bShow;
      *input>>bShow;
      if (!bShow) continue;
      static char const *wh[]= {"Axis", "GridMain", "GridHelp", "Descr"};
      f << "show" << char('X'+i) << wh[j] << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool) {
    // CHANGEME
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a pool, create a false one\n"));
      first=false;
    }
    pool=getNewItemPool(StarItemPool::T_Unknown);
  }
  static STOFFVec2i const titleLimitsVec[]= {
    STOFFVec2i(4,4) /*SCHATTR_TEXT_ORIENT*/, STOFFVec2i(53,53) /*SCHATTR_TEXT_ORIENT*/,
    STOFFVec2i(100,100)/*SCHATTR_USER_DEFINED_ATTR*/, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
    STOFFVec2i(1018,1046) /*XATTR_FILL_FIRST, XATTR_FILL_LAST*/, STOFFVec2i(1067,1078) /*XATTR_SHADOW_FIRST, XATTR_SHADOW_LAST*/,
    STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/
  };
  std::vector<STOFFVec2i> titleLimits;
  for (auto i : titleLimitsVec) titleLimits.push_back(i);
  static STOFFVec2i const allAxisLimitsVec[]= {
    STOFFVec2i(4,4) /*SCHATTR_TEXT_ORIENT*/, STOFFVec2i(53,54) /*SCHATTR_TEXT_ORIENT, SCHATTR_TEXT_OVERLAP*/,
    STOFFVec2i(85,85) /* SCHATTR_AXIS_SHOWDESCR */, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
    STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/, STOFFVec2i(30587,30587) /* SID_TEXTBREAK*/
  };
  std::vector<STOFFVec2i> allAxisLimits;
  for (auto i : allAxisLimitsVec) allAxisLimits.push_back(i);
  static STOFFVec2i const compatAxisLimitsVec[]= {
    STOFFVec2i(4,39) /*SCHATTR_TEXT_START, SCHATTR_AXISTYPE*/, STOFFVec2i(53,54)/*SCHATTR_TEXT_DEGREES,SCHATTR_TEXT_OVERLAP*/,
    STOFFVec2i(70,95) /*SCHATTR_AXIS_START,SCHATTR_AXIS_END*/, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
    STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/, STOFFVec2i(30587,30587) /* SID_TEXTBREAK*/,
    STOFFVec2i(10585,10585) /*SID_ATTR_NUMBERFORMAT_VALUE*/
  };
  std::vector<STOFFVec2i> compatAxisLimits;
  for (auto i : compatAxisLimitsVec) compatAxisLimits.push_back(i);
  std::vector<STOFFVec2i> gridLimits;
  gridLimits.push_back(STOFFVec2i(1000,1016)); //XATTR_LINE_FIRST, XATTR_LINE_LAST
  gridLimits.push_back(STOFFVec2i(100,100)); // SCHATTR_USER_DEFINED_ATTR
  std::vector<STOFFVec2i> diagramAreaLimits=gridLimits;
  diagramAreaLimits.push_back(STOFFVec2i(1018,1046)); //XATTR_FILL_FIRST, XATTR_FILL_LAST
  std::vector<STOFFVec2i> legendLimits=diagramAreaLimits;
  legendLimits.push_back(STOFFVec2i(1067,1078)); //XATTR_SHADOW_FIRST, XATTR_SHADOW_LAST
  legendLimits.push_back(STOFFVec2i(3,3)); // SCHATTR_LEGEND_END
  legendLimits.push_back(STOFFVec2i(3994,4037)); // EE_ITEMS_START, EE_ITEMS_END

  for (int i=0; i<10+11; ++i) {
    StarItemSet itemSet;
    if (!readItemSet(zone, i<6 ? titleLimits : i==6 ? allAxisLimits : i<10 ? compatAxisLimits :
                     i<17 ? gridLimits : i < 20 ? diagramAreaLimits : legendLimits, lastPos,
                     itemSet, pool.get(), false) ||
        input->tell()>lastPos) {
      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
  }

  std::vector<STOFFVec2i> rowLimits(1, STOFFVec2i(0,0)); // CHART_ROW_WHICHPAIRS
  int nLoop = version<4 ? 2 : 3;
  for (int loop=0; loop<nLoop; ++loop) { // row attr styles, point attr list, switchPoint attr list
    pos=input->tell();
    f.str("");
    f << "SCHAttributesC:";
    *input>>nInt16;
    f << "n=" << nInt16 << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (int i=0; i<int(nInt16); ++i) {
      StarItemSet itemSet;
      if (!readItemSet(zone,rowLimits,lastPos, itemSet, pool.get(), false) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
#if 0
      std::cout << i << "[" << loop << "]->" << itemSet.printChild() << "\n";
#endif
    }
  }

  pos=input->tell();
  f.str("");
  f << "SCHAttributesD:";
  int16_t nDataDesc;
  bool bShowSym, bSwitchData;
  *input>>nDataDesc >> bShowSym >> bSwitchData;
  if (nDataDesc) f << "dataDesc=" << nDataDesc << ",";
  if (bShowSym) f << "showSym,";
  if (bSwitchData) f << "switchData,";
  if (version==0) {
    bool bDataPercent;
    *input>>bDataPercent;
    if (bDataPercent) f << "dataPercent,";
  }
  uint32_t nValFormat, nValPercentFormat, nDescrFormat, nDescrPercentFormat;
  *input >> nValFormat >> nValPercentFormat >> nDescrFormat >> nDescrPercentFormat;
  f << "valFormat=" << nValFormat << "[" << nValPercentFormat << "],";
  f << "descrFormat=" << nDescrFormat << "[" << nDescrPercentFormat << "],";
  for (int i=0; i<3; ++i) {
    // chaxis.cxx: ChartAxis::LoadMemberCompat
    f << "chart" << char('X'+i) << "=[";
    double mfMin, mfMax, mfStep, mfStepHelp, mfOrigin=0;
    *input>>mfMin>> mfMax>> mfStep>> mfStepHelp;
    // sometimes the zone stops just here...
    if (input->tell()<lastPos) *input >> mfOrigin;
    f << "minMax=" << mfMin << ":" << mfMax << ",";
    f << "step=" << mfStep << "[" << mfStepHelp << "],";
    if (mfOrigin<0||mfOrigin>0) f << "origin=" << mfOrigin << ",";
    f << "],";
    if (input->tell()>=lastPos) {
      if (input->tell()!=lastPos) {
        STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a axis\n"));
        f << "###axis,";
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
  }
  double fMaxData;
  *input >> fMaxData;
  if (fMaxData < 0 || fMaxData>0) f << "fMaxData="  << fMaxData << ",";
  uint32_t moreData=0;
  if (!bNoMore) {
    *input >> moreData;
    f << "moreData=" << moreData << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (moreData>1) {
    pos=input->tell();
    f.str("");
    f << "SCHAttributes[moreData]:";

    for (int i=0; i<4; ++i) {
      std::vector<uint32_t> string;
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a string\n"));
        f << "###string,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
      if (!string.empty()) f << "someData" << i << "=" << libstoff::getString(string).cstr() << ",";
    }
    if (moreData>=3) {
      double fSpotIntensity;
      *input>>fSpotIntensity;
      if (fSpotIntensity<0||fSpotIntensity>0) f << "spotIntensity=" << fSpotIntensity << ",";
    }
    if (moreData>=4) {
      bool bShowAverage, bDummy;
      int16_t errorKind, eIndicate;
      double indicate[4];
      *input >> bShowAverage >> errorKind >> bDummy >> eIndicate;
      if (bShowAverage) f << "show[average],";
      if (errorKind) f << "errorKind=" << errorKind << ",";
      if (eIndicate) f << "eIndicate=" << eIndicate << ",";
      *input >> indicate[0] >> indicate[1] >> indicate[2] >> indicate[3];
      f << "indicate=[" << indicate[0] << "," << indicate[1] << "," << indicate[2] << "," << indicate[3] << "],";
    }
    if (moreData>=5) {
      *input >> nInt16;
      if (nInt16) f << "eRegression=" << nInt16 << ",";
    }
    if (moreData>=6) {
      int32_t splineDepth, granularity;
      *input >> splineDepth >> granularity;
      if (splineDepth) f << "splineDepth=" << splineDepth << ",";
      if (granularity) f << "granularity=" << granularity << ",";
    }
    if (moreData>=7) {
      bool legendVisible;
      *input >> legendVisible;
      if (legendVisible) f << "legendVisible,";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (moreData>=8) {
      for (int loop=0; loop<3; ++loop) {
        pos=input->tell();
        f.str("");
        f << "SCHAttributes[attrList]:";
        *input>>nInt16;
        f << "n=" << nInt16 << ",";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        for (int i=0; i<int(nInt16); ++i) {
          StarItemSet itemSet;
          if (!readItemSet(zone, gridLimits, lastPos, itemSet, pool.get(), false) ||
              input->tell()>lastPos) {
            zone.closeSCHHeader("SCHAttributes");
            return true;
          }
        }
      }

      pos=input->tell();
      f.str("");
      f << "SCHAttributes[moreDataB]:";
    }
    if (moreData>=9 && input->tell()<lastPos) {
      double fAmbientIntensity;
      *input>>fAmbientIntensity;
      if (fAmbientIntensity<0||fAmbientIntensity>0) f << "ambientIntensity=" << fAmbientIntensity << ",";
    }
    if (moreData>=10 && input->tell()<lastPos) {
      bool textScalable;
      *input>>textScalable;
      if (textScalable) f << "textScalable,";
    }
    if (moreData>=11 && input->tell()<lastPos) {
      int32_t sizeX, sizeY;
      *input >> sizeX >> sizeY;
      f << "initSize=" << sizeX << "x" << sizeY << ",";
    }
    if (moreData >= 12 && input->tell()<lastPos) {
      for (int i=0; i<4; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        if (i<3)
          f << "format" << char('X'+i) << "TextInMultiLine,";
        else
          f << "formatLegentTextInMultiLine,";
      }
    }
    if (moreData>=13 && input->tell()<lastPos) {
      for (int i=0; i<3; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        f << char('X'+i) << "AxisTextMaxNumLines=" << nInt16 << ",";
      }
      int32_t widthFirstXAxisText, widthLastXAxisText;
      *input >> widthFirstXAxisText >> widthLastXAxisText;
      if (widthFirstXAxisText) f << "widthFirstXAxisText=" << widthFirstXAxisText << ",";
      if (widthLastXAxisText) f << "widthLastXAxisText=" << widthLastXAxisText << ",";
      for (int i=0; i<2; ++i) {
        int32_t posX, posY;
        *input >> posX >> posY;
        if (posX!=-1 || posY!=-1) f << (i==0 ? "title" : "subTitle") << "TopCenter=" << posX << "x" << posY << ",";
      }
      int32_t diagramRectangle[4];
      *input >> diagramRectangle[0] >> diagramRectangle[1] >> diagramRectangle[2] >> diagramRectangle[3];
      f << "diagRect=" << diagramRectangle[0] << "x" << diagramRectangle[1] << "<->" << diagramRectangle[2] << "x" << diagramRectangle[3] << ",";
      for (int i=0; i<4; ++i) {
        int32_t posX, posY;
        *input >> posX >> posY;
        if (posX==-1 && posY==-1) continue;
        if (i==0)
          f << "legendTopLeft=" << posX << "x" << posY << ",";
        else
          f << "title" << char('X'+i-1) << "AxisPos=" << posX << "x" << posY << ",";
      }
      for (int i=0; i<7; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        static char const *wh[]= {"useRelPosForChartGroup","adjMargForLegend","adjMargForTitle","adjMargForSubTitle",
                                  "adjMargForXAxisTitle","adjMargForYAxisTitle","adjMargForZAxisTitle"
                                 };
        f << wh[i] << ",";
      }
    }
    if (moreData>=14 && input->tell()<lastPos) {
      for (int i=0; i<2; ++i) {
        STOFFColor col;
        if (!input->readColor(col)) {
          STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a color\n"));
          f << "###color,";
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());

          zone.closeSCHHeader("SCHAttributes");
          return true;
        }
        if (!col.isWhite())
          f << (i==0 ? "spotColor" : "ambientColor") << "=" << col << ",";
      }
      for (int i=0; i<7; ++i) {
        bool hasMoved;
        *input>>hasMoved;
        if (!hasMoved) continue;
        static char const *wh[]= {"diagram", "title", "subTitle", "legend", "XAxis", "YAxis", "ZAxis"};
        f << wh[i] << "HasMoved,";
      }
    }
    if (moreData>=15 && input->tell()<lastPos) {
      for (int i=0; i<3; ++i) {
        *input >> nInt16;
        if (!nInt16) continue;
        f << "adjust" << char('X'+i) << "AxesTitle=" << nInt16 << ",";
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
    }

    if (moreData>=16 && input->tell()<lastPos && getFormatManager()) {
      for (int i=0; i<3; ++i) {
        pos=input->tell();
        if (!getFormatManager()->readNumberFormatter(zone)) {
          ascFile.addPos(pos);
          ascFile.addNote("SCHAttributes[format]:####");
          STOFF_DEBUG_MSG(("StarObjectChart::readSCHAttributes: can not read a format zone\n"));
          zone.closeSCHHeader("SCHAttributes");
          return true;
        }
      }
      pos=input->tell();
    }
    if (pos!=input->tell()) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
  }

  pos=input->tell();
  if (pos>=lastPos) {
    zone.closeSCHHeader("SCHAttributes");
    return true;
  }
  f.str("");
  f << "SCHAttributesE:";

  if (version>=9) {
    int16_t barPercentWidth;
    int32_t defaultColSet, numLineColChart, nXLastNumFmt, nYLastNumFmt, nBLastNumFmt;
    *input >> barPercentWidth >> defaultColSet >> numLineColChart >> nXLastNumFmt >> nYLastNumFmt >> nBLastNumFmt;
    if (barPercentWidth) f << "barPercentWidth=" << barPercentWidth << ",";
    if (defaultColSet) f << "defaultColSet=" << defaultColSet << ",";
    if (numLineColChart) f << "numLineColChart=" << numLineColChart << ",";
    if (nXLastNumFmt!=-1 || nYLastNumFmt!=-1 || nBLastNumFmt!=-1)
      f << "lastNumFmt=[" << nXLastNumFmt << "," << nYLastNumFmt << "," << nBLastNumFmt << "],";
    input->seek(4, librevenge::RVNG_SEEK_CUR); // dummy -1
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (version>=11 && input->tell()<lastPos) {
    for (int loop=0; loop<3; ++loop) { // the StockXXXAttr
      StarItemSet itemSet;
      if (!readItemSet(zone,rowLimits,lastPos, itemSet, pool.get(), false) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
    }
  }

  if (version>=12) {
    static STOFFVec2i const axisLimitsVec[]= {
      STOFFVec2i(4,6) /*SCHATTR_TEXT_START, SCHATTR_TEXT_END*/, STOFFVec2i(39,39) /*SCHATTR_AXISTYPE*/,
      STOFFVec2i(53,54)/*SCHATTR_TEXT_DEGREES,SCHATTR_TEXT_OVERLAP*/, STOFFVec2i(70,95) /*SCHATTR_AXIS_START,SCHATTR_AXIS_END*/,
      STOFFVec2i(100,100) /* SCHATTR_USER_DEFINED_ATTR */, STOFFVec2i(1000,1016) /*XATTR_LINE_FIRST, XATTR_LINE_LAST*/,
      STOFFVec2i(3994,4037) /*EE_ITEMS_START, EE_ITEMS_END*/, STOFFVec2i(11432,11432) /* SID_ATTR_NUMBERFORMAT_VALUE */,
      STOFFVec2i(30587,30587) /* SID_TEXTBREAK*/, STOFFVec2i(10585,10585) /*SID_ATTR_NUMBERFORMAT_VALUE*/
    };
    std::vector<STOFFVec2i> axisLimits;
    for (int i=0; i<8; ++i) axisLimits.push_back(axisLimitsVec[i]);
    while (input->tell()<lastPos) {
      pos=input->tell();
      int32_t nAxisId;
      *input >> nAxisId;
      f.str("");
      f << "SCHAttributesE:id=" << nAxisId << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      if (nAxisId==-1) break;
      StarItemSet itemSet;
      if (!readItemSet(zone,axisLimits,lastPos, itemSet, pool.get(), false) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
    }
  }

  pos=input->tell();
  f.str("");
  f << "SCHAttributesF:";
  if (version>=14 && input->tell()<lastPos) {
    uint32_t nGap1, nOverlap1, nGap2, nOverlap2;
    *input >> nGap1 >> nOverlap1 >> nGap2 >> nOverlap2;
    if (nGap1) f << "nGap1=" << nGap1 << ",";
    if (nOverlap1) f << "nOverlap1=" << nOverlap1 << ",";
    if (nGap2) f << "nGap2=" << nGap2 << ",";
    if (nOverlap2) f << "nOverlap2=" << nOverlap2 << ",";
  }
  if (version>=15 && input->tell()<lastPos) {
    bool bDiagramHasBeenMoved;
    *input >> bDiagramHasBeenMoved;
    if (bDiagramHasBeenMoved) f << "diagram[movedOrRemoved],";
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  zone.closeSCHHeader("SCHAttributes");
  return true;
}

bool StarObjectChart::readSCHMemChart(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  // memchrt.cxx: operator>> (... SchMemChart&)
  if (!zone.openSCHHeader()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCHMemChart)[" << zone.getRecordLevel() << "]:";
  int version=zone.getHeaderVersion();
  f << "vers=" << version << ",";
  uint16_t nCol, nRow;
  *input >> nCol >> nRow;
  f << "nCol=" << nCol << ",nRow=" << nRow << ",";
  if (static_cast<unsigned long>(lastPos-input->tell())<8*static_cast<unsigned long>(nCol)*static_cast<unsigned long>(nRow) ||
      static_cast<unsigned long>(input->tell())+8*static_cast<unsigned long>(nCol)*static_cast<unsigned long>(nRow)>static_cast<unsigned long>(lastPos)) {
    STOFF_DEBUG_MSG(("StarObjectChart::readSCHMemChart: dim seems bad\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSCHHeader("SCHMemChart");
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (nCol && nRow) {
    for (int c=0; c < int(nCol); ++c) {
      pos=input->tell();
      f.str("");
      f << "SCHMemChart-col" << c << ":val=[";
      for (int r=0; r < int(nRow); ++r) {
        double val;
        *input>>val;
        f << val << ",";
      }
      f << "],";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
  }
  pos=input->tell();
  f.str("");
  f << "SCHMemChart-A:";
  int16_t charSet, nDataType;
  *input>>charSet;
  if (charSet) f << "charSet=" << charSet << ",";
  for (int i=0; i<5+int(nCol)+int(nRow); ++i) {
    std::vector<uint32_t> string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectChart::readSCHMemChart: can not read a title\n"));
      f << "###title";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCHHeader("SCHMemChart");
      return true;
    }
    if (string.empty()) continue;
    if (i<5) {
      static char const *wh[]= {"mainTitle","subTitle","xAxisTitle","yAxisTitle","zAxisTitle"};
      f << wh[i] << "=" << libstoff::getString(string).cstr() << ",";
    }
    else if (i<5+int(nCol))
      f << "colTitle" << i-5 << "=" << libstoff::getString(string).cstr() << ",";
    else
      f << "rowTitle" << i-5-int(nCol) << "=" << libstoff::getString(string).cstr() << ",";
  }
  *input >> nDataType;
  if (nDataType) f << "dataType=" << nDataType << ",";
  if (version>=1) {
    int32_t val;
    f << "colTable=[";
    for (int i=0; i<int(nCol); ++i) {
      *input >> val;
      if (val)
        f << val << ",";
      else
        f << "_,";
    }
    f << "],";
    f << "rowTable=[";
    for (int i=0; i<int(nRow); ++i) {
      *input >> val;
      if (val)
        f << val << ",";
      else
        f << "_,";
    }
    f << "],";
  }
  if (version>=2) {
    int32_t nTranslated;
    *input >> nTranslated;
    if (nTranslated) f << "translated=" << nTranslated << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSCHHeader("SCHMemChart");
  return true;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
