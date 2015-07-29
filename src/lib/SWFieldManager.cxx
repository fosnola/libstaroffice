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

#include "SWZone.hxx"

#include "SWFieldManager.hxx"

/** Internal: the structures of a SWFieldManager */
namespace SWFieldManagerInternal
{
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

bool SWFieldManager::readField(SWZone &zone, char cKind)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  char type;
  long pos=input->tell();
  if (cKind!='_' && (input->peek()!=cKind || !zone.openRecord(type))) {
    input->seek(pos, librevenge::RVNG_SEEK_SET);
    return false;
  }

  // sw_sw3field.cxx inFieldType('Y') or inField('y' or '_')
  libstoff::DebugStream f;
  if (cKind!='_')
    f << "Entries(SWFieldType)[" << cKind << "-" << zone.getRecordLevel() << "]:";
  else
    f << "Entries(SWFieldType)[" << zone.getRecordLevel() << "]:";
  int fieldType=(int) input->readULong(cKind=='Y' ? 1 : 2);
  if (cKind!='Y') {
    if (zone.isCompatibleWith(0x0202)) {
      f << "fldFmt=" << input->readULong(4) << ",";
      f << "subType=" << input->readULong(2) << ",";
    }
    else if (zone.isCompatibleWith(0x0200))
      f << "fldFmt=" << input->readULong(4) << ",";
    else
      f << "fldFmt=" << input->readULong(2) << ",";
  }

