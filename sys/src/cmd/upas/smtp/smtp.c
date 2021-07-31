#include "common.h"
#include "smtp.h"
#include <ctype.h>

char*	connect(char*);
char*	hello(char*);
char*	mailfrom(char*);
char*	rcptto(char*);
char*	data(String*, int);
void	quit(void);
int	getreply(void);
void	addhostdom(String*, char*);
String*	bangtoat(char*);
void	convertheader(String*);
void	printheader(void);
char*	domainify(char*, char*);
void	putcrnl(char*, int);
char*	getcrnl(String*);
void	printdate(Node*);
char	*rewritezone(char *);
int	mxdial(char*, int*, char*);
int	dBprint(char*, ...);

#define Retry "Temporary Failure, Retry"
#define Giveup "Permanent Failure"

int	debug;		/* true if we're debugging */
String	*reply;		/* last reply */
String	*toline;
char	*sender;	/* who to bounce message to */
int	last = 'n';	/* last character sent by putcrnl() */
int	filter;
int	unix;
int	gateway;	/* true if we are traversing a mail gateway */
char	ddomain[1024];	/* domain name of destination machine */
char	*gdomain;	/* domain name of gateway */
char	*uneaten;	/* first character after rfc822 headers */
char	hostdomain[256];
Biobuf	bin;
Biobuf	bout;
Biobuf	berr;

void
usage(void)
{
	fprint(2, "usage: smtp [-du] [-hhost] [.domain] net!host[!service] sender rcpt-list\n");
	exits(Giveup); 
}

void
timeout(void *x, char *msg)
{
	USED(x);
	if(strstr(msg, "alarm")){
		fprint(2, "smtp timeout: no retries");
		exits(Giveup);
	}
	noted(NDFLT);
}

void
main(int argc, char **argv)
{
	char *domain;
	char *host;
	String *from;
	String *fromm;
	char *addr;
	char *rv;

	reply = s_new();
	unix = 0;
	host = 0;
	ARGBEGIN{
	case 'f':
		filter = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'g':
		gdomain = ARGF();
		break;
	case 'u':
		unix = 1;
		break;
	case 'h':
		host = ARGF();
		break;
	default:
		usage();
		break;
	}ARGEND;

	Binit(&berr, 2, OWRITE);

	/*
	 *  get domain and add to host name
	 */
	domain = csquery("soa", "", "dom");
	if(*argv && **argv=='.')
		domain = *argv++;
	if(host == 0)
		host = sysname_read();
	strcpy(hostdomain, domainify(host, domain));

	/*
	 *  get destination address
	 */
	if(*argv == 0)
		usage();
	addr = *argv++;

	/*
	 *  get sender's machine.
	 *  get sender in internet style.  domainify if necessary.
	 */
	if(*argv == 0)
		usage();
	sender = *argv++;
	fromm = s_copy(sender);
	rv = strrchr(s_to_c(fromm), '!');
	if(rv)
		*rv = 0;
	else
		*s_to_c(fromm) = 0;
	from = bangtoat(sender);

	/*
	 *  send the mail
	 */
	if(filter){
		Binit(&bin, 0, OREAD);
		Binit(&bout, 1, OWRITE);
	} else {
		/* 10 minutes to get through the initial handshake */
		notify(timeout);
		alarm(10*60*1000);

		if((rv = connect(addr)) != 0)
			exits(rv);
		if((rv = hello(hostdomain)) != 0)
			goto error;
		if((rv = mailfrom(s_to_c(from))) != 0)
			goto error;
		while(*argv)
			if((rv = rcptto(*argv++))!=0)
				goto error;

		alarm(0);
	}
	rv = data(from, unix);
	if(rv != 0)
		goto error;
	if(!filter)
		quit();
	exits("");
error:
	fprint(2, "%s\n", s_to_c(reply));
	exits(rv);
}

/*
 *  connect to the remote host
 */
