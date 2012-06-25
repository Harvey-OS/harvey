typedef struct Ref Ref;
typedef struct Urestart Urestart;
typedef struct Uproc Uproc;
typedef struct Uproctab Uproctab;
typedef struct Uwaitq Uwaitq;
typedef struct Uwait Uwait;

typedef struct Udev Udev;
typedef struct Ufile Ufile;
typedef struct Ustat Ustat;
typedef struct Udirent Udirent;

typedef struct Ureg Ureg;
typedef struct Usiginfo Usiginfo;

enum {
	HZ = 100,
	PAGESIZE = 0x1000,

	MAXPROC	= 128,
	MAXFD	= 256,

	USTACK	= 8*1024*1024,
	KSTACK	= 8*1024,
};

struct Ref
{
	long	ref;
};

struct Urestart
{
	Urestart		*link;
	char			*syscall;

	union {
		struct {
			vlong	timeout;
		}			nanosleep;
		struct {
			vlong	timeout;
		}			poll;
		struct {
			vlong	timeout;
		}			select;
		struct {
			vlong	timeout;
		}			futex;
	};
};

struct Uproc
{
	QLock;

	int		tid;
	int		pid;
	int		ppid;
	int		pgid;
	int		psid;
	int		uid;
	int		gid;
	int		umask;
	int		tlsmask;

	int		kpid;
	int		notefd;
	int		argsfd;

	int		wstate;
	int		wevent;
	int		exitcode;
	int		exitsignal;

	int		*cleartidptr;

	vlong	timeout;

	vlong	alarm;
	Uproc	*alarmq;

	char  	*state;
	char  	*xstate;
	int		innote;
	int		notified;
	Ureg		*ureg;
	char		*syscall;
	void		(*sysret)(int errno);
	Urestart	*restart;
	Urestart	restart0;
	Uwait	*freewait;

	void		(*traceproc)(void *arg);
	void		*tracearg;

	int		linkloop;
	char		*root;
	char		*cwd;
	char		*kcwd;

	void		*fdtab;
	void		*mem;
	void		*trace;
	void		*signal;

	char		*comm;
	int		ncomm;
	ulong	codestart;
	ulong	codeend;
	ulong	stackstart;
	vlong	starttime;
};

struct Uproctab
{
	QLock;
	int		nextpid;
	int		alloc;
	Uproc	proc[MAXPROC];
};

struct Uwaitq
{
	QLock;
	Uwait	*w;
};

struct Uwait
{
	Uwait	*next;
	Uwaitq	*q;
	Uwait	*nextq;
	Uproc	*proc;
	Ufile  	*file;
};

enum {
	ROOTDEV,
	SOCKDEV,
	PIPEDEV,
	CONSDEV,
	MISCDEV,
	DSPDEV,
	PTYDEV,
	PROCDEV,
	MAXDEV,
};

/* device */
struct Udev
{
	int		(*open)(char *path, int mode, int perm, Ufile **pf);
	int		(*access)(char *path, int perm);
	int		(*stat)(char *path, int link, Ustat *ps);

	int		(*link)(char *old, char *new, int sym);
	int		(*unlink)(char *path, int rmdir);
	int		(*readlink)(char *path, char *buf, int len);
	int		(*rename)(char *old, char *new);
	int		(*mkdir)(char *path, int mode);
	int		(*utime)(char *path, long atime, long mtime);
	int		(*chmod)(char *path, int mode);
	int		(*chown)(char *path, int uid, int gid, int link);
	int		(*truncate)(char *path, vlong size);

	int		(*read)(Ufile *file, void *buf, int len, vlong off);
	int		(*write)(Ufile *file, void *buf, int len, vlong off);

	vlong	(*size)(Ufile *file);
	int		(*poll)(Ufile *file, void *tab);
	int		(*ioctl)(Ufile *file, int cmd, void *arg);
	int		(*close)(Ufile *file);

	int		(*fstat)(Ufile *file, Ustat *ps);
	int		(*readdir)(Ufile *file, Udirent **pd);
	
	int		(*fchmod)(Ufile *file, int mode);
	int		(*fchown)(Ufile *file, int uid, int gid);
	int		(*ftruncate)(Ufile *file, vlong size);
};

struct Ufile
{
	Ref;

	int		mode;
	int		dev;
	char		*path;
	int		fd;
	vlong	off;

	Udirent	*rdaux;	/* aux pointer to hold Udirent* chains */
};

struct Ustat
{
	int		mode;
	int		uid;
	int		gid;
	int		dev;
	int		rdev;
	vlong	size;
	ulong	atime;
	ulong	mtime;
	ulong	ctime;
	uvlong	ino;
};

struct Udirent
{
	Udirent	*next;

	uvlong	ino;
	int		mode;
	char  	name[];
};

struct Usiginfo
{
	int		signo;
	int		errno;
	int		code;

	union {
		/* kill() */
		struct {
			int	pid;		/* sender's pid */
			int	uid;		/* sender's uid */
		} kill;

		/* POSIX.1b timers */
		struct {
			int	tid;			/* timer id */
			int	overrun;		/* overrun count */
			int	val;			/* same as below */
			int	sys_private;	/* not to be passed to user */
		} timer;

		/* POSIX.1b signals */
		struct {
			int	pid;			/* sender's pid */
			int	uid;			/* sender's uid */
			int	val;
		} rt;

		/* SIGCHLD */
		struct {
			int	pid;			/* which child */
			int	uid;			/* sender's uid */
			int	status; 		/* exit code */
			long	utime;
			long	stime;
		} chld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS */
		struct {
			void	*addr;		/* faulting insn/memory ref. */
			int 	trapno;		/* TRAP # which caused the signal */
		} fault;

		/* SIGPOLL */
		struct {
			long	band;		/* POLL_IN, POLL_OUT, POLL_MSG */
			int	fd;
		} poll;
	};

	int		topid;
	int		group;
};

int debug;
long *kstack;
long *exitjmp;
Uproc **pcurrent;
#define current (*pcurrent)
vlong boottime;

Udev *devtab[MAXDEV];
Uproctab proctab;
