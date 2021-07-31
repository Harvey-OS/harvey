enum {
	Maxsteps = 20 * 2 * 100,	/* 100 periods of 20 procs */

	/* Edf.flags field */
	Admitted		= 0x01,
	Sporadic		= 0x02,
	Yieldonblock	= 0x04,
	Sendnotes		= 0x08,
	Deadline		= 0x10,
	Yield			= 0x20,

	Infinity = ~0ULL,
};

typedef struct Edf		Edf;

struct Edf {
	/* time intervals */
	vlong		D;			/* Deadline */
	vlong		Delta;		/* Inherited deadline */
	vlong		T;			/* period */
	vlong		C;			/* Cost */
	vlong		S;			/* Slice: time remaining in this period */
	/* times */
	vlong		r;			/* (this) release time */
	vlong		d;			/* (this) deadline */
	vlong		t;			/* Start of next period, t += T at release */
	vlong		s;			/* Time at which this proc was last scheduled */
	/* for schedulability testing */
	vlong		testDelta;
	int			testtype;	/* Release or Deadline */
	vlong		testtime;
	Proc		*testnext;
	/* other */
	ushort		flags;
	/* Stats */
	vlong		edfused;
	vlong		extraused;
	vlong		aged;
	ulong		periods;
	ulong		missed;
};

extern Lock	edftestlock;	/* for atomic admitting/expelling */

#pragma	varargck	type	"t"		vlong
#pragma	varargck	type	"U"		uvlong

/* Interface: */
void		edflock(void);
void		edfunlock(void);
