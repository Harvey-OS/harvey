#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <ip.h>
#include <libsec.h>
#include <String.h>

#include "glob.h"

enum
{
	/* telnet control character */
	Iac=		255,

	/* representation types */
	Tascii=		0,
	Timage=		1,

	/* transmission modes */
	Mstream=	0,
	Mblock=		1,
	Mpage=		2,

	/* file structure */
	Sfile=		0,
	Sblock=		1,
	Scompressed=	2,

	/* read/write buffer size */
	Nbuf=		4096,

	/* maximum ms we'll wait for a command */
	Maxwait=	1000*60*30,		/* inactive for 30 minutes, we hang up */

	Maxerr=		128,
	Maxpath=	512,
};

int	abortcmd(char*);
int	appendcmd(char*);
int	cdupcmd(char*);
int	cwdcmd(char*);
int	delcmd(char*);
int	helpcmd(char*);
int	listcmd(char*);
int	mdtmcmd(char*);
int	mkdircmd(char*);
int	modecmd(char*);
int	namelistcmd(char*);
int	nopcmd(char*);
int	passcmd(char*);
int	pasvcmd(char*);
int	portcmd(char*);
int	pwdcmd(char*);
int	quitcmd(char*);
int	rnfrcmd(char*);
int	rntocmd(char*);
int	reply(char*, ...);
int	restartcmd(char*);
int	retrievecmd(char*);
int	sitecmd(char*);
int	sizecmd(char*);
int	storecmd(char*);
int	storeucmd(char*);
int	structcmd(char*);
int	systemcmd(char*);
int	typecmd(char*);
int	usercmd(char*);

int	dialdata(void);
char*	abspath(char*);
int	crlfwrite(int, char*, int);
int	sodoff(void);
int	accessok(char*);

typedef struct Cmd	Cmd;
struct Cmd
{
	char	*name;
	int	(*f)(char*);
	int	needlogin;
};

Cmd cmdtab[] =
{
	{ "abor",	abortcmd,	0, },
	{ "appe",	appendcmd,	1, },
	{ "cdup",	cdupcmd,	1, },
	{ "cwd",	cwdcmd,		1, },
	{ "dele",	delcmd,		1, },
	{ "help",	helpcmd,	0, },
	{ "list",	listcmd,	1, },
	{ "mdtm",	mdtmcmd,	1, },
	{ "mkd",	mkdircmd,	1, },
	{ "mode",	modecmd,	0, },
	{ "nlst",	namelistcmd,	1, },
	{ "noop",	nopcmd,		0, },
	{ "pass",	passcmd,	0, },
	{ "pasv",	pasvcmd,	1, },
	{ "pwd",	pwdcmd,		0, },
	{ "port", 	portcmd,	1, },
	{ "quit",	quitcmd,	0, },
	{ "rest",	restartcmd,	1, },
	{ "retr",	retrievecmd,	1, },
	{ "rmd",	delcmd,		1, },
	{ "rnfr",	rnfrcmd,	1, },
	{ "rnto",	rntocmd,	1, },
	{ "site", sitecmd, 1, },
	{ "size", 	sizecmd,	1, },
	{ "stor", 	storecmd,	1, },
	{ "stou", 	storeucmd,	1, },
	{ "stru",	structcmd,	1, },
	{ "syst",	systemcmd,	0, },
	{ "type", 	typecmd,	0, },
	{ "user",	usercmd,	0, },
	{ 0, 0, 0 },
};

#define NONENS "/lib/namespace.ftp"	/* default ns for none */

char	user[Maxpath];		/* logged in user */
char	curdir[Maxpath];	/* current directory path */
Chalstate	*ch;
int	loggedin;
int	type;			/* transmission type */
int	mode;			/* transmission mode */
int	structure;		/* file structure */
char	data[64];		/* data address */
int	pid;			/* transfer process */
int	encryption;		/* encryption state */
int	isnone, anon_ok, anon_only, anon_everybody;
char	cputype[Maxpath];	/* the environment variable of the same name */
char	bindir[Maxpath];	/* bin directory for this architecture */
char	mailaddr[Maxpath];
char	*namespace = NONENS;
int	debug;
NetConnInfo	*nci;
int	createperm = 0660;
int	isnoworld;
vlong	offset;			/* from restart command */

ulong id;

typedef struct Passive Passive;
struct Passive
{
	int	inuse;
	char	adir[40];
	int	afd;
	int	port;
	uchar	ipaddr[IPaddrlen];
} passive;

#define FTPLOG "ftp"

void
logit(char *fmt, ...)
{
	char buf[8192];
	va_list arg;
	char errstr[128];

	rerrstr(errstr, sizeof errstr);
	va_start(arg, fmt);
	vseprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	syslog(0, FTPLOG, "%s.%s %s", nci->rsys, nci->rserv, buf);
	werrstr(errstr, sizeof errstr);
}

static void
usage(void)
{
	syslog(0, "ftp", "usage: %s [-aAde] [-n nsfile]", argv0);
	fprint(2, "usage: %s [-aAde] [-n nsfile]\n", argv0);
	exits("usage");
}

/*
 *  read commands from the control stream and dispatch
 */
