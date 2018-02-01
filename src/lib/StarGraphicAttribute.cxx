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

#include <iostream>

#include "STOFFCellStyle.hxx"
#include "STOFFFont.hxx"
#include "STOFFGraphicStyle.hxx"
#include "STOFFPageSpan.hxx"
#include "STOFFParagraph.hxx"

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarGraphicStruct.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "StarGraphicAttribute.hxx"

namespace StarGraphicAttribute
{
//! a character bool attribute
class StarGAttributeBool final : public StarAttributeBool
{
public:
  //! constructor
  StarGAttributeBool(Type type, std::string const &debugName, bool value)
    : StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeBool(*this));
  }
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;

protected:
  //! copy constructor
  StarGAttributeBool(StarGAttributeBool const &) = default;
};

//! a character color attribute
class StarGAttributeColor final : public StarAttributeColor
{
public:
  //! constructor
  StarGAttributeColor(Type type, std::string const &debugName, STOFFColor const &value)
    : StarAttributeColor(type, debugName, value)
  {
  }
  //! destructor
  ~StarGAttributeColor() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeColor(*this));
  }
  //! add to a graphic style
  // void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarGAttributeColor(StarGAttributeColor const &) = default;
};

StarGAttributeColor::~StarGAttributeColor()
{
}

//! an integer attribute
class StarGAttributeFraction final : public StarAttribute
{
public:
  //! constructor
  StarGAttributeFraction(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_numerator(0)
    , m_denominator(1)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeFraction(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName;
    if (m_numerator) o << "=" << m_numerator << "/" << m_denominator;
    o << ",";
  }

protected:
  //! copy constructor
  StarGAttributeFraction(StarGAttributeFraction const &) = default;
  // numerator
  int m_numerator;
  // denominator
  int m_denominator;
};

bool StarGAttributeFraction::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_numerator=int(input->readLong(4));
  m_denominator=int(input->readLong(4));

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return pos+8<=endPos;
}

//! a character integer attribute
class StarGAttributeInt final : public StarAttributeInt
{
public:
  //! constructor
  StarGAttributeInt(Type type, std::string const &debugName, int intSize, int value)
    : StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarGAttributeInt(StarGAttributeInt const &) = default;
};

//! a character unsigned integer attribute
class StarGAttributeUInt final : public StarAttributeUInt
{
public:
  //! constructor
  StarGAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value)
    : StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeUInt(*this));
  }
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarGAttributeUInt(StarGAttributeUInt const &) = default;
};

//! a void attribute
class StarGAttributeVoid final : public StarAttributeVoid
{
public:
  //! constructor
  StarGAttributeVoid(Type type, std::string const &debugName)
    : StarAttributeVoid(type, debugName)
  {
  }
  //! destructor
  ~StarGAttributeVoid() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeVoid(*this));
  }
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarGAttributeVoid(StarGAttributeVoid const &) = default;
};

StarGAttributeVoid::~StarGAttributeVoid()
{
}

//! a list of item attribute of StarAttributeInternal
class StarGAttributeItemSet final : public StarAttributeItemSet
{
public:
  //! constructor
  StarGAttributeItemSet(Type type, std::string const &debugName, std::vector<STOFFVec2i> const &limits)
    : StarAttributeItemSet(type, debugName, limits)
  {
  }
  //! destructor
  ~StarGAttributeItemSet() final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(*this));
  }

protected:
  //! copy constructor
  StarGAttributeItemSet(StarGAttributeItemSet const &) = default;
};

StarGAttributeItemSet::~StarGAttributeItemSet()
{
}

void StarGAttributeBool::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_LINESTARTCENTER)
    state.m_graphic.m_propertyList.insert("draw:marker-start-center", m_value);
  else if (m_type==XATTR_LINEENDCENTER)
    state.m_graphic.m_propertyList.insert("draw:marker-end-center", m_value);
  else if (m_type==XATTR_FILLBMP_TILE && m_value)
    state.m_graphic.m_propertyList.insert("style:repeat", "repeat");
  else if (m_type==XATTR_FILLBMP_STRETCH && m_value)
    state.m_graphic.m_propertyList.insert("style:repeat", "stretch");
  else if (m_type==XATTR_FILLBACKGROUND)
    state.m_graphic.m_hasBackground=m_value;
  else if (m_type==StarAttribute::SDRATTR_SHADOW)
    state.m_graphic.m_propertyList.insert("draw:shadow", m_value ? "visible" : "hidden");
  else if (m_type==SDRATTR_TEXT_AUTOGROWHEIGHT)
    state.m_graphic.m_propertyList.insert("draw:auto-grow-height", m_value);
  else if (m_type==SDRATTR_TEXT_AUTOGROWWIDTH)
    state.m_graphic.m_propertyList.insert("draw:auto-grow-width", m_value);
  else if (m_type==SDRATTR_TEXT_ANISTARTINSIDE)
    state.m_graphic.m_propertyList.insert("text:animation-start-inside", m_value);
  else if (m_type==SDRATTR_TEXT_ANISTOPINSIDE)
    state.m_graphic.m_propertyList.insert("text:animation-stop-inside", m_value);
  else if (m_type==SDRATTR_TEXT_CONTOURFRAME) // checkme
    state.m_graphic.m_propertyList.insert("style:wrap-contour", m_value);
  else if (m_type==SDRATTR_OBJMOVEPROTECT)
    state.m_graphic.m_protections[0]=m_value;
  else if (m_type==SDRATTR_OBJSIZEPROTECT)
    state.m_graphic.m_protections[1]=m_value;
  else if (m_type==SDRATTR_OBJPRINTABLE)
    state.m_graphic.m_protections[2]=!m_value;
  else if (m_type==SDRATTR_MEASUREBELOWREFEDGE)
    state.m_graphic.m_propertyList.insert("draw:placing", m_value ? "below" : "above");
  else if (m_type==SDRATTR_MEASURESHOWUNIT)
    state.m_graphic.m_propertyList.insert("draw:show-unit", m_value);
  else if (m_type==SDRATTR_GRAFINVERT)
    state.m_graphic.m_propertyList.insert("draw:color-inversion", m_value);
  // TODO: XATTR_FILLBMP_SIZELOG
}

void StarGAttributeInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_LINEWIDTH)
    state.m_graphic.m_propertyList.insert("svg:stroke-width", state.convertInPoint(m_value),librevenge::RVNG_POINT);
  else if (m_type==XATTR_LINESTARTWIDTH)
    state.m_graphic.m_propertyList.insert("draw:marker-start-width", state.convertInPoint(m_value),librevenge::RVNG_POINT);
  else if (m_type==XATTR_LINEENDWIDTH)
    state.m_graphic.m_propertyList.insert("draw:marker-end-width", state.convertInPoint(m_value),librevenge::RVNG_POINT);
  else if (m_type==XATTR_FILLBMP_SIZEX)
    state.m_graphic.m_propertyList.insert("draw:fill-image-width", state.convertInPoint(m_value),librevenge::RVNG_POINT);
  else if (m_type==XATTR_FILLBMP_SIZEY)
    state.m_graphic.m_propertyList.insert("draw:fill-image-height", state.convertInPoint(m_value),librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_SHADOWXDIST)
    state.m_graphic.m_propertyList.insert("draw:shadow-offset-x", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_SHADOWYDIST)
    state.m_graphic.m_propertyList.insert("draw:shadow-offset-y", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_TEXT_MAXFRAMEHEIGHT)
    state.m_graphic.m_propertyList.insert("fo:max-height", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_TEXT_MINFRAMEHEIGHT) // checkme
    state.m_graphic.m_propertyList.insert("fo:min-height", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_TEXT_MAXFRAMEWIDTH)
    state.m_graphic.m_propertyList.insert("fo:max-width", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_TEXT_MINFRAMEWIDTH) // checkme
    state.m_graphic.m_propertyList.insert("fo:min-width", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_CIRCSTARTANGLE)
    state.m_graphic.m_propertyList.insert("draw:start-angle", double(m_value)/100.,  librevenge::RVNG_GENERIC);
  else if (m_type==SDRATTR_CIRCENDANGLE)
    state.m_graphic.m_propertyList.insert("draw:end-angle", double(m_value)/100.,  librevenge::RVNG_GENERIC);
  else if (m_type==SDRATTR_MEASURELINEDIST)
    state.m_graphic.m_propertyList.insert("draw:line-distance", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_MEASUREOVERHANG)
    state.m_graphic.m_propertyList.insert("draw:guide-overhang", state.convertInPoint(m_value), librevenge::RVNG_POINT);
  else if (m_type==SDRATTR_GRAFRED)
    state.m_graphic.m_propertyList.insert("draw:red", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_GRAFGREEN)
    state.m_graphic.m_propertyList.insert("draw:green", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_GRAFBLUE)
    state.m_graphic.m_propertyList.insert("draw:blue", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_GRAFLUMINANCE)
    state.m_graphic.m_propertyList.insert("draw:luminance", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_GRAFCONTRAST)
    state.m_graphic.m_propertyList.insert("draw:contrast", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_ECKENRADIUS)
    state.m_graphic.m_propertyList.insert("draw:corner-radius", state.convertInPoint(m_value),librevenge::RVNG_POINT);
}

void StarGAttributeUInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_LINESTYLE) {
    if (m_value<=2) {
      char const *wh[]= {"none", "solid", "dash"};
      state.m_graphic.m_propertyList.insert("draw:stroke", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown line style %d\n", int(m_value)));
    }
  }
  else if (m_type==XATTR_FILLSTYLE) {
    if (m_value<=4) {
      char const *wh[]= {"none", "solid", "gradient", "hatch", "bitmap"};
      state.m_graphic.m_propertyList.insert("draw:fill", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown fill style %d\n", int(m_value)));
    }
  }
  else if (m_type==XATTR_LINETRANSPARENCE)
    state.m_graphic.m_propertyList.insert("svg:stroke-opacity", 1-double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_FILLTRANSPARENCE)
    state.m_graphic.m_propertyList.insert("draw:opacity", 1-double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_LINEJOINT) {
    if (m_value<=4) {
      char const *wh[]= {"none", "middle", "bevel", "miter", "round"};
      state.m_graphic.m_propertyList.insert("draw:stroke-linejoin", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown line join %d\n", int(m_value)));
    }
  }
  else if (m_type==XATTR_FILLBMP_POS) {
    if (m_value<9) {
      char const *wh[]= {"top-left", "top", "top-right", "left", "center", "right", "bottom-left", "bottom", "bottom-right"};
      state.m_graphic.m_propertyList.insert("draw:fill-image-ref-point", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown bmp position %d\n", int(m_value)));
    }
  }
  // CHECKME: from here never seen
  else if (m_type==XATTR_GRADIENTSTEPCOUNT)
    state.m_graphic.m_propertyList.insert("draw:gradient-step-count", m_value, librevenge::RVNG_GENERIC);
  else if (m_type==XATTR_FILLBMP_POSOFFSETX)
    state.m_graphic.m_propertyList.insert("draw:fill-image-ref-point-x", double(m_value)/100, librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_FILLBMP_POSOFFSETY)
    state.m_graphic.m_propertyList.insert("draw:fill-image-ref-point-y", double(m_value)/100, librevenge::RVNG_PERCENT);
  else if (m_type==XATTR_FILLBMP_TILEOFFSETX || m_type==XATTR_FILLBMP_TILEOFFSETY) {
    std::stringstream s;
    s << m_value << "% " << (m_type==XATTR_FILLBMP_TILEOFFSETX ? "horizontal" : "vertical");
    state.m_graphic.m_propertyList.insert("draw:tile-repeat-offset", s.str().c_str());
  }
  else if (m_type==SDRATTR_SHADOWTRANSPARENCE)
    state.m_graphic.m_propertyList.insert("draw:shadow-opacity", 1.0-double(m_value)/255., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_TEXT_LEFTDIST || m_type==SDRATTR_TEXT_RIGHTDIST ||
           m_type==SDRATTR_TEXT_UPPERDIST || m_type==SDRATTR_TEXT_LOWERDIST) {
    char const *wh[] = {"left", "right", "top", "bottom"};
    state.m_graphic.m_propertyList.insert((std::string("padding-")+wh[(m_type-SDRATTR_TEXT_LEFTDIST)]).c_str(), state.convertInPoint(m_value), librevenge::RVNG_POINT);
  }
  else if (m_type==SDRATTR_TEXT_FITTOSIZE)
    // TODO: 0: none, 1: proportional, allline, autofit
    state.m_graphic.m_propertyList.insert("draw:fit-to-size", m_value!=0);
  else if (m_type==SDRATTR_TEXT_HORZADJUST) {
    if (m_value<4) {
      char const *wh[]= {"left", "center", "right", "justify"};
      state.m_graphic.m_propertyList.insert("draw:textarea-horizontal-align", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown text horizontal position %d\n", int(m_value)));
    }
  }
  else if (m_type==SDRATTR_TEXT_VERTADJUST) {
    if (m_value<4) {
      char const *wh[]= {"top", "middle", "bottom", "justify"};
      state.m_graphic.m_propertyList.insert("draw:textarea-vertical-align", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown text vertical position %d\n", int(m_value)));
    }
  }
  else if (m_type==SDRATTR_TEXT_ANIAMOUNT) // unsure
    state.m_graphic.m_propertyList.insert("text:animation-steps", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_TEXT_ANICOUNT)
    state.m_graphic.m_propertyList.insert("text:animation-repeat", int(m_value));
  else if (m_type==SDRATTR_TEXT_ANIDELAY) { // check unit
    librevenge::RVNGString delay;
    delay.sprintf("PT%fS", double(m_value));
    state.m_graphic.m_propertyList.insert("text:animation-delay", delay);
  }
  else if (m_type==SDRATTR_TEXT_ANIDIRECTION) {
    if (m_value<4) {
      char const *wh[]= {"left", "right", "up", "down"};
      state.m_graphic.m_propertyList.insert("text:animation-direction", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown animation direction %d\n", int(m_value)));
    }
  }
  else if (m_type==SDRATTR_TEXT_ANIKIND) {
    if (m_value<5) {
      char const *wh[]= {"none", "none" /*blink*/, "scroll", "alternate", "slide"};
      state.m_graphic.m_propertyList.insert("text:animation", wh[m_value]);
      if (m_value) {
        if (!state.m_graphic.m_propertyList["text:animation-direction"])
          state.m_graphic.m_propertyList.insert("text:animation-direction", "left");
        if (!state.m_graphic.m_propertyList["text:animation-steps"])
          state.m_graphic.m_propertyList.insert("text:animation-steps", 0.02, librevenge::RVNG_PERCENT);
      }
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown animation direction %d\n", int(m_value)));
    }
  }
  else if (m_type==SDRATTR_CIRCKIND) {
    if (m_value<4) {
      char const *wh[]= {"full", "section", "cut", "arc"};
      state.m_graphic.m_propertyList.insert("draw:kind", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown circle kind %d\n", int(m_value)));
    }
  }
  else if (m_type==SDRATTR_GRAFGAMMA)
    state.m_graphic.m_propertyList.insert("draw:gamma", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_GRAFTRANSPARENCE)
    state.m_graphic.m_propertyList.insert("draw:opacity", 1.0-double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==SDRATTR_GRAFMODE) {
    if (m_value<4) {
      char const *wh[]= {"standard", "greyscale", "mono", "watermark"};
      state.m_graphic.m_propertyList.insert("draw:color-mode", wh[m_value]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeUInt::addTo: unknown graf color mode %d\n", int(m_value)));
    }
  }
}

void StarGAttributeVoid::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==StarAttribute::SDRATTR_SHADOW3D)
    state.m_graphic.m_propertyList.insert("dr3d:shadow", "visible");
  // also SDRATTR_SHADOWPERSP, ok to ignore ?
}

//! add a bool attribute
inline void addAttributeBool(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarGAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarGAttributeColor(type,debugName, defValue));
}
//! add a fraction attribute
inline void addAttributeFraction(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarGAttributeFraction(type,debugName));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarGAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarGAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarGAttributeVoid(type,debugName));
}

}

namespace StarGraphicAttribute
{
// ------------------------------------------------------------

//! a box info attribute
class StarGAttributeBoxInfo final : public StarAttribute
{
public:
  //! constructor
  StarGAttributeBoxInfo(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_distance(0)
    , m_borderList()
    , m_flags(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeBoxInfo(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_distance) o << "dist=" << m_distance << ",";
    for (size_t i=0; i<m_borderList.size(); ++i) {
      if (!m_borderList[i].isEmpty())
        o << "border" << i << "=[" << m_borderList[i] << "],";
    }
    if (m_flags&1) o << "set[table],";
    if (m_flags&2) o << "set[dist],";
    if (m_flags&4) o << "set[minDist],";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeBoxInfo(StarGAttributeBoxInfo const &) = default;
  //! the distance
  int m_distance;
  //! the boxInfo list: top, left, right, bottom
  std::vector<STOFFBorderLine> m_borderList;
  //! some flags: setTable, setDist, setMinDist
  int m_flags;
};

//! a crop attribute
class StarGAttributeCrop final : public StarAttribute
{
public:
  //! constructor
  StarGAttributeCrop(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_leftTop(0,0)
    , m_rightBottom(0,0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeCrop(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[" << m_leftTop << "<->" << m_rightBottom << "],";
  }

protected:
  //! copy constructor
  StarGAttributeCrop(StarGAttributeCrop const &) = default;
  //! the cropping left/top
  STOFFVec2i m_leftTop;
  //! the cropping right/bottom
  STOFFVec2i m_rightBottom;
};

//! a named attribute
class StarGAttributeNamed : public StarAttribute
{
public:
  //! constructor
  StarGAttributeNamed(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_named(), m_namedId(0)
  {
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName << "=[";
    if (!m_named.empty())
      o << m_named.cstr() << ",";
    if (m_namedId>=0)
      o << "id=" << m_namedId << ",";
    o << "]";
  }

protected:
  //! copy constructor
  StarGAttributeNamed(StarGAttributeNamed const &) = default;
  //! the named
  librevenge::RVNGString m_named;
  //! the name id
  int m_namedId;
};

//! a arrow's named attribute
class StarGAttributeNamedArrow final : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedArrow(Type type, std::string const &debugName)
    : StarGAttributeNamed(type, debugName)
    , m_polygon()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeNamedArrow(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    StarGAttributeNamed::printData(o);
    o << ":[" << m_polygon << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedArrow(StarGAttributeNamedArrow const &) = default;
  //! the polygon
  StarGraphicStruct::StarPolygon m_polygon;
};

//! a bitmap's named attribute
class StarGAttributeNamedBitmap final : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedBitmap(Type type, std::string const &debugName)
    : StarGAttributeNamed(type, debugName)
    , m_bitmap()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeNamedBitmap(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    StarGAttributeNamed::printData(o);
    if (m_bitmap.isEmpty()) o << "empty";
    o << ",";
  }

protected:
  //! copy constructor
  StarGAttributeNamedBitmap(StarGAttributeNamedBitmap const &) = default;
  //! the bitmap
  STOFFEmbeddedObject m_bitmap;
};

//! a color's named attribute
class StarGAttributeNamedColor final : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedColor(Type type, std::string const &debugName, STOFFColor const &defColor)
    : StarGAttributeNamed(type, debugName)
    , m_color(defColor)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeNamedColor(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font/graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    StarGAttributeNamed::printData(o);
    o << ":[" << m_color << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedColor(StarGAttributeNamedColor const &) = default;
  //! the color
  STOFFColor m_color;
};

//! a dash's named attribute
class StarGAttributeNamedDash final : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedDash(Type type, std::string const &debugName)
    : StarGAttributeNamed(type, debugName)
    , m_dashStyle(0)
    , m_distance(20)
  {
    for (int &number : m_numbers) number=1;
    for (int &length : m_lengths) length=20;
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeNamedDash(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    StarGAttributeNamed::printData(o);
    o << ":[";
    o << "style=" << m_dashStyle << ",";
    for (int i=0; i<2; ++i) {
      if (m_numbers[i])
        o << (i==0 ? "dots" : "dashes") << "=" << m_numbers[i] << ":" << m_lengths[i] << ",";
    }
    if (m_distance) o << "distance=" << m_distance << ",";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedDash(StarGAttributeNamedDash const &) = default;
  //! the style:  XDASH_RECT, XDASH_ROUND, XDASH_RECTRELATIVE, XDASH_ROUNDRELATIVE
  int m_dashStyle;
  //! the number of dot/dash
  int m_numbers[2];
  //! the lengths of dot/dash
  int m_lengths[2];
  //! the distance
  int m_distance;
};

//! a gradient's named attribute
class StarGAttributeNamedGradient final : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedGradient(Type type, std::string const &debugName)
    : StarGAttributeNamed(type, debugName)
    , m_gradientType(0)
    , m_enable(true)
    , m_angle(0)
    , m_border(0)
    , m_step(0)
  {
    for (int i=0; i<2; ++i) {
      m_offsets[i]=50;
      m_intensities[i]=100;
    }
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeNamedGradient(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    StarGAttributeNamed::printData(o);
    o << ":[";
    o << "type=" << m_gradientType << ",";
    if (m_angle) o << "angle=" << m_angle << ",";
    if (m_border) o << "border=" << m_border << ",";
    if (m_step) o << "step=" << m_step << ",";
    o << m_colors[0] << "->" << m_colors[1] << ",";
    o << "offset=" << m_offsets[0] << "->" << m_offsets[1] << ",";
    o << "intensity=" << m_intensities[0] << "->" << m_intensities[1] << ",";
    if (!m_enable) o << "disable,";
    o << "],";
  }

protected:
  //! copy constructor
  StarGAttributeNamedGradient(StarGAttributeNamedGradient const &) = default;
  //! the gradient type
  int m_gradientType;
  //! a flag to know if the gradient is enable
  bool m_enable;
  //! the angle
  int m_angle;
  //! the border
  int m_border;
  //! the step
  int m_step;
  //! the colors
  STOFFColor m_colors[2];
  //! the x offsets
  int m_offsets[2];
  //! the intensities
  int m_intensities[2];
};

//! a hatch's named attribute
class StarGAttributeNamedHatch final : public StarGAttributeNamed
{
public:
  //! constructor
  StarGAttributeNamedHatch(Type type, std::string const &debugName)
    : StarGAttributeNamed(type, debugName)
    , m_hatchType(0)
    , m_color(STOFFColor::black())
    , m_distance(0)
    , m_angle(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarGAttributeNamedHatch(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a graphic style
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    StarGAttributeNamed::printData(o);
    o << ":[";
    o << "type=" << m_hatchType << ",";
    o << m_color << ",";
    if (m_distance) o << "distance=" << m_distance << ",";
    if (m_angle) o << "angle=" << m_angle << ",";
  }

protected:
  //! copy constructor
  StarGAttributeNamedHatch(StarGAttributeNamedHatch const &) = default;
  //! the type
  int m_hatchType;
  //! the color
  STOFFColor m_color;
  //! the distance
  int m_distance;
  //! the angle
  int m_angle;
};

void StarGAttributeCrop::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==SDRATTR_GRAFCROP) {
    if (m_leftTop==STOFFVec2i(0,0) && m_rightBottom==STOFFVec2i(0,0))
      state.m_graphic.m_propertyList.insert("fo:clip", "auto");
    else {
      librevenge::RVNGString clip;
      clip.sprintf("rect(%fpt,%ftt,%fpt,%fpt)", state.convertInPoint(m_leftTop[1]), state.convertInPoint(m_rightBottom[0]),
                   state.convertInPoint(m_rightBottom[1]), state.convertInPoint(m_leftTop[0]));
      state.m_graphic.m_propertyList.insert("fo:clip", clip);
    }
  }
}

void StarGAttributeNamedArrow::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_LINESTART || m_type==XATTR_LINEEND) {
    char const *pathName=m_type==XATTR_LINESTART ? "draw:marker-start-path" : "draw:marker-end-path";
    char const *viewboxName=m_type==XATTR_LINESTART ? "draw:marker-start-viewbox" : "draw:marker-end-viewbox";
    if (m_polygon.empty()) {
      if (state.m_graphic.m_propertyList[pathName]) state.m_graphic.m_propertyList.remove(pathName);
      if (state.m_graphic.m_propertyList[viewboxName]) state.m_graphic.m_propertyList.remove(viewboxName);
    }
    else {
      librevenge::RVNGString path, viewbox;
      if (m_polygon.convert(path, viewbox, state.m_global->m_relativeUnit, STOFFVec2f(0,0))) {
        state.m_graphic.m_propertyList.insert(pathName, path);
        state.m_graphic.m_propertyList.insert(viewboxName, viewbox);
      }
    }
  }
}

void StarGAttributeNamedBitmap::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_FILLBITMAP) {
    if (!m_bitmap.isEmpty())
      m_bitmap.addAsFillImageTo(state.m_graphic.m_propertyList);
    else {
      STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::addTo: can not find the bitmap\n"));
    }
  }
}

void StarGAttributeNamedColor::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_LINECOLOR)
    state.m_graphic.m_propertyList.insert("svg:stroke-color", m_color.str().c_str());
  else if (m_type==XATTR_FILLCOLOR)
    state.m_graphic.m_propertyList.insert("draw:fill-color", m_color.str().c_str());
  else if (m_type==SDRATTR_SHADOWCOLOR) {
    state.m_graphic.m_propertyList.insert("draw:shadow-color", m_color.str().c_str());
    state.m_font.m_shadowColor=m_color;
  }
}

void StarGAttributeNamedDash::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_LINEDASH) {
    state.m_graphic.m_propertyList.insert("draw:dots1", m_numbers[0]);
    state.m_graphic.m_propertyList.insert("draw:dots1-length", state.convertInPoint(m_lengths[0]), librevenge::RVNG_POINT);
    state.m_graphic.m_propertyList.insert("draw:dots2", m_numbers[1]);
    state.m_graphic.m_propertyList.insert("draw:dots2-length", state.convertInPoint(m_lengths[1]), librevenge::RVNG_POINT);
    state.m_graphic.m_propertyList.insert("draw:distance", state.convertInPoint(m_distance), librevenge::RVNG_POINT);
  }
}

