#include "all.h"
#include "io.h"
#include "mem.h"

#define DEBUG	0
#define DPRINT	if(DEBUG)print
#define IDEBUG	0
#define IDPRINT if(IDEBUG)print

typedef	struct Drive		Drive;
typedef	struct Ident		Ident;
typedef	struct Controller	Controller;

enum
{
	/* ports */
	Pbase0=		0x1F0,	/* primary */
	Pbase1=		0x170,	/* secondary */
	Pbase2=		0x1E8,	/* tertiary */
	Pbase3=		0x168,	/* quaternary */
	Pdata=		0,	/* data port (16 bits) */
	Perror=		1,	/* error port (read) */
	 Eabort=	(1<<2),
	Pfeature=	1,	/* buffer mode port (write) */
	Pcount=		2,	/* sector count port */
	Psector=	3,	/* sector number port */
	Pcyllsb=	4,	/* least significant byte cylinder # */
	Pcylmsb=	5,	/* most significant byte cylinder # */
	Pdh=		6,	/* drive/head port */
	 DHmagic=	0xA0,
	 DHslave=	0x10,
	Pstatus=	7,	/* status port (read) */
	 Sbusy=		 (1<<7),
	 Sready=	 (1<<6),
	 Sdrq=		 (1<<3),
	 Serr=		 (1<<0),
	Pcmd=		7,	/* cmd port (write) */

	/* commands */
	Crecal=		0x10,
	Cread=		0x20,
	Cwrite=		0x30,
	Cident=		0xEC,
	Cident2=	0xFF,	/* pseudo command for post Cident interrupt */
	Csetbuf=	0xEF,

	Cpktcmd=	0xA0,
	Cidentd=	0xA1,
	Ctur=		0x00,
	Creqsense=	0x03,
	Ccapacity=	0x25,
	Cread2=		0x28,
	Cwrite2=	0x2A,

	ATAtimo=	6000,		/* ms to wait for things to complete */
	ATAPItimo=	10000,

	NCtlr=		4,
	NDrive=		NCtlr*2,

	Maxloop=	1000000,
};

/*
 *  ident sector from drive.  this is from ANSI X3.221-1994
 */
struct Ident
{
	ushort	config;		/* general configuration info */
	ushort	cyls;		/* # of cylinders (default) */
	ushort	reserved0;
	ushort	heads;		/* # of heads (default) */
	ushort	b2t;		/* unformatted bytes/track */
	ushort	b2s;		/* unformated bytes/sector */
	ushort	s2t;		/* sectors/track (default) */
	ushort	reserved1[3];
/* 10 */
	ushort	serial[10];	/* serial number */
	ushort	type;		/* buffer type */
	ushort	bsize;		/* buffer size/512 */
	ushort	ecc;		/* ecc bytes returned by read long */
	ushort	firm[4];	/* firmware revision */
	ushort	model[20];	/* model number */
/* 47 */
	ushort	s2i;		/* number of sectors/interrupt */
	ushort	dwtf;		/* double word transfer flag */
	ushort	capabilities;
	ushort	reserved2;
	ushort	piomode;
	ushort	dmamode;
	ushort	cvalid;		/* (cvald&1) if next 4 words are valid */
	ushort	ccyls;		/* current # cylinders */
	ushort	cheads;		/* current # heads */
	ushort	cs2t;		/* current sectors/track */
	ushort	ccap[2];	/* current capacity in sectors */
	ushort	cs2i;		/* current number of sectors/interrupt */
/* 60 */
	ushort	lbasecs[2];	/* # LBA user addressable sectors */
	ushort	dmasingle;
	ushort	dmadouble;
/* 64 */
	ushort	reserved3[64];
	ushort	vendor[32];	/* vendor specific */
	ushort	reserved4[96];
};

enum {
	Maxxfer		= 16*1024,	/* maximum transfer size/cmd */
	Npart		= 8+2,		/* 8 sub partitions, disk, and partition */
};

typedef struct {
	ulong	start;
	ulong	end;
	char	name[NAMELEN+1];
} Partition;

typedef struct {
	int	online;
	int	npart;		/* number of real partitions */
	Partition p[Npart];
	ulong	offset;
	Partition *current;	/* current partition */

	ulong	cap;		/* total sectors */
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* heads/cyl */
	long	cyl;		/* cylinders/drive */

	char	lba;		/* true if drive has logical block addressing */
	char	multi;		/* non-zero if drive does multiple block xfers */
} Disc;

