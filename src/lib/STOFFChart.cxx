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
 * Structure to store and construct a chart from an unstructured list
 * of cell
 *
 */

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFInputStream.hxx"
#include "STOFFListener.hxx"
#include "STOFFPosition.hxx"
#include "STOFFSpreadsheetListener.hxx"
#include "STOFFSubDocument.hxx"

#include "STOFFChart.hxx"

/** Internal: the structures of a STOFFChart */
namespace STOFFChartInternal
{
////////////////////////////////////////
//! Internal: the subdocument of a STOFFChart
class SubDocument final : public STOFFSubDocument
{
public:
  SubDocument(STOFFChart *chart, STOFFChart::TextZone::Type textZone) :
    STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry()), m_chart(chart), m_textZone(textZone)
  {
  }

  //! destructor
  ~SubDocument() final {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const final
  {
    if (STOFFSubDocument::operator!=(doc))
      return true;
    auto const *subDoc=dynamic_cast<SubDocument const *>(&doc);
    if (!subDoc) return true;
    return m_chart!=subDoc->m_chart || m_textZone!=subDoc->m_textZone;
  }
  //! operator==
  bool operator==(STOFFSubDocument const &doc) const
  {
    return !operator!=(doc);
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;
protected:
  //! the chart
  STOFFChart *m_chart;
  //! the textzone type
  STOFFChart::TextZone::Type m_textZone;
private:
  SubDocument(SubDocument const &orig);
  SubDocument &operator=(SubDocument const &orig);
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("STOFFChartInternal::SubDocument::parse: no listener\n"));
    return;
  }

  if (!m_chart) {
    STOFF_DEBUG_MSG(("STOFFChartInternal::SubDocument::parse: can not find the chart\n"));
    return;
  }
  m_chart->sendTextZoneContent(m_textZone, listener);
}

}

////////////////////////////////////////////////////////////
// STOFFChart
////////////////////////////////////////////////////////////
STOFFChart::STOFFChart(STOFFVec2f const &dim)
  : m_dimension(dim)
  , m_type(STOFFChart::Serie::S_Bar)
  , m_dataStacked(false)
  , m_dataPercentStacked(false)
  , m_dataVertical(false)
  , m_is3D(false)
  , m_is3DDeep(false)

  , m_style()
  , m_name()

  , m_plotAreaPosition()
  , m_plotAreaStyle()

  , m_legendPosition()

  , m_floorStyle()
  , m_wallStyle()

  , m_gridColor(179,179,179)
  , m_legend()
  , m_serieMap()
  , m_textZoneMap()
{
  // checkme define the default wall/floor line color
}

STOFFChart::~STOFFChart()
{
}

STOFFChart::Axis &STOFFChart::getAxis(int coord)
{
  if (coord<0 || coord>3) {
    STOFF_DEBUG_MSG(("STOFFChart::getAxis: called with bad coord\n"));
    return  m_axis[4];
  }
  return m_axis[coord];
}

STOFFChart::Axis const &STOFFChart::getAxis(int coord) const
{
  if (coord<0 || coord>3) {
    STOFF_DEBUG_MSG(("STOFFChart::getAxis: called with bad coord\n"));
    return  m_axis[4];
  }
  return m_axis[coord];
}

STOFFChart::Serie *STOFFChart::getSerie(int id, bool create)
{
  if (m_serieMap.find(id)!=m_serieMap.end())
    return &m_serieMap.find(id)->second;
  if (!create)
    return nullptr;
  m_serieMap[id]=STOFFChart::Serie();
  return &m_serieMap.find(id)->second;
}

STOFFChart::TextZone *STOFFChart::getTextZone(STOFFChart::TextZone::Type type, bool create)
{
  if (m_textZoneMap.find(type)!=m_textZoneMap.end())
    return &m_textZoneMap.find(type)->second;
  if (!create)
    return nullptr;
  m_textZoneMap.insert(std::map<TextZone::Type, TextZone>::value_type(type,STOFFChart::TextZone(type)));
  return &m_textZoneMap.find(type)->second;
}

void STOFFChart::sendTextZoneContent(STOFFChart::TextZone::Type type, STOFFListenerPtr &listener) const
{
  if (m_textZoneMap.find(type)==m_textZoneMap.end()) {
    STOFF_DEBUG_MSG(("STOFFChart::sendTextZoneContent: called with unknown zone(%d)\n", int(type)));
    return;
  }
  sendContent(m_textZoneMap.find(type)->second, listener);
}

