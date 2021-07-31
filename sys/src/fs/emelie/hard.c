#include "all.h"
#include "io.h"
#include "mem.h"

typedef	struct Disc		Disc;
typedef	struct Ident		Ident;
typedef	struct Controller	Controller;

enum {
	Maxxfer		= 16*1024,	/* maximum transfer size/cmd */
	Npart		= 8+2,		/* 8 sub partitions, disk, and partition */
};

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

	Timeout=	10,		/* seconds to wait for things to complete */
};

/*
 *  ident sector from drive
 */
struct Ident
{
	ushort	magic;		/* must be 0x0A5A */
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

typedef struct {
	ulong	start;
	ulong	end;
	char	name[NAMELEN+1];
} Partition;

/*
 *  a hard drive
 */
struct Disc
{
	Controller *cp;
	int	drive;
	int	confused;	/* needs to be recalibrated (or worse) */

	int	online;
	int	npart;		/* number of real partitions */
	Partition p[Npart];
	ulong	offset;
	Partition *current;	/* current partition */

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
	Disc	*dp;		/* drive being accessed */
};

Controller	*hardc;
Disc		*hard;

static void	hardintr(Ureg*, void*);
static long	hardxfer(Disc*, Partition*, int, long, long);
static int	hardident(Disc*);
static void	hardsetbuf(Disc*, int);
static void	hardpart(Disc*);
static int	hardparams(Disc*);
static void	hardrecal(Disc*);
static int	hardprobe(Disc*, int, int, int);

/*
 *  we assume drives 0 and 1 are on the first controller, 2 and 3 on the
 *  second, etc.  Discover drive parameters.  BUG! we are only guessing about
 *  the port locations for disks other than 0 and 1.
 */
int
hardinit(void)
{
	Disc *dp;
	Controller *cp;
	int drive;
	int disks, nhard;
	uchar equip;

	/*
	 *  read nvram for number of hard drives (2 max)
	 */
	nhard = 2;
	equip = nvramread(0x12);
	if((equip>>4) == 0)
		nhard = 0;
	if(nhard > 1 && (equip&0xf) == 0)
		nhard = 1;
	if(nhard == 0)
		return 0;

	disks = 0;

	hard = ialloc(nhard * sizeof(Disc), 0);
	hardc = ialloc(((nhard+1)/2) * sizeof(Controller), 0);
	
	for(drive = 0; drive < nhard; drive++){
		dp = &hard[drive];
		cp = &hardc[drive/2];
		dp->drive = drive&1;
		dp->online = 0;
		dp->cp = cp;
		if((drive&1) == 0){
			cp->buf = ialloc(Maxxfer, 0);
			cp->cmd = 0;
			cp->pbase = Pbase + (cp-hardc)*8;
			/*
			 *  clear any pending intr from drive
			 */
			inb(cp->pbase+Pstatus);
			setvec(Hardvec + (cp-hardc)*8, hardintr, cp);
		}
	}

	for(dp = hard; dp < &hard[nhard]; dp++){
		hardsetbuf(dp, 0);
		if(hardparams(dp) == 0){
print("hd%d: %d cylinders %d heads %d sectors/track %d bytes\n", disks,
	dp->cyl, dp->heads, dp->sectors, dp->cap);
			dp->online = 1;
			hardpart(dp);
			hardsetbuf(dp, 1);
			disks++;
		} else
			dp->online = 0;
	}
	return disks;
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
		print("hardwait timed out %ux\n", inb(cp->pbase+Pstatus));
		hardintr(0, cp);
	}
	splx(x);
}

int
sethardpart(int dev, char *p)
{
	Partition *pp;
	Disc *dp;

	dp = &hard[dev];
	for(pp = dp->p; pp < &dp->p[dp->npart]; pp++)
		if(strcmp(pp->name, p) == 0){
			dp->current = pp;
			return 1;
		}
	return 0;
}

