/*
 * bltypes.c
 *
 * Copyright (c) Chris Putnam 2008-2012
 * Copyright (c) Johannes Wilm 2010-2012
 *
 * Program and source code released under the GPL
 *
 */

#include <stdio.h>
#include "fields.h"
#include "reftypes.h"

/* Entry types for biblatex formatted bibliographies */

/*
 * Article in a journal, newspaper, other periodical
 */
static lookups article[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_MAIN },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_MAIN },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_MAIN },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?", SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "journal",         "TITLE",        SIMPLE, LEVEL_HOST },
	{ "journaltitle",    "TITLE",        SIMPLE, LEVEL_HOST },
	{ "journalsubtitle", "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "shortjournal",    "SHORTTITLE",   SIMPLE, LEVEL_HOST },
	{ "issuetitle",      "TITLE",        SIMPLE, LEVEL_SERIES }, /*WRONG*/
	{ "issuesubtitle",   "SUBTITLE",     SIMPLE, LEVEL_SERIES }, /*WRONG*/
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "origyear",        "YEAR",         SIMPLE, LEVEL_ORIG },
	{ "origtitle",       "TITLE",        SIMPLE, LEVEL_ORIG },
	{ "origlocation",    "LOCATION",     SIMPLE, LEVEL_ORIG },
	{ "origpublisher",   "PUBLISHER",    SIMPLE, LEVEL_ORIG },
	{ "series",          "TITLE",        SIMPLE, LEVEL_SERIES },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_MAIN },
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "eid",             "EID",          SIMPLE, LEVEL_MAIN },
	{ "issue",           "ISSUE",        SIMPLE, LEVEL_MAIN },
	{ "date",            "DATE",         SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "day",             "PARTDAY",      SIMPLE, LEVEL_MAIN },
	{ "month",           "PARTMONTH",    SIMPLE, LEVEL_MAIN },
	{ "year",            "PARTYEAR",     SIMPLE, LEVEL_MAIN },
	{ "pages",           "PAGES",        PAGES,  LEVEL_MAIN },
	{ "version",         "VERSION",      SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "issn",            "ISSN",         SIMPLE, LEVEL_HOST },
	{ "abstract",        "ABSTRACT",     SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?addendum?",   SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",  BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",     SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "urlmonth",        "?urlmonth?",   SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",            "INTERNAL_TYPE|ARTICLE",           ALWAYS, LEVEL_MAIN },
	{ " ",            "ISSUANCE|continuing",    ALWAYS, LEVEL_HOST },
	{ " ",            "RESOURCE|text",          ALWAYS, LEVEL_MAIN },
        { " ",            "GENRE|journal article",  ALWAYS, LEVEL_MAIN },
	{ " ",            "GENRE|periodical",       ALWAYS, LEVEL_HOST }
};

/* Book */

static lookups book[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_MAIN },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_MAIN },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_MAIN },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_MAIN },
	{ "introduction",    "?introduction?",SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "foreward",        "?foreward?",   SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "afterward",       "?afterward?",  SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?", SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "maintitle",       "TITLE",        SIMPLE, LEVEL_HOST },
	{ "mainsubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "maintitleaddon",  "?maintitleaddon?",SIMPLE, LEVEL_HOST },   /*WRONG*/
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "origyear",        "YEAR",         SIMPLE, LEVEL_ORIG },
	{ "origtitle",       "TITLE",        SIMPLE, LEVEL_ORIG },
	{ "origlocation",    "LOCATION",     SIMPLE, LEVEL_ORIG },
	{ "origpublisher",   "PUBLISHER",    SIMPLE, LEVEL_ORIG },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_HOST },
	{ "part",            "PART",         SIMPLE, LEVEL_HOST },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "volumes",         "NUMVOLUMES",   SIMPLE, LEVEL_HOST },
	{ "series",          "TITLE",        SIMPLE, LEVEL_HOST }, /* WRONG */
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "?pages?",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "pagetotal",       "?pagetotal?",    SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?addendum?",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?urlmonth?",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|BOOK",    ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",               "ISSUANCE|monographic", ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|book",   ALWAYS, LEVEL_MAIN }
};

