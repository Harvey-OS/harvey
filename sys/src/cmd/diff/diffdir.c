/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include "diff.h"

static int
itemcmp(void *v1, void *v2)
{
	int8_t **d1 = v1, **d2 = v2;

	return strcmp(*d1, *d2);
}

static int8_t **
scandir(int8_t *name)
{
	int8_t **cp;
	Dir *db;
	int nitems;
	int fd, n;

	if ((fd = open(name, OREAD)) < 0) {
		fprint(2, "%s: can't open %s: %r\n", argv0, name);
		/* fake an empty directory */
		cp = MALLOC(int8_t*, 1);
		cp[0] = 0;
		return cp;
	}
	cp = 0;
	nitems = 0;
	if((n = dirreadall(fd, &db)) > 0){
		while (n--) {
			cp = REALLOC(cp, int8_t *, (nitems+1));
			cp[nitems] = MALLOC(int8_t, strlen((db+n)->name)+1);
			strcpy(cp[nitems], (db+n)->name);
			nitems++;
		}
		free(db);
	}
	cp = REALLOC(cp, int8_t*, (nitems+1));
	cp[nitems] = 0;
	close(fd);
	qsort((int8_t *)cp, nitems, sizeof(int8_t*), itemcmp);
	return cp;
}

static int
isdotordotdot(int8_t *p)
{
	if (*p == '.') {
		if (!p[1])
			return 1;
		if (p[1] == '.' && !p[2])
			return 1;
	}
	return 0;
}

void
diffdir(int8_t *f, int8_t *t, int level)
{
	int8_t  **df, **dt, **dirf, **dirt;
	int8_t *from, *to;
	int res;
	int8_t fb[MAXPATHLEN+1], tb[MAXPATHLEN+1];

	df = scandir(f);
	dt = scandir(t);
	dirf = df;
	dirt = dt;
	while (*df || *dt) {
		from = *df;
		to = *dt;
		if (from && isdotordotdot(from)) {
			df++;
			continue;
		}
		if (to && isdotordotdot(to)) {
			dt++;
			continue;
		}
		if (!from)
			res = 1;
		else if (!to)
			res = -1;
		else
			res = strcmp(from, to);
		if (res < 0) {
			if (mode == 0 || mode == 'n')
				Bprint(&stdout, "Only in %s: %s\n", f, from);
			df++;
			continue;
		}
		if (res > 0) {
			if (mode == 0 || mode == 'n')
				Bprint(&stdout, "Only in %s: %s\n", t, to);
			dt++;
			continue;
		}
		if (mkpathname(fb, f, from))
			continue;
		if (mkpathname(tb, t, to))
			continue;
		diff(fb, tb, level+1);
		df++; dt++;
	}
	for (df = dirf; *df; df++)
		FREE(*df);
	for (dt = dirt; *dt; dt++)
		FREE(*dt);
	FREE(dirf);
	FREE(dirt);
}
