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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>

#include <librevenge/librevenge.h>

#include "STOFFGraphicShape.hxx"
#include "STOFFGraphicStyle.hxx"
#include "STOFFListener.hxx"
#include "STOFFSubDocument.hxx"
#include "STOFFTextListener.hxx"

#include "StarAttribute.hxx"
#include "StarBitmap.hxx"
#include "StarFileManager.hxx"
#include "StarGraphicStruct.hxx"
#include "StarObject.hxx"
#include "StarObjectChart.hxx"
#include "StarObjectMath.hxx"
#include "StarObjectSmallText.hxx"
#include "StarObjectText.hxx"
#include "StarItemPool.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "StarObjectSmallGraphic.hxx"

/** Internal: the structures of a StarObjectSmallGraphic */
namespace StarObjectSmallGraphicInternal
{
////////////////////////////////////////
//! Internal: virtual class to store a glue point
class GluePoint
{
public:
  //! constructor
  GluePoint(int x=0, int y=0)
    : m_dimension(x, y)
    , m_direction(0)
    , m_id(0)
    , m_align(0)
    , m_percent(false)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, GluePoint const &pt)
  {
    o << "dim=" << pt.m_dimension << ",";
    if (pt.m_direction) o << "escDir=" << pt.m_direction << ",";
    if (pt.m_id) o << "id=" << pt.m_id << ",";
    if (pt.m_align) o << "align=" << pt.m_align << ",";
    if (pt.m_percent) o << "percent,";
    return o;
  }
  //! the dimension
  STOFFVec2i m_dimension;
  //! the esc direction
  int m_direction;
  //! the id
  int m_id;
  //! the alignment
  int m_align;
  //! a flag to know if this is percent
  bool m_percent;
};
////////////////////////////////////////
//! Internal: virtual class to store a outliner paragraph object
class OutlinerParaObject
{
public:
  //! small struct use to define a Zone: v<=3
  struct Zone {
    //! constructor
    Zone()
      : m_text()
      , m_depth(0)
      , m_backgroundColor(STOFFColor::white())
      , m_background()
      , m_colorName("")
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Zone const &zone)
    {
      if (!zone.m_text) o << "noText,";
      if (zone.m_depth) o << "depth=" << zone.m_depth << ",";
      if (!zone.m_backgroundColor.isWhite()) o << "color=" << zone.m_backgroundColor << ",";
      if (!zone.m_background.isEmpty()) o << "hasBitmap,";
      if (!zone.m_colorName.empty()) o << "color[name]=" << zone.m_colorName.cstr() << ",";
      return o;
    }
    //! the text
    std::shared_ptr<StarObjectSmallText> m_text;
    //! the depth
    int m_depth;
    //! the background color
    STOFFColor m_backgroundColor;
    //! the background bitmap
    STOFFEmbeddedObject m_background;
    //! the color name
    librevenge::RVNGString m_colorName;
  };
  //! constructor
  OutlinerParaObject()
    : m_version(0)
    , m_zones()
    , m_textZone()
    , m_depthList()
    , m_isEditDoc(false)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, OutlinerParaObject const &obj)
  {
    o << "version=" << obj.m_version << ",";
    if (!obj.m_zones.empty()) {
      o << "zones=[";
      for (auto const &z : obj.m_zones)
        o << "[" << z << "],";
      o << "],";
    }
    if (obj.m_textZone) o << "hasTextZone,";
    if (!obj.m_depthList.empty()) {
      o << "depth=[";
      for (auto d : obj.m_depthList)
        o << d << ",";
      o << "],";
    }
    if (obj.m_isEditDoc) o << "isEditDoc,";
    return o;
  }
  //! try to send the text to the listener
  bool send(STOFFListenerPtr &listener)
  {
    if (!listener) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::OutlinerParaObject::send: no listener\n"));
      return false;
    }
    if (m_textZone)
      m_textZone->send(listener);
    else {
      bool first=true;
      for (auto const &z : m_zones) {
        if (first)
          first=false;
        else
          listener->insertEOL();
        if (z.m_text)
          z.m_text->send(listener, z.m_depth);
      }
    }
    return true;
  }
  //! the version
  int m_version;
  //! the list of zones: version<=3
  std::vector<Zone> m_zones;
  //! list of text zone: version==4
  std::shared_ptr<StarObjectSmallText> m_textZone;
  //! list of depth data
  std::vector<int> m_depthList;
  //! true if the object is a edit document
  bool m_isEditDoc;
};

////////////////////////////////////////
//! Internal: the subdocument of a StarObjectSmallGraphic
class SubDocument final : public STOFFSubDocument
{
public:
  explicit SubDocument(std::shared_ptr<OutlinerParaObject> &text)
    : STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry())
    , m_text(text) {}

  //! destructor
  ~SubDocument() final {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const final
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    auto const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_text.get() != sDoc->m_text.get()) return true;
    return false;
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;

protected:
  //! the text
  std::shared_ptr<OutlinerParaObject> m_text;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (!m_text)
    listener->insertChar(' ');
  else
    m_text->send(listener);
}

/////////////////////////////////////////////////////////////

////////////////////////////////////////
//! Internal: virtual class to store a graphic
class Graphic
{
public:
  //! constructor
  explicit Graphic(int id)
    : m_identifier(id)
  {
  }
  //! destructor
  virtual ~Graphic();
  //! basic print function
  virtual std::string print() const
  {
    return getName();
  }
  //! return the object name
  virtual std::string getName() const = 0;
  //! try to send the graphic to the listener
  virtual bool send(STOFFListenerPtr &/*listener*/, STOFFFrameStyle const &/*pos*/, StarObject &/*object*/, bool /*inMasterPage*/)
  {
    static bool first=true;
    if (first) {
      first=false;
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::Graphic::send: not implemented for identifier %d\n", m_identifier));
    }
    return false;
  }
  //! the type
  int m_identifier;
};

Graphic::~Graphic()
{
}


////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic
class SdrGraphic : public Graphic
{
public:
  //! constructor
  explicit SdrGraphic(int id)
    : Graphic(id)
    , m_bdbox()
    , m_layerId(-1)
    , m_anchorPosition(0,0)
    , m_polygon()
    , m_userDataList()
  {
    for (bool &flag : m_flags) flag=false;
  }
  //! destructor
  ~SdrGraphic() override;
  //! return the object name
  std::string getName() const override
  {
    if (m_identifier>0 && m_identifier<=32) {
      char const *wh[]= {"none", "group", "line", "rect", "circle",
                         "sector", "arc", "ccut", "poly", "polyline",
                         "pathline", "pathfill", "freeline", "freefill", "splineline",
                         "splinefill", "text", "textextended", "fittext", "fitalltext",
                         "titletext", "outlinetext", "graf", "ole2", "edge",
                         "caption", "pathpoly", "pathpline", "page", "measure",
                         "dummy","frame", "uno"
                        };
      return wh[m_identifier];
    }
    std::stringstream s;
    s << "###type=" << m_identifier << ",";
    return s.str();
  }
  //! return a pool corresponding to an object
  StarState getState(StarObject &object, STOFFListenerPtr listener, STOFFFrameStyle const &/*pos*/) const
  {
    auto pool=object.findItemPool(StarItemPool::T_XOutdevPool, false);
    StarState state(pool.get(), object);
    if (std::dynamic_pointer_cast<STOFFTextListener>(listener))
      state.m_global->m_offset=state.convertVectorInPoint(STOFFVec2f(float(-m_anchorPosition[0]),float(-m_anchorPosition[1])));
    return state;
  }
  //! try to update the graphic style
  void updateStyle(StarState &state, STOFFListenerPtr /*listener*/) const
  {
    state.m_frame.addStyleTo(state.m_graphic.m_propertyList);
    if (m_flags[0] && m_flags[1])
      state.m_graphic.m_propertyList.insert("style:protect", "position size");
    else if (m_flags[0])
      state.m_graphic.m_propertyList.insert("style:protect", "position");
    else if (m_flags[1])
      state.m_graphic.m_propertyList.insert("style:protect", "size");
    state.m_graphic.m_propertyList.insert("style:print-content", !m_flags[2]);
    // todo noVisible as master ie hide in master
  }
  //! basic print function
  std::string print() const override
  {
    std::stringstream s;
    s << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphic const &graph)
  {
    o << graph.getName() << ",";
    o << "bdbox=" << graph.m_bdbox << ",";
    if (graph.m_layerId>=0) o << "layer[id]=" << graph.m_layerId << ",";
    if (graph.m_anchorPosition!=STOFFVec2i(0,0)) o << "anchor[pos]=" << graph.m_anchorPosition << ",";
    for (int i=0; i<6; ++i) {
      if (!graph.m_flags[i]) continue;
      char const *wh[]= {"move[protected]", "size[protected]", "print[no]", "mark[protected]", "empty", "notVisibleAsMaster"};
      o << wh[i] << ",";
    }
    if (!graph.m_polygon.empty()) {
      o << "poly=[";
      for (const auto &i : graph.m_polygon)
        o << i << ",";
      o << "],";
    }
    return o;
  }
  //! the bdbox
  STOFFBox2i m_bdbox;
  //! the layer id
  int m_layerId;
  //! the anchor position
  STOFFVec2i m_anchorPosition;
  //! a polygon
  std::vector<GluePoint> m_polygon;
  //! a list of flag
  bool m_flags[6];
  //! the user data list
  std::vector<std::shared_ptr<StarObjectSmallGraphicInternal::SDRUserData> > m_userDataList;
};

SdrGraphic::~SdrGraphic()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic attribute
class SdrGraphicAttribute : public SdrGraphic
{
public:
  //! constructor
  explicit SdrGraphicAttribute(int id)
    : SdrGraphic(id)
    , m_itemList()
    , m_sheetStyle("")
  {
  }
  //! destructor
  ~SdrGraphicAttribute() override;
  //! basic print function
  std::string print() const override
  {
    std::stringstream s;
    s << SdrGraphic::print() << *this << ",";
    return s.str();
  }
  //! try to update the style
  void updateStyle(StarState &state, STOFFListenerPtr listener) const
  {
    SdrGraphic::updateStyle(state, listener);
    if (state.m_global->m_pool && !m_sheetStyle.empty()) {
      auto const *mStyle=state.m_global->m_pool->findStyleWithFamily(m_sheetStyle, StarItemStyle::F_Paragraph);
      if (mStyle && !mStyle->m_names[0].empty()) {
        if (listener) state.m_global->m_pool->defineGraphicStyle(listener, mStyle->m_names[0], state.m_global->m_object);
        state.m_graphic.m_propertyList.insert("librevenge:parent-display-name", mStyle->m_names[0]);
      }
      else if (mStyle) {
        for (auto it : mStyle->m_itemSet.m_whichToItemMap) {
          if (it.second && it.second->m_attribute)
            it.second->m_attribute->addTo(state);
        }
      }
    }
    for (auto &item : m_itemList) {
      if (item && item->m_attribute)
        item->m_attribute->addTo(state);
    }
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicAttribute const &graph)
  {
    o << graph.getName() << ",";
    for (auto &item : graph.m_itemList) {
      if (!item || !item->m_attribute) continue;
      libstoff::DebugStream f;
      item->m_attribute->printData(f);
      o << "[" << f.str() << "],";
    }
    if (!graph.m_sheetStyle.empty()) o << "sheetStyle[name]=" << graph.m_sheetStyle.cstr() << ",";
    return o;
  }
  //! the list of star item
  std::vector<std::shared_ptr<StarItem> > m_itemList;
  //! the sheet style name
  librevenge::RVNGString m_sheetStyle;
};

SdrGraphicAttribute::~SdrGraphicAttribute()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic group
class SdrGraphicGroup final : public SdrGraphic
{
public:
  //! constructor
  explicit SdrGraphicGroup(int id)
    : SdrGraphic(id)
    , m_groupName()
    , m_child()
    , m_refPoint()
    , m_hasRefPoint(false)
    , m_groupDrehWink(0)
    , m_groupShearWink(0)
  {
  }
  //! destructor
  ~SdrGraphicGroup() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphic::print() << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage) final
  {
    if (!listener) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicGroup::send: unexpected listener\n"));
      return false;
    }
    STOFFFrameStyle finalPos(pos);
    StarState state(getState(object, listener, finalPos));
    finalPos.m_position.m_offset=state.m_global->m_offset;
    //finalPos.m_position.m_offset=true;
    listener->openGroup(pos);
    for (auto &child : m_child) {
      if (child)
        child->send(listener, finalPos, object, inMasterPage);
    }
    listener->closeGroup();
    return true;
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicGroup const &graph)
  {
    o << graph.getName() << ",";
    if (!graph.m_groupName.empty()) o << graph.m_groupName.cstr() << ",";
    if (!graph.m_child.empty()) o << "num[child]=" << graph.m_child.size() << ",";
    if (graph.m_hasRefPoint) o << "refPt=" << graph.m_refPoint << ",";
    if (graph.m_groupDrehWink) o << "drehWink=" << graph.m_groupDrehWink << ",";
    if (graph.m_groupShearWink) o << "shearWink=" << graph.m_groupShearWink << ",";
    return o;
  }
  //! the group name
  librevenge::RVNGString m_groupName;
  //! the child
  std::vector<std::shared_ptr<StarObjectSmallGraphic> > m_child;
  //! the ref point
  STOFFVec2i m_refPoint;
  //! flag to know if we use the ref point
  bool m_hasRefPoint;
  //! the dreh wink: rotation?
  int m_groupDrehWink;
  //! the shear wink
  int m_groupShearWink;
};

SdrGraphicGroup::~SdrGraphicGroup()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic text
class SdrGraphicText : public SdrGraphicAttribute
{
public:
  //! constructor
  explicit SdrGraphicText(int id)
    : SdrGraphicAttribute(id)
    , m_textKind(0)
    , m_textRectangle()
    , m_textDrehWink(0)
    , m_textShearWink(0)
    , m_outlinerParaObject()
    , m_textBound()
  {
  }
  //! destructor
  ~SdrGraphicText() override;
  //! try to update the transformation
  void updateTransformProperties(librevenge::RVNGPropertyList &list, double relUnit) const
  {
    if (m_textDrehWink) {
      // rotation around the first point, let do that by hand
      librevenge::RVNGString transform;
      if (m_textRectangle[0]==STOFFVec2i(0,0))
        transform.sprintf("rotate(%f)", m_textDrehWink/100.*M_PI/180.);
      else {
        STOFFVec2f center=STOFFVec2f(m_textRectangle[0]);
        transform.sprintf("translate(%fpt %fpt) rotate(%f) translate(%fpt %fpt)",
                          -relUnit*center[0],-relUnit*center[1],
                          m_textDrehWink/100.*M_PI/180., // gradient
                          relUnit*center[0],relUnit*center[1]);
      }
      list.insert("draw:transform", transform);
    }
  }
  //! basic print function
  std::string print() const override
  {
    std::stringstream s;
    s << SdrGraphicAttribute::print() << *this << ",";
    return s.str();
  }
  //! try to send the text zone to the listener
  bool sendTextZone(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object)
  {
    // for basic text zone, we use the real textbox, for other zones (text link to a shape) we use the shape box
    STOFFBox2f const &box=(m_identifier!=3 && m_identifier!=16 && m_identifier!=17 && m_identifier!=20 && m_identifier!=21) ? m_bdbox : m_textRectangle;
    if (!listener || box.size()[0]<=0 || box.size()[1]<=0) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicText::sendTextZone: can not send a shape\n"));
      return false;
    }
    StarState state(getState(object, listener, pos));
    STOFFFrameStyle frame=pos;
    STOFFPosition &position=frame.m_position;
    position.setOrigin(state.convertPointInPoint(box[0]));
    position.setSize(state.convertVectorInPoint(box.size()));
    if (position.m_anchorTo==STOFFPosition::Unknown)
      position.setAnchor(STOFFPosition::Page);
    updateStyle(state, listener);
    // if (!state.m_graphic.m_hasBackground) state.m_graphic.m_propertyList.insert("draw:fill", "none"); checkme
    state.m_graphic.m_propertyList.insert("draw:fill", "none");
    state.m_graphic.m_propertyList.insert("draw:shadow", "hidden"); // the text is not shadowed
    if (m_textDrehWink) {
      // checkme: this can not work for a text listener. We must create a rectangle with text instead
      STOFFVec2f const &orig=position.m_origin;
      state.m_graphic.m_propertyList.insert("librevenge:rotate-cx", orig[0], librevenge::RVNG_POINT);
      state.m_graphic.m_propertyList.insert("librevenge:rotate-cy", orig[1], librevenge::RVNG_POINT);
      state.m_graphic.m_propertyList.insert("librevenge:rotate", -(m_textDrehWink/100.), librevenge::RVNG_GENERIC);
    }
    std::shared_ptr<SubDocument> doc(new SubDocument(m_outlinerParaObject));
    listener->insertTextBox(frame, doc, state.m_graphic);
    return true;
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicText const &graph)
  {
    o << graph.getName() << ",";
    o << "textKind=" << graph.m_textKind << ",";
    o << "rect=" << graph.m_textRectangle << ",";
    if (graph.m_textDrehWink) o << "drehWink=" << graph.m_textDrehWink << ",";
    if (graph.m_textShearWink) o << "shearWink=" << graph.m_textShearWink << ",";
    if (graph.m_outlinerParaObject) o << "outliner=[" << *graph.m_outlinerParaObject << "],";
    if (graph.m_textBound.size()!=STOFFVec2i(0,0)) o << "bound=" << graph.m_textBound << ",";
    return o;
  }
  //! the text kind
  int m_textKind;
  //! the text rectangle
  STOFFBox2i m_textRectangle;
  //! the dreh wink: rotation?
  int m_textDrehWink;
  //! the shear wink
  int m_textShearWink;
  //! the outliner object
  std::shared_ptr<OutlinerParaObject> m_outlinerParaObject;
  //! the text bound
  STOFFBox2i m_textBound;
};

SdrGraphicText::~SdrGraphicText()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic rectangle
class SdrGraphicRect : public SdrGraphicText
{
public:
  //! constructor
  explicit SdrGraphicRect(int id)
    : SdrGraphicText(id)
    , m_eckRag(0)
  {
  }
  //! destructor
  ~SdrGraphicRect() override;
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage) override
  {
    if (!listener || m_textRectangle.size()[0]<=0 || m_textRectangle.size()[1]<=0) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicText::send: can not send a shape\n"));
      return false;
    }
    if (inMasterPage && (m_identifier==20 || m_identifier==21))
      return false;
    if (m_identifier!=16 && m_identifier!=17 && m_identifier!=20 && m_identifier!=21) { // basic rect
      StarState state(getState(object, listener, pos));
      STOFFGraphicShape shape;
      shape.m_command=STOFFGraphicShape::C_Rectangle;
      shape.m_bdbox=STOFFBox2f(state.convertPointInPoint(m_textRectangle[0]), state.convertPointInPoint(m_textRectangle[1]));
      updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
      updateStyle(state, listener);
      listener->insertShape(pos, shape, state.m_graphic);
      if (m_outlinerParaObject)
        sendTextZone(listener, pos, object);
    }
    else
      sendTextZone(listener, pos, object);
    return true;
  }
  //! basic print function
  std::string print() const override
  {
    std::stringstream s;
    s << SdrGraphicText::print() << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicRect const &graph)
  {
    o << graph.getName() << ",";
    if (graph.m_eckRag) o << "eckRag=" << graph.m_eckRag << ",";
    return o;
  }
  //! the eckRag?
  int m_eckRag;
};

SdrGraphicRect::~SdrGraphicRect()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic caption
class SdrGraphicCaption final : public SdrGraphicRect
{
public:
  //! constructor
  SdrGraphicCaption() : SdrGraphicRect(25), m_captionPolygon(), m_captionItem()
  {
  }
  //! destructor
  ~SdrGraphicCaption() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicRect::print() << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicCaption const &graph)
  {
    o << graph.getName() << ",";
    if (!graph.m_captionPolygon.empty()) {
      o << "poly=[";
      for (auto const &p : graph.m_captionPolygon)
        o << p << ",";
      o << "],";
    }
    if (graph.m_captionItem && graph.m_captionItem->m_attribute) {
      libstoff::DebugStream f;
      graph.m_captionItem->m_attribute->printData(f);
      o << "[" << f.str() << "],";
    }
    return o;
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool /*inMasterPage*/) final
  {
    if (!listener || m_captionPolygon.empty()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicCaption::send: can not send a shape\n"));
      return false;
    }
    StarState state(getState(object, listener, pos));
    // checkme never seen
    STOFFGraphicShape shape;
    shape.m_command=STOFFGraphicShape::C_Polyline;
    StarGraphicStruct::StarPolygon polygon;
    for (auto const &p : m_captionPolygon)
      polygon.m_points.push_back(StarGraphicStruct::StarPolygon::Point(p, 0));
    librevenge::RVNGPropertyListVector path;
    polygon.addToPath(path, false, state.m_global->m_relativeUnit, state.m_global->m_offset);
    shape.m_propertyList.insert("svg:d", path);
    updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
    updateStyle(state, listener);
    listener->insertShape(pos, shape, state.m_graphic);
    return true;
  }
  //! a polygon
  std::vector<STOFFVec2i> m_captionPolygon;
  //! the caption attributes
  std::shared_ptr<StarItem> m_captionItem;
};

SdrGraphicCaption::~SdrGraphicCaption()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic circle
class SdrGraphicCircle final : public SdrGraphicRect
{
public:
  //! constructor
  explicit SdrGraphicCircle(int id) : SdrGraphicRect(id), m_circleItem()
  {
    m_angles[0]=m_angles[1]=0;
  }
  //! destructor
  ~SdrGraphicCircle() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicRect::print() << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool /*inMasterPage*/) final
  {
    if (!listener || m_textRectangle.size()[0]<=0 || m_textRectangle.size()[1]<=0) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicCircle::send: can not send a shape\n"));
      return false;
    }
    StarState state(getState(object, listener, pos));

    STOFFGraphicShape shape;
    shape.m_command=STOFFGraphicShape::C_Ellipse;
    STOFFVec2f center=state.convertPointInPoint(0.5f*STOFFVec2f(m_textRectangle[0]+m_textRectangle[1]));
    shape.m_propertyList.insert("svg:cx",20*center.x(), librevenge::RVNG_TWIP);
    shape.m_propertyList.insert("svg:cy",20*center.y(), librevenge::RVNG_TWIP);
    STOFFVec2f radius=state.convertVectorInPoint(0.5f*STOFFVec2f(m_textRectangle[1]-m_textRectangle[0]));
    shape.m_propertyList.insert("svg:rx",20*radius.x(), librevenge::RVNG_TWIP);
    shape.m_propertyList.insert("svg:ry",20*radius.y(), librevenge::RVNG_TWIP);
    if (m_identifier!=4) {
      shape.m_propertyList.insert("draw:start-angle", double(m_angles[0]), librevenge::RVNG_GENERIC);
      shape.m_propertyList.insert("draw:end-angle", double(m_angles[1]), librevenge::RVNG_GENERIC);
    }
    if (m_identifier>=4 && m_identifier<=7) {
      char const *wh[]= {"full", "section", "arc", "cut"};
      shape.m_propertyList.insert("draw:kind", wh[m_identifier-4]);
    }
    updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
    updateStyle(state, listener);
    listener->insertShape(pos, shape, state.m_graphic);
    if (m_outlinerParaObject)
      sendTextZone(listener, pos, object);
    return true;
  }
  //! try to update the style
  void updateStyle(StarState &state, STOFFListenerPtr listener) const
  {
    SdrGraphicRect::updateStyle(state, listener);
    if (m_circleItem && m_circleItem->m_attribute)
      m_circleItem->m_attribute->addTo(state);
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicCircle const &graph)
  {
    o << graph.getName() << ",";
    if (graph.m_angles[0]<0||graph.m_angles[0]>0 || graph.m_angles[1]<0||graph.m_angles[1]>0)
      o << "angles=" << graph.m_angles[0] << "x" << graph.m_angles[1] << ",";
    if (graph.m_circleItem && graph.m_circleItem->m_attribute) {
      libstoff::DebugStream f;
      graph.m_circleItem->m_attribute->printData(f);
      o << "[" << f.str() << "],";
    }
    return o;
  }
  //! the two angles
  float m_angles[2];
  //! the circle attributes
  std::shared_ptr<StarItem> m_circleItem;
};

