#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

#define DPRINT if(0)print

typedef	struct Drive		Drive;
typedef	struct Ident		Ident;
typedef	struct Controller	Controller;
typedef struct Partition	Partition;
typedef struct Repl		Repl;

enum
{
	/* ports */
	Pbase0=		0x1F0,
	Pbase1=		0x170,
	Pdata=		0,	/* data port (16 bits) */
	Perror=		1,	/* error port (read) */
	Pprecomp=	1,	/* buffer mode port (write) */
	Pcount=		2,	/* sector count port */
	Psector=	3,	/* sector number port */
	Pcyllsb=	4,	/* least significant byte cylinder # */
	Pcylmsb=	5,	/* most significant byte cylinder # */
	Pdh=		6,	/* drive/head port */
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
	Cinitparam=	0x91,

	/* conner specific commands */
	Cstandby=	0xE2,
	Cidle=		0xE1,
	Cpowerdown=	0xE3,

	/* disk states */
	Sspinning,
	Sstandby,
	Sidle,
	Spowerdown,

	/* something we have to or into the drive/head reg */
	DHmagic=	0xA0,

	/* file types */
	Qdir=		0,

	Maxxfer=	BY2PG,		/* maximum transfer size/cmd */
	Npart=		8+2,		/* 8 sub partitions, disk, and partition */
	Nrepl=		64,		/* maximum replacement blocks */

	Hardtimeout=	4000,		/* disk access timeout */
};
#define PART(x)		((x)&0xF)
#define DRIVE(x)	(((x)>>4)&0x7)
#define MKQID(d,p)	(((d)<<4) | (p))

struct Partition
{
	ulong	start;
	ulong	end;
	char	name[NAMELEN+1];
};

struct Repl
{
	Partition *p;
	int	nrepl;
	ulong	blk[Nrepl];
};

#define PARTMAGIC	"plan9 partitions"
#define REPLMAGIC	"block replacements"

/*
 *  an ata drive
 */
struct Drive
{
	QLock;

	Controller *cp;
	int	drive;
	int	confused;	/* needs to be recalibrated (or worse) */
	int	online;
	int	npart;		/* number of real partitions */
	Partition p[Npart];
	Repl	repl;
	ulong	usetime;
	int	state;
	char	vol[NAMELEN];

	ulong	cap;		/* total bytes */
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* heads/cyl */
	long	cyl;		/* cylinders/drive */

	char	lba;		/* true if drive has logical block addressing */
	char	multi;		/* non-zero if drive does multiple block xfers */
};

/*
 *  a controller for 2 drives
 */
struct Controller
{
	QLock;			/* exclusive access to the controller */

	Lock	reglock;	/* exclusive access to the registers */

	int	confused;	/* needs to be recalibrated (or worse) */
	int	pbase;		/* base port */

	/*
	 *  current operation
	 */
	int	cmd;		/* current command */
	int	lastcmd;	/* debugging info */
	Rendez	r;		/* wait here for command termination */
	char	*buf;		/* xfer buffer */
	int	nsecs;		/* length of transfer (sectors) */
	int	sofar;		/* sectors transferred so far */
	int	status;
	int	error;
	Drive	*dp;		/* drive being accessed */
};

Controller	*atac;
Drive		*ata;
static int	spindowntime;

static void	ataintr(Ureg*, void*);
static long	ataxfer(Drive*, Partition*, int, long, long, char*);
static void	ataident(Drive*);
static void	atasetbuf(Drive*, int);
static void	ataparams(Drive*);
static void	atapart(Drive*);
static int	ataprobe(Drive*, int, int, int);

static int
atagen(Chan *c, Dirtab *tab, long ntab, long s, Dir *dirp)
{
	Qid qid;
	int drive;
	char name[NAMELEN+4];
	Drive *dp;
	Partition *pp;
	ulong l;

	USED(tab, ntab);
	qid.vers = 0;
	drive = s/Npart;
	s = s % Npart;
	if(drive >= conf.nhard)
		return -1;
	dp = &ata[drive];

	if(dp->online == 0 || s >= dp->npart)
		return 0;

	pp = &dp->p[s];
	sprint(name, "%s%s", dp->vol, pp->name);
	name[NAMELEN] = 0;
	qid.path = MKQID(drive, s);
	l = (pp->end - pp->start) * dp->bytes;
	devdir(c, qid, name, l, eve, 0660, dirp);
	return 1;
}

