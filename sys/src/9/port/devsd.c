/*
 * Storage Device.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

#include "../port/sd.h"

extern Dev sddevtab;
extern SDifc* sdifc[];

typedef struct SDevgrp {
	SDev*	dev;
	int	nunits;		/* num units in dev */
} SDevgrp;

static SDevgrp* devs;			/* all devices */
static QLock devslock;			/* insertion and removal of devices */
static int ndevs;			/* total number of devices in the system */

enum {
	Rawcmd,
	Rawdata,
	Rawstatus,
};

enum {
	Qtopdir		= 1,		/* top level directory */
	Qtopbase,
	Qtopctl = Qtopbase,
	Qtopstat,

	Qunitdir,			/* directory per unit */
	Qunitbase,
	Qctl		= Qunitbase,
	Qraw,
	Qpart,

	TypeLOG		= 4,
	NType		= (1<<TypeLOG),
	TypeMASK	= (NType-1),
	TypeSHIFT	= 0,

	PartLOG		= 8,
	NPart		= (1<<PartLOG),
	PartMASK	= (NPart-1),
	PartSHIFT	= TypeLOG,

	UnitLOG		= 8,
	NUnit		= (1<<UnitLOG),
	UnitMASK	= (NUnit-1),
	UnitSHIFT	= (PartLOG+TypeLOG),

	DevLOG		= 8,
	NDev		= (1 << DevLOG),
	DevMASK	= (NDev-1),
	DevSHIFT = (UnitLOG+PartLOG+TypeLOG),

	Ncmd = 20,
};

#define TYPE(q)		((((ulong)(q).path)>>TypeSHIFT) & TypeMASK)
#define PART(q)		((((ulong)(q).path)>>PartSHIFT) & PartMASK)
#define UNIT(q)		((((ulong)(q).path)>>UnitSHIFT) & UnitMASK)
#define DEV(q)		((((ulong)(q).path)>>DevSHIFT) & DevMASK)
#define QID(d,u, p, t)	(((d)<<DevSHIFT)|((u)<<UnitSHIFT)|\
					 ((p)<<PartSHIFT)|((t)<<TypeSHIFT))


static void
sdaddpart(SDunit* unit, char* name, ulong start, ulong end)
{
	SDpart *pp;
	int i, partno;

	/*
	 * Check name not already used
	 * and look for a free slot.
	 */
	if(unit->part != nil){
		partno = -1;
		for(i = 0; i < unit->npart; i++){
			pp = &unit->part[i];
			if(!pp->valid){
				if(partno == -1)
					partno = i;
				break;
			}
			if(strcmp(name, pp->name) == 0){
				if(pp->start == start && pp->end == end)
					return;
				error(Ebadctl);
			}
		}
	}
	else{
		if((unit->part = malloc(sizeof(SDpart)*SDnpart)) == nil)
			error(Enomem);
		unit->npart = SDnpart;
		partno = 0;
	}

	/*
	 * If no free slot found then increase the
	 * array size (can't get here with unit->part == nil).
	 */
	if(partno == -1){
		if(unit->npart >= NPart)
			error(Enomem);
		if((pp = malloc(sizeof(SDpart)*(unit->npart+SDnpart))) == nil)
			error(Enomem);
		memmove(pp, unit->part, sizeof(SDpart)*unit->npart);
		free(unit->part);
		unit->part = pp;
		partno = unit->npart;
		unit->npart += SDnpart;
	}

	/*
	 * Check size and extent are valid.
	 */
	if(start > end || end > unit->sectors)
		error(Eio);
	pp = &unit->part[partno];
	pp->start = start;
	pp->end = end;
	kstrdup(&pp->name, name);
	kstrdup(&pp->user, eve);
	pp->perm = 0640;
	pp->valid = 1;
}

static void
sddelpart(SDunit* unit,  char* name)
{
	int i;
	SDpart *pp;

	/*
	 * Look for the partition to delete.
	 * Can't delete if someone still has it open.
	 */
	pp = unit->part;
	for(i = 0; i < unit->npart; i++){
		if(strcmp(name, pp->name) == 0)
			break;
		pp++;
	}
	if(i >= unit->npart)
		error(Ebadctl);
	if(strcmp(up->user, pp->user) && !iseve())
		error(Eperm);
	pp->valid = 0;
	pp->vers++;
}