SdrGraphicCircle::~SdrGraphicCircle()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic edge
class SdrGraphicEdge final : public SdrGraphicText
{
public:
  //! the information record
  struct Information {
    //! constructor
    Information()
      : m_orthoForm(0)
    {
      m_angles[0]=m_angles[1]=0;
      for (int &i : m_n) i=0;
    }
    //! operator=
    friend std::ostream &operator<<(std::ostream &o, Information const &info)
    {
      o << "pts=[";
      for (auto point : info.m_points) o << point << ",";
      o << "],";
      o << "angles=" << info.m_angles[0] << "x" << info.m_angles[1] << ",";
      for (int i=0; i<3; ++i) {
        if (info.m_n[i]) o << "n" << i << "=" << info.m_n[i] << ",";
      }
      if (info.m_orthoForm) o << "orthoForm=" << info.m_orthoForm << ",";
      return o;
    }
    //! some points: obj1Line2, obj1Line3, obj2Line2, obj2Line3, middleLine
    STOFFVec2i m_points[5];
    //! two angles
    int m_angles[2];
    //! some values: nObj1Lines, nObj2Lines, middleLines
    int m_n[3];
    //! orthogonal form
    int m_orthoForm;
  };
  //! constructor
  SdrGraphicEdge() : SdrGraphicText(24), m_edgePolygon(), m_edgePolygonFlags(), m_edgeItem(), m_info()
  {
  }
  //! destructor
  ~SdrGraphicEdge() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicText::print() << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicEdge const &graph)
  {
    o << graph.getName() << ",";
    if (!graph.m_edgePolygon.empty()) {
      if (graph.m_edgePolygon.size()==graph.m_edgePolygonFlags.size()) {
        o << "poly=[";
        for (size_t i=0; i<graph.m_edgePolygon.size(); ++i)
          o << graph.m_edgePolygon[i] << ":" << graph.m_edgePolygonFlags[i] << ",";
        o << "],";
      }
      else {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicEdge::operator<<: unexpected number of flags\n"));
        o << "###poly,";
      }
    }
    if (graph.m_edgeItem && graph.m_edgeItem->m_attribute) {
      libstoff::DebugStream f;
      graph.m_edgeItem->m_attribute->printData(f);
      o << "[" << f.str() << "],";
    }
    return o;
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool /*inMasterPage*/) final
  {
    if (!listener || m_edgePolygon.empty()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicEdge::send: can not send a shape\n"));
      return false;
    }
    StarState state(getState(object, listener, pos));
    STOFFGraphicShape shape;
    shape.m_command=STOFFGraphicShape::C_Connector;
    size_t numFlags=m_edgePolygonFlags.size();
    StarGraphicStruct::StarPolygon polygon;
    for (size_t p=0; p<m_edgePolygon.size(); ++p)
      polygon.m_points.push_back(StarGraphicStruct::StarPolygon::Point(m_edgePolygon[p], p<numFlags ? m_edgePolygonFlags[p] : 0));
    librevenge::RVNGPropertyListVector path;
    polygon.addToPath(path, false, state.m_global->m_relativeUnit, state.m_global->m_offset);
    shape.m_propertyList.insert("svg:d", path);
    updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
    updateStyle(state, listener);
    listener->insertShape(pos, shape, state.m_graphic);
    return true;
  }
  //! the edge polygon
  std::vector<STOFFVec2i> m_edgePolygon;
  //! the edge polygon flags
  std::vector<int> m_edgePolygonFlags;
  // TODO: store the connector
  //! the edge attributes
  std::shared_ptr<StarItem> m_edgeItem;
  //! the information record
  Information m_info;
};

SdrGraphicEdge::~SdrGraphicEdge()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic graph
class SdrGraphicGraph final : public SdrGraphicRect
{
public:
  //! constructor
  SdrGraphicGraph()
    : SdrGraphicRect(22)
    , m_graphic()
    , m_graphRectangle()
    , m_mirrored(false)
    , m_hasGraphicLink(false)
    , m_graphItem()
  {
  }
  //! destructor
  ~SdrGraphicGraph() override;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicRect::print() << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage) final
  {
    if (!listener || m_bdbox.size()[0]<=0 || m_bdbox.size()[1]<=0) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicGraph::send: can not send a shape\n"));
      return false;
    }
    if ((!m_graphic || m_graphic->m_object.isEmpty()) && m_graphNames[1].empty()) {
      static bool first=true;
      if (first) {
        first=false;
        STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicGraph::send: sorry, can not find some graphic representation\n"));
      }
      return SdrGraphicRect::send(listener, pos, object, inMasterPage);
    }
    StarState state(getState(object, listener, pos));
    auto frame=pos;
    STOFFPosition &position=frame.m_position;
    position.setOrigin(state.convertPointInPoint(m_bdbox[0]));
    position.setSize(state.convertVectorInPoint(m_bdbox.size()));
    updateStyle(state, listener);
    if (!m_graphic || m_graphic->m_object.isEmpty()) {
      // CHECKME: we need probably correct the filename, transform ":" in "/", ...
      STOFFEmbeddedObject link;
      link.m_filenameLink=m_graphNames[1];
      listener->insertPicture(frame, link, state.m_graphic);
    }
    else
      listener->insertPicture(frame, m_graphic->m_object, state.m_graphic);

    return true;
  }
  //! try to update the style
  void updateStyle(StarState &state, STOFFListenerPtr listener) const
  {
    SdrGraphicRect::updateStyle(state, listener);
    if (m_graphItem && m_graphItem->m_attribute)
      m_graphItem->m_attribute->addTo(state);
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicGraph const &graph)
  {
    o << graph.getName() << ",";
    if (graph.m_graphic) {
      if (!graph.m_graphic->m_object.isEmpty())
        o << "hasObject,";
      else if (graph.m_graphic->m_bitmap)
        o << "hasBitmap,";
    }
    if (graph.m_graphRectangle.size()[0] || graph.m_graphRectangle.size()[1]) o << "rect=" << graph.m_graphRectangle << ",";
    for (int i=0; i<3; ++i) {
      if (graph.m_graphNames[i].empty()) continue;
      o << (i==0 ? "name" : i==1 ? "file[name]" : "filter[name]") << "=" << graph.m_graphNames[i].cstr() << ",";
    }
    if (graph.m_mirrored) o << "mirrored,";
    if (graph.m_hasGraphicLink) o << "hasGraphicLink,";
    if (graph.m_graphItem && graph.m_graphItem->m_attribute) {
      libstoff::DebugStream f;
      graph.m_graphItem->m_attribute->printData(f);
      o << "[" << f.str() << "],";
    }
    return o;
  }
  //! the graphic
  std::shared_ptr<StarGraphicStruct::StarGraphic> m_graphic;
  //! the rectangle
  STOFFBox2i m_graphRectangle;
  //! the name, filename, the filtername
  librevenge::RVNGString m_graphNames[3];
  //! flag to know if the image is mirrored
  bool m_mirrored;
  //! flag to know if the image has a graphic link
  bool m_hasGraphicLink;
  //! the graph attributes
  std::shared_ptr<StarItem> m_graphItem;
};

SdrGraphicGraph::~SdrGraphicGraph()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic edge
class SdrGraphicMeasure final : public SdrGraphicText
{
public:
  //! constructor
  SdrGraphicMeasure()
    : SdrGraphicText(29)
    , m_overwritten(false)
    , m_measureItem()
  {
  }
  //! destructor
  ~SdrGraphicMeasure() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicText::print() << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool /*inMasterPage*/) final
  {
    STOFFGraphicShape shape;
    StarState state(getState(object, listener, pos));
    updateStyle(state, listener);
    librevenge::RVNGPropertyListVector vect;
    shape.m_command=STOFFGraphicShape::C_Polyline;
    shape.m_propertyList.insert("draw:show-unit", true);
    librevenge::RVNGPropertyList list;
    for (auto &measurePoint : m_measurePoints) {
      auto measure=state.convertPointInPoint(measurePoint);
      list.insert("svg:x",measure[0], librevenge::RVNG_POINT);
      list.insert("svg:y",measure[1], librevenge::RVNG_POINT);
      vect.append(list);
    }
    shape.m_propertyList.insert("svg:points", vect);
    updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
    listener->insertShape(pos, shape, state.m_graphic);
    return true;
  }
  //! try to update the style
  void updateStyle(StarState &state, STOFFListenerPtr listener) const
  {
    SdrGraphicText::updateStyle(state, listener);
    if (m_measureItem && m_measureItem->m_attribute)
      m_measureItem->m_attribute->addTo(state);
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicMeasure const &graph)
  {
    o << graph.getName() << ",";
    if (graph.m_overwritten) o << "overwritten,";
    o << "pts=[";
    for (auto measurePoint : graph.m_measurePoints) o << measurePoint << ",";
    o << "],";
    if (graph.m_measureItem && graph.m_measureItem->m_attribute) {
      libstoff::DebugStream f;
      graph.m_measureItem->m_attribute->printData(f);
      o << "[" << f.str() << "],";
    }
    return o;
  }
  //! the points
  STOFFVec2i m_measurePoints[2];
  //! overwritten flag
  bool m_overwritten;
  //! the measure attributes
  std::shared_ptr<StarItem> m_measureItem;
};

SdrGraphicMeasure::~SdrGraphicMeasure()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic OLE
class SdrGraphicOLE final : public SdrGraphicRect
{
public:
  //! constructor
  explicit SdrGraphicOLE(int id)
    : SdrGraphicRect(id)
    , m_graphic()
    , m_oleParser()
  {
  }
  //! destructor
  ~SdrGraphicOLE() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicRect::print() << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage) final
  {
    if (!listener || m_bdbox.size()[0]<=0 || m_bdbox.size()[1]<=0) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: can not send a shape\n"));
      return false;
    }
    StarState state(getState(object, listener, pos));
    STOFFPosition &position=state.m_frame.m_position;
    position=pos.m_position;
    position.setOrigin(state.convertPointInPoint(m_bdbox[0]));
    position.setSize(state.convertVectorInPoint(m_bdbox.size()));
    updateStyle(state, listener);
    STOFFEmbeddedObject localPicture;
    if (!m_oleNames[0].empty() && m_oleParser) {
      auto dir=m_oleParser->getDirectory(m_oleNames[0].cstr());
      std::shared_ptr<StarObject> localObj;
      if (!dir || !StarFileManager::readOLEDirectory(m_oleParser, dir, localPicture, localObj) || localPicture.isEmpty()) {
        if (localObj) {
          auto chart=std::dynamic_pointer_cast<StarObjectChart>(localObj);
          if (chart && chart->send(listener, state.m_frame, state.m_graphic)) {
            if (m_graphic && !m_graphic->m_object.isEmpty()) {
              STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: find extra graphic for chart %s\n", m_oleNames[0].cstr()));
            }
            return true;
          }
          auto math=std::dynamic_pointer_cast<StarObjectMath>(localObj);
          if (math && math->send(listener, state.m_frame, state.m_graphic)) {
            if (m_graphic && !m_graphic->m_object.isEmpty()) {
              STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: find extra graphic for math %s\n", m_oleNames[0].cstr()));
            }
            return true;
          }
          if (std::dynamic_pointer_cast<StarObjectText>(localObj)) {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: sorry, unsure how to send a text object %s\n", m_oleNames[0].cstr()));
          }
          else {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: sorry, unexpected type for object %s\n", m_oleNames[0].cstr()));
          }
        }
        else {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: sorry, can not find object %s\n", m_oleNames[0].cstr()));
        }
      }
    }
    if (m_graphic && !m_graphic->m_object.isEmpty()) {
      size_t numTypes=m_graphic->m_object.m_typeList.size();
      for (size_t i=0; i<m_graphic->m_object.m_dataList.size(); ++i) {
        if (m_graphic->m_object.m_dataList[i].empty())
          continue;
        if (i<numTypes)
          localPicture.add(m_graphic->m_object.m_dataList[i], m_graphic->m_object.m_typeList[i]);
        else
          localPicture.add(m_graphic->m_object.m_dataList[i]);
      }
    }
    if (localPicture.isEmpty()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicOLE::send: sorry, can not find some graphic representation\n"));
      return SdrGraphicRect::send(listener, pos, object, inMasterPage);
    }
    listener->insertPicture(state.m_frame, localPicture, state.m_graphic);

    return true;
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicOLE const &graph)
  {
    o << graph.getName() << ",";
    for (int i=0; i<2; ++i) {
      if (!graph.m_oleNames[i].empty())
        o << (i==0 ? "persist" : "program") << "[name]=" << graph.m_oleNames[i].cstr() << ",";
    }
    if (graph.m_graphic) {
      if (!graph.m_graphic->m_object.isEmpty())
        o << "hasObject,";
      else if (graph.m_graphic->m_bitmap)
        o << "hasBitmap,";
    }
    return o;
  }
  //! the persist and the program name
  librevenge::RVNGString m_oleNames[2];
  //! the graphic
  std::shared_ptr<StarGraphicStruct::StarGraphic> m_graphic;
  //! the ole parser
  std::shared_ptr<STOFFOLEParser> m_oleParser;
};

