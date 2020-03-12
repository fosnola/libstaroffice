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

/** \file STOFFSpreadsheetListener.hxx
 * Defines STOFFSpreadsheetListener: the libstaroffice spreadsheet processor listener
 *
 * \note this class is the only class which does the interface with
 * the librevenge::RVNGSpreadsheetInterface
 */
#ifndef STOFF_SPREADSHEET_LISTENER_H
#define STOFF_SPREADSHEET_LISTENER_H

#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFListener.hxx"

class STOFFCell;
class STOFFCellContent;
class STOFFChart;
class STOFFGraphicStyle;
class STOFFTable;

namespace STOFFSpreadsheetListenerInternal
{
struct DocumentState;
struct State;
}

/** This class contents the main functions needed to create a spreadsheet processing Document */
class STOFFSpreadsheetListener final : public STOFFListener
{
public:
  /** constructor */
  STOFFSpreadsheetListener(STOFFListManagerPtr &listManager, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGSpreadsheetInterface *documentInterface);
  /** destructor */
  ~STOFFSpreadsheetListener() final;

  /** returns the listener type */
  Type getType() const final
  {
    return Spreadsheet;
  }

  /** sets the document language */
  void setDocumentLanguage(std::string locale) final;
  /** sets the document meta data */
  void setDocumentMetaData(const librevenge::RVNGPropertyList &list) final;

  /** starts the document */
  void startDocument() final;
  /** ends the document */
  void endDocument(bool sendDelayedSubDoc=true) final;
  /** returns true if a document is opened */
  bool isDocumentStarted() const final;

  /** function called to add a subdocument */
  void handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType) final;
  /** returns try if a subdocument is open  */
  bool isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const final;
  /** tries to open a frame */
  bool openFrame(STOFFFrameStyle const &frame, STOFFGraphicStyle const &style=STOFFGraphicStyle()) final;
  /** tries to close a frame */
  void closeFrame() final;
  /** open a group (not implemented) */
  bool openGroup(STOFFFrameStyle const &frame) final;
  /** close a group (not implemented) */
  void closeGroup() final;

  /** returns true if we can add text data */
  bool canWriteText() const final;

  // ------ page --------
  /** returns true if a page is opened */
  bool isPageSpanOpened() const final;
  /** returns the current page span

  \note this forces the opening of a new page if no page is opened.*/
  STOFFPageSpan const &getPageSpan() final;

  // ------ header/footer --------
  /** open a header  (interaction with STOFFPageSpan which fills the parameters for openHeader) */
  bool openHeader(librevenge::RVNGPropertyList const &extras) final;
  /** open a footer  (interaction with STOFFPageSpan which fills the parameters for openFooter) */
  bool openFooter(librevenge::RVNGPropertyList const &extras) final;
  /** close a header */
  bool closeHeader() final;
  /** close a footer */
  bool closeFooter() final;
  /** insert a header */
  bool insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which) final;
  /** insert a footer */
  bool insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which) final;
  /** returns true if the header/footer is open */
  bool isHeaderFooterOpened() const final;

  // ------- sheet -----------------
  /** open a sheet*/
  void openSheet(std::vector<float> const &colWidth, librevenge::RVNGUnit unit,
                 std::vector<int> const &repeatColWidthNumber=std::vector<int>(), librevenge::RVNGString const &name="");
  /** closes this sheet */
  void closeSheet();
  /** open a row with given height ( if h < 0.0, set min-row-height = -h )*/
  void openSheetRow(float h, librevenge::RVNGUnit unit, int numRepeated=1);
  /** closes this row */
  void closeSheetRow();
  /** open a cell */
  void openSheetCell(STOFFCell const &cell, STOFFCellContent const &content, int numRepeated=1);
  /** close a cell */
  void closeSheetCell();

  // ------- chart -----------------
  /** adds a chart in given position */
  void insertChart(STOFFFrameStyle const &frame, STOFFChart &chart, STOFFGraphicStyle const &style=STOFFGraphicStyle());

  // ------ text data -----------

  //! adds a basic character, ..
  void insertChar(uint8_t character) final;
  /** adds an unicode character.
   *  By convention if \a character=0xfffd(undef), no character is added */
  void insertUnicode(uint32_t character) final;
  //! adds a unicode string
  void insertUnicodeString(librevenge::RVNGString const &str) final;

  //! adds a tab
  void insertTab() final;
  //! adds an end of line ( by default an hard one)
  void insertEOL(bool softBreak=false) final;

  // ------ text format -----------
  //! sets the font
  void setFont(STOFFFont const &font) final;
  //! returns the actual font
  STOFFFont const &getFont() const final;

  // ------ paragraph format -----------
  //! returns true if a paragraph or a list is opened
  bool isParagraphOpened() const final;
  //! sets the paragraph
  void setParagraph(STOFFParagraph const &paragraph) final;
  //! returns the actual paragraph
  STOFFParagraph const &getParagraph() const final;

  // ------ style definition -----------
  /** defines a font styles */
  void defineStyle(STOFFFont const &style) final;
  /** check if a font style with a display name is already defined */
  bool isFontStyleDefined(librevenge::RVNGString const &name) const final;
  /** defines a graphic styles */
  void defineStyle(STOFFGraphicStyle const &style) final;
  /** check if a graphic style with a display name is already defined */
  bool isGraphicStyleDefined(librevenge::RVNGString const &name) const final;
  /** defines a paragraph styles */
  void defineStyle(STOFFParagraph const &style) final;
  /** check if a paragraph style with a display name is already defined */
  bool isParagraphStyleDefined(librevenge::RVNGString const &name) const final;

  // ------- fields ----------------
  //! adds a field type
  void insertField(STOFFField const &field) final;

  // ------- link ----------------
  //! open a link
  void openLink(STOFFLink const &link) final;
  //! close a link
  void closeLink() final;

  // ------- subdocument -----------------
  /** insert a note */
  void insertNote(STOFFNote const &note, STOFFSubDocumentPtr &subDocument) final;
  /** adds comment */
  void insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &creator="", librevenge::RVNGString const &date="") final;

  /** adds a picture with potential various representationin given position */
  void insertPicture(STOFFFrameStyle const &frame, STOFFEmbeddedObject const &picture,
                     STOFFGraphicStyle const &style=STOFFGraphicStyle()) final;
  /** adds a equation given a position */
  void insertEquation(STOFFFrameStyle const &frame, librevenge::RVNGString const &equation,
                      STOFFGraphicStyle const &style=STOFFGraphicStyle()) final;
  /** adds a shape picture in given position */
  void insertShape(STOFFFrameStyle const &frame, STOFFGraphicShape const &shape, STOFFGraphicStyle const &style) final;
  /** adds a textbox in given position */
  void insertTextBox(STOFFFrameStyle const &frame, STOFFSubDocumentPtr subDocument,
                     STOFFGraphicStyle const &frameStyle=STOFFGraphicStyle()) final;
  // ------- table -----------------
  /** open a table */
  void openTable(STOFFTable const &table) final;
  /** closes this table */
  void closeTable() final;
  /** open a row with given height ( if h < 0.0, set min-row-height = -h )*/
  void openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow=false) final;
  /** closes this row */
  void closeTableRow() final;
  /** open a cell */
  void openTableCell(STOFFCell const &cell) final;
  /** close a cell */
  void closeTableCell() final;
  /** add covered cell */
  void addCoveredTableCell(STOFFVec2i const &pos) final;
  /** add empty cell */
  void addEmptyTableCell(STOFFVec2i const &pos, STOFFVec2i span=STOFFVec2i(1,1)) final;

  // ------- section ---------------
  /** returns true if we can add open a section, add page break, ... */
  bool canOpenSectionAddBreak() const final
  {
    return false;
  }
  //! returns true if a section is opened
  bool isSectionOpened() const final
  {
    return false;
  }
  //! returns the actual section
  STOFFSection const &getSection() const final;
  //! open a section if possible
  bool openSection(STOFFSection const &section) final;
  //! close a section
  bool closeSection() final;
  //! inserts a break type: ColumBreak, PageBreak, ..
  void insertBreak(BreakType breakType) final;

