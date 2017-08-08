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
 * StarWriterStruct to read/parse some basic writer structure in StarOffice documents
 *
 */
#ifndef STAR_WRITER_STRUCT
#  define STAR_WRITER_STRUCT

#include <ostream>
#include <vector>

#include "libstaroffice_internal.hxx"

#include "STOFFDebug.hxx"

namespace StarFormatManagerInternal
{
struct FormatDef;
}
class StarAttribute;
class StarObject;
class StarZone;

/** \brief namespace use to keep basic writer structure */
namespace StarWriterStruct
{
/** \brief struct use an attribute and a position
 */
struct Attribute {
public:
  //! constructor
  Attribute()
    : m_attribute()
    , m_position(-1,-1)
  {
  }
  //! destructor
  ~Attribute();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Attribute const &attribute);
  //! try to read a attribute: 'A'
  bool read(StarZone &zone, StarObject &object);
  //! try to read a list of attribute: 'S'
  static bool readList(StarZone &zone, std::vector<Attribute> &attributeList, StarObject &object);
  //! the attribute
  std::shared_ptr<StarAttribute> m_attribute;
  //! the begin/end position
  STOFFVec2i m_position;
};

/** \brief structure to store a bookmark
 */
class Bookmark
{
public:
  //! the constructor
  Bookmark()
    : m_shortName("")
    , m_name("")
    , m_offset(0)
    , m_key(0)
    , m_modifier(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Bookmark const &mark);
  //! try to read a mark
  bool read(StarZone &zone);
  //! try to read a list of bookmark
  static bool readList(StarZone &zone, std::vector<Bookmark> &markList);
  //! the shortname
  librevenge::RVNGString m_shortName;
  //! the name
  librevenge::RVNGString m_name;
  //! the offset
  int m_offset;
  //! the key
  int m_key;
  //! the modifier
  int m_modifier;
  //! the macros names
  librevenge::RVNGString m_macroNames[4];
};

/** \brief structure to store a databaseName in a text zone
 */
struct DatabaseName {
public:
  //! constructor
  DatabaseName()
    : m_sql("")
    , m_dataList()
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, DatabaseName const &databaseName);
  //! try to read a databaseName: 'D'
  bool read(StarZone &zone);
  //! a data of a DatabaseName
  struct Data {
    //! constructor
    Data()
      : m_name("")
      , m_selection(0,0)
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Data const &data)
    {
      o << data.m_name.cstr() << ",";
      if (data.m_selection!=STOFFVec2i(0,0)) o << "select=" << STOFFVec2i(0,0) << ",";
      return o;
    }
    //! the name
    librevenge::RVNGString m_name;
    //! the start/end position
    STOFFVec2i m_selection;
  };
  //! the names: database, table
  librevenge::RVNGString m_names[2];
  //! the SQL string
  librevenge::RVNGString m_sql;
  //! the list of data
  std::vector<Data> m_dataList;
};

/** \brief structure to store a dictionary in a text zone
 */
struct Dictionary {
public:
  //! constructor
  Dictionary()
    : m_dataList()
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Dictionary const &dictionary);
  //! try to read a dictionary: 'j'
  bool read(StarZone &zone);
  //! a data of a Dictionary
  struct Data {
    //! constructor
    Data()
      : m_name("")
      , m_language(0)
      , m_id(0)
      , m_spellWrong(true)
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Data const &data)
    {
      o << data.m_name.cstr() << ",";
      if (data.m_language) o << "language=" << data.m_language << ",";
      if (data.m_id) o << "id=" << data.m_id << ",";
      if (data.m_spellWrong) o << "spellWrong,";
      return o;
    }
    //! the name
    librevenge::RVNGString m_name;
    //! the language
    int m_language;
    //! the id
    int m_id;
    //! a flag to know if we do spell or not
    bool m_spellWrong;
  };
  //! the list of data
  std::vector<Data> m_dataList;
};

/** \brief the doc statistic
 */
struct DocStats {
public:
  //! constructor
  DocStats()
    : m_isModified(false)
  {
    for (long &number : m_numbers) number=0;
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, DocStats const &docStats);
  //! try to read a docStats: 'd'
  bool read(StarZone &zone);
  //! the list of number: tbl, graf, ole, page, para, word, char
  long m_numbers[7];
  //! modified flags
  bool m_isModified;
};

/** \brief structure to store a macro in a text zone
 */
struct Macro {
public:
  //! constructor
  Macro()
    : m_key(0)
    , m_scriptType(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Macro const &macro);
  //! try to read a macro: 'm'
  bool read(StarZone &zone);
  //! try to read a list of macro: 'M'
  static bool readList(StarZone &zone, std::vector<Macro> &macroLis);
  //! the key
  int m_key;
  //! the names
  librevenge::RVNGString m_names[2];
  //! the scriptType
  int m_scriptType;
};

/** \brief structure to store a mark in a text zone
 */
struct Mark {
public:
  //! constructor
  Mark()
    : m_type(-1)
    , m_id(-1)
    , m_offset(-1)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Mark const &mark);
  //! try to read a mark
  bool read(StarZone &zone);
  //! the type:  2: bookmark-start, 3:bookmark-end
  int m_type;
  //! the id
  int m_id;
  //! the offset
  int m_offset;
};

/** \brief structure to store a nodeRedline in a text zone
 */
struct NodeRedline {
public:
  //! constructor
  NodeRedline()
    : m_id(-1)
    , m_offset(-1)
    , m_flags(0)
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, NodeRedline const &nodeRedline);
  //! try to read a nodeRedline
  bool read(StarZone &zone);
  //! the id
  int m_id;
  //! the offset
  int m_offset;
  //! the flags
  int m_flags;
};

