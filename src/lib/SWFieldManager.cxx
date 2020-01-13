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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include <librevenge/librevenge.h>

#include "SWFieldManager.hxx"

#include "StarObject.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "STOFFListener.hxx"
#include "STOFFSubDocument.hxx"

/** Internal: the structures of a SWFieldManager */
namespace SWFieldManagerInternal
{
static void updateDatabaseName(librevenge::RVNGString const &name, librevenge::RVNGPropertyList &pList)
{
  librevenge::RVNGString delim, dbName, tableName;
  libstoff::appendUnicode(0xff, delim);
  libstoff::splitString(name,delim, dbName, tableName);
  if (tableName.empty()) {
    if (!dbName.empty())
      pList.insert("text:table-name", dbName);
  }
  else {
    pList.insert("text:table-name", tableName);
    if (!dbName.empty())
      pList.insert("text:database-name", dbName);
  }
  // checkme: always table or is it a function of subtype
  pList.insert("text:table-type", "table");
}

Field::~Field()
{
}

void Field::print(std::ostream &o) const
{
  if (m_type>=0 && m_type<40) {
    char const *wh[]= {"db", "user", "filename", "dbName",
                       "inDate40", "inTime40", "pageNumber", "author",
                       "chapter", "docStat", "getExp", "setExp",
                       "getRef", "hiddenText", "postIt", "fixDate",
                       "fixTime", "reg", "varReg", "setRef", // checkme: 17-19
                       "input", "macro", "dde", "tbl",
                       "hiddenPara", "docInfo", "templName", "dbNextSet", // checkme: 27
                       "dbNumSet", "dbSetNumber", "extUser", "pageSet", // checkme: 29,31
                       "pageGet", "INet", "jumpEdit", "script", // checkme: 32-33,35
                       "dateTime", "authority", "combinedChar", "dropDown" // checkme: 36-39
                      };
    o << wh[m_type];
    if (m_subType>=0) o << "[" << m_subType << "]";
    o << ",";
  }
  else if (m_type>=0)
    o << "###field[type]=" << m_type << ",";
  if (m_format>=0)
    o << "format=" << m_format << ",";
  if (!m_name.empty())
    o << "name=" << m_name.cstr() << ",";
  if (!m_content.empty())
    o << "content=" << m_content.cstr() << ",";
  if (!m_textValue.empty())
    o << "val=" << m_textValue.cstr() << ",";
  else if (m_doubleValue<0||m_doubleValue>0)
    o << "val=" << m_doubleValue << ",";
  if (m_level)
    o << "level=" << m_level << ",";
}

bool Field::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: can not find the listener\n"));
    return false;
  }
  STOFFField field;
  if (m_type==1) {
    if (m_name.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: can not find the user name\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:user-defined");
    field.m_propertyList.insert("text:name", m_name);
    if (!m_content.empty())
      field.m_propertyList.insert("office:string-value", m_content);
    else if (!m_textValue.empty())
      field.m_propertyList.insert("office:string-value", m_textValue);
    else if (m_doubleValue<0 || m_doubleValue>0)
      field.m_propertyList.insert("office:value", m_doubleValue, librevenge::RVNG_GENERIC);
  }
  else if (m_type==2) {
    field.m_propertyList.insert("librevenge:field-type", "text:file-name");
    if (m_format>=0 && m_format<=5) {
      char const *wh[]= {"name-and-extension", "full", "path", "name", "name"/*uiname*/, "area" /* range*/ };
      field.m_propertyList.insert("text:display", wh[m_format]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: unknown filename type=%d\n", m_format));
    }
  }
  else if (m_type==3) {
    if (m_name.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: can not find the dbName\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:database-name");
    updateDatabaseName(m_name, field.m_propertyList);
    field.m_propertyList.insert("librevenge:field-content", m_name);
  }
  else if (m_type==7)
    field.m_propertyList.insert("librevenge:field-type", "text:author-name");
  else if (m_type==8) {
    field.m_propertyList.insert("librevenge:field-type", "text:chapter");
    if (m_format>=0 && m_format<=2) {
      char const *wh[]= {"number", "name", "number-and-name"};
      field.m_propertyList.insert("text:display", wh[m_format]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: unknown chapter type=%d\n", m_format));
    }
    if (m_level>=0)
      field.m_propertyList.insert("text:outline-level", m_level+1);
  }
  else if (m_type==9) {
    if (m_subType>=0 && m_subType<=6) {
      char const *wh[]= {
        "text:page-count", "text:paragraph-count", "text:word-count", "text:character-count", "text:table-count",
        "text:image-count","text:object-count"
      };
      field.m_propertyList.insert("librevenge:field-type", wh[m_subType]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: unknown doc type=%d\n", m_subType));
      return false;
    }
  }
  else if (m_type==10) {
    if (m_name.empty() || m_content.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: can not find the expression values\n"));
      return false;
    }
    if (m_subType&0x10) {
      field.m_propertyList.insert("librevenge:field-type", "text:expression");
      field.m_propertyList.insert("text:formula", m_name);
      field.m_propertyList.insert("office:string-value", m_content);
    }
    else {
      field.m_propertyList.insert("librevenge:field-type", "text:variable-get");
      field.m_propertyList.insert("text:name", m_name);
    }
    if (!m_content.empty())
      field.m_propertyList.insert("librevenge:field-content", m_content);
  }
  else if (m_type==12) {
    if (m_name.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: can not find the getRef values\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:reference-ref");
    field.m_propertyList.insert("text:ref-name", m_name);
    if (m_format>=0 && m_format<=10) {
      char const *wh[]= {"page", "chapter", "text", "direction",
                         "text"/* as page style*/, "category-and-value", "caption", "number",
                         "number"/* new ref*/, "number"/*no context*/, "number"/*full context*/
                        };
      field.m_propertyList.insert("text:reference-format",wh[m_format]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: unknown getRef format=%d\n", m_format));
    }
    if (!m_content.empty())
      field.m_propertyList.insert("librevenge:field-content", m_content);
  }
  else if (m_type==23) {
    if (m_name.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: can not find the expression values\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:expression");
    field.m_propertyList.insert("text:formula", m_name);
    if (!m_content.empty()) {
      field.m_propertyList.insert("office:string-value", m_content);
      field.m_propertyList.insert("librevenge:field-content", m_content);
    }
  }
  else if (m_type==25) {
    int subType=m_subType&0x7FF;
    if (subType>=4 && subType<8) {
      field.m_propertyList.insert("librevenge:field-type", "text:user-defined");
      field.m_propertyList.insert("text:name", state.m_global->m_object.getUserNameMetaData(subType-4));
    }
    else if (subType==9) {
      if (m_format>=0 && m_format<=2) {
        char const *wh[]= {
          "text:creator", "text:modification-time", "text:modification-date"
        };
        field.m_propertyList.insert("librevenge:field-type", wh[m_format]);
      }
      else {
        STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: sending custom type %d is not implemented\n", m_format));
      }
    }
    else if (subType==10) {
      if (m_format>=0 && m_format<=2) {
        char const *wh[]= {
          "text:printed-by", "text:print-time", "text:print-date"
        };
        field.m_propertyList.insert("librevenge:field-type", wh[m_format]);
      }
      else {
        STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: sending custom type %d is not implemented\n", m_format));
      }
    }
    else if (subType>=0 && subType<=12) {
      char const *wh[]= {
        "text:title", "text:subject", "text:keywords", "text:description",
        "text:Info1", "text:Info2", "text:Info3", "text:Info4",
        "text:editing-cycles", "", "",  "text:editing-cycles",
        "text:editing-duration"
      };
      field.m_propertyList.insert("librevenge:field-type", wh[subType]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: sending docInfo %d is not implemented\n", subType));
      return false;
    }
  }
  else if (m_type==20) {
    field.m_propertyList.insert("librevenge:field-type", "text:text-input");
    field.m_propertyList.insert("text:description", m_name);
    field.m_propertyList.insert("librevenge:field-content", m_content);
  }
  else if (m_type==21) {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: sending macros is not implemented\n"));
      first=false;
    }
    return true;
  }
  else if (m_type==26) {
    int subType=m_format&0x7FF;
    if (subType>=0 && subType<=5) {
      char const *wh[]= {"name-and-extension", "full", "path", "name", "title"/*uiname*/, "area" /* range*/ };
      field.m_propertyList.insert("librevenge:field-type", "text:template-name");
      field.m_propertyList.insert("text:display", wh[subType]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: unknown template type=%d\n", m_format));
      return false;
    }
  }
  else if (m_type==30) {
    if (m_subType>=0 && m_subType<=16) {
      char const *wh[]= {
        "text:sender-company", "text:sender-firstname", "text:sender-lastname", "text:sender-initials", "text:sender-street",
        "text:sender-country", "text:sender-postal-code", "text:sender-city", "text:sender-title", "text:sender-position",
        "text:sender-phone-private", "text:sender-phone-work", "text:sender-fax", "text:sender-email", "text:sender-state-or-province",
        "text:sender-lastname" /*father name*/, "text:sender-street" /* appartement*/
      };
      field.m_propertyList.insert("librevenge:field-type", wh[m_subType]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: unknown extUser type=%d\n", m_subType));
      return false;
    }
  }
  else {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::Field::send: sending type=%d is not implemented\n", m_type));
    return false;
  }
  listener->insertField(field);
  return true;
}

//! Internal: a fixed date time field
struct FieldDateTime final : public Field {
  //! constructor
  FieldDateTime() : Field(), m_dateTime(0), m_time(0), m_offset(0)
  {
  }
  //! copy constructor
  explicit FieldDateTime(Field const &orig) : Field(orig), m_dateTime(0), m_time(0), m_offset(0)
  {
  }
  //! destructor
  ~FieldDateTime() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (m_dateTime) o << "date/time=" << m_dateTime << ",";
    if (m_offset) o << "offset=" << m_offset << ",";
  }
  //! the dateTime
  long m_dateTime;
  //! the time
  long m_time;
  //! the offset
  long m_offset;
};

FieldDateTime::~FieldDateTime()
{
}

bool FieldDateTime::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldDateTime::send: can not find the listener\n"));
    return false;
  }
  STOFFField field;

  if (m_type==4 || m_type==36)
    field.m_propertyList.insert("librevenge:field-type", "text:date");
  else if (m_type==5)
    field.m_propertyList.insert("librevenge:field-type", "text:time");
  else if (m_type==15) {
    field.m_propertyList.insert("librevenge:field-type", "text:date");
    field.m_propertyList.insert("text:fixed", true);
    if (m_dateTime) {
      field.m_propertyList.insert("librevenge:year", int(m_dateTime/10000));
      field.m_propertyList.insert("librevenge:month", int((m_dateTime/100)%100));
      field.m_propertyList.insert("librevenge:day", int(m_dateTime%100));
    }
  }
  else if (m_type==16) {
    // FIXME: does not works because libodfgen does not regenerate the text zone...
    field.m_propertyList.insert("librevenge:field-type", "text:time");
    field.m_propertyList.insert("text:fixed", true);
    if (m_dateTime) {
      field.m_propertyList.insert("librevenge:hours", int(m_dateTime/1000000));
      field.m_propertyList.insert("librevenge:minutes", int((m_dateTime/10000)%100));
      field.m_propertyList.insert("librevenge:seconds", int((m_dateTime/100)%100));
    }
  }
  else
    return Field::send(listener, state);
  //TODO: set the format
  listener->insertField(field);
  return true;
}

//! Internal: a DB field field
struct FieldDBField final : public Field {
  //! constructor
  FieldDBField()
    : Field()
    , m_dbName("")
    , m_colName("")
    , m_longNumber(0)
  {
  }
  //! copy constructor
  explicit FieldDBField(Field const &orig) : Field(orig), m_dbName(""), m_colName(""), m_longNumber(0)
  {
  }
  //! destructor
  ~FieldDBField() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_dbName.empty()) o << "dbName=" << m_dbName.cstr() << ",";
    if (!m_colName.empty()) o << "colName=" << m_colName.cstr() << ",";
    else if (m_longNumber) o << "number=" << m_longNumber << ",";
  }
  //! the dbName
  librevenge::RVNGString m_dbName;
  //! the column name
  librevenge::RVNGString m_colName;
  //! the number as num
  long m_longNumber;
};

FieldDBField::~FieldDBField()
{
}

bool FieldDBField::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldDBField::send: can not find the listener\n"));
    return false;
  }
  STOFFField field;

  if (m_type==0) {
    if (m_colName.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldDBField::send: can not find the col value\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:database-display");
    if (!m_dbName.empty())
      updateDatabaseName(m_dbName, field.m_propertyList);
    field.m_propertyList.insert("text:column-name", m_colName);
  }
  else
    return Field::send(listener, state);
  //TODO: set the format
  listener->insertField(field);
  return true;
}

//! Internal: a hidden text/para field
struct FieldHiddenText final : public Field {
  //! constructor
  FieldHiddenText()
    : Field()
    , m_hidden(true)
    , m_condition("")
  {
  }
  //! copy constructor
  explicit FieldHiddenText(Field const &orig) : Field(orig), m_hidden(true), m_condition("")
  {
  }
  //! destructor
  ~FieldHiddenText() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_condition.empty()) o << "condition=" << m_condition.cstr() << ",";
    if (!m_hidden) o << "hidden=false,";
  }
  //! the hidden flag
  bool m_hidden;
  //! the condition
  librevenge::RVNGString m_condition;
};

FieldHiddenText::~FieldHiddenText()
{
}

bool FieldHiddenText::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldHiddenText::send: can not find the listener\n"));
    return false;
  }
  STOFFField field;
  if (m_type==13) {
    if (m_condition.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldHiddenText::send: can not find the condition\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:conditional-text");
    field.m_propertyList.insert("text:condition", m_condition);
    if (!m_content.empty()) {
      librevenge::RVNGString trueValue, falseValue;
      libstoff::splitString(m_content, "|", trueValue, falseValue);
      if (!trueValue.empty())
        field.m_propertyList.insert("text:string-value-if-true", trueValue);
      if (!falseValue.empty())
        field.m_propertyList.insert("text:string-value-if-false", falseValue);
    }
  }
  else if (m_type==24) {
    if (m_condition.empty()) {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldHiddenText::send: can not find the condition\n"));
      return false;
    }
    field.m_propertyList.insert("librevenge:field-type", "text:hidden-paragraph");
    field.m_propertyList.insert("text:condition", m_condition);
    field.m_propertyList.insert("text:is-hidden", m_hidden);
  }
  else // also ....
    return Field::send(listener, state);
  //TODO: set the format
  listener->insertField(field);
  return true;
}

