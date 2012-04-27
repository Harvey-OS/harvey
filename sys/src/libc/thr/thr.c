#include <u.h>
#include <libc.h>
#include <tos.h>
#include "dat.h"

#define DBG	if(tdebug)print

static Thr *runq0;	/* head of ready queue */
static Thr *runqn;	/* tail of ready queue */
static Thr *freeq;	/* dead threads to recycle */
static Thr *run;		/* running thread */
static Thr *sysq;	/* threads waiting for system calls */
static int tidgen;
static char *tstatus;	/* exit status */

int tdebug;

static void
dumpq(Thr *t, char *tag)
{
	print("%s:", tag);
	if(t == nil){
		print(" none\n");
		return;
	}
	for(; t != nil; t = t->next)
		print(" %s!%s(%d)", argv0, t->name, t->id);
	print("\n");
}

static void
dump(void)
{
	if(run != nil)
		print("run: %s!%s %d\n", argv0, run->name, run->id);
	dumpq(runq0, "runq");
	dumpq(sysq, "sysq");
	dumpq(freeq, "freeq");
}

static void
tfree(Thr *t)
{
	t->next = freeq;
	free(t->tag);
	t->tag = nil;
	freeq = t;
}

static Thr*
trecycle(void)
{
	Thr *t;

	t = freeq;
	if(t != nil){
		freeq = t->next;
		t->next = nil;
	}
	return t;
}

static Thr*
tnext(void)
{
	Thr *t;

	assert(runq0 != nil);
	t = runq0;
	runq0 = t->next;
	if(runq0 == nil)
		runqn = nil;
	return t;
}

static void
trdy(Thr *t)
{
	t->state = Trdy;
	if(runqn == nil)
		runq0 = t;
	else
		runqn->next = t;
	runqn = t;
}

static void
tunsys(Thr *t)
{
	if(t == sysq)
		sysq = t->next;
	if(t->next)
		t->next->prev = t->prev;
	if(t->prev)
		t->prev->next = t->next;
	t->next = t->prev = nil;
}

static void
tsys(Thr *t)
{
	t->state = Tsys;
	t->next = sysq;
	if(sysq)
		sysq->prev = t;
	sysq = t;
}

static void
syscalls(void)
{
	Thr *t;
	Nixret nr;

	/*
	 * Collect replies for previous system calls
	 */
	while((t = nixret(&nr)) != nil){
		t->nr = nr;
		DBG("nixret %s!%s %d: %d err %s\n",
			argv0, t->name, t->id, t->nr.sret, t->nr.err);
		tunsys(t);
		trdy(t);
		if(tdebug>1)dump();
	}
}

static void
tsched(int state)
{
	if(run != nil){
		if(setjmp(run->label) != 0)
			return;
		switch(state){
		case Trdy:
			trdy(run);
			break;
		case Tsys:
			tsys(run);
			break;
		case Tdead:
			tfree(run);
			break;
		default:
			sysfatal("tsched: state %d", state);
		}
		run = nil;
	}

	syscalls();
	while(runq0 == nil){
		syscalls();
		sleep(0);	/* BUG: poll for a while, then block */
		if(sysq == nil && runq0 == nil){
			DBG("%s: tsched: none ready\n", argv0);
			exits(tstatus);
		}
	}

	run = tnext();
	if(tdebug>1)dump();
	longjmp(run->label, 1);
}

void
texits(char *sts)
{
	if(sts != nil)
		tstatus = sts;
	DBG("texits %s!%s %d%s\n", argv0, run->name, run->id, sts?sts:"");
	tsched(Tdead);
}

static void
thr0(void)
{
	run->main(run->argc, run->argv);
	texits(nil);
}

int
gettid(void)
{
	return run->id;
}

int
newthr(char *tag, void (*f)(int, void*[]), int argc, void*argv[])
{
	Thr *t;
	static int once;

	t = trecycle();
	if(t == nil){
		t = mallocz(sizeof *t, 1);
		if(t == nil)
			sysfatal("newthr: no memory");
		t->tag = t;
		t->stk = malloc(Stack);
		if(t->stk == nil)
			sysfatal("newthr: no memory");
	}
	t->id = ++tidgen;
	t->name = strdup(tag);
	if(t->tag == nil)
		sysfatal("newthr: no memory");
	t->state = Trdy;
	t->main = f;
	t->argc = argc;
	t->argv = malloc(sizeof(void*)*argc);
	if(t->argv == nil)
		sysfatal("newthr: no memory");
	memmove(t->argv, argv, sizeof(void*)*argc);
	t->label[JMPBUFPC] = (uintptr)thr0;
	t->label[JMPBUFSP] = (uintptr)(t->stk + Stack - 2 * sizeof(ulong*));
	trdy(t);
	DBG("newthr %s!%s %d\n", argv0, t->name, t->id);
	/*
	 * The first call does not return, and jumps to the new thread.
	 */
	if(once == 0){
		once = 1;
		tsched(Trdy);
	}
	return t->id;
}

int
tsyscall(int nr, ...)
{
	run->scall = nr;
	va_start(run->sarg, nr);
	nixcall(run, 1);
	tsched(Tsys);
	va_end(run->sarg);
	return run->nr.sret;
}
