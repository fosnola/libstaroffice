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

/** \file STOFFDocument.cxx
 * libstoff API: implementation of main interface functions
 */

#include "SDAParser.hxx"
#include "SDCParser.hxx"
#include "SDGParser.hxx"
#include "SDWParser.hxx"
#include "SDXParser.hxx"

#include "STOFFHeader.hxx"
#include "STOFFGraphicDecoder.hxx"
#include "STOFFParser.hxx"
#include "STOFFPropertyHandler.hxx"
#include "STOFFSpreadsheetDecoder.hxx"

#include <libstaroffice/libstaroffice.hxx>

/** small namespace use to define private class/method used by STOFFDocument */
namespace STOFFDocumentInternal
{
std::shared_ptr<STOFFGraphicParser> getGraphicParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd);
std::shared_ptr<STOFFGraphicParser> getPresentationParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd);
std::shared_ptr<STOFFTextParser> getTextParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd);
std::shared_ptr<STOFFSpreadsheetParser> getSpreadsheetParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd);
STOFFHeader *getHeader(STOFFInputStreamPtr &input, bool strict);
bool checkHeader(STOFFInputStreamPtr &input, STOFFHeader &header, bool strict);
}

STOFFDocument::Confidence STOFFDocument::isFileFormatSupported(librevenge::RVNGInputStream *input, Kind &kind)
try
{
  kind = STOFF_K_UNKNOWN;

  if (!input) {
    STOFF_DEBUG_MSG(("STOFFDocument::isFileFormatSupported(): no input\n"));
    return STOFF_C_NONE;
  }

  STOFFInputStreamPtr ip(new STOFFInputStream(input, false));
  std::shared_ptr<STOFFHeader> header;
#ifdef DEBUG
  header.reset(STOFFDocumentInternal::getHeader(ip, false));
#else
  header.reset(STOFFDocumentInternal::getHeader(ip, true));
#endif

  if (!header.get())
    return STOFF_C_NONE;
  kind = static_cast<STOFFDocument::Kind>(header->getKind());
  return header->isEncrypted() ? STOFF_C_SUPPORTED_ENCRYPTION : STOFF_C_EXCELLENT;
}
catch (...)
{
  STOFF_DEBUG_MSG(("STOFFDocument::isFileFormatSupported: exception catched\n"));
  kind = STOFF_K_UNKNOWN;
  return STOFF_C_NONE;
}

STOFFDocument::Result STOFFDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *documentInterface, char const *password)
try
{
  if (!input)
    return STOFF_R_UNKNOWN_ERROR;

  STOFFInputStreamPtr ip(new STOFFInputStream(input, false));
  std::shared_ptr<STOFFHeader> header(STOFFDocumentInternal::getHeader(ip, false));

  if (!header.get()) return STOFF_R_UNKNOWN_ERROR;
  auto parser=STOFFDocumentInternal::getGraphicParserFromHeader(ip, header.get(), password);
  if (!parser) return STOFF_R_UNKNOWN_ERROR;
  parser->parse(documentInterface);
  return STOFF_R_OK;
}
catch (libstoff::FileException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: File exception trapped\n"));
  return STOFF_R_FILE_ACCESS_ERROR;
}
catch (libstoff::ParseException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse exception trapped\n"));
  return STOFF_R_PARSE_ERROR;
}
catch (libstoff::WrongPasswordException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse password trapped\n"));
  return STOFF_R_PASSWORD_MISSMATCH_ERROR;
}
catch (...)
{
  //fixme: too generic
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Unknown exception trapped\n"));
  return STOFF_R_UNKNOWN_ERROR;
}

STOFFDocument::Result STOFFDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGPresentationInterface *documentInterface, char const *password)
try
{
  if (!input)
    return STOFF_R_UNKNOWN_ERROR;

  STOFFInputStreamPtr ip(new STOFFInputStream(input, false));
  std::shared_ptr<STOFFHeader> header(STOFFDocumentInternal::getHeader(ip, false));

  if (!header.get()) return STOFF_R_UNKNOWN_ERROR;
  auto parser=STOFFDocumentInternal::getPresentationParserFromHeader(ip, header.get(), password);
  if (!parser) return STOFF_R_UNKNOWN_ERROR;
  parser->parse(documentInterface);
  return STOFF_R_OK;
}
catch (libstoff::FileException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: File exception trapped\n"));
  return STOFF_R_FILE_ACCESS_ERROR;
}
catch (libstoff::ParseException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse exception trapped\n"));
  return STOFF_R_PARSE_ERROR;
}
catch (libstoff::WrongPasswordException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse password trapped\n"));
  return STOFF_R_PASSWORD_MISSMATCH_ERROR;
}
catch (...)
{
  //fixme: too generic
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Unknown exception trapped\n"));
  return STOFF_R_UNKNOWN_ERROR;
}

