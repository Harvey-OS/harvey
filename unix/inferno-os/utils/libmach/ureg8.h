struct Ureg
{
	ulong	di;		/* general registers */
	ulong	si;		/* ... */
	ulong	bp;		/* ... */
	ulong	nsp;
	ulong	bx;		/* ... */
	ulong	dx;		/* ... */
	ulong	cx;		/* ... */
	ulong	ax;		/* ... */
	ulong	gs;		/* data segments */
	ulong	fs;		/* ... */
	ulong	es;		/* ... */
	ulong	ds;		/* ... */
	ulong	trap;		/* trap type */
	ulong	ecode;		/* error code (or zero) */
	ulong	pc;		/* pc */
	ulong	cs;		/* old context */
	ulong	flags;		/* old flags */
	union {
		ulong	usp;
		ulong	sp;
	} u0;
	ulong	ss;		/* old stack segment */
};
