typedef struct Ureg	Ureg;

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
	ulong	es;		/* data segment */
	ulong	ds;		/* data segment */
	ulong	trap;		/* trap type */
	ulong	ecode;		/* error code (or zero) */
	ulong	pc;		/* pc */
	ulong	cs;		/* old context */
	ulong	flags;		/* old flags */
	union {
		ulong	usp;
		ulong	sp;
	};
	ulong	ss;		/* old stack segment */
};
