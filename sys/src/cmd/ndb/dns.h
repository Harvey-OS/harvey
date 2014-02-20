#include <thread.h>		/* for Ref */

#define NS2MS(ns) ((ns) / 1000000L)
#define S2MS(s)   ((s)  * 1000LL)

#define timems()	NS2MS(nsec())

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
	Rtimeout=	1<<5,	/* timeout sending (for internal use only) */

	/* bits in flag word (other than opcode and response) */
	Fresp=		1<<15,	/* message is a response */
	Fauth=		1<<10,	/* true if an authoritative response */
	Ftrunc=		1<<9,	/* truncated message */
	Frecurse=	1<<8,	/* request recursion */
	Fcanrec=	1<<7,	/* server can recurse */

	Domlen=		256,	/* max domain name length (with NULL) */
	Labellen=	64,	/* max domain label length (with NULL) */
	Strlen=		256,	/* max string length (with NULL) */

	/* time to live values (in seconds) */
	Min=		60,
	Hour=		60*Min,		/* */
	Day=		24*Hour,	/* Ta, Tmx */
	Week=		7*Day,		/* Tsoa, Tns */
	Year=		52*Week,
	DEFTTL=		Day,

	/* reserved time (can't be timed out earlier) */
	Reserved=	5*Min,

	/* tcp & udp port number */
	Dnsport=	53,

	/*
	 * payload size.  originally, 512 bytes was the upper bound, to
	 * eliminate fragmentation when using udp transport.
	 * with edns (rfc 6891), that has been raised to 4096.
	 * we don't currently generate edns, but we might be sent edns packets.
	 */
	Maxdnspayload=	512,
	Maxpayload=	4096,

	/* length of domain name hash table */
	HTLEN= 		4*1024,

	Maxpath=	128,	/* size of mntpt */
	Maxlcks=	10,	/* max. query-type locks per domain name */

	RRmagic=	0xdeadbabe,
	DNmagic=	0xa110a110,

	/* parallelism: tune; was 32; allow lots */
	Maxactive=	250,

	/* tune; was 60*1000; keep it short */
	Maxreqtm=	8*1000,	/* max. ms to process a request */

	Notauthoritative = 0,
	Authoritative,
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
typedef struct Srv	Srv;
typedef struct Txt	Txt;

/*
 *  a structure to track a request and any slave process handling it
 */
struct Request
{
	int	isslave;	/* pid of slave */
	uvlong	aborttime;	/* time in ms at which we give up */
	jmp_buf	mret;		/* where master jumps to after starting a slave */
	int	id;
	char	*from;		/* who asked us? */
};

typedef struct Querylck Querylck;
struct Querylck
{
	QLock;
//	Rendez;
	Ref;
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
	/* refs was `char' but we've seen refs > 120, so go whole hog */
	ulong	refs;		/* for mark and sweep */
	ulong	ordinal;
	ushort	class;		/* RR class */
	uchar	keep;		/* flag: never age this name */
	uchar	respcode;	/* response code */
/* was:	char	nonexistent; /* true if we get an authoritative nx for this domain */
	/* permit only 1 query per (domain name, type) at a time */
	Querylck querylck[Maxlcks];
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
	uintptr	pc;		/* for tracking memory allocation */
	ulong	ttl;		/* time to live to be passed on */
	ulong	expire;		/* time this entry expires locally */
	ulong	marker;		/* used locally when scanning rrlists */
	ushort	type;		/* RR type */
	ushort	query;		/* query type is in response to */
	uchar	auth;		/* flag: authoritative */
	uchar	db;		/* flag: from database */
	uchar	cached;		/* flag: rr in cache */
	uchar	negative;	/* flag: this is a cached negative response */

