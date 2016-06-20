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

/*
 * Structure to store and construct a chart
 *
 */

#ifndef STOFF_CHART
#  define STOFF_CHART

#include <iostream>
#include <vector>
#include <map>

#include "libstaroffice_internal.hxx"

#include "STOFFEntry.hxx"
#include "STOFFFont.hxx"

namespace STOFFChartInternal
{
class SubDocument;
}
/** a class used to store a chart associated to a spreadsheet .... */
class STOFFChart
{
  friend class STOFFChartInternal::SubDocument;
public:
  //! a axis in a chart
  struct Axis {
    //! the axis content
    enum Type { A_None, A_Numeric, A_Logarithmic, A_Sequence, A_Sequence_Skip_Empty };
    //! constructor
    Axis();
    //! destructor
    ~Axis();
    //! add content to the propList
    void addContentTo(librevenge::RVNGString const &sheetName, int coord, librevenge::RVNGPropertyList &propList) const;
    //! add style to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Axis const &axis);
    //! the sequence type
    Type m_type;
    //! show or not the grid
    bool m_showGrid;
    //! show or not the label
    bool m_showLabel;
    //! the label range if defined
    STOFFBox2i m_labelRange;
#if 0
    //! the graphic style
    STOFFGraphicStyle m_style;
#endif
  };
  //! a legend in a chart
  struct Legend {
    //! constructor
    Legend() : m_show(false), m_autoPosition(true), m_relativePosition(libstoff::RightBit), m_position(0,0), m_font()
    {
    }
    //! add content to the propList
    void addContentTo(librevenge::RVNGPropertyList &propList) const;
    //! add style to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Legend const &legend);
    //! show or not the legend
    bool m_show;
    //! automatic position
    bool m_autoPosition;
    //! the automatic position libstoff::LeftBit|...
    int m_relativePosition;
    //! the position in points
    STOFFVec2f m_position;
    //! the font
    STOFFFont m_font;
#if 0
    //! the graphic style
    STOFFGraphicStyle m_style;
#endif
  };
  //! a series in a chart
  struct Series {
    //! the series type
    enum Type { S_Area, S_Bar, S_Column, S_Line, S_Pie, S_Scatter, S_Stock };
    //! constructor
    Series();
    //! destructor
    virtual ~Series();
    //! add content to the propList
    void addContentTo(librevenge::RVNGString const &sheetName, librevenge::RVNGPropertyList &propList) const;
    //! add style to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! returns a string corresponding to a series type
    static librevenge::RVNGString getSeriesTypeName(Type type);
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Series const &series);
    //! the type
    Type m_type;
    //! the data range
    STOFFBox2i m_range;
#if 0
    //! the graphic style
    STOFFGraphicStyle m_style;
#endif
  };
  //! a text zone a chart
  struct TextZone {
    //! the text type
    enum Type { T_Title, T_SubTitle, T_AxisX, T_AxisY, T_AxisZ };
    //! the text content type
    enum ContentType { C_Cell, C_Text };

    //! constructor
    TextZone();
    //! destructor
    ~TextZone();
    //! add content to the propList
    void addContentTo(librevenge::RVNGString const &sheetName, librevenge::RVNGPropertyList &propList) const;
    //! add to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, TextZone const &zone);
    //! the zone type
    Type m_type;
    //! the content type
    ContentType m_contentType;
    //! the position in the zone
    STOFFVec2f m_position;
    //! the cell position ( for title and subtitle )
    STOFFVec2i m_cell;
    //! the text entry
    STOFFEntry m_textEntry;
    //! the zone format
    STOFFFont m_font;
#if 0
    //! the graphic style
    STOFFGraphicStyle m_style;
#endif
  };

  //! the constructor
  STOFFChart(librevenge::RVNGString const &sheetName, STOFFVec2f const &dim=STOFFVec2f());
  //! the destructor
  virtual ~STOFFChart();
  //! send the chart to the listener
  void sendChart(STOFFSpreadsheetListenerPtr &listener, librevenge::RVNGSpreadsheetInterface *interface);
  //! send the zone content (called when the zone is of text type)
  virtual void sendContent(TextZone const &zone, STOFFListenerPtr &listener)=0;

  //! sets the chart type
  void setDataType(Series::Type type, bool dataStacked)
  {
    m_type=type;
    m_dataStacked=dataStacked;
  }

  //! return the chart dimension
  STOFFVec2f const &getDimension() const
  {
    return m_dim;
  }
  //! return the chart dimension
  void setDimension(STOFFVec2f const &dim)
  {
    m_dim=dim;
  }
  //! adds an axis (corresponding to a coord)
  void add(int coord, Axis const &axis);
  //! return an axis (corresponding to a coord)
  Axis const &getAxis(int coord) const;

  //! set the legend
  void set(Legend const &legend)
  {
    m_legend=legend;
  }
  //! return the legend
  Legend const &getLegend() const
  {
    return m_legend;
  }

  //! adds a series
  void add(Series const &series);
  //! return the list of series
  std::vector<Series> const &getSeries() const
  {
    return m_seriesList;
  }

  //! adds a textzone
  void add(TextZone const &textZone);
  //! returns a textzone content(if set)
  bool getTextZone(TextZone::Type type, TextZone &textZone);

protected:
  //! sends a textzone content
  void sendTextZoneContent(TextZone::Type type, STOFFListenerPtr &listener);

protected:
  //! the sheet name
  librevenge::RVNGString m_sheetName;
  //! the chart dimension in point
  STOFFVec2f m_dim;
  //! the chart type (if no series)
  Series::Type m_type;
  //! a flag to know if the data are stacked or not
  bool m_dataStacked;
  //! the x,y,z axis and a bad axis
  Axis m_axis[4];
  //! the legend
  Legend m_legend;
  //! the list of series
  std::vector<Series> m_seriesList;
  //! a map text zone type to text zone
  std::map<TextZone::Type, TextZone> m_textZoneMap;
private:
  STOFFChart(STOFFChart const &orig);
  STOFFChart &operator=(STOFFChart const &orig);
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
