#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>

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
};

int	abortcmd(char*);
int	appendcmd(char*);
int	cdupcmd(char*);
int	cwdcmd(char*);
int	delcmd(char*);
int	helpcmd(char*);
int	listcmd(char*);
int	mkdircmd(char*);
int	modecmd(char*);
int	namelistcmd(char*);
int	nopcmd(char*);
int	passcmd(char*);
int	portcmd(char*);
int	pwdcmd(char*);
int	quitcmd(char*);
int	rnfrcmd(char*);
int	rntocmd(char*);
int	reply(char*, ...);
int	retrievecmd(char*);
int	storecmd(char*);
int	storeucmd(char*);
int	structcmd(char*);
int	systemcmd(char*);
int	typecmd(char*);
int	usercmd(char*);

typedef struct Cmd	Cmd;
struct Cmd
{
	char	*name;
	int	(*f)(char*);
};

Cmd cmdtab[] =
{
	{ "abor",	abortcmd, },
	{ "appe",	appendcmd, },
	{ "cdup",	cdupcmd, },
	{ "cwd",	cwdcmd, },
	{ "dele",	delcmd, },
	{ "help",	helpcmd, },
	{ "list",	listcmd, },
	{ "mkd",	mkdircmd, },
	{ "mode",	modecmd, },
	{ "nlst",	namelistcmd, },
	{ "noop",	nopcmd, },
	{ "pass",	passcmd, },
	{ "pwd",	pwdcmd, },
	{ "port", 	portcmd, },
	{ "quit",	quitcmd, },
	{ "retr",	retrievecmd, },
	{ "rnfr",	rnfrcmd, },
	{ "rnto",	rntocmd, },
	{ "rmd",	delcmd, },
	{ "stor", 	storecmd, },
	{ "stou", 	storeucmd, },
	{ "stru",	structcmd, },
	{ "syst",	systemcmd, },
	{ "type", 	typecmd, },
	{ "user",	usercmd, },
	{ 0, 0 },
};

char	user[NAMELEN];
int	challfd = -1;
int	loggedin;
int	type;			/* transmission type */
int	mode;			/* transmission mode */
int	structure;		/* file structure */
char	data[64];		/* data address */
int	pid;			/* transfer process */

int	nonone;
int 	ftpdir;

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
	case 'N':
		nonone = 1;
		break;
	case 'd':
		ftpdir = 1;
		break;
	}ARGEND
	Binit(&in, 0, OREAD);
	reply("220 Plan 9 FTP server ready");
	while(cmd = Brdline(&in, '\n')){
		/*
		 *  get the next command
		 */
		i = Blinelen(&in)-1;
		cmd[i] = 0;
		if(i > 0 && cmd[i-1] == '\r')
			cmd[i-1] = 0;

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
		 *  lookup the command and do it
		 */
		for(p = cmd; *p; p++)
			*p = tolower(*p);
		for(t = cmdtab; t->name; t++)
			if(strcmp(cmd, t->name) == 0){
				if((*t->f)(arg) < 0)
					return;
				break;
			}
		if(t->name == 0)
			reply("502 %s command not implemented", cmd);
	}
}

/*
 *  reply to a command
 */
