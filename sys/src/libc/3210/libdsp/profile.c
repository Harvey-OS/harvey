#include	<u.h>
#include	<libc.h>

extern	long*	_clock;
long*	_lastclock;
extern	long	_callpc(void**);
extern	long	_savearg(void);

typedef	struct	Plink	Plink;
typedef	struct	Prof	Prof;
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

int
_profdump(void)
{
	long n;
	Plink *p;
	char *vp;

	__prof.pp = 0;
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
		n = -p->time / (55000/4);
		vp[0] = n>>24;
		vp[1] = n>>16;
		vp[2] = n>>8;
		vp[3] = n;
		vp += 4;
	}
	return vp - (char*)__prof.first;
}

void*
_profmain(void)
{
	static Plink links[2000];

	__prof.first = links;
	__prof.last = &links[2000];
	__prof.next = __prof.first;
	return &__prof;
}
