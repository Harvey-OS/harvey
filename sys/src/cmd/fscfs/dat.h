/*
 * fscfs
 */

typedef struct Attach Attach;
typedef struct Auth Auth;
typedef struct Data Data;
typedef struct Dref Dref;
typedef struct Fid Fid;
typedef struct File File;
typedef struct Host Host;
typedef struct Path Path;
typedef struct P9fs P9fs;
typedef struct Req Req;
typedef struct SFid SFid;
typedef struct String String;

#pragma incomplete P9fs

typedef u32int Tag;

struct String
{
	Ref;
	int	len;
	char*	s;
	String*	next;	/* hash chain */
};

struct Dref
{
	int	inuse;	/* reference count locally */
	int	faruse;	/* remote references */
	Tag	tag;	/* remote object tag */
	Host*	loc;	/* location of object (nil if here) */
	Host*	src;	/* source of object reference (nil if here) */
	int	depth;	/* 0 if here or loc == src */
	int	weight;	/* proxy weight */
};

enum{
	FidHash=	1<<8,
};

struct Attach
{
	/* parameters and result of a Tattach */
	u32int	fid;
	char*	uname;
	char*	aname;
	Path*	root;	/* qid from attach is root->qid, root->sfid is server fid */
	Attach*	next;
};

struct Auth
{
	/* could probably use a Ref */
	SFid*	afid;
	char*	uname;
	char*	aname;
	char*	error;
	Auth*	next;
	int	active;	/* active until attached, with or without success */
	Req*	pending;	/* later Auth requests with same parameters, and flush requests */
};

struct Host
{
	Ref;

	QLock;
	int	fd;	/* link to host */
	char*	name;	/* symbolic name (mainly for diagnostics) */
	Fid*	fids[FidHash];	/* fids active for this host */
	/* might need per-host auth/attach fid/afid for authentication? */
	/* implies separation of fid spaces, and thus separate Fids but not Files and Paths */
};

struct Path
{
	Ref;
	char*	name;
	Qid	qid;
	Path*	parent;
	Path*	child;
	Path*	next;	/* sibling */
	uint	nxtime;	/* zero (if exists) or last time we checked */
	char*	inval;	/* walk error if invalid */
	File*	file;		/* file data, if open */
	SFid*	sfid;	/* walked to this fid on server */
};

struct Data
{
	/* cached portion of a file */
	uint	min;		/* offsets */
	uint	max;
	uint	size;			/* size of buffer (power of 2) */
	uchar*	base;

	/* LRU stuff */
	Data*	forw;
	Data*	back;
	File*	owner;
	uint	n;		/* index in owner's cache */
};

struct SFid
{
	Ref;			/* by client fids */
	u32int	fid;	/* fid on server */
	SFid*	next;	/* on free or LRU list */
};

struct Fid
{
	u32int	fid;		/* fid on Host */
	Qid	qid;
	Path*	path;		/* shared data about file */
	SFid*	opened;	/* server fid once opened */
	uint	mode;	/* open mode (OREAD, OWRITE, ORDWR) */
	Fid*	next;	/* in fid hash list */
};

struct File
{
	Ref;

	SFid*	open[3];	/* cached sfids: OREAD, OWRITE, ORDWR */

	/* data from Dir */
	uint	mode;	/* permissions */
	uint	atime;	/* last read time */
	uint	mtime;	/* last write time */
	u64int	length;	/* file length from stat or 0 => use clength */
	String*	uid;	/* owner name */
	String*	gid;	/* group name */
	String*	muid;	/* last modifier name */

	Qid	qid;
	u32int	iounit;
	u64int	clength;	/* known length in cache */
	uint	ndata;	/* size of cache array */
	Data**	cached;

	/* Dref for local and remote references */
	/* possibly put expired ones on LRU? */
	File*	next;
};

struct Req
{
	u32int	tag;
	Fcall	t;
	SFid*	fid;	/* also afid in Tauth */
	SFid*	newfid;	/* also afid in Tattach */
	Fcall	r;
	uchar*	buf;
	uint	msize;
	Req*	flush;
	Req*	next;	/* in tag list, or flush list of another Req */
};