char *
connect(char* net)
{
	char *addr;
	char buf[256];
	int fd;

	addr = netmkaddr(net, 0, "smtp");

	/* try connecting to destination or any of it's mail routers */
	fd = mxdial(addr, &gateway, ddomain);

	/* try our mail gateway */
	if(fd < 0 && gdomain){
		gateway = 1;
		fd = dial(netmkaddr(gdomain, 0, "smtp"), 0, 0, 0);
	}

	if(fd < 0){
		errstr(buf);
		Bprint(&berr, "smtp: %s %s\n", buf, addr);
		if(strstr(buf, "illegal") || strstr(buf, "rejected"))
			return Giveup;
		else
			return Retry;
	}
	Binit(&bin, fd, OREAD);
	fd = dup(fd, -1);
	Binit(&bout, fd, OWRITE);
	return 0;
}

/*
 *  exchange names with remote host
 */
char *
hello(char *me)
{
	switch(getreply()){
	case 2:
		break;
	case 5:
		return Giveup;
	default:
		return Retry;
	}
	dBprint("HELO %s\r\n", me);
	switch(getreply()){
	case 2:
		break;
	case 5:
		return Giveup;
	default:
		return Retry;
	}
	return 0;
}

/*
 *  report sender to remote
 */
char *
mailfrom(char *from)
{
	if(strchr(from, '@'))
		dBprint("MAIL FROM:<%s>\r\n", from);
	else
		dBprint("MAIL FROM:<%s@%s>\r\n", from, hostdomain);
	switch(getreply()){
	case 2:
		break;
	case 5:
		return Giveup;
	default:
		return Retry;
	}
	return 0;
}

/*
 *  report a recipient to remote
 */
char *
rcptto(char *to)
{
	String *s;

	s = bangtoat(to);
	if(toline == 0){
		toline = s_new();
		s_append(toline, "To: ");
	} else
		s_append(toline, ", ");
	s_append(toline, s_to_c(s));
	if(strchr(s_to_c(s), '@'))
		dBprint("RCPT TO:<%s>\r\n", s_to_c(s));
	else{
		s_append(toline, "@");
		s_append(toline, ddomain);
		dBprint("RCPT TO:<%s@%s>\r\n", s_to_c(s), ddomain);
	}
	switch(getreply()){
	case 2:
		break;
	case 5:
		return Giveup;
	default:
		return Retry;
	}
	return 0;
}

/*
 *  send the damn thing
 */
char *
data(String *from, int unix)
{
	char buf[16*1024];
	int i, n;
	int eof;
	static char errmsg[ERRLEN];

	/*
	 *  input the first 16k bytes.  The header had better fit.
	 */
	eof = 0;
	for(n = 0; n < sizeof(buf) - 1; n += i){
		i = read(0, buf+n, sizeof(buf)-1-n);
		if(i <= 0){
			eof = 1;
			break;
		}
	}
	buf[n] = 0;

	/*
	 *  parse the header, turn all addresses into @ format
	 */
	yyinit(buf);
	if(!unix){
		yyparse();
		convertheader(from);
	}

	/*
	 *  print message observing '.' escapes and using \r\n for \n
	 */
	if(!filter){
		dBprint("DATA\r\n");
		switch(getreply()){
		case 3:
			break;
		case 5:
			return Giveup;
		default:
			return Retry;
		}
	}

	/*
	 *  send header.  add a sender and a date if there
	 *  isn't one
	 */
	uneaten = buf;
	if(!unix){
		if(originator==0 && usender)
			Bprint(&bout, "From: %s\r\n", s_to_c(from));
		if(destination == 0 && toline)
			Bprint(&bout, "%s\r\n", s_to_c(toline));
		if(date==0 && udate)
			printdate(udate);
		if (usys)
			uneaten = usys->end + 1;
		printheader();
		if (*uneaten != '\n')
			putcrnl("\n", 1);
	}

	/*
	 *  send body
	 */
	putcrnl(uneaten, buf+n - uneaten);
	if(eof == 0)
		for(;;){
			n = read(0, buf, sizeof(buf));
			if(n < 0){
				errstr(errmsg);
				return errmsg;
			}
			if(n == 0)
				break;
			putcrnl(buf, n);
		}
	if(!filter){
		if(last != '\n')
			dBprint("\r\n.\r\n");
		else
			dBprint(".\r\n");
		switch(getreply()){
		case 2:
			break;
		case 5:
			return Retry;
		default:
			return Giveup;
		}
	}
	return 0;
}

