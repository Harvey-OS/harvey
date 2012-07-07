#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/error.h"
#include <tos.h>
#include "ureg.h"

/* from linux */

struct iovec {
	void *base;
	int len;
};


struct utsname {
	char sysname[65];
	char nodename[65];
	char release[65];
	char version[65];
	char machine[65];
	char gnu[65];
};


struct timeval
  {
    u32int tv_sec;		/* Seconds.  */
    u32int tv_usec;	/* Microseconds.  */
  };

struct  rusage {
        struct timeval ru_utime;        /* user time used */
        struct timeval ru_stime;        /* system time used */
        long    ru_maxrss;              /* maximum resident set size */
        long    ru_ixrss;               /* integral shared memory size */
        long    ru_idrss;               /* integral unshared data size */
        long    ru_isrss;               /* integral unshared stack size */
        long    ru_minflt;              /* page reclaims */
        long    ru_majflt;              /* page faults */
        long    ru_nswap;               /* swaps */
        long    ru_inblock;             /* block input operations */
        long    ru_oublock;             /* block output operations */
        long    ru_msgsnd;              /* messages sent */
        long    ru_msgrcv;              /* messages received */
        long    ru_nsignals;            /* signals received */
        long    ru_nvcsw;               /* voluntary context switches */
        long    ru_nivcsw;              /* involuntary " */
};

/*Linux kerne.org 2.6.35-30-generic #54-Ubuntu SMP Tue Jun 7 18:41:54 UTC 2011 x86_64 GNU/Linux\n*/
struct utsname linuxutsname = {
	 "Linux", "mynode", "2.6.35", "NIX", "x86_64", "GNUsucks"
};

void
linuxexit(Ar0*, va_list list)
{
	int val;
	char exitstr[32] = "";
	
	val = va_arg(list, int);
	if (val)
		snprint(exitstr, sizeof(exitstr), "%d", val);
	if (up->attr & 128) print("%d:linuxexit %d\n", up->pid, val);
	up->attr = 0;
	pexit(exitstr, 1);
}

void
linuxuname(Ar0*ar, va_list list)
{
	void *va;
	va = va_arg(list, void *);
	if (up->attr & 128) print("%d:linuxuname va %p\n", up->pid, va);
	validaddr(va, 1, 1);
	memmove(va, &linuxutsname, sizeof(linuxutsname));
	// if this does not work we will need a /proc for bgl 
	// uname is just such a piece of shit. Some systems want things of the size in the struct,
	// others don't. Idiots. 
//#define BULLSHIT "Linux\0 NIX\0 2.6.19\0NIX\0x86_64\0GNUsucks"
//	memmove(va, BULLSHIT, strlen(BULLSHIT)+1);
	if (up->attr&128) print("Returns %s\n", linuxutsname.release);
	ar->i = 0;
}

/* special case: interpretation of '0' is done in USER MODE on Plan 9 */
/* but old deprecated sysbrk_ does what we need */
void
linuxbrk(Ar0* ar0, va_list list)
{
//	void linuxsbrk(Ar0* ar0, va_list list);
	void sysbrk_(Ar0*, va_list);
	uintptr va, newva;
	//void *arg[1];
	va = va_arg(list, uintptr);
	if (up->attr & 128) print("%d:linuxbrk va %#p: ", up->pid, (void *)va);
	//arg[0] = va;
	//sysbrk_(ar0, (va_list) arg);
	newva = ibrk(va, BSEG);
	/* it is possible, though unlikely, that libc wants exactly the value it asked for. Plan 9 is returning rounded-up-to-next-page values. */
	if (va)	
		ar0->v = (void *)va;
	else
		ar0->v = (void *)newva;
	if (up->attr & 128) print("returns %#p\n", va);

}

void
linuxopen(Ar0 *ar0, va_list list)
{
	va_list s = list;
	char *aname;
	int omode;
	void sysopen(Ar0 *, va_list);
	aname = va_arg(list, char*);
	omode = va_arg(list, int);
	USED(aname,omode);
	sysopen(ar0, s);
}

void
linuxclose(Ar0 *ar0, va_list list)
{
	int fd;
	void sysclose(Ar0 *, va_list);
	fd = va_arg(list, int);
	USED(fd);
	sysclose(ar0, list);
}

