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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#ifdef USE_ZIP
#  include <zlib.h>
#endif

#include <librevenge/librevenge.h>

#include "StarItemPool.hxx"
#include "StarZone.hxx"
#include "STOFFStringStream.hxx"

#include "StarBitmap.hxx"

/** Internal: the structures of a StarBitmap */
namespace StarBitmapInternal
{
////////////////////////////////////////////////////////////
//! internal: the bitmap information
struct Bitmap {
  //! constructor
  Bitmap()
    : m_width(0)
    , m_height(0)
    , m_planes(0)
    , m_bitCount(0)
    , m_compression(0)
    , m_sizeImage(0)
    , m_hasAlphaColor(false)
    , m_colorsList()
    , m_indexDataList()
    , m_colorDataList()
  {
    m_pixelsPerMeter[0]=m_pixelsPerMeter[1]=0;
    m_numColors[0]=m_numColors[1]=0;
  }
  //! try to return a ppm data (without alpha)
  bool getPPMData(librevenge::RVNGBinaryData &data) const
  {
    if (!m_width || !m_height || ((m_colorsList.empty() || m_indexDataList.empty()) && m_colorDataList.empty()))
      return false;
    data.clear();
    std::stringstream f;
    f << "P6\n" << m_width << " " << m_height << " 255\n";
    auto const header = f.str();
    data.append(reinterpret_cast<const unsigned char *>(header.c_str()), header.size());
    if (!m_colorDataList.empty()) {
      if (m_colorDataList.size()!=size_t(m_width*m_height)) {
        STOFF_DEBUG_MSG(("StarBitmapInternal::Bitmap::getPPMData: color data list's size is bad\n"));
        return false;
      }
      for (auto const &c : m_colorDataList) {
        uint32_t col=c.value();
        for (int comp=0, depl=16; comp<3; ++comp, depl-=8)
          data.append(static_cast<unsigned char>((col>>depl)&0xFF));
      }
      return true;
    }
    if (m_indexDataList.size()/m_width!=m_height || m_indexDataList.size()%m_width!=0) {
      STOFF_DEBUG_MSG(("StarBitmapInternal::Bitmap::getPPMData: index data list's size is bad\n"));
      return false;
    }
    auto numColors=int(m_colorsList.size());
    for (auto const &index : m_indexDataList) {
      if (index<0 || index>=numColors) {
        STOFF_DEBUG_MSG(("StarBitmapInternal::Bitmap::getPPMData: find bad index=%d\n", index));
        return false;
      }
      uint32_t col=m_colorsList[size_t(index)].value();
      for (int comp=0, depl=16; comp<3; ++comp, depl-=8)
        data.append(static_cast<unsigned char>((col>>depl)&0xFF));
    }
    return true;
  }

  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Bitmap const &info)
  {
    if (info.m_width || info.m_height) o << "sz=" << info.m_width << "x" << info.m_height << ",";
    if (info.m_planes) o << "num[planes]=" << info.m_planes << ",";
    if (info.m_bitCount) o << "bit[count]=" << info.m_bitCount << ",";
    if (info.m_compression==0x1004453) o << "zCompress,";
    else if (info.m_compression) o << "compression=" << info.m_compression << ",";
    if (info.m_sizeImage) o << "size[image]=" << info.m_sizeImage << ",";
    if (info.m_pixelsPerMeter[0] || info.m_pixelsPerMeter[1])
      o << "pixelsPerMeter=" << info.m_pixelsPerMeter[0] << "x" << info.m_pixelsPerMeter[1] << ",";
    if (info.m_numColors[0]) o << "num[colors]=" << info.m_numColors[0] << ",";
    if (info.m_numColors[1] && info.m_numColors[1]!=info.m_numColors[0]) o << "num[used]=" << info.m_numColors[1] << ",";
    return o;
  }
  //! bitmap width
  uint32_t m_width;
  //! bitmap height
  uint32_t m_height;
  //! the number of planes
  uint16_t m_planes;
  //! the bit count
  uint16_t m_bitCount;
  //! related to compression
  uint32_t m_compression;
  //! the image size?
  uint32_t m_sizeImage;
  //! the number of x/y pixel by meters
  uint32_t m_pixelsPerMeter[2];
  //! a flag to know if the color has alpha component
  bool m_hasAlphaColor;
  //! the number of used column (used, other)
  uint32_t m_numColors[2];
  //! the bitmap color list
  std::vector<STOFFColor> m_colorsList;
  //! the index bitmap data
  std::vector<int> m_indexDataList;
  //! the color bitmap data
  std::vector<STOFFColor> m_colorDataList;
};
////////////////////////////////////////
//! Internal: the state of a StarBitmap
struct State {
  //! constructor
  State()
    : m_bitmap()
  {
  }
  //! the bitmap
  Bitmap m_bitmap;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarBitmap::StarBitmap()
  : m_state(new StarBitmapInternal::State)
{
}

StarBitmap::~StarBitmap()
{
}

StarBitmap::StarBitmap(uint32_t const((&pixels)[32]), STOFFColor const((&colors)[2]))
  : m_state(new StarBitmapInternal::State)
{
  m_state->m_bitmap.m_width=m_state->m_bitmap.m_height=32;
  for (auto color : colors)
    m_state->m_bitmap.m_colorsList.push_back(color);
  m_state->m_bitmap.m_indexDataList.resize(32*32);
  size_t w=0;
  uint32_t const *read=pixels;
  for (int i=0; i<32; ++i) {
    uint32_t depl=0x80000000;
    uint32_t val=*(read++);
    for (int comp=0; comp<32; ++comp, depl>>=1)
      m_state->m_bitmap.m_indexDataList[w++]=(val&depl) ? 1 : 0;
  }
}

bool StarBitmap::getData(librevenge::RVNGBinaryData &data, std::string &type) const
{
  if (!m_state->m_bitmap.getPPMData(data))
    return false;
  type="image/ppm";
  return true;
}

STOFFVec2i StarBitmap::getBitmapSize() const
{
  return STOFFVec2i(int(m_state->m_bitmap.m_width), int(m_state->m_bitmap.m_height));
}

bool StarBitmap::readBitmap(StarZone &zone, bool inFileHeader, long lastPos, librevenge::RVNGBinaryData &result, std::string &type)
{
  result.clear();
  type="";
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long beginPos=input->tell(), pos=beginPos;
  libstoff::DebugStream f;
  f << "Entries(StarBitmap)[" << zone.getRecordLevel() << "]:";

  // bitmap2.cxx: Bitmap::Read
  long dataPos=0, offset=0;
  if (inFileHeader) {
    // ImplReadDIBFileHeader
    f << "header,";
    uint16_t header;
    *input >> header;
    bool ok=true;
    if (header==0x4142) {
      input->seek(12, librevenge::RVNG_SEEK_CUR);
      ok=(input->readULong(2)==0x4d42);
      input->seek(8, librevenge::RVNG_SEEK_CUR);
      offset=long(input->readULong(4));
    }
    else if (header==0x4d42) {
      input->seek(8, librevenge::RVNG_SEEK_CUR);
      offset=long(input->readULong(4));
    }
    else
      ok=false;
    f << "offset=" << offset << ",";
    if (!ok || offset<0) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not read the header\n"));
      f << "###";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  auto &bitmap=m_state->m_bitmap;
  if (!readBitmapInformation(zone, bitmap, lastPos)) return false;

  pos=input->tell();
  f.str("");
  f << "StarBitmap:";
  STOFFInputStreamPtr dInput=input;
  long endDataPos=lastPos;
  if (bitmap.m_compression==0x1004453) {
    uint32_t codeSize, uncodeSize;
    *input>>codeSize>>uncodeSize>>bitmap.m_compression;
    f << "size[coded]=" << codeSize << ",";
    f << "size[uncoded]=" << uncodeSize << ",";
    if (bitmap.m_compression) f << "compression=" << bitmap.m_compression << ",";
    if (input->tell()+long(codeSize)>lastPos || uncodeSize==0 || uncodeSize>10000000) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: bad code size\n"));
      f << "###codeSize=" << codeSize << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    lastPos=input->tell()+long(codeSize);
#ifdef USE_ZIP
    ascFile.skipZone(input->tell(),lastPos-1);
    unsigned long readBytes=0;
    uint8_t const *data=input->read(size_t(codeSize),readBytes);
    if (!data || static_cast<uint32_t>(readBytes)!=codeSize) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not read compressed data\n"));
      f << "###compressed";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }

    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wold-style-cast"
#endif
    ret = inflateInit(&strm);//,-MAX_WBITS);
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
    if (ret != Z_OK) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not init stream\n"));
      f << "###inflateInit";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      return true;
    }
    strm.avail_in = unsigned(codeSize);
    strm.next_in = reinterpret_cast<Bytef *>(const_cast<uint8_t *>(data));

    std::vector<unsigned char> converted;
    converted.resize(size_t(uncodeSize),0);

    strm.avail_out = uncodeSize;
    strm.next_out = reinterpret_cast<Bytef *>(&converted[0]);
    ret = inflate(&strm, Z_FINISH);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not decode stream\n"));
      f << "###inflateDecode,err=" << ret << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      input->seek(lastPos, librevenge::RVNG_SEEK_SET);
      (void)inflateEnd(&strm);
      return true;
    default:
      break;
    }
    (void)inflateEnd(&strm);
    std::shared_ptr<librevenge::RVNGInputStream> newStream(new STOFFStringStream(&converted[0], unsigned(converted.size())));
    dInput.reset(new STOFFInputStream(newStream, input->readInverted()));
    dataPos=offset=0;
    endDataPos=dInput->size();
#else
    STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not decode zip file\n"));
    f << "###codeSize=" << codeSize << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
#endif
  }
  if (offset && beginPos+long(offset)>endDataPos) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmap: the offset seems bad\n"));
    f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (offset)
    dataPos=beginPos+offset;
  int const bitCount=bitmap.m_bitCount<=1 ? 1 : bitmap.m_bitCount<=4 ? 4 : bitmap.m_bitCount<=8 ? 8 : 24;
  int nColors=0;
  if (bitCount <= 8) {
    if (bitmap.m_numColors[0] > 0 && bitmap.m_numColors[0] < (1u << bitCount))
      nColors = int(bitmap.m_numColors[0]);
    else
      nColors = int(1 << bitCount);
  }
  int const numComponent=bitmap.m_hasAlphaColor ? 4 : 3;
  if ((endDataPos-dInput->tell())/numComponent<nColors || dInput->tell()+numComponent*nColors > endDataPos) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not read the color\n"));
    f << "###";
    ascFile.addDelimiter(input->tell(),'|');
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    return true;
  }
  if (nColors) {
    f << "colors=[";
    unsigned char col[4]= {0,0,0,255};
    for (int i=0; i<nColors; ++i) {
      for (int c=0; c<numComponent; ++c) col[c]=static_cast<unsigned char>(dInput->readULong(1));
      bitmap.m_colorsList.push_back(STOFFColor(col[2],col[1],col[0],col[3]));
      f << bitmap.m_colorsList.back() << ",";
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  pos=input->tell();
  if (dataPos && dataPos!=dInput->tell())
    dInput->seek(dataPos, librevenge::RVNG_SEEK_SET);
  if (!readBitmapData(dInput, bitmap, endDataPos)) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not read the bitmap\n"));
    ascFile.addPos(pos);
    ascFile.addNote("StarBitmap:###unread");
    return false;
  }
  if (bitmap.m_compression!=0x1004453)
    ascFile.skipZone(pos,input->tell()-1);
  pos=input->tell();

  if (inFileHeader) {
    long actPos=input->tell();
    input->seek(beginPos, librevenge::RVNG_SEEK_SET);
    if (!input->readDataBlock(lastPos-beginPos,result)) {
      result.clear();
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not read the result\n"));
    }
    else {
      type="image/bm";
#if defined(DEBUG_WITH_FILES) && DEBUG_WITH_FILES
      static int bitmapNum=0;
      std::stringstream s;
      s << "Bitmap" << ++bitmapNum << ".bm";
      libstoff::Debug::dumpFile(result, s.str().c_str());
#endif
    }
    input->seek(actPos, librevenge::RVNG_SEEK_SET);
  }
  if (pos!=lastPos && bitmap.m_compression==0x1004453) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmap: find extra data\n"));
    ascFile.addDelimiter(pos, '|');
    ascFile.addPos(pos);
    ascFile.addNote("StarBitmap:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
#ifdef DEBUG_WITH_FILES
  if (1) {
    librevenge::RVNGBinaryData data;
    static int bitmapNum=0;
    std::stringstream s;
    s << "Bitmap" << ++bitmapNum << ".ppm";

    data.clear();
    if (!bitmap.getPPMData(data)) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmap: can not convert a bitmap\n"));
    }
    else
      libstoff::Debug::dumpFile(data, s.str().c_str());
  }
#endif
  return true;
}

