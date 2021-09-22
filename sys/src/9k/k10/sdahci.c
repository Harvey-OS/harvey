/*
 * advanced host controller interface (serial ata) driver
 * copyright © 2007-8 coraid, inc.
 *
 * see the (s)ata(pi) and ahci specs, and ansi t13 ata8-acs for smart.
 * the sata spec has its own terminology: `port' means `drive', each
 * of which has its own dma engines.  there's a lot of baggage.
 *
 * there is code to initialise intel's controller hub ahci controllers,
 * but this driver also works on non-intel controllers.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/sd.h"
#include <ctype.h>

#define SLOTBIT(s)		(1 << (s))
#define malign(size, align)	mallocalign(size, align, 0, 0)
#define ISAHCI(p) \
	((p)->ccrb == Pcibcstore && (p)->ccru == Pciscsata && (p)->ccrp == 1)

#define	dprint(...)	if(debug)	iprint(__VA_ARGS__); else USED(debug)
#define	idprint(...)	if(prid)	iprint(__VA_ARGS__); else USED(prid)
#define	aprint(...)	if(datapi)	iprint(__VA_ARGS__); else USED(datapi)

enum {
	Measure	= 1,			/* flag: measure transfer times */

	NCtlr	= 8,
	NCtlrdrv= 32,			/* on Intel, we usually enable 6 or 8 */
	NDrive	= NCtlr*NCtlrdrv,

	Xfrmaxsects = 128,		/* in sectors, thus 64KB */

	Nms	= 256,			/* ms. between drive checks */
	Mphywait=  2*1024/Nms - 1,
	Midwait	= 16*1024/Nms - 1,
	Mcomrwait=64*1024/Nms - 1,

	Obs	= 0xa0,			/* obsolete device bits from ata */

	/*
	 * if we get more than this many interrupts per tick (1/HZ s.)
	 * for a drive, either the hardware is broken or we've got a bug
	 * in this driver.
	 *
	 * At 6 Gb/s (sata 3 max.), 512-byte i/o generates at most
	 * ~1,573,000 interrupts/s.  8192-byte i/o generates ~98,300 max.
	 */
	Maxintrspertick = 18000000 / HZ,
	Iowaitms = 4000,	/* generous; should be done in under 1 s. */
	Iopktwaitms = 30*1000,	/* 6 sec. wasn't enough */

	Sigsata = 0,
	Sigatapi = 0xeb14,

	Didesb	= 0x2681,
};

/* ata errors: task byte 1 */
enum {
	Emed	= 1<<0,		/* media error */
	Enm	= 1<<1,		/* no media */
	Eabrt	= 1<<2,		/* aborted command */
	Emcr	= 1<<3,		/* media change request */
	Eidnf	= 1<<4,		/* no user-accessible disk address */
	Emc	= 1<<5,		/* media change */
	Eunc	= 1<<6,		/* uncorrectable data error */
	Ewp	= 1<<6,		/* write protect */
	Eicrc	= 1<<7,		/* interface crc error */

	Efatal	= Eidnf|Eicrc,	/* must sw reset */
};

/* ata status: task byte 0 */
enum {
	ASerr	= 1<<0,		/* error: task byte 1 is valid */
	ASsda	= 1<<1,		/* sense data available (obsolete) */
	ASalign	= 1<<2,		/* alignment error (obsolete) */
	ASdrq	= 1<<3,		/* data request */
	ASdwe	= 1<<4,		/* deferred write error (obsolete) */
	ASdf	= 1<<5,		/* device fault */
	ASdrdy	= 1<<6,		/* device ready */
	ASbsy	= 1<<7,		/* busy */

//	ASobs	= 1<<1|1<<2|1<<4,	/* not used */
};

/* pci configuration */
enum {
	Abar	= 5,
};

/*
 * ahci controller register layout
 *
 * 0000-002b	generic host control
 * 002c-009f	reserved, including nvmhci
 * 00a0-00ff	vendor specific
 * 0100-017f	port 0
 * 0180-01ff	port 1
 * ...
 * 1080-10ff	port 31
 */

/* ahci controller registers (per-controller) */
typedef struct {
	ulong	cap;		/* hba capabilities */
	ulong	ghc;		/* global hba control */
	ulong	isr;		/* interrupt status: per-port bits */
	ulong	pi;		/* ports implemented: per-port bits */
	ulong	ver;		/* ahci version */
	ulong	ccc;		/* cmd completion coalescing control */
	ulong	cccports;	/* ... per-port bits */
	ulong	emloc;		/* enclosure mgmt. location */
	ulong	emctl;		/* enclosure mgmt. control */
	ulong	cap2;		/* hba capabilities extended */
	ulong	bohc;		/* bios/os handoff csr */
} Ahba;

/* ahci per-drive controller registers */
typedef struct {
	ulong	list;		/* PxCLB cmd list base, must be 1kb aligned */
	ulong	listhi;
	ulong	fis;		/* fis base addr, 256-byte aligned */
	ulong	fishi;
	ulong	isr;		/* interrupt status */
	ulong	ie;		/* interrupt enable */
	ulong	cmd;		/* csr */
	ulong	_res1;
	ulong	task;	/* tfd. byte 0: ata status, byte 1: ata errors */
	ulong	sig;
	/* scr 0, 2, 1 & 3 */
	ulong	sstatus;
	ulong	sctl;
	ulong	serror;
	ulong	sactive;
	ulong	ci;		/* command issue, by slot */
	ulong	ntf;		/* scr 4: notification */
	ulong	fbs;
	ulong	devslp;
	uchar	_res2[0x70 - 0x48];
	uvlong	vendor;
} Aport;

/* cap bits: supported features */
enum {
	Hs64a	= 1<<31,	/* 64-bit addressing */
	Hsncq	= 1<<30,	/* ncq */
	Hssntf	= 1<<29,	/* snotification reg. */
	Hsmps	= 1<<28,	/* mech pres switch */
	Hsss	= 1<<27,	/* staggered spinup */
	Hsalp	= 1<<26,	/* aggressive link pm */
	Hsal	= 1<<25,	/* activity led */
	Hsclo	= 1<<24,	/* command-list override */
	Hiss	= 1<<20,	/* for interface speed */
//	Hsnzo	= 1<<19,
	Hsam	= 1<<18,	/* ahci-mode only */
	Hspm	= 1<<17,	/* port multiplier */
//	Hfbss	= 1<<16,
	Hpmb	= 1<<15,	/* multiple-block pio */
	Hssc	= 1<<14,	/* slumber state */
	Hpsc	= 1<<13,	/* partial-slumber state */
	Hncs	= MASK(5)<<8,	/* # command slots - 1 */
	Hcccs	= 1<<7,		/* cmd completion coalescing */
	Hems	= 1<<6,		/* enclosure mgmt. */
	Hsxs	= 1<<5,		/* external sata (esata) */
	Hnp	= MASK(5)<<0,	/* # ports - 1 */
};

/* ghc bits */
enum {
	Hae	= 1<<31,	/* enable ahci */
	Hie	= 1<<1,		/* " interrupts */
	Hhr	= 1<<0,		/* hba reset; cleared upon done by hw */
};

/* (drive) isr bits */
enum {
	Acpds	= 1<<31,	/* cold port detect status */
	Atfes	= 1<<30,	/* task file error status */
	Ahbfs	= 1<<29,	/* hba fatal */
	Ahbds	= 1<<28,	/* hba error (parity error) */
	Aifs	= 1<<27,	/* interface fatal  §6.1.2 */
	Ainfs	= 1<<26,	/* interface error (recovered) */
	Aofs	= 1<<24,	/* too many bytes from disk */
	Aipms	= 1<<23,	/* incorrect port multiplier status */
	Aprcs	= 1<<22,	/* PhyRdy change status Pxserr.diag.n */
	Adpms	= 1<<7,		/* mechanical presence status */
	Apcs 	= 1<<6,		/* port connect diag.x */
	Adps 	= 1<<5,		/* descriptor processed */
	Aufs 	= 1<<4,		/* unknown fis diag.f */
	Asdbs	= 1<<3,		/* set device bits fis received w/ i bit set */
	Adss	= 1<<2,		/* dma setup */
	Apio	= 1<<1,		/* pio setup fis */
	Adhrs	= 1<<0,		/* device to host register fis */

	IEM	= Acpds|Atfes|Ahbds|Ahbfs|Ahbds|Aifs|Ainfs|Aprcs|Apcs|Adps|
			Aufs|Asdbs|Adss|Adhrs,
	Ifatal	= Atfes|Ahbfs|Ahbds|Aifs,
};

/* serror bits */
enum {
	SerrX	= 1<<26,	/* exchanged */
	SerrF	= 1<<25,	/* unknown fis */
	SerrT	= 1<<24,	/* transition error */
	SerrS	= 1<<23,	/* link sequence */
	SerrH	= 1<<22,	/* handshake */
	SerrC	= 1<<21,	/* crc */
	SerrD	= 1<<20,	/* not used by ahci */
	SerrB	= 1<<19,	/* 10-tp-8 decode */
	SerrW	= 1<<18,	/* comm wake */
	SerrI	= 1<<17,	/* phy internal */
	SerrN	= 1<<16,	/* phyrdy change */

	ErrE	= 1<<11,	/* internal */
	ErrP	= 1<<10,	/* ata protocol violation */
	ErrC	= 1<<9,		/* communication */
	ErrT	= 1<<8,		/* transient */
	ErrM	= 1<<1,		/* recoverd comm */
	ErrI	= 1<<0,		/* recovered data integrety */

	ErrAll	= ErrE|ErrP|ErrC|ErrT|ErrM|ErrI,
	SerrAll	= SerrX|SerrF|SerrT|SerrS|SerrH|SerrC|SerrD|SerrB|SerrW|
			SerrI|SerrN|ErrAll,
	SerrBad	= SerrF|SerrT|SerrS|SerrH|SerrC|SerrD|SerrB,
};

/* cmd (csr) register bits */
enum {
	Aicc	= 1<<28,	/* interface communcations control. 4 bits */
	Aasp	= 1<<27,	/* aggressive slumber & partial sleep */
	Aalpe 	= 1<<26,	/* aggressive link pm enable */
	Adlae	= 1<<25,	/* drive led on atapi */
	Aatapi	= 1<<24,	/* device is atapi */
	Aesp	= 1<<21,	/* external sata (esata) port */
	Acpd	= 1<<20,	/* cold presence detect */
	Ampsp	= 1<<19,	/* mechanical pres. */
	Ahpcp	= 1<<18,	/* hot plug capable */
	Apma	= 1<<17,	/* pm attached */
	Acps	= 1<<16,	/* cold presence state */
	Acr	= 1<<15,	/* cmdlist running */
	Afr	= 1<<14,	/* fis running */
	Ampss	= 1<<13,	/* mechanical presence switch state */
	Accsshft=8,
	Accs	= MASK(5)<<Accsshft, /* current command slot 12:08 */
	Afre	= 1<<4,		/* fis enable receive */
	Aclo	= 1<<3,		/* command list override */
	Apod	= 1<<2,		/* power on dev (requires cold-pres. detect) */
	Asud	= 1<<1,		/* spin-up device;  requires ss capability */
	Ast	= 1<<0,		/* start */

	Arun	= Ast|Acr|Afre|Afr,
};

/* ctl register bits */
enum {
	Aipm	= 1<<8,		/* interface power mgmt. 3=off */
	Aspd	= 1<<4,
	Adet	= 1<<0,		/* device detection */
};

enum {
	/*
	 * Aport sstatus bits (actually states):
	 * 11-8 interface power management
	 *  7-4 current interface speed (generation #)
	 *  3-0 device detection
	 */
	Intslumber	= 0x600,
	Intpartpwr	= 0x200,
	Intactive	= 0x100,
	Intpm		= 0xf00,

	Devphyoffline	= 4,
	Devphycomm	= 2,		/* phy communication established */
	Devpresent	= 1,
	Devdet		= Devpresent | Devphycomm | Devphyoffline,
};

typedef struct Asleep Asleep;
typedef struct Ctlr Ctlr;
typedef struct Drive Drive;

/* in host's memory; not memory mapped */
typedef struct {		/* sata packets */
	uchar	*base;		/* offset 0 */
	uchar	*dma;		/* dma set up, offset 0 */
	uchar	*pio;		/* pio set up, offset 0x20 */
	uchar	*devres;	/* device-to-host, offset 0x40 */
	uchar	*u;		/* unknown; offset 0x60-0x9f, 0xa0-0xff rsvd */
	ulong	*devicebits;	/* offset 0x58-0x5f */
} Afis;

