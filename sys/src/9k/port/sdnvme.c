/*
 * driver for NVM Express 1.1 interface to PCI-Express solid state disk
 * (i.e., flash memory).
 *
 * currently the controller is in the drive, so there's no multiplexing
 * of drives through the controller.  multiple namespaces (actually number
 * spaces) are assumed to refer to different views of the same disk
 * (different block sizes).
 *
 * many features of NVME are ignored in the interest of simplicity and speed.
 * many of them are intended to jump on a bandwagon (e.g., VMs) or check a box.
 * using interrupts rather than polling costs us about 4% in large-block
 * sequential read performance.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/sd.h"

#define PAGEOF(ctlr, p) ((uintptr)(p) & ~((uintptr)(ctlr)->pgsz-1))

#define QFULL(qp)	((qp)->qidx.hd == qidxplus1((qp), (qp)->qidx.tl))
#define QEMPTY(qp)	((qp)->qidx.hd == (qp)->qidx.tl)

#define nvmeadmissue(ctlr, op, nsid, buf) \
	nvmeissue(ctlr, &ctlr->qpair[Qadmin], nil, op, nsid, buf, 0)

enum {
	/* fundamental constants */
	Qadmin,			/* queue-pair ordinals; Qadmin fixed at 0 */
	Qio,
	Nqueues,

	Vall = 1<<Qadmin | 1<<Qio,	/* all interesting vectors */

	Subq = 0,
	Complq,
	Qtypes,

	Nsunused = 0,
	Nsall	= ~0ul,

	Idns	= 0,
	Idctlr,
	Idnsids,

	Minsect	= 512,

	/* tunable parameters */
	Debugintr = 0,
	Debugns = 0,
	Measure	= 1,			/* flag: measure transfer times */

	Timeout = 20*1000,	/* adjust to taste. started at 2000 ms. */

	/*
	 * NVME page size must be >= sector size.  anything over 8K only
	 * benefits bulk copies and benchmarks.
	 */
	Startpgsz = Sdalign,	/* on samsung sm951, 4k ≤ page_size ≤ 128MB */

	Qlen	= 32,	/* defaults; queue lengths must be powers of 2 < 4K */
	Cqlen	= 16,

	NCtlr	= 8,	/* each takes a pci-e or m.2 slot */
	NCtlrdrv= 1,
	NDrive	= NCtlr * NCtlrdrv,

	Reserved = (ushort)~0,		/* placeholder cmdid */
};

/* admin commands */
enum Adminops {
	Admmkiosq	= 1,	/* create i/o submission q */
	Admmkiocq	= 5,	/* create i/o completion q */
	Admid		= 6,	/* identify */
};

/* I/O commands */
enum Opcode {
	Cmdflush	= 0,
	Cmdwrite	= 1,
	Cmdread		= 2,
	Cmdwriteuncorr	= 4,
	Cmdcompare	= 5,
	Cmddsm		= 9,
};

typedef struct Cmd Cmd;
typedef struct Completion Completion;
typedef struct Ctlr Ctlr;
typedef struct Ctlrid Ctlrid;
typedef struct Doorbell Doorbell;
typedef struct Lbafmt Lbafmt;
typedef struct Nsid Nsid;
typedef struct Nvindx Nvindx;
typedef struct Qpair Qpair;
typedef struct Regs Regs;
typedef struct Transfer Transfer;

extern SDifc sdnvmeifc;

struct Nvindx {
	ushort	hd;		/* remove at this index */
	ushort	tl;		/* add at this index */
};

struct Qpair {
	Cmd	*q;		/* base of Cmd array */
	Nvindx	qidx;
	short	sqlen;
	uchar	writelast;	/* flag: read or write in last cmd? */

	Completion *cmpl;	/* base of Completion array */
	Nvindx	cidx;
	short	cqlen;
	uchar	phase;		/* initial phase bit setting in cmpl */
};

/* these are reused and never freed */
struct Transfer {
	Transfer *next;
	Rendez;
	int	status;		/* from completion */
	ulong	qtm;		/* time at enqueue */
	ushort	cmdid;		/* 0 means available */
	uchar	done;		/* flag for rendezvous */
	uchar	rdwr;
	uvlong	stcyc;		/* cycles at enqueue */
};

struct Ctlr {
	Regs*	regs;		/* memory-mapped I/O registers */
	SDev*	sdev;
	Intrcommon;
	uintptr	port;		/* physical addr of I/O registers */

	int	pgsz;		/* size of an `nvme page' */
	int	minpgsz;
	int	mdts;		/* actual value, not log2; unit minpgsz */
	short	sqlen;		/* sub q len */
	short	cqlen;		/* compl q len */
	int	stride;	/* bytes from base of one doorbell reg. to the next */

	/* per controller */
	QLock;			/* serialise q notifications */
	Rendez;			/* q empty/full notifications */
	Lock;			/* intr svc */
	Lock	issuelock;	/* inflight & q heads & tail mostly */
	Lock	xfrlock;
	Lock	shutlock;
	short	inflight;	/* count of xfrs in progress */
	int	intrsena;	/* interrupts we care about */
	Transfer *xfrs;		/* transfers in flight or done */
	Qpair	qpair[Nqueues];	/* use a single admin queue pair */

	/* per-drive scalars, since there is only one drive */
	vlong	sectors;	/* total, copy to SDunits */
	int	secsize;	/* sector size, copy to SDunits */
	int	ns;		/* namespace index in Nsid->lbafmt */
	/* stats */
	/* high water marks of read, write queues: 2 & 3, so far. */
	short	maxqlen[2];
	uvlong	count[3];	/* number of xfers */
	uvlong	totus[3];	/* sum of (µs per xfer) */

	/* per-drive arrays */
	char	serial[20+1];
	char	model[40+1];
	char	fw[8+1];
};