void
atareset(void)
{
	Drive *dp;
	Controller *cp;
	uchar equip;
	char *p;

	equip = nvramread(0x12);
	if(equip == 0)
		equip = 0x10;		/* the Globalyst 250 lies */

	ata = xalloc(2 * sizeof(Drive));
	atac = xalloc(sizeof(Controller));

	cp = atac;
	cp->buf = 0;
	cp->lastcmd = cp->cmd;
	cp->cmd = 0;
	cp->pbase = Pbase0;
	setvec(ATAvec0, ataintr, 0);

	dp = ata;
	if(equip & 0xf0){
		dp->drive = 0;
		dp->online = 0;
		dp->cp = cp;
		dp++;
	}
	if((equip & 0x0f)){
		dp->drive = 1;
		dp->online = 0;
		dp->cp = cp;
		dp++;
	}
	conf.nhard = dp - ata;
	
	if(conf.nhard && (p = getconf("spindowntime")))
		spindowntime = atoi(p);
}

void
atainit(void)
{
}

/*
 *  Get the characteristics of each drive.  Mark unresponsive ones
 *  off line.
 */
Chan*
ataattach(char *spec)
{
	Drive *dp;

	for(dp = ata; dp < &ata[conf.nhard]; dp++){
		if(waserror()){
			dp->online = 0;
			qunlock(dp);
			continue;
		}
		qlock(dp);
		if(!dp->online){
			/*
			 * Make sure ataclock() doesn't
			 * interfere.
			 */
			dp->usetime = m->ticks;
			ataparams(dp);
			dp->online = 1;
			atasetbuf(dp, 1);
		}

		/*
		 *  read Plan 9 partition table
		 */
		atapart(dp);
		qunlock(dp);
		poperror();
	}
	return devattach('H', spec);
}

Chan*
ataclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
atawalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, atagen);
}

void
atastat(Chan *c, char *dp)
{
	devstat(c, dp, 0, 0, atagen);
}

Chan*
ataopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, 0, atagen);
}

void
atacreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
ataclose(Chan *c)
{
	Drive *d;
	Partition *p;

	if(c->mode != OWRITE && c->mode != ORDWR)
		return;

	d = &ata[DRIVE(c->qid.path)];
	p = &d->p[PART(c->qid.path)];
	if(strcmp(p->name, "partition") != 0)
		return;

	if(waserror()){
		qunlock(d);
		nexterror();
	}
	qlock(d);
	atapart(d);
	qunlock(d);
	poperror();
}

void
ataremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
atawstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

long
ataread(Chan *c, void *a, long n, ulong offset)
{
	Drive *dp;
	long rv, i;
	int skip;
	uchar *aa = a;
	Partition *pp;
	char *buf;

	if(c->qid.path == CHDIR)
		return devdirread(c, a, n, 0, 0, atagen);

	buf = smalloc(Maxxfer);
	if(waserror()){
		free(buf);
		nexterror();
	}

	dp = &ata[DRIVE(c->qid.path)];
	pp = &dp->p[PART(c->qid.path)];

	skip = offset % dp->bytes;
	for(rv = 0; rv < n; rv += i){
		i = ataxfer(dp, pp, Cread, offset+rv-skip, n-rv+skip, buf);
		if(i == 0)
			break;
		i -= skip;
		if(i > n - rv)
			i = n - rv;
		memmove(aa+rv, buf + skip, i);
		skip = 0;
	}

	free(buf);
	poperror();

	return rv;
}

