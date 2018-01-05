/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>

#define	NAMELEN	64	/* reasonable upper limit for name elements */

typedef struct Service	Service;
struct Service
{
	char	serv[NAMELEN];		/* name of the service */
	char	remote[3*NAMELEN];	/* address of remote system */
	char	prog[5*NAMELEN+1];	/* program to execute */
};

typedef struct Announce	Announce;
struct Announce
{
	Announce	*next;
	char	*a;
	int	announced;
	int	whined;
};

int	readstr(char*, char*, char*, int);
void	dolisten(char*, char*, int, char*, char*);
void	newcall(int, char*, char*, Service*);
int 	findserv(char*, char*, Service*, char*);
int	getserv(char*, char*, Service*);
void	error(char*);
void	scandir(char*, char*, char*);
void	becomenone(void);
void	listendir(char*, char*, int);

char	listenlog[] = "listen";

int	quiet;
int	immutable;
char	*cpu;
char	*proto;
Announce *announcements;
#define SEC 1000

char *namespace;

void
usage(void)
{
	error("usage: aux/listen [-q] [-n namespace] [-d servdir] [-t trustdir]"
		" [proto]");
}

/*
 * based on libthread's threadsetname, but drags in less library code.
 * actually just sets the arguments displayed.
 */
static void
procsetname(char *fmt, ...)
{
	int fd;
	char *cmdname;
	char buf[128];
	va_list arg;

	va_start(arg, fmt);
	cmdname = vsmprint(fmt, arg);
	va_end(arg);
	if (cmdname == nil)
		return;
	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, cmdname, strlen(cmdname)+1);
		close(fd);
	}
	free(cmdname);
}

void
main(int argc, char *argv[])
{
	Service *s;
	char *protodir;
	char *trustdir;
	char *servdir;

	servdir = 0;
	trustdir = 0;
	proto = "tcp";
	quiet = 0;
	immutable = 0;
	argv0 = argv[0];
	cpu = getenv("cputype");
	if(cpu == 0)
		error("can't get cputype");

	ARGBEGIN{
	case 'd':
		servdir = EARGF(usage());
		break;
	case 'q':
		quiet = 1;
		break;
	case 't':
		trustdir = EARGF(usage());
		break;
	case 'n':
		namespace = EARGF(usage());
		break;
	case 'i':
		/*
		 * fixed configuration, no periodic
		 * rescan of the service directory.
		 */
		immutable = 1;
		break;
	default:
		usage();
	}ARGEND;

	if(!servdir && !trustdir)
		servdir = "/bin/service";

	if(servdir && strlen(servdir) + NAMELEN >= sizeof(s->prog))
		error("service directory too long");
	if(trustdir && strlen(trustdir) + NAMELEN >= sizeof(s->prog))
		error("trusted service directory too long");

	switch(argc){
	case 1:
		proto = argv[0];
		break;
	case 0:
		break;
	default:
		usage();
	}

	syslog(0, listenlog, "started on %s", proto);

	protodir = proto;
	proto = strrchr(proto, '/');
	if(proto == 0)
		proto = protodir;
	else
		proto++;
	listendir(protodir, servdir, 0);
	listendir(protodir, trustdir, 1);

	/* command returns */
	exits(0);
}

static void
dingdong(void *v, char *msg)
{
	if(strstr(msg, "alarm") != nil)
		noted(NCONT);
	noted(NDFLT);
}

