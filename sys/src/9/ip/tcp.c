#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"ip.h"

enum
{
	QMAX		= 64*1024-1,
	IP_TCPPROTO	= 6,

	TCP4_IPLEN	= 8,
	TCP4_PHDRSIZE	= 12,
	TCP4_HDRSIZE	= 20,
	TCP4_TCBPHDRSZ	= 40,
	TCP4_PKT	= TCP4_IPLEN+TCP4_PHDRSIZE,

	TCP6_IPLEN	= 0,
	TCP6_PHDRSIZE	= 40,
	TCP6_HDRSIZE	= 20,
	TCP6_TCBPHDRSZ	= 60,
	TCP6_PKT	= TCP6_IPLEN+TCP6_PHDRSIZE,

	TcptimerOFF	= 0,
	TcptimerON	= 1,
	TcptimerDONE	= 2,
	MAX_TIME 	= (1<<20),	/* Forever */
	TCP_ACK		= 50,		/* Timed ack sequence in ms */
	MAXBACKMS	= 9*60*1000,	/* longest backoff time (ms) before hangup */

	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Acknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */

	EOLOPT		= 0,
	NOOPOPT		= 1,
	MSSOPT		= 2,
	MSS_LENGTH	= 4,		/* Maximum segment size */
	WSOPT		= 3,
	WS_LENGTH	= 3,		/* Bits to scale window size by */
	MSL2		= 10,
	MSPTICK		= 50,		/* Milliseconds per timer tick */
	DEF_MSS		= 1460,		/* Default maximum segment */
	DEF_MSS6	= 1280,		/* Default maximum segment (min) for v6 */
	DEF_RTT		= 500,		/* Default round trip */
	DEF_KAT		= 120000,	/* Default time (ms) between keep alives */
	TCP_LISTEN	= 0,		/* Listen connection */
	TCP_CONNECT	= 1,		/* Outgoing connection */
	SYNACK_RXTIMER	= 250,		/* ms between SYNACK retransmits */

	TCPREXMTTHRESH	= 3,		/* dupack threshhold for rxt */

	FORCE		= 1,
	CLONE		= 2,
	RETRAN		= 4,
	ACTIVE		= 8,
	SYNACK		= 16,

	LOGAGAIN	= 3,
	LOGDGAIN	= 2,

	Closed		= 0,		/* Connection states */
	Listen,
	Syn_sent,
	Syn_received,
	Established,
	Finwait1,
	Finwait2,
	Close_wait,
	Closing,
	Last_ack,
	Time_wait,

	Maxlimbo	= 1000,		/* maximum procs waiting for response to SYN ACK */
	NLHT		= 256,		/* hash table size, must be a power of 2 */
	LHTMASK		= NLHT-1,

	/*
	 * window is 64kb * 2ⁿ
	 * these factors determine the ultimate bandwidth-delay product.
	 * 64kb * 2⁵ = 2mb, or 2× overkill for 100mbps * 70ms.
	 */
	Maxqscale	= 4,		/* maximum queuing scale */
	Defadvscale	= 4,		/* default advertisement */
};

/* Must correspond to the enumeration above */
char *tcpstates[] =
{
	"Closed", 	"Listen", 	"Syn_sent", "Syn_received",
	"Established", 	"Finwait1",	"Finwait2", "Close_wait",
	"Closing", 	"Last_ack", 	"Time_wait"
};

typedef struct Tcptimer Tcptimer;
struct Tcptimer
{
	Tcptimer	*next;
	Tcptimer	*prev;
	Tcptimer	*readynext;
	int	state;
	int	start;
	int	count;
	void	(*func)(void*);
	void	*arg;
};

/*
 *  v4 and v6 pseudo headers used for
 *  checksuming tcp
 */
typedef struct Tcp4hdr Tcp4hdr;
struct Tcp4hdr
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	Unused;
	uchar	proto;
	uchar	tcplen[2];
	uchar	tcpsrc[4];
	uchar	tcpdst[4];
	uchar	tcpsport[2];
	uchar	tcpdport[2];
	uchar	tcpseq[4];
	uchar	tcpack[4];
	uchar	tcpflag[2];
	uchar	tcpwin[2];
	uchar	tcpcksum[2];
	uchar	tcpurg[2];
	/* Options segment */
	uchar	tcpopt[1];
};

typedef struct Tcp6hdr Tcp6hdr;
struct Tcp6hdr
{
	uchar	vcf[4];
	uchar	ploadlen[2];
	uchar	proto;
	uchar	ttl;
	uchar	tcpsrc[IPaddrlen];
	uchar	tcpdst[IPaddrlen];
	uchar	tcpsport[2];
	uchar	tcpdport[2];
	uchar	tcpseq[4];
	uchar	tcpack[4];
	uchar	tcpflag[2];
	uchar	tcpwin[2];
	uchar	tcpcksum[2];
	uchar	tcpurg[2];
	/* Options segment */
	uchar	tcpopt[1];
};

/*
 *  this represents the control info
 *  for a single packet.  It is derived from
 *  a packet in ntohtcp{4,6}() and stuck into
 *  a packet in htontcp{4,6}().
 */
typedef struct Tcp Tcp;
struct	Tcp
{
	ushort	source;
	ushort	dest;
	ulong	seq;
	ulong	ack;
	uchar	flags;
	uchar	update;
	ushort	ws;	/* window scale option */
	ulong	wnd;	/* prescaled window*/
	ushort	urg;
	ushort	mss;	/* max segment size option (if not zero) */
	ushort	len;	/* size of data */
};

/*
 *  this header is malloc'd to thread together fragments
 *  waiting to be coalesced
 */
typedef struct Reseq Reseq;
struct Reseq
{
	Reseq	*next;
	Tcp	seg;
	Block	*bp;
	ushort	length;
};

/*
 *  the qlock in the Conv locks this structure
 */
typedef struct Tcpctl Tcpctl;
struct Tcpctl
{
	uchar	state;			/* Connection state */
	uchar	type;			/* Listening or active connection */
	uchar	code;			/* Icmp code */
	struct {
		ulong	una;		/* Unacked data pointer */
		ulong	nxt;		/* Next sequence expected */
		ulong	ptr;		/* Data pointer */
		ulong	wnd;		/* Tcp send window */
		ulong	urg;		/* Urgent data pointer */
		ulong	wl2;
		uint	scale;		/* how much to right shift window */
					/* in xmitted packets */
		/* to implement tahoe and reno TCP */
		ulong	dupacks;	/* number of duplicate acks rcvd */
		ulong	partialack;
		int	recovery;	/* loss recovery flag */
		int	retransmit;	/* retransmit 1 packet @ una flag */
		int	rto;
		ulong	rxt;		/* right window marker for recovery */
					/* "recover" rfc3782 */
	} snd;
	struct {
		ulong	nxt;		/* Receive pointer to next uchar slot */
		ulong	wnd;		/* Receive window incoming */
		ulong	wsnt;		/* Last wptr sent.  important to */
					/* track for large bdp */
		ulong	wptr;
		ulong	urg;		/* Urgent pointer */
		ulong	ackptr;		/* last acked sequence */
		int	blocked;
		uint	scale;		/* how much to left shift window in */
					/* rcv'd packets */
	} rcv;
	ulong	iss;			/* Initial sequence number */
	ulong	cwind;			/* Congestion window */
	ulong	abcbytes;		/* appropriate byte counting rfc 3465 */
	uint	scale;			/* desired snd.scale */
	ulong	ssthresh;		/* Slow start threshold */
	int	resent;			/* Bytes just resent */
	int	irs;			/* Initial received squence */
	ushort	mss;			/* Maximum segment size */
	int	rerecv;			/* Overlap of data rerecevived */
	ulong	window;			/* Our receive window (queue) */
	uint	qscale;			/* Log2 of our receive window (queue) */
	uchar	backoff;		/* Exponential backoff counter */
	int	backedoff;		/* ms we've backed off for rexmits */
	uchar	flags;			/* State flags */
	Reseq	*reseq;			/* Resequencing queue */
	int	nreseq;
	int	reseqlen;
	Tcptimer	timer;			/* Activity timer */
	Tcptimer	acktimer;		/* Acknowledge timer */
	Tcptimer	rtt_timer;		/* Round trip timer */
	Tcptimer	katimer;		/* keep alive timer */
	ulong	rttseq;			/* Round trip sequence */
	int	srtt;			/* Smoothed round trip */
	int	mdev;			/* Mean deviation of round trip */
	int	kacounter;		/* count down for keep alive */
	uint	sndsyntime;		/* time syn sent */
	ulong	time;			/* time Finwait2 or Syn_received was sent */
	ulong	timeuna;		/* snd.una when time was set */
	int	nochecksum;		/* non-zero means don't send checksums */
	int	flgcnt;			/* number of flags in the sequence (FIN,SEQ) */

	union {
		Tcp4hdr	tcp4hdr;
		Tcp6hdr	tcp6hdr;
	} protohdr;		/* prototype header */
};

/*
 *  New calls are put in limbo rather than having a conversation structure
 *  allocated.  Thus, a SYN attack results in lots of limbo'd calls but not
 *  any real Conv structures mucking things up.  Calls in limbo rexmit their
 *  SYN ACK every SYNACK_RXTIMER ms up to 4 times, i.e., they disappear after 1 second.
 *
 *  In particular they aren't on a listener's queue so that they don't figure
 *  in the input queue limit.
 *
 *  If 1/2 of a T3 was attacking SYN packets, we'ld have a permanent queue
 *  of 70000 limbo'd calls.  Not great for a linear list but doable.  Therefore
 *  there is no hashing of this list.
 */
typedef struct Limbo Limbo;
struct Limbo
{
	Limbo	*next;

	uchar	laddr[IPaddrlen];
	uchar	raddr[IPaddrlen];
	ushort	lport;
	ushort	rport;
	ulong	irs;		/* initial received sequence */
	ulong	iss;		/* initial sent sequence */
	ushort	mss;		/* mss from the other end */
	ushort	rcvscale;	/* how much to scale rcvd windows */
	ushort	sndscale;	/* how much to scale sent windows */
	ulong	lastsend;	/* last time we sent a synack */
	uchar	version;	/* v4 or v6 */
	uchar	rexmits;	/* number of retransmissions */
};

int	tcp_irtt = DEF_RTT;	/* Initial guess at round trip time */

enum {
	/* MIB stats */
	MaxConn,
	Mss,
	ActiveOpens,
	PassiveOpens,
	EstabResets,
	CurrEstab,
	InSegs,
	OutSegs,
	RetransSegs,
	RetransSegsSent,
	RetransTimeouts,
	InErrs,
	OutRsts,

	/* non-MIB stats */
	CsumErrs,
	HlenErrs,
	LenErrs,
	Resequenced,
	OutOfOrder,
	ReseqBytelim,
	ReseqPktlim,
	Delayack,
	Wopenack,

	Recovery,
	RecoveryDone,
	RecoveryRTO,
	RecoveryNoSeq,
	RecoveryCwind,
	RecoveryPA,

	Nstats
};

static char *statnames[Nstats] =
{
[MaxConn]	"MaxConn",
[Mss]		"MaxSegment",
[ActiveOpens]	"ActiveOpens",
[PassiveOpens]	"PassiveOpens",
[EstabResets]	"EstabResets",
[CurrEstab]	"CurrEstab",
[InSegs]	"InSegs",
[OutSegs]	"OutSegs",
[RetransSegs]	"RetransSegs",
[RetransSegsSent]	"RetransSegsSent",
[RetransTimeouts]	"RetransTimeouts",
[InErrs]	"InErrs",
[OutRsts]	"OutRsts",
[CsumErrs]	"CsumErrs",
[HlenErrs]	"HlenErrs",
[LenErrs]	"LenErrs",
[OutOfOrder]	"OutOfOrder",
[Resequenced]	"Resequenced",
[ReseqBytelim]	"ReseqBytelim",
[ReseqPktlim]	"ReseqPktlim",
[Delayack]	"Delayack",
[Wopenack]	"Wopenack",

[Recovery]	"Recovery",
[RecoveryDone]	"RecoveryDone",
[RecoveryRTO]	"RecoveryRTO",

[RecoveryNoSeq]	"RecoveryNoSeq",
[RecoveryCwind]	"RecoveryCwind",
[RecoveryPA]	"RecoveryPA",
};

typedef struct Tcppriv Tcppriv;
struct Tcppriv
{
	/* List of active timers */
	QLock 	tl;
	Tcptimer *timers;

	/* hash table for matching conversations */
	Ipht	ht;

	/* calls in limbo waiting for an ACK to our SYN ACK */
	int	nlimbo;
	Limbo	*lht[NLHT];

	/* for keeping track of tcpackproc */
	QLock	apl;
	int	ackprocstarted;

	uvlong	stats[Nstats];
};

/*
 *  Setting tcpporthogdefense to non-zero enables Dong Lin's
 *  solution to hijacked systems staking out port's as a form
 *  of DoS attack.
 *
 *  To avoid stateless Conv hogs, we pick a sequence number at random.  If
 *  that number gets acked by the other end, we shut down the connection.
 *  Look for tcpporthogdefense in the code.
 */
