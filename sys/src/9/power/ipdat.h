typedef struct Ipconv	Ipconv;
typedef struct Ipifc	Ipifc;
typedef struct Fragq	Fragq;
typedef struct Ipfrag	Ipfrag;
typedef ulong		Ipaddr;
typedef struct Arpcache	Arpcache;
typedef ushort		Port;
typedef struct Udphdr	Udphdr;
typedef struct Etherhdr	Etherhdr;
typedef struct Reseq	Reseq;
typedef struct Tcp	Tcp;
typedef struct Tcpctl	Tcpctl;
typedef struct Tcphdr	Tcphdr;
typedef struct Timer	Timer;

struct Etherhdr {
#define ETHER_HDR	14
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];

	/* Now we have the ip fields */
#define ETHER_IPHDR	20
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	cksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */
};

/* Ethernet packet types */
#define ET_IP	0x0800

/* A userlevel data gram */
struct Udphdr {
#define UDP_EHSIZE	22
	uchar	d[6];		/* Ethernet destination */
	uchar	s[6];		/* Ethernet source */
	uchar	type[2];	/* Ethernet packet type */
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */

	/* Udp pseudo ip really starts here */
#define UDP_PHDRSIZE	12
#define UDP_HDRSIZE	20
	uchar	Unused;	
	uchar	udpproto;	/* Protocol */
	uchar	udpplen[2];	/* Header plus data length */
	uchar	udpsrc[4];	/* Ip source */
	uchar	udpdst[4];	/* Ip destination */
	uchar	udpsport[2];	/* Source port */
	uchar	udpdport[2];	/* Destination port */
	uchar	udplen[2];	/* data length */
	uchar	udpcksum[2];	/* Checksum */
};

#define TCP_PKT	(TCP_EHSIZE+TCP_IPLEN+TCP_PHDRSIZE)

struct Tcphdr {
#define TCP_EHSIZE	14
	uchar	d[6];		/* Ethernet destination */
	uchar	s[6];		/* Ethernet source */
	uchar	type[2];	/* Ethernet packet type */
#define TCP_IPLEN	8
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */

#define TCP_PHDRSIZE	12	
	uchar	Unused;
	uchar	proto;
	uchar	tcplen[2];
	uchar	tcpsrc[4];
	uchar	tcpdst[4];

#define TCP_HDRSIZE	20
	uchar	tcpsport[2];
	uchar	tcpdport[2];
	uchar	tcpseq[4];
	uchar	tcpack[4];
	uchar	tcpflag[2];
	uchar	tcpwin[2];
	uchar	tcpcksum[2];
	uchar	tcpurg[2];

	/* Options segment */
	uchar	tcpopt[2];
	uchar	tcpmss[2];
	};



struct Timer {
	Timer	*next;
	Timer	*prev;
	int	state;
	int	start;
	int	count;
	void	(*func)(void*);
	void	*arg;
	};

struct Tcpctl {
	QLock;
	uchar	state;		/* Connection state */
	uchar	type;		/* Listening or active connection */
	uchar	code;		/* Icmp code */		
	struct {
		int una;	/* Unacked data pointer */
		int nxt;	/* Next sequence expected */
		int ptr;	/* Data pointer */
		ushort wnd;	/* Tcp send window */
		int up;		/* Urgent data pointer */
		int wl1;
		int wl2;
	} snd;
	int	iss;
	ushort	cwind;
	ushort	ssthresh;
	int	resent;
	struct {
		int nxt;
		ushort wnd;
		int up;
	} rcv;
	int	irs;
	ushort	mss;
	int	rerecv;
	ushort	window;
	int	max_snd;
	int	last_ack;
	char	backoff;
	char	flags;
	char	tos;

	Block	*rcvq;
	ushort	rcvcnt;

	Block	*rcvoobq;
	ushort	rcvoobcnt;

	Block	*sndq;			/* List of data going out */
	ushort	sndcnt;			/* Amount of data in send queue */

	Block	*sndoobq;		/* List of blocks going oob */
	ushort	sndoobcnt;		/* Size of out of band queue */
	ushort	oobmark;		/* Out of band sequence mark */
	char	oobflags;		/* Out of band data flags */

	Reseq	*reseq;			/* Resequencing queue */
	Timer	timer;			 
	Timer	acktimer;		/* Acknoledge timer */
	Timer	rtt_timer;		/* Round trip timer */
	int	rttseq;			/* Round trip sequence */
	int	srtt;			/* Shortened round trip */
	int	mdev;			/* Mean deviation of round trip */
};

struct	Tcp {
	Port	source;
	Port	dest;
	int	seq;
	int	ack;
	char	flags;
	ushort	wnd;
	ushort	up;
	ushort	mss;
	};

struct Reseq {
	Reseq 	*next;
	Tcp	seg;
	Block	*bp;
	ushort	length;
	char	tos;
	};

