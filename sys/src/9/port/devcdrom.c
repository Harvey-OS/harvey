#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"

/*
 * CD-ROM.  Gutted version of devwren.
 */

#define DATASIZE	2048	/* max in a transfer */

typedef struct Drive		Drive;

enum {
	Ndisk=		8,	/* maximum disks */
	/* file types */
	Qdir=		0,
	Qdisk=		1,
};

struct Drive
{
	QLock;
	ulong		blocks;			/* blocks on disk */
	ulong		bsize;			/* bytes per block */
	int		drive;
};

static Drive	cdrom[Ndisk];

static void	cdromsize(int);
static long	cdromio(Chan*, char*, ulong, ulong);

static char	Ecap[] = "CD-ROM device capacity unknown";
static char	EnotCD[] = "not a CD-ROM device";

/*
 *  accepts [0-7].  lun is therefore always zero.
 */
static int
cdromdev(char *p)
{
	int drive;

	if(p == 0 || p[0] == '\0')
		error(Ebadarg);
	if(p[0] < '0' || p[0] > '7' || p[1])
		error(Ebadarg);
	drive = p[0] - '0';
	return (drive << 3) | 0;
}

static int
cdromgen(Chan *c, Dirtab *tab, long ntab, long s, Dir *dirp)
{
	char name[NAMELEN+4];
	Drive *dp;

	USED(tab, ntab);
	if(s != 0)
		return -1;
	sprint(name, "cd%d", c->dev>>3);
	dp = &cdrom[c->dev>>3];
	devdir(c, (Qid){Qdisk, 0}, name, dp->blocks*dp->bsize, eve, 0444, dirp);
	return 1;
}

void
cdromreset(void)
{
}

void
cdrominit(void)
{
}

/*
 *  param is #R<target>
 */
Chan*
cdromattach(char *param)
{
	Chan *c;
	int drive;

	drive = cdromdev(param);
	cdromsize(drive);
	c = devattach('R', param);
	c->dev = drive;
	return c;
}

Chan*
cdromclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
cdromwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, cdromgen);
}

void
cdromstat(Chan *c, char *dp)
{
	devstat(c, dp, 0, 0, cdromgen);
}

Chan*
cdromopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, 0, cdromgen);
}

void
cdromcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
cdromclose(Chan *c)
{
	USED(c);
}

void
cdromremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
cdromwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

long
cdromread(Chan *c, char *a, long n, ulong offset)
{
	if(c->qid.path == CHDIR)
		return devdirread(c, a, n, 0, 0, cdromgen);
	return cdromio(c, a, n, offset);
}

long
cdromwrite(Chan *c, char *a, long n, ulong offset)
{
	USED(c, a, n, offset);
	error(Eperm);
	return 0;
}

static long
cdromio(Chan *c, char *a, ulong len, ulong offset)
{
	Drive *d;
	Scsibuf *b;
	ulong block, n, max, x;

	d = &cdrom[c->dev>>3];

	block = offset / d->bsize;
	n = (offset + len + d->bsize - 1) / d->bsize - block;
	max = DATASIZE / d->bsize;
	if(n > max)
		n = max;
	if(block + n > d->blocks)
		n = d->blocks - block;
	if(block >= d->blocks || n == 0)
		return 0;
	b = scsibuf();
	if(waserror()){
		scsifree(b);
		nexterror();
	}
	offset %= d->bsize;
	x = scsibread(d->drive, b, n, d->bsize, block);
	if(x < offset)
		len = 0;
	else if(len > x - offset)
		len = x - offset;
	memmove(a, (char*)b->virt + offset, len);
	poperror();
	scsifree(b);
	return len;
}

/*
 *  size disk
 */
static void
cdromsize(int dev)
{
	Drive *dp;
	uchar buf[128];
	long nb, bs;

	dp = &cdrom[dev>>3];
	if(waserror()){
		qunlock(dp);
		nexterror();
	}
	qlock(dp);
	scsiready(dev);
	scsisense(dev, buf);
	/*
	 * capacity not defined on CD-ROM, but most implement it.
	 */
	if(scsicap(dev, buf)!=0)
		error(Ecap);
	nb = (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|(buf[3]<<0);
	bs = (buf[4]<<24)|(buf[5]<<16)|(buf[6]<<8)|(buf[7]<<0);
	if(nb==0 || bs==0)
		error(Ecap);

	scsiinquiry(dev, buf, sizeof buf);
	dp->drive = dev;
	dp->blocks = nb;
	dp->bsize = bs;
	poperror();
	qunlock(dp);
}
