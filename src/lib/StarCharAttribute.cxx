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

#include "StarCharAttribute.hxx"

#include "STOFFFont.hxx"
#include "STOFFListener.hxx"
#include "STOFFSubDocument.hxx"

#include "StarAttribute.hxx"
#include "StarFormatManager.hxx"
#include "StarItemPool.hxx"
#include "StarLanguage.hxx"
#include "StarObject.hxx"
#include "StarObjectText.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "SWFieldManager.hxx"

namespace StarCharAttribute
{
//! a character bool attribute
class StarCAttributeBool final : public StarAttributeBool
{
public:
  //! constructor
  StarCAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeBool(*this));
  }
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;

protected:
  //! copy constructor
  StarCAttributeBool(StarCAttributeBool const &) = default;
};

//! a character color attribute
class StarCAttributeColor final : public StarAttributeColor
{
public:
  //! constructor
  StarCAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeColor(*this));
  }
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarCAttributeColor(StarCAttributeColor const &) = default;
};

//! a character integer attribute
class StarCAttributeInt final : public StarAttributeInt
{
public:
  //! constructor
  StarCAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarCAttributeInt(StarCAttributeInt const &) = default;
};

//! a character unsigned integer attribute
class StarCAttributeUInt final : public StarAttributeUInt
{
public:
  //! constructor
  StarCAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeUInt(*this));
  }
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarCAttributeUInt(StarCAttributeUInt const &) = default;
};

//! a void attribute
class StarCAttributeVoid final : public StarAttributeVoid
{
public:
  //! constructor
  StarCAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeVoid(*this));
  }
protected:
  //! copy constructor
  StarCAttributeVoid(StarCAttributeVoid const &) = default;
};

//! add a bool attribute
inline void addAttributeBool(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarCAttributeBool(type,debugName, defValue));
}
//! add a color attribute
inline void addAttributeColor(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarCAttributeColor(type,debugName, defValue));
}
//! add a int attribute
inline void addAttributeInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarCAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
inline void addAttributeUInt(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarCAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
inline void addAttributeVoid(std::map<int, std::shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=std::shared_ptr<StarAttribute>(new StarCAttributeVoid(type,debugName));
}

void StarCAttributeBool::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_CHR_CONTOUR)
    state.m_font.m_propertyList.insert("style:text-outline", m_value);
  else if (m_type==ATTR_CHR_SHADOWED)
    state.m_font.m_propertyList.insert("fo:text-shadow", m_value ? "1pt 1pt" : "none");
  else if (m_type==ATTR_CHR_BLINK)
    state.m_font.m_propertyList.insert("style:text-blinking", m_value);
  else if (m_type==ATTR_CHR_WORDLINEMODE)  {
    state.m_font.m_propertyList.insert("style:text-line-through-mode", m_value ? "skip-white-space" : "continuous");
    state.m_font.m_propertyList.insert("style:text-underline-mode", m_value ? "skip-white-space" : "continuous");
  }
  else if (m_type==ATTR_CHR_AUTOKERN)
    state.m_font.m_propertyList.insert("style:letter-kerning", m_value);
  else if (m_type==ATTR_SC_HYPHENATE)
    state.m_font.m_propertyList.insert("fo:hyphenate", m_value);
  else if (m_type==ATTR_CHR_NOHYPHEN)
    state.m_font.m_hyphen=!m_value;
  else if (m_type==ATTR_CHR_NOLINEBREAK)
    state.m_font.m_lineBreak=!m_value;
}

void StarCAttributeColor::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_CHR_COLOR)
    state.m_font.m_propertyList.insert("fo:color", m_value.str().c_str());
}

void StarCAttributeInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_CHR_KERNING)
    state.m_font.m_propertyList.insert("fo:letter-spacing", m_value, librevenge::RVNG_TWIP);
}

