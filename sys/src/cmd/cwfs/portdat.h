/*
 * fundamental constants and types of the implementation
 * changing any of these changes the layout on disk
 */
enum {
	SUPER_ADDR	= 2,		/* block address of superblock */
	ROOT_ADDR	= 3,		/* block address of root directory */
};

/* more fundamental types */
typedef vlong Wideoff; /* type to widen Off to for printing; ≥ as wide as Off */
typedef short	Userid;		/* signed internal representation of user-id */
typedef long	Timet;		/* in seconds since epoch */
typedef vlong	Devsize;	/* in bytes */


/* macros */
#define NEXT(x, l)	(((x)+1) % (l))
#define PREV(x, l)	((x) == 0? (l)-1: (x)-1)
#define	HOWMANY(x, y)	(((x)+((y)-1)) / (y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y)) * (y))

#define	TK2MS(t) (((ulong)(t)*1000)/HZ)	/* ticks to ms - beware rounding */
#define	MS2TK(t) (((ulong)(t)*HZ)/1000)	/* ms to ticks - beware rounding */
#define	TK2SEC(t) ((t)/HZ)		/* ticks to seconds */

/* constants that don't affect disk layout */
enum {
	MAXDAT		= 8192,		/* max allowable data message */
	MAXMSG		= 128,		/* max protocol message sans data */

	MB		= 1024*1024,

	HZ		= 1,		/* clock frequency */
};

/*
 * tunable parameters
 */
enum {
	Maxword		= 256,		/* max bytes per command-line word */
	NTLOCK		= 200,		/* number of active file Tlocks */
};

typedef struct	Auth	Auth;
typedef	struct	Bp	Bp;
typedef	struct	Bucket	Bucket;
typedef	struct	Cache	Cache;
typedef	struct	Centry	Centry;
typedef	struct	Chan	Chan;
typedef	struct	Command	Command;
typedef	struct	Conf	Conf;
typedef	struct	Cons	Cons;
typedef	struct	Dentry	Dentry;
typedef struct	Device	Device;
typedef	struct	Fbuf	Fbuf;
typedef	struct	File	File;
typedef	struct	Filsys	Filsys;
typedef	struct	Filter	Filter;
typedef	struct	Flag	Flag;
typedef	struct	Hiob	Hiob;
typedef	struct	Iobuf	Iobuf;
typedef	struct	Lock	Lock;
typedef	struct	Msgbuf	Msgbuf;
typedef	struct	QLock	QLock;
typedef struct	Qid9p1	Qid9p1;
typedef	struct	Queue	Queue;
typedef	union	Rabuf	Rabuf;
typedef	struct	Rendez	Rendez;
typedef	struct	Rtc	Rtc;
typedef	struct	Startsb	Startsb;
typedef	struct	Super1	Super1;
typedef	struct	Superb	Superb;
typedef	struct	Tag	Tag;
typedef	struct	Time	Time;
typedef	struct	Tlock	Tlock;
typedef	struct	Tm	Tm;
typedef	struct	Uid	Uid;
typedef	struct	Wpath	Wpath;

#pragma incomplete Auth

struct	Tag
{
	short	pad;		/* make tag end at a long boundary */
	short	tag;
	Off	path;
};

/* DONT TOUCH, this is the disk structure */
struct	Qid9p1
{
	Off	path;			/* was long */
	ulong	version;		/* should be Off */
};

/* DONT TOUCH, this is the disk structure */
struct	Super1
{
	Off	fstart;
	Off	fsize;
	Off	tfree;
	Off	qidgen;		/* generator for unique ids */
	/*
	 * Stuff for WWC device
	 */
	Off	cwraddr;	/* cfs root addr */
	Off	roraddr;	/* dump root addr */
	Off	last;		/* last super block addr */
	Off	next;		/* next super block addr */
};

/* DONT TOUCH, this is the disk structure */
struct	Centry
{
	ushort	age;
	short	state;
	Off	waddr;		/* worm addr */
};