STOFFDocument::Result STOFFDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGSpreadsheetInterface *documentInterface, char const *password)
try
{
  if (!input)
    return STOFF_R_UNKNOWN_ERROR;

  STOFFInputStreamPtr ip(new STOFFInputStream(input, false));
  std::shared_ptr<STOFFHeader> header(STOFFDocumentInternal::getHeader(ip, false));

  if (!header.get()) return STOFF_R_UNKNOWN_ERROR;
  auto parser=STOFFDocumentInternal::getSpreadsheetParserFromHeader(ip, header.get(), password);
  if (!parser) return STOFF_R_UNKNOWN_ERROR;
  parser->parse(documentInterface);
  return STOFF_R_OK;
}
catch (libstoff::FileException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: File exception trapped\n"));
  return STOFF_R_FILE_ACCESS_ERROR;
}
catch (libstoff::ParseException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse exception trapped\n"));
  return STOFF_R_PARSE_ERROR;
}
catch (libstoff::WrongPasswordException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse password trapped\n"));
  return STOFF_R_PASSWORD_MISSMATCH_ERROR;
}
catch (...)
{
  //fixme: too generic
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Unknown exception trapped\n"));
  return STOFF_R_UNKNOWN_ERROR;
}

STOFFDocument::Result STOFFDocument::parse(librevenge::RVNGInputStream *input, librevenge::RVNGTextInterface *documentInterface, char const *password)
try
{
  if (!input)
    return STOFF_R_UNKNOWN_ERROR;

  STOFFInputStreamPtr ip(new STOFFInputStream(input, false));
  std::shared_ptr<STOFFHeader> header(STOFFDocumentInternal::getHeader(ip, false));

  if (!header.get()) return STOFF_R_UNKNOWN_ERROR;
  auto parser=STOFFDocumentInternal::getTextParserFromHeader(ip, header.get(), password);
  if (!parser) return STOFF_R_UNKNOWN_ERROR;
  parser->parse(documentInterface);

  return STOFF_R_OK;
}
catch (libstoff::FileException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: File exception trapped\n"));
  return STOFF_R_FILE_ACCESS_ERROR;
}
catch (libstoff::ParseException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse exception trapped\n"));
  return STOFF_R_PARSE_ERROR;
}
catch (libstoff::WrongPasswordException)
{
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Parse password trapped\n"));
  return STOFF_R_PASSWORD_MISSMATCH_ERROR;
}
catch (...)
{
  //fixme: too generic
  STOFF_DEBUG_MSG(("STOFFDocument::parse: Unknown exception trapped\n"));
  return STOFF_R_UNKNOWN_ERROR;
}

bool STOFFDocument::decodeGraphic(librevenge::RVNGBinaryData const &binary, librevenge::RVNGDrawingInterface *paintInterface)
try
{
  if (!paintInterface || !binary.size()) {
    STOFF_DEBUG_MSG(("STOFFDocument::decodeGraphic: called with no data or no converter\n"));
    return false;
  }
  STOFFGraphicDecoder tmpHandler(paintInterface);
  if (!tmpHandler.checkData(binary) || !tmpHandler.readData(binary)) return false;
  return true;
}
catch (...)
{
  STOFF_DEBUG_MSG(("STOFFDocument::decodeGraphic: unknown error\n"));
  return false;
}

bool STOFFDocument::decodeSpreadsheet(librevenge::RVNGBinaryData const &binary, librevenge::RVNGSpreadsheetInterface *sheetInterface)
try
{
  if (!sheetInterface || !binary.size()) {
    STOFF_DEBUG_MSG(("STOFFDocument::decodeSpreadsheet: called with no data or no converter\n"));
    return false;
  }
  STOFFSpreadsheetDecoder tmpHandler(sheetInterface);
  if (!tmpHandler.checkData(binary) || !tmpHandler.readData(binary)) return false;
  return true;
}
catch (...)
{
  STOFF_DEBUG_MSG(("STOFFDocument::decodeSpreadsheet: unknown error\n"));
  return false;
}

bool STOFFDocument::decodeText(librevenge::RVNGBinaryData const &, librevenge::RVNGTextInterface *)
{
  STOFF_DEBUG_MSG(("STOFFDocument::decodeText: unimplemented\n"));
  return false;
}

