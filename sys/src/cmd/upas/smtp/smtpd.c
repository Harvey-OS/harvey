#include "common.h"
#include "smtpd.h"
#include "ip.h"

char	*me;
char	*him="";
char	*hisaddr="";
char	*netroot="/net";
char	*dom;
process	*pp;

int	filterstate = ACCEPT;
int	trusted;
int	logged;
int	rejectcount;
int	hardreject;

ulong	peerip;
Biobuf	bin;

int	debug;
int	fflag;
int	rflag;
int	sflag;

List senders;
List rcvers;
List ourdoms;
List badguys;

int	pipemsg(int*);
String*	startcmd(void);
int	rejectcheck(void);

static int
catchalarm(void *a, char *msg)
{
	USED(a);

	if(strstr(msg, "alarm")){
		if(senders.first && rcvers.first)
			syslog(0, "smtpd", "note: %s->%s: %s\n", s_to_c(senders.first->p),
				s_to_c(rcvers.first->p), msg);
		else
			syslog(0, "smtpd", "note: %s\n", msg);
	}
	if(pp){
		syskillpg(pp->pid);	/* perhaps should wait also?? */
		proc_free(pp);
		pp = 0;
	}
	return 0;
}

	/* override string error functions to do something reasonable */
void
s_error(char *f, char *status)
{
	char errbuf[ERRLEN];

	errbuf[0] = 0;
	errstr(errbuf);
	if(f && *f)
		reply("452 out of memory %s: %s\r\n", f, errbuf);
	else
		reply("452 out of memory %s\r\n", errbuf);
	syslog(0, "smtpd", "++Malloc failure %s [%s]", him, hisaddr);
	exits(status);
}

void
main(int argc, char **argv)
{
	char *p, buf[1024];
	String *s;
	Link *l;
	uchar addr[IPv4addrlen];

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'n':				/* log peer ip address */
		p = ARGF();
		if(p && *p){
			netroot = p;
			hisaddr = remoteaddr(-1, p);
		}
		break;
	case 'f':				/* disallow relaying */
		fflag = 1;
		break;
	case 'h':				/* default domain name */
		dom = ARGF();
		break;
	case 'k':				/* prohibited ip address */
		p = ARGF();
		if (p){
			s = s_new();
			s_append(s, p);
			listadd(&badguys, s);
		}
		break;
	case 'r':
		rflag = 1;			/* verify sender's domain */
		break;
	case 's':				/* save blocked messages */
		sflag = 1;
		break;
	default:
		fprint(2, "usage: smtpd [-dfhrs] [-n net]\n");
		exits("usage");
	}ARGEND
	if(hisaddr == 0 || *hisaddr == 0)
		hisaddr = remoteaddr(0,0);	/* try to get peer addr from fd 0 */
	if(hisaddr && *hisaddr){
		v4parseip(addr, hisaddr);
		peerip = nhgetl(addr);
	}
		/* check if this IP address is banned */
	for(l = badguys.first; l; l = l->next)
		if(cidrcheck(s_to_c(l->p)))
			exits("banned");

	getconf();
	Binit(&bin, 0, OREAD);
	if(debug){
		close(2);
		snprint(buf, sizeof(buf), "%s/smtpd", UPASLOG);
		if (open(buf, OWRITE) >= 0) {
			seek(2, 0, 2);
			fprint(2, "%d smtpd %s\n", getpid(), thedate());
		} else
			debug = 0;
	}

	chdir(UPASLOG);
	me = sysname_read();
	if(dom == 0 || dom[0] == 0)
		dom = domainname_read();
	if(dom == 0 || dom[0] == 0)
		dom = me;
	sayhi();
	parseinit();
		/* allow 45 minutes to parse the header */
	atnotify(catchalarm, 1);
	alarm(45*60*1000);
	yyparse();
	exits(0);
}

