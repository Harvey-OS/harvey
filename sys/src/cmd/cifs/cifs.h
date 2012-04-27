/* cifs.h */

enum {
	Proot		= 1,		/* LSBs of qid.path for root dir */
	Pinfo		= 2,		/* LSBs of qid.path for info files */
	Pshare		= 4,		/* LSBs of qid.path for share dirs */
	NBHDRLEN	= 4,		/* length of a netbios header */
	T2HDRLEN	= 64,		/* Transaction2 header length */
	NO_UID		= 0xffff,	/* initial UID on connect */
	NO_TID		= 0xffff,	/* initial TID on connect */
	MTU		= 0xefff,	/* our MTU */
	CACHETIME	= 2,		/* seconds read-ahead is valid for */
	CIFS_FNAME_MAX	= 0xff,		/* max file path component len */
	OVERHEAD	= 80,		/* max packet overhead when reading & writing */
	IDLE_TIME	= 10,		/* keepalive send rate in mins */
	NBNSTOUT	= 300,		/* Netbios Name Service Timeout (300ms x 3retrys) */
	NBRPCTOUT	= 45*60*1000,	/* Netbios RPC Timeout (45sec) */
	MAX_SHARES	= 4096,		/* static table of shares attached */
	RAP_ERR_MOREINFO= 234,		/* non-error code, more info to be fetched */
	MAX_DFS_PATH	= 512,		/* MS says never more than 250 chars... */
};

enum {
	SMB_COM_CREATE_DIRECTORY	= 0x00,
	SMB_COM_DELETE_DIRECTORY	= 0x01,
	SMB_COM_OPEN			= 0x02,
	SMB_COM_CREATE			= 0x03,
	SMB_COM_CLOSE			= 0x04,
	SMB_COM_FLUSH			= 0x05,
	SMB_COM_DELETE			= 0x06,
	SMB_COM_RENAME			= 0x07,
	SMB_COM_QUERY_INFORMATION	= 0x08,
	SMB_COM_SET_INFORMATION		= 0x09,
	SMB_COM_READ			= 0x0A,
	SMB_COM_WRITE			= 0x0B,
	SMB_COM_LOCK_BYTE_RANGE		= 0x0C,
	SMB_COM_UNLOCK_BYTE_RANGE	= 0x0D,
	SMB_COM_CREATE_TEMPORARY	= 0x0E,
	SMB_COM_CREATE_NEW		= 0x0F,
	SMB_COM_CHECK_DIRECTORY		= 0x10,
	SMB_COM_PROCESS_EXIT		= 0x11,
	SMB_COM_SEEK			= 0x12,
	SMB_COM_LOCK_AND_READ		= 0x13,
	SMB_COM_WRITE_AND_UNLOCK	= 0x14,
	SMB_COM_READ_RAW		= 0x1A,
	SMB_COM_READ_MPX		= 0x1B,
	SMB_COM_READ_MPX_SECONDARY	= 0x1C,
	SMB_COM_WRITE_RAW		= 0x1D,
	SMB_COM_WRITE_MPX		= 0x1E,
	SMB_COM_WRITE_MPX_SECONDARY	= 0x1F,
	SMB_COM_WRITE_COMPLETE		= 0x20,
	SMB_COM_QUERY_SERVER		= 0x21,
	SMB_COM_SET_INFORMATION2	= 0x22,
	SMB_COM_QUERY_INFORMATION2	= 0x23,
	SMB_COM_LOCKING_ANDX		= 0x24,
	SMB_COM_TRANSACTION		= 0x25,
	SMB_COM_TRANSACTION_SECONDARY	= 0x26,
	SMB_COM_IOCTL			= 0x27,
	SMB_COM_IOCTL_SECONDARY		= 0x28,
	SMB_COM_COPY			= 0x29,
	SMB_COM_MOVE			= 0x2A,
	SMB_COM_ECHO			= 0x2B,
	SMB_COM_WRITE_AND_CLOSE		= 0x2C,
	SMB_COM_OPEN_ANDX		= 0x2D,
	SMB_COM_READ_ANDX		= 0x2E,
	SMB_COM_WRITE_ANDX		= 0x2F,
	SMB_COM_NEW_FILE_SIZE		= 0x30,
	SMB_COM_CLOSE_AND_TREE_DISC	= 0x31,
	SMB_COM_TRANSACTION2		= 0x32,
	SMB_COM_TRANSACTION2_SECONDARY	= 0x33,
	SMB_COM_FIND_CLOSE2		= 0x34,
	SMB_COM_FIND_NOTIFY_CLOSE	= 0x35,
	SMB_COM_TREE_CONNECT		= 0x70,
	SMB_COM_TREE_DISCONNECT		= 0x71,
	SMB_COM_NEGOTIATE		= 0x72,
	SMB_COM_SESSION_SETUP_ANDX	= 0x73,
	SMB_COM_LOGOFF_ANDX		= 0x74,
	SMB_COM_TREE_CONNECT_ANDX	= 0x75,
	SMB_COM_QUERY_INFORMATION_DISK	= 0x80,
	SMB_COM_SEARCH			= 0x81,
	SMB_COM_FIND			= 0x82,
	SMB_COM_FIND_UNIQUE		= 0x83,
	SMB_COM_FIND_CLOSE		= 0x84,
	SMB_COM_NT_TRANSACT		= 0xA0,
	SMB_COM_NT_TRANSACT_SECONDARY	= 0xA1,
	SMB_COM_NT_CREATE_ANDX		= 0xA2,
	SMB_COM_NT_CANCEL		= 0xA4,
	SMB_COM_NT_RENAME		= 0xA5,
	SMB_COM_OPEN_PRINT_FILE		= 0xC0,
	SMB_COM_WRITE_PRINT_FILE	= 0xC1,
	SMB_COM_CLOSE_PRINT_FILE	= 0xC2,
	SMB_COM_GET_PRINT_QUEUE		= 0xC3,
	SMB_COM_READ_BULK		= 0xD8,
	SMB_COM_WRITE_BULK		= 0xD9,
	SMB_COM_WRITE_BULK_DATA		= 0xDA,

