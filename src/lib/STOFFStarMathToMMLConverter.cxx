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

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <sstream>
#include <set>

#include "STOFFStarMathToMMLConverter.hxx"

//! namespace to define a basic lexer, parser
namespace STOFFStarMathToMMLConverterInternal
{
//! insenstive comparison
static bool icmp(const std::string &str1, const std::string &str2)
{
  if (str1.size() != str2.size())
    return false;
  for (std::string::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2) {
    if (std::tolower(*c1) != std::tolower(*c2))
      return false;
  }
  return true;
}
std::string toLower(std::string const &strToConvert)
{
  std::string res(strToConvert);
  std::transform(res.begin(), res.end(), res.begin(),
                 [](unsigned char c) -> unsigned char { return (unsigned char) std::tolower(c); });
  return res;
}
std::string toUpper(std::string const &strToConvert)
{
  std::string res(strToConvert);
  std::transform(res.begin(), res.end(), res.begin(),
                 [](unsigned char c) -> unsigned char { return (unsigned char) std::toupper(c); });
  return res;
}

//! a lexer data
struct LexerData {
  //! the different lexer type
  enum Type { Number, PlaceHolder, Special, Space, String, Unknown};
  //! constructor
  LexerData()
    : m_type(Unknown)
    , m_string()
  {
  }
  //! the main type
  Type m_type;
  //! the data
  std::string m_string;
};
//! a data node
struct Node {
  //! the node type
  enum Type { Root, Empty, Unknown,
              Sequence, SequenceRow,
              Relation, Addition, Multiplication, Position,
              Function, Accent, Integral,
              UnaryOperator,
              Parenthesis, ParenthesisLeft, ParenthesisRight,
              Special, String, Number
            };
  //! constructor
  explicit Node(Type type=Unknown, std::string const &spaces="")
    : m_type(type)
    , m_spaces(spaces)
    , m_function()
    , m_data()
    , m_childs()
  {
  }
  //! operator<<
  friend std::ostream &operator<<(std::ostream &o, Node const &nd)
  {
    if (!nd.m_function.empty()) o << nd.m_function << ",";
    if (!nd.m_data.empty()) o << nd.m_data << ",";
    if (!nd.m_spaces.empty()) o << nd.m_spaces << ",";
    if (!nd.m_childs.empty()) {
      o << "[";
      for (auto ch : nd.m_childs) {
        if (!ch)
          o << "_,";
        else
          o << "[" << *ch << "],";
      }
      o << "],";
    }
    return o;
  }
  //! the node type
  Type m_type;
  //! the different spaces
  std::string m_spaces;
  //! the function
  std::string m_function;
  //! the data
  std::string m_data;
  //! list of child
  std::vector<std::shared_ptr<Node> > m_childs;
};

//! class to parse a StarMath string
class Parser
{
public:
  //! constructor
  Parser()
    : m_dataList()
    , m_output()

    , m_fontSize(12)
    , m_bold(false)
    , m_italic(false)
    , m_fontName()
    , m_colorSet()
    , m_fontMap()
    , m_greekMap()
    , m_specialPercentMap()
    , m_otherSpecialMap()
    , m_parenthesisMap()
    , m_parenthesisRightSet()
    , m_parenthesisToStringMap()
    , m_unaryOperatorMap()
    , m_function1Set()
    , m_function2Set()
    , m_integralMap()
    , m_accent1Map()
    , m_accent1Set()
    , m_positionMap()
    , m_multiplicationMap()
    , m_multiplicationStringMap()
    , m_additionSet()
    , m_additionStringMap()
    , m_relationMap()
    , m_relationStringMap()
  {
    m_colorSet= {
      "black", "blue", "green", "red", "cyan", "magenta", "yellow", "gray", "lime",
      "maroon", "navy", "olive", "purple", "silver", "teal"
    };
    m_fontMap= {
      {"sans", "sans-serif"}, {"fixed", "monospace"}, {"serif", "normal"}
    };
    // % character
    m_greekMap= {
      {"alpha","&alpha;"}, {"beta","&beta;"}, {"chi","&chi;"}, {"delta","&delta;"}, {"epsilon","&epsilon;"},
      {"eta","&eta;"}, {"gamma","&gamma;"}, {"iota","&iota;"}, {"kappa","&kappa;"},
      {"lambda","&lambda;"}, {"mu","&mu;"}, {"nu","&nu;"}, {"omega","&omega;"},
      {"omicron","&omicron;"}, {"phi","&varphi;"}, {"pi","&pi;"}, {"psi","&psi;"},
      {"rho","&rho;"}, {"sigma","&sigma;"}, {"tau","&tau;"}, {"theta","&theta;"},
      {"upsilon","&upsilon;"}, {"varepsilon","&varepsilon;"}, {"varphi","&phi;"}, {"varpi","&varpi;"},
      {"varrho","&varrho;"}, {"varsigma","&varsigma;"}, {"vartheta","&vartheta;"}, {"xi","&xi;"},
      {"zeta","&zeta;"},
      {"ALPHA","&Alpha;"}, {"BETA","&Beta;"}, {"CHI","&Chi;"}, {"DELTA","&Delta;"},
      {"EPSILON","&Epsilon;"}, {"ETA","&Eta;"}, {"GAMMA","&Gamma;"}, {"IOTA","&Iota;"},
      {"KAPPA","&Kappa;"}, {"LAMBDA","&Lambda;"}, {"MU","&Mu;"}, {"NU","&Nu;"},
      {"OMEGA","&Omega;"}, {"OMICRON","&Omicron;"}, {"PHI","&Phi;"}, {"PI","&Pi;"},
      {"PSI","&Psi;"}, {"RHO","&Rho;"}, {"SIGMA","&Sigma;"}, {"TAU","&Tau;"},
      {"THETA","&Theta;"}, {"UPSILON","&Upsilon;"}, {"XI","&Xi;"}, {"ZETA","&Zeta;"}
    };
    m_specialPercentMap= {
      { "and", "&and;"},
      { "angle", "&#x2993;"},
      { "element", "&isin;"},
      { "identical", "&equiv;"},
      { "infinite", "&infin;"},
      { "noelement", "&notin;"},
      { "notequal", "&neq;"},
      { "or", "&or;"},
      { "perthousand", "&permil;"},
      { "strictlygreaterthan", "&gt;"},
      { "strictlylessthan", "&lt;"},
      { "tendto", "&rarr;"} // checkme
    };
    m_otherSpecialMap= {
      { "emptyset", "&emptyset;" },
      { "aleph", "&aleph;" },
      { "setn", "&naturals;" },
      { "setz", "&integers;" },
      { "setq", "&rationals;" },
      { "setr", "&reals;" },
      { "setc", "&complexes;" },
      { "infinity", "&#x221e;" },
      { "infty", "&#x221e;" },
      { "partial", "&#x2202;" },
      { "nabla", "&#x2207;" },
      { "exists", "&#x2203;" },
      { "notexists", "&#x2204;" },
      { "forall", "&#x2200;" },
      { "hbar", "&#x210f;" },
      { "lambdabar", "&#x19b;" },
      { "re", "&#x211c;" },
      { "im", "&#x2111;" },
      { "wp", "&#x2118;" },
      { "leftarrow", "&#x2190;" },
      { "rightarrow", "&#x2192;" },
      { "uparrow", "&#x2191;" },
      { "downarrow", "&#x2193;" },
      { "dotslow", "&#x2026;" },
      { "dotsaxis", "&#x22ef;" },
      { "dotsvert", "&#x22ee;" },
      { "dotsup", "&#x22f0;" },
      { "dotsdiag", "&#x22f0;" },
      { "dotsdown", "&#x22f1;" },
      { "backepsilon", "&#x3F6;" },
    };
    m_parenthesisMap= {
      { "(", ")"},
      { "[", "]"},
      { "ldbracket", "rdbracket"},
      { "lbrace", "rbrace"},
      { "langle", "rangle"},
      { "lceil", "rceil"},
      { "lfloor", "rfloor"},
      { "lline", "rline"},
      { "ldline", "rdline"}
      // and left right
      // and { }
    };
    for (auto it : m_parenthesisMap)
      m_parenthesisRightSet.insert(it.second);
    m_parenthesisToStringMap = {
      { "(", "("},
      { ")", ")"},
      { "[", "["},
      { "]", "]"},
      { "ldbracket", "&#x27e6;"},
      { "rdbracket", "&#x27e7;"},
      { "lbrace", "{"},
      { "rbrace", "}"},
      { "langle", "&#x2329;"},
      { "rangle", "&#x232a;"},
      { "lceil", "&#x2308;"},
      { "rceil", "&#x2309;"},
      { "lfloor", "&#x230a;"},
      { "rfloor", "&#x230b;"},
      { "lline", "|"},
      { "rline", "|"},
      { "ldline", "|"},
      { "rdline", "|"}
    };
    m_unaryOperatorMap= {
      { "+", "+"},
      { "-", "-"},
      { "plusminus", "&#xb1;"},
      { "minusplus", "&#x2213;"},
      { "+-", "&#xb1;"},
      {"-+", "&#x2213;"},
      { "neg", "&#xac;"},
      { "uoper", ""}
    };
    m_function1Set= {
      "abs", "fact", "sqrt", "ln",
      "exp", "log", "sin", "cos", "tan",
      "cot", "sinh", "cosh", "tanh", "coth",
      "arcsin", "arccos", "arctan", "arccot", "arsinh",
      "arcosh", "artanh", "arcoth",

      // also matrix stack
    };

    m_function2Set= {
      "nroot",
      "binom"
      // also func
    };

    // func 1-3 (ie. from, to optional)
    m_integralMap= {
      {"liminf", "liminf"},
      {"limsup", "limsup"},
      {"lim", "lim"},
      {"sum", "&#x2211;"},
      {"prod", "&#x220f;"},
      {"coprod", "&#x2210;"},
      {"int", "&#x222b;"},
      {"intd", "&#x222b;"},
      {"iint", "&#x222c;"},
      {"iiint", "&#x222d;"},
      {"lint", "&#x222e;"},
      {"llint", "&#x222f;"},
      {"lllint", "&#x2230;"}
      // also oper
    };

    m_accent1Map= {
      {"acute", "&#xb4;"}, {"grave", "`"}, {"breve", "&#x2d8;"}, {"circle", "&#x2da;"},
      {"dot", "&#x2d9;"}, {"ddot", "&#xa8;"}, {"dddot", "&#x20db;"}, {"bar", "&#xaf;"}, {"vec", "&#x20d7;"},
      {"tilde", "~"}, {"hat", "^"}, {"check", "&#x2c7;"},
      { "widevec", "&#x20d7;"}, {"widetilde", "~"}, {"widehat", "^"}
    };
    m_accent1Set= {
      "overline", "underline", "overstrike", "phantom",
      "nospace",

      "bold", "nbold", "ital", "italic", "nitalic",
      "alignl", "alignr", "alignc",
      "alignt", "alignm", "alignb"
      // size, font, color
    };
    m_positionMap= {
      { "lsub", 0},
      { "csub", 1},
      { "rsub", 2}, { "sub", 2}, {"_", 2},
      { "underbrace", 3},
      { "lsup", 4},
      { "csup", 5},
      { "rsup", 6}, { "sup", 6}, {"^", 6},
      { "overbrace", 7}
    };
    m_multiplicationMap= {
      {"*", "&#x2736;"},
      {"/", "/"},
      {"&", "&#x2227;"}
    };
    m_multiplicationStringMap= {
      {"cdot", "&#x22c5;"},
      {"times", "&#xd7;"},
      {"over", ""},
      {"div", "&#xf7;"},
      {"slash", "/"},
      {"wideslash", ""},
      {"widebslash", "&#x2216;"},
      {"and", "&#x2227;"},
      {"or", "&#x2228;"},
      {"bslash", "&#x2216;"},
      {"odivide", "&#x2298;"},
      {"odot", "&#x2299;"},
      {"ominus", "&#x2296;"},
      {"oplus", "&#x2295;"},
      {"otimes", "&#x2297;"}
      // "boper" oper <?>
    };
    m_additionSet= { "+", "-" };
    m_additionStringMap= {
      {"in", "&#x2208;"},
      {"notin", "&#x2209;"},
      {"owns", "&#x220b;"},
      {"intersection", "&#x2229;"},
      {"union", "&#x222a;"},
      {"setminus", "&#x2216;"},
      {"bslash", "&#x2216;"},
      {"slash", "/"},
      {"subset", "&#x2282;"},
      {"subseteq", "&#x2286;"},
      {"supset", "&#x2283;"},
      {"supseteq", "&#x2287;"},
      {"nsubset", "&#x2284;"},
      {"nsubseteq", "&#x2288;"},
      {"nsupset", "&#x2285;"},
      {"nsupseteq", "&#x2289;"},
      {"circ", "&#x2218;"}
    };
    m_relationMap= {
      {"=", "="},
      {"<>", "&#x2260;"},
      {"<", "&lt;"},
      {"<=", "&#x2264;"},
      {">", "&gt;"},
      {">=", "&#x2265;"},
      {"<<", "&#x226a;"},
      {">>", "&#x226b;"}
    };
    m_relationStringMap= {
      {"neq", "&#x2260;"},
      {"lt", "&lt;"},
      {"leslant", "&#x2264;"},
      {"gt", "&gt;"},
      {"geslant", "&#x2265;"},
      {"ll", "&#x226a;"},
      {"gg", "&#x226b;"},
      {"approx", "&#x2248;"},
      {"sim", "&#x223c;"},
      {"simeq", "&#x2243;"},
      {"equiv", "&#x2261;"},
      {"prop", "&#x221d;"},
      {"parallel", "&#x2225;"},
      {"ortho", "&#x22a5;"},
      {"divides", "&#x2223;"},
      {"ndivides", "&#x2224;"},
      {"toward", "&#x2192;"},
      {"dlarrow", "&#x21d0;"},
      {"dlrarrow", "&#x21d4;"},
      {"prec", "&#x227a;"},
      {"succ", "&#x227b;"},
      {"preccurlyeq", "&#x227c;"},
      {"succcurlyeq", "&#x227d;"},
      {"precsim", "&#x227e;"},
      {"succsim", "&#x227f;"},
      {"nprec", "&#x2280;"},
      {"nsucc", "&#x2281;"},
      {"def", "&#x225d;"},
      {"transl", "&#x22b7;"},
      {"transr", "&#x22b6;"}
    };

  }
  //! try to parse a StarMath string
  bool parse(librevenge::RVNGString const &formula, librevenge::RVNGString &res);
protected:
  /** convert a StarMath string in a list of lexer data.

      try to merge together the number zone, the placeholder <?>, the "XXX" string zone
      and some double symbols +-, -+, ##, <=, ...
  */
  bool convert(librevenge::RVNGString const &starMath, std::vector<LexerData> &lexList) const;

