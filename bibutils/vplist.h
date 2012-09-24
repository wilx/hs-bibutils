/*
 * vplist.h
 *
 * Copyright (c) Chris Putnam 2011-2012
 *
 * Source code released under the GPL
 *
 */

#ifndef VPLISTS_H
#define VPLISTS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct vplist {
	int n, max;
	void **data;
} vplist;

extern void    vplist_init( vplist *a );
extern int     vplist_add( vplist *a, void *value );
extern void    vplist_free( vplist *a );
extern void*   vplist_get( vplist *a, int n );
extern void    vplist_empty( vplist *a );


#endif
