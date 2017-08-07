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

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

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

#define TOOLNAME "sdc2csv"

static int printUsage()
{
  printf("`" TOOLNAME "' converts StarOffice spreadsheets to CSV.\n");
  printf("\n");
  printf("Usage: " TOOLNAME " [OPTION] INPUT\n");
  printf("\n");
  printf("Options:\n");
  printf("\t-h           show this help message\n");
  printf("\t-d C         set decimal separator: default `.'\n");
  printf("\t-f C         set field separator: default `,'\n");
  printf("\t-t C         set text separator: default `\"'\n");
  printf("\t-F           set to output the formula which exists in the file\n");
  printf("\t-D FORMAT    set date format: default \"%%m/%%d/%%y\"\n");
  printf("\t-T FORMAT    set time format: default \"%%H:%%M:%%S\"\n");
  printf("\t-N           print the number of sheets\n");
  printf("\t-n NUM       choose the sheet to convert (1: means first sheet)\n");
  printf("\t-o OUTPUT    write ouput to OUTPUT\n");
  printf("\t-v           show version information\n");
  printf("\n");
  printf("Examples:\n");
  printf("\t" TOOLNAME " -d, -D\"%%d/%%m/%%y\" file : Converts a file using french locale\n");
  printf("\n");
  printf("Note:\n");
  printf("\t If -F is present, the formula are generated which english names.\n");
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
  bool printHelp=false;
  bool printNumberOfSheet=false;
  bool generateFormula=false;
  int sheetToConvert=0;
  char const *output = nullptr;
  int ch;
  char decSeparator='.', fieldSeparator=',', textSeparator='"';
  std::string dateFormat("%m/%d/%y"), timeFormat("%H:%M:%S");

  while ((ch = getopt(argc, argv, "hvo:d:f:t:D:FNn:T:")) != -1) {
    switch (ch) {
    case 'D':
      dateFormat=optarg;
      break;
    case 'F':
      generateFormula=true;
      break;
    case 'N':
      printNumberOfSheet=true;
      break;
    case 'T':
      timeFormat=optarg;
      break;
    case 'd':
      decSeparator=optarg[0];
      break;
    case 'f':
      fieldSeparator=optarg[0];
      break;
    case 't':
      textSeparator=optarg[0];
      break;
    case 'n':
      sheetToConvert=std::atoi(optarg);
      break;
    case 'o':
      output=optarg;
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
  char const *file=argv[optind];
  librevenge::RVNGFileStream input(file);

  STOFFDocument::Kind kind;
  auto confidence = STOFFDocument::STOFF_C_NONE;
  try {
    confidence = STOFFDocument::isFileFormatSupported(&input, kind);
  }
  catch (...) {
    confidence = STOFFDocument::STOFF_C_NONE;
  }
  if (confidence != STOFFDocument::STOFF_C_EXCELLENT &&
      confidence != STOFFDocument::STOFF_C_SUPPORTED_ENCRYPTION) {
    fprintf(stderr,"ERROR: Unsupported file format!\n");
    return 1;
  }
  if (kind != STOFFDocument::STOFF_K_SPREADSHEET && kind != STOFFDocument::STOFF_K_DATABASE) {
    fprintf(stderr,"ERROR: not a spreadsheet!\n");
    return 1;
  }
  auto error=STOFFDocument::STOFF_R_OK;
  librevenge::RVNGStringVector vec;

  try {
    librevenge::RVNGCSVSpreadsheetGenerator listenerImpl(vec, generateFormula);
    listenerImpl.setSeparators(fieldSeparator, textSeparator, decSeparator);
    listenerImpl.setDTFormats(dateFormat.c_str(),timeFormat.c_str());
    error= STOFFDocument::parse(&input, &listenerImpl);
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

  if (printNumberOfSheet) {
    std::cout << vec.size() << "\n";
    return 0;
  }

  unsigned page=sheetToConvert>0 ? unsigned(sheetToConvert-1) : 0;
  if (page>=vec.size()) {
    fprintf(stderr, "ERROR: can not find page %d!\n", int(page));
    return 1;
  }
  if (!output)
    std::cout << vec[page].cstr() << std::endl;
  else {
    std::ofstream out(output);
    out << vec[page].cstr() << std::endl;
  }
  return 0;
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
