#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <libsec.h>

#include "../ip/telnet.h"

/*  console state (for consctl) */
typedef struct Consstate	Consstate;
struct Consstate{
	int raw;
	int hold;
};
Consstate *cons;

int notefd;		/* for sending notes to the child */
int noproto;		/* true if we shouldn't be using the telnet protocol */
int trusted;		/* true if we need not authenticate - current user
				is ok */
int nonone = 1;		/* don't allow none logins */
int noworldonly;	/* only noworld accounts */

enum
{
	Maxpath=	256,
	Maxuser=	64,
	Maxvar=		32,
};

/* input and output buffers for network connection */
Biobuf	netib;
Biobuf	childib;
char	remotesys[Maxpath];	/* name of remote system */

int	alnum(int);
int	conssim(void);
int	fromchild(char*, int);
int	fromnet(char*, int);
int	termchange(Biobuf*, int);
int	termsub(Biobuf*, uchar*, int);
int	xlocchange(Biobuf*, int);
int	xlocsub(Biobuf*, uchar*, int);
int	challuser(char*);
int	noworldlogin(char*);
void*	share(ulong);
int	doauth(char*);

#define TELNETLOG "telnet"

void
logit(char *fmt, ...)
{
	va_list arg;
	char buf[8192];

	va_start(arg, fmt);
	vseprint(buf, buf + sizeof(buf) / sizeof(*buf), fmt, arg);
	va_end(arg);
	syslog(0, TELNETLOG, "(%s) %s", remotesys, buf);
}

void
getremote(char *dir)
{
	int fd, n;
	char remfile[Maxpath];

	sprint(remfile, "%s/remote", dir);
	fd = open(remfile, OREAD);
	if(fd < 0)
		strcpy(remotesys, "unknown2");
	n = read(fd, remotesys, sizeof(remotesys)-1);
	if(n>0)
		remotesys[n-1] = 0;
	else
		strcpy(remotesys, remfile);
	close(fd);
}

void
main(int argc, char *argv[])
{
	char buf[1024];
	int fd;
	char user[Maxuser];
	int tries = 0;
	int childpid;
	int n, eofs;

	memset(user, 0, sizeof(user));
	ARGBEGIN {
	case 'n':
		opt[Echo].local = 1;
		noproto = 1;
		break;
	case 'p':
		noproto = 1;
		break;
	case 'a':
		nonone = 0;
		break;
	case 't':
		trusted = 1;
		strncpy(user, getuser(), sizeof(user)-1);
		break;
	case 'u':
		strncpy(user, ARGF(), sizeof(user)-1);
		break;
	case 'd':
		debug = 1;
		break;
	case 'N':
		noworldonly = 1;
		break;
	} ARGEND

	if(argc)
		getremote(argv[argc-1]);
	else
		strcpy(remotesys, "unknown");

	/* options we need routines for */
	opt[Term].change = termchange;
	opt[Term].sub = termsub;
	opt[Xloc].sub = xlocsub;

	/* setup default telnet options */
	if(!noproto){
		send3(1, Iac, Will, opt[Echo].code);
		send3(1, Iac, Do, opt[Term].code);
		send3(1, Iac, Do, opt[Xloc].code);
	}

	/* shared data for console state */
	cons = share(sizeof(Consstate));
	if(cons == 0)
		fatal("shared memory", 0, 0);

	/* authenticate and create new name space */
	Binit(&netib, 0, OREAD);
	if (!trusted){
		while(doauth(user) < 0)
			if(++tries == 5){
				logit("failed as %s: %r", user);
				print("authentication failure:%r\r\n");
				exits("authentication");
			}
	}
	logit("logged in as %s", user);
	putenv("service", "con");

	/* simulate /dev/consctl and /dev/cons using pipes */
	fd = conssim();
	if(fd < 0)
		fatal("simulating", 0, 0);
	Binit(&childib, fd, OREAD);

	/* start a shell in a different process group */
	switch(childpid = rfork(RFPROC|RFNAMEG|RFFDG|RFNOTEG)){
	case -1:
		fatal("fork", 0, 0);
	case 0:
		close(fd);
		fd = open("/dev/cons", OREAD);
		dup(fd, 0);
		close(fd);
		fd = open("/dev/cons", OWRITE);
		dup(fd, 1);
		dup(fd, 2);
		close(fd);
		segdetach(cons);
		execl("/bin/rc", "rc", "-il", nil);
		fatal("/bin/rc", 0, 0);
	default:
		sprint(buf, "/proc/%d/notepg", childpid);
		notefd = open(buf, OWRITE);
		break;
	}

	/* two processes to shuttle bytes twixt children and network */
	switch(fork()){
	case -1:
		fatal("fork", 0, 0);
	case 0:
		eofs = 0;
		for(;;){
			n = fromchild(buf, sizeof(buf));
			if(n <= 0){
				if(eofs++ > 2)
					break;
				continue;
			}
			eofs = 0;
			if(write(1, buf, n) != n)
				break;
		}
		break;
	default:
		while((n = fromnet(buf, sizeof(buf))) >= 0)
			if(write(fd, buf, n) != n)
				break;
		break;
	}

	/* kill off all server processes */
	sprint(buf, "/proc/%d/notepg", getpid());
	fd = open(buf, OWRITE);
	write(fd, "die", 3);
	exits(0);
}