/* in host's memory; memory mapped */
typedef struct {		/* command header */
	ulong	flags;		/* and more */
	ulong	len;
	ulong	ctab;
	ulong	ctabhi;
	uchar	reserved[16];
} Alist;

enum {				/* Alist.flags bits */
	Lprdtl	= 1<<16,	/* physical region descriptor table len */
	Lpmp	= 1<<12,	/* port multiplier port */
	Lclear	= 1<<10,	/* clear busy on R_OK */
	Lbist	= 1<<9,
	Lreset	= 1<<8,
	Lpref	= 1<<7,		/* prefetchable */
	Lwrite	= 1<<6,
	Latapi	= 1<<5,
	Lcfl	= 5<<0,		/* command fis length in longs (5 bits) */
};

typedef struct {		/* physical region descriptor */
	ulong	dba;
	ulong	dbahi;
	ulong	pad;
	ulong	count;
} Aprdt;

typedef struct {		/* command */
	uchar	cfis[0x40];	/* host-to-device register fis */
	uchar	atapi[0x10];
	uchar	pad[0x30];
	Aprdt	prdt;		/* first of potentially many */
} Actab;

typedef struct {		/* a port multiplier? optional bad idea */
	QLock;			/* limits concurrent i/o ops per drive to 1 */
	Rendez;

	uchar	flag;
	uchar	feat;
	uchar	smart;
	Afis	fis;
	Alist	*list;
	Actab	*ctab;
} Aportm;

enum {				/* Aportm.flag bits */
	Ferror	= 1,
	Fdone	= 2,
};

enum {				/* Aportm.feat feature bits */
	Dllba 	= 1<<0,
	Dsmart	= 1<<1,
	Dpower	= 1<<2,
	Dnop	= 1<<3,
	Datapi	= 1<<4,
	Datapi16= 1<<5,
};

enum {
	Tunk,
	Tsb600,
	Tesb,
	Tich,			/* intel.  includes mch, pch, etc. */
	Tich100,		/* newer, bluer, more incompatible! */
};

static char *tname[] = {
	"unknown",
	"sb600",
	"63xxesb",
	"ich classic",
	"ich series 100+",
};

enum {
	Dnull,
	Dmissing,
	Dnew,
	Dready,
	Derror,
	Dreset,
	Doffline,
	Dportreset,
	Dlast,
};

static char *diskstates[Dlast] = {
	"null",
	"missing",
	"new",
	"ready",
	"error",
	"reset",
	"offline",
	"portreset",
};

enum {
	DMautoneg,
	DMsatai,
	DMsataii,
	DMsata3,
};

static char *oldmodename[] = {		/* used only in old control messages */
	"auto",
	"satai",
	"sataii",
	"sata3",
};
static char *modename[] = {
	"auto",
	"sata1",
	"sata2",
	"sata3",
};

static char *flagname[] = {		/* of Aportm.feat bits */
	"llba",
	"smart",
	"power",
	"nop",
	"atapi",
	"atapi16",
	"bit 6",
	"bit 7",
};

typedef struct Stats Stats;
struct Stats {
	uvlong	lowwat;			/* in ns. */
	uvlong	hiwat;
	uvlong	tot;

	uvlong	count;
	uvlong	slow;
	uvlong	outliers;
};

struct Drive {
	Lock;			/* protects access to Drive, unit, port */

	Ctlr	*ctlr;		/* backward pointer */
	SDunit	*unit;
	Aport	*port;		/* ahci drive registers */
	Aportm;			/* port multiplier with QLock, Rendez */

	uchar	mediachange;
	uchar	state;
	uchar	smartrs;

	uvlong	sectors;
	ulong	secsize;
	ulong	intick;		/* start tick of current transfer */
	ulong	lastseen;
	int	wait;
	uchar	mode;		/* DM^(autoneg satai sataii sata3) */
	uchar	active;		/* op in progress; triggers wakeup upon intr */

	int	infosz;
	ushort	*info;

	int	driveno;	/* ctlr*NCtlrdrv + unit */
	/* controller port # != driveno when not all ports are enabled */
	int	portno;

	ulong	lastintr0;	/* time intrs was last zeroed */
	ulong	intrs;		/* interrupts this tick for this drive */
	ulong	lastcause;

	ulong	doneticks;
	uvlong	donens;
	Stats;

	char	serial[20+1];
	char	firmware[8+1];
	char	model[40+1];
};

struct Ctlr {
	Lock;			/* protects intrs & en/disabling of ctlr's */
	Intrcommon;

	SDev	*sdev;

	/* register virtual addresses */
	uchar	*mmio;
	Ahba	*hba;

	uchar	*physio;	/* physical register address */

	int	type;
	int	enabled;	/* flag: fully enabled? */
	int	intrson;	/* flag: interrupts enabled? */
	int	widecfg;	/* flag: new-style intel pci cfg; see Pmap, Ppcs */

	Drive	*rawdrive;
	Drive	*drive[NCtlrdrv];
	int	ndrive;
	int	mport;		/* highest drive # (0-origin) on ich9 at least */

	ulong	lastintr0;	/* time intrs was last zeroed */
	ulong	intrs;		/* this tick, not attributable to any drive */
	ulong	allintrs;
};

// fold this into Drive? separate watch bits permit multiple concurrent
// ops, but complicate error reporting.
struct Asleep {			/* we rendezvous using this to signal completion */
	Aport	*port;
	ulong	watchbits;	/* bitmap of cmd slots we're watching */
	Drive	*drive;
};

extern SDifc sdahciifc;

static	Ctlr	iactlr[NCtlr];
static	SDev	sdevs[NCtlr];
static	int	niactlr;

static	Drive	*iadrive[NDrive];
static	int	niadrive;

/* these are fiddled in iawtopctl() */
static	int	debug;
static	int	prid = 1;
static	int	datapi;

static char stab[] = {
[0]	'i', 'm',
[8]	't', 'c', 'p', 'e',
[16]	'N', 'I', 'W', 'B', 'D', 'C', 'H', 'S', 'T', 'F', 'X'
};

static int	readsmart(Drive *d);
static void	runsmartable(Drive *, int);

/*
 * use nanoseconds for better resolution than ticks give us,
 * to assist transfer time measurements.
 */
static void
tnssleep(Rendez *r, int (*fn)(void *), void *arg, long maxms)
{
	int msleft;
	uvlong nowns, endns;

	if (fn == nil)
		return;
	endns = todget(nil) + maxms * 1000000LL;
	while (!fn(arg)) {
		nowns = todget(nil);
		msleft = (endns - nowns) / 1000000;
		if (nowns > endns || msleft <= 0)
			break;
		while(waserror())
			;
		/*
		 * check the done condition frequently so that a masked
		 * interrupt doesn't cause a long wait.
		 */
		tsleep(r, fn, arg, msleft > 10? 10: msleft);
		/* NB: we may be running on a different cpu now */
		poperror();
	}
}

static void
tmssleep(Rendez *r, int (*fn)(void *), void *arg, long maxms)
{
	int msleft;
	uvlong endms;

	if (fn == nil)
		return;
	endms = TK2MS((uvlong)sys->ticks) + maxms;
	while (!fn(arg)) {
		msleft = endms - TK2MS((uvlong)sys->ticks);
		if (msleft <= 0)
			break;
		while(waserror())
			;
		/*
		 * check the done condition frequently so that a masked
		 * interrupt doesn't cause a long wait.
		 */
		tsleep(r, fn, arg, msleft > 10? 10: msleft);
		/* NB: we may be running on a different cpu now */
		poperror();
	}
}

static void
esleep(int ms)
{
	if(waserror())
		return;
	tsleep(&up->sleep, return0, 0, ms);
	poperror();
}

#define tsleep (Measure? tnssleep: tmssleep)

static void
asleep(int ms)
{
	if(up == nil)
		delay(ms);
	else
		esleep(ms);
}

static void
serrstr(ulong r, char *s, char *e)
{
	int i;

	e -= 3;
	for(i = 0; i < nelem(stab) && s < e; i++)
		if(r & (1<<i) && stab[i]){
			*s++ = stab[i];
			if(SerrBad & (1<<i))
				*s++ = '*';
		}
	*s = 0;
}

static void
preg(uchar *reg, int n)
{
	int i;
	char buf[25*3+1], *e;

	e = buf;
	for(i = 0; i < n; i++)
		e = seprint(e, buf + sizeof buf, "%02x%c", reg[i],
			i < n-1? ' ': '\n');
	dprint(buf);
}

static char *taskbits[16] = {
	/* ata status */
	"error",
	"sense data available",
	"alignment error",
	"data request",
	"deferred write error",
	"device fault",
	"device ready",
	"busy",

	/* ata errors */
	"media error",
	"no media",
	"aborted command",
	"media change request",
	"no user-accessible disk address",
	"media change",
	"uncorrectable data error or write protect",
	"interface crc error",
};

static void
prtask(int task)
{
	int i, first;

	first = 1;
	for (i = 0; i < 16; i++)
		if (task & (1<<i)) {
			print("%s%s", (!first? ", ": ""), taskbits[i]);
			first = 0;
		}
}

static void
dumpreg(char *s, Aport *port)
{
	print("ahci: %stask=%#lux; cmd=%#lux; ci=%#lux; is=%#lux\n",
		s, port->task, port->cmd, port->ci, port->isr);
}

static void
dreg(char *s, Aport *port)
{
	if (debug)
		dumpreg(s, port);
}

static void
setdonetimes(Drive *drive)
{
	if (drive->doneticks == 0)
		drive->doneticks = sys->ticks;
	if (Measure && drive->donens == 0)
		drive->donens = todget(nil);
}

static Aport *
assetup(Drive *drive, Asleep *as)
{
	as->drive = drive;
	as->watchbits = SLOTBIT(0);		/* only 1 slot guaranteed */
	return as->port = drive->port;
}

static void
ckdriveidle(Drive *drive, Asleep *as, char *who, uintptr caller)
{
	if (drive->port->ci & as->watchbits)
		iprint("%s: slot 0 still in use\n", who);
	if (drive->active)
		panic("%s: called with drive active, from %#p", who, caller);
}

/*
 * each sata drive could in principle have a queue of outstanding requests,
 * but we only seem to allow one outstanding request per drive currently,
 * which does simplify error handling.
 */
static void
doneio(Drive *drive)
{
	ilock(drive);
	drive->active--;
	drive->intick = 0;
	iunlock(drive);
}

/* returns true if ahci is quiet (all watched commands have completed) */
static int
ahciopdone(void *v)
{
	Asleep *as;
	Drive *drive;

	as = v;
	drive = as->drive;
	if (drive == nil)
		return (as->port->ci & as->watchbits) == 0;
	if (!drive->doneticks) {
		dprint("ahciopdone ci %#lux watchbits %#lux...",
			as->port->ci, as->watchbits);
		if ((as->port->ci & as->watchbits) == 0)
			setdonetimes(drive);
	}
	return drive->doneticks;
}

static void
awaitiodone(Asleep *as, int ms)
{
	while(waserror())
		;
	tsleep(as->drive, ahciopdone, as, ms);
	poperror();
}

/*
 * issue the command (drive->list) in this drive's slot 0 & wait for
 * normal completion, error, or time-out.
 * must be called with qlock(drive) held.
 *
 * seems to be used for small tweaks, such as smart, with issueio
 * handling bulk i/o.
 */
static int
ahciexec(Drive *drive, int ms)
{
	int rv, err;
	long ticks;
	ulong sttick;
	Aport *port;
	Asleep *as;

	if (canqlock(drive))
		panic("ahciexec: called without qlock(drive) held, from %#p",
			getcallerpc(&drive));
	rv = err = 0;
	as = smalloc(sizeof *as);
	port = assetup(drive, as);
	ilock(drive);
	ckdriveidle(drive, as, "ahciexec", getcallerpc(&drive));
	drive->active++;		/* ahciintr should generate a wakeup */
	port->ci = as->watchbits;	/* issue cmd in drive's slot 0 */
	iunlock(drive);

	while (port->ci & as->watchbits && !err && ms > 0) {
		/* wait until ahci is quiet, or we time-out */
		sttick = sys->ticks;
		awaitiodone(as, ms);
		ticks = sys->ticks - sttick;
		if (ticks > 0)
			ms -= TK2MS(ticks);
		err = port->task & ASerr;
		/*
		 * If not done yet and no error yet either, I don't know why
		 * this happens.  Maybe we compete with actual i/o from
		 * issueio, also using slot 0.
		 */
	}
	if (err || ms <= 0) {
		dumpreg(err? "ahciexec error ": "ahciexec timed out ",
			drive->port);
		port->ci &= ~as->watchbits;
		rv = -1;
	}
	free(as);
	doneio(drive);
	return rv;
}

