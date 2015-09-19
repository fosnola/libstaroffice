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

#ifndef STOFFPAGESPAN_H
#define STOFFPAGESPAN_H

#include <vector>

#include "libstaroffice_internal.hxx"

#include "STOFFFont.hxx"

class STOFFListener;

/** a class which stores the header/footer data */
class STOFFHeaderFooter
{
public:
  /** the header/footer type */
  enum Type { HEADER, FOOTER, UNDEF };
  /** the header/footer occurrence in the file */
  enum Occurrence { ODD, EVEN, ALL, NEVER };
  /** a fixed page number position */
  enum PageNumberPosition { None = 0, TopLeft, TopCenter, TopRight, BottomLeft, BottomCenter, BottomRight };

  //! constructor
  STOFFHeaderFooter(Type const type=UNDEF, Occurrence const occurrence=NEVER);
  //! destructor
  ~STOFFHeaderFooter();
  //! returns true if the header footer is defined
  bool isDefined() const
  {
    return m_type != UNDEF;
  }
  /** send to header to the listener */
  void send(STOFFListener *listener) const;
  //! operator==
  bool operator==(STOFFHeaderFooter const &headerFooter) const;
  //! operator!=
  bool operator!=(STOFFHeaderFooter const &headerFooter) const
  {
    return !operator==(headerFooter);
  }
  //! insert a page number
  void insertPageNumberParagraph(STOFFListener *listener) const;

public:
  //! the type header/footer
  Type m_type;
  //! the actual occurrence
  Occurrence m_occurrence;
  //! the height ( if known )
  double m_height;
  //! the page number position ( or none)
  PageNumberPosition m_pageNumberPosition;
  //! the page numbering type
  libstoff::NumberingType m_pageNumberType;
  //! the page numbering font
  STOFFFont m_pageNumberFont;
  //! the document data
  STOFFSubDocumentPtr m_subDocument;
};

typedef shared_ptr<STOFFHeaderFooter> STOFFHeaderFooterPtr;

/** A class which defines the page properties */
class STOFFPageSpan
{
public:
  /** the page orientation */
  enum FormOrientation { PORTRAIT, LANDSCAPE };
  /** a fixed page number position */
  enum PageNumberPosition { None = 0, TopLeft, TopCenter, TopRight,
                            BottomLeft, BottomCenter, BottomRight
                          };
public:
  //! constructor
  STOFFPageSpan();
  //! destructor
  virtual ~STOFFPageSpan();

  //! returns the page length
  double getFormLength() const
  {
    return m_formLength;
  }
  //! returns the page width
  double getFormWidth() const
  {
    return m_formWidth;
  }
  //! returns the page orientation
  FormOrientation getFormOrientation() const
  {
    return m_formOrientation;
  }
  //! returns the left margin
  double getMarginLeft() const
  {
    return m_margins[libstoff::Left];
  }
  //! returns the right margin
  double getMarginRight() const
  {
    return m_margins[libstoff::Right];
  }
  //! returns the top margin
  double getMarginTop() const
  {
    return m_margins[libstoff::Top];
  }
  //! returns the bottom margin
  double getMarginBottom() const
  {
    return m_margins[libstoff::Bottom];
  }
  //! returns the page length (form width without margin )
  double getPageLength() const
  {
    return m_formLength-m_margins[libstoff::Top]-m_margins[libstoff::Bottom];
  }
  //! returns the page width (form width without margin )
  double getPageWidth() const
  {
    return m_formWidth-m_margins[libstoff::Left]-m_margins[libstoff::Right];
  }
  //! returns the background color
  STOFFColor backgroundColor() const
  {
    return m_backgroundColor;
  }
  int getPageNumber() const
  {
    return m_pageNumber;
  }
  int getPageSpan() const
  {
    return m_pageSpan;
  }

