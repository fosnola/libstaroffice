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

#include "STOFFParagraph.hxx"

#include "StarAttribute.hxx"
#include "StarItemPool.hxx"
#include "StarLanguage.hxx"
#include "StarObject.hxx"
#include "StarZone.hxx"

#include "StarParagraphAttribute.hxx"

namespace StarParagraphAttribute
{
//! a character bool attribute
class StarPAttributeBool : public StarAttributeBool
{
public:
  //! constructor
  StarPAttributeBool(Type type, std::string const &debugName, bool value) :
    StarAttributeBool(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeBool(*this));
  }
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/) const;

protected:
  //! copy constructor
  StarPAttributeBool(StarPAttributeBool const &orig) : StarAttributeBool(orig)
  {
  }
};

//! a character color attribute
class StarPAttributeColor : public StarAttributeColor
{
public:
  //! constructor
  StarPAttributeColor(Type type, std::string const &debugName, STOFFColor const &value) : StarAttributeColor(type, debugName, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeColor(*this));
  }
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarPAttributeColor(StarPAttributeColor const &orig) : StarAttributeColor(orig)
  {
  }
};

//! a character integer attribute
class StarPAttributeInt : public StarAttributeInt
{
public:
  //! constructor
  StarPAttributeInt(Type type, std::string const &debugName, int intSize, int value) :
    StarAttributeInt(type, debugName, intSize, value)
  {
  }
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeInt(*this));
  }
protected:
  //! copy constructor
  StarPAttributeInt(StarPAttributeInt const &orig) : StarAttributeInt(orig)
  {
  }
};

//! a character unsigned integer attribute
class StarPAttributeUInt : public StarAttributeUInt
{
public:
  //! constructor
  StarPAttributeUInt(Type type, std::string const &debugName, int intSize, unsigned int value) :
    StarAttributeUInt(type, debugName, intSize, value)
  {
  }
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeUInt(*this));
  }
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/) const;
protected:
  //! copy constructor
  StarPAttributeUInt(StarPAttributeUInt const &orig) : StarAttributeUInt(orig)
  {
  }
};

//! a void attribute
class StarPAttributeVoid : public StarAttributeVoid
{
public:
  //! constructor
  StarPAttributeVoid(Type type, std::string const &debugName) : StarAttributeVoid(type, debugName)
  {
  }
  //! add to a para
  // virtual void addTo(STOFFParagraph &para, StarItemPool const */*pool*/) const;
  //! create a new attribute
  virtual shared_ptr<StarAttribute> create() const
  {
    return shared_ptr<StarAttribute>(new StarPAttributeVoid(*this));
  }
protected:
  //! copy constructor
  StarPAttributeVoid(StarPAttributeVoid const &orig) : StarAttributeVoid(orig)
  {
  }
};

//! add a bool attribute
void addAttributeBool(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, bool defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeBool(type,debugName, defValue));
}
//! add a color attribute
void addAttributeColor(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, STOFFColor const &defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeColor(type,debugName, defValue));
}
//! add a int attribute
void addAttributeInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeInt(type,debugName, numBytes, defValue));
}
//! add a unsigned int attribute
void addAttributeUInt(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName, int numBytes, unsigned int defValue)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeUInt(type,debugName, numBytes, defValue));
}
//! add a void attribute
void addAttributeVoid(std::map<int, shared_ptr<StarAttribute> > &map, StarAttribute::Type type, std::string const &debugName)
{
  map[type]=shared_ptr<StarAttribute>(new StarPAttributeVoid(type,debugName));
}

}

namespace StarParagraphAttribute
{
// ------------------------------------------------------------
}

namespace StarParagraphAttribute
{
void addInitTo(std::map<int, shared_ptr<StarAttribute> > &/*map*/)
{
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
