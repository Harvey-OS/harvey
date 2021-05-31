/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Alarms Alarms;
typedef struct Block Block;
typedef struct Chan Chan;
typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
typedef struct Confmem Confmem;
typedef struct Dev Dev;
typedef struct DevConf DevConf;
typedef struct Dirtab Dirtab;
typedef struct Edf Edf;
typedef struct Egrp Egrp;
typedef struct Evalue Evalue;
typedef struct Fastcall Fastcall;
typedef struct Fgrp Fgrp;
typedef struct Image Image;
typedef struct Kzio Kzio;
typedef struct Ldseg Ldseg;
typedef struct Log Log;
typedef struct Logflag Logflag;
typedef struct Lockstats Lockstats;
typedef struct Mhead Mhead;
typedef struct Mnt Mnt;
typedef struct Mntcache Mntcache;
typedef struct Mntrpc Mntrpc;
typedef struct Mntwalk Mntwalk;
typedef struct Mount Mount;
typedef struct Note Note;
typedef struct Page Page;
typedef struct Path Path;
typedef struct Perf Perf;
typedef struct Pgalloc Pgalloc;
typedef struct Pgrp Pgrp;
typedef struct Pgsza Pgsza;
typedef struct Physseg Physseg;
typedef struct PhysUart PhysUart;
typedef struct Proc Proc;
typedef struct Procalloc Procalloc;
typedef struct Pte Pte;
typedef struct QLock QLock;
typedef struct QLockstats QLockstats;
typedef struct Queue Queue;
typedef struct Ref Ref;
typedef struct Rendez Rendez;
typedef struct Rgrp Rgrp;
typedef struct RWlock RWlock;
typedef struct Sched Sched;
typedef struct Schedq Schedq;
typedef struct Segment Segment;
typedef struct Sema Sema;
typedef struct Strace Strace;
typedef struct Timer Timer;
typedef struct Timers Timers;
typedef struct Uart Uart;
typedef struct Waitq Waitq;
typedef struct Waitstats Waitstats;
typedef struct Walkqid Walkqid;
typedef struct Watchdog Watchdog;
typedef struct Watermark Watermark;
typedef struct Zseg Zseg;
typedef int Devgen(Chan *, char *, Dirtab *, int, int, Dir *);

#include <fcall.h>

#define ROUND(s, sz) (((s) + (sz - 1)) & ~(sz - 1))

struct Ref {
	Lock l;
	int ref;
};

struct Rendez {
	Lock l;
	Proc *_p;	 // There is already a Proc *p into Lock
};

enum {
	NWstats = 500,
	WSlock = 0,
	WSqlock,
	WSslock,
};

/*
 * different arrays with stat info, so we can memset any of them
 * to 0 to clear stats.
 */
struct Waitstats {
	int on;
	int npcs;
	int *type;
	usize *pcs;
	int *ns;
	u64 *wait;
	u64 *total;
};
extern Waitstats waitstats;

struct Lockstats {
	u32 locks;
	u32 glare;
	u32 inglare;
};
extern Lockstats lockstats;

struct QLockstats {
	u32 rlock;
	u32 rlockq;
	u32 wlock;
	u32 wlockq;
	u32 qlock;
	u32 qlockq;
};
extern QLockstats qlockstats;

struct QLock {
	Lock use;   /* to access Qlock structure */
	Proc *head; /* next process waiting for object */
	Proc *tail; /* last process waiting for object */
	int locked; /* flag */
	usize pc;
};

struct RWlock {
	Lock use;
	Proc *head; /* list of waiting processes */
	Proc *tail;
	usize wpc; /* pc of writer */
	Proc *wproc;   /* writing proc */
	int readers;   /* number of readers */
	int writer;    /* number of writers */
};

struct Alarms {
	QLock ql;
	Proc *_head;
};

/*
 * Access types in namec & channel flags
 */
enum {
	Aaccess, /* as in stat, wstat */
	Abind,	 /* for left-hand-side of bind */
	Atodir,	 /* as in chdir */
	Aopen,	 /* for i/o */
	Amount,	 /* to be mounted or mounted upon */
	Acreate, /* is to be created */
	Aremove, /* will be removed by caller */

