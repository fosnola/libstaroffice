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

#include <string>

#include <librevenge/librevenge.h>

#include "StarLayout.hxx"

#include "STOFFInputStream.hxx"

#include "StarFormatManager.hxx"
#include "StarObject.hxx"
#include "StarZone.hxx"

int StarLayout::readNumber(STOFFInputStreamPtr input, int vers) const
{
  if (m_version<vers)
    return int(input->readULong(2));
  auto N=int(input->readULong(1));
  if (N) return N;
  return int(input->readULong(2));
}

bool StarLayout::readDataBlock(StarZone &zone, libstoff::DebugStream &f) const
{
  STOFFInputStreamPtr input=zone.input();
  auto type2=int(input->readULong(1));
  f << "type=" << std::hex << type2 << std::dec << ",";
  if (type2&0x40)
    f << "da40=" << std::hex << input->readULong(2) << std::dec << ",";
  if (type2&0x20)
    f << "da20=" << std::hex << input->readULong(2) << "x" << input->readULong(1) << std::dec << ",";
  if (type2&0x10)
    f << "da10=" << std::hex << input->readULong(2) << std::dec << ",";
  if (type2&4)
    f << "da4=" << std::hex << input->readULong(2) << std::dec << ",";
  if (type2&2)
    f << "da2=" << std::hex << input->readULong(2) << "x" << input->readULong(1) << std::dec << ",";
  if (type2&1)
    f << "da1=" << std::hex << input->readULong(2) << std::dec << ",";
  type2&=0x88;
  bool ok=type2==0 && input->tell()<=zone.getRecordLastPosition();
  if (!ok) {
    STOFF_DEBUG_MSG(("StarLayout::readDataBlock: can not read a data block\n"));
    f << "###";
  }
  return ok;
}

