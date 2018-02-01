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

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <string>
#include <sstream>
#include <time.h>

#include <ctype.h>
#include <locale.h>

#include <librevenge-stream/librevenge-stream.h>

#include "libstaroffice_internal.hxx"

/** namespace used to regroup all libwpd functions, enumerations which we have redefined for internal usage */
namespace libstoff
{
uint8_t readU8(librevenge::RVNGInputStream *input)
{
  unsigned long numBytesRead;
  uint8_t const *p = input->read(sizeof(uint8_t), numBytesRead);

  if (!p || numBytesRead != sizeof(uint8_t))
    throw libstoff::FileException();

  return *p;
}

librevenge::RVNGString getString(std::vector<uint32_t> const &unicode)
{
  librevenge::RVNGString res("");
  for (unsigned int i : unicode) {
    if (i<0x20 && i!=0x9 && i!=0xa && i!=0xd) {
      static int numErrors=0;
      if (++numErrors<10) {
        STOFF_DEBUG_MSG(("libstoff::getString: find odd char %x\n", static_cast<unsigned int>(i)));
      }
    }
    else if (i<0x80)
      res.append(char(i));
    else
      appendUnicode(i, res);
  }
  return res;
}

void appendUnicode(uint32_t val, librevenge::RVNGString &buffer)
{
  uint8_t first;
  int len;
  if (val < 0x80) {
    first = 0;
    len = 1;
  }
  else if (val < 0x800) {
    first = 0xc0;
    len = 2;
  }
  else if (val < 0x10000) {
    first = 0xe0;
    len = 3;
  }
  else if (val < 0x200000) {
    first = 0xf0;
    len = 4;
  }
  else if (val < 0x4000000) {
    first = 0xf8;
    len = 5;
  }
  else {
    first = 0xfc;
    len = 6;
  }

  char outbuf[7];
  int i;
  for (i = len - 1; i > 0; --i) {
    outbuf[i] = char((val & 0x3f) | 0x80);
    val >>= 6;
  }
  outbuf[0] = char(val | first);
  outbuf[len] = 0;
  buffer.append(outbuf);
}
}

namespace libstoff
{
std::string numberingTypeToString(NumberingType type)
{
  switch (type) {
  case ARABIC:
    return "1";
  case LOWERCASE:
    return "a";
  case UPPERCASE:
    return "A";
  case LOWERCASE_ROMAN:
    return "i";
  case UPPERCASE_ROMAN:
    return "I";
  case NONE:
  case BULLET:
  default:
    break;
  }
  STOFF_DEBUG_MSG(("libstoff::numberingTypeToString: must not be called with type %d\n", int(type)));
  return "1";
}

std::string numberingValueToString(NumberingType type, int value)
{
  std::stringstream ss;
  std::string s("");
  switch (type) {
  case ARABIC:
    ss << value;
    return ss.str();
  case LOWERCASE:
  case UPPERCASE:
    if (value <= 0) {
      STOFF_DEBUG_MSG(("libstoff::numberingValueToString: value can not be negative or null for type %d\n", int(type)));
      return (type == LOWERCASE) ? "a" : "A";
    }
    while (value > 0) {
      s = char((type == LOWERCASE ? 'a' : 'A')+((value-1)%26))+s;
      value = (value-1)/26;
    }
    return s;
  case LOWERCASE_ROMAN:
  case UPPERCASE_ROMAN: {
    static char const *romanS[] = {"M", "CM", "D", "CD", "C", "XC", "L",
                                   "XL", "X", "IX", "V", "IV", "I"
                                  };
    static char const *romans[] = {"m", "cm", "d", "cd", "c", "xc", "l",
                                   "xl", "x", "ix", "v", "iv", "i"
                                  };
    static int const romanV[] = {1000, 900, 500, 400,  100, 90, 50,
                                 40, 10, 9, 5, 4, 1
                                };
    if (value <= 0 || value >= 4000) {
      STOFF_DEBUG_MSG(("libstoff::numberingValueToString: out of range value for type %d\n", int(type)));
      return (type == LOWERCASE_ROMAN) ? "i" : "I";
    }
    for (int p = 0; p < 13; p++) {
      while (value >= romanV[p]) {
        ss << ((type == LOWERCASE_ROMAN) ? romans[p] : romanS[p]);
        value -= romanV[p];
      }
    }
    return ss.str();
  }
  case NONE:
    return "";
  case BULLET:
  default:
    STOFF_DEBUG_MSG(("libstoff::numberingValueToString: must not be called with type %d\n", int(type)));
    break;
  }
  return "";
}
}

