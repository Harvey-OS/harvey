#pragma src "/sys/src/libthread"
#pragma lib "libthread.a"

#pragma	varargck	argpos	threadprint	2

typedef struct Proc	Proc;
typedef struct Thread	Thread;
typedef struct Tqueue	Tqueue;
typedef struct Alt	Alt;
typedef struct Channel	Channel;
typedef struct Ref	Ref;

/* Channel structure.  S is the size of the buffer.  For unbuffered channels
 * s is zero.  v is an array of s values.  If s is zero, v is unused.
 * f and n represent the state of the queue pointed to by v.
 * rcvrs and sndrs must be initialized to nil and should not be touched
 * by code outside channel.c
 */

struct Channel {
	int		s;			// Size of the channel (may be zero)
	uint	f;			// Extraction point (insertion pt: (f + n) % s)
	uint	n;			// Number of values in the channel
	int		e;			// Element size
	int		freed;		// Set when channel is being deleted
	ulong	qused;		// Bitmap of used entries in rcvrs
	Alt	*	qentry[32];	// Receivers/senders waiting
	uchar	v[1];		// Array of max(1, s) values in the channel
};


/* Channel operations for alt: */
#define CHANEND		0
#define CHANSND		1
#define CHANRCV		2
#define CHANNOP		3
#define CHANNOBLK	4

struct Alt {
	Channel	*c;		/* channel */
	void	*v;		/* pointer to value */
	int	op;		/* operation 0 == send, 1 == receive */

	/* the next variables are used internally to alt
	 * they need not be initialized
	 */
	Channel	**tag;	/* pointer to rendez-vous tag */
	ulong	q;
};

struct Ref {
	long ref;
};

int		alt(Alt alts[]);
Channel*	chancreate(int elemsize, int bufsize);
int		chaninit(Channel *c, int elemsize, int elemcnt);
void		chanfree(Channel *c);
long		decref(Ref *r);		/* returns 0 iff value is now zero */
void		incref(Ref *r);
int		nbrecv(Channel *c, void *v);
void*		nbrecvp(Channel *c);
ulong		nbrecvul(Channel *c);
int		nbsend(Channel *c, void *v);
int		nbsendp(Channel *c, void *v);
int		nbsendul(Channel *c, ulong v);
int		proccreate(void (*f)(void *arg), void *arg, uint stacksize);
int		procrfork(void (*f)(void *arg), void *arg, uint stacksize, int flag);
ulong *		procdata(void);
void		procexec(Channel *, char *, char *[]);
void		procexecl(Channel *, char *, ...);
int		recv(Channel *c, void *v);
void*		recvp(Channel *c);
ulong		recvul(Channel *c);
int		send(Channel *c, void *v);
int		sendp(Channel *c, void *v);
int		sendul(Channel *c, ulong v);
int		threadcreate(void (*f)(void *arg), void *arg, uint stacksize);
ulong*		threaddata(void);
void		threadexits(char *);
void		threadexitsall(char *);
int		threadgetgrp(void);	/* return thread group of current thread */
char*		threadgetname(void);
void		threadkill(int);	/* kill thread */
void		threadkillgrp(int);	/* kill threads in group */
void		threadmain(int argc, char *argv[]);
void		threadnonotes(void);
int		threadpid(int);
int		threadprint(int, char*, ...);
int		threadsetgrp(int);	/* set thread group, return old */
void		threadsetname(char *name);
Channel*	threadwaitchan(void);
void		yield(void);
