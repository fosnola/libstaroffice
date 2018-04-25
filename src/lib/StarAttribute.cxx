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

#include "StarBitmap.hxx"
#include "StarCellAttribute.hxx"
#include "StarCharAttribute.hxx"
#include "StarFrameAttribute.hxx"
#include "StarGraphicAttribute.hxx"
#include "StarPageAttribute.hxx"
#include "StarParagraphAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarObject.hxx"
#include "StarObjectSmallText.hxx"
#include "StarObjectText.hxx"
#include "StarState.hxx"
#include "StarZone.hxx"

#include "StarAttribute.hxx"

/** Internal: the structures of a StarAttribute */
namespace StarAttributeInternal
{
//! xml attribute of StarAttributeInternal
class StarAttributeXML final : public StarAttributeVoid
{
  // call SfxPoolItem::Create which does nothing
public:
  //! constructor
  StarAttributeXML(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! destructor
  ~StarAttributeXML() override;
  //! create a new attribute
  std::shared_ptr<StarAttribute> create() const final
  {
    return std::shared_ptr<StarAttribute>(new StarAttributeXML(*this));
  }
};

StarAttributeXML::~StarAttributeXML()
{
}
////////////////////////////////////////
//! Internal: the state of a StarAttribute
struct State {
  //! constructor
  State() : m_whichToAttributeMap()
  {
    initAttributeMap();
  }
  //! init the attribute map list
  void initAttributeMap();
  //! a map which to an attribute
  std::map<int, std::shared_ptr<StarAttribute> > m_whichToAttributeMap;
protected:
  //! add a void attribute
  void addAttributeVoid(StarAttribute::Type type, std::string const &debugName)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeVoid(type,debugName));
  }
  //! add a XML attribute
  void addAttributeXML(StarAttribute::Type type, std::string const &debugName)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeXML(type,debugName));
  }
  //! add a bool attribute
  void addAttributeBool(StarAttribute::Type type, std::string const &debugName, bool defValue)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeBool(type,debugName, defValue));
  }
  //! add a int attribute
  void addAttributeInt(StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeInt(type,debugName, numBytes, defValue));
  }
  //! add a unsigned int attribute
  void addAttributeUInt(StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeUInt(type,debugName, numBytes, defValue));
  }
  //! add a double attribute
  void addAttributeDouble(StarAttribute::Type type, std::string const &debugName, double defValue)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeDouble(type,debugName, defValue));
  }
  //! add a color attribute
  void addAttributeColor(StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeColor(type,debugName, defValue));
  }
  //! add a itemSet attribute
  void addAttributeItemSet(StarAttribute::Type type, std::string const &debugName, std::vector<STOFFVec2i> const &limits)
  {
    m_whichToAttributeMap[type]=std::shared_ptr<StarAttribute>(new StarAttributeItemSet(type,debugName, limits));
  }
};

