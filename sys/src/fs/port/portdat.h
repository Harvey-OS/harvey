/*
 * fundamental constants
 */
#define SUPER_ADDR	2		/* address of superblock */
#define ROOT_ADDR	3		/* address of superblock */
#define	ERRREC		64		/* size of a error record */
#define	DIRREC		116		/* size of a directory ascii record */
#define	NAMELEN		28		/* size of names */
#define	NDBLOCK		6		/* number of direct blocks in Dentry */
#define	RACHUNK		100		/* 600k read-ahead */
#define	RAOVERLAP	10		/* plus 60k overlap read-ahead */
#define	MAXDAT		8192		/* max allowable data message */
#define	MAXMSG		128		/* max size protocol message sans data */
#define	OFFMSG		60		/* offset of msg in buffer */
#define NDRIVE		16		/* size of drive structure */
#define NTLOCK		200		/* number of active file Tlocks */
#define	LRES		3		/* profiling resolution */
#define NATTID		10		/* the last 10 ID's in attaches */

/*
 *  more wonderful constants for authentication
 */
#include <auth.h>

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
#define	CDEV(dev)	cwdevs[(dev).part]
#define	WDEV(dev)	cwdevs[(dev).part+1]


typedef	struct	Vmedevice Vmedevice;	/* not defined here */

typedef struct	Alarm	Alarm;
typedef	struct	Conf	Conf;
typedef	struct	Label	Label;
typedef	struct	List	List;
typedef	struct	Lock	Lock;
typedef	struct	Mach	Mach;
typedef	struct	QLock	QLock;
typedef	struct	Ureg	Ureg;
typedef	struct	User	User;
typedef	struct	Fcall	Fcall;
typedef	struct	Fbuf	Fbuf;
typedef	struct	Super1	Super1;
typedef	struct	Superb	Superb;
typedef	struct	Filsys	Filsys;
typedef	struct	Dentry	Dentry;
typedef	struct	Tag	Tag;
typedef struct  Talarm	Talarm;
typedef	struct	Uid	Uid;
typedef struct	Device	Device;
typedef struct	Qid	Qid;
typedef	struct	Iobuf	Iobuf;
typedef	struct	Wpath	Wpath;
typedef	struct	File	File;
typedef	struct	Chan	Chan;
typedef	struct	P9call	P9call;
typedef	struct	Cons	Cons;
typedef	struct	Time	Time;
typedef	struct	Tm	Tm;
typedef	struct	Rtc	Rtc;
typedef	struct	Hiob	Hiob;
typedef	struct	RWlock	RWlock;
typedef	struct	Msgbuf	Msgbuf;
typedef	struct	Queue	Queue;
typedef	struct	Drive	Drive;
typedef	struct	Command	Command;
typedef	struct	Flag	Flag;
typedef	struct	Bp	Bp;
typedef	struct	Bit	Bit;
typedef	struct	Rabuf	Rabuf;
typedef	struct	Rout	Rout;
typedef	struct	Rendez	Rendez;
typedef	struct	Filter	Filter;
typedef		ulong	Float;
typedef	struct	Tlock	Tlock;
typedef	struct	Filta	Filta;
typedef	struct	Enpkt	Enpkt;
typedef	struct	Arppkt	Arppkt;
typedef	struct	Ippkt	Ippkt;
typedef	struct	Ilpkt	Ilpkt;
typedef	struct	Udppkt	Udppkt;
typedef	struct	Ifc	Ifc;

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
	Float	filter[3];		/* filters for 1m 10m 100m */ 
};

struct	Filta
{
	Filter*	f;
	int	scale;
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
	char	type;
	char	ctrl;
	char	unit;
	char	part;
};

struct	Rabuf
{
	union
	{
		struct
		{
			Device	dev;
			long	addr;
		};
		Rabuf*	link;
	};
};

/* DONT TOUCH, this is the disk structure */
struct	Qid
{
	long	path;
	long	version;
};

struct	Hiob
{
	Iobuf*	link;
	Lock;
};

enum
{
	Easize		= 6,		/* Ether address size */
	Pasize		= 4,		/* IP protocol address size */
};

