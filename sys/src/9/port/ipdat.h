typedef struct Etherhdr	Etherhdr;
typedef struct Fragq	Fragq;
typedef struct Ilcb	Ilcb;
typedef struct Ilhdr	Ilhdr;
typedef ulong		Ipaddr;
typedef struct Ipconv	Ipconv;
typedef struct Ipfrag	Ipfrag;
typedef struct Ipifc	Ipifc;
typedef ushort		Port;
typedef struct Reseq	Reseq;
typedef struct Tcp	Tcp;
typedef struct Tcpctl	Tcpctl;
typedef struct Tcphdr	Tcphdr;
typedef struct Timer	Timer;
typedef struct Udphdr	Udphdr;

struct Etherhdr
{
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

struct Udphdr
{
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

struct Ilhdr
{
#define IL_EHSIZE	34
	uchar	d[6];		/* Ethernet destination */
	uchar	s[6];		/* Ethernet source */
	uchar	type[2];	/* Ethernet packet type */

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
#define IL_HDRSIZE	18	
	uchar	ilsum[2];	/* Checksum including header */
	uchar	illen[2];	/* Packet length */
	uchar	iltype;		/* Packet type */
	uchar	ilspec;		/* Special */
	uchar	ilsrc[2];	/* Src port */
	uchar	ildst[2];	/* Dst port */
	uchar	ilid[4];	/* Sequence id */
	uchar	ilack[4];	/* Acked sequence */
};

struct Ilcb			/* Control block */
{
	int	state;		/* Connection state */

	Rendez	syncer;		/* where syncer waits for a connect */

	QLock	ackq;		/* Unacknowledged queue */
	Block	*unacked;
	Block	*unackedtail;

	QLock	outo;		/* Out of order packet queue */
	Block	*outoforder;

	Lock	nxl;
	ulong	next;		/* Id of next to send */
	ulong	recvd;		/* Last packet received */
	ulong	start;		/* Local start id */
	ulong	rstart;		/* Remote start id */

	int	timeout;	/* Time out counter */
	int	slowtime;	/* Slow time counter */
	int	fasttime;	/* Retransmission timer */
	int	acktime;	/* Acknowledge timer */
	int	querytime;	/* Query timer */
	int	deathtime;	/* Time to kill connection */

	int	rtt;		/* Average round trip time */
	ulong	rttack;		/* The ack we are waiting for */
	ulong	ackms;		/* Time we issued */

	int	window;		/* Maximum receive window */
};

enum				/* Packet types */
{
	Ilsync,
	Ildata,
	Ildataquery,
	Ilack,
	Ilquerey,
	Ilstate,
	Ilclose,
};

enum				/* Connection state */
{
	Ilclosed,
	Ilsyncer,
	Ilsyncee,
	Ilestablished,
	Illistening,
	Ilclosing,
};

#define TCP_PKT	(TCP_EHSIZE+TCP_IPLEN+TCP_PHDRSIZE)

struct Tcphdr
{
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

enum
{
	TimerOFF	= 0,
	TimerON		= 1,
	TimerDONE	= 2,
};

struct Timer
{
	Timer	*next;
	Timer	*prev;
	int	state;
	int	start;
	int	count;
	void	(*func)(void*);
	void	*arg;
};

struct Tctl
{
	uchar	state;			/* Connection state */
	uchar	type;			/* Listening or active connection */
	uchar	code;			/* Icmp code */		
	struct {
		int	una;		/* Unacked data pointer */
		int	nxt;		/* Next sequence expected */
		int	ptr;		/* Data pointer */
		ushort	wnd;		/* Tcp send window */
		int	up;		/* Urgent data pointer */
		int	wl1;
		int	wl2;
	} snd;
	struct {
		int	nxt;		/* Receive pointer to next byte slot */
		ushort	wnd;		/* Receive window incoming */
		int	up;		/* Urgent pointer */
	} rcv;
	int	iss;			/* Initial sequence number */
	ushort	cwind;			/* Congestion window */
	ushort	ssthresh;		/* Slow start threshold */
	int	resent;			/* Bytes just resent */
	int	irs;			/* Initial received squence */
	ushort	mss;			/* Mean segment size */
	int	rerecv;			/* Overlap of data rerecevived */
	ushort	window;			/* Recevive window */
	int	max_snd;		/* Max send */
	int	last_ack;		/* Last acknowledege received */
	char	backoff;		/* Exponential backoff counter */
	char	flags;			/* State flags */
	char	tos;			/* Type of service */

	Blist	rcvq;			/* Received data */
	ulong	rcvcnt;			/* Bytes queued for upstream */

	Block	*sndq;			/* List of data going out */
	ulong	sndcnt;			/* Amount of data in send queue */
	Rendez	sndr;			/* process flow control */
	QLock	sndrlock;
	int	sndfull;

