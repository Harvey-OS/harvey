#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"../port/tube.h"

typedef struct Tio Tio;

enum
{
	Qdir,
	Qctl,
	Qdata
};

/*
 * data read from c is sent to tin;
 * data received from tout is written into c
 */
struct Tio
{
	Ref;
	ulong	path;
	char*	user;
	ulong	perm;
	char*	conf;
	Proc*	uproc;
	Proc*	tinproc;
	Proc*	toutproc;
	KTube*	tin;
	KTube*	tout;
	Chan*	c;
	Tio*	next;
};

static ulong qgen;
static Tio *ftio;
static QLock ftiolck;

static void
ftdump(void)
{
	Tio *ft;

	if(DBGFLG == 0)
		return;
	qlock(&ftiolck);
	for(ft = ftio; ft != nil; ft = ft->next)
		print("ft %#p path %#ulx %s\n", ft, ft->path,
			ft->conf ? ft->conf : "none");
	qunlock(&ftiolck);
}

static void
dupsegs(Proc *p)
{
	int i;

	qlock(&p->seglock);
	for(i = 0; i < NSEG; i++)
		if(p->seg[i] != nil){
			up->seg[i] = p->seg[i];
			incref(up->seg[i]);
		}
	qunlock(&p->seglock);
}

static void
tinproc(void *arg)
{
	Tio *ft;
	KTube *kt;
	Tube *t;
	ft = arg;

	DBG("tinproc %d for proc %d\n", up->pid, ft->uproc->pid);
	dupsegs(ft->uproc);
	mfence();
	ft->tinproc = up;
	kt = ft->tin;
	t = kt->t;
	for(;;){
		if(waserror()){
			/* report up->errstr */
			break;
		}
		USED(t);
		/* n = ft->c->dev->read(ft->c, va, t->msz, ft->c->offset); */
		/* send to tin; */
	}
	pexit(nil, 1);
}

static void
toutproc(void *arg)
{
	Tio *ft;

	ft = arg;

	DBG("toutproc %d for proc %d\n", up->pid, ft->uproc->pid);
	dupsegs(ft->uproc);
	mfence();
	ft->toutproc = up;
	for(;;){
		/*
		recv from tout
		write to chan
		 */
	}
	//pexit(nil, 1);
}

static void
ftstart(Tio *ft)
{
	ft->uproc = up;
	if(ft->tin != nil){
		kproc("tin", tinproc, ft);
		while(ft->tinproc == nil)
			sched();
	}
	if(ft->tout != nil){
		kproc("tout", toutproc, ft);
		while(ft->toutproc == nil)
			sched();
	}
}

static void
ftstop(Tio *ft)
{
	if(ft->tin != nil)
		; /* post a note? */
	if(ft->tout != nil)
		; /* post a note? */
}

static int
ftgen(Chan *c, char*, Dirtab*, int, int s, Dir *dp)
{
	Qid q;
	Tio *ft;

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#=", 0, eve, DMDIR|0555, dp);
		return 1;
	}

	if(s == 0){
		mkqid(&q, Qctl, 0, QTFILE);
		devdir(c, q, "ctl", 0, eve, 0664, dp);
		return 1;
	}
	s--;

	qlock(&ftiolck);
	for(ft = ftio; ft != nil && s > 0; ft = ft->next)
		s--;

	if(ft == nil){
		qunlock(&ftiolck);
		return -1;
	}

	mkqid(&q, ft->path, 0, QTFILE);
	snprint(up->genbuf, sizeof up->genbuf, "%uld", ft->path);
	devdir(c, q, up->genbuf, 0, ft->user, ft->perm, dp);
	qunlock(&ftiolck);
	return 1;
}

static Chan*
ftattach(char *spec)
{
	return devattach('=', spec);
}

static Walkqid*
ftwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Walkqid *wq;

	wq = devwalk(c, nc, name, nname, 0, 0, ftgen);
	if(wq != nil && wq->clone != nil && (c->qid.type&QTDIR) == 0){
		if(c->aux != nil)
			incref(c->aux);
		wq->clone->aux = c->aux;
	}
	return wq;
}

static long
ftstat(Chan *c, uchar *db, long n)
{
	return devstat(c, db, n, 0, 0, ftgen);
}

