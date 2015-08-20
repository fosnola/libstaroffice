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

#include "STOFFOLEParser.hxx"

#include "StarFileManager.hxx"
#include "StarAttribute.hxx"
#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"
#include "StarDocument.hxx"
#include "StarItemPool.hxx"
#include "StarZone.hxx"

#include "SDCParser.hxx"

/** Internal: the structures of a SDCParser */
namespace SDCParserInternal
{
////////////////////////////////////////
//! Internal: the state of a SDCParser
struct State {
  //! constructor
  State()
  {
  }
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SDCParser::SDCParser() : m_state(new SDCParserInternal::State)
{
}

SDCParser::~SDCParser()
{
}

////////////////////////////////////////////////////////////
//
// Intermediate level
//
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
bool SDCParser::readChartDocument(STOFFInputStreamPtr input, std::string const &name, StarDocument &document)
{
  StarZone zone(input, name, "SWChartDocument");
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  libstoff::DebugStream f;
  f << "Entries(SCChartDocument):";
  // sch_docshell.cxx: SchChartDocShell::Load
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not find header zone\n"));
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
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: bad version\n"));
    f << "###n=" << n << ",";
  }
  ascFile.addPos(0);
  ascFile.addNote(f.str().c_str());
  long pos=input->tell(), lastPos= zone.getRecordLastPosition();
  shared_ptr<StarItemPool> pool=document.getNewItemPool(StarItemPool::T_VCControlPool);
  if (!pool || !pool->read(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);

  pos=input->tell();
  StarFileManager fileManager;
  if (pos!=lastPos && !fileManager.readJobSetUp(zone))
    input->seek(pos, librevenge::RVNG_SEEK_SET);

  zone.closeRecord("SCChartDocument");

  if (input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find end after first zone\n"));
    return true;
  }

  pos=input->tell();
  if (!readSdrModel(zone)) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not find the SdrModel\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SCChartDocument:###SdrModel");
    return true;
  }

  pos=input->tell();
  if (input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find no attribute\n"));
    return true;
  }
  f.str("");
  f << "SCChartDocument[attributes]:";
  if (!zone.openSCHHeader()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not open SCHAttributes\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  f << "vers=" << zone.getSCHVersion() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  // chtmode2.cxx ChartModel::LoadAttributes
  if (!readSCHAttributes(zone)) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: can not open the attributes zone\n"));
  }
  zone.closeSCHHeader("SCChartDocumentAttr");

  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readChartDocument: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SCChartDocument:##extra");
  }

  return true;
}

