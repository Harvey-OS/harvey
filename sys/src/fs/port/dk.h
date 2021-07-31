#define	DEBUG		(cons.flags & dkflag)
#define	CEBUG		((cons.flags|up->flags) & dkflag)
#define DPRINT		if(DEBUG)print
#define CPRINT		if(CEBUG)print

enum
{
	CCCHAN		= 4,		/* the common control channel */
	SRCHAN,				/* channel for service requests */
	CKCHAN,				/* channel for call to clock */
	NDK		= 256,		/* number of channels per hsvme */
	MaxDk		= 4,		/* max number of hsvmes */
};
#define	DKTIME(n)	(n/2)
#define WSIZE		WS_512

typedef	struct	Hsdev	Hsdev;
typedef	struct	Dialin	Dialin;
typedef	struct	Dk	Dk;

/*
 * Unixp message structure
 */
struct	Dialin
{
	uchar	type;
	uchar	srv;
	uchar	param0l, param0h;
	uchar	param1l, param1h;
	uchar	param2l, param2h;
	uchar	param3l, param3h;
	uchar	param4l, param4h;
};

/*
 * Message codes to UNIX driver
 */
#define	T_SRV	1		/* request for service */
#define	T_SRV2	'k'		/* alternative request for service */
#define		D_SERV	1	/* announce a server */
#define		D_DIAL	2	/* dialout from host */

#define	T_REPLY	2		/* reply to request for service */
#define		D_OK	1	/* ? */
#define		D_OPEN	2	/* successful open */
#define		D_FAIL	3	/* NAK return */

#define	T_CHG	3		/* change in channel state */
#define		D_CLOSE	1	/* one side wishes to close a channel */
#define		D_ISCLOSED 2	/* response to UNIX close */
#define		D_CLOSEALL 3	/* sent to close all UNIX channels */

#define	T_ALIVE	4		/* keep-alive message */
#define		D_STILL	0	/* still alive */
#define		D_RESTART 1	/* just rebooted */
#define		D_XINIT	7	/* ? */

#define	T_RESTART 8		/* host rebooted, re-init channel 1 */



/*  URP window-size negotiation.
 *
 *  The 16 bits of the window-size argument are defined as:
 *
 *  -----------------------------------------------------------------
 *  |valid|   spare   |   originator   |   destination  |   traffic |
 *  |     |           |   window size  |   window size  |           |
 *  -----------------------------------------------------------------
 *     1        3              4                4              4
 *
 *  The "valid" bit equals 1 to indicate that the window size values
 *  are specified.  Zero means they should be ignored and that default
 *  values must be used instead.  Old controllers that do not have
 *  window size negotiation happen to set this bit to zero, fortunately.
 *
 *  The actual window size is computed from the 4-bit window size 
 *  parameter (n) as 2**(n+4).  Thus, window sizes range from 16 to
 *  512K bytes.
 *
 *  The originator of a call sets his "originator window size" to be
 *  less than or equal to the size of his receive buffer, and he
 *  requests through the "destination window size" the size of the
 *  buffer that he would like to use for transmission.  As the call
 *  is being set up, trunk processes and the destination line process
 *  may lower either or both window sizes.  The resulting window sizes
 *  are returned to the originator in the call ANSWER message.
 */

/* Format originator, destination, and traffic values */

#define W_WINDOW(o,d,t)	((o<<8) | (d<<4) | t | 0100000)
#define W_VALID(x)	((x) & 0100000)
#define W_ORIG(x)	(((x)>>8) & 017)
#define W_DEST(x)	(((x)>>4) & 017)
#define W_TRAF(x)	((x) & 017)
#define W_DESTMAX(x,y)	W_WINDOW(W_ORIG(x), MIN(W_DEST(x),y), W_TRAF(x))
#define W_LIMIT(x,y)	W_WINDOW(MIN(W_ORIG(x),y), MIN(W_DEST(x),y), W_TRAF(x))
#define	W_VALUE(x)	(1<<((x)+4))

