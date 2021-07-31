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

static Egrp	*envgrp(Chan *c);
static int	envwriteable(Chan *c);

static Egrp	confegrp;	/* global environment group containing the kernel configuration */

static Evalue*
envlookup(Egrp *eg, char *name, ulong qidpath)
{
	Evalue *e;
	int i;

	for(i=0; i<eg->nent; i++){
		e = eg->ent[i];
		if(e->qid.path == qidpath || (name && e->name[0]==name[0] && strcmp(e->name, name) == 0))
			return e;
	}
	return nil;
}

static int
envgen(Chan *c, char *name, Dirtab*, int, int s, Dir *dp)
{
	Egrp *eg;
	Evalue *e;

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#e", 0, eve, DMDIR|0775, dp);
		return 1;
	}

	eg = envgrp(c);
	rlock(eg);
	e = 0;
	if(name)
		e = envlookup(eg, name, -1);
	else if(s < eg->nent)
		e = eg->ent[s];

	if(e == 0) {
		runlock(eg);
		return -1;
	}

	/* make sure name string continues to exist after we release lock */
	kstrcpy(up->genbuf, e->name, sizeof up->genbuf);
	devdir(c, e->qid, up->genbuf, e->len, eve, 0666, dp);
	runlock(eg);
	return 1;
}

static Chan*
envattach(char *spec)
{
	Chan *c;
	Egrp *egrp = nil;

	if(spec && *spec) {
		if(strcmp(spec, "c") == 0)
			egrp = &confegrp;
		if(egrp == nil)
			error(Ebadarg);
	}

	c = devattach('e', spec);
	c->aux = egrp;
	return c;
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
		c->qid.vers = envgrp(c)->vers;
	return devstat(c, db, n, 0, 0, envgen);
}