/*
 *  we're leaving
 */
void
quit(void)
{
	dBprint("QUIT\r\n");
	getreply();
}

/*
 *  read a reply into a string, return the reply code
 */
int
getreply(void)
{
	char *line;
	int rv;

	reply = s_reset(reply);
	for(;;){
		line = getcrnl(reply);
		if(line == 0)
			return -1;
		if(!isdigit(line[0]) || !isdigit(line[1]) || !isdigit(line[2]))
			return -1;
		if(line[3] != '-')
			break;
	}
	rv = atoi(line)/100;
	return rv;
}

/*
 *	Convert from `bang' to `source routing' format.
 *
 *	   a.x.y!b.p.o!c!d ->	@a.x.y:c!d@b.p.o
 */
void
addhostdom(String *buf, char *host)
{
	s_append(buf, "@");
	s_append(buf, host);
}
String *
bangtoat(char *addr)
{
	String *buf;
	register int i;
	int j, d;
	char *field[128];

	/* parse the '!' format address */
	buf = s_new();
	for(i = 0; addr; i++){
		field[i] = addr;
		addr = strchr(addr, '!');
		if(addr)
			*addr++ = 0;
	}
	if (i==1) {
		s_append(buf, field[0]);
		return buf;
	}

	/*
	 *  count leading domain fields (non-domains don't count)
	 */
	d = 0;
	for( ; d<i-1; d++)
		if(strchr(field[d], '.')==0)
			break;
	/*
	 *  if there are more than 1 leading domain elements,
	 *  put them in as source routing
	 */
	if(d > 1){
		addhostdom(buf, field[0]);
		for(j=1; j<d-1; j++){
			s_append(buf, ",");
			s_append(buf, "@");
			s_append(buf, field[j]);
		}
		s_append(buf, ":");
	}

	/*
	 *  throw in the non-domain elements separated by '!'s
	 */
	s_append(buf, field[d]);
	for(j=d+1; j<=i-1; j++) {
		s_append(buf, "!");
		s_append(buf, field[j]);
	}
	if(d)
		addhostdom(buf, field[d-1]);
	return buf;
}

/*
 *  convert header addresses to @ format.
 *  if the address is a source address, and a domain is specified,
 *  make sure it falls in the domain.
 */
void
convertheader(String *from)
{
	Field *f;
	Node *p;
	String *a;

	if(!unix && strchr(s_to_c(from), '@') == 0){
		s_append(from, "@");
		s_append(from, hostdomain);
	}
	for(f = firstfield; f; f = f->next){
		for(p = f->node; p; p = p->next){
			if(!p->addr)
				continue;
			a = bangtoat(s_to_c(p->s));
			s_free(p->s);
			if(!unix && strchr(s_to_c(a), '@') == 0){
				s_append(a, "@");
				s_append(a, hostdomain);
			}
			p->s = a;
		}
	}
}

/*
 *  print out the parsed header
 */
void
printheader(void)
{
	Field *f;
	Node *p;
	char *cp;
	char c[1];

	for(f = firstfield; f; f = f->next){
		for(p = f->node; p; p = p->next){
			if(p->s)
				Bprint(&bout, "%s", s_to_c(p->s));
			else {
				c[0] = p->c;
				putcrnl(c, 1);
			}
			if(p->white){
				cp = s_to_c(p->white);
				putcrnl(cp, strlen(cp));
			}
			uneaten = p->end;
		}
		putcrnl("\n", 1);
		uneaten++;		/* skip newline */
	}
}

