typedef struct Alarms	Alarms;
typedef struct Block	Block;
typedef struct Blist	Blist;
typedef struct Chan	Chan;
typedef struct Dev	Dev;
typedef struct Dirtab	Dirtab;
typedef struct Egrp	Egrp;
typedef struct Evalue	Evalue;
typedef struct Etherpkt	Etherpkt;
typedef struct Fgrp	Fgrp;
typedef struct Image	Image;
typedef struct IOQ	IOQ;
typedef struct KIOQ	KIOQ;
typedef struct List	List;
typedef struct Mount	Mount;
typedef struct Mnt	Mnt;
typedef struct Mhead	Mhead;
typedef struct Netinf	Netinf;
typedef struct Netprot	Netprot;
typedef struct Network	Network;
typedef struct Note	Note;
typedef struct Page	Page;
typedef struct Palloc	Palloc;
typedef struct Parallax	Parallax;
typedef struct Pgrps	Pgrps;
typedef struct Pgrp	Pgrp;
typedef struct Physseg	Physseg;
typedef struct Proc	Proc;
typedef struct Pte	Pte;
typedef struct Qinfo	Qinfo;
typedef struct QLock	QLock;
typedef struct Queue	Queue;
typedef struct Ref	Ref;
typedef struct Rendez	Rendez;
typedef struct RWlock	RWlock;
typedef struct Sargs	Sargs;
typedef struct Session	Session;
typedef struct Scsi	Scsi;
typedef struct Scsibuf	Scsibuf;
typedef struct Scsidata	Scsidata;
typedef struct Segment	Segment;
typedef struct Stream	Stream;
typedef struct Talarm	Talarm;
typedef struct Waitq	Waitq;
typedef int    Devgen(Chan*, Dirtab*, int, int, Dir*);
typedef	void   Streamput(Queue*, Block*);
typedef	void   Streamopen(Queue*, Stream*);
typedef	void   Streamclose(Queue*);
typedef	void   Streamreset(void);

#include <auth.h>
#include <fcall.h>

struct Ref
{
	Lock;
	long	ref;
};

struct Rendez
{
	Lock;
	Proc	*p;
};

struct QLock
{
	Lock	use;			/* to access Qlock structure */
	Proc	*head;			/* next process waiting for object */
	Proc	*tail;			/* last process waiting for object */
	int	locked;			/* flag */
};

struct RWlock
{
	Lock;				/* Lock modify lock */
	QLock	x;			/* Mutual exclusion lock */
	QLock	k;			/* Lock for waiting writers held for readers */
	int	readers;		/* Count of readers in lock */
};

struct Talarm
{
	Lock;
	Proc	*list;
};

struct Alarms
{
	QLock;
	Proc	*head;
};

#define MAXSYSARG	5		/* for mount(fd, mpt, flag, arg, srv) */
struct Sargs
{
	ulong	args[MAXSYSARG];
};

/* Block.flags */
#define S_DELIM 0x80
#define S_CLASS 0x07

/* Block.type */
#define M_DATA 0
#define M_CTL 1
#define M_HANGUP 2

struct Block
{
	Block	*next;
	Block	*list;			/* chain of block lists */
	uchar	*rptr;			/* first unconsumed byte */
	uchar	*wptr;			/* first empty byte */
	uchar	*lim;			/* 1 past the end of the buffer */
	uchar	*base;			/* start of the buffer */
	uchar	flags;
	uchar	type;
	ulong	pc;			/* pc of caller */
};

struct Blist
{
	Lock;
	Block	*first;			/* first data block */
	Block	*last;			/* last data block */
	long	len;			/* length of list in bytes */
	int	nb;			/* number of blocks in list */
};

/*
 * Access types in namec
 */
enum
{
	Aaccess,			/* as in access, stat */
	Atodir,				/* as in chdir */
	Aopen,				/* for i/o */
	Amount,				/* to be mounted upon */
	Acreate,			/* file is to be created */
};

/*
 *  Chan.flags
 */
#define	COPEN	1			/* for i/o */
#define	CMSG	2			/* is the message channel for a mount */
#define	CCREATE	4			/* permits creation if c->mnt */
#define	CCEXEC	8			/* close on exec */
#define	CFREE	16			/* not in use */
#define	CRCLOSE	32			/* remove on close */

