/* Runtime data structures */

enum
{
	Ptab		= 0x0ffdf000,	/* Private stack */
	Execstk		= 0x0fbe1000,	/* Exec stack linkage area */
};

/*
 * Passed from the loader to extend stack root by max become
 * frame size
 */
extern byte ALEFbecome[];
#define	MAXBECOME	(uint)ALEFbecome

typedef aggr Tdb;
typedef aggr Task;

void	*ALEF_getrp(void);
void	ALEF_linktask(void);
void	*ALEF_switch(Task*, Task*, void*);
void	ALEF_linksp(int*, byte*, void(*)(void));
void	ALEF_sched(Task*);
void*	ALEFalloc(uint, int);

/*
 * The loader rearranges stack offset to ensure double alignment off SP for
 * automatics and doubles. This allows FxxxD instructions to be used.
 */
#define ALIGN(s, n)	((uint)(s)&~(n-1))
#define ALIGN_UP(s, n)	ALIGN(((s)+(n)-1), n)

#define	SP_DELTA	0		/* fixup to SP in task startup */
#define	PC_DELTA	8		/* CALL saves PC, so return adds 8 */
#define	ARGV_DELTA	4		/* fixup of SP in init */

adt Rendez
{
	Lock;
extern	Task	*t;

	void	Sleep(*Rendez, void**, int);
	void	Wakeup(*Rendez);
};

aggr Msgbuf
{
	Msgbuf	*next;
	uint	signature;
	union {
		byte	data[1];
		int	i;
		sint	s;
		float	f;
	};
};

aggr Chan
{
	Lock;

	int	async;
	uint	signature;

	Msgbuf	*free;
	Msgbuf	*qh;
	Msgbuf	*qt;
	Rendez	br;

	Chan	*sellink;
	Task	*selt;
	int	selp;
	int	seltst;

	Rendez	sndr;
	void	*sva;
	QLock	snd;

	Rendez	rcvr;
	void	*rva;
	QLock	rcv;
};

aggr Task
{
	uint	sp;		/* pc, sp - Known by ALEF_switch(alefasm.s) */
	uint	pc;

	Tdb	*tdb;
	Task	*link;

	Task	*qlink;

	Chan	*slist;
	Chan	*stail;

	Rendez;

	byte	*stack;
};

aggr Tdb
{
	Lock;

	int	ntask;		/* Number of task in this proc */
	Task	*ctask;		/* Current task */
	Task	*runhd;		/* Head of tasks ready to run */
	Task	*runtl;		/* Tail of tasks ready */
	int	sleeper;	/* true if sched() needs to rendezvous with a sleeper */
};

aggr Proc
{
	Tdb	*tdb;
};
#define	PROC		(*((Proc*)Ptab))
