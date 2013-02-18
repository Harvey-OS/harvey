typedef struct Block	Block;
typedef struct Chan	Chan;
typedef struct Cmdbuf	Cmdbuf;
typedef struct Cmdtab	Cmdtab;
typedef struct Cname	Cname;
typedef struct Dev	Dev;
typedef struct Dirtab	Dirtab;
typedef struct Egrp	Egrp;
typedef struct Evalue	Evalue;
typedef struct Fgrp	Fgrp;
typedef struct Mount	Mount;
typedef struct Mntcache Mntcache;
typedef struct Mntparam Mntparam;
typedef struct Mntrpc	Mntrpc;
typedef struct Mntwalk	Mntwalk;
typedef struct Mnt	Mnt;
typedef struct Mhead	Mhead;
typedef struct Osenv	Osenv;
typedef struct Pgrp	Pgrp;
typedef struct Proc	Proc;
typedef struct Queue	Queue;
typedef struct Ref	Ref;
typedef struct Rendez	Rendez;
typedef struct Rept	Rept;
typedef struct Rootdata Rootdata;
/*typedef struct RWlock	RWlock;*/
typedef struct RWLock	RWlock;
typedef struct Procs	Procs;
typedef struct Signerkey Signerkey;
typedef struct Skeyset	Skeyset;
typedef struct Uqid	Uqid;
typedef struct Uqidtab	Uqidtab;
typedef struct Walkqid	Walkqid;

#include "lib9.h"
#undef CHDIR
#undef NAMELEN
#undef ERRLEN

#include "emu.h"

#pragma incomplete Queue
#pragma incomplete Mntrpc

#include "fcall.h"

#include "pool.h"

typedef int    Devgen(Chan*, char*, Dirtab*, int, int, Dir*);

enum
{
	NERR		= 32,
	KNAMELEN	= 28,
	MAXROOT		= 5*KNAMELEN, 	/* Maximum root pathname len of devfs-* */
	NUMSIZE		= 11,
	PRINTSIZE	= 256,
	READSTR		= 1000		/* temporary buffer size for device reads */
};

struct Ref
{
	Lock	lk;
	long	ref;
};

struct Rendez
{
	Lock	l;
	Proc*	p;
};

struct Rept
{
	Lock	l;
	Rendez	r;
	void	*o;
	int	t;
	int	(*active)(void*);
	int	(*ck)(void*, int);
	void	(*f)(void*);	/* called with VM acquire()'d */
};

/*
 * Access types in namec & channel flags
 */
enum
{
	Aaccess,			/* as in access, stat */
	Abind,			/* for left-hand-side of bind */
	Atodir,				/* as in chdir */
	Aopen,				/* for i/o */
	Amount,				/* to be mounted upon */
	Acreate,			/* file is to be created */
	Aremove,			/* will be removed by caller */

	COPEN	= 0x0001,		/* for i/o */
	CMSG	= 0x0002,		/* the message channel for a mount */
/*rsc	CCREATE	= 0x0004,		/* permits creation if c->mnt */
	CCEXEC	= 0x0008,		/* close on exec */
	CFREE	= 0x0010,		/* not in use */
	CRCLOSE	= 0x0020,		/* remove on close */
	CCACHE	= 0x0080		/* client cache */
};

struct Chan
{
	Lock	l;
	Ref	r;
	Chan*	next;			/* allocation */
	Chan*	link;
	vlong	offset;			/* in file */
	ushort	type;
	ulong	dev;
	ushort	mode;			/* read/write */
	ushort	flag;
	Qid	qid;
	int	fid;			/* for devmnt */
	ulong	iounit;	/* chunk size for i/o; 0==default */
	Mhead*	umh;			/* mount point that derived Chan; used in unionread */
	Chan*	umc;			/* channel in union; held for union read */
	QLock	umqlock;		/* serialize unionreads */
	int	uri;			/* union read index */
	int	dri;			/* devdirread index */
	ulong	mountid;
	Mntcache *mcp;			/* Mount cache pointer */
	Mnt		*mux;		/* Mnt for clients using me for messages */
	void*	aux;		/* device specific data */
	Chan*	mchan;			/* channel to mounted server */
	Qid	mqid;			/* qid of root of mount point */
	Cname	*name;
};

struct Cname
{
	Ref	r;
	int	alen;			/* allocated length */
	int	len;			/* strlen(s) */
	char	*s;
};

struct Dev
{
	int	dc;
	char*	name;

	void	(*init)(void);
	Chan*	(*attach)(char*);
	Walkqid*	(*walk)(Chan*, Chan*, char**, int);
	int	(*stat)(Chan*, uchar*, int);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, ulong);
	void	(*close)(Chan*);
	long	(*read)(Chan*, void*, long, vlong);
	Block*	(*bread)(Chan*, long, ulong);
	long	(*write)(Chan*, void*, long, vlong);
	long	(*bwrite)(Chan*, Block*, ulong);
	void	(*remove)(Chan*);
	int	(*wstat)(Chan*, uchar*, int);
};

