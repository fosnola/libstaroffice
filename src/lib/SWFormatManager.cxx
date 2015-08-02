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

#include "SWZone.hxx"

#include "SWAttributeManager.hxx"
#include "SWFormatManager.hxx"

/** Internal: the structures of a SWFormatManager */
namespace SWFormatManagerInternal
{
////////////////////////////////////////
//! Internal: the state of a SWFormatManager
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
SWFormatManager::SWFormatManager() : m_state(new SWFormatManagerInternal::State)
{
}

SWFormatManager::~SWFormatManager()
{
}

bool SWFormatManager::readSWFormatDef(SWZone &zone, char kind, SDWParser &manager)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!=kind || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  // sw_sw3fmts.cxx InFormat
  libstoff::DebugStream f;
  f << "Entries(SWFormatDef)[" << zone.getRecordLevel() << "]:";
  int flags=zone.openFlagZone();
  f << "nDerived=" << input->readULong(2) << ",";
  f << "nPoolId=" << input->readULong(2) << ",";
  int stringId=0xFFFF;
  if (flags&0x10) {
    stringId=(int) input->readULong(2);
    if (stringId!=0xffff)
      f << "nStringId=" << stringId << ",";
  }
  if (flags&0x20)
    f << "nObjRef=" << input->readULong(4) << ",";
  int moreFlags=0;
  if (flags&(zone.isCompatibleWith(0x201) ? 0x80:0x40)) {
    moreFlags=(int) input->readULong(1);
    f << "flags[more]=" << std::hex << moreFlags << std::dec << ",";
  }
  zone.closeFlagZone();

  bool readName;
  if (zone.isCompatibleWith(0x201))
    readName=(moreFlags&0x20);
  else
    readName=(stringId==0xffff);
  librevenge::RVNGString string;
  if (readName) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWFormatDef: can not read the name\n"));
      f << "###name";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeRecord(kind);
      return true;
    }
    else if (!string.empty())
      f << string.cstr() << ",";
  }
  else if (stringId!=0xffff) {
    if (!zone.getPoolName(stringId, string))
      f << "###nPoolId=" << stringId << ",";
    else if (!string.empty())
      f << string.cstr() << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  SWAttributeManager attributeManager;
  while (input->tell()<lastPos) {
    pos=(int) input->tell();
    int rType=input->peek();
    if (rType=='S' && attributeManager.readAttributeList(zone, manager))
      continue;

    input->seek(pos, librevenge::RVNG_SEEK_SET);
    if (!zone.openRecord(type)) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWFormatDef: find extra data\n"));
      ascFile.addPos(pos);
      ascFile.addNote("SWFormatDef:###");
      break;
    }
    f.str("");
    f << "SWFormatDef[" << type << "-" << zone.getRecordLevel() << "]:";
    STOFF_DEBUG_MSG(("SWFormatManager::readSwFormatDef: find unexpected type\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
  }

  zone.closeRecord(kind);
  return true;
}

bool SWFormatManager::readSWNumberFormat(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='n' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWNumbFormat)[" << zone.getRecordLevel() << "]:";
  // sw_sw3num.cxx Sw3IoImp::InNumFmt
  librevenge::RVNGString string;
  for (int i=0; i<4; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormat: can not read a string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      zone.closeRecord('n');
      return true;
    }
    if (string.empty()) continue;
    static char const *(wh[])= {"prefix", "postfix", "fontname", "fontstyle"};
    f << wh[i] << "=" << string.cstr() << ",";
  }
  f << "nFmt=" << input->readULong(2) << ",";
  f << "eType=" << input->readULong(1) << ",";
  f << "cBullet=" << input->readULong(1) << ",";
  f << "nUpperLevel=" << input->readULong(1) << ","; // int or bool
  f << "nStart=" << input->readULong(2) << ",";
  f << "eNumAdjust=" << input->readULong(1) << ",";
  f << "nLSpace=" << input->readLong(4) << ",";
  f << "nFirstLineOffset=" << input->readLong(4) << ",";
  f << "eFamily=" << input->readULong(1) << ",";
  f << "ePitch=" << input->readULong(1) << ",";
  f << "eCharSet=" << input->readULong(1) << ",";
  int cFlags=zone.openFlagZone();
  zone.closeFlagZone();
  if (cFlags&0xF0) f << "cFlags=" << (cFlags>>4) << ",";
  if (zone.isCompatibleWith(0x17, 0x22))
    f << "nFmt=" << input->readULong(2) << ",";
  if (zone.isCompatibleWith(0x17,0x22, 0x101,0x201)) {
    f << "bRelSpace=" << input->readULong(1) << ",";
    f << "nRelLSpace=" << input->readLong(4) << ",";
  }
  if (zone.isCompatibleWith(0x17,0x22, 0x101)) {
    f << "nTextOffset=" << input->readULong(2) << ",";
    if (input->tell()<zone.getRecordLastPosition()) {
      f << "width=" << input->readLong(4) << ",";
      f << "height=" << input->readLong(4) << ",";
      int cF=(int) input->readULong(1);
      if (cF&1) f << "nVer[brush]=" << input->readULong(2) << ",";
      if (cF&2) f << "nVer[vertOrient]=" << input->readULong(2) << ",";
    }
  }
  if (input->tell()!=zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SWFormatManager::readSwNumberFormat: find extra data\n"));
    f << "###extra:";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord('n');
  return true;
}