	TRANS2_OPEN2			= 0x00,
	TRANS2_FIND_FIRST2		= 0x01,
	TRANS2_FIND_NEXT2		= 0x02,
	TRANS2_QUERY_FS_INFORMATION	= 0x03,
	TRANS2_QUERY_PATH_INFORMATION	= 0x05,
	TRANS2_SET_PATH_INFORMATION	= 0x06,
	TRANS2_QUERY_FILE_INFORMATION	= 0x07,
	TRANS2_SET_FILE_INFORMATION	= 0x08,
	TRANS2_CREATE_DIRECTORY 	= 0x0D,
	TRANS2_SESSION_SETUP		= 0x0E,
	TRANS2_GET_DFS_REFERRAL		= 0x10,

	NT_TRANSACT_CREATE 		= 0x01,
	NT_TRANSACT_IOCTL 		= 0x02,
	NT_TRANSACT_SET_SECURITY_DESC 	= 0x03,
	NT_TRANSACT_NOTIFY_CHANGE 	= 0x04,
	NT_TRANSACT_RENAME 		= 0x05,
	NT_TRANSACT_QUERY_SECURITY_DESC = 0x06
};

enum {						/* CIFS flags */
	FL_CASELESS_NAMES	= 1<<3,
	FL_CANNONICAL_NAMES	= 1<<4,

	FL2_KNOWS_LONG_NAMES	= 1<<0,
	FL2_PACKET_SIGNATURES	= 1<<2,
	FL2_HAS_LONG_NAMES	= 1<<6,
	FL2_EXTENDED_SECURITY	= 1<<11,
	FL2_DFS			= 1<<12,
	FL2_PAGEING_IO		= 1<<13,	/* allow read of exec only files */
	FL2_NT_ERRCODES		= 1<<14,
	FL2_UNICODE		= 1<<15,
};

