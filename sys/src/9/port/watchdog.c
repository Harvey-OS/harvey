#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define WDMAGIC 0xfaceface

struct Watchdog
{
	Watchdog *next;
	ulong	magic;
	ulong	ticks;
	void	(*f)(int);
	ulong	arg;
	uchar	armed;
};

struct {
	Lock;
	Watchdog *wd;
} wdalloc;

Watchdog*
wdCreate(void)
{
	Watchdog *wd;

	wd = mallocz(sizeof(Watchdog), 1);
	if(wd == nil)
		return nil;
	wd->magic = WDMAGIC;
	return wd;
}

static char*
wdunlink(Watchdog *wd, ulong magic)
{
	char *rv;
	Watchdog **l;

	rv = "not started";
	ilock(&wdalloc);
	wd->magic = magic;
	for(l = &wdalloc.wd; *l != nil; l = &(*l)->next)
		if(*l == wd){
			*l = wd->next;
			rv = nil;
			break;
		}
	wd->armed = 0;
	iunlock(&wdalloc);
	return rv;
}

char*
wdDelete(Watchdog *wd)
{
	if(wd->magic != WDMAGIC)
		return "not a watchdog";

	wdunlink(wd, ~WDMAGIC);
	free(wd);

	return nil;
}

char*
wdStart(Watchdog *wd, ulong ms, void (*f)(int), int arg)
{
	Watchdog **l;

	if(wd->magic != WDMAGIC)
		return "not a watchdog";

	/* unchain it in case it already is chained */
	if(wd->armed)
		wdunlink(wd, WDMAGIC);

	/* chain the watchdogs in time order */
	ilock(&wdalloc);
	wd->ticks = MS2TK(ms) + MACHP(0)->ticks;
	wd->f = f;
	wd->arg = arg;
	wd->armed = 1;
	for(l = &wdalloc.wd; *l != nil; l = &(*l)->next)
		if(wd->ticks < (*l)->ticks)
			break;
	wd->next = *l;
	*l = wd;
	iunlock(&wdalloc);

	return nil;
}

char*
wdCancel(Watchdog *wd)
{
	if(wd->magic != WDMAGIC)
		return "not a watchdog";
	return wdunlink(wd, WDMAGIC);
}

static void
wdclock(void)
{
	Watchdog *wd, **l;

	/* find the barking dogs, remoe from the chain */
	ilock(&wdalloc);
	wd = wdalloc.wd;
	for(l = &wdalloc.wd; *l != nil; l = &(*l)->next)
		if(MACHP(0)->ticks - (*l)->ticks >= 0x10000000)
			break;
	if(l == &wdalloc.wd)
		wd = nil;
	else {
		wdalloc.wd = *l;
		*l = nil;
	}
	iunlock(&wdalloc);

	/* let them run */
	for(; wd != nil; wd = wd->next){
		wd->ticks = 0;
		wd->armed = 0;
		(*wd->f)(wd->arg);
	}
}

void
watchdoglink(void)
{
	addclock0link(wdclock);
}
