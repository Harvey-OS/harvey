/*
 * NCR 53c8xx driver for Plan 9
 * Nigel Roles (ngr@cotswold.demon.co.uk)
 *
 * 04/08/98     Added missing locks to interrupt handler. Marked places where 
 *		multiple controller extensions could go
 *
 * 18/05/97	Fixed overestimate in size of local SCRIPT RAM
 *
 * 17/05/97	Bug fix to return status
 *
 * 06/10/96	Enhanced list of chip IDs. 875 revision 1 has no clock doubler, so assume it
 *		is shipped with 80MHz crystal. Use bit 3 of the GPREG to recognise differential
 *		boards. This is Symbios specific, but since they are about the only suppliers of
 *		differential cards.
 *
 * 23/9/96	Wide and Ultra supported. 825A and 860 added to variants. Dual compiling
 *		version for fileserver and cpu. 80MHz default clock for 860
 *		
 * 5/8/96	Waits for an Inquiry message before initiating synchronous negotiation
 *		in case capabilities byte [7] indicates device does not support it. Devices
 *		which do target initiated negotiation will typically get in first; a few
 *		bugs in handling this have been fixed
 *
 * 3/8/96	Added differential support (put scsi0=diff in plan9.ini)
 *		Split exec() into exec() and io(). Exec() is small, and Io() does not
 *		use any Plan 9 specific data structures, so alternate exec() functions
 *		may be done for other environments, such as the fileserver
 *
 * GENERAL
 *
 * Works on 810 and 875
 * Should work on 815, 825, 810A, 825A, 860A
 * Uses local RAM, large FIFO, prefetch, burst opcode fetch, and 16 byte synch. offset
 * where applicable
 * Supports multi-target, wide, Ultra
 * Differential mode can be enabled by putting scsi0=diff in plan9.ini
 * NO SUPPORT FOR tagged queuing (yet)
 *
 * Known problems
 */

/* Define one or the other ... */
//#define CPU
#define FS

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#ifndef FS
#include "../port/error.h"
#endif

#include "ureg.h"
#ifdef NAINCLUDE
#include "na/na.h"
#include "na/nasupport.h"
#endif /* NAINCLUDE */

/**********************************/
/* Portable configuration macros  */
/**********************************/

//#define BOOTDEBUG
//#define SINGLE_TARGET
//#define ASYNC_ONLY
//#define	QUICK_TIMEOUT
//#define	INTERNAL_SCLK
//#define ALWAYS_DO_WDTR
#define WMR_DEBUG

/**********************************/
/* CPU specific macros            */
/**********************************/

#ifdef CPU
#ifdef BOOTDEBUG

#define KPRINT oprint
#define IPRINT iprint
#define DEBUG(n) 0
#define IFLUSH() iflush()

#else

#define KPRINT kprint
#define IPRINT kprint
#define DEBUG(n) scsidebugs[n]
#define IFLUSH()

#endif
#endif

/*******************************/
/* Fileserver specific defines */
/*******************************/
#ifdef FS
#define xalloc(n) ialloc(n, 1)
#define KADDR(a) ((void*)((ulong)(a)|KZERO))

#define KPRINT if(0)print
#define IPRINT if(0)print
#define DEBUG(n) 0
#define IFLUSH()
#endif

/*******************************/
/* General                     */
/*******************************/

#ifndef DMASEG
#define DMASEG(x) PADDR(x)
#define legetl(x) (*(ulong*)(x))
#define lesetl(x,v) (*(ulong*)(x) = (v))
#define swabl(a,b,c)
#define IRQBASE Int0vec
#else
#define IRQBASE (PCIvec + 5)
#endif
#define DMASEG_TO_KADDR(x) KADDR(PADDR(x))

#define MEGA 1000000L
#ifdef INTERNAL_SCLK
#define	SCLK (33 * MEGA)
#else
#define SCLK (40 * MEGA)
#endif
#define ULTRA_NOCLOCKDOUBLE_SCLK (80 * MEGA)

#define MAXSYNCSCSIRATE (5 * MEGA)
#define MAXFASTSYNCSCSIRATE (10 * MEGA)
#define MAXULTRASYNCSCSIRATE (20 * MEGA)
#define MAXASYNCCORERATE (25 * MEGA)
#define MAXSYNCCORERATE (25 * MEGA)
#define MAXFASTSYNCCORERATE (50 * MEGA)
#define MAXULTRASYNCCORERATE (80 * MEGA)


#define X_MSG	1
#define X_MSG_SDTR 1
#define X_MSG_WDTR 3

#ifndef NAINCLUDE
struct na_patch {
	unsigned lwoff;
	unsigned char type;
};
#endif /* NAINCLUDE */

extern int scsidebugs[];

typedef struct Ncr {
	uchar scntl0;	/* 00 */
	uchar scntl1;
	uchar scntl2;
	uchar scntl3;

	uchar scid;	/* 04 */
	uchar sxfer;
	uchar sdid;
	uchar gpreg;

	uchar sfbr;	/* 08 */
	uchar socl;
	uchar ssid;
	uchar sbcl;

	uchar dstat;	/* 0c */
	uchar sstat0;
	uchar sstat1;
	uchar sstat2;

	uchar dsa[4];	/* 10 */

	uchar istat;	/* 14 */
	uchar istatpad[3];

	uchar ctest0;	/* 18 */
	uchar ctest1;
	uchar ctest2;
	uchar ctest3;

	uchar temp[4];	/* 1c */

	uchar dfifo;	/* 20 */
	uchar ctest4;
	uchar ctest5;
	uchar ctest6;

	union {		/* 24 */
		ulong dbc;	/* only 24 bits usable LONG */
		struct {
			uchar dcmdpad[3];
			uchar dcmd;
		};
	};
	
	uchar dnad[4];	/* 28 */
	uchar dsp[4];	/* 2c */
	uchar dsps[4];	/* 30 */

	uchar scratcha[4];	/* 34 */

	uchar dmode;	/* 38 */
	uchar dien;
	uchar dwt;
	uchar dcntl;

	uchar adder[4];	/* 3c */
	
	uchar sien0;	/* 40 */
	uchar sien1;
	uchar sist0;
	uchar sist1;

	uchar slpar;	/* 44 */
	uchar slparpad0;
	uchar macntl;
	uchar gpcntl;

	uchar stime0;	/* 48 */
	uchar stime1;
	uchar respid;
	uchar respidpad0;

	uchar stest0;	/* 4c */
	uchar stest1;
	uchar stest2;
	uchar stest3;

	uchar sidl;	/* 50 */
	uchar sidlpad[3];

	uchar sodl;	/* 54 */
	uchar sodlpad[3];

	uchar sbdl;	/* 58 */
	uchar sbdlpad[3];

	union {		/* 5c */
		uchar scratchb[4];
		ulong scratchbncr;
	};
} Ncr;

typedef struct Movedata {
	uchar dbc[4];
	uchar pa[4];
} Movedata;

typedef enum NegoState {
	NeitherDone, WideInit, WideResponse, WideDone,
	SyncInit, SyncResponse, BothDone
} NegoState;

typedef enum State {
	Allocated, Queued, Active, Done
} State;

typedef struct Dsa {
	union {
		uchar state[4];
		struct {
			uchar stateb;
			uchar result;
			uchar dmablks;
			uchar flag;	/* setbyte(state,3,...) */
		};
	};

	union {
		ulong dmancr;		/* For block transfer: NCR order (little-endian) */
		uchar dmaaddr[4];
	};

	uchar target;			/* Target */
	uchar pad0[3];

	uchar lun;			/* Logical Unit Number */
	uchar pad1[3];

	uchar scntl3;
	uchar sxfer;
	uchar pad2[2];

	uchar next[4];			/* chaining for SCRIPT (NCR byte order) */
	struct Dsa *freechain;		/* chaining for freelist */
	Rendez;
	uchar scsi_id_buf[4];
	Movedata msg_out_buf;
	Movedata cmd_buf;
	Movedata data_buf;
	Movedata status_buf;
	uchar msg_out[10];		/* enough to include SDTR */
	uchar status;
	ushort p9status;
	uchar parityerror;
} Dsa;

typedef enum Feature {
	BigFifo = 1,			/* 536 byte fifo */
	BurstOpCodeFetch = 2,		/* burst fetch opcodes */
	Prefetch = 4,			/* prefetch 8 longwords */
	LocalRAM = 8,			/* 4K longwords of local RAM */
	Differential = 16,		/* Differential support */
	Wide = 32,			/* Wide capable */
	Ultra = 64,			/* Ultra capable */
	ClockDouble = 128,		/* Has clock doubler */
} Feature;

typedef enum Burst {
	Burst2 = 0,
	Burst4 = 1,
	Burst8 = 2,
	Burst16 = 3,
	Burst32 = 4,
	Burst64 = 5,
	Burst128 = 6
} Burst;