void STOFFChart::sendChart(STOFFSpreadsheetListenerPtr &listener, librevenge::RVNGSpreadsheetInterface *interface)
{
  if (!listener || !interface) {
    STOFF_DEBUG_MSG(("STOFFChart::sendChart: can not find listener or interface\n"));
    return;
  }
  if (m_serieMap.empty()) {
    STOFF_DEBUG_MSG(("STOFFChart::sendChart: can not find the series\n"));
    return;
  }
  std::shared_ptr<STOFFListener> genericListener=listener;
  int styleId=0;

  librevenge::RVNGPropertyList style;
  style.insert("librevenge:chart-id", styleId);
  m_style.addTo(style);
  interface->defineChartStyle(style);

  librevenge::RVNGPropertyList chart;
  if (m_dimension[0]>0 && m_dimension[1]>0) {
    // set the size if known
    chart.insert("svg:width", double(m_dimension[0]), librevenge::RVNG_POINT);
    chart.insert("svg:height", double(m_dimension[1]), librevenge::RVNG_POINT);
  }
  if (!m_serieMap.empty())
    chart.insert("chart:class", Serie::getSerieTypeName(m_serieMap.begin()->second.m_type).c_str());
  else
    chart.insert("chart:class", Serie::getSerieTypeName(m_type).c_str());
  chart.insert("librevenge:chart-id", styleId++);
  interface->openChart(chart);

  // legend
  if (m_legend.m_show) {
    bool autoPlace=m_legendPosition==STOFFBox2f()||m_dimension==STOFFVec2f();
    style=librevenge::RVNGPropertyList();
    m_legend.addStyleTo(style);
    style.insert("librevenge:chart-id", styleId);
    style.insert("chart:auto-position",autoPlace);
    interface->defineChartStyle(style);
    librevenge::RVNGPropertyList legend;
    m_legend.addContentTo(legend);
    legend.insert("librevenge:chart-id", styleId++);
    legend.insert("librevenge:zone-type", "legend");
    if (!autoPlace) {
      legend.insert("svg:x", double(m_legendPosition[0][0])*double(m_dimension[0]), librevenge::RVNG_POINT);
      legend.insert("svg:y", double(m_legendPosition[0][1])*double(m_dimension[1]), librevenge::RVNG_POINT);
      legend.insert("svg:width", double(m_legendPosition.size()[0])*double(m_dimension[0]), librevenge::RVNG_POINT);
      legend.insert("svg:height", double(m_legendPosition.size()[1])*double(m_dimension[1]), librevenge::RVNG_POINT);
    }
    interface->openChartTextObject(legend);
    interface->closeChartTextObject();
  }
  for (auto textIt : m_textZoneMap) {
    TextZone const &zone= textIt.second;
    if (!zone.valid()) continue;
    style=librevenge::RVNGPropertyList();
    zone.addStyleTo(style);
    style.insert("librevenge:chart-id", styleId);
    interface->defineChartStyle(style);
    librevenge::RVNGPropertyList textZone;
    zone.addContentTo(textZone);
    textZone.insert("librevenge:chart-id", styleId++);
    textZone.insert("librevenge:zone-type",
                    zone.m_type==TextZone::T_Title ? "title": zone.m_type==TextZone::T_SubTitle ?"subtitle" : "footer");
    interface->openChartTextObject(textZone);
    if (zone.m_contentType==TextZone::C_Text) {
      std::shared_ptr<STOFFSubDocument> doc(new STOFFChartInternal::SubDocument(this, zone.m_type));
      listener->handleSubDocument(doc, libstoff::DOC_CHART_ZONE);
    }
    interface->closeChartTextObject();
  }
  // plot area
  style=librevenge::RVNGPropertyList();
  bool autoPlace=m_plotAreaPosition==STOFFBox2f()||m_dimension==STOFFVec2f();
  m_plotAreaStyle.addTo(style);
  style.insert("librevenge:chart-id", styleId);
  style.insert("chart:include-hidden-cells","false");
  style.insert("chart:auto-position",autoPlace);
  style.insert("chart:auto-size",autoPlace);
  style.insert("chart:treat-empty-cells","leave-gap");
  style.insert("chart:right-angled-axes","true");
  style.insert("chart:stacked", m_dataStacked);
  style.insert("chart:percentage", m_dataPercentStacked);
  if (m_dataVertical)
    style.insert("chart:vertical", true);
  if (m_is3D) {
    style.insert("chart:three-dimensional", true);
    style.insert("chart:deep", m_is3DDeep);
  }
  interface->defineChartStyle(style);

  librevenge::RVNGPropertyList plotArea;
  if (!autoPlace) {
    plotArea.insert("svg:x", double(m_plotAreaPosition[0][0])*double(m_dimension[0]), librevenge::RVNG_POINT);
    plotArea.insert("svg:y", double(m_plotAreaPosition[0][1])*double(m_dimension[1]), librevenge::RVNG_POINT);
    plotArea.insert("svg:width", double(m_plotAreaPosition.size()[0])*double(m_dimension[0]), librevenge::RVNG_POINT);
    plotArea.insert("svg:height", double(m_plotAreaPosition.size()[1])*double(m_dimension[1]), librevenge::RVNG_POINT);
  }
  plotArea.insert("librevenge:chart-id", styleId++);

  librevenge::RVNGPropertyList floor, wall;
  librevenge::RVNGPropertyListVector vect;
  style=librevenge::RVNGPropertyList();
  // add floor
  m_floorStyle.addTo(style);
  style.insert("librevenge:chart-id", styleId);
  interface->defineChartStyle(style);
  floor.insert("librevenge:type", "floor");
  floor.insert("librevenge:chart-id", styleId++);
  vect.append(floor);

  // add wall
  style=librevenge::RVNGPropertyList();
  m_wallStyle.addTo(style);
  style.insert("librevenge:chart-id", styleId);
  interface->defineChartStyle(style);
  wall.insert("librevenge:type", "wall");
  wall.insert("librevenge:chart-id", styleId++);
  vect.append(wall);

  plotArea.insert("librevenge:childs", vect);

  interface->openChartPlotArea(plotArea);
  // axis : x, y, second, z
  for (int i=0; i<4; ++i) {
    if (m_axis[i].m_type==Axis::A_None) continue;
    style=librevenge::RVNGPropertyList();
    m_axis[i].addStyleTo(style);
    style.insert("librevenge:chart-id", styleId);
    interface->defineChartStyle(style);
    librevenge::RVNGPropertyList axis;
    m_axis[i].addContentTo(i, axis);
    axis.insert("librevenge:chart-id", styleId++);
    interface->insertChartAxis(axis);
  }
  // series
  for (auto it : m_serieMap) {
    auto const &serie = it.second;
    if (!serie.valid()) continue;
    style=librevenge::RVNGPropertyList();
    serie.addStyleTo(style);
    style.insert("librevenge:chart-id", styleId);
    interface->defineChartStyle(style);
    librevenge::RVNGPropertyList series;
    serie.addContentTo(series);
    series.insert("librevenge:chart-id", styleId++);
    interface->openChartSerie(series);
    interface->closeChartSerie();
  }
  interface->closeChartPlotArea();

  interface->closeChart();
}

