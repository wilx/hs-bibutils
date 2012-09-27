/*
 * biblatexin.c
 *
 * Copyright (c) Chris Putnam 2008-2012
 * Copyright (c) Johannes Wilm 2010-2012
 *
 * Program and source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "is_ws.h"
#include "strsearch.h"
#include "newstr.h"
#include "newstr_conv.h"
#include "fields.h"
#include "list.h"
#include "name.h"
#include "reftypes.h"
#include "biblatexin.h"

void
biblatexin_initparams( param *p, const char *progname )
{
	p->readformat       = BIBL_BIBLATEXIN;
	p->charsetin        = BIBL_CHARSET_DEFAULT;
	p->charsetin_src    = BIBL_SRC_DEFAULT;
	p->latexin          = 1;
	p->xmlin            = 0;
	p->utf8in           = 0;
	p->nosplittitle     = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->output_raw       = 0;

	p->readf    = biblatexin_readf;
	p->processf = biblatexin_processf;
	p->cleanf   = biblatexin_cleanf;
	p->typef    = biblatexin_typef;
	p->convertf = biblatexin_convertf;
	p->all      = biblatex_all;
	p->nall     = biblatex_nall;

	list_init( &(p->asis) );
	list_init( &(p->corps) );

	if ( !progname ) p->progname = NULL;
	else p->progname = strdup( progname );
}

/*
 * readf can "read too far", so we store this information in line, thus
 * the next new text is in line, either from having read too far or
 * from the next chunk obtained via newstr_fget()
 *
 * return 1 on success, 0 on error/end-of-file
 *
 */
static int
readmore( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line )
{
	if ( line->len ) return 1;
	else return newstr_fget( fp, buf, bufsize, bufpos, line );
}

/*
 * readf()
 *
 * returns zero if cannot get reference and hit end of-file
 * returns 1 if last reference in file, 2 if reference within file
 */
int
biblatexin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset )
{
	int haveref = 0;
	char *p;
	while ( haveref!=2 && readmore( fp, buf, bufsize, bufpos, line ) ) {
		if ( line->len == 0 ) continue; /* blank line */
		p = &(line->data[0]);
		p = skip_ws( p );
		if ( *p == '%' ) { /* commented out line */
			newstr_empty( line );
			continue;
		}
		if ( *p == '@' ) haveref++;
		if ( haveref && haveref<2 ) {
			newstr_strcat( reference, p );
			newstr_addchar( reference, '\n' );
			newstr_empty( line );
		} else if ( !haveref ) newstr_empty( line );
	
	}
	*fcharset = CHARSET_UNKNOWN;
	return haveref;
}

static char *
biblatex_item( char *p, newstr *s )
{
	int nquotes = 0;
	int nbrackets = 0;
	while ( *p ) {
		if ( !nquotes && !nbrackets ) {
			if ( *p==',' || *p=='=' || *p=='}' || *p==')' )
				goto out;
		}
		if ( *p=='\"' && *(p-1)!='\\' && nbrackets==0 ) {
			nquotes = ( nquotes==0 );
			newstr_addchar( s, *p );
		} else if ( *p=='{' ) {
			nbrackets++;
			/*if ( s->len!=0 )*/ newstr_addchar( s, *p );
		} else if ( *p=='}' ) {
			nbrackets--;
			/*if ( nbrackets>0 )*/ newstr_addchar( s, *p );
		} else {
			if ( s->len!=0 || ( s->len==0 && !is_ws( *p ) ) )
				newstr_addchar( s, *p );
		}
		p++;
	}
out:
	newstr_trimendingws( s );
	return p;
}

static char *
process_biblatexline( char *p, newstr *tag, newstr *data )
{
	p = skip_ws( p );
	p = biblatex_item( p, tag );
	p = skip_ws( p );
	if ( *p=='=' ) {
		p++;
		p = skip_ws( p );
		p = biblatex_item( p, data );
		p = skip_ws( p );
	}
	if ( *p==',' || *p=='}' || *p==')' ) p++;
	p = skip_ws( p );
	return p;
}