void
main(int argc, char **argv)
{
	char *cmd;
	char *arg;
	char *p;
	Cmd *t;
	Biobuf in;
	int i;

	ARGBEGIN{
	case 'a':		/* anonymous OK */
		anon_ok = 1;
		break;
	case 'A':
		anon_ok = 1;
		anon_only = 1;
		break;
	case 'd':
		debug++;
		break;
	case 'e':
		anon_ok = 1;
		anon_everybody = 1;
		break;
	case 'n':
		namespace = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	/* open log file before doing a newns */
	syslog(0, FTPLOG, nil);

	/* find out who is calling */
	if(argc < 1)
		nci = getnetconninfo(nil, 0);
	else
		nci = getnetconninfo(argv[argc-1], 0);
	if(nci == nil)
		sysfatal("ftpd needs a network address");

	strcpy(mailaddr, "?");
	id = getpid();

	/* figure out which binaries to bind in later (only for none) */
	arg = getenv("cputype");
	if(arg)
		strecpy(cputype, cputype+sizeof cputype, arg);
	else
		strcpy(cputype, "mips");
	/* shurely /%s/bin */
	snprint(bindir, sizeof(bindir), "/bin/%s/bin", cputype);

	Binit(&in, 0, OREAD);
	reply("220 Plan 9 FTP server ready");
	alarm(Maxwait);
	while(cmd = Brdline(&in, '\n')){
		alarm(0);

		/*
		 *  strip out trailing cr's & lf and delimit with null
		 */
		i = Blinelen(&in)-1;
		cmd[i] = 0;
		if(debug)
			logit("%s", cmd);
		while(i > 0 && cmd[i-1] == '\r')
			cmd[--i] = 0;

		/*
		 *  hack for GatorFTP+, look for a 0x10 used as a delimiter
		 */
		p = strchr(cmd, 0x10);
		if(p)
			*p = 0;

		/*
		 *  get rid of telnet control sequences (we don't need them)
		 */
		while(*cmd && (uchar)*cmd == Iac){
			cmd++;
			if(*cmd)
				cmd++;
		}

		/*
		 *  parse the message (command arg)
		 */
		arg = strchr(cmd, ' ');
		if(arg){
			*arg++ = 0;
			while(*arg == ' ')
				arg++;
		}

		/*
		 *  ignore blank commands
		 */
		if(*cmd == 0)
			continue;

		/*
		 *  lookup the command and do it
		 */
		for(p = cmd; *p; p++)
			*p = tolower(*p);
		for(t = cmdtab; t->name; t++)
			if(strcmp(cmd, t->name) == 0){
				if(t->needlogin && !loggedin)
					sodoff();
				else if((*t->f)(arg) < 0)
					exits(0);
				break;
			}
		if(t->f != restartcmd){
			/*
			 *  the file offset is set to zero following
			 *  all commands except the restart command
			 */
			offset = 0;
		}
		if(t->name == 0){
			/*
			 *  the OOB bytes preceding an abort from UCB machines
			 *  comes out as something unrecognizable instead of
			 *  IAC's.  Certainly a Plan 9 bug but I can't find it.
			 *  This is a major hack to avoid the problem. -- presotto
			 */
			i = strlen(cmd);
			if(i > 4 && strcmp(cmd+i-4, "abor") == 0){
				abortcmd(0);
			} else{
				logit("%s (%s) command not implemented", cmd, arg?arg:"");
				reply("502 %s command not implemented", cmd);
			}
		}
		alarm(Maxwait);
	}
	if(pid)
		postnote(PNPROC, pid, "kill");
}

/*
 *  reply to a command
 */
int
reply(char *fmt, ...)
{
	va_list arg;
	char buf[8192], *s;

	va_start(arg, fmt);
	s = vseprint(buf, buf+sizeof(buf)-3, fmt, arg);
	va_end(arg);
	if(debug){
		*s = 0;
		logit("%s", buf);
	}
	*s++ = '\r';
	*s++ = '\n';
	write(1, buf, s - buf);
	return 0;
}

int
sodoff(void)
{
	return reply("530 Sod off, service requires login");
}

/*
 *  run a command in a separate process
 */
int
asproc(void (*f)(char*, int), char *arg, int arg2)
{
	int i;

	if(pid){
		/* wait for previous command to finish */
		for(;;){
			i = waitpid();
			if(i == pid || i < 0)
				break;
		}
	}

	switch(pid = rfork(RFFDG|RFPROC|RFNOTEG)){
	case -1:
		return reply("450 Out of processes: %r");
	case 0:
		(*f)(arg, arg2);
		exits(0);
	default:
		break;
	}
	return 0;
}

/*
 * run a command to filter a tail
 */
int
transfer(char *cmd, char *a1, char *a2, char *a3, int image)
{
	int n, dfd, fd, bytes, eofs, pid;
	int pfd[2];
	char buf[Nbuf], *p;
	Waitmsg *w;

	reply("150 Opening data connection for %s (%s)", cmd, data);
	dfd = dialdata();
	if(dfd < 0)
		return reply("425 Error opening data connection: %r");

	if(pipe(pfd) < 0)
		return reply("520 Internal Error: %r");

	bytes = 0;
	switch(pid = rfork(RFFDG|RFPROC|RFNAMEG)){
	case -1:
		return reply("450 Out of processes: %r");
	case 0:
		logit("running %s %s %s %s pid %d",
			cmd, a1?a1:"", a2?a2:"" , a3?a3:"",getpid());
		close(pfd[1]);
		close(dfd);
		dup(pfd[0], 1);
		dup(pfd[0], 2);
		if(isnone){
			fd = open("#s/boot", ORDWR);
			if(fd < 0
			|| bind("#/", "/", MAFTER) < 0
			|| amount(fd, "/bin", MREPL, "") < 0
			|| bind("#c", "/dev", MAFTER) < 0
			|| bind(bindir, "/bin", MREPL) < 0)
				exits("building name space");
			close(fd);
		}
		execl(cmd, cmd, a1, a2, a3, nil);
		exits(cmd);
	default:
		close(pfd[0]);
		eofs = 0;
		while((n = read(pfd[1], buf, sizeof buf)) >= 0){
			if(n == 0){
				if(eofs++ > 5)
					break;
				else
					continue;
			}
			eofs = 0;
			p = buf;
			if(offset > 0){
				if(n > offset){
					p = buf+offset;
					n -= offset;
					offset = 0;
				} else {
					offset -= n;
					continue;
				}
			}
			if(!image)
				n = crlfwrite(dfd, p, n);
			else
				n = write(dfd, p, n);
			if(n < 0){
				postnote(PNPROC, pid, "kill");
				bytes = -1;
				break;
			}
			bytes += n;
		}
		close(pfd[1]);
		close(dfd);
		break;
	}

	/* wait for this command to finish */
	for(;;){
		w = wait();
		if(w == nil || w->pid == pid)
			break;
		free(w);
	}
	if(w != nil && w->msg != nil && w->msg[0] != 0){
		bytes = -1;
		logit("%s", w->msg);
		logit("%s %s %s %s failed %s", cmd, a1?a1:"", a2?a2:"" , a3?a3:"", w->msg);
	}
	free(w);
	reply("226 Transfer complete");
	return bytes;
}


/*
 *  just reply OK
 */
int
nopcmd(char *arg)
{
	USED(arg);
	reply("510 Plan 9 FTP daemon still alive");
	return 0;
}

/*
 *  login as user
 */
int
loginuser(char *user, char *nsfile, int gotoslash)
{
	logit("login %s %s %s %s", user, mailaddr, nci->rsys, nsfile);
	if(nsfile != nil && newns(user, nsfile) < 0){
		logit("namespace file %s does not exist", nsfile);
		return reply("530 Not logged in: login out of service");
	}
	getwd(curdir, sizeof(curdir));
	if(gotoslash){
		chdir("/");
		strcpy(curdir, "/");
	}
	putenv("service", "ftp");
	loggedin = 1;
	if(debug == 0)
		reply("230- If you have problems, send mail to 'postmaster'.");
	return reply("230 Logged in");
}

static void
slowdown(void)
{
	static ulong pause;

	if (pause) {
		sleep(pause);			/* deter guessers */
		if (pause < (1UL << 20))
			pause *= 2;
	} else
		pause = 1000;
}

/*
 *  get a user id, reply with a challenge.  The users 'anonymous'
 *  and 'ftp' are equivalent to 'none'.  The user 'none' requires
 *  no challenge.
 */
int
usercmd(char *name)
{
	slowdown();

	logit("user %s %s", name, nci->rsys);
	if(loggedin)
		return reply("530 Already logged in as %s", user);
	if(name == 0 || *name == 0)
		return reply("530 user command needs user name");
	isnoworld = 0;
	if(*name == ':'){
		debug = 1;
		name++;
	}
	strncpy(user, name, sizeof(user));
	if(debug)
		logit("debugging");
	user[sizeof(user)-1] = 0;
	if(strcmp(user, "anonymous") == 0 || strcmp(user, "ftp") == 0)
		strcpy(user, "none");
	else if(anon_everybody)
		strcpy(user,"none");

	if(strcmp(user, "Administrator") == 0 || strcmp(user, "admin") == 0)
		return reply("530 go away, script kiddie");
	else if(strcmp(user, "*none") == 0){
		if(!anon_ok)
			return reply("530 Not logged in: anonymous disallowed");
		return loginuser("none", namespace, 1);
	}
	else if(strcmp(user, "none") == 0){
		if(!anon_ok)
			return reply("530 Not logged in: anonymous disallowed");
		return reply("331 Send email address as password");
	}
	else if(anon_only)
		return reply("530 Not logged in: anonymous access only");

	isnoworld = noworld(name);
	if(isnoworld)
		return reply("331 OK");

	/* consult the auth server */
	if(ch)
		auth_freechal(ch);
	if((ch = auth_challenge("proto=p9cr role=server user=%q", user)) == nil)
		return reply("421 %r");
	return reply("331 encrypt challenge, %s, as a password", ch->chal);
}

/*
 *  get a password, set up user if it works.
 */
int
passcmd(char *response)
{
	char namefile[128];
	AuthInfo *ai;

	if(response == nil)
		response = "";

	if(strcmp(user, "none") == 0 || strcmp(user, "*none") == 0){
		/* for none, accept anything as a password */
		isnone = 1;
		strncpy(mailaddr, response, sizeof(mailaddr)-1);
		return loginuser("none", namespace, 1);
	}

	if(isnoworld){
		/* noworld gets a password in the clear */
		if(login(user, response, "/lib/namespace.noworld") < 0)
			return reply("530 Not logged in");
		createperm = 0664;
		/* login has already setup the namespace */
		return loginuser(user, nil, 0);
	} else {
		/* for everyone else, do challenge response */
		if(ch == nil)
			return reply("531 Send user id before encrypted challenge");
		ch->resp = response;
		ch->nresp = strlen(response);
		ai = auth_response(ch);
		if(ai == nil || auth_chuid(ai, nil) < 0) {
			slowdown();
			return reply("530 Not logged in: %r");
		}
		auth_freechal(ch);
		ch = nil;

		/* if the user has specified a namespace for ftp, use it */
		snprint(namefile, sizeof(namefile), "/usr/%s/lib/namespace.ftp", user);
		strcpy(mailaddr, user);
		createperm = 0660;
		if(access(namefile, 0) == 0)
			return loginuser(user, namefile, 0);
		else
			return loginuser(user, "/lib/namespace", 0);
	}
}

/*
 *  print working directory
 */
int
pwdcmd(char *arg)
{
	if(arg)
		return reply("550 Pwd takes no argument");
	return reply("257 \"%s\" is the current directory", curdir);
}

/*
 *  chdir
 */
int
cwdcmd(char *dir)
{
	char *rp;
	char buf[Maxpath];

	/* shell cd semantics */
	if(dir == 0 || *dir == 0){
		if(isnone)
			rp = "/";
		else {
			snprint(buf, sizeof buf, "/usr/%s", user);
			rp = buf;
		}
		if(accessok(rp) == 0)
			rp = nil;
	} else
		rp = abspath(dir);

	if(rp == nil)
		return reply("550 Permission denied");

	if(chdir(rp) < 0)
		return reply("550 Cwd failed: %r");
	strcpy(curdir, rp);
	return reply("250 directory changed to %s", curdir);
}

/*
 *  chdir ..
 */
int
cdupcmd(char *dp)
{
	USED(dp);
	return cwdcmd("..");
}

int
quitcmd(char *arg)
{
	USED(arg);
	reply("200 Bye");
	if(pid)
		postnote(PNPROC, pid, "kill");
	return -1;
}

int
typecmd(char *arg)
{
	int c;
	char *x;

	x = arg;
	if(arg == 0)
		return reply("501 Type command needs arguments");

	while(c = *arg++){
		switch(tolower(c)){
		case 'a':
			type = Tascii;
			break;
		case 'i':
		case 'l':
			type = Timage;
			break;
		case '8':
		case ' ':
		case 'n':
		case 't':
		case 'c':
			break;
		default:
			return reply("501 Unimplemented type %s", x);
		}
	}
	return reply("200 Type %s", type==Tascii ? "Ascii" : "Image");
}

int
modecmd(char *arg)
{
	if(arg == 0)
		return reply("501 Mode command needs arguments");
	while(*arg){
		switch(tolower(*arg)){
		case 's':
			mode = Mstream;
			break;
		default:
			return reply("501 Unimplemented mode %c", *arg);
		}
		arg++;
	}
	return reply("200 Stream mode");
}

int
structcmd(char *arg)
{
	if(arg == 0)
		return reply("501 Struct command needs arguments");
	for(; *arg; arg++){
		switch(tolower(*arg)){
		case 'f':
			structure = Sfile;
			break;
		default:
			return reply("501 Unimplemented structure %c", *arg);
		}
	}
	return reply("200 File structure");
}

int
portcmd(char *arg)
{
	char *field[7];
	int n;

	if(arg == 0)
		return reply("501 Port command needs arguments");
	n = getfields(arg, field, 7, 0, ", ");
	if(n != 6)
		return reply("501 Incorrect port specification");
	snprint(data, sizeof data, "tcp!%.3s.%.3s.%.3s.%.3s!%d", field[0], field[1], field[2],
		field[3], atoi(field[4])*256 + atoi(field[5]));
	return reply("200 Data port is %s", data);
}

int
mountnet(void)
{
	int rv;

	rv = 0;

	if(bind("#/", "/", MAFTER) < 0){
		logit("can't bind #/ to /: %r");
		return reply("500 can't bind #/ to /: %r");
	}

	if(bind(nci->spec, "/net", MBEFORE) < 0){
		logit("can't bind %s to /net: %r", nci->spec);
		rv = reply("500 can't bind %s to /net: %r", nci->spec);
		unmount("#/", "/");
	}

	return rv;
}

void
unmountnet(void)
{
	unmount(0, "/net");
	unmount("#/", "/");
}

int
pasvcmd(char *arg)
{
	NetConnInfo *nnci;
	Passive *p;

	USED(arg);
	p = &passive;

	if(p->inuse){
		close(p->afd);
		p->inuse = 0;
	}

	if(mountnet() < 0)
		return 0;

	p->afd = announce("tcp!*!0", passive.adir);
	if(p->afd < 0){
		unmountnet();
		return reply("500 No free ports");
	}
	nnci = getnetconninfo(p->adir, -1);
	unmountnet();

	/* parse the local address */
	if(debug)
		logit("local sys is %s", nci->lsys);
	parseip(p->ipaddr, nci->lsys);
	if(ipcmp(p->ipaddr, v4prefix) == 0 || ipcmp(p->ipaddr, IPnoaddr) == 0)
		parseip(p->ipaddr, nci->lsys);
	p->port = atoi(nnci->lserv);

	freenetconninfo(nnci);
	p->inuse = 1;

	return reply("227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
		p->ipaddr[IPv4off+0], p->ipaddr[IPv4off+1], p->ipaddr[IPv4off+2], p->ipaddr[IPv4off+3],
		p->port>>8, p->port&0xff);
}

enum
{
	Narg=32,
};
int Cflag, rflag, tflag, Rflag;
int maxnamelen;
int col;

char*
mode2asc(int m)
{
	static char asc[12];
	char *p;

	strcpy(asc, "----------");
	if(DMDIR & m)
		asc[0] = 'd';
	if(DMAPPEND & m)
		asc[0] = 'a';
	else if(DMEXCL & m)
		asc[3] = 'l';

	for(p = asc+1; p < asc + 10; p += 3, m<<=3){
		if(m & 0400)
			p[0] = 'r';
		if(m & 0200)
			p[1] = 'w';
		if(m & 0100)
			p[2] = 'x';
	}
	return asc;
}
void
listfile(Biobufhdr *b, char *name, int lflag, char *dname)
{
	char ts[32];
	int n, links, pad;
	long now;
	char *x;
	Dir *d;

	x = abspath(name);
	if(x == nil)
		return;
	d = dirstat(x);
	if(d == nil)
		return;
	if(isnone){
		if(strncmp(x, "/incoming/", sizeof("/incoming/")-1) != 0)
			d->mode &= ~0222;
		d->uid = "none";
		d->gid = "none";
	}

	strcpy(ts, ctime(d->mtime));
	ts[16] = 0;
	now = time(0);
	if(now - d->mtime > 6*30*24*60*60)
		memmove(ts+11, ts+23, 5);
	if(lflag){
		/* Unix style long listing */
		if(DMDIR&d->mode){
			links = 2;
			d->length = 512;
		} else
			links = 1;

		Bprint(b, "%s %3d %-8s %-8s %7lld %s ",
			mode2asc(d->mode), links,
			d->uid, d->gid, d->length, ts+4);
	}
	if(Cflag && maxnamelen < 40){
		n = strlen(name);
		pad = ((col+maxnamelen)/(maxnamelen+1))*(maxnamelen+1);
		if(pad+maxnamelen+1 < 60){
			Bprint(b, "%*s", pad-col+n, name);
			col = pad+n;
		}
		else{
			Bprint(b, "\r\n%s", name);
			col = n;
		}
	}
	else{
		if(dname)
			Bprint(b, "%s/", dname);
		Bprint(b, "%s\r\n", name);
	}
	free(d);
}
int
dircomp(void *va, void *vb)
{
	int rv;
	Dir *a, *b;

	a = va;
	b = vb;

	if(tflag)
		rv = b->mtime - a->mtime;
	else
		rv = strcmp(a->name, b->name);
	return (rflag?-1:1)*rv;
}
void
listdir(char *name, Biobufhdr *b, int lflag, int *printname, Globlist *gl)
{
	Dir *p;
	int fd, n, i, l;
	char *dname;
	uvlong total;

	col = 0;

	fd = open(name, OREAD);
	if(fd < 0){
		Bprint(b, "can't read %s: %r\r\n", name);
		return;
	}
	dname = 0;
	if(*printname){
		if(Rflag || lflag)
			Bprint(b, "\r\n%s:\r\n", name);
		else
			dname = name;
	}
	n = dirreadall(fd, &p);
	close(fd);
	if(Cflag){
		for(i = 0; i < n; i++){
			l = strlen(p[i].name);
			if(l > maxnamelen)
				maxnamelen = l;
		}
	}

	/* Unix style total line */
	if(lflag){
		total = 0;
		for(i = 0; i < n; i++){
			if(p[i].qid.type & QTDIR)
				total += 512;
			else
				total += p[i].length;
		}
		Bprint(b, "total %ulld\r\n", total/512);
	}

	qsort(p, n, sizeof(Dir), dircomp);
	for(i = 0; i < n; i++){
		if(Rflag && (p[i].qid.type & QTDIR)){
			*printname = 1;
			globadd(gl, name, p[i].name);
		}
		listfile(b, p[i].name, lflag, dname);
	}
	free(p);
}
void
list(char *arg, int lflag)
{
	Dir *d;
	Globlist *gl;
	Glob *g;
	int dfd, printname;
	int i, n, argc;
	char *alist[Narg];
	char **argv;
	Biobufhdr bh;
	uchar buf[512];
	char *p, *s;

	if(arg == 0)
		arg = "";

	if(debug)
		logit("ls %s (. = %s)", arg, curdir);

	/* process arguments, understand /bin/ls -l option */
	argv = alist;
	argv[0] = "/bin/ls";
	argc = getfields(arg, argv+1, Narg-2, 1, " \t") + 1;
	argv[argc] = 0;
	rflag = 0;
	tflag = 0;
	Rflag = 0;
	Cflag = 0;
	col = 0;
	ARGBEGIN{
	case 'l':
		lflag++;
		break;
	case 'R':
		Rflag++;
		break;
	case 'C':
		Cflag++;
		break;
	case 'r':
		rflag++;
		break;
	case 't':
		tflag++;
		break;
	}ARGEND;
	if(Cflag)
		lflag = 0;

	dfd = dialdata();
	if(dfd < 0){
		reply("425 Error opening data connection:%r");
		return;
	}
	reply("150 Opened data connection (%s)", data);

	Binits(&bh, dfd, OWRITE, buf, sizeof(buf));
	if(argc == 0){
		argc = 1;
		argv = alist;
		argv[0] = ".";
	}

	for(i = 0; i < argc; i++){
		chdir(curdir);
		gl = glob(argv[i]);
		if(gl == nil)
			continue;

		printname = gl->first != nil && gl->first->next != nil;
		maxnamelen = 8;

		if(Cflag)
			for(g = gl->first; g; g = g->next)
				if(g->glob && (n = strlen(s_to_c(g->glob))) > maxnamelen)
					maxnamelen = n;
		while(s = globiter(gl)){
			if(debug)
				logit("glob %s", s);
			p = abspath(s);
			if(p == nil){
				free(s);
				continue;
			}
			d = dirstat(p);
			if(d == nil){
				free(s);
				continue;
			}
			if(d->qid.type & QTDIR)
				listdir(s, &bh, lflag, &printname, gl);
			else
				listfile(&bh, s, lflag, 0);
			free(s);
			free(d);
		}
		globlistfree(gl);
	}
	if(Cflag)
		Bprint(&bh, "\r\n");
	Bflush(&bh);
	close(dfd);

	reply("226 Transfer complete (list %s)", arg);
}
int
namelistcmd(char *arg)
{
	return asproc(list, arg, 0);
}
int
listcmd(char *arg)
{
	return asproc(list, arg, 1);
}

/*
 * fuse compatability
 */
int
oksiteuser(void)
{
	char buf[64];
	int fd, n;

	fd = open("#c/user", OREAD);
	if(fd < 0)
		return 1;
	n = read(fd, buf, sizeof buf - 1);
	if(n > 0){
		buf[n] = 0;
		if(strcmp(buf, "none") == 0)
			n = -1;
	}
	close(fd);
	return n > 0;
}

int
sitecmd(char *arg)
{
	char *f[4];
	int nf, r;
	Dir *d;

	if(arg == 0)
		return reply("501 bad site command");
	nf = tokenize(arg, f, nelem(f));
	if(nf != 3 || cistrcmp(f[0], "chmod") != 0)
		return reply("501 bad site command");
	if(!oksiteuser())
		return reply("550 Permission denied");
	d = dirstat(f[2]);
	if(d == nil)
		return reply("501 site chmod: file does not exist");
	d->mode &= ~0777;
	d->mode |= strtoul(f[1], 0, 8) & 0777;
	r = dirwstat(f[2], d);
	free(d);
	if(r < 0)
		return reply("550 Permission denied %r");
	return reply("200 very well, then");
 }

/*
 *  return the size of the file
 */
int
sizecmd(char *arg)
{
	Dir *d;
	int rv;

	if(arg == 0)
		return reply("501 Size command requires pathname");
	arg = abspath(arg);
	d = dirstat(arg);
	if(d == nil)
		return reply("501 %r accessing %s", arg);
	rv = reply("213 %lld", d->length);
	free(d);
	return rv;
}

/*
 *  return the modify time of the file
 */
int
mdtmcmd(char *arg)
{
	Dir *d;
	Tm *t;
	int rv;

	if(arg == 0)
		return reply("501 Mdtm command requires pathname");
	if(arg == 0)
		return reply("550 Permission denied");
	d = dirstat(arg);
	if(d == nil)
		return reply("501 %r accessing %s", arg);
	t = gmtime(d->mtime);
	rv = reply("213 %4.4d%2.2d%2.2d%2.2d%2.2d%2.2d",
			t->year+1900, t->mon+1, t->mday,
			t->hour, t->min, t->sec);
	free(d);
	return rv;
}

/*
 *  set an offset to start reading a file from
 *  only lasts for one command
 */
int
restartcmd(char *arg)
{
	if(arg == 0)
		return reply("501 Restart command requires offset");
	offset = atoll(arg);
	if(offset < 0){
		offset = 0;
		return reply("501 Bad offset");
	}

	return reply("350 Restarting at %lld. Send STORE or RETRIEVE", offset);
}

/*
 *  send a file to the user
 */
int
crlfwrite(int fd, char *p, int n)
{
	char *ep, *np;
	char buf[2*Nbuf];

	for(np = buf, ep = p + n; p < ep; p++){
		if(*p == '\n')
			*np++ = '\r';
		*np++ = *p;
	}
	if(write(fd, buf, np - buf) == np - buf)
		return n;
	else
		return -1;
}
void
retrievedir(char *arg)
{
	int n;
	char *p;
	String *file;

	if(type != Timage){
		reply("550 This file requires type binary/image");
		return;
	}

	file = s_copy(arg);
	p = strrchr(s_to_c(file), '/');
	if(p != s_to_c(file)){
		*p++ = 0;
		chdir(s_to_c(file));
	} else {
		chdir("/");
		p = s_to_c(file)+1;
	}

	n = transfer("/bin/tar", "c", p, 0, 1);
	if(n < 0)
		logit("get %s failed", arg);
	else
		logit("get %s OK %d", arg, n);
	s_free(file);
}
void
retrieve(char *arg, int arg2)
{
	int dfd, fd, n, i, bytes;
	Dir *d;
	char buf[Nbuf];
	char *p, *ep;

	USED(arg2);

	p = strchr(arg, '\r');
	if(p){
		logit("cr in file name", arg);
		*p = 0;
	}

	fd = open(arg, OREAD);
	if(fd == -1){
		n = strlen(arg);
		if(n > 4 && strcmp(arg+n-4, ".tar") == 0){
			*(arg+n-4) = 0;
			d = dirstat(arg);
			if(d != nil){
				if(d->qid.type & QTDIR){
					retrievedir(arg);
					free(d);
					return;
				}
				free(d);
			}
		}
		logit("get %s failed", arg);
		reply("550 Error opening %s: %r", arg);
		return;
	}
	if(offset != 0)
		if(seek(fd, offset, 0) < 0){
			reply("550 %s: seek to %lld failed", arg, offset);
			close(fd);
			return;
		}
	d = dirfstat(fd);
	if(d != nil){
		if(d->qid.type & QTDIR){
			reply("550 %s: not a plain file.", arg);
			close(fd);
			free(d);
			return;
		}
		free(d);
	}

	n = read(fd, buf, sizeof(buf));
	if(n < 0){
		logit("get %s failed", arg, mailaddr, nci->rsys);
		reply("550 Error reading %s: %r", arg);
		close(fd);
		return;
	}

	if(type != Timage)
		for(p = buf, ep = &buf[n]; p < ep; p++)
			if(*p & 0x80){
				close(fd);
				reply("550 This file requires type binary/image");
				return;
			}

	reply("150 Opening data connection for %s (%s)", arg, data);
	dfd = dialdata();
	if(dfd < 0){
		reply("425 Error opening data connection:%r");
		close(fd);
		return;
	}

	bytes = 0;
	do {
		switch(type){
		case Timage:
			i = write(dfd, buf, n);
			break;
		default:
			i = crlfwrite(dfd, buf, n);
			break;
		}
		if(i != n){
			close(fd);
			close(dfd);
			logit("get %s %r to data connection after %d", arg, bytes);
			reply("550 Error writing to data connection: %r");
			return;
		}
		bytes += n;
	} while((n = read(fd, buf, sizeof(buf))) > 0);

	if(n < 0)
		logit("get %s %r after %d", arg, bytes);

	close(fd);
	close(dfd);
	reply("226 Transfer complete");
	logit("get %s OK %d", arg, bytes);
}
int
retrievecmd(char *arg)
{
	if(arg == 0)
		return reply("501 Retrieve command requires an argument");
	arg = abspath(arg);
	if(arg == 0)
		return reply("550 Permission denied");

	return asproc(retrieve, arg, 0);
}

/*
 *  get a file from the user
 */
int
lfwrite(int fd, char *p, int n)
{
	char *ep, *np;
	char buf[Nbuf];

	for(np = buf, ep = p + n; p < ep; p++){
		if(*p != '\r')
			*np++ = *p;
	}
	if(write(fd, buf, np - buf) == np - buf)
		return n;
	else
		return -1;
}
void
store(char *arg, int fd)
{
	int dfd, n, i;
	char buf[Nbuf];

	reply("150 Opening data connection for %s (%s)", arg, data);
	dfd = dialdata();
	if(dfd < 0){
		reply("425 Error opening data connection:%r");
		close(fd);
		return;
	}

	while((n = read(dfd, buf, sizeof(buf))) > 0){
		switch(type){
		case Timage:
			i = write(fd, buf, n);
			break;
		default:
			i = lfwrite(fd, buf, n);
			break;
		}
		if(i != n){
			close(fd);
			close(dfd);
			reply("550 Error writing file");
			return;
		}
	}
	close(fd);
	close(dfd);
	logit("put %s OK", arg);
	reply("226 Transfer complete");
}
int
storecmd(char *arg)
{
	int fd, rv;

	if(arg == 0)
		return reply("501 Store command requires an argument");
	arg = abspath(arg);
	if(arg == 0)
		return reply("550 Permission denied");
	if(isnone && strncmp(arg, "/incoming/", sizeof("/incoming/")-1))
		return reply("550 Permission denied");
	if(offset){
		fd = open(arg, OWRITE);
		if(fd == -1)
			return reply("550 Error opening %s: %r", arg);
		if(seek(fd, offset, 0) == -1)
			return reply("550 Error seeking %s to %d: %r",
				arg, offset);
	} else {
		fd = create(arg, OWRITE, createperm);
		if(fd == -1)
			return reply("550 Error creating %s: %r", arg);
	}

	rv = asproc(store, arg, fd);
	close(fd);
	return rv;
}
int
appendcmd(char *arg)
{
	int fd, rv;

	if(arg == 0)
		return reply("501 Append command requires an argument");
	if(isnone)
		return reply("550 Permission denied");
	arg = abspath(arg);
	if(arg == 0)
		return reply("550 Error creating %s: Permission denied", arg);
	fd = open(arg, OWRITE);
	if(fd == -1){
		fd = create(arg, OWRITE, createperm);
		if(fd == -1)
			return reply("550 Error creating %s: %r", arg);
	}
	seek(fd, 0, 2);

	rv = asproc(store, arg, fd);
	close(fd);
	return rv;
}
int
storeucmd(char *arg)
{
	int fd, rv;
	char name[Maxpath];

	USED(arg);
	if(isnone)
		return reply("550 Permission denied");
	strncpy(name, "ftpXXXXXXXXXXX", sizeof name);
	mktemp(name);
	fd = create(name, OWRITE, createperm);
	if(fd == -1)
		return reply("550 Error creating %s: %r", name);

	rv = asproc(store, name, fd);
	close(fd);
	return rv;
}

int
mkdircmd(char *name)
{
	int fd;

	if(name == 0)
		return reply("501 Mkdir command requires an argument");
	if(isnone)
		return reply("550 Permission denied");
	name = abspath(name);
	if(name == 0)
		return reply("550 Permission denied");
	fd = create(name, OREAD, DMDIR|0775);
	if(fd < 0)
		return reply("550 Can't create %s: %r", name);
	close(fd);
	return reply("226 %s created", name);
}

int
delcmd(char *name)
{
	if(name == 0)
		return reply("501 Rmdir/delete command requires an argument");
	if(isnone)
		return reply("550 Permission denied");
	name = abspath(name);
	if(name == 0)
		return reply("550 Permission denied");
	if(remove(name) < 0)
		return reply("550 Can't remove %s: %r", name);
	else
		return reply("226 %s removed", name);
}

/*
 *  kill off the last transfer (if the process still exists)
 */
int
abortcmd(char *arg)
{
	USED(arg);

	logit("abort pid %d", pid);
	if(pid){
		if(postnote(PNPROC, pid, "kill") == 0)
			reply("426 Command aborted");
		else
			logit("postnote pid %d %r", pid);
	}
	return reply("226 Abort processed");
}

int
systemcmd(char *arg)
{
	USED(arg);
	return reply("215 UNIX Type: L8 Version: Plan 9");
}

int
helpcmd(char *arg)
{
	int i;
	char buf[80];
	char *p, *e;

	USED(arg);
	reply("214- the following commands are implemented:");
	p = buf;
	e = buf+sizeof buf;
	for(i = 0; cmdtab[i].name; i++){
		if((i%8) == 0){
			reply("214-%s", buf);
			p = buf;
		}
		p = seprint(p, e, " %-5.5s", cmdtab[i].name);
	}
	if(p != buf)
		reply("214-%s", buf);
	reply("214 ");
	return 0;
}

/*
 *  renaming a file takes two commands
 */
static String *filepath;

int
rnfrcmd(char *from)
{
	if(isnone)
		return reply("550 Permission denied");
	if(from == 0)
		return reply("501 Rename command requires an argument");
	from = abspath(from);
	if(from == 0)
		return reply("550 Permission denied");
	if(filepath == nil)
		filepath = s_copy(from);
	else{
		s_reset(filepath);
		s_append(filepath, from);
	}
	return reply("350 Rename %s to ...", s_to_c(filepath));
}
int
rntocmd(char *to)
{
	int r;
	Dir nd;
	char *fp, *tp;

	if(isnone)
		return reply("550 Permission denied");
	if(to == 0)
		return reply("501 Rename command requires an argument");
	to = abspath(to);
	if(to == 0)
		return reply("550 Permission denied");
	if(filepath == nil || *(s_to_c(filepath)) == 0)
		return reply("503 Rnto must be preceeded by an rnfr");

	tp = strrchr(to, '/');
	fp = strrchr(s_to_c(filepath), '/');
	if((tp && fp == 0) || (fp && tp == 0)
	|| (fp && tp && (fp-s_to_c(filepath) != tp-to || memcmp(s_to_c(filepath), to, tp-to))))
		return reply("550 Rename can't change directory");
	if(tp)
		to = tp+1;

	nulldir(&nd);
	nd.name = to;
	if(dirwstat(s_to_c(filepath), &nd) < 0)
		r = reply("550 Can't rename %s to %s: %r\n", s_to_c(filepath), to);
	else
		r = reply("250 %s now %s", s_to_c(filepath), to);
	s_reset(filepath);

	return r;
}

/*
 *  to dial out we need the network file system in our
 *  name space.
 */
int
dialdata(void)
{
	int fd, cfd;
	char ldir[40];
	char err[Maxerr];

	if(mountnet() < 0)
		return -1;

	if(!passive.inuse){
		fd = dial(data, "20", 0, 0);
		errstr(err, sizeof err);
	} else {
		alarm(5*60*1000);
		cfd = listen(passive.adir, ldir);
		alarm(0);
		errstr(err, sizeof err);
		if(cfd < 0)
			return -1;
		fd = accept(cfd, ldir);
		errstr(err, sizeof err);
		close(cfd);
	}
	if(fd < 0)
		logit("can't dial %s: %s", data, err);

	unmountnet();
	werrstr(err, sizeof err);
	return fd;
}

int
postnote(int group, int pid, char *note)
{
	char file[128];
	int f, r;

	/*
	 * Use #p because /proc may not be in the namespace.
	 */
	switch(group) {
	case PNPROC:
		sprint(file, "#p/%d/note", pid);
		break;
	case PNGROUP:
		sprint(file, "#p/%d/notepg", pid);
		break;
	default:
		return -1;
	}

	f = open(file, OWRITE);
	if(f < 0)
		return -1;

	r = strlen(note);
	if(write(f, note, r) != r) {
		close(f);
		return -1;
	}
	close(f);
	return 0;
}

/*
 *  to circumscribe the accessible files we have to eliminate ..'s
 *  and resolve all names from the root.  We also remove any /bin/rc
 *  special characters to avoid later problems with executed commands.
 */
char *special = "`;| ";

char*
abspath(char *origpath)
{
	char *p, *sp, *path;
	static String *rpath;

	if(rpath == nil)
		rpath = s_new();
	else
		s_reset(rpath);

	if(origpath == nil)
		s_append(rpath, curdir);
	else{
		if(*origpath != '/'){
			s_append(rpath, curdir);
			s_append(rpath, "/");
		}
		s_append(rpath, origpath);
	}
	path = s_to_c(rpath);

	for(sp = special; *sp; sp++){
		p = strchr(path, *sp);
		if(p)
			*p = 0;
	}

	cleanname(s_to_c(rpath));
	rpath->ptr = rpath->base+strlen(rpath->base);

	if(!accessok(s_to_c(rpath)))
		return nil;

	return s_to_c(rpath);
}

typedef struct Path Path;
struct Path {
	Path	*next;
	String	*path;
	int	inuse;
	int	ok;
};

enum
{
	Maxlevel = 16,
	Maxperlevel= 8,
};

Path *pathlevel[Maxlevel];

Path*
unlinkpath(char *path, int level)
{
	String *s;
	Path **l, *p;
	int n;

	n = 0;
	for(l = &pathlevel[level]; *l; l = &(*l)->next){
		p = *l;
		/* hit */
		if(strcmp(s_to_c(p->path), path) == 0){
			*l = p->next;
			p->next = nil;
			return p;
		}
		/* reuse */
		if(++n >= Maxperlevel){
			*l = p->next;
			s = p->path;
			s_reset(p->path);
			memset(p, 0, sizeof *p);
			p->path = s_append(s, path);
			return p;
		}
	}

	/* allocate */
	p = mallocz(sizeof *p, 1);
	p->path = s_copy(path);
	return p;
}

void
linkpath(Path *p, int level)
{
	p->next = pathlevel[level];
	pathlevel[level] = p;
	p->inuse = 1;
}

void
addpath(Path *p, int level, int ok)
{
	p->ok = ok;
	p->next = pathlevel[level];
	pathlevel[level] = p;
}

int
_accessok(String *s, int level)
{
	Path *p;
	char *cp;
	int lvl, offset;
	static char httplogin[] = "/.httplogin";

	if(level < 0)
		return 1;
	lvl = level;
	if(lvl >= Maxlevel)
		lvl = Maxlevel - 1;

	p = unlinkpath(s_to_c(s), lvl);
	if(p->inuse){
		/* move to front */
		linkpath(p, lvl);
		return p->ok;
	}
	cp = strrchr(s_to_c(s), '/');
	if(cp == nil)
		offset = 0;
	else
		offset = cp - s_to_c(s);
	s_append(s, httplogin);
	if(access(s_to_c(s), AEXIST) == 0){
		addpath(p, lvl, 0);
		return 0;
	}

	/*
	 * There's no way to shorten a String without
	 * knowing the implementation.
	 */
	s->ptr = s->base+offset;
	s_terminate(s);
	addpath(p, lvl, _accessok(s, level-1));

	return p->ok;
}

/*
 * check for a subdirectory containing .httplogin
 * at each level of the path.
 */
int
accessok(char *path)
{
	int level, r;
	char *p;
	String *npath;

	npath = s_copy(path);
	p = s_to_c(npath)+1;
	for(level = 1; level < Maxlevel; level++){
		p = strchr(p, '/');
		if(p == nil)
			break;
		p++;
	}

	r = _accessok(npath, level-1);
	s_free(npath);

	return r;
}