void
listendir(char *protodir, char *srvdir, int trusted)
{
	int ctl, pid, start;
	char dir[40], err[128];
	Announce *a;
	Waitmsg *wm;

	if (srvdir == 0)
		return;

	/*
 	 * insulate ourselves from later
	 * changing of console environment variables
	 * erase privileged crypt state
	 */
	switch(rfork(RFNOTEG|RFPROC|RFFDG|RFNOWAIT|RFENVG|RFNAMEG)) {
	case -1:
		error("fork");
	case 0:
		break;
	default:
		return;
	}

	procsetname("%s %s %s", protodir, srvdir, namespace);
	if (!trusted)
		becomenone();

	notify(dingdong);

	pid = getpid();
	scandir(proto, protodir, srvdir);
	for(;;){
		/*
		 * loop through announcements and process trusted services in
		 * invoker's ns and untrusted in none's.
		 */
		for(a = announcements; a; a = a->next){
			if(a->announced > 0)
				continue;

			sleep((pid*10)%200);

			/* a process per service */
			switch(pid = rfork(RFFDG|RFPROC)){
			case -1:
				syslog(1, listenlog, "couldn't fork for %s", a->a);
				break;
			case 0:
				for(;;){
					ctl = announce(a->a, dir);
					if(ctl < 0) {
						errstr(err, sizeof err);
						if (!a->whined)
							syslog(1, listenlog,
							   "giving up on %s: %r",
							a->a);
						if(strstr(err, "address in use")
						    != nil)
							exits("addr-in-use");
						else
							exits("ctl");
					}
					dolisten(proto, dir, ctl, srvdir, a->a);
					close(ctl);
				}
			default:
				a->announced = pid;
				break;
			}
		}

		/*
		 * if not running a fixed configuration,
		 * pick up any children that gave up and
		 * sleep for at least 60 seconds.
		 * If a service process dies in a fixed
		 * configuration what should be done -
		 * nothing? restart? restart after a delay?
		 * - currently just wait for something to
		 * die and delay at least 60 seconds
		 * between restarts.
		 */
		start = time(0);
		if(!immutable)
			alarm(60*1000);
		while((wm = wait()) != nil) {
			for(a = announcements; a; a = a->next)
				if(a->announced == wm->pid) {
					a->announced = 0;
					if (strstr(wm->msg, "addr-in-use") !=
					    nil)
						/* don't fill log file */
						a->whined = 1;
				}
			free(wm);
			if(immutable)
				break;
		}
		if(!immutable){
			alarm(0);
			scandir(proto, protodir, srvdir);
		}
		start = 60 - (time(0)-start);
		if(start > 0)
			sleep(start*1000);
	}
	/* not reached */
}

/*
 *  make a list of all services to announce for
 */
void
addannounce(char *str)
{
	Announce *a, **l;

	/* look for duplicate */
	l = &announcements;
	for(a = announcements; a; a = a->next){
		if(strcmp(str, a->a) == 0)
			return;
		l = &a->next;
	}

	/* accept it */
	a = mallocz(sizeof(*a) + strlen(str) + 1, 1);
	if(a == 0)
		return;
	a->a = ((char*)a)+sizeof(*a);
	strcpy(a->a, str);
	a->announced = 0;
	*l = a;
}

/*
 *  delete a service for announcement list
 */
void
delannounce(char *str)
{
	Announce *a, **l;

	/* look for service */
	l = &announcements;
	for(a = announcements; a; a = a->next){
		if(strcmp(str, a->a) == 0)
			break;
		l = &a->next;
	}
	if (a == nil)
		return;
	*l = a->next;			/* drop from the list */
	if (a->announced > 0)
		postnote(PNPROC, a->announced, "die");
	a->announced = 0;
	free(a);
}

static int
ignore(char *srvdir, char *name)
{
	int rv;
	char *file = smprint("%s/%s", srvdir, name);
	Dir *d = dirstat(file);

	rv = !d || d->length <= 0;	/* ignore unless it's non-empty */
	free(d);
	free(file);
	return rv;
}

void
scandir(char *proto, char *protodir, char *dname)
{
	int fd, i, n, nlen;
	char *nm;
	char ds[128];
	Dir *db;

	fd = open(dname, OREAD);
	if(fd < 0)
		return;

	nlen = strlen(proto);
	while((n=dirread(fd, &db)) > 0){
		for(i=0; i<n; i++){
			nm = db[i].name;
			if(!(db[i].qid.type&QTDIR) &&
			    strncmp(nm, proto, nlen) == 0) {
				snprint(ds, sizeof ds, "%s!*!%s", protodir,
					nm + nlen);
				if (ignore(dname, nm))
					delannounce(ds);
				else
					addannounce(ds);
			}
		}
		free(db);
	}

	close(fd);
}

void
becomenone(void)
{
	int fd;

	fd = open("#c/user", OWRITE);
	if(fd < 0 || write(fd, "none", strlen("none")) < 0)
		error("can't become none");
	close(fd);
	if(newns("none", namespace) < 0)
		error("can't build namespace");
}

