#include <u.h>
#include <libc.h>

#include "rustream.h"
#include "ruttyio.h"
#include "rusignal.h"
#include "rufilio.h"

int debug;		/* true if debugging */
int ctl = -1;		/* control fd (for break's) */
int raw;		/* true if raw is on */
int consctl = -1;	/* control fd for cons */
int ttypid;		/* pid's if the 2 processes (used to kill them) */
int msgfd = -1;		/* mesgld file descriptor (for signals to be written to) */
int outfd = 1;		/* local output file descriptor */
int cooked;		/* non-zero forces cooked mode */
int returns;		/* non-zero forces carriage returns not to be filtered out */
int crtonl;			/* non-zero forces carriage returns to be converted to nls coming from net */
int	strip;		/* strip off parity bits */
char firsterr[2*ERRMAX];
char transerr[2*ERRMAX];
int limited;
char *remuser;
int verbose;
int baud;
int notkbd;
int nltocr;		/* translate kbd nl to cr  and vice versa */

typedef struct Msg Msg;
#define MAXMSG (2*8192)

int	dodial(char*, char*, char*);
void	fromkbd(int);
void	fromnet(int);
long	iread(int, void*, int);
long	iwrite(int, void*, int);
int	menu(int);
void	msgfromkbd(int);
void	msgfromnet(int);
int	msgwrite(int, void*, int);
void	notifyf(void*, char*);
void	pass(int, int, int);
void	rawoff(void);
void	rawon(void);
int	readupto(int, char*, int);
int	sendctl(int, int);
int	sendctl1(int, int, int);
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
	punt("usage: con [-CdnrRsTv] [-b baud] [-l [user]] [-c cmd] net!host[!service]");
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
	case 'd':
		debug = 1;
		break;
	case 'l':
		limited = 1;
		if(argv[1] != nil && argv[1][0] != '-')
			remuser = ARGF();
		break;
	case 'n':
		notkbd = 1;
		break;
	case 'r':
		returns = 0;
		break;
	case 'R':
		nltocr = 1;
		break;
	case 'T':
		crtonl = 1;
		break;
	case 'C':
		cooked = 1;
		break;
	case 'c':
		cmd = ARGF();
		break;
	case 'v':
		verbose = 1;
		break;
	case 's':
		strip = 1;
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
		if(ctl >= 0)
			fprint(ctl, "b%d", baud);
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
			if(msgfd <= 0)
				write(net, buf, 1);
			else
				sendctl1(msgfd, M_SIGNAL, SIGQUIT);
			done = 1;
			break;
		case 'b':
			if(msgfd >= 0)
				sendctl(msgfd, M_BREAK);
			else if(ctl >= 0)
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

/*
 *  the real work.  two processes pass bytes back and forth between the
 *  terminal and the network.
 */
