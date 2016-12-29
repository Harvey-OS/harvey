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
typedef struct Ldseg	Ldseg;
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
typedef struct Sema	Sema;
typedef struct Strace   Strace;
typedef struct Timer	Timer;
typedef struct Timers	Timers;
typedef struct Uart	Uart;
typedef struct Waitq	Waitq;
typedef struct Waitstats Waitstats;
typedef struct Walkqid	Walkqid;
typedef struct Watchdog	Watchdog;
typedef struct Watermark Watermark;
typedef struct Zseg	Zseg;
typedef int    Devgen(Chan*, char*, Dirtab*, int, int, Dir*);


#include <fcall.h>

#define	ROUND(s, sz)	(((s)+(sz-1))&~(sz-1))

struct Ref
{
	Lock l;
	int	ref;
};

struct Rendez
{
	Lock l;
	Proc	*_p; // There is already a Proc *p into Lock
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
	uintptr_t*	pcs;
	int*	ns;
	uint64_t*	wait;
	uint64_t* total;
};
extern Waitstats waitstats;

struct Lockstats
{
	uint32_t	locks;
	uint32_t	glare;
	uint32_t	inglare;
};
extern Lockstats lockstats;

struct QLockstats
{
	uint32_t rlock;
	uint32_t rlockq;
	uint32_t wlock;
	uint32_t wlockq;
	uint32_t qlock;
	uint32_t qlockq;
};
extern QLockstats qlockstats;

struct QLock
{
	Lock	use;		/* to access Qlock structure */
	Proc	*head;		/* next process waiting for object */
	Proc	*tail;		/* last process waiting for object */
	int	locked;		/* flag */
	uintptr_t	pc;
};

struct RWlock
{
	Lock	use;
	Proc	*head;		/* list of waiting processes */
	Proc	*tail;
	uintptr_t	wpc;		/* pc of writer */
	Proc	*wproc;		/* writing proc */
	int	readers;	/* number of readers */
	int	writer;		/* number of writers */
};

struct Alarms
{
	QLock ql;
	Proc	*_head;
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
/*rsc	CCREATE	= 0x0004,*/		/* permits creation if c->mnt */
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
	int32_t	ref;
	Block*	next;
	Block*	list;
	unsigned char*	rp;			/* first unconsumed byte */
	unsigned char*	wp;			/* first empty byte */
	unsigned char*	lim;			/* 1 past the end of the buffer */
	unsigned char*	base;			/* start of the buffer */
	void	(*free)(Block*);
	uint16_t	flag;
	uint16_t	checksum;		/* IP checksum of complete packet (minus media header) */
	uint32_t	magic;
};
#define BLEN(s)	((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

struct Chan
{
	Ref r;				/* the Lock in this Ref is also Chan's lock */
	Chan*	next;			/* allocation */
	Chan*	link;
	int64_t	offset;			/* in fd */
	int64_t	devoffset;		/* in underlying device; see read */
	Dev*	dev;
	uint	devno;
	uint16_t	mode;			/* read/write */
	uint16_t	flag;
	Qid	qid;
	int	fid;			/* for devmnt */
	uint32_t	iounit;			/* chunk size for i/o; 0==default */
	Mhead*	umh;			/* mount point that derived Chan; used in unionread */
	Chan*	umc;			/* channel in union; held for union read */
	QLock	umqlock;		/* serialize unionreads */
	int	uri;			/* union read index */
	int	dri;			/* devdirread index */
	unsigned char*	dirrock;		/* directory entry rock for translations */
	int	nrock;
	int	mrock;
	QLock	rockqlock;
	int	ismtpt;
	Mntcache*mc;			/* Mount cache pointer */
	Mnt*	mux;			/* Mnt for clients using me for messages */
	union {
		void*	aux;
		Qid	pgrpid;		/* for #p/notepg */
		uint32_t	mid;		/* for ns in devproc */
	};
	Chan*	mchan;			/* channel to mounted server */
	Qid	mqid;			/* qid of root of mount point */
	Path*	path;
	unsigned char *writebuff;
	int	buffend;
	int	writeoffset;
	int	buffsize;
};

