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

static QLock sdqlock;
static SDev* sdlist;
static SDunit** sdunit;
static int* sdunitflg;
static int sdnunit;

enum {
	Rawcmd,
	Rawdata,
	Rawstatus,
};

enum {
	Qtopdir		= 1,		/* top level directory */
	Qtopbase,

	Qunitdir,			/* directory per unit */
	Qunitbase,
	Qctl		= Qunitbase,
	Qlog,
	Qraw,
	Qpart,
};

#define TYPE(q)		((q).path & 0x0F)
#define PART(q)		(((q).path>>4) & 0x0F)
#define UNIT(q)		(((q).path>>8) & 0xFF)
#define QID(u, p, t)	(((u)<<8)|((p)<<4)|(t))

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
			if(strncmp(name, pp->name, NAMELEN) == 0){
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
	strncpy(pp->name, name, NAMELEN);
	strncpy(pp->user, eve, NAMELEN);
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
		if(strncmp(name, pp->name, NAMELEN) == 0)
			break;
		pp++;
	}
	if(i >= unit->npart)
		error(Ebadctl);
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
			nf = getfields(p, f, nelem(f), 1, " \t\r");
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
		/*
		 * Probe the unit only once. This decision
		 * may be a little severe and reviewed later.
		 */
		if(sdunitflg[index]){
			qunlock(&sdqlock);
			return nil;
		}
		if((unit = malloc(sizeof(SDunit))) == nil){
			qunlock(&sdqlock);
			return nil;
		}
		sdunitflg[index] = 1;

		if(sdev->enabled == 0 && sdev->ifc->enable)
			sdev->ifc->enable(sdev);
		sdev->enabled = 1;

		snprint(unit->name, NAMELEN, "%s%d", sdev->name, subno);
		strncpy(unit->user, eve, NAMELEN);
		unit->perm = 0555;
		unit->subno = subno;
		unit->dev = sdev;

		unit->log.logmask = ~0;
		unit->log.nlog = 16*1024;
		unit->log.minread = 4*1024;

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
sdreset(void)
{
	int i;
	SDev *sdev, *tail;

	/*
	 * Probe all configured controllers and make a list
	 * of devices found, accumulating a possible maximum number
	 * of units attached and marking each device with an index
	 * into the linear top-level directory array of units.
	 */
	tail = nil;
	for(i = 0; sdifc[i] != nil; i++){
		if(sdifc[i]->pnp == nil || (sdev = sdifc[i]->pnp()) == nil)
			continue;
		if(sdlist != nil)
			tail->next = sdev;
		else
			sdlist = sdev;
		for(tail = sdev; tail->next != nil; tail = tail->next){
			sdev->index = sdnunit;
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
		return;
	if((sdunit = malloc(sdnunit*sizeof(SDunit*))) == nil)
		return;
	if((sdunitflg = malloc(sdnunit*sizeof(int))) == nil){
		free(sdunit);
		sdunit = nil;
		return;
	}
	for(i = 0; sdifc[i] != nil; i++){
		/*
		 * BUG: no check is made here or later when a
		 * unit is attached that the id and name are set.
		 */
		if(sdifc[i]->id)
			sdifc[i]->id(sdlist);
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

	unit = sdunit[UNIT(c->qid)];
	switch(i){
	case Qlog:
		q = (Qid){QID(UNIT(c->qid), PART(c->qid), Qlog), unit->vers};
		perm = &unit->rawperm;
		if(perm->user[0] == '\0'){
			strncpy(perm->user, eve, NAMELEN);
			perm->perm = 0666;
		}
		devdir(c, q, "log", 0, perm->user, perm->perm, dp);
		return 1;
	case Qctl:
		q = (Qid){QID(UNIT(c->qid), PART(c->qid), Qctl), unit->vers};
		perm = &unit->ctlperm;
		if(perm->user[0] == '\0'){
			strncpy(perm->user, eve, NAMELEN);
			perm->perm = 0640;
		}
		devdir(c, q, "ctl", 0, perm->user, perm->perm, dp);
		return 1;
	case Qraw:
		q = (Qid){QID(UNIT(c->qid), PART(c->qid), Qraw), unit->vers};
		perm = &unit->rawperm;
		if(perm->user[0] == '\0'){
			strncpy(perm->user, eve, NAMELEN);
			perm->perm = CHEXCL|0600;
		}
		devdir(c, q, "raw", 0, perm->user, perm->perm, dp);
		return 1;
	case Qpart:
		pp = &unit->part[PART(c->qid)];
		l = (pp->end - pp->start) * (vlong)unit->secsize;
		q = (Qid){QID(UNIT(c->qid), PART(c->qid), Qpart), unit->vers+pp->vers};
		if(pp->user[0] == '\0')
			strncpy(pp->user, eve, NAMELEN);
		devdir(c, q, pp->name, l, pp->user, pp->perm, dp);
		return 1;
	}	
	return -1;
}

static int
sd1gen(Chan*, int i, Dir*)
{
	switch(i){
	default:
		return -1;
	}
	return -1;
}

static int
sdgen(Chan* c, Dirtab*, int, int s, Dir* dp)
{
	Qid q;
	vlong l;
	int i, r;
	SDpart *pp;
	SDunit *unit;
	char name[NAMELEN];

	switch(TYPE(c->qid)){
	case Qtopdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, 0, Qtopdir)|CHDIR, 0};
			snprint(name, NAMELEN, "#%C", sddevtab.dc);
			devdir(c, q, name, 0, eve, 0555, dp);
			return 1;
		}
		if(s < sdnunit){
			if((unit = sdunit[s]) == nil){
				if((unit = sdindex2unit(s)) == nil)
					return 0;
			}
			q = (Qid){QID(s, 0, Qunitdir)|CHDIR, 0};
			if(unit->user[0] == '\0')
				strncpy(unit->user, eve, NAMELEN);
			devdir(c, q, unit->name, 0, unit->user, unit->perm, dp);
			return 1;
		}
		s -= sdnunit;
		return sd1gen(c, s+Qtopbase, dp);
	case Qunitdir:
		if(s == DEVDOTDOT){
			q = (Qid){QID(0, 0, Qtopdir)|CHDIR, 0};
			snprint(name, NAMELEN, "#%C", sddevtab.dc);
			devdir(c, q, name, 0, eve, 0555, dp);
			return 1;
		}
		unit = sdunit[UNIT(c->qid)];
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
			return r;
		}
		i -= Qpart;
		if(unit->part == nil || i >= unit->npart){
			qunlock(&unit->ctl);
			break;
		}
		pp = &unit->part[i];
		if(!pp->valid){
			qunlock(&unit->ctl);
			return 0;
		}
		l = (pp->end - pp->start) * (vlong)unit->secsize;
		q = (Qid){QID(UNIT(c->qid), i, Qpart), unit->vers+pp->vers};
		if(pp->user[0] == '\0')
			strncpy(pp->user, eve, NAMELEN);
		devdir(c, q, pp->name, l, pp->user, pp->perm, dp);
		qunlock(&unit->ctl);
		return 1;
	case Qraw:
	case Qctl:
	case Qpart:
	case Qlog:
		unit = sdunit[UNIT(c->qid)];
		qlock(&unit->ctl);
		r = sd2gen(c, TYPE(c->qid), dp);
		qunlock(&unit->ctl);
		return r;
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

	if(sdnunit == 0 || *spec == '\0'){
		c = devattach(sddevtab.dc, spec);
		c->qid = (Qid){QID(0, 0, Qtopdir)|CHDIR, 0};
		return c;
	}

	if(spec[0] != 's' || spec[1] != 'd')
		error(Ebadspec);
	idno = spec[2];
	subno = strtol(&spec[3], &p, 0);
	if(p == &spec[3])
		error(Ebadspec);
	for(sdev = sdlist; sdev != nil; sdev = sdev->next){
		if(sdev->idno == idno && subno < sdev->nunit)
			break;
	}
	if(sdev == nil || sdgetunit(sdev, subno) == nil)
		error(Enonexist);

	c = devattach(sddevtab.dc, spec);
	c->qid = (Qid){QID(sdev->index+subno, 0, Qunitdir)|CHDIR, 0};
	c->dev = sdev->index+subno;
	return c;
}

