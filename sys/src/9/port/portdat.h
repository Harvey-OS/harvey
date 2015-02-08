/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Alarms	Alarms;
typedef struct Block	Block;
typedef struct Chan	Chan;
typedef struct Cmdbuf	Cmdbuf;
typedef struct Cmdtab	Cmdtab;
typedef struct Confmem	Confmem;
typedef struct Dev	Dev;
typedef struct DevConf	DevConf;
typedef struct Dirtab	Dirtab;
typedef struct Edf	Edf;
typedef struct Egrp	Egrp;
typedef struct Evalue	Evalue;
typedef struct Fastcall Fastcall;
typedef struct Fgrp	Fgrp;
typedef struct Image	Image;
typedef struct Kzio 	Kzio;
typedef struct Log	Log;
typedef struct Logflag	Logflag;
typedef struct Lockstats Lockstats;
typedef struct Mhead	Mhead;
typedef struct Mnt	Mnt;
typedef struct Mntcache Mntcache;
typedef struct Mntrpc	Mntrpc;
typedef struct Mntwalk	Mntwalk;
typedef struct Mount	Mount;
typedef struct Note	Note;
typedef struct Page	Page;
typedef struct Pallocmem	Pallocmem;
typedef struct Path	Path;
typedef struct Perf	Perf;
typedef struct Pgalloc Pgalloc;
typedef struct Pgrp	Pgrp;
typedef struct Pgsza	Pgsza;
typedef struct Physseg	Physseg;
typedef struct PhysUart	PhysUart;
typedef struct Proc	Proc;
typedef struct Procalloc	Procalloc;
typedef struct Pte	Pte;
typedef struct QLock	QLock;
typedef struct QLockstats QLockstats;
typedef struct Queue	Queue;
typedef struct Ref	Ref;
typedef struct Rendez	Rendez;
typedef struct Rgrp	Rgrp;
typedef struct RWlock	RWlock;
typedef struct Sched	Sched;
typedef struct Schedq	Schedq;
typedef struct Segment	Segment;
typedef struct Sem	Sem;
typedef struct Sema	Sema;
typedef struct Sems	Sems;
typedef struct Timer	Timer;
typedef struct Timers	Timers;
typedef struct Uart	Uart;
typedef struct Waitq	Waitq;
typedef struct Waitstats Waitstats;
typedef struct Walkqid	Walkqid;
typedef struct Watchdog	Watchdog;
typedef struct Zseg	Zseg;
typedef int    Devgen(Chan*, char*, Dirtab*, int, int, Dir*);

#pragma incomplete DevConf
#pragma incomplete Edf
#pragma incomplete Mntcache
#pragma incomplete Mntrpc
#pragma incomplete Queue
#pragma incomplete Timers

#include <fcall.h>

struct Ref
{
	Lock;
	int	ref;
};

struct Rendez
{
	Lock;
	Proc	*p;
};

enum{
	NWstats = 500,
	WSlock = 0,
	WSqlock,
	WSslock,
};

/*
 * different arrays with stat info, so we can memset any of them
 * to 0 to clear stats.
 */
struct Waitstats
{
	int	on;
	int	npcs;
	int*	type;
	uintptr*	pcs;
	int*	ns;
	uvlong*	wait;
	uvlong* total;
};
extern Waitstats waitstats;

struct Lockstats
{
	ulong	locks;
	ulong	glare;
	ulong	inglare;
};
extern Lockstats lockstats;

struct QLockstats
{
	ulong rlock;
	ulong rlockq;
	ulong wlock;
	ulong wlockq;
	ulong qlock;
	ulong qlockq;
};
extern QLockstats qlockstats;

struct QLock
{
	Lock	use;		/* to access Qlock structure */
	Proc	*head;		/* next process waiting for object */
	Proc	*tail;		/* last process waiting for object */
	int	locked;		/* flag */
	uintptr	pc;
};

struct RWlock
{
	Lock	use;
	Proc	*head;		/* list of waiting processes */
	Proc	*tail;
	uintptr	wpc;		/* pc of writer */
	Proc	*wproc;		/* writing proc */
	int	readers;	/* number of readers */
	int	writer;		/* number of writers */
};