void
listfree(List *l)
{
	Link *lp;
	Link *next;

	for(lp = l->first; lp; lp = next){
		next = lp->next;
		s_free(lp->p);
		free(lp);
	}
	l->first = l->last = 0;
}

void
listadd(List *l, String *path)
{
	Link *lp;

	lp = (Link *)malloc(sizeof(Link));
	lp->p = path;
	lp->next = 0;

	if(l->last)
		l->last->next = lp;
	else
		l->first = lp;
	l->last = lp;
}

#define	SIZE	4096
int
reply(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg;
	int n;

	va_start(arg, fmt);
	out = doprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	n = (long)(out-buf);
	if(debug) {
		seek(2, 0, 2);
		write(2, buf, n);
	}
	write(1, buf, n);
	return n;
}

void
reset(void)
{
	if(rejectcheck())
		return;
	listfree(&rcvers);
	listfree(&senders);
	if(filterstate != DIALUP){
		logged = 0;
		filterstate = ACCEPT;
	}
	reply("250 ok\r\n");
}

void
sayhi(void)
{
	reply("220 %s SMTP\r\n", dom);
}

void
hello(String *himp)
{
	if(rejectcheck())
		return;
	him = s_to_c(himp);
	reply("250 %s you are %s\r\n", dom, him);
}

void
sender(String *path)
{
	String *s;
	char *cp;
	static char *lastsender;

	if(rejectcheck())
		return;
	if(him == 0 || *him == 0){
		rejectcount++;
		reply("503 Start by saying HELO, please.\r\n", s_to_c(path));
		return;
	}
	if(strchr(s_to_c(path), '!') == 0){
		s = s_new();
		s_append(s, him);
		s_append(s, "!");
		s_append(s, s_to_c(path));
		s_terminate(s);
		s_free(path);
		path = s;
	}
	if(shellchars(s_to_c(path))){
		rejectcount++;
		reply("503 Bad character in sender address %s.\r\n", s_to_c(path));
		return;
	}

	/*
	 * if the last sender address resulted in a rejection because the sending
	 * domain didn't exist and this sender has the same domain, reject immediately.
	 */
	if(lastsender){
		if (strncmp(lastsender, s_to_c(path), strlen(lastsender)) == 0){
			filterstate = REFUSED;
			rejectcount++;
			reply("554 Sender domain must exist: %s\r\n", s_to_c(path));
			return;
		}
		free(lastsender);	/* different sender domain */
		lastsender = 0;
	}

	/*
	 * see if this ip address, domain name, user name or account is blocked
	 */
	filterstate = blocked(path);

	/*
	 * perform DNS lookup to see if sending domain exists
	 */
	if(filterstate == ACCEPT && rflag && !trusted && returnable(s_to_c(path))){
		if(rmtdns(netroot, s_to_c(path)) < 0){
			filterstate = REFUSED;
			lastsender = strdup(s_to_c(path));
			cp = strrchr(lastsender, '!');
			if(cp)
				*cp = 0;
		}
	}
	logged = 0;
	listadd(&senders, path);
	reply("250 sender is %s\r\n", s_to_c(path));
}

void
receiver(String *path)
{
	char *sender;

	if(rejectcheck())
		return;
	if(him == 0 || *him == 0){
		rejectcount++;
		reply("503 Start by saying HELO, please\r\n");
		return;
	}
	if(senders.last)
		sender = s_to_c(senders.last->p);
	else
		sender = "<unknown>";

	if(!recipok(s_to_c(path))){
		rejectcount++;
		syslog(0, "smtpd", "Disallowed %s (%s/%s) to %s",
				sender, him, hisaddr, s_to_c(path));
		reply("550 %s ... user unknown\r\n", s_to_c(path));
		return;
	}

	logged = 0;
		/* forwarding() can modify 'path' on loopback request */
	if(filterstate == ACCEPT && fflag && forwarding(path)) {
		syslog(0, "smtpd", "Bad Forward %s (%s/%s) (%s)",
			s_to_c(senders.last->p), him, hisaddr, s_to_c(path));
		rejectcount++;
		reply("550 we don't relay.  send to your-path@[] for loopback.\r\n");
		return;
	}
	listadd(&rcvers, path);
	reply("250 receiver is %s\r\n", s_to_c(path));
}

