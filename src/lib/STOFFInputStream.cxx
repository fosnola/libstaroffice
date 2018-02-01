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

#include <string.h>

#include <limits>
#include <cmath>
#include <cstring>

#include <librevenge-stream/librevenge-stream.h>
#include <librevenge/librevenge.h>

#include "STOFFDebug.hxx"

#include "STOFFInputStream.hxx"

STOFFInputStream::STOFFInputStream(std::shared_ptr<librevenge::RVNGInputStream> inp, bool inverted)
  : m_stream(inp)
  , m_streamSize(0)
  , m_inverseRead(inverted)
{
  updateStreamSize();
}

STOFFInputStream::STOFFInputStream(librevenge::RVNGInputStream *inp, bool inverted)
  : m_stream()
  , m_streamSize(0)
  , m_inverseRead(inverted)
{
  if (!inp) return;

  m_stream = std::shared_ptr<librevenge::RVNGInputStream>(inp, STOFF_shared_ptr_noop_deleter<librevenge::RVNGInputStream>());
  updateStreamSize();
  if (m_stream)
    seek(0, librevenge::RVNG_SEEK_SET);
}

STOFFInputStream::~STOFFInputStream()
{
}

std::shared_ptr<STOFFInputStream> STOFFInputStream::get(librevenge::RVNGBinaryData const &data, bool inverted)
{
  std::shared_ptr<STOFFInputStream> res;
  if (!data.size())
    return res;
  auto *dataStream = const_cast<librevenge::RVNGInputStream *>(data.getDataStream());
  if (!dataStream) {
    STOFF_DEBUG_MSG(("STOFFInputStream::get: can not retrieve a librevenge::RVNGInputStream\n"));
    return res;
  }
  res.reset(new STOFFInputStream(dataStream, inverted));
  if (res && res->size()>=long(data.size())) {
    res->seek(0, librevenge::RVNG_SEEK_SET);
    return res;
  }
  STOFF_DEBUG_MSG(("STOFFInputStream::get: the final stream seems bad\n"));
  res.reset();
  return res;
}

void STOFFInputStream::updateStreamSize()
{
  if (!m_stream)
    m_streamSize=0;
  else {
    long actPos = tell();
    m_stream->seek(0, librevenge::RVNG_SEEK_END);
    m_streamSize=tell();
    m_stream->seek(actPos, librevenge::RVNG_SEEK_SET);
  }
}

const uint8_t *STOFFInputStream::read(size_t numBytes, unsigned long &numBytesRead)
{
  if (!hasDataFork())
    throw libstoff::FileException();
  return m_stream->read(numBytes,numBytesRead);
}

long STOFFInputStream::tell()
{
  if (!hasDataFork())
    return 0;
  return m_stream->tell();
}

int STOFFInputStream::seek(long offset, librevenge::RVNG_SEEK_TYPE seekType)
{
  if (!hasDataFork()) {
    if (offset == 0)
      return 0;
    throw libstoff::FileException();
  }

  if (seekType == librevenge::RVNG_SEEK_CUR)
    offset += tell();
  else if (seekType==librevenge::RVNG_SEEK_END)
    offset += m_streamSize;

  if (offset < 0)
    offset = 0;
  if (offset > size())
    offset = size();

  return m_stream->seek(offset, librevenge::RVNG_SEEK_SET);
}

bool STOFFInputStream::isEnd()
{
  if (!hasDataFork())
    return true;
  long pos = m_stream->tell();
  if (pos >= size()) return true;

  return m_stream->isEnd();
}

unsigned long STOFFInputStream::readULong(librevenge::RVNGInputStream *stream, int num, unsigned long a, bool inverseRead)
{
  if (!stream || num == 0 || stream->isEnd()) return a;
  if (inverseRead) {
    unsigned long val = readU8(stream);
    return val + (readULong(stream, num-1,0, inverseRead) << 8);
  }
  switch (num) {
  case 4:
  case 2:
  case 1: {
    unsigned long numBytesRead;
    uint8_t const *p = stream->read(static_cast<unsigned long>(num), numBytesRead);
    if (!p || int(numBytesRead) != num)
      return 0;
    switch (num) {
    case 4:
      return static_cast<unsigned long>(p[3])|(static_cast<unsigned long>(p[2])<<8)|
             (static_cast<unsigned long>(p[1])<<16)|(static_cast<unsigned long>(p[0])<<24)|((a<<16)<<16);
    case 2:
      return (static_cast<unsigned long>(p[1]))|(static_cast<unsigned long>(p[0])<<8)|(a<<16);
    case 1:
      return (static_cast<unsigned long>(p[0]))|(a<<8);
    default:
      break;
    }
    break;
  }
  default:
    return readULong(stream, num-1,(a<<8) + static_cast<unsigned long>(readU8(stream)), inverseRead);
  }
  return 0;
}

