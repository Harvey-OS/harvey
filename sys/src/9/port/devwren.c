#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"

#define DATASIZE	(8*1024)

typedef struct Partition	Partition;
typedef struct Drive		Drive;

enum {
	Npart=		8+2,	/* 8 sub partitions, disk, and partition */
	Ndisk=		64,	/* maximum disks; if you change it, you must
				   map from dev to disk */

	/* file types */
	Qdir=		0,
};
#define PART(x)		((x)&0xF)
#define DRIVE(x)	(((x)>>4)&(Ndisk-1))
#define MKQID(d,p)	(((d)<<4) | (p))

struct Partition
{
	ulong	start;
	ulong	end;
	char	name[NAMELEN+1];
};

struct Drive
{
	QLock;
	ulong		bytes;			/* bytes per block */
	int		npart;			/* actual number of partitions */
	int		drive;
	int		readonly;
	Partition	p[Npart];
};

static Drive	wren[Ndisk];

static void	wrenpart(int);
static long	wrenio(Chan*, int, char*, ulong, ulong);

/*
 *  accepts [0-7].[0-7], or abbreviation
 */
static int
wrendev(char *p)
{
	int drive, unit;

	if(p == 0 || p[0] == '\0')
		return 0;
	if(p[0] < '0' || p[0] > '7')
		error(Ebadarg);
	drive = p[0] - '0';
	unit = 0;
	if(p[1]){
		if(p[1] != '.' || p[2] < '0' || p[2] > '7' || p[3] != '\0')
			error(Ebadarg);
		unit = p[2] - '0';
	}
	return (drive << 3) | unit;
}

static int
wrengen(Chan *c, Dirtab *tab, long ntab, long s, Dir *dirp)
{
	Qid qid;
	int drive;
	char name[NAMELEN+4];
	Drive *dp;
	Partition *pp;
	ulong l;

	USED(tab, ntab);
	qid.vers = 0;
	drive = c->dev;
	if(drive >= Ndisk)
		return -1;
	dp = &wren[drive];

	if(s >= dp->npart)
		return -1;

	pp = &dp->p[s];
	if(drive & 7)
		sprint(name, "hd%d.%d%s", drive>>3, drive&7, pp->name);
	else
		sprint(name, "hd%d%s", drive>>3, pp->name);
	name[NAMELEN] = 0;
	qid.path = MKQID(drive, s);
	l = (pp->end - pp->start) * dp->bytes;
	devdir(c, qid, name, l, eve, dp->readonly ? 0444 : 0666, dirp);
	return 1;
}

void
wrenreset(void)
{
}

void
wreninit(void)
{
}

/*
 *  param is #w<target>.<lun>
 */
Chan *
wrenattach(char *param)
{
	Chan *c;
	int drive;

	drive = wrendev(param);
	wrenpart(drive);
	c = devattach('w', param);
	c->dev = drive;
	return c;
}

Chan*
wrenclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
wrenwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, wrengen);
}

void
wrenstat(Chan *c, char *dp)
{
	devstat(c, dp, 0, 0, wrengen);
}

Chan*
wrenopen(Chan *c, int omode)
{
	return devopen(c, omode, 0, 0, wrengen);
}

void
wrencreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
wrenclose(Chan *c)
{
	USED(c);
}

void
wrenremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
wrenwstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

long
wrenread(Chan *c, char *a, long n, ulong offset)
{
	if(c->qid.path == CHDIR)
		return devdirread(c, a, n, 0, 0, wrengen);
	return wrenio(c, 0, a, n, offset);
}

long
wrenwrite(Chan *c, char *a, long n, ulong offset)
{
	n = wrenio(c, 1, a, n, offset);
	if(n)
		return n;
	error("end of device");
	return 0;		/* not reached */
}