/*
 *  an ata drive
 */
struct Drive
{
	Controller *cp;
	uchar	driveno;
	uchar	dh;
	uchar	atapi;		/* ATAPI */
	uchar	drqintr;	/* ATAPI */
	ulong	vers;		/* ATAPI */

	int	partok;

	Disc;
};

/*
 *  a controller for 2 drives
 */
struct Controller
{
	int	pbase;		/* base port */
	uchar	ctlrno;

	/*
	 *  current operation
	 */
	int	cmd;		/* current command */
	uchar	cmdblk[12];	/* ATAPI */
	int	len;		/* ATAPI */
	int	count;		/* ATAPI */
	uchar	lastcmd;	/* debugging info */
	uchar	*buf;		/* xfer buffer */
	int	tcyl;		/* target cylinder */
	int	thead;		/* target head */
	int	tsec;		/* target sector */
	int	tbyte;		/* target byte */
	int	nsecs;		/* length of transfer (sectors) */
	int	sofar;		/* sectors transferred so far */
	int	status;
	int	error;
	Drive	*dp;		/* drive being accessed */
};

static int atactlrmask;
static Controller *atactlr[NCtlr];
static int atadrivemask;
static Drive *atadrive[NDrive];
static int pbase[NCtlr] = {
	Pbase0, Pbase1, Pbase2, Pbase3,
};
static int defirq[NCtlr] = {
	14, 15, 0, 0,
};

static void	ataintr(Ureg*, void*);
static long	ataxfer(Drive*, Partition*, int, ulong, long);
static int	ataident(Drive*);
static Drive*	atapart(Drive*);
static int	ataparams(Drive*);
static void	atarecal(Drive*);
static int	ataprobe(Drive*, int, int, int);
static Drive*	atapipart(Drive*);
static void	atapiintr(Controller*);
static long	atapiio(Drive*, long);

static void
atactlrprobe(int ctlrno, int irq)
{
	Controller *ctlr;
	Drive *drive;
	int driveno, port;

	if(atactlrmask & (1<<ctlrno))
		return;
	atactlrmask |= 1<<ctlrno;

	port = pbase[ctlrno];
	outb(port+Pdh, DHmagic);
	delay(1);
	if((inb(port+Pdh) & 0xFF) != DHmagic){
		DPRINT("ata%d: DHmagic not ok\n", ctlrno);
		return;
	}
	DPRINT("ata%d: DHmagic ok\n", ctlrno);

	atactlr[ctlrno] = ialloc(sizeof(Controller), 0);
	ctlr = atactlr[ctlrno];
	ctlr->pbase = port;
	ctlr->ctlrno = ctlrno;
	ctlr->buf = ialloc(Maxxfer, 0);
	inb(ctlr->pbase+Pstatus);
	setvec(irq, ataintr, ctlr);

	driveno = ctlrno*2;
	atadrive[driveno] = ialloc(sizeof(Drive), 0);
	drive = atadrive[driveno];
	drive->cp = ctlr;
	drive->driveno = driveno;
	drive->dh = DHmagic;

	driveno++;
	atadrive[driveno] = ialloc(sizeof(Drive), 0);
	drive = atadrive[driveno];
	drive->cp = ctlr;
	drive->driveno = driveno;
	drive->dh = DHmagic|DHslave;
}

static Drive*
atadriveprobe(int driveno)
{
	Drive *drive;
	int ctlrno;
	ISAConf isa;

	ctlrno = driveno/2;
	if(atactlr[ctlrno] == 0){
		if(atactlrmask & (1<<ctlrno))
			return 0;
		memset(&isa, 0, sizeof(ISAConf));
		if(isaconfig("ata", ctlrno, &isa) == 0)
			return 0;
		if(ctlrno && isa.irq)
			atactlrprobe(ctlrno, Int0vec+isa.irq);
		if(atactlr[ctlrno] == 0)
			return 0;
	}

	drive = atadrive[driveno];
	if(drive->online == 0){
		if(atadrivemask & (1<<driveno))
			return 0;
		atadrivemask |= 1<<driveno;
		if(ataparams(drive))
			return 0;
		if(drive->lba)
			print("hd%d: LBA %lud sectors\n",
				drive->driveno, drive->cap);
		else
			print("hd%d: CHS %ld/%d/%d %lud sectors\n",
				drive->driveno, drive->cyl, drive->heads,
				drive->sectors, drive->cap);
		drive->online = 1;
	}

	return atapart(drive);
}

