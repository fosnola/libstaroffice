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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <librevenge/librevenge.h>
#include <librevenge-generators/librevenge-generators.h>
#include <librevenge-stream/librevenge-stream.h>

#include <libstaroffice/libstaroffice.hxx>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef VERSION
#define VERSION "UNKNOWN VERSION"
#endif

#define TOOLNAME "sd2svg"

static int printUsage()
{
  printf("`" TOOLNAME "' converts StarOffice graphic documents and presentations to SVG.\n");
  printf("\n");
  printf("Usage: " TOOLNAME " [OPTION] INPUT\n");
  printf("\n");
  printf("Options:\n");
  printf("\t-h                 show this help message\n");
  printf("\t-o OUTPUT          write ouput to OUTPUT\n");
  printf("\t-N                 Output the number of sheets\n");
  printf("\t-n NUM             choose the page to convert (1: means first page)\n");
  printf("\t-v                 show version information\n");
  printf("\n");
  printf("Report bugs to <https://github.com/fosnola/libstaroffice/issues>.\n");
  return -1;
}

static int printVersion()
{
  printf("%s %s\n", TOOLNAME, VERSION);
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
    return printUsage();

  char const *output = nullptr;
  bool printHelp=false;
  bool printNumberOfPages=false;
  int ch, pageToConvert=0;

  while ((ch = getopt(argc, argv, "ho:n:vN")) != -1) {
    switch (ch) {
    case 'o':
      output=optarg;
      break;
    case 'v':
      printVersion();
      return 0;
    case 'n':
      pageToConvert=std::atoi(optarg);
      break;
    case 'N':
      printNumberOfPages=true;
      break;
    default:
    case 'h':
      printHelp = true;
      break;
    }
  }

  if (argc != 1+optind || printHelp) {
    printUsage();
    return -1;
  }
  librevenge::RVNGFileStream input(argv[optind]);

  STOFFDocument::Kind kind;
  auto confidence = STOFFDocument::STOFF_C_NONE;
  try {
    confidence = STOFFDocument::isFileFormatSupported(&input, kind);
  }
  catch (...) {
    confidence = STOFFDocument::STOFF_C_NONE;
  }
  if (confidence != STOFFDocument::STOFF_C_EXCELLENT  &&
      confidence != STOFFDocument::STOFF_C_SUPPORTED_ENCRYPTION) {
    fprintf(stderr,"ERROR: Unsupported file format!\n");
    return 1;
  }
  auto error=STOFFDocument::STOFF_R_OK;
  librevenge::RVNGStringVector vec;

  try {
    if (kind == STOFFDocument::STOFF_K_DRAW) {
      librevenge::RVNGSVGDrawingGenerator listener(vec, "");
      error = STOFFDocument::parse(&input, &listener);
    }
    else if (kind == STOFFDocument::STOFF_K_PRESENTATION) {
      librevenge::RVNGSVGPresentationGenerator listener(vec);
      error = STOFFDocument::parse(&input, &listener);
    }
    else {
      fprintf(stderr,"ERROR: not a graphic/presentation document!\n");
      return 1;
    }
    if (error==STOFFDocument::STOFF_R_OK && (vec.empty() || vec[0].empty()))
      error = STOFFDocument::STOFF_R_UNKNOWN_ERROR;
  }
  catch (STOFFDocument::Result const &err) {
    error=err;
  }
  catch (...) {
    error=STOFFDocument::STOFF_R_UNKNOWN_ERROR;
  }
  if (error == STOFFDocument::STOFF_R_FILE_ACCESS_ERROR)
    fprintf(stderr, "ERROR: File Exception!\n");
  else if (error == STOFFDocument::STOFF_R_PARSE_ERROR)
    fprintf(stderr, "ERROR: Parse Exception!\n");
  else if (error == STOFFDocument::STOFF_R_OLE_ERROR)
    fprintf(stderr, "ERROR: File is an OLE document!\n");
  else if (error != STOFFDocument::STOFF_R_OK)
    fprintf(stderr, "ERROR: Unknown Error!\n");

  if (error != STOFFDocument::STOFF_R_OK)
    return 1;

  if (printNumberOfPages) {
    std::cout << vec.size() << "\n";
    return 0;
  }

  unsigned page=pageToConvert>0 ? unsigned(pageToConvert-1) : 0;
  if (page>=vec.size()) {
    fprintf(stderr, "ERROR: can not find page %d!\n", int(page));
    return 1;
  }
  if (!output) {
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    std::cout << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"";
    std::cout << " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    std::cout << vec[page].cstr() << std::endl;
  }
  else {
    std::ofstream out(output);
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    out << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"";
    out << " \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    out << vec[page].cstr() << std::endl;
  }
  return 0;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
