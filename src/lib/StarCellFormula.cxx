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

//! namespace used to define StarCellFormula structures
namespace StarCellFormulaInternal
{
//! a structure used to store a token
struct Token {
  //! the different type
  enum Type { Function, Long, Double, String, String2, External, Cell, CellList, Index, Jump, Empty, Missing, Error, Unknown };
  //! the content type
  enum Content { C_Data, C_FunctionOperator };
  //! constructor
  Token()
    : m_type(Unknown)
    , m_content(C_Data)
    , m_operation(0)
    , m_longValue(0)
    , m_doubleValue(0)
    , m_textValue("")
    , m_index(0)
    , m_jumpPositions()
    , m_instruction()
    , m_extra("")
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Token const &tok)
  {
    switch (tok.m_type) {
    case Long:
      o << tok.m_longValue << ",";
      break;
    case Double:
      o << tok.m_doubleValue << ",";
      break;
    case String:
    case String2:
    case External:
      if (tok.m_type==String2) o << "string2,";
      else if (tok.m_type==External) o << "external,";
      o << tok.m_textValue.cstr() << ",";
      break;
    case Cell:
      o << "cells=" << tok.m_positions[0] << "[" << tok.m_relPositions[0] << "],";
      break;
    case CellList:
      o << "cells=" << tok.m_positions[0] << "[" << tok.m_relPositions[0] << "]"
        << "<->" << tok.m_positions[1] << "[" << tok.m_relPositions[1] << "],";
      break;
    case Index:
      o << "index=" << tok.m_longValue << ",";
      break;
    case Function:
      o << tok.m_instruction << ",";
      break;
    case Jump: {
      o << tok.m_instruction << ",";
      o << "jump=[";
      for (int jumpPosition : tok.m_jumpPositions)
        o << jumpPosition << ",";
      o << "],";
      break;
    }
    case Missing:
      o << "#missing,";
      break;
    case Error:
      o << "#error,";
      break;
    case Empty:
      break;
    case Unknown:
    default:
      o << "###unknown,";
      break;
    }
    o << tok.m_extra;
    return o;
  }
  //! return a instruction corresponding to a token
  bool get(STOFFCellContent::FormulaInstruction &instr, bool &ignore);
  //! try to update the function/operator
  bool updateFunction();
  //! a static function to recompile a formula from Polish notation
  static bool addToken(std::vector<std::vector<Token> > &stack, Token const &token);
  //! the type
  Type m_type;
  //! the content type
  Content m_content;
  //! the operation
  unsigned m_operation;
  //! the long value
  long m_longValue;
  //! the double value
  double m_doubleValue;
  //! the string value
  librevenge::RVNGString m_textValue;
  //! the cells positions: col, row, tab
  STOFFVec3i m_positions[2];
  //! the cells relative positions
  STOFFVec3b m_relPositions[2];
  //! the index
  int m_index;
  //! the jump position(for if, choose, ...)
  std::vector<int> m_jumpPositions;
  //! the final instruction
  STOFFCellContent::FormulaInstruction m_instruction;
  //! extra data
  std::string m_extra;
};

bool Token::get(STOFFCellContent::FormulaInstruction &instr, bool &ignore)
{
  ignore=false;
  switch (m_type) {
  case Jump:
  case Function:
    instr=m_instruction;
    ignore=instr.m_content.empty();
    return true;
  case Long:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Long;
    instr.m_longValue=m_longValue;
    return true;
  case Double:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Double;
    instr.m_doubleValue=m_doubleValue;
    return true;
  case String:
  case String2:
  case External:
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Text;
    instr.m_content=m_textValue;
    return true;
  case Cell:
  case CellList:
    instr.m_type=m_type==Cell ? STOFFCellContent::FormulaInstruction::F_Cell :
                 STOFFCellContent::FormulaInstruction::F_CellList;
    instr.m_position[0]=STOFFVec2i(m_positions[0][0],m_positions[0][1]);
    instr.m_positionRelative[0]=STOFFVec2b(m_relPositions[0][0],m_relPositions[0][1]);
    instr.m_sheetId=m_positions[0][2];
    if (m_type==Cell)
      return true;
    instr.m_position[1]=STOFFVec2i(m_positions[1][0],m_positions[1][1]);
    instr.m_positionRelative[1]=STOFFVec2b(m_relPositions[1][0],m_relPositions[1][1]);
    return true;
  case Empty:
  case Missing:
  case Error:
    ignore=true;
    return true;
  case Unknown:
  case Index:
  default:
    break;
  }
  return false;
}

