typedef struct Tcpc Tcpc;
typedef struct Pstate Pstate;
typedef struct Chap Chap;
typedef struct Qualstats Qualstats;
typedef struct Comptype Comptype;
typedef struct Uncomptype Uncomptype;
typedef struct PPP PPP;
typedef struct Lcpmsg Lcpmsg;
typedef struct Lcpopt Lcpopt;
typedef struct Qualpkt Qualpkt;
typedef struct Block Block;

typedef uchar Ipaddr[IPaddrlen];	

#pragma incomplete Tcpc

/*
 *  data blocks
 */
struct Block
{
	Block	*next;
	Block	*flist;
	Block	*list;			/* chain of block lists */
	uchar	*rptr;			/* first unconsumed uchar */
	uchar	*wptr;			/* first empty uchar */
	uchar	*lim;			/* 1 past the end of the buffer */
	uchar	*base;			/* start of the buffer */
	uchar	flags;
	void	*flow;
	ulong	pc;
	ulong	bsz;
};
#define BLEN(b)	((b)->wptr-(b)->rptr)

enum
{
	/* block flags */
	S_DELIM 	= (1<<0),
	S_HANGUP	= (1<<1),
	S_RHANGUP	= (1<<2),

	/* queue states */
	QHUNGUP		= (1<<0),
	QFLOW		= (1<<1),	/* queue is flow controlled */
};

Block*	allocb(int);
void	freeb(Block*);
Block*	concat(Block*);
int	blen(Block*);
Block*	pullup(Block*, int);
Block*	padb(Block*, int);
Block*	btrim(Block*, int, int);
Block*	copyb(Block*, int);
int	pullb(Block**, int);

enum {
	HDLC_frame=	0x7e,
	HDLC_esc=	0x7d,

	/* PPP frame fields */
	PPP_addr=	0xff,
	PPP_ctl=	0x3,
	PPP_initfcs=	0xffff,
	PPP_goodfcs=	0xf0b8,

	/* PPP phases */
	Pdead=		0,	
	Plink,				/* doing LCP */
	Pauth,				/* doing chap */
	Pnet,				/* doing IPCP, CCP */
	Pterm,				/* closing down */

	/* PPP protocol types */
	Pip=		0x21,		/* ip v4 */
	Pipv6=		0x57,		/* ip v6 */
	Pvjctcp=	0x2d,		/* compressing van jacobson tcp */
	Pvjutcp=	0x2f,		/* uncompressing van jacobson tcp */
	Pcdata=		0xfd,		/* compressed datagram */
	Pipcp=		0x8021,		/* ip control */
	Pecp=		0x8053,		/* encryption control */
	Pccp=		0x80fd,		/* compressed datagram control */
	Plcp=		0xc021,		/* link control */
	Ppasswd=	0xc023,		/* passwd authentication */
	Plqm=		0xc025,		/* link quality monitoring */
	Pchap=		0xc223,		/* challenge/response */

	/* LCP codes */
	Lconfreq=	1,
	Lconfack=	2,
	Lconfnak=	3,
	Lconfrej=	4,
	Ltermreq=	5,
	Ltermack=	6,
	Lcoderej=	7,
	Lprotorej=	8,
	Lechoreq=	9,
	Lechoack=	10,
	Ldiscard=	11,
	Lresetreq=	14,
	Lresetack=	15,

	/* Lcp configure options */
	Omtu=		1,
	Octlmap=	2,
	Oauth=		3,
	Oquality=	4,
	Omagic=		5,
	Opc=		7,
	Oac=		8,

	/* authentication protocols */
	APmd5=		5,
	APmschap=	128,
	APpasswd=	Ppasswd,		/* use Pap, not Chap */

	/* lcp flags */
	Fmtu=		1<<Omtu,
	Fctlmap=	1<<Octlmap,
	Fauth=		1<<Oauth,
	Fquality=	1<<Oquality,
	Fmagic=		1<<Omagic,
	Fpc=		1<<Opc,
	Fac=		1<<Oac,

