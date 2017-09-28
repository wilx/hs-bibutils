/*
 * xml.c
 *
 * Copyright (c) Chris Putnam 2004-2017
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "is_ws.h"
#include "strsearch.h"
#include "xml.h"

char *xml_pns = NULL;

static xml_attrib *
xmlattrib_new( void )
{
	xml_attrib *a = (xml_attrib *) malloc( sizeof( xml_attrib ) );
	if ( a ) {
		slist_init( &(a->attrib) );
		slist_init( &(a->value) );
	}
	return a;
}

static void
xmlattrib_add( xml_attrib *a, char *attrib, char *value  )
{
	if ( attrib ) slist_addc( &(a->attrib), attrib );
	else slist_addc( &(a->attrib), "" );
	if ( value ) slist_addc( &(a->value), value );
	else slist_addc( &(a->value), "" );
}

static void
xmlattrib_free( xml_attrib *a )
{
	slist_free( &(a->attrib) );
	slist_free( &(a->value ) );
}

static xml *
xml_new( void )
{
	xml *x = ( xml * ) malloc( sizeof( xml ) );
	if ( x ) xml_init( x );
	return x;
}

void
xml_free( xml *x )
{
	if ( x->tag ) {
		str_free( x->tag );
		free( x->tag );
	}
	if ( x->value ) {
		str_free( x->value );
		free( x->value );
	}
	if ( x->a ) {
		xmlattrib_free( x->a );
		free( x->a );
	}
	if ( x->down ) xml_free( x->down );
	if ( x->next ) xml_free( x->next );
}

void
xml_init( xml *x )
{
	x->tag = str_new();
	x->value = str_new();
	x->a = NULL;
	x->down = NULL;
	x->next = NULL;
	if ( !(x->tag) || !(x->value) ) {
		fprintf(stderr,"xml_init: memory error.\n");
		exit( EXIT_FAILURE );
	}
}

enum {
	XML_DESCRIPTOR,
	XML_COMMENT,
	XML_OPEN,
	XML_CLOSE,
	XML_OPENCLOSE
};

static int
xml_terminator( char *p, int *type )
{
	if ( *p=='>' ) {
		return 1;
	} else if ( *p=='/' && *(p+1)=='>' ) {
		if ( *type==XML_OPENCLOSE ) return 1;
		else if ( *type==XML_OPEN ) {
			*type = XML_OPENCLOSE;
			return 1;
		}
	} else if ( *p=='?' && *(p+1)=='>' && *type==XML_DESCRIPTOR ) {
		return 1;
	} else if ( *p=='!' && *(p+1)=='>' && *type==XML_COMMENT ) {
		return 1;
	}
	return 0;
}

static char *
xml_processattrib( char *p, xml_attrib **ap, int *type )
{
	xml_attrib *a = NULL;
	char quote_character = '\"';
	int inquotes = 0;
	str aname, aval;
	str_init( &aname );
	str_init( &aval );
	while ( *p && !xml_terminator(p,type) ) {
		/* get attribute name */
		while ( *p==' ' || *p=='\t' ) p++;
		while ( *p && !strchr( "= \t", *p ) && !xml_terminator(p,type)){
			str_addchar( &aname, *p );
			p++;
		}
		while ( *p==' ' || *p=='\t' ) p++;
		if ( *p=='=' ) p++;
		/* get attribute value */
		while ( *p==' ' || *p=='\t' ) p++;
		if ( *p=='\"' || *p=='\'' ) {
			if ( *p=='\'' ) quote_character = *p;
			inquotes=1;
			p++;
		}
		while ( *p && ((!xml_terminator(p,type) && !strchr("= \t", *p ))||inquotes)){
			if ( *p==quote_character ) inquotes=0;
			else str_addchar( &aval, *p );
			p++;
		}
		if ( str_has_value( &aname ) ) {
			if ( !a ) a = xmlattrib_new();
			xmlattrib_add( a, str_cstr( &aname ), str_cstr( &aval ) );
		}
		str_empty( &aname );
		str_empty( &aval );
	}
	str_free( &aname );
	str_free( &aval );
	*ap = a;
	return p;
}