////////////////////////////////////////////////////////////
// Position
////////////////////////////////////////////////////////////
librevenge::RVNGString STOFFChart::Position::getCellName() const
{
  if (!valid()) {
    STOFF_DEBUG_MSG(("STOFFChart::Position::getCellName: called on invalid cell\n"));
    return librevenge::RVNGString();
  }
  std::string cellName=libstoff::getCellName(m_pos);
  if (cellName.empty())
    return librevenge::RVNGString();
  std::stringstream o;
  o << m_sheetName.cstr() << "." << cellName;
  return librevenge::RVNGString(o.str().c_str());
}
std::ostream &operator<<(std::ostream &o, STOFFChart::Position const &pos)
{
  if (pos.valid())
    o << pos.m_pos << "[" << pos.m_sheetName.cstr() << "]";
  else
    o << "_";
  return o;
}

////////////////////////////////////////////////////////////
// Axis
////////////////////////////////////////////////////////////
STOFFChart::Axis::Axis()
  : m_type(STOFFChart::Axis::A_None)
  , m_automaticScaling(true)
  , m_scaling()
  , m_showGrid(true)
  , m_showLabel(true)
  , m_showTitle(true)
  , m_titleRange()
  , m_title()
  , m_subTitle()
  , m_style()
{
  m_style.m_propertyList.insert("svg:stroke-width", 0, librevenge::RVNG_POINT); // checkme
}

