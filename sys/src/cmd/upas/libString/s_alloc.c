#include <u.h>
#include <libc.h>
#include "String.h"

#define STRLEN 128

extern void
s_free(String *sp)
{
	if (sp != nil) {
		if(sp->base != nil)
			free(sp->base);
		free(sp);
	}
}

/* allocate a String head */
extern String *
s_alloc(void)
{
	String *s;

	s = mallocz(sizeof *s, 1);
	return s;
}

/* create a new `short' String */
extern String *
s_newalloc(int len)
{
	String *sp;

	sp = s_alloc();
	if(sp == nil){
		s_error("String (malloc)", "malloc");
		/* should never return */
		exits("malloc");
	}
	if(len < STRLEN)
		len = STRLEN;
	sp->base = sp->ptr = malloc(len);
	if (sp->base == nil) {
		free(sp);
		s_error("String (malloc)", "malloc");
		/* should never return */
		exits("malloc");
	}
	sp->end = sp->base + len;
	s_terminate(sp);
	return sp;
}

/* create a new `short' String */
extern String *
s_new(void)
{
	String *sp;

	sp = s_alloc();
	if(sp == nil){
		s_error("String (malloc)", "malloc");
		/* should never return */
		exits("malloc");
	}
	sp->base = sp->ptr = malloc(STRLEN);
	if (sp->base == nil) {
		free(sp);
		s_error("String (malloc)", "malloc");
		/* should never return */
		exits("malloc");
	}
	sp->end = sp->base + STRLEN;
	s_terminate(sp);
	return sp;
}
