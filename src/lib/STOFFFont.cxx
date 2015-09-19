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

#include "STOFFFont.hxx"

// line function

std::ostream &operator<<(std::ostream &o, STOFFFont::Line const &line)
{
  if (!line.isSet())
    return o;
  switch (line.m_style) {
  case STOFFFont::Line::Dot:
    o << "dotted";
    break;
  case STOFFFont::Line::LargeDot:
    o << "dotted[large]";
    break;
  case STOFFFont::Line::Dash:
    o << "dash";
    break;
  case STOFFFont::Line::Simple:
    o << "solid";
    break;
  case STOFFFont::Line::Wave:
    o << "wave";
    break;
  case STOFFFont::Line::None:
  default:
    break;
  }
  switch (line.m_type) {
  case STOFFFont::Line::Double:
    o << ":double";
    break;
  case STOFFFont::Line::Triple:
    o << ":triple";
    break;
  case STOFFFont::Line::Single:
  default:
    break;
  }
  if (line.m_word) o << ":byword";
  if (line.m_width < 1.0 || line.m_width > 1.0)
    o << ":w=" << line.m_width ;
  if (line.m_color.isSet())
    o << ":col=" << line.m_color.get();
  return o;
}

void STOFFFont::Line::addTo(librevenge::RVNGPropertyList &propList, std::string const &type) const
{
  if (!isSet()) return;

  std::stringstream s;
  s << "style:text-" << type << "-type";
  propList.insert(s.str().c_str(), (m_type==Single) ? "single" : "double");

  if (m_word) {
    s.str("");
    s << "style:text-" << type << "-mode";
    propList.insert(s.str().c_str(), "skip-white-space");
  }

  s.str("");
  s << "style:text-" << type << "-style";
  switch (m_style) {
  case Dot:
  case LargeDot:
    propList.insert(s.str().c_str(), "dotted");
    break;
  case Dash:
    propList.insert(s.str().c_str(), "dash");
    break;
  case Simple:
    propList.insert(s.str().c_str(), "solid");
    break;
  case Wave:
    propList.insert(s.str().c_str(), "wave");
    break;
  case None:
  default:
    break;
  }
  if (m_color.isSet()) {
    s.str("");
    s << "style:text-" << type << "-color";
    propList.insert(s.str().c_str(), m_color.get().str().c_str());
  }
  //normal, bold, thin, dash, medium, and thick
  s.str("");
  s << "style:text-" << type << "-width";
  if (m_width <= 0.6)
    propList.insert(s.str().c_str(), "thin");
  else if (m_width >= 1.5)
    propList.insert(s.str().c_str(), "thick");
}

// script function

std::string STOFFFont::Script::str(float fSize) const
{
  if (!isSet() || ((m_delta<=0&&m_delta>=0) && m_scale==100))
    return "";
  std::stringstream o;
  if (m_deltaUnit == librevenge::RVNG_GENERIC) {
    STOFF_DEBUG_MSG(("STOFFFont::Script::str: can not be called with generic position\n"));
    return "";
  }
  float delta = m_delta;
  if (m_deltaUnit != librevenge::RVNG_PERCENT) {
    // first transform in point
    if (m_deltaUnit != librevenge::RVNG_POINT)
      delta=libstoff::getScaleFactor(m_deltaUnit, librevenge::RVNG_POINT)*delta;
    // now transform in percent
    if (fSize<=0) {
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("STOFFFont::Script::str: can not be find font size (supposed 12pt)\n"));
        first = false;
      }
      fSize=12;
    }
    delta=100.f*delta/fSize;
    if (delta > 100) delta = 100;
    else if (delta < -100) delta = -100;
  }
  o << delta << "% " << m_scale << "%";
  return o.str();
}

// font function

std::ostream &operator<<(std::ostream &o, STOFFFont const &font)
{
  if (!font.getFontName().empty()) o << "nam='" << font.getFontName().cstr() << "',";
  if (font.size() > 0) o << "sz=" << font.size() << ",";
  if (font.m_deltaSpacing.isSet()) {
    if (font.m_deltaSpacingUnit.get()==librevenge::RVNG_PERCENT)
      o << "extend/condensed=" << font.m_deltaSpacing.get() << "%,";
    else if (font.m_deltaSpacing.get() > 0)
      o << "extended=" << font.m_deltaSpacing.get() << ",";
    else if (font.m_deltaSpacing.get() < 0)
      o << "condensed=" << -font.m_deltaSpacing.get() << ",";
  }
  if (font.m_widthStreching.isSet())
    o << "scaling[width]=" <<  font.m_widthStreching.get()*100.f << "%,";
  if (font.m_scriptPosition.isSet() && font.m_scriptPosition.get().isSet())
    o << "script=" << font.m_scriptPosition.get().str(font.size()) << ",";
  if (font.m_flags.isSet() && font.m_flags.get()) {
    o << "fl=";
    uint32_t flag = font.m_flags.get();
    if (flag&STOFFFont::boldBit) o << "b:";
    if (flag&STOFFFont::italicBit) o << "it:";
    if (flag&STOFFFont::embossBit) o << "emboss:";
    if (flag&STOFFFont::shadowBit) o << "shadow:";
    if (flag&STOFFFont::outlineBit) o << "outline:";
    if (flag&STOFFFont::smallCapsBit) o << "smallCaps:";
    if (flag&STOFFFont::uppercaseBit) o << "uppercase:";
    if (flag&STOFFFont::lowercaseBit) o << "lowercase:";
    if (flag&STOFFFont::initialcaseBit) o << "capitalise:";
    if (flag&STOFFFont::hiddenBit) o << "hidden:";
    if (flag&STOFFFont::reverseVideoBit) o << "reverseVideo:";
    if (flag&STOFFFont::blinkBit) o << "blink:";
    if (flag&STOFFFont::boxedBit) o << "box:";
    if (flag&STOFFFont::boxedRoundedBit) o << "box[rounded]:";
    if (flag&STOFFFont::reverseWritingBit) o << "reverseWriting:";
    o << ",";
  }
  if (font.m_overline.isSet() && font.m_overline->isSet())
    o << "overline=[" << font.m_overline.get() << "],";
  if (font.m_strikeoutline.isSet() && font.m_strikeoutline->isSet())
    o << "strikeOut=[" << font.m_strikeoutline.get() << "],";
  if (font.m_underline.isSet() && font.m_underline->isSet())
    o << "underline=[" << font.m_underline.get() << "],";
  if (font.hasColor())
    o << "col=" << font.m_color.get()<< ",";
  if (font.m_backgroundColor.isSet() && !font.m_backgroundColor.get().isWhite())
    o << "backCol=" << font.m_backgroundColor.get() << ",";
  if (font.m_language.isSet() && font.m_language.get().length())
    o << "lang=" << font.m_language.get() << ",";
  o << font.m_extra;
  return o;
}