bool StarLayout::read(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  if (input->peek()!='U' || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarLayout::read: can not read a layout\n"));
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(StarLayout)[" << zone.getRecordLevel() << "]:";
  int fl=zone.openFlagZone();
  if (fl&0xf0) f << "fl=" << (fl>>4) << ",";
  if (zone.getFlagLastPosition()-input->tell()==2) {
    m_version=uint16_t(0x200+input->readULong(1));
    f << "vers=" << std::hex << m_version << std::dec << ",";
    f << "f1=" << input->readULong(1) << ",";
  }
  else {
    *input>>m_version;
    f << "vers=" << std::hex << m_version << std::dec << ",";
    f << "f1=" << input->readULong(2) << ",";
  }
  zone.closeFlagZone();
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  while (input->tell()<lastPos && readChild(zone, object))
    ; // find child d2 and d7

  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarLayout::read: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout:###");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord('U', "StarLayout");
  return true;
}


bool StarLayout::readC1(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  int mainType=input->peek();
  if ((mainType!=0xc1 && mainType!=0xcc && mainType!=0xcd) || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarLayout::readC1: can not read a layout\n"));
    return false;
  }
  std::string what(mainType==0xc1 ? "C1" : mainType==0xcc ? "CC" : "CD");
  long lastPos=zone.getRecordLastPosition();
  f << "StarLayout[" << what << "-" << zone.getRecordLevel() << "]:";
  int type1;
  bool ok=readHeader(zone, f, type1);
  int const dSize=mainType==0xcd ? 0 : 9;
  if (!ok || input->tell()+dSize+1>lastPos) {
    if (!ok) {
      STOFF_DEBUG_MSG(("StarLayout::readC1: the zone seems too short\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarLayout");
    return true;
  }
  if (dSize) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(dSize,librevenge::RVNG_SEEK_CUR);
    ascFile.addDelimiter(input->tell(),'|');
  }
  if (m_version<0x200) {
    f << "block2=[";
    ok=readDataBlock(zone, f);
    f << "],";
    if (type1&0x40) {
      f << "block3=[";
      ok=readDataBlock(zone, f);
      f << "],";
    }
  }
  unsigned long N=0;
  if (ok) {
    if (m_version>=0x200) {
      if (!input->readCompressedULong(N)) {
        f << "###N";
        ok=false;
      }
      else
        f << "N=" << std::hex << N << std::dec << ",";
    }
    else
      N=input->readULong(2);
  }
  if (ok && N!=0)
    f << "N=" << N << ",";
  else
    N=0;
  for (int i=0; i<int(N); ++i) {
    if (!readChild(zone, object))
      break;
    // find type c7|cd|d4|d8
  }

  if (input->tell()!=lastPos) {
    if (!ok) {
      STOFF_DEBUG_MSG(("StarLayout::readC1: find extra data\n"));
    }
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSWRecord(type, "StarLayout");
  return true;
}

bool StarLayout::readC2(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  int mainType=input->peek();
  if ((mainType!=0xc2 && mainType!=0xc3 && mainType!=0xc6 && mainType!=0xc8 && mainType!=0xc9 && mainType!=0xce &&
       mainType!=0xd2 && mainType!=0xd3 && mainType!=0xd4 && mainType!=0xd7 && mainType!=0xe3 && mainType!=0xf2)
      || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarLayout::readC2: can not read a layout\n"));
    return false;
  }
  std::string what(mainType==0xc2 ? "C2" : mainType==0xc3 ? "C3" : mainType==0xc6 ? "C6" :
                   mainType==0xc8 ? "C8" : mainType==0xc9 ? "C9" : mainType==0xce ? "CE" :
                   mainType==0xd2 ? "D2" : mainType==0xd3 ? "D3" : mainType==0xd4 ? "D4" :
                   mainType==0xd7 ? "D7" : mainType==0xe3 ? "E3" : "F2");
  f << "StarLayout[" << what << "-" << zone.getRecordLevel() << "]:vers=" << m_version << ",";
  long lastPos=zone.getRecordLastPosition();
  int type1;
  bool ok=readHeader(zone, f, type1, (mainType<=0xc3||mainType==0xce || mainType>=0xd2) ? 2 : mainType==0xc9 ? 0 : 1);
  if (!ok || input->tell()+(mainType==0xc9 ? 21 : mainType==0xce ? 4 : mainType==0xd4 ? 3 : mainType==0xd2 || mainType==0xd7 ? 2 : 0)+1>lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarLayout::readC2: the zone seems too short\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarLayout");
    return true;
  }
  if (mainType==0xc9) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(12, librevenge::RVNG_SEEK_CUR);
    ascFile.addDelimiter(input->tell(),'|');
    auto val=int(input->readULong(m_version>=0x200 ? 1 : 2)); // checkme: probably read long compressed
    if (val) f << "g0=" << val << ",";
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(8, librevenge::RVNG_SEEK_CUR);
    ascFile.addDelimiter(input->tell(),'|');
  }
  else if (mainType==0xce) {
    auto val=int(input->readULong(2));
    if (val) f << "g0=" << val << ",";
    val=int(input->readULong(2));
    if (val) f << "g1=" << val << ",";
  }
  else if (mainType==0xd4) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(input->tell()+2, librevenge::RVNG_SEEK_SET);
    ascFile.addDelimiter(input->tell(),'|');
    auto N0=int(input->readULong(m_version<0x200 ? 2 : 1)); // checkme
    if (N0) f << "N0=" << N0 << ",";
  }
  else if (mainType==0xd2 || mainType==0xd7) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(input->tell()+1, librevenge::RVNG_SEEK_SET);
    ascFile.addDelimiter(input->tell(),'|');
    int N0=readNumber(input,0x200);
    if (N0) f << "N0=" << N0 << ",";
  }
  int N=readNumber(input, 0x200);
  f << "N=" << N << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int i=0; i<N; ++i) {
    if (!readChild(zone, object))
      break;
    // find cd|ce|d3|d4|d8
  }

  pos=input->tell();
  if (mainType==0xc9 && pos+4<=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout[C9-B]:");
    input->seek(4, librevenge::RVNG_SEEK_CUR);
  }
  else if ((mainType==0xd2 || mainType==0xd7) && m_version>=0x101) {
    if (pos!=lastPos && input->peek()=='l') {
      std::shared_ptr<StarFormatManagerInternal::FormatDef> format;
      if (!object.getFormatManager()->readSWFormatDef(zone,'l', format, object))
        input->seek(pos, librevenge::RVNG_SEEK_SET);
    }
    else if (pos!=lastPos && mainType==0xd7 && pos+4<=lastPos) {
      ascFile.addPos(pos);
      ascFile.addNote("StarLayout[D7-B]:");
      input->seek(4, librevenge::RVNG_SEEK_CUR);
    }
  }
  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarLayout::readC2: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout:###");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord(type, "StarLayout");
  return true;
}