	/* Chap codes */
	Cchallenge=	1,
	Cresponse=	2,
	Csuccess=	3,
	Cfailure=	4,

	/* Pap codes */
	Pauthreq=	1,
	Pauthack=	2,
	Pauthnak=	3,

	/* Chap state */
	Cunauth=	0,
	Cchalsent,
	Cauthfail,
	Cauthok,

	/* link states */
	Sclosed=	0,
	Sclosing,
	Sreqsent,
	Sackrcvd,
	Sacksent,
	Sopened,

	/* ccp configure options */
	Ocoui=		0,	/* proprietary compression */
	Ocstac=		17,	/* stac electronics LZS */
	Ocmppc=		18,	/* microsoft ppc */
	Octhwack=	31,	/* thwack; unofficial */

	/* ccp flags */
	Fcoui=		1<<Ocoui,
	Fcstac=		1<<Ocstac,
	Fcmppc=		1<<Ocmppc,
	Fcthwack=	1<<Octhwack,

	/* ecp configure options */
	Oeoui=		0,	/* proprietary compression */
	Oedese=		1,	/* DES */

	/* ecp flags */
	Feoui=		1<<Oeoui,
	Fedese=		1<<Oedese,

	/* ipcp configure options */
	Oipaddrs=	1,
	Oipcompress=	2,
	Oipaddr=	3,
	Oipdns=		129,
	Oipwins=	130,
	Oipdns2=	131,
	Oipwins2=	132,

	/* ipcp flags */
	Fipaddrs=	1<<Oipaddrs,
	Fipcompress=	1<<Oipcompress,
	Fipaddr=	1<<Oipaddr,
	Fipdns=		1<<8, 	// Oipdns,
	Fipwins=	1<<9,	// Oipwins,
	Fipdns2=	1<<10,	// Oipdns2,
	Fipwins2=	1<<11,	// Oipwins2,

	Period=		5*1000,	/* period of retransmit process (in ms) */
	Timeout=	20,	/* xmit timeout (in Periods) */
	Buflen=		4096,

	MAX_STATES=	16,		/* van jacobson compression states */
	Defmtu=		1450,		/* default that we will ask for */
	Minmtu=		128,		/* minimum that we will accept */
	Maxmtu=		2000,		/* maximum that we will accept */
};


struct Pstate
{
	int	proto;		/* protocol type */
	int	timeout;	/* for current state */
	int	rxtimeout;	/* for current retransmit */
	ulong	flags;		/* options received */
	uchar	id;		/* id of current message */
	uchar	confid;		/* id of current config message */
	uchar	termid;		/* id of current termination message */
	uchar	rcvdconfid;	/* id of last conf message received */
	uchar	state;		/* PPP link state */
	ulong	optmask;	/* which options to request */
	int	echoack;	/* recieved echo ack */
	int	echotimeout;	/* echo timeout */
};

/* server chap state */
struct Chap
{
	int	proto;		/* chap proto */
	int	state;		/* chap state */
	uchar	id;		/* id of current message */
	int	timeout;	/* for current state */
	Chalstate *cs;
};

struct Qualstats
{
	ulong	reports;
	ulong	packets;
	ulong	uchars;
	ulong	discards;
	ulong	errors;
};

struct Comptype
{
	void*		(*init)(PPP*);
	Block*		(*compress)(PPP*, ushort, Block*, int*);
	Block*		(*resetreq)(void*, Block*);
	void		(*fini)(void*);
};

struct Uncomptype
{
	void*		(*init)(PPP*);
	Block*		(*uncompress)(PPP*, Block*, int*, Block**);
	void		(*resetack)(void*, Block*);
	void		(*fini)(void*);
};

struct PPP
{
	QLock;

	int		ipfd;		/* fd to ip stack */
	int		ipcfd;		/* fd to control channel of ip stack */
	int		mediain;	/* fd to media */
	int		mediaout;	/* fd to media */
	char		*net;		/* ip stack to use */
	int		framing;	/* non-zero to use framing characters */
	Ipaddr		local;
	Ipaddr		curlocal;
	int		localfrozen;
	Ipaddr		remote;
	Ipaddr		curremote;
	int		remotefrozen;

