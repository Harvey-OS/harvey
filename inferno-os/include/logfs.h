#pragma src "/usr/inferno/liblogfs"

typedef struct LogfsLowLevel LogfsLowLevel;
typedef struct LogfsBoot LogfsBoot;
typedef struct Logfs Logfs;
typedef struct LogfsServer LogfsServer;
typedef struct LogfsIdentityStore LogfsIdentityStore;

#pragma incomplete Logfs
#pragma incomplete LogfsServer
#pragma incomplete LogfsIdentityStore
#pragma incomplete LogfsBoot

typedef u64int Pageset;

#define	BITSPERSET	(sizeof(Pageset)*8)
#define	PAGETOP	((Pageset)1<<(BITSPERSET-1))

enum {
	LogfsTnone = 0xff,
	LogfsTboot = 0x01,
	LogfsTlog = 0x06,
	LogfsTdata = 0x18,
	LogfsTbad = -1,
	LogfsTworse = -2,
	LogfsMagic = 'V',
};

enum {
	LogfsLogTstart = 's',
	LogfsLogTcreate = 'c',
	LogfsLogTtrunc = 't',
	LogfsLogTremove = 'r',
	LogfsLogTwrite = 'w',
	LogfsLogTwstat = 'W',
	LogfsLogTend = 'e',
};

enum {
	LogfsOpenFlagNoPerm = 1,
	LogfsOpenFlagWstatAllow = 2,
};

typedef enum LogfsLowLevelReadResult {
	LogfsLowLevelReadResultOk,
	LogfsLowLevelReadResultSoftError,
	LogfsLowLevelReadResultHardError,
	LogfsLowLevelReadResultBad,
	LogfsLowLevelReadResultAllOnes,
} LogfsLowLevelReadResult;

typedef short LOGFSGETBLOCKTAGFN(LogfsLowLevel*, long);
typedef void LOGFSSETBLOCKTAGFN(LogfsLowLevel*, long, short);
typedef ulong LOGFSGETBLOCKPATHFN(LogfsLowLevel*, long);
typedef void LOGFSSETBLOCKPATHFN(LogfsLowLevel*, long, ulong);
typedef long LOGFSFINDFREEBLOCKFN(LogfsLowLevel*, long*);
typedef char *LOGFSREADBLOCKFN(LogfsLowLevel*, void*, long, LogfsLowLevelReadResult*);
typedef char *LOGFSWRITEBLOCKFN(LogfsLowLevel*, void*, uchar, ulong, int, long*, long);
typedef char *LOGFSERASEBLOCKFN(LogfsLowLevel*, long, void **, int*);
typedef char *LOGFSFORMATBLOCKFN(LogfsLowLevel*, long, uchar, long, long, long, int, long*, void*, int*);
typedef char *LOGFSREFORMATBLOCKFN(LogfsLowLevel*, long, uchar, long, int, long*, void*, int*);
typedef void LOGFSMARKBLOCKBADFN(LogfsLowLevel*, long);
typedef int LOGFSGETBLOCKSFN(LogfsLowLevel*);
typedef long LOGFSGETBASEBLOCKFN(LogfsLowLevel*);
typedef int LOGFSGETBLOCKSIZEFN(LogfsLowLevel*);
typedef int LOGFSGETBLOCKPARTIALFORMATSTATUSFN(LogfsLowLevel*, long);
typedef ulong LOGFSCALCRAWADDRESSFN(LogfsLowLevel*, long, int);
typedef char *LOGFSOPENFN(LogfsLowLevel*, long, long, int, int, long*);
typedef char *LOGFSGETBLOCKSTATUSFN(LogfsLowLevel*, long, int*, void **, LogfsLowLevelReadResult*);
typedef int LOGFSCALCFORMATFN(LogfsLowLevel*, long, long, long, long*, long*, long*);
typedef int LOGFSGETOPENSTATUSFN(LogfsLowLevel*);
typedef void LOGFSFREEFN(LogfsLowLevel*);
typedef char *LOGFSREADPAGERANGEFN(LogfsLowLevel*, uchar*, long, int, int, int, LogfsLowLevelReadResult*);
typedef char *LOGFSWRITEPAGEFN(LogfsLowLevel*, uchar*, long, int);
typedef char *LOGFSSYNCFN(LogfsLowLevel*);