struct Path
{
	Ref r;
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
	int32_t	(*stat)(Chan*, unsigned char*, int32_t);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, int);
	void	(*close)(Chan*);
	int32_t	(*read)(Chan*, void*, int32_t, int64_t);
	Block*	(*bread)(Chan*, int32_t, int64_t);
	int32_t	(*write)(Chan*, void*, int32_t, int64_t);
	int32_t	(*bwrite)(Chan*, Block*, int64_t);
	void	(*remove)(Chan*);
	int32_t	(*wstat)(Chan*, unsigned char*, int32_t);
	void	(*power)(int);	/* power mgt: power(1) => on, power (0) => off */
	int	(*config)(int, char*, DevConf*);	/* returns 0 on error */
	int	(*zread)(Chan*, Kzio*, int, usize, int64_t);
	int	(*zwrite)(Chan*, Kzio*, int, int64_t);
};

struct Dirtab
{
	char	name[KNAMELEN];
	Qid	qid;
	int64_t	length;
	int32_t	perm;
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
	Ref r;
	RWlock	lock;
	Chan*	from;			/* channel mounted upon */
	Mount*	mount;			/* what's mounted upon it */
	Mhead*	hash;			/* Hash chain */
};

struct Mnt
{
	Lock l;
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
	Lock l;
	uintmem	pa;			/* Physical address in memory */
	uintptr_t	va;			/* Virtual address for user */
	uint32_t	daddr;			/* Disc address on swap */
	int	ref;			/* Reference count */
	unsigned char	modref;			/* Simulated modify/reference bits */
	int	color;			/* Cache coloring */
	char	cachectl[MACHMAX];	/* Cache flushing control for mmuput */
	Image	*image;			/* Associated text or swap image */
	Page	*next;			/* Lru free list */
	Page	*prev;
	Page	*hash;			/* Image hash chains */
	int	pgszi;			/* size index in machp()->pgsz[] */
};

struct Image
{
	Ref r;
	Chan *c;
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
#define PTEVALID	(1<<0)
#define PTEWRITE	(1<<1)
#define PTERONLY	(0<<1)
#define PTEUSER	(1<<2)
#define PTENOEXEC	(1<<3)
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
	/* TODO(aki): these types are going to go */
	SG_TYPE		= 0xf,		/* Mask type of segment */
	SG_BAD0		= 0x0,
	SG_TEXT		= 0x1,
	SG_DATA		= 0x2,
	SG_BSS		= 0x3,
	SG_STACK	= 0x4,
	SG_SHARED	= 0x5,
	SG_PHYSICAL	= 0x6,
	SG_MMAP		= 0x7,
	SG_LOAD		= 0x8,	/* replaces SG_TEXT, SG_DATA */

	SG_PERM		= 0xf0,
	SG_READ		= 0x10,
	SG_WRITE	= 0x20,
	SG_EXEC		= 0x40,

	SG_FLAG		= 0xf00,
	SG_CACHED	= 0x100,	/* Physseg can be cached */
	SG_CEXEC	= 0x200,	/* Detach at exec */
	SG_ZIO		= 0x400,	/* used for zero copy */
	SG_KZIO		= 0x800,  	/* kernel zero copy segment */
};
extern char *segtypes[]; /* port/segment.c */

enum
{
	FT_WRITE = 0,
	FT_READ,
	FT_EXEC,
};
extern char *faulttypes[]; /* port/fault.c */

#define PG_ONSWAP	1
#define onswap(s)	(PTR2UINT(s) & PG_ONSWAP)
#define pagedout(s)	(PTR2UINT(s) == 0 || onswap(s))
#define swapaddr(s)	(PTR2UINT(s) & ~PG_ONSWAP)