struct Chan
{
	Ref;
	union{
		Chan	*next;		/* allocation */
		ulong	offset;		/* in file */
	};
	ushort	type;
	ulong	dev;
	ushort	mode;			/* read/write */
	ushort	flag;
	Qid	qid;
	Mount	*mnt;			/* mount point that derived Chan */
	ulong	mountid;
	int	fid;			/* for devmnt */
	Stream	*stream;		/* for stream channels */
	union {
		void	*aux;
		Qid	pgrpid;		/* for #p/notepg */
		Mnt	*mntptr;	/* for devmnt */
	};
	Chan	*mchan;			/* channel to mounted server */
	Qid	mqid;			/* qid of root of mount point */
	Session	*session;
};

struct Dev
{
	void	(*reset)(void);
	void	(*init)(void);
	Chan*	(*attach)(char*);
	Chan*	(*clone)(Chan*, Chan*);
	int	(*walk)(Chan*, char*);
	void	(*stat)(Chan*, char*);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, ulong);
	void	(*close)(Chan*);
	long	(*read)(Chan*, void*, long, ulong);
	long	(*write)(Chan*, void*, long, ulong);
	void	(*remove)(Chan*);
	void	(*wstat)(Chan*, char*);
};

struct Dirtab
{
	char	name[NAMELEN];
	Qid	qid;
	long	length;
	long	perm;
};

/*
 *  Ethernet packet buffers.
 */
struct Etherpkt
{
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];
	uchar	data[1500];
	uchar	crc[4];
};

enum
{
	ETHERMINTU =	64,		/* minimum transmit size */
	ETHERMAXTU =	1514,		/* maximum transmit size */
	ETHERHDRSIZE =	14,		/* size of an ethernet header */
};

/* SCSI devices. */
enum
{
	ScsiTestunit	= 0x00,
	ScsiExtsens	= 0x03,
	ScsiInquiry	= 0x12,
	ScsiModesense	= 0x1a,
	ScsiStartunit	= 0x1B,
	ScsiStopunit	= 0x1B,
	ScsiGetcap	= 0x25,
	ScsiRead	= 0x08,
	ScsiWrite	= 0x0a,
	ScsiExtread	= 0x28,
	ScsiExtwrite	= 0x2a,

	/* data direction */
	ScsiIn		= 1,
	ScsiOut		= 0,
};

struct Scsibuf
{
	void*		virt;
	void*		phys;
	Scsibuf*	next;
};

struct Scsidata
{
	uchar*		base;
	uchar*		lim;
	uchar*		ptr;
};

struct Scsi
{
	QLock;
	int		bus;
	ulong		pid;
	ushort		target;
	ushort		lun;
	ushort		rflag;
	ushort		status;
	Scsidata 	cmd;
	Scsidata 	data;
	Scsibuf*	b;
	uchar*		save;
	uchar		cmdblk[16];
};

/* character based IO (mouse, keyboard, console screen) */
#define NQ	4096
struct IOQ
{
	Lock;
	uchar	buf[NQ];
	uchar	*in;
	uchar	*out;
	int	state;
	Rendez	r;
	union{
		void	(*puts)(IOQ*, void*, int);	/* output */
		int	(*putc)(IOQ*, int);		/* input */
	};
	union{
		int	(*gets)(IOQ*, void*, int);	/* input */
		int	(*getc)(IOQ*);			/* output */
	};
	void	*ptr;
};

struct KIOQ
{
	QLock;
	IOQ;
	int	repeat;
	int	c;
	int	count;
};

struct Mount
{
	ulong	mountid;
	Mount	*next;
	Mhead	*head;
	Chan	*to;			/* channel replacing underlying channel */
};

struct Mhead
{
	Chan	*from;			/* channel mounted upon */
	Mount	*mount;			/* what's mounted upon it */
	Mhead	*hash;			/* Hash chain */
};

enum
{
	NUser,				/* note provided externally */
	NExit,				/* deliver note quietly */
	NDebug,				/* print debug message */
};

struct Note
{
	char	msg[ERRLEN];
	int	flag;			/* whether system posted it */
};

enum
{
	PG_NOFLUSH	= 0,		/* Fields for cache control of pages */
	PG_TXTFLUSH	= 1,
	PG_DATFLUSH	= 2,
	PG_MOD		= 0x01,		/* Simulated modified and referenced bits */
	PG_REF		= 0x02,
};

struct Page
{
	Lock;
	ulong	pa;			/* Physical address in memory */
	ulong	va;			/* Virtual address for user */
	ulong	daddr;			/* Disc address on swap */
	ushort	ref;			/* Reference count */
	char	modref;			/* Simulated modify/reference bits */
	char	cachectl[MAXMACH];	/* Cache flushing control for putmmu */
	Image	*image;			/* Associated text or swap image */
	Page	*next;			/* Lru free list */
	Page	*prev;
	Page	*hash;			/* Image hash chains */
};