enum {						/* Capabilities Negoiated */
	CAP_RAW_MODE		= 1,
	CAP_MPX_MODE		= 1<<1,
	CAP_UNICODE		= 1<<2,
	CAP_LARGE_FILES		= 1<<3,		/* 64 bit files */
	CAP_NT_SMBS		= 1<<4,
	CAP_RPC_REMOTE_APIS	= 1<<5,
	CAP_STATUS32		= 1<<6,
	CAP_L2_OPLOCKS		= 1<<7,
	CAP_LOCK_READ		= 1<<8,
	CAP_NT_FIND		= 1<<9,
	CAP_DFS			= 1<<12,
	CAP_INFO_PASSTHRU	= 1<<13,
	CAP_LARGE_READX		= 1<<14,
	CAP_LARGE_WRITEX	= 1<<15,
	CAP_UNIX		= 1<<23,
	CAP_BULK_TRANSFER	= 1<<29,
	CAP_COMPRESSED		= 1<<30,
	CAP_EX_SECURE		= 1<<31
};

enum {	/* string prefixes */
	STR_DIALECT 		= 2,
	STR_PATH 		= 3,
	STR_ASCII 		= 4,
};

enum {	/* optional support bits in treeconnect */
	SMB_SUPPROT_SEARCH_BITS = 1,
	SMB_SHARE_IS_IN_DFS 	= 2,
};

enum {	/* DFS referal header flags */
	DFS_HEADER_ROOT	 	= 1,	/* Server type, returns root targets */
	DFS_HEADER_STORAGE 	= 2,	/* server has storage, no more referals */
	DFS_HEADER_FAILBACK 	= 4,	/* target failback enabled */
};

enum {	/* DFS referal entry flags */
	DFS_SERVER_ROOT	 	= 1,	/* Server type, returns root targets */
	DFS_REFERAL_LIST 	= 0x200,	/* reply is a list (v3 only) */
	DFS_REFERAL_SET 	= 0x400,	/* target is a member of a set */
};

enum {	/* share types */
	STYPE_DISKTREE		= 0,
	STYPE_PRINTQ		= 1,
	STYPE_DEVICE		= 2,
	STYPE_IPC		= 3,
	STYPE_SPECIAL		= 4,
	STYPE_TEMP		= 5,
};

enum {	/* Security */
	SECMODE_USER		= 0x01,	/* i.e. not share level security */
	SECMODE_PW_ENCRYPT	= 0x02,
	SECMODE_SIGN_ENABLED	= 0x04,
	SECMODE_SIGN_REQUIRED	= 0x08,
};

enum {	/* file access rights */
	DELETE			= 0x00010000,
	SYNCHRONIZE		= 0x00100000,

	READ_CONTROL		= 0x00020000,
	GENERIC_ALL		= 0x10000000,
	GENERIC_EXECUTE		= 0x20000000,
	GENERIC_WRITE		= 0x40000000,
	GENERIC_READ		= 0x80000000,

	ATTR_READONLY 		= 0x0001,
	ATTR_HIDDEN   		= 0x0002,
	ATTR_SYSTEM   		= 0x0004,
	ATTR_VOLUME   		= 0x0008,
	ATTR_DIRECTORY		= 0x0010,
	ATTR_ARCHIVE  		= 0x0020,
	ATTR_DEVICE   		= 0x0040,
	ATTR_NORMAL   		= 0x0080,
	ATTR_TEMPORARY		= 0x0100,
	ATTR_SPARSE   		= 0x0200,
	ATTR_REPARSE  		= 0x0400,
	ATTR_COMPRESSED		= 0x0800,
	ATTR_OFFLINE   		= 0x100,	/* offline storage */
	ATTR_NOT_CONTENT_INDEXED= 0x2000,
	ATTR_ENCRYPTED 		= 0x4000,
	ATTR_POSIX_SEMANTICS	= 0x01000000,
	ATTR_BACKUP_SEMANTICS	= 0x02000000,
	ATTR_DELETE_ON_CLOSE	= 0x04000000,
	ATTR_SEQUENTIAL_SCAN	= 0x08000000,
	ATTR_RANDOM_ACCESS  	= 0x10000000,
	ATTR_NO_BUFFERING   	= 0x20000000,
	ATTR_WRITE_THROUGH  	= 0x80000000,

	/* ShareAccess flags */
	FILE_NO_SHARE    	= 0,
	FILE_SHARE_READ  	= 1,
	FILE_SHARE_WRITE 	= 2,
	FILE_SHARE_DELETE	= 4,
	FILE_SHARE_ALL   	= 7,