static void
biblatex_cleantoken( newstr *s, param *p )
{

	if ( p && p->latexin==0 ) return;

	/* 'textcomp' annotations */
	newstr_findreplace( s, "\\textit", "" );
	newstr_findreplace( s, "\\textbf", "" );
	newstr_findreplace( s, "\\textsl", "" );
	newstr_findreplace( s, "\\textsc", "" );
	newstr_findreplace( s, "\\textsf", "" );
	newstr_findreplace( s, "\\texttt", "" );
	newstr_findreplace( s, "\\textsubscript", "" );
	newstr_findreplace( s, "\\textsuperscript", "" );
	newstr_findreplace( s, "\\emph", "" );
	newstr_findreplace( s, "\\url", "" );

	/* Other text annotations */
	newstr_findreplace( s, "\\it ", "" );
	newstr_findreplace( s, "\\em ", "" );

	newstr_findreplace( s, "\\%", "%" );
	newstr_findreplace( s, "\\$", "$" );
	newstr_findreplace( s, "{", "" );
	newstr_findreplace( s, "}", "" );
	while ( newstr_findreplace( s, "  ", " " ) ) {}

	/* 'textcomp' annotations that we don't want to substitute on output*/
	newstr_findreplace( s, "\\textdollar", "$" );
	newstr_findreplace( s, "\\textunderscore", "_" );
}

static int
biblatex_protected( newstr *data )
{
	if ( data->data[0]=='{' && data->data[data->len-1]=='}' ) return 1;
	if ( data->data[0]=='\"' && data->data[data->len-1]=='\"' ) return 1;
	return 0;
}

static void
biblatex_removeprotection( newstr *data )
{
	newstr s;
	if ( data->len<3 ) newstr_empty( data ); /* "", {} to nothing */
	else {
		newstr_init( &s );
		newstr_segcpy( &s, &(data->data[1]), 
				&(data->data[data->len-1]) );
		newstr_newstrcpy( data, &s );
		newstr_free( &s );
	}
}

static void
biblatex_split( list *tokens, newstr *s )
{
	newstr currtok;
	int nquotes = 0, nbrackets = 0;
	int i, n = s->len;
	newstr_init( &currtok );
	for ( i=0; i<n; ++i ) {
		if ( s->data[i]=='\"' ) {
			if ( nquotes ) nquotes = 0;
			else nquotes = 1;
			newstr_addchar( &currtok, '\"' );
		} else if ( s->data[i]=='{' ) {
			nbrackets++;
			newstr_addchar( &currtok, '{' );
		} else if ( s->data[i]=='}' ) {
			nbrackets--;
			newstr_addchar( &currtok, '}' );
		} else if ( s->data[i]=='#' && !nquotes && !nbrackets ) {
			if ( currtok.len ) list_add( tokens, currtok.data );
			newstr_empty( &currtok );
		} else if ( !is_ws( s->data[i] ) || nquotes || nbrackets ) {
			newstr_addchar( &currtok, s->data[i] );
		}
	}
	if ( currtok.len ) list_add( tokens, currtok.data );
	for ( i=0; i<tokens->n; ++i ) {
		newstr_trimendingws( &(tokens->str[i]) );
	}
	newstr_free( &currtok );
}

/* get reference type */
static char*
process_biblatextype( char *p, newstr *data )
{
	newstr tmp;
	newstr_init( &tmp );

	newstr_empty( data );

	if ( *p=='@' ) p++; /* skip '@' character */
	while ( *p && *p!='{' && *p!='(' ) newstr_addchar( &tmp, *p++ );
	if ( *p=='{' || *p=='(' ) p++;
	p = skip_ws( p );

	if ( tmp.len ) {
		/* add '{' and '}' to protect from string expansion */
		newstr_addchar( data, '{' );
		newstr_strcat( data, tmp.data );
		newstr_addchar( data, '}' );
	}
	newstr_free( &tmp );
	return p;
}
/* get reference name */
static char*
process_biblatexid( char *p, newstr *data )
{
	newstr tmp;
	char *start_p = p;
	newstr_init( &tmp );
	newstr_empty( data );

	while ( *p && *p!=',' ) newstr_addchar( &tmp, *p++ );
	if ( *p==',' ) p++;
	p = skip_ws( p ); /* skip ending newline/carriage return */

	if ( tmp.len ) {
		if ( strchr( tmp.data, '=' ) ) {
			/* Endnote writes biblatex files w/o fields, try to
			 * distinguish via presence of an equal sign.... if
			 * it's there, assume that it's a tag/data pair instead
			 * and roll back.
			 */
			p = start_p;
		} else {
			/* add '{' and '}' to protect from string expansion */
			newstr_addchar( data, '{' );
			newstr_strcat( data, tmp.data );
			newstr_addchar( data, '}' );
		}
	}

	newstr_free( &tmp );
	return p;
}

