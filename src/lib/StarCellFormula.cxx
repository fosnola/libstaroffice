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

#include "STOFFOLEParser.hxx"

#include "StarEncoding.hxx"
#include "StarZone.hxx"

#include "StarCellFormula.hxx"


////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
void StarCellFormula::updateFormula(STOFFCellContent &content, std::vector<librevenge::RVNGString> const &sheetNames, int sheetId)
{
  int numNames=(int) sheetNames.size();
  for (size_t i=0; i<content.m_formula.size(); ++i) {
    STOFFCellContent::FormulaInstruction &form=content.m_formula[i];
    if ((form.m_type!=STOFFCellContent::FormulaInstruction::F_Cell &&
         form.m_type!=STOFFCellContent::FormulaInstruction::F_CellList) ||
        form.m_sheetId<0 || form.m_sheetId==sheetId)
      continue;
    if (form.m_sheetId>=numNames) {
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarCellFormula::updateFormula: some sheetId are bad\n"));
        first=false;
      }
      continue;
    }
    form.m_sheet=sheetNames[size_t(form.m_sheetId)];
  }
}

bool StarCellFormula::readSCFormula(StarZone &zone, STOFFCell &cell, STOFFCellContent &content, int version, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCFormula)[" << zone.getRecordLevel() << "]:";
  // sc_token.cxx ScTokenArray::Load
  uint8_t fFlags;
  *input>>fFlags;
  if (fFlags&0xf) input->seek((fFlags&0xf), librevenge::RVNG_SEEK_CUR);
  f << "cMode=" << input->readULong(1) << ","; // if (version<0x201) old mode
  if (fFlags&0x10) f << "nRefs=" << input->readLong(2) << ",";
  if (fFlags&0x20) f << "nErrors=" << input->readULong(2) << ",";
  bool ok=true;
  if (fFlags&0x40) { // token
    uint16_t nLen;
    *input>>nLen;
    for (int tok=0; tok<nLen; ++tok) {
      if (!readSCToken(zone, cell, content, version, lastPos) || input->tell()>lastPos) {
        f << "###";
        ok=false;
      }
    }
    f << "formula=";
    bool hasIndex=false;
    for (size_t i=0; i<content.m_formula.size(); ++i) {
      f << content.m_formula[i];
      if (content.m_formula[i].m_type==STOFFCellContent::FormulaInstruction::F_Index)
        hasIndex=true;
    }
    f << ",";

    if (ok && hasIndex) {
      static bool first=true;
      if (first) {
        STOFF_DEBUG_MSG(("StarCellFormula::readSCFormula: formula with index are not implemented\n"));
        first=false;
      }
      f << "##index,";
    }
    else if (ok)
      content.m_contentType=STOFFCellContent::C_FORMULA;
  }
  if (!ok) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }
  if (fFlags&0x80) {
    uint16_t nRPN;
    *input >> nRPN;
    f << "rpn=[";
    STOFFCellContent rContent;
    for (int rpn=0; ok && rpn<int(nRPN); ++rpn) {
      uint8_t b1;
      *input >> b1;
      if (b1==0xff) {
        rContent.m_formula.clear();
        if (!readSCToken(zone, cell, rContent, version, lastPos)) {
          ok=false;
          f << "###";
        }
        for (size_t i=0; i<rContent.m_formula.size(); ++i)
          f << rContent.m_formula[i] << ",";
      }
      else if (b1&0x40)
        f << "[Index" << ((b1&0x3f) | (input->readULong(1)<<6)) << "]";
      else
        f << "[Index" << int(b1) << "]";
      if (input->tell()>lastPos) {
        f << "###";
        ok=false;
      }
    }
    f << "],";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok;
}