SdrGraphicOLE::~SdrGraphicOLE()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic page
class SdrGraphicPage final : public SdrGraphic
{
public:
  //! constructor
  SdrGraphicPage()
    : SdrGraphic(28)
    , m_page(0)
  {
  }
  //! destructor
  ~SdrGraphicPage() override;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphic::print() << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &/*listener*/, STOFFFrameStyle const &/*pos*/, StarObject &/*object*/, bool /*inMasterPage*/) final
  {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicPage::send: unexpected call\n"));
    return false;
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicPage const &graph)
  {
    if (graph.m_page>=0) o << "page=" << graph.m_page << ",";
    return o;
  }
  //! the page
  int m_page;
};

SdrGraphicPage::~SdrGraphicPage()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic path
class SdrGraphicPath final : public SdrGraphicText
{
public:
  //! constructor
  explicit SdrGraphicPath(int id)
    : SdrGraphicText(id)
    , m_pathPolygons()
  {
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool /*inMasterPage*/) final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicText::print() << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicPath const &graph)
  {
    o << graph.getName() << ",";
    if (!graph.m_pathPolygons.empty()) {
      for (size_t p=0; p<graph.m_pathPolygons.size(); ++p)
        o << "poly" << p << "=[" << graph.m_pathPolygons[p] << "],";
    }
    return o;
  }
  //! the path polygon
  std::vector<StarGraphicStruct::StarPolygon> m_pathPolygons;
};

bool SdrGraphicPath::send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage)
{
  if (!listener || m_pathPolygons.empty() || m_pathPolygons[0].empty()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicPath::send: can not send a shape\n"));
    return false;
  }

  STOFFGraphicShape shape;
  StarState state(getState(object, listener, pos));
  updateStyle(state, listener);
  librevenge::RVNGPropertyListVector vect;
  bool isClosed=false;
  switch (m_identifier) {
  case 2: // line
    if (m_pathPolygons.size()==2) {
      // version <6 : two poly, one for each arrow?
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicPath::send: find a line defined by two polygons, unsure\n"));
        first=false;
      }
      if (m_pathPolygons[0].empty() || m_pathPolygons[1].empty()) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicPath::send: the number of points is bad for a line\n"));
        return false;
      }
      shape.m_command=STOFFGraphicShape::C_Polyline;
      for (size_t i=0; i<2; ++i) {
        librevenge::RVNGPropertyList list;
        auto pt=state.convertPointInPoint(m_pathPolygons[i].m_points[0].m_point);
        list.insert("svg:x",pt[0], librevenge::RVNG_POINT);
        list.insert("svg:y",pt[1], librevenge::RVNG_POINT);
        vect.append(list);
      }
      shape.m_propertyList.insert("svg:points", vect);
      updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
      listener->insertShape(pos, shape, state.m_graphic);
      if (m_outlinerParaObject)
        sendTextZone(listener, pos, object);
      return true;
    }
    if (m_pathPolygons[0].size()!=2) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicPath::send: the number of points is bad for a line\n"));
      return false;
    }
    break;
  case 8: // polygon
  case 11: // pathfill
  case 13: // freefill
  case 15: // splinefill
  case 26: // pathpoly
    isClosed=true;
    break;
  case 9: // polyline
  case 10: // pathline
  case 12: // freeline
  case 14: // splineline
  case 27: // pathpline
    break;
  default:
    STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SdrGraphicPath::send: find unexpected identifier %d\n", m_identifier));
    return SdrGraphicText::send(listener, pos, object, inMasterPage);
  }

  // first check if we have some spline, bezier flags
  bool hasSpecialPoint=false;
  for (auto const &poly : m_pathPolygons) {
    if (poly.hasSpecialPoints()) {
      hasSpecialPoint=true;
      break;
    }
  }
  if (!hasSpecialPoint && m_pathPolygons.size()==1) {
    shape.m_command=isClosed ? STOFFGraphicShape::C_Polygon : STOFFGraphicShape::C_Polyline;
    librevenge::RVNGPropertyList list;
    for (size_t i=0; i<m_pathPolygons[0].size(); ++i) {
      auto pt=state.convertPointInPoint(m_pathPolygons[0].m_points[i].m_point);
      list.insert("svg:x",pt[0], librevenge::RVNG_POINT);
      list.insert("svg:y",pt[1], librevenge::RVNG_POINT);
      vect.append(list);
    }
    shape.m_propertyList.insert("svg:points", vect);
  }
  else {
    shape.m_command=STOFFGraphicShape::C_Path;
    librevenge::RVNGPropertyListVector path;
    for (auto const &poly : m_pathPolygons)
      poly.addToPath(path, isClosed, state.m_global->m_relativeUnit, state.m_global->m_offset);
    shape.m_propertyList.insert("svg:d", path);
  }
  updateTransformProperties(shape.m_propertyList, state.m_global->m_relativeUnit);
  listener->insertShape(pos, shape, state.m_graphic);
  if (m_outlinerParaObject)
    sendTextZone(listener, pos, object);
  return true;
}

////////////////////////////////////////
//! Internal: virtual class to store a Sdr graphic uno
class SdrGraphicUno final : public SdrGraphicRect
{
public:
  //! constructor
  SdrGraphicUno()
    : SdrGraphicRect(32)
    , m_unoName()
  {
  }
  //! destructor
  ~SdrGraphicUno() final;
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << SdrGraphicRect::print() << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SdrGraphicUno const &graph)
  {
    o << graph.getName() << ",";
    if (!graph.m_unoName.empty()) o << graph.m_unoName.cstr() << ",";
    return o;
  }
  //! the uno name
  librevenge::RVNGString m_unoName;
};

SdrGraphicUno::~SdrGraphicUno()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a user data
class SDRUserData : public Graphic
{
public:
  // constructor
  explicit SDRUserData(int id=-1)
    : Graphic(id)
  {
  }
  //! return the object name
  std::string getName() const override
  {
    return "userData";
  }
  //! basic print function
  std::string print() const override;
};

std::string SDRUserData::print() const
{
  return "unknown";
}
////////////////////////////////////////
//! Internal: virtual class to store a SCHU graphic
class SCHUGraphic final : public SDRUserData
{
public:
  //! constructor
  explicit SCHUGraphic(int id)
    : SDRUserData(id)
    , m_id(0)
    , m_adjust(0)
    , m_orientation(0)
    , m_column(0)
    , m_row(0)
    , m_factor(0)
    , m_group()
  {
  }
  //! destructor
  ~SCHUGraphic() final;
  //! return the object name
  std::string getName() const final
  {
    if (m_identifier>0 && m_identifier<=7) {
      char const *wh[]= {"none", "group",  "objectId", "objectAdjustId", "dataRowId",
                         "dataPointId", "lightfactorId", "axisId"
                        };
      return wh[m_identifier];
    }
    std::stringstream s;
    s << "###type=" << m_identifier << "[SCHU],";
    return s.str();
  }
  //! basic print function
  std::string print() const final
  {
    std::stringstream s;
    s << *this << ",";
    return s.str();
  }
  //! try to send the graphic to the listener
  bool send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage) final
  {
    if (m_identifier && m_group)
      return m_group->send(listener, pos, object, inMasterPage);
    static bool first=true;
    if (first) {
      first=false;
      STOFF_DEBUG_MSG(("StarObjectSmallGraphicInternal::SCHUGraphic::send: not implemented for identifier %d\n", m_identifier));
    }
    return false;
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SCHUGraphic const &graph)
  {
    o << graph.getName() << ",";
    switch (graph.m_identifier) {
    case 2:
    case 7:
      o << "id=" << graph.m_id << ",";
      break;
    case 3:
      o << "adjust=" << graph.m_adjust << ",";
      if (graph.m_orientation)
        o << "orientation=" << graph.m_orientation << ",";
      break;
    case 4:
      o << "row=" << graph.m_row << ",";
      break;
    case 5:
      o << "column=" << graph.m_column << ",";
      o << "row=" << graph.m_row << ",";
      break;
    case 6:
      o << "factor=" << graph.m_factor << ",";
      break;
    default:
      break;
    }
    return o;
  }
  //! the id
  int m_id;
  //! the adjust data
  int m_adjust;
  //! the orientation
  int m_orientation;
  //! the column
  int m_column;
  //! the row
  int m_row;
  //! the factor
  double m_factor;
  //! the group node
  std::shared_ptr<SdrGraphicGroup> m_group;
};

SCHUGraphic::~SCHUGraphic()
{
}

////////////////////////////////////////
//! Internal: virtual class to store a SDUD graphic
class SDUDGraphic : public SDRUserData
{
public:
  //! constructor
  explicit SDUDGraphic(int id)
    : SDRUserData(id)
  {
  }
  //! destructor
  ~SDUDGraphic() override;
  //! return the object name
  std::string getName() const override
  {
    if (m_identifier>0 && m_identifier<=2) {
      char const *wh[]= {"none", "animationInfo",  "imapInfo" };
      return wh[m_identifier];
    }
    std::stringstream s;
    s << "###type=" << m_identifier << "[SDUD],";
    return s.str();
  }
  //! basic print function
  std::string print() const override
  {
    std::stringstream s;
    s << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SDUDGraphic const &graph)
  {
    o << graph.getName() << ",";
    return o;
  }
};

SDUDGraphic::~SDUDGraphic()
{
}
////////////////////////////////////////
//! Internal: virtual class to store a SDUD graphic animation
class SDUDGraphicAnimation final : public SDUDGraphic
{
public:
  //! constructor
  SDUDGraphicAnimation()
    : SDUDGraphic(1)
    , m_polygon()
    , m_order(0)
  {
    for (int &value : m_values) value=0;
    for (auto &color : m_colors) color=STOFFColor::white();
    for (bool &flag : m_flags) flag=false;
    for (bool &boolean : m_booleans) boolean=false;
  }
  //! destructor
  ~SDUDGraphicAnimation() override;
  //! basic print function
  std::string print() const override
  {
    std::stringstream s;
    s << *this << ",";
    return s.str();
  }
  //! print object data
  friend std::ostream &operator<<(std::ostream &o, SDUDGraphicAnimation const &graph)
  {
    o << graph.getName() << ",";
    if (!graph.m_polygon.empty()) {
      o << "poly=[";
      for (auto const &p : graph.m_polygon)
        o << p << ",";
      o << "],";
    }
    if (graph.m_limits[0]!=STOFFVec2i(0,0)) o << "start=" << graph.m_limits[0] << ",";
    if (graph.m_limits[1]!=STOFFVec2i(0,0)) o << "end=" << graph.m_limits[1] << ",";
    for (int i=0; i<8; ++i) {
      if (!graph.m_values[i]) continue;
      char const *wh[]= {"pres[effect]", "speed", "clickAction", "pres[effect,second]", "speed[second]",
                         "invisible", "verb", "text[effect]"
                        };
      o << wh[i] << "=" << graph.m_values[i] << ",";
    }
    for (int i=0; i<3; ++i) {
      if (!graph.m_flags[i]) continue;
      char const *wh[]= {"active", "dim[previous]", "isMovie"};
      o << wh[i] << ",";
    }
    for (int i=0; i<2; ++i) {
      if (!graph.m_colors[i].isWhite())
        o << (i==0 ? "blueScreen" : "dim[color]") << "=" << graph.m_colors[i] << ",";
    }
    for (int i=0; i<3; ++i) {
      if (graph.m_names[i].empty()) continue;
      char const *wh[]= {"sound[file]", "bookmark", "sound[file,second]"};
      o << wh[i] << "=" << graph.m_names[i].cstr() << ",";
    }
    for (int i=0; i<5; ++i) {
      if (!graph.m_booleans[i]) continue;
      char const *wh[]= {"hasSound", "playFull","hasSound[second]", "playFull[second]","dim[hide]"};
      o << wh[i] << ",";
    }
    if (graph.m_order) o << "order=" << graph.m_order << ",";
    return o;
  }
  //! the polygon
  std::vector<STOFFVec2i> m_polygon;
  //! the limits start, end
  STOFFVec2i m_limits[2];
  //! the values: presentation effect, speed, clickAction, presentation effect[second], speed[second], invisible, verb, text effect
  int m_values[8];
  //! the colors
  STOFFColor m_colors[2];
  //! some flags : active, dim[previous], isMovie
  bool m_flags[3];
  //! some bool : hasSound, playFull, sound[second], playFull[second], dim[hide]
  bool m_booleans[5];
  //! the names : sound file, bookmark, sound file[second]
  librevenge::RVNGString m_names[3];
  //! the presentation order
  int m_order;
  // TODO add surrogate
};

