/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

static char Echange[] = "media or partition has changed";

static char devletters[] = "0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static SDev *devs[sizeof devletters-1];
static QLock devslock;

enum {
	Rawcmd,
	Rawdata,
	Rawstatus,
};

enum {
	Qtopdir		= 1,		/* top level directory */
	Qtopbase,
	Qtopctl		 = Qtopbase,

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
	DevMASK		= (NDev-1),
	DevSHIFT	 = (UnitLOG+PartLOG+TypeLOG),

	Ncmd = 20,
};

#define TYPE(q)		((((uint32_t)(q).path)>>TypeSHIFT) & TypeMASK)
#define PART(q)		((((uint32_t)(q).path)>>PartSHIFT) & PartMASK)
#define UNIT(q)		((((uint32_t)(q).path)>>UnitSHIFT) & UnitMASK)
#define DEV(q)		((((uint32_t)(q).path)>>DevSHIFT) & DevMASK)
#define QID(d,u, p, t)	(((d)<<DevSHIFT)|((u)<<UnitSHIFT)|\
					 ((p)<<PartSHIFT)|((t)<<TypeSHIFT))


void
sdaddpart(SDunit* unit, char* name, uint64_t start, uint64_t end)
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
			if(strcmp(name, pp->SDperm.name) == 0){
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
	kstrdup(&pp->SDperm.name, name);
	kstrdup(&pp->SDperm.user, eve);
	pp->SDperm.perm = 0640;
	pp->valid = 1;
}

static void
sddelpart(SDunit* unit, char* name)
{
	Proc *up = externup();
	int i;
	SDpart *pp;
	/*
	 * Look for the partition to delete.
	 * Can't delete if someone still has it open.
	 */
	pp = unit->part;
	for(i = 0; i < unit->npart; i++){
		if(strcmp(name, pp->SDperm.name) == 0)
			break;
		pp++;
	}
	if(i >= unit->npart)
		error(Ebadctl);
	if(strcmp(up->user, pp->SDperm.user) && !iseve())
		error(Eperm);
	pp->valid = 0;
	pp->vers++;
}

static void
sdincvers(SDunit *unit)
{
	int i;

	unit->vers++;
	if(unit->part){
		for(i = 0; i < unit->npart; i++){
			unit->part[i].valid = 0;
			unit->part[i].vers++;
		}
	}
}

static int
sdinitpart(SDunit* unit)
{
#if 0
	Mach *m;
	int nf;
	uint64_t start, end;
	char *f[4], *p, *q, buf[10];

	m = machp();
#endif
	if(unit->sectors > 0){
		unit->sectors = unit->secsize = 0;
		sdincvers(unit);
	}

	/* device must be connected or not; other values are trouble */
	if(unit->inquiry[0] & 0xC0)	/* see SDinq0periphqual */
		return 0;
	switch(unit->inquiry[0] & SDinq0periphtype){
	case SDperdisk:
	case SDperworm:
	case SDpercd:
	case SDpermo:
		break;
	default:
		return 0;
	}

	if(unit->dev->ifc->online)
		unit->dev->ifc->online(unit);
	if(unit->sectors){
		sdincvers(unit);
		sdaddpart(unit, "data", 0, unit->sectors);

		/*
		 * Use partitions passed from boot program,
		 * e.g.
		 *	sdC0part=dos 63 123123/plan9 123123 456456
		 * This happens before /boot sets hostname so the
		 * partitions will have the null-string for user.
		 * The gen functions patch it up.
		 */
#if 0
		snprint(buf, sizeof buf, "%spart", unit->SDperm.name);
		for(p = getconf(buf); p != nil; p = q){
			if(q = strchr(p, '/'))
				*q++ = '\0';
			nf = tokenize(p, f, nelem(f));
			if(nf < 3)
				continue;

			start = strtoull(f[1], 0, 0);
			end = strtoull(f[2], 0, 0);
			if(!waserror()){
				sdaddpart(unit, f[0], start, end);
				poperror();
			}
		}
#endif
	}

	return 1;
}

static int
sdindex(int idno)
{
	char *p;

	p = strchr(devletters, idno);
	if(p == nil)
		return -1;
	return p-devletters;
}