static Chan*
envopen(Chan *c, int omode)
{
	Egrp *eg;
	Evalue *e;
	int trunc;

	eg = envgrp(c);
	if(c->qid.type & QTDIR) {
		if(omode != OREAD)
			error(Eperm);
	}
	else {
		trunc = omode & OTRUNC;
		if(omode != OREAD && !envwriteable(c))
			error(Eperm);
		if(trunc)
			wlock(eg);
		else
			rlock(eg);
		e = envlookup(eg, nil, c->qid.path);
		if(e == 0) {
			if(trunc)
				wunlock(eg);
			else
				runlock(eg);
			error(Enonexist);
		}
		if(trunc && e->value) {
			e->qid.vers++;
			free(e->value);
			e->value = 0;
			e->len = 0;
		}
		if(trunc)
			wunlock(eg);
		else
			runlock(eg);
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
	Evalue **ent;

	if(c->qid.type != QTDIR)
		error(Eperm);

	omode = openmode(omode);
	eg = envgrp(c);

	wlock(eg);
	if(waserror()) {
		wunlock(eg);
		nexterror();
	}

	if(envlookup(eg, name, -1))
		error(Eexist);

	e = smalloc(sizeof(Evalue));
	e->name = smalloc(strlen(name)+1);
	strcpy(e->name, name);

	if(eg->nent == eg->ment){
		eg->ment += 32;
		ent = smalloc(sizeof(eg->ent[0])*eg->ment);
		if(eg->nent)
			memmove(ent, eg->ent, sizeof(eg->ent[0])*eg->nent);
		free(eg->ent);
		eg->ent = ent;
	}
	e->qid.path = ++eg->path;
	e->qid.vers = 0;
	eg->vers++;
	eg->ent[eg->nent++] = e;
	c->qid = e->qid;

	wunlock(eg);
	poperror();

	c->offset = 0;
	c->mode = omode;
	c->flag |= COPEN;
}

static void
envremove(Chan *c)
{
	int i;
	Egrp *eg;
	Evalue *e;

	if(c->qid.type & QTDIR)
		error(Eperm);

	eg = envgrp(c);
	wlock(eg);
	e = 0;
	for(i=0; i<eg->nent; i++){
		if(eg->ent[i]->qid.path == c->qid.path){
			e = eg->ent[i];
			eg->nent--;
			eg->ent[i] = eg->ent[eg->nent];
			eg->vers++;
			break;
		}
	}
	wunlock(eg);
	if(e == 0)
		error(Enonexist);
	free(e->name);
	if(e->value)
		free(e->value);
	free(e);
}

static void
envclose(Chan *c)
{
	/*
	 * cclose can't fail, so errors from remove will be ignored.
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

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, envgen);

	eg = envgrp(c);
	rlock(eg);
	e = envlookup(eg, nil, c->qid.path);
	if(e == 0) {
		runlock(eg);
		error(Enonexist);
	}

	if(offset > e->len)	/* protects against overflow converting vlong to ulong */
		n = 0;
	else if(offset + n > e->len)
		n = e->len - offset;
	if(n <= 0)
		n = 0;
	else
		memmove(a, e->value+offset, n);
	runlock(eg);
	return n;
}

static long
envwrite(Chan *c, void *a, long n, vlong off)
{
	char *s;
	ulong len;
	Egrp *eg;
	Evalue *e;
	ulong offset = off;

	if(n <= 0)
		return 0;
	if(offset > Maxenvsize || n > (Maxenvsize - offset))
		error(Etoobig);

	eg = envgrp(c);
	wlock(eg);
	e = envlookup(eg, nil, c->qid.path);
	if(e == 0) {
		wunlock(eg);
		error(Enonexist);
	}

	len = offset+n;
	if(len > e->len) {
		s = smalloc(len);
		if(e->value){
			memmove(s, e->value, e->len);
			free(e->value);
		}
		e->value = s;
		e->len = len;
	}
	memmove(e->value+offset, a, n);
	e->qid.vers++;
	eg->vers++;
	wunlock(eg);
	return n;
}

Dev envdevtab = {
	'e',
	"env",

	devreset,
	devinit,
	devshutdown,
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
	devwstat,
};

void
envcpy(Egrp *to, Egrp *from)
{
	int i;
	Evalue *ne, *e;

	rlock(from);
	to->ment = (from->nent+31)&~31;
	to->ent = smalloc(to->ment*sizeof(to->ent[0]));
	for(i=0; i<from->nent; i++){
		e = from->ent[i];
		ne = smalloc(sizeof(Evalue));
		ne->name = smalloc(strlen(e->name)+1);
		strcpy(ne->name, e->name);
		if(e->value){
			ne->value = smalloc(e->len);
			memmove(ne->value, e->value, e->len);
			ne->len = e->len;
		}
		ne->qid.path = ++to->path;
		to->ent[i] = ne;
	}
	to->nent = from->nent;
	runlock(from);
}

void
closeegrp(Egrp *eg)
{
	int i;
	Evalue *e;

	if(decref(eg) == 0){
		for(i=0; i<eg->nent; i++){
			e = eg->ent[i];
			free(e->name);
			if(e->value)
				free(e->value);
			free(e);
		}
		free(eg->ent);
		free(eg);
	}
}

static Egrp*
envgrp(Chan *c)
{
	if(c->aux == nil)
		return up->egrp;
	return c->aux;
}

static int
envwriteable(Chan *c)
{
	return iseve() || c->aux == nil;
}

/*
 *  to let the kernel set environment variables
 */
void
ksetenv(char *ename, char *eval, int conf)
{
	Chan *c;
	char buf[2*KNAMELEN];
	
	snprint(buf, sizeof(buf), "#e%s/%s", conf?"c":"", ename);
	c = namec(buf, Acreate, OWRITE, 0600);
	devtab[c->type]->write(c, eval, strlen(eval), 0);
	cclose(c);
}

/*
 * Return a copy of configuration environment as a sequence of strings.
 * The strings alternate between name and value.  A zero length name string
 * indicates the end of the list
 */
char *
getconfenv(void)
{
	Egrp *eg = &confegrp;
	Evalue *e;
	char *p, *q;
	int i, n;

	rlock(eg);
	if(waserror()) {
		runlock(eg);
		nexterror();
	}
	
	/* determine size */
	n = 0;
	for(i=0; i<eg->nent; i++){
		e = eg->ent[i];
		n += strlen(e->name) + e->len + 2;
	}
	p = malloc(n + 1);
	if(p == nil)
		error(Enomem);
	q = p;
	for(i=0; i<eg->nent; i++){
		e = eg->ent[i];
		strcpy(q, e->name);
		q += strlen(q) + 1;
		memmove(q, e->value, e->len);
		q[e->len] = 0;
		/* move up to the first null */
		q += strlen(q) + 1;
	}
	*q = 0;
	
	poperror();
	runlock(eg);
	return p;
}
