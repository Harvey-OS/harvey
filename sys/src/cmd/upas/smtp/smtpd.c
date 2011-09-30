#include "common.h"
#include "smtpd.h"
#include "smtp.h"
#include <ctype.h>
#include <ip.h>
#include <ndb.h>
#include <mp.h>
#include <libsec.h>
#include <auth.h>
#include "../smtp/y.tab.h"

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

ulong	starttime;

Biobuf	bin;

int	debug;
int	Dflag;
int	fflag;
int	gflag;
int	rflag;
int	sflag;
int	authenticate;
int	authenticated;
int	passwordinclear;
char	*tlscert;

uchar	rsysip[IPaddrlen];

List	senders;
List	rcvers;

char	pipbuf[ERRMAX];
char	*piperror;

String*	mailerpath(char*);
int	pipemsg(int*);
int	rejectcheck(void);
String*	startcmd(void);

static void	logmsg(char *action);

static int
catchalarm(void *a, char *msg)
{
	int rv;

	USED(a);

	/* log alarms but continue */
	if(strstr(msg, "alarm") != nil){
		if(senders.first && senders.first->p &&
		    rcvers.first && rcvers.first->p)
			syslog(0, "smtpd", "note: %s->%s: %s",
				s_to_c(senders.first->p),
				s_to_c(rcvers.first->p), msg);
		else
			syslog(0, "smtpd", "note: %s", msg);
		rv = Atnoterecog;
	} else
		rv = Atnoteunknown;
	if (debug) {
		seek(2, 0, 2);
		fprint(2, "caught note: %s\n", msg);
	}

	/* kill the children if there are any */
	if(pp && pp->pid > 0) {
		syskillpg(pp->pid);
		/* none can't syskillpg, so try a variant */
		sleep(500);
		syskill(pp->pid);
	}

	return rv;
}

/* override string error functions to do something reasonable */
void
s_error(char *f, char *status)
{
	char errbuf[Errlen];

	errbuf[0] = 0;
	rerrstr(errbuf, sizeof(errbuf));
	if(f && *f)
		reply("452 4.3.0 out of memory %s: %s\r\n", f, errbuf);
	else
		reply("452 4.3.0 out of memory %s\r\n", errbuf);
	syslog(0, "smtpd", "++Malloc failure %s [%s]", him, nci->rsys);
	exits(status);
}

