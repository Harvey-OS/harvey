#include	"dat.h"
#include	"fns.h"

static struct
{
	QLock	l;
	Rendez	producer;
	Rendez	consumer;
	Rendez	clock;
	ulong	randomcount;
	uchar	buf[1024];
	uchar	*ep;
	uchar	*rp;
	uchar	*wp;
	uchar	next;
	uchar	bits;
	uchar	wakeme;
	uchar	filled;
	int	kprocstarted;
	ulong	randn;
	int	target;
} rb;

static int
rbnotfull(void *v)
{
	int i;

	USED(v);
	i = rb.wp - rb.rp;
	if(i < 0)
		i += sizeof(rb.buf);
	return i < rb.target;
}

static int
rbnotempty(void *v)
{
	USED(v);
	return rb.wp != rb.rp;
}

/*
 *  spin counting up
 */
static void
genrandom(void *v)
{
	USED(v);
	oslopri();
	for(;;){
		for(;;)
			if(++rb.randomcount > 65535)
				break;
		if(rb.filled || !rbnotfull(0))
			Sleep(&rb.producer, rbnotfull, 0);
	}
}

/*
 *  produce random bits in a circular buffer
 */
static void
randomclock(void *v)
{
	uchar *p;

	USED(v);
	for(;; osmillisleep(20)){
		while(!rbnotfull(0)){
			rb.filled = 1;
			Sleep(&rb.clock, rbnotfull, 0);
		}
		if(rb.randomcount == 0)
			continue;

		rb.bits = (rb.bits<<2) ^ rb.randomcount;
		rb.randomcount = 0;

		rb.next++;
		if(rb.next != 8/2)
			continue;
		rb.next = 0;

		p = rb.wp;
		*p ^= rb.bits;
		if(++p == rb.ep)
			p = rb.buf;
		rb.wp = p;

		if(rb.wakeme)
			Wakeup(&rb.consumer);
	}
}

void
randominit(void)
{
	rb.target = 16;
	rb.ep = rb.buf + sizeof(rb.buf);
	rb.rp = rb.wp = rb.buf;
}

/*
 *  consume random bytes from a circular buffer
 */
ulong
randomread(void *xp, ulong n)
{
	uchar *e, *p, *r;
	ulong x;
	int i;

	p = xp;

if(0)print("A%ld.%d.%lux|", n, rb.target, getcallerpc(&xp));
	if(waserror()){
		qunlock(&rb.l);
		nexterror();
	}

	qlock(&rb.l);
	if(!rb.kprocstarted){
		rb.kprocstarted = 1;
		kproc("genrand", genrandom, 0, 0);
		kproc("randomclock", randomclock, 0, 0);
	}

	for(e = p + n; p < e; ){
		r = rb.rp;
		if(r == rb.wp){
			rb.wakeme = 1;
			Wakeup(&rb.clock);
			Wakeup(&rb.producer);
			Sleep(&rb.consumer, rbnotempty, 0);
			rb.wakeme = 0;
			continue;
		}

		/*
		 *  beating clocks will be predictable if
		 *  they are synchronized.  Use a cheap pseudo
		 *  random number generator to obscure any cycles.
		 */
		x = rb.randn*1103515245 ^ *r;
		*p++ = rb.randn = x;

		if(++r == rb.ep)
			r = rb.buf;
		rb.rp = r;
	}
	if(rb.filled && rb.wp == rb.rp){
		i = 2*rb.target;
		if(i > sizeof(rb.buf) - 1)
			i = sizeof(rb.buf) - 1;
		rb.target = i;
		rb.filled = 0;
	}
	qunlock(&rb.l);
	poperror();

	Wakeup(&rb.clock);
	Wakeup(&rb.producer);

if(0)print("B");
	return n;
}
