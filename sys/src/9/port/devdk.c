#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define	DPRINT	if(0) print

#define	NOW	(MACHP(0)->ticks)

typedef struct Dkmsg	Dkmsg;
typedef struct Line	Line;
typedef struct Dk	Dk;

enum {
	Maxdk = 4,
};

/*
 *  types of possible dkcalls
 */
enum {
	Dial,
	Announce,
	Redial
};

/*
 *  format of messages to/from the datakit controller on the common
 *  signalling line
 */
struct Dkmsg {
	uchar	type;
	uchar	srv;
	uchar	param0l;
	uchar	param0h;
	uchar	param1l;
	uchar	param1h;
	uchar	param2l;
	uchar	param2h;
	uchar	param3l;
	uchar	param3h;
	uchar	param4l;
	uchar	param4h;
};

/*
 *  message codes (T_xxx == dialin.type, D_xxx == dialin.srv)
 */
#define	T_SRV	1		/* service request */
#define   D_SERV	1		/* (host to dkmux) announce a service */
#define   D_DIAL	2		/* (host to dkmux) connect to a service */
#define   D_XINIT	7		/* (dkmux to host) line has been spliced */
#define	T_REPLY	2		/* reply to T_SRV/D_SERV or T_SRV/D_DIAL */
#define	  D_OK		1		/* not used */
#define	  D_OPEN	2		/* (dkmux to host) connection established */
#define	  D_FAIL	3		/* (dkmux to host) connection failed */
#define	T_CHG	3		/* change the status of a connection */
#define	  D_CLOSE	1		/* close the connection */
#define	  D_ISCLOSED	2		/* (dkmux to host) confirm a close */
#define	  D_CLOSEALL	3		/* (dkmux to host) close all connections */
#define	  D_REDIAL	6		/* (host to dkmux) redial a call */
#define	T_ALIVE	4		/* (host to dkmux) keep alive message */
#define	  D_CONTINUE	0		/* host has not died since last msg */
#define	  D_RESTART	1		/* host has restarted */
#define   D_MAXCHAN	2		/* request maximum line number */
#define	T_RESTART 8		/* (dkmux to host) datakit restarted */

/*
 *  macros for cracking/forming the window negotiation parameter
 */
#define MIN(x,y)  (x < y ? x : y)
#define W_WINDOW(o,d,t)  ((o<<8) | (d<<4) | t | 0100000)
#define W_VALID(x)  ((x) & 0100000)
#define W_ORIG(x)  (((x)>>8) & 017)
#define W_DEST(x)  (((x)>>4) & 017)
#define W_TRAF(x)  ((x) & 017)
#define W_DESTMAX(x,y)  (W_WINDOW(W_ORIG(x),MIN(W_DEST(x),y),W_TRAF(x)))
#define W_LIMIT(x,y)  (W_WINDOW(MIN(W_ORIG(x),y),MIN(W_DEST(x),y),W_TRAF(x)))
#define	W_VALUE(x)	(1<<((x)+4))

struct Line {
	QLock;
	Netprot;		/* stat info */
	int	lineno;
	Rendez	r;		/* wait here for dial */
	int	state;		/* dial state */
	int	err;		/* dialing error (if non zero) */
	int	window;		/* negotiated window */
	int	timestamp;	/* timestamp of last call received on this line */
	int	calltolive;	/* multiple of 15 seconds for dialing state to last */
	Queue	*rq;
	char	addr[64];
	char	raddr[64];
	char	ruser[32];
	Dk *dp;			/* interface contianing this line */
};

/*
 *  a dkmux dk.  one exists for every stream that a 
 *  dkmux line discipline is pushed onto.
 */
struct Dk
{
	QLock	netlock;
	Network	net;		/* stat info */
	Line	**linep;	/* array of line structures */

	QLock	csclock;
	Chan	*csc;

	char	name[64];	/* dk name */
	Queue	*wq;		/* dk output queue */
	int	ncsc;		/* csc line number */
	int	lines;		/* number of lines */
	int	restart;
	int	urpwindow;
	Rendez	timer;
	int	closeall;	/* set when we receive a closeall message */
	Rendez	closeallr;	/* wait here for a closeall */
};
Lock	dklock;
Dk	*dk[Maxdk];

/*
 *  conversation states (for Line.state)
 */
typedef enum {
	Lclosed=0,
	Lopened,		/* opened but no call out */
	Lconnected,		/* opened and a call set up on htis line */
	Lrclose,		/* remote end has closed down */
	Llclose,		/* local end has closed down */
	Ldialing,		/* dialing a new call */
	Llistening,		/* this line listening for calls */
	Lackwait,		/* incoming call waiting for ack/nak */
	Laccepting,		/* waiting for user to accept or reject the call */
} Lstate;
char *dkstate[] =
{
	[Lclosed]	"Closed",
	[Lopened]	"Opened",
	[Lconnected]	"Established",
	[Lrclose]	"Rclose",
	[Llclose]	"Lclose",
	[Ldialing]	"Dialing",
	[Llistening]	"Listen",
	[Lackwait]	"Ackwait",
	[Laccepting]	"Accepting",
};

/*
 *  map datakit error to errno 
 */
