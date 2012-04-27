typedef struct SmbSession SmbSession;
typedef struct SmbTree SmbTree;
typedef struct SmbService SmbService;
typedef struct SmbPeerInfo SmbPeerInfo;
typedef struct SmbTransaction SmbTransaction;
typedef struct SmbBuffer SmbBuffer;
typedef struct SmbIdMap SmbIdMap;
typedef struct SmbSearch SmbSearch;
typedef struct SmbDirCache SmbDirCache;
typedef struct SmbFile SmbFile;
typedef struct SmbSharedFile SmbSharedFile;
typedef struct SmbCifsSession SmbCifsSession;
typedef struct SmbServerInfo SmbServerInfo;
typedef struct SmbRapServerInfo1 SmbRapServerInfo1;
typedef struct SmbFindFileBothDirectoryInfo SmbFindFileBothDirectoryInfo;
typedef struct SmbLock SmbLock;
typedef struct SmbLockList SmbLockList;
typedef struct SmbSlut SmbSlut;

#pragma incomplete SmbIdMap
#pragma incomplete SmbBuffer
#pragma incomplete SmbLockList

typedef int SMBCIFSWRITEFN(SmbCifsSession *cifs, void *buf, long n);
typedef int SMBCIFSACCEPTFN(SmbCifsSession *cifs, SMBCIFSWRITEFN **fnp);
typedef void SMBIDMAPAPPLYFN(void *magic, void *p);

struct SmbPeerInfo {
	ulong capabilities;
	ushort maxlen;
	uchar securitymode;
	ushort maxmpxcount;
	ushort maxnumbervcs;
	ulong maxbuffersize;
	ulong maxrawsize;
	ulong sessionkey;
	vlong utc;
	short tzoff;
	uchar encryptionkeylength;
	uchar *encryptionkey;
	char *oemdomainname;
};

struct SmbTransaction {
	struct {
		char *name;
		ulong tpcount;
		uchar *parameters;
		ulong pcount;
		ulong tdcount;
		uchar *data;
		ulong maxpcount;
		ulong maxdcount;
		ulong maxscount;
		ulong dcount;
		ushort scount;
		ushort *setup;
		ushort flags;
	} in;
	struct {
		ulong tpcount;
		ulong tdcount;
		SmbBuffer *parameters;
		SmbBuffer *data;
		ushort *setup;
	} out;
};

enum {
	SmbSessionNeedNegotiate,
	SmbSessionNeedSetup,
	SmbSessionEstablished,
};

struct SmbSession {
	NbSession *nbss;
	SmbCifsSession *cifss;
	uchar nextcommand;
	SmbBuffer *response;
	SmbPeerInfo peerinfo;
	Chalstate *cs;
	struct {
		char *accountname;	
		char *primarydomain;
		char *nativeos;
		char *nativelanman;
		ushort maxmpxcount;
		MSchapreply mschapreply;
	} client;
	SmbTransaction transaction;
	SmbIdMap *fidmap;
	SmbIdMap *tidmap;
	SmbIdMap *sidmap;
	int state;
	uchar errclass;
	ushort error;
	int tzoff;		// as passed to client during negotiation
	SmbService *serv;
};

typedef struct SmbHeader {
	uchar command;
	union {
		struct {
			uchar errclass;
			ushort error;
		};
		ulong status;
	};
	uchar flags;
	ushort flags2;
	union {
		struct {
			ushort pidhigh;
			uchar securitysignature[8];
		};
	};
	ushort tid;
	ushort pid;
	ushort uid;
	ushort mid;
	uchar wordcount;
} SmbHeader;

typedef enum SmbProcessResult {
	SmbProcessResultOk,
	SmbProcessResultUnimp,
	SmbProcessResultFormat,
	SmbProcessResultMisc,
	SmbProcessResultError,
	SmbProcessResultReply,
	SmbProcessResultDie,
} SmbProcessResult;

typedef SmbProcessResult SMBPROCESSFN(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *);
typedef struct SmbOpTableEntry SmbOpTableEntry;
struct SmbOpTableEntry {
	char *name;
	SMBPROCESSFN *process;
	int debug;
};

extern SmbOpTableEntry smboptable[256];

typedef struct SmbGlobals SmbGlobals;

extern SmbGlobals smbglobals;

struct SmbServerInfo {
	char *name;
	char *nativelanman;
	uchar vmaj, vmin;
	ulong stype;
	char *remark;
};