struct LogfsLowLevel {
	int l2pagesize;
	int l2pagesperblock;
	long blocks;
	int pathbits;
	LOGFSOPENFN *open;
	LOGFSGETBLOCKTAGFN *getblocktag;
	LOGFSSETBLOCKTAGFN *setblocktag;
	LOGFSGETBLOCKPATHFN *getblockpath;
	LOGFSSETBLOCKPATHFN *setblockpath;
	LOGFSREADPAGERANGEFN *readpagerange;
	LOGFSWRITEPAGEFN *writepage;
	LOGFSFINDFREEBLOCKFN *findfreeblock;
	LOGFSREADBLOCKFN *readblock;
	LOGFSWRITEBLOCKFN *writeblock;
	LOGFSERASEBLOCKFN *eraseblock;
	LOGFSFORMATBLOCKFN *formatblock;
	LOGFSREFORMATBLOCKFN *reformatblock;
	LOGFSMARKBLOCKBADFN *markblockbad;
	LOGFSGETBASEBLOCKFN *getbaseblock;
	LOGFSGETBLOCKSIZEFN *getblocksize;
	LOGFSGETBLOCKPARTIALFORMATSTATUSFN *getblockpartialformatstatus;
	LOGFSCALCRAWADDRESSFN *calcrawaddress;
	LOGFSGETBLOCKSTATUSFN *getblockstatus;
	LOGFSCALCFORMATFN *calcformat;
	LOGFSGETOPENSTATUSFN *getopenstatus;
	LOGFSFREEFN *free;
	LOGFSSYNCFN *sync;
};

char *logfstagname(uchar);

char *logfsisnew(LogfsIdentityStore **);
void logfsisfree(LogfsIdentityStore **);
char *logfsisgroupcreate(LogfsIdentityStore*, char*, char*);
char *logfsisgrouprename(LogfsIdentityStore*, char*, char*);
char *logfsisgroupsetleader(LogfsIdentityStore*, char*, char*);
char *logfsisgroupaddmember(LogfsIdentityStore*, char*, char*);
char *logfsisgroupremovemember(LogfsIdentityStore*, char*, char*);
char *logfsisusersread(LogfsIdentityStore*, void*, long, ulong, long*);

char *logfsformat(LogfsLowLevel*, long, long, long, int);
char *logfsbootopen(LogfsLowLevel*, long, long, int, int, LogfsBoot**);
void logfsbootfree(LogfsBoot*);
char *logfsbootread(LogfsBoot*, void*, long, ulong);
char *logfsbootwrite(LogfsBoot*, void*, long, ulong);
char *logfsbootio(LogfsBoot*, void*, long, ulong, int);
char *logfsbootmap(LogfsBoot*, ulong, ulong*, int*, int*, int*, ulong*, ulong*);
long logfsbootgetiosize(LogfsBoot*);
long logfsbootgetsize(LogfsBoot*);
void logfsboottrace(LogfsBoot*, int);

char *logfsserverattach(LogfsServer*, u32int, char*, Qid*);
char *logfsserverclunk(LogfsServer*, u32int);
char *logfsservercreate(LogfsServer*, u32int, char*, u32int, uchar, Qid*);
char *logfsserverflush(LogfsServer*);
char *logfsservernew(LogfsBoot*, LogfsLowLevel*, LogfsIdentityStore*, ulong, int, LogfsServer**);
char *logfsserveropen(LogfsServer*, u32int, uchar mode, Qid*);
char *logfsserverread(LogfsServer*, u32int, u32int, u32int, uchar*, u32int, u32int*);
char *logfsserverremove(LogfsServer*, u32int);
char *logfsserverstat(LogfsServer*, u32int, uchar*, u32int, ushort*);
char *logfsserverwalk(LogfsServer*, u32int, u32int, ushort, char **, ushort*, Qid*);
char *logfsserverwrite(LogfsServer*, u32int, u32int, u32int, uchar*, u32int*);
char *logfsserverwstat(LogfsServer*, u32int, uchar*, ushort nstat);
void logfsserverfree(LogfsServer **);
char *logfsserverlogsweep(LogfsServer*, int, int*);
char *logfsserverreadpathextent(LogfsServer*, u32int, int, u32int*, u32int*, long*, int*, int*);

char *logfsservertestcmd(LogfsServer*, int, char **);
void logfsservertrace(LogfsServer*, int);

/*
 * implemented by the environment
 */
ulong logfsnow(void);
void *logfsrealloc(void*, ulong);
void logfsfreemem(void*);
int	nrand(int);

extern char Eio[];
extern char Ebadarg[];
extern char Eperm[];
