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

#ifndef STOFF_LISTENER_H
#define STOFF_LISTENER_H

#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFGraphicStyle.hxx"
#include "STOFFPosition.hxx"

class STOFFCell;
class STOFFTable;

/** This class contains a virtual interface to all listener */
class STOFFListener
{
public:
  //! destructor
  virtual ~STOFFListener();

  //! the listener type
  enum Type { Graphic, Presentation, Spreadsheet, Text };
  /** the different break type */
  enum BreakType { PageBreak=0, SoftPageBreak, ColumnBreak };

  //------- generic accessor ---
  /** returns the listener type */
  virtual Type getType() const = 0;
  /** returns true if we can add text data */
  virtual bool canWriteText() const =0;
  /// return the list manager
  STOFFListManagerPtr getListManager() const
  {
    return m_listManager;
  }
  // ------ main document -------
  /** sets the documents language */
  virtual void setDocumentLanguage(std::string locale) = 0;
  /** sets the document meta data */
  virtual void setDocumentMetaData(const librevenge::RVNGPropertyList &list) = 0;
  /** starts the document */
  virtual void startDocument() = 0;
  /** returns true if a document is opened */
  virtual bool isDocumentStarted() const =0;
  /** ends the document */
  virtual void endDocument(bool sendDelayedSubDoc=true) = 0;

  // ------ page --------
  /** returns true if a page is opened */
  virtual bool isPageSpanOpened() const = 0;
  /** returns the current page span

  \note this forces the opening of a new page if no page is opened.*/
  virtual STOFFPageSpan const &getPageSpan() = 0;

