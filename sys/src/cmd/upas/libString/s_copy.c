#include <u.h>
#include <libc.h>
#include "String.h"


/* return a String containing a copy of the passed char array */
extern String*
s_copy(char *cp)
{
	String *sp;
	int len;

	sp = s_alloc();
	len = strlen(cp)+1;
	sp->base = malloc(len);
	if (sp->base == 0) {
		perror("String:");
		exits("malloc");
	}
	sp->end = sp->base + len;	/* point past end of allocation */
	strcpy(sp->base, cp);
	sp->ptr = sp->end - 1;		/* point to 0 terminator */
	return sp;
}
