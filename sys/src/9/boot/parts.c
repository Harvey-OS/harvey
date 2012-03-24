/*
 * read disk partition tables, intended for early use on systems
 * that don't use 9load.  borrowed from 9load.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>
#include "../boot/boot.h"

typedef struct Fs Fs;
#include "/sys/src/boot/pc/dosfs.h"

#define	GSHORT(p)	(((p)[1]<<8)|(p)[0])
#define	GLONG(p)	((GSHORT((p)+2)<<16)|GSHORT(p))

#define trace 0

enum {
	parttrace = 0,

	Npart = 64,
	SDnpart = Npart,

	Maxsec = 2048,
	Cdsec = 2048,
	Normsec = 512,			/* disks */

	NAMELEN = 256,			/* hack */
};

typedef struct SDpart SDpart;
typedef struct SDunit SDunit;

typedef struct SDpart {
	uvlong	start;
	uvlong	end;
	char	name[NAMELEN];
	int	valid;
} SDpart;

typedef struct SDunit {
	int	ctl;			/* fds */
	int	data;

	char	name[NAMELEN];

	uvlong	sectors;
	ulong	secsize;
	SDpart*	part;
	int	npart;			/* of valid partitions */
} SDunit;

static uchar *mbrbuf, *partbuf;

static void
sdaddpart(SDunit* unit, char* name, uvlong start, uvlong end)
{
	SDpart *pp;
	int i, partno;

	if(parttrace)
		print("add %d %s %s %lld %lld\n", unit->npart, unit->name, name, start, end);
	/*
	 * Check name not already used
	 * and look for a free slot.
	 */
	if(unit->part != nil){
		partno = -1;
		for(i = 0; i < SDnpart; i++){
			pp = &unit->part[i];
			if(!pp->valid){
				if(partno == -1)
					partno = i;
				break;
			}
			if(strcmp(name, pp->name) == 0){
				if(pp->start == start && pp->end == end){
					if(parttrace)
						print("already present\n");
					return;
				}
			}
		}
	}else{
		if((unit->part = malloc(sizeof(SDpart)*SDnpart)) == nil){
			if(parttrace)
				print("malloc failed\n");
			return;
		}
		partno = 0;
	}

	/*
	 * Check there is a free slot and size and extent are valid.
	 */
	if(partno == -1 || start > end || end > unit->sectors){
		print("cannot add %s!%s [%llud,%llud) to disk [0,%llud): %s\n",
			unit->name, name, start, end, unit->sectors, 
			partno==-1 ? "no free partitions" : "partition boundaries out of range");
		return;
	}
	pp = &unit->part[partno];
	pp->start = start;
	pp->end = end;
	strncpy(pp->name, name, NAMELEN);
	pp->valid = 1;
	unit->npart++;

	/* update devsd's in-memory partition table */
	if (fprint(unit->ctl, "part %s %lld %lld\n", name, start, end) < 0)
		fprint(2, "can't update %s's devsd partition table for %s: %r\n",
			unit->name, name);
	if (debugboot)
		print("part %s %lld %lld\n", name, start, end);
}

static long
sdread(SDunit *unit, SDpart *pp, void* va, long len, vlong off)
{
	long l, secsize;
	uvlong bno, nb;

	/*
	 * Check the request is within partition bounds.
	 */
	secsize = unit->secsize;
	if (secsize == 0)
		sysfatal("sdread: zero sector size");
	bno = off/secsize + pp->start;
	nb = (off+len+secsize-1)/secsize + pp->start - bno;
	if(bno+nb > pp->end)
		nb = pp->end - bno;
	if(bno >= pp->end || nb == 0)
		return 0;

	seek(unit->data, bno * secsize, 0);
	assert(va);				/* "sdread" */
	l = read(unit->data, va, len);
	if (l < 0)
		return 0;
	return l;
}

static int
sdreadblk(SDunit *unit, SDpart *part, void *a, vlong off, int mbr)
{
	uchar *b;

	assert(a);			/* sdreadblk */
	if(sdread(unit, part, a, unit->secsize, off) != unit->secsize){
		if(trace)
			print("%s: read %lud at %lld failed\n", unit->name,
				unit->secsize, (vlong)part->start*unit->secsize+off);
		return -1;
	}
	b = a;
	if(mbr && (b[0x1FE] != 0x55 || b[0x1FF] != 0xAA)){
		if(trace)
			print("%s: bad magic %.2ux %.2ux at %lld\n",
				unit->name, b[0x1FE], b[0x1FF],
				(vlong)part->start*unit->secsize+off);
		return -1;
	}
	return 0;
}

