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

	Infinity = 0xffffffffffffffffULL,

};

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

typedef enum Edfstate		Edfstate;
typedef struct Edf			Edf;
typedef struct Head			Head;
typedef struct List			List;
typedef struct Resource		Resource;
typedef struct Task			Task;
typedef struct Taskq			Taskq;
typedef struct CSN			CSN;
typedef struct TaskLink		TaskLink;

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
	Head	csns;			/* List of resources */
	CSN		*curcsn;		/* Position in CSN tree or nil */
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

struct CSN {
	List;					/* links and identifies the resource (must be first) */
	Task			*t;		/* task the CSN belongs to */
	Ticks		C;		/* cost */
	int			R;		/* read-only access (as opposed to exclusive access) */
	Ticks		Delta;	/* of the Tasks critical section */
	Ticks		testDelta;
	Ticks		S;		/* Remaining slice */
	CSN*		p;		/* parent resource items */
};

struct TaskLink {
	List;				/* links and identifies the task (must be first) */
	Ticks		C;	/* cost */
	int			R;	/* read-only access (as opposed to exclusive access) */
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
char *	parsetime(Time*, char*);
void *	findlist(Head*, void*);
Task *	findtask(int);
List *		onlist(Head*, void*);
int		timeconv(Fmt*);
void		resourcefree(Resource*);
Resource*	resource(char*, int);
void		removetask(Task*);
void		taskfree(Task*);
char *	parseresource(Head*, CSN*, char*);
char *	seprintresources(char*, char*);
char *	seprintcsn(char*, char*, Head*);
void		resourcetimes(Task*, Head*);
char*	dumpq(char*, char*, Taskq*, Ticks);
char*	seprinttask(char*, char*, Task*, Ticks);
char*	dumpq(char*, char*, Taskq*, Ticks);

#define	DEBUG	if(1){}else iprint
