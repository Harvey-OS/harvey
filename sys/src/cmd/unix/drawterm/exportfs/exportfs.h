/*
 * exportfs.h - definitions for exporting file server
 */

#define DEBUG		if(!dbg){}else fprint
#define DFD		2
#define fidhash(s)	fhash[s%FHASHSIZE]

#define Proc	Exproc


typedef struct Fsrpc Fsrpc;
typedef struct Fid Fid;
typedef struct File File;
typedef struct Proc Proc;
typedef struct Qidtab Qidtab;

struct Fsrpc
{
	int	busy;		/* Work buffer has pending rpc to service */
	int	pid;		/* Pid of slave process executing the rpc */
	int	canint;		/* Interrupt gate */
	int	flushtag;	/* Tag on which to reply to flush */
	Fcall work;		/* Plan 9 incoming Fcall */
	uchar	*buf;	/* Data buffer */
};

struct Fid
{
	int	fid;		/* system fd for i/o */
	File	*f;		/* File attached to this fid */
	int	mode;
	int	nr;		/* fid number */
	int	mid;		/* Mount id */
	Fid	*next;		/* hash link */
};

struct File
{
	char	*name;
	int	ref;
	Qid	qid;
	Qidtab	*qidt;
	int	inval;
	File	*parent;
	File	*child;
	File	*childlist;
};

struct Proc
{
	int	pid;
	int	busy;
	Proc	*next;
};

struct Qidtab
{
	int	ref;
	int	type;
	int	dev;
	vlong	path;
	vlong	uniqpath;
	Qidtab	*next;
};

enum
{
	MAXPROC		= 50,
	FHASHSIZE	= 64,
	Nr_workbufs 	= 50,
	Fidchunk	= 1000,
	Npsmpt		= 32,
	Nqidbits		= 5,
	Nqidtab		= (1<<Nqidbits),
};

#define Enomem Exenomem
#define Ebadfix Exebadfid
#define Enotdir Exenotdir
#define Edupfid Exedupfid
#define Eopen Exeopen
#define Exmnt Exexmnt
#define Emip Exemip
#define Enopsmt Exenopsmt

extern char Ebadfid[];
extern char Enotdir[];
extern char Edupfid[];
extern char Eopen[];
extern char Exmnt[];
extern char Enomem[];
extern char Emip[];
extern char Enopsmt[];

Extern Fsrpc	*Workq;
Extern int  	dbg;
Extern File	*root;
Extern File	*psmpt;
Extern Fid	**fhash;
Extern Fid	*fidfree;
Extern Proc	*Proclist;
Extern char	psmap[Npsmpt];
Extern Qidtab	*qidtab[Nqidtab];
Extern ulong	messagesize;
Extern int		srvfd;

/* File system protocol service procedures */
void Xattach(Fsrpc*);
void Xauth(Fsrpc*);
void Xclunk(Fsrpc*); 
void Xcreate(Fsrpc*);
void Xflush(Fsrpc*); 
void Xnop(Fsrpc*);
void Xremove(Fsrpc*);
void Xstat(Fsrpc*);
void Xversion(Fsrpc*);
void Xwalk(Fsrpc*);
void Xwstat(Fsrpc*);
void slave(Fsrpc*);

void	reply(Fcall*, Fcall*, char*);
Fid 	*getfid(int);
int	freefid(int);
Fid	*newfid(int);
Fsrpc	*getsbuf(void);
void	initroot(void);
void	fatal(char*, ...);
char*	makepath(File*, char*);
File	*file(File*, char*);
void	freefile(File*);
void	slaveopen(Fsrpc*);
void	slaveread(Fsrpc*);
void	slavewrite(Fsrpc*);
void	blockingslave(void*);
void	reopen(Fid *f);
void	noteproc(int, char*);
void	flushaction(void*, char*);
void	pushfcall(char*);
Qidtab* uniqueqid(Dir*);
void	freeqid(Qidtab*);
char*	estrdup(char*);
void*	emallocz(uint);
int		readmessage(int, char*, int);

#define notify(x)
#define noted(x)
#define exits(x)
