/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * fundamental constants
 */
#define NAMELEN 28  /* size of names */
#define NDBLOCK 6   /* number of direct blocks in Dentry */
#define MAXDAT 8192 /* max allowable data message */
#define NTLOCK 200  /* number of active file Tlocks */

typedef struct Fbuf Fbuf;
typedef struct Super1 Super1;
typedef struct Superb Superb;
// typedef struct	Qid	Qid;
typedef struct Dentry Dentry;
typedef struct Tag Tag;

typedef struct Device Device;
typedef struct Qid9p1 Qid9p1;
typedef struct File File;
typedef struct Filsys Filsys;
typedef struct Filta Filta;
typedef struct Filter Filter;
typedef uint32_t Float;
typedef struct Hiob Hiob;
typedef struct Iobuf Iobuf;
typedef struct P9call P9call;
typedef struct Tlock Tlock;
// typedef	struct	Tm	Tm;
typedef struct Uid Uid;
typedef struct Wpath Wpath;
typedef struct AuthRpc AuthRpc;

/*
 * DONT TOUCH -- data structures stored on disk
 */
/* DONT TOUCH, this is the disk structure */
struct Qid9p1 {
	int32_t path;
	int32_t version;
};

/* DONT TOUCH, this is the disk structure */
struct Dentry {
	char name[NAMELEN];
	short uid;
	short gid;
	uint16_t mode;
#define DALLOC 0x8000
#define DDIR 0x4000
#define DAPND 0x2000
#define DLOCK 0x1000
#define DREAD 0x4
#define DWRITE 0x2
#define DEXEC 0x1
	Qid9p1 qid;
	int32_t size;
	int32_t dblock[NDBLOCK];
	int32_t iblock;
	int32_t diblock;
	int32_t atime;
	int32_t mtime;
};

/* DONT TOUCH, this is the disk structure */
struct Tag {
	short pad;
	short tag;
	int32_t path;
};

/* DONT TOUCH, this is the disk structure */
struct Super1 {
	int32_t fstart;
	int32_t fsize;
	int32_t tfree;
	int32_t qidgen; /* generator for unique ids */

	int32_t fsok; /* file system ok */

	/*
	 * garbage for WWC device
	 */
	int32_t roraddr; /* dump root addr */
	int32_t last;    /* last super block addr */
	int32_t next;    /* next super block addr */
};

/* DONT TOUCH, this is the disk structure */
struct Fbuf {
	int32_t nfree;
	int32_t free[1]; /* changes based on BUFSIZE */
};

/* DONT TOUCH, this is the disk structure */
struct Superb {
	Super1;
	Fbuf fbuf;
};

struct Device {
	char type;
	char ctrl;
	char unit;
	char part;
};

/*
 * for load stats
 */
struct Filter {
	uint32_t count;    /* count and old count kept separate */
	uint32_t oldcount; /* so interrput can read them */
	Float filter[3];   /* filters for 1m 10m 100m */
};

struct Filta {
	Filter *f;
	int scale;
};

/*
 * array of qids that are locked
 */
struct Tlock {
	Device dev;
	int32_t time;
	int32_t qpath;
	File *file;
};

struct File {
	QLock;
	Qid qid;
	Wpath *wpath;
	Chan *cp;     /* null means a free slot */
	Tlock *tlock; /* if file is locked */
	File *next;   /* in cp->flist */
	File *list;   /* in list of free files */
	Filsys *fs;
	int32_t addr;
	int32_t slot;
	int32_t lastra; /* read ahead address */
	short fid;
	short uid;
	char open;
#define FREAD 1
#define FWRITE 2
#define FREMOV 4
#define FWSTAT 8
	int32_t doffset; /* directory reading */
	uint32_t dvers;
	int32_t dslot;

	/* for network authentication */
	AuthRpc *rpc;
	short cuid;
};

