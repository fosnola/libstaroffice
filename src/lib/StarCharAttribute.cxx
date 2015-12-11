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

#include "STOFFFont.hxx"

#include "StarAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarZone.hxx"

#include "StarCharAttribute.hxx"

namespace StarCharAttribute
{
//! a character bool attribute
class StarCAttributeBool : public StarAttributeBool
{
public:
  //! constructor
  StarCAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeBool(*this));
  }
protected:
  //! copy constructor
  StarCAttributeBool(StarCAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

//! a character color attribute
class StarCAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarCAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeColor(*this));
  }
  //! add to a font
  virtual void addTo(STOFFFont &font) const;
protected:
  //! copy constructor
  StarCAttributeColor(StarCAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

//! a character integer attribute
class StarCAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarCAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarCAttributeInt(StarCAttributeInt const &orig) : StarAttributeInt(orig)
  {
  }
};

//! a character unsigned integer attribute
class StarCAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarCAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeUInt(*this));
  }
  //! add to a font
  virtual void addTo(STOFFFont &font) const;
protected:
  //! copy constructor
  StarCAttributeUInt(StarCAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};


//! add a bool attribute
void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeBool(type,debugName, defValue));
}
//! add a color attribute
void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeColor(type,debugName, defValue));
}
//! add a int attribute
void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarCAttributeUInt(type,debugName, numBytes, defValue));
}

void StarCAttributeColor::addTo(STOFFFont &font) const
{
  if (m_type==ATTR_CHR_COLOR)
    font.m_propertyList.insert("fo:color", m_value.str().c_str());
}

void StarCAttributeUInt::addTo(STOFFFont &font) const
{
  if (m_type==ATTR_CHR_CROSSEDOUT) {
    switch (m_value) {
    case 0: // none
      break;
    case 1: // single
    case 2: // double
      font.m_propertyList.insert("style:text-line-through", m_value==1 ? "single" : "double");
      font.m_propertyList.insert("style:text-line-through-style", "solid");
      break;
    case 3: // dontknow
      break;
    case 4: // bold
      font.m_propertyList.insert("style:text-line-through", "single");
      font.m_propertyList.insert("style:text-line-through-style", "solid");
      font.m_propertyList.insert("style:text-line-through-width", "thick");
      break;
    case 5: // slash
    case 6: // X
      font.m_propertyList.insert("style:text-line-through", "single");
      font.m_propertyList.insert("style:text-line-through-style", "solid");
      font.m_propertyList.insert("style:text-line-through-text", m_value==5 ? "/" : "X");
      break;
    default:
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown crossedout enum=%d\n", m_value));
      break;
    }
  }
  else if (m_type==ATTR_CHR_UNDERLINE) {
    switch (m_value) {
    case 0: // none
    case 4: // unknown
      break;
    case 1: // single
    case 2: // double
      font.m_propertyList.insert("style:text-underline", m_value==1 ? "single" : "double");
      font.m_propertyList.insert("style:text-underline-style", "solid");
      break;
    case 3: // dot
    case 5: // dash
    case 6: // long-dash
    case 7: // dash-dot
    case 8: // dash-dot-dot
      font.m_propertyList.insert("style:text-underline", "single");
      font.m_propertyList.insert("style:text-underline-style", m_value==3 ? "dotted" : m_value==5 ? "dash" :
                                 m_value==6 ? "long-dash" : m_value==7 ? "dot-dash" : "dot-dot-dash");
      break;
    case 9: // small wave
    case 10: // wave
    case 11: // double wave
      font.m_propertyList.insert("style:text-underline", m_value==11 ? "single" : "double");
      font.m_propertyList.insert("style:text-underline-style", "wave");
      if (m_value==9)
        font.m_propertyList.insert("style:text-underline-width", "thin");
      break;
    case 12: // bold
    case 13: // bold dot
    case 14: // bold dash
    case 15: // bold long-dash
    case 16: // bold dash-dot
    case 17: // bold dash-dot-dot
    case 18: { // bold wave
      char const *(wh[])= {"solid", "dotted", "dash", "long-dash", "dot-dash", "dot-dot-dash", "wave"};
      font.m_propertyList.insert("style:text-underline", "single");
      font.m_propertyList.insert("style:text-underline-style", wh[m_value-12]);
      font.m_propertyList.insert("style:text-underline-width", "thick");
      break;
    }
    default:
      STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeUInt: find unknown underline enum=%d\n", m_value));
      break;
    }
  }
  else if (m_type==ATTR_CHR_POSTURE || m_type==ATTR_CHR_CJK_POSTURE || m_type==ATTR_CHR_CTL_POSTURE) {
    std::string wh(m_type==ATTR_CHR_POSTURE ? "fo:font-style" :  m_type==ATTR_CHR_CJK_POSTURE ? "style:font-style-asian" : "style:font-style-compllex");
    if (m_value==1)
      font.m_propertyList.insert(wh.c_str(), "oblique");
    else if (m_value==2)
      font.m_propertyList.insert(wh.c_str(), "italic");
  }
  else if (m_type==ATTR_CHR_WEIGHT || m_type==ATTR_CHR_CJK_WEIGHT || m_type==ATTR_CHR_CTL_WEIGHT) {
    std::string wh(m_type==ATTR_CHR_WEIGHT ? "fo:font-weight" :  m_type==ATTR_CHR_CJK_WEIGHT ? "style:font-weight-asian" : "style:font-weight-compllex");
    if (m_value==5)
      font.m_propertyList.insert(wh.c_str(), "normal");
    else if (m_value==8)
      font.m_propertyList.insert(wh.c_str(), "bold");
    else if (m_value>=1 && m_value<=9)
      font.m_propertyList.insert(wh.c_str(), m_value*100, librevenge::RVNG_GENERIC);
    // 10: WEIGHT_BLACK
  }
}
}