int
reply(char *fmt, ...)
{
	char buf[8192], *s;

	s = buf;
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
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
 *  login as user
 */
int
login(char *user)
{
	newns(user, 0);
	putenv("service", "ftp");
	loggedin = 1;
	return reply("230 Logged in");
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
		pid = 0;
	}

	switch(pid = fork()){
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
 *  get a user id, reply with a challenge.  The users 'anonymous'
 *  and 'ftp' are equivalent to 'none'.  The user 'none' requires
 *  no challenge.
 */
int
usercmd(char *name)
{
	char chall[NETCHLEN];

	if(loggedin)
		return reply("530 Already logged in as %s", user);
	strncpy(user, name, sizeof(user));
	user[sizeof(user)-1] = 0;
	if(strcmp(user, "none") == 0
	|| strcmp(user, "anonymous") == 0
	|| strcmp(user, "ftp") == 0)
		return login("none");
	challfd = getchall(user, chall);
	if(challfd < 0)
		return reply("430 unknown user %s or authenticator sick", user);
	return reply("331 encrpyt challenge, %s, as a password", chall);
}

/*
 *  get a password, set up user if it works
 */
int
passcmd(char *response)
{
	if(challfd < 0)
		return reply("531 Send user id before encrypted challenge");
	if(challreply(challfd, user, response) < 0){
		challfd = -1;
		return reply("530 Not logged in");
	}
	challfd = -1;
	return login(user);
}

/*
 *  print working directory
 */
int
pwdcmd(char *arg)
{
	char dir[512];

	if(arg)
		return reply("550 Pwd takes no argument");
	if(getwd(dir, sizeof(dir)) == 0)
		return reply("550 Can't determine current directory: %r");
	return reply("257 \"%s\" is the current directory", dir);
}

/*
 *  chdir
 */
int
cwdcmd(char *dir)
{
	char path[512];

	if(!loggedin)
		return sodoff();
	if(chdir(dir) < 0)
		return reply("550 Cwd failed: %r");
	if(getwd(path, sizeof(path)) == 0)
		return reply("257 directory changed");
	return reply("257 directory changed to %s", path);
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
	return -1;
}

int
typecmd(char *arg)
{
	int c;

	if(arg == 0)
		return reply("501 Type command needs arguments");
	while(c = *arg++){
		switch(tolower(c)){
		case 'a':
			type = Tascii;
			break;
		case 'i':
			type = Timage;
			break;
		case 'n':
		case 't':
		case 'c':
			break;
		default:
			return reply("501 Unimplemented type %c", c);
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
	}
	return reply("200 Stream mode");
}

int
structcmd(char *arg)
{
	if(arg == 0)
		return reply("501 Struct command needs arguments");
	while(*arg){
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
	setfields(",");
	n = getfields(arg, field, 7);
	if(n != 6)
		return reply("501 Incorrect port specification");
	sprint(data, "tcp!%.3s.%.3s.%.3s.%.3s!%d", field[0], field[1], field[2],
		field[3], atoi(field[4])*256 + atoi(field[5]));
	return reply("220 Data port is %s", data);
}

/*
 *  list a directory 
 */
char*
mode2asc(int m)
{
	static char asc[12];
	char *p;

	strcpy(asc, "-----------");
	if(CHDIR & m)
		asc[0] = 'd';
	if(CHAPPEND & m)
		asc[0] = 'a';
	else if(CHEXCL & m)
		asc[1] = 'l';

	for(p = asc+2; p < asc + 11; p += 3, m<<=3){
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
listfile(int fd, Dir *d, int lflag)
{
	char ts[32];

	strcpy(ts, ctime(d->mtime));
	ts[16] = 0;
	if(lflag)
		fprint(fd, "%s %-8s %-8s %6d %s %s\r\n", mode2asc(d->mode),
			d->uid, d->gid, d->length, ts+4, d->name);
	else
		fprint(fd, "%s\r\n", d->name);
}
void
list(char *arg, int lflag)
{
	Dir db[32];
	int fd;
	int dfd;
	long i, n;

	if(arg == 0)
		arg = ".";

	fd = open(arg, OREAD);
	if(fd == -1){
		reply("550 Error opening %s: %r", arg);
		return;
	}
	if(dirfstat(fd, db) < 0){
		close(fd);
		reply("550 Can't stat %s: %r", arg);
		return;
	}

	dfd = dial(data, 0, 0, 0);
	if(dfd < 0){
		close(fd);
		reply("520 Error opening data connection:%r");
		return;
	}
	reply("150 Opened data connection (%s)", data);

	if((db[0].mode & CHDIR) == 0)
		listfile(dfd, db, lflag);
	else {
		while((n=dirread(fd, db, sizeof db)) > 0){
			n /= sizeof(Dir);
			for(i=0; i<n; i++)
				listfile(dfd, &db[i], lflag);
		}
	}

	reply("226 Transfer complete");
	close(dfd);
	close(fd);
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
retrieve(char *arg, int arg2)
{
	int dfd, fd, n, i;
	char buf[Nbuf];
	char *p, *ep;

	USED(arg2);
	fd = open(arg, OREAD);
	if(fd == -1){
		reply("550 Error opening %s: %r", arg);
		return;
	}

	n = read(fd, buf, sizeof(buf));
	if(n < 0){
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
	dfd = dial(data, 0, 0, 0);
	if(dfd < 0){
		close(fd);
		reply("520 Error opening data connection:%r");
		return;
	}

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
			reply("550 Error writing to data connection: %r");
			return;
		}
	} while((n = read(fd, buf, sizeof(buf))) > 0);

	close(fd);
	close(dfd);
	reply("226 Transfer complete");
}
int
retrievecmd(char *arg)
{
	if(arg == 0)
		return reply("501 Retrieve command requires an argument");
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
	dfd = dial(data, 0, 0, 0);
	if(dfd < 0){
		close(fd);
		reply("520 Error opening data connection:%r");
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
	reply("226 Transfer complete");

}
int
storecmd(char *arg)
{
	int fd;

	if(arg == 0)
		return reply("501 Store command requires an argument");

	fd = create(arg, OWRITE, 0660);
	if(fd == -1)
		return reply("550 Error creating %s: %r", arg);

	return asproc(store, arg, fd);
}
int
appendcmd(char *arg)
{
	int fd;

	if(arg == 0)
		return reply("501 Append command requires an argument");

	fd = open(arg, OWRITE);
	if(fd == -1){
		fd = create(arg, OWRITE, 0660);
		if(fd == -1)
			return reply("550 Error creating %s: %r", arg);
	}
	seek(fd, 0, 2);

	return asproc(store, arg, fd);
}
int
storeucmd(char *arg)
{
	int fd;
	char name[NAMELEN];

	USED(arg);
	strcpy(name, "ftpXXXXXXXXXXX");
	mktemp(name);
	fd = create(name, OWRITE, 0660);
	if(fd == -1)
		return reply("550 Error creating %s: %r", name);

	return asproc(store, name, fd);
}

int
mkdircmd(char *name)
{
	int fd;

	if(name == 0)
		return reply("501 Mkdir command requires an argument");
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
	int fd;
	char path[128];

	USED(arg);
	if(pid){
		sprint(path, "/proc/%d/note");
		fd = open(path, OWRITE);
		if(fd >= 0){
			if(write(fd, "die", 3) > 0)
				reply("426 Command aborted");
			close(fd);
		}
	}
	return reply("226 Abort processed");
}

int
systemcmd(char *arg)
{
	USED(arg);
	return reply("210 Plan 9");
}

int
helpcmd(char *arg)
{
	int i;

	USED(arg);
	print("214- the following commands are implemented:");
	for(i = 0; cmdtab[i].name; i++){
		if((i%8) == 0)
			print("\r\n");
		print("\t%s", cmdtab[i].name);
	}
	print("\r\n214 \r\n");
	return 0;
}

/*
 *  renaming a file takes two commands
 */
static char path[256];

int
rnfrcmd(char *from)
{
	if(from == 0)
		return reply("501 Rename command requires an argument");
	strncpy(path, from, sizeof(path));
	path[sizeof(path)-1] = 0;
	return reply("320 Rename %s to ...", path);
}
int
rntocmd(char *to)
{
	Dir d;
	char *fp;
	char *tp;

	if(to == 0)
		return reply("501 Rename command requires an argument");
	if(*path == 0)
		return reply("500 Rnto must be preceeded by an rnfr");

	tp = strrchr(to, '/');
	fp = strrchr(path, '/');
	if((tp && fp == 0) || (fp && tp == 0)
	|| (fp && tp && (fp-path != tp-to || memcmp(path, to, tp-to))))
		return reply("500 Rename can't change directory");
	if(tp)
		to = tp+1;

	if(dirstat(path, &d) < 0)
		return reply("520 Can't rename %s: %r\n", path);
	strncpy(d.name, to, sizeof(d.name));
	if(dirwstat(path, &d) < 0)
		return reply("520 Can't rename %s to %s: %r\n", path, to);

	path[0] = 0;
	return reply("220 %s now %s", path, to);
}