/* fill in cfis boilerplate */
static uchar *
cfissetup(Drive *drive)
{
	uchar *cfis;

	cfis = drive->ctab->cfis;
	memset(cfis, 0, 0x20);
	cfis[0] = 0x27;
	cfis[1] = 0x80;
	cfis[7] = Obs;
	return cfis;
}

/* initialise pc's list */
static void
listsetup(Drive *drive, int flags)
{
	uvlong pctab;
	Alist *list;

	list = drive->list;
	list->flags = flags | 5;
	list->len = 0;
	pctab = PCIWADDR(drive->ctab);
	list->ctab   = pctab;
	list->ctabhi = pctab>>32;
}

static int
ahcinop(Drive *drive)
{
	uchar *cfis;

	if((drive->feat & Dnop) == 0)
		return -1;
	cfis = cfissetup(drive);
	cfis[2] = 0;
	listsetup(drive, Lwrite);
	return ahciexec(drive, 3*1000);
}

static int
setfeatures(Drive *drive, uchar f)
{
	uchar *cfis;

	cfis = cfissetup(drive);
	cfis[2] = 0xef;
	cfis[3] = f;
	listsetup(drive, Lwrite);
	return ahciexec(drive, 3*1000);
}

static int
setudmamode(Drive *drive, uchar f)
{
	uchar *cfis;

	/* hack */
	if((drive->port->sig >> 16) == Sigatapi)
		return 0;
	cfis = cfissetup(drive);
	cfis[2] = 0xef;
	cfis[3] = 3;		/* set transfer mode */
	cfis[12] = 0x40 | f;	/* sector count */
	listsetup(drive, Lwrite);
	return ahciexec(drive, 3*1000);
}

static int
ahciportreset(Drive *drive)
{
	ulong *cmd, i;
	Aport *port;

	port = drive->port;
	cmd = &port->cmd;
	*cmd &= ~(Afre|Ast);
	coherence();
	for(i = 0; i < 500 && *cmd & Acr; i += 25)
		asleep(25);
	port->sctl = 1 | (port->sctl & ~7);
	coherence();
	delay(1);
	port->sctl &= ~7;
	return 0;
}

/* n == 0 enables smart; n == 1 disables it. */
static int
smart(Drive *drive, int n)
{
	uchar *cfis;

	if((drive->feat&Dsmart) == 0)
		return 0;		/* optical drive */
	cfis = cfissetup(drive);
	cfis[2] = 0xb0;
	cfis[3] = 0xd8 + n;		/* (en|dis)able smart */
	cfis[5] = 0x4f;
	cfis[6] = 0xc2;
	listsetup(drive, Lwrite);
	if(ahciexec(drive, 1000) == -1 || drive->port->task & (ASerr|ASdf)){
		dprint("ahci: smart fail %#lux\n", drive->port->task);
//		preg(drive->fis.devres, 20);
		return -1;
	}
	if(n)
		return 0;
	return 1;
}

static int
smartrs(Drive *drive)
{
	uchar *cfis;

	if((drive->feat&Dsmart) == 0)
		return 0;		/* optical drive */
	cfis = cfissetup(drive);
	cfis[2] = 0xb0;
	cfis[3] = 0xda;			/* return smart status */
	cfis[5] = 0x4f;
	cfis[6] = 0xc2;
	listsetup(drive, Lwrite);

	cfis = drive->fis.devres;
	if(ahciexec(drive, 1000) == -1 || drive->port->task & (ASerr|ASdf)){
		dprint("ahci: smart fail %#lux\n", drive->port->task);
		preg(cfis, 20);
		return -1;
	}
	if(cfis[5] == 0x4f && cfis[6] == 0xc2)
		return 1;
	return 0;
}

static int
ahciflushcache(Drive *drive)
{
	uchar *c;

	c = cfissetup(drive);
	c[2] = drive->feat & Dllba? 0xea: 0xe7;	/* ATA vs ATAPI */
	listsetup(drive, Lwrite);
	if(ahciexec(drive, 60000) == -1 || drive->port->task & (ASerr|ASdf)){
		dprint("ahciflushcache: fail %#lux\n", drive->port->task);
//		preg(drive->fis.devres, 20);
		return -1;
	}
	return 0;
}

static ushort
gbit16(void *a)
{
	uchar *i;

	i = a;
	return i[1]<<8 | i[0];
}

static ulong
gbit32(void *a)
{
	uchar *i;

	i = a;
	return i[3] << 24 | i[2] << 16 | i[1] << 8 | i[0];
}

static uvlong
gbit64(void *a)
{
	uchar *i;

	i = a;
	return (uvlong)gbit32(i+4) << 32 | gbit32(a);
}

/* issue identify command */
static int
ahciidentify0(Drive *drive, void *id, int atapi)
{
	uvlong pdba;
	uchar *cfis;
	Aprdt *prdt;
	static uchar tab[] = { 0xec, 0xa1, };	/* magic: ata vs atapi */

	cfis = cfissetup(drive);
	cfis[2] = tab[atapi];
	listsetup(drive, Lprdtl);

	memset(id, 0, 0x100);			/* magic */
	prdt = &drive->ctab->prdt;
	pdba = PCIWADDR(id);
	prdt->dba = pdba;
	prdt->dbahi = pdba>>32;
	prdt->count = 1<<31 | (0x200-2) | 1;
	return ahciexec(drive, 3*1000);
}

/*
 * map signature & results of identify command to Aportm.feat.
 * return device size in sectors.
 */
static vlong
ahciidentify(Drive *drive, ushort *id)
{
	int i, sig;
	vlong s;

	drive->feat = drive->smart = 0;
	i = 0;
	sig = drive->port->sig >> 16;
	if(sig == Sigatapi){
		drive->feat |= Datapi;
		i = 1;
	}
	if(ahciidentify0(drive, id, i) == -1)
		return -1;

	i = gbit16(id+83) | gbit16(id+86);
	if(i & (1<<10)){
		drive->feat |= Dllba;
		s = gbit64(id+100);
	}else
		s = gbit32(id+60);

	if(drive->feat&Datapi){
		i = gbit16(id+0);
		if(i&1)
			drive->feat |= Datapi16;
	}

	i = gbit16(id+83);
	if((i>>14) == 1) {
		if(i & (1<<3))
			drive->feat |= Dpower;
		i = gbit16(id+82);
		if(i & 1)
			drive->feat |= Dsmart;
		if(i & (1<<14))
			drive->feat |= Dnop;
	}
	return s;
}

static int
ahciidle(Aport *port)
{
	ulong i, r;
	ulong *p;

	if (port == nil)
		return 0;
	p = &port->cmd;
	if((*p & Arun) == 0)		/* not running? */
		return 0;

	*p &= ~Ast;
	coherence();
	for (i = 500; i > 0 && *p & Acr; i -= 25)
		asleep(25);
	r = 0;
	if(*p & Acr)			/* still running? */
		r = -1;

	if((*p & Afre) == 0)
		return r;
	*p &= ~Afre;
	coherence();
	for(i = 0; i < 500; i += 25){
		if((*p & Afre) == 0)
			return 0;
		asleep(25);
	}
	return -1;
}

/*
 * § 6.2.2.1  first part; comreset handled by resetdisk.
 *	- remainder is handled by ahciconfigdrive.
 *	- ahcirecover is a quick recovery from a failed command.
 */
static int
ahciswreset(Drive *drive)
{
	int i;

	i = ahciidle(drive->port);
	drive->port->cmd |= Afre;
	coherence();
	if(i == -1)
		return -1;
	if(drive->port->task & (ASdrq|ASbsy))
		return -1;
	return 0;
}

static int
ahcirecover(Drive *drive)
{
	if (ahciswreset(drive) < 0)
		print("swreset failed...");
	drive->port->cmd |= Ast;
	coherence();
	if(setudmamode(drive, 5) == -1)
		return -1;
	return 0;
}

static void
setupfis(Afis *f)
{
	f->base = malign(0x100, 0x100);		/* magic */
	f->dma = f->base + 0;
	f->pio = f->base + 0x20;
	f->devres = f->base + 0x40;
	f->u = f->base + 0x60;
	f->devicebits = (ulong*)(f->base + 0x58);
}

static void
ahciwakeup(Aport *port)
{
	ushort s;

	s = port->sstatus;
	if((s & Intpm) != Intslumber && (s & Intpm) != Intpartpwr)
		return;
	if((s & Devdet) != Devpresent){	/* not (device, no phy) */
		iprint("ahci: slumbering drive unwakable %#ux\n", s);
		return;
	}
	port->sctl = 3*Aipm | 0*Aspd | Adet;
	coherence();
	delay(1);
	port->sctl &= ~7;
//	iprint("ahci: wake %#ux -> %#ux\n", s, port->sstatus);
}

static int
ahciconfigdrive(Drive *drive)
{
	uvlong phys;
	char *name;
	Aport *port;

	if (drive == nil || drive->ctlr == nil) {
		print("ahciconfigdrive: nil drive or drive->ctlr!\n");
		return 0;
	}
	if(drive->list == 0){
		setupfis(&drive->fis);
		drive->list = malign(sizeof *drive->list, 1024);
		drive->ctab = malign(sizeof *drive->ctab, 128);
	}

	if (drive->unit)
		name = drive->unit->name;
	else
		name = "sd??";
	port = drive->port;
	if(port->sstatus & (Devphycomm|Devpresent) &&
	    drive->ctlr->hba->cap & Hsss){
		/* device connected & staggered spin-up */
		dprint("ahci: configdrive: %s: spinning up ... [%#lux]\n",
			name, port->sstatus);
		port->cmd |= Apod|Asud;
		coherence();
		asleep(1400);
	}

	port->serror = SerrAll;		/* clear outstanding errors */

	phys = PCIWADDR(drive->list);
	port->list = phys;
	port->listhi = phys>>32;
	phys = PCIWADDR(drive->fis.base);
	port->fis = phys;
	port->fishi = phys>>32;
	coherence();
	port->cmd |= Afre|Ast;
	coherence();

	/* drive coming up in slumbering? */
	if((port->sstatus & Devdet) == Devpresent &&
	   ((port->sstatus & Intpm) == Intslumber ||
	    (port->sstatus & Intpm) == Intpartpwr))
		ahciwakeup(port);

	/* "disable power managment" sequence from book. */
	port->sctl = (3*Aipm) | (drive->mode*Aspd) | (0*Adet);
	port->cmd &= ~(Aalpe | Accs);
	port->cmd |= 0<<Accsshft;	/* drive cmd slot 0 */
	coherence();

	port->ie = IEM;
	coherence();
	return 0;
}

static void
ahcienable(Ahba *hba)
{
	/* intel at least seems to always want Hae set */
	hba->ghc |= Hae;	/* talk ahci & sata */
	coherence();
	hba->ghc |= Hie;
	coherence();
}

static void
ahcidisable(Ahba *h)
{
	h->ghc &= ~Hie;
	coherence();
}

/* returns number of active ports */
static int
ahciconf(Ctlr *ctlr)
{
	Ahba *hba;
	int pi, nports;
	ulong cap;

	ctlr->hba = hba = (Ahba*)ctlr->mmio;
	ctlr->hba->isr = ~0;		/* squelch leftover interrupts */
	cap = hba->cap;
	/* intel at least seems to always want Hae set */
	hba->ghc |= Hae;

	/* nports can be > number of bits set in hba->pi */
	nports = (cap & MASK(5)) + 1;
	pi = countbits(hba->pi);
	if (pi > nports)
		pi = nports;		/* sanity */
	dprint("#S/sd%C: type %s regs %#p: staggered spin-up %ld, %ld command slots,\n"
		" coalesce %ld, %d ports (%d in use), cmd-list override %ld,"
		" enclosure mgmt %ld\n",
		ctlr->sdev->idno, tname[ctlr->type], ctlr->physio,
		(cap>>27) & 1, (cap>>8) & 0x1f,
		(cap>>7) & 1, nports, pi, (cap>>24) & 1, (cap>>6) & 1);
	return pi;
}

static int
ahcihbareset(Ahba *h)
{
	int wait;

	dprint("ahci: reset...");
	h->ghc |= Hhr;
	coherence();
	for(wait = 0; wait < 3000; wait += 50){
		if((h->ghc & Hhr) == 0)
			return 0;
		delay(50);
	}
	print("ahci: hba reset didn't finish\n");
	return -1;
}