	/* CreateDisposition flags */
	FILE_SUPERSEDE   	= 0,
	FILE_OPEN		= 1,
	FILE_CREATE		= 2,
	FILE_OPEN_IF		= 3,
	FILE_OVERWRITE		= 4,
	FILE_OVERWRITE_IF	= 5,

	/* CreateOptions */
	FILE_DIRECTORY_FILE		= 0x00000001,
	FILE_WRITE_THROUGH		= 0x00000002,
	FILE_SEQUENTIAL_ONLY		= 0x00000004,
	FILE_NO_INTERMEDIATE_BUFFERING	= 0x00000008,
	FILE_SYNCHRONOUS_IO_ALERT	= 0x00000010,
	FILE_SYNCHRONOUS_IO_NONALERT	= 0x00000020,
	FILE_NON_DIRECTORY_FILE		= 0x00000040,
	FILE_CREATE_TREE_CONNECTION	= 0x00000080,
	FILE_COMPLETE_IF_OPLOCKED	= 0x00000100,
	FILE_NO_EA_KNOWLEDGE		= 0x00000200,
	FILE_OPEN_FOR_RECOVERY		= 0x00000400,
	FILE_EIGHT_DOT_THREE_ONLY	= 0x00000400,	/* samba source says so... */
	FILE_RANDOM_ACCESS		= 0x00000800,
	FILE_DELETE_ON_CLOSE		= 0x00001000,
	FILE_OPEN_BY_FILE_ID		= 0x00002000,
	FILE_OPEN_FOR_BACKUP_INTENT	= 0x00004000,
	FILE_NO_COMPRESSION		= 0x00008000,

	/* open/create result codes */
	FILE_WAS_OPENED			= 1,
	FILE_WAS_CREATED		= 2,
	FILE_WAS_OVERWRITTEN		= 3,

	/* ImpersonationLevel flags */
	SECURITY_ANONYMOUS     		= 0,
	SECURITY_IDENTIFICATION		= 1,
	SECURITY_IMPERSONATION		= 2,
	SECURITY_DELEGATION		= 3,

	/* SecurityFlags */
	SECURITY_CONTEXT_TRACKING 	= 1,
	SECURITY_EFFECTIVE_ONLY		= 2,

	/* security descriptor bitmask */
	QUERY_OWNER_SECURITY_INFORMATION = 1,
	QUERY_GROUP_SECURITY_INFORMATION = 2,
	QUERY_DACL_SECURITY_INFORMATION = 4,
	QUERY_SACL_SECURITY_INFORMATION = 8,

};

enum {	/* PathInfo/FileInfo infolevels */
	SMB_INFO_STANDARD              	= 0x1,
	SMB_INFO_IS_NAME_VALID         	= 0x6,
	SMB_QUERY_FILE_BASIC_INFO      	= 0x101,
	SMB_QUERY_FILE_STANDARD_INFO   	= 0x102,
	SMB_QUERY_FILE_NAME_INFO       	= 0x104,
	SMB_QUERY_FILE_ALLOCATION_INFO 	= 0x105,
	SMB_QUERY_FILE_END_OF_FILE_INFO = 0x106,
	SMB_QUERY_FILE_ALL_INFO        	= 0x107,
	SMB_QUERY_ALT_NAME_INFO        	= 0x108,
	SMB_QUERY_FILE_STREAM_INFO     	= 0x109,
	SMB_QUERY_FILE_COMPRESSION_INFO	= 0x10b,
	SMB_QUERY_FILE_UNIX_BASIC      	= 0x200,
	SMB_QUERY_FILE_UNIX_LINK       	= 0x201,

	SMB_SET_FILE_BASIC_INFO	       	= 0x101,
	SMB_SET_FILE_DISPOSITION_INFO  	= 0x102,
	SMB_SET_FILE_ALLOCATION_INFO   	= 0x103,
	SMB_SET_FILE_END_OF_FILE_INFO  	= 0x104,
	SMB_SET_FILE_UNIX_BASIC        	= 0x200,
	SMB_SET_FILE_UNIX_LINK         	= 0x201,
	SMB_SET_FILE_UNIX_HLINK        	= 0x203,
	SMB_SET_FILE_BASIC_INFO2       	= 0x3ec,
	SMB_SET_FILE_RENAME_INFORMATION	= 0x3f2,
	SMB_SET_FILE_ALLOCATION_INFO2  	= 0x3fb,
	SMB_SET_FILE_END_OF_FILE_INFO2 	= 0x3fc,

