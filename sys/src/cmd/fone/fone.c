#include "all.h"

int	debug;
int	initflag;
int	apiflag;
int	nomute;
char *	dialcmd;
char *	dquerycmd;
int	Startquery = 8;	/* default for 7506 set */
int	Nextquery = 9;

char *	srvname;
int	pipefd[2];
int	ttypid;
int	alarmpid;
int	fonepid;
int	fonefd;
int	fonectlfd;

char *	logname = "call.log";
int	logfd = -1;

#define	NCALLS	26		/* number of appearances */

Call	calls[NCALLS+1];	/* 1 extra */
Call *	callsN = &calls[NCALLS];
Call *	callsNout = &calls[4];

char	display[2][64];

int	switchhook;
char *	dialstr;
ulong	dialflags;
Call *	dialother;
char	dialbuf[64];

void
main(int argc, char **argv)
{
	Biobufhdr bpp;
	uchar buf[512]; char *l;
	int n;

	/*peek();/**/
	ARGBEGIN{
	case 'a':
		setapiflag(1);
		break;
	case 'm':
		++nomute;
		break;
	case 'n':
		n = strtoul(ARGF(), 0, 0);
		if(n < NCALLS)
			callsNout = &calls[n+1];
		break;
	case 'f':
		fname = ARGF();
		break;
	case 'F':
		srvname = ARGF();
		break;
	case 't':
		telcmd = ARGF();
		break;
	case 'l':
		logname = ARGF();
		break;
	case 's':
		srvname = dsrvname;
		break;
	case 'D':
		++debug;
		break;
	}ARGEND
	USED(argc, argv);
	plumb();
	fonepid = readfone();
	alarmpid = readalarm();
	ttypid = readtty();
	close(0);
	close(pipefd[1]);
	logfd = open(logname, OWRITE);
	if(logfd >= 0){
		if(seek(logfd, 0, 2) < 0){
			perror(logname);
			close(logfd);
			logfd = -1;
		}
	}
	Binits((Biobuf *)&bpp, pipefd[0], OREAD, buf, sizeof buf);
	foneinit();
	while(l = Brdline((Biobuf *)&bpp, '\n')){	/* assign = */
		n = BLINELEN((Biobuf *)&bpp);
		if(n <= 0){
			perror("pipe");
			exits("read");
		}
		if(n < 2)
			break;
		l[n-1] = 0;
		switch(l[0]){
		case 't':
			dotty(l+1);
			break;
		case 'q':
			goto out;
		case 'f':
			if(dofone(l+1) < 0)
				goto out;
			break;
		case 'a':
			doalarm();
			break;
		}
	}
out:
	killproc(fonepid);
	killproc(alarmpid);
	killproc(ttypid);
	exits(0);
}

