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
 * file to read/parse StarOffice attributes
 *
 */
#ifndef STAR_ATTRIBUTE
#  define STAR_ATTRIBUTE

#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

namespace StarAttributeInternal
{
struct State;
}

class StarZone;
class SDWParser;

/** \brief the main class to read/.. a StarOffice attribute
 *
 *
 *
 */
class StarAttribute
{
public:
  //! the attribute list
  enum {
    ATR_CHR_CASEMAP = 1,					        	    		// 1
    ATR_CHR_CHARSETCOLOR,					        	    		// 2
    ATR_CHR_COLOR,								        	    		// 3
    ATR_CHR_CONTOUR,							        	    		// 4
    ATR_CHR_CROSSEDOUT,						        	    		// 5
    ATR_CHR_ESCAPEMENT,						        	    		// 6
    ATR_CHR_FONT,									        	    		// 7
    ATR_CHR_FONTSIZE,							        	    		// 8
    ATR_CHR_KERNING, 							        	    		// 9
    ATR_CHR_LANGUAGE,							        	    		// 10
    ATR_CHR_POSTURE, 							        	    		// 11
    ATR_CHR_PROPORTIONALFONTSIZE,		      	    		// 12
    ATR_CHR_SHADOWED,							        	    		// 13
    ATR_CHR_UNDERLINE,						        	    		// 14
    ATR_CHR_WEIGHT,								        	    		// 15
    ATR_CHR_WORDLINEMODE,					        	    		// 16
    ATR_CHR_AUTOKERN,							        	    		// 17
    ATR_CHR_BLINK,								        	    		// 18
    ATR_CHR_NOHYPHEN,							        	    		// 19
    ATR_CHR_NOLINEBREAK, 					        	    		// 20
    ATR_CHR_BACKGROUND,						        	    		// 21
    ATR_CHR_CJK_FONT,							        	    		// 22
    ATR_CHR_CJK_FONTSIZE,					        	    		// 23
    ATR_CHR_CJK_LANGUAGE,					        	    		// 24
    ATR_CHR_CJK_POSTURE, 					        	    		// 25
    ATR_CHR_CJK_WEIGHT,						        	    		// 26
    ATR_CHR_CTL_FONT,							        	    		// 27
    ATR_CHR_CTL_FONTSIZE,					        	    		// 28
    ATR_CHR_CTL_LANGUAGE,					        	    		// 29
    ATR_CHR_CTL_POSTURE, 					        	    		// 30
    ATR_CHR_CTL_WEIGHT,						        	    		// 31
    ATR_CHR_ROTATE,								        	    		// 32
    ATR_CHR_EMPHASIS_MARK,				        	    		// 33
    ATR_CHR_TWO_LINES, 						        	    		// 34
    ATR_CHR_SCALEW,								        	    		// 35
    ATR_CHR_RELIEF,								        	    		// 36
    ATR_CHR_DUMMY1,								        	    		// 37

    ATR_TXT_INETFMT,							        	    		// 38
    ATR_TXT_DUMMY4,								        	    		// 39
    ATR_TXT_REFMARK, 							        	    		// 40
    ATR_TXT_TOXMARK, 							        	    		// 41
    ATR_TXT_CHARFMT, 							        	    		// 42
    ATR_TXT_DUMMY5, 								      	    		// 43
    ATR_TXT_CJK_RUBY, 							      	    		// 44
    ATR_TXT_UNKNOWN_CONTAINER,					  	    		// 45
    ATR_TXT_DUMMY6,								        	    		// 46
    ATR_TXT_DUMMY7,								        	    		// 47

    ATR_TXT_FIELD,								        	    		// 48
    ATR_TXT_FLYCNT,								        	    		// 49
    ATR_TXT_FTN, 									        	    		// 50
    ATR_TXT_SOFTHYPH,							        	    		// 51
    ATR_TXT_HARDBLANK,							      	    		// 52
    ATR_TXT_DUMMY1,								        	    		// 53
    ATR_TXT_DUMMY2,								        	    		// 54

