#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"io.h"

enum{
	Qrtc = 1,
	Qnvram,
	Qnvram2,
	Qled,

	/* sccr */
	RTDIV=	1<<24,
	RTSEL=	1<<23,

	/* rtcsc */
	RTE=	1<<0,
	R38K=	1<<4,

	Nvoff=		4*1024,	/* where usable nvram lives */
	Nvsize=		4*1024,
	Nvoff2=		8*1024,	/* where second usable nvram lives */
	Nvsize2=	4*1024,
};

static	QLock	rtclock;		/* mutex on clock operations */
static Lock nvrtlock;

static Dirtab rtcdir[]={
	"rtc",		{Qrtc, 0},	12,	0666,
	"nvram",	{Qnvram, 0},	Nvsize,	0664,
	"nvram2",	{Qnvram2, 0},	Nvsize,	0664,
	"led",	{Qled, 0},	0,	0664,
};
#define	NRTC	(sizeof(rtcdir)/sizeof(rtcdir[0]))

static void
rtcreset(void)
{
	IMM *io;
	int n;

	io = m->iomem;
	io->rtcsck = KEEP_ALIVE_KEY;
	n = (RTClevel<<8)|RTE;
	if(m->oscclk == 5)
		n |= R38K;
	io->rtcsc = n;
	io->rtcsck = ~KEEP_ALIVE_KEY;
print("sccr=#%8.8lux plprcr=#%8.8lux\n", io->sccr, io->plprcr);
}

static Chan*
rtcattach(char *spec)
{
	return devattach('r', spec);
}

static int	 
rtcwalk(Chan *c, char *name)
{
	return devwalk(c, name, rtcdir, NRTC, devgen);
}

static void	 
rtcstat(Chan *c, char *dp)
{
	devstat(c, dp, rtcdir, NRTC, devgen);
}

static Chan*
rtcopen(Chan *c, int omode)
{
	omode = openmode(omode);
	switch(c->qid.path){
	case Qrtc:
	case Qled:
		if(strcmp(up->user, eve)!=0 && omode!=OREAD)
			error(Eperm);
		break;
	case Qnvram:
	case Qnvram2:
		if(strcmp(up->user, eve)!=0)
			error(Eperm);
		break;
	}
	return devopen(c, omode, rtcdir, NRTC, devgen);
}

static void	 
rtcclose(Chan*)
{
}

static long	 
rtcread(Chan *c, void *buf, long n, vlong offset)
{
	ulong t;

	if(c->qid.path & CHDIR)
		return devdirread(c, buf, n, rtcdir, NRTC, devgen);

	switch(c->qid.path){
	case Qrtc:
		t = m->iomem->rtc;
		n = readnum(offset, buf, n, t, 12);
		return n;
	case Qnvram:
		return 0;
		if(offset >= Nvsize)
			return 0;
		t = offset;
		if(t + n > Nvsize)
			n = Nvsize - t;
		ilock(&nvrtlock);
		memmove(buf, (uchar*)(NVRAMMEM + Nvoff + t), n);
		iunlock(&nvrtlock);
		return n;
	case Qnvram2:
		return 0;
		if(offset >= Nvsize2)
			return 0;
		t = offset;
		if(t + n > Nvsize2)
			n = Nvsize2 - t;
		ilock(&nvrtlock);
		memmove(buf, (uchar*)(NVRAMMEM + Nvoff2 + t), n);
		iunlock(&nvrtlock);
		return n;
	case Qled:
		return 0;
	}
	error(Egreg);
	return 0;		/* not reached */
}

static long	 
rtcwrite(Chan *c, void *buf, long n, vlong offset)
{
	ulong secs;
	char *cp, *ep;
	IMM *io;
	ulong t;

	switch(c->qid.path){
	case Qrtc:
		if(offset!=0)
			error(Ebadarg);
		/*
		 *  read the time
		 */
		cp = ep = buf;
		ep += n;
		while(cp < ep){
			if(*cp>='0' && *cp<='9')
				break;
			cp++;
		}
		secs = strtoul(cp, 0, 0);
		/*
		 * set it
		 */
		io = ioplock();
		io->rtck = KEEP_ALIVE_KEY;
		io->rtc = secs;
		io->rtck = ~KEEP_ALIVE_KEY;
		iopunlock();
		return n;
	case Qnvram:
		return 0;
		if(offset >= Nvsize)
			return 0;
		t = offset;
		if(t + n > Nvsize)
			n = Nvsize - t;
		ilock(&nvrtlock);
		memmove((uchar*)(NVRAMMEM + Nvoff + offset), buf, n);
		iunlock(&nvrtlock);
		return n;
	case Qnvram2:
		return 0;
		if(offset >= Nvsize2)
			return 0;
		t = offset;
		if(t + n > Nvsize2)
			n = Nvsize2 - t;
		ilock(&nvrtlock);
		memmove((uchar*)(NVRAMMEM + Nvoff2 + offset), buf, n);
		iunlock(&nvrtlock);
		return n;
	case Qled:
		if(strncmp(buf, "on", 2) == 0)
			powerupled();
		else if(strncmp(buf, "off", 3) == 0)
			powerdownled();
		else
			error("unknown command");
		return n;
	}
	error(Egreg);
	return 0;		/* not reached */
}

long
rtctime(void)
{
	return m->iomem->rtc;
}

Dev rtcdevtab = {
	'r',
	"rtc",

	rtcreset,
	devinit,
	rtcattach,
	devclone,
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
