/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */


/*
 * Maps from ulongs to void*s.
 */
typedef struct Intmap	Intmap;


Intmap*	allocmap(void (*inc)(void*));
void	freemap(Intmap*, void (*destroy)(void*));
void*	lookupkey(Intmap*, u32);
void*	insertkey(Intmap*, u32, void*);
int	caninsertkey(Intmap*, u32, void*);
void*	deletekey(Intmap*, u32);

/*
 * Fid and Request structures.
 */
typedef struct Fid	Fid;
typedef struct Req	Req;
typedef struct Fidpool	Fidpool;
typedef struct Reqpool	Reqpool;
typedef struct File	File;
typedef struct Filelist	Filelist;
typedef struct Tree	Tree;
typedef struct Readdir	Readdir;
typedef struct Srv 	Srv;


struct Fid
{
	u32	fid;
	char		omode;	/* -1 = not open */
	File*		file;
	char*		uid;
	Qid		qid;
	void*		aux;

/* below is implementation-specific; don't use */
	Readdir*	rdir;
	Ref		ref;
	Fidpool*	pool;
	i64		diroffset;
	i32		dirindex;
};

struct Req
{
	u32	tag;
	void*		aux;
	Fcall		ifcall;
	Fcall		ofcall;
	Dir		d;
	Req*		oldreq;
	Fid*		fid;
	Fid*		afid;
	Fid*		newfid;
	Srv*		srv;

/* below is implementation-specific; don't use */
	QLock		lk;
	Ref		ref;
	Reqpool*	pool;
	u8 *	buf;
	u8		type;
	u8		responded;
	char*		error;
	void*		rbuf;
	Req**		flush;
	int		nflush;
};

/*
 * Pools to maintain Fid <-> fid and Req <-> tag maps.
 */

struct Fidpool {
	Intmap	*map;
	void	(*destroy)(Fid*);
	Srv	*srv;
};

struct Reqpool {
	Intmap	*map;
	void	(*destroy)(Req*);
	Srv	*srv;
};

Fidpool*	allocfidpool(void (*destroy)(Fid*));
void		freefidpool(Fidpool*);
Fid*		allocfid(Fidpool*, u32);
Fid*		lookupfid(Fidpool*, u32);
void		closefid(Fid*);
Fid*		removefid(Fidpool*, u32);

Reqpool*	allocreqpool(void (*destroy)(Req*));
void		freereqpool(Reqpool*);
Req*		allocreq(Reqpool*, u32);
Req*		lookupreq(Reqpool*, u32);
void		closereq(Req*);
Req*		removereq(Reqpool*, u32);

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
	RWLock 		RWLock;
	Filelist	*filelist;
	Tree 		*tree;
	int 		nchild;
	int 		allocd;
	int 		nxchild;
	Ref 		readers;
};

struct Tree {
	File 		*root;
	void		(*destroy)(File *file);

/* below is implementation-specific; don't use */
	Lock 		genlock;
	u32 	qidgen;
	u32 	dirqidgen;
};

Tree*		alloctree(char*, char*, u32, void(*destroy)(File*));
void		freetree(Tree*);
File*		createfile(File*, char*, char*, u32, void*);
int		removefile(File*);
void		closefile(File*);
File*		walkfile(File*, char*);
Readdir*	opendirfile(File*);
i32		readdirfile(Readdir*, u8*, i32);
void		closedirfile(Readdir*);

/*
 * Kernel-style command parser
 */
typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
Cmdbuf*	parsecmd(char *a, int n);
void	respondcmderror(Req*, Cmdbuf*, char*, ...);
Cmdtab*	lookupcmd(Cmdbuf*, Cmdtab*, int);
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
	void	(*destroyfid)(Fid*);
	void	(*destroyreq)(Req*);
	void	(*end)(Srv*);
	void*	aux;

	void	(*attach)(Req*);
	void	(*auth)(Req*);
	void	(*open)(Req*);
	void	(*create)(Req*);
	void	(*read)(Req*);
	void	(*write)(Req*);
	void	(*remove)(Req*);
	void	(*flush)(Req*);
	void	(*stat)(Req*);
	void	(*wstat)(Req*);
	void	(*walk)(Req*);
	char*	(*clone)(Fid*, Fid*);
	char*	(*walk1)(Fid*, char*, Qid*);

	int	infd;
	int	outfd;
	int	nopipe;
	int	srvfd;
	int	leavefdsopen;	/* magic for acme win */
	char*	keyspec;

/* below is implementation-specific; don't use */
	Fidpool*	fpool;
	Reqpool*	rpool;
	u32		msize;

	u8 *	rbuf;
	QLock		rlock;
	u8 *	wbuf;
	QLock		wlock;
	
	char*		addr;
};

void		srv(Srv*);
void		postmountsrv(Srv*, char*, char*, int);
void		_postmountsrv(Srv*, char*, char*, int);
void		postsharesrv(Srv*, char*, char*, char*);
void		_postsharesrv(Srv*, char*, char*, char*);
void		listensrv(Srv*, char*);
void		_listensrv(Srv*, char*);
int 		postfd(char*, int);
int		chatty9p;
void		respond(Req*, char*);
void		responderror(Req*);
void		threadpostmountsrv(Srv*, char*, char*, int);
void		threadpostsharesrv(Srv*, char*, char*, char*);
void		threadlistensrv(Srv *s, char *addr);

/*
 * Helper.  Assumes user is same as group.
 */
int	hasperm(File*, char*, int);

void*	emalloc9p(u32);
void*	erealloc9p(void*, u32);
char*	estrdup9p(char*);

enum {
	OMASK = 3
};

void	readstr(Req*, char*);
void	readbuf(Req*, void*, i32);
void	walkandclone(Req*, char*(*walk1)(Fid*,char*,void*), 
		char*(*clone)(Fid*,Fid*,void*), void*);

void	auth9p(Req*);
void	authread(Req*);
void	authwrite(Req*);
void	authdestroy(Fid*);
int	authattach(Req*);

extern void (*_forker)(void (*)(void*), void*, int);
