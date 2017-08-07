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

#include "STOFFSubDocument.hxx"

#include "STOFFInputStream.hxx"

STOFFSubDocument::STOFFSubDocument(STOFFParser *pars, STOFFInputStreamPtr ip, STOFFEntry const &z)
  : m_parser(pars)
  , m_input(ip)
  , m_zone(z)
{
}

STOFFSubDocument::STOFFSubDocument(STOFFSubDocument const &doc)
  : m_parser(nullptr)
  , m_input()
  , m_zone()
{
  *this = doc;
}

STOFFSubDocument::~STOFFSubDocument()
{
}

STOFFSubDocument &STOFFSubDocument::operator=(STOFFSubDocument const &doc)
{
  if (&doc != this) {
    m_parser = doc.m_parser;
    m_input = doc.m_input;
    m_zone = doc.m_zone;
  }
  return *this;
}

bool STOFFSubDocument::operator!=(STOFFSubDocument const &doc) const
{
  if (doc.m_parser != m_parser) return true;
  if (doc.m_input.get() != m_input.get()) return true;
  if (doc.m_zone != m_zone) return true;
  return false;
}

bool STOFFSubDocument::operator!=(std::shared_ptr<STOFFSubDocument> const &doc) const
{
  if (!doc) return true;
  return operator!=(*doc.get());
}

// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:

