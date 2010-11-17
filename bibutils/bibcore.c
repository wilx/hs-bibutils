/*
 * bibcore.c
 *
 * Copyright (c) Chris Putnam 2005-2010
 *
 * Source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "bibutils.h"

/* internal includes */
#include "reftypes.h"
#include "newstr_conv.h"
#include "is_ws.h"

static void
bibl_duplicateparams( param *np, param *op )
{
	list_init( &(np->asis) );
	list_init( &(np->corps) );
	list_copy( &(np->asis), &(op->asis ) );
	list_copy( &(np->corps), &(op->corps ) );
	
	if ( !op->progname ) np->progname = NULL;
	else np->progname = strdup( op->progname );

	np->readformat = op->readformat;
	np->writeformat = op->writeformat;

	np->charsetin = op->charsetin;
	np->charsetin_src = op->charsetin_src;
	np->utf8in = op->utf8in;
	np->latexin = op->latexin;
	np->xmlin = op->xmlin;

	np->charsetout = op->charsetout;
	np->charsetout_src = op->charsetout_src;
	np->utf8out = op->utf8out;
	np->utf8bom = op->utf8bom;
	np->latexout = op->latexout;
	np->xmlout = op->xmlout;
	np->nosplittitle = op->nosplittitle;

	np->verbose = op->verbose;
	np->format_opts = op->format_opts;
	np->addcount = op->addcount;
	np->output_raw = op->output_raw;
	np->singlerefperfile = op->singlerefperfile;

	np->readf = op->readf;
	np->processf = op->processf;
	np->cleanf = op->cleanf;
	np->typef = op->typef;
	np->convertf = op->convertf;
	np->headerf = op->headerf;
	np->footerf = op->footerf;
	np->writef = op->writef;
	np->all = op->all;
	np->nall = op->nall;

}

static void
bibl_setreadparams( param *np, param *op )
{
	bibl_duplicateparams( np, op );
	np->utf8out = 1;
	np->charsetout = BIBL_CHARSET_UNICODE;
	np->charsetout_src = BIBL_SRC_DEFAULT;
	np->xmlout = 0;
	np->latexout = 0;
}

static void
bibl_setwriteparams( param *np, param *op )
{
	bibl_duplicateparams( np, op );
	np->xmlin = 0;
	np->latexin = 0;
	np->utf8in = 1;
	np->charsetin = BIBL_CHARSET_UNICODE;
	np->charsetin_src = BIBL_SRC_DEFAULT;
}

void
bibl_freeparams( param *p )
{
	list_free( &(p->asis) );
	list_free( &(p->corps) );
	if ( p->progname ) free( p->progname );
}

static void
bibl_readlist( list *pl, char *progname, char *filename )
{
	if ( !list_fill( pl, filename ) ) {
		fprintf( stderr, "%s: warning problems reading '%s' "
			"obtained %d elements\n", progname, filename,
			pl->n );
	}
}

void
bibl_readasis( param *p, char *filename )
{
	bibl_readlist( &(p->asis), p->progname, filename );
}

void
bibl_readcorps( param *p, char *filename )
{
	bibl_readlist( &(p->corps), p->progname, filename );
}

static void
bibl_addtolist( list *pl, char *entry )
{
	list_add( pl, entry );
}

void
bibl_addtoasis( param *p, char *entry )
{
	bibl_addtolist( &(p->asis), entry );
}

void
bibl_addtocorps( param *p, char *entry )
{
	bibl_addtolist( &(p->corps), entry );
}

void
bibl_reporterr( int err )
{
	fprintf( stderr, "Bibutils: " );
	switch( err ) {
		case BIBL_OK:
			fprintf( stderr, "No error." ); break;
		case BIBL_ERR_BADINPUT:
			fprintf( stderr, "Bad input." ); break;
		case BIBL_ERR_MEMERR:
			fprintf( stderr, "Memory error." ); break;
		case BIBL_ERR_CANTOPEN:
			fprintf( stderr, "Can't open." ); break;
		default:
			fprintf( stderr, "Cannot identify error code."); break;
	}
	fprintf( stderr, "\n" );
}

static int
bibl_illegalinmode( int mode )
{
	if ( mode < BIBL_FIRSTIN || mode > BIBL_LASTIN ) return 1;
	else return 0;
}

static int
bibl_illegaloutmode( int mode )
{
	if ( mode < BIBL_FIRSTOUT || mode > BIBL_LASTOUT ) return 1;
	else return 0;
}

void
bibl_verbose2( fields *info, char *filename, long nrefs )
{
	int i;
	fprintf( stderr, "======== %s %ld : converted\n", filename, nrefs );
	for ( i=0; i<info->nfields; ++i ) {
		fprintf( stderr, "'%s'='%s' level=%d\n", info->tag[i].data,
				info->data[i].data , info->level[i]);
	}
	fprintf( stderr, "\n" );
	fflush( stderr );
}

void
bibl_verbose1( fields *info, fields *orig, char *filename, long nrefs )
{
	int i;
	fprintf( stderr, "======== %s %ld : processed\n", filename, nrefs );
	for ( i=0; i<orig->nfields; ++i ) {
		fprintf( stderr, "'%s'='%s' level=%d\n", orig->tag[i].data,
				orig->data[i].data , orig->level[i]);
	}
	if ( info ) bibl_verbose2( info, filename, nrefs );
}

void
bibl_verbose0( bibl *bin )
{
	int i;
	for ( i=0; i<bin->nrefs; ++i )
		bibl_verbose2( bin->ref[i], "", i+1 );
}

static void
process_alwaysadd( fields *info, int reftype, param *r )
{
        char tag[512], data[512], *p, *q;
        int i, process, level;
        for ( i=0; i<r->all[reftype].ntags; ++i ) {
                process = ((r->all[reftype]).tags[i]).processingtype;
                if ( process!=ALWAYS ) continue;
                level   = ((r->all[reftype]).tags[i]).level;
                p = ((r->all[reftype]).tags[i]).newstr;
                q = tag;
                while ( p && *p && *p!='|' ) {
                        *q = *p;
                        q++;
                        p++;
                }
                *q = '\0';
                q = data;
                if ( p && *p=='|' ) p++;
                while ( p && *p ) {
                        *q = *p;
                        q++;
                        p++;
                }
                *q = '\0';
                fields_add( info, tag, data, level );
        }
}

static int
read_ref( FILE *fp, bibl *bin, char *filename, param *p )
{
	newstr reference, line;
	fields *ref;
	char buf[256]="";
	int nrefs = 0, bufpos = 0, fcharset;/* = CHARSET_UNKNOWN;*/
	newstr_init( &reference );
	newstr_init( &line );
	while ( p->readf( fp, buf, sizeof(buf), &bufpos, &line, &reference, &fcharset ) ) {
		if ( reference.len==0 ) continue;
		ref = fields_new();
		if ( !ref ) return BIBL_ERR_MEMERR;
		if ( p->processf( ref, reference.data, filename, nrefs+1 )){
			bibl_addref( bin, ref );
		} else {
			fields_free( ref );
			free( ref );
		}
		newstr_empty( &reference );
		if ( fcharset!=CHARSET_UNKNOWN ) {
			/* charset from file takes priority over default, but
			 * not user-specified */
			if ( p->charsetin_src!=BIBL_SRC_USER ) {
				p->charsetin_src = BIBL_SRC_FILE;
				p->charsetin = fcharset;
				if ( fcharset!=CHARSET_UNICODE ) p->utf8in = 0;
			}
		}
	}
	if ( p->charsetin==CHARSET_UNICODE ) p->utf8in = 1;
	newstr_free( &line );
	newstr_free( &reference );
	return BIBL_OK;
}

static void
bibl_fixcharsets( bibl *b, param *p )
{
	fields *ref;
	long i, j;
	int swap = 0;
	int latexout = p->latexout;
	int latexin  = p->latexin;
	for ( i=0; i<b->nrefs; ++i ) {
		ref = b->ref[i];
		for ( j=0; j<ref->nfields; ++j ) {
			if ( latexin || latexout ) {
				/* don't texify/detexify URL's and the like */
				if ( !strcasecmp( ref->tag[j].data, "DOI" ) ||
				     !strcasecmp( ref->tag[j].data, "URL" ) ||
				     !strcasecmp( ref->tag[j].data, "REFNUM" )){
					latexin  = 0;
					latexout = 0;
					swap = 1;
				}
			}
			newstr_convert( &(ref->data[j]),
				p->charsetin,  latexin,  p->utf8in,  
				p->xmlin,
				p->charsetout, latexout, p->utf8out, 
				p->xmlout );
			if ( swap ) {
				swap = 0;
				latexin  = p->latexin;
				latexout = p->latexout;
			}
		}
	}
}

static int
build_refnum( fields *info, long nrefs )
{
	newstr refnum;
	char *p, num[512];
	int y, a;
	newstr_init( &refnum );
	y = fields_find( info, "YEAR", 0 );
	if ( y==-1 ) y = fields_find( info, "YEAR", -1 );
	if ( y==-1 ) y = fields_find( info, "PARTYEAR", -1 );
	a = fields_find( info, "AUTHOR", 0 );
	if ( a==-1 ) a = fields_find( info, "AUTHOR", -1 );
	if ( a==-1 ) a = fields_find( info, "AUTHOR:CORP", -1 );
	if ( a==-1 ) a = fields_find( info, "AUTHOR:ASIS", -1 );
	if ( a!=-1 && y!=-1 ) {
		p = info->data[a].data;
		while ( p && *p && *p!='|' )
			newstr_addchar( &refnum, *p++ );
		p = info->data[y].data;
		while ( p && *p && *p!=' ' && *p!='\t' )
			newstr_addchar( &refnum, *p++ );
	} else {
		sprintf( num, "%ld", nrefs );
		newstr_strcpy( &refnum, "ref" );
		newstr_strcat( &refnum, num );
	}
	fields_add( info, "REFNUM", refnum.data, 0 );
	newstr_free( &refnum );
	return fields_find( info, "REFNUM", 0 );
}

static void
bibl_checkrefid( bibl *b, param *p )
{
	fields *ref;
	long i;
	char buf[512];
	int n;
	for ( i=0; i<b->nrefs; ++i ) {
		ref = b->ref[i];
		n = fields_find( ref, "REFNUM", 0 );
		if ( n==-1 ) n = build_refnum( ref, i+1 );
		if ( p->addcount ) {
			sprintf( buf, "_%ld", i+1 );
			newstr_strcat( &(ref->data[n]), buf );
		}
	}
}

static int
generate_citekey( fields *info, int nref )
{
	newstr citekey;
	int n1, n2;
	char *p, buf[100];
	newstr_init( &citekey );
	n1 = fields_find( info, "AUTHOR", 0 );
	if ( n1==-1 ) n1 = fields_find( info, "AUTHOR", -1 );
	n2 = fields_find( info, "YEAR", 0 );
	if ( n2==-1 ) n2 = fields_find( info, "YEAR", -1 );
	if ( n2==-1 ) n2 = fields_find( info, "PARTYEAR", 0 );
	if ( n2==-1 ) n2 = fields_find( info, "PARTYEAR", -1 );
	if ( n1!=-1 && n2!=-1 ) {
		p = info->data[n1].data;
		while ( p && *p && *p!='|' ) {
			if ( !is_ws( *p ) ) newstr_addchar( &citekey, *p ); 
			p++;
		}
		p = info->data[n2].data;
		while ( p && *p ) {
			if ( !is_ws( *p ) ) newstr_addchar( &citekey, *p );
			p++;
		}
		fields_add( info, "REFNUM", citekey.data, 0 );
	} else {
		sprintf( buf, "ref%d\n", nref );
		newstr_strcpy( &citekey, buf );
	}
	newstr_free( &citekey );
	return fields_find( info, "REFNUM", -1 );
}

static void
resolve_citekeys( bibl *b, list *citekeys, int *dup )
{
	char abc[]="abcdefghijklmnopqrstuvwxyz";
	newstr tmp;
	int nsame, ntmp, n, i, j;

	newstr_init( &tmp );

	for ( i=0; i<citekeys->n; ++i ) {
		if ( dup[i]==-1 ) continue;
		nsame = 0;
		for ( j=i; j<citekeys->n; ++j ) {
			if ( dup[j]!=i ) continue;
			newstr_newstrcpy( &tmp, &(citekeys->str[j]) );
			ntmp = nsame;
			while ( ntmp >= 26 ) {
				newstr_addchar( &tmp, 'a' );
					ntmp -= 26;
			}
			if ( ntmp<26 && ntmp>=0 )
			newstr_addchar( &tmp, abc[ntmp] );
			nsame++;
			dup[j] = -1;
			n = fields_find( b->ref[j], "REFNUM", -1 );
			if ( n!=-1 )
				newstr_newstrcpy(&((b->ref[j])->data[n]),&tmp);
		}
	}
	newstr_free( &tmp );
}

static void
get_citekeys( bibl *b, list *citekeys )
{
	fields *info;
	int i, n;
	for ( i=0; i<b->nrefs; ++i ) {
		info = b->ref[i];
		n = fields_find( info, "REFNUM", -1 );
		if ( n==-1 ) n = generate_citekey( info, i );
		if ( n!=-1 && info->data[n].data )
			list_add( citekeys, info->data[n].data );
		else
			list_add( citekeys, "" );
	}
}

static int 
dup_citekeys( bibl *b, list *citekeys )
{
	int i, j, *dup, ndup=0;
	dup = ( int * ) malloc( sizeof( int ) * citekeys->n );
	if ( !dup ) return 0;
	for ( i=0; i<citekeys->n; ++i ) dup[i] = -1;
	for ( i=0; i<citekeys->n-1; ++i ) {
		if ( dup[i]!=-1 ) continue;
		for ( j=i+1; j<citekeys->n; ++j ) {
			if ( !strcmp( citekeys->str[i].data, 
				citekeys->str[j].data ) ) {
					dup[i] = i;
					dup[j] = i;
					ndup++;
			}
		}
	}
	if ( ndup ) resolve_citekeys( b, citekeys, dup );
	free( dup );
	return ndup;
}

static void
uniqueify_citekeys( bibl *b )
{
	list citekeys;
	list_init( &citekeys );
	get_citekeys( b, &citekeys );
	dup_citekeys( b, &citekeys );
	list_free( &citekeys );
}

static int 
convert_ref( bibl *bin, char *fname, bibl *bout, param *p )
{
	fields *rin, *rout;
	long i;
	int reftype;
	if ( p->cleanf ) p->cleanf( bin, p );
	for ( i=0; i<bin->nrefs; ++i ) {
		rin = bin->ref[i];
		rout = fields_new();
		if ( !rout ) return BIBL_ERR_MEMERR;
		if ( p->typef ) 
			reftype = p->typef( rin, fname, i+1, p, p->all, p->nall );
		else reftype = 0;
		if ( p->all ) process_alwaysadd( rout, reftype, p );
		p->convertf( rin, rout, reftype, p, p->all, p->nall );
		if ( p->verbose ) 
			bibl_verbose1( rout, rin, fname, i+1 );
		bibl_addref( bout, rout );
	}
	uniqueify_citekeys( bout );
	return BIBL_OK;
}