//! Internal: a set field field
struct FieldINet final : public Field {
  //! constructor
  FieldINet()
    : Field()
    , m_url("")
    , m_target("")
    , m_libNames()
  {
  }
  //! copy constructor
  explicit FieldINet(Field const &orig)
    : Field(orig)
    , m_url("")
    , m_target("")
    , m_libNames()
  {
  }
  //! destructor
  ~FieldINet() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_url.empty()) o << "url=" << m_url.cstr() << ",";
    if (!m_target.empty()) o << "target=" << m_target.cstr() << ",";
    if (!m_libNames.empty()) {
      o << "libNames=[";
      for (size_t i=0; i+1<m_libNames.size(); i+=2)
        o << m_libNames[i].cstr() << ":" <<  m_libNames[i+1].cstr() << ",";
      o << "],";
    }
  }
  //! the url
  librevenge::RVNGString m_url;
  //! the target
  librevenge::RVNGString m_target;
  //! the lib names
  std::vector<librevenge::RVNGString> m_libNames;
};

FieldINet::~FieldINet()
{
}

bool FieldINet::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldINet::send: can not find the listener\n"));
    return false;
  }
  if (m_type!=33)
    return Field::send(listener, state);
  if (m_url.empty()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldINet::send: the url is empty\n"));
    return false;
  }
  STOFFLink link;
  link.m_HRef=m_url.cstr();
  listener->openLink(link);
  if (!m_target.empty())
    listener->insertUnicodeString(m_target);
  else {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldINet::send: can not find any representation\n"));
  }
  listener->closeLink();
  return true;
}

