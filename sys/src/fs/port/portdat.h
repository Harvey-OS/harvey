/*
 * fundamental constants
 */
#define SUPER_ADDR	2		/* address of superblock */
#define ROOT_ADDR	3		/* address of superblock */
#define	NAMELEN		28		/* size of names */
#define	NDBLOCK		6		/* number of direct blocks in Dentry */
#define	MAXDAT		8192		/* max allowable data message */
#define	MAXMSG		128		/* max size protocol message sans data */
#define	OFFMSG		60		/* offset of msg in buffer */
#define NDRIVE		16		/* size of drive structure */
#define NTLOCK		200		/* number of active file Tlocks */
#define	LRES		3		/* profiling resolution */
#define NATTID		10		/* the last 10 ID's in attaches */
#define	C0a		59		/* time constants for filters */
#define	C0b		60
#define	C1a		599
#define	C1b		600
#define	C2a		5999
#define	C2b		6000

/*
 * derived constants
 */
#define	BUFSIZE		(RBUFSIZE-sizeof(Tag))
#define DIRPERBUF	(BUFSIZE/sizeof(Dentry))
#define INDPERBUF	(BUFSIZE/sizeof(long))
#define INDPERBUF2	(INDPERBUF*INDPERBUF)
#define FEPERBUF	((BUFSIZE-sizeof(Super1)-sizeof(long))/sizeof(long))
#define	SMALLBUF	(MAXMSG)
#define	LARGEBUF	(MAXMSG+MAXDAT+256)
#define	RAGAP		(300*1024)/BUFSIZE		/* readahead parameter */
#define CEPERBK		((BUFSIZE-BKPERBLK*sizeof(long))/\
				(sizeof(Centry)*BKPERBLK))
#define	BKPERBLK	10

typedef struct	Alarm	Alarm;
typedef struct	Auth	Auth;
typedef	struct	Conf	Conf;
typedef	struct	Label	Label;
typedef	struct	Lock	Lock;
typedef	struct	Mach	Mach;
typedef	struct	QLock	QLock;
typedef	struct	Ureg	Ureg;
typedef	struct	User	User;
typedef	struct	Fbuf	Fbuf;
typedef	struct	Super1	Super1;
typedef	struct	Superb	Superb;
typedef	struct	Filsys	Filsys;
typedef	struct	Startsb	Startsb;
typedef	struct	Dentry	Dentry;
typedef	struct	Tag	Tag;
typedef struct  Talarm	Talarm;
typedef	struct	Uid	Uid;
typedef struct	Device	Device;
typedef struct	Qid9p1	Qid9p1;
typedef	struct	Iobuf	Iobuf;
typedef	struct	Wpath	Wpath;
typedef	struct	File	File;
typedef	struct	Chan	Chan;
typedef	struct	Cons	Cons;
typedef	struct	Time	Time;
typedef	struct	Tm	Tm;
typedef	struct	Rtc	Rtc;
typedef	struct	Hiob	Hiob;
typedef	struct	RWlock	RWlock;
typedef	struct	Msgbuf	Msgbuf;
typedef	struct	Queue	Queue;
typedef	struct	Command	Command;
typedef	struct	Flag	Flag;
typedef	struct	Bp	Bp;
typedef	struct	Rabuf	Rabuf;
typedef	struct	Rendez	Rendez;
typedef	struct	Filter	Filter;
typedef		ulong	Float;
typedef	struct	Tlock	Tlock;
typedef	struct	Cache	Cache;
typedef	struct	Centry	Centry;
typedef	struct	Bucket	Bucket;

struct	Lock
{
	ulong*	sbsem;		/* addr of sync bus semaphore */
	ulong	pc;
	ulong	sr;
};

struct	Rendez
{
	Lock;
	User*	p;
};

struct	Filter
{
	ulong	count;			/* count and old count kept separate */
	ulong	oldcount;		/*	so interrput can read them */
	int	c1;			/* time const multiplier */
	int	c2;			/* time const divider */
	int	c3;			/* scale for printing */
	Float	filter;			/* filter */ 
};

struct	QLock
{
	Lock;			/* to use object */
	User*	head;		/* next process waiting for object */
	User*	tail;		/* last process waiting for object */
	char*	name;		/* for diagnostics */
	int	locked;		/* flag, is locked */
};

struct	RWlock
{
	int	nread;
	QLock	wr;
	QLock	rd;
};

