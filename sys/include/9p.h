#pragma src "/sys/src/lib9p"
#pragma lib "lib9p.a"

typedef struct Fid		Fid;
typedef struct Fidpool	Fidpool;
typedef struct File		File;
typedef struct Filelist	Filelist;
typedef struct Req		Req;
typedef struct Reqpool	Reqpool;
typedef struct Intmap	Intmap;
typedef struct Srv		Srv;
typedef struct Tree		Tree;

struct Fid
{
	ulong	fid;
	char	omode;	/* -1 = not open */
	File*		file;
	char		uid[NAMELEN];
	Qid		qid;
	void*	aux;

/* below is implementation-specific; don't use */
	Ref		ref;
	Fidpool*	pool;
};

struct Req
{
	Lock		taglock;
	ulong	tag;
	void*	aux;
	Fcall		fcall;
	Fcall		ofcall;

/* below is implementation-specific; don't use */
	Ref		ref;
	Reqpool*	pool;
	char*	buf;
	uchar	type;
	uchar	responded;
	Dir		d;
	char*	error;
	Fid*		fid;
	Fid*		newfid;
	Req*		oldreq;
	void*	rbuf;
};

struct Srv {
	Tree*	tree;
	void		(*attach)(Req*, Fid*, char*, Qid*);
	void		(*session)(Req*, char*, char*, char*);
	void		(*clone)(Req*, Fid*, Fid*);
	void		(*walk)(Req*, Fid*, char*, Qid*);
	void		(*open)(Req*, Fid*, int, Qid*);
	void		(*create)(Req*, Fid*, char*, int, ulong, Qid*);
	void		(*read)(Req*, Fid*, void*, long*, vlong);
	void		(*write)(Req*, Fid*, void*, long*, vlong);
	void		(*remove)(Req*, Fid*);
	void		(*stat)(Req*, Fid*, Dir*);
	void		(*wstat)(Req*, Fid*, Dir*);
	void		(*flush)(Req*, Req*);
	void		(*clunkaux)(Fid*);

/* below is implementation-specific; don't use */
	Fidpool*	fpool;
	Reqpool*	rpool;
	int	fd;

	void		*aux;
};

struct File {
	Dir;
	File *parent;
	void *aux;

/* below is implementation-specific; don't use */
	Ref		ref;
	QLock;
	Filelist *filelist;
	Tree *tree;
};

struct Tree {
	File *root;
	void	(*rmaux)(File *file);

/* below is implementation-specific; don't use */
	ulong qidgen;
	ulong dirqidgen;
};

/* fid.c */
Fidpool*	allocfidpool(void);
Fid*		allocfid(Fidpool*, ulong);
int		closefid(Fid*);
void		freefid(Fid*);
Fid*		lookupfid(Fidpool*, ulong);

/* file.c */
File*		fcreate(File*, char*, char*, char*, ulong);
void		fremove(File*);
File*		fwalk(File*, char*);
void		fdump(File*, int);
Tree*	mktree(char*, char*, ulong);
char*	fdirread(File*, char*, long*, vlong);
int		fclose(File*);

/* intmap.c */
Intmap*	allocmap(void (*inc)(void*));
void		freemap(Intmap*);
void*	lookupkey(Intmap*, ulong);
void*	insertkey(Intmap*, ulong, void*);
void*	lookupkey(Intmap*, ulong);
int		caninsertkey(Intmap*, ulong, void*);
void*	deletekey(Intmap*, ulong);

/* req.c */
Reqpool*	allocreqpool(void);
Req*		allocreq(Reqpool*, ulong);
int		closereq(Req*);
void		freereq(Req*);
Req*		lookupreq(Reqpool*, ulong);

/* srv.c */
void		srv(Srv*, int);
void		postmountsrv(Srv*, char*, char*, int);
int		lib9p_chatty;
void		respond(Req*, char*);

/* threadsrv.c */
void		threadpostmountsrv(Srv*, char*, char*, int);

/* uid.c */
int		hasperm(File*, char*, int);

/* util.c */
void*	_lib9p_emalloc(ulong);
void*	_lib9p_erealloc(void*, ulong);
char*	_lib9p_estrdup(char*);

enum {
	OMASK = 3
};

void readstr(vlong, void*, long*, char*);
void readbuf(vlong, void*, long*, void*, long);

/* crummy hack; need to work out better way */
extern void (*endsrv)(void*);

struct Reqpool {
	Intmap	*map;
	Srv		*srv;
};

