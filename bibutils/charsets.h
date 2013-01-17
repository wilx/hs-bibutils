/*
 * charsets.h
 *
 * Copyright (c) Chris Putnam 2003-2013
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef CHARSETS_H
#define CHARSETS_H

#define CHARSET_UNKNOWN (-1)
#define CHARSET_UNICODE (-2)
#define CHARSET_GB18030 (-3)

typedef unsigned int charconvert;

typedef struct allcharconvert_t {
	char name[15];
	char name2[25];
	charconvert *table;
	int ntable;
} allcharconvert_t;
extern allcharconvert_t allcharconvert[];
extern int nallcharconvert;

#endif