void
dotty(char *buf)
{
	char tonestr[32];
	int id, nactive, smart, slot;
	Call *cp;

	if(debug){
		char *t = ascnow();
		t[19] = 0;
		print("%s t %s\n", &t[11], buf);
	}
	while(*buf == ' ' || *buf == '\t')
		buf++;
	if(*buf == 0)
		return;
	if(!initflag && buf[0] != 'I'){
		print("Telephone not responding, restart or try `I'\n");
		return;
	}
	switch(buf[0]){
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		if(!actcall()){
			smart = 1;
			goto Dialit;
		}
		/* fall through */
	case '*': case '#':
	case '.':
		if(keyparse(tonestr, buf, sizeof tonestr))
			putcmd("AT%s%s\r", dialcmd, tonestr);
		else
			print("can't parse touchtone string\n");
		break;
	case '/':
		smart = 1;
		goto Dialit;
	case 'C':
	case 'c':
		if(!buf[1])
			break;
		smart = (*buf++ == 'c');
	Dialit:
		if(*buf == '/'){
			buf++;
			if(*buf >= 'a' && *buf <= 'z')
				slot = *buf++ - 'a' + 1;
			else
				slot = strtoul(buf, &buf, 10);
		}else
			slot = 0;
		if(!dialparse(smart, dialbuf, buf, sizeof dialbuf)){
			print("can't parse dialstring\n");
			break;
		}
		cp = outcall(slot);
		if(cp == 0){
			print(slot>0 ? "call slot %d not available\n" :
				"no call slots available\n", slot);
			break;
		}
		cp->dialp = dialbuf;
		if(debug)
			print("id=%d, dial=%s\n", cp->id, cp->dialp);
		if(cp->state == Null){
			putcmd("AT*B%c\r", '1' + cp->id - 1);
			cp->flags |= Spkrdial;
		}else
			dialcall(cp);
		break;
	case 'a':
		putcmd("AT*A\r");
		break;
	case 'd':
		cp = actcall();
		if(apiflag){
			if(!cp || cp->confcount == 0)
				putcmd("AT*H\r");	/* hangup */
			else
				putcmd("AT*J\r");	/* DROP button */
		}else{
			if((!cp || cp->confcount == 0) && (switchhook&~Mute) == Speaker)
				putcmd("ATT*BS\r");	/* SPEAKER button */
			else
				putcmd("ATT*J\r");	/* DROP button */
		}
		break;
	case 'h':
		putcmd("AT*W\r");
		break;
	case 'I':
		foneinit();
		break;
	case 'k':
		xfrconf(Confdial, 'K', buf+1);
		break;
	case 'L':
		putcmd("AT&&L\r");
		break;
	case 'q':
		querystart(buf+1);
		break;
	case 'r':
		id = strtol(buf+1,0,10);
		for(cp=calls; cp<callsN; cp++){
			if(cp->state != Suspended)
				continue;
			if(id == 0 || cp->id == id)
				break;
		}
		if(cp >= callsN){
			print("no suspended calls\n");
			break;
		}
		putcmd("AT*B%c\r", '1' + cp->id - 1);
		break;
	case 's':
		nactive = 0;
		for(cp=calls; cp<callsN; cp++)
			if(cp->state != Null){
				++nactive;
				showstate(cp);
			}
		if(nactive == 0)
			print("no active calls\n");
		break;
	case 't':
		telparse(buf+1);
		break;
	case '!':
		shcmd(buf+1);
		break;
	case 'T':
		if(apiflag)
			setclock();
		else
			print("Can't set clock.\n");
		break;
	case 'x':
		xfrconf(Xfrdial, 'T', buf+1);
		break;
	case '?':
		print("%s", helptext);
		break;
	default:
		print("unknown command, type ? for help\n");
		break;
	}
}

void
xfrconf(ulong flag, int code, char *buf)
{
	Call *act, *cp;

	act=actcall();
	if(!act){
		print("no active call\n");
		return;
	}
	for(cp=calls; cp<callsN; cp++){
		if(cp->flags & flag)
			break;
	}
	if(cp >= callsN && !*buf){
		print("dialstring required\n");
		return;
	}
	if(cp < callsN && *buf){
		print("dialstring illegal\n");
		return;
	}
	if(*buf){
		if(!dialparse(1, dialbuf, buf, sizeof dialbuf)){
			print("can't parse dialstring\n");
			return;
		}
		dialstr = dialbuf;
		dialflags = flag;
		dialother = act;
	}
	putcmd("AT*%c\r", code);
}

