/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *  Performance counters
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"amd64.h"
#include	"pmc.h"

enum{
	Qdir		= 0,
	Qgctl,
	Qcore,

	Qctr,
	Qdata,
	Qctl,

	PmcCtlRdStr = 4*1024,
};

#define PMCTYPE(x)	(((uintptr_t)x)&0xffful)
#define PMCID(x)	(((uintptr_t)x)>>12)
#define PMCQID(i, t)	((((uintptr_t)i)<<12)|(t))

Dirtab *pmctab;
static int npmctab;
Dirtab *toptab;
static int ntoptab;
int pmcdebug;

static void
topdirinit(int ncores)
{
	int i;
	Dirtab *d;

	ntoptab = 2 + ncores;
	toptab = malloc(ntoptab * sizeof(Dirtab));
	if (toptab == nil)
		return;
	d = toptab;
	strncpy(d->name, ".", KNAMELEN);
	mkqid(&d->qid, Qdir, 0, QTDIR);
	d->perm = DMDIR|0555;
	d++;
	strncpy(d->name, "ctrdesc", KNAMELEN);
	mkqid(&d->qid, Qgctl, 0, 0);
	d->perm = 0444;
	for (i = 2; i < ncores + 2; i++) {
		d = &toptab[i];
		snprint(d->name, KNAMELEN, "core%4.4ud", i - 2);
		mkqid(&d->qid, PMCQID(i - 2, Qcore), 0, QTDIR);
		d->perm = DMDIR|0555;
	}

}

static void
ctrdirinit(void)
{
	int nr, i;
	Dirtab *d;

	nr = pmcnregs();

	npmctab = 1 + 2*nr;
	pmctab = malloc(npmctab * sizeof(Dirtab));
	if (pmctab == nil){
		free(toptab);
		toptab = nil;
		return;
	}

	d = pmctab;
	strncpy(d->name, ".", KNAMELEN);
	mkqid(&d->qid, Qctr, 0, QTDIR);
	d->perm = DMDIR|0555;
	for (i = 1; i < nr + 1; i++) {
		d = &pmctab[i];
		snprint(d->name, KNAMELEN, "ctr%2.2ud", i - 1);
		mkqid(&d->qid, PMCQID(i - 1, Qdata), 0, 0);
		d->perm = 0600;

		d = &pmctab[nr + i];
		snprint(d->name, KNAMELEN, "ctr%2.2udctl", i - 1);
		mkqid(&d->qid, PMCQID(i - 1, Qctl), 0, 0);
		d->perm = 0600;
	}

}

static void
pmcnull(PmcCtl *p)
{
	memset(p, 0xff, sizeof(PmcCtl));
	p->enab = PmcCtlNullval;
	p->user = PmcCtlNullval;
	p->os = PmcCtlNullval;
	p->reset = PmcCtlNullval;
	p->nodesc = 1;
}

static void
pmcinit(void)
{
	int i, j, ncores, nr;
	Mach *mp;

	_pmcupdate = pmcupdate;
	ncores = 0;
	nr = pmcnregs();
	for(i = 0; i < MACHMAX; i++)
		if((mp = sys->machptr[i]) != nil && mp->online){
			ncores++;
			for(j = 0; j < nr; j++)
				pmcnull(&mp->pmc[j].PmcCtl);
		}
	topdirinit(ncores);
	ctrdirinit();
}

static Chan *
pmcattach(char *spec)
{
	if (pmctab == nil)
		error(Enomem);
	return devattach(L'ε', spec);
}
int
pmcgen(Chan *c, char *name, Dirtab* dir, int j, int s, Dir *dp)
{
	int t, i, n;
	Dirtab *l, *d;

	if(s == DEVDOTDOT){
		devdir(c, (Qid){Qdir, 0, QTDIR}, "#ε", 0, eve, 0555, dp);
		c->aux = nil;
		return 1;
	}
	/* first, for directories, generate children */
	switch((int)PMCTYPE(c->qid.path)){
	case Qdir:
		return devgen(c, name, toptab, ntoptab, s, dp);
	case Qctr:
		return devgen(c, name, pmctab, npmctab, s, dp);
	case Qcore:
		c->aux = (void *)PMCID(c->qid.path);		/* core no */
		return devgen(c, name, pmctab, npmctab, s, dp);
	default:
		if(s != 0)
			return -1;

		t = PMCTYPE(c->qid.path);
		if(t < Qctr){
			i = t;
			l = toptab;
			n = ntoptab;
		}else{
			i = PMCID(t);
			if (t == Qctl)
				i += (npmctab - 1)/2;
			l = pmctab;
			n = npmctab;
		}
		if(i >=n)
			return -1;

		d = &l[i];

		devdir(c, d->qid, d->name, d->length, eve, d->perm, dp);
		return 1;
	}
}

static Walkqid*
pmcwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, nil, 0, pmcgen);
}

static int32_t
pmcstat(Chan *c, uint8_t *dp, int32_t n)
{
	return devstat(c, dp, n, nil, 0, pmcgen);
}

static Chan*
pmcopen(Chan *c, int omode)
{
	if (!iseve())
		error(Eperm);
	return devopen(c, omode, nil, 0, pmcgen);
}

static void
pmcclose(Chan *c)
{
}



