enum {
	NbNameLen = 16,	
	NbnsTimeoutBroadcast = 1000,
	NbnsRetryBroadcast = 3,
	NbnsPort = 137,
	NbDgramMaxLen = 576,
};

typedef struct NbnsHdr {
	uchar name_trn_id[2];
	uchar ctrl[2];
	uchar qdcount[2];
	uchar ancount[2];
	uchar nscount[2];
	uchar arcount[2];
} NbnsHdr;

enum {
	NbnsResponse = 1 << 15
};

enum {
	NbnsOpShift = 11,
	NbnsOpMask = 0xf,
	NbnsOpQuery = 0,
	NbnsOpRegistration = 5,
	NbnsOpRelease = 6,
	NbnsOpWack = 7,
	NbnsOpRefresh = 8
};

enum {
	NbnsFlagBroadcast = (1 << 4),
	NbnsFlagRecursionAvailable = (1 << 7),
	NbnsFlagRecursionDesired = (1 << 8),
	NbnsFlagTruncation = (1 << 9),
	NbnsFlagAuthoritativeAnswer = (1 << 10),
};

enum {
	NbnsRcodeShift = 0,
	NbnsRcodeMask = 0xf,
};

enum {
	NbnsQuestionTypeNb = 0x0020,
	NbnsQuestionTypeNbStat = 0x0021,
	NbnsQuestionClassIn = 0x0001,
};

enum {
	NbnsResourceTypeA = 0x0001,
	NbnsResourceTypeNs = 0x0002,
	NbnsResourceTypeNull = 0x000a,
	NbnsResourceTypeNb = 0x0020,
	NbnsResourceTypeNbStat = 0x0021,
	NbnsResourceClassIn = 0x0001,
};

typedef struct NbnsMessageQuestion NbnsMessageQuestion;
typedef struct NbnsMessageResource NbnsMessageResource;

typedef uchar NbName[NbNameLen];
int nbnamedecode(uchar *base, uchar *p, uchar *ep, NbName name);
int nbnameencode(uchar *p, uchar *ep, NbName name);
int nbnameequal(NbName name1, NbName name2);
void nbnamecpy(NbName n1, NbName n2);
void nbmknamefromstring(NbName nbname, char *string);
void nbmknamefromstringandtype(NbName nbname, char *string, uchar type);
void nbmkstringfromname(char *buf, int buflen, NbName name);
#pragma varargck type "B" uchar *

int nbnamefmt(Fmt *);

struct NbnsMessageQuestion {
	NbName name;
	ushort type;
	ushort class;
	NbnsMessageQuestion *next;
};

NbnsMessageQuestion *nbnsmessagequestionnew(NbName name, ushort type, ushort class);

struct NbnsMessageResource {
	NbName name;
	ushort type;
	ushort class;
	ulong ttl;
	ushort rdlength;
	uchar *rdata;
	NbnsMessageResource *next;
};
NbnsMessageResource *nbnsmessageresourcenew(NbName name, ushort type, ushort class, ulong ttl, int rdcount, uchar *rdata);

typedef struct NbnsMessage {
	ushort id;
	int response;
	int opcode;
	int broadcast;
	int recursionavailable;
	int recursiondesired;
	int truncation;
	int authoritativeanswer;
	int rcode;
	NbnsMessageQuestion *q;
	NbnsMessageResource *an;
	NbnsMessageResource *ns;
	NbnsMessageResource *ar;
} NbnsMessage;

NbnsMessage *nbnsmessagenew(void);
void nbnsmessageaddquestion(NbnsMessage *s, NbnsMessageQuestion *q);
void nbnsmessageaddresource(NbnsMessageResource **rp, NbnsMessageResource *r);
NbnsMessage *nbnsconvM2S(uchar *ap, int nap);
void nbnsmessagefree(NbnsMessage **sp);
void nbnsdumpmessage(NbnsMessage *s);
int nbnsconvS2M(NbnsMessage *s, uchar *ap, int nap);


NbnsMessage *nbnsmessagenamequeryrequestnew(ushort id, int broadcast, NbName name);
NbnsMessage *nbnsmessagenameregistrationrequestnew(ushort id, int broadcast, NbName name, ulong ttl, uchar *ipaddr);