bool SWFormatManager::readSWNumberFormatterList(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='q' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWNumberFormatter)[" << zone.getRecordLevel() << "]:";
  long dataSz=(int) input->readULong(4);
  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+dataSz+6>lastPos) {
    STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: data size seems bad\n"));
    f << "###dataSz";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord(type);
    return true;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  // svt_numhead::ImpSvNumMultipleReadHeader
  long actPos=input->tell();
  long endDataPos=input->tell()+dataSz;
  f.str("");
  f << "SWNumberFormatter-T:";
  input->seek(endDataPos, librevenge::RVNG_SEEK_SET);
  int val=(int) input->readULong(2);
  if (val) f << "nId=" << val << ",";
  long nSizeTableLen=(long) input->readULong(4);
  f << "nTableSize=" << nSizeTableLen << ","; // copy in pMemStream
  std::vector<long> fieldSize;
  if (input->tell()+nSizeTableLen>lastPos) {
    STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: the table length seems bad\n"));
    f << "###";
    nSizeTableLen=lastPos-input->tell();
  }
  f << "listSize=[";
  for (long i=0; i<nSizeTableLen/4; ++i) {
    fieldSize.push_back((long) input->readULong(4));
    f << std::hex << fieldSize.back() << std::dec << ",";
  }
  f << "],";
  if (nSizeTableLen) ascFile.addDelimiter(input->tell(),'|');
  ascFile.addPos(endDataPos);
  ascFile.addNote(f.str().c_str());

  // svt_zforList::Load
  input->seek(actPos, librevenge::RVNG_SEEK_SET);
  f.str("");
  f << "SWNumberFormatter:";
  int nVers=(int) input->readULong(2);
  f << "vers=" << nVers << ",";
  val=(int) input->readULong(2);
  if (val) f << "nSysOnStore=" << std::hex << val << std::dec << ",";
  val=(int) input->readULong(2);
  if (val) f << "eLge=" << val << ",";
  ascFile.addPos(actPos);
  ascFile.addNote(f.str().c_str());

  unsigned long nPos=(unsigned long) input->readULong(4);
  size_t n=0;
  while (nPos!=0xffffffff) {
    pos=input->tell();
    if (pos==endDataPos) break;
    if (pos>endDataPos) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: data size seems bad\n"));
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }

    f.str("");
    f << "SWNumberFormatter-A" << n << ":";

    input->seek(2, librevenge::RVNG_SEEK_CUR);
    val=(int) input->readULong(2);
    if (val) f << "eLge=" << val << ",";

    if (n>=fieldSize.size()||input->tell()+fieldSize[n]>endDataPos) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: can not find end of field\n"));
      f << "###unknownN";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    long endFieldPos=input->tell()+fieldSize[n++];

    librevenge::RVNGString text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: can not read the format string\n"));
      f << "###string";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      break;
    }
    f << text.cstr() << ",";
    val=(int) input->readULong(2);
    if (val) f << "eType=" << val << ",";
    for (int i=0; i<2; ++i) { // checkme
      double res;
      bool isNan;
      if (!input->readDoubleReverted8(res, isNan)) {
        STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: can not read a double\n"));
        f << "##limit" << i << ",";
      }
      else if (res<0||res>0)
        f << "limit" << i << "=" << res << ",";
    }
    for (int i=0; i<2; ++i) {
      val=(int) input->readULong(2);
      if (val) f << "nOp" << i << "=" << val << ",";
    }
    val=(int) input->readULong(1);
    if (val) f << "bStandart=" << val << ",";
    val=(int) input->readULong(1);
    if (val) f << "bIsUsed=" << val << ",";

    bool ok=true;
    for (int i=0; i<4; ++i) {
      // ImpSvNumFor::Load
      f << "numFor" << i << "=[";
      int N=(int) input->readULong(2);
      if (input->tell()+4*N>endFieldPos) break;
      for (int c=0; c<N; ++c) {
        if (!zone.readString(text)) {
          STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: can not read the format string\n"));
          f << "###SvNumFor" << c;
          ok=false;
          break;
        }
        f << text.cstr() << ":" << input->readULong(2) << ",";
      }
      if (!ok) break;
      val=(int) input->readLong(2);
      if (val) f << "eScannedType=" << val << ",";
      val=(int) input->readULong(1);
      if (val) f << "bThousand=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nThousand=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nCntPre=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nCntPost=" << val << ",";
      val=(int) input->readULong(2);
      if (val) f << "nCntExp=" << val << ",";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: can not read the color name\n"));
        f << "###colorname";
        ok=false;
        break;
      }
      if (!text.empty())
        f << text.cstr() << ",";
      f << "],";
    }
    if (input->tell()>endFieldPos) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: we read too much\n"));
      f << "###pos";
      ok=false;
    }
    if (ok && input->tell()!=endFieldPos && !zone.readString(text)) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormatterList: can not read the comment\n"));
      f << "###comment";
      ok=false;
    }
    else {
      if (!text.empty())
        f << "comment=" << text.cstr() << ",";
    }

    if (ok && input->tell()!=endFieldPos) {
      val=(int) input->readULong(2);
      if (val) f << "nNewStandardDefined=" << val << ",";
    }

    if (input->tell()!=endFieldPos) {
      // now there can still be a list of currency version....
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("SWFormatManager::readSWNumberFormat: find extra data\n"));
        first=false;
      }
      ascFile.addDelimiter(input->tell(),'|');
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    if (endFieldPos==endDataPos) break;
    input->seek(endFieldPos, librevenge::RVNG_SEEK_SET);
    nPos=(unsigned long) input->readULong(4);
  }
  zone.closeRecord(type);
  return true;
}

