/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"kexec.h"

enum
{
	Maxkexecsize = 16300,
};

int kxdbg = 0;
#define KXDBG if(!kxdbg) {} else print



static Kexecgrp	*kexecgrp(Chan *c);
static int	kexecwriteable(Chan *c);


static Kexecgrp	kgrp;	/* global kexec group containing the kernel configuration */

static Kvalue*
kexeclookup(Kexecgrp *kg, uintptr addr, uint32_t qidpath)
{
	Kvalue *e;
	int i;

	for(i=0; i<kg->nent; i++){
		e = kg->ent[i];
		if(e->qid.path == qidpath || e->addr==addr)
			return e;
	}
	return nil;
}

static int
kexecgen(Chan *c, char *name, Dirtab*, int, int s, Dir *dp)
{
	Kexecgrp *kg;
	Kvalue *e;
	uintptr addr;

	print("starting gen name %s\n", name);

	if(s == DEVDOTDOT){
		devdir(c, c->qid, "#§", 0, eve, DMDIR|0775, dp);
		return 1;
	}
	print("getting kg name %s\n", name);
	
	kg = kexecgrp(c);
	rlock(kg);
	e = 0;
	if(name) {
		addr = strtoull(name, nil, 0);
		print("got addr %p\n", addr);

		e = kexeclookup(kg, addr, -1);
	}else if(s < kg->nent)
		e = kg->ent[s];

	if(e == 0) {
		runlock(kg);
		return -1;
	}

	/* make sure name string continues to exist after we release lock */
	// how will we free this?
	snprint(up->genbuf, sizeof up->genbuf, "0x%p", addr);
	print("up->genbuf %s e 0x%p\n", up->genbuf, e);
	print("e qid %d e->addr 0x%p size %ld len %ld\n", e->qid, e->addr, e->size, e->len);

	devdir(c, e->qid, up->genbuf, e->len, eve, 0666, dp);
	runlock(kg);
	print("finished gen\n");
	
	return 1;
}

#define QPATH(p,d,t)    ((p)<<16 | (d)<<8 | (t)<<0)

static Chan*
kexecattach(char *spec)
{
	Chan *c;
//	Kexecgrp *kgrp = nil;
        Qid qid;
	

	c = devattach(L'§', spec);
	c->aux = &kgrp;
	return c;
}

static Walkqid*
kexecwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, 0, 0, kexecgen);
}


static int32_t
kexecstat(Chan *c, uint8_t *db, int32_t n)
{
	int32_t nn;

	if(c->qid.type & QTDIR)
		c->qid.vers = kexecgrp(c)->vers;
	nn = devstat(c, db, n, 0, 0, kexecgen);

	return nn;
}

