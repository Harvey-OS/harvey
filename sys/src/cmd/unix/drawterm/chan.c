#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

struct
{
	Lock	l;
	int	fid;
	Chan	*free;
	Chan	*list;
}chanalloc;

static char isfrog[256]=
{
	/*NUL*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*BKS*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*DLE*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*CAN*/	1, 1, 1, 1, 1, 1, 1, 1
};

void
chandevinit(void)
{
	int i;


	isfrog[' '] = 1;
	isfrog['/'] = 1;
	isfrog[0x7f] = 1;

	for(i=0; devchar[i]; i++)
		(*devtab[i].init)();
}

Chan*
newchan(void)
{
	Chan *c;

	lock(&chanalloc.l);
	c = chanalloc.free;
	if(c != 0)
		chanalloc.free = c->next;
	unlock(&chanalloc.l);

	if(c == 0) {
		c = mallocz(sizeof(Chan));
		lock(&chanalloc.l);
		c->fid = ++chanalloc.fid;
		c->link = chanalloc.list;
		chanalloc.list = c;
		unlock(&chanalloc.l);
	}

	/* if you get an error before associating with a dev,
	   close calls rootclose, a nop */
	c->type = 0;
	c->flag = 0;
	c->r.ref = 1;
	c->dev = 0;
	c->offset = 0;
	c->mnt = 0;
	memset(&c->u, 0, sizeof(c->u));
	c->mchan = 0;
	c->path = 0;
	c->mqid.path = 0;
	c->mqid.vers = 0;
	return c;
}

void
chanfree(Chan *c)
{
	c->flag = CFREE;

	/*
	 * Channel can be closed before a path is created or the last
	 * channel in a mount which has already cleared its pt names
	 */
	if(c->path)
		refdec(&c->path->r);

	lock(&chanalloc.l);
	c->next = chanalloc.free;
	chanalloc.free = c;
	unlock(&chanalloc.l);
}

void
cclose(Chan *c)
{
	if(c == 0)
		return;

	if(c->flag&CFREE)
		panic("close");

	if(refdec(&c->r))
		return;

	if(!waserror()) {
		(*devtab[c->type].close)(c);
		poperror();
	}

	chanfree(c);
}

void
isdir(Chan *c)
{
	if(c->qid.path & CHDIR)
		return;
	error(Enotdir);
}

int
eqqid(Qid a, Qid b)
{
	return a.path==b.path && a.vers==b.vers;
}

int
eqchan(Chan *a, Chan *b, int pathonly)
{
	if(a->qid.path != b->qid.path)
		return 0;
	if(!pathonly && a->qid.vers!=b->qid.vers)
		return 0;
	if(a->type != b->type)
		return 0;
	if(a->dev != b->dev)
		return 0;
	return 1;
}

int
cmount(Chan *new, Chan *old, int flag, char *spec)
{
	Pgrp *pg;
	int order, flg;
	Mhead *m, **l;
	Mount *nm, *f, *um, **h;

	if(CHDIR & (old->qid.path^new->qid.path))
		error(Emount);

	order = flag&MORDER;

	if((old->qid.path&CHDIR)==0 && order != MREPL)
		error(Emount);

	pg = up->pgrp;
	wlock(&pg->ns);
	if(waserror()) {
		wunlock(&pg->ns);
		nexterror();
	}

	l = &MOUNTH(pg, old);
	for(m = *l; m; m = m->hash) {
		if(eqchan(m->from, old, 1))
			break;
		l = &m->hash;
	}

	if(m == 0) {
		/*
		 *  nothing mounted here yet.  create a mount
		 *  head and add to the hash table.
		 */
		m = mallocz(sizeof(Mhead));
		m->from = old;
		refinc(&old->r);
		m->hash = *l;
		*l = m;

		/*
		 *  if this is a union mount, add the old
		 *  node to the mount chain.
		 */
		if(order != MREPL) 
			m->mount = newmount(m, old, 0, 0);
	}

	nm = newmount(m, new, flag, spec);
	if(new->mnt != 0) {
		/*
		 *  copy a union when binding it onto a directory
		 */
		flg = order;
		if(order == MREPL)
			flg = MAFTER;
		h = &nm->next;
		for(um = new->mnt->next; um; um = um->next) {
			f = newmount(m, um->to, flg, um->spec);
			*h = f;
			h = &f->next;
		}
	}

	if(m->mount && order == MREPL) {
		mountfree(m->mount);
		m->mount = 0;
	}

	if(flag & MCREATE)
		new->flag |= CCREATE;

	if(m->mount && order == MAFTER) {
		for(f = m->mount; f->next; f = f->next)
			;
		f->next = nm;
	}
	else {
		for(f = nm; f->next; f = f->next)
			;
		f->next = m->mount;
		m->mount = nm;
	}

	wunlock(&pg->ns);
	poperror();
	return nm->mountid;
}