void STOFFFont::addTo(librevenge::RVNGPropertyList &pList) const
{
  if (!getFontName().empty())
    pList.insert("style:font-name", getFontName().cstr());
  pList.insert("fo:font-size", size(), librevenge::RVNG_POINT);

  uint32_t attributeBits = m_flags.get();
  if (attributeBits & italicBit)
    pList.insert("fo:font-style", "italic");
  if (attributeBits & boldBit)
    pList.insert("fo:font-weight", "bold");
  if (attributeBits & outlineBit)
    pList.insert("style:text-outline", "true");
  if (attributeBits & blinkBit)
    pList.insert("style:text-blinking", "true");
  if (attributeBits & shadowBit)
    pList.insert("fo:text-shadow", "1pt 1pt");
  if (attributeBits & hiddenBit)
    pList.insert("text:display", "none");
  if (attributeBits & lowercaseBit)
    pList.insert("fo:text-transform", "lowercase");
  else if (attributeBits & uppercaseBit)
    pList.insert("fo:text-transform", "uppercase");
  else if (attributeBits & initialcaseBit)
    pList.insert("fo:text-transform", "capitalize");
  if (attributeBits & smallCapsBit)
    pList.insert("fo:font-variant", "small-caps");
  if (attributeBits & embossBit)
    pList.insert("style:font-relief", "embossed");
  else if (attributeBits & engraveBit)
    pList.insert("style:font-relief", "engraved");

  if (m_scriptPosition.isSet() && m_scriptPosition->isSet()) {
    std::string pos=m_scriptPosition->str(size());
    if (pos.length())
      pList.insert("style:text-position", pos.c_str());
  }

  if (m_overline.isSet() && m_overline->isSet())
    m_overline->addTo(pList, "overline");
  if (m_strikeoutline.isSet() && m_strikeoutline->isSet())
    m_strikeoutline->addTo(pList, "line-through");
  if (m_underline.isSet() && m_underline->isSet())
    m_underline->addTo(pList, "underline");
  if ((attributeBits & boxedBit) || (attributeBits & boxedRoundedBit)) {
    // do minimum: add a overline and a underline box
    Line simple(Line::Simple);
    if (!m_overline.isSet() || !m_overline->isSet())
      simple.addTo(pList, "overline");
    if (!m_underline.isSet() || !m_underline->isSet())
      simple.addTo(pList, "underline");
  }
  if (m_deltaSpacing.isSet()) {
    if (m_deltaSpacingUnit.get()==librevenge::RVNG_PERCENT) {
      if (m_deltaSpacing.get() < 1 || m_deltaSpacing.get()>1) {
        std::stringstream s;
        s << m_deltaSpacing.get() << "em";
        pList.insert("fo:letter-spacing", s.str().c_str());
      }
    }
    else if (m_deltaSpacing.get() < 0 || m_deltaSpacing.get()>0)
      pList.insert("fo:letter-spacing", m_deltaSpacing.get(), librevenge::RVNG_POINT);
  }
  if (m_widthStreching.isSet() && m_widthStreching.get() > 0.0 &&
      (m_widthStreching.get()>1.0||m_widthStreching.get()<1.0))
    pList.insert("style:text-scale", m_widthStreching.get(), librevenge::RVNG_PERCENT);
  if (attributeBits & reverseVideoBit) {
    pList.insert("fo:color", m_backgroundColor->str().c_str());
    pList.insert("fo:background-color", m_color->str().c_str());
  }
  else {
    pList.insert("fo:color", m_color->str().c_str());
    if (m_backgroundColor.isSet() && !m_backgroundColor->isWhite())
      pList.insert("fo:background-color", m_backgroundColor->str().c_str());
  }
  if (m_language.isSet()) {
    size_t len=m_language->length();
    std::string lang(m_language.get());
    std::string country("none");
    if (len > 3 && lang[2]=='_') {
      country=lang.substr(3);
      lang=m_language->substr(0,2);
    }
    else if (len==0)
      lang="none";
    pList.insert("fo:language", lang.c_str());
    pList.insert("fo:country", country.c_str());
  }
  if (attributeBits & reverseWritingBit) {
    static bool first = true;
    if (first) {
      first = false;
      STOFF_DEBUG_MSG(("STOFFFont::addTo: sorry, reverse writing is not umplemented\n"));
    }
  }
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
