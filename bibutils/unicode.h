
#include "newstr.h"

#define UNICODE_SYMBOL    (1)
#define UNICODE_UPPER     (2) /* Uppercase letter */
#define UNICODE_LOWER     (4) /* Lowercase letter */
#define UNICODE_NUMBER    (8) /* Numeric character */
#define UNICODE_MIXEDCASE ( UNICODE_UPPER | UNICODE_LOWER )

extern unsigned short unicode_utf8_classify( char *p );
extern unsigned short unicode_utf8_classify_newstr( newstr *s );