	/* Find File infolevels */
	SMB_FIND_FILE_DIRECTORY_INFO	= 0x101,
	SMB_FIND_FILE_FULL_DIRECTORY_INFO= 0x102,
	SMB_FIND_FILE_NAMES_INFO	= 0x103,
	SMB_FIND_FILE_BOTH_DIRECTORY_INFO= 0x104,
	SMB_FIND_FILE_UNIX		= 0x202,

	/* Trans2 FindFirst & FindNext */
	CIFS_SEARCH_CLOSE_ALWAYS	= 0x0001,
	CIFS_SEARCH_CLOSE_AT_END	= 0x0002,
	CIFS_SEARCH_RETURN_RESUME	= 0x0004,
	CIFS_SEARCH_CONTINUE_FROM_LAST	= 0x0008,
	CIFS_SEARCH_BACKUP_SEARCH	= 0x0010,

	/* Trans2 FsInfo */
	SMB_INFO_ALLOCATION		= 0x1,
	SMB_INFO_VOLUME			= 0x2,
	SMB_QUERY_FS_VOLUME_INFO	= 0x102,
	SMB_QUERY_FS_SIZE_INFO		= 0x103,
	SMB_QUERY_FS_DEVICE_INFO	= 0x104,
	SMB_QUERY_FS_ATTRIBUTE_INFO	= 0x105,
	SMB_QUERY_CIFS_UNIX_INFO	= 0x200,
};

enum {	/* things to search for in server lookups */
	LOCAL_AUTHORATIVE_ONLY	= 0x40000000,
	LIST_DOMAINS_ONLY	= 0x80000000,
	ALL_LEARNT_IN_DOMAIN	= 0xFFFFFFFF
};

typedef struct {
	char	*user;		/* username */
	char	*windom;	/* remote server's domain name */
	char	*resp[2];	/* ASCII/Unicode or LM/NTLM keys */
	int	len[2];		/* length of above */
	uchar	*mackey[2];	/* Message Authentication key */
} Auth;

typedef struct {
	int	fd;		/* File descriptor for I/O  */
	int	nbt;		/* am using cifs over netbios */
	int	trn;		/* TRN (unique RPC) ID  */
	int	uid;		/* user (authentication) ID  */
	int	seq;		/* sequence number */
	int	seqrun;		/* sequence numbering active */
	int	caps;		/* server capabilities */
	int	support;	/* support bits */
	int	flags;		/* SMB flags  */
	int	flags2;		/* SMB flags 2  */
	int	nocache;	/* disable write behind caching in server */
	int	pid;		/* process ID  */
	int	mid;		/* multiplex ID */
	int	mtu;		/* max size of packet  */
	int	tz;		/* Timezone, mins from UTC  */
	int	isguest;	/* logged in as guest */
	int	secmode;	/* security mode  */
	int	macidx;		/* which MAC is in use, -1 is none */
	uchar	chal[0xff +1];	/* server's challange for authentication  */
	int	challen;	/* length of challange */
	long	slip;		/* time difference between the server and us */
	uvlong	lastfind;	/* nsec when last find peformed */
	QLock	seqlock;	/* sequence number increment */
	QLock	rpclock;	/* actual remote procedure call */
	char	*cname;		/* remote hosts called name (for info) */
	char	*remos;		/* remote hosts OS (for info) */
	Auth	*auth;		/* authentication info */
} Session;

typedef struct {
	Session *s;

	int tid;		/* tree ID received from server */
	int seq;		/* sequence number expected in reply */

	uchar *seqbase; 	/* cifs: pos of sequence number in packet */
	uchar *wordbase; 	/* cifs: base of words section of data  */
	uchar *bytebase; 	/* cifs: base of bytes section of data  */
	uchar *tbase;		/* transactions: start of trans packet */
	uchar *tsetup;		/* transactions: start of setup section */
	uchar *tparam; 		/* transactions: start of params section */
	uchar *tdata; 		/* transactions: start of data section */

	uchar *eop;		/* received end of packet */
	uchar *pos;		/* current pos in packet  */
	uchar *buf;		/* packet buffer, must be last entry in struct  */
} Pkt;