static int
sdinitpart(SDunit* unit)
{
	int i, nf;
	ulong start, end;
	char *f[4], *p, *q, buf[10];

	unit->vers++;
	unit->sectors = unit->secsize = 0;
	if(unit->part){
		for(i = 0; i < unit->npart; i++){
			unit->part[i].valid = 0;
			unit->part[i].vers++;
		}
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

	if(unit->dev->ifc->online)
		unit->dev->ifc->online(unit);
	if(unit->sectors){
		sdaddpart(unit, "data", 0, unit->sectors);
	
		/*
		 * Use partitions passed from boot program,
		 * e.g.
		 *	sdC0part=dos 63 123123/plan9 123123 456456
		 * This happens before /boot sets hostname so the
		 * partitions will have the null-string for user.
		 * The gen functions patch it up.
		 */
		snprint(buf, sizeof buf, "%spart", unit->name);
		for(p = getconf(buf); p != nil; p = q){
			if(q = strchr(p, '/'))
				*q++ = '\0';
			nf = tokenize(p, f, nelem(f));
			if(nf < 3)
				continue;
		
			start = strtoul(f[1], 0, 0);
			end = strtoul(f[2], 0, 0);
			if(!waserror()){
				sdaddpart(unit, f[0], start, end);
				poperror();
			}
		}			
	}

	return 1;
}

static SDev*
sdgetdev(int idno)
{
	SDev *sdev;
	int i;

	qlock(&devslock);
	for(i = 0; i != ndevs; i++)
		if(devs[i].dev->idno == idno)
			break;
	
	if(i == ndevs)
		sdev = nil;
	else{
		sdev = devs[i].dev;
		incref(&sdev->r);
	}
	qunlock(&devslock);
	return sdev;
}

static SDunit*
sdgetunit(SDev* sdev, int subno)
{
	SDunit *unit;
	char buf[32];

	/*
	 * Associate a unit with a given device and sub-unit
	 * number on that device.
	 * The device will be probed if it has not already been
	 * successfully accessed.
	 */
	qlock(&sdev->unitlock);
	if(subno > sdev->nunit){
		qunlock(&sdev->unitlock);
		return nil;
	}

	unit = sdev->unit[subno];
	if(unit == nil){
		/*
		 * Probe the unit only once. This decision
		 * may be a little severe and reviewed later.
		 */
		if(sdev->unitflg[subno]){
			qunlock(&sdev->unitlock);
			return nil;
		}
		if((unit = malloc(sizeof(SDunit))) == nil){
			qunlock(&sdev->unitlock);
			return nil;
		}
		sdev->unitflg[subno] = 1;

		snprint(buf, sizeof(buf), "%s%d", sdev->name, subno);
		kstrdup(&unit->name, buf);
		kstrdup(&unit->user, eve);
		unit->perm = 0555;
		unit->subno = subno;
		unit->dev = sdev;

		if(sdev->enabled == 0 && sdev->ifc->enable)
			sdev->ifc->enable(sdev);
		sdev->enabled = 1;

		/*
		 * No need to lock anything here as this is only
		 * called before the unit is made available in the
		 * sdunit[] array.
		 */
		if(unit->dev->ifc->verify(unit) == 0){
			qunlock(&sdev->unitlock);
			free(unit);
			return nil;
		}
		sdev->unit[subno] = unit;
	}
	qunlock(&sdev->unitlock);
	return unit;
}

static void
sdreset(void)
{
	int i;
	SDev *sdev, *tail, *sdlist;

	/*
	 * Probe all configured controllers and make a list
	 * of devices found, accumulating a possible maximum number
	 * of units attached and marking each device with an index
	 * into the linear top-level directory array of units.
	 */
	tail = sdlist = nil;
	for(i = 0; sdifc[i] != nil; i++){
		if(sdifc[i]->pnp == nil || (sdev = sdifc[i]->pnp()) == nil)
			continue;
		if(sdlist != nil)
			tail->next = sdev;
		else
			sdlist = sdev;
		for(tail = sdev; tail->next != nil; tail = tail->next){
			tail->unit = (SDunit**)malloc(tail->nunit * sizeof(SDunit*));
			tail->unitflg = (int*)malloc(tail->nunit * sizeof(int));
			assert(tail->unit && tail->unitflg);
			ndevs++;
		}
		tail->unit = (SDunit**)malloc(tail->nunit * sizeof(SDunit*));
		tail->unitflg = (int*)malloc(tail->nunit * sizeof(int));
		ndevs++;
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
	if(ndevs == 0)
		return;
	for(i = 0; sdifc[i] != nil; i++){
		/*
		 * BUG: no check is made here or later when a
		 * unit is attached that the id and name are set.
		 */
		if(sdifc[i]->id)
			sdifc[i]->id(sdlist);
	}

	/* 
	  * The IDs have been set, unlink the sdlist and copy the spec to
	  * the devtab.
	  */
	devs = (SDevgrp*)malloc(ndevs * sizeof(SDevgrp));
	memset(devs, 0, ndevs * sizeof(SDevgrp));
	i = 0;
	while(sdlist != nil){
		devs[i].dev = sdlist;
		devs[i].nunits = sdlist->nunit;
		sdlist = sdlist->next;
		devs[i].dev->next = nil;
		i++;
	}
}

static int
sd2gen(Chan* c, int i, Dir* dp)
{
	Qid q;
	vlong l;
	SDpart *pp;
	SDperm *perm;
	SDunit *unit;
	SDev *sdev;
	int rv;

	sdev = sdgetdev(DEV(c->qid));
	assert(sdev);
	unit = sdev->unit[UNIT(c->qid)];

	rv = -1;
	switch(i){
	case Qctl:
		mkqid(&q, QID(DEV(c->qid), UNIT(c->qid), PART(c->qid), Qctl), 
			   unit->vers, QTFILE);
		perm = &unit->ctlperm;
		if(emptystr(perm->user)){
			kstrdup(&perm->user, eve);
			perm->perm = 0640;
		}
		devdir(c, q, "ctl", 0, perm->user, perm->perm, dp);
		rv = 1;
		break;

	case Qraw:
		mkqid(&q, QID(DEV(c->qid), UNIT(c->qid), PART(c->qid), Qraw), 
			   unit->vers, QTFILE);
		perm = &unit->rawperm;
		if(emptystr(perm->user)){
			kstrdup(&perm->user, eve);
			perm->perm = DMEXCL|0600;
		}
		devdir(c, q, "raw", 0, perm->user, perm->perm, dp);
		rv = 1;
		break;

	case Qpart:
		pp = &unit->part[PART(c->qid)];
		l = (pp->end - pp->start) * (vlong)unit->secsize;
		mkqid(&q, QID(DEV(c->qid), UNIT(c->qid), PART(c->qid), Qpart), 
			   unit->vers+pp->vers, QTFILE);
		if(emptystr(pp->user))
			kstrdup(&pp->user, eve);
		devdir(c, q, pp->name, l, pp->user, pp->perm, dp);
		rv = 1;
		break;
	}
	
	decref(&sdev->r);
	return rv;
}

static int
sd1gen(Chan* c, int i, Dir* dp)
{
	Qid q;

	switch(i){
	case Qtopctl:
		mkqid(&q, QID(0, 0, 0, Qtopctl), 0, QTFILE);
		devdir(c, q, "sdctl", 0, eve, 0640, dp);
		return 1;
	case Qtopstat:
		mkqid(&q, QID(0, 0, 0, Qtopstat), 0, QTFILE);
		devdir(c, q, "sdstat", 0, eve, 0640, dp);
		return 1;
	}
	return -1;
}

static int
sdgen(Chan* c, char*, Dirtab*, int, int s, Dir* dp)
{
	Qid q;
	vlong l;
	int i, r;
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;

	switch(TYPE(c->qid)){
	case Qtopdir:
		if(s == DEVDOTDOT){
			mkqid(&q, QID(0, s, 0, Qtopdir), 0, QTDIR);
			sprint(up->genbuf, "#%C", sddevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}

		if(s == 0 || s == 1)
			return sd1gen(c, s + Qtopbase, dp);
		s -= 2;

		qlock(&devslock);
		for(i = 0; i != ndevs; i++){
			if(s < devs[i].nunits)
				break;
			s -= devs[i].nunits;
		}
		
		if(i == ndevs){
			/* Run of the end of the list */
			qunlock(&devslock);
			return -1;
		}

		if((sdev = devs[i].dev) == nil){
			qunlock(&devslock);
			return 0;
		}

		incref(&sdev->r);
		qunlock(&devslock);

		if((unit = sdev->unit[s]) == nil)
			if((unit = sdgetunit(sdev, s)) == nil){
				decref(&sdev->r);
				return 0;
			}

		mkqid(&q, QID(sdev->idno, s, 0, Qunitdir), 0, QTDIR);
		if(emptystr(unit->user))
			kstrdup(&unit->user, eve);
		devdir(c, q, unit->name, 0, unit->user, unit->perm, dp);
		decref(&sdev->r);
		return 1;

	case Qunitdir:
		if(s == DEVDOTDOT){
			mkqid(&q, QID(0, s, 0, Qtopdir), 0, QTDIR);
			sprint(up->genbuf, "#%C", sddevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}
		
		if((sdev = sdgetdev(DEV(c->qid))) == nil){
			devdir(c, q, "unavailable", 0, eve, 0, dp);
			return 1;
		}

		unit = sdev->unit[UNIT(c->qid)];
		qlock(&unit->ctl);

		/*
		 * Check for media change.
		 * If one has already been detected, sectors will be zero.
		 * If there is one waiting to be detected, online
		 * will return > 1.
		 * Online is a bit of a large hammer but does the job.
		 */
		if(unit->sectors == 0
		|| (unit->dev->ifc->online && unit->dev->ifc->online(unit) > 1))
			sdinitpart(unit);

		i = s+Qunitbase;
		if(i < Qpart){
			r = sd2gen(c, i, dp);
			qunlock(&unit->ctl);
			decref(&sdev->r);
			return r;
		}
		i -= Qpart;
		if(unit->part == nil || i >= unit->npart){
			qunlock(&unit->ctl);
			decref(&sdev->r);
			break;
		}
		pp = &unit->part[i];
		if(!pp->valid){
			qunlock(&unit->ctl);
			decref(&sdev->r);
			return 0;
		}
		l = (pp->end - pp->start) * (vlong)unit->secsize;
		mkqid(&q, QID(DEV(c->qid), UNIT(c->qid), i, Qpart), 
			    unit->vers+pp->vers, QTFILE);
		if(emptystr(pp->user))
			kstrdup(&pp->user, eve);
		devdir(c, q, pp->name, l, pp->user, pp->perm, dp);
		qunlock(&unit->ctl);
		decref(&sdev->r);
		return 1;
	case Qraw:
	case Qctl:
	case Qpart:
		if((sdev = sdgetdev(DEV(c->qid))) == nil){
			devdir(c, q, "unavailable", 0, eve, 0, dp);
			return 1;
		}
		unit = sdev->unit[UNIT(c->qid)];
		qlock(&unit->ctl);
		r = sd2gen(c, TYPE(c->qid), dp);
		qunlock(&unit->ctl);
		decref(&sdev->r);
		return r;
	case Qtopctl:
	case Qtopstat:
		return sd1gen(c, TYPE(c->qid), dp);
	default:
		break;
	}

	return -1;
}

static Chan*
sdattach(char* spec)
{
	Chan *c;
	char *p;
	SDev *sdev;
	int idno, subno, i;

	if(ndevs == 0 || *spec == '\0'){
		c = devattach(sddevtab.dc, spec);
		mkqid(&c->qid, QID(0, 0, 0, Qtopdir), 0, QTDIR);
		return c;
	}

	if(spec[0] != 's' || spec[1] != 'd')
		error(Ebadspec);
	idno = spec[2];
	subno = strtol(&spec[3], &p, 0);
	if(p == &spec[3])
		error(Ebadspec);

	qlock(&devslock);
	for (sdev = nil, i = 0; i != ndevs; i++)
		if((sdev = devs[i].dev) != nil && sdev->idno == idno)
			break;

	if(i == ndevs || subno >= sdev->nunit || sdgetunit(sdev, subno) == nil){
		qunlock(&devslock);
		error(Enonexist);
	}
	incref(&sdev->r);
	qunlock(&devslock);

	c = devattach(sddevtab.dc, spec);
	mkqid(&c->qid, QID(sdev->idno, subno, 0, Qunitdir), 0, QTDIR);
	c->dev = (sdev->idno << UnitLOG) + subno;
	decref(&sdev->r);
	return c;
}

static Walkqid*
sdwalk(Chan* c, Chan* nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, sdgen);
}

static int
sdstat(Chan* c, uchar* db, int n)
{
	return devstat(c, db, n, nil, 0, sdgen);
}

static Chan*
sdopen(Chan* c, int omode)
{
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;
	uchar tp;

	c = devopen(c, omode, 0, 0, sdgen);
	if((tp = TYPE(c->qid)) != Qctl && tp != Qraw && tp != Qpart)
		return c;

	sdev = sdgetdev(DEV(c->qid));
	if(sdev == nil)
		error(Enonexist);
	unit = sdev->unit[UNIT(c->qid)];

	switch(TYPE(c->qid)){
	case Qctl:
		c->qid.vers = unit->vers;
		break;
	case Qraw:
		c->qid.vers = unit->vers;
		if(tas(&unit->rawinuse) != 0){
			c->flag &= ~COPEN;
			error(Einuse);
		}
		unit->state = Rawcmd;
		break;
	case Qpart:
		qlock(&unit->ctl);
		if(waserror()){
			qunlock(&unit->ctl);
			c->flag &= ~COPEN;
			nexterror();
		}
		pp = &unit->part[PART(c->qid)];
		c->qid.vers = unit->vers+pp->vers;
		qunlock(&unit->ctl);
		poperror();
		break;
	}
	decref(&sdev->r);
	return c;
}

static void
sdclose(Chan* c)
{
	SDunit *unit;
	SDev *sdev;

	if(c->qid.type & QTDIR)
		return;
	if(!(c->flag & COPEN))
		return;

	switch(TYPE(c->qid)){
	default:
		break;
	case Qraw:
		sdev = sdgetdev(DEV(c->qid));
		if(sdev){
			unit = sdev->unit[UNIT(c->qid)];
			unit->rawinuse = 0;
			decref(&sdev->r);
		}
		break;
	}
}

static long
sdbio(Chan* c, int write, char* a, long len, vlong off)
{
	int nchange;
	long l;
	uchar *b;
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;
	ulong bno, max, nb, offset;

	sdev = sdgetdev(DEV(c->qid));
	if(sdev == nil)
		error(Enonexist);
	unit = sdev->unit[UNIT(c->qid)];
	if(unit == nil)
		error(Enonexist);

	nchange = 0;
	qlock(&unit->ctl);
	while(waserror()){
		/* notification of media change; go around again */
		if(strcmp(up->errstr, Eio) == 0 && unit->sectors == 0 && nchange++ == 0){
			sdinitpart(unit);
			continue;
		}

		/* other errors; give up */
		qunlock(&unit->ctl);
		decref(&sdev->r);
		nexterror();
	}
	pp = &unit->part[PART(c->qid)];
	if(unit->vers+pp->vers != c->qid.vers)
		error(Eio);

	/*
	 * Check the request is within bounds.
	 * Removeable drives are locked throughout the I/O
	 * in case the media changes unexpectedly.
	 * Non-removeable drives are not locked during the I/O
	 * to allow the hardware to optimise if it can; this is
	 * a little fast and loose.
	 * It's assumed that non-removeable media parameters
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
		if(write)
			error(Eio);
		qunlock(&unit->ctl);
		decref(&sdev->r);
		poperror();
		return 0;
	}
	if(!(unit->inquiry[1] & 0x80)){
		qunlock(&unit->ctl);
		poperror();
	}

	b = sdmalloc(nb*unit->secsize);
	if(b == nil)
		error(Enomem);
	if(waserror()){
		sdfree(b);
		if(!(unit->inquiry[1] & 0x80))
			decref(&sdev->r);		/* gadverdamme! */
		nexterror();
	}

	offset = off%unit->secsize;
	if(offset+len > nb*unit->secsize)
		len = nb*unit->secsize - offset;
	if(write){
		if(offset || (len%unit->secsize)){
			l = unit->dev->ifc->bio(unit, 0, 0, b, nb, bno);
			if(l < 0)
				error(Eio);
			if(l < (nb*unit->secsize)){
				nb = l/unit->secsize;
				l = nb*unit->secsize - offset;
				if(len > l)
					len = l;
			}
		}
		memmove(b+offset, a, len);
		l = unit->dev->ifc->bio(unit, 0, 1, b, nb, bno);
		if(l < 0)
			error(Eio);
		if(l < offset)
			len = 0;
		else if(len > l - offset)
			len = l - offset;
	}
	else{
		l = unit->dev->ifc->bio(unit, 0, 0, b, nb, bno);
		if(l < 0)
			error(Eio);
		if(l < offset)
			len = 0;
		else if(len > l - offset)
			len = l - offset;
		memmove(a, b+offset, len);
	}
	sdfree(b);
	poperror();

	if(unit->inquiry[1] & 0x80){
		qunlock(&unit->ctl);
		poperror();
	}

	decref(&sdev->r);
	return len;
}

static long
sdrio(SDreq* r, void* a, long n)
{
	void *data;

	if(n >= SDmaxio || n < 0)
		error(Etoobig);

	data = nil;
	if(n){
		if((data = sdmalloc(n)) == nil)
			error(Enomem);
		if(r->write)
			memmove(data, a, n);
	}
	r->data = data;
	r->dlen = n;

	if(waserror()){
		if(data != nil){
			sdfree(data);
			r->data = nil;
		}
		nexterror();
	}

	if(r->unit->dev->ifc->rio(r) != SDok)
		error(Eio);

	if(!r->write && r->rlen > 0)
		memmove(a, data, r->rlen);
	if(data != nil){
		sdfree(data);
		r->data = nil;
	}
	poperror();

	return r->rlen;
}

static long
sdread(Chan *c, void *a, long n, vlong off)
{
	char *p, *e, *buf;
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;
	ulong offset;
	int i, l, status;

	offset = off;
	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qtopstat:
		p = buf = malloc(READSTR);
		assert(p);
		e = p + READSTR;
		qlock(&devslock);
		for(i = 0; i != ndevs; i++){
			SDev *sdev = devs[i].dev;

			if(sdev->ifc->stat)
				p = sdev->ifc->stat(sdev, p, e);
			else
				p = seprint(e, "%s; no statistics available\n", sdev->name);
		}
		qunlock(&devslock);
		n = readstr(off, a, n, buf);
		free(buf);
		return n;

	case Qtopdir:
	case Qunitdir:
		return devdirread(c, a, n, 0, 0, sdgen);

	case Qctl:
		sdev = sdgetdev(DEV(c->qid));
		if(sdev == nil)
			error(Enonexist);

		unit = sdev->unit[UNIT(c->qid)];
		p = malloc(READSTR);
		l = snprint(p, READSTR, "inquiry %.48s\n",
			(char*)unit->inquiry+8);
		qlock(&unit->ctl);
		/*
		 * If there's a device specific routine it must
		 * provide all information pertaining to night geometry
		 * and the garscadden trains.
		 */
		if(unit->dev->ifc->rctl)
			l += unit->dev->ifc->rctl(unit, p+l, READSTR-l);
		if(unit->sectors == 0)
			sdinitpart(unit);
		if(unit->sectors){
			if(unit->dev->ifc->rctl == nil)
				l += snprint(p+l, READSTR-l,
					"geometry %ld %ld\n",
					unit->sectors, unit->secsize);
			pp = unit->part;
			for(i = 0; i < unit->npart; i++){
				if(pp->valid)
					l += snprint(p+l, READSTR-l,
						"part %s %lud %lud\n",
						pp->name, pp->start, pp->end);
				pp++;
			}
		}
		qunlock(&unit->ctl);
		decref(&sdev->r);
		l = readstr(offset, a, n, p);
		free(p);
		return l;

	case Qraw:
		sdev = sdgetdev(DEV(c->qid));
		if(sdev == nil)
			error(Enonexist);

		unit = sdev->unit[UNIT(c->qid)];
		qlock(&unit->raw);
		if(waserror()){
			qunlock(&unit->raw);
			decref(&sdev->r);
			nexterror();
		}
		if(unit->state == Rawdata){
			unit->state = Rawstatus;
			i = sdrio(unit->req, a, n);
		}
		else if(unit->state == Rawstatus){
			status = unit->req->status;
			unit->state = Rawcmd;
			free(unit->req);
			unit->req = nil;
			i = readnum(0, a, n, status, NUMSIZE);
		} else
			i = 0;
		qunlock(&unit->raw);
		decref(&sdev->r);
		poperror();
		return i;

	case Qpart:
		return sdbio(c, 0, a, n, off);
	}

	return 0;
}

typedef struct Confdata Confdata;
struct Confdata {
	int	on;
	char*	spec;
	DevConf	cf;
};

static void
parseswitch(Confdata* cd, char* option)
{
	if(!strcmp("on", option))
		cd->on = 1;
	else if(!strcmp("off", option))
		cd->on = 0;
	else
		error(Ebadarg);
}

static void
parsespec(Confdata* cd, char* option)
{
	if(strlen(option) > 1) 
		error(Ebadarg);
	cd->spec = option;
}

static Devport*
getnewport(DevConf* dc)
{
	Devport *p;

	p = (Devport *)malloc((dc->nports + 1) * sizeof(Devport));
	if(dc->nports > 0){
		memmove(p, dc->ports, dc->nports * sizeof(Devport));
		free(dc->ports);
	}
	dc->ports = p;
	p = &dc->ports[dc->nports++];
	p->size = -1;
	p->port = (ulong)-1;
	return p;
}

static void
parseport(Confdata* cd, char* option)
{
	char *e;
	Devport *p;

	if(cd->cf.nports == 0 || cd->cf.ports[cd->cf.nports-1].port != (ulong)-1)
		p = getnewport(&cd->cf);
	else
		p = &cd->cf.ports[cd->cf.nports-1];
	p->port = strtol(option, &e, 0);
	if(e == nil || *e != '\0')
		error(Ebadarg);
}

static void
parsesize(Confdata* cd, char* option)
{
	char *e;
	Devport *p;

	if(cd->cf.nports == 0 || cd->cf.ports[cd->cf.nports-1].size != -1)
		p = getnewport(&cd->cf);
	else
		p = &cd->cf.ports[cd->cf.nports-1];
	p->size = (int)strtol(option, &e, 0);
	if(e == nil || *e != '\0')
		error(Ebadarg);
}

static void
parseirq(Confdata* cd, char* option)
{
	char *e;

	cd->cf.intnum = strtoul(option, &e, 0);
	if(e == nil || *e != '\0')
		error(Ebadarg);
}

static void
parsetype(Confdata* cd, char* option)
{
	cd->cf.type = option;
}

static struct {
	char	*option;
	void	(*parse)(Confdata*, char*);
} options[] = {
	{ 	"switch",	parseswitch,	},
	{	"spec",		parsespec,	},
	{	"port",		parseport,	},
	{	"size",		parsesize,	},
	{	"irq",		parseirq,	},
	{	"type",		parsetype,	},
};

static long
sdwrite(Chan* c, void* a, long n, vlong off)
{
	Cmdbuf *cb;
	SDreq *req;
	SDunit *unit;
	SDev *sdev;
	ulong end, start;

	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qtopctl: {
		Confdata cd;
		char buf[256], *field[Ncmd];
		int nf, i, j;

		memset(&cd, 0, sizeof(Confdata));
		if(n > sizeof(buf)-1) n = sizeof(buf)-1;
		memmove(buf, a, n);
		buf[n] = '\0';

		cd.on = -1;
		cd.spec = '\0';
		memset(&cd.cf, 0, sizeof(DevConf));

		nf = tokenize(buf, field, Ncmd);
		for(i = 0; i < nf; i++){
			char *opt = field[i++];
			if(i >= nf)
				error(Ebadarg);
			for(j = 0; j != nelem(options); j++)
				if(!strcmp(opt, options[j].option))
					break;
					
			if(j == nelem(options))
				error(Ebadarg);
			options[j].parse(&cd, field[i]);
		}

		if(cd.on < 0) 
			error(Ebadarg);

		if(cd.on){
			if(cd.spec == '\0' || cd.cf.nports == 0 || 
			     cd.cf.intnum == 0 || cd.cf.type == nil)
				error(Ebadarg);
		}
		else{
			if(cd.spec == '\0')
				error(Ebadarg);
		}

		if(sddevtab.config == nil)
			error("No configuration function");
		sddevtab.config(cd.on, cd.spec, &cd.cf);
		break;
	}
	case Qctl:
		cb = parsecmd(a, n);
		sdev = sdgetdev(DEV(c->qid));
		if(sdev == nil)
			error(Enonexist);
		unit = sdev->unit[UNIT(c->qid)];

		qlock(&unit->ctl);
		if(waserror()){
			qunlock(&unit->ctl);
			decref(&sdev->r);
			free(cb);
			nexterror();
		}
		if(unit->vers != c->qid.vers)
			error(Eio);

		if(cb->nf < 1)
			error(Ebadctl);
		if(strcmp(cb->f[0], "part") == 0){
			if(cb->nf != 4)
				error(Ebadctl);
			if(unit->sectors == 0 && !sdinitpart(unit))
				error(Eio);
			start = strtoul(cb->f[2], 0, 0);
			end = strtoul(cb->f[3], 0, 0);
			sdaddpart(unit, cb->f[1], start, end);
		}
		else if(strcmp(cb->f[0], "delpart") == 0){
			if(cb->nf != 2 || unit->part == nil)
				error(Ebadctl);
			sddelpart(unit, cb->f[1]);
		}
		else if(unit->dev->ifc->wctl)
			unit->dev->ifc->wctl(unit, cb);
		else
			error(Ebadctl);
		qunlock(&unit->ctl);
		decref(&sdev->r);
		poperror();
		free(cb);
		break;

	case Qraw:
		sdev = sdgetdev(DEV(c->qid));
		if(sdev == nil)
			error(Enonexist);
		unit = sdev->unit[UNIT(c->qid)];
		qlock(&unit->raw);
		if(waserror()){
			qunlock(&unit->raw);
			decref(&sdev->r);
			nexterror();
		}
		switch(unit->state){
		case Rawcmd:
			if(n < 6 || n > sizeof(req->cmd))
				error(Ebadarg);
			if((req = malloc(sizeof(SDreq))) == nil)
				error(Enomem);
			req->unit = unit;
			memmove(req->cmd, a, n);
			req->clen = n;
			req->flags = SDnosense;
			req->status = ~0;

			unit->req = req;
			unit->state = Rawdata;
			break;

		case Rawstatus:
			unit->state = Rawcmd;
			free(unit->req);
			unit->req = nil;
			error(Ebadusefd);

		case Rawdata:
			if(unit->state != Rawdata)
				error(Ebadusefd);
			unit->state = Rawstatus;

			unit->req->write = 1;
			n = sdrio(unit->req, a, n);
		}
		qunlock(&unit->raw);
		decref(&sdev->r);
		poperror();
		break;
	case Qpart:
		return sdbio(c, 1, a, n, off);
	}

	return n;
}