void
quit(void)
{
	reply("221 Successful termination\r\n");
	close(0);
	exits(0);
}

void
turn(void)
{
	if(rejectcheck())
		return;
	reply("502 TURN unimplemented\r\n");
}

void
noop(void)
{
	if(rejectcheck())
		return;
	reply("250 Stop wasting my time!\r\n");
}

void
help(String *cmd)
{
	if(rejectcheck())
		return;
	if(cmd)
		s_free(cmd);
	reply("250 Read rfc821 and stop wasting my time\r\n");
}

void
verify(String *path)
{
	String *cmd;
	char *p, *q;
	char *av[4];

	if(rejectcheck())
		return;
	if(shellchars(s_to_c(path))){
		reply("503 Bad character in address %s.\r\n", s_to_c(path));
		return;
	}
	cmd = s_new();
	s_append(cmd, UPASBIN);
	s_append(cmd, "/send");
	av[0] = s_to_c(cmd);
	av[1] = "-x";
	av[2] = s_to_c(path);
	av[3] = 0;

	pp = noshell_proc_start(av, (stream *)0, outstream(),  (stream *)0, 1, 0);
	if (pp == 0) {
		reply("450 We're busy right now, try later\r\n");
		s_free(cmd);
		return;
	}

	p = Brdline(pp->std[1]->fp, '\n');
	if(p == 0){
		reply("550 String does not match anything.\r\n");
	} else {
		p[Blinelen(pp->std[1]->fp)-1] = 0;
		if(strchr(p, ':'))
			reply("550 String does not match anything.\r\n");
		else{
			q = strrchr(p, '!');
			if(q)
				p = q+1;
			reply("250 %s <%s@%s>\r\n", s_to_c(path), p, dom);
		}
	}
	proc_wait(pp);
	proc_free(pp);
	pp = 0;
	s_free(cmd);
}

/*
 *  get a line that ends in crnl or cr, turn terminating crnl into a nl
 *
 *  return 0 on EOF
 */
static int
getcrnl(char *buf, int n, Biobuf *fp)
{
	int c;
	char *ep;
	char *bp;

	bp = buf;
	ep = bp + n - 1;
	while(bp != ep){
		c = Bgetc(fp);
		if(debug) {
			seek(2, 0, 2);
			fprint(2, "%c", c);
		}
		switch(c){
		case -1:
			*bp = 0;
			if(bp==buf)
				return 0;
			else
				return bp-buf;
		case '\r':
			c = Bgetc(fp);
			if(c == '\n'){
				if(debug) {
					seek(2, 0, 2);
					fprint(2, "%c", c);
				}
				*bp++ = '\n';
				*bp = 0;
				return bp-buf;
			}
			Bungetc(fp);
			c = '\r';
			break;
		case '\n':
			*bp++ = '\n';
			*bp = 0;
			return bp-buf;
		}
		*bp++ = c;
	}
	*bp = 0;
	return bp-buf;
}

void
logcall(int nbytes)
{
	Link *l;
	String *to, *from;

	to = s_new();
	from = s_new();
	for(l = senders.first; l; l = l->next){
		if(l != senders.first)
			s_append(from, ", ");
		s_append(from, s_to_c(l->p));
	}
	for(l = rcvers.first; l; l = l->next){
		if(l != rcvers.first)
			s_append(to, ", ");
		s_append(to, s_to_c(l->p));
	}
	syslog(0, "smtpd", "[%s/%s] %s sent %d bytes to %s", him, hisaddr,
		s_to_c(from), nbytes, s_to_c(to));
	s_free(to);
	s_free(from);
}

