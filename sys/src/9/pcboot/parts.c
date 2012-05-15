/*
 * read disk partition tables, intended for early use on systems
 * that don't use (the new) 9load.  borrowed from old 9load.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"
#include	"pool.h"
#include	"../port/error.h"
#include	"../port/netif.h"
#include	"dosfs.h"
#include	"../port/sd.h"
#include	"iso9660.h"

#define gettokens(l, a, an, del)	getfields(l, a, an, 1, del)

enum {
	Trace	= 0,
	Parttrace = 0,
	Debugboot = 0,

	Maxsec	= 2048,
	Normsec	= 512,			/* mag disks */

	/* from devsd.c */
	PartLOG		= 8,
	NPart		= (1<<PartLOG),
};

typedef struct PSDunit PSDunit;
struct PSDunit {
	SDunit;
	Chan	*ctlc;
	Chan	*data;
};

static uchar *mbrbuf, *partbuf;
static char buf[128], buf2[128];

static void
psdaddpart(PSDunit* unit, char* name, uvlong start, uvlong end)
{
	int len, nw;

	sdaddpart(unit, name, start, end);

	/* update devsd's in-memory partition table. */
	len = snprint(buf, sizeof buf, "part %s %lld %lld\n", name, start, end);
	nw = devtab[unit->ctlc->type]->write(unit->ctlc, buf, len,
		unit->ctlc->offset);
	if (nw != len)
		print("can't update devsd's partition table\n");
	if (Debugboot)
		print("part %s %lld %lld\n", name, start, end);
}

static long
psdread(PSDunit *unit, SDpart *pp, void* va, long len, vlong off)
{
	long l, secsize;
	uvlong bno, nb;

	/*
	 * Check the request is within partition bounds.
	 */
	secsize = unit->secsize;
	if (secsize == 0)
		panic("psdread: %s: zero sector size", unit->name);
	bno = off/secsize + pp->start;
	nb = (off+len+secsize-1)/secsize + pp->start - bno;
	if(bno+nb > pp->end)
		nb = pp->end - bno;
	if(bno >= pp->end || nb == 0)
		return 0;

	unit->data->offset = bno * secsize;
	l = myreadn(unit->data, va, len);
	if (l < 0)
		return 0;
	return l;
}