  //! add a header/footer on some page
  void setHeaderFooter(STOFFHeaderFooter const &headerFooter);
  //! set the total page length
  void setFormLength(const double formLength)
  {
    m_formLength = formLength;
  }
  //! set the total page width
  void setFormWidth(const double formWidth)
  {
    m_formWidth = formWidth;
  }
  //! set the form orientation
  void setFormOrientation(const FormOrientation formOrientation)
  {
    m_formOrientation = formOrientation;
  }
  //! set the page left margin
  void setMarginLeft(const double marginLeft)
  {
    m_margins[libstoff::Left] = (marginLeft >= 0) ? marginLeft : 0.01;
  }
  //! set the page right margin
  void setMarginRight(const double marginRight)
  {
    m_margins[libstoff::Right] = (marginRight >= 0) ? marginRight : 0.01;
  }
  //! set the page top margin
  void setMarginTop(const double marginTop)
  {
    m_margins[libstoff::Top] =(marginTop >= 0) ? marginTop : 0.01;
  }
  //! set the page bottom margin
  void setMarginBottom(const double marginBottom)
  {
    m_margins[libstoff::Bottom] = (marginBottom >= 0) ? marginBottom : 0.01;
  }
  //! set all the margins
  void setMargins(double margin, int wh=libstoff::LeftBit|libstoff::RightBit|libstoff::TopBit|libstoff::BottomBit)
  {
    if (margin < 0.0) margin = 0.01;
    if (wh&libstoff::LeftBit)
      m_margins[libstoff::Left]=margin;
    if (wh&libstoff::RightBit)
      m_margins[libstoff::Right]=margin;
    if (wh&libstoff::TopBit)
      m_margins[libstoff::Top]=margin;
    if (wh&libstoff::BottomBit)
      m_margins[libstoff::Bottom]=margin;
  }
  //! check if the page margins are consistent with the page dimension, if not update them
  void checkMargins();
  //! set the page name
  void setPageName(librevenge::RVNGString const &name)
  {
    m_name=name;
  }
  //! return true if the page has a name
  bool hasPageName() const
  {
    return !m_name.empty();
  }
  //! return the page name
  librevenge::RVNGString const &getPageName() const
  {
    return m_name;
  }
  //! set the page master name
  void setMasterPageName(librevenge::RVNGString const &name)
  {
    m_masterName=name;
  }
  //! return true if the masterPage has a name
  bool hasMasterPageName() const
  {
    return !m_masterName.empty();
  }
  //! return the page master name
  librevenge::RVNGString const &getMasterPageName() const
  {
    return m_masterName;
  }
  //! set the background color
  void setBackgroundColor(STOFFColor color=STOFFColor::white())
  {
    m_backgroundColor=color;
  }
  //! set the page number
  void setPageNumber(const int pageNumber)
  {
    m_pageNumber = pageNumber;
  }
  //! set the page span ( default 1)
  void setPageSpan(const int pageSpan)
  {
    m_pageSpan = pageSpan;
  }
  //! operator==
  bool operator==(shared_ptr<STOFFPageSpan> const &pageSpan) const;
  //! operator!=
  bool operator!=(shared_ptr<STOFFPageSpan> const &pageSpan) const
  {
    return !operator==(pageSpan);
  }

  // interface with STOFFListener
  //! add the page properties in pList
  void getPageProperty(librevenge::RVNGPropertyList &pList) const;
  //! send the page's headers/footers if some exists
  void sendHeaderFooters(STOFFListener *listener) const;
  //! send the page's headers/footers corresponding to an occurrence if some exists
  void sendHeaderFooters(STOFFListener *listener, STOFFHeaderFooter::Occurrence occurrence) const;

protected:
  //! return the header footer positions in m_headerFooterList
  int getHeaderFooterPosition(STOFFHeaderFooter::Type type, STOFFHeaderFooter::Occurrence occurrence);
  //! remove a header footer
  void removeHeaderFooter(STOFFHeaderFooter::Type type, STOFFHeaderFooter::Occurrence occurrence);
  //! return true if we have a header footer in this position
  bool containsHeaderFooter(STOFFHeaderFooter::Type type, STOFFHeaderFooter::Occurrence occurrence);
private:
  double m_formLength/** the form length*/, m_formWidth /** the form width */;
  /** the form orientation */
  FormOrientation m_formOrientation;
  /** the margins: libstoff::Left, ... */
  double m_margins[4];
  //! the page name
  librevenge::RVNGString m_name;
  //! the page master name
  librevenge::RVNGString m_masterName;
  /** the page background color: default white */
  STOFFColor m_backgroundColor;
  //! the page number ( or -1)
  int m_pageNumber;
  //! the list of header
  std::vector<STOFFHeaderFooter> m_headerFooterList;
  //! the number of page
  int m_pageSpan;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