namespace StarCharAttribute
{
// ------------------------------------------------------------

//! a font attribute
class StarCAttributeFont : public StarAttribute
{
public:
  //! constructor
  StarCAttributeFont(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_name(""), m_style(""), m_encoding(0), m_family(0), m_pitch(0)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeFont(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFFont &font) const;
  //! debug function to print the data
  virtual void print(std::ostream &o) const
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
  StarCAttributeFont(StarCAttributeFont const &orig) : StarAttribute(orig), m_name(orig.m_name), m_style(orig.m_style), m_encoding(orig.m_encoding), m_family(orig.m_family), m_pitch(orig.m_pitch)
  {
  }
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

void StarCAttributeFont::addTo(STOFFFont &font) const
{
  if (!m_name.empty()) {
    if (m_type==ATTR_CHR_FONT)
      font.m_propertyList.insert("style:font-name", m_name);
    else if (m_type==ATTR_CHR_CJK_FONT)
      font.m_propertyList.insert("style:font-name-asian", m_name);
    else if (m_type==ATTR_CHR_CTL_FONT)
      font.m_propertyList.insert("style:font-name-complex", m_name);
  }
  if (m_pitch==1 || m_pitch==2) {
    if (m_type==ATTR_CHR_FONT)
      font.m_propertyList.insert("style:font-pitch", m_pitch==1 ? "fixed" : "variable");
    else if (m_type==ATTR_CHR_CJK_FONT)
      font.m_propertyList.insert("style:font-pitch-asian", m_pitch==1 ? "fixed" : "variable");
    else if (m_type==ATTR_CHR_CTL_FONT)
      font.m_propertyList.insert("style:font-pitch-complex", m_pitch==1 ? "fixed" : "variable");
  }
  // TODO m_style, m_family
}

bool StarCAttributeFont::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";

  m_family=(int) input->readULong(1);
  m_pitch=(int) input->readULong(1);
  m_encoding=(int) input->readULong(1);
  std::vector<uint32_t> fName, string;
  if (!zone.readString(fName)) {
    STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the name\n"));
    print(f);
    f << "###aName,";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  m_name=libstoff::getString(fName);
  if (!zone.readString(string)) {
    STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the style\n"));
    print(f);
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
        print(f);
        f << "###aName,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      if (!fName.empty())
        f << "aNameUni=" << libstoff::getString(fName).cstr() << ",";
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarCharAttribute::StarCAttributeFont::read: can not find the style\n"));
        print(f);
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

  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

//! a font size attribute
class StarCAttributeFontSize : public StarAttribute
{
public:
  //! constructor
  StarCAttributeFontSize(Type type, std::string const &debugName) :
    StarAttribute(type, debugName), m_size(240), m_proportion(100), m_unit(13)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarCAttributeFontSize(*this));
  }
  //! read a zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &object);
  //! add to a font
  virtual void addTo(STOFFFont &font) const;
  //! debug function to print the data
  virtual void print(std::ostream &o) const
  {
    o << m_debugName << "=[";
    if (m_size!=240) o << "sz=" << m_size << ",";
    if (m_proportion!=100) o << "prop=" << m_proportion << ",";
    if (m_unit>=0 && m_unit<=14) {
      char const *(wh[])= {"mm/100", "mm/10", "mm", "cm", "in/1000", "in/100", "in/10", "in",
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
  StarCAttributeFontSize(StarCAttributeFontSize const &orig) : StarAttribute(orig), m_size(orig.m_size), m_proportion(orig.m_proportion), m_unit(orig.m_unit)
  {
  }
  //! the font size
  int m_size;
  //! the font proportion
  int m_proportion;
  //! the font unit
  int m_unit;
};

bool StarCAttributeFontSize::read(StarZone &zone, int nVers, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";
  m_size=(int) input->readULong(2);
  m_proportion=(int) input->readULong((nVers>=1) ? 2 : 1);
  if (nVers>=2) m_unit=(int) input->readULong(2);
  print(f);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

void StarCAttributeFontSize::addTo(STOFFFont &font) const
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
    font.m_propertyList.insert(wh.c_str(), float(m_size)*0.02756f, librevenge::RVNG_POINT);
    break;
  case 1:
    font.m_propertyList.insert(wh.c_str(), float(m_size)*0.2756f, librevenge::RVNG_POINT);
    break;
  case 2:
    font.m_propertyList.insert(wh.c_str(), float(m_size)*2.756f, librevenge::RVNG_POINT);
    break;
  case 3:
    font.m_propertyList.insert(wh.c_str(), float(m_size)*27.56f, librevenge::RVNG_POINT);
    break;
  case 4:
    font.m_propertyList.insert(wh.c_str(), float(m_size)/1000.f, librevenge::RVNG_INCH);
    break;
  case 5:
    font.m_propertyList.insert(wh.c_str(), float(m_size)/100.f, librevenge::RVNG_INCH);
    break;
  case 6:
    font.m_propertyList.insert(wh.c_str(), float(m_size)/10.f, librevenge::RVNG_INCH);
    break;
  case 7:
    font.m_propertyList.insert(wh.c_str(), float(m_size), librevenge::RVNG_INCH);
    break;
  case 8:
    font.m_propertyList.insert(wh.c_str(), float(m_size), librevenge::RVNG_POINT);
    break;
  case 9: // TWIP
    font.m_propertyList.insert(wh.c_str(), float(m_size)/20., librevenge::RVNG_POINT);
    break;
  case 10: // pixel
    font.m_propertyList.insert(wh.c_str(), float(m_size), librevenge::RVNG_POINT);
    break;
  default: // checkme
    font.m_propertyList.insert(wh.c_str(), float(m_size)/20., librevenge::RVNG_POINT);
    break;
  }
}
}

namespace StarCharAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &map)
{
  map[StarAttribute::ATTR_CHR_FONT]=shared_ptr<StarAttribute>(new StarCAttributeFont(StarAttribute::ATTR_CHR_FONT,"chrAtrFont"));
  map[StarAttribute::ATTR_CHR_CJK_FONT]=shared_ptr<StarAttribute>(new StarCAttributeFont(StarAttribute::ATTR_CHR_CJK_FONT,"chrAtrCJKFont"));
  map[StarAttribute::ATTR_CHR_CTL_FONT]=shared_ptr<StarAttribute>(new StarCAttributeFont(StarAttribute::ATTR_CHR_CTL_FONT,"chrAtrCTLFont"));
  map[StarAttribute::ATTR_CHR_FONTSIZE]=shared_ptr<StarAttribute>(new StarCAttributeFontSize(StarAttribute::ATTR_CHR_FONTSIZE,"chrAtrFontsize"));
  map[StarAttribute::ATTR_CHR_CJK_FONTSIZE]=shared_ptr<StarAttribute>(new StarCAttributeFontSize(StarAttribute::ATTR_CHR_CJK_FONTSIZE,"chrAtrCJKFontsize"));
  map[StarAttribute::ATTR_CHR_CTL_FONTSIZE]=shared_ptr<StarAttribute>(new StarCAttributeFontSize(StarAttribute::ATTR_CHR_CTL_FONTSIZE,"chrAtrCTLFontsize"));
  addAttributeColor(map, StarAttribute::ATTR_CHR_COLOR,"char[color]",STOFFColor::black());
  // bold
  addAttributeUInt(map,StarAttribute::ATTR_CHR_WEIGHT,"char[weight]",1,5); // normal
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CJK_WEIGHT,"char[cjk,weight]",1,5); // normal
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CTL_WEIGHT,"char[ctl,weight]",1,5); // normal
  // italic
  addAttributeUInt(map,StarAttribute::ATTR_CHR_POSTURE,"char[posture]",1,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CJK_POSTURE,"char[cjk,posture]",1,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CTL_POSTURE,"char[ctl,posture]",1,0); // none
  addAttributeUInt(map, StarAttribute::ATTR_CHR_CROSSEDOUT,"char[crossedout]",1,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_UNDERLINE,"char[underline]",1,0); // none

