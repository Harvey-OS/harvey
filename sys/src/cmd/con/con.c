#include <u.h>
#include <libc.h>

int debug;		/* true if debugging */
int ctl = -1;		/* control fd (for break's) */
int raw;		/* true if raw is on */
int consctl = -1;	/* control fd for cons */
int ttypid;		/* pid's if the 2 processes (used to kill them) */
int outfd = 1;		/* local output file descriptor */
int cooked;		/* non-zero forces cooked mode */
int returns;		/* non-zero forces carriage returns not to be filtered out */
int crtonl;			/* non-zero forces carriage returns to be converted to nls coming from net */
int	strip;		/* strip off parity bits */
char firsterr[2*ERRMAX];
char transerr[2*ERRMAX];
int limited;
char *remuser;		/* for BSD rlogin authentication */
int verbose;
int baud;
int notkbd;
int nltocr;		/* translate kbd nl to cr  and vice versa */

static char *srv;

#define MAXMSG (2*8192)

int	dodial(char*, char*, char*);
void	fromkbd(int);
void	fromnet(int);
long	iread(int, void*, int);
long	iwrite(int, void*, int);
int	menu(int);
void	notifyf(void*, char*);
void	pass(int, int, int);
void	rawoff(void);
void	rawon(void);
void	stdcon(int);
char*	system(int, char*);
void	dosystem(int, char*);
int	wasintr(void);
void	punt(char*);
char*	syserr(void);
void	seterr(char*);

/* protocols */
void	device(char*, char*);
void	rlogin(char*, char*);
void	simple(char*, char*);

void
usage(void)
{
	punt("usage: con [-CdnrRsTv] [-b baud] [-l [user]] [-c cmd] [-S svc] "
		"net!host[!service]");
}

void
main(int argc, char *argv[])
{
	char *dest;
	char *cmd = 0;

	returns = 1;
	ARGBEGIN{
	case 'b':
		baud = atoi(EARGF(usage()));
		break;
	case 'C':
		cooked = 1;
		break;
	case 'c':
		cmd = EARGF(usage());
		break;
	case 'd':
		debug = 1;
		break;
	case 'l':
		limited = 1;
		if(argv[1] != nil && argv[1][0] != '-')
			remuser = EARGF(usage());
		break;
	case 'n':
		notkbd = 1;
		break;
	case 'r':
		returns = 0;
		break;
	case 's':
		strip = 1;
		break;
	case 'S':
		srv = EARGF(usage());
		break;
	case 'R':
		nltocr = 1;
		break;
	case 'T':
		crtonl = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1){
		if(remuser == 0)
			usage();
		dest = remuser;
		remuser = 0;
	} else
		dest = argv[0];
	if(*dest == '/' && strchr(dest, '!') == 0)
		device(dest, cmd);
	else if(limited){
		simple(dest, cmd);	/* doesn't return if dialout succeeds */
		rlogin(dest, cmd);	/* doesn't return if dialout succeeds */
	} else {
		rlogin(dest, cmd);	/* doesn't return if dialout succeeds */
		simple(dest, cmd);	/* doesn't return if dialout succeeds */
	}
	punt(firsterr);
}

/*
 *  just dial and use as a byte stream with remote echo
 */
void
simple(char *dest, char *cmd)
{
	int net;

	net = dodial(dest, 0, 0);
	if(net < 0)
		return;

	if(cmd)
		dosystem(net, cmd);

	if(!cooked)
		rawon();
	stdcon(net);
	exits(0);
}

/*
 *  dial, do UCB authentication, use as a byte stream with local echo
 *
 *  return if dial failed
 */
void
rlogin(char *dest, char *cmd)
{
	int net;
	char buf[128];
	char *p;
	char *localuser;

	/* only useful on TCP */
	if(strchr(dest, '!')
	&& (strncmp(dest, "tcp!", 4)!=0 && strncmp(dest, "net!", 4)!=0))
		return;

	net = dodial(dest, "tcp", "login");
	if(net < 0)
		return;

	/*
	 *  do UCB rlogin authentication
	 */
	localuser = getuser();
	if(remuser == 0){
		if(limited)
			remuser = ":";
		else
			remuser = localuser;
	}
	p = getenv("TERM");
	if(p == 0)
		p = "p9";
	if(write(net, "", 1)<0
	|| write(net, localuser, strlen(localuser)+1)<0
	|| write(net, remuser, strlen(remuser)+1)<0
	|| write(net, p, strlen(p)+1)<0){
		close(net);
		punt("BSD authentication failed");
	}
	if(read(net, buf, 1) != 1)
		punt("BSD authentication failed1");
	if(buf[0] != 0){
		fprint(2, "con: remote error: ");
		while(read(net, buf, 1) == 1){
			write(2, buf, 1);
			if(buf[0] == '\n')
				break;
		}
		exits("read");
	}

	if(cmd)
		dosystem(net, cmd);

	if(!cooked)
		rawon();
	nltocr = 1;
	stdcon(net);
	exits(0);
}