  /// html escape a string
  static std::string getEscapedString(std::string const &str)
  {
    if (str.empty())
      return str;
    return librevenge::RVNGString::escapeXML(str.c_str()).cstr();
  }
  /// try to convert a node in a starMML
  bool convertInMML(Node const &node, bool inRow=false);
  /// try to convert a position node in a starMML
  bool convertPositionOverbraceInMML(Node const &node);
  /// try to convert a position node in a starMML
  bool convertPositionUnderbraceInMML(Node const &node);
  /// try to convert a position node in a starMML
  bool convertPositionInMML(Node const &node);
  /// try to add the alignment
  void findAndAddAlignment(Node &node, bool &colFound, bool &rowFound);
  /// try to send the font style
  bool sendMathVariant();
  //! try to parse an expr: {newline|sequenceExpr}*
  std::shared_ptr<Node> expr() const;
  //! try to parse a sequence of expr
  std::shared_ptr<Node> sequenceExpr(size_t &pos, bool newLineIsBad=false, int stackOrMatrixType=0) const;
  //! try to parse an relation expr: additionExpr{[=,==,...]additionExpr}
  std::shared_ptr<Node> relationExpr(size_t &pos) const;
  //! try to parse an addition expr: multiplyExpr{[+,-,in,or,...]multiplyExpr}
  std::shared_ptr<Node> additionExpr(size_t &pos) const;
  //! try to parse an multiplication expr: positionExpr{[*,/,over,...]positionExpr}
  std::shared_ptr<Node> multiplicationExpr(size_t &pos) const;
  /** try to parse a position expr: functionExpr{[^,_,sub,sup]functionExpr}
      first child is sub, second child is sup
   */
  std::shared_ptr<Node> positionExpr(size_t &pos) const;
  //! try to parse a unary operator expr:
  std::shared_ptr<Node> unaryOperatorExpr(size_t &pos, bool inPosition=false) const;
  //! try to parse a function expr: blockExpr | unaryOpExpr
  std::shared_ptr<Node> functionExpr(size_t &pos, bool inPosition=false) const;
  //! try to parse a parenthesis expr:
  std::shared_ptr<Node> parenthesisExpr(size_t &pos) const;
  //! look for alone right parenthesis
  std::shared_ptr<Node> rightParenthesisExpr(size_t &pos, std::shared_ptr<Node> term) const;
  //! look for alone left parenthesis
  std::shared_ptr<Node> leftParenthesisExpr(size_t &pos, std::function<std::shared_ptr<Node>(size_t &)> function) const;
  //! try to parse a element expr:
  std::shared_ptr<Node> elementExpr(size_t &pos) const;
  //! ignore the following space
  void ignoreSpaces(size_t &pos) const
  {
    while (pos<m_dataList.size() && m_dataList[pos].m_type==LexerData::Space && m_dataList[pos].m_string==" ")
      ++pos;
  }
  //! ignore the following space
  void ignoreSpaces(size_t &pos, std::string &spaces) const
  {
    spaces.clear();
    while (pos<m_dataList.size() && m_dataList[pos].m_type==LexerData::Space) {
      if (m_dataList[pos].m_string!=" ")
        spaces+=m_dataList[pos].m_string;
      ++pos;
    }
  }
  //! the star math data
  std::vector<LexerData> m_dataList;
  //! the output stream
  std::stringstream m_output;
  //! the current font size
  double m_fontSize;
  //! a flag to know if we are in bold or not
  bool m_bold;
  //! a flag to know if we are in italic or not
  bool m_italic;
  //! the font name
  std::string m_fontName;
  //! the set of potential color
  std::set<std::string> m_colorSet;
  //! the font convert map
  std::map<std::string, std::string> m_fontMap;
  //! the greek convert map
  std::map<std::string, std::string> m_greekMap;
  //! the special percent convert map
  std::map<std::string, std::string> m_specialPercentMap;
  //! the remaining special key word
  std::map<std::string, std::string> m_otherSpecialMap;
  //! the parenthesis left/right map
  std::map<std::string, std::string> m_parenthesisMap;
  //! the parenthesis right map
  std::set<std::string> m_parenthesisRightSet;
  //! the parenthesis unicode map
  std::map<std::string, std::string> m_parenthesisToStringMap;
  //! the unary operator
  std::map<std::string, std::string> m_unaryOperatorMap;
  //! the function which have one argument
  std::set<std::string> m_function1Set;
  //! the function which have two argument
  std::set<std::string> m_function2Set;
  //! the function which have one to three argument (from to optional)
  std::map<std::string, std::string> m_integralMap;
  //! the accent which have one argument
  std::map<std::string, std::string> m_accent1Map;
  //! the accent which have one argument
  std::set<std::string> m_accent1Set;
  //! position symbol map
  std::map<std::string, int> m_positionMap;
  //! multiplication symbol operator
  std::map<std::string, std::string> m_multiplicationMap;
  //! multiplication symbol string operator
  std::map<std::string, std::string> m_multiplicationStringMap;
  //! addition symbol operator
  std::set<std::string> m_additionSet;
  //! addition symbol string operator
  std::map<std::string, std::string> m_additionStringMap;
  //! relation symbol operator
  std::map<std::string, std::string> m_relationMap;
  //! relation symbol string operator
  std::map<std::string, std::string> m_relationStringMap;
};

std::shared_ptr<Node> Parser::expr() const
{
  auto root=std::make_shared<Node>(Node::Root);
  size_t pos=0;
  std::shared_ptr<Node> actTerm;
  while (pos<m_dataList.size()) {
    auto actPos=pos;
    ignoreSpaces(pos);
    if (pos>=m_dataList.size())
      break;
    try {
      pos=actPos;
      auto child=sequenceExpr(pos, true);
      if (!child || pos==actPos)
        throw "parser::expr: no child";
      if (!actTerm)
        actTerm=child;
      else {
        if (actTerm->m_type!=Node::Sequence) {
          auto newTerm=std::make_shared<Node>(Node::Sequence);
          if (actTerm->m_type!=Node::Empty)
            newTerm->m_childs.push_back(actTerm);
          actTerm=newTerm;
        }
        if (child->m_type==Node::Sequence)
          actTerm->m_childs.insert(actTerm->m_childs.end(), child->m_childs.begin(), child->m_childs.end());
        else
          actTerm->m_childs.push_back(child);
      }
    }
    catch (...) {
      pos=actPos;
    }
    if (pos!=actPos)
      continue;
    std::string spaces;
    ignoreSpaces(pos, spaces);
    if (!actTerm)
      actTerm=std::make_shared<Node>(Node::Sequence, spaces);
    else if (actTerm->m_type!=Node::Sequence) {
      auto newTerm=std::make_shared<Node>(Node::Sequence, spaces);
      if (actTerm->m_type!=Node::Empty)
        newTerm->m_childs.push_back(actTerm);
      actTerm=newTerm;
    }
    auto data=m_dataList[pos++];
    if (data.m_type!=LexerData::String && icmp(data.m_string, "newline")) {
      root->m_childs.push_back(actTerm);
      actTerm.reset();
      continue;
    }
    STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::parser::exp: must convert %s[%d] in string\n", data.m_string.c_str(), int(pos-1)));
    auto node=std::make_shared<Node>(Node::String, spaces);
    node->m_data=data.m_string;
    actTerm->m_childs.push_back(node);
  }
  if (actTerm)
    root->m_childs.push_back(actTerm);
  if (root->m_childs.empty())
    return std::make_shared<Node>(Node::Empty);
  if (root->m_childs.size()==1)
    return root->m_childs[0];
  return root;
}

std::shared_ptr<Node> Parser::sequenceExpr(size_t &pos, bool newLineIsBad, int stackOrMatrixType) const
{
  auto seq=std::make_shared<Node>(Node::Sequence);
  while (pos<m_dataList.size()) {
    auto actPos=pos;
    std::string spaces;
    ignoreSpaces(pos,spaces);
    if (pos>=m_dataList.size()) break;
    auto data=m_dataList[pos];
    bool badString=false;
    if (data.m_type==LexerData::Unknown && icmp(data.m_string, "newline")) {
      if (newLineIsBad)
        break;
      badString=true;
    }
    if (data.m_type==LexerData::Special && data.m_string=="#") {
      if (stackOrMatrixType)
        break;
      badString=true;
    }
    if (data.m_type==LexerData::Special && data.m_string=="##") {
      if (stackOrMatrixType>1)
        break;
      badString=true;
    }
    if (badString) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::parser::sequenceExp: must convert %s[%d] in string\n", data.m_string.c_str(), int(pos-1)));
      ++pos;
      auto child=std::make_shared<Node>(Node::String, spaces);
      child->m_data=data.m_string;
      seq->m_childs.push_back(child);
      continue;
    }
    bool ok=true;
    try {
      pos=actPos;
      auto child=relationExpr(pos);
      if (!child)
        ok=false;
      else if (child->m_type!=Node::Empty)
        seq->m_childs.push_back(child);
    }
    catch (...) {
      ok=false;
    }
    if (!ok) {
      pos=actPos;
      break;
    }
  }
  if (seq->m_childs.empty())
    return std::make_shared<Node>(Node::Empty);
  if (seq->m_childs.size()==1)
    return seq->m_childs[0];
  return seq;
}

