/* avoid name conflicts */
#undef accept
#undef listen

/* sys calls */
#undef bind
#undef chdir
#undef close
#undef create
#undef dup
#undef export
#undef fstat
#undef fwstat
#undef mount
#undef open
#undef start
#undef read
#undef remove
#undef seek
#undef stat
#undef write
#undef wstat
#undef unmount
#undef pipe

#include "auth.h"
#include "fcall.h"


typedef struct Block	Block;
typedef struct Chan	Chan;
typedef struct Dev	Dev;
typedef struct Dirtab	Dirtab;
typedef struct Fgrp	Fgrp;
typedef struct Mount	Mount;
typedef struct Mntrpc	Mntrpc;
typedef struct Mntwalk	Mntwalk;
typedef struct Mnt	Mnt;
typedef struct Mhead	Mhead;
typedef struct Path	Path;
typedef struct Pgrp	Pgrp;
typedef struct Proc	Proc;
typedef struct Pthash	Pthash;
typedef struct Queue	Queue;
typedef struct Session	Session;
typedef struct RWlock	RWlock;
typedef int    Devgen(Chan*, Dirtab*, int, int, Dir*);

struct RWlock
{
	Lock	l;			/* Lock modify lock */
	Qlock	x;			/* Mutual exclusion lock */
	Qlock	k;			/* Lock for waiting writers */
	int	readers;		/* Count of readers in lock */
};

/*
 * Access types in namec & channel flags
 */
enum
{
	Aaccess,			/* as in access, stat */
	Atodir,				/* as in chdir */
	Aopen,				/* for i/o */
	Amount,				/* to be mounted upon */
	Acreate,			/* file is to be created */

	COPEN	= 0x0001,		/* for i/o */
	CMSG	= 0x0002,		/* the message channel for a mount */
	CCREATE	= 0x0004,		/* permits creation if c->mnt */
	CCEXEC	= 0x0008,		/* close on exec */
	CFREE	= 0x0010,		/* not in use */
	CRCLOSE	= 0x0020		/* remove on close */
};

struct Path
{
	Ref	r;
	Path*	hash;
	Path*	parent;
	Pthash*	pthash;
	char	elem[NAMELEN];
};

struct Chan
{
	Ref	r;
	Chan*	next;			/* allocation */
	Chan*	link;
	ulong	offset;			/* in file */
	ushort	type;
	ulong	dev;
	ushort	mode;			/* read/write */
	ushort	flag;
	Qid	qid;
	int	fid;			/* for devmnt */
	Path*	path;
	Mount*	mnt;			/* mount point that derived Chan */
	Mount*	xmnt;			/* Last mount point crossed */
	ulong	mountid;
	union {
		void*	aux;
		Mnt*	mntptr;		/* for devmnt */
	} u;
	Chan*	mchan;			/* channel to mounted server */
	Qid	mqid;			/* qid of root of mount point */
	Session	*session;
};

struct Dev
{
	void	(*init)(void);
	Chan*	(*attach)(void*);
	Chan*	(*clone)(Chan*, Chan*);
	int	(*walk)(Chan*, char*);
	void	(*stat)(Chan*, char*);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, ulong);
	void	(*close)(Chan*);
	long	(*read)(Chan*, void*, long, ulong);
	Block*	(*bread)(Chan*, long, ulong);
	long	(*write)(Chan*, void*, long, ulong);
	long	(*bwrite)(Chan*, Block*, ulong);
	void	(*remove)(Chan*);
	void	(*wstat)(Chan*, char*);
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
};
#define BLEN(s)	((s)->wp - (s)->rp)

struct Dirtab
{
	char	name[NAMELEN];
	Qid	qid;
	long	length;
	long	perm;
};

enum
{
	NSMAX	=	1000,
	NSLOG	=	7,
	NSCACHE	=	(1<<NSLOG)
};

struct Pthash
{
	Qlock	ql;
	int	npt;
	Path*	root;
	Path*	hash[NSCACHE];
};

struct Mntwalk				/* state for /proc/#/ns */
{
	ulong	id;
	Mhead*	mh;
	Mount*	cm;
};

struct Mount
{
	ulong	mountid;
	Mount*	next;
	Mhead*	head;
	Mount*	copy;
	Mount*	order;
	Chan*	to;			/* channel replacing channel */
	int	flag;
	char	spec[NAMELEN];
};

struct Mhead
{
	Chan*	from;			/* channel mounted upon */
	Mount*	mount;			/* what's mounted upon it */
	Mhead*	hash;			/* Hash chain */
};