enum {
	DKok,
	DKbusy,
	DKnetotl,
	DKdestotl,
	DKbadnet,
	DKnetbusy,
	DKinuse,
	DKreject,
};
char* dkerr[]={
	[DKok]		"",
	[DKbusy]	"devdk: destination busy",
	[DKnetotl]	"devdk: network not answering",
	[DKdestotl]	"devdk: destination not answering",
	[DKbadnet]	"devdk: unknown address",
	[DKnetbusy]	"devdk: network busy",
	[DKinuse]	"devdk: service in use",
	[DKreject]	"devdk: connection refused", 
};
#define DKERRS sizeof(dkerr)/sizeof(char*)

/*
 *  imported
 */
extern Qinfo urpinfo;

/*
 *  predeclared
 */
Chan*		dkattach(char*);
static void	dkmuxconfig(Queue*, Block*);
static Chan*	dkopenline(Dk*, int);
static Chan*	dkopencsc(Dk*);
static int	dkmesg(Chan*, int, int, int, int);
static void	dkcsckproc(void*);
static void	dkanswer(Chan*, int, int);
static void	dkwindow(Chan*);
static void	dkcall(int, Chan*, char*, char*, char*);
static void	dktimer(void*);
static void	dkchgmesg(Chan*, Dk*, Dkmsg*, int);
static void	dkreplymesg(Dk*, Dkmsg*, int);
Chan*		dkopen(Chan*, int);
static void	dkhangup(Line*);

/*
 *  for standard network interface (net.c)
 */
static int	dkcloneline(Chan*);
static int	dklisten(Chan*);
static void	dkfilladdr(Chan*, char*, int);
static void	dkfillraddr(Chan*, char*, int);
static void	dkfillruser(Chan*, char*, int);
static void	dkfillstatus(Chan*, char*, int);

extern Qinfo dkinfo;

/*
 *  the datakit multiplexor stream module definition
 */
static Streamopen dkmuxopen;
static Streamput dkmuxoput;
static Streamput dkmuxiput;
Qinfo dkmuxinfo =
{
	dkmuxiput,
	dkmuxoput,
	dkmuxopen,
	0,
	"dkmux"
};

/*
 *  allocate a line if it doesn't exist
 */
static Line*
linealloc(Dk *dp, int lineno, int dolock)
{
	Line *lp;

	if(dolock)
		qlock(&dp->netlock);
	if(lineno > dp->lines)
		panic("linealloc");
	lp = dp->linep[lineno];
	if(lp == 0){
		lp = smalloc(sizeof(Line));
		lp->lineno = lineno;
		netadd(&dp->net, lp, lineno);
		dp->linep[lineno] = lp;
	}
	if(dolock)
		qunlock(&dp->netlock);
	return lp;
}

/*
 *  a new dkmux.  hold the stream in place so it can never be closed down.
 */
static void
dkmuxopen(Queue *q, Stream *s)
{
	RD(q)->ptr = 0;
	WR(q)->ptr = 0;

	naildownstream(s);
}

/*
 *  handle configuration
 */
static void
dkmuxoput(Queue *q, Block *bp)
{
	if(bp->type != M_DATA){
		if(streamparse("config", bp))
			dkmuxconfig(q, bp);
		else
			PUTNEXT(q, bp);
		return;
	}
	PUTNEXT(q, bp);
}

/*
 *  gather a message and send it up the appropriate stream
 *
 *  The first two bytes of each message contains the channel
 *  number, low order byte first.
 *
 *  Simplifying assumption:  one put == one message && the channel number
 *	is in the first block.  If this isn't true, demultiplexing will not
 *	work.
 */
static void
dkmuxiput(Queue *q, Block *bp)
{
	Dk *dp;
	Line *lp;
	int line;

	/*
	 *  not configured yet
	 */
	if(q->other->ptr == 0){
		freeb(bp);
		return;
	}

	dp = (Dk *)q->ptr;
	if(bp->type != M_DATA){
		PUTNEXT(q, bp);
		return;
	}

	line = bp->rptr[0] | (bp->rptr[1]<<8);
	bp->rptr += 2;
	if(line<0 || line>=dp->lines){
		DPRINT("dkmuxiput bad line %d\n", line);
		freeb(bp);
		return;
	}

	lp = linealloc(dp, line, 1);
	if(lp && canqlock(lp)){
		if(lp->rq)
			PUTNEXT(lp->rq, bp);
		else{
			DPRINT("dkmuxiput unopened line %d\n", line);
			freeb(bp);
		}
		qunlock(lp);
	} else {
		DPRINT("dkmuxiput unopened line %d\n", line);
		freeb(bp);
	}
}

/*
 *  the datakit line stream module definition
 */
static Streamopen dkstopen;
static Streamclose dkstclose;
static Streamput dkoput, dkiput;
Qinfo dkinfo =
{
	dkiput,
	dkoput,
	dkstopen,
	dkstclose,
	"dk"
};

/*
 *  open and save a pointer to the conversation
 */
static void
dkstopen(Queue *q, Stream *s)
{
	Dk *dp;
	Line *lp;

	dp = dk[s->dev];
	q->other->ptr = q->ptr = lp = dp->linep[s->id];
	lp->dp = dp;
	lp->rq = q;
	if(lp->state==Lclosed)
		lp->state = Lopened;
}

