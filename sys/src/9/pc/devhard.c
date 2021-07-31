#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

#define DPRINT if(1)print

typedef	struct Drive		Drive;
typedef	struct Ident		Ident;
typedef	struct Controller	Controller;
typedef struct Partition	Partition;
typedef struct Repl		Repl;

enum
{
	/* ports */
	Pbase=		0x1F0,
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

	/* file types */
	Qdir=		0,

	Maxxfer=	BY2PG,		/* maximum transfer size/cmd */
	Npart=		8+2,		/* 8 sub partitions, disk, and partition */
	Nrepl=		64,		/* maximum replacement blocks */
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
 *  a hard drive
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

	ulong	cap;		/* total bytes */
	int	bytes;		/* bytes/sector */
	int	sectors;	/* sectors/track */
	int	heads;		/* heads/cyl */
	long	cyl;		/* cylinders/drive */
};

/*
 *  a controller for 2 drives
 */
struct Controller
{
	QLock;			/* exclusive access to the drive */

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

Controller	*hardc;
Drive		*hard;

static void	hardintr(Ureg*);
static long	hardxfer(Drive*, Partition*, int, long, long, char*);
static void	hardident(Drive*);
static void	hardsetbuf(Drive*, int);
static void	hardparams(Drive*);
static void	hardpart(Drive*);
static int	hardprobe(Drive*, int, int, int);

static int
hardgen(Chan *c, Dirtab *tab, long ntab, long s, Dir *dirp)
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
	dp = &hard[drive];

	if(s >= dp->npart)
		return 0;

	pp = &dp->p[s];
	sprint(name, "hd%d%s", drive, pp->name);
	name[NAMELEN] = 0;
	qid.path = MKQID(drive, s);
	l = (pp->end - pp->start) * dp->bytes;
	devdir(c, qid, name, l, eve, 0666, dirp);
	return 1;
}

/*
 *  we assume drives 0 and 1 are on the first controller, 2 and 3 on the
 *  second, etc.
 */
void
hardreset(void)
{
	Drive *dp;
	Controller *cp;
	int drive;
	uchar equip;

	hard = xalloc(conf.nhard * sizeof(Drive));
	hardc = xalloc(((conf.nhard+1)/2) * sizeof(Controller));
	
	/*
	 *  read nvram for number of hard drives (2 max)
	 */
	equip = nvramread(0x12);
	if(conf.nhard > 0 && (equip>>4) == 0)
		conf.nhard = 0;
	if(conf.nhard > 1 && (equip&0xf) == 0)
		conf.nhard = 1;

	for(drive = 0; drive < conf.nhard; drive++){
		dp = &hard[drive];
		cp = &hardc[drive/2];
		dp->drive = drive&1;
		dp->online = 0;
		dp->cp = cp;
		if((drive&1) == 0){
			cp->buf = 0;
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->pbase = Pbase + (cp-hardc)*8;	/* BUG!! guessing */
			setvec(Hardvec + (cp-hardc)*8, hardintr); /* BUG!! guessing */
		}
	}
}

void
hardinit(void)
{
}

/*
 *  Get the characteristics of each drive.  Mark unresponsive ones
 *  off line.
 */
Chan*
hardattach(char *spec)
{
	Drive *dp;

	for(dp = hard; dp < &hard[conf.nhard]; dp++){
		if(waserror()){
			dp->online = 0;
			qunlock(dp);
			continue;
		}
		qlock(dp);
		if(!dp->online){
			hardparams(dp);
			dp->online = 1;
			hardsetbuf(dp, 1);
		}

		/*
		 *  read Plan 9 partition table
		 */
		hardpart(dp);
		qunlock(dp);
		poperror();
	}
	return devattach('w', spec);
}

Chan*
hardclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
hardwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, hardgen);
}

void
hardstat(Chan *c, char *dp)
{
	devstat(c, dp, 0, 0, hardgen);
}

Chan*
hardopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, 0, hardgen);
}

void
hardcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
hardclose(Chan *c)
{
	USED(c);
}

void
hardremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
hardwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