/** \brief structure to store a endnoteInfo or a footnoteInfo in a text zone
 */
struct NoteInfo {
public:
  //! constructor
  explicit NoteInfo(bool isFootnote)
    : m_isFootnote(isFootnote)
    , m_type(0)
    , m_ftnOffset(0)
    , m_posType(0)
    , m_numType(0)
  {
    for (int &i : m_idx) i=0xFFFF;
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, NoteInfo const &noteInfo);
  //! try to read a noteInfo
  bool read(StarZone &zone);
  //! a flag to know if this corresponds to a footnote or a endnote
  bool m_isFootnote;
  //! the type
  int m_type;
  //! the list of idx: the page, the coll, the char and the anchorChar
  int m_idx[4];
  //! the ftnOffset
  int m_ftnOffset;
  //! the strings: prefix, suffix, quoValis, ergoSum
  librevenge::RVNGString m_strings[4];
  //! the posType
  int m_posType;
  //! the numType
  int m_numType;
};

/** \brief the doc statistic
 */
struct PrintData {
public:
  //! constructor
  PrintData()
    : m_flags(0)
    , m_colRow(1,1)
  {
    for (int &spacing : m_spacings) spacing=0;
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, PrintData const &printData);
  //! try to read a printData: '8'
  bool read(StarZone &zone);
  //! the flags
  int m_flags;
  //! the row, col dim
  STOFFVec2i m_colRow;
  //! the spaces: left, right, top, bottom, horizontal, verticals
  int m_spacings[6];
};

/** \brief structure to store a redline in a text zone
 */
struct Redline {
public:
  //! constructor
  Redline()
    : m_type(0)
    , m_stringId(0)
    , m_date(0)
    , m_time(0)
    , m_comment()
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Redline const &redline);
  //! try to read a redline : D
  bool read(StarZone &zone);
  //! try to read a list of redline : R
  static bool readList(StarZone &zone, std::vector<Redline> &redlineList);
  //! try to read a list of list of redline : V
  static bool readListList(StarZone &zone, std::vector<std::vector<Redline> > &redlineListList);
  //! the type
  int m_type;
  //! the stringId
  int m_stringId;
  //! the date
  long m_date;
  //! the time
  long m_time;
  //! the comment
  librevenge::RVNGString m_comment;
};

/** \brief structure to store a TOX in a text zone
 */
struct TOX {
public:
  //! constructor
  TOX()
    : m_type(0)
    , m_createType(0)
    , m_captionDisplay(0)
    , m_styleId(0xFFFF)
    , m_data(0)
    , m_formFlags(0)
    , m_title("")
    , m_name("")
    , m_OLEOptions(0)
    , m_stringIdList()
    , m_styleList()
    , m_titleLength()
    , m_formatList()
  {
    for (int &stringId : m_stringIds) stringId=0xFFFF;
  }
  //! destructor
  ~TOX();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, TOX const &tox);
  //! try to read a TOX
  bool read(StarZone &zone, StarObject &object);
  //! try to read a list of TOX
  static bool readList(StarZone &zone, std::vector<TOX> &toxList, StarObject &object);

  //! a style
  struct Style {
    //! constructor
    Style()
      : m_level(0)
      , m_names()
    {
    }
    //! operator<<
    friend std::ostream &operator<<(std::ostream &o, Style const &style)
    {
      o << "level=" << style.m_level << ",";
      if (!style.m_names.empty()) {
        o << "names=[";
        for (auto const &name : style.m_names) o << name.cstr() << ",";
        o << "],";
      }
      return o;
    }
    //! the level
    int m_level;
    //! the list of names
    std::vector<librevenge::RVNGString> m_names;
  };
  //! the type
  int m_type;
  //! the createType
  int m_createType;
  //! the captionDisplay
  int m_captionDisplay;
  //! the string id, the seq string id, the sect string id
  int m_stringIds[3];
  //! the style id
  int m_styleId;
  //! the number of data?
  int m_data;
  //! the format flags?
  int m_formFlags;
  //! the title
  librevenge::RVNGString m_title;
  //! the name
  librevenge::RVNGString m_name;
  //! the ole options
  int m_OLEOptions;
  //! a list of template string ids
  std::vector<int> m_stringIdList;
  //! a list of style names?
  std::vector<Style> m_styleList;
  //! the title length
  long m_titleLength;
  //! the format
  std::vector<std::shared_ptr<StarFormatManagerInternal::FormatDef> > m_formatList;
};

/** \brief structure to store a TOX51 in a text zone
 */
struct TOX51 {
public:
  //! constructor
  TOX51()
    : m_typeName("")
    , m_type(0)
    , m_createType(0)
    , m_firstTabPos(0)
    , m_title("")
    , m_patternList()
    , m_stringIdList()
    , m_infLevel(0)
  {
  }
  //! destructor
  ~TOX51();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, TOX51 const &tox);
  //! try to read a TOX51
  bool read(StarZone &zone, StarObject &object);
  //! try to read a list of TOX51
  static bool readList(StarZone &zone, std::vector<TOX51> &toxList, StarObject &object);

  //! the typeName
  librevenge::RVNGString m_typeName;
  //! the type
  int m_type;
  //! the createType
  int m_createType;
  //! the firstTabPos
  int m_firstTabPos;
  //! the title
  librevenge::RVNGString m_title;
  //! the pattern list
  std::vector<librevenge::RVNGString> m_patternList;
  //! a list of template string ids
  std::vector<int> m_stringIdList;
  //! the inf level
  int m_infLevel;
};

}
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
