/*
 * adsout.h
 *
 * Copyright (c) Richard Mathar 2007-10
 * Copyright (c) Chris Putnam 2007-10
 *
 * Program and source code released under the GPL
 *
 */
#ifndef ADSOUT_H
#define ADSOUT_H

#include <stdio.h>
#include "bibutils.h"

extern void adsout_write( fields *info, FILE *fp, param *p,
		unsigned long refnum );
extern void adsout_writeheader( FILE *outptr, param *p );

extern void adsout_initparams( param *p, const char *progname );

#endif
