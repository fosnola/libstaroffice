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
 * StarEncoding to read/parse some other Korean encoding in StarOffice documents
 *
 */
#ifndef STAR_ENCODING_OTHER_KOREAN
#  define STAR_ENCODING_OTHER_KOREAN

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFInputStream.hxx"

#include "StarEncoding.hxx"

class StarDocument;
class StarZone;

/** \brief the main class to read/.. some other Korean encoding in StarOffice documents
 *
 *
 *
 */
class StarEncodingOtherKorean
{
public:

  //! constructor
  StarEncodingOtherKorean();
  //! destructor
  virtual ~StarEncodingOtherKorean();

  /** try to read a caracter and add it to string: BIG5_HKSCS */
  static bool readKoreanBig5(STOFFInputStreamPtr &input, StarEncoding::Encoding encoding, long endPos,
                             librevenge::RVNGString &string, std::vector<unsigned long> &limits);
  /** try to read a caracter and add it to string: MS_1361 */
  static bool readKoreanMS1361(STOFFInputStreamPtr &input, StarEncoding::Encoding encoding, long endPos,
                               librevenge::RVNGString &string, std::vector<unsigned long> &limits);
protected:
  //
  // data
  //
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