#define SEGMAXPG	(SEGMAPSIZE)

struct Physseg
{
	uint32_t	attr;			/* Segment attributes */
	char	*name;			/* Attach name */
	uintmem	pa;			/* Physical address */
	usize	size;			/* Maximum segment size in pages */
	int	pgszi;			/* Page size index in Mach  */
	Page	*(*pgalloc)(Segment*, uintptr_t);	/* Allocation if we need it */
	void	(*pgfree)(Page*);
	uintptr_t	gva;			/* optional global virtual address */
};

struct Sema
{
	Rendez rend;
	int*	addr;
	int	waiting;
	Sema*	next;
	Sema*	prev;
};

/* Zero copy per-segment information (locked using Segment.lk) */
struct Zseg
{
	void*	map;	/* memory map for buffers within this segment */
	uintptr_t	*addr;	/* array of addresses released */
	int	naddr;	/* size allocated for the array */
	int	end;	/* 1+ last used index in addr */
	Rendez	rr;	/* process waiting to read free addresses */
};

#define NOCOLOR -1

/* demand loading params of a segment */
struct Ldseg {
	int64_t	memsz;
	int64_t	filesz;
	int64_t	pg0fileoff;
	uintptr_t	pg0vaddr;
	uint32_t	pg0off;
	uint32_t	pgsz;
	uint16_t	type;
};

struct Segment
{
	Ref r;
	QLock	lk;
	uint16_t	steal;		/* Page stealer lock */
	uint16_t	type;		/* segment type */
	int	pgszi;		/* page size index in Mach MMMU */
	uint	ptepertab;
	int	color;
	uintptr_t	base;		/* virtual base */
	uintptr_t	top;		/* virtual top */
	usize	size;		/* size in pages */
	Ldseg	ldseg;
	int	flushme;	/* maintain icache for this segment */
	Image	*image;		/* text in file attached to this segment */
	Physseg *pseg;
	uint32_t	*profile;	/* Tick profile area */
	Pte	**map;
	int	mapsize;
	Pte	*ssegmap[SSEGMAPSIZE];
	Lock	semalock;
	Sema	sema;
	Zseg	zseg;
};


/*
 * NIX zero-copy IO structure.
 */
struct Kzio
{
	Zio Zio;
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
	Ref r;				/* also used as a lock when mounting */
	int	noattach;
	uint32_t	pgrpid;
	QLock	debug;			/* single access via devproc.c */
	RWlock	ns;			/* Namespace n read/one write lock */
	Mhead	*mnthash[MNTHASH];
};

struct Rgrp
{
	Ref r;				/* the Ref's lock is also the Rgrp's lock */
	Proc	*rendhash[RENDHASH];	/* Rendezvous tag hash */
};

struct Egrp
{
	Ref r;
	RWlock rwl;
	Evalue	**ent;
	int	nent;
	int	ment;
	uint32_t	path;	/* qid.path of next Evalue to be allocated */
	uint32_t	vers;	/* of Egrp */
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
	Ref r;
	Chan	**fd;
	int	nfd;			/* number allocated */
	int	maxfd;			/* highest fd in use */
	int	exceed;			/* debugging */
};

enum
{
	DELTAFD	= 20		/* incremental increase in Fgrp.fd's */
};

struct Pgsza
{
	uint32_t	freecount;	/* how many pages in the free list? */
	Ref	npages;		/* how many pages of this size? */
	Page	*head;		/* MRU */
	Page	*tail;		/* LRU */
};