struct Swapalloc
{
	Lock;				/* Free map lock */
	int	free;			/* Number of currently free swap pages */
	uchar	*swmap;			/* Base of swap map in memory */
	uchar	*alloc;			/* Round robin allocator */
	uchar	*last;			/* Speed swap allocation */
	uchar	*top;			/* Top of swap map */
	Rendez	r;			/* Pager kproc idle sleep */
	ulong	highwater;		/* Threshold beyond which we must page */
	ulong	headroom;		/* Space pager keeps free under highwater */
}swapalloc;

struct Image
{
	Ref;
	Chan	*c;			/* Channel associated with running image */
	Qid 	qid;			/* Qid for page cache coherence checks */
	Qid	mqid;
	Chan	*mchan;
	ushort	type;			/* Device type of owning channel */
	Segment *s;			/* TEXT segment for image if running */
	Image	*hash;			/* Qid hash chains */
	Image	*next;			/* Free list */
};

struct Pte
{
	Page	*pages[PTEPERTAB];	/* Page map for this chunk of pte */
	Page	**first;		/* First used entry */
	Page	**last;			/* Last used entry */
	Pte	*next;			/* Free list */
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
	SG_SHDATA	= 06,

	SG_RONLY	= 040,		/* Segment is read only */
	SG_CEXEC	= 0100,		/* Detach at exec */
};

#define PG_ONSWAP	1
#define onswap(s)	(((ulong)s)&PG_ONSWAP)
#define pagedout(s)	(((ulong)s)==0 || onswap(s))
#define swapaddr(s)	(((ulong)s)&~PG_ONSWAP)

#define SEGMAXSIZE	(SEGMAPSIZE*PTEMAPMEM)

struct Physseg
{
	ulong	attr;			/* Segment attributes */
	char	*name;			/* Attach name */
	ulong	pa;			/* Physical address */
	ulong	size;			/* Maximum segment size in pages */
	Page	*(*pgalloc)(Segment*, ulong);	/* Allocation if we need it */
	void	(*pgfree)(Page*);
};

struct Segment
{
	Ref;
	QLock	lk;
	ushort	steal;		/* Page stealer lock */
	Segment	*next;		/* free list pointers */
	ushort	type;		/* segment type */
	ulong	base;		/* virtual base */
	ulong	top;		/* virtual top */
	ulong	size;		/* size in pages */
	ulong	fstart;		/* start address in file for demand load */
	ulong	flen;		/* length of segment in file */
	int	flushme;	/* maintain consistent icache for this segment */
	Image	*image;		/* text in file attached to this segment */
	Physseg *pseg;
	Pte	*map[SEGMAPSIZE];	/* segment pte map */
};

enum
{
	RENDHASH =	32,		/* Hash to lookup rendezvous tags */
	MNTHASH	=	32,		/* Hash to walk mount table */
	NFD =		100,		/* Number of per process file descriptors */
	PGHLOG  =	9,
	PGHSIZE	=	1<<PGHLOG,	/* Page hash for image lookup */
};
#define REND(p,s)	((p)->rendhash[(s)%RENDHASH])
#define MOUNTH(p,s)	((p)->mnthash[(s)->qid.path%MNTHASH])

struct Pgrp
{
	Ref;				/* also used as a lock when mounting */
	Pgrp	*next;			/* free list */
	ulong	pgrpid;
	QLock	debug;			/* single access via devproc.c */
	RWlock	ns;			/* Namespace many read/one write lock */
	Mhead	*mnthash[MNTHASH];
	Proc	*rendhash[RENDHASH];	/* Rendezvous tag hash */
};

struct Egrp
{
	Ref;
	QLock;
	Evalue	*entries;
	ulong	path;
};

struct Evalue
{
	char	*name;
	char	*value;
	int	len;
	ulong	path;
	Evalue	*link;
};

struct Fgrp
{
	Ref;
	Chan	*fd[NFD];
	int	maxfd;			/* highest fd in use */
};

struct Palloc
{
	Lock;
	ulong	p0, p1;			/* base of pages in bank 0/1 */
	ulong	np0, np1;		/* number of pages in bank 0/1 */
	Page	*head;			/* most recently used */
	Page	*tail;			/* least recently used */
	ulong	freecount;		/* how many pages on free list now */
	ulong	user;			/* how many user pages */
	Page	*hash[PGHSIZE];
	Lock	hashlock;
	Rendez	r;			/* Sleep for free mem */
	QLock	pwait;			/* Queue of procs waiting for memory */
	int	wanted;			/* Do the wakeup at free */
	ulong	cmembase;		/* Key memory protected from read by devproc */
	ulong	cmemtop;
};

