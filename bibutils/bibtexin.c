/*
 * bibtexin.c
 *
 * Copyright (c) Chris Putnam 2003-2012
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
#include "title.h"
#include "reftypes.h"
#include "bibtexin.h"

list find    = { 0, 0, 0, NULL };
list replace = { 0, 0, 0, NULL };

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
			/*
			if ( s->len!=0 || ( s->len==0 && !is_ws( *p ) ) )
				newstr_addchar( s, *p );
			*/
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
bibtex_cleantoken( newstr *s, param *p )
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
	bibtex_cleantoken( &s2, NULL );
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
bibtex_removeprotection( newstr *data )
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
bibtex_split( list *tokens, newstr *s )
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

static int
bibtex_usestrings( newstr *s )
{
	int i;
	for ( i=0; i<find.n; ++i ) {
		if ( !strcasecmp( s->data, (find.str[i]).data ) ) {
			newstr_findreplace( s, (find.str[i]).data, 
					(replace.str[i]).data );
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

static void
bibtex_cleandata( newstr *s, fields *info, param *p )
{
	list tokens;
	int i;
	if ( !s->len ) return;
	list_init( &tokens );
	bibtex_split( &tokens, s );
	for ( i=0; i<tokens.n; ++i ) {
		if ( !bibtex_protected( &(tokens.str[i] ) ) ) {
			bibtex_usestrings( &(tokens.str[i]) );
		} else {
			if (!strncasecmp(tokens.str[i].data,"\\href{", 6)) {
				bibtex_addtitleurl( info, &(tokens.str[i]) );
			}
			bibtex_cleantoken( &(tokens.str[i]), p );
		}
	}
	newstr_empty( s );
	for ( i=0; i<tokens.n; ++i ) {
		if ( bibtex_protected( &(tokens.str[i]) ) )
			bibtex_removeprotection( &(tokens.str[i]));
		newstr_strcat( s, tokens.str[i].data ); 
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
		bibtex_cleandata( d, bibin, p );
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
bibtexin_cleanf( bibl *bin, param *p )
{
	long i;
        for ( i=0; i<bin->nrefs; ++i )
		bibtexin_cleanref( bin->ref[i], p );
	bibtexin_crossref( bin, p );
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
	newstr_findreplace( data, "|and ", "|" );
	name_add( info, tag, data->data, level, asis, corps );
}

static void
process_pages( fields *info, newstr *s, int level )
{
	char *p, *q;
	newstr sp, ep;

	newstrs_init( &sp, &ep, NULL );
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

	newstrs_free( &sp, &ep, NULL );
}

static void
process_urlcore( fields *info, newstr *d, int level, char *default_tag )
{
	char *p = d->data;
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
process_url( fields *info, newstr *d, int level )
{
	process_urlcore( info, d, level, "URL" );
}

/* Split keywords="" with semicolons.
 * Commas are also frequently used, but will break
 * entries like:
 *       keywords="Microscopy, Confocal"
 */
static void
process_keywords( fields *info, newstr *d, int level )
{
	newstr keyword;
	char *p;

	if ( !d || d->len==0 ) return;

	p = d->data;
	newstr_init( &keyword );

	while ( *p ) {
		p = skip_ws( p );
		while ( *p && *p!=';' ) newstr_addchar( &keyword, *p++ );
		newstr_trimendingws( &keyword );
		if ( keyword.len ) {
			fields_add( info, "KEYWORD", keyword.data, level );
			newstr_empty( &keyword );
		}
		if ( *p==';' ) p++;
	}
	newstr_free( &keyword );
}

static void
process_howpublished( fields *info, newstr *d, int level )
{
	char *p = d->data;
	/* Some users put Diploma thesis in howpublished */
	if ( !strncasecmp( p, "Diplom", 6 ) )
		fields_replace_or_add( info, "GENRE", "Diploma thesis", level );
	else if ( !strncasecmp( p, "Habilitation", 13 ) )
		fields_replace_or_add( info, "GENRE", "Habilitation thesis", level );
	else 
		process_urlcore( info, d, level, "DESCRIPTION" );
}

/*
 * sentelink = {file://localhost/full/path/to/file.pdf,Sente,PDF}
 */
static void
process_sente( fields *info, newstr *d, int level )
{
	newstr link;
	char *p = d->data;
	newstr_init( &link );
	while ( *p && *p!=',' ) newstr_addchar( &link, *p++ );
	newstr_trimstartingws( &link );
	newstr_trimendingws( &link );
	if ( link.len ) fields_add( info, "FILEATTACH", link.data, level );
	newstr_free( &link );
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
static void
process_file( fields *info, newstr *d, int level )
{
	char *p = d->data;
	newstr link;
	int i, n, n1, n2;

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
		if ( link.len ) fields_add( info, "FILEATTACH", link.data, level );
		newstr_free( &link );
	} else {
		/* This field isn't formatted properly, so just copy directly */
		fields_add( info, "FILEATTACH", p, level );
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
report( fields *f )
{
	int i, n;
	n = fields_num( f );
	for ( i=0; i<n; ++i )
		fprintf(stderr, "'%s' %d = '%s'\n",
			(char*)fields_tag( f, i, FIELDS_CHRP ),
			fields_level( f, i ),
			(char*)fields_value( f, i, FIELDS_CHRP ) );
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
static void
bibtexin_title_process( fields *info, char *outtag, fields *bibin, newstr *t, newstr *d, int level, int nosplittitle )
{
	char *intag = t->data;
	char *indata = d->data;
	if ( bibtexin_titleinbook_isbooktitle( intag, bibin ) ) level=LEVEL_MAIN;
	title_process( info, outtag, indata, level, nosplittitle );
}
static void
bibtex_simple( fields *info, char *outtag, newstr *d, int level )
{
	fields_add( info, outtag, d->data, level );
}

void
bibtexin_convertf( fields *bibin, fields *info, int reftype, param *p,
		variants *all, int nall )
{
	int process, level, i, n, nfields;
	newstr *t, *d;
	char *outtag;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

		if ( fields_used( bibin, i ) ) continue; /* e.g. successful crossref */
		if ( fields_nodata( bibin, i ) ) continue;

		t = fields_tag( bibin, i, FIELDS_STRP );
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
			bibtex_simple( info, outtag, d, level );
			break;

		case TITLE:
			bibtexin_title_process( info, "TITLE", bibin, t, d, level, p->nosplittitle );
			break;

		case PERSON:
			process_names( info, outtag, d, level, &(p->asis), &(p->corps) );
			break;

		case PAGES:
			process_pages( info, d, level );
			break;

		case KEYWORD:
			process_keywords( info, d, level );
			break;

		case HOWPUBLISHED:
			process_howpublished( info, d, level );
			break;

		case BIBTEX_URL:
			process_url( info, d, level );
			break;

		case LINKEDFILE:
			process_file( info, d, level );
			break;

		case BIBTEX_SENTE:
			process_sente( info, d, level );
			break;

		case ALWAYS:
			/* added by core bibutils code */
			break;

		default:
			break;
		}
	}
	if ( p->verbose ) report( info );
}