typedef struct NbnsTransaction NbnsTransaction;

struct NbnsTransaction {
	ushort id;
	Channel *c;
	NbnsTransaction *next;
};
ushort nbnsnextid(void);

int nbnsfindname(uchar *serveripaddr, NbName name, uchar *ipaddr, ulong *ttlp);
int nbnsaddname(uchar *serveripaddr, NbName name, ulong ttl, uchar *ipaddr);

NbnsTransaction *nbnstransactionnew(NbnsMessage *request, uchar *ipaddr);
void nbnstransactionfree(NbnsTransaction **tp);

typedef struct NbnsAlarm NbnsAlarm;

struct NbnsAlarm {
	Channel *c;
	vlong expirems;
	NbnsAlarm *next;
};

void nbnsalarmset(NbnsAlarm *a, ulong millisec);
void nbnsalarmcancel(NbnsAlarm *a);
void nbnsalarmfree(NbnsAlarm **ap);
NbnsAlarm *nbnsalarmnew(void);
void nbnsalarmend(void);

typedef struct NbSession NbSession;
typedef int NBSSWRITEFN(NbSession *s, void *buf, long n);

struct NbSession {
	int fd;
	void *magic;
	NbName from;
	NbName to;
};

int nbsslisten(NbName to, NbName from, int (*accept)(void *magic, NbSession *s, NBSSWRITEFN **write), void *magic);
NbSession *nbssconnect(NbName to, NbName from);
void nbssfree(NbSession *s);

typedef struct NbScatterGather NbScatterGather;

struct NbScatterGather {
	void *p;
	long l;
};

int nbssgatherwrite(NbSession *s, NbScatterGather *a);
long nbssscatterread(NbSession *, NbScatterGather *a);
int nbsswrite(NbSession *s, void *buf, long n);
long nbssread(NbSession *s, void *buf, long n);
void *nbemalloc(ulong);

int nbnameresolve(NbName name, uchar *ipaddr);

void nbdumpdata(void *data, long datalen);

typedef struct NbDgram {
	uchar type;
	uchar flags;
	ushort id;
	uchar srcip[IPaddrlen];
	ushort srcport;
	union {
		struct {
			ushort length;
			ushort offset;
			NbName srcname;
			NbName dstname;
			uchar *data;
		} datagram;
		struct {
			uchar code;
		} error;
		struct {
			NbName dstname;
		} query;
	};
} NbDgram;

enum {
	NbDgramDirectUnique = 0x10,
	NbDgramDirectGroup,
	NbDgramBroadcast,
	NbDgramError,
	NbDgramQueryRequest,
	NbDgramPositiveQueryResponse,
	NbDgramNegativeQueryResponse,
	NbDgramMore = 1,
	NbDgramFirst = 2,
	NbDgramPort = 138,
	NbDgramErrorDestinationNameNotPresent = 0x82,
	NbDgramMaxPacket = 576,
};

typedef struct NbDgramSendParameters {
	NbName to;
	uchar type;
} NbDgramSendParameters;

int nbdgramconvM2S(NbDgram *s, uchar *p, uchar *ep);
int nbdgramconvS2M(uchar *p, uchar *ep, NbDgram *s);
void nbdgramdump(NbDgram *s);
int nbdgramsendto(uchar *ipaddr, ushort port, NbDgram *s);
int nbdgramsend(NbDgramSendParameters *p, unsigned char *data, long datalen);
char *nbdgramlisten(NbName to, int (*deliver)(void *magic, NbDgram *s), void *magic);

int nbnametablefind(NbName name, int add);
int nbnameisany(NbName name);

int nbremotenametablefind(NbName name, uchar *ipaddr);
int nbremotenametableadd(NbName name, uchar *ipaddr, ulong ttl);

typedef struct NbGlobals {
	uchar myipaddr[IPaddrlen];
	uchar bcastaddr[IPaddrlen];
	NbName myname;
} NbGlobals;

extern NbGlobals nbglobals;
extern NbName nbnameany;

int nbinit(void);
char *nbudpannounce(ushort port, int *fdp);

extern int nbudphdrsize;
