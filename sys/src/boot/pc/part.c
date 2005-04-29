#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"sd.h"
#include	"fs.h"

enum {
	Npart = 32
};

uchar *mbrbuf, *partbuf;
int nbuf;
#define trace 0

int
tsdbio(SDunit *unit, SDpart *part, void *a, vlong off, int mbr)
{
	uchar *b;

	if(sdbio(unit, part, a, unit->secsize, off) != unit->secsize){
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
	ulong n, start, end;
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

	if(tsdbio(unit, pp, partbuf, 0, 0) < 0)
		return;

	if(strncmp((char*)partbuf, MAGIC, sizeof(MAGIC)-1) != 0) {
		/* not found on 2nd last sector; look on last sector */
		pp->start++;
		pp->end++;
		if(tsdbio(unit, pp, partbuf, 0, 0) < 0)
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
	n = getfields((char*)partbuf, line, Npart+1, '\n');
	if(n && strncmp(line[0], MAGIC, sizeof(MAGIC)-1) == 0){
		for(i = 1; i < n && unit->npart < SDnpart; i++){
			if(getfields(line[i], field, 3, ' ') != 3)
				break;
			start = strtoul(field[1], 0, 0);
			end = strtoul(field[2], 0, 0);
			if(start >= end || end > unit->sectors)
				break;
			sdaddpart(unit, field[0], start, end);
		}
	}	
}

static void
p9part(SDunit *unit, char *name)
{
	SDpart *p;
	char *field[4], *line[Npart+1];
	ulong start, end;
	int i, n;
	
	p = sdfindpart(unit, name);
	if(p == nil)
		return;

	if(tsdbio(unit, p, partbuf, unit->secsize, 0) < 0)
		return;
	partbuf[unit->secsize-1] = '\0';

	if(strncmp((char*)partbuf, "part ", 5) != 0)
		return;

	n = getfields((char*)partbuf, line, Npart+1, '\n');
	if(n == 0)
		return;
	for(i = 0; i < n && unit->npart < SDnpart; i++){
		if(strncmp(line[i], "part ", 5) != 0)
			break;
		if(getfields(line[i], field, 4, ' ') != 4)
			break;
		start = strtoul(field[2], 0, 0);
		end = strtoul(field[3], 0, 0);
		if(start >= end || end > unit->sectors)
			break;
		sdaddpart(unit, field[1], p->start+start, p->start+end);
	}
}

int
isdos(int t)
{
	return t==FAT12 || t==FAT16 || t==FATHUGE || t==FAT32 || t==FAT32X;
}

int
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
	ulong taboffset, start, end;
	ulong firstxpart, nxtxpart;
	int havedos, i, nplan9;
	char name[10];

	taboffset = 0;
	dp = (Dospart*)&mbrbuf[0x1BE];
	if(1) {
		/* get the MBR (allowing for DMDDO) */
		if(tsdbio(unit, &unit->part[0], mbrbuf, (vlong)taboffset*unit->secsize, 1) < 0)
			return -1;
		for(i=0; i<4; i++)
			if(dp[i].type == DMDDO) {
				if(trace)
					print("DMDDO partition found\n");
				taboffset = 63;
				if(tsdbio(unit, &unit->part[0], mbrbuf, (vlong)taboffset*unit->secsize, 1) < 0)
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
		if(tsdbio(unit, &unit->part[0], mbrbuf, (vlong)taboffset*unit->secsize, 1) < 0)
			return -1;
		if(trace) {
			if(firstxpart)
				print("%s ext %lud ", unit->name, taboffset);
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
					print("link %lud...", nxtxpart);
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
	uchar buf[2048];
	ulong a, n;
	uchar *p;

	if(unit->secsize != 2048)
		return -1;

	if(sdbio(unit, &unit->part[0], buf, 2048, 17*2048) < 0)
		return -1;

	if(buf[0] || strcmp((char*)buf+1, "CD001\x01EL TORITO SPECIFICATION") != 0)
		return -1;

	
	p = buf+0x47;
	a = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);

	if(sdbio(unit, &unit->part[0], buf, 2048, a*2048) < 0)
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
	n /= 2048;

	print("found partition %s!cdboot; %lud+%lud\n", unit->name, a, n);
	sdaddpart(unit, "cdboot", a, a+n);
	return 0;
}

enum {
	NEW = 1<<0,
	OLD = 1<<1
};

void
partition(SDunit *unit)
{
	int type;
	char *p;

	if(unit->part == 0)
		return;

	if(part9660(unit) == 0)
		return;

	p = getconf("partition");
	if(p == nil)
		p = defaultpartition;

	if(p != nil && strncmp(p, "new", 3) == 0)
		type = NEW;
	else if(p != nil && strncmp(p, "old", 3) == 0)
		type = OLD;
	else
		type = NEW|OLD;

	if(nbuf < unit->secsize) {
		free(mbrbuf);
		free(partbuf);
		mbrbuf = malloc(unit->secsize);
		partbuf = malloc(unit->secsize);
		if(mbrbuf==nil || partbuf==nil) {
			free(mbrbuf);
			free(partbuf);
			partbuf = mbrbuf = nil;
			nbuf = 0;
			return;
		}
		nbuf = unit->secsize;
	}

	if((type & NEW) && mbrpart(unit) >= 0){
		/* nothing to do */;
	}
	else if(type & OLD)
		oldp9part(unit);
}