/* Booklet */

static lookups booklet[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "howpublished",    "",       HOWPUBLISHED, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "type",            "GENRE",  BIBTEX_GENRE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "?pages?",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "pagetotal",       "?pagetotal?",    SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?addendum?",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?urlmonth?",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|BOOK",    ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",               "ISSUANCE|monographic", ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|book",   ALWAYS, LEVEL_MAIN }
};

static lookups collection[] = {
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_MAIN },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_MAIN },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_MAIN },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "introduction",    "?introduction?",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "foreward",        "?foreward?",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "afterward",       "?afterward?",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "maintitle",       "TITLE",        SIMPLE, LEVEL_HOST },
	{ "mainsubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "maintitleaddon",  "?maintitleaddon?",        SIMPLE, LEVEL_HOST },   /*WRONG*/
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "origyear",        "YEAR",         SIMPLE, LEVEL_ORIG },
	{ "origtitle",       "TITLE",        SIMPLE, LEVEL_ORIG },
	{ "origlocation",    "LOCATION",     SIMPLE, LEVEL_ORIG },
	{ "origpublisher",   "PUBLISHER",    SIMPLE, LEVEL_ORIG },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_HOST },
	{ "part",            "PART",         SIMPLE, LEVEL_HOST },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "volumes",         "NUMVOLUMES",   SIMPLE, LEVEL_HOST },
	{ "series",          "?series?",        SIMPLE, LEVEL_HOST }, /* WRONG */
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "?pages?",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "pagetotal",       "?pagetotal?",    SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?addendum?",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?urlmonth?",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|BOOK",    ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",               "ISSUANCE|monographic", ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|book",   ALWAYS, LEVEL_MAIN }
};

/* Part of a book (e.g. chapter or section) */

static lookups inbook[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_HOST },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_HOST },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_HOST },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_HOST },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_HOST },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "introduction",    "?introduction?",        SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "foreward",        "?foreward?",        SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "afterward",       "?afterward?",        SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "maintitle",       "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "mainsubtitle",    "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "maintitleaddon",  "?maintitleaddon?",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "booktitle",       "TITLE",        SIMPLE, LEVEL_HOST },
	{ "booksubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "booktitleaddon",  "?booktitleaddon?",       SIMPLE, LEVEL_HOST },
	{ "bookauthor",      "AUTHOR",       PERSON, LEVEL_HOST },
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "origyear",        "YEAR",         SIMPLE, LEVEL_ORIG },
	{ "origtitle",       "TITLE",        SIMPLE, LEVEL_ORIG },
	{ "origlocation",    "LOCATION",     SIMPLE, LEVEL_ORIG },
	{ "origpublisher",   "PUBLISHER",    SIMPLE, LEVEL_ORIG },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_MAIN },
	{ "pages",           "PAGES",        PAGES,  LEVEL_MAIN },
	{ "part",            "PART",         SIMPLE, LEVEL_HOST },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "volumes",         "NUMVOLUMES",   SIMPLE, LEVEL_HOST },
	{ "series",          "?series?",     SIMPLE, LEVEL_HOST }, /* WRONG */
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_HOST },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?addendum?",   SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",     SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?urlmonth?",   SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|INBOOK",    ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",               "ISSUANCE|monographic", ALWAYS, LEVEL_HOST },
        { " ",               "GENRE|book chapter",   ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|book",   ALWAYS, LEVEL_HOST }
};

/* incollection */

