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

#include <set>
#include <sstream>
#include <vector>

#include "STOFFDebug.hxx"
#include "STOFFEntry.hxx"
#include "STOFFInputStream.hxx"

#include "StarItem.hxx"

namespace StarAttributeInternal
{
struct State;
}

class StarItemPool;
class StarObject;
class StarState;
class StarZone;

//! virtual class used to store the different attribute
class StarAttribute
{
public:
  //! the attribute list
  enum Type {
    ATTR_SPECIAL=-1,                                  // special
    ATTR_CHR_CASEMAP = 1,					        	    		// 1
    ATTR_CHR_CHARSETCOLOR,					        	    	// 2
    ATTR_CHR_COLOR,								        	    		// 3
    ATTR_CHR_CONTOUR,							        	    		// 4
    ATTR_CHR_CROSSEDOUT,						        	    	// 5
    ATTR_CHR_ESCAPEMENT,						        	    	// 6
    ATTR_CHR_FONT,									        	    	// 7
    ATTR_CHR_FONTSIZE,							        	    	// 8
    ATTR_CHR_KERNING, 							        	    	// 9
    ATTR_CHR_LANGUAGE,							        	    	// 10
    ATTR_CHR_POSTURE, 							        	    	// 11
    ATTR_CHR_PROPORTIONALFONTSIZE,		      	    	// 12
    ATTR_CHR_SHADOWED,							        	    	// 13
    ATTR_CHR_UNDERLINE,						        	    		// 14
    ATTR_CHR_WEIGHT,								        	    	// 15
    ATTR_CHR_WORDLINEMODE,					        	    	// 16
    ATTR_CHR_AUTOKERN,							        	    	// 17
    ATTR_CHR_BLINK,								        	    		// 18
    ATTR_CHR_NOHYPHEN,							        	    	// 19
    ATTR_CHR_NOLINEBREAK, 					        	    	// 20
    ATTR_CHR_BACKGROUND,						        	    	// 21
    ATTR_CHR_CJK_FONT,							        	    	// 22
    ATTR_CHR_CJK_FONTSIZE,					        	    	// 23
    ATTR_CHR_CJK_LANGUAGE,					        	    	// 24
    ATTR_CHR_CJK_POSTURE, 					        	    	// 25
    ATTR_CHR_CJK_WEIGHT,						        	    	// 26
    ATTR_CHR_CTL_FONT,							        	    	// 27
    ATTR_CHR_CTL_FONTSIZE,					        	    	// 28
    ATTR_CHR_CTL_LANGUAGE,					        	    	// 29
    ATTR_CHR_CTL_POSTURE, 					        	    	// 30
    ATTR_CHR_CTL_WEIGHT,						        	    	// 31
    ATTR_CHR_ROTATE,								        	    	// 32
    ATTR_CHR_EMPHASIS_MARK,				        	    		// 33
    ATTR_CHR_TWO_LINES, 						        	    	// 34
    ATTR_CHR_SCALEW,								        	    	// 35
    ATTR_CHR_RELIEF,								        	    	// 36
    ATTR_CHR_DUMMY1,								        	    	// 37

    ATTR_TXT_INETFMT,							        	    		// 38
    ATTR_TXT_DUMMY4,								        	    	// 39
    ATTR_TXT_REFMARK, 							        	    	// 40
    ATTR_TXT_TOXMARK, 							        	    	// 41
    ATTR_TXT_CHARFMT, 							        	    	// 42
    ATTR_TXT_DUMMY5, 								      	    		// 43
    ATTR_TXT_CJK_RUBY, 							      	    		// 44
    ATTR_TXT_UNKNOWN_CONTAINER,					  	    		// 45
    ATTR_TXT_DUMMY6,								        	    	// 46
    ATTR_TXT_DUMMY7,								        	    	// 47

    ATTR_TXT_FIELD,								        	    		// 48
    ATTR_TXT_FLYCNT,								        	    	// 49
    ATTR_TXT_FTN, 									        	    	// 50
    ATTR_TXT_SOFTHYPH,							        	    	// 51
    ATTR_TXT_HARDBLANK,							      	    		// 52
    ATTR_TXT_DUMMY1,								        	    	// 53
    ATTR_TXT_DUMMY2,								        	    	// 54

    ATTR_PARA_LINESPACING,		              	    	// 55
    ATTR_PARA_ADJUST,								    	    			// 56
    ATTR_PARA_SPLIT,								              	// 57
    ATTR_PARA_ORPHANS, 							    	    			// 58
    ATTR_PARA_WIDOWS,								    	    			// 59
    ATTR_PARA_TABSTOP, 							    	    			// 60
    ATTR_PARA_HYPHENZONE,							    	    		// 61
    ATTR_PARA_DROP,								              		// 62
    ATTR_PARA_REGISTER,							    	    			// 63
    ATTR_PARA_NUMRULE, 							    	    			// 64
    ATTR_PARA_SCRIPTSPACE,							    	    	// 65
    ATTR_PARA_HANGINGPUNCTUATION,					    	    // 66
    ATTR_PARA_FORBIDDEN_RULES,						    	    // 67
    ATTR_PARA_VERTALIGN,								            // 68
    ATTR_PARA_SNAPTOGRID,                    	    	// 69
    ATTR_PARA_CONNECT_BORDER,                	    	// 70
    ATTR_PARA_DUMMY5,								              	// 71
    ATTR_PARA_DUMMY6,								              	// 72
    ATTR_PARA_DUMMY7,								              	// 73
    ATTR_PARA_DUMMY8,								              	// 74

    ATTR_FRM_FILL_ORDER,                           	// 75
    ATTR_FRM_FRM_SIZE,                             	// 76
    ATTR_FRM_PAPER_BIN,                            	// 77
    ATTR_FRM_LR_SPACE,                             	// 78
    ATTR_FRM_UL_SPACE,                             	// 79
    ATTR_FRM_PAGEDESC,                             	// 80
    ATTR_FRM_BREAK,                                	// 81
    ATTR_FRM_CNTNT,                                	// 82
    ATTR_FRM_HEADER,                               	// 83
    ATTR_FRM_FOOTER,                               	// 84
    ATTR_FRM_PRINT,                                	// 85
    ATTR_FRM_OPAQUE,                               	// 86
    ATTR_FRM_PROTECT,                              	// 87
    ATTR_FRM_SURROUND,                             	// 88
    ATTR_FRM_VERT_ORIENT,                          	// 89
    ATTR_FRM_HORI_ORIENT,                          	// 90
    ATTR_FRM_ANCHOR,                               	// 91
    ATTR_FRM_BACKGROUND,                           	// 92
    ATTR_FRM_BOX,                                  	// 93
    ATTR_FRM_SHADOW,                               	// 94
    ATTR_FRM_FRMMACRO,                             	// 95
    ATTR_FRM_COL,                                  	// 96
    ATTR_FRM_KEEP,                                 	// 97
    ATTR_FRM_URL,                                  	// 98
    ATTR_FRM_EDIT_IN_READONLY,                     	// 99
    ATTR_FRM_LAYOUT_SPLIT,                         	// 100
    ATTR_FRM_CHAIN,                                	// 101
    ATTR_FRM_TEXTGRID,                             	// 102
    ATTR_FRM_LINENUMBER,                         	// 103
    ATTR_FRM_FTN_AT_TXTEND,                        	// 104
    ATTR_FRM_END_AT_TXTEND,                        	// 105
    ATTR_FRM_COLUMNBALANCE,                        	// 106
    ATTR_FRM_FRAMEDIR,                             	// 107
    ATTR_FRM_HEADER_FOOTER_EAT_SPACING,            	// 108
    ATTR_FRM_FRMATTR_DUMMY9,                        // 109

    ATTR_GRF_MIRRORGRF,  	                 	      	// 110
    ATTR_GRF_CROPGRF,                       	      // 111
    ATTR_GRF_ROTATION,                      	      // 112
    ATTR_GRF_LUMINANCE,                     	      // 113
    ATTR_GRF_CONTRAST,                      	      // 114
    ATTR_GRF_CHANNELR,                      	      // 115
    ATTR_GRF_CHANNELG,                      	      // 116
    ATTR_GRF_CHANNELB,                      	      // 117
    ATTR_GRF_GAMMA,                         	      // 118
    ATTR_GRF_INVERT,                        	      // 119
    ATTR_GRF_TRANSPARENCY,                  	      // 120
    ATTR_GRF_DRAWMODE,                      	      // 121
    ATTR_GRF_DUMMY1,                        	      // 122
    ATTR_GRF_DUMMY2,                        	      // 123
    ATTR_GRF_DUMMY3,                        	      // 124
    ATTR_GRF_DUMMY4,                        	      // 125
    ATTR_GRF_DUMMY5,                        	      // 126

    ATTR_BOX_FORMAT,     	                 	      	// 127
    ATTR_BOX_FORMULA,                       	      // 128
    ATTR_BOX_VALUE,                         	      // 129

    // other
    ATTR_SC_USERDEF,                         	      // 130
    ATTR_SC_HYPHENATE,
    ATTR_SC_HORJUSTIFY,
    ATTR_SC_INDENT,
    ATTR_SC_VERJUSTIFY,
    ATTR_SC_ORIENTATION,
    ATTR_SC_ROTATE_VALUE,
    ATTR_SC_ROTATE_MODE,
    ATTR_SC_VERTICAL_ASIAN,
    ATTR_SC_WRITINGDIR,
    ATTR_SC_LINEBREAK,                         	     // 140
    ATTR_SC_MARGIN,
    ATTR_SC_MERGE,
    ATTR_SC_MERGE_FLAG,
    ATTR_SC_VALUE_FORMAT,
    ATTR_SC_LANGUAGE_FORMAT,
    ATTR_SC_BACKGROUND,
    ATTR_SC_PROTECTION,
    ATTR_SC_BORDER,
    ATTR_SC_BORDER_INNER,
    ATTR_SC_SHADOW,																	// 150
    ATTR_SC_VALIDDATA,
    ATTR_SC_CONDITIONAL,
    ATTR_SC_PATTERN,
    ATTR_SC_PAGE,
    ATTR_SC_PAGE_PAPERTRAY,
    ATTR_SC_PAGE_SIZE,
    ATTR_SC_PAGE_MAXSIZE,
    ATTR_SC_PAGE_HORCENTER,
    ATTR_SC_PAGE_VERCENTER,
    ATTR_SC_PAGE_ON,
    ATTR_SC_PAGE_DYNAMIC,
    ATTR_SC_PAGE_SHARED,
    ATTR_SC_PAGE_NOTES,
    ATTR_SC_PAGE_GRID,
    ATTR_SC_PAGE_HEADERS,
    ATTR_SC_PAGE_CHARTS,
    ATTR_SC_PAGE_OBJECTS,
    ATTR_SC_PAGE_DRAWINGS,
    ATTR_SC_PAGE_TOPDOWN,
    ATTR_SC_PAGE_SCALE,
    ATTR_SC_PAGE_SCALETOPAGES,
    ATTR_SC_PAGE_FIRSTPAGENO,
    ATTR_SC_PAGE_PRINTAREA,
    ATTR_SC_PAGE_REPEATROW,
    ATTR_SC_PAGE_REPEATCOL,
    ATTR_SC_PAGE_PRINTTABLES,
    ATTR_SC_PAGE_HEADERLEFT,
    ATTR_SC_PAGE_FOOTERLEFT,
    ATTR_SC_PAGE_HEADERRIGHT,
    ATTR_SC_PAGE_FOOTERRIGHT,
    ATTR_SC_PAGE_HEADERSET,
    ATTR_SC_PAGE_FOOTERSET,
    ATTR_SC_PAGE_FORMULAS,
    ATTR_SC_PAGE_NULLVALS,