void
linuxwritev(Ar0 *ar0, va_list list)
{
	void sys_write(Ar0* ar0, va_list list);
	int fd;
	struct iovec *iov;
	int iovcnt;
	int i;
	fd = va_arg(list, int);
	iov = va_arg(list, struct iovec *);
	iovcnt = va_arg(list, int);
	if (up->attr & 128) print("%d:linuxwritev (%d, %p, %d):", up->pid, fd, iov, iovcnt);
	validaddr(iov, iovcnt * sizeof(*iov), 0);
	/* don't validate all the pointers in the iov; sys_write does this */	
	for(i = 0; i < iovcnt; i++) {
		Ar0 war0;
		uintptr arg[3];
		if (up->attr & 128) print("[%p,%d],", iov[i].base, iov[i].len);
		arg[0] = fd;
		arg[1] = (uintptr) iov[i].base;
		arg[2] = iov[i].len;
		sys_write(&war0, (va_list) arg);
		if (war0.l < 0)
			break;
		/* if the first one fails, we get 0 */
		ar0->l += war0.l;
	}
	if (up->attr & 128) print("\n");
}


void
linuxsocketcall(Ar0 *ar0, va_list list)
{
	int fd;
	uintptr *args;

	USED(ar0);

	fd = va_arg(list, int);
	args = va_arg(list, uintptr *);
	if (up->attr & 128) print("%d:linuxsocketcall (%d, %p):", up->pid, fd, args);
	validaddr(args,sizeof(*args), 0);
	if (up->attr & 128) print("\n");
}


void
linuxgeteuid(Ar0 *ar0, va_list)
{
	ar0->i = 0;
}

/* ow this hurts. */
typedef unsigned long int __ino_t;
typedef long long int __quad_t;
typedef unsigned int __mode_t;
typedef unsigned int __nlink_t;
typedef long int __off_t;
typedef unsigned int __uid_t;
typedef unsigned int __gid_t;
typedef long int __blksize_t;
typedef long int __time_t;
typedef long int __blkcnt_t;

typedef unsigned long long int __u_quad_t;

typedef __u_quad_t __dev_t;

struct timespec
  {
    __time_t tv_sec;
    long int tv_nsec;
  };
/*
# 103 "/bgsys/drivers/V1R2M0_200_2008-080513P/ppc/gnu-linux/lib/gcc/powerpc-bgp-linux/4.1.2/../../../../powerpc-bgp-linux/sys-include/sys/stat.h" 3 4
*/
/* how many stat structs does linux have? too many. */
struct stat {
     __dev_t st_dev;
    unsigned short int __pad1;
    __ino_t st_ino;
    __mode_t st_mode;
    __nlink_t st_nlink;
    __uid_t st_uid;
    __gid_t st_gid;
    __dev_t st_rdev;
    unsigned short int __pad2;
    __off_t st_size;
    __blksize_t st_blksize;
    __blkcnt_t st_blocks;
    struct timespec st_atim;
    struct timespec st_mtim;
    struct timespec st_ctim;
    unsigned long int __unused4;
    unsigned long int __unused5;
} stupid = {
	.st_blksize = 4096,
	.st_dev = 1,
	.st_gid = 0,
	.st_ino = 0x12345,
	.st_mode = 0664 | 020000,
	.st_nlink = 1,
	.st_rdev = 501
};

void
fstat64(Ar0 *ar0, va_list list)
{
	void *v;
	int fd;
	fd = va_arg(list, int);
	v = va_arg(list, void *);
	validaddr(v, 1, 0);
	switch(fd) {
		case 0:
		case 1:
		case 2: 
			ar0->i = 0;
			memmove(v, &stupid, sizeof(stupid));
			break;
	}

}


/* do nothing, succesfully */
void
returnok(Ar0*, va_list)
{

	return;
}

/* void  *  mmap(void *start, size_t length, int prot , int flags, int fd,
       off_t offset); */
/* can be a malloc or a true map of a file. fd == -1 is a malloc */

