/*
 * marc_auth.c
 *
 * Copyright (c) Chris Putnam 2004-2018
 *
 * Source code released under the GPL version 2
 *
 */
#include "marc_auth.h"
#include <string.h>

static const char *marc_genre[] = {
	"abstract or summary",
	"art original",
	"art reproduction",
	"article",
	"atlas",
	"autobiography",
	"bibliography",
	"biography",
	"book",
	"calendar",
	"catalog",
	"chart",
	"comic or graphic novel",
	"comic strip",
	"conference publication",
	"database",
	"dictionary",
	"diorama",
	"directory",
	"discography",
	"drama",
	"encyclopedia",
	"essay",
	"festschrift",
	"fiction",
	"filmography",
	"filmstrip",
	"finding aid",
	"flash card",
	"folktale",
	"font",
	"game",
	"government publication",
	"graphic",
	"globe",
	"handbook",
	"history",
	"humor, satire",
	"hymnal",
	"index",
	"instruction",
	"interview",
	"issue",
	"journal",
	"kit",
	"language instruction",
	"law report or digest",
	"legal article",
	"legal case and case notes",
	"legislation",
	"letter",
	"loose-leaf",
	"map",
	"memoir",
	"microscope slide",
	"model",
	"motion picture",
	"multivolume monograph",
	"newspaper",
	"novel",
	"numeric data",
	"offprint",
	"online system or service",
	"patent",
	"periodical",
	"picture",
	"poetry",
	"programmed text",
	"realia",
	"rehearsal",
	"remote sensing image",
	"reporting",
	"review",
	"series",
	"short story",
	"slide",
	"sound",
	"speech",
	"standard or specification",
	"statistics",
	"survey of literature",
	"technical drawing",
	"technical report",
	"thesis",
	"toy",
	"transparency",
	"treaty",
	"videorecording",
	"web site",
	"yearbook",
};
static int nmarc_genre = sizeof( marc_genre ) / sizeof( const char* );

static const char *marc_resource[] = {
	"cartographic",
	"kit",
	"mixed material",
	"moving image",
	"notated music",
	"software, multimedia",
	"sound recording",
	"sound recording - musical",
	"sound recording - nonmusical",
	"still image",
	"text",
	"three dimensional object"
};
static int nmarc_resource = sizeof( marc_resource ) / sizeof( const char* );

static int
position_in_list( const char *list[], int nlist, const char *query )
{
	int i;
	for ( i=0; i<nlist; ++i ) {
		if ( !strcasecmp( query, list[i] ) ) return i;
	}
	return -1;
}

int
marc_findgenre( const char *query )
{
	return position_in_list( marc_genre, nmarc_genre, query );
}

int
is_marc_genre( const char *query )
{
	if ( marc_findgenre( query ) != -1 ) return 1;
	else return 0;
}

int
marc_findresource( const char *query )
{
	return position_in_list( marc_resource, nmarc_resource, query );
}

int
is_marc_resource( const char *query )
{
	if ( marc_findresource( query ) != -1 ) return 1;
	else return 0;
}