typedef struct Variant {
	ushort did;
	uchar maxrid;			/* maximum allowed revision ID */
	char *name;
	Burst burst;			/* codings for max burst */
	uchar maxsyncoff;		/* max synchronous offset */
	unsigned char feature;
} Variant;

static unsigned char cf2[] = { 6, 2, 3, 4, 6, 8 };
#define NULTRASCF (sizeof(cf2)/sizeof(cf2[0]))
#define NSCF (NULTRASCF - 1)

typedef struct Controller {
	Lock;
	int ctlrno;
	uchar synctab[NULTRASCF - 1][8];/* table of legal tpfs */
	NegoState s[8];
	uchar scntl3[8];
	uchar sxfer[8];
	uchar cap[8];			/* capabilities byte from Identify */
	uchar capvalid;			/* bit per target for validity of cap[] */
	uchar wide;			/* bit per target set if wide negotiated */
	ulong sclk;			/* clock speed of controller */
	uchar clockdouble;		/* set by synctabinit */
	uchar ccf;			/* CCF bits */
	uchar tpf;			/* best tpf value for this controller */
	uchar feature;			/* requested features */
	int running;			/* is the script processor running? */
	int ssm;			/* single step mode */
	Ncr *n;				/* pointer to registers */
	Variant *v;			/* pointer to variant type */
	ulong *script;			/* where the real script is */
	ulong scriptpa;			/* where the real script is */
	QLock q[8];			/* queues for each target */
} Controller;

static Controller *ctlrxx[MaxScsi];

/* ISTAT */
enum { Abrt = 0x80, Srst = 0x40, Sigp = 0x20, Sem = 0x10, Con = 0x08, Intf = 0x04, Sip = 0x02, Dip = 0x01 };

/* DSTAT */
enum { Dfe = 0x80, Mdpe = 0x40, Bf = 0x20, Abrted = 0x10, Ssi = 0x08, Sir = 0x04, Iid = 0x01 };

/* SSTAT */
enum { DataOut, DataIn, Cmd, Status, ReservedOut, ReservedIn, MessageOut, MessageIn };

#define STATUS_COMPLETE 0x6000
#define STATUS_FAIL 0x8000
#define STATUS_SELECTION_TIMEOUT 0x0200

static void setmovedata(Movedata*, ulong, ulong);
static void advancedata(Movedata*, long);

static char *phase[] = {
	"data out", "data in", "command", "status",
	"reserved out", "reserved in", "message out", "message in"
};

#ifdef BOOTDEBUG
#define DEBUGSIZE 10240
char debugbuf[DEBUGSIZE];
char *debuglast;

void
iprint(char *format, ...)
{
	if (debuglast == 0)
		debuglast = debugbuf;
	debuglast = doprint(debuglast, debugbuf + (DEBUGSIZE - 1), format, (&format + 1));
}

void
iflush()
{
	int s;
	char *endp;
	s = splhi();
	if (debuglast == 0)
		debuglast = debugbuf;
	if (debuglast == debugbuf) {
		splx(s);
		return;
	}
	endp = debuglast;
	splx(s);
	screenputs(debugbuf, endp - debugbuf);
	s = splhi();
	memmove(debugbuf, endp, debuglast - endp);
	debuglast -= endp - debugbuf;
	splx(s);
}

void
oprint(char *format, ...)
{
	int s;

	iflush();
	s = splhi();
	if (debuglast == 0)
		debuglast = debugbuf;
	debuglast = doprint(debuglast, debugbuf + (DEBUGSIZE - 1), format, (&format + 1));
	splx(s);
	iflush();	
}
#endif

#include "script.i"

static struct {
	Lock;
	uchar head[4];	/* head of free list (NCR byte order) */
	Dsa	*tail;
	Dsa	*free;
} dsalist;

Dsa *
dsaalloc(int target, int lun)
{
	Dsa *d;

	lock(&dsalist);
	if ((d = dsalist.free) == 0) {
		d = xalloc(sizeof(*d));
		if (DEBUG(1))
			KPRINT("ncr53c8xx: %d/%d: allocated new dsa %lux\n", target, lun, (ulong)d);
		lesetl(d->next, 0);
		lesetl(d->state, A_STATE_ALLOCATED);
		if (legetl(dsalist.head) == 0)
			lesetl(dsalist.head, DMASEG(d));	/* ATOMIC?!? */
		else
			lesetl(dsalist.tail->next, DMASEG(d));	/* ATOMIC?!? */
		dsalist.tail = d;
	}
	else {
		if (DEBUG(1))
			KPRINT("ncr53c8xx: %d/%d: reused dsa %lux\n", target, lun, (ulong)d);
		dsalist.free = d->freechain;
		lesetl(d->state, A_STATE_ALLOCATED);
	}
	unlock(&dsalist);
	d->target = target;
	d->lun = lun;
	return d;
}

void
dsafree(Dsa *d)
{
	lock(&dsalist);
	d->freechain = dsalist.free;
	dsalist.free = d;
	lesetl(d->state, A_STATE_FREE);
	unlock(&dsalist);
}

static void
dumpncrregs(Controller *c, int intr)
{
	int i;
	Ncr *n = c->n;

	KPRINT("sa = %.8lux\n", c->scriptpa);
	for (i = 0; i < 6; i++) {
		int j;
		for (j = 0; j < 4; j++) {
			int k = j * 6 + i;
			uchar *p;

			/* display little-endian to make 32-bit values readable */
			p = (uchar*)n+k*4;
			if (intr)
				IPRINT(" %.2x%.2x%.2x%.2x %.2x %.2x", p[3], p[2], p[1], p[0], k * 4, (k * 4) + 0x80);
			else
				KPRINT(" %.2x%.2x%.2x%.2x %.2x %.2x", p[3], p[2], p[1], p[0], k * 4, (k * 4) + 0x80);
			USED(p);
		}
		if (intr)
			IPRINT("\n");
		else
			KPRINT("\n");
	}
}	

static int
chooserate(Controller *c, int tpf, int *scfp, int *xferpp)
{
	/* find lowest entry >= tpf */
	int besttpf = 1000;
	int bestscfi = 0;
	int bestxferp = 0;
	int scf, xferp;

	/*
	 * search large clock factors first since this should
	 * result in more reliable transfers
	 */
	for (scf = ((c->v->feature & Ultra) ? NULTRASCF : NSCF); scf >= 1; scf--) {
		for (xferp = 0; xferp < 8; xferp++) {
			unsigned char v = c->synctab[scf - 1][xferp];
			if (v == 0)
				continue;
			if (v >= tpf && v < besttpf) {
				besttpf = v;
				bestscfi = scf;
				bestxferp = xferp;
			}
		}
	}
	if (besttpf == 1000)
		return 0;
	if (scfp)
		*scfp = bestscfi;
	if (xferpp)
		*xferpp = bestxferp;
	return besttpf;
}