static lookups incollection[] = {

	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?", SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },

	{ "introduction",    "?introduction?",SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "foreward",        "?foreward?",   SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "afterward",       "?afterward?",  SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "bookauthor",      "AUTHOR",       PERSON, LEVEL_HOST },
	{ "booktitle",       "TITLE",        SIMPLE, LEVEL_HOST },
	{ "booksubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "booktitleaddon",  "?booktitleaddon?",SIMPLE, LEVEL_HOST },
	{ "editor",          "EDITOR",       PERSON, LEVEL_HOST },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_HOST },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_HOST },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_HOST },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_HOST },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_HOST },
	{ "part",            "PART",         SIMPLE, LEVEL_HOST },
	{ "volumes",         "NUMVOLUMES",   SIMPLE, LEVEL_HOST },
	
	{ "maintitle",       "TITLE",        SIMPLE, LEVEL_HOST+1 },
	{ "mainsubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST+1 },
	{ "maintitleaddon",  "?maintitleaddon?",        SIMPLE, LEVEL_HOST+1 },   /*WRONG*/

	{ "series",          "TITLE",        SIMPLE, LEVEL_HOST+2 }, /* WRONG */

	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "origyear",        "YEAR",         SIMPLE, LEVEL_ORIG },
	{ "origtitle",       "TITLE",        SIMPLE, LEVEL_ORIG },
	{ "origlocation",    "LOCATION",     SIMPLE, LEVEL_ORIG },
	{ "origpublisher",   "PUBLISHER",    SIMPLE, LEVEL_ORIG },

	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_HOST },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_HOST },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "PAGES",        PAGES,  LEVEL_MAIN }, /* WRONG */
	{ "addendum",        "?addendum?",   SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",     SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?urlmonth?",   SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|INCOLLECTION",    ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",               "ISSUANCE|monographic", ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|collection",   ALWAYS, LEVEL_HOST }
};

/* inproceedings */

static lookups inproceedings[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_HOST },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_HOST },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_HOST },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_HOST },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_HOST },
	{ "eventtitle",      "EVENT:CONF",   SIMPLE, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "introduction",    "?introduction?",SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "foreward",        "?foreward?",   SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "afterward",       "?afterward?",  SIMPLE, LEVEL_HOST }, /*WRONG*/
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?titleaddon?", SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "booktitle",       "TITLE",        SIMPLE, LEVEL_HOST },
	{ "booksubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "booktitleaddon",  "?booktitleaddon?",       SIMPLE, LEVEL_HOST },   /*WRONG*/
	{ "maintitle",       "TITLE",        SIMPLE, LEVEL_HOST+1 },
	{ "mainsubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST+1 },
	{ "maintitleaddon",  "?maintitleaddon?",        SIMPLE, LEVEL_HOST+1 },   /*WRONG*/
	{ "series",          "TITLE",        SIMPLE, LEVEL_HOST+2 },
	{ "venue",           "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "organization",    "ORGANIZER:CORP", SIMPLE, LEVEL_MAIN },
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_HOST },
	{ "part",            "PART",         SIMPLE, LEVEL_HOST },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "volumes",         "NUMVOLUMES",   SIMPLE, LEVEL_HOST },
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_HOST },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_HOST },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "PAGES",        PAGES,  LEVEL_MAIN }, /* WRONG */
	{ "addendum",        "?addendnum?",  SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?urlday?",     SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?urlmonth?",   SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|INPROCEEDINGS",    ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",               "ISSUANCE|monographic", ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|conference publication",   ALWAYS, LEVEL_HOST }
};

static lookups manual[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_MAIN },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_MAIN },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_MAIN },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "introduction",    "?????",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "foreward",        "?????",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "afterward",       "?????",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "version",         "???????",      SIMPLE, LEVEL_MAIN },
	{ "type",            "GENRE",  BIBTEX_GENRE, LEVEL_MAIN },
	{ "series",          "?????",        SIMPLE, LEVEL_HOST }, /* WRONG */
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "organization",    "ORGANIZER:CORP", SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "?????????",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "pagetotal",       "?????????",    SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|REPORT",       ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",     ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|instruction", ALWAYS, LEVEL_MAIN }
};