void
stdcon(int net)
{
	int netpid;

	ttypid = getpid();
	switch(netpid = rfork(RFMEM|RFPROC)){
	case -1:
		perror("con");
		exits("fork");
	case 0:
		notify(notifyf);
		fromnet(net);
		postnote(PNPROC, ttypid, "die yankee dog");
		exits(0);
	default:
		notify(notifyf);
		fromkbd(net);
		if(notkbd)
			for(;;)sleep(0);
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
		while((n = read(pfd[1], buf, sizeof(buf))) > 0){
			if(msgfd >= 0){
				if(msgwrite(fd, buf, n) != n)
					break;
			} else {
				if(write(fd, buf, n) != n)
					break;
			}
		}
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

/*
 *  The rest is to support the V10 mesgld protocol.
 */

/*
 *  network orderings
 */
#define get2byte(p) ((p)[0] + ((p)[1]<<8))
#define get4byte(p) ((p)[0] + ((p)[1]<<8) + ((p)[2]<<16) + ((p)[3]<<24))
#define put2byte(p, i) ((p)[0]=(i), (p)[1]=(i)>>8)
#define put4byte(p, i) ((p)[0]=(i), (p)[1]=(i)>>8, (p)[2]=(i)>>16, (p)[3]=(i)>>24)

/*
 *  tty parameters
 */
int sgflags = ECHO;

/*
 *  a mesgld message
 */
struct Msg {
	struct mesg h;
	char b[MAXMSG];
};


/*
 *  send an empty mesgld message
 */
int
sendctl(int net, int type)
{
	Msg m;

	m.h.type = type;
	m.h.magic = MSGMAGIC;
	put2byte(m.h.size, 0);
	if(iwrite(net, &m, sizeof(struct mesg)) != sizeof(struct mesg))
		return -1;
	return 0;
}

/*
 *  send a one byte mesgld message
 */
int
sendctl1(int net, int type, int parm)
{
	Msg m;

	m.h.type = type;
	m.h.magic = MSGMAGIC;
	m.b[0] = parm;
	put2byte(m.h.size, 1);
	if(iwrite(net, &m, sizeof(struct mesg)+1) != sizeof(struct mesg)+1)
		return -1;
	return 0;
}

/*
 *  read n bytes.  return -1 if it fails, 0 otherwise.
 */
int
readupto(int from, char *a, int len)
{
	int n;

	while(len > 0){
		n = iread(from, a, len);
		if(n < 0)
			return -1;
		a += n;
		len -= n;
	}
	return 0;
}

/*
 *  Decode a mesgld message from the network
 */
void
msgfromnet(int net)
{
	ulong com;
	struct stioctl *io;
	struct sgttyb *sg;
	struct ttydevb *td;
	struct tchars *tc;
	int len;
	Msg m;

	for(;;){
		/* get a complete mesgld message */
		if(readupto(net, (char*)&m.h, sizeof(struct mesg)) < 0)
			break;
		if(m.h.magic != MSGMAGIC){
			fprint(2, "con: bad message magic 0x%ux\n", m.h.magic);
			break;
		}
		len = get2byte(m.h.size);
		if(len > sizeof(m.b)){
			len = sizeof(m.b);
			fprint(2, "con: mesgld message too long\n");
		}
		if(len && readupto(net, m.b, len) < 0)
			break;

		/* decode */
		switch(m.h.type){
		case M_HANGUP:
			if(debug)
				fprint(2, "M_HANGUP\n");
			return;
		case M_DATA:
			if(debug)
				fprint(2, "M_DATA %d bytes\n", len);
			if(iwrite(outfd, m.b, len) != len){
				if(outfd == 1)
					return;
				outfd = 1;
				if(iwrite(outfd, m.b, len) != len)
					return;
			}
			continue;
		case M_IOCTL:
			break;
		default:
			/* ignore */
			if(debug)
				fprint(2, "con: unknown message\n");
			continue;
		}
	
		/*
		 *  answer an ioctl
		 */
		io = (struct stioctl *)m.b;
		com = get4byte(io->com);
		if(debug)
			fprint(2, "M_IOCTL %lud\n", com);
		switch(com){
		case FIOLOOKLD:
			put4byte(io->data, tty_ld);
			len = 0;
			break;
		case TIOCGETP:
			sg = (struct sgttyb *)io->data;
			sg->sg_ispeed = sg->sg_ospeed = B9600;
			sg->sg_erase = 0010;	/* back space */
			sg->sg_kill = 0025;	/* CNTL U */
			put2byte(sg->sg_flags, sgflags);
			len = sizeof(struct sgttyb);
			break;
		case TIOCSETN:
		case TIOCSETP:
			sg = (struct sgttyb *)io->data;
			sgflags = get2byte(sg->sg_flags);
			if((sgflags&(RAW|CBREAK)) || !(sgflags&ECHO))
				rawon();
			else
				rawoff();
			len = 0;
			break;
		case TIOCGETC:
			tc = (struct tchars *)io->data;
			tc->t_intrc = 0177;
			tc->t_quitc = 0034;
			tc->t_startc = 0;
			tc->t_stopc = 0;
			tc->t_eofc = 0004;
			tc->t_brkc = 0;
			len = sizeof(struct tchars);
			break;
		case TIOCSETC:
			len = 0;
			break;
		case TIOCGDEV:
			td = (struct ttydevb *)io->data;
			td->ispeed = td->ospeed = B9600;
			put2byte(td->flags, 0);
			len = sizeof(struct ttydevb);
			break;
		case TIOCSDEV:
			len = 0;
			break;
		default:
			/*
			 *  unimplemented
			 */
			m.b[len] = 0;
			if(sendctl(net, M_IOCNAK) < 0)
				return;
			continue;
		}
	
		/*
		 *  acknowledge
		 */
		m.h.type = M_IOCACK;
		m.h.magic = MSGMAGIC;
		len += 4;
		put2byte(m.h.size, len);
		len += sizeof(struct mesg);
		if(iwrite(net, &m, len) != len)
			return;
	}
}

/*
 *  Read the keyboard, convert to mesgld messages, and write it to the network.
 *  '^\' gets us into the menu.
 */
void
msgfromkbd(int net)
{
	long n;
	char buf[MAXMSG];

	for(;;){
		n = iread(0, buf, sizeof(buf));
		if(n < 0)
			return;
		if(n && memchr(buf, 0034, n)){
			if(menu(net) < 0)
				return;
		} else {
			if(msgwrite(net, buf, n) != n)
				return;
		}
	}
}

int
msgwrite(int fd, void *buf, int len)
{
	Msg m;
	int n;

	n = len;
	memmove(m.b, buf, n);
	put2byte(m.h.size, n);
	m.h.magic = MSGMAGIC;
	m.h.type = M_DATA;
	n += sizeof(struct mesg);
	if(iwrite(fd, &m, n) != n)
		return -1;
	
	put2byte(m.h.size, 0);
	m.h.magic = MSGMAGIC;
	m.h.type = M_DELIM;
	n = sizeof(struct mesg);
	if(iwrite(fd, &m, n) != n)
		return -1;

	return len;
}

