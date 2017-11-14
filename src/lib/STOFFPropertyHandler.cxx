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

/* This header contains code specific to a small picture
 */
#include <iostream>
#include <sstream>
#include <string.h>

#include <stack>

#include "libstaroffice_internal.hxx"

#include <librevenge/librevenge.h>
#include <librevenge-stream/librevenge-stream.h>
#include <libstaroffice/libstaroffice.hxx>

#include "STOFFPropertyHandler.hxx"

////////////////////////////////////////////////////
//
// STOFFPropertyHandlerEncoder
//
////////////////////////////////////////////////////
STOFFPropertyHandlerEncoder::STOFFPropertyHandlerEncoder()
  : m_f(std::ios::in | std::ios::out | std::ios::binary)
{
}

void STOFFPropertyHandlerEncoder::insertElement(const char *psName)
{
  m_f << 'E';
  writeString(psName);
}

void STOFFPropertyHandlerEncoder::insertElement
(const char *psName, const librevenge::RVNGPropertyList &xPropList)
{
  m_f << 'S';
  writeString(psName);
  writePropertyList(xPropList);
}

void STOFFPropertyHandlerEncoder::characters(librevenge::RVNGString const &sCharacters)
{
  if (sCharacters.len()==0) return;
  m_f << 'T';
  writeString(sCharacters);
}

void STOFFPropertyHandlerEncoder::writeString(const librevenge::RVNGString &string)
{
  unsigned long sz = string.size()+1;
  writeLong(long(sz));
  m_f.write(string.cstr(), int(sz));
}

void STOFFPropertyHandlerEncoder::writeLong(long val)
{
  auto value=static_cast<int32_t>(val);
  unsigned char const allValue[]= {static_cast<unsigned char>(value&0xFF), static_cast<unsigned char>((value>>8)&0xFF), static_cast<unsigned char>((value>>16)&0xFF), static_cast<unsigned char>((value>>24)&0xFF)};
  m_f.write(reinterpret_cast<const char *>(allValue), 4);
}

void STOFFPropertyHandlerEncoder::writeProperty(const char *key, const librevenge::RVNGProperty &prop)
{
  if (!key) {
    STOFF_DEBUG_MSG(("STOFFPropertyHandlerEncoder::writeProperty: key is NULL\n"));
    return;
  }
  writeString(key);
  writeString(prop.getStr());
}

void STOFFPropertyHandlerEncoder::writePropertyList(const librevenge::RVNGPropertyList &xPropList)
{
  librevenge::RVNGPropertyList::Iter i(xPropList);
  long numElt = 0;
  for (i.rewind(); i.next();) numElt++;
  writeLong(numElt);
  for (i.rewind(); i.next();) {
    auto const *child=xPropList.child(i.key());
    if (!child) {
      m_f << 'p';
      writeProperty(i.key(),*i());
      continue;
    }
    m_f << 'v';
    writeString(i.key());
    writePropertyListVector(*child);
  }
}

void STOFFPropertyHandlerEncoder::writePropertyListVector(const librevenge::RVNGPropertyListVector &vect)
{
  writeLong(long(vect.count()));
  for (unsigned long i=0; i < vect.count(); i++)
    writePropertyList(vect[i]);
}

bool STOFFPropertyHandlerEncoder::getData(librevenge::RVNGBinaryData &data)
{
  data.clear();
  std::string d=m_f.str();
  if (d.length() == 0) return false;
  data.append(reinterpret_cast<const unsigned char *>(d.c_str()), d.length());
  return true;
}

/* \brief Internal: the property decoder
 *
 * \note see STOFFPropertyHandlerEncoder for the format
*/
class STOFFPropertyHandlerDecoder
{
public:
  //! constructor given a STOFFPropertyHandler
  explicit STOFFPropertyHandlerDecoder(STOFFPropertyHandler *hdl=nullptr):m_handler(hdl) {}

