enum {	/* NCP Packet Types */
	ConfReq=1,	ConfAck,	ConfNak,	ConfRej,
	TermReq,	TermAck,	CodeRej,	ProtocolRej,
	EchoReq,	EchoReply,	DiscardReq,
	NCP_MAXCODE
};

enum {	/* LCP Configuration Options */
	MaxRecUnit=1,
	AsyncCtlCharMap,
	AuthPrtcl,
	QualPrtcl,
	MagicNo,
	RESERVED1,
	PrtclCompression,
	AddrCtlCompression,
	MAXLCPOPTIONS
};

enum { /* IPCP Configuration Options */
	DEPRECATED1=1,
	IPCompressionPrtcl,
	IPAddress,
	MAXIPCPOPTIONS
};

enum {
	VJCompression=0x002d,
	ILCompression=0x003d,
};

enum Pstate {	/* automaton states */
	Pclosed,
	Pclosing,
	Preqsent,
	Packrcvd,
	Packsent,
	Popened,
};

enum Response {	/* responses to config requests */
	OK,	/* request is ok */
	Rej,	/* request cannot be fulfilled */
	Nak,	/* request better be changed to be oked */
};

enum {	/* A bunch of special numbers from the rfc */
	PPP_flag=0x7e,
	PPP_addr=0xff,
	PPP_ctl=0x3,

	PRTCL_IPCP=0x8021,		/* ip control protocol */
	PRTCL_LCP=0xc021,		/* link control protocol */
	PRTCL_IP=0x0021,		/* regular ip and above */
	PRTCL_TCPCOMPRSSD=0x002d,	/* compressed tcp/ip */
	PRTCL_TCPUNCOMPRSSD=0x002f,	/* uncompressed tcp/ip */
	PRTCL_ILCOMPRSSD=0x003d,	/* compressed il */
	PRTCL_ILUNCOMPRSSD=0x003f,	/* uncompressed il */

	HDLCesc=0x7d,
	PPP_initfcs=0xffff,
	PPP_goodfcs=0xf0b8,

	ETHER_HDR=14,		/* amount of prepended bytes */
	TCPIP_HDR=128,
	Maxip=10000,		/* Max packet size BUG BUG: 64K */

	PPP_HDR=5,
	PPP_FLAGSZ=1,
	PPP_DATASZ=1500,
	PPP_FCSSZ=2,
	LCP_HDR=4,
};

aggr PPPpkt {
	byte	flag;
	byte	addr;
	byte	ctl;
	byte	protocol[2];
	byte	info[PPP_DATASZ];
	byte	fcs[PPP_FCSSZ];
};

aggr Ctlpkt {			/* NCP Control Packet */
	byte	code;
	byte	ident;
	byte	length[2];
	byte	rawdata[PPP_DATASZ - LCP_HDR];
};

aggr Cfrinfo {		/* config requests */
	byte cfrtype;
	byte cfrlength;
	byte data[PPP_DATASZ - LCP_HDR - 2];
};

aggr Echoinfo {		/* echo packets */
	byte	magicno[4];
	byte	echodata[PPP_DATASZ - LCP_HDR - 4];
};

aggr Prtclrej {		/* protocol rejects */
	byte	rejprtcl[2];
	byte	rejinfo[PPP_DATASZ - LCP_HDR - 2];
};

aggr Vjcompressreq {		/* VJ TCP/IP Compress Option */
	byte	prtcl[2];
	byte	maxslots;
	byte	compslots;
};

typedef	adt PPPstate;
typedef aggr Tcpcomp;
typedef aggr Ilcomp;

adt Ncp {
	usint	prtcl;
extern	Pstate	state;
	Lock;
extern	uint	options;	/* mask of lcp options that are on */
	byte	ident;		/* counter for identifiers */
	byte	lastident;	/* id of last packet sent */

	union {
		aggr {	/* IPCP */
			uint	coptions;
			byte	myipaddr[4];
			byte	peeripaddr[4];
		};
		aggr {	/* LCP */
			usint	maxsize;
			usint	peermaxsize;
			uint	asyncctlmap;
			uint	magicno;
			uint	peermagicno;
		};
	};

	int	parsecfn(*Ncp, byte*, uint);
	void	ncpparse(*Ncp, PPPstate*, Ctlpkt*, uint);
	void	ncptimeout(*Ncp, PPPstate*);
	void	ncpsenddata(*Ncp, PPPstate*, byte, void*, usint, byte);
	void	lcpinit(*Ncp);
	void	ipcpinit(*Ncp, byte*, byte*);
intern	int	lcpmakecfr(*Ncp, byte*);
intern	int	ipcpmakecfr(*Ncp, byte*);
	void	sendcfr(*Ncp, PPPstate*);
intern	Response	lcpparsecfr(*Ncp, Cfrinfo*);
intern	Response	ipcpparsecfr(*Ncp, Cfrinfo*);
	int	parsecfr(*Ncp, PPPstate*, uint, byte*, uint);
};

adt PPPstate {
	sint	pppfd;

extern	Ncp	lcp;
extern	Ncp	ipcp;
extern	uint	allset;		/* true when the line is set up */
	uint	cksumerr;	/* count of checksum errors */
extern	Tcpcomp	*tcpcomp;		/* tcp compression state */
extern	Ilcomp	*ilcomp;

	void	initppp(*PPPstate, int, byte*, byte*);
	void	killppp(*PPPstate, byte*);
	void	pppdecode(*PPPstate);
	void	pppencode(*PPPstate, usint, byte*, int, int);
	void	slowdown(*PPPstate);
	void	starttimer(*PPPstate);
	void	resettimer(*PPPstate);
intern	void	pppgotpacket(*PPPstate, PPPpkt*, int);
intern	int	hdlccpy(*PPPstate, void*, void*, int);
intern	int	hdlcprune(*PPPstate, void*, int);
};

/*
 * ppp.l
 */
void	ipconfig(byte*, int);
void	timer(void*, byte*);
void	placeonq(PPPstate *s);
void	doalarms(void);

/*
 * fcs.l
 */
usint	calcfcs(usint fcs, byte *cp, uint len);
usint	ip_csum(byte *addr);
void	fcsputs(byte *ptr, usint val);
void	hnputs(byte *ptr, usint val);
void	hnputl(byte *ptr, uint val);
uint	nhgetl(byte *ptr);
usint	nhgets(byte *ptr);

#define nelem(a)	(sizeof(a)/sizeof(a[0]))
#define DPRINT		if(debug)print
extern int	debug;
extern int	ipfd;
