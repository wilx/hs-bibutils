/*
 * biblatexin.c
 *
 * Copyright (c) Chris Putnam 2008-2016
 * Copyright (c) Johannes Wilm 2010-2016
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
#include "reftypes.h"
#include "biblatexin.h"

extern const char progname[];

static list find    = { 0, 0, NULL, 0 };
static list replace = { 0, 0, NULL, 0 };

/*****************************************************
 PUBLIC: void biblatexin_initparams()
*****************************************************/

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

/*****************************************************
 PUBLIC: int biblatexin_readf()
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

/*****************************************************
 PUBLIC: int biblatexin_processf()
*****************************************************/

static char *
process_biblatextype( char *p, newstr *type )
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

static char *
process_biblatexid( char *p, newstr *id )
{
	char *start_p = p;
	newstr tmp;

	newstr_init( &tmp );
	p = newstr_cpytodelim( &tmp, p, ",", 1 );

	if ( tmp.len ) {
		if ( strchr( tmp.data, '=' ) ) {
			/* Endnote writes biblatex files w/o fields, try to
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
biblatex_tag( char *p, newstr *tag )
{
	p = newstr_cpytodelim( tag, skip_ws( p ), "= \t\r\n", 0 );
	return skip_ws( p );
}

static char *
biblatex_data( char *p, fields *bibin, list *tokens )
{
	unsigned int nbracket = 0, nquotes = 0;
	char *startp = p;
	newstr tok, *s;

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
				s = list_add( tokens, &tok );
				if ( !s ) { p = NULL; goto outerr; }
				newstr_empty( &tok );
			}
		} else if ( *p=='#' && !nquotes && !nbracket ) {
			if ( tok.len ) {
				s = list_add( tokens, &tok );
				if ( !s ) { p = NULL; goto outerr; }
			}
			newstr_strcpy( &tok, "#" );
			s = list_add( tokens, &tok );
			if ( !s ) { p = NULL; goto outerr; }
			newstr_empty( &tok );
		} else if ( *p=='{' && !nquotes && ( p==startp || *(p-1)!='\\' ) ) {
			nbracket++;
			newstr_addchar( &tok, *p );
		} else if ( *p=='}' && !nquotes && ( p==startp || *(p-1)!='\\' ) ) {
			nbracket--;
			newstr_addchar( &tok, *p );
			if ( nbracket==0 ) {
				s = list_add( tokens, &tok );
				if ( !s ) { p = NULL; goto outerr; }
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
				s = list_add( tokens, &tok );
				if ( !s ) { p = NULL; goto outerr; }
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
		s = list_add( tokens, &tok );
		if ( !s ) p = NULL;
	}
outerr:
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
	int i, status;
	newstr *s, *t;
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

static char *
process_biblatexline( char *p, newstr *tag, newstr *data, uchar stripquotes )
{
	int i, status;
	list tokens;
	newstr *s;

	newstr_empty( data );

	p = biblatex_tag( p, tag );
	if ( tag->len==0 ) return p;

	list_init( &tokens );

	if ( *p=='=' ) p = biblatex_data( p+1, NULL, &tokens );

	replace_strings( &tokens, NULL );

	status = string_concatenate( &tokens, NULL );
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

static int
process_cite( fields *bibin, char *p, char *filename, long nref )
{
	int fstatus, status = BIBL_OK;
	newstr tag, data;
	newstrs_init( &tag, &data, NULL );
	p = process_biblatextype( p, &data );
	if ( data.len ) {
		fstatus = fields_add( bibin, "INTERNAL_TYPE", data.data, 0 );
		if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
	}
	p = process_biblatexid ( p, &data );
	if ( data.len ) {
		fstatus = fields_add( bibin, "REFNUM", data.data, 0 );
		if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
	}
	while ( *p ) {
		p = process_biblatexline( p, &tag, &data, 1 );
		if ( !p ) { status = BIBL_ERR_MEMERR; goto out; }
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
	newstr s1, s2, *s;
	newstrs_init( &s1, &s2, NULL );
	while ( *p && *p!='{' && *p!='(' ) p++;
	if ( *p=='{' || *p=='(' ) p++;
	p = process_biblatexline( skip_ws( p ), &s1, &s2, 0 );
	if ( s2.data ) {
		newstr_findreplace( &s2, "\\ ", " " );
		if ( newstr_memerr( &s2 ) ) { status = BIBL_ERR_MEMERR; goto out; }
	}
	if ( s1.data ) {
		n = list_find( &find, s1.data );
		if ( n==-1 ) {
			s = list_add( &find, &s1 );
			if ( s==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
			if ( s2.data ) s = list_add( &replace, &s2 );
			else s = list_addc( &replace, "" );
			if ( s==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		} else {
			if ( s2.data ) s = list_set( &replace, n, &s2 );
			else s = list_setc( &replace, n, "" );
			if ( s==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		}
	}
out:
	newstrs_free( &s1, &s2, NULL );
	return status;
}

int
biblatexin_processf( fields *bibin, char *data, char *filename, long nref )
{
	if ( !strncasecmp( data, "@STRING", 7 ) ) {
		process_string( data+7 );
		return 0;
        } else {
		process_cite( bibin, data, filename, nref );
		return 1;
	}
}

/*****************************************************
 PUBLIC: void biblatexin_cleanf()
*****************************************************/

static void
biblatex_process_tilde( newstr *s )
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
biblatex_process_bracket( newstr *s )
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

static int
biblatex_cleantoken( newstr *s )
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

	/* Other text annotations */
	newstr_findreplace( s, "\\it ", "" );
	newstr_findreplace( s, "\\em ", "" );

	newstr_findreplace( s, "\\%", "%" );
	newstr_findreplace( s, "\\$", "$" );
	while ( newstr_findreplace( s, "  ", " " ) ) {}

	/* 'textcomp' annotations that we don't want to substitute on output*/
	newstr_findreplace( s, "\\textdollar", "$" );
	newstr_findreplace( s, "\\textunderscore", "_" );

	biblatex_process_bracket( s );
	biblatex_process_tilde( s );

	if ( !newstr_memerr( s ) ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

static int
biblatex_split( list *tokens, newstr *s )
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
			if ( newstr_memerr( &tok ) ) { status = BIBL_ERR_MEMERR; goto out; }
			if ( tok.len ) {
				t = list_add( tokens, &tok );
				if ( !t ) { status = BIBL_ERR_MEMERR; goto out; }
			}
			newstr_empty( &tok );
		}
	}
	if ( tok.len ) {
		if ( newstr_memerr( &tok ) ) { status = BIBL_ERR_MEMERR; goto out; }
		t = list_add( tokens, &tok );
		if ( !t ) { status = BIBL_ERR_MEMERR; goto out; }
	}

	for ( i=0; i<tokens->n; ++i ) {
		t = list_get( tokens, i );
		newstr_trimstartingws( t );
		newstr_trimendingws( t );
		if ( newstr_memerr( t ) ) { status = BIBL_ERR_MEMERR; goto out; }
	}
out:
	newstr_free( &tok );
	return status;
}

static int
biblatexin_addtitleurl( fields *info, newstr *in )
{
	int fstatus, status = BIBL_OK;
	newstr s;
	char *p;

	newstr_init( &s );
	/* skip past "\href{" */
	p = newstr_cpytodelim( &s, in->data + 6, "}", 1 );
	if ( newstr_memerr( &s ) ) {
		status = BIBL_ERR_MEMERR;
		goto out;
	}
	fstatus = fields_add( info, "URL", s.data, 0 );
	if ( fstatus!=FIELDS_OK ) {
		status = BIBL_ERR_MEMERR;
		goto out;
	}

	p = newstr_cpytodelim( &s, p, "", 0 );
	if ( newstr_memerr( &s ) ) {
		status = BIBL_ERR_MEMERR;
		goto out;
	}
	newstr_swapstrings( &s, in );
out:
	newstr_free( &s );
	return status;
}

static int
is_name_tag( newstr *tag )
{
	if ( tag->len ) {
		if ( !strcasecmp( tag->data, "author" ) ) return 1;
		if ( !strcasecmp( tag->data, "editor" ) ) return 1;
		if ( !strcasecmp( tag->data, "editorb" ) ) return 1;
		if ( !strcasecmp( tag->data, "editorc" ) ) return 1;
		if ( !strcasecmp( tag->data, "director" ) ) return 1;
		if ( !strcasecmp( tag->data, "producer" ) ) return 1;
		if ( !strcasecmp( tag->data, "execproducer" ) ) return 1;
		if ( !strcasecmp( tag->data, "writer" ) ) return 1;
		if ( !strcasecmp( tag->data, "redactor" ) ) return 1;
		if ( !strcasecmp( tag->data, "annotator" ) ) return 1;
		if ( !strcasecmp( tag->data, "commentator" ) ) return 1;
		if ( !strcasecmp( tag->data, "translator" ) ) return 1;
		if ( !strcasecmp( tag->data, "foreword" ) ) return 1;
		if ( !strcasecmp( tag->data, "afterword" ) ) return 1;
		if ( !strcasecmp( tag->data, "introduction" ) ) return 1;
	}
	return 0;
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
biblatexin_cleandata( newstr *tag, newstr *s, fields *info, param *p )
{
	list tokens;
	newstr *tok;
	int i, status = BIBL_OK;
	if ( !s->len ) return status;
	/* protect url from undergoing any parsing */
	if ( is_url_tag( tag ) ) return status;
	list_init( &tokens );
	biblatex_split( &tokens, s );
	for ( i=0; i<tokens.n; ++i ) {
		if (!strncasecmp(tokens.str[i].data,"\\href{", 6)) {
			status = biblatexin_addtitleurl( info, &(tokens.str[i]) );
			if ( status!=BIBL_OK ) goto out;
		}
		if ( p && p->latexin && !is_name_tag( tag ) ) {
			status = biblatex_cleantoken( &(tokens.str[i]) );
			if ( status!=BIBL_OK ) goto out;
		}
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

static int
biblatexin_crossref_oneref( fields *ref, fields *cross )
{
	int j, nl, ntype, fstatus;
	char *type, *nt, *nd;
	ntype = fields_find( ref, "INTERNAL_TYPE", -1 );
	type = ( char * ) fields_value( ref, ntype, FIELDS_CHRP_NOUSE );
	for ( j=0; j<cross->n; ++j ) {
		nt = ( char * ) fields_tag( cross, j, FIELDS_CHRP_NOUSE );
		if ( !strcasecmp( nt, "INTERNAL_TYPE" ) ) continue;
		if ( !strcasecmp( nt, "REFNUM" ) ) continue;
		if ( !strcasecmp( nt, "TITLE" ) ) {
			if ( !strcasecmp( type, "Inproceedings" ) ||
			     !strcasecmp( type, "Incollection" ) )
				nt = "booktitle";
		}
		nd = ( char * ) fields_value( cross, j, FIELDS_CHRP_NOUSE );
		nl = fields_level( cross, j ) + 1;
		fstatus = fields_add( ref, nt, nd, nl );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}
	return BIBL_OK;
}

static int
biblatexin_crossref( bibl *bin, param *p )
{
	int n, ncross, status = BIBL_OK;
	fields *ref, *cross;
	long i;
        for ( i=0; i<bin->nrefs; ++i ) {
		ref = bin->ref[i];
		n = fields_find( ref, "CROSSREF", -1 );
		if ( n==-1 ) continue;
		fields_setused( ref, n );
		ncross = biblatexin_findref(bin, (char*)fields_value(ref,n, FIELDS_CHRP_NOUSE));
		if ( ncross==-1 ) {
			biblatexin_nocrossref( bin, i, n, p );
			continue;
		}
		cross = bin->ref[ncross];
		status = biblatexin_crossref_oneref( ref, cross );
		if ( status!=BIBL_OK ) return status;
	}
	return status;
}

static int
biblatexin_cleanref( fields *bibin, param *p )
{
	int i, n, status;
	newstr *t, *d;
	n = fields_num( bibin );
	for ( i=0; i<n; ++i ) {
		t = fields_tag( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		status = biblatexin_cleandata( t, d, bibin, p );
		if ( status!=BIBL_OK ) return status;
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
	return BIBL_OK;
}

int
biblatexin_cleanf( bibl *bin, param *p )
{
	int status;
	long i;
        for ( i=0; i<bin->nrefs; ++i ) {
		status = biblatexin_cleanref( bin->ref[i], p );
		if ( status!=BIBL_OK ) return status;
	}
	status = biblatexin_crossref( bin, p );
	return status;
}

/*****************************************************
 PUBLIC: void biblatexin_typef()
*****************************************************/

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

/*****************************************************
 PUBLIC: int biblatexin_convertf(), returns BIBL_OK or BIBL_ERR_MEMERR
*****************************************************/

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
	if ( s->len==0 ) return status;

	newstr_init( &page );
	p = skip_ws( s->data );
	while ( *p && !is_ws(*p) && *p!='-' && *p!=-30 )
		newstr_addchar( &page, *p++ );
	if ( page.len>0 ) {
		fstatus = fields_add( info, "PAGESTART", page.data, level );
		if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
	}

	while ( *p && (is_ws(*p) || *p=='-' ) ) p++;
	if ( *p && is_utf8_emdash( p ) ) p+=3;
	if ( *p && is_utf8_endash( p ) ) p+=3;

	newstr_empty( &page );
	while ( *p && !is_ws(*p) && *p!='-' && *p!=-30 )
		newstr_addchar( &page, *p++ );
	if ( page.len>0 ) {
		fstatus = fields_add( info, "PAGEEND", page.data, level );
		if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
	}
out:
	newstr_free( &page );
	return status;
}

static int
process_url( fields *info, char *p, int level )
{
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
		fstatus = fields_add( info, "URL", p, level );
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}


/* process_howpublished()
 *
 *    howpublished={},
 *
 * Normally indicates the manner in which something was
 * published in lieu of a formal publisher, so typically
 * 'howpublished' and 'publisher' will never be in the
 * same reference.
 *
 * Occasionally, people put Diploma thesis information
 * into this field, so check for that first.
 */
static int
process_howpublished( fields *info, char *p, int level )
{
	int fstatus;
        if ( !strncasecmp( p, "Diplom", 6 ) )
                fstatus = fields_replace_or_add( info, "NGENRE", "Diploma thesis", level );
        else if ( !strncasecmp( p, "Habilitation", 13 ) )
                fstatus = fields_replace_or_add( info, "NGENRE", "Habilitation thesis", level );
        else
		fstatus = fields_add( info, "PUBLISHER", p, level );
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

static int
process_thesistype( fields *info, char *p, int level )
{
	int fstatus = FIELDS_OK;
	/* type in the @thesis is used to distinguish Ph.D. and Master's thesis */
	if ( !strncasecmp( p, "phdthesis", 9 ) ) {
		fstatus = fields_replace_or_add( info, "NGENRE", "Ph.D. thesis", level );
	} else if ( !strncasecmp( p, "mastersthesis", 13 ) || !strncasecmp( p, "masterthesis", 12 ) ) {
		fstatus = fields_replace_or_add( info, "NGENRE", "Masters thesis", level );
	} else if ( !strncasecmp( p, "mathesis", 8 ) ) {
		fstatus = fields_replace_or_add( info, "NGENRE", "Masters thesis", level );
	} else if ( !strncasecmp( p, "diploma", 7 ) ) {
		fstatus = fields_replace_or_add( info, "NGENRE", "Diploma thesis", level );
	} else if ( !strncasecmp( p, "habilitation", 12 ) ) {
		fstatus = fields_replace_or_add( info, "NGENRE", "Habilitation thesis", level );
	}
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

/* biblatex drops school field if institution is present */
static int
process_school( fields *bibin, fields *info, char *tag, char *value, int level )
{
	int fstatus;
	if ( fields_find( bibin, "institution", LEVEL_ANY ) != -1 )
		return BIBL_OK;
	else {
		fstatus = fields_add( info, tag, value, level );
		if ( fstatus==FIELDS_OK ) return BIBL_OK;
		else return BIBL_ERR_MEMERR;
	}
}

/* biblatex drops school field if institution is present */
static int
process_subtype( fields *bibin, fields *info, char *tag, char *value, int level )
{
	int fstatus;
	if ( !strcasecmp( value, "magazine" ) ) {
		fstatus = fields_add( info, "NGENRE", "magazine article", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		fstatus = fields_add( info, "NGENRE", "magazine", LEVEL_HOST );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	} else if ( !strcasecmp( value, "newspaper" ) ) {
		fstatus = fields_add( info, "NGENRE", "newspaper article", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		fstatus = fields_add( info, "GENRE", "newspaper", LEVEL_HOST );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}
	return BIBL_OK;
}

static int
process_eprint( fields *bibin, fields *info, int level )
{
	int neprint, netype, fstatus;
	char *eprint = NULL, *etype = NULL;
	neprint = fields_find( bibin, "eprint", -1 );
	netype  = fields_find( bibin, "eprinttype", -1 );
	if ( neprint!=-1 ) eprint = bibin->data[neprint].data;
	if ( netype!=-1 ) etype = bibin->data[netype].data;
	if ( eprint && etype ) {
		if ( !strncasecmp( etype, "arxiv", 5 ) ) {
			fstatus = fields_add( info, "ARXIV", eprint, level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		} else if ( !strncasecmp( etype, "jstor", 5 ) ) {
			fstatus = fields_add( info, "JSTOR", eprint, level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		} else if ( !strncasecmp( etype, "pubmed", 6 ) ) {
			fstatus = fields_add( info, "PMID", eprint, level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		} else if ( !strncasecmp( etype, "medline", 7 ) ) {
			fstatus = fields_add( info, "MEDLINE", eprint, level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		} else {
			fstatus = fields_add( info, "EPRINT", eprint, level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
			fstatus = fields_add( info, "EPRINTTYPE", etype, level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
		fields_setused( bibin, neprint );
		fields_setused( bibin, netype );
	} else if ( eprint ) {
		fstatus = fields_add( info, "EPRINT", eprint, level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		fields_setused( bibin, neprint );
	} else if ( etype ) {
		fstatus = fields_add( info, "EPRINTTYPE", etype, level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		fields_setused( bibin, netype );
	}
	return BIBL_OK;
}

static void
report( fields *f )
{
	int i, n;
	n = fields_num( f );
	for ( i=0; i<n; ++i )
		fprintf(stderr, "%d '%s' = '%s'\n",
			fields_level( f, i ),
			(char*)fields_tag( f, i, FIELDS_CHRP_NOUSE ),
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

/* get_title_elements()
 *
 * find all of the biblatex title elements for the current level
 *    internal "TITLE"      -> "title", "booktitle", "maintitle"
 *    internal "SUBTITLE"   -> "subtitle", "booksubtitle", "mainsubtitle"
 *    internal "TITLEADDON" -> "titleaddon", "booktitleaddon", "maintitleaddon"
 *
 * place in ttl, subttl, and ttladdon strings
 *
 * return 1 if an element is found, 0 if not
 */
static int
get_title_elements( fields *bibin, int currlevel, int reftype, variants *all, int nall,
	newstr *ttl, newstr *subttl, newstr *ttladdon )
{
	int nfields, process, level, i, n;
	newstr *t, *d;
	char *newtag;

	newstrs_empty( ttl, subttl, ttladdon, NULL );

	nfields = fields_num( bibin );

	for ( i=0; i<nfields; ++i ) {

		/* ...skip already used titles */
		if ( fields_used( bibin, i ) ) continue;

		/* ...skip empty elements */
		t = fields_tag  ( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		if ( d->len == 0 ) continue;

		n = translate_oldtag( t->data, reftype, all, nall, &process, &level, &newtag );
		if ( n==-1 ) continue;
		if ( process != TITLE ) continue;
		if ( level != currlevel ) continue;

		fields_setused( bibin, i );

		if ( !strcasecmp( newtag, "TITLE" ) ) {
			if ( ttl->len ) newstr_addchar( ttl, ' ' );
			newstr_newstrcat( ttl, d );
		} else if ( !strcasecmp( newtag, "SUBTITLE" ) ) {
			if ( subttl->len ) newstr_addchar( subttl, ' ' );
			newstr_newstrcat( subttl, d );
		} else if ( !strcasecmp( newtag, "TITLEADDON" ) ) {
			if ( ttladdon->len ) newstr_addchar( ttladdon, ' ' );
			newstr_newstrcat( ttladdon, d );
		}
	}

	return ( ttl->len>0 || subttl->len > 0 || ttladdon->len > 0 );
}

/* attach_addon()
 *
 * Add titleaddon to the title.
 */
static void
attach_addon( newstr *title, newstr *addon )
{
	if ( title->len ) {
		if ( title->data[title->len-1]!='.' )
			newstr_addchar( title, '.' );
		newstr_addchar( title, ' ' );
	}
	newstr_newstrcat( title, addon );
}

static int
process_combined_title( fields *info, newstr *ttl, newstr *subttl, newstr *ttladdon, int currlevel )
{
	int fstatus, status = BIBL_OK;
	newstr combined;

	newstr_init( &combined );
	newstr_newstrcpy( &combined, ttl );
	if ( subttl->len ) {
		if ( combined.len && combined.data[combined.len-1]!=':' && combined.data[combined.len-1]!='?' )
			newstr_addchar( &combined, ':' );
		newstr_addchar( &combined, ' ' );
		newstr_newstrcat( &combined, subttl );
	}
	if ( ttladdon->len ) attach_addon( &combined, ttladdon );
	if ( newstr_memerr( &combined ) ) { status = BIBL_ERR_MEMERR; goto out; }
	fstatus = fields_add( info, "TITLE", combined.data, currlevel );
	if ( fstatus==FIELDS_OK ) status = BIBL_ERR_MEMERR;
out:
	newstr_free( &combined );
	return status;
}

static int
process_separated_title( fields *info, newstr *ttl, newstr *subttl, newstr *ttladdon, int currlevel )
{
	int fstatus;
	if ( ttladdon->len ) {
		if ( subttl->len ) attach_addon( subttl, ttladdon );
		else attach_addon( ttl, ttladdon );
	}
	if ( ttl->len ) {
		fstatus = fields_add( info, "TITLE", ttl->data, currlevel );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}
	if ( subttl->len ) {
		fstatus = fields_add( info, "SUBTITLE", subttl->data, currlevel );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}
	return BIBL_OK;
}

static int
process_title_all( fields *bibin, fields *info, int reftype, param *p,
	variants *all, int nall )
{
	int currlevel, found, status = BIBL_OK;
	newstr ttl, subttl, ttladdon;
	newstrs_init( &ttl, &subttl, &ttladdon, NULL );
	for ( currlevel = 0; currlevel<LEVEL_SERIES+2; currlevel++ ) {
		found = get_title_elements( bibin, currlevel, reftype, all, nall,
				&ttl, &subttl, &ttladdon );
		if ( !found ) continue;
		if ( p->nosplittitle )
			status = process_combined_title( info, &ttl, &subttl, &ttladdon, currlevel );
		else
			status = process_separated_title( info, &ttl, &subttl, &ttladdon, currlevel );
		if ( status!=BIBL_OK ) goto out;
	}
out:
	newstrs_free( &ttl, &subttl, &ttladdon, NULL );
	return status;
}


static int
biblatex_matches_list( fields *info, char *tag, char *suffix, newstr *data, int level,
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

static int
biblatex_names( fields *info, char *tag, newstr *data, int level, list *asis, list *corps )
{
	int begin, end, ok, n, etal, i, match, status = BIBL_OK;
	list tokens;

	/* If we match the asis or corps list add and bail. */
	status = biblatex_matches_list( info, tag, ":ASIS", data, level, asis, &match );
	if ( match==1 || status!=BIBL_OK ) return status;
	status = biblatex_matches_list( info, tag, ":CORP", data, level, corps, &match );
	if ( match==1 || status!=BIBL_OK ) return status;

	list_init( &tokens );

	biblatex_split( &tokens, data );
	for ( i=0; i<tokens.n; ++i )
		biblatex_cleantoken( list_get( &tokens, i ) );

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

		/* Handle repeated 'and' errors */
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

/*
 * biblatex has multiple editor fields "editor", "editora", "editorb", "editorc",
 * each of which can be modified from a type of "EDITOR" via "editortype",
 * "editoratype", "editorbtype", "editorctype".
 *
 * Defined types:
 *     "editor"
 *     "collaborator"
 *     "compiler"
 *     "redactor"
 *
 *     "reviser" ?
 *     "founder" ?
 *     "continuator" ?
 *
 *  bibtex-chicago
 *
 *     "director"
 *     "producer"
 *     "conductor"
 *     "none" (for performer)
 */
static int
process_editor( fields *bibin, fields *info, newstr *tag, newstr *value, int level, list *asis, list *corps )
{
	char *editor_fields[] = { "editor", "editora", "editorb", "editorc" };
	char *editor_types[]  = { "editortype", "editoratype", "editorbtype", "editorctype" };
	int i, n = 0, ntype, neditors = sizeof( editor_fields ) / sizeof( editor_fields[0] );
	char *type, *outtag = "EDITOR";
	for ( i=1; i<neditors; ++i )
		if ( !strcasecmp( tag->data, editor_fields[i] ) ) n = i;
	ntype = fields_find( bibin, editor_types[n], LEVEL_ANY );
	if ( ntype!=-1 ) {
		type = fields_value( bibin, ntype, FIELDS_CHRP_NOUSE );
		if ( !strcasecmp( type, "collaborator" ) )  outtag = "COLLABORATOR";
		else if ( !strcasecmp( type, "compiler" ) ) outtag = "COMPILER";
		else if ( !strcasecmp( type, "redactor" ) ) outtag = "REDACTOR";
		else if ( !strcasecmp( type, "director" ) ) outtag = "DIRECTOR";
		else if ( !strcasecmp( type, "producer" ) ) outtag = "PRODUCER";
		else if ( !strcasecmp( type, "none" ) )     outtag = "PERFORMER";
	}
	return biblatex_names( info, outtag, value, level, asis, corps );
}

static int
biblatexin_simple( fields *f, char *tag, char *value, int level )
{
	int fstatus = fields_add( f, tag, value, level );
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

int
biblatexin_convertf( fields *bibin, fields *info, int reftype, param *p,
		variants *all, int nall )
{
	int process, level, i, n, nfields, status = BIBL_OK;
	newstr *t, *d;
	char *newtag;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

               /* skip ones already "used" such as successful crossref */
                if ( fields_used( bibin, i ) ) continue;

		/* skip ones with no data or no tags (e.g. don't match ALWAYS/DEFAULT entries) */
		t = fields_tag  ( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		if ( t->len == 0 || d->len == 0 ) continue;

		n = translate_oldtag( t->data, reftype, all, nall, &process, &level, &newtag );
		if ( n==-1 ) {
			biblatexin_notag( p, t->data );
			continue;
		}

		switch ( process ) {

		case SIMPLE:
			status = biblatexin_simple( info, newtag, d->data, level );
			fields_setused( bibin, i );
			break;

		case PERSON:
			status = biblatex_names( info, newtag, d, level, &(p->asis), &(p->corps) );
			fields_setused( bibin, i );
			break;

		case BLT_EDITOR:
			status = process_editor( bibin, info, t, d, level, &(p->asis), &(p->corps) );
			fields_setused( bibin, i );
			break;

		case PAGES:
			status = process_pages( info, d, level);
			fields_setused( bibin, i );
			break;

		case HOWPUBLISHED:
			status = process_howpublished( info, d->data, level );
			fields_setused( bibin, i );
			break;

		case BT_URL:
			status = process_url( info, d->data, level );
			fields_setused( bibin, i );
			break;

		case BT_GENRE:
			status = biblatexin_simple( info, "NGENRE", d->data, level );
			fields_setused( bibin, i );
			break;

		case BT_EPRINT:
			status = process_eprint( bibin, info, level );
			fields_setused( bibin, i );
			break;

		case BLT_THESIS_TYPE:
			status = process_thesistype( info, d->data, level );
			fields_setused( bibin, i );
			break;

		case BLT_SCHOOL:
			status = process_school( bibin, info, newtag, d->data, level );
			fields_setused( bibin, i );
			break;

		case BLT_SUBTYPE:
			status = process_subtype( bibin, info, newtag, d->data, level );
			fields_setused( bibin, i );
			break;

		case BLT_SKIP:
			status = BIBL_OK;
			fields_setused( bibin, i );
			break;

		case TITLE:
			status = BIBL_OK; /* delay title processing until later */
			break;

		default:
			status = BIBL_OK;
			break;

		}
		if ( status!=BIBL_OK ) return status;

	}

	status = process_title_all( bibin, info, reftype, p, all, nall );

	if ( status==BIBL_OK && p->verbose ) report( info );

	return status;
}