/*
 *  add a domain onto an name, return the new name
 */
char *
domainify(char *name, char *domain)
{
	static String *s;

	if(domain==0 || strchr(name, '.')!=0)
		return name;

	s = s_reset(s);
	s_append(s, name);
	if(*domain != '.')
		s_append(s, ".");
	s_append(s, domain);
	return s_to_c(s);
}

/*
 *  print message observing '.' escapes and using \r\n for \n
 */
void
putcrnl(char *cp, int n)
{
	int c;

	for(; n; n--, cp++){
		c = *cp;
		if(c == '\n')
			Bputc(&bout, '\r');
		else if(c == '.' && last=='\n')
			Bputc(&bout, '.');
		Bputc(&bout, c);
		last = c;
	}
}

/*
 *  Get a line including a crnl into a string.  Convert crnl into nl.
 */
char *
getcrnl(String *s)
{
	int c;
	int count;

	count = 0;
	for(;;){
		c = Bgetc(&bin);
		if(debug)
			Bputc(&berr, c);
		switch(c){
		case -1:
			s_terminate(s);
			fprint(2, "smtp: connection closed unexpectedly by remote system\n");
			return 0;
		case '\r':
			c = Bgetc(&bin);
			if(c == '\n'){
				s_putc(s, c);
				if(debug)
					Bputc(&berr, c);
				count++;
				s_terminate(s);
				return s->ptr - count;
			}
			Bungetc(&bin);
			s_putc(s, '\r');
			if(debug)
				Bputc(&berr, '\r');
			count++;
			break;
		default:
			s_putc(s, c);
			count++;
			break;
		}
	}
	return 0;
}

/*
 *  print out a parsed date
 */
void
printdate(Node *p)
{
	int sep = 0;

	Bprint(&bout, "Date: %s,", s_to_c(p->s));
	for(p = p->next; p; p = p->next){
		if(p->s){
			if(sep == 0)
				Bputc(&bout, ' ');
			if (p->next)
				Bprint(&bout, "%s", s_to_c(p->s));
			else
				Bprint(&bout, "%s", rewritezone(s_to_c(p->s)));
			sep = 0;
		} else {
			Bputc(&bout, p->c);
			sep = 1;
		}
	}
	Bprint(&bout, "\r\n");
}

char *
rewritezone(char *z)
{
	Tm *t;
	int h, m, offset;
	char sign, *p;
	char *zones = "ECMP";
	static char numeric[6];

	t = localtime(0);
	if (t->hour >= 12) {
		sign = '-';
		if (t->min == 0) {
			h = 24 - t->hour;
			m = 0;
		}
		else {
			h = 23 - t->hour;
			m = 60 - t->min;
		}
	}
	else {
		sign = '+';
		h = t->hour;
		m = t->min;
	}
	sprint(numeric, "%c%.2d%.2d", sign, h, m);

	/* leave zone alone if we didn't generate it */
	if (strncmp(z, t->zone, 4) != 0)
		return z;
	if (strcmp(z, "GMT") == 0 || strcmp(z, "UT") == 0)
		if (t->hour == 0 && t->min == 0)
			return z;
		else
			return numeric;
	/* check for North American time zone */
	if (z[2] == 'T' && (z[1] == 'S' || z[1] == 'D')) {
		p = strchr(zones, z[0]);
		if (p) {
			offset = 24 - 5 - (p - zones) + (z[1] == 'D');
			if (offset == t->hour)
				return z;
		}
	}
	return numeric;
}

/*
 *  stolen from libc/port/print.c
 */
#define	SIZE	4096
#define	DOTDOT	(&fmt+1)
int
dBprint(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	if(debug){
		Bwrite(&berr, buf, (long)(out-buf));
		Bflush(&berr);
	}
	n = Bwrite(&bout, buf, (long)(out-buf));
	Bflush(&bout);
	return n;
}