void StarGAttributeNamedGradient::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_FILLGRADIENT) {
    // TODO XATTR_FILLFLOATTRANSPARENCE
    if (!m_enable)
      return;
    if (m_gradientType>=0 && m_gradientType<=5) {
      char const *wh[]= {"linear", "axial", "radial", "ellipsoid", "square", "rectangle"};
      state.m_graphic.m_propertyList.insert("draw:style", wh[m_gradientType]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGAttributeNamedGradient::addTo: unknown hash type %d\n", m_gradientType));
    }
    state.m_graphic.m_propertyList.insert("draw:angle", double(m_angle)/10, librevenge::RVNG_GENERIC);
    state.m_graphic.m_propertyList.insert("draw:border", double(m_border)/100, librevenge::RVNG_PERCENT);
    state.m_graphic.m_propertyList.insert("draw:start-color", m_colors[0].str().c_str());
    state.m_graphic.m_propertyList.insert("librevenge:start-opacity", double(m_intensities[0])/100, librevenge::RVNG_PERCENT);
    state.m_graphic.m_propertyList.insert("draw:end-color", m_colors[1].str().c_str());
    state.m_graphic.m_propertyList.insert("librevenge:end-opacity", double(m_intensities[1])/100, librevenge::RVNG_PERCENT);
    state.m_graphic.m_propertyList.insert("svg:cx", double(m_offsets[0])/100, librevenge::RVNG_PERCENT);
    state.m_graphic.m_propertyList.insert("svg:cy", double(m_offsets[1])/100, librevenge::RVNG_PERCENT);
    // m_step ?
  }
}

void StarGAttributeNamedHatch::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==XATTR_FILLHATCH && m_distance>0) {
    if (m_hatchType>=0 && m_hatchType<3) {
      char const *wh[]= {"single", "double", "triple"};
      state.m_graphic.m_propertyList.insert("draw:style", wh[m_hatchType]);
    }
    else {
      STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeNamedHatch::addTo: unknown hash type %d\n", m_hatchType));
    }
    state.m_graphic.m_propertyList.insert("draw:color", m_color.str().c_str());
    state.m_graphic.m_propertyList.insert("draw:distance", state.convertInPoint(m_distance),librevenge::RVNG_POINT);
    if (m_angle) state.m_graphic.m_propertyList.insert("draw:rotation", double(m_angle)/10, librevenge::RVNG_GENERIC);
  }
}

