/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <mp.h>
#include <libsec.h>

typedef struct State{
	QLock		lock;
	int		seeded;
	uint64_t		seed;
	DES3state	des3;
} State;
static State x917state;

static void
X917(uint8_t *rand, int nrand)
{
	int i, m, n8;
	uint64_t I, x;

	/* 1. Compute intermediate value I = Ek(time). */
	I = nsec();
	triple_block_cipher(x917state.des3.expanded, (uint8_t*)&I, 0); /* two-key EDE */

	/* 2. x[i] = Ek(I^seed);  seed = Ek(x[i]^I); */
	m = (nrand+7)/8;
	for(i=0; i<m; i++){
		x = I ^ x917state.seed;
		triple_block_cipher(x917state.des3.expanded, (uint8_t*)&x, 0);
		n8 = (nrand>8) ? 8 : nrand;
		memcpy(rand, (uint8_t*)&x, n8);
		rand += 8;
		nrand -= 8;
		x ^= I;
		triple_block_cipher(x917state.des3.expanded, (uint8_t*)&x, 0);
		x917state.seed = x;
	}
}

static void
X917init(void)
{
	int n;
	uint8_t mix[128];
	uint8_t key3[3][8];
	uint32_t *ulp;

	ulp = (uint32_t*)key3;
	for(n = 0; n < sizeof(key3)/sizeof(uint32_t); n++)
		ulp[n] = truerand();
	setupDES3state(&x917state.des3, key3, nil);
	X917(mix, sizeof mix);
	x917state.seeded = 1;
}

void
genrandom(uint8_t *p, int n)
{
	qlock(&x917state.lock);
	if(x917state.seeded == 0)
		X917init();
	X917(p, n);
	qunlock(&x917state.lock);
}
