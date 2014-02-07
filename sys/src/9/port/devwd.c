/*
 * watchdog framework
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

/*
 * these are exposed so that delay() and the like can disable the watchdog
 * before busy looping for a long time.
 */
Watchdog*watchdog;
int	watchdogon;

static Watchdog *wd;
static int wdautopet;
static int wdclock0called;
static Ref refs;
static Dirtab wddir[] = {
	".",		{ Qdir, 0, QTDIR },	0,		0555,
	"wdctl",	{ Qwdctl, 0 },		0,		0664,
};


void
addwatchdog(Watchdog *wdog)
{
	if(wd){
		print("addwatchdog: watchdog already installed\n");
		return;
	}
	wd = watchdog = wdog;
	if(wd)
		wd->disable();
}

static int
wdallowed(void)
{
	return getconf("*nowatchdog") == nil;
}

static void
wdshutdown(void)
{
	if (wd) {
		wd->disable();
		watchdogon = 0;
	}
}

/* called from clock interrupt, so restart needs ilock internally */
static void
wdpet(void)
{
	/* watchdog could be paused; if so, don't restart */
	if (wdautopet && watchdogon)
		wd->restart();
}

/*
 * reassure the watchdog from the clock interrupt
 * until the user takes control of it.
 */
static void
wdautostart(void)
{
	if (wdautopet || !wd || !wdallowed())
		return;
	if (waserror()) {
		print("watchdog: automatic enable failed\n");
		return;
	}
	wd->enable();
	poperror();

	wdautopet = watchdogon = 1;
	if (!wdclock0called) {
		addclock0link(wdpet, 200);
		wdclock0called = 1;
	}
}

/*
 * disable strokes from the clock interrupt.
 * have to disable the watchdog to mark it `not in use'.
 */
static void
wdautostop(void)
{
	if (!wdautopet)
		return;
	wdautopet = 0;
	wdshutdown();
}

/*
 * user processes exist and up is non-nil when the
 * device init routines are called.
 */
static void
wdinit(void)
{
	wdautostart();
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
	wdautostop();
	c = devopen(c, omode, wddir, nelem(wddir), devgen);
	if (c->qid.path == Qwdctl)
		incref(&refs);
	return c;
}

static void
wdclose(Chan *c)
{
	if(c->qid.path == Qwdctl && c->flag&COPEN && decref(&refs) <= 0)
		wdshutdown();
}

static long
wdread(Chan* c, void* a, long n, vlong off)
{
	ulong offset = off;
	char *p;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, wddir, nelem(wddir), devgen);

	case Qwdctl:
		if(wd == nil || wd->stat == nil)
			return 0;

		p = malloc(READSTR);
		if(p == nil)
			error(Enomem);
		if(waserror()){
			free(p);
			nexterror();
		}

		wd->stat(p, p + READSTR);
		n = readstr(offset, a, n, p);
		free(p);
		poperror();
		return n;

	default:
		error(Egreg);
		break;
	}
	return 0;
}

static long
wdwrite(Chan* c, void* a, long n, vlong off)
{
	ulong offset = off;
	char *p;

	switch((ulong)c->qid.path){
	case Qdir:
		error(Eperm);

	case Qwdctl:
		if(wd == nil)
			return n;

		if(offset || n >= READSTR)
			error(Ebadarg);

		if((p = strchr(a, '\n')) != nil)
			*p = 0;

		if(strncmp(a, "enable", n) == 0) {
			if (waserror()) {
				print("watchdog: enable failed\n");
				nexterror();
			}
			wd->enable();
			poperror();
			watchdogon = 1;
		} else if(strncmp(a, "disable", n) == 0)
			wdshutdown();
		else if(strncmp(a, "restart", n) == 0)
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
	wdinit,
	wdshutdown,
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