static Chan*
kexecopen(Chan *c, int omode)
{
	Kexecgrp *kg;
	Kvalue *e;
	int trunc;

	kg = kexecgrp(c);
	if(c->qid.type & QTDIR) {
		if(omode != OREAD)
			error(Eperm);
	}else {
		trunc = omode & OTRUNC;
		if(omode != OREAD && !kexecwriteable(c))
			error(Eperm);
		if(trunc)
			wlock(kg);
		else
			rlock(kg);
		e = kexeclookup(kg, 0, c->qid.path);
		if(e == 0) {
			if(trunc)
				wunlock(kg);
			else
				runlock(kg);
			error(Enonexist);
		}
		if(trunc && e->size) { // better validity check?
			e->qid.vers++;
			e->size = 0;
			e->len = 0;
		}
		if(trunc)
			wunlock(kg);
		else
			runlock(kg);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
kexeccreate(Chan *c, char *name, int omode, int)
{
	Kexecgrp *kg;
	Kvalue *e;
	Kvalue **ent;
	uintptr addr;

	addr = strtoull(name, nil, 0);

	if(c->qid.type != QTDIR)
		error(Eperm);

	omode = openmode(omode);
	kg = kexecgrp(c);

	wlock(kg);
	if(waserror()) {
		wunlock(kg);
		nexterror();
	}

	if(kexeclookup(kg, addr, -1))
		error(Eexist);

	e = smalloc(sizeof(Kvalue));
	e->addr = addr;

	if(kg->nent == kg->ment){
		kg->ment += 32;
		ent = smalloc(sizeof(kg->ent[0])*kg->ment);
		if(kg->nent)
			memmove(ent, kg->ent, sizeof(kg->ent[0])*kg->nent);
		free(kg->ent);
		kg->ent = ent;
	}
	e->qid.path = ++kg->path;
	e->qid.vers = 0;
	kg->vers++;
	kg->ent[kg->nent++] = e;
	c->qid = e->qid;

	wunlock(kg);
	poperror();

	c->offset = 0;
	c->mode = omode;
	c->flag |= COPEN;
}

static void
kexecremove(Chan *c)
{
	int i;
	Kexecgrp *kg;
	Kvalue *e;

	if(c->qid.type & QTDIR)
		error(Eperm);

	kg = kexecgrp(c);
	wlock(kg);
	e = 0;
	for(i=0; i<kg->nent; i++){
		if(kg->ent[i]->qid.path == c->qid.path){
			e = kg->ent[i];
			kg->nent--;
			kg->ent[i] = kg->ent[kg->nent];
			kg->vers++;
			break;
		}
	}
	wunlock(kg);
	if(e == 0)
		error(Enonexist);
	free(e);
}

static void
kexecclose(Chan *c)
{
	/*
	 * cclose can't fail, so errors from remove will be ignored.
	 * since permissions aren't checked,
	 * kexecremove can't not remove it if its there.
	 */
	if(c->flag & CRCLOSE)
		kexecremove(c);
}

static int32_t
kexecread(Chan *c, void *a, int32_t n, int64_t off)
{
	Kexecgrp *kg;
	Kvalue *e;
	int32_t offset;

	if(c->qid.type & QTDIR)
		return devdirread(c, a, n, 0, 0, kexecgen);

	kg = kexecgrp(c);
	rlock(kg);
	e = kexeclookup(kg, 0, c->qid.path);
	if(e == 0) {
		runlock(kg);
		error(Enonexist);
	}

	offset = off;
	if(offset > e->len)	/* protects against overflow converting vlong to long */
		n = 0;
	else if(offset + n > e->len)
		n = e->len - offset;
	if(n <= 0)
		n = 0;
//	else
//		memmove(a, e->value+offset, n);
	runlock(kg);
	return n;
}

/*

need to make slots. the slots themselves can be set somewhere else.

need make the writes 

open will handle the parsing of the hex numbers.

no, do it the other way around. just define the slots. 
can work on the interface later. 

kmap the space where the values need to stay safe. 

then when that is correct you can do it the other. 

kmap address range
put it in 


write is going to be significantly different. 

the first thing to do is to make this just work. 
	add to the kernel cfg.

*/

static int32_t
kexecwrite(Chan *c, void *a, int32_t n, int64_t off)
{
	Kexecgrp *kg;
	Kvalue *e;
	int32_t offset;

	if(n <= 0)
		return 0;
	offset = off;
	if(offset > Maxkexecsize || n > (Maxkexecsize - offset))
		error(Etoobig);
	print("a: %s\n", a);
	kg = kexecgrp(c);
	wlock(kg);
	e = kexeclookup(kg, 0, c->qid.path);
	if(e == 0) {
		wunlock(kg);
		error(Enonexist);
	}

	// XXX: what to do with what is written?

	e->qid.vers++;
	kg->vers++;
	wunlock(kg);
	return n;
}

Dev kexecdevtab = {
	L'§',
	"kexec",

	devreset,
	devinit,
	devshutdown,
	kexecattach,
	kexecwalk,
	kexecstat,
	kexecopen,
	kexeccreate,
	kexecclose,
	kexecread,
	devbread,
	kexecwrite,
	devbwrite,
	kexecremove,
	devwstat,
};

void
kexeccpy(Kexecgrp *to, Kexecgrp *from)
{
	int i;
	Kvalue *ne, *e;

	rlock(from);
	to->ment = (from->nent+31)&~31;
	to->ent = smalloc(to->ment*sizeof(to->ent[0]));
	for(i=0; i<from->nent; i++){
		e = from->ent[i];
		ne = smalloc(sizeof(Kvalue));
		ne->addr = e->addr;
		ne->size = e->size;
		ne->qid.path = ++to->path;
		to->ent[i] = ne;
	}
	to->nent = from->nent;
	runlock(from);
}

void
closekgrp(Kexecgrp *kg)
{
	int i;
	Kvalue *e;

	if(decref(kg) == 0){
		for(i=0; i<kg->nent; i++){
			e = kg->ent[i];
			free(e);
		}
		free(kg->ent);
		free(kg);
	}
}

static Kexecgrp*
kexecgrp(Chan *c)
{
	if(c->aux == nil)
		return &kgrp;
	return c->aux;
}

static int
kexecwriteable(Chan *c)
{
	return iseve() || c->aux == nil;
}