void State::initAttributeMap()
{
  StarCellAttribute::addInitTo(m_whichToAttributeMap);
  StarCharAttribute::addInitTo(m_whichToAttributeMap);
  StarFrameAttribute::addInitTo(m_whichToAttributeMap);
  StarGraphicAttribute::addInitTo(m_whichToAttributeMap);
  StarParagraphAttribute::addInitTo(m_whichToAttributeMap);
  StarPageAttribute::addInitTo(m_whichToAttributeMap);

  std::stringstream s;

  // --- sw --- sw_init.cxx
  addAttributeUInt(StarAttribute::ATTR_FRM_FILL_ORDER,"frm[fill,order]",1,0); // topDown
  addAttributeUInt(StarAttribute::ATTR_FRM_PAPER_BIN,"frm[paperBin]",1,0xFF); // settings
  addAttributeBool(StarAttribute::ATTR_FRM_PRINT,"print",true);
  addAttributeBool(StarAttribute::ATTR_FRM_OPAQUE,"opaque",true);
  addAttributeBool(StarAttribute::ATTR_FRM_KEEP,"frm[keep]",false); // keep together?
  addAttributeBool(StarAttribute::ATTR_FRM_EDIT_IN_READONLY,"edit[readOnly]",false);
  addAttributeBool(StarAttribute::ATTR_FRM_COLUMNBALANCE,"col[noBalanced]", true);
  addAttributeUInt(StarAttribute::ATTR_FRM_FRAMEDIR,"frame[dir]",2,4); // frame environment
  addAttributeBool(StarAttribute::ATTR_FRM_HEADER_FOOTER_EAT_SPACING,"headFoot[eat,spacing]",false);
  addAttributeBool(StarAttribute::ATTR_FRM_FRMATTR_DUMMY9,"grfDummy9", false);

  addAttributeInt(StarAttribute::ATTR_GRF_ROTATION,"grf[rotation]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_LUMINANCE,"grf[luminance]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CONTRAST,"grf[contrast]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CHANNELR,"grf[channelR]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CHANNELG,"grf[channelG]",2,0);
  addAttributeInt(StarAttribute::ATTR_GRF_CHANNELB,"grf[channelB]",2,0);
  addAttributeDouble(StarAttribute::ATTR_GRF_GAMMA,"grf[gamma]",1);
  addAttributeBool(StarAttribute::ATTR_GRF_INVERT,"grf[invert]", false);
  addAttributeUInt(StarAttribute::ATTR_GRF_TRANSPARENCY,"grf[transparency]",1,0);
  addAttributeUInt(StarAttribute::ATTR_GRF_DRAWMODE,"grf[draw,mode]",2,0);
  for (int type=StarAttribute::ATTR_GRF_DUMMY1; type<=StarAttribute::ATTR_GRF_DUMMY5; ++type) {
    s.str("");
    s << "grafDummy" << type-StarAttribute::ATTR_GRF_DUMMY1+1;
    addAttributeBool(StarAttribute::Type(type), s.str(), false);
  }
  // --- ee --- svx_editdoc.cxx and svx_eerdll.cxx
  addAttributeVoid(StarAttribute::ATTR_EE_CHR_RUBI_DUMMY, "chr[rubi,dummy]");
  addAttributeXML(StarAttribute::ATTR_EE_CHR_XMLATTRIBS, "chr[xmlAttrib]");
  // --- sch --- sch_itempool.cxx
  addAttributeBool(StarAttribute::ATTR_SCH_DATADESCR_SHOW_SYM,"dataDescr[showSym]", false);
  addAttributeUInt(StarAttribute::ATTR_SCH_DATADESCR_DESCR,"dataDescr[descr]",2,0); // none
  addAttributeUInt(StarAttribute::ATTR_SCH_LEGEND_POS,"legend[pos]",2,3); // right
  addAttributeUInt(StarAttribute::ATTR_SCH_TEXT_ORIENT,"text[orient]",2,1); // standard
  addAttributeUInt(StarAttribute::ATTR_SCH_TEXT_ORDER,"text[order]",2,0); // side by side

  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_MIN,"xAxis[autoMin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MIN,"yAxis[autoMin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MIN,"zAxis[autoMin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_MIN,"axis[autoMin]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_MIN,"xAxis[min]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_MIN,"yAxis[min]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_MIN,"zAxis[min]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_MIN,"axis[min]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_MAX,"xAxis[autoMax]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_MAX,"yAxis[autoMax]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_MAX,"zAxis[autoMax]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_MAX,"axis[autoMax]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_MAX,"xAxis[max]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_MAX,"yAxis[max]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_MAX,"zAxis[max]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_MAX,"axis[max]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_MAIN,"xAxis[autoStepMain]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_MAIN,"yAxis[autoStepMain]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_MAIN,"zAxis[autoStepMain]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_MAIN,"axis[autoStepMain]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_STEP_MAIN,"xAxis[step,main]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_STEP_MAIN,"yAxis[step,main]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_STEP_MAIN,"zAxis[step,main]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_STEP_MAIN,"axis[step,main]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_STEP_HELP,"xAxis[autoStepHelp]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_STEP_HELP,"yAxis[autoStepHelp]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_STEP_HELP,"zAxis[autoStepHelp]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_STEP_HELP,"axis[autoStepHelp]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_STEP_HELP,"xAxis[step,help]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_STEP_HELP,"yAxis[step,help]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_STEP_HELP,"zAxis[step,help]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_STEP_HELP,"axis[step,help]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_LOGARITHM,"xAxis[logarithm]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_LOGARITHM,"yAxis[logarithm]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_LOGARITHM,"zAxis[logarithm]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_LOGARITHM,"axis[logarithm]", false);
  addAttributeDouble(StarAttribute::ATTR_SCH_X_AXIS_ORIGIN,"xAxis[origin]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Y_AXIS_ORIGIN,"yAxis[origin]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_Z_AXIS_ORIGIN,"zAxis[origin]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_AXIS_ORIGIN,"axis[origin]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_X_AXIS_AUTO_ORIGIN,"xAxis[autoOrigin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Y_AXIS_AUTO_ORIGIN,"yAxis[autoOrigin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_Z_AXIS_AUTO_ORIGIN,"zAxis[autoOrigin]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_AUTO_ORIGIN,"axis[autoOrigin]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_AXISTYPE, "axis[type]",4, 0); // XAxis
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY0, "sch[dummy0]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY1, "sch[dummy1]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY2, "sch[dummy2]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY3, "sch[dummy3]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_DUMMY_END, "sch[dummy4]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_STAT_KIND_ERROR, "stat[error,kind]",4, 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_PERCENT,"stat[percent]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_BIGERROR,"stat[big,error]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_CONSTPLUS,"stat[const,plus]", 0);
  addAttributeDouble(StarAttribute::ATTR_SCH_STAT_CONSTMINUS,"stat[const,minus]", 0);
  addAttributeBool(StarAttribute::ATTR_SCH_STAT_AVERAGE,"stat[average]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_STAT_REGRESSTYPE, "stat[regress,type]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_STAT_INDICATE, "stat[indicate]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DEGREES, "text[degrees]",4, 0);
  addAttributeBool(StarAttribute::ATTR_SCH_TEXT_OVERLAP,"text[overlap]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY0, "text[dummy0]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY1, "text[dummy1]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY2, "text[dummy2]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_TEXT_DUMMY3, "text[dummy3]",4, 0);

  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_DEEP,"style[deep]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_3D,"style[3d]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_VERTICAL,"style[vertical]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_BASETYPE, "style[base,type]",4, 0);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_LINES,"style[lines]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_PERCENT,"style[percent]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STYLE_STACKED,"style[stacked]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_SPLINES, "style[splines]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_SYMBOL, "style[symbol]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_STYLE_SHAPE, "style[shape]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_AXIS, "axis",4, 2);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_TICKS, "axis[ticks]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_HELPTICKS, "axis[help,ticks]",4, 2);
  addAttributeUInt(StarAttribute::ATTR_SCH_AXIS_NUMFMT,"axis[numFmt]",4,0);
  addAttributeUInt(StarAttribute::ATTR_SCH_AXIS_NUMFMTPERCENT,"axis[numFmt,percent]",4,11);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWAXIS,"axis[show]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWDESCR,"axis[showDescr]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWMAINGRID,"axis[show,mainGrid]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_SHOWHELPGRID,"axis[show,helpGrid]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_AXIS_TOPDOWN,"axis[topDown]", false);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY0, "axis[dummy0]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY1, "axis[dummy1]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY2, "axis[dummy2]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_AXIS_DUMMY3, "axis[dummy3]",4, 0);

  addAttributeInt(StarAttribute::ATTR_SCH_BAR_OVERLAP, "bar[overlap]",4, 0);
  addAttributeInt(StarAttribute::ATTR_SCH_BAR_GAPWIDTH, "bar[gap,width]",4, 0);

  addAttributeBool(StarAttribute::ATTR_SCH_STOCK_VOLUME,"stock[volume]", false);
  addAttributeBool(StarAttribute::ATTR_SCH_STOCK_UPDOWN,"stock[updown]", false);
  addAttributeXML(StarAttribute::ATTR_SCH_USER_DEFINED_ATTR,"sch[userDefined]");

  // --- xattr --- svx_xpool.cxx
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSTYLE,"formText[style]",2,4); // None
  addAttributeUInt(StarAttribute::XATTR_FORMTXTADJUST,"formText[adjust]",2,3); // Adjust
  addAttributeInt(StarAttribute::XATTR_FORMTXTDISTANCE, "formText[distance]",4, 0); // metric
  addAttributeInt(StarAttribute::XATTR_FORMTXTSTART, "formText[start]",4, 0); // metric
  addAttributeBool(StarAttribute::XATTR_FORMTXTMIRROR,"formText[mirror]", false);
  addAttributeBool(StarAttribute::XATTR_FORMTXTOUTLINE,"formText[outline]", false);
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSHADOW,"formText[shadow]",2,0); // None
  addAttributeInt(StarAttribute::XATTR_FORMTXTSHDWXVAL, "formText[shadow,xDist]",4, 0); // metric
  addAttributeInt(StarAttribute::XATTR_FORMTXTSHDWYVAL, "formText[shadow,yDist]",4, 0); // metric
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSTDFORM,"formText[stdForm]",2,0); // None
  addAttributeBool(StarAttribute::XATTR_FORMTXTHIDEFORM,"formText[hide]", false);
  addAttributeUInt(StarAttribute::XATTR_FORMTXTSHDWTRANSP,"formText[shadow,trans]",2,0);
  for (int type=StarAttribute::XATTR_FTRESERVED2; type<=StarAttribute::XATTR_FTRESERVED_LAST; ++type) {
    s.str("");
    s << "form[reserved" << type-StarAttribute::XATTR_FTRESERVED2+2 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  // ---- sdr ---- svx_svdattr.cxx

  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_PERCENT_DIAGONAL, "obj3d[percent,diagonal]",2,10);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_BACKSCALE, "obj3d[backscale]",2,100);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_DEPTH, "obj3d[depth]",4,1000);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_HORZ_SEGS, "obj3d[hori,segments]",4,24);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_VERT_SEGS, "obj3d[vert,segments]",4,24);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_END_ANGLE, "obj3d[endAngle]",4,3600);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_DOUBLE_SIDED,"obj3d[doubleSided]", false);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_NORMALS_KIND, "obj3d[normal,kind]",2,0);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_NORMALS_INVERT,"obj3d[invertNormal]", false);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_X, "obj3d[texture,projX]",2,0);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_PROJ_Y, "obj3d[texture,projY]",2,0);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_SHADOW_3D,"obj3d[3dShadow]", false);
  addAttributeColor(StarAttribute::SDRATTR_3DOBJ_MAT_COLOR, "obj3d[mat,color]",STOFFColor(0xff,0xb8,0,0)); // check order
  addAttributeColor(StarAttribute::SDRATTR_3DOBJ_MAT_EMISSION, "obj3d[mat,emission]",STOFFColor(0,0,0,0));
  addAttributeColor(StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR, "obj3d[mat,specular]",STOFFColor(0xff,0xff,0xff,0));
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY, "obj3d[matSpecIntensity]",2,15);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_KIND, "obj3d[texture,kind]",2,3);
  addAttributeUInt(StarAttribute::SDRATTR_3DOBJ_TEXTURE_MODE, "obj3d[texture,mode]",2,2);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_TEXTURE_FILTER,"obj3d[textureFilter]", false);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_SMOOTH_NORMALS,"obj3d[smoothNormals]", true);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_SMOOTH_LIDS,"obj3d[smoothLids]", false);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_CHARACTER_MODE,"obj3d[charMode]", false);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_CLOSE_FRONT,"obj3d[closeFront]", true);
  addAttributeBool(StarAttribute::SDRATTR_3DOBJ_CLOSE_BACK,"obj3d[closeBack]", true);
  for (int type=StarAttribute::SDRATTR_3DOBJ_RESERVED_06; type<=StarAttribute::SDRATTR_3DOBJ_RESERVED_20; ++type) {
    s.str("");
    s << "obj3d[reserved" << type-StarAttribute::SDRATTR_3DOBJ_RESERVED_06+6 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_PERSPECTIVE, "scene3d[perspective]",2,1); // perspective
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_DISTANCE, "scene3d[distance]",4,100);
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_FOCAL_LENGTH, "scene3d[focal,length]",4,100);
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_SHADOW_SLANT, "scene3d[shadow,slant]",2,0);
  addAttributeUInt(StarAttribute::SDRATTR_3DSCENE_SHADE_MODE, "scene3d[shade,mode]",2,2);
  addAttributeBool(StarAttribute::SDRATTR_3DSCENE_TWO_SIDED_LIGHTING, "scene3d[twoSidedLighting]", false);
  for (int type=StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1; type<=StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_8; ++type) {
    s.str("");
    s << "scene3d[light,color" << type-StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1+1 << "]";
    addAttributeColor(StarAttribute::Type(type), s.str(),
                      type==StarAttribute::SDRATTR_3DSCENE_LIGHTCOLOR_1 ? STOFFColor(0xcc,0xcc,0xcc) : STOFFColor(0,0,0,0));
  }
  addAttributeColor(StarAttribute::SDRATTR_3DSCENE_AMBIENTCOLOR, "scene3d[ambient,color]",STOFFColor(0x66,0x66,0x66,0));
  for (int type=StarAttribute::SDRATTR_3DSCENE_LIGHTON_1; type<=StarAttribute::SDRATTR_3DSCENE_LIGHTON_8; ++type) {
    s.str("");
    s << "scene3d[lighton" << type-StarAttribute::SDRATTR_3DSCENE_LIGHTON_1+1 << "]";
    addAttributeBool(StarAttribute::Type(type), s.str(), type==StarAttribute::SDRATTR_3DSCENE_LIGHTON_1);
  }
  for (int type=StarAttribute::SDRATTR_3DSCENE_RESERVED_01; type<=StarAttribute::SDRATTR_3DSCENE_RESERVED_20; ++type) {
    s.str("");
    s << "scene3d[reserved" << type-StarAttribute::SDRATTR_3DSCENE_RESERVED_01+1 << "]";
    addAttributeVoid(StarAttribute::Type(type), s.str());
  }

  std::vector<STOFFVec2i> limits;
  limits.resize(1);
  limits[0]=STOFFVec2i(3989,4037);  // EE_ITEMS_START, EE_ITEMS_END
  addAttributeItemSet(StarAttribute::SDRATTR_SET_OUTLINER,"setOutliner",limits);
}

}