struct Alarms
{
	QLock;
	Proc	*head;
};

/*
 * Access types in namec & channel flags
 */
enum
{
	Aaccess,			/* as in stat, wstat */
	Abind,				/* for left-hand-side of bind */
	Atodir,				/* as in chdir */
	Aopen,				/* for i/o */
	Amount,				/* to be mounted or mounted upon */
	Acreate,			/* is to be created */
	Aremove,			/* will be removed by caller */

	COPEN	= 0x0001,		/* for i/o */
	CMSG	= 0x0002,		/* the message channel for a mount */
/*rsc	CCREATE	= 0x0004,		/* permits creation if c->mnt */
	CCEXEC	= 0x0008,		/* close on exec */
	CFREE	= 0x0010,		/* not in use */
	CRCLOSE	= 0x0020,		/* remove on close */
	CCACHE	= 0x0080,		/* client cache */
};

/* flag values */
enum
{
	BINTR	=	(1<<0),

	Bipck	=	(1<<2),		/* ip checksum */
	Budpck	=	(1<<3),		/* udp checksum */
	Btcpck	=	(1<<4),		/* tcp checksum */
	Bpktck	=	(1<<5),		/* packet checksum */
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
	ushort	flag;
	ushort	checksum;		/* IP checksum of complete packet (minus media header) */
};
#define BLEN(s)	((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

struct Chan
{
	Ref;				/* the Lock in this Ref is also Chan's lock */
	Chan*	next;			/* allocation */
	Chan*	link;
	vlong	offset;			/* in fd */
	vlong	devoffset;		/* in underlying device; see read */
	Dev*	dev;
	uint	devno;
	ushort	mode;			/* read/write */
	ushort	flag;
	Qid	qid;
	int	fid;			/* for devmnt */
	ulong	iounit;			/* chunk size for i/o; 0==default */
	Mhead*	umh;			/* mount point that derived Chan; used in unionread */
	Chan*	umc;			/* channel in union; held for union read */
	QLock	umqlock;		/* serialize unionreads */
	int	uri;			/* union read index */
	int	dri;			/* devdirread index */
	uchar*	dirrock;		/* directory entry rock for translations */
	int	nrock;
	int	mrock;
	QLock	rockqlock;
	int	ismtpt;
	Mntcache*mc;			/* Mount cache pointer */
	Mnt*	mux;			/* Mnt for clients using me for messages */
	union {
		void*	aux;
		Qid	pgrpid;		/* for #p/notepg */
		ulong	mid;		/* for ns in devproc */
	};
	Chan*	mchan;			/* channel to mounted server */
	Qid	mqid;			/* qid of root of mount point */
	Path*	path;
};

struct Path
{
	Ref;
	char*	s;
	Chan**	mtpt;			/* mtpt history */
	int	len;			/* strlen(s) */
	int	alen;			/* allocated length of s */
	int	mlen;			/* number of path elements */
	int	malen;			/* allocated length of mtpt */
};

struct Dev
{
	int	dc;
	char*	name;

	void	(*reset)(void);
	void	(*init)(void);
	void	(*shutdown)(void);
	Chan*	(*attach)(char*);
	Walkqid*(*walk)(Chan*, Chan*, char**, int);
	long	(*stat)(Chan*, uchar*, long);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, int);
	void	(*close)(Chan*);
	long	(*read)(Chan*, void*, long, vlong);
	Block*	(*bread)(Chan*, long, vlong);
	long	(*write)(Chan*, void*, long, vlong);
	long	(*bwrite)(Chan*, Block*, vlong);
	void	(*remove)(Chan*);
	long	(*wstat)(Chan*, uchar*, long);
	void	(*power)(int);	/* power mgt: power(1) => on, power (0) => off */
	int	(*config)(int, char*, DevConf*);	/* returns 0 on error */
	int	(*zread)(Chan*, Kzio*, int, usize, vlong);
	int	(*zwrite)(Chan*, Kzio*, int, vlong);
};

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
	NSCACHE	=	(1<<NSLOG),
};

