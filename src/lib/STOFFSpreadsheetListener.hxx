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
class STOFFSpreadsheetListener : public STOFFListener
{
public:
  /** constructor */
  STOFFSpreadsheetListener(STOFFListManagerPtr listManager, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGSpreadsheetInterface *documentInterface);
  /** destructor */
  virtual ~STOFFSpreadsheetListener();

  /** returns the listener type */
  Type getType() const
  {
    return Spreadsheet;
  }

  /** sets the document language */
  void setDocumentLanguage(std::string locale);
  /** sets the document meta data */
  void setDocumentMetaData(const librevenge::RVNGPropertyList &list);

  /** starts the document */
  void startDocument();
  /** ends the document */
  void endDocument(bool sendDelayedSubDoc=true);
  /** returns true if a document is opened */
  bool isDocumentStarted() const;

  /** function called to add a subdocument */
  void handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType);
  /** returns try if a subdocument is open  */
  bool isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const;
  /** tries to open a frame */
  bool openFrame(STOFFPosition const &pos, STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** tries to close a frame */
  void closeFrame();
  /** open a group (not implemented) */
  bool openGroup(STOFFPosition const &pos);
  /** close a group (not implemented) */
  void closeGroup();

  /** returns true if we can add text data */
  bool canWriteText() const;

  // ------ page --------
  /** returns true if a page is opened */
  bool isPageSpanOpened() const;
  /** returns the current page span

  \note this forces the opening of a new page if no page is opened.*/
  STOFFPageSpan const &getPageSpan();

  // ------ header/footer --------
  /** open a header  (interaction with STOFFPageSpan which fills the parameters for openHeader) */
  bool openHeader(librevenge::RVNGPropertyList const &extras);
  /** open a footer  (interaction with STOFFPageSpan which fills the parameters for openFooter) */
  bool openFooter(librevenge::RVNGPropertyList const &extras);
  /** close a header */
  bool closeHeader();
  /** close a footer */
  bool closeFooter();
  /** insert a header */
  bool insertHeaderRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which);
  /** insert a footer */
  bool insertFooterRegion(STOFFSubDocumentPtr subDocument, librevenge::RVNGString const &which);
  /** returns true if the header/footer is open */
  bool isHeaderFooterOpened() const;

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
  void insertChart(STOFFPosition const &pos, STOFFChart &chart, STOFFGraphicStyle const &style=STOFFGraphicStyle());

  // ------ text data -----------

  //! adds a basic character, ..
  void insertChar(uint8_t character);
  /** adds an unicode character.
   *  By convention if \a character=0xfffd(undef), no character is added */
  void insertUnicode(uint32_t character);
  //! adds a unicode string
  void insertUnicodeString(librevenge::RVNGString const &str);

  //! adds a tab
  void insertTab();
  //! adds an end of line ( by default an hard one)
  void insertEOL(bool softBreak=false);

  // ------ text format -----------
  //! sets the font
  void setFont(STOFFFont const &font);
  //! returns the actual font
  STOFFFont const &getFont() const;

  // ------ paragraph format -----------
  //! returns true if a paragraph or a list is opened
  bool isParagraphOpened() const;
  //! sets the paragraph
  void setParagraph(STOFFParagraph const &paragraph);
  //! returns the actual paragraph
  STOFFParagraph const &getParagraph() const;

  // ------ style definition -----------
  /** defines a graphic styles */
  void defineStyle(STOFFGraphicStyle const &style);
  /** check if a graphic style with a display name is already defined */
  bool isGraphicStyleDefined(librevenge::RVNGString const &name) const;
  /** defines a paragraph styles, return the paragraph id */
  void defineStyle(STOFFParagraph const &style);
  /** check if a paragraph style with a display name is already defined */
  bool isParagraphStyleDefined(librevenge::RVNGString const &name) const;

  // ------- fields ----------------
  //! adds a field type
  void insertField(STOFFField const &field);

  // ------- link ----------------
  //! open a link
  void openLink(STOFFLink const &link);
  //! close a link
  void closeLink();

  // ------- subdocument -----------------
  /** insert a note */
  void insertNote(STOFFNote const &note, STOFFSubDocumentPtr &subDocument);
  /** adds comment */
  void insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &creator="", librevenge::RVNGString const &date="");

  /** adds a picture with potential various representationin given position */
  void insertPicture(STOFFPosition const &pos, STOFFEmbeddedObject const &picture,
                     STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** adds a shape picture in given position */
  void insertShape(STOFFGraphicShape const &shape, STOFFGraphicStyle const &style, STOFFPosition const &pos);
  /** adds a textbox in given position */
  void insertTextBox(STOFFPosition const &pos, STOFFSubDocumentPtr subDocument,
                     STOFFGraphicStyle const &frameStyle=STOFFGraphicStyle());
  // ------- table -----------------
  /** adds a table in given position */
  void insertTable(STOFFPosition const &pos, STOFFTable &table, STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** open a table */
  void openTable(STOFFTable const &table);
  /** closes this table */
  void closeTable();
  /** open a row with given height ( if h < 0.0, set min-row-height = -h )*/
  void openTableRow(float h, librevenge::RVNGUnit unit, bool headerRow=false);
  /** closes this row */
  void closeTableRow();
  /** open a cell */
  void openTableCell(STOFFCell const &cell);
  /** close a cell */
  void closeTableCell();
  /** add empty cell */
  void addEmptyTableCell(STOFFVec2i const &pos, STOFFVec2i span=STOFFVec2i(1,1));

  // ------- section ---------------
  /** returns true if we can add open a section, add page break, ... */
  bool canOpenSectionAddBreak() const
  {
    return false;
  }
  //! returns true if a section is opened
  bool isSectionOpened() const
  {
    return false;
  }
  //! returns the actual section
  STOFFSection const &getSection() const;
  //! open a section if possible
  bool openSection(STOFFSection const &section);
  //! close a section
  bool closeSection();
  //! inserts a break type: ColumBreak, PageBreak, ..
  void insertBreak(BreakType breakType);

protected:
  //! does open a new page (low level)
  void _openPageSpan(bool sendHeaderFooters=true);
  //! does close a page (low level)
  void _closePageSpan();

  void _startSubDocument();
  void _endSubDocument();

  void _handleFrameParameters(librevenge::RVNGPropertyList &propList, STOFFPosition const &pos);

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
  shared_ptr<STOFFSpreadsheetListenerInternal::State> _pushParsingState();
  //! resets the previous parsing state
  void _popParsingState();

protected:
  //! the main parse state
  shared_ptr<STOFFSpreadsheetListenerInternal::DocumentState> m_ds;
  //! the actual local parse state
  shared_ptr<STOFFSpreadsheetListenerInternal::State> m_ps;
  //! stack of local state
  std::vector<shared_ptr<STOFFSpreadsheetListenerInternal::State> > m_psStack;
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