bool StarLayout::readC4(StarZone &zone, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  int mainType=input->peek();
  if ((mainType!=0xc4 && mainType!=0xc7) || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarLayout::readC4: can not read a layout\n"));
    return false;
  }
  std::string what(mainType==0xc4 ? "C4" : "C7");
  f << "StarLayout[" << what << "-" << zone.getRecordLevel() << "]:";
  long lastPos=zone.getRecordLastPosition();
  int type1;
  bool ok=readHeader(zone, f, type1, 0);
  if (!ok || input->tell()+1>lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarLayout::readC4: the zone seems too short\n"));
      f << "###";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(type, "StarLayout");
    return true;
  }
  int val=mainType==0xc4 ? int(input->readULong(2)) : readNumber(input, 0x200); // c7: main two bytes if version<0x200
  if (val) f << "g0=" << val << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarLayout::readC4: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout:###");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord(type, "StarLayout");
  return true;
}

bool StarLayout::readD0(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  if (input->peek()!=0xd0 || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarLayout::readD0: can not read a layout\n"));
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  f << "StarLayout[D0-" << zone.getRecordLevel() << "]:vers=" << m_version << ",";
  int type1;
  bool ok=readHeader(zone, f, type1);
  bool hasN=(type1&0xf0); // unsure
  if (!ok || input->tell()+4+(hasN ? 1 : 0)>lastPos) {
    STOFF_DEBUG_MSG(("StarLayout::readD0: the zone seems too short\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeSWRecord(static_cast<unsigned char>(0xd0), "StarLayout");
    return true;
  }
  f << "g0=" << input->readULong(m_version<0xa ? 1 : 2) << ",";
  f << "idx1?=" << input->readULong(2) << ",";
  f << "id=" << readNumber(input,0x200) << ",";
  int N1=0;
  if (hasN) {
    // condition seems ok when version<=3 or version>=10, unsure when 3<version<0x10 :-~
    N1=readNumber(input,0x200);
    f << "N=" << N1 << ",";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  for (int i=0; i<N1; ++i) {
    if (!readChild(zone, object))
      break;
    // find c2,c3,c6,c8
  }

  pos=input->tell();
  if (ok && pos+(m_version>=0x200 ? 3:4)<=lastPos) {
    // vers<=0101 && vers>=201 ok, other?
    int N2=readNumber(input,0x200);
    if (input->tell()+5*N2>lastPos)
      input->seek(pos, librevenge::RVNG_SEEK_SET);
    else {
      f.str("");
      f << "StarLayout[D0-B]:N=" << N2 << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      for (int i=0; i<N2; ++i) {
        pos=input->tell();
        if (pos+5>lastPos) {
          STOFF_DEBUG_MSG(("StarLayout::readD0: oops can not find a B-data\n"));
          break;
        }
        f.str("");
        f << "StarLayout[D0-B" << i << "]:";
        f << "id=" << input->readULong(2) << ",";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        if (!readChild(zone, object))
          break;
        // find type c1|c4|cc
      }
    }
  }
  pos=input->tell();
  if (pos!=lastPos) {
    STOFF_DEBUG_MSG(("StarLayout::readD0: find extraData\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarLayout[D0]:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSWRecord(static_cast<unsigned char>(0xd0), "StarLayout");
  return true;
}

bool StarLayout::readD8(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long pos=input->tell();
  unsigned char type;
  if (input->peek()!=0xd8 || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    STOFF_DEBUG_MSG(("StarLayout::readD8: can not read a layout\n"));
    return false;
  }
  f << "StarLayout[D8-" << zone.getRecordLevel() << "]:vers=" << m_version << ",";
  long lastPos=zone.getRecordLastPosition();
  int type1;
  bool ok=readHeader(zone, f, type1);
  unsigned long N=0;
  if (ok && (type1&0xF0) && type1<0xc0) {
    if (m_version>=0x200) {
      if (!input->readCompressedULong(N)) {
        f << "###N";
        ok=false;
      }
      else
        f << "N=" << std::hex << N << std::dec << ",";
    }
    else
      N=input->readULong(2);
    if (ok && N) {
      if (type1&0x20)
        f << "N=" << N << ",";
      else {
        f << "g0=" << N << ",";
        N=0;
      }
    }
    else
      N=0;
  }
  for (int i=0; i<int(N); ++i) {
    if (!readChild(zone, object))
      break;
    // find type c9
  }

  if (input->tell()!=lastPos) {
    if (!ok) {
      STOFF_DEBUG_MSG(("StarLayout::readD8: find extra data\n"));
    }
    ascFile.addDelimiter(input->tell(),'|');
    f << "###extra";
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  zone.closeSWRecord(static_cast<unsigned char>(0xd8), "StarLayout");
  return true;
}

bool StarLayout::readHeader(StarZone &zone, libstoff::DebugStream &f, int &type, int valueMode) const
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long lastPos=zone.getRecordLastPosition();
  ascFile.addDelimiter(input->tell(),'|');
  type=int(input->readULong(1));

  if (type>=0xc0) // checkme: find here c[13]01, unsure what this means...
    type=int(static_cast<unsigned long>(type)+(input->readULong(1)<<8));
  if (type) f << "type1=" << std::hex << type << std::dec << ",";

  auto val=int(input->readULong(1));
  if (val) f << "f0=" << std::hex << val << std::dec << ",";
  val=int(input->readULong(1));
  if (val) f << "f1=" << std::hex << val << std::dec << ",";
  for (int s=0; s<2; ++s) {
    if ((type&(1<<s))==0) continue;
    f << "block" << s << "=[";
    bool ok=readDataBlock(zone, f);
    f << "],";
    if (!ok) return false;
  }
  if ((type&0xc)) {
    if ((type&0xc)==0xc && m_version<0x200)
      f << "db[" << ((type&0xc) >> 2) << "]=" << std::hex << input->readULong(2) << ":" << input->readULong(2) << std::dec << ",";
    else
      f << "db[" << ((type&0xc) >> 2) << "]=" << std::hex << input->readULong(2) << std::dec << ",";
  }
  if (type>=0xc0)
    f << "dc[" << ((type&0xfc) >> 10) << "]=" << std::hex << input->readULong(2) << std::dec << ",";
  switch (valueMode) {
  case 0:
    break;
  case 1:
    f << "val=[";
    for (int i=0; i<3; ++i) {
      unsigned long N;
      if (m_version>=0x200) {
        if (!input->readCompressedULong(N)) {
          STOFF_DEBUG_MSG(("StarLayout::readHeader: oops, can not read some value\n"));
          f << "###val";
          return false;
        }
      }
      else
        N=input->readULong(1);
      f << N;
      if (i==1 && type>=0xc0)
        f << "[dc=" << input->readULong(2) << "]";
      f  << ",";
    }
    f << "],";
    break;
  case 2: // unsure, often 3fffff|3ffeff, maybe a small double
    f << "val=[";
    for (int i=0; i<3; ++i)
      f << input->readULong(1) << ",";
    f << "],";
    break;
  default:
    STOFF_DEBUG_MSG(("StarLayout::readHeader: oops, unknown value mode %d\n", valueMode));
    f << "###mode=" << valueMode << ",";
    break;
  }
  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarLayout::readHeader: oops, the zone seems too short\n"));
    f << "###toShort,";
    return false;
  }
  return true;
}

bool StarLayout::readChild(StarZone &zone, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  bool done=false;
  switch (input->peek()) {
  case 0xc1:
  case 0xcc:
  case 0xcd:
    done=readC1(zone, object);
    break;
  case 0xc2:
  case 0xc3:
  case 0xc6:
  case 0xc8:
  case 0xc9: // child c7|d4|d8
  case 0xce:
  case 0xd2: // child d0
  case 0xd3:
  case 0xd4: // child f2
  case 0xd7: // child d0
  case 0xe3: // child d8
  case 0xf2: // child e3
    done=readC2(zone, object);
    break;
  case 0xc4:
  case 0xc7:
    done=readC4(zone, object);
    break;
  case 0xd0:
    done=readD0(zone, object);
    break;
  case 0xd8:
    done=readD8(zone, object);
    break;
  default:
    break;
  }
  if (done && input->tell()>pos && input->tell()<=zone.getRecordLastPosition()) return true;
  unsigned char type;
  if ((input->peek()&0xE0)!=0xc0 || !zone.openSWRecord(type)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugStream f;
  f << "StarLayout[###" << std::hex << int(static_cast<unsigned char>(type)) << std::dec << "-" << zone.getRecordLevel() << "]:";
  STOFF_DEBUG_MSG(("StarLayout::readChild: find unexpected child\n"));
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
  zone.closeSWRecord(type, "StarLayout");
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