SDUDGraphicAnimation::~SDUDGraphicAnimation()
{
}
////////////////////////////////////////
//! Internal: the state of a StarObjectSmallGraphic
struct State {
  //! constructor
  State()
    : m_graphic()
  {
  }
  //! the graphic object
  std::shared_ptr<Graphic> m_graphic;
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarObjectSmallGraphic::StarObjectSmallGraphic(StarObject const &orig, bool duplicateState)
  : StarObject(orig, duplicateState)
  , m_graphicState(new StarObjectSmallGraphicInternal::State)
{
}

StarObjectSmallGraphic::~StarObjectSmallGraphic()
{
}

std::ostream &operator<<(std::ostream &o, StarObjectSmallGraphic const &graphic)
{
  if (graphic.m_graphicState->m_graphic)
    o << graphic.m_graphicState->m_graphic->print();
  return o;
}

bool StarObjectSmallGraphic::send(STOFFListenerPtr &listener, STOFFFrameStyle const &pos, StarObject &object, bool inMasterPage)
{
  if (!listener) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::send: can not find the listener\n"));
    return false;
  }
  if (!m_graphicState->m_graphic) {
    static bool first=true;
    if (first) {
      first=false;
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::send: no object\n"));
    }
    return false;
  }
  return m_graphicState->m_graphic->send(listener, pos, object, inMasterPage);
}

////////////////////////////////////////////////////////////
// the parser
////////////////////////////////////////////////////////////

bool StarObjectSmallGraphic::readSdrObject(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrOb" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrObject)[" << zone.getRecordLevel() << "]:";
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  long lastPos=zone.getRecordLastPosition();
  if (lastPos==input->tell()) {
    zone.closeSDRHeader("SdrObject");
    return true;
  }
  // svdobj.cxx SdrObjFactory::MakeNewObject
  pos=input->tell();
  f.str("");
  f << "SdrObject:";
  magic="";
  for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
  uint16_t identifier;
  *input>>identifier;
  f << magic << ", ident=" << std::hex << identifier << std::dec << ",";
  bool ok=true;
  std::shared_ptr<StarObjectSmallGraphicInternal::Graphic> graphic;
  if (magic=="SVDr")
    graphic=readSVDRObject(zone, int(identifier));
  else if (magic=="SCHU")
    graphic=readSCHUObject(zone, int(identifier));
  else if (magic=="FM01") // FmFormInventor
    graphic=readFmFormObject(zone, int(identifier));
  // to do magic=="E3D1" // E3dInventor
  if (graphic)
    m_graphicState->m_graphic=graphic;
  else {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: can not read an object\n"));
    f << "###";
    ok=false;
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (ok) {
    pos=input->tell();
    if (pos==lastPos) {
      zone.closeSDRHeader("SdrObject");
      return true;
    }
    f.str("");
    f << "SVDR:##extra";
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObject: read object, find extra data\n"));
      first=false;
    }
    f << "##";
  }
  if (pos!=input->tell())
    ascFile.addDelimiter(input->tell(),'|');

  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeSDRHeader("SdrObject");
  return true;
}

////////////////////////////////////////////////////////////
//  SVDR
////////////////////////////////////////////////////////////
std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphic> StarObjectSmallGraphic::readSVDRObject(StarZone &zone, int identifier)
{
  STOFFInputStreamPtr input=zone.input();
  long pos;

  long endPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  bool ok=true;
  std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphic> graphic;
  switch (identifier) {
  case 1: { // group
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicGroup> graphicGroup(new StarObjectSmallGraphicInternal::SdrGraphicGroup(identifier));
    graphic=graphicGroup;
    ok=readSVDRObjectGroup(zone, *graphicGroup);
    break;
  }
  case 2: // line
  case 8: // poly
  case 9: // polyline
  case 10: // pathline
  case 11: // pathfill
  case 12: // freeline
  case 13: // freefill
  case 26: // pathpoly
  case 27: { // pathline
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicPath> graphicPath(new StarObjectSmallGraphicInternal::SdrGraphicPath(identifier));
    graphic=graphicPath;
    ok=readSVDRObjectPath(zone, *graphicPath);
    break;
  }
  case 4: // circle
  case 5: // sector
  case 6: // arc
  case 7: { // cut
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicCircle> graphicCircle(new StarObjectSmallGraphicInternal::SdrGraphicCircle(identifier));
    graphic=graphicCircle;
    ok=readSVDRObjectCircle(zone, *graphicCircle);
    break;
  }
  case 3: // rect
  case 16: // text
  case 17: // textextended
  case 20: // title text
  case 21: { // outline text
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicRect> graphicRect(new StarObjectSmallGraphicInternal::SdrGraphicRect(identifier));
    graphic=graphicRect;
    ok=readSVDRObjectRect(zone, *graphicRect);
    break;
  }
  case 24: { // edge
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicEdge> graphicEdge(new StarObjectSmallGraphicInternal::SdrGraphicEdge());
    graphic=graphicEdge;
    ok=readSVDRObjectEdge(zone, *graphicEdge);
    break;
  }
  case 22: { // graph
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicGraph> graphicGraph(new StarObjectSmallGraphicInternal::SdrGraphicGraph());
    graphic=graphicGraph;
    ok=readSVDRObjectGraph(zone, *graphicGraph);
    break;
  }
  case 23: // ole
  case 31: { // frame
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicOLE> graphicOLE(new StarObjectSmallGraphicInternal::SdrGraphicOLE(identifier));
    graphic=graphicOLE;
    ok=readSVDRObjectOLE(zone, *graphicOLE);
    break;
  }
  case 25: { // caption
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicCaption> graphicCaption(new StarObjectSmallGraphicInternal::SdrGraphicCaption());
    graphic=graphicCaption;
    ok=readSVDRObjectCaption(zone, *graphicCaption);
    break;
  }
  case 28: { // page
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicPage> graphicPage(new StarObjectSmallGraphicInternal::SdrGraphicPage());
    graphic=graphicPage;
    ok=readSVDRObjectHeader(zone, *graphicPage);
    if (!ok) break;
    pos=input->tell();
    if (!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not open page record\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
    graphicPage->m_page=int(input->readULong(2));
    f << "SVDR[page]:page=" << graphicPage->m_page << ",";
    ok=input->tell()<=zone.getRecordLastPosition();
    if (!ok)
      f << "###";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord("SVDR");
    break;
  }
  case 29: { // measure
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicMeasure> graphicMeasure(new StarObjectSmallGraphicInternal::SdrGraphicMeasure());
    graphic=graphicMeasure;
    ok=readSVDRObjectMeasure(zone, *graphicMeasure);
    break;
  }
  case 32: { // uno
    std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicUno> graphicUno(new StarObjectSmallGraphicInternal::SdrGraphicUno());
    graphic=graphicUno;
    ok=readSVDRObjectRect(zone, *graphicUno);
    pos=input->tell();
    if (!zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not open uno record\n"));
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ok=false;
      break;
    }
    f << "SVDR[uno]:";
    // + SdrUnoObj::ReadData (checkme)
    std::vector<uint32_t> string;
    if (input->tell()!=zone.getRecordLastPosition() && (!zone.readString(string) || input->tell()>zone.getRecordLastPosition())) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not read uno string\n"));
      f << "###uno";
      ok=false;
    }
    else if (!string.empty()) {
      graphicUno->m_unoName=libstoff::getString(string);
      f << graphicUno->m_unoName.cstr() << ",";
    }
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    zone.closeRecord("SVDR");
    break;
  }
  default:
    graphic.reset(new StarObjectSmallGraphicInternal::SdrGraphic(identifier));
    ok=readSVDRObjectHeader(zone, *graphic);
    break;
  }
  pos=input->tell();
  if (!ok) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: can not read some zone\n"));
    ascFile.addPos(pos);
    ascFile.addNote("Entries(SVDR):###");
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    graphic.reset(new StarObjectSmallGraphicInternal::SdrGraphic(identifier));
    return graphic;
  }
  if (input->tell()==endPos)
    return graphic;
  graphic.reset(new StarObjectSmallGraphicInternal::SdrGraphic(identifier));
  static bool first=true;
  if (first) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: find unexpected data\n"));
  }
  if (identifier<=0 || identifier>32) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObject: unknown identifier\n"));
    ascFile.addPos(pos);
    ascFile.addNote("Entries(SVDR):###");
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
    return graphic;
  }

  while (input->tell()<endPos) {
    pos=input->tell();
    f.str("");
    f << "SVDR:id=" << identifier << ",###unknown,";
    if (!zone.openRecord())
      return graphic;
    long lastPos=zone.getRecordLastPosition();
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeRecord("SVDR");
  }
  return graphic;
}

bool StarObjectSmallGraphic::readSVDRObjectAttrib(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicAttribute &graphic)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!readSVDRObjectHeader(zone, graphic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;

  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  long lastPos=zone.getRecordLastPosition();
  auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool)
    pool=getNewItemPool(StarItemPool::T_VCControlPool);
  int vers=zone.getHeaderVersion();
  // svx_svdoattr: SdrAttrObj:ReadData
  bool ok=true;
  f << "[";
  for (int i=0; i<6; ++i) {
    if (vers<11) input->seek(2, librevenge::RVNG_SEEK_CUR);
    uint16_t const what[]= {1017/*XATTRSET_LINE*/, 1047/*XATTRSET_FILL*/, 1066/*XATTRSET_TEXT*/, 1079/*SDRATTRSET_SHADOW*/,
                            1096 /*SDRATTRSET_OUTLINER*/, 1126 /*SDRATTRSET_MISC*/
                           };
    uint16_t nWhich=what[i];
    auto item=pool->loadSurrogate(zone, nWhich, false, f);
    if (input->tell()>lastPos) { // null is ok
      f << "###";
      ok=false;
      break;
    }
    if (item)
      graphic.m_itemList.push_back(item);
    if (vers<5 && i==3) break;
    if (vers<6 && i==4) break;
  }
  f << "],";
  std::vector<uint32_t> string;
  if (ok && (!zone.readString(string) || input->tell()>lastPos)) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: can not read the sheet style name\n"));
    ok=false;
  }
  else if (!string.empty()) {
    graphic.m_sheetStyle=libstoff::getString(string);
    f << "eFamily=" << input->readULong(2) << ",";
    if (vers>0 && vers<11) // in this case, we must convert the style name
      f << "charSet=" << input->readULong(2) << ",";
  }
  if (ok && vers==9 && input->tell()+2==lastPos) // probably a charset even when string.empty()
    f << "#charSet?=" << input->readULong(2) << ",";
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectAttrib: find extra data\n"));
      f << "###extra,vers=" << vers;
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  std::string extra=f.str();
  f.str("");
  f << "SVDR[" << zone.getRecordLevel() << "]:attrib," << graphic << extra;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectCaption(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicCaption &graphic)
{
  if (!readSVDRObjectRect(zone, graphic))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:caption,";
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectCaption: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  // svx_svdocapt.cxx SdrCaptionObj::ReadData
  bool ok=true;
  uint16_t n;
  *input >> n;
  if (input->tell()+8*n>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectCaption: the number of point seems bad\n"));
    f << "###n=" << n << ",";
    ok=false;
    n=0;
  }
  for (int pt=0; pt<int(n); ++pt) {
    int dim[2];
    for (int &i : dim) i=int(input->readLong(4));
    graphic.m_captionPolygon.push_back(STOFFVec2i(dim[0],dim[1]));
  }
  if (ok) {
    auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=getNewItemPool(StarItemPool::T_XOutdevPool);
    uint16_t nWhich=1195; // SDRATTRSET_CAPTION
    std::shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos)
      f << "###";
    else
      graphic.m_captionItem=item;
  }
  f << graphic;
  if (!ok) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectCircle(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicCircle &graphic)
{
  if (!readSVDRObjectRect(zone, graphic))
    return false;
  int const &id=graphic.m_identifier;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  // svx_svdocirc SdrCircObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectCircle: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  if (id!=4) {
    for (float &angle : graphic.m_angles)
      angle=float(input->readLong(4))/100.f;
  }
  if (input->tell()!=lastPos) {
    auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=getNewItemPool(StarItemPool::T_XOutdevPool);
    uint16_t nWhich=1179; // SDRATTRSET_CIRC
    std::shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos) {
      f << "###";
    }
    else
      graphic.m_circleItem=item;
  }
  zone.closeRecord("SVDR");

  std::string extra=f.str();
  f.str("");
  f << "SVDR[" << zone.getRecordLevel() << "]:" << graphic << extra;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectEdge(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicEdge &graphic)
{
  if (!readSVDRObjectText(zone, graphic))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  // svx_svdoedge SdrEdgeObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  bool ok=true;
  if (vers<2) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: unexpected version\n"));
    f << "##badVers,";
    ok=false;
  }

  bool openRec=false;
  if (ok && vers>=11) {
    openRec=zone.openRecord();
    if (!openRec) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: can not edgeTrack record\n"));
      f << "###record";
      ok=false;
    }
  }
  if (ok) {
    uint16_t n;
    *input >> n;
    if (input->tell()+9*n>zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: the number of point seems bad\n"));
      f << "###n=" << n << ",";
      ok=false;
    }
    else {
      for (int pt=0; pt<int(n); ++pt) {
        int dim[2];
        for (int &i : dim) i=int(input->readLong(4));
        graphic.m_edgePolygon.push_back(STOFFVec2i(dim[0],dim[1]));
      }
      for (int pt=0; pt<int(n); ++pt) graphic.m_edgePolygonFlags.push_back(int(input->readULong(1)));
    }
  }
  f << graphic;
  if (openRec) {
    if (!ok) input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
    zone.closeRecord("SVDR");
  }
  if (ok && input->tell()<lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    f.str("");
    f << "SVDR[edgeB]:";
    pos=input->tell();

    for (int i=0; i<2; ++i) { // TODO: storeme
      if (!readSDRObjectConnection(zone)) {
        f << "##connector,";
        ok=false;
        break;
      }
      pos=input->tell();
    }
  }
  if (ok && input->tell()<lastPos) {
    auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
    if (!pool)
      pool=getNewItemPool(StarItemPool::T_XOutdevPool);
    uint16_t nWhich=1146; // SDRATTRSET_EDGE
    auto item=pool->loadSurrogate(zone, nWhich, false, f);
    if (!item || input->tell()>lastPos) {
      f << "###";
    }
    else {
      graphic.m_edgeItem=item;
      if (item->m_attribute)
        item->m_attribute->printData(f);
    }
  }
  if (ok && input->tell()<lastPos) {
    // svx_svdoedge.cxx SdrEdgeInfoRec operator>>
    if (input->tell()+5*8+2*4+3*2+1>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: SdrEdgeInfoRec seems too short\n"));
      ok=false;
    }
    else {
      auto &info=graphic.m_info;
      for (auto &point : info.m_points) {
        int dim[2];
        for (int &i : dim) i=int(input->readLong(4));
        point=STOFFVec2i(dim[0],dim[1]);
      }
      for (int &angle : info.m_angles) angle=int(input->readLong(4));
      for (int &i : info.m_n) i=int(input->readULong(2));
      info.m_orthoForm=int(input->readULong(1));
      f << "infoRec=[" << info << "],";
    }
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectEdge: find extra data\n"));
      f << "###extra,vers=" << vers;
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectHeader(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphic &graphic)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SVDR)[" << zone.getRecordLevel() << "]:header,";

  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  // svx_svdobj: SdrObject::ReadData
  int dim[4];    // gen.cxx operator>>(Rect) : test compression here
  for (int &i : dim) i=int(input->readLong(4));
  graphic.m_bdbox=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
  graphic.m_layerId=int(input->readULong(2));
  for (int i=0; i<2; ++i) dim[i]=int(input->readLong(4));
  graphic.m_anchorPosition=STOFFVec2i(dim[0],dim[1]);
  for (int i=0; i<5; ++i) *input >> graphic.m_flags[i];
  if (vers>=4) *input >> graphic.m_flags[5];
  bool ok=true;
  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: oops read to much data\n"));
    f << "###bad,";
    ok=false;
  }
  if (ok && vers<11) {
    // poly.cxx operator>>(Polygon) : test compression here
    uint16_t n;
    *input >> n;
    if (input->tell()+8*n>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: the number of point seems bad\n"));
      f << "###n=" << n << ",";
      ok=false;
      n=0;
    }
    for (int pt=0; pt<int(n); ++pt) {
      for (int i=0; i<2; ++i) dim[i]=int(input->readLong(4));
      graphic.m_polygon.push_back(StarObjectSmallGraphicInternal::GluePoint(dim[0],dim[1]));
    }
  }
  if (ok && vers>=11) {
    bool bTmp;
    *input >> bTmp;
    if (bTmp) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
      f.str("");
      f << "SVDR[headerB]:";
      if (!readSDRGluePointList(zone, graphic.m_polygon)) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: can not find the gluePoints record\n"));
        f << "###gluePoint";
        ok=false;
      }
      else
        pos=input->tell();
    }
  }
  f << graphic;
  if (ok) {
    bool readUser=true;
    if (vers>=11) *input >> readUser;
    if (readUser) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
      f.str("");
      f << "SVDR[headerC]:";
      if (!readSDRUserDataList(zone, vers>=11, graphic.m_userDataList)) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectHeader: can not find the data list record\n"));
        f << "###dataList";
      }
      else
        pos=input->tell();