	union {			/* discriminated by negative & type */
		DN	*negsoaowner;	/* soa for cached negative response */
		DN	*host;	/* hostname - soa, cname, mb, md, mf, mx, ns, srv */
		DN	*cpu;	/* cpu type - hinfo */
		DN	*mb;	/* mailbox - mg, minfo */
		DN	*ip;	/* ip address - a, aaaa */
		DN	*rp;	/* rp arg - rp */
		uintptr	arg0;	/* arg[01] are compared to find dups in dn.c */
	};
	union {			/* discriminated by negative & type */
		int	negrcode; /* response code for cached negative resp. */
		DN	*rmb;	/* responsible maibox - minfo, soa, rp */
		DN	*ptr;	/* pointer to domain name - ptr */
		DN	*os;	/* operating system - hinfo */
		ulong	pref;	/* preference value - mx */
		ulong	local;	/* ns served from local database - ns */
		ushort	port;	/* - srv */
		uintptr	arg1;	/* arg[01] are compared to find dups in dn.c */
	};
	union {			/* discriminated by type */
		SOA	*soa;	/* soa timers - soa */
		Key	*key;
		Cert	*cert;
		Sig	*sig;
		Null	*null;
		Txt	*txt;
		Srv	*srv;
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
 *  timers for a start-of-authority record.  all ulongs are in seconds.
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
 * srv (service location) record (rfc2782):
 * _service._proto.name ttl class(IN) 'SRV' priority weight port target
 */
struct Srv
{
	ushort	pri;
	ushort	weight;
};

typedef struct Rrlist Rrlist;
struct Rrlist
{
	int	count;
	RR	*rrs;
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

typedef struct Cfg Cfg;
struct Cfg {
	int	cachedb;
	int	resolver;
	int	justforw;	/* flag: pure resolver, just forward queries */
	int	serve;		/* flag: serve udp queries */
	int	inside;
	int	straddle;
};

/* (udp) query stats */
typedef struct {
	QLock;
	ulong	slavehiwat;	/* procs */
	ulong	qrecvd9p;	/* query counts */
	ulong	qrecvdudp;
	ulong	qsent;
	ulong	qrecvd9prpc;	/* packet count */
	ulong	alarms;
	/* reply times by count */
	ulong	under10ths[3*10+2];	/* under n*0.1 seconds, n is index */
	ulong	tmout;
	ulong	tmoutcname;
	ulong	tmoutv6;

	ulong	answinmem;	/* answers in memory */
	ulong	negans;		/* negative answers received */
	ulong	negserver;	/* neg ans with Rserver set */
	ulong	negbaddeleg;	/* neg ans with bad delegations */
	ulong	negbdnoans;	/* ⋯ and no answers */
	ulong	negnorname;	/* neg ans with no Rname set */
	ulong	negcached;	/* neg ans cached */
} Stats;

Stats stats;

enum
{
	Recurse,
	Dontrecurse,
	NOneg,
	OKneg,
};

extern Cfg	cfg;
extern char	*dbfile;
extern int	debug;
extern Area	*delegated;
extern char	*logfile;
extern int	maxage;		/* age of oldest entry in cache (secs) */
extern char	mntpt[];
extern int	needrefresh;
extern int	norecursion;
extern ulong	now;		/* time base */
extern vlong	nowns;
extern Area	*owned;
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
extern Lock	dnlock;

void	abort(); /* char*, ... */;
void	addserver(Server**, char*);
Server*	copyserverlist(Server*);
void	db2cache(int);
void	dnage(DN*);
void	dnageall(int);
void	dnagedb(void);
void	dnageallnever(void);
void	dnagenever(DN *, int);
void	dnauthdb(void);
void	dncheck(void*, int);
void	dndump(char*);
void	dnget(void);
void	dninit(void);
DN*	dnlookup(char*, int, int);
void	dnptr(uchar*, uchar*, char*, int, int, int);
void	dnpurge(void);
void	dnput(void);
void	dnslog(char*, ...);
void	dnstats(char *file);
void*	emalloc(int);
char*	estrdup(char*);
void	freeanswers(DNSmsg *mp);
void	freeserverlist(Server*);
int	getactivity(Request*, int);
Area*	inmyarea(char*);
void	putactivity(int);
RR*	randomize(RR*);
RR*	rralloc(int);
void	rrattach(RR*, int);
int	rravfmt(Fmt*);
RR*	rrcat(RR**, RR*);
RR**	rrcopy(RR*, RR**);
int	rrfmt(Fmt*);
void	rrfree(RR*);
void	rrfreelist(RR*);
RR*	rrlookup(DN*, int, int);
char*	rrname(int, char*, int);
RR*	rrremneg(RR**);
RR*	rrremtype(RR**, int);
int	rrsupported(int);
int	rrtype(char*);
void	slave(Request*);
int	subsume(char*, char*);
int	tsame(int, int);
void	unique(RR*);
void	warning(char*, ...);

/* dnarea.c */
void	refresh_areas(Area*);
void	freearea(Area**);
void	addarea(DN *dp, RR *rp, Ndbtuple *t);

/* dblookup.c */
int	baddelegation(RR*, RR*, uchar*);
RR*	dbinaddr(DN*, int);
RR*	dblookup(char*, int, int, int, int);
void	dnforceage(void);
RR*	dnsservers(int);
RR*	domainlist(int);
int	insideaddr(char *dom);
int	insidens(uchar *ip);
int	myaddr(char *addr);
int	opendatabase(void);
uchar*	outsidens(int);

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
int	seerootns(void);
void	initdnsmsg(DNSmsg *mp, RR *rp, int flags, ushort reqno);
DNSmsg*	newdnsmsg(RR *rp, int flags, ushort reqno);

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

#pragma varargck argpos dnslog 1
