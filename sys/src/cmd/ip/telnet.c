#include <u.h>
#include <libc.h>
#include <bio.h>
#include "telnet.h"

int ctl = -1;		/* control fd (for break's) */
int consctl = -1;	/* consctl fd */

int ttypid;		/* pid's if the 2 processes (used to kill them) */
int netpid;
int stopped;
int interrupted;
int localecho;
int returns;

int	dodial(char*);
void	fromkbd(int);
void	fromnet(int);
int	menu(Biobuf*);
int	notifyf(void*, char*);
void	rawoff(void);
void	rawon(void);
void	telnet(int);
char*	system(int, char*);
int	echochange(Biobuf*, int);
int	termsub(Biobuf*, uchar*, int);
int	xlocsub(Biobuf*, uchar*, int);

void
usage(void)
{
	fatal("usage: telnet [-d] net!host[!service]", 0, 0);
}

void
main(int argc, char *argv[])
{
	returns = 1;
	ARGBEGIN{
	case 'C':
		opt[Echo].noway = 1;
		break;
	case 'd':
		debug = 1;
		break;
	case 'r':
		returns = 0;
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

	telnet(dodial(argv[0]));
}

/*
 *  dial and return a data connection
 */
int
dodial(char *dest)
{
	char name[3*NAMELEN];
	int data;
	char devdir[40];

	strcpy(name, netmkaddr(dest, "tcp", "telnet"));
	data = dial(name, 0, devdir, 0);
	if(data < 0)
		fatal("dialing %s: %r", name, 0);
	fprint(2, "connected to %s on %s\n", name, devdir);
	return data;
}

/*
 *  two processes pass bytes back and forth between the
 *  terminal and the network.
 */
void
telnet(int net)
{
	ttypid = getpid();
	switch(netpid = fork()){
	case -1:
		perror("con");
		exits("fork");
	case 0:
		rawoff();
		atnotify(notifyf, 1);
		fromnet(net);
		sendnote(ttypid, "die");
		exits(0);
	default:
		atnotify(notifyf, 1);
		fromkbd(net);
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
	int c;
	int eofs;

	Binit(&ib, 0, OREAD);
	Binit(&ob, net, OWRITE);
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
			if(eofs++ > 10)
				return;
			c = 004;
		} else
			eofs = 0;

		/*
		 *  if not in binary mode, look for the ^\ escape to menu.
		 *  also turn \n into \r\n
		 */
		if(!opt[Binary].local){
			if(c == 0034){ /* CTRL \ */
				if(Bflush(&ob) < 0)
					return;
				if(menu(&ib) < 0)
					return;
				continue;
			}
			if(c == '\n')
				if(Bputc(&ob, '\r') < 0)
					return;
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
	int crnl = 0;
	Biobuf ib, ob;

	Binit(&ib, net, OREAD);
	Binit(&ob, 1, OWRITE); 
	for(;; Bbuffered(&ib) == 0 ? Bflush(&ob) : 0){
		if(interrupted){
			interrupted = 0;
			send2(net, Iac, Interrupt);
		}
		c = Bgetc(&ib);
		if(c < 0)
			return;
		switch(c){
		/* \r\n or \n\r become \n */
		case '\n':
			if(!opt[Binary].local && !returns){
				if(crnl == '\r'){
					crnl = 0;
					continue;
				}
				crnl = c;
			}
			break;
		case '\r':
			if(!opt[Binary].local && !returns){
				if(crnl == '\n'){
					crnl = 0;
					continue;
				}
				crnl = c;
				c = '\n';
			}
			break;

		case Iac:
			crnl = 0;
			c = Bgetc(&ib);
			if(c == Iac)
				break;
			if(Bflush(&ob) < 0)
				return;
			if(control(&ib, c) < 0)
				return;
			continue;

		default:
			crnl = 0;
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
		fprint(2, "can't open consctl\n");
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
		fprint(2, "can't open consctl\n");
		return;
	}
	write(consctl, "rawoff", 6);
}

/*
 *  control menu
 */
#define STDHELP	"\t(b)reak, (i)nterrupt, (q)uit, (!cmd), (.)continue\n"

int
menu(Biobuf *bp)
{
	char *cp;
	int done;

/*	sendnote(netpid, "stop");/**/

	fprint(2, ">>> ");
	for(done = 0; !done; ){
		cp = Brdline(bp, '\n');
		if(cp == 0)
			return -1;
		cp[Blinelen(bp)-1] = 0;
		switch(*cp){
		case '!':
			system(Bfildes(bp), cp+1);
			break;
		case '.':
			done = 1;
			break;
		case 'q':
			return -1;
			break;
		case 'i':
			send2(Bfildes(bp), Iac, Interrupt);
			break;
		case 'b':
			send2(Bfildes(bp), Iac, Break);
			break;
		default:
			fprint(2, STDHELP);
			break;
		}
		if(!done)
			fprint(2, ">>> ");
	}

	sendnote(netpid, "go");
	return 0;
}

/*
 *  ignore interrupts
 */
int
notifyf(void *a, char *msg)
{
	USED(a);

	if(strcmp(msg, "interrupt") == 0){
		interrupted = 1;
		return 1;
	}
	if(strcmp(msg, "hangup") == 0)
		return 1;
	if(strcmp(msg, "stop") == 0){
		stopped = 1;
		return 1;
	}
	if(strcmp(msg, "go") == 0){
		stopped = 0;
		return 1;
	}
	return 0;
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
		dup(fd, 1);
		close(ctl);
		close(fd);
		if(*cmd)
			execl("/bin/rc", "rc", "-c", cmd, 0);
		else
			execl("/bin/rc", "rc", 0);
		perror("con");
		exits("exec");
	}
	for(p = wait(&msg); p >= 0; p = wait(&msg)){
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