bool StarCellFormula::readSCFormula3(StarZone &zone, STOFFCell const &cell, STOFFCellContent &content,
                                     int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCFormula)[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  for (int tok=0; tok<512; ++tok) {
    bool endData;
    if (!readSCToken3(zone, cell, content, endData, lastPos) || input->tell()>lastPos) {
      f << "###";
      ok=false;
      break;
    }
    if (endData) break;
  }
  bool hasIndex=false;
  for (size_t i=0; i<content.m_formula.size(); ++i) {
    f << content.m_formula[i];
    if (content.m_formula[i].m_type==STOFFCellContent::FormulaInstruction::F_Index)
      hasIndex=true;
  }

  if (ok && hasIndex) {
    STOFF_DEBUG_MSG(("StarCellFormula::readSCFormula3: formula with index are not implemented\n"));
    f << "##index,";
  }
  else if (ok)
    content.m_contentType=STOFFCellContent::C_FORMULA;
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarCellFormula::readSCToken(StarZone &zone, STOFFCell const &/*cell*/, STOFFCellContent &content, int version, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugStream f;
  // sc_token.cxx ScRawToken::Load
  uint16_t nOp;
  uint8_t type;
  *input >> nOp >> type;
  bool ok=true;
  STOFFCellContent::FormulaInstruction instr;
  // first read the data
  switch (type) {
  case 0: {
    int8_t val;
    *input>>val;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Long;
    instr.m_longValue=val;
    break;
  }
  case 1: {
    double val;
    *input>>val;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Double;
    instr.m_doubleValue=val;
    break;
  }
  case 2: // string
  case 8: // external
  default: { // ?
    if (type==8) {
      int cByte=(int) input->readULong(1);
      if (cByte)
        f << "cByte=" << cByte << ",";
    }
    uint8_t nBytes;
    *input >> nBytes;
    if (input->tell()+int(nBytes)>lastPos) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read text zone\n"));
      f << "###text";
      ok=false;
      break;
    }
    std::vector<uint8_t> text;
    for (int i=0; i<int(nBytes); ++i) text.push_back((uint8_t) input->readULong(1));
    std::vector<uint32_t> string;
    StarEncoding::convert(text, zone.getEncoding(), string);
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Text;
    instr.m_content=libstoff::getString(string);
    if (type==8)
      f << "external,";
    else if (type!=2)
      f << "f" << type << ",";
    break;
  }
  case 3:
  case 4: {
    int16_t nCol, nRow, nTab;
    uint8_t nByte;
    *input >> nCol >> nRow >> nTab >> nByte;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Cell;
    instr.m_sheetId=nTab;
    instr.m_position[0][0]=nCol;
    instr.m_position[0][1]=nRow;
    if (version<0x10) {
      instr.m_positionRelative[0][0]=(nByte&3)!=0;
      instr.m_positionRelative[0][1]=((nByte>>2)&3)!=0;
      instr.m_sheetIdRelative=((nByte>>4)&3)!=0;
      if (nByte>>6) f << "fl=" << int(nByte>>6) << ",";
    }
    else {
      instr.m_positionRelative[0][0]=(nByte&1)!=0;
      instr.m_positionRelative[0][1]=((nByte>>2)&1)!=0;
      instr.m_sheetIdRelative=((nByte>>4)&1)!=0;
      if (nByte&0xEA) f << "fl=" << std::hex << int(nByte&0xEF) << std::dec << ",";
    }
    if (type==4) {
      *input >> nCol >> nRow >> nTab >> nByte;
      instr.m_type=STOFFCellContent::FormulaInstruction::F_CellList;
      instr.m_position[1][0]=nCol;
      instr.m_position[1][1]=nRow;
      if (nTab!=instr.m_sheetId) {
        STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: referencing different sheet is not implemented\n"));
        f << "#sheet2=" << nTab << ",";
      }
      if (version<0x10) {
        instr.m_positionRelative[1][0]=(nByte&3)!=0;
        instr.m_positionRelative[1][1]=((nByte>>2)&3)!=0;
        if (nByte>>6) f << "fl2=" << int(nByte>>6) << ",";
      }
      else {
        instr.m_positionRelative[0][0]=(nByte&1)!=0;
        instr.m_positionRelative[0][1]=((nByte>>2)&1)!=0;
        instr.m_sheetIdRelative=((nByte>>4)&1)!=0;
        if (nByte&0xEA) f << "fl=" << std::hex << int(nByte&0xEA) << std::dec << ",";
      }
    }
    break;
  }
  case 6:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Index;
    instr.m_longValue=(long)input->readULong(2);
    break;
  case 7: {
    uint8_t nByte;
    *input >> nByte;
    if (input->tell()+2*int(nByte)>lastPos) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read the jump\n"));
      f << "###jump";
      ok=false;
      break;
    }
    if (nOp==5 || nOp==6) { // safe to ignore
      input->seek(2*int(nByte), librevenge::RVNG_SEEK_CUR);
      break;
    }
    f << "J=[";
    for (int i=0; i<(int) nByte; ++i) f << input->readLong(2) << ",";
    f << "],";
    break;
  }
  case 0x70:
    f << "#missing";
    break;
  case 0x71:
    f << "#error";
    break;
  }
  bool ignoreInstr=false;
  switch (nOp) {
  case 0:
    break;
  case 2: // endData
    ignoreInstr=true;
    break;
  case 3:
    if (type!=8) f << "##type=" << type << ",";
    break;
  case 4: // index
    if (type!=6) f << "##type=" << type << ",";
    break;
  case 5:
    if (type!=7) f << "##type=" << type << ",";
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="If";
    break;
  case 7:
    if (type) f << "##type=" << type << ",";
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="(";
    break;
  case 8:
    if (type) f << "##type=" << type << ",";
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=")";
    break;
  case 9:
    if (type) f << "##type=" << type << ",";
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=";";
    break;
  case 12:
    // extra space
    if (type) f << "##type=" << type << ",";
    ignoreInstr=true;
    break;
  case 17:
    if (type) f << "##type=" << type << ",";
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="%";
    break;
  default:
    if (type!=0) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read a formule\n"));
      f << "##f" << nOp << "[" << type << "],";
      ok=false;
    }
    else if (nOp==33 || nOp==34) { // change, ie reconstructor a&&b in AND(a,b), ...
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
      instr.m_content=nOp==33 ? "and" : "or";
    }
    else if (nOp>=21 && nOp<=37) {
      static char const *(wh[])=
      {"+", "-", "*", "/", "&", "^", "=", "<>", "<", ">", "<=", ">=", "OR", "AND", "!", "~", ":"};
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
      instr.m_content=wh[nOp-21];
    }
    else if (nOp==41) { // changeme ie reconstruct ~a in NOT(a)
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
      instr.m_content="Not";
    }
    else if (nOp==42 || nOp==43) {
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
      instr.m_content="-";
    }
    else if (!setFunction(nOp, instr)) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read a formule\n"));
      f << "##f" << nOp << ",";
      ok=false;
    }
    break;
  }
  instr.m_extra=f.str();
  if (!ignoreInstr)
    content.m_formula.push_back(instr);
  return ok && input->tell()<=lastPos;
}