    ATTR_EE_PARA_XMLATTRIBS,
    ATTR_EE_PARA_ASIANCJKSPACING,
    ATTR_EE_PARA_NUMBULLET,
    ATTR_EE_PARA_BULLETSTATE,
    ATTR_EE_PARA_OUTLLR_SPACE,
    ATTR_EE_PARA_OUTLLEVEL,
    ATTR_EE_PARA_BULLET,
    ATTR_EE_CHR_SCALEW,
    ATTR_EE_CHR_RUBI_DUMMY,
    ATTR_EE_CHR_XMLATTRIBS,
    ATTR_EE_FEATURE_TAB,
    ATTR_EE_FEATURE_LINEBR,
    ATTR_EE_FEATURE_FIELD,

    ATTR_SCH_DATADESCR_DESCR,
    ATTR_SCH_DATADESCR_SHOW_SYM,
    ATTR_SCH_LEGEND_POS,
    ATTR_SCH_TEXT_ORIENT,
    ATTR_SCH_TEXT_ORDER,

    ATTR_SCH_X_AXIS_AUTO_MIN,
    ATTR_SCH_X_AXIS_MIN,
    ATTR_SCH_X_AXIS_AUTO_MAX,
    ATTR_SCH_X_AXIS_MAX,
    ATTR_SCH_X_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_X_AXIS_STEP_MAIN,
    ATTR_SCH_X_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_X_AXIS_STEP_HELP,
    ATTR_SCH_X_AXIS_LOGARITHM,
    ATTR_SCH_X_AXIS_AUTO_ORIGIN,
    ATTR_SCH_X_AXIS_ORIGIN,
    ATTR_SCH_Y_AXIS_AUTO_MIN,
    ATTR_SCH_Y_AXIS_MIN,
    ATTR_SCH_Y_AXIS_AUTO_MAX,
    ATTR_SCH_Y_AXIS_MAX,
    ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_Y_AXIS_STEP_MAIN,
    ATTR_SCH_Y_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_Y_AXIS_STEP_HELP,
    ATTR_SCH_Y_AXIS_LOGARITHM,
    ATTR_SCH_Y_AXIS_AUTO_ORIGIN,
    ATTR_SCH_Y_AXIS_ORIGIN,
    ATTR_SCH_Z_AXIS_AUTO_MIN,
    ATTR_SCH_Z_AXIS_MIN,
    ATTR_SCH_Z_AXIS_AUTO_MAX,
    ATTR_SCH_Z_AXIS_MAX,
    ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_Z_AXIS_STEP_MAIN,
    ATTR_SCH_Z_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_Z_AXIS_STEP_HELP,
    ATTR_SCH_Z_AXIS_LOGARITHM,
    ATTR_SCH_Z_AXIS_AUTO_ORIGIN,
    ATTR_SCH_Z_AXIS_ORIGIN,

    ATTR_SCH_AXISTYPE,
    ATTR_SCH_DUMMY0,
    ATTR_SCH_DUMMY1,
    ATTR_SCH_DUMMY2,
    ATTR_SCH_DUMMY3,
    ATTR_SCH_DUMMY_END,

    ATTR_SCH_STAT_AVERAGE,
    ATTR_SCH_STAT_KIND_ERROR,
    ATTR_SCH_STAT_PERCENT,
    ATTR_SCH_STAT_BIGERROR,
    ATTR_SCH_STAT_CONSTPLUS,
    ATTR_SCH_STAT_CONSTMINUS,
    ATTR_SCH_STAT_REGRESSTYPE,
    ATTR_SCH_STAT_INDICATE,
    ATTR_SCH_TEXT_DEGREES,
    ATTR_SCH_TEXT_OVERLAP,
    ATTR_SCH_TEXT_DUMMY0,
    ATTR_SCH_TEXT_DUMMY1,
    ATTR_SCH_TEXT_DUMMY2,
    ATTR_SCH_TEXT_DUMMY3,

    ATTR_SCH_STYLE_DEEP,
    ATTR_SCH_STYLE_3D,
    ATTR_SCH_STYLE_VERTICAL,
    ATTR_SCH_STYLE_BASETYPE,
    ATTR_SCH_STYLE_LINES,
    ATTR_SCH_STYLE_PERCENT,
    ATTR_SCH_STYLE_STACKED,
    ATTR_SCH_STYLE_SPLINES,
    ATTR_SCH_STYLE_SYMBOL,
    ATTR_SCH_STYLE_SHAPE,

