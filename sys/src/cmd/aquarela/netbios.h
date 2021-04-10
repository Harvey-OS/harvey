/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

enum {
	NbNameLen = 16,
	NbnsTimeoutBroadcast = 1000,
	NbnsRetryBroadcast = 3,
	NbnsPort = 137,
	NbDgramMaxLen = 576,
};

typedef struct NbnsHdr {
	u8 name_trn_id[2];
	u8 ctrl[2];
	u8 qdcount[2];
	u8 ancount[2];
	u8 nscount[2];
	u8 arcount[2];
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

typedef u8 NbName[NbNameLen];
int nbnamedecode(u8 *base, u8 *p, u8 *ep, NbName name);
int nbnameencode(u8 *p, u8 *ep, NbName name);
int nbnameequal(NbName name1, NbName name2);
void nbnamecpy(NbName n1, NbName n2);
void nbmknamefromstring(NbName nbname, char *string);
void nbmknamefromstringandtype(NbName nbname, char *string, u8 type);
void nbmkstringfromname(char *buf, int buflen, NbName name);

int nbnamefmt(Fmt *);

struct NbnsMessageQuestion {
	NbName name;
	u16 type;
	u16 class;
	NbnsMessageQuestion *next;
};

NbnsMessageQuestion *nbnsmessagequestionnew(NbName name, u16 type, u16 class);

struct NbnsMessageResource {
	NbName name;
	u16 type;
	u16 class;
	u32 ttl;
	u16 rdlength;
	u8 *rdata;
	NbnsMessageResource *next;
};
NbnsMessageResource *nbnsmessageresourcenew(NbName name, u16 type, u16 class,
					    u32 ttl, int rdcount,
					    u8 *rdata);

typedef struct NbnsMessage {
	u16 id;
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
NbnsMessage *nbnsconvM2S(u8 *ap, int nap);
void nbnsmessagefree(NbnsMessage **sp);
void nbnsdumpmessage(NbnsMessage *s);
int nbnsconvS2M(NbnsMessage *s, u8 *ap, int nap);


NbnsMessage *nbnsmessagenamequeryrequestnew(u16 id, int broadcast, NbName name);
NbnsMessage *nbnsmessagenameregistrationrequestnew(u16 id, int broadcast, NbName name,
						   u32 ttl,
						   u8 *ipaddr);

typedef struct NbnsTransaction NbnsTransaction;

struct NbnsTransaction {
	u16 id;
	Channel *c;
	NbnsTransaction *next;
};
u16 nbnsnextid(void);

int nbnsfindname(u8 *serveripaddr, NbName name, u8 *ipaddr,
		 u32 *ttlp);
int nbnsaddname(u8 *serveripaddr, NbName name, u32 ttl,
		u8 *ipaddr);

NbnsTransaction *nbnstransactionnew(NbnsMessage *request, u8 *ipaddr);
void nbnstransactionfree(NbnsTransaction **tp);

typedef struct NbnsAlarm NbnsAlarm;

struct NbnsAlarm {
	Channel *c;
	i64 expirems;
	NbnsAlarm *next;
};

void nbnsalarmset(NbnsAlarm *a, u32 millisec);
void nbnsalarmcancel(NbnsAlarm *a);
void nbnsalarmfree(NbnsAlarm **ap);
NbnsAlarm *nbnsalarmnew(void);
void nbnsalarmend(void);

typedef struct NbSession NbSession;
typedef int NBSSWRITEFN(NbSession *s, void *buf, i32 n);

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
	i32 l;
};

int nbssgatherwrite(NbSession *s, NbScatterGather *a);
i32 nbssscatterread(NbSession *, NbScatterGather *a);
int nbsswrite(NbSession *s, void *buf, i32 n);
i32 nbssread(NbSession *s, void *buf, i32 n);
void *nbemalloc(u32);

int nbnameresolve(NbName name, u8 *ipaddr);

void nbdumpdata(void *data, i32 datalen);

typedef struct NbDgram {
	u8 type;
	u8 flags;
	u16 id;
	u8 srcip[IPaddrlen];
	u16 srcport;
	union {
		struct {
			u16 length;
			u16 offset;
			NbName srcname;
			NbName dstname;
			u8 *data;
		} datagram;
		struct {
			u8 code;
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
	u8 type;
} NbDgramSendParameters;

int nbdgramconvM2S(NbDgram *s, u8 *p, u8 *ep);
int nbdgramconvS2M(u8 *p, u8 *ep, NbDgram *s);
void nbdgramdump(NbDgram *s);
int nbdgramsendto(u8 *ipaddr, u16 port, NbDgram *s);
int nbdgramsend(NbDgramSendParameters *p, unsigned char *data, i32 datalen);
char *nbdgramlisten(NbName to, int (*deliver)(void *magic, NbDgram *s), void *magic);

int nbnametablefind(NbName name, int add);
int nbnameisany(NbName name);

int nbremotenametablefind(NbName name, u8 *ipaddr);
int nbremotenametableadd(NbName name, u8 *ipaddr, u32 ttl);

typedef struct NbGlobals {
	u8 myipaddr[IPaddrlen];
	u8 bcastaddr[IPaddrlen];
	NbName myname;
} NbGlobals;

extern NbGlobals nbglobals;
extern NbName nbnameany;

int nbinit(void);
char *nbudpannounce(u16 port, int *fdp);

extern int nbudphdrsize;
