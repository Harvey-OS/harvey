#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

struct
{
	Lock;
	Chan	*free;
	int	fid;
}chanalloc;

int
incref(Ref *r)
{
	int x;

	lock(r);
	x = ++r->ref;
	unlock(r);
	return x;
}

int
decref(Ref *r)
{
	int x;

	lock(r);
	x = --r->ref;
	unlock(r);
	if(x < 0) 
		panic("decref");

	return x;
}

void
chandevreset(void)
{
	int i;

	for(i=0; i<devchar[i]; i++)
		(*devtab[i].reset)();
}

void
chandevinit(void)
{
	int i;

	for(i=0; i<devchar[i]; i++)
		(*devtab[i].init)();
}

Chan*
newchan(void)
{
	Chan *c;
	int nfid;

	SET(nfid);

	lock(&chanalloc);
	c = chanalloc.free;
	if(c)
		chanalloc.free = c->next;
	else
		nfid = ++chanalloc.fid;
	unlock(&chanalloc);

	if(c == 0) {
		c = smalloc(sizeof(Chan));
		c->fid = nfid;
	}

	/* if you get an error before associating with a dev,
	   close calls rootclose, a nop */
	c->type = 0;
	c->flag = 0;
	c->ref = 1;
	c->dev = 0;
	c->offset = 0;
	c->mnt = 0;
	c->stream = 0;
	c->aux = 0;
	c->mchan = 0;
	c->mqid = (Qid){0, 0};
	return c;
}

void
chanfree(Chan *c)
{
	c->flag = CFREE;
	lock(&chanalloc);
	c->next = chanalloc.free;
	chanalloc.free = c;
	unlock(&chanalloc);
}

void
close(Chan *c)
{
	if(c->flag & CFREE)
		panic("close");

	if(decref(c) == 0){
		if(!waserror()) {
			(*devtab[c->type].close)(c);
			poperror();
		}
		chanfree(c);
	}
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
mount(Chan *new, Chan *old, int flag)
{
	Pgrp *pg;
	Mount *nm, *f;
	Mhead *m, **l;
	int order;

	if(CHDIR & (old->qid.path^new->qid.path))
		error(Emount);

	order = flag&MORDER;

	if((old->qid.path&CHDIR)==0 && order != MREPL)
		error(Emount);

	pg = u->p->pgrp;
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
		m = smalloc(sizeof(Mhead));
		m->from = old;
		incref(old);
		m->hash = *l;
		*l = m;
		if(order != MREPL) 
			m->mount = newmount(m, old);
	}

	if(m->mount && order == MREPL) {
		mountfree(m->mount);
		m->mount = 0;
	}

	nm = newmount(m, new);
	if(flag & MCREATE)
		new->flag |= CCREATE;

	if(m->mount && order == MAFTER) {
		for(f = m->mount; f->next; f = f->next)
			;
		f->next = nm;
	}
	else {
		nm->next = m->mount;
		m->mount = nm;
	}

	wunlock(&pg->ns);
	poperror();
	return nm->mountid;
}

