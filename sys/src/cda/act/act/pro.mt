prologue {
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"

#define MPBLOCKSIZ 50000

typedef struct Cost {
	int gate;
	int pin;
} Cost;
Cost gatecost = {1,0};
Cost pincost = {0,1};
Cost zerocost = {0,0};
Cost infinity = {1000000,0};
Cost bigcost = {1000,0};

#define COST		Cost
#define INFINITY	infinity
#define ZEROCOST	gatecost	/* default cost of a rule */
#define ADDCOST(x,y)	(x.gate += y.gate, x.pin += y.pin)
#define COSTLESS(x,y)	((x.gate)<(y.gate) || (x.gate==y.gate && x.pin<y.pin))
};
node C0 C1 ID CK not and or xor dff lat inbuf outbuf bibuf tribuf buffer mux clkbuf;
label e ck;

ck:	CK
	{cost = pincost;}
	= {
		push($$);
	};

ck:	e
	{}
	= {
	};

e:	ID
	{cost = pincost;}
	= {
		push($$);
	};

e:	C0
	{cost = zerocost;}
	= {
		push($$);
	};

e:	C1
	{cost = zerocost;}
	= {
		push($$);
	};

e:	not(C0)
	{cost = zerocost;}
	= {
		push(ONE);
	};

e:	not(C1)
	{cost = zerocost;}
	= {
		push(ZERO);
	};

e:	not(not(e))
	{cost.gate -= 2; TOPDOWN;}
	= {
		tDO($%1$);
	};

/*
 * progagate negation through muxes
e:	mux(e,not(e),not(e))
	{cost.gate -= 1; REWRITE;}
	= {
		return nameit($$->id,notnode(muxnode($1$,$2.1$,$3.1$)));
	};
 */

/*
 * propagate negation through xors
 */
e:	xor(not(e),e)
	{REWRITE;}
	= {
		return nameit($$->id,notnode(xornode($1.1$,$2$)));
	};

e:	xor(e,not(e))
	{REWRITE;}
	= {
		return nameit($$->id,notnode(xornode($1$,$2.1$)));
	};

/*
 * constant folding, only needed for xors simulating tff's
 */
e:	xor(C0,e)
	{REWRITE;}
	= {
		return $2$;
	};

e:	xor(C1,e)
	{REWRITE;}
	= {
		return notnode($2$);
	};

/*
 * canon - mebbe good, mebbe bad
 */

e:	or(and(e,e),and(not(e),e))
	{
		if ($1.1$->op == not)
			ABORT;
		REWRITE;
	}
	= {
		return nameit($$->id,ornode(andnode(notnode($2.1.1$),$2.2$),andnode($1.1$,$1.2$)));
	};


