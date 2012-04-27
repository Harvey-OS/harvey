typedef struct Callq Callq;
typedef struct Nixcall Nixcall;
typedef struct Nixret Nixret;
typedef struct Plink Plink;
typedef struct Tos Tos;

#pragma incomplete Plink

#define CALLQSZ	512

struct Nixcall
{
	void*	tag;
	int	scall;
	va_list	sarg;
};

struct Nixret
{
	void*	tag;
	int	sret;
	char*	err;
};

struct Callq
{
	int	ksleep;
	unsigned int	qr;	/* don't use uint for ape */
	unsigned int	qw;
	unsigned int	rr;
	unsigned int	rw;
	Nixcall q[CALLQSZ];
	Nixret	r[CALLQSZ];
};

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
	ulong	pid;		/* might as well put the pid here */
	ulong	clock;

	int	nixtype;		/* role of the core we are running at */
	int	core;		/* core we are running at */
	Callq	callq;		/* NIX queue based system calls */
	/* top of stack is here */
};

extern Tos *_tos;