////////////////////////////////////////////////////////////
// basic attribute function
////////////////////////////////////////////////////////////
StarAttribute::~StarAttribute()
{
}

void StarAttributeItemSet::addTo(StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarAttributeItemSet::addTo: find a cycle\n"));
    return;
  }
  done.insert(this);
  StarItemSet finalSet;
  bool newSet=false;
  if (state.m_global->m_pool && !m_itemSet.m_style.empty()) {
    finalSet=m_itemSet;
    state.m_global->m_pool->updateUsingStyles(finalSet);
    newSet=true;
  }
  StarItemSet const &set=newSet ? finalSet : m_itemSet;
  for (auto const &it : set.m_whichToItemMap) {
    if (it.second && it.second->m_attribute)
      it.second->m_attribute->addTo(state, done);
  }
}

void StarAttributeItemSet::print(libstoff::DebugStream &o, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarAttributeItemSet::print: find a cycle\n"));
    o << "###cycle[" << m_debugName << "]";
    return;
  }
  done.insert(this);
  o << m_debugName;
  if (!m_itemSet.empty()) {
    o << "[";
    for (auto const &it : m_itemSet.m_whichToItemMap) {
      if (it.second && it.second->m_attribute)
        it.second->m_attribute->print(o, done);
      else
        o << "_";
      o << ",";
    }
    o << "]";
  }
  o << ",";
}

