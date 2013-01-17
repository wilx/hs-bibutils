/*
 * intlist.c
 *
 * Copyright (c) Chris Putnam 2007-2013
 *
 * Source code released under the GPL version 2
 *
 * Implements a simple managed array of ints
 *
 */
#include "intlist.h"

static void
memerr( const char *f )
{
	fprintf( stderr, "Memory error in %s\n", f );
	exit( EXIT_FAILURE );
}

static void
intlist_alloc( intlist *il )
{
	int alloc = 20, i;
	il->data = ( int * ) malloc( sizeof( int ) * alloc );
	if ( !(il->data) ) memerr( __FUNCTION__ );
	for ( i=0; i<alloc; ++i )
		il->data[i] = 0;
	il->max = alloc;
	il->n = 0;
}

static void
intlist_realloc( intlist *il )
{
	int *more;
	int alloc = il->max * 2, i;
	more = ( int * ) realloc( il->data, sizeof( int ) * alloc );
	if ( !more ) memerr( __FUNCTION__ );
	il->data = more;
	for ( i=il->max; i<alloc; ++i )
		il->data[i] = 0;
	il->max = alloc;
}

int
intlist_add( intlist *il, int value )
{
	if ( il->max==0 ) intlist_alloc( il );
	else if ( il->n >= il->max ) intlist_realloc( il );
	il->data[ il->n ] = value;
	il->n++;
	return il->n - 1;
}

int
intlist_add_unique( intlist *il, int value )
{
	if ( intlist_find( il, value )!=-1 ) return il->n - 1;
	else return intlist_add( il, value );
}

int
intlist_find( intlist *il, int value )
{
	int i;
	for ( i=0; i<il->n; ++i )
		if ( il->data[i]==value ) return i;
	return -1;
}

/* Here we know that pos is valid */
static void
intlist_remove_pos_core( intlist *il, int pos )
{
	int i;
	for ( i=pos; i<il->n-1; ++i )
		il->data[i] = il->data[i+1];
	il->n -= 1;
}

void
intlist_remove_pos( intlist *il, int pos )
{
	if ( pos < 0 || pos >= il->n ) return;
	intlist_remove_pos_core( il, pos );
}

void
intlist_remove( intlist *il, int value )
{
	int pos = intlist_find( il, value );
	if ( pos==-1 ) return;
	intlist_remove_pos_core( il, pos );
}

/* Return the index of searched-for string.
 * If cannot find string, add to list and then
 * return the index
 */
int
intlist_find_or_add( intlist *il, int value )
{
	int n = intlist_find( il, value );
	if ( n==-1 )
		n = intlist_add( il, value );
	return n;
}

/* don't actually free space, just reset counter */
void
intlist_empty( intlist *il )
{
	il->n = 0;
}

void
intlist_free( intlist *il )
{
	if ( il->data ) free( il->data );
	intlist_init( il );
}

void
intlist_init( intlist *il  )
{
	il->data = NULL;
	il->max = 0;
	il->n = 0;
}

void
intlist_init_range( intlist *il, int low, int high, int step )
{
	int i;
	intlist_init( il );
	for ( i=low; i<high; i+= step )
		intlist_add( il, i );
}

intlist *
intlist_new( void )
{
	intlist *il = ( intlist * ) malloc( sizeof( intlist ) );
	if ( il ) intlist_init( il );
	return il;
}

intlist *
intlist_new_range( int low, int high, int step )
{
	intlist *il = ( intlist * ) malloc( sizeof( intlist ) );
	if ( il ) intlist_init_range( il, low, high, step );
	return il;
}

static int
intcomp( const void *v1, const void *v2 )
{
	int *i1 = ( int * ) v1;
	int *i2 = ( int * ) v2;
	if ( *i1 < *i2 ) return -1;
	else if ( *i1 > *i2 ) return 1;
	return 0;
}

void
intlist_sort( intlist *il )
{
	qsort( il->data, il->n, sizeof( int ), intcomp );
}

/* Returns random integer in the range [floor,ceil) */
static int
randomint( int floor, int ceil )
{
	int len = ceil - floor;
	return floor + rand() % len;
}

static void
swap( int *a, int *b )
{
	int tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

void
intlist_randomize( intlist *il )
{
	int i, j;
	if ( il->n < 2 ) return;
	for ( i=0; i<il->n; ++i ) {
		j = randomint( i, il->n );
		if ( i==j ) continue;
		swap( &(il->data[i]), &(il->data[j]) );
	}
}

void
intlist_copy( intlist *to, intlist *from )
{
	int i;
	intlist_free( to );
	to->data = ( int* ) malloc( sizeof( int ) * from->n );
	if ( !to->data ) return;
	to->n = to->max = from->n;
	for ( i=0; i<from->n; ++i ) 
		to->data[i] = from->data[i];
}

intlist *
intlist_dup( intlist *il )
{
	intlist *l;
	l = ( intlist* ) malloc( sizeof( intlist ) );
	if ( !l ) goto err0;
	intlist_copy( l, il );
	return l;
	free( l );
err0:
	return l;
}

void
intlist_append( intlist *to, intlist *from )
{
	int i;
	for ( i=0; i<from->n; ++i )
		intlist_add( to, from->data[i] );
}

void
intlist_append_unique( intlist *to, intlist *from )
{
	int i;
	for ( i=0; i<from->n; ++i ) {
		if ( intlist_find( to, from->data[i] )!=-1 ) continue;
		intlist_add( to, from->data[i] );
	}
}

int
intlist_get( intlist *il, int pos )
{
	if ( pos<0 || pos>=il->n ) return 0;
	else return il->data[pos];
}

