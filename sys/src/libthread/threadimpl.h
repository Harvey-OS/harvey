#include "thread.h"

typedef enum {
	Running,
	Runnable,
	Rendezvous,
} State;
	
typedef enum {
	Callnil,
	Callalt,
	Callsnd,
	Callrcv,
} Callstate;

enum {
	DOEXEC = 1,
	DOEXIT = 2,
	DOPROC = 3,
};

struct Tqueue {		// Thread queue
	Lock	lock;
	Thread	*head;
	Thread	*tail;
};

struct Thread {
	Lock		lock;		// protects thread data structure
	int			id;		// thread id
	int 		grp;		// thread group
	State		state;		// state of thread
	int			exiting;	// should it die?
	Callstate	call;		// which `system call' is current
	char		*cmdname;	// ptr to name of thread
	Thread		*next;		// next on queue (run, rendezvous)
	Thread		*nextt;		// next on list of all theads
	Proc		*proc;		// proc of this thread
	ulong		tag;		// rendez-vous tag
	Alt			*alt;		// pointer to alt structure
	ulong		value;		// rendez-vous value
	Thread		*garbage;	// ptr to thread to clean up
	jmp_buf		env;		// jump buf for launching or switching threads
	uchar		*stk;		// top of stack (lowest address of stack)
	uint		stksize;	// stack size

	ulong	udata;			// User per-thread data pointer
};

struct Proc {
	Lock	lock;
	int	pid;

	jmp_buf	oldenv;			// jump buf for returning to original stack

	int	nthreads;
	Tqueue	threads;		// All threads of this proc
	Tqueue	runnable;		// Runnable threads
	Thread	*curthread;		// Running thread

	int	blocked;		// In a rendezvous
	uint	nextID;			// ID of most recently created thread
	Proc	*next;			// linked list of Procs

	void	*arg;			// passed between shared and unshared stk
	char	str[ERRLEN];	// used by threadexits to avoid malloc

	ulong	udata;			// User per-proc data pointer
};

typedef struct Newproc {
	uchar	*stack;
	uint	stacksize;
	ulong	*stackptr;
	ulong	launcher;
	int		grp;
	int		rforkflag;
} Newproc;

typedef struct Execproc {
	Proc	*procp;
	char	*file;
	char	**arg;
	char	data[4096];
} Execproc;

struct Pqueue {		// Proc queue
	Lock	lock;
	Proc	*head;
	Proc	*tail;
};

extern struct Pqueue pq;	// Linked list of procs
extern Proc **procp;		// Pointer to pointer to proc's Proc structure

int		tas(int*);
int		inc(int*, int);
int		cas(Lock *lock, Lock old, Lock new);
ulong	_threadrendezvous(ulong, ulong);
void	*_threadmalloc(long size);
void	_xinc(long*);
long	_xdec(long*);