enum
{
	/*
	 * window sizes
	 */
	WS_16	= 0,
	WS_32,
	WS_64,
	WS_128,
	WS_256,
	WS_512,
	WS_1K,
	WS_2K,
	WS_4K,
	WS_8K,
	WS_16K,
	WS_32K,
	WS_64K,
	WS_128K,
	WS_256K,
	WS_512K,

/*
 * control characters
 */
	CSEQ0	= 8,
	CECHO0	= 16,
	CREJ0	= 24,
	CACK0	= 32,
	CBOT	= 40,
	CBOTM	= 41,
	CBOTS	= 42,
	CENQ	= 45,
	CINITREQ= 47,		/* request initialization */
	CINIT0	= 48,
	CINIT1	= 49,
	CAINIT	= 50,

/*
 * xx
 */
	SUDAT	= 0,
	SIGNOR,
	SMARK,
	SECHO,
	SREJ,
	SACK,
	SBOT,
	SBOTM,
	SBOTS,
	SENQ,
	SINIT1,
	SSEQ,
	SUCTRL,
	SAINIT,
	SINITREQ,
/*
 * urpstate's
 */
	INITING 	= 0,	/* sent INIT1, awaiting AINIT */
	INITED,			/* received AINIT */

/*
 * common dkstate's
 */
	CLOSED		= 0,
	DIALING,
	LCLOSE,
	OPENED,

/*
 * indexes in timeout array
 */
	DKATO	= 0,
	DKBTO,
	TIMETO,
	NTIMEOUT,

/*
 * length of timeouts
 */
	DKATIME	= SECOND(2),	/* enq */
	DKBTIME	= SECOND(15),	/* alive */
	TIMENTIME= HOUR(4),	/* call for time */
	TIMEQTIME= MINUTE(1),	/* call for time until established */
};

/*
 *  hsvme datakit board
 */
struct	Hsdev
{
	ushort	version;
	ushort	pad1;
	ushort	vector;
	ushort	pad2;
	ushort	csr;
		#define ALIVE		0x0001
		#define IENABLE		0x0004
		#define EXOFLOW		0x0008
		#define IRQ		0x0010
		#define EMUTE		0x0020
		#define EPARITY		0x0040
		#define EFRAME		0x0080
		#define EROFLOW		0x0100
		#define REF		0x0800
		#define XFF		0x4000
		#define XHF		0x8000
		/* after reset */
		#define FORCEW		0x0008
		#define IPL(x)		((x)<<5)
		#define NORESET		0xFF00
		#define RESET		0x0000
	ushort	pad3;
	ushort	data;
		#define CTL		0x0100
		#define CHNO		0x0200
		#define TXEOD		0x0400
		#define NND		0x8000
	ushort	pad4;
};

struct	Dk
{
	Filter	irate;		/* aggrigate bytes */
	Filter	orate;		/* aggrigate bytes */
	Rendez	rren;		/* receiver non-empty */
	Rendez	xren;		/* xmitter half-full */
	Rendez	dkto;		/* timer */
	Queue*	reply;		/* return from file system server, gets Msgbuf* */
	Queue*	local;		/* server of dk special channels */
	Queue*	poke;		/* output to dk device, gets Chan* */
	Chan*	dkchan;		/* points to NDK channels */
	Vmedevice* vme;		/* hardware info */

	long	timeout[NTIMEOUT];
	char	boot;		/* horrible flag in xpannounce */
	char	anytime;	/* horrible flag that we know the time */
};

Dk	dkctlr[MaxDk];

void	cccinit(Dk*);
void	xpinit(Chan*);
void	urpinit(Chan*);
void	urpxinit(Chan*, int wins);
void	urprinit(Chan*);
void	dk_rintr(int u);
void	dkreply(Chan *up, int c1, int c2);
int	dk_init(int j);
void	dk_start(int c, uchar *p, int n);
void	dk_timer(Dk*);
void	dkecho(Chan *up, int cc);
void	prwindow(Chan *up, int cc);
void	dkchaninit(Chan*);

void	xpmesg(Chan*, int type, int srv, int p0, int p1);
void	xplisten(Dk*, char *cp, int n);
void	xpannounce(Chan*, char *addr);
void	xpcall(Chan*, char *dialstr, int);
void	xphup(Chan*);
void	srlisten(Dk*, char *cp, int n);
void	dkservice(Chan *up);
void	dk_timeout(void);
long	readclock(char *buf, int n);
long	readclock(char*, int);

void	dklock(Dk*, Chan*);
void	dkunlock(Dk*, Chan*);
void	dkinput(void);
void	dkoutput(void);
void	dkpoke(void);
void	dkqmesg(Msgbuf*, Chan*);
void	dklocal(void);
void	dktimer(void);
void	dkinit(Dk*);
void	dkreset(Dk*, int);
