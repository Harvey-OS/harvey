/* Runtime data structures */

enum
{
	Ptab		= 0x0ffdf000	/* Private stack */
};

typedef aggr Tdb;
typedef aggr Task;

void	*ALEF_getrp(void);
void	ALEF_linktask(void);
void	*ALEF_switch(Task*, Task*, void*);
void	ALEF_linksp(int*, char *sp, void(*)(void));
void	ALEF_sched(Task*);

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
	union {
		char	data[1];
		int	i;
		sint	s;
		float	f;
	};
};

aggr Chan
{
	Lock;

	int	async;

	Msgbuf	*free;
	Msgbuf	*qh;
	Msgbuf	*qt;
	Rendez	br;

	Chan	*sellink;
	Task	*selt;
	int	selp;

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

	char	*stack;
};

aggr Tdb
{
	Lock;

	int	ntask;		/* Number of task in this proc */
	Task	*ctask;		/* Current task */
	Task	*runhd;		/* Head of tasks ready to run */
	Task	*runtl;		/* Tail of tasks ready */
};

aggr Proc
{
	Tdb	*tdb;
};
#define	PROC		(*((Proc*)Ptab))
