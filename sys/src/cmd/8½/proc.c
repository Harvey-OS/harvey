#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include "dat.h"
#include "fns.h"

Proc*
newproc(void (*f)(void), void *arg)
{
	Proc *p;

	p = malloc(sizeof(Proc));
	if(p == 0)
		error("newproc");
	p->label[JMPBUFPC] = ((ulong)f+JMPBUFDPC);
	p->label[JMPBUFSP] = (ulong)(&p->stack[NSTACK-2]);	/* old pc, new pc */
	p->arg = arg;
	p->next = 0;
	p->nextrun = 0;
	p->dead = 0;
	return p;
}

void
sched(void)
{
	while(proc.head == 0)
		io();
	if(setjmp(proc.p->label) == 0){
    again:
		proc.p = proc.head;
		if(proc.tail == proc.head)
			proc.tail = 0;
		proc.head = proc.head->nextrun;
		if(proc.p->dead){
			free(proc.p);
			goto again;
		}
		longjmp(proc.p->label, 1);
	}
}

void
run(Proc *p)
{
	Proc *q;

	/*
	 * First check not already there; there are rare cases when this could happen
	 */
	if(proc.head){
		q = proc.head;
		do
			if(p == q)
				return;
		while(q = q->next);	/* assign = */
	}
	p->nextrun = 0;
	if(proc.tail)
		proc.tail->nextrun = p;
	if(proc.head == 0)
		proc.head = p;
	proc.tail = p;
}