bool StarCellFormula::readSCToken3(StarZone &zone, STOFFCell const &/*cell*/, STOFFCellContent &content, bool &endData, long lastPos)
{
  endData=false;
  STOFFInputStreamPtr input=zone.input();
  // sc_token.cxx ScRawToken::Load30
  uint16_t nOp;
  *input >> nOp;
  bool ok=true;
  libstoff::DebugStream f;
  STOFFCellContent::FormulaInstruction instr;
  switch (nOp) {
  case 0: {
    uint8_t type;
    *input >> type;
    switch (type) {
    case 0: {
      int8_t val;
      *input>>val;
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Long;
      instr.m_longValue=val;
      break;
    }
    case 1: {
      double val;
      *input>>val;
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Double;
      instr.m_doubleValue=val;
      break;
    }
    case 2: {
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarCellFormula::readSCToken3: can not read text zone\n"));
        f << "###text";
        ok=false;
        break;
      }
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Text;
      instr.m_content=libstoff::getString(text);
      break;
    }
    case 3: {
      int16_t nPos[3];
      uint8_t relPos[3], oldFlag;
      *input >> nPos[0] >> nPos[1] >> nPos[2] >> relPos[0] >> relPos[1] >> relPos[2] >> oldFlag;
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Cell;
      instr.m_sheetId=nPos[2];
      if (relPos[2]) instr.m_sheetIdRelative=true;
      for (int i=0; i<2; ++i) {
        instr.m_position[0][i]=nPos[i];// + (relPos[i] ? cell.position()[i] : 0);
        if (relPos[i]) instr.m_positionRelative[0][i]=true;
      }
      if (oldFlag) f << "fl=" << int(oldFlag) << ",";
      break;
    }
    case 4: {
      int16_t nPos[2][3];
      uint8_t relPos[2][3], oldFlag[2];
      *input >> nPos[0][0] >> nPos[0][1] >> nPos[0][2] >> nPos[1][0] >> nPos[1][1] >> nPos[1][2]
             >> relPos[0][0] >> relPos[0][1] >> relPos[0][2] >> relPos[1][0] >> relPos[1][1] >> relPos[1][2]
             >> oldFlag[0] >> oldFlag[1];
      instr.m_type=STOFFCellContent::FormulaInstruction::F_CellList;
      instr.m_sheetId=nPos[0][2];
      if (relPos[0][2]) instr.m_sheetIdRelative=true;
      if (nPos[0][2]!=nPos[1][2] || relPos[0][2]!=relPos[1][2]) {
        STOFF_DEBUG_MSG(("StarCellFormula::readSCToken3: referencing different sheet is not implemented\n"));
        f << "#sheet2=" << nPos[1][2];
        if (relPos[1][2]) f << "[rel]";
        f << ",";
      }
      for (int c=0; c<2; ++c) {
        for (int i=0; i<2; ++i) {
          instr.m_position[c][i]=nPos[c][i];// + (relPos[c][i] ? cell.position()[i] : 0);
          if (relPos[c][i]) instr.m_positionRelative[c][i]=true;
        }
        if (oldFlag[c]) f << "fl" << c << "=" << int(oldFlag[c]) << ",";
      }
      break;
    }
    default:
      f << "##type=" << int(type) << ",";
      ok=false;
      break;
    }
    break;
  }
  case 2: // stop
    endData=true;
    break;
  case 3: { // external
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken3: can not read external zone\n"));
      f << "###external";
      ok=false;
      break;
    }
    f << "#external,";
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Text;
    instr.m_content=libstoff::getString(text);
    break;
  }
  case 4: { // name
    uint16_t index;
    *input >> index;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Index;
    instr.m_longValue=index;
    break;
  }
  case 5: // jump 3
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="If";
    break;
  case 6: // jump=maxjumpcount
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="Choose";
    break;
  case 7:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="(";
    break;
  case 8:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=")";
    break;
  case 9:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=";";
    break;
  case 17:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="%";
    break;
  default:
    if (nOp==33 || nOp==34) { // change, ie reconstructor a&&b in AND(a,b), ...
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
      instr.m_content=nOp==33 ? "and" : "or";
    }
    else if (nOp>=21 && nOp<=37) {
      static char const *(wh[])=
      {"+", "-", "*", "/", "&", "^", "=", "<>", "<", ">", "<=", ">=", "OR", "AND", "!", "~", ":"};
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
      instr.m_content=wh[nOp-21];
    }
    else if (nOp==41) { // changeme ie reconstruct ~a in NOT(a)
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
      instr.m_content="Not";
    }
    else if (nOp==42 || nOp==43) {
      instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
      instr.m_content="-";
    }
    else if (!setFunction(nOp, instr)) {
      ok=false;
      f << "###f" << nOp << ",";
    }
    break;
  }
  if (!endData) {
    instr.m_extra=f.str();
    content.m_formula.push_back(instr);
  }
  return ok && input->tell()<=lastPos;
}