	COPEN = 0x0001,		    /* for i/o */
	CMSG = 0x0002,		    /* the message channel for a mount */
	/*rsc	CCREATE	= 0x0004,*/ /* permits creation if c->mnt */
	CCEXEC = 0x0008,	    /* close on exec */
	CFREE = 0x0010,		    /* not in use */
	CRCLOSE = 0x0020,	    /* remove on close */
	CCACHE = 0x0080,	    /* client cache */
};

/* flag values */
enum {
	BINTR = (1 << 0),

	Bipck = (1 << 2),  /* ip checksum */
	Budpck = (1 << 3), /* udp checksum */
	Btcpck = (1 << 4), /* tcp checksum */
	Bpktck = (1 << 5), /* packet checksum */
};

struct Block {
	i32 ref;
	Block *next;
	Block *list;
	unsigned char *rp;   /* first unconsumed byte */
	unsigned char *wp;   /* first empty byte */
	unsigned char *lim;  /* 1 past the end of the buffer */
	unsigned char *base; /* start of the buffer */
	void (*free)(Block *);
	u16 flag;
	u16 checksum; /* IP checksum of complete packet (minus media header) */
	u32 magic;
};
#define BLEN(s) ((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

struct Chan {
	Ref r;	    /* the Lock in this Ref is also Chan's lock */
	Chan *next; /* allocation */
	Chan *link;
	i64 offset;	   /* in fd */
	i64 devoffset; /* in underlying device; see read */
	Dev *dev;
	u32 devno;
	u16 mode; /* read/write */
	u16 flag;
	Qid qid;
	int fid;		/* for devmnt */
	u32 iounit;	/* chunk size for i/o; 0==default */
	Mhead *umh;		/* mount point that derived Chan; used in unionread */
	Chan *umc;		/* channel in union; held for union read */
	QLock umqlock;		/* serialize unionreads */
	int uri;		/* union read index */
	int dri;		/* devdirread index */
	unsigned char *dirrock; /* directory entry rock for translations */
	int nrock;
	int mrock;
	QLock rockqlock;
	int ismtpt;
	Mntcache *mc; /* Mount cache pointer */
	Mnt *mux;     /* Mnt for clients using me for messages */
	union {
		void *aux;
		Qid pgrpid;   /* for #p/notepg */
		u32 mid; /* for ns in devproc */
	};
	Chan *mchan; /* channel to mounted server */
	Qid mqid;    /* qid of root of mount point */
	Path *path;
	unsigned char *writebuff;
	int buffend;
	int writeoffset;
	int buffsize;
};

struct Path {
	Ref r;
	char *s;
	Chan **mtpt; /* mtpt history */
	int len;     /* strlen(s) */
	int alen;    /* allocated length of s */
	int mlen;    /* number of path elements */
	int malen;   /* allocated length of mtpt */
};

struct Dev {
	int dc;
	char *name;

	void (*reset)(void);
	void (*init)(void);
	void (*shutdown)(void);
	Chan *(*attach)(char *);
	Walkqid *(*walk)(Chan *, Chan *, char **, int);
	i32 (*stat)(Chan *, unsigned char *, i32);
	Chan *(*open)(Chan *, int);
	void (*create)(Chan *, char *, int, int);
	void (*close)(Chan *);
	i32 (*read)(Chan *, void *, i32, i64);
	Block *(*bread)(Chan *, i32, i64);
	i32 (*write)(Chan *, void *, i32, i64);
	i32 (*bwrite)(Chan *, Block *, i64);
	void (*remove)(Chan *);
	i32 (*wstat)(Chan *, unsigned char *, i32);
	void (*power)(int);		       /* power mgt: power(1) => on, power (0) => off */
	int (*config)(int, char *, DevConf *); /* returns 0 on error */
	int (*zread)(Chan *, Kzio *, int, usize, i64);
	int (*zwrite)(Chan *, Kzio *, int, i64);
};

struct Dirtab {
	char name[KNAMELEN];
	Qid qid;
	i64 length;
	i32 perm;
};