/* DONT TOUCH, this is the disk structure */
struct	Dentry
{
	char	name[NAMELEN];
	Userid	uid;
	Userid	gid;
	ushort	mode;
		#define	DALLOC	0x8000
		#define	DDIR	0x4000
		#define	DAPND	0x2000
		#define	DLOCK	0x1000
		#define	DREAD	0x4
		#define	DWRITE	0x2
		#define	DEXEC	0x1
	Userid	muid;
	Qid9p1	qid;
	Off	size;
	Off	dblock[NDBLOCK];
	Off	iblocks[NIBLOCK];
	long	atime;
	long	mtime;
};

/*
 * derived constants
 */
enum {
	BUFSIZE		= RBUFSIZE - sizeof(Tag),
	DIRPERBUF	= BUFSIZE / sizeof(Dentry),
	INDPERBUF	= BUFSIZE / sizeof(Off),
	FEPERBUF	= (BUFSIZE-sizeof(Super1)-sizeof(Off)) / sizeof(Off),
	SMALLBUF	= MAXMSG,
	LARGEBUF	= MAXMSG+MAXDAT+256,
	RAGAP		= (300*1024)/BUFSIZE,	/* readahead parameter */
	BKPERBLK	= 10,
	CEPERBK		= (BUFSIZE - BKPERBLK*sizeof(Off)) /
				(sizeof(Centry)*BKPERBLK),
};

/*
 * send/recv queue structure
 */
struct	Queue
{
	QLock;			/* to manipulate values */
	Rendez	empty;
	Rendez	full;

	int	waitedfor;	/* flag */
	char*	name;		/* for debugging */

	int	size;		/* size of queue */
	int	loc;		/* circular pointer */
	int	count;		/* how many in queue */
	void*	args[1];	/* list of saved pointers, [->size] */
};

struct	Device
{
	uchar	type;
	uchar	init;
	Device*	link;			/* link for mcat/mlev/mirror */
	Device*	dlink;			/* link all devices */
	void*	private;
	Devsize	size;
	union {
		struct {		/* disk, (l)worm in j.j, sides */
			int	ctrl;	/* disks only */
			int	targ;
			int	lun;	/* not implemented in sd(3) */

			int	mapped;
			char*	file;	/* ordinary file or dir instead */

			int	fd;
			char*	sddir;	/* /dev/sdXX name, for juke drives */
			char*	sddata;	/* /dev/sdXX/data or other file */
		} wren;
		struct {		/* mcat mlev mirror */
			Device*	first;
			Device*	last;
			int	ndev;
		} cat;
		struct {		/* cw */
			Device*	c;	/* cache device */
			Device*	w;	/* worm device */
			Device*	ro;	/* dump - readonly */
		} cw;
		struct {		/* juke */
			Device*	j;	/* (robotics, worm drives) - wrens */
			Device*	m;	/* (sides) - r or l devices */
		} j;
		struct {		/* ro */
			Device*	parent;
		} ro;
		struct {		/* fworm */
			Device*	fw;
		} fw;
		struct {		/* part */
			Device*	d;
			long	base;	/* percentages */
			long	size;
		} part;
		struct {		/* byte-swapped */
			Device*	d;
		} swab;
	};
};

typedef struct Sidestarts {
	Devsize	sstart;			/* blocks before start of side */
	Devsize	s1start;		/* blocks before start of next side */
} Sidestarts;

union Rabuf {
	struct {
		Device*	dev;
		Off	addr;
	};
	Rabuf*	link;
};

struct	Hiob
{
	Iobuf*	link;
	Lock;
};

/* a 9P connection */
struct	Chan
{
	char	type;			/* major driver type i.e. Dev* */
	int	(*protocol)(Msgbuf*);	/* version */
	int	msize;			/* version */
	char	whochan[50];
	char	whoname[NAMELEN];
	void	(*whoprint)(Chan*);
	ulong	flags;
	int	chan;			/* overall channel #, mostly for printing */
	int	nmsgs;			/* outstanding messages, set under flock -- for flush */

	Timet	whotime;
	int	nfile;			/* used by cmd_files */

	RWLock	reflock;
	Chan*	next;			/* link list of chans */
	Queue*	send;
	Queue*	reply;

	uchar	authinfo[64];

	void*	pdata;			/* sometimes is a Netconn* */
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
	Off	startsb;
};

struct	Time
{
	Timet	lasttoy;
	Timet	offset;
};

/*
 * array of qids that are locked
 */
