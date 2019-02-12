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

#include <math.h>
#include <sstream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "StarBitmap.hxx"
#include "StarFileManager.hxx"
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
  buffer[position++] = static_cast<unsigned char>(value & 0xFF);
  buffer[position++] = static_cast<unsigned char>((value >> 8) & 0xFF);
}

static void writeU32(unsigned char *buffer, unsigned &position, const unsigned value)
{
  buffer[position++] = static_cast<unsigned char>(value & 0xFF);
  buffer[position++] = static_cast<unsigned char>((value >> 8) & 0xFF);
  buffer[position++] = static_cast<unsigned char>((value >> 16) & 0xFF);
  buffer[position++] = static_cast<unsigned char>((value >> 24) & 0xFF);
}

//! Internal: helper function to create a BMP for a color bitmap (freely inspired from libpwg::WPGBitmap.cpp)
static std::unique_ptr<unsigned char[]> createAndInitBMPData(STOFFVec2i const &sz, unsigned &dibFileSize, unsigned &bufferPosition)
{
  if (sz[0]*sz[1]<=0) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::createAndInitBMPData: the image size seems bad\n"));
    return nullptr;
  }
  auto tmpPixelSize = unsigned(sz[0]*sz[1]);
  unsigned tmpDIBImageSize = tmpPixelSize * 4;
  if (tmpPixelSize > tmpDIBImageSize) // overflow !!!
    return nullptr;

  unsigned const headerSize=56;
  unsigned tmpDIBOffsetBits = 14 + headerSize;
  dibFileSize = tmpDIBOffsetBits + tmpDIBImageSize;
  if (tmpDIBImageSize > dibFileSize) // overflow !!!
    return nullptr;

  std::unique_ptr<unsigned char[]> tmpDIBBuffer{new unsigned char[dibFileSize]};
  if (!tmpDIBBuffer) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::createAndInitBMPData: fail to allocated the data buffer\n"));
    return nullptr;
  }
  bufferPosition = 0;
  // Create DIB file header
  writeU16(tmpDIBBuffer.get(), bufferPosition, 0x4D42);  // Type
  writeU32(tmpDIBBuffer.get(), bufferPosition, unsigned(dibFileSize)); // Size
  writeU16(tmpDIBBuffer.get(), bufferPosition, 0); // Reserved1
  writeU16(tmpDIBBuffer.get(), bufferPosition, 0); // Reserved2
  writeU32(tmpDIBBuffer.get(), bufferPosition, unsigned(tmpDIBOffsetBits)); // OffsetBits

  // Create DIB Info header
  writeU32(tmpDIBBuffer.get(), bufferPosition, headerSize); // Size
  writeU32(tmpDIBBuffer.get(), bufferPosition, unsigned(sz[0]));  // Width
  writeU32(tmpDIBBuffer.get(), bufferPosition, unsigned(sz[1])); // Height
  writeU16(tmpDIBBuffer.get(), bufferPosition, 1); // Planes
  writeU16(tmpDIBBuffer.get(), bufferPosition, 32); // BitCount
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0); // Compression
  writeU32(tmpDIBBuffer.get(), bufferPosition, unsigned(tmpDIBImageSize)); // SizeImage
  writeU32(tmpDIBBuffer.get(), bufferPosition, 5904); // XPelsPerMeter: 300ppi
  writeU32(tmpDIBBuffer.get(), bufferPosition, 5904); // YPelsPerMeter: 300ppi
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0); // ColorsUsed
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0); // ColorsImportant

  // Create DIB V3 Info header

  /* this is needed to create alpha picture ; but as both LibreOffice/OpenOffice ignore the alpha channel... */
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0x00FF0000); /* biRedMask */
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0x0000FF00); /* biGreenMask */
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0x000000FF); /* biBlueMask */
  writeU32(tmpDIBBuffer.get(), bufferPosition, 0xFF000000); /* biAlphaMask */

  return tmpDIBBuffer;
}