bool StarAttributeBool::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  *input>>m_value;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << (m_value ? "true" : "false") << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return pos+1<=endPos;
}

bool StarAttributeColor::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName;
  bool ok=input->readColor(m_value);
  if (!ok) {
    STOFF_DEBUG_MSG(("StarAttributeColor::read: can not read a color\n"));
    f << ",###color,";
  }
  else if (m_value!=m_defValue)
    f << "=" << m_value << ",";
  else
    f << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarAttributeDouble::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  *input>>m_value;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName;
  if (m_value<0 || m_value>0)
    f << "=" << m_value << ",";
  else
    f << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeInt::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (m_intSize) m_value=int(input->readLong(m_intSize));
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeVec2i::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (m_intSize) {
    int dim[2];
    for (int &i : dim) i=int(input->readLong(m_intSize));
    m_value=STOFFVec2i(dim[0],dim[1]);
  }
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeItemSet::read(StarZone &zone, int /*vers*/, long endPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "StarAttribute[" << zone.getRecordLevel() << "]:" << m_debugName << ",";
  bool ok=object.readItemSet(zone, m_limits, endPos, m_itemSet, object.getCurrentPool(false).get());
  if (!ok) f << "###";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return ok && input->tell()<=endPos;
}

bool StarAttributeUInt::read(StarZone &zone, int /*vers*/, long endPos, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  long pos=input->tell();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  if (m_intSize) m_value=static_cast<unsigned int>(input->readULong(m_intSize));
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << "=" << m_value << ",";
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return input->tell()<=endPos;
}