void
synctabinit(Controller *c)
{
	int scf;
	unsigned long scsilimit;
	int xferp;
	unsigned long cr, sr;
	int tpf;
	int fast;

	/*
	 * for chips with no clock doubler, but Ultra capable (e.g. 860, or interestingly the
	 * first spin of the 875), assume 80MHz
	 * otherwise use the internal (33 Mhz) or external (40MHz) default
	 */

	if ((c->v->feature & Ultra) != 0 && (c->v->feature & ClockDouble) == 0)
		c->sclk = ULTRA_NOCLOCKDOUBLE_SCLK;
	else
		c->sclk = SCLK;

	/*
	 * otherwise, if the chip is Ultra capable, but has a slow(ish) clock,
	 * invoke the doubler
	 */

	if (SCLK <= 40000000 && (c->v->feature & Ultra) != 0 && (c->v->feature & ClockDouble)) {
		c->sclk *= 2;
		c->clockdouble = 1;
	}
	else
		c->clockdouble = 0;

	/* derive CCF from sclk */
	/* woebetide anyone with SCLK < 16.7 or > 80MHz */
	if (c->sclk <= 25 * MEGA)
		c->ccf = 1;
	else if (c->sclk <= 3750000)
		c->ccf = 2;
	else if (c->sclk <= 50 * MEGA)
		c->ccf = 3;
	else if (c->sclk <= 75 * MEGA)
		c->ccf = 4;
	else if ((c->v->feature & Ultra) && c->sclk <= 80 * MEGA)
		c->ccf = 5;

	for (scf = 1; scf < ((c->v->feature & Ultra) ? NULTRASCF : NSCF); scf++) {
		/* check for legal core rate */
		/* round up so we run slower for safety */
	   	cr = (c->sclk * 2 + cf2[scf] - 1) / cf2[scf];
		if (cr <= MAXSYNCCORERATE) {
			scsilimit = MAXSYNCSCSIRATE;
			fast = 0;
		}
		else if (cr <= MAXFASTSYNCCORERATE) {
			scsilimit = MAXFASTSYNCSCSIRATE;
			fast = 1;
		}
		else if ((c->v->feature & Ultra) && cr <= MAXULTRASYNCCORERATE) {
			scsilimit = MAXULTRASYNCSCSIRATE;
			fast = 2;
		}
		else
			continue;
		for (xferp = 11; xferp >= 4; xferp--) {
			int ok;
			int tp;
			/* calculate scsi rate - round up again */
			/* start from sclk for accuracy */
			int totaldivide = xferp * cf2[scf];
			sr = (c->sclk * 2 + totaldivide - 1) / totaldivide;
			if (sr > scsilimit)
				break;
			/*
			 * now work out transfer period
			 * round down now so that period is pessimistic
			 */
			tp = (MEGA * 1000) / sr;
			/*
			 * bounds check it
			 */
			if (tp < 50 || tp > 255 * 4)
				continue;
			/*
			 * spot stupid special case for Ultra
			 * while working out factor
			 */
			if (tp < 52)
				tpf = 12;
			else
				tpf = tp / 4;
			/*
			 * now check tpf looks sensible
			 * given core rate
			 */
			switch (fast) {
			case 0:
				/* scf must be ccf for SCSI 1 */
				ok = tpf >= 50 && scf == c->ccf;
				break;
			case 1:
				ok = tpf >= 25 && tpf < 50;
				break;
			case 2:
				/*
				 * must use xferp of 4, or 5 at a pinch
				 * for an Ultra transfer
				 */
				ok = xferp <= 5 && tpf >= 12 && tpf < 25;
				break;
			default:
				ok = 0;
			}
			if (!ok)
				continue;
			c->synctab[scf - 1][xferp - 4] = tpf;
		}
	}

	for (tpf = (c->v->feature & Ultra) ? 12 : 25; tpf < 256; tpf++) {
		if (chooserate(c, tpf, &scf, &xferp) == tpf) {
			unsigned tp = tpf == 12 ? 50 : tpf * 4;
			unsigned long khz = (MEGA + tp - 1) / (tp);
			KPRINT("ncr53c8xx: tpf=%d scf=%d.%.1d xferp=%d mhz=%ld.%.3ld\n",
			    tpf, cf2[scf] / 2, (cf2[scf] & 1) ? 5 : 0,
			    xferp + 4, khz / 1000, khz % 1000);
			USED(khz);
			if (c->tpf == 0)
				c->tpf = tpf;	/* note lowest value for controller */
		}
	}
}

static void
synctodsa(Dsa *dsa, Controller *c)
{
/*
	KPRINT("synctodsa(dsa=%lux, target=%d, scntl3=%.2lx sxfer=%.2x)\n",
	    dsa, dsa->target, c->scntl3[dsa->target], c->sxfer[dsa->target]);
*/
	dsa->scntl3 = c->scntl3[dsa->target];
	dsa->sxfer = c->sxfer[dsa->target];
}

static void
setsync(Dsa *dsa, Controller *c, int target, uchar ultra, uchar scf, uchar xferp, uchar reqack)
{
	c->scntl3[target] =
	    (c->scntl3[target] & 0x08) | (((scf << 4) | c->ccf | (ultra << 7)) & ~0x08);
	c->sxfer[target] = (xferp << 5) | reqack;
	c->s[target] = BothDone;
	if (dsa) {
		synctodsa(dsa, c);
		c->n->scntl3 = c->scntl3[target];
		c->n->sxfer = c->sxfer[target];
	}
}

static void
setasync(Dsa *dsa, Controller *c, int target)
{
	setsync(dsa, c, target, 0, c->ccf, 0, 0);
}

static void
setwide(Dsa *dsa, Controller *c, int target, uchar wide)
{
	c->scntl3[target] = wide ? (1 << 3) : 0;
	setasync(dsa, c, target);
	c->s[target] = WideDone;
}

static int
buildsdtrmsg(uchar *buf, uchar tpf, uchar offset)
{
	*buf++ = X_MSG;
	*buf++ = 3;
	*buf++ = X_MSG_SDTR;
	*buf++ = tpf;
	*buf = offset;
	return 5;
}

static int
buildwdtrmsg(uchar *buf, uchar expo)
{
	*buf++ = X_MSG;
	*buf++ = 2;
	*buf++ = X_MSG_WDTR;
	*buf = expo;
	return 4;
}

static void
start(Controller *c, long entry)
{
	ulong p;

	if (c->running)
		panic("ncr53c8xx: start called while running");
	c->running = 1;
	p = c->scriptpa + entry;
	lesetl(c->n->dsp, p);
	if (c->ssm)
		c->n->dcntl |= 0x4;		/* start DMA in SSI mode */
}

static void
ncrcontinue(Controller *c)
{
	if (c->running)
		panic("ncr53c8xx: ncrcontinue called while running");
	/* set the start DMA bit to continue execution */
	c->running = 1;
	c->n->dcntl |= 0x4;
}

static void
softreset(Controller *c)
{
	Ncr *n = c->n;

	n->istat = Srst;		/* software reset */
	n->istat = 0;
	/* general initialisation */
	n->scid = (1 << 6) | 7;		/* respond to reselect, ID 7 */
	n->respid = 1 << 7;		/* response ID = 7 */

#ifdef INTERNAL_SCLK
	n->stest1 = 0x80;		/* disable external scsi clock */
#else
	n->stest1 = 0x00;
#endif

	n->stime0 = 0xdd;		/* about 0.5 second timeout on each device */
	n->scntl0 |= 0x8;		/* Enable parity checking */
	
	/* continued setup */
	n->sien0 = 0x8f;
	n->sien1 = 0x04;
	n->dien = 0x7d;
	n->stest3 = 0x80;		/* TolerANT enable */
	c->running = 0;

	if (c->v->feature & BigFifo)
		n->ctest5 = (1 << 5);
	n->dmode = c->v->burst << 6;	/* set burst length bits */
	if (c->v->burst & 4)
		n->ctest5 |= (1 << 2);	/* including overflow into ctest5 bit 2 */
	if (c->v->feature & Prefetch)
		n->dcntl |= (1 << 5);	/* prefetch enable */
	else if (c->v->feature & BurstOpCodeFetch)
		n->dmode |= (1 << 1);	/* burst opcode fetch */
	if (c->v->feature & Differential) {
		/* chip capable */
		if ((c->feature & Differential) || (n->gpreg & 0x8) == 0) {
			/* user enabled, or bit 3 of GPREG clear (Symbios cards) */
			if (n->sstat2 & (1 << 2))
				print("ncr53c8xx: can't go differential; wrong cable\n");
			else {
				n->stest2 = (1 << 5);
				print("ncr53c8xx: differential mode set\n");
			}
		}
	}
	if (c->clockdouble) {
		n->stest1 |= (1 << 3);	/* power up doubler */
		delay(2);
		n->stest3 |= (1 << 5);	/* stop clock */
		n->stest1 |= (1 << 2);	/* enable doubler */
		n->stest3 &= ~(1 << 5);	/* start clock */
		/* pray */
	}
}

