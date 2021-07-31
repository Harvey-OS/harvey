typedef vlong				Time;
typedef uvlong				Ticks;
typedef struct Schedevent	Schedevent;

enum {
	Onemicrosecond =	1000ULL,
	Onemillisecond =	1000000ULL,
	Onesecond =		1000000000ULL,
};

typedef enum SEvent {
	SAdmit,		/* new proc arrives*/
	SRelease,		/* released, but not yet scheduled (on qreleased) */
	SRun,		/* one of this task's procs started running */
	SPreempt,		/* the running proc was preempted */
	SBlock,		/* none of the procs are runnable as a result of sleeping */
	SResume,		/* one or more procs became runnable */
	SDeadline,	/* proc's deadline */
	SYield,		/* proc reached voluntary early deadline */
	SSlice,		/* slice exhausted */
	SExpel,		/* proc is gone */
	SResacq,		/* acquire resource */
	SResrel,		/* release resource */
} SEvent;

struct Schedevent {
	ushort	tid;		/* Task ID */
	SEvent	etype;	/* Event type */
	Time		ts;		/* Event time */
};