	Reseq	*reseq;			/* Resequencing queue */
	Timer	timer;			/* Activity timer */
	Timer	acktimer;		/* Acknoledge timer */
	Timer	rtt_timer;		/* Round trip timer */
	int	rttseq;			/* Round trip sequence */
	int	srtt;			/* Shortened round trip */
	int	mdev;			/* Mean deviation of round trip */
};

struct Tcpctl
{
	QLock;
	Rendez syner;
	struct Tctl;
};

struct	Tcp
{
	Port	source;
	Port	dest;
	int	seq;
	int	ack;
	char	flags;
	ushort	wnd;
	ushort	up;
	ushort	mss;
};

struct Reseq
{
	Reseq 	*next;
	Tcp	seg;
	Block	*bp;
	ushort	length;
	char	tos;
};

/* An ip interface used for UDP/TCP/IL */
struct Ipconv
{
	QLock;				/* Ref count lock */
	Netprot;			/* stat info */
	int 	ref;
	Ipaddr	dst;			/* Destination from connect */
	Port	psrc;			/* Source port */
	Port	pdst;			/* Destination port */

	Ipifc	*ifc;			/* Ip protocol interface */
	Queue	*readq;			/* Pointer to upstream read q */
	QLock	listenq;		/* List of people waiting incoming cons */
	Rendez	listenr;		/* Some where to sleep while waiting */
		
	char	*err;			/* Async protocol error */
	int	backlog;		/* Maximum number of waiting connections */
	int	headers;		/* include header in packet */
	int	curlog;			/* Number of waiting connections */
	Ipconv 	*newcon;		/* This is the start of a connection */

	union {
		Tcpctl	tcpctl;		/* Tcp control block */
		Ilcb	ilctl;		/* Il control block */
	};
};

enum
{
	MAX_TIME 	= (1<<20),	/* Forever */
	TCP_ACK		= 200,		/* Timed ack sequence every 200ms */

	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Aknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */

	EOL_KIND	= 0,
	NOOP_KIND	= 1,
	MSS_KIND	= 2,

	MSS_LENGTH	= 4,		/* Mean segment size */
	MSL2		= 10,
	MSPTICK		= 100,		/* Milliseconds per timer tick */
	DEF_MSS		= 1024,		/* Default mean segment */
	DEF_RTT		= 1000,		/* Default round trip */

	TCP_PASSIVE	= 0,		/* Listen connection */
	TCP_ACTIVE	= 1,		/* Outgoing connection */
	IL_PASSIVE	= 0,
	IL_ACTIVE	= 1,

	MAXBACKOFF	= 5,
	FORCE		= 1,
	CLONE		= 2,
	RETRAN		= 4,
	ACTIVE		= 8,
	SYNACK		= 16,
	AGAIN		= 8,
	DGAIN		= 4,
};

#define	set_timer(t,x)	(((t)->start) = (x)/MSPTICK)
#define	run_timer(t)	((t)->state == TimerON)

enum					/* Tcp connection states */
{
	Closed		= 0,
	Listen,
	Syn_sent,
	Syn_received,
	Established,
	Finwait1,
	Finwait2,
	Close_wait,
	Closing,
	Last_ack,
	Time_wait
};

enum
{
	Nipconv=	512,		/* max conversations per interface */
	Udphdrsize=	6,		/* size if a to/from user Udp header */
};

/*
 * Ip interface structure. We have one for each active protocol driver
 */
struct Ipifc 
{
	QLock;
	Network;				/* user level network interface */
	Ipifc		*next;
	int 		inited;
	uchar		protocol;		/* Ip header protocol number */
	void (*iprcv)	(Ipifc*, Block*);	/* Receive demultiplexor */
	int		maxmtu;			/* Maximum transfer unit */
	int		minmtu;			/* Minumum tranfer unit */
	int		hsize;			/* Media header size */	
	ulong		chkerrs;		/* checksum errors */
	Ipconv		**conv;			/* conversations */
};


struct Fragq
{
	QLock;
	Block  *blist;
	Fragq  *next;
	Ipaddr src;
	Ipaddr dst;
	ushort id;
	ulong  age;
};

struct Ipfrag
{
	ushort	foff;
	ushort	flen;
};

enum {
	IP_VER		= 0x40,			/* Using IP version 4 */
	IP_HLEN		= 0x05,			/* Header length in characters */
	IP_DF		= 0x4000,		/* Don't fragment */
	IP_MF		= 0x2000,		/* More fragments */

	/* Sizes */
	IP_MAX		= (32*1024),		/* Maximum Internet packet size */
	UDP_MAX		= (IP_MAX-ETHER_IPHDR),	/* Maximum UDP datagram size */
	UDP_DATMAX	= (UDP_MAX-UDP_HDRSIZE),/* Maximum amount of udp data */
	IL_DATMAX	= (IP_MAX-IL_HDRSIZE),	/* Maximum IL data in one ip packet */