struct Mntwalk				/* state for /proc/#/ns */
{
	int	cddone;
	Mhead*	mh;
	Mount*	cm;
};

struct Mount
{
	int	mountid;
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
	Ref;
	RWlock	lock;
	Chan*	from;			/* channel mounted upon */
	Mount*	mount;			/* what's mounted upon it */
	Mhead*	hash;			/* Hash chain */
};

struct Mnt
{
	Lock;
	/* references are counted using c->ref; channels on this mount point incref(c->mchan) == Mnt.c */
	Chan	*c;		/* Channel to file service */
	Proc	*rip;		/* Reader in progress */
	Mntrpc	*queue;		/* Queue of pending requests on this channel */
	uint	id;		/* Multiplexer id for channel check */
	Mnt	*list;		/* Free list */
	int	flags;		/* cache */
	int	msize;		/* data + IOHDRSZ */
	char	*version;	/* 9P version */
	Queue	*q;		/* input queue */
};

enum
{
	NUser,				/* note provided externally */
	NExit,				/* deliver note quietly */
	NDebug,				/* print debug message */
};

struct Note
{
	char	msg[ERRMAX];
	int	flag;			/* whether system posted it */
};

enum
{
	PG_NOFLUSH	= 0,
	PG_TXTFLUSH	= 1,		/* flush dcache and invalidate icache */
	PG_DATFLUSH	= 2,		/* flush both i & d caches (UNUSED) */
	PG_NEWCOL	= 3,		/* page has been recolored */

	PG_MOD		= 0x01,		/* software modified bit */
	PG_REF		= 0x02,		/* software referenced bit */
};

struct Page
{
	Lock;
	uintmem	pa;			/* Physical address in memory */
	uintptr	va;			/* Virtual address for user */
	ulong	daddr;			/* Disc address on swap */
	int	ref;			/* Reference count */
	uchar	modref;			/* Simulated modify/reference bits */
	int	color;			/* Cache coloring */
	char	cachectl[MACHMAX];	/* Cache flushing control for mmuput */
	Image	*image;			/* Associated text or swap image */
	Page	*next;			/* Lru free list */
	Page	*prev;
	Page	*hash;			/* Image hash chains */
	int	pgszi;			/* size index in m->pgsz[] */
};

struct Image
{
	Ref;
	Chan	*c;			/* channel to text file */
	Qid 	qid;			/* Qid for page cache coherence */
	Qid	mqid;
	Chan	*mchan;
	int	dc;			/* Device type of owning channel */
//subtype
	Segment *s;			/* TEXT segment for image if running */
	Image	*hash;			/* Qid hash chains */
	Image	*next;			/* Free list or lru list */
	Image	*prev;			/* lru list */
	int	notext;			/* no file associated */
	int	color;
};

/*
 *  virtual MMU
 */
#define PTEMAPMEM	(1ULL*GiB)
#define SEGMAPSIZE	1984
#define SSEGMAPSIZE	16		/* XXX: shouldn't be 32 at least? */

/*
 * Interface between fixfault and mmuput.
 */
#define PTEVALID		(1<<0)
#define PTEWRITE		(1<<1)
#define PTERONLY		(0<<1)
#define PTEUSER		(1<<2)
#define PTEUNCACHED	(1<<4)

struct Pte
{
	Page	**first;		/* First used entry */
	Page	**last;		/* Last used entry */
	Page	*pages[];	/* Page map for this chunk of pte */
};

/* Segment types */
enum
{
	SG_TYPE		= 07,		/* Mask type of segment */
	SG_TEXT		= 00,
	SG_DATA		= 01,
	SG_BSS		= 02,
	SG_STACK	= 03,
	SG_SHARED	= 04,
	SG_PHYSICAL	= 05,

	SG_CACHED	= 0020,		/* Physseg can be cached */
	SG_RONLY	= 0040,		/* Segment is read only */
	SG_CEXEC	= 0100,		/* Detach at exec */
	SG_ZIO		= 0200,		/* used for zero copy */
	SG_KZIO		= 0400,  	/* kernel zero copy segment */
};