    ATR_PARA_LINESPACING,		              	    		// 55
    ATR_PARA_ADJUST,								    	    			// 56
    ATR_PARA_SPLIT,								              		// 57
    ATR_PARA_ORPHANS, 							    	    			// 58
    ATR_PARA_WIDOWS,								    	    			// 59
    ATR_PARA_TABSTOP, 							    	    			// 60
    ATR_PARA_HYPHENZONE,							    	    		// 61
    ATR_PARA_DROP,								              		// 62
    ATR_PARA_REGISTER,							    	    			// 63
    ATR_PARA_NUMRULE, 							    	    			// 64
    ATR_PARA_SCRIPTSPACE,							    	    		// 65
    ATR_PARA_HANGINGPUNCTUATION,					    	    // 66
    ATR_PARA_FORBIDDEN_RULES,						    	    	// 67
    ATR_PARA_VERTALIGN,								              // 68
    ATR_PARA_SNAPTOGRID,                    	    	// 69
    ATR_PARA_CONNECT_BORDER,                	    	// 70
    ATR_PARA_DUMMY5,								              	// 71
    ATR_PARA_DUMMY6,								              	// 72
    ATR_PARA_DUMMY7,								              	// 73
    ATR_PARA_DUMMY8,								              	// 74

    ATR_FRM_FILL_ORDER,                           	// 75
    ATR_FRM_FRM_SIZE,                             	// 76
    ATR_FRM_PAPER_BIN,                            	// 77
    ATR_FRM_LR_SPACE,                             	// 78
    ATR_FRM_UL_SPACE,                             	// 79
    ATR_FRM_PAGEDESC,                             	// 80
    ATR_FRM_BREAK,                                	// 81
    ATR_FRM_CNTNT,                                	// 82
    ATR_FRM_HEADER,                               	// 83
    ATR_FRM_FOOTER,                               	// 84
    ATR_FRM_PRINT,                                	// 85
    ATR_FRM_OPAQUE,                               	// 86
    ATR_FRM_PROTECT,                              	// 87
    ATR_FRM_SURROUND,                             	// 88
    ATR_FRM_VERT_ORIENT,                          	// 89
    ATR_FRM_HORI_ORIENT,                          	// 90
    ATR_FRM_ANCHOR,                               	// 91
    ATR_FRM_BACKGROUND,                           	// 92
    ATR_FRM_BOX,                                  	// 93
    ATR_FRM_SHADOW,                               	// 94
    ATR_FRM_FRMMACRO,                             	// 95
    ATR_FRM_COL,                                  	// 96
    ATR_FRM_KEEP,                                 	// 97
    ATR_FRM_URL,                                  	// 98
    ATR_FRM_EDIT_IN_READONLY,                     	// 99
    ATR_FRM_LAYOUT_SPLIT,                         	// 100
    ATR_FRM_CHAIN,                                	// 101
    ATR_FRM_TEXTGRID,                             	// 102
    ATR_FRM_LINENUMBER  ,                         	// 103
    ATR_FRM_FTN_AT_TXTEND,                        	// 104
    ATR_FRM_END_AT_TXTEND,                        	// 105
    ATR_FRM_COLUMNBALANCE,                        	// 106
    ATR_FRM_FRAMEDIR,                             	// 107
    ATR_FRM_HEADER_FOOTER_EAT_SPACING,            	// 108
    ATR_FRM_FRMATR_DUMMY9,                        	// 109

    ATR_GRF_MIRRORGRF,  	                 	      	// 110
    ATR_GRF_CROPGRF,                       	      	// 111
    ATR_GRF_ROTATION,                      	      	// 112
    ATR_GRF_LUMINANCE,                     	      	// 113
    ATR_GRF_CONTRAST,                      	      	// 114
    ATR_GRF_CHANNELR,                      	      	// 115
    ATR_GRF_CHANNELG,                      	      	// 116
    ATR_GRF_CHANNELB,                      	      	// 117
    ATR_GRF_GAMMA,                         	      	// 118
    ATR_GRF_INVERT,                        	      	// 119
    ATR_GRF_TRANSPARENCY,                  	      	// 120
    ATR_GRF_DRAWMODE,                      	      	// 121
    ATR_GRF_DUMMY1,                        	      	// 122
    ATR_GRF_DUMMY2,                        	      	// 123
    ATR_GRF_DUMMY3,                        	      	// 124
    ATR_GRF_DUMMY4,                        	      	// 125
    ATR_GRF_DUMMY5,                        	      	// 126

    ATR_BOX_FORMAT,     	                 	      	// 127
    ATR_BOX_FORMULA,                       	      	// 128
    ATR_BOX_VALUE                         	      	// 129
  };
  //! constructor
  StarAttribute();
  //! destructor
  virtual ~StarAttribute();


  //! try to read an attribute
  bool readAttribute(StarZone &zone, int which, int vers, long endPos, SDWParser &manager);

  //
  // data
  //
private:
  //! the state
  shared_ptr<StarAttributeInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
