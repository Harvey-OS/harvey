#include <alef.h>
#include "run.h"

Lock		block;
intern int 	rand;
intern Ref	nproc;

/*
 * The loader rearranges stack offset to ensure double alignment off SP for
 * automatics and doubles. This allows FxxxD instructions to be used.
 * This is the only difference between the MIPS and SPARC runtime.
 */
#define ALIGN(s, n)	((uint)(s)&~(n-1))
/*
 * Default stack allocation
 */
int ALEFstack = 16384;

void	main(int, char*);

/*
 * Build argv/argc on shared memory stack.  Link the process stack into
 * shared memory and call main.
 */

void
ALEF_init(int argc, char **argv)
{
	Tdb *tdb;
	Task *t;
	int i;
	uint ssize, *rp;
	char *stack, *sp;

	rand = -1;

	/*
	 * count arg size
	 */
	ssize = sizeof(int)+(2*sizeof(char*));
	for(i = 0; i < argc; i++)
		ssize += strlen(argv[i]) + sizeof(char*) + 1;

	/*
	 * Link to stack in shared memory
	 */
	stack = malloc(ALEFstack+ssize+sizeof(Task)+sizeof(Tdb));
	if(stack == nil)
		abort();

	/*
	 * set up the u-level scheduler
	 */
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
	sp = stack+ALEFstack;
	sp = (char*)ALIGN(sp, 8);	/* Double align */
	sp -= 4;			/* ALEF_linksp does sp-4 so this will align */
	rp = (uint*)sp;
	rp[0] = argc;
	rp[1] = (int)&rp[2];
	rp += 2;
	stack = (char*)rp + argc*sizeof(char*);
	for(i = 0; i < argc; i++) {
		*rp++ = (int)stack;
		strcpy(stack, argv[i]);
		stack += strlen(argv[i])+1;
	}

	nproc.inc();
	PROC.tdb = tdb;
	ALEF_linksp(&i, sp, main);
}

/*
 * Called by alloc to allocate and initialise channels
 */

void*
ALEF_chana(int nbuf, int bsize)
{
	int s;
	Chan *c;
	Msgbuf *b;
	
	c = malloc(sizeof(Chan));
	if(nbuf && c != nil) {
		s = (sizeof(Msgbuf)+bsize)*nbuf;
		b = malloc(s);
		c->free = b;
		c->async = 1;
		while(nbuf > 1) {
			b->next = b+1;
			b++;
			nbuf--;
		}
	}
	return c;
}

void*
ALEF_chanu(Chan *c)
{
	Msgbuf *m, *next;

	/* Channel is still busy */
	if(c->qh != nil || c->sva != nil || c->rva != nil)
		abort();

	for(m = c->free; m; m = next) {
		next = m->next;
		free(m);		
	}
	free(c);
}

/*
 * called by failed checks
 */
void
ALEF_assert(char *s)
{
	write(2, s, strlen(s));
	abort();
}

/*
 * Return true if a process can send on this channel without blocking
 */

int
ALEF_csnd(Chan *c)
{
	if(c->rva || c->free)
		return 1;

	return 0;
}

/*
 * Return true if a process can receive on this channel without blocking
 */

int
ALEF_crcv(Chan *c)
{
	if(c->sva || c->qh)
		return 1;

	return 0;
}

/*
 * Append a channel structure to a process private select structure ready
 * to do a selrecv.
 */

void
ALEF_selrecv(Chan *c)
{
	Task *t;

	t = PROC.tdb->ctask;

	c->lock();
	if(t->slist)
		t->stail->sellink = c;
	else
		t->slist = c;
	t->stail = c;
	c->sellink = nil;
	c->selt = t;
	c->unlock();
}

/*
 * Build a list of IO ready channels, pick a random ready channel and return
 * its index in the list. This drives switch code generated for the select
 */

int
ALEF_doselect(void)
{
	Chan *f;
	Task *t;
	int *sema;
	int nrdy, n, idx;

	t = PROC.tdb->ctask;
	sema = &t->slist->selp;

	for(;;) {
		nrdy = 0;
		*sema = 0;
		for(f = t->slist; f; f = f->sellink)
			if(f->sva || f->qh)
				nrdy++;
	
		if(nrdy == 0) {
			t->Sleep((void**)sema, 1);
			n = 0;
		}
		else {
			rand += rand;
			if(rand < 0)
				rand ^= 0x88888EEF;
			n = rand%nrdy;
		}

		idx = 0;
		for(f = t->slist; f; f = f->sellink) {
			if(f->sva || f->qh) {
				if(n == 0) {
					for(f = t->slist; f; f = f->sellink)
						f->selt = nil;
					t->slist = nil;
					return idx;
				}
				n--;
			}
			idx++;
		}
	}
}