static void
usage(void)
{
	fprint(2,
	  "usage: smtpd [-adDfghprs] [-c cert] [-k ip] [-m mailer] [-n net]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *netdir;
	char buf[1024];

	netdir = nil;
	quotefmtinstall();
	fmtinstall('I', eipfmt);
	starttime = time(0);
	ARGBEGIN{
	case 'a':
		authenticate = 1;
		break;
	case 'c':
		tlscert = EARGF(usage());
		break;
	case 'D':
		Dflag++;
		break;
	case 'd':
		debug++;
		break;
	case 'f':				/* disallow relaying */
		fflag = 1;
		break;
	case 'g':
		gflag = 1;
		break;
	case 'h':				/* default domain name */
		dom = EARGF(usage());
		break;
	case 'k':				/* prohibited ip address */
		addbadguy(EARGF(usage()));
		break;
	case 'm':				/* set mail command */
		mailer = mailerpath(EARGF(usage()));
		break;
	case 'n':				/* log peer ip address */
		netdir = EARGF(usage());
		break;
	case 'p':
		passwordinclear = 1;
		break;
	case 'r':
		rflag = 1;			/* verify sender's domain */
		break;
	case 's':				/* save blocked messages */
		sflag = 1;
		break;
	case 't':
		fprint(2, "%s: the -t option is no longer supported, see -c\n",
			argv0);
		tlscert = "/sys/lib/ssl/smtpd-cert.pem";
		break;
	default:
		usage();
	}ARGEND;

	nci = getnetconninfo(netdir, 0);
	if(nci == nil)
		sysfatal("can't get remote system's address: %r");
	parseip(rsysip, nci->rsys);

	if(mailer == nil)
		mailer = mailerpath("send");

	if(debug){
		snprint(buf, sizeof buf, "%s/smtpdb/%ld", UPASLOG, time(0));
		close(2);
		if (create(buf, OWRITE | OEXCL, 0662) >= 0) {
			seek(2, 0, 2);
			fprint(2, "%d smtpd %s\n", getpid(), thedate());
		} else
			debug = 0;
	}
	getconf();
	if (isbadguy())
		exits("banned");
	Binit(&bin, 0, OREAD);

	if (chdir(UPASLOG) < 0)
		syslog(0, "smtpd", "no %s: %r", UPASLOG);
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
	Link *lp, *next;

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

	lp = (Link *)malloc(sizeof *lp);
	lp->p = path;
	lp->next = 0;

	if(l->last)
		l->last->next = lp;
	else
		l->first = lp;
	l->last = lp;
}

void
stamp(void)
{
	if(debug) {
		seek(2, 0, 2);
		fprint(2, "%3lud ", time(0) - starttime);
	}
}

#define	SIZE	4096

int
reply(char *fmt, ...)
{
	long n;
	char buf[SIZE], *out;
	va_list arg;

	va_start(arg, fmt);
	out = vseprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);

	n = out - buf;
	if(debug) {
		seek(2, 0, 2);
		stamp();
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
	reply("250 2.0.0 ok\r\n");
}

void
sayhi(void)
{
	reply("220 %s ESMTP\r\n", dom);
}

/*
 * make callers from class A networks infested by spammers
 * wait longer.
 */

static char netaspam[256] = {
	[58]	1,
	[66]	1,
	[71]	1,

	[76]	1,
	[77]	1,
	[78]	1,
	[79]	1,
	[80]	1,
	[81]	1,
	[82]	1,
	[83]	1,
	[84]	1,
	[85]	1,
	[86]	1,
	[87]	1,
	[88]	1,
	[89]	1,

	[190]	1,
	[201]	1,
	[217]	1,
};

static int
delaysecs(void)
{
	if (trusted)
		return 0;
	if (0 && netaspam[rsysip[0]])
		return 20;
	return 12;
}

void
hello(String *himp, int extended)
{
	char **mynames;
	char *ldot, *rdot;

	him = s_to_c(himp);
	syslog(0, "smtpd", "%s from %s as %s", extended? "ehlo": "helo",
		nci->rsys, him);
	if(rejectcheck())
		return;

	if (strchr(him, '.') && nci && !trusted && fflag &&
	    strcmp(nci->rsys, nci->lsys) != 0){
		/*
		 * We don't care if he lies about who he is, but it is
		 * not okay to pretend to be us.  Many viruses do this,
		 * just parroting back what we say in the greeting.
		 */
		if(strcmp(him, dom) == 0)
			goto Liarliar;
		for(mynames = sysnames_read(); mynames && *mynames; mynames++){
			if(cistrcmp(*mynames, him) == 0){
Liarliar:
				syslog(0, "smtpd",
					"Hung up on %s; claimed to be %s",
					nci->rsys, him);
				if(Dflag)
					sleep(delaysecs()*1000);
				reply("554 5.7.0 Liar!\r\n");
				exits("client pretended to be us");
				return;
			}
		}
	}

	/*
	 * it is unacceptable to claim any string that doesn't look like
	 * a domain name (e.g., has at least one dot in it), but
	 * Microsoft mail client software gets this wrong, so let trusted
	 * (local) clients omit the dot.
	 */
	rdot = strrchr(him, '.');
	if (rdot && rdot[1] == '\0') {
		*rdot = '\0';			/* clobber trailing dot */
		rdot = strrchr(him, '.');	/* try again */
	}
	if (!trusted && rdot == nil)
		goto Liarliar;
	/*
	 * Reject obviously bogus domains and those reserved by RFC 2606.
	 */
	if (rdot == nil)
		rdot = him;
	else
		rdot++;
	if (!trusted && (cistrcmp(rdot, "localdomain") == 0 ||
	    cistrcmp(rdot, "localhost") == 0 ||
	    cistrcmp(rdot, "example") == 0 ||
	    cistrcmp(rdot, "invalid") == 0 ||
	    cistrcmp(rdot, "test") == 0))
		goto Liarliar;			/* bad top-level domain */
	/* check second-level RFC 2606 domains: example\.(com|net|org) */
	if (rdot != him)
		*--rdot = '\0';
	ldot = strrchr(him, '.');
	if (rdot != him)
		*rdot = '.';
	if (ldot == nil)
		ldot = him;
	else
		ldot++;
	if (cistrcmp(ldot, "example.com") == 0 ||
	    cistrcmp(ldot, "example.net") == 0 ||
	    cistrcmp(ldot, "example.org") == 0)
		goto Liarliar;

	/*
	 * similarly, if the claimed domain is not an address-literal,
	 * require at least one letter, which there will be in
	 * at least the last component (e.g., .com, .net) if it's real.
	 * this rejects non-address-literal IP addresses,
	 * among other bogosities.
	 */
	if (!trusted && him[0] != '[') {
		char *p;

		for (p = him; *p != '\0'; p++)
			if (isascii(*p) && isalpha(*p))
				break;
		if (*p == '\0')
			goto Liarliar;
	}
	if(strchr(him, '.') == 0 && nci != nil && strchr(nci->rsys, '.') != nil)
		him = nci->rsys;

	if(Dflag)
		sleep(delaysecs()*1000);
	reply("250%c%s you are %s\r\n", extended ? '-' : ' ', dom, him);
	if (extended) {
		reply("250-ENHANCEDSTATUSCODES\r\n");	/* RFCs 2034 and 3463 */
		if(tlscert != nil)
			reply("250-STARTTLS\r\n");
		if (passwordinclear)
			reply("250 AUTH CRAM-MD5 PLAIN LOGIN\r\n");
		else
			reply("250 AUTH CRAM-MD5\r\n");
	}
}

void
sender(String *path)
{
	String *s;
	static char *lastsender;

	if(rejectcheck())
		return;
	if (authenticate && !authenticated) {
		rejectcount++;
		reply("530 5.7.0 Authentication required\r\n");
		return;
	}
	if(him == 0 || *him == 0){
		rejectcount++;
		reply("503 Start by saying HELO, please.\r\n", s_to_c(path));
		return;
	}

	/* don't add the domain onto black holes or we will loop */
	if(strchr(s_to_c(path), '!') == 0 && strcmp(s_to_c(path), "/dev/null") != 0){
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
		reply("501 5.1.3 Bad character in sender address %s.\r\n",
			s_to_c(path));
		return;
	}

	/*
	 * if the last sender address resulted in a rejection because the sending
	 * domain didn't exist and this sender has the same domain, reject
	 * immediately.
	 */
	if(lastsender){
		if (strncmp(lastsender, s_to_c(path), strlen(lastsender)) == 0){
			filterstate = REFUSED;
			rejectcount++;
			reply("554 5.1.8 Sender domain must exist: %s\r\n",
				s_to_c(path));
			return;
		}
		free(lastsender);	/* different sender domain */
		lastsender = 0;
	}

	/*
	 * see if this ip address, domain name, user name or account is blocked
	 */
	logged = 0;
	filterstate = blocked(path);
	/*
	 * permanently reject what we can before trying smtp ping, which
	 * often leads to merely temporary rejections.
	 */
	switch (filterstate){
	case DENIED:
		syslog(0, "smtpd", "Denied %s (%s/%s)",
			s_to_c(path), him, nci->rsys);
		rejectcount++;
		logged++;
		reply("554-5.7.1 We don't accept mail from %s.\r\n",
			s_to_c(path));
		reply("554 5.7.1 Contact postmaster@%s for more information.\r\n",
			dom);
		return;
	case REFUSED:
		syslog(0, "smtpd", "Refused %s (%s/%s)",
			s_to_c(path), him, nci->rsys);
		rejectcount++;
		logged++;
		reply("554 5.7.1 Sender domain must exist: %s\r\n",
			s_to_c(path));
		return;
	}

	listadd(&senders, path);
	reply("250 2.0.0 sender is %s\r\n", s_to_c(path));
}

enum { Rcpt, Domain, Ntoks };

typedef struct Sender Sender;
struct Sender {
	Sender	*next;
	char	*rcpt;
	char	*domain;
};
static Sender *sendlist, *sendlast;

static int
rdsenders(void)
{
	int lnlen, nf, ok = 1;
	char *line, *senderfile;
	char *toks[Ntoks];
	Biobuf *sf;
	Sender *snd;
	static int beenhere = 0;

	if (beenhere)
		return 1;
	beenhere = 1;

	/*
	 * we're sticking with a system-wide sender list because
	 * per-user lists would require fully resolving recipient
	 * addresses to determine which users they correspond to
	 * (barring exploiting syntactic conventions).
	 */
	senderfile = smprint("%s/senders", UPASLIB);
	sf = Bopen(senderfile, OREAD);
	free(senderfile);
	if (sf == nil)
		return 1;
	while ((line = Brdline(sf, '\n')) != nil) {
		if (line[0] == '#' || line[0] == '\n')
			continue;
		lnlen = Blinelen(sf);
		line[lnlen-1] = '\0';		/* clobber newline */
		nf = tokenize(line, toks, nelem(toks));
		if (nf != nelem(toks))
			continue;		/* malformed line */

		snd = malloc(sizeof *snd);
		if (snd == nil)
			sysfatal("out of memory: %r");
		memset(snd, 0, sizeof *snd);
		snd->next = nil;

		if (sendlast == nil)
			sendlist = snd;
		else
			sendlast->next = snd;
		sendlast = snd;
		snd->rcpt = strdup(toks[Rcpt]);
		snd->domain = strdup(toks[Domain]);
	}
	Bterm(sf);
	return ok;
}

/*
 * read (recipient, sender's DNS) pairs from /mail/lib/senders.
 * Only allow mail to recipient from any of sender's IPs.
 * A recipient not mentioned in the file is always permitted.
 */
static int
senderok(char *rcpt)
{
	int mentioned = 0, matched = 0;
	uchar dnsip[IPaddrlen];
	Sender *snd;
	Ndbtuple *nt, *next, *first;

	rdsenders();
	for (snd = sendlist; snd != nil; snd = snd->next) {
		if (strcmp(rcpt, snd->rcpt) != 0)
			continue;
		/*
		 * see if this domain's ips match nci->rsys.
		 * if not, perhaps a later entry's domain will.
		 */
		mentioned = 1;
		if (parseip(dnsip, snd->domain) != -1 &&
		    memcmp(rsysip, dnsip, IPaddrlen) == 0)
			return 1;
		/*
		 * NB: nt->line links form a circular list(!).
		 * we need to make one complete pass over it to free it all.
		 */
		first = nt = dnsquery(nci->root, snd->domain, "ip");
		if (first == nil)
			continue;
		do {
			if (strcmp(nt->attr, "ip") == 0 &&
			    parseip(dnsip, nt->val) != -1 &&
			    memcmp(rsysip, dnsip, IPaddrlen) == 0)
				matched = 1;
			next = nt->line;
			free(nt);
			nt = next;
		} while (nt != first);
	}
	if (matched)
		return 1;
	else
		return !mentioned;
}

void
receiver(String *path)
{
	char *sender, *rcpt;

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
		syslog(0, "smtpd",
		 "Disallowed %s (%s/%s) to blocked, unknown or invalid name %s",
			sender, him, nci->rsys, s_to_c(path));
		reply("550 5.1.1 %s ... user unknown\r\n", s_to_c(path));
		return;
	}
	rcpt = s_to_c(path);
	if (!senderok(rcpt)) {
		rejectcount++;
		syslog(0, "smtpd", "Disallowed sending IP of %s (%s/%s) to %s",
			sender, him, nci->rsys, rcpt);
		reply("550 5.7.1 %s ... sending system not allowed\r\n", rcpt);
		return;
	}

	logged = 0;

	/* forwarding() can modify 'path' on loopback request */
	if(filterstate == ACCEPT && fflag && !authenticated && forwarding(path)) {
		syslog(0, "smtpd", "Bad Forward %s (%s/%s) (%s)",
			senders.last && senders.last->p?
				s_to_c(senders.last->p): sender,
			him, nci->rsys, path? s_to_c(path): rcpt);
		rejectcount++;
		reply("550 5.7.1 we don't relay.  send to your-path@[] for "
			"loopback.\r\n");
		return;
	}
	listadd(&rcvers, path);
	reply("250 2.0.0 receiver is %s\r\n", s_to_c(path));
}

