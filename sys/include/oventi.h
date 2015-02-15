/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"liboventi.a"
#pragma	src	"/sys/src/liboventi"

typedef struct VtSession	VtSession;
typedef struct VtSha1		VtSha1;
typedef struct Packet		Packet;
typedef struct VtLock 		VtLock;
typedef struct VtRendez		VtRendez;
typedef struct VtRoot		VtRoot;
typedef struct VtEntry		VtEntry;
typedef struct VtServerVtbl	VtServerVtbl;

#pragma incomplete VtSession
#pragma incomplete VtSha1
#pragma incomplete Packet
#pragma incomplete VtLock
#pragma incomplete VtRendez

enum {
	VtScoreSize	= 20, /* Venti */
	VtMaxLumpSize	= 56*1024,
	VtPointerDepth	= 7,	
	VtEntrySize	= 40,
	VtRootSize 	= 300,
	VtMaxStringSize	= 1000,
	VtAuthSize 	= 1024,  /* size of auth group - in bits - must be multiple of 8 */
	MaxFragSize 	= 9*1024,
	VtMaxFileSize	= (1ULL<<48) - 1,
	VtRootVersion	= 2,
};

/* crypto strengths */
enum {
	VtCryptoStrengthNone,
	VtCryptoStrengthAuth,
	VtCryptoStrengthWeak,
	VtCryptoStrengthStrong,
};

/* crypto suites */
enum {
	VtCryptoNone,
	VtCryptoSSL3,
	VtCryptoTLS1,

	VtCryptoMax
};

/* codecs */
enum {
	VtCodecNone,

	VtCodecDeflate,
	VtCodecThwack,

	VtCodecMax
};

/* Lump Types */
enum {
	VtErrType,		/* illegal */

	VtRootType,
	VtDirType,
	VtPointerType0,
	VtPointerType1,
	VtPointerType2,
	VtPointerType3,
	VtPointerType4,
	VtPointerType5,
	VtPointerType6,
	VtPointerType7,		/* not used */
	VtPointerType8,		/* not used */
	VtPointerType9,		/* not used */
	VtDataType,

	VtMaxType
};

/* Dir Entry flags */
enum {
	VtEntryActive = (1<<0),		/* entry is in use */
	VtEntryDir = (1<<1),		/* a directory */
	VtEntryDepthShift = 2,		/* shift for pointer depth */
	VtEntryDepthMask = (0x7<<2),	/* mask for pointer depth */
	VtEntryLocal = (1<<5),		/* used for local storage: should not be set for Venti blocks */
	VtEntryNoArchive = (1<<6),	/* used for local storage: should not be set for Venti blocks */
};

struct VtRoot {
	uint16_t version;
	int8_t name[128];
	int8_t type[128];
	uint8_t score[VtScoreSize];	/* to a Dir block */
	uint16_t blockSize;		/* maximum block size */
	uint8_t prev[VtScoreSize];	/* last root block */
};

struct VtEntry {
	uint32_t gen;			/* generation number */
	uint16_t psize;			/* pointer block size */
	uint16_t dsize;			/* data block size */
	uint8_t depth;			/* unpacked from flags */
	uint8_t flags;
	uint64_t size;
	uint8_t score[VtScoreSize];
};

struct VtServerVtbl {
	Packet *(*read)(VtSession*, uint8_t score[VtScoreSize], int type,
			int n);
	int (*write)(VtSession*, uint8_t score[VtScoreSize], int type,
		     Packet *p);
	void (*closing)(VtSession*, int clean);
	void (*sync)(VtSession*);
};

/* versions */
enum {
	/* experimental versions */
	VtVersion01 = 1,
	VtVersion02,
};

/* score of zero length block */
extern uint8_t vtZeroScore[VtScoreSize];	

/* both sides */
void vtAttach(void);
void vtDetach(void);
void vtClose(VtSession *s);
void vtFree(VtSession *s);
int8_t *vtGetUid(VtSession *s);
int8_t *vtGetSid(VtSession *s);
int vtSetDebug(VtSession *s, int);
int vtGetDebug(VtSession *s);
int vtSetFd(VtSession *s, int fd);
int vtGetFd(VtSession *s);
int vtConnect(VtSession *s, int8_t *password);
int vtSetCryptoStrength(VtSession *s, int);
int vtGetCryptoStrength(VtSession *s);
int vtSetCompression(VtSession *s, int);
int vtGetCompression(VtSession *s);
int vtGetCrypto(VtSession *s);
int vtGetCodec(VtSession *s);
int8_t *vtGetVersion(VtSession *s);
int8_t *vtGetError(void);
int vtErrFmt(Fmt *fmt);
void vtDebug(VtSession*, int8_t *, ...);
void vtDebugMesg(VtSession *z, Packet *p, int8_t *s);

/* internal */
VtSession *vtAlloc(void);
void vtReset(VtSession*);
int vtAddString(Packet*, int8_t*);
int vtGetString(Packet*, int8_t**);
int vtSendPacket(VtSession*, Packet*);
Packet *vtRecvPacket(VtSession*);
void vtDisconnect(VtSession*, int);
int vtHello(VtSession*);