void StarCAttributeUInt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_CHR_CROSSEDOUT) {
    switch (m_value) {
    case 0: // none
      state.m_font.m_propertyList.insert("style:text-line-through-type", "none");
      break;
    case 1: // single
    case 2: // double
      state.m_font.m_propertyList.insert("style:text-line-through-type", m_value==1 ? "single" : "double");
      state.m_font.m_propertyList.insert("style:text-line-through-style", "solid");
      break;
    case 3: // dontknow
      break;
    case 4: // bold
      state.m_font.m_propertyList.insert("style:text-line-through-type", "single");
      state.m_font.m_propertyList.insert("style:text-line-through-style", "solid");
      state.m_font.m_propertyList.insert("style:text-line-through-width", "thick");
      break;
    case 5: // slash
    case 6: // X
      state.m_font.m_propertyList.insert("style:text-line-through-type", "single");
      state.m_font.m_propertyList.insert("style:text-line-through-style", "solid");
      state.m_font.m_propertyList.insert("style:text-line-through-text", m_value==5 ? "/" : "X");
      break;
    default:
      state.m_font.m_propertyList.insert("style:text-line-through-type", "none");
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown crossedout enum=%d\n", m_value));
      break;
    }
  }
  else if (m_type==ATTR_CHR_UNDERLINE) {
    switch (m_value) {
    case 0: // none
    case 4: // unknown
      state.m_font.m_propertyList.insert("style:text-underline-type", "none");
      break;
    case 1: // single
    case 2: // double
      state.m_font.m_propertyList.insert("style:text-underline-type", m_value==1 ? "single" : "double");
      state.m_font.m_propertyList.insert("style:text-underline-style", "solid");
      break;
    case 3: // dot
    case 5: // dash
    case 6: // long-dash
    case 7: // dash-dot
    case 8: // dash-dot-dot
      state.m_font.m_propertyList.insert("style:text-underline-type", "single");
      state.m_font.m_propertyList.insert("style:text-underline-style", m_value==3 ? "dotted" : m_value==5 ? "dash" :
                                         m_value==6 ? "long-dash" : m_value==7 ? "dot-dash" : "dot-dot-dash");
      break;
    case 9: // small wave
    case 10: // wave
    case 11: // double wave
      state.m_font.m_propertyList.insert("style:text-underline-type", m_value==11 ? "single" : "double");
      state.m_font.m_propertyList.insert("style:text-underline-style", "wave");
      if (m_value==9)
        state.m_font.m_propertyList.insert("style:text-underline-width", "thin");
      break;
    case 12: // bold
    case 13: // bold dot
    case 14: // bold dash
    case 15: // bold long-dash
    case 16: // bold dash-dot
    case 17: // bold dash-dot-dot
    case 18: { // bold wave
      char const *wh[]= {"solid", "dotted", "dash", "long-dash", "dot-dash", "dot-dot-dash", "wave"};
      state.m_font.m_propertyList.insert("style:text-underline-type", "single");
      state.m_font.m_propertyList.insert("style:text-underline-style", wh[m_value-12]);
      state.m_font.m_propertyList.insert("style:text-underline-width", "thick");
      break;
    }
    default:
      state.m_font.m_propertyList.insert("style:text-underline-type", "none");
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown underline enum=%d\n", m_value));
      break;
    }
  }
  else if (m_type==ATTR_CHR_POSTURE || m_type==ATTR_CHR_CJK_POSTURE || m_type==ATTR_CHR_CTL_POSTURE) {
    std::string wh(m_type==ATTR_CHR_POSTURE ? "fo:font-style" :  m_type==ATTR_CHR_CJK_POSTURE ? "style:font-style-asian" : "style:font-style-complex");
    if (m_value==1)
      state.m_font.m_propertyList.insert(wh.c_str(), "oblique");
    else if (m_value==2)
      state.m_font.m_propertyList.insert(wh.c_str(), "italic");
    else if (m_value) {
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown posture enum=%d\n", m_value));
    }
  }
  else if (m_type==ATTR_CHR_RELIEF) {
    if (m_value==0)
      state.m_font.m_propertyList.insert("style:font-relief", "none");
    else if (m_value==1)
      state.m_font.m_propertyList.insert("style:font-relief", "embossed");
    else if (m_value==2)
      state.m_font.m_propertyList.insert("style:font-relief", "engraved");
    else if (m_value) {
      state.m_font.m_propertyList.insert("style:font-relief", "none");
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown relief enum=%d\n", m_value));
    }
  }
  else if (m_type==ATTR_CHR_WEIGHT || m_type==ATTR_CHR_CJK_WEIGHT || m_type==ATTR_CHR_CTL_WEIGHT) {
    std::string wh(m_type==ATTR_CHR_WEIGHT ? "fo:font-weight" :  m_type==ATTR_CHR_CJK_WEIGHT ? "style:font-weight-asian" : "style:font-weight-complex");
    if (m_value==5)
      state.m_font.m_propertyList.insert(wh.c_str(), "normal");
    else if (m_value==8)
      state.m_font.m_propertyList.insert(wh.c_str(), "bold");
    else if (m_value>=1 && m_value<=9)
      state.m_font.m_propertyList.insert(wh.c_str(), m_value*100, librevenge::RVNG_GENERIC);
    else // 10: WEIGHT_BLACK
      state.m_font.m_propertyList.insert(wh.c_str(), "normal");
  }
  else if (m_type==ATTR_CHR_CASEMAP) {
    if (m_value==0)
      state.m_font.m_propertyList.insert("fo:text-transform", "none");
    else if (m_value==1)
      state.m_font.m_propertyList.insert("fo:text-transform", "uppercase");
    else if (m_value==2)
      state.m_font.m_propertyList.insert("fo:text-transform", "lowercase");
    else if (m_value==3)
      state.m_font.m_propertyList.insert("fo:text-transform", "capitalize");
    else if (m_value==4)
      state.m_font.m_propertyList.insert("fo:font-variant", "small-caps");
    else if (m_value) {
      state.m_font.m_propertyList.insert("fo:text-transform", "none");
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown casemap enum=%d\n", m_value));
    }
  }
  else if (m_type==ATTR_CHR_LANGUAGE || m_type==ATTR_CHR_CJK_LANGUAGE || m_type==ATTR_CHR_CTL_LANGUAGE || m_type==ATTR_SC_LANGUAGE_FORMAT) {
    bool const basic=m_type==ATTR_CHR_LANGUAGE||m_type==ATTR_SC_LANGUAGE_FORMAT;
    std::string prefix(basic ? "fo:" : "style:");
    std::string extension(basic ? "" :  m_type==ATTR_CHR_CJK_LANGUAGE ? "-asian" : "-complex");
    std::string lang, country;
    if (StarLanguage::getLanguageId(int(m_value), lang, country)) {
      if (!lang.empty())
        state.m_font.m_propertyList.insert((prefix + "language" + extension).c_str(), lang.c_str());
      if (!country.empty())
        state.m_font.m_propertyList.insert((prefix + "country" + extension).c_str(), country.c_str());
    }
  }
  else if (m_type==ATTR_CHR_PROPORTIONALFONTSIZE && m_value!=100) // checkme: maybe font-size with %
    state.m_font.m_propertyList.insert("style:text-scale", double(m_value)/100., librevenge::RVNG_PERCENT);
  else if (m_type==ATTR_CHR_EMPHASIS_MARK) {
    if (m_value && ((m_value&0xC000)==0 || (m_value&0x3000)==0x3000 || (m_value&0xFFF)>4 || (m_value&0xFFF)==0)) {
      state.m_font.m_propertyList.insert("style:text-emphasize", "none");
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown emphasis mark=%x\n", unsigned(m_value)));
    }
    else if (m_value) {
      std::string what((m_value&7)== 1 ? "dot" : (m_value&7)== 2 ? "circle" : (m_value&7)== 3 ? "disc" : "accent");
      state.m_font.m_propertyList.insert("style:text-emphasize", (what+((m_value&0x1000) ? " above" : " below")).c_str());
    }
    else
      state.m_font.m_propertyList.insert("style:text-emphasize", "none");
  }
}