int
dofone(char *buf)
{
	static int trouble;
	int msg, type, pattern, r, c;
	char *p, *q;
	Call *cp;

	switch(*buf){
	case 'R':
		msg = Red_lamp;
		buf += 2;
		break;
	case 'G':
		msg = Green_lamp;
		buf += 2;
		break;
	case '0':
		p = 0;
		if(memcmp(buf, "03- 750", 7) == 0)
			p = buf+7;
		else if(memcmp(buf+1, "01-N- 750", 9) == 0)
			p = buf+9;
		if(p && (*p == '5' || *p == '6' || *p == '7')){
			if(memcmp(buf+strlen(buf)-5, " PASS", 5) == 0){
				msg = API_phone;
				if(*p == '7'){
					Startquery = 33;
					Nextquery = 34;
				}
				break;
			}
		}
		/* fall through */
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8':
		msg = msgcode(&buf);
		break;
	case '9':
	case 'P':
		if(memcmp(buf, "969", 3) == 0 ||
		   memcmp(buf, "ProPhone", 8) == 0){
			msg = Pro_phone;
			break;
		}
		/* fall through */
	default:
		if(++trouble > 3 || debug)
			showmsg(-1, buf);
		if(trouble > 3){
			print("couldn't init telephone, please try again\n");
			return -1;
		}
		foneinit();
		return 0;
	}
	if(debug)
		showmsg(msg, buf);
	switch(msg){
	case Pro_phone:
		if(buf[0] == 'P')
			print("%s\n", buf);
		initflag = 1;
		setapiflag(0);
		break;
	case API_phone:
		print("API %s\n", buf);
		initflag = 1;
		setapiflag(1);
		putcmd("AT&D3\r");
		putcmd("AT%%A0=3\r");
		break;
	case OK:
		cmdpending = 0;
		sendcmd();
		break;
	case Red_lamp:
		print("Red: %s\n", buf);
		break;
	case Green_lamp:
		print("Green: %s\n", buf);
		break;
	case Connected:
		cp = getcall(&buf);
		if(cp->assoc == Asc_setup){
			cp->assoc = Asc_connect;
			logstate(cp, "[connected]");
			break;
		}
		cp->state = Active;
		logstate(cp, "connected");
		break;
	case Ring:
		cp = getcall(&buf);
		incall(cp);
		break;
	case Cleared:
		if(*buf == 0)
			break;
		cp = getcall(&buf);
		logstate(cp, "disconnected");
		memset(cp, 0, sizeof(Call));
		cp->state = Null;
		break;
	case Error:
		print("error\n");
		cmdpending = 0;
		sendcmd();
		break;
	case Delivered:
		cp = getcall(&buf);
		cp->state = Call_delivered;
		break;
	case Display:
		dodisplay(buf);
		break;
	case Busy:
		break;
	case Feature:
		break;
	case Proceeding:
		cp = getcall(&buf);
		cp->state = Out_proceeding;
		break;
	case Progress:
		break;
	case Prompt:
		cp = getcall(&buf);
		cp->state = Overlap_send;
		if(cp->dialp){
			putcmd("AT%s%s\r", dialcmd, cp->dialp);
			cp->dialp = 0;
			break;
		}
		break;
	case Signal:
		cp = getcall(&buf);
		pattern = hexcode(&buf);
		if(cp->state == Null && pattern == 0x40)
			incall(cp);
		break;
	case Outcall:
		cp = getcall(&buf);
		cp->state = Overlap_send;
		cp->assoc = 0;
		memset(cp->num, 0, sizeof cp->num);
		strcpy(cp->who, "Outgoing call");
		memset(cp->tty, 0, sizeof cp->tty);
		if(!cp->dialp){
			if(!dialstr)
				break;	/* sic */
			cp->dialp = dialstr;
			cp->flags = dialflags;
			cp->other = dialother;
			dialstr = 0;
			dialflags = 0;
			dialother = 0;
		}
		if(cp->flags & Spkrdial){
			cp->flags &= ~Spkrdial;
			if(!nomute){
				if(apiflag)
					apibutton(B_Mute, 0);
				else
					putcmd("AT*BM\r");
			}
		}
		dialcall(cp);
		break;
	case Completed:
		cp = getcall(&buf);
		switch(hexcode(&buf)){
		case 0x02:	/* added to conference */
		case 0x90:	/* ProPhone */
			if(cp->other)
				cp->other->confcount++;
			break;
		case 0x03:	/* dropped from conference */
		case 0x98:	/* ProPhone */
			cp->confcount--;
			break;
		case 0x00:	/* on hold */
		case 0x9B:	/* ProPhone */
			logstate(cp, "suspended");
			cp->state = Suspended;
			break;
		case 0x01:	/* reconnected */
		case 0x9E:	/* ProPhone */
			logstate(cp, "reconnected");
			cp->state = Active;
			break;
		}
		break;
	case Rejected:
		print("request rejected\n");
		break;
	case ATT_lamp:
		break;
	case ATT_tones:
		break;
	case Switchhook:
		type = hexcode(&buf);
		switch(type){
		case Sw_Handset:
			switchhook = Handset;
			break;
		case Sw_Handset|Off:
			switchhook &= ~Handset;
			break;
		case Sw_Adjunct:
			switchhook |= Adjunct;
			break;
		case Sw_Adjunct|Off:
			switchhook &= ~Adjunct;
			break;
		case Sw_Speaker:
		case Sw_Remote:
			switchhook |= Speaker;
			switchhook &= ~Mute;
			break;
		case Sw_Speaker|Off:
		case Sw_Remote|Off:
			switchhook &= ~Speaker;
			break;
		case Sw_Spokesman:
			switchhook |= Spokesman;
			break;
		case Sw_Spokesman|Off:
			switchhook &= ~Spokesman;
			break;
		case Sw_Mute:
			if(switchhook&Speaker)
				switchhook |= Mute;
			break;
		case Sw_Mute|Off:
			if(switchhook&Speaker)
				switchhook &= ~Mute;
			break;
		}
		break;
	case Disconnect:
		cp = getcall(&buf);
		cp->state = Release_req;
		break;
	case L3_error:
		break;
	case ATT_display:
		break;
	case Associated:
		cp = getcall(&buf);
		if(!apiflag)
			hexcode(&buf);
		type = hexcode(&buf);
		switch(type){
		case Asc_setup:
			cp->state = Suspended;
			cp->assoc = type;
			logstate(cp, "[setup]");
			break;
		case Asc_connect:
			if(cp->state == Call_received)
				cp->state = Suspended;
			cp->assoc = type;
			logstate(cp, "[connected]");
			break;
		case Asc_hold:
			cp->assoc = type;
			logstate(cp, "[hold]");
			break;
		case Asc_reconn:
			cp->assoc = type;
			logstate(cp, "[reconnected]");
			break;
		case Asc_excl:
			logstate(cp, "exclusion");
			break;
		case Asc_noconn:
			cp->state = Suspended;
			logstate(cp, "connection denied");
			break;
		case Asc_noclear:
			cp->state = Suspended;
			logstate(cp, "clearing denied");
			break;
		}
		break;
	case ATT_cause:
		break;
	case ATT_HI:
		type = hexcode(&buf);
		r = hexcode(&buf);
		if(type == 0x11 && 1<= r && r <= 2){
			q = display[r-1];
			c = buf[2];
			p = buf+3;
			while(*p){
				if(*p != c)
					*q++ = *p++;
				else if(p[1] == c)
					*q++ = c, p+=2;
				else
					break;
			}
			*q = 0;
			if(r == 2)
				clockcruft();
		}
		break;
	}
	return 0;
}

