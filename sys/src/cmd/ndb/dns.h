typedef struct Ndbtuple Ndbtuple;

enum
{
	/* RR types */
	Ta=	1,
	Tns=	2,
	Tmd=	3,
	Tmf=	4,
	Tcname=	5,
	Tsoa=	6,
	Tmb=	7,
	Tmg=	8,
	Tmr=	9,
	Tnull=	10,
	Twks=	11,
	Tptr=	12,
	Thinfo=	13,
	Tminfo=	14,
	Tmx=	15,
	Ttxt=	16,
	Trp=	17,
	Tsig=	24,
	Tkey=	25,
	Tcert=	37,

	/* query types (all RR types are also queries) */
	Tixfr=	251,	/* incremental zone transfer */
	Taxfr=	252,	/* zone transfer */
	Tmailb=	253,	/* { Tmb, Tmg, Tmr } */	
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
	Oinverse=	1<<11,		/* inverse query */
	Ostatus=	2<<11,		/* status request */
	Omask=		0xf<<11,	/* mask for opcode */

	/* response codes */
	Rok=		0,
	Rformat=	1,	/* format error */
	Rserver=	2,	/* server failure (e.g. no answer from something) */
	Rname=		3,	/* bad name */
	Runimplimented=	4,	/* unimplemented */
	Rrefused=	5,	/* we don't like you */
	Rmask=		0xf,	/* mask for response */
	Rtimeout=	0x10,	/* timeout sending (for internal use only) */

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

	/* packet header sizes */
	Maxudp=		512,	/* maximum bytes per udp message */
	Maxudpin=	2048,	/* maximum bytes per udp message */

	/* length of domain name hash table */
	HTLEN= 		4*1024,

	RRmagic=	0xdeadbabe,
	DNmagic=	0xa110a110,

	/* parallelism */
	Maxactive=	32,
};

typedef struct DN	DN;
typedef struct DNSmsg	DNSmsg;
typedef struct RR	RR;
typedef struct SOA	SOA;
typedef struct Area	Area;
typedef struct Request	Request;
typedef struct Key	Key;
typedef struct Cert	Cert;
typedef struct Sig	Sig;
typedef struct Null	Null;

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
	int	magic;
	DN	*next;		/* hash collision list */
	char	*name;		/* owner */
	RR	*rr;		/* resource records off this name */
	ulong	referenced;	/* time last referenced */
	ulong	lookuptime;	/* last time we tried to get a better value */
	ushort	class;		/* RR class */
	char	refs;		/* for mark and sweep */
	char	nonexistent;	/* true if we get an authoritative nx for this domain */
};

/*
 *  security info
 */
struct Key
{
	int	flags;
	int	proto;
	int	alg;
	int	dlen;
	uchar	*data;
};
struct Cert
{
	int	type;
	int	tag;
	int	alg;
	int	dlen;
	uchar	*data;
};
struct Sig
{
	int	type;
	int	alg;
	int	labels;
	ulong	ttl;
	ulong	exp;
	ulong	incep;
	int	tag;
	DN	*signer;
	int	dlen;
	uchar	*data;
};
struct Null
{
	int	dlen;
	uchar	*data;
};

/*
 *  an unpacked resource record
 */
struct RR
{
	int	magic;
	RR	*next;
	DN	*owner;		/* domain that owns this resource record */
	uchar	negative;	/* this is a cached negative response */
	ulong	pc;
	ulong	ttl;		/* time to live to be passed on */
	ulong	expire;		/* time this entry expires locally */
	ushort	type;		/* RR type */
	ushort	query;		/* query tyis is in response to */
	uchar	auth;		/* authoritative */
	uchar	db;		/* from database */
	uchar	cached;		/* rr in cache */
	uchar	marker;		/* used locally when scanning rrlists */
	union {
		DN	*negsoaowner;	/* soa for cached negative response */
		DN	*host;	/* hostname - soa, cname, mb, md, mf, mx, ns */
		DN	*cpu;	/* cpu type - hinfo */
		DN	*mb;	/* mailbox - mg, minfo */
		DN	*ip;	/* ip addrss - a */
		DN	*txt;	/* first text string - txt, rp */
		int	cruftlen;
		ulong	arg0;
	};
	union {
		int	negrcode;	/* response code for cached negative response */
		DN	*rmb;	/* responsible maibox - minfo, soa, rp */
		DN	*ptr;	/* pointer to domain name - ptr */
		DN	*os;	/* operating system - hinfo */
		ulong	pref;	/* preference value - mx */
		ulong	local;	/* ns served from local database - ns */
		ulong	arg1;
	};
	union {
		SOA	*soa;	/* soa timers - soa */
		Key	*key;
		Cert	*cert;
		Sig	*sig;
		Null	*null;
	};
};