int tcpporthogdefense = 0;

static	int	addreseq(Fs*, Tcpctl*, Tcppriv*, Tcp*, Block*, ushort);
static	int	dumpreseq(Tcpctl*);
static	void	getreseq(Tcpctl*, Tcp*, Block**, ushort*);
static	void	limbo(Conv*, uchar*, uchar*, Tcp*, int);
static	void	limborexmit(Proto*);
static	void	localclose(Conv*, char*);
static	void	procsyn(Conv*, Tcp*);
static	void	tcpacktimer(void*);
static	void	tcpiput(Proto*, Ipifc*, Block*);
static	void	tcpkeepalive(void*);
static	void	tcpoutput(Conv*);
static	void	tcprcvwin(Conv*);
static	void	tcprxmit(Conv*);
static	void	tcpsetkacounter(Tcpctl*);
static	void	tcpsetscale(Conv*, Tcpctl*, ushort, ushort);
static	void	tcpsettimer(Tcpctl*);
static	void	tcpsndsyn(Conv*, Tcpctl*);
static	void	tcpstart(Conv*, int);
static	void	tcpsynackrtt(Conv*);
static	void	tcptimeout(void*);
static	int	tcptrim(Tcpctl*, Tcp*, Block**, ushort*);

static void
tcpsetstate(Conv *s, uchar newstate)
{
	Tcpctl *tcb;
	uchar oldstate;
	Tcppriv *tpriv;

	tpriv = s->p->priv;

	tcb = (Tcpctl*)s->ptcl;

	oldstate = tcb->state;
	if(oldstate == newstate)
		return;

	if(oldstate == Established)
		tpriv->stats[CurrEstab]--;
	if(newstate == Established)
		tpriv->stats[CurrEstab]++;

	switch(newstate) {
	case Closed:
		qclose(s->rq);
		qclose(s->wq);
		qclose(s->eq);
		break;

	case Close_wait:		/* Remote closes */
		qhangup(s->rq, nil);
		break;
	}

	tcb->state = newstate;

	if(oldstate == Syn_sent && newstate != Closed)
		Fsconnected(s, nil);
}

static char*
tcpconnect(Conv *c, char **argv, int argc)
{
	char *e;
	Tcpctl *tcb;

	tcb = (Tcpctl*)(c->ptcl);
	if(tcb->state != Closed)
		return Econinuse;

	e = Fsstdconnect(c, argv, argc);
	if(e != nil)
		return e;
	tcpstart(c, TCP_CONNECT);

	return nil;
}

static int
tcpstate(Conv *c, char *state, int n)
{
	Tcpctl *s;

	s = (Tcpctl*)(c->ptcl);

	return snprint(state, n,
		"%s qin %d qout %d rq %d.%d srtt %d mdev %d sst %lud cwin %lud "
		"swin %lud>>%d rwin %lud>>%d qscale %d timer.start %d "
		"timer.count %d rerecv %d katimer.start %d katimer.count %d\n",
		tcpstates[s->state],
		c->rq ? qlen(c->rq) : 0,
		c->wq ? qlen(c->wq) : 0,
		s->nreseq, s->reseqlen,
		s->srtt, s->mdev, s->ssthresh,
		s->cwind, s->snd.wnd, s->rcv.scale, s->rcv.wnd, s->snd.scale,
		s->qscale,
		s->timer.start, s->timer.count, s->rerecv,
		s->katimer.start, s->katimer.count);
}

static int
tcpinuse(Conv *c)
{
	Tcpctl *s;

	s = (Tcpctl*)(c->ptcl);
	return s->state != Closed;
}

static char*
tcpannounce(Conv *c, char **argv, int argc)
{
	char *e;
	Tcpctl *tcb;

	tcb = (Tcpctl*)(c->ptcl);
	if(tcb->state != Closed)
		return Econinuse;

	e = Fsstdannounce(c, argv, argc);
	if(e != nil)
		return e;
	tcpstart(c, TCP_LISTEN);
	Fsconnected(c, nil);

	return nil;
}

/*
 *  tcpclose is always called with the q locked
 */
static void
tcpclose(Conv *c)
{
	Tcpctl *tcb;

	tcb = (Tcpctl*)c->ptcl;

	qhangup(c->rq, nil);
	qhangup(c->wq, nil);
	qhangup(c->eq, nil);
	qflush(c->rq);

	switch(tcb->state) {
	case Listen:
		/*
		 *  reset any incoming calls to this listener
		 */
		Fsconnected(c, "Hangup");

		localclose(c, nil);
		break;
	case Closed:
	case Syn_sent:
		localclose(c, nil);
		break;
	case Syn_received:
	case Established:
		tcb->flgcnt++;
		tcb->snd.nxt++;
		tcpsetstate(c, Finwait1);
		tcpoutput(c);
		break;
	case Close_wait:
		tcb->flgcnt++;
		tcb->snd.nxt++;
		tcpsetstate(c, Last_ack);
		tcpoutput(c);
		break;
	}
}

static void
tcpkick(void *x)
{
	Conv *s = x;
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;

	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);

	switch(tcb->state) {
	case Syn_sent:
	case Syn_received:
	case Established:
	case Close_wait:
		/*
		 * Push data
		 */
		tcpoutput(s);
		break;
	default:
		localclose(s, "Hangup");
		break;
	}

	qunlock(s);
	poperror();
}

static int seq_lt(ulong, ulong);

static void
tcprcvwin(Conv *s)				/* Call with tcb locked */
{
	int w;
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;
	w = tcb->window - qlen(s->rq);
	if(w < 0)
		w = 0;
	/* RFC 1122 § 4.2.2.17 do not move right edge of window left */
	if(seq_lt(tcb->rcv.nxt + w, tcb->rcv.wptr))
		w = tcb->rcv.wptr - tcb->rcv.nxt;
	if(w != tcb->rcv.wnd)
	if(w>>tcb->rcv.scale == 0 || tcb->window > 4*tcb->mss && w < tcb->mss/4){
		tcb->rcv.blocked = 1;
		netlog(s->p->f, Logtcp, "tcprcvwin: window %lud qlen %d ws %ud lport %d\n",
			tcb->window, qlen(s->rq), tcb->rcv.scale, s->lport);
	}
	tcb->rcv.wnd = w;
	tcb->rcv.wptr = tcb->rcv.nxt + w;
}

static void
tcpacktimer(void *v)
{
	Tcpctl *tcb;
	Conv *s;

	s = v;
	tcb = (Tcpctl*)s->ptcl;

	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);
	if(tcb->state != Closed){
		tcb->flags |= FORCE;
		tcpoutput(s);
	}
	qunlock(s);
	poperror();
}

static void
tcpcongestion(Tcpctl *tcb)
{
	ulong inflight;

	inflight = tcb->snd.nxt - tcb->snd.una;
	if(inflight > tcb->cwind)
		inflight = tcb->cwind;
	tcb->ssthresh = inflight / 2;
	if(tcb->ssthresh < 2*tcb->mss)
		tcb->ssthresh = 2*tcb->mss;
}

enum {
	L	= 2,	/* aggressive slow start; legal values ∈ (1.0, 2.0) */
};

static void
tcpabcincr(Tcpctl *tcb, uint acked)
{
	uint limit;

	tcb->abcbytes += acked;
	if(tcb->cwind < tcb->ssthresh){
		/* slow start */
		if(tcb->snd.rto)
			limit = tcb->mss;
		else
			limit = L*tcb->mss;
		tcb->cwind += MIN(tcb->abcbytes, limit);
		tcb->abcbytes = 0;
	} else {
		tcb->snd.rto = 0;
		/* avoidance */
		if(tcb->abcbytes >= tcb->cwind){
			tcb->abcbytes -= tcb->cwind;
			tcb->cwind += tcb->mss;
		}
	}
}

static void
tcpcreate(Conv *c)
{
	c->rq = qopen(QMAX, Qcoalesce, tcpacktimer, c);
	c->wq = qopen(QMAX, Qkick, tcpkick, c);
}

static void
timerstate(Tcppriv *priv, Tcptimer *t, int newstate)
{
	if(newstate != TcptimerON){
		if(t->state == TcptimerON){
			/* unchain */
			if(priv->timers == t){
				priv->timers = t->next;
				if(t->prev != nil)
					panic("timerstate1");
			}
			if(t->next)
				t->next->prev = t->prev;
			if(t->prev)
				t->prev->next = t->next;
			t->next = t->prev = nil;
		}
	} else {
		if(t->state != TcptimerON){
			/* chain */
			if(t->prev != nil || t->next != nil)
				panic("timerstate2");
			t->prev = nil;
			t->next = priv->timers;
			if(t->next)
				t->next->prev = t;
			priv->timers = t;
		}
	}
	t->state = newstate;
}

static void
tcpackproc(void *a)
{
	Tcptimer *t, *tp, *timeo;
	Proto *tcp;
	Tcppriv *priv;
	int loop;

	tcp = a;
	priv = tcp->priv;

	for(;;) {
		tsleep(&up->sleep, return0, 0, MSPTICK);

		qlock(&priv->tl);
		timeo = nil;
		loop = 0;
		for(t = priv->timers; t != nil; t = tp) {
			if(loop++ > 10000)
				panic("tcpackproc1");
			tp = t->next;
 			if(t->state == TcptimerON) {
				t->count--;
				if(t->count == 0) {
					timerstate(priv, t, TcptimerDONE);
					t->readynext = timeo;
					timeo = t;
				}
			}
		}
		qunlock(&priv->tl);

		loop = 0;
		for(t = timeo; t != nil; t = t->readynext) {
			if(loop++ > 10000)
				panic("tcpackproc2");
			if(t->state == TcptimerDONE && t->func != nil && !waserror()){
				(*t->func)(t->arg);
				poperror();
			}
		}

		limborexmit(tcp);
	}
}

static void
tcpgo(Tcppriv *priv, Tcptimer *t)
{
	if(t == nil || t->start == 0)
		return;

	qlock(&priv->tl);
	t->count = t->start;
	timerstate(priv, t, TcptimerON);
	qunlock(&priv->tl);
}

static void
tcphalt(Tcppriv *priv, Tcptimer *t)
{
	if(t == nil)
		return;

	qlock(&priv->tl);
	timerstate(priv, t, TcptimerOFF);
	qunlock(&priv->tl);
}

static int
backoff(int n)
{
	return 1 << n;
}

static void
localclose(Conv *s, char *reason)	/* called with tcb locked */
{
	Tcpctl *tcb;
	Tcppriv *tpriv;

	tpriv = s->p->priv;
	tcb = (Tcpctl*)s->ptcl;

	iphtrem(&tpriv->ht, s);

	tcphalt(tpriv, &tcb->timer);
	tcphalt(tpriv, &tcb->rtt_timer);
	tcphalt(tpriv, &tcb->acktimer);
	tcphalt(tpriv, &tcb->katimer);

	/* Flush reassembly queue; nothing more can arrive */
	dumpreseq(tcb);

	if(tcb->state == Syn_sent)
		Fsconnected(s, reason);
	if(s->state == Announced)
		wakeup(&s->listenr);

	qhangup(s->rq, reason);
	qhangup(s->wq, reason);

	tcpsetstate(s, Closed);
}

/* mtu (- TCP + IP hdr len) of 1st hop */
static int
tcpmtu(Proto *tcp, uchar *addr, int version, uint *scale)
{
	Ipifc *ifc;
	int mtu;

	ifc = findipifc(tcp->f, addr, 0);
	switch(version){
	default:
	case V4:
		mtu = DEF_MSS;
		if(ifc != nil)
			mtu = ifc->maxtu - ifc->m->hsize - (TCP4_PKT + TCP4_HDRSIZE);
		break;
	case V6:
		mtu = DEF_MSS6;
		if(ifc != nil)
			mtu = ifc->maxtu - ifc->m->hsize - (TCP6_PKT + TCP6_HDRSIZE);
		break;
	}
	/*
	 * set the ws.  it doesn't commit us to anything.
	 * ws is the ultimate limit to the bandwidth-delay product.
	 */
	*scale = Defadvscale;

	return mtu;
}