//! Internal: helper function to create a BMP for a color bitmap (freely inspired from libpwg::WPGBitmap.cpp)
inline bool getBMPData(std::vector<std::vector<STOFFColor> > const &orig, librevenge::RVNGBinaryData &data)
{
  if (orig.empty() || orig[0].empty()) return false;
  STOFFVec2i sz(int(orig[0].size()),int(orig.size()));
  unsigned tmpBufferPosition, tmpDIBFileSize;
  auto tmpDIBBuffer=createAndInitBMPData(sz, tmpDIBFileSize, tmpBufferPosition);
  if (!tmpDIBBuffer) return false;

  // Write DIB Image data
  for (int i = sz[1] - 1; i >= 0 && tmpBufferPosition < tmpDIBFileSize; i--) {
    std::vector<STOFFColor> const &row = orig[size_t(i)];
    for (int j = 0; j < sz[0] && tmpBufferPosition < tmpDIBFileSize; j++) {
      uint32_t col = (j< int(row.size())) ? row[size_t(j)].value() : 0;

      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>(col&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>((col>>8)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>((col>>16)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>((col>>24)&0xFF);
    }
  }
  data.clear();
  data.append(tmpDIBBuffer.get(), tmpDIBFileSize);

  return true;
}
//! Internal: helper function to create a BMP for a color bitmap from a 8*8 patterns, defined with 4 uint16_t
static bool getBMPData(uint16_t const *pattern, STOFFColor const &col0,  STOFFColor const &col1, librevenge::RVNGBinaryData &data)
{
  if (!pattern) return false;
  STOFFVec2i sz(8,8);
  unsigned tmpBufferPosition, tmpDIBFileSize;
  auto tmpDIBBuffer=createAndInitBMPData(sz, tmpDIBFileSize, tmpBufferPosition);
  if (!tmpDIBBuffer) return false;

  uint32_t cols[2]= {col0.value(), col1.value()};
  // Write DIB Image data
  for (int i = 7; i>=0 && tmpBufferPosition < tmpDIBFileSize; i--) {
    uint16_t row=(i%2) ? uint16_t(pattern[i/2]&0xff) : uint16_t(pattern[i/2]>>4);
    for (int j = 0, depl=0x80; j < 8 && tmpBufferPosition < tmpDIBFileSize; j++, depl>>=1) {
      uint32_t const &col = cols[(row&depl) ? 1 : 0];

      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>(col&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>((col>>8)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>((col>>16)&0xFF);
      tmpDIBBuffer[tmpBufferPosition++]=static_cast<unsigned char>((col>>24)&0xFF);
    }
  }
  data.clear();
  data.append(tmpDIBBuffer.get(), tmpDIBFileSize);

  return true;
}

////////////////////////////////////////////////////////////
// utilities to compute bdbox
////////////////////////////////////////////////////////////
static double getInchValue(librevenge::RVNGProperty const *prop)
{
  double value=prop->getDouble();
  switch (prop->getUnit()) {
  case librevenge::RVNG_INCH:
  case librevenge::RVNG_GENERIC: // assume inch
    return value;
  case librevenge::RVNG_POINT:
    return value/72.;
  case librevenge::RVNG_TWIP:
    return value/1440.;
  case librevenge::RVNG_PERCENT:
  case librevenge::RVNG_UNIT_ERROR:
  default: {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarGraphicStruct::getInchValue: call with no double value\n"));
      first=false;
    }
    break;
  }
  }
  return value;
}

static double quadraticExtreme(double t, double a, double b, double c)
{
  return (1.0-t)*(1.0-t)*a + 2.0*(1.0-t)*t*b + t*t*c;
}

static double quadraticDerivative(double a, double b, double c)
{
  double denominator = a - 2.0*b + c;
  if (fabs(denominator)>1e-10*(a-b))
    return (a - b)/denominator;
  return -1.0;
}

static void getQuadraticBezierBBox(double x0, double y0, double x1, double y1, double x, double y,
                                   double &xmin, double &ymin, double &xmax, double &ymax)
{
  xmin = x0 < x ? x0 : x;
  xmax = x0 > x ? x0 : x;
  ymin = y0 < y ? y0 : y;
  ymax = y0 > y ? y0 : y;

  double t = quadraticDerivative(x0, x1, x);
  if (t>=0 && t<=1) {
    double tmpx = quadraticExtreme(t, x0, x1, x);
    xmin = tmpx < xmin ? tmpx : xmin;
    xmax = tmpx > xmax ? tmpx : xmax;
  }

  t = quadraticDerivative(y0, y1, y);
  if (t>=0 && t<=1) {
    double tmpy = quadraticExtreme(t, y0, y1, y);
    ymin = tmpy < ymin ? tmpy : ymin;
    ymax = tmpy > ymax ? tmpy : ymax;
  }
}

static double cubicBase(double t, double a, double b, double c, double d)
{
  return (1.0-t)*(1.0-t)*(1.0-t)*a + 3.0*(1.0-t)*(1.0-t)*t*b + 3.0*(1.0-t)*t*t*c + t*t*t*d;
}

static void getCubicBezierBBox(double x0, double y0, double x1, double y1, double x2, double y2, double x, double y,
                               double &xmin, double &ymin, double &xmax, double &ymax)
{
  xmin = x0 < x ? x0 : x;
  xmax = x0 > x ? x0 : x;
  ymin = y0 < y ? y0 : y;
  ymax = y0 > y ? y0 : y;

  for (int i=0; i<=100; ++i) {
    double t=double(i)/100.;
    double tmpx = cubicBase(t, x0, x1, x2, x);
    xmin = tmpx < xmin ? tmpx : xmin;
    xmax = tmpx > xmax ? tmpx : xmax;
    double tmpy = cubicBase(t, y0, y1, y2, y);
    ymin = tmpy < ymin ? tmpy : ymin;
    ymax = tmpy > ymax ? tmpy : ymax;
  }
}

//! Internal: helper to compute a path bdbox
static bool getPathBBox(const librevenge::RVNGPropertyListVector &path, double &px, double &py, double &qx, double &qy)
{
  // This must be a mistake and we do not want to crash lower
  if (!path.count() || !path[0]["librevenge:path-action"] || path[0]["librevenge:path-action"]->getStr() == "Z") {
    STOFF_DEBUG_MSG(("libodfgen:getPathBdBox: get a spurious path\n"));
    return false;
  }

  // try to find the bounding box
  // this is simple convex hull technique, the bounding box might not be
  // accurate but that should be enough for this purpose
  bool isFirstPoint = true;

  double lastX = 0.0;
  double lastY = 0.0;
  double lastPrevX = 0.0;
  double lastPrevY = 0.0;
  px = py = qx = qy = 0.0;

  for (unsigned long k = 0; k < path.count(); ++k) {
    if (!path[k]["librevenge:path-action"])
      continue;
    std::string action=path[k]["librevenge:path-action"]->getStr().cstr();
    if (action.length()!=1 || action[0]=='Z') continue;

    bool coordOk=path[k]["svg:x"]&&path[k]["svg:y"];
    bool coord1Ok=coordOk && path[k]["svg:x1"]&&path[k]["svg:y1"];
    bool coord2Ok=coord1Ok && path[k]["svg:x2"]&&path[k]["svg:y2"];
    double x=lastX, y=lastY;
    if (isFirstPoint) {
      if (!coordOk) {
        STOFF_DEBUG_MSG(("StarGraphicStruct::getPathBBox: the first point has no coordinate\n"));
        continue;
      }
      qx = px = x = getInchValue(path[k]["svg:x"]);
      qy = py = y = getInchValue(path[k]["svg:y"]);
      lastPrevX = lastX = px;
      lastPrevY = lastY = py;
      isFirstPoint = false;
    }
    else {
      if (path[k]["svg:x"]) x=getInchValue(path[k]["svg:x"]);
      if (path[k]["svg:y"]) y=getInchValue(path[k]["svg:y"]);
      px = (px > x) ? x : px;
      py = (py > y) ? y : py;
      qx = (qx < x) ? x : qx;
      qy = (qy < y) ? y : qy;
    }

    double xmin=px, xmax=qx, ymin=py, ymax=qy;
    bool lastPrevSet=false;

    if (action[0] == 'C' && coord2Ok) {
      getCubicBezierBBox(lastX, lastY, getInchValue(path[k]["svg:x1"]), getInchValue(path[k]["svg:y1"]),
                         getInchValue(path[k]["svg:x2"]), getInchValue(path[k]["svg:y2"]),
                         x, y, xmin, ymin, xmax, ymax);
      lastPrevSet=true;
      lastPrevX=2*x-getInchValue(path[k]["svg:x2"]);
      lastPrevY=2*y-getInchValue(path[k]["svg:y2"]);
    }
    else if (action[0] == 'S' && coord1Ok) {
      getCubicBezierBBox(lastX, lastY, lastPrevX, lastPrevY,
                         getInchValue(path[k]["svg:x1"]), getInchValue(path[k]["svg:y1"]),
                         x, y, xmin, ymin, xmax, ymax);
      lastPrevSet=true;
      lastPrevX=2*x-getInchValue(path[k]["svg:x1"]);
      lastPrevY=2*y-getInchValue(path[k]["svg:y1"]);
    }
    else if (action[0] == 'Q' && coord1Ok) {
      getQuadraticBezierBBox(lastX, lastY, getInchValue(path[k]["svg:x1"]), getInchValue(path[k]["svg:y1"]),
                             x, y, xmin, ymin, xmax, ymax);
      lastPrevSet=true;
      lastPrevX=2*x-getInchValue(path[k]["svg:x1"]);
      lastPrevY=2*y-getInchValue(path[k]["svg:y1"]);
    }
    else if (action[0] == 'T' && coordOk) {
      getQuadraticBezierBBox(lastX, lastY, lastPrevX, lastPrevY,
                             x, y, xmin, ymin, xmax, ymax);
      lastPrevSet=true;
      lastPrevX=2*x-lastPrevX;
      lastPrevY=2*y-lastPrevY;
    }
    else if (action[0] != 'M' && action[0] != 'L' && action[0] != 'H' && action[0] != 'V') {
      STOFF_DEBUG_MSG(("StarGraphicStruct::getPathBBox: problem reading a path\n"));
    }
    px = (px > xmin ? xmin : px);
    py = (py > ymin ? ymin : py);
    qx = (qx < xmax ? xmax : qx);
    qy = (qy < ymax ? ymax : qy);
    lastX = x;
    lastY = y;
    if (!lastPrevSet) {
      lastPrevX=lastX;
      lastPrevY=lastY;
    }
  }
  return true;
}

//! Internal: helper to convert a path in a string
static librevenge::RVNGString convertPath(const librevenge::RVNGPropertyListVector &path)
{
  librevenge::RVNGString sValue("");
  for (unsigned long i = 0; i < path.count(); ++i) {
    if (!path[i]["librevenge:path-action"])
      continue;
    std::string action=path[i]["librevenge:path-action"]->getStr().cstr();
    if (action.length()!=1) continue;
    bool coordOk=path[i]["svg:x"]&&path[i]["svg:y"];
    bool coord1Ok=coordOk && path[i]["svg:x1"]&&path[i]["svg:y1"];
    bool coord2Ok=coord1Ok && path[i]["svg:x2"]&&path[i]["svg:y2"];
    librevenge::RVNGString sElement;
    // 2540 is 2.54*1000, 2.54 in = 1 inch
    if (path[i]["svg:x"] && action[0] == 'H') {
      sElement.sprintf("H%i", int(getInchValue(path[i]["svg:x"])*2540));
      sValue.append(sElement);
    }
    else if (path[i]["svg:y"] && action[0] == 'V') {
      sElement.sprintf("V%i", int(getInchValue(path[i]["svg:y"])*2540));
      sValue.append(sElement);
    }
    else if (coordOk && (action[0] == 'M' || action[0] == 'L' || action[0] == 'T')) {
      sElement.sprintf("%c%i %i", action[0], int(getInchValue(path[i]["svg:x"])*2540),
                       int(getInchValue(path[i]["svg:y"])*2540));
      sValue.append(sElement);
    }
    else if (coord1Ok && (action[0] == 'Q' || action[0] == 'S')) {
      sElement.sprintf("%c%i %i %i %i", action[0], int(getInchValue(path[i]["svg:x1"])*2540),
                       int(getInchValue(path[i]["svg:y1"])*2540), int(getInchValue(path[i]["svg:x"])*2540),
                       int(getInchValue(path[i]["svg:y"])*2540));
      sValue.append(sElement);
    }
    else if (coord2Ok && action[0] == 'C') {
      sElement.sprintf("C%i %i %i %i %i %i", int(getInchValue(path[i]["svg:x1"])*2540),
                       int(getInchValue(path[i]["svg:y1"])*2540), int(getInchValue(path[i]["svg:x2"])*2540),
                       int(getInchValue(path[i]["svg:y2"])*2540), int(getInchValue(path[i]["svg:x"])*2540),
                       int(getInchValue(path[i]["svg:y"])*2540));
      sValue.append(sElement);
    }
    else if (coordOk && path[i]["svg:rx"] && path[i]["svg:ry"] && action[0] == 'A') {
      sElement.sprintf("A%i %i %i %i %i %i %i", int((getInchValue(path[i]["svg:rx"]))*2540),
                       int((getInchValue(path[i]["svg:ry"]))*2540), (path[i]["librevenge:rotate"] ? path[i]["librevenge:rotate"]->getInt() : 0),
                       (path[i]["librevenge:large-arc"] ? path[i]["librevenge:large-arc"]->getInt() : 1),
                       (path[i]["librevenge:sweep"] ? path[i]["librevenge:sweep"]->getInt() : 1),
                       int(getInchValue(path[i]["svg:x"])*2540), int(getInchValue(path[i]["svg:y"])*2540));
      sValue.append(sElement);
    }
    else if (action[0] == 'Z')
      sValue.append(" Z");
  }
  return sValue;
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
  static uint16_t const s_pattern[4*10] = {
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
    char const *wh[] = {"none", "solid", "horizontal", "vertical",
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
    char const *wh[]= {"none", "lt", "mt", "rt", "lm", "mm", "rm", "lb", "mb", "rb", "area", "tile"};
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
  m_style=int(input->readULong(1));
  if (nVers<1) {
    m_position=10; // area
    return true;
  }
  auto doLoad=int(input->readULong(2));
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
  m_position=int(input->readULong(1));
  return true;
}

////////////////////////////////////////////////////////////
//  StarGraphic
////////////////////////////////////////////////////////////
bool StarGraphicStruct::StarGraphic::read(StarZone &zone, long endPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  long lastPos=endPos>0 ? endPos : zone.getRecordLevel()==0 ? input->size() : zone.getRecordLastPosition();
  // impgraph.cxx operator>>(ImpGraphic&)
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SDRGraphic)[" << zone.getRecordLevel() << "]:";
  if (pos+4>lastPos) {
    STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: the zone seems too short\n"));
    f << "###short,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  std::string header;
  for (int i=0; i<4; ++i) header+=char(input->readULong(1));
  bool ok=true;
  if (header=="NAT5") { // NAT5
    if (!zone.openVersionCompatHeader()) {
      STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: can not open version compat header\n"));
      f << "###vCompat,";
      ok=false;
    }
    else {
      // gfxlink.cxx
      f << "native,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      zone.closeVersionCompatHeader("SDRGraphic");

      pos=input->tell();
      f.str("");
      f << "SDRGraphic:native";
      if (!zone.openVersionCompatHeader()) {
        STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: can not open version compat header\n"));
        f << "###vCompat,";
        ok=false;
      }
      long endZone=zone.getRecordLastPosition();
      if (ok && pos+10>endZone) {
        STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: the zone seems to short\n"));
        f << "###link,";
        ok=false;
      }
      long size=0;
      if (ok) {
        f << "type=" << int(input->readULong(2)) << ",";
        size=long(input->readULong(4));
        f << "sz=" << size << ",";
        f << "userId=" << input->readULong(4) << ",";
      }
      if (input->tell()!=endZone && input->tell()!=pos)
        ascFile.addDelimiter(input->tell(),'|');
      input->seek(endZone, librevenge::RVNG_SEEK_SET);
      zone.closeVersionCompatHeader("SDRGraphic");
      if (ok && size>0 && input->tell()+size<=lastPos) {
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        pos=input->tell();
        f.str("");
        f << "SDRGraphic:native";
        librevenge::RVNGBinaryData data;
        if (!input->readDataBlock(size,data)) {
          STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: can not save a Nat5 file\n"));
          input->seek(pos, librevenge::RVNG_SEEK_SET);
        }
        else {
          m_object.add(data, "image/pict");
          ascFile.skipZone(pos,pos+size-1);
          return true;
        }
      }
      else if (ok) {
        STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: the picture size seems bad\n"));
        f << "###size,";
      }
      ok=true;
    }
  }
  else if (header=="SVGD") {
    StarFileManager fileManager;
    input->seek(-4, librevenge::RVNG_SEEK_CUR);
    if (fileManager.readSVGDI(zone)) {
      long actPos=input->tell();
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      librevenge::RVNGBinaryData data;
      if (!input->readDataBlock(actPos-pos,data)) {
        STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: can not save a SVGD file\n"));
        input->seek(actPos, librevenge::RVNG_SEEK_SET);
      }
      else
        m_object.add(data, "image/svg");
      return true;
    }

    STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: can not read a SVGD file\n"));
    f << "##SVGD,";
    ok=false;
  }
  // can also some pict here or a data size[8 bytes]+bitmap ...
  else if (header[0]=='B') {
    input->seek(-4, librevenge::RVNG_SEEK_CUR);
    StarBitmap bitmap;
    librevenge::RVNGBinaryData data;
    std::string type;
    if (!bitmap.readBitmap(zone, true, lastPos, data, type)) {
      if (endPos>0)
        f << "#unknown,";
      else {
        STOFF_DEBUG_MSG(("StarGraphicStruct::StarGraphic::read: unexpected header\n"));
        f << "##header=" << header << ",";
        ok=false;
      }
    }
    else
      m_object.add(data, type);
  }
  else if (endPos>0)
    f << "#unknown,";
  if (endPos>0)
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok;
}

// StarPolygon
void StarPolygon::addToPath(librevenge::RVNGPropertyListVector &path, bool isClosed, double relUnit, STOFFVec2f const &decal) const
{
  librevenge::RVNGPropertyList element;
  for (size_t i=0; i<m_points.size(); ++i) {
    if (m_points[i].m_flags==2 && i+2<m_points.size() && m_points[i].m_flags==2) {
      element.insert("svg:x1",relUnit*(m_points[i].m_point[0]-double(decal[0])), librevenge::RVNG_POINT);
      element.insert("svg:y1",relUnit*(m_points[i].m_point[1]-double(decal[1])), librevenge::RVNG_POINT);
      element.insert("svg:x2",relUnit*(m_points[++i].m_point[0]-double(decal[0])), librevenge::RVNG_POINT);
      element.insert("svg:y2",relUnit*(m_points[i].m_point[1]-double(decal[1])), librevenge::RVNG_POINT);
      element.insert("svg:x",relUnit*(m_points[++i].m_point[0]-double(decal[0])), librevenge::RVNG_POINT);
      element.insert("svg:y",relUnit*(m_points[i].m_point[1]-double(decal[1])), librevenge::RVNG_POINT);
      element.insert("librevenge:path-action", "C");
    }
    else if (m_points[i].m_flags==2 && i+1<m_points.size()) {
      /* unsure, let asume that this means the previous point is symetric,
         but maybe we can also have a Bezier patch */
      element.insert("svg:x1",relUnit*(m_points[i].m_point[0]-double(decal[0])), librevenge::RVNG_POINT);
      element.insert("svg:y1",relUnit*(m_points[i].m_point[1]-double(decal[1])), librevenge::RVNG_POINT);
      element.insert("svg:x",relUnit*(m_points[++i].m_point[0]-double(decal[0])), librevenge::RVNG_POINT);
      element.insert("svg:y",relUnit*(m_points[i].m_point[1]-double(decal[1])), librevenge::RVNG_POINT);
      element.insert("librevenge:path-action", "S");
    }
    else {
      if (m_points[i].m_flags==2) {
        STOFF_DEBUG_MSG(("StarGraphicStruct::StarPolygon::addToPath: find unexpected flags\n"));
      }
      element.insert("svg:x",relUnit*(m_points[i].m_point[0]-double(decal[0])), librevenge::RVNG_POINT);
      element.insert("svg:y",relUnit*(m_points[i].m_point[1]-double(decal[1])), librevenge::RVNG_POINT);
      element.insert("librevenge:path-action", (i==0 ? "M" : "L"));
    }
    path.append(element);
  }
  if (isClosed) {
    element.insert("librevenge:path-action", "Z");
    path.append(element);
  }
}

bool StarPolygon::convert(librevenge::RVNGString &path, librevenge::RVNGString &viewbox, double relUnit, STOFFVec2f const &decal) const
{
  librevenge::RVNGPropertyListVector pathVect;
  addToPath(pathVect, true, relUnit, decal);
  path=convertPath(pathVect);
  double bounds[4];
  if (path.empty() || !getPathBBox(pathVect, bounds[0], bounds[1], bounds[2], bounds[3]))
    return false;
  std::stringstream s;
  s << long(bounds[0]*1440) << " " << long(bounds[1]*1440) << " " << long(bounds[2]*1440) << " " << long(bounds[3]*1440);
  viewbox=s.str().c_str();

  return true;
}

std::ostream &operator<<(std::ostream &o, StarPolygon const &poly)
{
  o << "points=[";
  for (auto const &pt : poly.m_points)
    o << pt << ",";
  o << "],";
  return o;
}

}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