static lookups misc[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "day",             "DAY",          SIMPLE, LEVEL_MAIN },
	{ "month",           "MONTH",        SIMPLE, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "howpublished",    "",       HOWPUBLISHED, LEVEL_MAIN },
	{ "version",         "???????",      SIMPLE, LEVEL_MAIN },
	{ "type",            "GENRE",  BIBTEX_GENRE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "organization",    "ORGANIZER:CORP",SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|MISC",       ALWAYS, LEVEL_MAIN },
};

static lookups online[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "date",            "DATE",         SIMPLE, LEVEL_MAIN },
	{ "day",             "DAY",          SIMPLE, LEVEL_MAIN },
	{ "month",           "MONTH",        SIMPLE, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "version",         "???????",      SIMPLE, LEVEL_MAIN },
	{ "type",            "GENRE",  BIBTEX_GENRE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "organization",    "ORGANIZER:CORP",    SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
        { " ",  "RESOURCE|software, multimedia",    ALWAYS, LEVEL_MAIN },
	{ " ",         "GENRE|web page",       ALWAYS, LEVEL_MAIN },
};

static lookups patent[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "holder",          "ASSIGNEE",     PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "date",            "DATE",         SIMPLE, LEVEL_MAIN },
	{ "day",             "DAY",          SIMPLE, LEVEL_MAIN },
	{ "month",           "MONTH",        SIMPLE, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "version",         "???????",      SIMPLE, LEVEL_MAIN },
	{ "type",            "GENRE",  BIBTEX_GENRE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "organization",    "ORGANIZER:CORP",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",   ALWAYS, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|PATENT",    ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|patent",    ALWAYS, LEVEL_MAIN },
};

/*
 * An entire issue of a periodical
 */
static lookups periodical[] = {
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "issuetitle",      "TITLE",        SIMPLE, LEVEL_SERIES }, /*WRONG*/
	{ "issuesubtitle",   "SUBTITLE",     SIMPLE, LEVEL_SERIES }, /*WRONG*/
	{ "series",          "TITLE",        SIMPLE, LEVEL_SERIES },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_MAIN },
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "issue",           "ISSUE",        SIMPLE, LEVEL_MAIN },
	{ "date",            "DATE",         SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "day",             "PARTDAY",      SIMPLE, LEVEL_MAIN },
	{ "month",           "PARTMONTH",    SIMPLE, LEVEL_MAIN },
	{ "year",            "PARTYEAR",     SIMPLE, LEVEL_MAIN },
	{ "pages",           "PAGES",        PAGES,  LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "issn",            "ISSN",         SIMPLE, LEVEL_HOST },
	{ "addendum",        "?????",        SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????",        SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "urlmonth",        "?????",        SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "urlyear",         "?????",        SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",            "ISSUANCE|continuing",    ALWAYS, LEVEL_MAIN },
	{ " ",            "RESOURCE|text",          ALWAYS, LEVEL_MAIN },
	{ " ",            "GENRE|periodical",       ALWAYS, LEVEL_MAIN }
};