static void
inittcpctl(Conv *s, int mode)
{
	Tcpctl *tcb;
	Tcp4hdr* h4;
	Tcp6hdr* h6;
	Tcppriv *tpriv;
	int mss;

	tcb = (Tcpctl*)s->ptcl;

	memset(tcb, 0, sizeof(Tcpctl));

	tcb->ssthresh = QMAX;			/* reset by tcpsetscale() */
	tcb->srtt = tcp_irtt<<LOGAGAIN;
	tcb->mdev = 0;

	/* setup timers */
	tcb->timer.start = tcp_irtt / MSPTICK;
	tcb->timer.func = tcptimeout;
	tcb->timer.arg = s;
	tcb->rtt_timer.start = MAX_TIME;
	tcb->acktimer.start = TCP_ACK / MSPTICK;
	tcb->acktimer.func = tcpacktimer;
	tcb->acktimer.arg = s;
	tcb->katimer.start = DEF_KAT / MSPTICK;
	tcb->katimer.func = tcpkeepalive;
	tcb->katimer.arg = s;

	mss = DEF_MSS;

	/* create a prototype(pseudo) header */
	if(mode != TCP_LISTEN){
		if(ipcmp(s->laddr, IPnoaddr) == 0)
			findlocalip(s->p->f, s->laddr, s->raddr);

		switch(s->ipversion){
		case V4:
			h4 = &tcb->protohdr.tcp4hdr;
			memset(h4, 0, sizeof(*h4));
			h4->proto = IP_TCPPROTO;
			hnputs(h4->tcpsport, s->lport);
			hnputs(h4->tcpdport, s->rport);
			v6tov4(h4->tcpsrc, s->laddr);
			v6tov4(h4->tcpdst, s->raddr);
			break;
		case V6:
			h6 = &tcb->protohdr.tcp6hdr;
			memset(h6, 0, sizeof(*h6));
			h6->proto = IP_TCPPROTO;
			hnputs(h6->tcpsport, s->lport);
			hnputs(h6->tcpdport, s->rport);
			ipmove(h6->tcpsrc, s->laddr);
			ipmove(h6->tcpdst, s->raddr);
			mss = DEF_MSS6;
			break;
		default:
			panic("inittcpctl: version %d", s->ipversion);
		}
	}

	tcb->mss = tcb->cwind = mss;
	tcb->abcbytes = 0;
	tpriv = s->p->priv;
	tpriv->stats[Mss] = tcb->mss;

	/* default is no window scaling */
	tcpsetscale(s, tcb, 0, 0);
}

/*
 *  called with s qlocked
 */
static void
tcpstart(Conv *s, int mode)
{
	Tcpctl *tcb;
	Tcppriv *tpriv;
	char kpname[KNAMELEN];

	tpriv = s->p->priv;

	if(tpriv->ackprocstarted == 0){
		qlock(&tpriv->apl);
		if(tpriv->ackprocstarted == 0){
			snprint(kpname, sizeof kpname, "#I%dtcpack", s->p->f->dev);
			kproc(kpname, tcpackproc, s->p);
			tpriv->ackprocstarted = 1;
		}
		qunlock(&tpriv->apl);
	}

	tcb = (Tcpctl*)s->ptcl;

	inittcpctl(s, mode);

	iphtadd(&tpriv->ht, s);
	switch(mode) {
	case TCP_LISTEN:
		tpriv->stats[PassiveOpens]++;
		tcb->flags |= CLONE;
		tcpsetstate(s, Listen);
		break;

	case TCP_CONNECT:
		tpriv->stats[ActiveOpens]++;
		tcb->flags |= ACTIVE;
		tcpsndsyn(s, tcb);
		tcpsetstate(s, Syn_sent);
		tcpoutput(s);
		break;
	}
}

static char*
tcpflag(char *buf, char *e, ushort flag)
{
	char *p;

	p = seprint(buf, e, "%d", flag>>10);	/* Head len */
	if(flag & URG)
		p = seprint(p, e, " URG");
	if(flag & ACK)
		p = seprint(p, e, " ACK");
	if(flag & PSH)
		p = seprint(p, e, " PSH");
	if(flag & RST)
		p = seprint(p, e, " RST");
	if(flag & SYN)
		p = seprint(p, e, " SYN");
	if(flag & FIN)
		p = seprint(p, e, " FIN");
	USED(p);
	return buf;
}

static Block*
htontcp6(Tcp *tcph, Block *data, Tcp6hdr *ph, Tcpctl *tcb)
{
	int dlen;
	Tcp6hdr *h;
	ushort csum;
	ushort hdrlen, optpad = 0;
	uchar *opt;

	hdrlen = TCP6_HDRSIZE;
	if(tcph->flags & SYN){
		if(tcph->mss)
			hdrlen += MSS_LENGTH;
		if(tcph->ws)
			hdrlen += WS_LENGTH;
		optpad = hdrlen & 3;
		if(optpad)
			optpad = 4 - optpad;
		hdrlen += optpad;
	}

	if(data) {
		dlen = blocklen(data);
		data = padblock(data, hdrlen + TCP6_PKT);
		if(data == nil)
			return nil;
	}
	else {
		dlen = 0;
		data = allocb(hdrlen + TCP6_PKT + 64);	/* the 64 pad is to meet mintu's */
		if(data == nil)
			return nil;
		data->wp += hdrlen + TCP6_PKT;
	}

	/* copy in pseudo ip header plus port numbers */
	h = (Tcp6hdr *)(data->rp);
	memmove(h, ph, TCP6_TCBPHDRSZ);

	/* compose pseudo tcp header, do cksum calculation */
	hnputl(h->vcf, hdrlen + dlen);
	h->ploadlen[0] = h->ploadlen[1] = h->proto = 0;
	h->ttl = ph->proto;

	/* copy in variable bits */
	hnputl(h->tcpseq, tcph->seq);
	hnputl(h->tcpack, tcph->ack);
	hnputs(h->tcpflag, (hdrlen<<10) | tcph->flags);
	hnputs(h->tcpwin, tcph->wnd>>(tcb != nil ? tcb->snd.scale : 0));
	hnputs(h->tcpurg, tcph->urg);

	if(tcph->flags & SYN){
		opt = h->tcpopt;
		if(tcph->mss != 0){
			*opt++ = MSSOPT;
			*opt++ = MSS_LENGTH;
			hnputs(opt, tcph->mss);
			opt += 2;
		}
		if(tcph->ws != 0){
			*opt++ = WSOPT;
			*opt++ = WS_LENGTH;
			*opt++ = tcph->ws;
		}
		while(optpad-- > 0)
			*opt++ = NOOPOPT;
	}

	if(tcb != nil && tcb->nochecksum){
		h->tcpcksum[0] = h->tcpcksum[1] = 0;
	} else {
		csum = ptclcsum(data, TCP6_IPLEN, hdrlen+dlen+TCP6_PHDRSIZE);
		hnputs(h->tcpcksum, csum);
	}

	/* move from pseudo header back to normal ip header */
	memset(h->vcf, 0, 4);
	h->vcf[0] = IP_VER6;
	hnputs(h->ploadlen, hdrlen+dlen);
	h->proto = ph->proto;

	return data;
}

static Block*
htontcp4(Tcp *tcph, Block *data, Tcp4hdr *ph, Tcpctl *tcb)
{
	int dlen;
	Tcp4hdr *h;
	ushort csum;
	ushort hdrlen, optpad = 0;
	uchar *opt;

	hdrlen = TCP4_HDRSIZE;
	if(tcph->flags & SYN){
		if(tcph->mss)
			hdrlen += MSS_LENGTH;
		if(1)
			hdrlen += WS_LENGTH;
		optpad = hdrlen & 3;
		if(optpad)
			optpad = 4 - optpad;
		hdrlen += optpad;
	}

	if(data) {
		dlen = blocklen(data);
		data = padblock(data, hdrlen + TCP4_PKT);
		if(data == nil)
			return nil;
	}
	else {
		dlen = 0;
		data = allocb(hdrlen + TCP4_PKT + 64);	/* the 64 pad is to meet mintu's */
		if(data == nil)
			return nil;
		data->wp += hdrlen + TCP4_PKT;
	}

	/* copy in pseudo ip header plus port numbers */
	h = (Tcp4hdr *)(data->rp);
	memmove(h, ph, TCP4_TCBPHDRSZ);

	/* copy in variable bits */
	hnputs(h->tcplen, hdrlen + dlen);
	hnputl(h->tcpseq, tcph->seq);
	hnputl(h->tcpack, tcph->ack);
	hnputs(h->tcpflag, (hdrlen<<10) | tcph->flags);
	hnputs(h->tcpwin, tcph->wnd>>(tcb != nil ? tcb->snd.scale : 0));
	hnputs(h->tcpurg, tcph->urg);

	if(tcph->flags & SYN){
		opt = h->tcpopt;
		if(tcph->mss != 0){
			*opt++ = MSSOPT;
			*opt++ = MSS_LENGTH;
			hnputs(opt, tcph->mss);
			opt += 2;
		}
		/* always offer.  rfc1323 §2.2 */
		if(1){
			*opt++ = WSOPT;
			*opt++ = WS_LENGTH;
			*opt++ = tcph->ws;
		}
		while(optpad-- > 0)
			*opt++ = NOOPOPT;
	}

	if(tcb != nil && tcb->nochecksum){
		h->tcpcksum[0] = h->tcpcksum[1] = 0;
	} else {
		csum = ptclcsum(data, TCP4_IPLEN, hdrlen+dlen+TCP4_PHDRSIZE);
		hnputs(h->tcpcksum, csum);
	}

	return data;
}

static int
ntohtcp6(Tcp *tcph, Block **bpp)
{
	Tcp6hdr *h;
	uchar *optr;
	ushort hdrlen;
	ushort optlen;
	int n;

	*bpp = pullupblock(*bpp, TCP6_PKT+TCP6_HDRSIZE);
	if(*bpp == nil)
		return -1;

	h = (Tcp6hdr *)((*bpp)->rp);
	tcph->source = nhgets(h->tcpsport);
	tcph->dest = nhgets(h->tcpdport);
	tcph->seq = nhgetl(h->tcpseq);
	tcph->ack = nhgetl(h->tcpack);
	hdrlen = (h->tcpflag[0]>>2) & ~3;
	if(hdrlen < TCP6_HDRSIZE) {
		freeblist(*bpp);
		return -1;
	}

	tcph->flags = h->tcpflag[1];
	tcph->wnd = nhgets(h->tcpwin);
	tcph->urg = nhgets(h->tcpurg);
	tcph->mss = 0;
	tcph->ws = 0;
	tcph->update = 0;
	tcph->len = nhgets(h->ploadlen) - hdrlen;

	*bpp = pullupblock(*bpp, hdrlen+TCP6_PKT);
	if(*bpp == nil)
		return -1;

	optr = h->tcpopt;
	n = hdrlen - TCP6_HDRSIZE;
	while(n > 0 && *optr != EOLOPT) {
		if(*optr == NOOPOPT) {
			n--;
			optr++;
			continue;
		}
		optlen = optr[1];
		if(optlen < 2 || optlen > n)
			break;
		switch(*optr) {
		case MSSOPT:
			if(optlen == MSS_LENGTH)
				tcph->mss = nhgets(optr+2);
			break;
		case WSOPT:
			if(optlen == WS_LENGTH && *(optr+2) <= 14)
				tcph->ws = *(optr+2);
			break;
		}
		n -= optlen;
		optr += optlen;
	}
	return hdrlen;
}

static int
ntohtcp4(Tcp *tcph, Block **bpp)
{
	Tcp4hdr *h;
	uchar *optr;
	ushort hdrlen;
	ushort optlen;
	int n;

	*bpp = pullupblock(*bpp, TCP4_PKT+TCP4_HDRSIZE);
	if(*bpp == nil)
		return -1;

	h = (Tcp4hdr *)((*bpp)->rp);
	tcph->source = nhgets(h->tcpsport);
	tcph->dest = nhgets(h->tcpdport);
	tcph->seq = nhgetl(h->tcpseq);
	tcph->ack = nhgetl(h->tcpack);

	hdrlen = (h->tcpflag[0]>>2) & ~3;
	if(hdrlen < TCP4_HDRSIZE) {
		freeblist(*bpp);
		return -1;
	}

	tcph->flags = h->tcpflag[1];
	tcph->wnd = nhgets(h->tcpwin);
	tcph->urg = nhgets(h->tcpurg);
	tcph->mss = 0;
	tcph->ws = 0;
	tcph->update = 0;
	tcph->len = nhgets(h->length) - (hdrlen + TCP4_PKT);

	*bpp = pullupblock(*bpp, hdrlen+TCP4_PKT);
	if(*bpp == nil)
		return -1;

	optr = h->tcpopt;
	n = hdrlen - TCP4_HDRSIZE;
	while(n > 0 && *optr != EOLOPT) {
		if(*optr == NOOPOPT) {
			n--;
			optr++;
			continue;
		}
		optlen = optr[1];
		if(optlen < 2 || optlen > n)
			break;
		switch(*optr) {
		case MSSOPT:
			if(optlen == MSS_LENGTH)
				tcph->mss = nhgets(optr+2);
			break;
		case WSOPT:
			if(optlen == WS_LENGTH && *(optr+2) <= 14)
				tcph->ws = *(optr+2);
			break;
		}
		n -= optlen;
		optr += optlen;
	}
	return hdrlen;
}

/*
 *  For outgoing calls, generate an initial sequence
 *  number and put a SYN on the send queue
 */