long
atawrite(Chan *c, void *a, long n, ulong offset)
{
	Drive *dp;
	long rv, i, partial;
	uchar *aa = a;
	Partition *pp;
	char *buf;

	if(c->qid.path == CHDIR)
		error(Eisdir);

	dp = &ata[DRIVE(c->qid.path)];
	pp = &dp->p[PART(c->qid.path)];
	buf = smalloc(Maxxfer);
	if(waserror()){
		free(buf);
		nexterror();
	}

	/*
	 *  if not starting on a sector boundary,
	 *  read in the first sector before writing
	 *  it out.
	 */
	partial = offset % dp->bytes;
	if(partial){
		ataxfer(dp, pp, Cread, offset-partial, dp->bytes, buf);
		if(partial+n > dp->bytes)
			rv = dp->bytes - partial;
		else
			rv = n;
		memmove(buf+partial, aa, rv);
		ataxfer(dp, pp, Cwrite, offset-partial, dp->bytes, buf);
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
		memmove(buf, aa+rv, i);
		i = ataxfer(dp, pp, Cwrite, offset+rv, i, buf);
		if(i == 0)
			break;
	}

	/*
	 *  if not ending on a sector boundary,
	 *  read in the last sector before writing
	 *  it out.
	 */
	if(partial){
		ataxfer(dp, pp, Cread, offset+rv, dp->bytes, buf);
		memmove(buf, aa+rv, partial);
		ataxfer(dp, pp, Cwrite, offset+rv, dp->bytes, buf);
		rv += partial;
	}

	free(buf);
	poperror();

	return rv;
}

/*
 *  did an interrupt happen?
 */
static int
cmddone(void *a)
{
	Controller *cp = a;

	return cp->cmd == 0;
}

/*
 * Wait for the controller to be ready to accept a command.
 * This is protected from intereference by ataclock() by
 * setting dp->usetime before it is called.
 */
static void
cmdreadywait(Drive *dp)
{
	long start;
	int period;
	Controller *cp = dp->cp;

	/* give it 2 seconds to spin down and up */
	if(dp->state == Sspinning)
		period = 10;
	else
		period = 2000;

	start = m->ticks;
	while((inb(cp->pbase+Pstatus) & (Sready|Sbusy)) != Sready)
		if(TK2MS(m->ticks - start) > period){
			DPRINT("cmdreadywait failed\n");
			error(Eio);
		}
}

static void
atarepl(Drive *dp, long bblk)
{
	int i;

	if(dp->repl.p == 0)
		return;
	for(i = 0; i < dp->repl.nrepl; i++){
		if(dp->repl.blk[i] == bblk)
			DPRINT("found bblk %ld at offset %ld\n", bblk, i);
	}
}

static void
atasleep(Controller *cp)
{
	tsleep(&cp->r, cmddone, cp, Hardtimeout);
	if(cp->cmd && cp->cmd != Cident2){
		DPRINT("hard drive timeout\n");
		error("ata drive timeout");
	}
}


/*
 *  transfer a number of sectors.  ataintr will perform all the iterative
 *  parts.
 */
