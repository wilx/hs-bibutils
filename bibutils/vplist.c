/*
 * vplist.c
 *
 * Copyright (c) Chris Putnam 2011-2012
 *
 * Source code released under the GPL
 *
 * Implements a simple managed array of newstrs.
 *
 */
#include "vplist.h"

void *
vplist_get( vplist *a, int n )
{
	if ( n<0 || n>a->n ) return NULL;
	else return a->data[n];
}

static int
vplist_alloc( vplist *a )
{
	int alloc = 20;
	a->data = malloc( sizeof( void * ) * alloc );
	if ( !(a->data) ) return 0;
	a->max = alloc;
	a->n = 0;
	return 1;
}

static int
vplist_realloc( vplist *a )
{
	void **more;
	int alloc = a->max * 2;
	more = ( void** ) realloc( a->data, sizeof( void * ) * alloc );
	if ( !more ) return 0;
	a->data = more;
	a->max = alloc;
	return 1;
}

int
vplist_add( vplist *a, void *value )
{
	int ok = 1;

	/* ensure sufficient space */
	if ( a->max==0 ) ok = vplist_alloc( a );
	else if ( a->n >= a->max ) ok = vplist_realloc( a );

	if ( ok ) {
		a->data[a->n] = value;
		a->n++;
	}

	return ok;
}

void
vplist_empty( vplist *a )
{
	a->n = 0;
}

void
vplist_free( vplist *a )
{
	free( a->data );
	vplist_init( a );
}

void
vplist_init( vplist *a  )
{
	a->data = NULL;
	a->max = 0;
	a->n = 0;
}