struct Filsys {
	char *name; /* name of filesys */
	Device dev; /* device that filesys is on */
	int flags;
#define FREAM (1 << 1)    /* mkfs */
#define FRECOVER (1 << 2) /* install last dump */
};

struct Hiob {
	Iobuf *link;
	Lock;
};

struct Iobuf {
	QLock;
	Device dev;
	Iobuf *next;  /* for hash */
	Iobuf *fore;  /* for lru */
	Iobuf *back;  /* for lru */
	char *iobuf;  /* only active while locked */
	char *xiobuf; /* "real" buffer pointer */
	int32_t addr;
	int flags;
};

struct P9call {
	uint8_t calln;
	uint8_t rxflag;
	short msize;
	void (*func)(Chan *, int);
};

// struct	Tm
// {
// 	/* see ctime(3) */
// 	int	sec;
// 	int	min;
// 	int	hour;
// 	int	mday;
// 	int	mon;
// 	int	year;
// 	int	wday;
// 	int	yday;
// 	int	isdst;
// };

struct Uid {
	short uid;    /* user id */
	short lead;   /* leader of group */
	short offset; /* byte offset in uidspace */
};

struct Wpath {
	Wpath *up;    /* pointer upwards in path */
	Wpath *list;  /* link in free chain */
	int32_t addr; /* directory entry addr of parent */
	int32_t slot; /* directory entry slot of parent */
	short refs;   /* number of files using this structure */
};

#define MAXFDATA 8192

/*
 * error codes generated from the file server
 */
enum {
	Ebadspc = 1,
	Efid,
	Efidinuse,
	Echar,
	Eopen,
	Ecount,
	Ealloc,
	Eqid,
	Eauth,
	Eauthmsg,
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
	Etoolong,
	Ersc,
	Eqidmode,
	Econvert,
	Enotm,
	Enotd,
	Enotl,
	Enotw,
	Esystem,

	MAXERR
};

/*
 * devnone block numbers
 */
enum {
	Cwio1 = 1,
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
enum {
	Tnone = 0,
	Tsuper, /* the super block */
	Tdir,   /* directory contents */
	Tind1,  /* points to blocks */
	Tind2,  /* points to Tind1 */
	Tfile,  /* file contents */
	Tfree,  /* in free list */
	Tbuck,  /* cache fs bucket */
	Tvirgo, /* fake worm virgin bits */
	Tcache, /* cw cache things */
	MAXTAG
};

/*
 * flags to getbuf
 */
enum {
	Bread = (1 << 0),  /* read the block if miss */
	Bprobe = (1 << 1), /* return null if miss */
	Bmod = (1 << 2),   /* set modified bit in buffer */
	Bimm = (1 << 3),   /* set immediate bit in buffer */
	Bres = (1 << 4),   /* reserved, never renammed */
};

/*
 * open modes passed into P9 open/create
 */
enum {
	MREAD = 0,
	MWRITE,
	MBOTH,
	MEXEC,
	MTRUNC = (1 << 4),  /* truncate on open */
	MCEXEC = (1 << 5),  /* close on exec (host) */
	MRCLOSE = (1 << 6), /* remove on close */
};

/*
 * check flags
 */
enum {
	Crdall = (1 << 0), /* read all files */
	Ctag = (1 << 1),   /* rebuild tags */
	Cpfile = (1 << 2), /* print files */
	Cpdir = (1 << 3),  /* print directories */
	Cfree = (1 << 4),  /* rebuild free list */
	Cream = (1 << 6),  /* clear all bad tags */
	Cbad = (1 << 7),   /* clear all bad blocks */
	Ctouch = (1 << 8), /* touch old dir and indir */
	Cquiet = (1 << 9), /* report just nasty things */
};

/*
 * buffer size variables
 */
extern int RBUFSIZE;
extern int BUFSIZE;
extern int DIRPERBUF;
extern int INDPERBUF;
extern int INDPERBUF2;
extern int FEPERBUF;