void StarCAttributeVoid::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_type==ATTR_TXT_SOFTHYPH)
    state.m_font.m_softHyphen=true;
  else if (m_type==ATTR_EE_FEATURE_TAB)
    state.m_font.m_tab=true;
  else if (m_type==ATTR_EE_FEATURE_LINEBR)
    state.m_font.m_lineBreak=true;
}

}

namespace StarCharAttribute
{
// ------------------------------------------------------------
////////////////////////////////////////
//! Internal: the subdocument of a StarObjectSpreadsheet
class SubDocument final : public STOFFSubDocument
{
public:
  SubDocument(std::shared_ptr<StarObjectTextInternal::Content> const &content, std::shared_ptr<StarState::GlobalState> const &state) :
    STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry()), m_content(content), m_state(state) {}

  //! destructor
  ~SubDocument() final {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const final
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    auto const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_state.get() != sDoc->m_state.get()) return true;
    return false;
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;

protected:
  //! the content
  std::shared_ptr<StarObjectTextInternal::Content> m_content;
  //! the global state
  std::shared_ptr<StarState::GlobalState> m_state;
private:
  SubDocument(SubDocument const &);
  SubDocument &operator=(SubDocument const &);
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSpreadsheetInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (m_content) {
    StarState state(*m_state);
    m_content->send(listener, state);
  }
}

//! a escapement attribute
class StarCAttributeEscapement final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeEscapement(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_delta(0), m_scale(100)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeEscapement(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_delta) o << "decal=" << m_delta << "%,";
    if (m_scale!=100) o << "scale=" << m_scale << "%,";
    o << "],";
  }
protected:
  //! copy constructor
  StarCAttributeEscapement(StarCAttributeEscapement const &) = default;
  //! the sub/super decal in %
  int m_delta;
  //! the scaling
  int m_scale;
};

