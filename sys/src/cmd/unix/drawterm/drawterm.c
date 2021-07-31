#include "lib9.h"
#include "auth.h"

char	*cpuaddr = "anna.cs.bell-labs.com";
char	*authaddr = "dinar.cs.bell-labs.com";

static char *pbmsg = "AS protocol botch";
static char *ccmsg = "can't connect to AS";

int	debug = 0;
int	mousequeue = 1;
int	depth;

static void	fdwritestr(int, char*, char*, int);
static int	fdreadstr(int, char*, int);
static void	noteproc(char *pid);
static void	getpasswd(char *p, int len);
static void	userpasswd(int islocal);
static int	outin(char *prompt, char *def, int len);
static char	*checkkey(char *name, char *key);
static int	writefile(char *name, char *buf, int len);
static int	authtcp(void);
static int	sendmsg(int fd, char *msg);

extern void	procinit(void);
extern char	*hostlookup(char*);
extern char	*base;
void	usage(void);
int	nncpu(void);

int	setam(char*);

int
main(int argc, char *argv[])
{
	char buf[128];
	int fd;

	/*
	 * Needed by various parts of the code.
	 * This is a huge bug.
	 */
	assert(sizeof(char)==1);
	assert(sizeof(short)==2);
	assert(sizeof(ushort)==2);
	assert(sizeof(int)==4);
	assert(sizeof(uint)==4);
	assert(sizeof(long)==4);
	assert(sizeof(ulong)==4);
	assert(sizeof(vlong)==8);
	assert(sizeof(uvlong)==8);

	threadinit();
	procinit();

	ARGBEGIN {
	case 'a':
		authaddr = ARGF();
		break;
	case 'c':
		cpuaddr = ARGF();
		break;
	case 'd':
		depth = atoi(ARGF());
		break;
	case 'n':
		setam("netkey");
		break;
	case 'm':
		mousequeue = !mousequeue;
		break;
	case 'r':
		base = ARGF();
		break;
	case 'o':
		/* from the outside */
		cpuaddr = "achille.cs.bell-labs.com";
		authaddr = "achille.cs.bell-labs.com";
		break;
	case '@':
		/* from Comcast @Home */
		cpuaddr = "10.252.0.122";
		authaddr = "10.252.0.122";
		break;
	default:
		usage();
	} ARGEND;

	if(argc != 0)
		usage();
	
	if(bind("#i", "/dev", MBEFORE) < 0)
		iprint("bind failed: %r\n");

	if(bind("#m", "/dev", MBEFORE) < 0)
		iprint("bind failed: %r\n");

	if(bind("#I", "/net", MBEFORE) < 0)
		iprint("bind failed: %r\n");

	authaddr = hostlookup(authaddr);
	cpuaddr = hostlookup(cpuaddr);

	fd = nncpu();	/* fd = cpu(); */
	
	/* Tell the remote side the command to execute and where our working directory is */
	fdwritestr(fd, "NO", "dir", 0);

	/*
	 * old cpu protocol
	 *
	if(fdreadstr(fd, buf, sizeof(buf)) < 0)
		fatal("bad pid");
	noteproc(buf);
	 *
	 */
	
	/* Wait for the other end to execute and start our file service
	 * of /mnt/term */
	if(fdreadstr(fd, buf, sizeof(buf)) < 0)
		fatal("waiting for FS");
	if(strncmp("FS", buf, 2) != 0)
		fatal("remote cpu: %s", buf);

	if(fdreadstr(fd, buf, sizeof(buf)) < 0)
		fatal("waiting for root");
	if(write(fd, "OK", 2) < 2)
		fatal("ack root");

	export(fd);
	return 0;
}

int
cpu(void)
{
	int fd;
	char na[256];

	userpasswd(0);
	
	netmkaddr(na, cpuaddr, "tcp", "17005");
	if((fd = dial(na, 0, 0, 0)) < 0) {
		fatal("can't dial %s: %r", na);
	}
	if(auth(fd) < 0)
		fatal("can't authenticate: %r");
	return fd;
}

