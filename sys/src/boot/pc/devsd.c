/*
 * Storage Device.
 */
#include "u.h"
#include "mem.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "error.h"

#include "sd.h"
#include "fs.h"

#define parttrace 0


extern SDifc* sdifc[];

static SDev* sdlist;
static SDunit** sdunit;
static int sdnunit;
static int _sdmask;
static int cdmask;
static int sdmask;

enum {
	Rawcmd,
	Rawdata,
	Rawstatus,
};

void
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
}

void
sddelpart(SDunit* unit,  char* name)
{
	int i;
	SDpart *pp;

	if(parttrace)
		print("del %d %s %s\n", unit->npart, unit->name, name);
	/*
	 * Look for the partition to delete.
	 * Can't delete if someone still has it open.
	 * If it's the last valid partition zap the
	 * whole table.
	 */
	pp = unit->part;
	for(i = 0; i < SDnpart; i++){
		if(strncmp(name, pp->name, NAMELEN) == 0)
			break;
		pp++;
	}
	if(i >= SDnpart)
		return;
	pp->valid = 0;

	unit->npart--;
	if(unit->npart == 0){
		free(unit->part);
		unit->part = nil;
	}
}

static int
sdinitpart(SDunit* unit)
{
	unit->sectors = unit->secsize = 0;
	unit->npart = 0;
	if(unit->part){
		free(unit->part);
		unit->part = nil;
	}

	if(unit->inquiry[0] & 0xC0)
		return 0;
	switch(unit->inquiry[0] & 0x1F){
	case 0x00:			/* DA */
	case 0x04:			/* WORM */
	case 0x05:			/* CD-ROM */
	case 0x07:			/* MO */
		break;
	default:
		return 0;
	}

	if(unit->dev->ifc->online == nil || unit->dev->ifc->online(unit) == 0)
		return 0;
	sdaddpart(unit, "data", 0, unit->sectors);
	return 1;
}

static SDunit*
sdgetunit(SDev* sdev, int subno)
{
	int index;
	SDunit *unit;

	/*
	 * Associate a unit with a given device and sub-unit
	 * number on that device.
	 * The device will be probed if it has not already been
	 * successfully accessed.
	 */
	qlock(&sdqlock);
	index = sdev->index+subno;
	unit = sdunit[index];
	if(unit == nil){
		if((unit = malloc(sizeof(SDunit))) == nil){
			qunlock(&sdqlock);
			return nil;
		}

		if(sdev->enabled == 0 && sdev->ifc->enable)
			sdev->ifc->enable(sdev);
		sdev->enabled = 1;

		snprint(unit->name, NAMELEN, "sd%c%d", sdev->idno, subno);
		unit->subno = subno;
		unit->dev = sdev;

		/*
		 * No need to lock anything here as this is only
		 * called before the unit is made available in the
		 * sdunit[] array.
		 */
		if(unit->dev->ifc->verify(unit) == 0){
			qunlock(&sdqlock);
			free(unit);
			return nil;
		}
		sdunit[index] = unit;
	}
	qunlock(&sdqlock);

	return unit;
}

static SDunit*
sdindex2unit(int index)
{
	SDev *sdev;

	/*
	 * Associate a unit with a given index into the top-level
	 * device directory.
	 * The device will be probed if it has not already been
	 * successfully accessed.
	 */
	for(sdev = sdlist; sdev != nil; sdev = sdev->next){
		if(index >= sdev->index && index < sdev->index+sdev->nunit)
			return sdgetunit(sdev, index-sdev->index);
	}

	return nil;
}

static void
_sddetach(void)
{
	SDev *sdev;

	for(sdev = sdlist; sdev != nil; sdev = sdev->next){
		if(sdev->enabled == 0)
			continue;
		if(sdev->ifc->disable)
			sdev->ifc->disable(sdev);
		sdev->enabled = 0;
	}
}

static void
sddump(void)
{
	SDev *sdev;

	print("sdevs:\n");
	for(sdev = sdlist; sdev != nil; sdev = sdev->next){
		print("sdev %c index %d nunit %d: ",
			sdev->idno, sdev->index, sdev->nunit);
		print("\n");
	}
}