enum
{
	BINTR		=	(1<<0),
	BFREE		=	(1<<1),
	BMORE		=	(1<<2)		/* continued in next block */
};

struct Block
{
	Block*	next;
	Block*	list;
	uchar*	rp;			/* first unconsumed byte */
	uchar*	wp;			/* first empty byte */
	uchar*	lim;			/* 1 past the end of the buffer */
	uchar*	base;			/* start of the buffer */
	void	(*free)(Block*);
	ulong	flag;
};
#define BLEN(s)	((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

struct Dirtab
{
	char	name[KNAMELEN];
	Qid	qid;
	vlong	length;
	long	perm;
};

struct Walkqid
{
	Chan	*clone;
	int	nqid;
	Qid	qid[1];
};

enum
{
	NSMAX	=	1000,
	NSLOG	=	7,
	NSCACHE	=	(1<<NSLOG)
};

struct Mntwalk				/* state for /proc/#/ns */
{
	int		cddone;
	ulong	id;
	Mhead*	mh;
	Mount*	cm;
};

struct Mount
{
	ulong	mountid;
	Mount*	next;
	Mhead*	head;
	Mount*	copy;
	Mount*	order;
	Chan*	to;			/* channel replacing channel */
	int	mflag;
	char	*spec;
};

struct Mhead
{
	Ref	r;
	RWlock	lock;
	Chan*	from;			/* channel mounted upon */
	Mount*	mount;			/* what's mounted upon it */
	Mhead*	hash;			/* Hash chain */
};

struct Mnt
{
	Lock	l;
	/* references are counted using c->ref; channels on this mount point incref(c->mchan) == Mnt.c */
	Chan*	c;		/* Channel to file service */
	Proc*	rip;		/* Reader in progress */
	Mntrpc*	queue;		/* Queue of pending requests on this channel */
	ulong	id;		/* Multiplexor id for channel check */
	Mnt*	list;		/* Free list */
	int	flags;		/* cache */
	int	msize;		/* data + IOHDRSZ */
	char	*version;			/* 9P version */
	Queue	*q;		/* input queue */
};

enum
{
	MNTLOG	=	5,
	MNTHASH =	1<<MNTLOG,		/* Hash to walk mount table */
	DELTAFD=		20,		/* allocation quantum for process file descriptors */
	MAXNFD =		4000,		/* max per process file descriptors */
	MAXKEY =		8	/* keys for signed modules */
};
#define MOUNTH(p,qid)	((p)->mnthash[(qid).path&((1<<MNTLOG)-1)])

struct Mntparam {
	Chan*	chan;
	Chan*	authchan;
	char*	spec;
	int	flags;
};

struct Pgrp
{
	Ref	r;			/* also used as a lock when mounting */
	ulong	pgrpid;
	RWlock	ns;			/* Namespace n read/one write lock */
	QLock	nsh;
	Mhead*	mnthash[MNTHASH];
	int	progmode;
	Chan*	dot;
	Chan*	slash;
	int	nodevs;
	int	pin;
};

enum
{
	Nopin =	-1
};

struct Fgrp
{
	Lock	l;
	Ref	r;
	Chan**	fd;
	int	nfd;			/* number of fd slots */
	int	maxfd;			/* highest fd in use */
	int	minfd;			/* lower bound on free fd */
};

struct Evalue
{
	char	*var;
	char	*val;
	int	len;
	Qid	qid;
	Evalue	*next;
};

struct Egrp
{
	Ref	r;
	QLock	l;
	ulong	path;
	ulong	vers;
	Evalue	*entries;
};

struct Signerkey
{
	Ref	r;
	char*	owner;
	ushort	footprint;
	ulong	expires;
	void*	alg;
	void*	pk;
	void	(*pkfree)(void*);
};

struct Skeyset
{
	Ref	r;
	QLock	l;
	ulong	flags;
	char*	devs;
	int	nkey;
	Signerkey	*keys[MAXKEY];
};

struct Uqid
{
	Ref	r;
	int	type;
	int	dev;
	vlong	oldpath;
	vlong	newpath;
	Uqid*	next;
};

enum
{
	Nqidhash = 32
};

struct Uqidtab
{
	QLock	l;
	Uqid*	qids[Nqidhash];
	ulong	pathgen;
};