/*
 * xml_processtag
 *
 *      XML_COMMENT   <!-- ....  -->
 * 	XML_DESCRIPTOR   <?.....>
 * 	XML_OPEN      <A>
 * 	XML_CLOSE     </A>
 * 	XML_OPENCLOSE <A/>
 */
static char *
xml_processtag( char *p, str *tag, xml_attrib **attrib, int *type )
{
	*attrib = NULL;
	if ( *p=='<' ) p++;
	if ( *p=='!' ) {
		while ( *p && *p!='>' ) str_addchar( tag, *p++ );
		*type = XML_COMMENT;
	} else if ( *p=='?' ) {
		*type = XML_DESCRIPTOR;
		p++; /* skip '?' */
		while ( *p && !strchr( " \t", *p ) && !xml_terminator(p,type) )
			str_addchar( tag, *p++ );
		if ( *p==' ' || *p=='\t' )
			p = xml_processattrib( p, attrib, type );
	} else if ( *p=='/' ) {
		while ( *p && !strchr( " \t", *p ) && !xml_terminator(p,type) )
			str_addchar( tag, *p++ );
		*type = XML_CLOSE;
		if ( *p==' ' || *p=='\t' ) 
			p = xml_processattrib( p, attrib, type );
	} else {
		*type = XML_OPEN;
		while ( *p && !strchr( " \t", *p ) && !xml_terminator(p,type) )
			str_addchar( tag, *p++ );
		if ( *p==' ' || *p=='\t' ) 
			p = xml_processattrib( p, attrib, type );
	}
	while ( *p && *p!='>' ) p++;
	if ( *p=='>' ) p++;
	return p;
}

static void
xml_appendnode( xml *onode, xml *nnode )
{
	if ( !onode->down ) onode->down = nnode;
	else {
		xml *p = onode->down;
		while ( p->next ) p = p->next;
		p->next = nnode;
	}
}

char *
xml_tree( char *p, xml *onode )
{
	str tag;
	xml_attrib *attrib;
	int type, is_style = 0;

	str_init( &tag );

	while ( *p ) {
		/* retain white space for <style> tags in endnote xml */
		if ( onode->tag && str_cstr( onode->tag ) &&
			!strcasecmp( str_cstr( onode->tag ),"style") ) is_style=1;
		while ( *p && *p!='<' ) {
			if ( onode->value->len>0 || is_style || !is_ws( *p ) )
				str_addchar( onode->value, *p );
			p++;
		}
		if ( *p=='<' ) {
			str_empty( &tag );
			p = xml_processtag( p, &tag, &attrib, &type );
			if ( type==XML_OPEN || type==XML_OPENCLOSE ||
			     type==XML_DESCRIPTOR ) {
				xml *nnode = xml_new();
				str_strcpy( nnode->tag, &tag );
				nnode->a = attrib;
				xml_appendnode( onode, nnode );
				if ( type==XML_OPEN )
					p = xml_tree( p, nnode );
			} else if ( type==XML_CLOSE ) {
				/*check to see if it's closing for this one*/
				goto out; /* assume it's right for now */
			}
		}
	}
out:
	str_free( &tag );
	return p;
}

void
xml_draw( xml *x, int n )
{
	int i,j;
	if ( !x ) return;
	for ( i=0; i<n; ++i ) printf( "    " );
	printf("n=%d tag='%s' value='%s'\n", n, str_cstr( x->tag ), str_cstr( x->value ) );
	if ( x->a ) {
		for ( j=0; j<x->a->value.n; ++j ) {
			for ( i=0; i<n; ++i ) printf( "    " );
			printf("    attrib='%s' value='%s'\n",
				slist_cstr( &(x->a)->attrib, j ),
				slist_cstr( &(x->a)->attrib, j ) );
		}
	}
	if ( x->down ) xml_draw( x->down, n+1 );
	if ( x->next ) xml_draw( x->next, n );
}

