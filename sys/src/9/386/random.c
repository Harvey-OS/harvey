/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"

struct Rb
{
	QLock;
	Rendez	producer;
	Rendez	consumer;
	uint32_t	randomcount;
	//unsigned char	buf[1024];
	/** WORKAROUND **/
	unsigned char	buf[10];
	/** END WORKAROUND **/
	unsigned char	*ep;
	unsigned char	*rp;
	unsigned char	*wp;
	unsigned char	next;
	unsigned char	wakeme;
	uint16_t	bits;
	uint32_t	randn;
} rb;

static int
rbnotfull(void* v)
{
	int i;

	i = rb.rp - rb.wp;
	return i != 1 && i != (1 - sizeof(rb.buf));
}

static int
rbnotempty(void* v)
{
	return rb.wp != rb.rp;
}

static void
genrandom(void* v)
{
	Proc *up = machp()->externup;
	up->basepri = PriNormal;
	up->priority = up->basepri;

	for(;;){
		for(;;)
			if(++rb.randomcount > 100000)
				break;
		if(anyhigher())
			sched();
		if(!rbnotfull(0))
			sleep(&rb.producer, rbnotfull, 0);
	}
}

/*
 *  produce random bits in a circular buffer
 */
static void
randomclock(void)
{
	if(rb.randomcount == 0 || !rbnotfull(0))
		return;

	rb.bits = (rb.bits<<2) ^ rb.randomcount;
	rb.randomcount = 0;

	rb.next++;
	if(rb.next != 8/2)
		return;
	rb.next = 0;

	*rb.wp ^= rb.bits;
	if(rb.wp+1 == rb.ep)
		rb.wp = rb.buf;
	else
		rb.wp = rb.wp+1;

	if(rb.wakeme)
		wakeup(&rb.consumer);
}

void
randominit(void)
{
	/* Frequency close but not equal to HZ */
	addclock0link(randomclock, 13);
	rb.ep = rb.buf + sizeof(rb.buf);
	rb.rp = rb.wp = rb.buf;
	kproc("genrandom", genrandom, 0);
}

/*
 *  consume random bytes from a circular buffer
 */
uint32_t
randomread(void *xp, uint32_t n)
{

	Proc *up = machp()->externup;
	uint8_t *e, *p;
	uint32_t x;

	p = xp;

	if(waserror()){
		qunlock(&rb);
		nexterror();
	}

	qlock(&rb);
	
	/** WORKAROUND **/
	for(e = p + n; p < e; ){
		x = (2 * rb.randn +1)%1103515245;
		*p++ = rb.randn = x;
	}
	qunlock(&rb);
	poperror();
	return n;
	/** END WORKAROUND **/
	for(e = p + n; p < e; ){
		if(rb.wp == rb.rp){
			rb.wakeme = 1;
			wakeup(&rb.producer);
			sleep(&rb.consumer, rbnotempty, 0);
			rb.wakeme = 0;
			continue;
		}

		/*
		 *  beating clocks will be precictable if
		 *  they are synchronized.  Use a cheap pseudo
		 *  random number generator to obscure any cycles.
		 */
		x = rb.randn*1103515245 ^ *rb.rp;
		*p++ = rb.randn = x;

		if(rb.rp+1 == rb.ep)
			rb.rp = rb.buf;
		else
			rb.rp = rb.rp+1;
	}
	qunlock(&rb);
	poperror();

	wakeup(&rb.producer);

	return n;
}

/**
 * Fast random generator
 **/
uint32_t
urandomread(void *xp, uint32_t n)
{
	Proc *up = machp()->externup;
	uint64_t seed[16];
	uint8_t *e, *p;
	uint32_t x=0;
	uint64_t s0;
	uint64_t s1;
	
	if(waserror()){
		nexterror();
	}
	//The initial seed is from a good random pool.
	randomread(seed, sizeof(seed));
	
	p = xp;
	for(e = p + n; p < e; ){
		s0 = seed[ x ];
		s1 = seed[ x = (x+1) & 15 ];
		s1 ^= s1 << 31;
		s1 ^= s1 >> 11;
		s0 ^= s0 >> 30;
		*p++=( seed[x] = s0 ^ s1 ) * 1181783497276652981LL;
	}
	poperror();
	return n;
}
