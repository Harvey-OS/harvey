#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#define DPRINT if(0)print

typedef	struct Drive		Drive;
typedef	struct Ident		Ident;
typedef	struct Controller	Controller;

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

	/* file types */
	Qdir=		0,

	/* magic bit in drive head register */
	Dmagic=		(1<<5),

	Timeout=	4,		/* seconds to wait for things to complete */
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

/*
 *  a hard drive
 */
struct Drive
{
	Controller *cp;
	int	drive;
	int	confused;	/* needs to be recalibrated (or worse) */

	Disc;
};

/*
 *  a controller for 2 drives
 */
struct Controller
{
	int	confused;	/* needs to be recalibrated (or worse) */
	int	pbase;		/* base port */

	/*
	 *  current operation
	 */
	int	cmd;		/* current command */
	char	*buf;		/* xfer buffer */
	int	tcyl;		/* target cylinder */
	int	thead;		/* target head */
	int	tsec;		/* target sector */
	int	tbyte;		/* target byte */
	int	nsecs;		/* length of transfer (sectors) */
	int	sofar;		/* bytes transferred so far */
	int	status;
	int	error;
	Drive	*dp;		/* drive being accessed */
};

Controller	*hardc;
Drive		*hard;

static void	hardintr(Ureg*, void*);
static long	hardxfer(Drive*, Partition*, int, long, long);
static int	hardident(Drive*);
static void	hardsetbuf(Drive*, int);
static void	hardpart(Drive*);
static int	hardparams(Drive*);
static void	hardrecal(Drive*);
static int	hardprobe(Drive*, int, int, int);

/*
 *  we assume drives 0 and 1 are on the first controller, 2 and 3 on the
 *  second, etc.  Discover drive parameters.  BUG! we are only guessing about
 *  the port locations for disks other than 0 and 1.
 */
int
hardinit(void)
{
	Drive *dp;
	Controller *cp;
	uchar equip;
	int mask, nhard;

	equip = nvramread(0x12);
	if(equip == 0)
		equip = 0x10;		/* the Globalyst 250 lies */

	hard = ialloc(2 * sizeof(Drive), 0);
	hardc = ialloc(sizeof(Controller), 0);

	cp = hardc;
	cp->buf = ialloc(Maxxfer, 0);
	cp->cmd = 0;
	cp->pbase = Pbase;
	/*
	 *  clear any pending intr from drive
	 */
	inb(cp->pbase+Pstatus);
	setvec(Hardvec, hardintr, cp);
	
	dp = hard;
	if(equip & 0xf0){
		dp->drive = 0;
		dp->online = 0;
		dp->cp = cp;
		dp++;
	}
	if(equip & 0x0f){
		dp->drive = 1;
		dp->online = 0;
		dp->cp = cp;
		dp++;
	}
	nhard = dp-hard;

	mask = 0;
	for(dp = hard; dp < &hard[nhard]; dp++){
		if(hardparams(dp) == 0){
			print("hd%d: %d cylinders %d heads %d sectors/track %d bytes\n",
				dp->drive, dp->cyl, dp->heads, dp->sectors, dp->cap);
			dp->online = 1;
			hardpart(dp);
			hardsetbuf(dp, 1);
			mask |= 1<<dp->drive;
		} else
			dp->online = 0;
	}
	return mask;
}

long
hardseek(int dev, long off)
{
	hard[dev].offset = off;
	return off;
}

/*
 *  did an interrupt happen?
 */
static void
hardwait(Controller *cp)
{
	ulong start;
	int x;

	x = spllo();
	for(start = m->ticks; TK2SEC(m->ticks - start) < Timeout && cp->cmd;)
		if(cp->cmd == Cident2 && TK2SEC(m->ticks - start) >= 1)
			break;
	if(TK2SEC(m->ticks - start) >= Timeout){
		DPRINT("hardwait timed out %ux\n", inb(cp->pbase+Pstatus));
		hardintr(0, cp);
	}
	splx(x);
}

Partition*
sethardpart(int dev, char *p)
{
	Partition *pp;
	Drive *dp;

	dp = &hard[dev];
	for(pp = dp->p; pp < &dp->p[dp->npart]; pp++)
		if(strcmp(pp->name, p) == 0){
			dp->current = pp;
			return pp;
		}
	return 0;
}

