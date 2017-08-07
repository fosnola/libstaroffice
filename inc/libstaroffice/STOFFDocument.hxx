/* -*- Mode: C++; c-default-style: "k&r"; indent-tabs-mode: nil; tab-width: 2; c-basic-offset: 2 -*- */

/* libstaroffice
 * Version: MPL 2.0 / LGPLv2.1+
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Major Contributor(s):
 * Copyright (C) 2003-2005 William Lachance (william.lachance@sympatico.ca)
 * Copyright (C) 2003 Marc Maurer (uwog@uwog.net)
 *
 * For minor contributions see the git repository.
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU Lesser General Public License Version 2.1 or later
 * (LGPLv2.1+), in which case the provisions of the LGPLv2.1+ are
 * applicable instead of those above.
 *
 * For further information visit http://libstoff.sourceforge.net
 */


#ifndef STOFFDOCUMENT_HXX
#define STOFFDOCUMENT_HXX

#ifdef _WINDLL
#ifdef BUILD_STOFF
#define STOFFLIB __declspec(dllexport)
#else
#define STOFFLIB __declspec(dllimport)
#endif
#else // !DLL_EXPORT
#ifdef LIBSTAROFFICE_VISIBILITY
#define STOFFLIB __attribute__((visibility("default")))
#else
#define STOFFLIB
#endif
#endif

namespace librevenge
{
class RVNGBinaryData;
class RVNGDrawingInterface;
class RVNGPresentationInterface;
class RVNGSpreadsheetInterface;
class RVNGTextInterface;
class RVNGInputStream;
}

/**
This class provides all the functions an application would need to parse StarOffice documents.
*/
class STOFFDocument
{
public:
  /** an enum which defines if we have confidence that a file is supported */
  enum Confidence {
    STOFF_C_NONE=0/**< not supported */,
    STOFF_C_UNSUPPORTED_ENCRYPTION /** encryption not supported*/,
    STOFF_C_SUPPORTED_ENCRYPTION /** encryption supported */,
    STOFF_C_EXCELLENT /** supported */
  };
  /** an enum to define the kind of document */
  enum Kind {
    STOFF_K_UNKNOWN=0 /**< unknown*/,
    STOFF_K_BITMAP /** bitmap/image */,
    STOFF_K_CHART /** chart */,
    STOFF_K_DATABASE /** database */,
    STOFF_K_DRAW /** vectorized graphic: .sda*/,
    STOFF_K_MATH /** math*/,
    STOFF_K_PRESENTATION /** presentation*/,
    STOFF_K_SPREADSHEET /** spreadsheet: .sdc */,
    STOFF_K_TEXT /** word processing file*/,
    STOFF_K_GRAPHIC /** gallery graphic: .sdg */
  };
  /** an enum which defines the result of the file parsing */
  enum Result {
    STOFF_R_OK=0 /**< conversion ok*/,
    STOFF_R_FILE_ACCESS_ERROR /** problem when accessing file*/,
    STOFF_R_OLE_ERROR /** problem when reading the OLE structure*/,
    STOFF_R_PARSE_ERROR /** problem when parsing the file*/,
    STOFF_R_PASSWORD_MISSMATCH_ERROR /** problem when using the given password*/,
    STOFF_R_UNKNOWN_ERROR /** unknown error*/
  };

  /** Analyzes the content of an input stream to see if it can be parsed
      \param input The input stream
      \param kind The document kind ( filled if the file is supported )
      \return A confidence value which represents the likelyhood that the content from
      the input stream can be parsed */
  static STOFFLIB Confidence isFileFormatSupported(librevenge::RVNGInputStream *input, Kind &kind);

  // ------------------------------------------------------------
  // the different main parsers
  // ------------------------------------------------------------

  /** Parses the input stream content. It will make callbacks to the functions provided by a
     librevenge::RVNGTextInterface class implementation when needed. This is often commonly called the
     'main parsing routine'.
     \param input The input stream
     \param documentInterface A RVNGTextInterface implementation
     \param password The file password

   \note Reserved for future use. Actually, it only returns false */
  static STOFFLIB Result parse(librevenge::RVNGInputStream *input, librevenge::RVNGTextInterface *documentInterface, char const *password=nullptr);

