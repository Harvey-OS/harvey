/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "a.h"

/*
 * 17.  Environment switching.
 */
typedef struct Env Env;
struct Env
{
	int s;
	int s0;
	int f;
	int f0;
	int fi;
	int ad;
	int ce;
	int v;
	int v0;
	int ls;
	int ls0;
	int it;
	/* - ta */
	/* - tc */
	/* - lc */
	/* - ul */
	/* - cu */
	/* - cc */
	/* - c2 */
	/* - nh */
	/* - hy */
	/* - hc */
	/* - lt */
	/* - nm */
	/* - nn */
	/* - mc */
};

Env defenv =
{
	10,
	10,
	1,
	1,
	1,
	1,
	0,
	12,
	12,
	0,
	0,
	0,
};

Env env[3];
Env *evstack[20];
int nevstack;

void
saveenv(Env *e)
{
	e->s = getnr(L(".s"));
	e->s0 = getnr(L(".s0"));
	e->f = getnr(L(".f"));
	e->f0 = getnr(L(".f0"));
	e->fi = getnr(L(".fi"));
	e->ad = getnr(L(".ad"));
	e->ce = getnr(L(".ce"));
	e->v = getnr(L(".v"));
	e->v0 = getnr(L(".v0"));
	e->ls = getnr(L(".ls"));
	e->ls0 = getnr(L(".ls0"));
	e->it = getnr(L(".it"));
}

void
restoreenv(Env *e)
{
	nr(L(".s"), e->s);
	nr(L(".s0"), e->s0);
	nr(L(".f"), e->f);
	nr(L(".f0"), e->f0);
	nr(L(".fi"), e->fi);
	nr(L(".ad"), e->ad);
	nr(L(".ce"), e->ce);
	nr(L(".v"), e->v);
	nr(L(".v0"), e->v0);
	nr(L(".ls"), e->ls);
	nr(L(".ls0"), e->ls0);
	nr(L(".it"), e->it);

	nr(L(".ev"), e-env);
	runmacro1(L("font"));
}


void
r_ev(int argc, Rune **argv)
{
	int i;
	Env *e;
	
	if(argc == 1){
		if(nevstack <= 0){
			if(verbose) warn(".ev stack underflow");
			return;
		}
		restoreenv(evstack[--nevstack]);
		return;
	}
	if(nevstack >= nelem(evstack))
		sysfatal(".ev stack overflow");
	i = eval(argv[1]);
	if(i < 0 || i > 2){
		warn(".ev bad environment %d", i);
		i = 0;
	}
	e = &env[getnr(L(".ev"))];
	saveenv(e);
	evstack[nevstack++] = e;
	restoreenv(&env[i]);
}

void
t17init(void)
{
	int i;
	
	for(i=0; i<nelem(env); i++)
		env[i] = defenv;

	addreq(L("ev"), r_ev, -1);
}