static void
msgsm(Dsa *dsa, Controller *c, int msg, int *cont, int *wakeme)
{
	uchar histpf, hisreqack;
	int tpf;
	int scf, xferp;
	int len;

	Ncr *n = c->n;

	switch (c->s[dsa->target]) {
	case SyncInit:
		switch (msg) {
		case A_SIR_MSG_SDTR:
			/* reply to my SDTR */
			histpf = n->scratcha[2];
			hisreqack = n->scratcha[3];
			KPRINT("ncr53c8xx: %d: SDTN response %d %d\n",
			    dsa->target, histpf, hisreqack);

			if (hisreqack == 0)
				setasync(dsa, c, dsa->target);
			else {
				/* hisreqack should be <= c->v->maxsyncoff */
				tpf = chooserate(c, histpf, &scf, &xferp);
				KPRINT("ncr53c8xx: %d: SDTN: using %d %d\n",
				    dsa->target, tpf, hisreqack);
				setsync(dsa, c, dsa->target, tpf < 25, scf, xferp, hisreqack);
			}
			*cont = -2;
			return;
		case A_SIR_EV_PHASE_SWITCH_AFTER_ID:
			/* target ignored ATN for message after IDENTIFY - not SCSI-II */
			KPRINT("ncr53c8xx: %d: illegal phase switch after ID message - SCSI-1 device?\n", dsa->target);
			KPRINT("ncr53c8xx: %d: SDTN: async\n", dsa->target);
			setasync(dsa, c, dsa->target);
			*cont = E_to_decisions;
			return;
		case A_SIR_MSG_REJECT:
			/* rejection of my SDTR */
			KPRINT("ncr53c8xx: %d: SDTN: rejected SDTR\n", dsa->target);
		//async:
			KPRINT("ncr53c8xx: %d: SDTN: async\n", dsa->target);
			setasync(dsa, c, dsa->target);
			*cont = -2;
			return;
		}
		break;
	case WideInit:
		switch (msg) {
		case A_SIR_MSG_WDTR:
			/* reply to my WDTR */
			KPRINT("ncr53c8xx: %d: WDTN: response %d\n",
			    dsa->target, n->scratcha[2]);
			setwide(dsa, c, dsa->target, n->scratcha[2]);
			*cont = -2;
			return;
		case A_SIR_EV_PHASE_SWITCH_AFTER_ID:
			/* target ignored ATN for message after IDENTIFY - not SCSI-II */
			KPRINT("ncr53c8xx: %d: illegal phase switch after ID message - SCSI-1 device?\n", dsa->target);
			setwide(dsa, c, dsa->target, 0);
			*cont = E_to_decisions;
			return;
		case A_SIR_MSG_REJECT:
			/* rejection of my SDTR */
			KPRINT("ncr53c8xx: %d: WDTN: rejected WDTR\n", dsa->target);
			setwide(dsa, c, dsa->target, 0);
			*cont = -2;
			return;
		}
		break;
		
	case NeitherDone:
	case WideDone:
	case BothDone:
		switch (msg) {
		case A_SIR_MSG_WDTR: {
			uchar hiswide, mywide;
			hiswide = n->scratcha[2];
			mywide = (c->v->feature & Wide) != 0;
			KPRINT("ncr53c8xx: %d: WDTN: target init %d\n",
			    dsa->target, hiswide);
			if (hiswide < mywide)
				mywide = hiswide;
			KPRINT("ncr53c8xx: %d: WDTN: responding %d\n",
			    dsa->target, mywide);
			setwide(dsa, c, dsa->target, mywide);
			len = buildwdtrmsg(dsa->msg_out, mywide);
			setmovedata(&dsa->msg_out_buf, DMASEG(dsa->msg_out), len);
			*cont = E_response;
			c->s[dsa->target] = WideResponse;
			return;
		}
		case A_SIR_MSG_SDTR:
#ifdef ASYNC_ONLY
			*cont = E_reject;
			return;
#else
			/* target decides to renegotiate */
			histpf = n->scratcha[2];
			hisreqack = n->scratcha[3];
			KPRINT("ncr53c8xx: %d: SDTN: target init %d %d\n",
			    dsa->target, histpf, hisreqack);
			if (hisreqack == 0) {
				/* he wants asynchronous */
				setasync(dsa, c, dsa->target);
				tpf = 0;
			}
			else {
				/* he wants synchronous */
				tpf = chooserate(c, histpf, &scf, &xferp);
				if (hisreqack > c->v->maxsyncoff)
					hisreqack = c->v->maxsyncoff;
				KPRINT("ncr53c8xx: %d: using %d %d\n",
				    dsa->target, tpf, hisreqack);
				setsync(dsa, c, dsa->target, tpf < 25, scf, xferp, hisreqack);
			}
			/* build my SDTR message */
			len = buildsdtrmsg(dsa->msg_out, tpf, hisreqack);
			setmovedata(&dsa->msg_out_buf, DMASEG(dsa->msg_out), len);
			*cont = E_response;
			c->s[dsa->target] = SyncResponse;
			return;
#endif
		}
		break;
	case WideResponse:
		switch (msg) {
		case A_SIR_EV_RESPONSE_OK:
			c->s[dsa->target] = WideDone;
			KPRINT("ncr53c8xx: %d: WDTN: response accepted\n", dsa->target);
			*cont = -2;
			return;
		case A_SIR_MSG_REJECT:
			setwide(dsa, c, dsa->target, 0);
			KPRINT("ncr53c8xx: %d: WDTN: response REJECTed\n", dsa->target);
			*cont = -2;
			return;
		}
		break;
	case SyncResponse:
		switch (msg) {
		case A_SIR_EV_RESPONSE_OK:
			c->s[dsa->target] = BothDone;
			KPRINT("ncr53c8xx: %d: SDTN: response accepted (%s)\n",
			    dsa->target, phase[n->sstat1 & 7]);
			*cont = -2;
			return;	/* chf */
		case A_SIR_MSG_REJECT:
			setasync(dsa, c, dsa->target);
			KPRINT("ncr53c8xx: %d: SDTN: response REJECTed\n", dsa->target);
			*cont = -2;
			return;
		}
		break;
	}
	KPRINT("ncr53c8xx: %d: msgsm: state %d msg %d\n",
	    dsa->target, c->s[dsa->target], msg);
	*wakeme = 1;
	return;
}

void
calcblockdma(Dsa *d, ulong base, ulong count)
{
	ulong blocks;
	if (DEBUG(3))
		blocks = 0;
	else {
		blocks = count / A_BSIZE;
		if (blocks > 255)
			blocks = 255;
	}
	d->dmablks = blocks;
	d->dmaaddr[0] = base;
	d->dmaaddr[1] = base >> 8;
	d->dmaaddr[2] = base >> 16;
	d->dmaaddr[3] = base >> 24;
	setmovedata(&d->data_buf, base + blocks * A_BSIZE, count - blocks * A_BSIZE);
	if (legetl(d->data_buf.dbc) == 0)
		d->flag = 1;
}

ulong
read_mismatch_recover(Controller *c, Ncr *n, Dsa *dsa)
{
	ulong dbc = n->dbc & 0xffffff;
	uchar dfifo = n->dfifo;
	int inchip;
	if (n->ctest5 & (1 << 5))
		inchip = ((dfifo | ((n->ctest5 & 3) << 8)) - (dbc & 0x3ff)) & 0x3ff;
	else
		inchip = ((dfifo & 0x7f) - (dbc & 0x7f)) & 0x7f;
	if (inchip) {
		IPRINT("ncr53c8xx: %d/%d: read_mismatch_recover: DMA FIFO = %d\n",
		    dsa->target, dsa->lun, inchip);
	}
	if (n->sxfer & 0xf) {
		/* SCSI FIFO */
		uchar fifo = n->sstat1 >> 4;
		if (c->v->maxsyncoff > 8)
			fifo |= (n->sstat2 & (1 << 4));
		if (fifo) {
			inchip += fifo;
			IPRINT("ncr53c8xx: %d/%d: read_mismatch_recover: SCSI FIFO = %d\n",
			    dsa->target, dsa->lun, fifo);
		}
	}
	else {
		if (n->sstat0 & (1 << 7)) {
			inchip++;
			IPRINT("ncr53c8xx: %d/%d: read_mismatch_recover: SIDL full\n",
			    dsa->target, dsa->lun);
		}
		if (n->sstat2 & (1 << 7)) {
			inchip++;
			IPRINT("ncr53c8xx: %d/%d: read_mismatch_recover: SIDL msb full\n",
			    dsa->target, dsa->lun);
		}
	}
	USED(inchip);
	return dbc;
}

ulong
write_mismatch_recover(Ncr *n, Dsa *dsa)
{
	ulong dbc = n->dbc & 0xffffff;
	uchar dfifo = n->dfifo;
	int inchip;

	USED(dsa);
	if (n->ctest5 & (1 << 5))
		inchip = ((dfifo | ((n->ctest5 & 3) << 8)) - (dbc & 0x3ff)) & 0x3ff;
	else
		inchip = ((dfifo & 0x7f) - (dbc & 0x7f)) & 0x7f;
#ifdef WMR_DEBUG
	if (inchip) {
		IPRINT("ncr53c8xx: %d/%d: write_mismatch_recover: DMA FIFO = %d\n",
		    dsa->target, dsa->lun, inchip);
	}
#endif
	if (n->sstat0 & (1 << 5)) {
		inchip++;
#ifdef WMR_DEBUG
		IPRINT("ncr53c8xx: %d/%d: write_mismatch_recover: SODL full\n", dsa->target, dsa->lun);
#endif
	}
	if (n->sstat2 & (1 << 5)) {
		inchip++;
#ifdef WMR_DEBUG
		IPRINT("ncr53c8xx: %d/%d: write_mismatch_recover: SODL msb full\n", dsa->target, dsa->lun);
#endif
	}
	if (n->sxfer & 0xf) {
		/* synchronous SODR */
		if (n->sstat0 & (1 << 6)) {
			inchip++;
#ifdef WMR_DEBUG
			IPRINT("ncr53c8xx: %d/%d: write_mismatch_recover: SODR full\n",
			    dsa->target, dsa->lun);
#endif
		}
		if (n->sstat2 & (1 << 6)) {
			inchip++;
#ifdef WMR_DEBUG
			IPRINT("ncr53c8xx: %d/%d: write_mismatch_recover: SODR msb full\n",
			    dsa->target, dsa->lun);
#endif
		}
	}
	/* clear the dma fifo */
	n->ctest3 |= (1 << 2);
	/* wait till done */
	while ((n->dstat & Dfe) == 0)
		;
	return dbc + inchip;
}

