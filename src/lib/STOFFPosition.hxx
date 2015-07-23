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

#ifndef STOFF_POSITION_H
#define STOFF_POSITION_H

#include <ostream>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

/** Class to define the position of an object (textbox, picture, ..) in the document
 *
 * Stores the page, object position, object size, anchor, wrapping, ...
 */
class STOFFPosition
{
public:
  //! a list of enum used to defined the anchor
  enum AnchorTo { Char, CharBaseLine, Frame, Paragraph, Page, Unknown };
  //! an enum used to define the wrapping: none, ...
  enum Wrapping { WNone, WBackground, WDynamic, WForeground, WParallel, WRunThrough };
  //! an enum used to define the relative X position
  enum XPos { XRight, XLeft, XCenter, XFull };
  //! an enum used to define the relative Y position
  enum YPos { YTop, YBottom, YCenter, YFull };

public:
  //! constructor
  STOFFPosition(STOFFVec2f const &orig=STOFFVec2f(), STOFFVec2f const &sz=STOFFVec2f(), librevenge::RVNGUnit theUnit=librevenge::RVNG_INCH):
    m_anchorTo(Unknown), m_xPos(XLeft), m_yPos(YTop), m_wrapping(WNone),
    m_page(0), m_orig(orig), m_size(sz), m_naturalSize(), m_LTClip(), m_RBClip(), m_unit(theUnit), m_order(0) {}

  virtual ~STOFFPosition() {}
  //! operator<<
  friend  std::ostream &operator<<(std::ostream &o, STOFFPosition const &pos)
  {
    STOFFVec2f dest(pos.m_orig+pos.m_size);
    o << "Pos=(" << pos.m_orig << ")x(" << dest << ")";
    switch (pos.m_unit) {
    case librevenge::RVNG_INCH:
      o << "(inch)";
      break;
    case librevenge::RVNG_POINT:
      o << "(pt)";
      break;
    case librevenge::RVNG_TWIP:
      o << "(tw)";
      break;
    case librevenge::RVNG_PERCENT:
    case librevenge::RVNG_GENERIC:
    case librevenge::RVNG_UNIT_ERROR:
    default:
      break;
    }
    if (pos.page()>0) o << ", page=" << pos.page();
    return o;
  }
  //! basic operator==
  bool operator==(STOFFPosition const &f) const
  {
    return cmp(f) == 0;
  }
  //! basic operator!=
  bool operator!=(STOFFPosition const &f) const
  {
    return cmp(f) != 0;
  }
  //! basic operator<
  bool operator<(STOFFPosition const &f) const
  {
    return cmp(f) < 0;
  }

  //! returns the frame page
  int page() const
  {
    return m_page;
  }
  //! return the frame origin
  STOFFVec2f const &origin() const
  {
    return m_orig;
  }
  //! returns the frame size
  STOFFVec2f const &size() const
  {
    return m_size;
  }
  //! returns the natural size (if known)
  STOFFVec2f const &naturalSize() const
  {
    return m_naturalSize;
  }
  //! returns the left top clipping
  STOFFVec2f const &leftTopClipping() const
  {
    return m_LTClip;
  }
  //! returns the right bottom clipping
  STOFFVec2f const &rightBottomClipping() const
  {
    return m_RBClip;
  }
  //! returns the unit
  librevenge::RVNGUnit unit() const
  {
    return m_unit;
  }
  static float getScaleFactor(librevenge::RVNGUnit orig, librevenge::RVNGUnit dest)
  {
    float actSc = 1.0, newSc = 1.0;
    switch (orig) {
    case librevenge::RVNG_TWIP:
      break;
    case librevenge::RVNG_POINT:
      actSc=20;
      break;
    case librevenge::RVNG_INCH:
      actSc = 1440;
      break;
    case librevenge::RVNG_PERCENT:
    case librevenge::RVNG_GENERIC:
    case librevenge::RVNG_UNIT_ERROR:
    default:
      STOFF_DEBUG_MSG(("STOFFPosition::getScaleFactor %d unit must not appear\n", int(orig)));
    }
    switch (dest) {
    case librevenge::RVNG_TWIP:
      break;
    case librevenge::RVNG_POINT:
      newSc=20;
      break;
    case librevenge::RVNG_INCH:
      newSc = 1440;
      break;
    case librevenge::RVNG_PERCENT:
    case librevenge::RVNG_GENERIC:
    case librevenge::RVNG_UNIT_ERROR:
    default:
      STOFF_DEBUG_MSG(("STOFFPosition::getScaleFactor %d unit must not appear\n", int(dest)));
    }
    return actSc/newSc;
  }
  //! returns a float which can be used to scale some data in object unit
  float getInvUnitScale(librevenge::RVNGUnit fromUnit) const
  {
    return getScaleFactor(fromUnit, m_unit);
  }

