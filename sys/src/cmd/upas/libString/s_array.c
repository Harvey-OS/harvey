#include <u.h>
#include <libc.h>
#include "String.h"


/* return a String containing a character array (this had better not grow) */
extern String *
s_array(char *cp, int len)
{
	String *sp = s_alloc();

	sp->base = sp->ptr = cp;
	sp->end = sp->base + len;
	return sp;
}