/*
 * send/recv queue structure
 */
struct	Queue
{
	Lock;			/* to manipulate values */
	int	size;		/* size of queue */
	int	loc;		/* circular pointer */
	int	count;		/* how many in queue */
	User*	rhead;		/* process's waiting for send */
	User*	rtail;
	User*	whead;		/* process's waiting for recv */
	User*	wtail;
	void*	args[1];	/* list of saved pointers, [->size] */
};

struct	Tag
{
	short	pad;
	short	tag;
	long	path;
};

struct	Device
{
	uchar	type;
	uchar	init;
	Device*	link;			/* link for mcat/mlev */
	Device*	dlink;			/* link all devices */
	void*	private;
	long	size;
	union
	{
		struct			/* worm wren */
		{
			int	ctrl;
			int	targ;
			int	lun;
		} wren;
		struct			/* mcat mlev */
		{
			Device*	first;
			Device*	last;
			int	ndev;
		} cat;
		struct			/* cw */
		{
			Device*	c;
			Device*	w;
			Device*	ro;
		} cw;
		struct			/* juke */
		{
			Device*	j;
			Device*	m;
		} j;
		struct			/* ro */
		{
			Device*	parent;
		} ro;
		struct			/* fworm */
		{
			Device*	fw;
		} fw;
		struct			/* part */
		{
			Device*	d;
			long	base;
			long	size;
		} part;
		struct			/* part */
		{
			Device*	d;
		} swab;
	};
};

struct	Rabuf
{
	union
	{
		struct
		{
			Device*	dev;
			long	addr;
		};
		Rabuf*	link;
	};
};

typedef
struct Qid
{
	uvlong	path;
	ulong	vers;
	uchar	type;
} Qid;

/* bits in Qid.type */
#define QTDIR		0x80		/* type bit for directories */
#define QTAPPEND	0x40		/* type bit for append only files */
#define QTEXCL		0x20		/* type bit for exclusive use files */
#define QTMOUNT		0x10		/* type bit for mounted channel */
#define QTAUTH		0x08		/* type bit for authentication file */
#define QTFILE		0x00		/* plain file */

/* DONT TOUCH, this is the disk structure */
struct	Qid9p1
{
	long	path;
	long	version;
};

struct	Hiob
{
	Iobuf*	link;
	Lock;
};

struct	Chan
{
	char	type;			/* major driver type i.e. Dev* */
	int	(*protocol)(Msgbuf*);	/* version */
	int	msize;			/* version */
	char	whochan[50];
	char	whoname[NAMELEN];
	void	(*whoprint)(Chan*);
	ulong	flags;
	int	chan;			/* overall channel number, mostly for printing */
	int	nmsgs;			/* outstanding messages, set under flock -- for flush */
	ulong	whotime;
	Filter	work;
	Filter	rate;
	int	nfile;			/* used by cmd_files */
	RWlock	reflock;
	Chan*	next;			/* link list of chans */
	Queue*	send;
	Queue*	reply;

	uchar	authinfo[64];

	void*	ifc;
	void*	pdata;
};

struct	Filsys
{
	char*	name;			/* name of filsys */
	char*	conf;			/* symbolic configuration */
	Device*	dev;			/* device that filsys is on */
	int	flags;
		#define	FREAM		(1<<0)	/* mkfs */
		#define	FRECOVER	(1<<1)	/* install last dump */
		#define	FEDIT		(1<<2)	/* modified */
};

struct	Startsb
{
	char*	name;
	long	startsb;
};

struct	Time
{
	ulong	lasttoy;
	long	bias;
	long	offset;
};

/*
 * array of qids that are locked
 */
struct	Tlock
{
	Device*	dev;
	ulong	time;
	long	qpath;
	File*	file;
};

struct	Cons
{
	ulong	flags;		/* overall flags for all channels */
	QLock;			/* generic qlock for mutex */
	int	uid;		/* botch -- used to get uid on cons_create */
	int	gid;		/* botch -- used to get gid on cons_create */
	int	nuid;		/* number of uids */
	int	ngid;		/* number of gids */
	long	offset;		/* used to read files, c.f. fchar */
	int	chano;		/* generator for channel numbers */
	Chan*	chan;		/* console channel */
	Filsys*	curfs;		/* current filesystem */

	int	profile;	/* are we profiling? */
	long*	profbuf;
	ulong	minpc;
	ulong	maxpc;
	ulong	nprofbuf;

