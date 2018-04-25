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

#include <libstaroffice/STOFFDocument.hxx>

#include "libstaroffice_internal.hxx"
#include "STOFFPosition.hxx"

#include "STOFFDebug.hxx"

namespace STOFFOLEParserInternal
{
struct State;
}

/** \brief a class used to parse some basic oles
    Tries to read the different ole parts and stores their contents in form of picture.
 */
class STOFFOLEParser
{
public:
  struct OleDirectory;

  /** constructor */
  STOFFOLEParser();

  /** destructor */
  ~STOFFOLEParser();

  /** tries to parse basic OLE (excepted mainName)
      \return false if fileInput is not an Ole file */
  bool parse(STOFFInputStreamPtr fileInput);

  //! returns the list of directory ole
  std::vector<std::shared_ptr<OleDirectory> > &getDirectoryList();
  //! returns a OleDirectory corresponding to a dir if found
  std::shared_ptr<OleDirectory> getDirectory(std::string const &dir);
  //! returns the main compobj program name
  bool getCompObjName(STOFFInputStreamPtr fileInput, std::string &programName);

  /** structure use to store an object content */
  struct OleContent {
    //! constructor
    OleContent(std::string const &dir, std::string const &base)
      : m_dir(dir)
      , m_base(base)
      , m_isParsed(false)
      , m_position()
      , m_imageData()
      , m_imageType("")
    {
    }
    //! returns the base name
    std::string getBaseName()
    {
      return m_base;
    }
    //! returns the ole name
    std::string getOleName() const
    {
      if (m_dir.empty()) return m_base;
      return m_dir+"/"+m_base;
    }
    //! returns true if the object if parsed
    bool isParsed() const
    {
      return m_isParsed;
    }
    //! sets the parsed flag
    void setParsed(bool flag=true)
    {
      m_isParsed=flag;
    }
    //! return the image position
    STOFFPosition const &getPosition() const
    {
      return m_position;
    }
    //! set the image position
    void setPosition(STOFFPosition const &pos)
    {
      m_position=pos;
    }
    //! returns the image data
    bool getImageData(librevenge::RVNGBinaryData &data, std::string &type) const
    {
      if (m_imageData.empty()) {
        data.clear();
        type="";
        return false;
      }
      data=m_imageData;
      type=m_imageType;
      return true;
    }
    //! sets the image data
    void setImageData(librevenge::RVNGBinaryData const &data, std::string const &type)
    {
      m_imageData=data;
      m_imageType=type;
    }
  protected:
    //! the directory
    std::string m_dir;
    //! the base name
    std::string m_base;
    //! true if the data has been parsed
    bool m_isParsed;
    //! the image position (if known)
    STOFFPosition m_position;
    //! the image content ( if known )
    librevenge::RVNGBinaryData m_imageData;
    //! the image type ( if known)
    std::string m_imageType;
  };

  /** Internal: internal method to keep ole directory and their content */
  struct OleDirectory {
    //! constructor
    OleDirectory(STOFFInputStreamPtr &input, std::string const &dir)
      : m_input(input)
      , m_dir(dir)
      , m_contentList()
      , m_kind(STOFFDocument::STOFF_K_UNKNOWN)
      , m_hasCompObj(false)
      , m_clsName("")
      , m_clipName("")
      , m_parsed(false)
      , m_inUse(false) { }
    //! add a new base file
    void addNewBase(std::string const &base)
    {
      if (base=="CompObj")
        m_hasCompObj=true;
      else
        m_contentList.push_back(OleContent(m_dir,base));
    }
    //! returns the list of unknown ole
    std::vector<std::string> getUnparsedOles() const
    {
      std::vector<std::string> res;
      for (auto const &c : m_contentList) {
        if (c.isParsed()) continue;
        res.push_back(c.getOleName());
      }
      return res;
    }
    //! the main input
    STOFFInputStreamPtr m_input;
    /**the dir name*/
    std::string m_dir;
    /**the list of base name*/
    std::vector<OleContent> m_contentList;
    //! the ole kind
    STOFFDocument::Kind m_kind;
    /** true if the directory contains a compobj object */
    bool m_hasCompObj;
    /** the compobj CLSname */
    std::string m_clsName;
    /** the compobj clipname */
    std::string m_clipName;
    /** a flag to know if the directory is parsed or not */
    bool m_parsed;
    /** a flag to know if the directory is currently used */
    mutable bool m_inUse;
  };

protected:
  //! the summary information
  static bool readSummaryInformation(STOFFInputStreamPtr input, std::string const &oleName,
                                     libstoff::DebugFile &ascii);
  //!  parse the "CompObj" contains : UserType,ClipName,ProgIdName
  bool readCompObj(STOFFInputStreamPtr ip, OleDirectory &directory);
  //!  the "Ole" small structure : unknown contain
  static bool readOle(STOFFInputStreamPtr ip, std::string const &oleName,
                      libstoff::DebugFile &ascii);
  //!  the "ObjInfo" small structure : seems to contain 3 ints=0,3,4
  static bool readObjInfo(STOFFInputStreamPtr input, std::string const &oleName,
                          libstoff::DebugFile &ascii);

  /** the OlePres001 seems to contain standart picture file and size */
  static  bool isOlePres(STOFFInputStreamPtr ip, std::string const &oleName);
  /** extracts the picture of OlePres001 if it is possible.

   \note it only sets the image size with setNaturalSize.*/
  static bool readOlePres(STOFFInputStreamPtr ip, OleContent &content);

  //! theOle10Native : basic Windows© picture, with no size
  static bool isOle10Native(STOFFInputStreamPtr ip, std::string const &oleName);
  /** extracts the picture if it is possible.

   \note it does not set the image position*/
  static bool readOle10Native(STOFFInputStreamPtr ip, OleContent &content);

  /** \brief the Contents : in general a picture : a PNG, an JPEG, a basic metafile,
   * I find also a Word art picture, which are not sucefull read
   */
  bool readContents(STOFFInputStreamPtr input, OleContent &content);

  /** the CONTENTS : seems to store a header size, the header
   * and then a object in EMF (with the same header)...
   * \note I only find such lib in 2 files, so the parsing may be incomplete
   *  and many such Ole rejected
   */
  bool readCONTENTS(STOFFInputStreamPtr input, OleContent &content);

protected:
  //
  // data
  //

  //! the class state
  std::shared_ptr<STOFFOLEParserInternal::State> m_state;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
