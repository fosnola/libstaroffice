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

#include <sstream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "StarObject.hxx"
#include "StarZone.hxx"

#include "StarGraphicStruct.hxx"

// tools functions

namespace StarGraphicStruct
{
//
// functions used by getPBMData (freely inspired from libpwg::WPGBitmap.cpp)
//
static void writeU16(unsigned char *buffer, unsigned &position, const unsigned value)
{
  buffer[position++] = (unsigned char)(value & 0xFF);
  buffer[position++] = (unsigned char)((value >> 8) & 0xFF);
}

static void writeU32(unsigned char *buffer, unsigned &position, const unsigned value)
{
  buffer[position++] = (unsigned char)(value & 0xFF);
  buffer[position++] = (unsigned char)((value >> 8) & 0xFF);
  buffer[position++] = (unsigned char)((value >> 16) & 0xFF);
  buffer[position++] = (unsigned char)((value >> 24) & 0xFF);
}

//! Internal: helper function to create a BMP for a color bitmap (freely inspired from libpwg::WPGBitmap.cpp)
static unsigned char *createAndInitBMPData(STOFFVec2i const &sz, unsigned &dibFileSize, unsigned &bufferPosition)
{
  if (sz[0]*sz[1]<=0) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::createAndInitBMPData: the image size seems bad\n"));
    return 0;
  }
  unsigned tmpPixelSize = unsigned(sz[0]*sz[1]);
  unsigned tmpDIBImageSize = tmpPixelSize * 4;
  if (tmpPixelSize > tmpDIBImageSize) // overflow !!!
    return 0;

  unsigned const headerSize=56;
  unsigned tmpDIBOffsetBits = 14 + headerSize;
  dibFileSize = tmpDIBOffsetBits + tmpDIBImageSize;
  if (tmpDIBImageSize > dibFileSize) // overflow !!!
    return 0;

  unsigned char *tmpDIBBuffer = new unsigned char[dibFileSize];
  if (!tmpDIBBuffer) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::createAndInitBMPData: fail to allocated the data buffer\n"));
    return 0;
  }
  bufferPosition = 0;
  // Create DIB file header
  writeU16(tmpDIBBuffer, bufferPosition, 0x4D42);  // Type
  writeU32(tmpDIBBuffer, bufferPosition, (unsigned) dibFileSize); // Size
  writeU16(tmpDIBBuffer, bufferPosition, 0); // Reserved1
  writeU16(tmpDIBBuffer, bufferPosition, 0); // Reserved2
  writeU32(tmpDIBBuffer, bufferPosition, (unsigned) tmpDIBOffsetBits); // OffsetBits

  // Create DIB Info header
  writeU32(tmpDIBBuffer, bufferPosition, headerSize); // Size
  writeU32(tmpDIBBuffer, bufferPosition, (unsigned) sz[0]);  // Width
  writeU32(tmpDIBBuffer, bufferPosition, (unsigned) sz[1]); // Height
  writeU16(tmpDIBBuffer, bufferPosition, 1); // Planes
  writeU16(tmpDIBBuffer, bufferPosition, 32); // BitCount
  writeU32(tmpDIBBuffer, bufferPosition, 0); // Compression
  writeU32(tmpDIBBuffer, bufferPosition, (unsigned)tmpDIBImageSize); // SizeImage
  writeU32(tmpDIBBuffer, bufferPosition, 5904); // XPelsPerMeter: 300ppi
  writeU32(tmpDIBBuffer, bufferPosition, 5904); // YPelsPerMeter: 300ppi
  writeU32(tmpDIBBuffer, bufferPosition, 0); // ColorsUsed
  writeU32(tmpDIBBuffer, bufferPosition, 0); // ColorsImportant

  // Create DIB V3 Info header

  /* this is needed to create alpha picture ; but as both LibreOffice/OpenOffice ignore the alpha channel... */
  writeU32(tmpDIBBuffer, bufferPosition, 0x00FF0000); /* biRedMask */
  writeU32(tmpDIBBuffer, bufferPosition, 0x0000FF00); /* biGreenMask */
  writeU32(tmpDIBBuffer, bufferPosition, 0x000000FF); /* biBlueMask */
  writeU32(tmpDIBBuffer, bufferPosition, 0xFF000000); /* biAlphaMask */

  return tmpDIBBuffer;
}