void
cunmount(Chan *mnt, Chan *mounted)
{
	Pgrp *pg;
	Mhead *m, **l;
	Mount *f, **p;

	pg = up->pgrp;
	wlock(&pg->ns);

	l = &MOUNTH(pg, mnt);
	for(m = *l; m; m = m->hash) {
		if(eqchan(m->from, mnt, 1))
			break;
		l = &m->hash;
	}

	if(m == 0) {
		wunlock(&pg->ns);
		error(Eunmount);
	}

	if(mounted == 0) {
		*l = m->hash;
		wunlock(&pg->ns);
		mountfree(m->mount);
		cclose(m->from);
		free(m);
		return;
	}

	p = &m->mount;
	for(f = *p; f; f = f->next) {
		/* BUG: Needs to be 2 pass */
		if(eqchan(f->to, mounted, 1) ||
		  (f->to->mchan && eqchan(f->to->mchan, mounted, 1))) {
			*p = f->next;
			f->next = 0;
			mountfree(f);
			if(m->mount == 0) {
				*l = m->hash;
				wunlock(&pg->ns);
				cclose(m->from);
				free(m);
				return;
			}
			wunlock(&pg->ns);
			return;
		}
		p = &f->next;
	}
	wunlock(&pg->ns);
	error(Eunion);
}

Chan*
clone(Chan *c, Chan *nc)
{
	return (*devtab[c->type].clone)(c, nc);
}

Chan*
domount(Chan *c)
{
	Chan *nc;
	Mhead *m;
	Pgrp *pg;

	pg = up->pgrp;
	rlock(&pg->ns);
	if(waserror()) {
		runlock(&pg->ns);
		nexterror();
	}
	c->mnt = 0;

	for(m = MOUNTH(pg, c); m; m = m->hash)
		if(eqchan(m->from, c, 1)) {
			nc = clone(m->mount->to, 0);
			nc->mnt = m->mount;
			nc->xmnt = nc->mnt;
			nc->mountid = m->mount->mountid;
			cclose(c);
			c = nc;	
			break;			
		}

	poperror();
	runlock(&pg->ns);
	return c;
}

Chan*
undomount(Chan *c)
{
	Mount *t;
	Pgrp *pg;
	Mhead **h, **he, *f;

	pg = up->pgrp;
	rlock(&pg->ns);
	if(waserror()) {
		runlock(&pg->ns);
		nexterror();
	}

	he = &((Pgrp*)pg)->mnthash[MNTHASH];
	for(h = pg->mnthash; h < he; h++) {
		for(f = *h; f; f = f->hash) {
			for(t = f->mount; t; t = t->next) {
				if(eqchan(c, t->to, 1)) {
					cclose(c);
					c = clone(t->head->from, 0);
					break;
				}
			}
		}
	}
	poperror();
	runlock(&pg->ns);
	return c;
}

Chan*
walk(Chan *ac, char *name, int domnt)
{
	Mount *f;
	int dotdot;
	Pgrp *pg;
	struct { Chan *c; } c;

	if(name[0] == '\0')
		return ac;

	dotdot = 0;
	if(name[0] == '.' && name[1] == '.' && name[2] == '\0') {
		ac = undomount(ac);
		dotdot = 1;
	}

	if((*devtab[ac->type].walk)(ac, name) != 0) {
		if(dotdot)
			ac = undomount(ac);
		if(domnt)
			ac = domount(ac);
		return ac;
	}

	if(ac->mnt == 0) 
		return 0;

	c.c = 0;
	pg = up->pgrp;

	rlock(&pg->ns);
	if(waserror()) {
		runlock(&pg->ns);
		cclose(c.c);
		nexterror();
	}
	for(f = ac->mnt; f; f = f->next) {
		c.c = clone(f->to, 0);
		if((*devtab[c.c->type].walk)(c.c, name) != 0)
			break;
		cclose(c.c);
		c.c = 0;
	}
	poperror();
	runlock(&pg->ns);

	if(c.c == 0)
		return 0;

	if(dotdot)
		c.c = undomount(c.c);

	c.c->mnt = 0;
	if(domnt) {
		if(waserror()) {
			cclose(c.c);
			nexterror();
		}
		c.c = domount(c.c);
		poperror();
	}
	cclose(ac);
	return c.c;	
}

/*
 * c is a mounted non-creatable directory.  find a creatable one.
 */
Chan*
createdir(Chan *c)
{
	Mount *f;
	Chan *nc;
	Pgrp *pg;

	pg = up->pgrp;
	rlock(&pg->ns);
	if(waserror()) {
		runlock(&pg->ns);
		nexterror();
	}
	for(f = c->mnt; f; f = f->next) {
		if(f->to->flag&CCREATE) {
			nc = clone(f->to, 0);
			nc->mnt = f;
			runlock(&pg->ns);
			poperror();
			cclose(c);
			return nc;
		}
	}
	error(Enocreate);
	return 0;		/* not reached */
}

/*
 * Turn a name into a channel.
 */
