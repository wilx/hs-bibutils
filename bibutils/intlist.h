/*
 * intlist.h
 *
 * Copyright (c) Chris Putnam 2007-2013
 *
 * Source code released under the GPL version 2
 *
 */

#ifndef INTLIST_H
#define INTLIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct intlist {
	int n, max;
	int *data;
} intlist;

extern void intlist_init( intlist *info );
extern void intlist_init_range( intlist *info, int low, int high, int step );
extern intlist* intlist_new( void );
extern intlist* intlist_new_range( int low, int high, int step );
extern void intlist_sort( intlist *info );
extern void intlist_randomize( intlist *info );
extern int  intlist_add(  intlist *info, int value );
extern int  intlist_add_unique(  intlist *info, int value );
extern int  intlist_find( intlist *info, int searchvalue );
extern int  intlist_find_or_add( intlist *info, int searchvalue );
extern void intlist_empty( intlist *info );
extern void intlist_free( intlist *info );
extern void intlist_copy( intlist *to, intlist *from );
extern int  intlist_get( intlist *info, int pos );
extern void intlist_remove( intlist *info, int searchvalue );
extern void intlist_remove_pos( intlist *info, int pos );
extern void intlist_append( intlist *to, intlist *from );
extern void intlist_append_unique( intlist *to, intlist *from );

#endif