struct Regs {
	uvlong	cap;		/* controller capabilities */
	ulong	vs;		/* version */
	/* intm* bits are actually vector number offsets */
	ulong	intmset;	/* intr mask set: bit # is i/o completion q # */
	ulong	intmclr;	/* intr mask clear: " */
	ulong	cc;		/* controller configuration */
	ulong	nssrc;		/* reset, iff cap.nssrs set */
	ulong	csts;		/* controller status */
	ulong	_rsvd2;		/* reserved */
	ulong	aqa;		/* admin queue attributes */
	uvlong	asq;		/* admin submit queue base address */
	uvlong	acq;		/* admin completion queue base address */
	uchar	_pad0[0x1000 - 0x38];
	/* this is the nominal doorbell layout, with stride of 4 */
	struct Doorbell {
		ulong	sqtl;	/* submission queue tail */
		ulong	cqhd;	/* completion queue head */
	} doorbell[Nqueues];
};

/*
 * making the doorbell stride variable at run time requires changing the
 * declaration and addressing of the Regs->doorbell array, making it clunkier.
 * supposedly non-zero strides are only desirable in VMs, for efficiency.
 */
/* clunky doorbell register addressing for any stride */
/* instead of &ctlr->regs->doorbell[qid].sqtl */
#define doorbellsqtl(ctlr, qp) (ulong *)\
	((char *)(ctlr)->regs->doorbell + (ctlr)->stride*(Qtypes*(qp) + Subq))
/* instead of &ctlr->regs->doorbell[qid].cqhd */
#define doorbellcqhd(ctlr, qp) (ulong *)\
	((char *)(ctlr)->regs->doorbell + (ctlr)->stride*(Qtypes*(qp) + Complq))

enum {
	/* cap */
	Nssrs		= 1ull << 36,

	/* cc */
	Enable		= 1 << 0,
	Cssnvm		= 0 << 4,	/* nvm command set */
	Cssmask		= 7 << 4,
	Shnnone		= 0 << 14,	/* shutdown style */
	Shnnormal	= 1 << 14,
	Shnabrupt	= 2 << 14,
	Shnmask		= 3 << 14,

	/* csts */
	Rdy		= 1 << 0,	/* okay to add to sub. q */
	Cfs		= 1 << 1,	/* controller fatal status */
	Shstnormal	= 0 << 2,	/* shutdown status */
	Shstoccur	= 1 << 2,
	Shstcmplt	= 2 << 2,
	Shstmask	= 3 << 2,
	Nssro		= 1 << 4,
};

struct Cmd {
	/* common 40-byte header */
	uchar	opcode;		/* command dword 0 */
	uchar	flags;
	ushort	cmdid;
	ulong	nsid;
	ulong	cdw2[2];	/* not used */
	uvlong	metadata;
	uvlong	prp1;		/* buffer memory address */
	uvlong	prp2;		/* zero, buffer addr, or prp list addr */
	union {
		ulong	cdw10[6]; /* admin: command dwords 10-15 */
		struct {	/* nvm i/o */
			uvlong	slba;
			ushort	length;
			ushort	control;
			ulong	dsmgmt;
			/* rest are for end-to-end protection only */
			ulong	reftag;
			ushort	apptag;
			ushort	appmask;
		};
	};
};

enum {
	/* cdw10[1] for Admmkiocq */
	Ien	= 1<<1,		/* intr enabled */
	Pc	= 1<<0,		/* physically contiguous */
};

struct Completion {
	ulong	specific;
	ulong	_pad;
	ushort	sqhd;
	ushort	sqid;
	ushort	cmdid;
	ushort	stsphs;		/* status + 1 phase bit */
};

enum {
	Phase	= 1,		/* phase bit in stsphs */
};

struct Ctlrid {			/* identify cmd, cns=1 */
	ushort	pcivid;
	ushort	pcissvid;
	char	serial[20];	/* space-padded, unterminated strings */
	char	model[40];
	char	fw[8];
	char	_72_[77-72];
	uchar	mdts;		/* log2(max data xfr size), unit: min pg sz */
				/* 0 is unlimited */
	char	_78_[516-78];	/* ... lots of uninteresting stuff ... */
	ulong	nns;		/* number of namespaces present */

	char	_520_[526-520];	/* ... lots of uninteresting stuff ... */
	/* after 1.1 */
	ushort	awun;		/* atomic write unit */
	/* ... lots of uninteresting stuff through offset 4095 */
};

struct Nsid {			/* identify cmd, cns=0 */
	uvlong	size;		/* in logical blocks */
	uvlong	cap;
	uvlong	used;
	uchar	feat;		/* option feature bits */
	uchar	nlbafmts;	/* elements in lbafmt */
	uchar	fmtlbasz;	/* low 4 bits index lbafmt */
	uchar	mdcap;
	uchar	dpc;		/* end-to-end data protection stuff */
	uchar	dps;
	uchar	optnmic;
	uchar	optrescap;	/* reservation capabilities */

	/* after 1.1 */
	uchar	fpi;
	uchar	dlfeat;
	/* atomicity */
	ushort	nawun;		/* atomic write size; see awun */
	ushort	nawupf;		/* " under power failure */
	ushort	nacwu;		/* compare-&-swap size */
	ushort	nabsn;		/* boundary size */
	ushort	nabspf;		/* " under power failure */
	ushort	nabo;		/* boundary offset */
	/* end atomicity */
	ushort	noiob;		/* optimal i/o boundary */
	/* end after 1.1 */
	uchar	_pad0[128-48];	/* uninteresting stuff */

	struct Lbafmt {
		ushort	mdsize;
		uchar	lglbasize; /* log2(lba size) */
		uchar	relperf;
	} lbafmt[16];
	/* ... uninteresting stuff through offset 4095 */
};

CTASSERT(sizeof(Cmd) == 64, cmd_wrong_size);
CTASSERT(sizeof(Completion) == 16, compl_wrong_size);

static Lock clocklck;
static int clockrunning;
static ulong iosttck;		/* tick of most recently-started i/o */
static int nctlrs;
static Ctlr *ctlrs[NCtlr];

static void
cidxincr(Ctlr *ctlr, Qpair *qp)
{
	if (++qp->cidx.hd >=  ctlr->cqlen) {
		qp->cidx.hd = 0;
		qp->phase ^= Phase;
	}
}

#ifdef unused
static void
isfatal(Regs *regs, char *where)
{
	if (regs->csts & Cfs)
		panic("nvme: fatal controller error %s", where);
}
#endif