void
prompt(char *p, char *b, int n, int raw)
{
	char *e;
	int i;
	int echo;

	echo = opt[Echo].local;
	if(raw)
		opt[Echo].local = 0;
	print("%s: ", p);
	for(e = b+n; b < e;){
		i = fromnet(b, e-b);
		if(i <= 0)
			exits("fromnet: hungup");
		b += i;
		if(*(b-1) == '\n' || *(b-1) == '\r'){
			*(b-1) = 0;
			break;
		}
	}
	if(raw)
		opt[Echo].local = echo;
}

/*
 *  challenge user
 */
int
challuser(char *user)
{
	char nchall[64];
	char response[64];
	Chalstate *ch;
	AuthInfo *ai;

	if(strcmp(user, "none") == 0){
		if(nonone)
			return -1;
		newns("none", nil);
		return 0;
	}
	if((ch = auth_challenge("proto=p9cr role=server user=%q", user)) == nil)
		return -1;
	snprint(nchall, sizeof nchall, "challenge: %s\r\nresponse", ch->chal);
	prompt(nchall, response, sizeof response, 0);
	ch->resp = response;
	ch->nresp = strlen(response);
	ai = auth_response(ch);
	auth_freechal(ch);
	if(ai == nil){
		rerrstr(response, sizeof response);
		print("!%s\n", response);
		return -1;
	}
	if(auth_chuid(ai, nil) < 0)
		return -1;
	return 0;
}
/*
 *  use the in the clear apop password to change user id
 */
int
noworldlogin(char *user)
{
	char password[256];

	prompt("password", password, sizeof(password), 1);
	if(login(user, password, "/lib/namespace.noworld") < 0)
		return -1;
	rfork(RFNOMNT);	/* sandbox */
	return 0;
}

int
doauth(char *user)
{
	if(*user == 0)
		prompt("user", user, Maxuser, 0);
	if(noworld(user))
		return noworldlogin(user);
	if(noworldonly)
		return -1;
	return challuser(user);
		
}

/*
 *  Process some input from the child, add protocol if needed.  If
 *  the input buffer goes empty, return.
 */
int
fromchild(char *bp, int len)
{
	int c;
	char *start;

	for(start = bp; bp-start < len-1; ){
		c = Bgetc(&childib);
		if(c < 0){
			if(bp == start)
				return -1;
			else
				break;
		}
		if(cons->raw == 0 && c == '\n')
			*bp++ = '\r';
		*bp++ = c;
		if(Bbuffered(&childib) == 0)
			break;
	}
	return bp-start;
}

/*
 *  Read from the network up to a '\n' or some other break.
 *
 *  If in binary mode, buffer characters but don't 
 *
 *  The following characters are special:
 *	'\r\n's and '\r's get turned into '\n's.
 *	^H erases the last character buffered.
 *	^U kills the whole line buffered.
 *	^W erases the last word
 *	^D causes a 0-lenght line to be returned.
 *	Intr causes an "interrupt" note to be sent to the children.
 */
#define ECHO(c) { *ebp++ = (c); }
int
fromnet(char *bp, int len)
{
	int c;
	char echobuf[1024];
	char *ebp;
	char *start;
	static int crnl;
	static int doeof;


	/* simulate an EOF as a 0 length input */
	if(doeof){
		doeof = 0;
		return 0;
	}

	for(ebp = echobuf,start = bp; bp-start < len && ebp-echobuf < sizeof(echobuf); ){
		c = Bgetc(&netib);
		if(c < 0){
			if(bp == start)
				return -1;
			else
				break;
		}

		/* telnet protocol only */
		if(!noproto){
			/* protocol messages */
			switch(c){
			case Iac:
				crnl = 0;
				c = Bgetc(&netib);
				if(c == Iac)
					break;
				control(&netib, c);
				continue;
			}

		}

		/* \r\n or \n\r become \n  */
		if(c == '\r' || c == '\n'){
			if(crnl && crnl != c){
				crnl = 0;
				continue;
			}
			if(cons->raw == 0 && opt[Echo].local){
				ECHO('\r');
				ECHO('\n');
			}
			crnl = c;
			if(cons->raw == 0)
				*bp++ = '\n';
			else
				*bp++ = c;
			break;
		} else
			crnl = 0;

		/* raw processing (each character terminates */
		if(cons->raw){
			*bp++ = c;
			break;
		}

		/* in binary mode, there are no control characters */
		if(opt[Binary].local){
			if(opt[Echo].local)
				ECHO(c);
			*bp++ = c;
			continue;
		}

		/* cooked processing */
		switch(c){
		case 0x00:
			if(noproto)		/* telnet ignores nulls */
				*bp++ = c;
			continue;
		case 0x04:
			if(bp != start)
				doeof = 1;
			goto out;

		case 0x08:	/* ^H */
			if(start < bp)
				bp--;
			if(opt[Echo].local)
				ECHO(c);
			break;

		case 0x15:	/* ^U */
			bp = start;
			if(opt[Echo].local){
				ECHO('^');
				ECHO('U');
				ECHO('\r');
				ECHO('\n');
			}
			break;

		case 0x17:	/* ^W */
			if (opt[Echo].local) {
				while (--bp >= start && !alnum(*bp))
					ECHO('\b');
				while (bp >= start && alnum(*bp)) {
					ECHO('\b');
					bp--;
				}
				bp++;
			}
			break;

		case 0x7f:	/* Del */
			write(notefd, "interrupt", 9);
			bp = start;
			break;

		default:
			if(opt[Echo].local)
				ECHO(c);
			*bp++ = c;
		}
		if(ebp != echobuf)
			write(1, echobuf, ebp-echobuf);
		ebp = echobuf;
	}
out:
	if(ebp != echobuf)
		write(1, echobuf, ebp-echobuf);
	return bp - start;
}

