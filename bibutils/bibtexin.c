/*
 * bibtexin.c
 *
 * Copyright (c) Chris Putnam 2003-2016
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

static list find    = { 0, 0, NULL, 0 };
static list replace = { 0, 0, NULL, 0 };

extern const char progname[];

/*****************************************************
 PUBLIC: void bibtexin_initparams()
*****************************************************/
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

/*****************************************************
 PUBLIC: int bibtexin_readf()
*****************************************************/

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

/*****************************************************
 PUBLIC: int bibtexin_processf()
*****************************************************/

static char*
process_bibtextype( char *p, newstr *type )
{
	newstr tmp;
	newstr_init( &tmp );

	if ( *p=='@' ) p++;
	p = newstr_cpytodelim( &tmp, p, "{( \t\r\n", 0 );
	p = skip_ws( p );
	if ( *p=='{' || *p=='(' ) p++;
	p = skip_ws( p );

	if ( tmp.len ) newstr_strcpy( type, tmp.data );
	else newstr_empty( type );

	newstr_free( &tmp );
	return p;
}

static char*
process_bibtexid( char *p, newstr *id )
{
	char *start_p = p;
	newstr tmp;

	newstr_init( &tmp );
	p = newstr_cpytodelim( &tmp, p, ",", 1 );

	if ( tmp.len ) {
		if ( strchr( tmp.data, '=' ) ) {
			/* Endnote writes bibtex files w/o fields, try to
			 * distinguish via presence of an equal sign.... if
			 * it's there, assume that it's a tag/data pair instead
			 * and roll back.
			 */
			p = start_p;
			newstr_empty( id );
		} else {
			newstr_strcpy( id, tmp.data );
		}
	} else {
		newstr_empty( id );
	}

	newstr_free( &tmp );
	return skip_ws( p );
}

static char *
bibtex_tag( char *p, newstr *tag )
{
	p = newstr_cpytodelim( tag, skip_ws( p ), "= \t\r\n", 0 );
	if ( newstr_memerr( tag ) ) return NULL;
	return skip_ws( p );
}

static char *
bibtex_data( char *p, fields *bibin, list *tokens )
{
	unsigned int nbracket = 0, nquotes = 0;
	char *startp = p;
	newstr tok, *t;

	newstr_init( &tok );
	while ( p && *p ) {
		if ( !nquotes && !nbracket ) {
			if ( *p==',' || *p=='=' || *p=='}' || *p==')' )
				goto out;
		}
		if ( *p=='\"' && nbracket==0 && ( p==startp || *(p-1)!='\\' ) ) {
			nquotes = !nquotes;
			newstr_addchar( &tok, *p );
			if ( !nquotes ) {
				if ( newstr_memerr( &tok ) ) { p=NULL; goto out; }
				t = list_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out0; }
				newstr_empty( &tok );
			}
		} else if ( *p=='#' && !nquotes && !nbracket ) {
			if ( tok.len ) {
				if ( newstr_memerr( &tok ) ) { p=NULL; goto out; }
				t = list_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out0; }
			}
			newstr_strcpy( &tok, "#" );
			t = list_add( tokens, &tok );
			if ( !t ) { p=NULL; goto out0; }
			newstr_empty( &tok );
		} else if ( *p=='{' && !nquotes && ( p==startp || *(p-1)!='\\' ) ) {
			nbracket++;
			newstr_addchar( &tok, *p );
		} else if ( *p=='}' && !nquotes && ( p==startp || *(p-1)!='\\' ) ) {
			nbracket--;
			newstr_addchar( &tok, *p );
			if ( nbracket==0 ) {
				if ( newstr_memerr( &tok ) ) { p=NULL; goto out; }
				t = list_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out; }
				newstr_empty( &tok );
			}
		} else if ( !is_ws( *p ) || nquotes || nbracket ) {
			if ( !is_ws( *p ) ) newstr_addchar( &tok, *p );
			else {
				if ( tok.len!=0 && *p!='\n' && *p!='\r' )
					newstr_addchar( &tok, *p );
				else if ( tok.len!=0 && (*p=='\n' || *p=='\r')) {
					newstr_addchar( &tok, ' ' );
					while ( is_ws( *(p+1) ) ) p++;
				}
			}
		} else if ( is_ws( *p ) ) {
			if ( tok.len ) {
				if ( newstr_memerr( &tok ) ) { p=NULL; goto out; }
				t = list_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out; }
				newstr_empty( &tok );
			}
		}
		p++;
	}
