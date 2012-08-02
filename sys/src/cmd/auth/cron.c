#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libsec.h>
#include <auth.h>
#include "authcmdlib.h"

char CRONLOG[] = "cron";

enum {
	Minute = 60,
	Hour = 60 * Minute,
	Day = 24 * Hour,
};

typedef struct Job	Job;
typedef struct Time	Time;
typedef struct User	User;

struct Time{			/* bit masks for each valid time */
	uvlong	min;
	ulong	hour;
	ulong	mday;
	ulong	wday;
	ulong	mon;
};

struct Job{
	char	*host;		/* where ... */
	Time	time;			/* when ... */
	char	*cmd;			/* and what to execute */
	Job	*next;
};

struct User{
	Qid	lastqid;			/* of last read /cron/user/cron */
	char	*name;			/* who ... */
	Job	*jobs;			/* wants to execute these jobs */
};

User	*users;
int	nuser;
int	maxuser;
char	*savec;
char	*savetok;
int	tok;
int	debug;
ulong	lexval;

void	rexec(User*, Job*);
void	readalljobs(void);
Job	*readjobs(char*, User*);
int	getname(char**);
uvlong	gettime(int, int);
int	gettok(int, int);
void	initcap(void);
void	pushtok(void);
void	usage(void);
void	freejobs(Job*);
User	*newuser(char*);
void	*emalloc(ulong);
void	*erealloc(void*, ulong);
int	myauth(int, char*);
void	createuser(void);
int	mkcmd(char*, char*, int);
void	printjobs(void);
int	qidcmp(Qid, Qid);
int	becomeuser(char*);

ulong
minute(ulong tm)
{
	return tm - tm%Minute;		/* round down to the minute */
}

int
sleepuntil(ulong tm)
{
	ulong now = time(0);
	
	if (now < tm)
		return sleep((tm - now)*1000);
	else
		return 0;
}

#pragma varargck	argpos clog 1
#pragma varargck	argpos fatal 1

