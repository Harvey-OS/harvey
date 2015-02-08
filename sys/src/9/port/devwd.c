/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

enum {
	Qdir,
	Qwdctl,
};

static Watchdog *wd;
static Dirtab wddir[] = {
	".",		{ Qdir, 0, QTDIR },	0,		0550,
	"wdctl",	{ Qwdctl, 0 },		0,		0660,
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

static Chan*
wdattach(char *spec)
{
	return devattach('w', spec);
}

static Walkqid*
wdwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, wddir, nelem(wddir), devgen);
}

static long
wdstat(Chan *c, uchar *dp, long n)
{
	return devstat(c, dp, n, wddir, nelem(wddir), devgen);
}

static Chan*
wdopen(Chan* c, int omode)
{
	return devopen(c, omode, wddir, nelem(wddir), devgen);
}

static void
wdclose(Chan*)
{
}

static long
wdread(Chan* c, void* a, long n, vlong off)
{
	long offset;
	char s[READSTR];

	offset = off;
	switch((ulong)c->qid.path){
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

static long
wdwrite(Chan* c, void* a, long n, vlong off)
{
	char *p;

	switch((ulong)c->qid.path){
	case Qdir:
		error(Eperm);

	case Qwdctl:
		if(wd == nil)
			return n;

		if(off != 0ll)
			error(Ebadarg);

		if(p = strchr(a, '\n'))
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
	'w',
	"watchdog",

	devreset,
	devinit,
	devshutdown,
	wdattach,
	wdwalk,
	wdstat,
	wdopen,
	devcreate,
	wdclose,
	wdread,
	devbread,
	wdwrite,
	devbwrite,
	devremove,
	devwstat,
	devpower,
};