void linuxmmap(Ar0 *ar0, va_list list)
{
	void *v;
	uintptr length, prot, flags, fd, nn;
	u64int offset;
	void linuxsbrk(Ar0* ar0, va_list list);
        Chan *c = nil;
	uintptr newv, oldv, iop;
	uintptr ret;

	v = va_arg(list, void *);
	length = va_arg(list, uintptr);
	prot = va_arg(list, int);
	flags = va_arg(list, int);
	fd = va_arg(list, int);
	offset = va_arg(list, u64int);
	if (up->attr & 128)
		print("%d: mmap %p %#x %#ullx %#x %d %#ulx\n", up->pid, v, length, prot, flags, fd, offset);
	/* if we are going to use the fd make sure it's valid */
	if (fd > -1)
        	c = fdtochan(fd, OREAD, 1, 1);

        if(waserror()){
		if (c)
			cclose(c);
		if (up->attr & 128)
			print("%d: mmap failed, return %#ullx\n", up->pid, ar0->p);
                nexterror();
	}

	/* allocate the memory we need. We might allocate more than needed. */
	oldv = ibrk(0, BSEG);

	if (up->attr & 128)
		print("%d:mmap anon: current is %p\n", up->pid, oldv);
	newv =  oldv + length;
	if (up->attr & 128)
		print("%d:mmap anon: ask for %p\n", up->pid, newv);
	ret = ibrk(newv, BSEG);
	if (up->attr & 128)
		print("%d:mmap anon: new is %p\n", up->pid, ret);
	nixprepage(BSEG);

	if (fd == -1){
		ar0->p = oldv;
		poperror();
		return;
	}

	iop = oldv;
	while (length > 0){
		nn = c->dev->read(c, (void *)oldv, length, offset);
		if (nn <= 0)
			break;
		offset += nn;
		length -= nn;
		oldv += nn;
	}
	poperror();
	cclose(c);
	if (up->attr & 128)
		print("%d:mmap file at %p\n", up->pid, iop);
	ar0->p = iop;
}


void linuxprocid(Ar0 *ar0, va_list)
{
	ar0->i = 0;
}

/*Kernel_Ranks2Coords((kernel_coords_t *)_mapcache, _fullSize);*/


/* int sigaction(int sig, const struct sigaction *restrict act,
              struct sigaction *restrict oact); */

void sigaction(Ar0 *ar0, va_list list)
{
	void *act, *oact;
	act = va_arg(list, void *);
	oact = va_arg(list, void *);
	if (up->attr & 128) print("%d:sigaction, %p %p\n", up->pid, act, oact);
	ar0->i = 0;
}

/*long rt_sigprocmask (int how, sigset_t *set, sigset_t *oset, size_t sigsetsize); */

void rt_sigprocmask(Ar0 *ar0, va_list list)
{
	int how;
	void *set, *oset;
	int size;
	how = va_arg(list,int);
	set = va_arg(list, void *);
	oset = va_arg(list, void *);
	size = va_arg(list, int);
	if (up->attr & 128) print("%d:sigaction, %d %p %p %d\n", up->pid, how, set, oset, size);
	ar0->l = 0;
}

/* damn. Did not want to do futtocks */
#define FUTEX_WAIT              0
#define FUTEX_WAKE              1
#define FUTEX_FD                2
#define FUTEX_REQUEUE           3
#define FUTEX_CMP_REQUEUE       4
#define FUTEX_WAKE_OP           5
#define FUTEX_LOCK_PI           6
#define FUTEX_UNLOCK_PI         7
#define FUTEX_TRYLOCK_PI        8
#define FUTEX_WAIT_BITSET       9
#define FUTEX_WAKE_BITSET       10


void futex(Ar0 *ar0, va_list list)
{
	int *uaddr, op, val;
	int *uaddr2, val3;
	struct timespec *timeout;
	uaddr = va_arg(list,int *);
	op = va_arg(list, int);
	val = va_arg(list, int);
	timeout = va_arg(list, struct timespec *);
	uaddr2 = va_arg(list, int *);
	val3 = va_arg(list, int);
	USED(uaddr);
	USED(op);
	USED(val);
	USED(timeout);
	USED(uaddr2);
	USED(val3);
	if (up->attr & 128) print("%d:futex, uaddr %p op %x val %x uaddr2 %p timeout  %p val3 %x\n", up->pid, 
			uaddr, op, val, uaddr2, timeout, val);
	switch(op) {
		default: 
			ar0->i = -1;
			break;
		case FUTEX_WAIT: 
			/*
			 * This  operation atomically verifies that the futex address uaddr
			 * still contains the value val, and sleeps awaiting FUTEX_WAKE  on
			 * this  futex  address.   If the timeout argument is non-NULL, its
			 * contents describe the maximum duration of  the  wait,  which  is
			 * infinite  otherwise.  The arguments uaddr2 and val3 are ignored.
			 */
			validaddr(uaddr, sizeof(*uaddr), 0);
			if (up->attr & 128) print("%d:futex: value at %p is %#x, val is %#x\n", up->pid, uaddr, *uaddr, val);
			if (*uaddr != val) {
				ar0->i = -11;
				return;
			}
			if (timeout) {
				validaddr(timeout, sizeof(*timeout), 0);
				if (timeout->tv_sec == timeout->tv_nsec == 0)
					return;
			}
			if (up->attr & 128) print("%d:Not going to sleep\n", up->pid);
			break;
	}
}