//! a font attribute
class StarCAttributeFont final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeFont(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_name(""), m_style(""), m_encoding(0), m_family(0), m_pitch(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeFont(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (!m_name.empty()) o << "name=" << m_name.cstr() << ",";
    if (!m_style.empty()) o << "style=" << m_style.cstr() << ",";
    if (m_encoding) o << "encoding=" << m_encoding << ",";
    switch (m_family) {
    case 0: // dont know
      break;
    case 1:
      o << "decorative,";
      break;
    case 2:
      o << "modern,";
      break;
    case 3:
      o << "roman,";
      break;
    case 4:
      o << "script,";
      break;
    case 5:
      o << "swiss,";
      break;
    case 6:
      o << "modern,";
      break;
    default:
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont: find unknown family\n"));
      o << "#family=" << m_family << ",";
      break;
    }
    switch (m_pitch) {
    case 0: // dont know
      break;
    case 1:
      o << "pitch[fixed],";
      break;
    case 2:
      o << "pitch[fixed],";
      break;
    default:
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont: find unknown pitch\n"));
      o << "#pitch=" << m_pitch << ",";
      break;
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarCAttributeFont(StarCAttributeFont const &) = default;
  //! the font name
  librevenge::RVNGString m_name;
  //! the style
  librevenge::RVNGString m_style;
  //! the font encoding
  int m_encoding;
  //! the font family
  int m_family;
  //! the font pitch
  int m_pitch;
};


//! a font size attribute
class StarCAttributeFontSize final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeFontSize(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_size(240), m_proportion(100), m_unit(13)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeFontSize(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_size!=240) o << "sz=" << m_size << ",";
    if (m_proportion!=100) o << "prop=" << m_proportion << ",";
    if (m_unit>=0 && m_unit<=14) {
      char const *wh[]= {"mm/100", "mm/10", "mm", "cm", "in/1000", "in/100", "in/10", "in",
                         "pt", "twip", "pixel", "sys[font]", "app[font]", "rel", "abs"
                        };
      o << wh[m_unit] << ",";
    }
    else {
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFontSize::print: unknown font unit\n"));
      o << "#unit=" << m_unit << ",";
    }
    o << "],";
  }

protected:
  //! copy constructor
  StarCAttributeFontSize(StarCAttributeFontSize const &) = default;
  //! the font size
  int m_size;
  //! the font proportion
  int m_proportion;
  //! the font unit
  int m_unit;
};

//! a charFormat attribute
class StarCAttributeCharFormat final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeCharFormat(Type type, std::string const &debugName)
    : StarAttribute(type, debugName), m_name("")
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeCharFormat(*this));
  }
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName;
    if (!m_name.empty()) o << "=" << m_name.cstr();
    o << ",";
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarCAttributeCharFormat(StarCAttributeCharFormat const &) = default;
  //! the charFormat
  librevenge::RVNGString m_name;
};

//! a content attribute
class StarCAttributeContent final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeContent(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_content()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeContent(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (!m_content) o << "empty,";
    o << "],";
  }
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const final;
protected:
  //! copy constructor
  StarCAttributeContent(StarCAttributeContent const &) = default;
  //! the content
  std::shared_ptr<StarObjectTextInternal::Content> m_content;
};

//! a field attribute
class StarCAttributeField final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeField(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_field()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeField(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
protected:
  //! copy constructor
  StarCAttributeField(StarCAttributeField const &) = default;
  //! the field
  std::shared_ptr<SWFieldManagerInternal::Field> m_field;
};

//! a txtFlyCnt attribute
class StarCAttributeFlyCnt final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeFlyCnt(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_format()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeFlyCnt(*this));
  }
  //! add to the state
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << ",";
  }
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const final;
protected:
  //! copy constructor
  StarCAttributeFlyCnt(StarCAttributeFlyCnt const &) = default;
  //! the format
  std::shared_ptr<StarFormatManagerInternal::FormatDef> m_format;
};

//! a footnote attribute
class StarCAttributeFootnote final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeFootnote(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_number(0)
    , m_label("")
    , m_content()
    , m_numSeq(0)
    , m_flags(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeFootnote(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (m_number) o << "number=" << m_number << ",";
    if (!m_label.empty()) o << "label=" << m_label.cstr() << ",";
    if (!m_content) o << "empty,";
    if (m_numSeq) o << "numSeq=" << m_numSeq << ",";
    if (m_flags) o << "flags=" << std::hex << m_flags << std::dec << ",";
    o << "],";
  }
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const final;
protected:
  //! copy constructor
  StarCAttributeFootnote(StarCAttributeFootnote const &) = default;
  //! the numbering
  int m_number;
  //! the label
  librevenge::RVNGString m_label;
  //! the content
  std::shared_ptr<StarObjectTextInternal::Content> m_content;
  //! the sequential number
  int m_numSeq;
  //! the flags
  int m_flags;
};


//! a hardBlank attribute
class StarCAttributeHardBlank final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeHardBlank(Type type, std::string const &debugName) : StarAttribute(type, debugName), m_char(0)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeHardBlank(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName;
    if (m_char) o << "=" << char(m_char);
    o << ",";
  }
protected:
  //! copy constructor
  StarCAttributeHardBlank(StarCAttributeHardBlank const &) = default;
  //! the character
  uint8_t m_char;
};