	/* Protocol numbers */
	IP_UDPPROTO	= 17,
	IP_TCPPROTO	= 6,
	IP_ILPROTO	= 40,

	/* Protocol port numbers */
	PORTALLOC	= 5000,			/* First automatic allocated port */
	PRIVPORTALLOC	= 600,			/* First priveleged port allocated */
	UNPRIVPORTALLOC	= 1024,			/* First unpriveleged port allocated */
	PORTMAX		= 30000,		/* Last port to allocte */
};

void	add_reseq(Tcpctl *, char, Tcp *, Block *, ushort);
int	arp_lookup(uchar*, uchar*);
int	backoff(int);
Block*	btrim(Block*, int, int);
void	localclose(Ipconv *, char []);
int	dupb(Block **, Block *, int, int);
void	extract_oob(Block **, Block **, Tcp *);
void	get_reseq(Tcpctl *, char *, Tcp *, Block **, ushort *);
void	hnputl(uchar*, ulong);
void	hnputs(uchar*, ushort);
Block*	htontcp(Tcp *, Block *, Tcphdr *);
Block*	htontcp(Tcp *, Block *, Tcphdr *);
void	iloutoforder(Ipconv*, Ilhdr*, Block*);
void	ilstart(Ipconv *, int, int);
int	inb_window(Tcpctl *, int);
void	init_tcpctl(Ipconv *);
void	initfrag(int);
void	initipifc(Ipifc*, uchar, void (*)(Ipifc*, Block*), int, int, int);
Ipconv*	ip_conn(Ipifc*, Port, Port, Ipaddr dest);
ushort	ip_csum(uchar*);
Block*	ip_reassemble(int, Block*, Etherhdr*);
int	ipclonecon(Chan *);
int	ipconbusy(Ipconv*);
Ipconv*	ipcreateconv(Ipifc*, int);
int	ipforme(uchar*);
Fragq*	ipfragallo(void);
void	ipfragfree(Fragq*, int);
Ipconv*	ipincoming(Ipifc*, Ipconv*);
int	iplisten(Chan *);
void	iplocalfill(Chan*, char*, int);
void	ipmkdir(Qinfo *, Dirtab *, Ipconv *);
Ipaddr	ipparse(char*);
void	ipremotefill(Chan*, char*, int);
void	iproute(uchar*, uchar*);
void	ipsetaddrs(void);
void	ipstatusfill(Chan*, char*, int);
Port	nextport(Ipifc*, int);
ushort	nhgets(uchar*);
ulong	nhgetl(uchar*);
int	ntohtcp(Tcp *, Block **);
int	ntohtcp(Tcp*, Block**);
Ipconv*	portused(Ipifc*, Port);
void	ppkt(Block*);
void	proc_syn(Ipconv*, char, Tcp*);
ushort	ptcl_csum(Block*bp, int, int);
int	pullb(Block **, int);
void	reset(Ipaddr, Ipaddr, char, ushort, Tcp*);
void	tcpsndsyn(Tcpctl*);
int	seq_ge(int, int);
int	seq_gt(int, int);
int	seq_gt(int, int);
int	seq_le(int, int);
int	seq_lt(int, int);
int	seq_within(int, int, int);
int	seq_within(int, int, int);
void	tcpsetstate(Ipconv *, char);
void	tcpgo(Timer *);
void	tcphalt(Timer *);
void	tcpxstate(Ipconv*, char oldstate, char newstate);
void	tcpacktimer(void *);
void	tcpinput(Ipifc*, Block *);
void	tcpoutput(Ipconv*);
void	tcptimeout(void *);
void	tcpackproc(void*);
void	tcpflow(void*);
void	tcpflushincoming(Ipconv*);
void	tcprcvwin(Ipconv *);
void	tcpstart(Ipconv *, int, ushort, char);
int	trim(Tcpctl *, Tcp *, Block **, ushort *);
void	udprcvmsg(Ipifc*, Block*);
void	update(Ipconv *, Tcp *);

#define	fmtaddr(xx)	(xx>>24)&0xff,(xx>>16)&0xff,(xx>>8)&0xff,xx&0xff
#define	MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define BLKIP(xp)	((Etherhdr *)((xp)->rptr))
#define BLKFRAG(xp)	((Ipfrag *)((xp)->base))
#define PREC(x)		((x)>>5 & 7)

extern Ipaddr Myip[7];
extern Ipaddr Mymask;
extern Ipaddr Mynetmask;
extern Ipaddr classmask[4];
extern Ipifc *ipifc[];
extern char *tcpstate[];
extern char *ilstate[];
extern Rendez tcpflowr;
extern Qinfo tcpinfo;
extern Qinfo ipinfo;
extern Qinfo udpinfo;
extern Qinfo ilinfo;
extern Qinfo arpinfo;
extern Queue *Ipoutput;

/* offsets into Myip */
enum
{
	Myself		= 0,
	Mybcast		= 1,
	Mynet		= 3,
	Mysubnet	= 5,
};