static long
wrenio(Chan *c, int write, char *a, ulong len, ulong offset)
{
	Drive *d;
	Partition *p;
	Scsibuf *b;
	ulong block, n, max, x;

	d = &wren[DRIVE(c->qid.path)];
	if(d->npart == 0)			/* drive repartitioned */
		error(Eio);
	p = &d->p[PART(c->qid.path)];

	block = offset / d->bytes + p->start;
	n = (offset + len + d->bytes - 1) / d->bytes + p->start - block;
	max = DATASIZE / d->bytes;
	if(n > max)
		n = max;
	if(block + n > p->end)
		n = p->end - block;
	if(block >= p->end || n == 0)
		return 0;
	b = scsibuf();
	if(waserror()){
		scsifree(b);
		nexterror();
	}
	offset %= d->bytes;
	if(write){
		if(offset || len % d->bytes){
			x = scsibread(d->drive, b, n, d->bytes, block);
			if(x < n * d->bytes){
				n = x / d->bytes;
				x = n * d->bytes - offset;
				if(len > x)
					len = x;
			}
		}
		memmove((char*)b->virt + offset, a, len);
		x = scsibwrite(d->drive, b, n, d->bytes, block);
		if(x < offset)
			len = 0;
		else if(len > x - offset)
			len = x - offset;
	}else{
		x = scsibread(d->drive, b, n, d->bytes, block);
		if(x < offset)
			len = 0;
		else if(len > x - offset)
			len = x - offset;
		memmove(a, (char*)b->virt + offset, len);
	}
	poperror();
	scsifree(b);
	return len;
}

/*
 *  read partition table.  The partition table is just ascii strings.
 */
#define MAGIC "plan9 partitions"
static void
wrenpart(int dev)
{
	Drive *dp;
	Partition *pp;
	uchar buf[128];
	Scsibuf *b;
	char *rawpart, *line[Npart+1], *field[3];
	ulong n;
	int i;

	dp = &wren[dev];
	if(waserror()){
		qunlock(dp);
		nexterror();
	}
	qlock(dp);
	scsiready(dev);
	scsisense(dev, buf);
	scsistartstop(dev, ScsiStartunit);
	scsisense(dev, buf);
	if(scsicap(dev, buf))
		error(Eio);
	dp->drive = dev;
	dp->readonly = scsiwp(dev);

	/*
	 *  we always have a partition for the whole disk
	 *  and one for the partition table
	 */
	dp->bytes = (buf[4]<<24)+(buf[5]<<16)+(buf[6]<<8)+(buf[7]);
	pp = &dp->p[0];
	strcpy(pp->name, "disk");
	pp->start = 0;
	pp->end = (buf[0]<<24)+(buf[1]<<16)+(buf[2]<<8)+(buf[3]) + 1;
	pp++;
	strcpy(pp->name, "partition");
	pp->start = dp->p[0].end - 1;
	pp->end = dp->p[0].end;

	scsiinquiry(dev, buf, sizeof buf);
	if(memcmp(&buf[8], "INSITE  I325VM        *F", 24) == 0)
		scsimodesense(dev, 0x2e, buf, 0x2a);
	/*
	 *  read partition table from disk, null terminate
	 */
	b = scsibuf();
	if(waserror()){
		scsifree(b);
		nexterror();
	}
	scsibread(dev, b, 1, dp->bytes, dp->p[0].end-1);
	rawpart = b->virt;
	rawpart[dp->bytes-1] = 0;

	/*
	 *  parse partition table.
	 */
	pp = &dp->p[2];
	n = getfields(rawpart, line, Npart+1, '\n');
	if(n > 0 && strncmp(line[0], MAGIC, sizeof(MAGIC)-1) == 0){
		for(i = 1; i < n; i++){
			if(getfields(line[i], field, 3, ' ') != 3)
				break;
			strncpy(pp->name, field[0], NAMELEN);
			pp->start = strtoul(field[1], 0, 0);
			pp->end = strtoul(field[2], 0, 0);
			if(pp->start > pp->end || pp->start >= dp->p[0].end)
				break;
			pp++;
		}
	}
	dp->npart = pp - dp->p;
	poperror();
	scsifree(b);
	poperror();
	qunlock(dp);
}
