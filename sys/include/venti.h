/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */



/* XXX should be own library? */
/*
 * Packets
 */
enum
{
	MaxFragSize = 9*1024
};

typedef struct Packet Packet;

Packet*	packetalloc(void);
void	packetappend(Packet*, u8 *buf, int n);
u32	packetasize(Packet*);
int	packetcmp(Packet*, Packet*);
int	packetcompact(Packet*);
void	packetconcat(Packet*, Packet*);
int	packetconsume(Packet*, u8 *buf, int n);
int	packetcopy(Packet*, u8 *buf, int offset, int n);
Packet*	packetdup(Packet*, int offset, int n);
Packet*	packetforeign(u8 *buf, int n, void (*free)(void *a),
			     void *a);
int	packetfragments(Packet*, IOchunk*, int nio, int offset);
void	packetfree(Packet*);
u8 *	packetheader(Packet*, int n);
u8 *	packetpeek(Packet*, u8 *buf, int offset, int n);
void	packetprefix(Packet*, u8 *buf, int n);
void	packetsha1(Packet*, u8 sha1[20]);
u32	packetsize(Packet*);
Packet*	packetsplit(Packet*, int n);
void	packetstats(void);
u8 *	packettrailer(Packet*, int n);
int	packettrim(Packet*, int offset, int n);

/* XXX should be own library? */
/*
 * Logging
 */
typedef struct VtLog VtLog;
typedef struct VtLogChunk VtLogChunk;

struct VtLog
{
	VtLog	*next;		/* in hash table */
	char	*name;
	VtLogChunk *chunk;
	u32	nchunk;
	VtLogChunk *w;
	QLock	lk;
	int	ref;
};

struct VtLogChunk
{
	char	*p;
	char	*ep;
	char	*wp;
};

VtLog*	vtlogopen(char *name, u32 size);
void	vtlogprint(VtLog *log, char *fmt, ...);
void	vtlog(char *name, char *fmt, ...);
void	vtlogclose(VtLog*);
void	vtlogremove(char *name);
char**	vtlognames(int*);
void	vtlogdump(int fd, VtLog*);

/* XXX begin actual venti.h */

typedef struct VtFcall VtFcall;
typedef struct VtConn VtConn;
typedef struct VtEntry VtEntry;
typedef struct VtRoot VtRoot;

/*
 * Fundamental constants.
 */
enum
{
	VtScoreSize	= 20,
	VtMaxStringSize = 1024,
	VtMaxLumpSize	= 56*1024,
	VtPointerDepth	= 7
};
#define VtMaxFileSize ((1ULL<<48)-1)


/* 
 * Strings in packets.
 */
int vtputstring(Packet*, char*);
int vtgetstring(Packet*, char**);

/*
 * Block types.
 * 
 * The initial Venti protocol had a much
 * less regular list of block types.
 * VtToDiskType converts from new to old.
 */
enum
{
	VtDataType	= 0<<3,
	/* VtDataType+1, ... */
	VtDirType	= 1<<3,
	/* VtDirType+1, ... */
	VtRootType	= 2<<3,
	VtMaxType,
	VtCorruptType = 0xFF,

	VtTypeDepthMask = 7,
	VtTypeBaseMask = ~VtTypeDepthMask
};

/* convert to/from on-disk type numbers */
u32 vttodisktype(u32);
u32 vtfromdisktype(u32);

/*
 * VtEntry describes a Venti stream
 *
 * The _ enums are only used on the wire.
 * They are not present in the VtEntry structure
 * and should not be used by client programs.
 * (The info is in the type field.)
 */
enum
{
	VtEntryActive = 1<<0,		/* entry is in use */
	_VtEntryDir = 1<<1,		/* a directory */
	_VtEntryDepthShift = 2,		/* shift for pointer depth */
	_VtEntryDepthMask = 7<<2,	/* mask for pointer depth */
	VtEntryLocal = 1<<5		/* for local storage only */
};
enum
{
	VtEntrySize = 40
};
struct VtEntry
{
	u32	gen;			/* generation number */
	u16	psize;			/* pointer block size */
	u16	dsize;			/* data block size */
	u8	type;
	u8	flags;
	u64	size;
	u8	score[VtScoreSize];
};

void vtentrypack(VtEntry*, u8*, int index);
int vtentryunpack(VtEntry*, u8*, int index);

struct VtRoot
{
	char	name[128];
	char	type[128];
	u8	score[VtScoreSize];	/* to a Dir block */
	u16	blocksize;		/* maximum block size */
	u8	prev[VtScoreSize];	/* last root block */
};