static void
clog(char *fmt, ...)
{
	char msg[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(msg, msg + sizeof msg, fmt, arg);
	va_end(arg);
	syslog(0, CRONLOG, msg);
}

static void
fatal(char *fmt, ...)
{
	char msg[256];
	va_list arg;

	va_start(arg, fmt);
	vseprint(msg, msg + sizeof msg, fmt, arg);
	va_end(arg);
	clog("%s", msg);
	error("%s", msg);
}

static int
openlock(char *file)
{
	return create(file, ORDWR, 0600);
}

static int
mklock(char *file)
{
	int fd, try;
	Dir *dir;

	fd = openlock(file);
	if (fd >= 0) {
		/* make it a lock file if it wasn't */
		dir = dirfstat(fd);
		if (dir == nil)
			error("%s vanished: %r", file);
		dir->mode |= DMEXCL;
		dir->qid.type |= QTEXCL;
		dirfwstat(fd, dir);
		free(dir);

		/* reopen in case it wasn't a lock file at last open */
		close(fd);
	}
	for (try = 0; try < 65 && (fd = openlock(file)) < 0; try++)
		sleep(10*1000);
	return fd;
}

void
main(int argc, char *argv[])
{
	Job *j;
	Tm tm;
	Time t;
	ulong now, last;		/* in seconds */
	int i, lock;

	debug = 0;
	ARGBEGIN{
	case 'c':
		createuser();
		exits(0);
	case 'd':
		debug = 1;
		break;
	default:
		usage();
	}ARGEND

	if(debug){
		readalljobs();
		printjobs();
		exits(0);
	}

	initcap();		/* do this early, before cpurc removes it */

	switch(fork()){
	case -1:
		fatal("can't fork: %r");
	case 0:
		break;
	default:
		exits(0);
	}

	/*
	 * it can take a few minutes before the file server notices that
	 * we've rebooted and gives up the lock.
	 */
	lock = mklock("/cron/lock");
	if (lock < 0)
		fatal("cron already running: %r");

	argv0 = "cron";
	srand(getpid()*time(0));
	last = time(0);
	for(;;){
		readalljobs();
		/*
		 * the system's notion of time may have jumped forward or
		 * backward an arbitrary amount since the last call to time().
		 */
		now = time(0);
		/*
		 * if time has jumped backward, just note it and adapt.
		 * if time has jumped forward more than a day,
		 * just execute one day's jobs.
		 */
		if (now < last) {
			clog("time went backward");
			last = now;
		} else if (now - last > Day) {
			clog("time advanced more than a day");
			last = now - Day;
		}
		now = minute(now);
		for(last = minute(last); last <= now; last += Minute){
			tm = *localtime(last);
			t.min = 1ULL << tm.min;
			t.hour = 1 << tm.hour;
			t.wday = 1 << tm.wday;
			t.mday = 1 << tm.mday;
			t.mon =  1 << (tm.mon + 1);
			for(i = 0; i < nuser; i++)
				for(j = users[i].jobs; j; j = j->next)
					if(j->time.min & t.min
					&& j->time.hour & t.hour
					&& j->time.wday & t.wday
					&& j->time.mday & t.mday
					&& j->time.mon & t.mon)
						rexec(&users[i], j);
		}
		seek(lock, 0, 0);
		write(lock, "x", 1);	/* keep the lock alive */
		/*
		 * if we're not at next minute yet, sleep until a second past
		 * (to allow for sleep intervals being approximate),
		 * which synchronises with minute roll-over as a side-effect.
		 */
		sleepuntil(now + Minute + 1);
	}
	/* not reached */
}

void
createuser(void)
{
	Dir d;
	char file[128], *user;
	int fd;

	user = getuser();
	snprint(file, sizeof file, "/cron/%s", user);
	fd = create(file, OREAD, 0755|DMDIR);
	if(fd < 0)
		fatal("couldn't create %s: %r", file);
	nulldir(&d);
	d.gid = user;
	dirfwstat(fd, &d);
	close(fd);
	snprint(file, sizeof file, "/cron/%s/cron", user);
	fd = create(file, OREAD, 0644);
	if(fd < 0)
		fatal("couldn't create %s: %r", file);
	nulldir(&d);
	d.gid = user;
	dirfwstat(fd, &d);
	close(fd);
}

void
readalljobs(void)
{
	User *u;
	Dir *d, *du;
	char file[128];
	int i, n, fd;

	fd = open("/cron", OREAD);
	if(fd < 0)
		fatal("can't open /cron: %r");
	while((n = dirread(fd, &d)) > 0){
		for(i = 0; i < n; i++){
			if(strcmp(d[i].name, "log") == 0 ||
			    !(d[i].qid.type & QTDIR))
				continue;
			if(strcmp(d[i].name, d[i].uid) != 0){
				syslog(1, CRONLOG, "cron for %s owned by %s",
					d[i].name, d[i].uid);
				continue;
			}
			u = newuser(d[i].name);
			snprint(file, sizeof file, "/cron/%s/cron", d[i].name);
			du = dirstat(file);
			if(du == nil || qidcmp(u->lastqid, du->qid) != 0){
				freejobs(u->jobs);
				u->jobs = readjobs(file, u);
			}
			free(du);
		}
		free(d);
	}
	close(fd);
}

/*
 * parse user's cron file
 * other lines: minute hour monthday month weekday host command
 */
Job *
readjobs(char *file, User *user)
{
	Biobuf *b;
	Job *j, *jobs;
	Dir *d;
	int line;

	d = dirstat(file);
	if(!d)
		return nil;
	b = Bopen(file, OREAD);
	if(!b){
		free(d);
		return nil;
	}
	jobs = nil;
	user->lastqid = d->qid;
	free(d);
	for(line = 1; savec = Brdline(b, '\n'); line++){
		savec[Blinelen(b) - 1] = '\0';
		while(*savec == ' ' || *savec == '\t')
			savec++;
		if(*savec == '#' || *savec == '\0')
			continue;
		if(strlen(savec) > 1024){
			clog("%s: line %d: line too long", user->name, line);
			continue;
		}
		j = emalloc(sizeof *j);
		j->time.min = gettime(0, 59);
		if(j->time.min && (j->time.hour = gettime(0, 23))
		&& (j->time.mday = gettime(1, 31))
		&& (j->time.mon = gettime(1, 12))
		&& (j->time.wday = gettime(0, 6))
		&& getname(&j->host)){
			j->cmd = emalloc(strlen(savec) + 1);
			strcpy(j->cmd, savec);
			j->next = jobs;
			jobs = j;
		}else{
			clog("%s: line %d: syntax error", user->name, line);
			free(j);
		}
	}
	Bterm(b);
	return jobs;
}

void
printjobs(void)
{
	char buf[8*1024];
	Job *j;
	int i;

	for(i = 0; i < nuser; i++){
		print("user %s\n", users[i].name);
		for(j = users[i].jobs; j; j = j->next)
			if(!mkcmd(j->cmd, buf, sizeof buf))
				print("\tbad job %s on host %s\n",
					j->cmd, j->host);
			else
				print("\tjob %s on host %s\n", buf, j->host);
	}
}

User *
newuser(char *name)
{
	int i;

	for(i = 0; i < nuser; i++)
		if(strcmp(users[i].name, name) == 0)
			return &users[i];
	if(nuser == maxuser){
		maxuser += 32;
		users = erealloc(users, maxuser * sizeof *users);
	}
	memset(&users[nuser], 0, sizeof(users[nuser]));
	users[nuser].name = strdup(name);
	users[nuser].jobs = 0;
	users[nuser].lastqid.type = QTFILE;
	users[nuser].lastqid.path = ~0LL;
	users[nuser].lastqid.vers = ~0L;
	return &users[nuser++];
}

void
freejobs(Job *j)
{
	Job *next;

	for(; j; j = next){
		next = j->next;
		free(j->cmd);
		free(j->host);
		free(j);
	}
}

int
getname(char **namep)
{
	int c;
	char buf[64], *p;

	if(!savec)
		return 0;
	while(*savec == ' ' || *savec == '\t')
		savec++;
	for(p = buf; (c = *savec) && c != ' ' && c != '\t'; p++){
		if(p >= buf+sizeof buf -1)
			return 0;
		*p = *savec++;
	}
	*p = '\0';
	*namep = strdup(buf);
	if(*namep == 0){
		clog("internal error: strdup failure");
		_exits(0);
	}
	while(*savec == ' ' || *savec == '\t')
		savec++;
	return p > buf;
}

/*
 * return the next time range (as a bit vector) in the file:
 * times: '*'
 * 	| range
 * range: number
 *	| number '-' number
 *	| range ',' range
 * a return of zero means a syntax error was discovered
 */
uvlong
gettime(int min, int max)
{
	uvlong n, m, e;

	if(gettok(min, max) == '*')
		return ~0ULL;
	n = 0;
	while(tok == '1'){
		m = 1ULL << lexval;
		n |= m;
		if(gettok(0, 0) == '-'){
			if(gettok(lexval, max) != '1')
				return 0;
			e = 1ULL << lexval;
			for( ; m <= e; m <<= 1)
				n |= m;
			gettok(min, max);
		}
		if(tok != ',')
			break;
		if(gettok(min, max) != '1')
			return 0;
	}
	pushtok();
	return n;
}

void
pushtok(void)
{
	savec = savetok;
}

int
gettok(int min, int max)
{
	char c;

	savetok = savec;
	if(!savec)
		return tok = 0;
	while((c = *savec) == ' ' || c == '\t')
		savec++;
	switch(c){
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		lexval = strtoul(savec, &savec, 10);
		if(lexval < min || lexval > max)
			return tok = 0;
		return tok = '1';
	case '*': case '-': case ',':
		savec++;
		return tok = c;
	default:
		return tok = 0;
	}
}

int
call(char *host)
{
	char *na, *p;

	na = netmkaddr(host, 0, "rexexec");
	p = utfrune(na, L'!');
	if(!p)
		return -1;
	p = utfrune(p+1, L'!');
	if(!p)
		return -1;
	if(strcmp(p, "!rexexec") != 0)
		return -2;
	return dial(na, 0, 0, 0);
}

/*
 * convert command to run properly on the remote machine
 * need to escape the quotes so they don't get stripped
 */
int
mkcmd(char *cmd, char *buf, int len)
{
	char *p;
	int n, m;

	n = sizeof "exec rc -c '" -1;
	if(n >= len)
		return 0;
	strcpy(buf, "exec rc -c '");
	while(p = utfrune(cmd, L'\'')){
		p++;
		m = p - cmd;
		if(n + m + 1 >= len)
			return 0;
		strncpy(&buf[n], cmd, m);
		n += m;
		buf[n++] = '\'';
		cmd = p;
	}
	m = strlen(cmd);
	if(n + m + sizeof "'</dev/null>/dev/null>[2=1]" >= len)
		return 0;
	strcpy(&buf[n], cmd);
	strcpy(&buf[n+m], "'</dev/null>/dev/null>[2=1]");
	return 1;
}

void
rexec(User *user, Job *j)
{
	char buf[8*1024];
	int n, fd;
	AuthInfo *ai;

	switch(rfork(RFPROC|RFNOWAIT|RFNAMEG|RFENVG|RFFDG)){
	case 0:
		break;
	case -1:
		clog("can't fork a job for %s: %r\n", user->name);
	default:
		return;
	}

	if(!mkcmd(j->cmd, buf, sizeof buf)){
		clog("internal error: cmd buffer overflow");
		_exits(0);
	}

	/*
	 * local call, auth, cmd with no i/o
	 */
	if(strcmp(j->host, "local") == 0){
		if(becomeuser(user->name) < 0){
			clog("%s: can't change uid for %s on %s: %r",
				user->name, j->cmd, j->host);
			_exits(0);
		}
		putenv("service", "rx");
		clog("%s: ran '%s' on %s", user->name, j->cmd, j->host);
		execl("/bin/rc", "rc", "-lc", buf, nil);
		clog("%s: exec failed for %s on %s: %r",
			user->name, j->cmd, j->host);
		_exits(0);
	}

	/*
	 * remote call, auth, cmd with no i/o
	 * give it 2 min to complete
	 */
	alarm(2*Minute*1000);
	fd = call(j->host);
	if(fd < 0){
		if(fd == -2)
			clog("%s: dangerous host %s", user->name, j->host);
		clog("%s: can't call %s: %r", user->name, j->host);
		_exits(0);
	}
	clog("%s: called %s on %s", user->name, j->cmd, j->host);
	if(becomeuser(user->name) < 0){
		clog("%s: can't change uid for %s on %s: %r",
			user->name, j->cmd, j->host);
		_exits(0);
	}
	ai = auth_proxy(fd, nil, "proto=p9any role=client");
	if(ai == nil){
		clog("%s: can't authenticate for %s on %s: %r",
			user->name, j->cmd, j->host);
		_exits(0);
	}
	clog("%s: authenticated %s on %s", user->name, j->cmd, j->host);
	write(fd, buf, strlen(buf)+1);
	write(fd, buf, 0);
	while((n = read(fd, buf, sizeof(buf)-1)) > 0){
		buf[n] = 0;
		clog("%s: %s\n", j->cmd, buf);
	}
	_exits(0);
}

void *
emalloc(ulong n)
{
	void *p;

	if(p = mallocz(n, 1))
		return p;
	fatal("out of memory");
	return 0;
}

void *
erealloc(void *p, ulong n)
{
	if(p = realloc(p, n))
		return p;
	fatal("out of memory");
	return 0;
}

void
usage(void)
{
	fprint(2, "usage: cron [-c]\n");
	exits("usage");
}

int
qidcmp(Qid a, Qid b)
{
	/* might be useful to know if a > b, but not for cron */
	return(a.path != b.path || a.vers != b.vers);
}

void
memrandom(void *p, int n)
{
	uchar *cp;

	for(cp = (uchar*)p; n > 0; n--)
		*cp++ = fastrand();
}

/*
 *  keep caphash fd open since opens of it could be disabled
 */
static int caphashfd;

void
initcap(void)
{
	caphashfd = open("#¤/caphash", OCEXEC|OWRITE);
	if(caphashfd < 0)
		fprint(2, "%s: opening #¤/caphash: %r\n", argv0);
}

/*
 *  create a change uid capability 
 */
char*
mkcap(char *from, char *to)
{
	uchar rand[20];
	char *cap;
	char *key;
	int nfrom, nto, ncap;
	uchar hash[SHA1dlen];

	if(caphashfd < 0)
		return nil;

	/* create the capability */
	nto = strlen(to);
	nfrom = strlen(from);
	ncap = nfrom + 1 + nto + 1 + sizeof(rand)*3 + 1;
	cap = emalloc(ncap);
	snprint(cap, ncap, "%s@%s", from, to);
	memrandom(rand, sizeof(rand));
	key = cap+nfrom+1+nto+1;
	enc64(key, sizeof(rand)*3, rand, sizeof(rand));

	/* hash the capability */
	hmac_sha1((uchar*)cap, strlen(cap), (uchar*)key, strlen(key), hash, nil);

	/* give the kernel the hash */
	key[-1] = '@';
	if(write(caphashfd, hash, SHA1dlen) < 0){
		free(cap);
		return nil;
	}

	return cap;
}

int
usecap(char *cap)
{
	int fd, rv;

	fd = open("#¤/capuse", OWRITE);
	if(fd < 0)
		return -1;
	rv = write(fd, cap, strlen(cap));
	close(fd);
	return rv;
}

int
becomeuser(char *new)
{
	char *cap;
	int rv;

	cap = mkcap(getuser(), new);
	if(cap == nil)
		return -1;
	rv = usecap(cap);
	free(cap);

	newns(new, nil);
	return rv;
}
