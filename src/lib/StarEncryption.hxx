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
 * StarEncryption to read/parse some basic encryption in StarOffice documents
 *
 */
#ifndef STAR_ENCRYPTION
#  define STAR_ENCRYPTION

#include <vector>

#include "libstaroffice_internal.hxx"
#include "STOFFDebug.hxx"

/** \brief the main class to read/.. some basic encryption in StarOffice documents
 *
 *
 *
 */
class StarEncryption
{
public:
  //! constructor
  StarEncryption();
  //! constructor knowing the original password
  explicit StarEncryption(std::string const &password);
  //! destructor
  virtual ~StarEncryption();

  //! decodes a string
  bool decode(std::vector<uint8_t> &data) const
  {
    return decode(data, m_password);
  }
  //! checks that the password is correct
  bool checkPassword(uint32_t date, uint32_t time, std::vector<uint8_t> const &cryptDateTime) const;
  /** tries to guess the password, assuming that the user's password
      has at most 15 characters.

      \note if we know the date, the time and the encrypted date time
         string, there is at most 256 encrypted passwords which can
         correspond (in pratical case, I find about 10 potential
         candidates).  So we loop over these potential encrypted
         passwords and recompute the original password to select the
         passwords which ends with more ' ' characters than the other
         one. We fail if we find no encrypted password or if we can
         not select an unique best canditate.

         As the StarOffice's algorithm adds trailing spaces if the
         password does not contain 16 characters, this may give almost
         alway good result if the user has not chosen a password with
         16 characters.

         If we success, we store the encrypted password to be used for
         further encryption.
   */
  bool guessPassword(uint32_t date, uint32_t time, std::vector<uint8_t> const &cryptDateTime);

  //! decode a zone given a mask
  static STOFFInputStreamPtr decodeStream(STOFFInputStreamPtr input, uint8_t mask);
  /** retrieves a mask needed to decode a stream knowing a src and dest bytes

      \note given a passwd, SvStream creates a mask from the passwd and used it to
            decode a stream, thus we need only this mask for decoding a crypt stream.

            Moreover as this method is used to crypt a StarCalcDocument stream,
            and we know that a valid StarCalcDocument stream must begin by XX42,
            we can easily retrieve the password mask and use it to uncrypt
            this stream...
  */
  static uint8_t getMaskToDecodeStream(uint8_t src, uint8_t dest);
protected:
  //! decodes a string
  static bool decode(std::vector<uint8_t> &data, std::vector<uint8_t> const &cryptPasswd);
  /** try to find the crypter knowing the original data(16 bytes), the
      final data(16 bytes) and the value of c0+c1

      \note if we know the value of crypter[0]+crypter[1] (modulo
      256), there is at most one potential candidate. So we compute it
      checking that crypter[0]+crypter[1] verifies the modulo relation.
   */
  static bool findEncryptedPassword(std::vector<uint8_t> const &src, std::vector<uint8_t> const &dest, uint8_t c0c1, std::vector<uint8_t> &crypter);
  //! the crypted password
  std::vector<uint8_t> m_password;
};
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
