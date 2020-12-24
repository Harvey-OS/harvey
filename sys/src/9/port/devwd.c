/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

enum {
	Qdir,
	Qwdctl,
};

static Watchdog *wd;
static Dirtab wddir[] = {
	{".", {Qdir, 0, QTDIR}, 0, 0550},
	{"wdctl", {Qwdctl, 0}, 0, 0660},
};

void
addwatchdog(Watchdog *watchdog)
{
	if(wd){
		print("addwatchdog: watchdog already installed\n");
		return;
	}
	wd = watchdog;
	if(wd)
		wd->disable();
}

static Chan *
wdattach(char *spec)
{
	return devattach('w', spec);
}

static Walkqid *
wdwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, wddir, nelem(wddir), devgen);
}

static i32
wdstat(Chan *c, u8 *dp, i32 n)
{
	return devstat(c, dp, n, wddir, nelem(wddir), devgen);
}

static Chan *
wdopen(Chan *c, int omode)
{
	return devopen(c, omode, wddir, nelem(wddir), devgen);
}

static void
wdclose(Chan *c)
{
}

static i32
wdread(Chan *c, void *a, i32 n, i64 off)
{
	i32 offset;
	char s[READSTR];

	offset = off;
	switch((u32)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, wddir, nelem(wddir), devgen);

	case Qwdctl:
		if(wd == nil || wd->stat == nil)
			return 0;

		wd->stat(s, s + READSTR);
		return readstr(offset, a, n, s);

	default:
		error(Egreg);
		break;
	}
	return 0;
}

static i32
wdwrite(Chan *c, void *a, i32 n, i64 off)
{
	char *p;

	switch((u32)c->qid.path){
	case Qdir:
		error(Eperm);

	case Qwdctl:
		if(wd == nil)
			return n;

		if(off != 0ll)
			error(Ebadarg);

		if((p = strchr(a, '\n')) != nil)
			*p = 0;

		if(!strncmp(a, "enable", n))
			wd->enable();
		else if(!strncmp(a, "disable", n))
			wd->disable();
		else if(!strncmp(a, "restart", n))
			wd->restart();
		else
			error(Ebadarg);
		return n;

	default:
		error(Egreg);
		break;
	}

	return 0;
}

Dev wddevtab = {
	.dc = 'w',
	.name = "watchdog",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = wdattach,
	.walk = wdwalk,
	.stat = wdstat,
	.open = wdopen,
	.create = devcreate,
	.close = wdclose,
	.read = wdread,
	.bread = devbread,
	.write = wdwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
	.power = devpower,
};