out:
	if ( nbracket!=0 ) {
		fprintf( stderr, "%s: Mismatch in number of brackets in reference.\n", progname );
	}
	if ( nquotes!=0 ) {
		fprintf( stderr, "%s: Mismatch in number of quotes in reference.\n", progname );
	}
	if ( tok.len ) {
		if ( newstr_memerr( &tok ) ) { p = NULL; goto out; }
		t = list_add( tokens, &tok );
		if ( !t ) p = NULL;
	}
out0:
	newstr_free( &tok );
	return p;
}

/* replace_strings()
 *
 * do string replacement -- only if unprotected by quotation marks or curly brackets
 */
static void
replace_strings( list *tokens, fields *bibin )
{
	int i, n, ok;
	newstr *s;
	char *q;
	i = 0;
	while ( i < tokens->n ) {
		s = list_get( tokens, i );
		if ( !strcmp( s->data, "#" ) ) {
		} else if ( s->data[0]!='\"' && s->data[0]!='{' ) {
			n = list_find( &find, s->data );
			if ( n!=-1 ) {
				newstr_newstrcpy( s, list_get( &replace, n ) );
			} else {
				q = s->data;
				ok = 1;
				while ( *q && ok ) {
					if ( !isdigit( *q ) ) ok = 0;
					q++;
				}
				if ( !ok ) {
					fprintf( stderr, "%s: Warning: Non-numeric "
					   "BibTeX elements should be in quotations or "
					   "curly brackets in reference.\n", progname );
				}
			}
		}
		i++;
	}
}

static int
string_concatenate( list *tokens, fields *bibin )
{
	newstr *s, *t;
	int i, status;
	i = 0;
	while ( i < tokens->n ) {
		s = list_get( tokens, i );
		if ( !strcmp( s->data, "#" ) ) {
			if ( i==0 || i==tokens->n-1 ) {
				fprintf( stderr, "%s: Warning: Stray string concatenation "
					"('#' character) in reference\n", progname );
				status = list_remove( tokens, i );
				if ( status!=LIST_OK ) return BIBL_ERR_MEMERR;
				continue;
			}
			s = list_get( tokens, i-1 );
			if ( s->data[0]!='\"' && s->data[s->len-1]!='\"' )
				fprintf( stderr, "%s: Warning: String concentation should "
					"be used in context of quotations marks.\n", progname );
			t = list_get( tokens, i+1 );
			if ( t->data[0]!='\"' && t->data[s->len-1]!='\"' )
				fprintf( stderr, "%s: Warning: String concentation should "
					"be used in context of quotations marks.\n", progname );
			if ( ( s->data[s->len-1]=='\"' && t->data[0]=='\"') || (s->data[s->len-1]=='}' && t->data[0]=='{') ) {
				newstr_trimend( s, 1 );
				newstr_trimbegin( t, 1 );
				newstr_newstrcat( s, t );
			} else {
				newstr_newstrcat( s, t );
			}
			status = list_remove( tokens, i );
			if ( status!=LIST_OK ) return BIBL_ERR_MEMERR;
			status = list_remove( tokens, i );
			if ( status!=LIST_OK ) return BIBL_ERR_MEMERR;
		} else i++;
	}
	return BIBL_OK;
}