typedef
struct
{
	Queue*	reply;		/* ethernet output */
	uchar	ea[Easize];	/* my ether address */
	uchar	ipmy[Pasize];	/* my ip address (index) */
	uchar	iphis[Pasize];	/* his ip address (index) */
	uchar	ipgate[Pasize];	/* his gateway address */
	int	usegate;	/* flag set if not on this subnet */
	Chan*	link;		/* list of il channels */
} Enp;

typedef
struct	Ilp
{
	Enp;			/* must be first -- botch */

	int	alloc;		/* 1 means allocated */
	int	srcp;		/* source port (index) */
	int	dstp;		/* source port (index) */
	int	state;		/* connection state */

	Msgbuf*	unacked;
	Msgbuf*	unackedtail;

	Msgbuf*	outoforder;

	ulong	next;		/* id of next to send */
	ulong	recvd;		/* last packet received */
	ulong	start;		/* local start id */
	ulong	rstart;		/* remote start id */

	int	timeout;	/* time out counter */
	int	slowtime;	/* slow time counter */
	int	fasttime;	/* retransmission timer */
	int	acktime;	/* acknowledge timer */
	int	querytime;	/* Query timer */
	int	deathtime;	/* Time to kill connection */

	int	rtt;		/* average round trip time */
	ulong	rttack;		/* the ack we are waiting for */
	ulong	ackms;		/* time we issued */

	int	window;		/* maximum receive window */

	Rendez	syn;		/* connect hang out */
} Ilp;

typedef
struct	Fchan
{
	Chan*	link;		/* list of fil channels */

	char*	msg;		/* pointer to channel error message */
	Rendez*	cr;		/* Conenct rendezvous */

	void*	ifaddr;		/* Hardware address */

	int	alloc;		/* 1 means allocated */
	ulong	laddress;	/* Local and remote address */
	ulong	raddress;
	ushort	lport;		/* Local and remote port */
	ushort	rport;

	int	state;		/* Connection state */
	Lock	ackq;		/* Unacknowledged queue */
	Msgbuf*	unacked;
	Msgbuf*	unackedtail;
	ulong	unackedbytes;	/* Number of bytes in unacked queue */
	ulong	need;
	ulong	sndwindow;
	Rendez	flowr;
	QLock	txq;

	Lock	outo;		/* Out of order packet queue */
	Msgbuf*	outoforder;

	ulong	next;		/* Id of next to send */
	ulong	recvd;		/* Last packet received */
	ulong	start;		/* Local start id */
	ulong	rstart;		/* Remote start id */

	int	timeout;	/* Time out counter */
	int	slowtime;	/* Slow time counter */
	int	fasttime;	/* Retransmission timer */
	int	acktime;	/* Acknowledge timer */
	int	querytime;	/* Query timer */
	int	deathtime;	/* Time to kill connection */
	int	rtt;		/* Average round trip time */
	ulong	rttack;		/* The ack we are waiting for */
	ulong	ackms;		/* Time we issued */
	int	window;		/* Maximum receive window */
} Fchan;

struct	Chan
{
	char	type;			/* major driver type ie Devdk */
	char	whochan[50];
	char	whoname[NAMELEN];
	ulong	flags;
	int	chan;			/* overall channel number, mostly for printing */
	int	nmsgs;			/* outstanding messages, set under flock -- for flush */
	long	whotime;
	int	nfile;			/* used by cmd_files */
	RWlock	reflock;
	Chan*	next;			/* link list of chans */
	Queue*	send;
	Queue*	reply;
	uchar	chal[CHALLEN];		/* locally generated challenge */
	uchar	rchal[CHALLEN];		/* remotely generated challenge */
	Lock	idlock;
	ulong	idoffset;		/* offset of id vector */
	ulong	idvec;			/* vector of acceptable id's */
	union
	{
		/*
		 * il ether circuit structure
		 */
		Ilp	ilp;

		/*
		 * fil fiber circuit structure
		 */
		Fchan	filp;

		/*
		 * data kit circuit structure
		 */
		struct
		{
			QLock;
			Msgbuf*	inmsg;		/* input message */
			Msgbuf*	outmsg;		/* output messages */
			Msgbuf*	ccmsg;		/* messages for CCCHAN */
			long	urptimeout;
			long	timeout;
			int	pokeflg;