std::shared_ptr<Node> Parser::relationExpr(size_t &pos) const
{
  if (pos>=m_dataList.size())
    throw "Parser::relationExpr: no data";
  auto term=additionExpr(pos);
  if (!term)
    throw "Parser::relationExpr: no data";
  std::vector<std::shared_ptr<Node> > listTerm;
  std::vector<std::string> listOp;
  std::vector<std::string> listSpaces;
  listTerm.push_back(term);
  while (true) {
    auto actPos=pos;
    std::string spaces;
    ignoreSpaces(pos, spaces);
    if (pos>=m_dataList.size()) break;
    auto const &data=m_dataList[pos];
    if ((data.m_type==LexerData::Special && m_relationMap.find(data.m_string)!=m_relationMap.end()) ||
        (data.m_type==LexerData::Unknown && m_relationStringMap.find(toLower(data.m_string))!=m_relationStringMap.end())) {
      try {
        ++pos;
        term=additionExpr(pos);
        if (term) {
          listTerm.push_back(term);
          listOp.push_back(data.m_string);
          listSpaces.push_back(spaces);
          continue;
        }
      }
      catch (...) {
      }
    }
    pos=actPos;
    break;
  }
  if (listTerm.size()==1)
    return listTerm[0];
  term=listTerm.back();
  for (size_t c=listTerm.size()-1; c>0; --c) {
    auto newTerm=std::make_shared<Node>(Node::Relation, listSpaces[c-1]);
    newTerm->m_function=listOp[c-1];
    newTerm->m_childs.push_back(listTerm[c-1]);
    newTerm->m_childs.push_back(term);
    term=newTerm;
  }
  return term;
}

std::shared_ptr<Node> Parser::additionExpr(size_t &pos) const
{
  if (pos>=m_dataList.size())
    throw "Parser::additionExpr: no data";
  auto term=multiplicationExpr(pos);
  if (!term)
    throw "Parser::additionExpr: no data";
  while (true) {
    auto actPos=pos;
    std::string spaces;
    ignoreSpaces(pos, spaces);
    if (pos>=m_dataList.size()) break;
    auto const &data=m_dataList[pos];
    if ((data.m_type==LexerData::Special && m_additionSet.find(data.m_string)!=m_additionSet.end()) ||
        (data.m_type==LexerData::Unknown && m_additionStringMap.find(toLower(data.m_string))!=m_additionStringMap.end())) {
      try {
        ++pos;
        auto newChild=multiplicationExpr(pos);
        if (newChild) {
          auto newTerm=std::make_shared<Node>(Node::Addition, spaces);
          newTerm->m_function=data.m_string;
          newTerm->m_childs.push_back(term);
          newTerm->m_childs.push_back(newChild);
          term=newTerm;
          continue;
        }
      }
      catch (...) {
      }
    }
    pos=actPos;
    break;
  }
  return term;
}

std::shared_ptr<Node> Parser::multiplicationExpr(size_t &position) const
{
  auto actPos=position;
  auto lExpr=leftParenthesisExpr(position, [this](size_t &pos) {
    return this->multiplicationExpr(pos);
  });
  if (lExpr) return lExpr;
  position=actPos;
  auto term=positionExpr(position);
  if (!term)
    throw "Parser::multiplicationExpr: no data";
  while (true) {
    actPos=position;
    std::string spaces;
    ignoreSpaces(position, spaces);
    if (position+1>=m_dataList.size()) break;
    auto const &cData=m_dataList[position];
    if ((cData.m_type==LexerData::Special && m_multiplicationMap.find(cData.m_string)!=m_multiplicationMap.end()) ||
        (cData.m_type==LexerData::Unknown && m_multiplicationStringMap.find(toLower(cData.m_string))!=m_multiplicationStringMap.end())) {
      try {
        ++position;
        auto newChild=positionExpr(position);
        if (newChild) {
          auto newTerm=std::make_shared<Node>(Node::Multiplication, spaces);
          newTerm->m_function=cData.m_string;
          newTerm->m_childs.push_back(term);
          newTerm->m_childs.push_back(newChild);
          term=newTerm;
          continue;
        }
      }
      catch (...) {
      }
    }
    else if (cData.m_type==LexerData::Unknown && icmp(cData.m_string,"boper") && position+2<=m_dataList.size()) {
      ++position;
      ignoreSpaces(position);
      if (position+1<=m_dataList.size() && !m_dataList[position].m_string.empty()) {
        auto oper=m_dataList[position].m_string;
        try {
          ++position;
          auto newChild=positionExpr(position);
          if (newChild) {
            auto newTerm=std::make_shared<Node>(Node::Multiplication, spaces);
            newTerm->m_function=cData.m_string;
            newTerm->m_data=oper;
            newTerm->m_childs.push_back(term);
            newTerm->m_childs.push_back(newChild);
            term=newTerm;
            continue;
          }
        }
        catch (...) {
        }
      }
    }
    position=actPos;
    break;
  }
  return rightParenthesisExpr(position,term);
}