bool StarAttributeVoid::read(StarZone &zone, int /*vers*/, long /*endPos*/, StarObject &/*object*/)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:" << m_debugName << ",";
  ascFile.addPos(input->tell());
  ascFile.addNote(f.str().c_str());
  return true;
}

bool StarAttributeItemSet::send(STOFFListenerPtr &listener, StarState &state, std::set<StarAttribute const *> &done) const
{
  if (done.find(this)!=done.end()) {
    STOFF_DEBUG_MSG(("StarAttributeItemSet::send: find a cycle\n"));
    return false;
  }
  done.insert(this);
  for (auto const &it : m_itemSet.m_whichToItemMap) {
    if (it.second && it.second->m_attribute)
      it.second->m_attribute->send(listener, state, done);
  }
  return true;
}

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////

StarAttributeManager::StarAttributeManager()
  : m_state(new StarAttributeInternal::State)
{
}

StarAttributeManager::~StarAttributeManager()
{
}

std::shared_ptr<StarAttribute> StarAttributeManager::getDummyAttribute(int id)
{
  if (id<=0)
    return std::shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::ATTR_CHR_DUMMY1, "unknownAttribute"));
  std::stringstream s;
  s << "attrib" << id;
  return std::shared_ptr<StarAttribute>(new StarAttributeVoid(StarAttribute::ATTR_CHR_DUMMY1, s.str()));
}