//! a INetFmt attribute: ie. a link, ...
class StarCAttributeINetFmt final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeINetFmt(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_url("")
    , m_target("")
    , m_name("")
    , m_libNames()
  {
    for (int &indice : m_indices) indice=0xFFFF;
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeINetFmt(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName << "=[";
    if (!m_url.empty()) o << "url=" << m_url.cstr() << ",";
    if (!m_target.empty()) o << "target=" << m_target.cstr() << ",";
    if (!m_name.empty()) o << "name=" << m_name.cstr() << ",";
    for (int i=0; i<2; ++i) {
      if (m_indices[i]!=0xFFFF) o << "index" << i << "=" << m_indices[i] << ",";
    }
    if (!m_libNames.empty()) {
      o << "libNames=[";
      for (size_t i=0; i+1<m_libNames.size(); i+=2)
        o << m_libNames[i].cstr() << ":" <<  m_libNames[i+1].cstr() << ",";
      o << "],";
    }
    o << "],";
  }
protected:
  //! copy constructor
  StarCAttributeINetFmt(StarCAttributeINetFmt const &) = default;
  //! the url
  librevenge::RVNGString m_url;
  //! the target
  librevenge::RVNGString m_target;
  //! the name
  librevenge::RVNGString m_name;
  //! two indices
  int m_indices[2];
  //! the lib names
  std::vector<librevenge::RVNGString> m_libNames;
  // also a list of key, name1, name2, scriptType
};

//! a refMark attribute
class StarCAttributeRefMark final : public StarAttribute
{
public:
  //! constructor
  StarCAttributeRefMark(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
    , m_name("")
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarCAttributeRefMark(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) final;
  //! add to a font
  void addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const final;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const final
  {
    o << m_debugName;
    if (!m_name.empty()) o << "=" << m_name.cstr();
    o << ",";
  }
protected:
  //! copy constructor
  StarCAttributeRefMark(StarCAttributeRefMark const &) = default;
  //! the name
  librevenge::RVNGString m_name;
};

void StarCAttributeEscapement::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  std::stringstream s;
  s << m_delta << "% " << m_scale << "%";
  state.m_font.m_propertyList.insert("style:text-position", s.str().c_str());
}

void StarCAttributeFont::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (!m_name.empty()) {
    if (m_type==ATTR_CHR_FONT)
      state.m_font.m_propertyList.insert("style:font-name", m_name);
    else if (m_type==ATTR_CHR_CJK_FONT)
      state.m_font.m_propertyList.insert("style:font-name-asian", m_name);
    else if (m_type==ATTR_CHR_CTL_FONT)
      state.m_font.m_propertyList.insert("style:font-name-complex", m_name);
  }
  if (m_pitch==1 || m_pitch==2) {
    if (m_type==ATTR_CHR_FONT)
      state.m_font.m_propertyList.insert("style:font-pitch", m_pitch==1 ? "fixed" : "variable");
    else if (m_type==ATTR_CHR_CJK_FONT)
      state.m_font.m_propertyList.insert("style:font-pitch-asian", m_pitch==1 ? "fixed" : "variable");
    else if (m_type==ATTR_CHR_CTL_FONT)
      state.m_font.m_propertyList.insert("style:font-pitch-complex", m_pitch==1 ? "fixed" : "variable");
  }
  // TODO m_style, m_family
}

void StarCAttributeFontSize::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  std::string wh(m_type==ATTR_CHR_FONTSIZE ? "fo:font-size" :
                 m_type==ATTR_CHR_CJK_FONTSIZE ? "style:font-size-asian" :
                 m_type==ATTR_CHR_CTL_FONTSIZE ? "style:font-size-complex" :
                 "");
  if (wh.empty())
    return;
  // TODO: use m_proportion?
  switch (m_unit) {
  case 0:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)*0.02756, librevenge::RVNG_POINT);
    break;
  case 1:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)*0.2756, librevenge::RVNG_POINT);
    break;
  case 2:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)*2.756, librevenge::RVNG_POINT);
    break;
  case 3:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)*27.56, librevenge::RVNG_POINT);
    break;
  case 4:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)/1000., librevenge::RVNG_INCH);
    break;
  case 5:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)/100., librevenge::RVNG_INCH);
    break;
  case 6:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)/10., librevenge::RVNG_INCH);
    break;
  case 7:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size), librevenge::RVNG_INCH);
    break;
  case 8:
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size), librevenge::RVNG_POINT);
    break;
  case 9: // TWIP
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)/20., librevenge::RVNG_POINT);
    break;
  case 10: // pixel
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size), librevenge::RVNG_POINT);
    break;
  case 13: // rel, checkme
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)*state.m_global->m_relativeUnit, librevenge::RVNG_POINT);
    break;
  default: // checkme
    state.m_font.m_propertyList.insert(wh.c_str(), double(m_size)/20., librevenge::RVNG_POINT);
    break;
  }
}

void StarCAttributeCharFormat::addTo(StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this) != done.end())
    return;
  done.insert(this);
  if (m_type==ATTR_TXT_CHARFMT) {
    if (m_name.empty() || !state.m_global->m_pool) return;
    auto const *style=state.m_global->m_pool->findStyleWithFamily(m_name, StarItemStyle::F_Char);
    if (style) {
      state.m_font=STOFFFont();
      StarItemSet const &itemSet=style->m_itemSet;
      std::map<int, std::shared_ptr<StarItem> >::const_iterator it;
      for (it=itemSet.m_whichToItemMap.begin(); it!=itemSet.m_whichToItemMap.end(); ++it) {
        if (it->second && it->second->m_attribute)
          it->second->m_attribute->addTo(state, done);
      }

    }
  }
}

void StarCAttributeContent::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  state.m_content=true;
}

void StarCAttributeField::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  state.m_field=m_field;
}

void StarCAttributeFlyCnt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  state.m_flyCnt=true;
}

void StarCAttributeFootnote::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  state.m_footnote=true;
}

void StarCAttributeHardBlank::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  state.m_font.m_hardBlank=true;
}