void
ALEF_sndint(Chan *c, int val)
{
	Msgbuf *b;

	c->snd.lock();
	c->lock();
	if(c->rva) {
		*(int*)c->rva = val;
		c->rva = nil;
		c->unlock();
		c->snd.unlock();
		c->rcvr.Wakeup();
		return;
	}
	if(c->async) {
		for(;;) {
			b = c->free;
			if(b == nil)
				break;
			c->unlock();
			c->br.Sleep(&c->free, 1);
			c->lock();
		}
		c->free = b->next;
		b->i = val;
		if(c->qh == nil)
			c->qh = b;
		else
			c->qt->next = b;
		c->qt = b;
		b->next = nil;
		c->unlock();
		c->snd.unlock();
		return;
	}
	c->sva = &val;
	if(c->selt) {
		c->selp = 1;
		c->selt->Wakeup();
	}
	c->unlock();
	c->sndr.Sleep(&c->sva, 0);
	c->snd.unlock();
}

void
ALEF_sndflt(Chan *c, float val)
{
	Msgbuf *b;

	c->snd.lock();
	c->lock();
	if(c->rva) {
		*(float*)c->rva = val;
		c->rva = nil;
		c->unlock();
		c->snd.unlock();
		c->rcvr.Wakeup();
		return;
	}
	if(c->async) {
		for(;;) {
			b = c->free;
			if(b == nil)
				break;
			c->unlock();
			c->br.Sleep(&c->free, 1);
			c->lock();
		}
		c->free = b->next;
		b->f = val;
		if(c->qh == nil)
			c->qh = b;
		else
			c->qt->next = b;
		c->qt = b;
		b->next = nil;
		c->unlock();
		c->snd.unlock();
		return;
	}
	c->sva = &val;
	c->unlock();
	if(c->selt) {
		c->selp = 1;
		c->selt->Wakeup();
	}
	c->sndr.Sleep(&c->sva, 0);
	c->snd.unlock();
}

void
ALEF_sndsint(Chan *c, sint val)
{
	Msgbuf *b;

	c->snd.lock();
	c->lock();
	if(c->rva) {
		*(sint*)c->rva = val;
		c->rva = nil;
		c->unlock();
		c->snd.unlock();
		c->rcvr.Wakeup();
		return;
	}
	if(c->async) {
		for(;;) {
			b = c->free;
			if(b == nil)
				break;
			c->unlock();
			c->br.Sleep(&c->free, 1);
			c->lock();
		}
		c->free = b->next;
		b->s = val;
		if(c->qh == nil)
			c->qh = b;
		else
			c->qt->next = b;
		c->qt = b;
		b->next = nil;
		c->unlock();
		c->snd.unlock();
		return;
	}
	c->sva = &val;
	c->unlock();
	if(c->selt) {
		c->selp = 1;
		c->selt->Wakeup();
	}
	c->sndr.Sleep(&c->sva, 0);
	c->snd.unlock();
}

void
ALEF_sndmem(Chan *c, void *p, int len)
{
	Msgbuf *b;

	c->snd.lock();
	c->lock();
	if(c->rva) {
		memmove(c->rva, p, len);
		c->rva = nil;
		c->unlock();
		c->snd.unlock();
		c->rcvr.Wakeup();
		return;
	}
	if(c->async) {
		for(;;) {
			b = c->free;
			if(b == nil)
				break;
			c->unlock();
			c->br.Sleep(&c->free, 1);
			c->lock();
		}
		c->free = b->next;
		memmove(b->data, p, len);
		if(c->qh == nil)
			c->qh = b;
		else
			c->qt->next = b;
		c->qt = b;
		b->next = nil;
		c->unlock();
		c->snd.unlock();
		return;
	}
	c->sva = p;
	if(c->selt) {
		c->selp = 1;
		c->selt->Wakeup();
	}
	c->unlock();
	c->sndr.Sleep(&c->sva, 0);
	c->snd.unlock();
}