/* there are n+1 bytes in dest; copy them byte-swapped from src. */
static void
idmove(char *dest, ushort *src, int n)
{
	int i;
	char *p, *e;

	/* copy with bytes swapped */
	p = dest;
	for(i = 0; i < n/2; i++){
		*p++ = src[i] >> 8;
		*p++ = src[i];
	}
	*p = '\0';

	/* strip trailing spaces, leave e pointing at the trailing NUL */
	for (e = p; e > dest && e[-1] == ' '; )
		*--e = '\0';

	/* strip leading spaces */
	for (p = dest; *p == ' '; p++)
		;

	/* copy stripped string into place */
	memmove(dest, p, e + 1 - p);

	/* down-case */
	for (p = dest; *p != '\0'; p++)
		*p = tolower(*p);
}

static int
identify(Drive *drive)
{
	ushort *id;
	vlong osects, sects;
	uchar oserial[sizeof drive->serial];
	SDunit *unit;

	if(drive->info == nil) {
		drive->infosz = 512 * sizeof(ushort);
		drive->info = smalloc(drive->infosz);
	}
	id = drive->info;
	sects = ahciidentify(drive, id);
	if(sects == -1){
		drive->state = Derror;
		return -1;
	}
	osects = drive->sectors;
	memmove(oserial, drive->serial, sizeof drive->serial);

	unit = drive->unit;
	drive->sectors = sects;
	drive->secsize = unit->secsize;
	if(drive->secsize == 0)
		drive->secsize = 512;		/* default */
	drive->smartrs = 0;

	idmove(drive->serial, id+10, sizeof drive->serial - 1);
	idmove(drive->firmware, id+23, sizeof drive->firmware - 1);
	idmove(drive->model, id+27, sizeof drive->model - 1);

	memset(unit->inquiry, 0, sizeof unit->inquiry);
	unit->inquiry[2] = 2;
	unit->inquiry[3] = 2;
	unit->inquiry[4] = sizeof unit->inquiry - 4;
	memmove(unit->inquiry+8, drive->model, sizeof drive->model - 1);

	if(osects != sects || memcmp(oserial, drive->serial, sizeof oserial) != 0){
		drive->mediachange = 1;
		unit->sectors = 0;
	}
	return 0;
}

static void
clearci(Aport *port)
{
	if(port->cmd & Ast) {
		port->cmd &= ~Ast;
		coherence();
		microdelay(10);
		port->cmd |=  Ast;
	}
}

/*
 * call with drive locked.
 * side-effect: clears all of port's interrupt sources (port->isr).
 */
static void
updatedrive(Drive *drive)
{
	ulong cause, serr, s0, pr, ewake;
	char *name;
	Aport *port;

	port = drive->port;
	cause = port->isr;
	port->isr = cause;		/* clear drive's interrupt sources */
	name = "sd??";
	if(drive->unit && drive->unit->name)
		name = drive->unit->name;

	pr = 0;
	if((port->ci & SLOTBIT(0)) == 0 && drive->active) {
		drive->flag |= Fdone;	/* i/o was in progress on slot */
		setdonetimes(drive);
		wakeup(drive);			/* report done to any sleeper */
	} else if(!(cause & Adps))		/* descriptor not processed? */
		pr = 1;

	ewake = 0;
	if(cause & Ifatal){
		ewake = 1;
		dprint("%s: updatedrive: fatal task error %#lux\n", name, cause);
	}
	serr = port->serror;
	if(cause & Adhrs){		/* device has status for us to read */
		if(port->task & (ASerr|ASdf)){
			dprint("ahci: %s: Adhrs cause %#lux serr %#lux "
				"task %#lux\n", name, cause, serr, port->task);
			drive->flag |= Ferror;
			ewake = 1;
		}
		pr = 0;
	}
	if(port->task & ASerr && drive->lastcause != cause)
		dprint("%s: err cause %#lux serr %#lux task %#lux sstat %#lux\n",
			name, cause, serr, port->task, port->sstatus);
	if(pr)
		dprint("%s: upd cause %#lux task %#lux\n", name, cause, port->task);

	if(cause & (Aprcs|Aifs)){		/* change in status */
		s0 = drive->state;
		switch(port->sstatus & Devdet){
		case 0:				/* no device */
			drive->state = Dmissing;
			break;
		case Devpresent:		/* device but no phy comm. */
			if((port->sstatus & Intpm) == Intslumber ||
			   (port->sstatus & Intpm) == Intpartpwr)
				drive->state = Dnew;	/* slumbering */
			else
				drive->state = Derror;
			break;
		case Devpresent|Devphycomm:
			/* power mgnt crap for surprise removal (or insertion?) */
			port->ie |= Aprcs|Apcs;	/* is this required? */
			coherence();
			drive->state = Dreset;
			break;
		case Devphyoffline:
			drive->state = Doffline;
			break;
		}
		dprint("%s: %s -> %s [Apcrs] %#lux\n", name,
			diskstates[s0], diskstates[drive->state], port->sstatus);
		if(s0 == Dready && drive->state != Dready)
			idprint("%s: drive removed\n", name);
		if(drive->state != Dready)
			drive->flag |= Ferror;
		ewake = 1;
	}
	port->serror = serr;		/* clear the errors we saw */
	if(ewake){			/* error or status change to report */
		clearci(port);
		wakeup(drive);
	}
	drive->lastcause = cause;
}

static void
pstatus(Drive *drive, ulong s)
{
	/*
	 * s is sstatus masked with Devdet.
	 *
	 * bogus code because the first interrupt is currently dropped.
	 * likely my fault.  serror may be cleared at the wrong time.
	 */
	switch(s){
	case 0:			/* no device */
		drive->state = Dmissing;
		break;
	case Devpresent:	/* device but no phy. comm. */
		break;
	case Devphycomm:	/* should this be missing?  need testcase. */
		dprint("ahci: pstatus 2\n");
		/* fallthrough */
	case Devpresent|Devphycomm:
		drive->wait = 0;
		drive->state = Dnew;
		break;
	case Devphyoffline:
		drive->state = Doffline;
		break;
	case Devphyoffline|Devphycomm:	/* does this make sense? */
		drive->state = Dnew;
		break;
	}
}

static int
configdrive(Drive *drive)
{
	if(ahciconfigdrive(drive) == -1)
		return -1;
	ilock(drive);
	pstatus(drive, drive->port->sstatus & Devdet);
	iunlock(drive);
	return 0;
}

static void
setstate(Drive *drive, int state)
{
	ilock(drive);
	drive->state = state;
	iunlock(drive);
}

static int
resetdisk(Drive *drive)
{
	uint state, det, stat;
	Aport *port;

	port = drive->port;
	det = port->sctl & 7;
	stat = port->sstatus & Devdet;
	state = (port->cmd>>28) & MASK(4);
	dprint("ahci: resetdisk: icc %#ux  det %d sdet %d\n", state, det, stat);

	ilock(drive);
	state = drive->state;
	if(state != Dready || state != Dnew)
		drive->flag |= Ferror;
	clearci(port);			/* satisfy sleep condition. */
	wakeup(drive);			/* bad news for any sleeper */
	if(stat != (Devpresent|Devphycomm)){
		/* device absent or phy not communicating */
		drive->state = Dportreset;
		iunlock(drive);
		return -1;
	}
	drive->state = Derror;
	iunlock(drive);

	qlock(drive);
	if(port->cmd&Ast && ahciswreset(drive) == -1)
		setstate(drive, Dportreset);	/* get a bigger stick. */
	else {
		setstate(drive, Dmissing);
		configdrive(drive);
	}
	dprint("ahci: %s: resetdisk: %s -> %s\n",
		(drive->unit? drive->unit->name: ""),
		diskstates[state], diskstates[drive->state]);
	qunlock(drive);
	return 0;
}

static int
newdrive(Drive *drive)
{
	char *name;

	name = drive->unit->name;
	if(name == nil)
		name = "??";

	if(drive->port->task == ASbsy)
		return -1;
	qlock(drive);
	if(setudmamode(drive, 5) == -1){
		idprint("%s: can't set udma mode\n", name);
//		goto lose;	// see if intel ssd works without udmamode set
	}
	if(identify(drive) == -1){
		idprint("%s: identify failure\n", name);
		goto lose;
	}
	if(drive->feat & Dpower && setfeatures(drive, 0x85) == -1){
		drive->feat &= ~Dpower;
		coherence();
		if(ahcirecover(drive) == -1) {
			idprint("%s: ahcirecover failed\n", name);
			goto lose;
		}
	}
	setstate(drive, Dready);
	qunlock(drive);

	if (prid)
		print("%s: %slba %,llud sectors: %s fw %s serial %s%s\n",
			drive->unit->name, (drive->feat & Dllba? "l": ""),
			drive->sectors, drive->model, drive->firmware,
			drive->serial, drive->mediachange? " [mediachange]": "");
	drive->Stats.lowwat = 1000000;
	return 0;

lose:
	idprint("%s: can't initialize\n", drive->unit->name);
	setstate(drive, Dnull);
	qunlock(drive);
	return -1;
}

static void
westerndigitalhung(Drive *drive)
{
	if((drive->feat&Datapi) == 0 && drive->active && drive->intick &&
	    TK2MS(sys->ticks - drive->intick) > 5000){
		dprint("%s: drive hung; resetting [%#lux] ci %#lx\n",
			drive->unit->name, drive->port->task, drive->port->ci);
		drive->state = Dreset;
		drive->intick = 0;
	}
}

static int
doportreset(Drive *drive)
{
	int i;

	qlock(drive);
	i = ahciportreset(drive);
	if(i == -1)
		dprint("ahci: doportreset: failed\n");
	qunlock(drive);
	dprint("ahci: doportreset: portreset -> %s  [task %#lux]\n",
		diskstates[drive->state], drive->port->task);
	return i;
}

/* drive must be locked */
static void
statechange(Drive *drive)
{
	switch(drive->state){
	case Dnull:
	case Doffline:
		if(drive->unit->sectors != 0){
			drive->sectors = 0;
			drive->mediachange = 1;
		}
		/* fallthrough */
	case Dready:
		drive->wait = 0;
		break;
	}
}

/* call with drive locked, will be locked again upon return */
static int
driveopunlocked(Drive *drive, int (*op)(Drive *))
{
	int r;

	iunlock(drive);
	r = op(drive);
	ilock(drive);
	return r;
}

static void
portresetstate(Drive *drive, int s)
{
	if(drive->wait++ & 0xff && !(s & Intactive))
		return;

	/* device is active */
	dprint("%s: portreset [%s]: mode %d; status %06#ux\n",
		drive->unit->name, diskstates[drive->state], drive->mode, s);
	drive->flag |= Ferror;
	clearci(drive->port);
	wakeup(drive);			/* bad news for any sleeper */
	if(s & Devdet)			/* device present?*/
		driveopunlocked(drive, doportreset);
	else
		drive->state = Dmissing;
}

static void
resetstate(Drive *drive, int s)
{
	/* advance to next sata speed */
	if(++drive->mode > DMsata3)
		drive->mode = 0;	/* DMautoneg */
	if(drive->mode == DMsatai){
		/*
		 * we started at DMsatai, so we have wrapped
		 * around, so we've tried all sata modes once.
		 */
		drive->state = Dportreset;
		portresetstate(drive, s);
	} else {
		dprint("%s: reset; new mode %s\n", drive->unit->name,
			modename[drive->mode]);
		driveopunlocked(drive, resetdisk);
	}
}

static ushort olds[NDrive];

static void
checkdrive(Drive *drive, int i)
{
	ushort s;
	char *name;

	if (drive == nil)
		return;
	ilock(drive);
	if(drive->unit == nil || drive->port == nil) {
		iunlock(drive);
		return;
	}
	name = drive->unit->name;
	s = drive->port->sstatus;
	if(s)
		drive->lastseen = sys->ticks;
	if(s != olds[i]){
		dprint("%s: status: %06#ux -> %06#ux: %s\n",
			name, olds[i], s, diskstates[drive->state]);
		olds[i] = s;
		drive->wait = 0;
	}
	westerndigitalhung(drive);

	switch(drive->state){
	case Dnull:
	case Dready:
		break;
	case Doffline:
		if(drive->wait++ & Mcomrwait)
			break;
		/* fallthrough */
	case Derror:
	case Dreset:
		dprint("%s: reset [%s]: mode %d; status %06#ux\n",
			name, diskstates[drive->state], drive->mode, s);
		driveopunlocked(drive, resetdisk);
		break;
	case Dportreset:
		portresetstate(drive, s);
		break;
	case Dmissing:
	case Dnew:
		/* the inner state machine, for the non-trivial cases */
		switch(s & (Intactive | Devdet)){
		case Devpresent:  /* no device (pm), device but no phy. comm. */
			ahciwakeup(drive->port);
			/* fall through */
		case 0:			/* no device */
			break;
		default:
			dprint("%s: unknown status %06#ux\n", name, s);
			/* fall through */
		case Intactive:		/* active, no device */
			if((++drive->wait & Mphywait) == 0)
				resetstate(drive, s);
			break;
		case Intactive|Devphycomm|Devpresent:
			if((++drive->wait & Midwait) == 0){
				dprint("%s: slow reset %06#ux task=%#lux; %d\n",
					name, s, drive->port->task, drive->wait);
				resetstate(drive, s);
			} else {
				/* get only ata status, not errs */
				s = drive->port->task & 0xff;
				if(s != 0x7f &&
				    ((drive->port->sig >> 16) == Sigatapi ||
				     (s & ~0x17) == ASdrdy))
					driveopunlocked(drive, newdrive);
			}
			break;
		}
		break;
	}
	statechange(drive);
	iunlock(drive);
}

