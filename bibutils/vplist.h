/*
 * vplist.h
 *
 * Version: 9/20/2012
 *
 * Copyright (c) Chris Putnam 2011-2013
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef VPLIST_H
#define VPLIST_H

/* vplist = void pointer list (generic container struct)
 */
typedef struct vplist {
	int n, max;
	void **data;
} vplist;

extern void   vplist_init( vplist *vpl );
extern int    vplist_add( vplist *vpl, void *v );
extern void   vplist_empty( vplist *vpl );
extern int    vplist_copy( vplist *to, vplist *from );
extern int    vplist_append( vplist *to, vplist *from );
extern void * vplist_get( vplist *vpl, int n );
extern void   vplist_set( vplist *vpl, int n, void *v );
extern void   vplist_remove( vplist *vpl, int n );
extern void   vplist_removevp( vplist *vpl, void *v );
extern int    vplist_find( vplist *vpl, void *v );
/*
 * vplist_free frees the space for the data array of void * elements.
 *
 * if members require their own free calls, then, vplist_free()
 * should be wrapped in another deallocater:
 *
 * void foolist_free( vplist *vpl )
 * {
 *     int i;
 *     for ( i=0; i<vpl->n; ++i )
 *          if ( vpl->data[i] ) foo_free( (struct foo *) vpl->data[i] );
 *     vplist_free( vpl );
 * }
 *
 * vplist_free also does not deallocate any space corresponding
 * to vplist.
 */
extern void vplist_free( vplist *vpl );
/*
 * vplist_destroy does vplist_free and deallocates the struct
 * vplist * and replaces with NULL.
 *
 * void foolist_destroy( vplist **vpl )
 * {
 *     int i;
 *     for ( i=0; i<(*vpl)->n; ++i )
 *         if ( (*vpl)->data[i] ) foo_free( (struct foo *) (*vpl)->data[i] );
 *     vplist_destroy( vpl );
 * }
 */
extern void vplist_destroy( vplist **vpl );

#endif
