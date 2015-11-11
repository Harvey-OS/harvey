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

static QLock rl;


/*
 * Add entropy
 */
 void
 random_add(void *xp){
	Proc *up = externup();

	if(waserror()){
		qunlock(&rl);
		nexterror();
	}

	qlock(&rl);
	fortuna_add_entropy(xp, sizeof(xp));
	qunlock(&rl);

	poperror();
 }


/*
 *  consume random bytes
 */
uint32_t
randomread(void *xp, uint32_t n){
	Proc *up = externup();

	if(waserror()){
		qunlock(&rl);
		nexterror();
	}

	qlock(&rl);
	fortuna_get_bytes(n, xp);
	qunlock(&rl);

	poperror();

	return n;
}

/**
 * Fast random generator
 **/
uint32_t
urandomread(void *xp, uint32_t n){
	Proc *up = externup();
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