namespace STOFFDocumentInternal
{
/** return the header corresponding to an input. Or 0L if no input are found */
STOFFHeader *getHeader(STOFFInputStreamPtr &ip, bool strict)
try
{
  if (!ip.get()) return nullptr;

  if (ip->size() < 10) return nullptr;

  ip->seek(0, librevenge::RVNG_SEEK_SET);
  ip->setReadInverted(false);

  auto listHeaders = STOFFHeader::constructHeader(ip);
  for (auto &h : listHeaders) {
    if (!STOFFDocumentInternal::checkHeader(ip, h, strict))
      continue;
    return new STOFFHeader(h);
  }
  return nullptr;
}
catch (libstoff::FileException)
{
  STOFF_DEBUG_MSG(("STOFFDocumentInternal::STOFFDocument[getHeader]:File exception trapped\n"));
  return nullptr;
}
catch (libstoff::ParseException)
{
  STOFF_DEBUG_MSG(("STOFFDocumentInternal::getHeader:Parse exception trapped\n"));
  return nullptr;
}
catch (...)
{
  //fixme: too generic
  STOFF_DEBUG_MSG(("STOFFDocumentInternal::getHeader:Unknown exception trapped\n"));
  return nullptr;
}

/** Factory wrapper to construct a parser corresponding to an graphic header */
std::shared_ptr<STOFFGraphicParser> getGraphicParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd)
{
  std::shared_ptr<STOFFGraphicParser> parser;
  if (!header || (header->getKind()!=STOFFDocument::STOFF_K_DRAW && header->getKind()!=STOFFDocument::STOFF_K_GRAPHIC))
    return parser;
  try {
    if (header->getKind()==STOFFDocument::STOFF_K_DRAW) {
      SDAParser *sdaParser=new SDAParser(input, header);
      parser.reset(sdaParser);
      if (passwd) sdaParser->setDocumentPassword(passwd);
    }
    else {
      SDGParser *sdgParser=new SDGParser(input, header);
      parser.reset(sdgParser);
      if (passwd) sdgParser->setDocumentPassword(passwd);
    }
  }
  catch (...) {
  }
  return parser;
}

/** Factory wrapper to construct a parser corresponding to an presentation header */
std::shared_ptr<STOFFGraphicParser> getPresentationParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd)
{
  std::shared_ptr<STOFFGraphicParser> parser;
  if (!header || header->getKind()!=STOFFDocument::STOFF_K_PRESENTATION)
    return parser;
  try {
    SDAParser *sdaParser=new SDAParser(input, header);
    parser.reset(sdaParser);
    if (passwd) sdaParser->setDocumentPassword(passwd);
  }
  catch (...) {
  }
  return parser;
}

/** Factory wrapper to construct a parser corresponding to an text header */
std::shared_ptr<STOFFTextParser> getTextParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd)
{
  std::shared_ptr<STOFFTextParser> parser;
  if (!header)
    return parser;
  if (header->getKind()==STOFFDocument::STOFF_K_TEXT) {
    try {
      SDWParser *sdwParser=new SDWParser(input, header);
      parser.reset(sdwParser);
      if (passwd) sdwParser->setDocumentPassword(passwd);
      return parser;
    }
    catch (...) {
    }
  }
#ifdef DEBUG
  if (header->getKind()==STOFFDocument::STOFF_K_SPREADSHEET || header->getKind()==STOFFDocument::STOFF_K_DRAW || header->getKind()==STOFFDocument::STOFF_K_GRAPHIC)
    return parser;
  try {
    SDXParser *sdxParser=new SDXParser(input, header);
    parser.reset(sdxParser);
    if (passwd) sdxParser->setDocumentPassword(passwd);
  }
  catch (...) {
  }
#endif
  return parser;
}

/** Factory wrapper to construct a parser corresponding to an spreadsheet header */
std::shared_ptr<STOFFSpreadsheetParser> getSpreadsheetParserFromHeader(STOFFInputStreamPtr &input, STOFFHeader *header, char const *passwd)
{
  std::shared_ptr<STOFFSpreadsheetParser> parser;
  if (!header || header->getKind()!=STOFFDocument::STOFF_K_SPREADSHEET)
    return parser;
  try {
    SDCParser *sdcParser=new SDCParser(input, header);
    parser.reset(sdcParser);
    if (passwd) sdcParser->setDocumentPassword(passwd);
  }
  catch (...) {
  }
  return parser;
}

/** Wrapper to check a basic header of a mac file */
bool checkHeader(STOFFInputStreamPtr &input, STOFFHeader &header, bool strict)
try
{
  std::shared_ptr<STOFFParser> parser=getTextParserFromHeader(input, &header, nullptr);
  if (!parser) parser=getSpreadsheetParserFromHeader(input, &header, nullptr);
  if (!parser) parser=getGraphicParserFromHeader(input, &header, nullptr);
  if (!parser) return false;
  return parser->checkHeader(&header, strict);
}
catch (...)
{
  STOFF_DEBUG_MSG(("STOFFDocumentInternal::checkHeader:Unknown exception trapped\n"));
  return false;
}

}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
