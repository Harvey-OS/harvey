/*
 * NCR 53c8xx driver for Plan 9
 * Nigel Roles (ngr@cotswold.demon.co.uk)
 *
 * 08/07/99	Ultra2 fixed. Brazil #ifdefs added. Fixed script error 6 diagnostics.
 *
 * 09/06/99	Enhancements to support 895 and 896 correctly. Attempt at Ultra 2 negotiation,
 *		though no device to test with yet.
 *		Variant now contains the number of valid chip registers to assist
 *		dumpncrregs() 
 *
 * 06/10/98	Various bug fixes and Brazil compiler inspired changes from jmk
 *
 * 05/10/98	Small fix to handle command length being greater than expected by device
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

#define MAXTARGET	16		/* can be 8 or 16 */

#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "error.h"

#include "sd.h"
extern SDifc sd53c8xxifc;

#define waserror()	(0)
#define poperror()
typedef struct QLock{ int r; } QLock;
typedef struct Rendez{ int r; } Rendez;
#define	intrenable(irq, f, c, tbdf, name)	setvec(VectorPIC+(irq), f, c);

/**********************************/
/* Portable configuration macros  */
/**********************************/

//#define BOOTDEBUG
//#define ASYNC_ONLY
//#define	INTERNAL_SCLK
//#define ALWAYS_DO_WDTR
#define WMR_DEBUG

/**********************************/
/* CPU specific macros            */
/**********************************/

#ifdef BOOTDEBUG

#define KPRINT oprint
#define IPRINT intrprint
#define DEBUG(n) 0
#define IFLUSH() iflush()

#else

#define KPRINT	if(0)print
#define IPRINT	if(0)print
#define DEBUG(n)	(0)
#define IFLUSH()

#endif /* BOOTDEBUG */

/*******************************/
/* General                     */
/*******************************/

#ifndef DMASEG
#define DMASEG(x) PADDR(x)
#define legetl(x) (*(ulong*)(x))
#define lesetl(x,v) (*(ulong*)(x) = (v))
#define swabl(a,b,c)
#else
#endif /*DMASEG */
#define DMASEG_TO_KADDR(x) KADDR(PADDR(x))
#define KPTR(x) ((x) == 0 ? 0 : DMASEG_TO_KADDR(x))

#define MEGA 1000000L
#ifdef INTERNAL_SCLK
#define	SCLK (33 * MEGA)
#else
#define SCLK (40 * MEGA)
#endif /* INTERNAL_SCLK */
#define ULTRA_NOCLOCKDOUBLE_SCLK (80 * MEGA)

#define MAXSYNCSCSIRATE (5 * MEGA)
#define MAXFASTSYNCSCSIRATE (10 * MEGA)
#define MAXULTRASYNCSCSIRATE (20 * MEGA)
#define MAXULTRA2SYNCSCSIRATE (40 * MEGA)
#define MAXASYNCCORERATE (25 * MEGA)
#define MAXSYNCCORERATE (25 * MEGA)
#define MAXFASTSYNCCORERATE (50 * MEGA)
#define MAXULTRASYNCCORERATE (80 * MEGA)
#define MAXULTRA2SYNCCORERATE (160 * MEGA)


#define X_MSG	1
#define X_MSG_SDTR 1
#define X_MSG_WDTR 3

struct na_patch {
	unsigned lwoff;
	unsigned char type;
};

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

	uchar dbc[3];	/* 24 */
	uchar dcmd;	/* 27 */

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

	uchar scratchb[4];	/* 5c */
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
	int p9status;
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
	ClockQuad = 256,		/* Has clock quadrupler (same as Ultra2) */
	Ultra2 = 256,
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
	uchar registers;		/* number of 32 bit registers */
	unsigned feature;
} Variant;

static unsigned char cf2[] = { 6, 2, 3, 4, 6, 8, 12, 16 };
#define NULTRA2SCF (sizeof(cf2)/sizeof(cf2[0]))
#define NULTRASCF (NULTRA2SCF - 2)
#define NSCF (NULTRASCF - 1)

typedef struct Controller {
	Lock;
	struct {
		uchar scntl3;
		uchar stest2;
	} bios;
	uchar synctab[NULTRA2SCF - 1][8];/* table of legal tpfs */
	NegoState s[MAXTARGET];
	uchar scntl3[MAXTARGET];
	uchar sxfer[MAXTARGET];
	uchar cap[MAXTARGET];		/* capabilities byte from Identify */
	ushort capvalid;		/* bit per target for validity of cap[] */
	ushort wide;			/* bit per target set if wide negotiated */
	ulong sclk;			/* clock speed of controller */
	uchar clockmult;		/* set by synctabinit */
	uchar ccf;			/* CCF bits */
	uchar tpf;			/* best tpf value for this controller */
	uchar feature;			/* requested features */
	int running;			/* is the script processor running? */
	int ssm;			/* single step mode */
	Ncr *n;				/* pointer to registers */
	Variant *v;			/* pointer to variant type */
	ulong *script;			/* where the real script is */
	ulong scriptpa;			/* where the real script is */
	Pcidev* pcidev;
	SDev*	sdev;

	struct {
		Lock;
		uchar head[4];		/* head of free list (NCR byte order) */
		Dsa	*tail;
		Dsa	*freechain;
	} dsalist;

	QLock q[MAXTARGET];		/* queues for each target */
} Controller;

static Controller controller;

/* ISTAT */
enum { Abrt = 0x80, Srst = 0x40, Sigp = 0x20, Sem = 0x10, Con = 0x08, Intf = 0x04, Sip = 0x02, Dip = 0x01 };

/* DSTAT */
enum { Dfe = 0x80, Mdpe = 0x40, Bf = 0x20, Abrted = 0x10, Ssi = 0x08, Sir = 0x04, Iid = 0x01 };

/* SSTAT */
enum { DataOut, DataIn, Cmd, Status, ReservedOut, ReservedIn, MessageOut, MessageIn };

static void setmovedata(Movedata*, ulong, ulong);
static void advancedata(Movedata*, long);
static int bios_set_differential(Controller *c);