#define PG_ONSWAP	1
#define onswap(s)	(PTR2UINT(s) & PG_ONSWAP)
#define pagedout(s)	(PTR2UINT(s) == 0 || onswap(s))
#define swapaddr(s)	(PTR2UINT(s) & ~PG_ONSWAP)

#define SEGMAXPG	(SEGMAPSIZE)

struct Physseg
{
	ulong	attr;			/* Segment attributes */
	char	*name;			/* Attach name */
	uintmem	pa;			/* Physical address */
	usize	size;			/* Maximum segment size in pages */
	int	pgszi;			/* Page size index in Mach  */
	Page	*(*pgalloc)(Segment*, uintptr);	/* Allocation if we need it */
	void	(*pgfree)(Page*);
	uintptr	gva;			/* optional global virtual address */
};

struct Sema
{
	Rendez;
	int*	addr;
	int	waiting;
	Sema*	next;
	Sema*	prev;
};

/* NIX semaphores */
struct Sem
{
	Lock;
	int*	np;		/* user-level semaphore */
	Proc**	q;
	int	nq;
	Sem*	next;		/* in list of semaphores for this Segment */
};

/* NIX semaphores */
struct Sems
{
	Sem**	s;
	int	ns;
};

/* Zero copy per-segment information (locked using Segment.lk) */
struct Zseg
{
	void*	map;	/* memory map for buffers within this segment */
	uintptr	*addr;	/* array of addresses released */
	int	naddr;	/* size allocated for the array */
	int	end;	/* 1+ last used index in addr */
	Rendez	rr;	/* process waiting to read free addresses */
};

#define NOCOLOR -1

struct Segment
{
	Ref;
	QLock	lk;
	ushort	steal;		/* Page stealer lock */
	ushort	type;		/* segment type */
	int	pgszi;		/* page size index in Mach MMMU */
	uint	ptepertab;
	int	color;
	uintptr	base;		/* virtual base */
	uintptr	top;		/* virtual top */
	usize	size;		/* size in pages */
	ulong	fstart;		/* start address in file for demand load */
	ulong	flen;		/* length of segment in file */
	int	flushme;	/* maintain icache for this segment */
	Image	*image;		/* text in file attached to this segment */
	Physseg *pseg;
	ulong*	profile;	/* Tick profile area */
	Pte	**map;
	int	mapsize;
	Pte	*ssegmap[SSEGMAPSIZE];
	Lock	semalock;
	Sema	sema;
	Sems	sems;
	Zseg	zseg;
};

/*
 * NIX zero-copy IO structure.
 */
struct Kzio
{
	Zio;
	Segment* seg;
};

enum
{
	RENDLOG	=	5,
	RENDHASH =	1<<RENDLOG,	/* Hash to lookup rendezvous tags */
	MNTLOG	=	5,
	MNTHASH =	1<<MNTLOG,	/* Hash to walk mount table */
	NFD =		100,		/* per process file descriptors */
	PGHLOG  =	9,
	PGHSIZE	=	1<<PGHLOG,	/* Page hash for image lookup */
};
#define REND(p,s)	((p)->rendhash[(s)&((1<<RENDLOG)-1)])
#define MOUNTH(p,qid)	((p)->mnthash[(qid).path&((1<<MNTLOG)-1)])

struct Pgrp
{
	Ref;				/* also used as a lock when mounting */
	int	noattach;
	ulong	pgrpid;
	QLock	debug;			/* single access via devproc.c */
	RWlock	ns;			/* Namespace n read/one write lock */
	Mhead	*mnthash[MNTHASH];
};

struct Rgrp
{
	Ref;				/* the Ref's lock is also the Rgrp's lock */
	Proc	*rendhash[RENDHASH];	/* Rendezvous tag hash */
};

struct Egrp
{
	Ref;
	RWlock;
	Evalue	**ent;
	int	nent;
	int	ment;
	ulong	path;	/* qid.path of next Evalue to be allocated */
	ulong	vers;	/* of Egrp */
};

struct Evalue
{
	char	*name;
	char	*value;
	int	len;
	Evalue	*link;
	Qid	qid;
};

struct Fgrp
{
	Ref;
	Chan	**fd;
	int	nfd;			/* number allocated */
	int	maxfd;			/* highest fd in use */
	int	exceed;			/* debugging */
};