static void
process_cite( fields *bibin, char *p, char *filename, long nref )
{
	newstr tag, data;
	newstrs_init( &tag, &data, NULL );
	p = process_biblatextype( p, &data );
	if ( data.len ) fields_add( bibin, "INTERNAL_TYPE", data.data, 0 );
	if ( *p ) p = process_biblatexid ( p, &data );
	if ( data.len ) fields_add( bibin, "REFNUM", data.data, 0 );
	newstr_empty( &data );
	while ( *p ) {
		p = process_biblatexline( p, &tag, &data );
		/* no anonymous or empty fields allowed */
		if ( tag.len && data.len )
			fields_add( bibin, tag.data, data.data, 0 );
		newstrs_empty( &tag, &data, NULL );
	}
	newstrs_free( &tag, &data, NULL );
}

int
biblatexin_processf( fields *bibin, char *data, char *filename, long nref )
{
	process_cite( bibin, data, filename, nref );
	return 1;
}

static void
biblatex_addtitleurl( fields *info, newstr *in )
{
	newstr s;
	char *p,*q;
	newstr_init( &s );
	q = p = in->data + 6; /*skip past \href{ */
	while ( *q && *q!='}' ) q++;
	newstr_segcpy( &s, p, q );
	fields_add( info, "URL", s.data, 0 );
	newstr_empty( &s );
	if ( *q=='}' ) q++;
	p = q;
	while ( *q ) q++;
	newstr_segcpy( &s, p, q );
	newstr_swapstrings( &s, in );
	newstr_free( &s );
}

static void
biblatex_cleandata( newstr *s, fields *info, param *p )
{
	list tokens;
	int i;
	if ( !s->len ) return;
	list_init( &tokens );
	biblatex_split( &tokens, s );
	for ( i=0; i<tokens.n; ++i ) {
		if (!strncasecmp(tokens.str[i].data,"\\href{", 6)) {
			biblatex_addtitleurl( info, &(tokens.str[i]) );
		}
		biblatex_cleantoken( &(tokens.str[i]), p );
	}
	newstr_empty( s );
	for ( i=0; i<tokens.n; ++i ) {
		if ( biblatex_protected( &(tokens.str[i]) ) )
			biblatex_removeprotection( &(tokens.str[i]));
		newstr_strcat( s, tokens.str[i].data ); 
	}
	list_free( &tokens );
}

static long
biblatexin_findref( bibl *bin, char *citekey )
{
	int n;
	long i;
	for ( i=0; i<bin->nrefs; ++i ) {
		n = fields_find( bin->ref[i], "refnum", -1 );
		if ( n==-1 ) continue;
		if ( !strcmp( bin->ref[i]->data[n].data, citekey ) ) return i;
	}
	return -1;
}

static void
biblatexin_nocrossref( bibl *bin, long i, int n, param *p )
{
	int n1 = fields_find( bin->ref[i], "REFNUM", -1 );
	if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
	fprintf( stderr, "Cannot find cross-reference '%s'", 
			bin->ref[i]->data[n].data);
	if ( n1!=-1 )
		fprintf( stderr, " for reference '%s'\n", 
				bin->ref[i]->data[n1].data );
	fprintf( stderr, "\n" );
}

static void
biblatexin_crossref( bibl *bin, param *p )
{
	char booktitle[] = "booktitle";
	long i, j, ncross;
	char *nt, *nd, *type;
	int n, ntype, nl;
        for ( i=0; i<bin->nrefs; ++i ) {
		n = fields_find( bin->ref[i], "CROSSREF", -1 );
		if ( n==-1 ) continue;
		ncross = biblatexin_findref( bin, bin->ref[i]->data[n].data );
		if ( ncross==-1 ) {
			biblatexin_nocrossref( bin, i, n, p );
#if 0
			int n1 = fields_find( bin->ref[i], "REFNUM", -1 );
			fprintf( stderr, "%s: Cannot find cross-reference '%s'",
				progname, bin->ref[i]->data[n].data);
			if ( n1!=-1 )
				fprintf( stderr, " for reference '%s'\n",
					bin->ref[i]->data[n1].data );
			fprintf( stderr, "\n" );
#endif			
			continue;
		}
		ntype = fields_find( bin->ref[i], "INTERNAL_TYPE", -1 );
		type = bin->ref[i]->data[ntype].data;
		fields_setused( bin->ref[i], n );
/*		bin->ref[i]->used[n] = 1; */
		for ( j=0; j<bin->ref[ncross]->n; ++j ) {
			nt = bin->ref[ncross]->tag[j].data;
			if ( !strcasecmp( nt, "INTERNAL_TYPE" ) ) continue;
			if ( !strcasecmp( nt, "REFNUM" ) ) continue;
			if ( !strcasecmp( nt, "TITLE" ) ) {
				if ( !strcasecmp( type, "Inproceedings" ) ||
				     !strcasecmp( type, "Incollection" ) )
					nt = booktitle;
			}
			nd = bin->ref[ncross]->data[j].data;
			nl = bin->ref[ncross]->level[j] + 1;
			fields_add( bin->ref[i], nt, nd, nl );

		}
	}
}