//! Internal: helper function to create a BMP for a color bitmap (freely inspired from libpwg::WPGBitmap.cpp)
bool getBMPData(std::vector<std::vector<STOFFColor> > const &orig, librevenge::RVNGBinaryData &data)
{
  if (orig.empty() || orig[0].empty()) return false;
  STOFFVec2i sz((int) orig[0].size(),(int) orig.size());
  unsigned tmpBufferPosition, tmpDIBFileSize;
  unsigned char *tmpDIBBuffer=createAndInitBMPData(sz, tmpDIBFileSize, tmpBufferPosition);
  if (!tmpDIBBuffer) return false;

  // Write DIB Image data
  for (int i = sz[1] - 1; i >= 0 && tmpBufferPosition < tmpDIBFileSize; i--) {
    std::vector<STOFFColor> const &row = orig[size_t(i)];
    for (int j = 0; j < sz[0] && tmpBufferPosition < tmpDIBFileSize; j++) {
      uint32_t col = (j< (int) row.size()) ? row[size_t(j)].value() : 0;

      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)(col&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)((col>>8)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)((col>>16)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)((col>>24)&0xFF);
    }
  }
  data.clear();
  data.append(tmpDIBBuffer, tmpDIBFileSize);
  // Cleanup things before returning
  delete [] tmpDIBBuffer;

  return true;
}
//! Internal: helper function to create a BMP for a color bitmap from a 8*8 patterns, defined with 4 uint16_t
static bool getBMPData(uint16_t const *pattern, STOFFColor const &col0,  STOFFColor const &col1, librevenge::RVNGBinaryData &data)
{
  if (!pattern) return false;
  STOFFVec2i sz(8,8);
  unsigned tmpBufferPosition, tmpDIBFileSize;
  unsigned char *tmpDIBBuffer=createAndInitBMPData(sz, tmpDIBFileSize, tmpBufferPosition);
  if (!tmpDIBBuffer) return false;

  uint32_t cols[2]= {col0.value(), col1.value()};
  // Write DIB Image data
  for (int i = 7; i>=0 && tmpBufferPosition < tmpDIBFileSize; i--) {
    uint16_t row=(i%2) ? uint16_t(pattern[i/2]&0xff) : uint16_t(pattern[i/2]>>4);
    for (int j = 0, depl=0x80; j < 8 && tmpBufferPosition < tmpDIBFileSize; j++, depl>>=1) {
      uint32_t const &col = cols[(row&depl) ? 1 : 0];

      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)(col&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)((col>>8)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)((col>>16)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=(unsigned char)((col>>24)&0xFF);
    }
  }
  data.clear();
  data.append(tmpDIBBuffer, tmpDIBFileSize);
  // Cleanup things before returning
  delete [] tmpDIBBuffer;

  return true;
}

bool StarBrush::getColor(STOFFColor &color) const
{
  if (m_style==0 || m_style>=11)
    return false;
  if (m_style==1) {
    color=m_color;
    return true;
  }
  float const s_alpha[]= {1.0f, 0.25f, 0.25f, 0.4375f, 0.5f, 0.25f, 0.25f, 0.25f, 0.5f, 0.75f };
  float alpha=s_alpha[m_style-1];
  color=STOFFColor::barycenter(alpha, m_color, 1.f-alpha, m_fillColor);
  return true;
}

