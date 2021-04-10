/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Sun RPC; see RFC 1057
 */


typedef u8 u1int;

typedef struct SunAuthInfo SunAuthInfo;
typedef struct SunAuthUnix SunAuthUnix;
typedef struct SunRpc SunRpc;
typedef struct SunCall SunCall;

enum
{
	/* Authinfo.flavor */
	SunAuthNone = 0,
	SunAuthSys,
	SunAuthShort,
	SunAuthDes,
};

typedef enum {
	SunAcceptError = 0x10000,
	SunRejectError = 0x20000,
	SunAuthError = 0x40000,

	/* Reply.status */
	SunSuccess = 0,

	SunProgUnavail = SunAcceptError | 1,
	SunProgMismatch,
	SunProcUnavail,
	SunGarbageArgs,
	SunSystemErr,

	SunRpcMismatch = SunRejectError | 0,

	SunAuthBadCred = SunAuthError | 1,
	SunAuthRejectedCred,
	SunAuthBadVerf,
	SunAuthRejectedVerf,
	SunAuthTooWeak,
	SunAuthInvalidResp,
	SunAuthFailed,
} SunStatus;

struct SunAuthInfo
{
	uint flavor;
	u8 *data;
	uint ndata;
};

struct SunAuthUnix
{
	u32 stamp;
	char *sysname;
	u32 uid;
	u32 gid;
	u32 g[16];
	u32 ng;
};

struct SunRpc
{
	u32 xid;
	uint iscall;

	/*
	 * only sent on wire in call
	 * caller fills in for the reply unpackers.
	 */
	u32 proc;

	/* call */
	// uint proc;
	u32 prog, vers;
	SunAuthInfo cred;
	SunAuthInfo verf;
	u8 *data;
	uint ndata;

	/* reply */
	u32 status;
	// SunAuthInfo verf;
	u32 low, high;
	// uchar *data;
	// uint ndata;
};

typedef enum
{
	SunCallTypeTNull,
	SunCallTypeRNull,
} SunCallType;

struct SunCall
{
	SunRpc rpc;
	SunCallType type;
};

void sunErrstr(SunStatus);

void sunRpcPrint(Fmt*, SunRpc*);
uint sunRpcSize(SunRpc*);
SunStatus sunRpcPack(u8*, u8*, u8**, SunRpc*);
SunStatus sunRpcUnpack(u8*, u8*, u8**, SunRpc*);

void sunAuthInfoPrint(Fmt*, SunAuthInfo*);
uint sunAuthInfoSize(SunAuthInfo*);
int sunAuthInfoPack(u8*, u8*, u8**, SunAuthInfo*);
int sunAuthInfoUnpack(u8*, u8*, u8**, SunAuthInfo*);

void sunAuthUnixPrint(Fmt*, SunAuthUnix*);
uint sunAuthUnixSize(SunAuthUnix*);
int sunAuthUnixPack(u8*, u8*, u8**, SunAuthUnix*);
int sunAuthUnixUnpack(u8*, u8*, u8**, SunAuthUnix*);

int sunEnumPack(u8*, u8*, u8**, int*);
int sunEnumUnpack(u8*, u8*, u8**, int*);
int sunUint1Pack(u8*, u8*, u8**, u1int*);
int sunUint1Unpack(u8*, u8*, u8**, u1int*);

int sunStringPack(u8*, u8*, u8**, char**, u32);
int sunStringUnpack(u8*, u8*, u8**, char**, u32);
uint sunStringSize(char*);

int sunUint32Pack(u8*, u8*, u8**, u32*);
int sunUint32Unpack(u8*, u8*, u8**, u32*);
int sunUint64Pack(u8*, u8*, u8**, u64*);
int sunUint64Unpack(u8*, u8*, u8**, u64*);

int sunVarOpaquePack(u8*, u8*, u8**, u8**, u32*,
		     u32);
int sunVarOpaqueUnpack(u8*, u8*, u8**, u8**, u32*,
		       u32);
uint sunVarOpaqueSize(u32);

int sunFixedOpaquePack(u8*, u8*, u8**, u8*, u32);
int sunFixedOpaqueUnpack(u8*, u8*, u8**, u8*, u32);
uint sunFixedOpaqueSize(u32);

/*
 * Sun RPC Program
 */
typedef struct SunProc SunProc;
typedef struct SunProg SunProg;
struct SunProg
{
	uint prog;
	uint vers;
	SunProc *proc;
	int nproc;
};

struct SunProc
{
	int (*pack)(u8*, u8*, u8**, SunCall*);
	int (*unpack)(u8*, u8*, u8**, SunCall*);
	uint (*size)(SunCall*);
	void (*fmt)(Fmt*, SunCall*);
	uint sizeoftype;
};

SunStatus sunCallPack(SunProg*, u8*, u8*, u8**, SunCall*);
SunStatus sunCallUnpack(SunProg*, u8*, u8*, u8**, SunCall*);
SunStatus sunCallUnpackAlloc(SunProg*, SunCallType, u8*, u8*,
			     u8**, SunCall**);