struct Osenv
{
	char	*syserrstr;	/* last error from a system call, errbuf0 or 1 */
	char	*errstr;	/* reason we're unwinding the error stack, errbuf1 or 0 */
	char	errbuf0[ERRMAX];
	char	errbuf1[ERRMAX];
	Pgrp*	pgrp;		/* Ref to namespace, working dir and root */
	Fgrp*	fgrp;		/* Ref to file descriptors */
	Egrp*	egrp;	/* Environment vars */
	Skeyset*		sigs;		/* Signed module keys */
	Rendez*	rend;		/* Synchro point */
	Queue*	waitq;		/* Info about dead children */
	Queue*	childq;		/* Info about children for debuggers */
	void*	debug;		/* Debugging master */
	char*	user;	/* Inferno user name */
	FPU	fpu;		/* Floating point thread state */
	int	uid;		/* Numeric user id for host system */
	int	gid;		/* Numeric group id for host system */
	void	*ui;		/* User info for NT */
};

enum
{
	Unknown	= 0xdeadbabe,
	IdleGC	= 0x16,
	Interp	= 0x17,
	BusyGC	= 0x18,
	Moribund
};

struct Proc
{
	int	type;		/* interpreter or not */
	char	text[KNAMELEN];
	Proc*	qnext;		/* list of processes waiting on a Qlock */
	long	pid;
	Proc*	next;		/* list of created processes */
	Proc*	prev;
	Lock	rlock;	/* sync between sleep/swiproc for r */
	Rendez*	r;		/* rendezvous point slept on */
	Rendez	sleep;		/* place to sleep */
	int		killed;		/* by swiproc */
	int	swipend;	/* software interrupt pending for Prog */
	int	syscall;	/* set true under sysio for interruptable syscalls */
	int	intwait;	/* spin wait for note to turn up */
	int	sigid;		/* handle used for signal/note/exception */
	Lock	sysio;		/* note handler lock */
	char	genbuf[128];	/* buffer used e.g. for last name element from namec */
	int	nerr;		/* error stack SP */
	osjmpbuf	estack[NERR];	/* vector of error jump labels */
	char*	kstack;
	void	(*func)(void*);	/* saved trampoline pointer for kproc */
	void*	arg;		/* arg for invoked kproc function */
	void*	iprog;		/* work for Prog after release */
	void*	prog;		/* fake prog for slaves eg. exportfs */
	Osenv*	env;		/* effective operating system environment */
	Osenv	defenv;		/* default env for slaves with no prog */
	osjmpbuf	privstack;	/* private stack for making new kids */
	osjmpbuf	sharestack;
	Proc	*kid;
	void	*kidsp;
	void	*os;		/* host os specific data */
};

#define poperror()	up->nerr--
#define	waserror()	(up->nerr++, ossetjmp(up->estack[up->nerr-1]))

enum
{
	/* kproc flags */
	KPDUPPG		= (1<<0),
	KPDUPFDG	= (1<<1),
	KPDUPENVG	= (1<<2),
	KPX11		= (1<<8),		/* needs silly amount of stack */
	KPDUP		= (KPDUPPG|KPDUPFDG|KPDUPENVG)
};

struct Procs
{
	Lock	l;
	Proc*	head;
	Proc*	tail;
};

struct Rootdata
{
	int	dotdot;
	void	*ptr;
	int	size;
	int	*sizep;
};

extern	Dev*	devtab[];
extern	char	*ossysname;
extern	char	*eve;
extern	Queue*	kbdq;
extern	Queue*	gkbdq;
extern	Queue*	gkscanq;
extern	int	Xsize;
extern	int	Ysize;
extern	Pool*	mainmem;
extern	char	rootdir[MAXROOT];		/* inferno root */
extern	Procs	procs;
extern	int	sflag;
extern	int	xtblbit;
extern	int	globfs;
extern	int	greyscale;
extern	uint	qiomaxatomic;

/*
 * floating point control and status register masks
 */
enum
{
	INVAL		= 0x0001,
	ZDIV		= 0x0002,
	OVFL		= 0x0004,
	UNFL		= 0x0008,
	INEX		= 0x0010,
	RND_NR		= 0x0000,
	RND_NINF	= 0x0100,
	RND_PINF	= 0x0200,
	RND_Z		= 0x0300,
	RND_MASK	= 0x0300
};

struct Cmdbuf
{
	char	*buf;
	char	**f;
	int	nf;
};

struct Cmdtab
{
	int	index;	/* used by client to switch on result */
	char	*cmd;	/* command name */
	int	narg;	/* expected #args; 0 ==> variadic */
};

/* queue state bits,  Qmsg, Qcoalesce, and Qkick can be set in qopen */
enum
{
	/* Queue.state */
	Qstarve		= (1<<0),	/* consumer starved */
	Qmsg		= (1<<1),	/* message stream */
	Qclosed		= (1<<2),	/* queue has been closed/hungup */
	Qflow		= (1<<3),	/* producer flow controlled */
	Qcoalesce	= (1<<4),	/* coallesce packets on read */
	Qkick		= (1<<5),	/* always call the kick routine after qwrite */
};

#define DEVDOTDOT -1

#pragma varargck	type	"I" uchar*
#pragma	varargck	type	"E" uchar*

extern void	(*mainmonitor)(int, void*, ulong);