static void
tcpsndsyn(Conv *s, Tcpctl *tcb)
{
	Tcppriv *tpriv;

	tcb->iss = (nrand(1<<16)<<16)|nrand(1<<16);
	tcb->rttseq = tcb->iss;
	tcb->snd.wl2 = tcb->iss;
	tcb->snd.una = tcb->iss;
	tcb->snd.rxt = tcb->iss;
	tcb->snd.ptr = tcb->rttseq;
	tcb->snd.nxt = tcb->rttseq;
	tcb->flgcnt++;
	tcb->flags |= FORCE;
	tcb->sndsyntime = NOW;

	/* set desired mss and scale */
	tcb->mss = tcpmtu(s->p, s->laddr, s->ipversion, &tcb->scale);
	tpriv = s->p->priv;
	tpriv->stats[Mss] = tcb->mss;
}

void
sndrst(Proto *tcp, uchar *source, uchar *dest, ushort length, Tcp *seg, uchar version, char *reason)
{
	Block *hbp;
	uchar rflags;
	Tcppriv *tpriv;
	Tcp4hdr ph4;
	Tcp6hdr ph6;

	netlog(tcp->f, Logtcp, "sndrst: %s\n", reason);

	tpriv = tcp->priv;

	if(seg->flags & RST)
		return;

	/* make pseudo header */
	switch(version) {
	case V4:
		memset(&ph4, 0, sizeof(ph4));
		ph4.vihl = IP_VER4;
		v6tov4(ph4.tcpsrc, dest);
		v6tov4(ph4.tcpdst, source);
		ph4.proto = IP_TCPPROTO;
		hnputs(ph4.tcplen, TCP4_HDRSIZE);
		hnputs(ph4.tcpsport, seg->dest);
		hnputs(ph4.tcpdport, seg->source);
		break;
	case V6:
		memset(&ph6, 0, sizeof(ph6));
		ph6.vcf[0] = IP_VER6;
		ipmove(ph6.tcpsrc, dest);
		ipmove(ph6.tcpdst, source);
		ph6.proto = IP_TCPPROTO;
		hnputs(ph6.ploadlen, TCP6_HDRSIZE);
		hnputs(ph6.tcpsport, seg->dest);
		hnputs(ph6.tcpdport, seg->source);
		break;
	default:
		panic("sndrst: version %d", version);
	}

	tpriv->stats[OutRsts]++;
	rflags = RST;

	/* convince the other end that this reset is in band */
	if(seg->flags & ACK) {
		seg->seq = seg->ack;
		seg->ack = 0;
	}
	else {
		rflags |= ACK;
		seg->ack = seg->seq;
		seg->seq = 0;
		if(seg->flags & SYN)
			seg->ack++;
		seg->ack += length;
		if(seg->flags & FIN)
			seg->ack++;
	}
	seg->flags = rflags;
	seg->wnd = 0;
	seg->urg = 0;
	seg->mss = 0;
	seg->ws = 0;
	switch(version) {
	case V4:
		hbp = htontcp4(seg, nil, &ph4, nil);
		if(hbp == nil)
			return;
		ipoput4(tcp->f, hbp, 0, MAXTTL, DFLTTOS, nil);
		break;
	case V6:
		hbp = htontcp6(seg, nil, &ph6, nil);
		if(hbp == nil)
			return;
		ipoput6(tcp->f, hbp, 0, MAXTTL, DFLTTOS, nil);
		break;
	default:
		panic("sndrst2: version %d", version);
	}
}

/*
 *  send a reset to the remote side and close the conversation
 *  called with s qlocked
 */
static char*
tcphangup(Conv *s)
{
	Tcp seg;
	Tcpctl *tcb;
	Block *hbp;

	tcb = (Tcpctl*)s->ptcl;
	if(waserror())
		return commonerror();
	if(ipcmp(s->raddr, IPnoaddr) != 0) {
		if(!waserror()){
			memset(&seg, 0, sizeof seg);
			seg.flags = RST | ACK;
			seg.ack = tcb->rcv.nxt;
			tcb->rcv.ackptr = seg.ack;
			seg.seq = tcb->snd.ptr;
			seg.wnd = 0;
			seg.urg = 0;
			seg.mss = 0;
			seg.ws = 0;
			switch(s->ipversion) {
			case V4:
				tcb->protohdr.tcp4hdr.vihl = IP_VER4;
				hbp = htontcp4(&seg, nil, &tcb->protohdr.tcp4hdr, tcb);
				ipoput4(s->p->f, hbp, 0, s->ttl, s->tos, s);
				break;
			case V6:
				tcb->protohdr.tcp6hdr.vcf[0] = IP_VER6;
				hbp = htontcp6(&seg, nil, &tcb->protohdr.tcp6hdr, tcb);
				ipoput6(s->p->f, hbp, 0, s->ttl, s->tos, s);
				break;
			default:
				panic("tcphangup: version %d", s->ipversion);
			}
			poperror();
		}
	}
	localclose(s, nil);
	poperror();
	return nil;
}

/*
 *  (re)send a SYN ACK
 */
static int
sndsynack(Proto *tcp, Limbo *lp)
{
	Block *hbp;
	Tcp4hdr ph4;
	Tcp6hdr ph6;
	Tcp seg;
	uint scale;

	/* make pseudo header */
	switch(lp->version) {
	case V4:
		memset(&ph4, 0, sizeof(ph4));
		ph4.vihl = IP_VER4;
		v6tov4(ph4.tcpsrc, lp->laddr);
		v6tov4(ph4.tcpdst, lp->raddr);
		ph4.proto = IP_TCPPROTO;
		hnputs(ph4.tcplen, TCP4_HDRSIZE);
		hnputs(ph4.tcpsport, lp->lport);
		hnputs(ph4.tcpdport, lp->rport);
		break;
	case V6:
		memset(&ph6, 0, sizeof(ph6));
		ph6.vcf[0] = IP_VER6;
		ipmove(ph6.tcpsrc, lp->laddr);
		ipmove(ph6.tcpdst, lp->raddr);
		ph6.proto = IP_TCPPROTO;
		hnputs(ph6.ploadlen, TCP6_HDRSIZE);
		hnputs(ph6.tcpsport, lp->lport);
		hnputs(ph6.tcpdport, lp->rport);
		break;
	default:
		panic("sndrst: version %d", lp->version);
	}

	memset(&seg, 0, sizeof seg);
	seg.seq = lp->iss;
	seg.ack = lp->irs+1;
	seg.flags = SYN|ACK;
	seg.urg = 0;
	seg.mss = tcpmtu(tcp, lp->laddr, lp->version, &scale);
	seg.wnd = QMAX;

	/* if the other side set scale, we should too */
	if(lp->rcvscale){
		seg.ws = scale;
		lp->sndscale = scale;
	} else {
		seg.ws = 0;
		lp->sndscale = 0;
	}

	switch(lp->version) {
	case V4:
		hbp = htontcp4(&seg, nil, &ph4, nil);
		if(hbp == nil)
			return -1;
		ipoput4(tcp->f, hbp, 0, MAXTTL, DFLTTOS, nil);
		break;
	case V6:
		hbp = htontcp6(&seg, nil, &ph6, nil);
		if(hbp == nil)
			return -1;
		ipoput6(tcp->f, hbp, 0, MAXTTL, DFLTTOS, nil);
		break;
	default:
		panic("sndsnack: version %d", lp->version);
	}
	lp->lastsend = NOW;
	return 0;
}

#define hashipa(a, p) ( ( (a)[IPaddrlen-2] + (a)[IPaddrlen-1] + p )&LHTMASK )

/*
 *  put a call into limbo and respond with a SYN ACK
 *
 *  called with proto locked
 */
static void
limbo(Conv *s, uchar *source, uchar *dest, Tcp *seg, int version)
{
	Limbo *lp, **l;
	Tcppriv *tpriv;
	int h;

	tpriv = s->p->priv;
	h = hashipa(source, seg->source);

	for(l = &tpriv->lht[h]; *l != nil; l = &lp->next){
		lp = *l;
		if(lp->lport != seg->dest || lp->rport != seg->source || lp->version != version)
			continue;
		if(ipcmp(lp->raddr, source) != 0)
			continue;
		if(ipcmp(lp->laddr, dest) != 0)
			continue;

		/* each new SYN restarts the retransmits */
		lp->irs = seg->seq;
		break;
	}
	lp = *l;
	if(lp == nil){
		if(tpriv->nlimbo >= Maxlimbo && tpriv->lht[h]){
			lp = tpriv->lht[h];
			tpriv->lht[h] = lp->next;
			lp->next = nil;
		} else {
			lp = malloc(sizeof(*lp));
			if(lp == nil)
				return;
			tpriv->nlimbo++;
		}
		*l = lp;
		lp->version = version;
		ipmove(lp->laddr, dest);
		ipmove(lp->raddr, source);
		lp->lport = seg->dest;
		lp->rport = seg->source;
		lp->mss = seg->mss;
		lp->rcvscale = seg->ws;
		lp->irs = seg->seq;
		lp->iss = (nrand(1<<16)<<16)|nrand(1<<16);
	}

	if(sndsynack(s->p, lp) < 0){
		*l = lp->next;
		tpriv->nlimbo--;
		free(lp);
	}
}

/*
 *  resend SYN ACK's once every SYNACK_RXTIMER ms.
 */
static void
limborexmit(Proto *tcp)
{
	Tcppriv *tpriv;
	Limbo **l, *lp;
	int h;
	int seen;
	ulong now;

	tpriv = tcp->priv;

	if(!canqlock(tcp))
		return;
	seen = 0;
	now = NOW;
	for(h = 0; h < NLHT && seen < tpriv->nlimbo; h++){
		for(l = &tpriv->lht[h]; *l != nil && seen < tpriv->nlimbo; ){
			lp = *l;
			seen++;
			if(now - lp->lastsend < (lp->rexmits+1)*SYNACK_RXTIMER)
				continue;

			/* time it out after 1 second */
			if(++(lp->rexmits) > 5){
				tpriv->nlimbo--;
				*l = lp->next;
				free(lp);
				continue;
			}

			/* if we're being attacked, don't bother resending SYN ACK's */
			if(tpriv->nlimbo > 100)
				continue;

			if(sndsynack(tcp, lp) < 0){
				tpriv->nlimbo--;
				*l = lp->next;
				free(lp);
				continue;
			}

			l = &lp->next;
		}
	}
	qunlock(tcp);
}

/*
 *  lookup call in limbo.  if found, throw it out.
 *
 *  called with proto locked
 */
static void
limborst(Conv *s, Tcp *segp, uchar *src, uchar *dst, uchar version)
{
	Limbo *lp, **l;
	int h;
	Tcppriv *tpriv;

	tpriv = s->p->priv;

	/* find a call in limbo */
	h = hashipa(src, segp->source);
	for(l = &tpriv->lht[h]; *l != nil; l = &lp->next){
		lp = *l;
		if(lp->lport != segp->dest || lp->rport != segp->source || lp->version != version)
			continue;
		if(ipcmp(lp->laddr, dst) != 0)
			continue;
		if(ipcmp(lp->raddr, src) != 0)
			continue;

		/* RST can only follow the SYN */
		if(segp->seq == lp->irs+1){
			tpriv->nlimbo--;
			*l = lp->next;
			free(lp);
		}
		break;
	}
}

static void
initialwindow(Tcpctl *tcb)
{
	/* RFC 3390 initial window */
	if(tcb->mss < 1095)
		tcb->cwind = 4*tcb->mss;
	else if(tcb->mss < 2190)
		tcb->cwind = 2*2190;
	else
		tcb->cwind = 2*tcb->mss;
}

/*
 *  come here when we finally get an ACK to our SYN-ACK.
 *  lookup call in limbo.  if found, create a new conversation
 *
 *  called with proto locked
 */