#define KPTR(x) ((x) == 0 ? 0 : DMASEG_TO_KADDR(x))

static void
interrupt(Ureg *ur, void *a)
{
	uchar istat;
	ushort sist;
	uchar dstat;
	int wakeme = 0;
	int cont = -1;
	Dsa *dsa;
	Controller *c = a;
	Ncr *n = c->n;

	USED(ur);
	if (DEBUG(1))
		IPRINT("ncr53c8xx: int\n");
ilock(c);
for(;;){
	istat = n->istat;
	if (istat & Intf) {
		Dsa *d;
		int wokesomething = 0;
		if (DEBUG(1))
			IPRINT("ncr53c8xx: Intfly\n");
		n->istat = Intf;
		/* search for structures in A_STATE_DONE */
		for (d = KPTR(legetl(dsalist.head)); d; d = KPTR(legetl(d->next))) {
			if (d->stateb == A_STATE_DONE) {
				d->p9status = STATUS_COMPLETE | d->status;
				if (DEBUG(1))
					IPRINT("ncr53c8xx: waking up dsa %lux\n", (ulong)d);
				wakeup(d);
				wokesomething = 1;
			}
		}
		if (!wokesomething)
			IPRINT("ncr53c8xx: nothing to wake up\n");
continue;
	}

	if ((istat & (Sip | Dip)) == 0) {
		if (DEBUG(1))
			IPRINT("ncr53c8xx: int end %x\n", istat);
		unlock(c);
		return;
	}

	sist = (n->sist1<<8)|n->sist0;	/* BUG? can two-byte read be inconsistent? */
	dstat = n->dstat;
	dsa = (Dsa *)DMASEG_TO_KADDR(legetl(n->dsa));
	c->running = 0;
	if (istat & Sip) {
		if (DEBUG(1))
			IPRINT("sist = %.4x\n", sist);
		if (sist & 0x80) {
			ulong addr;
			ulong sa;
			ulong dbc;
			ulong tbc;
			int dmablks;
			ulong dmaaddr;

			addr = legetl(n->dsp);
			sa = addr - c->scriptpa;
			if (DEBUG(1) || DEBUG(2))
				IPRINT("ncr53c8xx: %d/%d: Phase Mismatch sa=%.8lux\n",
				    dsa->target, dsa->lun, sa);
			/*
			 * now recover
			 */
			if (sa == E_data_in_mismatch) {
				dbc = read_mismatch_recover(c, n, dsa);
				tbc = legetl(dsa->data_buf.dbc) - dbc;
				advancedata(&dsa->data_buf, tbc);
				if (DEBUG(1) || DEBUG(2))
					IPRINT("ncr53c8xx: %d/%d: transferred = %ld residue = %ld\n",
					    dsa->target, dsa->lun, tbc, legetl(dsa->data_buf.dbc));
				cont = E_to_decisions;
			}
			else if (sa == E_data_in_block_mismatch) {
				dbc = read_mismatch_recover(c, n, dsa);
				tbc = A_BSIZE - dbc;
				/* recover current state from registers */
				dmablks = n->scratcha[2];
				dmaaddr = legetl(n->scratchb);
				/* we have got to dmaaddr + tbc */
				/* we have dmablks * A_BSIZE - tbc + residue left to do */
				/* so remaining transfer is */
				IPRINT("in_block_mismatch: dmaaddr = 0x%lx tbc=%lud dmablks=%d\n",
				    dmaaddr, tbc, dmablks);
				calcblockdma(dsa, dmaaddr + tbc,
				    dmablks * A_BSIZE - tbc + legetl(dsa->data_buf.dbc));
				/* copy changes into scratch registers */
				IPRINT("recalc: dmablks %d dmaaddr 0x%lx pa 0x%lx dbc %ld\n",
				    dsa->dmablks, legetl(dsa->dmaaddr),
				    legetl(dsa->data_buf.pa), legetl(dsa->data_buf.dbc));
				n->scratcha[2] = dsa->dmablks;
				n->scratchbncr = dsa->dmancr;
				cont = E_data_block_mismatch_recover;
			}
			else if (sa == E_data_out_mismatch) {
				dbc = write_mismatch_recover(n, dsa);
				tbc = legetl(dsa->data_buf.dbc) - dbc;
				advancedata(&dsa->data_buf, tbc);
				if (DEBUG(1) || DEBUG(2))
					IPRINT("ncr53c8xx: %d/%d: transferred = %ld residue = %ld\n",
					    dsa->target, dsa->lun, tbc, legetl(dsa->data_buf.dbc));
				cont = E_to_decisions;
			}
			else if (sa == E_data_out_block_mismatch) {
				dbc = write_mismatch_recover(n, dsa);
				tbc = legetl(dsa->data_buf.dbc) - dbc;
				/* recover current state from registers */
				dmablks = n->scratcha[2];
				dmaaddr = legetl(n->scratchb);
				/* we have got to dmaaddr + tbc */
				/* we have dmablks blocks - tbc + residue left to do */
				/* so remaining transfer is */
				IPRINT("out_block_mismatch: dmaaddr = %lx tbc=%lud dmablks=%d\n",
				    dmaaddr, tbc, dmablks);
				calcblockdma(dsa, dmaaddr + tbc,
				    dmablks * A_BSIZE - tbc + legetl(dsa->data_buf.dbc));
				/* copy changes into scratch registers */
				n->scratcha[2] = dsa->dmablks;
				n->scratchbncr = dsa->dmancr;
				cont = E_data_block_mismatch_recover;
			}
			else if (sa == E_id_out_mismatch) {
				/*
				 * target switched phases while attention held during
				 * message out. The possibilities are:
				 * 1. It didn't like the last message. This is indicated
				 *    by the new phase being message_in. Use script to recover
				 *
				 * 2. It's not SCSI-II compliant. The new phase will be other
				 *    than message_in. We should also indicate that the device
				 *    is asynchronous, if it's the SDTR that got ignored
				 * 
				 * For now, if the phase switch is not to message_in, and
				 * and it happens after IDENTIFY and before SDTR, we
				 * notify the negotiation state machine.
				 */
				ulong lim = legetl(dsa->msg_out_buf.dbc);
				uchar p = n->sstat1 & 7;
				dbc = write_mismatch_recover(n, dsa);
				tbc = lim - dbc;
				IPRINT("ncr53c8xx: %d/%d: msg_out_mismatch: %lud/%lud sent, phase %s\n",
				    dsa->target, dsa->lun, tbc, lim, phase[p]);
				if (p != MessageIn && tbc == 1) {
					msgsm(dsa, c, A_SIR_EV_PHASE_SWITCH_AFTER_ID, &cont, &wakeme);
				}
				else
					cont = E_id_out_mismatch_recover;
			}
			else {
				IPRINT("ncr53c8xx: %d/%d: ma sa=%.8lux wanted=%s got=%s\n",
				    dsa->target, dsa->lun, sa,
				    phase[n->dcmd & 7],
				    phase[n->sstat1 & 7]);
				dumpncrregs(c, 1);
				dsa->p9status = STATUS_FAIL;	/* chf */
				wakeme = 1;
			}
		}
		/*else*/ if (sist & 0x400) {
			if (DEBUG(0))
				IPRINT("ncr53c8xx: %d/%d Sto\n", dsa->target, dsa->lun);
			dsa->p9status = STATUS_SELECTION_TIMEOUT;
			dsa->stateb = A_STATE_DONE;
			softreset(c);
			cont = E_issue_check;
			wakeme = 1;
		}
		if (sist & 0x1) {
			IPRINT("ncr53c8xx: %d/%d: parity error\n", dsa->target, dsa->lun);
			dsa->parityerror = 1;
		}
		if (sist & 0x4) {
			IPRINT("ncr53c8xx: %d/%d: unexpected disconnect\n",
			    dsa->target, dsa->lun);
			dumpncrregs(c, 1);
			//wakeme = 1;
			dsa->p9status = STATUS_FAIL;
		}
	}
	if (istat & Dip) {
		if (DEBUG(1))
			IPRINT("dstat = %.2x\n", dstat);
		/*else*/ if (dstat & Ssi) {
			ulong *p = DMASEG_TO_KADDR(legetl(n->dsp));
			ulong w = (uchar *)p - (uchar *)c->script;
			IPRINT("[%lux]", w);
			USED(w);
			cont = -2;	/* restart */
		}
		if (dstat & Sir) {
			switch (legetl(n->dsps)) {
			case A_SIR_MSG_IO_COMPLETE:
				dsa->p9status = STATUS_COMPLETE | dsa->status;
				wakeme = 1;
				break;
			case A_SIR_MSG_SDTR:
			case A_SIR_MSG_WDTR:
			case A_SIR_MSG_REJECT:
			case A_SIR_EV_RESPONSE_OK:
				msgsm(dsa, c, legetl(n->dsps), &cont, &wakeme);
				break;
			case A_SIR_MSG_IGNORE_WIDE_RESIDUE:
				/* back up one in the data transfer */
				IPRINT("ncr53c8xx: %d/%d: ignore wide residue %d, WSR = %d\n",
				    dsa->target, dsa->lun, n->scratcha[1], n->scntl2 & 1);
				if (dsa->dmablks == 0 && dsa->flag)
					IPRINT("ncr53c8xx: %d/%d: transfer over; residue ignored\n",
					    dsa->target, dsa->lun);
				else
					calcblockdma(dsa, legetl(dsa->dmaaddr) - 1,
					    dsa->dmablks * A_BSIZE + legetl(dsa->data_buf.dbc) + 1);
				cont = -2;
				break;
			case A_SIR_NOTIFY_MSG_IN:
				IPRINT("ncr53c8xx: %d/%d: msg_in %d\n",
				    dsa->target, dsa->lun, n->sfbr);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DISC:
				IPRINT("ncr53c8xx: %d/%d: disconnect:", dsa->target, dsa->lun);
				goto dsadump;
			case A_SIR_NOTIFY_STATUS:
				IPRINT("ncr53c8xx: %d/%d: status\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_COMMAND:
				IPRINT("ncr53c8xx: %d/%d: commands\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DATA_IN:
				IPRINT("ncr53c8xx: %d/%d: data in a %lx b %lx\n",
				    dsa->target, dsa->lun, legetl(n->scratcha), legetl(n->scratchb));
				cont = -2;
				break;
			case A_SIR_NOTIFY_BLOCK_DATA_IN:
				IPRINT("ncr53c8xx: %d/%d: block data in: a2 %x b %lx\n",
				    dsa->target, dsa->lun, n->scratcha[2], legetl(n->scratchb));
				cont = -2;
				break;
			case A_SIR_NOTIFY_DATA_OUT:
				IPRINT("ncr53c8xx: %d/%d: data out\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DUMP:
				IPRINT("ncr53c8xx: %d/%d: dump\n", dsa->target, dsa->lun);
				dumpncrregs(c, 1);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DUMP2:
				IPRINT("ncr53c8xx: %d/%d: dump2:", dsa->target, dsa->lun);
				IPRINT(" sa %lux", legetl(n->dsp) - c->scriptpa);
				IPRINT(" dsa %lux", legetl(n->dsa));
				IPRINT(" sfbr %ux", n->sfbr);
				IPRINT(" a %lux", (ulong)n->scratcha);
				IPRINT(" b %lux", legetl(n->scratchb));
				IPRINT(" ssid %ux", n->ssid);
				IPRINT("\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_WAIT_RESELECT:
				IPRINT("ncr53c8xx: wait reselect\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_RESELECT:
				IPRINT("ncr53c8xx: reselect: ssid %.2x sfbr %.2x at %ld\n",
				    n->ssid, n->sfbr, TK2MS(m->ticks));
				cont = -2;
				break;
			case A_SIR_NOTIFY_ISSUE:
				IPRINT("ncr53c8xx: %d/%d: issue:", dsa->target, dsa->lun);
			dsadump:
				IPRINT(" tgt=%d", dsa->target);
				IPRINT(" time=%ld", TK2MS(m->ticks));
				IPRINT("\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_ISSUE_CHECK:
				IPRINT("ncr53c8xx: issue check\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_SIGP:
				IPRINT("ncr53c8xx: responded to SIGP\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_DUMP_NEXT_CODE: {
				ulong *dsp = DMASEG_TO_KADDR(legetl(n->dsp));
				int x;
				IPRINT("ncr53c8xx: code at %lux", dsp - c->script);
				for (x = 0; x < 6; x++)
					IPRINT(" %.8lux", dsp[x]);
				IPRINT("\n");
				USED(dsp);
				cont = -2;
				break;
			}
			case A_SIR_NOTIFY_WSR:
				IPRINT("ncr53c8xx: %d/%d: WSR set\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_LOAD_SYNC:
				IPRINT("ncr53c8xx: %d/%d: scntl=%.2x sxfer=%.2x\n",
				    dsa->target, dsa->lun, n->scntl3, n->sxfer);
				cont = -2;
				break;
			case A_SIR_NOTIFY_RESELECTED_ON_SELECT:
				IPRINT("ncr53c8xx: %d/%d: reselected during select\n",
				    dsa->target, dsa->lun);
				cont = -2;
				break;
			default:
				IPRINT("ncr53c8xx: %d/%d: script error %lud\n",
					dsa->target, dsa->lun, legetl(n->dsps));
				dumpncrregs(c, 1);
				wakeme = 1;
			}
		}
		/*else*/ if (dstat & Iid) {
			ulong addr = legetl(n->dsp);
			IPRINT("ncr53c8xx: %d/%d: Iid pa=%.8lux sa=%.8lux dbc=%lux\n",
			    dsa->target, dsa->lun,
			    addr, addr - c->scriptpa, n->dbc);
			addr = (ulong)DMASEG_TO_KADDR(addr);
			IPRINT("%.8lux %.8lux %.8lux\n",
			    *(ulong *)(addr - 12), *(ulong *)(addr - 8), *(ulong *)(addr - 4));
			USED(addr);
			dsa->p9status = STATUS_FAIL;
			wakeme = 1;
		}
		/*else*/ if (dstat & Bf) {
			IPRINT("ncr53c8xx: Bus Fault\n");
			dumpncrregs(c, 1);
			dsa->p9status = STATUS_FAIL;
			wakeme = 1;
		}
	}
	if (cont == -2)
		ncrcontinue(c);
	else if (cont >= 0)
		start(c, cont);
	if (wakeme){
		if(dsa->p9status == 0xffff)
			dsa->p9status = STATUS_FAIL;
		wakeup(dsa);
	}
}
iunlock(c);
	if (DEBUG(1)) {
		IPRINT("ncr53c8xx: int end 1\n");
	}
}

static int
done(void *arg)
{
	return ((Dsa *)arg)->p9status != 0xffff;
}

#define offsetof(s, t) ((ulong)&((s *)0)->t)

static int
xfunc(enum na_external x, unsigned long *v)
{
	switch (x)
	{
	case X_scsi_id_buf:
		*v = offsetof(Dsa, scsi_id_buf[0]); return 1;
	case X_msg_out_buf:
		*v = offsetof(Dsa, msg_out_buf); return 1;
	case X_cmd_buf:
		*v = offsetof(Dsa, cmd_buf); return 1;
	case X_data_buf:
		*v = offsetof(Dsa, data_buf); return 1;
	case X_status_buf:
		*v = offsetof(Dsa, status_buf); return 1;
	case X_dsa_head:
		*v = DMASEG(&dsalist.head[0]); return 1;
	default:
		print("xfunc: can't find external %d\n", x);
		return 0;
	}
	return 1;
}

static void
setmovedata(Movedata *d, ulong pa, ulong bc)
{
	d->pa[0] = pa;
	d->pa[1] = pa>>8;
	d->pa[2] = pa>>16;
	d->pa[3] = pa>>24;
	d->dbc[0] = bc;
	d->dbc[1] = bc>>8;
	d->dbc[2] = bc>>16;
	d->dbc[3] = bc>>24;
}

static void
advancedata(Movedata *d, long v)
{
	lesetl(d->pa, legetl(d->pa) + v);
	lesetl(d->dbc, legetl(d->dbc) - v);
}

static void
dumpwritedata(uchar *data, int datalen)
{
	int i;
	uchar *bp;
	if (!DEBUG(0))
		return;

	if (datalen) {
		KPRINT("ncr53c8xx:write:");
		for (i = 0, bp = data; i < 50 && i < datalen; i++, bp++)
			KPRINT("%.2ux", *bp);
		if (i < datalen) {
			KPRINT("...");
		}
		KPRINT("\n");
	}
}

static void
dumpreaddata(uchar *data, int datalen)
{
	int i;
	uchar *bp;
	if (!DEBUG(0))
		return;

	if (datalen) {
		KPRINT("ncr53c8xx:read:");
		for (i = 0, bp = data; i < 50 && i < datalen; i++, bp++)
			KPRINT("%.2ux", *bp);
		if (i < datalen) {
			KPRINT("...");
		}
		KPRINT("\n");
	}
}

static void
busreset(Controller *c)
{
	int x;

	/* bus reset */
	c->n->scntl1 |= (1 << 3);
	delay(500);
	c->n->scntl1 &= ~(1 << 3);
	for (x = 0; x < 8; x++) {
		setwide(0, c, x, 0);
#ifndef ASYNC_ONLY
		c->s[x] = NeitherDone;
#endif
	}
	c->capvalid = 0;
}

static void
reset(Controller *c)
{
	/* should wakeup all pending tasks */
	softreset(c);
	busreset(c);
}

#ifdef SINGLE_TARGET
#define TARG 0
#else
#define TARG target
#endif

static int
io(Controller *c, uchar target, uchar lun, int rw, uchar *cmd, int cmdlen, uchar *data, int datalen,
    int *transferred)
{
	uchar *bp;
	int bc;
	Dsa *d;
	Ncr *n = c->n;
	uchar target_expo, my_expo;
	ushort status;

	d = dsaalloc(target, lun);

	if (c->ssm)
		n->dcntl |= 0x10;		/* SSI */
	
	/* load the transfer control stuff */
	d->scsi_id_buf[0] = 0;
	d->scsi_id_buf[1] = c->sxfer[target];
	d->scsi_id_buf[2] = target;
	d->scsi_id_buf[3] = c->scntl3[target];
	synctodsa(d, c);
	
	bc = 0;

	d->msg_out[bc] = 0x80 | lun;

#ifndef NO_DISCONNECT
	d->msg_out[bc] |= (1 << 6);
#endif
	bc++;

	/* work out what to do about negotiation */
	switch (c->s[target]) {
	default:
		KPRINT("ncr53c8xx: %d: strange nego state %d\n", target, c->s[target]);
		c->s[target] = NeitherDone;
		/* fall through */
	case NeitherDone:
		if ((c->capvalid & (1 << target)) == 0)
			break;
		target_expo = (c->cap[target] >> 5) & 3;
		my_expo = (c->v->feature & Wide) != 0;
		if (target_expo < my_expo)
			my_expo = target_expo;
#ifdef ALWAYS_DO_WDTR
		bc += buildwdtrmsg(d->msg_out + bc, my_expo);
		KPRINT("ncr53c8xx: %d: WDTN: initiating expo %d\n", target, my_expo);
		c->s[target] = WideInit;
		break;
#else
		if (my_expo) {
			bc += buildwdtrmsg(d->msg_out + bc, (c->v->feature & Wide) ? 1 : 0);
			KPRINT("ncr53c8xx: %d: WDTN: initiating expo %d\n", target, my_expo);
			c->s[target] = WideInit;
			break;
		}
		KPRINT("ncr53c8xx: %d: WDTN: narrow\n", target);
		/* fall through */
#endif
	case WideDone:
		if (c->cap[target] & (1 << 4)) {
			KPRINT("ncr53c8xx: %d: SDTN: initiating\n", target);
			bc += buildsdtrmsg(d->msg_out + bc, c->tpf, c->v->maxsyncoff);
			c->s[target] = SyncInit;
			break;
		}
		KPRINT("ncr53c8xx: %d: SDTN: async only\n", target);
		c->s[target] = BothDone;
		break;

	case BothDone:
		break;
	}

	setmovedata(&d->msg_out_buf, DMASEG(d->msg_out), bc);
	setmovedata(&d->cmd_buf, DMASEG(cmd), cmdlen);
	calcblockdma(d, DMASEG(data), datalen);

	if (DEBUG(0)) {
		KPRINT("ncr53c8xx: %d/%d: exec: ", target, lun);
		for (bp = cmd; bp < &cmd[cmdlen]; bp++)
			KPRINT("%.2ux", *bp);
		KPRINT("\n");
		if (rw)
			KPRINT("ncr53c8xx: %d/%d: exec: limit=(%d)%ld\n",
			    target, lun, d->dmablks, legetl(d->data_buf.dbc));
		else
			dumpwritedata(data, datalen);
	}

	setmovedata(&d->status_buf, DMASEG(&d->status), 1);	

	d->p9status = 0xffff;
	d->parityerror = 0;

	qlock(&c->q[TARG]);			/* obtain access to target */

	d->stateb = A_STATE_ISSUE;		/* start operation */

	ilock(c);
	if (c->running) {
		n->istat |= Sigp;
	}
	else {
		start(c, E_issue_check);
	}
	iunlock(c);

#ifdef CPU
	while(waserror())
		;
#ifdef QUICK_TIMEOUT
	tsleep(d, done, d, 30 * 1000);
#else
	tsleep(d, done, d, 60 * 60 * 1000);
#endif
	poperror();

	if (!done(d)) {
		KPRINT("ncr53c8xx: %d/%d: exec: Timed out", target, lun);
		dumpncrregs(c, 0);
		dsafree(d);
		reset(c);
		qunlock(&c->q[TARG]);
		error(Eio);
	}
#endif

#ifdef FS
	sleep(d, done, d);
#endif

	if (d->p9status == STATUS_SELECTION_TIMEOUT)
		c->s[target] = NeitherDone;
	if (d->parityerror) {
		d->p9status = STATUS_FAIL;
	}
	if (DEBUG(0))
		KPRINT("ncr53c8xx: %d/%d: exec: status=%.4x\n",
		    target, lun, d->p9status);
	/*
	 * adjust datalen
	 */
	if (d->dmablks > 0)
		datalen -= d->dmablks * A_BSIZE;
	else if (d->flag == 0)
		datalen -= legetl(d->data_buf.dbc);
	if (transferred)
		*transferred = datalen;
	if (rw)
		dumpreaddata(data, datalen);
	/*
	 * spot the identify
	 */
	if ((c->capvalid & (1 << target)) == 0 &&
	    d->p9status == STATUS_COMPLETE && cmd[0] == 0x12 && datalen >= 8) {
		c->capvalid |= 1 << target;
		c->cap[target] = data[7];
		KPRINT("ncr53c8xx: %d: capabilities %.2x\n", target, data[7]);
	}
	status = d->p9status;
	dsafree(d);
	qunlock(&c->q[TARG]);
	return status;
}

#ifdef CPU
static int
exec(Scsi *p, int rw)
{
	int transferred;
	Controller *c = &controller;		/* MULTIPLE - map from scsi bus to controller instance */

	p->status = io(c, p->target, p->lun, rw,
	    p->cmd.base, p->cmd.lim - p->cmd.base,
	    p->data.base, p->data.lim - p->data.base, &transferred);
	p->data.ptr = p->data.base + transferred;
	return p->status;	
}
#endif

#ifdef Plan9fs
static int
exec(Device d, int rw, uchar *cmd, int clen, void *data, int dlen)
{
	Controller *c = &controller;		/* MULTIPLE - map from scsi bus to controller instance */

	return io(c, d.unit, (cmd[1] >> 5) & 7, rw, cmd, clen, data, dlen, 0);
}
#endif /* Plan9fs

#ifdef FS
static int
exec(Target* t, int rw, uchar* cmd, int cbytes, void* data, int* dbytes)
{
	int n, s;
	Controller *ctlr;

	if((ctlr = ctlrxx[t->ctlrno]) == nil || ctlr->n == nil)
		return STharderr;
	if(t->targetno == 0x07)
		return STownid;
	if(dbytes)
		n = *dbytes;
	else
		n = 0;
	s = io(ctlr, t->targetno, (cmd[1] >> 5) & 7, rw, cmd, cbytes, data, n, dbytes);
	switch(s){
	case STATUS_COMPLETE:
		return STok;
	case STATUS_FAIL:
		return STharderr;
	case STATUS_SELECTION_TIMEOUT:
		return STtimeout;
	case 0x6002:
		return STcheck;
	default:
		print("scsi#%d: exec status 0x%ux\n", ctlr->ctlrno, s);
		return STharderr;
	}
}
#endif

#define NCR_VID 	0x1000
#define NCR_810_DID 	0x0001
#define NCR_820_DID	0x0002	/* don't know enough about this one to support it */
#define NCR_825_DID	0x0003
#define NCR_815_DID	0x0004
#define SYM_810AP_DID	0x0005
#define SYM_860_DID	0x0006
#define SYM_896_DID	0x000b
#define SYM_895_DID	0x000c
#define SYM_885_DID	0x000d	/* ditto */
#define SYM_875_DID	0x000f	/* ditto */

static Variant variant[] = {
{ NCR_810_DID,   0x0f, "NCR53C810",	Burst16,   8, 0 },
{ NCR_810_DID,   0x1f, "SYM53C810ALV",	Burst16,   8, Prefetch },
{ NCR_810_DID,   0xff, "SYM53C810A",	Burst16,   8, Prefetch },
{ SYM_810AP_DID, 0xff, "SYM53C810AP",	Burst16,   8, Prefetch },
{ NCR_815_DID,   0xff, "NCR53C815",	Burst16,   8, BurstOpCodeFetch },
{ NCR_825_DID,   0x0f, "NCR53C825",	Burst16,   8, Wide|BurstOpCodeFetch|Differential },
{ NCR_825_DID,   0xff, "SYM53C825A",	Burst128, 16, Prefetch|LocalRAM|BigFifo|Differential|Wide },
{ SYM_860_DID,   0x0f, "SYM53C860",	Burst16,   8, Prefetch|Ultra },
{ SYM_860_DID,   0xff, "SYM53C860LV",	Burst16,   8, Prefetch|Ultra },
{ SYM_875_DID,   0x01, "SYM53C875r1",	Burst128, 16, Prefetch|LocalRAM|BigFifo|Differential|Wide|Ultra },
{ SYM_875_DID,   0xff, "SYM53C875",	Burst128, 16, Prefetch|LocalRAM|BigFifo|Differential|Wide|Ultra|ClockDouble },
{ SYM_885_DID,   0xff, "SYM53C885",	Burst128, 16, Prefetch|LocalRAM|BigFifo|Wide|Ultra|ClockDouble },
{ SYM_896_DID,   0xff, "SYM53C896",	Burst128, 16, Prefetch|LocalRAM|BigFifo|Wide|Ultra|ClockDouble },
};
#define MAXVAR (sizeof(variant)/sizeof(variant[0]))

#ifndef NAINCLUDE
static int
na_fixup(ulong *script, ulong pa_script, ulong pa_reg,
    struct na_patch *patch, int patches,
    int (*externval)(int x, ulong *v))
{
	int p;
	int v;
	unsigned long lw, lv;
	for (p = 0; p < patches; p++) {
		switch (patch[p].type) {
		case 1:
			/* script relative */
			script[patch[p].lwoff] += pa_script;
			break;
		case 2:
			/* register i/o relative */
			script[patch[p].lwoff] += pa_reg;
			break;
		case 3:
			/* data external */
			lw = script[patch[p].lwoff];
			v = (lw >> 8) & 0xff;
			if (!(*externval)(v, &lv))
				return 0;
			v = lv & 0xff;
			script[patch[p].lwoff] = (lw & 0xffff00ffL) | (v << 8);
			break;
		case 4:
			/* 32 bit external */
			lw = script[patch[p].lwoff];
			if (!(*externval)(lw, &lv))
				return 0;
			script[patch[p].lwoff] = lv;
			break;
		case 5:
			/* 24 bit external */
			lw = script[patch[p].lwoff];
			if (!(*externval)(lw & 0xffffff, &lv))
				return 0;
			script[patch[p].lwoff] = (lw & 0xff000000L) | (lv & 0xffffffL);
			break;
		}
	}
	return 1;
}
#endif /* NAINCLUDE */

typedef struct Adapter {
	Pcidev*	pcidev;
	int	port;
	int	irq;
	int	rid;
} Adapter;
static Msgbuf* adapter;

static void
scanpci(void)
{
	Pcidev *p;
	Msgbuf *mb, *last;
	Adapter *ap;

	p = nil;
	last = nil;
	while(p = pcimatch(p, NCR_VID, 0)){
		mb = mballoc(sizeof(Adapter), 0, Mxxx);
		ap = (Adapter*)mb->data;
		ap->pcidev = p;
		ap->port = p->mem[0].bar & ~0x01;

		if(adapter == nil)
			adapter = mb;
		else
			last->next = mb;
		last = mb;
	}
}

static int
init(Controller* ctlr, ISAConf* isa, int differential)
{
	int is64, var, rid;
	ulong regpa, scriptpa;
	Pcidev *pcidev;
	Msgbuf *mb, **mbb;
	Adapter *ap;
	extern ulong upamalloc(ulong, int, int);

	/*
	 * Any adapter matches if no isa->port is supplied,
	 * otherwise the ports must match.
	 */
	pcidev = nil;
	mbb = &adapter;
	for(mb = *mbb; mb; mb = mb->next){
		ap = (Adapter*)mb->data;
		if(isa->port == 0 || isa->port == ap->port){
			pcidev = ap->pcidev;
			*mbb = mb->next;
			mbfree(mb);
			break;
		}
		mbb = &mb->next;
	}
	if(pcidev == nil)
		return 0;

	rid = pcicfgr8(pcidev, PciRID);
	for (var = 0; var < MAXVAR; var++) {
		if (pcidev->did == variant[var].did && rid <= variant[var].maxrid)
			break;
	}
	if (var >= MAXVAR)
		return 0;
	print("scsi#%d: %s rev. 0x%.2x intr=%d command=%.4x\n",
		ctlr->ctlrno, variant[var].name, rid,
		pcidev->intl, pcicfgr16(pcidev, PciPCR));

	is64 = pcidev->mem[1].bar & 0x04;
	if(is64 && pcidev->mem[2].bar){
		print("scsi#%d: registers in 64-bit space\n",
			ctlr->ctlrno);
		return 0;
	}
	regpa = upamalloc(pcidev->mem[1].bar & ~0x0F, pcidev->mem[1].size, 0);
	if (regpa == 0) {
		print("scsi#%d: failed to map registers\n", ctlr->ctlrno);
		return 0;
	}
	ctlr->n = KADDR(regpa);

	ctlr->v = &variant[var];

	scriptpa = 0;
	if ((ctlr->v->feature & LocalRAM) && sizeof(na_script) <= 4096) {
		if(is64){
			if((pcidev->mem[3].bar & 0x04) && pcidev->mem[4].bar){
				print("scsi#%d: RAM in 64-bit space\n",
					ctlr->ctlrno);
				scriptpa = 0;
			}
			else
				scriptpa = upamalloc(pcidev->mem[3].bar & ~0x0F,
					pcidev->mem[3].size, 0);
		}
		else
			scriptpa = upamalloc(pcidev->mem[2].bar & ~0x0F,
					pcidev->mem[2].size, 0);
		if (scriptpa == 0)
			print("scsi#%d: failed to map onboard RAM\n", ctlr->ctlrno);
		else {
			ctlr->script = KADDR(scriptpa);
			ctlr->scriptpa = scriptpa;
			memmove(ctlr->script, na_script, sizeof(na_script));
			print("scsi#%d: local SCRIPT ram enabled\n", ctlr->ctlrno);
		}
	}

	if (ctlr->script == 0) {
		/* either the map failed, or this chip does not have local RAM
		 * it will need a copy of the microcode
		 */
		/*
		 * should allocate memory and copy na_script into it for
		 * multiple controllers here 
		 * single controller version uses the microcode in place
		 */
		ctlr->script = na_script;
		ctlr->scriptpa = DMASEG(na_script);
	}

	/* fixup script */
	if (!na_fixup(ctlr->script, scriptpa, regpa,
	    na_patches, NA_PATCHES, xfunc)) {
		print("script fixup failed\n");
		return 0;
	}

	if (differential)
		ctlr->feature |= Differential;

	swabl(ctlr->script, ctlr->script, sizeof(na_script));

	dsalist.free = 0;
	lesetl(dsalist.head, 0);

	isa->port = (ulong)KADDR(regpa);
	isa->irq = pcidev->intl;
	
	synctabinit(ctlr);
	/*	
	intrenable(isa->irq, interrupt, ctlr, pcidev->tbdf);
	 */
	setvec(IRQBASE + isa->irq, interrupt, ctlr);
	reset(ctlr);

	return 1;
}

#ifdef CPU
int (*
ncr53c8xxreset(void))(Scsi*, int)
{
	ISAConf isaconf;
	int ic;
	int differential = 0;
	int o;
	Controller *c;

	memset(&isaconf, 0, sizeof(isaconf));
	strcpy(isaconf.type, "ncr53c8xx");
	ic = isaconfig("scsi", 0, &isaconf);

	/* search any ISA opts */
	if (ic) {
		for (o = 0; o < isaconf.nopt; o++) {
			if (strcmp(isaconf.opt[o], "diff") == 0)
				differential = 1;
		}
	}

	c = &controller; 			/* MULTIPLE - map from scsi bus to controller instance */
	if (init(c, differential))
		return exec;
	return 0;
}
#endif

#ifdef Plan9fs
int (*
ncr53c8xxreset(int /*ctlrno*/, ISAConf */*isa*/))(Device, int, uchar*, int, void*, int)
{
	Controller *c = &controller;		/* MULTIPLE - map from scsi bus to controller instance */
	if (init(c, 0))
		return exec;
	return 0;
}
#endif /* Plan9fs */

#ifdef FS
Scsiio
ncr53c8xxreset(int ctlrno, ISAConf* isa)
{
	int differential, o;
	Controller *ctlr;
	static int scandone;

	if(scandone == 0){
		scanpci();
		scandone = 1;
	}

	differential = 0;
	for (o = 0; o < isa->nopt; o++) {
		if (strcmp(isa->opt[o], "diff") == 0)
			differential = 1;
	}

	if((ctlr = xalloc(sizeof(Controller))) == 0){
		print("scsi#%d: %s: controller allocation failed\n",
			ctlrno, isa->type);
		return 0;
	}
	ctlrxx[ctlrno] = ctlr;
	ctlr->ctlrno = ctlrno;

	if (init(ctlr, isa, differential))
		return exec;

	return 0;
}
#endif /* FS */
