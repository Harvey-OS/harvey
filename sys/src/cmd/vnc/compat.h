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
	Lock;
	int	ref;
};

struct Rendez
{
	Lock;
	Proc	*p;
};

struct Chan
{
	Ref;
	Chan*	next;			/* allocation */
	Chan*	link;
	vlong	offset;			/* in file */
	ushort	type;
	ulong	dev;
	ushort	mode;			/* read/write */
	ushort	flag;
	Qid	qid;
	int	fid;			/* for devmnt */
	ulong	iounit;			/* chunk size for i/o; 0==default */
	void*	aux;
	Cname	*name;
};

struct Cname
{
	Ref;
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
	int	(*stat)(Chan*, uchar*, int);
	Chan*	(*open)(Chan*, int);
	void	(*create)(Chan*, char*, int, ulong);
	void	(*close)(Chan*);
	long	(*read)(Chan*, void*, long, vlong);
	Block*	(*bread)(Chan*, long, ulong);
	long	(*write)(Chan*, void*, long, vlong);
	long	(*bwrite)(Chan*, Block*, ulong);
	void	(*remove)(Chan*);
	int	(*wstat)(Chan*, uchar*, int);
};

struct Dirtab
{
	char	name[KNAMELEN];
	Qid	qid;
	vlong length;
	long	perm;
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
Block*		devbread(Chan*, long, ulong);
long		devbwrite(Chan*, Block*, ulong);
void		devcreate(Chan*, char*, int, ulong);
void		devdir(Chan*, Qid, char*, vlong, char*, long, Dir*);
long		devdirread(Chan*, char*, long, Dirtab*, int, Devgen*);
Devgen		devgen;
void		devinit(void);
Chan*		devopen(Chan*, int, Dirtab*, int, Devgen*);
void		devremove(Chan*);
void		devreset(void);
int		devstat(Chan*, uchar*, int, Dirtab*, int, Devgen*);
Walkqid*		devwalk(Chan*, Chan*, char**, int, Dirtab*, int, Devgen*);
int		devwstat(Chan*, uchar*, int);
void		error(char*);
int		incref(Ref*);
void		isdir(Chan*);
void		kproc(char*, void(*)(void*), void*);
void		mkqid(Qid*, vlong, ulong, int);
void		nexterror(void);
Chan*		newchan(void);
Cname*		newcname(char*);
int		openmode(ulong);
void		panic(char*, ...);
int		readstr(ulong, char*, ulong, char*);
long		seconds(void);
void*		smalloc(ulong);

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

#pragma	varargck	argpos	panic		1
