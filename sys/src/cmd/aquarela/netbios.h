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
	uint8_t name_trn_id[2];
	uint8_t ctrl[2];
	uint8_t qdcount[2];
	uint8_t ancount[2];
	uint8_t nscount[2];
	uint8_t arcount[2];
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

typedef uint8_t NbName[NbNameLen];
int nbnamedecode(uint8_t *base, uint8_t *p, uint8_t *ep, NbName name);
int nbnameencode(uint8_t *p, uint8_t *ep, NbName name);
int nbnameequal(NbName name1, NbName name2);
void nbnamecpy(NbName n1, NbName n2);
void nbmknamefromstring(NbName nbname, char *string);
void nbmknamefromstringandtype(NbName nbname, char *string, uint8_t type);
void nbmkstringfromname(char *buf, int buflen, NbName name);

int nbnamefmt(Fmt *);

struct NbnsMessageQuestion {
	NbName name;
	uint16_t type;
	uint16_t class;
	NbnsMessageQuestion *next;
};

NbnsMessageQuestion *nbnsmessagequestionnew(NbName name, uint16_t type, uint16_t class);

struct NbnsMessageResource {
	NbName name;
	uint16_t type;
	uint16_t class;
	uint32_t ttl;
	uint16_t rdlength;
	uint8_t *rdata;
	NbnsMessageResource *next;
};
NbnsMessageResource *nbnsmessageresourcenew(NbName name, uint16_t type, uint16_t class,
					    uint32_t ttl, int rdcount,
					    uint8_t *rdata);

typedef struct NbnsMessage {
	uint16_t id;
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
NbnsMessage *nbnsconvM2S(uint8_t *ap, int nap);
void nbnsmessagefree(NbnsMessage **sp);
void nbnsdumpmessage(NbnsMessage *s);
int nbnsconvS2M(NbnsMessage *s, uint8_t *ap, int nap);


NbnsMessage *nbnsmessagenamequeryrequestnew(uint16_t id, int broadcast, NbName name);
NbnsMessage *nbnsmessagenameregistrationrequestnew(uint16_t id, int broadcast, NbName name,
						   uint32_t ttl,
						   uint8_t *ipaddr);

typedef struct NbnsTransaction NbnsTransaction;

struct NbnsTransaction {
	uint16_t id;
	Channel *c;
	NbnsTransaction *next;
};
uint16_t nbnsnextid(void);

int nbnsfindname(uint8_t *serveripaddr, NbName name, uint8_t *ipaddr,
		 uint32_t *ttlp);
int nbnsaddname(uint8_t *serveripaddr, NbName name, uint32_t ttl,
		uint8_t *ipaddr);

NbnsTransaction *nbnstransactionnew(NbnsMessage *request, uint8_t *ipaddr);
void nbnstransactionfree(NbnsTransaction **tp);

typedef struct NbnsAlarm NbnsAlarm;

struct NbnsAlarm {
	Channel *c;
	int64_t expirems;
	NbnsAlarm *next;
};

void nbnsalarmset(NbnsAlarm *a, uint32_t millisec);
void nbnsalarmcancel(NbnsAlarm *a);
void nbnsalarmfree(NbnsAlarm **ap);
NbnsAlarm *nbnsalarmnew(void);
void nbnsalarmend(void);

typedef struct NbSession NbSession;
typedef int NBSSWRITEFN(NbSession *s, void *buf, int32_t n);

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
	int32_t l;
};

int nbssgatherwrite(NbSession *s, NbScatterGather *a);
int32_t nbssscatterread(NbSession *, NbScatterGather *a);
int nbsswrite(NbSession *s, void *buf, int32_t n);
int32_t nbssread(NbSession *s, void *buf, int32_t n);
void *nbemalloc(uint32_t);

int nbnameresolve(NbName name, uint8_t *ipaddr);

void nbdumpdata(void *data, int32_t datalen);

typedef struct NbDgram {
	uint8_t type;
	uint8_t flags;
	uint16_t id;
	uint8_t srcip[IPaddrlen];
	uint16_t srcport;
	union {
		struct {
			uint16_t length;
			uint16_t offset;
			NbName srcname;
			NbName dstname;
			uint8_t *data;
		} datagram;
		struct {
			uint8_t code;
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
	uint8_t type;
} NbDgramSendParameters;

int nbdgramconvM2S(NbDgram *s, uint8_t *p, uint8_t *ep);
int nbdgramconvS2M(uint8_t *p, uint8_t *ep, NbDgram *s);
void nbdgramdump(NbDgram *s);
int nbdgramsendto(uint8_t *ipaddr, uint16_t port, NbDgram *s);
int nbdgramsend(NbDgramSendParameters *p, unsigned char *data, int32_t datalen);
char *nbdgramlisten(NbName to, int (*deliver)(void *magic, NbDgram *s), void *magic);

int nbnametablefind(NbName name, int add);
int nbnameisany(NbName name);

int nbremotenametablefind(NbName name, uint8_t *ipaddr);
int nbremotenametableadd(NbName name, uint8_t *ipaddr, uint32_t ttl);

typedef struct NbGlobals {
	uint8_t myipaddr[IPaddrlen];
	uint8_t bcastaddr[IPaddrlen];
	NbName myname;
} NbGlobals;

extern NbGlobals nbglobals;
extern NbName nbnameany;

int nbinit(void);
char *nbudpannounce(uint16_t port, int *fdp);

extern int nbudphdrsize;