static int
sdwstat(Chan* c, uchar* dp, int n)
{
	Dir *d;
	SDpart *pp;
	SDperm *perm;
	SDunit *unit;
	SDev *sdev;

	if(c->qid.type & QTDIR)
		error(Eperm); 

	sdev = sdgetdev(DEV(c->qid));
	if(sdev == nil)
		error(Enonexist);
	unit = sdev->unit[UNIT(c->qid)];
	qlock(&unit->ctl);
	d = nil;
	if(waserror()){
		free(d);
		qunlock(&unit->ctl);
		decref(&sdev->r);
		nexterror();
	}

	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qctl:
		perm = &unit->ctlperm;
		break;
	case Qraw:
		perm = &unit->rawperm;
		break;
	case Qpart:
		pp = &unit->part[PART(c->qid)];
		if(unit->vers+pp->vers != c->qid.vers)
			error(Enonexist);
		perm = &pp->SDperm;
		break;
	}

	if(strcmp(up->user, perm->user) && !iseve())
		error(Eperm);

	d = smalloc(sizeof(Dir)+n);
	n = convM2D(dp, n, &d[0], (char*)&d[1]);
	if(n == 0)
		error(Eshortstat);
	if(!emptystr(d[0].uid))
		kstrdup(&perm->user, d[0].uid);
	if(d[0].mode != ~0UL)
		perm->perm = (perm->perm & ~0777) | (d[0].mode & 0777);

	free(d);
	qunlock(&unit->ctl);
	decref(&sdev->r);
	poperror();
	return n;
}

