/*
 * sas-able sdscsi
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

#define	reqio(r)		(r)->unit->dev->ifc->rio(r)
#define	dprint(...)	print(__VA_ARGS__)
#define	Ticks		sys->ticks
#define	generror(...)	snprint(up->genbuf, sizeof up->genbuf, __VA_ARGS__)

enum {
	Xtimeout	= 5*1000,
	Deftimeout	= 30*1000,		/* default timeout */
	Forever		= 600*1000,		/* default raw timeout */

	/* oob signals */
	Oobspinup	= 0,
	Oobpowerloss	= 1,
};

static uvlong border = 0x0001020304050607ull;
static uvlong lorder = 0x0706050403020100ull;

uvlong
getle(uchar *t, int w)
{
	uint i;
	uvlong r;

	r = 0;
	for(i = w; i != 0; )
		r = r<<8 | t[--i];
	return r;
}

void
putle(uchar *t, uvlong r, int w)
{
	uchar *o, *f;
	uint i;

	f = (uchar*)&r;
	o = (uchar*)&lorder;
	for(i = 0; i < w; i++)
		t[o[i]] = f[i];
}

uvlong
getbe(uchar *t, int w)
{
	uint i;
	uvlong r;

	r = 0;
	for(i = 0; i < w; i++)
		r = r<<8 | t[i];
	return r;
}

void
putbe(uchar *t, uvlong r, int w)
{
	uchar *o, *f;
	uint i;

	f = (uchar*)&r;
	o = (uchar*)&border + (sizeof border-w);
	for(i = 0; i < w; i++)
		t[i] = f[o[i]];
}

static char*
unam(SDunit *u)
{
	return u->name;
}

static uint
asckey(SDreq *r)
{
	uint fmt, n;
	uchar *s;

	s = r->sense;
//	if((s[0] & 0x80) == 0){
//		dprint("%s: non-scsi sense %.2ux\n", unam(r->unit), s[0]);
//		return ~0;
//	}
	fmt = s[0] & 0x7f;
	n = 18;	/* botch should be r->slen; */
	/* spc3 §4.5.3; 0x71 is deferred. */
	if(n >= 18 && (fmt == 0x70 || fmt == 0x71))
		return (s[2] & 0xf)<<16 | s[12]<<8 | s[13];
	dprint("%s: cmd %.2ux unknown sense fmt %.2ux\n", unam(r->unit), r->cmd[0], fmt);
	return (s[2] & 0xf)<<16 | s[12]<<8 | s[13];
}

/*
 * other suspects:
 * key	asc/q
 * 02	0401	becoming ready
 * 	040b	target port in standby state
 * 	0b01	overtemp
 * 	0b0[345]	background *
 * 	0c01	write error - recovered with auto reallocation
 * 	0c02	write error - auto reallocation failed
 * 	0c03	write error - recommend reassignment
 * 	17*	recovered data
 * 	18*	recovered data
 * 	5d*	smart-style reporting (disk/smart handles)
 * 	5e*	power state change
 */
static int
classify(SDunit *u, int key)
{
	switch(key>>16){
	case 0x00:
	case 0x01:
		return SDretry;
	}

	if(key == 0x062902 || key == 0x062901 || key == 0x062900){
		dprint("%s: power on sense\n", unam(u));
		return SDretry;
	}
	if(key == 0x062800 && u->inquiry[1] & 0x80){
		dprint("%s: media change\n", unam(u));
		u->sectors = 0;
	}
	if(key == 0x020401){
		dprint("%s: becoming ready\n", unam(u));
		return SDretry;
	}
	if(key == 0x020411){
		dprint("%s: need notify (enable spinup)\n", unam(u));
		return SDspinup;
	}
	return SDcheck;
}

ulong
totk(ulong u)
{
	if(u == 0)
		u = Deftimeout;
	return Ticks + u;
}

ulong
gettotk(Sfisx *f)
{
	return totk(f->tler);
}

int
setreqto(SDreq *r, ulong tk)
{
	long ms;

	ms = TK2MS(tk - Ticks);
	if(ms < 2)
		return -1;
	if(ms > 750)
		ms = 750;
	r->timeout = Ticks + Ms2tk(ms);
	return 0;
}