  /** Parses the input stream content. It will make callbacks to the functions provided by a
     librevenge::RVNGDrawingInterface class implementation when needed. This is often commonly called the
     'main parsing routine'.
     \param input The input stream
     \param documentInterface A RVNGDrawingInterface implementation
     \param password The file password

     \note Reserved for future use. Actually, it only returns false. */
  static STOFFLIB Result parse(librevenge::RVNGInputStream *input, librevenge::RVNGDrawingInterface *documentInterface, char const *password=nullptr);

  /** Parses the input stream content. It will make callbacks to the functions provided by a
     librevenge::RVNGPresentationInterface class implementation when needed. This is often commonly called the
     'main parsing routine'.
     \param input The input stream
     \param documentInterface A RVNGPresentationInterface implementation
     \param password The file password

     \note Reserved for future use. Actually, it only returns false. */
  static STOFFLIB Result parse(librevenge::RVNGInputStream *input, librevenge::RVNGPresentationInterface *documentInterface, char const *password=nullptr);

  /** Parses the input stream content. It will make callbacks to the functions provided by a
     librevenge::RVNGSpreadsheetInterface class implementation when needed. This is often commonly called the
     'main parsing routine'.
     \param input The input stream
     \param documentInterface A RVNGSpreadsheetInterface implementation
     \param password The file password

   \note Can only convert some basic documents: retrieving more cells' contents but no formating. */
  static STOFFLIB Result parse(librevenge::RVNGInputStream *input, librevenge::RVNGSpreadsheetInterface *documentInterface, char const *password=nullptr);

  // ------------------------------------------------------------
  // decoders of the embedded zones created by libstoff
  // ------------------------------------------------------------

  /** Parses the graphic contained in the binary data and called documentInterface to reconstruct
    a graphic. The input is normally send to a librevenge::RVNGXXXInterface with mimeType="image/stoff-odg",
    ie. it must correspond to a picture created by the STOFFGraphicEncoder class via
    a STOFFPropertyEncoder.

   \param binary a list of librevenge::RVNGDrawingInterface stored in a documentInterface,
   \param documentInterface the RVNGDrawingInterface which will convert the graphic is some specific format.

   \note Reserved for future use. Actually, it only returns false. */
  static STOFFLIB bool decodeGraphic(librevenge::RVNGBinaryData const &binary, librevenge::RVNGDrawingInterface *documentInterface);

  /** Parses the spreadsheet contained in the binary data and called documentInterface to reconstruct
    a spreadsheet. The input is normally send to a librevenge::RVNGXXXInterface with mimeType="image/stoff-ods",
    ie. it must correspond to a spreadsheet created by the STOFFSpreadsheetInterface class via
    a STOFFPropertyEncoder.

   \param binary a list of librevenge::RVNGSpreadsheetInterface stored in a documentInterface,
   \param documentInterface the RVNGSpreadsheetInterface which will convert the spreadsheet is some specific format.

   \note Reserved for future use. Actually, it only returns false. */
  static STOFFLIB bool decodeSpreadsheet(librevenge::RVNGBinaryData const &binary, librevenge::RVNGSpreadsheetInterface *documentInterface);

  /** Parses the text contained in the binary data and called documentInterface to reconstruct
    a text. The input is normally send to a librevenge::RVNGXXXInterface with mimeType="image/stoff-odt",
    ie. it must correspond to a text created by the STOFFTextInterface class via
    a STOFFPropertyEncoder.

   \param binary a list of librevenge::RVNGTextInterface stored in a documentInterface,
   \param documentInterface the RVNGTextInterface which will convert the text is some specific format.

   \note Reserved for future use. Actually, it only returns false. */
  static STOFFLIB bool decodeText(librevenge::RVNGBinaryData const &binary, librevenge::RVNGTextInterface *documentInterface);
};

#endif /* STOFFDOCUMENT_HXX */
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
