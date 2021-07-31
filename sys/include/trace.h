typedef enum Tevent {
	SAdmit,		/* Edf admit */
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
	SUser,		/* user event */
	Nevent,
} Tevent;

typedef struct Traceevent	Traceevent;
struct Traceevent {
	ulong	pid;	
	Tevent	etype;		/* Event type */
	vlong	time;		/* time stamp (ns)  */
};