static int
ereqio(Sfisx *f, SDreq *r)
{
	int rv;

	if(f == nil)
		return reqio(r);
	rv = -1;
	rlock(f);
	if(!waserror()){
		rv = reqio(r);
		poperror();
	}
	runlock(f);
	return rv;
}

void
oob(Sfisx *f, SDunit *u, int oobmsg)
{
	SDreq *r;

	r = malloc(sizeof r);
	if(r == nil)
		return;
	r->cmd[0] = 0xf0;
	r->cmd[1] = 0xca;
	r->cmd[2] = 0xfe;
	r->cmd[3] = 0xba;
	putbe(r->cmd + 4, oobmsg, 4);
	r->clen = 16;
	r->unit = u;
	r->timeout = totk(Ms2tk(Xtimeout));
	if(!waserror()){
		ereqio(f, r);
		poperror();
	}
	free(r);
}

int
scsiriox(Sfisx *f, SDreq *r)
{
	int t, s;

	r->status = ~0;
	if(r->timeout == 0)
		r->timeout = totk(Ms2tk(Forever));
	for(t = r->timeout; setreqto(r, t) != -1;){
		s = ereqio(f, r);
		if(s == SDcheck && r->flags & SDvalidsense)
			s = classify(r->unit, asckey(r));
		switch(s){
		default:
			return s;
		case SDspinup:
			print("%s: OOB\n", unam(r->unit));
			/* don't acknowledge oobspinup */
			oob(f, r->unit, Oobspinup);
		}
	}
	sdsetsense(r, SDcheck, 0x02, 0x3e, 0x02);
	return SDtimeout;
}

void
edelay(ulong ms, ulong tk)
{
	int d;

	d = TK2MS(tk - Ticks);
	if(d <= 0)
		return;
	if(d < ms)
		ms = d/2;
	if(ms <= 0)
		return;
	if(up){
		while(waserror())
			;
		tsleep(&up->sleep, return0, 0, ms);
		poperror();
	}else
		delay(ms);
}

static int
scsiexec(Sfisx *f, SDreq *r)
{
	ulong s, t;

	for(t = r->timeout; setreqto(r, t) != -1; edelay(250, t)){
		if((s = scsiriox(f, r)) != SDok)
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

/* open address fises */
enum{
	Initiator		= 0x80,
	Openaddr	= 1,
	Awms		= 0x8000,
	Smp		= 0,
	Ssp		= 1,
	Stp		= 2,
};

static void
oafis(Cfis *f, uchar *c, int type, int spd)
{
	c[0] = Initiator | type<<4 | Openaddr;
	c[1] = spd;
	if(type == Smp)
		memset(c + 2, 0xff, 2);
	else
		memmove(c + 2, f->ict, 2);
	memmove(c + 4, f->tsasaddr, 8);		/* dest "port identifier" §4.2.6 */
	memmove(c + 12, f->ssasaddr, 8);
}

static int
inquiry(SDunit *u)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.cmd[0] = 0x12;
	r.cmd[4] = 0xff;
	r.clen = 6;
	r.unit = u;
	r.timeout = totk(Ms2tk(Xtimeout));
	r.data = u->inquiry;
	r.dlen = sizeof u->inquiry;
	return scsiexec(nil, &r);
}

int
tur(SDunit *u, int timeout, uint *key)
{
	int rv;
	SDreq r;

	memset(&r, 0, sizeof r);
	r.clen = 6;
	r.unit = u;
	r.timeout = totk(timeout);
	rv = scsiexec(nil, &r);
	*key = r.status;
	if(r.flags & SDvalidsense)
		*key = asckey(&r);
	return rv;
}

static int
sasvpd(SDunit *u, uchar *buf, int l)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.cmd[0] = 0x12;
	r.cmd[1] = 1;
	r.cmd[2] = 0x80;
	r.cmd[4] = l;
	r.clen = 6;
	r.data = buf;
	r.dlen = l;
	r.unit = u;
	r.timeout = totk(Ms2tk(Xtimeout));
	return scsiexec(nil, &r);
}