enum
{
	VtRootSize = 300,
	VtRootVersion = 2
};

void vtrootpack(VtRoot*, u8*);
int vtrootunpack(VtRoot*, u8*);

/*
 * score of zero length block
 */
extern u8 vtzeroscore[VtScoreSize];

/*
 * zero extend and truncate blocks
 */
void vtzeroextend(int type, u8 *buf, u32 n, u32 nn);
u32 vtzerotruncate(int type, u8 *buf, u32 n);

/*
 * parse score: mungs s
 */
int vtparsescore(char *s, char **prefix, u8[VtScoreSize]);

/*
 * formatting
 * other than noted, these formats all ignore
 * the width and precision arguments, and all flags
 *
 * V	a venti score
 */

int vtscorefmt(Fmt*);

/*
 * error-checking malloc et al.
 */
void	vtfree(void *);
void*	vtmalloc(int);
void*	vtmallocz(int);
void*	vtrealloc(void *p, int);
void*	vtbrk(int n);
char*	vtstrdup(char *);

/*
 * Venti protocol
 */

/*
 * Crypto strengths
 */
enum
{
	VtCryptoStrengthNone,
	VtCryptoStrengthAuth,
	VtCryptoStrengthWeak,
	VtCryptoStrengthStrong
};

/*
 * Crypto suites
 */
enum
{
	VtCryptoNone,
	VtCryptoSSL3,
	VtCryptoTLS1,
	VtCryptoMax
};

/* 
 * Codecs
 */
enum
{
	VtCodecNone,
	VtCodecDeflate,
	VtCodecThwack,
	VtCodecMax
};

enum
{
	VtRerror	= 1,
	VtTping		= 2,
	VtRping,
	VtThello	= 4,
	VtRhello,
	VtTgoodbye	= 6,
	VtRgoodbye,	/* not used */
	VtTauth0	= 8,
	VtRauth0,
	VtTauth1	= 10,
	VtRauth1,
	VtTread		= 12,
	VtRread,
	VtTwrite	= 14,
	VtRwrite,
	VtTsync		= 16,
	VtRsync,

	VtTmax
};

struct VtFcall
{
	u8	msgtype;
	u8	tag;

	char	*error;		/* Rerror */

	char	*version;	/* Thello */
	char	*uid;		/* Thello */
	u8	strength;	/* Thello */
	u8	*crypto;	/* Thello */
	u32	ncrypto;	/* Thello */
	u8	*codec;		/* Thello */
	u32	ncodec;		/* Thello */
	char	*sid;		/* Rhello */
	u8	rcrypto;	/* Rhello */
	u8	rcodec;		/* Rhello */
	u8	*auth;		/* TauthX, RauthX */
	u32	nauth;		/* TauthX, RauthX */
	u8	score[VtScoreSize];	/* Tread, Rwrite */
	u8	blocktype;	/* Tread, Twrite */
	u16	count;		/* Tread */
	Packet	*data;		/* Rread, Twrite */
};

Packet*	vtfcallpack(VtFcall*);
int	vtfcallunpack(VtFcall*, Packet*);
void	vtfcallclear(VtFcall*);
int	vtfcallfmt(Fmt*);

enum
{
	VtStateAlloc,
	VtStateConnected,
	VtStateClosed
};

struct VtConn
{
	QLock	lk;
	QLock	inlk;
	QLock	outlk;
	int	debug;
	int	infd;
	int	outfd;
	int	muxer;
	void	*writeq;
	void	*readq;
	int	state;
	void	*wait[256];
	u32	ntag;
	u32	nsleep;
	Packet	*part;
	Rendez	tagrend;
	Rendez	rpcfork;
	char	*version;
	char	*uid;
	char	*sid;
	char	addr[256];	/* address of other side */
};

VtConn*	vtconn(int infd, int outfd);
VtConn*	vtdial(char*);
void	vtfreeconn(VtConn*);
int	vtsend(VtConn*, Packet*);
Packet*	vtrecv(VtConn*);
int	vtversion(VtConn* z);
void	vtdebug(VtConn* z, char*, ...);
void	vthangup(VtConn* z);
int	vtgoodbye(VtConn* z);

/* #pragma varargck argpos vtdebug 2 */

/* server */
typedef struct VtSrv VtSrv;
typedef struct VtReq VtReq;
struct VtReq
{
	VtFcall	tx;
	VtFcall	rx;
/* private */
	VtSrv	*srv;
	void	*sc;
};

int	vtsrvhello(VtConn*);
VtSrv*	vtlisten(char *addr);
VtReq*	vtgetreq(VtSrv*);
void	vtrespond(VtReq*);