  addAttributeUInt(map,StarAttribute::ATTR_CHR_CASEMAP,"char[casemap]",1,0); // no mapped
  addAttributeBool(map,StarAttribute::ATTR_CHR_CONTOUR,"char[contour]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_SHADOWED,"char[shadowed]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_WORDLINEMODE,"char[word,linemode]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_AUTOKERN,"char[autoKern]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_BLINK,"char[blink]",false);
  addAttributeBool(map,StarAttribute::ATTR_CHR_NOHYPHEN,"char[noHyphen]",true);
  addAttributeBool(map,StarAttribute::ATTR_CHR_NOLINEBREAK,"char[nolineBreak]",true);
  addAttributeUInt(map,StarAttribute::ATTR_CHR_KERNING,"char[kerning]",2,0);
  addAttributeUInt(map,StarAttribute::ATTR_CHR_LANGUAGE,"char[language]",2,0x3ff); // unknown
  addAttributeUInt(map,StarAttribute::ATTR_CHR_PROPORTIONALFONTSIZE,"char[proportionalfontsize]",2,100); // 100%
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CJK_LANGUAGE,"char[cjk,language]",2,0x3ff); // unknown
  addAttributeUInt(map,StarAttribute::ATTR_CHR_CTL_LANGUAGE,"char[ctl,language]",2,0x3ff); // unknown
  addAttributeUInt(map,StarAttribute::ATTR_CHR_EMPHASIS_MARK,"char[emphasisMark]",2,0); // none
  addAttributeUInt(map,StarAttribute::ATTR_CHR_RELIEF,"char[relief]",2,0); // none
  addAttributeBool(map,StarAttribute::ATTR_CHR_DUMMY1,"char[dummy1]",false);
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