struct Mnt
{
	Ref	r;		/* Count of attached channels */
	Chan*	c;		/* Channel to file service */
	int	rip;		/* Reader in progress */
	Mntrpc*	queue;		/* Queue of pending requests on this channel */
	ulong	id;		/* Multiplexor id for channel check */
	Mnt*	list;		/* Free list */
	int	flags;		/* recover/cache */
	char	recprog;	/* Recovery in progress */
	int	blocksize;	/* read/write block size */
	ushort	flushtag;	/* Tag to send flush on */
	ushort	flushbase;	/* Base tag of flush window for this buffer */
	Pthash	tree;		/* Path names from this mount point */
	int	npart;		/* Partial buffer count */
	uchar	part[1];	/* Partial buffer MUST be last */
};

enum
{
	MNTHASH	=	32,		/* Hash to walk mount table */
	NFD =		100		/* per process file descriptors */
};
#define MOUNTH(p,s)	((p)->mnthash[(s)->qid.path%MNTHASH])

struct Pgrp
{
	Ref	r;			/* also used as a lock when mounting */
	RWlock	ns;			/* Namespace n read/one write lock */
	Qlock	nsh;
	Mhead*	mnthash[MNTHASH];
};

struct Fgrp
{
	Ref	r;
	Chan*	fd[NFD];
	int	maxfd;			/* highest fd in use */
};

struct	Proc
{
	ulong	pid;
	char	user[NAMELEN];
	char	elem[NAMELEN];
	int	uid;
	int	gid;
	Pgrp*	pgrp;
	Fgrp*	fgrp;
	Chan*	slash;
	Chan*	dot;
};

extern	Rune	devchar[];
extern	Dev	devtab[];
extern	char	sysname[3*NAMELEN];
extern	char	eve[NAMELEN];
extern	Pthash	syspt;
extern	Queue*	kbdq;

/* Set up private thread space */
extern	Proc	*up;

extern	Block*	allocb(int);
extern  ulong	authrequest(Session*, Fcall*);
extern  void	authreply(Session*, ulong, Fcall*);
extern  long	authread(Chan*, char*, int);
extern  long	authwrite(Chan*, char*, int);
extern  long	authcheck(Chan*, char*, int);
extern  long	authentwrite(Chan*, char*, int);
extern  long	authentread(Chan*, char*, int);
extern  void	authclose(Chan*);
extern	int	canlock(Lock*);
extern	void	cclose(Chan*);
extern	void	chandevinit(void);
extern	void	chanfree(Chan*);
extern	void	clearintr(void);
extern	Chan*	clone(Chan*, Chan*);
extern	void	closepgrp(Pgrp*);
extern	void	closefgrp(Fgrp*);
extern	Chan*	createdir(Chan*);
extern	int	cmount(Chan*, Chan*, int, char*);
extern	void	cunmount(Chan*, Chan*);
extern	int	decrypt(void*, void*, int);
extern	Chan*	domount(Chan*);
extern	void	drawflush(void);
extern	Fgrp*	dupfgrp(Fgrp*);
extern	int	encrypt(void*, void*, int);
extern	int	eqqid(Qid, Qid);
extern	int	export(int);
extern	Chan*	fdtochan(int, int, int, int);
extern	void	freeb(Block*);
extern	int	getfields(char*, char**, int, int, char*);
extern	long	hostownerwrite(char*, int);
extern	int	iprint(char*, ...);
extern	void	isdir(Chan*);
extern  void	intr(void*);
extern  long	keyread(char*, int, long);
extern  long	keywrite(char*, int);
extern	void	kbdinit(void);
extern	void	kbdputc(int);
extern	void	pminit(void);
extern	long	latin1(Rune*, int);
extern	void	libinit(void);
extern	void	lock(Lock*);
extern	void	mountfree(Mount*);
extern	Chan*	namec(char*, int, int, ulong);
extern	void	nameok(char*);
extern	Chan*	newchan(void);
extern	Fgrp*	newfgrp(void);
extern	Mount*	newmount(Mhead*, Chan*, int, char*);
extern	Pgrp*	newpgrp(void);
extern	char*	nextelem(char*, char*);
extern	void	nexterror(void);
extern	int	nrand(int n);
extern  int	openmode(ulong);
extern	void	osfillproc(Proc*);
extern	Block*	padblock(Block*, int);
extern	void	panic(char*, ...);
extern	void	pexit(char*, int);
extern	void	procserve(void);
extern	Path*	ptenter(Pthash*, Path*, char*);
extern	void	ptclose(Pthash *pt);
extern	Block*	qbread(Queue*, int);
extern	long	qbwrite(Queue*, Block*);
extern	int	qcanread(Queue*);
extern	void	qclose(Queue*);
extern	int	qconsume(Queue*, void*, int);
extern	void	qflush(Queue*);
extern	void	qhangup(Queue*, char*);
extern	void	qinit(void);
extern	int	qlen(Queue*);
extern	Queue*	qopen(int, int, void (*)(void*), void*);
extern	int	qpass(Queue*, Block*);
extern	int	qproduce(Queue*, void*, int);
extern	long	qread(Queue*, void*, int);
extern	void	qreopen(Queue*);
extern	int	qwindow(Queue*);
extern	long	qiwrite(Queue*, void*, int);
extern	long	qwrite(Queue*, void*, int);
extern	void	qsetlimit(Queue*, int);
extern	void	qnoblock(Queue*, int);
extern	int	readkbd(void);
extern	int	readstr(ulong, char*, ulong, char*);
extern	void	rlock(RWlock*);
extern  void	runlock(RWlock*);
extern  int	running(void);
extern	void	(*screenputs)(char*, int);
extern	char*	skipslash(char*);
extern	void	srvrtinit(void);
extern  int	ticks(void);
extern	long	unionread(Chan*, void*, long);
extern	Chan*	walk(Chan*, char*, int);
extern	void	wlock(RWlock*);
extern	void	wunlock(RWlock*);
extern	void	vmachine(void*);
extern	void	terminit(void);