std::shared_ptr<Node> Parser::positionExpr(size_t &pos) const
{
  if (pos>=m_dataList.size())
    throw "Parser::positionExpr: no data";
  auto term=unaryOperatorExpr(pos);
  if (!term)
    throw "Parser::positionExpr: no data";
  size_t const numPositions=8;
  std::shared_ptr<Node> nodes[numPositions];
  bool findSome=false;
  while (true) {
    auto actPos=pos;
    std::string spaces;
    ignoreSpaces(pos, spaces);
    if (pos+1>=m_dataList.size()) break;
    auto const &data=m_dataList[pos];
    if (m_positionMap.find(data.m_string)==m_positionMap.end() ||
        (std::isalpha(data.m_string[0]) && data.m_type!=LexerData::Unknown) ||
        (!std::isalpha(data.m_string[0]) && data.m_type!=LexerData::Special)) {
      pos=actPos;
      break;
    }
    try {
      auto id=m_positionMap.find(data.m_string)->second;
      if (nodes[id])
        break;
      ++pos;
      ignoreSpaces(pos, spaces);
      if (!spaces.empty()) { // space a_~1 a_'1 seems to mean {a_~} 1 ...
        nodes[id]=std::make_shared<Node>(Node::Empty, spaces);
        findSome=true;
        continue;
      }
      auto newChild=unaryOperatorExpr(pos, true);
      if (!newChild)
        break;
      if ((newChild->m_type==Node::Unknown && !newChild->m_data.empty() && std::isalpha(newChild->m_data[0]))
          || newChild->m_type==Node::Number) {
        // special case a_1n2+2 means a_{1n2}+2...
        size_t cPos;
        std::shared_ptr<Node> seq;
        while (true) {
          cPos=pos;
          ignoreSpaces(pos, spaces);
          if (pos!=cPos || pos+1>=m_dataList.size())
            break;
          try {
            auto sibling=unaryOperatorExpr(pos, true);
            if (pos==cPos || !sibling)
              break;
            if (sibling->m_type==Node::Unknown) {
              if (sibling->m_data.empty() || !std::isalpha(sibling->m_data[0]))
                break;
            }
            else if (sibling->m_type!=Node::Number)
              break;
            if (!seq) {
              seq=std::make_shared<Node>(Node::Sequence);
              seq->m_childs.push_back(newChild);
            }
            seq->m_childs.push_back(sibling);
            continue;
          }
          catch (...) {
          }
          break;
        }
        pos=cPos;
        if (seq) newChild=seq;
      }
      nodes[id]=newChild;
      findSome=true;
      continue;
    }
    catch (...) {
    }
    pos=actPos;
    break;
  }
  if (!findSome)
    return term;
  auto mainNode=std::make_shared<Node>(Node::Position);
  mainNode->m_childs.resize(numPositions+1);
  mainNode->m_childs[0]=term;
  for (size_t i=0; i<numPositions; ++i)
    mainNode->m_childs[i+1]=nodes[i];
  return mainNode;
}

std::shared_ptr<Node> Parser::unaryOperatorExpr(size_t &pos, bool inPosition) const
{
  auto actPos=pos;
  std::string spaces;
  ignoreSpaces(pos, spaces);
  if (pos>=m_dataList.size())
    throw "Parser::unaryOperatorExpr: no data";
  auto data=m_dataList[pos];
  if (data.m_type!=LexerData::String && m_unaryOperatorMap.find(toLower(data.m_string))!=m_unaryOperatorMap.end()) {
    ++pos;
    auto term=unaryOperatorExpr(pos, inPosition);
    if (!term)
      throw "Parser::unaryOperatorExpr: no unary data";
    if (data.m_string=="-" && term->m_type==Node::Number && !term->m_data.empty() && term->m_data[0]!='-') {
      term->m_spaces = spaces;
      term->m_data.insert(0,1,'-');
      return term;
    }
    auto res=std::make_shared<Node>(Node::UnaryOperator, spaces);
    res->m_function=data.m_string;
    res->m_childs.push_back(term);
    return res;
  }
  pos=actPos;
  return functionExpr(pos, inPosition);
}

std::shared_ptr<Node> Parser::functionExpr(size_t &pos, bool inPosition) const
{
  auto actPos=pos;
  std::string spaces;
  ignoreSpaces(pos, spaces);
  if (pos>=m_dataList.size())
    throw "Parser::functionExpr: no data";
  auto const &data=m_dataList[pos];
  size_t numArg=0;
  auto type=Node::Function;
  bool isFunc=false;
  bool specialAccent=false;
  int stackMatrixType=0;
  if (data.m_type==LexerData::Unknown) {
    if (m_function1Set.find(toLower(data.m_string))!=m_function1Set.end())
      numArg=1;
    else if (m_function2Set.find(toLower(data.m_string))!=m_function2Set.end())
      numArg=2;
    else if (icmp(data.m_string,"func")) {
      isFunc=true;
      numArg=1;
    }
    else if (m_accent1Map.find(toLower(data.m_string))!=m_accent1Map.end() ||
             m_accent1Set.find(toLower(data.m_string))!=m_accent1Set.end()) {
      type=Node::Accent;
      numArg=1;
    }
    else if (icmp(data.m_string,"font") || icmp(data.m_string,"color") || icmp(data.m_string,"size")) {
      type=Node::Accent;
      numArg=1;
      specialAccent=true;
    }
    else if (m_integralMap.find(toLower(data.m_string))!=m_integralMap.end()) {
      type=Node::Integral;
      numArg=1;
    }
    else if (icmp(data.m_string,"oper")) {
      type=Node::Integral;
      numArg=1;
    }
    else if (icmp(data.m_string,"stack")) {
      stackMatrixType=1;
      numArg=1;
    }
    else if (icmp(data.m_string,"matrix")) {
      stackMatrixType=2;
      numArg=1;
    }
  }
  if (numArg==0 || pos+1+numArg>=m_dataList.size()) {
    pos=actPos;
    return parenthesisExpr(pos);
  }
  pos++;
  auto funcNode=std::make_shared<Node>(type, spaces);
  funcNode->m_function=data.m_string;
  auto cPos=pos;
  ignoreSpaces(pos);
  bool ok=pos+numArg<=m_dataList.size();
  if (inPosition && type==Node::Function && !stackMatrixType) {
    // special case f_sin or f_func sin is ok
    numArg=0;
    type=Node::String;
    ok=pos < m_dataList.size();
  }
  if (isFunc && ok) { // special case func XXX
    if (inPosition)
      funcNode->m_function=m_dataList[pos++].m_string;
    else
      funcNode->m_data=m_dataList[pos++].m_string;
    numArg=0;
  }
  else if (specialAccent && ok) {
    if (icmp(data.m_string,"size")) {
      auto actData=m_dataList[pos++];
      if (actData.m_type==LexerData::Number)
        funcNode->m_data=actData.m_string;
      else if (actData.m_type==LexerData::Special &&
               (actData.m_string=="+" || actData.m_string=="-" || actData.m_string=="*" || actData.m_string=="/")) {
        ignoreSpaces(pos);
        if (pos+1<m_dataList.size() && m_dataList[pos].m_type==LexerData::Number)
          funcNode->m_data=actData.m_string+m_dataList[pos++].m_string;
        else
          ok=false;
      }
      else
        ok=false;
    }
    else
      funcNode->m_data=m_dataList[pos++].m_string;
  }
  else if (type==Node::Integral && ok) {
    if (icmp(data.m_string,"oper"))
      funcNode->m_data=m_dataList[pos++].m_string;
    std::shared_ptr<Node> minMax[2];
    for (int i=0; i<2; ++i) {
      ignoreSpaces(pos);
      if (pos+1>=m_dataList.size() || m_dataList[pos].m_type!=LexerData::Unknown)
        break;
      bool isMin=false;
      if (!minMax[0] && icmp(m_dataList[pos].m_string, "from"))
        isMin=true;
      else if (minMax[1] || !icmp(m_dataList[pos].m_string, "to"))
        break;
      try {
        ++pos;
        auto newChild=relationExpr(pos);
        if (newChild) {
          minMax[isMin ? 0 : 1]=newChild;
          continue;
        }
      }
      catch (...) {
      }
      ok=false;
      break;
    }
    for (auto n : minMax)
      funcNode->m_childs.push_back(n);
  }
  else if (stackMatrixType) { // space case must be { data }
    ignoreSpaces(pos);
    if (pos+2>=m_dataList.size() || m_dataList[pos].m_type!=LexerData::Special || m_dataList[pos].m_string!="{")
      ok=false;
    if (ok) {
      ++pos;
      auto actNode=funcNode;
      if (stackMatrixType==2)
        actNode=std::make_shared<Node>(Node::SequenceRow);
      while (pos+1<m_dataList.size()) {
        try {
          auto term=sequenceExpr(pos, false, stackMatrixType);
          actNode->m_childs.push_back(term);
        }
        catch (...) {
        }
        ignoreSpaces(pos);
        if (pos<m_dataList.size() && m_dataList[pos].m_type==LexerData::Special) {
          if (m_dataList[pos].m_string=="#") {
            ++pos;
            continue;
          }
          else if (stackMatrixType==2 && m_dataList[pos].m_string=="##") {
            ++pos;
            funcNode->m_childs.push_back(actNode);
            actNode=std::make_shared<Node>(Node::SequenceRow);
            continue;
          }
        }
        break;
      }
      if (stackMatrixType==2)
        funcNode->m_childs.push_back(actNode);
      if (pos>=m_dataList.size() || m_dataList[pos].m_type!=LexerData::Special || m_dataList[pos].m_string!="}")
        ok=false;
      ++pos;
    }
    numArg=0;
  }
  else
    pos=cPos;
  for (size_t arg=0; ok && arg<numArg; ++arg) {
    try {
      auto newChild=type==Node::Accent ? unaryOperatorExpr(pos, inPosition) : relationExpr(pos);
      if (newChild) {
        funcNode->m_childs.push_back(newChild);
        continue;
      }
    }
    catch (...) {
    }
    ok=false;
    break;
  }
  if (!ok)
    throw "Parser::functionExpr: can not read function argument";
  return funcNode;
}

