#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <ip.h>
#include <libsec.h>

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

	Maxpath=	512,
};

typedef struct Endpoints Endpoints;
struct Endpoints
{
	char	*lsys;
	char	*lserv;
	char	*rsys;
	char	*rserv;
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
int	sizecmd(char*);
int	storecmd(char*);
int	storeucmd(char*);
int	structcmd(char*);
int	systemcmd(char*);
int	typecmd(char*);
int	usercmd(char*);

int	dialdata(void);
char*	abspath(char*);
Endpoints*	getendpoints(char*);
void	freeendpoints(Endpoints*);
int	crlfwrite(int, char*, int);
void	setipifc(char*);
int	sodoff(void);

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
	{ "rnfr",	rnfrcmd,	1, },
	{ "rnto",	rntocmd,	1, },
	{ "rmd",	delcmd,		1, },
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

char	user[NAMELEN];		/* logged in user */
char	curdir[Maxpath];	/* current directory path */
Chalstate	ch;
int	loggedin;
int	type;			/* transmission type */
int	mode;			/* transmission mode */
int	structure;		/* file structure */
char	data[64];		/* data address */
int	pid;			/* transfer process */
int	encryption;		/* encryption state */
int	isnone, nonone;
char	cputype[NAMELEN];	/* the environment variable of the same name */
char	bindir[2*NAMELEN];	/* bin directory for this architecture */
char	mailaddr[2*NAMELEN];
char	*namespace = NONENS;
char	ipifc[32];
int	debug;
Endpoints	*ends;
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

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	syslog(0, FTPLOG, "%s.%s %s", ends->rsys, ends->rserv, buf);
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

	nonone = 1;

	ARGBEGIN{
	case 'd':
		debug++;
		break;
	case 'a':
		nonone = 0;
		break;
	case 'n':
		namespace = ARGF();
		break;
	}ARGEND

	/* open log file before doing a newns */
	syslog(0, FTPLOG, nil);

	/* find out who is calling */
	if(argc)
		ends = getendpoints(argv[argc-1]);
	else
		ends = getendpoints("/dev/null");

	/* remember which interface to bind in later */
	setipifc(argv[argc-1]);

	strcpy(mailaddr, "?");
	id = getpid();