void
dialcall(Call *cp)
{
	switch(*cp->dialp){
	case '9':
		putcmd("AT%s%c\r", dialcmd, *cp->dialp++);
		break;
	default:
		putcmd("AT%s%s\r", dialcmd, cp->dialp);
		cp->dialp = 0;
		break;
	}
}

void
dodisplay(char *buf)
{
	int id;
	char ibuf[4];
	char *ib = ibuf;
	static Call *cp;
	char *p = 0, *q;

	if(apiflag){
		hexcode(&buf);
		ibuf[0] = *buf++;
		ibuf[1] = *buf++;
		ibuf[2] = 0;
		id = hexcode(&ib);
	}else{
		id = hexcode(&buf);
		hexcode(&buf);
	}
	while(*buf == ' ')
		++buf;
	switch(id){
	case Call_appear_id:
		cp = getcall(&buf);
		break;
	case Called_id:
	case Calling_id:
		if(cp)
			p = cp->num;
		break;
	case Calling_name:
		if(cp)
			p = cp->who;
		break;
	case Isdn_call_id:
		if(cp)
			p = cp->tty;
		break;
	case Dir_info:
		queryinfo(buf);
		break;
	case Date_Time:
		break;
	}
	if(!(cp && p))
		return;
	q = &buf[strlen(buf)];
	while(*--q == ' ')
		*q = 0;
	if(*buf && strcmp(p, buf) != 0){
		strcpy(p, buf);
		cp->flags |= Altered;
		if(*cp->tty)
			logstate(cp, 0);
	}
}

static int	clockset;

void
setclock(void)
{
	int i;

	apibutton(B_Normal, 1);
	apibutton(B_Select, 1);
	apibutton(B_Clock, 1);
	for(i=0; i<6; i++)
		apibutton(B_Keypad|'#', i<5);
	putcmd("AT&&X0,0,3\r");
	clockset = 1;
}

static void
showtm(Tm *tm)
{
	print("%2d/%.2d/%d %2d:%.2d:%.2d\n",
		tm->mon+1, tm->mday, tm->year%100,
		tm->hour, tm->min, tm->sec);
}

static void
clkadvance(int n, int mod)
{
	if(n < 0)
		n += mod;
	while(--n >= 0)
		apibutton(B_Keypad|'*', 1);
	apibutton(B_Keypad|'#', 1);
}