enum
{
	DELTAFD	= 20		/* incremental increase in Fgrp.fd's */
};

struct Pallocmem
{
	uintmem	base;
	ulong	npage;
};

struct Pgsza
{
	ulong	freecount;	/* how many pages in the free list? */
	Ref	npages;		/* how many pages of this size? */
	Page	*head;		/* MRU */
	Page	*tail;		/* LRU */
};

struct Pgalloc
{
	Lock;
	int	userinit;	/* working in user init mode */
	Pgsza	pgsza[NPGSZ];	/* allocs for m->npgsz page sizes */
	Page*	hash[PGHSIZE];	/* only used for user pages */
	Lock	hashlock;
	Rendez	r;		/* sleep for free mem */
	QLock	pwait;		/* queue of procs waiting for this pgsz */
};

struct Waitq
{
	Waitmsg	w;
	Waitq	*next;
};

/*
 * fasttick timer interrupts
 */
enum {
	/* Mode */
	Trelative,	/* timer programmed in ns from now */
	Tperiodic,	/* periodic timer, period in ns */
};

struct Timer
{
	/* Public interface */
	int	tmode;		/* See above */
	vlong	tns;		/* meaning defined by mode */
	void	(*tf)(Ureg*, Timer*);
	void	*ta;
	/* Internal */
	Lock;
	Timers	*tt;		/* Timers queue this timer runs on */
	vlong	twhen;		/* ns represented in fastticks */
	Timer	*tnext;
};

enum
{
	RFNAMEG		= (1<<0),
	RFENVG		= (1<<1),
	RFFDG		= (1<<2),
	RFNOTEG		= (1<<3),
	RFPROC		= (1<<4),
	RFMEM		= (1<<5),
	RFNOWAIT	= (1<<6),
	RFCNAMEG	= (1<<10),
	RFCENVG		= (1<<11),
	RFCFDG		= (1<<12),
	RFREND		= (1<<13),
	RFNOMNT		= (1<<14),
	RFPREPAGE	= (1<<15),
	RFCPREPAGE	= (1<<16),
	RFCORE		= (1<<17),
	RFCCORE		= (1<<18),
};

/* execac */
enum
{
	EXTC = 0,	/* exec on time-sharing */
	EXAC,		/* want an AC for the exec'd image */
	EXXC,		/* want an XC for the exec'd image */
};


/*
 *  process memory segments - NSEG always last !
 *  HSEG is a potentially huge bss segment.
 */
enum
{
	SSEG, TSEG, DSEG, BSEG, HSEG, ESEG, LSEG, SEG1, SEG2, SEG3, SEG4, NSEG
};

enum
{
	Dead = 0,		/* Process states */
	Moribund,
	Ready,
	Scheding,
	Running,
	Queueing,
	QueueingR,
	QueueingW,
	Wakeme,
	Broken,
	Stopped,
	Rendezvous,
	Waitrelease,
	Exotic,			/* NIX */
	Semdown,

	Proc_stopme = 1, 	/* devproc requests */
	Proc_exitme,
	Proc_traceme,
	Proc_exitbig,
	Proc_tracesyscall,
	Proc_toac,
	Proc_totc,

	TUser = 0, 		/* Proc.time */
	TSys,
	TReal,
	TCUser,
	TCSys,
	TCReal,

	NERR = 64,
	NNOTE = 5,

	Npriq		= 20,		/* number of scheduler priority levels */
	Nrq		= Npriq+2,	/* number of priority levels including real time */
	PriRelease	= Npriq,	/* released edf processes */
	PriEdf		= Npriq+1,	/* active edf processes */
	PriNormal	= 10,		/* base priority for normal processes */
	PriExtra	= Npriq-1,	/* edf processes at high best-effort pri */
	PriKproc	= 13,		/* base priority for kernel processes */
	PriRoot		= 13,		/* base priority for root processes */
};

struct Schedq
{
	Proc*	head;
	Proc*	tail;
	int	n;
};

