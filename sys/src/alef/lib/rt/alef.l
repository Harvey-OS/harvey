#include	<alef.h>
#include	"run.h"

/* Default stack allocation */
int ALEFstack;
int ALEFrfflag = RFNOWAIT|RFMEM|RFPROC;

int 	ALEFxrand;
Ref	ALEFnproc;

void	main(int, byte*);
void	ALEF_lockinit(void);

/*
 * Build argv/argc on shared memory stack.  Link the process stack into
 * shared memory and call main.
 */

void
ALEF_init(int argc, byte **argv)
{
	Tdb *tdb;
	Task *t;
	int i;
	uint ssize, *rp;
	byte *stack, *sp;

	if(ALEFstack == 0)
		ALEFstack = 16000;

	ALEF_lockinit();

	ALEFxrand = -1;

	/*
	 * count arg size
	 */
	ssize = sizeof(int)+(2*sizeof(byte*));
	for(i = 0; i < argc; i++)
		ssize += strlen(argv[i]) + sizeof(byte*) + 1;
	ssize += sizeof(byte*);

	/*
	 * Link to stack in shared memory
	 */
	stack = ALEFalloc(ALEFstack+ssize+sizeof(Task)+sizeof(Tdb), 0);
	if(stack == nil)
		abort();

	/*
	 * set up the u-level scheduler
	 */
	memset(stack, 0, sizeof(Task)+sizeof(Tdb));
	t = (Task*)stack;
	t->stack = stack;
	stack += sizeof(Task);

	tdb = (Tdb*)stack;
	stack += sizeof(Tdb);
	tdb->ntask = 1;
	tdb->runhd = nil;
	tdb->ctask = t;
	t->tdb = tdb;

	/*
	 * build argv/argc
	 */
	sp = stack+ALEFstack-MAXBECOME;
	sp = (byte*) ALIGN(sp, 8);		/* see ../$objtype/run.h */
	sp -= ARGV_DELTA;			/* see ../$objtype/run.h */
	rp = (uint*)sp;
	rp[0] = argc;
	rp[1] = (int)&rp[2];
	rp += 2;
	stack = (byte*)rp + argc*sizeof(byte*)+sizeof(byte*);
	for(i = 0; i < argc; i++) {
		*rp++ = (int)stack;
		strcpy(stack, argv[i]);
		stack += strlen(argv[i])+1;
	}
	*rp = 0;

	ALEFnproc.inc();
	PROC.tdb = tdb;
	ALEF_linksp(&i, sp, main);
}

/*
 * default check
 */
void
ALEFassert(byte *locn, byte *mesg)
{
	byte err[ERRLEN];

	errstr(err);
	write(2, locn, strlen(locn));
	write(2, " ", 1);
	write(2, mesg, strlen(mesg));
	write(2, ", errstr(", 9);
	write(2, err, strlen(err));
	write(2, ")\n", 2);
	exits("ALEFcheck");
}
void	(*ALEFcheck)(byte*, byte*) = ALEFassert;

intern
void
ALEF_trans()
{
	void *s;

	s = PROC.tdb->ctask->stack;
	free(s);
	_exits(nil);
}

void
terminate(byte *s)
{
	int p;
	Tdb *tdb;
	void *stack;
	Task *t, *me;

	tdb = PROC.tdb;

	tdb->ntask--;
	if(tdb->ntask == 0) {
		p = ALEFnproc.dec();
		if(p == 0)
			exits(s);
		p = 1;
		ALEF_linksp(&p, (byte*)Ptab-4, ALEF_trans);
		while(p != 0)
			;
	}

	tdb->lock();
	me = tdb->ctask;
	t = tdb->runhd;
	if(t != nil)
		tdb->runhd = t->link;
	tdb->ctask = t;
	tdb->sleeper = t == nil;
	tdb->unlock();

	if(t == nil)
		rendezvous(tdb, 0);

	stack = ALEF_switch(me, tdb->ctask, me->stack);
	if(stack != nil)
		free(stack);
}

void*
ALEF_tid()
{
	return PROC.tdb->ctask;
}

void
ALEF_gin(Lock **lk)
{
	if(*lk == nil)
		*lk = malloc(sizeof(Lock));
	(*lk)->lock();
}

void
ALEF_gou(Lock **lk)
{
	(*lk)->unlock();
}