long STOFFInputStream::readLong(int num)
{
  auto v = long(readULong(num));
  switch (num) {
  case 4:
    return static_cast<int32_t>(v);
  case 2:
    return static_cast<int16_t>(v);
  case 1:
    return static_cast<int8_t>(v);
  default:
    break;
  }

  if ((v & long(0x1 << (num*8-1))) == 0) return v;
  return v | long(0xFFFFFFFF << 8*num);
}

uint8_t STOFFInputStream::readU8(librevenge::RVNGInputStream *stream)
{
  if (!stream)
    return 0;
  unsigned long numBytesRead;
  uint8_t const *p = stream->read(sizeof(uint8_t), numBytesRead);

  if (!p || numBytesRead != sizeof(uint8_t))
    return 0;

  return *reinterpret_cast<uint8_t const *>(p);
}

int STOFFInputStream::peek()
{
  if (isEnd()) return -1;
  auto res=int(readULong(1));
  seek(-1, librevenge::RVNG_SEEK_CUR);
  return res;
}

bool STOFFInputStream::readColor(STOFFColor &color)
{
  if (!m_stream || !checkPosition(tell()+2)) return false;
  auto colId=int(readULong(2));
  if (colId & 0x8000) {
    if (!checkPosition(tell()+6)) return false;
    unsigned char col[3];
    for (unsigned char &i : col) i=static_cast<unsigned char>(readULong(2)>>8);
    color=STOFFColor(col[0],col[1],col[2]);
    return true;
  }
  static uint32_t const listColors[] = {
    0,                          // COL_BLACK
    0x000080,                           // COL_BLUE
    0x008000,                          // COL_GREEN
    0x008080,                           // COL_CYAN
    0x800000,                            // COL_RED
    0x800080,                        // COL_MAGENTA
    0x808000,                          // COL_BROWN
    0x808080,                           // COL_GRAY
    0xc0c0c0,                      // COL_LIGHTGRAY
    0x0000ff,                      // COL_LIGHTBLUE
    0x00ff00,                     // COL_LIGHTGREEN
    0x00ffff,                      // COL_LIGHTCYAN
    0xff0000,                       // COL_LIGHTRED
    0xff00ff,                   // COL_LIGHTMAGENTA
    0xffff00,                         // COL_YELLOW
    0xffffff,                          // COL_WHITE
    0xffffff,                          // COL_MENUBAR
    0,                          // COL_MENUBARTEXT
    0xffffff,                          // COL_POPUPMENU
    0,                          // COL_POPUPMENUTEXT
    0,                          // COL_WINDOWTEXT
    0xffffff,                          // COL_WINDOWWORKSPACE
    0,                          // COL_HIGHLIGHT
    0xffffff,                          // COL_HIGHLIGHTTEXT
    0,                          // COL_3DTEXT
    0xc0c0c0,                      // COL_3DFACE
    0xffffff,                          // COL_3DLIGHT
    0x808080,                           // COL_3DSHADOW
    0xc0c0c0,                      // COL_SCROLLBAR
    0xffffff,                          // COL_FIELD
    0                           // COL_FIELDTEXT
  };
  if (colId<0 || colId>=int(STOFF_N_ELEMENTS(listColors))) {
    STOFF_DEBUG_MSG(("STOFFInputStream::readColor: can not find color %d\n", colId));
    return false;
  }
  color=STOFFColor(listColors[colId]);
  return true;
}

bool STOFFInputStream::readCompressedULong(unsigned long &res)
{
  // sw_sw3imp.cxx Sw3IoImp::InULong
  if (!m_stream)
    return false;

  unsigned long numBytesRead;
  uint8_t const *p = m_stream->read(sizeof(uint8_t), numBytesRead);

  if (!p || numBytesRead != sizeof(uint8_t))
    return false;
  if ((p[0]&0x80)==0) {
    res=p[0]&0x7f;
    return true;
  }
  if ((p[0]&0xC0)==0x80) {
    res=(p[0]&0x3f);
    p = m_stream->read(sizeof(uint8_t), numBytesRead);
    if (!p || numBytesRead != sizeof(uint8_t))
      return false;
    res=(res<<8)|p[0];
    return true;
  }
  if ((p[0]&0xe0)==0xc0) {
    res=p[0]&0x1f;
    p = m_stream->read(2*sizeof(uint8_t), numBytesRead);

    if (!p || numBytesRead != 2*sizeof(uint8_t))
      return false;
    res= (res<<16)|static_cast<unsigned long>(p[1]<<8)|p[0];//checkme
    return true;
  }
  if ((p[0]&0xf0)==0xe0) {
    res=p[0]&0xf;
    p = m_stream->read(3*sizeof(uint8_t), numBytesRead);

    if (!p || numBytesRead != 3*sizeof(uint8_t))
      return false;
    res=(res<<24)|static_cast<unsigned long>(p[0]<<16)|static_cast<unsigned long>(p[2]<<8)|p[1];//checkme
    return true;
  }
  if ((p[0]&0xf8)==0xf0) {
    res=readULong(4);
    return true;
  }
  return false;
}