static char*
ftconfstr(void)
{
	int n;
	Tio *ft;
	char *buf, *s;

	qlock(&ftiolck);
	n = 0;
	for(ft = ftio; ft != nil; ft = ft->next)
		n += strlen(ft->conf) + 1;
	n++;
	buf = smalloc(n);
	s = buf;
	for(ft = ftio; ft != nil; ft = ft->next)
		s = seprint(s, buf+n, "%s\n", ft->conf);
	qunlock(&ftiolck);
	return buf;	
}

static Chan*
ftopen(Chan *c, int omode)
{
	Tio *ft, **ftl;

	if(c->qid.type&QTDIR){
		if(omode & ORCLOSE)
			error(Eperm);
		if(omode != OREAD)
			error(Eisdir);
		c->mode = openmode(omode);
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	c->mode = openmode(omode);
	ft = c->aux;
	if(ft == nil){
		ft = smalloc(sizeof *ft);
		ft->ref = 1;
		qlock(&ftiolck);
		ft->path = Qdata + qgen++;
		mkqid(&c->qid, ft->path, 0, QTFILE);
		for(ftl = &ftio; *ftl != nil; ftl = &(*ftl)->next)
			;
		*ftl = ft;
		qunlock(&ftiolck);
		c->aux = ft;
	}else
		incref(ft);
	c->flag |= COPEN;
	c->offset = 0;

	ftdump();
	return c;
}

static void
ftclose(Chan *c)
{
	ulong q;
	Tio *ft, **ftl;

	q = (ulong)c->qid.path;
	switch(q){
	case Qdir:
	case Qctl:
		break;
	default:
		if((c->flag&COPEN) == 0)
			break;
		ft = c->aux;
		if(decref(ft) > 0)
			break;
		qlock(&ftiolck);
		for(ftl = &ftio; *ftl != nil; ftl = &(*ftl)->next)
			if(*ftl == ft)
				break;
		if(*ftl == nil)
			panic("ftclose: Tio not found");
		*ftl = ft->next;
		if(ft->c != nil)
			ftstop(ft);
		qunlock(&ftiolck);
		break;
	}

	ftdump();
}

static long
ftread(Chan *c, void *a, long n, vlong offset)
{
	ulong q;
	Tio *ft;
	char *s;

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, 0, 0, ftgen);
	q = (ulong)c->qid.path;
	switch(q){
	case Qctl:
		n = 0;
		break;
	default:
		ft = c->aux;
		s = ft->conf;
		if(s == nil)
			s = "none\n";
		n = readstr(offset, a, n, s);
		break;
	}
	return n;
}

typedef struct Testarg Testarg;
struct Testarg
{
	Rendez;
	Proc *p;
	union{
		Sem *s;
		KTube *kt;
	};
	union{
		int isup;
		int issend;
	};
	int started;
};

static int
started(void *x)
{
	Testarg *arg = x;

	return arg->started;
}

/*
 * Test semaphores shared with the user.
 * Only 10 ups or downs.
 */
static void
semproc(void *x)
{
	Testarg *arg = x;
	int i;

	dupsegs(arg->p);
	DBG("semproc started\n");
	arg->started = 1;
	wakeup(arg);
	for(i = 0; i < 10; i++){
		if(arg->isup)	/* let the user block... */
			tsleep(&up->sleep, return0, 0, 3000);
		DBG("kern: %s...\n", arg->isup?"up":"down");
		if(arg->isup)
			upsem(arg->s);
		else
			downsem(arg->s, 0);
		DBG("kern: %s done\n", arg->isup?"up":"down");
	}
	DBG("semproc exiting\n");
	free(arg);
	pexit(nil, 1);
}

static void
tubeproc(void *x)
{
	Testarg *arg = x;
	int i;
	uchar *addr;
	KTube *kt;

	kt = arg->kt;
	dupsegs(arg->p);
	DBG("tubeproc started\n");
	arg->started = 1;
	wakeup(arg);
	addr = x;
	for(i = 0; i < 10; i++){
		if(arg->issend)	/* let the user block... */
			tsleep(&up->sleep, return0, 0, 3000);
		DBG("kern: %s...\n", arg->issend?"send":"recv");
		if(arg->issend)
			tsend(kt, &addr);
		else
			trecv(kt, &addr);
		DBG("kern: %s done (-> %#p)\n",
			arg->issend?"send":"recv", addr);
		addr++;
	}
	DBG("tubeproc exiting\n");
	free(arg);
	pexit(nil, 1);
}


