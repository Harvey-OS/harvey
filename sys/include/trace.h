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
	ulong	pid;	
	ulong	etype;	/* Event type */
	uvlong	time;	/* time stamp  */
	ulong	core;	/* core number */
};