bool SDCParser::readSfxStyleSheets(STOFFInputStreamPtr input, std::string const &name, StarDocument &document)
{
  StarZone zone(input, name, "SfxStyleSheets");
  input->seek(0, librevenge::RVNG_SEEK_SET);
  libstoff::DebugFile &ascFile=zone.ascii();
  ascFile.open(name);

  StarFileManager fileManager;
  // first check for calc: sc_docsh.cxx ScDocShell::LoadCalc and sc_document.cxx: ScDocument::LoadPool
  uint16_t nId;
  *input >> nId;
  if (nId==0x4210 || nId==0x4213) {
    long pos=0;
    libstoff::DebugStream f;
    f << "Entries(SfxStyleSheets):id=" << std::hex << nId << std::dec << ",calc,";
    if (!zone.openSCRecord()) {
      STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not open main zone\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    long lastPos=zone.getRecordLastPosition();
    int poolVers=1;
    while (input->tell()+6 < lastPos) {
      pos=input->tell();
      f.str("");
      *input >> nId;
      f << "SfxStyleSheets-1:id=" << std::hex << nId << std::dec << ",";
      if (!zone.openSCRecord()) {
        STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not open second zone\n"));
        f << "###";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        break;
      }
      switch (nId) {
      case 0x4211:
      case 0x4214: {
        f << (nId==0x411 ? "pool" : "pool[edit]") << ",";
        shared_ptr<StarItemPool> pool=document.getNewItemPool(nId==0x4211 ? StarItemPool::T_SpreadsheetPool : StarItemPool::T_EditEnginePool);
        if (pool && pool->read(zone))
          poolVers=pool->getVersion();
        else {
          STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not readPoolZone\n"));
          f << "###";
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
          break;
        }
        break;
      }
      case 0x4212:
        f << "style[pool],";
        if (!StarItemPool::readStyle(zone, poolVers)) {
          STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: can not readStylePool\n"));
          f << "###";
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
          break;
        }
        break;
      case 0x422c: {
        uint8_t cSet, cGUI;
        *input >> cGUI >> cSet;
        f << "charset=" << int(cSet) << ",";
        if (cGUI) f << "gui=" << int(cGUI) << ",";
        break;
      }
      default:
        STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: find unexpected tag\n"));
        input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        f << "###";
        break;
      }
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCRecord("SfxStyleSheets");
    }
    zone.closeSCRecord("SfxStyleSheets");

    if (!input->isEnd()) {
      STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: find extra data\n"));
      ascFile.addPos(input->tell());
      ascFile.addNote("SfxStyleSheets:###extra");
    }
    return true;
  }

  // check for chart sch_docshell.cxx SchChartDocShell::Load
  input->seek(0, librevenge::RVNG_SEEK_SET);
  while (!input->isEnd()) {
    long pos=input->tell();
    shared_ptr<StarItemPool> pool=document.getNewItemPool(StarItemPool::T_Unknown); // CHANGEME
    if (pool && pool->read(zone))
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (input->isEnd()) return true;
  long pos=input->tell();
  if (!StarItemPool::readStyle(zone, input->peek()==3 ? 2 : 1))
    input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (!input->isEnd()) {
    STOFF_DEBUG_MSG(("SDCParser::readSfxStyleSheets: find extra data\n"));
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
bool SDCParser::readSCHAttributes(StarZone &zone)
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
  int version=zone.getSCHVersion();
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
        STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not open memchart\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: find unknown chart id\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a color\n"));
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
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read the number of pie segment\n"));
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
    librevenge::RVNGString string;
    *input>>bShow;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a string\n"));
      f << "###string,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
    static char const *(wh[])= {"mainTitle", "subTitle", "xAxisTitle", "yAxisTitle", "zAxisTitle" };
    f << wh[i] << "=" << string.cstr();
    if (bShow) f << ":show";
    f << ",";
  }
  for (int i=0; i<3; ++i) {
    for (int j=0; j<4; ++j) {
      bool bShow;
      *input>>bShow;
      if (!bShow) continue;
      static char const *(wh[])= {"Axis", "GridMain", "GridHelp", "Descr"};
      f << "show" << char('X'+i) << wh[j] << ",";
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  StarFileManager fileManager;
  for (int i=0; i<10 + 11; ++i) {
    if (!fileManager.readSfxItemList(zone)) {
      zone.closeSCHHeader("SCHAttributes");
      return true;
    }
  }

  int nLoop = version<4 ? 2 : 3;
  for (int loop=0; loop<nLoop; ++loop) {
    pos=input->tell();
    f.str("");
    f << "SCHAttributesC:";
    *input>>nInt16;
    f << "n=" << nInt16 << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    for (int i=0; i<int(nInt16); ++i) {
      if (!fileManager.readSfxItemList(zone)) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
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
    double mfMin, mfMax, mfStep, mfStepHelp, mfOrigin;
    *input>>mfMin>> mfMax>> mfStep>> mfStepHelp>> mfOrigin;
    f << "minMax=" << mfMin << ":" << mfMax << ",";
    f << "step=" << mfStep << "[" << mfStepHelp << "],";
    if (mfOrigin<0||mfOrigin>0) f << "origin=" << mfOrigin << ",";
    f << "],";
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
      librevenge::RVNGString string;
      if (!zone.readString(string)||input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a string\n"));
        f << "###string,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
      if (!string.empty()) f << "someData" << i << "=" << string.cstr() << ",";
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
          if (!fileManager.readSfxItemList(zone)) {
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
          STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a color\n"));
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

    if (moreData>=16 && input->tell()<lastPos) {
      SWFormatManager formatManager;
      for (int i=0; i<3; ++i) {
        pos=input->tell();
        if (!formatManager.readNumberFormatter(zone)) {
          ascFile.addPos(pos);
          ascFile.addNote("SCHAttributes[format]:####");
          STOFF_DEBUG_MSG(("SDCParser::readSCHAttributes: can not read a format zone\n"));
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
      if (!fileManager.readSfxItemList(zone) || input->tell()>lastPos) {
        zone.closeSCHHeader("SCHAttributes");
        return true;
      }
    }

    pos=input->tell();
  }

  if (version>=12) {
    while (input->tell()<lastPos) {
      pos=input->tell();
      int32_t nAxisId;
      *input >> nAxisId;
      f.str("");
      f << "SCHAttributesE:id=" << nAxisId << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      if (nAxisId==-1) break;
      if (!fileManager.readSfxItemList(zone) || input->tell()>lastPos) {
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

bool SDCParser::readSCHMemChart(StarZone &zone)
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
  int version=zone.getSCHVersion();
  f << "vers=" << version << ",";
  uint16_t nCol, nRow;
  *input >> nCol >> nRow;
  f << "nCol=" << nCol << ",nRow=" << nRow << ",";
  if (input->tell()+8*long(nCol)*long(nRow)>lastPos) {
    STOFF_DEBUG_MSG(("SDCParser::readSCHMemChart: dim seems bad\n"));
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
    librevenge::RVNGString string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSCHMemChart: can not read a title\n"));
      f << "###title";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSCHHeader("SCHMemChart");
      return true;
    }
    if (string.empty()) continue;
    if (i<5) {
      static char const *(wh[])= {"mainTitle","subTitle","xAxisTitle","yAxisTitle","zAxisTitle"};
      f << wh[i] << "=" << string.cstr() << ",";
    }
    else if (i<5+int(nCol))
      f << "colTitle" << i-5 << "=" << string.cstr() << ",";
    else
      f << "rowTitle" << i-5-int(nCol) << "=" << string.cstr() << ",";
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

bool SDCParser::readSdrLayer(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrLy") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrLayerDef)[" << zone.getRecordLevel() << "]:";
  // svdlayer.cxx operator>>(..., SdrLayer &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayer: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getSDRVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLy") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayer: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }

  uint8_t layerId;
  *input>>layerId;
  if (layerId) f << "layerId=" << int(layerId) << ",";
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayer: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrLayerDef");
    return true;
  }
  f << string.cstr() << ",";
  if (version>=1) {
    uint16_t nType;
    *input>>nType;
    if (nType==0) f << "userLayer,";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrLayerDef");
  return true;
}

bool SDCParser::readSdrLayerSet(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrLS") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrLayerSet)[" << zone.getRecordLevel() << "]:";
  // svdlayer.cxx operator>>(..., SdrLayerSet &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayerSet: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getSDRVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrLS") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayerSet: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  for (int i=0; i<2; ++i) { // checkme: set of 32 bytes ?
    f << (i==0 ? "member" : "exclude") << "=[";
    for (int j=0; j<32; ++j) {
      int c=(int) input->readULong(1);
      if (c)
        f << c << ",";
      else
        f << ",";
    }
    f << "],";
  }
  librevenge::RVNGString string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("SDCParser::readSdrLayerSet: can not read a string\n"));
    f << "###string";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrLayerSet");
    return true;
  }
  f << string.cstr() << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrLayerSet");
  return true;
}

bool SDCParser::readSdrModel(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrMd") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrModel)[" << zone.getRecordLevel() << "]:";
  // svdmodel.cxx operator>>(..., SdrModel &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getSDRVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrMd" || version>17) {
    STOFF_DEBUG_MSG(("SDCParser::readSdrModel: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();

  f.str("");
  f << "SdrModelA[" << zone.getRecordLevel() << "]:";
  // SdrModel::ReadData
  if (!zone.openRecord()) { // SdrModelA1
    STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not open downCompat\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrModel");
    return true;
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f.str("");
  f << "SdrModelA[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  if (version>=11) {
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    f << "joeM";
    if (magic!="JoeM" || !zone.openRecord()) { // SdrModelA2
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: unexpected magic2 data\n"));
      f << "###magic" << magic;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrModelA1");
      zone.closeSDRHeader("SdrModel");
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SdrModelInfo[" << zone.getRecordLevel() << "]:";
    if (!zone.openRecord()) { // SdrModelInfo
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read model info\n"));
      f << "###";
      ok=false;
    }
    else {
      // operator>>(..., SdrModelInfo&)
      uint32_t date, time;
      uint8_t enc;
      *input >> date >> time >> enc;
      f << "date=" << date << ",time=" << time << ",encoding=" << int(enc) << ",";
      input->seek(3, librevenge::RVNG_SEEK_CUR); // creationGUI, creationCPU, creationSys
      *input >> date >> time >> enc;
      f << "modDate=" << date << ", modTime=" << time << ",encoding[mod]=" << int(enc) << ",";
      input->seek(3, librevenge::RVNG_SEEK_CUR); // writeGUI, writeCPU, writeSys
      *input >> date >> time >> enc;
      f << "lastReadDate=" << date << ", lastReadTime=" << time << ",encoding[lastRead]=" << int(enc) << ",";
      input->seek(3, librevenge::RVNG_SEEK_CUR); // lastReadGUI, lastReadCPU, lastReadSys
      *input >> date >> time;
      f << "lastPrintDate=" << date << ", lastPrintTime=" << time << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeRecord("SdrModelInfo");
      pos=input->tell();
      f.str("");
      f << "SdrModelStats[" << zone.getRecordLevel() << "]:";
      if (!zone.openRecord()) {
        STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read model statistic\n"));
        f << "###";
        ok=false;
      }
      else {
        zone.closeRecord("SdrModelStat");
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());

        pos=input->tell();
        f.str("");
        f << "SdrModelFormatCompat[" << zone.getRecordLevel() << "]:";
      }
    }
    if (ok&&!zone.openRecord()) {
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read model format compat\n"));
      f << "###";
      ok=false;
    }
    else if (ok) {
      if (input->tell()+6<=zone.getRecordLastPosition()) {
        int16_t nTmp; // find 0
        *input >> nTmp;
        if (nTmp) f << "unk=" << nTmp << ",";
      }
      if (input->tell()+4<=zone.getRecordLastPosition()) {
        f << "nStreamCompressed=" << input->readULong(2) << ",";
        f << "nStreamNumberFormat=" << input->readULong(2) << ",";
      }
      zone.closeRecord("SdrModelFormatCompat");
    }

    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    pos=input->tell();
    f.str("");
    f << "SdrModelA[" << zone.getRecordLevel() << "]-data:";
  }
  if (ok) {
    int32_t nNum, nDen;
    uint16_t nTmp;
    *input>> nNum >> nDen >> nTmp;
    f << "objectUnit=" << nNum << "/" << nDen << ",";
    if (nTmp) f << "eObjUnit=" << nTmp << ",";
    *input>>nTmp;
    if (nTmp==1) f << "pageNotValid,";
    input->seek(2, librevenge::RVNG_SEEK_CUR); // ???, reserved

    if (version<1) {
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: reading model format for version 0 is not implemented\n"));
      f << "###version0";
      ok=false;
    }
    else {
      if (version<11) f << "nCharSet=" << input->readLong(2) << ",";
      librevenge::RVNGString string;
      for (int i=0; i<6; ++i) {
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SDCParser::readSdrModel: can not read a string\n"));
          f << "###string";
          ok=false;
          break;
        }
        if (string.empty()) continue;
        static char const *(wh[])= {"cTableName", "dashName", "lineEndName", "hashName", "gradientName", "bitmapName"};
        f << wh[i] << "=" << string.cstr() << ",";
      }
    }
  }
  if (ok && version>=12 && input->tell()<zone.getRecordLastPosition()) {
    int32_t nNum, nDen;
    uint16_t nTmp;
    *input>> nNum >> nDen >> nTmp;
    f << "UIUnit=" << nNum << "/" << nDen << ",";
    if (nTmp) f << "eUIUnit=" << nTmp << ",";
  }
  if (ok && version>=13 && input->tell()<zone.getRecordLastPosition()) {
    int32_t nNum1, nNum2;
    *input>> nNum1 >> nNum2;
    if (nNum1) f << "nDefTextHgt=" << nNum1 << ",";
    if (nNum2) f << "nDefTab=" << nNum2 << ",";
  }
  if (ok && version>=14 && input->tell()<zone.getRecordLastPosition()) {
    uint16_t pNum;
    *input>> pNum;
    if (pNum) f << "nStartPageNum=" << pNum << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (version>=11) {
    zone.closeRecord("SdrModelA2");
    ok=true;
  }

  while (ok) {
    pos=input->tell();
    if (pos+4>zone.getRecordLastPosition())
      break;
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrLy" && readSdrLayer(zone)) continue;
    if (magic=="DrLS" && readSdrLayerSet(zone)) continue;
    if ((magic=="DrPg" || magic=="DrMP") && readSdrPage(zone)) continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openSDRHeader(magic)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SdrModel[" << magic << "-" << zone.getRecordLevel() << "]:";
    if (magic!="DrXX") {
      STOFF_DEBUG_MSG(("SDCParser::readSdrModel: find unexpected child\n"));
      f << "###type";
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrModel");
  }

  zone.closeRecord("SdrModelA1");
  zone.closeSDRHeader("SdrModel");
  return true;
}

bool SDCParser::readSdrObject(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrOb" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrObject)[" << zone.getRecordLevel() << "]:";
  int version=zone.getSDRVersion();
  f << magic << ",nVers=" << version << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  if (lastPos==input->tell()) {
    zone.closeSDRHeader("SdrObject");
    return true;
  }
  // svdobject.cxx SdrObjFactory::MakeNewObject
  pos=input->tell();
  f.str("");
  f << "SdrObject:";
  magic="";
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  uint16_t identifier;
  *input>>identifier;
  f << magic << ", ident=" << std::hex << identifier << std::dec << ",";
  if (magic=="SVDr") {
    static bool first=true;
    if (first) { // see SdrObjFactory::MakeNewObject
      STOFF_DEBUG_MSG(("SDCParser::readSdrObject: read SVDr object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else if (magic=="SCHU") {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("SDCParser::readSdrObject: read SCHU object is not implemented\n"));
      first=false;
    }
    f << "##";
  }
  else {
    STOFF_DEBUG_MSG(("SDCParser::readSdrObject: find unknown magic\n"));
    f << "###";
  }
  ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeSDRHeader("SdrObject");
  return true;
}

bool SDCParser::readSdrPage(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrPg" && magic!="DrMP") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrPageDef)[" << zone.getRecordLevel() << "]:";
  if (magic=="DrMP") f << "master,";
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getSDRVersion();
  f << magic << ",nVers=" << version << ",";

  if (magic!="DrPg" && magic!="DrMP") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();

  f.str("");
  f << "SdrPageDefA[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx SdrPageDef::ReadData
  if (!zone.openRecord()) { // SdrPageDefA1
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not open downCompat\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSDRHeader("SdrPageDef");
    return true;
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  f.str("");
  f << "SdrPageDefA[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  if (version>=11) {
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    f << "joeM";
    if (magic!="JoeM" || !zone.openRecord()) { // SdrPageDefA2
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: unexpected magic2 data\n"));
      f << "###magic" << magic;
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeRecord("SdrPageDefA1");
      zone.closeSDRHeader("SdrPageDef");
      return true;
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SdrPageDefData[" << zone.getRecordLevel() << "]:";
  }
  if (ok) {
    int32_t nWdt, nHgt, nBordLft, nBordUp, nBordRgt, nBordLwr;
    uint16_t n;
    *input >> nWdt >> nHgt >> nBordLft >> nBordUp >> nBordRgt >> nBordLwr >> n;
    f << "dim=" << nWdt << "x" << nHgt << ",";
    f << "border=[" << nBordLft << "x" << nBordUp << "-" << nBordRgt << "x" << nBordLwr << "],";
    if (n) f << "n=" << n << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (version>=11) {
    zone.closeRecord("SdrPageDefData");
    ok=true;
  }

  long lastPos=zone.getRecordLastPosition();
  while (ok) {
    pos=input->tell();
    if (pos+4>lastPos)
      break;
    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrLy" && readSdrLayer(zone)) continue;
    if (magic=="DrLS" && readSdrLayerSet(zone)) continue;
    if (magic=="DrMP" && readSdrMPageDesc(zone)) continue;
    if (magic=="DrML" && readSdrMPageDescList(zone)) continue;
    break;
  }
  if (ok && version==0) {
    pos=input->tell();
    f.str("");
    f << "SdrPageDef[masterPage]:";
    uint16_t n;
    *input >> n;
    if (pos+2+2*n>lastPos) {
      STOFF_DEBUG_MSG(("SDCParser::readSdrPage: n is bad\n"));
      f << "###n=" << n << ",";
      ok=false;
    }
    else {
      f << "pages=[";
      for (int i=0; i<int(n); ++i) f << input->readULong(2) << ",";
      f << "],";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  // SdrObjList::Load
  while (ok) {
    pos=input->tell();
    if (pos+4>lastPos)
      break;
    if (readSdrObject(zone)) continue;

    magic="";
    for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (magic=="DrXX" && zone.openSDRHeader(magic)) {
      f.str("");
      f << "SdrPageDef[DrXX-" << zone.getRecordLevel() << "]:";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeSDRHeader("SdrPageDef");
      break;
    }
    ok=false;
    STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not find end object zone\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  if (ok && version>=16) {
    pos=input->tell();
    f.str("");
    f << "SdrPageDef[background-" << zone.getRecordLevel() << "]:";
    bool background;
    *input>>background;
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (background) {
      if (!readSdrObject(zone)) {
        STOFF_DEBUG_MSG(("SDCParser::readSdrPage: can not find backgroound object\n"));
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        ok=false;
      }
    }
  }

  zone.closeRecord("SdrPageDefA1");
  zone.closeSDRHeader("SdrPageDef");
  return true;
}

bool SDCParser::readSdrMPageDesc(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrMP") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrMPageDesc)[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx operator>>(..., SdrMaterPageDescriptor &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDesc: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getSDRVersion();
  f << magic << ",nVers=" << version << ",";
  if (magic!="DrMP") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDesc: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  uint16_t nPageNum;
  *input >> nPageNum;
  f << "pageNum=" << nPageNum << ",";
  f << "aVisLayer=[";
  for (int i=0; i<32; ++i) {
    int c=(int) input->readULong(1);
    if (c)
      f << c << ",";
    else
      f << ",";
  }
  f << "],";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSDRHeader("SdrMPageDesc");
  return true;
}

bool SDCParser::readSdrMPageDescList(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=(char) input->readULong(1);
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrML") return false;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrMPageList)[" << zone.getRecordLevel() << "]:";
  // svdpage.cxx operator>>(..., SdrMaterPageDescListriptor &)
  if (!zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDescList: can not open the SDR Header\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  int version=zone.getSDRVersion();
  uint16_t n;
  *input>>n;
  f << magic << ",nVers=" << version << ",N=" << n << ",";

  if (magic!="DrML") {
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDescList: unexpected magic data\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  for (int i=0; i<n; ++i) {
    pos=input->tell();
    if (pos<lastPos && readSdrMPageDesc(zone)) continue;
    STOFF_DEBUG_MSG(("SDCParser::readSdrMPageDescList: can not find a page desc\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SdrMPageList:###pageDesc");
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  zone.closeSDRHeader("SdrMPageList");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