/* sys calls */
extern	int	sysbind(char*, char*, int);
extern	int	sysclose(int);
extern	int	syscreate(char*, int, ulong);
extern	int	syschdir(char*);
extern	int	sysdup(int, int);
extern	int	sysfstat(int, char*);
extern	int	sysfwstat(int, char*);
extern	int	sysmount(int, char*, int, char*);
extern	int	sysunmount(char*, char*);
extern	int	sysopen(char*, int);
extern	long	sysread(int, void*, long);
extern	int	sysremove(char*);
extern	long	sysseek(int, long, int);
extern	int	sysstat(char*, char*);
extern	long	syswrite(int, void*, long);
extern	int	syswstat(char*, char*);
extern	int	sysdirstat(char*, Dir*);
extern	int	sysdirfstat(int, Dir*);
extern	int	sysdirwstat(char*, Dir*);
extern	int	sysdirfwstat(int, Dir*);
extern	long	sysdirread(int, Dir*, long);

/* generic device functions */
extern	void	devinit(void);
extern  void	devreset(void);
extern	Chan*	devattach(int, char*);
extern	Chan*	devattach(int, char*);
extern	Block*	devbread(Chan*, long, ulong);
extern	long	devbwrite(Chan*, Block*, ulong);
extern	void	devcreate(Chan*, char*, int, ulong);
extern  void	devdir(Chan*, Qid, char*, long, char*, long, Dir*);
extern  long	devdirread(Chan*, char*, long, Dirtab*, int, Devgen*);
extern	Chan*	devclone(Chan*, Chan*);
extern	int	devgen(Chan*, Dirtab*, int, int, Dir*);
extern	int	devno(int, int);
extern	Chan*	devopen(Chan*, int, Dirtab*, int, Devgen*);
extern	void	devstat(Chan*, char*, Dirtab*, int, Devgen*);
extern	int	devwalk(Chan*, char*, Dirtab*, int, Devgen*);
extern	void	devremove(Chan*);
extern	void	devwstat(Chan*, char*);

