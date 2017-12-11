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

typedef int SMBCIFSWRITEFN(SmbCifsSession *cifs, void *buf, int32_t n);
typedef int SMBCIFSACCEPTFN(SmbCifsSession *cifs, SMBCIFSWRITEFN **fnp);
typedef void SMBIDMAPAPPLYFN(void *magic, void *p);

struct SmbPeerInfo {
	uint32_t capabilities;
	uint16_t maxlen;
	uint8_t securitymode;
	uint16_t maxmpxcount;
	uint16_t maxnumbervcs;
	uint32_t maxbuffersize;
	uint32_t maxrawsize;
	uint32_t sessionkey;
	int64_t utc;
	int16_t tzoff;
	uint8_t encryptionkeylength;
	uint8_t *encryptionkey;
	char *oemdomainname;
};

struct SmbTransaction {
	struct {
		char *name;
		uint32_t tpcount;
		uint8_t *parameters;
		uint32_t pcount;
		uint32_t tdcount;
		uint8_t *data;
		uint32_t maxpcount;
		uint32_t maxdcount;
		uint32_t maxscount;
		uint32_t dcount;
		uint16_t scount;
		uint16_t *setup;
		uint16_t flags;
	} in;
	struct {
		uint32_t tpcount;
		uint32_t tdcount;
		SmbBuffer *parameters;
		SmbBuffer *data;
		uint16_t *setup;
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
	uint8_t nextcommand;
	SmbBuffer *response;
	SmbPeerInfo peerinfo;
	Chalstate *cs;
	struct {
		char *accountname;
		char *primarydomain;
		char *nativeos;
		char *nativelanman;
		uint16_t maxmpxcount;
		MSchapreply mschapreply;
	} client;
	SmbTransaction transaction;
	SmbIdMap *fidmap;
	SmbIdMap *tidmap;
	SmbIdMap *sidmap;
	int state;
	uint8_t errclass;
	uint16_t error;
	int tzoff;		// as passed to client during negotiation
	SmbService *serv;
};

typedef struct SmbHeader {
	uint8_t command;
	union {
		struct {
			uint8_t errclass;
			uint16_t error;
		};
		uint32_t status;
	};
	uint8_t flags;
	uint16_t flags2;
	union {
		struct {
			uint16_t pidhigh;
			uint8_t securitysignature[8];
		};
	};
	uint16_t tid;
	uint16_t pid;
	uint16_t uid;
	uint16_t mid;
	uint8_t wordcount;
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

typedef SmbProcessResult SMBPROCESSFN(SmbSession *s, SmbHeader *h, uint8_t *pdata, SmbBuffer *b);
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
	uint8_t vmaj, vmin;
	uint32_t stype;
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
	int32_t id;
	SmbService *serv;
};

struct SmbService {
	Ref	ref;
	char *name;
	char *type;
	uint16_t stype;
	char *path;
	char *remark;
	SmbService *next;
};

extern SmbService *smbservices;

typedef struct SmbClient SmbClient;


typedef struct SmbTransactionMethod {
	int (*encodeprimary)(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
		SmbBuffer *ob, uint8_t *wordcount, uint16_t *bytecount, char **errmsgp);
	int (*encodesecondary)(SmbTransaction *t, SmbHeader *h, SmbBuffer *ob, char **errmsgp);
	int (*sendrequest)(void *magic, SmbBuffer *ob, char **errmsgp);
	int (*receiveintermediate)(void *magic, uint8_t *wordcountp, uint16_t *bytecountp, char **errmsgp);
	int (*receiveresponse)(void *magic, SmbBuffer *ib, char **errmsgp);
	int (*decoderesponse)(SmbTransaction *t, SmbHeader *h, uint8_t *pdata, SmbBuffer *b, char **errmsgp);
	int (*encoderesponse)(SmbTransaction *t, SmbHeader *h, SmbPeerInfo *p,
		SmbBuffer *ob, char **errmsgp);
	int (*sendresponse)(void *magic, SmbBuffer *ob, char **errmsgp);
} SmbTransactionMethod;

extern SmbTransactionMethod smbtransactionmethoddgram;

struct SmbSearch {
	int32_t id;
	SmbTree *t;
	SmbDirCache *dc;
	Reprog *rep;
	uint16_t tid;
};

struct SmbFile {
	int32_t id;
	SmbTree *t;		// tree this beint32_ts to
	int fd;
	char *name;
	int p9mode;		// how it was opened
	int share;			// additional sharing restictions added by this fid
	int ioallowed;
	SmbSharedFile *sf;
};

struct SmbSharedFile {
	uint16_t type;
	uint32_t dev;
	int64_t path;
//	char *name;
	int share;			// current share level
	int deleteonclose;
	SmbLockList *locklist;
};

struct SmbLock {
	int64_t base;
	int64_t limit;
	SmbSession *s;		// owning session
	uint16_t pid;		// owning pid
};

struct SmbCifsSession {
	int fd;
	void *magic;
};

struct SmbClient {
	SmbPeerInfo peerinfo;
	NbSession *nbss;
	SmbBuffer *b;
	uint16_t ipctid;
	uint16_t sharetid;
	SmbHeader protoh;
};

struct SmbRapServerInfo1 {
	char name[16];
	uint8_t vmaj;
	uint8_t vmin;
	uint32_t type;
	char *remark;
};

struct SmbFindFileBothDirectoryInfo {
	uint32_t fileindex;
	int64_t creationtime;
	int64_t lastaccesstime;
	int64_t lastwritetime;
	int64_t changetime;
	int64_t endoffile;
	int64_t allocationsize;
	uint32_t extfileattributes;
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
	int32_t n;
	int32_t i;
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