bool StarBrush::getPattern(STOFFEmbeddedObject &object, STOFFVec2i &sz) const
{
  object=STOFFEmbeddedObject();
  if (m_style==0 || m_style>=11)
    return false;
  static uint16_t const(s_pattern[4*10]) = {
    0xffff, 0xffff, 0xffff, 0xffff, // solid
    0xff00, 0x0000, 0xff00, 0x0000, // hori
    0x8888, 0x8888, 0x8888, 0x8888, // vertical
    0xff88, 0x8888, 0xff88, 0x8888, // cross
    0x99cc, 0x6633, 0x99cc, 0x6633, // diag cross
    0x1122, 0x4488, 0x1122, 0x4488, // updiag
    0x8844, 0x2211, 0x8844, 0x2211, // down diag
    0x8822, 0x8822, 0x8822, 0x8822, // brush25
    0xaa55, 0xaa55, 0xaa55, 0xaa55, // brush50
    0x77dd, 0x77dd, 0x77dd, 0x77dd, // brush75
  };
  librevenge::RVNGBinaryData data;
  if (!getBMPData(&(s_pattern[4*(m_style-1)]), m_fillColor, m_color, data) || data.empty())
    return false;
  sz=STOFFVec2i(8,8);
  object.add(data, "image/bmp");
  return true;
}
}

namespace StarGraphicStruct
{
// brush function
std::ostream &operator<<(std::ostream &o, StarBrush const &brush)
{
  if (brush.m_style==0) {
    o << "none," << brush.m_extra;
    return o;
  }
  o << "[";
  if (brush.m_transparency) o << "transparency=" << brush.m_transparency << ",";
  if (!brush.m_color.isWhite()) o << "col=" << brush.m_color << ",";
  if (!brush.m_fillColor.isWhite()) o << "col[fill]=" << brush.m_fillColor << ",";
  if (brush.m_style>0 && brush.m_style<=11) {
    char const *(wh[]) = {"none", "solid", "horizontal", "vertical",
                          "cross", "diagcross", "updiag", "downdiag",
                          "brush25", "brush50", "brush75", "bitmap"
                         };
    o << wh[brush.m_style] << ",";
  }
  else {
    STOFF_DEBUG_MSG(("StarBrush::operator<<: find unknown style\n"));
    o << "##style=" << brush.m_style << ",";
  }
  if (brush.m_position>=0 && brush.m_position<=11) {
    char const *(wh[])= {"none", "lt", "mt", "rt", "lm", "mm", "rm", "lb", "mb", "rb", "area", "tile"};
    o << "pos=" << wh[brush.m_position] << ",";
  }
  else {
    STOFF_DEBUG_MSG(("StarBrush::operator<<: find unknown position\n"));
    o << "##pos=" << brush.m_position << ",";
  }
  if (!brush.m_linkName.empty()) o << "link[name]=" << brush.m_linkName.cstr() << ",";
  if (!brush.m_filterName.empty()) o << "filter[name]=" << brush.m_filterName.cstr() << ",";
  o << brush.m_extra << "]";
  return o;
}

bool StarBrush::read(StarZone &zone, int nVers, long /*endPos*/, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  *this=StarBrush();
  if (!input->readColor(m_color)) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::StarBrush::read: can not read color\n"));
    m_extra="###color,";
    return false;
  }
  if (!input->readColor(m_fillColor)) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::StarBrush::read: can not read fill color\n"));
    m_extra="###fillColor,";
    return false;
  }
  m_style=(int) input->readULong(1);
  if (nVers<1) {
    m_position=10; // area
    return true;
  }
  int doLoad=(int) input->readULong(2);
  if (doLoad & 1) { // TODO: svx_frmitems.cxx:1948
    STOFF_DEBUG_MSG(("StarGraphicStruct::StarBrush::read: do not know how to load graphic\n"));
    return false;
  }
  if (doLoad & 2) {
    std::vector<uint32_t> link;
    if (!zone.readString(link)) {
      STOFF_DEBUG_MSG(("StarGraphicStruct::StarBrush::read: can not find the link\n"));
      m_extra="###link,";
      return false;
    }
    else
      m_linkName=libstoff::getString(link);
  }
  if (doLoad & 4) {
    std::vector<uint32_t> filter;
    if (!zone.readString(filter)) {
      STOFF_DEBUG_MSG(("StarGraphicStruct::StarBrush::read: can not find the filter\n"));
      m_extra="###filter,";
      return false;
    }
    else
      m_filterName=libstoff::getString(filter);
  }
  m_position=(int) input->readULong(1);
  return true;
}


}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
