/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

//#pragma incomplete SmbIdMap
//#pragma incomplete SmbBuffer
//#pragma incomplete SmbLockList

typedef int SMBCIFSWRITEFN(SmbCifsSession *cifs, void *buf, i32 n);
typedef int SMBCIFSACCEPTFN(SmbCifsSession *cifs, SMBCIFSWRITEFN **fnp);
typedef void SMBIDMAPAPPLYFN(void *magic, void *p);

struct SmbPeerInfo {
	u32 capabilities;
	u16 maxlen;
	u8 securitymode;
	u16 maxmpxcount;
	u16 maxnumbervcs;
	u32 maxbuffersize;
	u32 maxrawsize;
	u32 sessionkey;
	i64 utc;
	i16 tzoff;
	u8 encryptionkeylength;
	u8 *encryptionkey;
	char *oemdomainname;
};

struct SmbTransaction {
	struct {
		char *name;
		u32 tpcount;
		u8 *parameters;
		u32 pcount;
		u32 tdcount;
		u8 *data;
		u32 maxpcount;
		u32 maxdcount;
		u32 maxscount;
		u32 dcount;
		u16 scount;
		u16 *setup;
		u16 flags;
	} in;
	struct {
		u32 tpcount;
		u32 tdcount;
		SmbBuffer *parameters;
		SmbBuffer *data;
		u16 *setup;
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
	u8 nextcommand;
	SmbBuffer *response;
	SmbPeerInfo peerinfo;
	Chalstate *cs;
	struct {
		char *accountname;
		char *primarydomain;
		char *nativeos;
		char *nativelanman;
		u16 maxmpxcount;
		MSchapreply mschapreply;
	} client;
	SmbTransaction transaction;
	SmbIdMap *fidmap;
	SmbIdMap *tidmap;
	SmbIdMap *sidmap;
	int state;
	u8 errclass;
	u16 error;
	int tzoff;		// as passed to client during negotiation
	SmbService *serv;
};

typedef struct SmbHeader {
	u8 command;
	union {
		struct {
			u8 errclass;
			u16 error;
		};
		u32 status;
	};
	u8 flags;
	u16 flags2;
	union {
		struct {
			u16 pidhigh;
			u8 securitysignature[8];
		};
	};
	u16 tid;
	u16 pid;
	u16 uid;
	u16 mid;
	u8 wordcount;
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

typedef SmbProcessResult SMBPROCESSFN(SmbSession *s, SmbHeader *h, u8 *pdata,
				      SmbBuffer *b);
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
	u8 vmaj, vmin;
	u32 stype;
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
	i32 id;
	SmbService *serv;
};

struct SmbService {
	Ref	ref;
	char *name;
	char *type;
	u16 stype;
	char *path;
	char *remark;
	SmbService *next;
};

extern SmbService *smbservices;

typedef struct SmbClient SmbClient;


typedef struct SmbTransactionMethod {
	int (*encodeprimary)(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
		SmbBuffer *ob, u8 *wordcount, u16 *bytecount,
		char **errmsgp);
	int (*encodesecondary)(SmbTransaction *t, SmbHeader *h, SmbBuffer *ob, char **errmsgp);
	int (*sendrequest)(void *magic, SmbBuffer *ob, char **errmsgp);
	int (*receiveintermediate)(void *magic, u8 *wordcountp,
				   u16 *bytecountp, char **errmsgp);
	int (*receiveresponse)(void *magic, SmbBuffer *ib, char **errmsgp);
	int (*decoderesponse)(SmbTransaction *t, SmbHeader *h, u8 *pdata,
			      SmbBuffer *b, char **errmsgp);
	int (*encoderesponse)(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
		SmbBuffer *ob, char **errmsgp);
	int (*sendresponse)(void *magic, SmbBuffer *ob, char **errmsgp);
} SmbTransactionMethod;

extern SmbTransactionMethod smbtransactionmethoddgram;

struct SmbSearch {
	i32 id;
	SmbTree *t;
	SmbDirCache *dc;
	Reprog *rep;
	u16 tid;
};

struct SmbFile {
	i32 id;
	SmbTree *t;		// tree this bei32s to
	int fd;
	char *name;
	int p9mode;		// how it was opened
	int share;			// additional sharing restictions added by this fid
	int ioallowed;
	SmbSharedFile *sf;
};

struct SmbSharedFile {
	u16 type;
	u32 dev;
	i64 path;
//	char *name;
	int share;			// current share level
	int deleteonclose;
	SmbLockList *locklist;
};

struct SmbLock {
	i64 base;
	i64 limit;
	SmbSession *s;		// owning session
	u16 pid;		// owning pid
};

struct SmbCifsSession {
	int fd;
	void *magic;
};

struct SmbClient {
	SmbPeerInfo peerinfo;
	NbSession *nbss;
	SmbBuffer *b;
	u16 ipctid;
	u16 sharetid;
	SmbHeader protoh;
};

struct SmbRapServerInfo1 {
	char name[16];
	u8 vmaj;
	u8 vmin;
	u32 type;
	char *remark;
};

struct SmbFindFileBothDirectoryInfo {
	u32 fileindex;
	i64 creationtime;
	i64 lastaccesstime;
	i64 lastwritetime;
	i64 changetime;
	i64 endoffile;
	i64 allocationsize;
	u32 extfileattributes;
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
	i32 n;
	i32 i;
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
