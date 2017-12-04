/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define Rendez KRendez

typedef struct Block	Block;
typedef struct Chan	Chan;
typedef struct Cname	Cname;
typedef struct Dev	Dev;
typedef struct Dirtab	Dirtab;
typedef struct Proc	Proc;
typedef struct Ref	Ref;
typedef struct Rendez	Rendez;
typedef struct Walkqid Walkqid;
typedef int    Devgen(Chan*, Dirtab*, int, int, Dir*);

enum
{
	KNAMELEN	= 28,
	NERR		= 15,

	COPEN		= 0x0001,		/* for i/o */
	CFREE		= 0x0010,		/* not in use */
};

struct Ref
{
	Lock lock;
	int	ref;
};

struct Rendez
{
	Lock lock;
	Proc	*p;
};

struct Chan
{
	Ref	ref;
	Chan*	next;			/* allocation */
	Chan*	link;
	int64_t	offset;			/* in file */
	uint16_t	type;
	uint32_t	dev;
	uint16_t	mode;			/* read/write */
	uint16_t	flag;
	Qid	qid;
	int	fid;			/* for devmnt */
	uint32_t	iounit;			/* chunk size for i/o; 0==default */
	void*	aux;
	Cname	*name;
};

struct Cname
{
	Ref	ref;
	int	alen;			/* allocated length */
	int	len;			/* strlen(s) */
	char	*s;
};

struct Dev
{
	int	dc;
	char*	name;

	void	(*reset)(void);
	void	(*init)(void);
	Chan*	(*attach)(char*);
	Walkqid*	(*walk)(Chan*, Chan*, char**, int);
	int	(*stat)(Chan*, uint8_t*, int);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, uint32_t);
	void	(*close)(Chan*);
	int32_t	(*read)(Chan*, void*, int32_t, int64_t);
	Block*	(*bread)(Chan*, int32_t, uint32_t);
	int32_t	(*write)(Chan*, void*, int32_t, int64_t);
	int32_t	(*bwrite)(Chan*, Block*, uint32_t);
	void	(*remove)(Chan*);
	int	(*wstat)(Chan*, uint8_t*, int);
};

struct Dirtab
{
	char	name[KNAMELEN];
	Qid	qid;
	int64_t length;
	int32_t	perm;
};

struct Walkqid
{
	Chan	*clone;
	int	nqid;
	Qid	qid[1];
};

struct Proc
{
	Lock	rlock;		/* for rendsleep, rendwakeup, intr */
	Rendez	*r;
	int	intr;

	char	name[KNAMELEN];
	char	*user;
	char	error[ERRMAX];
	int	nerrlab;
	jmp_buf	errlab[NERR];
	char	genbuf[128];	/* buffer used e.g. for last name element from namec */
};

#define DEVDOTDOT -1

extern	Proc	**privup;
#define	up	(*privup)
extern  char	*eve;
extern	Dev*	devtab[];

Chan*		cclone(Chan*);
void		cclose(Chan*);
void		cnameclose(Cname*);
int		decref(Ref*);
Chan*		devattach(int, char*);
Block*		devbread(Chan*, int32_t, uint32_t);
int32_t		devbwrite(Chan*, Block*, uint32_t);
void		devcreate(Chan*, char*, int, uint32_t);
void		devdir(Chan*, Qid, char*, int64_t, char*, int32_t, Dir*);
int32_t		devdirread(Chan*, char*, int32_t, Dirtab*, int, Devgen*);
Devgen		devgen;
void		devinit(void);
Chan*		devopen(Chan*, int, Dirtab*, int, Devgen*);
void		devremove(Chan*);
void		devreset(void);
int		devstat(Chan*, uint8_t*, int, Dirtab*, int, Devgen*);
Walkqid*		devwalk(Chan*, Chan*, char**, int, Dirtab*, int, Devgen*);
int		devwstat(Chan*, uint8_t*, int);
void		error(char*);
int		incref(Ref*);
void		isdir(Chan*);
void		kproc(char*, void(*)(void*), void*);
void		mkqid(Qid*, int64_t, uint32_t, int);
void		nexterror(void);
Chan*		newchan(void);
Cname*		newcname(char*);
int		openmode(uint32_t);
void		panic(char*, ...);
int		readstr(uint32_t, char*, uint32_t, char*);
int32_t		seconds(void);
void*		smalloc(uint32_t);

#define		poperror()	up->nerrlab--
#define		waserror()	(up->nerrlab++, setjmp(up->errlab[up->nerrlab-1]))

void		initcompat(void);
void		rendintr(void *v);
void		rendclearintr(void);
void		rendsleep(Rendez*, int(*)(void*), void*);
int		rendwakeup(Rendez*);
void		kexit(void);
int		sysexport(int fd, Chan **roots, int nroots);
int		errdepth(int ed);
void		newup(char *name);

int		exporter(Dev**, int*, int*);
int		mounter(char *mntpt, int how, int fds, int n);
void		shutdown(void);

void		screeninit(int, int, char*);

//#pragma	varargck	argpos	panic		1