std::shared_ptr<Node> Parser::parenthesisExpr(size_t &position) const
{
  auto actPos=position;
  auto lExpr=leftParenthesisExpr(position, [this](size_t &pos) {
    return this->parenthesisExpr(pos);
  });
  if (lExpr) return lExpr;
  position=actPos;
  std::string spaces;
  ignoreSpaces(position, spaces);
  if (position>=m_dataList.size())
    throw "Parser::parenthesisExpr: no data";
  auto data=m_dataList[position];
  if (data.m_type!=LexerData::String &&
      (m_parenthesisMap.find(toLower(data.m_string))!=m_parenthesisMap.end() ||
       data.m_string=="{" || icmp(data.m_string,"left"))) {
    ++position;
    try {
      auto node=std::make_shared<Node>(Node::Parenthesis, spaces);
      bool leftRight=icmp(data.m_string,"left");
      node->m_function=data.m_string;
      if (leftRight) {
        ignoreSpaces(position);
        if (position>=m_dataList.size())
          throw "Parser::parenthesisExpr: no left parenthesis";
        node->m_function=m_dataList[position++].m_string;
        node->m_data="right";
      }
      else if (data.m_string=="{")
        node->m_data="}";
      else
        node->m_data=m_parenthesisMap.find(toLower(data.m_string))->second;

      auto newChild=sequenceExpr(position);
      if (newChild) {
        node->m_childs.push_back(newChild);
        ignoreSpaces(position, spaces);
        if (position>=m_dataList.size())
          throw "Parser::parenthesisExpr: can not find right parenthesis";
        auto parData=m_dataList[position];
        if (parData.m_type==LexerData::String || !icmp(parData.m_string, node->m_data))
          throw "Parser::parenthesisExpr: unexpected parenthesis";
        if (!spaces.empty()) {
          if (newChild->m_type!=Node::Sequence) {
            auto newRoot=std::make_shared<Node>(Node::Sequence);
            newRoot->m_childs.push_back(newChild);
            newChild=newRoot;
          }
          newChild->m_childs.push_back(std::make_shared<Node>(Node::Empty, spaces));
        }
        ++position;
        if (leftRight) {
          ignoreSpaces(position);
          if (position>=m_dataList.size())
            throw "Parser::parenthesisExpr: no right parenthesis";
          node->m_data=m_dataList[position++].m_string;
        }
        else if (data.m_string=="{")
          node->m_function=node->m_data="";
        return node;
      }
    }
    catch (...) {
    }
    throw "Parser::parenthesisExpr: can not read a parenthesis block";
  }
  position=actPos;
  auto term=elementExpr(position);
  if (!term)
    throw "Parser::parenthesisExpr: no data";
  return rightParenthesisExpr(position, term);
}

std::shared_ptr<Node> Parser::leftParenthesisExpr(size_t &pos, std::function<std::shared_ptr<Node>(size_t &)> function) const
{
  std::string spaces;
  ignoreSpaces(pos, spaces);
  if (pos>=m_dataList.size())
    throw "Parser::leftParenthesisExpr: no data";
  auto data=m_dataList[pos];
  if (data.m_type!=LexerData::String && data.m_string.size()>=2 && data.m_string[0]=='\\' &&
      m_parenthesisMap.find(toLower(data.m_string.c_str()+1))!=m_parenthesisMap.end()) {
    auto node=std::make_shared<Node>(Node::ParenthesisLeft, spaces);
    try {
      ++pos;
      auto newChild=function(pos);
      if (newChild) {
        node->m_function=data.m_string.c_str()+1;
        node->m_childs.push_back(newChild);
        return node;
      }
    }
    catch (...) {
    }
    throw "Parser::leftParenthesisExpr: left parenthesis is alone";
  }
  else if (data.m_type!=LexerData::String &&
           (m_parenthesisRightSet.find(toLower(data.m_string))!=m_parenthesisRightSet.end() ||
            data.m_string=="}" || icmp(data.m_string,"right")))
    throw "Parser::leftParenthesisExpr: right parenthesis";
  return std::shared_ptr<Node>();
}

std::shared_ptr<Node> Parser::rightParenthesisExpr(size_t &pos, std::shared_ptr<Node> term) const
{
  // look for right parenthesis
  while (true) {
    auto actPos=pos;
    std::string spaces;
    ignoreSpaces(pos,spaces);
    if (pos>=m_dataList.size())
      break;
    auto data=m_dataList[pos];
    if (data.m_type!=LexerData::String && data.m_string.size()>=2 && data.m_string[0]=='\\' &&
        m_parenthesisRightSet.find(toLower(data.m_string.c_str()+1))!=m_parenthesisRightSet.end()) {
      ++pos;
      auto node=std::make_shared<Node>(Node::ParenthesisRight, spaces);
      node->m_function=data.m_string.c_str()+1;
      node->m_childs.push_back(term);
      term=node;
      continue;
    }
    pos=actPos;
    break;
  }
  return term;
}

std::shared_ptr<Node> Parser::elementExpr(size_t &pos) const
{
  std::string spaces;
  ignoreSpaces(pos, spaces);
  if (pos>=m_dataList.size())
    throw "Parser::elementExpr: no data";
  auto data=m_dataList[pos++];
  auto len=data.m_string.size();
  bool special=false;
  if (len>1 && data.m_type==LexerData::Unknown && data.m_string[0]=='%') {
    special = m_greekMap.find(data.m_string.c_str()+1)!=m_greekMap.end() ||
              m_specialPercentMap.find(toLower(data.m_string.c_str()+1))!=m_specialPercentMap.end() ||
              (len>2 && data.m_string[1]=='i' && m_greekMap.find(data.m_string.c_str()+2)!=m_greekMap.end());
  }
  else if (len>=1 && data.m_type!=LexerData::String)
    special = m_otherSpecialMap.find(toLower(data.m_string))!=m_otherSpecialMap.end();
  auto str=std::make_shared<Node>(special ? Node::Special :
                                  data.m_type==LexerData::Number ? Node::Number :
                                  data.m_type==LexerData::String ? Node::String : Node::Unknown, spaces);
  str->m_data=data.m_string;
  return str;
}

bool Parser::sendMathVariant()
{
  std::string newVariant;
  if (m_fontName.empty() || m_fontName=="normal")
    newVariant = m_italic ? (m_bold ? "bold-italic" : "italic") : m_bold ? "bold" : "normal";
  else if (m_fontName=="sans-serif")
    newVariant = m_italic ? (m_bold ? "sans-serif-bold-italic" : "sans-serif-italic") : m_bold ? "bold-sans-serif" : "sans-serif";
  else if (m_fontName=="monospace")
    newVariant="monospace";
  if (newVariant.empty())
    return false;
  m_output << "<mstyle mathvariant=\"" << newVariant << "\">";
  return true;
}

void Parser::findAndAddAlignment(Node &node, bool &colFound, bool &rowFound)
{
  bool lookInChild=node.m_type==Node::Root || node.m_type==Node::Sequence ||
                   (node.m_type==Node::Parenthesis && node.m_function.empty() && node.m_data.empty());
  // can also appear in size ....
  if (node.m_type==Node::Accent && icmp(node.m_function.substr(0,5),"align")) {
    bool done=false;
    if (!colFound) {
      done = true;
      if (icmp(node.m_function,"alignl"))
        m_output << " columnalign=\"left\"";
      else if (icmp(node.m_function,"alignc"))
        ;
      else if (icmp(node.m_function,"alignr"))
        m_output << " columnalign=\"right\"";
      else
        done = false;
      if (done)
        colFound=true;
    }
    if (!done && !rowFound) {
      done = true;
      if (icmp(node.m_function,"alignt"))
        m_output << " rowalign=\"top\"";
      else if (icmp(node.m_function,"alignm"))
        m_output << " rowalign=\"center\"";
      else if (icmp(node.m_function,"alignb"))
        m_output << " rowalign=\"bottom\"";
      else
        done = false;
      if (done)
        rowFound=true;
    }
    if (done) node.m_function.clear();
    lookInChild=true;
  }
  if (lookInChild && (!colFound || !rowFound)) {
    for (auto c : node.m_childs) {
      if (c)
        findAndAddAlignment(*c, colFound, rowFound);
    }
  }
}