STOFFChart::Axis::~Axis()
{
}

void STOFFChart::Axis::addContentTo(int coord, librevenge::RVNGPropertyList &propList) const
{
  std::string axis("");
  axis += coord==0 ? 'x' : coord==3 ? 'z' : 'y';
  propList.insert("chart:dimension",axis.c_str());
  axis = (coord==2 ? "secondary-y" : "primary-"+axis);
  propList.insert("chart:name",axis.c_str());
  librevenge::RVNGPropertyListVector childs;
  if (m_showGrid && (m_type==A_Numeric || m_type==A_Logarithmic)) {
    librevenge::RVNGPropertyList grid;
    grid.insert("librevenge:type", "grid");
    grid.insert("chart:class", "major");
    childs.append(grid);
  }
  if (m_labelRanges[0].valid(m_labelRanges[1]) && m_showLabel) {
    librevenge::RVNGPropertyList range;
    range.insert("librevenge:sheet-name", m_labelRanges[0].m_sheetName);
    range.insert("librevenge:start-row", m_labelRanges[0].m_pos[1]);
    range.insert("librevenge:start-column", m_labelRanges[0].m_pos[0]);
    if (m_labelRanges[0].m_sheetName!=m_labelRanges[1].m_sheetName)
      range.insert("librevenge:end-sheet-name", m_labelRanges[1].m_sheetName);
    range.insert("librevenge:end-row", m_labelRanges[1].m_pos[1]);
    range.insert("librevenge:end-column", m_labelRanges[1].m_pos[0]);
    librevenge::RVNGPropertyListVector vect;
    vect.append(range);
    librevenge::RVNGPropertyList categories;
    categories.insert("librevenge:type", "categories");
    categories.insert("table:cell-range-address", vect);
    childs.append(categories);
  }
  if (m_showTitle && (!m_title.empty() || !m_subTitle.empty())) {
    auto finalString(m_title);
    if (!m_title.empty() && !m_subTitle.empty()) finalString.append(" - ");
    finalString.append(m_subTitle);
    librevenge::RVNGPropertyList title;
    title.insert("librevenge:type", "title");
    title.insert("librevenge:text", finalString);
    childs.append(title);
  }
  else if (m_showTitle && m_titleRange.valid()) {
    librevenge::RVNGPropertyList title;
    title.insert("librevenge:type", "title");
    librevenge::RVNGPropertyList range;
    range.insert("librevenge:sheet-name", m_titleRange.m_sheetName);
    range.insert("librevenge:start-row", m_titleRange.m_pos[1]);
    range.insert("librevenge:start-column", m_titleRange.m_pos[0]);
    librevenge::RVNGPropertyListVector vect;
    vect.append(range);
    title.insert("table:cell-range", vect);
    childs.append(title);
  }
  if (!childs.empty())
    propList.insert("librevenge:childs", childs);
}

void STOFFChart::Axis::addStyleTo(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("chart:display-label", m_showLabel);
  propList.insert("chart:axis-position", 0, librevenge::RVNG_GENERIC);
  propList.insert("chart:reverse-direction", false);
  propList.insert("chart:logarithmic", m_type==STOFFChart::Axis::A_Logarithmic);
  propList.insert("text:line-break", false);
  if (!m_automaticScaling) {
    propList.insert("chart:minimum", m_scaling[0], librevenge::RVNG_GENERIC);
    propList.insert("chart:maximum", m_scaling[1], librevenge::RVNG_GENERIC);
  }
  m_style.addTo(propList);
}

