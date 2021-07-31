typedef struct Ndbtuple Ndbtuple;

enum
{
	/* RR types; see: http://www.iana.org/assignments/dns-parameters */
	Ta=		1,
	Tns=		2,
	Tmd=		3,
	Tmf=		4,
	Tcname=		5,
	Tsoa=		6,
	Tmb=		7,
	Tmg=		8,
	Tmr=		9,
	Tnull=		10,
	Twks=		11,
	Tptr=		12,
	Thinfo=		13,
	Tminfo=		14,
	Tmx=		15,
	Ttxt=		16,
	Trp=		17,
	Tafsdb=		18,
	Tx25=		19,
	Tisdn=		20,
	Trt=		21,
	Tnsap=		22,
	Tnsapptr=	23,
	Tsig=		24,
	Tkey=		25,
	Tpx=		26,
	Tgpos=		27,
	Taaaa=		28,
	Tloc=		29,
	Tnxt=		30,
	Teid=		31,
	Tnimloc=	32,
	Tsrv=		33,
	Tatma=		34,
	Tnaptr=		35,
	Tkx=		36,
	Tcert=		37,
	Ta6=		38,
	Tdname=		39,
	Tsink=		40,
	Topt=		41,
	Tapl=		42,
	Tds=		43,
	Tsshfp=		44,
	Tipseckey=	45,
	Trrsig=		46,
	Tnsec=		47,
	Tdnskey=	48,

	Tspf=		99,
	Tuinfo=		100,
	Tuid=		101,
	Tgid=		102,
	Tunspec=	103,

	/* query types (all RR types are also queries) */
	Ttkey=	249,	/* transaction key */
	Ttsig=	250,	/* transaction signature */
	Tixfr=	251,	/* incremental zone transfer */
	Taxfr=	252,	/* zone transfer */
	Tmailb=	253,	/* { Tmb, Tmg, Tmr } */
	Tmaila= 254,	/* obsolete */
	Tall=	255,	/* all records */

	/* classes */
	Csym=	0,	/* internal symbols */
	Cin=	1,	/* internet */
	Ccs,		/* CSNET (obsolete) */
	Cch,		/* Chaos net */
	Chs,		/* Hesiod (?) */

	/* class queries (all class types are also queries) */
	Call=	255,	/* all classes */

	/* opcodes */
	Oquery=		0<<11,		/* normal query */
	Oinverse=	1<<11,		/* inverse query (retired) */
	Ostatus=	2<<11,		/* status request */
	Onotify=	4<<11,		/* notify slaves of updates */
	Oupdate=	5<<11,
	Omask=		0xf<<11,	/* mask for opcode */

	/* response codes */
	Rok=		0,
	Rformat=	1,	/* format error */
	Rserver=	2,	/* server failure (e.g. no answer from something) */
	Rname=		3,	/* bad name */
	Runimplimented=	4,	/* unimplemented */
	Rrefused=	5,	/* we don't like you */
	Ryxdomain=	6,	/* name exists when it should not */
	Ryxrrset=	7,	/* rr set exists when it should not */
	Rnxrrset=	8,	/* rr set that should exist does not */
	Rnotauth=	9,	/* not authoritative */
	Rnotzone=	10,	/* name not in zone */
	Rbadvers=	16,	/* bad opt version */
/*	Rbadsig=	16, */	/* also tsig signature failure */
	Rbadkey=	17,		/* key not recognized */
	Rbadtime=	18,		/* signature out of time window */
	Rbadmode=	19,		/* bad tkey mode */
	Rbadname=	20,		/* duplicate key name */
	Rbadalg=	21,		/* algorithm not supported */
	Rmask=		0x1f,	/* mask for response */
	Rtimeout=	0x20,	/* timeout sending (for internal use only) */

	/* bits in flag word (other than opcode and response) */
	Fresp=		1<<15,	/* message is a response */
	Fauth=		1<<10,	/* true if an authoritative response */
	Ftrunc=		1<<9,	/* truncated message */
	Frecurse=	1<<8,	/* request recursion */
	Fcanrec=	1<<7,	/* server can recurse */

	Domlen=		256,	/* max domain name length (with NULL) */
	Labellen=	256,	/* max domain label length (with NULL) */
	Strlen=		256,	/* max string length (with NULL) */
	Iplen=		32,	/* max ascii ip address length (with NULL) */

	/* time to live values (in seconds) */
	Min=		60,
	Hour=		60*Min,		/* */
	Day=		24*Hour,	/* Ta, Tmx */
	Week=		7*Day,		/* Tsoa, Tns */
	Year=		52*Week,
	DEFTTL=		Day,