bool Parser::convertInMML(Node const &node, bool addRow)
{
  if (addRow) m_output << "<mrow>";
  for (auto c : node.m_spaces) {
    if (c=='`')
      m_output << "<mspace width=\"2px\"/>";
    else if (c=='~')
      m_output << "<mspace width=\"4px\"/>";
  }
  bool childDone=false;
  switch (node.m_type) {
  case Node::Empty:
    break;
  case Node::Number:
    if (!node.m_data.empty())
      m_output << "<mn>" << getEscapedString(node.m_data) << "</mn>";
    break;
  case Node::String:
    if (!node.m_data.empty())
      m_output << "<mtext>" << getEscapedString(node.m_data) << "</mtext>";
    break;
  case Node::Unknown:
    if (icmp(node.m_data, "`"))
      m_output << "<mspace width=\"2px\"/>";
    else if (icmp(node.m_data, "~"))
      m_output << "<mspace width=\"4px\"/>";
    else if (!node.m_data.empty())
      m_output << "<mi>" << getEscapedString(node.m_data) << "</mi>";
    break;
  case Node::Special: {
    if (node.m_data.empty()) break;
    auto len=node.m_data.size();
    bool done=true;
    if (len>1 && node.m_data[0]=='%') {
      if (m_greekMap.find(node.m_data.c_str()+1)!=m_greekMap.end())
        m_output << "<mi>" << m_greekMap.find(node.m_data.c_str()+1)->second << "</mi>";
      else if (len>2 && node.m_data[1]=='i' && m_greekMap.find(node.m_data.c_str()+2)!=m_greekMap.end())
        m_output << "<mi mathvariant='italic'>" << m_greekMap.find(node.m_data.c_str()+2)->second << "</mi>";
      else if (m_specialPercentMap.find(toLower(node.m_data.c_str()+1))!=m_specialPercentMap.end())
        m_output << "<mi>" << m_specialPercentMap.find(toLower(node.m_data.c_str()+1))->second << "</mi>";
      else
        done=false;
    }
    else if (len>0 && m_otherSpecialMap.find(toLower(node.m_data))!=m_otherSpecialMap.end())
      m_output << "<mi>" << m_otherSpecialMap.find(toLower(node.m_data))->second << "</mi>";
    else
      done=false;
    if (done)
      break;
    STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: must convert %s in string\n", node.m_data.c_str()));
    m_output << "<mi>" << getEscapedString(node.m_data) << "</mi>";
    break;
  }
  case Node::Accent: {
    if (node.m_childs.size()!=1 || !node.m_childs[0]) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the accent child\n"));
      break;
    }
    childDone=true;
    if (m_accent1Map.find(toLower(node.m_function)) != m_accent1Map.end()) {
      m_output << "<mover accent=\"true\">";
      convertInMML(*node.m_childs[0], true);
      if (icmp(node.m_function.substr(0,4), "wide"))
        m_output << "<mo stretchy=\"true\">" << m_accent1Map.find(toLower(node.m_function))->second << "</mo>";
      else
        m_output << "<mo stretchy=\"false\">" << m_accent1Map.find(toLower(node.m_function))->second << "</mo>";
      m_output << "</mover>";
      break;
    }
    else if (icmp(node.m_function,"overline")) {
      m_output << "<mover accent=\"true\">";
      convertInMML(*node.m_childs[0], true);
      m_output << "<mo>&#xaf;</mo>";
      m_output << "</mover>";
    }
    else if (icmp(node.m_function,"underline")) {
      m_output << "<munder accentunder=\"true\">";
      convertInMML(*node.m_childs[0], true);
      m_output << "<mo>&#xaf;</mo>";
      m_output << "</munder>";
    }
    else if (icmp(node.m_function,"overstrike")) {
      m_output << "<menclose notation=\"horizontalstrike\">";
      convertInMML(*node.m_childs[0]);
      m_output << "</menclose>";
    }
    else if (icmp(node.m_function,"phantom")) {
      m_output << "<mphantom>";
      convertInMML(*node.m_childs[0]);
      m_output << "</mphantom>";
    }
    else if (icmp(node.m_function,"nospace")) {
      childDone=false;
      break;
    }
    else if (icmp(node.m_function,"color")) {
      if (m_colorSet.find(toLower(node.m_data))!=m_colorSet.end()) {
        m_output << "<mstyle color=\"" << toLower(node.m_data) << "\">";
        convertInMML(*node.m_childs[0]);
        m_output << "</mstyle>";
      }
      else {
        STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: unknown color %s\n", node.m_data.c_str()));
        childDone=false;
      }
    }
    else if (icmp(node.m_function,"bold") || icmp(node.m_function,"nbold")) {
      auto oldBold=m_bold;
      m_bold=!icmp(node.m_function,"nbold");
      bool closeStyle=sendMathVariant();
      convertInMML(*node.m_childs[0]);
      if (closeStyle) m_output << "</mstyle>";
      m_bold=oldBold;
    }
    else if (icmp(node.m_function,"ital") || icmp(node.m_function,"italic") || icmp(node.m_function,"nitalic")) {
      auto oldItalic=m_italic;
      m_italic=!icmp(node.m_function,"nitalic");
      bool closeStyle=sendMathVariant();
      convertInMML(*node.m_childs[0]);
      if (closeStyle) m_output << "</mstyle>";
      m_italic=oldItalic;
    }
    else if (icmp(node.m_function,"font")) {
      auto oldName=m_fontName;
      if (m_fontMap.find(toLower(node.m_data))!=m_fontMap.end()) {
        m_fontName=m_fontMap.find(toLower(node.m_data))->second;
        bool closeStyle=sendMathVariant();
        convertInMML(*node.m_childs[0]);
        if (closeStyle) m_output << "</mstyle>";
      }
      else {
        STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: unknown font %s\n", node.m_data.c_str()));
        childDone=false;
      }
      m_fontName=oldName;
    }
    else if (icmp(node.m_function,"size")) {
      auto oldSize=m_fontSize;
      std::string op="";
      std::stringstream s;
      if (!node.m_data.empty() && !std::isdigit(node.m_data[0])) {
        op=node.m_data[0];
        s << (node.m_data.c_str()+1);
      }
      else
        s << node.m_data;
      double newSize=0.;
      bool ok=false;
      if (s >> newSize) ok=true;
      if (ok) {
        if (op=="+") {
          m_fontSize+=newSize;
          m_output << "<mstyle mathsize=\"" << m_fontSize << "pt\">";
        }
        else if (op=="-") {
          m_fontSize-=newSize;
          m_output << "<mstyle mathsize=\"" << m_fontSize << "pt\">";
        }
        else if (op=="*") {
          m_fontSize*=newSize;
          m_output << "<mstyle mathsize=\"" << 100*newSize << "%\">";
        }
        else if (op=="/" && newSize>0) {
          m_fontSize/=newSize;
          m_output << "<mstyle mathsize=\"" << 100/newSize << "%\">";
        }
        else if (op.empty() && newSize>0) {
          m_fontSize=newSize;
          m_output << "<mstyle mathsize=\"" << m_fontSize << "pt\">";
        }
        else
          ok=false;
      }
      if (ok) {
        convertInMML(*node.m_childs[0]);
        m_output << "</mstyle>";
      }
      else {
        convertInMML(*node.m_childs[0]);
        STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: unknown font size %s\n", node.m_data.c_str()));
      }
      m_fontSize=oldSize;
      childDone=true;
    }
    else {
      if (!node.m_function.empty()) {
        STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: unexpected accent %s\n", node.m_function.c_str()));
      }
      childDone=false;
    }
    break;
  }
  case Node::Function: {
    if (m_function1Set.find(toLower(node.m_function)) != m_function1Set.end() && node.m_childs.size()==1 && node.m_childs[0]) {
      // special function 1
      if (icmp(node.m_function, "abs")) {
        if (!addRow) m_output << "<mrow>";
        m_output << "<mo fence=\"true\" stretchy=\"true\">|</mo>";
        convertInMML(*node.m_childs[0], true);
        m_output << "<mo fence=\"true\" stretchy=\"true\">|</mo>";
        if (!addRow) m_output << "</mrow>";
        childDone=true;
        break;
      }
      if (icmp(node.m_function, "fact")) {
        if (!addRow) m_output << "<mrow>";
        convertInMML(*node.m_childs[0], true);
        m_output << "<mo stretchy=\"false\">!</mo>";
        if (!addRow) m_output << "</mrow>";
        childDone=true;
        break;
      }
      if (icmp(node.m_function, "sqrt")) {
        m_output << "<msqrt>";
        convertInMML(*node.m_childs[0]);
        m_output << "</msqrt>";
        childDone=true;
        break;
      }
    }
    if (m_function2Set.find(toLower(node.m_function)) != m_function2Set.end() && node.m_childs.size()==2 && node.m_childs[0] && node.m_childs[1]) {
      if (icmp(node.m_function, "binom")) {
        m_output << "<mtable>";
        for (auto c : node.m_childs) {
          m_output << "<mtr><mtd>";
          convertInMML(*c,true);
          m_output << "</mtd></mtr>";
        }
        m_output << "</mtable>";
        childDone=true;
        break;
      }
      if (icmp(node.m_function, "nroot")) {
        m_output << "<mroot>";
        convertInMML(*node.m_childs[1], true);
        convertInMML(*node.m_childs[0], true);
        m_output << "</mroot>";
        childDone=true;
        break;
      }
    }
    if (icmp(node.m_function, "stack")) {
      m_output << "<mtable>";
      for (auto c : node.m_childs) {
        m_output << "<mtr><mtd>";
        if (c)
          convertInMML(*c, true);
        m_output << "</mtd></mtr>";
      }
      m_output << "</mtable>";
      childDone=true;
      break;
    }
    if (icmp(node.m_function, "matrix")) {
      m_output << "<mtable>";
      for (auto c : node.m_childs) {
        m_output << "<mtr>";
        if (!c)
          m_output << "<mtd></mtd>";
        else if (c->m_type==Node::SequenceRow)
          convertInMML(*c);
        else {
          m_output << "<mtd>";
          convertInMML(*c, true);
          m_output << "</mtd>";
        }
        m_output << "</mtr>";
      }
      m_output << "</mtable>";
      childDone=true;
      break;
    }
    if (icmp(node.m_function,"func"))
      m_output << "<mi>" << getEscapedString(toLower(node.m_data)) << "</mi>";
    else
      m_output << "<mi>" << getEscapedString(toLower(node.m_function)) << "</mi>";
    break;
  }
  case Node::Sequence:
    if (!addRow) m_output << "<mrow>";
    for (auto c : node.m_childs) {
      if (c)
        convertInMML(*c);
    }
    if (!addRow) m_output << "</mrow>";
    childDone=true;
    break;
  case Node::SequenceRow:
    for (auto c : node.m_childs) {
      m_output << "<mtd>";
      if (c)
        convertInMML(*c, true);
      m_output << "</mtd>";
    }
    childDone=true;
    break;
  case Node::UnaryOperator:
    if (node.m_childs.size()!=1 || !node.m_childs[0]) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the unary operator\n"));
      break;
    }
    if (!addRow) m_output << "<mrow>";
    if (icmp(node.m_function,"uoper"))
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_data) << "</mo>";
    else if (m_unaryOperatorMap.find(toLower(node.m_function))!=m_unaryOperatorMap.end())
      m_output << "<mo stretchy=\"false\">" << m_unaryOperatorMap.find(toLower(node.m_function))->second << "</mo>";
    else
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_function) << "</mo>";
    convertInMML(*node.m_childs[0], true);
    if (!addRow) m_output << "</mrow>";
    childDone=true;
    break;

  case Node::Multiplication:
    if (node.m_childs.size()!=2 || !node.m_childs[0] || !node.m_childs[1]) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the relation's childs\n"));
      break;
    }
    if (icmp(node.m_function, "over") || icmp(node.m_function, "wideslash")) {
      if (icmp(node.m_function, "over"))
        m_output << "<mfrac>";
      else
        m_output << "<mfrac bevelled=\"true\">";
      for (auto c : node.m_childs) {
        convertInMML(*c, true);
      }
      m_output << "</mfrac>";
      childDone=true;
      break;
    }
    convertInMML(*node.m_childs[0], true);
    if (m_multiplicationStringMap.find(toLower(node.m_function))!=m_multiplicationStringMap.end()) {
      auto op=m_multiplicationStringMap.find(toLower(node.m_function))->second;
      if (!op.empty())
        m_output << "<mo stretchy=\"false\">" << m_multiplicationStringMap.find(toLower(node.m_function))->second << "</mo>";
      else if (icmp(op, "widebslah"))
        m_output << "<mo stretchy=\"false\">&#x2216;</mo>";
      else
        m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_function) << "</mo>";
    }
    else if (m_multiplicationMap.find(node.m_function)!=m_multiplicationMap.end())
      m_output << "<mo stretchy=\"false\">" << m_multiplicationMap.find(node.m_function)->second << "</mo>";
    else if (icmp(node.m_function, "boper"))
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_data) << "</mo>";
    else
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_function) << "</mo>";
    convertInMML(*node.m_childs[1], true);
    childDone=true;
    break;
  case Node::Addition:
    if (node.m_childs.size()!=2 || !node.m_childs[0] || !node.m_childs[1]) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the addition's childs\n"));
      break;
    }
    convertInMML(*node.m_childs[0], true);
    if (m_additionStringMap.find(toLower(node.m_function))!=m_additionStringMap.end())
      m_output << "<mo stretchy=\"false\">" << m_additionStringMap.find(toLower(node.m_function))->second << "</mo>";
    else
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_function) << "</mo>";
    convertInMML(*node.m_childs[1], true);
    childDone=true;
    break;
  case Node::Relation:
    if (node.m_childs.size()!=2 || !node.m_childs[0] || !node.m_childs[1]) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the relation's childs\n"));
      break;
    }
    convertInMML(*node.m_childs[0], true);
    if (m_relationMap.find(node.m_function)!=m_relationMap.end())
      m_output << "<mo stretchy=\"false\">" << m_relationMap.find(node.m_function)->second << "</mo>";
    else if (m_relationStringMap.find(toLower(node.m_function))!=m_relationStringMap.end())
      m_output << "<mo stretchy=\"false\">" << m_relationStringMap.find(toLower(node.m_function))->second << "</mo>";
    else
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_function) << "</mo>";
    convertInMML(*node.m_childs[1], true);
    childDone=true;
    break;
  case Node::Integral: {
    if (node.m_childs.size()!=3) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the relation's childs\n"));
      break;
    }
    if (!addRow) m_output << "<mrow>";
    std::string what(node.m_childs[0] ? (node.m_childs[1] ? "munderover" : "munder") : node.m_childs[1] ? "mover" : "");
    if (!what.empty()) m_output << "<" << what << ">";
    if (m_integralMap.find(toLower(node.m_function))!=m_integralMap.end())
      m_output << "<mo stretchy=\"false\">" << m_integralMap.find(toLower(node.m_function))->second << "</mo>";
    else if (icmp(node.m_function,"oper"))
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_data) << "</mo>";
    else
      m_output << "<mo stretchy=\"false\">" << getEscapedString(node.m_function) << "</mo>";
    if (node.m_childs[0])
      convertInMML(*node.m_childs[0], true);
    if (node.m_childs[1])
      convertInMML(*node.m_childs[1], true);
    if (!what.empty()) m_output << "</" << what << ">";
    if (node.m_childs[2])
      convertInMML(*node.m_childs[2], true);
    if (!addRow) m_output << "</mrow>";
    childDone=true;
    break;
  }
  case Node::Position: {
    if (node.m_childs.size()!=9) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the relation's childs\n"));
      break;
    }
    bool ok=convertPositionOverbraceInMML(node);
    if (addRow) m_output << "</mrow>";
    return ok;
  }
  case Node::Parenthesis:
  case Node::ParenthesisLeft:
  case Node::ParenthesisRight: {
    if (node.m_childs.size()!=1 || !node.m_childs[0]) {
      STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertInMML: can not find the parenthesis's childs\n"));
      break;
    }
    std::string left(node.m_type==Node::ParenthesisRight ? "" : node.m_function);
    std::string right(node.m_type==Node::ParenthesisRight ? node.m_function : node.m_data);
    if (left.empty() && right.empty()) {
      if (!addRow) m_output << "<mrow>";
      convertInMML(*node.m_childs[0], true);
      if (!addRow) m_output << "</mrow>";
    }
    else {
      m_output << "<mfenced open=\"";
      if (m_parenthesisToStringMap.find(toLower(left))!=m_parenthesisToStringMap.end())
        m_output << m_parenthesisToStringMap.find(toLower(left))->second;
      else if (!left.empty() && !icmp(left,"none"))
        m_output << getEscapedString(left);
      m_output << "\" close=\"";
      if (m_parenthesisToStringMap.find(toLower(right))!=m_parenthesisToStringMap.end())
        m_output << m_parenthesisToStringMap.find(toLower(right))->second;
      else if (!right.empty() && !icmp(right,"none"))
        m_output << getEscapedString(right);
      m_output << "\">";
      convertInMML(*node.m_childs[0], true);
      m_output << "</mfenced>";
    }
    childDone=true;
    break;
  }
  case Node::Root:
    m_output << "<mtable>";
    for (auto c : node.m_childs) {
      m_output << "<mtr>";
      // we must look the alignment "alignl", "alignr", "alignc", "alignt", "alignm", "alignb"
      if (c) {
        m_output << "<mtd";
        bool colDef=false, rowDef=false;
        findAndAddAlignment(*c, colDef, rowDef);
        m_output << ">";
        convertInMML(*c);
        m_output << "</mtd>";
      }
      else
        m_output << "<mtd><mrow /></mtd>";
      m_output << "</mtr>";
    }
    m_output << "</mtable>";
    childDone=true;
    break;
  default:
    break;
  }
  if (childDone) {
    if (addRow) m_output << "</mrow>";
    return true;
  }
  for (auto c : node.m_childs) {
    if (c) {
      if (!convertInMML(*c))
        return false;
    }
  }
  if (addRow) m_output << "</mrow>";
  return true;
}