  // ------ header/footer --------
  /** open a header  (interaction with STOFFPageSpan which fills the parameters for openHeader) */
  virtual bool openHeader(librevenge::RVNGPropertyList const &extras) = 0;
  /** open a footer  (interaction with STOFFPageSpan which fills the parameters for openFooter) */
  virtual bool openFooter(librevenge::RVNGPropertyList const &extras) = 0;
  /** close a header */
  virtual bool closeHeader() = 0;
  /** close a footer */
  virtual bool closeFooter() = 0;
  /** insert a header */
  virtual bool insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which) = 0;
  /** insert a footer */
  virtual bool insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which) = 0;
  /** returns true if the header/footer is open */
  virtual bool isHeaderFooterOpened() const = 0;

  // ------ text data -----------
  //! adds a basic character, ..
  virtual void insertChar(uint8_t character)=0;
  /** adds an unicode character.
   *  By convention if \a character=0xfffd(undef), no character is added */
  virtual void insertUnicode(uint32_t character)=0;
  /** try to insert a list of unicode character */
  void insertUnicodeList(std::vector<uint32_t> const &list)
  {
    if (list.empty() || !canWriteText())
      return;
    for (unsigned int i : list) {
      if (i==0x9) insertTab();
      else if (i==0xa || i==0xc) insertEOL(); // checkme: use softBreak ?
      else insertUnicode(i);
    }
  }
  //! adds a unicode string
  virtual void insertUnicodeString(librevenge::RVNGString const &str)=0;

  //! adds a tab
  virtual void insertTab()=0;
  //! adds an end of line ( by default an hard one)
  virtual void insertEOL(bool softBreak=false)=0;

  // ------ text format -----------
  //! sets the font
  virtual void setFont(STOFFFont const &font)=0;
  //! returns the actual font
  virtual STOFFFont const &getFont() const=0;

  // ------ paragraph format -----------
  //! returns true if a paragraph or a list is opened
  virtual bool isParagraphOpened() const=0;
  //! sets the paragraph
  virtual void setParagraph(STOFFParagraph const &paragraph)=0;
  //! returns the actual paragraph
  virtual STOFFParagraph const &getParagraph() const=0;

  // ------ style definition -----------
  /** defines a font styles */
  virtual void defineStyle(STOFFFont const &style) = 0;
  /** check if a font style with a display name is already defined */
  virtual bool isFontStyleDefined(librevenge::RVNGString const &name) const = 0;
  /** defines a graphic styles */
  virtual void defineStyle(STOFFGraphicStyle const &style) = 0;
  /** check if a graphic style with a display name is already defined */
  virtual bool isGraphicStyleDefined(librevenge::RVNGString const &name) const = 0;
  /** defines a paragraph styles */
  virtual void defineStyle(STOFFParagraph const &style) = 0;
  /** check if a paragraph style with a display name is already defined */
  virtual bool isParagraphStyleDefined(librevenge::RVNGString const &name) const = 0;
  // ------- fields ----------------
  //! adds a field type
  virtual void insertField(STOFFField const &field)=0;

  // ------- link ----------------

  //! open a link
  virtual void openLink(STOFFLink const &link)=0;
  //! close a link
  virtual void closeLink()=0;

  // ------- table -----------------
  /** open a table*/
  virtual void openTable(STOFFTable const &table) = 0;
  /** closes this table */
  virtual void closeTable() = 0;
  /** open a row with given height ( if h < 0.0, set min-row-height = -h )*/
  virtual void openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow=false) = 0;
  /** closes this row */
  virtual void closeTableRow() = 0;
  /** open a cell */
  virtual void openTableCell(STOFFCell const &cell) = 0;
  /** close a cell */
  virtual void closeTableCell() = 0;
  /** add covered cell */
  virtual void addCoveredTableCell(STOFFVec2i const &pos) = 0;
  /** add empty cell */
  virtual void addEmptyTableCell(STOFFVec2i const &pos, STOFFVec2i span=STOFFVec2i(1,1)) = 0;

  // ------- section ---------------
  /** returns true if we can add open a section, add page break, ... */
  virtual bool canOpenSectionAddBreak() const =0;
  //! returns true if a section is opened
  virtual bool isSectionOpened() const=0;
  //! returns the actual section
  virtual STOFFSection const &getSection() const=0;
  //! open a section if possible
  virtual bool openSection(STOFFSection const &section)=0;
  //! close a section
  virtual bool closeSection()=0;
  //! inserts a break type: ColumBreak, PageBreak, ..
  virtual void insertBreak(BreakType breakType)=0;

  // ------- subdocument ---------------
  /** insert a note */
  virtual void insertNote(STOFFNote const &note, STOFFSubDocumentPtr &subDocument)=0;
  /** adds comment */
  virtual void insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &creator="", librevenge::RVNGString const &date="") = 0;
  /** adds a equation given a position */
  virtual void insertEquation(STOFFFrameStyle const &frame, librevenge::RVNGString const &equation,
                              STOFFGraphicStyle const &style=STOFFGraphicStyle()) = 0;
  /** adds a picture with various representationin given position.
   \note by default only send the first picture*/
  virtual void insertPicture(STOFFFrameStyle const &frame, STOFFEmbeddedObject const &picture,
                             STOFFGraphicStyle const &style=STOFFGraphicStyle())=0;
  /** adds a shape picture in given position */
  virtual void insertShape(STOFFFrameStyle const &frame, STOFFGraphicShape const &shape, STOFFGraphicStyle const &style) = 0;
  /** adds a textbox in given position */
  virtual void insertTextBox(STOFFFrameStyle const &frame, STOFFSubDocumentPtr subDocument,
                             STOFFGraphicStyle const &frameStyle=STOFFGraphicStyle()) = 0;
#if 0
  /** adds a textbox in given position */
  virtual void insertTextBoxInShape(STOFFFrameStyle const &frame, STOFFSubDocumentPtr subDocument,
                                    STOFFGraphicShape const &/*shape*/,
                                    STOFFGraphicStyle const &frameStyle=STOFFGraphicStyle())
  {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("STOFFListener::insertTextBoxInShape: umimplemented, revert to basic insertTextBox\n"));
      first=false;
    }
    insertTextBox(frame, subDocument, frameStyle);
  }
#endif
  /** low level: tries to open a frame */
  virtual bool openFrame(STOFFFrameStyle const &frame, STOFFGraphicStyle const &style=STOFFGraphicStyle()) = 0;
  /** low level: tries to close the last opened frame */
  virtual void closeFrame() = 0;
  /** low level: tries to open a group */
  virtual bool openGroup(STOFFFrameStyle const &frame) = 0;
  /** low level: tries to close the last opened group */
  virtual void closeGroup() = 0;
  /** low level: function called to add a subdocument */
  virtual void handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType) = 0;
  /** returns true if a subdocument is open  */
  virtual bool isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const = 0;

protected:
  /// constructor
  explicit STOFFListener(STOFFListManagerPtr &listManager);
  /// the list manager
  STOFFListManagerPtr m_listManager;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