Chan*
namec(char *name, int amode, int omode, ulong perm)
{
	Rune r;
	Chan *nc;
	int t, n, mntok, isdot;
	Chan *c;
	Chan *cc;
	char *elem, createerr[ERRLEN];

	if(name[0] == 0)
		error(Enonexist);

	elem = up->elem;
	mntok = 1;
	isdot = 0;
	switch(name[0]) {
	case '/':
		c = clone(up->slash, 0);
		name = skipslash(name);
		break;
	case '#':
		mntok = 0;
		elem[0] = 0;
		n = 0;
		while(*name && (*name != '/' || n < 2))
			elem[n++] = *name++;
		elem[n] = '\0';
		n = chartorune(&r, elem+1)+1;
		if(r == 'M')
			error(Ebadsharp);
		t = devno(r, 1);
		if(t == -1)
			error(Ebadsharp);

		c = (*devtab[t].attach)(elem+n);
		name = skipslash(name);
		break;
	default:
		c = clone(up->dot, 0);
		name = skipslash(name);
		if(*name == 0)
			isdot = 1;
	}

	if(waserror()){
		cclose(c);
		nexterror();
	}

	name = nextelem(name, elem);

	/*
	 *  If mounting, don't follow the mount entry for root or the
	 *  current directory.
	 */
	if(mntok && !isdot && !(amode==Amount && elem[0]==0))
		c = domount(c);		/* see case Atodir below */

	while(*name) {
		nc = walk(c, elem, mntok);
		if(nc == 0)
			error(Enonexist);
		c = nc;
		name = nextelem(name, elem);
	}

	switch(amode) {
	case Aaccess:
		if(isdot) {
			c = domount(c);
			break;
		}
		nc = walk(c, elem, mntok);
		if(nc == 0)
			error(Enonexist);
		c = nc;
		break;

	case Atodir:
		/*
		 * Directories (e.g. for cd) are left before the mount point,
		 * so one may mount on / or . and see the effect.
		 */
		nc = walk(c, elem, 0);
		if(nc == 0)
			error(Enonexist);
		c = nc;
		isdir(c);
		break;

	case Aopen:
		if(isdot)
			c = domount(c);
		else {
			nc = walk(c, elem, mntok);
			if(nc == 0)
				error(Enonexist);
			c = nc;
		}
	Open:
		c = (*devtab[c->type].open)(c, omode);

		if(omode & OCEXEC)
			c->flag |= CCEXEC;
		if(omode & ORCLOSE)
			c->flag |= CRCLOSE;
		break;

	case Amount:
		/*
		 * When mounting on an already mounted upon directory,
		 * one wants subsequent mounts to be attached to the 
		 * original directory, not the replacement.
		 */
		nc = walk(c, elem, 0);
		if(nc == 0)
			error(Enonexist);
		c = nc;
		break;

	case Acreate:
		if(isdot)
			error(Eisdir);

		/*
		 *  Walk the element before trying to create it
		 *  to see if it exists.  We clone the channel
		 *  first, just in case someone is trying to
		 *  use clwalk outside the kernel.
		 */
		cc = clone(c, 0);
		if(waserror()){
			cclose(cc);
			nexterror();
		}
		nameok(elem);
		nc = walk(cc, elem, 1);
		if(nc != 0) {
			poperror();
			cclose(c);
			c = nc;
			omode |= OTRUNC;
			goto Open;
		}
		cclose(cc);
		poperror();

		/*
		 *  the file didn't exist, try the create
		 */
		if(c->mnt && !(c->flag&CCREATE))
			c = createdir(c);

		/*
		 * protect against the open/create race.
		 * This is not a complete fix. It just reduces the window.
		 */
		if(waserror()) {
			strcpy(createerr, threaderr());
			nc = walk(c, elem, 1);
			if(nc == 0)
				error(createerr);
			c = nc;
			omode |= OTRUNC;
			goto Open;
		}
		(*devtab[c->type].create)(c, elem, omode, perm);
		if(omode & OCEXEC)
			c->flag |= CCEXEC;
		poperror();
		break;

	default:
		panic("unknown namec access %d\n", amode);
	}
	poperror();
	return c;
}

/*
 * name[0] is addressable.
 */
char*
skipslash(char *name)
{
    Again:
	while(*name == '/')
		name++;
	if(*name=='.' && (name[1]==0 || name[1]=='/')){
		name++;
		goto Again;
	}
	return name;
}

void
nameok(char *elem)
{
	char *eelem;

	eelem = elem+NAMELEN;
	while(*elem) {
		if(isfrog[*(uchar*)elem])
			error(Ebadchar);
		elem++;
		if(elem >= eelem)
			error(Efilename);
	}
}

/*
 * name[0] should not be a slash.
 */
char*
nextelem(char *name, char *elem)
{
	int w;
	char *end;
	Rune r;

	if(*name == '/')
		error(Efilename);
	end = utfrune(name, '/');
	if(end == 0)
		end = strchr(name, 0);
	w = end-name;
	if(w >= NAMELEN)
		error(Efilename);
	memmove(elem, name, w);
	elem[w] = 0;
	while(name < end) {
		name += chartorune(&r, name);
		if(r<sizeof(isfrog) && isfrog[r])
			error(Ebadchar);
	}
	return skipslash(name);
}