struct SmbGlobals {
	int maxreceive;
	int unicode;
	SmbServerInfo serverinfo;
	char *nativeos;
	char *primarydomain;
	NbName nbname;
	char *accountname;
	char *mailslotbrowse;
	char *pipelanman;
	int l2sectorsize;
	int l2allocationsize;
	int convertspace;
	struct {
		int fd;
		int print;
		int tids;
		int sids;
		int fids;
		int rap2;
		int find;
		int query;
		int sharedfiles;
		int sessions;
		int rep;
		int poolparanoia;
		int locks;
	} log;
};

struct SmbTree {
	long id;
	SmbService *serv;
};

struct SmbService {
	Ref;
	char *name;
	char *type;
	ushort stype;
	char *path;
	char *remark;
	SmbService *next;
};

extern SmbService *smbservices;

typedef struct SmbClient SmbClient;


typedef struct SmbTransactionMethod {
	int (*encodeprimary)(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
		SmbBuffer *ob, uchar *wordcount, ushort *bytecount, char **errmsgp);
	int (*encodesecondary)(SmbTransaction *t, SmbHeader *h, SmbBuffer *ob, char **errmsgp);
	int (*sendrequest)(void *magic, SmbBuffer *ob, char **errmsgp);
	int (*receiveintermediate)(void *magic, uchar *wordcountp, ushort *bytecountp, char **errmsgp);
	int (*receiveresponse)(void *magic, SmbBuffer *ib, char **errmsgp);
	int (*decoderesponse)(SmbTransaction *t, SmbHeader *h, uchar *pdata, SmbBuffer *b, char **errmsgp);
	int (*encoderesponse)(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
		SmbBuffer *ob, char **errmsgp);
	int (*sendresponse)(void *magic, SmbBuffer *ob, char **errmsgp);
} SmbTransactionMethod;

extern SmbTransactionMethod smbtransactionmethoddgram;

struct SmbSearch {
	long id;
	SmbTree *t;
	SmbDirCache *dc;
	Reprog *rep;
	ushort tid;
};

struct SmbFile {
	long id;
	SmbTree *t;		// tree this belongs to
	int fd;
	char *name;
	int p9mode;		// how it was opened
	int share;			// additional sharing restictions added by this fid
	int ioallowed;
	SmbSharedFile *sf;
};

struct SmbSharedFile {
	ushort type;
	ulong dev;
	vlong path;
//	char *name;
	int share;			// current share level
	int deleteonclose;
	SmbLockList *locklist;
};

struct SmbLock {
	vlong base;
	vlong limit;
	SmbSession *s;		// owning session
	ushort pid;		// owning pid
};

struct SmbCifsSession {
	int fd;
	void *magic;
};

struct SmbClient {
	SmbPeerInfo peerinfo;
	NbSession *nbss;
	SmbBuffer *b;
	ushort ipctid;
	ushort sharetid;
	SmbHeader protoh;
};

struct SmbRapServerInfo1 {
	char name[16];
	uchar vmaj;
	uchar vmin;
	ulong type;
	char *remark;
};

struct SmbFindFileBothDirectoryInfo {
	ulong fileindex;
	vlong creationtime;
	vlong lastaccesstime;
	vlong lastwritetime;
	vlong changetime;
	vlong endoffile;
	vlong allocationsize;
	ulong extfileattributes;
	char *filename;
};

enum {
	SMB_STRING_UNALIGNED = 1,
	SMB_STRING_UPCASE = 2,
	SMB_STRING_UNTERMINATED = 4,
	SMB_STRING_UNICODE = 8,
	SMB_STRING_ASCII = 16,
	SMB_STRING_REVPATH = 32,
	SMB_STRING_PATH = 64,
	SMB_STRING_CONVERT_MASK = SMB_STRING_PATH | SMB_STRING_REVPATH | SMB_STRING_UPCASE,
};

struct SmbDirCache {
	Dir *buf;
	long n;
	long i;
};

typedef struct SmbTrans2OpTableEntry SmbTrans2OpTableEntry;
typedef SmbProcessResult SMBTRANS2PROCESSFN(SmbSession *s, SmbHeader *h);
struct SmbTrans2OpTableEntry {
	char *name;
	SMBTRANS2PROCESSFN *process;
	int debug;
};
extern SmbTrans2OpTableEntry smbtrans2optable[];
extern int smbtrans2optablesize;

struct SmbSlut {
	char *name;
	int val;
};

extern SmbSlut smbopenmodeslut[];
extern SmbSlut smbsharemodeslut[];