static lookups proceedings[] = {
	{ "editor",          "EDITOR",       PERSON, LEVEL_MAIN },
	{ "redactor",        "REDACTOR",     PERSON, LEVEL_MAIN },
	{ "annotator",       "ANNOTATOR",    PERSON, LEVEL_MAIN },
	{ "commentator",     "COMMENTATOR",  PERSON, LEVEL_MAIN },
	{ "translator",      "TRANSLATOR",   PERSON, LEVEL_MAIN },
	{ "eventtitle",      "EVENT:CONF",   SIMPLE, LEVEL_MAIN },
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "introduction",    "?????",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "foreward",        "?????",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "afterward",       "?????",        SIMPLE, LEVEL_MAIN }, /*WRONG*/
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "maintitle",       "TITLE",        SIMPLE, LEVEL_HOST },
	{ "mainsubtitle",    "SUBTITLE",     SIMPLE, LEVEL_HOST },
	{ "maintitleaddon",  "?????",        SIMPLE, LEVEL_HOST },   /*WRONG*/
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "origlanguage",    "LANGUAGE",     SIMPLE, LEVEL_ORIG },
	{ "volume",          "VOLUME",       SIMPLE, LEVEL_HOST },
	{ "part",            "PART",         SIMPLE, LEVEL_HOST },
	{ "edition",         "EDITION",      SIMPLE, LEVEL_MAIN },
	{ "volumes",         "NUMVOLUMES",   SIMPLE, LEVEL_HOST },
	{ "series",          "?????",        SIMPLE, LEVEL_HOST }, /* WRONG */
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "organization",    "ORGANIZER:CORP",SIMPLE, LEVEL_MAIN },
	{ "publisher",       "PUBLISHER",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "isbn",            "ISBN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "?????????",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "pagetotal",       "?????????",    SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|BOOK",    ALWAYS, LEVEL_MAIN },
        { " ",               "RESOURCE|text",       ALWAYS, LEVEL_MAIN },
        { " ",               "GENRE|conference publication",   ALWAYS, LEVEL_MAIN }
};



/* Technical reports */
static lookups report[] = {
	{ "author",          "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "title",           "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",        "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",      "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "year",            "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "language",        "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "number",          "NUMBER",       SIMPLE, LEVEL_MAIN },
	{ "note",            "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",          "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",      "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "version",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "location",        "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "isrn",            "ISRN",         SIMPLE, LEVEL_MAIN },
	{ "chapter",         "CHAPTER",      SIMPLE, LEVEL_MAIN },
	{ "pages",           "?????????",    SIMPLE, LEVEL_MAIN }, /* WRONG */
	{ "pagetotal",       "?????????",    SIMPLE, LEVEL_MAIN },
	{ "addendum",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "doi",             "DOI",          SIMPLE, LEVEL_MAIN },
	{ "eprint",          "",       BIBTEX_EPRINT, LEVEL_MAIN },
	{ "eprinttype",      "",   BIBTEX_EPRINT, LEVEL_MAIN },
	{ "url",             "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",         "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",          "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",        "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",         "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",         "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",          "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",               "INTERNAL_TYPE|REPORT",     ALWAYS, LEVEL_MAIN },
	{ " ",               "RESOURCE|text",   ALWAYS, LEVEL_MAIN },
	{ " ",               "GENRE|report",    ALWAYS, LEVEL_MAIN }
};

