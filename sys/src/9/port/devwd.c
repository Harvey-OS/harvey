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

static int
wdstat(Chan *c, uchar *dp, int n)
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
	char *p;
	ulong offset = off;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, wddir, nelem(wddir), devgen);

	case Qwdctl:
		if(wd == nil || wd->stat == nil)
			return 0;

		p = malloc(READSTR);
		if(waserror()){
			free(p);
			nexterror();
		}

		wd->stat(p, p + READSTR);
		n = readstr(offset, a, n, p);
		poperror();
		free(p);
		return n;

	default:
		error(Egreg);
		break;
	}
	return 0;
}

static void
wdctl(char *a)
{
	int n;
	char *field[6];

	n = tokenize(a, field, nelem(field));
	if(n < 1)
		error(Ebadarg);

	if(!strcmp(field[0], "enable"))
		wd->enable();
	else if(!strcmp(field[0], "disable"))
		wd->disable();
	else if(!strcmp(field[0], "restart"))
		wd->restart();
	else
		error(Ebadarg);
}

static long
wdwrite(Chan* c, void* a, long n, vlong off)
{
	char *p;
	ulong offset = off;

	switch((ulong)c->qid.path){
	case Qdir:
		error(Eperm);

	case Qwdctl:
		if(wd == nil)
			return n;

		if(offset || n >= READSTR)
			error(Ebadarg);

		p = malloc(READSTR);
		if(waserror()){
			free(p);
			nexterror();
		}
		memmove(p, a, n);
		p[n] = 0;
		wdctl(p);
		poperror();
		free(p);
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
