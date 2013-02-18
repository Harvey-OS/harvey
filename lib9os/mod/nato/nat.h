#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <thread.h>
#include "packethdr.h"

// NAT port allocation and session state

enum{	
	MAXSESSIONS = 100003,	// prime, so search hits sentinel
	TCP_TIMEOUT = 21600,	// rfc1631 24hr = 86400; Linux 15min
	UDP_TIMEOUT = 300,	// Cisco 5min = 300; Linux 15min
	FIN_TIMEOUT = 60,	// rfc1631 1min = 60; Linux 15min

	// action byte
	EMPTYBIT	= 0x80,	// 0=empty
	DELETEDBIT	= 0x40,	// 1=deleted
	STATICBIT	= 0x20, // 1=static mapping
	FINIBIT		= 0x08, // 1=inbound  FIN or RST seen
	FINOBIT		= 0x04, // 1=outbound FIN or RST seen
	FINBIT		= FINIBIT|FINOBIT
};

enum{
	OUTBOUND	= 1,	// dir

	// session
	NAT_ERR		= -2,
	NAT_DROP	= -1,
	NAT_PASS	= 0,
	NAT_REWRITE	= 1,
};

// various parameters needed for the control plane

enum{
	A2ISZ 		= 997,  // size of a2imap
	MAXBUFFSZ 	= 1516, // max 1500/20 updates + 16B header
	RANGESZ		= 512,	// chunks of port numbers 
	NOWITHDRAWLIM	= 60,	// seconds before a withdraw can't happen

	MAXNUMNATS	= 16,	// max num of nat boxes (should this be fixed?)
	MAXPREF     	= 16,	// max num of prefixes 
	NWINDOWS    	= 16,	// num of updates remembered for averaging
	MAXEXTADDR    	= 256,	// num of external addresses
	MAXMSGBUFFS	= 5,	// msgbuffs for ring in natbox
	NFREEBUFFS	= 10,	// free msgbuff pool size
	RP		= 521,	// prime for ins2outs

	ACT		= 1,	// used in timerinfo
	NOACT		= 2,
	DONE		= 3,
	EMPTY		= 4,
	MINE		= 5,	// used in i2amapper
	OTHER		= 6,
				// values for Msgbuff.current 
	CURRENT		= 1, 	// being filled in 
	WRITE		= 2, 	// being written out
	FREE 		= 3,	// free to be filled in

				// values for i2amapper.current
	CURR		= 1,	// this range current
	NOTCURR		= 2,	// not

				// values for hashlookup, NATsession
	FINDSLOT	= 1,	// find an empty slot
	FINDSESN	= 2, 	// find a session
	RDLOCK		= 3,
	WRLOCK		= 4,

	HRTBTINTRVL	= 1000,	// control heart-beat interval
};

enum { 
	LO_PORT 	= 511, 
	MID_PORT 	= 1023, 
	HI_PORT 	= 65535, 
	MAXPORT, 
	AVAILABLE 	= -1, 
	EXPOFFSET 	= 10000, 
};

enum {
	RTable		= 0,
	TTable, 
	RUpdate,
	TUpdate,
	RDist,
	TDist, 
	RAge,
	TAge,
	RRange,
	TRange,
	RWithdraw,
	TWithdraw,
	RParttable,
	TParttable, 
};

// message types

typedef struct NATSession {
	uchar iaddr[4];  // inside (local addr of inside host)
	uchar iport[2];
	uchar proto;
	uchar oaddr[4];  // outside (global addr of remote host)
	uchar oport[2];
	uchar action;
	uchar rport[2];
	uchar raddr;     // index for rewritten addr of inside host
	uchar expires[3]; // 16*expires + offset = unix time (seconds)
} NATSession;

typedef struct FiveTuple {
	uchar iaddr[4];	// inside host (local or global addr, port)
	uchar iport[2];
	uchar proto;
	uchar oaddr[4];	// outside host
	uchar oport[2];
} FiveTuple;

typedef struct NATHdr {
	uchar version;
	uchar sid;          // sender id
	uchar mid[4];       // message id
	uchar mtype;        // type
	uchar pktsz[4];     // packetsize (in bytes)
	uchar slack;
} NATHdr;

