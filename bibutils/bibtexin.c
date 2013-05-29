/*
 * bibtexin.c
 *
 * Copyright (c) Chris Putnam 2003-2013
 *
 * Program and source code released under the GPL version 2
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
#include "title.h"
#include "reftypes.h"
#include "bibtexin.h"

static list find    = { 0, 0, 0, NULL };
static list replace = { 0, 0, 0, NULL };

void
bibtexin_initparams( param *p, const char *progname )
{
	p->readformat       = BIBL_BIBTEXIN;
	p->charsetin        = BIBL_CHARSET_DEFAULT;
	p->charsetin_src    = BIBL_SRC_DEFAULT;
	p->latexin          = 1;
	p->xmlin            = 0;
	p->utf8in           = 0;
	p->nosplittitle     = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->output_raw       = 0;

	p->readf    = bibtexin_readf;
	p->processf = bibtexin_processf;
	p->cleanf   = bibtexin_cleanf;
	p->typef    = bibtexin_typef;
	p->convertf = bibtexin_convertf;
	p->all      = bibtex_all;
	p->nall     = bibtex_nall;

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
bibtexin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset )
{
	int haveref = 0;
	char *p;
	*fcharset = CHARSET_UNKNOWN;
	while ( haveref!=2 && readmore( fp, buf, bufsize, bufpos, line ) ) {
		if ( line->len == 0 ) continue; /* blank line */
		p = &(line->data[0]);
		/* Recognize UTF8 BOM */
		if ( line->len > 2 && 
				(unsigned char)(p[0])==0xEF &&
				(unsigned char)(p[1])==0xBB &&
				(unsigned char)(p[2])==0xBF ) {
			*fcharset = CHARSET_UNICODE;
			p += 3;
		}
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
	return haveref;
}

static char *
bibtex_item( char *p, newstr *s )
{
	int surrounding_quotes = 0;
	int nbrackets = 0;
	int nquotes = 0;
	while ( *p ) {
		if ( !nquotes && !nbrackets ) {
			if ( *p==',' || *p=='=' || *p=='}' || *p==')' )
				goto out;
		}
		if ( *p=='\"' && *(p-1)!='\\' && nbrackets==0 ) {
			if ( s->len == 0 ) surrounding_quotes = 1;
			else {
				if ( !surrounding_quotes ) newstr_addchar( s, *p );
				if ( nquotes ) surrounding_quotes = 0;
			}
			nquotes = ( nquotes==0 );
		} else if ( *p=='{' ) {
			if ( nbrackets>0 || nquotes ) newstr_addchar( s, *p );
			nbrackets++;
		} else if ( *p=='}' ) {
			nbrackets--;
			if ( nbrackets>0 || nquotes ) newstr_addchar( s, *p );
		} else {
			if ( !is_ws( *p ) ) newstr_addchar( s, *p );
			else {
				if ( s->len!=0 && *p!='\n' && *p!='\r' )
					newstr_addchar( s, *p );
				else if ( s->len!=0 && (*p=='\n' || *p=='\r')) {
					newstr_addchar( s, ' ' );
					while ( is_ws( *(p+1) ) ) p++;
				}
			}
		}
		p++;
	}
out:
	newstr_trimendingws( s );
	return p;
}

static char *
process_bibtexline( char *p, newstr *tag, newstr *data )
{
	p = skip_ws( p );
	p = bibtex_item( p, tag );
	p = skip_ws( p );
	if ( *p=='=' ) {
		p++;
		p = skip_ws( p );
		p = bibtex_item( p, data );
		p = skip_ws( p );
	}
	if ( *p==',' || *p=='}' || *p==')' ) p++;
	p = skip_ws( p );
	return p;
}

static void
bibtex_process_tilde( newstr *s )
{
	char *p, *q;
	int n = 0;

	p = q = s->data;
	while ( *p ) {
		if ( *p=='~' ) {
			*q = ' ';
		} else if ( *p=='\\' && *(p+1)=='~' ) {
			n++;
			p++;
			*q = '~';
		} else {
			*q = *p;
		}
		p++;
		q++;
	}
	*q = '\0';
	s->len -= n;
}

static void
bibtex_cleantoken( newstr *s )
{
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
	newstr_findreplace( s, "\\mbox", "" );

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

	bibtex_process_tilde( s );

}

