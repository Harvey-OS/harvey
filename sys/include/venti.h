/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma lib "libventi.a"
#pragma src "/sys/src/libventi"


/* XXX should be own library? */
/*
 * Packets
 */
enum
{
	MaxFragSize = 9*1024
};

typedef struct Packet Packet;
#pragma incomplete Packet

Packet*	packetalloc(void);
void	packetappend(Packet*, uint8_t *buf, int n);
uint	packetasize(Packet*);
int	packetcmp(Packet*, Packet*);
int	packetcompact(Packet*);
void	packetconcat(Packet*, Packet*);
int	packetconsume(Packet*, uint8_t *buf, int n);
int	packetcopy(Packet*, uint8_t *buf, int offset, int n);
Packet*	packetdup(Packet*, int offset, int n);
Packet*	packetforeign(uint8_t *buf, int n, void (*free)(void *a),
			     void *a);
int	packetfragments(Packet*, IOchunk*, int nio, int offset);
void	packetfree(Packet*);
uint8_t*	packetheader(Packet*, int n);
uint8_t*	packetpeek(Packet*, uint8_t *buf, int offset, int n);
void	packetprefix(Packet*, uint8_t *buf, int n);
void	packetsha1(Packet*, uint8_t sha1[20]);
uint	packetsize(Packet*);
Packet*	packetsplit(Packet*, int n);
void	packetstats(void);
uint8_t*	packettrailer(Packet*, int n);
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
	int8_t	*name;
	VtLogChunk *chunk;
	uint	nchunk;
	VtLogChunk *w;
	QLock	lk;
	int	ref;
};

struct VtLogChunk
{
	int8_t	*p;
	int8_t	*ep;
	int8_t	*wp;
};

VtLog*	vtlogopen(int8_t *name, uint size);
void	vtlogprint(VtLog *log, int8_t *fmt, ...);
void	vtlog(int8_t *name, int8_t *fmt, ...);
void	vtlogclose(VtLog*);
void	vtlogremove(int8_t *name);
int8_t**	vtlognames(int*);
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
int vtputstring(Packet*, int8_t*);
int vtgetstring(Packet*, int8_t**);

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
uint vttodisktype(uint);
uint vtfromdisktype(uint);

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
	uint32_t	gen;			/* generation number */
	uint16_t	psize;			/* pointer block size */
	uint16_t	dsize;			/* data block size */
	uint8_t	type;
	uint8_t	flags;
	uint64_t	size;
	uint8_t	score[VtScoreSize];
};

void vtentrypack(VtEntry*, uint8_t*, int index);
int vtentryunpack(VtEntry*, uint8_t*, int index);

struct VtRoot
{
	int8_t	name[128];
	int8_t	type[128];
	uint8_t	score[VtScoreSize];	/* to a Dir block */
	uint16_t	blocksize;		/* maximum block size */
	uint8_t	prev[VtScoreSize];	/* last root block */
};

enum
{
	VtRootSize = 300,
	VtRootVersion = 2
};

void vtrootpack(VtRoot*, uint8_t*);
int vtrootunpack(VtRoot*, uint8_t*);

/*
 * score of zero length block
 */
extern uint8_t vtzeroscore[VtScoreSize];

/*
 * zero extend and truncate blocks
 */
void vtzeroextend(int type, uint8_t *buf, uint n, uint nn);
uint vtzerotruncate(int type, uint8_t *buf, uint n);

/*
 * parse score: mungs s
 */
int vtparsescore(int8_t *s, int8_t **prefix, uint8_t[VtScoreSize]);

/*
 * formatting
 * other than noted, these formats all ignore
 * the width and precision arguments, and all flags
 *
 * V	a venti score
 */
#pragma	varargck	type	"V"	uchar*
#pragma	varargck	type	"F"	VtFcall*
#pragma	varargck	type	"T"	void
#pragma	varargck	type	"lT"	void

int vtscorefmt(Fmt*);

/*
 * error-checking malloc et al.
 */
void	vtfree(void *);
void*	vtmalloc(int);
void*	vtmallocz(int);
void*	vtrealloc(void *p, int);
void*	vtbrk(int n);
int8_t*	vtstrdup(int8_t *);

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
	uint8_t	msgtype;
	uint8_t	tag;

	int8_t	*error;		/* Rerror */

	int8_t	*version;	/* Thello */
	int8_t	*uid;		/* Thello */
	uint8_t	strength;	/* Thello */
	uint8_t	*crypto;	/* Thello */
	uint	ncrypto;	/* Thello */
	uint8_t	*codec;		/* Thello */
	uint	ncodec;		/* Thello */
	int8_t	*sid;		/* Rhello */
	uint8_t	rcrypto;	/* Rhello */
	uint8_t	rcodec;		/* Rhello */
	uint8_t	*auth;		/* TauthX, RauthX */
	uint	nauth;		/* TauthX, RauthX */
	uint8_t	score[VtScoreSize];	/* Tread, Rwrite */
	uint8_t	blocktype;	/* Tread, Twrite */
	uint16_t	count;		/* Tread */
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
	uint	ntag;
	uint	nsleep;
	Packet	*part;
	Rendez	tagrend;
	Rendez	rpcfork;
	int8_t	*version;
	int8_t	*uid;
	int8_t	*sid;
	int8_t	addr[256];	/* address of other side */
};