	/* reserved time (can't be timed out earlier) */
	Reserved=	5*Min,

	/* packet sizes */
	Maxudp=		512,	/* maximum bytes per udp message */
	Maxudpin=	2048,	/* maximum bytes per udp message */

	/* length of domain name hash table */
	HTLEN= 		4*1024,

	Maxpath=	128,	/* size of mntpt */

	RRmagic=	0xdeadbabe,
	DNmagic=	0xa110a110,

	/* parallelism */
	Maxactive=	32,

	Maxreqtm=	60,	/* max. seconds to process a request */
};

typedef struct Area	Area;
typedef struct Block	Block;
typedef struct Cert	Cert;
typedef struct DN	DN;
typedef struct DNSmsg	DNSmsg;
typedef struct Key	Key;
typedef struct Null	Null;
typedef struct RR	RR;
typedef struct Request	Request;
typedef struct SOA	SOA;
typedef struct Server	Server;
typedef struct Sig	Sig;
typedef struct Txt	Txt;

/*
 *  a structure to track a request and any slave process handling it
 */
struct Request
{
	int	isslave;	/* pid of slave */
	ulong	aborttime;	/* time at which we give up */
	jmp_buf	mret;		/* where master jumps to after starting a slave */
	int	id;
};

/*
 *  a domain name
 */
struct DN
{
	DN	*next;		/* hash collision list */
	ulong	magic;
	char	*name;		/* owner */
	RR	*rr;		/* resource records off this name */
	ulong	referenced;	/* time last referenced */
	ulong	lookuptime;	/* last time we tried to get a better value */
	ushort	class;		/* RR class */
	char	refs;		/* for mark and sweep */
	char	nonexistent;	/* true if we get an authoritative nx for this domain */
	ulong	ordinal;
	QLock	querylck;	/* permit only 1 query per domain name at a time */
};

/*
 *  security info
 */
struct Block
{
	int	dlen;
	uchar	*data;
};
struct Key
{
	int	flags;
	int	proto;
	int	alg;
	Block;
};
struct Cert
{
	int	type;
	int	tag;
	int	alg;
	Block;
};
struct Sig
{
	Cert;
	int	labels;
	ulong	ttl;
	ulong	exp;
	ulong	incep;
	DN	*signer;
};
struct Null
{
	Block;
};

/*
 *  text strings
 */
struct Txt
{
	Txt	*next;
	char	*p;
};

/*
 *  an unpacked resource record
 */
struct RR
{
	RR	*next;
	ulong	magic;
	DN	*owner;		/* domain that owns this resource record */
	uintptr	pc;
	ulong	ttl;		/* time to live to be passed on */
	ulong	expire;		/* time this entry expires locally */
	ulong	marker;		/* used locally when scanning rrlists */
	ushort	type;		/* RR type */
	ushort	query;		/* query type is in response to */
	uchar	auth;		/* flag: authoritative */
	uchar	db;		/* flag: from database */
	uchar	cached;		/* flag: rr in cache */
	uchar	negative;	/* flag: this is a cached negative response */

	union {			/* discriminated how? negative & type? */
		DN	*negsoaowner;	/* soa for cached negative response */
		DN	*host;	/* hostname - soa, cname, mb, md, mf, mx, ns */
		DN	*cpu;	/* cpu type - hinfo */
		DN	*mb;	/* mailbox - mg, minfo */
		DN	*ip;	/* ip address - a */
		DN	*rp;	/* rp arg - rp */
		ulong	arg0;
	};
	union {			/* discriminated how? negative & type? */
		int	negrcode; /* response code for cached negative response */
		DN	*rmb;	/* responsible maibox - minfo, soa, rp */
		DN	*ptr;	/* pointer to domain name - ptr */
		DN	*os;	/* operating system - hinfo */
		ulong	pref;	/* preference value - mx */
		ulong	local;	/* ns served from local database - ns */
		ulong	arg1;
	};
	union {			/* discriminated by type */
		SOA	*soa;	/* soa timers - soa */
		Key	*key;
		Cert	*cert;
		Sig	*sig;
		Null	*null;
		Txt	*txt;
	};
};

/*
 *  list of servers
 */
struct Server
{
	Server	*next;
	char	*name;
};

/*
 *  timers for a start of authenticated record.  all ulongs are in seconds.
 */
