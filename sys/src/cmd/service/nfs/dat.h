enum
{
	FHSIZE	= 32
};

typedef struct Accept	Accept;
typedef struct Auth	Auth;
typedef struct Authunix	Authunix;
typedef struct Chalstuff Chalstuff;
typedef uchar		Fhandle[FHSIZE];
typedef struct Fid	Fid;
typedef struct Lock	Lock;
typedef struct Procmap	Procmap;
typedef struct Progmap	Progmap;
typedef struct Reject	Reject;
typedef struct Rpccall	Rpccall;
typedef struct Rpccache	Rpccache;
typedef struct Sattr	Sattr;
typedef struct Session	Session;
typedef struct String	String;
typedef struct Strnode	Strnode;
typedef struct Unixid	Unixid;
typedef struct Unixidmap Unixidmap;
typedef struct Unixmap	Unixmap;
typedef struct Unixscmap Unixscmap;
typedef struct Xfid	Xfid;
typedef struct Xfile	Xfile;

struct Lock
{
	ulong	key;
};

struct String
{
	ulong	n;
	char *	s;
};

struct Progmap
{
	int	progno;
	int	vers;
	void	(*init)(int, char**);
	Procmap *pmap;
};

struct Procmap
{
	int	procno;
	int	(*procp)(int, Rpccall*, Rpccall*);
};

struct Auth
{
	ulong	flavor;
	ulong	count;
	void *	data;
};

struct Authunix
{
	ulong	stamp;
	String	mach;
	ulong	uid;
	ulong	gid;
	int	gidlen;
	ulong	gids[10];
};

struct Accept
{
	Auth	averf;
	ulong	astat;
	union{
		void *	results;	/* SUCCESS */
		struct{			/* PROG_MISMATCH */
			ulong	plow;	/* acceptable version numbers */
			ulong	phigh;
		};
	};
};

struct Reject
{
	ulong	rstat;
	union{
		struct{			/* RPC_MISMATCH */
			ulong	rlow;	/* acceptable  rpc version numbers */
			ulong	rhigh;
		};
		ulong	authstat;	/* AUTH_ERROR */
	};
};

struct Rpccall
{
	ulong	host;		/* prefixed to RPC message */
	ulong	port;		/* prefixed to RPC message */
	ulong	xid;		/* transaction id */
	ulong	mtype;		/* CALL or REPLY */
	union{
		struct{		/* CALL */
			ulong	rpcvers;	/* must be equal to two (2) */
			ulong	prog;		/* program number */
			ulong	vers;		/* program version */
			ulong	proc;		/* procedure number */
			Auth	cred;		/* authentication credentials */
			Auth	verf;		/* authentication verifier */
			Unixidmap *up;
			char *	user;
			void *	args;		/* procedure-specific */
		};
		struct{		/* REPLY */
			ulong	stat;		/* MSG_ACCEPTED or MSG_DENIED */
			union{
				Accept;
				Reject;
			};
		};
	};
};

struct Rpccache
{
	Rpccache *prev;
	Rpccache *next;
	ulong	host;
	ulong	port;
	ulong	xid;
	int	n;
	uchar	data[4];
};

struct Sattr
{
	ulong	mode;
	ulong	uid;
	ulong	gid;
	ulong	size;
	ulong	atime;		/* sec's */
	ulong	ausec;		/* microsec's */
	ulong	mtime;
	ulong	musec;
};

struct Strnode
{
	Strnode *next;	/* in hash bucket */
	char	str[4];
};

struct Unixid
{
	Unixid *next;
	char *	name;
	int	id;
};

struct Unixmap
{
	char *	file;
	int	style;
	long	timestamp;
	Unixid *ids;
};

struct Unixidmap
{
	Unixidmap *next;
	int	flag;
	char *	server;
	char *	client;
	Reprog *sexp;
	Reprog *cexp;
	Unixmap	u;
	Unixmap	g;
};

struct Unixscmap
{
	Unixscmap *next;
	char *	server;
	char *	client;
	Unixidmap *map;
};

struct Xfile
{
	Xfile *	next;		/* hash chain */
	Session	*s;
	ulong	qidpath;	/* from stat */
	Xfile *	parent;
	Xfile *	child;		/* if directory */
	Xfile *	sib;		/* siblings */
	char *	name;		/* path element */
	Xfid *	users;
};

enum
{
	Oread	= 1,
	Owrite	= 2,
	Open	= 3,
	Trunc	= 4
};

struct Xfid
{
	Xfid *	next;		/* Xfile's user list */
	Xfile *	xp;
	char *	uid;
	Fid *	urfid;
	Fid *	opfid;
	ulong	mode;		/* open mode, if opfid is non-zero */
	ulong	offset;
};

struct Fid
{
	Fid **	owner;		/* null for root fids */
	Fid *	prev;
	Fid *	next;
	long	tstale;		/* auto-clunk */
};

struct Session
{
	Session *next;
	char *	service;		/* for dial */
	int	fd;
	char	cchal[CHALLEN];		/* client challenge */
	char	schal[CHALLEN];		/* server challenge */
	char	authid[NAMELEN];	/* server encryption uid */
	char	authdom[DOMLEN];	/* server encryption domain */
	int	count;			/* number of attaches */
	char *	spec;			/* for attach */
	Xfile *	root;			/* to answer mount rpc */
	ushort	tag;
	Fcall	f;
	char	data[MAXFDATA+MAXMSG];
	Fid *	free;			/* available */
	Fid	list;			/* active, most-recently-used order */
	Fid	fids[1000];
};

struct Chalstuff
{
	Chalstuff *next;
	Xfid *	xf;
	long	tstale;
	Chalstate;
};

extern int	rpcdebug;
extern int	chatty;
extern Progmap progmap[];
extern int	myport;
extern void	(*rpcalarm)(void);
extern long	starttime;
extern long	nfstime;
extern int	noauth;
extern char *	config;