static void
biblatexin_cleanref( fields *bibin, param *p )
{
	newstr *t, *d;
	int i, n;
	n = fields_num( bibin );
	for ( i=0; i<n; ++i ) {
		t = fields_tag( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		biblatex_cleandata( d, bibin, p );
		if ( !strsearch( t->data, "AUTHORS" ) ) {
			newstr_findreplace( d, "\n", " " );
			newstr_findreplace( d, "\r", " " );
		}
		else if ( !strsearch( t->data, "ABSTRACT" ) ||
		     !strsearch( t->data, "SUMMARY" ) || 
		     !strsearch( t->data, "NOTE" ) ) {
			newstr_findreplace( d, "\n", "" );
			newstr_findreplace( d, "\r", "" );
		}
	}
}

void
biblatexin_cleanf( bibl *bin, param *p )
{
	long i;
        for ( i=0; i<bin->nrefs; ++i )
		biblatexin_cleanref( bin->ref[i], p );
	biblatexin_crossref( bin, p );
}

/*
 * process_names( info, newtag, field, level);
 *
 * split names in author list separated by and's (use '|' character)
 * and add names
 */
static void
process_names( fields *info, char *tag, newstr *data, int level, list *asis,
	list *corps )
{
	newstr_findreplace( data, " and ", "|" );
	name_add( info, tag, data->data, level, asis, corps );
}

static void
process_pages( fields *info, newstr *s, int level )
{
	char *p, *q;
	newstr sp, ep;

	newstr_init( &sp );
	newstr_init( &ep );

	newstr_findreplace( s, " ", "" );

	p = q = s->data;
	while ( isdigit( *q ) ) q++;
	newstr_segcpy( &sp, p, q );
	if ( sp.len>0 )
		fields_add( info, "PAGESTART", sp.data, level );

	p = q;
	while ( *p && !isdigit(*p) ) p++;
	q = p;
	while ( isdigit( *q ) ) q++;
	newstr_segcpy( &ep, p, q );
	if ( ep.len>0 )
		fields_add( info, "PAGEEND", ep.data, level );

	newstr_free(&sp);
	newstr_free(&ep);
}

static void
process_urlcore( fields *info, char *p, int level, char *default_tag )
{
	if ( !strncasecmp( p, "\\urllink", 8 ) )
		fields_add( info, "URL", p+8, level );
	else if ( !strncasecmp( p, "\\url", 4 ) )
		fields_add( info, "URL", p+4, level );
	else if ( !strncasecmp( p, "arXiv:", 6 ) )
		fields_add( info, "ARXIV", p+6, level ); 
	else if ( !strncasecmp( p, "http://arxiv.org/abs/", 21 ) )
		fields_add( info, "ARXIV", p+21, level );
	else if ( !strncasecmp( p, "http:", 5 ) )
		fields_add( info, "URL", p, level );
	else fields_add( info, default_tag, p, level );
}

static void
process_url( fields *info, char *p, int level )
{
	process_urlcore( info, p, level, "URL" );
}

static void
process_howpublished( fields *info, char *p, int level )
{
        /* Some users put Diploma thesis in "type" */
        if ( !strncasecmp( p, "Diplom", 6 ) )
                fields_replace_or_add( info, "GENRE", "Diploma thesis", level );
        else if ( !strncasecmp( p, "Habilitation", 13 ) )
                fields_replace_or_add( info, "GENRE", "Habilitation thesis", level );
        else 
		process_urlcore( info, p, level, "DESCRIPTION" );
}

static void
process_genre( fields *info, char *p, int level )
{
	/* Some users put Diploma thesis in "type" */
	if ( !strncasecmp( p, "Diplom", 6 ) )
		fields_replace_or_add( info, "GENRE", "Diploma thesis", level );
	else if ( !strncasecmp( p, "Habilitation", 13 ) )
		fields_replace_or_add( info, "GENRE", "Habilitation thesis", level );
	else
		fields_add( info, "GENRE", p, level );
}

static void
process_eprint( fields *bibin, fields *info, int level )
{
	int neprint, netype;
	char *eprint = NULL, *etype = NULL;
	neprint = fields_find( bibin, "eprint", -1 );
	netype  = fields_find( bibin, "eprinttype", -1 );
	if ( neprint!=-1 ) eprint = bibin->data[neprint].data;
	if ( netype!=-1 ) etype = bibin->data[netype].data;
	if ( eprint && etype ) {
		if ( !strncasecmp( etype, "arxiv", 5 ) )
			fields_add( info, "ARXIV", eprint, level );
		else if ( !strncasecmp( etype, "jstor", 5 ) )
			fields_add( info, "JSTOR", eprint, level );
		else if ( !strncasecmp( etype, "pubmed", 6 ) )
			fields_add( info, "PMID", eprint, level );
		else if ( !strncasecmp( etype, "medline", 7 ) )
			fields_add( info, "MEDLINE", eprint, level );
		else {
			fields_add( info, "EPRINT", eprint, level );
			fields_add( info, "EPRINTTYPE", etype, level );
		}
		fields_setused( bibin, neprint );
		fields_setused( bibin, netype );
	} else if ( eprint ) {
		fields_add( info, "EPRINT", eprint, level );
		fields_setused( bibin, neprint );
	} else if ( etype ) {
		fields_add( info, "EPRINTTYPE", etype, level );
		fields_setused( bibin, netype );
	}
}

int
biblatexin_typef( fields *bibin, char *filename, int nrefs, param *p,
		variants *all, int nall )
{
	char *refnum = "";
	int reftype, n, nrefnum;
	n = fields_find( bibin, "INTERNAL_TYPE", 0 );
	nrefnum = fields_find( bibin, "REFNUM", 0 );
	if ( nrefnum!=-1 ) refnum = (bibin->data[nrefnum]).data;
	if ( n!=-1 )
		/* figure out type */
		reftype = get_reftype( (bibin->data[n]).data, nrefs,
			p->progname, all, nall, refnum );
	else
		/* no type info, go for default */
		reftype = get_reftype( "", nrefs, p->progname, all, nall, refnum );
	return reftype;
}

static void
report( fields *f )
{
	int i, n;
	n = fields_num( f );
	for ( i=0; i<n; ++i )
		fprintf(stderr, "'%s' %d = '%s'\n",
			(char*)fields_tag( f, i, FIELDS_CHRP_NOUSE ),
			fields_level( f, i ),
			(char*)fields_value( f, i, FIELDS_CHRP_NOUSE ) );
}

static void
biblatexin_notag( param *p, char *tag )
{
	if ( p->verbose && strcmp( tag, "INTERNAL_TYPE" ) ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, " Cannot find tag '%s'\n", tag );
	}
}