	long	nlarge;		/* number of large message buffers */
	long	nsmall;		/* ... small ... */
	long	nwormre;	/* worm read errors */
	long	nwormwe;	/* worm write errors */
	long	nwormhit;	/* worm read cache hits */
	long	nwormmiss;	/* worm read cache non-hits */
	int	noage;		/* dont update cache age, dump and check */
	long	nwrenre;	/* disk read errors */
	long	nwrenwe;	/* disk write errors */
	long	nreseq;		/* cache bucket resequence */

	Filter	work[3];	/* thruput in messages */
	Filter	rate[3];	/* thruput in bytes */
	Filter	bhit[3];	/* getbufs that hit */
	Filter	bread[3];	/* getbufs that miss and read */
	Filter	brahead[3];	/* messages to readahead */
	Filter	binit[3];	/* getbufs that miss and dont read */
};

struct	File
{
	QLock;
	Qid	qid;
	Wpath*	wpath;
	Chan*	cp;		/* null means a free slot */
	Tlock*	tlock;		/* if file is locked */
	File*	next;		/* in cp->flist */
	Filsys*	fs;
	long	addr;
	long	slot;
	long	lastra;		/* read ahead address */
	ulong	fid;
	short	uid;
	Auth	*auth;
	char	open;
		#define	FREAD	1
		#define	FWRITE	2
		#define	FREMOV	4

	long	doffset;	/* directory reading */
	ulong	dvers;
	long	dslot;
};

struct	Wpath
{
	Wpath*	up;		/* pointer upwards in path */
	long	addr;		/* directory entry addr */
	long	slot;		/* directory entry slot */
	short	refs;		/* number of files using this structure */
};

struct	Iobuf
{
	QLock;
	Device*	dev;
	Iobuf*	fore;		/* for lru */
	Iobuf*	back;		/* for lru */
	char*	iobuf;		/* only active while locked */
	char*	xiobuf;		/* "real" buffer pointer */
	long	addr;
	int	flags;
};

struct	Uid
{
	short	uid;		/* user id */
	short	lead;		/* leader of group */
	short	*gtab;		/* group table */
	int	ngrp;		/* number of group entries */
	char	name[NAMELEN];	/* user name */
};

/* DONT TOUCH, this is the disk structure */
struct	Dentry
{
	char	name[NAMELEN];
	short	uid;
	short	gid;
	ushort	mode;
		#define	DALLOC	0x8000
		#define	DDIR	0x4000
		#define	DAPND	0x2000
		#define	DLOCK	0x1000
		#define	DREAD	0x4
		#define	DWRITE	0x2
		#define	DEXEC	0x1
	short	muid;
	Qid9p1	qid;
	long	size;
	long	dblock[NDBLOCK];
	long	iblock;
	long	diblock;
	long	atime;
	long	mtime;
};

/* DONT TOUCH, this is the disk structure */
struct	Super1
{
	long	fstart;
	long	fsize;
	long	tfree;
	long	qidgen;		/* generator for unique ids */
	/*
	 * Stuff for WWC device
	 */
	long	cwraddr;	/* cfs root addr */
	long	roraddr;	/* dump root addr */
	long	last;		/* last super block addr */
	long	next;		/* next super block addr */
};

/* DONT TOUCH, this is the disk structure */
struct	Fbuf
{
	long	nfree;
	long	free[FEPERBUF];
};

/* DONT TOUCH, this is the disk structure */
struct	Superb
{
	Fbuf	fbuf;
	Super1;
};

struct	Label
{
	ulong	pc;
	ulong	sp;
};

struct	Alarm
{
	Lock;
	Alarm*	next;
	int	busy;
	int	dt;		/* in ticks */
	void	(*f)(Alarm*, void*);
	void*	arg;
};

struct Talarm
{
	Lock;
	User	*list;
};