struct Waitq
{
	Waitmsg	w;
	Waitq	*next;
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
	RFCFDG		= (1<<12)
};

/*
 *  process memory segments - NSEG always last !
 */
enum
{
	SSEG, TSEG, DSEG, BSEG, ESEG, LSEG, SEG1, SEG2, NSEG
};

enum
{
	Dead = 0,		/* Process states */
	Moribund,
	Ready,
	Scheding,
	Running,
	Queueing,
	Wakeme,
	Broken,
	Stopped,
	Rendezvous,

	Proc_stopme = 1, 	/* devproc requests */
	Proc_exitme,
	Proc_traceme,

	TUser = 0, 		/* Proc.time */
	TSys,
	TReal,
	TCUser,
	TCSys,
	TCReal,

	Nrq		= 20,	/* number of scheduler priority levels */
	PriNormal	= 10,	/* base priority for normal processes */
	PriKproc	= 13,	/* base priority for kernel processes */
	PriRoot		= 13,	/* base priority for root processes */
};

struct Proc
{
	Label	sched;
	Mach	*mach;			/* machine running this proc */
	char	text[NAMELEN];
	char	user[NAMELEN];
	Proc	*rnext;			/* next process in run queue */
	Proc	*qnext;			/* next process on queue for a QLock */
	QLock	*qlock;			/* address of qlock being queued for DEBUG */
	ulong	qlockpc;		/* pc of last call to qlock */
	int	state;
	char	*psstate;		/* What /proc/#/status reports */
	Page	*upage;			/* page from palloc */
	Segment	*seg[NSEG];
	ulong	pid;
	ulong	noteid;			/* Equivalent of note group */

	Lock	exl;			/* Lock count and waitq */
	Waitq	*waitq;			/* Exited processes wait children */
	int	nchild;			/* Number of living children */
	int	nwait;			/* Number of uncollected wait records */
	QLock	qwaitr;
	Rendez	waitr;			/* Place to hang out in wait */
	Proc	*parent;

	Pgrp	*pgrp;			/* Process group for notes and namespace */
	Egrp 	*egrp;			/* Environment group */
	Fgrp	*fgrp;			/* File descriptor group */

	ulong	parentpid;
	ulong	time[6];		/* User, Sys, Real; child U, S, R */
	short	insyscall;
	int	fpstate;

	QLock	debug;			/* to access debugging elements of User */
	Proc	*pdbg;			/* the debugging process */
	ulong	procmode;		/* proc device file mode */
	int	hang;			/* hang at next exec for debug */
	int	procctl;		/* Control for /proc debugging */
	ulong	pc;			/* DEBUG only */

	Rendez	*r;			/* rendezvous point slept on */
	Rendez	sleep;			/* place for syssleep/debug */
	int	notepending;		/* note issued but not acted on */
	int	kp;			/* true if a kernel process */
	Proc	*palarm;		/* Next alarm time */
	ulong	alarm;			/* Time of call */
	int 	hasspin;		/* I hold a spin lock */
	int	newtlb;			/* Pager has touched my tables so I must flush */

	ulong	rendtag;		/* Tag for rendezvous */ 
	ulong	rendval;		/* Value for rendezvous */
	Proc	*rendhash;		/* Hash list for tag values */

	ulong	twhen;
	Rendez	*trend;
	Proc	*tlink;
	int	(*tfn)(void*);

	Mach	*mp;		/* machine this process last ran on */
	ulong	priority;	/* priority level */
	ulong	basepri;	/* base priority level */
	ulong	rt;		/* # ticks used since last blocked */
	ulong	art;		/* avg # ticks used since last blocked */
	ulong	movetime;	/* last time process switched processors */
	ulong	readytime;	/* time process went ready */

	/*
	 *  machine specific MMU
	 */
	PMMU;
};

/*
 *  operations available to a queue
 */
struct Qinfo
{
	Streamput	*iput;		/* input routine */
	Streamput	*oput;		/* output routine */
	Streamopen	*open;
	Streamclose	*close;
	char		*name;
	Streamreset	*reset;		/* initialization */
	char		nodelim;	/* True if stream does not preserve delimiters */
	Qinfo		*next;
};

/* Queue.flag */
enum
{
	QHUNGUP	=	0x1,	/* stream has been hung up */
	QINUSE =	0x2,	/* allocation check */
	QHIWAT =	0x4,	/* queue has gone past the high water mark */	
	QDEBUG =	0x8,
};

