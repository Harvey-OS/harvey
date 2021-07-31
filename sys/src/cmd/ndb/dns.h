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

	/* query types (all RR types are also queries) */
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
	Rserver=	2,	/* server failure */
	Rname=		3,	/* bad name */
	Runimplimented=	4,	/* unimplemented */
	Rrefused=	5,	/* we don't like you */
	Rmask=		0xf,	/* mask for response */

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

	/* time ti live values (in seconds) */
	Min=		60,
	Hour=		60*Min,		/* */
	Day=		24*Hour,	/* Ta, Tmx */
	Week=		7*Day,		/* Tsoa, Tns */

	/* packet header sizes */
	Maxudp=		512,	/* maximum bytes per udp message */
};

typedef struct DN	DN;
typedef struct DNSmsg	DNSmsg;
typedef struct RR	RR;
typedef struct SOA	SOA;
typedef struct Request	Request;

/*
 *  a structure to track a request and any slave process handling it
 */
struct Request
{
	int	isslave;	/* pid of slave */
	jmp_buf	mret;		/* where master jumps to after starting a slave */
};

/*
 *  a domain name
 */
struct DN
{
	DN	*next;		/* hash collision list */
	char	*name;		/* owner */
	RR	*rr;		/* resource records off this name */
	int	class;		/* RR class */
	ulong	reserved;	/* don't age until after this time */
};

/*
 *  an unpacked resource record
 */
struct RR
{
	RR	*next;
	DN	*owner;		/* domain that owns this resource record */
	ulong	ttl;		/* time to live (secs) */
	ushort	type;		/* RR type */
	uchar	auth;		/* authoritative */
	uchar	db;		/* from database */
	union {
		DN	*host;	/* hostname - soa, cname, mb, md, mf, mx, ns */
		DN	*cpu;	/* cpu type - hinfo */
		DN	*mb;	/* mailbox - mg, minfo */
		DN	*ip;	/* ip addrss - a */
		ulong	arg0;
	};
	union {
		DN	*rmb;	/* responsible maibox - minfo, soa */
		DN	*ptr;	/* pointer to a name space node - ptr */
		DN	*os;	/* operating system - hinfo */
		ulong	pref;	/* preference value - mx */
		ulong	local;	/* ns served from local database - ns */
		ulong	arg1;
	};
	SOA	*soa;	/* soa timers - soa */
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
	ulong	id;
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

/* dn.c */
extern char	*rrtname[];
extern char	*rname[];
extern char	*opname[];
extern void	dninit(void);
extern DN*	dnlookup(char*, int, int);
extern void	dnage(DN*);
extern void	rrattach(RR*, int);
extern RR*	rralloc(int);
extern void	rrfree(RR*);
extern void	rrfreelist(RR*);
extern RR*	rrlookup(DN*, int);
extern RR*	rrcat(RR**, RR*, int);
extern int	rrconv(void*, Fconv*);
extern int	rrtype(char*);
extern char*	rrname(int, char*);
extern int	cistrcmp(char*, char*);
extern int	tsame(int, int);
extern void	dndump(char*);

/* dblookup.c */
extern RR*	dblookup(char*, int, int, int);
extern RR*	dbinaddr(DN*);

/* dns.c */
extern char*	walkup(char*);
extern RR*	dnresolve(char*, int, int, Request*, RR**);
extern void	fatal(char*, ...);
extern void	warning(char*, ...);
extern void	slave(Request*);

/* dnserver.c */
extern void	dnserver(void);

/* convDNS2M.c */
extern int	convDNS2M(DNSmsg*, uchar*, int);

/* convM2DNS.c */
extern char*	convM2DNS(uchar*, int, DNSmsg*);

extern int debug;
extern char *dbfile;