bool StarGAttributeBoxInfo::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  // frmitem.cxx SvxBoxInfoItem::Create
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_flags=int(input->readULong(1));
  m_distance=int(input->readULong(2));
  bool ok=true;
  while (input->tell()<endPos) {
    auto cLine=int(input->readULong(1));
    if (cLine>1) break;
    STOFFBorderLine border;
    if (!input->readColor(border.m_color)) {
      STOFF_DEBUG_MSG(("StarGraphicAttribute::StarGAttributeBoxInfo::read: can not find a box's color\n"));
      f << "###color,";
      ok=false;
      break;
    }
    border.m_outWidth=int(input->readULong(2));
    border.m_inWidth=int(input->readULong(2));
    border.m_distance=int(input->readULong(2));
    m_borderList.push_back(border);
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeCrop::read(StarZone &zone, int vers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName;
  if (vers!=0) {
    int dim[4]; // TLRB
    for (int &i : dim) i=int(input->readLong(4));
    m_leftTop=STOFFVec2i(dim[1],dim[0]);
    m_rightBottom=STOFFVec2i(dim[2],dim[3]);
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return pos+8<=endPos;
}

bool StarGAttributeNamed::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  std::vector<uint32_t> text;
  if (!zone.readString(text)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamed::read: can not read a string\n"));
    return false;
  }
  m_named=libstoff::getString(text);
  m_namedId=int(input->readLong(4));
  return input->tell()<=endPos;
}

bool StarGAttributeNamedArrow::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeArrowNamed::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  bool ok=true;
  if (m_namedId<0) {
    uint32_t nPoints;
    *input >> nPoints;
    if (uint32_t(endPos-input->tell())/12<nPoints || input->tell()+12*long(nPoints)>endPos) {
      STOFF_DEBUG_MSG(("StarGAttributeArrowNamed::read: bad num point\n"));
      f << "###nPoints=" << nPoints << ",";
      ok=false;
      nPoints=0;
    }
    f << "pts=[";
    m_polygon.m_points.resize(size_t(nPoints));
    for (size_t i=0; i<size_t(nPoints); ++i) {
      int dim[2];
      for (int &j : dim) j=int(input->readLong(4));
      m_polygon.m_points[i].m_point=STOFFVec2i(dim[0],dim[1]);
      m_polygon.m_points[i].m_flags=int(input->readULong(4));
    }
    f << "],";
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamedBitmap::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  bool ok=true;
  if (m_namedId<0) {
    if (nVers==1) {
      int16_t style, type;
      *input >> style >> type;
      if (style) f << "style=" << style << ",";
      if (type) f << "type=" << type << ",";
      if (type==1) {
        if (input->tell()+128+2>endPos) {
          STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: the zone seems too short\n"));
          f << "###short,";
          ok=false;
        }
        else {
          f << "val=[";
          uint32_t values[32];
          for (unsigned int &value : values) {
            *input>>value;
            if (value) f << std::hex << value << std::dec << ",";
            else f << "_,";
          }
          f << "],";
          STOFFColor colors[2];
          for (int i=0; i<2; ++i) {
            if (!input->readColor(colors[i])) {
              STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: can not read a color\n"));
              f << "###color,";
              ok=false;
              break;
            }
            f << "col" << i << "=" << colors[i] << ",";
          }
          if (ok) {
            StarBitmap bitmap(values, colors);
            librevenge::RVNGBinaryData data;
            std::string dType;
            if (bitmap.getData(data, dType))
              m_bitmap.add(data,dType);
          }
        }
      }
      else if (type!=2 && type!=0) {
        STOFF_DEBUG_MSG(("StarGAttributeNamedBitmap::read: unexpected type\n"));
        f << "###type,";
        ok=false;
      }
      if (type!=0 || !ok) {
        printData(f);
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return ok && input->tell()<=endPos;
      }
    }
    StarBitmap bitmap;
    librevenge::RVNGBinaryData data;
    std::string dType;
    if (bitmap.readBitmap(zone, true, endPos, data, dType))
      m_bitmap.add(data,dType);
    else
      ok=false;
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamedColor::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedColor::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  bool ok=true;
  if (m_namedId<0) {
    if (!input->readColor(m_color)) {
      STOFF_DEBUG_MSG(("StarGAttributeNamedColor::read: can not read a color\n"));
      f << "###color,";
      ok=false;
    }
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarGAttributeNamedDash::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedDash::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (m_namedId<0) {
    m_dashStyle=int(input->readULong(4));
    for (int i=0; i<2; ++i) {
      m_numbers[i]=int(input->readULong(2));
      m_lengths[i]=int(input->readULong(4));
    }
    m_distance=int(input->readULong(4));
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarGAttributeNamedGradient::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedGradient::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (m_namedId<0) {
    m_gradientType=int(input->readULong(2));
    uint16_t red, green, blue, red2, green2, blue2;
    *input >> red >> green >> blue >> red2 >> green2 >> blue2;
    m_colors[0]=STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8));
    m_colors[1]=STOFFColor(uint8_t(red2>>8),uint8_t(green2>>8),uint8_t(blue2>>8));
    m_angle=int(input->readULong(4));
    m_border=int(input->readULong(2));
    for (int &offset : m_offsets) offset=int(input->readULong(2));
    for (int &intensity : m_intensities) intensity=int(input->readULong(2));
    if (nVers>=1) m_step=int(input->readULong(2));
    if (m_type==XATTR_FILLFLOATTRANSPARENCE) *input >> m_enable;
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarGAttributeNamedHatch::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (!StarGAttributeNamed::read(zone, nVers, endPos, object)) {
    STOFF_DEBUG_MSG(("StarGAttributeNamedHatch::read: can not read header\n"));
    f << "###header";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (m_namedId<0) {
    // XFillHatchItem::XFillHatchItem svx_xattr.cxx
    m_hatchType=int(input->readULong(2));
    uint16_t red, green, blue;
    *input >> red >> green >> blue;
    m_color=STOFFColor(uint8_t(red>>8),uint8_t(green>>8),uint8_t(blue>>8));
    m_distance=int(input->readLong(4));
    m_angle=int(input->readLong(4));
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

}

namespace StarGraphicAttribute
{
void addInitTo(std::map<int, std::shared_ptr<StarAttribute> > &map)
{
  // --- xattr --- svx_xpool.cxx
  addAttributeUInt(map, StarAttribute::XATTR_LINESTYLE,"line[style]",2,1); // solid
  addAttributeInt(map, StarAttribute::XATTR_LINEWIDTH, "line[width]",4, 0); // metric
  addAttributeInt(map, StarAttribute::XATTR_LINESTARTWIDTH, "line[start,width]",4, 200); // metric
  addAttributeInt(map, StarAttribute::XATTR_LINEENDWIDTH, "line[end,width]",4, 200); // metric
  addAttributeBool(map, StarAttribute::XATTR_LINESTARTCENTER,"line[startCenter]", false);
  addAttributeBool(map, StarAttribute::XATTR_LINEENDCENTER,"line[endCenter]", false);
  addAttributeUInt(map, StarAttribute::XATTR_LINETRANSPARENCE,"line[transparence]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_LINEJOINT,"line[joint]",2,4); // use arc
  addAttributeUInt(map, StarAttribute::XATTR_FILLSTYLE,"fill[style]",2,1); // solid
  addAttributeUInt(map, StarAttribute::XATTR_FILLTRANSPARENCE,"fill[transparence]",2,0);
  addAttributeInt(map, StarAttribute::XATTR_FILLBMP_SIZEX, "fill[bmp,sizeX]",4, 0); // metric
  addAttributeInt(map, StarAttribute::XATTR_FILLBMP_SIZEY, "fill[bmp,sizeY]",4, 0); // metric
  addAttributeBool(map, StarAttribute::XATTR_FILLBMP_TILE,"fill[bmp,tile]", true);
  addAttributeBool(map, StarAttribute::XATTR_FILLBMP_STRETCH,"fill[bmp,stretch]", true);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_POS,"fill[bmp,pos]",2,4); // middle-middle
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_POSOFFSETX,"fill[bmp,pos,offX]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_POSOFFSETY,"fill[bmp,pos,offY]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_TILEOFFSETX,"fill[bmp,tile,offX]",2,0);
  addAttributeUInt(map, StarAttribute::XATTR_FILLBMP_TILEOFFSETY,"fill[bmp,tile,offY]",2,0);
  addAttributeBool(map, StarAttribute::XATTR_FILLBACKGROUND,"fill[background]", false);
  addAttributeUInt(map, StarAttribute::XATTR_GRADIENTSTEPCOUNT,"gradient[stepCount]",2,0);

  addAttributeInt(map, StarAttribute::SDRATTR_ECKENRADIUS, "radius[ecken]",4, 0); // metric
  addAttributeBool(map, StarAttribute::SDRATTR_SHADOW,"shadow", false); // onOff
  addAttributeInt(map, StarAttribute::SDRATTR_SHADOWXDIST, "shadow[xDist]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_SHADOWYDIST, "shadow[yDist]",4, 0); // metric
  addAttributeUInt(map, StarAttribute::SDRATTR_SHADOWTRANSPARENCE, "shadow[transparence]",2,0);
  addAttributeVoid(map, StarAttribute::SDRATTR_SHADOW3D, "shadow[3d]");
  addAttributeVoid(map, StarAttribute::SDRATTR_SHADOWPERSP, "shadow[persp]"); // useme
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_LEFTDIST, "text[left,dist]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_RIGHTDIST, "text[right,dist]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_UPPERDIST, "text[upper,dist]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_LOWERDIST, "text[lower,dist]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_MINFRAMEHEIGHT, "text[min,frameHeight]",4, 0); // metric
  addAttributeBool(map, StarAttribute::SDRATTR_TEXT_AUTOGROWHEIGHT,"text[autoGrow,height]", true); // onOff
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_FITTOSIZE, "text[fitToSize]",2,0); // none
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_VERTADJUST, "text[vert,adjust]",2,0); // top
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_MAXFRAMEHEIGHT, "text[max,frameHeight]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_MINFRAMEWIDTH, "text[min,frameWidth]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_MAXFRAMEWIDTH, "text[max,frameWidth]",4, 0); // metric
  addAttributeBool(map, StarAttribute::SDRATTR_TEXT_AUTOGROWWIDTH,"text[autoGrow,width]", false); // onOff
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_HORZADJUST, "text[horz,adjust]",2,3); // block
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_ANIKIND, "text[ani,kind]",2,0); // none
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_ANIDIRECTION, "text[ani,direction]",2,0); // left
  addAttributeBool(map, StarAttribute::SDRATTR_TEXT_ANISTARTINSIDE,"text[ani,startInside]", false); // yesNo
  addAttributeBool(map, StarAttribute::SDRATTR_TEXT_ANISTOPINSIDE,"text[ani,stopInside]", false); // yesNo
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_ANICOUNT, "text[ani,count]",2,0);
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXT_ANIDELAY, "text[ani,delay]",2,0);
  addAttributeInt(map, StarAttribute::SDRATTR_TEXT_ANIAMOUNT, "text[ani,amount]",2, 0);
  addAttributeBool(map, StarAttribute::SDRATTR_TEXT_CONTOURFRAME,"text[contourFrame]", false); // onOff
  addAttributeVoid(map, StarAttribute::SDRATTR_XMLATTRIBUTES,"sdr[xmlAttrib]");
  addAttributeUInt(map, StarAttribute::SDRATTR_CIRCKIND, "circle[kind]",2,0); // full
  addAttributeInt(map, StarAttribute::SDRATTR_CIRCSTARTANGLE, "circle[angle,start]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_CIRCENDANGLE, "circle[angle,end]",4, 36000); // sdrAngle

  addAttributeInt(map, StarAttribute::SDRATTR_MEASURELINEDIST, "measure[line,dist]",4, 800); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_MEASUREOVERHANG, "measure[overHang]",4, 600); // metric
  addAttributeBool(map, StarAttribute::SDRATTR_MEASUREBELOWREFEDGE,"measure[belowRefEdge]", false); // yesNo
  addAttributeBool(map, StarAttribute::SDRATTR_MEASURESHOWUNIT,"measure[showUnit]", false); // yesNo
  addAttributeBool(map, StarAttribute::SDRATTR_OBJMOVEPROTECT,"obj[move,protect]", false); // yesNo
  addAttributeBool(map, StarAttribute::SDRATTR_OBJSIZEPROTECT,"obj[size,protect]", false); // yesNo
  addAttributeBool(map, StarAttribute::SDRATTR_OBJPRINTABLE,"obj[printable]", false); // yesNo

  addAttributeInt(map, StarAttribute::SDRATTR_GRAFRED, "graf[red]",2, 0); // signed percent
  addAttributeInt(map, StarAttribute::SDRATTR_GRAFGREEN, "graf[green]",2, 0); // signed percent
  addAttributeInt(map, StarAttribute::SDRATTR_GRAFBLUE, "graf[blue]",2, 0); // signed percent
  addAttributeInt(map, StarAttribute::SDRATTR_GRAFLUMINANCE, "graf[luminance]",2, 0); // signed percent
  addAttributeInt(map, StarAttribute::SDRATTR_GRAFCONTRAST, "graf[contrast]",2, 0); // signed percent
  addAttributeUInt(map, StarAttribute::SDRATTR_GRAFGAMMA, "graf[gamma]",4,100);
  addAttributeUInt(map, StarAttribute::SDRATTR_GRAFTRANSPARENCE, "graf[transparence]",2,0);
  addAttributeBool(map, StarAttribute::SDRATTR_GRAFINVERT,"graf[invert]", false); // onOff
  addAttributeUInt(map, StarAttribute::SDRATTR_GRAFMODE, "graf[mode]",2,0);  // standard

  map[StarAttribute::XATTR_LINECOLOR]=std::shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedColor(StarAttribute::XATTR_LINECOLOR,"line[color]",STOFFColor::black()));
  map[StarAttribute::XATTR_LINESTART]=std::shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedArrow(StarAttribute::XATTR_LINESTART,"line[start]"));
  map[StarAttribute::XATTR_LINEEND]=std::shared_ptr<StarAttribute>
                                    (new StarGAttributeNamedArrow(StarAttribute::XATTR_LINEEND,"line[end]"));
  map[StarAttribute::XATTR_LINEDASH]=std::shared_ptr<StarAttribute>
                                     (new StarGAttributeNamedDash(StarAttribute::XATTR_LINEDASH,"line[dash]"));
  map[StarAttribute::XATTR_FILLCOLOR]=std::shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedColor(StarAttribute::XATTR_FILLCOLOR,"fill[color]",STOFFColor::white()));
  map[StarAttribute::XATTR_FILLGRADIENT]=std::shared_ptr<StarAttribute>
                                         (new StarGAttributeNamedGradient(StarAttribute::XATTR_FILLGRADIENT,"gradient[fill]"));
  map[StarAttribute::XATTR_FILLHATCH]=std::shared_ptr<StarAttribute>
                                      (new StarGAttributeNamedHatch(StarAttribute::XATTR_FILLHATCH,"hatch[fill]"));
  map[StarAttribute::XATTR_FILLBITMAP]=std::shared_ptr<StarAttribute>
                                       (new StarGAttributeNamedBitmap(StarAttribute::XATTR_FILLBITMAP,"bitmap[fill]"));
  map[StarAttribute::XATTR_FORMTXTSHDWCOLOR]=std::shared_ptr<StarAttribute>
      (new StarGAttributeNamedColor(StarAttribute::XATTR_FORMTXTSHDWCOLOR,"shadow[form,color]",STOFFColor::black()));
  map[StarAttribute::SDRATTR_SHADOWCOLOR]=std::shared_ptr<StarAttribute>
                                          (new StarGAttributeNamedColor(StarAttribute::SDRATTR_SHADOWCOLOR,"sdrShadow[color]",STOFFColor::black()));
  map[StarAttribute::SDRATTR_GRAFCROP]=std::shared_ptr<StarAttribute>
                                       (new StarGAttributeCrop(StarAttribute::SDRATTR_GRAFCROP, "graf[crop],"));

  std::vector<STOFFVec2i> limits;
  limits.resize(1);
  limits[0]=STOFFVec2i(1000,1016);
  map[StarAttribute::XATTR_SET_LINE]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::XATTR_SET_LINE,"setLine",limits));
  limits[0]=STOFFVec2i(1018,1046);
  map[StarAttribute::XATTR_SET_FILL]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::XATTR_SET_FILL,"setFill",limits));
  limits[0]=STOFFVec2i(1048,1064);
  map[StarAttribute::XATTR_SET_TEXT]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::XATTR_SET_TEXT,"setText",limits));
  limits[0]=STOFFVec2i(1067,1078);
  map[StarAttribute::SDRATTR_SET_SHADOW]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_SHADOW,"setShadow",limits));
  limits[0]=STOFFVec2i(1080,1094);
  map[StarAttribute::SDRATTR_SET_CAPTION]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_CAPTION,"setCaption",limits));
  limits[0]=STOFFVec2i(1097,1125);
  map[StarAttribute::SDRATTR_SET_MISC]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_MISC,"setMisc",limits));
  limits[0]=STOFFVec2i(1127,1145);
  map[StarAttribute::SDRATTR_SET_EDGE]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_EDGE,"setEdge",limits));
  limits[0]=STOFFVec2i(1147,1170);
  map[StarAttribute::SDRATTR_SET_MEASURE]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_MEASURE,"setMeasure",limits));
  limits[0]=STOFFVec2i(1172,1178);
  map[StarAttribute::SDRATTR_SET_CIRC]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_CIRC,"setCircle",limits));
  limits[0]=STOFFVec2i(1180,1242);
  map[StarAttribute::SDRATTR_SET_GRAF]=std::shared_ptr<StarAttribute>(new StarGAttributeItemSet(StarAttribute::SDRATTR_SET_GRAF,"setGraf",limits));

  // probably ok to ignore as we have the path
  addAttributeUInt(map, StarAttribute::SDRATTR_EDGEKIND, "edge[kind]",2,0); // ortholine
  addAttributeInt(map, StarAttribute::SDRATTR_EDGENODE1HORZDIST, "edge[node1,hori,dist]",4, 500); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGENODE1VERTDIST, "edge[node1,vert,dist]",4, 500); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGENODE2HORZDIST, "edge[node2,hori,dist]",4, 500); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGENODE2VERTDIST, "edge[node2,vert,dist]",4, 500); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGENODE1GLUEDIST, "edge[node1,glue,dist]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGENODE2GLUEDIST, "edge[node2,glue,dist]",4, 0); // metric
  addAttributeUInt(map, StarAttribute::SDRATTR_EDGELINEDELTAANZ, "edge[line,deltaAnz]",2,0);
  addAttributeInt(map, StarAttribute::SDRATTR_EDGELINE1DELTA, "edge[delta,line1]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGELINE2DELTA, "edge[delta,line2]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_EDGELINE3DELTA, "edge[delta,line3]",4, 0); // metric
  // can we retrieve these properties ?
  addAttributeUInt(map, StarAttribute::SDRATTR_MEASUREKIND, "measure[kind]",2,0); // standard
  addAttributeUInt(map, StarAttribute::SDRATTR_MEASURETEXTHPOS, "measure[text,hpos]",2,0); // auto
  addAttributeUInt(map, StarAttribute::SDRATTR_MEASURETEXTVPOS, "measure[text,vpos]",2,0); // auto
  addAttributeInt(map, StarAttribute::SDRATTR_MEASUREHELPLINEOVERHANG, "measure[help,line,overhang]",4, 200); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_MEASUREHELPLINEDIST, "measure[help,line,dist]",4, 100); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_MEASUREHELPLINE1LEN, "measure[help,line1,len]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_MEASUREHELPLINE2LEN, "measure[help,line2,len]",4, 0); // metric
  addAttributeBool(map, StarAttribute::SDRATTR_MEASURETEXTROTA90,"measure[textRot90]", false); // yesNo
  addAttributeBool(map, StarAttribute::SDRATTR_MEASURETEXTUPSIDEDOWN,"measure[textUpsideDown]", false); // yesNo
  addAttributeUInt(map, StarAttribute::SDRATTR_MEASUREUNIT, "measure[unit]",2,0); // NONE
  addAttributeBool(map, StarAttribute::SDRATTR_MEASURETEXTAUTOANGLE,"measure[text,isAutoAngle]", true); // yesNo
  addAttributeInt(map, StarAttribute::SDRATTR_MEASURETEXTAUTOANGLEVIEW,"measure[text,autoAngle]", 4,31500); // angle
  addAttributeBool(map, StarAttribute::SDRATTR_MEASURETEXTISFIXEDANGLE,"measure[text,isFixedAngle]", false); // yesNo
  addAttributeInt(map, StarAttribute::SDRATTR_MEASURETEXTFIXEDANGLE,"measure[text,fixedAngle]", 4,0); // angle
  addAttributeInt(map, StarAttribute::SDRATTR_MEASUREDECIMALPLACES,"measure[decimal,place]", 2,2);
  addAttributeFraction(map, StarAttribute::SDRATTR_MEASURESCALE, "measure[scale,mult]");
  // can we retrieve these properties ?
  addAttributeUInt(map, StarAttribute::SDRATTR_CAPTIONTYPE, "caption[type]",2,2); // type3
  addAttributeBool(map, StarAttribute::SDRATTR_CAPTIONFIXEDANGLE,"caption[fixedAngle]", true); // onOff
  addAttributeInt(map, StarAttribute::SDRATTR_CAPTIONANGLE, "caption[angle]",4, 0); // angle
  addAttributeInt(map, StarAttribute::SDRATTR_CAPTIONGAP, "caption[gap]",4, 0); // metric
  addAttributeUInt(map, StarAttribute::SDRATTR_CAPTIONESCDIR, "caption[esc,dir]",2,0); // horizontal
  addAttributeBool(map, StarAttribute::SDRATTR_CAPTIONESCISREL,"caption[esc,isRel]", true); // yesNo
  addAttributeInt(map, StarAttribute::SDRATTR_CAPTIONESCREL, "caption[esc,rel]",4, 5000);
  addAttributeInt(map, StarAttribute::SDRATTR_CAPTIONESCABS, "caption[esc,abs]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_CAPTIONLINELEN, "caption[line,len]",4, 0); // metric
  addAttributeBool(map, StarAttribute::SDRATTR_CAPTIONFITLINELEN,"caption[fit,lineLen]", true); // yesNo

  for (int type=StarAttribute::XATTR_LINERESERVED2; type<=StarAttribute::XATTR_LINERESERVED_LAST; ++type) {
    std::stringstream s;
    s << "line[reserved" << type-StarAttribute::XATTR_LINERESERVED2+2 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::XATTR_FILLRESERVED2; type<=StarAttribute::XATTR_FILLRESERVED_LAST; ++type) {
    std::stringstream s;
    s << "fill[reserved" << type-StarAttribute::XATTR_FILLRESERVED2+2 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_SHADOWRESERVE1; type<=StarAttribute::SDRATTR_SHADOWRESERVE5; ++type) {
    std::stringstream s;
    s << "shadow[reserved" << type-StarAttribute::SDRATTR_SHADOWRESERVE1+1 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_RESERVE15; type<=StarAttribute::SDRATTR_RESERVE19; ++type) {
    std::stringstream s;
    s << "sdr[reserved" << type-StarAttribute::SDRATTR_RESERVE15+15 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_CIRCRESERVE0; type<=StarAttribute::SDRATTR_CIRCRESERVE3; ++type) {
    std::stringstream s;
    s << "circle[reserved" << type-StarAttribute::SDRATTR_CIRCRESERVE0 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_EDGERESERVE02; type<=StarAttribute::SDRATTR_EDGERESERVE09; ++type) {
    std::stringstream s;
    s << "edge[reserved" << type-StarAttribute::SDRATTR_EDGERESERVE02+2 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_MEASURERESERVE05; type<=StarAttribute::SDRATTR_MEASURERESERVE07; ++type) {
    std::stringstream s;
    s << "measure[reserved" << type-StarAttribute::SDRATTR_MEASURERESERVE05+5 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_CAPTIONRESERVE1; type<=StarAttribute::SDRATTR_CAPTIONRESERVE5; ++type) {
    std::stringstream s;
    s << "caption[reserved" << type-StarAttribute::SDRATTR_CAPTIONRESERVE1+1 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_GRAFRESERVE3; type<=StarAttribute::SDRATTR_GRAFRESERVE6; ++type) {
    std::stringstream s;
    s << "graf[reserved" << type-StarAttribute::SDRATTR_GRAFRESERVE3+3 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }
  for (int type=StarAttribute::SDRATTR_NOTPERSISTRESERVE2; type<=StarAttribute::SDRATTR_NOTPERSISTRESERVE15; ++type) {
    std::stringstream s;
    s << "notpersist[reserved" << type-StarAttribute::SDRATTR_NOTPERSISTRESERVE2+2 << "]";
    addAttributeVoid(map, StarAttribute::Type(type), s.str());
  }

  // to do
  addAttributeUInt(map, StarAttribute::SDRATTR_LAYERID, "layer[id]",2,0);
  addAttributeInt(map, StarAttribute::SDRATTR_ALLPOSITIONX, "positionX[all]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ALLPOSITIONY, "positionY[all]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ALLSIZEWIDTH, "size[all,width]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ALLSIZEHEIGHT, "size[all,height]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ONEPOSITIONX, "poitionX[one]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ONEPOSITIONY, "poitionY[one]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ONESIZEWIDTH, "size[one,width]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ONESIZEHEIGHT, "size[one,height]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_LOGICSIZEWIDTH, "size[logic,width]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_LOGICSIZEHEIGHT, "size[logic,height]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ROTATEANGLE, "rotate[angle]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_SHEARANGLE, "shear[angle]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_MOVEX, "moveX",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_MOVEY, "moveY",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_ROTATEONE, "rotate[one]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_HORZSHEARONE, "shear[horzOne]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_VERTSHEARONE, "shear[vertOne]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_ROTATEALL, "rotate[all]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_HORZSHEARALL, "shear[horzAll]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_VERTSHEARALL, "shear[vertAll]",4, 0); // sdrAngle
  addAttributeInt(map, StarAttribute::SDRATTR_TRANSFORMREF1X, "transform[ref1X]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TRANSFORMREF1Y, "transform[ref1Y]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TRANSFORMREF2X, "transform[ref2X]",4, 0); // metric
  addAttributeInt(map, StarAttribute::SDRATTR_TRANSFORMREF2Y, "transform[ref2Y]",4, 0); // metric
  addAttributeUInt(map, StarAttribute::SDRATTR_TEXTDIRECTION, "text[direction]",2,0); // LRTB
  addAttributeFraction(map, StarAttribute::SDRATTR_RESIZEXONE, "sdrResizeX[one]");
  addAttributeFraction(map, StarAttribute::SDRATTR_RESIZEYONE, "sdrResizeY[one]");
  addAttributeFraction(map, StarAttribute::SDRATTR_RESIZEXALL, "sdrResizeX[all]");
  addAttributeFraction(map, StarAttribute::SDRATTR_RESIZEYALL, "sdrResizeY[all]");

  addAttributeBool(map, StarAttribute::XATTR_FILLBMP_SIZELOG,"fill[bmp,sizeLog]", true);

  map[StarAttribute::XATTR_FILLFLOATTRANSPARENCE]=std::shared_ptr<StarAttribute>
      (new StarGAttributeNamedGradient(StarAttribute::XATTR_FILLFLOATTRANSPARENCE,"gradient[trans,fill]"));


  // TOUSE
  map[StarAttribute::ATTR_SC_BORDER_INNER]=std::shared_ptr<StarAttribute>(new StarGAttributeBoxInfo(StarAttribute::ATTR_SC_BORDER_INNER,"scBorder[inner]"));

}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