typedef struct {
	char	*name;
	int	tid;		/* not part of the protocol, housekeeping */
	int	options;	/* not part of the protocol, housekeeping */
} Share;

typedef struct {
	long	created;	/* last access time */
	long	accessed;	/* last access time */
	long	written;	/* last written time */
	long	changed;	/* change time */
	uvlong	size;		/* file size */
	long	attribs;	/* attributes */
	char	name[CIFS_FNAME_MAX +1]; /* name */
} FInfo;

typedef struct {
	char	*wrkstn;
	char	*user;
	long	sesstime;
	long	idletime;
} Sessinfo;

typedef struct {
	char	*name;
} Namelist;

typedef struct {
	char	*user;
	char	*comment;
	char	*user_comment;
	char	*fullname;
} Userinfo;

typedef struct {
	char	*name;
	int	type;
	char	*comment;
	int	perms;
	int	maxusrs;
	int	activeusrs;
	char	*path;
	char	*passwd;
} Shareinfo2;

typedef struct {
	char	*name;
	int	major;
	int	minor;
	int	type;
	char	*comment;
} Serverinfo;

typedef struct {
	int	type;	/* o=unknown, 1=CIFS, 2=netware 3=domain */
	int	flags;	/* 1 == strip off consumed chars before resubmitting */
	int	ttl;	/* time to live of this info in secs */
	int	prox;	/* lower value is preferred */
	char	*path;	/* new path */
	char	*addr;	/* new server */
} Refer;

typedef struct {
	char	*node;
	char	*user;
	char	*langroup;
	int	major;
	int	minor;
	char	*pridom;
	char	*otherdoms;
} Wrkstainfo;

typedef struct {
	int	ident;
	int	perms;
	int	locks;
	char	*path;
	char	*user;
} Fileinfo;

extern int Active;
extern int Checkcase;
extern int Dfstout;
extern char *Debug;
extern char *Host;
extern Session *Sess;
extern Share Ipc;

extern Share Shares[MAX_SHARES];
extern int Nshares;

/* main.c */
Qid	mkqid(char *, int, long, int, long);

/* auth.c */
Auth	*getauth(char *, char *, char *, int, uchar *, int);
void	autherr(void);
int	macsign(Pkt *, int);

/* cifs.c */
Session	*cifsdial(char *, char *, char *);
void	cifsclose(Session *);
Pkt	*cifshdr(Session *, Share *, int);
void	pbytes(Pkt *);
int	cifsrpc(Pkt *);
int	CIFSnegotiate(Session *, long *, char *, int, char *, int);
int	CIFSsession(Session *);
int	CIFStreeconnect(Session *, char *, char *, Share *);
int	CIFSlogoff(Session *);
int	CIFStreedisconnect(Session *, Share *);
int	CIFSdeletefile(Session *, Share *, char *);
int	CIFSdeletedirectory(Session *, Share *, char *);
int	CIFScreatedirectory(Session *, Share *, char *);
int	CIFSrename(Session *, Share *, char *, char *);
int	CIFS_NT_opencreate(Session *, Share *, char *, int, int, int, int, int, int, int *, FInfo *);
int	CIFS_SMB_opencreate(Session *, Share *, char *, int, int, int, int *);
vlong	CIFSwrite(Session *, Share *, int, uvlong, void *, vlong);
vlong	CIFSread(Session *, Share *, int, uvlong, void *, vlong, vlong);
int	CIFSflush(Session *, Share *, int);
int	CIFSclose(Session *, Share *, int);
int	CIFSfindclose2(Session *, Share *, int);
int	CIFSecho(Session *);
int	CIFSsetinfo(Session *, Share *, char *, FInfo *);
void	goff(Pkt *, uchar *, char *, int);

/* dfs.c */
char	*mapfile(char *);
int	mapshare(char *, Share **);
int	redirect(Session *, Share *s, char *);
int	dfscacheinfo(Fmt *);
char	*trimshare(char *);

/* doserrstr.c */
char	*doserrstr(uint);