bool STOFFInputStream::readCompressedLong(long &res)
{
  if (!m_stream)
    return false;

  unsigned long numBytesRead;
  uint8_t const *p = m_stream->read(sizeof(uint8_t), numBytesRead);

  if (!p || numBytesRead != sizeof(uint8_t))
    return false;
  if (p[0]&0x80) {
    res=p[0]&0x7f;
    return true;
  }
  if (p[0]&0x40) {
    res=p[0]&0x3f;
    p = m_stream->read(sizeof(uint8_t), numBytesRead);

    if (!p || numBytesRead != sizeof(uint8_t))
      return false;
    res=(res<<8)|p[0];
    return true;
  }
  else if (p[0]&0x20) {
    res=p[0]&0x1f;
    p = m_stream->read(3*sizeof(uint8_t), numBytesRead);

    if (!p || numBytesRead != 3*sizeof(uint8_t))
      return false;
    res=(res<<24)|(p[0]<<16)|(p[2]<<8)|p[1];//checkme
    return true;
  }
  else if (p[0]==0x10) {
    long pos=tell();
    if (pos+4 > m_streamSize) return false;
    res=long(readULong(4));
    return true;
  }
  return false;
}

bool STOFFInputStream::readDouble8(double &res, bool &isNotANumber)
{
  if (!m_stream) return false;
  long pos=tell();
  if (pos+8 > m_streamSize) return false;

  isNotANumber=false;
  res=0;
  auto mantExp=int(readULong(1));
  auto val=int(readULong(1));
  int exp=(mantExp<<4)+(val>>4);
  double mantisse=double(val&0xF)/16.;
  double factor=1./16/256.;
  for (int j = 0; j < 6; ++j, factor/=256)
    mantisse+=double(readULong(1))*factor;
  int sign = 1;
  if (exp & 0x800) {
    exp &= 0x7ff;
    sign = -1;
  }
  if (exp == 0) {
    if (mantisse <= 1.e-5 || mantisse >= 1-1.e-5)
      return true;
    // a Nan representation ?
    return false;
  }
  if (exp == 0x7FF) {
    if (mantisse >= 1.-1e-5) {
      isNotANumber=true;
      res=std::numeric_limits<double>::quiet_NaN();
      return true; // ok 0x7FF and 0xFFF are nan
    }
    return false;
  }
  exp -= 0x3ff;
  res = std::ldexp(1.+mantisse, exp);
  if (sign == -1)
    res *= -1.;
  return true;
}

bool STOFFInputStream::readDouble10(double &res, bool &isNotANumber)
{
  if (!m_stream) return false;
  long pos=tell();
  if (pos+10 > m_streamSize) return false;

  auto exp = int(readULong(2));
  int sign = 1;
  if (exp & 0x8000) {
    exp &= 0x7fff;
    sign = -1;
  }
  exp -= 0x3fff;

  isNotANumber=false;
  auto mantisse = static_cast<unsigned long>(readULong(4));
  if ((mantisse & 0x80000001) == 0) {
    // unormalized number are not frequent, but can appear at least for date, ...
    if (readULong(4) != 0)
      seek(-4, librevenge::RVNG_SEEK_CUR);
    else {
      if (exp == -0x3fff && mantisse == 0) {
        res=0;
        return true; // ok zero
      }
      if (exp == 0x4000 && (mantisse & 0xFFFFFFL)==0) { // ok Nan
        isNotANumber = true;
        res=std::numeric_limits<double>::quiet_NaN();
        return true;
      }
      return false;
    }
  }
  // or std::ldexp((total value)/double(0x80000000), exp);
  res=std::ldexp(double(readULong(4)),exp-63)+std::ldexp(double(mantisse),exp-31);
  if (sign == -1)
    res *= -1.;
  return true;
}