//! Internal: a jump edit field
struct FieldJumpEdit final : public Field {
  //! constructor
  FieldJumpEdit()
    : Field()
    , m_help("")
  {
  }
  //! copy constructor
  explicit FieldJumpEdit(Field const &orig)
    : Field(orig)
    , m_help("")
  {
  }
  //! destructor
  ~FieldJumpEdit() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_help.empty()) o << "help=" << m_help.cstr() << ",";
  }
  //! the help
  librevenge::RVNGString m_help;
};

FieldJumpEdit::~FieldJumpEdit()
{
}

bool FieldJumpEdit::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldJumpEdit::send: can not find the listener\n"));
    return false;
  }
  STOFFField field;
  if (m_type==34) {
    field.m_propertyList.insert("librevenge:field-type", "text:placeholder");
    field.m_propertyList.insert("librevenge:field-content", m_content);
    if (m_format>=0 && m_format<=4) {
      char const *wh[]= {"text", "table", "text-box", "image", "object"};
      field.m_propertyList.insert("text:placeholder-type",wh[m_format]);
    }
    else {
      STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldJumpEdit::send: unknown format=%d\n", m_format));
    }
    if (!m_help.empty())
      field.m_propertyList.insert("text:description", m_help);
  }
  else
    return Field::send(listener, state);
  listener->insertField(field);
  return true;
}

//! Internal: a pageNumber field
struct FieldPageNumber final : public Field {
  //! constructor
  FieldPageNumber()
    : Field()
    , m_userString("")
    , m_offset(0)
    , m_isOn(true)
  {
  }
  //! copy constructor
  explicit FieldPageNumber(Field const &orig) : Field(orig), m_userString(""), m_offset(0), m_isOn(true)
  {
  }
  //! destructor
  ~FieldPageNumber() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_userString.empty())
      o << "userString=" << m_userString.cstr() << ",";
    if (m_offset) o << "offset=" << m_offset << ",";
    if (!m_isOn) o << "off,";
  }
  //! the userString
  librevenge::RVNGString m_userString;
  //! the offset
  int m_offset;
  //! a flag to know if isOn
  bool m_isOn;
};

FieldPageNumber::~FieldPageNumber()
{
}

bool FieldPageNumber::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldPageNumber::send: can not find the listener\n"));
    return false;
  }
  STOFFField field;
  if (m_type==6) {
    field.m_propertyList.insert("librevenge:field-type", "text:page-number");
    if (m_offset<0)
      field.m_propertyList.insert("text:select-page", "previous");
    else if (m_offset>0)
      field.m_propertyList.insert("text:select-page", "next");
  }
  else // also 31 which is setPageRef
    return Field::send(listener, state);
  //TODO: set the format
  listener->insertField(field);
  return true;
}

//! Internal: a postit field
struct FieldPostIt final : public Field {
  //! constructor
  FieldPostIt()
    : Field()
    , m_author("")
    , m_date(0)
  {
  }
  //! copy constructor
  explicit FieldPostIt(Field const &orig)
    : Field(orig)
    , m_author("")
    , m_date(0)
  {
  }
  //! destructor
  ~FieldPostIt() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_author.empty()) o << "author=" << m_author.cstr() << ",";
    if (m_date) o << "date=" << m_date << ",";
  }
  //! the author
  librevenge::RVNGString m_author;
  //! the date
  long m_date;
};

FieldPostIt::~FieldPostIt()
{
}

//! Internal: a script field
struct FieldScript final : public Field {
  //! constructor
  FieldScript()
    : Field()
    , m_code("")
    , m_scriptType("")
  {
  }
  //! copy constructor
  explicit FieldScript(Field const &orig)
    : Field(orig)
    , m_code("")
    , m_scriptType("")
  {
  }
  //! destructor
  ~FieldScript() final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_code.empty()) o << "code=" << m_code.cstr() << ",";
    if (!m_scriptType.empty()) o << "script[type]=" << m_scriptType.cstr() << ",";
  }
  //! the code
  librevenge::RVNGString m_code;
  //! the scriptType
  librevenge::RVNGString m_scriptType;
};

FieldScript::~FieldScript()
{
}