/* fs.c */
int	shareinfo(Fmt *);
int	conninfo(Fmt *);
int	sessioninfo(Fmt *);
int	userinfo(Fmt *);
int	groupinfo(Fmt *);
int	domaininfo(Fmt *);
int	workstationinfo(Fmt *);
int	dfsrootinfo(Fmt *);
int	openfileinfo(Fmt *);
int	dfsrootinfo(Fmt *);
int	filetableinfo(Fmt *); 	/* is in main.c due to C scope */

/* info.c */
int	walkinfo(char *);
int	numinfo(void);
int	dirgeninfo(int, Dir *);
int	makeinfo(int);
int	readinfo(int, char *, int, int);
void	freeinfo(int);

/* main.c */
void	usage(void);
void	dmpkey(char *, void *, int);
void	main(int, char **);

/* misc.c */
char	*strupr(char *);
char	*strlwr(char *);

/* netbios.c */
void	Gmem(uchar **, void *, int);
int	calledname(char *, char *);
int	nbtdial(char *, char *, char *);
void	nbthdr(Pkt *);
int	nbtrpc(Pkt *);
void	xd(char *, void *, int);

/* nterrstr.c */
char	*nterrstr(uint);

/* pack.c */
void	*pmem(Pkt *, void *, int);
void	*ppath(Pkt *, char *);
void	*pstr(Pkt *, char *);
void	*pascii(Pkt *, char *);
void	*pl64(Pkt *, uvlong);
void	*pb32(Pkt *, uint);
void	*pl32(Pkt *, uint);
void	*pb16(Pkt *, uint);
void	*pl16(Pkt *, uint);
void	*p8(Pkt *, uint);
void	*pname(Pkt *, char *, char);
void	*pvtime(Pkt *, uvlong);
void	*pdatetime(Pkt *, long);
void	gmem(Pkt *, void *, int);
void	gstr(Pkt *, char *, int);
void	gascii(Pkt *, char *, int);
uvlong	gl64(Pkt *);
uvlong	gb48(Pkt *);
uint	gb32(Pkt *);
uint	gl32(Pkt *);
uint	gb16(Pkt *);
uint	gl16(Pkt *);
uint	g8(Pkt *);
long	gdatetime(Pkt *);
long	gvtime(Pkt *);
void	gconv(Pkt *, int, char *, int);

/* raperrstr.c */
char	*raperrstr(uint);

/* sid2name.c */
void	upd_names(Session *, Share *, char *, Dir *);

/* trans.c */
int	RAPshareenum(Session *, Share *, Share **);
int	RAPshareinfo(Session *, Share *, char *, Shareinfo2 *);

int	RAPsessionenum(Session *, Share *, Sessinfo **);

int	RAPgroupenum(Session *, Share *, Namelist **);
int	RAPgroupusers(Session *, Share *, char *, Namelist **);

int	RAPuserenum(Session *, Share *, Namelist **);
int	RAPuserenum2(Session *, Share *, Namelist **);
int	RAPuserinfo(Session *, Share *, char *, Userinfo *);

int	RAPServerenum2(Session *, Share *, char *, int, int *, Serverinfo **);
int	RAPServerenum3(Session *, Share *, char *, int, int, Serverinfo *);

int	RAPFileenum2(Session *, Share *, char *, char *, Fileinfo **);

/* trans2.c */
int	T2findfirst(Session *, Share *, int, char *, int *, long *, FInfo *);
int	T2findnext(Session *, Share *, int, char *, int *, long *, FInfo *, int);
int	T2queryall(Session *, Share *, char *, FInfo *);
int	T2querystandard(Session *, Share *, char *, FInfo *);
int	T2setpathinfo(Session *, Share *, char *, FInfo *);
int	T2setfilelength(Session *, Share *, int, FInfo *);
int	T2fsvolumeinfo(Session *, Share *, long *, long *, char *, int);
int	T2fssizeinfo(Session *, Share *, uvlong *, uvlong *);
int	T2getdfsreferral(Session *, Share *, char *, int *, int *, Refer *, int);

/* transnt.c */
int	TNTquerysecurity(Session *, Share *, int, char **, char **);

/* ping.c */
int	ping(char *, int);