  int val;
  librevenge::RVNGString name, poolName;
  long lastPos=zone.getRecordLastPosition();
  switch (fieldType) {
  case 0: { // default
    f << "default,";
    if (cKind=='Y' || !zone.isCompatibleWith(0x202)) {
      if (!zone.isCompatibleWith(0xa)) {
        if (!zone.readString(poolName)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else
          f << poolName.cstr() << ",";
      }
      else {
        val=(int) input->readULong(2);
        if (!zone.getPoolName(val, poolName))
          f << "###nPoolId=" << val << ",";
        else if (!poolName.empty())
          f << poolName.cstr() << ",";
      }
      if (cKind=='Y') {
        if ((zone.isCompatibleWith(0x10) && !zone.isCompatibleWith(0x22)) ||
            zone.isCompatibleWith(0x101)) {
          val=(int) input->readULong(2);
          if (!zone.getPoolName(val, poolName))
            f << "###nDbName=" << val << ",";
          else if (!poolName.empty())
            f << "dbName=" << poolName.cstr() << ",";
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
        f << "expand=" << name.cstr() << ",";
      if (zone.isCompatibleWith(0xa))
        f << "cFlag=" << std::hex << input->readULong(1) << std::dec << ",";
      if ((zone.isCompatibleWith(0x10) && !zone.isCompatibleWith(0x22)) ||
          zone.isCompatibleWith(0x101)) {
        val=(int) input->readULong(2);
        if (!zone.getPoolName(val, poolName))
          f << "###nDbName=" << val << ",";
        else if (!poolName.empty())
          f << "dbName=" << poolName.cstr() << ",";
      }
      break;
    }
    // lcl_sw3io_InDBField
    int cFlag=(int) input->readULong(1);
    if (cFlag)
      f << "cFlag=" << std::hex << cFlag << std::dec << ",";
    val=(int) input->readULong(2);
    if (!zone.getPoolName(val, poolName))
      f << "###nColNamePoolId=" << val << ",";
    else if (!poolName.empty())
      f << "colName=" << poolName.cstr() << ",";
    val=(int) input->readULong(2);
    if (!zone.getPoolName(val, poolName))
      f << "###dbNamePoolId=" << val << ",";
    else if (!poolName.empty())
      f << "dbName=" << poolName.cstr() << ",";
    if (cFlag&1) {
      double res;
      bool isNan;
      if (!input->readDouble8(res, isNan)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
        f << "##value,";
      }
      else if (res<0||res>0)
        f << "value=" << res << ",";
    }
    else if (!zone.readString(name)) {
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string value\n"));
      f << "##value,";
      break;
    }
    else if (!name.empty())
      f << "value=" << name.cstr() << ",";
    break;
  }
  case 1: { // userfld
    f << "user,";
    if (!zone.isCompatibleWith(0xa)) {
      if (!zone.readString(poolName)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        f << poolName.cstr() << ",";
    }
    else {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
    }
    if (cKind!='Y' && zone.isCompatibleWith(0xa)) break;
    librevenge::RVNGString text("");
    if (!zone.readString(text)) {
      f << "###aContent,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a aContent\n"));
      break;
    }
    else
      f << text.cstr() << ",";
    if (!zone.isCompatibleWith(0x0202)) {
      if (!zone.readString(text)) {
        f << "###aValue,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a aValue\n"));
        break;
      }
      else
        f << text.cstr() << ",";
    }
    else {
      double res;
      bool isNan;
      if (!input->readDouble8(res, isNan)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
        f << "##value,";
        break;
      }
      else if (res<0||res>0)
        f << "value=" << res << ",";
    }
    val=(int) input->readULong(2);
    f << "type=" << val << ",";
    break;
  }
  case 2:
    f << "filename,";
    if (cKind=='Y') break;
    // lcl_sw3io_InFileNameField
    if (input->tell()>=lastPos) break;
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "name=" << name.cstr() << ",";
    break;
  case 3:
    f << "dbNameField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InDBNameField
    if ((zone.isCompatibleWith(0x10) && !zone.isCompatibleWith(0x22)) ||
        zone.isCompatibleWith(0x101)) {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nDbName=" << val << ",";
      else if (!poolName.empty())
        f << "dbName=" << poolName.cstr() << ",";
    }
    break;
  case 4:
    f << "inDateField40,";
    // lcl_sw3io_InDateField40
    break;
  case 5:
    f << "inTimeField40,";
    // lcl_sw3io_InTimeField40
    break;
  case 6:
    f << "pageNumberField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InPageNumberField40 && lcl_sw3io_InPageNumberField: TODO
    if (!zone.isCompatibleWith(0x202)) {
      f << "nOff=" << input->readLong(2) << ",";
      int nSub=(int) input->readULong(2);
      f << "nSub=" << nSub << ",";
      if (!zone.isCompatibleWith(0x9)) break;
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "userString=" << name.cstr() << ",";
      if (zone.isCompatibleWith(0x14) && !zone.isCompatibleWith(0x22) && (nSub==1 || nSub==2))
        f << "nOff=" << input->readLong(2) << ",";
      break;
    }
    f << "nOff=" << input->readLong(2) << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "userString=" << name.cstr() << ",";
    break;
  case 7:
    f << "inAuthorField,";
    if (cKind=='Y') break;
    if (zone.isCompatibleWith(0x204)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "expand=" << name.cstr() << ",";
    }
    break;
  case 8:
    f << "chapterField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InChapterField
    if (zone.isCompatibleWith(0x9)) {
      f << "level=" << input->readULong(1) << ",";
    }
    break;
  case 9:
    f << "docStatField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InDocStatField40 or lcl_sw3io_InDocStatField
    if (!zone.isCompatibleWith(0x202))
      f << "subType=" << input->readULong(2) << ",";
    break;
  case 10:
    f << "getExp,";
    if (cKind=='Y') break;
    // lcl_sw3io_InGetExpField40 or lcl_sw3io_InGetExpField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aText=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "expand=" << name.cstr() << ",";
    if (!zone.isCompatibleWith(0x202))
      f << "nSub=" << input->readULong(2) << ",";
    break;
  case 11: { // setexpfield
    f << "setExp,";
    if (cKind!='Y' && zone.isCompatibleWith(0x202)) {
      // lcl_sw3io_InSetExpField
      int cFlags=(int) input->readULong(1);
      if (cFlags) f << "flag=" << cFlags << ",";
      f << "nPoolId=" << input->readULong(2) << ",";
      if (!zone.readString(name)) {
        f << "###formula,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "formula=" << name.cstr() << ",";
      if (cFlags & 0x10) {
        if (!zone.readString(name)) {
          f << "###prompt,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else if (!name.empty())
          f << "prompt=" << name.cstr() << ",";
      }
      if (cFlags & 0x20) {
        f << "nSeqVal=" << input->readULong(2) << ",";
        f << "nSeqNo=" << input->readULong(2) << ",";
      }
      if (cFlags & 0x40) {
        if (!zone.readString(name)) {
          f << "###expand,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else if (!name.empty())
          f << "expand=" << name.cstr() << ",";
      }
      break;
    }
    int nType=0;
    if (cKind=='Y') {
      nType=(int) input->readULong(2);
      f << "nType=" << nType << ",";
    }
    if (!zone.isCompatibleWith(0xa)) {
      if (!zone.readString(poolName)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        f << poolName.cstr() << ",";
    }
    else {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
    }
    if (cKind=='Y') {
      if ((nType&8) && zone.isCompatibleWith(0x202)) {
        val=(int) input->readULong(1);
        if (val) f << "cDelim=" << val << ",";
        val=(int) input->readULong(1);
        if (val) f << "nLevel=" << val << ",";
      }
      break;
    }
    // lcl_sw3io_InSetExpField40 end
    int cFlags=(int) input->readULong(1);
    if (cFlags)
      f << "cFlags=" << cFlags << ",";
    if (!zone.readString(name)) {
      f << "###formula,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "formula=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###expand,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "expand=" << name.cstr() << ",";
    if ((cFlags & 0x10) && zone.isCompatibleWith(0x10)) {
      if (!zone.readString(name)) {
        f << "###prompt,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "prompt=" << name.cstr() << ",";
    }
    if (cFlags & 0x20)
      f << "nSeqNo=" << input->readULong(2) << ",";
    break;
  }
  case 12:
    f << "getRefField";
    if (cKind=='Y') break;
    // lcl_sw3io_InGetRefField40 or lcl_sw3io_InGetRefField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aName=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "expand=" << name.cstr() << ",";
    if (zone.isCompatibleWith(0x21) && !zone.isCompatibleWith(0x22)) {
      f << "fmt=" << input->readULong(2) << ",";
      f << "subType=" << input->readULong(2) << ",";
      f << "nSeqNo=" << input->readULong(2) << ",";
    }
    else if (zone.isCompatibleWith(0x202))
      f << "nSeqNo=" << input->readULong(2) << ",";
    else if (zone.isCompatibleWith(0x101)) {
      f << "subType=" << input->readULong(2) << ",";
      f << "nSeqNo=" << input->readULong(2) << ",";
    }
    break;
  case 13:
    f << "inHiddenTextField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InHiddenTxtField40 or lcl_sw3io_InHiddenTxtField
    f << "cFlags=" << input->readULong(1) << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aText=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aCond=" << name.cstr() << ",";
    if (!zone.isCompatibleWith(0x202))
      f << "nSubType=" << input->readULong(2) << ",";
    break;
  case 14:
    f << "postItFld,";
    if (cKind=='Y') break;
    // lcl_sw3io_InPostItField
    f << "date=" << input->readULong(4) << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aAuthor=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aText=" << name.cstr() << ",";
    break;
  case 15:
    f << "fixDateField,";
    if (cKind=='Y' || zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InFixDateField40
    f << input->readULong(4) << ",";
    break;
  case 16:
    f << "fixTimeField,";
    if (cKind=='Y' || zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InFixTimeField40
    f << input->readULong(4) << ",";
    break;
  case 17:
    f << "regFld,";
    break;
  case 18:
    f << "varRegFld,";
    break;
  case 19:
    f << "setRefFld,";
    break;
  case 20:
    f << "inputfld,";
    if (cKind=='Y') break;
    // lcl_sw3io_InInputField40 or lcl_sw3io_InInputField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aContent=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aPrompt=" << name.cstr() << ",";
    if (!zone.isCompatibleWith(0x202))
      f << "nSubType=" << input->readULong(2) << ",";
    break;
  case 21:
    f << "macroField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InMacroField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aName=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aText=" << name.cstr() << ",";
    break;
  case 22: { // ddefld
    f << "link,";
    if (cKind!='Y' && !zone.isCompatibleWith(0xa)) {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
      break;
    }
    // lcl_sw3io_InDDEField
    val=(int) input->readULong(2);
    f << "nType=" << val << ",";
    if (!zone.isCompatibleWith(0xa)) {
      if (!zone.readString(poolName)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else
        f << poolName.cstr() << ",";
    }
    else {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
    }
    librevenge::RVNGString text("");
    if (!zone.readString(text)) {
      f << "###text,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a text\n"));
      break;
    }
    else
      f << text.cstr() << ",";
    break;
  }
  case 23:
    f << "inTblField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InTblField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aFormula=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aText=" << name.cstr() << ",";
    if (!zone.isCompatibleWith(0x202))
      f << "nSub=" << input->readULong(2) << ",";
    break;
  case 24:
    f << "inHiddenParaField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InHiddenParaField
    f << "bHidden=" << input->readULong(1) << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aCond=" << name.cstr() << ",";
    break;
  case 25:
    f << "inDocInfoField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InDocInfoField40 or lcl_sw3io_InDocInfoField
    if (!zone.isCompatibleWith(0x202))
      f << "nSub=" << input->readULong(2) << ",";
    else {
      int flag=(int) input->readULong(1);
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "aContent=" << name.cstr() << ",";
      if (flag&1) {
        double res;
        bool isNan;
        if (!input->readDouble8(res, isNan)) {
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
          f << "##value,";
          break;
        }
        else if (res<0||res>0)
          f << "value=" << res << ",";
      }
    }
    break;
  case 26:
    f << "inTemplNameField,";
    // lcl_sw3io_InTemplNameField
    break;
  case 27:
    f << "inDBNextSetField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InDBNextSetField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aCond=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aName=" << name.cstr() << ",";
    if ((zone.isCompatibleWith(0x10) && !zone.isCompatibleWith(0x22)) ||
        zone.isCompatibleWith(0x101)) {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
    }
    break;
  case 28: {
    f << "inDbNumSetField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InDBNumSetField
    bool inverted=(zone.isCompatibleWith(0x22) && !zone.isCompatibleWith(0x101));
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << (inverted ? "aNumber=" : "aCond=") << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << (inverted ? "aCond=" : "aNumber=") << name.cstr() << ",";
    if ((zone.isCompatibleWith(0x10) && !zone.isCompatibleWith(0x22)) ||
        zone.isCompatibleWith(0x101)) {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
    }
    break;
  }
  case 29:
    f << "inDbSetNumberField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InDBSetNumberField
    f << "n=" << input->readULong(4) << ",";
    if ((zone.isCompatibleWith(0x10) && !zone.isCompatibleWith(0x22)) ||
        zone.isCompatibleWith(0x101)) {
      val=(int) input->readULong(2);
      if (!zone.getPoolName(val, poolName))
        f << "###nPoolId=" << val << ",";
      else if (!poolName.empty())
        f << poolName.cstr() << ",";
    }
    break;
  case 30:
    f << "inExtUserField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InExtUserField40 or lcl_sw3io_InExtUserField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aData" << name.cstr() << ",";
    if (!zone.isCompatibleWith(0x202))
      f << "nSubType=" << input->readULong(2);
    else if (zone.isCompatibleWith(0x204)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "aExpand" << name.cstr() << ",";
    }
    break;
  case 31:
    f << "refPageSetField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InRefPageSetField
    f << "nOffset=" << input->readLong(2) << ",";
    f << "cIsOn=" << input->readULong(1) << ",";
    break;
  case 32:
    f << "refPageGetField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InRefPageGetField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aText=" << name.cstr() << ",";
    break;
  case 33:
    f << "InINertField31,";
    if (cKind=='Y' || zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InINetField31
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "aURL=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "text=" << name.cstr() << ",";
    if (zone.isCompatibleWith(0x11) && !zone.isCompatibleWith(0x22)) {
      if (!zone.readString(name)) {
        f << "###string,";
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
        break;
      }
      else if (!name.empty())
        f << "target=" << name.cstr() << ",";
    }
    if (zone.isCompatibleWith(0x11) && !zone.isCompatibleWith(0x13)) {
      int nCnt=(int) input->readULong(2);
      f << "N=" << nCnt << ",";
      f << "libMac=[";
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
        else if (!name.empty())
          f << name.cstr() << ":";
        if (!zone.readString(name)) {
          f << "###string,";
          STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
          break;
        }
        else if (!name.empty())
          f << name.cstr();
        f << ",";
      }
      f << "],";
    }
    break;
  case 34:
    f << "InJumpEditField,";
    if (cKind=='Y') break;
    // lcl_sw3io_InJumpEditField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "text=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "help=" << name.cstr() << ",";
    break;
  case 35: {
    f << "InScriptFld,";
    if (cKind=='Y') break;
    // lcl_sw3io_InScriptField40 or lcl_sw3io_InScriptField
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "type=" << name.cstr() << ",";
    if (!zone.readString(name)) {
      f << "###string,";
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a string\n"));
      break;
    }
    else if (!name.empty())
      f << "code=" << name.cstr() << ",";
    if (zone.isCompatibleWith(0x200))
      f << "cFlags=" << input->readULong(1) << ",";
    break;
  }
  case 36: {
    f << "InDataTimeField";
    if (cKind=='Y' || !zone.isCompatibleWith(0x202)) break;
    // lcl_sw3io_InDateTimeField
    double res;
    bool isNan;
    if (!input->readDouble8(res, isNan)) {
      STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read a double\n"));
      f << "##value,";
      break;
    }
    else if (res<0||res>0)
      f << "value=" << res << ",";
    if (zone.isCompatibleWith(0x205))
      f << "offset=" << input->readULong(4) << ",";
    break;
  }
  case 37: {
    f << "authority,";
    if (cKind!='Y' && zone.isCompatibleWith(0x202)) {
      // lcl_sw3io_InAuthorityField
      zone.openFlagZone();
      f << "nPos=" << input->readULong(2) << ",";
      zone.closeFlagZone();
      break;
    }
    if (cKind!='Y') break;
    val=(int) zone.openFlagZone();
    if (val&0xf0) f << "flag=" << (val>>4) << ",";
    int N=(int) input->readULong(2);
    if (N) f << "N=" << N << ",";
    val=(int) input->readULong(1);
    if (val) f << "cPrefix=" << val << ",";
    val=(int) input->readULong(1);
    if (val) f << "cSuffix=" << val << ",";
    int nSort=(int) input->readULong(2);
    if (nSort) f << "cSortCount=" << nSort << ",";
    zone.closeFlagZone();

    bool ok=true;
    for (int i=0; i<N; ++i) {
      long actPos=input->tell();
      input->seek(actPos, librevenge::RVNG_SEEK_SET);

      char authType;
      libstoff::DebugStream f2;
      f2<<"Field[auth-A" << i << "]:";
      if (input->peek()!='E' || !zone.openRecord(authType)) {
        STOFF_DEBUG_MSG(("SWFieldManager::readField: can not read an authority zone\n"));

        f2<< "###";
        ascFile.addPos(actPos);
        ascFile.addNote(f2.str().c_str());
        break;
      }
      ascFile.addPos(actPos);
      ascFile.addNote(f2.str().c_str());
      zone.closeRecord(authType);
    }
    if (!ok || !nSort) break;
    long actPos=input->tell();
    libstoff::DebugStream f2;
    f2<<"Field[auth-B]:";
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
  case 38:
    f << "combinedChar,";
    break;
  case 39:
    f << "dropdown108791,";
    break;
  default:
    STOFF_DEBUG_MSG(("SWFieldManager::readField: find unexpected flag\n"));
    f << "##type=" << fieldType << ",";
    break;
  }

  if (input->tell()!=zone.getRecordLastPosition()) {
    STOFF_DEBUG_MSG(("SWFieldManager::readField: find extra data\n"));
    f << "###extra";
    ascFile.addDelimiter(input->tell(),'|');
    if (cKind=='_')
      input->seek(zone.getRecordLastPosition(), librevenge::RVNG_SEEK_SET);
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());

  if (cKind!='_')
    zone.closeRecord(cKind);
  return true;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
