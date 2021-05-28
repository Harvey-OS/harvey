/*
 *  Performance counters
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"pmc.h"


enum{
	Qdir		= 0,
	Qdesc,
	Qcore,
	PmcCtlRdStr = 4*1024,
};

#define PMCTYPE(x)	(((unsigned)x)&0xffful)
#define PMCID(x)	(((unsigned)x)>>12)
#define PMCQID(i, t)	((((unsigned)i)<<12)|(t))

static Dirtab *toptab;
static Lock toptablck;
static int ntoptab;
int pmcdebug;

static void
topdirinit(void)
{
	Dirtab *d;
	int nent;

	nent = 1  + MACHMAX;
	toptab = mallocz(nent * sizeof(Dirtab), 1);
	if (toptab == nil)
		return;
	d = toptab;
	strncpy(d->name, "ctrdesc", KNAMELEN);
	mkqid(&d->qid, Qdesc, 0, 0);
	d->perm = 0440;

}

static int
corefilesinit(void)
{
	int i, nc, newn;
	Dirtab *d;
	Mach *mp;

	nc = 0;
	lock(&toptablck);
	for(i = 0; i < MACHMAX; i++) {
		if((mp = sys->machptr[i]) != nil && mp->online != 0){
			d = &toptab[nc + 1];
			/* if you take them out, be careful in pmcgen too */
			if(d->name[0] != '\0'){
				if(PMCQID(i, Qcore) == d->qid.path){
					nc++;
					continue;
				}else{
					/* a new one appeared, make space, should almost never happen */
					memmove(d + 1, d, (MACHMAX - i)*sizeof(*d));
					memset(d, 0, sizeof(*d));
				}
			}
			snprint(d->name, KNAMELEN, "core%4.4ud", i);
			mkqid(&d->qid, PMCQID(i, Qcore), 0, 0);
			d->perm = 0660;
			nc++;
		}
	}
	newn = 1 + nc;
	ntoptab = newn;
	unlock(&toptablck);
	return newn;

}


static void
pmcinit(void)
{
	pmcconfigure();
	topdirinit();
	corefilesinit();
}

static Chan *
pmcattach(char *spec)
{
	corefilesinit();
	return devattach(L'ε', spec);
}

int
pmcgen(Chan *c, char *, Dirtab*, int, int s, Dir *dp)
{
	int ntab;
	Dirtab *d;

	ntab = corefilesinit();
	if(s == DEVDOTDOT){
		devdir(c, (Qid){Qdir, 0, QTDIR}, "#ε", 0, eve, 0555, dp);
		c->aux = nil;
		return 1;
	}
	/* first, for directories, generate children */
	switch((int)PMCTYPE(c->qid.path)){
	case Qdir:
	case Qcore:
		if(s >= ntab)
			return -1;
		d = &toptab[s];
		devdir(c, d->qid, d->name, d->length, eve, d->perm, dp);
		return 1;
	default:
		return -1;
	}
}

static Walkqid*
pmcwalk(Chan *c, Chan *nc, char **name, int nname)
{
	if(PMCTYPE(c->qid.path) == Qcore)
		c->aux = (void *)PMCID(c->qid.path);		/* core no */
	return devwalk(c, nc, name, nname, nil, 0, pmcgen);
}

static long
pmcstat(Chan *c, uchar *dp, long n)
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
pmcclose(Chan *)
{
}

static int
pmcctlstr(char *str, int nstr, PmcCtl *p, vlong v)
{
	int ns;

	ns = 0;
	ns += snprint(str + ns,  nstr - ns, "%#ullx ", v);
	if (p->enab && p->enab != PmcCtlNullval)
		ns += snprint(str + ns, nstr - ns, "on ");
	else
		ns += snprint(str + ns, nstr - ns, "off ");

	if (p->user && p->user != PmcCtlNullval)
		ns += snprint(str + ns, nstr - ns, "user ");
	else
		ns += snprint(str + ns, nstr - ns, "nouser ");

	if (p->os && p->user != PmcCtlNullval)
		ns += snprint(str + ns, nstr - ns, "os ");
	else
		ns += snprint(str + ns, nstr - ns, "noos ");

	/* TODO, inverse pmctrans? */
	if(!p->nodesc)
		ns += snprint(str + ns, nstr - ns, "%s", p->descstr);
	else
		ns += snprint(str + ns, nstr - ns, "no desc");
	ns += snprint(str + ns, nstr - ns, "\n");
	return ns;
}