/* client side */
VtSession *vtClientAlloc(void);
VtSession *vtDial(int8_t *server, int canfail);
int vtRedial(VtSession*, int8_t *server);
VtSession *vtStdioServer(int8_t *server);
int vtPing(VtSession *s);
int vtSetUid(VtSession*, int8_t *uid);
int vtRead(VtSession*, uint8_t score[VtScoreSize], int type, uint8_t *buf,
	   int n);
int vtWrite(VtSession*, uint8_t score[VtScoreSize], int type, uint8_t *buf,
	    int n);
Packet *vtReadPacket(VtSession*, uint8_t score[VtScoreSize], int type,
		     int n);
int vtWritePacket(VtSession*, uint8_t score[VtScoreSize], int type,
		  Packet *p);
int vtSync(VtSession *s);

int vtZeroExtend(int type, uint8_t *buf, int n, int nn);
int vtZeroTruncate(int type, uint8_t *buf, int n);
int vtParseScore(int8_t*, uint, uint8_t[VtScoreSize]);

void vtRootPack(VtRoot*, uint8_t*);
int vtRootUnpack(VtRoot*, uint8_t*);
void vtEntryPack(VtEntry*, uint8_t*, int index);
int vtEntryUnpack(VtEntry*, uint8_t*, int index);

/* server side */
VtSession *vtServerAlloc(VtServerVtbl*);
int vtSetSid(VtSession *s, int8_t *sid);
int vtExport(VtSession *s);

/* sha1 */
VtSha1* vtSha1Alloc(void);
void vtSha1Free(VtSha1*);
void vtSha1Init(VtSha1*);
void vtSha1Update(VtSha1*, uint8_t *, int n);
void vtSha1Final(VtSha1*, uint8_t sha1[VtScoreSize]);
void vtSha1(uint8_t score[VtScoreSize], uint8_t *, int);
int vtSha1Check(uint8_t score[VtScoreSize], uint8_t *, int);
int vtScoreFmt(Fmt *fmt);

/* Packet */
Packet *packetAlloc(void);
void packetFree(Packet*);
Packet *packetForeign(uint8_t *buf, int n, void (*free)(void *a), void *a);
Packet *packetDup(Packet*, int offset, int n);
Packet *packetSplit(Packet*, int n);
int packetConsume(Packet*, uint8_t *buf, int n);
int packetTrim(Packet*, int offset, int n);
uint8_t *packetHeader(Packet*, int n);
uint8_t *packetTrailer(Packet*, int n);
int packetPrefix(Packet*, uint8_t *buf, int n);
int packetAppend(Packet*, uint8_t *buf, int n);
int packetConcat(Packet*, Packet*);
uint8_t *packetPeek(Packet*, uint8_t *buf, int offset, int n);
int packetCopy(Packet*, uint8_t *buf, int offset, int n);
int packetFragments(Packet*, IOchunk*, int nio, int offset);
int packetSize(Packet*);
int packetAllocatedSize(Packet*);
void packetSha1(Packet*, uint8_t sha1[VtScoreSize]);
int packetCompact(Packet*);
int packetCmp(Packet*, Packet*);
void packetStats(void);

/* portability stuff - should be a seperate library */

void vtMemFree(void *);
void *vtMemAlloc(int);
void *vtMemAllocZ(int);
void *vtMemRealloc(void *p, int);
void *vtMemBrk(int n);
int8_t *vtStrDup(int8_t *);
void vtFatal(int8_t *, ...);
int8_t *vtGetError(void);
int8_t *vtSetError(int8_t *, ...);
int8_t *vtOSError(void);

/* locking/threads */
int vtThread(void (*f)(void*), void *rock);
void vtThreadSetName(int8_t*);

VtLock *vtLockAlloc(void);
/* void vtLockInit(VtLock**); */
void vtLock(VtLock*);
int vtCanLock(VtLock*);
void vtRLock(VtLock*);
int vtCanRLock(VtLock*);
void vtUnlock(VtLock*);
void vtRUnlock(VtLock*);
void vtLockFree(VtLock*);

VtRendez *vtRendezAlloc(VtLock*);
void vtRendezFree(VtRendez*);
int vtSleep(VtRendez*);
int vtWakeup(VtRendez*);
int vtWakeupAll(VtRendez*);

/* fd functions - really network (socket) functions */
void vtFdClose(int);
int vtFdRead(int, uint8_t*, int);
int vtFdReadFully(int, uint8_t*, int);
int vtFdWrite(int, uint8_t*, int);

/*
 * formatting
 * other than noted, these formats all ignore
 * the width and precision arguments, and all flags
 *
 * V	a venti score
 * R	venti error
 */
#pragma	varargck	type	"V"		uchar*
#pragma	varargck	type	"R"		void

#pragma	varargck	argpos	vtSetError	1

