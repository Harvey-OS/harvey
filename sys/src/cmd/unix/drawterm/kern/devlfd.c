/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include <errno.h>
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

#undef pread
#undef pwrite

Chan *
lfdchan(int fd)
{
	Chan *c;

	c = newchan();
	c->type = devno('L', 0);
	c->aux = (void *)(uintptr)fd;
	c->name = newcname("fd");
	c->mode = ORDWR;
	c->qid.type = 0;
	c->qid.path = 0;
	c->qid.vers = 0;
	c->dev = 0;
	c->offset = 0;
	return c;
}

int
lfdfd(int fd)
{
	return newfd(lfdchan(fd));
}

static Chan *
lfdattach(char *x)
{
	USED(x);

	error(Egreg);
	return nil;
}

static Walkqid *
lfdwalk(Chan *c, Chan *nc, char **name, int nname)
{
	USED(c);
	USED(nc);
	USED(name);
	USED(nname);

	error(Egreg);
	return nil;
}

static int
lfdstat(Chan *c, uint8_t *dp, int n)
{
	USED(c);
	USED(dp);
	USED(n);
	error(Egreg);
	return -1;
}

static Chan *
lfdopen(Chan *c, int omode)
{
	USED(c);
	USED(omode);

	error(Egreg);
	return nil;
}

static void
lfdclose(Chan *c)
{
	close((int)(uintptr)c->aux);
}

static int32_t
lfdread(Chan *c, void *buf, int32_t n, int64_t off)
{
	USED(off); /* can't pread on pipes */
	n = read((int)(uintptr)c->aux, buf, n);
	if(n < 0) {
		iprint("error %d\n", errno);
		oserror();
	}
	return n;
}

static int32_t
lfdwrite(Chan *c, void *buf, int32_t n, int64_t off)
{
	USED(off); /* can't pread on pipes */

	n = write((int)(uintptr)c->aux, buf, n);
	if(n < 0) {
		iprint("error %d\n", errno);
		oserror();
	}
	return n;
}

Dev lfddevtab = {
    'L',
    "lfd",

    devreset,
    devinit,
    devshutdown,
    lfdattach,
    lfdwalk,
    lfdstat,
    lfdopen,
    devcreate,
    lfdclose,
    lfdread,
    devbread,
    lfdwrite,
    devbwrite,
    devremove,
    devwstat,
};