struct Pgalloc
{
	Lock l;
	int	userinit;	/* working in user init mode */
	Pgsza	pgsza[NPGSZ];	/* allocs for m->npgsz page sizes */
	Page*	hash[PGHSIZE];	/* only used for user pages */
	Lock	hashlock;
	Rendez	rend;		/* sleep for free mem */
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
	int64_t	tns;		/* meaning defined by mode */
	void	(*tf)(Ureg*, Timer*);
	void	*ta;
	/* Internal */
	Lock l;
	Timers	*tt;		/* Timers queue this timer runs on */
	int64_t	twhen;		/* ns represented in fastticks */
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
 *  Maximum number of process memory segments
 */
enum {
	NSEG = 12
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
	Lock l;			/* runq */
	int	nrdy;
	uint32_t delayedscheds;	/* statistics */
	int32_t skipscheds;
	int32_t preempts;
	int schedgain;
	uint32_t balancetime;
	Schedq	runq[Nrq];
	uint32_t	runvec;
	int	nmach;		/* # of cores with this color */
	uint32_t	nrun;		/* to compute load */
};

typedef union Ar0 Ar0;
union Ar0 {
	intptr_t	i;
	int32_t	l;
	uintptr_t	p;
	usize	u;
	void*	v;
	int64_t	vl;
};

typedef struct Nixpctl Nixpctl;

struct Strace {
	int opened;
	int tracing;
	int inherit;
	Ref procs; /* when procs goes to zero, q is hung up. */
	Ref users; /* when users goes to zero, q and struct are freed. */
	Queue *q;
};

struct Proc
{
	Label	sched;		/* known to l.s */
	char	*kstack;	/* known to l.s */
	void	*dbgreg;	/* known to l.s User registers for devproc */
	uintptr_t	tls;		/* known to l.s thread local storage */
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
	uint64_t	time[6];	/* User, Sys, Real; child U, S, R */

	uint64_t	kentry;		/* Kernel entry time stamp (for profiling) */
	/*
	 * pcycles: cycles spent in this process (updated on procsave/restore)
	 * when this is the current proc and we're in the kernel
	 * (procrestores outnumber procsaves by one)
	 * the number of cycles spent in the proc is pcycles + cycles()
	 * when this is not the current process or we're in user mode
	 * (procrestores and procsaves balance), it is pcycles.
	 */
	int64_t	pcycles;

	int	insyscall;

	QLock	debug;		/* to access debugging elements of User */
	Proc	*pdbg;		/* the debugging process */
	uint32_t	procmode;	/* proc device file mode */
	uint32_t	privatemem;	/* proc does not let anyone read mem */
	int	hang;		/* hang at next exec for debug */
	int	procctl;	/* Control for /proc debugging */
	uintptr	pc;		/* DEBUG only */

	Lock	rlock;		/* sync sleep/wakeup with postnote */
	Rendez	*r;		/* rendezvous point slept on */
	Rendez	sleep;		/* place for syssleep/debug/tsleep */
	int	notepending;	/* note issued but not acted on */
	int	kp;		/* true if a kernel process */
	Proc	*palarm;	/* Next alarm time */
	uint32_t	alarm;		/* Time of call */
	int	newtlb;		/* Pager has changed my pte's, I must flush */
	int	noswap;		/* process is not swappable */

	uintptr	rendtag;	/* Tag for rendezvous */
	uintptr	rendval;	/* Value for rendezvous */
	Proc	*rendhash;	/* Hash list for tag values */

	Timer Timer;			/* For tsleep and real-time */
	Rendez	*trend;
	int	(*tfn)(void*);
	void	(*kpfun)(void*);
	void	*kparg;

	int	scallnr;	/* system call number */
	unsigned char	arg[MAXSYSARG*sizeof(void*)];	/* system call arguments */
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
	uint32_t	delaysched;
	uint32_t	priority;	/* priority level */
	uint32_t	basepri;	/* base priority level */
	int	fixedpri;	/* priority level does not change */
	uint64_t	cpu;		/* cpu average */
	uint64_t	lastupdate;
	uint64_t	readytime;	/* time process came ready */
	uint64_t	movetime;	/* last time process switched processors */
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
	Page	*acPageTableRoot;
	int	prepagemem;
	Nixpctl *nixpctl;	/* NIX queue based system calls */