  //! tries to read the data
  bool readData(librevenge::RVNGBinaryData const &encoded)
  {
    try {
      auto *inp = const_cast<librevenge::RVNGInputStream *>(encoded.getDataStream());
      if (!inp) return false;

      while (!inp->isEnd()) {
        unsigned const char *c;
        unsigned long numRead;

        c = inp->read(1,numRead);
        if (!c || numRead != 1) {
          STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder: can not read data type \n"));
          return false;
        }
        switch (*c) {
        case 'E':
          if (!readInsertElement(*inp)) return false;
          break;
        case 'S':
          if (!readInsertElementWithList(*inp)) return false;
          break;
        case 'T':
          if (!readCharacters(*inp)) return false;
          break;
        default:
          STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder: unknown type='%c' \n", *c));
          return false;
        }
      }
    }
    catch (...) {
      return false;
    }
    return true;
  }

protected:
  //! reads an simple element
  bool readInsertElement(librevenge::RVNGInputStream &input)
  {
    librevenge::RVNGString s;
    if (!readString(input, s)) return false;

    if (s.empty()) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readInsertElement find empty tag\n"));
      return false;
    }
    if (m_handler) m_handler->insertElement(s.cstr());
    return true;
  }

  //! reads an element with a property list
  bool readInsertElementWithList(librevenge::RVNGInputStream &input)
  {
    librevenge::RVNGString s;
    if (!readString(input, s)) return false;

    if (s.empty()) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readInsertElementWithProperty: find empty tag\n"));
      return false;
    }
    librevenge::RVNGPropertyList lists;
    if (!readPropertyList(input, lists)) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readInsertElementWithProperty: can not read propertyList for tag %s\n",
                       s.cstr()));
      return false;
    }

    if (m_handler) m_handler->insertElement(s.cstr(), lists);
    return true;
  }

  //! reads a set of characters
  bool readCharacters(librevenge::RVNGInputStream &input)
  {
    librevenge::RVNGString s;
    if (!readString(input, s)) return false;
    if (!s.size()) return true;
    if (m_handler) m_handler->characters(s);
    return true;
  }

  //
  // low level
  //

  //! low level: reads a property vector: number of properties list followed by list of properties list
  bool readPropertyListVector(librevenge::RVNGInputStream &input, librevenge::RVNGPropertyListVector &vect)
  {
    long numElt;
    if (!readLong(input, numElt)) return false;

    if (numElt < 0) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readPropertyListVector: can not read numElt=%ld\n",
                       numElt));
      return false;
    }
    for (long i = 0; i < numElt; i++) {
      librevenge::RVNGPropertyList lists;
      if (readPropertyList(input, lists)) {
        vect.append(lists);
        continue;
      }
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readPropertyListVector: can not read property list %ld\n", i));
      return false;
    }
    return true;
  }

  //! low level: reads a property list: number of properties followed by list of properties
  bool readPropertyList(librevenge::RVNGInputStream &input, librevenge::RVNGPropertyList &lists)
  {
    long numElt;
    if (!readLong(input, numElt)) return false;

    if (numElt < 0) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readPropertyList: can not read numElt=%ld\n",
                       numElt));
      return false;
    }
    for (long i = 0; i < numElt; i++) {
      unsigned const char *c;
      unsigned long numRead;
      c = input.read(1,numRead);
      if (!c || numRead != 1) {
        STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder:readPropertyList can not read data type for child %ld\n", i));
        return false;
      }
      switch (*c) {
      case 'p':
        if (readProperty(input, lists)) break;
        STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readPropertyList: can not read property %ld\n", i));
        return false;
      case 'v': {
        librevenge::RVNGString key;
        librevenge::RVNGPropertyListVector vect;
        if (!readString(input, key) || key.empty() || !readPropertyListVector(input, vect)) {
          STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readPropertyList: can not read propertyVector for child %ld\n", i));
          return false;
        }
        lists.insert(key.cstr(),vect);
        break;
      }
      default:
        STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder:readPropertyList find unknown type %c for child %ld\n", char(*c), i));
        return false;
      }
    }
    return true;
  }

  //! low level: reads a property and its value, adds it to \a list
  bool readProperty(librevenge::RVNGInputStream &input, librevenge::RVNGPropertyList &list)
  {
    librevenge::RVNGString key, val;
    if (!readString(input, key)) return false;
    if (!readString(input, val)) return false;

    list.insert(key.cstr(), val);
    return true;
  }

  //! low level: reads a string : size and string
  bool readString(librevenge::RVNGInputStream &input, librevenge::RVNGString &s)
  {
    long numC = 0;
    if (!readLong(input, numC)) return false;
    if (numC==0) {
      s = librevenge::RVNGString("");
      return true;
    }
    unsigned long numRead;
    const unsigned char *dt = input.read(static_cast<unsigned long>(numC), numRead);
    if (dt == nullptr || numRead != static_cast<unsigned long>(numC)) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readString: can not read a string\n"));
      return false;
    }
    s = librevenge::RVNGString(reinterpret_cast<const char *>(dt));
    return true;
  }

  //! low level: reads an long value
  static bool readLong(librevenge::RVNGInputStream &input, long &val)
  {
    unsigned long numRead = 0;
    auto const *dt = input.read(4, numRead);
    if (dt == nullptr || numRead != 4) {
      STOFF_DEBUG_MSG(("STOFFPropertyHandlerDecoder::readLong: can not read long\n"));
      return false;
    }
    val = long((dt[3]<<24)|(dt[2]<<16)|(dt[1]<<8)|dt[0]);
    return true;
  }
private:
  STOFFPropertyHandlerDecoder(STOFFPropertyHandlerDecoder const &orig);
  STOFFPropertyHandlerDecoder &operator=(STOFFPropertyHandlerDecoder const &);

protected:
  //! the streamfile
  STOFFPropertyHandler *m_handler;
};

////////////////////////////////////////////////////
//
// STOFFPropertyHandler
//
////////////////////////////////////////////////////
STOFFPropertyHandler::~STOFFPropertyHandler()
{
}

bool STOFFPropertyHandler::checkData(librevenge::RVNGBinaryData const &encoded)
{
  STOFFPropertyHandlerDecoder decod;
  return decod.readData(encoded);
}

bool STOFFPropertyHandler::readData(librevenge::RVNGBinaryData const &encoded)
{
  STOFFPropertyHandlerDecoder decod(this);
  return decod.readData(encoded);
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