static Transfer *
findxfr(Ctlr *ctlr, int cmdid)
{
	Transfer *xfr;

	for (xfr = ctlr->xfrs; xfr; xfr = xfr->next)
		if (xfr->cmdid == cmdid)
			return xfr;
	return nil;
}

/*
 * cqhd is head of the completion queue.
 * mark its transfer done, notify anybody waiting for it.
 */
static void
completexfr(Ctlr *ctlr, Completion *cqhd, int qid)
{
	Transfer *xfr;

	if (Debugintr)
		iprint("intr q %d cmdid %d...", qid, cqhd->cmdid);
	xfr = findxfr(ctlr, cqhd->cmdid);
	if (xfr == nil)
		panic("sd%C0: nvmeinterrupt: unexpected completion cmd id %d",
			ctlr->sdev->idno, cqhd->cmdid);
	if (xfr->qtm && TK2MS(sys->ticks) - xfr->qtm >= Timeout)
		iprint("sd%C0: nvmeinterrupt: completed cmd id %d but "
			"took more than %d s.\n",
			ctlr->sdev->idno, cqhd->cmdid, Timeout/1000);

	/* cycle-based measurements */
	if (Measure) {
		uvlong cycs;

		cycles(&cycs);
		cycs -= xfr->stcyc;
		ctlr->totus[xfr->rdwr] += cycs / m->cpumhz;
		ctlr->count[xfr->rdwr]++;
	}

	xfr->status = cqhd->stsphs & ~Phase;
	xfr->done = 1;
	xfr->qtm = 0;
	wakeup(xfr);		/* notify of completion */
}

/* advance sub. q head to completion's, notify waiters */
static void
advancesqhd(Ctlr *ctlr, Qpair *qp, Completion *cqhd, int qid)
{
	if (Debugintr)
		iprint("sw q %d sqhd set to %d...", qid, cqhd->sqhd);
	qp->qidx.hd = cqhd->sqhd;
	wakeup(ctlr);		/* notify of sqhd advance */
}

/*
 * advance compl. q head, notify ctlr., which will extinguish intr source
 * (by acknowledging this completion) and remove cqhd from the compl. q.
 */
static void
advancecqhd(Ctlr *ctlr, Qpair *qp, int qid)
{
	cidxincr(ctlr, qp);
	if (Debugintr)
		iprint("doorbell q %d cqhd set to %d\n", qid, qp->cidx.hd);
	*doorbellcqhd(ctlr, qid) = qp->cidx.hd;
	coherence();
}

/*
 * Act on and clear the interrupt(s).
 * In order to share PCI IRQs, just ignore spurious interrupts.
 * Advances queue head indices past completed operations.
 */
static Intrsvcret
nvmeinterrupt(Ureg *, void* arg)
{
	int qid, ndone, donepass; /* qid's not a great name (see path.qid) */
	ulong causes;
	Completion *cqhd;
	Ctlr *ctlr;
	Qpair *qp;
	Regs *regs;

	ctlr = arg;
	regs = ctlr->regs;
	causes = regs->intmset;
	USED(causes);
	ilock(&ctlr->issuelock); /* keep other cpus out of intr svc, indices */
	if (ctlr->inflight == 0) {	/* not expecting an interrupt? */
		/* probably lost a race with polling: nothing to do */
		iunlock(&ctlr->issuelock);
		return Intrnotforme;
	}

	ndone = 0;
	do {
		donepass = 0;
		for (qid = Nqueues - 1; qid >= 0; qid--) /* scan i/o q 1st */
			for (qp = &ctlr->qpair[qid]; ; ) {
				cqhd = &qp->cmpl[qp->cidx.hd];
				if ((cqhd->stsphs & Phase) == qp->phase)
					break;
				completexfr(ctlr, cqhd, qid);
				advancesqhd(ctlr, qp, cqhd, qid);
				/*
				 * toggles qp->phase if qp->cidx.hd wraps when
				 * incr'd.
				 */
				advancecqhd(ctlr, qp, qid);
				if (--ctlr->inflight < 0)
					iprint("nvmeinterrupt: inflight botch\n");
				ndone++, donepass++;
			}
	} while (donepass > 0);
	/* unmask intr. sources of interest iff transfers are in flight */
	if (ctlr->inflight == 0) {
		iosttck = 0;
		ctlr->intrsena = 0;
	} else
		regs->intmclr = Vall;
	iunlock(&ctlr->issuelock);
	if (ndone > 0)
		return Intrforme;
	else
		return Intrnotforme;
}

/* return cmd id other than zero and Reserved */
static int
cidalloc(void)
{
	int thisid;
	static int cid;
	static Lock cidlck;

	ilock(&cidlck);
	++cid;
	if ((ushort)cid == 0 || (ushort)cid == Reserved)
		cid = 1;
	thisid = cid;
	iunlock(&cidlck);
	return thisid;
}

static void
mkadmcmd(Ctlr *ctlr, Cmd *cmd, int op)
{
	/* we are using single-message msi */
	switch (op) {
	case Admmkiocq:
		cmd->cdw10[0] = (ctlr->cqlen - 1)<<16 | Qio;
		cmd->cdw10[1] = Ien | Pc;	/* vector 0 since no msi-x */
		break;
	case Admmkiosq:
		cmd->cdw10[0] = (ctlr->sqlen - 1)<<16 | Qio;
		cmd->cdw10[1] = Qio<<16 | Pc;	/* completion q id */
		break;
	case Admid:
		if (cmd->nsid == Nsall) {
			cmd->cdw10[0] = Idctlr;
			cmd->nsid = 0;
		} else
			cmd->cdw10[0] = Idns;
		break;
	}
}