String*
startcmd(void)
{
	int n;
	Link *l;
	char **av;
	String *cmd;
	char *filename;

	switch (filterstate){
	case BLOCKED:
	case DELAY:
		rejectcount++;
		if(!logged){
			logged = 1;
			for(l = rcvers.first; l; l = l->next)
				syslog(0, "smtpd", "Blocked %s (%s/%s) (%s)",
					s_to_c(senders.last->p), him, hisaddr, s_to_c(l->p));
		}
		filename = dumpfile(s_to_c(senders.last->p));
		cmd = s_new();
		s_append(cmd, "cat > ");
		s_append(cmd, filename);
		pp = proc_start(s_to_c(cmd), instream(), 0, outstream(), 0, 0);
		break;
	case DIALUP:
		if(!logged){
			logged = 1;
			for(l = rcvers.first; l; l = l->next)
				syslog(0, "smtpd", "Dialup %s (%s/%s) (%s)",
					s_to_c(senders.last->p), him, hisaddr, s_to_c(l->p));
		}
		rejectcount++;
		reply("554 We don't accept mail from dial-up ports.\r\n");
		/*
		 * we could exit here, because we're never going to accept mail from this
		 * ip address, but it's unclear that RFC821 allows that.  Instead we set
		 * the hardreject flag and go stupid.
		 */
		hardreject = 1;
		return 0;
	case DENIED:
		if(!logged){
			logged = 1;
			for(l = rcvers.first; l; l = l->next)
				syslog(0, "smtpd", "Denied %s (%s/%s) (%s)",
					s_to_c(senders.last->p), him, hisaddr, s_to_c(l->p));
		}
		rejectcount++;
		reply("554-We don't accept mail from %s.\r\n", s_to_c(senders.last->p));
		reply("554 Contact postmaster@%s for more information.\r\n", dom);
		return 0;
	case REFUSED:
		if(!logged){
			logged = 1;
			for(l = rcvers.first; l; l = l->next)
				syslog(0, "smtpd", "Refused %s (%s/%s) (%s)",
					s_to_c(senders.last->p), him, hisaddr, s_to_c(l->p));
		}
		rejectcount++;
		reply("554 Sender domain must exist: %s\r\n", s_to_c(senders.last->p));
		return 0;
	case ACCEPT:
	default:
		/*
		 *  set up mail command
		 */
		cmd = s_new();
		s_append(cmd, UPASBIN);
		s_append(cmd, "/send");
		n = 3;
		for(l = rcvers.first; l; l = l->next)
			n++;
		av = malloc(n*sizeof(char*));
		if(av == nil){
			reply("450 We're busy right now, try later\n");
			s_free(cmd);
			return 0;
		}

			n = 0;
		av[n++] = s_to_c(cmd);
		av[n++] = "-r";
		for(l = rcvers.first; l; l = l->next)
			av[n++] = s_to_c(l->p);
		av[n] = 0;
		/*
		 *  start mail process
		 */
		pp = noshell_proc_start(av, instream(), 0, outstream(), 0, 0);
		free(av);
		break;
	}
	if(pp == 0) {
		reply("450 We're busy right now, try later\n");
		s_free(cmd);
		return 0;
	}
	return cmd;
}