VtConn*	vtconn(int infd, int outfd);
VtConn*	vtdial(int8_t*);
void	vtfreeconn(VtConn*);
int	vtsend(VtConn*, Packet*);
Packet*	vtrecv(VtConn*);
int	vtversion(VtConn* z);
void	vtdebug(VtConn* z, int8_t*, ...);
void	vthangup(VtConn* z);
int	vtgoodbye(VtConn* z);

/* #pragma varargck argpos vtdebug 2 */

/* server */
typedef struct VtSrv VtSrv;
#pragma incomplete VtSrv
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
VtSrv*	vtlisten(int8_t *addr);
VtReq*	vtgetreq(VtSrv*);
void	vtrespond(VtReq*);

/* client */
Packet*	vtrpc(VtConn*, Packet*);
Packet*	_vtrpc(VtConn*, Packet*, VtFcall*);
void	vtrecvproc(void*);	/* VtConn */
void	vtsendproc(void*);	/* VtConn */

int	vtconnect(VtConn*);
int	vthello(VtConn*);
int	vtread(VtConn*, uint8_t score[VtScoreSize], uint type,
		  uint8_t *buf, int n);
int	vtwrite(VtConn*, uint8_t score[VtScoreSize], uint type,
		   uint8_t *buf, int n);
Packet*	vtreadpacket(VtConn*, uint8_t score[VtScoreSize], uint type,
			    int n);
int	vtwritepacket(VtConn*, uint8_t score[VtScoreSize], uint type,
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
#pragma incomplete VtCache

struct VtBlock
{
	VtCache	*c;
	QLock	lk;

	uint8_t	*data;
	uint8_t	score[VtScoreSize];
	uint8_t	type;			/* BtXXX */

	/* internal to cache */
	int	nlock;
	int	iostate;
	int	ref;
	uint32_t	heap;
	VtBlock	*next;
	VtBlock	**prev;
	uint32_t	used;
	uint32_t	used2;
	uint32_t	addr;
	uintptr	pc;
};

uint32_t	vtglobaltolocal(uint8_t[VtScoreSize]);
void	vtlocaltoglobal(uint32_t, uint8_t[VtScoreSize]);

VtCache*vtcachealloc(VtConn*, int blocksize, uint32_t nblocks);
void	vtcachefree(VtCache*);
VtBlock*vtcachelocal(VtCache*, uint32_t addr, int type);
VtBlock*vtcacheglobal(VtCache*, uint8_t[VtScoreSize], int type);
VtBlock*vtcacheallocblock(VtCache*, int type);
void	vtcachesetwrite(VtCache*,
	int(*)(VtConn*, uint8_t[VtScoreSize], uint, uint8_t*, int));
void	vtblockput(VtBlock*);
uint32_t	vtcacheblocksize(VtCache*);
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
	uint8_t	score[VtScoreSize];	/* score of block containing this file */

/* immutable */
	VtCache	*c;
	int	mode;
	uint32_t	gen;
	int	dsize;
	int	psize;
	int	dir;
	VtFile	*parent;
	int	epb;			/* entries per block in parent */
	uint32_t	offset; 		/* entry offset in parent */
};

enum
{
	VtOREAD,
	VtOWRITE,
	VtORDWR
};

VtBlock*vtfileblock(VtFile*, uint32_t, int mode);
int	vtfileblockscore(VtFile*, uint32_t, uint8_t[VtScoreSize]);
void	vtfileclose(VtFile*);
VtFile*	_vtfilecreate(VtFile*, int offset, int psize, int dsize, int dir);
VtFile*	vtfilecreate(VtFile*, int psize, int dsize, int dir);
VtFile*	vtfilecreateroot(VtCache*, int psize, int dsize, int type);
int	vtfileflush(VtFile*);
int	vtfileflushbefore(VtFile*, uint64_t);
uint32_t	vtfilegetdirsize(VtFile*);
int	vtfilegetentry(VtFile*, VtEntry*);
uint64_t	vtfilegetsize(VtFile*);
void	vtfileincref(VtFile*);
int	vtfilelock2(VtFile*, VtFile*, int);
int	vtfilelock(VtFile*, int);
VtFile*	vtfileopen(VtFile*, uint32_t, int);
VtFile*	vtfileopenroot(VtCache*, VtEntry*);
int32_t	vtfileread(VtFile*, void*, int32_t, int64_t);
int	vtfileremove(VtFile*);
int	vtfilesetdirsize(VtFile*, uint32_t);
int	vtfilesetentry(VtFile*, VtEntry*);
int	vtfilesetsize(VtFile*, uint64_t);
int	vtfiletruncate(VtFile*);
void	vtfileunlock(VtFile*);
int32_t	vtfilewrite(VtFile*, void*, int32_t, int64_t);

int	vttimefmt(Fmt*);

extern int chattyventi;
extern int ventidoublechecksha1;
extern int ventilogging;

extern int8_t *VtServerLog;