long
hardread(Chan *c, void *a, long n, ulong offset)
{
	Drive *dp;
	long rv, i;
	int skip;
	uchar *aa = a;
	Partition *pp;
	char *buf;

	if(c->qid.path == CHDIR)
		return devdirread(c, a, n, 0, 0, hardgen);

	buf = smalloc(Maxxfer);
	if(waserror()){
		free(buf);
		nexterror();
	}

	dp = &hard[DRIVE(c->qid.path)];
	pp = &dp->p[PART(c->qid.path)];

	skip = offset % dp->bytes;
	for(rv = 0; rv < n; rv += i){
		i = hardxfer(dp, pp, Cread, offset+rv-skip, n-rv+skip, buf);
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
hardwrite(Chan *c, void *a, long n, ulong offset)
{
	Drive *dp;
	long rv, i, partial;
	uchar *aa = a;
	Partition *pp;
	char *buf;

	if(c->qid.path == CHDIR)
		error(Eisdir);

	dp = &hard[DRIVE(c->qid.path)];
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
		hardxfer(dp, pp, Cread, offset-partial, dp->bytes, buf);
		if(partial+n > dp->bytes)
			rv = dp->bytes - partial;
		else
			rv = n;
		memmove(buf+partial, aa, rv);
		hardxfer(dp, pp, Cwrite, offset-partial, dp->bytes, buf);
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
		i = hardxfer(dp, pp, Cwrite, offset+rv, i, buf);
		if(i == 0)
			break;
	}

	/*
	 *  if not ending on a sector boundary,
	 *  read in the last sector before writing
	 *  it out.
	 */
	if(partial){
		hardxfer(dp, pp, Cread, offset+rv, dp->bytes, buf);
		memmove(buf, aa+rv, partial);
		hardxfer(dp, pp, Cwrite, offset+rv, dp->bytes, buf);
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
 *  wait for the controller to be ready to accept a command
 */
static void
cmdreadywait(Controller *cp)
{
	long start;

	start = m->ticks;
	while((inb(cp->pbase+Pstatus) & (Sready|Sbusy)) != Sready)
		if(TK2MS(m->ticks - start) > 5){
			DPRINT("cmdreadywait failed\n");
			error(Eio);
		}
}

static void
hardrepl(Drive *dp, long bblk)
{
	int i;

	if(dp->repl.p == 0)
		return;
	for(i = 0; i < dp->repl.nrepl; i++){
		if(dp->repl.blk[i] == bblk)
			DPRINT("found bblk %ld at offset %ld\n", bblk, i);
	}
}

/*
 *  transfer a number of sectors.  hardintr will perform all the iterative
 *  parts.
 */
static long
hardxfer(Drive *dp, Partition *pp, int cmd, long start, long len, char *buf)
{
	Controller *cp;
	long lblk;
	int cyl, sec, head;
	int loop, s;

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
	cyl = lblk/(dp->sectors*dp->heads);
	sec = (lblk % dp->sectors) + 1;
	head =  ((lblk/dp->sectors) % dp->heads);

	cp = dp->cp;
	qlock(cp);
	if(waserror()){
		cp->buf = 0;
		qunlock(cp);
		nexterror();
	}

	cmdreadywait(cp);

	/*
	 *  splhi to make command atomic
	 */
	s = splhi();
	cp->sofar = 0;
	cp->buf = buf;
	cp->nsecs = len;
	cp->cmd = cmd;
	cp->dp = dp;
	cp->status = 0;
	outb(cp->pbase+Pcount, cp->nsecs);
	outb(cp->pbase+Psector, sec);
	outb(cp->pbase+Pdh, 0x20 | (dp->drive<<4) | head);
	outb(cp->pbase+Pcyllsb, cyl);
	outb(cp->pbase+Pcylmsb, cyl>>8);
	outb(cp->pbase+Pcmd, cmd);
	if(cmd == Cwrite){
		loop = 0;
		while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
			if(++loop > 10000)
				panic("hardxfer");
		outss(cp->pbase+Pdata, cp->buf, dp->bytes/2);
	}
	splx(s);

	/*
	 *  wait for command to complete.  if we get a note,
	 *  remember it but keep waiting to let the disk finish
	 *  the current command.
	 */
	loop = 0;
	while(waserror()){
		DPRINT("interrupted hardxfer\n");
		if(loop++ > 10){
			print("hard disk error\n");
			nexterror();
		}
	}
	sleep(&cp->r, cmddone, cp);
	poperror();
	if(loop)
		nexterror();

	if(cp->status & Serr){
		DPRINT("hd%d err: lblk %ld status %lux, err %lux\n",
			dp-hard, lblk, cp->status, cp->error);
		DPRINT("\tcyl %d, sec %d, head %d\n", cyl, sec, head);
		DPRINT("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
		hardrepl(dp, lblk+cp->sofar);
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
hardsetbuf(Drive *dp, int on)
{
	Controller *cp = dp->cp;

	qlock(cp);
	if(waserror()){
		qunlock(cp);
		nexterror();
	}

	cmdreadywait(cp);

	cp->cmd = Csetbuf;
	outb(cp->pbase+Pprecomp, on ? 0xAA : 0x55);
	outb(cp->pbase+Pdh, 0x20 | (dp->drive<<4));
	outb(cp->pbase+Pcmd, Csetbuf);

	sleep(&cp->r, cmddone, cp);

/*	if(cp->status & Serr)
		DPRINT("hd%d setbuf err: status %lux, err %lux\n",
			dp-hard, cp->status, cp->error);/**/

	poperror();
	qunlock(cp);
}

/*
 *  ident sector from drive
 */
struct Ident
{
	ushort	magic;		/* drive type magic */
	ushort	lcyls;		/* logical number of cylinders */
	ushort	rcyl;		/* number of removable cylinders */
	ushort	lheads;		/* logical number of heads */
	ushort	b2t;		/* unformatted bytes/track */
	ushort	b2s;		/* unformated bytes/sector */
	ushort	ls2t;		/* logical sectors/track */
	ushort	gap;		/* bytes in inter-sector gaps */
	ushort	sync;		/* bytes in sync fields */
	ushort	magic2;		/* must be 0x0000 */
	ushort	serial[10];	/* serial number */
	ushort	type;		/* controller type (0x0003) */
	ushort	bsize;		/* buffer size/512 */
	ushort	ecc;		/* ecc bytes returned by read long */
	ushort	firm[4];	/* firmware revision */
	ushort	model[20];	/* model number */
	ushort	s2i;		/* number of sectors/interrupt */
	ushort	dwtf;		/* double word transfer flag */
	ushort	alernate;
	ushort	piomode;
	ushort	dmamode;
	ushort	reserved[76];
	ushort	ncyls;		/* native number of cylinders */
	ushort	nheads;		/* native number of heads, sectors */
	ushort	dlcyls;		/* default logical number of cyinders */
	ushort	dlheads;	/* default logical number of heads, sectors */
	ushort	interface;
	ushort	power;		/* 0xFFFF if power commands supported */
	ushort	flags;
	ushort	ageprog;	/* MSB = age, LSB = program */
	ushort	reserved2[120];
};

/*
 *  get parameters from the drive
 */
static void
hardident(Drive *dp)
{
	Controller *cp;
	char *buf;
	Ident *ip;
	int s;

	cp = dp->cp;
	buf = smalloc(Maxxfer);
	qlock(cp);
	if(waserror()){
		qunlock(cp);
		nexterror();
	}

	cmdreadywait(cp);

	s = splhi();
	cp->nsecs = 1;
	cp->sofar = 0;
	cp->cmd = Cident;
	cp->dp = dp;
	cp->buf = buf;
	outb(cp->pbase+Pdh, 0x20 | (dp->drive<<4));
	outb(cp->pbase+Pcmd, Cident);
	splx(s);

	sleep(&cp->r, cmddone, cp);
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
		tsleep(&cp->r, return0, 0, 10);

	dp->cyl = ip->lcyls;
	dp->heads = ip->lheads;
	dp->sectors = ip->ls2t;
	dp->cap = dp->bytes * dp->cyl * dp->heads * dp->sectors;
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
hardprobe(Drive *dp, int cyl, int sec, int head)
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

	cmdreadywait(cp);

	cp->cmd = Cread;
	cp->dp = dp;
	cp->status = 0;
	cp->nsecs = 1;
	cp->sofar = 0;

	outb(cp->pbase+Pcount, 1);
	outb(cp->pbase+Psector, sec+1);
	outb(cp->pbase+Pdh, 0x20 | head | (dp->drive<<4));
	outb(cp->pbase+Pcyllsb, cyl);
	outb(cp->pbase+Pcylmsb, cyl>>8);
	outb(cp->pbase+Pcmd, Cread);

	sleep(&cp->r, cmddone, cp);

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
hardparams(Drive *dp)
{
	int i, hi, lo;

	/*
	 *  first try the easy way, ask the drive and make sure it
	 *  isn't lying.
	 */
	dp->bytes = 512;
	hardident(dp);
	if(hardprobe(dp, dp->cyl-1, dp->sectors-1, dp->heads-1) == 0)
		return;

	/*
	 *  the drive lied, determine parameters by seeing which ones
	 *  work to read sectors.
	 */
	for(i = 0; i < 32; i++)
		if(hardprobe(dp, 0, 0, i) < 0)
			break;
	dp->heads = i;
	for(i = 0; i < 128; i++)
		if(hardprobe(dp, 0, i, 0) < 0)
			break;
	dp->sectors = i;
	for(i = 512; ; i += 512)
		if(hardprobe(dp, i, dp->sectors-1, dp->heads-1) < 0)
			break;
	lo = i - 512;
	hi = i;
	for(; hi-lo > 1;){
		i = lo + (hi - lo)/2;
		if(hardprobe(dp, i, dp->sectors-1, dp->heads-1) < 0)
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
hardreplinit(Drive *dp)
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
	hardxfer(dp, dp->repl.p, Cread, 0, dp->bytes, buf);
	buf[dp->bytes-1] = 0;

	/*
	 *  parse replacement table.
	 */
	n = getfields(buf, line, Nrepl+1, '\n');
	if(strncmp(line[0], REPLMAGIC, sizeof(REPLMAGIC)-1)){
		dp->repl.p = 0;
	} else {
		for(dp->repl.nrepl = 0, i = 1; i < n; i++, dp->repl.nrepl++){
			if(getfields(line[i], field, 1, ' ') != 1)
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
hardpart(Drive *dp)
{
	Partition *pp;
	char *line[Npart+1];
	char *field[3];
	ulong n;
	int i;
	char *buf;

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
	 *  read partition table from disk, null terminate
	 */
	hardxfer(dp, pp, Cread, 0, dp->bytes, buf);
	buf[dp->bytes-1] = 0;

	/*
	 *  parse partition table.
	 */
	n = getfields(buf, line, Npart+1, '\n');
	if(n && strncmp(line[0], PARTMAGIC, sizeof(PARTMAGIC)-1) == 0){
		for(i = 1; i < n; i++){
			pp++;
			if(getfields(line[i], field, 3, ' ') != 3)
				break;
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
	free(buf);
	poperror();

	if(dp->repl.p)
		hardreplinit(dp);
}

/*
 *  we get an interrupt for every sector transferred
 */
static void
hardintr(Ureg *ur)
{
	Controller *cp;
	Drive *dp;
	long loop;
	char *addr;

	USED(ur);

	/*
 	 *  BUG!! if there is ever more than one controller, we need a way to
	 *	  distinguish which interrupted
	 */
	cp = &hardc[0];
	dp = cp->dp;

	loop = 0;
	while((cp->status = inb(cp->pbase+Pstatus)) & Sbusy){
		if(++loop > 100) {
			DPRINT("cmd=%lux status=%lux\n",
				cp->cmd, inb(cp->pbase+Pstatus));
			panic("hardintr: wait busy");
		}
	}
	switch(cp->cmd){
	case Cwrite:
		if(cp->status & Serr){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->error = inb(cp->pbase+Perror);
			wakeup(&cp->r);
			return;
		}
		cp->sofar++;
		if(cp->sofar < cp->nsecs){
			loop = 0;
			while(((cp->status = inb(cp->pbase+Pstatus)) & Sdrq) == 0)
				if(++loop > 100) {
					DPRINT("cmd=%lux status=%lux\n",
						cp->cmd, inb(cp->pbase+Pstatus));
					panic("hardintr: write");
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
		while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
			if(++loop > 10000) {
				DPRINT("cmd=%lux status=%lux\n",
					cp->cmd, inb(cp->pbase+Pstatus));
				panic("hardintr: read/ident");
		}
		if(cp->status & Serr){
			cp->lastcmd = cp->cmd;
			cp->cmd = 0;
			cp->error = inb(cp->pbase+Perror);
			wakeup(&cp->r);
			return;
		}
		addr = cp->buf;
		if(addr){
			addr += cp->sofar*dp->bytes;
			inss(cp->pbase+Pdata, addr, dp->bytes/2);
		}
		cp->sofar++;
		if(cp->sofar > cp->nsecs)
			print("hardintr %d %d\n", cp->sofar, cp->nsecs);
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
}
