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

uchar *buf;
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

	if(sdbio(unit, pp, buf, unit->secsize, 0) != unit->secsize){
//		print("%s: sector read failed\n", unit->name);
		return;
	}

	if(strncmp((char*)buf, MAGIC, sizeof(MAGIC)-1) != 0) {
		/* not found on 2nd last sector; look on last sector */
		pp->start++;
		pp->end++;
		if(sdbio(unit, pp, buf, unit->secsize, 0) != unit->secsize){
//			print("%s: sector read failed\n", unit->name);
			return;
		}
		if(strncmp((char*)buf, MAGIC, sizeof(MAGIC)-1) != 0)
			return;
		print("%s: using old plan9 partition table on last sector\n", unit->name);
	}else
		print("%s: using old plan9 partition table on 2nd-to-last sector\n", unit->name);

	/* we found a partition table, so add a partition partition */
	unit->npart++;
	buf[unit->secsize-1] = '\0';

	/*
	 * parse partition table
	 */
	n = getfields((char*)buf, line, Npart+1, '\n');
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

/* 
 * Fetch the first dos and plan9 partitions out of the MBR partition table.
 * We return -1 if we did not find a plan9 partition.
 */
static int
mbrpart(SDunit *unit)
{
	ulong mbroffset;
	Dospart *dp;
	ulong start, end;
	int havep9, havedos, i;

	if(sdbio(unit, &unit->part[0], buf, unit->secsize, 0) != unit->secsize){
//		print("%s: sector read failed\n", unit->name);
		return -1;
	}
	if(buf[0x1FE] != 0x55 || buf[0x1FF] != 0xAA){
//		print("%s: bad mbr %x %x\n", unit->name, buf[0x1fe], buf[0x1ff]);
		return -1;
	}

	mbroffset = 0;
	dp = (Dospart*)&buf[0x1BE];
	for(i=0; i<4; i++, dp++)
		if(dp->type == DMDDO) {
			mbroffset = 63;
			if(sdbio(unit, &unit->part[0], buf, unit->secsize, mbroffset) != unit->secsize){
//				print("%s: sector read failed\n", unit->name);
				return -1;
			}
		}

	if(buf[0x1FE] != 0x55 || buf[0x1FF] != 0xAA){
		return -1;
	}

	havep9 = havedos = 0;
	dp = (Dospart*)&buf[0x1BE];
	for(i=0; i<4; i++, dp++) {
		start = mbroffset+GLONG(dp->start);
		end = start+GLONG(dp->len);

		if(havep9 == 0 && dp->type == PLAN9) {
			havep9 = 1;
			sdaddpart(unit, "plan9", start, end);
		}

		/*
		 * We used to take the active partition (and then the first
		 * when none are active).  We have to take the first here,
		 * so that the partition we call ``dos'' agrees with the
		 * partition fdisk calls ``dos''. 
		 */
		if(havedos == 0 && (dp->type == FAT12 || dp->type == FAT16
					|| dp->type == FATHUGE || dp->type == FAT32)) {
			havedos = 1;
			sdaddpart(unit, "dos", start, end);
		}
	}
	return havep9 ? 0 : -1;
}

static void
p9part(SDunit *unit)
{
	SDpart *p;
	char *field[4], *line[Npart+1];
	ulong start, end;
	int i, n;
	
	p = sdfindpart(unit, "plan9");
	if(p == nil)
		return;

	if(sdbio(unit, p, buf, unit->secsize, unit->secsize) != unit->secsize){
//		print("%s: sector read failed\n", unit->name);
		return;
	}
	buf[unit->secsize-1] = '\0';

	if(strncmp((char*)buf, "part ", 5) != 0)
		return;

	n = getfields((char*)buf, line, Npart+1, '\n');
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
	if(p != nil && strncmp(p, "new", 3) == 0)
		type = NEW;
	else if(p != nil && strncmp(p, "old", 3) == 0)
		type = OLD;
	else
		type = NEW|OLD;

	if(nbuf < unit->secsize) {
		buf = malloc(unit->secsize);
		nbuf = unit->secsize;
	}

	if((type & NEW) && mbrpart(unit) >= 0)
		p9part(unit);
	else if(type & OLD)
		oldp9part(unit);
}