/* mprotect is used to set a stack red zone. We may want to use
 * segattach for anon page alloc and then use segdetach for the same purpose. 
 */
void linuxmprotect(Ar0 *ar0, va_list list)
{
	u32int addr, len, prot;
	addr = va_arg(list, u32int);
	len = va_arg(list, u32int);
	prot = va_arg(list, u32int);
	if (up->attr & 128) print("%d:mprotect: %#x %#x %#x\n", up->pid, addr, len, prot);
	ar0->i = 0;
}

/* this is a hack. */
Segment*
linuxdupseg(Segment **seg, int segno, int share)
{
	int i, size;
	Pte *pte;
	Segment *n, *s;

	SET(n);
	s = seg[segno];

	qlock(&s->lk);
	if(waserror()){
		qunlock(&s->lk);
		nexterror();
	}
	switch(s->type&SG_TYPE) {
	case SG_TEXT:		/* New segment shares pte set */
	case SG_SHARED:
	case SG_PHYSICAL:
		goto sameseg;

	case SG_STACK:
		/* linux wants to share the stack. */
		if(share){ if (up->attr & 128) print("CLONE STACK IS SHARE\n");
			goto sameseg;
		}
		/* that is all the change */
if (up->attr & 128) print("CLONE STACK IS NEW\n");
		n = newseg(s->type, s->base, s->size);
		break;

	case SG_BSS:		/* Just copy on write */
		if(share)
			goto sameseg;
if (up->attr & 128) print("CLONE NEW BSS\n");
		n = newseg(s->type, s->base, s->size);
		break;

	case SG_DATA:		/* Copy on write plus demand load info */
		if(segno == TSEG){
			poperror();
			qunlock(&s->lk);
			return data2txt(s);
		}

		if(share)
			goto sameseg;
		n = newseg(s->type, s->base, s->size);

		incref(s->image);
		n->image = s->image;
		n->fstart = s->fstart;
		n->flen = s->flen;
		break;
	}
	size = s->mapsize;
	for(i = 0; i < size; i++)
		if(pte = s->map[i])
			n->map[i] = ptecpy(n, pte);

	n->flushme = s->flushme;
	if(s->ref > 1)
		procflushseg(s);
	poperror();
	qunlock(&s->lk);
	return n;

sameseg:
	incref(s);
	poperror();
	qunlock(&s->lk);
	return s;
}

/* the big problem here is that linux clone wants to allow the user to set the 
 * stack. It's stupid but it's what they do. Linux NPTL pretty much requires it. 
 * we are going to be dumb here for now and assume we only use 
 * RFPROC|RFMEM. 
 * What do we do about stack longer term? It gets a bit weird. 
 * the child process stack is in the data segment. Should we make the 
 * child process stack segment a DATA segment, share the data segment, 
 * and make it a STACK for the child? If we did things right in linuxclone
 * we could remove our private dupseg. 
 */