struct Walkqid {
	Chan *clone;
	int nqid;
	Qid qid[1];
};

enum {
	NSMAX = 1000,
	NSLOG = 7,
	NSCACHE = (1 << NSLOG),
};

struct Mntwalk /* state for /proc/#/ns */
{
	int cddone;
	Mhead *mh;
	Mount *cm;
};

struct Mount {
	int mountid;
	Mount *next;
	Mhead *head;
	Mount *copy;
	Mount *order;
	Chan *to; /* channel replacing channel */
	int mflag;
	char *spec;
};

struct Mhead {
	Ref r;
	RWlock lock;
	Chan *from;   /* channel mounted upon */
	Mount *mount; /* what's mounted upon it */
	Mhead *hash;  /* Hash chain */
};

struct Mnt {
	Lock l;
	/* references are counted using c->ref; channels on this mount point incref(c->mchan) == Mnt.c */
	Chan *c;       /* Channel to file service */
	Proc *rip;     /* Reader in progress */
	Mntrpc *queue; /* Queue of pending requests on this channel */
	u32 id;       /* Multiplexer id for channel check */
	Mnt *list;     /* Free list */
	int flags;     /* cache */
	int msize;     /* data + IOHDRSZ */
	char *version; /* 9P version */
	Queue *q;      /* input queue */
};

enum {
	NUser,	/* note provided externally */
	NExit,	/* deliver note quietly */
	NDebug, /* print debug message */
};

struct Note {
	char msg[ERRMAX];
	int flag; /* whether system posted it */
};

enum {
	PG_NOFLUSH = 0,
	PG_TXTFLUSH = 1, /* flush dcache and invalidate icache */
	PG_DATFLUSH = 2, /* flush both i & d caches (UNUSED) */
	PG_NEWCOL = 3,	 /* page has been recolored */

	PG_MOD = 0x01, /* software modified bit */
	PG_REF = 0x02, /* software referenced bit */
};

struct Page {
	Lock l;
	u64 pa;		/* Physical address in memory */
	usize va;		/* Virtual address for user */
	u32 daddr;		/* Disc address on swap */
	int ref;		/* Reference count */
	unsigned char modref;	/* Simulated modify/reference bits */
	int color;		/* Cache coloring */
	char cachectl[MACHMAX]; /* Cache flushing control for mmuput */
	Image *image;		/* Associated text or swap image */
	Page *next;		/* Lru free list */
	Page *prev;
	Page *hash; /* Image hash chains */
	int pgszi;  /* size index in machp()->pgsz[] */
};

struct Image {
	Ref r;
	Chan *c;
	Qid qid; /* Qid for page cache coherence */
	Qid mqid;
	Chan *mchan;
	int dc;	     /* Device type of owning channel */
		     //subtype
	Segment *s;  /* TEXT segment for image if running */
	Image *hash; /* Qid hash chains */
	Image *next; /* Free list or lru list */
	Image *prev; /* lru list */
	int notext;  /* no file associated */
	int color;
};

/*
 *  virtual MMU
 */
#define PTEMAPMEM (1ULL * GiB)
#define SEGMAPSIZE 1984
#define SSEGMAPSIZE 16 /* XXX: shouldn't be 32 at least? */

/*
 * Interface between fixfault and mmuput.
 */
#define PTEVALID (1 << 0)
#define PTEWRITE (1 << 1)
#define PTERONLY (0 << 1)
#define PTEUSER (1 << 2)
#define PTENOEXEC (1 << 3)
#define PTEUNCACHED (1 << 4)

struct Pte {
	Page **first;  /* First used entry */
	Page **last;   /* Last used entry */
	Page *pages[]; /* Page map for this chunk of pte */
};

/* Segment types */
enum {
	/* TODO(aki): these types are going to go */
	SG_TYPE = 0xf, /* Mask type of segment */
	SG_BAD0 = 0x0,
	SG_TEXT = 0x1,
	SG_DATA = 0x2,
	SG_BSS = 0x3,
	SG_STACK = 0x4,
	SG_SHARED = 0x5,
	SG_PHYSICAL = 0x6,
	SG_MMAP = 0x7,
	SG_LOAD = 0x8, /* replaces SG_TEXT, SG_DATA */