static int
capacity10(SDunit *u, uchar *buf, int l)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.cmd[0] = 0x25;
	r.clen = 10;
	r.data = buf;
	r.dlen = l;
	r.unit = u;
	r.timeout = totk(Ms2tk(Xtimeout));
	return scsiexec(nil, &r);
}

static int
capacity16(SDunit *u, uchar *buf, int l)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.cmd[0] = 0x9e;
	r.cmd[1] = 0x10;
	r.cmd[13] = l;
	r.clen = 16;
	r.data = buf;
	r.dlen = l;
	r.unit = u;
	r.timeout = totk(Ms2tk(Xtimeout));
	return scsiexec(nil, &r);
}

startstop(SDunit *u, int code)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.cmd[0] = 0x1b;
	r.cmd[1] = 0<<5 | 1;		/* lun depricated */
	r.cmd[4] = code;
	r.clen = 6;
	r.unit = u;
	r.timeout = totk(Ms2tk(Xtimeout));
	return scsiexec(nil, &r);
}

static void
frmove(char *p, uchar *c, int n)
{
	char *s, *e;

	memmove(p, c, n);
	p[n] = 0;
	for(e = p + n - 1; e > p && *e == ' '; e--)
		*e = 0;
	for(s = p; *s == ' '; )
		s++;
	memmove(p, s, (e - s) + 2);
}

static void
chkinquiry(Sfisx *f, uchar *c)
{
	char buf[32], buf2[32], omod[sizeof f->model];

	memmove(omod, f->model, sizeof f->model);
	frmove(buf, c + 8, 8);
	frmove(buf2, c + 16, 16);
	snprint(f->model, sizeof f->model, "%s %s", buf, buf2);
	frmove(f->firmware, c + 23, 4);
	if(memcmp(omod, f->model, sizeof omod) != 0)
		f->drivechange = 1;
}

static void
chkvpd(Sfisx *f, uchar *c, int n)
{
	char buf[sizeof f->serial];
	int l;

	if(n > sizeof buf - 1)
		n = sizeof buf - 1;
	l = c[3];
	if(l > n)
		l = n;
	frmove(buf, c + 4, l);
	if(strcmp(buf, f->serial) != 0)
		f->drivechange = 1;
	memmove(f->serial, buf, sizeof buf);
}

static int
adjcapacity(Sfisx *f, uvlong ns, uint nss)
{
	if(ns != 0)
		ns++;
	if(nss == 2352)
		nss = 2048;
	if(f->sectors != ns || f->secsize != nss){
		f->drivechange = 1;
		f->sectors = ns;
		f->secsize = nss;
	}
	return 0;
}

static int
chkcapacity10(uchar *p, uvlong *ns, uint *nss)
{
	*ns = getbe(p, 4);
	*nss = getbe(p + 4, 4);
	return 0;
}

static int
chkcapacity16(uchar *p, uvlong *ns, uint *nss)
{
	*ns = getbe(p, 8);
	*nss = getbe(p + 8, 4);
	return 0;
}

typedef struct Spdtab Spdtab;
struct Spdtab {
	int	spd;
	char	*s;
};
Spdtab spdtab[] = {
[1]	Spd60,	"6.0gbps",
	Spd30,	"3.0gbps",
	Spd15,	"1.5gbps",
};

static int
sasspd(SDunit *u, Sfisx *f)
{
	int i, r;
	uint key;

	for(i = 1;; i++){
		if(i == nelem(spdtab))
			return SDrate;
		if(spdtab[i].spd > f->maxspd)
			continue;
		oafis(f, f->oaf, Ssp, spdtab[i].spd);
		dprint("%s: rate %s\n", unam(u), spdtab[i].s);
		/* this timeout is too long */
		if((r = tur(u, 2*Xtimeout, &key)) == SDok){
			f->sasspd = i;
			return SDok;
		}
		dprint("%s: key is %d / key=%.6ux\n", unam(u), key, key);
		if(key != SDrate || ++i == nelem(spdtab))
			return r;
	}
}