    ATTR_SCH_AXIS,
    ATTR_SCH_AXIS_AUTO_MIN,
    ATTR_SCH_AXIS_MIN,
    ATTR_SCH_AXIS_AUTO_MAX,
    ATTR_SCH_AXIS_MAX,
    ATTR_SCH_AXIS_AUTO_STEP_MAIN,
    ATTR_SCH_AXIS_STEP_MAIN,
    ATTR_SCH_AXIS_AUTO_STEP_HELP,
    ATTR_SCH_AXIS_STEP_HELP,
    ATTR_SCH_AXIS_LOGARITHM,
    ATTR_SCH_AXIS_AUTO_ORIGIN,
    ATTR_SCH_AXIS_ORIGIN,

    ATTR_SCH_AXIS_TICKS,
    ATTR_SCH_AXIS_NUMFMT,
    ATTR_SCH_AXIS_NUMFMTPERCENT,
    ATTR_SCH_AXIS_SHOWAXIS,
    ATTR_SCH_AXIS_SHOWDESCR,
    ATTR_SCH_AXIS_SHOWMAINGRID,
    ATTR_SCH_AXIS_SHOWHELPGRID,
    ATTR_SCH_AXIS_TOPDOWN,
    ATTR_SCH_AXIS_HELPTICKS,

    ATTR_SCH_AXIS_DUMMY0,
    ATTR_SCH_AXIS_DUMMY1,
    ATTR_SCH_AXIS_DUMMY2,
    ATTR_SCH_AXIS_DUMMY3,
    ATTR_SCH_BAR_OVERLAP,
    ATTR_SCH_BAR_GAPWIDTH,

    ATTR_SCH_SYMBOL_BRUSH,
    ATTR_SCH_STOCK_VOLUME,
    ATTR_SCH_STOCK_UPDOWN,
    ATTR_SCH_SYMBOL_SIZE,
    ATTR_SCH_USER_DEFINED_ATTR,

    XATTR_LINESTYLE,
    XATTR_LINEDASH,
    XATTR_LINEWIDTH,
    XATTR_LINECOLOR,
    XATTR_LINESTART,
    XATTR_LINEEND,
    XATTR_LINESTARTWIDTH,
    XATTR_LINEENDWIDTH,
    XATTR_LINESTARTCENTER,
    XATTR_LINEENDCENTER,
    XATTR_LINETRANSPARENCE,
    XATTR_LINEJOINT,
    XATTR_LINERESERVED2,
    XATTR_LINERESERVED3,
    XATTR_LINERESERVED4,
    XATTR_LINERESERVED5,
    XATTR_LINERESERVED_LAST,
    XATTR_SET_LINE,
    XATTR_FILLSTYLE,
    XATTR_FILLCOLOR,
    XATTR_FILLGRADIENT,
    XATTR_FILLHATCH,
    XATTR_FILLBITMAP,
    XATTR_FILLTRANSPARENCE,
    XATTR_GRADIENTSTEPCOUNT,
    XATTR_FILLBMP_TILE,
    XATTR_FILLBMP_POS,
    XATTR_FILLBMP_SIZEX,
    XATTR_FILLBMP_SIZEY,
    XATTR_FILLFLOATTRANSPARENCE,
    XATTR_FILLBMP_SIZELOG,
    XATTR_FILLBMP_TILEOFFSETX,
    XATTR_FILLBMP_TILEOFFSETY,
    XATTR_FILLBMP_STRETCH,
    XATTR_FILLBMP_POSOFFSETX,
    XATTR_FILLBMP_POSOFFSETY,
    XATTR_FILLBACKGROUND,
    XATTR_FILLRESERVED2,
    XATTR_FILLRESERVED3,
    XATTR_FILLRESERVED4,
    XATTR_FILLRESERVED5,
    XATTR_FILLRESERVED6,
    XATTR_FILLRESERVED7,
    XATTR_FILLRESERVED8,
    XATTR_FILLRESERVED10,
    XATTR_FILLRESERVED11,
    XATTR_FILLRESERVED_LAST,
    XATTR_SET_FILL,
    XATTR_FORMTXTSTYLE,
    XATTR_FORMTXTADJUST,
    XATTR_FORMTXTDISTANCE,
    XATTR_FORMTXTSTART,
    XATTR_FORMTXTMIRROR,
    XATTR_FORMTXTOUTLINE,
    XATTR_FORMTXTSHADOW,
    XATTR_FORMTXTSHDWCOLOR,
    XATTR_FORMTXTSHDWXVAL,
    XATTR_FORMTXTSHDWYVAL,
    XATTR_FORMTXTSTDFORM,
    XATTR_FORMTXTHIDEFORM,
    XATTR_FORMTXTSHDWTRANSP,
    XATTR_FTRESERVED2,
    XATTR_FTRESERVED3,
    XATTR_FTRESERVED4,
    XATTR_FTRESERVED5,
    XATTR_FTRESERVED_LAST,
    XATTR_SET_TEXT,

    // SDR
    SDRATTR_SHADOW,
    SDRATTR_SHADOWCOLOR,
    SDRATTR_SHADOWXDIST,
    SDRATTR_SHADOWYDIST,
    SDRATTR_SHADOWTRANSPARENCE,
    SDRATTR_SHADOW3D,
    SDRATTR_SHADOWPERSP,
    SDRATTR_SHADOWRESERVE1,
    SDRATTR_SHADOWRESERVE2,
    SDRATTR_SHADOWRESERVE3,
    SDRATTR_SHADOWRESERVE4,
    SDRATTR_SHADOWRESERVE5,
    SDRATTR_SET_SHADOW,

