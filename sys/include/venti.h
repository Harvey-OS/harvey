#pragma	lib	"libventi.a"
#pragma	src	"/sys/src/libventi"

typedef struct VtSession	VtSession;
typedef struct VtSha1		VtSha1;
typedef struct Packet		Packet;
typedef struct VtLock 		VtLock;
typedef struct VtRendez		VtRendez;
typedef struct VtRootLump	VtRootLump;
typedef struct VtDirEntry1	VtDirEntry1;
typedef struct VtDirEntry2	VtDirEntry2;
typedef struct VtServerVtbl	VtServerVtbl;
typedef VtDirEntry2	VtDirEntry;

enum {
	VtScoreSize	= 20, /* Venti */
	VtMaxLumpSize	= 56*1024,
	VtPointerDepth	= 7,	
	VtDirEntrySize1	= 32,
	VtDirEntrySize2	= 40,
	VtDirEntrySize	= VtDirEntrySize2,
	VtRootSize 	= 300,
	VtMaxStringSize	= 1000,
	VtAuthSize 	= 1024,  /* size of auth group - in bits - must be multiple of 8 */
	MaxFragSize 	= 9*1024,
	VtMaxFileSize	= (1ULL<<48) - 1,
	VtRootVersion1	= 1,
	VtRootVersion2	= 2,
	VtRootVersion	= VtRootVersion2,
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

	VtMaxType,

	VtPrivateTypes = 0xf0,	/* for interal use - not to go over protocol */
};

/* Dir Entry flags */
enum {
	VtDirEntryActive = (1<<0),	/* entry is in use */
	VtDirEntryDir = (1<<1),		/* a directory */
	VtDirEntryDepthShift = 2,	/* shift for pointer depth */
	VtDirEntryDepthMask = (0x7<<2),	/* mask for pointer depth */
};

/*
 * VtRootLump & VtDirEntry perhaps need to be more portable
 */
struct VtRootLump {
	uchar version[2];
	char name[128];
	char type[128];
	uchar score[VtScoreSize];	/* to a Dir block */
	uchar blockSize[2];		/* maximum block size */
	uchar prev[VtScoreSize];	/* last root block */
};


/* the old directory structure - should go away */
struct VtDirEntry1 {
	uchar psize[2];			/* pointer block size */
	uchar dsize[2];			/* data block size */
	uchar flag;
	uchar gen;
	uchar size[6];
	uchar score[VtScoreSize];
};

struct VtDirEntry2 {
	uchar gen[4];			/* generation number */
	uchar psize[2];			/* pointer block size */
	uchar dsize[2];			/* data block size */
	uchar flag;
	uchar reserved[5];
	uchar size[6];
	uchar score[VtScoreSize];
};

struct VtServerVtbl {
	Packet *(*read)(VtSession*, uchar score[VtScoreSize], int type, int n);
	int (*write)(VtSession*, uchar score[VtScoreSize], int type, Packet *p);
	void (*closing)(VtSession*, int clean);
};

/* versions */
enum {
	/* experimental versions */
	VtVersion01 = 1,
	VtVersion02,
};

/* score of zero length block */
extern uchar vtZeroScore[VtScoreSize];	

/* both sides */
void vtAttach(void);
void vtDetach(void);
void vtClose(VtSession *s);
void vtFree(VtSession *s);
char *vtGetUid(VtSession *s);
char *vtGetSid(VtSession *s);
int vtSetDebug(VtSession *s, int);
int vtGetDebug(VtSession *s);
int vtSetFd(VtSession *s, int fd);
int vtGetFd(VtSession *s);
int vtConnect(VtSession *s, char *password);
int vtSetCryptoStrength(VtSession *s, int);
int vtGetCryptoStrength(VtSession *s);
int vtSetCompression(VtSession *s, int);
int vtGetCompression(VtSession *s);
int vtGetCrypto(VtSession *s);
int vtGetCodec(VtSession *s);
char *vtGetVersion(VtSession *s);
char *vtGetError(void);
int vtErrFmt(Fmt *fmt);
void vtDebug(VtSession*, char *, ...);
void vtDebugMesg(VtSession *z, Packet *p, char *s);


