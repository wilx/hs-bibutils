/*
 * xml.h
 *
 * Copyright (c) Chris Putnam 2004-2017
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef XML_H
#define XML_H

#include "slist.h"
#include "str.h"

typedef struct xml_attrib {
	slist attrib;
	slist value;
} xml_attrib;

typedef struct xml {
	str *tag;
	str *value;
	xml_attrib *a;
	struct xml *down;
	struct xml *next;
} xml;

void     xml_init        ( xml *x );
void     xml_free        ( xml *x );
str *    xml_getattrib   ( xml *node, char *attrib );
char *   xml_findstart   ( char *buffer, char *tag );
char *   xml_findend     ( char *buffer, char *tag );
int      xml_tagexact    ( xml *node, char *s );
int      xml_tag_attrib  ( xml *node, char *s, char *attrib, char *value );
int      xml_hasattrib   ( xml *node, char *attrib, char *value );
char *   xml_tree        ( char *p, xml *onode );
int      xml_hasvalue    ( xml *node );
char *   xml_value       ( xml *node );
char *   xml_tag         ( xml *node );
int      xml_tagwithvalue( xml *node, char *tag );

char *   xml_pns; /* global Namespace */

#endif