std::ostream &operator<<(std::ostream &o, STOFFChart::Axis const &axis)
{
  switch (axis.m_type) {
  case STOFFChart::Axis::A_None:
    o << "none,";
    break;
  case STOFFChart::Axis::A_Numeric:
    o << "numeric,";
    break;
  case STOFFChart::Axis::A_Logarithmic:
    o << "logarithmic,";
    break;
  case STOFFChart::Axis::A_Sequence:
    o << "sequence,";
    break;
  case STOFFChart::Axis::A_Sequence_Skip_Empty:
    o << "sequence[noEmpty],";
    break;
#if !defined(__clang__)
  default:
    o << "###type,";
    STOFF_DEBUG_MSG(("STOFFChart::Axis: unexpected type\n"));
    break;
#endif
  }
  if (axis.m_showGrid) o << "show[grid],";
  if (axis.m_showLabel) o << "show[label],";
  if (axis.m_labelRanges[0].valid(axis.m_labelRanges[1]))
    o << "label[range]=" << axis.m_labelRanges[0] << ":" << axis.m_labelRanges[1] << ",";
  if (axis.m_showTitle) {
    if (axis.m_titleRange.valid()) o << "title[range]=" << axis.m_titleRange << ",";
    if (!axis.m_title.empty()) o << "title=" << axis.m_title.cstr() << ",";
    if (!axis.m_subTitle.empty()) o << "subTitle=" << axis.m_subTitle.cstr() << ",";
  }
  if (!axis.m_automaticScaling && axis.m_scaling!=STOFFVec2f())
    o << "scaling=manual[" << axis.m_scaling[0] << "->" << axis.m_scaling[1] << ",";
  o << axis.m_style;
  return o;
}

////////////////////////////////////////////////////////////
// Legend
////////////////////////////////////////////////////////////
void STOFFChart::Legend::addContentTo(librevenge::RVNGPropertyList &propList) const
{
  if (m_position[0]>0 && m_position[1]>0) {
    propList.insert("svg:x", double(m_position[0]), librevenge::RVNG_POINT);
    propList.insert("svg:y", double(m_position[1]), librevenge::RVNG_POINT);
  }
  if (!m_autoPosition || !m_relativePosition)
    return;
  std::stringstream s;
  if (m_relativePosition&libstoff::TopBit)
    s << "top";
  else if (m_relativePosition&libstoff::BottomBit)
    s << "bottom";
  if (s.str().length() && (m_relativePosition&(libstoff::LeftBit|libstoff::RightBit)))
    s << "-";
  if (m_relativePosition&libstoff::LeftBit)
    s << "start";
  else if (m_relativePosition&libstoff::RightBit)
    s << "end";
  propList.insert("chart:legend-position", s.str().c_str());
}

void STOFFChart::Legend::addStyleTo(librevenge::RVNGPropertyList &propList) const
{
  propList.insert("chart:auto-position", m_autoPosition);
  m_font.addTo(propList);
  m_style.addTo(propList);
}

std::ostream &operator<<(std::ostream &o, STOFFChart::Legend const &legend)
{
  if (legend.m_show)
    o << "show,";
  if (legend.m_autoPosition) {
    o << "automaticPos[";
    if (legend.m_relativePosition&libstoff::TopBit)
      o << "t";
    else if (legend.m_relativePosition&libstoff::RightBit)
      o << "b";
    else
      o << "c";
    if (legend.m_relativePosition&libstoff::LeftBit)
      o << "L";
    else if (legend.m_relativePosition&libstoff::BottomBit)
      o << "R";
    else
      o << "C";
    o << "]";
  }
  else
    o << "pos=" << legend.m_position << ",";
  o << legend.m_style;
  return o;
}

////////////////////////////////////////////////////////////
// Serie
////////////////////////////////////////////////////////////
STOFFChart::Serie::Serie()
  : m_type(STOFFChart::Serie::S_Bar)
  , m_useSecondaryY(false)
  , m_font()
  , m_legendRange()
  , m_legendText()
  , m_style()
  , m_pointType(P_None)
{
}

STOFFChart::Serie::~Serie()
{
}

std::string STOFFChart::Serie::getSerieTypeName(Type type)
{
  switch (type) {
  case S_Area:
    return "chart:area";
  case S_Bar:
    return "chart:bar";
  case S_Bubble:
    return "chart:bubble";
  case S_Circle:
    return "chart:circle";
  case S_Column:
    return "chart:column";
  case S_Gantt:
    return "chart:gantt";
  case S_Line:
    return "chart:line";
  case S_Radar:
    return "chart:radar";
  case S_Ring:
    return "chart:ring";
  case S_Scatter:
    return "chart:scatter";
  case S_Stock:
    return "chart:stock";
  case S_Surface:
    return "chart:surface";
#if !defined(__clang__)
  default:
    break;
#endif
  }
  return "chart:bar";
}

void STOFFChart::Serie::addContentTo(librevenge::RVNGPropertyList &serie) const
{
  serie.insert("chart:class",getSerieTypeName(m_type).c_str());
  if (m_useSecondaryY)
    serie.insert("chart:attached-axis","secondary-y");
  librevenge::RVNGPropertyList datapoint;
  librevenge::RVNGPropertyListVector vect;
  if (m_ranges[0].valid(m_ranges[1])) {
    librevenge::RVNGPropertyList range;
    range.insert("librevenge:sheet-name", m_ranges[0].m_sheetName);
    range.insert("librevenge:start-row", m_ranges[0].m_pos[1]);
    range.insert("librevenge:start-column", m_ranges[0].m_pos[0]);
    if (m_ranges[0].m_sheetName != m_ranges[1].m_sheetName)
      range.insert("librevenge:end-sheet-name", m_ranges[1].m_sheetName);
    range.insert("librevenge:end-row", m_ranges[1].m_pos[1]);
    range.insert("librevenge:end-column", m_ranges[1].m_pos[0]);
    vect.append(range);
    serie.insert("chart:values-cell-range-address", vect);
    vect.clear();
  }

  // to do use labelRanges

  // USEME: set the font here
  if (m_legendRange.valid()) {
    librevenge::RVNGPropertyList label;
    label.insert("librevenge:sheet-name", m_legendRange.m_sheetName);
    label.insert("librevenge:start-row", m_legendRange.m_pos[1]);
    label.insert("librevenge:start-column", m_legendRange.m_pos[0]);
    vect.append(label);
    serie.insert("chart:label-cell-address", vect);
    vect.clear();
  }
  if (!m_legendText.empty()) {
    // replace ' ' and non basic caracters by _ because this causes LibreOffice's problem
    std::string basicString(m_legendText.cstr());
    for (auto &c : basicString) {
      if (c==' ' || (unsigned char)(c)>=0x80)
        c='_';
    }
    serie.insert("chart:label-string", basicString.c_str());
  }
  datapoint.insert("librevenge:type", "data-point");
  STOFFVec2i dataSize=m_ranges[1].m_pos-m_ranges[0].m_pos;
  datapoint.insert("chart:repeated", 1+std::max(dataSize[0],dataSize[1]));
  vect.append(datapoint);
  serie.insert("librevenge:childs", vect);
}

void STOFFChart::Serie::addStyleTo(librevenge::RVNGPropertyList &propList) const
{
  m_style.addTo(propList);
  if (m_pointType != STOFFChart::Serie::P_None) {
    char const *what[] = {
      "none", "automatic", "square", "diamond", "arrow-down",
      "arrow-up", "arrow-right", "arrow-left", "bow-tie", "hourglass",
      "circle", "star", "x", "plus", "asterisk",
      "horizontal-bar", "vertical-bar"
    };
    if (m_pointType == STOFFChart::Serie::P_Automatic)
      propList.insert("chart:symbol-type", "automatic");
    else if (m_pointType < STOFF_N_ELEMENTS(what)) {
      propList.insert("chart:symbol-type", "named-symbol");
      propList.insert("chart:symbol-name", what[m_pointType]);
    }
  }
  //propList.insert("chart:data-label-number","value");
  //propList.insert("chart:data-label-text","false");
}

std::ostream &operator<<(std::ostream &o, STOFFChart::Serie const &serie)
{
  switch (serie.m_type) {
  case STOFFChart::Serie::S_Area:
    o << "area,";
    break;
  case STOFFChart::Serie::S_Bar:
    o << "bar,";
    break;
  case STOFFChart::Serie::S_Bubble:
    o << "bubble,";
    break;
  case STOFFChart::Serie::S_Circle:
    o << "circle,";
    break;
  case STOFFChart::Serie::S_Column:
    o << "column,";
    break;
  case STOFFChart::Serie::S_Gantt:
    o << "gantt,";
    break;
  case STOFFChart::Serie::S_Line:
    o << "line,";
    break;
  case STOFFChart::Serie::S_Radar:
    o << "radar,";
    break;
  case STOFFChart::Serie::S_Ring:
    o << "ring,";
    break;
  case STOFFChart::Serie::S_Scatter:
    o << "scatter,";
    break;
  case STOFFChart::Serie::S_Stock:
    o << "stock,";
    break;
  case STOFFChart::Serie::S_Surface:
    o << "surface,";
    break;
#if !defined(__clang__)
  default:
    o << "###type,";
    STOFF_DEBUG_MSG(("STOFFChart::Serie: unexpected type\n"));
    break;
#endif
  }
  o << "range=" << serie.m_ranges[0] << ":" << serie.m_ranges[1] << ",";
  o << serie.m_style;
  if (serie.m_labelRanges[0].valid(serie.m_labelRanges[1]))
    o << "label[range]=" << serie.m_labelRanges[0] << "<->" << serie.m_labelRanges[1] << ",";
  if (serie.m_legendRange.valid())
    o << "legend[range]=" << serie.m_legendRange << ",";
  if (!serie.m_legendText.empty())
    o << "label[text]=" << serie.m_legendText.cstr() << ",";
  if (serie.m_pointType != STOFFChart::Serie::P_None) {
    char const *what[] = {
      "none", "automatic", "square", "diamond", "arrow-down",
      "arrow-up", "arrow-right", "arrow-left", "bow-tie", "hourglass",
      "circle", "star", "x", "plus", "asterisk",
      "horizontal-bar", "vertical-bar"
    };
    if (serie.m_pointType>0 && serie.m_pointType < STOFF_N_ELEMENTS(what)) {
      o << "point=" << what[serie.m_pointType] << ",";
    }
    else if (serie.m_pointType>0)
      o << "#point=" << serie.m_pointType << ",";
  }
  return o;
}

