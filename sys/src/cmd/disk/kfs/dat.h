/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef	struct	Chan	Chan;
typedef struct	Command	Command;
typedef	struct	Conf	Conf;
typedef	struct	Cons	Cons;
typedef struct	Devcall	Devcall;

#define MAXBUFSIZE	(16*1024)	/* max. buffer size */

#include "portdat.h"

struct	Chan
{
	int	chan;			/* fd request came in on */
	QLock rlock, wlock;		/* lock for reading/writing messages on chan */
	int	type;
	int	flags;
	i32	whotime;
	File*	flist;			/* base of file structures */
	Lock	flock;			/* manipulate flist */
	RWLock	reflock;		/* lock for Tflush */
	int	msize;			/* version */
	int	authed;		/* someone other than ``none'' has authed */

/* 9p1 auth */
	u8	chal[8];
	u8	rchal[8];
	int	idoffset;
	int	idvec;
	Lock	idlock;
};

/*
 * console cons.flag flags
 */
enum
{
	Fchat	= (1<<0),	/* print out filesys rpc traffic */
	Fuid	= (1<<2),	/* print out uids */
				/* debugging flags for drivers */
};

struct	Cons
{
	int	flags;		/* overall flags for all channels */
	int	uid;		/* botch -- used to get uid on cons_create */
	int	gid;		/* botch -- used to get gid on cons_create */
	int	allow;		/* no-protection flag */
	i32	offset;		/* used to read files, c.f. fchar */
	char*	arg;		/* pointer to remaining line */

	Chan	*chan;	/* console channel */
	Chan	*srvchan;	/* local server channel */

	Filter	work;		/* thruput in messages */
	Filter	rate;		/* thruput in bytes */
	Filter	bhit;		/* getbufs that hit */
	Filter	bread;		/* getbufs that miss and read */
	Filter	binit;		/* getbufs that miss and dont read */
	Filter	tags[MAXTAG];	/* reads of each type of block */
};

struct	Conf
{
	u32	niobuf;		/* number of iobufs to allocate */
	u32	nuid;		/* distinct uids */
	u32	uidspace;	/* space for uid names -- derrived from nuid */
	u32	gidspace;	/* space for gid names -- derrived from nuid */
	u32	nserve;		/* server processes */
	u32	nfile;		/* number of fid -- system wide */
	u32	nwpath;		/* number of active paths, derrived from nfile */
	u32	bootsize;	/* number of bytes reserved for booting */
};

struct	Command
{
	char	*string;
	void	(*func)(void);
	char	*args;
};

struct Devcall
{
	void	(*init)(Device);
	void	(*ream)(Device);
	int	(*check)(Device);
	i32	(*super)(Device);
	i32	(*root)(Device);
	i32	(*size)(Device);
	int	(*read)(Device, i32, void*);
	int	(*write)(Device, i32, void*);
};

/*
 * device types
 */
enum
{
	Devnone 	= 0,
	Devwren,
	MAXDEV
};

/*
 * file systems
 */
enum
{
	MAXFILSYS = 4
};

/*
 * should be in portdat.h
 */
#define	QPDIR	0x80000000L
#define	QPNONE	0
#define	QPROOT	1
#define	QPSUPER	2

/*
 * perm argument in p9 create
 */
#define	PDIR	(1L<<31)	/* is a directory */
#define	PAPND	(1L<<30)	/* is append only */
#define	PLOCK	(1L<<29)	/* is locked on open */

#define	NOF	(-1)

#define	FID1		1
#define	FID2		2
#define	FID3		3

#define SECOND(n) 	(n)
#define MINUTE(n)	(n*SECOND(60))
#define HOUR(n)		(n*MINUTE(60))
#define DAY(n)		(n*HOUR(24))
#define	TLOCK		MINUTE(5)

#define	CHAT(cp)	(chat)
#define	QID9P1(a,b)	(Qid9p1){a,b}

extern	Uid*	uid;
extern	char*	uidspace;
extern	short*	gidspace;
extern	char*	errstring[MAXERR];
extern	Chan*	chans;
extern	RWLock	mainlock;
extern	i32	boottime;
extern	Tlock	*tlocks;
extern	Device	devnone;
extern	Filsys	filesys[];
extern	char	service[];
extern	char*	tagnames[];
extern	Conf	conf;
extern	Cons	cons;
extern	Command	command[];
extern	Chan	*chan;
extern	Devcall	devcall[];
extern	char	*progname;
extern	char	*procname;
extern	i32	niob;
extern	i32	nhiob;
extern	Hiob	*hiob;
extern	int	chat;
extern	int	writeallow;
extern	int	wstatallow;
extern	int	allownone;
extern	int	noatime;
extern	int	writegroup;

extern Lock wpathlock;