/* An ip interface used for UDP/TCP/ARP/ICMP */
struct Ipconv {
	QLock;				/* Ref count lock */
	int 	ref;
	Qinfo	*stproto;		/* Stream protocol for this device */
	Ipaddr	dst;			/* Destination from connect */

	Port	psrc;			/* Source port */
	Port	pdst;			/* Destination port */

	uchar	ptype;			/* Source port type */
	Ipifc	*ipinterface;		/* Ip protocol interface */
	Queue	*readq;			/* Pointer to upstream read q */

	QLock	listenq;		/* List of people waiting incoming cons */
	Rendez	listenr;		/* Some where to sleep while waiting */
	Ipconv	*listen;
		
	char	err;			/* Async protocol error */
	int	backlog;		/* Maximum number of waiting connections */
	int	curlog;			/* Number of waiting connections */
	int 	contype;
	Tcpctl	tcpctl;			/* Tcp control block */
};

#define	MAX_TIME	100000000	/* Forever */
#define TCP_ACK		200		/* Timed ack sequence every 200ms */

#define URG	0x20
#define ACK	0x10
#define PSH	0x08
#define RST	0x04
#define SYN	0x02
#define FIN	0x01

#define EOL_KIND	0
#define NOOP_KIND	1
#define MSS_KIND	2

#define MSS_LENGTH	4
#define MSL2		10
#define MSPTICK		200
#define DEF_MSS		1024
#define DEF_RTT		1000
#define	TCPOOB_HADDATA	1
#define	TCPOOB_HAVEDATA 2

#define TCP_PASSIVE	0
#define TCP_ACTIVE	1

#define MAXBACKOFF	5
#define FORCE		1
#define	CLONE		2
#define RETRAN		4
#define ACTIVE		8
#define SYNACK		16
#define AGAIN		8
#define DGAIN		4

#define TIMER_STOP	0
#define TIMER_RUN	1
#define TIMER_EXPIRE	2

#define	set_timer(t,x)	(((t)->start) = (x)/MSPTICK)
#define	dur_timer(t)	((t)->start)
#define	read_timer(t)	((t)->count)
#define	run_timer(t)	((t)->state == TIMER_RUN)

enum {
	CLOSED = 0,
	LISTEN,
	SYN_SENT,
	SYN_RECEIVED,
	ESTABLISHED,
	FINWAIT1,
	FINWAIT2,
	CLOSE_WAIT,
	CLOSING,
	LAST_ACK,
	TIME_WAIT
	};

/*
 * Ip interface structure. We have one for each active protocol driver
 */
struct Ipifc {
	QLock;
	int 		ref;
	uchar		protocol;		/* Ip header protocol number */
	char		name[NAMELEN];		/* Protocol name */
	void (*iprcv)	(Ipconv *, Block *);	/* Receive demultiplexor */
	Ipconv		*connections;		/* Connection list */
	int		maxmtu;			/* Maximum transfer unit */
	int		minmtu;			/* Minumum tranfer unit */
	int		hsize;			/* Media header size */	
	Lock;	
};

struct Fragq {
	QLock;
	Block  *blist;
	Fragq  *next;
	Ipaddr src;
	Ipaddr dst;
	ushort id;
	};

struct Ipfrag {
	ushort	foff;
	ushort	flen;
	};

struct Arpcache {
	uchar	status;		/* Entry status */
	uchar	type;		/* Entry type */
	Ipaddr	ip;		/* Host byte order */
	uchar	eip[4];		/* Network byte order */
	uchar	et[6];		/* Ethernet address for this ip */
	int	age;		/* Entry timeout */
	Arpcache *hash;
	Arpcache **hashhd;
	Arpcache *frwd;
	Arpcache *prev;
};
#define ARP_FREE	0
#define ARP_OK		1
#define ARP_ASKED	2
#define ARP_TEMP	0
#define ARP_PERM	1
#define Arphashsize	32
#define ARPHASH(p)	arphash[((p[2]^p[3])%Arphashsize)]
#define ARP_WAITMS	2500		/* Wait for arp replys */

#define IP_VER	0x40			/* Using IP version 4 */
#define IP_HLEN 0x05			/* Header length in characters */
#define IP_DF	0x4000			/* Don't fragment */
#define IP_MF	0x2000			/* More fragments */

#define	ICMP_ECHOREPLY		0	/* Echo Reply */
#define	ICMP_UNREACH		3	/* Destination Unreachable */
#define	ICMP_SOURCEQUENCH	4	/* Source Quench */
#define	ICMP_REDIRECT		5	/* Redirect */
#define	ICMP_ECHO		8	/* Echo Request */
#define	ICMP_TIMXCEED		11	/* Time-to-live Exceeded */
#define	ICMP_PARAMPROB		12	/* Parameter Problem */
#define	ICMP_TSTAMP		13	/* Timestamp */
#define	ICMP_TSTAMPREPLY	14	/* Timestamp Reply */
#define	ICMP_IREQ		15	/* Information Request */
#define	ICMP_IREQREPLY		16	/* Information Reply */