/* return NULL on memory error */
static char *
process_bibtexline( char *p, newstr *tag, newstr *data, uchar stripquotes, fields *bibin )
{
	int i, status;
	list tokens;
	newstr *s;

	newstr_empty( data );

	p = bibtex_tag( p, tag );
	if ( p==NULL || tag->len==0 ) return p;

	list_init( &tokens );

	if ( *p=='=' ) {
		p = bibtex_data( p+1, bibin, &tokens );
		if ( p==NULL ) goto out;
	}

	replace_strings( &tokens, bibin );

	status = string_concatenate( &tokens, bibin );
	if ( status!=BIBL_OK ) {
		p = NULL;
		goto out;
	}

	for ( i=0; i<tokens.n; i++ ) {
		s = list_get( &tokens, i );
		if ( ( stripquotes && s->data[0]=='\"' && s->data[s->len-1]=='\"' ) ||
		     ( s->data[0]=='{' && s->data[s->len-1]=='}' ) ) {
			newstr_trimbegin( s, 1 );
			newstr_trimend( s, 1 );
		}
		newstr_newstrcat( data, list_get( &tokens, i ) );
	}
out:
	list_free( &tokens );
	return p;
}

/* process_cite()
 *
 */
static int
process_cite( fields *bibin, char *p, char *filename, long nref )
{
	int fstatus, status = BIBL_OK;
	newstr tag, data;

	newstrs_init( &tag, &data, NULL );

	p = process_bibtextype( p, &data );
	if ( data.len ) {
		fstatus = fields_add( bibin, "INTERNAL_TYPE", data.data, 0 );
		if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
	}

	p = process_bibtexid( p, &data );
	if ( data.len ) {
		fstatus = fields_add( bibin, "REFNUM", data.data, 0 );
		if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
	}

	while ( *p ) {
		p = process_bibtexline( p, &tag, &data, 1, bibin );
		if ( p==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		/* no anonymous or empty fields allowed */
		if ( tag.len && data.len ) {
			fstatus = fields_add( bibin, tag.data, data.data, 0 );
			if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
		}
		newstrs_empty( &tag, &data, NULL );
	}
out:
	newstrs_free( &tag, &data, NULL );
	return status;
}

/* process_string()
 *
 * Handle lines like:
 *
 * '@STRING{TL = {Tetrahedron Lett.}}'
 *
 * p should point to just after '@STRING'
 *
 * In BibTeX, if a string is defined several times, the last one is kept.
 *
 */
static int
process_string( char *p )
{
	int n, status = BIBL_OK;
	newstr s1, s2, *t;
	newstrs_init( &s1, &s2, NULL );
	while ( *p && *p!='{' && *p!='(' ) p++;
	if ( *p=='{' || *p=='(' ) p++;
	p = process_bibtexline( skip_ws( p ), &s1, &s2, 0, NULL );
	if ( p==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
	if ( s2.data ) {
		newstr_findreplace( &s2, "\\ ", " " );
	}
	if ( s1.data ) {
		n = list_find( &find, s1.data );
		if ( n==-1 ) {
			t = list_add( &find, &s1 );
			if ( t==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
			if ( s2.data ) t = list_add( &replace, &s2 );
			else t = list_addc( &replace, "" );
			if ( t==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		} else {
			if ( s2.data ) t = list_set( &replace, n, &s2 );
			else t = list_setc( &replace, n, "" );
			if ( t==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		}
	}
out:
	newstrs_free( &s1, &s2, NULL );
	return status;
}

/* bibtexin_processf()
 *
 * Handle '@STRING', '@reftype', and ignore '@COMMENT'
 */
int
bibtexin_processf( fields *bibin, char *data, char *filename, long nref )
{
	if ( !strncasecmp( data, "@STRING", 7 ) ) {
		process_string( data+7 );
		return 0;
	} else if ( !strncasecmp( data, "@COMMENT", 8 ) ) {
		/* Not sure if these are real Bibtex, but not references */
		return 0;
	} else {
		process_cite( bibin, data, filename, nref );
		return 1;
	}
}

/*****************************************************
 PUBLIC: void bibtexin_cleanf()
*****************************************************/

static int
bibtex_protected( newstr *data )
{
	if ( data->data[0]=='{' && data->data[data->len-1]=='}' ) return 1;
	if ( data->data[0]=='\"' && data->data[data->len-1]=='\"' ) return 1;
	return 0;
}

static int
bibtex_split( list *tokens, newstr *s )
{
	int i, n = s->len, nbrackets = 0, status = BIBL_OK;
	newstr tok, *t;

	newstr_init( &tok );

	for ( i=0; i<n; ++i ) {
		if ( s->data[i]=='{' && ( i==0 || s->data[i-1]!='\\' ) ) {
			nbrackets++;
			newstr_addchar( &tok, '{' );
		} else if ( s->data[i]=='}' && ( i==0 || s->data[i-1]!='\\' ) ) {
			nbrackets--;
			newstr_addchar( &tok, '}' );
		} else if ( !is_ws( s->data[i] ) || nbrackets ) {
			newstr_addchar( &tok, s->data[i] );
		} else if ( is_ws( s->data[i] ) ) {
			if ( tok.len ) {
				t = list_add( tokens, &tok );
				if ( !t ) {
					status = BIBL_ERR_MEMERR;
					goto out;
				}
			}
			newstr_empty( &tok );
		}
	}
	if ( tok.len ) {
		t = list_add( tokens, &tok );
		if ( !t ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

	for ( i=0; i<tokens->n; ++i ) {
		newstr_trimstartingws( list_get( tokens, i ) );
		newstr_trimendingws( list_get( tokens, i ) );
	}
out:
	newstr_free( &tok );
	return status;
}

static int
bibtex_addtitleurl( fields *info, newstr *in )
{
	int fstatus, status = BIBL_OK;
	newstr s;
	char *p;

	newstr_init( &s );

	/* ...skip past "\href{" and copy to "}" */
	p = newstr_cpytodelim( &s, in->data + 6, "}", 1 );
	if ( newstr_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }

	/* ...add to URL */
	fstatus = fields_add( info, "URL", s.data, 0 );
	if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }

	/* ...return deleted fragment to newstr in */
	p = newstr_cpytodelim( &s, p, "", 0 );
	if ( newstr_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }
	newstr_swapstrings( &s, in );

out:
	newstr_free( &s );
	return status;
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
bibtex_process_tilde( newstr *s )
{
	char *p, *q;
	int n = 0;

	p = q = s->data;
	if ( !p ) return;
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
bibtex_process_bracket( newstr *s )
{
	char *p, *q;
	int n = 0;

	p = q = s->data;
	if ( !p ) return;
	while ( *p ) {
		if ( *p=='\\' && ( *(p+1)=='{' || *(p+1)=='}' ) ) {
			n++;
			p++;
			*q = *p;
			q++;
		} else if ( *p=='{' || *p=='}' ) {
			n++;
		} else {
			*q = *p;
			q++;
		}
		p++;
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
	while ( newstr_findreplace( s, "  ", " " ) ) {}

	/* 'textcomp' annotations that we don't want to substitute on output*/
	newstr_findreplace( s, "\\textdollar", "$" );
	newstr_findreplace( s, "\\textunderscore", "_" );

	bibtex_process_bracket( s );
	bibtex_process_tilde( s );

}

static int
bibtex_cleandata( newstr *tag, newstr *s, fields *info, param *p )
{
	int i, status;
	list tokens;
	newstr *tok;
	if ( !s->len ) return BIBL_OK;
	/* protect url from undergoing any parsing */
	if ( is_url_tag( tag ) ) return BIBL_OK;
	list_init( &tokens );
	status = bibtex_split( &tokens, s );
	if ( status!=BIBL_OK ) goto out;
	for ( i=0; i<tokens.n; ++i ) {
		tok = list_get( &tokens, i );
		if ( bibtex_protected( tok ) ) {
			if (!strncasecmp(tok->data,"\\href{", 6)) {
				bibtex_addtitleurl( info, tok );
			}
		}
		if ( p->latexin && !is_name_tag( tag ) && !is_url_tag( tag ) )
			bibtex_cleantoken( tok );
	}
	newstr_empty( s );
	for ( i=0; i<tokens.n; ++i ) {
		tok = list_get( &tokens, i );
		if ( i>0 ) newstr_addchar( s, ' ' );
		newstr_newstrcat( s, tok );
	}
out:
	list_free( &tokens );
	return status;
}

static int
bibtexin_cleanref( fields *bibin, param *p )
{
	int i, n, status;
	newstr *t, *d;
	n = fields_num( bibin );
	for ( i=0; i<n; ++i ) {
		t = fields_tag( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		status = bibtex_cleandata( t, d, bibin, p );
		if ( status!=BIBL_OK ) return status;
	}
	return BIBL_OK;
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

static int
bibtexin_crossref_oneref( fields *bibref, fields *bibcross )
{
	int j, n, nl, ntype, fstatus, status = BIBL_OK;
	char *type, *nt, *nv;

	ntype = fields_find( bibref, "INTERNAL_TYPE", -1 );
	type = ( char * ) fields_value( bibref, ntype, FIELDS_CHRP_NOUSE );

	n = fields_num( bibcross );
	for ( j=0; j<n; ++j ) {
		nt = ( char * ) fields_tag( bibcross, j, FIELDS_CHRP_NOUSE );
		if ( !strcasecmp( nt, "INTERNAL_TYPE" ) ) continue;
		if ( !strcasecmp( nt, "REFNUM" ) ) continue;
		if ( !strcasecmp( nt, "TITLE" ) ) {
			if ( !strcasecmp( type, "Inproceedings" ) ||
			     !strcasecmp( type, "Incollection" ) )
				nt = "booktitle";
		}
		nv = ( char * ) fields_value( bibcross, j, FIELDS_CHRP_NOUSE );
		nl = fields_level( bibcross, j ) + 1;
		fstatus = fields_add( bibref, nt, nv, nl );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}
out:
	return status;
}

static int
bibtexin_crossref( bibl *bin, param *p )
{
	int i, n, ncross, status = BIBL_OK;
	fields *bibref, *bibcross;

	for ( i=0; i<bin->nrefs; ++i ) {
		bibref = bin->ref[i];
		n = fields_find( bibref, "CROSSREF", -1 );
		if ( n==-1 ) continue;
		fields_setused( bibref, n );
		ncross = bibtexin_findref( bin, (char*) fields_value( bibref, n, FIELDS_CHRP ) );
		if ( ncross==-1 ) {
			bibtexin_nocrossref( bin, i, n, p );
			continue;
		}
		bibcross = bin->ref[ncross];
		status = bibtexin_crossref_oneref( bibref, bibcross );
		if ( status!=BIBL_OK ) goto out;
	}
out:
	return status;
}

int
bibtexin_cleanf( bibl *bin, param *p )
{
	int status = BIBL_OK;
	long i;

        for ( i=0; i<bin->nrefs; ++i )
		status = bibtexin_cleanref( bin->ref[i], p );
	bibtexin_crossref( bin, p );
	return status;
}

/*****************************************************
 PUBLIC: int bibtexin_typef()
*****************************************************/

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

/*****************************************************
 PUBLIC: int bibtexin_convertf(), returns BIBL_OK or BIBL_ERR_MEMERR
*****************************************************/

static int
bibtex_matches_list( fields *info, char *tag, char *suffix, newstr *data, int level,
		list *names, int *match )
{
	int i, fstatus, status = BIBL_OK;
	newstr newtag;

	*match = 0;
	if ( names->n==0 ) return status;

	newstr_init( &newtag );

	for ( i=0; i<names->n; ++i ) {
		if ( strcmp( data->data, list_getc( names, i ) ) ) continue;
		newstr_initstr( &newtag, tag );
		newstr_strcat( &newtag, suffix );
		fstatus = fields_add( info, newtag.data, data->data, level );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		*match = 1;
		goto out;
	}

out:
	newstr_free( &newtag );
	return status;
}

/*
 * bibtex_names( info, newtag, field, level);
 *
 * split names in author list separated by and's (use '|' character)
 * and add names
 *
 * returns BIBL_OK on success, BIBL_ERR_MEMERR on memory error
 */
static int
bibtex_names( fields *info, char *tag, newstr *data, int level, list *asis,
	list *corps )
{
	int begin, end, ok, n, etal, i, status, match;
	list tokens;

	/* If we match the asis or corps list add and bail. */
	status = bibtex_matches_list( info, tag, ":ASIS", data, level, asis, &match );
	if ( match==1 || status!=BIBL_OK ) return status;
	status = bibtex_matches_list( info, tag, ":CORP", data, level, corps, &match );
	if ( match==1 || status!=BIBL_OK ) return status;

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
			if ( !ok ) { status = BIBL_ERR_MEMERR; goto out; }
		} else {
			ok = name_addmultielement( info, tag, &tokens, begin, end, level );
			if ( !ok ) { status = BIBL_ERR_MEMERR; goto out; }
		}

		begin = end + 1;

		/* Handle repeated 'and' errors: authors="G. F. Author and and B. K. Author" */
		while ( begin < n && !strcasecmp( list_getc( &tokens, begin ), "and" ) )
			begin++;
	}

	if ( etal ) {
		ok = name_addsingleelement( info, tag, "et al.", level, 0 );
		if ( !ok ) status = BIBL_ERR_MEMERR;
	}

out:
	list_free( &tokens );
	return status;
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
	int fstatus, status = BIBL_OK;
	newstr page;
	char *p;

	newstr_findreplace( s, " ", "" );
	if ( s->len==0 ) return 1;

	newstr_init( &page );
	p = skip_ws( s->data );
	while ( *p && !is_ws(*p) && *p!='-' && *p!='\r' && *p!='\n' && *p!=-30 )
		newstr_addchar( &page, *p++ );
	if ( page.len>0 ) {
		fstatus = fields_add( info, "PAGESTART", page.data, level );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

	while ( *p && (is_ws(*p) || *p=='-' ) ) p++;
	if ( *p && is_utf8_emdash( p ) ) p+=3;
	if ( *p && is_utf8_endash( p ) ) p+=3;

	newstr_empty( &page );
	while ( *p && !is_ws(*p) && *p!='-' && *p!='\r' && *p!='\n' )
		newstr_addchar( &page, *p++ );
	if ( page.len>0 ) {
		fstatus = fields_add( info, "PAGEEND", page.data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR; 
	}

out:
	newstr_free( &page );
	return status;
}

static int
process_urlcore( fields *info, newstr *d, int level, char *default_tag )
{
	char *p = d->data;
	int fstatus;
	if ( !strncasecmp( p, "\\urllink", 8 ) )
		fstatus = fields_add( info, "URL", p+8, level );
	else if ( !strncasecmp( p, "\\url", 4 ) )
		fstatus = fields_add( info, "URL", p+4, level );
	else if ( !strncasecmp( p, "arXiv:", 6 ) )
		fstatus = fields_add( info, "ARXIV", p+6, level );
	else if ( !strncasecmp( p, "http://arxiv.org/abs/", 21 ) )
		fstatus = fields_add( info, "ARXIV", p+21, level );
	else if ( !strncasecmp( p, "http:", 5 ) )
		fstatus = fields_add( info, "URL", p, level );
	else
		fstatus = fields_add( info, default_tag, p, level );
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
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
 * Returns BIBL_OK or BIBL_ERR_MEMERR
 */
static int
process_keywords( fields *info, newstr *d, int level )
{
	int fstatus, status = BIBL_OK;
	newstr keyword;
	char *p;

	if ( !d || d->len==0 ) return BIBL_OK;

	p = d->data;
	newstr_init( &keyword );

	while ( *p ) {
		p = newstr_cpytodelim( &keyword, skip_ws( p ), ";", 1 );
		newstr_trimendingws( &keyword );
		if ( newstr_memerr( &keyword ) ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		if ( keyword.len ) {
			fstatus = fields_add( info, "KEYWORD", keyword.data, level );
			if ( fstatus!=FIELDS_OK ) {
				status = BIBL_ERR_MEMERR;
				goto out;
			}
		}
	}
out:
	newstr_free( &keyword );
	return status;
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
 *
 * Returns BIBL_OK or BIBL_ERR_MEMERR
 */
static int
process_howpublished( fields *info, newstr *d, int level )
{
	int fstatus, status = BIBL_OK;
	char *p = d->data;
	if ( !strncasecmp( p, "Diplom", 6 ) ) {
		fstatus = fields_replace_or_add( info, "GENRE", "Diploma thesis", level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	} else if ( !strncasecmp( p, "Habilitation", 13 ) ) {
		fstatus = fields_replace_or_add( info, "GENRE", "Habilitation thesis", level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	} else if ( !strncasecmp( d->data, "http:", 5 ) ) {
		status = process_url( info, d, level );
	} else if ( !strncasecmp( d->data, "arXiv:", 6 ) ) {
		status = process_url( info, d, level );
	} else {
		fstatus = fields_add( info, "PUBLISHER", p, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	return status;
}

/*
 * sentelink = {file://localhost/full/path/to/file.pdf,Sente,PDF}
 */
static int
process_sente( fields *info, newstr *d, int level )
{
	int fstatus, status = BIBL_OK;
	newstr link;

	newstr_init( &link );
	newstr_cpytodelim( &link, skip_ws( d->data ), ",", 0 );
	newstr_trimendingws( &link );
	if ( newstr_memerr( &link ) ) status = BIBL_ERR_MEMERR;

	if ( status==BIBL_OK && link.len ) {
		fstatus = fields_add( info, "FILEATTACH", link.data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

	newstr_free( &link );
	return status;
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
	int n, fstatus;
	n = fields_find( bibin, "publisher", LEVEL_ANY );
	if ( n==-1 )
		fstatus = fields_add( info, "PUBLISHER", d->data, level );
	else
		fstatus = fields_add( info, "ORGANIZER:CORP", d->data, level );
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
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
	int fstatus, status = BIBL_OK;
	char *p = d->data;
	int i, n, n1, n2;
	newstr link;

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
		if ( newstr_memerr( &link ) ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		if ( link.len ) {
			fstatus = fields_add( info, "FILEATTACH", link.data, level );
			if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
		}
out:
		newstr_free( &link );
	} else {
		/* This field isn't formatted properly, so just copy directly */
		fstatus = fields_add( info, "FILEATTACH", p, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	return status;
}

static int
process_note( fields *info, newstr *d, int level )
{
	int fstatus, status = BIBL_OK;
	if ( !strncasecmp( d->data, "http:", 5 ) ||
	     !strncasecmp( d->data, "arXiv:", 6 ) ) {
		status = process_url( info, d, level );
	} else {
		fstatus = fields_add( info, "NOTES", d->data, level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}
	return status;
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
	int ok;
	if ( bibtexin_titleinbook_isbooktitle( intag, bibin ) ) level=LEVEL_MAIN;
	ok = title_process( info, outtag, indata, level, nosplittitle );
	if ( ok ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

static int
bibtex_simple( fields *info, char *outtag, newstr *d, int level )
{
	int fstatus;
	fstatus = fields_add( info, outtag, d->data, level );
	if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	return BIBL_OK;
}

int
bibtexin_convertf( fields *bibin, fields *info, int reftype, param *p,
		variants *all, int nall )
{
	int process, level, i, n, nfields, status = BIBL_OK;
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
			status = bibtex_simple( info, outtag, d, level );
			break;

		case TITLE:
			status = bibtexin_title_process( info, "TITLE", bibin, t, d, level, p->nosplittitle );
			break;

		case PERSON:
			status = bibtex_names( info, outtag, d, level, &(p->asis), &(p->corps) );
			break;

		case PAGES:
			status = process_pages( info, d, level );
			break;

		case KEYWORD:
			status = process_keywords( info, d, level );
			break;

		case HOWPUBLISHED:
			status = process_howpublished( info, d, level );
			break;

		case LINKEDFILE:
			status = process_file( info, d, level );
			break;

		case BT_NOTE:
			status = process_note( info, d, level );
			break;

		case BT_SENTE:
			status = process_sente( info, d, level );
			break;

		case BT_URL:
			status = process_url( info, d, level );
			break;

		case BT_ORG:
			status = process_organization( bibin, info, d, level );
			break;

		default:
			status = BIBL_OK;
			break;
		}

		if ( status!=BIBL_OK ) return status;
	}
	return status;
}