int
termchange(Biobuf *bp, int cmd)
{
	char buf[8];
	char *p = buf;

	if(cmd != Will)
		return 0;

	/* ask other side to send term type info */
	*p++ = Iac;
	*p++ = Sb;
	*p++ = opt[Term].code;
	*p++ = 1;
	*p++ = Iac;
	*p++ = Se;
	return iwrite(Bfildes(bp), buf, p-buf);
}

int
termsub(Biobuf *bp, uchar *sub, int n)
{
	char term[Maxvar];

	USED(bp);
	if(n-- < 1 || sub[0] != 0)
		return 0;
	if(n >= sizeof term)
		n = sizeof term;
	strncpy(term, (char*)sub, n);
	putenv("TERM", term);
	return 0;
}

int
xlocchange(Biobuf *bp, int cmd)
{
	char buf[8];
	char *p = buf;

	if(cmd != Will)
		return 0;

	/* ask other side to send x display info */
	*p++ = Iac;
	*p++ = Sb;
	*p++ = opt[Xloc].code;
	*p++ = 1;
	*p++ = Iac;
	*p++ = Se;
	return iwrite(Bfildes(bp), buf, p-buf);
}

int
xlocsub(Biobuf *bp, uchar *sub, int n)
{
	char xloc[Maxvar];

	USED(bp);
	if(n-- < 1 || sub[0] != 0)
		return 0;
	if(n >= sizeof xloc)
		n = sizeof xloc;
	strncpy(xloc, (char*)sub, n);
	putenv("DISPLAY", xloc);
	return 0;
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

/*
 *  bind a pipe onto consctl and keep reading it to
 *  get changes to console state.
 */
int
conssim(void)
{
	int i, n;
	int fd;
	int tries;
	char buf[128];
	char *field[10];

	/* a pipe to simulate the /dev/cons */
	if(bind("#|", "/mnt/cons/cons", MREPL) < 0)
		fatal("/dev/cons1", 0, 0);
	if(bind("/mnt/cons/cons/data1", "/dev/cons", MREPL) < 0)
		fatal("/dev/cons2", 0, 0);

	/* a pipe to simulate consctl */
	if(bind("#|", "/mnt/cons/consctl", MBEFORE) < 0
	|| bind("/mnt/cons/consctl/data1", "/dev/consctl", MREPL) < 0)
		fatal("/dev/consctl", 0, 0);

	/* a process to read /dev/consctl and set the state in cons */
	switch(fork()){
	case -1:
		fatal("forking", 0, 0);
	case 0:
		break;
	default:
		return open("/mnt/cons/cons/data", ORDWR);
	}

	for(tries = 0; tries < 100; tries++){
		cons->raw = 0;
		cons->hold = 0;
		fd = open("/mnt/cons/consctl/data", OREAD);
		if(fd < 0)
			continue;
		tries = 0;
		for(;;){
			n = read(fd, buf, sizeof(buf)-1);
			if(n <= 0)
				break;
			buf[n] = 0;
			n = getfields(buf, field, 10, 1, " ");
			for(i = 0; i < n; i++){
				if(strcmp(field[i], "rawon") == 0) {
					if(debug) fprint(2, "raw = 1\n");
					cons->raw = 1;
				} else if(strcmp(field[i], "rawoff") == 0) {
					if(debug) fprint(2, "raw = 0\n");
					cons->raw = 0;
				} else if(strcmp(field[i], "holdon") == 0) {
					cons->hold = 1;
					if(debug) fprint(2, "raw = 1\n");
				} else if(strcmp(field[i], "holdoff") == 0) {
					cons->hold = 0;
					if(debug) fprint(2, "raw = 0\n");
				}
			}
		}
		close(fd);
	}
	exits(0);
	return -1;
}

int
alnum(int c)
{
	/*
	 * Hard to get absolutely right.  Use what we know about ASCII
	 * and assume anything above the Latin control characters is
	 * potentially an alphanumeric.
	 */
	if(c <= ' ')
		return 0;
	if(0x7F<=c && c<=0xA0)
		return 0;
	if(strchr("!\"#$%&'()*+,-./:;<=>?@`[\\]^{|}~", c))
		return 0;
	return 1;
}
