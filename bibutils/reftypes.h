/*
 * reftypes.h
 *
 * Copyright (c) Chris Putnam 2003-2013
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef REFTYPES_H
#define REFTYPES_H

/* Reftypes handled by core code */
#define ALWAYS              (0)
#define DEFAULT             (1)

/* Reftypes to be handled by converters */
#define SIMPLE              (2)
#define TYPE                (3)
#define PERSON              (4)
#define DATE                (5)
#define PAGES               (6)
#define SERIALNO            (7)
#define TITLE               (8)
#define NOTES               (9)
#define DOI                 (10)
#define HOWPUBLISHED        (11)
#define LINKEDFILE          (12)
#define KEYWORD             (13)
#define BIBTEX_URL          (14)
#define BIBTEX_NOTE         (15)
#define BIBTEX_SENTE        (16)
#define BIBTEX_GENRE        (17)
#define BIBTEX_EPRINT       (18)
#define BIBTEX_ORGANIZATION (19)
#define BLT_THESIS_TYPE     (20)
#define BLT_SCHOOL          (21)
#define BLT_EDITOR          (22)
#define BLT_SKIP            (23)

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
extern int translate_oldtag( char *oldtag, int reftype, variants all[], int nall, int *processingtype, int *level, char **newtag );



#endif