bool Parser::convertPositionOverbraceInMML(Node const &node)
{
  if (node.m_childs.size()<9 || !node.m_childs[8])
    return convertPositionUnderbraceInMML(node);
  m_output << "<mover>";
  m_output << "<mover>";
  convertPositionUnderbraceInMML(node);
  m_output << "<mo stretchy=\"true\">&#x23de;</mo>";
  m_output << "</mover>";
  convertInMML(*node.m_childs[8], true);
  m_output << "</mover>";
  return true;
}

bool Parser::convertPositionUnderbraceInMML(Node const &node)
{
  if (node.m_childs.size()<9 || !node.m_childs[4])
    return convertPositionInMML(node);
  m_output << "<munder>";
  m_output << "<munder>";
  convertPositionInMML(node);
  m_output << "<mo stretchy=\"true\">&#x23df;</mo>";
  m_output << "</munder>";
  convertInMML(*node.m_childs[4], true);
  m_output << "</munder>";
  return true;
}

bool Parser::convertPositionInMML(Node const &node)
{
  if (node.m_childs.size()!=9) {
    STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::Parser::convertPositionInMML: can not find the relation's childs\n"));
    return false;
  }
  // simple case: sub, sup, supsub, none
  if (!node.m_childs[1] && !node.m_childs[2] && !node.m_childs[5] && !node.m_childs[6]) {
    std::string what(node.m_childs[3] ? (node.m_childs[7] ? "msubsup" : "msub") :
                     (node.m_childs[7] ? "msup" : ""));
    if (!what.empty())
      m_output << "<" << what << ">";
    if (node.m_childs[0])
      convertInMML(*node.m_childs[0], true);
    else
      m_output << "<mrow></mrow>";
    if (node.m_childs[3])
      convertInMML(*node.m_childs[3], true);
    if (node.m_childs[7])
      convertInMML(*node.m_childs[7], true);
    if (!what.empty())
      m_output << "</" << what << ">";
    return true;
  }

  // under/over/underover or more complex
  bool needSuperscript=node.m_childs[1] || node.m_childs[3] || node.m_childs[5] || node.m_childs[7];
  if (needSuperscript)
    m_output << "<mmultiscripts>";

  std::string what(node.m_childs[2] ? (node.m_childs[6] ? "munderover" : "munder") :
                   (node.m_childs[6] ? "mover" : ""));
  if (!what.empty())
    m_output << "<" << what << ">";
  if (node.m_childs[0])
    convertInMML(*node.m_childs[0], true);
  else
    m_output << "<mrow></mrow>";
  if (node.m_childs[2])
    convertInMML(*node.m_childs[2], true);
  if (node.m_childs[6])
    convertInMML(*node.m_childs[6], true);
  if (!what.empty())
    m_output << "</" << what << ">";

  if (!needSuperscript) // end under/over/underover
    return true;
  if (node.m_childs[3])
    convertInMML(*node.m_childs[3], true);
  else
    m_output << "<none />";
  if (node.m_childs[7])
    convertInMML(*node.m_childs[7], true);
  else
    m_output << "<none />";
  m_output << "<mprescripts />";
  if (node.m_childs[1])
    convertInMML(*node.m_childs[1], true);
  else
    m_output << "<none />";
  if (node.m_childs[5])
    convertInMML(*node.m_childs[5], true);
  else
    m_output << "<none />";
  m_output << "</mmultiscripts>";
  return true;
}