/*
 *  close down a datakit conversation
 */
static void
dkstclose(Queue *q)
{
	Dk *dp;
	Line *lp;
	Chan *c;

	lp = (Line *)q->ptr;
	dp = lp->dp;

	/*
	 *  if we never got going, we're done
	 */
	if(lp->rq == 0){
		lp->state = Lclosed; 
		return;
	}

	/*
	 *  these states don't need the datakit
	 */
	switch(lp->state){
	case Lclosed:
	case Llclose:
	case Lopened:
		lp->state = Lclosed;
		goto out;
	}

	c = 0;
	if(waserror()){
		lp->state = Lclosed;
		if(c)
			close(c);
		goto out;
	}	
	c = dkopencsc(dp);

	/*
	 *  shake hands with dk
	 */
	switch(lp->state){
	case Lrclose:
		dkmesg(c, T_CHG, D_CLOSE, lp->lineno, 0);
		lp->state = Lclosed;
		break;

	case Lackwait:
		dkmesg(c, T_CHG, D_CLOSE, lp->lineno, 0);
		lp->state = Llclose;
		break;

	case Llistening:
		dkmesg(c, T_CHG, D_CLOSE, lp->lineno, 0);
		lp->state = Llclose;
		break;

	case Lconnected:
		dkmesg(c, T_CHG, D_CLOSE, lp->lineno, 0);
		lp->state = Llclose;
		break;
	}
	poperror();
	close(c);

out:
	qlock(lp);
	lp->rq = 0;
	qunlock(lp);

	netdisown(lp);
	lp->window = 0;
}

/*
 *  this is only called by hangup
 */
static void
dkiput(Queue *q, Block *bp)
{
	PUTNEXT(q, bp);
}

/*
 *  we assume that each put is a message.
 *
 *  add a 2 byte channel number to the start of each message,
 *  low order byte first.  Make sure the first block contains
 *  both the 2 channel bytes and the control byte.
 */
static void
dkoput(Queue *q, Block *bp)
{
	Line *lp;
	Dk *dp;
	int line;

	if(bp->type != M_DATA){
		freeb(bp);
		error(Ebadarg);
	}

	lp = (Line *)q->ptr;
	dp = lp->dp;
	line = lp->lineno;

	bp = padb(bp, 2);
	bp->rptr[0] = line;
	bp->rptr[1] = line>>8;

	FLOWCTL(dp->wq, bp);
}

/*
 *  configure a datakit multiplexor.  this takes 5 arguments separated
 *  by spaces:
 *	the line number of the common signalling channel (must be > 0)
 *	the number of lines in the device (optional)
 *	the word `restart' or `norestart' (optional/default==restart)
 *	the name of the dk (default==dk)
 *	the urp window size (default==2048)
 *
 *  we can configure only once
 */
static int
haveca(void *arg)
{
	Dk *dp;

	dp = arg;
	return dp->closeall;
}
static void
dkmuxconfig(Queue *q, Block *bp)
{
	Dk *dp;
	char *fields[5];
	int n;
	char buf[64];
	char name[NAMELEN];
	int lines;
	int ncsc;
	int restart;
	int window;

	if(WR(q)->ptr){
		freeb(bp);
		error(Egreg);
	}

	/*
	 *  defaults
	 */
	ncsc = 1;
	restart = 1;
	lines = 16;
	window = 2048;
	strcpy(name, "dk");

	/*
	 *  parse
	 */
	n = getfields((char *)bp->rptr, fields, 5, " ");
	switch(n){
	case 5:
		window = strtoul(fields[4], 0, 0);
		if(window < 16)
			window = 1<<(window+4);
	case 4:
		strncpy(name, fields[3], sizeof(name));
		name[sizeof(name)-1] = 0;
	case 3:
		if(strcmp(fields[2], "restart")!=0)
			restart = 0;
	case 2:
		lines = strtoul(fields[1], 0, 0);
	case 1:
		ncsc = strtoul(fields[0], 0, 0);
		break;
	default:
		freeb(bp);
		error(Ebadarg);
	}
	freeb(bp);
	if(ncsc <= 0 || lines <= ncsc)
		error(Ebadarg);

	/*
	 *  find a free dk slot.  it name is already configured
	 *  or no slots are left, error.
	 */
	lock(&dklock);
	if(waserror()){
		unlock(&dklock);
		nexterror();
	}
	for(n = 0; n < Maxdk; n++){
		dp = dk[n];
		if(dp == 0)
			break;
		if(strcmp(name, dp->name) == 0)
			error(Einuse);
	}
	if(n == Maxdk)
		error(Enoifc);

	/*
	 *  allocate both a dk structure and an array of pointers to line
	 *  structures
	 */
	dp = smalloc(sizeof(Dk));
	dp->ncsc = ncsc;
	dp->lines = lines;
	dp->linep = smalloc(sizeof(Line*) * dp->lines);
	strcpy(dp->name, name);
	dp->net.name = dp->name;
	dp->net.nconv = dp->lines;
	dp->net.devp = &dkinfo;
	dp->net.protop = &urpinfo;
	dp->net.listen = dklisten;
	dp->net.clone = dkcloneline;
	dp->net.ninfo = 5;
	dp->net.info[0].name = "local";
	dp->net.info[0].fill = dkfilladdr;
	dp->net.info[1].name = "remote";
	dp->net.info[1].fill = dkfillraddr;
	dp->net.info[2].name = "ruser";
	dp->net.info[2].fill = dkfillruser;
	dp->net.info[3].name = "urpstats";
	dp->net.info[3].fill = urpfillstats;
	dp->net.info[4].name = "status";
	dp->net.info[4].fill = dkfillstatus;
	dp->restart = restart;
	dp->urpwindow = window;
	dp->wq = WR(q);
	q->ptr = q->other->ptr = dp;
	dk[n] = dp;
	unlock(&dklock);
	poperror();

	/*
	 *  open csc here so that boot, dktimer, and dkcsckproc aren't
	 *  all fighting for it at once.
	 */
	dkopencsc(dp);

	/*
	 *  start a process to listen to csc messages
	 */
	sprint(buf, "csc.%s.%d", dp->name, dp->ncsc);
	kproc(buf, dkcsckproc, dp);

	/*
	 *  tell datakit we've rebooted. It should close all channels.
	 *  do this here to get it done before trying to open a channel.
	 */
	if(dp->restart) {
		DPRINT("dktimer: restart %s\n", dp->name);
		dp->closeall = 0;
		dkmesg(dp->csc, T_ALIVE, D_RESTART, 0, 0);
	}
	tsleep(&dp->closeallr, haveca, dp, 15000);

	/*
	 *  start a keepalive process
	 */
	sprint(buf, "timer.%s.%d", dp->name, dp->ncsc);
	kproc(buf, dktimer, dp);
}

void
dkreset(void)
{
	newqinfo(&dkmuxinfo);
}

void
dkinit(void)
{
}

Chan*
dkattach(char *spec)
{
	Chan *c;
	Dk *dp;
	int dev;

	/*
	 *  find a multiplexor with the same name (default dk)
	 */
	if(*spec == 0)
		spec = "dk";
	for(dev = 0; dev < Maxdk; dev++){
		dp = dk[dev];
		if(dp && strcmp(dp->name, spec) == 0)
			break;
	}
	if(dev == Maxdk)
		error(Enoifc);

	/*
	 *  return the new channel
	 */
	c = devattach('k', spec);
	c->dev = dev;
	return c;
}

Chan*
dkclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int	 
dkwalk(Chan *c, char *name)
{
	return netwalk(c, name, &dk[c->dev]->net);
}

void	 
dkstat(Chan *c, char *dp)
{
	netstat(c, dp, &dk[c->dev]->net);
}

Chan*
dkopen(Chan *c, int omode)
{
	Dk *dp;

	dp = dk[c->dev];
	linealloc(dp, STREAMID(c->qid.path), 1);
	return netopen(c, omode, &dp->net);
}

void	 
dkcreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c);
	USED(name);
	USED(omode);
	USED(perm);
	error(Eperm);
}

void	 
dkclose(Chan *c)
{
	if(c->stream)
		streamclose(c);
}

long	 
dkread(Chan *c, void *a, long n, ulong offset)
{
	return netread(c, a, n, offset, &dk[c->dev]->net);
}

long	 
dkwrite(Chan *c, void *a, long n, ulong offset)
{
	int t;
	char buf[256];
	char *field[5];
	int m;

	USED(offset);
	t = STREAMTYPE(c->qid.path);

	/*
	 *  get data dispatched as quickly as possible
	 */
	if(t == Sdataqid)
		return streamwrite(c, a, n, 0);

	/*
	 *  easier to do here than in dkoput
	 */
	if(t == Sctlqid){
		if(n > sizeof buf - 1)
			n = sizeof buf - 1;
		strncpy(buf, a, n);
		buf[n] = '\0';
		m = getfields(buf, field, 5, " ");
		if(strncmp(field[0], "connect", 7)==0){
			if(m < 2)
				error(Ebadarg);
			dkcall(Dial, c, field[1], 0, 0);
		} else if(strncmp(field[0], "announce", 8)==0){
			if(m < 2)
				error(Ebadarg);
			dkcall(Announce, c, field[1], 0, 0);
		} else if(strncmp(field[0], "redial", 6)==0){
			if(m < 4)
				error(Ebadarg);
			dkcall(Redial, c, field[1], field[2], field[3]);
		} else if(strncmp(field[0], "accept", 6)==0){
			if(m < 2)
				error(Ebadarg);
			dkanswer(c, strtoul(field[1], 0, 0), 0);
		} else if(strncmp(field[0], "reject", 6)==0){
			if(m < 3)
				error(Ebadarg);
			for(m = 0; m < DKERRS-1; m++)
				if(strcmp(field[2], dkerr[m]) == 0)
					break;
			dkanswer(c, strtoul(field[1], 0, 0), m);
		} else
			return streamwrite(c, a, n, 0);
		return n;
	}

	error(Eperm);
	return -1;		/* never reached */
}

void	 
dkremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void	 
dkwstat(Chan *c, char *dp)
{
	netwstat(c, dp, &dk[c->dev]->net);
}

/*
 *  return the number of an unused line (reserve it)
 */