typedef struct a2imapper {
	uchar nataddr[4];
	int ind;
} a2imapper;

typedef struct Portrange Portrange;
struct Portrange {
	ushort lo;
	ushort hi_port;	// most recently allocated port
	ushort nused;
	ushort current;
	Portrange *next;
	Portrange *prev;
};

typedef struct i2amapper {
	RWLock;
	uchar nataddr[4];
	Portrange *myrangefirst;
	Portrange *myrangelast;
	Portrange *myrangecurrent;
	int conncfd;	// file descriptor (ctl) to the network connection 
	int conndfd;	// file descriptor (data) to the network connection
	ushort nconns;	// num of connections on this external address
	int nranges;	// num of ranges allocated to owner
	ulong lastreq;
	int *p2s;	// pointer to a port2session table
} i2amapper;

typedef struct hosti2a {
	i2amapper i2amap[MAXEXTADDR];
} hosti2a;

typedef struct Owners {
	hosti2a hosts[MAXNUMNATS+1];
} Owners;

typedef struct stable {
	RWLock;
	QLock finlock;
	NATSession table[MAXSESSIONS];
} stable;

typedef struct Msgbuff Msgbuff;
struct Msgbuff {
	QLock;
	uchar buff[MAXBUFFSZ];  // mesg
	int index;              // where to write/read in Msgbuff
	int current;		// = CURRENT, WRITE, FREE 
	Msgbuff *prev;
	Msgbuff *next;
};

typedef struct Msgbuffhd {
	QLock;
	int alloc;
	int len;
	Msgbuff *buffhead;
	Msgbuff *bufftail;
} Msgbuffhd;

typedef struct Timerinfo  Timerinfo;
struct Timerinfo {
	vlong fireat;
	int actiontype;
	int targets[MAXNUMNATS];
	int targetind;
	int targetdone;
	Msgbuff *data;
	Timerinfo *prev;
	Timerinfo *next;
};

typedef struct Timerinfohd {
	QLock;
	int alloc;
	int len;
	Timerinfo *ehead;
	Timerinfo *etail;
} Timerinfohd;

typedef struct buffring {
	int cp;		//current
	int lw;		//last written
	Msgbuff buffs[MAXMSGBUFFS];
} buffring;

extern int numextaddr, numnats, nportranges;
extern Biobuf *bp;
extern hosti2a myhosti2a;
extern Owners allhosts;
extern a2imapper a2imap[A2ISZ];
extern buffring br;
extern Timerinfohd tq, evq;
extern Msgbuffhd freeMlist;
extern stable sessiontable;
extern int verbose;
extern Biobuf *Bout;
extern int natid, msgid;
//extern char** msgtypetab;

extern int	Error(char*,...);
extern void*	emalloc(int);
extern void*	erealloc(void*,int);
extern char*	estrdup(char*);

extern void netlog(char *fmt, ...);
extern char* protoname(uchar proto);
extern char dn(int dir);
extern char* tcpflag(ushort flag);
extern void Bbytes(int n, uchar *b);
extern int FiveTuple_conv(Fmt*);
extern void pr_udp(int dir, uchar *p);
extern void appendT(Timerinfohd *th, Timerinfo *ti);
extern void sinsertT(Timerinfohd *th, Timerinfo *ti);
extern Timerinfo* deleteT(Timerinfohd *th);
extern int NATsession(double t, int dir, FiveTuple *f, ushort synfin, int *raddrind);
extern int proc(char **argv, int fd0, int fd1, int fd2);
extern void NATinit(char *fname);
extern int NATpacket(double tim, int dir, uchar *packet, int *raddrind);
extern void allocinitM(Msgbuffhd *mh, int n);
extern Msgbuff* allocM(Msgbuffhd *mh);
extern void freeM(Msgbuffhd *mh, Msgbuff* mb);
extern Msgbuff* deleteM(Msgbuffhd *mh);
extern void inita2i(int nb);
extern void initi2a(char *fname);
extern void writebuff(void);