static void
bibtex_addstring( char *p )
{
	newstr s1, s2;
	newstrs_init( &s1, &s2, NULL );
	p = skip_ws( p );
	if ( *p=='(' || *p=='{' ) p++;
	p = process_bibtexline( p, &s1, &s2 );
	newstr_findreplace( &s2, "\\ ", " " );
	bibtex_cleantoken( &s2 );
	if ( s1.data ) {
		list_add( &find, s1.data );
		if ( s2.data ) list_add( &replace, s2.data );
		else list_add( &replace, "" );
	}
	newstrs_free( &s1, &s2, NULL );
}

static int
bibtex_protected( newstr *data )
{
	if ( data->data[0]=='{' && data->data[data->len-1]=='}' ) return 1;
	if ( data->data[0]=='\"' && data->data[data->len-1]=='\"' ) return 1;
	return 0;
}

static void
bibtex_split( list *tokens, newstr *s )
{
	int i, n = s->len, nbrackets = 0;
	newstr currtok;

	newstr_init( &currtok );

	for ( i=0; i<n; ++i ) {
		if ( s->data[i]=='{' ) {
			nbrackets++;
			newstr_addchar( &currtok, '{' );
		} else if ( s->data[i]=='}' ) {
			nbrackets--;
			newstr_addchar( &currtok, '}' );
		} else if ( s->data[i]=='#' && !nbrackets ) {
			if ( currtok.len ) list_add( tokens, currtok.data );
			newstr_empty( &currtok );
		} else if ( !is_ws( s->data[i] ) || nbrackets ) {
			newstr_addchar( &currtok, s->data[i] );
		} else if ( is_ws( s->data[i] ) ) {
			if ( currtok.len ) list_add( tokens, currtok.data );
			newstr_empty( &currtok );
		}
	}
	if ( currtok.len ) list_add( tokens, currtok.data );
	for ( i=0; i<tokens->n; ++i ) {
		newstr_trimstartingws( list_get( tokens, i ) );
		newstr_trimendingws( list_get( tokens, i ) );
	}
	newstr_free( &currtok );
}

static int
bibtex_usestrings( newstr *s )
{
	char *p;
	int i;
	for ( i=0; i<find.n; ++i ) {
		p = list_getc( &find, i );
		if ( !strcasecmp( s->data, p ) ) {
			newstr_findreplace( s, p, list_getc( &replace, i ) );
			return 1;
		}
	}
	return 0;
}

/* get reference type */
static char*
process_bibtextype( char *p, newstr *data )
{
	newstr tmp;
	newstr_init( &tmp );

	newstr_empty( data );

	if ( *p=='@' ) p++; /* skip '@' character */
	while ( *p && *p!='{' && *p!='(' && !is_ws( *p ) ) newstr_addchar( &tmp, *p++ );
	p = skip_ws( p );
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
process_bibtexid( char *p, newstr *data )
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
			/* Endnote writes bibtex files w/o fields, try to
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
	p = process_bibtextype( p, &data );
	if ( data.len ) fields_add( bibin, "INTERNAL_TYPE", data.data, 0 );
	if ( *p ) p = process_bibtexid ( p, &data );
	if ( data.len ) fields_add( bibin, "REFNUM", data.data, 0 );
	newstr_empty( &data );
	while ( *p ) {
		p = process_bibtexline( p, &tag, &data );
		/* no anonymous or empty fields allowed */
		if ( tag.len && data.len )
			fields_add( bibin, tag.data, data.data, 0 );
		newstrs_empty( &tag, &data, NULL );
	}
	newstrs_free( &tag, &data, NULL );
}

static void
process_string( char *p )
{
	while ( *p && *p!='{' && *p!='(' ) p++;
	bibtex_addstring( p );
}

int
bibtexin_processf( fields *bibin, char *data, char *filename, long nref )
{
	if ( !strncasecmp( data, "@STRING", 7 ) ) {
		process_string( data );
		return 0;
	} else if ( !strncasecmp( data, "@COMMENT", 8 ) ) {
		/* Not sure if these are real Bibtex, but not references */
		return 0;
	} else {
		process_cite( bibin, data, filename, nref );
		return 1;
	}
}

static void
bibtex_addtitleurl( fields *info, newstr *in )
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

static int
is_url_tag( newstr *tag )
{
	if ( tag->len ) {
		if ( !strcasecmp( tag->data, "url" ) ) return 1;
	}
	return 0;
}

