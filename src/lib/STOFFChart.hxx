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
#include "STOFFGraphicStyle.hxx"

namespace STOFFChartInternal
{
class SubDocument;
}
/** a class used to store a chart associated to a spreadsheet .... */
class STOFFChart
{
  friend class STOFFChartInternal::SubDocument;
public:
  //! a cell position
  struct Position {
    //! constructor
    explicit Position(STOFFVec2i pos=STOFFVec2i(-1,-1), librevenge::RVNGString const &sheetName="")
      : m_pos(pos)
      , m_sheetName(sheetName)
    {
    }
    //! return true if the position is valid
    bool valid() const
    {
      return m_pos[0]>=0 && m_pos[1]>=0 && !m_sheetName.empty();
    }
    //! return true if the position is valid
    bool valid(Position const &maxPos) const
    {
      return valid() && maxPos.valid() && maxPos.m_pos[0]>=m_pos[0] && maxPos.m_pos[1]>=m_pos[1];
    }
    //! return the cell name
    librevenge::RVNGString getCellName() const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Position const &pos);
    //! operator==
    bool operator==(Position &pos) const
    {
      return m_pos==pos.m_pos && m_sheetName==pos.m_sheetName;
    }
    //! operator!=
    bool operator!=(Position &pos) const
    {
      return !(operator==(pos));
    }
    //! the cell column and row
    STOFFVec2i m_pos;
    //! the cell sheet name
    librevenge::RVNGString m_sheetName;
  };
  struct Axis {
    //! the axis content
    enum Type { A_None, A_Numeric, A_Logarithmic, A_Sequence, A_Sequence_Skip_Empty };
    //! constructor
    Axis();
    //! destructor
    ~Axis();
    //! add content to the propList
    void addContentTo(int coord, librevenge::RVNGPropertyList &propList) const;
    //! add style to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Axis const &axis);
    //! the sequence type
    Type m_type;
    //! automatic scaling (or manual)
    bool m_automaticScaling;
    //! the minimum, maximum scaling(if manual)
    STOFFVec2f m_scaling;
    //! show or not the grid
    bool m_showGrid;
    //! show or not the label
    bool m_showLabel;
    //! the label range if defined
    Position m_labelRanges[2];

    //! show or not the title/subtitle
    bool m_showTitle;
    //! the title cell range
    Position m_titleRange;
    //! the title label
    librevenge::RVNGString m_title;
    //! the subtitle label
    librevenge::RVNGString m_subTitle;
    //! the graphic style
    STOFFGraphicStyle m_style;
  };
  //! a legend in a chart
  struct Legend {
    //! constructor
    Legend()
      : m_show(false)
      , m_autoPosition(true)
      , m_relativePosition(libstoff::RightBit)
      , m_position(0,0)
      , m_font()
      , m_style()
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
    //! the graphic style
    STOFFGraphicStyle m_style;
  };
  //! a serie in a chart
  struct Serie {
    //! the series type
    enum Type { S_Area, S_Bar, S_Bubble, S_Circle, S_Column, S_Gantt, S_Line, S_Radar, S_Ring, S_Scatter, S_Stock, S_Surface };
    //! the point type
    enum PointType {
      P_None=0, P_Automatic, P_Square, P_Diamond, P_Arrow_Down,
      P_Arrow_Up, P_Arrow_Right, P_Arrow_Left, P_Bow_Tie, P_Hourglass,
      P_Circle, P_Star, P_X, P_Plus, P_Asterisk,
      P_Horizontal_Bar, P_Vertical_Bar
    };
    //! constructor
    Serie();
    Serie(Serie const &)=default;
    Serie(Serie &&)=default;
    Serie &operator=(Serie const &)=default;
    Serie &operator=(Serie &&)=default;
    //! destructor
    virtual ~Serie();
    //! return true if the serie style is 1D
    bool is1DStyle() const
    {
      return m_type==S_Line || m_type==S_Radar || (m_type==S_Scatter && m_pointType==P_None);
    }
    //! return true if the serie is valid
    bool valid() const
    {
      return m_ranges[0].valid(m_ranges[0]);
    }
    //! add content to the propList
    void addContentTo(librevenge::RVNGPropertyList &propList) const;
    //! add style to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! returns a string corresponding to a series type
    static std::string getSerieTypeName(Type type);
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Serie const &series);
    //! the type
    Type m_type;
    //! the data range
    Position m_ranges[2];
    //! use or not the secondary y axis
    bool m_useSecondaryY;
    //! the label font
    STOFFFont m_font;
    //! the label ranges if defined(unused)
    Position m_labelRanges[2];
    //! the legend range if defined
    Position m_legendRange;
    //! the legend name if defined
    librevenge::RVNGString m_legendText;
    //! the graphic style
    STOFFGraphicStyle m_style;
    //! the point type
    PointType m_pointType;
  };
  //! a text zone a chart
  struct TextZone {
    //! the text type
    enum Type { T_Title, T_SubTitle, T_Footer };
    //! the text content type
    enum ContentType { C_Cell, C_Text };

    //! constructor
    explicit TextZone(Type type);
    //! destructor
    ~TextZone();
    //! returns true if the textbox is valid
    bool valid() const
    {
      if (!m_show) return false;
      if (m_contentType==C_Cell)
        return m_cell.valid();
      if (m_contentType!=C_Text)
        return false;
      for (auto &e : m_textEntryList) {
        if (e.valid()) return true;
      }
      return false;
    }
    //! add content to the propList
    void addContentTo(librevenge::RVNGPropertyList &propList) const;
    //! add to the propList
    void addStyleTo(librevenge::RVNGPropertyList &propList) const;
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, TextZone const &zone);
    //! the zone type
    Type m_type;
    //! the content type
    ContentType m_contentType;
    //! true if the zone is visible
    bool m_show;
    //! the position in the zone
    STOFFVec2f m_position;
    //! the cell position ( or title and subtitle)
    Position m_cell;
    //! the text entry (or the list of text entry)
    std::vector<STOFFEntry> m_textEntryList;
    //! the zone format
    STOFFFont m_font;
    //! the graphic style
    STOFFGraphicStyle m_style;
  };

  //! the constructor
  STOFFChart(STOFFVec2f const &dim=STOFFVec2f());
  //! the destructor
  virtual ~STOFFChart();
  //! send the chart to the listener
  void sendChart(STOFFSpreadsheetListenerPtr &listener, librevenge::RVNGSpreadsheetInterface *interface);
  //! send the zone content (called when the zone is of text type)
  virtual void sendContent(TextZone const &zone, STOFFListenerPtr &listener) const =0;

  //! set the grid color
  void setGridColor(STOFFColor const &color)
  {
    m_gridColor=color;
  }
  //! return an axis (corresponding to a coord)
  Axis &getAxis(int coord);
  //! return an axis (corresponding to a coord)
  Axis const &getAxis(int coord) const;

  //! returns the legend
  Legend const &getLegend() const
  {
    return m_legend;
  }
  //! returns the legend
  Legend &getLegend()
  {
    return m_legend;
  }

  //! return a serie
  Serie *getSerie(int id, bool create);
  //! returns the list of defined series
  std::map<int, Serie> const &getIdSerieMap() const
  {
    return m_serieMap;
  }
  //! returns a textzone content
  TextZone *getTextZone(TextZone::Type type, bool create=false);