/* Sizes */
#define IP_MAX		8192			/* Maximum Internet packet size */
#define UDP_MAX		(IP_MAX-ETHER_IPHDR)	/* Maximum UDP datagram size */
#define UDP_DATMAX	(UDP_MAX-UDP_HDRSIZE)	/* Maximum amount of udp data */

/* Protocol numbers */
#define IP_UDPPROTO	17
#define IP_TCPPROTO	6

/* Protocol port numbers */
#define PORTALLOC	5000		/* First automatic allocated port */
#define PRIVPORTALLOC	600		/* First priveleged port allocated */
#define PORTMAX		30000		/* Last port to allocte */

/* Stuff to go in funs.h someday */
Ipifc   *newipifc(uchar, void (*)(Ipconv *, Block*), Ipconv *, int, int, int, char*);
void	closeipifc(Ipifc*);
ushort	ip_csum(uchar*);
int	arp_lookup(uchar*, uchar*);
Ipaddr	ipparse(char*);
void	hnputs(uchar*, ushort);
void	hnputl(uchar*, ulong);
ulong	nhgetl(uchar*);
ushort	nhgets(uchar*);
ushort	ptcl_csum(Block*bp, int, int);
void	ppkt(Block*);
void	udprcvmsg(Ipconv *, Block*);
Block	*btrim(Block*, int, int);
Block	*ip_reassemble(int, Block*, Etherhdr*);
Ipconv	*portused(Ipconv *, Port);
Port	nextport(Ipconv *, Port);
void	arp_enter(Arpentry*, int);
void	arp_flush(void);
int	arp_delete(char*);
void	arplinkhead(Arpcache*);
Fragq   *ipfragallo(void);
void	ipfragfree(Fragq*);
void	iproute(uchar*, uchar*);
void	initfrag(int);
Block	*copyb(Block*, int);
int	ntohtcp(Tcp*, Block**);
void	reset(Ipaddr, Ipaddr, char, ushort, Tcp*);
void	proc_syn(Ipconv*, char, Tcp*);
void	send_syn(Tcpctl*);
void	tcp_output(Ipconv*);
int	seq_within(int, int, int);
void	update(Ipconv *, Tcp *);
int	trim(Tcpctl *, Tcp *, Block **, ushort *);
void	add_reseq(Tcpctl *, char, Tcp *, Block *, ushort);
void	close_self(Ipconv *, int);
int	seq_gt(int, int);
void	appendb(Block **, Block *);
Ipconv	*ip_conn(Ipconv *, Port, Port, Ipaddr dest, char proto);
void	ipmkdir(Qinfo *, Dirtab *, Ipconv *);
int	inb_window(Tcpctl *, int);
Block	*htontcp(Tcp *, Block *, Tcphdr *);
void	start_timer(Timer *);
void	stop_timer(Timer *);
int	copyupb(Block **, uchar *, int);
void	init_tcpctl(Ipconv *);
void	close_self(Ipconv *, int);
int	iss(void);
int	seq_within(int, int, int);
int	seq_lt(int, int);
int	seq_le(int, int);
int	seq_gt(int, int);
int	seq_ge(int, int);
void	setstate(Ipconv *, char);
void	tcpackproc(void*);
Block 	*htontcp(Tcp *, Block *, Tcphdr *);
int	ntohtcp(Tcp *, Block **);
void	extract_oob(Block **, Block **, Tcp *);
void	get_reseq(Tcpctl *, char *, Tcp *, Block **, ushort *);
void	state_upcall(Ipconv*, char oldstate, char newstate);
int	backoff(int);
int	dupb(Block **, Block *, int, int);
void	tcp_input(Ipconv *, Block *);
void 	tcprcvwin(Ipconv *);
void	open_tcp(Ipconv *, int, ushort, char);
void	tcpflow(void*);
void 	tcp_timeout(void *);
void	tcp_acktimer(void *);
Ipconv  *ipclonecon(Chan *);
void	iplisten(Chan *, Ipconv *, Ipconv *);

#define	fmtaddr(xx)	(xx>>24)&0xff,(xx>>16)&0xff,(xx>>8)&0xff,xx&0xff
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define BLKIP(xp)	((Etherhdr *)((xp)->rptr))
#define BLKFRAG(xp)	((Ipfrag *)((xp)->rptr))
#define PREC(x)		((x)>>5 & 7)

#define WORKBUF		64

extern Ipaddr Myip;
extern Ipaddr Mymask;
extern Ipaddr classmask[4];
extern Ipconv *ipconv[];
extern char *tcpstate[];
extern Rendez tcpflowr;
extern Qinfo tcpinfo;
extern Qinfo ipinfo;
extern Qinfo udpinfo;
