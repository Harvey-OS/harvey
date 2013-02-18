#include	"dat.h"
#include	"fns.h"
#include	"error.h"


enum
{
	KSTACK	= 16*1024,
	DELETE	= 0x7F,
};

Proc	**Xup;

extern	void	killrefresh(void);
extern	void	tramp(char*, void (*)(void*), void*);

extern	int	usenewwin;

int	*ustack;	/* address on unshared stack: see vstack in asm*.s */
extern	int	dflag;
char *hosttype = "Plan9";
char *cputype;

void
osblock(void)
{
	rendezvous(up, nil);
}

void
osready(Proc *p)
{
	rendezvous(p, nil);
}

void
pexit(char *msg, int)
{
	Osenv *e;

	USED(msg);

	lock(&procs.l);
	if(up->prev) 
		up->prev->next = up->next;
	else
		procs.head = up->next;

	if(up->next)
		up->next->prev = up->prev;
	else
		procs.tail = up->prev;
	unlock(&procs.l);

/*	print("pexit: %s: %s\n", up->text, msg);	/**/
	e = up->env;
	if(e != nil) {
		closefgrp(e->fgrp);
		closepgrp(e->pgrp);
		closeegrp(e->egrp);
		closesigs(e->sigs);
	}
	free(e->user);
	free(up->prog);
	up->prog = nil;
	up->type = Moribund;
	longjmp(up->privstack, 1);
}

int
kproc1(char *name, void (*func)(void*), void *arg, int flags)
{
	int pid;
	Proc *p;
	Pgrp *pg;
	Fgrp *fg;
	Egrp *eg;

	p = newproc();
	if(p == nil)
		panic("kproc: no memory");
	p->kstack = mallocz(KSTACK, 0);
	if(p->kstack == nil)
		panic("kproc: no memory");

	if(flags & KPDUPPG) {
		pg = up->env->pgrp;
		incref(&pg->r);
		p->env->pgrp = pg;
	}
	if(flags & KPDUPFDG) {
		fg = up->env->fgrp;
		incref(&fg->r);
		p->env->fgrp = fg;
	}
	if(flags & KPDUPENVG) {
		eg = up->env->egrp;
		incref(&eg->r);
		p->env->egrp = eg;
	}

	p->env->uid = up->env->uid;
	p->env->gid = up->env->gid;
	kstrdup(&p->env->user, up->env->user);

	strcpy(p->text, name);

	p->func = func;
	p->arg = arg;

	lock(&procs.l);
	if(procs.tail != nil) {
		p->prev = procs.tail;
		procs.tail->next = p;
	}
	else {
		procs.head = p;
		p->prev = nil;
	}
	procs.tail = p;
	unlock(&procs.l);

	/*
	 * switch back to the unshared stack to do the fork
	 * only the parent returns from kproc
	 */
	up->kid = p;
	up->kidsp = p->kstack;
	pid = setjmp(up->sharestack);
	if(pid == 0)
		longjmp(up->privstack, 1);
	return pid;
}

void
kproc(char *name, void (*func)(void*), void *arg, int flags)
{
	kproc1(name, func, arg, flags);
}

void
traphandler(void *reg, char *msg)
{
	int intwait;

	intwait = up->intwait;
	up->intwait = 0;
	/* Ignore pipe writes from devcmd */
	if(strstr(msg, "write on closed pipe") != nil)
		noted(NCONT);

	if(sflag) {
		if(intwait && strcmp(msg, Eintr) == 0)
			noted(NCONT);
		else
			noted(NDFLT);
	}
	if(intwait == 0)
		disfault(reg, msg);
	noted(NCONT);
}

static int
readfile(char *path, char *buf, int n)
{
	int fd;

	fd = open(path, OREAD);
	if(fd >= 0) {
		n = read(fd, buf, n-1);
		if(n > 0)			/* both calls to readfile() have a ``default'' */
			buf[n] = '\0';
		close(fd);
		return n;
	}
	return 0;
}

static void
dobinds(void)
{
	char dir[MAXROOT+9];

	snprint(dir, sizeof(dir), "%s/net", rootdir);
	bind("/net", dir, MREPL);

	snprint(dir, sizeof(dir), "%s/net.alt", rootdir);
	bind("/net.alt", dir, MREPL);

	snprint(dir, sizeof(dir), "%s/dev", rootdir);
	bind("#t", dir, MAFTER);
	bind("#A", dir, MAFTER);
	bind("#p", "/proc", MAFTER);
}

void adddynproc();

