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

#pragma lib "libsunrpc.a"
#pragma src "/sys/src/libsunrpc"

typedef uint8_t u1int;

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
	uint8_t *data;
	uint ndata;
};

struct SunAuthUnix
{
	uint32_t stamp;
	int8_t *sysname;
	uint32_t uid;
	uint32_t gid;
	uint32_t g[16];
	uint32_t ng;
};

struct SunRpc
{
	uint32_t xid;
	uint iscall;

	/*
	 * only sent on wire in call
	 * caller fills in for the reply unpackers.
	 */
	uint32_t proc;

	/* call */
	// uint proc;
	uint32_t prog, vers;
	SunAuthInfo cred;
	SunAuthInfo verf;
	uint8_t *data;
	uint ndata;

	/* reply */
	uint32_t status;
	// SunAuthInfo verf;
	uint32_t low, high;
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
SunStatus sunRpcPack(uint8_t*, uint8_t*, uint8_t**, SunRpc*);
SunStatus sunRpcUnpack(uint8_t*, uint8_t*, uint8_t**, SunRpc*);

void sunAuthInfoPrint(Fmt*, SunAuthInfo*);
uint sunAuthInfoSize(SunAuthInfo*);
int sunAuthInfoPack(uint8_t*, uint8_t*, uint8_t**, SunAuthInfo*);
int sunAuthInfoUnpack(uint8_t*, uint8_t*, uint8_t**, SunAuthInfo*);

void sunAuthUnixPrint(Fmt*, SunAuthUnix*);
uint sunAuthUnixSize(SunAuthUnix*);
int sunAuthUnixPack(uint8_t*, uint8_t*, uint8_t**, SunAuthUnix*);
int sunAuthUnixUnpack(uint8_t*, uint8_t*, uint8_t**, SunAuthUnix*);

int sunEnumPack(uint8_t*, uint8_t*, uint8_t**, int*);
int sunEnumUnpack(uint8_t*, uint8_t*, uint8_t**, int*);
int sunUint1Pack(uint8_t*, uint8_t*, uint8_t**, u1int*);
int sunUint1Unpack(uint8_t*, uint8_t*, uint8_t**, u1int*);

int sunStringPack(uint8_t*, uint8_t*, uint8_t**, int8_t**, uint32_t);
int sunStringUnpack(uint8_t*, uint8_t*, uint8_t**, int8_t**, uint32_t);
uint sunStringSize(int8_t*);

int sunUint32Pack(uint8_t*, uint8_t*, uint8_t**, uint32_t*);
int sunUint32Unpack(uint8_t*, uint8_t*, uint8_t**, uint32_t*);
int sunUint64Pack(uint8_t*, uint8_t*, uint8_t**, uint64_t*);
int sunUint64Unpack(uint8_t*, uint8_t*, uint8_t**, uint64_t*);

int sunVarOpaquePack(uint8_t*, uint8_t*, uint8_t**, uint8_t**, uint32_t*,
		     uint32_t);
int sunVarOpaqueUnpack(uint8_t*, uint8_t*, uint8_t**, uint8_t**, uint32_t*,
		       uint32_t);
uint sunVarOpaqueSize(uint32_t);

int sunFixedOpaquePack(uint8_t*, uint8_t*, uint8_t**, uint8_t*, uint32_t);
int sunFixedOpaqueUnpack(uint8_t*, uint8_t*, uint8_t**, uint8_t*, uint32_t);
uint sunFixedOpaqueSize(uint32_t);

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
	int (*pack)(uint8_t*, uint8_t*, uint8_t**, SunCall*);
	int (*unpack)(uint8_t*, uint8_t*, uint8_t**, SunCall*);
	uint (*size)(SunCall*);
	void (*fmt)(Fmt*, SunCall*);
	uint sizeoftype;
};

SunStatus sunCallPack(SunProg*, uint8_t*, uint8_t*, uint8_t**, SunCall*);
SunStatus sunCallUnpack(SunProg*, uint8_t*, uint8_t*, uint8_t**, SunCall*);
SunStatus sunCallUnpackAlloc(SunProg*, SunCallType, uint8_t*, uint8_t*,
			     uint8_t**, SunCall**);
uint sunCallSize(SunProg*, SunCall*);
void sunCallSetup(SunCall*, SunProg*, uint);

/*
 * Formatting
 */
#pragma varargck type "B" SunRpc*
#pragma varargck type "C" SunCall*

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
	uint8_t *data;
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
int	sunSrvAnnounce(SunSrv *srv, int8_t *address);
int	sunSrvUdp(SunSrv *srv, int8_t *address);
int	sunSrvNet(SunSrv *srv, int8_t *address);
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
	uint32_t	maxwait;
	uint32_t	xidgen;
	int		nsend;
	int		nresend;
	struct {
		uint32_t min;
		uint32_t max;
		uint32_t avg;
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

SunClient	*sunDial(int8_t*);

int	sunClientRpc(SunClient*, uint32_t, SunCall*, SunCall*, uint8_t**);
void	sunClientClose(SunClient*);
void	sunClientFlushRpc(SunClient*, uint32_t);
void	sunClientProg(SunClient*, SunProg*);


/*
 * Provided by callers.
 * Should remove dependence on this, but hard.
 */
void	*emalloc(uint32_t);
void *erealloc(void*, uint32_t);


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
	uint32_t prog;
	uint32_t vers;
	uint32_t prot;
	uint32_t port;
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
	uint32_t port;
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
	uint32_t prog;
	uint32_t vers;
	uint32_t proc;
	uint8_t *data;
	uint32_t count;
};

struct PortRCallit {
	SunCall call;
	uint32_t port;
	uint8_t *data;
	uint32_t count;
};

extern SunProg portProg;