/*
 *  read partition table.  The partition table is just ascii strings.
 */
#define MAGIC "plan9 partitions"
static void
oldp9part(SDunit *unit)
{
	SDpart *pp;
	char *field[3], *line[Npart+1];
	ulong n;
	uvlong start, end;
	int i;

	/*
	 *  We have some partitions already.
	 */
	pp = &unit->part[unit->npart];

	/*
	 * We prefer partition tables on the second to last sector,
	 * but some old disks use the last sector instead.
	 */
	strcpy(pp->name, "partition");
	pp->start = unit->sectors - 2;
	pp->end = unit->sectors - 1;

	if(debugboot)
		print("oldp9part %s\n", unit->name);
	if(sdreadblk(unit, pp, partbuf, 0, 0) < 0)
		return;

	if(strncmp((char*)partbuf, MAGIC, sizeof(MAGIC)-1) != 0) {
		/* not found on 2nd last sector; look on last sector */
		pp->start++;
		pp->end++;
		if(sdreadblk(unit, pp, partbuf, 0, 0) < 0)
			return;
		if(strncmp((char*)partbuf, MAGIC, sizeof(MAGIC)-1) != 0)
			return;
		print("%s: using old plan9 partition table on last sector\n", unit->name);
	}else
		print("%s: using old plan9 partition table on 2nd-to-last sector\n", unit->name);

	/* we found a partition table, so add a partition partition */
	unit->npart++;
	partbuf[unit->secsize-1] = '\0';

	/*
	 * parse partition table
	 */
	n = gettokens((char*)partbuf, line, Npart+1, "\n");
	if(n && strncmp(line[0], MAGIC, sizeof(MAGIC)-1) == 0){
		for(i = 1; i < n && unit->npart < SDnpart; i++){
			if(gettokens(line[i], field, 3, " ") != 3)
				break;
			start = strtoull(field[1], 0, 0);
			end = strtoull(field[2], 0, 0);
			if(start >= end || end > unit->sectors)
				break;
			sdaddpart(unit, field[0], start, end);
		}
	}	
}

static SDpart*
sdfindpart(SDunit *unit, char *name)
{
	int i;

	if(parttrace)
		print("findpart %d %s %s: ", unit->npart, unit->name, name);
	for(i=0; i<unit->npart; i++) {
		if(parttrace)
			print("%s...", unit->part[i].name);
		if(strcmp(unit->part[i].name, name) == 0){
			if(parttrace)
				print("\n");
			return &unit->part[i];
		}
	}
	if(parttrace)
		print("not found\n");
	return nil;
}

/*
 * look for a plan 9 partition table on drive `unit' in the second
 * sector (sector 1) of partition `name'.
 * if found, add the partitions defined in the table.
 */
static void
p9part(SDunit *unit, char *name)
{
	SDpart *p;
	char *field[4], *line[Npart+1];
	uvlong start, end;
	int i, n;

	if(debugboot)
		print("p9part %s %s\n", unit->name, name);
	p = sdfindpart(unit, name);
	if(p == nil)
		return;

	if(sdreadblk(unit, p, partbuf, unit->secsize, 0) < 0)
		return;
	partbuf[unit->secsize-1] = '\0';

	if(strncmp((char*)partbuf, "part ", 5) != 0)
		return;

	n = gettokens((char*)partbuf, line, Npart+1, "\n");
	if(n == 0)
		return;
	for(i = 0; i < n && unit->npart < SDnpart; i++){
		if(strncmp(line[i], "part ", 5) != 0)
			break;
		if(gettokens(line[i], field, 4, " ") != 4)
			break;
		start = strtoull(field[2], 0, 0);
		end   = strtoull(field[3], 0, 0);
		if(start >= end || end > unit->sectors)
			break;
		sdaddpart(unit, field[1], p->start+start, p->start+end);
	}
}