void
clockcruft(void)
{
	static char *mnames[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
		0,
	};
	static int mdays[] = {
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};
	int i;
	Tm *tm, ctm;
	char *p;

	print("%s\n%s\n", display[0], display[1]);
	if(clockset++ != 1){
		clockset = 0;
		return;
	}
	i = time(0);
	tm = Localtime(i);
	/*showtm(tm);*/
	if(tm->sec >= 30)
		tm = Localtime(i+30);
	p = display[0];
	while(*p == ' ')
		p++;
	for(i=0; mnames[i]; i++)
		if(memcmp(p, mnames[i], 3) == 0)
			break;
	if(!mnames[i])
		return;
	memset(&ctm, 0, sizeof ctm);
	ctm.mon = i;
	ctm.mday = strtol(p+3, &p, 10);
	if(!*p)
		return;
	ctm.year = strtol(p+1, &p, 10)-1900;
	ctm.hour = strtol(p, &p, 10);
	if(ctm.hour == 12)
		ctm.hour = 0;
	if(!*p)
		return;
	ctm.min = strtol(p+1, &p, 10);
	while(*p == ' ')
		p++;
	if(*p == 'P')
		ctm.hour += 12;
	else if(*p != 'A')
		return;
	if(p[1] != 'M')
		return;
	/*showtm(&ctm);*/
	apibutton(B_Normal, 1);
	apibutton(B_Select, 1);
	apibutton(B_Clock, 1);
	clkadvance(tm->mon - ctm.mon, 12);
	if(ctm.mday > mdays[tm->mon])
		ctm.mday = mdays[tm->mon];
	clkadvance(tm->mday - ctm.mday, mdays[tm->mon]);
	clkadvance(tm->year - ctm.year, 25);
	clkadvance(tm->hour - ctm.hour, 24);
	clkadvance(tm->min - ctm.min, 60);
	apibutton(B_Keypad|'#', 0);
	putcmd("AT&&X0,0,3\r");
}

void
apibutton(int code, int keepem)
{
	static int ctrl[8];
	int group, number;

	group = (code&B_Group)>>8;
	number = code&B_Number;
	if(!ctrl[group]){
		ctrl[group] = 1;
		putcmd("AT&&X0,%d,1,0\r", group);
	}
	if(group == (B_Keypad>>8) && number > 9){
		putcmd("AT&&X%d,%c,1\r", group, number);
		putcmd("AT&&X%d,%c,0\r", group, number);
	}else{
		putcmd("AT&&X%d,%d,1\r", group, number);
		putcmd("AT&&X%d,%d,0\r", group, number);
	}
	if(keepem)
		return;
	for(group=0; group<nelem(ctrl); group++)
		if(ctrl[group]){
			ctrl[group] = 0;
			putcmd("AT&&X0,%d,0,0\r", group);
		}
}

void
logstate(Call *cp, char *p)
{
	char *t, buf[128], *bp;
	int n;

	if(p == 0 && !(cp->flags & Altered))
		return;
 	cp->flags &= ~Altered;
	t = ascnow();
	t[10]=t[19]=0;

	bp = buf;
	bp += sprint(bp, "%s:\t%s %s\t%d",
		p ? p : cp->who, &t[4], &t[11], cp->id);
	if(cp->tty[0] || cp->num[0])
		bp += sprint(bp, "\t%s", cp->tty);
	if(cp->num[0])
		bp += sprint(bp, "\t%s", cp->num);
	bp += sprint(bp, "\n");
	n = bp - buf;
	write(1, buf, n);
	if(logfd >= 0)
		write(logfd, buf, n);
}

void
incall(Call *cp)
{
	cp->state = Call_received;
	cp->assoc = 0;
	memset(cp->num, 0, sizeof cp->num);
	strcpy(cp->who, "Incoming call");
	memset(cp->tty, 0, sizeof cp->tty);
}

Call *
getcall(char **bufp)
{
	Call *cp;
	int id;

	id = hexcode(bufp);
	cp = &calls[id];
	if(cp >= callsN){
		fprint(2, "bogus id %d\n", id);
		cp = callsN;
	}
	cp->id = id;
	return cp;
}

Call *
outcall(int slot)
{
	Call *cp;

	if(slot > 0){
		cp = &calls[slot];
		if(cp >= callsNout)
			return 0;
		if(cp->state == Null || (cp->state==Overlap_send && !cp->dialp))
			goto found;
		return 0;
	}
	for(cp=calls+1; cp<callsNout; cp++)
		if(cp->state==Null || (cp->state==Overlap_send && !cp->dialp))
			goto found;
	return 0;
found:
	cp->id = cp-calls;
	return cp;
}

Call *
actcall(void)
{
	Call *cp;

	for(cp=calls; cp<callsN; cp++)
		if(cp->state==Active)
			return cp;
	return 0;
}

void
showmsg(int msg, char *buf)
{
	char *t;
	Mname *m;

	t = ascnow();
	t[19] = 0;
	for(m=M_Messages; m->name; m++)
		if(msg == m->msg)
			break;
	if(m->name)
		print("%s %s %s\n", &t[11], m->name, buf);
	else
		print("%s [%.2ux] %s\n", &t[11], msg, buf);
}