std::shared_ptr<StarAttribute> StarAttributeManager::getDefaultAttribute(int nWhich)
{
  if (m_state->m_whichToAttributeMap.find(nWhich)!=m_state->m_whichToAttributeMap.end() &&
      m_state->m_whichToAttributeMap.find(nWhich)->second)
    return m_state->m_whichToAttributeMap.find(nWhich)->second->create();
  return getDummyAttribute();
}

std::shared_ptr<StarAttribute> StarAttributeManager::readAttribute(StarZone &zone, int nWhich, int nVers, long lastPos, StarObject &object)
{
  STOFFInputStreamPtr input=zone.input();
  libstoff::DebugFile &ascFile=zone.ascii();
  libstoff::DebugStream f;
  f << "Entries(StarAttribute)[" << zone.getRecordLevel() << "]:";

  long pos=input->tell();
  if (m_state->m_whichToAttributeMap.find(nWhich)!=m_state->m_whichToAttributeMap.end() &&
      m_state->m_whichToAttributeMap.find(nWhich)->second) {
    auto attrib=m_state->m_whichToAttributeMap.find(nWhich)->second->create();
    if (!attrib || !attrib->read(zone, nVers, lastPos, object)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read an attribute\n"));
      f << "###bad";
      ascFile.addPos(pos);
      ascFile.addNote(f.str().c_str());
      return std::shared_ptr<StarAttribute>();
    }
    return attrib;
  }

  int val;
  switch (nWhich) {
  case StarAttribute::ATTR_CHR_CHARSETCOLOR: {
    f << "chrAtrCharSetColor=" << input->readULong(1) << ",";
    STOFFColor color;
    if (!input->readColor(color)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a color\n"));
      f << "###aColor,";
      break;
    }
    if (!color.isBlack())
      f << color << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_ROTATE:
    f << "chrAtrRotate,";
    f << "nVal=" << input->readULong(2) << ",";
    f << "b=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_CHR_TWO_LINES: { // checkme
    f << "chrAtrTwoLines=" << input->readULong(1) << ",";
    f << "nStart[unicode]=" << input->readULong(2) << ",";
    f << "nEnd[unicode]=" << input->readULong(2) << ",";
    break;
  }
  case StarAttribute::ATTR_CHR_SCALEW:
  case StarAttribute::ATTR_EE_CHR_SCALEW:
    f << (nWhich==StarAttribute::ATTR_CHR_SCALEW ? "chrAtrScaleW":"eeScaleW") << "=" << input->readULong(2) << ",";
    if (input->tell()<lastPos) {
      f << "nVal=" << input->readULong(2) << ",";
      f << "test=" << input->readULong(2) << ",";
    }
    break;

  case StarAttribute::ATTR_TXT_TOXMARK: {
    f << "textAtrToXMark,";
    auto cType=int(input->readULong(1));
    f << "cType=" << cType << ",";
    f << "nLevel=" << input->readULong(2) << ",";
    std::vector<uint32_t> string;
    int nStringId=0xFFFF;
    if (nVers<1) {
      if (!zone.readString(string)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aTypeName\n"));
        f << "###aTypeName,";
        break;
      }
      if (!string.empty())
        f << "aTypeName=" << libstoff::getString(string).cstr() << ",";
    }
    else {
      nStringId=int(input->readULong(2));
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
    }
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aAltText\n"));
      f << "###aAltText,";
      break;
    }
    if (!string.empty())
      f << "aAltText=" << libstoff::getString(string).cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aPrimKey\n"));
      f << "###aPrimKey,";
      break;
    }
    if (!string.empty())
      f << "aPrimKey=" << libstoff::getString(string).cstr() << ",";
    if (!zone.readString(string)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find aSecKey\n"));
      f << "###aSecKey,";
      break;
    }
    if (!string.empty())
      f << "aSecKey=" << libstoff::getString(string).cstr() << ",";
    if (nVers>=2) {
      cType=int(input->readULong(1));
      f << "cType=" << cType << ",";
      nStringId=int(input->readULong(2));
      if (nStringId!=0xFFFF)
        f << "nStringId=" << nStringId << ",";
      f << "cFlags=" << input->readULong(1) << ",";
    }
    if (nVers>=1 && nStringId!=0xFFFF) {
      librevenge::RVNGString poolName;
      if (!zone.getPoolName(nStringId, poolName)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a nId name\n"));
        f << "###nId=" << nStringId << ",";
      }
      else
        f << poolName.cstr() << ",";
    }
    break;
  }
  case StarAttribute::ATTR_TXT_CJK_RUBY: // string("")+bool
    f << "textAtrCJKRuby=" << input->readULong(1) << ",";
    break;

  // frame parameter
  case StarAttribute::ATTR_FRM_PROTECT:
    f << "protect,";
    val=int(input->readULong(1));
    if (val&1) f << "pos[protect],";
    if (val&2) f << "size[protect],";
    if (val&4) f << "cntnt[protect],";
    break;
  case StarAttribute::ATTR_FRM_FRMMACRO: { // macitem.cxx SvxMacroTableDtor::Read
    f << "frmMacro,";
    if (nVers>=1) {
      nVers=static_cast<uint16_t>(input->readULong(2));
      f << "nVersion=" << nVers << ",";
    }
    int16_t nMacros;
    *input>>nMacros;
    f << "macros=[";
    for (int16_t i=0; i<nMacros; ++i) {
      uint16_t nCurKey;
      bool ok=true;
      f << "[";
      *input>>nCurKey;
      if (nCurKey) f << "nCurKey=" << nCurKey << ",";
      for (int j=0; j<2; ++j) {
        std::vector<uint32_t> text;
        if (!zone.readString(text) || input->tell()>lastPos) {
          STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find a macro string\n"));
          f << "###string" << j << ",";
          ok=false;
          break;
        }
        else if (!text.empty())
          f << (j==0 ? "lib" : "mac") << "=" << libstoff::getString(text).cstr() << ",";
      }
      if (!ok) break;
      if (nVers>=1) {
        uint16_t eType;
        *input>>eType;
        if (eType) f << "eType=" << eType << ",";
      }
      f << "],";
    }
    f << "],";
    break;
  }
  case StarAttribute::ATTR_FRM_URL:
    f << "url,";
    if (!StarObjectText::readSWImageMap(zone))
      break;
    if (nVers>=1) {
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the setName\n"));
        f << "###name1,";
        break;
      }
      else if (!text.empty())
        f << "name1=" << libstoff::getString(text).cstr() << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_CHAIN:
    f << "chain,";
    if (nVers>0) {
      f << "prevIdx=" << input->readULong(2) << ",";
      f << "nextIdx=" << input->readULong(2) << ",";
    }
    break;
  case StarAttribute::ATTR_FRM_TEXTGRID: // SwTextGridItem::Create
    f << "textgrid=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_FRM_FTN_AT_TXTEND:
  case StarAttribute::ATTR_FRM_END_AT_TXTEND:
    f << (nWhich==StarAttribute::ATTR_FRM_FTN_AT_TXTEND ? "ftnAtTextEnd" : "ednAtTextEnd") << "=" << input->readULong(1) << ",";
    if (nVers>0) {
      f << "offset=" << input->readULong(2) << ",";
      f << "fmtType=" << input->readULong(2) << ",";
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the prefix\n"));
        f << "###prefix,";
        break;
      }
      else if (!text.empty())
        f << "prefix=" << libstoff::getString(text).cstr() << ",";
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the suffix\n"));
        f << "###suffix,";
        break;
      }
      else if (!text.empty())
        f << "suffix=" << libstoff::getString(text).cstr() << ",";
    }
    break;
  // graphic attribute
  case StarAttribute::ATTR_GRF_MIRRORGRF:
    f << "grfMirrorGrf=" << input->readULong(1) << ",";
    if (nVers>0) f << "nToggle=" << input->readULong(1) << ",";
    break;
  case StarAttribute::ATTR_GRF_CROPGRF:
    f << "grfCropGrf,";
    f << "top=" << input->readLong(4) << ",";
    f << "left=" << input->readLong(4) << ",";
    f << "right=" << input->readLong(4) << ",";
    f << "bottom=" << input->readLong(4) << ",";
    break;

  case StarAttribute::ATTR_BOX_FORMAT:
    f << "boxFormat=" << input->readULong(1) << ",";
    f << "nTmp=" << input->readULong(4) << ",";
    break;
  case StarAttribute::ATTR_BOX_FORMULA: {
    f << "boxFormula,";
    std::vector<uint32_t> text;
    if (!zone.readString(text)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the formula\n"));
      f << "###formula,";
      break;
    }
    else if (!text.empty())
      f << "formula=" << libstoff::getString(text).cstr() << ",";
    break;
  }
  case StarAttribute::ATTR_BOX_VALUE:
    f << "boxAtrValue,";
    if (nVers==0) {
      std::vector<uint32_t> text;
      if (!zone.readString(text)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not find the dValue\n"));
        f << "###dValue,";
        break;
      }
      else if (!text.empty())
        f << "dValue=" << libstoff::getString(text).cstr() << ",";
    }
    else {
      double res;
      bool isNan;
      if (!input->readDoubleReverted8(res, isNan)) {
        STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a double\n"));
        f << "##value,";
      }
      else if (res<0||res>0)
        f << "value=" << res << ",";
    }
    break;

  case StarAttribute::ATTR_SCH_SYMBOL_SIZE:
    f << "symbolSize[sch]=" << input->readLong(4) << "x" << input->readLong(4) << ",";
    break;

  case StarAttribute::SDRATTR_AUTOSHAPE_ADJUSTMENT:
    f << "autoShapeAdjust[sdr],";
    if (nVers) {
      uint32_t n;
      *input >> n;
      if (n) f << "nCount=" << n << ",";
    }
    break;

  case StarAttribute::SDRATTR_MEASUREFORMATSTRING: { // string
    f << "measure[formatString]=";
    std::vector<uint32_t> format;
    if (!zone.readString(format)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    f << libstoff::getString(format).cstr() << ",";
    break;
  }

  case StarAttribute::SDRATTR_LAYERNAME:
  case StarAttribute::SDRATTR_OBJECTNAME: {
    f << (nWhich==StarAttribute::SDRATTR_LAYERNAME ? "sdrLayerName" : "sdrObjectName") << ",";
    std::vector<uint32_t> name;
    if (!zone.readString(name)) {
      STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: can not read a string\n"));
      f << "###string";
      break;
    }
    f << libstoff::getString(name).cstr() << ",";
    break;
  }

  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1: // SvxVector3DItem (Vector3d)
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_2:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_3:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_4:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_5:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_6:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_7:
  case StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_8:
    f << "scene3d[lightDir" << nWhich-StarAttribute::SDRATTR_3DSCENE_LIGHTDIRECTION_1+1 << "=";
    for (int i=0; i<3; ++i) {
      double coord;
      *input >> coord;
      f << coord << (i==2 ? "," : "x");
    }
    break;
  default:
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: reading not format attribute is not implemented\n"));
    f << "#unimplemented[wh=" << std::hex << nWhich << std::dec << ",";
  }
  if (input->tell()>lastPos) {
    STOFF_DEBUG_MSG(("StarAttributeManager::readAttribute: read too much data\n"));
    f << "###too much,";
    ascFile.addDelimiter(input->tell(), '|');
  }
  ascFile.addPos(pos);
  ascFile.addNote(f.str().c_str());
  return getDummyAttribute(nWhich); // fixme
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
