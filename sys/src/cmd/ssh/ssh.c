#include "ssh.h"

int cooked = 0;		/* user wants cooked mode */
int raw = 0;		/* console is in raw mode */
int crstrip;
int interactive = -1;
int usemenu = 1;
int isatty(int);
int rawhack;
int forwardagent = 0;
char *buildcmd(int, char**);
void fromnet(Conn*);
void fromstdin(Conn*);
void winchanges(Conn*);
static void	sendwritemsg(Conn *c, char *buf, int n);

Cipher *allcipher[] = {
	&cipherrc4,
	&cipherblowfish,
	&cipher3des,
	&cipherdes,
	&ciphernone,
	&ciphertwiddle,
};

Auth *allauth[] = {
	&authpassword,
	&authrsa,
	&authtis,
};

char *cipherlist = "blowfish rc4 3des";
char *authlist = "rsa password tis";

Cipher*
findcipher(char *name, Cipher **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown cipher %s", name);
	return nil;
}

Auth*
findauth(char *name, Auth **list, int nlist)
{
	int i;

	for(i=0; i<nlist; i++)
		if(strcmp(name, list[i]->name) == 0)
			return list[i];
	error("unknown auth %s", name);
	return nil;
}

void
usage(void)
{
	fprint(2, "usage: ssh [-CiImPpRr] [-A authlist] [-c cipherlist] [user@]hostname [cmd [args]]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, dowinchange, fd, usepty;
	char *host, *cmd, *user, *p;
	char *f[16];
	Conn c;
	Msg *m;

	fmtinstall('B', mpfmt);
	fmtinstall('H', encodefmt);
	atexit(atexitkiller);
	atexitkill(getpid());

	dowinchange = 0;
	if(getenv("LINES"))
		dowinchange = 1;
	usepty = -1;
	user = nil;
	ARGBEGIN{
	case 'B':	/* undocumented, debugging */
		doabort = 1;
		break;
	case 'D':	/* undocumented, debugging */
		debuglevel = strtol(EARGF(usage()), nil, 0);
		break;
	case 'l':	/* deprecated */
	case 'u':
		user = EARGF(usage());
		break;
	case 'a':	/* used by Unix scp implementations; we must ignore them. */
	case 'x':
		break;

	case 'A':
		authlist = EARGF(usage());
		break;
	case 'C':
		cooked = 1;
		break;
	case 'c':
		cipherlist = EARGF(usage());
		break;
	case 'f':
		forwardagent = 1;
		break;
	case 'I':
		interactive = 0;
		break;
	case 'i':
		interactive = 1;
		break;
	case 'm':
		usemenu = 0;
		break;
	case 'P':
		usepty = 0;
		break;
	case 'p':
		usepty = 1;
		break;
	case 'R':
		rawhack = 1;
		break;
	case 'r':
		crstrip = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc < 1)
		usage();

	host = argv[0];

	cmd = nil;
	if(argc > 1)
		cmd = buildcmd(argc-1, argv+1);

	if((p = strchr(host, '@')) != nil){
		*p++ = '\0';
		user = host;
		host = p;
	}
	if(user == nil)
		user = getenv("user");
	if(user == nil)
		sysfatal("cannot find user name");

	privatefactotum();
	if(interactive==-1)
		interactive = isatty(0);

	if((fd = dial(netmkaddr(host, "tcp", "ssh"), nil, nil, nil)) < 0)
		sysfatal("dialing %s: %r", host);

	memset(&c, 0, sizeof c);
	c.interactive = interactive;
	c.fd[0] = c.fd[1] = fd;
	c.user = user;
	c.host = host;
	setaliases(&c, host);

	c.nokcipher = getfields(cipherlist, f, nelem(f), 1, ", ");
	c.okcipher = emalloc(sizeof(Cipher*)*c.nokcipher);
	for(i=0; i<c.nokcipher; i++)
		c.okcipher[i] = findcipher(f[i], allcipher, nelem(allcipher));

	c.nokauth = getfields(authlist, f, nelem(f), 1, ", ");
	c.okauth = emalloc(sizeof(Auth*)*c.nokauth);
	for(i=0; i<c.nokauth; i++)
		c.okauth[i] = findauth(f[i], allauth, nelem(allauth));

	sshclienthandshake(&c);

	if(forwardagent){
		if(startagent(&c) < 0)
			forwardagent = 0;
	}
	if(usepty == -1)
		usepty = cmd==nil;
	if(usepty)
		requestpty(&c);
	if(cmd){
		m = allocmsg(&c, SSH_CMSG_EXEC_CMD, 4+strlen(cmd));
		putstring(m, cmd);
	}else
		m = allocmsg(&c, SSH_CMSG_EXEC_SHELL, 0);
	sendmsg(m);

	fromstdin(&c);
	rfork(RFNOTEG);	/* only fromstdin gets notes */
	if(dowinchange)
		winchanges(&c);
	fromnet(&c);
	exits(0);
}

int
isatty(int fd)
{
	char buf[64];

	buf[0] = '\0';
	fd2path(fd, buf, sizeof buf);
	if(strlen(buf)>=9 && strcmp(buf+strlen(buf)-9, "/dev/cons")==0)
		return 1;
	return 0;
}

char*
buildcmd(int argc, char **argv)
{
	int i, len;
	char *s, *t;

	len = argc-1;
	for(i=0; i<argc; i++)
		len += strlen(argv[i]);
	s = emalloc(len+1);
	t = s;
	for(i=0; i<argc; i++){
		if(i)
			*t++ = ' ';
		strcpy(t, argv[i]);
		t += strlen(t);
	}
	return s;
}

void
fromnet(Conn *c)
{
	int fd, len;
	char *s, *es, *r, *w;
	ulong ex;
	char buf[64];
	Msg *m;

	for(;;){
		m = recvmsg(c, -1);
		if(m == nil)
			break;
		switch(m->type){
		default:
			badmsg(m, 0);

		case SSH_SMSG_EXITSTATUS:
			ex = getlong(m);
			if(ex==0)
				exits(0);
			sprint(buf, "%lud", ex);
			exits(buf);

		case SSH_MSG_DISCONNECT:
			s = getstring(m);
			error("disconnect: %s", s);

		/*
		 * If we ever add reverse port forwarding, we'll have to
		 * revisit this.  It assumes that the agent connections are
		 * the only ones.
		 */
		case SSH_SMSG_AGENT_OPEN:
			if(!forwardagent)
				error("server tried to use agent forwarding");
			handleagentopen(m);
			break;
		case SSH_MSG_CHANNEL_INPUT_EOF:
			if(!forwardagent)
				error("server tried to use agent forwarding");
			handleagentieof(m);
			break;
		case SSH_MSG_CHANNEL_OUTPUT_CLOSED:
			if(!forwardagent)
				error("server tried to use agent forwarding");
			handleagentoclose(m);
			break;
		case SSH_MSG_CHANNEL_DATA:
			if(!forwardagent)
				error("server tried to use agent forwarding");
			handleagentmsg(m);
			break;

		case SSH_SMSG_STDOUT_DATA:
			fd = 1;
			goto Dataout;
		case SSH_SMSG_STDERR_DATA:
			fd = 2;
			goto Dataout;
		Dataout:
			len = getlong(m);
			s = (char*)getbytes(m, len);
			if(crstrip){
				es = s+len;
				for(r=w=s; r<es; r++)
					if(*r != '\r')
						*w++ = *r;
				len = w-s;
			}
			write(fd, s, len);
			break;
		}
		free(m);
	}
}		

/*
 * Lifted from telnet.c, con.c
 */

static int consctl = -1;
static int outfd1=1, outfd2=2;	/* changed during system */
static void system(Conn*, char*);

/*
 *  turn keyboard raw mode on
 */
static void
rawon(void)
{
	if(raw)
		return;
	if(cooked)
		return;
	if(consctl < 0)
		consctl = open("/dev/consctl", OWRITE);
	if(consctl < 0)
		return;
	if(write(consctl, "rawon", 5) != 5)
		return;
	raw = 1;
}

/*
 *  turn keyboard raw mode off
 */
static void
rawoff(void)
{
	if(raw == 0)
		return;
	if(consctl < 0)
		return;
	if(write(consctl, "rawoff", 6) != 6)
		return;
	close(consctl);
	consctl = -1;
	raw = 0;
}

/*
 *  control menu
 */
#define STDHELP	"\t(q)uit, (i)nterrupt, toggle printing (r)eturns, (.)continue, (!cmd)\n"

static int
menu(Conn *c)
{
	char buf[1024];
	long n;
	int done;
	int wasraw;

	wasraw = raw;
	if(wasraw)
		rawoff();

	buf[0] = '?';
	fprint(2, ">>> ");
	for(done = 0; !done; ){
		n = read(0, buf, sizeof(buf)-1);
		if(n <= 0)
			return -1;
		buf[n] = 0;
		switch(buf[0]){
		case '!':
			print(buf);
			system(c, buf+1);
			print("!\n");
			done = 1;
			break;
		case 'i':
			buf[0] = 0x1c;
			sendwritemsg(c, buf, 1);
			done = 1;
			break;
		case '.':
		case 'q':
			done = 1;
			break;
		case 'r':
			crstrip = 1-crstrip;
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
	return buf[0];
}

static void
sendwritemsg(Conn *c, char *buf, int n)
{
	Msg *m;

	if(n==0)
		m = allocmsg(c, SSH_CMSG_EOF, 0);
	else{
		m = allocmsg(c, SSH_CMSG_STDIN_DATA, 4+n);
		putlong(m, n);
		putbytes(m, buf, n);
	}
	sendmsg(m);
}

/*
 *  run a command with the network connection as standard IO
 */
static void
system(Conn *c, char *cmd)
{
	int pid;
	int p;
	int pfd[2];
	int n;
	int wasconsctl;
	char buf[4096];

	if(pipe(pfd) < 0){
		perror("pipe");
		return;
	}
	outfd1 = outfd2 = pfd[1];

	wasconsctl = consctl;
	close(consctl);
	consctl = -1;
	switch(pid = fork()){
	case -1:
		perror("con");
		return;
	case 0:
		close(pfd[1]);
		dup(pfd[0], 0);
		dup(pfd[0], 1);
		close(c->fd[0]);	/* same as c->fd[1] */
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
			sendwritemsg(c, buf, n);
		p = waitpid();
		outfd1 = 1;
		outfd2 = 2;
		close(pfd[1]);
		if(p < 0 || p != pid)
			return;
		break;
	}
	if(wasconsctl >= 0){
		consctl = open("/dev/consctl", OWRITE);
		if(consctl < 0)
			error("cannot open consctl");
	}
}

static void
cookedcatchint(void*, char *msg)
{
	if(strstr(msg, "interrupt"))
		noted(NCONT);
	else if(strstr(msg, "kill"))
		noted(NDFLT);
	else
		noted(NCONT);
}

static int
wasintr(void)
{
	char err[64];

	rerrstr(err, sizeof err);
	return strstr(err, "interrupt") != 0;
}

void
fromstdin(Conn *c)
{
	int n;
	char buf[1024];
	int pid;
	int eofs;

	switch(pid = rfork(RFMEM|RFPROC|RFNOWAIT)){
	case -1:
		error("fork: %r");
	case 0:
		break;
	default:
		atexitkill(pid);
		return;
	}

	atexit(atexitkiller);
	if(interactive)
		rawon();

	notify(cookedcatchint);

	eofs = 0;
	for(;;){
		n = read(0, buf, sizeof(buf));
		if(n < 0){
			if(wasintr()){
				if(!raw){
					buf[0] = 0x7f;
					n = 1;
				}else
					continue;
			}else
				break;
		}
		if(n == 0){
			if(!c->interactive || ++eofs > 32)
				break;
		}else
			eofs = 0;
		if(interactive && usemenu && n && memchr(buf, 0x1c, n)) {
			if(menu(c)=='q'){
				sendwritemsg(c, "", 0);
				exits("quit");
			}
			continue;
		}
		if(!raw && n==0){
			buf[0] = 0x4;
			n = 1;
		}
		sendwritemsg(c, buf, n);
	}
	sendwritemsg(c, "", 0);
	atexitdont(atexitkiller);
	exits(nil);
}

void
winchanges(Conn *c)
{
	int nrow, ncol, width, height;
	int pid;

	switch(pid = rfork(RFMEM|RFPROC|RFNOWAIT)){
	case -1:
		error("fork: %r");
	case 0:
		break;
	default:
		atexitkill(pid);
		return;
	}

	for(;;){
		if(readgeom(&nrow, &ncol, &width, &height) < 0)
			break;
		sendwindowsize(c, nrow, ncol, width, height);
	}
	exits(nil);
}