static int
dkcloneline(Chan *c)
{
	Line *lp;
	Dk *dp;
	int line;

	dp = dk[c->dev];
	/*
	 *  get an unused device and open its control file
	 */
	qlock(&dp->netlock);
	for(line = dp->ncsc+1; line < dp->lines; line++){
		lp = dp->linep[line];
		if(lp == 0 || lp->state == Lclosed){
			lp = linealloc(dp, line, 0);
			lp->state = Lopened;

			/* current user becomes owner */
			netown(lp, u->p->user, 0);

			qunlock(&dp->netlock);
			return lp->lineno;
		}
	}
	qunlock(&dp->netlock);
	error(Enodev);
	return -1;		/* never reached */
}

static Chan*
dkopenline(Dk *dp, int line)
{
	Chan *c;

	c = 0;
	if(waserror()){
		if(c)
			close(c);
		nexterror();
	}
	c = dkattach(dp->name);
	c->qid.path = STREAMQID(line, Sdataqid);
	dkopen(c, ORDWR);
	poperror();

	return c;
}

/*
 *  open the common signalling channel (dp->csc's reference count never goes below 1)
 */
static Chan*
dkopencsc(Dk *dp)
{
	qlock(&dp->csclock);
	if(dp->csc == 0)
		dp->csc = dkopenline(dp, dp->ncsc);
	incref(dp->csc);
	qunlock(&dp->csclock);
	return dp->csc;
}

/*
 *  return the contents of the info files
 */
void
dkfilladdr(Chan *c, char *buf, int len)
{
	if(len < sizeof(dk[0]->linep[0]->addr)+2)
		error(Ebadarg);
	sprint(buf, "%s\n", dk[c->dev]->linep[STREAMID(c->qid.path)]->addr);
}
void
dkfillraddr(Chan *c, char *buf, int len)
{
	if(len < sizeof(dk[0]->linep[0]->raddr)+2)
		error(Ebadarg);
	sprint(buf, "%s\n", dk[c->dev]->linep[STREAMID(c->qid.path)]->raddr);
}
void
dkfillruser(Chan *c, char *buf, int len)
{
	if(len < sizeof(dk[0]->linep[0]->ruser)+2)
		error(Ebadarg);
	sprint(buf, "%s\n", dk[c->dev]->linep[STREAMID(c->qid.path)]->ruser);
}
void
dkfillstatus(Chan *c, char *buf, int len)
{
	Dk *dp;
	Line *lp;
	int line;
	char lbuf[65];

	line = STREAMID(c->qid.path);
	dp = dk[c->dev];
	lp = linealloc(dp, line, 1);
	sprint(lbuf, "%s/%d %d %s window %d\n", dp->name, line,
		lp->state != Lclosed ? 1 : 0, dkstate[lp->state], lp->window);
	strncpy(buf, lbuf, len);
}

/*
 *  send a message to the datakit on the common signaling line
 */
static int
dkmesg(Chan *c, int type, int srv, int p0, int p1)
{
	Dkmsg d;

	if(waserror()){
		print("dkmesg: error\n");
		return -1;
	}
	d.type = type;
	d.srv = srv;
	d.param0l = p0;
	d.param0h = p0>>8;
	d.param1l = p1;
	d.param1h = p1>>8;
	d.param2l = 0;
	d.param2h = 0;
	d.param3l = 0;
	d.param3h = 0;
	d.param4l = 0;
	d.param4h = 0;
	streamwrite(c, (char *)&d, sizeof(Dkmsg), 1);
	poperror();
	return 0;
}

/*
 *  call out on a datakit
 */
static int
calldone(void *a)
{
	Line *lp;

	lp = (Line *)a;
	return lp->state != Ldialing;
}
static void
dkcall(int type, Chan *c, char *addr, char *nuser, char *machine)
{
 	char dialstr[66];
	int line, win;
	char dialtone;
	int t_val, d_val;
	Dk *dp;
	Line *lp;
	Chan *dc;
	Chan *csc;
	char *bang, *dot;
	
	line = STREAMID(c->qid.path);
	dp = dk[c->dev];
	lp = linealloc(dp, line, 1);

	/*
	 *  only dial on virgin lines
	 */
	if(lp->state != Lopened)
		error(Ebadarg);

	DPRINT("dkcall(line=%d, type=%d, dest=%s)\n", line, type, addr);

	/*
	 *  build dial string
	 *	- guard against new lines
	 *	- change ! into . to delimit service
	 */
	if(strchr(addr, '\n'))
		error(Ebadarg);
	if(strlen(addr)+strlen(u->p->user)+2 >= sizeof(dialstr))
		error(Ebadarg);
	strcpy(dialstr, addr);
	bang = strchr(dialstr, '!');
	if(bang){
		dot = strchr(dialstr, '.');
		if(dot==0 || dot > bang)
			*bang = '.';
	}
	switch(type){
	case Dial:
		t_val = T_SRV;
		d_val = D_DIAL;
		strcat(dialstr, "\n");
		strcat(dialstr, u->p->user);
		strcat(dialstr, "\n");
		break;
	case Announce:
		t_val = T_SRV;
		d_val = D_SERV;
		break;
	case Redial:
		t_val = T_CHG;
		d_val = D_REDIAL;
		strcat(dialstr, "\n");
		strcat(dialstr, nuser);
		strcat(dialstr, "\n");
		strcat(dialstr, machine);
		strcat(dialstr, "\n");
		break;
	default:
		t_val = 0;
		d_val = 0;
		panic("bad dial type");
	}

	/*
	 *  open the data file
	 */
	dc = dkopenline(dp, line);
	if(waserror()){
		close(dc);
		nexterror();
	}
	lp->calltolive = 4;
	lp->state = Ldialing;

	/*
	 *  tell the controller we want to make a call
	 */
	DPRINT("dialout\n");
	csc = dkopencsc(dp);
	if(waserror()){
		close(csc);
		nexterror();
	}
	for(win = 0; ; win++)
		if(W_VALUE(win) >= dp->urpwindow || win == 15)
			break;
	dkmesg(csc, t_val, d_val, line, W_WINDOW(win, win, 2));
	poperror();
	close(csc);

	/*
	 *  if redial, wait for a dial tone (otherwise we might send
	 *  the dialstr to the previous other end and not the controller)
	 */
	if(type==Redial){
		if(streamread(dc, &dialtone, 1L) != 1L){
			lp->state = Lconnected;
			error(Ebadarg);
		}
	}

	/*
	 *  make the call
	 */
	DPRINT("dialstr %s\n", dialstr);
	streamwrite(dc, dialstr, (long)strlen(dialstr), 1);
	close(dc);
	poperror();

	/*
	 *  redial's never get a reply, assume it worked
	 */
	if(type == Redial) {
		lp->state = Lconnected;
		return;
	}

	/*
	 *  wait for a reply
	 */
	DPRINT("reply wait\n");
	sleep(&lp->r, calldone, lp);

	/*
	 *  if there was an error, translate it to a plan 9
	 *  errno and report it to the user.
	 */
	DPRINT("got reply %d\n", lp->state);
	if(lp->state != Lconnected) {
		if(lp->err >= DKERRS)
			error(dkerr[0]);
		else
			error(dkerr[lp->err]);
	}

	/*
	 *  change state if serving
	 */
	if(type == D_SERV){
		lp->state = Llistening;
	}
	DPRINT("connected!\n");

	/*
	 *  decode the window size
	 */
	if (W_VALID(lp->window)){
		/*
		 *  a 1127 window negotiation
		 */
		lp->window = W_VALUE(W_DEST(lp->window));
	} else if(lp->window>2 && lp->window<31){
		/*
		 *  a generic window negotiation
		 */
		lp->window = 1<<lp->window;
	} else
		lp->window = 0;

	/*
	 *  tag the connection
	 */
	strncpy(lp->addr, addr, sizeof(lp->addr)-1);
	strncpy(lp->raddr, addr, sizeof(lp->raddr)-1);

	/*
	 *  reset the protocol
	 */
	dkwindow(c);
}

/*
 *  listen for a call, reflavor the 
 */
static int
dklisten(Chan *c)
{
	char dialstr[512];
	char *line[12];
	char *field[8];
	Line *lp;
	Dk *dp;
	int n, lineno, ts, window;
	int from;
	Chan *dc;
	static int dts;
	char *cp;

	dp = dk[c->dev];
	from = STREAMID(c->qid.path);

	/*
	 *  open the data file
	 */
	dc = dkopenline(dp, STREAMID(c->qid.path));
	if(waserror()){
		close(dc);
		nexterror();
	}

	/*
	 *  wait for a call in
	 */
	for(;;){
		/*
		 *  read the dialstring and null terminate it
		 */
		n = streamread(dc, dialstr, sizeof(dialstr)-1);
		DPRINT("returns %d\n", n);
		if(n <= 0)
			error(Eio);
		dialstr[n] = 0;
		DPRINT("dialstr = %s\n", dialstr);

		/*
		 *  break the dial string into lines
		 */
		n = getfields(dialstr, line, 12, "\n");
		if (n < 2) {
			DPRINT("bad dialstr from dk (1 line)\n");
			error(Eio);
		}

		/*
		 * line 0 is `line.tstamp.traffic[.urpparms.window]'
		 */
		window = 0;
		switch(getfields(line[0], field, 5, ".")){
		case 5:
			/*
			 *  generic way of passing window
			 */
			window = strtoul(field[4], 0, 0);
			if(window > 0 && window <31)
				window = 1<<window;
			else
				window = 0;
			/*
			 *  intentional fall through
			 */
		case 3:
			/*
			 *  1127 way of passing window
			 */
			if(window == 0){
				window = strtoul(field[2], 0, 0);
				if(W_VALID(window))
					window = W_VALUE(W_ORIG(window));
				else
					window = 0;
			}
			break;
		default:
			print("bad message from dk(bad first line)\n");
			continue;
		}
		lineno = strtoul(field[0], 0, 0);
		if(lineno >= dp->lines){
			print("dklisten: illegal line %d\n", lineno);
			continue;
		}
		lp = linealloc(dp, lineno, 1);
		ts = strtoul(field[1], 0, 0);

		/*
		 *  this could be a duplicate request
		 */
		if(ts == lp->timestamp){
			if((dts++ % 1000) == 0)
				print("dklisten: repeat timestamp %d\n", lineno);
			if(lp->state != Lconnected)
				dkanswer(c, lineno, DKbusy);
			continue;
		}
	
		/*
		 *  take care of glare (datakit picked an inuse channel
		 *  for the call to come in on).
		 */
		if(!canqlock(lp)){
			DPRINT("DKbusy1\n");
			dkanswer(c, lineno, DKbusy);
			continue;
		} else {
			if(lp->state != Lclosed){
				qunlock(lp);
				DPRINT("DKbusy2 %ux\n", lp->state);
				dkanswer(c, lineno, DKbusy);
				continue;
			}
		}
		lp->window = window;

		/*
		 *  Line 1 is `my-dk-name.service[.more-things]'.
		 *  Special characters are escaped by '\'s.  Convert to
		 *  a plan 9 address, i.e. system!service.
		 */
		strncpy(lp->addr, line[1], sizeof(lp->addr)-1);
		if(cp = strchr(lp->addr, '.')){
			*cp = '!';
			if(cp = strchr(cp, '.'))
				*cp = 0;
		}
	
		/*
		 *  the rest is variable length
		 */
		switch(n) {
		case 2:
			/* no more lines */
			lp->ruser[0] = 0;
			lp->raddr[0] = 0;
			break;
		case 3:
			/* line 2 is `source.user.param1.param2' */
			getfields(line[2], field, 3, ".");
			strncpy(lp->raddr, field[0], sizeof(lp->raddr)-1);
			strncpy(lp->ruser, field[1], sizeof(lp->ruser)-1);
			break;
		case 4:
			/* line 2 is `user.param1.param2' */
			getfields(line[2], field, 2, ".");
			strncpy(lp->ruser, field[0], sizeof(lp->ruser)-1);
	
			/* line 3 is `source.node.mod.line' */
			strncpy(lp->raddr, line[3], sizeof(lp->raddr)-1);
			break;
		default:
			print("bad message from dk(>4 line)\n");
			qunlock(lp);
			error(Ebadarg);
		}

		DPRINT("src(%s)user(%s)dest(%s)w(%d)\n", lp->raddr, lp->ruser,
			lp->addr, W_TRAF(lp->window));

		lp->timestamp = ts;
		lp->state = Lconnected;

		/* listener becomes owner */
		netown(lp, dp->linep[from]->owner, 0);

		qunlock(lp);
		close(dc);
		poperror();
		DPRINT("dklisten returns %d\n", lineno);
		return lineno;
	}
	panic("dklisten terminates strangely\n");
	return -1;		/* never reached */
}

/*
 *  answer a call
 */
static void
dkanswer(Chan *c, int line, int code)
{
	char reply[64];
	Chan *dc;
	Line *lp;
	Dk *dp;

	dp = dk[c->dev];
	lp = linealloc(dp, line, 1);

	/*
	 *  open the data file (c is a control file)
	 */
	dc = dkattach(dp->name);
	if(waserror()){
		close(dc);
		nexterror();
	}
	dc->qid.path = STREAMQID(STREAMID(c->qid.path), Sdataqid);
	dkopen(dc, ORDWR);

	/*
	 *  send the reply
	 */
	sprint(reply, "%ud.%ud.%ud", line, lp->timestamp, code);
	DPRINT("dkanswer %s\n", reply);
	streamwrite(dc, reply, strlen(reply), 1);
	close(dc);
	poperror();

	/*
 	 *  set window size
	 */
	if(code == 0){
		if(waserror()){
			close(dc);
			nexterror();
		}
		sprint(reply, "init %d %d", lp->window, Streamhi);
		dc = dkopenline(dp, line);
		dc->qid.path = STREAMQID(line, Sctlqid);
		streamwrite(dc, reply, strlen(reply), 1);
		close(dc);
		poperror();
	}
}

/*
 *  set the window size and reset the protocol
 */
static void
dkwindow(Chan *c)
{
	char buf[64];
	Line *lp;

	lp = linealloc(dk[c->dev], STREAMID(c->qid.path), 1);
	if(lp->window == 0)
		lp->window = 64;
	sprint(buf, "init %d %d", lp->window, Streamhi);
	streamwrite(c, buf, strlen(buf), 1);
}

/*
 *  hangup a datakit connection
 */
static void
dkhangup(Line *lp)
{
	Block *bp;

	qlock(lp);
	if(lp->rq){
		bp = allocb(0);
		bp->type = M_HANGUP;
		PUTNEXT(lp->rq, bp);
	}
	qunlock(lp);
}

/*
 *  A process which listens to all input on a csc line
 */
static void
dkcsckproc(void *a)
{
	long n;
	Dk *dp;
	Dkmsg d;
	int line;

	dp = a;

	if(waserror()){
		close(dp->csc);
		return;
	}
	DPRINT("dkcsckproc: %d\n", dp->ncsc);

	/*
	 *  loop forever listening
	 */
	for(;;){
		n = streamread(dp->csc, (char *)&d, (long)sizeof(d));
		if(n != sizeof(d)){
			if(n == 0)
				error(Ehungup);
			print("strange csc message %d\n", n);
			continue;
		}
		line = (d.param0h<<8) + d.param0l;
		DPRINT("t(%d)s(%d)l(%d)\n", d.type, d.srv, line);
		switch (d.type) {

		case T_CHG:	/* controller wants to close a line */
			dkchgmesg(dp->csc, dp, &d, line);
			break;
		
		case T_REPLY:	/* reply to a dial request */
			dkreplymesg(dp, &d, line);
			break;
		
		case T_SRV:	/* ignore it, it's useless */
/*			print("dksrvmesg(%d)\n", line);		/**/
			break;
		
		case T_RESTART:	/* datakit reboot */
			if(line >=0 && line<dp->lines)
				dp->lines = line+1;
			break;
		
		default:
			print("unrecognized csc message %o.%o(%o)\n",
				d.type, d.srv, line);
			break;
		}
	}
}

/*
 *  datakit requests or confirms closing a line
 */
static void
dkchgmesg(Chan *c, Dk *dp, Dkmsg *dialp, int line)
{
	Line *lp;

	switch (dialp->srv) {

	case D_CLOSE:		/* remote shutdown */
		if (line <= 0 || line >= dp->lines || (lp = dp->linep[line]) == 0) {
			/* tell controller this line is not in use */
			dkmesg(c, T_CHG, D_CLOSE, line, 0);
			return;
		}
		switch (lp->state) {

		case Ldialing:
			/* simulate a failed connection */
			dkreplymesg(dp, (Dkmsg *)0, line);
			lp->state = Lrclose;
			break;

		case Lrclose:
		case Lconnected:
		case Llistening:
		case Lackwait:
			dkhangup(lp);
			lp->state = Lrclose;
			break;

		case Lopened:
			dkmesg(c, T_CHG, D_CLOSE, line, 0);
			break;

		case Llclose:
		case Lclosed:
			dkhangup(lp);
			dkmesg(c, T_CHG, D_CLOSE, line, 0);
			lp->state = Lclosed;
			break;
		}
		break;
	
	case D_ISCLOSED:	/* acknowledging a local shutdown */
		if (line <= 0 || line >= dp->lines || (lp = dp->linep[line]) == 0)
			return;
		switch (lp->state) {
		case Llclose:
		case Lclosed:
			lp->state = Lclosed;
			break;

		case Lrclose:
		case Lconnected:
		case Llistening:
		case Lackwait:
			break;
		}
		break;

	case D_CLOSEALL:
		/*
		 *  datakit wants us to close all lines
		 */
		for(line = dp->ncsc+1; line < dp->lines; line++){
			lp = dp->linep[line];
			if(lp == 0)
				continue;
			switch (lp->state) {
	
			case Ldialing:
				/* simulate a failed connection */
				dkreplymesg(dp, (Dkmsg *)0, line);
				lp->state = Lrclose;
				break;
	
			case Lrclose:
			case Lconnected:
			case Llistening:
			case Lackwait:
				lp->state = Lrclose;
				dkhangup(lp);
				break;
	
			case Lopened:
				break;
	
			case Llclose:
			case Lclosed:
				lp->state = Lclosed;
				break;
			}
		}
		dp->closeall = 1;
		wakeup(&dp->closeallr);
		break;

	default:
		print("unrecognized T_CHG\n");
	}
}

/*
 *  datakit replies to a dialout.  capture reply code and traffic parameters
 */
static void
dkreplymesg(Dk *dp, Dkmsg *dialp, int line)
{
	Line *lp;

	DPRINT("dkreplymesg(%d)\n", line);

	if(line < 0 || line >= dp->lines || (lp = dp->linep[line]) == 0)
		return;

	if(lp->state != Ldialing)
		return;

	if(dialp){
		/*
		 *  a reply from the dk
		 */
		lp->state = (dialp->srv==D_OPEN) ? Lconnected : Lrclose;
		lp->err = (dialp->param1h<<8) + dialp->param1l;
		lp->window = lp->err;
		DPRINT("dkreplymesg: %d\n", lp->state);
	} else {
		/*
		 *  a local abort
		 */
		lp->state = Lrclose;
		lp->err = 0;
	}

	if(lp->state==Lrclose){
		dkhangup(lp);
	}
	wakeup(&lp->r);
}

/*
 *  send a I'm alive message every 7.5 seconds and remind the dk of
 *  any closed channels it hasn't acknowledged.
 */
static void
dktimer(void *a)
{
	int i;
	Dk *dp;
	Line *lp;
	Chan *c;

	dp = (Dk *)a;
	c = dkopencsc(dp);

	while(waserror());

	for(;;){
		/*
		 * send keep alive
		 */
		DPRINT("keep alive\n");
		dkmesg(c, T_ALIVE, D_CONTINUE, 0, 0);

		/*
		 *  remind controller of dead lines and
		 *  timeout calls that take to long
		 */
		for (i=dp->ncsc+1; i<dp->lines; i++){
			lp = dp->linep[i];
			if(lp == 0)
				continue;
			switch(lp->state){
			case Llclose:
				dkmesg(c, T_CHG, D_CLOSE, i, 0);
				break;

			case Ldialing:
				if(lp->calltolive==0 || --lp->calltolive!=0)
					break;
				dkreplymesg(dp, (Dkmsg *)0, i);
				break;
			}
		}
		tsleep(&dp->timer, return0, 0, 7500);
	}
}
