#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	"sys9.h"

extern	long*	_clock;
extern	long	_callpc(void**);
extern	long	_savearg(void);
extern	void*	sbrk(int);

typedef unsigned long ulong;

typedef	struct	Plink	Plink;
struct	Plink
{
	Plink	*old;		/* known to be 0(ptr) */
	Plink	*down;
	Plink	*link;
	long	pc;
	long	count;
	long	time;		/* known to be 20(ptr) */
};

struct
{
	Plink	*pp;		/* known to be 0(ptr) */
	Plink	*next;		/* known to be 4(ptr) */
	Plink	*last;
	Plink	*first;
} __prof;

ulong
_profin(void)
{
	void *dummy;
	long pc;
	Plink *pp, *p;
	ulong arg;

	arg = _savearg();
	pc = _callpc(&dummy);
	pp = __prof.pp;
	if(pp == 0)
		return arg;

	for(p=pp->down; p; p=p->link)
		if(p->pc == pc)
			goto out;
	p = __prof.next + 1;
	if(p >= __prof.last) {
		__prof.pp = 0;
		return arg;
	}
	__prof.next = p;
	p->link = pp->down;
	pp->down = p;
	p->pc = pc;
	p->old = pp;
	p->down = 0;
	p->count = 0;

out:
	__prof.pp = p;
	p->count++;
	p->time += *_clock;
	return arg;		/* disgusting linkage */
}

ulong
_profout(void)
{
	Plink *p;
	ulong arg;

	arg = _savearg();
	p = __prof.pp;
	if(p) {
		p->time -= *_clock;
		__prof.pp = p->old;
	}
	return arg;
}

void
_profdump(void)
{
	int f;
	long n;
	Plink *p;
	char *vp;

	__prof.pp = 0;
	f = creat("prof.out", 0666);
	if(f < 0) {
		perror("create prof.out");
		return;
	}
	__prof.first->time = -*_clock;
	vp = (char*)__prof.first;
	for(p = __prof.first; p <= __prof.next; p++) {
		/*
		 * short down
		 */
		n = 0xffff;
		if(p->down)
			n = p->down - __prof.first;
		vp[0] = n>>8;
		vp[1] = n;

		/*
		 * short right
		 */
		n = 0xffff;
		if(p->link)
			n = p->link - __prof.first;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * long pc
		 */
		n = p->pc;
		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * long count
		 */
		n = p->count;
		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;

		/*
		 * long time
		 */
		n = -p->time;
		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;
	}
	write(f, (char*)__prof.first, vp - (char*)__prof.first);
	close(f);
}

void
_profmain(void)
{
	char ename[50];
	int n, f;

	n = 1000;
	f = _OPEN("/env/profsize", 0);
	if(f >= 0) {
		memset(ename, 0, sizeof(ename));
		_READ(f, ename, sizeof(ename)-1);
		_CLOSE(f);
		n = atol(ename);
	}
	__prof.first = sbrk(n*sizeof(Plink));
	__prof.last = sbrk(0);
	__prof.next = __prof.first;
	atexit(_profdump);
	*_clock = 1;
}