static long
ataxfer(Drive *dp, Partition *pp, int cmd, long start, long len, char *buf)
{
	Controller *cp;
	long lblk;
	int cyl, sec, head;
	int loop, stat;

	if(dp->online == 0)
		error(Eio);

	/*
	 *  cut transfer size down to disk buffer size
	 */
	start = start / dp->bytes;
	if(len > Maxxfer)
		len = Maxxfer;
	len = (len + dp->bytes - 1) / dp->bytes;
	if(len == 0)
		return 0;

	/*
	 *  calculate physical address
	 */
	lblk = start + pp->start;
	if(lblk >= pp->end)
		return 0;
	if(lblk+len > pp->end)
		len = pp->end - lblk;
	if(dp->lba){
		sec = lblk & 0xff;
		cyl = (lblk>>8) & 0xffff;
		head = (lblk>>24) & 0xf;
	} else {
		cyl = lblk/(dp->sectors*dp->heads);
		sec = (lblk % dp->sectors) + 1;
		head = ((lblk/dp->sectors) % dp->heads);
	}

	cp = dp->cp;
	qlock(cp);
	if(waserror()){
		cp->buf = 0;
		qunlock(cp);
		nexterror();
	}

	/*
	 * Make sure hardclock() doesn't
	 * interfere.
	 */
	dp->usetime = m->ticks;
	cmdreadywait(dp);

	ilock(&cp->reglock);
	cp->sofar = 0;
	cp->buf = buf;
	cp->nsecs = len;
	cp->cmd = cmd;
	cp->dp = dp;
	cp->status = 0;

	outb(cp->pbase+Pcount, cp->nsecs);
	outb(cp->pbase+Psector, sec);
	outb(cp->pbase+Pdh, DHmagic | (dp->drive<<4) | (dp->lba<<6) | head);
	outb(cp->pbase+Pcyllsb, cyl);
	outb(cp->pbase+Pcylmsb, cyl>>8);
	outb(cp->pbase+Pcmd, cmd);

	if(cmd == Cwrite){
		loop = 0;
		while((stat = inb(cp->pbase+Pstatus) & (Serr|Sdrq)) == 0)
			if(++loop > 10000)
				panic("ataxfer");
		outss(cp->pbase+Pdata, cp->buf, dp->bytes/2);
	} else
		stat = 0;
	iunlock(&cp->reglock);

	if(stat & Serr)
		error(Eio);

	/*
	 *  wait for command to complete.  if we get a note,
	 *  remember it but keep waiting to let the disk finish
	 *  the current command.
	 */
	loop = 0;
	while(waserror()){
		DPRINT("interrupted ataxfer\n");
		if(loop++ > 10){
			print("ata disk error\n");
			nexterror();
		}
	}
	atasleep(cp);
	dp->state = Sspinning;
	dp->usetime = m->ticks;
	poperror();
	if(loop)
		nexterror();

	if(cp->status & Serr){
		DPRINT("hd%d err: lblk %ld status %lux, err %lux\n",
			dp-ata, lblk, cp->status, cp->error);
		DPRINT("\tcyl %d, sec %d, head %d\n", cyl, sec, head);
		DPRINT("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
		atarepl(dp, lblk+cp->sofar);
		error(Eio);
	}
	cp->buf = 0;
	len = cp->sofar*dp->bytes;
	qunlock(cp);
	poperror();

	return len;
}

/*
 *  set read ahead mode
 */
static void
atasetbuf(Drive *dp, int on)
{
	Controller *cp = dp->cp;

	qlock(cp);
	if(waserror()){
		qunlock(cp);
		nexterror();
	}

	cmdreadywait(dp);

	ilock(&cp->reglock);
	cp->cmd = Csetbuf;
	outb(cp->pbase+Pprecomp, on ? 0xAA : 0x55);	/* read look ahead */
	outb(cp->pbase+Pdh, DHmagic | (dp->drive<<4));
	outb(cp->pbase+Pcmd, Csetbuf);
	iunlock(&cp->reglock);

	atasleep(cp);

/*	if(cp->status & Serr)
		DPRINT("hd%d setbuf err: status %lux, err %lux\n",
			dp-ata, cp->status, cp->error);/**/

	poperror();
	qunlock(cp);
}

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

/*
 *  get parameters from the drive
 */
static void
ataident(Drive *dp)
{
	Controller *cp;
	char *buf;
	Ident *ip;
	char id[21];

	cp = dp->cp;
	buf = smalloc(Maxxfer);
	qlock(cp);
	if(waserror()){
		qunlock(cp);
		nexterror();
	}

	cmdreadywait(dp);

	ilock(&cp->reglock);
	cp->nsecs = 1;
	cp->sofar = 0;
	cp->cmd = Cident;
	cp->dp = dp;
	cp->buf = buf;
	outb(cp->pbase+Pdh, DHmagic | (dp->drive<<4));
	outb(cp->pbase+Pcmd, Cident);
	iunlock(&cp->reglock);

	atasleep(cp);

	if(cp->status & Serr){
		DPRINT("bad disk ident status\n");
		error(Eio);
	}
	ip = (Ident*)buf;

	/*
	 * this function appears to respond with an extra interrupt after
	 * the ident information is read, except on the safari.  The following
	 * delay gives this extra interrupt a chance to happen while we are quiet.
	 * Otherwise, the interrupt may come during a subsequent read or write,
	 * causing a panic and much confusion.
	 */
	if (cp->cmd == Cident2)
		tsleep(&cp->r, return0, 0, Hardtimeout);

	memmove(id, ip->model, sizeof(id)-1);
	id[sizeof(id)-1] = 0;

	if(ip->capabilities & (1<<9)){
		dp->lba = 1;
		dp->sectors = (ip->lbasecs[0]) | (ip->lbasecs[1]<<16);
		dp->cap = dp->bytes * dp->sectors;
/*print("\nata%d model %s with %d lba sectors\n", dp->drive, id, dp->sectors);/**/
	} else {
		dp->lba = 0;

		/* use default (unformatted) settings */
		dp->cyl = ip->cyls;
		dp->heads = ip->heads;
		dp->sectors = ip->s2t;
/*print("\nata%d model %s with default %d cyl %d head %d sec\n", dp->drive,
			id, dp->cyl, dp->heads, dp->sectors);/**/

		if(ip->cvalid&(1<<0)){
			/* use current settings */
			dp->cyl = ip->ccyls;
			dp->heads = ip->cheads;
			dp->sectors = ip->cs2t;
/*print("\tchanged to %d cyl %d head %d sec\n", dp->cyl, dp->heads, dp->sectors);/**/
		}
		dp->cap = dp->bytes * dp->cyl * dp->heads * dp->sectors;
	}
	cp->lastcmd = cp->cmd;
	cp->cmd = 0;
	cp->buf = 0;
	free(buf);
	poperror();
	qunlock(cp);
}

/*
 *  probe the given sector to see if it exists
 */
static int
ataprobe(Drive *dp, int cyl, int sec, int head)
{
	Controller *cp;
	char *buf;
	int rv;

	cp = dp->cp;
	buf = smalloc(Maxxfer);
	qlock(cp);
	if(waserror()){
		free(buf);
		qunlock(cp);
		nexterror();
	}

	cmdreadywait(dp);

	ilock(&cp->reglock);
	cp->cmd = Cread;
	cp->dp = dp;
	cp->status = 0;
	cp->nsecs = 1;
	cp->sofar = 0;

	outb(cp->pbase+Pcount, 1);
	outb(cp->pbase+Psector, sec+1);
	outb(cp->pbase+Pdh, DHmagic | head | (dp->lba<<6) | (dp->drive<<4));
	outb(cp->pbase+Pcyllsb, cyl);
	outb(cp->pbase+Pcylmsb, cyl>>8);
	outb(cp->pbase+Pcmd, Cread);
	iunlock(&cp->reglock);

	atasleep(cp);

	if(cp->status & Serr)
		rv = -1;
	else
		rv = 0;

	cp->buf = 0;
	free(buf);
	poperror();
	qunlock(cp);
	return rv;
}

/*
 *  figure out the drive parameters
 */
static void
ataparams(Drive *dp)
{
	int i, hi, lo;

	/*
	 *  first try the easy way, ask the drive and make sure it
	 *  isn't lying.
	 */
	dp->bytes = 512;
	ataident(dp);
	if(dp->lba){
		i = dp->sectors - 1;
		if(ataprobe(dp, (i>>8)&0xffff, (i&0xff)-1, (i>>24)&0xf) == 0)
			return;
	} else {
		if(ataprobe(dp, dp->cyl-1, dp->sectors-1, dp->heads-1) == 0)
			return;
	}

	/*
	 *  the drive lied, determine parameters by seeing which ones
	 *  work to read sectors.
	 */
	dp->lba = 0;
	for(i = 0; i < 32; i++)
		if(ataprobe(dp, 0, 0, i) < 0)
			break;
	dp->heads = i;
	for(i = 0; i < 128; i++)
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
	dp->cap = dp->bytes * dp->cyl * dp->heads * dp->sectors;
}

/*
 *  Read block replacement table.
 *  The table is just ascii block numbers.
 */
static void
atareplinit(Drive *dp)
{
	char *line[Nrepl+1];
	char *field[1];
	ulong n;
	int i;
	char *buf;

	/*
	 *  check the partition is big enough
	 */
	if(dp->repl.p->end - dp->repl.p->start < Nrepl+1){
		dp->repl.p = 0;
		return;
	}

	buf = smalloc(Maxxfer);
	if(waserror()){
		free(buf);
		nexterror();
	}

	/*
	 *  read replacement table from disk, null terminate
	 */
	ataxfer(dp, dp->repl.p, Cread, 0, dp->bytes, buf);
	buf[dp->bytes-1] = 0;

	/*
	 *  parse replacement table.
	 */
	n = getfields(buf, line, Nrepl+1, "\n");
	if(strncmp(line[0], REPLMAGIC, sizeof(REPLMAGIC)-1)){
		dp->repl.p = 0;
	} else {
		for(dp->repl.nrepl = 0, i = 1; i < n; i++, dp->repl.nrepl++){
			if(getfields(line[i], field, 1, " ") != 1)
				break;
			dp->repl.blk[dp->repl.nrepl] = strtoul(field[0], 0, 0);
			if(dp->repl.blk[dp->repl.nrepl] <= 0)
				break;
		}
	}
	free(buf);
	poperror();
}

/*
 *  read partition table.  The partition table is just ascii strings.
 */
static void
atapart(Drive *dp)
{
	Partition *pp;
	char *line[Npart+1];
	char *field[3];
	ulong n;
	int i;
	char *buf;

	sprint(dp->vol, "hd%d", dp - ata);

	/*
	 *  we always have a partition for the whole disk
	 *  and one for the partition table
	 */
	pp = &dp->p[0];
	strcpy(pp->name, "disk");
	pp->start = 0;
	pp->end = dp->cap / dp->bytes;
	pp++;
	strcpy(pp->name, "partition");
	pp->start = dp->p[0].end - 1;
	pp->end = dp->p[0].end;
	dp->npart = 2;

	/*
	 * initialise the bad-block replacement info
	 */
	dp->repl.p = 0;

	buf = smalloc(Maxxfer);
	if(waserror()){
		free(buf);
		nexterror();
	}

	/*
	 *  read last sector from disk, null terminate.  This used
	 *  to be the sector we used for the partition tables.
	 *  However, this sector is special on some PC's so we've
	 *  started to use the second last sector as the partition
	 *  table instead.  To avoid reconfiguring all our old systems
	 *  we first look to see if there is a valid partition
	 *  table in the last sector.  If so, we use it.  Otherwise
	 *  we switch to the second last.
	 */
	ataxfer(dp, pp, Cread, 0, dp->bytes, buf);
	buf[dp->bytes-1] = 0;
	n = getfields(buf, line, Npart+1, "\n");
	if(n == 0 || strncmp(line[0], PARTMAGIC, sizeof(PARTMAGIC)-1)){
		dp->p[0].end--;
		dp->p[1].start--;
		dp->p[1].end--;
		ataxfer(dp, pp, Cread, 0, dp->bytes, buf);
		buf[dp->bytes-1] = 0;
		n = getfields(buf, line, Npart+1, "\n");
	}

	/*
	 *  parse partition table.
	 */
	if(n && strncmp(line[0], PARTMAGIC, sizeof(PARTMAGIC)-1) == 0){
		for(i = 1; i < n; i++){
			pp++;
			switch(getfields(line[i], field, 3, " ")) {
			case 2:
				if(strcmp(field[0], "unit") == 0)
					strncpy(dp->vol, field[1], NAMELEN);
				break;	
			case 3:
				strncpy(pp->name, field[0], NAMELEN);
				if(strncmp(pp->name, "repl", NAMELEN) == 0)
					dp->repl.p = pp;
				pp->start = strtoul(field[1], 0, 0);
				pp->end = strtoul(field[2], 0, 0);
				if(pp->start > pp->end || pp->start >= dp->p[0].end)
					break;
				dp->npart++;
			}
		}
	}
	free(buf);
	poperror();

	if(dp->repl.p)
		atareplinit(dp);
}

enum
{
	Maxloop=	10000,
};

/*
 *  we get an interrupt for every sector transferred
 */
static void
ataintr(Ureg *ur, void *arg)
{
	Controller *cp;
	Drive *dp;
	long loop;
	char *addr;

	USED(ur, arg);

	/*
 	 *  BUG!! if there is ever more than one controller, we need a way to
	 *	  distinguish which interrupted (use arg).
	 */
	cp = &atac[0];
	dp = cp->dp;

	ilock(&cp->reglock);

	loop = 0;
	while((cp->status = inb(cp->pbase+Pstatus)) & Sbusy){
		if(++loop > Maxloop) {
			DPRINT("cmd=%lux status=%lux\n",
				cp->cmd, inb(cp->pbase+Pstatus));
			panic("ataintr: wait busy");
		}
	}

	switch(cp->cmd){
	case Cwrite:
		if(cp->status & Serr){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->error = inb(cp->pbase+Perror);
			wakeup(&cp->r);
			break;
		}
		cp->sofar++;
		if(cp->sofar < cp->nsecs){
			loop = 0;
			while(((cp->status = inb(cp->pbase+Pstatus)) & Sdrq) == 0)
				if(++loop > Maxloop) {
					DPRINT("cmd=%lux status=%lux\n",
						cp->cmd, inb(cp->pbase+Pstatus));
					panic("ataintr: write");
				}
			addr = cp->buf;
			if(addr){
				addr += cp->sofar*dp->bytes;
				outss(cp->pbase+Pdata, addr, dp->bytes/2);
			}
		} else{
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			wakeup(&cp->r);
		}
		break;
	case Cread:
	case Cident:
		loop = 0;
		while((cp->status & (Serr|Sdrq)) == 0){
			if(++loop > Maxloop) {
				DPRINT("cmd=%lux status=%lux\n",
					cp->cmd, inb(cp->pbase+Pstatus));
				panic("ataintr: read/ident");
			}
			cp->status = inb(cp->pbase+Pstatus);
		}
		if(cp->status & Serr){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->error = inb(cp->pbase+Perror);
			wakeup(&cp->r);
			break;
		}
		addr = cp->buf;
		if(addr){
			addr += cp->sofar*dp->bytes;
			inss(cp->pbase+Pdata, addr, dp->bytes/2);
		}
		cp->sofar++;
		if(cp->sofar > cp->nsecs)
			print("ataintr %d %d\n", cp->sofar, cp->nsecs);
		if(cp->sofar >= cp->nsecs){
			cp->lastcmd = cp->cmd;
			if (cp->cmd == Cread)
				cp->cmd = 0;
			else
				cp->cmd = Cident2;
			wakeup(&cp->r);
		}
		break;
	case Cinitparam:
	case Csetbuf:
	case Cidle:
	case Cstandby:
	case Cpowerdown:
		cp->lastcmd = cp->cmd;
		cp->cmd = 0;
		wakeup(&cp->r);
		break;
	case Cident2:
		cp->lastcmd = cp->cmd;
		cp->cmd = 0;
		break;
	default:
		print("weird disk interrupt, cmd=%.2ux, lastcmd= %.2ux status=%.2ux\n",
			cp->cmd, cp->lastcmd, cp->status);
		break;
	}

	iunlock(&cp->reglock);
}

void
hardclock(void)
{
	int drive;
	Drive *dp;
	Controller *cp;
	int diff;

	if(spindowntime <= 0)
		return;

	for(drive = 0; drive < conf.nhard; drive++){
		dp = &ata[drive];
		cp = dp->cp;

		diff = TK2SEC(m->ticks - dp->usetime);
		if((dp->state == Sspinning) && (diff >= spindowntime)){
			ilock(&cp->reglock);
			cp->cmd = Cstandby;
			outb(cp->pbase+Pcount, 0);
			outb(cp->pbase+Pdh, DHmagic | (dp->drive<<4) | 0);
			outb(cp->pbase+Pcmd, cp->cmd);
			iunlock(&cp->reglock);
			dp->state = Sstandby;
		}
	}
}