/* authentication mechanisms */
static int	netkeyauth(int);

typedef struct AuthMethod AuthMethod;
struct AuthMethod {
	char	*name;			/* name of method */
	int	(*cf)(int);		/* client side authentication */
} authmethod[] =
{
	{ "p9",		auth},
	{ "netkey",	netkeyauth},
	{ nil,	nil}
};
AuthMethod *am = authmethod;	/* default is p9 */

int
setam(char *name)
{
	for(am = authmethod; am->name != nil; am++)
		if(strcmp(am->name, name) == 0)
			return 0;
	am = authmethod;
	return -1;
}

char *negstr = "negotiating authentication method";
int
nncpu(void)
{
	char na[128];
	char err[ERRLEN];
	int fd, n;

	netmkaddr(na, cpuaddr, "tcp", "17013");
	if((fd = dial(na, 0, 0, 0)) < 0) {
		fatal("can't dial %s: %r", na);
	}

	if(am->cf == auth)
		userpasswd(0);
	
	/* negotiate authentication mechanism */
	fdwritestr(fd, am->name, negstr, 0);
	n = fdreadstr(fd, err, ERRLEN);
	if(n < 0)
		fatal(negstr);
	if(*err)
		fatal("%s: %s", negstr, err);

	/* authenticate */
	if((*am->cf)(fd) < 0)
		fatal("cannot authenticate with %s", am->name);
	return fd;
}

static int
readln(char *buf, int n)
{
	char *p = buf;

	n--;
	while(n > 0){
		if(read(0, p, 1) != 1)
			break;
		if(*p == '\n' || *p == '\r'){
			*p = 0;
			return p-buf;
		}
		p++;
	}
		
	*p = 0;
	return p-buf;
}

static int
netkeyauth(int fd)
{
	char chall[NAMELEN];
	char resp[NAMELEN];

	strcpy(chall, getuser());
	print("user[%s]: ", chall);
	if(readln(resp, sizeof(resp)) < 0)
		return -1;
	if(*resp != 0)
		strcpy(chall, resp);
	fdwritestr(fd, chall, "challenge/response", 1);

	for(;;){
		if(fdreadstr(fd, chall, sizeof chall) < 0)
			break;
		if(*chall == 0)
			return 0;
		print("challenge: %s\nresponse: ", chall);
		if(readln(resp, sizeof(resp)) < 0)
			break;
		fdwritestr(fd, resp, "challenge/response", 1);
	}
	return -1;
}

void
usage(void)
{
	iprint("usage: %s [-o@] [-n] [-m] [-c cpusrv] [-a authsrv] [-r root]\n", argv0);
	exits(0);
}

void
fdwritestr(int fd, char *str, char *thing, int ignore)
{
	int l, n;

	l = strlen(str);
	n = write(fd, str, l+1);
	if(!ignore && n < 0)
		fatal("writing network: %s", thing);
}

int
fdreadstr(int fd, char *str, int len)
{
	int n;

	while(len) {
		n = read(fd, str, 1);
		if(n < 0)
			return -1;
		if(*str == '\0')
			return 0;
		str++;
		len--;
	}
	return -1;
}

void
noteproc(char *pid)
{
	char cmd[NAMELEN];
	int fd;
	char na[256];

	netmkaddr(na, cpuaddr, "tcp", "17006");
	if((fd = dial(na, 0, 0, 0)) < 0)
		fatal("can't dial");
	if(auth(fd) < 0)
		fatal("can't authenticate");

	sprint(cmd, "%s", pid);
	fdwritestr(fd, cmd, "notepid", 0);

	return;
/*
	notify(catcher);

	for(;;) {
		n = wait(&w);
		if(n < 0) {
			writestr(notechan, notebuf, "catcher", 1);
			errstr(syserr);
			if(strcmp(syserr, "interrupted") != 0){
				iprint("cpu: wait: %s\n", syserr);
				exits("waiterr");
			}
		}
		if(n == notepid)
			break;
	}
	exits(w.msg);
*/
}