static void
satakproc(void*)
{
	int i;

	for(;;){
		tsleep(&up->sleep, return0, 0, Nms);
		for(i = 0; i < niadrive; i++)
			if(iadrive[i] != nil)
				checkdrive(iadrive[i], i);
	}
}

static int
isctlrjabbering(Ctlr *ctlr, ulong cause)
{
	ulong now;

	now = sys->ticks;		/* might not fit */
	if (now != ctlr->lastintr0) {	/* new tick? */
		ctlr->intrs = 0;
		ctlr->lastintr0 = now;
	}
	if (++ctlr->intrs > Maxintrspertick) {
		iprint("#S/sd%C: %lud intrs per tick for no serviced "
			"drive; cause %#lux mport %d\n",
			ctlr->sdev->idno, ctlr->intrs, cause, ctlr->mport);
		ctlr->intrs = 0;
		return 1;
	}
	return 0;
}

static int
isdrivejabbering(Drive *drive)
{
	ulong now;

	now = sys->ticks;		/* might not fit */
	if (now != drive->lastintr0) {	/* new tick? */
		drive->intrs = 0;
		drive->lastintr0 = now;
	}
	if (++drive->intrs > Maxintrspertick) {
		iprint("#S/%s: %lud interrupts per tick\n",
			drive->unit->name, drive->intrs);
		drive->intrs = 0;
		return 1;
	}
	return 0;
}

static Intrsvcret
ahciintr(Ureg*, void *vc)
{
	uint portno, pass;
	ulong cause, mask;
	Ahba *hba;
	Ctlr *ctlr;
	Drive *drive;

	ctlr = vc;
	ilock(ctlr);
	hba = ctlr->hba;
	if (hba->isr == 0) {
		iunlock(ctlr);
		return Intrnotforme;		/* irq could be shared */
	}
	ctlr->allintrs++;
	/*
	 * an interrupt may be posted for a port after we have checked
	 * it, so go around a few times or until hba->isr goes to 0.
	 * don't livelock by making unlimited passes.
	 * the interrupt could be in response to ahciexec() or issueio().
	 */
	for (pass = NCtlrdrv; (cause = hba->isr) != 0 && pass > 0; pass--)
		while (cause) {
			portno = Clzbits - 1 - clz(cause);
			if (portno >= NCtlrdrv)
				panic("ahciintr: implausible port # %d",
					portno);
			mask = 1 << portno;
			cause &= ~mask;
			if (portno > ctlr->mport) {
				iprint("ahciintr: port # %d out of range\n",
					portno);
				continue;
			}

			drive = ctlr->rawdrive + portno;
			ilock(drive);
			isdrivejabbering(drive);
			if(drive->port->isr && hba->pi & mask)
				updatedrive(drive);
			hba->isr = mask; /* clear interrupts from this port */
			iunlock(drive);
		}
	if (cause) {
		iprint("#S/sd%C: interrupting port(s) unserviced, bits: %#lux\n",
			ctlr->sdev->idno, cause);
		isctlrjabbering(ctlr, cause);
	}
	iunlock(ctlr);
	return Intrforme;
}

/* checkdrive, called from satakproc, will prod the drive while we wait */
static void
awaitspinup(Drive *drive)
{
	int ms;
	ushort s;
	char *name;

	ilock(drive);
	if(drive->unit == nil || drive->port == nil) {
		panic("awaitspinup: nil drive->unit or drive->port");
		iunlock(drive);
		return;
	}
	name = (drive->unit? drive->unit->name: nil);
	s = drive->port->sstatus;
	if(!(s & Devpresent)) {			/* never going to be ready? */
		iunlock(drive);
		return;
	}

	for (ms = 20000; ms > 0; ms -= 50)
		switch(drive->state){
		case Dnull:
			/* absent; done */
			iunlock(drive);
			dprint("awaitspinup: %s in null state\n", name);
			return;
		case Dready:
		case Dnew:
			if(drive->sectors || drive->mediachange) {
				/* ready to use; done */
				iunlock(drive);
				dprint("awaitspinup: %s ready!\n", name);
				return;
			}
			/* fall through */
		default:
		case Dmissing:			/* normal waiting states */
		case Dreset:
		case Doffline:			/* transitional states */
		case Derror:
		case Dportreset:
			iunlock(drive);
			asleep(50);
			ilock(drive);
			break;
		}
	print("awaitspinup: %s didn't spin up after 20 seconds\n", name);
	iunlock(drive);
}

static int
intrson(Ctlr *ctlr)
{
	char name[32];
	SDev *sdev;

	ilock(ctlr);
	if(!ctlr->intrson) {
		sdev = ctlr->sdev;
		if (sdev)
			snprint(name, sizeof name, "%s (%s)",
				(sdev->name && sdev->name[0]? sdev->name:
				 "sdE"), sdev->ifc->name);
		else
			strcpy(name, "#S");
		ctlr->hba = (Ahba *)ctlr->mmio;	/* hack */
		enableintr(ctlr, ahciintr, ctlr, name);
		ahcienable(ctlr->hba);
		ctlr->intrson = 1;
	}
	iunlock(ctlr);
	return 1;
}

static int
iaverify(SDunit *unit)
{
	Ctlr *ctlr;
	Drive *drive;

	ctlr = unit->dev->ctlr;
	drive = ctlr->drive[unit->subno];
	ilock(drive);
	drive->unit = unit;
	iunlock(drive);
	checkdrive(drive, drive->driveno);  /* ctlr->dma0 + drive->driveno */

	/*
	 * hang around until disks are spun up and thus available as
	 * nvram, dos file systems, etc.  you wouldn't expect it, but
	 * the intel 330 ssd takes a while to `spin up'.
	 */
	awaitspinup(drive);
	return 1;
}

static int
iaenable(SDev *sdev)
{
	Ctlr *ctlr;
	static int once;

	ctlr = sdev->ctlr;
	ilock(ctlr);
	if(!ctlr->enabled) {
		if(once == 0) {
			once = 1;
			kproc("ahci", satakproc, 0);
		}
		if(ctlr->ndrive == 0)
			panic("iaenable: zero ctlr->ndrive");
		pcisetbme(ctlr->pcidev);
		ctlr->enabled = 1;
	}
	iunlock(ctlr);
	intrson(ctlr);
	return 1;
}

static int
iadisable(SDev *sdev)
{
	Ctlr *ctlr;
	char name[32];

	ctlr = sdev->ctlr;
	ilock(ctlr);
	if (ctlr->enabled) {
		ahcidisable(ctlr->hba);
		snprint(name, sizeof(name), "sd%c (%s)", sdev->idno,
			sdev->ifc->name);
		disableintr(ctlr, ahciintr, ctlr, name);
		ctlr->enabled = 0;
	}
	iunlock(ctlr);
	return 1;
}

/*
 * Initialise a drive known to exist.
 * Returns boolean for success, except if there is a media change waiting
 * to be detected, when it will return >= 2.
 */
static int
iaonline(SDunit *unit)
{
	int r;
	Ctlr *ctlr;
	Drive *drive;

	ctlr = unit->dev->ctlr;
	drive = ctlr->drive[unit->subno];
	r = 0;

	if(drive == nil)
		return 0;
	if(drive->feat & Datapi && drive->mediachange){
		r = scsionline(unit);
		if(r > 0)
			drive->mediachange = 0;
		return r;
	}
	if (drive->state == Dmissing)
		return 0;
	if (unit->sectors && !drive->mediachange)
		return 1;		/* current medium already inited */

	ilock(drive);
	if(drive->mediachange){
		r = SDmedchanged;
		drive->mediachange = 0;
		/* devsd resets this after online is called; why? */
		unit->sectors = drive->sectors;
		unit->secsize = 512;		/* default size */
	} else if(drive->state == Dready)
		r = 1;
	iunlock(drive);

	/*
	 * smartrs==0 is disabled, 0xff is enable error, perhaps because there's
	 * no smart feature.
	 */
	if (drive->smartrs == 0) {
		runsmartable(drive, 0);	/* turn on smart, if available */
		readsmart(drive);
	}
	return r;
}

/*
 * returns locked list d->portm->list!
 * the ahci command is built in drive->ctab->cfis.
 */
static Alist*
ahcibuild(Drive *drive, SDreq *r, void *data, int n, vlong lba)
{
	uchar dir, llba;
	uvlong phys;
	uchar *cfis;
	Alist *list;
	Aprdt *prdt;
	/* combinations of (read, write)x(not llba, llba) cmds */
	static uchar tab[2][2] = { 0xc8, 0x25, 0xca, 0x35, };

	llba = drive->feat & Dllba? 1: 0;
	dir = isscsiwrite(r->cmd[0]);

	qlock(drive);
	cfis = drive->ctab->cfis;
	cfis[0] = 0x27;
	cfis[1] = 0x80;
	cfis[2] = tab[dir][llba];	/* ahci command */
	cfis[3] = 0;
	/*
	 * next cfis bytes: sector   or lba low	7:0
	 *		cylinder low or lba mid	15:8
	 *		cylinder hi  or lba hi	23:16
	 */
	leputl(&cfis[4], lba);
	cfis[7] = Obs | 0x40;	/* overwrite highest lba byte; 0x40 == lba */
	if(llba == 0)
		cfis[7] |= (lba>>24) & 7;
	/*
	 * sector (exp)		lba 	31:24
	 * cylinder low (exp)	lba	39:32
	 * cylinder hi (exp)	lba	47:40
	 */
	leputl(&cfis[8], lba >> 24);
	cfis[11] = 0;		/* overwrite highest lba byte; features (exp) */
	cfis[12] = n;		/* sector count */
	cfis[13] = n >> 8;	/* sector count (exp) */
	cfis[14] = 0;		/* r */
	cfis[15] = 0;		/* control */
	*(ulong *)(cfis + 16) = 0;
	coherence();

	list = drive->list;
	list->flags = Lprdtl | Lpref | Lcfl;
	if(dir == Write)
		list->flags |= Lwrite;
	list->len = 0;
	phys = PCIWADDR(drive->ctab);	/* cfis, etc. */
	list->ctab = phys;
	list->ctabhi = phys>>32;
	coherence();

	prdt = &drive->ctab->prdt;
	phys = PCIWADDR(data);
	prdt->dba = phys;
	prdt->dbahi = phys>>32;
	prdt->count = 1<<31 | (drive->unit->secsize*n - 2) | 1;
	coherence();

	return list;
}

/*
 * returns locked list d->portm->list!
 * the ahci command is built in drive->ctab->cfis.
 */
static Alist*
ahcibuildpkt(Drive *drive, SDreq *r, void *data, int n)
{
	int fill, len;
	uvlong phys;
	uchar *cfis;
	Actab *ctab;
	Alist *list;
	Aprdt *prdt;

	qlock(drive);
	fill = drive->feat&Datapi16? 16: 12;
	if((len = r->clen) > fill)
		len = fill;
	ctab = drive->ctab;
	memmove(ctab->atapi, r->cmd, len);
	memset(ctab->atapi+len, 0, fill-len);

	cfis = ctab->cfis;
	cfis[0] = 0x27;
	cfis[1] = 0x80;
	cfis[2] = 0xa0;
	if(n != 0)
		cfis[3] = 1;	/* dma */
	else
		cfis[3] = 0;	/* features (exp); */
	cfis[4] = 0;		/* sector		lba low	7:0 */
	cfis[5] = n;		/* cylinder low		lba mid	15:8 */
	cfis[6] = n >> 8;	/* cylinder hi		lba hi	23:16 */
	cfis[7] = Obs;
	*(ulong*)(cfis + 8) = 0;
	*(ulong*)(cfis + 12) = 0;
	*(ulong*)(cfis + 16) = 0;
	coherence();

	list = drive->list;
	list->flags = Lprdtl | Lpref | Latapi | Lcfl;
	if(r->write != 0 && data)
		list->flags |= Lwrite;
	list->len = 0;
	phys = PCIWADDR(ctab);		/* cfis, etc. */
	list->ctab = phys;
	list->ctabhi = phys>>32;
	coherence();

	if(data == 0)
		return list;

	prdt = &ctab->prdt;
	phys = PCIWADDR(data);
	prdt->dba = phys;
	prdt->dbahi = phys>>32;
	prdt->count = 1<<31 | (n - 2) | 1;
	coherence();
	return list;
}

