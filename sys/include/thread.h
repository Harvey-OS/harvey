#pragma src "/sys/src/libthread"
#pragma lib "libthread.a"

#pragma	varargck	argpos	chanprint	2

typedef struct Alt	Alt;
typedef struct Channel	Channel;
typedef struct Ref	Ref;

/*
 * Channel structure.  S is the size of the buffer.  For unbuffered channels
 * s is zero.  v is an array of s values.  If s is zero, v is unused.
 * f and n represent the state of the queue pointed to by v.
 */

enum {
	Nqwds = 2,
	Nqshift = 5,		/* log₂ # of bits in long */
	Nqmask =  -1,
	Nqbits = (1 << Nqshift) * 2,
};

struct Channel {
	int	s;		/* Size of the channel (may be zero) */
	uint	f;		/* Extraction point (insertion pt: (f+n) % s) */
	uint	n;		/* Number of values in the channel */
	int	e;		/* Element size */
	int	freed;		/* Set when channel is being deleted */
	volatile Alt **qentry;	/* Receivers/senders waiting (malloc) */
	volatile int nentry;	/* # of entries malloc-ed */
	volatile int closed;	/* channel is closed */
	uchar	v[1];		/* Array of s values in the channel */
};


/* Channel operations for alt: */
typedef enum {
	CHANEND,
	CHANSND,
	CHANRCV,
	CHANNOP,
	CHANNOBLK,
} ChanOp;

struct Alt {
	Channel	*c;		/* channel */
	void	*v;		/* pointer to value */
	ChanOp	op;		/* operation */
	char	*err;		/* did the op fail? */
	/*
	 * the next variables are used internally to alt
	 * they need not be initialized
	 */
	Channel	**tag;		/* pointer to rendez-vous tag */
	int	entryno;	/* entry number */
};

struct Ref {
	long	ref;
};

int	alt(Alt alts[]);
int	chanclose(Channel*);
int	chanclosing(Channel *c);
Channel*chancreate(int elemsize, int bufsize);
int	chaninit(Channel *c, int elemsize, int elemcnt);
void	chanfree(Channel *c);
int	chanprint(Channel *, char *, ...);
long	decref(Ref *r);			/* returns 0 iff value is now zero */
void	incref(Ref *r);
int	nbrecv(Channel *c, void *v);
void*	nbrecvp(Channel *c);
ulong	nbrecvul(Channel *c);
int	nbsend(Channel *c, void *v);
int	nbsendp(Channel *c, void *v);
int	nbsendul(Channel *c, ulong v);
void	needstack(int);
int	proccreate(void (*f)(void *arg), void *arg, uint stacksize);
int	procrfork(void (*f)(void *arg), void *arg, uint stacksize, int flag);
void**	procdata(void);
void	procexec(Channel *, char *, char *[]);
void	procexecl(Channel *, char *, ...);
int	recv(Channel *c, void *v);
void*	recvp(Channel *c);
ulong	recvul(Channel *c);
int	send(Channel *c, void *v);
int	sendp(Channel *c, void *v);
int	sendul(Channel *c, ulong v);
int	threadcreate(void (*f)(void *arg), void *arg, uint stacksize);
void**	threaddata(void);
void	threadexits(char *);
void	threadexitsall(char *);
int	threadgetgrp(void);	/* return thread group of current thread */
char*	threadgetname(void);
void	threadint(int);		/* interrupt thread */
void	threadintgrp(int);	/* interrupt threads in grp */
void	threadkill(int);	/* kill thread */
void	threadkillgrp(int);	/* kill threads in group */
void	threadmain(int argc, char *argv[]);
void	threadnonotes(void);
int	threadnotify(int (*f)(void*, char*), int in);
int	threadid(void);
int	threadpid(int);
int	threadsetgrp(int);		/* set thread group, return old */
void	threadsetname(char *fmt, ...);
Channel*threadwaitchan(void);
int	tprivalloc(void);
void	tprivfree(int);
void	**tprivaddr(int);
void	yield(void);

extern	int	mainstacksize;

/* slave I/O processes */
typedef struct Ioproc Ioproc;

#pragma incomplete Ioproc


Ioproc*	ioproc(void);
void	closeioproc(Ioproc*);
void	iointerrupt(Ioproc*);

int	ioclose(Ioproc*, int);
int	iodial(Ioproc*, char*, char*, char*, int*);
int	ioopen(Ioproc*, char*, int);
long	ioread(Ioproc*, int, void*, long);
long	ioreadn(Ioproc*, int, void*, long);
long	iowrite(Ioproc*, int, void*, long);
int	iosleep(Ioproc*, long);

long	iocall(Ioproc*, long (*)(va_list*), ...);
void	ioret(Ioproc*, int);