/* fill in submission queue entry *cmd */
static void
mkcmd(Ctlr *ctlr, Cmd *cmd, SDreq *r, int op, ulong nsid, void *buf, int qid,
	vlong lba)
{
	long count;
	ulong pgsz, dlen;
	uintptr addr;
	SDunit *unit;

	memset(cmd, 0, sizeof *cmd);
	cmd->opcode = op;
	cmd->cmdid = cidalloc();
	cmd->nsid = nsid;
	addr = (uintptr)buf;			/* will be Sdalign-ed */
	dlen = (r? r->dlen: 512);
	if (addr != 0) {
		if (addr < KZERO)
			print("nvme mkcmd: %#p not kernel virtual address\n",
				addr);
		/* each prp entry points to at most an nvme page */
		cmd->prp1 = PCIWADDR((void *)addr);	/* must be Sdalign-ed */
		pgsz = ctlr->pgsz;
		assert(dlen <= 2*pgsz);
		if (r && dlen > pgsz && dlen <= 2*pgsz)
			cmd->prp2 = PAGEOF(ctlr, cmd->prp1) + pgsz;
		else
			cmd->prp2 = 0;
	}
	if (qid == Qadmin) {
		mkadmcmd(ctlr, cmd, op);	/* uncommon case */
		return;
	}

	if (op != Cmdread && op != Cmdwrite && op != Cmdflush)
		panic("sdnvme: bad cmd %d\n", op);
	cmd->slba = lba;
	count = 0;
	cmd->length = (ushort)(count - 1);	/* sectors */
	if (r) {
		assert(r->data == buf);
		unit = r->unit;
		if (unit) {
			count = dlen / unit->secsize;
			if (count <= 0) {
				print("nvmeissue: sector count %ld for i/o of length %ld\n",
					count, dlen);
				return;
			}
			assert(unit->secsize * count <= dlen);
		}
	}
	assert(nsid);
	cmd->length = (ushort)(count - 1);	/* sectors */
}

static void
updmaxqlen(Ctlr *ctlr, Qpair *qp)
{
	int qlen;
	short *qlenp;

	qlen = (qp->qidx.tl + qp->sqlen - qp->qidx.hd) % qp->sqlen;
	qlenp = &ctlr->maxqlen[qp->writelast];
	if (qlen > *qlenp)
		*qlenp = qlen;
}

/*
 * send a command via the submission queue.
 * call with ctlr->issuelock held.
 * advances submission queue's tail index.
 */
static void
sendcmd(Ctlr *ctlr, Qpair *qp, Cmd *qtl, Transfer *xfr)
{
	int qid;

	xfr->done = 0;
	xfr->cmdid = qtl->cmdid;
	xfr->qtm = TK2MS(sys->ticks);
	qid = qp - ctlr->qpair;
	if (Debugintr)
		iprint("issue q %d cmdid %d...", qid, xfr->cmdid);

	/*
	 * Notify controller of new submission queue entry,
	 * which triggers execution of it.
	 */
	updmaxqlen(ctlr, qp);
	if (Measure)
		cycles(&xfr->stcyc);
	else
		xfr->stcyc = 0;

	ctlr->inflight++;
	iosttck = sys->ticks;
	*doorbellsqtl(ctlr, qid) = qp->qidx.tl;		/* start i/o */
	coherence();
	ctlr->regs->intmclr = ctlr->intrsena = Vall;	/* unmask intrs */
}

static int
doneio(void* arg)
{
	return ((Transfer *)arg)->done;
}

static uint
qidxplus1(Qpair *qp, uint idx)
{
	if (++idx >= qp->sqlen)
		idx = 0;
	return idx;
}

static int
qnotfull(void *arg)
{
	return !QFULL((Qpair *)arg);
}

static int
qempty(void *arg)
{
	return QEMPTY((Qpair *)arg);
}

static Transfer *
getfreexfr(Ctlr *ctlr)
{
	Transfer *xfr;

	ilock(&ctlr->xfrlock);			/* allocate xfr */
	xfr = findxfr(ctlr, 0);
	if (xfr == nil) {
		xfr = malloc(sizeof *xfr);
		if (xfr == nil)
			panic("nvmeissue: out of memory");
		xfr->next = ctlr->xfrs;
		ctlr->xfrs = xfr;	/* add new xfr to chain */
	}
	xfr->cmdid = Reserved;
	xfr->qtm = 0;
	iunlock(&ctlr->xfrlock);
	return xfr;
}

/*
 * if needed, wait for the sub q to drain a lot or a little.
 * not infallible, so test afterward under lock.
 */
static void
qdrain(Ctlr *ctlr, Qpair *qp, SDreq *r)
{
	if (QFULL(qp)) {
		qlock(ctlr);			/* wait for q space */
		while (QFULL(qp))
			sleep(ctlr, qnotfull, qp);
		qunlock(ctlr);
	}
	/*
	 * don't mix reads and writes in the queue, to avoid read-before-write
	 * problems.
	 */
	if (r && qp->writelast != r->write) {
		qlock(ctlr);
		if (qp->writelast != r->write)
			sleep(ctlr, qempty, qp);  /* changing, so drain */
		qp->writelast = r->write;
		qunlock(ctlr);
	}
}

/* drain and return with ctlr->issuelock held */
static void
qdrainilock(Ctlr *ctlr, Qpair *qp, SDreq *r)
{
	int again;

	/* serialise composition of cmd in place at sq tail */
	do {
		qdrain(ctlr, qp, r);

		again = 0;
		ilock(&ctlr->issuelock);
		/* test again under lock */
		if (QFULL(qp) || r && qp->writelast != r->write) {
			/* lost a race; uncommon case */
			iunlock(&ctlr->issuelock);
			again = 1;
		}
	} while (again);
	/* issuelock still held */
}

static void
prerr(int sts)
{
	if (sts)
		iprint("nvmeissue: cmd error status %#ux: "
			"code %#ux type %d more %d do-not-retry %d\n", sts,
			(sts >>  1) & MASK(8), (sts >>  9) & MASK(3),
			(sts >> 14) & MASK(1), (sts >> 15) & MASK(1));
}

/*
 * add new nvme command to tail of submission queue of Qpair,
 * and wait for it to complete.  return status with phase bit zeroed.
 */