/*
 *  just open a device and use it as a connection
 */
void
device(char *dest, char *cmd)
{
	int net;
	char cname[128];

	net = open(dest, ORDWR);
	if(net < 0) {
		fprint(2, "con: cannot open %s: %r\n", dest);
		exits("open");
	}
	snprint(cname, sizeof cname, "%sctl", dest);
	ctl = open(cname, ORDWR);
	if (baud > 0) {
		if(ctl >= 0){
			/* set speed and use fifos if available */
			fprint(ctl, "b%d i1", baud);
		}
		else
			fprint(2, "con: cannot open %s: %r\n", cname);
	}

	if(cmd)
		dosystem(net, cmd);

	if(!cooked)
		rawon();
	stdcon(net);
	exits(0);
}

/*
 *  ignore interrupts
 */
void
notifyf(void *a, char *msg)
{
	USED(a);

	if(strstr(msg, "yankee"))
		noted(NDFLT);
	if(strstr(msg, "closed pipe")
	|| strcmp(msg, "interrupt") == 0
	|| strcmp(msg, "hangup") == 0)
		noted(NCONT);
	noted(NDFLT);
}

/*
 *  turn keyboard raw mode on
 */
void
rawon(void)
{
	if(debug)
		fprint(2, "rawon\n");
	if(raw)
		return;
	if(consctl < 0)
		consctl = open("/dev/consctl", OWRITE);
	if(consctl < 0){
//		fprint(2, "can't open consctl\n");
		return;
	}
	write(consctl, "rawon", 5);
	raw = 1;
}

/*
 *  turn keyboard raw mode off
 */
void
rawoff(void)
{
	if(debug)
		fprint(2, "rawoff\n");
	if(raw == 0)
		return;
	if(consctl < 0)
		consctl = open("/dev/consctl", OWRITE);
	if(consctl < 0){
//		fprint(2, "can't open consctl\n");
		return;
	}
	write(consctl, "rawoff", 6);
	raw = 0;
}

/*
 *  control menu
 */
#define STDHELP	"\t(b)reak, (q)uit, (i)nterrupt, toggle printing (r)eturns, (.)continue, (!cmd)\n"

int
menu(int net)
{
	char buf[MAXMSG];
	long n;
	int done;
	int wasraw = raw;

	if(wasraw)
		rawoff();

	fprint(2, ">>> ");
	for(done = 0; !done; ){
		n = read(0, buf, sizeof(buf)-1);
		if(n <= 0)
			return -1;
		buf[n] = 0;
		switch(buf[0]){
		case '!':
			print(buf);
			system(net, buf+1);
			print("!\n");
			done = 1;
			break;
		case '.':
			done = 1;
			break;
		case 'q':
			return -1;
		case 'i':
			buf[0] = 0x1c;
			write(net, buf, 1);
			done = 1;
			break;
		case 'b':
			if(ctl >= 0)
				write(ctl, "k", 1);
			done = 1;
			break;
		case 'r':
			returns = 1-returns;
			done = 1;
			break;
		default:
			fprint(2, STDHELP);
			break;
		}
		if(!done)
			fprint(2, ">>> ");
	}

	if(wasraw)
		rawon();
	else
		rawoff();
	return 0;
}

