enum {
	Nproc = 8,
	Nres = 8,
	Ntask = 8,
	Maxtasks = 20,
	Maxresources = 20,
	Maxsteps = Maxtasks * 2 * 100,	/* 100 periods of maximum # of tasks */

	/* Edf.flags field */
	Verbose = 0x1,
	Useblocking = 0x2,
};

typedef vlong				Time;
typedef uvlong				Ticks;

typedef struct Task			Task;
typedef struct Resource		Resource;
typedef struct ResourceItem	ResourceItem;
typedef struct Edf			Edf;
typedef struct Taskq			Taskq;
typedef struct List			List;
typedef struct Head			Head;

enum Edfstate {
	EdfUnused,		/* task structure not in use */
	EdfExpelled,		/* in initialization, not yet admitted */
	EdfAdmitted,		/* admitted, but not started */

	EdfIdle,			/* admitted, but no member processes */
	EdfAwaitrelease,	/* released, but too early (on qwaitrelease) */
	EdfReleased,		/* released, but not yet scheduled (on qreleased) */
	EdfRunning,		/* one of this task's procs is running (on stack) */
	EdfExtra,			/* one of this task's procs is running in extra time (off stack) */
	EdfPreempted,		/* the running proc was preempted */
	EdfBlocked,		/* none of the procs are runnable as a result of sleeping */
	EdfDeadline,		/* none of the procs are runnable as a result of scheduling */
};
typedef enum Edfstate	Edfstate;

struct List {
	List	*	next;		/* next in list */
	void	*	i;		/* item in list */
};

struct Head {
	List	*next;		/* First item in list */
	int	n;			/* number of items in list */
};

struct Edf {
	/* time intervals */
	Ticks	D;		/* Deadline */
	Ticks	Delta;	/* Inherited deadline */
	Ticks	T;		/* period */
	Ticks	C;		/* Cost */
	Ticks	S;		/* Slice: time remaining in this period */
	/* times */
	Ticks	r;		/* (this) release time */
	Ticks	d;		/* (this) deadline */
	Ticks	t;		/* Start of next period, t += T at release */
	/* for schedulability testing */
	Ticks	testDelta;
	int		testtype;	/* Release or Deadline */
	Ticks	testtime;
	Task	*	testnext;
	/* statistics gathering */
	ulong	periods;	/* number of periods */
	ulong	missed;	/* number of deadlines missed */
	ulong	preemptions;
	Ticks	total;		/* total time used */
	Ticks	aged;	/* aged time used */
	/* other */
	Edfstate	state;
};

struct Task {
	QLock;
	Ref;					/* ref count for farbage collection */
	int		taskno;		/* task number in Qid  */
	Edf;
	Ticks	scheduled;
	Schedq	runq;		/* Queue of runnable member procs */
	Head	procs;		/* List of member procs */
	Head	res;			/* List of resources */
	char		*user;		/* mallocated */
	Dirtab	dir;
	int		flags;		/* e.g., Verbose */
	Task		*rnext;
};

struct Taskq
{
	Lock;
	Task*	head;
	int		(*before)(Task*, Task*);	/* ordering function for queue (nil: fifo) */
};

struct Resource
{
	Ref;
	char	*	name;
	Head	tasks;
	Ticks	Delta;
	/* for schedulability testing */
	Ticks	testDelta;
};

struct ResourceItem {
	List;			/* links and identifies the resource (must be first) */
	Ticks	C;	/* cost */
	int		x;	/* exclusive access (as opposed to shared-read access) */
	Head	h;	/* sub resource items */
};

extern QLock		edfschedlock;
extern Head		tasks;
extern Head		resources;
extern int			nresources;
extern Lock		edflock;
extern Taskq		qwaitrelease;
extern Taskq		qreleased;
extern Taskq		qextratime;
extern Taskq		edfstack[];
extern int			edfstateupdate;
extern void		(*devrt)(Task *, Ticks, int);
extern char *		edfstatename[];

#pragma	varargck	type	"T"		Time
#pragma	varargck	type	"U"		Ticks

Time		ticks2time(Ticks);
Ticks	time2ticks(Time);
int		putlist(Head*, List*);
int		enlist(Head*, void*);
int		delist(Head*, void*);