struct	Conf
{
	ulong	nmach;		/* processors */
	ulong	nproc;		/* processes */
	ulong	mem;		/* total physical bytes of memory */
	ulong	sparemem;	/* memory left for check/dump and chans */
	ulong	nalarm;		/* alarms */
	ulong	nuid;		/* distinct uids */
	ulong	nserve;		/* server processes */
	ulong	nfile;		/* number of fid -- system wide */
	ulong	nwpath;		/* number of active paths, derrived from nfile */
	ulong	gidspace;	/* space for gid names -- derrived from nuid */
	ulong	nlgmsg;		/* number of large message buffers */
	ulong	nsmmsg;		/* number of small message buffers */
	ulong	wcpsize;	/* memory for worm copies */
	ulong	recovcw;	/* recover addresses */
	ulong	recovro;
	ulong	firstsb;
	ulong	recovsb;
	ulong	nauth;		/* number of Auth structs */
	uchar	nodump;		/* no periodic dumps */
	uchar	ripoff;
	uchar	dumpreread;	/* read and compare in dump copy */

	ulong	npage0;		/* total physical pages of memory */
	ulong	npage1;		/* total physical pages of memory */
	ulong	base0;		/* base of bank 0 */
	ulong	base1;		/* base of bank 1 */
};

/*
 * message buffers
 * 2 types, large and small
 */
struct	Msgbuf
{
	short	count;
	short	flags;
		#define	LARGE	(1<<0)
		#define	FREE	(1<<1)
		#define BFREE	(1<<2)
		#define BTRACE	(1<<7)
	Chan*	chan;
	Msgbuf*	next;
	ulong	param;
	int	category;
	uchar*	data;
	uchar*	xdata;
};

/*
 * message buffer categories
 */
enum
{
	Mxxx		= 0,
	Mbreply1,
	Mbreply2,
	Mbreply3,
	Mbreply4,
	Mbarp1,
	Mbarp2,
	Mbip1,
	Mbip2,
	Mbip3,
	Mbil1,
	Mbil2,
	Mbil3,
	Mbil4,
	Mbilauth,
	Maeth1,
	Maeth2,
	Maeth3,
	Mbeth1,
	Mbeth2,
	Mbeth3,
	Mbeth4,
	Mbsntp,
	MAXCAT,
};

struct	Mach
{
	int	machno;		/* physical id of processor */
	int	mmask;		/* 1<<m->machno */
	ulong	ticks;		/* of the clock since boot time */
	int	lights;		/* light lights, this processor */
	User*	proc;		/* current process on this processor */
	Label	sched;		/* scheduler wakeup */
	Lock	alarmlock;	/* access to alarm list */
	void*	alarm;		/* alarms bound to this clock */
	void	(*intr)(Ureg*, ulong);	/* pending interrupt */
	User*	intrp;		/* process that was interrupted */
	ulong	cause;		/* arg to intr */
	Ureg*	ureg;		/* arg to intr */
	uchar	stack[1];
};

#define	MAXSTACK 4000
#define	NHAS	100
struct	User
{
	Label	sched;
	Mach*	mach;		/* machine running this proc */
	User*	rnext;		/* next process in run queue */
	User*	qnext;		/* next process on queue for a QLock */
	void	(*start)(void);	/* startup function */
	char*	text;		/* name of this process */
	void*	arg;
	Filter	time[3];	/* cpu time used */
	int	exiting;
	int	pid;
	int	state;
	Rendez	tsleep;

	ulong	twhen;
	Rendez	*trend;
	User	*tlink;
	int	(*tfn)(void*);

	struct
	{
		QLock*	q[NHAS];/* list of locks this process has */
		QLock*	want;	/* lock waiting */
	} has;
	uchar	stack[MAXSTACK];
};

#define	PRINTSIZE	256
struct
{
	Lock;
	int	machs;
	int	exiting;
} active;

struct	Command
{
	char*	arg0;
	char*	help;
	void	(*func)(int, char*[]);
};

struct	Flag
{
	char*	arg0;
	char*	help;
	ulong	flag;
};

struct	Tm
{
	/* see ctime(3) */
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
	int	wday;
	int	yday;
	int	isdst;
};

struct	Rtc
{
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
};

/*
 * cw device
 */

/* DONT TOUCH, this is the disk structure */
struct	Cache
{
	long	maddr;		/* cache map addr */
	long	msize;		/* cache map size in buckets */
	long	caddr;		/* cache addr */
	long	csize;		/* cache size */
	long	fsize;		/* current size of worm */
	long	wsize;		/* max size of the worm */
	long	wmax;		/* highwater write */

	long	sbaddr;		/* super block addr */
	long	cwraddr;	/* cw root addr */
	long	roraddr;	/* dump root addr */

	long	toytime;	/* somewhere convienent */
	long	time;
};

/* DONT TOUCH, this is the disk structure */
struct	Centry
{
	ushort	age;
	short	state;
	long	waddr;		/* worm addr */
};