static char
getspec(char base)
{
	while(1){
		int i;
		SDev *sdev;

		for(i = 0; i != ndevs; i++)
			if((sdev = devs[i].dev) != nil && (char)sdev->idno == base)
				break;

		if(i == ndevs)
			return base;
		base++;
	}
	return '\0';
}

static int
configure(char* spec, DevConf* cf)
{
	ISAConf isa;
	SDevgrp *tmpdevs;
	SDev *tail, *sdev, *(*probe)(DevConf*);
	char *p, name[32];
	int i, nnew;

	if((p = strchr(cf->type, '/')) != nil)
		*p++ = '\0';

	for(i = 0; sdifc[i] != nil; i++)
		if(!strcmp(sdifc[i]->name, cf->type))
			break;

	if(sdifc[i] == nil)
		error("type not found");
	
	if((probe = sdifc[i]->probe) == nil)
		error("No probe function");

	if(p){
		/* Try to find the card on the ISA bus.  This code really belongs
		     in sdata and I'll move it later.  Really! */
		memset(&isa, 0, sizeof(isa));
		isa.port = cf->ports[0].port;
		isa.irq = cf->intnum;

		if(pcmspecial(p, &isa) < 0)
			error("Cannot find controller");
	}

	qlock(&devslock);
	if(waserror()){
		qunlock(&devslock);
		nexterror();
	}
	
	for(i = 0; i != ndevs; i++)
		if((sdev = devs[i].dev) != nil && sdev->idno == *spec)
			break;
	if(i != ndevs)
		error(Eexist);

	if((sdev = (*probe)(cf)) == nil)
		error("Cannot probe controller");
	poperror();

	nnew = 0;
	tail = sdev;
	while(tail){
		nnew++;
		tail = tail->next;
	}
	
	tmpdevs = (SDevgrp*)malloc((ndevs + nnew) * sizeof(SDevgrp));
	memmove(tmpdevs, devs, ndevs * sizeof(SDevgrp));
	free(devs);
	devs = tmpdevs;

	while(sdev){
		/* Assign `spec' to the device */
		*spec = getspec(*spec);
		snprint(name, sizeof(name), "sd%c", *spec);
		kstrdup(&sdev->name, name);
		sdev->idno = *spec;
		sdev->unit = (SDunit **)malloc(sdev->nunit * sizeof(SDunit*));
		sdev->unitflg = (int *)malloc(sdev->nunit * sizeof(int));
		assert(sdev->unit && sdev->unitflg);

		devs[ndevs].dev = sdev;
		devs[ndevs].nunits = sdev->nunit;
		sdev = sdev->next;
		devs[ndevs].dev->next = nil;
		ndevs++;
	}

	qunlock(&devslock);
	return 0;
}

