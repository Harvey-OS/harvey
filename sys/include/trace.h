/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef enum Tevent {
	SAdmit = 0,	/* Edf admit */
	SRelease,	/* Edf release, waiting to be scheduled */
	SEdf,		/* running under EDF */
	SRun,		/* running best effort */
	SReady,		/* runnable but not running  */
	SSleep,		/* blocked */
	SYield,		/* blocked waiting for release */
	SSlice,		/* slice exhausted */
	SDeadline,	/* proc's deadline */
	SExpel,		/* Edf expel */
	SDead,		/* proc dies */
	SInts,		/* Interrupt start */
	SInte,		/* Interrupt end */
	SUser,		/* user event */
	SLock,		/* blocked on a queue or lock */
	Nevent,
} Tevent;

typedef struct Traceevent	Traceevent;
struct Traceevent {
	u32	pid;	
	u32	etype;	/* Event type */
	i64	time;	/* time stamp  */
	u32	core;	/* core number */
};