	Ipaddr		dns[2];		/* dns servers */
	Ipaddr		wins[2];	/* wins servers */

	Block*		inbuf;		/* input buffer */
	Block*		outbuf;		/* output buffer */
	QLock		outlock;	/*  and its lock */
	ulong		magic;		/* magic number to detect loop backs */
	ulong		rctlmap;	/* map of chars to ignore in rcvr */
	ulong		xctlmap;	/* map of chars to excape in xmit */
	int		phase;		/* PPP phase */
	Pstate*		lcp;		/* lcp state */
	Pstate*		ccp;		/* ccp state */
	Pstate*		ipcp;		/* ipcp state */
	Chap*		chap;		/* chap state */
	Tcpc*		ctcp;		/* tcp compression state */
	ulong		mtu;		/* maximum xmit size */
	ulong		mru;		/* maximum recv size */

	/* data compression */
	int		ctries;		/* number of negotiation tries */
	Comptype	*ctype;		/* compression virtual table */
	void		*cstate;	/* compression state */
	Uncomptype	*unctype;	/* uncompression virtual table */
	void		*uncstate;	/* uncompression state */
	
	/* encryption key */
	uchar		key[16];
	int		sendencrypted;

	/* authentication */
	char		secret[256];	/* md5 key */
	char		chapname[256];	/* chap system name */

	/* link quality monitoring */
	int		period;	/* lqm period */
	int		timeout; /* time to next lqm packet */
	Qualstats	in;	/* local */
	Qualstats	out;
	Qualstats	pin;	/* peer */
	Qualstats	pout;
	Qualstats	sin;	/* saved */

	struct {
		ulong	ipsend;
		ulong	iprecv;
		ulong	iprecvbadsrc;
		ulong	iprecvnotup;
		ulong	comp;
		ulong	compin;
		ulong	compout;
		ulong	compreset;
		ulong	uncomp;
		ulong	uncompin;
		ulong	uncompout;
		ulong	uncompreset;
		ulong	vjin;
		ulong	vjout;
		ulong	vjfail;
	} stat;
};

extern Block*	pppread(PPP*);
extern int	pppwrite(PPP*, Block*);
extern void	pppopen(PPP*, int, int, char*, Ipaddr, Ipaddr, int, int);

struct Lcpmsg
{
	uchar	code;
	uchar	id;
	uchar	len[2];
	uchar	data[1];
};

struct Lcpopt
{
	uchar	type;
	uchar	len;
	uchar	data[1];
};

struct Qualpkt
{
	uchar	magic[4];

	uchar	lastoutreports[4];
	uchar	lastoutpackets[4];
	uchar	lastoutuchars[4];
	uchar	peerinreports[4];
	uchar	peerinpackets[4];
	uchar	peerindiscards[4];
	uchar	peerinerrors[4];
	uchar	peerinuchars[4];
	uchar	peeroutreports[4];
	uchar	peeroutpackets[4];
	uchar	peeroutuchars[4];
};

extern Block*	compress(Tcpc*, Block*, int*);
extern void	compress_error(Tcpc*);
extern Tcpc*	compress_init(Tcpc*);
extern int	compress_negotiate(Tcpc*, uchar*);
extern Block*	tcpcompress(Tcpc*, Block*, int*);
extern Block*	tcpuncompress(Tcpc*, Block*, int);
extern Block*	alloclcp(int, int, int, Lcpmsg**);
extern ushort	ptclcsum(Block*, int, int);
extern ushort	ptclbsum(uchar*, int);
extern ushort	ipcsum(uchar*);

extern	Comptype	cmppc;
extern	Uncomptype	uncmppc;

extern	Comptype	cthwack;
extern	Uncomptype	uncthwack;

extern void	netlog(char*, ...);
#pragma	varargck	argpos	netlog	1

extern char	*LOG;