// color function
STOFFColor STOFFColor::barycenter(float alpha, STOFFColor const &colA,
                                  float beta, STOFFColor const &colB)
{
  uint32_t res = 0;
  for (int i=0, depl=0; i<4; i++, depl+=8) {
    float val=alpha*float((colA.m_value>>depl)&0xFF)+beta*float((colB.m_value>>depl)&0xFF);
    if (val < 0) val=0;
    if (val > 256) val=256;
    auto comp= static_cast<unsigned char>(val);
    res+=uint32_t(comp<<depl);
  }
  return STOFFColor(res);
}

std::ostream &operator<< (std::ostream &o, STOFFColor const &c)
{
  const std::streamsize width = o.width();
  const char fill = o.fill();
  o << "#" << std::hex << std::setfill('0') << std::setw(6)
    << (c.m_value&0xFFFFFF)
    // std::ios::width() takes/returns std::streamsize (long), but
    // std::setw() takes int. Go figure...
    << std::dec << std::setfill(fill) << std::setw(static_cast<int>(width));
  return o;
}

std::string STOFFColor::str() const
{
  std::stringstream stream;
  stream << *this;
  return stream.str();
}

// field function
void STOFFField::addTo(librevenge::RVNGPropertyList &pList) const
{
  librevenge::RVNGPropertyList::Iter i(m_propertyList);
  for (i.rewind(); i.next();) {
    if (i.child()) {
      pList.insert(i.key(), *i.child());
      continue;
    }
    pList.insert(i.key(), i()->clone());
  }
}

// link function
bool STOFFLink::addTo(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("xlink:type","simple");
  if (!m_HRef.empty())
    propList.insert("xlink:href",m_HRef.c_str());
  return true;
}

// border function
bool STOFFBorderLine::addTo(librevenge::RVNGPropertyList &propList, std::string const which) const
{
  std::stringstream stream, field;
  field << "fo:border";
  if (which.length())
    field << "-" << which;
  if (isEmpty()) {
    propList.insert(field.str().c_str(), "none");
    return true;
  }
  stream << float(m_inWidth+m_distance+m_outWidth)/20.f << "pt ";
  stream << ((m_inWidth && m_outWidth) ? "double" : "solid") << " " << m_color;
  propList.insert(field.str().c_str(), stream.str().c_str());

  if (m_inWidth && m_outWidth) {
    field.str("");
    field << "style:border-line-width";
    if (which.length())
      field << "-" << which;
    stream.str("");
    stream << float(m_inWidth)/20.f << "pt " << float(m_distance)/20.f << "pt " << float(m_outWidth)/20.f << "pt";
    propList.insert(field.str().c_str(), stream.str().c_str());
  }
  return true;
}

std::ostream &operator<< (std::ostream &o, STOFFBorderLine const &border)
{
  if (border.m_outWidth) o << "width[out]=" << border.m_outWidth << ":";
  if (border.m_inWidth) o << "width[in]=" << border.m_inWidth << ":";
  if (border.m_distance) o << "distance=" << border.m_distance << ":";
  if (!border.m_color.isBlack()) o << "col=" << border.m_color << ":";
  o << ",";
  return o;
}

// picture function
STOFFEmbeddedObject::~STOFFEmbeddedObject()
{
}

bool STOFFEmbeddedObject::addAsFillImageTo(librevenge::RVNGPropertyList &propList) const
{
  for (size_t i=0; i<m_dataList.size(); ++i) {
    if (m_dataList[i].empty()) continue;
    std::string type=m_typeList.size() ? m_typeList[i] : "image/pict";
    propList.insert("librevenge:mime-type", type.c_str());
    propList.insert("draw:fill-image", m_dataList[i].getBase64Data());
    return true;
  }
  return false;
}