static Conv*
tcpincoming(Conv *s, Tcp *segp, uchar *src, uchar *dst, uchar version)
{
	Conv *new;
	Tcpctl *tcb;
	Tcppriv *tpriv;
	Tcp4hdr *h4;
	Tcp6hdr *h6;
	Limbo *lp, **l;
	int h;

	/* unless it's just an ack, it can't be someone coming out of limbo */
	if((segp->flags & SYN) || (segp->flags & ACK) == 0)
		return nil;

	tpriv = s->p->priv;

	/* find a call in limbo */
	h = hashipa(src, segp->source);
	for(l = &tpriv->lht[h]; (lp = *l) != nil; l = &lp->next){
		netlog(s->p->f, Logtcp, "tcpincoming s %I!%ud/%I!%ud d %I!%ud/%I!%ud v %d/%d\n",
			src, segp->source, lp->raddr, lp->rport,
			dst, segp->dest, lp->laddr, lp->lport,
			version, lp->version
 		);

		if(lp->lport != segp->dest || lp->rport != segp->source || lp->version != version)
			continue;
		if(ipcmp(lp->laddr, dst) != 0)
			continue;
		if(ipcmp(lp->raddr, src) != 0)
			continue;

		/* we're assuming no data with the initial SYN */
		if(segp->seq != lp->irs+1 || segp->ack != lp->iss+1){
			netlog(s->p->f, Logtcp, "tcpincoming s %lux/%lux a %lux %lux\n",
				segp->seq, lp->irs+1, segp->ack, lp->iss+1);
			lp = nil;
		} else {
			tpriv->nlimbo--;
			*l = lp->next;
		}
		break;
	}
	if(lp == nil)
		return nil;

	new = Fsnewcall(s, src, segp->source, dst, segp->dest, version);
	if(new == nil)
		return nil;

	memmove(new->ptcl, s->ptcl, sizeof(Tcpctl));
	tcb = (Tcpctl*)new->ptcl;
	tcb->flags &= ~CLONE;
	tcb->timer.arg = new;
	tcb->timer.state = TcptimerOFF;
	tcb->acktimer.arg = new;
	tcb->acktimer.state = TcptimerOFF;
	tcb->katimer.arg = new;
	tcb->katimer.state = TcptimerOFF;
	tcb->rtt_timer.arg = new;
	tcb->rtt_timer.state = TcptimerOFF;

	tcb->irs = lp->irs;
	tcb->rcv.nxt = tcb->irs+1;
	tcb->rcv.wptr = tcb->rcv.nxt;
	tcb->rcv.wsnt = 0;
	tcb->rcv.urg = tcb->rcv.nxt;

	tcb->iss = lp->iss;
	tcb->rttseq = tcb->iss;
	tcb->snd.wl2 = tcb->iss;
	tcb->snd.una = tcb->iss+1;
	tcb->snd.ptr = tcb->iss+1;
	tcb->snd.nxt = tcb->iss+1;
	tcb->snd.rxt = tcb->iss+1;
	tcb->flgcnt = 0;
	tcb->flags |= SYNACK;

	/* our sending max segment size cannot be bigger than what he asked for */
	if(lp->mss != 0 && lp->mss < tcb->mss) {
		tcb->mss = lp->mss;
		tpriv->stats[Mss] = tcb->mss;
	}

	/* window scaling */
	tcpsetscale(new, tcb, lp->rcvscale, lp->sndscale);

	/* congestion window */
	tcb->snd.wnd = segp->wnd;
	initialwindow(tcb);

	/* set initial round trip time */
	tcb->sndsyntime = lp->lastsend+lp->rexmits*SYNACK_RXTIMER;
	tcpsynackrtt(new);

	free(lp);

	/* set up proto header */
	switch(version){
	case V4:
		h4 = &tcb->protohdr.tcp4hdr;
		memset(h4, 0, sizeof(*h4));
		h4->proto = IP_TCPPROTO;
		hnputs(h4->tcpsport, new->lport);
		hnputs(h4->tcpdport, new->rport);
		v6tov4(h4->tcpsrc, dst);
		v6tov4(h4->tcpdst, src);
		break;
	case V6:
		h6 = &tcb->protohdr.tcp6hdr;
		memset(h6, 0, sizeof(*h6));
		h6->proto = IP_TCPPROTO;
		hnputs(h6->tcpsport, new->lport);
		hnputs(h6->tcpdport, new->rport);
		ipmove(h6->tcpsrc, dst);
		ipmove(h6->tcpdst, src);
		break;
	default:
		panic("tcpincoming: version %d", new->ipversion);
	}

	tcpsetstate(new, Established);

	iphtadd(&tpriv->ht, new);

	return new;
}

static int
seq_within(ulong x, ulong low, ulong high)
{
	if(low <= high){
		if(low <= x && x <= high)
			return 1;
	}
	else {
		if(x >= low || x <= high)
			return 1;
	}
	return 0;
}

static int
seq_lt(ulong x, ulong y)
{
	return (int)(x-y) < 0;
}

static int
seq_le(ulong x, ulong y)
{
	return (int)(x-y) <= 0;
}

static int
seq_gt(ulong x, ulong y)
{
	return (int)(x-y) > 0;
}

static int
seq_ge(ulong x, ulong y)
{
	return (int)(x-y) >= 0;
}

/*
 *  use the time between the first SYN and it's ack as the
 *  initial round trip time
 */
static void
tcpsynackrtt(Conv *s)
{
	Tcpctl *tcb;
	int delta;
	Tcppriv *tpriv;

	tcb = (Tcpctl*)s->ptcl;
	tpriv = s->p->priv;

	delta = NOW - tcb->sndsyntime;
	tcb->srtt = delta<<LOGAGAIN;
	tcb->mdev = delta<<LOGDGAIN;

	/* halt round trip timer */
	tcphalt(tpriv, &tcb->rtt_timer);
}

static void
update(Conv *s, Tcp *seg)
{
	int rtt, delta;
	Tcpctl *tcb;
	ulong acked;
	Tcppriv *tpriv;

	if(seg->update)
		return;
	seg->update = 1;

	tpriv = s->p->priv;
	tcb = (Tcpctl*)s->ptcl;

	/* catch zero-window updates, update window & recover */
	if(tcb->snd.wnd == 0 && seg->wnd > 0 &&
	    seq_lt(seg->ack, tcb->snd.ptr)){
		netlog(s->p->f, Logtcp, "tcp: zwu ack %lud una %lud ptr %lud win %lud\n",
			seg->ack,  tcb->snd.una, tcb->snd.ptr, seg->wnd);
		tcb->snd.wnd = seg->wnd;
		goto recovery;
	}

	/* newreno fast retransmit */
	if(seg->ack == tcb->snd.una && tcb->snd.una != tcb->snd.nxt &&
	    ++tcb->snd.dupacks == 3){		/* was TCPREXMTTHRESH */
recovery:
		if(tcb->snd.recovery){
			tpriv->stats[RecoveryCwind]++;
			tcb->cwind += tcb->mss;
		}else if(seq_le(tcb->snd.rxt, seg->ack)){
			tpriv->stats[Recovery]++;
			tcb->abcbytes = 0;
			tcb->snd.recovery = 1;
			tcb->snd.partialack = 0;
			tcb->snd.rxt = tcb->snd.nxt;
			tcpcongestion(tcb);
			tcb->cwind = tcb->ssthresh + 3*tcb->mss;
			netlog(s->p->f, Logtcpwin, "recovery inflate %ld ss %ld @%lud\n",
				tcb->cwind, tcb->ssthresh, tcb->snd.rxt);
			tcprxmit(s);
		}else{
			tpriv->stats[RecoveryNoSeq]++;
			netlog(s->p->f, Logtcpwin, "!recov %lud not ≤ %lud %ld\n",
				tcb->snd.rxt, seg->ack, tcb->snd.rxt - seg->ack);
			/* don't enter fast retransmit, don't change ssthresh */
		}
	}else if(tcb->snd.recovery){
		tpriv->stats[RecoveryCwind]++;
		tcb->cwind += tcb->mss;
	}

	/*
	 *  update window
	 */
	if(seq_gt(seg->ack, tcb->snd.wl2)
	|| (tcb->snd.wl2 == seg->ack && seg->wnd > tcb->snd.wnd)){
		/* clear dupack if we advance wl2 */
		if(tcb->snd.wl2 != seg->ack)
			tcb->snd.dupacks = 0;
		tcb->snd.wnd = seg->wnd;
		tcb->snd.wl2 = seg->ack;
	}

	if(!seq_gt(seg->ack, tcb->snd.una)){
		/*
		 *  don't let us hangup if sending into a closed window and
		 *  we're still getting acks
		 */
		if((tcb->flags&RETRAN) && tcb->snd.wnd == 0)
			tcb->backedoff = MAXBACKMS/4;
		return;
	}

	/* Compute the new send window size */
	acked = seg->ack - tcb->snd.una;

	/* avoid slow start and timers for SYN acks */
	if((tcb->flags & SYNACK) == 0) {
		tcb->flags |= SYNACK;
		acked--;
		tcb->flgcnt--;
		goto done;
	}

	/*
	 * congestion control
	 */
	if(tcb->snd.recovery){
		if(seq_ge(seg->ack, tcb->snd.rxt)){
			/* recovery finished; deflate window */
			tpriv->stats[RecoveryDone]++;
			tcb->snd.dupacks = 0;
			tcb->snd.recovery = 0;
			tcb->cwind = (tcb->snd.nxt - tcb->snd.una) + tcb->mss;
			if(tcb->ssthresh < tcb->cwind)
				tcb->cwind = tcb->ssthresh;
			netlog(s->p->f, Logtcpwin, "recovery deflate %ld %ld\n",
				tcb->cwind, tcb->ssthresh);
		} else {
			/* partial ack; we lost more than one segment */
			tpriv->stats[RecoveryPA]++;
			if(tcb->cwind > acked)
				tcb->cwind -= acked;
			else{
				netlog(s->p->f, Logtcpwin, "partial ack neg\n");
				tcb->cwind = tcb->mss;
			}
			netlog(s->p->f, Logtcpwin, "partial ack %ld left %ld cwind %ld\n",
				acked, tcb->snd.rxt - seg->ack, tcb->cwind);

			if(acked >= tcb->mss)
				tcb->cwind += tcb->mss;
			tcb->snd.partialack++;
		}
	} else
		tcpabcincr(tcb, acked);

	/* Adjust the timers according to the round trip time */
	/* TODO: fix sloppy treatment of overflow cases here. */
	if(tcb->rtt_timer.state == TcptimerON && seq_ge(seg->ack, tcb->rttseq)) {
		tcphalt(tpriv, &tcb->rtt_timer);
		if((tcb->flags&RETRAN) == 0) {
			tcb->backoff = 0;
			tcb->backedoff = 0;
			rtt = tcb->rtt_timer.start - tcb->rtt_timer.count;
			if(rtt == 0)
				rtt = 1; /* else all close sys's will rexmit in 0 time */
			rtt *= MSPTICK;
			if(tcb->srtt == 0) {
				tcb->srtt = rtt << LOGAGAIN;
				tcb->mdev = rtt << LOGDGAIN;
			} else {
				delta = rtt - (tcb->srtt>>LOGAGAIN);
				tcb->srtt += delta;
				if(tcb->srtt <= 0)
					tcb->srtt = 1;

				delta = abs(delta) - (tcb->mdev>>LOGDGAIN);
				tcb->mdev += delta;
				if(tcb->mdev <= 0)
					tcb->mdev = 1;
			}
			tcpsettimer(tcb);
		}
	}

done:
	if(qdiscard(s->wq, acked) < acked)
		tcb->flgcnt--;
	tcb->snd.una = seg->ack;

	/* newreno fast recovery */
	if(tcb->snd.recovery)
		tcprxmit(s);

	if(seq_gt(seg->ack, tcb->snd.urg))
		tcb->snd.urg = seg->ack;

	if(tcb->snd.una != tcb->snd.nxt){
		/* `impatient' variant */
		if(!tcb->snd.recovery || tcb->snd.partialack == 1){
			tcb->time = NOW;
			tcb->timeuna = tcb->snd.una;
			tcpgo(tpriv, &tcb->timer);
		}
	} else
		tcphalt(tpriv, &tcb->timer);

	if(seq_lt(tcb->snd.ptr, tcb->snd.una))
		tcb->snd.ptr = tcb->snd.una;

	if(!tcb->snd.recovery)
		tcb->flags &= ~RETRAN;
	tcb->backoff = 0;
	tcb->backedoff = 0;
}

