#include <u.h>
#include <libc.h>
#include <thread.h>
#include "threadimpl.h"

int
threadid(void)
{
	return _threadgetproc()->thread->id;
}

int
threadpid(int id)
{
	int pid;
	Proc *p;
	Thread *t;

	if (id < 0)
		return -1;
	if (id == 0)
		return _threadgetproc()->pid;
	lock(&_threadpq.lock);
	for (p = _threadpq.head; p->next; p = p->next){
		lock(&p->lock);
		for (t = p->threads.head; t; t = t->nextt)
			if (t->id == id){
				pid = p->pid;
				unlock(&p->lock);
				unlock(&_threadpq.lock);
				return pid;
			}
		unlock(&p->lock);
	}
	unlock(&_threadpq.lock);
	return -1;
}

int
threadsetgrp(int ng)
{
	int og;
	Thread *t;

	t = _threadgetproc()->thread;
	og = t->grp;
	t->grp = ng;
	return og;
}

int
threadgetgrp(void)
{
	return _threadgetproc()->thread->grp;
}

void
threadsetname(char *name)
{
	int fd;
	char buf[40];
	Proc *p;
	Thread *t;

	p = _threadgetproc();
	t = p->thread;
	if (t->cmdname)
		free(t->cmdname);
	t->cmdname = strdup(name);
	if(p->nthreads == 1){
		snprint(buf, sizeof buf, "#p/%d/args", getpid());
		if((fd = open(buf, OWRITE)) >= 0){
			write(fd, name, strlen(name));
			close(fd);
		}
	}
}

char*
threadgetname(void)
{
	return _threadgetproc()->thread->cmdname;
}

void**
threaddata(void)
{
	return &_threadgetproc()->thread->udata;
}

void**
procdata(void)
{
	return &_threadgetproc()->udata;
}