static void
testsem(uintptr va, int isup)
{
	Segment *sg;
	Sem *s;
	Testarg *arg;

	sg = seg(up, va, 0);
	if(sg == nil)
		error("no such semaphore");
	s = segmksem(sg, UINT2PTR(va));

	arg = smalloc(sizeof *arg);
	arg->p = up;
	arg->s = s;
	arg->isup = isup;
	kproc("sem", semproc, arg);
	sleep(arg, started, arg);
}

static void
testtube(uintptr va, int issend)
{
	Tube *t;
	KTube *kt;
	Testarg *arg;

	t = validaddr(UINT2PTR(va), sizeof *t, 1);
	evenaddr(PTR2UINT(t));
	kt = u2ktube(t);
	arg = smalloc(sizeof *arg);
	arg->p = up;
	arg->kt = kt;
	arg->issend = issend;
	kproc("tube", tubeproc, arg);
	sleep(arg, started, arg);
}

/*
 * Ctls:
 *	up sem_address		-- make the kernel do 10 ups there
 *	down sem_address	-- make the kernel do 10 downs there
 *	fd addr addr		-- configure tubes for in and out through fd
 *	send tube_address	-- make the kernel send ptrs
 *	recv tube_address	-- make the kernel recv ptrs and print them
 */
static void
testing(char *tok[])
{
	if(strcmp(tok[0], "up") == 0)
		testsem(strtoull(tok[1], 0, 0), 1);
	else if(strcmp(tok[0], "down") == 0)
		testsem(strtoull(tok[1], 0, 0), 0);
	else if(strcmp(tok[0], "send") == 0)
		testtube(strtoull(tok[1], 0, 0), 1);
	else if(strcmp(tok[0], "recv") == 0)
		testtube(strtoull(tok[1], 0, 0), 0);
	else
		error("unknown testing ctl");
}

static long
ftwrite(Chan *c, void *a, long n, vlong)
{
	char *tok[5], *s;
	int ntok, fd;
	ulong q;
	uintptr a1, a2;
	Tio *ft;

	q = c->qid.path;
	switch(q){
	case Qctl:
	case Qdir:
		break;
	default:
		s = up->genbuf;
		if(n > sizeof up->genbuf - 1)
			error("config string is too large");
		memmove(s, a, n);
		s[n] = 0;
		ntok = tokenize(s, tok, nelem(tok));
		if(ntok == 2){
			testing(tok);
			break;
		}
		if(ntok != 3)
			error("config must be 'fd addr addr'");
		qlock(&ftiolck);
		if(waserror()){
			qunlock(&ftiolck);
			nexterror();
		}
		ft = c->aux;
		if(ft->c != nil)
			error("can't configure twice");
		ft->conf = smalloc(n+2);	/* n bytes + \n + \0 */
		memmove(ft->conf, a, n);
		strcpy(ft->conf+n, "\n");
		a1 = strtoull(tok[1], 0, 0);
		if(a1 != 0)
			ft->tin = validaddr(UINT2PTR(a1), sizeof(Tube), 1);
		a2 = strtoull(tok[2], 0, 0);
		if(a2 != 0)
			ft->tout = validaddr(UINT2PTR(a2), sizeof(Tube), 1);
		if(a1 == 0 && a2 == 0)
			error("both in and out tubes are nil");
		fd = strtoul(tok[0], 0, 0);
		ft->c = fdtochan(fd, -1, 0, 1);	/* check errors and incref */

		ftstart(ft);
		qunlock(&ftiolck);
		poperror();
		break;
	}
	return n;
}

Dev tubedevtab = {
	'=',
	"tube",
	devreset,
	devinit,
	devshutdown,
	ftattach,
	ftwalk,
	ftstat,
	ftopen,
	devcreate,
	ftclose,
	ftread,
	devbread,
	ftwrite,
	devbwrite,
	devremove,
	devwstat,
};