static int
nvmeissue(Ctlr *ctlr, Qpair *qp, SDreq *r, int op, ulong nsid, void *buf,
	vlong lba)
{
	ushort sts;
	Cmd *qtl;
	Transfer *xfr;

	xfr = getfreexfr(ctlr);
	if (op == Cmdwrite)
		xfr->rdwr = Write;
	else if (op == Cmdread)
		xfr->rdwr = Read;
	else
		xfr->rdwr = 2;

	/* serialise composition of cmd in place at sq tail */
	qdrainilock(ctlr, qp, r);
	/* ctlr->issuelock is now held */

	/* Reserve a space and update sub. q tail index past it. */
	qtl = &qp->q[qp->qidx.tl];
	qp->qidx.tl = qidxplus1(qp, qp->qidx.tl);

	/*
	 * Compose the command struct at the tail of the submission queue.
	 * mkcmd converts buf to physical address space.
	 */
	mkcmd(ctlr, qtl, r, op, nsid, buf, qp - ctlr->qpair, lba);
	sendcmd(ctlr, qp, qtl, xfr);			/* start cmd */
	iunlock(&ctlr->issuelock);

	/* this is the only process waiting for this xfr. */
	while(waserror())
		;
	tsleep(xfr, doneio, xfr, Timeout);
	poperror();
	if (!xfr->done) {
		/* we see this with the Samsung 983 DCT. */
		nvmeinterrupt(nil, ctlr);
		if (!xfr->done)
			panic("sd%C0: nvmeissue: cmd id %d didn't complete "
				"in %d s.", ctlr->sdev->idno, xfr->cmdid,
				Timeout/1000);
	}

	sts = xfr->status;
	xfr->cmdid = 0;				/* xfr available for re-use */
	if (sts)
		prerr(sts);
	return sts;
}

/* map scsi to nvm opcodes */
static int
scsiop2nvme(uchar* cmd)
{
	if (isscsiread(*cmd))
	 	return Cmdread;
	else if (isscsiwrite(*cmd))
	 	return Cmdwrite;
	else {
		iprint("scsiop2nvme: scsi cmd %#ux unexpected\n", *cmd);
		return -1;
	}
}

static int
issueios(SDreq *r)
{
	int n, max, iostat, nvmcmd;
	ulong count;			/* sectors */
	uvlong lba;
	Ctlr *ctlr;
	SDunit *unit;

	unit = r->unit;
	ctlr = unit->dev->ctlr;
	nvmcmd = scsiop2nvme(r->cmd);
	if (nvmcmd == -1)
		error("nvme: scsi cmd unexpected");
	scsilbacount(r->cmd, r->clen, &lba, &count);
	if(count * unit->secsize > r->dlen)
		count = r->dlen / unit->secsize;
	max = 2*ctlr->pgsz / unit->secsize;	/* needs 1 or 2 prp addrs */
	/* to do this in generality, need to allocate a prp list page */
	if (0)
		max = (ctlr->mdts? ctlr->mdts * ctlr->minpgsz: 128*KB) /
			unit->secsize;
	iostat = 0;

	for (; count > 0; count -= n){
		n = MIN(count, max);
		r->dlen = n * unit->secsize;
		iostat = nvmeissue(ctlr, &ctlr->qpair[Qio], r, nvmcmd,
			ctlr->ns, r->data, lba);
		if (iostat)
			break;
		lba += n;
		r->data = (uchar *)r->data + r->dlen;
	}
	return iostat;
}

/*
 * Issue an I/O (SCSI) command to a controller and wait for it to complete.
 * The command and its length is contained in r->cmd and r->cmdlen.
 * If any data is to be returned, r->dlen should be non-zero, and
 * the returned data will be placed in r->data.
 */
static int
nvmerio(SDreq* r)
{
	int i, iostat;
	ulong origdlen;
	uchar *origdata;
	static char info[256];

	if(*r->cmd == ScmdSynccache || *r->cmd == ScmdSynccache16)
		return sdsetsense(r, SDok, 0, 0, 0);

	/* scsi command to get information about the drive or disk? */
	if((i = sdfakescsi(r, info, sizeof info)) != SDnostatus){
		r->status = i;
		return i;
	}

	if(r->data == nil)
		return SDok;

	/*
	 * Cap the size of individual transfers and repeat if needed.
	 * Save r->data and r->dlen, and restore them after issueios.
	 * could call scsibio, which allocates an SDreq.
	 */
	origdata = r->data;
	origdlen = r->dlen;

	assert(r->unit->secsize >= Minsect &&
		r->unit->secsize <= ((Ctlr *)r->unit->dev->ctlr)->pgsz);
	iostat = issueios(r);

	r->rlen = (uchar *)r->data - origdata;
	r->data = origdata;
	r->dlen = origdlen;
	r->status = SDok;
	if (iostat != 0) {
		r->status = SDeio;
		/* 3, 0xc, 2: write error, reallocation failed */
		sdsetsense(r, SDcheck, 3, 0xc, 2);
	}
	return r->status;
}

static int
nvmerctl(SDunit* unit, char* p, int l)
{
	int n;
	Ctlr *ctlr;
	Regs *regs;

	if((ctlr = unit->dev->ctlr) == nil)
		return 0;
	regs = ctlr->regs;
	n = snprint(p, l, "config %#lux capabilities %#llux status %#lux\n",
		regs->cc, regs->cap, regs->csts);
	/*
	 * devsd has already generated "inquiry" line using the model,
	 * so printing ctlr->model here would be redundant.
	 */
	n += snprint(p+n, l-n, "serial %s\n", ctlr->serial);
	if(unit->sectors)
		n += snprint(p+n, l-n, "geometry %lld %lud\n",
			unit->sectors, unit->secsize);
	return n;
}

static uvlong
nonzerocnt(Ctlr *ctlr, int rdwr)
{
	uvlong count;

	count = ctlr->count[rdwr];
	return count? count: 1;			/* don't divide by zero */
}

/*
 * must emit exactly one line per controller (sd(3)).
 *
 * on a busy fossil, the output was:
 *	sdo nvme [⋯]: max q lens: rd 2/32 wr 2/32; avg µs: rd 24 wr 10
 *
 *	; grep sdo /dev/irqalloc 
 *	 97  11   19489834 msi sdo (nvme) cpu2 lapic 4
 *
 * which suggests that more than one item per queue is rare,
 * thus one gets most of the benefit we see from a queue depth of 1,
 * at least with page size of 4K and block size of 8K.
 */
