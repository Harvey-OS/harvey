/*
 * Sun RPC; see RFC 1057
 */

#pragma lib "libsunrpc.a"
#pragma src "/sys/src/libsunrpc"

typedef uchar u1int;

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
	uchar *data;
	uint ndata;
};

struct SunAuthUnix
{
	u32int stamp;
	char *sysname;
	u32int uid;
	u32int gid;
	u32int g[16];
	u32int ng;
};

struct SunRpc
{
	u32int xid;
	uint iscall;

	/*
	 * only sent on wire in call
	 * caller fills in for the reply unpackers.
	 */
	u32int proc;

	/* call */
	// uint proc;
	u32int prog, vers;
	SunAuthInfo cred;
	SunAuthInfo verf;
	uchar *data;
	uint ndata;

	/* reply */
	u32int status;
	// SunAuthInfo verf;
	u32int low, high;
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
SunStatus sunRpcPack(uchar*, uchar*, uchar**, SunRpc*);
SunStatus sunRpcUnpack(uchar*, uchar*, uchar**, SunRpc*);

void sunAuthInfoPrint(Fmt*, SunAuthInfo*);
uint sunAuthInfoSize(SunAuthInfo*);
int sunAuthInfoPack(uchar*, uchar*, uchar**, SunAuthInfo*);
int sunAuthInfoUnpack(uchar*, uchar*, uchar**, SunAuthInfo*);

void sunAuthUnixPrint(Fmt*, SunAuthUnix*);
uint sunAuthUnixSize(SunAuthUnix*);
int sunAuthUnixPack(uchar*, uchar*, uchar**, SunAuthUnix*);
int sunAuthUnixUnpack(uchar*, uchar*, uchar**, SunAuthUnix*);

int sunEnumPack(uchar*, uchar*, uchar**, int*);
int sunEnumUnpack(uchar*, uchar*, uchar**, int*);
int sunUint1Pack(uchar*, uchar*, uchar**, u1int*);
int sunUint1Unpack(uchar*, uchar*, uchar**, u1int*);

int sunStringPack(uchar*, uchar*, uchar**, char**, u32int);
int sunStringUnpack(uchar*, uchar*, uchar**, char**, u32int);
uint sunStringSize(char*);

int sunUint32Pack(uchar*, uchar*, uchar**, u32int*);
int sunUint32Unpack(uchar*, uchar*, uchar**, u32int*);
int sunUint64Pack(uchar*, uchar*, uchar**, u64int*);
int sunUint64Unpack(uchar*, uchar*, uchar**, u64int*);

int sunVarOpaquePack(uchar*, uchar*, uchar**, uchar**, u32int*, u32int);
int sunVarOpaqueUnpack(uchar*, uchar*, uchar**, uchar**, u32int*, u32int);
uint sunVarOpaqueSize(u32int);

int sunFixedOpaquePack(uchar*, uchar*, uchar**, uchar*, u32int);
int sunFixedOpaqueUnpack(uchar*, uchar*, uchar**, uchar*, u32int);
uint sunFixedOpaqueSize(u32int);

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
	int (*pack)(uchar*, uchar*, uchar**, SunCall*);
	int (*unpack)(uchar*, uchar*, uchar**, SunCall*);
	uint (*size)(SunCall*);
	void (*fmt)(Fmt*, SunCall*);
	uint sizeoftype;
};

SunStatus sunCallPack(SunProg*, uchar*, uchar*, uchar**, SunCall*);
SunStatus sunCallUnpack(SunProg*, uchar*, uchar*, uchar**, SunCall*);
SunStatus sunCallUnpackAlloc(SunProg*, SunCallType, uchar*, uchar*, uchar**, SunCall**);
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
	uchar *data;
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
	ulong	maxwait;
	ulong	xidgen;
	int		nsend;
	int		nresend;
	struct {
		ulong min;
		ulong max;
		ulong avg;
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

int	sunClientRpc(SunClient*, ulong, SunCall*, SunCall*, uchar**);
void	sunClientClose(SunClient*);
void	sunClientFlushRpc(SunClient*, ulong);
void	sunClientProg(SunClient*, SunProg*);


/*
 * Provided by callers.
 * Should remove dependence on this, but hard.
 */
void	*emalloc(ulong);
void *erealloc(void*, ulong);


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
	u32int prog;
	u32int vers;
	u32int prot;
	u32int port;
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
	u32int port;
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
	u32int prog;
	u32int vers;
	u32int proc;
	uchar *data;
	u32int count;
};

struct PortRCallit {
	SunCall call;
	u32int port;
	uchar *data;
	u32int count;
};

extern SunProg portProg;
