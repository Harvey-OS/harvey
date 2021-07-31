#include	"all.h"
#include	"io.h"


typedef struct Jag Jag;
typedef	struct Cmd Cmd;
typedef struct Ctlr Ctlr;

enum
{
	MaxJag		= 4,
};

struct	Cmd
{
	int 	flag;
	long	active;		/* toytime chan went active */
	int	retstat;
	QLock	ulock;		/* to get at each queue */
	Rendez	ren;
};

static
struct	Ctlr
{
	Vmedevice* vme;
	int	nqueue;
	QLock;			/* to get at controller */
	Cmd	cmd[NDRIVE];
	Drive	drive[NDRIVE];
} ctlr[MaxJag];

typedef	struct	Mcsb	Mcsb;
typedef	struct	Cqe	Cqe;
typedef	struct	Cib	Cib;
typedef	struct	Crb	Crb;
typedef	struct	Iopb	Iopb;

#define	NCMD	2				/* makes it easy to count mod 1 */
#define	IOTYPE1	((1<<10)|(2<<8)|(0x0b))		/* block mode, 32-bit, A32 block */
#define	IOTYPE2	((0<<10)|(2<<8)|(0x09))		/* normal mode, 32-bit, A32 nonpriv data */
#define	IOTYPE3	((0<<10)|(1<<8)|(0x09))		/* normal mode, 16-bit, A32 nonpriv data */
#define	IOTYPE	IOTYPE1
#define	GREG	0x40				/* its all greg's fault */

struct	Mcsb
{
	ushort	msr;
		#define	CNA	(1<<0)
		#define	BOK	(1<<1)
		#define	QFC	(1<<2)
	ushort	mcr;
		#define	SQM	(1<<0)
		#define	FLQR	(1<<2)
		#define	FLQ	(1<<11)
		#define	RES	(1<<12)
		#define	SFEN	(1<<13)
	ushort	iqar;
		#define	IQHE	(1<<14)		/* interrupt on q half empty */
		#define	IQEA	(1<<15)		/* interrupt on q entry available */
	ushort	qhp;
	ushort	twqr;
	ushort	pad1[3];
};

struct	Cqe
{
	ushort	qecr;
		#define	GO	(1<<0)
		#define	AA	(1<<1)
		#define	HPC	(1<<2)
	ushort	iopba;
	ushort	tag[2];
	ushort	qnumb;
	ushort	pad2;
};

struct	Iopb
{
	ushort	command;
	ushort	options;
		#define	IE	(1<<0)
	ushort	retstat;
	ushort	pad4;
	ushort	intvec;
	ushort	intlev;
	ushort	pad5;
	ushort	iotype;
	ushort	addr[2];
	ushort	size[2];
	ushort	pad6[2];
		#define	NIOB	(18*2)				/* size to here */
	union
	{
		/* for initialize work queue */
		struct
		{
			ushort	wqnumb;
			ushort	wqoptions;
				#define	AE	(1<<0)
				#define	FZE	(1<<2)
				#define	PE	(1<<3)
				#define	IWQ	(1<<15)
			ushort	wqslots;
			ushort	wqpriority;
				#define	WIOB	(NIOB+4*2)	/* size to here */
		};
		/* for scsi operations */
		struct
		{
			ushort	pad7;
			ushort	unit;
				#define	SIOB	(NIOB+2*2)	/* size to here */
			uchar	param[64-SIOB];			/* total size is 64 */
		};
	};
};

struct	Crb
{
	ushort	crsw;
		#define	CRBV	(1<<0)
		#define	CC	(1<<1)
		#define	ER	(1<<2)
		#define	EX	(1<<3)
		#define	AQ	(1<<4)
		#define	QMS	(1<<5)
		#define	CQA	(1<<6)
	ushort	pad8;
	ushort	tag[2];
	ushort	qnumb;
	ushort	pad9;
	Iopb;
};

struct	Cib
{
	ushort	nqueue;
	ushort	dmacount;
	ushort	normvec;
	ushort	errvec;
	ushort	priscsi;
	ushort	secscsi;
	ushort	crba;
	ushort	scsitim1[2];
	ushort	scsitim2[2];
	ushort	vmetim[2];
	ushort	pada[3];
};