static char *
nvmertopctl(SDev *sdev, char *p, char *e)
{
	uvlong rdcnt, wrcnt;
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	rdcnt = nonzerocnt(ctlr, Read);
	wrcnt = nonzerocnt(ctlr, Write);
	return seprint(p, e, "sd%c nvme regs %#p irq %d: max q lens: rd %d/%d "
		"wr %d/%d; avg µs: rd %lld wr %lld\n", sdev->idno, ctlr->port,
		ctlr->irq, ctlr->maxqlen[Read], ctlr->sqlen,
		ctlr->maxqlen[Write], ctlr->sqlen,
		ctlr->totus[Read] / rdcnt, ctlr->totus[Write] / wrcnt);
}

static void
reset(Regs *regs)
{
	if (regs->cc & Enable) {
		if (awaitbitpat(&regs->csts, Rdy, Rdy) < 0)
			print("nvme reset timed out awaiting ready\n");
		regs->cc &= ~Enable;
		coherence();
	}
	/* else may have previously cleared Enable & be waiting for not ready */
	if (awaitbitpat(&regs->csts, Rdy, 0) < 0)
		print("nvme reset timed out awaiting not ready\n");
}

static void
nvmedrive(SDunit *unit)
{
	uchar *p;
	Ctlr *ctlr;

	unit->sense[0] = 0x70;
	unit->sense[7] = sizeof(unit->sense)-7;

	memset(unit->inquiry, 0, sizeof unit->inquiry);
	unit->inquiry[0] = SDperdisk;
	unit->inquiry[2] = 2;
	unit->inquiry[3] = 2;
	unit->inquiry[4] = sizeof unit->inquiry - 4;
	p = &unit->inquiry[8];
	ctlr = unit->dev->ctlr;
	/* model is smaller than unit->inquiry-8 */
	strncpy((char *)p, ctlr->model, sizeof ctlr->model);

	unit->secsize = ctlr->secsize;
	unit->sectors = ctlr->sectors;
	print("sd%C%d: nvme %,lld sectors: %s fw %s serial %s\n",
		unit->dev->idno, unit->subno, unit->sectors,
		ctlr->model, ctlr->fw, ctlr->serial);
}

static void
pickpgsz(Ctlr *ctlr)
{
	ulong minpgsz, maxpgsz;

	minpgsz = 1 << (12 + ((ctlr->regs->cap >> 48) & MASK(4)));
	maxpgsz = 1 << (12 + ((ctlr->regs->cap >> 52) & MASK(4)));
	ctlr->minpgsz = minpgsz;		/* for Ctlrid->mdts */
	ctlr->pgsz = MIN(Startpgsz, maxpgsz);
	if (ctlr->pgsz < minpgsz)
		ctlr->pgsz = minpgsz;
	if (Sdalign >= 4*KB && ctlr->pgsz > Sdalign)
		ctlr->pgsz = Sdalign;
	if (ctlr->pgsz < 4*KB)			/* sanity */
		ctlr->pgsz = 4*KB;
}

static void
pickqlens(Ctlr *ctlr)
{
	ulong mqes;

	mqes = (ctlr->regs->cap & MASK(16)) + 1;  /* max i/o [sc] q len */
	ctlr->sqlen = MIN(mqes, Qlen);
	ctlr->cqlen = MIN(mqes, Cqlen);
}

static SDev*
nvmeprobe(Pcidev *p)
{
	int logstride;
	uintptr port;
	Ctlr *ctlr;
	Regs *regs;
	SDev *sdev;
	static int count;

	assert(p->mem[1].bar == 0);	/* upper 32 bits of 64-bit addr */
	port = p->mem[0].bar & ~0x0f;
	regs = vmap(port, p->mem[0].size);
	if(regs == nil){
		print("nvmeprobe: phys address %#p in use did=%#ux\n",
			port, p->did);
		return nil;
	}

	if ((ctlr = malloc(sizeof(Ctlr))) == nil ||
	    (sdev = malloc(sizeof(SDev))) == nil) {
		free(ctlr);
		vunmap(regs, p->mem[0].size);
		return nil;
	}
	ctlr->regs = regs;
	ctlr->port = port;
	ctlr->irq = p->intl;
	/*
	 * Attempt to hard-reset the board.
	 */
	reset(regs);
	logstride = ((regs->cap >> 32) & MASK(4));	/* doorbell stride */
	if (logstride != 0)
		panic("nvmeprobe: doorbell stride must be 0 (for now), not %d",
			logstride);
	ctlr->stride = 1 << (2 + logstride);	/* 2^(2+logstride) */
	if (0 && regs->cap & Nssrs) {		/* nvm subsys reset avail.? */
		regs->cc |= Nssro;		/* clear Nssro by setting it */
		regs->nssrc = 'N'<<24 | 'V'<<16 | 'M'<<8 | 'e';
		if (awaitbitpat(&regs->csts, Nssro, Nssro) < 0)
			print("nvme subsys reset timed out awaiting Nssro\n");
	}

	pickpgsz(ctlr);
	pickqlens(ctlr);
	/* we haven't chosen a namespace yet, so we don't know sector size */

	sdev->ifc = &sdnvmeifc;
	sdev->idno = 'n';	/* actually assigned in sdadddevs() */
	sdev->nunit = NCtlrdrv;	/* max. drives (can be number found) */
	sdev->ctlr = ctlr;		/* cross link */
	ctlr->sdev = sdev;

	/*
	 * we (pnp) don't have a `spec' argument, so
	 * we'll assume that sdn0 goes to the first nvme host
	 * adapter found, sdo0 to the next, etc.
	 */
	print("#S/sd%c: nvme %ld.%ld.%ld: irq %d regs %#p page size %d\n",
		sdev->idno + count++, regs->vs>>16, (regs->vs>>8) & MASK(8),
		regs->vs & MASK(8), ctlr->irq, ctlr->port, ctlr->pgsz);

	/* would probe for drives here if there could be more than one. */
	/* upon return, this many sdev->units will be allocated. */
	sdev->nunit = 1;
	return sdev;
}

static void
sdevadd(SDev *sdev, SDev **head, SDev **tail)
{
	if(*head != nil)
		(*tail)->next = sdev;
	else
		*head = sdev;
	*tail = sdev;
}

/*
 * find all nvme controllers
 */
