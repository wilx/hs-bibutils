/* generic.c
 *
 * Copyright (c) Chris Putnam 2016-2017
 *
 * Source code released under GPL version 2
 *
 * xxxx_convertf() stubs that can be shared.
 */
#include "name.h"
#include "notes.h"
#include "pages.h"
#include "serialno.h"
#include "title.h"
#include "url.h"
#include "generic.h"

/* stub for processtypes that aren't used, such as DEFAULT and ALWAYS handled by bibcore.c  */
int
generic_null( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	return BIBL_OK;
}

int
generic_url( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	return urls_split_and_add( invalue->data, bibout, level );
}

int
generic_notes( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        if ( notes_add( bibout, invalue, level ) ) return BIBL_OK;
        else return BIBL_ERR_MEMERR;
}

int
generic_pages( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        if ( pages_add( bibout, outtag, invalue, level ) ) return BIBL_OK;
        else return BIBL_ERR_MEMERR;
}

int
generic_person( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        if ( name_add( bibout, outtag, invalue->data, level, &(pm->asis), &(pm->corps) ) ) return BIBL_OK;
        else return BIBL_ERR_MEMERR;
}

int
generic_serialno( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	if ( addsn( bibout, invalue->data, level ) ) return BIBL_OK;
	return BIBL_ERR_MEMERR;
}

/* SIMPLE = just copy */
int
generic_simple( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	if ( fields_add( bibout, outtag, invalue->data, level ) == FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

/* just like generic_null(), but useful if we need one that isn't identical to generic_null() ala biblatexin.c */
int
generic_skip( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	fields_setused( bibin, n );
	return BIBL_OK;
}

int
generic_title( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        if ( title_process( bibout, outtag, invalue->data, level, pm->nosplittitle ) ) return BIBL_OK;
        else return BIBL_ERR_MEMERR;
}