struct	Jag
{
	Mcsb;
	Cqe	cmd[NCMD];			/* command queue, zeroth is mqe */
	Iopb	iopb[NCMD];			/* io parameter blocks 1:1 with cqe's */
	Cib	cib;
	char	padb[2048
			-sizeof(Mcsb)
			-NCMD*sizeof(Cqe)
			-NCMD*sizeof(Iopb)
			-sizeof(Cib)
			-sizeof(Crb)
			-120];
	Crb;
	uchar	css[120];			/* controller space */
};

static	void	cmd_statj(int, char *[]);

static
int
jagstat(Jag *jag)
{
	int c, i, s;

	c = 0;	/* set */
	for(i=100000000; i>0; i--) {
		c = jag->crsw;
		if(c & CRBV)
			break;
	}
	if(i == 0)
		print("jagstat timeout\n");
	s = jag->retstat;
	if((c & (CRBV|CC|ER)) != (CRBV|CC)) {
		if(s == 0)
			s = GREG;
		if(c & QMS)
			s = 0;
		if(0 && s)
			print("crsw = %.4ux; error = %.4ux\n", c, s);
	}
	jag->crsw = 0;
	return s;
}

static
int
jagcmd(Jag *jag)
{
	Cqe *cq;
	int c, i;

	cq = &jag->cmd[0];

	cq->qecr = GO;
	c = 0; /* set */
	for(i=0; i<1000000; i++) {
		c = cq->qecr;
		if(!(c & GO))
			break;
	}
	if(c & GO)
		print("go didnt fall: qcer = %.4ux\n", c);
	c = jagstat(jag);
	return c;
}

static
void
hclear(void *ap, int n)
{
	ushort *p;

	p = ap;
	while(n > 0) {
		*p++ = 0;
		n -= sizeof(ushort);
	}
}

Drive*
scsidrive(Device d)
{
	Ctlr *ctl;

	if(d.ctrl >= MaxJag || d.unit >= NDRIVE)
		return 0;
	ctl = &ctlr[d.ctrl];
	if(ctl->vme == 0)
		return 0;
	ctl->drive[d.unit].dev = d;
	return &ctl->drive[d.unit];
}

