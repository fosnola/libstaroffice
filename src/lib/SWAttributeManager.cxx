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

#include "SDWParser.hxx"
#include "SWFieldManager.hxx"
#include "SWFormatManager.hxx"
#include "SWZone.hxx"

#include "SWAttributeManager.hxx"

/** Internal: the structures of a SWAttributeManager */
namespace SWAttributeManagerInternal
{
////////////////////////////////////////
//! Internal: the state of a SWAttributeManager
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
SWAttributeManager::SWAttributeManager() : m_state(new SWAttributeManagerInternal::State)
{
}

SWAttributeManager::~SWAttributeManager()
{
}

bool SWAttributeManager::readAttribute(SWZone &zone, SDWParser &manager)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='A' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // sw_sw3fmts.cxx InAttr
  libstoff::DebugStream f;
  f << "Entries(SWAttributeDef)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  int nWhich=(int) input->readULong(2);
  if (nWhich>0x6001 && zone.getDocumentVersion()!=0x0219) // bug correction 0x95500
    nWhich+=15;
  f << "wh=" << std::hex << nWhich << std::dec << ",";
  int nVers=(int) input->readULong(2);
  if (nVers) f << "nVers=" << nVers << ",";
  int val;
  if (fl&0x10) {
    val=(int) input->readULong(2);
    if (val) f << "nBgin=" << val << ",";
  }
  if (fl&0x20) {
    val=(int) input->readULong(2);
    if (val) f << "nEnd=" << val << ",";
  }
  zone.closeFlagZone();