static char	password[NAMELEN];
char	username[NAMELEN];

char *homsg = "can't set user name or key; please reboot";

/*
 *  get/set user name and password.  verify password with auth server.
 */
void
userpasswd(int islocal)
{
	int fd;
	char *msg;
	char hostkey[DESKEYLEN];

	if(*username == 0 || strcmp(username, "none") == 0){
		strcpy(username, "none");
		outin("user", username, sizeof(username));
	}

	if(!atlocalconsole()){
		print("not at local console; won't ask for password\n");
		p9sleep(10000);
		exits(0);
	}
	
	fd = -1;
	while(strcmp(username, "none") != 0){
		getpasswd(password, sizeof password);
		passtokey(hostkey, password);
		memset(password, 0, sizeof(password));
		fd = -1;
		if(islocal)
			break;
		msg = checkkey(username, hostkey);
		if(msg == 0)
			break;
		print("?%s\n", msg);
		outin("user", username, sizeof(username));
	}
	if(fd > 0)
		close(fd);

	/* set host's key */
	if(writefile("#c/key", hostkey, DESKEYLEN) < 0)
		fatal(homsg);

	/* set host's owner (and uid of current process) */
	if(writefile("#c/hostowner", username, strlen(username)) < 0)
		fatal(homsg);
	close(fd);
}

static char*
fromauth(char *trbuf, char *tbuf)
{
	int afd;
	char t;
	char *msg;
	static char error[2*ERRLEN];

	afd = authtcp();
	if(afd < 0) {
		sprint(error, "%s: %r", ccmsg);
		return error;
	}

	if(write(afd, trbuf, TICKREQLEN) < 0 || read(afd, &t, 1) != 1){
		close(afd);
		sprint(error, "%s: %r", pbmsg);
		return error;
	}
	switch(t){
	case AuthOK:
		msg = 0;
		if(readn(afd, tbuf, 2*TICKETLEN) < 0) {
			sprint(error, "%s: %r", pbmsg);
			msg = error;
		}
		break;
	case AuthErr:
		if(readn(afd, error, ERRLEN) < 0) {
			sprint(error, "%s: %r", pbmsg);
			msg = error;
		}
		else {
			error[ERRLEN-1] = 0;
			msg = error;
		}
		break;
	default:
		msg = pbmsg;
		break;
	}

	close(afd);
	return msg;
}

char*
checkkey(char *name, char *key)
{
	char *msg;
	Ticketreq tr;
	Ticket t;
	char trbuf[TICKREQLEN];
	char tbuf[2*TICKETLEN];

	memset(&tr, 0, sizeof tr);
	tr.type = AuthTreq;
	strcpy(tr.authid, name);
	strcpy(tr.hostid, name);
	strcpy(tr.uid, name);
	convTR2M(&tr, trbuf);
	msg = fromauth(trbuf, tbuf);
	if(msg == ccmsg){
		iprint("boot: can't contact auth server, passwd unchecked\n");
		return 0;
	}
	if(msg)
		return msg;
	convM2T(tbuf, &t, key);
	if(t.num == AuthTc && strcmp(name, t.cuid)==0)
		return 0;
	return "no match";
}

void
getpasswd(char *p, int len)
{
	char c;
	int i, n, fd;

	fd = open("#c/consctl", OWRITE);
	if(fd < 0)
		fatal("can't open consctl; please reboot");
	write(fd, "rawon", 5);
 Prompt:		
	print("password: ");
	n = 0;
	for(;;){
		do{
			i = read(0, &c, 1);
			if(i < 0)
				fatal("can't read cons; please reboot");
		}while(i == 0);
		switch(c){
		case '\n':
			p[n] = '\0';
			close(fd);
			print("\n");
			return;
		case '\b':
			if(n > 0)
				n--;
			break;
		case 'u' - 'a' + 1:		/* cntrl-u */
			print("\n");
			goto Prompt;
		default:
			if(n < len - 1)
				p[n++] = c;
			break;
		}
	}
}