bool StarCellFormula::setFunction(int nOp, STOFFCellContent::FormulaInstruction &instr)
{
  if (nOp>=46 && nOp<=53) {  // 60 endNoPar
    static char const *(wh[])= {
      "Pi", "Random", "True", "False", "GetActDate", "Today", "Now"/*getActTime*/,
      "NA", "Current"
    };
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content=wh[nOp-46];
  }
  else if (nOp==89) {
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Text;
    libstoff::appendUnicode(0xb1, instr.m_content);
  }
  else if (nOp>=61 && nOp<=131) { // 200 endOnePar
    static char const *(wh[])= {
      "Degrees", "Radians", "Sin", "Cos", "Tan", "Cot", "Asin", "Acos", "Atan", "ACot", // 70
      "SinH", "CosH", "TanH", "CotH", "AsinH", "ACosH", "ATanH", "ACosH", // 78
      "Exp", "Ln", "Sqrt", "Fact", // 82
      "Year", "Month", "Day", "Hour", "Minute", "Second", "PlusMinus" /* checkme*/, // 89
      "Abs", "Int", "Phi", "Gauss", "IsBlank", "IsText", "IsNonText", "IsLogical", // 97
      "Type", "IsRef", "IsNumber",  "IsFormula", "IsNA", "IsErr", "IsError", "IsEven", // 105
      "IsOdd", "N", // 107
      "DateValue", "TimeValue", "Code", "Trim", "Upper", "Proper", "Lower", "Len", "T", // 116
      "Value", "Clean", "Char", "Log10", "Even", "Odd", "NormDist", "Fisher", "FisherInv", // 125
      "NormSInv", "GammaLn", "ErrorType", "IsErr" /* errCell*/, "Formula", "Arabic"
    };
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content=wh[nOp-61];
  }
  else if (nOp>=201 && nOp<=386) {
    static char const *(wh[])= {
      "Atan2", "Ceil", "Floor", "Round", "RoundUp", "RoundDown", "Trunc", "Log", // 208
      "Power", "GCD", "LCM", "Mod", "SumProduct", "SumSQ", "SumX2MY2", "SumX2PY2", "SumXMY2", // 217
      "Date", "Time", "Days", "Days360", "Min", "Max", "Sum", "Product", "Average", "Count", // 227
      "CountA", "NPV", "IRR", "Var", "VarP", "StDev", "StDevP", "B", "NormDist", "ExpDist", // 237
      "BinomDist", "Poisson", "Combin", "CombinA", "Permut",  "PermutationA", "PV", "SYD", "DDB", "DB",
      "VDB", "Duration", "SLN", "PMT", "Columns", "Rows", "Column", "Row", "RRI", "FV", // 257
      "NPER", "Rate", "IPMT", "PPMT", "CUMIPMT", "CUMPRINC", "Effective", "Nominal", "SubTotal", "DSum", // 267
      "DCount", "DCountA", "DAverage", "DGet", "DMax", "DMin", "DProduct", "DStDev", "DStDevP", "DVar",
      "DVarP", "Indirect", "Address", "Match", "CountBlank", "CountIf", "SumIf", "LookUp", "VLookUp", "HLookUp", // 287
      "MultiRange", "Offset", "Index", "Areas", "Dollar", "Replace", "Fixed", "Find", "Exact", "Left", // 297
      "Right", "Search", "Mid", "Text", "Substitute", "Rept", "Concatenate", "MValue", "MDeterm", "MInverse",
      "MMult", "Transpose", "MUnit", "GoalSeek", "HypGeomDist", "HYPGEOM.DIST", "LogNormDist", "TDist", "FDist", "ChiDist", "WeiBull",
      "NegBinomDist", "CritBinom", "Kurt", "HarMean", "GeoMean", "Standardize", "AveDev", "Skew", "DevSQ", "Median", // 327
      "Mode", "ZTest", "TTest", "Rank", "Percentile", "PercentRank", "Large", "Small", "Frequency", "Quartile",
      "NormInv", "Confidence", "FTest", "TrimMean", "Prob", "CorRel", "CoVar", "Pearson", "RSQ", "STEYX", // 347
      "Slope", "Intercept", "Trend", "Growth", "Linest", "Logest", "Forecast", "ChiInv", "GammaDist", "GammaInv", // 357
      "TInv", "FInv", "ChiTest", "LogInv", "Multiple.Operations", "BetaDist", "BetaInv", "WeekNum", "WeekDay", "#Name!", // 367
      "Style", "DDE", "Base", "Sheet", "Sheets", "MinA", "MaxA", "AverageA", "StDevA", "StDevPA", // 377
      "VarA", "VarPA", "EasterSunday", "Decimal", "Convert", "Roman", "MIRR", "Cell", "IsPMT"
    };
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content=wh[nOp-201];
  }
  else
    return false;
  return true;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