	SG_PERM = 0xf0,
	SG_READ = 0x10,
	SG_WRITE = 0x20,
	SG_EXEC = 0x40,

	SG_FLAG = 0xf00,
	SG_CACHED = 0x100, /* Physseg can be cached */
	SG_CEXEC = 0x200,  /* Detach at exec */
	SG_ZIO = 0x400,	   /* used for zero copy */
	SG_KZIO = 0x800,   /* kernel zero copy segment */
};
extern char *segtypes[]; /* port/segment.c */

enum {
	FT_WRITE = 0,
	FT_READ,
	FT_EXEC,
};
extern char *faulttypes[]; /* port/fault.c */

#define PG_ONSWAP 1
#define onswap(s) (PTR2UINT(s) & PG_ONSWAP)
#define pagedout(s) (PTR2UINT(s) == 0 || onswap(s))
#define swapaddr(s) (PTR2UINT(s) & ~PG_ONSWAP)

#define SEGMAXPG (SEGMAPSIZE)

struct Physseg {
	u32 attr;				/* Segment attributes */
	char *name;				/* Attach name */
	u64 pa;				/* Physical address */
	usize size;				/* Maximum segment size in pages */
	int pgszi;				/* Page size index in Mach  */
	Page *(*pgalloc)(Segment *, usize); /* Allocation if we need it */
	void (*pgfree)(Page *);
	usize gva; /* optional global virtual address */
};

struct Sema {
	Rendez rend;
	int *addr;
	int waiting;
	Sema *next;
	Sema *prev;
};

/* Zero copy per-segment information (locked using Segment.lk) */
struct Zseg {
	void *map;	 /* memory map for buffers within this segment */
	usize *addr; /* array of addresses released */
	int naddr;	 /* size allocated for the array */
	int end;	 /* 1+ last used index in addr */
	Rendez rr;	 /* process waiting to read free addresses */
};

#define NOCOLOR -1

/* demand loading params of a segment */
struct Ldseg {
	i64 memsz;
	i64 filesz;
	i64 pg0fileoff;
	usize pg0vaddr;
	u32 pg0off;
	u32 pgsz;
	u16 type;
};

struct Segment {
	Ref r;
	QLock lk;
	u16 steal; /* Page stealer lock */
	u16 type;	/* segment type */
	int pgszi;	/* page size index in Mach MMMU */
	u32 ptepertab;
	int color;
	usize base; /* virtual base */
	usize top;	/* virtual top */
	usize size;	/* size in pages */
	Ldseg ldseg;
	int flushme;  /* maintain icache for this segment */
	Image *image; /* text in file attached to this segment */
	Physseg *pseg;
	u32 *profile; /* Tick profile area */
	Pte **map;
	int mapsize;
	Pte *ssegmap[SSEGMAPSIZE];
	Lock semalock;
	Sema sema;
	Zseg zseg;
};

/*
 * NIX zero-copy IO structure.
 */
struct Kzio {
	Zio Zio;
	Segment *seg;
};

enum {
	RENDLOG = 5,
	RENDHASH = 1 << RENDLOG, /* Hash to lookup rendezvous tags */
	MNTLOG = 5,
	MNTHASH = 1 << MNTLOG, /* Hash to walk mount table */
	NFD = 100,	       /* per process file descriptors */
	PGHLOG = 9,
	PGHSIZE = 1 << PGHLOG, /* Page hash for image lookup */
};
#define REND(p, s) ((p)->rendhash[(s) & ((1 << RENDLOG) - 1)])
#define MOUNTH(p, qid) ((p)->mnthash[(qid).path & ((1 << MNTLOG) - 1)])

struct Pgrp {
	Ref r; /* also used as a lock when mounting */
	int noattach;
	u32 pgrpid;
	QLock debug; /* single access via devproc.c */
	RWlock ns;   /* Namespace n read/one write lock */
	Mhead *mnthash[MNTHASH];
};