static void
tcpiput(Proto *tcp, Ipifc*, Block *bp)
{
	Tcp seg;
	Tcp4hdr *h4;
	Tcp6hdr *h6;
	int hdrlen;
	Tcpctl *tcb;
	ushort length, csum;
	uchar source[IPaddrlen], dest[IPaddrlen];
	Conv *s;
	Fs *f;
	Tcppriv *tpriv;
	uchar version;

	f = tcp->f;
	tpriv = tcp->priv;

	tpriv->stats[InSegs]++;

	h4 = (Tcp4hdr*)(bp->rp);
	h6 = (Tcp6hdr*)(bp->rp);

	if((h4->vihl&0xF0)==IP_VER4) {
		version = V4;
		length = nhgets(h4->length);
		v4tov6(dest, h4->tcpdst);
		v4tov6(source, h4->tcpsrc);

		h4->Unused = 0;
		hnputs(h4->tcplen, length-TCP4_PKT);
		if(!(bp->flag & Btcpck) && (h4->tcpcksum[0] || h4->tcpcksum[1]) &&
			ptclcsum(bp, TCP4_IPLEN, length-TCP4_IPLEN)) {
			tpriv->stats[CsumErrs]++;
			tpriv->stats[InErrs]++;
			netlog(f, Logtcp, "bad tcp proto cksum\n");
			freeblist(bp);
			return;
		}

		hdrlen = ntohtcp4(&seg, &bp);
		if(hdrlen < 0){
			tpriv->stats[HlenErrs]++;
			tpriv->stats[InErrs]++;
			netlog(f, Logtcp, "bad tcp hdr len\n");
			return;
		}

		/* trim the packet to the size claimed by the datagram */
		length -= hdrlen+TCP4_PKT;
		bp = trimblock(bp, hdrlen+TCP4_PKT, length);
		if(bp == nil){
			tpriv->stats[LenErrs]++;
			tpriv->stats[InErrs]++;
			netlog(f, Logtcp, "tcp len < 0 after trim\n");
			return;
		}
	}
	else {
		int ttl = h6->ttl;
		int proto = h6->proto;

		version = V6;
		length = nhgets(h6->ploadlen);
		ipmove(dest, h6->tcpdst);
		ipmove(source, h6->tcpsrc);

		h6->ploadlen[0] = h6->ploadlen[1] = h6->proto = 0;
		h6->ttl = proto;
		hnputl(h6->vcf, length);
		if((h6->tcpcksum[0] || h6->tcpcksum[1]) &&
		    (csum = ptclcsum(bp, TCP6_IPLEN, length+TCP6_PHDRSIZE)) != 0) {
			tpriv->stats[CsumErrs]++;
			tpriv->stats[InErrs]++;
			netlog(f, Logtcp,
			    "bad tcpv6 proto cksum: got %#ux, computed %#ux\n",
				h6->tcpcksum[0]<<8 | h6->tcpcksum[1], csum);
			freeblist(bp);
			return;
		}
		h6->ttl = ttl;
		h6->proto = proto;
		hnputs(h6->ploadlen, length);

		hdrlen = ntohtcp6(&seg, &bp);
		if(hdrlen < 0){
			tpriv->stats[HlenErrs]++;
			tpriv->stats[InErrs]++;
			netlog(f, Logtcp, "bad tcpv6 hdr len\n");
			return;
		}

		/* trim the packet to the size claimed by the datagram */
		length -= hdrlen;
		bp = trimblock(bp, hdrlen+TCP6_PKT, length);
		if(bp == nil){
			tpriv->stats[LenErrs]++;
			tpriv->stats[InErrs]++;
			netlog(f, Logtcp, "tcpv6 len < 0 after trim\n");
			return;
		}
	}

	/* lock protocol while searching for a conversation */
	qlock(tcp);

	/* Look for a matching conversation */
	s = iphtlook(&tpriv->ht, source, seg.source, dest, seg.dest);
	if(s == nil){
		netlog(f, Logtcp, "iphtlook(src %I!%d, dst %I!%d) failed\n",
			source, seg.source, dest, seg.dest);
reset:
		qunlock(tcp);
		sndrst(tcp, source, dest, length, &seg, version, "no conversation");
		freeblist(bp);
		return;
	}

	/* if it's a listener, look for the right flags and get a new conv */
	tcb = (Tcpctl*)s->ptcl;
	if(tcb->state == Listen){
		if(seg.flags & RST){
			limborst(s, &seg, source, dest, version);
			qunlock(tcp);
			freeblist(bp);
			return;
		}

		/* if this is a new SYN, put the call into limbo */
		if((seg.flags & SYN) && (seg.flags & ACK) == 0){
			limbo(s, source, dest, &seg, version);
			qunlock(tcp);
			freeblist(bp);
			return;
		}

		/*
		 *  if there's a matching call in limbo, tcpincoming will
		 *  return it in state Syn_received
		 */
		s = tcpincoming(s, &seg, source, dest, version);
		if(s == nil)
			goto reset;
	}

	/* The rest of the input state machine is run with the control block
	 * locked and implements the state machine directly out of the RFC.
	 * Out-of-band data is ignored - it was always a bad idea.
	 */
	tcb = (Tcpctl*)s->ptcl;
	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);
	qunlock(tcp);

	/* fix up window */
	seg.wnd <<= tcb->rcv.scale;

	/* every input packet in puts off the keep alive time out */
	tcpsetkacounter(tcb);

	switch(tcb->state) {
	case Closed:
		sndrst(tcp, source, dest, length, &seg, version, "sending to Closed");
		goto raise;
	case Syn_sent:
		if(seg.flags & ACK) {
			if(!seq_within(seg.ack, tcb->iss+1, tcb->snd.nxt)) {
				sndrst(tcp, source, dest, length, &seg, version,
					 "bad seq in Syn_sent");
				goto raise;
			}
		}
		if(seg.flags & RST) {
			if(seg.flags & ACK)
				localclose(s, Econrefused);
			goto raise;
		}

		if(seg.flags & SYN) {
			procsyn(s, &seg);
			if(seg.flags & ACK){
				update(s, &seg);
				tcpsynackrtt(s);
				tcpsetstate(s, Established);
				tcpsetscale(s, tcb, seg.ws, tcb->scale);
			}
			else {
				tcb->time = NOW;
				tcpsetstate(s, Syn_received);	/* DLP - shouldn't this be a reset? */
			}

			if(length != 0 || (seg.flags & FIN))
				break;

			freeblist(bp);
			goto output;
		}
		else
			freeblist(bp);

		qunlock(s);
		poperror();
		return;
	case Syn_received:
		/* doesn't matter if it's the correct ack, we're just trying to set timing */
		if(seg.flags & ACK)
			tcpsynackrtt(s);
		break;
	}

	/*
	 *  One DOS attack is to open connections to us and then forget about them,
	 *  thereby tying up a conv at no long term cost to the attacker.
	 *  This is an attempt to defeat these stateless DOS attacks.  See
	 *  corresponding code in tcpsendka().
	 */
	if(tcb->state != Syn_received && (seg.flags & RST) == 0){
		if(tcpporthogdefense
		&& seq_within(seg.ack, tcb->snd.una-(1<<31), tcb->snd.una-(1<<29))){
			print("stateless hog %I.%d->%I.%d f %ux %lux - %lux - %lux\n",
				source, seg.source, dest, seg.dest, seg.flags,
				tcb->snd.una-(1<<31), seg.ack, tcb->snd.una-(1<<29));
			localclose(s, "stateless hog");
		}
	}

	/* Cut the data to fit the receive window */
	tcprcvwin(s);
	if(tcptrim(tcb, &seg, &bp, &length) == -1) {
		if(seg.seq+1 != tcb->rcv.nxt || length != 1)
		netlog(f, Logtcp, "tcp: trim: !inwind: seq %lud-%lud win "
			"%lud-%lud l %d from %I\n", seg.seq,
			seg.seq + length - 1, tcb->rcv.nxt,
			tcb->rcv.nxt + tcb->rcv.wnd-1, length, s->raddr);
		update(s, &seg);
		if(qlen(s->wq)+tcb->flgcnt == 0 && tcb->state == Closing) {
			tcphalt(tpriv, &tcb->rtt_timer);
			tcphalt(tpriv, &tcb->acktimer);
			tcphalt(tpriv, &tcb->katimer);
			tcpsetstate(s, Time_wait);
			tcb->timer.start = MSL2*(1000 / MSPTICK);
			tcpgo(tpriv, &tcb->timer);
		}
		if(!(seg.flags & RST)) {
			tcb->flags |= FORCE;
			goto output;
		}
		qunlock(s);
		poperror();
		return;
	}

	/* Cannot accept so answer with a rst */
	if(length && tcb->state == Closed) {
		sndrst(tcp, source, dest, length, &seg, version, "sending to Closed");
		goto raise;
	}

	/* The segment is beyond the current receive pointer so
	 * queue the data in the resequence queue
	 */
	if(seg.seq != tcb->rcv.nxt)
	if(length != 0 || (seg.flags & (SYN|FIN))) {
		update(s, &seg);
		if(addreseq(f, tcb, tpriv, &seg, bp, length) < 0)
			print("reseq %I.%d -> %I.%d\n", s->raddr, s->rport,
				s->laddr, s->lport);
		tcb->flags |= FORCE;	/* force duplicate ack; RFC 5681 §3.2 */
		goto output;
	}

	if(tcb->nreseq > 0)
		tcb->flags |= FORCE; /* filled hole in seq. space; RFC 5681 §3.2 */

	/*
	 *  keep looping till we've processed this packet plus any
	 *  adjacent packets in the resequence queue
	 */
	for(;;) {
		if(seg.flags & RST) {
			if(tcb->state == Established) {
				tpriv->stats[EstabResets]++;
				if(tcb->rcv.nxt != seg.seq)
					print("out of order RST rcvd: %I.%d -> "
						"%I.%d, rcv.nxt %lux seq %lux\n",
						s->raddr, s->rport, s->laddr,
						s->lport, tcb->rcv.nxt, seg.seq);
			}
			localclose(s, Econrefused);
			goto raise;
		}

		if((seg.flags&ACK) == 0)
			goto raise;

		switch(tcb->state) {
		case Syn_received:
			if(!seq_within(seg.ack, tcb->snd.una+1, tcb->snd.nxt)){
				sndrst(tcp, source, dest, length, &seg, version,
					"bad seq in Syn_received");
				goto raise;
			}
			update(s, &seg);
			tcpsetstate(s, Established);
		case Established:
		case Close_wait:
			update(s, &seg);
			break;
		case Finwait1:
			update(s, &seg);
			if(qlen(s->wq)+tcb->flgcnt == 0){
				tcphalt(tpriv, &tcb->rtt_timer);
				tcphalt(tpriv, &tcb->acktimer);
				tcpsetkacounter(tcb);
				tcb->time = NOW;
				tcpsetstate(s, Finwait2);
				tcb->katimer.start = MSL2 * (1000 / MSPTICK);
				tcpgo(tpriv, &tcb->katimer);
			}
			break;
		case Finwait2:
			update(s, &seg);
			break;
		case Closing:
			update(s, &seg);
			if(qlen(s->wq)+tcb->flgcnt == 0) {
				tcphalt(tpriv, &tcb->rtt_timer);
				tcphalt(tpriv, &tcb->acktimer);
				tcphalt(tpriv, &tcb->katimer);
				tcpsetstate(s, Time_wait);
				tcb->timer.start = MSL2*(1000 / MSPTICK);
				tcpgo(tpriv, &tcb->timer);
			}
			break;
		case Last_ack:
			update(s, &seg);
			if(qlen(s->wq)+tcb->flgcnt == 0) {
				localclose(s, nil);
				goto raise;
			}
		case Time_wait:
			tcb->flags |= FORCE;
			if(tcb->timer.state != TcptimerON)
				tcpgo(tpriv, &tcb->timer);
		}

		if((seg.flags&URG) && seg.urg) {
			if(seq_gt(seg.urg + seg.seq, tcb->rcv.urg)) {
				tcb->rcv.urg = seg.urg + seg.seq;
				pullblock(&bp, seg.urg);
			}
		}
		else
		if(seq_gt(tcb->rcv.nxt, tcb->rcv.urg))
			tcb->rcv.urg = tcb->rcv.nxt;

		if(length == 0) {
			if(bp != nil)
				freeblist(bp);
		}
		else {
			switch(tcb->state){
			default:
				/* Ignore segment text */
				if(bp != nil)
					freeblist(bp);
				break;

			case Syn_received:
			case Established:
			case Finwait1:
				/* If we still have some data place on
				 * receive queue
				 */
				if(bp) {
					bp = packblock(bp);
					if(bp == nil)
						panic("tcp packblock");
					qpassnolim(s->rq, bp);
					bp = nil;
				}
				tcb->rcv.nxt += length;

				/*
				 *  turn on the acktimer if there's something
				 *  to ack
				 */
				if(tcb->acktimer.state != TcptimerON)
					tcpgo(tpriv, &tcb->acktimer);

				break;
			case Finwait2:
				/* no process to read the data, send a reset */
				if(bp != nil)
					freeblist(bp);
				sndrst(tcp, source, dest, length, &seg, version,
					"send to Finwait2");
				qunlock(s);
				poperror();
				return;
			}
		}

		if(seg.flags & FIN) {
			tcb->flags |= FORCE;

			switch(tcb->state) {
			case Syn_received:
			case Established:
				tcb->rcv.nxt++;
				tcpsetstate(s, Close_wait);
				break;
			case Finwait1:
				tcb->rcv.nxt++;
				if(qlen(s->wq)+tcb->flgcnt == 0) {
					tcphalt(tpriv, &tcb->rtt_timer);
					tcphalt(tpriv, &tcb->acktimer);
					tcphalt(tpriv, &tcb->katimer);
					tcpsetstate(s, Time_wait);
					tcb->timer.start = MSL2*(1000/MSPTICK);
					tcpgo(tpriv, &tcb->timer);
				}
				else
					tcpsetstate(s, Closing);
				break;
			case Finwait2:
				tcb->rcv.nxt++;
				tcphalt(tpriv, &tcb->rtt_timer);
				tcphalt(tpriv, &tcb->acktimer);
				tcphalt(tpriv, &tcb->katimer);
				tcpsetstate(s, Time_wait);
				tcb->timer.start = MSL2 * (1000/MSPTICK);
				tcpgo(tpriv, &tcb->timer);
				break;
			case Close_wait:
			case Closing:
			case Last_ack:
				break;
			case Time_wait:
				tcpgo(tpriv, &tcb->timer);
				break;
			}
		}

		/*
		 *  get next adjacent segment from the resequence queue.
		 *  dump/trim any overlapping segments
		 */
		for(;;) {
			if(tcb->reseq == nil)
				goto output;

			if(seq_ge(tcb->rcv.nxt, tcb->reseq->seg.seq) == 0)
				goto output;

			getreseq(tcb, &seg, &bp, &length);

			tcprcvwin(s);
			if(tcptrim(tcb, &seg, &bp, &length) == 0){
				tcb->flags |= FORCE;
				break;
			}
		}
	}