  //! sets the page
  void setPage(int pg) const
  {
    const_cast<STOFFPosition *>(this)->m_page = pg;
  }
  //! sets the frame origin
  void setOrigin(STOFFVec2f const &orig)
  {
    m_orig = orig;
  }
  //! sets the frame size
  void setSize(STOFFVec2f const &sz)
  {
    m_size = sz;
  }
  //! sets the natural size (if known)
  void setNaturalSize(STOFFVec2f const &naturalSz)
  {
    m_naturalSize = naturalSz;
  }
  //! sets the dimension unit
  void setUnit(librevenge::RVNGUnit newUnit)
  {
    m_unit = newUnit;
  }
  //! sets/resets the page and the origin
  void setPagePos(int pg, STOFFVec2f const &newOrig) const
  {
    const_cast<STOFFPosition *>(this)->m_page = pg;
    const_cast<STOFFPosition *>(this)->m_orig = newOrig;
  }

  //! sets the relative position
  void setRelativePosition(AnchorTo anchor, XPos X = XLeft, YPos Y=YTop)
  {
    m_anchorTo = anchor;
    m_xPos = X;
    m_yPos = Y;
  }

  //! sets the clipping position
  void setClippingPosition(STOFFVec2f lTop, STOFFVec2f rBottom)
  {
    m_LTClip = lTop;
    m_RBClip = rBottom;
  }

  //! returns background/foward order
  int order() const
  {
    return m_order;
  }
  //! set background/foward order
  void setOrder(int ord) const
  {
    m_order = ord;
  }

  //! anchor position
  AnchorTo m_anchorTo;
  //! X relative position
  XPos m_xPos;
  //! Y relative position
  YPos m_yPos;
  //! Wrapping
  Wrapping m_wrapping;

protected:
  //! basic function to compare two positions
  int cmp(STOFFPosition const &f) const
  {
    int diff = int(m_anchorTo) - int(f.m_anchorTo);
    if (diff) return diff < 0 ? -1 : 1;
    diff = int(m_xPos) - int(f.m_xPos);
    if (diff) return diff < 0 ? -1 : 1;
    diff = int(m_yPos) - int(f.m_yPos);
    if (diff) return diff < 0 ? -1 : 1;
    diff = page() - f.page();
    if (diff) return diff < 0 ? -1 : 1;
    diff = int(m_unit) - int(f.m_unit);
    if (diff) return diff < 0 ? -1 : 1;
    diff = m_orig.cmpY(f.m_orig);
    if (diff) return diff;
    diff = m_size.cmpY(f.m_size);
    if (diff) return diff;
    diff = m_naturalSize.cmpY(f.m_naturalSize);
    if (diff) return diff;
    diff = m_LTClip.cmpY(f.m_LTClip);
    if (diff) return diff;
    diff = m_RBClip.cmpY(f.m_RBClip);
    if (diff) return diff;

    return 0;
  }

  //! the page
  int m_page;
  STOFFVec2f m_orig /** the origin position in a page */, m_size /* the size of the data*/, m_naturalSize /** the natural size of the data (if known) */;
  STOFFVec2f m_LTClip /** the left top clip position */, m_RBClip /* the right bottom clip position */;
  //! the unit used in \a orig, in \a m_size and in \a m_LTClip , .... Default: in inches
  librevenge::RVNGUnit m_unit;
  //! background/foward order
  mutable int m_order;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
