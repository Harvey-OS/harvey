#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"../port/tube.h"

static int semtrytimes = 100;


/*
 * Support for user-level optimistic semaphores.
 *
 * A semaphore is an integer n.
 * If n > 0, there are tickets
 * If n == 0, there are no tickets
 * If n < 0, there are |n| processes waiting for tickets
 * or preparing to wait for tickets.
 *
 * CAUTION: Do not use adec/ainc for semaphores, they would
 * trap if the integer is negative, but that's ok for semaphores.
 */


static void
semwakeup(Sem *s, int didwake, int dolock)
{
	Proc *p;

	DBG("semwakeup up %#p sem %#p\n", up, s->np);
	if(dolock)
		lock(s);
	/*
	 * If there are more processes sleeping than |*s->np| then
	 * there are ups not yet seen by sleepers, wake up those that
	 * have tickets.
	 */
	while(s->nq > 0 && s->nq > - *s->np){
		p = s->q[0];
		s->nq--;
		s->q[0] = s->q[s->nq];
		if(didwake){
			DBG("semwakeup up %#p waking up %#p\n", up, p);
			p->waitsem = s;
			/*
			 * p can be up if it's being killed, in which
			 * case it might be consuming a ticket being
			 * put while dying. In that case, another
			 * process might wait because we would get the
			 * ticket. Up must call semwakeup to be sure
			 * that other process no longer sleeps.
			 * But we can't be put in the scheduler queue.
			 */
			if(p != up)
				ready(p);
		}
	}
	if(dolock)
		unlock(s);
}

static void
semsleep(Sem *s, int dontblock)
{
	DBG("semsleep up %#p sem %#p\n", up, s->np);
	if(dontblock){
		/*
		 * User tried to down non-blocking, but someone else
		 * got the ticket between looking at n and adec(n).
		 * we have to safely undo our temporary down here.
		 * Adjust the value of the semaphore to reflect that we
		 * wanted a ticket for a while but no longer want one.
		 * Make sure that no other process is waiting because we
		 * made a temporary down.
		 */
		semainc(s->np);
		semwakeup(s, 1, 1);
		return;
	}
	lock(s);
	if(*s->np >= 0){
		/*
		 * A ticket came, either it came while calling the kernel,
		 * or it was a temporary sleep that didn't block.
		 * Either way, we are done.
		 */
		unlock(s);
		goto Done;
	}
	/*
	 * Commited to wait, we'll have to wait until
	 * some other process changes our state.
	 */
	s->q = realloc(s->q, (s->nq+1) * sizeof s->q[0]);
	if(s->q == nil)
		panic("semsleep: no memory");
	s->q[s->nq++] = up;
	up->waitsem = nil;
	up->state = Semdown;
	unlock(s);
	DBG("semsleep up %#p blocked\n", up);
	sched();
Done:
	DBG("semsleep up %#p awaken\n", up);
	if(up->waitsem == nil){
		/*
		 * nobody did awake us, we are probably being
		 * killed; we no longer want a ticket.
		 */
		lock(s);
		semainc(s->np);	/* we are no longer waiting; killed */
		semwakeup(s, 1, 0);
		unlock(s);
	}
}

void
syssemsleep(Ar0*, va_list list)
{
	int *np;
	int dontblock;
	Sem *s;
	Segment *sg;

	/*
	 * void semsleep(int*);
	 */
	np = va_arg(list, int*);
	np = validaddr(np, sizeof *np, 1);
	evenaddr(PTR2UINT(np));
	dontblock = va_arg(list, int);
	if((sg = seg(up, PTR2UINT(np), 0)) == nil)
		error(Ebadarg);
	s = segmksem(sg, np);
	semsleep(s, dontblock);
}

void
syssemwakeup(Ar0*, va_list list)
{
	int *np;
	Sem *s;
	Segment *sg;

	/*
	 * void semwakeup(int*);
	 */
	np = va_arg(list, int*);
	np = validaddr(np, sizeof *np, 1);
	evenaddr(PTR2UINT(np));
	if((sg = seg(up, PTR2UINT(np), 0)) == nil)
		error(Ebadarg);
	s = segmksem(sg, np);
	semwakeup(s, 1, 1);
}

static void
semdequeue(Sem *s)
{
	int i;

	assert(s != nil);
	lock(s);
	for(i = 0; i < s->nq; i++)
		if(s->q[i] == up)
			break;

	if(i == s->nq){
		/*
		 * We didn't perform a down on s, yet we are no longer queued
		 * on it; it must be because someone gave us its
		 * ticket in the mean while. We must put it back.
		 */
		semainc(s->np);
		semwakeup(s, 0, 0);
	}else{
		s->nq--;
		s->q[i] = s->q[s->nq];
	}
	unlock(s);
}

/*
 * Alternative down of a Sem in ss[].
 * The logic is similar to multiple downs, see comments in semsleep().
 */
static int
semalt(Sem *ss[], int n)
{
	int i, j, r;
	Sem *s;

	DBG("semalt up %#p ss[0] %#p\n", up, ss[0]->np);
	r = -1;
	for(i = 0; i < n; i++){
		s = ss[i];
		n = semadec(s->np);
		if(n >= 0){
			r = i;
			goto Done;
		}
		lock(s);
		s->q = realloc(s->q, (s->nq+1) * sizeof s->q[0]);
		if(s->q == nil)
			panic("semalt: not enough memory");
		s->q[s->nq++] = up;
		unlock(s);
	}

	DBG("semalt up %#p blocked\n", up);
	up->state = Semdown;
	sched();

Done:
	DBG("semalt up %#p awaken\n", up);
	for(j = 0; j < i; j++){
		assert(ss[j] != nil);
		if(ss[j] != up->waitsem)
			semdequeue(ss[j]);
		else
			r = j;
	}
	if(r < 0)
		panic("semalt");
	return r;
}

