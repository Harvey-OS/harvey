/*
 * fundamental constants
 */
#define	ERRREC		64		/* size of a ascii erro message */
#define	DIRREC		116		/* size of a directory ascii record */
#define	NAMELEN		28		/* size of names */
#define	NDBLOCK		6		/* number of direct blocks in Dentry */
#define	MAXDAT		8192		/* max allowable data message */
#define	MAXMSG		128		/* max size protocol message sans data */
#define NTLOCK		200		/* number of active file Tlocks */

#include <auth.h>

typedef	struct	Fbuf	Fbuf;
typedef	struct	Super1	Super1;
typedef	struct	Superb	Superb;
typedef struct	Qid	Qid;
typedef	struct	Dentry	Dentry;
typedef	struct	Tag	Tag;

typedef struct	Device	Device;
typedef	struct	Fcall	Fcall;
typedef	struct	File	File;
typedef	struct	Filsys	Filsys;
typedef	struct	Filta	Filta;
typedef	struct	Filter	Filter;
typedef		ulong	Float;
typedef	struct	Hiob	Hiob;
typedef	struct	Iobuf	Iobuf;
typedef	struct	P9call	P9call;
typedef	struct	Tlock	Tlock;
typedef	struct	Tm	Tm;
typedef	struct	Uid	Uid;
typedef	struct	Wpath	Wpath;

/*
 * DONT TOUCH -- data structures stored on disk
 */
/* DONT TOUCH, this is the disk structure */
struct	Qid
{
	long	path;
	long	version;
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
	Qid	qid;
	long	size;
	long	dblock[NDBLOCK];
	long	iblock;
	long	diblock;
	long	atime;
	long	mtime;
};

/* DONT TOUCH, this is the disk structure */
struct	Tag
{
	short	pad;
	short	tag;
	long	path;
};

/* DONT TOUCH, this is the disk structure */
struct	Super1
{
	long	fstart;
	long	fsize;
	long	tfree;
	long	qidgen;		/* generator for unique ids */

	long	fsok;		/* file system ok */

	/*
	 * garbage for WWC device
	 */
	long	roraddr;	/* dump root addr */
	long	last;		/* last super block addr */
	long	next;		/* next super block addr */
};

/* DONT TOUCH, this is the disk structure */
struct	Fbuf
{
	long	nfree;
	long	free[1];		/* changes based on BUFSIZE */
};

/* DONT TOUCH, this is the disk structure */
struct	Superb
{
	Super1;
	Fbuf	fbuf;
};

struct	Device
{
	char	type;
	char	ctrl;
	char	unit;
	char	part;
};

/*
 * for load stats
 */
struct	Filter
{
	ulong	count;			/* count and old count kept separate */
	ulong	oldcount;		/* so interrput can read them */
	Float	filter[3];		/* filters for 1m 10m 100m */ 
};

struct	Filta
{
	Filter*	f;
	int	scale;
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

struct	Filsys
{
	char*	name;		/* name of filsys */
	Device	dev;		/* device that filsys is on */
	int	flags;
		#define	FREAM		(1<<1)	/* mkfs */
		#define	FRECOVER	(1<<2)	/* install last dump */
};

struct	Hiob
{
	Iobuf*	link;
	Lock;
};

struct	Iobuf
{
	QLock;
	Device	dev;
	Iobuf	*next;		/* for hash */
	Iobuf	*fore;		/* for lru */
	Iobuf	*back;		/* for lru */
	char	*iobuf;		/* only active while locked */
	char	*xiobuf;	/* "real" buffer pointer */
	long	addr;
	int	flags;
};

struct	P9call
{
	uchar	calln;
	uchar	rxflag;
	short	msize;
	void	(*func)(Chan*, int);
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

struct	Uid
{
	short	uid;		/* user id */
	short	lead;		/* leader of group */
	short	offset;		/* byte offset in uidspace */
};

struct	Wpath
{
	Wpath	*up;		/* pointer upwards in path */
	long	addr;		/* directory entry addr */
	long	slot;		/* directory entry slot */
	short	refs;		/* number of files using this structure */
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

#define	MAXFDATA	8192

enum
{
	Tmux =		48,
	Rmux,			/* illegal */
	Tnop =		50,
	Rnop,
	Tosession =	52,	/* illegal */
	Rosession,		/* illegal */
	Terror =	54,	/* illegal */
	Rerror,
	Tflush =	56,
	Rflush,
	Toattach =	58,	/* illegal */
	Roattach,		/* illegal */
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
	Eauth,
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

	MAXERR
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
};

/*
 * tags on block
 */
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
	MAXTAG
};

/*
 * flags to getbuf
 */
enum
{
	Bread	= (1<<0),	/* read the block if miss */
	Bprobe	= (1<<1),	/* return null if miss */
	Bmod	= (1<<2),	/* set modified bit in buffer */
	Bimm	= (1<<3),	/* set immediate bit in buffer */
	Bres	= (1<<4),	/* reserved, never renammed */
};

/*
 * open modes passed into P9 open/create
 */
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
 * check flags
 */
enum
{
	Crdall	= (1<<0),	/* read all files */
	Ctag	= (1<<1),	/* rebuild tags */
	Cpfile	= (1<<2),	/* print files */
	Cpdir	= (1<<3),	/* print directories */
	Cfree	= (1<<4),	/* rebuild free list */
	Cream	= (1<<6),	/* clear all bad tags */
	Cbad	= (1<<7),	/* clear all bad blocks */
	Ctouch	= (1<<8),	/* touch old dir and indir */
	Cquiet	= (1<<9),	/* report just nasty things */
};

/*
 * buffer size variables
 */
extern int	RBUFSIZE;
extern int	BUFSIZE;
extern int	DIRPERBUF;
extern int	INDPERBUF;
extern int	INDPERBUF2;
extern int	FEPERBUF;