struct Queue
{
	Blist;
	int	flag;
	Qinfo	*info;			/* line discipline definition */
	Queue	*other;			/* opposite direction, same line discipline */
	Queue	*next;			/* next queue in the stream */
	void	(*put)(Queue*, Block*);
	QLock	rlock;			/* mutex for processes sleeping at r */
	Rendez	r;			/* standard place to wait for flow control */
	Rendez	*rp;			/* where flow control wakeups go to */
	void	*ptr;			/* private info for the queue */
};

struct Stream
{
	QLock;				/* structure lock */
	Stream	*next;
	short	inuse;			/* number of processes in stream */
	short	opens;			/* number of processes with stream open */
	ushort	hread;			/* number of reads after hangup */
	ushort	type;			/* correlation with Chan */
	ushort	dev;			/* ... */
	ulong	id;			/* ... */
	QLock	rdlock;			/* read lock */
	Queue	*procq;			/* write queue at process end */
	Queue	*devq;			/* read queue at device end */
	Block	*err;			/* error message from down stream */
	int	flushmsg;		/* flush up till the next delimiter */
};

/*
 *  useful stream macros
 */
#define	RD(q)		((q)->other < (q) ? (q->other) : q)
#define	WR(q)		((q)->other > (q) ? (q->other) : q)
#define STREAMTYPE(x)	((x)&0x1f)
#define STREAMID(x)	(((x)&~CHDIR)>>5)
#define STREAMQID(i,t)	(((i)<<5)|(t))
#define PUTTHIS(q,b)	(*(q)->put)((q), b)
#define PUTNEXT(q,b)	(*(q)->next->put)((q)->next, b)
#define BLEN(b)		((b)->wptr - (b)->rptr)
#define QFULL(q)	((q)->flag & QHIWAT)
#define FLOWCTL(q,b)	{ if(QFULL(q->next)) flowctl(q,b); else PUTNEXT(q,b);}

/*
 *  stream file qid's & high water mark
 */
enum
{
	Shighqid	= STREAMQID(1,0) - 1,
	Sdataqid	= Shighqid,
	Sctlqid		= Sdataqid-1,
	Slowqid		= Sctlqid,
	Streamhi	= (32*1024),	/* byte count high water mark */
	Streambhi	= 128,		/* block count high water mark */
};

/*
 *  a multiplexed network
 */
struct Netprot
{
	int	id;
	Netprot	*next;		/* linked list of protections */
	ulong	mode;
	char	owner[NAMELEN];
};

struct Netinf
{
	char	*name;
	void	(*fill)(Chan*, char*, int);
};

struct Network
{
	Lock;
	char	*name;
	int	nconv;			/* max # of conversations */
	Qinfo	*devp;			/* device end line disc */
	Qinfo	*protop;		/* protocol line disc */
	int	(*listen)(Chan*);
	int	(*clone)(Chan*);
	int	ninfo;
	Netinf	info[5];
	Netprot	*prot;			/* linked list of protections */
};

/*
 *  Parallax for pen pointers
 */
struct Parallax
{
	int	xoff;
	int	xnum;
	int	xdenom;

	int	yoff;
	int	ynum;
	int	ydenom;
};

enum
{
	PRINTSIZE =	256,
	MAXCRYPT = 	127,
	NUMSIZE	=	12,		/* size of formatted number */
	MB =		(1024*1024),
};

extern	Conf	conf;
extern	char*	conffile;
extern	int	cpuserver;
extern	Rune*	devchar;
extern	Dev	devtab[];
extern  char	eve[];
extern	char	hostdomain[];
extern	int	hwcurs;
extern	uchar	initcode[];
extern	FPsave	initfp;
extern  KIOQ	kbdq;
extern  IOQ	lineq;
extern  IOQ	mouseq;
extern  Ref	noteidalloc;
extern	int	nrdy;
extern  IOQ	printq;
extern	char*	statename[];
extern  Image	swapimage;
extern	char	sysname[NAMELEN];
extern	Talarm	talarm;
extern	Palloc 	palloc;
extern	int	cpuserver;
extern	Physseg physseg[];

enum
{
	CHDIR =		0x80000000L,
	CHAPPEND = 	0x40000000L,
	CHEXCL =	0x20000000L,
	CHMOUNT	=	0x10000000L,
};

/*
 * auth messages
 */
enum
{
	FScchal	= 1,
	FSschal,
	FSok,
	FSctick,
	FSstick,
	FSerr,

	RXschal	= 0,
	RXstick	= 1,
};

/*
 *  mouse types
 */
enum
{
	Mouseother=	0,
	Mouseserial=	1,
	MousePS2=	2,
	Mousekurta=	3,
};
extern int mouseshifted;
extern int mousetype;
extern int mouseswap;
