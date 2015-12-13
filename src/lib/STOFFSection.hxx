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

#ifndef STOFF_SECTION_H
#define STOFF_SECTION_H

#include <iostream>
#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

/** a class which stores section properties */
class STOFFSection
{
public:
  struct Column;
  //! constructor
  STOFFSection() : m_columns(), m_width(0), m_balanceText(false), m_backgroundColor(STOFFColor::white())
  {
  }
  //! destructor
  virtual ~STOFFSection()
  {
  }
  /** a function which sets n uniform columns

  \note: this erases previous columns and border if there are some
   */
  void setColumns(int num, double width, librevenge::RVNGUnit widthUnit, double colSep=0);
  //! returns the number of columns
  int numColumns() const
  {
    return m_columns.size() <= 1 ? 1 : int(m_columns.size());
  }
  //! returns the true if the section has only one columns
  bool hasSingleColumns() const
  {
    return m_columns.size() <= 1;
  }
  //! add to the propList
  void addTo(librevenge::RVNGPropertyList &propList) const;
  //! add tabs to the propList
  void addColumnsTo(librevenge::RVNGPropertyListVector &propList) const;
  //! operator <<
  friend std::ostream &operator<<(std::ostream &o, STOFFSection const &sec);
  //! operator!=
  bool operator!=(STOFFSection const &sec) const
  {
    if (m_columns.size()!=sec.m_columns.size())
      return true;
    for (size_t c=0; c < m_columns.size(); c++) {
      if (m_columns[c] != sec.m_columns[c])
        return true;
    }
    if (m_balanceText!=sec.m_balanceText || m_backgroundColor!=sec.m_backgroundColor)
      return true;
    return false;
  }
  //! operator==
  bool operator==(STOFFSection const &sec) const
  {
    return !operator!=(sec);
  }

  //! the different column
  std::vector<Column> m_columns;
  //! the total section width ( if set )
  double m_width;
  //! true if the text is balanced between different columns
  bool m_balanceText;
  //! the background color
  STOFFColor m_backgroundColor;

public:
  /** struct to store the columns properties */
  struct Column {
    //! constructor
    Column() : m_width(0), m_widthUnit(librevenge::RVNG_INCH)
    {
      for (int i = 0; i < 4; i++)
        m_margins[i]=0;
    }
    //! add a column to the propList
    bool addTo(librevenge::RVNGPropertyList &propList) const;
    //! operator <<
    friend std::ostream &operator<<(std::ostream &o, Column const &column);
    //! operator!=
    bool operator!=(Column const &col) const
    {
      if (m_width<col.m_width || m_width>col.m_width || m_widthUnit!=col.m_widthUnit)
        return true;
      for (int i = 0; i < 4; i++) {
        if (m_margins[i]<col.m_margins[i] || m_margins[i]>col.m_margins[i])
          return true;
      }
      return false;
    }
    //! operator==
    bool operator==(Column const &col) const
    {
      return !operator!=(col);
    }

    //! the columns width
    double m_width;
    /** the width unit (default inches) */
    librevenge::RVNGUnit m_widthUnit;
    //! the margins in inches using libstoff::Position orders
    double m_margins[4];
  };
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