static SDev*
nvmepnp(void)
{
	Ctlr *ctlr;
	Pcidev *p;
	SDev *sdev, *head, *tail;

	p = nil;
	head = tail = nil;
	while(p = pcimatch(p, 0, 0)){
		/* ccrp 2 is NVME */
		if(p->ccrb != Pcibcstore || p->ccru != Pciscnvm || p->ccrp != 2)
			continue;
		if((sdev = nvmeprobe(p)) == nil)
			continue;
		ctlr = sdev->ctlr;
		ctlr->pcidev = p;
		sdevadd(sdev, &head, &tail);
		if (nctlrs >= NCtlr)
			print("too many nvme controllers\n");
		else
			ctlrs[nctlrs++] = ctlr;
	}
	return head;
}

static void
allocqpair(Ctlr *ctlr, Qpair *qp)
{
	assert(ctlr->pgsz);
	qp->sqlen = ctlr->sqlen;
	qp->cqlen = ctlr->cqlen;
	qp->q    = mallocalign(qp->sqlen * sizeof *qp->q,    ctlr->pgsz, 0, 0);
	qp->cmpl = mallocalign(qp->cqlen * sizeof *qp->cmpl, ctlr->pgsz, 0, 0);
	if (qp->q == nil || qp->cmpl == nil)
		panic("nvmectlrenable: out of memory for queues");
	qp->writelast = Read;
}

static void
configure(Ctlr *ctlr, Qpair *qpadm)
{
	Regs *regs = ctlr->regs;

	regs->aqa = (ctlr->cqlen - 1)<<16 | (ctlr->sqlen - 1);
	regs->asq = PCIWADDR((void *)qpadm->q);
	regs->acq = PCIWADDR((void *)qpadm->cmpl);
	regs->cc = log2(sizeof(Completion))<<20 | log2(sizeof(Cmd))<<16 |
		(log2(ctlr->pgsz)-12) << 7 | Cssnvm;
	coherence();
}

static void
enable(Regs *regs)
{
	if (!(regs->cc & Enable)) {
		if (awaitbitpat(&regs->csts, Rdy, 0) < 0)
			print("nvme enable timed out awaiting not ready\n");
		regs->cc |= Enable;
		coherence();
	}
	/* else may have previously set Enable & be waiting for ready */
	if (awaitbitpat(&regs->csts, Rdy, Rdy) < 0)
		print("nvme enable timed out awaiting ready\n");
}

/*
 * ns numbers start at 1 and are densely-packed.
 * pick one with 512-byte blocks, return preferred lbafmt via *lbafmtp.
 */
static int
bestns(Ctlr *ctlr, int nns, Nsid *nsid, int *lbafmtp)
{
	int i, ns, second, nssecond, lbasize;
	Lbafmt *lbafmt;

	second = 0;
	nssecond = 0;
	*lbafmtp = 0;
	for (ns = 1; ns <= nns; ns++) {
		if (nvmeadmissue(ctlr, Admid, ns, nsid) != 0)
			panic("nvmectlrenable: Admid(%d) failed", ns);
		for (i = 0; i < nelem(nsid->lbafmt); i++) {
			lbafmt = &nsid->lbafmt[i];
			if (lbafmt->lglbasize == 0)	/* end lbafmt list? */
				break;
			lbasize = 1 << lbafmt->lglbasize;
			if (Debugns)
				print("nvme ns %d: lba %d mdsize %d perf %d\n",
					ns, lbasize, lbafmt->mdsize,
					lbafmt->relperf & 3);
			if (lbafmt->mdsize == 0 && lbasize == Minsect) {
				*lbafmtp = i;
				return ns;
			}
			/* settle for 4k if that's all there is */
			if (lbafmt->mdsize == 0 && lbasize == 4096) {
				second = i;
				nssecond = ns;
			}
		}
	}
	if (nssecond)
		*lbafmtp = second;
	return second;
}

/*
 * copy id string from controller, trim trailing blanks, downcase.
 * assumes src is unterminated and dest is at least one byte larger.
 */
static void
idcopy(char *dest, char *src, int size)
{
	char *p, *pend;

	memmove(dest, src, size);
	pend = &dest[size];
	*pend-- = '\0';
	for (p = pend; p > dest && *p == ' '; p--)
		*p = '\0';
	for (p = dest; p <= pend && *p != '\0'; p++)
		*p = tolower(*p);
}

static void
nvmeintron(SDev *sdev)
{
	char name[32];
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	snprint(name, sizeof(name), "sd%c (%s)", sdev->idno, sdev->ifc->name);
	enableintr(ctlr, nvmeinterrupt, ctlr, name);
	ctlr->regs->intmset = ~0;	/* mask all interrupt sources */
}

static void
zeroqhdtls(Qpair *qp)
{
	qp->cidx.hd = qp->qidx.tl = 0;
	qp->cidx.tl = qp->qidx.hd = 0;	/* paranoia */
	coherence();
}