void
dolisten(char *proto, char *dir, int ctl, char *srvdir,
	 char *dialstr)
{
	Service s;
	char ndir[40];
	int nctl, data;

	procsetname("%s %s", dir, dialstr);
	for(;;){
		/*
		 *  wait for a call (or an error)
		 */
		nctl = listen(dir, ndir);
		if(nctl < 0){
			if(!quiet)
				syslog(1, listenlog, "listen: %r");
			return;
		}

		/*
		 *  start a subprocess for the connection
		 */
		switch(rfork(RFFDG|RFPROC|RFNOWAIT|RFENVG|RFNAMEG|RFNOTEG)){
		case -1:
			reject(nctl, ndir, "host overloaded");
			close(nctl);
			continue;
		case 0:
			/*
			 *  see if we know the service requested
			 */
			memset(&s, 0, sizeof s);
			if(!findserv(proto, ndir, &s, srvdir)){
				if(!quiet)
					syslog(1, listenlog, "%s: unknown service '%s' from '%s': %r",
						proto, s.serv, s.remote);
				reject(nctl, ndir, "connection refused");
				exits(0);
			}
			data = accept(nctl, ndir);
			if(data < 0){
				syslog(1, listenlog, "can't open %s/data: %r", ndir);
				exits(0);
			}
			fprint(nctl, "keepalive");
			close(ctl);
			close(nctl);
			newcall(data, proto, ndir, &s);
			exits(0);
		default:
			close(nctl);
			break;
		}
	}
}

/*
 * look in the service directory for the service.
 * if the shell script or program is zero-length, ignore it,
 * thus providing a way to disable a service with a bind.
 */
int
findserv(char *proto, char *dir, Service *s, char *srvdir)
{
	int rv;
	Dir *d;

	if(!getserv(proto, dir, s))
		return 0;
	snprint(s->prog, sizeof s->prog, "%s/%s", srvdir, s->serv);
	d = dirstat(s->prog);
	rv = d && d->length > 0;	/* ignore unless it's non-empty */
	free(d);
	return rv;
}

/*
 *  get the service name out of the local address
 */
int
getserv(char *proto, char *dir, Service *s)
{
	char addr[128], *serv, *p;
	int32_t n;

	readstr(dir, "remote", s->remote, sizeof(s->remote)-1);
	if((p = utfrune(s->remote, L'\n')) != nil)
		*p = '\0';

	n = readstr(dir, "local", addr, sizeof(addr)-1);
	if(n <= 0)
		return 0;
	if((p = utfrune(addr, L'\n')) != nil)
		*p = '\0';
	serv = utfrune(addr, L'!');
	if(!serv)
		serv = "login";
	else
		serv++;

	/*
	 * disallow service names like
	 * ../../usr/user/bin/rc/su
	 */
	if(strlen(serv) +strlen(proto) >= NAMELEN || utfrune(serv, L'/') || *serv == '.')
		return 0;
	snprint(s->serv, sizeof s->serv, "%s%s", proto, serv);

	return 1;
}

char *
remoteaddr(char *dir)
{
	char buf[128], *p;
	int n, fd;

	snprint(buf, sizeof buf, "%s/remote", dir);
	fd = open(buf, OREAD);
	if(fd < 0)
		return strdup("");
	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n > 0){
		buf[n] = 0;
		p = strchr(buf, '!');
		if(p)
			*p = 0;
		return strdup(buf);
	}
	return strdup("");
}

void
newcall(int fd, char *proto, char *dir, Service *s)
{
	char data[4*NAMELEN];
	char *p;

	if(!quiet){
		if(dir != nil){
			p = remoteaddr(dir);
			syslog(0, listenlog, "%s call for %s on chan %s (%s)",
				proto, s->serv, dir, p);
			free(p);
		} else
			syslog(0, listenlog, "%s call for %s on chan %s",
				proto, s->serv, dir);
	}

	snprint(data, sizeof data, "%s/data", dir);
	bind(data, "/dev/cons", MREPL);
	dup(fd, 0);
	dup(fd, 1);
	dup(fd, 2);
	close(fd);

	/*
	 * close all the fds
	 */
	for(fd=3; fd<20; fd++)
		close(fd);
	execl(s->prog, s->prog, s->serv, proto, dir, nil);
	error(s->prog);
}

void
error(char *s)
{
	syslog(1, listenlog, "%s: %s: %r", proto, s);
	exits(0);
}

/*
 *  read a string from a device
 */
int
readstr(char *dir, char *info, char *s, int len)
{
	int n, fd;
	char buf[3*NAMELEN+4];

	snprint(buf, sizeof buf, "%s/%s", dir, info);
	fd = open(buf, OREAD);
	if(fd<0)
		return 0;

	n = read(fd, s, len-1);
	if(n<=0){
		close(fd);
		return -1;
	}
	s[n] = 0;
	close(fd);

	return n+1;
}
