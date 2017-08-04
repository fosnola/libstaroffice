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

#include <map>
#include <string>

#include <librevenge/librevenge.h>

#include "STOFFStringStream.hxx"

#include "StarLanguage.hxx"

namespace StarLanguage
{
//! the different language
enum Language {
  LANGUAGE_DONTKNOW=0x03FF,
  LANGUAGE_NONE=0x00FF,
  LANGUAGE_SYSTEM=0x0000,
  LANGUAGE_AFRIKAANS=0x0436,
  LANGUAGE_ALBANIAN=0x041C,
  LANGUAGE_ARABIC=0x0001,
  LANGUAGE_ARABIC_SAUDI_ARABIA=0x0401,
  LANGUAGE_ARABIC_IRAQ=0x0801,
  LANGUAGE_ARABIC_EGYPT=0x0C01,
  LANGUAGE_ARABIC_LIBYA=0x1001,
  LANGUAGE_ARABIC_ALGERIA=0x1401,
  LANGUAGE_ARABIC_MOROCCO=0x1801,
  LANGUAGE_ARABIC_TUNISIA=0x1C01,
  LANGUAGE_ARABIC_OMAN=0x2001,
  LANGUAGE_ARABIC_YEMEN=0x2401,
  LANGUAGE_ARABIC_SYRIA=0x2801,
  LANGUAGE_ARABIC_JORDAN=0x2C01,
  LANGUAGE_ARABIC_LEBANON=0x3001,
  LANGUAGE_ARABIC_KUWAIT=0x3401,
  LANGUAGE_ARABIC_UAE=0x3801,
  LANGUAGE_ARABIC_BAHRAIN=0x3C01,
  LANGUAGE_ARABIC_QATAR=0x4001,
  LANGUAGE_ARMENIAN=0x042B,
  LANGUAGE_ASSAMESE=0x044D,
  LANGUAGE_AZERI=0x002C,
  LANGUAGE_AZERI_LATIN=0x042C,
  LANGUAGE_AZERI_CYRILLIC=0x082C,
  LANGUAGE_BASQUE=0x042D,
  LANGUAGE_BELARUSIAN=0x0423,
  LANGUAGE_BENGALI=0x0445,
  LANGUAGE_BULGARIAN=0x0402,
  LANGUAGE_BURMESE=0x0455,
  LANGUAGE_CATALAN=0x0403,
  LANGUAGE_CHINESE=0x0004,
  LANGUAGE_CHINESE_TRADITIONAL=0x0404,
  LANGUAGE_CHINESE_SIMPLIFIED=0x0804,
  LANGUAGE_CHINESE_HONGKONG=0x0C04,
  LANGUAGE_CHINESE_SINGAPORE=0x1004,
  LANGUAGE_CHINESE_MACAU=0x1404,
  LANGUAGE_CZECH=0x0405,
  LANGUAGE_DANISH=0x0406,
  LANGUAGE_DUTCH=0x0413,
  LANGUAGE_DUTCH_BELGIAN=0x0813,
  LANGUAGE_ENGLISH=0x0009,
  LANGUAGE_ENGLISH_US=0x0409,
  LANGUAGE_ENGLISH_UK=0x0809,
  LANGUAGE_ENGLISH_AUS=0x0C09,
  LANGUAGE_ENGLISH_CAN=0x1009,
  LANGUAGE_ENGLISH_NZ=0x1409,
  LANGUAGE_ENGLISH_EIRE=0x1809,
  LANGUAGE_ENGLISH_SAFRICA=0x1C09,
  LANGUAGE_ENGLISH_JAMAICA=0x2009,
  LANGUAGE_ENGLISH_CARRIBEAN=0x2409,
  LANGUAGE_ENGLISH_BELIZE=0x2809,
  LANGUAGE_ENGLISH_TRINIDAD=0x2C09,
  LANGUAGE_ENGLISH_ZIMBABWE=0x3009,
  LANGUAGE_ENGLISH_PHILIPPINES=0x3409,
  LANGUAGE_ESTONIAN=0x0425,
  LANGUAGE_FAEROESE=0x0438,
  LANGUAGE_FARSI=0x0429,
  LANGUAGE_FINNISH=0x040B,
  LANGUAGE_FRENCH=0x040C,
  LANGUAGE_FRENCH_BELGIAN=0x080C,
  LANGUAGE_FRENCH_CANADIAN=0x0C0C,
  LANGUAGE_FRENCH_SWISS=0x100C,
  LANGUAGE_FRENCH_LUXEMBOURG=0x140C,
  LANGUAGE_FRENCH_MONACO=0x180C,
  LANGUAGE_FRENCH_WEST_INDIES=0x1C0C,
  LANGUAGE_FRENCH_REUNION=0x200C,
  LANGUAGE_FRENCH_ZAIRE=0x240C,
  LANGUAGE_FRENCH_SENEGAL=0x280C,
  LANGUAGE_FRENCH_CAMEROON=0x2C0C,
  LANGUAGE_FRENCH_COTE_D_IVOIRE=0x300C,
  LANGUAGE_FRENCH_MALI=0x340C,
  LANGUAGE_FRISIAN_NETHERLANDS=0x0462,
  LANGUAGE_GAELIC_SCOTLAND=0x043C,
  LANGUAGE_GAELIC_IRELAND=0x083C,
  LANGUAGE_GALICIAN=0x0456,
  LANGUAGE_GEORGIAN=0x0437,
  LANGUAGE_GERMAN=0x0407,
  LANGUAGE_GERMAN_SWISS=0x0807,
  LANGUAGE_GERMAN_AUSTRIAN=0x0C07,
  LANGUAGE_GERMAN_LUXEMBOURG=0x1007,
  LANGUAGE_GERMAN_LIECHTENSTEIN=0x1407,
  LANGUAGE_GREEK=0x0408,
  LANGUAGE_GUJARATI=0x0447,
  LANGUAGE_HEBREW=0x040D,
  LANGUAGE_HINDI=0x0439,
  LANGUAGE_HUNGARIAN=0x040E,
  LANGUAGE_ICELANDIC=0x040F,
  LANGUAGE_INDONESIAN=0x0421,
  LANGUAGE_ITALIAN=0x0410,
  LANGUAGE_ITALIAN_SWISS=0x0810,
  LANGUAGE_JAPANESE=0x0411,
  LANGUAGE_KANNADA=0x044B,
  LANGUAGE_KASHMIRI=0x0460,
  LANGUAGE_KASHMIRI_INDIA=0x0860,
  LANGUAGE_KAZAK=0x043F,
  LANGUAGE_KHMER=0x0453,
  LANGUAGE_KIRGHIZ=0x0440,
  LANGUAGE_KONKANI=0x0457,
  LANGUAGE_KOREAN=0x0412,
  LANGUAGE_KOREAN_JOHAB=0x0812,
  LANGUAGE_LAO=0x0454,
  LANGUAGE_LATVIAN=0x0426,
  LANGUAGE_LITHUANIAN=0x0427,
  LANGUAGE_LITHUANIAN_CLASSIC=0x0827,
  LANGUAGE_MACEDONIAN=0x042F,
  LANGUAGE_MALAY=0x003E,
  LANGUAGE_MALAY_MALAYSIA=0x043E,
  LANGUAGE_MALAY_BRUNEI_DARUSSALAM=0x083E,
  LANGUAGE_MALAYALAM=0x044C,
  LANGUAGE_MALTESE=0x043A,
  LANGUAGE_MANIPURI=0x0458,
  LANGUAGE_MARATHI=0x044E,
  LANGUAGE_MONGOLIAN=0x0450,
  LANGUAGE_NEPALI=0x0461,
  LANGUAGE_NEPALI_INDIA=0x0861,
  LANGUAGE_NORWEGIAN=0x0014,
  LANGUAGE_NORWEGIAN_BOKMAL=0x0414,
  LANGUAGE_NORWEGIAN_NYNORSK=0x0814,
  LANGUAGE_SEPEDI=0x046C,
  LANGUAGE_NORTHERNSOTHO=LANGUAGE_SEPEDI,
  LANGUAGE_ORIYA=0x0448,
  LANGUAGE_POLISH=0x0415,
  LANGUAGE_PORTUGUESE=0x0816,
  LANGUAGE_PORTUGUESE_BRAZILIAN=0x0416,
  LANGUAGE_PUNJABI=0x0446,
  LANGUAGE_RHAETO_ROMAN=0x0417,
  LANGUAGE_ROMANIAN=0x0418,
  LANGUAGE_ROMANIAN_MOLDOVA=0x0818,
  LANGUAGE_RUSSIAN=0x0419,
  LANGUAGE_RUSSIAN_MOLDOVA=0x0819,
  LANGUAGE_SAMI_LAPPISH=0x043B,
  LANGUAGE_SANSKRIT=0x044F,
  LANGUAGE_SERBIAN=0x001A,
  LANGUAGE_CROATIAN=0x041A,
  LANGUAGE_SERBIAN_LATIN=0x081A,
  LANGUAGE_SERBIAN_CYRILLIC=0x0C1A,
  LANGUAGE_SESOTHO=0x0430,
  LANGUAGE_SINDHI=0x0459,
  LANGUAGE_SLOVAK=0x041B,
  LANGUAGE_SLOVENIAN=0x0424,
  LANGUAGE_SORBIAN=0x042E,
  LANGUAGE_SPANISH=0x040A,
  LANGUAGE_SPANISH_MEXICAN=0x080A,
  LANGUAGE_SPANISH_MODERN=0x0C0A,
  LANGUAGE_SPANISH_GUATEMALA=0x100A,
  LANGUAGE_SPANISH_COSTARICA=0x140A,
  LANGUAGE_SPANISH_PANAMA=0x180A,
  LANGUAGE_SPANISH_DOMINICAN_REPUBLIC=0x1C0A,
  LANGUAGE_SPANISH_VENEZUELA=0x200A,
  LANGUAGE_SPANISH_COLOMBIA=0x240A,
  LANGUAGE_SPANISH_PERU=0x280A,
  LANGUAGE_SPANISH_ARGENTINA=0x2C0A,
  LANGUAGE_SPANISH_ECUADOR=0x300A,
  LANGUAGE_SPANISH_CHILE=0x340A,
  LANGUAGE_SPANISH_URUGUAY=0x380A,
  LANGUAGE_SPANISH_PARAGUAY=0x3C0A,
  LANGUAGE_SPANISH_BOLIVIA=0x400A,
  LANGUAGE_SPANISH_EL_SALVADOR=0x440A,
  LANGUAGE_SPANISH_HONDURAS=0x480A,
  LANGUAGE_SPANISH_NICARAGUA=0x4C0A,
  LANGUAGE_SPANISH_PUERTO_RICO=0x500A,
  LANGUAGE_SWAHILI=0x0441,
  LANGUAGE_SWEDISH=0x041D,
  LANGUAGE_SWEDISH_FINLAND=0x081D,
  LANGUAGE_TAJIK=0x0428,
  LANGUAGE_TAMIL=0x0449,
  LANGUAGE_TATAR=0x0444,
  LANGUAGE_TELUGU=0x044A,
  LANGUAGE_THAI=0x041E,
  LANGUAGE_TIBETAN=0x0451,
  LANGUAGE_TSONGA=0x0431,
  LANGUAGE_TSWANA=0x0432,
  LANGUAGE_TURKISH=0x041F,
  LANGUAGE_TURKMEN=0x0442,
  LANGUAGE_UKRAINIAN=0x0422,
  LANGUAGE_URDU=0x0020,
  LANGUAGE_URDU_PAKISTAN=0x0420,
  LANGUAGE_URDU_INDIA=0x0820,
  LANGUAGE_UZBEK=0x0043,
  LANGUAGE_UZBEK_LATIN=0x0443,
  LANGUAGE_UZBEK_CYRILLIC=0x0843,
  LANGUAGE_VENDA=0x0433,
  LANGUAGE_VIETNAMESE=0x042A,
  LANGUAGE_WELSH=0x0452,
  LANGUAGE_XHOSA=0x0434,
  LANGUAGE_ZULU=0x0435,
  LANGUAGE_USER1=0x0201,
  LANGUAGE_USER2=0x0202,
  LANGUAGE_USER3=0x0203,
  LANGUAGE_USER4=0x0204,
  LANGUAGE_USER5=0x0205,
  LANGUAGE_USER6=0x0206,
  LANGUAGE_USER7=0x0207,
  LANGUAGE_USER8=0x0208,
  LANGUAGE_USER9=0x0209,
  LANGUAGE_SYSTEM_DEFAULT=0x0800,
  LANGUAGE_PROCESS_OR_USER_DEFAULT=0x0400,
  LANGUAGE_USER_LATIN=0x0610,
  LANGUAGE_USER_ESPERANTO=0x0611,
  LANGUAGE_USER_MAORI=0x0620,
  LANGUAGE_USER_KINYARWANDA=0x0621
};

struct IdIsoLanguageEntry {
  int m_languageId;
  char m_language[3];
  std::string m_country;
};

class IdIsoLanguageMap
{
public:
  IdIsoLanguageMap() : m_idLanguageMap()
  {
    IdIsoLanguageEntry const idIsoList[] = {
      { LANGUAGE_ENGLISH,                     "en", ""   },
      { LANGUAGE_ENGLISH_US,                  "en", "US" },
      { LANGUAGE_ENGLISH_UK,                  "en", "GB" },
      { LANGUAGE_ENGLISH_AUS,                 "en", "AU" },
      { LANGUAGE_ENGLISH_CAN,                 "en", "CA" },
      { LANGUAGE_FRENCH,                      "fr", "FR" },
      { LANGUAGE_GERMAN,                      "de", "DE" },
      { LANGUAGE_ITALIAN,                     "it", "IT" },
      { LANGUAGE_DUTCH,                       "nl", "NL" },
      { LANGUAGE_SPANISH,                     "es", "ES" },
      { LANGUAGE_SPANISH_MODERN,              "es", "ES" },
      { LANGUAGE_PORTUGUESE,                  "pt", "PT" },
      { LANGUAGE_PORTUGUESE_BRAZILIAN,        "pt", "BR" },
      { LANGUAGE_DANISH,                      "da", "DK" },
      { LANGUAGE_GREEK,                       "el", "GR" },
      { LANGUAGE_CHINESE,                     "zh", ""   },
      { LANGUAGE_CHINESE_TRADITIONAL,         "zh", "TW" },
      { LANGUAGE_CHINESE_SIMPLIFIED,          "zh", "CN" },
      { LANGUAGE_CHINESE_HONGKONG,            "zh", "HK" },
      { LANGUAGE_CHINESE_SINGAPORE,           "zh", "SG" },
      { LANGUAGE_CHINESE_MACAU,               "zh", "MO" },
      { LANGUAGE_JAPANESE,                    "ja", "JP" },
      { LANGUAGE_KOREAN,                      "ko", "KR" },
      { LANGUAGE_KOREAN_JOHAB,                "ko", "KR" },
      { LANGUAGE_KOREAN,                      "ko", "KP" },   // North Korea
      { LANGUAGE_SWEDISH,                     "sv", "SE" },
      { LANGUAGE_SWEDISH_FINLAND,             "sv", "FI" },
      { LANGUAGE_FINNISH,                     "fi", "FI" },
      { LANGUAGE_RUSSIAN,                     "ru", "RU" },
      { LANGUAGE_ENGLISH_NZ,                  "en", "NZ" },
      { LANGUAGE_ENGLISH_EIRE,                "en", "IE" },
      { LANGUAGE_ENGLISH_SAFRICA,             "en", "ZA" },
      { LANGUAGE_DUTCH_BELGIAN,               "nl", "BE" },
      { LANGUAGE_FRENCH_BELGIAN,              "fr", "BE" },
      { LANGUAGE_FRENCH_CANADIAN,             "fr", "CA" },
      { LANGUAGE_FRENCH_SWISS,                "fr", "CH" },
      { LANGUAGE_GERMAN_SWISS,                "de", "CH" },
      { LANGUAGE_GERMAN_AUSTRIAN,             "de", "AT" },
      { LANGUAGE_ITALIAN_SWISS,               "it", "CH" },
      { LANGUAGE_ARABIC,                      "ar", ""   },
      { LANGUAGE_ARABIC_SAUDI_ARABIA,         "ar", "SA" },
      { LANGUAGE_ARABIC_EGYPT,                "ar", "EG" },
      { LANGUAGE_ARABIC_UAE,                  "ar", "AE" },
      { LANGUAGE_AFRIKAANS,                   "af", "ZA" },
      { LANGUAGE_ALBANIAN,                    "sq", "AL" },
      { LANGUAGE_ARABIC_IRAQ,                 "ar", "IQ" },
      { LANGUAGE_ARABIC_LIBYA,                "ar", "LY" },
      { LANGUAGE_ARABIC_ALGERIA,              "ar", "DZ" },
      { LANGUAGE_ARABIC_MOROCCO,              "ar", "MA" },
      { LANGUAGE_ARABIC_TUNISIA,              "ar", "TN" },
      { LANGUAGE_ARABIC_OMAN,                 "ar", "OM" },
      { LANGUAGE_ARABIC_YEMEN,                "ar", "YE" },
      { LANGUAGE_ARABIC_SYRIA,                "ar", "SY" },
      { LANGUAGE_ARABIC_JORDAN,               "ar", "JO" },
      { LANGUAGE_ARABIC_LEBANON,              "ar", "LB" },
      { LANGUAGE_ARABIC_KUWAIT,               "ar", "KW" },
      { LANGUAGE_ARABIC_BAHRAIN,              "ar", "BH" },
      { LANGUAGE_ARABIC_QATAR,                "ar", "QA" },
      { LANGUAGE_BASQUE,                      "eu", ""   },
      { LANGUAGE_BULGARIAN,                   "bg", "BG" },
      { LANGUAGE_CROATIAN,                    "hr", "HR" },
      { LANGUAGE_CZECH,                       "cs", "CZ" },
      { LANGUAGE_CZECH,                       "cz", ""   },
      { LANGUAGE_ENGLISH_JAMAICA,             "en", "JM" },
      { LANGUAGE_ENGLISH_CARRIBEAN,           "en", "BS" },   // not 100%, because AG is Bahamas
      { LANGUAGE_ENGLISH_BELIZE,              "en", "BZ" },
      { LANGUAGE_ENGLISH_TRINIDAD,            "en", "TT" },
      { LANGUAGE_ENGLISH_ZIMBABWE,            "en", "ZW" },
      { LANGUAGE_ENGLISH_PHILIPPINES,         "en", "PH" },
      { LANGUAGE_ESTONIAN,                    "et", "EE" },
      { LANGUAGE_FAEROESE,                    "fo", "FO" },
      { LANGUAGE_FARSI,                       "fa", ""   },
      { LANGUAGE_FRENCH_LUXEMBOURG,           "fr", "LU" },
      { LANGUAGE_FRENCH_MONACO,               "fr", "MC" },
      { LANGUAGE_GERMAN_LUXEMBOURG,           "de", "LU" },
      { LANGUAGE_GERMAN_LIECHTENSTEIN,        "de", "LI" },
      { LANGUAGE_HEBREW,                      "he", "IL" },   // new: old was "iw"
      { LANGUAGE_HEBREW,                      "iw", "IL" },   // old: new is "he"
      { LANGUAGE_HUNGARIAN,                   "hu", "HU" },
      { LANGUAGE_ICELANDIC,                   "is", "IS" },
      { LANGUAGE_INDONESIAN,                  "id", "ID" },   // new: old was "in"
      { LANGUAGE_INDONESIAN,                  "in", "ID" },   // old: new is "id"
      { LANGUAGE_NORWEGIAN,                   "no", "NO" },
      { LANGUAGE_NORWEGIAN_BOKMAL,            "nb", "NO" },
      { LANGUAGE_NORWEGIAN_NYNORSK,           "nn", "NO" },
      { LANGUAGE_POLISH,                      "pl", "PL" },
      { LANGUAGE_RHAETO_ROMAN,                "rm", ""   },
      { LANGUAGE_ROMANIAN,                    "ro", "RO" },
      { LANGUAGE_ROMANIAN_MOLDOVA,            "ro", "MD" },
      { LANGUAGE_SLOVAK,                      "sk", "SK" },
      { LANGUAGE_SLOVENIAN,                   "sl", "SI" },
      { LANGUAGE_SPANISH_MEXICAN,             "es", "MX" },
      { LANGUAGE_SPANISH_GUATEMALA,           "es", "GT" },
      { LANGUAGE_SPANISH_COSTARICA,           "es", "CR" },
      { LANGUAGE_SPANISH_PANAMA,              "es", "PA" },
      { LANGUAGE_SPANISH_DOMINICAN_REPUBLIC,  "es", "DO" },
      { LANGUAGE_SPANISH_VENEZUELA,           "es", "VE" },
      { LANGUAGE_SPANISH_COLOMBIA,            "es", "CO" },
      { LANGUAGE_SPANISH_PERU,                "es", "PE" },
      { LANGUAGE_SPANISH_ARGENTINA,           "es", "AR" },
      { LANGUAGE_SPANISH_ECUADOR,             "es", "EC" },
      { LANGUAGE_SPANISH_CHILE,               "es", "CL" },
      { LANGUAGE_SPANISH_URUGUAY,             "es", "UY" },
      { LANGUAGE_SPANISH_PARAGUAY,            "es", "PY" },
      { LANGUAGE_SPANISH_BOLIVIA,             "es", "BO" },
      { LANGUAGE_SPANISH_EL_SALVADOR,         "es", "SV" },
      { LANGUAGE_SPANISH_HONDURAS,            "es", "HN" },
      { LANGUAGE_SPANISH_NICARAGUA,           "es", "NI" },
      { LANGUAGE_SPANISH_PUERTO_RICO,         "es", "PR" },
      { LANGUAGE_TURKISH,                     "tr", "TR" },
      { LANGUAGE_UKRAINIAN,                   "uk", "UA" },
      { LANGUAGE_VIETNAMESE,                  "vi", "VN" },
      { LANGUAGE_LATVIAN,                     "lv", "LV" },
      { LANGUAGE_MACEDONIAN,                  "mk", "MK" },
      { LANGUAGE_MALAY,                       "ms", ""   },
      { LANGUAGE_MALAY_MALAYSIA,              "ms", "MY" },
      { LANGUAGE_MALAY_BRUNEI_DARUSSALAM,     "ms", "BN" },
      { LANGUAGE_THAI,                        "th", "TH" },
      { LANGUAGE_LITHUANIAN,                  "lt", "LT" },
      { LANGUAGE_LITHUANIAN_CLASSIC,          "lt", "LT" },
      { LANGUAGE_CROATIAN,                    "hr", "HR" },   // Croatian in Croatia
      { LANGUAGE_SERBIAN_LATIN,               "sh", "YU" },   // Serbo-Croatian in Yugoslavia (default)
      { LANGUAGE_SERBIAN_LATIN,               "sh", "BA" },   // Serbo-Croatian in Bosnia And Herzegovina
      { LANGUAGE_SERBIAN_CYRILLIC,            "sr", "YU" },
      { LANGUAGE_SERBIAN,                     "sr", ""   },   // SERBIAN is only LID, MS-LCID not defined (was dupe of CROATIAN)
      { LANGUAGE_ARMENIAN,                    "hy", "AM" },
      { LANGUAGE_AZERI,                       "az", ""   },
      { LANGUAGE_BENGALI,                     "bn", "BD" },
      { LANGUAGE_KAZAK,                       "kk", "KZ" },
      { LANGUAGE_URDU,                        "ur", "IN" },
      { LANGUAGE_HINDI,                       "hi", "IN" },
      { LANGUAGE_GUJARATI,                    "gu", "IN" },
      { LANGUAGE_KANNADA,                     "kn", "IN" },
      { LANGUAGE_ASSAMESE,                    "as", "IN" },
      { LANGUAGE_KASHMIRI,                    "ks", ""   },
      { LANGUAGE_KASHMIRI_INDIA,              "ks", "IN" },
      { LANGUAGE_MALAYALAM,                   "ml", "IN" },
      { LANGUAGE_MARATHI,                     "mr", "IN" },
      { LANGUAGE_NEPALI,                      "ne", "NP" },
      { LANGUAGE_NEPALI_INDIA,                "ne", "IN" },
      { LANGUAGE_ORIYA,                       "or", "IN" },
      { LANGUAGE_PUNJABI,                     "pa", "IN" },
      { LANGUAGE_SANSKRIT,                    "sa", "IN" },
      { LANGUAGE_SINDHI,                      "sd", "IN" },
      { LANGUAGE_TAMIL,                       "ta", "IN" },
      { LANGUAGE_TELUGU,                      "te", "IN" },
      { LANGUAGE_BELARUSIAN,                  "be", "BY" },
      { LANGUAGE_CATALAN,                     "ca", "ES" },   // Spain (default)
      { LANGUAGE_CATALAN,                     "ca", "AD" },   // Andorra
      { LANGUAGE_FRENCH_CAMEROON,             "fr", "CM" },
      { LANGUAGE_FRENCH_COTE_D_IVOIRE,        "fr", "CI" },
      { LANGUAGE_FRENCH_MALI,                 "fr", "ML" },
      { LANGUAGE_FRENCH_SENEGAL,              "fr", "SN" },
      { LANGUAGE_FRENCH_ZAIRE,                "fr", "CD" },   // Democratic Republic Of Congo
      { LANGUAGE_FRISIAN_NETHERLANDS,         "fy", "NL" },
      { LANGUAGE_GAELIC_IRELAND,              "ga", "IE" },
      { LANGUAGE_GAELIC_SCOTLAND,             "gd", "GB" },
      { LANGUAGE_GALICIAN,                    "gl", "ES" },
      { LANGUAGE_GEORGIAN,                    "ka", "GE" },
      { LANGUAGE_KHMER,                       "km", "KH" },
      { LANGUAGE_KIRGHIZ,                     "ky", "KG" },
      { LANGUAGE_LAO,                         "lo", "LA" },
      { LANGUAGE_MALTESE,                     "mt", "MT" },
      { LANGUAGE_MONGOLIAN,                   "mn", "MN" },
      { LANGUAGE_RUSSIAN_MOLDOVA,             "mo", "MD" },
      { LANGUAGE_SESOTHO,                     "st", "LS" },   // Lesotho (default)
      { LANGUAGE_SESOTHO,                     "st", "ZA" },   // South Africa
      { LANGUAGE_SWAHILI,                     "sw", "KE" },
      { LANGUAGE_TAJIK,                       "tg", "TJ" },
      { LANGUAGE_TIBETAN,                     "bo", "CN" },   // CN politically correct?
      { LANGUAGE_TSONGA,                      "ts", "ZA" },
      { LANGUAGE_TSWANA,                      "tn", "BW" },   // Botswana (default)
      { LANGUAGE_TSWANA,                      "tn", "ZA" },   // South Africa
      { LANGUAGE_TURKMEN,                     "tk", "TM" },
      { LANGUAGE_WELSH,                       "cy", "GB" },
      { LANGUAGE_NORTHERNSOTHO,               "ns", "ZA" },
      { LANGUAGE_XHOSA,                       "xh", "ZA" },
      { LANGUAGE_ZULU,                        "zu", "ZA" },
      { LANGUAGE_USER_KINYARWANDA,            "rw", "RW" },
      { LANGUAGE_USER_MAORI,                  "mi", "NZ" },
      { LANGUAGE_USER_LATIN,                  "la", ""   },
      { LANGUAGE_USER_ESPERANTO,              "eo", ""   },
      { LANGUAGE_NORWEGIAN_BOKMAL,            "no", "BOK"      }, // registered subtags for "no" in rfc1766
      { LANGUAGE_NORWEGIAN_NYNORSK,           "no", "NYN"      }, // registered subtags for "no" in rfc1766
      { LANGUAGE_SERBIAN_LATIN,               "sr", "latin"    },
      { LANGUAGE_SERBIAN_CYRILLIC,            "sr", "cyrillic" },
      { LANGUAGE_AZERI_LATIN,                 "az", "latin"    },
      { LANGUAGE_AZERI_CYRILLIC,              "az", "cyrillic" }
    };

    for (auto const &l : idIsoList)
      m_idLanguageMap.insert(std::map<int,IdIsoLanguageEntry>::value_type(l.m_languageId,l));
  }
  bool getLanguageId(int id, std::string &lang, std::string &country) const
  {
    if (m_idLanguageMap.find(id)==m_idLanguageMap.end())
      return false;
    lang=m_idLanguageMap.find(id)->second.m_language;
    country=m_idLanguageMap.find(id)->second.m_country;
    return true;
  }
protected:
  std::map<int,IdIsoLanguageEntry> m_idLanguageMap;
};

static IdIsoLanguageMap s_idLanguageMap;
bool getLanguageId(int id, std::string &lang, std::string &country)
{
  return s_idLanguageMap.getLanguageId(id, lang, country);
}
}
// vim: set filetype=cpp tabstop=2 shiftwidth=2 cindent autoindent smartindent noexpandtab:
