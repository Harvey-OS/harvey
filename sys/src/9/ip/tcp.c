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
	TCP_IPLEN	= 8,
	TCP_PHDRSIZE	= 12,
	TCP_HDRSIZE	= 20,
	TCP_TCBPHDRSZ	= 40,
	TCP_PKT		= TCP_IPLEN+TCP_PHDRSIZE,
	TcptimerOFF	= 0,
	TcptimerON		= 1,
	TcptimerDONE	= 2,
	MAX_TIME 	= (1<<20),	/* Forever */
	TCP_ACK		= 200,		/* Timed ack sequence in ms */

	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Acknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */

	EOLOPT		= 0,
	NOOPOPT		= 1,
	MAXBACKMS	= 30000,	/* longest backoff time (ms) before hangup */
	MSSOPT		= 2,
	MSS_LENGTH	= 4,		/* Mean segment size */
	MSL2		= 10,
	MSPTICK		= 50,		/* Milliseconds per timer tick */
	DEF_MSS		= 1460,		/* Default mean segment */
	DEF_RTT		= 500,		/* Default round trip */
	DEF_KAT		= 30000,	/* Default time ms) between keep alives */
	TCP_LISTEN	= 0,		/* Listen connection */
	TCP_CONNECT	= 1,		/* Outgoing connection */

	TCPREXMTTHRESH  = 3,            /* dupack threshhold for rxt */

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
	Time_wait
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

typedef struct Tcphdr Tcphdr;
struct Tcphdr
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
	uchar	tcpopt[2];
	uchar	tcpmss[2];
};

typedef struct Tcp Tcp;
struct	Tcp
{
	ushort	source;
	ushort	dest;
	ulong	seq;
	ulong	ack;
	uchar	flags;
	ushort	wnd;
	ushort	urg;
	ushort	mss;
        ushort  len;	/* size of data */
};

typedef struct Reseq Reseq;
struct Reseq
{
	Reseq 	*next;
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
		ushort	wnd;		/* Tcp send window */
		ulong	urg;		/* Urgent data pointer */
		ulong	wl2;

                /* to implement tahoe and reno TCP */
                ulong dupacks;          /* number of duplicate acks rcvd */
	        int   recovery;         /* loss recovery flag */
	        ulong rxt;              /* right window marker for recovery */
	} snd;
	struct {
		ulong	nxt;		/* Receive pointer to next uchar slot */
		ushort	wnd;		/* Receive window incoming */
		ulong	urg;		/* Urgent pointer */
		int	blocked;
	} rcv;
	ulong	iss;			/* Initial sequence number */
	ushort	cwind;			/* Congestion window */
	ushort	ssthresh;		/* Slow start threshold */
	int	resent;			/* Bytes just resent */
	int	irs;			/* Initial received squence */
	ushort	mss;			/* Mean segment size */
	int	rerecv;			/* Overlap of data rerecevived */
	ushort	window;			/* Recevive window */
	int	max_snd;		/* Max send */
	ulong	last_ack;		/* Last acknowledege received */
	uchar	backoff;		/* Exponential backoff counter */
	int	backedoff;		/* ms we've backed off for rexmits */
	uchar	flags;			/* State flags */
	Reseq	*reseq;			/* Resequencing queue */
	Tcptimer	timer;			/* Activity timer */
	Tcptimer	acktimer;		/* Acknowledge timer */
	Tcptimer	rtt_timer;		/* Round trip timer */
	Tcptimer	katimer;		/* keep alive timer */
	ulong	rttseq;			/* Round trip sequence */
	int	srtt;			/* Shortened round trip */
	int	mdev;			/* Mean deviation of round trip */
	int	kacounter;		/* count down for keep alive */
	uint	sndsyntime;		/* time syn sent */
	ulong	time;			/* time Finwait2 or Syn_received was sent */
	int	nochecksum;		/* non-zero means don't send checksums */
	int	flgcnt;			/* 1 when we're waiting for a SYN/FIN ACK */

	Tcphdr	protohdr;		/* prototype header */
};

int	tcp_irtt = DEF_RTT;	/* Initial guess at round trip time */
ushort	tcp_mss  = DEF_MSS;	/* Maximum segment size to be sent */

enum {
	/* MIB stats */
	MaxConn,
	ActiveOpens,
	PassiveOpens,
	EstabResets,
	CurrEstab,
	InSegs,
	OutSegs,
	RetransSegs,
	RetransTimeouts,
	InErrs,
	OutRsts,

	/* non-MIB stats */
	CsumErrs,
	HlenErrs,
	LenErrs,
	OutOfOrder,

	Nstats
};

static char *statnames[] =
{
[MaxConn]	"MaxConn",
[ActiveOpens]	"ActiveOpens",
[PassiveOpens]	"PassiveOpens",
[EstabResets]	"EstabResets",
[CurrEstab]	"CurrEstab",
[InSegs]	"InSegs",
[OutSegs]	"OutSegs",
[RetransSegs]	"RetransSegs",
[RetransTimeouts]	"RetransTimeouts",
[InErrs]	"InErrs",
[OutRsts]	"OutRsts",
[CsumErrs]	"CsumErrs",
[HlenErrs]	"HlenErrs",
[LenErrs]	"LenErrs",
[OutOfOrder]	"OutOfOrder",
};

typedef struct Tcppriv Tcppriv;
struct Tcppriv
{
	Tcptimer 	*timers;		/* List of active timers */
	QLock 	tl;			/* Protect timer list */
	Rendez	tcpr;			/* used by tcpackproc */

	/* hash table for matching conversations */
	Ipht	ht;

	ulong	stats[Nstats];

	/* for keeping track of tcpackproc */
	int	ackprocstarted;
	QLock	apl;
};

int	addreseq(Tcpctl*, Tcp*, Block*, ushort);
void	getreseq(Tcpctl*, Tcp*, Block**, ushort*);
void	localclose(Conv*, char*);
void	procsyn(Conv*, Tcp*);
void	tcpiput(Proto*, Ipifc*, Block*);
void	tcpoutput(Conv*);
int	tcptrim(Tcpctl*, Tcp*, Block**, ushort*);
void	tcpstart(Conv*, int, ushort);
void	tcptimeout(void*);
void	tcpsndsyn(Tcpctl*);
void	tcprcvwin(Conv*);
void	tcpacktimer(void*);
void	tcpkeepalive(void*);
void	tcpsetkacounter(Tcpctl*);
void    tcprxmit(Conv*);
void	tcpsettimer(Tcpctl*);

void
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

	/**
	print( "%d/%d %s->%s CurrEstab=%d\n", s->lport, s->rport,
		tcpstates[oldstate], tcpstates[newstate], tpriv->tstats.tcpCurrEstab );
	**/

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

	e = Fsstdconnect(c, argv, argc);
	if(e != nil)
		return e;
	tcpstart(c, TCP_CONNECT, QMAX);

	return nil;
}

static int
tcpstate(Conv *c, char *state, int n)
{
	Tcpctl *s;

	s = (Tcpctl*)(c->ptcl);

	return snprint(state, n,
		"%s srtt %d mdev %d cwin %d swin %d rwin %d timer.start %d timer.count %d rerecv %d\n",
		tcpstates[s->state], s->srtt, s->mdev,
		s->cwind, s->snd.wnd, s->rcv.wnd,
		s->timer.start, s->timer.count, s->rerecv);
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

	e = Fsstdannounce(c, argv, argc);
	if(e != nil)
		return e;
	tcpstart(c, TCP_LISTEN, QMAX);
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

void
tcpkick(Conv *s)
{
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;

	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);

	switch(tcb->state) {
	case Listen:
		tcb->flags |= ACTIVE;
		tcpsndsyn(tcb);
		tcpsetstate(s, Syn_sent);
		/* No break */
	case Syn_sent:
	case Syn_received:
	case Established:
	case Close_wait:
		/*
		 * Push data
		 */
		tcprcvwin(s);
		tcpoutput(s);
		break;
	default:
		localclose(s, "Hangup");
		break;
	}

	qunlock(s);
	poperror();
}

void
tcprcvwin(Conv *s)				/* Call with tcb locked */
{
	int w;
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;
	w = QMAX - qlen(s->rq);
	if(w < 0)
		w = 0;
	tcb->rcv.wnd = w;
	if(w == 0)
		tcb->rcv.blocked = 1;
}

void
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
		tcprcvwin(s);
		tcpoutput(s);
	}
	qunlock(s);
	poperror();
}

static void
tcpcreate(Conv *c)
{
	c->rq = qopen(QMAX, -1, tcpacktimer, c);
	c->wq = qopen(2*QMAX, 0, 0, 0);
}