static int
isdos(int t)
{
	return t==FAT12 || t==FAT16 || t==FATHUGE || t==FAT32 || t==FAT32X;
}

static int
isextend(int t)
{
	return t==EXTEND || t==EXTHUGE || t==LEXTEND;
}

/* 
 * Fetch the first dos and all plan9 partitions out of the MBR partition table.
 * We return -1 if we did not find a plan9 partition.
 */
static int
mbrpart(SDunit *unit)
{
	Dospart *dp;
	uvlong taboffset, start, end;
	uvlong firstxpart, nxtxpart;
	int havedos, i, nplan9;
	char name[10];

	taboffset = 0;
	dp = (Dospart*)&mbrbuf[0x1BE];
	{
		/* get the MBR (allowing for DMDDO) */
		if(sdreadblk(unit, &unit->part[0], mbrbuf,
		    (vlong)taboffset * unit->secsize, 1) < 0)
			return -1;
		for(i=0; i<4; i++)
			if(dp[i].type == DMDDO) {
				if(trace)
					print("DMDDO partition found\n");
				taboffset = 63;
				if(sdreadblk(unit, &unit->part[0], mbrbuf,
				    (vlong)taboffset * unit->secsize, 1) < 0)
					return -1;
				i = -1;	/* start over */
			}
	}

	/*
	 * Read the partitions, first from the MBR and then
	 * from successive extended partition tables.
	 */
	nplan9 = 0;
	havedos = 0;
	firstxpart = 0;
	for(;;) {
		if(sdreadblk(unit, &unit->part[0], mbrbuf,
		    (vlong)taboffset * unit->secsize, 1) < 0)
			return -1;
		if(trace) {
			if(firstxpart)
				print("%s ext %llud ", unit->name, taboffset);
			else
				print("%s mbr ", unit->name);
		}
		nxtxpart = 0;
		for(i=0; i<4; i++) {
			if(trace)
				print("dp %d...", dp[i].type);
			start = taboffset+GLONG(dp[i].start);
			end = start+GLONG(dp[i].len);

			if(dp[i].type == PLAN9) {
				if(nplan9 == 0)
					strcpy(name, "plan9");
				else
					sprint(name, "plan9.%d", nplan9);
				sdaddpart(unit, name, start, end);
				p9part(unit, name);
				nplan9++;
			}

			/*
			 * We used to take the active partition (and then the first
			 * when none are active).  We have to take the first here,
			 * so that the partition we call ``dos'' agrees with the
			 * partition disk/fdisk calls ``dos''. 
			 */
			if(havedos==0 && isdos(dp[i].type)){
				havedos = 1;
				sdaddpart(unit, "dos", start, end);
			}

			/* nxtxpart is relative to firstxpart (or 0), not taboffset */
			if(isextend(dp[i].type)){
				nxtxpart = start-taboffset+firstxpart;
				if(trace)
					print("link %llud...", nxtxpart);
			}
		}
		if(trace)
			print("\n");

		if(!nxtxpart)
			break;
		if(!firstxpart)
			firstxpart = nxtxpart;
		taboffset = nxtxpart;
	}	
	return nplan9 ? 0 : -1;
}

/*
 * To facilitate booting from CDs, we create a partition for
 * the boot floppy image embedded in a bootable CD.
 */
static int
part9660(SDunit *unit)
{
	uchar buf[Maxsec];
	ulong a, n;
	uchar *p;

	if(unit->secsize != Cdsec)
		return -1;

	if(sdread(unit, &unit->part[0], buf, Cdsec, 17*Cdsec) < 0)
		return -1;

	if(buf[0] || strcmp((char*)buf+1, "CD001\x01EL TORITO SPECIFICATION") != 0)
		return -1;

	
	p = buf+0x47;
	a = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);

	if(sdread(unit, &unit->part[0], buf, Cdsec, a*Cdsec) < 0)
		return -1;

	if(memcmp(buf, "\x01\x00\x00\x00", 4) != 0
	|| memcmp(buf+30, "\x55\xAA", 2) != 0
	|| buf[0x20] != 0x88)
		return -1;

	p = buf+0x28;
	a = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);

	switch(buf[0x21]){
	case 0x01:
		n = 1200*1024;
		break;
	case 0x02:
		n = 1440*1024;
		break;
	case 0x03:
		n = 2880*1024;
		break;
	default:
		return -1;
	}
	n /= Cdsec;

	print("found partition %s!cdboot; %lud+%lud\n", unit->name, a, n);
	sdaddpart(unit, "cdboot", a, a+n);
	return 0;
}

