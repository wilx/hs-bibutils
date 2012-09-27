/*
 * reftypes.h
 *
 * Copyright (c) Chris Putnam 2003-2012
 *
 * Source code released under the GPL
 *
 */
#ifndef REFTYPES_H
#define REFTYPES_H

#define SIMPLE              (0)
#define TYPE                (1)
#define PERSON              (2)
#define DATE                (3)
#define PAGES               (4)
#define SERIALNO            (5)
#define TITLE               (6)
#define ALWAYS              (7)
#define NOTES               (8)
#define DOI                 (9)
#define HOWPUBLISHED        (10)
#define LINKEDFILE          (11)
#define KEYWORD             (12)
#define BIBTEX_URL          (13)
#define BIBTEX_SENTE        (14)
#define BIBTEX_GENRE        (15)
#define BIBTEX_EPRINT       (16)
#define BIBTEX_ORGANIZATION (17)

typedef struct {
	char *oldstr;
	char *newstr;
	int  processingtype;
	int  level;
} lookups;

typedef struct {
	char    type[25];
	lookups *tags;
	int     ntags;
} variants;

extern int get_reftype( char *q, long refnum, char *progname, variants *all, int nall, char *tag );
extern int process_findoldtag( char *oldtag, int reftype, variants all[], int nall );


#endif