void
post(char *srv, int fd)
{
	int f;
	char buf[32];

	f = create(srv, OWRITE /* |ORCLOSE */ , 0666);
	if(f < 0)
		sysfatal("create %s: %r", srv);
	snprint(buf, sizeof buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		sysfatal("write %s: %r", srv);
	close(f);
}

/*
 *  the real work.  two processes pass bytes back and forth between the
 *  terminal and the network.
 */
void
stdcon(int net)
{
	int netpid;
	int p[2];
	char *svc;

	svc = nil;
	if (srv) {
		if(pipe(p) < 0)
			sysfatal("pipe: %r");
		if (srv[0] != '/')
			svc = smprint("/srv/%s", srv);
		else
			svc = srv;
		post(svc, p[0]);
		close(p[0]);
		dup(p[1], 0);
		dup(p[1], 1);
		/* pipe is now std in & out */
	}
	ttypid = getpid();
	switch(netpid = rfork(RFMEM|RFPROC)){
	case -1:
		perror("con");
		exits("fork");
	case 0:
		notify(notifyf);
		fromnet(net);
		if (svc)
			remove(svc);
		postnote(PNPROC, ttypid, "die yankee dog");
		exits(0);
	default:
		notify(notifyf);
		fromkbd(net);
		if (svc)
			remove(svc);
		if(notkbd)
			for(;;)
				sleep(0);
		postnote(PNPROC, netpid, "die yankee dog");
		exits(0);
	}
}

/*
 *  Read the keyboard and write it to the network.  '^\' gets us into
 *  the menu.
 */
void
fromkbd(int net)
{
	long n;
	char buf[MAXMSG];
	char *p, *ep;
	int eofs;

	eofs = 0;
	for(;;){
		n = read(0, buf, sizeof(buf));
		if(n < 0){
			if(wasintr()){
				if(!raw){
					buf[0] = 0x7f;
					n = 1;
				} else
					continue;
			} else
				return;
		}
		if(n == 0){
			if(++eofs > 32)
				return;
		} else
			eofs = 0;
		if(n && memchr(buf, 0x1c, n)){
			if(menu(net) < 0)
				return;
		}else{
			if(!raw && n==0){
				buf[0] = 0x4;
				n = 1;
			}
			if(nltocr){
				ep = buf+n;
				for(p = buf; p < ep; p++)
					switch(*p){
					case '\r':
						*p = '\n';
						break;
					case '\n':
						*p = '\r';
						break;
					}
			}
			if(iwrite(net, buf, n) != n)
				return;
		}
	}
}

/*
 *  Read from the network and write to the screen.
 *  Filter out spurious carriage returns.
 */
void
fromnet(int net)
{
	long n;
	char buf[MAXMSG];
	char *cp, *ep;

	for(;;){
		n = iread(net, buf, sizeof(buf));
		if(n < 0)
			return;
		if(n == 0)
			continue;

		if (strip)
			for (cp=buf; cp<buf+n; cp++)
				*cp &= 0177;

		if(crtonl) {
			/* convert cr's to nl's */
			for (cp = buf; cp < buf + n; cp++)
				if (*cp == '\r')
					*cp = '\n';
		}
		else if(!returns){
			/* convert cr's to null's */
			cp = buf;
			ep = buf + n;
			while(cp < ep && (cp = memchr(cp, '\r', ep-cp))){
				memmove(cp, cp+1, ep-cp-1);
				ep--;
				n--;
			}
		}

		if(n > 0 && iwrite(outfd, buf, n) != n){
			if(outfd == 1)
				return;
			outfd = 1;
			if(iwrite(1, buf, n) != n)
				return;
		}
	}
}

/*
 *  dial and return a data connection
 */
int
dodial(char *dest, char *net, char *service)
{
	char name[128];
	char devdir[128];
	int data;

	devdir[0] = 0;
	strcpy(name, netmkaddr(dest, net, service));
	data = dial(name, 0, devdir, &ctl);
	if(data < 0){
		seterr(name);
		return -1;
	}
	fprint(2, "connected to %s on %s\n", name, devdir);
	return data;
}

void
dosystem(int fd, char *cmd)
{
	char *p;

	p = system(fd, cmd);
	if(p){
		print("con: %s terminated with %s\n", cmd, p);
		exits(p);
	}
}

/*
 *  run a command with the network connection as standard IO
 */
char *
system(int fd, char *cmd)
{
	int pid;
	int p;
	static Waitmsg msg;
	int pfd[2];
	int n;
	char buf[4096];

	if(pipe(pfd) < 0){
		perror("pipe");
		return "pipe failed";
	}
	outfd = pfd[1];

	close(consctl);
	consctl = -1;
	switch(pid = fork()){
	case -1:
		perror("con");
		return "fork failed";
	case 0:
		close(pfd[1]);
		dup(pfd[0], 0);
		dup(fd, 1);
		close(ctl);
		close(fd);
		close(pfd[0]);
		if(*cmd)
			execl("/bin/rc", "rc", "-c", cmd, nil);
		else
			execl("/bin/rc", "rc", nil);
		perror("con");
		exits("exec");
		break;
	default:
		close(pfd[0]);
		while((n = read(pfd[1], buf, sizeof(buf))) > 0)
			if(write(fd, buf, n) != n)
				break;
		p = waitpid();
		outfd = 1;
		close(pfd[1]);
		if(p < 0 || p != pid)
			return "lost child";
		break;
	}
	return msg.msg;
}

int
wasintr(void)
{
	return strcmp(syserr(), "interrupted") == 0;
}

void
punt(char *msg)
{
	if(*msg == 0)
		msg = transerr;
	fprint(2, "con: %s\n", msg);
	exits(msg);
}

char*
syserr(void)
{
	static char err[ERRMAX];
	errstr(err, sizeof err);
	return err;
}

void
seterr(char *addr)
{
	char *se = syserr();

	if(verbose)
		fprint(2, "'%s' calling %s\n", se, addr);
	if(firsterr[0] && (strstr(se, "translate") ||
	 strstr(se, "file does not exist") ||
	 strstr(se, "unknown address") ||
	 strstr(se, "directory entry not found")))
		return;
	strcpy(firsterr, se);
}


long
iread(int f, void *a, int n)
{
	long m;

	for(;;){
		m = read(f, a, n);
		if(m >= 0 || !wasintr())
			break;
	}
	return m;
}

long
iwrite(int f, void *a, int n)
{
	long m;

	m = write(f, a, n);
	if(m < 0 && wasintr())
		return n;
	return m;
}