/* client */
Packet*	vtrpc(VtConn*, Packet*);
Packet*	_vtrpc(VtConn*, Packet*, VtFcall*);
void	vtrecvproc(void*);	/* VtConn */
void	vtsendproc(void*);	/* VtConn */

int	vtconnect(VtConn*);
int	vthello(VtConn*);
int	vtread(VtConn*, u8 score[VtScoreSize], u32 type,
		  u8 *buf, int n);
int	vtwrite(VtConn*, u8 score[VtScoreSize], u32 type,
		   u8 *buf, int n);
Packet*	vtreadpacket(VtConn*, u8 score[VtScoreSize], u32 type,
			    int n);
int	vtwritepacket(VtConn*, u8 score[VtScoreSize], u32 type,
			 Packet *p);
int	vtsync(VtConn*);
int	vtping(VtConn*);

/*
 * Data blocks and block cache.
 */
enum
{
	NilBlock = ~0
};

typedef struct VtBlock VtBlock;
typedef struct VtCache VtCache;

struct VtBlock
{
	VtCache	*c;
	QLock	lk;

	u8	*data;
	u8	score[VtScoreSize];
	u8	type;			/* BtXXX */

	/* internal to cache */
	int	nlock;
	int	iostate;
	int	ref;
	u32	heap;
	VtBlock	*next;
	VtBlock	**prev;
	u32	used;
	u32	used2;
	u32	addr;
	usize	pc;
};

u32	vtglobaltolocal(u8[VtScoreSize]);
void	vtlocaltoglobal(u32, u8[VtScoreSize]);

VtCache*vtcachealloc(VtConn*, int blocksize, u32 nblocks);
void	vtcachefree(VtCache*);
VtBlock*vtcachelocal(VtCache*, u32 addr, int type);
VtBlock*vtcacheglobal(VtCache*, u8[VtScoreSize], int type);
VtBlock*vtcacheallocblock(VtCache*, int type);
void	vtcachesetwrite(VtCache*,
	int(*)(VtConn*, u8[VtScoreSize], u32, u8*, int));
void	vtblockput(VtBlock*);
u32	vtcacheblocksize(VtCache*);
int	vtblockwrite(VtBlock*);
VtBlock*vtblockcopy(VtBlock*);
void	vtblockduplock(VtBlock*);

extern int vtcachencopy, vtcachenread, vtcachenwrite;
extern int vttracelevel;

/*
 * Hash tree file tree.
 */
typedef struct VtFile VtFile;
struct VtFile
{
	QLock	lk;
	int	ref;
	int	local;
	VtBlock	*b;			/* block containing this file */
	u8	score[VtScoreSize];	/* score of block containing this file */

/* immutable */
	VtCache	*c;
	int	mode;
	u32	gen;
	int	dsize;
	int	psize;
	int	dir;
	VtFile	*parent;
	int	epb;			/* entries per block in parent */
	u32	offset; 		/* entry offset in parent */
};

enum
{
	VtOREAD,
	VtOWRITE,
	VtORDWR
};

VtBlock*vtfileblock(VtFile*, u32, int mode);
int	vtfileblockscore(VtFile*, u32, u8[VtScoreSize]);
void	vtfileclose(VtFile*);
VtFile*	_vtfilecreate(VtFile*, int offset, int psize, int dsize, int dir);
VtFile*	vtfilecreate(VtFile*, int psize, int dsize, int dir);
VtFile*	vtfilecreateroot(VtCache*, int psize, int dsize, int type);
int	vtfileflush(VtFile*);
int	vtfileflushbefore(VtFile*, u64);
u32	vtfilegetdirsize(VtFile*);
int	vtfilegetentry(VtFile*, VtEntry*);
u64	vtfilegetsize(VtFile*);
void	vtfileincref(VtFile*);
int	vtfilelock2(VtFile*, VtFile*, int);
int	vtfilelock(VtFile*, int);
VtFile*	vtfileopen(VtFile*, u32, int);
VtFile*	vtfileopenroot(VtCache*, VtEntry*);
i32	vtfileread(VtFile*, void*, i32, i64);
int	vtfileremove(VtFile*);
int	vtfilesetdirsize(VtFile*, u32);
int	vtfilesetentry(VtFile*, VtEntry*);
int	vtfilesetsize(VtFile*, u64);
int	vtfiletruncate(VtFile*);
void	vtfileunlock(VtFile*);
i32	vtfilewrite(VtFile*, void*, i32, i64);

int	vttimefmt(Fmt*);

extern int chattyventi;
extern int ventidoublechecksha1;
extern int ventilogging;

extern char *VtServerLog;
