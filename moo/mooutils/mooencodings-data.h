/*
 *   mooencodings-data.h
 *
 *   Copyright (C) 2004-2010 by Yevgen Muntyan <emuntyan@users.sourceforge.net>
 *
 *   This file is part of medit.  medit is free software; you can
 *   redistribute it and/or modify it under the terms of the
 *   GNU Lesser General Public License as published by the
 *   Free Software Foundation; either version 2.1 of the License,
 *   or (at your option) any later version.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with medit.  If not, see <http://www.gnu.org/licenses/>.
 */

enum {
    ENCODING_GROUP_WEST_EUROPEAN,
    ENCODING_GROUP_EAST_EUROPEAN,
    ENCODING_GROUP_EAST_ASIAN,
    ENCODING_GROUP_SE_SW_ASIAN,
    ENCODING_GROUP_MIDDLE_EASTERN,
    ENCODING_GROUP_UNICODE,
    N_ENCODING_GROUPS
};

static const char * const moo_encoding_groups_names[] = {
    N_("West European"),
    N_("East European"),
    N_("East Asian"),
    N_("SE & SW Asian"),
    N_("Middle Eastern"),
    N_("Unicode")
};

/* The encodings list below is from profterm:
 *
 * Copyright (C) 2002 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 */

static const struct {
    const char *name;
    const char *display_subgroup;
    const char *short_display_name;
    guint group : 3;
} moo_encodings_data[] =
{
    { "ASCII",          N_("Ascii"),               "ASCII",            ENCODING_GROUP_WEST_EUROPEAN },

    { "ISO-8859-1",     N_("Western"),             "ISO-8859-1",       ENCODING_GROUP_WEST_EUROPEAN },
    { "ISO-8859-2",     N_("Central European"),    "ISO-8859-2",       ENCODING_GROUP_EAST_EUROPEAN },
    { "ISO-8859-3",     N_("South European"),      "ISO-8859-3",       ENCODING_GROUP_WEST_EUROPEAN },
    { "ISO-8859-4",     N_("Baltic"),              "ISO-8859-4",       ENCODING_GROUP_EAST_EUROPEAN },
    { "ISO-8859-5",     N_("Cyrillic"),            "ISO-8859-5",       ENCODING_GROUP_EAST_EUROPEAN },
    { "ISO-8859-6",     N_("Arabic"),              "ISO-8859-6",       ENCODING_GROUP_MIDDLE_EASTERN },
    { "ISO-8859-7",     N_("Greek"),               "ISO-8859-7",       ENCODING_GROUP_WEST_EUROPEAN },
    { "ISO-8859-8",     N_("Hebrew Visual"),       "ISO-8859-8",       ENCODING_GROUP_MIDDLE_EASTERN },
    { "ISO-8859-8-I",   N_("Hebrew"),              "ISO-8859-8-I",     ENCODING_GROUP_MIDDLE_EASTERN },
    { "ISO-8859-9",     N_("Turkish"),             "ISO-8859-9",       ENCODING_GROUP_SE_SW_ASIAN },
    { "ISO-8859-10",    N_("Nordic"),              "ISO-8859-10",      ENCODING_GROUP_WEST_EUROPEAN },
    { "ISO-8859-13",    N_("Baltic"),              "ISO-8859-13",      ENCODING_GROUP_EAST_EUROPEAN },
    { "ISO-8859-14",    N_("Celtic"),              "ISO-8859-14",      ENCODING_GROUP_WEST_EUROPEAN },
    { "ISO-8859-15",    N_("Western"),             "ISO-8859-15",      ENCODING_GROUP_WEST_EUROPEAN },
    { "ISO-8859-16",    N_("Romanian"),            "ISO-8859-16",      ENCODING_GROUP_EAST_EUROPEAN },

    // Label of an encoding menu item, remove the part before and including |
    { "UTF-7",          NULL, N_("encoding label|UTF-7"),                                 ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32BE-BOM",   NULL, N_("encoding label|UTF-32BE-BOM (big-endian, with BOM)"),   ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32LE-BOM",   NULL, N_("encoding label|UTF-32LE-BOM (little-endian, with BOM)"),ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32BE",       NULL, N_("encoding label|UTF-32BE (big-endian, no BOM)"),         ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32LE",       NULL, N_("encoding label|UTF-32LE (little-endian, no BOM)"),      ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16BE-BOM",   NULL, N_("encoding label|UTF-16BE-BOM (big-endian, with BOM)"),   ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16LE-BOM",   NULL, N_("encoding label|UTF-16LE-BOM (little-endian, with BOM)"),ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16BE",       NULL, N_("encoding label|UTF-16BE (big-endian, no BOM)"),         ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16LE",       NULL, N_("encoding label|UTF-16LE (little-endian, no BOM)"),      ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-8-BOM",      NULL, N_("encoding label|UTF-8-BOM"),                             ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32-BOM",     NULL, N_("encoding label|UTF-32-BOM"),                            ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16-BOM",     NULL, N_("encoding label|UTF-16-BOM"),                            ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32-BOM",     NULL, N_("encoding label|UTF-32-BOM"),                            ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16-BOM",     NULL, N_("encoding label|UTF-16-BOM"),                            ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-32",         NULL, N_("encoding label|UTF-32"),                                ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-16",         NULL, N_("encoding label|UTF-16"),                                ENCODING_GROUP_UNICODE },
    // Label of an encoding menu item, remove the part before and including |
    { "UTF-8",          NULL, N_("encoding label|UTF-8"),                                 ENCODING_GROUP_UNICODE },

    { "ARMSCII-8",      N_("Armenian"),            "ARMSCII-8",        ENCODING_GROUP_SE_SW_ASIAN },
    { "BIG5",           N_("Chinese Traditional"), "Big5",             ENCODING_GROUP_EAST_ASIAN },
    { "BIG5-HKSCS",     N_("Chinese Traditional"), "Big5-HKSCS",       ENCODING_GROUP_EAST_ASIAN },
    { "CP866",          N_("Cyrillic/Russian"),    "CP866",            ENCODING_GROUP_EAST_EUROPEAN },

    { "EUC-JP",         N_("Japanese"),            "EUC-JP",           ENCODING_GROUP_EAST_ASIAN },
    { "EUC-KR",         N_("Korean"),              "EUC-KR",           ENCODING_GROUP_EAST_ASIAN },
    { "EUC-TW",         N_("Chinese Traditional"), "EUC-TW",           ENCODING_GROUP_EAST_ASIAN },

    { "GB18030",        N_("Chinese Simplified"),  "GB18030",          ENCODING_GROUP_EAST_ASIAN },
    { "GB2312",         N_("Chinese Simplified"),  "GB2312",           ENCODING_GROUP_EAST_ASIAN },
    { "GBK",            N_("Chinese Simplified"),  "GBK",              ENCODING_GROUP_EAST_ASIAN },
    { "GEORGIAN-PS",    N_("Georgian"),            "GEORGIAN-PS",      ENCODING_GROUP_SE_SW_ASIAN },
    { "HZ",             N_("Chinese Simplified"),  "HZ",               ENCODING_GROUP_EAST_ASIAN },

    { "IBM850",         N_("Western"),             "IBM850",           ENCODING_GROUP_WEST_EUROPEAN },
    { "IBM852",         N_("Central European"),    "IBM852",           ENCODING_GROUP_EAST_EUROPEAN },
    { "IBM855",         N_("Cyrillic"),            "IBM855",           ENCODING_GROUP_EAST_EUROPEAN },
    { "IBM857",         N_("Turkish"),             "IBM857",           ENCODING_GROUP_SE_SW_ASIAN },
    { "IBM862",         N_("Hebrew"),              "IBM862",           ENCODING_GROUP_MIDDLE_EASTERN },
    { "IBM864",         N_("Arabic"),              "IBM864",           ENCODING_GROUP_MIDDLE_EASTERN },

    { "ISO2022JP",      N_("Japanese"),            "ISO2022JP",        ENCODING_GROUP_EAST_ASIAN },
    { "ISO2022KR",      N_("Korean"),              "ISO2022KR",        ENCODING_GROUP_EAST_ASIAN },
    { "ISO-IR-111",     N_("Cyrillic"),            "ISO-IR-111",       ENCODING_GROUP_EAST_EUROPEAN },
    { "JOHAB",          N_("Korean"),              "JOHAB",            ENCODING_GROUP_EAST_ASIAN },
    { "KOI8-R",         N_("Cyrillic"),            "KOI8-R",           ENCODING_GROUP_EAST_EUROPEAN },
    { "KOI8-U",         N_("Cyrillic/Ukrainian"),  "KOI8-U",           ENCODING_GROUP_EAST_EUROPEAN },

    { "MAC_ARABIC",     N_("Arabic"),              "MacArabic",        ENCODING_GROUP_MIDDLE_EASTERN },
    { "MAC_CE",         N_("Central European"),    "MacCE",            ENCODING_GROUP_EAST_EUROPEAN },
    { "MAC_CROATIAN",   N_("Croatian"),            "MacCroatian",      ENCODING_GROUP_EAST_EUROPEAN },
    { "MAC-CYRILLIC",   N_("Cyrillic"),            "MacCyrillic",      ENCODING_GROUP_EAST_EUROPEAN },
    { "MAC_DEVANAGARI", N_("Hindi"),               "MacDevanagari",    ENCODING_GROUP_SE_SW_ASIAN },
    { "MAC_FARSI",      N_("Persian"),             "MacFarsi",         ENCODING_GROUP_MIDDLE_EASTERN },
    { "MAC_GREEK",      N_("Greek"),               "MacGreek",         ENCODING_GROUP_WEST_EUROPEAN },
    { "MAC_GUJARATI",   N_("Gujarati"),            "MacGujarati",      ENCODING_GROUP_SE_SW_ASIAN },
    { "MAC_GURMUKHI",   N_("Gurmukhi"),            "MacGurmukhi",      ENCODING_GROUP_SE_SW_ASIAN },
    { "MAC_HEBREW",     N_("Hebrew"),              "MacHebrew",        ENCODING_GROUP_MIDDLE_EASTERN },
    { "MAC_ICELANDIC",  N_("Icelandic"),           "MacIcelandic",     ENCODING_GROUP_WEST_EUROPEAN },
    { "MAC_ROMAN",      N_("Western"),             "MacRoman",         ENCODING_GROUP_WEST_EUROPEAN },
    { "MAC_ROMANIAN",   N_("Romanian"),            "MacRomanian",      ENCODING_GROUP_EAST_EUROPEAN },
    { "MAC_TURKISH",    N_("Turkish"),             "MacTurkish",       ENCODING_GROUP_SE_SW_ASIAN },
    { "MAC_UKRAINIAN",  N_("Cyrillic/Ukrainian"),  "MacUkrainian",     ENCODING_GROUP_EAST_EUROPEAN },

    { "SHIFT-JIS",      N_("Japanese"),            "Shift_JIS",        ENCODING_GROUP_EAST_ASIAN },
    { "TCVN",           N_("Vietnamese"),          "TCVN",             ENCODING_GROUP_EAST_ASIAN },
    { "TIS-620",        N_("Thai"),                "TIS-620",          ENCODING_GROUP_SE_SW_ASIAN },
    { "UHC",            N_("Korean"),              "UHC",              ENCODING_GROUP_EAST_ASIAN },
    { "VISCII",         N_("Vietnamese"),          "VISCII",           ENCODING_GROUP_EAST_ASIAN },

    { "WINDOWS-1250",   N_("Central European"),    "Windows-1250",     ENCODING_GROUP_EAST_EUROPEAN },
    { "WINDOWS-1251",   N_("Cyrillic"),            "Windows-1251",     ENCODING_GROUP_EAST_EUROPEAN },
    { "WINDOWS-1252",   N_("Western"),             "Windows-1252",     ENCODING_GROUP_WEST_EUROPEAN },
    { "WINDOWS-1253",   N_("Greek"),               "Windows-1253",     ENCODING_GROUP_WEST_EUROPEAN },
    { "WINDOWS-1254",   N_("Turkish"),             "Windows-1254",     ENCODING_GROUP_SE_SW_ASIAN },
    { "WINDOWS-1255",   N_("Hebrew"),              "Windows-1255",     ENCODING_GROUP_MIDDLE_EASTERN },
    { "WINDOWS-1256",   N_("Arabic"),              "Windows-1256",     ENCODING_GROUP_MIDDLE_EASTERN },
    { "WINDOWS-1257",   N_("Baltic"),              "Windows-1257",     ENCODING_GROUP_EAST_EUROPEAN },
    { "WINDOWS-1258",   N_("Vietnamese"),          "Windows-1258",     ENCODING_GROUP_EAST_ASIAN }
};


/* The alias list is from config.charset
 *
#   Copyright (C) 2000-2002 Free Software Foundation, Inc.
#
#   This program is free software; you can redistribute it and/or modify it
#   under the terms of the GNU Library General Public License as published
#   by the Free Software Foundation; either version 2, or (at your option)
#   any later version.
 */

static const struct {
    const char *alias;
    const char *name;
} moo_encoding_aliases[] = {
    {"UTF8",        "UTF-8"},
    {"UTF16",       "UTF-16"},
    {"UTF32",       "UTF-32"},

    {"646",         "ASCII"},
    {"C",           "ASCII"},
    {"US-ASCII",    "ASCII"},
    {"CP20127",     "ASCII"},

    {"CP20866",     "KOI8-R"},
#if 0
    {"CP21866",   "KOI8-RU"},
#endif

    {"CP936",       "GBK"},
    {"CP1361",      "JOHAB"},
    {"IBM-EUCCN",   "GB2312"},
    {"EUCCN",       "GB2312"},
    {"IBM-EUCJP",   "EUC-JP"},
    {"EUCJP",       "EUC-JP"},
    {"IBM-EUCKR",   "EUC-KR"},
    {"EUCCR",       "EUC-KR"},
    {"5601",        "EUC-KR"},
    {"IBM-EUCTW",   "EUC-TW"},
    {"EUCTW",       "EUC-TW"},
    {"CNS11643",    "EUC-TW"},
    {"PCK",         "SHIFT-JIS"},
    {"SHIFT_JIS",   "SHIFT-JIS"},

    {"CP28591",     "ISO-8859-1"},
    {"CP28592",     "ISO-8859-2"},
    {"CP28593",     "ISO-8859-3"},
    {"CP28594",     "ISO-8859-4"},
    {"CP28595",     "ISO-8859-5"},
    {"CP28596",     "ISO-8859-6"},
    {"CP28597",     "ISO-8859-7"},
    {"CP28598",     "ISO-8859-8"},
    {"CP28599",     "ISO-8859-9"},
    {"CP28605",     "ISO-8859-15"},
    {"ISO8859-1",   "ISO-8859-1"},
    {"ISO8859-2",   "ISO-8859-2"},
    {"ISO8859-3",   "ISO-8859-3"},
    {"ISO8859-4",   "ISO-8859-4"},
    {"ISO8859-5",   "ISO-8859-5"},
    {"ISO8859-6",   "ISO-8859-6"},
    {"ISO8859-7",   "ISO-8859-7"},
    {"ISO8859-8",   "ISO-8859-8"},
    {"ISO8859-9",   "ISO-8859-9"},
    {"ISO8859-15",  "ISO-8859-15"},
    {"ISO_8859-1",  "ISO-8859-1"},
    {"ISO_8859-2",  "ISO-8859-2"},
    {"ISO_8859-3",  "ISO-8859-3"},
    {"ISO_8859-4",  "ISO-8859-4"},
    {"ISO_8859-5",  "ISO-8859-5"},
    {"ISO_8859-6",  "ISO-8859-6"},
    {"ISO_8859-7",  "ISO-8859-7"},
    {"ISO_8859-8",  "ISO-8859-8"},
    {"ISO_8859-9",  "ISO-8859-9"},
    {"ISO_8859-15", "ISO-8859-15"},
    {"ISO88591",    "ISO-8859-1"},
    {"ISO88592",    "ISO-8859-2"},
    {"ISO88593",    "ISO-8859-3"},
    {"ISO88594",    "ISO-8859-4"},
    {"ISO88595",    "ISO-8859-5"},
    {"ISO88596",    "ISO-8859-6"},
    {"ISO88597",    "ISO-8859-7"},
    {"ISO88598",    "ISO-8859-8"},
    {"ISO88599",    "ISO-8859-9"},
    {"ISO885915",   "ISO-8859-15"},

    {"CP850",       "IBM850"},
    {"IBM-850",     "IBM850"},
    {"IBM-1252",    "WINDOWS-1252"},
    {"CP1252",      "WINDOWS-1252"},
    {"ANSI-1251",   "WINDOWS-1251"},
    {"CP1251",      "WINDOWS-1251"},
#if 0
    {"IBM-921",     "ISO-8859-13"},
    {"IBM-856",     "CP856"},
    {"IBM-922",     "CP922"},
    {"IBM-932",     "CP932"},
    {"IBM-943",     "CP943"},
    {"IBM-1046",    "CP1046"},
    {"IBM-1124",    "CP1124"},
    {"IBM-1129",    "CP1129"},
#endif
};