////////////////////////////////////////////////////////////
// TextZone
////////////////////////////////////////////////////////////
STOFFChart::TextZone::TextZone(Type type)
  : m_type(type)
  , m_contentType(STOFFChart::TextZone::C_Text)
  , m_show(true)
  , m_position(-1,-1)
  , m_cell()
  , m_textEntryList()
  , m_font()
  , m_style()
{
  m_style.m_propertyList.insert("svg:stroke-width", 0, librevenge::RVNG_POINT); // checkme
}

STOFFChart::TextZone::~TextZone()
{
}

void STOFFChart::TextZone::addContentTo(librevenge::RVNGPropertyList &propList) const
{
  if (m_position[0]>0 && m_position[1]>0) {
    propList.insert("svg:x", double(m_position[0]), librevenge::RVNG_POINT);
    propList.insert("svg:y", double(m_position[1]), librevenge::RVNG_POINT);
  }
  else
    propList.insert("chart:auto-position",true);
  propList.insert("chart:auto-size",true);
  switch (m_type) {
  case T_Footer:
    propList.insert("librevenge:zone-type", "footer");
    break;
  case T_Title:
    propList.insert("librevenge:zone-type", "title");
    break;
  case T_SubTitle:
    propList.insert("librevenge:zone-type", "subtitle");
    break;
#if !defined(__clang__)
  default:
    propList.insert("librevenge:zone-type", "label");
    STOFF_DEBUG_MSG(("STOFFChart::TextZone:addContentTo: unexpected type\n"));
    break;
#endif
  }
  if (m_contentType==C_Cell && m_cell.valid()) {
    librevenge::RVNGPropertyList range;
    librevenge::RVNGPropertyListVector vect;
    range.insert("librevenge:sheet-name", m_cell.m_sheetName);
    range.insert("librevenge:row", m_cell.m_pos[1]);
    range.insert("librevenge:column", m_cell.m_pos[0]);
    vect.append(range);
    propList.insert("table:cell-range", vect);
  }
}

void STOFFChart::TextZone::addStyleTo(librevenge::RVNGPropertyList &propList) const
{
  m_font.addTo(propList);
  m_style.addTo(propList);
}

std::ostream &operator<<(std::ostream &o, STOFFChart::TextZone const &zone)
{
  switch (zone.m_type) {
  case STOFFChart::TextZone::T_SubTitle:
    o << "sub";
    STOFF_FALLTHROUGH;
  case STOFFChart::TextZone::T_Title:
    o << "title,";
    break;
  case STOFFChart::TextZone::T_Footer:
    o << "footer,";
    break;
#if !defined(__clang__)
  default:
    o << "###type,";
    STOFF_DEBUG_MSG(("STOFFChart::TextZone: unexpected type\n"));
    break;
#endif
  }
  if (zone.m_contentType==STOFFChart::TextZone::C_Text)
    o << "text,";
  else if (zone.m_contentType==STOFFChart::TextZone::C_Cell)
    o << "cell=" << zone.m_cell << ",";
  if (zone.m_position[0]>0 || zone.m_position[1]>0)
    o << "pos=" << zone.m_position << ",";
  o << zone.m_style;
  return o;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
