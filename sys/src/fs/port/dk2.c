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
 *  like andrew's getmfields but no hidden state
 */
int
getfields(char *lp, char **fields, int n, char sep)
{
	int i;

	for(i=0; lp && *lp && i<n; i++){
		while(*lp == sep)
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp != sep)
			lp++;
	}
	return i;
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
	char *line[12];
	char *field[8];
	char dialstr[512];
	char reply[64];
	char *p, *error, *user, *addr, *source;
	int i, c, window, reason, timestamp;

	error = "SR bad format";
	reason = 7;
	user = "?";
	source = "?";

	/*
	 *  copy string to someplace we can alter
	 */
	i = n;
	if(i >= sizeof(dialstr)-1)
		i = sizeof(dialstr)-1;
	if(DEBUG) {
		memmove(dialstr, cp, i);
		dialstr[i] = 0;
		p = dialstr;
		while(p = strchr(p, '\n'))
			*p = '\\';
		print("srlisten: %s\n", dialstr);
	}
	memmove(dialstr, cp, i);
	dialstr[i] = 0;

	/*
	 *  break the dial string into lines
	 */
	n = getfields(dialstr, line, 12, '\n');

	/*
	 * line 0 is `channel.timestamp.traffic[.urpparms.window]'
	 */
	window = 0;
	switch(getfields(line[0], field, 5, '.')){
	case 5:
		/*
		 *  generic way of passing window
		 */
		window = number(field[4], 0, 10);
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
			window = number(field[2], 0, 10);
			if(W_VALID(window))
				window = W_VALUE(W_ORIG(window));
			else
				window = 0;
		}
		break;
	default:
		c = 0;
		timestamp = 0;
		goto out;
	}
	c = number(field[0], 0, 10);
	timestamp = number(field[1], 0, 10);

	/*
	 *  Line 1 is `my-dk-name.service[.more-things]'.
	 */
	addr = line[1];

	/*
	 *  the rest is variable length
	 */
	switch(n) {
	case 2:
		/* no more lines */
		break;
	case 3:
		/* line 2 is `source.user.param1.param2' */
		getfields(line[2], field, 3, '.');
		source = field[0];
		user = field[1];
		break;
	case 4:
		/* line 2 is `user.param1.param2' */
		getfields(line[2], field, 2, '.');
		user = field[0];
		source = line[3];
		break;
	default:
		goto out;
	}

	if(c < 0 || c >= NDK) {
		print("srlisten ctobig: addr=%s c=%d user=%s src=%s\n",
			addr, c, user, source);
		error = "SR for non-existant channel";
		goto out;
	}
	if(c <= CKCHAN){
		error = "SR for dedicated channel";
		goto out;
	}
	up = &dk->dkchan[c];
	DPRINT("C%d: %s/%d, %s, %s\n", up->dkp.cno, addr, c, user, source);

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
	urprinit(up);
	urpxinit(up, window);
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
	i = sprint(reply, "%d.%d.%d", c, timestamp, reason);

	up = &dk->dkchan[SRCHAN];
	dklock(dk, up);
	mb = mballoc(i, up, Mbdksrlisten2);
	memmove(mb->data, reply, mb->count);
	dkqmesg(mb, up);
	dkunlock(dk, up);
}