			int	cno;		/* channel number  */
			int	repw;		/* write ptr */
			int	repr;		/* read ptr */

			int	icc;		/* input character pointer */
			int	gcc;		/* good characters */
			int	bot;		/* #bytes at BOT */
			int	last;		/* which BOT is seen */
			int	ack;
			int	ec;
			int	rej;

			int	urpstate;
			int	dkstate;
			int	calltimeout;
			int	seq;
			int	maxout;
			int	mlen;
			int	nout;
			int	mout;
			int	mcount;

			char	repchar[20];	/* list of echo characters */
		} dkp;
		/*
		 * hotrod fiber uart
		 */
		struct
		{
			void*	bitp;
		} hot;
	};
};

struct	Filsys
{
	char*	name;			/* name of filsys */
	char*	conf;			/* symbolic configuration */
	Device	dev;			/* device that filsys is on */
	int	flags;
		#define	FREAM		(1<<0)	/* mkfs */
		#define	FRECOVER	(1<<1)	/* install last dump */
		#define	FEDIT		(1<<2)	/* modified */
};

struct	Time
{
	long	lasttoy;
	long	bias;
	long	offset;
};

/*
 * array of qids that are locked
 */
struct	Tlock
{
	Device	dev;
	long	time;
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

	Filter	work;		/* thruput in messages */
	Filter	rate;		/* thruput in bytes */
	Filter	bhit;		/* getbufs that hit */
	Filter	bread;		/* getbufs that miss and read */
	Filter	brahead;	/* messages to readahead */
	Filter	binit;		/* getbufs that miss and dont read */
};

struct	P9call
{
	uchar	calln;
	uchar	rxflag;
	short	msize;
	void	(*func)(Chan*, int);
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
	short	fid;
	short	uid;
	char	open;
		#define	FREAD	1
		#define	FWRITE	2
		#define	FREMOV	4
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
	Device	dev;
	Iobuf*	next;		/* for hash */
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
	short	wuid;
	Qid	qid;
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

struct	Fcall
{
	char	type;
	short	fid;
	short	err;
	short	tag;
	union
	{
		struct
		{
			short	uid;		/* T-Userstr */
			short	oldtag;		/* T-nFlush */
			Qid	qid;		/* R-Attach, R-Clwalk, R-Walk,
						 * R-Open, R-Create */
			char	rauth[AUTHENTLEN];	/* R-attach */
		};
		struct
		{
			char	uname[NAMELEN];	/* T-nAttach */
			char	aname[NAMELEN];	/* T-nAttach */
			char	ticket[TICKETLEN];	/* T-attach */
			char	auth[AUTHENTLEN];	/* T-attach */
		};
		struct
		{
			char	ename[ERRREC];	/* R-nError */
			char	chal[CHALLEN];	/* T-session, R-session */
			char	authid[NAMELEN];	/* R-session */
			char	authdom[DOMLEN];	/* R-session */
		};
		struct
		{
			char	name[NAMELEN];	/* T-Walk, T-Clwalk, T-Create, T-Remove */
			long	perm;		/* T-Create */
			short	newfid;		/* T-Clone, T-Clwalk */
			char	mode;		/* T-Create, T-Open */
		};
		struct
		{
			long	offset;		/* T-Read, T-Write */
			long	count;		/* T-Read, T-Write, R-Read */
			char*	data;		/* T-Write, R-Read */
		};
		struct
		{
			char	stat[DIRREC];	/* T-Wstat, R-Stat */
		};
	};
};

struct	List
{
	void*	next;
};

struct	Label
{
	ulong	pc;
	ulong	sp;
};

struct	Alarm
{
	List;
	Lock;
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
	ulong	nrahead;	/* read ahead processes */
	ulong	nfile;		/* number of fid -- system wide */
	ulong	nwpath;		/* number of active paths, derrived from nfile */
	ulong	gidspace;	/* space for gid names -- derrived from nuid */
	ulong	nlgmsg;		/* number of large message buffers */
	ulong	nsmmsg;		/* number of small message buffers */
	ulong	wcpsize;	/* memory for worm copies */
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
	Chan*	chan;
	Msgbuf*	next;
	ulong	param;
	int	category;
	char*	data;
	char*	xdata;
};

/*
 * message buffer categories
 */
enum
{
	Mxxx		= 0,
	Mbbit,
	Mbreply1,
	Mbreply2,
	Mbreply3,
	Mbreply4,
	Mbdkxpmesg,
	Mbdkxpcall,
	Mbdksrlisten1,
	Mbdksrlisten2,
	Mbdkinput,
	Mbcycl1,
	Mbcycl2,
	Mbcycl3,
	Mbcycl4,
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
	Mbfil,
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
	Filter	time;		/* cpu time used */
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

struct	Drive
{
	Device	dev;
	char	status;
	char	qnum;
	char	fflag;
	long	block;			/* size of a block -- from config */
	long	nblock;			/* number of blocks -- from config */
	long	mult;			/* multiplier to get physical blocks */
	long	max;			/* number of logical blocks */
	Filter	work;
	Filter	rate;
};

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
 * Drive status
 */
enum
{
	Dunavail,
	Dready,
	Dnready,
	Dself,

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

#define	MAXFDATA	8192
/*
 * P9 protocol message types
 */
/* DONT TOUCH, this the 9P protocol */
enum
{
	Tnop =		50,
	Rnop,
	Tosession =	52,
	Rosession,
	Terror =	54,	/* illegal */
	Rerror,
	Tflush =	56,
	Rflush,
	Toattach =	58,
	Roattach,
	Tclone =	60,
	Rclone,
	Twalk =		62,
	Rwalk,
	Topen =		64,
	Ropen,
	Tcreate =	66,
	Rcreate,
	Tread =		68,
	Rread,
	Twrite =	70,
	Rwrite,
	Tclunk =	72,
	Rclunk,
	Tremove =	74,
	Rremove,
	Tstat =		76,
	Rstat,
	Twstat =	78,
	Rwstat,
	Tclwalk =	80,
	Rclwalk,
	Tauth =		82,	/* illegal */
	Rauth,			/* illegal */
	Tsession =	84,
	Rsession,
	Tattach =	86,
	Rattach,

