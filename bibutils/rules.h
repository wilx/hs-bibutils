/*
 * rules.h
 *
 * Copyright (c) Chris Putnam 2005-2010
 *
 * Source code released under the GPL
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "bibutils.h"

/* internal includes */
#include "reftypes.h"
#include "bibtexin.h"
#include "biblatexin.h"
#include "bibtexout.h"
#include "copacin.h"
#include "endin.h"
#include "endxmlin.h"
#include "endout.h"
#include "isiin.h"
#include "isiout.h"
#include "medin.h"
#include "modsin.h"
#include "risin.h"
#include "wordin.h"
#include "risout.h"
#include "modsout.h"
#include "wordout.h"
#include "adsout.h"
#include "newstr_conv.h"
#include "is_ws.h"

#include "ebiin.h"

typedef struct convert_rules {
	int  (*readf)(FILE*,char*,int,int*,newstr*,newstr*,int*);
	int  (*processf)(fields*,char*,char*,long);
	void (*cleanf)(bibl*,param*);
	int  (*typef) (fields*,char*,int,param*,variants*,int);
	void (*convertf)(fields*,fields*,int,param*,variants*,int);
	void (*headerf)(FILE*,param*);
	void (*footerf)(FILE*);
	void (*writef)(fields*,FILE*,param*,unsigned long);
	variants *all;
	int  nall;
} convert_rules;