static int
unconfigure(char* spec)
{
	int i;	
	SDev *sdev;

	qlock(&devslock);
	if(waserror()){
		qunlock(&devslock);
		nexterror();
	}

	sdev = nil;
	for(i = 0; i != ndevs; i++)
		if((sdev = devs[i].dev) != nil && sdev->idno == *spec)
			break;

	if(i == ndevs)
		error(Enonexist);

	if(sdev->r.ref)
		error(Einuse);

	/* make sure no interrupts arrive anymore before removing resources */
	if(sdev->enabled && sdev->ifc->disable)
		sdev->ifc->disable(sdev);

	/* we're alone and the device tab is locked; make the device unavailable */
	memmove(&devs[i], &devs[ndevs - 1], sizeof(SDevgrp));
	memset(&devs[ndevs - 1], 0, sizeof(SDevgrp));
	ndevs--;

	qunlock(&devslock);
	poperror();

	for(i = 0; i != sdev->nunit; i++)
		if(sdev->unit[i]){
			SDunit *unit = sdev->unit[i];

			free(unit->name);
			free(unit->user);
			free(unit);
		}

	if(sdev->ifc->clear)
		sdev->ifc->clear(sdev);
	return 0;
}

static int
sdconfig(int on, char* spec, DevConf* cf)
{
	if(on)
		return configure(spec, cf);
	return unconfigure(spec);
}

Dev sddevtab = {
	'S',
	"sd",

	sdreset,
	devinit,
	devshutdown,
	sdattach,
	sdwalk,
	sdstat,
	sdopen,
	devcreate,
	sdclose,
	sdread,
	devbread,
	sdwrite,
	devbwrite,
	devremove,
	sdwstat,
	devpower,
	sdconfig,
};