int
pipemsg(int *byteswritten)
{
	int status = 0;
	char *cp;
	char buf[4096];
	int sol, n, nbytes;
	int sawdot;

	/*
	 *  read first line.  If it is a 'From ' line, leave it.  Otherwise,
	 *  add one.
	 */
	n = getcrnl(buf, sizeof(buf), &bin);
	cp = buf;
	nbytes = 0;
	nbytes += Bprint(pp->std[0]->fp, "From %s %s remote from \n", s_to_c(senders.first->p),
			thedate());
	nbytes += Bprint(pp->std[0]->fp, "Received: from %s ", him);
	if(hisaddr && *hisaddr)
		nbytes += Bprint(pp->std[0]->fp, "([%s]) ", hisaddr);
	nbytes += Bprint(pp->std[0]->fp, "by %s; %s\n", me, thedate());


	/*
	 *  pass message to mailer.  take care of '.' escapes.
	 */
	pipesig(&status);	/* set status to 1 on write to closed pipe */
	sawdot = 0;
	for(sol = 1; status == 0 && n != 0;){
		if(n > 0){
			if(sol && *cp=='.'){
				/* '.'s at the start of line is an escape */
				cp++;
				n--;
				if(*cp=='\n'){
					sawdot = 1;
					break;
				}
			}
			sol = cp[n-1] == '\n';
		}
		nbytes += n;
		if(Bwrite(pp->std[0]->fp, cp, n) < 0)
			status = 1;
		n = getcrnl(buf, sizeof(buf), &bin);
		cp = buf;
	}
	pipesigoff();

	if(sawdot == 0){
		/* message did not terminate normally */
		syskillpg(pp->pid);
		status = 1;
	}

	if(Bflush(pp->std[0]->fp) < 0)
		status = 1;
	stream_free(pp->std[0]);
	pp->std[0] = 0;
	*byteswritten = nbytes;
	return status;
}

void
data(void)
{
	String *cmd;
	String *err;
	int status, nbytes;
	char *cp, *ep;

	if(rejectcheck())
		return;
	if(senders.last == 0){
		reply("503 Data without MAIL FROM:\r\n");
		rejectcount++;
		return;
	}
	if(rcvers.last == 0){
		reply("503 Data without RCPT TO:\r\n");
		rejectcount++;
		return;
	}

	cmd = startcmd();
	if(cmd == 0)
		return;

	reply("354 Input message; end with <CRLF>.<CRLF>\r\n");

	/*
	 *  allow 45 more minutes to move the data
	 */
	alarm(45*60*1000);

	status = pipemsg(&nbytes);

	/*
	 *  read any error messages
	 */
	err = s_new();
	while(s_read_line(pp->std[2]->fp, err))
		;

	alarm(0);
	atnotify(catchalarm, 0);

	status |= proc_wait(pp);
	if(debug){
		seek(2, 0, 2);
		fprint(2, "%d status %ux\n", getpid(), status);
		if(*s_to_c(err))
			fprint(2, "%d error %s\n", getpid(), s_to_c(err));
	}
	proc_free(pp);
	pp = 0;

	/*
	 *  if process terminated abnormally, send back error message
	 */
	if(status){
		syslog(0, "smtpd", "++[%s/%s] %s returned %d", him, hisaddr, s_to_c(cmd), status);
		for(cp = s_to_c(err); ep = strchr(cp, '\n'); cp = ep){
			*ep++ = 0;
			reply("450-%s\r\n", cp);
			syslog(0, "smtpd", "450-%s", cp);
		}
		reply("450 mail process terminated abnormally\r\n");
	} else {
		if(filterstate == BLOCKED)
			reply("554 we believe this is spam.  we don't accept it.\r\n");
		else
		if(filterstate == DELAY)
			reply("554 There will be a delay in delivery of this message.\r\n");
		else {
			reply("250 sent\r\n");
			logcall(nbytes);
		}
	}
	s_free(cmd);
	s_free(err);

	listfree(&senders);
	listfree(&rcvers);
}

/*
 * when we have blocked a transaction based on IP address, there is nothing
 * that the sender can do to convince us to take the message.  after the
 * first rejection, some spammers continually RSET and give a new MAIL FROM:
 * filling our logs with rejections.  rejectcheck() limits the retries and
 * swiftly rejects all further commands after the first 500-series message
 * is issued.
 */
int
rejectcheck(void)
{

	if(rejectcount > MAXREJECTS){
		syslog(0, "smtpd", "Rejected (%s/%s)", him, hisaddr);
		reply("554 too many errors.  transaction failed.\r\n");
		exits("errcount");
	}
	if(hardreject){
		rejectcount++;
		reply("554 We don't accept mail from dial-up ports.\r\n");
	}
	return hardreject;
}