	/* figure out which binaries to bind in later */
	arg = getenv("cputype");
	if(arg)
		strcpy(cputype, arg);
	else
		strcpy(cputype, "mips");
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
		while(*cmd && *cmd == Iac){
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
		if(t->f == restartcmd){
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
		postnote(PNGROUP, pid, "kill");
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
	s = doprint(buf, buf+sizeof(buf), fmt, arg);
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
	Waitmsg w;
	int i;

	if(pid){
		/* wait for previous command to finish */
		for(;;){
			i = wait(&w);
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
	Waitmsg w;
	int i, n, dfd, fd, bytes, eofs, pid;
	int pfd[2];
	char buf[Nbuf], *p;

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
		close(pfd[1]);
		close(dfd);
		dup(pfd[0], 1);
		dup(pfd[0], 2);
		if(isnone){
			fd = open("#s/boot", ORDWR);
			if(fd < 0
			|| bind("#/", "/", MAFTER) < 0
			|| mount(fd, "/bin", MREPL, "") < 0
			|| bind("#c", "/dev", MAFTER) < 0
			|| bind(bindir, "/bin", MREPL) < 0)
				exits("building name space");
			close(fd);
		}
		execl(cmd, cmd, a1, a2, a3, 0);
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
		close(dfd);
		break;
	}

	/* wait for this command to finish */
	w.msg[0] = 0;
	for(;;){
		i = wait(&w);
		if(i == pid || i < 0)
			break;
	}
	if(w.msg[0]){
		bytes = -1;
		logit("%s\n", w.msg);
		logit("%s %s %s %s failed %s\n", cmd, a1?a1:"", a2?a2:"" , a3?a3:"", w.msg);
	}
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
	logit("login %s %s %s %s", user, mailaddr, ends->rsys, nsfile);
	if(nsfile != nil && newns(user, nsfile) < 0){
		logit("namespace file %s does not exist", nsfile);
		return reply("530 Not logged in: login out of service");
		return -1;
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

/*
 *  get a user id, reply with a challenge.  The users 'anonymous'
 *  and 'ftp' are equivalent to 'none'.  The user 'none' requires
 *  no challenge.
 */
int
usercmd(char *name)
{
	logit("user %s %s", name, ends->rsys);
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
	if(strcmp(user, "*none") == 0){
		if(nonone)
			return reply("530 Not logged in: anonymous disallowed");
		return loginuser("none", namespace, 1);
	}
	if(strcmp(user, "none") == 0){
		if(nonone)
			return reply("530 Not logged in: anonymous disallowed");
		return reply("331 Send email address as password");
	}
	isnoworld = noworld(name);
	if(isnoworld)
		return reply("331 OK");
	if(getchal(&ch, user) < 0)
		return reply("421 %r");
	return reply("331 encrypt challenge, %s, as a password", ch.chal);
}

/*
 *  get a password, set up user if it works.
 */
int
passcmd(char *response)
{
	char namefile[128];
	char x[DIRLEN];

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
		if(ch.afd < 0)
			return reply("531 Send user id before encrypted challenge");
		if(chalreply(&ch, response) < 0)
			return reply("530 Not logged in: %r");

		/* if the user has specified a namespace for ftp, use it */
		snprint(namefile, sizeof(namefile), "/usr/%s/lib/namespace.ftp", user);
		strcpy(mailaddr, user);
		createperm = 0660;
		if(stat(namefile, x) == 0)
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
	char buf[2*NAMELEN];

	/* shell cd semantics */
	if(dir == 0 || *dir == 0){
		if(isnone)
			rp = "/";
		else {
			snprint(buf, sizeof buf, "/usr/%s", user);
			rp = buf;
		}
	} else
		rp = abspath(dir);

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
		postnote(PNGROUP, pid, "kill");
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
			break;
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
			break;
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
			break;
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

/* figure out which ip interface to mount later */
void
setipifc(char *p)
{
	Dir d;

	if(dirstat(p, &d) < 0)
		sprint(ipifc, "#I0");
	else
		sprint(ipifc, "#I%d", d.dev);
}

int
mountnet(void)
{
	if(bind("#/", "/", MAFTER) < 0){
		logit("can't bind #/ to /: %r");
		return reply("500 can't bind #/ to /: %r");
	}

	if(bind(ipifc, "/net", MBEFORE) < 0){
		unmount("#/", "/");
		logit("can't bind %s to /net: %r", ipifc);
		return reply("500 can't bind %s to /net: %r", ipifc);
	}

	return 0;
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
	Endpoints *ep;
	Passive *p;
	static Ipifc *ifcs;

	USED(arg);
	p = &passive;

	if(p->inuse){
		close(p->afd);
		p->inuse = 0;
	}

	if(mountnet() < 0)
		return 0;

	/* announce on a port picked by the system */
	p->afd = announce("tcp!*!0", passive.adir);
	if(p->afd < 0){
		unmountnet();
		return reply("500 No free ports");
	}
	ep = getendpoints(p->adir);

	/* get the address the client called, of failing that one that
         * should work.
	 */
	parseip(p->ipaddr, ends->lsys);
	if(ipcmp(p->ipaddr, IPnoaddr) == 0 || ipcmp(p->ipaddr, v4prefix) == 0 ){
		if(ifcs == nil)
			ifcs = readipifc("/net", ifcs);
		ipmove(p->ipaddr, ifcs->ip);
	}
	p->port = atoi(ep->lserv);
	unmountnet();
	freeendpoints(ep);
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
	if(CHDIR & m)
		asc[0] = 'd';
	if(CHAPPEND & m)
		asc[0] = 'a';
	else if(CHEXCL & m)
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
	char buf[256];
	int n, links;
	long now;
	char *x;
	Dir d;

	x = abspath(name);
	if(dirstat(x, &d) < 0)
		return;
	if(isnone){
		if(strncmp(x, "/incoming/", sizeof("/incoming/")-1) != 0)
			d.mode &= ~0222;
		strcpy(d.uid, "none");
		strcpy(d.gid, "none");
	}

	strcpy(ts, ctime(d.mtime));
	ts[16] = 0;
	now = time(0);
	if(now - d.mtime > 6*30*24*60*60)
		memmove(ts+11, ts+23, 5);
	if(lflag){
		/* Unix style long listing */
		if(CHDIR&d.mode){
			links = 2;
			d.length = 512;
		} else
			links = 1;
		
		n = sprint(buf, "%s %3d %-8s %-8s %7lld %s ", mode2asc(d.mode), links,
			d.uid, d.gid, d.length, ts+4);
	} else
		n = 0;
	if(dname)
		n += sprint(buf+n, "%s/", dname);
	n += sprint(buf+n, "%s", name);
	if(Cflag && col + maxnamelen + 1 < 40){
		if(n < maxnamelen+1){
			memset(buf+n, ' ', maxnamelen+1-n);
			buf[maxnamelen+1] = 0;
		}
		col += maxnamelen + 1;
	} else {
		sprint(buf+n, "\r\n");
		col = 0;
	}
	Bwrite(b, buf, strlen(buf));
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
listdir(char *name, Biobufhdr *b, int lflag, int *printname)
{
	Dir *p;
	int fd, n, na, i;
	ulong total;
	char *dname;
	char buf[Maxpath];

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
	na = 64;
	p = malloc(na*sizeof(Dir));
	if(Cflag){
		for(;;){
			i = dirread(fd, p, sizeof(Dir)*na);
			if(i <= 0)
				break;
			n = i/sizeof(Dir);
			for(i = 0; i < n; i++)
				if(strlen(p[i].name) > maxnamelen)
					maxnamelen = strlen(p[i].name);
		}
		close(fd);
		fd = open(name, OREAD);
	}
	n = 0;
	for(;;){
		i = dirread(fd, &p[n], sizeof(Dir)*(na-n));
		if(i <= 0)
			break;
		n += i/sizeof(Dir);
		if(n == na){
			na += 32;
			p = realloc(p, na*sizeof(Dir));
		}
	}
	close(fd);

	/* Unix style total line: heh, heh, cool */
	if(lflag){
		total = 0;
		for(i = 0; i < n; i++){
			if(p[i].mode & CHDIR)
				total += 512;
			else
				total += p[i].length;
		}
		Bprint(b, "total %uld\r\n", total/512);
	}
	
	qsort(p, n, sizeof(Dir), dircomp);
	for(i = 0; i < n; i++){
		if(Rflag && (p[i].qid.path & CHDIR)){
			*printname = 1;
			snprint(buf, sizeof buf, "%s/%s", name, p[i].name);
			globadd(buf);
		}
		listfile(b, p[i].name, lflag, dname);
	}
	free(p);
}
void
list(char *arg, int lflag)
{
	Dir d;
	Glob *g, *topg;
	int dfd, printname;
	int i, n, argc;
	char *alist[Narg];
	char path[Maxpath];
	char **argv;
	Biobufhdr bh;
	uchar buf[512];

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
		strncpy(path, argv[i], sizeof(path));
		path[sizeof(path)-1] = 0;
		topg = glob(path);
		printname = topg->next != 0;
		maxnamelen = 8;

		if(Cflag)
			for(g = topg; g; g = g->next)
				if(g->glob && (n = strlen(g->glob)) > maxnamelen)
					maxnamelen = n;

		for(g = topg; g; g = g->next){
			if(debug){
				if(g->glob == 0)
					logit("glob <null>");
				else
					logit("glob %s", g->glob);
			}
			if(g->glob == 0 || *g->glob == 0)
				continue;
			if(dirstat(abspath(g->glob), &d) < 0)
				continue;
			if(d.qid.path & CHDIR)
				listdir(g->glob, &bh, lflag, &printname);
			else
				listfile(&bh, g->glob, lflag, 0);
		}
		globfree(topg);
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
 *  return the size of the file
 */
int
sizecmd(char *arg)
{
	Dir d;

	if(arg == 0)
		return reply("501 Size command requires pathname");
	arg = abspath(arg);
	if(dirstat(arg, &d) < 0)
		return reply("501 %r accessing %s", arg);
	return reply("213 %lld", d.length);
}

/*
 *  return the modify time of the file
 */
int
mdtmcmd(char *arg)
{
	Dir d;
	Tm *t;

	if(arg == 0)
		return reply("501 Mdtm command requires pathname");
	arg = abspath(arg);
	if(dirstat(arg, &d) < 0)
		return reply("501 %r accessing %s", arg);
	t = gmtime(d.mtime);
	return reply("213 %4.4d%2.2d%2.2d%2.2d%2.2d%2.2d",
			t->year+1900, t->mon+1, t->mday,
			t->hour, t->min, t->sec);
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
	char file[Maxpath];

	if(type != Timage){
		reply("550 This file requires type binary/image");
		return;
	}

	strcpy(file, arg);
	p = strrchr(arg, '/');
	if(p != arg){
		*p++ = 0;
		chdir(arg);
	} else {
		chdir("/");
		p = arg+1;
	}

	n = transfer("/bin/tar", "c", p, 0, 1);
	if(n < 0)
		logit("get %s failed", file);
	else
		logit("get %s OK %d", file, n);
}
void
retrieve(char *arg, int arg2)
{
	int dfd, fd, n, i, bytes;
	Dir d;
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
			if(dirstat(arg, &d) == 0)
				if(d.qid.path & CHDIR){
					retrievedir(arg);
					return;
				}
		}
		logit("get %s failed", arg);
		reply("550 Error opening %s: %r", arg);
		return;
	}
	if(offset != 0)
		if(seek(fd, 0, offset) < 0){
			reply("550 %s: seek to %lld failed", arg, offset);
			close(fd);
			return;
		}
	if(dirfstat(fd, &d) == 0){
		if(d.qid.path & CHDIR){
			reply("550 %s: not a plain file.", arg);
			close(fd);
			return;
		}
	}

	n = read(fd, buf, sizeof(buf));
	if(n < 0){
		logit("get %s failed", arg, mailaddr, ends->rsys);
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
	if(isnone && strncmp(arg, "/incoming/", sizeof("/incoming/")-1))
		return reply("550 Permission denied");
	if(offset){
		fd = open(arg, OWRITE);
		if(fd == -1)
			return reply("550 Error opening %s: %r", arg);
		if(seek(fd, 0, offset) == -1)
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
	char name[NAMELEN];

	USED(arg);
	if(isnone)
		return reply("550 Permission denied");
	strcpy(name, "ftpXXXXXXXXXXX");
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
	fd = create(name, OREAD, CHDIR|0775);
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
	if(pid && postnote(PNGROUP, pid, "kill") == 0)
		reply("426 Command aborted");
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

	USED(arg);
	reply("214- the following commands are implemented:");
	for(i = 0; cmdtab[i].name; i++){
		if((i%8) == 0)
			print("\r\n");
		reply("\t%s", cmdtab[i].name);
	}
	reply("\r\n214 \r\n");
	return 0;
}

/*
 *  renaming a file takes two commands
 */
static char filepath[256];

int
rnfrcmd(char *from)
{
	if(isnone)
		return reply("550 Permission denied");
	if(from == 0)
		return reply("501 Rename command requires an argument");
	from = abspath(from);
	strncpy(filepath, from, sizeof(filepath));
	filepath[sizeof(filepath)-1] = 0;
	return reply("350 Rename %s to ...", filepath);
}
int
rntocmd(char *to)
{
	Dir d;
	char *fp;
	char *tp;

	if(isnone)
		return reply("550 Permission denied");
	if(to == 0)
		return reply("501 Rename command requires an argument");
	to = abspath(to);
	if(*filepath == 0)
		return reply("503 Rnto must be preceeded by an rnfr");

	tp = strrchr(to, '/');
	fp = strrchr(filepath, '/');
	if((tp && fp == 0) || (fp && tp == 0)
	|| (fp && tp && (fp-filepath != tp-to || memcmp(filepath, to, tp-to))))
		return reply("550 Rename can't change directory");
	if(tp)
		to = tp+1;

	if(dirstat(filepath, &d) < 0)
		return reply("550 Can't rename %s: %r\n", filepath);
	strncpy(d.name, to, sizeof(d.name));
	if(dirwstat(filepath, &d) < 0)
		return reply("550 Can't rename %s to %s: %r\n", filepath, to);

	filepath[0] = 0;
	return reply("250 %s now %s", filepath, to);
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
	char err[ERRLEN];

	if(mountnet() < 0)
		return -1;

	if(!passive.inuse){
		fd = dial(data, "20", 0, 0);
		errstr(err);
	} else {
		alarm(5*60*1000);
		cfd = listen(passive.adir, ldir);
		alarm(0);
		errstr(err);
		if(cfd < 0)
			return -1;
		fd = accept(cfd, ldir);
		errstr(err);
		close(cfd);
	}
	if(fd < 0)
		logit("can't dial %s: %s", data, err);

	unmountnet();
	werrstr(err);
	return fd;
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
	int n, c;
	char *p, *sp, *path;
	char work[Maxpath];
	static char rpath[Maxpath];

	if(origpath == 0)
		*work = 0;
	else
		strncpy(work, origpath, sizeof(work));
	path = work;

	for(sp = special; *sp; sp++){
		p = strchr(path, *sp);
		if(p)
			*p = 0;
	}

	if(*path == '-')
		return path;

	if(*path == '/')
		rpath[0] = 0;
	else
		strcpy(rpath, curdir);
	n = strlen(rpath);

	while(*path && n < Maxpath-2){
		p = strchr(path, '/');
		if(p)
			*p++ = 0;
		if(strcmp(path, "..") == 0){
			while(n > 1){
				n--;
				c = rpath[n];
				rpath[n] = 0;
				if(c == '/')
					break;
			}
		} else if(strcmp(path, ".") == 0)
			;
		else if(n == 1)
			n += snprint(rpath+n, Maxpath - n, "%s", path);
		else
			n += snprint(rpath+n, Maxpath - n - 1, "/%s", path);
		if(p)
			path = p;
		else
			break;
	}

	return rpath;
}

void
getendpoint(char *dir, char *file, char **sysp, char **servp)
{
	int fd, n;
	char buf[Maxpath];
	char *sys, *serv;

	sys = serv = 0;

	snprint(buf, sizeof buf, "%s/%s", dir, file);
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv){
				*serv++ = 0;
				serv = strdup(serv);
			}
			sys = strdup(buf);
		}
		close(fd);
	}
	if(serv == 0)
		serv = strdup("unknown");
	if(sys == 0)
		sys = strdup("unknown");
	*servp = serv;
	*sysp = sys;
}

Endpoints *
getendpoints(char *dir)
{
	Endpoints *ep;

	ep = malloc(sizeof(*ep));
	getendpoint(dir, "local", &ep->lsys, &ep->lserv);
	getendpoint(dir, "remote", &ep->rsys, &ep->rserv);
	return ep;
}

void
freeendpoints(Endpoints *ep)
{
	free(ep->lsys);
	free(ep->rsys);
	free(ep->lserv);
	free(ep->rserv);
	free(ep);
}

int
postnote(int group, int pid, char *note)
{
	char file[128];
	int f, r;

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