static SDev*
sdgetdev(int idno)
{
	SDev *sdev;
	int i;

	if((i = sdindex(idno)) < 0)
		return nil;

	qlock(&devslock);
	if(sdev = devs[i])
		incref(&sdev->r);
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
		kstrdup(&unit->SDperm.name, buf);
		kstrdup(&unit->SDperm.user, eve);
		unit->SDperm.perm = 0555;
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
	SDev *sdev;

	/*
	 * Probe all known controller types and register any devices found.
	 */
	for(i = 0; sdifc[i] != nil; i++){
		if(sdifc[i]->pnp == nil || (sdev = sdifc[i]->pnp()) == nil)
			continue;
		sdadddevs(sdev);
	}
}

void
sdadddevs(SDev *sdev)
{
	int i, j, id;
	SDev *next;

	for(; sdev; sdev=next){
		next = sdev->next;

		sdev->unit = (SDunit**)malloc(sdev->nunit * sizeof(SDunit*));
		sdev->unitflg = (int*)malloc(sdev->nunit * sizeof(int));
		if(sdev->unit == nil || sdev->unitflg == nil){
			print("sdadddevs: out of memory\n");
		giveup:
			free(sdev->unit);
			free(sdev->unitflg);
			if(sdev->ifc->clear)
				sdev->ifc->clear(sdev);
			free(sdev);
			continue;
		}
		id = sdindex(sdev->idno);
		if(id == -1){
			print("sdadddevs: bad id number %d (%C)\n", id, id);
			goto giveup;
		}
		qlock(&devslock);
		for(i=0; i<nelem(devs); i++){
			if(devs[j = (id+i)%nelem(devs)] == nil){
				sdev->idno = devletters[j];
				devs[j] = sdev;
				snprint(sdev->name, sizeof sdev->name, "sd%c", devletters[j]);
				break;
			}
		}
		qunlock(&devslock);
		if(i == nelem(devs)){
			print("sdadddevs: out of device letters\n");
			goto giveup;
		}
	}
}

// void
// sdrmdevs(SDev *sdev)
// {
// 	char buf[2];
//
// 	snprint(buf, sizeof buf, "%c", sdev->idno);
// 	unconfigure(buf);
// }

void
sdaddallconfs(void (*addconf)(SDunit *))
{
	int i, u;
	SDev *sdev;

	for(i = 0; i < nelem(devs); i++)		/* each controller */
		for(sdev = devs[i]; sdev; sdev = sdev->next)
			for(u = 0; u < sdev->nunit; u++)	/* each drive */
				(*addconf)(sdev->unit[u]);
}

static int
sd2gen(Chan* c, int i, Dir* dp)
{
	Qid q;
	uint64_t l;
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
			perm->perm = 0644;	/* nothing secret in ctl */
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
		l = (pp->end - pp->start) * unit->secsize;
		mkqid(&q, QID(DEV(c->qid), UNIT(c->qid), PART(c->qid), Qpart),
			unit->vers+pp->vers, QTFILE);
		if(emptystr(pp->SDperm.user))
			kstrdup(&pp->SDperm.user, eve);
		devdir(c, q, pp->SDperm.name, l, pp->SDperm.user, pp->SDperm.perm, dp);
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
		devdir(c, q, "sdctl", 0, eve, 0644, dp);	/* no secrets */
		return 1;
	}
	return -1;
}