int
authtcp(void)
{
	char na[256];

	netmkaddr(na, authaddr, "tcp", "567");

	return dial(na, 0, 0, 0);
}

int
plumb(char *dir, char *dest, int *efd, char *here)
{
	char buf[128];
	char name[128];
	int n;

	sprint(name, "%s/clone", dir);
	efd[0] = open(name, ORDWR);
	if(efd[0] < 0)
		return -1;
	n = read(efd[0], buf, sizeof(buf)-1);
	if(n < 0){
		close(efd[0]);
		return -1;
	}
	buf[n] = 0;
	sprint(name, "%s/%s/data", dir, buf);
	if(here){
		sprint(buf, "announce %s", here);
		if(sendmsg(efd[0], buf) < 0){
			close(efd[0]);
			return -1;
		}
	}
	sprint(buf, "connect %s", dest);
	if(sendmsg(efd[0], buf) < 0){
		close(efd[0]);
		return -1;
	}
	efd[1] = open(name, ORDWR);
	if(efd[1] < 0){
		close(efd[0]);
		return -1;
	}
	return efd[1];
}

int
sendmsg(int fd, char *msg)
{
	int n;

	n = strlen(msg);
	if(write(fd, msg, n) != n)
		return -1;
	return 0;
}

void
warning(char *s)
{
	char buf[ERRLEN];

	errstr(buf);
	iprint("boot: %s: %s\n", s, buf);
}

int
readfile(char *name, char *buf, int len)
{
	int f, n;

	buf[0] = 0;
	f = open(name, OREAD);
	if(f < 0)
		return -1;
	n = read(f, buf, len-1);
	if(n >= 0)
		buf[n] = 0;
	close(f);
	return 0;
}

int
writefile(char *name, char *buf, int len)
{
	int f, n;

	f = open(name, OWRITE);
	if(f < 0)
		return -1;
	n = write(f, buf, len);
	close(f);
	return (n != len) ? -1 : 0;
}

void
srvcreate(char *name, int fd)
{
	char *srvname;
	int f;
	char buf[2*NAMELEN];

	srvname = strrchr(name, '/');
	if(srvname)
		srvname++;
	else
		srvname = name;

	sprint(buf, "#s/%s", srvname);
	f = create(buf, 1, 0666);
	if(f < 0)
		fatal(buf);
	sprint(buf, "%d", fd);
	if(write(f, buf, strlen(buf)) != (int)strlen(buf))
		fatal("write");
	close(f);
}

int
outin(char *prompt, char *def, int len)
{
	int n;
	char buf[256];

	print("%s[%s]: ", prompt, *def ? def : "no default");
	n = read(0, buf, len);
	if(n <= 0)
		return -1;
	
	if(n != 1){			
		buf[n-1] = 0;
		strcpy(def, buf);
	}
	return n;
}

/*
 *  get second word of the terminal environment variable.   If it
 *  ends in "boot", get rid of that part.
 */
void
getconffile(char *conffile, char *terminal)
{
	char *p, *q;
	char *s;
	int n;

	s = conffile;
	*conffile = 0;
	p = terminal;
	if((p = strchr(p, ' ')) == 0 || p[1] == ' ' || p[1] == 0)
		return;
	p++;
	for(q = p; *q && *q != ' '; q++)
		;
	while(p < q)
		*conffile++ = *p++;
	*conffile = 0;

	/* dump a trailing boot */
	n = strlen(s);
	if(n > 4 && strcmp(s + n - 4, "boot") == 0)
		*(s+n-4) = 0;
}

/*
 * Case insensitive strcmp
 */
int
cistrncmp(char *a, char *b, int n)
{
	unsigned ac, bc;

	while(n > 0){
		ac = *a++;
		bc = *b++;
		n--;

		if(ac >= 'A' && ac <= 'Z')
			ac = 'a' + (ac - 'A');
		if(bc >= 'A' && bc <= 'Z')
			bc = 'a' + (bc - 'A');

		ac -= bc;
		if(ac)
			return ac;
		if(bc == 0)
			break;
	}

	return 0;
}