static int32_t
pmcread(Chan *c, void *a, int32_t n, int64_t offset)
{
	Proc *up = externup();
	uint32_t type, id;
	PmcCtl p;
	char *s;
	uint64_t v;
	uint64_t coreno;

	type = PMCTYPE(c->qid.path);
	id = PMCID(c->qid.path);

	switch(type){
	case Qcore:
	case Qdir:
	case Qctr:
		return devdirread(c, a, n, nil, 0, pmcgen);
	}

	s = malloc(PmcCtlRdStr);
	if(waserror()){
		free(s);
		nexterror();
	}
	coreno = (uint64_t)c->aux;
	p._coreno = coreno;
	switch(type){
	case Qdata:
		v = pmcgetctr(coreno, id);
		snprint(s, PmcCtlRdStr, "%#ullx", v);
		break;
	case Qctl:
		if (pmcgetctl(coreno, &p, id) < 0)
			error("bad ctr");
		if (pmcctlstr(s, PmcCtlRdStr, &p) < 0)
			error("bad pmc");
		break;
	case Qgctl:
		if (pmcdescstr(s, PmcCtlRdStr) < 0)
			error("bad pmc");
		break;
	default:
		error(Eperm);
	}
	n = readstr(offset, a, n, s);
	free(s);
	poperror();
	return n;
}

enum{
	Enable,
	Disable,
	User,
	Os,
	NoUser,
	NoOs,
	Reset,
	Debug,
};

static Cmdtab pmcctlmsg[] =
{
	Enable,		"enable",	0,
	Disable,	"disable",	0,
	User,		"user",		0,
	Os,		"os",		0,
	NoUser,		"nouser",		0,
	NoOs,		"noos",		0,
	Reset,		"reset",	0,
	Debug, 		"debug",	0,
};

typedef void (*APfunc)(void);

typedef struct AcPmcArg AcPmcArg;
struct AcPmcArg {
	int regno;
	int coreno;
	PmcCtl PmcCtl;
};

typedef struct AcCtrArg AcCtrArg;
struct AcCtrArg {
	int regno;
	int coreno;
	uint64_t v;
};

void
acpmcsetctl(void)
{
	Proc *up = externup();
	AcPmcArg p;
	Mach *mp;

	mp = up->ac;
	memmove(&p, mp->NIX.icc->data, sizeof(AcPmcArg));

	mp->NIX.icc->rc = pmcsetctl(p.coreno, &p.PmcCtl, p.regno);
	return;
}

void
acpmcsetctr(void)
{
	Proc *up = externup();
	AcCtrArg ctr;
	Mach *mp;

	mp = up->ac;
	memmove(&ctr, mp->NIX.icc->data, sizeof(AcCtrArg));

	mp->NIX.icc->rc = pmcsetctr(ctr.coreno, ctr.v, ctr.regno);
	return;
}


static int32_t
pmcwrite(Chan *c, void *a, int32_t n, int64_t mm)
{
	Proc *up = externup();
	Cmdbuf *cb;
	Cmdtab *ct;
	uint32_t type;
	char str[64];	/* 0x0000000000000000\0 */
	AcPmcArg p;
	AcCtrArg ctr;
	uint64_t coreno;
	Mach *mp;

	if (c->qid.type == QTDIR)
		error(Eperm);
	if (c->qid.path == Qgctl)
		error(Eperm);
	if (n >= sizeof(str))
		error(Ebadctl);

	pmcnull(&p.PmcCtl);
	coreno = (uint64_t)c->aux;
	p.coreno = coreno;
	type = PMCTYPE(c->qid.path);
	p.regno = PMCID(c->qid.path);
	memmove(str, a, n);
	str[n] = '\0';
	mp = up->ac;

	ctr.coreno = coreno;
	ctr.regno = p.regno;
	if (type == Qdata) {
		/* I am a handler for a proc in the core, run an RPC*/
		if (mp != nil && mp->machno == coreno) {
			if (runac(mp, acpmcsetctr, 0, &ctr, sizeof(AcCtrArg)) < 0)
				n = -1;
		} else {
		if (pmcsetctr(coreno, strtoull(str, 0, 0), p.regno) < 0)
			n = -1;
		}
		return n;
	}


	/* TODO: should iterate through multiple lines */
	if (strncmp(str, "set ", 4) == 0){
		memmove(p.PmcCtl.descstr, (char *)str + 4, n - 4);
		p.PmcCtl.descstr[n - 4] = '\0';
		p.PmcCtl.nodesc = 0;
	} else {
		cb = parsecmd(a, n);
		if(waserror()){
			free(cb);
			nexterror();
		}
		ct = lookupcmd(cb, pmcctlmsg, nelem(pmcctlmsg));
		switch(ct->index){
		case Enable:
			p.PmcCtl.enab = 1;
			break;
		case Disable:
			p.PmcCtl.enab = 0;
			break;
		case User:
			p.PmcCtl.user = 1;
			break;
		case Os:
			p.PmcCtl.os = 1;
			break;
		case NoUser:
			p.PmcCtl.user = 0;
			break;
		case NoOs:
			p.PmcCtl.os = 0;
			break;
		case Reset:
			p.PmcCtl.reset = 1;
			break;
		case Debug:
			pmcdebug = ~pmcdebug;
			break;
		default:
			cmderror(cb, "invalid ctl");
		break;
		}
		free(cb);
		poperror();
	}
	/* I am a handler for a proc in the core, run an RPC*/
	if (mp != nil && mp->machno == coreno) {
		if (runac(mp, acpmcsetctl, 0, &p, sizeof(AcPmcArg)) < 0)
			n = -1;
	} else {
		if (pmcsetctl(coreno, &p.PmcCtl, p.regno) < 0)
			n = -1;
	}
	return n;
}


Dev pmcdevtab = {
	.dc = L'ε',
	.name = "pmc",

	.reset = pmcinit,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = pmcattach,
	.walk = pmcwalk,
	.stat = pmcstat,
	.open = pmcopen,
	.create = devcreate,
	.close = pmcclose,
	.read = pmcread,
	.bread = devbread,
	.write = pmcwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