static void
timerstate(Tcppriv *priv, Tcptimer *t, int newstate)
{
	if(newstate != TcptimerON){
		if(t->state == TcptimerON){
			// unchain
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
			// chain
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

void
tcpackproc(void *a)
{
	Tcptimer *t, *tp, *timeo;
	Proto *tcp;
	Tcppriv *priv;
	int loop;

	tcp = a;
	priv = tcp->priv;

	for(;;) {
		tsleep(&priv->tcpr, return0, 0, MSPTICK);

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
	}
}

void
tcpgo(Tcppriv *priv, Tcptimer *t)
{
	if(t == nil || t->start == 0)
		return;

	qlock(&priv->tl);
	t->count = t->start;
	timerstate(priv, t, TcptimerON);
	qunlock(&priv->tl);
}

void
tcphalt(Tcppriv *priv, Tcptimer *t)
{
	if(t == nil)
		return;

	qlock(&priv->tl);
	timerstate(priv, t, TcptimerOFF);
	qunlock(&priv->tl);
}

int
backoff(int n)
{
	if(n < 5)
		return 1 << n;

	return 64;
}

void
localclose(Conv *s, char *reason)	 /*  called with tcb locked */
{
	Tcpctl *tcb;
	Reseq *rp,*rp1;
	Tcppriv *tpriv;

	tpriv = s->p->priv;
	tcb = (Tcpctl*)s->ptcl;

	iphtrem(&tpriv->ht, s);

	tcphalt(tpriv, &tcb->timer);
	tcphalt(tpriv, &tcb->rtt_timer);
	tcphalt(tpriv, &tcb->acktimer);
	tcphalt(tpriv, &tcb->katimer);

	/* Flush reassembly queue; nothing more can arrive */
	for(rp = tcb->reseq; rp != nil; rp = rp1) {
		rp1 = rp->next;
		freeblist(rp->bp);
		free(rp);
	}
	tcb->reseq = nil;

	if(tcb->state == Syn_sent)
		Fsconnected(s, reason);
	if(s->state == Announced)
		wakeup(&s->listenr);

	qhangup(s->rq, reason);
	qhangup(s->wq, reason);

	tcpsetstate(s, Closed);
}

/* mtu (- TCP + IP hdr len) of 1st hop */
int
tcpmtu(Conv *s)
{
	Ipifc *ifc;
	int mtu;

	mtu = 0;
	ifc = findipifc(s->p->f, s->raddr, 0);
	if(ifc != nil)
		mtu = ifc->maxmtu - ifc->m->hsize - (TCP_PKT + TCP_HDRSIZE);
	if(mtu < 4)
		mtu = DEF_MSS;
	return restrict_mtu(s->raddr, mtu);
}

void
inittcpctl(Conv *s, int mode)
{
	Tcpctl *tcb;
	Tcphdr *h;

	tcb = (Tcpctl*)s->ptcl;

	memset(tcb, 0, sizeof(Tcpctl));

	tcb->ssthresh = 65535;
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

	/* create a prototype(pseudo) header */
	if(mode != TCP_LISTEN)
	if(ipcmp(s->laddr, IPnoaddr) == 0)
		findlocalip(s->p->f, s->laddr, s->raddr);
	h = &tcb->protohdr;
	memset(h, 0, sizeof(*h));
	h->proto = IP_TCPPROTO;
	hnputs(h->tcpsport, s->lport);
	hnputs(h->tcpdport, s->rport);
	v6tov4(h->tcpsrc, s->laddr);
	v6tov4(h->tcpdst, s->raddr);

	tcb->mss = tcb->cwind = tcpmtu(s);
}

/*
 *  called with s qlocked
 */
void
tcpstart(Conv *s, int mode, ushort window)
{
	Tcpctl *tcb;
	Tcppriv *tpriv;
	char kpname[KNAMELEN];

	tpriv = s->p->priv;

	if(tpriv->ackprocstarted == 0){
		qlock(&tpriv->apl);
		if(tpriv->ackprocstarted == 0){
			sprint(kpname, "#I%dtcpack", s->p->f->dev);
			kproc(kpname, tcpackproc, s->p);
			tpriv->ackprocstarted = 1;
		}
		qunlock(&tpriv->apl);
	}

	tcb = (Tcpctl*)s->ptcl;

	/* Send SYN, go into SYN_SENT state */
	inittcpctl(s, mode);
	tcb->window = window;
	tcb->rcv.wnd = window;

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
		tcpsndsyn(tcb);
		tcpsetstate(s, Syn_sent);
		tcpoutput(s);
		break;
	}
}

static char*
tcpflag(ushort flag)
{
	static char buf[128];

	sprint(buf, "%d", flag>>10);	/* Head len */
	if(flag & URG)
		strcat(buf, " URG");
	if(flag & ACK)
		strcat(buf, " ACK");
	if(flag & PSH)
		strcat(buf, " PSH");
	if(flag & RST)
		strcat(buf, " RST");
	if(flag & SYN)
		strcat(buf, " SYN");
	if(flag & FIN)
		strcat(buf, " FIN");

	return buf;
}

Block *
htontcp(Tcp *tcph, Block *data, Tcphdr *ph, Tcpctl *tcb)
{
	int dlen;
	Tcphdr *h;
	ushort csum;
	ushort hdrlen;

	hdrlen = TCP_HDRSIZE;
	if(tcph->mss)
		hdrlen += MSS_LENGTH;

	if(data) {
		dlen = blocklen(data);
		data = padblock(data, hdrlen + TCP_PKT);
		if(data == nil)
			return nil;
	}
	else {
		dlen = 0;
		data = allocb(hdrlen + TCP_PKT + 64);	/* the 64 pad is to meet mintu's */
		if(data == nil)
			return nil;
		data->wp += hdrlen + TCP_PKT;
	}

	/* copy in pseudo ip header plus port numbers */
	h = (Tcphdr *)(data->rp);
	memmove(h, ph, TCP_TCBPHDRSZ);

	/* copy in variable bits */
	hnputs(h->tcplen, hdrlen + dlen);
	hnputl(h->tcpseq, tcph->seq);
	hnputl(h->tcpack, tcph->ack);
	hnputs(h->tcpflag, (hdrlen<<10) | tcph->flags);
	hnputs(h->tcpwin, tcph->wnd);
	hnputs(h->tcpurg, tcph->urg);

	if(tcph->mss != 0){
		h->tcpopt[0] = MSSOPT;
		h->tcpopt[1] = MSS_LENGTH;
		hnputs(h->tcpmss, tcph->mss);
	}
	if(tcb != nil && tcb->nochecksum){
		h->tcpcksum[0] = h->tcpcksum[1] = 0;
	} else {
		csum = ptclcsum(data, TCP_IPLEN, hdrlen+dlen+TCP_PHDRSIZE);
		hnputs(h->tcpcksum, csum);
	}

/*	netlog(f, Logtcpmsg, "%d > %d s %l8.8ux a %8.8lux %s w %.4ux l %d\n",
		tcph->source, tcph->dest,
		tcph->seq, tcph->ack, tcpflag((hdrlen<<10)|tcph->flags),
		tcph->wnd, dlen); */

	return data;
}

int
ntohtcp(Tcp *tcph, Block **bpp)
{
	Tcphdr *h;
	uchar *optr;
	ushort hdrlen;
	ushort optlen;
	int n;

	*bpp = pullupblock(*bpp, TCP_PKT+TCP_HDRSIZE);
	if(*bpp == nil)
		return -1;

	h = (Tcphdr *)((*bpp)->rp);
	tcph->source = nhgets(h->tcpsport);
	tcph->dest = nhgets(h->tcpdport);
	tcph->seq = nhgetl(h->tcpseq);
	tcph->ack = nhgetl(h->tcpack);

	hdrlen = (h->tcpflag[0] & 0xf0)>>2;
	if(hdrlen < TCP_HDRSIZE) {
		freeblist(*bpp);
		return -1;
	}

	tcph->flags = h->tcpflag[1];
	tcph->wnd = nhgets(h->tcpwin);
	tcph->urg = nhgets(h->tcpurg);
	tcph->mss = 0;
	tcph->len = nhgets(h->length) - (hdrlen + TCP_PKT);

	*bpp = pullupblock(*bpp, hdrlen+TCP_PKT);
	if(*bpp == nil)
		return -1;

/*	netlog(Logtcpmsg, "%d > %d s %l8.8ux a %8.8lux %s w %.4ux l %d\n",
		tcph->source, tcph->dest,
		tcph->seq, tcph->ack, tcpflag((hdrlen<<10)|tcph->flags),
		tcph->wnd, nhgets(h->length)-hdrlen-TCP_PKT); */

	optr = h->tcpopt;
	n = hdrlen - TCP_HDRSIZE;
	while(n > 0 && *optr != EOLOPT) {
		if(*optr == NOOPOPT) {
			n--;
			optr++;
			continue;
		}
		optlen = optr[1];
		if(optlen < 2 || optlen > n)
			break;
if(0) print("tcpopt %d %d\n", *optr, optlen);
		switch(*optr) {
		case MSSOPT:
			if(optlen == MSS_LENGTH)
				tcph->mss = nhgets(optr+2);
			break;
		}
		n -= optlen;
		optr += optlen;
	}
	return hdrlen;
}

/* Generate an initial sequence number and put a SYN on the send queue */
void
tcpsndsyn(Tcpctl *tcb)
{
	tcb->iss = (nrand(1<<16)<<16)|nrand(1<<16);
	tcb->rttseq = tcb->iss;
	tcb->snd.wl2 = tcb->iss;
	tcb->snd.una = tcb->iss;
	tcb->snd.ptr = tcb->rttseq;
	tcb->snd.nxt = tcb->rttseq;
	tcb->flgcnt++;
	tcb->flags |= FORCE;
	tcb->sndsyntime = msec;
}


/*
 *  called with v4 (4 byte) addresses
 */
void
sndrst(Proto *tcp, uchar *source, uchar *dest, ushort length, Tcp *seg)
{
	Tcphdr ph;
	Block *hbp;
	uchar rflags;
	Tcppriv *tpriv;

	tpriv = tcp->priv;

	if(seg->flags & RST)
		return;

	/* make pseudo header */
	memset(&ph, 0, sizeof(ph));
	v6tov4(ph.tcpsrc, dest);
	v6tov4(ph.tcpdst, source);
	ph.proto = IP_TCPPROTO;
	hnputs(ph.tcplen, TCP_HDRSIZE);
	hnputs(ph.tcpsport, seg->dest);
	hnputs(ph.tcpdport, seg->source);

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
	hbp = htontcp(seg, nil, &ph, nil);
	if(hbp == nil)
		return;

	ipoput(tcp->f, hbp, 0, MAXTTL, DFLTTOS);
}

/*
 *  send a reset to the remote side and close the conversation
 *  called with s qlocked
 */
char*
tcphangup(Conv *s)
{
	Tcp seg;
	Tcpctl *tcb;
	Tcphdr ph;
	Block *hbp;

	tcb = (Tcpctl*)s->ptcl;
	if(waserror())
		return commonerror();
	if(s->raddr != 0) {
		seg.flags = RST | ACK;
		seg.ack = tcb->rcv.nxt;
		seg.seq = tcb->snd.ptr;
		seg.wnd = 0;
		seg.urg = 0;
		seg.mss = 0;
		tcb->last_ack = tcb->rcv.nxt;
		hnputs(ph.tcplen, TCP_HDRSIZE);
		hbp = htontcp(&seg, nil, &tcb->protohdr, tcb);
		ipoput(s->p->f, hbp, 0, s->ttl, s->tos);
	}
	localclose(s, nil);
	poperror();
	return nil;
}

Conv*
tcpincoming(Conv *s, Tcp *segp, uchar *src, uchar *dst)
{
	Conv *new;
	Tcpctl *tcb;
	Tcphdr *h;
	Tcppriv *tpriv;

	new = Fsnewcall(s, src, segp->source, dst, segp->dest);
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

	h = &tcb->protohdr;
	memset(h, 0, sizeof(*h));
	h->proto = IP_TCPPROTO;
	hnputs(h->tcpsport, new->lport);
	hnputs(h->tcpdport, new->rport);
	v6tov4(h->tcpsrc, dst);
	v6tov4(h->tcpdst, src);

	tpriv = new->p->priv;
	iphtadd(&tpriv->ht, new);

	return new;
}

int
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

int
seq_lt(ulong x, ulong y)
{
	return (int)(x-y) < 0;
}

int
seq_le(ulong x, ulong y)
{
	return (int)(x-y) <= 0;
}

int
seq_gt(ulong x, ulong y)
{
	return (int)(x-y) > 0;
}

int
seq_ge(ulong x, ulong y)
{
	return (int)(x-y) >= 0;
}

/*
 *  use the time between the first SYN and it's ack as the
 *  initial round trip time
 */
void
tcpsynackrtt(Conv *s)
{
	Tcpctl *tcb;
	int delta;
	Tcppriv *tpriv;

	tcb = (Tcpctl*)s->ptcl;
	tpriv = s->p->priv;

	delta = msec - tcb->sndsyntime;
	tcb->srtt = delta<<LOGAGAIN;
	tcb->mdev = delta<<LOGDGAIN;

	/* halt round trip timer */
	tcphalt(tpriv, &tcb->rtt_timer);
}

void
update(Conv *s, Tcp *seg)
{
	int rtt, delta;
	Tcpctl *tcb;
	ushort acked, expand;
	Tcppriv *tpriv;

	tpriv = s->p->priv;
	tcb = (Tcpctl*)s->ptcl;

	/* if everything has been acked, force output(?) */
	if(seq_gt(seg->ack, tcb->snd.nxt)) {
		tcb->flags |= FORCE;
		return;
	}

	/* added by Dong for fast retransmission */
	if(seg->ack == tcb->snd.una
	&& tcb->snd.una != tcb->snd.nxt
	&& seg->len == 0
	&& seg->wnd == tcb->snd.wnd) {

		/* this is a pure ack w/o window update */
		netlog(s->p->f, Logtcpmsg, "dupack %lud ack %lud sndwnd %d advwin %d\n",
			tcb->snd.dupacks, seg->ack, tcb->snd.wnd, seg->wnd);

		if(++tcb->snd.dupacks == TCPREXMTTHRESH) {
			/*
			 *  tahoe tcp rxt the packet, half sshthresh,
 			 *  and set cwnd to one packet
			 */
			tcb->snd.recovery = 1;
			tcb->snd.rxt = tcb->snd.nxt;
			netlog(s->p->f, Logtcpmsg, "fast rxt %lud, nxt %lud\n", tcb->snd.una, tcb->snd.nxt);
			tcprxmit(s);
		} else {
			/* do reno tcp here. */
		}
	}

	/*
	 *  update window
	 */
	if( seq_gt(seg->ack, tcb->snd.wl2)
	||  (tcb->snd.wl2 == seg->ack && seg->wnd > tcb->snd.wnd)){
		tcb->snd.wnd = seg->wnd;
		tcb->snd.wl2 = seg->ack;
	}


	if(!seq_gt(seg->ack, tcb->snd.una)){
		/*
		 *  don't let us hangup if sending into a closed window and
		 *  we're still getting acks
		 */
		if((tcb->flags&RETRAN) && tcb->snd.wnd == 0){
			tcb->backedoff = MAXBACKMS/4;
		}
		return;
	}

        /*
	 *  any positive ack turns off fast rxt,
         *  (should we do new-reno on partial acks?)
	 */
	if(!tcb->snd.recovery || seq_ge(seg->ack, tcb->snd.rxt)) {
		tcb->snd.dupacks = 0;
		tcb->snd.recovery = 0;
	} else
		netlog(s->p->f, Logtcp, "rxt next %lud, cwin %ud\n", seg->ack, tcb->cwind);

	/* Compute the new send window size */
	acked = seg->ack - tcb->snd.una;

	/* avoid slow start and timers for SYN acks */
	if((tcb->flags & SYNACK) == 0) {
		tcb->flags |= SYNACK;
		acked--;
		tcb->flgcnt--;
		goto done;
	}

	/* slow start as long as we're not recovering from lost packets */
	if(tcb->cwind < tcb->snd.wnd && !tcb->snd.recovery) {
		if(tcb->cwind < tcb->ssthresh) {
			expand = tcb->mss;
			if(acked < expand)
				expand = acked;
		}
		else
			expand = ((int)tcb->mss * tcb->mss) / tcb->cwind;

		if(tcb->cwind + expand < tcb->cwind)
			expand = 65535 - tcb->cwind;
		if(tcb->cwind + expand > tcb->snd.wnd)
			expand = tcb->snd.wnd - tcb->cwind;
		if(expand != 0)
			tcb->cwind += expand;
	}

	/* Adjust the timers according to the round trip time */
	if(tcb->rtt_timer.state == TcptimerON && seq_ge(seg->ack, tcb->rttseq)) {
		tcphalt(tpriv, &tcb->rtt_timer);
		if((tcb->flags&RETRAN) == 0) {
			tcb->backoff = 0;
			tcb->backedoff = 0;
			rtt = tcb->rtt_timer.start - tcb->rtt_timer.count;
			if(rtt == 0)
				rtt = 1;	/* otherwise all close systems will rexmit in 0 time */
			rtt *= MSPTICK;
			if (tcb->srtt == 0) {
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
	if(seq_gt(seg->ack, tcb->snd.urg))
		tcb->snd.urg = seg->ack;

	tcphalt(tpriv, &tcb->timer);
	if(tcb->snd.una != tcb->snd.nxt)
		tcpgo(tpriv, &tcb->timer);

	if(seq_lt(tcb->snd.ptr, tcb->snd.una))
		tcb->snd.ptr = tcb->snd.una;

	tcb->flags &= ~RETRAN;
	tcb->backoff = 0;
	tcb->backedoff = 0;
}

void
tcpiput(Proto *tcp, Ipifc*, Block *bp)
{
	Tcp seg;
	Tcphdr *h;
	int hdrlen;
	Tcpctl *tcb;
	ushort length;
	uchar source[IPaddrlen], dest[IPaddrlen];
	Conv *s;
	Fs *f;
	Tcppriv *tpriv;

	f = tcp->f;
	tpriv = tcp->priv;
	
	tpriv->stats[InSegs]++;

	h = (Tcphdr*)(bp->rp);

	length = nhgets(h->length);
	v4tov6(dest, h->tcpdst);
	v4tov6(source, h->tcpsrc);

	h->Unused = 0;
	hnputs(h->tcplen, length-TCP_PKT);
	if((h->tcpcksum[0] || h->tcpcksum[1]) && 
	    ptclcsum(bp, TCP_IPLEN, length-TCP_IPLEN)) {
		tpriv->stats[CsumErrs]++;
		tpriv->stats[InErrs]++;
		netlog(f, Logtcp, "bad tcp proto cksum\n");
		freeblist(bp);
		return;
	}

	hdrlen = ntohtcp(&seg, &bp);
	if(hdrlen < 0){
		tpriv->stats[HlenErrs]++;
		tpriv->stats[InErrs]++;
		netlog(f, Logtcp, "bad tcp hdr len\n");
		return;
	}

	/* trim the packet to the size claimed by the datagram */
	length -= hdrlen+TCP_PKT;
	bp = trimblock(bp, hdrlen+TCP_PKT, length);
	if(bp == nil){
		tpriv->stats[LenErrs]++;
		tpriv->stats[InErrs]++;
		netlog(f, Logtcp, "tcp len < 0 after trim\n");
		return;
	}

	/* lock protocol while searching for a conversation */
	qlock(tcp);

	/* Look for a matching conversation */
	s = iphtlook(&tpriv->ht, source, seg.source, dest, seg.dest);
	if(s == nil){
reset:
		qunlock(tcp);
		sndrst(tcp, source, dest, length, &seg);
		freeblist(bp);
		return;
	}

	/* if it's a listener, look for the right flags and get a new conv */
	tcb = (Tcpctl*)s->ptcl;
	if(tcb->state == Listen){
		if(seg.flags & RST){
			qunlock(tcp);
			freeblist(bp);
			return;
		}
		if((seg.flags & SYN) == 0 || (seg.flags & ACK) != 0)
			goto reset;

		s = tcpincoming(s, &seg, source, dest);
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

	if(tcb->kacounter > 0)
		tcb->kacounter = MAXBACKMS / (tcb->katimer.start*MSPTICK);
	if(tcb->kacounter < 3)
		tcb->kacounter = 3;

	switch(tcb->state) {
	case Closed:
		sndrst(tcp, source, dest, length, &seg);
		goto raise;
	case Listen:
		if(seg.flags & SYN) {
			procsyn(s, &seg);
			tcpsndsyn(tcb);
			tcb->time = msec;
			tcpsetstate(s, Syn_received);
			if(length != 0 || (seg.flags & FIN))
				break;
		}
		goto raise;
	case Syn_sent:
		if(seg.flags & ACK) {
			if(!seq_within(seg.ack, tcb->iss+1, tcb->snd.nxt)) {
				sndrst(tcp, source, dest, length, &seg);
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
			}
			else {
				tcb->time = msec;
				tcpsetstate(s, Syn_received);
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

	/* Cut the data to fit the receive window */
	if(tcptrim(tcb, &seg, &bp, &length) == -1) {
		netlog(f, Logtcp, "tcp len < 0, %lux\n", seg.seq);
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
		sndrst(tcp, source, dest, length, &seg);
		goto raise;
	}

	/* The segment is beyond the current receive pointer so
	 * queue the data in the resequence queue
	 */
	if(seg.seq != tcb->rcv.nxt)
	if(length != 0 || (seg.flags & (SYN|FIN))) {
		update(s, &seg);
		tpriv->stats[OutOfOrder]++;
		if(addreseq(tcb, &seg, bp, length) < 0)
			print("reseq %I.%d -> %I.%d\n", s->raddr, s->rport, s->laddr, s->lport);
		tcb->flags |= FORCE;
		goto output;
	}

	/*
	 *  keep looping till we've processed this packet plus any
	 *  adjacent packets in the resequence queue
	 */
	for(;;) {
		if(seg.flags & RST) {
			if(tcb->state == Established)
				tpriv->stats[EstabResets]++;
			localclose(s, Econrefused);
			goto raise;
		}

		if((seg.flags&ACK) == 0)
			goto raise;

		switch(tcb->state) {
		case Syn_received:
			if(!seq_within(seg.ack, tcb->snd.una+1, tcb->snd.nxt)){
				sndrst(tcp, source, dest, length, &seg);
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
				tcb->time = msec;
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
				 *  update our rcv window
				 */
				tcprcvwin(s);

				/*
				 *  force an ack if we've got 2 segs since we
				 *  last acked.
				 */
				if(tcb->rcv.nxt - tcb->last_ack >= 2*tcb->mss)
					tcb->flags |= FORCE;

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
				sndrst(tcp, source, dest, length, &seg);
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

			if(tcptrim(tcb, &seg, &bp, &length) == 0)
				break;
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
void
tcpoutput(Conv *s)
{
	Tcp seg;
	int msgs;
	Tcpctl *tcb;
	Block *hbp, *bp;
	int sndcnt, n;
	ulong ssize, dsize, usable, sent;
	Fs *f;
	Tcppriv *tpriv;

	f = s->p->f;
	tpriv = s->p->priv;

	for(msgs = 0; msgs < 100; msgs++) {
		tcb = (Tcpctl*)s->ptcl;
	
		switch(tcb->state) {
		case Listen:
		case Closed:
		case Finwait2:
			return;
		}
	
		/* force an ack when a window has opened up */
		if(tcb->rcv.blocked && tcb->rcv.wnd > 0){
			tcb->rcv.blocked = 0;
			tcb->flags |= FORCE;
		}
	
		sndcnt = qlen(s->wq)+tcb->flgcnt;
		sent = tcb->snd.ptr - tcb->snd.una;

		/* Don't send anything else until our SYN has been acked */
		if(tcb->snd.ptr != tcb->iss && (tcb->flags & SYNACK) == 0)
			break;

		/* Compute usable segment based on offered window and limit
		 * window probes to one
		 */
		if(tcb->snd.wnd == 0){
			if(sent != 0) {
				if ((tcb->flags&FORCE) == 0)
					break;
				tcb->snd.ptr = tcb->snd.una;
			}
			usable = 1;
		}
		else {
			usable = tcb->cwind;
			if(tcb->snd.wnd < usable)
				usable = tcb->snd.wnd;
			usable -= sent;
		}
		ssize = sndcnt-sent;
		if(ssize && usable < 2)
			netlog(s->p->f, Logtcp, "throttled snd.wnd 0x%ux cwind 0x%ux\n",
				tcb->snd.wnd, tcb->cwind);
		if(usable < ssize)
			ssize = usable;
		if(tcb->mss < ssize)
			ssize = tcb->mss;
		dsize = ssize;
		seg.urg = 0;

		if(ssize == 0)
		if((tcb->flags&FORCE) == 0)
			break;

		tcphalt(tpriv, &tcb->acktimer);

		tcb->flags &= ~FORCE;
		tcprcvwin(s);

		/* By default we will generate an ack */
		seg.source = s->lport;
		seg.dest = s->rport;
		seg.flags = ACK;
		seg.mss = 0;

		switch(tcb->state){
		case Syn_sent:
			seg.flags = 0;
			if(tcb->snd.ptr == tcb->iss){
				seg.flags |= SYN;
				dsize--;
				seg.mss = tcpmtu(s);
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
				seg.mss = tcpmtu(s);
			}
			break;
		}
		tcb->last_ack = tcb->rcv.nxt;
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
/*			netlog(f, Logtcp, "qcopy: dlen %d blen %d sndcnt %d qlen %d sent %d rp[0] %d\n",
				dsize, BLEN(bp), sndcnt, qlen(s->wq), sent, bp->rp[0]); */
		}

		if(sent+dsize == sndcnt)
			seg.flags |= PSH;

		/* keep track of balance of resent data */
		if(seq_lt(tcb->snd.ptr, tcb->snd.nxt)) {
			n = tcb->snd.nxt - tcb->snd.ptr;
			if(ssize < n)
				n = ssize;
			tcb->resent += n;
			netlog(f, Logtcp, "rexmit: %I.%d -> %I.%d ptr %lux nxt %lux\n",
				s->raddr, s->rport, s->laddr, s->lport, tcb->snd.ptr, tcb->snd.nxt);
			tpriv->stats[RetransSegs]++;
		}

		tcb->snd.ptr += ssize;

		/* Pull up the send pointer so we can accept acks
		 * for this window
		 */
		if(seq_gt(tcb->snd.ptr,tcb->snd.nxt))
			tcb->snd.nxt = tcb->snd.ptr;

		/* Build header, link data and compute cksum */
		hbp = htontcp(&seg, bp, &tcb->protohdr, tcb);
		if(hbp == nil) {
			freeblist(bp);
			return;
		}

		/* Start the transmission timers if there is new data and we
		 * expect acknowledges
		 */
		if(ssize != 0){
			if(tcb->timer.state != TcptimerON)
				tcpgo(tpriv, &tcb->timer);

			/*  If round trip timer isn't running, start it.
			 *  measure the longest packet only in case the
			 *  transmission time dominates RTT
			 */
			if(tcb->rtt_timer.state != TcptimerON)
			if(ssize == tcb->mss) {
				tcpgo(tpriv, &tcb->rtt_timer);
				tcb->rttseq = tcb->snd.ptr;
			}
		}

		tpriv->stats[OutSegs]++;
		if(tcb->kacounter > 0)
			tcpgo(tpriv, &tcb->katimer);
		qunlock(s);
		if(waserror()){
			qlock(s);
			nexterror();
		}
		ipoput(f, hbp, 0, s->ttl, s->tos);
		qlock(s);
		poperror();
	}
}

/*
 *  the BSD convention (hack?) for keep alives.  resend last uchar acked.
 */
void
tcpsendka(Conv *s)
{
	Tcp seg;
	Tcpctl *tcb;
	Block *hbp,*dbp;

	tcb = (Tcpctl*)s->ptcl;


	dbp = nil;
	seg.urg = 0;
	seg.source = s->lport;
	seg.dest = s->rport;
	seg.flags = ACK|PSH;
	seg.mss = 0;
	seg.seq = tcb->snd.una-1;
	seg.ack = tcb->rcv.nxt;
	seg.wnd = tcb->rcv.wnd;
	tcb->last_ack = tcb->rcv.nxt;
	if(tcb->state == Finwait2){
		seg.flags |= FIN;
	} else {
		dbp = allocb(1);
		dbp->wp++;
	}

	/* Build header, link data and compute cksum */
	hbp = htontcp(&seg, dbp, &tcb->protohdr, tcb);
	if(hbp == nil) {
		freeblist(dbp);
		return;
	}

	ipoput(s->p->f, hbp, 0, s->ttl, s->tos);
}

/*
 *  if we've timed out, close the connection
 *  otherwise, send a keepalive and restart the timer
 */
void
tcpsetkacounter(Tcpctl *tcb)
{
	tcb->kacounter = MAXBACKMS / (tcb->katimer.start*MSPTICK);;
	if(tcb->kacounter < 3)
		tcb->kacounter = 3;
}
void
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
char*
tcpstartka(Conv *s, char **f, int n)
{
	Tcpctl *tcb;
	int x;

	tcb = (Tcpctl*)s->ptcl;
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
char*
tcpsetchecksum(Conv *s, char **f, int)
{
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;
	tcb->nochecksum = !atoi(f[1]);

	return nil;
}

void
tcprxmit(Conv *s)
{
	Tcpctl *tcb;

	tcb = (Tcpctl*)s->ptcl;

	tcb->flags |= RETRAN|FORCE;
	tcb->snd.ptr = tcb->snd.una;

	/*
	 *  We should be halving the slow start thershhold (down to one
	 *  mss) but leaving it at mss seems to work well enough
	 */
 	tcb->ssthresh = tcb->mss;

	/*
	 *  pull window down to a single packet
	 */
	tcb->cwind = tcb->mss;
	tcpoutput(s);
}

void
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
		tcpsettimer(tcb);
		netlog(s->p->f, Logtcp, "timeout rexmit 0x%lux\n", tcb->snd.una);
		tcprxmit(s);
		tpriv->stats[RetransTimeouts]++;
		tcb->snd.dupacks = 0;
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

int
inwindow(Tcpctl *tcb, int seq)
{
	return seq_within(seq, tcb->rcv.nxt, tcb->rcv.nxt+tcb->rcv.wnd-1);
}

void
procsyn(Conv *s, Tcp *seg)
{
	Tcpctl *tcb;
	int mtu;

	tcb = (Tcpctl*)s->ptcl;
	tcb->flags |= FORCE;

	tcb->rcv.nxt = seg->seq + 1;
	tcb->rcv.urg = tcb->rcv.nxt;
	tcb->irs = seg->seq;
	tcb->snd.wnd = seg->wnd;

	if(seg->mss != 0)
		tcb->mss = seg->mss;

	tcb->max_snd = seg->wnd;

	mtu = tcpmtu(s);
	if(tcb->mss > mtu)
		tcb->mss = mtu;
	tcb->cwind = tcb->mss;
}

int
addreseq(Tcpctl *tcb, Tcp *seg, Block *bp, ushort length)
{
	Reseq *rp, *rp1;
	int i;
	static int once;

	rp = malloc(sizeof(Reseq));
	if(rp == nil){
		freeblist(bp);	/* bp always consumed by add_reseq */
		return 0;
	}

	rp->seg = *seg;
	rp->bp = bp;
	rp->length = length;

	/* Place on reassembly list sorting by starting seq number */
	rp1 = tcb->reseq;
	if(rp1 == nil || seq_lt(seg->seq, rp1->seg.seq)) {
		rp->next = rp1;
		tcb->reseq = rp;
		return 0;
	}

	length = 0;
	for(i = 0;; i++) {
		length += rp1->length;
		if(rp1->next == nil || seq_lt(seg->seq, rp1->next->seg.seq)) {
			rp->next = rp1->next;
			rp1->next = rp;
			break;
		}
		rp1 = rp1->next;
	}
	if(length > QMAX && once++ == 0){
		print("very long tcp resequence queue: %d\n", length);
		for(rp1 = tcb->reseq, i = 0; i < 10 && rp1 != nil; rp1 = rp1->next, i++)
			print("0x%lux 0x%lux 0x%ux\n", rp1->seg.seq, rp1->seg.ack,
				rp1->seg.flags);
		return -1;
	}
	return 0;
}

void
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

	free(rp);
}

int
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

			if (seg->urg > 1)
				seg->urg--;
			else
				seg->flags &= ~URG;
			dupcnt--;
		}
		if(dupcnt > 0){
			pullblock(bp, (ushort)dupcnt);
			seg->seq += dupcnt;
			*length -= dupcnt;

			if (seg->urg > dupcnt)
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

void
tcpadvise(Proto *tcp, Block *bp, char *msg)
{
	Tcphdr *h;
	Tcpctl *tcb;
	uchar source[IPaddrlen];
	uchar dest[IPaddrlen];
	ushort psource, pdest;
	Conv *s, **p;

	h = (Tcphdr*)(bp->rp);

	v4tov6(dest, h->tcpdst);
	v4tov6(source, h->tcpsrc);
	psource = nhgets(h->tcpsport);
	pdest = nhgets(h->tcpdport);

	/* Look for a connection */
	qlock(tcp);
	if(strcmp(msg, "unfragmentable") == 0){
		for(p = tcp->conv; *p; p++) {
			s = *p;
			tcb = (Tcpctl*)s->ptcl;
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
	} else {
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
	}
	qunlock(tcp);
	freeblist(bp);
}

/* called with c qlocked */
char*
tcpctl(Conv* c, char** f, int n)
{
	if(n == 1 && strcmp(f[0], "hangup") == 0)
		return tcphangup(c);
	if(n >= 1 && strcmp(f[0], "keepalive") == 0)
		return tcpstartka(c, f, n);
	if(n >= 1 && strcmp(f[0], "checksum") == 0)
		return tcpsetchecksum(c, f, n);
	return "unknown control request";
}

int
tcpstats(Proto *tcp, char *buf, int len)
{
	Tcppriv *priv;
	char *p, *e;
	int i;

	priv = tcp->priv;
	p = buf;
	e = p+len;
	for(i = 0; i < Nstats; i++)
		p = seprint(p, e, "%s: %lud\n", statnames[i], priv->stats[i]);
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
int
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
			if(msec - tcb->time > 5000){
				localclose(c, "timed out");
				n++;
			}
			break;
		case Finwait2:
			if(msec - tcb->time > 5*60*1000){
				localclose(c, "timed out");
				n++;
			}
			break;
		}
		qunlock(c);
	}
	return n;
}

void
tcpsettimer(Tcpctl *tcb)
{
	int x;

	/* round trip depenency */
	x = backoff(tcb->backoff) *
	    (tcb->mdev + (tcb->srtt>>LOGAGAIN) + MSPTICK) / MSPTICK;

	/* take into account delayed ack */
	if((tcb->snd.ptr - tcb->snd.una) <= 2*tcb->mss)
		x += TCP_ACK/MSPTICK;

	/* sanity check */
	if(x > (10000/MSPTICK))
		x = 10000/MSPTICK;
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
	tcp->kick = tcpkick;
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
