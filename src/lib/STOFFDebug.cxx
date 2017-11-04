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
#include <set>

#include <librevenge/librevenge.h>
#include <librevenge-stream/librevenge-stream.h>

#include "libstaroffice_internal.hxx"
#include "STOFFDebug.hxx"

#if defined(DEBUG_WITH_FILES)
#include <iomanip>
#include <iostream>
namespace libstoff
{
bool DebugFile::open(std::string const &filename)
{
  m_on=true;
  m_fileName=filename;
  return true;
}

void DebugFile::addPos(long pos)
{
  if (!m_on) return;
  m_actOffset = pos;
}

void DebugFile::addNote(char const *note)
{
  if (!m_on || note == 0L) return;

  size_t numNotes = m_notes.size();

  if (!numNotes || m_notes[numNotes-1].m_pos != m_actOffset) {
    std::string empty("");
    m_notes.push_back(NotePos(m_actOffset, empty));
    numNotes++;
  }
  m_notes[numNotes-1].m_text += std::string(note);
}

void DebugFile::addDelimiter(long pos, char c)
{
  if (!m_on) return;
  std::string s;
  s+=c;
  m_notes.push_back(NotePos(pos,s,false));
}

void DebugFile::sort()
{
  if (!m_on) return;
  size_t numNotes = m_notes.size();

  if (m_actOffset >= 0 && (numNotes == 0 || m_notes[numNotes-1].m_pos != m_actOffset)) {
    std::string empty("");
    m_notes.push_back(NotePos(m_actOffset, empty));
    numNotes++;
  }

  std::set<NotePos, NotePos::NotePosLt> set;
  for (size_t i = 0; i < numNotes; i++)
    set.insert(m_notes[i]);

  size_t i = 0;
  for (auto const &notePos : set)
    m_notes[i++] = notePos;
  if (i != numNotes) m_notes.resize(i);

  STOFFVec2i::MapX sMap;
  size_t numSkip = m_skipZones.size();
  for (i = 0; i < numSkip; i++) sMap[m_skipZones[i]] = 0;

  i = 0;
  for (auto &skipIt : sMap)
    m_skipZones[i++] = skipIt.first;
  if (i < numSkip) m_skipZones.resize(i);
}

void DebugFile::write()
{
  if (!m_on || m_input.get() == 0) return;

  auto name=Debug::flattenFileName(m_fileName);
  if (name.empty()) return;
  name += ".ascii";
  m_file.open(name.c_str());
  if (!m_file.is_open()) return;
  sort();

  long readPos = m_input->tell();

  auto noteIter = m_notes.begin();

  //! write the notes which does not have any position
  while (noteIter != m_notes.end() && noteIter->m_pos < 0) {
    if (!noteIter->m_text.empty())
      std::cerr << "DebugFile::write: skipped: " << noteIter->m_text << std::endl;
    ++noteIter;
  }

  long actualPos = 0;
  int numSkip = int(m_skipZones.size()), actSkip = (numSkip == 0) ? -1 : 0;
  long actualSkipEndPos = (numSkip == 0) ? -1 : m_skipZones[0].x();

  m_input->seek(0,librevenge::RVNG_SEEK_SET);
  m_file << std::hex << std::right << std::setfill('0') << std::setw(6) << 0 << " ";

  do {
    bool printAdr = false;
    bool stop = false;
    while (actualSkipEndPos != -1 && actualPos >= actualSkipEndPos) {
      printAdr = true;
      actualPos = m_skipZones[size_t(actSkip)].y()+1;
      m_file << "\nSkip : " << std::hex << std::setw(6) << actualSkipEndPos << "-"
             << actualPos-1 << "\n\n";
      m_input->seek(actualPos, librevenge::RVNG_SEEK_SET);
      stop = m_input->isEnd();
      actSkip++;
      actualSkipEndPos = (actSkip < numSkip) ? m_skipZones[size_t(actSkip)].x() : -1;
    }
    if (stop) break;
    while (noteIter != m_notes.end() && noteIter->m_pos < actualPos) {
      if (!noteIter->m_text.empty())
        m_file << "Skipped: " << noteIter->m_text << std::endl;
      ++noteIter;
    }
    bool printNote = noteIter != m_notes.end() && noteIter->m_pos == actualPos;
    if (printAdr || (printNote && noteIter->m_breaking))
      m_file << "\n" << std::setw(6) << actualPos << " ";
    while (noteIter != m_notes.end() && noteIter->m_pos == actualPos) {
      if (noteIter->m_text.empty()) {
        ++noteIter;
        continue;
      }
      if (noteIter->m_breaking)
        m_file << "[" << noteIter->m_text << "]";
      else
        m_file << noteIter->m_text;
      ++noteIter;
    }

    long ch = long(m_input->readULong(1));
    m_file << std::setw(2) << ch;
    actualPos++;

  }
  while (!m_input->isEnd());

  m_file << "\n\n";

  m_input->seek(readPos,librevenge::RVNG_SEEK_SET);

  m_actOffset=-1;
  m_notes.resize(0);
}

////////////////////////////////////////////////////////////
//
// save librevenge::RVNGBinaryData in a file
//
////////////////////////////////////////////////////////////
namespace Debug
{
bool dumpFile(librevenge::RVNGBinaryData &data, char const *fileName)
{
  if (!fileName) return false;
  auto fName = Debug::flattenFileName(fileName);
  if (!data.size() || !data.getDataBuffer()) {
    STOFF_DEBUG_MSG(("Debug::dumpFile: can not find data for %s\n", fileName));
    return false;
  }
  FILE *file = fopen(fName.c_str(), "wb");
  if (!file) return false;
  fwrite(data.getDataBuffer(), data.size(), 1, file);
  fclose(file);
  return true;
}

std::string flattenFileName(std::string const &name)
{
  std::string res;
  for (size_t i = 0; i < name.length(); i++) {
    unsigned char c = static_cast<unsigned char>(name[i]);
    switch (c) {
    case '\0':
    case '/':
    case '\\':
    case ':': // potential file system separator
    case ' ':
    case '\t':
    case '\n': // potential text separator
      res += '_';
      break;
    default:
      if (c <= 28) res += '#'; // other trouble potential char
      else if (c > 0x80) res += '#'; // other trouble potential char
      else res += char(c);
    }
  }
  return res;
}
}

}
#endif
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
