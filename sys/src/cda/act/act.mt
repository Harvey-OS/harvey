prologue {
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"

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
	{cost.gate -= 1; TOPDOWN;}
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


e:	inbuf(e)
	{TOPDOWN;}
	= {
		tDO($%1$); namepin("PAD", 0);
		func($$,"INBUF",1,"Y","PAD");
	};

e:	clkbuf(e)
	{TOPDOWN;}
	= {
		tDO($%1$); namepin("PAD", 0);
		func($$,"CLKBUF",1,"Y","PAD");
	};

e:	outbuf(e)
	{TOPDOWN;}
	= {
		tDO($%1$); namepin("D", 0);
		func($$,"OUTBUF",1,"PAD","D");
	};

e:	bibuf(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("E", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"BIBUF",2,"PADY","D","E");
	};

e:	tribuf(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("E", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"TRIBUFF",2,"PAD","D","E");
	};

e:	not(ck)
	{cost.gate += 1000; TOPDOWN;}
	= {
		push(ONE); namepin("A", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GNAND2",2,"Y","G","A");
	};

e:	and(e,ck)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"GAND2",2,"Y","A","G");
	};

e:	and(ck,e)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GAND2",2,"Y","G","A");
	};

e:	mux(not(e),not(ck),C1)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"GNAND2",2,"Y","A","G");
	};

e:	mux(not(ck),not(e),C1)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GNAND2",2,"Y","G","A");
	};

e:	or(e,ck)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"GOR2",2,"Y","A","G");
	};

e:	or(ck,e)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GOR2",2,"Y","G","A");
	};

e:	and(not(e),not(ck))
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"GNOR2",2,"Y","A","G");
	};

e:	and(not(ck),not(e))
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GNOR2",2,"Y","G","A");
	};

e:	xor(e,ck)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"GXOR2",2,"Y","A","G");
	};

e:	xor(ck,e)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GXOR2",2,"Y","G","A");
	};