	MAXSYSCALL
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
	Enotu,
	Enotg,
	Ename,
	Ewalk,
	Eronly,
	Efull,
	Eoffset,
	Elocked,
	Ebroken,
	Eauth,
	Enoattach,
	MAXERR
};

/*
 * device types
 */
enum
{
	Devnone 	= 0,
	Devcon,			/* console */
	Devdk,			/* datakit norm chan */
	Devdkcc,		/* datakit common control chan */
	Devdksr,		/* datakit service request chan */
	Devdkck,		/* datakit clock chan */
	Devwren,		/* scsi disk drive */
	Devworm,		/* scsi video drive */
	Devfworm,		/* fake read-only device */
	Devcw,			/* cache with worm */
	Devro,			/* readonly worm */
	Devcycl,		/* cyclone fiber uart */
	Devmcat,		/* multiple cat devices */
	Devmlev,		/* multiple interleave devices */
	Devil,			/* internet link */
	Devpart,		/* partition */
	Devfloppy,		/* floppy drive */
	Devide,			/* IDE drive */
	Devfil,			/* fiber interface */
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

/*
 * open modes passed into P9 open/create
 */
/* DONT TOUCH, this the P9 protocol */
enum
{
	MREAD	= 0,
	MWRITE,
	MBOTH,
	MEXEC,
	MTRUNC	= (1<<4),	/* truncate on open */
	MCEXEC	= (1<<5),	/* close on exec (host) */
	MRCLOSE	= (1<<6),	/* remove on close */
};


/*
 * Ethernet header
 */
enum
{
	ETHERMINTU	= 60,		/* minimum transmit size */
	ETHERMAXTU	= 1514,		/* maximum transmit size */

	Arptype		= 0x0806,
	Iptype		= 0x0800,
	Ilproto		= 40,
	Udpproto	= 17,
	Nqueue		= 20,
	Nfrag		= 6,		/* max number of non-contig ip fragments */
	Nrock		= 20,		/* number of partial ip assembly stations */
	Nb		= 211,		/* number of arp hash buckets */
	Ne		= 10,		/* number of entries in each arp hash bucket */