struct Sched
{
	Lock;			/* runq */
	int	nrdy;
	ulong delayedscheds;	/* statistics */
	long skipscheds;
	long preempts;
	int schedgain;
	ulong balancetime;
	Schedq	runq[Nrq];
	ulong	runvec;
	int	nmach;		/* # of cores with this color */
	ulong	nrun;		/* to compute load */
};

typedef union Ar0 Ar0;
union Ar0 {
	int	i;
	long	l;
	uintptr	p;
	usize	u;
	void*	v;
};

typedef struct Nixpctl Nixpctl;
#pragma incomplete Nixpctl

struct Proc
{
	Label	sched;		/* known to l.s */
	char	*kstack;	/* known to l.s */
	void	*dbgreg;	/* known to l.s User registers for devproc */
	Mach	*mach;		/* machine running this proc */
	char	*text;
	char	*user;
	char	*args;
	int	nargs;		/* number of bytes of args */
	Proc	*rnext;		/* next process in run queue */
	Proc	*qnext;		/* next process on queue for a QLock */
	QLock	*qlock;		/* addr of qlock being queued for DEBUG */
	int	state;
	char	*psstate;	/* What /proc/#/status reports */
	Segment	*seg[NSEG];
	QLock	seglock;	/* locked whenever seg[] changes */
	int	pid;
	int	index;		/* index (slot) in proc array */
	int	ref;		/* indirect reference */
	int	noteid;		/* Equivalent of note group */
	Proc	*pidhash;	/* next proc in pid hash */

	Lock	exl;		/* Lock count and waitq */
	Waitq	*waitq;		/* Exited processes wait children */
	int	nchild;		/* Number of living children */
	int	nwait;		/* Number of uncollected wait records */
	QLock	qwaitr;
	Rendez	waitr;		/* Place to hang out in wait */
	Proc	*parent;

	Pgrp	*pgrp;		/* Process group for namespace */
	Egrp 	*egrp;		/* Environment group */
	Fgrp	*fgrp;		/* File descriptor group */
	Rgrp	*rgrp;		/* Rendez group */

	Fgrp	*closingfgrp;	/* used during teardown */

	int	parentpid;
	ulong	time[6];	/* User, Sys, Real; child U, S, R */

	uvlong	kentry;		/* Kernel entry time stamp (for profiling) */
	/*
	 * pcycles: cycles spent in this process (updated on procsave/restore)
	 * when this is the current proc and we're in the kernel
	 * (procrestores outnumber procsaves by one)
	 * the number of cycles spent in the proc is pcycles + cycles()
	 * when this is not the current process or we're in user mode
	 * (procrestores and procsaves balance), it is pcycles.
	 */
	vlong	pcycles;

	int	insyscall;

	QLock	debug;		/* to access debugging elements of User */
	Proc	*pdbg;		/* the debugging process */
	ulong	procmode;	/* proc device file mode */
	ulong	privatemem;	/* proc does not let anyone read mem */
	int	hang;		/* hang at next exec for debug */
	int	procctl;	/* Control for /proc debugging */
	uintptr	pc;		/* DEBUG only */

	Lock	rlock;		/* sync sleep/wakeup with postnote */
	Rendez	*r;		/* rendezvous point slept on */
	Rendez	sleep;		/* place for syssleep/debug */
	int	notepending;	/* note issued but not acted on */
	int	kp;		/* true if a kernel process */
	Proc	*palarm;	/* Next alarm time */
	ulong	alarm;		/* Time of call */
	int	newtlb;		/* Pager has changed my pte's, I must flush */
	int	noswap;		/* process is not swappable */

	uintptr	rendtag;	/* Tag for rendezvous */
	uintptr	rendval;	/* Value for rendezvous */
	Proc	*rendhash;	/* Hash list for tag values */

	Timer;			/* For tsleep and real-time */
	Rendez	*trend;
	int	(*tfn)(void*);
	void	(*kpfun)(void*);
	void	*kparg;