static int
_sdinit(void)
{
	ulong m;
	int i;
	SDev *sdev, *tail;
	SDunit *unit;

	/*
	 * Probe all configured controllers and make a list
	 * of devices found, accumulating a possible maximum number
	 * of units attached and marking each device with an index
	 * into the linear top-level directory array of units.
	 */
	tail = nil;
	for(i = 0; sdifc[i] != nil; i++){
		if((sdev = sdifc[i]->pnp()) == nil)
			continue;
		if(sdlist != nil)
			tail->next = sdev;
		else
			sdlist = sdev;
		for(tail = sdev; tail->next != nil; tail = tail->next){
			tail->index = sdnunit;
			sdnunit += tail->nunit;
		}
		tail->index = sdnunit;
		sdnunit += tail->nunit;
	}
	/*
	 * Legacy and option code goes here. This will be hard...
	 */

	/*
	 * The maximum number of possible units is known, allocate
	 * placeholders for their datastructures; the units will be
	 * probed and structures allocated when attached.
	 * Allocate controller names for the different types.
	 */
	if(sdnunit == 0)
		return 0;
	if((sdunit = malloc(sdnunit*sizeof(SDunit*))) == nil)
		return 0;
	sddetach = _sddetach;

	for(i = 0; sdifc[i] != nil; i++){
		if(sdifc[i]->id)
			sdifc[i]->id(sdlist);
	}
	if (0)
		sddump();

	m = 0;
	cdmask = sdmask = 0;
	for(i=0; i<sdnunit && i < 32; i++) {
		unit = sdindex2unit(i);
		if(unit == nil)
			continue;
		sdinitpart(unit);
		partition(unit);
		if(unit->npart > 0){	/* BUG */
			if((unit->inquiry[0] & 0x1F) == 0x05)
				cdmask |= (1<<i);
			else
				sdmask |= (1<<i);
			m |= (1<<i);
		}
	}

//notesdinfo();
	_sdmask = m;
	return m;
}

int
cdinit(void)
{
	if(sdnunit == 0)
		_sdinit();
	return cdmask;
}

int
sdinit(void)
{
	if(sdnunit == 0)
		_sdinit();
	return sdmask;
}

void
sdinitdev(int i, char *s)
{
	SDunit *unit;

	unit = sdindex2unit(i);
	strcpy(s, unit->name);
}

void
sdprintdevs(int i)
{
	char *s;
	SDunit *unit;

	unit = sdindex2unit(i);
	for(i=0; i<unit->npart; i++){
		s = unit->part[i].name;
		if(strncmp(s, "dos", 3) == 0
		|| strncmp(s, "9fat", 4) == 0
		|| strncmp(s, "fs", 2) == 0)
			print(" %s!%s", unit->name, s);
	}
}

