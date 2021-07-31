#include "common.h"
#include "smtpd.h"
#include "smtp.h"
#include "ip.h"

char	*me;
char	*him="";
char	*dom;
process	*pp;
String	*mailer;
NetConnInfo *nci;

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
String*	mailerpath(char*);

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
	char errbuf[Errlen];

	errbuf[0] = 0;
	rerrstr(errbuf, sizeof(errbuf));
	if(f && *f)
		reply("452 out of memory %s: %s\r\n", f, errbuf);
	else
		reply("452 out of memory %s\r\n", errbuf);
	syslog(0, "smtpd", "++Malloc failure %s [%s]", him, nci->rsys);
	exits(status);
}

void
main(int argc, char **argv)
{
	char *p, buf[1024];
	String *s;
	Link *l;
	uchar addr[IPv4addrlen];
	char *netdir;

	netdir = nil;
	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'n':				/* log peer ip address */
		netdir = ARGF();
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
	case 'm':				/* set mail command */
		p = ARGF();
		if(p)
			mailer = mailerpath(p);
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
	}ARGEND;

	nci = getnetconninfo(netdir, 0);
	if(nci == nil)
		sysfatal("can't get remote system's address");

	if(mailer == nil)
		mailer = mailerpath("send");

	v4parseip(addr, nci->rsys);
	peerip = nhgetl(addr);

	/* check if this IP address is banned */
	for(l = badguys.first; l; l = l->next)
		if(cidrcheck(s_to_c(l->p)))
			exits("banned");

	if(debug){
		close(2);
		snprint(buf, sizeof(buf), "%s/smtpd", UPASLOG);
		if (open(buf, OWRITE) >= 0) {
			seek(2, 0, 2);
			fprint(2, "%d smtpd %s\n", getpid(), thedate());
		} else
			debug = 0;
	}
	getconf();
	Binit(&bin, 0, OREAD);

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
	zzparse();
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
	out = vseprint(buf, buf+SIZE, fmt, arg);
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
	if(strchr(him, '.') == 0 && nci != nil && strchr(nci->rsys, '.') != nil)
		him = nci->rsys;
		
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
	if(filterstate == ACCEPT && rflag && returnable(s_to_c(path))){
		if(rmtdns(nci->root, s_to_c(path)) < 0){
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
				sender, him, nci->rsys, s_to_c(path));
		reply("550 %s ... user unknown\r\n", s_to_c(path));
		return;
	}

	logged = 0;
		/* forwarding() can modify 'path' on loopback request */
	if(filterstate == ACCEPT && fflag && forwarding(path)) {
		syslog(0, "smtpd", "Bad Forward %s (%s/%s) (%s)",
			s_to_c(senders.last->p), him, nci->rsys, s_to_c(path));
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
	char *p, *q;
	char *av[4];

	if(rejectcheck())
		return;
	if(shellchars(s_to_c(path))){
		reply("503 Bad character in address %s.\r\n", s_to_c(path));
		return;
	}
	av[0] = s_to_c(mailer);
	av[1] = "-x";
	av[2] = s_to_c(path);
	av[3] = 0;

	pp = noshell_proc_start(av, (stream *)0, outstream(),  (stream *)0, 1, 0);
	if (pp == 0) {
		reply("450 We're busy right now, try later\r\n");
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
}

/*
 *  get a line that ends in crnl or cr, turn terminating crnl into a nl
 *
 *  return 0 on EOF
 */
static int
getcrnl(String *s, Biobuf *fp)
{
	int c;

	for(;;){
		c = Bgetc(fp);
		if(debug) {
			seek(2, 0, 2);
			fprint(2, "%c", c);
		}
		switch(c){
		case -1:
			goto out;
		case '\r':
			c = Bgetc(fp);
			if(c == '\n'){
				if(debug) {
					seek(2, 0, 2);
					fprint(2, "%c", c);
				}
				s_putc(s, '\n');
				goto out;
			}
			Bungetc(fp);
			s_putc(s, '\r');
			break;
		case '\n':
			s_putc(s, c);
			goto out;
		default:
			s_putc(s, c);
			break;
		}
	}
out:
	s_terminate(s);
	return s_len(s);
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
	syslog(0, "smtpd", "[%s/%s] %s sent %d bytes to %s", him, nci->rsys,
		s_to_c(from), nbytes, s_to_c(to));
	s_free(to);
	s_free(from);
}

static void
logmsg(char *action)
{
	Link *l;

	if(logged)
		return;

	logged = 1;
	for(l = rcvers.first; l; l = l->next)
		syslog(0, "smtpd", "%s %s (%s/%s) (%s)", action,
			s_to_c(senders.last->p), him, nci->rsys, s_to_c(l->p));
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
		logmsg("Blocked");
		filename = dumpfile(s_to_c(senders.last->p));
		cmd = s_new();
		s_append(cmd, "cat > ");
		s_append(cmd, filename);
		pp = proc_start(s_to_c(cmd), instream(), 0, outstream(), 0, 0);
		break;
	case DIALUP:
		logmsg("Dialup");
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
		logmsg("Denied");
		rejectcount++;
		reply("554-We don't accept mail from %s.\r\n", s_to_c(senders.last->p));
		reply("554 Contact postmaster@%s for more information.\r\n", dom);
		return 0;
	case REFUSED:
		logmsg("Refused");
		rejectcount++;
		reply("554 Sender domain must exist: %s\r\n", s_to_c(senders.last->p));
		return 0;
	default:
	case NONE:
		logmsg("Confused");
		rejectcount++;
		reply("554-We have had an internal mailer error classifying your message.\r\n");
		reply("554-Filterstate is %d\r\n", filterstate);
		reply("554 Contact postmaster@%s for more information.\r\n", dom);
		return 0;
	case ACCEPT:
	case TRUSTED:
		/*
		 *  set up mail command
		 */
		cmd = s_clone(mailer);
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

/*
 *  print out a header line, expanding any domainless addresses into
 *  address@him
 */
char*
bprintnode(Biobuf *b, Node *p)
{
	if(p->s){
		if(p->addr && strchr(s_to_c(p->s), '@') == nil){
			if(Bprint(b, "%s@%s", s_to_c(p->s), him) < 0)
				return nil;
		} else {
			if(Bwrite(b, s_to_c(p->s), s_len(p->s)) < 0)
				return nil;
		}
	}else{
		if(Bputc(b, p->c) < 0)
			return nil;
	}
	if(p->white)
		if(Bwrite(b, s_to_c(p->white), s_len(p->white)) < 0)
			return nil;
	return p->end+1;
}

/*
 *  pipe message to mailer with the following transformations:
 *	- change \r\n into \n.
 *	- add sender's domain to any addrs with no domain
 *	- add a From: if none of From:, Sender:, or Replyto: exists
 *	- add a Received: line
 */
int
pipemsg(int *byteswritten)
{
	int status;
	char *cp;
	String *line;
	String *hdr;
	int n, nbytes;
	int sawdot;
	Field *f;
	Node *p;

	pipesig(&status);	/* set status to 1 on write to closed pipe */
	sawdot = 0;
	status = 0;

	/*
	 *  add a 'From ' line as envelope
	 */
	nbytes = 0;
	nbytes += Bprint(pp->std[0]->fp, "From %s %s remote from \n",
			s_to_c(senders.first->p), thedate());

	/*
	 *  add our own Received: stamp
	 */
	nbytes += Bprint(pp->std[0]->fp, "Received: from %s ", him);
	if(nci->rsys)
		nbytes += Bprint(pp->std[0]->fp, "([%s]) ", nci->rsys);
	nbytes += Bprint(pp->std[0]->fp, "by %s; %s\n", me, thedate());

	/*
	 *  read first 16k obeying '.' escape.  we're assuming
	 *  the header will all be there.
	 */
	line = s_new();
	hdr = s_new();
	while(sawdot == 0 && s_len(hdr) < 16*1024){
		n = getcrnl(s_reset(line), &bin);

		/* eof or error ends the message */
		if(n <= 0)
			break;

		/* a line with only a '.' ends the message */
		cp = s_to_c(line);
		if(n == 2 && *cp == '.' && *(cp+1) == '\n'){
			sawdot = 1;
			break;
		}

		s_append(hdr, s_to_c(line));
	}

	/*
 	 *  parse header
	 */
	yyinit(s_to_c(hdr), s_len(hdr));
	yyparse();

	/*
	 *  add an orginator if there isn't one
	 */
	if(originator == 0)
		Bprint(pp->std[0]->fp, "From: /dev/null@%s\n", him);

	/*
	 *  add sender's domain to any domainless addresses
	 *  (to avoid forging local addresses)
	 */
	cp = s_to_c(hdr);
	for(f = firstfield; cp != nil && f; f = f->next){
		for(p = f->node; cp != 0 && p; p = p->next)
			cp = bprintnode(pp->std[0]->fp, p);
		Bprint(pp->std[0]->fp, "\n");
	}
	if(cp == nil)
		status = 1;

	/* write anything we read following the header */
	if(status == 0)
		if(Bwrite(pp->std[0]->fp, cp, s_to_c(hdr) + s_len(hdr) - cp) < 0)
			status = 1;
	s_free(hdr);

	/*
	 *  pass rest of message to mailer.  take care of '.'
	 *  escapes.
	 */
	while(status == 0 && sawdot == 0){
		n = getcrnl(s_reset(line), &bin);

		/* eof or error ends the message */
		if(n <= 0)
			break;

		/* a line with only a '.' ends the message */
		cp = s_to_c(line);
		if(n == 2 && *cp == '.' && *(cp+1) == '\n'){
			sawdot = 1;
			break;
		}
		nbytes += n;
		if(Bwrite(pp->std[0]->fp, s_to_c(line), n) < 0){
			status = 1;
			break;
		}
	}
	s_free(line);
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
		syslog(0, "smtpd", "++[%s/%s] %s returned %d", him, nci->rsys, s_to_c(cmd), status);
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
		syslog(0, "smtpd", "Rejected (%s/%s)", him, nci->rsys);
		reply("554 too many errors.  transaction failed.\r\n");
		exits("errcount");
	}
	if(hardreject){
		rejectcount++;
		reply("554 We don't accept mail from dial-up ports.\r\n");
	}
	return hardreject;
}

/*
 *  create abs path of the mailer
 */
String*
mailerpath(char *p)
{
	String *s;

	if(p == nil)
		return nil;
	if(*p == '/')
		return s_copy(p);
	s = s_new();
	s_append(s, UPASBIN);
	s_append(s, "/");
	s_append(s, p);
	return s;
}