int
bibl_read( bibl *b, FILE *fp, char *filename, param *p )
{
	param lp;
	bibl bin;

	if ( !b ) return BIBL_ERR_BADINPUT;
	if ( !fp ) return BIBL_ERR_BADINPUT;
	if ( !p ) return BIBL_ERR_BADINPUT;
	if ( bibl_illegalinmode( p->readformat ) ) return BIBL_ERR_BADINPUT;

	bibl_setreadparams( &lp, p );
	bibl_init( &bin );
	read_ref( fp, &bin, filename, &lp );
	if ( !lp.output_raw || ( lp.output_raw & BIBL_RAW_WITHCHARCONVERT ) )
		bibl_fixcharsets( &bin, &lp );
	if ( !lp.output_raw )
		convert_ref( &bin, filename, b, &lp );
	else {
		if ( p->verbose > 1 ) bibl_verbose0( &bin );
		bibl_copy( b, &bin );
	}
	if ( !lp.output_raw || ( lp.output_raw & BIBL_RAW_WITHMAKEREFID ) )
		bibl_checkrefid( b, &lp );
	bibl_free( &bin );

	return BIBL_OK;
}

static FILE *
singlerefname( fields *reffields, long nref, int mode )
{
	char outfile[2048];
	char suffix[5] = "xml";
	FILE *fp;
	long count;
	int  found;
	if ( mode==BIBL_BIBTEXOUT ) strcpy( suffix, "bib" );
	else if ( mode==BIBL_RISOUT ) strcpy( suffix, "ris" );
	else if ( mode==BIBL_ENDNOTEOUT ) strcpy( suffix, "end" );
	else if ( mode==BIBL_ADSABSOUT ) strcpy( suffix, "ads" );
	found = fields_find( reffields, "REFNUM", 0 );
	/* find new filename based on reference */
	if ( found!=-1 ) {
		sprintf( outfile,"%s.%s",reffields->data[found].data, suffix );
	} else  sprintf( outfile,"%ld.%s",nref, suffix );
	count = 0;
	fp = fopen( outfile, "r" );
	while ( fp ) {
		fclose(fp);
		count++;
		if ( count==60000 ) return NULL;
		if ( found!=-1 )
			sprintf( outfile, "%s_%ld.%s", 
				reffields->data[found].data, count, suffix  );
		else sprintf( outfile,"%ld_%ld.%s",nref, count, suffix );
		fp = fopen( outfile, "r" );
	}
	return fopen( outfile, "w" );
}

static int
output_bibl( FILE *fp, bibl *b, param *p )
{
	long i;
	if ( !p->singlerefperfile && p->headerf ) p->headerf( fp, p );
	for ( i=0; i<b->nrefs; ++i ) {
		if ( p->singlerefperfile ) { 
			fp = singlerefname( b->ref[i], i, p->writeformat );
			if ( fp ) {
				if ( p->headerf ) p->headerf( fp, p );
			} else return BIBL_ERR_CANTOPEN;
		}
		p->writef( b->ref[i], fp, p, i );
		if ( p->singlerefperfile ) {
			if ( p->footerf ) p->footerf( fp );
			fclose( fp );
		}
	}
	if ( !p->singlerefperfile && p->footerf ) p->footerf( fp );
	return 1;
}

int
bibl_write( bibl *b, FILE *fp, param *p )
{
	param lp;

	if ( !b ) return BIBL_ERR_BADINPUT;
	if ( !p ) return BIBL_ERR_BADINPUT;
	if ( bibl_illegaloutmode( p->writeformat ) ) return BIBL_ERR_BADINPUT;
	if ( !fp && ( !p || !p->singlerefperfile ) ) return BIBL_ERR_BADINPUT;

	bibl_setwriteparams( &lp, p );
	bibl_fixcharsets( b, &lp );
	output_bibl( fp, b, &lp );

	return BIBL_OK;
}