struct	Tlock
{
	Device*	dev;
	Timet	time;
	Off	qpath;
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
	Off	offset;		/* used to read files, c.f. fchar */
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

//	Filter	work[3];	/* thruput in messages */
//	Filter	rate[3];	/* thruput in bytes */
//	Filter	bhit[3];	/* getbufs that hit */
//	Filter	bread[3];	/* getbufs that miss and read */
//	Filter	brahead[3];	/* messages to readahead */
//	Filter	binit[3];	/* getbufs that miss and dont read */
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
	Off	addr;
	long	slot;		/* ordinal # of Dentry with a directory block */
	Off	lastra;		/* read ahead address */
	ulong	fid;
	Userid	uid;
	Auth	*auth;
	char	open;
		#define	FREAD	1
		#define	FWRITE	2
		#define	FREMOV	4

	Off	doffset;	/* directory reading */
	ulong	dvers;
	long	dslot;
};

struct	Wpath
{
	Wpath*	up;		/* pointer upwards in path */
	Off	addr;		/* directory entry addr */
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
	Off	addr;
	int	flags;
};

struct	Uid
{
	Userid	uid;		/* user id */
	Userid	lead;		/* leader of group */
	Userid	*gtab;		/* group table */
	int	ngrp;		/* number of group entries */
	char	name[NAMELEN];	/* user name */
};

/* DONT TOUCH, this is the disk structure */
struct	Fbuf
{
	Off	nfree;
	Off	free[FEPERBUF];
};

/* DONT TOUCH, this is the disk structure */
struct	Superb
{
	Fbuf	fbuf;
	Super1;
};

struct	Conf
{
	ulong	nmach;		/* processors */
	ulong	mem;		/* total physical bytes of memory */
	ulong	nuid;		/* distinct uids */
	ulong	nserve;		/* server processes */
	ulong	nfile;		/* number of fid -- system wide */
	ulong	nwpath;		/* number of active paths, derived from nfile */
	ulong	gidspace;	/* space for gid names -- derived from nuid */

	ulong	nlgmsg;		/* number of large message buffers */
	ulong	nsmmsg;		/* number of small message buffers */

	Off	recovcw;	/* recover addresses */
	Off	recovro;
	Off	firstsb;
	Off	recovsb;

	ulong	configfirst;	/* configure before starting normal operation */
	char	*confdev;
	char	*devmap;	/* name of config->file device mapping file */

	ulong	nauth;		/* number of Auth structs */
	uchar	nodump;		/* no periodic dumps */
	uchar	dumpreread;	/* read and compare in dump copy */
};

enum {
	Mbmagic = 0xb0ffe3,
};

/*
 * message buffers
 * 2 types, large and small
 */
struct	Msgbuf
{
	ulong	magic;
	short	count;
	short	flags;
		#define	LARGE	(1<<0)
		#define	FREE	(1<<1)
		#define BFREE	(1<<2)
		#define BTRACE	(1<<7)
	Chan*	chan;		/* file server conn within a net. conn */
	Msgbuf*	next;
	uintptr	param;		/* misc. use; keep Conn* here */

	int	category;
	uchar*	data;		/* rp or wp: current processing point */
	uchar*	xdata;		/* base of allocation */
};

/*
 * message buffer categories
 */
enum
{
	Mxxx		= 0,
	Mbeth1,
	Mbreply1,
	Mbreply2,
	Mbreply3,
	Mbreply4,
	MAXCAT,
};

enum { PRINTSIZE = 256 };

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

struct	Rtc
{
	int	sec;
	int	min;
	int	hour;
	int	mday;
	int	mon;
	int	year;
};

typedef struct
{
	/* constants during a given truncation */
	Dentry	*d;
	Iobuf	*p;			/* the block containing *d */
	int	uid;
	Off	newsize;
	Off	lastblk;		/* last data block of file to keep */

	/* variables */
	Off	relblk;			/* # of current data blk within file */
	int	pastlast;		/* have we walked past lastblk? */
	int	err;
} Truncstate;

/*
 * cw device
 */

