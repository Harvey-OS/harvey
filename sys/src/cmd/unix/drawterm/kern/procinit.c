#include "u.h"
#include "lib.h"
#include "dat.h"
#include "fns.h"
#include "error.h"

Rgrp *thergrp;

void
procinit0(void)
{
	Proc *p;

	p = newproc();
	p->fgrp = dupfgrp(nil);
	p->rgrp = newrgrp();
	p->pgrp = newpgrp();
	_setproc(p);

	up->slash = namec("#/", Atodir, 0, 0);
	cnameclose(up->slash->name);
	up->slash->name = newcname("/");
	up->dot = cclone(up->slash);
}

Ref pidref;

Proc*
newproc(void)
{
	Proc *p;

	p = mallocz(sizeof(Proc), 1);
	p->pid = incref(&pidref);
	strcpy(p->user, eve);
	p->syserrstr = p->errbuf0;
	p->errstr = p->errbuf1;
	strcpy(p->text, "drawterm");
	osnewproc(p);
	return p;
}

int
kproc(char *name, void (*fn)(void*), void *arg)
{
	Proc *p;

	p = newproc();
	p->fn = fn;
	p->arg = arg;
	p->slash = cclone(up->slash);
	p->dot = cclone(up->dot);
	p->rgrp = up->rgrp;
	if(p->rgrp)
		incref(&p->rgrp->ref);
	p->pgrp = up->pgrp;
	if(up->pgrp)
		incref(&up->pgrp->ref);
	p->fgrp = up->fgrp;
	if(p->fgrp)
		incref(&p->fgrp->ref);
	strecpy(p->text, p->text+sizeof p->text, name);

	osproc(p);
	return p->pid;
}

