/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	Maxsteps = 200 * 100 * 2,	/* 100 periods of 200 procs */

	/* Edf.flags field */
	Admitted		= 0x01,
	Sporadic		= 0x02,
	Yieldonblock		= 0x04,
	Sendnotes		= 0x08,
	Deadline		= 0x10,
	Yield			= 0x20,
	Extratime		= 0x40,

	Infinity = ~0ULL,
};

typedef struct Edf		Edf;

struct Edf {
	/* All times in µs */
	/* time intervals */
	long		D;		/* Deadline */
	long		Delta;		/* Inherited deadline */
	long		T;		/* period */
	long		C;		/* Cost */
	long		S;		/* Slice: time remaining in this period */
	/* times (only low-order bits of absolute time) */
	long		r;		/* (this) release time */
	long		d;		/* (this) deadline */
	long		t;		/* Start of next period, t += T at release */
	long		s;		/* Time at which this proc was last scheduled */
	/* for schedulability testing */
	long		testDelta;
	int		testtype;	/* Release or Deadline */
	long		testtime;
	Proc		*testnext;
	/* other */
	uint16_t		flags;
	Timer Timer;
	/* Stats */
	long		edfused;
	long		extraused;
	long		aged;
	uint32_t		periods;
	uint32_t		missed;
};

extern Lock	edftestlock;	/* for atomic admitting/expelling */

#pragma	varargck	type	"t"		long
#pragma	varargck	type	"U"		uint64_t

/* Interface: */
Edf*		edflock(Proc*);
void		edfunlock(void);


/* sched interface, used only by edf */
Sched*		procsched(Proc*);
