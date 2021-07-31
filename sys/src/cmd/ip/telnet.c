#include <u.h>
#include <libc.h>
#include <bio.h>
#include "telnet.h"

int ctl = -1;		/* control fd (for break's) */
int consctl = -1;	/* consctl fd */

int ttypid;		/* pid's if the 2 processes (used to kill them) */
int netpid;
int interrupted;
int localecho;
int notkbd;

static char *srv;

typedef struct Comm Comm;
struct Comm {
	int returns;
	int stopped;
};
Comm *comm;

int	dodial(char*);
void	fromkbd(int);
void	fromnet(int);
int	menu(Biobuf*,  int);
void	notifyf(void*, char*);
void	rawoff(void);
void	rawon(void);
void	telnet(int);
char*	system(int, char*);
int	echochange(Biobuf*, int);
int	termsub(Biobuf*, uchar*, int);
int	xlocsub(Biobuf*, uchar*, int);
void*	share(ulong);

static int islikeatty(int);

void
usage(void)
{
	fatal("usage: telnet [-Cdnr] [-s srv] net!host[!service]", 0, 0);
}

void
main(int argc, char *argv[])
{
	int returns;

	returns = 1;
	ARGBEGIN{
	case 'C':
		opt[Echo].noway = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'n':
		notkbd = 1;
		break; 
	case 'r':
		returns = 0;
		break;
	case 's':
		srv = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	/* options we need routines for */
	opt[Echo].change = echochange;
	opt[Term].sub = termsub;
	opt[Xloc].sub = xlocsub;

	comm = share(sizeof(comm));
	comm->returns = returns;

	telnet(dodial(argv[0]));
}

/*
 *  dial and return a data connection
 */
int
dodial(char *dest)
{
	char *name;
	int data;
	char devdir[NETPATHLEN];

	name = netmkaddr(dest, "tcp", "telnet");
	data = dial(name, 0, devdir, 0);
	if(data < 0)
		fatal("%s: %r", name, 0);
	fprint(2, "connected to %s on %s\n", name, devdir);
	return data;
}

void
post(char *srv, int fd)
{
	int f;
	char buf[32];

	f = create(srv, OWRITE, 0666);
	if(f < 0)
		sysfatal("create %s: %r", srv);
	snprint(buf, sizeof buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != strlen(buf))
		sysfatal("write %s: %r", srv);
	close(f);
}

/*
 *  two processes pass bytes back and forth between the
 *  terminal and the network.
 */
void
telnet(int net)
{
	int pid;
	int p[2];
	char *svc;

	rawoff();
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
	switch(pid = rfork(RFPROC|RFFDG|RFMEM)){
	case -1:
		perror("con");
		exits("fork");
	case 0:
		rawoff();
		notify(notifyf);
		fromnet(net);
		if (svc)
			remove(svc);
		sendnote(ttypid, "die");
		exits(0);
	default:
		netpid = pid;
		notify(notifyf);
		fromkbd(net);
		if(notkbd)
			for(;;)
				sleep(0);
		if (svc)
			remove(svc);
		sendnote(netpid, "die");
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
	Biobuf ib, ob;
	int c, likeatty;
	int eofs;

	Binit(&ib, 0, OREAD);
	Binit(&ob, net, OWRITE);

	likeatty = islikeatty(0);
	eofs = 0;
	for(;;){
		c = Bgetc(&ib);

		/*
		 *  with raw off, all ^D's get turned into Eof's.
		 *  change them back.
		 *  10 in a row implies that the terminal is really gone so
		 *  just hang up.
		 */
		if(c < 0){
			if(notkbd)
				return;
			if(eofs++ > 10)
				return;
			c = 004;
		} else
			eofs = 0;

		/*
		 *  if not in binary mode, look for the ^\ escape to menu.
		 *  also turn \n into \r\n
		 */
		if(likeatty || !opt[Binary].local){
			if(c == 0034){ /* CTRL \ */
				if(Bflush(&ob) < 0)
					return;
				if(menu(&ib, net) < 0)
					return;
				continue;
			}
		}
		if(!opt[Binary].local){
			if(c == '\n'){
				/*
				 *  This is a very strange use of the SGA option.
				 *  I did this because some systems that don't
				 *  announce a willingness to supress-go-ahead
				 *  need the \r\n sequence to recognize input.
				 *  If someone can explain this to me, please
				 *  send me mail. - presotto
				 */
				if(opt[SGA].remote){
					c = '\r';
				} else {
					if(Bputc(&ob, '\r') < 0)
						return;
				}
			}
		}
		if(Bputc(&ob, c) < 0)
			return;
		if(Bbuffered(&ib) == 0)
			if(Bflush(&ob) < 0)
				return;
	}
}

/*
 *  Read from the network and write to the screen.  If 'stopped' is set
 *  spin and don't read.  Filter out spurious carriage returns.
 */
void
fromnet(int net)
{
	int c;
	int crnls = 0, freenl = 0, eofs;
	Biobuf ib, ob;

	Binit(&ib, net, OREAD);
	Binit(&ob, 1, OWRITE);
	eofs = 0;
	for(;;){
		if(Bbuffered(&ib) == 0)
			Bflush(&ob);
		if(interrupted){
			interrupted = 0;
			send2(net, Iac, Interrupt);
		}
		c = Bgetc(&ib);
		if(c < 0){
			if(eofs++ >= 2)
				return;
			continue;
		}
		eofs = 0;
		switch(c){
		case '\n':	/* skip nl after string of cr's */
			if(!opt[Binary].local && !comm->returns){
				++crnls;
				if(freenl == 0)
					break;
				freenl = 0;
				continue;
			}
			break;
		case '\r':	/* first cr becomes nl, remainder dropped */
			if(!opt[Binary].local && !comm->returns){
				if(crnls++ == 0){
					freenl = 1;
					c = '\n';
					break;
				}
				continue;
			}
			break;
		case 0:		/* remove nulls from crnl string */
			if(crnls)
				continue;
			break;

		case Iac:
			crnls = 0;
			freenl = 0;
			c = Bgetc(&ib);
			if(c == Iac)
				break;
			if(Bflush(&ob) < 0)
				return;
			if(control(&ib, c) < 0)
				return;
			continue;

		default:
			crnls = 0;
			freenl = 0;
			break;
		}
		if(Bputc(&ob, c) < 0)
			return;
	}
}

/*
 *  turn keyboard raw mode on
 */
void
rawon(void)
{
	if(debug)
		fprint(2, "rawon\n");
	if(consctl < 0)
		consctl = open("/dev/consctl", OWRITE);
	if(consctl < 0){
		fprint(2, "can't open consctl: %r\n");
		return;
	}
	write(consctl, "rawon", 5);
}

/*
 *  turn keyboard raw mode off
 */
void
rawoff(void)
{
	if(debug)
		fprint(2, "rawoff\n");
	if(consctl < 0)
		consctl = open("/dev/consctl", OWRITE);
	if(consctl < 0){
		fprint(2, "can't open consctl: %r\n");
		return;
	}
	write(consctl, "rawoff", 6);
}

/*
 *  control menu
 */
#define STDHELP	"\t(b)reak, (i)nterrupt, (q)uit, (r)eturns, (!cmd), (.)continue\n"

int
menu(Biobuf *bp, int net)
{
	char *cp;
	int done;

	comm->stopped = 1;

	rawoff();
	fprint(2, ">>> ");
	for(done = 0; !done; ){
		cp = Brdline(bp, '\n');
		if(cp == 0){
			comm->stopped = 0;
			return -1;
		}
		cp[Blinelen(bp)-1] = 0;
		switch(*cp){
		case '!':
			system(Bfildes(bp), cp+1);
			done = 1;
			break;
		case '.':
			done = 1;
			break;
		case 'q':
			comm->stopped = 0;
			return -1;
		case 'o':
			switch(*(cp+1)){
			case 'd':
				send3(net, Iac, Do, atoi(cp+2));
				break;
			case 'w':
				send3(net, Iac, Will, atoi(cp+2));
				break;
			}
			break;
		case 'r':
			comm->returns = !comm->returns;
			done = 1;
			break;
		case 'i':
			send2(net, Iac, Interrupt);
			break;
		case 'b':
			send2(net, Iac, Break);
			break;
		default:
			fprint(2, STDHELP);
			break;
		}
		if(!done)
			fprint(2, ">>> ");
	}

	rawon();
	comm->stopped = 0;
	return 0;
}

/*
 *  ignore interrupts
 */
void
notifyf(void *a, char *msg)
{
	USED(a);
	if(strcmp(msg, "interrupt") == 0){
		interrupted = 1;
		noted(NCONT);
	}
	if(strcmp(msg, "hangup") == 0)
		noted(NCONT);
	noted(NDFLT);
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

	if((pid = fork()) == -1){
		perror("con");
		return "fork failed";
	}
	else if(pid == 0){
		dup(fd, 0);
		close(ctl);
		close(fd);
		if(*cmd)
			execl("/bin/rc", "rc", "-c", cmd, nil);
		else
			execl("/bin/rc", "rc", nil);
		perror("con");
		exits("exec");
	}
	for(p = waitpid(); p >= 0; p = waitpid()){
		if(p == pid)
			return msg.msg;	
	}
	return "lost child";
}

/*
 *  suppress local echo if the remote side is doing it
 */
int
echochange(Biobuf *bp, int cmd)
{
	USED(bp);

	switch(cmd){
	case Will:
		rawon();
		break;
	case Wont:
		rawoff();
		break;
	}
	return 0;
}

/*
 *  send terminal type to the other side
 */
int
termsub(Biobuf *bp, uchar *sub, int n)
{
	char buf[64];
	char *term;
	char *p = buf;

	if(n < 1)
		return 0;
	if(sub[0] == 1){
		*p++ = Iac;
		*p++ = Sb;
		*p++ = opt[Term].code;
		*p++ = 0;
		term = getenv("TERM");
		if(term == 0 || *term == 0)
			term = "p9win";
		strncpy(p, term, sizeof(buf) - (p - buf) - 2);
		buf[sizeof(buf)-2] = 0;
		p += strlen(p);
		*p++ = Iac;
		*p++ = Se;
		return iwrite(Bfildes(bp), buf, p-buf);
	}
	return 0;
}

/*
 *  send an x display location to the other side
 */
int
xlocsub(Biobuf *bp, uchar *sub, int n)
{
	char buf[64];
	char *term;
	char *p = buf;

	if(n < 1)
		return 0;
	if(sub[0] == 1){
		*p++ = Iac;
		*p++ = Sb;
		*p++ = opt[Xloc].code;
		*p++ = 0;
		term = getenv("XDISP");
		if(term == 0 || *term == 0)
			term = "unknown";
		strncpy(p, term, p - buf - 2);
		p += strlen(term);
		*p++ = Iac;
		*p++ = Se;
		return iwrite(Bfildes(bp), buf, p-buf);
	}
	return 0;
}

static int
islikeatty(int fd)
{
	char buf[64];

	if(fd2path(fd, buf, sizeof buf) != 0)
		return 0;

	/* might be /mnt/term/dev/cons */
	return strlen(buf) >= 9 && strcmp(buf+strlen(buf)-9, "/dev/cons") == 0;
}

/*
 *  create a shared segment.  Make is start 2 meg higher than the current
 *  end of process memory.
 */
void*
share(ulong len)
{
	uchar *vastart;

	vastart = sbrk(0);
	if(vastart == (void*)-1)
		return 0;
	vastart += 2*1024*1024;

	if(segattach(0, "shared", vastart, len) == (void*)-1)
		return 0;

	return vastart;
}
