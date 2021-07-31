#include	"all.h"
#include	"dk.h"

/*
 *  disconnect a dk channel, a CLOSED channel can do no I/O
 *	called with up locked
 */
void
xpinit(Chan *up)
{
	Msgbuf *mb;

	up->dkp.dkstate = CLOSED;
	while(up->dkp.outmsg) {
		mb = up->dkp.outmsg->next;
		mbfree(up->dkp.outmsg);
		up->dkp.outmsg = mb;
	}
	if(up->dkp.inmsg) {
		mbfree(up->dkp.inmsg);
		up->dkp.inmsg = 0;
	}
	up->dkp.repr = 0;
	up->dkp.repw = 0;
	up->dkp.mout = 0;
	up->dkp.nout = 0;
	up->dkp.mcount = 0;
	fileinit(up);
}

/*
 *  start up the common controller channel (done once)
 *	CCCHAN is NOT locked
 */
void
cccinit(Dk *dk)
{
	Chan *up;

	up = &dk->dkchan[CCCHAN];
	dklock(dk, up);
	up->dkp.dkstate = OPENED;
	urpinit(up);
	xpmesg(up, T_ALIVE, D_RESTART, 0, 0);
	dkunlock(dk, up);
}

/*
 *  send a message to CMC
 *	the message is queued on up and placed
 *	on CCCHAN when this chan is unlocked.
 *	up is locked
 */
void
xpmesg(Chan *up, int type, int srv, int p0, int p1)
{
	Dialin *dp;
	Msgbuf *mb;

	mb = mballoc(sizeof(Dialin), 0, Mbdkxpmesg);
	dp = (Dialin*)mb->data;

	dp->type = type;
	dp->srv = srv;
	dp->param0l = p0;
	dp->param0h = p0>>8;
	dp->param1l = p1;
	dp->param1h = p1>>8;
	dp->param2l = 0;
	dp->param2h = 0;
	dp->param3l = 0;
	dp->param3h = 0;
	dp->param4l = 0;
	dp->param4h = 0;

	mb->next = up->dkp.ccmsg;
	up->dkp.ccmsg = mb;
}

static
char	*msgs[] =
{
	"DK controller system error",
	"destination busy",
	"remote node not answering",
	"destination not answering",
	"unassigned destination",
	"DK system overload",
	"server already exists",
	"call rejected by destination",
};


/*
 * parse message from CMC
 *	nothing locked
 */
void
xplisten(Dk *dk, char *cp, int n)
{
	Dialin *dialp;
	Chan *up;
	char *error;
	int i, j, p1;

	if(n < sizeof(Dialin)) {
		print("xplisten: n=%d\n", n);
		return;
	}
	error = 0;
	dialp = (Dialin*)cp;
	i = (dialp->param0h << 8) + dialp->param0l;
	p1 = (dialp->param1h << 8) + dialp->param1l;

	up = 0;
	if(i > CCCHAN && i < NDK) {
		up = &dk->dkchan[i];
		dklock(dk, up);
	}

	DPRINT("xplisten type=%d, srv=%d, chan=%d, p1=%d\n",
		dialp->type, dialp->srv, i, p1);

	switch(dialp->type) {
	case T_REPLY:	/* a solicited reply */
		if(!up || up->dkp.dkstate != DIALING) {
			error = "T_REPLY for non-existant or idle channel";
			goto out;
		}
		switch (dialp->srv) {
		case D_OPEN:
			up->dkp.dkstate = OPENED;
			urpxinit(up, p1);
			break;

		default:
			if(p1 > 7)
				p1 = 0;
			print("channel %d call failed, %s\n", up->dkp.cno, msgs[p1]);
			xpinit(up);
			xpmesg(up, T_CHG, D_CLOSE, i, 0);
			break;
		}
		break;

	case T_CHG:	/* something has changed */
		if(!up && dialp->srv != D_CLOSEALL) {
			error = "T_CHG for non-existant or idle channel";
			goto out;
		}
		switch(dialp->srv) {
		case D_ISCLOSED:	/* acknowledging a local shutdown */
			CPRINT("D_ISCLOSED %d\n", i);
			xpinit(up);
			break;
		case D_CLOSE:		/* remote shutdown */
			CPRINT("T_CHG, D_CLOSE, %d\n", i);
			xpinit(up);
			xpmesg(up, T_CHG, D_CLOSE, i, 0);
			break;
		case D_CLOSEALL:	/* close every channel, except CCCHAN */
			print("T_CHG D_CLOSEALL: (type=%d, srv=%d), chan=%d, p1=%d\n",
				dialp->type, dialp->srv, i, p1);
			if(up)
				dkunlock(dk, up);
			for(j=SRCHAN; j<NDK; j++) {
				up = &dk->dkchan[j];
				dklock(dk, up);
				xpinit(up);
				dkunlock(dk, up);
			}
			up = 0;
			break;
		default:
			error="unrecognized T_CHG message";
			break;
		}
		break;

	case T_RESTART:
		DPRINT("T_RESTART: (type=%d), srv=%d, chan=%d, p1=%d\n",
			dialp->type, dialp->srv, i, p1);
		break;
	default:
		error = "unrecognized csc message";
	}
out:
	if(error)
		print("xplisten: error %s\n\ttype=%d, srv=%d, chan=%d, p1=%d\n",
			error, dialp->type, dialp->srv, i, p1);
	if(up)
		dkunlock(dk, up);
}

