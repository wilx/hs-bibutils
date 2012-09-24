/*
 * bibl.c
 *
 * Copyright (c) Chris Putnam 2005-2012
 *
 * Source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "bibl.h"

void
bibl_init( bibl *b )
{
	b->nrefs = b->maxrefs = 0L;
	b->ref = NULL;
}

static void
bibl_malloc( bibl * b )
{
	int alloc = 50;
	b->nrefs = 0;
	b->ref = ( fields ** ) malloc( sizeof( fields* ) * alloc );
	if ( b->ref ) {
		b->maxrefs = alloc;
	} else {
		fprintf( stderr, "bibl_malloc: allocation error\n" );
		exit( EXIT_FAILURE );
	}
}

static void
bibl_realloc( bibl * b )
{
	int alloc = b->maxrefs * 2;
	fields **more;
	more = ( fields ** ) realloc( b->ref, sizeof( fields* ) * alloc );
	if ( more ) {
		b->ref = more;
		b->maxrefs = alloc;
	} else {
		fprintf( stderr, "bibl_realloc: allocation error\n" );
		exit( EXIT_FAILURE );
	}
}

void
bibl_addref( bibl *b, fields *ref )
{
	if ( b->maxrefs==0 ) bibl_malloc( b );
	else if ( b->nrefs >= b->maxrefs ) bibl_realloc( b );
	b->ref[ b->nrefs ] = ref;
	b->nrefs++;
}

void
bibl_free( bibl *b )
{
	long i;
	for ( i=0; i<b->nrefs; ++i )
		fields_free( b->ref[i] );
	free( b->ref );
	b->ref = NULL;
	b->nrefs = b->maxrefs = 0;
}

void
bibl_copy( bibl *bout, bibl *bin )
{
	fields *refin, *refout;
	char *tag, *value;
	int level;
	int i, j, n;
	for ( i=0; i<bin->nrefs; ++i ) {
		refin = bin->ref[i];
		refout = fields_new();
		n = fields_num( refin );
		for ( j=0; j<n; ++j ) {
			tag = fields_tag( refin, j, FIELDS_CHRP );
			value = fields_value( refin, j, FIELDS_CHRP );
			level = fields_level( refin, j );
			if ( tag && value )
				fields_add( refout, tag, value, level );
		}
		bibl_addref( bout, refout );
	}
}

