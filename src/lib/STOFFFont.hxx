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

#ifndef STOFF_FONT
#  define STOFF_FONT

#include <string>
#include <vector>

#include "libstaroffice_internal.hxx"

//! Class to store font
class STOFFFont
{
public:
  /** a small struct to define a line in STOFFFont */
  struct Line {
    /** the line style */
    enum Style { None, Simple, Dot, LargeDot, Dash, Wave };
    /** the line style */
    enum Type { Single, Double, Triple };
    //! constructor
    Line(Style style=None, Type type=Single, bool wordFlag=false, float w=1.0) :
      m_style(style), m_type(type), m_word(wordFlag), m_width(w), m_color(STOFFColor::black()) { }
    //! return true if the line is not empty
    bool isSet() const
    {
      return m_style != None && m_width>0;
    }
    //! add a line to the propList knowing the type (line-through, underline, overline )
    void addTo(librevenge::RVNGPropertyList &propList, std::string const &type) const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Line const &line);
    //! operator==
    bool operator==(Line const &oth) const
    {
      return cmp(oth)==0;
    }
    //! operator!=
    bool operator!=(Line const &oth) const
    {
      return cmp(oth)!=0;
    }
    //! small comparison function
    int cmp(Line const &oth) const
    {
      if (m_style != oth.m_style) return int(m_style)-int(oth.m_style);
      if (m_type != oth.m_type) return int(m_type)-int(oth.m_type);
      if (m_word != oth.m_word) return m_word ? -1 : 1;
      if (m_width < oth.m_width) return -1;
      if (m_width > oth.m_width) return 1;
      if (m_color.isSet() != oth.m_color.isSet())
        return m_color.isSet();
      if (m_color.get() < oth.m_color.get()) return -1;
      if (m_color.get() > oth.m_color.get()) return 1;
      return 0;
    }
    /** the style */
    Style m_style;
    /** the type */
    Type m_type;
    /** word or not word line */
    bool m_word;
    /** the width in point */
    float m_width;
    /** the color ( if not set, we use the font color )*/
    STOFFVariable<STOFFColor> m_color;
  };
  /** a small struct to define the script position in STOFFFont */
  struct Script {
    //! constructor
    Script(float delta=0, librevenge::RVNGUnit deltaUnit=librevenge::RVNG_PERCENT, int scale=100) :
      m_delta(delta), m_deltaUnit(deltaUnit), m_scale(scale)
    {
    }
    //! return true if the position is not default
    bool isSet() const
    {
      return *this != Script();
    }
    //! return a yposition which correspond to a basic subscript
    static Script sub()
    {
      return Script(-33,librevenge::RVNG_PERCENT,58);
    }
    //! return a yposition which correspond to a basic subscript100
    static Script sub100()
    {
      return Script(-20);
    }
    //! return a yposition which correspond to a basic superscript
    static Script super()
    {
      return Script(33,librevenge::RVNG_PERCENT,58);
    }
    //! return a yposition which correspond to a basic superscript100
    static Script super100()
    {
      return Script(20);
    }
    //! return a string which correspond to style:text-position
    std::string str(float fSize) const;

    //! operator==
    bool operator==(Script const &oth) const
    {
      return cmp(oth)==0;
    }
    //! operator!=
    bool operator!=(Script const &oth) const
    {
      return cmp(oth)!=0;
    }
    //! operator<
    bool operator<(Script const &oth) const
    {
      return cmp(oth)<0;
    }
    //! operator<=
    bool operator<=(Script const &oth) const
    {
      return cmp(oth)<=0;
    }
    //! operator>
    bool operator>(Script const &oth) const
    {
      return cmp(oth)>0;
    }
    //! operator>=
    bool operator>=(Script const &oth) const
    {
      return cmp(oth)>=0;
    }
    //! small comparison function
    int cmp(Script const &oth) const
    {
      if (m_delta > oth.m_delta) return -1;
      if (m_delta < oth.m_delta) return 1;
      if (m_deltaUnit != oth.m_deltaUnit) return int(m_deltaUnit)-int(oth.m_deltaUnit);
      if (m_scale != oth.m_scale) return m_scale-oth.m_scale;
      return 0;
    }
    //! the ydelta
    float m_delta;
    //! the ydelta unit ( point or percent )
    librevenge::RVNGUnit m_deltaUnit;
    //! the font scaling ( in percent )
    int m_scale;
  };

  //! the different font bit
  enum FontBits { boldBit=1, italicBit=2, blinkBit=4, embossBit=8, engraveBit=0x10,
                  hiddenBit=0x20, outlineBit=0x40, shadowBit=0x80,
                  reverseVideoBit=0x100, smallCapsBit=0x200, uppercaseBit=0x400,
                  lowercaseBit=0x800,
                  initialcaseBit=2*lowercaseBit,
                  boxedBit=2*initialcaseBit,
                  boxedRoundedBit=2*boxedBit,
                  reverseWritingBit=2*boxedRoundedBit
                };
  /** constructor
   *
   * \param name the font name
   * \param sz the font size
   * \param f the font attributes bold, ... */
  STOFFFont(librevenge::RVNGString const &name="", float sz=12, uint32_t f = 0) : m_name(name), m_size(sz), m_deltaSpacing(0), m_deltaSpacingUnit(librevenge::RVNG_POINT), m_widthStreching(1), m_scriptPosition(),
    m_flags(f), m_overline(Line::None), m_strikeoutline(Line::None), m_underline(Line::None),
    m_color(STOFFColor::black()), m_backgroundColor(STOFFColor::white()), m_language(""), m_extra("")
  {
    resetColor();
  };
  //! inserts the set value in the current font
  void insert(STOFFFont const &ft)
  {
    m_name.insert(ft.m_name);
    m_size.insert(ft.m_size);
    m_deltaSpacing.insert(ft.m_deltaSpacing);
    m_deltaSpacingUnit.insert(ft.m_deltaSpacingUnit);
    m_widthStreching.insert(ft.m_widthStreching);
    m_scriptPosition.insert(ft.m_scriptPosition);
    if (ft.m_flags.isSet()) {
      if (m_flags.isSet())
        setFlags(flags()| ft.flags());
      else
        m_flags = ft.m_flags;
    }
    m_overline.insert(ft.m_overline);
    m_strikeoutline.insert(ft.m_strikeoutline);
    m_underline.insert(ft.m_underline);
    m_color.insert(ft.m_color);
    m_extra += ft.m_extra;
  }
  //! returns the font name
  librevenge::RVNGString getFontName() const
  {
    return m_name.get();
  }
  //! sets the font id
  void setFontName(librevenge::RVNGString const &name)
  {
    m_name = name;
  }

  //! returns the font size
  float size() const
  {
    return m_size.get();
  }
  //! sets the font size
  void setSize(float sz)
  {
    m_size = sz;
  }

  //! returns the condensed(negative)/extended(positive) width
  float deltaLetterSpacing() const
  {
    return m_deltaSpacing.get();
  }
  //! returns the condensed(negative)/extended(positive) unit
  librevenge::RVNGUnit deltaLetterSpacingUnit() const
  {
    return m_deltaSpacingUnit.get();
  }
  //! sets the letter spacing ( delta value in point )
  void setDeltaLetterSpacing(float d, librevenge::RVNGUnit unit=librevenge::RVNG_POINT)
  {
    m_deltaSpacing=d;
    m_deltaSpacingUnit=unit;
  }
  //! returns the text width streching
  float widthStreching() const
  {
    return m_widthStreching.get();
  }
  //! sets the text width streching
  void setWidthStreching(float scale=1.0)
  {
    m_widthStreching = scale;
  }
  //! returns the script position
  Script const &script() const
  {
    return m_scriptPosition.get();
  }

  //! sets the script position
  void set(Script const &newscript)
  {
    m_scriptPosition = newscript;
  }

  //! returns the font flags
  uint32_t flags() const
  {
    return m_flags.get();
  }
  //! sets the font attributes bold, ...
  void setFlags(uint32_t fl)
  {
    m_flags = fl;
  }

  //! returns true if the font color is not black
  bool hasColor() const
  {
    return m_color.isSet() && !m_color.get().isBlack();
  }
  //! returns the font color
  void getColor(STOFFColor &c) const
  {
    c = m_color.get();
  }
  //! sets the font color
  void setColor(STOFFColor color)
  {
    m_color = color;
  }

  //! returns the font background color
  void getBackgroundColor(STOFFColor &c) const
  {
    c = m_backgroundColor.get();
  }
  //! sets the font background color
  void setBackgroundColor(STOFFColor color)
  {
    m_backgroundColor = color;
  }
  //! resets the font color to black and the background color to white
  void resetColor()
  {
    m_color = STOFFColor::black();
    m_backgroundColor = STOFFColor::white();
  }

  //! return true if the font has decorations line (overline, strikeout, underline)
  bool hasDecorationLines() const
  {
    return (m_overline.isSet() && m_overline->isSet()) ||
           (m_strikeoutline.isSet() && m_strikeoutline->isSet()) ||
           (m_underline.isSet() && m_underline->isSet());
  }
  //! reset the decoration
  void resetDecorationLines()
  {
    if (m_overline.isSet()) m_overline=Line(Line::None);
    if (m_strikeoutline.isSet()) m_strikeoutline=Line(Line::None);
    if (m_underline.isSet()) m_underline=Line(Line::None);
  }
  //! returns the overline
  Line const &getOverline() const
  {
    return m_overline.get();
  }
  //! sets the overline
  void setOverline(Line const &line)
  {
    m_overline = line;
  }
  //! sets the overline style ( by default, we also reset the style)
  void setOverlineStyle(Line::Style style=Line::None, bool doReset=true)
  {
    if (doReset)
      m_overline = Line(style);
    else
      m_overline->m_style = style;
  }
  //! sets the overline type
  void setOverlineType(Line::Type type=Line::Single)
  {
    m_overline->m_type = type;
  }
  //! sets the overline word flag
  void setOverlineWordFlag(bool wordFlag=false)
  {
    m_overline->m_word = wordFlag;
  }
  //! sets the overline width
  void setOverlineWidth(float w)
  {
    m_overline->m_width = w;
  }
  //! sets the overline color
  void setOverlineColor(STOFFColor const &color)
  {
    m_overline->m_color = color;
  }

  //! returns the strikeoutline
  Line const &getStrikeOut() const
  {
    return m_strikeoutline.get();
  }
  //! sets the strikeoutline
  void setStrikeOut(Line const &line)
  {
    m_strikeoutline = line;
  }
  //! sets the strikeoutline style ( by default, we also reset the style)
  void setStrikeOutStyle(Line::Style style=Line::None, bool doReset=true)
  {
    if (doReset)
      m_strikeoutline = Line(style);
    else
      m_strikeoutline->m_style = style;
  }
  //! sets the strikeoutline type
  void setStrikeOutType(Line::Type type=Line::Single)
  {
    m_strikeoutline->m_type = type;
  }
  //! sets the strikeoutline word flag
  void setStrikeOutWordFlag(bool wordFlag=false)
  {
    m_strikeoutline->m_word = wordFlag;
  }
  //! sets the strikeoutline width
  void setStrikeOutWidth(float w)
  {
    m_strikeoutline->m_width = w;
  }
  //! sets the strikeoutline color
  void setStrikeOutColor(STOFFColor const &color)
  {
    m_strikeoutline->m_color = color;
  }

  //! returns the underline
  Line const &getUnderline() const
  {
    return m_underline.get();
  }
  //! sets the underline
  void setUnderline(Line const &line)
  {
    m_underline = line;
  }
  //! sets the underline style ( by default, we also reset the style)
  void setUnderlineStyle(Line::Style style=Line::None, bool doReset=true)
  {
    if (doReset)
      m_underline = Line(style);
    else
      m_underline->m_style = style;
  }
  //! sets the underline type
  void setUnderlineType(Line::Type type=Line::Single)
  {
    m_underline->m_type = type;
  }
  //! sets the underline word flag
  void setUnderlineWordFlag(bool wordFlag=false)
  {
    m_underline->m_word = wordFlag;
  }
  //! sets the underline width
  void setUnderlineWidth(float w)
  {
    m_underline->m_width = w;
  }
  //! sets the underline color
  void setUnderlineColor(STOFFColor const &color)
  {
    m_underline->m_color = color;
  }

  //! returns the language
  std::string const &language() const
  {
    return m_language.get();
  }
  //! set the language ( in the for en_US, en_GB, en, ...)
  void setLanguage(std::string const &lang)
  {
    m_language=lang;
  }
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList) const;

  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, STOFFFont const &font);

  //! operator==
  bool operator==(STOFFFont const &f) const
  {
    return cmp(f) == 0;
  }
  //! operator!=
  bool operator!=(STOFFFont const &f) const
  {
    return cmp(f) != 0;
  }

  //! a comparison function
  int cmp(STOFFFont const &oth) const
  {
    if (getFontName() < oth.getFontName()) return -1;
    if (getFontName() > oth.getFontName()) return 1;
    if (size() < oth.size()) return -1;
    if (size() > oth.size()) return -1;
    if (flags() < oth.flags()) return -1;
    if (flags() > oth.flags()) return 1;
    if (m_deltaSpacing.get() < oth.m_deltaSpacing.get()) return -1;
    if (m_deltaSpacing.get() > oth.m_deltaSpacing.get()) return 1;
    if (m_deltaSpacingUnit.get() < oth.m_deltaSpacingUnit.get()) return -1;
    if (m_deltaSpacingUnit.get() > oth.m_deltaSpacingUnit.get()) return 1;
    if (m_widthStreching.get() < oth.m_widthStreching.get()) return -1;
    if (m_widthStreching.get() > oth.m_widthStreching.get()) return 1;
    int diff = script().cmp(oth.script());
    if (diff != 0) return diff;
    diff = m_overline.get().cmp(oth.m_overline.get());
    if (diff != 0) return diff;
    diff = m_strikeoutline.get().cmp(oth.m_strikeoutline.get());
    if (diff != 0) return diff;
    diff = m_underline.get().cmp(oth.m_underline.get());
    if (diff != 0) return diff;
    if (m_color.get() < oth.m_color.get()) return -1;
    if (m_color.get() > oth.m_color.get()) return 1;
    if (m_backgroundColor.get() < oth.m_backgroundColor.get()) return -1;
    if (m_backgroundColor.get() > oth.m_backgroundColor.get()) return 1;
    if (m_language.get() < oth.m_language.get()) return -1;
    if (m_language.get() > oth.m_language.get()) return 1;
    return diff;
  }

protected:
  STOFFVariable<librevenge::RVNGString> m_name /** font identificator*/;
  STOFFVariable<float> m_size /** font size */;
  STOFFVariable<float> m_deltaSpacing /** expand(&gt; 0), condensed(&lt; 0) depl*/;
  STOFFVariable<librevenge::RVNGUnit> m_deltaSpacingUnit /** the delta spacing unit */;
  STOFFVariable<float> m_widthStreching /** the width streching in percent */;
  STOFFVariable<Script> m_scriptPosition /** the sub/super script definition */;
  STOFFVariable<uint32_t> m_flags /** font attributes */;
  STOFFVariable<Line> m_overline /** overline attributes */;
  STOFFVariable<Line> m_strikeoutline /** overline attributes */;
  STOFFVariable<Line> m_underline /** underline attributes */;
  STOFFVariable<STOFFColor> m_color /** font color */;
  STOFFVariable<STOFFColor> m_backgroundColor /** font background color */;
  STOFFVariable<std::string> m_language /** the language if set */;
public:
  //! extra data
  std::string m_extra;
};


#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