struct Rgrp {
	Ref r;			  /* the Ref's lock is also the Rgrp's lock */
	Proc *rendhash[RENDHASH]; /* Rendezvous tag hash */
};

struct Egrp {
	Ref r;
	RWlock rwl;
	Evalue **ent;
	int nent;
	int ment;
	u32 path; /* qid.path of next Evalue to be allocated */
	u32 vers; /* of Egrp */
};

struct Evalue {
	char *name;
	char *value;
	int len;
	Evalue *link;
	Qid qid;
};

struct Fgrp {
	Ref r;
	Chan **fd;
	int nfd;    /* number allocated */
	int maxfd;  /* highest fd in use */
	int exceed; /* debugging */
};

enum {
	DELTAFD = 20 /* incremental increase in Fgrp.fd's */
};

struct Pgsza {
	u32 freecount; /* how many pages in the free list? */
	Ref npages;	    /* how many pages of this size? */
	Page *head;	    /* MRU */
	Page *tail;	    /* LRU */
};

struct Pgalloc {
	Lock l;
	int userinit;	     /* working in user init mode */
	Pgsza pgsza[NPGSZ];  /* allocs for m->npgsz page sizes */
	Page *hash[PGHSIZE]; /* only used for user pages */
	Lock hashlock;
	Rendez rend; /* sleep for free mem */
	QLock pwait; /* queue of procs waiting for this pgsz */
};

struct Waitq {
	Waitmsg w;
	Waitq *next;
};

/*
 * fasttick timer interrupts
 */
enum {
	/* Mode */
	Trelative, /* timer programmed in ns from now */
	Tperiodic, /* periodic timer, period in ns */
};

struct Timer {
	/* Public interface */
	int tmode;   /* See above */
	i64 tns; /* meaning defined by mode */
	void (*tf)(Ureg *, Timer *);
	void *ta;
	/* Internal */
	Lock l;
	Timers *tt;    /* Timers queue this timer runs on */
	i64 twhen; /* ns represented in fastticks */
	Timer *tnext;
};

enum {
	RFNAMEG = (1 << 0),
	RFENVG = (1 << 1),
	RFFDG = (1 << 2),
	RFNOTEG = (1 << 3),
	RFPROC = (1 << 4),
	RFMEM = (1 << 5),
	RFNOWAIT = (1 << 6),
	RFCNAMEG = (1 << 10),
	RFCENVG = (1 << 11),
	RFCFDG = (1 << 12),
	RFREND = (1 << 13),
	RFNOMNT = (1 << 14),
	RFPREPAGE = (1 << 15),
	RFCPREPAGE = (1 << 16),
	RFCORE = (1 << 17),
	RFCCORE = (1 << 18),
};

/* execac */
enum {
	EXTC = 0, /* exec on time-sharing */
	EXAC,	  /* want an AC for the exec'd image */
	EXXC,	  /* want an XC for the exec'd image */
};

/*
 *  Maximum number of process memory segments
 */
enum {
	NSEG = 12
};

enum {
	Dead = 0, /* Process states */
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
	Exotic, /* NIX */
	Semdown,

	Proc_stopme = 1, /* devproc requests */
	Proc_exitme,
	Proc_traceme,
	Proc_exitbig,
	Proc_tracesyscall,
	Proc_toac,
	Proc_totc,

	TUser = 0, /* Proc.time */
	TSys,
	TReal,
	TCUser,
	TCSys,
	TCReal,

	NERR = 64,
	NNOTE = 5,

	Npriq = 20,	      /* number of scheduler priority levels */
	Nrq = Npriq + 2,      /* number of priority levels including real time */
	PriRelease = Npriq,   /* released edf processes */
	PriEdf = Npriq + 1,   /* active edf processes */
	PriNormal = 10,	      /* base priority for normal processes */
	PriExtra = Npriq - 1, /* edf processes at high best-effort pri */
	PriKproc = 13,	      /* base priority for kernel processes */
	PriRoot = 13,	      /* base priority for root processes */
};

struct Schedq {
	Proc *head;
	Proc *tail;
	int n;
};

