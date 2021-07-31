#include <u.h>
#include <libc.h>
#include "String.h"


/* return a String containing a copy of the passed char array */
extern String*
s_copy(char *cp)
{
	String *sp;
	int len;

	len = strlen(cp)+1;
	sp = s_newalloc(len);
	strcpy(sp->base, cp);
	sp->ptr = sp->base + len - 1;		/* point to 0 terminator */
	return sp;
}
