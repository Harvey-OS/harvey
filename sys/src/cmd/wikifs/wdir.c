#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

/* open, create relative to wiki dir */
char *wikidir;

static char*
wname(char *s)
{
	char *t;

	t = emalloc(strlen(wikidir)+1+strlen(s)+1);
	strcpy(t, wikidir);
	strcat(t, "/");
	strcat(t, s);
	return t;
}

int
wopen(char *fn, int mode)
{
	int rv;

	fn = wname(fn);
	rv = open(fn, mode);
	free(fn);
	return rv;
}

int
wcreate(char *fn, int mode, long perm)
{
	int rv;

	fn = wname(fn);
	rv = create(fn, mode, perm);
	free(fn);
	return rv;
}

Biobuf*
wBopen(char *fn, int mode)
{
	Biobuf *rv;

	fn = wname(fn);
	rv = Bopen(fn, mode);
	free(fn);
	return rv;
}

int
waccess(char *fn, int mode)
{
	int rv;

	fn = wname(fn);
	rv = access(fn, mode);
	free(fn);
	return rv;
}

Dir*
wdirstat(char *fn)
{
	Dir *d;

	fn = wname(fn);
	d = dirstat(fn);
	free(fn);
	return d;
}