bool STOFFEmbeddedObject::addTo(librevenge::RVNGPropertyList &propList) const
{
  bool firstSet=false;
  librevenge::RVNGPropertyListVector auxiliarVector;
  for (size_t i=0; i<m_dataList.size(); ++i) {
    if (m_dataList[i].empty()) continue;
    std::string type=m_typeList.size() ? m_typeList[i] : "image/pict";
    if (!firstSet) {
      propList.insert("librevenge:mime-type", type.c_str());
      propList.insert("office:binary-data", m_dataList[i]);
      firstSet=true;
      continue;
    }
    librevenge::RVNGPropertyList auxiList;
    auxiList.insert("librevenge:mime-type", type.c_str());
    auxiList.insert("office:binary-data", m_dataList[i]);
    auxiliarVector.append(auxiList);
  }
  if (!m_filenameLink.empty()) {
    if (!firstSet) {
      propList.insert("librevenge:xlink", m_filenameLink);
      firstSet=true;
    }
    else {
      librevenge::RVNGPropertyList auxiList;
      auxiList.insert("librevenge:xlink", m_filenameLink);
      auxiliarVector.append(auxiList);
    }
  }
  if (!auxiliarVector.empty())
    propList.insert("librevenge:replacement-objects", auxiliarVector);
  if (!firstSet) {
    STOFF_DEBUG_MSG(("STOFFEmbeddedObject::addTo: called without picture\n"));
    return false;
  }
  return true;
}

int STOFFEmbeddedObject::cmp(STOFFEmbeddedObject const &pict) const
{
  if (m_typeList.size()!=pict.m_typeList.size())
    return m_typeList.size()<pict.m_typeList.size() ? -1 : 1;
  for (size_t i=0; i<m_typeList.size(); ++i) {
    if (m_typeList[i]<pict.m_typeList[i]) return -1;
    if (m_typeList[i]>pict.m_typeList[i]) return 1;
  }
  if (m_dataList.size()!=pict.m_dataList.size())
    return m_dataList.size()<pict.m_dataList.size() ? -1 : 1;
  for (size_t i=0; i<m_dataList.size(); ++i) {
    if (m_dataList[i].size() < pict.m_dataList[i].size()) return 1;
    if (m_dataList[i].size() > pict.m_dataList[i].size()) return -1;

    auto const *ptr=m_dataList[i].getDataBuffer();
    auto const *aPtr=pict.m_dataList[i].getDataBuffer();
    if (!ptr || !aPtr) continue; // must only appear if the two buffers are empty
    for (unsigned long h=0; h < m_dataList[i].size(); ++h, ++ptr, ++aPtr) {
      if (*ptr < *aPtr) return 1;
      if (*ptr > *aPtr) return -1;
    }
  }
  return 0;
}

std::ostream &operator<<(std::ostream &o, STOFFEmbeddedObject const &pict)
{
  if (pict.isEmpty()) return o;
  o << "[";
  for (auto const &t : pict.m_typeList) {
    if (t.empty())
      o << "_,";
    else
      o << t << ",";
  }
  o << "],";
  return o;
}