static int
nvmectlrenable(Ctlr* ctlr)
{
	int i, nns, gotns;
	char *idpage;
	Ctlrid *ctlrid;
	Lbafmt *lbafmt;
	Nsid *nsid;
	Qpair *qpadm, *qpio;
	Regs *regs = ctlr->regs;
	SDev *sdev = ctlr->sdev;

	/* we need at least one admin queue and one i/o queue */
	qpadm = &ctlr->qpair[Qadmin];
	allocqpair(ctlr, qpadm);
	qpio = &ctlr->qpair[Qio];
	allocqpair(ctlr, qpio);

	assert(!(regs->cc & Enable));
	configure(ctlr, qpadm);	/* must do this while ctlr is disabled */
	enable(regs);
	zeroqhdtls(qpadm);		/* paranoia */

	regs->intmset = ~0;		/* mask all interrupt sources */
	nvmeintron(sdev);

	idpage = mallocalign(BY2PG, ctlr->pgsz, 0, 0);
	if (idpage == nil)
		panic("nvmectlrenable: out of memory");
	if (nvmeadmissue(ctlr, Admid, Nsall, idpage) != 0)
		panic("nvmectlrenable: Admid(Nsall) failed");
	ctlrid = (Ctlrid *)idpage;
	nns = ctlrid->nns;

	/* smuggle hw id strings into ctlr for later printing */
	idcopy(ctlr->serial, ctlrid->serial, sizeof ctlrid->serial);
	idcopy(ctlr->model, ctlrid->model, sizeof ctlrid->model);
	idcopy(ctlr->fw, ctlrid->fw, sizeof ctlrid->fw);
	if (ctlrid->mdts)
		ctlr->mdts = 1 << ctlrid->mdts;
//	iprint("nvme: max xfr size %d\n", ctlr->mdts * ctlr->minpgsz);

	/*
	 * create first i/o queue with admin queue cmds.
	 * completion queue must be created first.
	 */
	if (nvmeadmissue(ctlr, Admmkiocq, Nsunused, qpio->cmpl) != 0)
		panic("nvmectlrenable: Admmkiocq failed");
	if (nvmeadmissue(ctlr, Admmkiosq, Nsunused, qpio->q) != 0)
		panic("nvmectlrenable: Admmkiosq failed");
	zeroqhdtls(qpio);		/* paranoia */

	/* find a suitable namespace */
	nsid = (Nsid *)idpage;
	gotns = bestns(ctlr, nns, nsid, &i);	/* fills in nsid page */
	if (gotns == 0)
		panic("nvmectlrenable: no suitable namespace found");
	lbafmt = &nsid->lbafmt[i];
	ctlr->secsize = 1 << lbafmt->lglbasize;	/* remember for SDunit */
	ctlr->sectors = nsid->cap;		/* remember for SDunit */
	ctlr->ns = gotns;
	free(idpage);
	if (Debugns)
		print("nvme best ns: %d: sectors %,lld of %d bytes\n",
			ctlr->ns, ctlr->sectors, ctlr->secsize);
	return 1;
}

static void
freeqpair(Qpair *qp)
{
	free(qp->q);
	free(qp->cmpl);
	qp->q = nil;
	qp->cmpl = nil;
}

static void
ckstuck(void)
{
	int i;
	static int whined;

	for (i = 0; i < nctlrs; i++)
		nvmeinterrupt(nil, ctlrs[i]);
	if (iosttck && sys->ticks - iosttck > 5*HZ && ++whined < 5)
		iprint("nvme: stuck for 5 s.\n");
}

/*
 * activate a single nvme controller, sdev.
 * upon return, sdev->nunit SDunits will be allocated.
 */
static int
nvmeenable(SDev* sdev)
{
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	if(ctlr->qpair[Qadmin].q)
		return 0;

	pcisetbme(ctlr->pcidev);
	if(!nvmectlrenable(ctlr)) {
		freeqpair(&ctlr->qpair[Qadmin]);
		freeqpair(&ctlr->qpair[Qio]);
		return 0;
	}

	/* watch for hardware bugs */
	lock(&clocklck);
	if (!clockrunning) {
		addclock0link(ckstuck, 1000);
		clockrunning = 1;
	}
	unlock(&clocklck);
	return 1;
}

static void
nvmeintroff(SDev *sdev)
{
	char name[32];
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	ctlr->regs->intmset = ~0;		/* mask all interrupt sources */

	snprint(name, sizeof(name), "sd%c (%s)", sdev->idno, sdev->ifc->name);
	disableintr(ctlr, nvmeinterrupt, ctlr, name);
}

/*
 * returns when all in-flight transfers are done.
 * call with shutlock & issuelock held.
 */
static void
waitnoxfrs(Ctlr *ctlr)
{
	int i;

	for (i = 1000; i-- > 0 && ctlr->inflight > 0; ) {
		iunlock(&ctlr->shutlock);
		iunlock(&ctlr->issuelock);
		delay(1);
		ilock(&ctlr->issuelock);
		ilock(&ctlr->shutlock);
	}
	if (i <= 0)
		iprint("sdnvme: %d transfers still in flight after 1 s.\n",
			ctlr->inflight);
}

static int
nvmedisable(SDev* sdev)			/* disable interrupts for this sdev */
{
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	if (ctlr == nil)
		return 1;
	nvmeissue(ctlr, &ctlr->qpair[Qio], nil, Cmdflush, Nsall, nil, 0);

	ilock(&ctlr->issuelock);
	ilock(&ctlr->shutlock);

	waitnoxfrs(ctlr);
	nvmeintroff(sdev);
	pciclrbme(ctlr->pcidev);

	iunlock(&ctlr->shutlock);
	iunlock(&ctlr->issuelock);
	return 1;
}

static void
nvmeclear(SDev* sdev)			/* clear the interface for this sdev */
{
	Ctlr *ctlr;

	ctlr = sdev->ctlr;
	if (ctlr == nil)
		return;
	ilock(&ctlr->issuelock);
	ilock(&ctlr->shutlock);
	if (ctlr->regs) {
		waitnoxfrs(ctlr);
		reset(ctlr->regs);	/* ctlrs and drives are one-to-one */
	}
	iunlock(&ctlr->shutlock);
	iunlock(&ctlr->issuelock);
}

/*
 * see if a particular drive exists.
 * must not set unit->sectors here, but rather in nvmeonline.
 */
static int
nvmeverify(SDunit *unit)
{
	if (unit->subno != 0)
		return 0;
	return 1;
}

/*
 * initialise a drive known to exist.
 * returns boolean for success.
 */
static int
nvmeonline(SDunit *unit)
{
	int r;

	if (unit->subno != 0)		/* not me? */
		return 0;
	if (unit->sectors)		/* already inited? */
		return 1;
	r = scsionline(unit);
	if(r == 0)
		return r;
	nvmedrive(unit);
	/*
	 * could hang around until disks are spun up and thus available as
	 * nvram, dos file systems, etc.  you wouldn't expect it, but
	 * the intel 330 sata ssd takes a while to `spin up'.
	 */
	return 1;			/* drive ready */
}

SDifc sdnvmeifc = {
	"nvme",				/* name */

	nvmepnp,			/* pnp */
	nil,				/* legacy */
	nvmeenable,			/* enable */
	nvmedisable,			/* disable */

	nvmeverify,			/* verify */
	nvmeonline,			/* online */
	nvmerio,			/* rio */
	nvmerctl,			/* rctl */
	nil,				/* wctl */

	scsibio,			/* bio */
	nil,				/* probe */
	nvmeclear,			/* clear */
	nvmertopctl,			/* rtopctl */
	nil,				/* wtopctl */
};