	int	scallnr;	/* system call number */
	uchar	arg[MAXSYSARG*sizeof(void*)];	/* system call arguments */
	int	nerrlab;
	Label	errlab[NERR];
	char	*syserrstr;	/* last error from a system call, errbuf0 or 1 */
	char	*errstr;	/* reason we're unwinding the error stack, errbuf1 or 0 */
	char	errbuf0[ERRMAX];
	char	errbuf1[ERRMAX];
	char	genbuf[128];	/* buffer used e.g. for last name element from namec */
	Chan	*slash;
	Chan	*dot;

	Note	note[NNOTE];
	short	nnote;
	short	notified;	/* sysnoted is due */
	Note	lastnote;
	void	(*notify)(void*, char*);

	Lock	*lockwait;
	Lock	*lastlock;	/* debugging */
	Lock	*lastilock;	/* debugging */

	Mach	*wired;
	Mach	*mp;		/* machine this process last ran on */
	int	nlocks;		/* number of locks held by proc */
	ulong	delaysched;
	ulong	priority;	/* priority level */
	ulong	basepri;	/* base priority level */
	int	fixedpri;	/* priority level does not change */
	ulong	cpu;		/* cpu average */
	ulong	lastupdate;
	ulong	readytime;	/* time process came ready */
	ulong	movetime;	/* last time process switched processors */
	int	preempted;	/* true if this process hasn't finished the interrupt
				 *  that last preempted it
				 */
	Edf	*edf;		/* if non-null, real-time proc, edf contains scheduling params */
	int	trace;		/* process being traced? */

	uintptr	qpc;		/* pc calling last blocking qlock */

	int	setargs;

	void	*ureg;		/* User registers for notes */
	int	color;

	Fastcall* fc;
	int	fcount;
	char*	syscalltrace;

	/* NIX */
	Mach	*ac;
	Page	*acpml4;
	Sem	*waitsem;
	int	prepagemem;
	Nixpctl *nixpctl;	/* NIX queue based system calls */

	uint	ntrap;		/* # of traps while in this process */
	uint	nintr;		/* # of intrs while in this process */
	uint	nsyscall;	/* # of syscalls made by the process */
	uint	nactrap;	/* # of traps in the AC for this process */
	uint	nacsyscall;	/* # of syscalls in the AC for this process */
	uint	nicc;		/* # of ICCs for the process */
	uvlong	actime1;		/* ticks as of last call in AC */
	uvlong	actime;		/* ∑time from call in AC to ret to AC, and... */
	uvlong	tctime;		/* ∑time from call received to call handled */
	int	nqtrap;		/* # of traps in last quantum */
	int	nqsyscall;	/* # of syscalls in the last quantum */
	int	nfullq;

	/*
	 *  machine specific fpu, mmu and notify
	 */
	PFPU;
	PMMU;
	PNOTIFY;
};

struct Procalloc
{
	Lock;
	int	nproc;
	Proc*	ht[128];
	Proc*	arena;
	Proc*	free;
};

enum
{
	PRINTSIZE =	256,
	MAXCRYPT = 	127,
	NUMSIZE	=	12,		/* size of formatted number */
	MB =		(1024*1024),
	/* READSTR was 1000, which is way too small for usb's ctl file */
	READSTR =	4000,		/* temporary buffer size for device reads */
	WRITESTR =	256,		/* ctl file write max */
};

extern	Conf	conf;
extern	char*	conffile;
extern	int	cpuserver;
extern  char*	eve;
extern	char	hostdomain[];
extern	uchar	initcode[];
extern	int	kbdbuttons;
extern  Ref	noteidalloc;
extern	int	nphysseg;
extern	int	nsyscall;
extern	Pgalloc pga;
extern	Physseg	physseg[];
extern	Procalloc	procalloc;
extern	uint	qiomaxatomic;
extern	char*	statename[];
extern	char*	sysname;
extern struct {
	char*	n;
	void (*f)(Ar0*, va_list);
	Ar0	r;
} systab[];

enum
{
	LRESPROF	= 3,
};

/*
 *  action log
 */
struct Log {
	Lock;
	int	opens;
	char*	buf;
	char	*end;
	char	*rptr;
	int	len;
	int	nlog;
	int	minread;

	int	logmask;	/* mask of things to debug */

	QLock	readq;
	Rendez	readr;
};

struct Logflag {
	char*	name;
	int	mask;
};