namespace libstoff
{
// some date conversion
bool convertToDateTime(uint32_t date, uint32_t time, std::string &dateTime)
{
  std::stringstream s;
  s << std::setfill('0') << std::setw(8) << date;
  dateTime=s.str();
  if (dateTime.length()!=8) {
    STOFF_DEBUG_MSG(("libstoff:convertToDateTime: can not convert the date"));
    return false;
  }
  s.str("");
  s << std::setfill('0') << std::setw(8) << time;
  if (s.str().length()!=8) {
    STOFF_DEBUG_MSG(("libstoff:convertToDateTime: can not convert the time"));
    return false;
  }
  dateTime+=s.str();
  dateTime.insert(14,1,'.');
  dateTime.insert(12,1,':');
  dateTime.insert(10,1,':');
  dateTime.insert(8,1,'T');
  dateTime.insert(6,1,'-');
  dateTime.insert(4,1,'-');
  return true;
}

void splitString(librevenge::RVNGString const &string, librevenge::RVNGString const &delim,
                 librevenge::RVNGString &string1, librevenge::RVNGString &string2)
{
  std::string fString(string.cstr());
  std::string fDelim(delim.cstr());
  size_t pos=fString.find(fDelim);
  if (pos!=std::string::npos) {
    if (pos+fDelim.length()<fString.length())
      string2=librevenge::RVNGString(fString.substr(pos+fDelim.length()).c_str());
    if (pos>0)
      string1=librevenge::RVNGString(fString.substr(0,pos).c_str());
  }
  else
    string1=string;
}

librevenge::RVNGString simplifyString(librevenge::RVNGString const &s)
{
  librevenge::RVNGString res("");
  char const *ptr=s.cstr();
  if (!ptr) return res;
  int numBad=0;
  while (*ptr) {
    char c=*(ptr++);
    if (unsigned(c)<0x80) {
      if (numBad) {
        numBad=0;
        res.append('@');
      }
      res.append(c);
      continue;
    }
    if (numBad++>=4) {
      res.append('@');
      numBad=0;
    }
  }
  if (numBad) res.append('@');
  return res;
}

std::string getCellName(STOFFVec2i const &cellPos, STOFFVec2b const &relative)
{
  if (cellPos[0]<0 || cellPos[0]>=26*26*26 || cellPos[1]<0) {
    STOFF_DEBUG_MSG(("libwps::getCellName: invalid cell position\n"));
    return "";
  }
  std::stringstream o;
  if (!relative[0]) o << "$";
  if (cellPos[0]>=26*26) o << (char)(cellPos[0]/26/26-1 + 'A');
  if (cellPos[0]>=26) o << (char)((cellPos[0]%(26*26))/26-1 + 'A');
  o << (char)(cellPos[0]%26+'A');
  if (!relative[1]) o << "$";
  o << cellPos[1]+1;
  return o.str();
}
// a little geometry
STOFFVec2f rotatePointAroundCenter(STOFFVec2f const &point, STOFFVec2f const &center, float angle)
{
  float angl=float(M_PI/180.)*angle;
  STOFFVec2f pt = point-center;
  return center + STOFFVec2f(std::cos(angl)*pt[0]-std::sin(angl)*pt[1],
                             std::sin(angl)*pt[0]+std::cos(angl)*pt[1]);
}

STOFFBox2f rotateBoxFromCenter(STOFFBox2f const &box, float angle)
{
  STOFFVec2f center=box.center();
  STOFFVec2f minPt, maxPt;
  for (int p=0; p<4; ++p) {
    STOFFVec2f pt=rotatePointAroundCenter(STOFFVec2f(box[p<2?0:1][0],box[(p%2)?0:1][1]), center, angle);
    if (p==0) {
      minPt=maxPt=pt;
      continue;
    }
    for (int c=0; c<2; ++c) {
      if (pt[c]<minPt[c])
        minPt[c]=pt[c];
      else if (pt[c]>maxPt[c])
        maxPt[c]=pt[c];
    }
  }
  return STOFFBox2f(minPt,maxPt);
}

float getScaleFactor(librevenge::RVNGUnit orig, librevenge::RVNGUnit dest)
{
  float actSc = 1.0, newSc = 1.0;
  switch (orig) {
  case librevenge::RVNG_TWIP:
    break;
  case librevenge::RVNG_POINT:
    actSc=20;
    break;
  case librevenge::RVNG_INCH:
    actSc = 1440;
    break;
  case librevenge::RVNG_PERCENT:
  case librevenge::RVNG_GENERIC:
  case librevenge::RVNG_UNIT_ERROR:
  default:
    STOFF_DEBUG_MSG(("libstoff::getScaleFactor %d unit must not appear\n", int(orig)));
  }
  switch (dest) {
  case librevenge::RVNG_TWIP:
    break;
  case librevenge::RVNG_POINT:
    newSc=20;
    break;
  case librevenge::RVNG_INCH:
    newSc = 1440;
    break;
  case librevenge::RVNG_PERCENT:
  case librevenge::RVNG_GENERIC:
  case librevenge::RVNG_UNIT_ERROR:
  default:
    STOFF_DEBUG_MSG(("libstoff::getScaleFactor %d unit must not appear\n", int(dest)));
  }
  return actSc/newSc;
}

// debug message
#ifdef DEBUG
void printDebugMsg(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
}
#endif
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