protected:
  //! does open a new page (low level)
  void _openPageSpan(bool sendHeaderFooters=true);
  //! does close a page (low level)
  void _closePageSpan();

  void _startSubDocument();
  void _endSubDocument();

  void _handleFrameParameters(librevenge::RVNGPropertyList &propList, STOFFFrameStyle const &frame);

  void _openParagraph();
  void _closeParagraph();
  void _resetParagraphState(const bool isListElement=false);

  /** open a list level */
  void _openListElement();
  /** close a list level */
  void _closeListElement();
  /** update the list so that it corresponds to the actual level */
  void _changeList();
  /** low level: find a list id which corresponds to actual list and a change of level.

  \note called when the list id is not set
  */
  int _getListId() const;

  void _openSpan();
  void _closeSpan();

  void _flushText();
  void _flushDeferredTabs();

  /** creates a new parsing state (copy of the actual state)
   *
   * \return the old one */
  std::shared_ptr<STOFFSpreadsheetListenerInternal::State> _pushParsingState();
  //! resets the previous parsing state
  void _popParsingState();

protected:
  //! the main parse state
  std::shared_ptr<STOFFSpreadsheetListenerInternal::DocumentState> m_ds;
  //! the actual local parse state
  std::shared_ptr<STOFFSpreadsheetListenerInternal::State> m_ps;
  //! stack of local state
  std::vector<std::shared_ptr<STOFFSpreadsheetListenerInternal::State> > m_psStack;
  //! the document interface
  librevenge::RVNGSpreadsheetInterface *m_documentInterface;

private:
  //! copy constructor (unimplemented)
  STOFFSpreadsheetListener(const STOFFSpreadsheetListener &);
  //! operator= (unimplemented)
  STOFFSpreadsheetListener &operator=(const STOFFSpreadsheetListener &);
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
