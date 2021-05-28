typedef enum PTevent {
	SAdmit = 0,	/* Edf admit */
	SRelease,	/* Edf release, waiting to be scheduled */
	SEdf,		/* running under EDF */
	SRun,		/* running best effort */

	SReady,		/* runnable but not running  */
	SSleep,		/* blocked; arg is PSstate| pc<<8  */
	SYield,		/* blocked waiting for release */
	SSlice,		/* slice exhausted */

	SDeadline,	/* proc's deadline */
	SExpel,		/* Edf expel */
	SDead,		/* proc dies */
	SInts,		/* Interrupt start */

	SInte,		/* Interrupt end */
	STrap,		/* fault */
	SUser,		/* user event */
	SName,		/* used to report names for pids */
	Nevent,
} Tevent;

enum {
	PTsize = 4 + 4 + 4 + 8 + 8,

	/* STrap arg flags */
	STrapRPF = 0x1000000000000000ULL,	/* page fault (read) STrap arg */
	STrapWPF = 0x1000000000000000ULL,	/* page fault (write) STrap arg */
	STrapSC  = 0x2000000000000000ULL,	/* sys call STrap arg */
	STrapMask = 0x0FFFFFFFFFFFFFFFULL,	/* bits available in arg */

	/* Sleep states; keep in sync with the kernel schedstate
	 * BUG: generate automatically.
	 */
	PSDead = 0,		/* not used */
	PSMoribund,		/* not used */
	PSReady,		/* not used */
	PSScheding,		/* not used */
	PSRunning,		/* not used */
	PSQueueing,
	PSQueueingR,
	PSQueueingW,
	PSWakeme,
	PSBroken,		/* not used */
	PSStopped,		/* not used */
	PSRendezvous,
	PSWaitrelease,

};

typedef struct PTraceevent	PTraceevent;
struct PTraceevent {
	u32int	pid;	/* for the process */
	u32int	etype;	/* Event type */
	u32int	machno;	/* where the event happen */
	vlong	time;	/* time stamp  */
	u64int	arg;	/* for this event type */
};
