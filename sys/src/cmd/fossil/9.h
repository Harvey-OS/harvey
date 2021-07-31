#include <auth.h>
#include <fcall.h>

enum {
	NFidHash	= 503,
};

typedef struct Con Con;
typedef struct DirBuf DirBuf;
typedef struct Excl Excl;
typedef struct Fid Fid;
typedef struct Fsys Fsys;
typedef struct Msg Msg;

struct Msg {
	uchar*	data;
	u32int	msize;			/* actual size of data */
	Fcall	t;
	Fcall	r;
	Con*	con;
	int	flush;

	Msg*	next;
	Msg*	prev;
};

struct Con {
	VtLock*	lock;
	int	fd;
	char*	name;
	uchar*	data;			/* max, not negotiated */
	u32int	msize;			/* negotiated with Tversion */
	int	state;
	int	aok;
	Msg*	version;
	int	isconsole;

	Msg*	mhead;			/* active queue */
	Msg*	mtail;
	VtRendez* active;
	int	nmsg;

	VtLock*	fidlock;		/* */
	Fid*	fidhash[NFidHash];
	Fid*	fhead;
	Fid*	ftail;
	int	nfid;
};

enum {
	CsDead,
	CsNew,
	CsDown,
	CsInit,
	CsUp,
	CsMoribund,
};

struct Fid {
	VtLock*	lock;
	Con*	con;
	u32int	fidno;
	int	ref;			/* inc/dec under Con.fidlock */
	int	flags;

	int	open;
	File*	file;
	Qid	qid;
	char*	uid;
	char*	uname;
	DirBuf*	db;
	Excl*	excl;

	VtLock*	alock;			/* Tauth/Tattach */
	AuthRpc* rpc;
	Fsys*	fsys;
	char*	cuname;

	Fid*	hash;			/* lookup by fidno */
	Fid*	next;			/* clunk session with Tversion */
	Fid*	prev;
};

enum {					/* Fid.flags and fidGet(..., flags) */
	FidFCreate	= 0x01,
	FidFWlock	= 0x02,
};

enum {					/* Fid.open */
	FidOCreate	= 0x01,
	FidORead	= 0x02,
	FidOWrite	= 0x04,
	FidORclose	= 0x08,
};

/*
 * 9p.c
 */
extern int (*rFcall[Tmax])(Msg*);
extern int validFileName(char*);

/*
 * 9auth.c
 */
extern int authCheck(Fcall*, Fid*, Fsys*);
extern int authRead(Fid*, void*, int);
extern int authWrite(Fid*, void*, int);

/*
 * 9dir.c
 */
extern void dirBufFree(DirBuf*);
extern int dirDe2M(DirEntry*, uchar*, int);
extern int dirRead(Fid*, uchar*, int, vlong);

/*
 * 9excl.c
 */
extern int exclAlloc(Fid*);
extern void exclFree(Fid*);
extern void exclInit(void);
extern int exclUpdate(Fid*);

/*
 * 9fid.c
 */
extern void fidClunk(Fid*);
extern Fid* fidGet(Con*, u32int, int);
extern void fidInit(void);
extern void fidPut(Fid*);

/*
 * 9fsys.c
 */
extern Fsys* fsysGet(char*);
extern Fs* fsysGetFs(Fsys*);
extern void fsysFsRlock(Fsys*);
extern void fsysFsRUnlock(Fsys*);
extern File* fsysGetRoot(Fsys*, char*);
extern Fsys* fsysIncRef(Fsys*);
extern int fsysInit(void);
extern int fsysNoAuthCheck(Fsys*);
extern int fsysNoPermCheck(Fsys*);
extern void fsysPut(Fsys*);
extern int fsysWstatAllow(Fsys*);
extern char* fsysGetName(Fsys*);

/*
 * 9lstn.c
 */
extern int lstnInit(void);

/*
 * 9proc.c
 */
extern Con* conAlloc(int, char*);
extern void procInit(void);

/*
 * 9srv.c
 */
extern int srvInit(void);

/*
 * 9user.c
 */
extern int groupLeader(char*, char*);
extern int groupMember(char*, char*);
extern int groupWriteMember(char*);
extern char* unameByUid(char*);
extern char* uidByUname(char*);
extern int usersInit(void);
extern int validUserName(char*);

extern char* uidadm;
extern char* unamenone;
extern char* uidnoworld;

/*
 * Ccli.c
 */
extern int cliAddCmd(char*, int (*)(int, char*[]));
extern int cliError(char*, ...);
extern int cliInit(void);
extern int cliExec(char*);

/*
 * Ccmd.c
 */
extern int cmdInit(void);

/*
 * Ccons.c
 */
extern int consPrompt(char*);
extern int consInit(void);
extern int consOpen(int, int, int);
extern int consTTY(void);
extern int consWrite(char*, int);

/*
 * Clog.c
 */
extern int consPrint(char*, ...);
extern int consVPrint(char*, va_list);

/*
 * fossil.c
 */
extern int Dflag;