  long lastPos=zone.getRecordLastPosition();
  switch (nWhich) {
  case 0x1000: // no data
    f << "chrAtrCaseMap,";
    break;
  case 0x1001: {
    f << "chrAtrCharSetColor=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find a color\n"));
      f << "###aColor,";
      break;
    }
    if (!color.isBlack())
      f << color << ",";
    break;
  }
  case 0x1002: {
    f << "chrAtrColor,";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find a color\n"));
      f << "###aColor,";
      break;
    }
    if (!color.isBlack())
      f << color << ",";
    break;
  }
  case 0x1003:
    f << "chrAtrContour=" << input->readULong(1) << ",";
    break;
  case 0x1004:
    f << "chrAtrCrossedOut=" << input->readULong(1) << ",";
    break;
  case 0x1005:
    f << "chrAtrEscapement=" << input->readULong(1) << ",";
    f << "nEsc=" << input->readLong(2) << ",";
    break;
  case 0x1006:
  case 0x1015:
  case 0x101a: {
    if (nWhich==0x1006)
      f << "chrAtrFont,";
    else if (nWhich==0x1015)
      f << "chrAtrCJKFont,";
    else
      f << "chrAtrCTLFont,";
    f << "family=" << input->readULong(1) << ",";
    f << "ePitch=" << input->readULong(1) << ",";
    int encoding=(int) input->readULong(1);
    f << "eTextEncoding=" << encoding << ",";
    librevenge::RVNGString fName, string;
    if (!zone.readString(fName)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the name\n"));
      f << "###aName,";
      break;
    }
    if (!fName.empty())
      f << "aName=" << fName.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the style\n"));
      f << "###aStyle,";
      break;
    }
    if (!string.empty())
      f << "aStyle=" << string.cstr() << ",";
    if (encoding!=10 && fName=="StarBats" && input->tell()<lastPos) {
      if (input->readULong(4)==0xFE331188) {
        // reread data in unicode
        if (!zone.readString(fName)) {
          STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the name\n"));
          f << "###aName,";
          break;
        }
        if (!fName.empty())
          f << "aNameUni=" << fName.cstr() << ",";
        if (!zone.readString(string)) {
          STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the style\n"));
          f << "###aStyle,";
          break;
        }
        if (!string.empty())
          f << "aStyleUni=" << string.cstr() << ",";
      }
      else input->seek(-3, librevenge::RVNG_SEEK_CUR);
    }
    break;
  }
  case 0x1007:
  case 0x1016:
  case 0x101b:
    if (nWhich==0x1007)
      f << "chrAtrFontSize,";
    else if (nWhich==0x1016)
      f << "chrAtrCJKFontSize,";
    else
      f << "chrAtrCTLFontSize,";
    f << "size=" << input->readULong(2);
    f << "nProp=" << input->readULong((nVers>=1) ? 2 : 1);
    if (nVers>=2) f << "nPropUnit=" << input->readULong(2) << ",";
    break;
  case 0x1008:
    f << "chrAtrKerning=" << input->readULong(1) << ",";
    break;
  case 0x1009:
  case 0x1017:
  case 0x101c:
    f << (nWhich==0x1009 ? "chrAtrLanguage": nWhich==0x1017 ? "chrAtrCJKLanguage" : "chrAtrCTLLanguage");
    f << "=" << input->readULong(2) << ",";
    break;
  case 0x100a:
  case 0x1018:
  case 0x101d:
    f << (nWhich==0x100a ? "chrAtrPosture": nWhich==0x1018 ? "chrAtrCJKPosture" : "chrAtrCTLPosture");
    f << "=" << input->readULong(1) << ",";
    break;
  case 0x100b:
    f << "chrAtrProportionFontSize,";
    f << "size=" << input->readULong(2) << ",";
    break;
  case 0x100c:
    f << "chrAtrShadowed=" << input->readULong(1) << ",";
    break;
  case 0x100d:
    f << "chrAtrUnderline=" << input->readULong(1) << ",";
    break;
  case 0x100e:
  case 0x1019:
  case 0x101e:
    f << (nWhich==0x100e ? "chrAtrWeight" : nWhich==0x1019 ? "chrAtrCJKWeight" : "chrAtrCTLWeight");
    f << "=" << input->readULong(1) << ",";
    break;
  case 0x100f:
    f << "chrAtrWordlineMode=" << input->readULong(1) << ",";
    break;
  case 0x1010:
    f << "chrAtrAutoKern=" << input->readULong(1) << ",";
    break;
  case 0x1011:
    f << "chrAtrBlink=" << input->readULong(1) << ",";
    break;
  case 0x1012:
    f << "chrAtrNoHyphen=" << input->readULong(1) << ",";
    break;
  case 0x1013:
    f << "chrAtrNoLineBreak=" << input->readULong(1) << ",";
    break;
  case 0x1014:
  case 0x5011: {
    f << (nWhich==0x1014 ? "chrAtrBackground" : "background") << "=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read color\n"));
      f << "###color,";
      break;
    }
    else if (!color.isWhite())
      f << "color=" << color << ",";
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read fill color\n"));
      f << "###fillcolor,";
      break;
    }
    else if (!color.isWhite())
      f << "fillcolor=" << color << ",";
    f << "nStyle=" << input->readULong(1) << ",";
    if (nVers<1) break;
    int doLoad=(int) input->readULong(2);
    if (doLoad & 1) { // TODO: svx_frmitems.cxx:1948
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: do not know how to load graphic\n"));
      f << "###graphic,";
      break;
    }
    if (doLoad & 2) {
      librevenge::RVNGString link;
      if (!zone.readString(link)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the link\n"));
        f << "###link,";
        break;
      }
      if (!link.empty())
        f << "link=" << link.cstr() << ",";
    }
    if (doLoad & 4) {
      librevenge::RVNGString filter;
      if (!zone.readString(filter)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the filter\n"));
        f << "###filter,";
        break;
      }
      if (!filter.empty())
        f << "filter=" << filter.cstr() << ",";
    }
    f << "nPos=" << input->readULong(1) << ",";
    break;
  }
  case 0x101f:
    f << "chrAtrRotate,";
    f << "nVal=" << input->readULong(2) << ",";
    f << "b=" << input->readULong(1) << ",";
    break;
  case 0x1020:
    f << "chrAtrEmphasisMark=" << input->readULong(2) << ",";
    break;
  case 0x1021: { // checkme
    f << "chrAtrTwoLines=" << input->readULong(1) << ",";
    f << "nStart[unicode]=" << input->readULong(2) << ",";
    f << "nEnd[unicode]=" << input->readULong(2) << ",";
    break;
  }
  case 0x1022:
    f << "chrAtrScaleW=" << input->readULong(2);
    if (input->tell()<lastPos) {
      f << "nVal=" << input->readULong(2) << ",";
      f << "test=" << input->readULong(2) << ",";
    }
    break;
  case 0x1023:
    f << "chrAtrRelief=" << input->readULong(2);
    break;
  case 0x1024: // ok no data
    f << "chrAtrDummy1,";
    break;

  // text attribute
  case 0x2000: {
    f << "textAtrInetFmt,";
    // SwFmtINetFmt::Create
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find string\n"));
      f << "###url,";
      break;
    }
    if (!string.empty())
      f << "url=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find string\n"));
      f << "###targert,";
      break;
    }
    if (!string.empty())
      f << "target=" << string.cstr() << ",";
    f << "nId1=" << input->readULong(2) << ",";
    f << "nId2=" << input->readULong(2) << ",";
    int nCnt=(int) input->readULong(2);
    bool ok=true;
    f << "libMac=[";
    for (int i=0; i<nCnt; ++i) {
      if (input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a libmac name\n"));
        f << "###libname,";
        ok=false;
        break;
      }
      if (!zone.readString(string)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a string\n"));
        ok=false;
        break;
      }
      else if (!string.empty())
        f << string.cstr() << ":";
      if (!zone.readString(string)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a string\n"));
        ok=false;
        break;
      }
      else if (!string.empty())
        f << string.cstr();
      f << ",";
    }
    f << "],";
    if (!ok) break;
    if (nVers>=1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find string\n"));
        f << "###aName1,";
        break;
      }
      if (!string.empty())
        f << "aName1=" << string.cstr() << ",";
    }
    if (nVers>=2) {
      nCnt=(int) input->readULong(2);
      f << "libMac2=[";
      for (int i=0; i<nCnt; ++i) {
        f << "nCurKey=" << input->readULong(2) << ",";
        if (input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a libmac name\n"));
          f << "###libname2,";
          break;
        }
        if (!zone.readString(string)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a string\n"));
          break;
        }
        else if (!string.empty())
          f << string.cstr() << ":";
        if (!zone.readString(string)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a string\n"));
          break;
        }
        else if (!string.empty())
          f << string.cstr();
        f << "nScriptType=" << input->readULong(2) << ",";
      }
      f << "],";
    }
    break;
  }
  case 0x2001: // ok no data
    f << "textAtrDummy4,";
    break;
  case 0x2002: {
    f << "textAtrRefMark,";
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find aName\n"));
      f << "###aName,";
      break;
    }
    if (!string.empty())
      f << "aName=" << string.cstr() << ",";
    break;
  }
  case 0x2003: {
    f << "textAtrToXMark,";
    int cType=(int) input->readULong(1);
    f << "cType=" << cType << ",";
    f << "nLevel=" << input->readULong(2) << ",";
    librevenge::RVNGString string;
    int nStringId=0xFFFF;
    if (nVers<1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find aTypeName\n"));
        f << "###aTypeName,";
        break;
      }
      if (!string.empty())
        f << "aTypeName=" << string.cstr() << ",";
    }
    else {
      nStringId=(int) input->readULong(2);
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
    }
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find aAltText\n"));
      f << "###aAltText,";
      break;
    }
    if (!string.empty())
      f << "aAltText=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find aPrimKey\n"));
      f << "###aPrimKey,";
      break;
    }
    if (!string.empty())
      f << "aPrimKey=" << string.cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find aSecKey\n"));
      f << "###aSecKey,";
      break;
    }
    if (!string.empty())
      f << "aSecKey=" << string.cstr() << ",";
    if (nVers>=2) {
      cType=(int) input->readULong(1);
      f << "cType=" << cType << ",";
      nStringId=(int) input->readULong(2);
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
      f << "cFlags=" << input->readULong(1) << ",";
    }
    if (nVers>=1 && nStringId!=0xFFFF) {
      if (!zone.getPoolName(nStringId, string)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find a nId name\n"));
        f << "###nId=" << nStringId << ",";
      }
      else
        f << string.cstr() << ",";
    }
    break;
  }
  case 0x2004:
    f << "textAtrCharFmt=" << input->readULong(2) << ",";
    break;
  case 0x2005: // ok no data
    f << "textAtrDummy5,";
    break;
  case 0x2006:
    f << "textAtrCJKRuby=" << input->readULong(1) << ",";
    break;
  case 0x2007:
    f << "textAtrUnknownContainer,";
    // call SfxPoolItem::Create which does nothing
    break;
  case 0x2008: // ok no data
    f << "textAtrDummy6,";
    break;
  case 0x2009: // ok no data
    f << "textAtrDummy7,";
    break;

  // field...
  case 0x3000: {
    f << "textAtrField,";
    SWFieldManager fieldManager;
    fieldManager.readField(zone);
    break;
  }
  case 0x3001: {
    f << "textAtrFlycnt,";
    SWFormatManager formatManager;
    if (input->peek()=='o')
      formatManager.readSWFormatDef(zone,'o', manager);
    else
      formatManager.readSWFormatDef(zone,'l', manager);
    break;
  }
  case 0x3002: {
    f << "textAtrFtn,";
    // SwFmtFtn::Create
    f << "number1=" << input->readULong(1);
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the aNumber\n"));
      f << "###aNumber,";
      break;
    }
    if (!string.empty())
      f << "aNumber=" << string.cstr() << ",";
    break;
  }
  case 0x3003: // ok no data
    f << "textAtrSoftHyph,";
    break;
  case 0x3004: // ok no data
    f << "textAtrHardBlank,";
    break;
  case 0x3005: // ok no data
    f << "textAtrDummy1,";
    break;
  case 0x3006: // ok no data
    f << "textAtrDummy2,";
    break;

  // paragraph attribute
  case 0x4000:
    f << "parAtrLineSpacing,";
    f << "nPropSpace=" << input->readLong(1) << ",";
    f << "nInterSpace=" << input->readLong(2) << ",";
    f << "nHeight=" << input->readULong(2) << ",";
    f << "nRule=" << input->readULong(1) << ",";
    f << "nInterRule=" << input->readULong(1) << ",";
    break;
  case 0x4001:
    f << "parAtrAdjust=" << input->readULong(1) << ",";
    if (nVers>=1) f << "nFlags=" << input->readULong(1) << ",";
    break;
  case 0x4002:
    f << "parAtrSplit=" << input->readULong(1) << ",";
    break;
  case 0x4003:
    f << "parAtrOrphans,";
    f << "nLines="  << input->readLong(1) << ",";
    break;
  case 0x4004:
    f << "parAtrWidows,";
    f << "nLines="  << input->readLong(1) << ",";
    break;
  case 0x4005: {
    f << "parAtrTabStop,";
    int N=(int) input->readULong(1);
    if (input->tell()+7*N>lastPos) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: N is too big\n"));
      f << "###N=" << N << ",";
      N=int(lastPos-input->tell())/7;
    }
    f << "tabs=[";
    for (int i=0; i<N; ++i) {
      int nPos=(int) input->readLong(4);
      f << nPos << "->" << input->readLong(1) << ":" << input->readLong(1) << ":" << input->readLong(1) << ",";
    }
    f << "],";
    break;
  }
  case 0x4006:
    f << "parAtrHyphenZone=" << input->readLong(1) << ",";
    f << "bHyphenPageEnd=" << input->readLong(1) << ",";
    f << "nMinLead=" << input->readLong(1) << ",";
    f << "nMinTail=" << input->readLong(1) << ",";
    f << "nMaxHyphen=" << input->readLong(1) << ",";
    break;
  case 0x4007:
    f << "parAtrDrop,";
    f << "nFmt=" << input->readULong(2) << ",";
    f << "nLines1=" << input->readULong(2) << ",";
    f << "nChars1=" << input->readULong(2) << ",";
    f << "nDistance1=" << input->readULong(2) << ",";
    if (nVers>=1)
      f << "bWhole=" << input->readULong(1) << ",";
    else {
      f << "nX=" << input->readULong(2) << ",";
      f << "nY=" << input->readULong(2) << ",";
    }
    break;
  case 0x4008:
    f << "parAtrRegister=" << input->readULong(1) << ",";
    break;
  case 0x4009: {
    f << "parAtrNumRule,";
    librevenge::RVNGString string;
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the sTmp\n"));
      f << "###sTmp,";
      break;
    }
    if (!string.empty())
      f << "sTmp=" << string.cstr() << ",";
    if (nVers>0)
      f << "nPoolId=" << input->readULong(2) << ",";
    break;
  }
  case 0x400a:
    f << "parAtrScriptSpace=" << input->readULong(1) << ",";
    break;
  case 0x400b:
    f << "parAtrHangingPunctuation=" << input->readULong(1) << ",";
    break;
  case 0x400c:
    f << "parAtrForbiddenRules=" << input->readULong(1) << ",";
    break;
  case 0x400d:
    f << "parAtrVertAlign=" << input->readULong(2) << ",";
    break;
  case 0x400e:
    f << "parAtrSnapToGrid=" << input->readULong(1) << ",";
    break;
  case 0x400f:
    f << "parAtrConnectBorder=" << input->readULong(1) << ",";
    break;
  case 0x4010: // no data
    f << "parAtrDummy5,";
    break;
  case 0x4011: // no data
    f << "parAtrDummy6,";
    break;
  case 0x4012: // no data
    f << "parAtrDummy7,";
    break;
  case 0x4013: // no data
    f << "parAtrDummy8,";
    break;

  // frame parameter
  case 0x5000:
    f << "fillOrder=" << input->readULong(1) << ",";
    break;
  case 0x5001:
    f << "frmSize,";
    f << "sizeType=" << input->readULong(1) << ",";
    f << "width=" << input->readULong(4) << ",";
    f << "height=" << input->readULong(4) << ",";
    if (nVers>1)
      f << "percent=" << input->readULong(1) << "x"  << input->readULong(1) << ",";
    break;
  case 0x5002:
    f << "paperBin=" << input->readULong(1) << ",";
    break;
  case 0x5003:
    f << "lrSpace,";
    f << "left=" << input->readULong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "right=" << input->readULong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "firstLine=" << input->readLong(2);
    val=(int) input->readULong(nVers>=1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    if (nVers>=2)
      f << "textLeft=" << input->readLong(2) << ",";
    if (nVers>=3) {
      long marker=(long) input->readULong(4);
      if (marker==0x599401FE)
        f << "firstLine[bullet]=" << input->readULong(2);
      else if (input->tell()==lastPos+3) // normally end by 0
        input->seek(-3, librevenge::RVNG_SEEK_CUR);
      else
        input->seek(-4, librevenge::RVNG_SEEK_CUR);
    }
    break;
  case 0x5004:
    f << "ulSpace,";
    f << "upper=" << input->readULong(2);
    val=(int) input->readULong(nVers==1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    f << "lower=" << input->readULong(2);
    val=(int) input->readULong(nVers==1 ? 2 : 1);
    if (val) f << ":" << val;
    f << ",";
    break;
  case 0x5005:
    f << "pageDesc,";
    if (nVers<1)
      f << "bAutor=" << input->readULong(1) << ",";
    if (nVers< 2)
      f << "nOff=" << input->readULong(2) << ",";
    else {
      unsigned long nOff;
      if (!input->readCompressedULong(nOff)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read nOff\n"));
        f << "###nOff,";
        break;
      }
      f << "nOff=" << nOff << ",";
    }
    f << "nIdx=" << input->readULong(2) << ",";
    break;
  case 0x5006:
    f << "pageBreak=" << input->readULong(1) << ",";
    if (nVers<1) input->seek(1, librevenge::RVNG_SEEK_CUR); // dummy
    break;
  case 0x5007: // checkme
    f << "pageCntnt,";
    while (input->tell()<lastPos) {
      long actPos=input->tell();
      if (input->peek()!='N' || !manager.readSWContent(zone) || input->tell()<=actPos) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: find unknown pageCntnt child\n"));
        f << "###child";
        break;
      }
    }
    break;
  case 0x5008:
  case 0x5009: {
    f << (nWhich==0x5008 ? "header" : "footer") << ",";
    f << "active=" << input->readULong(1) << ",";
    long actPos=input->tell();
    if (actPos==lastPos)
      break;
    SWFormatManager formatManager;
    formatManager.readSWFormatDef(zone,'r',manager);
    break;
  }
  case 0x500a:
    f << "print=" << input->readULong(1) << ",";
    break;
  case 0x500b:
    f << "opaque=" << input->readULong(1) << ",";
    break;
  case 0x500c:
    f << "protect,";
    val=(int) input->readULong(1);
    if (val&1) f << "pos[protect],";
    if (val&2) f << "size[protect],";
    if (val&4) f << "cntnt[protect],";
    break;
  case 0x500d:
    f << "surround=" << input->readULong(1) << ",";
    if (nVers<5) f << "bGold=" << input->readULong(1) << ",";
    if (nVers>1) f << "bAnch=" << input->readULong(1) << ",";
    if (nVers>2) f << "bCont=" << input->readULong(1) << ",";
    if (nVers>3) f << "bOutside1=" << input->readULong(1) << ",";
    break;
  case 0x500e:
  case 0x500f:
    f << (nWhich==0x500e ? "vertOrient" : "horiOrient") << ",";
    f << "nPos=" << input->readULong(4) << ",";
    f << "nOrient=" << input->readULong(1) << ",";
    f << "nRel=" << input->readULong(1) << ",";
    if (nWhich==0x500f && nVers>=1) f << "bToggle=" << input->readULong(1) << ",";
    break;
  case 0x5010:
    f << "anchor=" << input->readULong(1) << ",";
    if (nVers<1)
      f << "nId=" << input->readULong(2) << ",";
    else {
      unsigned long nId;
      if (!input->readCompressedULong(nId)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read nId\n"));
        f << "###nId,";
        break;
      }
      else
        f << "nId=" << nId << ",";
    }
    break;
  // 5011 see case 0x1014
  case 0x5012: {
    f << "box,";
    f << "nDist=" << input->readULong(2) << ",";
    int cLine=0;
    bool ok=true;
    while (input->tell()<lastPos) {
      cLine=(int) input->readULong(1);
      if (cLine>3) break;
      f << "[";
      STOFFColor color;
      if (!input->readColor(color)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find a box's color\n"));
        f << "###color,";
        ok=false;
        break;
      }
      else if (!color.isBlack())
        f << "col=" << color << ",";
      f << "outline=" << input->readULong(2) << ",";
      f << "inline=" << input->readULong(2) << ",";
      f << "nDist=" << input->readULong(2) << ",";
      f << "],";
    }
    if (!ok) break;
    if (nVers>=1 && cLine&0x10) {
      f << "dist=[";
      for (int i=0; i<4; ++i) f << input->readULong(2) << ",";
      f << "],";
    }
    break;
  }
  case 0x5013: {
    f << "shadow,";
    f << "cLoc=" << input->readULong(1) << ",";
    f << "nWidth=" << input->readULong(2) << ",";
    f << "bTrans=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read color\n"));
      f << "###color,";
      break;
    }
    else if (!color.isWhite())
      f << "color=" << color << ",";
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read fill color\n"));
      f << "###fillcolor,";
      break;
    }
    else if (!color.isWhite())
      f << "fillcolor=" << color << ",";
    f << "style=" << input->readULong(1) << ",";
    break;
  }
  case 0x5014: // svt_macitem.cxx
    f << "frmMacro,";
    STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: read a macros is not implemented\n"));
    f << "###unimplemented,";
    break;
  case 0x5015: {
    f << "col,";
    f << "nLineAdj=" << input->readULong(1) << ",";
    f << "bOrtho=" << input->readULong(1) << ",";
    f << "nLineHeight=" << input->readULong(1) << ",";
    f << "nGutterWidth=" << input->readLong(2) << ",";
    int nWishWidth=(int) input->readULong(2);
    f << "nWishWidth=" << nWishWidth << ",";
    f << "nPenStyle=" << input->readULong(1) << ",";
    f << "nPenWidth=" << input->readLong(2) << ",";
    f << "nPenRed=" << (input->readULong(2)>>8) << ",";
    f << "nPenGreen=" << (input->readULong(2)>>8) << ",";
    f << "nPenBlue=" << (input->readULong(2)>>8) << ",";
    int nCol=(int) input->readULong(2);
    f << "N=" << nCol << ",";
    if (nWishWidth==0)
      break;
    if (input->tell()+10*nCol>lastPos) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: nCol is bad\n"));
      f << "###N,";
      break;
    }
    f << "[";
    for (int i=0; i<nCol; ++i) {
      f << "[";
      f << "nWishWidth=" << input->readULong(2) << ",";
      f << "nLeft=" << input->readULong(2) << ",";
      f << "nUpper=" << input->readULong(2) << ",";
      f << "nRight=" << input->readULong(2) << ",";
      f << "nBottom=" << input->readULong(2) << ",";
      f << "],";
    }
    f << "],";
    break;
  }
  case 0x5016:
    f << "keep=" << input->readULong(1) << ",";
    break;
  case 0x5017:
    f << "url,";
    if (!manager.readSWImageMap(zone))
      break;
    if (nVers>=1) {
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the setName\n"));
        f << "###name1,";
        break;
      }
      else if (!text.empty())
        f << "name1=" << text.cstr() << ",";
    }
    break;
  case 0x5018:
    f << "editInReadOnly=" << input->readULong(1) << ",";
    break;
  case 0x5019:
    f << "layoutSplit=" << input->readULong(1) << ",";
    break;
  case 0x501a:
    f << "chain,";
    if (nVers>0) {
      f << "prevIdx=" << input->readULong(2) << ",";
      f << "nextIdx=" << input->readULong(2) << ",";
    }
    break;
  case 0x501b:
    f << "textgrid=" << input->readULong(1) << ",";
    break;
  case 0x501c:
    f << "lineNumber,";
    f << "start=" << input->readULong(4) << ",";
    f << "count[lines]=" << input->readULong(1) << ",";
    break;
  case 0x501d:
  case 0x501e:
    f << (nWhich==0x501d ? "ftnAtTextEnd" : "ednAtTextEnd") << "=" << input->readULong(1) << ",";
    if (nVers>0) {
      f << "offset=" << input->readULong(2) << ",";
      f << "fmtType=" << input->readULong(2) << ",";
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the prefix\n"));
        f << "###prefix,";
        break;
      }
      else if (!text.empty())
        f << "prefix=" << text.cstr() << ",";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the suffix\n"));
        f << "###suffix,";
        break;
      }
      else if (!text.empty())
        f << "suffix=" << text.cstr() << ",";
    }
    break;
  case 0x501f:
    f << "columnBalance=" << input->readULong(1) << ",";
    break;
  case 0x5020:
    f << "frameDir=" << input->readULong(2) << ",";
    break;
  case 0x5021:
    f << "headerFooterEatSpacing=" << input->readULong(1) << ",";
    break;
  case 0x5022: // ok, no data
    f << "frmAtrDummy9,";
    break;
  // graphic attribute
  case 0x6000:
    f << "grfMirrorGrf=" << input->readULong(1) << ",";
    if (nVers>0) f << "nToggle=" << input->readULong(1) << ",";
    break;
  case 0x6001:
    f << "grfCropGrf,";
    f << "top=" << input->readLong(4) << ",";
    f << "left=" << input->readLong(4) << ",";
    f << "right=" << input->readLong(4) << ",";
    f << "bottom=" << input->readLong(4) << ",";
    break;
  // no saved ?
  case 0x6002:
    f << "grfRotation,";
    break;
  case 0x6003:
    f << "grfLuminance,";
    break;
  case 0x6004:
    f << "grfContrast,";
    break;
  case 0x6005:
    f << "grfChannelR,";
    break;
  case 0x6006:
    f << "grfChannelG,";
    break;
  case 0x6007:
    f << "grfChannelB,";
    break;
  case 0x6008:
    f << "grfGamma,";
    break;
  case 0x6009:
    f << "grfInvert,";
    break;
  case 0x600a:
    f << "grfTransparency,";
    break;
  case 0x600b:
    f << "grfDrawMode,";
    break;
  case 0x600c: // ok no data
    f << "grfDummy1,";
    break;
  case 0x600d: // ok no data
    f << "grfDummy2,";
    break;
  case 0x600e: // ok no data
    f << "grfDummy3,";
    break;
  case 0x600f: // ok no data
    f << "grfDummy4,";
    break;
  case 0x6010: // ok no data
    f << "grfDummy5,";
    break;

  case 0x6011:
    f << "boxFormat=" << input->readULong(1) << ",";
    f << "nTmp=" << input->readULong(4) << ",";
    break;
  case 0x6012: {
    f << "boxFormula,";
    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the formula\n"));
      f << "###formula,";
      break;
    }
    else if (!text.empty())
      f << "formula=" << text.cstr() << ",";
    break;
  }
  case 0x6013:
    f << "boxAtrValue,";
    if (nVers==0) {
      librevenge::RVNGString text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not find the dValue\n"));
        f << "###dValue,";
        break;
      }
      else if (!text.empty())
        f << "dValue=" << text.cstr() << ",";
    }
    else {
      double res;
      bool isNan;
      if (!input->readDoubleReverted8(res, isNan)) {
        STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: can not read a double\n"));
        f << "##value,";
      }
      else if (res<0||res>0)
        f << "value=" << res << ",";
    }
    break;

  default:
    STOFF_DEBUG_MSG(("SWAttributeManager::readAttribute: reading not format attribute is not implemented\n"));
    f << "#unimplemented,";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('A', "SWAttributeDef");
  return true;

}

bool SWAttributeManager::readAttributeList(SWZone &zone, SDWParser &manager)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='S' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWAttributeList)[" << zone.getRecordLevel() << "]:";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell() < zone.getRecordLastPosition()) { // normally only 2
    pos=input->tell();
    if (readAttribute(zone, manager) && input->tell()>pos) continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }
  zone.closeRecord('S', "SWAttributeList");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