long
hardread(int dev, void *a, long n)
{
	Disc *dp;
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

long
hardwrite(int dev, void *a, long n)
{
	Disc *dp;
	long rv, i, partial;
	uchar *aa = a;
	Partition *pp;
	Controller *cp;

	dp = &hard[dev];
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
		hardxfer(dp, pp, Cread, dp->offset-partial, dp->bytes);
		if(partial+n > dp->bytes)
			rv = dp->bytes - partial;
		else
			rv = n;
		memmove(cp->buf+partial, aa, rv);
		if(hardxfer(dp, pp, Cwrite, dp->offset-partial, dp->bytes) < 0)
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
		i = hardxfer(dp, pp, Cwrite, dp->offset+rv, i);
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
		if(hardxfer(dp, pp, Cread, dp->offset+rv, dp->bytes) < 0)
			return -1;
		memmove(cp->buf, aa+rv, partial);
		if(hardxfer(dp, pp, Cwrite, dp->offset+rv, dp->bytes) < 0)
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
cmdreadywait(Controller *cp)
{
	long start;

	start = m->ticks;
	while((inb(cp->pbase+Pstatus) & (Sready|Sbusy)) != Sready)
		if(TK2MS(m->ticks - start) > Timeout){
			/*print("cmdreadywait failed 0x%lux\n", inb(cp->pbase+Pstatus));/**/
			return -1;
		}
	return 0;
}

/*
 *  transfer a number of sectors.  hardintr will perform all the iterative
 *  parts.
 */
static long
hardxfer(Disc *dp, Partition *pp, int cmd, long start, long len)
{
	Controller *cp;
	long lsec;
	int loop;

	if(dp->online == 0){
		print("disk not on line\n");
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
		print("read past end of partition\n");
		return 0;
	}
	cp->tcyl = lsec/(dp->sectors*dp->heads);
	cp->tsec = (lsec % dp->sectors) + 1;
	cp->thead = (lsec/dp->sectors) % dp->heads;

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
/*print("xfer:\ttcyl %d, tsec %d, thead %d\n", cp->tcyl, cp->tsec, cp->thead);
print("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);/**/
	outb(cp->pbase+Pcount, cp->nsecs);
	outb(cp->pbase+Psector, cp->tsec);
	outb(cp->pbase+Pdh, Dmagic | (dp->drive<<4) | cp->thead);
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
print("hd%d err: status %lux, err %lux\n", dp-hard, cp->status, cp->error);
print("\ttcyl %d, tsec %d, thead %d\n", cp->tcyl, cp->tsec, cp->thead);
print("\tnsecs %d, sofar %d\n", cp->nsecs, cp->sofar);
		return -1;
	}

	return cp->nsecs*dp->bytes;
}

/*
 *  set read ahead mode (1 == on, 0 == off)
 */
static void
hardsetbuf(Disc *dp, int on)
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
hardident(Disc *dp)
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
	dp->cyl = ip->lcyls;
	dp->heads = ip->lheads;
	dp->sectors = ip->ls2t;
	dp->cap = dp->bytes * dp->cyl * dp->heads * dp->sectors;

	return 0;
}

/*
 *  probe the given sector to see if it exists
 */
static int
hardprobe(Disc *dp, int cyl, int sec, int head)
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
	outb(cp->pbase+Pdh, Dmagic | (dp->drive<<4) | head);
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
hardparams(Disc *dp)
{
	int i, hi, lo;

	/*
	 *  first try the easy way, ask the drive and make sure it
	 *  isn't lying.
	 */
	dp->bytes = 512;
	if(hardident(dp) < 0)
		return -1;
	if(hardprobe(dp, dp->cyl-1, dp->sectors-1, dp->heads-1) == 0)
		return 0;

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
hardpart(Disc *dp)
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
hardintr(Ureg *ur, void *arg)
{
	Controller *cp;
	Disc *dp;
	long loop;

	USED(ur);

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
				print("hardintr 2 cmd %ux status %ux", cp->cmd, inb(cp->pbase+Pstatus));
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
