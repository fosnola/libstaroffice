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

#include "StarZone.hxx"

#include "SWFieldManager.hxx"

/** Internal: the structures of a SWFieldManager */
namespace SWFieldManagerInternal
{
////////////////////////////////////////
//! Internal: a field
struct Field {
  //! constructor
  Field() : m_type(-1), m_subType(-1), m_format(-1), m_name(""), m_content(""), m_textValue(""), m_doubleValue(0), m_level(0)
  {
  }
  //! destructor
  virtual ~Field();
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Field const &field)
  {
    field.print(o);
    return o;
  }
  //! print a field
  virtual void print(std::ostream &o) const;
  //! the field type
  int m_type;
  //! the subtype
  int m_subType;
  //! the field format
  int m_format;
  //! the name
  librevenge::RVNGString m_name;
  //! the content
  librevenge::RVNGString m_content;
  //! the value text
  librevenge::RVNGString m_textValue;
  //! double
  double m_doubleValue;
  //! the chapter level
  int m_level;
protected:
  //! copy constructor
  Field(const Field &orig) : m_type(orig.m_type), m_subType(orig.m_subType), m_format(orig.m_format),
    m_name(orig.m_name), m_content(orig.m_content), m_textValue(orig.m_textValue), m_doubleValue(orig.m_doubleValue), m_level(orig.m_level)
  {
  }
};

Field::~Field()
{
}