//! Internal: a set expr field
struct FieldSetExp final : public Field {
  //! constructor
  FieldSetExp()
    : Field()
    , m_fieldType(0)
    , m_formula("")
    , m_prompt("")
    , m_seqVal(0)
    , m_seqNo(0)
    , m_delim('.')
  {
  }
  //! copy constructor
  explicit FieldSetExp(Field const &orig)
    : Field(orig)
    , m_fieldType(0)
    , m_formula("")
    , m_prompt("")
    , m_seqVal(0)
    , m_seqNo(0)
    , m_delim('.')
  {
  }
  //! destructor
  ~FieldSetExp() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (m_fieldType) o << "fieldType=" << m_fieldType << ",";
    if (!m_formula.empty()) o << "formula=" << m_formula.cstr() << ",";
    if (!m_prompt.empty()) o << "prompt=" << m_prompt.cstr() << ",";
    if (m_seqVal) o << "seqVal=" << m_seqVal << ",";
    if (m_seqNo) o << "seqNo=" << m_seqNo << ",";
    if (m_delim!='.') o << "delim=" << m_delim << ",";
  }
  //! the field type
  int m_fieldType;
  //! the formula
  librevenge::RVNGString m_formula;
  //! the prompt
  librevenge::RVNGString m_prompt;
  //! the seq value
  int m_seqVal;
  //! the seq number
  int m_seqNo;
  //! the deliminator
  char m_delim;
};

FieldSetExp::~FieldSetExp()
{
}

bool FieldSetExp::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldSetExp::send: can not find the listener\n"));
    return false;
  }
  //TODO: set the format
  STOFFField field;
  if (m_type==11) {
    if (m_format&8) // we must also set text:ref-name
      field.m_propertyList.insert("librevenge:field-type", "text:sequence");
    else
      field.m_propertyList.insert("librevenge:field-type", "text:variable-set");
    if (!m_name.empty())
      field.m_propertyList.insert("text:name", m_name);
    if (!m_formula.empty()) {
      if (m_format&8)
        field.m_propertyList.insert("text:formula", m_formula);
      else
        field.m_propertyList.insert("office:string-value", m_formula);
    }
    if (!m_content.empty())
      field.m_propertyList.insert("librevenge:field-content", m_content);
  }
  else
    return Field::send(listener, state);
  listener->insertField(field);
  return true;
}

//! Internal: a set field field
struct FieldSetField final : public Field {
  //! constructor
  FieldSetField()
    : Field()
    , m_condition("")
    , m_dbName("")
    , m_textNumber("")
    , m_longNumber(0)
  {
  }
  //! copy constructor
  explicit FieldSetField(Field const &orig)
    : Field(orig)
    , m_condition("")
    , m_dbName("")
    , m_textNumber("")
    , m_longNumber(0)
  {
  }
  //! destructor
  ~FieldSetField() final;
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const final;
  //! print a field
  void print(std::ostream &o) const final
  {
    Field::print(o);
    if (!m_condition.empty()) o << "condition=" << m_condition.cstr() << ",";
    if (!m_dbName.empty()) o << "dbName=" << m_dbName.cstr() << ",";
    if (!m_textNumber.empty()) o << "number=" << m_textNumber.cstr() << ",";
    else if (m_longNumber) o << "number=" << m_longNumber << ",";
  }
  //! the condition
  librevenge::RVNGString m_condition;
  //! the dbName
  librevenge::RVNGString m_dbName;
  //! the number as text
  librevenge::RVNGString m_textNumber;
  //! the number as num
  long m_longNumber;
};

FieldSetField::~FieldSetField()
{
}

bool FieldSetField::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldSetField::send: can not find the listener\n"));
    return false;
  }
  //TODO: set the format
  STOFFField field;
  if (m_type==28) {
    field.m_propertyList.insert("librevenge:field-type", "text:database-row-select");
    updateDatabaseName(m_dbName, field.m_propertyList);
    if (!m_condition.empty())
      field.m_propertyList.insert("text:condition", m_condition);
    if (!m_textNumber.empty())
      field.m_propertyList.insert("text:row-number", m_textNumber);
    else
      field.m_propertyList.insert("text:row-number", int(m_longNumber));
    // CHECKME: we need to set also text:table-type
  }
  else // also 27,29...
    return Field::send(listener, state);
  listener->insertField(field);
  return true;
}

////////////////////////////////////////
//! Internal: the state of a SWFieldManager
struct State {
  //! constructor
  State()
  {
  }
};

////////////////////////////////////////
//! Internal: the subdocument of a SWFieldManger
class SubDocument final : public STOFFSubDocument
{
public:
  explicit SubDocument(librevenge::RVNGString const &text)
    : STOFFSubDocument(nullptr, STOFFInputStreamPtr(), STOFFEntry())
    , m_text(text) {}

  //! destructor
  ~SubDocument() override {}

  //! operator!=
  bool operator!=(STOFFSubDocument const &doc) const final
  {
    if (STOFFSubDocument::operator!=(doc)) return true;
    auto const *sDoc = dynamic_cast<SubDocument const *>(&doc);
    if (!sDoc) return true;
    if (m_text != sDoc->m_text) return true;
    return false;
  }

  //! the parser function
  void parse(STOFFListenerPtr &listener, libstoff::SubDocumentType type) final;

protected:
  //! the text
  librevenge::RVNGString m_text;
};

void SubDocument::parse(STOFFListenerPtr &listener, libstoff::SubDocumentType /*type*/)
{
  if (!listener.get()) {
    STOFF_DEBUG_MSG(("SWFielManagerInternal::SubDocument::parse: no listener\n"));
    return;
  }
  if (m_text.empty())
    listener->insertChar(' ');
  else
    listener->insertUnicodeString(m_text);
}

bool FieldPostIt::send(STOFFListenerPtr &listener, StarState &state) const
{
  if (!listener || !listener->canWriteText()) {
    STOFF_DEBUG_MSG(("SWFieldManagerInternal::FieldPostIt::send: can not find the listener\n"));
    return false;
  }
  if (m_type==14) {
    std::shared_ptr<STOFFSubDocument> doc(new SubDocument(m_content));
    librevenge::RVNGString date;
    if (m_date)
      date.sprintf("%d/%d/%d", int((m_date/100)%100), int(m_date%100), int(m_date/10000));
    listener->insertComment(doc, m_author, date);
    return true;
  }
  return Field::send(listener, state);
}

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SWFieldManager::SWFieldManager()
  : m_state(new SWFieldManagerInternal::State)
{
}

SWFieldManager::~SWFieldManager()
{
}

