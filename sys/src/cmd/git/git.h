#include <bio.h>
#include <mp.h>
#include <libsec.h>
#include <flate.h>
#include <regexp.h>

typedef struct Conn	Conn;
typedef struct Hash	Hash;
typedef struct Delta	Delta;
typedef struct Cinfo	Cinfo;
typedef struct Tinfo	Tinfo;
typedef struct Object	Object;
typedef struct Objset	Objset;
typedef struct Pack	Pack;
typedef struct Buf	Buf;
typedef struct Dirent	Dirent;
typedef struct Idxent	Idxent;
typedef struct Objlist	Objlist;
typedef struct Dtab	Dtab;
typedef struct Dblock	Dblock;

enum {
	Pathmax		= 512,
	Npackcache	= 32,
	Hashsz		= 20,
	Pktmax		= 65536,
};

enum {
	GNone	= 0,
	GCommit	= 1,
	GTree	= 2,
	GBlob	= 3,
	GTag	= 4,
	GOdelta	= 6,
	GRdelta	= 7,
};

enum {
	Cloaded	= 1 << 0,
	Cidx	= 1 << 1,
	Ccache	= 1 << 2,
	Cexist	= 1 << 3,
	Cparsed	= 1 << 5,
	Cthin	= 1 << 6,
};

enum {
	ConnGit,
	ConnGit9,
	ConnSsh,
	ConnHttp,
};

struct Objlist {
	int idx;

	int fd;
	int state;
	int stage;

	Dir *top;
	int ntop;
	int topidx;
	Dir *loose;
	int nloose;
	int looseidx;
	Dir *pack;
	int npack;
	int packidx;
	int nent;
	int entidx;
};

struct Hash {
	uchar h[20];
};

struct Conn {
	int type;
	int rfd;
	int wfd;

	/* only used by http */
	int cfd;
	char *url;	/* note, first GET uses a different url */
	char *dir;
	char *direction;
};

struct Dirent {
	char *name;
	int mode;
	Hash h;
	char ismod;
	char islink;
};

struct Object {
	/* Git data */
	Hash	hash;
	int	type;

	/* Cache */
	int	id;
	int	flag;
	int	refs;
	Object	*next;
	Object	*prev;

	/* For indexing */
	vlong	off;
	vlong	len;
	u32int	crc;

	/* Everything below here gets cleared */
	char	*all;
	char	*data;
	/* size excludes header */
	vlong	size;

	/* Significant win on memory use */
	union {
		Cinfo	*commit;
		Tinfo	*tree;
	};
};

struct Tinfo {
	/* Tree */
	Dirent	*ent;
	int	nent;
};

struct Cinfo {
	/* Commit */
	Hash	*parent;
	int	nparent;
	Hash	tree;
	char	*author;
	char	*committer;
	char	*msg;
	int	nmsg;
	vlong	ctime;
	vlong	mtime;
};

struct Objset {
	Object	**obj;
	int	nobj;
	int	sz;
};

struct Dtab {
	Object	*o;
	uchar	*base;
	int	nbase;
	Dblock	*b;
	int	nb;
	int	sz;
};

struct Dblock {
	uchar	*buf;
	int	len;
	int	off;
	u64int	hash;
};

struct Delta {
	int	cpy;
	int	off;
	int	len;
};


#define GETBE16(b)\
		((((b)[0] & 0xFFul) <<  8) | \
		 (((b)[1] & 0xFFul) <<  0))

#define GETBE32(b)\
		((((b)[0] & 0xFFul) << 24) | \
		 (((b)[1] & 0xFFul) << 16) | \
		 (((b)[2] & 0xFFul) <<  8) | \
		 (((b)[3] & 0xFFul) <<  0))
