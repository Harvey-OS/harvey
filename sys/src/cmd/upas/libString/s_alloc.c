#include <u.h>
#include <libc.h>
#include "String.h"


#define STRLEN 128
#define STRALLOC 128

/* buffer pool for allocating String structures */
typedef struct Stralloc{
	String s[STRALLOC];
	int o;
} Stralloc;
static Stralloc *freep=0;

/* pool of freed Strings */
static String *freed=0;

extern void
s_free(String *sp)
{
	if (sp != 0) {
		sp->ptr = (char *)freed;
		freed = sp;
	}
}

/* allocate a String head */
extern String *
s_alloc(void)
{
	if (freep==0 || freep->o >= STRALLOC) {
		freep = (Stralloc *)malloc(sizeof(Stralloc));
		if (freep==0) {
			perror("allocating String");
			exits("malloc");
		}
		freep->o = 0;
	}
	return &(freep->s[freep->o++]);
}

/* create a new `short' String */
extern String *
s_new(void)
{
	String *sp;

	if (freed!=0) {
		sp = freed;
		freed = (String *)(freed->ptr);
		sp->ptr = sp->base;
		return sp;
	}
	sp = s_alloc();
	sp->base = sp->ptr = malloc(STRLEN);
	if (sp->base == 0) {
		perror("allocating String");
		exits("malloc");
	}
	sp->end = sp->base + STRLEN;
	s_terminate(sp);
	return sp;
}