struct Sched {
	Lock l; /* runq */
	int nrdy;
	u32 delayedscheds; /* statistics */
	i32 skipscheds;
	i32 preempts;
	int schedgain;
	u32 balancetime;
	Schedq runq[Nrq];
	u32 runvec;
	int nmach;     /* # of cores with this color */
	u32 nrun; /* to compute load */
};

typedef union Ar0 Ar0;
union Ar0 {
	isize i;
	i32 l;
	usize p;
	usize u;
	void *v;
	i64 vl;
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

struct Proc {
	Label sched;   /* known to l.s */
	char *kstack;  /* known to l.s */
	void *dbgreg;  /* known to l.s User registers for devproc */
	usize tls; /* known to l.s thread local storage */
	Mach *mach;    /* machine running this proc */
	char *text;
	char *user;
	char *args;
	int nargs;    /* number of bytes of args */
	Proc *rnext;  /* next process in run queue */
	Proc *qnext;  /* next process on queue for a QLock */
	QLock *qlock; /* addr of qlock being queued for DEBUG */
	int state;
	char *psstate; /* What /proc/#/status reports */
	Segment *seg[NSEG];
	QLock seglock; /* locked whenever seg[] changes */
	int pid;
	int index;     /* index (slot) in proc array */
	int ref;       /* indirect reference */
	int noteid;    /* Equivalent of note group */
	Proc *pidhash; /* next proc in pid hash */

	Lock exl;     /* Lock count and waitq */
	Waitq *waitq; /* Exited processes wait children */
	int nchild;   /* Number of living children */
	int nwait;    /* Number of uncollected wait records */
	QLock qwaitr;
	Rendez waitr; /* Place to hang out in wait */
	Proc *parent;

	Pgrp *pgrp; /* Process group for namespace */
	Egrp *egrp; /* Environment group */
	Fgrp *fgrp; /* File descriptor group */
	Rgrp *rgrp; /* Rendez group */

	Fgrp *closingfgrp; /* used during teardown */

	int parentpid;
	u64 time[6]; /* User, Sys, Real; child U, S, R */

	u64 kentry; /* Kernel entry time stamp (for profiling) */
	/*
	 * pcycles: cycles spent in this process (updated on procsave/restore)
	 * when this is the current proc and we're in the kernel
	 * (procrestores outnumber procsaves by one)
	 * the number of cycles spent in the proc is pcycles + cycles()
	 * when this is not the current process or we're in user mode
	 * (procrestores and procsaves balance), it is pcycles.
	 */
	i64 pcycles;

	int insyscall;

	QLock debug;	     /* to access debugging elements of User */
	Proc *pdbg;	     /* the debugging process */
	u32 procmode;   /* proc device file mode */
	u32 privatemem; /* proc does not let anyone read mem */
	int hang;	     /* hang at next exec for debug */
	int procctl;	     /* Control for /proc debugging */
	usize pc;	     /* DEBUG only */

	Lock rlock;	 /* sync sleep/wakeup with postnote */
	Rendez *r;	 /* rendezvous point slept on */
	Rendez sleep;	 /* place for syssleep/debug/tsleep */
	int notepending; /* note issued but not acted on */
	int kp;		 /* true if a kernel process */
	Proc *palarm;	 /* Next alarm time */
	u32 alarm;	 /* Time of call */
	int newtlb;	 /* Pager has changed my pte's, I must flush */
	int noswap;	 /* process is not swappable */

	usize rendtag; /* Tag for rendezvous */
	usize rendval; /* Value for rendezvous */
	Proc *rendhash;	 /* Hash list for tag values */

	Timer Timer; /* For tsleep and real-time */
	Rendez *trend;
	int (*tfn)(void *);
	void (*kpfun)(void *);
	void *kparg;

	int scallnr;				       /* system call number */
	unsigned char arg[MAXSYSARG * sizeof(void *)]; /* system call arguments */
	int nerrlab;
	Label errlab[NERR];
	char *syserrstr; /* last error from a system call, errbuf0 or 1 */
	char *errstr;	 /* reason we're unwinding the error stack, errbuf1 or 0 */
	char errbuf0[ERRMAX];
	char errbuf1[ERRMAX];
	char genbuf[128]; /* buffer used e.g. for last name element from namec */
	Chan *slash;
	Chan *dot;