/*
 *  make a call or an announcement
 *	called with up locked
 */
void
xpcall(Chan *up, char *dialstr, int announce)
{
	Msgbuf *mb;

	/*
	 *  tell datakit we're calling
	 */
	print("dialing: %s\n", dialstr);
	xpmesg(up, T_SRV, announce? D_SERV: D_DIAL,
		up->dkp.cno, W_WINDOW(WSIZE,WSIZE,2));

	/*
	 *  call out on channel
	 */
	urpinit(up);
	up->dkp.dkstate = DIALING;
	mb = mballoc(strlen(dialstr), up, Mbdkxpcall);
	memmove(mb->data, dialstr, mb->count);
	dkqmesg(mb, up);
	up->dkp.calltimeout = DKTIME(120);
}

/*
 *  hang up a call
 *	called with up locked
 */
void
xphup(Chan *up)
{
	up->dkp.dkstate = LCLOSE;
	xpmesg(up, T_CHG, D_CLOSE, up->dkp.cno, 0);
	up->dkp.timeout = DKTIME(20);
}

/*
 *  listen to service (connection) requests
 *	nothing locked
 */
void
srlisten(Dk *dk, char *cp, int n)
{
	Chan *up;
	Msgbuf *mb;
	char *p, *error, *addr, *user, source[100];
	int i, c, clock, traffic, reason, wins;

	if(DEBUG) {
		i = n;
		if(i >= sizeof(source)-1)
			i = sizeof(source)-1;
		memmove(source, cp, i);
		source[i] = 0;
		p = source;
		while(p = strchr(p, '\n'))
			*p = '\\';
		print("srlisten: %s\n", source);
	}

	p = cp;
	cp += n;	/* end pointer */
	error = "bad format";
	clock = 0;	/* set */
	reason = 7;

	c = number(p, 0, 10);
	if(!(p = strchr(p, '.')) || p >= cp)
		goto out;

	p++;
	clock = number(p, 0, 10);
	if(!(p = strchr(p, '.')) || p >= cp)
		goto out;

	p++;
	traffic = number(p, 0, 10);
	if(!(p = strchr(p, '\n')) || p >= cp)
		goto out;

	p++;
	addr = p;
	if(!(p = strchr(p, '\n')) || p >= cp)
		goto out;

	*p++ = 0;
	user = p;
	if(!(p = strchr(p, '\n')) || p >= cp)
		goto out;

	*p++ = 0;
	i = cp - p;
	if(i > sizeof(source)-1)
		i = sizeof(source)-1;
	memmove(source, p, i);
	source[i] = 0;
	if(c < 0 || c >= NDK) {
		print("srlisten ctobig: addr=%s c=%d user=%s src=%s\n",
			addr, c, user, source);
		error = "SR for non-existant channel";
		goto out;
	}

	up = &dk->dkchan[c];
	DPRINT("C%d: %s/%d, %s, %s\n", up->dkp.cno, addr, c, user, source);
	if(c <= CKCHAN){
		error = "SR for dedicated channel";
		goto out;
	}

	dklock(dk, up);
	if(up->dkp.dkstate != CLOSED) {
		error = "SR for active channel";
		dkunlock(dk, up);
		goto out;
	}
	reason = 0;
	error = 0;
	strncpy(up->whoname, user, sizeof(up->whoname));
	strncpy(up->whochan, source, sizeof(up->whochan));
	wins = 0;
	if(W_VALID(traffic))
		wins = W_VALUE(W_ORIG(traffic));
	urprinit(up);
	urpxinit(up, wins);
	up->dkp.dkstate = OPENED;

	/*
	 *  The evil OK
	 */
	p = "OK";
	mb = mballoc(strlen(p), up, Mbdksrlisten1);
	memmove(mb->data, p, mb->count);
	dkqmesg(mb, up);
	dkunlock(dk, up);

out:
	if(error)
		print("call rejected, %s\n", error);
	sprint(source, "%d.%d.%d", c, clock, reason);

	up = &dk->dkchan[SRCHAN];
	dklock(dk, up);
	mb = mballoc(strlen(source), up, Mbdksrlisten2);
	memmove(mb->data, source, mb->count);
	dkqmesg(mb, up);
	dkunlock(dk, up);
}