enum { Waitmissing = -1, Waitok, Waitcomingup, };

/*
 * wait up to 15 seconds for d to come ready.
 * return -1 on missing device, 0 if okay, 1 if coming up.
 */
static int
waitready(Drive *drive)
{
	ulong s, i, δ;

	dprint("waitready...");
	for(i = 0; i < 15000; i += 250){
		if(drive->state == Dreset || drive->state == Dportreset ||
		    drive->state == Dnew)
			return Waitcomingup;
		δ = sys->ticks - drive->lastseen;
		/* good idea? */
		if(0 && drive->state == Dmissing)
			return Waitmissing;
		if(drive->state == Dnull || δ > 10*1000)
			return Waitmissing;

		ilock(drive);
		s = drive->port->sstatus;
		iunlock(drive);
		if((s & Intpm) == 0 && δ > 1500)
			return Waitmissing;	/* no detect */
		if(drive->state == Dready &&
		    (s & Devdet) == (Devphycomm|Devpresent))
			return Waitok;	/* ready, present & phy. comm. */
		esleep(250);
	}
	print("%s: not responding; offline\n", drive->unit->name);
	setstate(drive, Doffline);
	return Waitmissing;
}

/* acquire qlock on drive->portm, return once the port is ready or timed out */
static int
qlockreadyport(Drive *drive)
{
	int i, n;

	qlock(drive);
	/* waitready waits up to 15 seconds */
	/* don't wait forever; drive could be missing or empty */
	for (n = 2; (i = waitready(drive)) == Waitcomingup && n > 0; n--) {
		qunlock(drive);
		esleep(1);
		qlock(drive);
	}
	return i;
}

static int
flushcache(Drive *drive)
{
	int i;

	i = -1;
	if(qlockreadyport(drive) == Waitok)
		i = ahciflushcache(drive);
	qunlock(drive);
	return i;
}

/* assumes drive->port->ci is 0.  drive->flag should *not* be 0. */
static void
prifnotdone(SDreq *r, Drive *drive, uvlong ns)
{
	if ((drive->flag & (Fdone|Ferror)) == 0)
		print("%s: scsi cmd %#ux: ahci clear but Fdone & Ferror "
			"not set after %,lld ns.!\n",
			drive->unit->name, r->cmd[0], ns);
}

enum {
	Fastms = 4,
	Slopms = 1,
};

static void
updstats(Stats *st, vlong ns)
{
	if (ns <= 0)
		return;			/* cpus had different time bases */
	if (ns < st->lowwat)
		st->lowwat = ns;
	if (ns > st->hiwat)
		st->hiwat = ns;
	if (ns > 100000000)		/* way slow, as in dying? */
		st->outliers++;
	else {
		st->tot += ns;
		st->count++;
	}
}

/*
 * drive->portm must contain the ahci command to be issued, and must be qlocked.
 * we issue it and wait for it using `as', for at most maxms.
 *
 * ahciopdone becomes true really fast sometimes, maybe before an ahci
 * interrupt (perhaps on another cpu) can be processed.
 * check the done condition soon so that a masked interrupt doesn't
 * cause a long wait.
 *
 * measurements of hardware in 2019 show these transfer times in µs.:
 * ssd: low 1 avg 193 high 4,280; slow 5/474,965 outliers 0
 * mag disks, oldest to newest:
 *	low 0 avg 65 high 746,406; slow 8,170/242,887,459 outliers 339
 *	low 1 avg 52 high 71,098; slow 6,248/8,624,315 outliers 0
 *	low 0 avg 28 high 17,588; slow 385/242,746,255 outliers 0
 *	low 1 avg 28 high 17,949; slow 10/8,389,518 outliers 0
 *	low 27 avg 200 high 71,204; slow 687/172,956 outliers 0
 * smart claims all drives are normal, but outliers are probably an early
 * warning of approaching disk failure.
 */
static int
issueio(SDreq *r, Drive *drive, Asleep *as, int maxms)
{
	int ret;
	ulong sttick, ticks;
	uvlong stns;
	Aport *port;

	ret = SDok;	/* in our case, just means: caller, don't retry */

	/* start this transfer */
	port = assetup(drive, as);
	ilock(drive);
	drive->flag = drive->doneticks = 0;
	drive->donens = 0;
	ckdriveidle(drive, as, "issueio", getcallerpc(&drive));
	drive->active++;
	port->ci = as->watchbits;	/* issue cmd in drive's slot 0 */
	stns = (Measure? todget(nil): 0);
	drive->intick = sttick = sys->ticks;
	iunlock(drive);

	/* wait for this operation to complete (or not), but not forever. */
	awaitiodone(as, Fastms);
	/* NB: we may be running on a different cpu now */

	if (!ahciopdone(as)) {		/* it's a slow operation? */
		/*
		 * we think this slop is needed due to cpus having slightly
		 * different notions of time.
		 */
		if (Measure && todget(nil) - stns >= (Fastms-Slopms)*1000000)
			drive->slow++;
		awaitiodone(as, maxms);
		/* NB: we may be running on a different cpu now */
	}
	if (Measure)
		updstats(drive, todget(nil) - stns);

	/* sort out what to do about any failures & advise caller */
	if(ahciopdone(as)) {
		/*
		 * if the interrupt hasn't been processed yet, simulate one
		 * to set the flag bits for caller.
		 */
		ilock(drive);
		if (drive->flag == 0)
			updatedrive(drive);
		iunlock(drive);
		prifnotdone(r, drive, drive->donens - stns);
	} else {
		ticks = sys->ticks - sttick;
		print("%s: scsi cmd %#ux timed out after %ld ticks; simulating interrupt\n",
			drive->unit->name, r->cmd[0], ticks);
		ahciintr(nil, drive->ctlr);		/* last gasp */
		if(ahciopdone(as))
			prifnotdone(r, drive, drive->donens - stns);
		else {
			print("%s: port->ci still not clear; retrying\n",
				drive->unit->name);
			ret = SDretry;
		}
	}

	doneio(drive);
	return ret;
}

static int
iswormwrite(Drive *d, uchar *cmd)
{
	switch(d->unit->inquiry[0] & SDinq0periphtype){
	case SDperworm:
	case SDpercd:
		if (isscsiwrite(cmd[0]))
			return 1;
		break;
	}
	return 0;
}

/*
 * my empty bd-r drive generates these:
 *
 * sdE1: medium change; task=0x2451 for scsi cmd 0x25:
 * 	error, deferred write error, device ready, aborted command, media change
 *
 * but not Enm (no media).
 */
static int
prerror(SDreq *r, Drive *drive, uchar *cmd, char *name, int task)
{
	if (task & (Emc<<8)) {
		/* don't chatter away about empty drives, normally */
		dprint("%s: medium change; task=%#ux for scsi cmd %#ux:\n\t",
			name, task, cmd[0]);
		if (debug)
			prtask(task);
		dprint("\n");
	}
	/* this test was wrong; it omitted the <<8, so tested ASdwe */
	else if((task & (Eidnf<<8)) == 0) {
		print("%s: atapi i/o error task=%#ux for scsi cmd %#ux:\n\t",
			name, task, cmd[0]);
		prtask(task);
		print("\n");
	}
	if (iswormwrite(drive, cmd))
		r->status = SDeio;
	else
		r->status = SDcheck;
	return r->status;
}

static int
iariopkt(SDreq *r, Drive *drive)
{
	int n, try, flag, task, sts;
	char *name;
	uchar *cmd, *data;
	Asleep *as;

	cmd = r->cmd;
	name = drive->unit->name;

	aprint("ahci: iariopkt: %04#ux %04#ux %c %d %p\n",
		cmd[0], cmd[2], "rw"[r->write], r->dlen, r->data);
	if(cmd[0] == ScmdMsense10 && (cmd[2] & 0x3f) == 0x3f)
		return sdmodesense(r, cmd, drive->info, drive->infosz);
	r->rlen = 0;

	as = malloc(sizeof *as);
	try = 0;
retry:
	for (;;) {
		data = r->data;
		n = MIN(r->dlen, Xfrmaxsects*512); /* truncate long transfers */

		/* construct atapi command.  acquires drive qlock. */
		ahcibuildpkt(drive, r, data, n);
		sts = waitready(drive);
		if (sts != Waitcomingup)
			break;
		qunlock(drive);
		esleep(1);
	}
	if (sts == Waitmissing) {
		qunlock(drive);
		free(as);
		return SDeio;
	}

	/* issue & wait for the atapi command */
	if (issueio(r, drive, as, Iopktwaitms) == SDretry) {	/* timed-out? */
		/*
		 * write retries cannot succeed on write-once media,
		 * so just accept any failure.
		 */
		qunlock(drive);
		r->status = (iswormwrite(drive, cmd)? SDeio: SDcheck);
		goto done;
	}

	/* deal with any errors */
	ilock(drive);
	flag = drive->flag;
	task = drive->port->task;
	iunlock(drive);
	if(task & (Efatal<<8) || task & (ASbsy|ASdrq) && drive->state == Dready){
		drive->port->ci = 0;
		ahcirecover(drive);
		task = drive->port->task;
		flag &= ~Fdone;		/* either an error or do-over */
	}
	qunlock(drive);			/* from ahcibuildpkt */

	if(flag & Ferror) {
		free(as);
		return prerror(r, drive, cmd, name, task);
	}
	if(flag & Fdone) {
		data += n;
		r->rlen = data - (uchar*)r->data;
		r->status = SDok;
	}
	/* not done, no error?  presumably timed-out */
	else if(++try >= 10){
		print("%s: too many retries; bad disk?\n", name);
		r->status = SDcheck;
	}
	/* write retries cannot succeed on write-once media. */
	else if (iswormwrite(drive, cmd))
		r->status = SDeio;
	else {
		print("%s: atapi retry\n", name);
		goto retry;
	}
done:
	free(as);
	return r->status;
}

/* returns true iff r contains an illegal scsi command or we fake it here. */
static int
isbadcmd(SDreq *r)
{
	int i;
	uchar *cmd;
	Drive *drive;
	SDunit *unit;

	unit = r->unit;
	drive = ((Ctlr *)unit->dev->ctlr)->drive[unit->subno];
	cmd = r->cmd;
	dprint("%s: iario: %04#ux %04#ux %c %d data %#p\n",
		drive->unit->name, cmd[0], cmd[2], "rw"[r->write],
		r->dlen, r->data);
	if(*cmd == ScmdSynccache || *cmd == ScmdSynccache16){
		if(flushcache(drive) == 0)
			sdsetsense(r, SDok, 0, 0, 0);
		else
			/* 3, 0xc, 2: write error, reallocation failed */
			sdsetsense(r, SDcheck, 3, 0xc, 2);
		return 1;
	}

	if((i = sdfakescsi(r, drive->info, drive->infosz)) != SDnostatus){
		r->status = i;
		return 1;
	}

	if(!isscsiread(*cmd) && !isscsiwrite(*cmd)){
		print("%s: bad scsi cmd %#.2ux\n", drive->unit->name, cmd[0]);
		r->status = SDcheck;
		return 1;
	}
	if(scsicmdlen(*cmd) == 16) {
		/* ata commands only go to 48-bit lba, currently */
		if(cmd[2] || cmd[3]) {
			/* 3, 0xc, 2: write error, reallocation failed */
			sdsetsense(r, SDcheck, 3, 0xc, 2);
			return 1;
		}
	}
	return 0;
}

/* determine: another retry, or just give up? */
static int
ckretries(SDreq *r, Drive *drive, int *tryp, char *rw, uvlong lba)
{
	if(++*tryp < 10){
		print("%s: retrying %s blk %lld\n", drive->unit->name, rw, lba);
		return SDretry;
	} else {
		print("%s: %s too many retries at blk %lld; bad disk?\n",
			drive->unit->name, rw, lba);
		r->status = SDeio;
		return SDeio;
	}
}

/* render an opinion about how to proceed after timing out */
static int
timeout(SDreq *r, Drive *drive, int *tryp, char *rw, uvlong lba)
{
	int flag, task;

	if(drive->flag == 0) {		/* Fdone and Ferror not set? */
		print("%s: timed-out; simulating interrupt\n",
			drive->unit->name);
		ahciintr(nil, drive->ctlr);		/* last gasp */
	}

	ilock(drive);
	flag = drive->flag;
	task = drive->port->task;
	iunlock(drive);
	if(flag == 0) {
		print("%s: timed-out; Fdone, Ferror still not set\n",
			drive->unit->name);
		return ckretries(r, drive, tryp, rw, lba);
	}
	if(flag & Ferror){
		print("%s: %s error at blk %lld, task=%#ux\n",
			drive->unit->name, rw, lba, task);
		r->status = SDeio;
		return SDeio;
	}
	return SDok;
}