/* client side */
VtSession *vtClientAlloc(void);
VtSession *vtDial(char *server);
VtSession *vtStdioServer(char *server);
int vtPing(VtSession *s);
int vtSetUid(VtSession*, char *uid);
int vtRead(VtSession*, uchar score[VtScoreSize], int type, uchar *buf, int n);
int vtWrite(VtSession*, uchar score[VtScoreSize], int type, uchar *buf, int n);
Packet *vtReadPacket(VtSession*, uchar score[VtScoreSize], int type, int n);
int vtWritePacket(VtSession*, uchar score[VtScoreSize], int type, Packet *p);

int vtZeroExtend(int type, uchar *buf, int n, int nn);
int vtZeroRetract(int type, uchar *buf, int n);

/* server side */
VtSession *vtServerAlloc(VtServerVtbl*);
int vtSetSid(VtSession *s, char *sid);
int vtExport(VtSession *s);

/* sha1 */
VtSha1* vtSha1Alloc(void);
void vtSha1Free(VtSha1*);
void vtSha1Init(VtSha1*);
void vtSha1Update(VtSha1*, uchar *, int n);
void vtSha1Final(VtSha1*, uchar sha1[VtScoreSize]);
void vtSha1(uchar score[VtScoreSize], uchar *, int);
int vtSha1Check(uchar score[VtScoreSize], uchar *, int);
int vtScoreFmt(Fmt *fmt);

/* Packet */
Packet *packetAlloc(void);
void packetFree(Packet*);
Packet *packetForeign(uchar *buf, int n, void (*free)(void *a), void *a);
Packet *packetDup(Packet*, int offset, int n);
Packet *packetSplit(Packet*, int n);
int packetConsume(Packet*, uchar *buf, int n);
int packetTrim(Packet*, int offset, int n);
uchar *packetHeader(Packet*, int n);
uchar *packetTrailer(Packet*, int n);
int packetPrefix(Packet*, uchar *buf, int n);
int packetAppend(Packet*, uchar *buf, int n);
int packetConcat(Packet*, Packet*);
uchar *packetPeek(Packet*, uchar *buf, int offset, int n);
int packetCopy(Packet*, uchar *buf, int offset, int n);
int packetFragments(Packet*, IOchunk*, int nio, int offset);
int packetSize(Packet*);
int packetAllocatedSize(Packet*);
void packetSha1(Packet*, uchar sha1[VtScoreSize]);
int packetCompact(Packet*);
int packetCmp(Packet*, Packet*);
void packetStats(void);

/* portability stuff - should be a seperate library */

void vtMemFree(void *);
void *vtMemAlloc(int);
void *vtMemAllocZ(int);
void *vtMemRealloc(void *p, int);
void *vtMemBrk(int n);
char *vtStrDup(char *);
void vtFatal(char *, ...);
char *vtGetError(void);
char *vtSetError(char *);
char *vtOSError(void);

int	vtGetUint16(uchar *p);
ulong	vtGetUint32(uchar *p);
uvlong	vtGetUint48(uchar *p);
uvlong	vtGetUint64(uchar *p);
void	vtPutUint16(uchar *p, int x);
void	vtPutUint32(uchar *p, ulong x);
void	vtPutUint48(uchar *p, uvlong x);
void	vtPutUint64(uchar *p, uvlong x);

/* locking/threads */
int vtThread(void (*f)(void*), void *rock);
VtLock *vtLockAlloc(void);
void vtLockInit(VtLock**);
void vtLock(VtLock *);
void vtUnlock(VtLock *);
void vtLockFree(VtLock *);
VtRendez *vtRendezAlloc(VtLock*);
void vtRendezFree(VtRendez*);
int vtSleep(VtRendez*);
int vtWakeup(VtRendez*);

/* fd functions - really network (socket) functions */
void vtFdClose(int);
int vtFdRead(int, uchar*, int);
int vtFdReadFully(int, uchar*, int);
int vtFdWrite(int, uchar*, int);

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