int
jaginit(Vmedevice *vp)
{
	Jag *jag;
	Cqe *cq;
	Cib *cb;
	Iopb *ib;
	Cmd *cp;
	int i, j, c, reset;
	Ctlr *ctl;

	print("jaginit %d\n", vp->ctlr);
	if(vp->ctlr >= MaxJag)
		return -1;
	ctl = &ctlr[vp->ctlr];
	if(ctl->vme)
		return -1;
	jag = vp->address;

	/*
	 * does the board exist?
	if(probe(&jag->msr, sizeof(jag->msr)))
		return -1;
	 */
	vp->private = ctl;
	reset = 0;

loop:
	memset(ctl, 0, sizeof(Ctlr));
	ctl->vme = vp;
	qlock(ctl);
	ctl->name = "jagc";
	qunlock(ctl);
	for(i=0; i<NDRIVE; i++) {
		cp = &ctl->cmd[i];
		lock(&cp->ren);
		unlock(&cp->ren);
		qlock(&cp->ulock);
		cp->ulock.name = "jagu";
		qunlock(&cp->ulock);
	}

	/*
	 * reset
	 */
	if(reset) {
		print("	reset\n");
		jag->mcr = RES;
		for(i=0; i<50*200; i++)
			;			/* reset for >50us */
		jag->msr = 0;
	}
	jag->mcr = 0;

	c = 0;	/* set */
	for(i=0; i<10000000; i++) {
		c = jag->msr & (BOK|CNA);
		if(c == BOK)
			break;
	}
	if(c != BOK) {
		print("	jaguar BOK msr = %.4ux\n", jag->msr);
		goto again;
	}

	cq = &jag->cmd[NCMD];
	ib = &jag->iopb[NCMD];
	for(i=0; i<NCMD; i++) {
		cq--;
		ib--;
		hclear(cq, sizeof(Cqe));
		cq->iopba = (char*)ib - (char*)jag;
	}

	cb = &jag->cib;

	/*
	 * initialize
	 */
	print("	init controller\n");
	hclear(ib, sizeof(Iopb));
	ib->command = 0x41;				/* init controller */
	ib->iotype = (0<<10) | (3<<8) | 0;
	ib->addr[1] = (char*)cb - (char*)jag;
	ib->size[1] = sizeof(Cib);

	hclear(cb, sizeof(Cib));
	cb->nqueue = NCMD-1;
	cb->priscsi = (0<<3)|7;				/* jag gets scsi id 7 */
	cb->secscsi = (0<<3)|7;				/* on both busses */
	cb->scsitim1[1] = 10;				/* 10 ms to connect */
	cb->scsitim2[1] = 100;				/* 3.2s to reconnect */
	cb->crba = (char*)&jag->crsw - (char*)jag;
	cb->normvec = (vp->irq<<8) | vp->vector;
	cb->errvec = (vp->irq<<8) | vp->vector;

	cq->qnumb = (NIOB<<8) | 0;			/* size and queue num */
	if(jagcmd(jag)) {
		print("	jag initialize controller\n");
		goto again;
	}

	/*
	 * scsi status command
	 */
	for(i=0; i<NDRIVE; i++) {
		if((i & 7) == 7) {
			ctl->drive[i].status = Dself;	/* sid is controller */
			continue;
		}
		hclear(ib, sizeof(Iopb));
		ib->command = 0x20;			/* scsi pass thru */
		ib->iotype = IOTYPE;
		ib->unit = ((i>>3)<<6)|(0<<3)|(i&7);	/* bus,lun,sid */
		ib->param[0] = 0x00;			/* status scsi cmd */
		cq->qnumb = ((SIOB + 6)<<8) | 0;	/* size and queue num */
		c = jagcmd(jag);

		ctl->drive[i].status = Dready;
		if(c == 0)
			continue;
		if((c & 0xff) == 0) {			/* drive error, jag ok */
			for(j=0; j<10; j++) {
				c = jagcmd(jag);	/* try again to clear sense */
				if(c == 0)
					break;
			}
			if(c != 0) {
				print("jag drive %d status 0x%x\n", i, c);
				ctl->drive[i].status = Dnready;
			}
			continue;
		}
		ctl->drive[i].status = Dunavail;	/* jag error, probably timeout */
	}

	/*
	 * initialize work queues
	 */
	print("	init queues\n");
	ctl->nqueue = 0;
	for(i=0; i<NDRIVE; i++) {
		if(ctl->drive[i].status == Dself)
			continue;
		ctl->nqueue++;
		ctl->drive[i].qnum = ctl->nqueue;
		hclear(ib, sizeof(Iopb));
		ib->command = 0x42;			/* initialize work queue */
		ib->wqnumb = ctl->nqueue;		/* queue number */
		ib->wqoptions = IWQ|PE;			/* really do it, honest */
		ib->wqslots = 10;			/* number of slots */
		ib->wqpriority = 14;			/* priority */
		cq->qnumb = (WIOB<<8) | 0;		/* size and queue num */
		if(jagcmd(jag)) {
			print("	jag queue: %.2o\n", i);
			goto again;
		}
	}
	jag->mcr = SQM;
	if(jagstat(jag)) {
		print("	start queue mode\n");
		goto again;
	}
	if(vp->ctlr == 0) {
		cmd_install("statj", "-- jaguar stats", cmd_statj);
	}
	return 0;

again:
	/*
	 * make 2 passes, first without resetting
	 * the jaguar cpu. this will prevent a
	 * scsi init which makes the sony worm
	 * go cata for 90 sec.
	 */
	if(reset)
		panic("jag init");
	reset = 1;
	goto loop;
}

void
jagintr(Vmedevice *vp)
{
	Jag *jag;
	Cmd *cp;
	int s, t;

	jag = vp->address;
	s = jag->retstat;
	t = jag->tag[0];
	if(jag->crsw != 3 || s != 0)
		print("bad jag intr %.4ux %.4ux\n", jag->crsw, s);
	jag->crsw = 0;

	if(t < 0 || t >= NDRIVE) {
		print("bad jag tag %d\n", t);
		t = 0;
	}
	cp = &(((Ctlr*)vp->private)->cmd[t]);
	if(cp->flag)
		print("jag intr flag set\n");
	cp->retstat = s;
	cp->flag = 1;
	wakeup(&cp->ren);
}