static Chan*
sdclone(Chan* c, Chan* nc)
{
	return devclone(c, nc);
}

static int
sdwalk(Chan* c, char* name)
{
	return devwalk(c, name, nil, 0, sdgen);
}

static void
sdstat(Chan* c, char* db)
{
	devstat(c, db, nil, 0, sdgen);
}

static Chan*
sdopen(Chan* c, int omode)
{
	SDpart *pp;
	SDunit *unit;

	c = devopen(c, omode, 0, 0, sdgen);
	switch(TYPE(c->qid)){
	default:
		break;
	case Qlog:
		unit = sdunit[UNIT(c->qid)];
		logopen(&unit->log);
		break;
	case Qctl:
		unit = sdunit[UNIT(c->qid)];
		c->qid.vers = unit->vers;
		break;
	case Qraw:
		unit = sdunit[UNIT(c->qid)];
		c->qid.vers = unit->vers;
		if(!canlock(&unit->rawinuse)){
			c->flag &= ~COPEN;
			error(Einuse);
		}
		unit->state = Rawcmd;
		break;
	case Qpart:
		unit = sdunit[UNIT(c->qid)];
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
	return c;
}

static void
sdclose(Chan* c)
{
	SDunit *unit;

	if(c->qid.path & CHDIR)
		return;
	if(!(c->flag & COPEN))
		return;

	switch(TYPE(c->qid)){
	default:
		break;
	case Qlog:
		unit = sdunit[UNIT(c->qid)];
		logclose(&unit->log);
		break;
	case Qraw:
		unit = sdunit[UNIT(c->qid)];
		unlock(&unit->rawinuse);
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
	ulong bno, max, nb, offset;

	unit = sdunit[UNIT(c->qid)];

	nchange = 0;
	qlock(&unit->ctl);
	while(waserror()){
		/* notification of media change; go around again */
		if(strcmp(up->error, Eio) == 0 && unit->sectors == 0 && nchange++ == 0){
			sdinitpart(unit);
			continue;
		}

		/* other errors; give up */
		qunlock(&unit->ctl);
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
		poperror();
		return 0;
	}
	if(!(unit->inquiry[1] & 0x80)){
		qunlock(&unit->ctl);
		poperror();
	}

	if(unit->log.opens) {
		int i;
		uchar lbuf[1+4+8], *p;
		ulong x[3];

		p = lbuf;
		*p++ = write ? 'w' : 'r';
		x[0] = off>>32;
		x[1] = off;
		x[2] = len;
		for(i=0; i<3; i++) {
			*p++ = x[i]>>24;
			*p++ = x[i]>>16;
			*p++ = x[i]>>8;
			*p++ = x[i];
		}

		logn(&unit->log, 1, lbuf, 1+4+8);
	}

	b = sdmalloc(nb*unit->secsize);
	if(b == nil)
		error(Enomem);
	if(waserror()){
		sdfree(b);
		nexterror();
	}

	offset = off%unit->secsize;
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
	else {
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
	char *p;
	SDpart *pp;
	SDunit *unit;
	ulong offset;
	int i, l, status;

	offset = off;
	switch(TYPE(c->qid)){
	default:
		error(Eperm);
	case Qtopdir:
	case Qunitdir:
		return devdirread(c, a, n, 0, 0, sdgen);
	case Qlog:
		unit = sdunit[UNIT(c->qid)];
		return logread(&unit->log, a, 0, n);
	case Qctl:
		unit = sdunit[UNIT(c->qid)];
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
						"part %.*s %lud %lud\n",
						NAMELEN, pp->name,
						pp->start, pp->end);
				pp++;
			}
		}
		qunlock(&unit->ctl);
		l = readstr(offset, a, n, p);
		free(p);
		return l;
	case Qraw:
		unit = sdunit[UNIT(c->qid)];
		qlock(&unit->raw);
		if(waserror()){
			qunlock(&unit->raw);
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
		poperror();
		return i;
	case Qpart:
		return sdbio(c, 0, a, n, off);
	}

	return 0;
}

static long
sdwrite(Chan *c, void *a, long n, vlong off)
{
	Cmdbuf *cb;
	SDreq *req;
	SDunit *unit;
	ulong end, start;

	switch(TYPE(c->qid)){
	default:
		error(Eperm);

	case Qlog:
		error(Ebadctl);

	case Qctl:
		cb = parsecmd(a, n);
		unit = sdunit[UNIT(c->qid)];

		qlock(&unit->ctl);
		if(waserror()){
			qunlock(&unit->ctl);
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
		poperror();
		free(cb);
		break;

	case Qraw:
		unit = sdunit[UNIT(c->qid)];
		qlock(&unit->raw);
		if(waserror()){
			qunlock(&unit->raw);
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
		poperror();
		return n;

	case Qpart:
		return sdbio(c, 1, a, n, off);
	}

	return n;
}

static void
sdwstat(Chan* c, char* dp)
{
	Dir d;
	SDpart *pp;
	SDperm *perm;
	SDunit *unit;

	if(c->qid.path & CHDIR)
		error(Eperm);

	unit = sdunit[UNIT(c->qid)];
	qlock(&unit->ctl);
	if(waserror()){
		qunlock(&unit->ctl);
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

	if(strncmp(up->user, perm->user, NAMELEN) && !iseve())
		error(Eperm);
	convM2D(dp, &d);
	strncpy(perm->user, d.uid, NAMELEN);
	perm->perm = (perm->perm & ~0777) | (d.mode & 0777);

	qunlock(&unit->ctl);
	poperror();
}

Dev sddevtab = {
	'S',
	"sd",

	sdreset,
	devinit,
	sdattach,
	sdclone,
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
};