/*
 *  timers for a start of authenticated record
 */
struct SOA
{
	ulong	serial;		/* zone serial # (sec) - soa */
	ulong	refresh;	/* zone refresh interval (sec) - soa */
	ulong	retry;		/* zone retry interval (sec) - soa */
	ulong	expire;		/* time to expiration (sec) - soa */
	ulong	minttl;		/* minimum time to live for any entry (sec) - soa */
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
	Area		*next;

	int		len;		/* strlen(area->soarr->owner->name) */
	RR		*soarr;		/* soa defining this area */
};

enum
{
	Recurse,
	Dontrecurse,
	NOneg,
	OKneg,
};

/* dn.c */
extern char	*rrtname[];
extern char	*rname[];
extern char	*opname[];
extern void	db2cache(int);
extern void	dninit(void);
extern DN*	dnlookup(char*, int, int);
extern void	dnage(DN*);
extern void	dnageall(int);
extern void	dnagedb(void);
extern void	dnauthdb(void);
extern void	dnget(void);
extern void	dnpurge(void);
extern void	dnput(void);
extern Area*	inmyarea(char*);
extern void	rrattach(RR*, int);
extern RR*	rralloc(int);
extern void	rrfree(RR*);
extern void	rrfreelist(RR*);
extern RR*	rrlookup(DN*, int, int);
extern RR*	rrcat(RR**, RR*);
extern RR**	rrcopy(RR*, RR**);
extern RR*	rrremneg(RR**);
extern RR*	rrremtype(RR**, int);
extern int	rrfmt(Fmt*);
extern int	rravfmt(Fmt*);
extern int	rrtype(char*);
extern char*	rrname(int, char*, int);
extern int	tsame(int, int);
extern void	dndump(char*);
extern int	getactivity(Request*);
extern void	putactivity(void);
extern void	abort(); /* char*, ... */;
extern void	warning(char*, ...);
extern void	slave(Request*);
extern void	dncheck(void*, int);
extern void	unique(RR*);
extern int	subsume(char*, char*);
extern RR*	randomize(RR*);
extern void*	emalloc(int);
extern char*	estrdup(char*);

/* dblookup.c */
extern RR*	dblookup(char*, int, int, int, int);
extern RR*	dbinaddr(DN*, int);
extern int	baddelegation(RR*, RR*, uchar*);
extern RR*	dnsservers(int);
extern RR*	domainlist(int);
extern int	opendatabase(void);

/* dns.c */
extern char*	walkup(char*);
extern RR*	dnresolve(char*, int, int, Request*, RR**, int, int, int, int*);
extern RR*	getdnsservers(int);
extern void	logreply(int, uchar*, DNSmsg*);
extern void	logsend(int, int, uchar*, char*, char*, int);

/* dnserver.c */
extern void	dnserver(DNSmsg*, DNSmsg*, Request*);
extern void	dnudpserver(char*);
extern void	dntcpserver(char*);

/* convDNS2M.c */
extern int	convDNS2M(DNSmsg*, uchar*, int);

/* convM2DNS.c */
extern char*	convM2DNS(uchar*, int, DNSmsg*);

/* malloc.c */
extern void	mallocsanity(void*);
extern void	lasthist(void*, int, ulong);

extern int debug;
extern char	*trace;
extern int	testing;	/* test cache whenever removing a DN */
extern int	cachedb;
extern int	needrefresh;
extern char	*dbfile;
extern char	mntpt[];
extern char	*logfile;
extern int	resolver;
extern int	maxage;		/* age of oldest entry in cache (secs) */

/* time base */
extern ulong	now;

#pragma	varargck	type	"R"	RR*
#pragma	varargck	type	"Q"	RR*

