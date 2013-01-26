//
// channel element driver
//
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "msaturn.h"

enum{
	Ucuunknown = 0,
	Ucu64,

	Ndsp = 16,

	Cedead = 0,
	Cereset,
	Celoaded,
	Cerunning,
	
	Qdir = 0,
	Qctl,
	Qce,

	Cpldbase = Saturn + 0x6000000,
		Cplducuversion_ucu = 0x1c,
	Cebase = Saturn + 0x3000000,
	Cesize = 0x100000,
};

typedef struct Cpld Cpld;
struct Cpld{
	uchar	led;
	uchar	fpga1;
	uchar	slotid;
	uchar	version;
	uchar	watchdog;
	uchar	spi;
	uchar	asicreset;
	uchar	dspreset;
	uchar	generalreset;
	uchar	ucuversion;
	uchar	fpga2;
};

typedef struct Circbuf Circbuf;
struct Circbuf{
	uchar	*nextin;
	uchar	*nextout;
	uchar	*start;
	uchar	*end;
};

typedef struct Dsp Dsp;
struct Dsp{
	Ref;
	int		state;
	Circbuf	*cb;
};

typedef struct Ce Ce;
struct Ce{
	int		ucutype;
	Ce		*ces[Ndsp];
};

static Cpld*cpld = (Cpld*)Cpldbase;
static Ce ce;

static void
ceinit(void)
{
	if(cpld->ucuversion & Cplducuversion_ucu)
		ce.ucutype = Ucu64;
	else{
		print("ceinit: unsuppoerted UCU\n");
		return;
	}

}

static Chan*
ceattach(char*spec)
{
	return devattach('C', spec);
}

#define DEV(q)			((int)(((q).path >> 8) & 0xff))
#define TYPE(q)			((int)((q).path & 0xff))
#define QID(d, t)		((((d) & 0xff) << 8) | (t))

static int
cegen(Chan*c, char*, Dirtab*, int, int i, Dir*dp)
{
	Qid qid;

	switch(TYPE(c->qid)){
	case Qdir:
		if(i == DEVDOTDOT){
			mkqid(&qid, QID(0, Qdir), 0, QTDIR);
			devdir(c, qid, "#C", 0, eve, 0555, dp);
			return 1;
		}

		if(i == 0){
			mkqid(&qid, QID(-1, Qctl), 0, QTFILE);
			devdir(c, qid, "cectl", 0, eve, 0644, dp);
			return 1;
		}

		if (--i >= Ndsp)
			return -1;
	
		mkqid(&qid, QID(Qce, i), 0, QTFILE);
		snprint(up->genbuf, sizeof(up->genbuf), "ce%d", i);
		devdir(c, qid, up->genbuf, 0, eve, 0644, dp);
		return 1;

	default:
		return -1;
	}
}

static Walkqid *
cewalk(Chan*c, Chan*nc, char**name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, cegen);
}

static int
cestat(Chan*c, uchar*db, int n)
{
	return devstat(c, db, n, 0, 0, cegen);
}

static Chan*
ceopen(Chan*c, int omode)
{
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
ceclose(Chan*)
{}

static long
ceread(Chan*c, void*a, long n, vlong)
{
	switch(TYPE(c->qid)){
	case Qdir:
		return devdirread(c, a, n, 0, 0, cegen);
	
	default:
		error("unsupported operation");
	}
	return 0;
}

static long
cewrite(Chan*, void*, long, vlong)
{
	return 0;
}

Dev cedevtab = {
	'C',
	"channel element",

	devreset,
	ceinit,
	devshutdown,
	ceattach,
	cewalk,
	cestat,
	ceopen,
	devcreate,
	ceclose,
	ceread,
	devbread,
	cewrite,
	devbwrite,
	devremove,
	devwstat,
	devpower,
};
