#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"


enum
{
	Maxenvsize = 16300,
};

static int
envgen(Chan *c, Dirtab*, int, int s, Dir *dp)
{
	Egrp *eg;
	Evalue *e;

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#e", 0, eve, 0775, dp);
		return 1;
	}

	eg = up->egrp;
	qlock(eg);

	for(e = eg->entries; e && s; e = e->link)
		s--;

	if(e == 0) {
		qunlock(eg);
		return -1;
	}

	devdir(c, e->qid, e->name, e->len, eve, 0666, dp);
	qunlock(eg);
	return 1;
}

static Evalue*
envlookup(Egrp *eg, char *name, ulong qidpath)
{
	Evalue *e;
	for(e = eg->entries; e; e = e->link)
		if(e->qid.path == qidpath || (name && strcmp(e->name, name) == 0))
			return e;
	return nil;
}

static Chan*
envattach(char *spec)
{
	return devattach('e', spec);
}

static int
envwalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, envgen);
}

static void
envstat(Chan *c, char *db)
{
	if(c->qid.path & CHDIR)
		c->qid.vers = up->egrp->vers;
	devstat(c, db, 0, 0, envgen);
}

static Chan*
envopen(Chan *c, int omode)
{
	Egrp *eg;
	Evalue *e;

	eg = up->egrp;
	if(c->qid.path & CHDIR) {
		if(omode != OREAD)
			error(Eperm);
	}
	else {
		qlock(eg);
		e = envlookup(eg, nil, c->qid.path);
		if(e == 0) {
			qunlock(eg);
			error(Enonexist);
		}
		if((omode & OTRUNC) && e->value) {
			e->qid.vers++;
			free(e->value);
			e->value = 0;
			e->len = 0;
		}
		qunlock(eg);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
envcreate(Chan *c, char *name, int omode, ulong)
{
	Egrp *eg;
	Evalue *e;

	if(c->qid.path != CHDIR)
		error(Eperm);

	omode = openmode(omode);
	eg = up->egrp;

	qlock(eg);
	if(waserror()) {
		qunlock(eg);
		nexterror();
	}

	if(envlookup(eg, name, -1))
		error(Eexist);

	e = smalloc(sizeof(Evalue));
	e->name = smalloc(strlen(name)+1);
	strcpy(e->name, name);

	e->qid.path = ++eg->path;
	e->qid.vers = 0;
	eg->vers++;
	e->link = eg->entries;
	eg->entries = e;
	c->qid = e->qid;

	qunlock(eg);
	poperror();

	c->offset = 0;
	c->mode = omode;
	c->flag |= COPEN;
}

static void
envremove(Chan *c)
{
	Egrp *eg;
	Evalue *e, **l;

	if(c->qid.path & CHDIR)
		error(Eperm);

	eg = up->egrp;
	qlock(eg);
	l = &eg->entries;
	for(e = *l; e; e = e->link) {
		if(e->qid.path == c->qid.path)
			break;
		l = &e->link;
	}

	if(e == 0) {
		qunlock(eg);
		error(Enonexist);
	}

	*l = e->link;
	eg->vers++;
	qunlock(eg);
	free(e->name);
	if(e->value)
		free(e->value);
	free(e);
}

static void
envclose(Chan *c)
{
	/*
	 * close can't fail, so errors from remove will be ignored anyway.
	 * since permissions aren't checked,
	 * envremove can't not remove it if its there.
	 */
	if(c->flag & CRCLOSE)
		envremove(c);
}

static long
envread(Chan *c, void *a, long n, vlong off)
{
	Egrp *eg;
	Evalue *e;
	ulong offset = off;

	if(c->qid.path & CHDIR)
		return devdirread(c, a, n, 0, 0, envgen);

	eg = up->egrp;
	qlock(eg);
	e = envlookup(eg, nil, c->qid.path);
	if(e == 0) {
		qunlock(eg);
		error(Enonexist);
	}

	if(offset + n > e->len)
		n = e->len - offset;
	if(n <= 0)
		n = 0;
	else
		memmove(a, e->value+offset, n);
	qunlock(eg);
	return n;
}

static long
envwrite(Chan *c, void *a, long n, vlong off)
{
	char *s;
	int vend;
	Egrp *eg;
	Evalue *e;
	ulong offset = off;

	if(n <= 0)
		return 0;

	vend = offset+n;
	if(vend > Maxenvsize)
		error(Etoobig);

	eg = up->egrp;
	qlock(eg);
	e = envlookup(eg, nil, c->qid.path);
	if(e == 0) {
		qunlock(eg);
		error(Enonexist);
	}

	if(vend > e->len) {
		s = smalloc(offset+n);
		if(e->value){
			memmove(s, e->value, e->len);
			free(e->value);
		}
		e->value = s;
		e->len = vend;
	}
	memmove(e->value+offset, a, n);
	e->qid.vers++;
	eg->vers++;
	qunlock(eg);
	return n;
}

Dev envdevtab = {
	'e',
	"env",

	devreset,
	devinit,
	envattach,
	devclone,
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
	devwstat,
};

void
envcpy(Egrp *to, Egrp *from)
{
	Evalue **l, *ne, *e;

	l = &to->entries;
	qlock(from);
	for(e = from->entries; e; e = e->link) {
		ne = smalloc(sizeof(Evalue));
		ne->name = smalloc(strlen(e->name)+1);
		strcpy(ne->name, e->name);
		if(e->value) {
			ne->value = smalloc(e->len);
			memmove(ne->value, e->value, e->len);
			ne->len = e->len;
		}
		ne->qid.path = ++to->path;
		*l = ne;
		l = &ne->link;
	}
	qunlock(from);
}

void
closeegrp(Egrp *eg)
{
	Evalue *e, *next;

	if(decref(eg) == 0) {
		for(e = eg->entries; e; e = next) {
			next = e->link;
			free(e->name);
			if(e->value)
				free(e->value);
			free(e);
		}
		free(eg);
	}
}

/*
 *  to let the kernel set environment variables
 */
void
ksetenv(char *ename, char *eval)
{
	Chan *c;
	char buf[2*NAMELEN];

	sprint(buf, "#e/%s", ename);
	c = namec(buf, Acreate, OWRITE, 0600);
	devtab[c->type]->write(c, eval, strlen(eval), 0);
	cclose(c);
}
