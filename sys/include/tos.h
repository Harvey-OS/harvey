typedef struct Plink Plink;
typedef struct Tos Tos;

#pragma incomplete Plink


struct Tos
{
	struct			/* Per process profiling */
	{
		Plink	*pp;	/* known to be 0(ptr) */
		Plink	*next;	/* known to be 4(ptr) */
		Plink	*last;
		Plink	*first;
		ulong	pid;
		ulong	what;
	}	prof;
	uvlong	cyclefreq;	/* cycle clock frequency if there is one, 0 otherwise */
	vlong	kcycles;	/* cycles spent in kernel */
	vlong	pcycles;	/* cycles spent in process (kernel + user) */
	ulong	clock;

	/*
	 * Fields below are not available on Plan 9 kernels.
	 */
	int	nixtype;		/* role of the core we are running at */
	int	core;		/* core we are running at */

	/* Used as TLS data in Go.*/
	void *Go_g;	/* goroutines */
	void *Go_m;	/* go threads */
	uvlong pid;
	/* top of stack is here */
};

extern Tos *_tos;
