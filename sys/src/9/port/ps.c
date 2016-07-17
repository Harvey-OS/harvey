/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

int
nprocs(void)
{
	return procalloc.nproc;
}

void
pshash(Proc *p)
{
	int h;

	h = p->pid % nelem(procalloc.ht);
	lock(&procalloc.l);
	p->pidhash = procalloc.ht[h];
	procalloc.ht[h] = p;
	unlock(&procalloc.l);
}

void
psunhash(Proc *p)
{
	int h;
	Proc **l;

	h = p->pid % nelem(procalloc.ht);
	lock(&procalloc.l);
	for(l = &procalloc.ht[h]; *l != nil; l = &(*l)->pidhash)
		if(*l == p){
			*l = p->pidhash;
			break;
		}
	unlock(&procalloc.l);
}

int
psindex(int pid)
{
	Proc *p;
	int h;
	int s;

	s = -1;
	h = pid % nelem(procalloc.ht);
	lock(&procalloc.l);
	for(p = procalloc.ht[h]; p != nil; p = p->pidhash)
		if(p->pid == pid){
			s = p->index;
			break;
		}
	unlock(&procalloc.l);
	return s;
}

Proc*
psincref(int i)
{
	/*
	 * Placeholder.
	 */
	if(i >= conf.nproc)
		return nil;
	return &procalloc.arena[i];
}

void
psdecref(Proc *p)
{
	/*
	 * Placeholder.
	 */
	USED(p);
}

void
psrelease(Proc* p)
{
	p->qnext = procalloc.free;
	procalloc.free = p;
	procalloc.nproc--;
}

Proc*
psalloc(void)
{
	Proc *p;

	lock(&procalloc.l);
	for(;;) {
		if(p = procalloc.free)
			break;

		unlock(&procalloc.l);
		resrcwait("no procs");
		lock(&procalloc.l);
	}
	procalloc.free = p->qnext;
	procalloc.nproc++;
	unlock(&procalloc.l);

	return p;
}

void
psinit(int nproc)
{
	Proc *p;
	int i;

	procalloc.free = malloc(nproc*sizeof(Proc));
	if(procalloc.free == nil)
		panic("cannot allocate %u procs (%uMB)\n", nproc, nproc*sizeof(Proc)/(1024*1024));
	procalloc.arena = procalloc.free;

	p = procalloc.free;
	for(i=0; i<nproc-1; i++,p++){
		p->qnext = p+1;
		p->index = i;
	}
	p->qnext = 0;
	p->index = i;
}