bool Parser::parse(librevenge::RVNGString const &formula, librevenge::RVNGString &res)
{
  if (!convert(formula, m_dataList)) {
    STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::parser::convert: can not lex %s\n", formula.cstr()));
    return false;
  }

  try {
    auto node=expr();
    if (node) {
      m_output.clear();
      m_output << "<math xmlns=\"http://www.w3.org/1998/Math/MathML\" display=\"block\">";
      m_output << "<semantics>";
      if (convertInMML(*node, node->m_type != Node::Root)) {
        m_output << "<annotation encoding=\"StarMath 5.0\">";
        m_output << getEscapedString(formula.cstr());
        m_output << "</annotation>";
        m_output << "</semantics>";
        m_output << "</math>";
        res=m_output.str().c_str();
        return true;
      }
    }
  }
  catch (...) {
  }
  STOFF_DEBUG_MSG(("STOFFStarMathToMMLConverterInternal::parser::convert: unexpected expection when parsing lex %s\n", formula.cstr()));
  return false;
}

bool Parser::convert(librevenge::RVNGString const &starMath, std::vector<LexerData> &lexList) const
{
  auto const *ptr=starMath.cstr();
  LexerData data;
  bool canConcatenate=false;
  std::vector<LexerData> dataList;
  while (true) {
    unsigned char c=(unsigned char)(*(ptr++));
    if (std::isalpha(int(c)) || c>=0x80) {
      data.m_string+=char(c);
      canConcatenate=false;
      if (c>=0x80) data.m_type=LexerData::String;
      continue;
    }
    if (!data.m_string.empty())
      dataList.push_back(data);
    if (c==0) break;
    data=LexerData();
    auto len=dataList.size();
    if (c=='\t') c=' '; // only one separator
    if (c && c<0x1f) { // internal endline or bad char
      canConcatenate=false;
      continue;
    }
    if (std::isdigit(c) || (c=='.' && std::isdigit(*ptr))) { // check for number
      bool isScientific=false;
      // check also if this is a scientific float 2e3 or 2e-3
      if (canConcatenate && len>=2 && c!='.') {
        if (dataList[len-1].m_type==LexerData::Unknown && (dataList[len-1].m_string=="e" || dataList[len-1].m_string=="E") &&
            dataList[len-2].m_type==LexerData::Number &&
            dataList[len-2].m_string.find('e')==std::string::npos && dataList[len-2].m_string.find('E')==std::string::npos) {
          dataList[len-2].m_string += dataList[len-1].m_string;
          dataList.resize(len-1);
          isScientific=true;
        }
        else if (len>=3 && dataList[len-1].m_type==LexerData::Special && dataList[len-1].m_string=="-" &&
                 dataList[len-2].m_type==LexerData::Unknown && (dataList[len-2].m_string=="e" || dataList[len-2].m_string=="E") &&
                 dataList[len-3].m_type==LexerData::Number &&
                 dataList[len-3].m_string.find('e')==std::string::npos && dataList[len-3].m_string.find('E')==std::string::npos) {
          dataList[len-3].m_string += dataList[len-2].m_string + dataList[len-1].m_string;
          dataList.resize(len-2);
          isScientific=true;
        }
      }
      bool seeDecimalPoint=isScientific; // 2e3.2 is parsed as 2e3 and 0.2
      data.m_type=LexerData::Number;
      data.m_string=char(c);
      while (true) {
        if (std::isdigit(*ptr))
          data.m_string+=*(ptr++);
        else if (!seeDecimalPoint && *ptr=='.') {
          data.m_string+=*(ptr++);
          seeDecimalPoint=true;
        }
        else
          break;
      }
      if (!isScientific) {
        dataList.push_back(data);
        canConcatenate=true;
      }
      else
        dataList.back().m_string+=data.m_string;
    }
    else if (c=='"') { // check for forced string
      data.m_type=LexerData::String;
      while (*ptr) {
        c=(unsigned char) *(ptr++);
        if (c=='"')
          break;
        if (c>0x1f) // tab seems ignored, so ok
          data.m_string += char(c);
      }
      // "XXXX... without the ending tags '"' seems ok
      if (!data.m_string.empty())
        dataList.push_back(data);
    }
    else if (c==' ' || c=='~' || c=='`') {
      // space or ' ' is used to delimite block
      // sequential space does not seem to have any meaning
      // so we must keep the first one
      if (len==0 || dataList.back().m_type!=LexerData::Space || c!=' ') {
        data.m_type=LexerData::Space;
        data.m_string += char(c);
        dataList.push_back(data);
      }
    }
    else {
      bool done=false;
      if (canConcatenate && len>=1 && dataList.back().m_type==LexerData::Special) {
        // check for double symbol
        if (c=='#' && dataList.back().m_string=="#") {
          dataList.back().m_string+=char(c);
          done=true;
        }
        else if (c=='-' && dataList.back().m_string=="+") {
          dataList.back().m_string+=char(c);
          done=true;
        }
        else if (c=='+' && dataList.back().m_string=="-") {
          dataList.back().m_string+=char(c);
          done=true;
        }
        else if ((c=='='||c=='>') && (dataList.back().m_string=="<" || dataList.back().m_string==">")) {
          dataList.back().m_string+=char(c);
          done=true;
        }
        else if (len>=2 && c=='>' && dataList.back().m_string=="?" &&
                 dataList[len-2].LexerData::Special && dataList[len-2].m_string=="<") {
          dataList[len-2].m_type=LexerData::PlaceHolder;
          dataList[len-2].m_string="<?>";
          dataList.resize(len-1);
          done=true;
        }
        else if (c=='<' && dataList.back().m_string=="<") {
          dataList.back().m_string+=char(c);
          done=true;
        }
      }
      if (!done) {
        data.m_type=LexerData::Special;
        data.m_string=char(c);
        dataList.push_back(data);
      }
      canConcatenate=true;
    }
    data=LexerData();

    if (*ptr==0) break;
  }

  // finally, merge the %xxx \%%% valid command
  for (size_t i=0; i<dataList.size(); ++i) {
    auto const &actData=dataList[i];
    if (i+1>=dataList.size() || actData.m_type!=LexerData::Special) {
      lexList.push_back(actData);
      continue;
    }
    if (actData.m_string=="%" && dataList[i+1].m_type==LexerData::Unknown &&
        (m_greekMap.find(dataList[i+1].m_string)!=m_greekMap.end() || // basic greek caracter
         (dataList[i+1].m_string.size()>2 && dataList[i+1].m_string[0]=='i' &&
          m_greekMap.find(dataList[i+1].m_string.c_str()+1)!=m_greekMap.end()) || // italic caracter
         m_specialPercentMap.find(toLower(dataList[i+1].m_string))!=m_specialPercentMap.end())) { // other caracter
      LexerData newData;
      newData.m_string=actData.m_string+dataList[++i].m_string;
      lexList.push_back(newData);
      continue;
    }
    if (actData.m_string=="\\" && dataList[i+1].m_type!=LexerData::String) {
      bool isSpecial=m_parenthesisMap.find(toLower(dataList[i+1].m_string))!=m_parenthesisMap.end() ||
                     m_parenthesisRightSet.find(toLower(dataList[i+1].m_string))!=m_parenthesisRightSet.end();
      if (isSpecial || dataList[i+1].m_string=="{" || dataList[i+1].m_string=="}") { // \{ and \} seems to be converted in caracter
        LexerData newData;
        if (!isSpecial) {
          newData.m_string=dataList[++i].m_string;
          newData.m_type=LexerData::String;
        }
        else {
          newData.m_string=actData.m_string+dataList[++i].m_string;
          if (!std::isalpha(dataList[i+1].m_string[0])) // ()[]
            newData.m_type=LexerData::Special;
        }
        lexList.push_back(newData);
        continue;
      }
    }
    lexList.push_back(actData);
  }
  return true;
}
}
bool STOFFStarMathToMMLConverter::convertStarMath(librevenge::RVNGString const &starMath, librevenge::RVNGString &mml)
{
  //std::cerr << "Try to convert " << starMath.cstr() << "\n";
  STOFFStarMathToMMLConverterInternal::Parser parser;
  bool ok=parser.parse(starMath, mml);
  //std::cout << mml.cstr() << "\n";
  return ok;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
