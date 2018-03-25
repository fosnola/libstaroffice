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

#include "STOFFStringStream.hxx"

#include "StarEncryption.hxx"

////////////////////////////////////////////////////////////
// constructor/destructor, ...
////////////////////////////////////////////////////////////
StarEncryption::StarEncryption() : m_password()
{
}

StarEncryption::StarEncryption(std::string const &password) : m_password()
{
  static const uint8_t cEncode[16] = {
    0xAB, 0x9E, 0x43, 0x05, 0x38, 0x12, 0x4d, 0x44,
    0xD5, 0x7e, 0xe3, 0x84, 0x98, 0x23, 0x3f, 0xba
  };
  std::vector<uint8_t> origCrypt(cEncode, cEncode+16);
  std::vector<uint8_t> data(16, uint8_t(' '));
  for (size_t i=0; i<password.size() && i<16; ++i)
    data[i]=uint8_t(password[i]);
  if (!decode(data, origCrypt)) {
    m_password.clear();
    return;
  }
  m_password=data;
}

StarEncryption::~StarEncryption()
{
}

bool StarEncryption::decode(std::vector<uint8_t> &data, std::vector<uint8_t> const &cryptPasswd)
{
  if (cryptPasswd.empty() || data.empty()) return true;
  if (cryptPasswd.size()!=16) {
    STOFF_DEBUG_MSG(("StarEncryption::decode: the encrypted password is bad\n"));
    return false;
  }

  std::vector<uint8_t> cryptBuf(cryptPasswd);
  uint8_t *dataPtr=&(data[0]);
  uint8_t *cryptPtr=&(cryptBuf[0]);
  size_t cryptPos=0;
  for (size_t c=0; c<data.size(); ++c) {
    *dataPtr=*dataPtr ^ *cryptPtr ^ uint8_t(cryptBuf[0]*cryptPos);
    *cryptPtr = uint8_t(*cryptPtr+(cryptPos<15 ? *(cryptPtr+1) : cryptBuf[0]));
    if (*cryptPtr==0) *cryptPtr=1;
    if (++cryptPos >= 16) {
      cryptPos=0;
      cryptPtr=&(cryptBuf[0]);
    }
    else
      ++cryptPtr;
    ++dataPtr;
  }
  return true;
}

bool StarEncryption::checkPassword(uint32_t date, uint32_t time, std::vector<uint8_t> const &cryptDateTime) const
{
  librevenge::RVNGString data;
  data.sprintf("%08x%08x", date, time);
  if ((!date && !time) || data.len()!=16) {
    STOFF_DEBUG_MSG(("StarEncryption::checkPassword: impossible to check the password\n"));
    return true;
  }
  std::vector<uint8_t> oData(16);
  for (size_t i=0; i<16; ++i)
    oData[i]=static_cast<uint8_t>(data.cstr()[i]);
  decode(oData);
  if (oData==cryptDateTime)
    return true;
  STOFF_DEBUG_MSG(("StarEncryption::checkPassword: bad password\n"));
  return false;
}

bool StarEncryption::guessPassword(uint32_t date, uint32_t time, std::vector<uint8_t> const &cryptDateTime)
{
  librevenge::RVNGString data;
  data.sprintf("%08x%08x", date, time);
  if ((!date && !time) || data.len()!=16 || cryptDateTime.size()!=16) {
    STOFF_DEBUG_MSG(("StarEncryption::guessPassword: impossible to guess the password\n"));
    return false;
  }
  std::vector<uint8_t> oData(16);
  for (size_t i=0; i<16; ++i)
    oData[i]=static_cast<uint8_t>(data.cstr()[i]);
  static const uint8_t cEncode[16] = {
    0xAB, 0x9E, 0x43, 0x05, 0x38, 0x12, 0x4d, 0x44,
    0xD5, 0x7e, 0xe3, 0x84, 0x98, 0x23, 0x3f, 0xba
  };
  std::vector<uint8_t> origCrypt(cEncode, cEncode+16);
  std::vector<uint8_t> cryptPassword, password, bestCryptPassword;
  int maxSpace=-1, numMaxSpace=0;
  for (int c0c1=0; c0c1<256; ++c0c1) {
    if (!findEncryptedPassword(oData, cryptDateTime, uint8_t(c0c1), cryptPassword) || cryptPassword.size()!=16)
      continue;
    password=cryptPassword;
    if (!decode(password, origCrypt) || password.size()!=16)
      continue;
    // check for 0 and count last space
    bool ok=true;
    int numSpace=0;
    for (size_t c=0; c<16; ++c) {
      if (password[c]==0) {
        ok=false;
        break;
      }
      if (password[c]==' ')
        ++numSpace;
      else
        numSpace=0;
    }
    if (!ok || numSpace<maxSpace) continue;
    if (numSpace>maxSpace) {
      maxSpace=numSpace;
      bestCryptPassword=cryptPassword;
      numMaxSpace=1;
    }
    else
      ++numMaxSpace;
  }
  if (numMaxSpace!=1) {
    STOFF_DEBUG_MSG(("StarEncryption::guessPassword: find %d password with %d end space\n", numMaxSpace, maxSpace));
    return false;
  }
  m_password=bestCryptPassword;
  return true;
}

bool StarEncryption::findEncryptedPassword(std::vector<uint8_t> const &src, std::vector<uint8_t> const &dest, uint8_t c0c1, std::vector<uint8_t> &crypter)
{
  if (src.size()!=16 || dest.size()!=16) {
    STOFF_DEBUG_MSG(("StarEncryption::findEncryptedPassword: expected source and size to be 16\n"));
    return false;
  }
  crypter.resize(16, 0);
  for (size_t i=0; i<16; ++i) {
    crypter[i]=src[i]^dest[i]^uint8_t(c0c1*i);
    if (crypter[i]==0)
      return false;
    if (i!=1) continue;
    auto calcC0c1=uint8_t(crypter[0]+crypter[1]);
    if (calcC0c1==0) calcC0c1=1;
    if (calcC0c1!=c0c1) return false;
  }
  return true;
}

STOFFInputStreamPtr StarEncryption::decodeStream(STOFFInputStreamPtr input, uint8_t mask)
{
  if (!mask || !input || input->size()==0) return input;

  STOFFInputStreamPtr res;
  unsigned long numRead=0;
  long dataSize=input->size();
  input->seek(0, librevenge::RVNG_SEEK_SET);
  const uint8_t *data=input->read(size_t(dataSize), numRead);
  if (!data || numRead!=static_cast<unsigned long>(dataSize)) {
    STOFF_DEBUG_MSG(("StarEncryption::decodeStream: can not read the original stream\n"));
    return res;
  }
  std::unique_ptr<uint8_t[]> finalData{new uint8_t[numRead]};
  uint8_t *finalDataPtr=finalData.get();
  for (long l=0; l<dataSize; ++l, ++data)
    *(finalDataPtr++) = uint8_t((*data>>4)|(*data<<4))^mask;
  std::shared_ptr<STOFFStringStream> stream(new STOFFStringStream(reinterpret_cast<const unsigned char *>(finalData.get()),static_cast<unsigned int>(dataSize)));
  if (!stream) return res;
  res.reset(new STOFFInputStream(stream, input->readInverted()));
  if (res) res->seek(0, librevenge::RVNG_SEEK_SET);
  return res;
}

uint8_t StarEncryption::getMaskToDecodeStream(uint8_t src, uint8_t dest)
{
  auto nibbleSrc=uint8_t((src>>4)|(src<<4));
  return nibbleSrc^dest;
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