void
unmount(Chan *mnt, Chan *mounted)
{
	Pgrp *pg;
	Mhead *m, **l;
	Mount *f, **p;

	pg = u->p->pgrp;
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
		close(m->from);
		free(m);
		return;
	}

	p = &m->mount;
	for(f = *p; f; f = f->next) {
		if(eqchan(f->to, mounted, 1)) {
			*p = f->next;
			f->next = 0;
			mountfree(f);
			if(m->mount == 0) {
				*l = m->hash;
				wunlock(&pg->ns);
				close(m->from);
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
	Pgrp *pg;
	Chan *nc;
	Mhead *m;

	pg = u->p->pgrp;
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
			nc->mountid = m->mount->mountid;
			close(c);
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
	Pgrp *pg;
	Mhead **h, **he, *f;
	Mount *t;

	pg = u->p->pgrp;
	rlock(&pg->ns);
	if(waserror()) {
		runlock(&pg->ns);
		nexterror();
	}

	he = &pg->mnthash[MNTHASH];
	for(h = pg->mnthash; h < he; h++) {
		for(f = *h; f; f = f->hash) {
			for(t = f->mount; t; t = t->next) {
				if(eqchan(c, t->to, 1)) {
					close(c);
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
	Pgrp *pg;
	Chan *c = 0;
	Mount *f;
	int dotdot;

	if(*name == '\0')
		return ac;

	dotdot = 0;
	if(name[0] == '.')
	if(name[1] == '.')
	if(name[2] == '\0') {
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

	pg = u->p->pgrp;
	rlock(&pg->ns);
	if(waserror()) {
		runlock(&pg->ns);
		if(c)
			close(c);
		nexterror();
	}
	for(f = ac->mnt; f; f = f->next) {
		c = clone(f->to, 0);
		if((*devtab[c->type].walk)(c, name) != 0)
			break;
		close(c);
		c = 0;
	}
	poperror();
	runlock(&pg->ns);

	if(c) {
		if(dotdot)
			c = undomount(c);
		c->mnt = 0;
		if(domnt) {
			if(waserror()) {
				close(c);
				nexterror();
			}
			c = domount(c);
			poperror();
		}
		close(ac);
	}
	return c;	
}

/*
 * c is a mounted non-creatable directory.  find a creatable one.
 */
Chan*
createdir(Chan *c)
{
	Pgrp *pg;
	Chan *nc;
	Mount *f;

	pg = u->p->pgrp;
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
			close(c);
			return nc;
		}
	}
	error(Enocreate);
	return 0;		/* not reached */
}

void
saveregisters(void)
{
}

/*
 * Turn a name into a channel.
 * &name[0] is known to be a valid address.  It may be a kernel address.
 */
Chan*
namec(char *name, int amode, int omode, ulong perm)
{
	Chan *c, *nc, *cc;
	int t;
	Rune r;
	int mntok, isdot;
	char *p;
	char *elem;
	char createerr[ERRLEN];

	if(name[0] == 0)
		error(Enonexist);

	/*
	 * Make sure all of name is o.k.  first byte is validated
	 * externally so if it's a kernel address we know it's o.k.
	 */
	if(!((ulong)name & KZERO)){
		p = name;
		t = BY2PG-((ulong)p&(BY2PG-1));
		while(vmemchr(p, 0, t) == 0){
			p += t;
			t = BY2PG;
		}
	}

	elem = u->elem;
	mntok = 1;
	isdot = 0;
	if(name[0] == '/') {
		c = clone(u->slash, 0);
		/*
		 * Skip leading slashes.
		 */
		name = skipslash(name);
	}
	else
	if(name[0] == '#') {
		mntok = 0;
		name++;
		name += chartorune(&r, name);
		if(r == 'M')
			error(Enonexist);
		t = devno(r, 1);
		if(t == -1)
			error(Ebadsharp);
		if(*name == '/'){
			name = skipslash(name);
			elem[0]=0;
		}else
			name = nextelem(name, elem);
		c = (*devtab[t].attach)(elem);
	}
	else {
		c = clone(u->dot, 0);
		name = skipslash(name);	/* eat leading ./ */
		if(*name == 0)
			isdot = 1;
	}

	if(waserror()){
		close(c);
		nexterror();
	}

	name = nextelem(name, elem);

	/*
	 *  If mounting, don't follow the mount entry for root or the
	 *  current directory.
	 */
	if(mntok && !isdot && !(amode==Amount && elem[0]==0))
		c = domount(c);			/* see case Atodir below */

	/*
	 * How to treat the last element of the name depends on the operation.
	 * Therefore do all but the last element by the easy algorithm.
	 */
	while(*name){
		if((nc=walk(c, elem, mntok)) == 0)
			error(Enonexist);
		c = nc;
		name = nextelem(name, elem);
	}

	/*
	 * Last element; act according to type of access.
	 */
	switch(amode){
	case Aaccess:
		if(isdot)
			c = domount(c);
		else{
			if((nc=walk(c, elem, mntok)) == 0)
				error(Enonexist);
			c = nc;
		}
		break;

	case Atodir:
		/*
		 * Directories (e.g. for cd) are left before the mount point,
		 * so one may mount on / or . and see the effect.
		 */
		if((nc=walk(c, elem, 0)) == 0)
			error(Enonexist);
		c = nc;
		if(!(c->qid.path & CHDIR))
			error(Enotdir);
		break;

	case Aopen:
		if(isdot)
			c = domount(c);
		else{
			if((nc=walk(c, elem, mntok)) == 0)
				error(Enonexist);
			c = nc;
		}
	Open:
		/* else error() in open has wrong value of c saved */
		saveregisters();	
		c = (*devtab[c->type].open)(c, omode);
		if(omode & OCEXEC)
			c->flag |= CCEXEC;
		if(omode & ORCLOSE)
			c->flag |= CRCLOSE;
		break;

	case Amount:
		/*
		 * When mounting on an already mounted upon directory, one wants
		 * subsequent mounts to be attached to the original directory, not
		 * the replacement.
		 */
		if((nc=walk(c, elem, 0)) == 0)
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
			close(cc);
			nexterror();
		}
		nameok(elem);
		if((nc=walk(cc, elem, 1)) != 0){
			poperror();
			close(c);
			c = nc;
			omode |= OTRUNC;
			goto Open;
		}
		close(cc);
		poperror();

		/*
		 *  the file didn't exist, try the create
		 */
		if(c->mnt && !(c->flag&CCREATE))
			c = createdir(c);

		/*
		 *  protect against the open/create race. This is not a complete
		 *  fix. It just reduces the window.
		 */
		if(waserror()) {
			strcpy(createerr, u->error);
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

char isfrog[256]={
	/*NUL*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*BKS*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*DLE*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*CAN*/	1, 1, 1, 1, 1, 1, 1, 1,
	[' ']	1,
	['/']	1,
	[0x7f]	1,
};

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
	while(name < end){
		name += chartorune(&r, name);
		if(r<sizeof(isfrog) && isfrog[r])
			error(Ebadchar);
	}
	return skipslash(name);
}

void
isdir(Chan *c)
{
	if(c->qid.path & CHDIR)
		return;
	error(Enotdir);
}