static lookups thesis[] = {
	{ "author",    "AUTHOR",             PERSON, LEVEL_MAIN },
	{ "title",     "TITLE",              SIMPLE, LEVEL_MAIN },
	{ "subtitle",  "SUBTITLE",           SIMPLE, LEVEL_MAIN },
	{ "titleaddon","?????",              SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "year",      "YEAR",               SIMPLE, LEVEL_MAIN },
	{ "month",     "MONTH",              SIMPLE, LEVEL_MAIN },
	{ "day",       "DAY",                SIMPLE, LEVEL_MAIN },
	{ "type",      "GENRE",        BIBTEX_GENRE, LEVEL_MAIN },
	{ "institution","DEGREEGRANTOR:ASIS",SIMPLE, LEVEL_MAIN },
	{ "school",     "DEGREEGRANTOR:ASIS",SIMPLE, LEVEL_MAIN },
	{ "doi",        "DOI",               SIMPLE, LEVEL_MAIN },
	{ "howpublished",    "",       HOWPUBLISHED, LEVEL_MAIN },
	{ "url",        "",              BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",    "URLDATE",           SIMPLE, LEVEL_MAIN },
	{ "urlday",     "?urlday?",          SIMPLE, LEVEL_MAIN },
	{ "urlmonth",   "?urlmonth?",        SIMPLE, LEVEL_MAIN },
	{ "urlyear",    "?urlyear?",         SIMPLE, LEVEL_MAIN },
	{ "language",   "LANGUAGE",          SIMPLE, LEVEL_MAIN },
	{ "location",   "LOCATION",          SIMPLE, LEVEL_MAIN },
	{ "note",       "NOTES",             SIMPLE, LEVEL_MAIN },
	{ "annote",     "NOTES",             SIMPLE, LEVEL_MAIN },
	{ "annotation", "NOTES",             SIMPLE, LEVEL_MAIN },
	{ "address",    "ADDRESS",           SIMPLE, LEVEL_MAIN },
	{ "refnum",    "REFNUM",             SIMPLE, LEVEL_MAIN },
	{ " ",         "INTERNAL_TYPE|THESIS",        ALWAYS, LEVEL_MAIN },
	{ " ",         "RESOURCE|text",      ALWAYS, LEVEL_MAIN },
	{ " ",         "GENRE|thesis",       ALWAYS, LEVEL_MAIN },
};

/* Unpublished */
static lookups unpublished[] = {
	{ "author",       "AUTHOR",       PERSON, LEVEL_MAIN },
	{ "title",        "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",     "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon",   "?????",        SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "howpublished", "",       HOWPUBLISHED, LEVEL_MAIN },
	{ "year",         "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "month",        "MONTH",        SIMPLE, LEVEL_MAIN },
	{ "day",          "DAY",          SIMPLE, LEVEL_MAIN },
	{ "date",         "DATE",         SIMPLE, LEVEL_MAIN },
	{ "url",          "",         BIBTEX_URL, LEVEL_MAIN },
	{ "urlday",       "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlmonth",     "?????????",    SIMPLE, LEVEL_MAIN },
	{ "urlyear",      "?????????",    SIMPLE, LEVEL_MAIN },
	{ "language",     "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "note",         "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",       "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation",   "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "addendum",     "?????????",    SIMPLE, LEVEL_MAIN },
	{ "address",      "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",       "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",            "INTERNAL_TYPE|BOOK",    ALWAYS, LEVEL_MAIN },
	{ " ",            "RESOURCE|text",ALWAYS, LEVEL_MAIN },
	{ " ",            "GENRE|unpublished",ALWAYS, LEVEL_MAIN }
};

static lookups phdthesis[] = {
	{ "author",     "AUTHOR",    PERSON, LEVEL_MAIN },
	{ "title",      "TITLE",        SIMPLE, LEVEL_MAIN },
	{ "subtitle",   "SUBTITLE",     SIMPLE, LEVEL_MAIN },
	{ "titleaddon", "?titleadon?",  SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "year",       "YEAR",         SIMPLE, LEVEL_MAIN },
	{ "month",      "MONTH",        SIMPLE, LEVEL_MAIN },
	{ "day",        "DAY",          SIMPLE, LEVEL_MAIN },
	{ "type",       "GENRE",  BIBTEX_GENRE, LEVEL_MAIN },
	{ "institution","DEGREEGRANTOR:ASIS",SIMPLE, LEVEL_MAIN },
	{ "school",     "DEGREEGRANTOR:ASIS",SIMPLE, LEVEL_MAIN },
	{ "doi",        "DOI",         SIMPLE, LEVEL_MAIN },
	{ "howpublished",    "",       HOWPUBLISHED, LEVEL_MAIN },
	{ "url",        "",        BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",    "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",     "?urlday?",     SIMPLE, LEVEL_MAIN },
	{ "urlmonth",   "?urlmonth?",   SIMPLE, LEVEL_MAIN },
	{ "urlyear",    "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "language",   "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "location",   "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "note",       "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",     "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation", "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "address",    "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",     "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",          "INTERNAL_TYPE|THESIS",   ALWAYS, LEVEL_MAIN },
	{ " ",          "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ " ",          "GENRE|Ph.D. thesis",  ALWAYS, LEVEL_MAIN }
};

static lookups mastersthesis[] = {
	{ "author",     "AUTHOR",            PERSON, LEVEL_MAIN },
	{ "title",      "TITLE",             SIMPLE, LEVEL_MAIN },
	{ "subtitle",   "SUBTITLE",          SIMPLE, LEVEL_MAIN },
	{ "titleaddon", "?titleaddon?",      SIMPLE, LEVEL_MAIN },   /*WRONG*/
	{ "year",       "YEAR",              SIMPLE, LEVEL_MAIN },
	{ "month",      "MONTH",             SIMPLE, LEVEL_MAIN },
	{ "day",        "DAY",               SIMPLE, LEVEL_MAIN },
	{ "type",       "GENRE",       BIBTEX_GENRE, LEVEL_MAIN },
	{ "institution","DEGREEGRANTOR:ASIS",SIMPLE, LEVEL_MAIN },
	{ "school",     "DEGREEGRANTOR:ASIS",SIMPLE, LEVEL_MAIN },
	{ "doi",        "DOI",               SIMPLE, LEVEL_MAIN },
	{ "howpublished",    "",       HOWPUBLISHED, LEVEL_MAIN },
	{ "url",        "",              BIBTEX_URL, LEVEL_MAIN },
	{ "urldate",    "URLDATE",      SIMPLE, LEVEL_MAIN },
	{ "urlday",     "?urlday?",     SIMPLE, LEVEL_MAIN },
	{ "urlmonth",   "?urlmonth?",   SIMPLE, LEVEL_MAIN },
	{ "urlyear",    "?urlyear?",    SIMPLE, LEVEL_MAIN },
	{ "language",   "LANGUAGE",     SIMPLE, LEVEL_MAIN },
	{ "location",   "LOCATION",     SIMPLE, LEVEL_MAIN },
	{ "note",       "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annote",     "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "annotation", "NOTES",        SIMPLE, LEVEL_MAIN },
	{ "address",    "ADDRESS",      SIMPLE, LEVEL_MAIN },
	{ "refnum",     "REFNUM",       SIMPLE, LEVEL_MAIN },
	{ " ",          "INTERNAL_TYPE|THESIS",   ALWAYS, LEVEL_MAIN },
	{ " ",          "RESOURCE|text", ALWAYS, LEVEL_MAIN },
	{ " ",          "GENRE|Masters thesis",  ALWAYS, LEVEL_MAIN },
};

#define ORIG(a) ( &(a[0]) )
#define SIZE(a) ( sizeof( a ) / sizeof( lookups ) )
#define REFTYPE(a,b) { a, ORIG(b), SIZE(b) }

variants biblatex_all[] = {
	REFTYPE( "article", article ),
	REFTYPE( "booklet", booklet ),
	REFTYPE( "book", book ),
	REFTYPE( "collection", collection ),
	REFTYPE( "inbook", inbook ),
	REFTYPE( "incollection", incollection ),
	REFTYPE( "inproceedings", inproceedings ),
	REFTYPE( "conference", inproceedings ), /* legacy */
	REFTYPE( "manual", manual ),
	REFTYPE( "misc", misc ),
	REFTYPE( "online", online ),
	REFTYPE( "electronic", online ), /* legacy */
	REFTYPE( "www", online ),        /* jurabib compatibility */
	REFTYPE( "patent", patent ),
	REFTYPE( "periodical", periodical ),
	REFTYPE( "proceedings", proceedings ),
	REFTYPE( "report", report ),
	REFTYPE( "techreport", report ),
/*	REFTYPE( "set", set ), */
	REFTYPE( "thesis", thesis ),
	REFTYPE( "phdthesis", phdthesis ), /* legacy */
	REFTYPE( "mastersthesis", mastersthesis ), /* legacy */
	REFTYPE( "unpublished", unpublished ),
};

int biblatex_nall = sizeof( biblatex_all ) / sizeof( variants );