output:
	tcpoutput(s);
	qunlock(s);
	poperror();
	return;
raise:
	qunlock(s);
	poperror();
	freeblist(bp);
	tcpkick(s);
}

/*
 *  always enters and exits with the s locked.  We drop
 *  the lock to ipoput the packet so some care has to be
 *  taken by callers.
 */
static void
tcpoutput(Conv *s)
{
	Tcp seg;
	uint msgs;
	Tcpctl *tcb;
	Block *hbp, *bp;
	int sndcnt;
	ulong ssize, dsize, sent;
	Fs *f;
	Tcppriv *tpriv;
	uchar version;

	f = s->p->f;
	tpriv = s->p->priv;
	version = s->ipversion;

	tcb = (Tcpctl*)s->ptcl;

	/* force ack every 2*mss */
	if((tcb->flags & FORCE) == 0 &&
	    tcb->rcv.nxt - tcb->rcv.ackptr >= 2*tcb->mss){
		tpriv->stats[Delayack]++;
		tcb->flags |= FORCE;
	}

	/* force ack if window opening */
	if((tcb->flags & FORCE) == 0){
		tcprcvwin(s);
		if((int)(tcb->rcv.wptr - tcb->rcv.wsnt) >= 2*tcb->mss){
			tpriv->stats[Wopenack]++;
			tcb->flags |= FORCE;
		}
	}

	for(msgs = 0; msgs < 100; msgs++) {
		switch(tcb->state) {
		case Listen:
		case Closed:
		case Finwait2:
			return;
		}

		/* Don't send anything else until our SYN has been acked */
		if(tcb->snd.ptr != tcb->iss && (tcb->flags & SYNACK) == 0)
			break;

		/* force an ack when a window has opened up */
		tcprcvwin(s);
		if(tcb->rcv.blocked && tcb->rcv.wnd > 0){
			tcb->rcv.blocked = 0;
			tcb->flags |= FORCE;
		}

		sndcnt = qlen(s->wq)+tcb->flgcnt;
		sent = tcb->snd.ptr - tcb->snd.una;
		ssize = sndcnt;
		if(tcb->snd.wnd == 0){
			/* zero window probe */
			if(sent > 0 && !(tcb->flags & FORCE))
				break;	/* already probing, rto re-probes */
			if(ssize < sent)
				ssize = 0;
			else{
				ssize -= sent;
				if(ssize > 0)
					ssize = 1;
			}
		} else {
			/* calculate usable segment size */
			if(ssize > tcb->cwind)
				ssize = tcb->cwind;
			if(ssize > tcb->snd.wnd)
				ssize = tcb->snd.wnd;

			if(ssize < sent)
				ssize = 0;
			else {
				ssize -= sent;
				if(ssize > tcb->mss)
					ssize = tcb->mss;
			}
		}

		dsize = ssize;
		seg.urg = 0;

		if(!(tcb->flags & FORCE))
			if(ssize == 0 ||
			    ssize < tcb->mss && tcb->snd.nxt == tcb->snd.ptr &&
			    sent > TCPREXMTTHRESH * tcb->mss)
				break;

		tcb->flags &= ~FORCE;

		/* By default we will generate an ack */
		tcphalt(tpriv, &tcb->acktimer);
		seg.source = s->lport;
		seg.dest = s->rport;
		seg.flags = ACK;
		seg.mss = 0;
		seg.ws = 0;
		seg.update = 0;
		switch(tcb->state){
		case Syn_sent:
			seg.flags = 0;
			if(tcb->snd.ptr == tcb->iss){
				seg.flags |= SYN;
				dsize--;
				seg.mss = tcb->mss;
				seg.ws = tcb->scale;
			}
			break;
		case Syn_received:
			/*
			 *  don't send any data with a SYN/ACK packet
			 *  because Linux rejects the packet in its
			 *  attempt to solve the SYN attack problem
			 */
			if(tcb->snd.ptr == tcb->iss){
				seg.flags |= SYN;
				dsize = 0;
				ssize = 1;
				seg.mss = tcb->mss;
				seg.ws = tcb->scale;
			}
			break;
		}
		seg.seq = tcb->snd.ptr;
		seg.ack = tcb->rcv.nxt;
		seg.wnd = tcb->rcv.wnd;

		/* Pull out data to send */
		bp = nil;
		if(dsize != 0) {
			bp = qcopy(s->wq, dsize, sent);
			if(BLEN(bp) != dsize) {
				seg.flags |= FIN;
				dsize--;
			}
		}

		if(sent+dsize == sndcnt && dsize)
			seg.flags |= PSH;

		tcb->snd.ptr += ssize;

		/* Pull up the send pointer so we can accept acks
		 * for this window
		 */
		if(seq_gt(tcb->snd.ptr,tcb->snd.nxt))
			tcb->snd.nxt = tcb->snd.ptr;

		/* Build header, link data and compute cksum */
		switch(version){
		case V4:
			tcb->protohdr.tcp4hdr.vihl = IP_VER4;
			hbp = htontcp4(&seg, bp, &tcb->protohdr.tcp4hdr, tcb);
			if(hbp == nil) {
				freeblist(bp);
				return;
			}
			break;
		case V6:
			tcb->protohdr.tcp6hdr.vcf[0] = IP_VER6;
			hbp = htontcp6(&seg, bp, &tcb->protohdr.tcp6hdr, tcb);
			if(hbp == nil) {
				freeblist(bp);
				return;
			}
			break;
		default:
			hbp = nil;	/* to suppress a warning */
			panic("tcpoutput: version %d", version);
		}

		/* Start the transmission timers if there is new data and we
		 * expect acknowledges
		 */
		if(ssize != 0){
			if(tcb->timer.state != TcptimerON){
				tcb->time = NOW;
				tcb->timeuna = tcb->snd.una;
				tcpgo(tpriv, &tcb->timer);
			}

			/*  If round trip timer isn't running, start it.
			 *  measure the longest packet only in case the
			 *  transmission time dominates RTT
			 */
			if(tcb->snd.retransmit == 0)
			if(tcb->rtt_timer.state != TcptimerON)
			if(ssize == tcb->mss) {
				tcpgo(tpriv, &tcb->rtt_timer);
				tcb->rttseq = tcb->snd.ptr;
			}
		}

		tpriv->stats[OutSegs]++;
		if(tcb->snd.retransmit)
			tpriv->stats[RetransSegsSent]++;
		tcb->rcv.ackptr = seg.ack;
		tcb->rcv.wsnt = tcb->rcv.wptr;

		/* put off the next keep alive */
		tcpgo(tpriv, &tcb->katimer);

		switch(version){
		case V4:
			if(ipoput4(f, hbp, 0, s->ttl, s->tos, s) < 0){
				/* a negative return means no route */
				localclose(s, "no route");
			}
			break;
		case V6:
			if(ipoput6(f, hbp, 0, s->ttl, s->tos, s) < 0){
				/* a negative return means no route */
				localclose(s, "no route");
			}
			break;
		default:
			panic("tcpoutput2: version %d", version);
		}
		if((msgs%4) == 3){
			qunlock(s);
			qlock(s);
		}
	}
}

/*
 *  the BSD convention (hack?) for keep alives.  resend last uchar acked.
 */
static void
tcpsendka(Conv *s)
{
	Tcp seg;
	Tcpctl *tcb;
	Block *hbp,*dbp;

	tcb = (Tcpctl*)s->ptcl;

	dbp = nil;
	memset(&seg, 0, sizeof seg);
	seg.urg = 0;
	seg.source = s->lport;
	seg.dest = s->rport;
	seg.flags = ACK|PSH;
	seg.mss = 0;
	seg.ws = 0;
	if(tcpporthogdefense)
		seg.seq = tcb->snd.una-(1<<30)-nrand(1<<20);
	else
		seg.seq = tcb->snd.una-1;
	seg.ack = tcb->rcv.nxt;
	tcb->rcv.ackptr = seg.ack;
	tcprcvwin(s);
	seg.wnd = tcb->rcv.wnd;
	if(tcb->state == Finwait2){
		seg.flags |= FIN;
	} else {
		dbp = allocb(1);
		dbp->wp++;
	}

	if(isv4(s->raddr)) {
		/* Build header, link data and compute cksum */
		tcb->protohdr.tcp4hdr.vihl = IP_VER4;
		hbp = htontcp4(&seg, dbp, &tcb->protohdr.tcp4hdr, tcb);
		if(hbp == nil) {
			freeblist(dbp);
			return;
		}
		ipoput4(s->p->f, hbp, 0, s->ttl, s->tos, s);
	}
	else {
		/* Build header, link data and compute cksum */
		tcb->protohdr.tcp6hdr.vcf[0] = IP_VER6;
		hbp = htontcp6(&seg, dbp, &tcb->protohdr.tcp6hdr, tcb);
		if(hbp == nil) {
			freeblist(dbp);
			return;
		}
		ipoput6(s->p->f, hbp, 0, s->ttl, s->tos, s);
	}
}

/*
 *  set connection to time out after 12 minutes
 */
static void
tcpsetkacounter(Tcpctl *tcb)
{
	tcb->kacounter = (12 * 60 * 1000) / (tcb->katimer.start*MSPTICK);
	if(tcb->kacounter < 3)
		tcb->kacounter = 3;
}

/*
 *  if we've timed out, close the connection
 *  otherwise, send a keepalive and restart the timer
 */
static void
tcpkeepalive(void *v)
{
	Tcpctl *tcb;
	Conv *s;

	s = v;
	tcb = (Tcpctl*)s->ptcl;
	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);
	if(tcb->state != Closed){
		if(--(tcb->kacounter) <= 0) {
			localclose(s, Etimedout);
		} else {
			tcpsendka(s);
			tcpgo(s->p->priv, &tcb->katimer);
		}
	}
	qunlock(s);
	poperror();
}

/*
 *  start keepalive timer
 */
static char*
tcpstartka(Conv *s, char **f, int n)
{
	Tcpctl *tcb;
	int x;

	tcb = (Tcpctl*)s->ptcl;
	if(tcb->state != Established)
		return "connection must be in Establised state";
	if(n > 1){
		x = atoi(f[1]);
		if(x >= MSPTICK)
			tcb->katimer.start = x/MSPTICK;
	}
	tcpsetkacounter(tcb);
	tcpgo(s->p->priv, &tcb->katimer);

	return nil;
}

/*
 *  turn checksums on/off
 */
static char*
tcpsetchecksum(Conv *s, char **f, int)
{
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;
	tcb->nochecksum = !atoi(f[1]);

	return nil;
}

/*
 *  retransmit (at most) one segment at snd.una.
 *  preserve cwind & snd.ptr
 */
static void
tcprxmit(Conv *s)
{
	Tcpctl *tcb;
	Tcppriv *tpriv;
	ulong tcwind, tptr;

	tcb = (Tcpctl*)s->ptcl;
	tcb->flags |= RETRAN|FORCE;

	tptr = tcb->snd.ptr;
	tcwind = tcb->cwind;
	tcb->snd.ptr = tcb->snd.una;
	tcb->cwind = tcb->mss;
	tcb->snd.retransmit = 1;
	tcpoutput(s);
	tcb->snd.retransmit = 0;
	tcb->cwind = tcwind;
	tcb->snd.ptr = tptr;

	tpriv = s->p->priv;
	tpriv->stats[RetransSegs]++;
}

/*
 *  TODO: RFC 4138 F-RTO
 */
static void
tcptimeout(void *arg)
{
	Conv *s;
	Tcpctl *tcb;
	int maxback;
	Tcppriv *tpriv;

	s = (Conv*)arg;
	tpriv = s->p->priv;
	tcb = (Tcpctl*)s->ptcl;

	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);
	switch(tcb->state){
	default:
		tcb->backoff++;
		if(tcb->state == Syn_sent)
			maxback = MAXBACKMS/2;
		else
			maxback = MAXBACKMS;
		tcb->backedoff += tcb->timer.start * MSPTICK;
		if(tcb->backedoff >= maxback) {
			localclose(s, Etimedout);
			break;
		}
		netlog(s->p->f, Logtcprxmt, "rxm %d/%d %ldms %lud rto %d %lud %s\n",
			tcb->srtt, tcb->mdev, NOW - tcb->time,
			tcb->snd.una - tcb->timeuna, tcb->snd.rto, tcb->snd.ptr,
			tcpstates[s->state]);
		tcpsettimer(tcb);
		if(tcb->snd.rto == 0)
			tcpcongestion(tcb);
		tcprxmit(s);
		tcb->snd.ptr = tcb->snd.una;
		tcb->cwind = tcb->mss;
		tcb->snd.rto = 1;
		tpriv->stats[RetransTimeouts]++;

		if(tcb->snd.recovery){
			tcb->snd.dupacks = 0;		/* reno rto */
			tcb->snd.recovery = 0;
			tpriv->stats[RecoveryRTO]++;
			tcb->snd.rxt = tcb->snd.nxt;
			netlog(s->p->f, Logtcpwin,
				"rto recovery rxt @%lud\n", tcb->snd.nxt);
		}

		tcb->abcbytes = 0;
		break;
	case Time_wait:
		localclose(s, nil);
		break;
	case Closed:
		break;
	}
	qunlock(s);
	poperror();
}

static int
inwindow(Tcpctl *tcb, int seq)
{
	return seq_within(seq, tcb->rcv.nxt, tcb->rcv.nxt+tcb->rcv.wnd-1);
}

/*
 *  set up state for a received SYN (or SYN ACK) packet
 */
static void
procsyn(Conv *s, Tcp *seg)
{
	Tcpctl *tcb;
	Tcppriv *tpriv;

	tcb = (Tcpctl*)s->ptcl;
	tcb->flags |= FORCE;

	tcb->rcv.nxt = seg->seq + 1;
	tcb->rcv.wptr = tcb->rcv.nxt;
	tcb->rcv.wsnt = 0;
	tcb->rcv.urg = tcb->rcv.nxt;
	tcb->irs = seg->seq;

	/* our sending max segment size cannot be bigger than what he asked for */
	if(seg->mss != 0 && seg->mss < tcb->mss) {
		tcb->mss = seg->mss;
		tpriv = s->p->priv;
		tpriv->stats[Mss] = tcb->mss;
	}

	tcb->snd.wnd = seg->wnd;
	initialwindow(tcb);
}

static int
dumpreseq(Tcpctl *tcb)
{
	Reseq *r, *next;

	for(r = tcb->reseq; r != nil; r = next){
		next = r->next;
		freeblist(r->bp);
		free(r);
	}
	tcb->reseq = nil;
	tcb->nreseq = 0;
	tcb->reseqlen = 0;
	return -1;
}

static void
logreseq(Fs *f, Reseq *r, ulong n)
{
	char *s;

	for(; r != nil; r = r->next){
		s = nil;
		if(r->next == nil && r->seg.seq != n)
			s = "hole/end";
		else if(r->next == nil)
			s = "end";
		else if(r->seg.seq != n)
			s = "hole";
		if(s != nil)
			netlog(f, Logtcp, "%s %lud-%lud (%ld) %#ux\n", s,
				n, r->seg.seq, r->seg.seq - n, r->seg.flags);
		n = r->seg.seq + r->seg.len;
	}
}

static int
addreseq(Fs *f, Tcpctl *tcb, Tcppriv *tpriv, Tcp *seg, Block *bp, ushort length)
{
	Reseq *rp, **rr;
	int qmax;

	rp = malloc(sizeof *rp);
	if(rp == nil){
		freeblist(bp);		/* bp always consumed by addreseq */
		return 0;
	}

	rp->seg = *seg;
	rp->bp = bp;
	rp->length = length;

	tcb->reseqlen += length;
	tcb->nreseq++;

	/* Place on reassembly list sorting by starting seq number */
	for(rr = &tcb->reseq; ; rr = &(*rr)->next)
		if(*rr == nil || seq_lt(seg->seq, (*rr)->seg.seq)){
			rp->next = *rr;
			*rr = rp;
			tpriv->stats[Resequenced]++;
			if(rp->next != nil)
				tpriv->stats[OutOfOrder]++;
			break;
		}

	qmax = tcb->window;
	if(tcb->reseqlen > qmax){
		netlog(f, Logtcp, "tcp: reseq: queue > window: %d > %d; %d packets\n",
			tcb->reseqlen, qmax, tcb->nreseq);
		logreseq(f, tcb->reseq, tcb->rcv.nxt);
		tpriv->stats[ReseqBytelim]++;
		return dumpreseq(tcb);
	}
	qmax = tcb->window / tcb->mss; /* ~190 for qscale=2, 390 for qscale=3 */
	if(tcb->nreseq > qmax){
		netlog(f, Logtcp, "resequence queue > packets: %d %d; %d bytes\n",
			tcb->nreseq, qmax, tcb->reseqlen);
		logreseq(f, tcb->reseq, tcb->rcv.nxt);
		tpriv->stats[ReseqPktlim]++;
		return dumpreseq(tcb);
	}
	return 0;
}

static void
getreseq(Tcpctl *tcb, Tcp *seg, Block **bp, ushort *length)
{
	Reseq *rp;

	rp = tcb->reseq;
	if(rp == nil)
		return;

	tcb->reseq = rp->next;

	*seg = rp->seg;
	*bp = rp->bp;
	*length = rp->length;

	tcb->nreseq--;
	tcb->reseqlen -= rp->length;

	free(rp);
}

static int
tcptrim(Tcpctl *tcb, Tcp *seg, Block **bp, ushort *length)
{
	ushort len;
	uchar accept;
	int dupcnt, excess;

	accept = 0;
	len = *length;
	if(seg->flags & SYN)
		len++;
	if(seg->flags & FIN)
		len++;

	if(tcb->rcv.wnd == 0) {
		if(len == 0 && seg->seq == tcb->rcv.nxt)
			return 0;
	}
	else {
		/* Some part of the segment should be in the window */
		if(inwindow(tcb,seg->seq))
			accept++;
		else
		if(len != 0) {
			if(inwindow(tcb, seg->seq+len-1) ||
			seq_within(tcb->rcv.nxt, seg->seq,seg->seq+len-1))
				accept++;
		}
	}
	if(!accept) {
		freeblist(*bp);
		return -1;
	}
	dupcnt = tcb->rcv.nxt - seg->seq;
	if(dupcnt > 0){
		tcb->rerecv += dupcnt;
		if(seg->flags & SYN){
			seg->flags &= ~SYN;
			seg->seq++;

			if(seg->urg > 1)
				seg->urg--;
			else
				seg->flags &= ~URG;
			dupcnt--;
		}
		if(dupcnt > 0){
			pullblock(bp, (ushort)dupcnt);
			seg->seq += dupcnt;
			*length -= dupcnt;

			if(seg->urg > dupcnt)
				seg->urg -= dupcnt;
			else {
				seg->flags &= ~URG;
				seg->urg = 0;
			}
		}
	}
	excess = seg->seq + *length - (tcb->rcv.nxt + tcb->rcv.wnd);
	if(excess > 0) {
		tcb->rerecv += excess;
		*length -= excess;
		*bp = trimblock(*bp, 0, *length);
		if(*bp == nil)
			panic("presotto is a boofhead");
		seg->flags &= ~FIN;
	}
	return 0;
}

static void
tcpadvise(Proto *tcp, Block *bp, char *msg)
{
	Tcp4hdr *h4;
	Tcp6hdr *h6;
	Tcpctl *tcb;
	uchar source[IPaddrlen];
	uchar dest[IPaddrlen];
	ushort psource, pdest;
	Conv *s, **p;

	h4 = (Tcp4hdr*)(bp->rp);
	h6 = (Tcp6hdr*)(bp->rp);

	if((h4->vihl&0xF0)==IP_VER4) {
		v4tov6(dest, h4->tcpdst);
		v4tov6(source, h4->tcpsrc);
		psource = nhgets(h4->tcpsport);
		pdest = nhgets(h4->tcpdport);
	}
	else {
		ipmove(dest, h6->tcpdst);
		ipmove(source, h6->tcpsrc);
		psource = nhgets(h6->tcpsport);
		pdest = nhgets(h6->tcpdport);
	}

	/* Look for a connection */
	qlock(tcp);
	for(p = tcp->conv; *p; p++) {
		s = *p;
		tcb = (Tcpctl*)s->ptcl;
		if(s->rport == pdest)
		if(s->lport == psource)
		if(tcb->state != Closed)
		if(ipcmp(s->raddr, dest) == 0)
		if(ipcmp(s->laddr, source) == 0){
			qlock(s);
			qunlock(tcp);
			switch(tcb->state){
			case Syn_sent:
				localclose(s, msg);
				break;
			}
			qunlock(s);
			freeblist(bp);
			return;
		}
	}
	qunlock(tcp);
	freeblist(bp);
}

static char*
tcpporthogdefensectl(char *val)
{
	if(strcmp(val, "on") == 0)
		tcpporthogdefense = 1;
	else if(strcmp(val, "off") == 0)
		tcpporthogdefense = 0;
	else
		return "unknown value for tcpporthogdefense";
	return nil;
}

/* called with c qlocked */
static char*
tcpctl(Conv* c, char** f, int n)
{
	if(n == 1 && strcmp(f[0], "hangup") == 0)
		return tcphangup(c);
	if(n >= 1 && strcmp(f[0], "keepalive") == 0)
		return tcpstartka(c, f, n);
	if(n >= 1 && strcmp(f[0], "checksum") == 0)
		return tcpsetchecksum(c, f, n);
	if(n >= 1 && strcmp(f[0], "tcpporthogdefense") == 0)
		return tcpporthogdefensectl(f[1]);
	return "unknown control request";
}

static int
tcpstats(Proto *tcp, char *buf, int len)
{
	Tcppriv *priv;
	char *p, *e;
	int i;

	priv = tcp->priv;
	p = buf;
	e = p+len;
	for(i = 0; i < Nstats; i++)
		p = seprint(p, e, "%s: %llud\n", statnames[i], priv->stats[i]);
	return p - buf;
}

/*
 *  garbage collect any stale conversations:
 *	- SYN received but no SYN-ACK after 5 seconds (could be the SYN attack)
 *	- Finwait2 after 5 minutes
 *
 *  this is called whenever we run out of channels.  Both checks are
 *  of questionable validity so we try to use them only when we're
 *  up against the wall.
 */
static int
tcpgc(Proto *tcp)
{
	Conv *c, **pp, **ep;
	int n;
	Tcpctl *tcb;


	n = 0;
	ep = &tcp->conv[tcp->nc];
	for(pp = tcp->conv; pp < ep; pp++) {
		c = *pp;
		if(c == nil)
			break;
		if(!canqlock(c))
			continue;
		tcb = (Tcpctl*)c->ptcl;
		switch(tcb->state){
		case Syn_received:
			if(NOW - tcb->time > 5000){
				localclose(c, Etimedout);
				n++;
			}
			break;
		case Finwait2:
			if(NOW - tcb->time > 5*60*1000){
				localclose(c, Etimedout);
				n++;
			}
			break;
		}
		qunlock(c);
	}
	return n;
}

static void
tcpsettimer(Tcpctl *tcb)
{
	int x;

	/* round trip dependency */
	x = backoff(tcb->backoff) *
		(tcb->mdev + (tcb->srtt>>LOGAGAIN) + MSPTICK) / MSPTICK;

	/* bounded twixt 0.3 and 64 seconds */
	if(x < 300/MSPTICK)
		x = 300/MSPTICK;
	else if(x > (64000/MSPTICK))
		x = 64000/MSPTICK;
	tcb->timer.start = x;
}

void
tcpinit(Fs *fs)
{
	Proto *tcp;
	Tcppriv *tpriv;

	tcp = smalloc(sizeof(Proto));
	tpriv = tcp->priv = smalloc(sizeof(Tcppriv));
	tcp->name = "tcp";
	tcp->connect = tcpconnect;
	tcp->announce = tcpannounce;
	tcp->ctl = tcpctl;
	tcp->state = tcpstate;
	tcp->create = tcpcreate;
	tcp->close = tcpclose;
	tcp->rcv = tcpiput;
	tcp->advise = tcpadvise;
	tcp->stats = tcpstats;
	tcp->inuse = tcpinuse;
	tcp->gc = tcpgc;
	tcp->ipproto = IP_TCPPROTO;
	tcp->nc = scalednconv();
	tcp->ptclsize = sizeof(Tcpctl);
	tpriv->stats[MaxConn] = tcp->nc;

	Fsproto(fs, tcp);
}

static void
tcpsetscale(Conv *s, Tcpctl *tcb, ushort rcvscale, ushort sndscale)
{
	/*
	 * guess at reasonable queue sizes.  there's no current way
	 * to know how many nic receive buffers we can safely tie up in the
	 * tcp stack, and we don't adjust our queues to maximize throughput
	 * and minimize bufferbloat.  n.b. the offer (rcvscale) needs to be
	 * respected, but we still control our own buffer commitment by
	 * keeping a seperate qscale.
	 */
	tcb->rcv.scale = rcvscale & 0xff;
	tcb->snd.scale = sndscale & 0xff;
	tcb->qscale = rcvscale & 0xff;
	if(rcvscale > Maxqscale)
		tcb->qscale = Maxqscale;

	if(rcvscale != tcb->rcv.scale)
		netlog(s->p->f, Logtcp, "tcpsetscale: window %lud "
			"qlen %d >> window %ud lport %d\n",
			tcb->window, qlen(s->rq), QMAX<<tcb->qscale, s->lport);
	tcb->window = QMAX << tcb->qscale;
	tcb->ssthresh = tcb->window;

	/*
	 * it's important to set wq large enough to cover the full
	 * bandwidth-delay product.  it's possible to be in loss
	 * recovery with a big window, and we need to keep sending
	 * into the inflated window.  the difference can be huge
	 * for even modest (70ms) ping times.
	 */
	qsetlimit(s->rq, tcb->window);
	qsetlimit(s->wq, tcb->window);
	tcprcvwin(s);
}