bool StarBitmap::readBitmapInformation(StarZone &zone, StarBitmapInternal::Bitmap &info, long lastPos)
{
  // bitmap2.cxx ImplReadDIBInfoHeader
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  long pos=input->tell();
  libstoff::DebugStream f;
  f << "StarBitmap[info-" << zone.getRecordLevel() << "]:";
  uint32_t hSz;
  *input>>hSz;
  if (hSz<12 || pos+long(hSz)>lastPos) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmapInformation: find bad header size"));
    f << "###hSz=" << hSz << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return true;
  }
  long endPos=pos+long(hSz);
  if (hSz==12) {
    uint16_t width, height, planes, bitCount;
    *input >> width >> height >> planes >> bitCount;
    info.m_width=width;
    info.m_height=height;
    info.m_planes=planes;
    info.m_bitCount=bitCount;
  }
  else {
    info.m_hasAlphaColor=true;
    *input >> info.m_width >> info.m_height >> info.m_planes >> info.m_bitCount;
    if (input->tell()+4 <= lastPos) {
      *input >> info.m_compression;
      bool ok=true;
      if (input->tell()+4 <= lastPos)
        *input >> info.m_sizeImage;
      else
        ok=false;
      for (int i=0; ok && i<2; ++i) {
        if (input->tell()+4 <= lastPos)
          *input >> info.m_pixelsPerMeter[i];
        else
          ok=false;
      }
      for (int i=0; ok && i<2; ++i) {
        if (input->tell()+4 <= lastPos)
          *input >> info.m_numColors[i];
        else
          ok=false;
      }
    }
  }
  if (input->tell() != endPos) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmapInformation: find extra data"));
    f << "###extra,";
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
  }
  f << info;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarBitmap::readBitmapData(STOFFInputStreamPtr &input, StarBitmapInternal::Bitmap &bitmap, long lastPos)
{
  // bitmap2.cxx Bitmap::ImplReadDIBBits

  uint32_t RGBMask[3]= {0,0,0};
  int RGBShift[3]= {0,0,0};
  if (bitmap.m_bitCount==16 || bitmap.m_bitCount==32) { // RGBMask
    if (bitmap.m_compression==3) { // BITFIELDS
      if (input->tell()<12) {
        STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: can not find RGB mask\n"));
        return false;
      }
      input->seek(-12, librevenge::RVNG_SEEK_CUR);
      for (int i=0; i<3; ++i) {
        *input >> RGBMask[i];
        uint32_t decal=0x80000000ul;
        for (int j=31; j>=7; --j, decal>>=1) {
          if ((RGBMask[i]&decal)==0) continue;
          RGBShift[i]=j-7;
          break;
        }
      }
    }
    else {
      RGBMask[0]= bitmap.m_bitCount == 16 ? 0x00007c00UL : 0x00ff0000UL;
      RGBMask[1]= bitmap.m_bitCount == 16 ? 0x000003e0UL : 0x0000ff00UL;
      RGBMask[2]= bitmap.m_bitCount == 16 ? 0x0000001fUL : 0x000000ffUL;
      RGBShift[0]= bitmap.m_bitCount == 16 ? 10 : 16;
      RGBShift[1]= bitmap.m_bitCount == 16 ? 5 : 8;
      RGBShift[2]= 0; // checkme
    }
  }

  if ((bitmap.m_bitCount==8 && bitmap.m_compression==1) || (bitmap.m_bitCount==4 && bitmap.m_compression==2)) { // bRLE
    if (bitmap.m_sizeImage && static_cast<unsigned long>(input->tell())+static_cast<unsigned long>(bitmap.m_sizeImage) > static_cast<unsigned long>(lastPos)) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: image size is bad\n"));
      return false;
    }
    if (bitmap.m_sizeImage) lastPos= input->tell() + long(bitmap.m_sizeImage);
    bool bit4=bitmap.m_compression==2;
    size_t wPos=0, lastWPos=size_t(bitmap.m_height*bitmap.m_width);
    if (lastWPos == 0 || lastWPos / bitmap.m_height != bitmap.m_width) {
      STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: bitmap dimensions are too big\n"));
      return false;
    }
    bitmap.m_indexDataList.resize(size_t(lastWPos),0);
    uint32_t x=0, y=0;
    while (true) {
      if (input->tell() > lastPos - 1) {
        STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: at the end of the bitmap record already\n"));
        return false;
      }
      if (y>=bitmap.m_height) break;
      auto nCount=int(input->readULong(1));
      if (!nCount) {
        auto nBytes=int(input->readULong(1));
        if (nBytes==0) { // new line
          ++y;
          x=0;
          wPos=size_t(y*bitmap.m_width);
          continue;
        }
        if (nBytes==1) // end decoding
          break;
        if (nBytes==2) {
          x = uint32_t(x+input->readULong(1));
          y = uint32_t(y+input->readULong(1));
          if (x>=bitmap.m_width) {
            STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: x seems bad\n"));
          }
          if (y>=bitmap.m_height) {
            STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: y seems bad\n"));
            break;
          }
          wPos=size_t(y*bitmap.m_width+x);
          continue;
        }
        int nRead=(bit4 ? (nBytes+1)/2 : nBytes);
        if (input->tell()+nRead> lastPos) {
          STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: can not read some lre count(1)\n"));
          return false;
        }
        for (int i=0; i<nBytes; ++i) {
          auto val=int(input->readULong(1));
          if (bit4) {
            if (++x<=bitmap.m_width && wPos<lastWPos) bitmap.m_indexDataList[wPos++]=(val>>4)&0xf;
            if (++i<nBytes && ++x<=bitmap.m_width && wPos<lastWPos) bitmap.m_indexDataList[wPos++]=val&0xf;
          }
          else if (++x<=bitmap.m_width && wPos<lastWPos)
            bitmap.m_indexDataList[wPos++]=val;
        }
        if (nRead&1)
          input->seek(1, librevenge::RVNG_SEEK_CUR);
        continue;
      }
      if (input->tell()+1 > lastPos) {
        STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: can not read some lre count(2)\n"));
        return false;
      }
      auto val=int(input->readULong(1));
      if (bit4) {
        for (int i=0; i<nCount; ++i) {
          if (++x>bitmap.m_width||wPos>=lastWPos) break;
          bitmap.m_indexDataList[wPos++]=(val>>4)&0xf;
          if (++i>=nCount || ++x>bitmap.m_width||wPos>=lastWPos) break;
          bitmap.m_indexDataList[wPos++]=val&0xf;
        }
      }
      else {
        for (int i=0; i<nCount; ++i) {
          if (++x>bitmap.m_width||wPos>=lastWPos) break;
          bitmap.m_indexDataList[wPos++]=val;
        }
      }
    }
    return true;
  }
  uint32_t alignWidth=bitmap.m_width*bitmap.m_bitCount;
  if (bitmap.m_width==0 || alignWidth/bitmap.m_width!=bitmap.m_bitCount) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: the bitmap width seems bad\n"));
    return false;
  }
  alignWidth=(((alignWidth+31)>>5)<<2);
  long actPos=input->tell();
  if (alignWidth==0 || uint32_t(lastPos-actPos)/alignWidth < bitmap.m_height ||
      actPos+long(bitmap.m_height*alignWidth)>lastPos) {
    STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: the zone seems too short\n"));
    return false;
  }
  switch (bitmap.m_bitCount) {
  case 1: {
    bitmap.m_indexDataList.resize(size_t(bitmap.m_height*bitmap.m_width));
    size_t wPos=0;
    for (uint32_t y=0; y<bitmap.m_height; ++y) {
      actPos=input->tell();
      int decal=7;
      unsigned char val=0;
      for (uint32_t x=0; x<bitmap.m_width; ++x) {
        if (decal==7)
          val=static_cast<unsigned char>(input->readULong(1));
        bitmap.m_indexDataList[wPos++]=int((val>>decal)&1);
        if (decal--==0)
          decal=7;
      }
      input->seek(actPos+long(alignWidth), librevenge::RVNG_SEEK_SET);
    }
    break;
  }
  case 4: {
    bitmap.m_indexDataList.resize(size_t(bitmap.m_height*bitmap.m_width));
    size_t wPos=0;
    for (uint32_t y=0; y<bitmap.m_height; ++y) {
      actPos=input->tell();
      unsigned char val=0;
      for (uint32_t x=0; x<bitmap.m_width; ++x) {
        if ((x%2)==0)
          val=static_cast<unsigned char>(input->readULong(1));
        bitmap.m_indexDataList[wPos++]=int(((x%2) ? val : (val>>4))&0xf);
      }
      input->seek(actPos+long(alignWidth), librevenge::RVNG_SEEK_SET);
    }
    break;
  }
  case 8: {
    bitmap.m_indexDataList.resize(size_t(bitmap.m_height*bitmap.m_width));
    size_t wPos=0;
    for (uint32_t y=0; y<bitmap.m_height; ++y) {
      actPos=input->tell();
      for (uint32_t x=0; x<bitmap.m_width; ++x)
        bitmap.m_indexDataList[wPos++]=int(input->readULong(1));
      input->seek(actPos+long(alignWidth), librevenge::RVNG_SEEK_SET);
    }
    break;
  }
  case 16: {
    bitmap.m_colorDataList.resize(size_t(bitmap.m_height*bitmap.m_width));
    size_t wPos=0;
    for (uint32_t y=0; y<bitmap.m_height; ++y) {
      actPos=input->tell();
      for (uint32_t x=0; x<bitmap.m_width; ++x) {
        auto val=static_cast<uint16_t>(input->readULong(2));
        bitmap.m_colorDataList[wPos++]=
          STOFFColor(static_cast<unsigned char>((val&RGBMask[0])>>RGBShift[0]), static_cast<unsigned char>((val&RGBMask[1])>>RGBShift[1]),
                     static_cast<unsigned char>((val&RGBMask[2])>>RGBShift[2]));
      }
      input->seek(actPos+long(alignWidth), librevenge::RVNG_SEEK_SET);
    }
    break;
  }
  case 24:
  case 32: {
    unsigned char col[4]= {0,0,0,255};
    int const numComponent= bitmap.m_bitCount==24 ? 3 : 4;
    bitmap.m_colorDataList.resize(size_t(bitmap.m_height*bitmap.m_width));
    size_t wPos=0;
    for (uint32_t y=0; y<bitmap.m_height; ++y) {
      actPos=input->tell();
      for (uint32_t x=0; x<bitmap.m_width; ++x) {
        for (int c=0; c<numComponent; ++c) col[c]=static_cast<unsigned char>(input->readULong(1));
        bitmap.m_colorDataList[wPos++]=STOFFColor(col[0],col[1],col[2],col[3]);
      }
      input->seek(actPos+long(alignWidth), librevenge::RVNG_SEEK_SET);
    }
    break;
  }
  default:
    STOFF_DEBUG_MSG(("StarBitmap::readBitmapData: find unexpected bit count %d\n", int(bitmap.m_bitCount)));
    input->seek(actPos+long(bitmap.m_height*alignWidth), librevenge::RVNG_SEEK_SET);
    break;
  }
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