    SDRATTR_CAPTIONTYPE,
    SDRATTR_CAPTIONFIXEDANGLE,
    SDRATTR_CAPTIONANGLE,
    SDRATTR_CAPTIONGAP,
    SDRATTR_CAPTIONESCDIR,
    SDRATTR_CAPTIONESCISREL,
    SDRATTR_CAPTIONESCREL,
    SDRATTR_CAPTIONESCABS,
    SDRATTR_CAPTIONLINELEN,
    SDRATTR_CAPTIONFITLINELEN,
    SDRATTR_CAPTIONRESERVE1,
    SDRATTR_CAPTIONRESERVE2,
    SDRATTR_CAPTIONRESERVE3,
    SDRATTR_CAPTIONRESERVE4,
    SDRATTR_CAPTIONRESERVE5,
    SDRATTR_SET_CAPTION,

    SDRATTR_SET_OUTLINER,

    SDRATTR_ECKENRADIUS,
    SDRATTR_TEXT_MINFRAMEHEIGHT,
    SDRATTR_TEXT_AUTOGROWHEIGHT,
    SDRATTR_TEXT_FITTOSIZE,
    SDRATTR_TEXT_LEFTDIST,
    SDRATTR_TEXT_RIGHTDIST,
    SDRATTR_TEXT_UPPERDIST,
    SDRATTR_TEXT_LOWERDIST,
    SDRATTR_TEXT_VERTADJUST,
    SDRATTR_TEXT_MAXFRAMEHEIGHT,
    SDRATTR_TEXT_MINFRAMEWIDTH,
    SDRATTR_TEXT_MAXFRAMEWIDTH,
    SDRATTR_TEXT_AUTOGROWWIDTH,
    SDRATTR_TEXT_HORZADJUST,
    SDRATTR_TEXT_ANIKIND,
    SDRATTR_TEXT_ANIDIRECTION,
    SDRATTR_TEXT_ANISTARTINSIDE,
    SDRATTR_TEXT_ANISTOPINSIDE,
    SDRATTR_TEXT_ANICOUNT,
    SDRATTR_TEXT_ANIDELAY,
    SDRATTR_TEXT_ANIAMOUNT,
    SDRATTR_TEXT_CONTOURFRAME,
    SDRATTR_AUTOSHAPE_ADJUSTMENT,
    SDRATTR_XMLATTRIBUTES,
    SDRATTR_RESERVE15,
    SDRATTR_RESERVE16,
    SDRATTR_RESERVE17,
    SDRATTR_RESERVE18,
    SDRATTR_RESERVE19,
    SDRATTR_SET_MISC,

    SDRATTR_EDGEKIND,
    SDRATTR_EDGENODE1HORZDIST,
    SDRATTR_EDGENODE1VERTDIST,
    SDRATTR_EDGENODE2HORZDIST,
    SDRATTR_EDGENODE2VERTDIST,
    SDRATTR_EDGENODE1GLUEDIST,
    SDRATTR_EDGENODE2GLUEDIST,
    SDRATTR_EDGELINEDELTAANZ,
    SDRATTR_EDGELINE1DELTA,
    SDRATTR_EDGELINE2DELTA,
    SDRATTR_EDGELINE3DELTA,
    SDRATTR_EDGERESERVE02,
    SDRATTR_EDGERESERVE03,
    SDRATTR_EDGERESERVE04,
    SDRATTR_EDGERESERVE05,
    SDRATTR_EDGERESERVE06,
    SDRATTR_EDGERESERVE07,
    SDRATTR_EDGERESERVE08,
    SDRATTR_EDGERESERVE09,
    SDRATTR_SET_EDGE,

    SDRATTR_MEASUREKIND,
    SDRATTR_MEASURETEXTHPOS,
    SDRATTR_MEASURETEXTVPOS,
    SDRATTR_MEASURELINEDIST,
    SDRATTR_MEASUREHELPLINEOVERHANG,
    SDRATTR_MEASUREHELPLINEDIST,
    SDRATTR_MEASUREHELPLINE1LEN,
    SDRATTR_MEASUREHELPLINE2LEN,
    SDRATTR_MEASUREBELOWREFEDGE,
    SDRATTR_MEASURETEXTROTA90,
    SDRATTR_MEASURETEXTUPSIDEDOWN,
    SDRATTR_MEASUREOVERHANG,
    SDRATTR_MEASUREUNIT,
    SDRATTR_MEASURESCALE,
    SDRATTR_MEASURESHOWUNIT,
    SDRATTR_MEASUREFORMATSTRING,
    SDRATTR_MEASURETEXTAUTOANGLE,
    SDRATTR_MEASURETEXTAUTOANGLEVIEW,
    SDRATTR_MEASURETEXTISFIXEDANGLE,
    SDRATTR_MEASURETEXTFIXEDANGLE,
    SDRATTR_MEASUREDECIMALPLACES,
    SDRATTR_MEASURERESERVE05,
    SDRATTR_MEASURERESERVE06,
    SDRATTR_MEASURERESERVE07,
    SDRATTR_SET_MEASURE,

    SDRATTR_CIRCKIND,
    SDRATTR_CIRCSTARTANGLE,
    SDRATTR_CIRCENDANGLE,
    SDRATTR_CIRCRESERVE0,
    SDRATTR_CIRCRESERVE1,
    SDRATTR_CIRCRESERVE2,
    SDRATTR_CIRCRESERVE3,
    SDRATTR_SET_CIRC,