static int
is_name_tag( newstr *tag )
{
	if ( tag->len ) {
		if ( !strcasecmp( tag->data, "author" ) ) return 1;
		if ( !strcasecmp( tag->data, "editor" ) ) return 1;
	}
	return 0;
}

static void
bibtex_cleandata( newstr *tag, newstr *s, fields *info, param *p )
{
	list tokens;
	newstr *tok;
	int i;
	if ( !s->len ) return;
	list_init( &tokens );
	bibtex_split( &tokens, s );
	for ( i=0; i<tokens.n; ++i ) {
		tok = list_get( &tokens, i );
		if ( !bibtex_protected( tok ) ) {
			bibtex_usestrings( tok );
		} else {
			if (!strncasecmp(tok->data,"\\href{", 6)) {
				bibtex_addtitleurl( info, tok );
			}
		}
		if ( p->latexin && !is_name_tag( tag ) && !is_url_tag( tag ) ) bibtex_cleantoken( tok );
	}
	newstr_empty( s );
	for ( i=0; i<tokens.n; ++i ) {
		tok = list_get( &tokens, i );
		if ( i>0 ) newstr_addchar( s, ' ' );
		newstr_newstrcat( s, tok );
	}
	list_free( &tokens );
}

static long
bibtexin_findref( bibl *bin, char *citekey )
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
bibtexin_nocrossref( bibl *bin, long i, int n, param *p )
{
	int n1 = fields_find( bin->ref[i], "REFNUM", -1 );
	if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
	fprintf( stderr, "Cannot find cross-reference '%s'",
			bin->ref[i]->data[n].data );
	if ( n1!=-1 ) fprintf( stderr, " for reference '%s'\n",
			bin->ref[i]->data[n1].data );
	fprintf( stderr, "\n" );
}

