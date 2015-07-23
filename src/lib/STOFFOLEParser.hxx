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
 *  freely inspired from istorage :
 * ------------------------------------------------------------
 *      Generic OLE Zones furnished with a copy of the file header
 *
 * Compound Storage (32 bit version)
 * Storage implementation
 *
 * This file contains the compound file implementation
 * of the storage interface.
 *
 * Copyright 1999 Francis Beaudet
 * Copyright 1999 Sylvain St-Germain
 * Copyright 1999 Thuy Nguyen
 * Copyright 2005 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * ------------------------------------------------------------
 */

#ifndef STOFF_OLE_PARSER_H
#define STOFF_OLE_PARSER_H

#include <string>
#include <vector>

#include <librevenge-stream/librevenge-stream.h>

#include "libstaroffice_internal.hxx"
#include "STOFFInputStream.hxx"

#include "STOFFDebug.hxx"

namespace STOFFOLEParserInternal
{
class CompObj;
}

/** \brief a class used to parse some basic oles
    Tries to read the different ole parts and stores their contents in form of picture.
 */
class STOFFOLEParser
{
public:
  /** constructor
      \param mainName name of the main ole, we must avoid to parse */
  STOFFOLEParser(std::string mainName);

  /** destructor */
  ~STOFFOLEParser();

  /** tries to parse basic OLE (excepted mainName)
      \return false if fileInput is not an Ole file */
  bool parse(STOFFInputStreamPtr fileInput);

  //! returns the list of unknown ole
  std::vector<std::string> const &getNotParse() const
  {
    return m_unknownOLEs;
  }

  //! returns the list of id for which we have find a representation
  std::vector<int> const &getObjectsId() const
  {
    return m_objectsId;
  }
  //! returns the list of data positions which have been read
  std::vector<STOFFPosition> const &getObjectsPosition() const
  {
    return m_objectsPosition;
  }
  //! returns the list of data which have been read
  std::vector<librevenge::RVNGBinaryData> const &getObjects() const
  {
    return m_objects;
  }
  //! returns the list of data type
  std::vector<std::string> const &getObjectsType() const
  {
    return m_objectsType;
  }

  //! returns the picture corresponding to an id
  bool getObject(int id, librevenge::RVNGBinaryData &obj, STOFFPosition &pos, std::string &type) const;

  /*! \brief sets an object
   * just in case, the external parsing find another representation
   */
  void setObject(int id, librevenge::RVNGBinaryData const &obj, STOFFPosition const &pos,
                 std::string const &type);

protected:
  //!  the "persist elements" small structure: the list of object
  static bool readPersists(STOFFInputStreamPtr input, std::string const &oleName,
                           libstoff::DebugFile &ascii);
  //! the document information
  static bool readDocumentInformation(STOFFInputStreamPtr input, std::string const &oleName,
                                      libstoff::DebugFile &ascii);
  //! the summary information
  static bool readSummaryInformation(STOFFInputStreamPtr input, std::string const &oleName,
                                     libstoff::DebugFile &ascii);
  //! the windows information
  static bool readSfxWindows(STOFFInputStreamPtr input, std::string const &oleName,
                             libstoff::DebugFile &ascii);
  //! the page style
  static bool readSwPageStyleSheets(STOFFInputStreamPtr input, std::string const &oleName,
                                  libstoff::DebugFile &ascii);
  
  //!  the "Ole" small structure : unknown contain
  static bool readOle(STOFFInputStreamPtr ip, std::string const &oleName,
                      libstoff::DebugFile &ascii);
  //!  the "ObjInfo" small structure : seems to contain 3 ints=0,3,4
  static bool readObjInfo(STOFFInputStreamPtr input, std::string const &oleName,
                          libstoff::DebugFile &ascii);
  //!  the "CompObj" contains : UserType,ClipName,ProgIdName
  bool readCompObj(STOFFInputStreamPtr ip, std::string const &oleName,
                   libstoff::DebugFile &ascii);

  /** the OlePres001 seems to contain standart picture file and size */
  static  bool isOlePres(STOFFInputStreamPtr ip, std::string const &oleName);
  /** extracts the picture of OlePres001 if it is possible */
  static bool readOlePres(STOFFInputStreamPtr ip, librevenge::RVNGBinaryData &data, STOFFPosition &pos,
                          libstoff::DebugFile &ascii);

  //! theOle10Native : basic Windows© picture, with no size
  static bool isOle10Native(STOFFInputStreamPtr ip, std::string const &oleName);
  /** extracts the picture if it is possible */
  static bool readOle10Native(STOFFInputStreamPtr ip, librevenge::RVNGBinaryData &data,
                              libstoff::DebugFile &ascii);

  /** \brief the Contents : in general a picture : a PNG, an JPEG, a basic metafile,
   * I find also a Word art picture, which are not sucefull read
   */
  bool readContents(STOFFInputStreamPtr input, std::string const &oleName,
                    librevenge::RVNGBinaryData &pict, STOFFPosition &pos, libstoff::DebugFile &ascii);

  /** the CONTENTS : seems to store a header size, the header
   * and then a object in EMF (with the same header)...
   * \note I only find such lib in 2 files, so the parsing may be incomplete
   *  and many such Ole rejected
   */
  bool readCONTENTS(STOFFInputStreamPtr input, std::string const &oleName,
                    librevenge::RVNGBinaryData &pict, STOFFPosition &pos, libstoff::DebugFile &ascii);


  //! if filled, does not parse content with this name
  std::string m_avoidOLE;
  //! list of ole which can not be parsed
  std::vector<std::string> m_unknownOLEs;

  //! list of pictures read
  std::vector<librevenge::RVNGBinaryData> m_objects;
  //! list of picture size ( if known)
  std::vector<STOFFPosition> m_objectsPosition;
  //! list of pictures id
  std::vector<int> m_objectsId;
  //! list of picture type
  std::vector<std::string> m_objectsType;

  //! a smart ptr used to stored the list of compobj id->name
  shared_ptr<STOFFOLEParserInternal::CompObj> m_compObjIdName;

};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