void StarCAttributeINetFmt::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  if (m_url.empty()) {
    STOFF_DEBUG_MSG(("StarCAttributeINetFmt::addTo: can not find the url\n"));
    return;
  }
  state.m_link=m_url;
}

void StarCAttributeRefMark::addTo(StarState &state, std::set<StarAttribute const *> &/*done*/) const
{
  state.m_refMark=m_name;
}

bool StarCAttributeEscapement::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_scale=int(input->readULong(1));
  m_delta=int(input->readLong(2));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeFont::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";

  m_family=int(input->readULong(1));
  m_pitch=int(input->readULong(1));
  m_encoding=int(input->readULong(1));
  std::vector<uint32_t> fName, string;
  if (!zone.readString(fName)) {
    STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the name\n"));
    printData(f);
    f << "###aName,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_name=libstoff::getString(fName);
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the style\n"));
    printData(f);
    f << "###aStyle,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_style=libstoff::getString(string).cstr();
  if (m_encoding!=10 && libstoff::getString(fName)=="StarBats" && input->tell()<endPos) {
    if (input->readULong(4)==0xFE331188) {
      // reread data in unicode
      if (!zone.readString(fName)) {
        STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the name\n"));
        printData(f);
        f << "###aName,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!fName.empty())
        f << "aNameUni=" << libstoff::getString(fName).cstr() << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the style\n"));
        printData(f);
        f << "###aStyle,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!string.empty())
        f << "aStyleUni=" << libstoff::getString(string).cstr() << ",";
    }
    else input->seek(-3, librevenge::RVNG_SEEK_CUR);
  }

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeFontSize::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_size=int(input->readULong(2));
  m_proportion=int(input->readULong((nVers>=1) ? 2 : 1));
  if (nVers>=2) m_unit=int(input->readULong(2));
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeCharFormat::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // fmtAttr2: SwFmtCharFmt
  auto id=int(input->readULong(2));
  if (!zone.getPoolName(id, m_name)) {
    STOFF_DEBUG_MSG(("StarCAttributeCharFormat::read: can not find the style name\n"));
    f << "###id=" << id << ",";
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeContent::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  StarObjectText text(object, false); // checkme
  if (!text.readSWContent(zone, m_content)) {
    STOFF_DEBUG_MSG(("StarCAttributeContent::read: can not find the content\n"));
    f << "###aContent,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeField::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  SWFieldManager fieldManager;
  if (m_type==StarAttribute::ATTR_TXT_FIELD)
    m_field=fieldManager.readField(zone);
  else
    m_field=fieldManager.readPersistField(zone, endPos);
  if (!m_field || input->tell()>endPos) {
    STOFF_DEBUG_MSG(("StarCAttributeField::read: can not find the field\n"));
    printData(f);
    f << "###field,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return m_field && input->tell()<=endPos;
}

bool StarCAttributeFlyCnt::read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (input->peek()=='o')
    object.getFormatManager()->readSWFormatDef(zone,'o', m_format, object);
  else
    object.getFormatManager()->readSWFormatDef(zone,'l', m_format, object);
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeFootnote::read(StarZone &zone, int nVers, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // sw_sw3npool.cxx SwFmtFtn::Create
  m_number=int(input->readULong(2));
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarCAttributeFootnote::read: can not find the aNumber\n"));
    printData(f);
    f << "###aNumber,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (!string.empty())
    m_label=libstoff::getString(string).cstr();
  // no sure, find this attribute once with a content here, so ...
  StarObjectText text(object, false); // checkme
  if (!text.readSWContent(zone, m_content)) {
    STOFF_DEBUG_MSG(("StarCAttributeFootnote::read: can not find the content\n"));
    f << "###aContent,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (nVers>=1)
    m_numSeq=int(input->readULong(2));
  if (nVers>=2)
    m_flags=int(input->readULong(1));

  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeHardBlank::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  if (nVers>=1)
    *input >> m_char;
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeINetFmt::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  // SwFmtINetFmt::Create
  std::vector<uint32_t> string;
  for (int i=0; i<2; ++i) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarCAttributeINetFmt::read: can not find string\n"));
      f << "###" << (i==0 ? "url" : "target") << ",";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    if (i==0)
      m_url=libstoff::getString(string);
    else
      m_target=libstoff::getString(string);
  }
  for (int &indice : m_indices) indice=int(input->readULong(2));
  auto nCnt=int(input->readULong(2));
  for (int i=0; i<2*nCnt; ++i) {
    if (!zone.readString(string) || input->tell()>endPos) {
      STOFF_DEBUG_MSG(("StarCAttributeINetFmt::read: can not read a string\n"));
      printData(f);
      f << "###string,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    m_libNames.push_back(libstoff::getString(string));
  }
  if (nVers>=1) {
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarCAttributeINetFmt::read: can not find string\n"));
      printData(f);
      f << "###aName1,";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    m_name=libstoff::getString(string);
  }
  if (nVers>=2) {
    nCnt=int(input->readULong(2));
    f << "libMac2=[";
    for (int i=0; i<nCnt; ++i) {
      f << "nCurKey=" << input->readULong(2) << ",";
      if (!zone.readString(string) || input->tell()>endPos) {
        STOFF_DEBUG_MSG(("StarCAttributeINetFmt::read: can not read a string\n"));
        f << "###aName1,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr() << ":";
      if (!zone.readString(string)|| input->tell()>endPos) {
        STOFF_DEBUG_MSG(("StarCAttributeINetFmt::read: can not read a string\n"));
        f << "###aName1,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      else if (!string.empty())
        f << libstoff::getString(string).cstr();
      f << "nScriptType=" << input->readULong(2) << ",";
    }
    f << "],";
  }
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeRefMark::read(StarZone &zone, int /*nVers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  std::vector<uint32_t> string;
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarCAttributeRefMark::read: can not find the name\n"));
    f << "###name,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_name=libstoff::getString(string);
  printData(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarCAttributeContent::send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarCAttributeContent::send: find a loop\n"));
    return false;
  }
  done.insert(this);
  if (!listener) {
    STOFF_DEBUG_MSG(("StarCAttributeContent::send: can not find the listener\n"));
    return false;
  }
  if (m_content) // checkme zone time, we need probably to create a frame
    m_content->send(listener, state, !state.m_headerFooter);
  return true;
}

bool StarCAttributeFlyCnt::send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarCAttributeFlyCnt::send: find a loop\n"));
    return false;
  }
  done.insert(this);
  if (!listener) {
    STOFF_DEBUG_MSG(("StarCAttributeFlyCnt::send: can not find the listener\n"));
    return false;
  }
  if (!m_format) {
    STOFF_DEBUG_MSG(("StarCAttributeFlyCnt::send: can not find the format\n"));
    return false;
  }
  return m_format->send(listener, state);
}

bool StarCAttributeFootnote::send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarCAttributeFootnote::send: find a loop\n"));
    return false;
  }
  done.insert(this);
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("StarCAttributeFootnote::send: can not find the listener\n"));
    return false;
  }
  STOFFSubDocumentPtr subDocument(new SubDocument(m_content, state.m_global));
  STOFFNote note(STOFFNote::FootNote);
  if (m_label.empty())
    note.m_label=m_label;
  else
    note.m_number=m_number;
  listener->insertNote(note, subDocument);
  return true;
}

}

namespace StarCharAttribute
{
void addInitTo(std::map<int, std::shared_ptr<StarAttribute> > &map)
{
  map[StarAttribute::ATTR_CHR_FONT]=std::shared_ptr<StarAttribute>(new StarCAttributeFont(StarAttribute::ATTR_CHR_FONT,"chrAtrFont"));
  map[StarAttribute::ATTR_CHR_CJK_FONT]=std::shared_ptr<StarAttribute>(new StarCAttributeFont(StarAttribute::ATTR_CHR_CJK_FONT,"chrAtrCJKFont"));
  map[StarAttribute::ATTR_CHR_CTL_FONT]=std::shared_ptr<StarAttribute>(new StarCAttributeFont(StarAttribute::ATTR_CHR_CTL_FONT,"chrAtrCTLFont"));
  map[StarAttribute::ATTR_CHR_FONTSIZE]=std::shared_ptr<StarAttribute>(new StarCAttributeFontSize(StarAttribute::ATTR_CHR_FONTSIZE,"chrAtrFontsize"));
  map[StarAttribute::ATTR_CHR_CJK_FONTSIZE]=std::shared_ptr<StarAttribute>(new StarCAttributeFontSize(StarAttribute::ATTR_CHR_CJK_FONTSIZE,"chrAtrCJKFontsize"));
  map[StarAttribute::ATTR_CHR_CTL_FONTSIZE]=std::shared_ptr<StarAttribute>(new StarCAttributeFontSize(StarAttribute::ATTR_CHR_CTL_FONTSIZE,"chrAtrCTLFontsize"));
  addAttributeColor(map, StarAttribute::ATTR_CHR_COLOR,"char[color]",STOFFColor::black());
  // bold
  addAttributeUInt(map,StarAttribute::ATTR_CHR_WEIGHT,"char[weight]",1,5); // normal
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CJK_WEIGHT,"char[cjk,weight]",1,5); // normal
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CTL_WEIGHT,"char[ctl,weight]",1,5); // normal
  // italic
  addAttributeUInt(map,StarAttribute::ATTR_CHR_POSTURE,"char[posture]",1,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CJK_POSTURE,"char[cjk,posture]",1,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CTL_POSTURE,"char[ctl,posture]",1,0); // none

  addAttributeUInt(map,StarAttribute::ATTR_CHR_CASEMAP,"char[casemap]",1,0); // no mapped
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CROSSEDOUT,"char[crossedout]",1,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_UNDERLINE,"char[underline]",1,0); // none
  addAttributeBool(map,StarAttribute::ATTR_CHR_CONTOUR,"char[contour]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_SHADOWED,"char[shadowed]",false);
  addAttributeUInt(map,StarAttribute::ATTR_CHR_RELIEF,"char[relief]",2,0); // none
  addAttributeBool(map,StarAttribute::ATTR_CHR_BLINK,"char[blink]",false);
  addAttributeUInt(map,StarAttribute::ATTR_CHR_EMPHASIS_MARK,"char[emphasisMark]",2,0); // none

  addAttributeBool(map,StarAttribute::ATTR_CHR_WORDLINEMODE,"char[word,linemode]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_AUTOKERN,"char[autoKern]",false);
  addAttributeInt(map,StarAttribute::ATTR_CHR_KERNING,"char[kerning]",2,0);
  map[StarAttribute::ATTR_CHR_ESCAPEMENT]=std::shared_ptr<StarAttribute>(new StarCAttributeEscapement(StarAttribute::ATTR_CHR_ESCAPEMENT,"chrAtrEscapement"));
  addAttributeUInt(map,StarAttribute::ATTR_CHR_PROPORTIONALFONTSIZE,"char[proportionalfontsize]",2,100); // 100%

  addAttributeUInt(map,StarAttribute::ATTR_CHR_LANGUAGE,"char[language]",2,0x3ff); // unknown
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CJK_LANGUAGE,"char[cjk,language]",2,0x3ff); // unknown
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CTL_LANGUAGE,"char[ctl,language]",2,0x3ff); // unknown
  addAttributeUInt(map,StarAttribute::ATTR_SC_LANGUAGE_FORMAT,"cell[language]",2,0x3ff); // unknown

  addAttributeBool(map,StarAttribute::ATTR_CHR_NOHYPHEN,"char[noHyphen]",true);
  addAttributeVoid(map,StarAttribute::ATTR_TXT_SOFTHYPH,"text[softHyphen]");
  map[StarAttribute::ATTR_TXT_CHARFMT]=std::shared_ptr<StarAttribute>(new StarCAttributeCharFormat(StarAttribute::ATTR_TXT_CHARFMT,"textAtrCharFmt"));
  map[StarAttribute::ATTR_TXT_FTN]=std::shared_ptr<StarAttribute>(new StarCAttributeFootnote(StarAttribute::ATTR_TXT_FTN,"textAtrFtn"));
  map[StarAttribute::ATTR_TXT_FIELD]=std::shared_ptr<StarAttribute>(new StarCAttributeField(StarAttribute::ATTR_TXT_FIELD,"textAtrField"));
  map[StarAttribute::ATTR_TXT_FLYCNT]=std::shared_ptr<StarAttribute>(new StarCAttributeFlyCnt(StarAttribute::ATTR_TXT_FLYCNT,"textAtrTxtFlyCnt"));
  map[StarAttribute::ATTR_TXT_HARDBLANK]=std::shared_ptr<StarAttribute>(new StarCAttributeHardBlank(StarAttribute::ATTR_TXT_HARDBLANK,"textAtrHardBlank"));
  map[StarAttribute::ATTR_TXT_INETFMT]=std::shared_ptr<StarAttribute>(new StarCAttributeINetFmt(StarAttribute::ATTR_TXT_INETFMT,"textAtrInetFmt"));
  map[StarAttribute::ATTR_TXT_REFMARK]=std::shared_ptr<StarAttribute>(new StarCAttributeRefMark(StarAttribute::ATTR_TXT_REFMARK,"textAtrRefMark"));
  addAttributeBool(map,StarAttribute::ATTR_CHR_NOLINEBREAK,"char[nolineBreak]",true);
  addAttributeBool(map,StarAttribute::ATTR_SC_HYPHENATE,"hyphenate", false);

  addAttributeVoid(map, StarAttribute::ATTR_EE_FEATURE_TAB, "feature[tab]");
  addAttributeVoid(map, StarAttribute::ATTR_EE_FEATURE_LINEBR, "feature[linebr]");
  map[StarAttribute::ATTR_EE_FEATURE_FIELD]=std::shared_ptr<StarAttribute>(new StarCAttributeField(StarAttribute::ATTR_EE_FEATURE_FIELD,"eeFeatureField"));

  addAttributeBool(map,StarAttribute::ATTR_CHR_DUMMY1,"char[dummy1]",false);
  addAttributeBool(map,StarAttribute::ATTR_TXT_DUMMY1,"text[dummy1]",false);
  addAttributeBool(map,StarAttribute::ATTR_TXT_DUMMY2,"text[dummy2]",false);
  addAttributeBool(map,StarAttribute::ATTR_TXT_DUMMY4,"text[dummy4]",false);
  addAttributeBool(map,StarAttribute::ATTR_TXT_DUMMY5,"text[dummy5]",false);
  addAttributeBool(map,StarAttribute::ATTR_TXT_DUMMY6,"text[dummy6]",false);
  addAttributeBool(map,StarAttribute::ATTR_TXT_DUMMY7,"text[dummy7]",false);
  addAttributeVoid(map,StarAttribute::ATTR_TXT_UNKNOWN_CONTAINER, "text[unknContainer]"); // XML attrib

  map[StarAttribute::ATTR_FRM_CNTNT]=std::shared_ptr<StarAttribute>(new StarCAttributeContent(StarAttribute::ATTR_FRM_CNTNT,"pageCntnt"));


}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