void
pokewcp(void)
{
	int c, u;
	long t, a;
	Ctlr *ctl;
	Cmd *cp;

	t = toytime() - SECOND(60);
	for(c=0; c<MaxJag; c++) {
		ctl = &ctlr[c];
		if(ctl->vme == 0)
			continue;
		for(u=0; u<NDRIVE; u++) {
			cp = &ctl->cmd[u];
			a = cp->active;
			if(a != 0 && a < t) {
				cp->active = toytime();		/* small race */
				if(cp->flag)
					print("jaguar timeout and flag set\n");
				else
					print("jaguar timeout\n");
				cp->retstat = GREG;
				cp->flag = 1;
				wakeup(&cp->ren);
			}
		}
	}
}

static int
jflag(void *v)
{
	return ((struct Cmd*)v)->flag;
}

int
scsiio(Device d, int rw, uchar *param, int nparam, void *addr, int size)
{
	Jag *jag;
	Cqe *cq;
	Iopb *ib;
	Drive *dr;
	Cmd *cp;
	ulong a, syspa;
	int i, s;
	Vmedevice *vp;
	Ctlr *ctl;
	static int uniq;

	if(d.ctrl >= MaxJag)
		return GREG;
	ctl = &ctlr[d.ctrl];
	vp = ctl->vme;
	if(vp == 0)
		return GREG;
	cp = &ctl->cmd[d.unit];

	qlock(&cp->ulock);
	qlock(ctl);

	jag = vp->address;
	ib = &jag->iopb[1];

	hclear(ib, sizeof(Iopb));
	ib->command = 0x20;				/* scsi */
	ib->options = (rw<<8) | IE;
	ib->intvec = (vp->vector<<8) | vp->vector;
	ib->intlev = vp->irq;
	ib->iotype = IOTYPE;

	syspa = vme2sysmap(vp->bus, (ulong)addr, size);
	wbackcache(addr, size);
	invalcache(addr, size);

	ib->addr[0] = syspa >> 16;
	ib->addr[1] = syspa;
	a = size;
	ib->size[0] = a >> 16;
	ib->size[1] = a;

	ib->unit = ((d.unit>>3)<<6) | ((param[1]>>5)<<3) | (d.unit&7);
	for(i=0; i<nparam; i++)
		ib->param[i] = param[i];

	cp->flag = 0;
	cp->active = toytime();
	cq = &jag->cmd[1];
	cq->qnumb = ((SIOB + nparam)<<8) | ctl->drive[d.unit].qnum;
	cq->tag[0] = d.unit;
	cq->qecr = GO;

	dr = &ctl->drive[d.unit];
	if(dr->fflag == 0) {
		dofilter(&dr->rate);
		dofilter(&dr->work);
		dr->fflag = 1;
	}
	dr->work.count++;
	dr->rate.count += size;

	while(cq->qecr & GO)
		;
	qunlock(ctl);

	while(!jflag(cp))
		sleep(&cp->ren, jflag, cp);

	vme2sysfree(vp->bus, syspa, size);
	if(rw == SCSIread)
		invalcache(addr, size);

	s = cp->retstat;
	cp->active = 0;
	qunlock(&cp->ulock);
	return s;
}

static
void
stats(Ctlr *ctl)
{
	Drive *dr;
	int i, s;

	print("jaguar stats %d\n", ctlr - ctl);
	for(i=0; i<NDRIVE; i++) {
		dr = &ctl->drive[i];
		if(dr->fflag == 0)
			continue;
		s = dr->status;
		if(s != Dready)
			print("	drive %.2o (%d)\n", i, s);
		else
			print("	drive %.2o = %10ld\n", i, dr->max);
		print("		work = %F\n", (Filta){&dr->work, 1});
		print("		rate = %F tBps\n", (Filta){&dr->rate, 1000});
	}
}

static
void
cmd_statj(int argc, char *argv[])
{
	Ctlr *ctl;
	int i;

	USED(argc);
	USED(argv);

	for(i=0; i<MaxJag; i++) {
		ctl = &ctlr[i];
		if(ctl->vme)
			stats(ctl);
	}
}