void
showstate(Call *cp)
{
	Mname *m;

	for(m=M_Callstates; m->name; m++)
		if(cp->state == m->msg)
			break;
	print("id: %d\t%-15s\t%s\t%s\t%s\n",
		cp->id, m->name ,cp->who, cp->tty, cp->num);
}

void
doalarm(void)
{}

void
setapiflag(int v)
{
	apiflag = v;
	dialcmd = apiflag ? "*D/" : "*D";
	dquerycmd = apiflag ? "*D00/" : "*D";
}

void
foneinit(void)
{
	if(debug)
		print("foneinit...");
	flushcmd();
	fprint(fonefd, "ATE0V0&D0\r");
	msleep(500);
	initflag = 0;
	putcmd("AT&&II3\r");
}

int
readfone(void)
{
	int pid, n;
	char *p;
	Biobufhdr in;
	uchar buf[512];

	openfone();
	switch(pid=fork()){
	default:
		return pid;
	case -1:
		perror("fork");
		exits("fork");
	case 0:
		break;
	}
	close(0);
	close(1);
	close(pipefd[0]);
	Binits((Biobuf *)&in, fonefd, OREAD, buf, sizeof buf);
	while(p = Brdline((Biobuf *)&in, '\r')){	/* assign = */
		n = BLINELEN((Biobuf *)&in);
		while(n > 0 && (p[n-1] == '\n' || p[n-1] == '\r'))
			--n;
		while(n > 0 && (*p == '\n' || *p == '\r'))
			--n, ++p;
		if(n <= 0)
			continue;
		p[n] = 0;
		if(fprint(pipefd[1], "f%s\n", p) < 0){
			perror("pipe");
			_exits("write");
		}
	}
	_exits(0);
	return 1;		/* dummy to shut up compiler */
}

int
readalarm(void)
{
	int pid;
	long talarm = 1000;

	switch(pid=fork()){
	default:
		return pid;
	case -1:
		perror("fork");
		exits("fork");
	case 0:
		break;
	}
	close(0);
	close(1);
	close(pipefd[0]);
	for(;;){
		msleep(talarm);
		if(write(pipefd[1], "a\n", 2) < 0)
			break;
	}

	_exits(0);
	return 1;		/* dummy to shut up compiler */
}

int
readtty(void)
{
	int pid, n;
	char buf[256];

	switch(pid=fork()){
	default:
		return pid;
	case -1:
		perror("fork");
		exits("fork");
	case 0:
		break;
	}
	close(1);
	close(pipefd[0]);
	buf[0] = 't';
	for(;;){
		n = read(0, buf+1, sizeof buf-1);
		if(n < 0)
			fprint(2, "%s: can't read /fd/0: %r\n", argv0);
		if(n <= 0){
			buf[0] = 'q';
			buf[1] = '\n';
			n = 1;
		}
		if(write(pipefd[1], buf, n+1) < 0){
			perror("pipe");
			_exits("write");
		}
		if(buf[0] == 'q')
			break;
	}
	_exits(0);
	return 1;		/* dummy to shut up compiler */
}

void
shcmd(char *cmd)
{
	char *q;

	switch(fork2()){
	case -1:
		perror("fork");
		break;
	case 0:
		close(pipefd[0]);
		close(fonefd);
		close(0);
		open("/dev/null", OREAD);
		if(q = strrchr(shname, '/'))	/* assign = */
			++q;
		else
			q = shname;
		execl(shname, q, "-c", cmd, 0);
		fprint(2, "%s: can't exec %s: %r\n", argv0, shname);
		_exits("exec");
	}
}

void
telparse(char *p)
{
	char *cmd;
	int n;

	while(*p == ' ')
		p++;
	n = strlen(telcmd)+strlen(p)+2;
	cmd = malloc(n);
	if(cmd == 0)
		print("%s: %s %s: can't malloc %d\n",
			argv0, telcmd, p, n);
	else{
		sprint(cmd, "%s %s", telcmd, p);
		shcmd(cmd);
		free(cmd);
	}
}

int
msgcode(char **pp)
{
	int msg;

	msg = strtoul(*pp, pp, apiflag ? 10 : 16);
	if(**pp)
		*pp += 1;
	return msg;
}

int
hexcode(char **pp)
{
	int msg;

	msg = strtoul(*pp, pp, 16);
	if(**pp)
		*pp += 1;
	return msg;
}