/* this should be safe to use even if there is no core anymore */
static long
pmcread(Chan *c, void *a, long n, vlong offset)
{
	ulong type;
	PmcCtl p;
	char *s;
	u64int v;
	u64int coreno;
	int nr, i, ns, nn;

	type = PMCTYPE(c->qid.path);
	coreno = PMCID(c->qid.path);

	if(type == Qdir)
		return devdirread(c, a, n, nil, 0, pmcgen);
	s = malloc(PmcCtlRdStr);
	if(waserror()){
		free(s);
		nexterror();
	}

	p.coreno = coreno;
	nr = pmcnregs();
	switch(type){
	case Qcore:
		ns = 0;
		for(i = 0; i < nr; i ++){
			if (pmcgetctl(coreno, &p, i) < 0)
				error("bad ctr");
			if(! p.enab)
				continue;
			v = pmcgetctr(coreno, i);
			ns += snprint(s + ns, PmcCtlRdStr - ns, "%2.2ud ", i);
			nn = pmcctlstr(s + ns, PmcCtlRdStr - ns, &p, v);
			if (n < 0)
				error("bad pmc");
			ns += nn;
		}
		break;
	case Qdesc:
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

static int
isset(char *str)
{
	return strncmp(str, "-", 2) != 0;
}

static int
pickregno(int coreno)
{
	PmcCtl p;
	int nr, i;

	nr = pmcnregs();
	for(i = 0; i < nr; i++){
		if (pmcgetctl(coreno, &p, i) || p.enab)
			continue;
		return i;
	}

	return -1;
}

static int
fillctl(PmcCtl *p, Cmdbuf *cb, int start, int end)
{
	int i;

	if(end > cb->nf -1)
		end = cb->nf -1;
	for(i = start; i <= end; i++){
		if(pmcdebug != 0)
			print("setting field %d to %s\n", i, cb->f[i]);
		if(!isset(cb->f[i]))
			continue;
		else if(strcmp("on", cb->f[i]) == 0)
			p->enab = 1;
		else if(strcmp("off", cb->f[i]) == 0)
			p->enab = 0;
		else if(strcmp("user", cb->f[i]) == 0)
			p->user = 1;
		else if(strcmp("os", cb->f[i]) == 0)
			p->os = 1;
		else if(strcmp("nouser", cb->f[i]) == 0)
			p->user = 0;
		else if(strcmp("noos", cb->f[i]) == 0)
			p->os = 0;
		else
			error("bad ctl");
	}
	return 0;
}

/* this should be safe to use even if there is no core anymore */
static long
pmcwrite(Chan *c, void *a, long n, vlong)
{
	Cmdbuf *cb;
	u64int coreno;
	int regno, i, ns;
	PmcCtl p;
	char *s;

	if (c->qid.type == QTDIR)
		error(Eperm);
	if (c->qid.path == Qdesc)
		error(Eperm);

	coreno = PMCID(c->qid.path);;
	p.coreno = coreno;

	/* TODO, multiple lines? */
	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	if(cb->nf < 1)
		error("short ctl");
	if(strcmp("debug", cb->f[0]) == 0)
			pmcdebug = ~pmcdebug;
	else{
		if(cb->nf < 2)
			error("short ctl");
		if(!isset(cb->f[0])){
			/* racy, it does not reserve the core */
			regno = pickregno(coreno);
			if(regno < 0)
				error("no free regno");
			if(pmcdebug != 0)
				print("picked regno %d\n", regno);
		}else{
			regno = strtoull(cb->f[0], 0, 0);
			if(regno > pmcnregs())
				error("ctr number too big");
			if(pmcdebug != 0)
				print("setting regno %d\n", regno);
		}
		if(isset(cb->f[1]))
			pmcsetctr(coreno, strtoull(cb->f[1], 0, 0), regno);

		pmcinitctl(&p);
		fillctl(&p, cb, 2, 4);
		ns = 0;
		s = p.descstr;
		s[0] = '\0';
		for(i = 5; i < cb->nf; i++){
			if(!isset(cb->f[i]))
				continue;
			ns += snprint(s + ns, KNAMELEN - ns, "%s ", cb->f[i]);
			p.nodesc = 0;
		}
		if(pmcdebug != 0)
			print("setting desc to %s\n", p.descstr);
		pmcsetctl(coreno, &p, regno);
	}
	free(cb);
	poperror();


	return n;
}


Dev pmcdevtab = {
	L'ε',
	"pmc",

	pmcinit,
	devinit,
	devshutdown,
	pmcattach,
	pmcwalk,
	pmcstat,
	pmcopen,
	devcreate,
	pmcclose,
	pmcread,
	devbread,
	pmcwrite,
	devbwrite,
	devremove,
	devwstat,
};
