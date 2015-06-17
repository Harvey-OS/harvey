/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum
{
	FHSIZE	= 32
};

typedef struct Accept	Accept;
typedef struct Auth	Auth;
typedef struct Authunix	Authunix;
typedef struct Chalstuff Chalstuff;
typedef unsigned char		Fhandle[FHSIZE];
typedef struct Fid	Fid;
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

struct String
{
	uint32_t	n;
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
	uint32_t	flavor;
	uint32_t	count;
	void *	data;
};

struct Authunix
{
	uint32_t	stamp;
	String	mach;
	uint32_t	uid;
	uint32_t	gid;
	int	gidlen;
	uint32_t	gids[10];
};

struct Accept
{
	Auth	averf;
	uint32_t	astat;
	union{
		void *	results;	/* SUCCESS */
		struct{			/* PROG_MISMATCH */
			uint32_t	plow;	/* acceptable version numbers */
			uint32_t	phigh;
		};
	};
};

struct Reject
{
	uint32_t	rstat;
	union{
		struct{			/* RPC_MISMATCH */
			uint32_t	rlow;	/* acceptable  rpc version numbers */
			uint32_t	rhigh;
		};
		uint32_t	authstat;	/* AUTH_ERROR */
	};
};

struct Rpccall
{
	/* corresponds to Udphdr */
	unsigned char	prefix0[12];
	uint32_t	host;		/* ipv4 subset: prefixed to RPC message */
	unsigned char	prefix1[12];
	uint32_t	lhost;		/* ipv4 subset: prefixed to RPC message */
	/* ignore ifcaddr */
	uint32_t	port;		/* prefixed to RPC message */
	uint32_t	lport;		/* prefixed to RPC message */

	uint32_t	xid;		/* transaction id */
	uint32_t	mtype;		/* CALL or REPLY */
	union{
		struct{		/* CALL */
			uint32_t	rpcvers;	/* must be equal to two (2) */
			uint32_t	prog;		/* program number */
			uint32_t	vers;		/* program version */
			uint32_t	proc;		/* procedure number */
			Auth	cred;		/* authentication credentials */
			Auth	verf;		/* authentication verifier */
			Unixidmap *up;
			char *	user;
			void *	args;		/* procedure-specific */
		};
		struct{		/* REPLY */
			uint32_t	stat;		/* MSG_ACCEPTED or MSG_DENIED */
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
	uint32_t	host;
	uint32_t	port;
	uint32_t	xid;
	int	n;
	unsigned char	data[4];
};

struct Sattr
{
	uint32_t	mode;
	uint32_t	uid;
	uint32_t	gid;
	uint32_t	size;
	uint32_t	atime;		/* sec's */
	uint32_t	ausec;		/* microsec's */
	uint32_t	mtime;
	uint32_t	musec;
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
	int32_t	timestamp;
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
	uint32_t	clientip;
	Unixidmap *map;
};

struct Xfile
{
	Xfile *	next;		/* hash chain */
	Session	*s;
	Qid		qid;	/* from stat */
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
	uint32_t	mode;		/* open mode, if opfid is non-zero */
	uint32_t	offset;
};

struct Fid
{
	Fid **	owner;		/* null for root fids */
	Fid *	prev;
	Fid *	next;
	int32_t	tstale;		/* auto-clunk */
};

enum
{
	Maxfdata = 8192,
	Maxstatdata = 2048,
};

struct Session
{
	Session *next;
	char *	service;		/* for dial */
	int	fd;
#define CHALLEN 1
	char	cchal[CHALLEN];		/* client challenge */
	char	schal[CHALLEN];		/* server challenge */
	char	authid[ANAMELEN];	/* server encryption uid */
	char	authdom[DOMLEN];	/* server encryption domain */
	char *	spec;			/* for attach */
	Xfile *	root;			/* to answer mount rpc */
	uint16_t	tag;
	Fcall	f;
	unsigned char	data[IOHDRSZ+Maxfdata];
	unsigned char	statbuf[Maxstatdata];
	Fid *	free;			/* available */
	Fid	list;			/* active, most-recently-used order */
	Fid	fids[1000];
	int	noauth;
};

struct Chalstuff
{
	Chalstuff *next;
	Xfid *	xf;
	int32_t	tstale;
	Chalstate;
};

extern int	rpcdebug;
extern int	p9debug;
extern int	chatty;
extern void	(*rpcalarm)(void);
extern int32_t	starttime;
extern int32_t	nfstime;
extern char *	config;
extern int	staletime;
extern int	messagesize;
extern char *	commonopts;
