#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

#include	"sd.h"
#include	"dosfs.h"

enum {
	Npart = 32
};

uchar *mbrbuf, *partbuf;
int nbuf;

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

	if(sdbio(unit, pp, partbuf, unit->secsize, 0) != unit->secsize){
//		print("%s: sector read failed\n", unit->name);
		return;
	}

	if(strncmp((char*)partbuf, MAGIC, sizeof(MAGIC)-1) != 0) {
		/* not found on 2nd last sector; look on last sector */
		pp->start++;
		pp->end++;
		if(sdbio(unit, pp, partbuf, unit->secsize, 0) != unit->secsize){
//			print("%s: sector read failed\n", unit->name);
			return;
		}
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

	if(sdbio(unit, p, partbuf, unit->secsize, unit->secsize) != unit->secsize){
	//	print("%s: sector read failed\n", unit->name);
		return;
	}
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

/* 
 * Fetch the first dos and all plan9 partitions out of the MBR partition table.
 * We return -1 if we did not find a plan9 partition.
 */
static int
mbrpart(SDunit *unit)
{
	ulong mbroffset;
	Dospart *dp;
	ulong start, end;
	int havedos, i, nplan9;
	char name[10];

	if(sdbio(unit, &unit->part[0], mbrbuf, unit->secsize, 0) != unit->secsize){
//		print("%s: sector read failed\n", unit->name);
		return -1;
	}

	if(mbrbuf[0x1FE] != 0x55 || mbrbuf[0x1FF] != 0xAA){
//		print("%s: bad mbr %x %x\n", unit->name, mbrbuf[0x1fe], mbrbuf[0x1ff]);
		return -1;
	}

	mbroffset = 0;
	dp = (Dospart*)&mbrbuf[0x1BE];
	for(i=0; i<4; i++, dp++)
		if(dp->type == DMDDO) {
			mbroffset = 63;
			if(sdbio(unit, &unit->part[0], mbrbuf, unit->secsize, mbroffset) != unit->secsize){
//				print("%s: sector read failed\n", unit->name);
				return -1;
			}
		}

	if(mbrbuf[0x1FE] != 0x55 || mbrbuf[0x1FF] != 0xAA){
		return -1;
	}

	nplan9 = 0;
	havedos = 0;
	dp = (Dospart*)&mbrbuf[0x1BE];
	for(i=0; i<4; i++, dp++) {
		start = mbroffset+GLONG(dp->start);
		end = start+GLONG(dp->len);

		if(dp->type == PLAN9) {
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
		if(havedos == 0){
			if(dp->type == FAT12 || dp->type == FAT16
			|| dp->type == FATHUGE || dp->type == FAT32
			|| dp->type == FAT32X){
				havedos = 1;
				sdaddpart(unit, "dos", start, end);
			}
		}
	}
	return nplan9 ? 0 : -1;
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

	if((type & NEW) && mbrpart(unit) >= 0)
		;
	else if(type & OLD)
		oldp9part(unit);
}
