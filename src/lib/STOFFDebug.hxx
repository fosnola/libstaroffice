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

#ifndef STOFF_DEBUG
#  define STOFF_DEBUG

#include <string>

#include "STOFFInputStream.hxx"

#  if defined(DEBUG_WITH_FILES)
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
//! some  basic tools
namespace libstoff
{
//! debugging tools
namespace Debug
{
//! a debug function to store in a datafile in the current directory
//! WARNING: this function erase the file fileName if it exists
//! (if debug_with_files is not defined, does nothing)
bool dumpFile(librevenge::RVNGBinaryData &data, char const *fileName);
//! returns a file name from an ole/... name
std::string flattenFileName(std::string const &name);
}

//! a basic stream (if debug_with_files is not defined, does nothing)
typedef std::stringstream DebugStream;

//! an interface used to insert comment in a binary file,
//! written in ascii form (if debug_with_files is not defined, does nothing)
class DebugFile
{
public:
  //! constructor given the input file
  explicit DebugFile(STOFFInputStreamPtr ip=STOFFInputStreamPtr())
    : m_fileName("")
    , m_file()
    , m_on(false)
    , m_input(ip)
    , m_actOffset(-1)
    , m_notes()
    , m_skipZones() { }

  //! resets the input
  void setStream(STOFFInputStreamPtr &ip)
  {
    m_input = ip;
  }
  //! destructor
  ~DebugFile()
  {
    reset();
  }
  //! opens/creates a file to store a result
  bool open(std::string const &filename);
  //! writes the current file and reset to zero
  void reset()
  {
    write();
    m_fileName="";
    m_file.close();
    m_on = false;
    m_notes.resize(0);
    m_skipZones.resize(0);
    m_actOffset = -1;
  }
  //! flushes the file
  void write();
  //! adds a new position in the file
  void addPos(long pos);
  //! adds a note in the file, in actual position
  void addNote(char const *note);
  //! adds a not breaking delimiter in position \a pos
  void addDelimiter(long pos, char c);

  //! skips the file zone defined by beginPos-endPos
  void skipZone(long beginPos, long endPos)
  {
    if (m_on) m_skipZones.push_back(STOFFVec2<long>(beginPos, endPos));
  }

protected:
  //! sorts the position/note date
  void sort();

  //! the file name
  mutable std::string m_fileName;
  //! a stream which is open to write the file
  mutable std::ofstream m_file;
  //! a flag to know if the result stream is open or note
  mutable bool m_on;

  //! the input
  STOFFInputStreamPtr m_input;

  //! \brief a note and its position (used to sort all notes)
  struct NotePos {
    //! empty constructor used by std::vector
    NotePos()
      : m_pos(-1)
      , m_text("")
      , m_breaking(false) { }

    //! constructor: given position and note
    NotePos(long p, std::string const &n, bool br=true)
      : m_pos(p)
      , m_text(n)
      , m_breaking(br) {}
    //! note offset
    long m_pos;
    //! note text
    std::string m_text;
    //! flag to indicate a non breaking note
    bool m_breaking;

    //! comparison operator based on the position
    bool operator<(NotePos const &p) const
    {
      long diff = m_pos-p.m_pos;
      if (diff) return (diff < 0) ? true : false;
      if (m_breaking != p.m_breaking) return m_breaking;
      return m_text < p.m_text;
    }
    /*! \struct NotePosLt
     * \brief internal struct used to sort the notes, sorted by position
     */
    struct NotePosLt {
      //! comparison operator
      bool operator()(NotePos const &s1, NotePos const &s2) const
      {
        return s1 < s2;
      }
    };
  };

  //! the actual offset (used to store note)
  long m_actOffset;
  //! list of notes
  std::vector<NotePos> m_notes;
  //! list of skipZone
  std::vector<STOFFVec2<long> > m_skipZones;
};
}
#  else
namespace libstoff
{
namespace Debug
{
inline bool dumpFile(librevenge::RVNGBinaryData &, char const *)
{
  return true;
}
//! returns a file name from an ole/... name
inline std::string flattenFileName(std::string const &name)
{
  return name;
}
}

class DebugStream
{
public:
  template <class T>
  DebugStream &operator<<(T const &)
  {
    return *this;
  }

  static std::string str()
  {
    return std::string("");
  }
  static void str(std::string const &) { }
};

class DebugFile
{
public:
  explicit DebugFile(STOFFInputStreamPtr) {}
  DebugFile() {}
  static void setStream(STOFFInputStreamPtr) {  }
  ~DebugFile() { }

  static bool open(std::string const &)
  {
    return true;
  }

  static void addPos(long) {}
  static void addNote(char const *) {}
  static void addDelimiter(long, char) {}

  static void write() {}
  static void reset() { }

  static void skipZone(long, long) {}
};
}
#  endif

#endif

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