protected:
  //! sends a textzone content
  void sendTextZoneContent(TextZone::Type type, STOFFListenerPtr &listener) const;

public:
  //! the chart dimension in point
  STOFFVec2f m_dimension;
  //! the chart type (if no series)
  Serie::Type m_type;
  //! a flag to know if the data are stacked or not
  bool m_dataStacked;
  //! a flag to know if the data are percent stacked or not
  bool m_dataPercentStacked;
  //! a flag to know if the data are vertical (for bar)
  bool m_dataVertical;
  //! a flag to know if the graphic is 3D
  bool m_is3D;
  //! a flag to know if real 3D or 2D-extended
  bool m_is3DDeep;

  // main

  //! the chart style
  STOFFGraphicStyle m_style;
  //! the chart name
  librevenge::RVNGString m_name;

  // plot area

  //! the plot area dimension in percent
  STOFFBox2f m_plotAreaPosition;
  //! the ploat area style
  STOFFGraphicStyle m_plotAreaStyle;

  // legend

  //! the legend dimension in percent
  STOFFBox2f m_legendPosition;

  //! floor
  STOFFGraphicStyle m_floorStyle;
  //! wall
  STOFFGraphicStyle m_wallStyle;

protected:
  //! the grid color
  STOFFColor m_gridColor;
  //! the x,y,y-second,z and a bad axis
  Axis m_axis[5];
  //! the legend
  Legend m_legend;
  //! the list of series
  std::map<int, Serie> m_serieMap;
  //! a map text zone type to text zone
  std::map<TextZone::Type, TextZone> m_textZoneMap;
private:
  explicit STOFFChart(STOFFChart const &orig) = delete;
  STOFFChart &operator=(STOFFChart const &orig) = delete;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