void Field::print(std::ostream &o) const
{
  if (m_type>=0 || m_type<40) {
    char const *(wh[])= {"db", "user", "filename", "dbName",
                         "inDate40", "inTime40", "pageNumber", "author",
                         "chapter", "docStat", "getExp", "setExp",
                         "getRef", "hiddenText", "postIt", "fixDate",
                         "fixTime", "reg", "varReg", "setRef",
                         "input", "macro", "dde", "tbl",
                         "hiddenPara", "docInfo", "templName", "dbNextSet",
                         "dbNumSet", "dbSetNumber", "extUser", "pageSet",
                         "pageGet", "INet", "jumpEdit", "script",
                         "dateTime", "authority", "combinedChar", "dropDown"
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

//! Internal: a fixed date time field
struct FieldDateTime : public Field {
  //! constructor
  FieldDateTime() : Field(), m_dateTime(0), m_offset(0)
  {
  }
  //! copy constructor
  FieldDateTime(Field const &orig) : Field(orig), m_dateTime(0), m_offset(0)
  {
  }
  //! destructor
  virtual ~FieldDateTime();
  //! print a field
  virtual void print(std::ostream &o) const
  {
    Field::print(o);
    if (m_dateTime) o << "date/time=" << m_dateTime << ",";
    if (m_offset) o << "offset=" << m_offset << ",";
  }
  //! the dateTime
  long m_dateTime;
  //! the offset
  long m_offset;
};

FieldDateTime::~FieldDateTime()
{
}

//! Internal: a DB field field
struct FieldDBField : public Field {
  //! constructor
  FieldDBField() : Field(), m_dbName(""), m_colName(""), m_longNumber(0)
  {
  }
  //! copy constructor
  FieldDBField(Field const &orig) : Field(orig), m_dbName(""), m_colName(""), m_longNumber(0)
  {
  }
  //! destructor
  virtual ~FieldDBField();
  //! print a field
  virtual void print(std::ostream &o) const
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

//! Internal: a hidden text/para field
struct FieldHiddenText : public Field {
  //! constructor
  FieldHiddenText() : Field(), m_hidden(true), m_condition("")
  {
  }
  //! copy constructor
  FieldHiddenText(Field const &orig) : Field(orig), m_hidden(true), m_condition("")
  {
  }
  //! destructor
  virtual ~FieldHiddenText();
  //! print a field
  virtual void print(std::ostream &o) const
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

//! Internal: a set field field
struct FieldINet : public Field {
  //! constructor
  FieldINet() : Field(), m_url(""), m_target(""), m_libNames()
  {
  }
  //! copy constructor
  FieldINet(Field const &orig) : Field(orig), m_url(""), m_target(""), m_libNames()
  {
  }
  //! destructor
  virtual ~FieldINet();
  //! print a field
  virtual void print(std::ostream &o) const
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
  //! the number as text
  std::vector<librevenge::RVNGString> m_libNames;
};

FieldINet::~FieldINet()
{
}

//! Internal: a jump edit field
struct FieldJumpEdit : public Field {
  //! constructor
  FieldJumpEdit() : Field(), m_help("")
  {
  }
  //! copy constructor
  FieldJumpEdit(Field const &orig) : Field(orig), m_help("")
  {
  }
  //! destructor
  virtual ~FieldJumpEdit();
  //! print a field
  virtual void print(std::ostream &o) const
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

//! Internal: a pageNumber field
struct FieldPageNumber : public Field {
  //! constructor
  FieldPageNumber() : Field(), m_userString(""), m_offset(0), m_isOn(true)
  {
  }
  //! copy constructor
  FieldPageNumber(Field const &orig) : Field(orig), m_userString(""), m_offset(0), m_isOn(true)
  {
  }
  //! destructor
  virtual ~FieldPageNumber();
  //! print a field
  virtual void print(std::ostream &o) const
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

//! Internal: a postit field
struct FieldPostIt : public Field {
  //! constructor
  FieldPostIt() : Field(), m_author(""), m_date(0)
  {
  }
  //! copy constructor
  FieldPostIt(Field const &orig) : Field(orig), m_author(""), m_date(0)
  {
  }
  //! destructor
  virtual ~FieldPostIt();
  //! print a field
  virtual void print(std::ostream &o) const
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
struct FieldScript : public Field {
  //! constructor
  FieldScript() : Field(), m_code(""), m_scriptType("")
  {
  }
  //! copy constructor
  FieldScript(Field const &orig) : Field(orig), m_code(""), m_scriptType("")
  {
  }
  //! destructor
  virtual ~FieldScript();
  //! print a field
  virtual void print(std::ostream &o) const
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
struct FieldSetExp : public Field {
  //! constructor
  FieldSetExp() : Field(), m_fieldType(0), m_formula(""), m_prompt(""), m_seqVal(0), m_seqNo(0), m_delim('.')
  {
  }
  //! copy constructor
  FieldSetExp(Field const &orig) : Field(orig), m_fieldType(0), m_formula(""), m_prompt(""), m_seqVal(0), m_seqNo(0), m_delim('.')
  {
  }
  //! destructor
  virtual ~FieldSetExp();
  //! print a field
  virtual void print(std::ostream &o) const
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

//! Internal: a set field field
struct FieldSetField : public Field {
  //! constructor
  FieldSetField() : Field(), m_condition(""), m_dbName(""), m_textNumber(""), m_longNumber(0)
  {
  }
  //! copy constructor
  FieldSetField(Field const &orig) : Field(orig), m_condition(""), m_dbName(""), m_textNumber(""), m_longNumber(0)
  {
  }
  //! destructor
  virtual ~FieldSetField();
  //! print a field
  virtual void print(std::ostream &o) const
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
////////////////////////////////////////
//! Internal: the state of a SWFieldManager
struct State {
  //! constructor
  State()
  {
  }
};

}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
SWFieldManager::SWFieldManager() : m_state(new SWFieldManagerInternal::State)
{
}

SWFieldManager::~SWFieldManager()
{
}

bool SWFieldManager::readField(StarZone &zone, char cKind)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (cKind!='_' && (input->peek()!=cKind || !zone.openSWRecord(type))) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // sw_sw3field.cxx inFieldType('Y') or inField('y' or '_')
  shared_ptr<SWFieldManagerInternal::Field> field(new SWFieldManagerInternal::Field);
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
    shared_ptr<SWFieldManagerInternal::FieldDBField> dBField(new SWFieldManagerInternal::FieldDBField(*field));
    field=dBField;
    if (cKind=='Y' || !zone.isCompatibleWith(0x202)) {
      if (!zone.isCompatibleWith(0xa)) {
        if (!zone.readString(name)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          field->m_textValue=libstoff::getString(name); // text
      }
      else {
        val=int(input->readULong(2));
        if (!zone.getPoolName(val, field->m_textValue)) // text
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
    int cFlag=int(input->readULong(1));
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
    break;
  case 5:
    // lcl_sw3io_InTimeField40
    break;
  case 6: {
    shared_ptr<SWFieldManagerInternal::FieldPageNumber> pageNumber(new SWFieldManagerInternal::FieldPageNumber(*field));
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
    if (!zone.isCompatibleWith(0x202))
      field->m_subType=int(input->readULong(2));
    break;
  case 11: { // setexpfield: ckind=y call lcl_sw3io_InSetExpFieldType
    shared_ptr<SWFieldManagerInternal::FieldSetExp> setExp(new SWFieldManagerInternal::FieldSetExp(*field));
    field=setExp;
    if (cKind!='Y' && zone.isCompatibleWith(0x202)) {
      // lcl_sw3io_InSetExpField
      int cFlags=int(input->readULong(1));
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
    int cFlags=int(input->readULong(1));
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
    shared_ptr<SWFieldManagerInternal::FieldHiddenText> hiddenText(new SWFieldManagerInternal::FieldHiddenText(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldPostIt> postIt(new SWFieldManagerInternal::FieldPostIt(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldDateTime> dateTime(new SWFieldManagerInternal::FieldDateTime(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldHiddenText> hiddenPara(new SWFieldManagerInternal::FieldHiddenText(*field));
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
      int flag=int(input->readULong(1));
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
    shared_ptr<SWFieldManagerInternal::FieldSetField> setField(new SWFieldManagerInternal::FieldSetField(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldSetField> setField(new SWFieldManagerInternal::FieldSetField(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldSetField> setField(new SWFieldManagerInternal::FieldSetField(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldPageNumber> pageNumber(new SWFieldManagerInternal::FieldPageNumber(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldINet> iNet(new SWFieldManagerInternal::FieldINet(*field));
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
      int nCnt=int(input->readULong(2));
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
    shared_ptr<SWFieldManagerInternal::FieldJumpEdit> jumpEdit(new SWFieldManagerInternal::FieldJumpEdit(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldScript> script(new SWFieldManagerInternal::FieldScript(*field));
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
    shared_ptr<SWFieldManagerInternal::FieldDateTime> dateTime(new SWFieldManagerInternal::FieldDateTime(*field));
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
    f << "authority,";
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
    int N=int(input->readULong(2));
    if (N) f << "N=" << N << ",";
    val=int(input->readULong(1));
    if (val) f << "cPrefix=" << val << ",";
    val=int(input->readULong(1));
    if (val) f << "cSuffix=" << val << ",";
    int nSort=int(input->readULong(2));
    if (nSort) f << "cSortCount=" << nSort << ",";
    zone.closeFlagZone();

    bool ok=true;
    for (int i=0; i<N; ++i) {
      long actPos=input->tell();
      input->seek(actPos, librevenge::RVNG_SEEK_SET);

      char authType;
      libstoff::DebugStream f2;
      f2<<"SWFieldType[auth-A" << i << "]:";
      if (input->peek()!='E' || !zone.openSWRecord(authType)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read an authority zone\n"));

        f2<< "###";
        ascFile.addPos(actPos);
        ascFile.addNote(f2.str().c_str());
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
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