int
atainit(void)
{
	ISAConf isa;
	int ctlrno;

	for(ctlrno = 0; ctlrno < NCtlr; ctlrno++){
		memset(&isa, 0, sizeof(ISAConf));
		if(isaconfig("ata", ctlrno, &isa) == 0 && ctlrno > 1)
			continue;
		if(isa.irq == 0 && (isa.irq = defirq[ctlrno]) == 0)
			continue;
		atactlrprobe(ctlrno, Int0vec+isa.irq);
	}
	DPRINT("atainit: mask 0x%ux 0x%ux\n", atactlrmask, atadrivemask);

	return 0xFF;
}

long
ataseek(int driveno, long offset)
{
	Drive *drive;

	if((drive = atadriveprobe(driveno)) == 0)
		return -1;
	drive->offset = offset;
	return offset;
}

/*
 *  did an interrupt happen?
 */
static void
atawait(Controller *cp, int timo)
{
	ulong start;
	int x;

	x = spllo();
	start = m->ticks;
	while(TK2MS(m->ticks - start) < timo){
		if(cp->cmd == 0)
			break;
		if(cp->cmd == Cident2 && TK2MS(m->ticks - start) >= 1000)
			break;
	}
	if(TK2MS(m->ticks - start) >= timo){
		DPRINT("atawait timed out 0x%ux\n", inb(cp->pbase+Pstatus));
		ataintr(0, cp);
	}
	splx(x);
}

int
setatapart(int driveno, char *p)
{
	Partition *pp;
	Drive *dp;

	if((dp = atadriveprobe(driveno)) == 0)
		return 0;

	for(pp = dp->p; pp < &dp->p[dp->npart]; pp++)
		if(strcmp(pp->name, p) == 0){
			dp->current = pp;
			return 1;
		}
	return 0;
}

long
ataread(int driveno, void *a, long n)
{
	Drive *dp;
	long rv, i;
	int skip;
	uchar *aa = a;
	Partition *pp;
	Controller *cp;

	if((dp = atadriveprobe(driveno)) == 0)
		return 0;

	pp = dp->current;
	if(pp == 0)
		return -1;
	cp = dp->cp;

	skip = dp->offset % dp->bytes;
	for(rv = 0; rv < n; rv += i){
		i = ataxfer(dp, pp, Cread, dp->offset+rv-skip, n-rv+skip);
		if(i == 0)
			break;
		if(i < 0)
			return -1;
		i -= skip;
		if(i > n - rv)
			i = n - rv;
		memmove(aa+rv, cp->buf + skip, i);
		skip = 0;
	}
	dp->offset += rv;

	return rv;
}

long
atawrite(int driveno, void *a, long n)
{
	Drive *dp;
	long rv, i, partial;
	uchar *aa = a;
	Partition *pp;
	Controller *cp;

	if((dp = atadriveprobe(driveno)) == 0)
		return 0;

	pp = dp->current;
	if(pp == 0)
		return -1;
	cp = dp->cp;

	/*
	 *  if not starting on a sector boundary,
	 *  read in the first sector before writing
	 *  it out.
	 */
	partial = dp->offset % dp->bytes;
	if(partial){
		ataxfer(dp, pp, Cread, dp->offset-partial, dp->bytes);
		if(partial+n > dp->bytes)
			rv = dp->bytes - partial;
		else
			rv = n;
		memmove(cp->buf+partial, aa, rv);
		if(ataxfer(dp, pp, Cwrite, dp->offset-partial, dp->bytes) < 0)
			return -1;
	} else
		rv = 0;

	/*
	 *  write out the full sectors
	 */
	partial = (n - rv) % dp->bytes;
	n -= partial;
	for(; rv < n; rv += i){
		i = n - rv;
		if(i > Maxxfer)
			i = Maxxfer;
		memmove(cp->buf, aa+rv, i);
		i = ataxfer(dp, pp, Cwrite, dp->offset+rv, i);
		if(i == 0)
			break;
		if(i < 0)
			return -1;
	}

	/*
	 *  if not ending on a sector boundary,
	 *  read in the last sector before writing
	 *  it out.
	 */
	if(partial){
		if(ataxfer(dp, pp, Cread, dp->offset+rv, dp->bytes) < 0)
			return -1;
		memmove(cp->buf, aa+rv, partial);
		if(ataxfer(dp, pp, Cwrite, dp->offset+rv, dp->bytes) < 0)
			return -1;
		rv += partial;
	}
	dp->offset += rv;
	return rv;
}

