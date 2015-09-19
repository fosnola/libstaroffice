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

#ifndef STOFF_PARAGRAPH
#  define STOFF_PARAGRAPH

#include <iostream>
#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"
#include "STOFFList.hxx"

/** class to store a tab use by STOFFParagraph */
struct STOFFTabStop {
  //! the tab alignment
  enum Alignment { LEFT, RIGHT, CENTER, DECIMAL, BAR };
  //! constructor
  STOFFTabStop(double position = 0.0, Alignment alignment = LEFT, uint16_t leaderCharacter='\0', uint16_t decimalCharacter = '.')  :
    m_position(position), m_alignment(alignment), m_leaderCharacter(leaderCharacter), m_decimalCharacter(decimalCharacter)
  {
  }
  //! add a tab to the propList
  void addTo(librevenge::RVNGPropertyListVector &propList, double decalX=0.0) const;
  //! operator==
  bool operator==(STOFFTabStop const &tabs) const
  {
    return cmp(tabs)==0;
  }
  //! operator!=
  bool operator!=(STOFFTabStop const &tabs) const
  {
    return cmp(tabs)!=0;
  }
  //! operator <<
  friend std::ostream &operator<<(std::ostream &o, STOFFTabStop const &ft);
  //! a comparison function
  int cmp(STOFFTabStop const &tabs) const;
  //! the tab position
  double m_position;
  //! the alignment ( left, center, ...)
  Alignment m_alignment;
  //! the leader char
  uint16_t m_leaderCharacter;
  //! the decimal char
  uint16_t m_decimalCharacter;
};

//! class to store the paragraph properties
class STOFFParagraph
{
public:
  /** some bit use to defined the break status */
  enum { NoBreakBit = 0x1, NoBreakWithNextBit=0x2 };
  /** an enum used to defined the paragraph justification: left, center, right, full ... */
  enum Justification { JustificationLeft, JustificationFull, JustificationCenter,
                       JustificationRight, JustificationFullAllLines
                     };
  /** the line spacing type: fixed or at least */
  enum LineSpacingType { Fixed, AtLeast};

  //! constructor
  STOFFParagraph();
  //! destructor
  virtual ~STOFFParagraph();
  //! operator==
  bool operator==(STOFFParagraph const &p) const
  {
    return cmp(p)==0;
  }
  //! operator!=
  bool operator!=(STOFFParagraph const &p) const
  {
    return cmp(p)!=0;
  }
  //! a comparison function
  int cmp(STOFFParagraph const &p) const;
  //! return the paragraph margin width (in inches)
  double getMarginsWidth() const;
  //! check if the paragraph has some borders
  bool hasBorders() const;
  //! check if the paragraph has different borders
  bool hasDifferentBorders() const;
  //! a function used to resize the borders list ( adding empty borders if needed )
  void resizeBorders(size_t newSize)
  {
    STOFFBorder empty;
    empty.m_style=STOFFBorder::None;
    m_borders.resize(newSize, empty);
  }
  //! set the interline
  void setInterline(double value, librevenge::RVNGUnit unit, LineSpacingType type=Fixed)
  {
    m_spacings[0]=value;
    m_spacingsInterlineUnit=unit;
    m_spacingsInterlineType=type;
  }
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList, bool inTable) const;

  //! insert the set values of para in the actual paragraph
  void insert(STOFFParagraph const &para);
  //! operator <<
  friend std::ostream &operator<<(std::ostream &o, STOFFParagraph const &ft);

  /** the margins
   *
   * - 0: first line left margin
   * - 1: left margin
   * - 2: right margin*/
  STOFFVariable<double> m_margins[3]; // 0: first line left, 1: left, 2: right
  /** the margins INCH, ... */
  STOFFVariable<librevenge::RVNGUnit> m_marginsUnit;
  /** the line spacing
   *
   * - 0: interline
   * - 1: before
   * - 2: after */
  STOFFVariable<double> m_spacings[3]; // 0: interline, 1: before, 2: after
  /** the interline unit PERCENT or INCH, ... */
  STOFFVariable<librevenge::RVNGUnit> m_spacingsInterlineUnit;
  /** the interline type: fixed, atLeast, ... */
  STOFFVariable<LineSpacingType> m_spacingsInterlineType;
  //! the tabulations
  STOFFVariable<std::vector<STOFFTabStop> > m_tabs;
  //! true if the tabs are relative to left margin, false if there are relative to the page margin (default)
  STOFFVariable<bool> m_tabsRelativeToLeftMargin;

  /** the justification */
  STOFFVariable<Justification> m_justify;
  /** a list of bits: 0x1 (unbreakable), 0x2 (do not break after) */
  STOFFVariable<int> m_breakStatus; // BITS: 1: unbreakable, 2: dont break after

  /** the actual level index */
  STOFFVariable<int> m_listLevelIndex;
  /** the list id (if know ) */
  STOFFVariable<int> m_listId;
  /** the list start value (if set ) */
  STOFFVariable<int> m_listStartValue;
  /** the actual level */
  STOFFVariable<STOFFListLevel> m_listLevel;

  //! the background color
  STOFFVariable<STOFFColor> m_backgroundColor;

  //! list of border ( order STOFFBorder::Pos)
  std::vector<STOFFVariable<STOFFBorder> > m_borders;

  //! the style name
  std::string m_styleName;
  //! a string to store some errors
  std::string m_extra;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
