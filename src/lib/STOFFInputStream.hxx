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

#ifndef STOFF_INPUT_STREAM_H
#define STOFF_INPUT_STREAM_H

#include <string>
#include <vector>

#include <librevenge/librevenge.h>
#include <librevenge-stream/librevenge-stream.h>
#include "libstaroffice_internal.hxx"

/*! \class STOFFInputStream
 * \brief Internal class used to read the file stream
 *  Internal class used to read the file stream,
 *    this class adds some usefull functions to the basic librevenge::RVNGInputStream:
 *  - read number (int8, int16, int32) in low or end endian
 *  - selection of a section of a stream
 *  - read block of data
 *  - interface with modified librevenge::RVNGOLEStream
 */
class STOFFInputStream
{
public:
  /*!\brief creates a stream with given endian
   * \param input the given input
   * \param inverted must be set to true for pc doc and ole part and to false for mac doc
   */
  STOFFInputStream(std::shared_ptr<librevenge::RVNGInputStream> input, bool inverted);

  /*!\brief creates a stream with given endian from an existing input
   *
   * Note: this functions does not delete input
   */
  STOFFInputStream(librevenge::RVNGInputStream *input, bool inverted);
  //! destructor
  ~STOFFInputStream();

  //! returns the basic librevenge::RVNGInputStream
  std::shared_ptr<librevenge::RVNGInputStream> input()
  {
    return m_stream;
  }
  //! returns a new input stream corresponding to a librevenge::RVNGBinaryData
  static std::shared_ptr<STOFFInputStream> get(librevenge::RVNGBinaryData const &data, bool inverted);

  //! returns the endian mode (see constructor)
  bool readInverted() const
  {
    return m_inverseRead;
  }
  //! sets the endian mode
  void setReadInverted(bool newVal)
  {
    m_inverseRead = newVal;
  }
  //
  // Position: access
  //

  /*! \brief seeks to a offset position, from actual, beginning or ending position
   * \return 0 if ok
   */
  int seek(long offset, librevenge::RVNG_SEEK_TYPE seekType);
  //! returns actual offset position
  long tell();
  //! returns the stream size
  long size() const
  {
    return m_streamSize;
  }
  //! checks if a position is or not a valid file position
  bool checkPosition(long pos) const
  {
    if (pos < 0) return false;
    return pos<=m_streamSize;
  }
  //! returns true if we are at the end of the section/file
  bool isEnd();

  //
  // get data
  //

  //! returns the value of the next caracters or -1, but does not increment the position counter
  int peek();
  //! operator>> for bool
  STOFFInputStream &operator>>(bool &res)
  {
    res=readULong(1)!=0 ? true : false;
    return *this;
  }
  //! operator>> for uint8_t
  STOFFInputStream &operator>>(uint8_t &res)
  {
    res=static_cast<uint8_t>(readULong(1));
    return *this;
  }
  //! operator>> for int8_t
  STOFFInputStream &operator>>(int8_t &res)
  {
    res=static_cast<int8_t>(readLong(1));
    return *this;
  }
  //! operator>> for uint16_t
  STOFFInputStream &operator>>(uint16_t &res)
  {
    res=static_cast<uint16_t>(readULong(2));
    return *this;
  }
  //! operator>> for int16_t
  STOFFInputStream &operator>>(int16_t &res)
  {
    res=static_cast<int16_t>(readLong(2));
    return *this;
  }
  //! operator>> for uint32_t
  STOFFInputStream &operator>>(uint32_t &res)
  {
    res=static_cast<uint32_t>(readULong(4));
    return *this;
  }
  //! operator>> for int32_t
  STOFFInputStream &operator>>(int32_t &res)
  {
    res=static_cast<int32_t>(readLong(4));
    return *this;
  }
  //! operator>> for double
  STOFFInputStream &operator>>(double &res)
  {
    bool isNan;
    long pos=tell();
    if (!readDoubleReverted8(res, isNan)) {
      STOFF_DEBUG_MSG(("STOFFInputStream::operator>>: can not read a double\n"));
      seek(pos+8, librevenge::RVNG_SEEK_SET);
      res=0;
    }
    return *this;
  }
  //! returns a uint8, uint16, uint32 readed from actualPos
  unsigned long readULong(int num)
  {
    return readULong(m_stream.get(), num, 0, m_inverseRead);
  }
  //! return a int8, int16, int32 readed from actualPos
  long readLong(int num);
  //! try to read a color
  bool readColor(STOFFColor &color);
  //! read a compressed long (pstm.cxx:ReadCompressed)
  bool readCompressedLong(long &res);
  //! read a compressed unsigned long (sw_sw3imp.cxx Sw3IoImp::InULong)
  bool readCompressedULong(unsigned long &res);
  //! try to read a double of size 8: 1.5 bytes exponent, 6.5 bytes mantisse
  bool readDouble8(double &res, bool &isNotANumber);
  //! try to read a double of size 8: 6.5 bytes mantisse, 1.5 bytes exponent
  bool readDoubleReverted8(double &res, bool &isNotANumber);
  //! try to read a double of size 10: 2 bytes exponent, 8 bytes mantisse
  bool readDouble10(double &res, bool &isNotANumber);
  /**! reads numbytes data, WITHOUT using any endian or section consideration
   * \return a pointer to the read elements
   */
  const uint8_t *read(size_t numBytes, unsigned long &numBytesRead);
  /*! \brief internal function used to read num byte,
   *  - where a is the previous read data
   */
  static unsigned long readULong(librevenge::RVNGInputStream *stream, int num, unsigned long a, bool inverseRead);

  //! reads a librevenge::RVNGBinaryData with a given size in the actual section/file
  bool readDataBlock(long size, librevenge::RVNGBinaryData &data);
  //! reads a librevenge::RVNGBinaryData from actPos to the end of the section/file
  bool readEndDataBlock(librevenge::RVNGBinaryData &data);

  //
  // OLE/Zip access
  //

  //! return true if the stream is ole
  bool isStructured();
  //! returns the number of substream
  unsigned subStreamCount();
  //! returns the name of the i^th substream
  std::string subStreamName(unsigned id);

  //! return a new stream for a ole zone
  std::shared_ptr<STOFFInputStream> getSubStreamByName(std::string const &name);
  //! return a new stream for a ole zone
  std::shared_ptr<STOFFInputStream> getSubStreamById(unsigned id);

  //
  // Resource Fork access
  //

  /** returns true if the data fork block exists */
  bool hasDataFork() const
  {
    return bool(m_stream);
  }

protected:
  //! update the stream size ( must be called in the constructor )
  void updateStreamSize();
  //! internal function used to read a byte
  static uint8_t readU8(librevenge::RVNGInputStream *stream);

private:
  STOFFInputStream(STOFFInputStream const &orig);
  STOFFInputStream &operator=(STOFFInputStream const &orig);

protected:
  //! the initial input
  std::shared_ptr<librevenge::RVNGInputStream> m_stream;
  //! the stream size
  long m_streamSize;

  //! big or normal endian
  bool m_inverseRead;
};

#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