char *
xml_findstart( char *buffer, char *tag )
{
	str starttag;
	char *p;

	str_init( &starttag );
	str_addchar( &starttag, '<' );
	str_strcatc( &starttag, tag );
	str_addchar( &starttag, ' ' );
	p = strsearch( buffer, str_cstr( &starttag ) );

	if ( !p ) {
		starttag.data[ starttag.len-1 ] = '>';
		p = strsearch( buffer, str_cstr( &starttag ) );
	}

	str_free( &starttag );
	return p;
}

char *
xml_findend( char *buffer, char *tag )
{
	str endtag;
	char *p;

	str_init( &endtag );
	str_strcpyc( &endtag, "</" );
	if ( xml_pns ) {
		str_strcatc( &endtag, xml_pns );
		str_addchar( &endtag, ':' );
	}
	str_strcatc( &endtag, tag );
	str_addchar( &endtag, '>' );

	p = strsearch( buffer, str_cstr( &endtag ) );

	if ( p && *p ) {
		if ( *p ) p++;  /* skip <random_tag></end> combo */
		while ( *p && *(p-1)!='>' ) p++;
	}

	str_free( &endtag );
	return p;
}

static int
xml_tagexact_simple( xml* node, char *s )
{
	if ( node->tag->len!=strlen( s ) ) return 0;
	if ( strcasecmp( str_cstr( node->tag ), s ) ) return 0;
	return 1;
}
static int
xml_tagexact_pns( xml* node, char *s )
{
	int found = 0;
	str tag;

	str_init( &tag );
	str_strcpyc( &tag, xml_pns );
	str_addchar( &tag, ':' );
	str_strcatc( &tag, s );
	if ( node->tag->len==tag.len &&
			!strcasecmp( str_cstr( node->tag ), str_cstr( &tag ) ) )
		found = 1;
	str_free( &tag );

	return found;
}
int
xml_tagexact( xml *node, char *s )
{
	if ( xml_pns ) return xml_tagexact_pns( node, s );
	else           return xml_tagexact_simple( node, s );
}

int
xml_hasattrib( xml *node, char *attrib, char *value )
{
	xml_attrib *na = node->a;
	char *a, *v;
	int i;

	if ( na ) {

		for ( i=0; i<na->attrib.n; ++i ) {
			a = slist_cstr( &(na->attrib), i );
			v = slist_cstr( &(na->value),  i );
			if ( !a || !v ) continue;
			if ( !strcasecmp( a, attrib ) && !strcasecmp( v, value ) )
				return 1;
		}

	}

	return 0;
}

int
xml_tag_attrib( xml *node, char *s, char *attrib, char *value )
{
	if ( !xml_tagexact( node, s ) ) return 0;
	return xml_hasattrib( node, attrib, value );
}

str *
xml_getattrib( xml *node, char *attrib )
{
	xml_attrib *na = node->a;
	str *ns = NULL;
	int i;

	if ( na ) {
		for ( i=0; i<na->attrib.n; ++i )
			if ( !strcasecmp( slist_cstr( &(na->attrib), i ), attrib ) )
				ns = slist_str( &(na->value), i );
	}
	return ns;
}

int
xml_hasvalue( xml *node )
{
	if ( node && node->value && str_cstr( node->value ) ) return 1;
	return 0;
}

char *
xml_tag( xml *node )
{
	return str_cstr( node->tag );
}

char *
xml_value( xml *node )
{
	return str_cstr( node->value );
}

int
xml_tagwithvalue( xml *node, char *tag )
{
	if ( !xml_hasvalue( node ) ) return 0;
	return xml_tagexact( node, tag );
}