enum
{
	NCMDFIELD = 128
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

/*
 *  routines to access UART hardware
 */
struct PhysUart
{
	char*	name;
	Uart*	(*pnp)(void);
	void	(*enable)(Uart*, int);
	void	(*disable)(Uart*);
	void	(*kick)(Uart*);
	void	(*dobreak)(Uart*, int);
	int	(*baud)(Uart*, int);
	int	(*bits)(Uart*, int);
	int	(*stop)(Uart*, int);
	int	(*parity)(Uart*, int);
	void	(*modemctl)(Uart*, int);
	void	(*rts)(Uart*, int);
	void	(*dtr)(Uart*, int);
	long	(*status)(Uart*, void*, long, long);
	void	(*fifo)(Uart*, int);
	void	(*power)(Uart*, int);
	int	(*getc)(Uart*);			/* polling version for rdb */
	void	(*putc)(Uart*, int);		/* polling version for iprint */
	void	(*poll)(Uart*);			/* polled interrupt routine */
};

enum {
	Stagesize=	2048
};

/*
 *  software UART
 */
struct Uart
{
	void*	regs;			/* hardware stuff */
	void*	saveregs;		/* place to put registers on power down */
	char*	name;			/* internal name */
	ulong	freq;			/* clock frequency */
	int	bits;			/* bits per character */
	int	stop;			/* stop bits */
	int	parity;			/* even, odd or no parity */
	int	baud;			/* baud rate */
	PhysUart*phys;
	int	console;		/* used as a serial console */
	int	special;		/* internal kernel device */
	Uart*	next;			/* list of allocated uarts */

	QLock;
	int	type;			/* ?? */
	int	dev;
	int	opens;

	int	enabled;
	Uart	*elist;			/* next enabled interface */

	int	perr;			/* parity errors */
	int	ferr;			/* framing errors */
	int	oerr;			/* rcvr overruns */
	int	berr;			/* no input buffers */
	int	serr;			/* input queue overflow */

	/* buffers */
	int	(*putc)(Queue*, int);
	Queue	*iq;
	Queue	*oq;

	Lock	rlock;
	uchar	istage[Stagesize];
	uchar	*iw;
	uchar	*ir;
	uchar	*ie;

	Lock	tlock;			/* transmit */
	uchar	ostage[Stagesize];
	uchar	*op;
	uchar	*oe;
	int	drain;

	int	modem;			/* hardware flow control on */
	int	xonoff;			/* software flow control on */
	int	blocked;
	int	cts, dsr, dcd;		/* keep track of modem status */
	int	ctsbackoff;
	int	hup_dsr, hup_dcd;	/* send hangup upstream? */
	int	dohup;

	Rendez	r;
};

extern	Uart*	consuart;

/*
 *  performance timers, all units in perfticks
 */
struct Perf
{
	ulong	intrts;		/* time of last interrupt */
	ulong	inintr;		/* time since last clock tick in interrupt handlers */
	ulong	avg_inintr;	/* avg time per clock tick in interrupt handlers */
	ulong	inidle;		/* time since last clock tick in idle loop */
	ulong	avg_inidle;	/* avg time per clock tick in idle loop */
	ulong	last;		/* value of perfticks() at last clock tick */
	ulong	period;		/* perfticks() per clock tick */
};

struct Watchdog
{
	void	(*enable)(void);	/* watchdog enable */
	void	(*disable)(void);	/* watchdog disable */
	void	(*restart)(void);	/* watchdog restart */
	void	(*stat)(char*, char*);	/* watchdog statistics */
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

/* Fast system call struct  -- these are dynamically allocted in Proc struct */
struct Fastcall {
	int	scnum;
	Chan*	c;
	void	(*fun)(Ar0*, Fastcall*);
	void*	buf;
	int	n;
	vlong	offset;
};



#define DEVDOTDOT -1

#pragma	varargck	type	"I"	uchar*
#pragma	varargck	type	"V"	uchar*
#pragma	varargck	type	"E"	uchar*
#pragma	varargck	type	"M"	uchar*
#pragma	varargck	type	"W"	u64int
#pragma	varargck	type	"Z"	Kzio*