static int
scsionline0(SDunit *u, Sfisx *f)
{
	uchar buf[0x40];
	int r;
	uint nss;
	uvlong ns;

	/* todo: cap the total sasprobe time, not just cmds */
	f->sasspd = 0;
	if(f->maxspd != 0 && (r = sasspd(u, f)) != 0)
		return r;
	if((r = inquiry(u)) != SDok)
		return r;
	chkinquiry(f, u->inquiry);
	/* vpd 0x80 (unit serial) is not mandatory; spc-4 §7.7 */
	memset(buf, 0, sizeof buf);
	if(sasvpd(u, buf, sizeof buf) == SDok)
		chkvpd(f, buf, sizeof buf);
	else{
		if(f->serial[0])
			f->drivechange = 1;
		f->serial[0] = 0;
	}
	if((r = capacity10(u, buf, 8)) != SDok)
		return r;
	chkcapacity10(buf, &ns, &nss);
	if(ns == 0xffffffff){
		if((r = capacity16(u, buf, 16)) != SDok)
			return r;
		chkcapacity16(buf, &ns, &nss);
	}
	adjcapacity(f, ns, nss);
	startstop(u, 0<<4 | 1);
	return 0;
}

int
scsionlinex(SDunit *u, Sfisx *f)
{
	int r;

	wlock(f);
	if(waserror())
		r = SDeio;
	else{
		r = scsionline0(u, f);
		poperror();
	}
	wunlock(f);
	return r;
}

static int
rwcdb(Sfis*, uchar *c, int write, ulong count, uvlong lba)
{
	int is16;
	static uchar tab[2][2] = {0x28, 0x88, 0x2a, 0x8a};

	is16 = lba>0xffffffffull;
	c[0] = tab[write][is16];
	if(is16){
		putbe(c + 2, lba, 8);
		putbe(c + 10, count, 4);
		c[14] = 0;
		c[15] = 0;
		return 16;
	}else{
		putbe(c + 2, lba, 4);
		c[6] = 0;
		putbe(c + 7, count, 2);
		return 10;
	}
}

static void
setlun(uchar *c, int lun)
{
	c[1] = lun<<5;		/* wrong for anything but ancient h/w */
}

long
scsibiox(SDunit *u, Sfisx *f, int lun, int write, void *data, long count, uvlong lba)
{
	SDreq r;

	memset(&r, 0, sizeof r);
	r.unit = u;
	r.lun = lun;
	r.data = data;
	r.dlen = count * u->secsize;
	r.clen = rwcdb(f, r.cmd, write, count, lba);
	setlun(r.cmd, lun);
	r.timeout = gettotk(f);

	switch(scsiexec(f, &r)){
	case 0:
		if((r.flags & SDvalidsense) == 0 || r.sense[2] == 0)
			return r.rlen;
	default:
		generror("%s cmd %.2ux sense %.6ux", Eio, r.cmd[0], asckey(&r));
		error(up->genbuf);
		return -1;
	case SDtimeout:
		generror("%s cmd %.2ux timeout", Eio, r.cmd[0]);
		error(up->genbuf);
		return -1;
	}
}

static char*
rctlsata(Sfis *f, char *p, char *e)
{
	p = seprint(p, e, "flag\t");
	p = pflag(p, e, f);
	p = seprint(p, e, "udma\t%d\n", f->udma);
	return p;
}

static char*
rctlsas(Sfisx *f, char *p, char *e)
{
	char *s;

	s = "none";
	if(f->sasspd < nelem(spdtab))
	if(spdtab[f->sasspd].s != nil)
//	if(f->state == Dnew || f->state == Dready)
		s = spdtab[f->sasspd].s;
	p = seprint(p, e, "sasspd	%s\n", s);
	return p;
}

char*
sfisxrdctl(Sfisx *f, char *p, char *e)
{
	p = seprint(p, e, "model\t%s\n", f->model);
	p = seprint(p, e, "serial\t%s\n", f->serial);
	p = seprint(p, e, "firm\t%s\n", f->firmware);
	p = seprint(p, e, "wwn\t%llux\n", f->wwn);
	p = seprint(p, e, "tler\t%ud\n", f->tler);
	if(f->type == Sata)
		p = rctlsata(f, p, e);
	if(f->type == Sas)
		p = rctlsas(f, p, e);
	return p;
}