/*
 * try the transfer once, with no retries but with advisory error status.
 * if return is not SDok nor SDretry, caller must free its Asleep*.
 */
static int
onetryio(SDreq *r, Drive *drive, Asleep *as, uchar *data, int nsects,
	uvlong lba, int *tryp)
{
	int task;
	char *rw;

	/*
	 * set up ahci command and wait for device ready.
	 * ahcibuild acquires drive->portm qlock.
	 */
	ahcibuild(drive, r, data, nsects, lba);
	switch(waitready(drive)){
	case Waitcomingup:
		qunlock(drive);
		esleep(1);
		print("%s: waiting for spin up\n", drive->unit->name);
		return SDretry;
	case Waitmissing:
		qunlock(drive);
		return SDeio;
	}

	rw = r->write? "write": "read";
	if (issueio(r, drive, as, Iowaitms) == SDretry) {
		qunlock(drive);
		return ckretries(r, drive, tryp, rw, lba);
	}

	/* gather error statuses, perhaps try to recover */
	ilock(drive);
	task = drive->port->task;
	iunlock(drive);
	if (task & (Efatal<<8) ||
	    task & (ASbsy|ASdrq) && drive->state == Dready){
		print("%s: unhappy drive; recovering\n", drive->unit->name);
		drive->port->ci = 0;
		ahcirecover(drive);
		drive->flag &= ~Fdone;	/* either an error or do-over */
	}
	qunlock(drive);			/* from ahcibuild */

	if(drive->flag == Fdone)
		return SDok;			/* common case: no errors */
	return timeout(r, drive, tryp, rw, lba);
}

static int
iario(SDreq *r)
{
	int nsects, try;
	ulong count, totcount;		/* # of sectors to transfer */
	uvlong origlba, lba;
	char *name, *rw;
	uchar *data;
	Asleep *as;
	Drive *drive;
	SDunit *unit;

	unit = r->unit;
	drive = ((Ctlr *)unit->dev->ctlr)->drive[unit->subno];
	if(drive->feat & Datapi)
		return iariopkt(r, drive);	/* optical drives, etc. */

	if (isbadcmd(r))
		return r->status;
	if(r->data == nil)
		return SDok;
	origlba = totcount = 0;
	scsilbacount(r->cmd, r->clen, &origlba, &totcount);
	if(r->dlen < totcount * unit->secsize)
		totcount = r->dlen / unit->secsize;

	name = drive->unit->name;
	rw = r->write? "write": "read";
	dprint("%s: iario: %s regs %#p clen %d lba %lld bytes %ld\n",
		name, rw, drive->port, r->clen, origlba,
		totcount * unit->secsize);

	/* dole out the transfer in 64KB pieces and retry on errors */
	as = malloc(sizeof *as);
	assetup(drive, as);
	try = 0;
	data = r->data;
	lba = origlba;
	count = totcount;
	while (count > 0) {
		nsects = MIN(count, Xfrmaxsects); /* limit sector count */
		switch (onetryio(r, drive, as, data, nsects, lba, &try)) {
		case SDok:
			break;
		case SDretry:			/* back up, do it all again */
			data = r->data;
			lba = origlba;
			count = totcount;
			continue;
		default:			/* unrecoverable; bail out */
			free(as);
			return r->status;
		}

		data += nsects * unit->secsize;
		lba  += nsects;
		count -= nsects;
	}
	r->rlen = data - (uchar*)r->data;
	r->status = SDok;
	free(as);
	return SDok;
}

/*
 * bloody intel have changed the pci config space in the series 100 and
 * c236 chipsets (e.g., did 0xa102).  Pmap and Ppcs were adjacent shorts;
 * they are now adjacent longs.
 */

/* pci space configuration for ahci in intel controller hubs */
enum {
	Pmap	= 0x90,		/* address map */
	Ppcs	= 0x92,		/* port control & status; original, short */
	Ppcs236	= 0x94,		/* " " "; new, long */

	Pmapoahci = 1<<6,	/* 2-bit field: enable ahci mode on ich9/10 */
	Pmapo6ports = 1<<5,	/* enable all 6 ports on ich9/10 */
};

/*
 * intel-specific ahci controller set up.
 * Ich has present & enabled bits in 92-93 (Ppcs), but those bits in Ich100
 * are high disable bits of Pmap.
 */
static void
iasetupahci(Ctlr *ctlr)
{
	int nports;
	ulong *lmmio;

	lmmio = (ulong *)ctlr->mmio;

	/* disable ide cmd block decoding for both ide ctlrs */
	pcicfgw16(ctlr->pcidev, 0x40, pcicfgr16(ctlr->pcidev, 0x40) & ~(1<<15));
	pcicfgw16(ctlr->pcidev, 0x42, pcicfgr16(ctlr->pcidev, 0x42) & ~(1<<15));

	nports = (lmmio[0] & MASK(5)) + 1;	/* ahci cap reg. */
	/* unfortunately, not all Tich100 controllers advertise > 6 ports */
	if (nports > 6) {
		ctlr->widecfg = 1;	/* ich9/10 only have bits for 6 ports */
		ctlr->type = Tich100;
	}
	lmmio[0x4/4] |= Hae;		/* enable ahci mode (ghc reg.) */
	lmmio[0xc/4] = MASK(nports);	/* mixed rw/ro pi reg. */
	coherence();

	if (!ctlr->widecfg)
		pcicfgw16(ctlr->pcidev, Pmap, Pmapoahci | Pmapo6ports);
}

/* configure all drives as ahci sata on intel sata controller. */
static int
iaahcimode(Ctlr *ctlr, int nports)
{
	ulong pmap, ppcs;
	Pcidev *p;

	p = ctlr->pcidev;
	if (ctlr->widecfg) {
		pmap = pcicfgr32(p, Pmap);
		ppcs = pcicfgr32(p, Ppcs236);
		pcicfgw32(p, Ppcs236, ppcs | MASK(nports));
	} else {
		pmap = pcicfgr16(p, Pmap);
		ppcs = pcicfgr16(p, Ppcs);
		pcicfgw16(p, Ppcs, ppcs | MASK(6));
	}
	dprint("iaahcimode: pci cfg Pmap %#lux Ppcs %#lux\n", pmap, ppcs);
	return 0;
}

/*
 * these device ids are only for SATA controllers in AHCI (including RAID)
 * mode, but not IDE.
 */

/*
 * ich100 seems to be what chipsets from 2015 forward use.
 * c232 and c242 provide only 6 ports, so can't be
 * automatically detected, alas.  x99, c236, c246, x299, c422 and c62?
 * implement at least 8.
 */
static ushort didich100[] = {
	0x8d02,			/* c610 series */
	0x8d04,			/* c610 series raid */
	0x8d06,			/* c610 series raid */
	0x8d0e,			/* c610 series raid */
	0x8d62,			/* c610 series */
	0x8d64,			/* c610 series raid */
	0x8d66,			/* c610 series raid */
	0x8d6e,			/* c610 series raid */
	0x9dd3,			/* c240, series 300, cannon point */
	0xa102,			/* c236 */
	0xa103,			/* c236 */
	0xa105,			/* c236? raid */
	0xa106,			/* c236 raid */
	0xa107,			/* c236 raid */
	0xa10f,			/* c236? raid */
	0xa182,			/* c620 series */
	0xa186,			/* c620 series raid */
	0xa1d2,			/* c620 series */
	0xa352,			/* c242 cannon lake: only 6 ports */
	0xa353,			/* c242 cannon lake: " */
};

typedef struct Iclassic Iclassic;
struct Iclassic {
	ushort	did;
	ushort	clrbits;
};

/* these are limited to 6 ports by their register layout */
static Iclassic didiclassic[] = {
	0x1c02, 1,		/* c200 series */
	0x1c04, 1,		/* c200 series raid */
	0x1c06, 0,		/* z68 raid */
	0x1d02, 0,		/* c600 series */
	0x1d04, 0,		/* c600 series raid */
	0x1e02, 1,		/* c210 series */
	0x1e04, 3,		/* c210 series raid */
	0x1e0e, 0,		/* c210 series raid */
	0x24d1, 0,		/* 82801eb/er ich5 */
	0x27c1, 4,		/* 82801g[bh]m ich7 */
	0x2821, 0,		/* 82801h[roh] ich8r */
	0x2824, 1,		/* 82801h[b] ich8 */
	0x2829, 0x100,		/* ich8/9m */
	0x2922, 1,		/* ich9(r) */
	0x3a02, 0,		/* 82801jd/do ich10 */
	0x3a22, 0x101,		/* ich10, 3400 series */
	0x3b28, 7,		/* 3400 series */
	0x8c02, 1,		/* c220 series */
	0x8c04, 3,		/* c220 series raid */
	0x8c0e, 1,		/* c220 series raid */
	0x9c83, 0,		/* wildcat point */
	0xa282, 0,		/* c200 series */
	0xa286, 0,		/* c200 series raid */
};

static int
didtype(Pcidev *p)
{
	int i, did, type;
	char *ahci;
	Iclassic *icp;

	did = p->did;
	switch(p->vid){
	case Vintel:
		for (i = 0; i < nelem(didich100); i++)
			if (did == didich100[i])
				return Tich100;
		for (icp = didiclassic; icp < didiclassic + nelem(didiclassic);
		    icp++)
			if ((did & ~icp->clrbits) == icp->did)
				return Tich;
		if(did == Didesb)
			return Tesb;
		break;
	case Vatiamd:
		if(did == 0x4380 || did == 0x4390 || did == 0x4391){
			print("ahci: sb600: vid %#ux did %#ux\n", p->vid, did);
			return Tsb600;
		}
		break;
	case Vmarvell:
		if (did == 0x9123)
			print("ahci: marvell: sata 3 controller has delusions "
				"of something on unit 7\n");
		break;
#ifdef VIA_RAID_SATA
	case 0x1106:				/* VIA */
		if (did == 0x3249) {	/* VIA VT6421 SATA/RAID (verify) */
			print("ahci: via: vid %#4.4ux did %#4.4ux\n",
				p->vid, did);
			return Tich;
		}
		break;
#endif
	}
	if (!ISAHCI(p))
		return -1;

	/* if it's actually Tich100, iasetupahci might figure it out */
	if (p->vid == Vintel) {
		type = Tich;
		ahci = getconf("*ahci");	/* override def type */
		if (ahci && strcmp(ahci, "ich") != 0)
			type = Tich100;
		print("ahci: unknown intel, assuming %s: did %#4.4ux\n",
			tname[type], did);
	} else {
		type = Tunk;
		print("ahci: %s: ahci 1.0 vid %#4.4ux did %#4.4ux\n",
			tname[type], p->vid, did);
	}
	return type;
}

static int
newctlr(Ctlr *ctlr, int nunit)
{
	int i, n;
	Drive *drive;
	Pcidev *p;
	SDev *sdev;

	sdev = ctlr->sdev;
	ctlr->ndrive = sdev->nunit = nunit;
	ctlr->mport = ctlr->hba->cap & MASK(5);

	i = (ctlr->hba->cap >> 20) & MASK(4);		/* iss */
	p = ctlr->pcidev;
	print("#S/sd%C: pci %ux/%ux %s %s: regs %#p, %d ports in use, irq %d\n",
		sdev->idno, p->vid, p->did, tname[ctlr->type],
		(i < nelem(modename)? modename[i]: "gok"), ctlr->physio,
		nunit, ctlr->pcidev->intl);

	/* map the drives -- they don't all need to be enabled. */
	ctlr->rawdrive = malloc(NCtlrdrv * sizeof(Drive));
	if(ctlr->rawdrive == nil) {
		print("ahci: out of memory\n");
		return -1;
	}
	n = 0;
	for(i = 0; i < NCtlrdrv; i++) {
		drive = ctlr->rawdrive + i;
		drive->portno = i;
		drive->port = nil;
		drive->driveno = -1;
		drive->sectors = 0;
		drive->serial[0] = ' ';
		drive->ctlr = ctlr;
		if((ctlr->hba->pi & (1<<i)) == 0)
			continue;
		drive->port = (Aport*)(ctlr->mmio + 0x100 + 0x80*i);
		drive->driveno = n;
		ctlr->drive[n] = drive;
		iadrive[niadrive + n++] = drive;
	}
	for(i = 0; i < n; i++)
		if(ctlr->drive[i] && ahciidle(ctlr->drive[i]->port) == -1){
			dprint("ahci: %s: port %d wedged; abort\n",
				tname[ctlr->type], i);
			return -1;
		}
	for(i = 0; i < n; i++) {
		drive = ctlr->drive[i];
		if (drive) {
			drive->mode = DMsatai;
			configdrive(drive);
		}
	}
	return n;
}