static void
bibtexin_crossref( bibl *bin, param *p )
{
	char booktitle[] = "booktitle";
	long i, j, ncross;
	char *nt, *nd, *type;
	int n, ntype, nl;
        for ( i=0; i<bin->nrefs; ++i ) {
		n = fields_find( bin->ref[i], "CROSSREF", -1 );
		if ( n==-1 ) continue;
		ncross = bibtexin_findref( bin, bin->ref[i]->data[n].data );
		if ( ncross==-1 ) {
			bibtexin_nocrossref( bin, i, n, p );
			continue;
		}
		ntype = fields_find( bin->ref[i], "INTERNAL_TYPE", -1 );
		type = bin->ref[i]->data[ntype].data;
		fields_setused( bin->ref[i], n );
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
bibtexin_cleanref( fields *bibin, param *p )
{
	newstr *t, *d;
	int i, n;
	n = fields_num( bibin );
	for ( i=0; i<n; ++i ) {
		t = fields_tag( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		bibtex_cleandata( t, d, bibin, p );
	}
}

void
bibtexin_cleanf( bibl *bin, param *p )
{
	long i;
        for ( i=0; i<bin->nrefs; ++i )
		bibtexin_cleanref( bin->ref[i], p );
	bibtexin_crossref( bin, p );
}

static int
bibtex_matches_asis_corps( fields *info, char *tag, newstr *data, int level,
	list *asis, list *corps )
{
	newstr newtag;
	int i;
	for ( i=0; i<asis->n; ++i ) {
		if ( !strcmp( data->data, list_getc( asis, i ) ) ) {
			newstr_initstr( &newtag, tag );
			newstr_strcat( &newtag, ":ASIS" );
			fields_add( info, newtag.data, data->data, level );
			newstr_free( &newtag );
			return 1;
		}
	}
	for ( i=0; i<corps->n; ++i ) {
		if ( !strcmp( data->data, list_getc( corps, i ) ) ) {
			newstr_initstr( &newtag, tag );
			newstr_strcat( &newtag, ":CORP" );
			fields_add( info, newtag.data, data->data, level );
			newstr_free( &newtag );
			return 1;
		}
	}
	return 0;
}

/*
 * bibtex_names( info, newtag, field, level);
 *
 * split names in author list separated by and's (use '|' character)
 * and add names
 */
static int
bibtex_names( fields *info, char *tag, newstr *data, int level, list *asis,
	list *corps )
{
	int begin, end, ok, n, etal, i, ret = 1;
	list tokens;

	/* If we match the asis or corps list add and bail. */
	if ( bibtex_matches_asis_corps( info, tag, data, level, asis, corps ) )
		return 1;

	list_init( &tokens );

	bibtex_split( &tokens, data );
	for ( i=0; i<tokens.n; ++i )
		bibtex_cleantoken( list_get( &tokens, i ) );

	etal = name_findetal( &tokens );

	begin = 0;
	n = tokens.n - etal;
	while ( begin < n ) {

		end = begin + 1;

		while ( end < n && strcasecmp( list_getc( &tokens, end ), "and" ) )
			end++;

		if ( end - begin == 1 ) {
			ok = name_addsingleelement( info, tag, list_getc( &tokens, begin ), level, 0 );
			if ( !ok ) { ret = 0; goto out; }
		} else {
			ok = name_addmultielement( info, tag, &tokens, begin, end, level );
			if ( !ok ) { ret = 0; goto out; }
		}

		begin = end + 1;

		/* Handle repeated 'and' errors like: authors="G. F. Author and and B. K. Author" */
		while ( begin < n && !strcasecmp( list_getc( &tokens, begin ), "and" ) )
			begin++;
	}

	if ( etal ) {
		ret = name_addsingleelement( info, tag, "et al.", level, 0 );
	}

out:
	list_free( &tokens );
	return ret;
}

/* is_utf8_emdash()
 *
 * Internally pages="A---B" will convert --- to a UTF8
 * emdash = 0xE2 (-30) 0x80 (-128) 0x94 (-108)
 */
static int
is_utf8_emdash( char *p )
{
	static char emdash[3] = { -30, -128, -108 };
	if ( strncmp( p, emdash, 3 ) ) return 0;
	return 1;
}
/* is_utf8_endash()
 *
 * Internally pages="A--B" will convert -- to a UTF8
 * endash = 0xE2 (-30) 0x80 (-128) 0x93 (-109)
 */
static int
is_utf8_endash( char *p )
{
	static char endash[3] = { -30, -128, -109 };
	if ( strncmp( p, endash, 3 ) ) return 0;
	return 1;
}

static int
process_pages( fields *info, newstr *s, int level )
{
	newstr page;
	char *p;
	int ok;

	newstr_findreplace( s, " ", "" );
	if ( s->len==0 ) return 1;

	newstr_init( &page );
	p = skip_ws( s->data );
	while ( *p && !is_ws(*p) && *p!='-' && *p!='\r' && *p!='\n' && *p!=-30 )
		newstr_addchar( &page, *p++ );
	if ( page.len>0 ) {
		ok = fields_add( info, "PAGESTART", page.data, level );
		if ( !ok ) return 0;
	}

	while ( *p && (is_ws(*p) || *p=='-' ) ) p++;
	if ( *p && is_utf8_emdash( p ) ) p+=3;
	if ( *p && is_utf8_endash( p ) ) p+=3;

	newstr_empty( &page );
	while ( *p && !is_ws(*p) && *p!='-' && *p!='\r' && *p!='\n' )
		newstr_addchar( &page, *p++ );
	if ( page.len>0 ) {
		ok = fields_add( info, "PAGEEND", page.data, level );
		if ( !ok ) return 0;
	}

	newstr_free( &page );
	return 1;
}

static int
process_urlcore( fields *info, newstr *d, int level, char *default_tag )
{
	char *p = d->data;
	if ( !strncasecmp( p, "\\urllink", 8 ) )
		return fields_add( info, "URL", p+8, level );
	else if ( !strncasecmp( p, "\\url", 4 ) )
		return fields_add( info, "URL", p+4, level );
	else if ( !strncasecmp( p, "arXiv:", 6 ) )
		return fields_add( info, "ARXIV", p+6, level );
	else if ( !strncasecmp( p, "http://arxiv.org/abs/", 21 ) )
		return fields_add( info, "ARXIV", p+21, level );
	else if ( !strncasecmp( p, "http:", 5 ) )
		return fields_add( info, "URL", p, level );
	else return fields_add( info, default_tag, p, level );
}

static int
process_url( fields *info, newstr *d, int level )
{
	return process_urlcore( info, d, level, "URL" );
}

/* Split keywords="" with semicolons.
 * Commas are also frequently used, but will break
 * entries like:
 *       keywords="Microscopy, Confocal"
 */
static int
process_keywords( fields *info, newstr *d, int level )
{
	newstr keyword;
	char *p;
	int ok;

	if ( !d || d->len==0 ) return 1;

	p = d->data;
	newstr_init( &keyword );

	while ( *p ) {
		p = skip_ws( p );
		while ( *p && *p!=';' ) newstr_addchar( &keyword, *p++ );
		newstr_trimendingws( &keyword );
		if ( keyword.len ) {
			ok = fields_add( info, "KEYWORD", keyword.data, level );
			if ( !ok ) return 0;
			newstr_empty( &keyword );
		}
		if ( *p==';' ) p++;
	}
	newstr_free( &keyword );
	return 1;
}

/* proces_howpublished()
 *
 *    howpublished={},
 *
 * Normally indicates the manner in which something was
 * published in lieu of a formal publisher, so typically
 * 'howpublished' and 'publisher' will never be in the
 * same reference.
 *
 * Occassionally, people put Diploma thesis information
 * into the field, so check that first.
 */
static int
process_howpublished( fields *info, newstr *d, int level )
{
	char *p = d->data;
	if ( !strncasecmp( p, "Diplom", 6 ) )
		return fields_replace_or_add( info, "GENRE", "Diploma thesis", level );
	else if ( !strncasecmp( p, "Habilitation", 13 ) )
		return fields_replace_or_add( info, "GENRE", "Habilitation thesis", level );
	else if ( !strncasecmp( d->data, "http:", 5 ) )
		return process_url( info, d, level );
	else if ( !strncasecmp( d->data, "arXiv:", 6 ) )
		return process_url( info, d, level );
	else 
		return fields_add( info, "PUBLISHER", p, level );
}

/*
 * sentelink = {file://localhost/full/path/to/file.pdf,Sente,PDF}
 */
static int
process_sente( fields *info, newstr *d, int level )
{
	int ret = 1;
	newstr link;
	char *p = d->data;
	newstr_init( &link );
	while ( *p && *p!=',' ) newstr_addchar( &link, *p++ );
	newstr_trimstartingws( &link );
	newstr_trimendingws( &link );
	if ( link.len ) ret = fields_add( info, "FILEATTACH", link.data, level );
	newstr_free( &link );
	return ret;
}

/*
 * BibTeX uses 'organization' in lieu of publisher if that field is missing.
 * Otherwise output as
 * <name type="corporate">
 *    <namePart>The organization</namePart>
 *    <role>
 *       <roleTerm authority="marcrelator" type="text">organizer of meeting</roleTerm>
 *    </role>
 * </name>
 */
static int
process_organization( fields *bibin, fields *info, newstr *d, int level )
{
	int n;
	n = fields_find( bibin, "publisher", LEVEL_ANY );
	if ( n==-1 )
		return fields_add( info, "PUBLISHER", d->data, level );
	else
		return fields_add( info, "ORGANIZER:CORP", d->data, level );
}

static int
count_colons( char *p )
{
	int n = 0;
	while ( *p ) {
		if ( *p==':' ) n++;
		p++;
	}
	return n;
}

static int
first_colon( char *p )
{
	int n = 0;
	while ( p[n] && p[n]!=':' ) n++;
	return n;
}

static int
last_colon( char *p )
{
	int n = strlen( p ) - 1;
	while ( n>0 && p[n]!=':' ) n--;
	return n;
}

/*
 * file={Description:/full/path/to/file.pdf:PDF}
 */
static int
process_file( fields *info, newstr *d, int level )
{
	char *p = d->data;
	newstr link;
	int i, n, n1, n2, ret = 1;

	n = count_colons( p );
	if ( n > 1 ) {
		/* A DOS file can contain a colon ":C:/....pdf:PDF" */
		/* Extract after 1st and up to last colons */
		n1 = first_colon( p ) + 1;
		n2 = last_colon( p );
		newstr_init( &link );
		for ( i=n1; i<n2; ++i ) {
			newstr_addchar( &link, p[i] );
		}
		newstr_trimstartingws( &link );
		newstr_trimendingws( &link );
		if ( link.len ) ret = fields_add( info, "FILEATTACH", link.data, level );
		newstr_free( &link );
	} else {
		/* This field isn't formatted properly, so just copy directly */
		ret = fields_add( info, "FILEATTACH", p, level );
	}
	return ret;
}

static int
process_note( fields *info, newstr *d, int level )
{
	if ( !strncasecmp( d->data, "http:", 5 ) ||
	     !strncasecmp( d->data, "arXiv:", 6 ) ) {
		return process_url( info, d, level );
	} else {
		return fields_add( info, "NOTES", d->data, level );
	}
}

int
bibtexin_typef( fields *bibin, char *filename, int nrefs, param *p,
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
bibtexin_notag( param *p, char *tag )
{
	if ( p->verbose && strcmp( tag, "INTERNAL_TYPE" ) ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Cannot find tag '%s'\n", tag );
	}
}

/* bibtexin_titleinbook_isbooktitle()
 *
 * Normally, the title field of inbook refers to the book.  The
 * section in a @inbook reference is untitled.  If it's titled,
 * the @incollection should be used.  For example, in:
 *
 * @inbook{
 *    title="xxx"
 * }
 *
 * the booktitle is "xxx".
 *
 * However, @inbook is frequently abused (and treated like
 * @incollection) so that title and booktitle are present
 * and title is now 'supposed' to refer to the section.  For example:
 *
 * @inbook{
 *     title="yyy",
 *     booktitle="xxx"
 * }
 *
 * Therefore report whether or not booktitle is present as well
 * as title in @inbook references.  If not, then make 'title'
 * correspond to the title of the book, not the section.
 *
 */
static int
bibtexin_titleinbook_isbooktitle( char *intag, fields *bibin )
{
	int n;

	/* ...look only at 'title="xxx"' elements */
	if ( strcasecmp( intag, "TITLE" ) ) return 0;

	/* ...look only at '@inbook' references */
	n = fields_find( bibin, "INTERNAL_TYPE", -1 );
	if ( n==-1 ) return 0;
	if ( strcasecmp( fields_value( bibin, n, FIELDS_CHRP ), "INBOOK" ) ) return 0;

	/* ...look to see if 'booktitle="yyy"' exists */
	n = fields_find( bibin, "BOOKTITLE", -1 );
	if ( n==-1 ) return 0;
	else return 1;
}
static int
bibtexin_title_process( fields *info, char *outtag, fields *bibin, newstr *t, newstr *d, int level, int nosplittitle )
{
	char *intag = t->data;
	char *indata = d->data;
	if ( bibtexin_titleinbook_isbooktitle( intag, bibin ) ) level=LEVEL_MAIN;
	title_process( info, outtag, indata, level, nosplittitle );
	return 1;
}
static int
bibtex_simple( fields *info, char *outtag, newstr *d, int level )
{
	return fields_add( info, outtag, d->data, level );
}

int
bibtexin_convertf( fields *bibin, fields *info, int reftype, param *p,
		variants *all, int nall )
{
	int process, level, i, n, nfields, ok;
	newstr *t, *d;
	char *outtag;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

		if ( fields_used( bibin, i ) ) continue; /* e.g. successful crossref */
		if ( fields_nodata( bibin, i ) ) continue;

		t = fields_tag( bibin, i, FIELDS_STRP );
		if ( t->len == 0 ) continue; /* Don't consider with null tags */
		n = process_findoldtag( t->data, reftype, all, nall );
		if ( n==-1 ) {
			bibtexin_notag( p, t->data );
			continue;
		}

		d = fields_value( bibin, i, FIELDS_STRP );

		process = ((all[reftype]).tags[n]).processingtype;
		level   = ((all[reftype]).tags[n]).level;
		outtag  = ((all[reftype]).tags[n]).newstr;

		switch( process ) {

		case SIMPLE:
			ok = bibtex_simple( info, outtag, d, level );
			break;

		case TITLE:
			ok = bibtexin_title_process( info, "TITLE", bibin, t, d, level, p->nosplittitle );
			break;

		case PERSON:
			ok = bibtex_names( info, outtag, d, level, &(p->asis), &(p->corps) );
			break;

		case PAGES:
			ok = process_pages( info, d, level );
			break;

		case KEYWORD:
			ok = process_keywords( info, d, level );
			break;

		case HOWPUBLISHED:
			ok = process_howpublished( info, d, level );
			break;

		case LINKEDFILE:
			ok = process_file( info, d, level );
			break;

		case BT_NOTE:
			ok = process_note( info, d, level );
			break;

		case BT_SENTE:
			ok = process_sente( info, d, level );
			break;

		case BT_URL:
			ok = process_url( info, d, level );
			break;

		case BT_ORG:
			ok = process_organization( bibin, info, d, level );
			break;

		default:
			ok = 1;
			break;
		}

		if ( !ok ) return BIBL_ERR_MEMERR;
	}
	return BIBL_OK;
}

