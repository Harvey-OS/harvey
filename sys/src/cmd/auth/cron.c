#include <u.h>
#include <libc.h>
#include <bio.h>
#include <authsrv.h>
#include "authcmdlib.h"

char CRONLOG[] = "cron";

typedef struct Job	Job;
typedef struct Time	Time;
typedef struct User	User;

struct Time{			/* bit masks for each valid time */
	ulong	min;			/* actually 1 bit for every 2 min */
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
ulong	gettime(int, int);
int	gettok(int, int);
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

void
main(int argc, char *argv[])
{
	Job *j;
	Tm tm;
	Time t;
	ulong now, last, x;
	int i;

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
	USED(argc, argv);

	if(debug){
		readalljobs();
		printjobs();
		exits(0);
	}

	switch(fork()){
	case -1:
		error("can't fork");
	case 0:
		break;
	default:
		exits(0);
	}

	argv0 = "cron";
	srand(getpid()*time(0));
	last = time(0) / 60;
	for(;;){
		readalljobs();
		now = time(0) / 60;
		for(; last <= now; last += 2){
			tm = *localtime(last*60);
			t.min = 1 << tm.min/2;
			t.hour = 1 << tm.hour;
			t.wday = 1 << tm.wday;
			t.mday = 1 << tm.mday;
			t.mon = 1 << (tm.mon + 1);
			for(i = 0; i < nuser; i++)
				for(j = users[i].jobs; j; j = j->next)
					if(j->time.min & t.min && j->time.hour & t.hour
					&& j->time.wday & t.wday
					&& j->time.mday & t.mday
					&& j->time.mon & t.mon)
						rexec(&users[i], j);
		}
		x = time(0) / 60;
		if(x - now < 2)
			sleep((2 - (x - now))*60*1000);
	}
	exits(0);
}

void
createuser(void)
{
	Dir d;
	char file[128], *user;
	int fd;

	user = getuser();
	sprint(file, "/cron/%s", user);
	fd = create(file, OREAD, 0755|DMDIR);
	if(fd < 0){
		fprint(2, "couldn't create %s: %r\n", file);
		exits("create");
	}
	nulldir(&d);
	d.gid = user;
	dirfwstat(fd, &d);
	close(fd);
	sprint(file, "/cron/%s/cron", user);
	fd = create(file, OREAD, 0644);
	if(fd < 0){
		fprint(2, "couldn't create %s: %r\n", file);
		exits("create");
	}
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
		error("can't open /cron\n");
	while((n = dirread(fd, &d)) > 0){
		for(i = 0; i < n; i++){
			if(strcmp(d[i].name, "log") == 0)
				continue;
			if(strcmp(d[i].name, d[i].uid) != 0){
				syslog(1, CRONLOG, "cron for %s owned by %s\n", d[i].name, d[i].uid);
				continue;
			}
			u = newuser(d[i].name);
			sprint(file, "/cron/%s/cron", d[i].name);
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
			syslog(0, CRONLOG, "%s: line %d: line too long", user->name, line);
			continue;
		}
		j = emalloc(sizeof *j);
		if((j->time.min = gettime(0, 59))
		&& (j->time.hour = gettime(0, 23))
		&& (j->time.mday = gettime(1, 31))
		&& (j->time.mon = gettime(1, 12))
		&& (j->time.wday = gettime(0, 6))
		&& getname(&j->host)){
			j->cmd = emalloc(strlen(savec) + 1);
			strcpy(j->cmd, savec);
			j->next = jobs;
			jobs = j;
		}else{
			syslog(0, CRONLOG, "%s: line %d: syntax error", user->name, line);
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
		for(j = users[i].jobs; j; j = j->next){
			if(!mkcmd(j->cmd, buf, sizeof buf))
				print("\tbad job %s on host %s\n", j->cmd, j->host);
			else
				print("\tjob %s on host %s\n", buf, j->host);
		}
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
		syslog(0, CRONLOG, "internal error: strdup failure");
		_exits(0);
	}
	while(*savec == ' ' || *savec == '\t')
		savec++;
	return p > buf;
}

/*
 * return the next time range in the file:
 * times: '*'
 * 	| range
 * range: number
 *	| number '-' number
 *	| range ',' range
 * a return of zero means a syntax error was discovered
 */
ulong
gettime(int min, int max)
{
	ulong n, m, e;

	if(gettok(min, max) == '*')
		return ~0;
	n = 0;
	while(tok == '1'){
		m = 1 << lexval;
		n |= m;
		if(gettok(0, 0) == '-'){
			if(gettok(lexval, max) != '1')
				return 0;
			e = 1 << lexval;
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
		if(max > 32)
			lexval /= 2;			/* yuk: correct min by / 2 */
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
 * need to escape the quotes wo they don't get stripped
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
	char buf[8*1024], key[DESKEYLEN];
	int n, fd;

	switch(rfork(RFPROC|RFNOWAIT|RFNAMEG|RFENVG|RFFDG)){
	case 0:
		break;
	case -1:
		syslog(0, CRONLOG, "can't fork a job for %s: %r\n", user->name);
	default:
		return;
	}
	if(findkey(KEYDB, user->name, key) == 0){
		syslog(0, CRONLOG, "%s: key not found", user->name);
		_exits(0);
	}

	if(!mkcmd(j->cmd, buf, sizeof buf)){
		syslog(0, CRONLOG, "internal error: cmd buffer overflow");
		_exits(0);
	}

	/*
	 * remote call, auth, cmd with no i/o
	 * give it 2 min to complete
	 */
	alarm(2*60*1000);
	fd = call(j->host);
	if(fd < 0){
		if(fd == -2){
			syslog(0, AUTHLOG, "%s: dangerous host %s", user->name, j->host);
			syslog(0, CRONLOG, "%s: dangerous host %s", user->name, j->host);
		}
		syslog(0, CRONLOG, "%s: can't call %s: %r", user->name, j->host);
		_exits(0);
	}
syslog(0, CRONLOG, "%s: called %s on %s", user->name, j->cmd, j->host);
	if(myauth(fd, user->name) < 0){
		syslog(0, CRONLOG, "%s: can't auth %s on %s: %r", user->name, j->cmd, j->host);
		_exits(0);
	}
syslog(0, CRONLOG, "%s: authenticated %s on %s", user->name, j->cmd, j->host);
	write(fd, buf, strlen(buf)+1);
	write(fd, buf, 0);
	while((n = read(fd, buf, sizeof(buf)-1)) > 0){
		buf[n] = 0;
		syslog(0, CRONLOG, "%s: %s\n", j->cmd, buf);
	}
	_exits(0);
}

void *
emalloc(ulong n)
{
	void *p;

	if(p = mallocz(n, 1))
		return p;
	error("out of memory");
	return 0;
}

void *
erealloc(void *p, ulong n)
{
	if(p = realloc(p, n))
		return p;
	error("out of memory");
	return 0;
}

void
usage(void)
{
	fprint(2, "usage: cron [-c]\n");
	exits("usage");
}

int
myauth(int fd, char *user)
{
	int i;
	char hkey[DESKEYLEN];
	char buf[512];
	Ticketreq tr;
	Ticket t;
	Authenticator a;

	/* get ticket request from remote machine */
	if(readn(fd, buf, TICKREQLEN) < 0){
		werrstr("bad request");
		return -1;
	}
	convM2TR(buf, &tr);
	if(tr.type != AuthTreq){
		werrstr("bad request");
		return -1;
	}
	if(findkey(KEYDB, tr.authid, hkey) == 0){
		werrstr("no key for authid %s", tr.authid);
		return -1;
	}

	/* create ticket+authenticator and send to destination */
	memset(&t, 0, sizeof t);
	memmove(t.chal, tr.chal, CHALLEN);
	strncpy(t.cuid, user, sizeof(t.cuid));
	strncpy(t.suid, user, sizeof(t.suid));
	srand(time(0));
	for(i = 0; i < DESKEYLEN; i++)
		t.key[i] = nrand(256);
	t.num = AuthTs;
	convT2M(&t, buf, hkey);
	memmove(a.chal, tr.chal, CHALLEN);
	a.id = 0;
	a.num = AuthAc;
	convA2M(&a, buf+TICKETLEN, t.key);
	if(write(fd, buf, TICKETLEN+AUTHENTLEN) != TICKETLEN+AUTHENTLEN){
		werrstr("connection dropped: %r");
		return -1;
	}

	/* get authenticator from server and check */
	if(readn(fd, buf, AUTHENTLEN) < 0){
		werrstr("connection dropped: %r");
		return -1;
	}
	convM2A(buf, &a, t.key);
	if(a.num != AuthAs){
		werrstr("bad reply authenticator");
		return -1;
	}
	return 0;
}

int
qidcmp(Qid a, Qid b)
{
	/* might be useful to know if a > b, but not for cron */
	return(a.path != b.path || a.vers != b.vers);
}