void linuxclone(Ar0 *ar0, va_list list)
{
	void linuxsysrforkchild(Proc* child, Proc* parent, uintptr newsp);
	u32int flags,  stack;
	Proc *p;
	int flag, i, n, pid;
	Mach *wm;
	flags = va_arg(list, u32int);
	stack = va_arg(list, uintptr);
	up->attr = 0xfe;
	if (up->attr & 128) print("%d:CLONE: %#x %#ullx\n", up->pid, flags, stack);
	if (flags != 0x7d0f00) {
		print("%d:CLONE: don't know what to do with flags %#x\n", up->pid, flags);
		ar0->i = -1;
		return;
	}
	flag = RFPROC | RFMEM;

	p = newproc();

	p->trace = up->trace;
	p->scallnr = up->scallnr;
	memmove(p->arg, up->arg, sizeof(up->arg));
	p->nerrlab = 0;
	p->slash = up->slash;
	p->dot = up->dot;
	incref(p->dot);

	memmove(p->note, up->note, sizeof(p->note));
	p->privatemem = up->privatemem;
	p->noswap = up->noswap;
	p->nnote = up->nnote;
	p->notified = 0;
	p->lastnote = up->lastnote;
	p->notify = up->notify;
	p->ureg = up->ureg;
	p->dbgreg = 0;
	p->attr = Linux;

	/* Make a new set of memory segments */
	n = flag & RFMEM;
	qlock(&p->seglock);
	if(waserror()){
		qunlock(&p->seglock);
		nexterror();
	}
	for(i = 0; i < NSEG; i++)
		if(up->seg[i])
			p->seg[i] = linuxdupseg(up->seg, i, n);
	qunlock(&p->seglock);
	poperror();

	/* File descriptors */
	if(flag & (RFFDG|RFCFDG)) {
		if(flag & RFFDG)
			p->fgrp = dupfgrp(up->fgrp);
		else
			p->fgrp = dupfgrp(nil);
	}
	else {
		p->fgrp = up->fgrp;
		incref(p->fgrp);
	}

	/* Process groups */
	if(flag & (RFNAMEG|RFCNAMEG)) {
		p->pgrp = newpgrp();
		if(flag & RFNAMEG)
			pgrpcpy(p->pgrp, up->pgrp);
		/* inherit noattach */
		p->pgrp->noattach = up->pgrp->noattach;
	}
	else {
		p->pgrp = up->pgrp;
		incref(p->pgrp);
	}
	if(flag & RFNOMNT)
		up->pgrp->noattach = 1;

	if(flag & RFREND)
		p->rgrp = newrgrp();
	else {
		incref(up->rgrp);
		p->rgrp = up->rgrp;
	}

	/* Environment group */
	if(flag & (RFENVG|RFCENVG)) {
		p->egrp = smalloc(sizeof(Egrp));
		p->egrp->ref = 1;
		if(flag & RFENVG)
			envcpy(p->egrp, up->egrp);
	}
	else {
		p->egrp = up->egrp;
		incref(p->egrp);
	}
	p->hang = up->hang;
	p->procmode = up->procmode;

	/* Craft a return frame which will cause the child to pop out of
	 * the scheduler in user mode with the return register zero
	 */
	/* fix the stack for linux semantics */
	linuxsysrforkchild(p, up, stack);

	p->parent = up;
	p->parentpid = up->pid;
	if(flag&RFNOWAIT)
		p->parentpid = 0;
	else {
		lock(&up->exl);
		up->nchild++;
		unlock(&up->exl);
	}
	if((flag&RFNOTEG) == 0)
		p->noteid = up->noteid;

	pid = p->pid;
	memset(p->time, 0, sizeof(p->time));
	p->time[TReal] = sys->ticks; // MACHP(0)->ticks;

	kstrdup(&p->text, up->text);
	kstrdup(&p->user, up->user);
	/*
	 *  since the bss/data segments are now shareable,
	 *  any mmu info about this process is now stale
	 *  (i.e. has bad properties) and has to be discarded.
	 */
	mmuflush();
	p->basepri = up->basepri;
	p->priority = up->basepri;
	p->fixedpri = up->fixedpri;
	p->mp = up->mp;
	wm = up->wired;
	if(wm)
		procwired(p, wm->machno);
	ready(p);
	sched();

	ar0->i = pid;
}

void timeconv(ulong l, struct timeval *t)
{
	u32int ms;
	ms = TK2MS(l);
	t->tv_sec += ms / 1000;
	ms %= 1000;
	t->tv_usec += ms * 1000;
}
void getrusage(Ar0 *ar0, va_list list)
{
	int what;
	struct rusage *r;	

	what = va_arg(list, int);
	r = va_arg(list, struct rusage *);
	validaddr(r, sizeof(*r), 1);

	if (up->attr & 128) print("%d:getrusage: %s %p:",up->pid,  !what? "self" : "kids", r);
	memset(r, 0, sizeof(*r));
	if (what) {
		timeconv(up->time[3], &r->ru_utime);
		timeconv(up->time[4], &r->ru_stime);
	} else {
		timeconv(up->time[0], &r->ru_utime);
		timeconv(up->time[1], &r->ru_stime);
		if (up->attr & 128) print("%#lx:%#lx, ", up->time[0], up->time[1]);
	}
	if (up->attr & 128) print("%#x:%#x\n", r->ru_utime.tv_sec, r->ru_stime.tv_sec);

	ar0->i = 0;	
}