/*
 *  wait for the controller to be ready to accept a command
 */
static int
cmdreadywait(Drive *drive)
{
	ulong end;
	uchar dh, status;
	Controller *ctlr;

	ctlr = drive->cp;
	end = m->ticks+MS2TK(10)+1;
	dh = (inb(ctlr->pbase+Pdh) & DHslave)^(drive->dh & DHslave);
	
	status = 0;
	while(m->ticks < end){
		status = inb(ctlr->pbase+Pstatus);
		if(status & Sbusy)
			continue;
		if(dh){
			outb(ctlr->pbase+Pdh, drive->dh);
			dh = 0;
			continue;
		}
		if(drive->atapi || (status & Sready))
			return 0;
	}
	USED(status);

	DPRINT("hd%d: cmdreadywait failed 0x%ux\n", drive->driveno, status);
	outb(ctlr->pbase+Pdh, DHmagic);
	return -1;
}

/*
 *  transfer a number of sectors.  ataintr will perform all the iterative
 *  parts.
 */
static long
ataxfer(Drive *dp, Partition *pp, int cmd, ulong start, long len)
{
	Controller *cp;
	long lsec;
	int loop;

	if(dp->online == 0){
		DPRINT("hd%d: disk not on line\n", dp->driveno);
		return -1;
	}

	/*
	 *  cut transfer size down to disk buffer size
	 */
	start = start / dp->bytes;
	if(len > Maxxfer)
		len = Maxxfer;
	len = (len + dp->bytes - 1) / dp->bytes;

	/*
	 *  calculate physical address
	 */
	cp = dp->cp;
	lsec = start + pp->start;
	if(lsec >= pp->end){
		DPRINT("hd%d: read past end of partition\n", dp->driveno);
		return 0;
	}
	if(dp->lba){
		cp->tsec = lsec & 0xff;
		cp->tcyl = (lsec>>8) & 0xffff;
		cp->thead = (lsec>>24) & 0xf;
	} else {
		cp->tcyl = lsec/(dp->sectors*dp->heads);
		cp->tsec = (lsec % dp->sectors) + 1;
		cp->thead = (lsec/dp->sectors) % dp->heads;
	}

	/*
	 *  can't xfer past end of disk
	 */
	if(lsec+len > pp->end)
		len = pp->end - lsec;
	cp->nsecs = len;

	if(dp->atapi){
		if(cmd == Cread)
			cp->cmd = Cread2;
		else
			cp->cmd = Cwrite2;
		cp->dp = dp;
		cp->sofar = 0;
		cp->status = 0;
		return atapiio(dp, lsec);
	}

	if(cmdreadywait(dp) < 0)
		return -1;

	/*
	 *  start the transfer
	 */
	cp->cmd = cmd;
	cp->dp = dp;
	cp->sofar = 0;
	cp->status = 0;
	DPRINT("hd%d: xfer:\ttcyl %d, tsec %d, thead %d\n",
		dp->driveno, cp->tcyl, cp->tsec, cp->thead);
	DPRINT("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
	outb(cp->pbase+Pcount, cp->nsecs);
	outb(cp->pbase+Psector, cp->tsec);
	outb(cp->pbase+Pdh, dp->dh | (dp->lba<<6) | cp->thead);
	outb(cp->pbase+Pcyllsb, cp->tcyl);
	outb(cp->pbase+Pcylmsb, cp->tcyl>>8);
	outb(cp->pbase+Pcmd, cmd);

	if(cmd == Cwrite){
		loop = 0;
		while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
			if(++loop > 10000)
				panic("ataxfer");
		outss(cp->pbase+Pdata, cp->buf, dp->bytes/2);
	}

	atawait(cp, ATAtimo);

	if(cp->status & Serr){
		DPRINT("hd%d: err: status 0x%ux, err 0x%ux\n",
			dp->driveno, cp->status, cp->error);
		DPRINT("\ttcyl %d, tsec %d, thead %d\n",
			cp->tcyl, cp->tsec, cp->thead);
		DPRINT("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
		return -1;
	}

	return cp->nsecs*dp->bytes;
}

static int
isatapi(Drive *drive)
{
	Controller *cp;

	cp = drive->cp;
	outb(cp->pbase+Pdh, drive->dh);
	IDPRINT("hd%d: isatapi %d\n", drive->driveno, drive->atapi);
	outb(cp->pbase+Pcmd, 0x08);
	drive->atapi = 1;
	if(cmdreadywait(drive)){
		drive->atapi = 0;
		return 0;
	}
	drive->atapi = 0;
	drive->bytes = 512;
	microdelay(1);
	if(inb(cp->pbase+Pstatus)){
		IDPRINT("hd%d: isatapi status 0x%ux\n",
			drive->driveno, inb(cp->pbase+Pstatus));
		return 0;
	}
	if(inb(cp->pbase+Pcylmsb) != 0xEB || inb(cp->pbase+Pcyllsb) != 0x14){
		IDPRINT("hd%d: isatapi cyl 0x%ux 0x%ux\n",
			drive->driveno, inb(cp->pbase+Pcylmsb), inb(cp->pbase+Pcyllsb));
		return 0;
	}
	drive->atapi = 1;

	return 1;
}

/*
 *  get parameters from the drive
 */
static int
ataident(Drive *dp)
{
	Controller *cp;
	Ident *ip;
	ulong lbasecs;
	char id[21];

	dp->bytes = 512;
	cp = dp->cp;

retryatapi:
	cp->nsecs = 1;
	cp->sofar = 0;
	cp->dp = dp;
	outb(cp->pbase+Pdh, dp->dh);
	microdelay(1);
	if(inb(cp->pbase+Pcylmsb) == 0xEB && inb(cp->pbase+Pcyllsb) == 0x14){
		dp->atapi = 1;
		cp->cmd = Cidentd;
	}
	else
		cp->cmd = Cident;
	outb(cp->pbase+Pcmd, cp->cmd);

	IDPRINT("hd%d: ident command 0x%ux sent\n", dp->driveno, cp->cmd);
	atawait(cp, ATAPItimo);

	if(cp->status & Serr){
		IDPRINT("hd%d: bad disk ident status\n", dp->driveno);
		if(cp->error & Eabort){
			if(isatapi(dp) && cp->cmd != Cidentd)
				goto retryatapi;
		}
		return -1;
	}
	
	atawait(cp, ATAtimo);

	ip = (Ident*)cp->buf;
	memmove(id, ip->model, sizeof(id)-1);
	id[sizeof(id)-1] = 0;

	IDPRINT("hd%d: config 0x%ux capabilities 0x%ux\n",
		dp->driveno, ip->config, ip->capabilities);
	if(dp->atapi){
		if((ip->config & 0x0060) == 0x0020)
			dp->drqintr = 1;
		if((ip->config & 0x1F00) == 0x0000)
			dp->atapi = 2;
	}

	lbasecs = (ip->lbasecs[0]) | (ip->lbasecs[1]<<16);
	if((ip->capabilities & (1<<9)) && (lbasecs & 0xf0000000) == 0){
		dp->lba = 1;
		dp->cap = lbasecs;
	} else {
		dp->lba = 0;
	
		if(ip->cvalid&(1<<0)){
			/* use current settings */
			dp->cyl = ip->ccyls;
			dp->heads = ip->cheads;
			dp->sectors = ip->cs2t;
		}
		else{
			/* use default (unformatted) settings */
			dp->cyl = ip->cyls;
			dp->heads = ip->heads;
			dp->sectors = ip->s2t;
		}

		/*
		 * Very strange, but some old non-LBA discs report
		 * sectors > 32.
		if(dp->heads >= 64 || dp->sectors >= 32)
			return -1;
		 */
		dp->cap = dp->cyl * dp->heads * dp->sectors;
	}
	IDPRINT("hd%d: %s  lba/atapi/drqintr: %d/%d/%d  C/H/S: %ld/%d/%d  CAP: %ld\n",
		dp->driveno, id,
		dp->lba, dp->atapi, dp->drqintr,
		dp->cyl, dp->heads, dp->sectors,
		dp->cap);

	return 0;
}

/*
 *  probe the given sector to see if it exists
 */
static int
ataprobe(Drive *dp, int cyl, int sec, int head)
{
	Controller *cp;

	cp = dp->cp;
	if(cmdreadywait(dp) < 0)
		return -1;

	/*
	 *  start the transfer
	 */
	cp->cmd = Cread;
	cp->dp = dp;
	cp->sofar = 0;
	cp->nsecs = 1;
	cp->status = 0;
	outb(cp->pbase+Pcount, 1);
	outb(cp->pbase+Psector, sec+1);
	outb(cp->pbase+Pdh, dp->dh | (dp->lba<<6) | head);
	outb(cp->pbase+Pcyllsb, cyl);
	outb(cp->pbase+Pcylmsb, cyl>>8);
	outb(cp->pbase+Pcmd, Cread);

	atawait(cp, ATAtimo);

	if(cp->status & Serr)
		return -1;

	return 0;
}

/*
 *  figure out the drive parameters
 */
static int
ataparams(Drive *dp)
{
	int i, hi, lo;

	/*
	 *  first try the easy way, ask the drive and make sure it
	 *  isn't lying.
	 */
	dp->bytes = 512;
	if(ataident(dp) < 0)
		return -1;
	if(dp->atapi)
		return 0;
	if(dp->lba){
		i = dp->cap - 1;
		if(ataprobe(dp, (i>>8)&0xffff, (i&0xff)-1, (i>>24)&0xf) == 0)
			return 0;
	} else {
		if(ataprobe(dp, dp->cyl-1, dp->sectors-1, dp->heads-1) == 0)
			return 0;
	}

	IDPRINT("hd%d: ataparam: cyl %ld sectors %d heads %d\n",
		dp->driveno, dp->cyl, dp->sectors, dp->heads);
	/*
	 *  the drive lied, determine parameters by seeing which ones
	 *  work to read sectors.
	 */
	dp->lba = 0;
	for(i = 0; i < 16; i++)
		if(ataprobe(dp, 0, 0, i) < 0)
			break;
	dp->heads = i;
	for(i = 0; i < 64; i++)
		if(ataprobe(dp, 0, i, 0) < 0)
			break;
	dp->sectors = i;
	for(i = 512; ; i += 512)
		if(ataprobe(dp, i, dp->sectors-1, dp->heads-1) < 0)
			break;
	lo = i - 512;
	hi = i;
	for(; hi-lo > 1;){
		i = lo + (hi - lo)/2;
		if(ataprobe(dp, i, dp->sectors-1, dp->heads-1) < 0)
			hi = i;
		else
			lo = i;
	}
	dp->cyl = lo + 1;
	dp->cap = dp->cyl * dp->heads * dp->sectors;

	if(dp->cyl == 0 || dp->heads == 0 || dp->sectors == 0)
		return -1;

	return 0;
}

static Drive*
atapart(Drive *dp)
{
	Partition *pp;

	if(dp->partok)
		return dp;

	/*
	 *  we always have a partition for the whole disk
	 */
	pp = &dp->p[0];
	strcpy(pp->name, "disk");
	dp->npart = 1;
	pp->start = 0;
	if(dp->atapi)
		return atapipart(dp);
	pp->end = dp->cap;
	dp->partok = 1;

	return dp;
}

/*
 *  we get an interrupt for every sector transferred
 */
static void
ataintr(Ureg*, void *arg)
{
	Controller *cp;
	Drive *dp;
	long loop;

	cp = arg;
	if((dp = cp->dp) == 0)
		return;

	loop = 0;
	while((cp->status = inb(cp->pbase+Pstatus)) & Sbusy)
		if(++loop > Maxloop){
			print("hd%d: intr busy: status 0x%ux\n", dp->driveno, cp->status);
			break;
		}
	switch(cp->cmd){
	case Cwrite:
		if(cp->status & Serr){
			cp->error = inb(cp->pbase+Perror);
			cp->cmd = 0;
			return;
		}
		cp->sofar++;
		if(cp->sofar < cp->nsecs){
			loop = 0;
			while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
				if(++loop > 10000){
					print("ataintr 1");
					cp->cmd = 0;
					return;
				}
			outss(cp->pbase+Pdata, &cp->buf[cp->sofar*dp->bytes],
				dp->bytes/2);
		} else{
			cp->cmd = 0;
		}
		break;
	case Cread:
	case Cident:
	case Cidentd:
		if(cp->status & Serr){
			cp->cmd = 0;
			cp->error = inb(cp->pbase+Perror);
			return;
		}
		loop = 0;
		while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
			if(++loop > Maxloop){
				print("hd%d: intr drq: cmd %ux status 0x%ux",
					dp->driveno, cp->cmd, inb(cp->pbase+Pstatus));
				cp->cmd = 0;
				return;
			}
		inss(cp->pbase+Pdata, &cp->buf[cp->sofar*dp->bytes],
			dp->bytes/2);
		cp->sofar++;
		if(cp->sofar >= cp->nsecs){
			if(cp->cmd == Cident && (cp->status & Sready) == 0)
				cp->cmd = Cident2; /* sometimes we get a second intr */
			else
				cp->cmd = 0;
			inb(cp->pbase+Pstatus);
		}
		break;
	case Csetbuf:
	case Cident2:
		cp->cmd = 0;
		break;
	case Cpktcmd:
		atapiintr(cp);
		break;
	default:
		cp->cmd = 0;
		break;
	}
}

static int
atapiexec(Drive *dp)
{
	Controller *cp;
	int loop, s;

	cp = dp->cp;

	if(cmdreadywait(dp))
		return -1;

	s = splhi();	
	cp->sofar = 0;
	cp->error = 0;
	cp->cmd = Cpktcmd;
	outb(cp->pbase+Pcount, 0);
	outb(cp->pbase+Psector, 0);
	outb(cp->pbase+Pfeature, 0);
	outb(cp->pbase+Pcyllsb, cp->len);
	outb(cp->pbase+Pcylmsb, cp->len>>8);
	outb(cp->pbase+Pdh, dp->dh);
	outb(cp->pbase+Pcmd, cp->cmd);

	if(dp->drqintr == 0){
		microdelay(1);
		for(loop = 0; (inb(cp->pbase+Pstatus) & (Serr|Sdrq)) == 0; loop++){
			if(loop < 10000)
				continue;
			panic("hd%d: cmddrqwait: cmd=0x%ux status=0x%ux\n",
				dp->driveno, cp->cmd, inb(cp->pbase+Pstatus));
		}
		outss(cp->pbase+Pdata, cp->cmdblk, sizeof(cp->cmdblk)/2);
	}
	splx(s);

	atawait(cp, ATAPItimo);

	if(cp->status & Serr){
		DPRINT("hd%d: Bad packet command 0x%ux, error 0x%ux\n",
			dp->driveno, cp->cmdblk[0], cp->error);
		return -1;
	}

	return 0;
}

static long
atapiio(Drive *dp, long lba)
{
	int n;
	Controller *cp;

	cp = dp->cp;

	n = cp->nsecs*dp->bytes;
	cp->len = n;
	cp->count = 0;
	memset(cp->cmdblk, 0, 12);
	cp->cmdblk[0] = cp->cmd;
	cp->cmdblk[2] = lba >> 24;
	cp->cmdblk[3] = lba >> 16;
	cp->cmdblk[4] = lba >> 8;
	cp->cmdblk[5] = lba;
	cp->cmdblk[7] = cp->nsecs>>8;
	cp->cmdblk[8] = cp->nsecs;
	if(atapiexec(dp))
		return -1;
	if(cp->count != n)
		print("hd%d: short read %d != %d\n", dp->driveno, cp->count, n);

	return n;
}

static Drive*
atapipart(Drive *dp)
{
	Controller *cp;
	Partition *pp;
	int retrycount;

	cp = dp->cp;

	pp = &dp->p[0];
	pp->end = 0;

	retrycount = 0;
retry:
	if(retrycount++){
		IDPRINT("hd%d: atapipart: cmd 0x%ux error 0x%ux, retry %d\n",
			dp->driveno, cp->cmdblk[0], cp->error, retrycount);
		if((cp->status & Serr) && (cp->error & 0xF0) == 0x60){
			dp->vers++;
			if(retrycount < 3)
				goto again;
		}
		cp->dp = 0;
		IDPRINT("hd%d: atapipart: cmd %ux return error %ux, retry %d\n",
			dp->driveno, cp->cmd, cp->error, retrycount);
		return 0;
	}
again:
	cp->dp = dp;

	cp->len = 18;
	cp->count = 0;
	memset(cp->cmdblk, 0, sizeof(cp->cmdblk));
	cp->cmdblk[0] = Creqsense;
	cp->cmdblk[4] = 18;
	DPRINT("reqsense %d\n", retrycount);
	atapiexec(dp);
	//if(atapiexec(dp))
	//	goto retry;
	if(cp->count != 18){
		print("cmd=0x%2.2ux, lastcmd=0x%2.2ux ", cp->cmd, cp->lastcmd);
		print("cdsize count %d, status 0x%2.2ux, error 0x%2.2ux\n",
			cp->count, cp->status, cp->error);
		return 0;
	}

	cp->len = 8;
	cp->count = 0;
	memset(cp->cmdblk, 0, sizeof(cp->cmdblk));
	cp->cmdblk[0] = Ccapacity;
	DPRINT("capacity %d\n", retrycount);
	if(atapiexec(dp))
		goto retry;
	if(cp->count != 8){
		print("cmd=0x%2.2ux, lastcmd=0x%2.2ux ", cp->cmd, cp->lastcmd);
		print("cdsize count %d, status 0x%2.2ux, error 0x%2.2ux\n",
			cp->count, cp->status, cp->error);
		return 0;
	}
	dp->sectors = (cp->buf[0]<<24)|(cp->buf[1]<<16)|(cp->buf[2]<<8)|cp->buf[3];
	dp->bytes = (cp->buf[4]<<24)|(cp->buf[5]<<16)|(cp->buf[6]<<8)|cp->buf[7];
	if(dp->bytes > 2048 && dp->bytes <= 2352)
		dp->bytes = 2048;
	dp->cap = dp->sectors;
	IDPRINT("hd%d: atapipart secs %ud, bytes %ud, cap %lud\n",
		dp->driveno, dp->sectors, dp->bytes, dp->cap);
	cp->dp = 0;

	pp->end = dp->sectors;
	dp->partok = 1;

	return dp;
}

static void
atapiintr(Controller *cp)
{
	uchar cause;
	int count, loop, pbase;

	pbase = cp->pbase;
	cause = inb(pbase+Pcount) & 0x03;
	DPRINT("hd%d: atapiintr 0x%ux\n", cp->dp->driveno, cause);
	switch(cause){

	case 1:						/* command */
		if(cp->status & Serr){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->error = inb(pbase+Perror);
			break;
		}
		outss(pbase+Pdata, cp->cmdblk, sizeof(cp->cmdblk)/2);
		break;

	case 0:						/* data out */
	case 2:						/* data in */
		if(cp->buf == 0){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			if(cp->status & Serr)
				cp->error = inb(pbase+Perror);
			cp->cmd = 0;
			break;	
		}
		loop = 0;
		while((cp->status & (Serr|Sdrq)) == 0){
			if(++loop > Maxloop){
				cp->status |= Serr;
				break;
			}
			cp->status = inb(pbase+Pstatus);
		}
		if(cp->status & Serr){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->error = inb(pbase+Perror);
			print("hd%d: Cpktcmd status=0x%ux, error=0x%ux\n",
				cp->dp->driveno, cp->status, cp->error);
			break;
		}
		count = inb(pbase+Pcyllsb)|(inb(pbase+Pcylmsb)<<8);
		if(cp->count+count > Maxxfer)
			panic("hd%d: count %d, already %d\n", count, cp->count);
		if(cause == 0)
			outss(pbase+Pdata, cp->buf+cp->count, count/2);
		else
			inss(pbase+Pdata, cp->buf+cp->count, count/2);
		cp->count += count;
		break;

	case 3:						/* status */
		cp->lastcmd = cp->cmd;
		cp->cmd = 0;
		if(cp->status & Serr)
			cp->error = inb(cp->pbase+Perror);
		break;
	}
}