static int
ahcictlrsetup(Ctlr *ctlr, SDev *sdev)
{
	int nunit;
	Pcidev *p;

	ctlr->hba = (Ahba*)ctlr->mmio;
	ctlr->enabled = 0;
	ctlr->widecfg = ctlr->type == Tich100;

	sdev->ifc = &sdahciifc;
	sdev->idno = 'E' + sdev - sdevs;
	sdev->ctlr = ctlr;
	ctlr->sdev = sdev;

	p = ctlr->pcidev;
	if(p->vid == Vintel && p->did != Didesb)
		iasetupahci(ctlr);
	/* switches hba to ahci mode */
	nunit = ahciconf(ctlr);
	if(nunit < 1)
		return -1;
	ahcihbareset(ctlr->hba);
	if(p->vid == Vintel && iaahcimode(ctlr, nunit) == -1)
		return -1;
	/* defer intrson to iaenable, hoping for non-0 cpu for intrs */
//	intrson(ctlr);
	return newctlr(ctlr, nunit);
}

static SDev*
iapnp(void)
{
	int n, type;
	ulong io;
	Ctlr *ctlr;
	Pcidev *p;
	SDev *head, *tail, *sdev;
	static int done;

	if(done++)
		return nil;

	memset(olds, 0xff, sizeof olds);
	p = nil;
	head = tail = nil;
	while((p = pcimatch(p, 0, 0)) != nil){
		type = didtype(p);
		if (type == -1 || !ISAHCI(p))
			continue;
		if(niactlr >= NCtlr){
			print("ahci: iapnp: %s: too many controllers\n",
				tname[type]);
			break;
		}
		io = p->mem[Abar].bar & ~0xf;
		if (io == 0)
			continue;

		ctlr = iactlr + niactlr;
		memset(ctlr, 0, sizeof *ctlr);
		ctlr->physio = (uchar *)io;
		ctlr->mmio = vmap(io, p->mem[Abar].size);
		if(ctlr->mmio == 0){
			print("ahci: %s: address %#luX in use did=%#x\n",
				tname[type], io, p->did);
			continue;
		}
		ctlr->pcidev = p;
		ctlr->type = type;

		sdev = sdevs + niactlr;
		memset(sdev, 0, sizeof *sdev);

		n = ahcictlrsetup(ctlr, sdev);
		if (n < 0) {
			vunmap(ctlr->mmio, p->mem[Abar].size);
			continue;
		}
		niadrive += n;
		niactlr++;
		if(head)
			tail->next = sdev;
		else
			head = sdev;
		tail = sdev;
	}
	return head;
}

static char* smarttab[] = {
	"unset",
	"error",
	"threshold exceeded",
	"normal"
};

static char *
pflag(char *s, char *e, uchar f)
{
	uchar i;

	for(i = 0; i < 8; i++)
		if(f & (1 << i))
			s = seprint(s, e, "%s ", flagname[i]);
	return seprint(s, e, "\n");
}

static int
iarctl(SDunit *unit, char *p, int l)
{
	char buf[32];
	char *e, *op;
	Aport *port;
	Ctlr *ctlr;
	Drive *drive;
	Stats *st;

	ctlr = unit->dev->ctlr;
	if(ctlr == nil) {
		print("iarctl: nil unit->dev->ctlr\n");
		return 0;
	}
	drive = ctlr->drive[unit->subno];
	port = drive->port;

	e = p+l;
	op = p;
	if(drive->state == Dready){
		/*
		 * devsd has already generated "inquiry" line using the model,
		 * so printing drive->model here would be redundant.
		 */
		p = seprint(p, e, "serial\t%s\n", drive->serial);
		p = seprint(p, e, "firmware %s\n", drive->firmware);
		p = seprint(p, e, "smart\t");
		if(drive->smartrs == 0xff)
			p = seprint(p, e, "enable error\n");
		else if(drive->smartrs == 0)
			p = seprint(p, e, "disabled\n");
		else
			p = seprint(p, e, "%s\n", smarttab[drive->smart]);
	}else
		p = seprint(p, e, "no disk present [%s]\n",
			diskstates[drive->state]);
	p = seprint(p, e, "flag\t");
	if (port->cmd & Aesp)
		p = seprint(p, e, "esata ");
	p = pflag(p, e, drive->feat);
	if (debug) {
		serrstr(port->serror, buf, buf + sizeof buf - 1);
		p = seprint(p, e, "reg\ttask %#lux cmd %#lux serr %#lux %s "
			"ci %#lux is %#lux; sig %#lux sstatus %06#lux\n",
			port->task, port->cmd, port->serror, buf,
			port->ci, port->isr, port->sig, port->sstatus);
	}
	st = &drive->Stats;
	if (Measure && st->count > 0)
		p = seprint(p, e, "transfers in µs: low %,lld avg %,lld "
			"high %,lld; slow %,lld/%,lld outliers %,lld\n",
			st->lowwat/1000, (st->tot/st->count)/1000,
			st->hiwat/1000, st->slow, st->count, st->outliers);
	p = seprint(p, e, "geometry %llud %lud\n", drive->sectors,
		unit->secsize);
	return p - op;
}

static void
runflushcache(Drive *drive)
{
	long t0;

	t0 = sys->ticks;
	if(flushcache(drive) != 0)
		error(Eio);
	dprint("ahci: flush in %ld ms\n", sys->ticks - t0);
}

static void
forcemode(Drive *drive, char *mode)
{
	int i;

	for(i = 0; i < nelem(modename); i++)
		if(strcmp(mode, modename[i]) == 0 ||
		   strcmp(mode, oldmodename[i]) == 0)
			break;
	if(i >= nelem(modename))
		i = 0;
	ilock(drive);
	drive->mode = i;
	iunlock(drive);
}

/* n == 0 enables smart; n == 1 disables it. */
static void
runsmartable(Drive *drive, int i)
{
	if(waserror()){
		qunlock(drive);
		drive->smartrs = 0;
		nexterror();
	}
	if(qlockreadyport(drive) == Waitmissing)
		error(Eio);
	drive->smartrs = smart(drive, i);
	drive->smart = 0;
	qunlock(drive);
	poperror();
}

static void
forcestate(Drive *drive, char *state)
{
	int i;

	for(i = 0; i < nelem(diskstates); i++)
		if(strcmp(state, diskstates[i]) == 0)
			break;
	if(i == nelem(diskstates))
		error(Ebadctl);
	setstate(drive, i);
}

/*
 * force this driver to notice a change of medium if the hardware doesn't
 * report it.
 */
static void
changemedia(SDunit *unit)
{
	Ctlr *ctlr;
	Drive *drive;

	ctlr = unit->dev->ctlr;
	drive = ctlr->drive[unit->subno];
	ilock(drive);
	drive->mediachange = 1;
	unit->sectors = 0;
	iunlock(drive);
}

static int
readsmart(Drive *drive)
{
	if(drive->smartrs == 0)
		return -1;
	if(waserror()){
		qunlock(drive);
		drive->smartrs = 0;
		nexterror();
	}
	if(qlockreadyport(drive) == Waitmissing)
		error(Eio);
	drive->smart = 2 + smartrs(drive);
	qunlock(drive);
	poperror();
	return 0;
}

static int
donop(Drive *drive, Cmdbuf *cmd)
{
	if((drive->feat & Dnop) == 0){
		cmderror(cmd, "no drive support");
		return -1;
	}
	if(waserror()){
		qunlock(drive);
		nexterror();
	}
	if(qlockreadyport(drive) == Waitmissing)
		error(Eio);
	ahcinop(drive);
	qunlock(drive);
	poperror();
	return 0;
}

static int
iawctl(SDunit *unit, Cmdbuf *cmd)
{
	char **f;
	Ctlr *ctlr;
	Drive *drive;
	uint i;

	ctlr = unit->dev->ctlr;
	drive = ctlr->drive[unit->subno];
	f = cmd->f;

	if(strcmp(f[0], "change") == 0)
		changemedia(unit);
	else if(strcmp(f[0], "flushcache") == 0)
		runflushcache(drive);
	else if(strcmp(f[0], "identify") ==  0){
		i = strtoul(f[1]? f[1]: "0", 0, 0);
		if(i > 0xff)
			i = 0;
		dprint("ahci: %d %#ux\n", i, drive->info[i]);
	}else if(strcmp(f[0], "mode") == 0)
		forcemode(drive, f[1]? f[1]: "satai");
	else if(strcmp(f[0], "nop") == 0)
		return donop(drive, cmd);
	else if(strcmp(f[0], "reset") == 0)
		forcestate(drive, "reset");
	else if(strcmp(f[0], "smart") == 0){
		if(drive->smartrs == 0){
			cmderror(cmd, "smart not enabled");
			return -1;
		}
		readsmart(drive);
	}else if(strcmp(f[0], "smartdisable") == 0)
		runsmartable(drive, 1);
	else if(strcmp(f[0], "smartenable") == 0)
		runsmartable(drive, 0);
	else if(strcmp(f[0], "state") == 0)
		forcestate(drive, f[1]? f[1]: "null");
	else{
		cmderror(cmd, Ebadctl);
		return -1;
	}
	return 0;
}

static char *
portrng(char *p, char *e, uint x)
{
	int i, a;

	p[0] = 0;
	a = -1;
	for(i = 0; i < 32; i++){
		if((x & (1<<i)) == 0){
			if(a != -1 && i - 1 != a)
				p = seprint(p, e, "-%d", i - 1);
			a = -1;
			continue;
		}
		if(a == -1){
			if(i > 0)
				p = seprint(p, e, ", ");
			p = seprint(p, e, "%d", a = i);
		}
	}
	if(a != -1 && i - 1 != a)
		p = seprint(p, e, "-%d", i - 1);
	return p;
}

/* must emit exactly one line per controller (per sd(3)) */
static char*
iartopctl(SDev *sdev, char *p, char *e)
{
	ulong cap;
	char pr[25];
	Ahba *hba;
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	hba = ctlr->hba;
	p = seprint(p, e, "sd%C ahci regs %#p irq %d: %,lud intrs, ",
		sdev->idno, ctlr->physio, ctlr->pcidev->intl, ctlr->allintrs);
	cap = hba->cap;
#define has(x, str) if(cap & (x)) p = seprint(p, e, "%s ", (str))
	has(Hs64a, "64-bit");
	has(Hsxs, "esata");
	if (debug) {
		has(Hsalp, "alp");
		has(Hsam, "am");
		has(Hsclo, "clo");
		has(Hcccs, "coal");
		has(Hems, "ems");
		has(Hsal, "led");
		has(Hsmps, "mps");
		has(Hsncq, "ncq");
		has(Hssntf, "ntf");
		has(Hspm, "pm");
		has(Hpsc, "pslum");
		has(Hssc, "slum");
		has(Hsss, "ss");
#undef has
		p = seprint(p, e, "iss %ld ncs %ld ",
			(cap>>20) & 0xf, (cap>>8) & 0x1f);
	}
	p = seprint(p, e, "%ld ports; ", (cap & 0x1f) + 1);
	if (debug)
		p = seprint(p, e, "ghc %#lux isr %#lux ", hba->ghc, hba->isr);
	portrng(pr, pr + sizeof pr, hba->pi);
	return seprint(p, e, "port bitmap %#lux %s ver %#lux\n",
		hba->pi, pr, hba->ver);
}

static int
iawtopctl(SDev *, Cmdbuf *cmd)
{
	int *v;
	char **f;

	v = nil;
	f = cmd->f;
	if (f[0] == nil)
		return 0;
	if(strcmp(f[0], "debug") == 0)
		v = &debug;
	else if(strcmp(f[0], "idprint") == 0)
		v = &prid;
	else if(strcmp(f[0], "aprint") == 0)
		v = &datapi;
	else
		cmderror(cmd, Ebadctl);

	switch(cmd->nf){
	default:
		cmderror(cmd, Ebadarg);
	case 1:
		*v ^= 1;
		break;
	case 2:
		if(f[1])
			*v = strcmp(f[1], "on") == 0;
		else
			*v ^= 1;
		break;
	}
	return 0;
}

SDifc sdahciifc = {
	"ahci",

	iapnp,
	nil,		/* legacy */
	iaenable,
	iadisable,

	iaverify,
	iaonline,
	iario,
	iarctl,
	iawctl,

	scsibio,
	nil,		/* probe */
	nil,		/* clear */
	iartopctl,
	iawtopctl,
};