/* DONT TOUCH, this is the disk structure */
struct	Cache
{
	Off	maddr;		/* cache map addr */
	Off	msize;		/* cache map size in buckets */
	Off	caddr;		/* cache addr */
	Off	csize;		/* cache size */
	Off	fsize;		/* current size of worm */
	Off	wsize;		/* max size of the worm */
	Off	wmax;		/* highwater write */

	Off	sbaddr;		/* super block addr */
	Off	cwraddr;	/* cw root addr */
	Off	roraddr;	/* dump root addr */

	Timet	toytime;	/* somewhere convienent */
	Timet	time;
};

/* DONT TOUCH, this is the disk structure */
struct	Bucket
{
	long	agegen;		/* generator for ages in this bkt */
	Centry	entry[CEPERBK];
};

/* DONT TOUCH, this is in disk structures */
enum { Labmagic = 0xfeedfacedeadbeefULL, };

/* DONT TOUCH, this is the disk structure */
typedef struct Label Label;
struct Label			/* label block on Devlworms, in last block */
{
	uvlong	magic;
	ushort	ord;		/* side number within Juke */
	char	service[64];	/* documentation only */
};

typedef struct Map Map;
struct Map {
	char	*from;
	Device	*fdev;
	char	*to;
	Device	*tdev;
	Map	*next;
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
 * devnone block numbers
 */
enum
{
	Cwio1	= 1,
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
	Eauthfile,
	Eedge,
	MAXERR
};

/*
 * device types
 */
enum
{
	Devnone	= 0,
	Devcon,			/* console */
	Devwren,		/* disk drive */
	Devworm,		/* scsi optical drive */
	Devlworm,		/* scsi optical drive (labeled) */
	Devfworm,		/* fake read-only device */
	Devjuke,		/* scsi jukebox */
	Devcw,			/* cache with worm */
	Devro,			/* readonly worm */
	Devmcat,		/* multiple cat devices */
	Devmlev,		/* multiple interleave devices */
	Devnet,			/* network connection */
	Devpart,		/* partition */
	Devfloppy,		/* floppy drive */
	Devswab,		/* swab data between mem and device */
	Devmirr,		/* mirror devices */
	MAXDEV
};

/*
 * tags on block
 */
/* DONT TOUCH, this is in disk structures */
/* also, the order from Tdir to Tmaxind is exploited in indirck() & isdirty() */
enum
{
	Tnone		= 0,
	Tsuper,			/* the super block */
#ifdef COMPAT32
	Tdir,			/* directory contents */
	Tind1,			/* points to blocks */
	Tind2,			/* points to Tind1 */
#else
	Tdirold,
	Tind1old,
	Tind2old,
#endif
	Tfile,			/* file contents; also defined in disk.h */
	Tfree,			/* in free list */
	Tbuck,			/* cache fs bucket */
	Tvirgo,			/* fake worm virgin bits */
	Tcache,			/* cw cache things */
	Tconfig,		/* configuration block */
#ifndef COMPAT32
	/* Tdir & indirect blocks are last, to allow for greater depth */
	Tdir,			/* directory contents */
	Tind1,			/* points to blocks */
	Tind2,			/* points to Tind1 */
	Tind3,			/* points to Tind2 */
	Tind4,			/* points to Tind3 */
	Maxtind,
#endif
	/* gap for more indirect block depth in future */
	Tlabel = 32,		/* Devlworm label in last block */
	MAXTAG,

#ifdef COMPAT32
	Tmaxind = Tind2,
#else
	Tmaxind = Maxtind - 1,
#endif
};

/*
 * flags to getbuf
 */
enum
{
	Brd	= (1<<0),	/* read the block if miss */
	Bprobe	= (1<<1),	/* return null if miss */
	Bmod	= (1<<2),	/* buffer is dirty, needs writing */
	Bimm	= (1<<3),	/* write immediately on putbuf */
	Bres	= (1<<4),	/* reserved, never renamed */
};

Conf	conf;
Cons	cons;

#pragma	varargck	type	"Z"	Device*
#pragma	varargck	type	"T"	Timet
#pragma	varargck	type	"I"	uchar*
#pragma	varargck	type	"E"	uchar*
#pragma	varargck	type	"G"	int

extern char	*annstrs[];
extern Biobuf	bin;
extern Map	*devmap;
extern int	(*fsprotocol[])(Msgbuf*);