uint sunCallSize(SunProg*, SunCall*);
void sunCallSetup(SunCall*, SunProg*, uint);

/*
 * Formatting
 */

int	sunRpcFmt(Fmt*);
int	sunCallFmt(Fmt*);
void	sunFmtInstall(SunProg*);


/*
 * Sun RPC Server
 */
typedef struct SunMsg SunMsg;
typedef struct SunSrv SunSrv;

enum
{
	SunStackSize = 8192,
};

struct SunMsg
{
	u8 *data;
	int count;
	SunSrv *srv;
	SunRpc rpc;
	SunProg *pg;
	SunCall *call;
	Channel *creply;	/* chan(SunMsg*) */
};

struct SunSrv
{
	int chatty;
	int cacheReplies;
	int alwaysReject;
	SunProg **map;
	Channel *crequest;

/* implementation use only */
	Channel **cdispatch;
	SunProg **prog;
	int nprog;
	void *cache;
	Channel *creply;
	Channel *cthread;
};

SunSrv *sunSrv(void);

void	sunSrvProg(SunSrv *srv, SunProg *prog, Channel *c);
int	sunSrvAnnounce(SunSrv *srv, char *address);
int	sunSrvUdp(SunSrv *srv, char *address);
int	sunSrvNet(SunSrv *srv, char *address);
int	sunSrvFd(SunSrv *srv, int fd);
void	sunSrvThreadCreate(SunSrv *srv, void (*fn)(void*), void*);
void	sunSrvClose(SunSrv*);

int	sunMsgReply(SunMsg*, SunCall*);
int	sunMsgDrop(SunMsg*);
int	sunMsgReplyError(SunMsg*, SunStatus);

/*
 * Sun RPC Client
 */
typedef struct SunClient SunClient;

struct SunClient
{
	int		fd;
	int		chatty;
	int		needcount;
	u32	maxwait;
	u32	xidgen;
	int		nsend;
	int		nresend;
	struct {
		u32 min;
		u32 max;
		u32 avg;
	} rtt;
	Channel	*dying;
	Channel	*rpcchan;
	Channel	*timerchan;
	Channel	*flushchan;
	Channel	*readchan;
	SunProg	**prog;
	int		nprog;
	int 		timertid;
	int 		nettid;
};

SunClient	*sunDial(char*);

int	sunClientRpc(SunClient*, u32, SunCall*, SunCall*, u8**);
void	sunClientClose(SunClient*);
void	sunClientFlushRpc(SunClient*, u32);
void	sunClientProg(SunClient*, SunProg*);


/*
 * Provided by callers.
 * Should remove dependence on this, but hard.
 */
void	*emalloc(u32);
void *erealloc(void*, u32);


/*
 * Sun RPC port mapper; see RFC 1057 Appendix A
 */

typedef struct PortMap PortMap;
typedef struct PortTNull PortTNull;
typedef struct PortRNull PortRNull;
typedef struct PortTSet PortTSet;
typedef struct PortRSet PortRSet;
typedef struct PortTUnset PortTUnset;
typedef struct PortRUnset PortRUnset;
typedef struct PortTGetport PortTGetport;
typedef struct PortRGetport PortRGetport;
typedef struct PortTDump PortTDump;
typedef struct PortRDump PortRDump;
typedef struct PortTCallit PortTCallit;
typedef struct PortRCallit PortRCallit;

typedef enum
{
	PortCallTNull,
	PortCallRNull,
	PortCallTSet,
	PortCallRSet,
	PortCallTUnset,
	PortCallRUnset,
	PortCallTGetport,
	PortCallRGetport,
	PortCallTDump,
	PortCallRDump,
	PortCallTCallit,
	PortCallRCallit,
} PortCallType;

enum
{
	PortProgram	= 100000,
	PortVersion	= 2,

	PortProtoTcp	= 6,	/* protocol number for TCP/IP */
	PortProtoUdp	= 17	/* protocol number for UDP/IP */
};

struct PortMap {
	u32 prog;
	u32 vers;
	u32 prot;
	u32 port;
};

struct PortTNull {
	SunCall call;
};

struct PortRNull {
	SunCall call;
};

struct PortTSet {
	SunCall call;
	PortMap map;
};

struct PortRSet {
	SunCall call;
	u1int b;
};

struct PortTUnset {
	SunCall call;
	PortMap map;
};

struct PortRUnset {
	SunCall call;
	u1int b;
};

struct PortTGetport {
	SunCall call;
	PortMap map;
};

struct PortRGetport {
	SunCall call;
	u32 port;
};

struct PortTDump {
	SunCall call;
};

struct PortRDump {
	SunCall call;
	PortMap *map;
	int nmap;
};

struct PortTCallit {
	SunCall call;
	u32 prog;
	u32 vers;
	u32 proc;
	u8 *data;
	u32 count;
};

struct PortRCallit {
	SunCall call;
	u32 port;
	u8 *data;
	u32 count;
};

extern SunProg portProg;