static char *phase[] = {
	"data out", "data in", "command", "status",
	"reserved out", "reserved in", "message out", "message in"
};

#ifdef BOOTDEBUG
#define DEBUGSIZE 10240
char debugbuf[DEBUGSIZE];
char *debuglast;

static void
intrprint(char *format, ...)
{
	if (debuglast == 0)
		debuglast = debugbuf;
	debuglast = doprint(debuglast, debugbuf + (DEBUGSIZE - 1), format, (&format + 1));
}

static void
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

static void
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

#include "sd53c8xx.i"

static Dsa *
dsaalloc(Controller *c, int target, int lun)
{
	Dsa *d;

	ilock(&c->dsalist);
	if ((d = c->dsalist.freechain) == 0) {
		d = xalloc(sizeof(*d));
		if (DEBUG(1))
			KPRINT("sd53c8xx: %d/%d: allocated new dsa %lux\n", target, lun, d);
		lesetl(d->next, 0);
		lesetl(d->state, A_STATE_ALLOCATED);
		if (legetl(c->dsalist.head) == 0)
			lesetl(c->dsalist.head, DMASEG(d));	/* ATOMIC?!? */
		else
			lesetl(c->dsalist.tail->next, DMASEG(d));	/* ATOMIC?!? */
		c->dsalist.tail = d;
	}
	else {
		if (DEBUG(1))
			KPRINT("sd53c8xx: %d/%d: reused dsa %lux\n", target, lun, d);
		c->dsalist.freechain = d->freechain;
		lesetl(d->state, A_STATE_ALLOCATED);
	}
	iunlock(&c->dsalist);
	d->target = target;
	d->lun = lun;
	return d;
}

static void
dsafree(Controller *c, Dsa *d)
{
	ilock(&c->dsalist);
	d->freechain = c->dsalist.freechain;
	c->dsalist.freechain = d;
	lesetl(d->state, A_STATE_FREE);
	iunlock(&c->dsalist);
}

static Dsa *
dsafind(Controller *c, uchar target, uchar lun, uchar state)
{
	Dsa *d;
	for (d = KPTR(legetl(c->dsalist.head)); d; d = KPTR(legetl(d->next))) {
		if (d->target != 0xff && d->target != target)
			continue;
		if (lun != 0xff && d->lun != lun)
			continue;
		if (state != 0xff && d->stateb != state)
			continue;
		break;
	}
	return d;
}

