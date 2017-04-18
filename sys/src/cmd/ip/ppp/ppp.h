/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

typedef uint8_t Ipaddr[IPaddrlen];	

#pragma incomplete Tcpc

/*
 *  data blocks
 */
struct Block
{
	Block	*next;
	Block	*flist;
	Block	*list;			/* chain of block lists */
	uint8_t	*rptr;			/* first unconsumed uint8_t */
	uint8_t	*wptr;			/* first empty uint8_t */
	uint8_t	*lim;			/* 1 past the end of the buffer */
	uint8_t	*base;			/* start of the buffer */
	uint8_t	flags;
	void	*flow;
	uint32_t	pc;
	uint32_t	bsz;
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
	uint32_t	flags;		/* options received */
	uint8_t	id;		/* id of current message */
	uint8_t	confid;		/* id of current config message */
	uint8_t	termid;		/* id of current termination message */
	uint8_t	rcvdconfid;	/* id of last conf message received */
	uint8_t	state;		/* PPP link state */
	uint32_t	optmask;	/* which options to request */
	int	echoack;	/* recieved echo ack */
	int	echotimeout;	/* echo timeout */
};

/* server chap state */
struct Chap
{
	int	proto;		/* chap proto */
	int	state;		/* chap state */
	uint8_t	id;		/* id of current message */
	int	timeout;	/* for current state */
	Chalstate *cs;
};

struct Qualstats
{
	uint32_t	reports;
	uint32_t	packets;
	uint32_t	uint8_ts;
	uint32_t	discards;
	uint32_t	errors;
};

struct Comptype
{
	void*		(*init)(PPP*);
	Block*		(*compress)(PPP*, uint16_t, Block*, int*);
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
	uint32_t		magic;		/* magic number to detect loop backs */
	uint32_t		rctlmap;	/* map of chars to ignore in rcvr */
	uint32_t		xctlmap;	/* map of chars to excape in xmit */
	int		phase;		/* PPP phase */
	Pstate*		lcp;		/* lcp state */
	Pstate*		ccp;		/* ccp state */
	Pstate*		ipcp;		/* ipcp state */
	Chap*		chap;		/* chap state */
	Tcpc*		ctcp;		/* tcp compression state */
	uint32_t		mtu;		/* maximum xmit size */
	uint32_t		mru;		/* maximum recv size */

	/* data compression */
	int		ctries;		/* number of negotiation tries */
	Comptype	*ctype;		/* compression virtual table */
	void		*cstate;	/* compression state */
	Uncomptype	*unctype;	/* uncompression virtual table */
	void		*uncstate;	/* uncompression state */
	
	/* encryption key */
	uint8_t		key[16];
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
		uint32_t	ipsend;
		uint32_t	iprecv;
		uint32_t	iprecvbadsrc;
		uint32_t	iprecvnotup;
		uint32_t	comp;
		uint32_t	compin;
		uint32_t	compout;
		uint32_t	compreset;
		uint32_t	uncomp;
		uint32_t	uncompin;
		uint32_t	uncompout;
		uint32_t	uncompreset;
		uint32_t	vjin;
		uint32_t	vjout;
		uint32_t	vjfail;
	} stat;
};

extern Block*	pppread(PPP*);
extern int	pppwrite(PPP*, Block*);
extern void	pppopen(PPP*, int, int, char*, Ipaddr, Ipaddr, int, int);

struct Lcpmsg
{
	uint8_t	code;
	uint8_t	id;
	uint8_t	len[2];
	uint8_t	data[1];
};

struct Lcpopt
{
	uint8_t	type;
	uint8_t	len;
	uint8_t	data[1];
};

struct Qualpkt
{
	uint8_t	magic[4];

	uint8_t	lastoutreports[4];
	uint8_t	lastoutpackets[4];
	uint8_t	lastoutuint8_ts[4];
	uint8_t	peerinreports[4];
	uint8_t	peerinpackets[4];
	uint8_t	peerindiscards[4];
	uint8_t	peerinerrors[4];
	uint8_t	peerinuint8_ts[4];
	uint8_t	peeroutreports[4];
	uint8_t	peeroutpackets[4];
	uint8_t	peeroutuint8_ts[4];
};

extern Block*	compress(Tcpc*, Block*, int*);
extern void	compress_error(Tcpc*);
extern Tcpc*	compress_init(Tcpc*);
extern int	compress_negotiate(Tcpc*, uint8_t*);
extern Block*	tcpcompress(Tcpc*, Block*, int*);
extern Block*	tcpuncompress(Tcpc*, Block*, int);
extern Block*	alloclcp(int, int, int, Lcpmsg**);
extern uint16_t	ptclcsum(Block*, int, int);
extern uint16_t	ptclbsum(uint8_t*, int);
extern uint16_t	ipcsum(uint8_t*);

extern	Comptype	cmppc;
extern	Uncomptype	uncmppc;

extern	Comptype	cthwack;
extern	Uncomptype	uncthwack;

extern void	netlog(char*, ...);
#pragma	varargck	argpos	netlog	1

extern char	*LOG;
