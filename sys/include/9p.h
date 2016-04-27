/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma src "/sys/src/lib9p"
#pragma lib "lib9p.a"

/*
 * Maps from ulongs to void*s.
 */
typedef struct Intmap	Intmap;

#pragma incomplete Intmap

Intmap*	allocmap(void (*inc)(void*));
void		freemap(Intmap*, void (*destroy)(void*));
void*	lookupkey(Intmap*, uint32_t);
void*	insertkey(Intmap*, uint32_t, void*);
int		caninsertkey(Intmap*, uint32_t, void*);
void*	deletekey(Intmap*, uint32_t);

/*
 * Fid and Request structures.
 */
typedef struct Fid		Fid;
typedef struct Req		Req;
typedef struct Fidpool	Fidpool;
typedef struct Reqpool	Reqpool;
typedef struct File		File;
typedef struct Filelist	Filelist;
typedef struct Tree		Tree;
typedef struct Readdir	Readdir;
typedef struct Srv Srv;

#pragma incomplete Filelist
#pragma incomplete Readdir

struct Fid
{
	uint32_t	fid;
	char		omode;	/* -1 = not open */
	File*		file;
	char*	uid;
	Qid		qid;
	void*	aux;

/* below is implementation-specific; don't use */
	Readdir*	rdir;
	Ref		ref;
	Fidpool*	pool;
	int64_t	diroffset;
	int32_t		dirindex;
};

struct Req
{
	uint32_t	tag;
	void*	aux;
	Fcall		ifcall;
	Fcall		ofcall;
	Dir		d;
	Req*		oldreq;
	Fid*		fid;
	Fid*		afid;
	Fid*		newfid;
	Srv*		srv;

/* below is implementation-specific; don't use */
	QLock	lk;
	Ref		ref;
	Reqpool*	pool;
	uint8_t*	buf;
	uint8_t	type;
	uint8_t	responded;
	char*	error;
	void*	rbuf;
	Req**	flush;
	int		nflush;
};

/*
 * Pools to maintain Fid <-> fid and Req <-> tag maps.
 */

struct Fidpool {
	Intmap	*map;
	void		(*destroy)(Fid*);
	Srv		*srv;
};

struct Reqpool {
	Intmap	*map;
	void		(*destroy)(Req*);
	Srv		*srv;
};

Fidpool*	allocfidpool(void (*destroy)(Fid*));
void		freefidpool(Fidpool*);
Fid*		allocfid(Fidpool*, uint32_t);
Fid*		lookupfid(Fidpool*, uint32_t);
void		closefid(Fid*);
Fid*		removefid(Fidpool*, uint32_t);

Reqpool*	allocreqpool(void (*destroy)(Req*));
void		freereqpool(Reqpool*);
Req*		allocreq(Reqpool*, uint32_t);
Req*		lookupreq(Reqpool*, uint32_t);
void		closereq(Req*);
Req*		removereq(Reqpool*, uint32_t);

typedef	int	Dirgen(int, Dir*, void*);
void		dirread9p(Req*, Dirgen*, void*);

/*
 * File trees.
 */
struct File {
	Ref Ref;
	Dir Dir;
	File *parent;
	void *aux;

/* below is implementation-specific; don't use */
	RWLock RWLock;
	Filelist *filelist;
	Tree *tree;
	int nchild;
	int allocd;
	int nxchild;
	Ref readers;
};

struct Tree {
	File *root;
	void	(*destroy)(File *file);

/* below is implementation-specific; don't use */
	Lock genlock;
	uint32_t qidgen;
	uint32_t dirqidgen;
};

Tree*	alloctree(char*, char*, uint32_t, void(*destroy)(File*));
void		freetree(Tree*);
File*		createfile(File*, char*, char*, uint32_t, void*);
int		removefile(File*);
void		closefile(File*);
File*		walkfile(File*, char*);
Readdir*	opendirfile(File*);
int32_t		readdirfile(Readdir*, uint8_t*, int32_t);
void		closedirfile(Readdir*);

/*
 * Kernel-style command parser
 */
typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
Cmdbuf*		parsecmd(char *a, int n);
void		respondcmderror(Req*, Cmdbuf*, char*, ...);
Cmdtab*	lookupcmd(Cmdbuf*, Cmdtab*, int);
#pragma varargck argpos respondcmderr 3
struct Cmdbuf
{
	char	*buf;
	char	**f;
	int	nf;
};

struct Cmdtab
{
	int	index;	/* used by client to switch on result */
	char	*cmd;	/* command name */
	int	narg;	/* expected #args; 0 ==> variadic */
};

/*
 * File service loop.
 */
struct Srv {
	Tree*	tree;
	void		(*destroyfid)(Fid*);
	void		(*destroyreq)(Req*);
	void		(*end)(Srv*);
	void*	aux;

	void		(*attach)(Req*);
	void		(*auth)(Req*);
	void		(*open)(Req*);
	void		(*create)(Req*);
	void		(*read)(Req*);
	void		(*write)(Req*);
	void		(*remove)(Req*);
	void		(*flush)(Req*);
	void		(*stat)(Req*);
	void		(*wstat)(Req*);
	void		(*walk)(Req*);
	char*	(*clone)(Fid*, Fid*);
	char*	(*walk1)(Fid*, char*, Qid*);

	int		infd;
	int		outfd;
	int		nopipe;
	int		srvfd;
	int		leavefdsopen;	/* magic for acme win */
	char*	keyspec;

/* below is implementation-specific; don't use */
	Fidpool*	fpool;
	Reqpool*	rpool;
	uint		msize;

	uint8_t*	rbuf;
	QLock	rlock;
	uint8_t*	wbuf;
	QLock	wlock;
	
	char*	addr;
};

void		srv(Srv*);
void		postmountsrv(Srv*, char*, char*, int);
void		_postmountsrv(Srv*, char*, char*, int);
void		listensrv(Srv*, char*);
void		_listensrv(Srv*, char*);
int 		postfd(char*, int);
int		chatty9p;
void		respond(Req*, char*);
void		responderror(Req*);
void		threadpostmountsrv(Srv*, char*, char*, int);
void		threadlistensrv(Srv *s, char *addr);

/*
 * Helper.  Assumes user is same as group.
 */
int		hasperm(File*, char*, int);

void*	emalloc9p(uint32_t);
void*	erealloc9p(void*, uint32_t);
char*	estrdup9p(char*);

enum {
	OMASK = 3
};

void		readstr(Req*, char*);
void		readbuf(Req*, void*, int32_t);
void		walkandclone(Req*, char*(*walk1)(Fid*,char*,void*), 
			char*(*clone)(Fid*,Fid*,void*), void*);

void		auth9p(Req*);
void		authread(Req*);
void		authwrite(Req*);
void		authdestroy(Fid*);
int		authattach(Req*);

extern void (*_forker)(void (*)(void*), void*, int);

