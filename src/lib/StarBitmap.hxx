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
 * StarBitmap to read/parse some basic bitmap/pattern in StarOffice documents
 *
 */
#ifndef STAR_BITMAP
#  define STAR_BITMAP

#include <vector>

#include "libstaroffice_internal.hxx"
#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

namespace StarBitmapInternal
{
struct Bitmap;
struct State;
}

class StarObject;
class StarZone;

/** \brief the main class to read/.. some basic bitmap/pattern in StarOffice documents
 *
 *
 *
 */
class StarBitmap
{
public:
  //! constructor
  StarBitmap();
  //! constructor for pixmap 32*32
  explicit StarBitmap(uint32_t const((&pixels)[32]), STOFFColor const((&colors)[2]));
  //! destructor
  virtual ~StarBitmap();

  //! low level

  /** try to read a bitmap

   \note only fill data and type if the bitmap has a file header*/
  bool readBitmap(StarZone &zone, bool inFileHeader, long lastPos, librevenge::RVNGBinaryData &data, std::string &type);
  //! try to convert the read data in ppm
  bool getData(librevenge::RVNGBinaryData &data, std::string &type) const;
  //! try to return the bitmap size (in point)
  STOFFVec2i getBitmapSize() const;
protected:
  //! try to read the bitmap information block
  bool readBitmapInformation(StarZone &zone, StarBitmapInternal::Bitmap &info, long lastPos);
  //! try to read the bitmap data block
  bool readBitmapData(STOFFInputStreamPtr &input, StarBitmapInternal::Bitmap &bitmap, long lastPos);
  //
  // data
  //
private:
  //! the state
  std::shared_ptr<StarBitmapInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