#if 0
      std::cout << graphic.print() << "=>";
      for (auto d : graphic.m_userDataList) {
        if (d) std::cout << "[" << d->print() << "],";
      }
      std::cout << "\n";
#endif
    }
  }

  if (input->tell()!=pos) {
    if (input->tell()!=lastPos)
      ascFile.addDelimiter(input->tell(),'|');
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectGraph(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicGraph &graphic)
{
  if (!readSVDRObjectRect(zone, graphic))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  // SdrGrafObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraph: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  bool ok=true;
  if (vers<11) {
    // ReadDataTilV10
    std::shared_ptr<StarGraphicStruct::StarGraphic> smallGraphic(new StarGraphicStruct::StarGraphic);
    long begPictPos=input->tell();
    if (!smallGraphic->read(zone) || input->tell()>lastPos) {
      f << "###graphic";
      ok=false;
    }
    else {
      if (smallGraphic->m_object.isEmpty()) {
        long endPictPos=input->tell();
        // try to recover can recover here the unknown graphic
        input->seek(begPictPos, librevenge::RVNG_SEEK_SET);
        librevenge::RVNGBinaryData data;
        if (!input->readDataBlock(endPictPos-begPictPos,data)) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraph: can not retrieve unknown data\n"));
        }
        else {
          smallGraphic->m_object.add(data, "image/pct");
          graphic.m_graphic=smallGraphic;
        }
        input->seek(endPictPos, librevenge::RVNG_SEEK_SET);
      }
      else
        graphic.m_graphic=smallGraphic;
    }
    if (ok && vers>=6) {
      int dim[4];
      for (int &i : dim) i=int(input->readLong(4));
      graphic.m_graphRectangle=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
    }
    if (ok && vers>=8) {
      std::vector<uint32_t> string;
      if (!zone.readString(string) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraph: can not read the file name\n"));
        f << "###fileName";
        ok=false;
      }
      else
        graphic.m_graphNames[1]=libstoff::getString(string);
    }
    if (ok && vers>=9) {
      std::vector<uint32_t> string;
      if (!zone.readString(string) || input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraph: can not read the filter name\n"));
        f << "###filter";
        ok=false;
      }
      else
        graphic.m_graphNames[2]=libstoff::getString(string);
    }
  }
  else {
    bool hasGraphic;
    *input >> hasGraphic;
    if (hasGraphic) {
      if (!zone.openRecord()) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraph: can not open graphic record\n"));
        f << "###graphRecord";
        ok=false;
      }
      else {
        f << "graf,";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        std::shared_ptr<StarGraphicStruct::StarGraphic> smallGraphic(new StarGraphicStruct::StarGraphic);
        if (!smallGraphic->read(zone, zone.getRecordLastPosition()) || input->tell()>zone.getRecordLastPosition()) {
          ascFile.addPos(pos);
          ascFile.addNote("SVDR[graph]:##graphic");
          input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
        }
        else if (smallGraphic)
          graphic.m_graphic=smallGraphic;
        pos=input->tell();
        f.str("");
        f << "SVDR[graph]:";
        zone.closeRecord("SVDR");
      }
    }
    if (ok) {
      int dim[4];
      for (int &i : dim) i=int(input->readLong(4));
      graphic.m_graphRectangle=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
      *input >> graphic.m_mirrored;
      for (auto &graphName : graphic.m_graphNames) {
        std::vector<uint32_t> string;
        if (!zone.readString(string) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraph: can not read a string\n"));
          f << "###string";
          ok=false;
          break;
        }
        graphName=libstoff::getString(string);
      }
    }
    if (ok)
      *input >> graphic.m_hasGraphicLink;
    if (ok && input->tell()<lastPos) {
      auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
      if (!pool)
        pool=getNewItemPool(StarItemPool::T_XOutdevPool);
      uint16_t nWhich=1243; // SDRATTRSET_GRAF
      std::shared_ptr<StarItem> item=pool->loadSurrogate(zone, nWhich, false, f);
      if (!item || input->tell()>lastPos) {
        f << "###";
      }
      else
        graphic.m_graphItem=item;
    }
  }
  f << graphic;
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGraphic: find extra data\n"));
      f << "###extra";
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectGroup(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicGroup &graphic)
{
  if (!readSVDRObjectHeader(zone, graphic))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  // svx_svdogrp SdrObjGroup::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  std::vector<uint32_t> string;
  bool ok=true;
  if (!zone.readString(string) || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read the name\n"));
    ok=false;
  }
  else if (!string.empty())
    graphic.m_groupName=libstoff::getString(string);
  if (ok) {
    *input >> graphic.m_hasRefPoint;
    int dim[2];
    for (int &i : dim) i=int(input->readLong(4));
    graphic.m_refPoint=STOFFVec2i(dim[0],dim[1]);
    if (input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: the zone seems too short\n"));
      f << "###short";
    }
  }
  f << graphic;
  while (ok && input->tell()+4<lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    f.str("");
    f << "SVDR:group,";
    pos=input->tell();
    // check magic
    std::string magic("");
    for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
    input->seek(-4, librevenge::RVNG_SEEK_CUR);
    if (magic=="DrXX" && zone.openSDRHeader(magic)) {
      ascFile.addPos(pos);
      ascFile.addNote("SVDR:DrXX");
      zone.closeSDRHeader("SVDR");
      pos=input->tell();
      break;
    }
    if (magic!="DrOb")
      break;
    std::shared_ptr<StarObjectSmallGraphic> child(new StarObjectSmallGraphic(*this, true));
    if (!child->readSdrObject(zone)) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: can not read an object\n"));
      f << "###object";
      ok=false;
      break;
    }
    graphic.m_child.push_back(child);
  }
  if (ok && vers>=2) {
    graphic.m_groupDrehWink=int(input->readLong(4));
    if (graphic.m_groupDrehWink)
      f << "drehWink=" << graphic.m_groupDrehWink << ",";
    graphic.m_groupShearWink=int(input->readLong(4));
    if (graphic.m_groupShearWink)
      f << "shearWink=" << graphic.m_groupShearWink << ",";
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectGroup: find extra data\n"));
      f << "###extra";
    }
    if (input->tell()!=pos)
      ascFile.addDelimiter(input->tell(),'|');
  }
  if (pos!=lastPos) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectMeasure(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicMeasure &graphic)
{
  if (!readSVDRObjectText(zone, graphic))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  // svx_svdomeas SdrMeasureObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectMeasure: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  for (auto &measurePoint : graphic.m_measurePoints) {
    int dim[2];
    for (int &i : dim) i=int(input->readLong(4));
    measurePoint=STOFFVec2i(dim[0],dim[1]);
  }
  *input >> graphic.m_overwritten;
  auto pool=findItemPool(StarItemPool::T_XOutdevPool, false);
  if (!pool)
    pool=getNewItemPool(StarItemPool::T_XOutdevPool);
  uint16_t nWhich=1171; // SDRATTRSET_MEASURE
  auto item=pool->loadSurrogate(zone, nWhich, false, f);
  if (!item || input->tell()>lastPos) {
    f << "###";
  }
  else
    graphic.m_measureItem=item;
  zone.closeRecord("SVDR");

  std::string extra=f.str();
  f.str("");
  f << "SVDR[" << zone.getRecordLevel() << "]:" << graphic << extra;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectOLE(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicOLE &graphic)
{
  if (!readSVDRObjectRect(zone, graphic))
    return false;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  // svx_svdoole2 SdrOle2Obj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  bool ok=true;
  for (auto &oleName : graphic.m_oleNames) {
    std::vector<uint32_t> string;
    if (!zone.readString(string) || input->tell()>lastPos) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: can not read a string\n"));
      f << "###string";
      ok=false;
      break;
    }
    if (!string.empty())
      oleName=libstoff::getString(string);
  }
  if (ok) {
    graphic.m_oleParser=m_oleParser;
    bool objValid, hasGraphic;
    *input >> objValid >> hasGraphic;
    if (objValid) f << "obj[refValid],";
    if (hasGraphic) {
      std::shared_ptr<StarGraphicStruct::StarGraphic> smallGraphic(new StarGraphicStruct::StarGraphic);
      long beginPos=input->tell();
      if (!smallGraphic->read(zone, lastPos) || input->tell()>lastPos || smallGraphic->m_object.isEmpty()) {
        // try to recover can recover here the unknown graphic
        input->seek(beginPos, librevenge::RVNG_SEEK_SET);
        librevenge::RVNGBinaryData data;
        if (!input->readDataBlock(lastPos-beginPos,data)) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: can not retrieve unknown data\n"));
          f << "###graphic";
          ok=false;
        }
        else {
          smallGraphic->m_object.add(data, "image/pct");
          graphic.m_graphic=smallGraphic;
        }
      }
      else
        graphic.m_graphic=smallGraphic;
    }
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectOLE: find extra data\n"));
      f << "###extra";
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");

  std::string extra=f.str();
  f.str("");
  f << "SVDR[" << zone.getRecordLevel() << "]:" << graphic << extra;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectPath(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicPath &graphic)
{
  if (!readSVDRObjectText(zone, graphic))
    return false;
  int const &id=graphic.m_identifier;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  int vers=zone.getHeaderVersion();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:";
  // svx_svdopath SdrPathObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  bool ok=true;
  findItemPool(StarItemPool::T_XOutdevPool, false); // useme ?
  if (vers<=6 && (id==2 || id==8 || id==9)) {
    int nPoly=id==2 ? 2 : id==8 ? 1 : int(input->readULong(2));
    for (int poly=0; poly<nPoly; ++poly) {
      uint16_t n;
      *input >> n;
      if (input->tell()+8*n>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: the number of point seems bad\n"));
        f << "###n=" << n << ",";
        ok=false;
        break;
      }
      graphic.m_pathPolygons.push_back(StarGraphicStruct::StarPolygon());
      auto &polygon=graphic.m_pathPolygons.back();
      for (int pt=0; pt<int(n); ++pt) {
        int dim[2];
        for (int &i : dim) i=int(input->readLong(4));
        polygon.m_points.push_back(StarGraphicStruct::StarPolygon::Point(STOFFVec2i(dim[0],dim[1])));
      }
    }
  }
  else {
    bool recOpened=false;
    if (vers>=11) {
      recOpened=zone.openRecord();
      if (!recOpened) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: can not open zone record\n"));
        ok=false;
      }
    }
    int nPoly=ok ? int(input->readULong(2)) : 0;
    for (int poly=0; poly<nPoly; ++poly) {
      uint16_t n;
      *input >> n;
      if (input->tell()+9*n>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: the number of point seems bad\n"));
        f << "###n=" << n << ",";
        ok=false;
        break;
      }
      graphic.m_pathPolygons.push_back(StarGraphicStruct::StarPolygon());
      auto &polygon=graphic.m_pathPolygons.back();
      polygon.m_points.resize(size_t(n));
      for (size_t pt=0; pt<size_t(n); ++pt) {
        int dim[2];
        for (int &i : dim) i=int(input->readLong(4));
        polygon.m_points[pt].m_point=STOFFVec2i(dim[0],dim[1]);
      }
      for (size_t pt=0; pt<size_t(n); ++pt)
        polygon.m_points[pt].m_flags=int(input->readULong(1));
    }
    if (recOpened) {
      if (input->tell()!=zone.getRecordLastPosition()) {
        if (ok) {
          f << "##";
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectPath: find extra data\n"));
        }
        ascFile.addDelimiter(input->tell(),'|');
      }
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
      zone.closeRecord("SVDR");
    }
    ok=false;
  }
  if (!ok) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeRecord("SVDR");
  f << graphic;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectRect(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicRect &graphic)
{
  if (!readSVDRObjectText(zone, graphic))
    return false;
  int const &id=graphic.m_identifier;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  int vers=zone.getHeaderVersion();
  if (vers<3 && (id==16 || id==17 || id==20 || id==21))
    return true;

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:rectZone,";
  // svx_svdorect.cxx SdrRectObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectRect: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  if (vers<=5) graphic.m_eckRag=int(input->readLong(4));
  f << graphic;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectSmallGraphic::readSVDRObjectText(StarZone &zone, StarObjectSmallGraphicInternal::SdrGraphicText &graphic)
{
  if (!readSVDRObjectAttrib(zone, graphic))
    return false;

  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "SVDR[" << zone.getRecordLevel() << "]:textZone,";
  // svx_svdotext SdrTextObj::ReadData
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectText: can not open record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  int vers=zone.getHeaderVersion();
  graphic.m_textKind=int(input->readULong(1));
  int dim[4];
  for (int &i : dim) i=int(input->readLong(4));
  graphic.m_textRectangle=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
  graphic.m_textDrehWink=int(input->readLong(4));
  graphic.m_textShearWink=int(input->readLong(4));
  f << graphic;
  bool paraObjectValid;
  *input >> paraObjectValid;
  bool ok=input->tell()<=lastPos;
  if (paraObjectValid) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    pos=input->tell();
    f.str("");
    f << "SVDR:textB";
    if (vers>=11 && !zone.openRecord()) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectText: can not open paraObject record\n"));
      paraObjectValid=ok=false;
      f << "##paraObject";
    }
    else {
      std::shared_ptr<StarObjectSmallGraphicInternal::OutlinerParaObject> paraObject(new StarObjectSmallGraphicInternal::OutlinerParaObject);
      if (!readSDROutlinerParaObject(zone, *paraObject)) {
        ok=false;
        f << "##paraObject";
      }
      else {
        graphic.m_outlinerParaObject=paraObject;
        pos=input->tell();
      }
    }
    if (paraObjectValid && vers>=11) {
      zone.closeRecord("SdrParaObject");
      ok=true;
    }
  }
  if (ok && vers>=10) {
    bool hasBound;
    *input >> hasBound;
    if (hasBound) {
      for (int &i : dim) i=int(input->readLong(4));
      graphic.m_textBound=STOFFBox2i(STOFFVec2i(dim[0],dim[1]),STOFFVec2i(dim[2],dim[3]));
      f << "bound=" << graphic.m_textBound << ",";
    }
    ok=input->tell()<=lastPos;
  }
  if (input->tell()!=lastPos) {
    if (ok) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSVDRObjectText: find extra data\n"));
      f << "###extra, vers=" << vers;
    }
    ascFile.addDelimiter(input->tell(),'|');
  }
  if (pos!=input->tell()) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
  }
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  zone.closeRecord("SVDR");
  return true;
}