	Ensize		= 14,		/* ether header size */
	Ipsize		= 20,		/* ip header size -- doesnt include Ensize */
	Arpsize		= 28,		/* arp header size -- doesnt include Ensize */
	Ilsize		= 18,		/* il header size -- doesnt include Ipsize/Ensize */
	Udpsize		= 8,		/* il header size -- doesnt include Ipsize/Ensize */
	Udpphsize	= 12,		/* udp pseudo ip header size */

	IP_VER		= 0x40,			/* Using IP version 4 */
	IP_HLEN		= Ipsize/4,		/* Header length in longs */
	IP_DF		= 0x4000,		/* Don't fragment */
	IP_MF		= 0x2000,		/* More fragments */

	Arprequest	= 1,
	Arpreply,

	Ilfsport	= 17008,
	Ilauthport	= 17020,
	Ilfsout		= 5000,
};

struct	Enpkt
{
	uchar	d[Easize];		/* destination address */
	uchar	s[Easize];		/* source address */
	uchar	type[2];		/* packet type */

	uchar	data[ETHERMAXTU-(6+6+2)];
	uchar	crc[4];
};

struct	Arppkt
{
	uchar	d[Easize];			/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	hrd[2];				/* hardware type, must be ether==1 */
	uchar	pro[2];				/* protocol, must be ip */
	uchar	hln;				/* hardware address len, must be Easize */
	uchar	pln;				/* protocol address len, must be Pasize */
	uchar	op[2];
	uchar	sha[Easize];
	uchar	spa[Pasize];
	uchar	tha[Easize];
	uchar	tpa[Pasize];
};

struct	Ippkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* Version and header length */
	uchar	tos;			/* Type of service */
	uchar	length[2];		/* packet length */
	uchar	id[2];			/* Identification */
	uchar	frag[2];		/* Fragment information */
	uchar	ttl;			/* Time to live */
	uchar	proto;			/* Protocol */
	uchar	cksum[2];		/* Header checksum */
	uchar	src[Pasize];		/* Ip source */
	uchar	dst[Pasize];		/* Ip destination */
};

struct	Ilpkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* ip header */
	uchar	tos;
	uchar	length[2];
	uchar	id[2];
	uchar	frag[2];
	uchar	ttl;
	uchar	proto;
	uchar	cksum[2];
	uchar	src[Pasize];
	uchar	dst[Pasize];

	uchar	ilsum[2];		/* Checksum including header */
	uchar	illen[2];		/* Packet length */
	uchar	iltype;			/* Packet type */
	uchar	ilspec;			/* Special */
	uchar	ilsrc[2];		/* Src port */
	uchar	ildst[2];		/* Dst port */
	uchar	ilid[4];		/* Sequence id */
	uchar	ilack[4];		/* Acked sequence */
};

struct	Udppkt
{
	uchar	d[Easize];		/* ether header */
	uchar	s[Easize];
	uchar	type[2];

	uchar	vihl;			/* ip header */
	uchar	tos;
	uchar	length[2];
	uchar	id[2];
	uchar	frag[2];
	uchar	ttl;
	uchar	proto;
	uchar	cksum[2];
	uchar	src[Pasize];
	uchar	dst[Pasize];

	uchar	udpsrc[2];		/* Src port */
	uchar	udpdst[2];		/* Dst port */
	uchar	udplen[2];		/* Packet length */
	uchar	udpsum[2];		/* Checksum including header */
};

struct	Ifc
{
	Lock;
	Queue*	reply;
	Filter	work;
	Filter	rate;
	ulong	rcverr;
	ulong	txerr;
	ulong	sumerr;
	ulong	rxpkt;
	ulong	txpkt;
	uchar	ea[Easize];		/* our ether address */
	uchar	ipa[Pasize];		/* our ip address, pulled from netdb */
	uchar	netgate[Pasize];	/* ip gateway, pulled from netdb */
	ulong	ipaddr;
	ulong	mask;
	ulong	cmask;
	Ifc	*next;			/* List of configured interfaces */
};

extern	register	Mach*	m;
extern	register	User*	u;
extern  Talarm		talarm;

Conf	conf;
Cons	cons;
#define	MACHP(n)	((Mach*)(MACHADDR+n*BY2PG))