enum {
	NEW = 1<<0,
	OLD = 1<<1
};

/*
 * read unit->data to look for partition tables.
 * if found, stash partitions in environment and write them to ctl too.
 */
static void
partition(SDunit *unit)
{
	int type;
	char *p;

	if(unit->part == 0)
		return;

	if(part9660(unit) == 0)
		return;

	p = getenv("partition");
	if(p != nil && strncmp(p, "new", 3) == 0)
		type = NEW;
	else if(p != nil && strncmp(p, "old", 3) == 0)
		type = OLD;
	else
		type = NEW|OLD;

	if(mbrbuf == nil) {
		mbrbuf = malloc(Maxsec);
		partbuf = malloc(Maxsec);
		if(mbrbuf==nil || partbuf==nil) {
			free(mbrbuf);
			free(partbuf);
			partbuf = mbrbuf = nil;
			return;
		}
	}

	/*
	 * there might be no mbr (e.g. on a very large device), so look for
	 * a bare plan 9 partition table if mbrpart fails.
	 */
	if((type & NEW) && mbrpart(unit) >= 0){
		/* nothing to do */
	}
	else if (type & NEW)
		p9part(unit, "data");
	else if(type & OLD)
		oldp9part(unit);
}

static void
rdgeom(SDunit *unit)
{
	char *line;
	char *flds[5];
	Biobuf bb;
	Biobuf *bp;
	static char geom[] = "geometry ";

	bp = &bb;
	seek(unit->ctl, 0, 0);
	Binit(bp, unit->ctl, OREAD);
	while((line = Brdline(bp, '\n')) != nil){
		line[Blinelen(bp) - 1] = '\0';
		if (strncmp(line, geom, sizeof geom - 1) == 0)
			break;
	}
	if (line != nil && tokenize(line, flds, nelem(flds)) >= 3) {
		unit->sectors = atoll(flds[1]);
		unit->secsize = atoll(flds[2]);
	}
	Bterm(bp);
	seek(unit->ctl, 0, 0);
}

static void
setpartitions(char *name, int ctl, int data)
{
	SDunit sdunit;
	SDunit *unit;
	SDpart *part0;

	unit = &sdunit;
	memset(unit, 0, sizeof *unit);
	unit->ctl = ctl;
	unit->data = data;

	unit->secsize = Normsec;	/* default: won't work for CDs */
	unit->sectors = ~0ull;
	rdgeom(unit);
	strncpy(unit->name, name, sizeof unit->name);
	unit->part = mallocz(sizeof(SDpart) * SDnpart, 1);

	part0 = &unit->part[0];
	part0->end = unit->sectors - 1;
	strcpy(part0->name, "data");
	part0->valid = 1;
	unit->npart++;

	mbrbuf = malloc(Maxsec);
	partbuf = malloc(Maxsec);
	partition(unit);
	free(unit->part);
}

/*
 * read disk partition tables so that readnvram via factotum
 * can see them.
 */
int
readparts(void)
{
	int i, n, ctl, data, fd;
	char *name, *ctlname, *dataname;
	Dir *dir;

	fd = open("/dev", OREAD);
	if(fd < 0)
		return -1;
	n = dirreadall(fd, &dir);
	close(fd);

	for(i = 0; i < n; i++) {
		name = dir[i].name;
		if (strncmp(name, "sd", 2) != 0)
			continue;

		ctlname  = smprint("/dev/%s/ctl", name);
		dataname = smprint("/dev/%s/data", name);
		if (ctlname == nil || dataname == nil) {
			free(ctlname);
			free(dataname);
			continue;
		}

		ctl  = open(ctlname, ORDWR);
		data = open(dataname, OREAD);
		free(ctlname);
		free(dataname);

		if (ctl >= 0 && data >= 0)
			setpartitions(dataname, ctl, data);
		close(ctl);
		close(data);
	}
	free(dir);
	return 0;
}
