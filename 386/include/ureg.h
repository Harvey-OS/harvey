struct Ureg
{
	/* order is that of PUSHAL instruction ... */
	ulong	di;		/* general registers */
	ulong	si;		/* ... */
	ulong	bp;		/* ... */
	ulong	nsp;
	ulong	bx;		/* ... */
	ulong	dx;		/* ... */
	ulong	cx;		/* ... */
	ulong	ax;		/* ... */
	/* ... to here */
	ulong	gs;		/* data segments */
	ulong	fs;		/* ... */
	ulong	es;		/* ... */
	ulong	ds;		/* ... */
	ulong	trap;		/* trap type */
	/* below are pushed by trap/intr mechanism */
	ulong	ecode;		/* error code (or zero) */
	ulong	pc;		/* pc */
	ulong	cs;		/* old context */
	ulong	flags;		/* old flags */
	/* below are pushed only on trap/intr to more-privileged code */
	union {
		ulong	usp;
		ulong	sp;
	};
	ulong	ss;		/* old stack segment */
};