    SDRATTR_OBJMOVEPROTECT,
    SDRATTR_OBJSIZEPROTECT,
    SDRATTR_OBJPRINTABLE,
    SDRATTR_LAYERID,
    SDRATTR_LAYERNAME,
    SDRATTR_OBJECTNAME,
    SDRATTR_ALLPOSITIONX,
    SDRATTR_ALLPOSITIONY,
    SDRATTR_ALLSIZEWIDTH,
    SDRATTR_ALLSIZEHEIGHT,
    SDRATTR_ONEPOSITIONX,
    SDRATTR_ONEPOSITIONY,
    SDRATTR_ONESIZEWIDTH,
    SDRATTR_ONESIZEHEIGHT,
    SDRATTR_LOGICSIZEWIDTH,
    SDRATTR_LOGICSIZEHEIGHT,
    SDRATTR_ROTATEANGLE,
    SDRATTR_SHEARANGLE,
    SDRATTR_MOVEX,
    SDRATTR_MOVEY,
    SDRATTR_RESIZEXONE,
    SDRATTR_RESIZEYONE,
    SDRATTR_ROTATEONE,
    SDRATTR_HORZSHEARONE,
    SDRATTR_VERTSHEARONE,
    SDRATTR_RESIZEXALL,
    SDRATTR_RESIZEYALL,
    SDRATTR_ROTATEALL,
    SDRATTR_HORZSHEARALL,
    SDRATTR_VERTSHEARALL,
    SDRATTR_TRANSFORMREF1X,
    SDRATTR_TRANSFORMREF1Y,
    SDRATTR_TRANSFORMREF2X,
    SDRATTR_TRANSFORMREF2Y,
    SDRATTR_TEXTDIRECTION,
    SDRATTR_NOTPERSISTRESERVE2,
    SDRATTR_NOTPERSISTRESERVE3,
    SDRATTR_NOTPERSISTRESERVE4,
    SDRATTR_NOTPERSISTRESERVE5,
    SDRATTR_NOTPERSISTRESERVE6,
    SDRATTR_NOTPERSISTRESERVE7,
    SDRATTR_NOTPERSISTRESERVE8,
    SDRATTR_NOTPERSISTRESERVE9,
    SDRATTR_NOTPERSISTRESERVE10,
    SDRATTR_NOTPERSISTRESERVE11,
    SDRATTR_NOTPERSISTRESERVE12,
    SDRATTR_NOTPERSISTRESERVE13,
    SDRATTR_NOTPERSISTRESERVE14,
    SDRATTR_NOTPERSISTRESERVE15,

    SDRATTR_GRAFRED,
    SDRATTR_GRAFGREEN,
    SDRATTR_GRAFBLUE,
    SDRATTR_GRAFLUMINANCE,
    SDRATTR_GRAFCONTRAST,
    SDRATTR_GRAFGAMMA,
    SDRATTR_GRAFTRANSPARENCE,
    SDRATTR_GRAFINVERT,
    SDRATTR_GRAFMODE,
    SDRATTR_GRAFCROP,
    SDRATTR_GRAFRESERVE3,
    SDRATTR_GRAFRESERVE4,
    SDRATTR_GRAFRESERVE5,
    SDRATTR_GRAFRESERVE6,
    SDRATTR_SET_GRAF,

    SDRATTR_3DOBJ_PERCENT_DIAGONAL,
    SDRATTR_3DOBJ_BACKSCALE,
    SDRATTR_3DOBJ_DEPTH,
    SDRATTR_3DOBJ_HORZ_SEGS,
    SDRATTR_3DOBJ_VERT_SEGS,
    SDRATTR_3DOBJ_END_ANGLE,
    SDRATTR_3DOBJ_DOUBLE_SIDED,
    SDRATTR_3DOBJ_NORMALS_KIND,
    SDRATTR_3DOBJ_NORMALS_INVERT,
    SDRATTR_3DOBJ_TEXTURE_PROJ_X,
    SDRATTR_3DOBJ_TEXTURE_PROJ_Y,
    SDRATTR_3DOBJ_SHADOW_3D,
    SDRATTR_3DOBJ_MAT_COLOR,
    SDRATTR_3DOBJ_MAT_EMISSION,
    SDRATTR_3DOBJ_MAT_SPECULAR,
    SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY,
    SDRATTR_3DOBJ_TEXTURE_KIND,
    SDRATTR_3DOBJ_TEXTURE_MODE,
    SDRATTR_3DOBJ_TEXTURE_FILTER,

    SDRATTR_3DOBJ_SMOOTH_NORMALS,
    SDRATTR_3DOBJ_SMOOTH_LIDS,
    SDRATTR_3DOBJ_CHARACTER_MODE,
    SDRATTR_3DOBJ_CLOSE_FRONT,
    SDRATTR_3DOBJ_CLOSE_BACK,

    SDRATTR_3DOBJ_RESERVED_06,
    SDRATTR_3DOBJ_RESERVED_07,
    SDRATTR_3DOBJ_RESERVED_08,
    SDRATTR_3DOBJ_RESERVED_09,
    SDRATTR_3DOBJ_RESERVED_10,
    SDRATTR_3DOBJ_RESERVED_11,
    SDRATTR_3DOBJ_RESERVED_12,
    SDRATTR_3DOBJ_RESERVED_13,
    SDRATTR_3DOBJ_RESERVED_14,
    SDRATTR_3DOBJ_RESERVED_15,
    SDRATTR_3DOBJ_RESERVED_16,
    SDRATTR_3DOBJ_RESERVED_17,
    SDRATTR_3DOBJ_RESERVED_18,
    SDRATTR_3DOBJ_RESERVED_19,
    SDRATTR_3DOBJ_RESERVED_20,

