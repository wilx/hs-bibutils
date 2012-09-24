/*
 * wordin.h
 *
 * Copyright (c) Chris Putnam 2009-2012
 *
 * Program and source code released under the GPL
 *
 */
#ifndef WORDIN_H
#define WORDIN_H

#include "newstr.h"
#include "fields.h"
#include "reftypes.h"
#include "bibutils.h"

extern int wordin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, newstr *line, newstr *reference, int *fcharset );
extern int wordin_processf( fields *wordin, char *data, char *filename, long nref );
extern void wordin_convertf( fields *wordin, fields *info, int reftype, int verbose, variants *all, int nall );

extern void wordin_initparams( param *p, const char *progname );

#endif