bool StarObjectSmallGraphic::readSDRObjectConnection(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  // first check magic
  std::string magic("");
  long pos=input->tell();
  for (int i=0; i<4; ++i) magic+=char(input->readULong(1));
  input->seek(pos, librevenge::RVNG_SEEK_SET);
  if (magic!="DrCn" || !zone.openSDRHeader(magic)) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  long lastPos=zone.getRecordLastPosition();
  f << "Entries(SdrObjConn)[" << zone.getRecordLevel() << "]:";
  // svx_svdoedge.cxx SdrObjConnection::Read
  int version=zone.getHeaderVersion();
  f << magic << ",nVers=" << version << ",";
  if (!readSDRObjectSurrogate(zone)) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: can not read object surrogate\n"));
    f << "###surrogate";
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrObjConn:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
    zone.closeSDRHeader("SdrObjConn");
    return true;
  }
  f << "condId=" << input->readULong(2) << ",";
  f << "dist=" << input->readLong(4) << "x" << input->readLong(4) << ",";
  for (int i=0; i<6; ++i) {
    bool val;
    *input>>val;
    if (!val) continue;
    char const *wh[]= {"bestConn", "bestVertex", "xDistOvr", "yDistOvr", "autoVertex", "autoCorner"};
    f << wh[i] << ",";
  }
  input->seek(8, librevenge::RVNG_SEEK_CUR);
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (input->tell()!=lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: find extra data\n"));
    ascFile.addPos(input->tell());
    ascFile.addNote("SdrObjConn:###extra");
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  zone.closeSDRHeader("SdrObjConn");
  return true;
}

bool StarObjectSmallGraphic::readSDRObjectSurrogate(StarZone &zone)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrObjSurr):";
  // svx_svdsuro.cxx SdrObjSurrogate::ImpRead
  auto id=int(input->readULong(1));
  f << "id=" << id << ",";
  bool ok=true;
  if (id) {
    int eid=id&0x1f;
    int nBytes=1+(id>>6);
    if (nBytes==3) {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: unexpected num bytes\n"));
      f << "###nBytes,";
      ok=false;
    }
    if (ok)
      f << "val=" << input->readULong(nBytes) << ",";
    if (ok && eid>=0x10 && eid<=0x1a)
      f << "page=" << input->readULong(2) << ",";
    if (ok && id&0x20) {
      auto grpLevel=int(input->readULong(2));
      f << "nChild=" << grpLevel << ",";
      if (input->tell()+nBytes*grpLevel>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSdrObjectConnection: num child is bas\n"));
        f << "###";
        ok=false;
      }
      else {
        f << "child=[";
        for (int i=0; i<grpLevel; ++i)
          f << input->readULong(nBytes) << ",";
        f << "],";
      }
    }
  }

  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=lastPos;
}

bool StarObjectSmallGraphic::readSDROutlinerParaObject(StarZone &zone, StarObjectSmallGraphicInternal::OutlinerParaObject &object)
{
  object=StarObjectSmallGraphicInternal::OutlinerParaObject();
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrParaObject):";
  // svx_outlobj.cxx OutlinerParaObject::Create
  auto N=long(input->readULong(4));
  f << "N=" << N << ",";
  auto syncRef=long(input->readULong(4));
  int vers=0;
  if (syncRef == 0x12345678)
    vers = 1;
  else if (syncRef == 0x22345678)
    vers = 2;
  else if (syncRef == 0x32345678)
    vers = 3;
  else if (syncRef == 0x42345678)
    vers = 4;
  else {
    f << "##syncRef,";
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not check the version\n"));
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return N==0;
  }
  object.m_version=vers;
  f << "version=" << vers << ",";
  if (vers<=3) {
    for (long i=0; i<N; ++i) {
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());

      pos=input->tell();
      f.str("");
      f << "SdrParaObject:";
      std::shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(*this, true));
      if (!smallText->read(zone, lastPos) || input->tell()>lastPos) {
        f << "###editTextObject";
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      pos=input->tell();
      StarObjectSmallGraphicInternal::OutlinerParaObject::Zone paraZone;
      paraZone.m_text=smallText;
      f << "sync=" << input->readULong(4) << ",";
      paraZone.m_depth=int(input->readULong(2));
      bool ok=true;
      if (vers==1) {
        auto flags=int(input->readULong(2));
        if (flags&1) {
          StarBitmap bitmap;
          librevenge::RVNGBinaryData data;
          std::string dType;
          if (!bitmap.readBitmap(zone, true, lastPos, data, dType)) {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not check the bitmpa\n"));
            ok=false;
          }
          else
            paraZone.m_background.add(data, dType);
        }
        else {
          if (!input->readColor(paraZone.m_backgroundColor)) {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not find a color\n"));
            f << "###aColor,";
            ok=false;
          }
          else
            input->seek(16, librevenge::RVNG_SEEK_CUR);
          std::vector<uint32_t> string;
          if (ok && (!zone.readString(string) || input->tell()>lastPos)) {
            STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: can not find string\n"));
            f << "###string,";
            ok=false;
          }
          else
            paraZone.m_colorName=libstoff::getString(string);
          if (ok)
            input->seek(12, librevenge::RVNG_SEEK_CUR);
        }
        input->seek(8, librevenge::RVNG_SEEK_CUR); // 2 long dummy
      }
      f << paraZone;
      if (input->tell()>lastPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDROutlinerParaObject: we read too much data\n"));
        f << "###bad,";
        ok=false;
      }
      if (!ok) {
        input->seek(pos, librevenge::RVNG_SEEK_SET);
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return false;
      }
      object.m_zones.push_back(paraZone);
      if (i+1!=N) f << "sync=" << input->readULong(4) << ",";
    }
    if (vers==3) {
      *input >> object.m_isEditDoc;
      if (object.m_isEditDoc) f << "isEditDoc,";
    }
  }
  else {
    pos=input->tell();
    f.str("");
    f << "SdrParaObject:";
    std::shared_ptr<StarObjectSmallText> smallText(new StarObjectSmallText(*this, true)); // checkme, we must use the text edit pool here
    if (!smallText->read(zone, lastPos) || N>(lastPos-input->tell())/2 || input->tell()+N*2>lastPos) {
      f << "###editTextObject";
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return false;
    }
    object.m_textZone=smallText;
    pos=input->tell();
    f << "depth=[";
    for (long i=0; i<N; ++i) {
      object.m_depthList.push_back(int(input->readULong(2)));
      f << object.m_depthList.back() << ",";
    }
    f << "],";
    *input >> object.m_isEditDoc;
    if (object.m_isEditDoc) f << "isEditDoc,";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarObjectSmallGraphic::readSDRGluePoint(StarZone &zone, StarObjectSmallGraphicInternal::GluePoint &pt)
{
  pt=StarObjectSmallGraphicInternal::GluePoint();
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrGluePoint):";
  // svx_svdglue_drawdoc.xx: operator>>(SdrGluePoint)
  int dim[2];
  for (int &i : dim) i=int(input->readULong(2));
  pt.m_dimension=STOFFVec2i(dim[0],dim[1]);
  pt.m_direction=int(input->readULong(2));
  pt.m_id=int(input->readULong(2));
  pt.m_align=int(input->readULong(2));
  bool noPercent;
  *input >> noPercent;
  pt.m_percent=!noPercent;
  f << "pt,";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeRecord("SdrGluePoint");
  return true;
}