bool SWFormatManager::readSWFlyFrameList(SWZone &zone, SDWParser &manager)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='F' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWFlyFrames)[" << zone.getRecordLevel() << "]:";
  // sw_sw3fmts.cxx sub part of Sw3IoImp::InFlyFrames
  long lastPos=zone.getRecordLastPosition();
  while (input->tell()<lastPos) {
    pos=input->tell();
    int rType=input->peek();
    if ((rType=='o' || rType=='l') && readSWFormatDef(zone, char(rType), manager))
      continue;
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    break;
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SWFormatManager::readSWPatternLCL: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWFlyFrames:###extra");
  }
  zone.closeRecord('F');
  return true;
}

bool SWFormatManager::readSWPatternLCL(SWZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (input->peek()!='P' || !zone.openRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "Entries(SWPatternLCL)[" << zone.getRecordLevel() << "]:";
  // sw_sw3misc.cxx sub part of Sw3IoImp::InTOXs
  long lastPos=zone.getRecordLastPosition();
  zone.openFlagZone();
  f << "nLevel=" << input->readULong(1) << ",";
  f << "nTokens=" << input->readULong(2) << ",";
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  librevenge::RVNGString string;
  while (input->tell()<lastPos) {
    pos=input->tell();
    if (input->peek()!='D' || !zone.openRecord(type)) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      break;
    }
    f.str("");
    f << "SWPatternLCL[token-" << zone.getRecordLevel() << "]:";
    zone.openFlagZone();
    int nType=(int) input->readULong(1);
    f << "nType=" << nType << ",";
    f << "nStrIdx=" << input->readULong(2) << ",";
    zone.closeFlagZone();
    switch (nType) {
    case 0:
      f << "tknEntryNo,";
      break;
    case 1:
      f << "tknEntryText,";
      break;
    case 3:
      f << "tknEntryTabStop,";
      f << "nTabStopPos=" << input->readLong(4) << ",";
      f << "nTabAlign=" << input->readULong(2) << ",";
      if (zone.isCompatibleWith(0x217))
        f << "fillChar=" << input->readULong(1) << ",";
      break;
    case 2:
      if (input->tell()==zone.getRecordLastPosition()) {
        f << "tknEntry,";
        break;
      }
    // #92986# some older versions created a wrong content index entry definition
    // fall through expected
    case 4: {
      f << "tknText,";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("SWFormatManager::readSWPatternLCL: can not read a string\n"));
        f << "###type,";
        break;
      }
      f << string.cstr() << ",";
      break;
    }
    case 5:
      f << "tknPageNums,";
      break;
    case 6:
      f << "tknChapterInfo,";
      f << "format=" << input->readULong(1);
      break;
    case 7:
      f << "tknLinkStart,";
      break;
    case 8:
      f << "tknLinkEnd,";
      break;
    case 9:
      f << "tknAutority,";
      f << "field=" << input->readULong(2) << ",";
      break;
    case 10: // end of token
      break;
    default:
      STOFF_DEBUG_MSG(("SWFormatManager::readSWPatternLCL: find unknown token\n"));
      f << "###type,";
      break;
    }
    if (input->tell()!=zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("SWFormatManager::readSWPatternLCL: find extra data in token\n"));
      f << "###extra";
    }
    zone.closeRecord('D');
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("SWFormatManager::readSWPatternLCL: find extra data\n"));
    ascFile.addPos(pos);
    ascFile.addNote("SWPatternLCL:###extra");
  }
  zone.closeRecord('P');
  return true;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