void
quit(void)
{
	reply("221 2.0.0 Successful termination\r\n");
	if(debug){
		seek(2, 0, 2);
		stamp();
		fprint(2, "# %d sent 221 reply to QUIT %s\n",
			getpid(), thedate());
	}
	close(0);
	exits(0);
}

void
noop(void)
{
	if(rejectcheck())
		return;
	reply("250 2.0.0 Nothing to see here. Move along ...\r\n");
}

void
help(String *cmd)
{
	if(rejectcheck())
		return;
	if(cmd)
		s_free(cmd);
	reply("250 2.0.0 See http://www.ietf.org/rfc/rfc2821\r\n");
}

void
verify(String *path)
{
	char *p, *q;
	char *av[4];

	if(rejectcheck())
		return;
	if(shellchars(s_to_c(path))){
		reply("503 5.1.3 Bad character in address %s.\r\n", s_to_c(path));
		return;
	}
	av[0] = s_to_c(mailer);
	av[1] = "-x";
	av[2] = s_to_c(path);
	av[3] = 0;

	pp = noshell_proc_start(av, (stream *)0, outstream(),  (stream *)0, 1, 0);
	if (pp == 0) {
		reply("450 4.3.2 We're busy right now, try later\r\n");
		return;
	}

	p = Brdline(pp->std[1]->fp, '\n');
	if(p == 0){
		reply("550 5.1.0 String does not match anything.\r\n");
	} else {
		p[Blinelen(pp->std[1]->fp)-1] = 0;
		if(strchr(p, ':'))
			reply("550 5.1.0  String does not match anything.\r\n");
		else{
			q = strrchr(p, '!');
			if(q)
				p = q+1;
			reply("250 2.0.0 %s <%s@%s>\r\n", s_to_c(path), p, dom);
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
		case 0:
			break;
		case -1:
			goto out;
		case '\r':
			c = Bgetc(fp);
			if(c == '\n'){
				if(debug) {
					seek(2, 0, 2);
					fprint(2, "%c", c);
					stamp();
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

static int
optoutall(int filterstate)
{
	Link *l;

	switch(filterstate){
	case ACCEPT:
	case TRUSTED:
		return filterstate;
	}

	for(l = rcvers.first; l; l = l->next)
		if(!optoutofspamfilter(s_to_c(l->p)))
			return filterstate;

	return ACCEPT;
}

String*
startcmd(void)
{
	int n;
	char *filename;
	char **av;
	Link *l;
	String *cmd;

	/*
	 *  ignore the filterstate if the all the receivers prefer it.
	 */
	filterstate = optoutall(filterstate);

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
		reply("554 5.7.1 We don't accept mail from dial-up ports.\r\n");
		/*
		 * we could exit here, because we're never going to accept mail
		 * from this ip address, but it's unclear that RFC821 allows
		 * that.  Instead we set the hardreject flag and go stupid.
		 */
		hardreject = 1;
		return 0;
	case DENIED:
		logmsg("Denied");
		rejectcount++;
		reply("554-5.7.1 We don't accept mail from %s.\r\n",
			s_to_c(senders.last->p));
		reply("554 5.7.1 Contact postmaster@%s for more information.\r\n",
			dom);
		return 0;
	case REFUSED:
		logmsg("Refused");
		rejectcount++;
		reply("554 5.7.1 Sender domain must exist: %s\r\n",
			s_to_c(senders.last->p));
		return 0;
	default:
	case NONE:
		logmsg("Confused");
		rejectcount++;
		reply("554-5.7.0 We have had an internal mailer error "
			"classifying your message.\r\n");
		reply("554-5.7.0 Filterstate is %d\r\n", filterstate);
		reply("554 5.7.0 Contact postmaster@%s for more information.\r\n",
			dom);
		return 0;
	case ACCEPT:
		/*
		 * now that all other filters have been passed,
		 * do grey-list processing.
		 */
		if(gflag)
			vfysenderhostok();
		/* fall through */

	case TRUSTED:
		/*
		 *  set up mail command
		 */
		cmd = s_clone(mailer);
		n = 3;
		for(l = rcvers.first; l; l = l->next)
			n++;
		av = malloc(n * sizeof(char*));
		if(av == nil){
			reply("450 4.3.2 We're busy right now, try later\r\n");
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
		pp = noshell_proc_start(av, instream(), outstream(),
			outstream(), 0, 0);
		free(av);
		break;
	}
	if(pp == 0) {
		reply("450 4.3.2 We're busy right now, try later\r\n");
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
bprintnode(Biobuf *b, Node *p, int *cntp)
{
	int len;

	*cntp = 0;
	if(p->s){
		if(p->addr && strchr(s_to_c(p->s), '@') == nil){
			if(Bprint(b, "%s@%s", s_to_c(p->s), him) < 0)
				return nil;
			*cntp += s_len(p->s) + 1 + strlen(him);
		} else {
			len = s_len(p->s);
			if(Bwrite(b, s_to_c(p->s), len) < 0)
				return nil;
			*cntp += len;
		}
	}else{
		if(Bputc(b, p->c) < 0)
			return nil;
		++*cntp;
	}
	if(p->white) {
		len = s_len(p->white);
		if(Bwrite(b, s_to_c(p->white), len) < 0)
			return nil;
		*cntp += len;
	}
	return p->end+1;
}

static String*
getaddr(Node *p)
{
	for(; p; p = p->next)
		if(p->s && p->addr)
			return p->s;
	return nil;
}

/*
 *  add warning headers of the form
 *	X-warning: <reason>
 *  for any headers that looked like they might be forged.
 *
 *  return byte count of new headers
 */
static int
forgedheaderwarnings(void)
{
	int nbytes;
	Field *f;

	nbytes = 0;

	/* warn about envelope sender */
	if(senders.last != nil && senders.last->p != nil &&
	    strcmp(s_to_c(senders.last->p), "/dev/null") != 0 &&
	    masquerade(senders.last->p, nil))
		nbytes += Bprint(pp->std[0]->fp,
			"X-warning: suspect envelope domain\n");

	/*
	 *  check Sender: field.  If it's OK, ignore the others because this
	 *  is an exploded mailing list.
	 */
	for(f = firstfield; f; f = f->next)
		if(f->node->c == SENDER)
			if(masquerade(getaddr(f->node), him))
				nbytes += Bprint(pp->std[0]->fp,
					"X-warning: suspect Sender: domain\n");
			else
				return nbytes;

	/* check From: */
	for(f = firstfield; f; f = f->next){
		if(f->node->c == FROM && masquerade(getaddr(f->node), him))
			nbytes += Bprint(pp->std[0]->fp,
				"X-warning: suspect From: domain\n");
	}
	return nbytes;
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
	int n, nbytes, sawdot, status, nonhdr, bpr;
	char *cp;
	Field *f;
	Link *l;
	Node *p;
	String *hdr, *line;

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

		s_append(hdr, *cp == '.' ? cp+1 : cp);
	}

	/*
	 *  parse header
	 */
	yyinit(s_to_c(hdr), s_len(hdr));
	yyparse();

	/*
	 *  Look for masquerades.  Let Sender: trump From: to allow mailing list
	 *  forwarded messages.
	 */
	if(fflag)
		nbytes += forgedheaderwarnings();

	/*
	 *  add an orginator and/or destination if either is missing
	 */
	if(originator == 0){
		if(senders.last == nil || senders.last->p == nil)
			nbytes += Bprint(pp->std[0]->fp, "From: /dev/null@%s\n",
				him);
		else
			nbytes += Bprint(pp->std[0]->fp, "From: %s\n",
				s_to_c(senders.last->p));
	}
	if(destination == 0){
		nbytes += Bprint(pp->std[0]->fp, "To: ");
		for(l = rcvers.first; l; l = l->next){
			if(l != rcvers.first)
				nbytes += Bprint(pp->std[0]->fp, ", ");
			nbytes += Bprint(pp->std[0]->fp, "%s", s_to_c(l->p));
		}
		nbytes += Bprint(pp->std[0]->fp, "\n");
	}

	/*
	 *  add sender's domain to any domainless addresses
	 *  (to avoid forging local addresses)
	 */
	cp = s_to_c(hdr);
	for(f = firstfield; cp != nil && f; f = f->next){
		for(p = f->node; cp != 0 && p; p = p->next) {
			bpr = 0;
			cp = bprintnode(pp->std[0]->fp, p, &bpr);
			nbytes += bpr;
		}
		if(status == 0 && Bprint(pp->std[0]->fp, "\n") < 0){
			piperror = "write error";
			status = 1;
		}
		nbytes++;		/* for newline */
	}
	if(cp == nil){
		piperror = "sender domain";
		status = 1;
	}

	/* write anything we read following the header */
	nonhdr = s_to_c(hdr) + s_len(hdr) - cp;
	if(status == 0 && Bwrite(pp->std[0]->fp, cp, nonhdr) < 0){
		piperror = "write error 2";
		status = 1;
	}
	nbytes += nonhdr;
	s_free(hdr);

	/*
	 *  pass rest of message to mailer.  take care of '.'
	 *  escapes.
	 */
	while(sawdot == 0){
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
		if(cp[0] == '.'){
			cp++;
			n--;
		}
		nbytes += n;
		if(status == 0 && Bwrite(pp->std[0]->fp, cp, n) < 0){
			piperror = "write error 3";
			status = 1;
		}
	}
	s_free(line);
	if(sawdot == 0){
		/* message did not terminate normally */
		snprint(pipbuf, sizeof pipbuf, "network eof: %r");
		piperror = pipbuf;
		if (pp->pid > 0) {
			syskillpg(pp->pid);
			/* none can't syskillpg, so try a variant */
			sleep(500);
			syskill(pp->pid);
		}
		status = 1;
	}

	if(status == 0 && Bflush(pp->std[0]->fp) < 0){
		piperror = "write error 4";
		status = 1;
	}
	if (debug) {
		stamp();
		fprint(2, "at end of message; %s .\n",
			(sawdot? "saw": "didn't see"));
	}
	stream_free(pp->std[0]);
	pp->std[0] = 0;
	*byteswritten = nbytes;
	pipesigoff();
	if(status && !piperror)
		piperror = "write on closed pipe";
	return status;
}

char*
firstline(char *x)
{
	char *p;
	static char buf[128];

	strncpy(buf, x, sizeof(buf));
	buf[sizeof(buf)-1] = 0;
	p = strchr(buf, '\n');
	if(p)
		*p = 0;
	return buf;
}

int
sendermxcheck(void)
{
	int pid;
	char *cp, *senddom, *user, *who;
	Waitmsg *w;

	who = s_to_c(senders.first->p);
	if(strcmp(who, "/dev/null") == 0){
		/* /dev/null can only send to one rcpt at a time */
		if(rcvers.first != rcvers.last){
			werrstr("rejected: /dev/null sending to multiple "
				"recipients");
			return -1;
		}
		return 0;
	}

	if(access("/mail/lib/validatesender", AEXEC) < 0)
		return 0;

	senddom = strdup(who);
	if((cp = strchr(senddom, '!')) == nil){
		werrstr("rejected: domainless sender %s", who);
		free(senddom);
		return -1;
	}
	*cp++ = 0;
	user = cp;

	switch(pid = fork()){
	case -1:
		werrstr("deferred: fork: %r");
		return -1;
	case 0:
		/*
		 * Could add an option with the remote IP address
		 * to allow validatesender to implement SPF eventually.
		 */
		execl("/mail/lib/validatesender", "validatesender",
			"-n", nci->root, senddom, user, nil);
		_exits("exec validatesender: %r");
	default:
		break;
	}

	free(senddom);
	w = wait();
	if(w == nil){
		werrstr("deferred: wait failed: %r");
		return -1;
	}
	if(w->pid != pid){
		werrstr("deferred: wait returned wrong pid %d != %d",
			w->pid, pid);
		free(w);
		return -1;
	}
	if(w->msg[0] == 0){
		free(w);
		return 0;
	}
	/*
	 * skip over validatesender 143123132: prefix from rc.
	 */
	cp = strchr(w->msg, ':');
	if(cp && *(cp+1) == ' ')
		werrstr("%s", cp+2);
	else
		werrstr("%s", w->msg);
	free(w);
	return -1;
}

void
data(void)
{
	int status, nbytes;
	char *cp, *ep;
	char errx[ERRMAX];
	Link *l;
	String *cmd, *err;

	if(rejectcheck())
		return;
	if(senders.last == 0){
		reply("503 2.5.2 Data without MAIL FROM:\r\n");
		rejectcount++;
		return;
	}
	if(rcvers.last == 0){
		reply("503 2.5.2 Data without RCPT TO:\r\n");
		rejectcount++;
		return;
	}
	if(!trusted && sendermxcheck()){
		rerrstr(errx, sizeof errx);
		if(strncmp(errx, "rejected:", 9) == 0)
			reply("554 5.7.1 %s\r\n", errx);
		else
			reply("450 4.7.0 %s\r\n", errx);
		for(l=rcvers.first; l; l=l->next)
			syslog(0, "smtpd", "[%s/%s] %s -> %s sendercheck: %s",
				him, nci->rsys, s_to_c(senders.first->p),
				s_to_c(l->p), errx);
		rejectcount++;
		return;
	}

	cmd = startcmd();
	if(cmd == 0)
		return;

	reply("354 Input message; end with <CRLF>.<CRLF>\r\n");
	if(debug){
		seek(2, 0, 2);
		stamp();
		fprint(2, "# sent 354; accepting DATA %s\n", thedate());
	}


	/*
	 *  allow 145 more minutes to move the data
	 */
	alarm(145*60*1000);

	status = pipemsg(&nbytes);

	/*
	 *  read any error messages
	 */
	err = s_new();
	if (debug) {
		stamp();
		fprint(2, "waiting for upas/send to close stderr\n");
	}
	while(s_read_line(pp->std[2]->fp, err))
		;

	alarm(0);
	atnotify(catchalarm, 0);

	if (debug) {
		stamp();
		fprint(2, "waiting for upas/send to exit\n");
	}
	status |= proc_wait(pp);
	if(debug){
		seek(2, 0, 2);
		stamp();
		fprint(2, "# %d upas/send status %#ux at %s\n",
			getpid(), status, thedate());
		if(*s_to_c(err))
			fprint(2, "# %d error %s\n", getpid(), s_to_c(err));
	}

	/*
	 *  if process terminated abnormally, send back error message
	 */
	if(status){
		int code;
		char *ecode;

		if(strstr(s_to_c(err), "mail refused")){
			syslog(0, "smtpd", "++[%s/%s] %s %s refused: %s",
				him, nci->rsys, s_to_c(senders.first->p),
				s_to_c(cmd), firstline(s_to_c(err)));
			code = 554;
			ecode = "5.0.0";
		} else {
			syslog(0, "smtpd", "++[%s/%s] %s %s %s%s%sreturned %#q %s",
				him, nci->rsys,
				s_to_c(senders.first->p), s_to_c(cmd),
				piperror? "error during pipemsg: ": "",
				piperror? piperror: "",
				piperror? "; ": "",
				pp->waitmsg->msg, firstline(s_to_c(err)));
			code = 450;
			ecode = "4.0.0";
		}
		for(cp = s_to_c(err); ep = strchr(cp, '\n'); cp = ep){
			*ep++ = 0;
			reply("%d-%s %s\r\n", code, ecode, cp);
		}
		reply("%d %s mail process terminated abnormally\r\n",
			code, ecode);
	} else {
		/*
		 * if a message appeared on stderr, despite good status,
		 * log it.  this can happen if rewrite.in contains a bad
		 * r.e., for example.
		 */
		if(*s_to_c(err))
			syslog(0, "smtpd",
				"%s returned good status, but said: %s",
				s_to_c(mailer), s_to_c(err));

		if(filterstate == BLOCKED)
			reply("554 5.7.1 we believe this is spam.  "
				"we don't accept it.\r\n");
		else if(filterstate == DELAY)
			reply("450 4.3.0 There will be a delay in delivery "
				"of this message.\r\n");
		else {
			reply("250 2.5.0 sent\r\n");
			logcall(nbytes);
			if(debug){
				seek(2, 0, 2);
				stamp();
				fprint(2, "# %d sent 250 reply %s\n",
					getpid(), thedate());
			}
		}
	}
	proc_free(pp);
	pp = 0;
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
		reply("554 5.5.0 too many errors.  transaction failed.\r\n");
		exits("errcount");
	}
	if(hardreject){
		rejectcount++;
		reply("554 5.7.1 We don't accept mail from dial-up ports.\r\n");
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

String *
s_dec64(String *sin)
{
	int lin, lout;
	String *sout;

	lin = s_len(sin);

	/*
	 * if the string is coming from smtpd.y, it will have no nl.
	 * if it is coming from getcrnl below, it will have an nl.
	 */
	if (*(s_to_c(sin)+lin-1) == '\n')
		lin--;
	sout = s_newalloc(lin+1);
	lout = dec64((uchar *)s_to_c(sout), lin, s_to_c(sin), lin);
	if (lout < 0) {
		s_free(sout);
		return nil;
	}
	sout->ptr = sout->base + lout;
	s_terminate(sout);
	return sout;
}

void
starttls(void)
{
	int certlen, fd;
	uchar *cert;
	TLSconn *conn;

	if (tlscert == nil) {
		reply("500 5.5.1 illegal command or bad syntax\r\n");
		return;
	}
	conn = mallocz(sizeof *conn, 1);
	cert = readcert(tlscert, &certlen);
	if (conn == nil || cert == nil) {
		if (conn != nil)
			free(conn);
		reply("454 4.7.5 TLS not available\r\n");
		return;
	}
	reply("220 2.0.0 Go ahead make my day\r\n");
	conn->cert = cert;
	conn->certlen = certlen;
	fd = tlsServer(Bfildes(&bin), conn);
	if (fd < 0) {
		free(cert);
		free(conn);
		syslog(0, "smtpd", "TLS start-up failed with %s", him);

		/* force the client to hang up */
		close(Bfildes(&bin));		/* probably fd 0 */
		close(1);
		exits("tls failed");
	}
	Bterm(&bin);
	Binit(&bin, fd, OREAD);
	if (dup(fd, 1) < 0)
		fprint(2, "dup of %d failed: %r\n", fd);
	passwordinclear = 1;
	syslog(0, "smtpd", "started TLS with %s", him);
}

void
auth(String *mech, String *resp)
{
	char *user, *pass, *scratch = nil;
	AuthInfo *ai = nil;
	Chalstate *chs = nil;
	String *s_resp1_64 = nil, *s_resp2_64 = nil, *s_resp1 = nil;
	String *s_resp2 = nil;

	if (rejectcheck())
		goto bomb_out;

	syslog(0, "smtpd", "auth(%s, %s) from %s", s_to_c(mech),
		"(protected)", him);

	if (authenticated) {
	bad_sequence:
		rejectcount++;
		reply("503 5.5.2 Bad sequence of commands\r\n");
		goto bomb_out;
	}
	if (cistrcmp(s_to_c(mech), "plain") == 0) {
		if (!passwordinclear) {
			rejectcount++;
			reply("538 5.7.1 Encryption required for requested "
				"authentication mechanism\r\n");
			goto bomb_out;
		}
		s_resp1_64 = resp;
		if (s_resp1_64 == nil) {
			reply("334 \r\n");
			s_resp1_64 = s_new();
			if (getcrnl(s_resp1_64, &bin) <= 0)
				goto bad_sequence;
		}
		s_resp1 = s_dec64(s_resp1_64);
		if (s_resp1 == nil) {
			rejectcount++;
			reply("501 5.5.4 Cannot decode base64\r\n");
			goto bomb_out;
		}
		memset(s_to_c(s_resp1_64), 'X', s_len(s_resp1_64));
		user = s_to_c(s_resp1) + strlen(s_to_c(s_resp1)) + 1;
		pass = user + strlen(user) + 1;
		ai = auth_userpasswd(user, pass);
		authenticated = ai != nil;
		memset(pass, 'X', strlen(pass));
		goto windup;
	}
	else if (cistrcmp(s_to_c(mech), "login") == 0) {
		if (!passwordinclear) {
			rejectcount++;
			reply("538 5.7.1 Encryption required for requested "
				"authentication mechanism\r\n");
			goto bomb_out;
		}
		if (resp == nil) {
			reply("334 VXNlcm5hbWU6\r\n");
			s_resp1_64 = s_new();
			if (getcrnl(s_resp1_64, &bin) <= 0)
				goto bad_sequence;
		}
		reply("334 UGFzc3dvcmQ6\r\n");
		s_resp2_64 = s_new();
		if (getcrnl(s_resp2_64, &bin) <= 0)
			goto bad_sequence;
		s_resp1 = s_dec64(s_resp1_64);
		s_resp2 = s_dec64(s_resp2_64);
		memset(s_to_c(s_resp2_64), 'X', s_len(s_resp2_64));
		if (s_resp1 == nil || s_resp2 == nil) {
			rejectcount++;
			reply("501 5.5.4 Cannot decode base64\r\n");
			goto bomb_out;
		}
		ai = auth_userpasswd(s_to_c(s_resp1), s_to_c(s_resp2));
		authenticated = ai != nil;
		memset(s_to_c(s_resp2), 'X', s_len(s_resp2));
windup:
		if (authenticated) {
			/* if you authenticated, we trust you despite your IP */
			trusted = 1;
			reply("235 2.0.0 Authentication successful\r\n");
		} else {
			rejectcount++;
			reply("535 5.7.1 Authentication failed\r\n");
			syslog(0, "smtpd", "authentication failed: %r");
		}
		goto bomb_out;
	}
	else if (cistrcmp(s_to_c(mech), "cram-md5") == 0) {
		int chal64n;
		char *resp, *t;

		chs = auth_challenge("proto=cram role=server");
		if (chs == nil) {
			rejectcount++;
			reply("501 5.7.5 Couldn't get CRAM-MD5 challenge\r\n");
			goto bomb_out;
		}
		scratch = malloc(chs->nchal * 2 + 1);
		chal64n = enc64(scratch, chs->nchal * 2, (uchar *)chs->chal,
			chs->nchal);
		scratch[chal64n] = 0;
		reply("334 %s\r\n", scratch);
		s_resp1_64 = s_new();
		if (getcrnl(s_resp1_64, &bin) <= 0)
			goto bad_sequence;
		s_resp1 = s_dec64(s_resp1_64);
		if (s_resp1 == nil) {
			rejectcount++;
			reply("501 5.5.4 Cannot decode base64\r\n");
			goto bomb_out;
		}
		/* should be of form <user><space><response> */
		resp = s_to_c(s_resp1);
		t = strchr(resp, ' ');
		if (t == nil) {
			rejectcount++;
			reply("501 5.5.4 Poorly formed CRAM-MD5 response\r\n");
			goto bomb_out;
		}
		*t++ = 0;
		chs->user = resp;
		chs->resp = t;
		chs->nresp = strlen(t);
		ai = auth_response(chs);
		authenticated = ai != nil;
		goto windup;
	}
	rejectcount++;
	reply("501 5.5.1 Unrecognised authentication type %s\r\n", s_to_c(mech));
bomb_out:
	if (ai)
		auth_freeAI(ai);
	if (chs)
		auth_freechal(chs);
	if (scratch)
		free(scratch);
	if (s_resp1)
		s_free(s_resp1);
	if (s_resp2)
		s_free(s_resp2);
	if (s_resp1_64)
		s_free(s_resp1_64);
	if (s_resp2_64)
		s_free(s_resp2_64);
}