/* Common Device Drivers */
extern	void	mntinit(void);
extern	Chan*	mntattach(void*);
extern	Chan*	mntclone(Chan*, Chan*);
extern	int	mntwalk(Chan*, char*);
extern	void	mntstat(Chan*, char*);
extern	Chan*	mntopen(Chan*, int);
extern	void	mntcreate(Chan*, char*, int, ulong);
extern	void	mntclose(Chan*);
extern	long	mntread(Chan*, void*, long, ulong);
extern	long	mntwrite(Chan*, void*, long, ulong);
extern	void	mntremove(Chan*);
extern	void	mntwstat(Chan*, char*);
extern	void	ipinit(void);
extern	Chan*	ipattach(void*);
extern	Chan*	ipclone(Chan*, Chan*);
extern	int	ipwalk(Chan*, char*);
extern	void	ipstat(Chan*, char*);
extern	Chan*	ipopen(Chan*, int);
extern	void	ipcreate(Chan*, char*, int, ulong);
extern	void	ipclose(Chan*);
extern	long	ipread(Chan*, void*, long, ulong);
extern	long	ipwrite(Chan*, void*, long, ulong);
extern	void	ipremove(Chan*);
extern	void	ipwstat(Chan*, char*);
extern  uchar	*clipread(void);
extern  int	clipwrite(uchar*, int);
extern	void	coninit(void);
extern	Chan*	conattach(void*);
extern	Chan*	conclone(Chan*, Chan*);
extern	int	conwalk(Chan*, char*);
extern	void	constat(Chan*, char*);
extern	Chan*	conopen(Chan*, int);
extern	void	concreate(Chan*, char*, int, ulong);
extern	void	conclose(Chan*);
extern	long	conread(Chan*, void*, long, ulong);
extern	long	conwrite(Chan*, void*, long, ulong);
extern	void	conwstat(Chan*, char*);
extern	void	conremove(Chan*);
extern	void	conwstat(Chan*, char*);
extern	void	fsinit(void);
extern	Chan*	fsattach(void*);
extern	Chan*	fsclone(Chan*, Chan*);
extern	int	fswalk(Chan*, char*);
extern	void	fsstat(Chan*, char*);
extern	Chan*	fsopen(Chan*, int);
extern	void	fscreate(Chan*, char*, int, ulong);
extern	void	fsclose(Chan*);
extern	long	fsread(Chan*, void*, long, ulong);
extern	long	fswrite(Chan*, void*, long, ulong);
extern	void	fsremove(Chan*);
extern	void	fswstat(Chan*, char*);
extern	void	drawinit(void);
extern	Chan*	drawattach(void*);
extern	Chan*	drawclone(Chan*, Chan*);
extern	int	drawwalk(Chan*, char*);
extern	void	drawstat(Chan*, char*);
extern	Chan*	drawopen(Chan*, int);
extern	void	drawcreate(Chan*, char*, int, ulong);
extern	void	drawclose(Chan*);
extern	long	drawread(Chan*, void*, long, ulong);
extern	long	drawwrite(Chan*, void*, long, ulong);
extern	void	drawwstat(Chan*, char*);
extern	void	drawremove(Chan*);
extern	void	drawwstat(Chan*, char*);
extern	void	mouseinit(void);
extern	Chan*	mouseattach(void*);
extern	Chan*	mouseclone(Chan*, Chan*);
extern	int	mousewalk(Chan*, char*);
extern	void	mousestat(Chan*, char*);
extern	Chan*	mouseopen(Chan*, int);
extern	void	mousecreate(Chan*, char*, int, ulong);
extern	void	mouseclose(Chan*);
extern	long	mouseread(Chan*, void*, long, ulong);
extern	long	mousewrite(Chan*, void*, long, ulong);
extern	void	mouseremove(Chan*);
extern	void	mousewstat(Chan*, char*);
extern	void	pipeinit(void);
extern	Chan*	pipeattach(void*);
extern	Chan*	pipeclone(Chan*, Chan*);
extern	int	pipewalk(Chan*, char*);
extern	void	pipestat(Chan*, char*);
extern	Chan*	pipeopen(Chan*, int);
extern	void	pipecreate(Chan*, char*, int, ulong);
extern	void	pipeclose(Chan*);
extern	long	piperead(Chan*, void*, long, ulong);
extern	Block*	pipebread(Chan*, long, ulong);
extern	long	pipewrite(Chan*, void*, long, ulong);
extern	long	pipebwrite(Chan*, Block*, ulong);
extern	void	piperemove(Chan*);
extern	void	pipewstat(Chan*, char*);
extern	void	pipeinit(void);
extern	void	rootinit(void);
extern	Chan*	rootattach(void*);
extern	Chan*	rootclone(Chan*, Chan*);
extern	int	rootwalk(Chan*, char*);
extern	void	rootstat(Chan*, char*);
extern	Chan*	rootopen(Chan*, int);
extern	void	rootcreate(Chan*, char*, int, ulong);
extern	void	rootclose(Chan*);
extern	long	rootread(Chan*, void*, long, ulong);
extern	long	rootwrite(Chan*, void*, long, ulong);
extern	void	rootremove(Chan*);
extern	void	rootwstat(Chan*, char*);

enum {
	SnarfSize = 64*1024
};