long
hardread(int dev, void *a, long n)
{
	Drive *dp;
	long rv, i;
	int skip;
	uchar *aa = a;
	Partition *pp;
	Controller *cp;

	dp = &hard[dev];
	pp = dp->current;
	if(pp == 0)
		return -1;
	cp = dp->cp;

	skip = dp->offset % dp->bytes;
	for(rv = 0; rv < n; rv += i){
		i = hardxfer(dp, pp, Cread, dp->offset+rv-skip, n-rv+skip);
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

/*
 *  wait for the controller to be ready to accept a command
 */
static int
cmdreadywait(Controller *cp)
{
	long start;

	start = m->ticks;
	while((inb(cp->pbase+Pstatus) & (Sready|Sbusy)) != Sready)
		if(TK2MS(m->ticks - start) > Timeout){
			DPRINT("cmdreadywait failed 0x%lux\n", inb(cp->pbase+Pstatus));
			return -1;
		}
	return 0;
}

/*
 *  transfer a number of sectors.  hardintr will perform all the iterative
 *  parts.
 */
static long
hardxfer(Drive *dp, Partition *pp, int cmd, long start, long len)
{
	Controller *cp;
	long lsec;
	int loop;

	if(dp->online == 0){
		DPRINT("disk not on line\n");
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
		DPRINT("read past end of partition\n");
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

	if(cmdreadywait(cp) < 0)
		return -1;

	/*
	 *  start the transfer
	 */
	cp->cmd = cmd;
	cp->dp = dp;
	cp->sofar = 0;
	cp->status = 0;
	DPRINT("xfer:\ttcyl %d, tsec %d, thead %d\n", cp->tcyl, cp->tsec, cp->thead);
	DPRINT("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
	outb(cp->pbase+Pcount, cp->nsecs);
	outb(cp->pbase+Psector, cp->tsec);
	outb(cp->pbase+Pdh, Dmagic | (dp->drive<<4) | (dp->lba<<6) | cp->thead);
	outb(cp->pbase+Pcyllsb, cp->tcyl);
	outb(cp->pbase+Pcylmsb, cp->tcyl>>8);
	outb(cp->pbase+Pcmd, cmd);

	if(cmd == Cwrite){
		loop = 0;
		while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
			if(++loop > 10000)
				panic("hardxfer");
		outss(cp->pbase+Pdata, cp->buf, dp->bytes/2);
	}

	hardwait(cp);

	if(cp->status & Serr){
		DPRINT("hd%d err: status %lux, err %lux\n",
			dp-hard, cp->status, cp->error);
		DPRINT("\ttcyl %d, tsec %d, thead %d\n",
			cp->tcyl, cp->tsec, cp->thead);
		DPRINT("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
		return -1;
	}

	return cp->nsecs*dp->bytes;
}

/*
 *  set read ahead mode (1 == on, 0 == off)
 */
static void
hardsetbuf(Drive *dp, int on)
{
	Controller *cp = dp->cp;

	if(cmdreadywait(cp) < 0)
		return;

	cp->cmd = Csetbuf;
	/* BUG: precomp varies by hard drive...this is safari-specific? */
	outb(cp->pbase+Pprecomp, on ? 0xAA : 0x55);
	outb(cp->pbase+Pdh, Dmagic | (dp->drive<<4));
	outb(cp->pbase+Pcmd, Csetbuf);

	hardwait(cp);
}

/*
 *  get parameters from the drive
 */
static int
hardident(Drive *dp)
{
	Controller *cp;
	Ident *ip;

	dp->bytes = 512;
	cp = dp->cp;

	if(cmdreadywait(cp) < 0)
		return -1;

	cp->nsecs = 1;
	cp->sofar = 0;
	cp->cmd = Cident;
	cp->dp = dp;
	outb(cp->pbase+Pdh, Dmagic | (dp->drive<<4));
	outb(cp->pbase+Pcmd, Cident);

	hardwait(cp);

	if(cp->status & Serr)
		return -1;
	
	hardwait(cp);

	ip = (Ident*)cp->buf;
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

	return 0;
}

/*
 *  probe the given sector to see if it exists
 */
static int
hardprobe(Drive *dp, int cyl, int sec, int head)
{
	Controller *cp;

	cp = dp->cp;
	if(cmdreadywait(cp) < 0)
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
	outb(cp->pbase+Pdh, Dmagic | head | (dp->lba<<6) | (dp->drive<<4));
	outb(cp->pbase+Pcyllsb, cyl);
	outb(cp->pbase+Pcylmsb, cyl>>8);
	outb(cp->pbase+Pcmd, Cread);

	hardwait(cp);

	if(cp->status & Serr)
		return -1;

	return 0;
}

/*
 *  figure out the drive parameters
 */
static int
hardparams(Drive *dp)
{
	int i, hi, lo;

	/*
	 *  first try the easy way, ask the drive and make sure it
	 *  isn't lying.
	 */
	dp->bytes = 512;
	if(hardident(dp) < 0)
		return -1;
	if(dp->lba){
		i = dp->sectors - 1;
		if(hardprobe(dp, (i>>8)&0xffff, (i&0xff)-1, (i>>24)&0xf) == 0)
			return 0;
	} else {
		if(hardprobe(dp, dp->cyl-1, dp->sectors-1, dp->heads-1) == 0)
			return 0;
	}

	DPRINT("hardparam: cyl %d sectors %d heads %d\n", dp->cyl, dp->sectors, dp->heads);
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

	if(dp->cyl == 0 || dp->heads == 0 || dp->sectors == 0 || dp->cap == 0)
		return -1;

	return 0;
}

/*
 *  read partition table.  The partition table is just ascii strings.
 */
#define MAGIC "plan9 partitions"
static void
hardpart(Drive *dp)
{
	Partition *pp;
	Controller *cp;
	char *line[Npart+1];
	char *field[3];
	ulong n;
	int i;

	cp = dp->cp;

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
	 *  read last sector from disk, null terminate.  This used
	 *  to be the sector we used for the partition tables.
	 *  However, this sector is special on some PC's so we've
	 *  started to use the second last sector as the partition
	 *  table instead.  To avoid reconfiguring all our old systems
	 *  we first look to see if there is a valid partition
	 *  table in the last sector.  If so, we use it.  Otherwise
	 *  we switch to the second last.
	 */
	hardxfer(dp, pp, Cread, 0, dp->bytes);
	cp->buf[dp->bytes-1] = 0;
	n = getfields(cp->buf, line, Npart+1, '\n');
	if(n == 0 || strncmp(line[0], MAGIC, sizeof(MAGIC)-1)){
		dp->p[0].end--;
		dp->p[1].start--;
		dp->p[1].end--;
		hardxfer(dp, pp, Cread, 0, dp->bytes);
		cp->buf[dp->bytes-1] = 0;
		n = getfields(cp->buf, line, Npart+1, '\n');
	}

	/*
	 *  parse partition table.
	 */
	if(n && strncmp(line[0], MAGIC, sizeof(MAGIC)-1) == 0){
		for(i = 1; i < n; i++){
			pp++;
			if(getfields(line[i], field, 3, ' ') != 3)
				break;
			strncpy(pp->name, field[0], NAMELEN);
			pp->start = strtoul(field[1], 0, 0);
			pp->end = strtoul(field[2], 0, 0);
			if(pp->start > pp->end || pp->start >= dp->p[0].end)
				break;
			dp->npart++;
		}
	}
	return;
}

/*
 *  we get an interrupt for every sector transferred
 */
static void
hardintr(Ureg*, void *arg)
{
	Controller *cp;
	Drive *dp;
	long loop;

	cp = arg;
	dp = cp->dp;

	loop = 0;
	while((cp->status = inb(cp->pbase+Pstatus)) & Sbusy)
		if(++loop > 100000){
			print("hardintr 0x%lux\n", cp->status);
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
					print("hardintr 1");
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
		if(cp->status & Serr){
			cp->cmd = 0;
			cp->error = inb(cp->pbase+Perror);
			return;
		}
		loop = 0;
		while((inb(cp->pbase+Pstatus) & Sdrq) == 0)
			if(++loop > 100000){
				print("hardintr 2 cmd %ux status %ux",
					cp->cmd, inb(cp->pbase+Pstatus));
				cp->cmd = 0;
				return;
			}
		inss(cp->pbase+Pdata, &cp->buf[cp->sofar*dp->bytes],
			dp->bytes/2);
		cp->sofar++;
		if(cp->sofar >= cp->nsecs){
			if (cp->cmd == Cread)
				cp->cmd = 0;
			else
				cp->cmd = Cident2; /* sometimes we get a second intr */
		}
		break;
	case Csetbuf:
	case Cident2:
		cp->cmd = 0;
		break;
	default:
		cp->cmd = 0;
		break;
	}
}
