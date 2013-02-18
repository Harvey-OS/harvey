/*
 *	devenv - environment
 */
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

enum
{
	Maxenvlen=	16*1024-1,
};


static void envremove(Chan*);

static int
envgen(Chan *c, char *name, Dirtab *d, int nd, int s, Dir *dp)
{
	Egrp *eg;
	Evalue *e;

	USED(name);
	USED(d);
	USED(nd);
	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#e", 0, eve, DMDIR|0775, dp);
		return 1;
	}
	eg = up->env->egrp;
	qlock(&eg->l);
	for(e = eg->entries; e != nil && s != 0; e = e->next)
		s--;
	if(e == nil) {
		qunlock(&eg->l);
		return -1;
	}
	/* make sure name string continues to exist after we release lock */
	kstrcpy(up->genbuf, e->var, sizeof up->genbuf);
	devdir(c, e->qid, up->genbuf, e->len, eve, 0666, dp);
	qunlock(&eg->l);
	return 1;
}

static Chan*
envattach(char *spec)
{
	return devattach('e', spec);
}

static Walkqid*
envwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, envgen);
}

static int
envstat(Chan *c, uchar *db, int n)
{
	if(c->qid.type & QTDIR)
		c->qid.vers = up->env->egrp->vers;
	return devstat(c, db, n, 0, 0, envgen);
}

static Chan *
envopen(Chan *c, int mode)
{
	Egrp *eg;
	Evalue *e;
	
	if(c->qid.type & QTDIR) {
		if(mode != OREAD)
			p9error(Eperm);
		c->mode = mode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}
	eg = up->env->egrp;
	qlock(&eg->l);
	for(e = eg->entries; e != nil; e = e->next)
		if(e->qid.path == c->qid.path)
			break;
	if(e == nil) {
		qunlock(&eg->l);
		p9error(Enonexist);
	}
	if(mode == (OWRITE|OTRUNC) && e->val) {
		free(e->val);
		e->val = 0;
		e->len = 0;
		e->qid.vers++;
	}
	qunlock(&eg->l);
	c->offset = 0;
	c->flag |= COPEN;
	c->mode = mode&~OTRUNC;
	return c;
}

static void
envcreate(Chan *c, char *name, int mode, ulong perm)
{
	Egrp *eg;
	Evalue *e, *le;

	USED(perm);
	if(c->qid.type != QTDIR)
		p9error(Eperm);
	if(strlen(name) >= sizeof(up->genbuf))
		p9error("name too long");	/* needs to fit for stat */
	eg = up->env->egrp;
	qlock(&eg->l);
	for(le = nil, e = eg->entries; e != nil; le = e, e = e->next)
		if(strcmp(e->var, name) == 0) {
			qunlock(&eg->l);
			p9error(Eexist);
		}
	e = smalloc(sizeof(Evalue));
	e->var = smalloc(strlen(name)+1);
	strcpy(e->var, name);
	e->val = 0;
	e->len = 0;
	e->qid.path = ++eg->path;
	if (le == nil)
		eg->entries = e;
	else
		le->next = e;
	e->qid.vers = 0;
	c->qid = e->qid;
	eg->vers++;
	qunlock(&eg->l);
	c->offset = 0;
	c->flag |= COPEN;
	c->mode = mode;
}

static void
envclose(Chan * c)
{
	if(c->flag & CRCLOSE)
		envremove(c);
}

static long
envread(Chan *c, void *a, long n, vlong offset)
{
	Egrp *eg;
	Evalue *e;

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, envgen);
	eg = up->env->egrp;
	qlock(&eg->l);
	for(e = eg->entries; e != nil; e = e->next)
		if(e->qid.path == c->qid.path)
			break;
	if(e == nil) {
		qunlock(&eg->l);
		p9error(Enonexist);
	}
	if(offset > e->len)	/* protects against overflow converting vlong to ulong */
		n = 0;
	else if(offset + n > e->len)
		n = e->len - offset;
	if(n <= 0)
		n = 0;
	else
		memmove(a, e->val+offset, n);
	qunlock(&eg->l);
	return n;
}

static long
envwrite(Chan *c, void *a, long n, vlong offset)
{
	char *s;
	ulong ve;
	Egrp *eg;
	Evalue *e;

	if(n <= 0)
		return 0;
	ve = offset+n;
	if(ve > Maxenvlen)
		p9error(Etoobig);
	eg = up->env->egrp;
	qlock(&eg->l);
	for(e = eg->entries; e != nil; e = e->next)
		if(e->qid.path == c->qid.path)
			break;
	if(e == nil) {
		qunlock(&eg->l);
		p9error(Enonexist);
	}
	if(ve > e->len) {
		s = smalloc(ve);
		memmove(s, e->val, e->len);
		if(e->val)
			free(e->val);
		e->val = s;
		e->len = ve;
	}
	memmove(e->val+offset, a, n);
	e->qid.vers++;
	qunlock(&eg->l);
	return n;
}

static void
envremove(Chan *c)
{
	Egrp *eg;
	Evalue *e, **l;

	if(c->qid.type & QTDIR)
		p9error(Eperm);
	eg = up->env->egrp;
	qlock(&eg->l);
	for(l = &eg->entries; *l != nil; l = &(*l)->next)
		if((*l)->qid.path == c->qid.path)
			break;
	e = *l;
	if(e == nil) {
		qunlock(&eg->l);
		p9error(Enonexist);
	}
	*l = e->next;
	eg->vers++;
	qunlock(&eg->l);
	free(e->var);
	if(e->val != nil)
		free(e->val);
	free(e);
}

Dev envdevtab = {
	'e',
	"env",

	devinit,
	envattach,
	envwalk,
	envstat,
	envopen,
	envcreate,
	envclose,
	envread,
	devbread,
	envwrite,
	devbwrite,
	envremove,
	devwstat
};