static int
sdgen(Chan* c, char* d, Dirtab* dir, int j, int s, Dir* dp)
{
	Proc *up = externup();
	Qid q;
	int64_t l;
	int i, r;
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;

	switch(TYPE(c->qid)){
	case Qtopdir:
		if(s == DEVDOTDOT){
			mkqid(&q, QID(0, 0, 0, Qtopdir), 0, QTDIR);
			sprint(up->genbuf, "#%C", sddevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}

		if(s+Qtopbase < Qunitdir)
			return sd1gen(c, s+Qtopbase, dp);
		s -= (Qunitdir-Qtopbase);

		qlock(&devslock);
		for(i=0; i<nelem(devs); i++){
			if(devs[i]){
				if(s < devs[i]->nunit)
					break;
				s -= devs[i]->nunit;
			}
		}

		if(i == nelem(devs)){
			/* Run off the end of the list */
			qunlock(&devslock);
			return -1;
		}

		if((sdev = devs[i]) == nil){
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
		if(emptystr(unit->SDperm.user))
			kstrdup(&unit->SDperm.user, eve);
		devdir(c, q, unit->SDperm.name, 0, unit->SDperm.user, unit->SDperm.perm, dp);
		decref(&sdev->r);
		return 1;

	case Qunitdir:
		if(s == DEVDOTDOT){
			mkqid(&q, QID(0, 0, 0, Qtopdir), 0, QTDIR);
			sprint(up->genbuf, "#%C", sddevtab.dc);
			devdir(c, q, up->genbuf, 0, eve, 0555, dp);
			return 1;
		}

		if((sdev = sdgetdev(DEV(c->qid))) == nil){
			devdir(c, c->qid, "unavailable", 0, eve, 0, dp);
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
		l = (pp->end - pp->start) * (int64_t)unit->secsize;
		mkqid(&q, QID(DEV(c->qid), UNIT(c->qid), i, Qpart),
			unit->vers+pp->vers, QTFILE);
		if(emptystr(pp->SDperm.user))
			kstrdup(&pp->SDperm.user, eve);
		devdir(c, q, pp->SDperm.name, l, pp->SDperm.user, pp->SDperm.perm, dp);
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
	int idno, subno;

	if(*spec == '\0'){
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

	if((sdev=sdgetdev(idno)) == nil)
		error(Enonexist);
	if(sdgetunit(sdev, subno) == nil){
		decref(&sdev->r);
		error(Enonexist);
	}

	c = devattach(sddevtab.dc, spec);
	mkqid(&c->qid, QID(sdev->idno, subno, 0, Qunitdir), 0, QTDIR);
	c->devno = (sdev->idno << UnitLOG) + subno;
	decref(&sdev->r);
	return c;
}

static Walkqid*
sdwalk(Chan* c, Chan* nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, sdgen);
}

static int32_t
sdstat(Chan* c, uint8_t* db, int32_t n)
{
	return devstat(c, db, n, nil, 0, sdgen);
}

static Chan*
sdopen(Chan* c, int omode)
{
	Proc *up = externup();
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;
	uint8_t tp;

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
		if(TAS(&unit->rawinuse) != 0){
			c->flag &= ~COPEN;
			decref(&sdev->r);
			error(Einuse);
		}
		unit->state = Rawcmd;
		break;
	case Qpart:
		qlock(&unit->ctl);
		if(waserror()){
			qunlock(&unit->ctl);
			c->flag &= ~COPEN;
			decref(&sdev->r);
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

static int32_t
sdbio(Chan* c, int write, char* a, int32_t len, int64_t off)
{
	Proc *up = externup();
	int nchange;
	uint8_t *b;
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;
	int64_t bno;
	int32_t l, max, nb, offset;

	sdev = sdgetdev(DEV(c->qid));
	if(sdev == nil){
		decref(&sdev->r);
		error(Enonexist);
	}
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
		error(Echange);

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
	if(!(unit->inquiry[1] & SDinq1removable)){
		qunlock(&unit->ctl);
		poperror();
	}

	b = sdmalloc(nb*unit->secsize);
	if(b == nil)
		error(Enomem);
	if(waserror()){
		sdfree(b);
		if(!(unit->inquiry[1] & SDinq1removable))
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

	if(unit->inquiry[1] & SDinq1removable){
		qunlock(&unit->ctl);
		poperror();
	}

	decref(&sdev->r);
	return len;
}

static int32_t
sdrio(SDreq* r, void* a, int32_t n)
{
	Proc *up = externup();
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
		sdfree(data);
		r->data = nil;
		nexterror();
	}

	if(r->unit->dev->ifc->rio(r) != SDok)
		error(Eio);

	if(!r->write && r->rlen > 0)
		memmove(a, data, r->rlen);
	sdfree(data);
	r->data = nil;
	poperror();

	return r->rlen;
}

/*
 * SCSI simulation for non-SCSI devices
 */
int
sdsetsense(SDreq *r, int status, int key, int asc, int ascq)
{
	int len;
	SDunit *unit;

	unit = r->unit;
	unit->sense[2] = key;
	unit->sense[12] = asc;
	unit->sense[13] = ascq;

	r->status = status;
	if(status == SDcheck && !(r->flags & SDnosense)){
		/* request sense case from sdfakescsi */
		len = sizeof unit->sense;
		if(len > sizeof r->sense-1)
			len = sizeof r->sense-1;
		memmove(r->sense, unit->sense, len);
		unit->sense[2] = 0;
		unit->sense[12] = 0;
		unit->sense[13] = 0;
		r->flags |= SDvalidsense;
		return SDok;
	}
	return status;
}

int
sdmodesense(SDreq *r, uint8_t *cmd, void *info, int ilen)
{
	int len;
	uint8_t *data;

	/*
	 * Fake a vendor-specific request with page code 0,
	 * return the drive info.
	 */
	if((cmd[2] & 0x3F) != 0 && (cmd[2] & 0x3F) != 0x3F)
		return sdsetsense(r, SDcheck, 0x05, 0x24, 0);
	len = (cmd[7]<<8)|cmd[8];
	if(len == 0)
		return SDok;
	if(len < 8+ilen)
		return sdsetsense(r, SDcheck, 0x05, 0x1A, 0);
	if(r->data == nil || r->dlen < len)
		return sdsetsense(r, SDcheck, 0x05, 0x20, 1);
	data = r->data;
	memset(data, 0, 8);
	data[0] = ilen>>8;
	data[1] = ilen;
	if(ilen)
		memmove(data+8, info, ilen);
	r->rlen = 8+ilen;
	return sdsetsense(r, SDok, 0, 0, 0);
}

int
sdfakescsi(SDreq *r, void *info, int ilen)
{
	uint8_t *cmd, *p;
	uint64_t len;
	SDunit *unit;

	cmd = r->cmd;
	r->rlen = 0;
	unit = r->unit;

	/*
	 * Rewrite read(6)/write(6) into read(10)/write(10).
	 */
	switch(cmd[0]){
	case 0x08:	/* read */
	case 0x0A:	/* write */
		cmd[9] = 0;
		cmd[8] = cmd[4];
		cmd[7] = 0;
		cmd[6] = 0;
		cmd[5] = cmd[3];
		cmd[4] = cmd[2];
		cmd[3] = cmd[1] & 0x0F;
		cmd[2] = 0;
		cmd[1] &= 0xE0;
		cmd[0] |= 0x20;
		break;
	}

	/*
	 * Map SCSI commands into ATA commands for discs.
	 * Fail any command with a LUN except INQUIRY which
	 * will return 'logical unit not supported'.
	 */
	if((cmd[1]>>5) && cmd[0] != 0x12)
		return sdsetsense(r, SDcheck, 0x05, 0x25, 0);

	switch(cmd[0]){
	default:
		return sdsetsense(r, SDcheck, 0x05, 0x20, 0);

	case 0x00:	/* test unit ready */
		return sdsetsense(r, SDok, 0, 0, 0);

	case 0x03:	/* request sense */
		if(cmd[4] < sizeof unit->sense)
			len = cmd[4];
		else
			len = sizeof unit->sense;
		if(r->data && r->dlen >= len){
			memmove(r->data, unit->sense, len);
			r->rlen = len;
		}
		return sdsetsense(r, SDok, 0, 0, 0);

	case 0x12:	/* inquiry */
		if(cmd[4] < sizeof unit->inquiry)
			len = cmd[4];
		else
			len = sizeof unit->inquiry;
		if(r->data && r->dlen >= len){
			memmove(r->data, unit->inquiry, len);
			r->rlen = len;
		}
		return sdsetsense(r, SDok, 0, 0, 0);

	case 0x1B:	/* start/stop unit */
		/*
		 * nop for now, can use power management later.
		 */
		return sdsetsense(r, SDok, 0, 0, 0);

	case 0x25:	/* read capacity */
		if((cmd[1] & 0x01) || cmd[2] || cmd[3])
			return sdsetsense(r, SDcheck, 0x05, 0x24, 0);
		if(r->data == nil || r->dlen < 8)
			return sdsetsense(r, SDcheck, 0x05, 0x20, 1);

		/*
		 * Read capacity returns the LBA of the last sector.
		 */
		len = unit->sectors - 1;
		p = r->data;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		len = 512;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		r->rlen = p - (uint8_t*)r->data;
		return sdsetsense(r, SDok, 0, 0, 0);

	case 0x9E:	/* long read capacity */
		if((cmd[1] & 0x01) || cmd[2] || cmd[3])
			return sdsetsense(r, SDcheck, 0x05, 0x24, 0);
		if(r->data == nil || r->dlen < 8)
			return sdsetsense(r, SDcheck, 0x05, 0x20, 1);
		/*
		 * Read capcity returns the LBA of the last sector.
		 */
		len = unit->sectors - 1;
		p = r->data;
		*p++ = len>>56;
		*p++ = len>>48;
		*p++ = len>>40;
		*p++ = len>>32;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		len = 512;
		*p++ = len>>24;
		*p++ = len>>16;
		*p++ = len>>8;
		*p++ = len;
		r->rlen = p - (uint8_t*)r->data;
		return sdsetsense(r, SDok, 0, 0, 0);

	case 0x5A:	/* mode sense */
		return sdmodesense(r, cmd, info, ilen);

	case 0x28:	/* read */
	case 0x2A:	/* write */
	case 0x88:	/* read16 */
	case 0x8a:	/* write16 */
		return SDnostatus;
	}
}

static int32_t
sdread(Chan *c, void *a, int32_t n, int64_t off)
{
	Proc *up = externup();
	char *p, *e, *buf;
	SDpart *pp;
	SDunit *unit;
	SDev *sdev;
	int32_t offset;
	int i, l, mm, status;

	offset = off;
	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qtopctl:
		mm = 64*1024;	/* room for register dumps */
		p = buf = malloc(mm);
		if(p == nil)
			error(Enomem);
		e = p + mm;
		qlock(&devslock);
		for(i = 0; i < nelem(devs); i++){
			sdev = devs[i];
			if(sdev && sdev->ifc->rtopctl)
				p = sdev->ifc->rtopctl(sdev, p, e);
		}
		qunlock(&devslock);
		n = readstr(offset, a, n, buf);
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
		mm = 16*1024;	/* room for register dumps */
		p = malloc(mm);
		if(p == nil)
			error(Enomem);
		l = snprint(p, mm, "inquiry %.48s\n",
			(char*)unit->inquiry+8);
		qlock(&unit->ctl);
		/*
		 * If there's a device specific routine it must
		 * provide all information pertaining to night geometry
		 * and the garscadden trains.
		 */
		if(unit->dev->ifc->rctl)
			l += unit->dev->ifc->rctl(unit, p+l, mm-l);
		if(unit->sectors == 0)
			sdinitpart(unit);
		if(unit->sectors){
			if(unit->dev->ifc->rctl == nil)
				l += snprint(p+l, mm-l,
					"geometry %llud %lu\n",
					unit->sectors, unit->secsize);
			pp = unit->part;
			for(i = 0; i < unit->npart; i++){
				if(pp->valid)
					l += snprint(p+l, mm-l,
						"part %s %llud %llud\n",
						pp->SDperm.name, pp->start, pp->end);
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
}

static void legacytopctl(Cmdbuf*);

static int32_t
sdwrite(Chan* c, void* a, int32_t n, int64_t off)
{
	Proc *up = externup();
	char *f0;
	int i;
	uint64_t end, start;
	Cmdbuf *cb;
	SDifc *ifc;
	SDreq *req;
	SDunit *unit;
	SDev *sdev;

	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qtopctl:
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		if(cb->nf == 0)
			error("empty control message");
		f0 = cb->f[0];
		cb->f++;
		cb->nf--;
		if(strcmp(f0, "config") == 0){
			/* wormhole into ugly legacy interface */
			legacytopctl(cb);
			poperror();
			free(cb);
			break;
		}
		/*
		 * "ata arg..." invokes sdifc[i]->wtopctl(nil, cb),
		 * where sdifc[i]->SDperm.name=="ata" and cb contains the args.
		 */
		ifc = nil;
		sdev = nil;
		for(i=0; sdifc[i]; i++){
			if(strcmp(sdifc[i]->name, f0) == 0){
				ifc = sdifc[i];
				sdev = nil;
				goto subtopctl;
			}
		}
		/*
		 * "sd1 arg..." invokes sdifc[i]->wtopctl(sdev, cb),
		 * where sdifc[i] and sdev match controller letter "1",
		 * and cb contains the args.
		 */
		if(f0[0]=='s' && f0[1]=='d' && f0[2] && f0[3] == 0){
			if((sdev = sdgetdev(f0[2])) != nil){
				ifc = sdev->ifc;
				goto subtopctl;
			}
		}
		error("unknown interface");

	subtopctl:
		if(waserror()){
			if(sdev)
				decref(&sdev->r);
			nexterror();
		}
		if(ifc->wtopctl)
			ifc->wtopctl(sdev, cb);
		else
			error(Ebadctl);
		poperror();
		poperror();
		if (sdev)
			decref(&sdev->r);
		free(cb);
		break;

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
			error(Echange);

		if(cb->nf < 1)
			error(Ebadctl);
		if(strcmp(cb->f[0], "part") == 0){
			if(cb->nf != 4)
				error(Ebadctl);
			if(unit->sectors == 0 && !sdinitpart(unit))
				error(Eio);
			start = strtoull(cb->f[2], 0, 0);
			end = strtoull(cb->f[3], 0, 0);
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

static int32_t
sdwstat(Chan* c, uint8_t* dp, int32_t n)
{
	Proc *up = externup();
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
	if(d[0].mode != (uint32_t)~0UL)
		perm->perm = (perm->perm & ~0777) | (d[0].mode & 0777);

	free(d);
	qunlock(&unit->ctl);
	decref(&sdev->r);
	poperror();
	return n;
}

static int
configure(char* spec, DevConf* cf)
{
	SDev *s, *sdev;
	char *p;
	int i;

	if(sdindex(*spec) < 0)
		error("bad sd spec");

	if((p = strchr(cf->type, '/')) != nil)
		*p++ = '\0';

	for(i = 0; sdifc[i] != nil; i++)
		if(strcmp(sdifc[i]->name, cf->type) == 0)
			break;
	if(sdifc[i] == nil)
		error("sd type not found");
	if(p)
		*(p-1) = '/';

	if(sdifc[i]->probe == nil)
		error("sd type cannot probe");

	sdev = sdifc[i]->probe(cf);
	for(s=sdev; s; s=s->next)
		s->idno = *spec;
	sdadddevs(sdev);
	return 0;
}

static int
unconfigure(char* spec)
{
	int i;
	SDev *sdev;
	SDunit *unit;

	if((i = sdindex(*spec)) < 0)
		error(Enonexist);

	qlock(&devslock);
	if((sdev = devs[i]) == nil){
		qunlock(&devslock);
		error(Enonexist);
	}
	if(sdev->r.ref){
		qunlock(&devslock);
		error(Einuse);
	}
	devs[i] = nil;
	qunlock(&devslock);

	/* make sure no interrupts arrive anymore before removing resources */
	if(sdev->enabled && sdev->ifc->disable)
		sdev->ifc->disable(sdev);

	for(i = 0; i != sdev->nunit; i++){
		if(unit = sdev->unit[i]){
			free(unit->SDperm.name);
			free(unit->SDperm.user);
			free(unit);
		}
	}

	if(sdev->ifc->clear)
		sdev->ifc->clear(sdev);
	free(sdev);
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
	.dc = 'S',
	.name = "sd",

	.reset = sdreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = sdattach,
	.walk = sdwalk,
	.stat = sdstat,
	.open = sdopen,
	.create = devcreate,
	.close = sdclose,
	.read = sdread,
	.bread = devbread,
	.write = sdwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = sdwstat,
	.power = devpower,
	.config = sdconfig,	/* probe; only called for pcmcia-like devices */
};

/*
 * This is wrong for so many reasons.  This code must go.
 */
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
	if(p == nil)
		error(Enomem);
	if(dc->nports > 0){
		memmove(p, dc->ports, dc->nports * sizeof(Devport));
		free(dc->ports);
	}
	dc->ports = p;
	p = &dc->ports[dc->nports++];
	p->size = -1;
	p->port = (uint32_t)-1;
	return p;
}

static void
parseport(Confdata* cd, char* option)
{
	char *e;
	Devport *p;

	if(cd->cf.nports == 0 || cd->cf.ports[cd->cf.nports-1].port != (uint32_t)-1)
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
	char	*name;
	void	(*parse)(Confdata*, char*);
} options[] = {
	"switch",	parseswitch,
	"spec",		parsespec,
	"port",		parseport,
	"size",		parsesize,
	"irq",		parseirq,
	"type",		parsetype,
};

static void
legacytopctl(Cmdbuf *cb)
{
	char *opt;
	int i, j;
	Confdata cd;

	memset(&cd, 0, sizeof cd);
	cd.on = -1;
	for(i=0; i<cb->nf; i+=2){
		if(i+2 > cb->nf)
			error(Ebadarg);
		opt = cb->f[i];
		for(j=0; j<nelem(options); j++)
			if(strcmp(opt, options[j].name) == 0){
				options[j].parse(&cd, cb->f[i+1]);
				break;
			}
		if(j == nelem(options))
			error(Ebadarg);
	}
	/* this has been rewritten to accomodate sdaoe */
	if(cd.on < 0 || cd.spec == 0)
		error(Ebadarg);
	if(cd.on && cd.cf.type == nil)
		error(Ebadarg);
	sdconfig(cd.on, cd.spec, &cd.cf);
}