SDpart*
sdfindpart(SDunit *unit, char *name)
{
	int i;

	if(parttrace)
		print("findpart %d %s %s\t\n", unit->npart, unit->name, name);
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

typedef struct Scsicrud Scsicrud;
struct Scsicrud {
	Fs fs;
	vlong offset;
	SDunit *unit;
	SDpart *part;
};

long
sdread(Fs *vcrud, void *v, long n)
{
	Scsicrud *crud;
	long x;

	crud = (Scsicrud*)vcrud;
	x = sdbio(crud->unit, crud->part, v, n, crud->offset);
	if(x > 0)
		crud->offset += x;
	return x;
}

vlong
sdseek(Fs *vcrud, vlong seek)
{
	((Scsicrud*)vcrud)->offset = seek;
	return seek;
}

void*
sdgetfspart(int i, char *s, int chatty)
{
	SDunit *unit;
	SDpart *p;
	Scsicrud *crud;

	if(cdmask&(1<<i)){
		if(strcmp(s, "cdboot") != 0)
			return nil;
	}else if(sdmask&(1<<i)){
		if(strcmp(s, "cdboot") == 0)
			return nil;
	}

	unit = sdindex2unit(i);
	if((p = sdfindpart(unit, s)) == nil){
		if(chatty)
			print("unknown partition %s!%s\n", unit->name, s);
		return nil;
	}
	if(p->crud == nil) {
		crud = malloc(sizeof(Scsicrud));
		crud->fs.dev = i;
		crud->fs.diskread = sdread;
		crud->fs.diskseek = sdseek;
	//	crud->start = 0;
		crud->unit = unit;
		crud->part = p;
		if(dosinit(&crud->fs) < 0 && dosinit(&crud->fs) < 0 && kfsinit(&crud->fs) < 0){
			if(chatty)
				print("partition %s!%s does not contain a DOS or KFS file system\n",
					unit->name, s);
			return nil;
		}
		p->crud = crud;
	}
	return p->crud;
}

/*
 * Leave partitions around for devsd to pick up.
 * (Needed by boot process; more extensive 
 * partitioning is done by termrc or cpurc).
 */
void
sdaddconf(int i)
{
	SDunit *unit;
	SDpart *pp;

	unit = sdindex2unit(i);
	
	/*
	 * If there were no partitions (just data and partition), don't bother.
	 */
	if(unit->npart<= 1 || (unit->npart==2 && strcmp(unit->part[1].name, "partition")==0))
		return;

	addconf("%spart=", unit->name);
	for(i=1, pp=&unit->part[i]; i<unit->npart; i++, pp++)	/* skip 0, which is "data" */
		addconf("%s%s %lld %lld", i==1 ? "" : "/", pp->name,
			pp->start, pp->end);
	addconf("\n");
}

int
sdboot(int dev, char *pname, Boot *b)
{
	char *file;
	Fs *fs;

	if((file = strchr(pname, '!')) == nil) {
		print("syntax is sdC0!partition!file\n");
		return -1;
	}
	*file++ = '\0';

	fs = sdgetfspart(dev, pname, 1);
	if(fs == nil)
		return -1;

	return fsboot(fs, file, b);
}

long
sdbio(SDunit *unit, SDpart *pp, void* va, long len, vlong off)
{
	long l;
	ulong bno, max, nb, offset;
	static uchar *b;
	char *a;
	static ulong bsz;

	a = va;
memset(a, 0xDA, len);
	qlock(&unit->ctl);
	if(unit->changed){
		qunlock(&unit->ctl);
		return 0;
	}

	/*
	 * Check the request is within bounds.
	 * Removeable drives are locked throughout the I/O
	 * in case the media changes unexpectedly.
	 * Non-removeable drives are not locked during the I/O
	 * to allow the hardware to optimise if it can; this is
	 * a little fast and loose.
	 * It's assumed that non-removable media parameters
	 * (sectors, secsize) can't change once the drive has
	 * been brought online.
	 */
	bno = (off/unit->secsize) + pp->start;
	nb = ((off+len+unit->secsize-1)/unit->secsize) + pp->start - bno;
	max = SDmaxio/unit->secsize;
	if(nb > max)
		nb = max;
	if(bno+nb > pp->end)
		nb = pp->end - bno;
	if(bno >= pp->end || nb == 0){
		qunlock(&unit->ctl);
		return 0;
	}
	if(!(unit->inquiry[1] & 0x80))
		qunlock(&unit->ctl);

	if(bsz < nb*unit->secsize){
		b = malloc(nb*unit->secsize);
		bsz = nb*unit->secsize;
	}
//	b = sdmalloc(nb*unit->secsize);
//	if(b == nil)
//		return 0;

	offset = off%unit->secsize;
	if((l = unit->dev->ifc->bio(unit, 0, 0, b, nb, bno)) < 0) {
//		sdfree(b);
		return 0;
	}

	if(l < offset)
		len = 0;
	else if(len > l - offset)
		len = l - offset;
	if(len)
		memmove(a, b+offset, len);
//	sdfree(b);

	if(unit->inquiry[1] & 0x80)
		qunlock(&unit->ctl);

	return len;
}

#ifdef DMA
long
sdrio(SDreq *r, void* a, long n)
{
	if(n >= SDmaxio || n < 0)
		return 0;

	r->data = nil;
	if(n){
		if((r->data = malloc(n)) == nil)
			return 0;
		if(r->write)
			memmove(r->data, a, n);
	}
	r->dlen = n;

	if(r->unit->dev->ifc->rio(r) != SDok){
// cgascreenputs("1", 1);
		if(r->data != nil){
			sdfree(r->data);
			r->data = nil;
		}
		return 0;
	}
// cgascreenputs("2", 1);

	if(!r->write && r->rlen > 0)
		memmove(a, r->data, r->rlen);
// cgascreenputs("3", 1);
	if(r->data != nil){
		sdfree(r->data);
		r->data = nil;
	}

// cgascreenputs("4", 1);
	return r->rlen;
}
#endif /* DMA */

void
sleep(void*, int (*fn)(void*), void *v)
{
	int x;
	x = spllo();
	while(!fn(v))
		;
	splx(x);
	return;
}

void
tsleep(void*, int (*fn)(void*), void *v, int msec)
{
	int x;
	ulong start;

	x = spllo();
	for(start = m->ticks; TK2MS(m->ticks - start) < msec
		&& !fn(v); )
		;
	splx(x);
	return;
}

void*
sdmalloc(void *p, ulong sz)
{
	if(p != nil) {
		memset(p, 0, sz);
		return p;
	}
	return malloc(sz);
}