static void
dumpncrregs(Controller *c, int intr)
{
	int i;
	Ncr *n = c->n;
	int depth = c->v->registers / 4;

	KPRINT("sa = %.8lux\n", c->scriptpa);
	for (i = 0; i < depth; i++) {
		int j;
		for (j = 0; j < 4; j++) {
			int k = j * depth + i;
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
	int maxscf;

	if (c->v->feature & Ultra2)
		maxscf = NULTRA2SCF;
	else if (c->v->feature & Ultra)
		maxscf = NULTRASCF;
	else
		maxscf = NSCF;

	/*
	 * search large clock factors first since this should
	 * result in more reliable transfers
	 */
	for (scf = maxscf; scf >= 1; scf--) {
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

static void
synctabinit(Controller *c)
{
	int scf;
	unsigned long scsilimit;
	int xferp;
	unsigned long cr, sr;
	int tpf;
	int fast;
	int maxscf;

	if (c->v->feature & Ultra2)
		maxscf = NULTRA2SCF;
	else if (c->v->feature & Ultra)
		maxscf = NULTRASCF;
	else
		maxscf = NSCF;

	/*
	 * for chips with no clock doubler, but Ultra capable (e.g. 860, or interestingly the
	 * first spin of the 875), assume 80MHz
	 * otherwise use the internal (33 Mhz) or external (40MHz) default
	 */

	if ((c->v->feature & Ultra) != 0 && (c->v->feature & (ClockDouble | ClockQuad)) == 0)
		c->sclk = ULTRA_NOCLOCKDOUBLE_SCLK;
	else
		c->sclk = SCLK;

	/*
	 * otherwise, if the chip is Ultra capable, but has a slow(ish) clock,
	 * invoke the doubler
	 */

	if (SCLK <= 40000000) {
		if (c->v->feature & ClockDouble) {
			c->sclk *= 2;
			c->clockmult = 1;
		}
		else if (c->v->feature & ClockQuad) {
			c->sclk *= 4;
			c->clockmult = 1;
		}
		else
			c->clockmult = 0;
	}
	else
		c->clockmult = 0;

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
	else if ((c->v->feature & ClockDouble) && c->sclk <= 80 * MEGA)
		c->ccf = 5;
	else if ((c->v->feature & ClockQuad) && c->sclk <= 120 * MEGA)
		c->ccf = 6;
	else if ((c->v->feature & ClockQuad) && c->sclk <= 160 * MEGA)
		c->ccf = 7;

	for (scf = 1; scf < maxscf; scf++) {
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
		else if ((c->v->feature & Ultra2) && cr <= MAXULTRA2SYNCCORERATE) {
			scsilimit = MAXULTRA2SYNCSCSIRATE;
			fast = 3;
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
			if (tp < 25 || tp > 255 * 4)
				continue;
			/*
			 * spot stupid special case for Ultra or Ultra2
			 * while working out factor
			 */
			if (tp == 25)
				tpf = 10;
			else if (tp == 50)
				tpf = 12;
			else if (tp < 52)
				continue;
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
			case 3:
				ok = xferp == 4 && (tpf == 10 || tpf == 11);
				break;
			default:
				ok = 0;
			}
			if (!ok)
				continue;
			c->synctab[scf - 1][xferp - 4] = tpf;
		}
	}

#ifndef NO_ULTRA2
	if (c->v->feature & Ultra2)
		tpf = 10;
	else
#endif
	if (c->v->feature & Ultra)
		tpf = 12;
	else
		tpf = 25;
	for (; tpf < 256; tpf++) {
		if (chooserate(c, tpf, &scf, &xferp) == tpf) {
			unsigned tp = tpf == 10 ? 25 : (tpf == 12 ? 50 : tpf * 4);
			unsigned long khz = (MEGA + tp - 1) / (tp);
			KPRINT("sd53c8xx: tpf=%d scf=%d.%.1d xferp=%d mhz=%ld.%.3ld\n",
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
		panic("sd53c8xx: start called while running");
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
		panic("sd53c8xx: ncrcontinue called while running");
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
		if ((c->feature & Differential) || bios_set_differential(c)) {
			/* user enabled, or some evidence bios set differential */
			if (n->sstat2 & (1 << 2))
				print("sd53c8xx: can't go differential; wrong cable\n");
			else {
				n->stest2 = (1 << 5);
				print("sd53c8xx: differential mode set\n");
			}
		}
	}
	if (c->clockmult) {
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
			KPRINT("sd53c8xx: %d: SDTN response %d %d\n",
			    dsa->target, histpf, hisreqack);

			if (hisreqack == 0)
				setasync(dsa, c, dsa->target);
			else {
				/* hisreqack should be <= c->v->maxsyncoff */
				tpf = chooserate(c, histpf, &scf, &xferp);
				KPRINT("sd53c8xx: %d: SDTN: using %d %d\n",
				    dsa->target, tpf, hisreqack);
				setsync(dsa, c, dsa->target, tpf < 25, scf, xferp, hisreqack);
			}
			*cont = -2;
			return;
		case A_SIR_EV_PHASE_SWITCH_AFTER_ID:
			/* target ignored ATN for message after IDENTIFY - not SCSI-II */
			KPRINT("sd53c8xx: %d: illegal phase switch after ID message - SCSI-1 device?\n", dsa->target);
			KPRINT("sd53c8xx: %d: SDTN: async\n", dsa->target);
			setasync(dsa, c, dsa->target);
			*cont = E_to_decisions;
			return;
		case A_SIR_MSG_REJECT:
			/* rejection of my SDTR */
			KPRINT("sd53c8xx: %d: SDTN: rejected SDTR\n", dsa->target);
		//async:
			KPRINT("sd53c8xx: %d: SDTN: async\n", dsa->target);
			setasync(dsa, c, dsa->target);
			*cont = -2;
			return;
		}
		break;
	case WideInit:
		switch (msg) {
		case A_SIR_MSG_WDTR:
			/* reply to my WDTR */
			KPRINT("sd53c8xx: %d: WDTN: response %d\n",
			    dsa->target, n->scratcha[2]);
			setwide(dsa, c, dsa->target, n->scratcha[2]);
			*cont = -2;
			return;
		case A_SIR_EV_PHASE_SWITCH_AFTER_ID:
			/* target ignored ATN for message after IDENTIFY - not SCSI-II */
			KPRINT("sd53c8xx: %d: illegal phase switch after ID message - SCSI-1 device?\n", dsa->target);
			setwide(dsa, c, dsa->target, 0);
			*cont = E_to_decisions;
			return;
		case A_SIR_MSG_REJECT:
			/* rejection of my SDTR */
			KPRINT("sd53c8xx: %d: WDTN: rejected WDTR\n", dsa->target);
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
			KPRINT("sd53c8xx: %d: WDTN: target init %d\n",
			    dsa->target, hiswide);
			if (hiswide < mywide)
				mywide = hiswide;
			KPRINT("sd53c8xx: %d: WDTN: responding %d\n",
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
			KPRINT("sd53c8xx: %d: SDTN: target init %d %d\n",
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
				KPRINT("sd53c8xx: %d: using %d %d\n",
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
			KPRINT("sd53c8xx: %d: WDTN: response accepted\n", dsa->target);
			*cont = -2;
			return;
		case A_SIR_MSG_REJECT:
			setwide(dsa, c, dsa->target, 0);
			KPRINT("sd53c8xx: %d: WDTN: response REJECTed\n", dsa->target);
			*cont = -2;
			return;
		}
		break;
	case SyncResponse:
		switch (msg) {
		case A_SIR_EV_RESPONSE_OK:
			c->s[dsa->target] = BothDone;
			KPRINT("sd53c8xx: %d: SDTN: response accepted (%s)\n",
			    dsa->target, phase[n->sstat1 & 7]);
			*cont = -2;
			return;	/* chf */
		case A_SIR_MSG_REJECT:
			setasync(dsa, c, dsa->target);
			KPRINT("sd53c8xx: %d: SDTN: response REJECTed\n", dsa->target);
			*cont = -2;
			return;
		}
		break;
	}
	KPRINT("sd53c8xx: %d: msgsm: state %d msg %d\n",
	    dsa->target, c->s[dsa->target], msg);
	*wakeme = 1;
	return;
}

static void
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

static ulong
read_mismatch_recover(Controller *c, Ncr *n, Dsa *dsa)
{
	ulong dbc;
	uchar dfifo = n->dfifo;
	int inchip;

	dbc = (n->dbc[2]<<16)|(n->dbc[1]<<8)|n->dbc[0];
	if (n->ctest5 & (1 << 5))
		inchip = ((dfifo | ((n->ctest5 & 3) << 8)) - (dbc & 0x3ff)) & 0x3ff;
	else
		inchip = ((dfifo & 0x7f) - (dbc & 0x7f)) & 0x7f;
	if (inchip) {
		IPRINT("sd53c8xx: %d/%d: read_mismatch_recover: DMA FIFO = %d\n",
		    dsa->target, dsa->lun, inchip);
	}
	if (n->sxfer & 0xf) {
		/* SCSI FIFO */
		uchar fifo = n->sstat1 >> 4;
		if (c->v->maxsyncoff > 8)
			fifo |= (n->sstat2 & (1 << 4));
		if (fifo) {
			inchip += fifo;
			IPRINT("sd53c8xx: %d/%d: read_mismatch_recover: SCSI FIFO = %d\n",
			    dsa->target, dsa->lun, fifo);
		}
	}
	else {
		if (n->sstat0 & (1 << 7)) {
			inchip++;
			IPRINT("sd53c8xx: %d/%d: read_mismatch_recover: SIDL full\n",
			    dsa->target, dsa->lun);
		}
		if (n->sstat2 & (1 << 7)) {
			inchip++;
			IPRINT("sd53c8xx: %d/%d: read_mismatch_recover: SIDL msb full\n",
			    dsa->target, dsa->lun);
		}
	}
	USED(inchip);
	return dbc;
}

static ulong
write_mismatch_recover(Ncr *n, Dsa *dsa)
{
	ulong dbc;
	uchar dfifo = n->dfifo;
	int inchip;

	dbc = (n->dbc[2]<<16)|(n->dbc[1]<<8)|n->dbc[0];
	USED(dsa);
	if (n->ctest5 & (1 << 5))
		inchip = ((dfifo | ((n->ctest5 & 3) << 8)) - (dbc & 0x3ff)) & 0x3ff;
	else
		inchip = ((dfifo & 0x7f) - (dbc & 0x7f)) & 0x7f;
#ifdef WMR_DEBUG
	if (inchip) {
		IPRINT("sd53c8xx: %d/%d: write_mismatch_recover: DMA FIFO = %d\n",
		    dsa->target, dsa->lun, inchip);
	}
#endif
	if (n->sstat0 & (1 << 5)) {
		inchip++;
#ifdef WMR_DEBUG
		IPRINT("sd53c8xx: %d/%d: write_mismatch_recover: SODL full\n", dsa->target, dsa->lun);
#endif
	}
	if (n->sstat2 & (1 << 5)) {
		inchip++;
#ifdef WMR_DEBUG
		IPRINT("sd53c8xx: %d/%d: write_mismatch_recover: SODL msb full\n", dsa->target, dsa->lun);
#endif
	}
	if (n->sxfer & 0xf) {
		/* synchronous SODR */
		if (n->sstat0 & (1 << 6)) {
			inchip++;
#ifdef WMR_DEBUG
			IPRINT("sd53c8xx: %d/%d: write_mismatch_recover: SODR full\n",
			    dsa->target, dsa->lun);
#endif
		}
		if (n->sstat2 & (1 << 6)) {
			inchip++;
#ifdef WMR_DEBUG
			IPRINT("sd53c8xx: %d/%d: write_mismatch_recover: SODR msb full\n",
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
		IPRINT("sd53c8xx: int\n");
	ilock(c);
	istat = n->istat;
	if (istat & Intf) {
		Dsa *d;
		int wokesomething = 0;
		if (DEBUG(1))
			IPRINT("sd53c8xx: Intfly\n");
		n->istat = Intf;
		/* search for structures in A_STATE_DONE */
		for (d = KPTR(legetl(c->dsalist.head)); d; d = KPTR(legetl(d->next))) {
			if (d->stateb == A_STATE_DONE) {
				d->p9status = d->status;
				if (DEBUG(1))
					IPRINT("sd53c8xx: waking up dsa %lux\n", d);
				wakeup(d);
				wokesomething = 1;
			}
		}
		if (!wokesomething)
			IPRINT("sd53c8xx: nothing to wake up\n");
	}

	if ((istat & (Sip | Dip)) == 0) {
		if (DEBUG(1))
			IPRINT("sd53c8xx: int end %x\n", istat);
		iunlock(c);
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
				IPRINT("sd53c8xx: %d/%d: Phase Mismatch sa=%.8lux\n",
				    dsa->target, dsa->lun, sa);
			/*
			 * now recover
			 */
			if (sa == E_data_in_mismatch) {
				dbc = read_mismatch_recover(c, n, dsa);
				tbc = legetl(dsa->data_buf.dbc) - dbc;
				advancedata(&dsa->data_buf, tbc);
				if (DEBUG(1) || DEBUG(2))
					IPRINT("sd53c8xx: %d/%d: transferred = %ld residue = %ld\n",
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
				IPRINT("in_block_mismatch: dmaaddr = 0x%lux tbc=%lud dmablks=%d\n",
				    dmaaddr, tbc, dmablks);
				calcblockdma(dsa, dmaaddr + tbc,
				    dmablks * A_BSIZE - tbc + legetl(dsa->data_buf.dbc));
				/* copy changes into scratch registers */
				IPRINT("recalc: dmablks %d dmaaddr 0x%lx pa 0x%lx dbc %ld\n",
				    dsa->dmablks, legetl(dsa->dmaaddr),
				    legetl(dsa->data_buf.pa), legetl(dsa->data_buf.dbc));
				n->scratcha[2] = dsa->dmablks;
				lesetl(n->scratchb, dsa->dmancr);
				cont = E_data_block_mismatch_recover;
			}
			else if (sa == E_data_out_mismatch) {
				dbc = write_mismatch_recover(n, dsa);
				tbc = legetl(dsa->data_buf.dbc) - dbc;
				advancedata(&dsa->data_buf, tbc);
				if (DEBUG(1) || DEBUG(2))
					IPRINT("sd53c8xx: %d/%d: transferred = %ld residue = %ld\n",
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
				IPRINT("out_block_mismatch: dmaaddr = %lux tbc=%lud dmablks=%d\n",
				    dmaaddr, tbc, dmablks);
				calcblockdma(dsa, dmaaddr + tbc,
				    dmablks * A_BSIZE - tbc + legetl(dsa->data_buf.dbc));
				/* copy changes into scratch registers */
				n->scratcha[2] = dsa->dmablks;
				lesetl(n->scratchb, dsa->dmancr);
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
				IPRINT("sd53c8xx: %d/%d: msg_out_mismatch: %lud/%lud sent, phase %s\n",
				    dsa->target, dsa->lun, tbc, lim, phase[p]);
				if (p != MessageIn && tbc == 1) {
					msgsm(dsa, c, A_SIR_EV_PHASE_SWITCH_AFTER_ID, &cont, &wakeme);
				}
				else
					cont = E_id_out_mismatch_recover;
			}
			else if (sa == E_cmd_out_mismatch) {
				/*
				 * probably the command count is longer than the device wants ...
				 */
				ulong lim = legetl(dsa->cmd_buf.dbc);
				uchar p = n->sstat1 & 7;
				dbc = write_mismatch_recover(n, dsa);
				tbc = lim - dbc;
				IPRINT("sd53c8xx: %d/%d: cmd_out_mismatch: %lud/%lud sent, phase %s\n",
				    dsa->target, dsa->lun, tbc, lim, phase[p]);
				USED(p, tbc);
				cont = E_to_decisions;
			}
			else {
				IPRINT("sd53c8xx: %d/%d: ma sa=%.8lux wanted=%s got=%s\n",
				    dsa->target, dsa->lun, sa,
				    phase[n->dcmd & 7],
				    phase[n->sstat1 & 7]);
				dumpncrregs(c, 1);
				dsa->p9status = SDeio;	/* chf */
				wakeme = 1;
			}
		}
		/*else*/ if (sist & 0x400) {
			if (DEBUG(0))
				IPRINT("sd53c8xx: %d/%d Sto\n", dsa->target, dsa->lun);
			dsa->p9status = SDtimeout;
			dsa->stateb = A_STATE_DONE;
			softreset(c);
			cont = E_issue_check;
			wakeme = 1;
		}
		if (sist & 0x1) {
			IPRINT("sd53c8xx: %d/%d: parity error\n", dsa->target, dsa->lun);
			dsa->parityerror = 1;
		}
		if (sist & 0x4) {
			IPRINT("sd53c8xx: %d/%d: unexpected disconnect\n",
			    dsa->target, dsa->lun);
			dumpncrregs(c, 1);
			//wakeme = 1;
			dsa->p9status = SDeio;
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
				dsa->p9status = dsa->status;
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
				IPRINT("sd53c8xx: %d/%d: ignore wide residue %d, WSR = %d\n",
				    dsa->target, dsa->lun, n->scratcha[1], n->scntl2 & 1);
				if (dsa->dmablks == 0 && dsa->flag)
					IPRINT("sd53c8xx: %d/%d: transfer over; residue ignored\n",
					    dsa->target, dsa->lun);
				else
					calcblockdma(dsa, legetl(dsa->dmaaddr) - 1,
					    dsa->dmablks * A_BSIZE + legetl(dsa->data_buf.dbc) + 1);
				cont = -2;
				break;
			case A_SIR_ERROR_NOT_MSG_IN_AFTER_RESELECT:
				IPRINT("sd53c8xx: %d: not msg_in after reselect (%s)",
				    n->ssid & 7, phase[n->sstat1 & 7]);
				dsa = dsafind(c, n->ssid & 7, -1, A_STATE_DISCONNECTED);
				dumpncrregs(c, 1);
				wakeme = 1;
				break;
			case A_SIR_NOTIFY_MSG_IN:
				IPRINT("sd53c8xx: %d/%d: msg_in %d\n",
				    dsa->target, dsa->lun, n->sfbr);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DISC:
				IPRINT("sd53c8xx: %d/%d: disconnect:", dsa->target, dsa->lun);
				goto dsadump;
			case A_SIR_NOTIFY_STATUS:
				IPRINT("sd53c8xx: %d/%d: status\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_COMMAND:
				IPRINT("sd53c8xx: %d/%d: commands\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DATA_IN:
				IPRINT("sd53c8xx: %d/%d: data in a %lx b %lx\n",
				    dsa->target, dsa->lun, legetl(n->scratcha), legetl(n->scratchb));
				cont = -2;
				break;
			case A_SIR_NOTIFY_BLOCK_DATA_IN:
				IPRINT("sd53c8xx: %d/%d: block data in: a2 %x b %lx\n",
				    dsa->target, dsa->lun, n->scratcha[2], legetl(n->scratchb));
				cont = -2;
				break;
			case A_SIR_NOTIFY_DATA_OUT:
				IPRINT("sd53c8xx: %d/%d: data out\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DUMP:
				IPRINT("sd53c8xx: %d/%d: dump\n", dsa->target, dsa->lun);
				dumpncrregs(c, 1);
				cont = -2;
				break;
			case A_SIR_NOTIFY_DUMP2:
				IPRINT("sd53c8xx: %d/%d: dump2:", dsa->target, dsa->lun);
				IPRINT(" sa %lux", legetl(n->dsp) - c->scriptpa);
				IPRINT(" dsa %lux", legetl(n->dsa));
				IPRINT(" sfbr %ux", n->sfbr);
				IPRINT(" a %lux", n->scratcha);
				IPRINT(" b %lux", legetl(n->scratchb));
				IPRINT(" ssid %ux", n->ssid);
				IPRINT("\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_WAIT_RESELECT:
				IPRINT("sd53c8xx: wait reselect\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_RESELECT:
				IPRINT("sd53c8xx: reselect: ssid %.2x sfbr %.2x at %ld\n",
				    n->ssid, n->sfbr, TK2MS(m->ticks));
				cont = -2;
				break;
			case A_SIR_NOTIFY_ISSUE:
				IPRINT("sd53c8xx: %d/%d: issue:", dsa->target, dsa->lun);
			dsadump:
				IPRINT(" tgt=%d", dsa->target);
				IPRINT(" time=%ld", TK2MS(m->ticks));
				IPRINT("\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_ISSUE_CHECK:
				IPRINT("sd53c8xx: issue check\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_SIGP:
				IPRINT("sd53c8xx: responded to SIGP\n");
				cont = -2;
				break;
			case A_SIR_NOTIFY_DUMP_NEXT_CODE: {
				ulong *dsp = DMASEG_TO_KADDR(legetl(n->dsp));
				int x;
				IPRINT("sd53c8xx: code at %lux", dsp - c->script);
				for (x = 0; x < 6; x++)
					IPRINT(" %.8lux", dsp[x]);
				IPRINT("\n");
				USED(dsp);
				cont = -2;
				break;
			}
			case A_SIR_NOTIFY_WSR:
				IPRINT("sd53c8xx: %d/%d: WSR set\n", dsa->target, dsa->lun);
				cont = -2;
				break;
			case A_SIR_NOTIFY_LOAD_SYNC:
				IPRINT("sd53c8xx: %d/%d: scntl=%.2x sxfer=%.2x\n",
				    dsa->target, dsa->lun, n->scntl3, n->sxfer);
				cont = -2;
				break;
			case A_SIR_NOTIFY_RESELECTED_ON_SELECT:
				IPRINT("sd53c8xx: %d/%d: reselected during select\n",
				    dsa->target, dsa->lun);
				cont = -2;
				break;
			default:
				IPRINT("sd53c8xx: %d/%d: script error %ld\n",
					dsa->target, dsa->lun, legetl(n->dsps));
				dumpncrregs(c, 1);
				wakeme = 1;
			}
		}
		/*else*/ if (dstat & Iid) {
			ulong addr = legetl(n->dsp);
			ulong dbc = (n->dbc[2]<<16)|(n->dbc[1]<<8)|n->dbc[0];
			IPRINT("sd53c8xx: %d/%d: Iid pa=%.8lux sa=%.8lux dbc=%lux\n",
			    dsa->target, dsa->lun,
			    addr, addr - c->scriptpa, dbc);
			addr = (ulong)DMASEG_TO_KADDR(addr);
			IPRINT("%.8lux %.8lux %.8lux\n",
			    *(ulong *)(addr - 12), *(ulong *)(addr - 8), *(ulong *)(addr - 4));
			USED(addr, dbc);
			dsa->p9status = SDeio;
			wakeme = 1;
		}
		/*else*/ if (dstat & Bf) {
			IPRINT("sd53c8xx: %d/%d: Bus Fault\n", dsa->target, dsa->lun);
			dumpncrregs(c, 1);
			dsa->p9status = SDeio;
			wakeme = 1;
		}
	}
	if (cont == -2)
		ncrcontinue(c);
	else if (cont >= 0)
		start(c, cont);
	if (wakeme){
		if(dsa->p9status == SDnostatus)
			dsa->p9status = SDeio;
		wakeup(dsa);
	}
	iunlock(c);
	if (DEBUG(1)) {
		IPRINT("sd53c8xx: int end 1\n");
	}
}

static int
done(void *arg)
{
	return ((Dsa *)arg)->p9status != SDnostatus;
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
	if (!DEBUG(0)){
		USED(data, datalen);
		return;
	}

	if (datalen) {
		KPRINT("sd53c8xx:write:");
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
	if (!DEBUG(0)){
		USED(data, datalen);
		return;
	}

	if (datalen) {
		KPRINT("sd53c8xx:read:");
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
	int x, ntarget;

	/* bus reset */
	c->n->scntl1 |= (1 << 3);
	delay(500);
	c->n->scntl1 &= ~(1 << 3);
	if(!(c->v->feature & Wide))
		ntarget = 8;
	else
		ntarget = MAXTARGET;
	for (x = 0; x < ntarget; x++) {
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

static int
symrio(SDreq* r)
{
	Dsa *d;
	uchar *bp;
	Controller *c;
	uchar target_expo, my_expo;
	int bc, check, status, target;

	if((target = r->unit->subno) == 0x07)
		return r->status = SDtimeout;	/* assign */
	c = r->unit->dev->ctlr;

	check = 0;
	d = dsaalloc(c, target, r->lun);

	qlock(&c->q[target]);			/* obtain access to target */
docheck:
	/* load the transfer control stuff */
	d->scsi_id_buf[0] = 0;
	d->scsi_id_buf[1] = c->sxfer[target];
	d->scsi_id_buf[2] = target;
	d->scsi_id_buf[3] = c->scntl3[target];
	synctodsa(d, c);

	bc = 0;

	d->msg_out[bc] = 0x80 | r->lun;

#ifndef NO_DISCONNECT
	d->msg_out[bc] |= (1 << 6);
#endif
	bc++;

	/* work out what to do about negotiation */
	switch (c->s[target]) {
	default:
		KPRINT("sd53c8xx: %d: strange nego state %d\n", target, c->s[target]);
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
		KPRINT("sd53c8xx: %d: WDTN: initiating expo %d\n", target, my_expo);
		c->s[target] = WideInit;
		break;
#else
		if (my_expo) {
			bc += buildwdtrmsg(d->msg_out + bc, (c->v->feature & Wide) ? 1 : 0);
			KPRINT("sd53c8xx: %d: WDTN: initiating expo %d\n", target, my_expo);
			c->s[target] = WideInit;
			break;
		}
		KPRINT("sd53c8xx: %d: WDTN: narrow\n", target);
		/* fall through */
#endif
	case WideDone:
		if (c->cap[target] & (1 << 4)) {
			KPRINT("sd53c8xx: %d: SDTN: initiating %d %d\n", target, c->tpf, c->v->maxsyncoff);
			bc += buildsdtrmsg(d->msg_out + bc, c->tpf, c->v->maxsyncoff);
			c->s[target] = SyncInit;
			break;
		}
		KPRINT("sd53c8xx: %d: SDTN: async only\n", target);
		c->s[target] = BothDone;
		break;

	case BothDone:
		break;
	}

	setmovedata(&d->msg_out_buf, DMASEG(d->msg_out), bc);
	setmovedata(&d->cmd_buf, DMASEG(r->cmd), r->clen);
	calcblockdma(d, DMASEG(r->data), r->dlen);

	if (DEBUG(0)) {
		KPRINT("sd53c8xx: %d/%d: exec: ", target, r->lun);
		for (bp = r->cmd; bp < &r->cmd[r->clen]; bp++)
			KPRINT("%.2ux", *bp);
		KPRINT("\n");
		if (!r->write)
			KPRINT("sd53c8xx: %d/%d: exec: limit=(%d)%ld\n",
			  target, r->lun, d->dmablks, legetl(d->data_buf.dbc));
		else
			dumpwritedata(r->data, r->dlen);
	}

	setmovedata(&d->status_buf, DMASEG(&d->status), 1);	

	d->p9status = SDnostatus;
	d->parityerror = 0;

	d->stateb = A_STATE_ISSUE;		/* start operation */

	ilock(c);
	if (c->ssm)
		c->n->dcntl |= 0x10;		/* SSI */
	if (c->running) {
		c->n->istat |= Sigp;
	}
	else {
		start(c, E_issue_check);
	}
	iunlock(c);

	while(waserror())
		;
	tsleep(d, done, d, 30 * 1000);
	poperror();

	if (!done(d)) {
		KPRINT("sd53c8xx: %d/%d: exec: Timed out\n", target, r->lun);
		dumpncrregs(c, 0);
		dsafree(c, d);
		reset(c);
		qunlock(&c->q[target]);
		r->status = SDtimeout;
		return r->status = SDtimeout;	/* assign */
	}

	if((status = d->p9status) == SDeio)
		c->s[target] = NeitherDone;
	if (d->parityerror) {
		status = SDeio;
	}

	/*
	 * adjust datalen
	 */
	r->rlen = r->dlen;
	if (d->dmablks > 0)
		r->rlen -= d->dmablks * A_BSIZE;
	else if (d->flag == 0)
		r->rlen -= legetl(d->data_buf.dbc);
	if(!r->write)
		dumpreaddata(r->data, r->rlen);
	if (DEBUG(0))
		KPRINT("53c8xx: %d/%d: exec: p9status=%d status %d rlen %ld\n",
		    target, r->lun, d->p9status, status, r->rlen);
	/*
	 * spot the identify
	 */
	if ((c->capvalid & (1 << target)) == 0
	 && (status == SDok || status == SDcheck)
	 && r->cmd[0] == 0x12 && r->dlen >= 8) {
		c->capvalid |= 1 << target;
		bp = r->data;
		c->cap[target] = bp[7];
		KPRINT("sd53c8xx: %d: capabilities %.2x\n", target, bp[7]);
	}
	if(!check && status == SDcheck && !(r->flags & SDnosense)){
		check = 1;
		r->write = 0;
		memset(r->cmd, 0, sizeof(r->cmd));
		r->cmd[0] = 0x03;
		r->cmd[1] = r->lun<<5;
		r->cmd[4] = sizeof(r->sense)-1;
		r->clen = 6;
		r->data = r->sense;
		r->dlen = sizeof(r->sense)-1;
		/*
		 * Clear out the microcode state
		 * so the Dsa can be re-used.
		 */
		lesetl(d->state, A_STATE_ALLOCATED);
		goto docheck;
	}
	qunlock(&c->q[target]);
	dsafree(c, d);

	if(status == SDok && check){
		status = SDcheck;
		r->flags |= SDvalidsense;
	}
	KPRINT("sd53c8xx: %d: r flags %8.8uX status %d rlen %ld\n",
		target, r->flags, status, r->rlen);
	return r->status = status;
}

static void
cribbios(Controller *c)
{
	c->bios.scntl3 = c->n->scntl3;
	c->bios.stest2 = c->n->stest2;
	print("sd53c8xx: bios scntl3(%.2x) stest2(%.2x)\n", c->bios.scntl3, c->bios.stest2);
}

static int
bios_set_differential(Controller *c)
{
	/* Concept lifted from FreeBSD - thanks Gerard */
	/* basically, if clock conversion factors are set, then there is
 	 * evidence the bios had a go at the chip, and if so, it would
	 * have set the differential enable bit in stest2
	 */
	return (c->bios.scntl3 & 7) != 0 && (c->bios.stest2 & 0x20) != 0;
}

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
#define SYM_1010_DID	0x0020
#define SYM_1011_DID	0x0021
#define SYM_875J_DID	0x008f

static Variant variant[] = {
{ NCR_810_DID,   0x0f, "NCR53C810",	Burst16,   8, 24, 0 },
{ NCR_810_DID,   0x1f, "SYM53C810ALV",	Burst16,   8, 24, Prefetch },
{ NCR_810_DID,   0xff, "SYM53C810A",	Burst16,   8, 24, Prefetch },
{ SYM_810AP_DID, 0xff, "SYM53C810AP",	Burst16,   8, 24, Prefetch },
{ NCR_815_DID,   0xff, "NCR53C815",	Burst16,   8, 24, BurstOpCodeFetch },
{ NCR_825_DID,   0x0f, "NCR53C825",	Burst16,   8, 24, Wide|BurstOpCodeFetch|Differential },
{ NCR_825_DID,   0xff, "SYM53C825A",	Burst128, 16, 24, Prefetch|LocalRAM|BigFifo|Differential|Wide },
{ SYM_860_DID,   0x0f, "SYM53C860",	Burst16,   8, 24, Prefetch|Ultra },
{ SYM_860_DID,   0xff, "SYM53C860LV",	Burst16,   8, 24, Prefetch|Ultra },
{ SYM_875_DID,   0x01, "SYM53C875r1",	Burst128, 16, 24, Prefetch|LocalRAM|BigFifo|Differential|Wide|Ultra },
{ SYM_875_DID,   0xff, "SYM53C875",	Burst128, 16, 24, Prefetch|LocalRAM|BigFifo|Differential|Wide|Ultra|ClockDouble },
{ SYM_875J_DID,   0xff, "SYM53C875j",	Burst128, 16, 24, Prefetch|LocalRAM|BigFifo|Differential|Wide|Ultra|ClockDouble },
{ SYM_885_DID,   0xff, "SYM53C885",	Burst128, 16, 24, Prefetch|LocalRAM|BigFifo|Wide|Ultra|ClockDouble },
{ SYM_895_DID,   0xff, "SYM53C895",	Burst128, 16, 24, Prefetch|LocalRAM|BigFifo|Wide|Ultra|Ultra2 },
{ SYM_896_DID,   0xff, "SYM53C896",	Burst128, 16, 64, Prefetch|LocalRAM|BigFifo|Wide|Ultra|Ultra2 },
{ SYM_1010_DID,   0xff, "SYM53C1010",	Burst128, 16, 64, Prefetch|LocalRAM|BigFifo|Wide|Ultra|Ultra2 },
{ SYM_1011_DID,   0xff, "SYM53C1010",	Burst128, 16, 64, Prefetch|LocalRAM|BigFifo|Wide|Ultra|Ultra2 },
};

#define offsetof(s, t) ((ulong)&((s *)0)->t)

static int
xfunc(Controller *c, enum na_external x, unsigned long *v)
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
		*v = DMASEG(&c->dsalist.head[0]); return 1;
	default:
		print("xfunc: can't find external %d\n", x);
		return 0;
	}
	return 1;
}

static int
na_fixup(Controller *c, ulong pa_reg,
    struct na_patch *patch, int patches,
    int (*externval)(Controller*, int, ulong*))
{
	int p;
	int v;
	ulong *script, pa_script;
	unsigned long lw, lv;

	script = c->script;
	pa_script = c->scriptpa;
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
			if (!(*externval)(c, v, &lv))
				return 0;
			v = lv & 0xff;
			script[patch[p].lwoff] = (lw & 0xffff00ffL) | (v << 8);
			break;
		case 4:
			/* 32 bit external */
			lw = script[patch[p].lwoff];
			if (!(*externval)(c, lw, &lv))
				return 0;
			script[patch[p].lwoff] = lv;
			break;
		case 5:
			/* 24 bit external */
			lw = script[patch[p].lwoff];
			if (!(*externval)(c, lw & 0xffffff, &lv))
				return 0;
			script[patch[p].lwoff] = (lw & 0xff000000L) | (lv & 0xffffffL);
			break;
		}
	}
	return 1;
}

static SDev*
sympnp(void)
{
	int ba;
	Pcidev *p;
	Variant *v;
	void *scriptma;
	Controller *ctlr;
	SDev *sdev, *head, *tail;
	ulong regpa, *script, scriptpa;

	p = nil;
	head = tail = nil;
	while(p = pcimatch(p, NCR_VID, 0)){
		for(v = variant; v < &variant[nelem(variant)]; v++){
			if(p->did == v->did && p->rid <= v->maxrid)
				break;
		}
		if(v >= &variant[nelem(variant)])
			continue;
		print("sd53c8xx: %s rev. 0x%2.2x intr=%d command=%4.4luX\n",
			v->name, p->rid, p->intl, p->pcr);

		regpa = p->mem[1].bar;
		ba = 2;
		if(regpa & 0x04){
			if(p->mem[2].bar)
				continue;
			ba++;
		}
		regpa = upamalloc(regpa & ~0x0F, p->mem[1].size, 0);
		if(regpa == 0)
			continue;

		script = nil;
		scriptpa = 0;
		scriptma = nil;
		if((v->feature & LocalRAM) && sizeof(na_script) <= 4096){
			scriptpa = p->mem[ba].bar;
			if((scriptpa & 0x04) && p->mem[ba+1].bar){
				upafree(regpa, p->mem[1].size);
				continue;
			}
			scriptpa = upamalloc(scriptpa & ~0x0F,
					p->mem[ba].size, 0);
			if(scriptpa)
				script = KADDR(scriptpa);
		}
		if(scriptpa == 0){
			/*
			 * Either the map failed, or this chip does not have
			 * local RAM. It will need a copy of the microcode.
			 */
			scriptma = malloc(sizeof(na_script));
			if(scriptma == nil){
				upafree(regpa, p->mem[1].size);
				continue;
			}
			scriptpa = DMASEG(scriptma);
			script = scriptma;
		}

		ctlr = malloc(sizeof(Controller));
		sdev = malloc(sizeof(SDev));
		if(ctlr == nil || sdev == nil){
buggery:
			if(ctlr)
				free(ctlr);
			if(sdev)
				free(sdev);
			if(scriptma)
				free(scriptma);
			else
				upafree(scriptpa, p->mem[ba].size);
			upafree(regpa, p->mem[1].size);
			continue;
		}

		ctlr->n = KADDR(regpa);
		ctlr->v = v;
		ctlr->script = script;
		memmove(ctlr->script, na_script, sizeof(na_script));
		ctlr->scriptpa = scriptpa;
		if(!na_fixup(ctlr, regpa, na_patches, NA_PATCHES, xfunc)){
			print("script fixup failed\n");
			goto buggery;
		}
		swabl(ctlr->script, ctlr->script, sizeof(na_script));

		ctlr->dsalist.freechain = 0;
		lesetl(ctlr->dsalist.head, 0);

		ctlr->pcidev = p;

		sdev->ifc = &sd53c8xxifc;
		sdev->ctlr = ctlr;
		if(!(v->feature & Wide))
			sdev->nunit = 8;
		else
			sdev->nunit = MAXTARGET;
		ctlr->sdev = sdev;

		if(head != nil)
			tail->next = sdev;
		else
			head = sdev;
		tail = sdev;
	}

	return head;
}

static SDev*
symid(SDev* sdev)
{
	return scsiid(sdev, &sd53c8xxifc);
}

static int
symenable(SDev* sdev)
{
	Pcidev *pcidev;
	Controller *ctlr;
	//char name[NAMELEN];

	ctlr = sdev->ctlr;
	pcidev = ctlr->pcidev;

	pcisetbme(pcidev);
	//snprint(name, NAMELEN, "%s (%s)", sdev->name, sdev->ifc->name);
	intrenable(pcidev->intl, interrupt, ctlr, pcidev->tbdf, name);

	ilock(ctlr);
	synctabinit(ctlr);
	cribbios(ctlr);
	reset(ctlr);
	iunlock(ctlr);

	return 1;
}

static int
symdisable(SDev* sdev)
{
	Ncr *n;
	Controller *ctlr;

	ctlr = sdev->ctlr;
	n = ctlr->n;

	n->istat = Srst;		/* software reset */
	microdelay(1);
	n->istat = 0;

	n->scntl1 |= (1 << 3);		/* bus reset */
	delay(500);
	n->scntl1 &= ~(1 << 3);

	return 1;
}

SDifc sd53c8xxifc = {
	"53c8xx",			/* name */

	sympnp,				/* pnp */
	nil,				/* legacy */
	symid,				/* id */
	symenable,			/* enable */
	symdisable,			/* disable */

	scsiverify,			/* verify */
	scsionline,			/* online */
	symrio,				/* rio */
	nil,				/* rctl */
	nil,				/* wctl */

	scsibio,			/* bio */
};