    SDRATTR_3DSCENE_PERSPECTIVE,
    SDRATTR_3DSCENE_DISTANCE,
    SDRATTR_3DSCENE_FOCAL_LENGTH,
    SDRATTR_3DSCENE_TWO_SIDED_LIGHTING,
    SDRATTR_3DSCENE_LIGHTCOLOR_1,
    SDRATTR_3DSCENE_LIGHTCOLOR_2,
    SDRATTR_3DSCENE_LIGHTCOLOR_3,
    SDRATTR_3DSCENE_LIGHTCOLOR_4,
    SDRATTR_3DSCENE_LIGHTCOLOR_5,
    SDRATTR_3DSCENE_LIGHTCOLOR_6,
    SDRATTR_3DSCENE_LIGHTCOLOR_7,
    SDRATTR_3DSCENE_LIGHTCOLOR_8,
    SDRATTR_3DSCENE_AMBIENTCOLOR,
    SDRATTR_3DSCENE_LIGHTON_1,
    SDRATTR_3DSCENE_LIGHTON_2,
    SDRATTR_3DSCENE_LIGHTON_3,
    SDRATTR_3DSCENE_LIGHTON_4,
    SDRATTR_3DSCENE_LIGHTON_5,
    SDRATTR_3DSCENE_LIGHTON_6,
    SDRATTR_3DSCENE_LIGHTON_7,
    SDRATTR_3DSCENE_LIGHTON_8,
    SDRATTR_3DSCENE_LIGHTDIRECTION_1,
    SDRATTR_3DSCENE_LIGHTDIRECTION_2,
    SDRATTR_3DSCENE_LIGHTDIRECTION_3,
    SDRATTR_3DSCENE_LIGHTDIRECTION_4,
    SDRATTR_3DSCENE_LIGHTDIRECTION_5,
    SDRATTR_3DSCENE_LIGHTDIRECTION_6,
    SDRATTR_3DSCENE_LIGHTDIRECTION_7,
    SDRATTR_3DSCENE_LIGHTDIRECTION_8,
    SDRATTR_3DSCENE_SHADOW_SLANT,
    SDRATTR_3DSCENE_SHADE_MODE,
    SDRATTR_3DSCENE_RESERVED_01,
    SDRATTR_3DSCENE_RESERVED_02,
    SDRATTR_3DSCENE_RESERVED_03,
    SDRATTR_3DSCENE_RESERVED_04,
    SDRATTR_3DSCENE_RESERVED_05,
    SDRATTR_3DSCENE_RESERVED_06,
    SDRATTR_3DSCENE_RESERVED_07,
    SDRATTR_3DSCENE_RESERVED_08,
    SDRATTR_3DSCENE_RESERVED_09,
    SDRATTR_3DSCENE_RESERVED_10,
    SDRATTR_3DSCENE_RESERVED_11,
    SDRATTR_3DSCENE_RESERVED_12,
    SDRATTR_3DSCENE_RESERVED_13,
    SDRATTR_3DSCENE_RESERVED_14,
    SDRATTR_3DSCENE_RESERVED_15,
    SDRATTR_3DSCENE_RESERVED_16,
    SDRATTR_3DSCENE_RESERVED_17,
    SDRATTR_3DSCENE_RESERVED_18,
    SDRATTR_3DSCENE_RESERVED_19,
    SDRATTR_3DSCENE_RESERVED_20
  };

  //! destructor
  virtual ~StarAttribute();
  //! returns the attribute type
  Type getType() const
  {
    return m_type;
  }
  //! create a new attribute
  virtual std::shared_ptr<StarAttribute> create() const=0;
  //! read an attribute zone
  virtual bool read(StarZone &zone, int vers, long endPos, StarObject &document)=0;
  //! add to a state
  void addTo(StarState &state) const
  {
    std::set<StarAttribute const *> done;
    addTo(state, done);
  }
  //! add to send the zone data
  bool send(STOFFListenerPtr &listener, StarState &state) const
  {
    std::set<StarAttribute const *> done;
    return send(listener, state, done);
  }
  //! add to a state(internal)
  virtual void addTo(StarState &/*state*/, std::set<StarAttribute const *> &/*done*/) const
  {
  }
  //! try to send the child zone(internal)
  virtual bool send(STOFFListenerPtr &/*listener*/, StarState &/*state*/, std::set<StarAttribute const *> &/*done*/) const
  {
    return false;
  }
  //! returns the debug name
  std::string const &getDebugName() const
  {
    return m_debugName;
  }
  //! debug function to print the data
  virtual void print(libstoff::DebugStream &o, std::set<StarAttribute const *> &done) const
  {
    if (done.find(this)!=done.end()) {
      o << m_debugName << ",";
      return;
    }
    done.insert(this);
    printData(o);
  }
  //! debug function to print the data
  virtual void printData(libstoff::DebugStream &o) const
  {
    o << m_debugName << ",";
  }
protected:
  //! constructor
  StarAttribute(Type type, std::string const &debugName)
    : m_type(type)
    , m_debugName(debugName)
  {
  }
  //! copy constructor
  explicit StarAttribute(StarAttribute const &orig)
    : m_type(orig.m_type)
    , m_debugName(orig.m_debugName)
  {
  }

  //
  // data
  //

  //! the type
  Type m_type;
  //! the debug name
  std::string m_debugName;

private:
  StarAttribute &operator=(StarAttribute const &orig);
};

//! a boolean attribute
class StarAttributeBool : public StarAttribute
{
public:
  //! constructor
  StarAttributeBool(Type type, std::string const &debugName, bool value)
    : StarAttribute(type, debugName)
    , m_value(value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeBool(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int ver, long endPos, StarObject &object) override;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName;
    if (m_value) o << "=true";
    o << ",";
  }
protected:
  //! copy constructor
  StarAttributeBool(StarAttributeBool const &) = default;
  // the bool value
  bool m_value;
};