bool StarObjectSmallGraphic::readSDRGluePointList
(StarZone &zone, std::vector<StarObjectSmallGraphicInternal::GluePoint> &listPoints)
{
  listPoints.clear();
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (!zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrGluePoint)[list]:";
  // svx_svdglue_drawdoc.xx: operator>>(SdrGluePointList)
  auto n=int(input->readULong(2));
  f << "n=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  for (int i=0; i<n; ++i) {
    pos=input->tell();
    StarObjectSmallGraphicInternal::GluePoint pt;
    if (!readSDRGluePoint(zone, pt) || input->tell()>lastPos) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRGluePointList: can not find a glue point\n"));
      break;
    }
    listPoints.push_back(pt);
  }
  zone.closeRecord("SdrGluePoint");
  return true;
}

std::shared_ptr<StarObjectSmallGraphicInternal::SDRUserData> StarObjectSmallGraphic::readSDRUserData(StarZone &zone, bool inRecord)
{
  std::shared_ptr<StarObjectSmallGraphicInternal::SDRUserData> res;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (inRecord && !zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return res;
  }

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrUserData):";
  // svx_svdobj.xx: SdrObject::ReadData
  long lastPos=zone.getRecordLastPosition();
  if (input->tell()+6>lastPos) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: the zone seems too short\n"));
  }
  else {
    std::string type("");
    for (int i=0; i<4; ++i) type+=char(input->readULong(1));
    auto id=int(input->readULong(2));
    f << type << ",id=" << id << ",";
    if (type=="SCHU" || type=="SDUD") {
      if (type=="SCHU")
        res=readSCHUObject(zone, id);
      else if (type=="SDUD")
        res=readSDUDObject(zone, id);
      if (!res) {
        f << "##";
        if (!inRecord) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: can not determine end size\n"));
          ascFile.addPos(pos);
          ascFile.addNote(f.str().c_str());
          return res;
        }
      }
      else if (!inRecord)
        lastPos=input->tell();
    }
    else {
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: find unknown type=%s\n", type.c_str()));
      f << "###";
      static bool first=true;
      if (first) {
        first=false;
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: reading data is not implemented\n"));
      }
      if (!inRecord) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserData: can not determine end size\n"));
        f << "##";
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        return res;
      }
      res=std::make_shared<StarObjectSmallGraphicInternal::SDRUserData>();
    }
  }
  if (input->tell()!=lastPos) {
    ascFile.addDelimiter(input->tell(),'|');
    input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (inRecord)
    zone.closeRecord("SdrUserData");
  return res;
}

bool StarObjectSmallGraphic::readSDRUserDataList(StarZone &zone, bool inRecord,
    std::vector<std::shared_ptr<StarObjectSmallGraphicInternal::SDRUserData> > &dataList)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  if (inRecord && !zone.openRecord()) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }
  long lastPos=zone.getRecordLastPosition();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SdrUserData)[list]:";
  // svx_svdglue_drawdoc.xx: operator>>(SdrUserDataList)
  auto n=int(input->readULong(2));
  f << "n=" << n << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  for (int i=0; i<n; ++i) {
    pos=input->tell();
    auto data=readSDRUserData(zone, inRecord);
    if (!data || input->tell()>lastPos) {
      input->seek(pos, librevenge::RVNG_SEEK_SET);
      STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDRUserDataList: can not find a glue point\n"));
      break;
    }
    dataList.push_back(data);
  }
  if (inRecord) zone.closeRecord("SdrUserData");
  return true;
}

////////////////////////////////////////////////////////////
// FM01 object
////////////////////////////////////////////////////////////
std::shared_ptr<StarObjectSmallGraphicInternal::Graphic> StarObjectSmallGraphic::readFmFormObject(StarZone &zone, int identifier)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(FM01):";

  if (identifier!=33) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readFmFormObject: find unknown identifier\n"));
    f << "###id=" << identifier << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return std::shared_ptr<StarObjectSmallGraphicInternal::Graphic>();
  }
  // svx_fmobj.cxx FmFormObj::ReadData
  // fixme: same code as SdrUnoObj::ReadData
  std::shared_ptr<StarObjectSmallGraphicInternal::SdrGraphicUno> graphic(new StarObjectSmallGraphicInternal::SdrGraphicUno());
  if (!readSVDRObjectRect(zone, *graphic)) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readFmFormObject: can not read rect data\n"));
    f << "###id=" << identifier << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return std::shared_ptr<StarObjectSmallGraphicInternal::Graphic>();
  }
  pos=input->tell();
  if (!zone.openRecord()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readFmFormObject: can not open uno record\n"));
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    f << "###id=" << identifier << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return std::shared_ptr<StarObjectSmallGraphicInternal::Graphic>();
  }
  f << "FM01[uno]:";
  // + SdrUnoObj::ReadData (checkme)
  std::vector<uint32_t> string;
  bool ok=true;
  if (input->tell()!=zone.getRecordLastPosition() && (!zone.readString(string) || input->tell()>zone.getRecordLastPosition())) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readFmFormObject: can not read uno string\n"));
    f << "###uno";
    ok=false;
  }
  else
    graphic->m_unoName=libstoff::getString(string);
  f << *graphic;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  if (!ok)
    input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
  zone.closeRecord("FM01");
  return graphic;
}

////////////////////////////////////////////////////////////
// SCHU object
////////////////////////////////////////////////////////////
std::shared_ptr<StarObjectSmallGraphicInternal::SDRUserData> StarObjectSmallGraphic::readSCHUObject(StarZone &zone, int identifier)
{
  std::shared_ptr<StarObjectSmallGraphicInternal::SCHUGraphic> graphic;
  if (identifier==1) {
    auto group=std::make_shared<StarObjectSmallGraphicInternal::SdrGraphicGroup>(1);
    if (readSVDRObjectGroup(zone, *group)) {
      graphic = std::make_shared<StarObjectSmallGraphicInternal::SCHUGraphic>(identifier);
      graphic->m_group=group;
    }
    return graphic;
  }
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCHU):";
  if (identifier<=0 || identifier>7) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSCHUObject: find unknown identifier\n"));
    f << "###id=" << identifier << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return graphic;
  }
  graphic = std::make_shared<StarObjectSmallGraphicInternal::SCHUGraphic>(identifier);
  // sch_objfac.cxx : SchObjFactory::MakeUserData
  auto vers=int(input->readULong(2));
  switch (identifier) {
  case 2:
  case 7:
    graphic->m_id=int(input->readULong(2));
    break;
  case 3:
    graphic->m_adjust=int(input->readULong(2));
    if (vers>=1)
      graphic->m_orientation=int(input->readULong(2));
    break;
  case 4:
    graphic->m_row=int(input->readLong(2));
    break;
  case 5:
    graphic->m_column=int(input->readLong(2));
    graphic->m_row=int(input->readLong(2));
    break;
  case 6:
    *input >> graphic->m_factor;
    break;
  default:
    f << "##";
    break;
  }
  f << *graphic;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return graphic;
}

////////////////////////////////////////////////////////////
// SDUD object
////////////////////////////////////////////////////////////
std::shared_ptr<StarObjectSmallGraphicInternal::SDRUserData> StarObjectSmallGraphic::readSDUDObject(StarZone &zone, int identifier)
{
  std::shared_ptr<StarObjectSmallGraphicInternal::SDUDGraphic> res;
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (identifier<=0 || identifier>2) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: find unknown identifier\n"));
    f << "Entries(SDUD):###id=" << identifier << ",";
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return res;
  }
  // sd_sdobjfac.cxx : SchObjFactory::MakeUserData
  auto vers=int(input->readULong(2));
  f << "vers=" << vers << ",";
  if (!zone.openSCHHeader()) {
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: can not open main record\n"));
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());

    return res;
  }
  vers=zone.getHeaderVersion();
  long endPos=zone.getRecordLastPosition();
  if (identifier==1) {
    // sd_anminfo.cxx SdAnimationInfo::ReadData
    auto graphic=std::make_shared<StarObjectSmallGraphicInternal::SDUDGraphicAnimation>();
    res=graphic;
    bool ok=true;
    if (input->readULong(2)) {
      uint16_t n;
      *input >> n;
      if (input->tell()+8*n>endPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: the number of point seems bad\n"));
        f << "###n=" << n << ",";
        ok=false;
      }
      else {
        for (int pt=0; pt<int(n); ++pt) {
          int dim[2];
          for (int &i : dim) i=int(input->readLong(4));
          graphic->m_polygon.push_back(STOFFVec2i(dim[0],dim[1]));
        }
      }
    }
    if (ok) {
      for (auto &limit : graphic->m_limits) {
        int dim[2];
        for (int &i : dim) i=int(input->readLong(4));
        limit=STOFFVec2i(dim[0],dim[1]);
      }
      for (int i=0; i<2; ++i)
        graphic->m_values[i]=int(input->readULong(2));
      for (bool &flag : graphic->m_flags) flag=input->readULong(2)!=0;
      if (input->tell()>endPos) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: the zone is too short\n"));
        f << "###short";
        ok=false;
      }
    }
    if (ok) {
      for (auto &c : graphic->m_colors) {
        STOFFColor color;
        if (!input->readColor(color) || input->tell()>endPos) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: can not find a color\n"));
          f << "###aColor,";
          ok=false;
          break;
        }
        else
          c=color;
      }
    }
    int encoding=0;
    if (ok && vers>0) {
      encoding=int(input->readULong(2));
      std::vector<uint32_t> string;
      if (ok && (!zone.readString(string, encoding) || input->tell()>endPos)) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: can not find string\n"));
        f << "###string,";
        ok=false;
      }
      else
        graphic->m_names[0]=libstoff::getString(string);
    }
    if (ok && vers>1)
      *input >> graphic->m_booleans[0];
    if (ok && vers>2)
      *input >> graphic->m_booleans[1];
    if (ok && vers>3) {
      auto nFlag=int(input->readULong(2));
      if (nFlag==1) {
        // TODO store surrogate
        ascFile.addPos(pos);
        ascFile.addNote(f.str().c_str());
        pos=input->tell();
        f.str("");
        f << "SDUD-B:";
        if (!readSDRObjectSurrogate(zone) || input->tell()>endPos) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: can not read object surrogate\n"));
          f << "###surrogate";
          ok=false;
        }
        else
          pos=input->tell();
      }
    }
    if (ok && vers>4) {
      for (int i=2; i<5; ++i)
        graphic->m_values[i]=int(input->readULong(2));
      for (int i=1; i<3; ++i) {
        std::vector<uint32_t> string;
        if (ok && (!zone.readString(string, encoding) || input->tell()>endPos)) {
          STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: can not find string\n"));
          f << "###string,";
          ok=false;
          break;
        }
        graphic->m_names[i]=libstoff::getString(string);
      }
      if (ok) {
        for (int i=5; i<7; ++i)
          graphic->m_values[i]=int(input->readULong(2));
      }
    }
    if (ok && vers>5)
      *input >> graphic->m_booleans[2] >> graphic->m_booleans[3];
    if (ok && vers>6)
      *input >> graphic->m_booleans[4];
    if (ok && vers>7)
      graphic->m_values[7]=int(input->readULong(2));
    if (ok && vers>8)
      graphic->m_order=int(input->readULong(4));
    if (input->tell()!=endPos) {
      ascFile.addDelimiter(input->tell(),'|');
      if (ok) {
        STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: find extra data\n"));
        f << "###";
      }
      input->seek(endPos, librevenge::RVNG_SEEK_SET);
    }
    std::string extra=f.str();
    f.str("");
    f << "Entries(SDUD):" << *graphic << extra;
  }
  else {
    res=std::make_shared<StarObjectSmallGraphicInternal::SDUDGraphic>(identifier);
    f.str("");
    f << "Entries(SDUD):imageMap,###";
    // imap2.cxx ImageMap::Read ; never seen, complex, so...
    STOFF_DEBUG_MSG(("StarObjectSmallGraphic::readSDUDObject: reading imageMap is not implemented\n"));
    f << "###";
    input->seek(endPos, librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  zone.closeSCHHeader("SDUD");
  return res;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