	uint	ntrap;		/* # of traps while in this process */
	uint	nintr;		/* # of intrs while in this process */
	uint	nsyscall;	/* # of syscalls made by the process */
	uint	nactrap;	/* # of traps in the AC for this process */
	uint	nacsyscall;	/* # of syscalls in the AC for this process */
	uint	nicc;		/* # of ICCs for the process */
	uint64_t	actime1;		/* ticks as of last call in AC */
	uint64_t	actime;		/* ∑time from call in AC to ret to AC, and... */
	uint64_t	tctime;		/* ∑time from call received to call handled */
	int	nqtrap;		/* # of traps in last quantum */
	int	nqsyscall;	/* # of syscalls in the last quantum */
	int	nfullq;

	/*
	 *  machine specific fpu, mmu and notify
	 */
	PFPU FPU;
	PMMU MMU;
	PNOTIFY NOTIFY;

	/*
	 * mmap support.
	 * For addresses that we reference that don't have a mapping,
	 * if this queue is not NULL, we will send a message on it to be
	 * handled by some other proc (not ourselves) and block on reading
	 * a result back.
	 */
	Queue *req, *resp;

	/* new strace support */

	Strace *strace;
	int strace_on;
	int strace_inherit;
};

struct Procalloc
{
	Lock l;
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
extern	char*	cputype;
extern	int	cpuserver;
extern  char*	eve;
extern	char	hostdomain[];
extern	uint8_t	initcode[];
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

typedef	struct Systab Systab;
struct Systab {
	char*	n;
	void (*f)(Ar0*, ...);
	Ar0	r;
};

extern Systab systab[];

enum
{
	LRESPROF	= 3,
};

/*
 *  action log
 */
struct Log {
	Lock l;
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
	int32_t	(*status)(Uart*, void*, int32_t, int32_t);
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
	uint32_t	freq;			/* clock frequency */
	int	bits;			/* bits per character */
	int	stop;			/* stop bits */
	int	parity;			/* even, odd or no parity */
	int	baud;			/* baud rate */
	PhysUart*phys;
	int	console;		/* used as a serial console */
	int	special;		/* internal kernel device */
	Uart*	next;			/* list of allocated uarts */

	QLock ql;
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
	unsigned char	istage[Stagesize];
	unsigned char	*iw;
	unsigned char	*ir;
	unsigned char	*ie;

	Lock	tlock;			/* transmit */
	unsigned char	ostage[Stagesize];
	unsigned char	*op;
	unsigned char	*oe;
	int	drain;

	int	modem;			/* hardware flow control on */
	int	xonoff;			/* software flow control on */
	int	blocked;
	int	cts, dsr, dcd;		/* keep track of modem status */
	int	ctsbackoff;
	int	hup_dsr, hup_dcd;	/* send hangup upstream? */
	int	dohup;

	Rendez	rend;
};

extern	Uart*	consuart;

/*
 *  performance timers, all units in perfticks
 */
struct Perf
{
	uint64_t	intrts;		/* time of last interrupt */
	uint64_t	inintr;		/* time since last clock tick in interrupt handlers */
	uint64_t	avg_inintr;	/* avg time per clock tick in interrupt handlers */
	uint64_t	inidle;		/* time since last clock tick in idle loop */
	uint64_t	avg_inidle;	/* avg time per clock tick in idle loop */
	uint64_t	last;		/* value of perfticks() at last clock tick */
	uint64_t	period;		/* perfticks() per clock tick */
};

struct Watchdog
{
	void	(*enable)(void);	/* watchdog enable */
	void	(*disable)(void);	/* watchdog disable */
	void	(*restart)(void);	/* watchdog restart */
	void	(*stat)(char*, char*);	/* watchdog statistics */
};

struct Watermark
{
	int	highwater;
	int	curr;
	int	max;
	int	hitmax;		/* count: how many times hit max? */
	char	*name;
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
	int64_t	offset;
};



#define DEVDOTDOT -1