bool STOFFInputStream::readDoubleReverted8(double &res, bool &isNotANumber)
{
  if (!m_stream) return false;
  long pos=tell();
  if (pos+8 > m_streamSize) return false;

  isNotANumber=false;
  res=0;
  int bytes[6];
  for (int &byte : bytes) byte=int(readULong(1));

  auto val=int(readULong(1));
  auto mantExp=int(readULong(1));
  int exp=(mantExp<<4)+(val>>4);
  double mantisse=double(val&0xF)/16.;
  double factor=1./16./256.;
  for (int j = 0; j < 6; ++j, factor/=256)
    mantisse+=double(bytes[5-j])*factor;
  int sign = 1;
  if (exp & 0x800) {
    exp &= 0x7ff;
    sign = -1;
  }
  if (exp == 0) {
    if (mantisse <= 1.e-5 || mantisse >= 1-1.e-5)
      return true;
    // a Nan representation ?
    return false;
  }
  if (exp == 0x7FF) {
    if (mantisse >= 1.-1e-5) {
      isNotANumber=true;
      res=std::numeric_limits<double>::quiet_NaN();
      return true; // ok 0x7FF and 0xFFF are nan
    }
    return false;
  }
  exp -= 0x3ff;
  res = std::ldexp(1.+mantisse, exp);
  if (sign == -1)
    res *= -1.;
  return true;
}

////////////////////////////////////////////////////////////
//
// OLE part
//
////////////////////////////////////////////////////////////

bool STOFFInputStream::isStructured()
{
  if (!m_stream) return false;
  long pos=m_stream->tell();
  bool ok=m_stream->isStructured();
  m_stream->seek(pos, librevenge::RVNG_SEEK_SET);
  return ok;
}

unsigned STOFFInputStream::subStreamCount()
{
  if (!m_stream || !m_stream->isStructured()) {
    STOFF_DEBUG_MSG(("STOFFInputStream::subStreamCount: called on unstructured file\n"));
    return 0;
  }
  return m_stream->subStreamCount();
}

std::string STOFFInputStream::subStreamName(unsigned id)
{
  if (!m_stream || !m_stream->isStructured()) {
    STOFF_DEBUG_MSG(("STOFFInputStream::subStreamName: called on unstructured file\n"));
    return std::string("");
  }
  auto const *nm=m_stream->subStreamName(id);
  if (!nm) {
    STOFF_DEBUG_MSG(("STOFFInputStream::subStreamName: can not find stream %d\n", int(id)));
    return std::string("");
  }
  return std::string(nm);
}

std::shared_ptr<STOFFInputStream> STOFFInputStream::getSubStreamByName(std::string const &name)
{
  std::shared_ptr<STOFFInputStream> empty;
  if (!m_stream || !m_stream->isStructured() || name.empty()) {
    STOFF_DEBUG_MSG(("STOFFInputStream::getSubStreamByName: called on unstructured file\n"));
    return empty;
  }

  long actPos = tell();
  seek(0, librevenge::RVNG_SEEK_SET);
  std::shared_ptr<librevenge::RVNGInputStream> res(m_stream->getSubStreamByName(name.c_str()));
  seek(actPos, librevenge::RVNG_SEEK_SET);

  if (!res)
    return empty;
  std::shared_ptr<STOFFInputStream> inp(new STOFFInputStream(res,m_inverseRead));
  inp->seek(0, librevenge::RVNG_SEEK_SET);
  return inp;
}

std::shared_ptr<STOFFInputStream> STOFFInputStream::getSubStreamById(unsigned id)
{
  std::shared_ptr<STOFFInputStream> empty;
  if (!m_stream || !m_stream->isStructured()) {
    STOFF_DEBUG_MSG(("STOFFInputStream::getSubStreamById: called on unstructured file\n"));
    return empty;
  }

  long actPos = tell();
  seek(0, librevenge::RVNG_SEEK_SET);
  std::shared_ptr<librevenge::RVNGInputStream> res(m_stream->getSubStreamById(id));
  seek(actPos, librevenge::RVNG_SEEK_SET);

  if (!res)
    return empty;
  std::shared_ptr<STOFFInputStream> inp(new STOFFInputStream(res,m_inverseRead));
  inp->seek(0, librevenge::RVNG_SEEK_SET);
  return inp;
}

////////////////////////////////////////////////////////////
//
//  a function to read a data block
//
////////////////////////////////////////////////////////////

bool STOFFInputStream::readDataBlock(long sz, librevenge::RVNGBinaryData &data)
{
  if (!hasDataFork()) return false;

  data.clear();
  if (sz < 0) return false;
  if (sz == 0) return true;
  long endPos=tell()+sz;
  if (endPos > size()) return false;

  const unsigned char *readData;
  unsigned long sizeRead;
  if ((readData=m_stream->read(static_cast<unsigned long>(sz), sizeRead)) == nullptr || long(sizeRead)!=sz)
    return false;
  data.append(readData, sizeRead);
  return true;
}

bool STOFFInputStream::readEndDataBlock(librevenge::RVNGBinaryData &data)
{
  data.clear();
  if (!hasDataFork()) return false;

  return readDataBlock(size()-tell(), data);
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