int
ALEF_rcvint(Chan *c)
{
	int ret;
	Msgbuf *b;

	c->rcv.lock();
	c->lock();
	if(c->qh) {
		b = c->qh;
		ret = b->i;
		c->qh = b->next;
		b->next = c->free;
		c->free = b;
		c->unlock();
		if(b->next == nil)
			c->br.Wakeup();
		c->rcv.unlock();
		return ret;
	}
	if(c->sva) {
		ret = *(int*)(c->sva);
		c->sva = nil;
		c->unlock();
		c->rcv.unlock();
		c->sndr.Wakeup();
		return ret;
	}
	c->rva = &ret;
	c->unlock();
	c->rcvr.Sleep(&c->rva, 0);
	c->rcv.unlock();
	return ret;
}

float
ALEF_rcvflt(Chan *c)
{
	float ret;
	Msgbuf *b;

	c->rcv.lock();
	c->lock();
	if(c->qh) {
		b = c->qh;
		ret = b->f;
		c->qh = b->next;
		b->next = c->free;
		c->free = b;
		c->unlock();
		if(b->next == nil)
			c->br.Wakeup();
		c->rcv.unlock();
		return ret;
	}
	if(c->sva) {
		ret = *(float*)(c->sva);
		c->sva = nil;
		c->unlock();
		c->rcv.unlock();
		c->sndr.Wakeup();
		return ret;
	}
	c->rva = &ret;
	c->unlock();
	c->rcvr.Sleep(&c->rva, 0);
	c->rcv.unlock();
	return ret;
}

sint
ALEF_rcvsint(Chan *c)
{
	sint ret;
	Msgbuf *b;

	c->rcv.lock();
	c->lock();
	if(c->qh) {
		b = c->qh;
		ret = b->s;
		c->qh = b->next;
		b->next = c->free;
		c->free = b;
		c->unlock();
		if(b->next == nil)
			c->br.Wakeup();
		c->rcv.unlock();
		return ret;
	}
	if(c->sva) {
		ret = *(sint*)(c->sva);
		c->sva = nil;
		c->unlock();
		c->rcv.unlock();
		c->sndr.Wakeup();
		return ret;
	}
	c->rva = &ret;
	c->unlock();
	c->rcvr.Sleep(&c->rva, 0);
	c->rcv.unlock();
	return ret;
}

void
ALEF_rcvmem(Chan *c, int l)
{
	void *p;
	Msgbuf *b;

	p = ALEF_getrp();
	c->rcv.lock();
	c->lock();
	if(c->qh) {
		b = c->qh;
		memmove(p, b->data, l);
		c->qh = b->next;
		b->next = c->free;
		c->free = b;
		c->unlock();
		if(b->next == nil)
			c->br.Wakeup();
		c->rcv.unlock();
		return;
	}
	if(c->sva) {
		memmove(p, c->sva, l);
		c->sva = nil;
		c->unlock();
		c->rcv.unlock();
		c->sndr.Wakeup();
		return;
	}
	c->rva = p;
	c->unlock();
	c->rcvr.Sleep(&c->rva, 0);
	c->rcv.unlock();
}

/*
 * Start a new process, build an argument area in the new stack, copy
 * accross the parameters. linksp clears the parents spin location after
 * stack transfer
 */
void
ALEF_proc(void (*f)(void), int argsize, ...)
{
	Task *t;
	Tdb *tdb;
	int spin;
	char *stack;

	stack = malloc(ALEFstack+sizeof(Task)+sizeof(Tdb));
	if(stack == nil)
		abort();

	t = (Task*)stack;
	t->stack = stack;
	stack += sizeof(Task);

	tdb = (Tdb*)stack;
	stack += sizeof(Tdb);
	tdb->ntask = 1;
	tdb->runhd = nil;
	tdb->ctask = t;
	t->tdb = tdb;

	stack = (char*)ALIGN(stack, 8);
	argsize = ALIGN(argsize+7, 8);
	stack = stack+ALEFstack-(argsize+sizeof(char*));
	memmove(stack, ..., argsize);

	nproc.inc();

	spin = 1;
	if(rfork(RFNOWAIT|RFMEM|RFPROC) == 0) {
		PROC.tdb = tdb;
		ALEF_linksp(&spin, stack, f);
	}

	while(spin)
		;
}