bool Token::updateFunction()
{
  unsigned const &nOp=m_operation;
  auto &instr=m_instruction;

  // binary op
  if (nOp==33 || nOp==34) { // change, ie reconstructor a&&b in AND(a,b), ...
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    m_longValue=2;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content=nOp==33 ? "and" : "or";
  }
  else if (nOp>=21 && nOp<=37) {
    static char const *wh[]=
    {"+", "-", "*", "/", "&", "^", "=", "<>", "<", ">", "<=", ">=", "OR", "AND", "!", "~", ":"};
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    m_longValue=2;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=wh[nOp-21];
  }
  // unary op
  else if (nOp==41) { // changeme ie reconstruct ~a in NOT(a)
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    m_longValue=1;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="Not";
  }
  else if (nOp==42 || nOp==43) { // neg and neg_sub
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    m_longValue=1;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="-";
  }
  // function no parameter
  else if (nOp>=46 && nOp<=53) {  // 60 endNoPar
    static char const *wh[]= {
      "Pi", "Random", "True", "False", "Today"/*getActDate*/, "Now"/*getActTime*/,
      "NA", "Current"
    };
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    m_longValue=0;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content=wh[nOp-46];
  }
  // one parameter
  else if (nOp==89) { // CHANGEME: pm operator
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Text;
    libstoff::appendUnicode(0xb1, instr.m_content);
  }
  else if (nOp>=61 && nOp<=131) { // 200 endOnePar
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    m_longValue=1;
    static char const *wh[]= {
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
  // multiple
  else if (nOp>=201 && nOp<=386) {
    m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    static char const *wh[]= {
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

bool Token::addToken(std::vector<std::vector<Token> > &stack, Token const &token)
{
  std::vector<Token> child;
  if (token.m_content==C_Data) {
    child.push_back(token);
    stack.push_back(child);
    return true;
  }
  bool isOperator=token.m_instruction.m_type==STOFFCellContent::FormulaInstruction::F_Operator;
  auto nChild=int(token.m_longValue);
  auto numElt=int(stack.size());
  int special=0;
  if (nChild<0) {
    special=-nChild;
    nChild=special==2 ? 1 : special;
  }
  if (nChild<0 || nChild>numElt || (special>3) || (isOperator && !special && nChild!=1 && nChild!=2)) {
    STOFF_DEBUG_MSG(("StarCellFormulaInternal::addToken: find unexpected number of child for token %s\n", token.m_instruction.m_content.cstr()));
    return false;
  }
  if (special==2 || (!special && (!isOperator || nChild==1)))
    child.push_back(token);
  Token sep;
  sep.m_type=StarCellFormulaInternal::Token::Function;
  sep.m_instruction.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
  sep.m_instruction.m_content="(";
  // todo: check if we really need to insert a ( when type is operator, ...
  if (!special || special==2)
    child.push_back(sep);
  for (int c=0; c<nChild; ++c) {
    if (special)
      ;
    else if (c && !isOperator) {
      sep.m_instruction.m_content=";";
      child.push_back(sep);
    }
    else if (c)
      child.push_back(token);
    auto const &node=stack[size_t(numElt-nChild+c)];
    child.insert(child.end(), node.begin(), node.end());
  }
  sep.m_instruction.m_content=")";
  if (!special)
    child.push_back(sep);
  else if (special==2) {
    sep.m_instruction.m_content=";";
    child.push_back(sep);
  }
  else
    child.push_back(token);
  stack.resize(size_t(numElt-nChild+1));
  stack[size_t(numElt-nChild)] = child;

  return true;
}

}
////////////////////////////////////////////////////////////
// main zone
////////////////////////////////////////////////////////////
void StarCellFormula::updateFormula(STOFFCellContent &content, std::vector<librevenge::RVNGString> const &sheetNames, int sheetId)
{
  auto numNames=int(sheetNames.size());
  for (auto &form : content.m_formula) {
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

bool StarCellFormula::readSCFormula(StarZone &zone, STOFFCellContent &content, int version, long lastPos)
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
  std::vector<StarCellFormulaInternal::Token> tokenList, rpnList;
  if (fFlags&0x40) { // token
    uint16_t nLen;
    *input>>nLen;
    f << "formula=";
    for (int tok=0; tok<nLen; ++tok) {
      StarCellFormulaInternal::Token token;
      if (!readSCToken(zone, token, version, lastPos) || input->tell()>lastPos) {
        f << "###";
        ok=false;
        break;
      }
      f << token;
      tokenList.push_back(token);
    }
    f << ",";
  }
  if (!ok) {
    ascFile.addPos(pos);
    ascFile.addNote(f.str().c_str());
    return false;
  }

  bool hasIndex=false, formulaSet=false;
  for (auto &token : tokenList) {
    STOFFCellContent::FormulaInstruction finalInstr;
    bool ignore;
    if (token.get(finalInstr,ignore) && !ignore)
      content.m_formula.push_back(finalInstr);
    else if (token.m_type==StarCellFormulaInternal::Token::Index)
      hasIndex=true;
  }
  if (hasIndex)
    content.m_formula.clear();
  else {
    content.m_contentType=STOFFCellContent::C_FORMULA;
    formulaSet=true;
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
        StarCellFormulaInternal::Token token;
        if (!readSCToken(zone, token, version, lastPos)) {
          ok=false;
          f << "###";
        }
        f << token;
        rpnList.push_back(token);
      }
      else {
        int idx;
        if (b1&0x40)
          idx=int((b1&0x3f) | (input->readULong(1)<<6));
        else
          idx=int(b1);
        if (idx<0 || idx>=int(tokenList.size())) {
          STOFF_DEBUG_MSG(("StarCellFormula::readSCFormula: can not find the original token\n"));
          f << "[###Index" << idx << "]";
        }
        else {
          rpnList.push_back(tokenList[size_t(idx)]);
          f << rpnList.back() << "[Id" << idx << "]";
        }
      }
      if (input->tell()>lastPos) {
        f << "###";
        ok=false;
      }
    }
    f << "],";
  }
  if (ok && !formulaSet && rpnList.size()) {
    std::vector<std::vector<StarCellFormulaInternal::Token> > stack;
    for (auto const &rpn : rpnList) {
      if (!StarCellFormulaInternal::Token::addToken(stack, rpn)) {
        ok=false;
        break;
      }
    }
    if (ok && stack.size()==1) {
      hasIndex=false;
      std::vector<StarCellFormulaInternal::Token> &code=stack[0];
      for (auto &codeData : code) {
        STOFFCellContent::FormulaInstruction finalInstr;
        bool ignore;
        if (codeData.get(finalInstr,ignore) && !ignore)
          content.m_formula.push_back(finalInstr);
        else if (codeData.m_type==StarCellFormulaInternal::Token::Index)
          hasIndex=true;
      }
      if (hasIndex)
        content.m_formula.clear();
      else {
        content.m_contentType=STOFFCellContent::C_FORMULA;
        formulaSet=true;
      }
    }
#if 0
    else {
      std::cerr << "Bad=[\n";
      for (auto const &codeList : stack) {
        std::cerr << "\t";
        for (auto const &code : codeList)
          std::cerr << code;
        std::cerr << "\n";
      }
      std::cerr << "]\n";
    }
#endif
  }
  if (!formulaSet) {
    static bool first=true;
    if (first) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCFormula: can not reconstruct some formula\n"));
      first=false;
    }
    f << "###";
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok;
}

bool StarCellFormula::readSCFormula3(StarZone &zone, STOFFCellContent &content, int /*version*/, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();

  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(SCFormula)[" << zone.getRecordLevel() << "]:";
  bool ok=true;
  bool hasIndex=false;
  for (int tok=0; tok<512; ++tok) {
    bool endData;
    StarCellFormulaInternal::Token token;
    if (!readSCToken3(zone, token, endData, lastPos) || input->tell()>lastPos) {
      f << "###";
      ok=false;
      break;
    }
    if (endData) break;
    f << token;
    STOFFCellContent::FormulaInstruction finalInstr;
    bool ignore;
    if (token.get(finalInstr,ignore) && !ignore)
      content.m_formula.push_back(finalInstr);
    else if (token.m_type==StarCellFormulaInternal::Token::Index)
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

bool StarCellFormula::readSCToken(StarZone &zone, StarCellFormulaInternal::Token &token, int version, long lastPos)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugStream f;
  // sc_token.cxx ScRawToken::Load
  uint16_t nOp;
  uint8_t type;
  *input >> nOp >> type;
  bool ok=true;
  token.m_operation=nOp;
  // first read the data
  switch (type) {
  case 0: {
    token.m_type=StarCellFormulaInternal::Token::Long;
    token.m_longValue=long(input->readLong(1));
    break;
  }
  case 1: {
    double val;
    *input>>val;
    token.m_type=StarCellFormulaInternal::Token::Double;
    token.m_doubleValue=val;
    break;
  }
  case 2: // string
  case 8: // external
  default: { // ?
    if (type==8) {
      auto cByte=int(input->readULong(1));
      if (cByte)
        f << "cByte=" << cByte << ",";
      f << "external,";
    }
    else if (type!=2)
      f << "f" << type << ",";
    uint8_t nBytes;
    *input >> nBytes;
    if (input->tell()+int(nBytes)>lastPos) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read text zone\n"));
      f << "###text";
      ok=false;
      break;
    }
    token.m_type=type==2 ? StarCellFormulaInternal::Token::String :
                 type==8 ? StarCellFormulaInternal::Token::External : StarCellFormulaInternal::Token::String2;
    std::vector<uint8_t> text;
    for (int i=0; i<int(nBytes); ++i) text.push_back(static_cast<uint8_t>(input->readULong(1)));
    std::vector<uint32_t> string;
    std::vector<size_t> srcPositions;
    StarEncoding::convert(text, zone.getEncoding(), string, srcPositions);
    token.m_textValue=libstoff::getString(string);
    break;
  }
  case 3:
  case 4: {
    int16_t nCol, nRow, nTab;
    uint8_t nByte;
    *input >> nCol >> nRow >> nTab >> nByte;
    token.m_type=type==3 ? StarCellFormulaInternal::Token::Cell : StarCellFormulaInternal::Token::CellList;
    token.m_positions[0]=STOFFVec3i(nCol, nRow, nTab);
    if (version<0x10) {
      token.m_relPositions[0]=STOFFVec3b((nByte&3)!=0, ((nByte>>2)&3)!=0, ((nByte>>4)&3)!=0);
      if (nByte>>6) f << "fl=" << int(nByte>>6) << ",";
    }
    else {
      token.m_relPositions[0]=STOFFVec3b((nByte&1)!=0, ((nByte>>2)&1)!=0, ((nByte>>4)&1)!=0);
      if (nByte&0xEA) f << "fl=" << std::hex << int(nByte&0xEF) << std::dec << ",";
    }
    if (type==4) {
      *input >> nCol >> nRow >> nTab >> nByte;
      token.m_positions[1]=STOFFVec3i(nCol, nRow, nTab);
      if (nTab!=token.m_positions[0][2]) {
        STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: referencing different sheet is not implemented\n"));
        f << "#sheet2=" << nTab << ",";
      }
      if (version<0x10) {
        token.m_relPositions[1]=STOFFVec3b((nByte&3)!=0, ((nByte>>2)&3)!=0, ((nByte>>4)&3)!=0);
        if (nByte>>6) f << "fl2=" << int(nByte>>6) << ",";
      }
      else {
        token.m_relPositions[1]=STOFFVec3b((nByte&1)!=0, ((nByte>>2)&1)!=0, ((nByte>>4)&1)!=0);
        if (nByte&0xEA) f << "fl=" << std::hex << int(nByte&0xEA) << std::dec << ",";
      }
    }
    break;
  }
  case 6:
    token.m_type=StarCellFormulaInternal::Token::Index;
    token.m_longValue=long(input->readULong(2));
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
    token.m_type=StarCellFormulaInternal::Token::Jump;
    f << "J=[";
    for (int i=0; i<int(nByte); ++i) {
      token.m_jumpPositions.push_back(int(input->readLong(2)));
      f << token.m_jumpPositions.back() << ",";
    }
    f << "],";
    break;
  }
  case 0x70:
    token.m_type=StarCellFormulaInternal::Token::Missing;
    f << "#missing";
    break;
  case 0x71:
    token.m_type=StarCellFormulaInternal::Token::Error;
    f << "#error";
    break;
  }
  STOFFCellContent::FormulaInstruction &instr=token.m_instruction;
  switch (nOp) {
  case 0:
    break;
  case 2: // endData
    token.m_type=StarCellFormulaInternal::Token::Empty;
    break;
  case 3:
    if (type!=8) f << "##type=" << type << ",";
    break;
  case 4: // index
    if (type!=6) f << "##type=" << type << ",";
    break;
  case 5:
    if (type!=7) f << "##type=" << type << ",";
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="If";
    // normally associated with a cond If
    token.m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    token.m_longValue=-2;
    break;
  case 7:
    if (type) f << "##type=" << type << ",";
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="(";
    break;
  case 8:
    if (type) f << "##type=" << type << ",";
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    // normally in If with [cond id] form0 form1
    token.m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    token.m_longValue=-3;
    instr.m_content=")";
    break;
  case 9:
    if (type) f << "##type=" << type << ",";
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=";";
    // normally in If to separe form0 and form1
    token.m_content=StarCellFormulaInternal::Token::C_FunctionOperator;
    token.m_longValue=-1;
    break;
  case 11:
    STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: find bad opcode\n"));
    f << "#bad[opcode],";
    ok=false;
    break;
  case 12:
    // extra space
    if (type) f << "##type=" << type << ",";
    token.m_type=StarCellFormulaInternal::Token::Empty;
    break;
  case 17:
    if (type) f << "##type=" << type << ",";
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="%";
    break;
  default:
    if (type!=0) {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read a formula\n"));
      f << "##f" << nOp << "[" << type << "],";
      ok=false;
    }
    else if (token.updateFunction())
      token.m_type=StarCellFormulaInternal::Token::Function;
    else {
      STOFF_DEBUG_MSG(("StarCellFormula::readSCToken: can not read a formula\n"));
      f << "##f" << nOp << ",";
      ok=false;
    }
    break;
  }
  token.m_extra=f.str();

  return ok && input->tell()<=lastPos;
}

bool StarCellFormula::readSCToken3(StarZone &zone, StarCellFormulaInternal::Token &token, bool &endData, long lastPos)
{
  endData=false;
  STOFFInputStreamPtr input=zone.input();
  // sc_token.cxx ScRawToken::Load30
  uint16_t nOp;
  *input >> nOp;
  token.m_operation=nOp;
  bool ok=true;
  libstoff::DebugStream f;
  auto &instr=token.m_instruction;
  switch (nOp) {
  case 0: {
    uint8_t type;
    *input >> type;
    switch (type) {
    case 0:
      token.m_type=StarCellFormulaInternal::Token::Long;
      token.m_longValue=long(input->readLong(1));
      break;
    case 1: {
      double val;
      *input>>val;
      token.m_type=StarCellFormulaInternal::Token::Double;
      token.m_doubleValue=val;
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
      token.m_type=StarCellFormulaInternal::Token::String;
      token.m_textValue=libstoff::getString(text);
      break;
    }
    case 3: {
      int16_t nPos[3];
      uint8_t relPos[3], oldFlag;
      *input >> nPos[0] >> nPos[1] >> nPos[2] >> relPos[0] >> relPos[1] >> relPos[2] >> oldFlag;
      token.m_type=StarCellFormulaInternal::Token::Cell;
      token.m_positions[0]=STOFFVec3i(nPos[0], nPos[1], nPos[2]);
      token.m_relPositions[0]=STOFFVec3b(relPos[0],relPos[1],relPos[2]);
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
      for (int c=0; c<2; ++c) {
        token.m_type=StarCellFormulaInternal::Token::CellList;
        token.m_positions[c]=STOFFVec3i(nPos[c][0], nPos[c][1], nPos[c][2]);
        token.m_relPositions[c]=STOFFVec3b(relPos[c][0],relPos[c][1],relPos[c][2]);
      }
      if (nPos[0][2]!=nPos[1][2] || relPos[0][2]!=relPos[1][2]) {
        STOFF_DEBUG_MSG(("StarCellFormula::readSCToken3: referencing different sheet is not implemented\n"));
        f << "#sheet2,";
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
    token.m_type=StarCellFormulaInternal::Token::Empty;
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
    token.m_type=StarCellFormulaInternal::Token::External;
    token.m_textValue=libstoff::getString(text);
    break;
  }
  case 4: // name
    token.m_type=StarCellFormulaInternal::Token::Index;
    token.m_longValue=long(input->readULong(2));
    break;
  case 5: // jump 3
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="If";
    break;
  case 6: // jump=maxjumpcount
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Function;
    instr.m_content="Choose";
    break;
  case 7:
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="(";
    break;
  case 8:
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=")";
    break;
  case 9:
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content=";";
    break;
  case 17:
    token.m_type=StarCellFormulaInternal::Token::Function;
    instr.m_type=STOFFCellContent::FormulaInstruction::F_Operator;
    instr.m_content="%";
    break;
  default:
    if (token.updateFunction())
      token.m_type=StarCellFormulaInternal::Token::Function;
    else {
      ok=false;
      f << "###f" << nOp << ",";
    }
    break;
  }
  token.m_extra=f.str();
  return ok && input->tell()<=lastPos;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