struct SOA
{
	ulong	serial;		/* zone serial # */
	ulong	refresh;	/* zone refresh interval */
	ulong	retry;		/* zone retry interval */
	ulong	expire;		/* time to expiration */
	ulong	minttl;		/* min. time to live for any entry */

	Server	*slaves;	/* slave servers */
};

/*
 *  domain messages
 */
struct DNSmsg
{
	ushort	id;
	int	flags;
	int	qdcount;	/* questions */
	RR 	*qd;
	int	ancount;	/* answers */
	RR	*an;
	int	nscount;	/* name servers */
	RR	*ns;
	int	arcount;	/* hints */
	RR	*ar;
};

/*
 *  definition of local area for dblookup
 */
struct Area
{
	Area	*next;

	int	len;		/* strlen(area->soarr->owner->name) */
	RR	*soarr;		/* soa defining this area */
	int	neednotify;
	int	needrefresh;
};

enum
{
	Recurse,
	Dontrecurse,
	NOneg,
	OKneg,
};

extern int	cachedb;
extern char	*dbfile;
extern int	debug;
extern Area	*delegated;
extern char	*logfile;
extern int	maxage;		/* age of oldest entry in cache (secs) */
extern char	mntpt[];
extern int	needrefresh;
extern int	norecursion;
extern ulong	now;		/* time base */
extern Area	*owned;
extern int	resolver;
extern int	sendnotifies;
extern ulong	target;
extern int	testing;	/* test cache whenever removing a DN */
extern char	*trace;
extern int	traceactivity;
extern char	*zonerefreshprogram;

#pragma	varargck	type	"R"	RR*
#pragma	varargck	type	"Q"	RR*


/* dn.c */
extern char	*rrtname[];
extern char	*rname[];
extern unsigned	nrname;
extern char	*opname[];

void	db2cache(int);
void	dninit(void);
DN*	dnlookup(char*, int, int);
void	dnage(DN*);
void	dnageall(int);
void	dnagedb(void);
void	dnauthdb(void);
void	dnget(void);
void	dnpurge(void);
void	dnput(void);
Area*	inmyarea(char*);
void	rrattach(RR*, int);
RR*	rralloc(int);
void	rrfree(RR*);
void	rrfreelist(RR*);
RR*	rrlookup(DN*, int, int);
RR*	rrcat(RR**, RR*);
RR**	rrcopy(RR*, RR**);
RR*	rrremneg(RR**);
RR*	rrremtype(RR**, int);
int	rrfmt(Fmt*);
int	rravfmt(Fmt*);
int	rrsupported(int);
int	rrtype(char*);
char*	rrname(int, char*, int);
int	tsame(int, int);
void	dndump(char*);
int	getactivity(Request*, int);
void	putactivity(int);
void	abort(); /* char*, ... */;
void	warning(char*, ...);
void	slave(Request*);
void	dncheck(void*, int);
void	unique(RR*);
int	subsume(char*, char*);
RR*	randomize(RR*);
void*	emalloc(int);
char*	estrdup(char*);
void	dnptr(uchar*, uchar*, char*, int, int);
void	addserver(Server**, char*);
Server*	copyserverlist(Server*);
void	freeserverlist(Server*);

/* dnarea.c */
void	refresh_areas(Area*);
void	freearea(Area**);
void	addarea(DN *dp, RR *rp, Ndbtuple *t);

/* dblookup.c */
RR*	dblookup(char*, int, int, int, int);
RR*	dbinaddr(DN*, int);
int	baddelegation(RR*, RR*, uchar*);
RR*	dnsservers(int);
RR*	domainlist(int);
int	opendatabase(void);

/* dns.c */
char*	walkup(char*);
RR*	getdnsservers(int);
void	logreply(int, uchar*, DNSmsg*);
void	logsend(int, int, uchar*, char*, char*, int);
void	procsetname(char *fmt, ...);

/* dnresolve.c */
RR*	dnresolve(char*, int, int, Request*, RR**, int, int, int, int*);
int	udpport(char *);
int	mkreq(DN *dp, int type, uchar *buf, int flags, ushort reqno);

/* dnserver.c */
void	dnserver(DNSmsg*, DNSmsg*, Request*, uchar *, int);
void	dnudpserver(char*);
void	dntcpserver(char*);

/* dnnotify.c */
void	dnnotify(DNSmsg*, DNSmsg*, Request*);
void	notifyproc(void);

/* convDNS2M.c */
int	convDNS2M(DNSmsg*, uchar*, int);

/* convM2DNS.c */
char*	convM2DNS(uchar*, int, DNSmsg*, int*);