void
syssemalt(Ar0 *ar0, va_list list)
{
	int **sl;
	int i, *np, ns;
	Segment *sg;
	Sem *ksl[16];

	/*
	 * void semalt(int*[], int);
	 */
	ar0->i = -1;
	sl = va_arg(list, int**);
	ns = va_arg(list, int);
	sl = validaddr(sl, ns * sizeof(int*), 1);
	if(ns > nelem(ksl))
		panic("syssemalt: bug: too many semaphores in alt");
	for(i = 0; i < ns; i++){
		np = sl[i];
		np = validaddr(np, sizeof(int), 1);
		evenaddr(PTR2UINT(np));
		if((sg = seg(up, PTR2UINT(np), 0)) == nil)
			error(Ebadarg);
		ksl[i] = segmksem(sg, np);
	}	
	ar0->i = semalt(ksl, ns);
}

/*
 * These are the entry points from the C library, adapted
 * for use within the kernel, so that kprocs may share sems
 * with users.
 * They must be run in a process context.
 * Within the kernel, semaphores are used through their
 * kernel Sem structures, and not directly by their int* value.
 * Otherwise, we would have to look up each time they are used.
 */

void
upsem(Sem *s)
{
	int n;

	n = semainc(s->np);
	if(n <= 0)
		semwakeup(s, 1, 1);
}

int
downsem(Sem *s, int dontblock)
{
	int n, i;

	for(i = 0; *s->np <= 0 && i < semtrytimes; i++)
		yield();
	if(*s->np <= 0 && dontblock)
		return -1;
	n = semadec(s->np);
	if(n < 0)
		semsleep(s, dontblock);
	return 0;
}

int
altsems(Sem *ss[], int n)
{
	int i, w;

	/* busy wait */
	for(w = 0; w < semtrytimes; w++){
		for(i = 0; i < n; i++)
			if(*ss[i]->np > 0)
				break;
		if(i < n)
			break;
	}
	for(i = 0; i < n; i++)
		if(downsem(ss[i], 1) != -1)
			return i;
	return semalt(ss, n);
}

/*
 * Tube entry points as seen by the kernel.
 * They use Sem*, and not int*
 */

enum{Doblock, Dontblock, Already};	/* xsend() nb argument */

/*
 * Assuming t points to valid memory,
 * create a KTube for the Tube pointed to by t.
 * This is suitable for using following functions to operate
 * on user tubes from the kernel.
 */
KTube*
u2ktube(Tube *t)
{
	KTube *kt;
	Segment *sg;

	kt = smalloc(sizeof *kt);
	kt->t = t;
	if((sg = seg(up, PTR2UINT(&t->nhole), 0)) == nil)
		panic("u2ktube: was the tube verified?");
	kt->hole = segmksem(sg, &t->nhole);
	if(seg(up, PTR2UINT(&t->nmsg), 0) != sg)
		panic("u2ktube: tricked user tube?");
	kt->msg = segmksem(sg, &t->nmsg);
	return kt;
}

static int
xsend(KTube *kt, void *p, int nb)
{
	int n;
	uchar *c;
	Tube *t;

	t = kt->t;
	assert(t != nil && p != nil);
	if(nb != Already && downsem(kt->hole, nb) < 0)
		return -1;
	n = ainc(&t->tl) - 1;
	n %= t->tsz;
	c = (uchar*)&t[1];
	c += (1+t->msz) * n;
	memmove(c+1, p, t->msz);
	coherence();
	*c = 1;
	upsem(kt->msg);
	return 0;
}

static int
xrecv(KTube *kt, void *p, int nb)
{
	int n;
	uchar *c;
	Tube *t;

	t = kt->t;
	assert(t != nil && p != nil);
	if(nb != Already && downsem(kt->msg, nb) < 0)
		return -1;
	n = ainc(&t->hd) - 1;
	n %= t->tsz;
	c = (uchar*)&t[1];
	c += (1+t->msz) * n;
	while(*c == 0)
		yield();
	memmove(p, c+1, t->msz);
	coherence();
	*c = 0;
	upsem(kt->hole);
	return 0;
}

void
tsend(KTube *t, void *p)
{
	xsend(t, p, Doblock);
}

void
trecv(KTube *t, void *p)
{
	xrecv(t, p, Doblock);
}

int
nbtsend(KTube *t, void *p)
{
	return xsend(t, p, Dontblock);
}

int
nbtrecv(KTube *t, void *p)
{
	return xrecv(t, p, Dontblock);
}

int
talt(KTalt a[], int na)
{
	int i, n;
	Sem **ss;

	assert(a != nil && na > 0);
	ss = smalloc(sizeof(Sem*) * na);
	n = 0;
	for(i = 0; i < na; i++)
		switch(a[i].op){
		case TSND:
			ss[n++] = a[i].kt->hole;
			break;
		case TRCV:
			ss[n++] = a[i].kt->msg;
			break;
		case TNBSND:
			if(nbtsend(a[i].kt, a[i].m) != -1)
				return i;
			break;
		case TNBRCV:
			if(nbtrecv(a[i].kt, a[i].m) != -1)
				return i;
			break;
		}
	if(n == 0)
		return -1;
	i = altsems(ss, n);
	free(ss);
	if(i < 0)
		return -1;
	switch(a[i].op){
	case TSND:
		xsend(a[i].kt, a[i].m, Already);
		break;
	case TRCV:
		xrecv(a[i].kt, a[i].m, Already);
		break;
	default:
		panic("talt");
	}
	return i;
}