void
biblatexin_convertf( fields *bibin, fields *info, int reftype, param *p,
		variants *all, int nall )
{
	newstr *t, *d;
	int process, level, i, n, nfields;
	char *newtag;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

               /* skip ones already "used" such as successful crossref */
                if ( bibin->used[i] ) continue;

		/* skip ones with no data */
		d = &( bibin->data[i] );
		if ( d->len == 0 ) continue;

		t = &( bibin->tag[i] );
		n = process_findoldtag( t->data, reftype, all, nall );
		if ( n==-1 ) {
			biblatexin_notag( p, t->data );
			continue;
		}

		process = ((all[reftype]).tags[n]).processingtype;
		if ( process == ALWAYS ) continue; /* added before */

		level   = ((all[reftype]).tags[n]).level;
		newtag  = ((all[reftype]).tags[n]).newstr;

		if ( process==SIMPLE || process==TITLE )
			fields_add( info, newtag, d->data, level );
		else if ( process==PERSON )
			process_names( info, newtag, d, level, &(p->asis), 
				&(p->corps) );
		else if ( process==PAGES )
			process_pages( info, d, level);
		else if ( process==HOWPUBLISHED )
			process_howpublished( info, d->data, level );
		else if ( process==BIBTEX_URL )
			process_url( info, d->data, level );
		else if ( process==BIBTEX_GENRE )
			process_genre( info, d->data, level );
		else if ( process==BIBTEX_EPRINT )
			process_eprint( bibin, info, level );
	}
	if ( p->verbose ) report( info );
}