	Note note[NNOTE];
	short nnote;
	short notified; /* sysnoted is due */
	Note lastnote;
	void (*notify)(void *, char *);

	Lock *lockwait;
	Lock *lastlock;	 /* debugging */
	Lock *lastilock; /* debugging */

	Mach *wired;
	Mach *mp;   /* machine this process last ran on */
	int nlocks; /* number of locks held by proc */
	u32 delaysched;
	u32 priority; /* priority level */
	u32 basepri;  /* base priority level */
	int fixedpri;	   /* priority level does not change */
	u64 cpu;	   /* cpu average */
	u64 lastupdate;
	u64 readytime; /* time process came ready */
	u64 movetime;  /* last time process switched processors */
	int preempted;	    /* true if this process hasn't finished the interrupt
				 *  that last preempted it
				 */
	Edf *edf;	    /* if non-null, real-time proc, edf contains scheduling params */
	int trace;	    /* process being traced? */

	usize qpc; /* pc calling last blocking qlock */

	int setargs;

	void *ureg; /* User registers for notes */
	int color;

	Fastcall *fc;
	int fcount;
	char *syscalltrace;

	/* NIX */
	Mach *ac;
	Page *acPageTableRoot;
	int prepagemem;
	Nixpctl *nixpctl; /* NIX queue based system calls */

	u32 ntrap;	  /* # of traps while in this process */
	u32 nintr;	  /* # of intrs while in this process */
	u32 nsyscall;	  /* # of syscalls made by the process */
	u32 nactrap;	  /* # of traps in the AC for this process */
	u32 nacsyscall;  /* # of syscalls in the AC for this process */
	u32 nicc;	  /* # of ICCs for the process */
	u64 actime1; /* ticks as of last call in AC */
	u64 actime;  /* ∑time from call in AC to ret to AC, and... */
	u64 tctime;  /* ∑time from call received to call handled */
	int nqtrap;	  /* # of traps in last quantum */
	int nqsyscall;	  /* # of syscalls in the last quantum */
	int nfullq;

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

	/* plan 9 binaries */
	int plan9;
};

struct Procalloc {
	Lock l;
	int nproc;
	Proc *ht[128];
	Proc *arena;
	Proc *free;
};

enum {
	PRINTSIZE = 256,
	MAXCRYPT = 127,
	NUMSIZE = 12, /* size of formatted number */
	MB = (1024 * 1024),
	/* READSTR was 1000, which is way too small for usb's ctl file */
	READSTR = 4000, /* temporary buffer size for device reads */
	WRITESTR = 256, /* ctl file write max */
};

extern Conf conf;
extern char *cputype;
extern int cpuserver;
extern char *eve;
extern char hostdomain[];
extern u8 initcode[];
extern int kbdbuttons;
extern Ref noteidalloc;
extern int nphysseg;
extern int nsyscall;
extern Pgalloc pga;
extern Physseg physseg[];
extern Procalloc procalloc;
extern u32 qiomaxatomic;
extern char *statename[];
extern char *sysname;

typedef struct Systab Systab;
struct Systab {
	char *n;
	void (*f)(Ar0 *, ...);
	Ar0 r;
};

extern Systab systab[];

enum {
	LRESPROF = 3,
};

/*
 *  action log
 */
struct Log {
	Lock l;
	int opens;
	char *buf;
	char *end;
	char *rptr;
	int len;
	int nlog;
	int minread;

	int logmask; /* mask of things to debug */