void
libinit(char *imod)
{
	char *sp;
	Proc *xup, *p;
	int fd, efd, n, pid;
	char nbuf[64];

// XXX: why does inferno do this?
	xup = nil;
	Xup = &xup;
//	Xup = malloc(1024*sizeof(Proc*));

	/*
	 * setup personality
	 */
	if(readfile("/dev/user", nbuf, sizeof nbuf))
		kstrdup(&eve, nbuf);
	if(readfile("/dev/sysname", nbuf, sizeof nbuf))
		kstrdup(&ossysname, nbuf);
	if(readfile("/env/cputype", nbuf, sizeof nbuf))
		kstrdup(&cputype, nbuf);


	/*
	 * guess at a safe stack for vstack
	 */
	ustack = &fd;

	rfork(RFNAMEG|RFREND);
/*
	if(!dflag){
		fd = open("/dev/consctl", OWRITE);
		if(fd < 0)
			fprint(2, "libinit: open /dev/consctl: %r\n");
		n = write(fd, "rawon", 5);
		if(n != 5)
			fprint(2, "keyboard rawon (n=%d, %r)\n", n);
	}
*/
	osmillisec();	/* set the epoch */
	dobinds();

	notify(traphandler);

	/*
	 * dummy up a up and stack so the first proc
	 * calls emuinit after setting up his private jmp_buf
	 */
	p = newproc();
	// XXX: are we going to do exceptions on the kstack?
	p->kstack = mallocz(KSTACK, 0);
	if(p == nil || p->kstack == nil)
		panic("libinit: no memory");
	sp = p->kstack;
	// XXX: is this what we want? 
	// So what are you  doing here? you need to be able to set up kprocs. 
	p->func = plan9init;
	p->arg = imod;

	/*
	 * set up a stack for forking kids on separate stacks.
	 * longjmp back here from kproc.
	 */
	while(setjmp(p->privstack)){
		if(up->type == Moribund){
			free(up->kstack);
			free(up);
			_exits("");
		}
		p = up->kid;
		sp = up->kidsp;
		up->kid = nil;
		switch(pid = rfork(RFPROC|RFMEM|RFNOWAIT)){
		case 0:
			/*
			 * send the kid around the loop to set up his private jmp_buf
			 */
			break;
		default:
			/*
			 * parent just returns to his shared stack in kproc
			 */
			longjmp(up->sharestack, pid);
			panic("longjmp failed");
		}
	}
	/*
	 * you get here only once per Proc
	 * go to the shared memory stack
	 */
	up = p;
	up->pid = up->sigid = getpid();
	tramp(sp+KSTACK, up->func, up->arg);
	panic("tramp returned");
}

void
oshostintr(Proc *p)
{
	postnote(PNPROC, p->sigid, Eintr);
}

void
oslongjmp(void *regs, osjmpbuf env, int val)
{
	if(regs != nil)
		notejmp(regs, env, val);
	else
		longjmp(env, val);
}

void
osreboot(char*, char**)
{
}

void
cleanexit(int x)
{
	USED(x);
// XXX: put this back when we do the win stuff.
//	killrefresh();
	postnote(PNGROUP, getpid(), "interrupt");
	exits("interrupt");
}

int
readkbd(void)
{
	int n;
	char buf[1];

	n = read(0, buf, sizeof(buf));
	if(n < 0)
		fprint(2, "emu: keyboard read error: %r\n");
	if(n <= 0)
		pexit("keyboard", 0);
	switch(buf[0]) {
	case DELETE:
		cleanexit(0);
	case '\r':
		buf[0] = '\n';
	}
	return buf[0];
}

static vlong
b2v(uchar *p)
{
	int i;
	vlong v;

	v = 0;
	for(i=0; i<sizeof(uvlong); i++)
		v = (v<<8)|p[i];
	return v;
}

vlong
nsec(void)
{
	int n;
	static int nsecfd = -1;
	uchar buf[sizeof(uvlong)];

	if(nsecfd < 0){
		nsecfd = open("/dev/bintime", OREAD|OCEXEC);  /* never closed */
		if(nsecfd<0){
			fprint(2,"can't open /dev/bintime: %r\n");
			return 0;
		}
	}
	n = read(nsecfd, buf, sizeof(buf));
	if(n!=sizeof(buf)) {
		fprint(2,"read err on /dev/bintime: %r\n");
		return 0;
	}
	return b2v(buf);
}

long
osmillisec(void)
{
	static vlong nsec0 = 0;

	if(nsec0 == 0){
		nsec0 = nsec();
		return 0;
	}
	return (nsec()-nsec0)/1000000;
}

/*
 * Return the time since the epoch in microseconds
 * The epoch is defined at 1 Jan 1970
 */
vlong
osusectime(void)
{
	return nsec()/1000;
}

int
osmillisleep(ulong milsec)
{
	sleep(milsec);
	return 0;
}

int
limbosleep(ulong milsec)
{
	return osmillisleep(milsec);
}

void
osyield(void)
{
	sleep(0);
}

void
ospause(void)
{
	for(;;)
		sleep(1000000);
}

void
oslopri(void)
{
	int fd;
	char buf[32]; 

	snprint(buf, sizeof(buf), "/proc/%d/ctl", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		fprint(fd, "pri 8");
		close(fd);
	}
}

