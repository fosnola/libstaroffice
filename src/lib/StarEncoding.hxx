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
 * StarEncoding to read/parse some basic encoding in StarOffice documents
 *
 */
#ifndef STAR_ENCODING
#  define STAR_ENCODING

#include <vector>

#include "libstaroffice_internal.hxx"

/** \brief the main class to read/.. some basic encoding in StarOffice documents
 *
 *
 *
 */
class StarEncoding
{
public:
  //! the different encoding
  enum Encoding {
    E_DONTKNOW=0,E_MS_1252,E_APPLE_ROMAN,E_IBM_437,E_IBM_850,E_IBM_860,E_IBM_861,E_IBM_863,
    E_IBM_865,/*SYSTEM=9,*/E_SYMBOL=10,E_ASCII_US,E_ISO_8859_1,E_ISO_8859_2,E_ISO_8859_3,E_ISO_8859_4,
    E_ISO_8859_5,E_ISO_8859_6,E_ISO_8859_7,E_ISO_8859_8,E_ISO_8859_9,E_ISO_8859_14,E_ISO_8859_15,E_IBM_737,
    E_IBM_775,E_IBM_852,E_IBM_855,E_IBM_857,E_IBM_862,E_IBM_864,E_IBM_866,E_IBM_869,
    E_MS_874,E_MS_1250,E_MS_1251,E_MS_1253,E_MS_1254,E_MS_1255,E_MS_1256,E_MS_1257,
    // 40
    E_MS_1258,/*E_APPLE_ARABIC,*/E_APPLE_CENTEURO=42,E_APPLE_CROATIAN,E_APPLE_CYRILLIC,/*E_APPLE_DEVANAGARI,E_APPLE_FARSI,*/E_APPLE_GREEK=47,
    /*E_APPLE_GUJARATI,E_APPLE_GURMUKHI,E_APPLE_HEBREW,*/E_APPLE_ICELAND=51,E_APPLE_ROMANIAN,/*E_APPLE_THAI,*/E_APPLE_TURKISH=54,E_APPLE_UKRAINIAN,
    E_APPLE_CHINSIMP,E_APPLE_CHINTRAD,E_APPLE_JAPANESE,E_APPLE_KOREAN,E_MS_932,E_MS_936,E_MS_949,E_MS_950,
    E_SHIFT_JIS,E_GB_2312,E_GBT_12345,E_GBK,E_BIG5,E_EUC_JP,E_EUC_CN,/*E_EUC_TW,*/
    /*E_ISO_2022_JP,E_ISO_2022_CN,*/E_KOI8_R=74,E_UTF7,E_UTF8,E_ISO_8859_10,E_ISO_8859_13,E_EUC_KR,
    // 80
    /*E_ISO_2022_KR,*/E_JIS_X_0201=81,E_JIS_X_0208,E_JIS_X_0212,E_MS_1361,/*E_GB_18030,*/E_BIG5_HKSCS=86,E_TIS_620,
    E_KOI8_U,E_ISCII_DEVANAGARI,

    // E_USER_START=0x8000,E_USER_END=0xEFFF,

    E_UCS4=0xFFFE,E_UCS2=0xFFFF
  };

  //! constructor
  StarEncoding();
  //! destructor
  virtual ~StarEncoding();

  //! return an encoding corresponding to an id
  static Encoding getEncodingForId(int id);
  //! try to convert a list of character and transforms it a unicode's list
  static bool convert(std::vector<uint8_t> const &src, Encoding encoding, std::vector<uint32_t> &dest, std::vector<size_t> &srcPositions);

protected:
  /** try to read a character and add it to string

      \note: normally, we only read caracter one by one but sometimes,
      we need to read a complete set of caracters (utf7, ...). limits can be use
      to retrieve the "original" caracters.*/
  static bool read(std::vector<uint8_t> const &src, size_t &pos, Encoding encoding, std::vector<uint32_t> &dest);
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
