/*
 * allocate and look up Procs, including hashing them.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void
pshash(Proc *p)
{
	int h;

	h = (uint)p->pid % nelem(procalloc.ht);
	lock(&procalloc);
	p->pidhash = procalloc.ht[h];
	procalloc.ht[h] = p;
	unlock(&procalloc);
}

void
psunhash(Proc *p)
{
	int h;
	Proc **l;

	h = (uint)p->pid % nelem(procalloc.ht);
	lock(&procalloc);
	for(l = &procalloc.ht[h]; *l != nil; l = &(*l)->pidhash)
		if(*l == p){
			*l = p->pidhash;
			break;
		}
	unlock(&procalloc);
}

int
psindex(uint pid)
{
	Proc *p;
	int h, s;

	s = -1;
	h = pid % nelem(procalloc.ht);
	lock(&procalloc);
	for(p = procalloc.ht[h]; p != nil; p = p->pidhash)
		if(p->pid == pid){
			s = p->index;
			break;
		}
	unlock(&procalloc);
	return s;
}

Proc*
psincref(int i)
{
	/*
	 * Placeholder.
	 */
	if(i >= PROCMAX)
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
}

static void
noprocpanic(char *msg)
{
	/*
	 * setting exiting will make hzclock() on each processor call exit(0).
	 * clearing our bit in machs avoids calling exit(0) from hzclock()
	 * on this processor.
	 */
	ilock(&active);
	cpuinactive(m->machno);
	active.exiting = 1;
	iunlock(&active);

	procdump();
	delay(1000);
	panic(msg);
}

Proc*
psalloc(void)
{
	char msg[64];
	Proc *p;

	lock(&procalloc);
	if((p = procalloc.free) == nil) {
		/*
		 * the situation is unlikely to heal itself.
		 * dump the proc table and restart.
		 */
		snprint(msg, sizeof msg, "no procs; %s forking",
			up? up->text: "kernel");
		noprocpanic(msg);
	}
	procalloc.free = p->qnext;
	p->hang = p->procctl = 0;	/* probably unnecessary paranoia */
	unlock(&procalloc);
	return p;
}

void
psinit(void)
{
	Proc *p;
	int i;

	procalloc.free = malloc(PROCMAX*sizeof(Proc));
	if(procalloc.free == nil)
		panic("cannot allocate %ud procs (%lludMB)",
			PROCMAX, (vlong)PROCMAX*sizeof(Proc)/MB);
	procalloc.arena = procalloc.free;

	p = procalloc.free;
	for(i=0; i<PROCMAX-1; i++,p++){
		p->qnext = p+1;
		p->index = i;
	}
	p->qnext = 0;
	p->index = i;
}
