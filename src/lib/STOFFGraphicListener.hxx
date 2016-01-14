/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstoff
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

#ifndef STOFF_GRAPHIC_LISTENER_H
#define STOFF_GRAPHIC_LISTENER_H

#include <vector>

#include <librevenge/librevenge.h>

#include "libstaroffice_internal.hxx"

#include "STOFFGraphicStyle.hxx"

#include "STOFFListener.hxx"

class STOFFGraphicShape;

namespace STOFFGraphicListenerInternal
{
struct GraphicState;
struct State;
}

/** This class contains the code needed to create Graphic document.

    \note All units are specified in librevenge::RVNG_POINT
 */
class STOFFGraphicListener : public STOFFListener
{
public:
  /** constructor */
  STOFFGraphicListener(STOFFParserState &parserState, std::vector<STOFFPageSpan> const &pageList, librevenge::RVNGDrawingInterface *documentInterface);
  /** destructor */
  virtual ~STOFFGraphicListener();

  /** returns the listener type */
  Type getType() const
  {
    return Graphic;
  }

  /** sets the documents language */
  void setDocumentLanguage(std::string locale);
  /** sets the document meta data */
  void setDocumentMetaData(const librevenge::RVNGPropertyList &list);
  /** starts a new document */
  void startDocument();
  /** ends the actual document */
  void endDocument(bool sendDelayedSubDoc=true);

  // ------ general information --------
  /** returns true if a text zone is opened */
  bool canWriteText() const;
  /** returns true if a document is opened */
  bool isDocumentStarted() const;

  /** function called to add a subdocument and modify the origin*/
  void handleSubDocument(STOFFSubDocumentPtr subDocument, libstoff::SubDocumentType subDocumentType);
  /** returns try if a subdocument is open  */
  bool isSubDocumentOpened(libstoff::SubDocumentType &subdocType) const;
  /** store the position and the style (which will be needed further to insert a textbox or a table with openTable) */
  bool openFrame(STOFFPosition const &pos, STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** close a frame */
  void closeFrame();
  /** open a group */
  bool openGroup();
  /** close a group */
  void closeGroup();
  /** open a layer */
  bool openLayer(librevenge::RVNGString const &name);
  /** close a layer */
  void closeLayer();

  // ------ page --------
  /** opens a master page */
  bool openMasterPage(STOFFPageSpan &masterPage);
  /** close a master page */
  void closeMasterPage()
  {
    _closePageSpan(true);
  }
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

  // ------ text data -----------
  //! adds a basic character, ..
  void insertChar(uint8_t character);
  /** insert a character using the font converter to find the utf8
      character */
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

  // ------- fields ----------------
  //! adds a field type
  void insertField(STOFFField const &field);

  // ------- link ----------------
  //! open a link
  void openLink(STOFFLink const &link);
  //! close a link
  void closeLink();

  // ------- subdocument -----------------
  /** adds a shape picture in given position */
  void insertShape(STOFFGraphicShape const &shape, STOFFGraphicStyle const &style);
  /** adds a textbox in given position */
  void insertTextBox(STOFFPosition const &pos, STOFFSubDocumentPtr subDocument, STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** adds a picture with potential various representationin given position */
  void insertPicture(STOFFPosition const &pos, STOFFEmbeddedObject const &picture,
                     STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** insert a note

   \note as RVNGDrawingInterface does not accept note, note can only be inserted in a text zone (and are inserted between --) */
  void insertNote(STOFFNote const &note, STOFFSubDocumentPtr &subDocument);
  /** adds comment

   \note as RVNGDrawingInterface does not accept comment, comment can only be inserted in a text zone (and are inserted between --) */
  void insertComment(STOFFSubDocumentPtr &subDocument, librevenge::RVNGString const &creator="", librevenge::RVNGString const &date="");

  // ------- table -----------------

  /** adds a table in given position */
  void insertTable(STOFFPosition const &pos, STOFFTable &table, STOFFGraphicStyle const &style=STOFFGraphicStyle());
  /** open a table (using the last parameters of openFrame for the position ) */
  void openTable(STOFFTable const &table);
  /** open a table in a given position */
  void openTable(STOFFPosition const &pos, STOFFTable const &table);
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
  bool closeSection()
  {
    return false;
  }
  //! inserts a break type: ColumBreak, PageBreak, ..
  void insertBreak(BreakType breakType);

protected:
  //! does open a new page (low level)
  void _openPageSpan(bool sendHeaderFooters=true);
  //! does close a page (low level)
  void _closePageSpan(bool masterPage=false);

  void _startSubDocument();
  void _endSubDocument();

  /** adds in propList the frame parameters.

   \note if there is some gradient, first draw a rectangle to print the gradient and them update propList */
  void _handleFrameParameters(librevenge::RVNGPropertyList &propList, STOFFPosition const &pos, STOFFGraphicStyle const &style);

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

  /** creates a new parsing state (copy of the actual state)
   *
   * \return the old one */
  shared_ptr<STOFFGraphicListenerInternal::State> _pushParsingState();
  //! resets the previous parsing state
  void _popParsingState();

protected:
  //! the actual global state
  shared_ptr<STOFFGraphicListenerInternal::GraphicState> m_ds;
  //! the actual local parse state
  shared_ptr<STOFFGraphicListenerInternal::State> m_ps;
  //! stack of local state
  std::vector<shared_ptr<STOFFGraphicListenerInternal::State> > m_psStack;
  //! the parser state
  STOFFParserState &m_parserState;
  //! the document interface
  librevenge::RVNGDrawingInterface *m_documentInterface;

private:
  STOFFGraphicListener(const STOFFGraphicListener &);
  STOFFGraphicListener &operator=(const STOFFGraphicListener &);
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