#define GETBE64(b)\
		((((b)[0] & 0xFFull) << 56) | \
		 (((b)[1] & 0xFFull) << 48) | \
		 (((b)[2] & 0xFFull) << 40) | \
		 (((b)[3] & 0xFFull) << 32) | \
		 (((b)[4] & 0xFFull) << 24) | \
		 (((b)[5] & 0xFFull) << 16) | \
		 (((b)[6] & 0xFFull) <<  8) | \
		 (((b)[7] & 0xFFull) <<  0))

#define PUTBE16(b, n)\
	do{ \
		(b)[0] = (n) >> 8; \
		(b)[1] = (n) >> 0; \
	} while(0)

#define PUTBE32(b, n)\
	do{ \
		(b)[0] = (n) >> 24; \
		(b)[1] = (n) >> 16; \
		(b)[2] = (n) >> 8; \
		(b)[3] = (n) >> 0; \
	} while(0)

#define PUTBE64(b, n)\
	do{ \
		(b)[0] = (n) >> 56; \
		(b)[1] = (n) >> 48; \
		(b)[2] = (n) >> 40; \
		(b)[3] = (n) >> 32; \
		(b)[4] = (n) >> 24; \
		(b)[5] = (n) >> 16; \
		(b)[6] = (n) >> 8; \
		(b)[7] = (n) >> 0; \
	} while(0)

#define QDIR(qid)	((int)(qid)->path & (0xff))
#define isblank(c) \
	(((c) != '\n') && isspace(c))

extern Reprog	*authorpat;
extern Objset	objcache;
extern Hash	Zhash;
extern int	chattygit;
extern int	cachemax;
extern int	interactive;

#pragma varargck type "H" Hash
#pragma varargck type "T" int
#pragma varargck type "O" Object*
#pragma varargck type "Q" Qid
int Hfmt(Fmt*);
int Tfmt(Fmt*);
int Ofmt(Fmt*);
int Qfmt(Fmt*);

void gitinit(void);

/* object io */
int	resolverefs(Hash **, char *);
int	resolveref(Hash *, char *);
int	listrefs(Hash **, char ***);
Object	*ancestor(Object *, Object *);
int	findtwixt(Hash *, int, Hash *, int, Object ***, int *);
Object	*readobject(Hash);
Object	*clearedobject(Hash, int);
void	parseobject(Object *);
int	indexpack(char *, char *, Hash);
int	writepack(int, Hash*, int, Hash*, int, Hash*);
int	hasheq(Hash *, Hash *);
Object	*ref(Object *);
void	unref(Object *);
void	cache(Object *);
Object	*emptydir(void);

/* object sets */
void	osinit(Objset *);
void	osclear(Objset *);
void	osadd(Objset *, Object *);
int	oshas(Objset *, Hash);
Object	*osfind(Objset *, Hash);

/* object listing */
Objlist	*mkols(void);
int	olsnext(Objlist *, Hash *);
void	olsfree(Objlist *);

/* util functions */
#define dprint(lvl, ...) \
	if(chattygit >= lvl) _dprint(__VA_ARGS__)
void	_dprint(char *, ...);
void	*eamalloc(ulong, ulong);
void	*emalloc(ulong);
void	*earealloc(void *, ulong, ulong);
void	*erealloc(void *, ulong);
char	*estrdup(char *);
int	slurpdir(char *, Dir **);
int	hparse(Hash *, char *);
int	hassuffix(char *, char *);
int	swapsuffix(char *, int, char *, char *, char *);
char	*strip(char *);
int	findrepo(char *, int);
int	showprogress(int, int);

/* packing */
void	dtinit(Dtab *, Object*);
void	dtclear(Dtab*);
Delta*	deltify(Object*, Dtab*, int*);

/* proto handling */
int	readpkt(Conn*, char*, int);
int	writepkt(Conn*, char*, int);
int	flushpkt(Conn*);
void	initconn(Conn*, int, int);
int	gitconnect(Conn *, char *, char *);
int	readphase(Conn *);
int	writephase(Conn *);
void	closeconn(Conn *);