/* DONT TOUCH, this is the disk structure */
struct	Bucket
{
	long	agegen;		/* generator for ages in this bkt */
	Centry	entry[CEPERBK];
};

/*
 * scsi i/o
 */
enum
{
	SCSIread = 0,
	SCSIwrite = 1,
};

/*
 * Process states
 */
enum
{
	Dead = 0,
	Moribund,
	Zombie,
	Ready,
	Scheding,
	Running,
	Queueing,
	Sending,
	Recving,
	MMUing,
	Exiting,
	Inwait,
	Wakeme,
	Broken,
};

/*
 * Lights
 */
enum
{
	Lreal	= 0,	/* blink in clock interrupt */
	Lintr,		/* on while in interrupt */
	Lpanic,		/* in panic */
	Lcwmap,		/* in cw lookup */
};

/*
 * devnone block numbers
 */
enum
{
	Cwio1 	= 1,
	Cwio2,
	Cwxx1,
	Cwxx2,
	Cwxx3,
	Cwxx4,
	Cwdump1,
	Cwdump2,
	Cuidbuf,
	Cckbuf,
};

/*
 * error codes generated from the file server
 */
enum
{
	Ebadspc = 1,
	Efid,
	Echar,
	Eopen,
	Ecount,
	Ealloc,
	Eqid,
	Eaccess,
	Eentry,
	Emode,
	Edir1,
	Edir2,
	Ephase,
	Eexist,
	Edot,
	Eempty,
	Ebadu,
	Enoattach,
	Ewstatb,
	Ewstatd,
	Ewstatg,
	Ewstatl,
	Ewstatm,
	Ewstato,
	Ewstatp,
	Ewstatq,
	Ewstatu,
	Ewstatv,
	Ename,
	Ewalk,
	Eronly,
	Efull,
	Eoffset,
	Elocked,
	Ebroken,
	Eauth,
	Eauth2,
	Efidinuse,
	Etoolong,
	Econvert,
	Eversion,
	Eauthdisabled,
	Eauthnone,
	Eedge,
	MAXERR
};

/*
 * device types
 */
enum
{
	Devnone 	= 0,
	Devcon,			/* console */
	Devwren,		/* scsi disk drive */
	Devworm,		/* scsi video drive */
	Devlworm,		/* scsi video drive (labeled) */
	Devfworm,		/* fake read-only device */
	Devjuke,		/* jukebox */
	Devcw,			/* cache with worm */
	Devro,			/* readonly worm */
	Devmcat,		/* multiple cat devices */
	Devmlev,		/* multiple interleave devices */
	Devil,			/* internet link */
	Devpart,		/* partition */
	Devfloppy,		/* floppy drive */
	Devide,			/* IDE drive */
	Devswab,		/* swab data between mem and device */
	MAXDEV
};

/*
 * tags on block
 */
/* DONT TOUCH, this is in disk structures */
enum
{
	Tnone		= 0,
	Tsuper,			/* the super block */
	Tdir,			/* directory contents */
	Tind1,			/* points to blocks */
	Tind2,			/* points to Tind1 */
	Tfile,			/* file contents */
	Tfree,			/* in free list */
	Tbuck,			/* cache fs bucket */
	Tvirgo,			/* fake worm virgin bits */
	Tcache,			/* cw cache things */
	Tconfig,		/* configuration block */
	MAXTAG
};

/*
 * flags to getbuf
 */
enum
{
	Bread	= (1<<0),	/* read the block if miss */
	Bprobe	= (1<<1),	/* return null if miss */
	Bmod	= (1<<2),	/* buffer is dirty, needs writing */
	Bimm	= (1<<3),	/* write immediately on putbuf */
	Bres	= (1<<4),	/* reserved, never renammed */
};

extern	register	Mach*	m;
extern	register	User*	u;
extern  Talarm		talarm;

Conf	conf;
Cons	cons;
#define	MACHP(n)	((Mach*)(MACHADDR+n*BY2PG))

#pragma	varargck	type	"Z"	Device*
#pragma	varargck	type	"T"	ulong
#pragma	varargck	type	"I"	uchar*
#pragma	varargck	type	"E"	uchar*
#pragma	varargck	type	"W"	Filter*
#pragma	varargck	type	"G"	int

extern int (*fsprotocol[])(Msgbuf*);