	QLock readq;
	Rendez readr;
};

struct Logflag {
	char *name;
	int mask;
};

enum {
	NCMDFIELD = 128
};

struct Cmdbuf {
	char *buf;
	char **f;
	int nf;
};

struct Cmdtab {
	int index; /* used by client to switch on result */
	char *cmd; /* command name */
	int narg;  /* expected #args; 0 ==> variadic */
};

/*
 *  routines to access UART hardware
 */
struct PhysUart {
	char *name;
	Uart *(*pnp)(void);
	void (*enable)(Uart *, int);
	void (*disable)(Uart *);
	void (*kick)(Uart *);
	void (*dobreak)(Uart *, int);
	int (*baud)(Uart *, int);
	int (*bits)(Uart *, int);
	int (*stop)(Uart *, int);
	int (*parity)(Uart *, int);
	void (*modemctl)(Uart *, int);
	void (*rts)(Uart *, int);
	void (*dtr)(Uart *, int);
	i32 (*status)(Uart *, void *, i32, i32);
	void (*fifo)(Uart *, int);
	void (*power)(Uart *, int);
	int (*getc)(Uart *);	   /* polling version for rdb */
	void (*putc)(Uart *, int); /* polling version for iprint */
	void (*poll)(Uart *);	   /* polled interrupt routine */
};

enum {
	Stagesize = 2048
};

/*
 *  software UART
 */
struct Uart {
	void *regs;	/* hardware stuff */
	void *saveregs; /* place to put registers on power down */
	char *name;	/* internal name */
	u32 freq;	/* clock frequency */
	int bits;	/* bits per character */
	int stop;	/* stop bits */
	int parity;	/* even, odd or no parity */
	int baud;	/* baud rate */
	PhysUart *phys;
	int console; /* used as a serial console */
	int special; /* internal kernel device */
	Uart *next;  /* list of allocated uarts */

	QLock ql;
	int type; /* ?? */
	int dev;
	int opens;

	int enabled;
	Uart *elist; /* next enabled interface */

	int perr; /* parity errors */
	int ferr; /* framing errors */
	int oerr; /* rcvr overruns */
	int berr; /* no input buffers */
	int serr; /* input queue overflow */

	/* buffers */
	int (*putc)(Queue *, int);
	Queue *iq;
	Queue *oq;

	Lock rlock;
	unsigned char istage[Stagesize];
	unsigned char *iw;
	unsigned char *ir;
	unsigned char *ie;

	Lock tlock; /* transmit */
	unsigned char ostage[Stagesize];
	unsigned char *op;
	unsigned char *oe;
	int drain;

	int modem;  /* hardware flow control on */
	int xonoff; /* software flow control on */
	int blocked;
	int cts, dsr, dcd; /* keep track of modem status */
	int ctsbackoff;
	int hup_dsr, hup_dcd; /* send hangup upstream? */
	int dohup;

	Rendez rend;
};

extern Uart *consuart;

/*
 *  performance timers, all units in perfticks
 */
struct Perf {
	u64 intrts;     /* time of last interrupt */
	u64 inintr;     /* time since last clock tick in interrupt handlers */
	u64 avg_inintr; /* avg time per clock tick in interrupt handlers */
	u64 inidle;     /* time since last clock tick in idle loop */
	u64 avg_inidle; /* avg time per clock tick in idle loop */
	u64 last;	     /* value of perfticks() at last clock tick */
	u64 period;     /* perfticks() per clock tick */
};

struct Watchdog {
	void (*enable)(void);	      /* watchdog enable */
	void (*disable)(void);	      /* watchdog disable */
	void (*restart)(void);	      /* watchdog restart */
	void (*stat)(char *, char *); /* watchdog statistics */
};

struct Watermark {
	int highwater;
	int curr;
	int max;
	int hitmax; /* count: how many times hit max? */
	char *name;
};

/* queue state bits,  Qmsg, Qcoalesce, and Qkick can be set in qopen */
enum {
	/* Queue.state */
	Qstarve = (1 << 0),   /* consumer starved */
	Qmsg = (1 << 1),      /* message stream */
	Qclosed = (1 << 2),   /* queue has been closed/hungup */
	Qflow = (1 << 3),     /* producer flow controlled */
	Qcoalesce = (1 << 4), /* coallesce packets on read */
	Qkick = (1 << 5),     /* always call the kick routine after qwrite */
};

/* Fast system call struct  -- these are dynamically allocted in Proc struct */
struct Fastcall {
	int scnum;
	Chan *c;
	void (*fun)(Ar0 *, Fastcall *);
	void *buf;
	int n;
	i64 offset;
};

#define DEVDOTDOT -1