std::shared_ptr<SWFieldManagerInternal::Field> SWFieldManager::readField(StarZone &zone, unsigned char cKind)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  unsigned char type;
  long pos=input->tell();
  std::shared_ptr<SWFieldManagerInternal::Field> field;
  if (cKind!='_' && (input->peek()!=cKind || !zone.openSWRecord(type))) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return field;
  }

  // sw_sw3field.cxx inFieldType('Y') or inField('y' or '_')
  field.reset(new SWFieldManagerInternal::Field);
  libstoff::DebugStream f;
  if (cKind!='_')
    f << "Entries(SWFieldType)[" << cKind << "-" << zone.getRecordLevel() << "]:";
  else
    f << "Entries(SWFieldType)[" << zone.getRecordLevel() << "]:";
  field->m_type=int(input->readULong(cKind=='Y' ? 1 : 2));
  if (cKind!='Y') {
    if (zone.isCompatibleWith(0x202)) {
      field->m_format=int(input->readULong(4));
      field->m_subType=int(input->readULong(2));
    }
    else if (zone.isCompatibleWith(0x200))
      field->m_format=int(input->readULong(4));
    else
      field->m_format=int(input->readULong(2));
  }
  int val;
  std::vector<uint32_t> name;
  librevenge::RVNGString poolName;
  long lastPos=zone.getRecordLastPosition();
  switch (field->m_type) {
  case 0: { // dbfld: cKind=='Y' call lcl_sw3io_InDBFieldType
    std::shared_ptr<SWFieldManagerInternal::FieldDBField> dBField(new SWFieldManagerInternal::FieldDBField(*field));
    field=dBField;
    if (cKind=='Y' || !zone.isCompatibleWith(0x202)) {
      if (!zone.isCompatibleWith(0xa)) {
        if (!zone.readString(name)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          dBField->m_colName=libstoff::getString(name);
      }
      else {
        val=int(input->readULong(2));
        if (!zone.getPoolName(val, dBField->m_colName))
          f << "###nPoolId=" << val << ",";
      }
      if (cKind=='Y') {
        if (zone.isCompatibleWith(0x10,0x22, 0x101)) {
          val=int(input->readULong(2));
          if (!zone.getPoolName(val, dBField->m_dbName)) // dbName
            f << "###nDbName=" << val << ",";
        }
        break;
      }
      // lcl_sw3io_InDBField40
      if (!zone.readString(name)) {
        f << "###expand,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        field->m_content=libstoff::getString(name); // expand
      if (zone.isCompatibleWith(0xa))
        f << "cFlag=" << std::hex << input->readULong(1) << std::dec << ",";
      if (zone.isCompatibleWith(0x10,0x22, 0x101)) {
        val=int(input->readULong(2));
        if (!zone.getPoolName(val, dBField->m_dbName)) // dbName
          f << "###nDbName=" << val << ",";
      }
      break;
    }
    // lcl_sw3io_InDBField
    auto cFlag=int(input->readULong(1));
    if (cFlag)
      f << "cFlag=" << std::hex << cFlag << std::dec << ",";
    val=int(input->readULong(2));
    if (!zone.getPoolName(val, dBField->m_colName)) // colName
      f << "###nColNamePoolId=" << val << ",";
    val=int(input->readULong(2));
    if (!zone.getPoolName(val, dBField->m_dbName))
      f << "###dbNamePoolId=" << val << ",";
    if (cFlag&1) {
      bool isNan;
      if (!input->readDoubleReverted8(field->m_doubleValue, isNan)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
        f << "##value,";
      }
    }
    else if (!zone.readString(name)) {
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string value\n"));
      f << "##value,";
      break;
    }
    else
      field->m_textValue=libstoff::getString(name);
    break;
  }
  case 1: {
    // userfld: cKind==Y lcl_sw3io_InUserFieldType|lcl_sw3io_InUserFieldType40
    if (!zone.isCompatibleWith(0xa)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        field->m_name=libstoff::getString(name);
    }
    else {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, field->m_name))
        f << "###name=" << val << ",";
    }
    if (cKind!='Y' && zone.isCompatibleWith(0xa)) break;
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      f << "###aContent,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a aContent\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(text);
    if (!zone.isCompatibleWith(0x0202)) {
      if (!zone.readString(text)) {
        f << "###aValue,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a aValue\n"));
        break;
      }
      else
        field->m_textValue=libstoff::getString(text);
    }
    else {
      bool isNan;
      if (!input->readDoubleReverted8(field->m_doubleValue, isNan)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
        f << "##value,";
        break;
      }
    }
    val=int(input->readULong(2)); // StoreMe
    f << "type=" << val << ",";
    break;
  }
  case 2:
    if (cKind=='Y') break;
    // lcl_sw3io_InFileNameField
    if (input->tell()>=lastPos) break;
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      field->m_name=libstoff::getString(name);
    break;
  case 3:
    if (cKind=='Y') break;
    // lcl_sw3io_InDBNameField
    if (zone.isCompatibleWith(0x10,0x22, 0x101)) {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, field->m_name))
        f << "###nDbName=" << val << ",";
    }
    break;
  case 4:
  // lcl_sw3io_InDateField40
  case 5:
    // lcl_sw3io_InTimeField40
    field.reset(new SWFieldManagerInternal::FieldDateTime(*field));
    break;
  case 6: {
    std::shared_ptr<SWFieldManagerInternal::FieldPageNumber> pageNumber(new SWFieldManagerInternal::FieldPageNumber(*field));
    field=pageNumber;
    if (cKind=='Y') break;
    // lcl_sw3io_InPageNumberField40 && lcl_sw3io_InPageNumberField
    if (!zone.isCompatibleWith(0x202)) {
      pageNumber->m_offset=int(input->readLong(2));
      pageNumber->m_subType=int(input->readULong(2)); // 1: next, 2:prev
      if (!zone.isCompatibleWith(0x9)) break;
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      pageNumber->m_userString=libstoff::getString(name);
      if (zone.isCompatibleWith(0x14,0x22) && (pageNumber->m_subType==1 || pageNumber->m_subType==2))
        pageNumber->m_offset=int(input->readLong(2));
      break;
    }
    pageNumber->m_offset=int(input->readLong(2));
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    pageNumber->m_userString=libstoff::getString(name);
    break;
  }
  case 7:
    // lcl_sw3io_InAuthorField
    if (cKind=='Y') break;
    if (zone.isCompatibleWith(0x204)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      field->m_content=libstoff::getString(name);
    }
    break;
  case 8:
    if (cKind=='Y') break;
    // lcl_sw3io_InChapterField
    if (zone.isCompatibleWith(0x9))
      field->m_level=int(input->readULong(1));
    break;
  case 9:
    if (cKind=='Y') break;
    // lcl_sw3io_InDocStatField40 or lcl_sw3io_InDocStatField
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    break;
  case 10:
    if (cKind=='Y') break;
    // lcl_sw3io_InGetExpField40 or lcl_sw3io_InGetExpField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    field->m_name=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    field->m_content=libstoff::getString(name).cstr();
    if (!zone.isCompatibleWith(0x202)) {
      field->m_subType=int(input->readULong(2));
    }
    break;
  case 11: { // setexpfield: ckind=y call lcl_sw3io_InSetExpFieldType
    std::shared_ptr<SWFieldManagerInternal::FieldSetExp> setExp(new SWFieldManagerInternal::FieldSetExp(*field));
    field=setExp;
    if (cKind!='Y' && zone.isCompatibleWith(0x202)) {
      // lcl_sw3io_InSetExpField
      auto cFlags=int(input->readULong(1));
      if (cFlags) f << "flag=" << cFlags << ",";
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, field->m_name)) // checkme
        f << "###nName=" << val << ",";
      if (!zone.readString(name)) {
        f << "###formula,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        setExp->m_formula=libstoff::getString(name);
      if (cFlags & 0x10) {
        if (!zone.readString(name)) {
          f << "###prompt,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          setExp->m_prompt=libstoff::getString(name);
      }
      if (cFlags & 0x20) {
        setExp->m_seqVal=int(input->readULong(2));
        setExp->m_seqNo=int(input->readULong(2));
      }
      if (cFlags & 0x40) {
        if (!zone.readString(name)) {
          f << "###expand,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          field->m_content=libstoff::getString(name);
      }
      break;
    }
    if (cKind=='Y')
      setExp->m_fieldType=int(input->readULong(2));
    if (!zone.isCompatibleWith(0xa)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        field->m_name=libstoff::getString(name);
    }
    else {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, field->m_name))
        f << "###name=" << val << ",";
    }
    if (cKind=='Y') {
      if ((setExp->m_fieldType&8) && zone.isCompatibleWith(0x202)) {
        setExp->m_delim=char(input->readULong(1));
        setExp->m_level=int(input->readULong(1));
      }
      break;
    }
    // lcl_sw3io_InSetExpField40 end
    auto cFlags=int(input->readULong(1));
    if (cFlags)
      f << "cFlags=" << cFlags << ",";
    if (!zone.readString(name)) {
      f << "###formula,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      setExp->m_formula=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###expand,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      setExp->m_content=libstoff::getString(name);
    if ((cFlags & 0x10) && zone.isCompatibleWith(0x10)) {
      if (!zone.readString(name)) {
        f << "###prompt,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        setExp->m_prompt=libstoff::getString(name);
    }
    if (cFlags & 0x20)
      setExp->m_seqNo=int(input->readULong(2));
    break;
  }
  case 12:
    if (cKind=='Y') break;
    // lcl_sw3io_InGetRefField40 or lcl_sw3io_InGetRefField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_name=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(name);
    if (zone.isCompatibleWith(0x21,0x22)) {
      field->m_format=int(input->readULong(2));
      field->m_subType=int(input->readULong(2));
    }
    else if (zone.isCompatibleWith(0x101,0x202))
      field->m_subType=int(input->readULong(2));
    f << "nSeqNo=" << input->readULong(2) << ",";
    break;
  case 13: {
    std::shared_ptr<SWFieldManagerInternal::FieldHiddenText> hiddenText(new SWFieldManagerInternal::FieldHiddenText(*field));
    field=hiddenText;
    if (cKind=='Y') break;
    // lcl_sw3io_InHiddenTxtField40 or lcl_sw3io_InHiddenTxtField
    f << "cFlags=" << input->readULong(1) << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      hiddenText->m_content=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      hiddenText->m_condition=libstoff::getString(name);
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    break;
  }
  case 14: {
    std::shared_ptr<SWFieldManagerInternal::FieldPostIt> postIt(new SWFieldManagerInternal::FieldPostIt(*field));
    field=postIt;
    if (cKind=='Y') break;
    // lcl_sw3io_InPostItField
    postIt->m_date=long(input->readULong(4));
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      postIt->m_author=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      postIt->m_content=libstoff::getString(name);
    break;
  }
  case 15: // date
  case 16: { // time
    std::shared_ptr<SWFieldManagerInternal::FieldDateTime> dateTime(new SWFieldManagerInternal::FieldDateTime(*field));
    field=dateTime;
    if (cKind=='Y' || zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InFixDateField40 or lcl_sw3io_InFixTimeField40
    dateTime->m_dateTime=long(input->readULong(4));
    break;
  }
  case 17:
    // unknown code
    break;
  case 18:
    // unknown code
    break;
  case 19:
    // unknown code
    break;
  case 20:
    if (cKind=='Y') break;
    // lcl_sw3io_InInputField40 or lcl_sw3io_InInputField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    field->m_content=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_name=libstoff::getString(name); // the prompt
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    break;
  case 21:
    if (cKind=='Y') break;
    // lcl_sw3io_InMacroField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_name=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(name);
    break;
  case 22: { // ddefld: cKind=Y call lcl_sw3io_InDDEFieldType
    if (cKind!='Y' && !zone.isCompatibleWith(0xa)) {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, field->m_name))
        f << "###name=" << val << ",";
      break;
    }
    // lcl_sw3io_InDDEField
    val=int(input->readULong(2));
    f << "nType=" << val << ","; // linkAlway, onUpdate, ...
    if (!zone.isCompatibleWith(0xa)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        field->m_name=libstoff::getString(name);
    }
    else {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, field->m_name))
        f << "###name=" << val << ",";
    }
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      f << "###text,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a text\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(text);
    break;
  }
  case 23:
    if (cKind=='Y') break;
    // lcl_sw3io_InTblField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else // formula
      field->m_name=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(name).cstr();
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    break;
  case 24: {
    std::shared_ptr<SWFieldManagerInternal::FieldHiddenText> hiddenPara(new SWFieldManagerInternal::FieldHiddenText(*field));
    field=hiddenPara;
    if (cKind=='Y') break;
    // lcl_sw3io_InHiddenParaField
    *input >> hiddenPara->m_hidden;
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      hiddenPara->m_condition=libstoff::getString(name);
    break;
  }
  case 25:
    if (cKind=='Y') break;
    // lcl_sw3io_InDocInfoField40 or lcl_sw3io_InDocInfoField
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    else {
      auto flag=int(input->readULong(1));
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        field->m_content=libstoff::getString(name);
      if (flag&1) {
        bool isNan;
        if (!input->readDoubleReverted8(field->m_doubleValue, isNan)) {
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
          f << "##value,";
          break;
        }
      }
    }
    break;
  case 26:
    // lcl_sw3io_InTemplNameField
    break;
  case 27: {
    std::shared_ptr<SWFieldManagerInternal::FieldSetField> setField(new SWFieldManagerInternal::FieldSetField(*field));
    field=setField;
    if (cKind=='Y') break;
    // lcl_sw3io_InDBNextSetField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      setField->m_condition=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      setField->m_name=libstoff::getString(name).cstr();
    if (zone.isCompatibleWith(0x10,0x22, 0x101)) {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, setField->m_dbName))
        f << "###dbName=" << val << ",";
    }
    break;
  }
  case 28: {
    std::shared_ptr<SWFieldManagerInternal::FieldSetField> setField(new SWFieldManagerInternal::FieldSetField(*field));
    field=setField;
    if (cKind=='Y') break;
    // lcl_sw3io_InDBNumSetField
    bool inverted=(zone.isCompatibleWith(0x22,0x101));
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (inverted)
      setField->m_textNumber=libstoff::getString(name);
    else
      setField->m_condition=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!inverted)
      setField->m_textNumber=libstoff::getString(name);
    else
      setField->m_condition=libstoff::getString(name);
    if (zone.isCompatibleWith(0x10,0x22, 0x101)) {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, setField->m_dbName))
        f << "###dbName=" << val << ",";
    }
    break;
  }
  case 29: {
    std::shared_ptr<SWFieldManagerInternal::FieldSetField> setField(new SWFieldManagerInternal::FieldSetField(*field));
    field=setField;
    if (cKind=='Y') break;
    // lcl_sw3io_InDBSetNumberField
    setField->m_longNumber=long(input->readULong(4));
    if (zone.isCompatibleWith(0x10,0x22, 0x101)) {
      val=int(input->readULong(2));
      if (!zone.getPoolName(val, setField->m_dbName))
        f << "###dbName=" << val << ",";
    }
    break;
  }
  case 30:
    if (cKind=='Y') break;
    // lcl_sw3io_InExtUserField40 or lcl_sw3io_InExtUserField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else // data
      field->m_name=libstoff::getString(name);
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    else if (zone.isCompatibleWith(0x204)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        field->m_content=libstoff::getString(name);
    }
    break;
  case 31: {
    std::shared_ptr<SWFieldManagerInternal::FieldPageNumber> pageNumber(new SWFieldManagerInternal::FieldPageNumber(*field));
    field=pageNumber;
    if (cKind=='Y') break;
    // lcl_sw3io_InRefPageSetField
    pageNumber->m_offset=int(input->readLong(2));
    *input >> pageNumber->m_isOn;
    break;
  }
  case 32:
    if (cKind=='Y') break;
    // lcl_sw3io_InRefPageGetField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(name);
    break;
  case 33: {
    std::shared_ptr<SWFieldManagerInternal::FieldINet> iNet(new SWFieldManagerInternal::FieldINet(*field));
    field=iNet;
    if (cKind=='Y' || zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InINetField31
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      iNet->m_url=libstoff::getString(name).cstr();
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else // text
      iNet->m_content=libstoff::getString(name);
    if (zone.isCompatibleWith(0x11,0x22)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        iNet->m_target=libstoff::getString(name);
    }
    if (zone.isCompatibleWith(0x11,0x13)) {
      auto nCnt=int(input->readULong(2));
      for (int i=0; i<nCnt; ++i) {
        if (input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a libmac name\n"));
          f << "###libname,";
          break;
        }
        if (!zone.readString(name)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          iNet->m_libNames.push_back(libstoff::getString(name).cstr());
        if (!zone.readString(name)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          iNet->m_libNames.push_back(libstoff::getString(name).cstr());
      }
    }
    break;
  }
  case 34: {
    std::shared_ptr<SWFieldManagerInternal::FieldJumpEdit> jumpEdit(new SWFieldManagerInternal::FieldJumpEdit(*field));
    field=jumpEdit;
    if (cKind=='Y') break;
    // lcl_sw3io_InJumpEditField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      field->m_content=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      jumpEdit->m_help=libstoff::getString(name);
    break;
  }
  case 35: {
    std::shared_ptr<SWFieldManagerInternal::FieldScript> script(new SWFieldManagerInternal::FieldScript(*field));
    field=script;
    if (cKind=='Y') break;
    // lcl_sw3io_InScriptField40 or lcl_sw3io_InScriptField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      script->m_scriptType=libstoff::getString(name);
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else
      script->m_code=libstoff::getString(name);
    if (zone.isCompatibleWith(0x200))
      f << "cFlags=" << input->readULong(1) << ",";
    break;
  }
  case 36: {
    std::shared_ptr<SWFieldManagerInternal::FieldDateTime> dateTime(new SWFieldManagerInternal::FieldDateTime(*field));
    field=dateTime;
    if (cKind=='Y' || !zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InDateTimeField
    bool isNan;
    if (!input->readDoubleReverted8(dateTime->m_doubleValue, isNan)) {
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
      f << "##value,";
      break;
    }
    if (zone.isCompatibleWith(0x205))
      dateTime->m_offset=long(input->readULong(4));
    break;
  }
  case 37: { // cKind==Y call lcl_sw3io_InAuthorityFieldType
    // TODO: store me
    if (cKind!='Y' && zone.isCompatibleWith(0x202)) {
      // lcl_sw3io_InAuthorityField
      zone.openFlagZone();
      f << "nPos=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      break;
    }
    if (cKind!='Y') break;
    val=int(zone.openFlagZone());
    if (val&0xf0) f << "flag=" << (val>>4) << ",";
    auto N=int(input->readULong(2));
    if (N) f << "N=" << N << ",";
    val=int(input->readULong(1));
    if (val) f << "cPrefix=" << val << ",";
    val=int(input->readULong(1));
    if (val) f << "cSuffix=" << val << ",";
    auto nSort=int(input->readULong(2));
    if (nSort) f << "cSortCount=" << nSort << ",";
    zone.closeFlagZone();

    bool ok=true;
    for (int i=0; i<N; ++i) {
      long actPos=input->tell();
      input->seek(actPos, librevenge::RVNG_SEEK_SET);

      unsigned char authType;
      libstoff::DebugStream f2;
      f2<<"SWFieldType[auth-A" << i << "]:";
      if (input->peek()!='E' || !zone.openSWRecord(authType)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read an authority zone\n"));

        f2<< "###";
        ascFile.addPos(actPos);
        ascFile.addNote(f2.str().c_str());
        ok=false;
        break;
      }
      ascFile.addPos(actPos);
      ascFile.addNote(f2.str().c_str());
      zone.closeSWRecord(authType, "SWFieldType");
    }
    if (!ok || !nSort) break;
    long actPos=input->tell();
    libstoff::DebugStream f2;
    f2<<"SWFieldType[auth-B]:";
    if (input->tell()+3*nSort>zone.getRecordLastPosition()) {
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read sort data\n"));
      f2 << "###";
      ascFile.addPos(actPos);
      ascFile.addNote(f2.str().c_str());
      break;
    }
    f2 << "sort[flag:type]=[";
    for (int i=0; i<nSort; ++i) {
      f2 << std::hex << input->readULong(1) << std::dec << ":" << input->readULong(2) << ",";
    }
    f2 << "],";
    ascFile.addPos(actPos);
    ascFile.addNote(f2.str().c_str());
    break;
  }
  case 38: // combinedChar
    break;
  case 39: // dropdown108791
    break;
  default:
    STOFF_DEBUG_MSG(("SWFieldManager::readField: find unexpected flag\n"));
    break;
  }
  f << *field;

  if (input->tell()!=zone.getRecordLastPosition() && cKind=='_') {
    STOFF_DEBUG_MSG(("SWFieldManager::readField: find extra data\n"));
    f << "###extra";
    ascFile.addDelimiter(input->tell(),'|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (cKind!='_')
    zone.closeSWRecord(cKind, "SWFieldType");
  return field;
}

std::shared_ptr<SWFieldManagerInternal::Field> SWFieldManager::readPersistField(StarZone &zone, long lastPos)
{
  std::shared_ptr<SWFieldManagerInternal::Field> field;
  // pstm.cxx SvPersistStream::ReadObj
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascii=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(PersistField)["<< zone.getRecordLevel() << "]:";
  // SvPersistStream::ReadId
  uint8_t hdr;
  *input >> hdr;
  long id=0, classId=0;
  bool ok=true;
  if (hdr&0x80) // nId=0
    ;
  else {
    if ((hdr&0xf)==0) {
      if ((hdr&0x20) || !(hdr&0x40))
        ok=input->readCompressedLong(id);
    }
    else if (hdr&0x10)
      ok=input->readCompressedLong(id);
    if (hdr&0x60)
      ok=input->readCompressedLong(classId);
  }
  if (id) f << "id=" << id << ",";
  if (classId) f << "id[class]=" << classId << ",";
  if (!ok || !hdr || input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: find unexpected header\n"));
    f << "###header";
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return field;
  }
  if (hdr&0x80 || (hdr&0x40)==0) {
    ascii.addPos(pos);
    ascii.addNote(f.str().c_str());
    return std::make_shared<SWFieldManagerInternal::Field>();
  }
  if (hdr&0x20) {
    ok=zone.openSCRecord();
    if (!ok || zone.getRecordLastPosition()>lastPos) {
      STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not open main zone\n"));
      if (ok) zone.closeSCRecord("PersistField");
      f << "###,";
      ascii.addPos(pos);
      ascii.addNote(f.str().c_str());
      return field;
    }
    lastPos=zone.getRecordLastPosition();
  }
  if (hdr&0x40) {
    switch (classId) {
    // case 1 SvxFieldData::
    case 2: {
      // SvxDateField::Load
      if (input->tell()+8>lastPos) {
        STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read date field\n"));
        f << "###,";
        break;
      }
      auto dateTime=std::make_shared<SWFieldManagerInternal::FieldDateTime>();
      field=dateTime;
      dateTime->m_type=15;
      dateTime->m_dateTime=long(input->readULong(4));
      dateTime->m_subType=int(input->readULong(2));
      dateTime->m_format=int(input->readULong(2));
      if (dateTime->m_dateTime && dateTime->m_subType==1) // type==0: mean fixed
        dateTime->m_type=4;
      f << "date[field],";
      f << "date=" << dateTime->m_dateTime << ",";
      f << "type=" << dateTime->m_subType << ",";
      f << "format=" << dateTime->m_format << ",";
      break;
    }
    case 3: { // flditem.cxx:void SvxURLField::Load
      f << "urlData,";
      auto inet=std::make_shared<SWFieldManagerInternal::FieldINet>();
      field=inet;
      field->m_type=33;
      field->m_format=int(input->readULong(2));
      if (field->m_format) f << "format=" << field->m_format << ",";
      for (int i=0; i<2; ++i) {
        std::vector<uint32_t> text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read a string\n"));
          f << "##string";
          break;
        }
        else if (!text.empty()) {
          if (i==0)
            inet->m_url=libstoff::getString(text);
          else
            inet->m_target=libstoff::getString(text);
          f << (i==0 ? "url" : "representation") << "=" << libstoff::getString(text).cstr() << ",";
        }
      }
      if (input->tell()==lastPos)
        break;
      uint32_t nFrameMarker;
      *input>>nFrameMarker;
      uint16_t val;
      switch (nFrameMarker) {
      case 0x21981357:
        *input>>val;
        if (val) f << "char[set]=" << val << ",";
        break;
      case 0x21981358:
        for (int i=0; i<2; ++i) {
          *input>>val;
          if (val) f << "f" << i << "=" << val << ",";
        }
        break;
      default:
        input->seek(-4, librevenge::RVNG_SEEK_CUR);
        break;
      }
      break;
    }
    case 50: // SdrMeasureField(unsure)
      f << "measureField,";
      if (input->tell()+2>lastPos) {
        STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read measure field\n"));
        f << "###,";
        break;
      }
      f << "kind=" << input->readULong(2) << ",";
      break;
    case 100: // flditem.cxx
      field=std::make_shared<SWFieldManagerInternal::FieldPageNumber>();
      field->m_type=6;
      f << "pageField,";
      break;
    case 101:
      field=std::make_shared<SWFieldManagerInternal::Field>();
      field->m_type=9;
      field->m_subType=0;
      f << "pagesField,";
      break;
    case 102:
      field=std::make_shared<SWFieldManagerInternal::FieldDateTime>();
      field->m_type=5;
      f << "timeField,";
      break;
    case 103:
      field=std::make_shared<SWFieldManagerInternal::Field>();
      field->m_type=2;
      f << "fileField,";
      break;
    case 104:
      f << "tableField,";
      break;
    case 105: {
      f << "timeField[extended],";
      if (input->tell()+8>lastPos) {
        STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read extended time field\n"));
        f << "###,";
        break;
      }
      auto dateTime=std::make_shared<SWFieldManagerInternal::FieldDateTime>();
      field=dateTime;
      dateTime->m_type=16;
      dateTime->m_dateTime=long(input->readULong(4));
      dateTime->m_subType=int(input->readULong(2));
      dateTime->m_format=int(input->readULong(2));
      if (dateTime->m_dateTime && dateTime->m_subType==1)  // type==0: mean fixed
        dateTime->m_type=5;

      f << "date[field],";
      if (dateTime->m_dateTime)
        f << "time=" << dateTime->m_dateTime << ",";
      f << "type=" << dateTime->m_subType << ",";
      f << "format=" << dateTime->m_format << ",";
      break;
    }
    case 106: {
      f << "fileField[extended],";
      field=std::make_shared<SWFieldManagerInternal::Field>();
      field->m_type=2;
      std::vector<uint32_t> text;
      if (!zone.readString(text) || input->tell()+4>lastPos) {
        STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read a string\n"));
        f << "##string";
        break;
      }
      else if (!text.empty())
        f << libstoff::getString(text).cstr() << ",";
      f << "type=" << input->readULong(2) << ",";
      field->m_format=int(input->readULong(2));
      f << "format=" << field->m_format << ",";
      break;
    }
    case 107: {
      field=std::make_shared<SWFieldManagerInternal::Field>();
      field->m_type=7;
      f << "authorField,";
      bool fieldOk=true;
      for (int i=0; i<3; ++i) {
        std::vector<uint32_t> text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read a string\n"));
          f << "##string";
          fieldOk=false;
          break;
        }
        else if (!text.empty())
          f << (i==0 ? "name" : i==1 ? "first[name]": "last[name]") << "=" << libstoff::getString(text).cstr() << ",";
      }
      if (!fieldOk) break;
      if (input->tell()+4>lastPos) {
        STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: can not read author field\n"));
        f << "###,";
        break;
      }
      f << "type=" << input->readULong(2) << ",";
      field->m_format=int(input->readULong(2));
      f << "format=" << field->m_format << ",";
      break;
    }
    default:
      STOFF_DEBUG_MSG(("SWFieldManager::readPersistField: unknown class id\n"));
      f << "##classId";
      break;
    }
  }
  if (input->tell()!=lastPos)
    ascii.addDelimiter(input->tell(),'|');
  input->seek(lastPos, librevenge::RVNG_SEEK_SET);
  if (hdr&0x20)
    zone.closeSCRecord("PersistField");

  ascii.addPos(pos);
  ascii.addNote(f.str().c_str());
  if (field) return field;
  return std::make_shared<SWFieldManagerInternal::Field>(); // create a dummy field
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
