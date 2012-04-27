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
	u32int	pid;	
	u32int	etype;	/* Event type */
	u64int	time;	/* time stamp  */
	u32int	core;	/* core number */
};