//! a color attribute
class StarAttributeColor : public StarAttribute
{
public:
  //! constructor
  StarAttributeColor(Type type, std::string const &debugName, STOFFColor const &value)
    : StarAttribute(type, debugName)
    , m_value(value)
    , m_defValue(value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeColor(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName << "[col=" << m_value << "],";
  }
protected:
  //! copy constructor
  StarAttributeColor(StarAttributeColor const &) = default;
  //! the color value
  STOFFColor m_value;
  //! the default value
  STOFFColor m_defValue;
};

//! a double attribute
class StarAttributeDouble : public StarAttribute
{
public:
  //! constructor
  StarAttributeDouble(Type type, std::string const &debugName, double value) : StarAttribute(type, debugName), m_value(value)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeDouble(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;

  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName;
    if (m_value<0 || m_value>0) o << "=" << m_value;
    o << ",";
  }
protected:
  //! copy constructor
  StarAttributeDouble(StarAttributeDouble const &) = default;
  // the double value
  double m_value;
};

//! an integer attribute
class StarAttributeInt : public StarAttribute
{
public:
  //! constructor
  StarAttributeInt(Type type, std::string const &debugName, int intSize, int value)
    : StarAttribute(type, debugName)
    , m_value(value)
    , m_intSize(intSize)
  {
    if (intSize!=1 && intSize!=2 && intSize!=4) {
      STOFF_DEBUG_MSG(("StarAttributeInt: bad num size\n"));
      m_intSize=0;
    }
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeInt(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName;
    if (m_value) o << "=" << m_value;
    o << ",";
  }

protected:
  //! copy constructor
  StarAttributeInt(StarAttributeInt const &) = default;
  // the int value
  int m_value;
  // number of byte 1,2,4
  int m_intSize;
};

//! a unsigned integer attribute
class StarAttributeUInt : public StarAttribute
{
public:
  //! constructor
  StarAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value)
    : StarAttribute(type, debugName)
    , m_value(value)
    , m_intSize(intSize)
  {
    if (intSize!=1 && intSize!=2 && intSize!=4) {
      STOFF_DEBUG_MSG(("StarAttributeUInt: bad num size\n"));
      m_intSize=0;
    }
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeUInt(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName;
    if (m_value) o << "=" << m_value;
    o << ",";
  }
protected:
  //! copy constructor
  StarAttributeUInt(StarAttributeUInt const &) = default;
  // the int value
  unsigned int m_value;
  // number of byte 1,2,4
  int m_intSize;
};

//! an Vec2i attribute
class StarAttributeVec2i : public StarAttribute
{
public:
  //! constructor
  StarAttributeVec2i(Type type, std::string const &debugName, int intSize, STOFFVec2i value=STOFFVec2i(0,0))
    : StarAttribute(type, debugName)
    , m_value(value)
    , m_intSize(intSize)
  {
    if (intSize!=1 && intSize!=2 && intSize!=4) {
      STOFF_DEBUG_MSG(("StarAttributeVec2i: bad num size\n"));
      m_intSize=0;
    }
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeVec2i(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
  //! debug function to print the data
  void printData(libstoff::DebugStream &o) const override
  {
    o << m_debugName;
    if (m_value!=STOFFVec2i(0,0)) o << "=" << m_value;
    o << ",";
  }

protected:
  //! copy constructor
  StarAttributeVec2i(StarAttributeVec2i const &) = default;
  // the int value
  STOFFVec2i m_value;
  // number of byte 1,2,4
  int m_intSize;
};

//! a list of item attribute of StarAttributeInternal
class StarAttributeItemSet : public StarAttribute
{
public:
  //! constructor
  StarAttributeItemSet(Type type, std::string const &debugName, std::vector<STOFFVec2i> const &limits)
    : StarAttribute(type, debugName)
    , m_limits(limits)
    , m_itemSet()
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeItemSet(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
  //! debug function to print the data
  void print(libstoff::DebugStream &o, std::set<StarAttribute const *> &done) const override;

protected:
  //! add to a state
  void addTo(StarState &state, std::set<StarAttribute const *> &done) const override;
  //! try to send the sone data
  bool send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const override;

  //! copy constructor
  StarAttributeItemSet(StarAttributeItemSet const &) = default;
  //! the pool limits id
  std::vector<STOFFVec2i> m_limits;
  //! the list of items
  StarItemSet m_itemSet;
};

//! void attribute of StarAttribute
class StarAttributeVoid : public StarAttribute
{
public:
  //! constructor
  StarAttributeVoid(Type type, std::string const &debugName)
    : StarAttribute(type, debugName)
  {
  }
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const override
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeVoid(*this));
  }
  //! read a zone
  bool read(StarZone &zone, int vers, long endPos, StarObject &object) override;
};

/** \brief the main class to read/.. a StarOffice attribute
 *
 *
 *
 */
class StarAttributeManager
{
public:
  //! constructor
  StarAttributeManager();
  //! destructor
  virtual ~StarAttributeManager();


  //! try to read an attribute and return it
  std::shared_ptr<StarAttribute> readAttribute(StarZone &zone, int which, int vers, long endPos, StarObject &document);
  //! try to return the default attribute
  std::shared_ptr<StarAttribute> getDefaultAttribute(int which);
  //! return a dummy attribute
  static std::shared_ptr<StarAttribute> getDummyAttribute(int type=-1);

protected:
  //
  // data
  //
private:
  //! the state
  std::shared_ptr<StarAttributeInternal::State> m_state;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
