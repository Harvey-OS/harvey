/*
 * ata analog to sdscsi
 * copyright © 2010 erik quanstrom
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/sd.h"
#include <fis.h>
#include "sdfis.h"

#define	reqio(r)		(r)->unit->dev->ifc->ataio(r)
#define	dprint(...)	print(__VA_ARGS__)

static char*
dnam(SDunit *u)
{
	return u->name;
}

static int
settxmode(SDunit *u, Sfis *f, uchar x)
{
	int t;
	SDreq r;

	memset(&r, 0, sizeof r);
	r.unit = u;
	if((t = txmodefis(f, r.cmd, x)) == -1)
		return 0;
	r.clen = 16;
	r.ataproto = t;
	r.timeout = totk(Ms2tk(1*1000));
	return reqio(&r);
}

static int
flushcache(SDunit *u, Sfis *f)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.unit = u;
	r.clen = 16;
	r.ataproto = flushcachefis(f, r.cmd);
	r.timeout = totk(Ms2tk(60*1000));
	return reqio(&r);
}

static int
setfeatures(SDunit *u, Sfis *f, uchar x, uint w)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.unit = u;
	r.clen = 16;
	r.ataproto = featfis(f, r.cmd, x);
	r.timeout = totk(w);
	return reqio(&r);
}

static int
identify1(SDunit *u, Sfis *f, void *id)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.unit = u;
	r.clen = 16;
	r.ataproto = identifyfis(f, r.cmd);
	r.data = id;
	r.dlen = 0x200;
	r.timeout = totk(Ms2tk(3*1000));
	return reqio(&r);
}

static int
identify0(SDunit *u, Sfisx *f, ushort *id)
{
	int i, n;
	vlong osectors, s;
	uchar oserial[21];

	for(i = 0;; i++){
		if(i > 5 || identify1(u, f, id) != 0)
			return -1;
		n = idpuis(id);
		if(n & Pspinup && setfeatures(u, f, 7, 20*1000) == -1)
			dprint("%s: puis spinup fail\n", dnam(u));
		if(n & Pidready)
			break;
	}

	s = idfeat(f, id);
	if(s == -1)
		return -1;
	if((f->feat&Dlba) == 0){
		dprint("%s: no lba support\n", dnam(u));
		return -1;
	}
	osectors = u->sectors;
	memmove(oserial, f->serial, sizeof f->serial);

	f->sectors = s;
	f->secsize = idss(f, id);

	idmove(f->serial, id+10, 20);
	idmove(f->firmware, id+23, 8);
	idmove(f->model, id+27, 40);
	f->wwn = idwwn(f, id);
	memset(u->inquiry, 0, sizeof u->inquiry);
	u->inquiry[2] = 2;
	u->inquiry[3] = 2;
	u->inquiry[4] = sizeof u->inquiry - 4;
	memmove(u->inquiry+8, f->model, 40);

	if(osectors != s || memcmp(oserial, f->serial, sizeof oserial)){
		f->drivechange = 1;
		u->sectors = 0;
	}
	return 0;
}

static int
identify(SDunit *u, Sfisx *f)
{
	int r;
	ushort *id;

	id = malloc(0x200);
	if(id == nil)
		error(Enomem);
	r = identify0(u, f, id);
	free(id);
	return r;
}

void
pronline(SDunit *u, Sfisx *f)
{
	char *s, *t;

	if(f->type == Sas)
		s = "sas";
	else{
		s = "lba";
		if(f->feat & Dllba)
			s = "llba";
		if(f->feat & Datapi)
			s = "atapi";
	}
	t = "";
	if(f->drivechange)
		t = "[newdrive]";
	print("%s: %s %,lld sectors\n", dnam(u), s, f->sectors);
	print("  %s %s %s %s\n", f->model, f->firmware, f->serial, t);
}

int
ataonline0(SDunit *u, Sfisx *f)
{
	if(identify(u, f) != 0){
		dprint("%s: identify failure\n", dnam(u));
		return SDeio;
	}
	if(f->feat & Dpower && setfeatures(u, f, 0x85, 3*1000)  != 0)
		f->feat &= ~Dpower;
	if(settxmode(u, f, f->udma) != 0){
		dprint("%s: can't set tx mode udma %d\n", dnam(u), f->udma);
		return SDeio;
	}
	return SDok;
}


int
ataonline(SDunit *u, Sfisx *f)
{
	int r;

	wlock(f);
	if(waserror())
		r = SDeio;
	else{
		r = ataonline0(u, f);
		poperror();
	}
	wunlock(f);
	return r;
}

static int
ereqio(Sfisx *f, SDreq *r)
{
	int rv;

	rv = -1;
	rlock(f);
	if(!waserror()){
		rv = reqio(r);
		poperror();
	}
	runlock(f);
	return rv;
}

static int
ataexec(Sfisx *f, SDreq *r)
{
	ulong s, t;

	for(t = r->timeout; setreqto(r, t) != -1; edelay(250, t)){
		if((s = ereqio(f, r)) != SDok)
			return s;
		switch(r->status){
		default:
			return r->status;
		case SDtimeout:
		case SDretry:
			continue;
		}
	}
	return -1;
}

long
atabio(SDunit* u, Sfisx *f, int lun, int write, void *d0, long count0, uvlong lba)
{
	char *data;
	uint llba, n, count;
	SDreq r;
//	Sfisx *f;

//	f = u->f;
	memset(&r, 0, sizeof r);
	r.unit = u;
	r.lun = lun;
	llba = (f->feat & Dlba) != 0;
	r.clen = 16;
	data = d0;
	r.timeout = gettotk(f);
	for(count = count0; count > 0; count -= n){
		n = count;
		if(llba && n > 65536)
			n = 65536;
		else if(!llba && n > 256)
			n = 256;
		if(n > f->atamaxxfr)
			n = f->atamaxxfr;
		r.data = data;
		r.dlen = n*f->secsize;
		r.ataproto = rwfis(f, r.cmd, write, n, lba);
		r.write = (r.ataproto & Pout) != 0;
		if(ataexec(f, &r) != SDok)
			return -1;
		data += r.dlen;
		lba += n;
	}
	return count0 * f->secsize;
}

int
atariosata(SDunit *u, Sfisx *f, SDreq *r)
{
	uchar *cmd;
	int i, n, count, rw;
	uvlong lba;

	cmd = r->cmd;
	if(cmd[0] == 0x35 || cmd[0] == 0x91){
		if(flushcache(u, f) == 0)
			return sdsetsense(r, SDok, 0, 0, 0);	/* stupid scuzz */
		return sdsetsense(r, SDcheck, 3, 0xc, 2);
	}
	if((i = sdfakescsi(r)) != SDnostatus){
		r->status = i;
		return i;
	}
	if((i = sdfakescsirw(r, &lba, &count, &rw)) != SDnostatus)
		return i;
	n = atabio(u, f, r->lun, r->write, r->data, count, lba);
	if(n == -1)
		return SDeio;
	r->rlen = n;
	return sdsetsense(r, SDok, 0, 0, 0);			/* stupid scuzz */
}
