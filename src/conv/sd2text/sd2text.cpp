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

#define TOOLNAME "sd2text"

static int printUsage()
{
  printf("`" TOOLNAME "' converts StarOffice documents to plain text.\n");
  printf("\n");
  printf("Usage: " TOOLNAME " [OPTION] INPUT\n");
  printf("\n");
  printf("Options:\n");
  printf("\t-i                show document metadata instead of the text\n");
  printf("\t-h                show this help message\n");
  printf("\t-o OUTPUT         write ouput to OUTPUT\n");
  printf("\t-p PASSWORD       set password to open the file\n");
  printf("\t-v                show version information\n");
  printf("\n");
  printf("Report bugs to <https://github.com/fosnola/libstaroffice/issues>.\n");
  return 0;
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
  char const *password=nullptr;
  bool isInfo = false;
  bool printHelp=false;
  int ch;

  while ((ch = getopt(argc, argv, "hio:p:v")) != -1) {
    switch (ch) {
    case 'i':
      isInfo=true;
      break;
    case 'o':
      output=optarg;
      break;
    case 'p':
      password=optarg;
      break;
    case 'v':
      printVersion();
      return 0;
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
    confidence=STOFFDocument::isFileFormatSupported(&input, kind);
  }
  catch (...) {
    confidence = STOFFDocument::STOFF_C_NONE;
  }

  if (confidence != STOFFDocument::STOFF_C_EXCELLENT && confidence != STOFFDocument::STOFF_C_SUPPORTED_ENCRYPTION) {
    printf("ERROR: Unsupported file format!\n");
    return 1;
  }

  librevenge::RVNGString document;
  librevenge::RVNGStringVector pages;
  bool useStringVector=false;
  auto error = STOFFDocument::STOFF_R_OK;
  try {
    if (kind == STOFFDocument::STOFF_K_DRAW || kind == STOFFDocument::STOFF_K_GRAPHIC) {
      if (isInfo) {
        printf("ERROR: can not print info concerning a graphic document!\n");
        return 1;
      }
      librevenge::RVNGTextDrawingGenerator documentGenerator(pages);
      error=STOFFDocument::parse(&input, &documentGenerator, password);
      if (error == STOFFDocument::STOFF_R_OK && !pages.size()) {
        printf("ERROR: find no graphics!\n");
        return 1;
      }
      useStringVector=true;
    }
    else if (kind == STOFFDocument::STOFF_K_SPREADSHEET || kind == STOFFDocument::STOFF_K_DATABASE) {
      librevenge::RVNGTextSpreadsheetGenerator documentGenerator(pages, isInfo);
      error=STOFFDocument::parse(&input, &documentGenerator, password);
      if (error == STOFFDocument::STOFF_R_OK && !pages.size()) {
        printf("ERROR: find no sheets!\n");
        return 1;
      }
      useStringVector=true;
    }
    else if (kind == STOFFDocument::STOFF_K_PRESENTATION) {
      if (isInfo) {
        printf("ERROR: can not print info concerning a presentation document!\n");
        return 1;
      }
      librevenge::RVNGTextPresentationGenerator documentGenerator(pages);
      error=STOFFDocument::parse(&input, &documentGenerator, password);
      if (error == STOFFDocument::STOFF_R_OK && !pages.size()) {
        printf("ERROR: find no slides!\n");
        return 1;
      }
      useStringVector=true;
    }
    else {
      librevenge::RVNGTextTextGenerator documentGenerator(document, isInfo);
      error=STOFFDocument::parse(&input, &documentGenerator, password);
    }
  }
  catch (STOFFDocument::Result const &err) {
    error=err;
  }
  catch (...) {
    error = STOFFDocument::STOFF_R_UNKNOWN_ERROR;
  }

  if (error == STOFFDocument::STOFF_R_FILE_ACCESS_ERROR)
    fprintf(stderr, "ERROR: File Exception!\n");
  else if (error == STOFFDocument::STOFF_R_PARSE_ERROR)
    fprintf(stderr, "ERROR: Parse Exception!\n");
  else if (error == STOFFDocument::STOFF_R_OLE_ERROR)
    fprintf(stderr, "ERROR: File is an OLE document!\n");
  else if (error == STOFFDocument::STOFF_R_PASSWORD_MISSMATCH_ERROR)
    fprintf(stderr, "ERROR: Bad password!\n");
  else if (error != STOFFDocument::STOFF_R_OK)
    fprintf(stderr, "ERROR: Unknown Error!\n");

  if (error != STOFFDocument::STOFF_R_OK)
    return 1;

  if (!output) {
    if (!useStringVector)
      printf("%s", document.cstr());
    else {
      for (unsigned i=0; i < pages.size(); ++i)
        printf("%s\n", pages[i].cstr());
    }
  }
  else {
    FILE *out=fopen(output, "wb");
    if (!out) {
      fprintf(stderr, "ERROR: can not open file %s!\n", output);
      return 1;
    }
    if (!useStringVector)
      fprintf(out, "%s", document.cstr());
    else {
      for (unsigned i=0; i < pages.size(); ++i)
        fprintf(out, "%s\n", pages[i].cstr());
    }
    fclose(out);
  }

  return 0;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
