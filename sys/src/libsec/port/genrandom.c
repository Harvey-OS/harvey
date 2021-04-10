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
	u64		seed;
	DES3state	des3;
} State;
static State x917state;

static void
X917(u8 *rand, int nrand)
{
	int i, m, n8;
	u64 I, x;

	/* 1. Compute intermediate value I = Ek(time). */
	I = nsec();
	triple_block_cipher(x917state.des3.expanded, (u8*)&I, 0); /* two-key EDE */

	/* 2. x[i] = Ek(I^seed);  seed = Ek(x[i]^I); */
	m = (nrand+7)/8;
	for(i=0; i<m; i++){
		x = I ^ x917state.seed;
		triple_block_cipher(x917state.des3.expanded, (u8*)&x, 0);
		n8 = (nrand>8) ? 8 : nrand;
		memcpy(rand, (u8*)&x, n8);
		rand += 8;
		nrand -= 8;
		x ^= I;
		triple_block_cipher(x917state.des3.expanded, (u8*)&x, 0);
		x917state.seed = x;
	}
}

static void
X917init(void)
{
	int n;
	u8 mix[128];
	u8 key3[3][8];
	u32 *ulp;

	ulp = (u32*)key3;
	for(n = 0; n < sizeof(key3)/sizeof(u32); n++)
		ulp[n] = truerand();
	setupDES3state(&x917state.des3, key3, nil);
	X917(mix, sizeof mix);
	x917state.seeded = 1;
}

void
genrandom(u8 *p, int n)
{
	qlock(&x917state.lock);
	if(x917state.seeded == 0)
		X917init();
	X917(p, n);
	qunlock(&x917state.lock);
}