void
ALEF_task(void (*f)(void), int argsize, ...)
{
	Task *t;
	Tdb *tdb;
	char *stack;

	stack = malloc(ALEFstack+sizeof(Task));
	if(stack == nil)
		abort();

	t = (Task*)stack;
	t->stack = stack;
	stack += sizeof(Task);

	tdb = PROC.tdb;
	tdb->lock();
	tdb->ntask++;
	if(tdb->runhd)
		tdb->runtl->link = t;
	else
		tdb->runhd = t;
	tdb->runtl = t;
	t->tdb = tdb;
	tdb->unlock();

	stack = (char*)ALIGN(stack, 8);
	argsize = ALIGN(argsize+7, 8);
	stack = stack+ALEFstack-(argsize+sizeof(char*));
	memmove(stack, ..., argsize);

	t->sp = (uint)stack-4;
	*(uint*)t->sp = (uint)f;
	t->pc = (uint)ALEF_linktask - 8; /* CALL saves pc of call, so return adds 8 */
}

void
exits(char *s)
{
	int p;
	Task *t, *me;
	Tdb *tdb;

	tdb = PROC.tdb;

	tdb->ntask--;
	if(tdb->ntask == 0) {
		p = nproc.dec();
		if(p == 0)
			_exits(s);
		else
			__exits(s);
	}

	tdb->lock();
	me = tdb->ctask;
	t = tdb->runhd;
	if(t != nil)
		tdb->runhd = t->link;
	tdb->ctask = t;
	tdb->unlock();

	if(tdb->ctask == nil)
		rendezvous(tdb, 0);
			
	ALEF_switch(me, tdb->ctask, me->stack);
}

int
ALEF_pfork(int n, char **stack)
{
	int *semap;
	int id, nid, sema;

	*stack++ = (char*)malloc(ALEFstack)+ALEFstack;

	id = 0;
	while(n) {
		sema = 1;
		/*
		 * Using &sema is required to prevent the optimiser
		 * from removing the assignment to sema
		 */
		semap = &sema;
		nid = id;
		if(rfork(RFNOWAIT|RFMEM|RFPROC) == 0) {
			*semap = 0;
			return nid;
		}
		while(*semap)
			;
		n--;
		id++;
	}
	*stack = nil;
	return id;
}

void
ALEF_pdone(int *sema, char **stv)
{
	char *p;

	while(*sema)
		;
	while(*stv) {
		p = *stv-ALEFstack;
		free(p);
		stv++;
	}
}

void*
ALEF_tid()
{
	return PROC.tdb->ctask;
}

void
ALEF_pexit(int *barrier)
{
	block.lock();
	(*barrier)--;
	block.unlock();
	__exits(nil);
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

void
Rendez.Sleep(Rendez *r, void **bool, int t)
{
	int i;
	void *s;
	Tdb *tdb;
	Task *me, *rh;

	r->lock();
	if(r->t)		/* Rendez already used */
		abort();

	if(bool) {
		i = *(int*)bool;
		if((i&&t) || (!i&&!t)) {
			r->unlock();
			return;
		}
	}

	tdb = PROC.tdb;
	me = tdb->ctask;
	r->t = me;

	tdb->lock();
	r->unlock();

	rh = tdb->runhd;
	if(rh != nil)
		tdb->runhd = rh->link;
	tdb->ctask = rh;
	tdb->unlock();

	if(tdb->ctask == nil)
		rendezvous(tdb, 0);

	s = ALEF_switch(me, tdb->ctask, nil);
	if(s)
		free(s);
}

void
Rendez.Wakeup(Rendez *r)
{
	Task *t;

	r->lock();
	t = r->t;
	if(t == nil) {
		r->unlock();
		return;
	}
	r->t = nil;
	r->unlock();

	ALEF_sched(t);
}

void
ALEF_sched(Task *t)
{
	void *s;
	Task *me;
	Tdb *tdb;

	tdb = t->tdb;
	tdb->lock();
	/* Synchronous swap between tasks ? */
	if(PROC.tdb == tdb) {
		me = tdb->ctask;
		me->link = nil;
		if(tdb->runhd)
			tdb->runtl->link = me;
		else
			tdb->runhd = me;
		tdb->runtl = me;

		tdb->ctask = t;
		tdb->unlock();
		s = ALEF_switch(me, t, nil);
		if(s)
			free(s);
		return;
	}

	if(tdb->ctask == nil) {
		tdb->ctask = t;
		rendezvous(tdb, 0);
	}
	else {
		t->link = nil;
		if(tdb->runhd)
			tdb->runtl->link = t;
		else
			tdb->runhd = t;
		tdb->runtl = t;
	}
	tdb->unlock();
}
