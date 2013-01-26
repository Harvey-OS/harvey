/*
 * raspberry pi doesn't have a realtime clock
 * fake a crude approximation from the kernel build time
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum{
	Qdir = 0,
	Qrtc,
};

Dirtab rtcdir[]={
	".",	{Qdir, 0, QTDIR},	0,	0555,
	"rtc",		{Qrtc, 0},	0,	0664,
};

extern ulong kerndate;

static ulong rtcsecs;

static void
rtctick(void)
{
	rtcsecs++;
}

static void
rtcinit(void)
{
	rtcsecs = kerndate;
	addclock0link(rtctick, 1000);
}

static long
rtcread(Chan *c, void *a, long n, vlong offset)
{
	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, rtcdir, nelem(rtcdir), devgen);

	switch((ulong)c->qid.path){
	case Qrtc:
		return readnum((ulong)offset, a, n, rtcsecs, 12);
	}
	error(Ebadarg);
	return 0;
}

static long
rtcwrite(Chan*c, void *a, long n, vlong)
{
	char b[13];
	ulong i;

	switch((ulong)c->qid.path){
	case Qrtc:
		if(n >= sizeof(b))
			error(Ebadarg);
		strncpy(b, (char*)a, n);
		i = strtol(b, 0, 0);
		if(i <= 0)
			error(Ebadarg);
		rtcsecs = i;
		return n;
	}
	error(Eperm);
	return 0;
}

static Chan*
rtcattach(char* spec)
{
	return devattach('r', spec);
}

static Walkqid*	 
rtcwalk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, rtcdir, nelem(rtcdir), devgen);
}

static int	 
rtcstat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, rtcdir, nelem(rtcdir), devgen);
}

static Chan*
rtcopen(Chan* c, int omode)
{
	return devopen(c, omode, rtcdir, nelem(rtcdir), devgen);
}

static void	 
rtcclose(Chan*)
{
}

Dev fakertcdevtab = {
	'r',
	"rtc",

	devreset,
	rtcinit,
	devshutdown,
	rtcattach,
	rtcwalk,
	rtcstat,
	rtcopen,
	devcreate,
	rtcclose,
	rtcread,
	devbread,
	rtcwrite,
	devbwrite,
	devremove,
	devwstat,
};