e:	mux(ck,mux(e,e,e),mux(e,e,e))
	{
		cost.gate += 1000;
		if (!eq($2.1$,$3.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%7$); namepin("D3", 0);
		tDO($%6$); namepin("D2", 0);
		tDO($%4$); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0", 0);
		tDO($%1$); namepin("G", 0);
		func($$,"GMX4",6,"Y","G","S0","D0","D1","D2","D3");
	};

e:	mux(e,mux(ck,e,e),mux(ck,e,e))
	{
		cost.gate += 1000;
		if (!eq($2.1$,$3.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%7$); namepin("D3", 0);
		tDO($%6$); namepin("D1", 0);
		tDO($%4$); namepin("D2", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("S0", 0);
		func($$,"GMX4",6,"Y","S0","G","D0","D2","D1","D3");
	};

e:	mux(e,ck,e)
	{cost.gate += 1000; TOPDOWN;}
	= {
		tDO($%3$); namepin("D1", 0);
		push(ONE); namepin("D2", 0);
		tDO($%3$); namepin("D1", 0);
		push(ZERO); namepin("D0", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("S0", 0);
		func($$,"GMX4",6,"Y","S0","G","D0","D1","D2","D3");
	};

e:	mux(e,e,ck)
	{cost.gate += 1000; TOPDOWN;}
	= {
		push(ONE); namepin("D3", 0);
		tDO($%2$); namepin("D0", 0);
		push(ZERO); namepin("D1", 0);
		tDO($%2$); namepin("D0", 0);
		tDO($%1$); namepin("S0", 0);
		func($$,"GMX4",5,"Y","S0","D0","D1","D2","D3");
	};

e:	or(and(e,e),or(and(e,e),and(e,e)))
	{
		if (!eq($1.1$,$2.1.1$) ||
			!eq($1.2$,$2.2.1$) ||
			!eq($2.1.2$,$2.2.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%4$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"MAJ3",3,"Y","A","B","C");
	};

e:	or(and(e,e),or(and(e,e),and(e,e)))
	{
		if (!eq($1.1$,$2.2.2$) ||
			!eq($2.1.1$,$2.2.1$) ||
			!eq($1.2$,$2.1.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("B", 0);
		func($$,"MAJ3",3,"Y","B","C","A");
	};

e:	or(and(not(e),and(e,e)),and(e,and(e,e)))
	{
		if (!eq($1.1.1$,$2.1$) ||
			!eq($1.2.1$,$2.2.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%6$); namepin("D", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO4A",4,"Y","A","C","B","D");
	};

e:	or(and(not(e),e),or(and(e,e),e))
	{
		if (!eq($1.1.1$,$2.1.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%5$); namepin("D", 0);
		tDO($%4$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO5A",4,"Y","A","B","C","D");
	};

e:	or(and(e,e),and(e,mux(e,e,C1)))
	{
		if (!eq($1.1$,$2.2.2$) ||
			!eq($1.2$,$2.2.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"MAJ3",3,"Y","A","B","C");
	};

e:	or(and(e,e),and(e,mux(e,e,C1)))
	{
		if (!eq($1.1$,$2.2.1$) ||
			!eq($1.2$,$2.2.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"MAJ3",3,"Y","A","B","C");
	};

e:	lat(e,ck,C1,C0)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DL1",2,"Q","D","G");
	};

e:	lat(e,not(ck),C1,C0)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DL1B",2,"Q","D","G");
	};

e:	lat(e,ck,e,C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DLE",3,"Q","D","G","E");
	};

e:	lat(e,ck,not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DLEA",3,"Q","D","G","E");
	};

e:	lat(e,not(ck),e,C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DLEB",3,"Q","D","G","E");
	};

e:	lat(e,not(ck),not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DLEC",3,"Q","D","G","E");
	};

e:	lat(e,ck,C1,not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DLC",3,"Q","D","G","CLR");
	};

e:	lat(e,not(ck),C1,not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("G", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"DLCA",3,"Q","D","G","CLR");
	};

e:	lat(mux(e,e,e),ck,C1,C0)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("G", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DLM",4,"Q","S","A","B","G");
	};

e:	lat(mux(e,e,e),not(ck),C1,C0)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("G", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DLMA",4,"Q","S","A","B","G");
	};

e:	lat(mux(e,e,e),not(ck),not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("E", 0);
		tDO($%4$); namepin("G", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DLME1A",5,"Q","S","A","B","G","E");
	};

e:	dff(e,ck,C1,C0,C0)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DF1",2,"Q","D","CLK");
	};

e:	dff(not(e),ck,C1,C0,C0)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DF1A",2,"QN","D","CLK");
	};

e:	dff(e,not(ck),C1,C0,C0)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DF1B",2,"Q","D","CLK");
	};

e:	dff(not(e),not(ck),C1,C0,C0)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DF1C",2,"QN","D","CLK");
	};

e:	dff(e,ck,e,C0,C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFE",3,"Q","D","CLK","E");
	};

e:	dff(e,ck,e,C0,e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("PRE", 0);
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFE4",4,"Q","D","CLK","E","PRE");
	};

e:	dff(e,ck,e,not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("CLR", 0);
		tDO($%3$); namepin("E", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFE3A",4,"Q","D","CLK","E","CLR");
	};

e:	dff(e,ck,C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("PRE", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFP1",3,"Q","D","CLK","PRE");
	};

e:	dff(e,ck,C1,e,C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1",3,"Q","D","CLK","CLR");
	};

e:	dff(not(e),ck,C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1C",3,"QN","D","CLK","CLR");
	};

e:	dff(e,not(ck),C1,e,C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1A",3,"Q","D","CLK","CLR");
	};

e:	dff(not(e),not(ck),C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1F",3,"QN","D","CLK","CLR");
	};

e:	dff(e,ck,C1,not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1B",3,"Q","D","CLK","CLR");
	};

e:	dff(not(e),ck,C1,C0,not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1E",3,"QN","D","CLK","CLR");
	};

e:	dff(e,not(ck),C1,not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1D",3,"Q","D","CLK","CLR");
	};

e:	dff(not(e),not(ck),C1,C0,not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFC1G",3,"QN","D","CLK","CLR");
	};

e:	dff(e,ck,C1,not(e),e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("PRE", 0);
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFPC",4,"Q","D","CLK","CLR","PRE");
	};

e:	dff(e,not(ck),C1,not(e),e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("PRE", 0);
		tDO($%3$); namepin("CLR", 0);
		tDO($%2$); namepin("CLK", 1);
		tDO($%1$); namepin("D", 0);
		func($$,"DFPCA",4,"Q","D","CLK","CLR","PRE");
	};

e:	dff(and(not(e),e),ck,not(e),C0,C0)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("E", 0);
		tDO($%3$); namepin("CLK", 1);
		push(ZERO); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFME1A",5,"Q","S","A","B","CLK","E");
	};

e:	dff(and(e,e),ck,C1,e,C0)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("CLR", 0);
		tDO($%3$); namepin("CLK", 1);
		tDO($%2$); namepin("B", 0);
		push(ZERO); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM3",5,"Q","S","A","B","CLK","CLR");
	};

e:	dff(and(e,e),ck,C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("PRE", 0);
		tDO($%3$); namepin("CLK", 1);
		tDO($%2$); namepin("B", 0);
		push(ZERO); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM4",5,"Q","S","A","B","CLK","PRE");
	};

e:	dff(and(not(e),e),ck,C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("PRE", 0);
		tDO($%3$); namepin("CLK", 1);
		push(ZERO); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM4",5,"Q","S","A","B","CLK","PRE");
	};

e:	dff(or(e,e),ck,C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("PRE", 0);
		tDO($%3$); namepin("CLK", 1);
		push(ONE); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM4",5,"Q","S","A","B","CLK","PRE");
	};

e:	dff(mux(e,e,e),ck,C1,C0,e)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("PRE", 0);
		tDO($%4$); namepin("CLK", 1);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM4",5,"Q","S","A","B","CLK","PRE");
	};

e:	dff(mux(e,e,e),ck,C1,not(e),C0)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("CLR", 0);
		tDO($%4$); namepin("CLK", 1);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFMB",5,"Q","S","A","B","CLK","CLR");
	};

e:	dff(mux(e,e,e),ck,C1,e,C0)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("CLR", 0);
		tDO($%4$); namepin("CLK", 1);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM3",5,"Q","S","A","B","CLK","CLR");
	};

e:	dff(mux(e,e,e),ck,not(e),C0,C0)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("E", 0);
		tDO($%4$); namepin("CLK", 1);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFME1A",5,"Q","S","A","B","CLK","E");
	};

e:	dff(mux(e,e,e),ck,C1,C0,C0)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("CLK", 1);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFM",4,"Q","S","A","B","CLK");
	};

e:	not(e)
	{TOPDOWN;}
	= {
		tDO($%1$); namepin("A", 0);
		func($$,"INV",1,"Y","A");
	};

e:	buffer(e)
	{TOPDOWN;}
	= {
		tDO($%1$); namepin("A", 0);
		func($$,"BUF",1,"Y","A");
	};

e:	and(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND2",2,"Y","A","B");
	};

e:	and(not(e),e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND2A",2,"Y","A","B");
	};

e:	and(not(e),not(e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND2B",2,"Y","A","B");
	};

e:	or(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR2",2,"Y","A","B");
	};

e:	or(not(e),e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR2A",2,"Y","A","B");
	};

e:	or(not(e),not(e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR2B",2,"Y","A","B");
	};

e:	not(and(e,e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND2",2,"Y","A","B");
	};

e:	not(and(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND2A",2,"Y","A","B");
	};

e:	not(and(not(e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND2B",2,"Y","A","B");
	};

e:	not(or(e,e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR2",2,"Y","A","B");
	};

e:	not(or(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR2A",2,"Y","A","B");
	};

e:	not(or(not(e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR2B",2,"Y","A","B");
	};

e:	and(e,and(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3",3,"Y","A","B","C");
	};

e:	and(not(e),and(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3A",3,"Y","A","B","C");
	};

e:	and(not(e),and(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3B",3,"Y","A","B","C");
	};

e:	and(not(e),and(not(e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3C",3,"Y","A","B","C");
	};

e:	or(e,or(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR3",3,"Y","A","B","C");
	};

e:	or(not(e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR3A",3,"Y","A","B","C");
	};

e:	or(not(e),or(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR3B",3,"Y","A","B","C");
	};

e:	not(and(not(e),and(e,e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND3A",3,"Y","A","B","C");
	};

e:	not(and(not(e),and(not(e),e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND3B",3,"Y","A","B","C");
	};

e:	not(and(not(e),and(not(e),not(e))))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND3C",3,"Y","A","B","C");
	};

e:	not(or(e,or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR3",3,"Y","A","B","C");
	};

e:	not(or(not(e),or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR3A",3,"Y","A","B","C");
	};

e:	not(or(not(e),or(not(e),e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR3B",3,"Y","A","B","C");
	};

e:	not(or(not(e),or(not(e),not(e))))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR3C",3,"Y","A","B","C");
	};

e:	and(not(e),and(not(e),and(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND4B",4,"Y","A","B","C","D");
	};

e:	and(not(e),and(not(e),and(not(e),e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND4C",4,"Y","A","B","C","D");
	};

e:	or(e,or(e,or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR4",4,"Y","A","B","C","D");
	};

e:	or(not(e),or(e,or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OR4A",4,"Y","A","B","C","D");
	};

e:	not(and(not(e),and(not(e),and(not(e),e))))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND4C",4,"Y","A","B","C","D");
	};

e:	not(and(not(e),and(not(e),and(not(e),not(e)))))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NAND4D",4,"Y","A","B","C","D");
	};

e:	not(or(not(e),or(e,or(e,e))))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR4A",4,"Y","A","B","C","D");
	};

e:	not(or(not(e),or(not(e),or(e,e))))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"NOR4B",4,"Y","A","B","C","D");
	};

e:	xor(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XOR",2,"Y","A","B");
	};

e:	not(xor(e,e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XNOR",2,"Y","A","B");
	};

e:	or(xor(e,e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XO1",3,"Y","A","B","C");
	};

e:	or(not(xor(e,e)),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XO1A",3,"Y","A","B","C");
	};

e:	or(e,xor(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XO1",3,"Y","C","A","B");
	};

e:	or(e,not(xor(e,e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XO1A",3,"Y","C","A","B");
	};

e:	and(xor(e,e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XA1",3,"Y","A","B","C");
	};

e:	and(e,mux(e,e,not(e)))
	{
		if (!eq($2.2$,$2.3.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XA1",3,"Y","C","A","B");
	};

e:	and(not(xor(e,e)),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XA1A",3,"Y","A","B","C");
	};

e:	and(e,xor(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XA1",3,"Y","C","A","B");
	};

e:	and(e,not(xor(e,e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XA1A",3,"Y","C","A","B");
	};

e:	not(xor(and(not(e),e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AX1A",3,"Y","A","B","C");
	};

e:	xor(and(not(e),e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AX1",3,"Y","A","B","C");
	};

e:	xor(and(not(e),not(e)),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AX1B",3,"Y","A","B","C");
	};

e:	or(and(e,e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1",3,"Y","A","B","C");
	};

e:	or(and(not(e),e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1A",3,"Y","A","B","C");
	};

e:	or(and(e,e),not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1B",3,"Y","A","B","C");
	};

e:	or(and(not(e),e),not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1C",3,"Y","A","B","C");
	};

e:	not(or(and(not(e),e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AOI1A",3,"Y","A","B","C");
	};

e:	not(or(and(e,e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AOI1B",3,"Y","A","B","C");
	};

e:	or(and(e,e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO2",4,"Y","A","B","C","D");
	};

e:	or(and(not(e),e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO2A",4,"Y","A","B","C","D");
	};

e:	not(or(and(not(e),e),or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AOI2A",4,"Y","A","B","C","D");
	};

e:	not(or(and(not(e),e),or(not(e),e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AOI2B",4,"Y","A","B","C","D");
	};

e:	and(or(e,e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA1",3,"Y","A","B","C");
	};

e:	and(or(not(e),e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA1A",3,"Y","A","B","C");
	};

e:	and(or(e,e),not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA1B",3,"Y","A","B","C");
	};

e:	and(or(not(e),e),not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA1C",3,"Y","A","B","C");
	};

e:	and(or(e,e),and(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA3",4,"Y","A","B","C","D");
	};

e:	and(e,and(e,or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("D", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA3",4,"Y","C","D","A","B");
	};

e:	and(or(e,e),and(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA3A",4,"Y","A","B","C","D");
	};

e:	and(or(not(e),e),and(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA3B",4,"Y","A","B","C","D");
	};

e:	and(or(e,e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA2",4,"Y","A","B","C","D");
	};

e:	and(or(not(e),e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA2A",4,"Y","A","B","C","D");
	};

e:	and(e,or(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1",3,"Y","C","A","B");
	};

e:	and(e,or(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1A",3,"Y","C","A","B");
	};

e:	and(not(e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1B",3,"Y","C","A","B");
	};

e:	or(and(e,e),and(e,e))
	{
		if (!eq($1.2$,$2.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA1",3,"Y","A","C","B");
	};

e:	or(and(not(e),e),and(e,e))
	{
		if (!eq($1.2$,$2.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA1A",3,"Y","A","C","B");
	};

e:	or(and(not(e),e),and(not(e),e))
	{
		if (!eq($1.1.1$,$2.1.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1B",3,"Y","C","A","B");
	};

e:	mux(e,mux(e,e,e),e)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("D2", 0);
		tDO($%5$); namepin("D2", 0);
		tDO($%4$); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MX4",6,"Y","S1","S0","D0","D1","D2","D3");
	};

e:	mux(e,e,mux(e,e,e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("D0", 0);
		tDO($%5$); namepin("D3", 0);
		tDO($%4$); namepin("D2", 0);
		tDO($%3$); namepin("S0", 0);
		tDO($%2$); namepin("D0", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MX4",6,"Y","S1","D0","S0","D2","D3","D1");
	};

e:	mux(e,and(e,e),e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D2", 0);
		tDO($%4$); namepin("D2", 0);
		tDO($%3$); namepin("D1", 0);
		push(ZERO); namepin("D0", 0);
		tDO($%2$); namepin("S0", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MX4",6,"Y","S1","S0","D0","D1","D2","D3");
	};

e:	mux(e,and(e,e),and(e,e))
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("D3", 0);
		push(ZERO); namepin("D2", 0);
		tDO($%4$); namepin("S0B", 0);
		tDO($%3$); namepin("D1", 0);
		push(ZERO); namepin("D0", 0);
		tDO($%2$); namepin("S0A", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MXT",7,"Y","S1","S0A","D0","D1","S0B","D2","D3");
	};

e:	mux(e,and(not(e),e),and(e,e))
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("D3", 0);
		push(ZERO); namepin("D2", 0);
		tDO($%4$); namepin("S0B", 0);
		push(ZERO); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0A", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MXT",7,"Y","S1","S0A","D0","D1","S0B","D2","D3");
	};

e:	mux(e,and(e,e),and(not(e),e))
	{TOPDOWN;}
	= {
		push(ZERO); namepin("D3", 0);
		tDO($%5$); namepin("D2", 0);
		tDO($%4$); namepin("S0B", 0);
		tDO($%3$); namepin("D1", 0);
		push(ZERO); namepin("D0", 0);
		tDO($%2$); namepin("S0A", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MXT",7,"Y","S1","S0A","D0","D1","S0B","D2","D3");
	};

e:	mux(e,and(not(e),e),and(not(e),e))
	{TOPDOWN;}
	= {
		push(ZERO); namepin("D3", 0);
		tDO($%5$); namepin("D2", 0);
		tDO($%4$); namepin("S0B", 0);
		push(ZERO); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0A", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MXT",7,"Y","S1","S0A","D0","D1","S0B","D2","D3");
	};

e:	mux(e,e,e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2",3,"Y","S","A","B");
	};

e:	mux(not(e),e,e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2",3,"Y","S","B","A");
	};

e:	mux(e,not(e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2A",3,"Y","S","A","B");
	};

e:	mux(not(e),not(e),e)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2B",3,"Y","S","B","A");
	};

e:	mux(e,e,not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2B",3,"Y","S","A","B");
	};

e:	mux(e,e,not(e))
	{
		if (!eq($2$,$3.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XOR",2,"Y","A","B");
	};

e:	mux(not(e),e,not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2A",3,"Y","S","B","A");
	};

e:	not(mux(e,e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2C",3,"Y","S","A","B");
	};

e:	mux(e,not(e),not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2C",3,"Y","S","A","B");
	};

e:	not(mux(not(e),e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2C",3,"Y","S","B","A");
	};

e:	mux(not(e),not(e),not(e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2C",3,"Y","S","B","A");
	};

e:	mux(e,mux(e,e,e),mux(e,e,e))
	{TOPDOWN;}
	= {
		tDO($%7$); namepin("D3", 0);
		tDO($%6$); namepin("D2", 0);
		tDO($%5$); namepin("S0B", 0);
		tDO($%4$); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0A", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MXT",7,"Y","S1","S0A","D0","D1","S0B","D2","D3");
	};

e:	mux(e,mux(e,e,e),mux(e,e,e))
	{
		if (!eq($2.1$,$3.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%7$); namepin("D3", 0);
		tDO($%6$); namepin("D2", 0);
		tDO($%4$); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MX4",6,"Y","S1","S0","D0","D1","D2","D3");
	};

e:	mux(e,mux(e,e,not(e)),mux(e,not(e),e))
	{
		if (!eq($2.1$,$3.1$) ||
			!eq($2.2$,$3.3$) ||
			!eq($2.2$,$2.3.1$) ||
			!eq($3.2.1$,$3.3$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XOR3",3,"Y","A","B","C");
	};

e:	mux(mux(e,e,e),e,e)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("D", 0);
		tDO($%4$); namepin("C", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MXC1",5,"Y","S","A","B","C","D");
	};

e:	mux(e,mux(e,e,e),e)
	{TOPDOWN;}
	= {
		push(ONE); namepin("D3", 0);
		push(ZERO); namepin("D2", 0);
		tDO($%5$); namepin("S0B", 0);
		tDO($%4$); namepin("D1", 0);
		tDO($%3$); namepin("D0", 0);
		tDO($%2$); namepin("S0A", 0);
		tDO($%1$); namepin("S1", 0);
		func($$,"MXT",7,"Y","S1","S0A","D0","D1","S0B","D2","D3");
	};

e:	or(and(not(e),e),and(e,e))
	{
		if (!eq($1.1.1$,$2.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2",3,"Y","S","A","B");
	};

e:	or(and(not(e),e),and(e,e))
	{
		if (!eq($1.1.1$,$2.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2",3,"Y","S","A","B");
	};

e:	or(and(not(e),not(e)),and(e,e))
	{
		if (!eq($1.1.1$,$2.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2A",3,"Y","S","A","B");
	};

e:	or(and(not(e),e),and(not(e),e))
	{
		if (!eq($1.1.1$,$2.2$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"MX2B",3,"Y","S","A","B");
	};

e:	or(and(not(e),e),and(not(e),e))
	{
		if (!eq($1.1.1$,$2.2$) ||
			!eq($1.2$,$2.1.1$))
			ABORT;
		TOPDOWN;
	}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"XOR",2,"Y","A","B");
	};

e:	dff(mux(e,e,e),e,not(e),C0,C0)
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("E", 0);
		tDO($%4$); namepin("CLK", 1);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("S", 0);
		func($$,"DFME1A",5,"Q","S","A","B","CLK","E");
	};

e:	and(e,mux(e,e,e))
	{TOPDOWN;}
	= {
		push(ZERO); namepin("C", 0);
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("S", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"MXC1",5,"Y","D","S","A","B","C");
	};

e:	or(e,mux(e,e,e))
	{TOPDOWN;}
	= {
		push(ONE); namepin("D", 0);
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("S", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"MXC1",5,"Y","C","S","A","B","D");
	};

e:	and(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND2",2,"Y","A","B");
	};

e:	and(not(e),e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND2A",2,"Y","A","B");
	};

e:	and(not(e),not(e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND2B",2,"Y","A","B");
	};

e:	mux(e,e,C1)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("B", 0);
		func($$,"OR2",2,"Y","B","A");
	};

e:	mux(e,not(e),C1)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("B", 0);
		func($$,"OR2A",2,"Y","B","A");
	};

e:	mux(not(e),not(e),C1)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("B", 0);
		func($$,"OR2B",2,"Y","B","A");
	};

e:	and(e,and(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3",3,"Y","A","B","C");
	};

e:	and(not(e),and(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3A",3,"Y","A","B","C");
	};

e:	and(not(e),and(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3B",3,"Y","A","B","C");
	};

e:	and(not(e),and(not(e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND3C",3,"Y","A","B","C");
	};

e:	or(e,or(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OR3",3,"Y","C","A","B");
	};

e:	or(e,or(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OR3A",3,"Y","C","A","B");
	};

e:	or(e,or(not(e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OR3B",3,"Y","C","A","B");
	};

e:	and(and(not(e),and(not(e),e)),e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND4B",4,"Y","A","B","C","D");
	};

e:	and(and(not(e),and(not(e),not(e))),e)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AND4C",4,"Y","A","B","C","D");
	};

e:	or(e,or(e,or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("C", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"OR4",4,"Y","D","A","B","C");
	};

e:	or(e,or(not(e),or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("C", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"OR4A",4,"Y","D","A","B","C");
	};

e:	xor(e,e)
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("B", 0);
		func($$,"XOR",2,"Y","B","A");
	};

e:	xor(e,not(e))
	{TOPDOWN;}
	= {
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("B", 0);
		func($$,"XNOR",2,"Y","B","A");
	};

e:	or(e,xor(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XO1",3,"Y","C","B","A");
	};

e:	or(e,xor(e,not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XO1A",3,"Y","C","B","A");
	};

e:	and(e,xor(e,e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XA1",3,"Y","C","B","A");
	};

e:	and(e,xor(e,not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"XA1A",3,"Y","C","B","A");
	};

e:	xor(e,and(not(e),e))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"AX1",3,"Y","C","A","B");
	};

e:	xor(e,mux(e,not(e),C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"AX1A",3,"Y","C","A","B");
	};

e:	xor(e,and(not(e),not(e)))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"AX1B",3,"Y","C","A","B");
	};

e:	mux(and(e,e),e,C1)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1",3,"Y","A","B","C");
	};

e:	mux(and(not(e),e),e,C1)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1A",3,"Y","A","B","C");
	};

e:	mux(and(e,e),not(e),C1)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1B",3,"Y","A","B","C");
	};

e:	mux(and(not(e),e),not(e),C1)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO1C",3,"Y","A","B","C");
	};

e:	and(not(e),mux(not(e),e,C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"AOI1A",3,"Y","C","B","A");
	};

e:	and(e,mux(not(e),not(e),C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"AOI1B",3,"Y","C","A","B");
	};

e:	or(and(e,e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("C", 0);
		tDO($%3$); namepin("D", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO2",4,"Y","A","B","D","C");
	};

e:	or(and(not(e),e),or(e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("C", 0);
		tDO($%3$); namepin("D", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO2A",4,"Y","A","B","D","C");
	};

e:	and(not(e),and(not(e),mux(not(e),e,C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("A", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"AOI2A",4,"Y","D","C","B","A");
	};

e:	and(not(e),and(e,mux(e,not(e),C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"AOI2B",4,"Y","D","C","A","B");
	};

e:	mux(and(not(e),and(e,e)),e,C1)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AO3",4,"Y","A","B","C","D");
	};

e:	or(and(e,e),and(e,mux(e,e,C1)))
	{TOPDOWN;}
	= {
		tDO($%5$); namepin("B", 0);
		tDO($%4$); namepin("A", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"MAJ3",5,"Y","A","B","C","A","B");
	};

e:	and(e,mux(e,e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"AO4A",4,"Y","C","A","B","D");
	};

e:	or(e,mux(e,e,e))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("C", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"AO5A",4,"Y","D","A","B","C");
	};

e:	or(e,and(e,mux(e,e,C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("D", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"AOI3A",4,"Y","A","D","C","B");
	};

e:	and(e,mux(e,e,C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1",3,"Y","C","A","B");
	};

e:	and(e,mux(not(e),e,C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("A", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1A",3,"Y","C","A","B");
	};

e:	and(not(e),mux(e,e,C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1B",3,"Y","C","B","A");
	};

e:	and(not(e),mux(e,not(e),C1))
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA1C",3,"Y","C","B","A");
	};

e:	and(e,and(e,mux(e,e,C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("A", 0);
		tDO($%3$); namepin("B", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"OA3",4,"Y","D","C","B","A");
	};

e:	and(e,and(not(e),mux(e,e,C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"OA3A",4,"Y","D","C","A","B");
	};

e:	and(e,and(not(e),mux(not(e),e,C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"OA3B",4,"Y","D","C","A","B");
	};

e:	or(and(e,mux(e,e,C1)),and(e,mux(e,e,C1)))
	{TOPDOWN;}
	= {
		tDO($%6$); namepin("A", 0);
		tDO($%5$); namepin("B", 0);
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA2",6,"Y","C","B","A","D","B","A");
	};

e:	or(and(e,mux(e,not(e),C1)),and(e,mux(e,not(e),C1)))
	{TOPDOWN;}
	= {
		tDO($%6$); namepin("A", 0);
		tDO($%5$); namepin("B", 0);
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("C", 0);
		func($$,"OA2A",6,"Y","C","B","A","D","B","A");
	};

e:	and(e,or(not(e),or(e,e)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("A", 0);
		tDO($%2$); namepin("C", 0);
		tDO($%1$); namepin("D", 0);
		func($$,"OA4A",4,"Y","D","C","A","B");
	};

e:	or(e,and(e,mux(e,e,C1)))
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("B", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("D", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OA5",4,"Y","A","D","C","B");
	};

e:	mux(and(not(e),not(e)),not(e),C1)
	{TOPDOWN;}
	= {
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OAI1",3,"Y","A","B","C");
	};

e:	mux(and(not(e),and(not(e),not(e))),e,C1)
	{TOPDOWN;}
	= {
		tDO($%4$); namepin("D", 0);
		tDO($%3$); namepin("C", 0);
		tDO($%2$); namepin("B", 0);
		tDO($%1$); namepin("A", 0);
		func($$,"OAI2A",4,"Y","A","B","C","D");
	};