static int
sdreadblk(PSDunit *unit, SDpart *part, void *a, vlong off, int mbr)
{
	uchar *b;

	assert(a);			/* sdreadblk */
	if(psdread(unit, part, a, unit->secsize, off) != unit->secsize){
		if(Trace)
			print("%s: read %lud at %lld failed\n", unit->name,
				unit->secsize, (vlong)part->start*unit->secsize+off);
		return -1;
	}
	b = a;
	if(mbr && (b[0x1FE] != 0x55 || b[0x1FF] != 0xAA)){
		if(Trace)
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
oldp9part(PSDunit *unit)
{
	SDpart *pp;
	char *field[3], *line[NPart+1];
	ulong n;
	uvlong start, end;
	int i;
	static SDpart fakepart;

	/*
	 * We prefer partition tables on the second to last sector,
	 * but some old disks use the last sector instead.
	 */

	pp = &fakepart;
	kstrdup(&pp->name, "partition");
	pp->start = unit->sectors - 2;
	pp->end = unit->sectors - 1;

	if(Debugboot)
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
	psdaddpart(unit, pp->name, pp->start, pp->end);

	/*
	 * parse partition table
	 */
	partbuf[unit->secsize-1] = '\0';
	n = gettokens((char*)partbuf, line, NPart+1, "\n");
	if(n && strncmp(line[0], MAGIC, sizeof(MAGIC)-1) == 0)
		for(i = 1; i < n; i++){
			if(gettokens(line[i], field, 3, " ") != 3)
				break;
			start = strtoull(field[1], 0, 0);
			end = strtoull(field[2], 0, 0);
			if(start >= end || end > unit->sectors)
				break;
			psdaddpart(unit, field[0], start, end);
		}
}

static SDpart*
sdfindpart(PSDunit *unit, char *name)
{
	int i;

	if(Parttrace)
		print("findpart %d %s %s: ", unit->npart, unit->name, name);
	for(i=0; i<unit->npart; i++) {
		if(Parttrace)
			print("%s...", unit->part[i].name);
		if(strcmp(unit->part[i].name, name) == 0){
			if(Parttrace)
				print("\n");
			return &unit->part[i];
		}
	}
	if(Parttrace)
		print("not found\n");
	return nil;
}

/*
 * look for a plan 9 partition table on drive `unit' in the second
 * sector (sector 1) of partition `name'.
 * if found, add the partitions defined in the table.
 */
static void
p9part(PSDunit *unit, char *name)
{
	SDpart *p;
	char *field[4], *line[NPart+1];
	uvlong start, end;
	int i, n;

	if(Debugboot)
		print("p9part %s %s\n", unit->name, name);
	p = sdfindpart(unit, name);
	if(p == nil)
		return;

	if(sdreadblk(unit, p, partbuf, unit->secsize, 0) < 0)
		return;
	partbuf[unit->secsize-1] = '\0';

	if(strncmp((char*)partbuf, "part ", 5) != 0)
		return;

	n = gettokens((char*)partbuf, line, NPart+1, "\n");
	if(n == 0)
		return;
	for(i = 0; i < n; i++){
		if(strncmp(line[i], "part ", 5) != 0)
			break;
		if(gettokens(line[i], field, 4, " ") != 4)
			break;
		start = strtoull(field[2], 0, 0);
		end   = strtoull(field[3], 0, 0);
		if(start >= end || end > unit->sectors)
			break;
		psdaddpart(unit, field[1], p->start+start, p->start+end);
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
mbrpart(PSDunit *unit)
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
				if(Trace)
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
		if(Trace) {
			if(firstxpart)
				print("%s ext %llud ", unit->name, taboffset);
			else
				print("%s mbr ", unit->name);
		}
		nxtxpart = 0;
		for(i=0; i<4; i++) {
			if(Trace)
				print("dp %d...", dp[i].type);
			start = taboffset+GLONG(dp[i].start);
			end = start+GLONG(dp[i].len);

			if(dp[i].type == PLAN9) {
				if(nplan9 == 0)
					strncpy(name, "plan9", sizeof name);
				else
					snprint(name, sizeof name, "plan9.%d",
						nplan9);
				psdaddpart(unit, name, start, end);
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
				psdaddpart(unit, "dos", start, end);
			}

			/* nxtxpart is relative to firstxpart (or 0), not taboffset */
			if(isextend(dp[i].type)){
				nxtxpart = start-taboffset+firstxpart;
				if(Trace)
					print("link %llud...", nxtxpart);
			}
		}
		if(Trace)
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
 * the FAT filesystem image embedded in a bootable CD.
 */
static int
part9660(PSDunit *unit)
{
	ulong a, n, i, j;
	uchar drecsz;
	uchar *p;
	uchar buf[Maxsec];
	Drec *rootdrec, *drec;
	Voldesc *v;
	static char stdid[] = "CD001\x01";

	if(unit->secsize == 0)
		unit->secsize = Cdsec;
	if(unit->secsize != Cdsec)
		return -1;

	if(psdread(unit, &unit->part[0], buf, Cdsec, VOLDESC*Cdsec) < 0)
		return -1;
	if(buf[0] != PrimaryIso ||
	    memcmp((char*)buf+1, stdid, sizeof stdid - 1) != 0)
		return -1;

	v = (Voldesc *)buf;
	rootdrec = (Drec *)v->z.desc.rootdir;
	assert(rootdrec);
	p = rootdrec->addr;
	a = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
	p = rootdrec->size;
	n = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
//	print("part9660: read %uld %uld\n", n, a);	/* debugging */

	if(n < Cdsec){
		print("warning: bad boot file size %ld in iso directory", n);
		n = Cdsec;
	}

	drec = nil;
	for(j = 0; j*Cdsec < n; j++){
		if(psdread(unit, &unit->part[0], buf, Cdsec, (a + j)*Cdsec) < 0)
			return -1;
		for(i = 0; i + j*Cdsec <= n && i < Cdsec; i += drecsz){
			drec = (Drec *)&buf[i];
			drecsz = drec->reclen;
			if(drecsz == 0 || drecsz + i > Cdsec)
				break;
			if(cistrncmp("bootdisk.img", (char *)drec->name, 12) == 0)
				goto Found;
		}
	}
Found:
	if(j*Cdsec >= n || drec == nil)
		return -1;

	p = drec->addr;
	a = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
	p = drec->size;
	n = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);

	print("found partition %s!9fat; %lud+%lud\n", unit->name, a, n);
	n /= Cdsec;
	psdaddpart(unit, "9fat", a, a+n);
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
partition(PSDunit *unit)
{
	int type;
	char *p;

	if(unit->part == 0)
		return;

	if(part9660(unit) == 0)
		return;

	p = getconf("partition");
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
rdgeom(PSDunit *unit)
{
	int n, f, lines;
	char *buf, *p;
	char *line[64], *fld[5];
	char ctl[64];
	static char geom[] = "geometry";

	buf = smalloc(Maxfile + 1);
	strncpy(ctl, unit->name, sizeof ctl);
	p = strrchr(ctl, '/');
	if (p)
		strcpy(p, "/ctl");		/* was "/data" */
	n = readfile(ctl, buf, Maxfile);
	if (n < 0) {
		print("rdgeom: can't read %s\n", ctl);
		free(buf);
		return;
	}
	buf[n] = 0;

	lines = getfields(buf, line, nelem(line), 0, "\r\n");
	for (f = 0; f < lines; f++)
		if (tokenize(line[f], fld, nelem(fld)) >= 3 &&
		    strcmp(fld[0], geom) == 0)
			break;
	if(f < lines){
		unit->sectors = strtoull(fld[1], nil, 0);
		unit->secsize = strtoull(fld[2], nil, 0);
	}
	if (f >= lines || unit->sectors == 0){
		/* no geometry line, so fake it */
		unit->secsize = Cdsec;
		unit->sectors = ~0ull / unit->secsize;
	}
	if(unit->secsize == 0)
		print("rdgeom: %s: zero sector size read from ctl file\n",
			unit->name);
	free(buf);
	unit->ctlc->offset = 0;
}

static void
setpartitions(char *name, Chan *ctl, Chan *data)
{
	PSDunit sdunit;
	PSDunit *unit;
	SDpart *part0;

	unit = &sdunit;
	memset(unit, 0, sizeof *unit);
	unit->ctlc = ctl;
	unit->data = data;

	unit->secsize = Normsec;	/* default: won't work for CDs */
	unit->sectors = ~0ull / unit->secsize;
	kstrdup(&unit->name, name);
	rdgeom(unit);
	unit->part = mallocz(sizeof(SDpart) * SDnpart, 1);
	unit->npart = SDnpart;

	part0 = &unit->part[0];
	part0->end = unit->sectors - 1;
	kstrdup(&part0->name, "data");
	part0->valid = 1;

	mbrbuf = malloc(Maxsec);
	partbuf = malloc(Maxsec);
	partition(unit);
	free(unit->part);
	unit->part = nil;
}

/*
 * read disk partition tables so that readnvram via factotum
 * can see them.
 */
int
readparts(char *disk)
{
	Chan *ctl, *data;

	snprint(buf, sizeof buf, "%s/ctl", disk);
	ctl  = namecopen(buf, ORDWR);
	snprint(buf2, sizeof buf2, "%s/data", disk);
	data = namecopen(buf2, OREAD);
	if (ctl != nil && data != nil)
		setpartitions(buf2, ctl, data);
	cclose(ctl);
	cclose(data);
	return 0;
}

/*
 * Leave partitions around for devsd in next kernel to pick up.
 * (Needed by boot process; more extensive
 * partitioning is done by termrc or cpurc).
 */
void
sdaddconf(SDunit *unit)
{
	int i;
	SDpart *pp;

	/*
	 * If there were no partitions (just data and partition), don't bother.
	 */
	if(unit->npart <= 1 || (unit->npart == 2 &&
	    strcmp(unit->part[1].name, "partition") == 0))
		return;

	addconf("%spart=", unit->name);
	/* skip 0, which is "data" */
	for(i = 1, pp = &unit->part[i]; i < unit->npart; i++, pp++)
		if (pp->valid)
			addconf("%s%s %lld %lld", i==1 ? "" : "/", pp->name,
				pp->start, pp->end);
	addconf("\n");
}
