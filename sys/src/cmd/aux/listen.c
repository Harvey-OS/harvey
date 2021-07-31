#include <u.h>
#include <libc.h>
#include <auth.h>

typedef struct Service	Service;
struct Service
{
	char	serv[NAMELEN];		/* name of the service */
	char	remote[3*NAMELEN];	/* address of remote system */
	char	prog[5*NAMELEN+1];	/* program to execute */
	char	trusted;		/* true if service in trusted dir */
};

int	readstr(char*, char*, char*, int);
void	netchown(int, char*);
void	dolisten(char*, char*, int);
void	newcall(int, char*, char*, Service*);
int 	findserv(char*, char*, Service*);
int	getserv(char*, char*, Service*);
void	error(char*);

char	listenlog[] = "listen";

int	quiet;
char	*cpu;
char	*net;
char	*trustdir;
char	*servdir;
#define SEC 1000

void
main(int argc, char *argv[])
{
	Service *s;
	int ctl, try;
	char *name;
	char aname[256];
	char dir[40];

	/*
 	 * insulate ourselves from later
	 * changing of console environment variables
	 * erase privileged crypt state
	 */
	if(rfork(RFENVG|RFNAMEG|RFNOTEG) < 0)
		error("can't make new pgrp");

	cpu = getenv("cputype");
	if(cpu == 0)
		error("can't get cputype");
	argv0 = argv[0];
	net = "dk";
	name = getenv("sysname");
	quiet = 0;
	ARGBEGIN{
	case 'd':
		servdir = ARGF();
		break;
	case 'q':
		quiet = 1;
		break;
	case 't':
		trustdir = ARGF();
		break;
	default:
		error("usage: listen [-d servdir] [-t trustdir] [net [name]]");
	}ARGEND
	if(!servdir && !trustdir)
		servdir = "/bin/service";

	if(servdir && strlen(servdir) + NAMELEN >= sizeof(s->prog))
		error("service directory too long");
	if(trustdir && strlen(trustdir) + NAMELEN >= sizeof(s->prog))
		error("trusted service directory too long");

	switch(argc){
	case 2:
		name = argv[1];
		/* fall through */
	case 1:
		net = argv[0];
		break;
	case 0:
		break;
	default:
		error("usage: listen [-d servdir] [net [name]]");
	}

	if(strcmp(net, "tcp")==0 || strcmp(net, "udp")==0 || strcmp(net, "il")==0){
		sprint(aname, "%s!*!*", net);
	} else {
		if(!name)
			error("can't announce without name");
		sprint(aname, "%s!%s", net, name);
	}

	switch(fork()){
	case 0:
		break;
	case -1:
		error("can't fork");
	default:
		exits(0);
	}

	ctl = -1;
	for(;;){
		for(try = 0; try < 5; try++){
			if(try || strcmp(net, "dk") == 0)
				sleep(10*SEC);
			ctl = announce(aname, dir);
			if(ctl >= 0)
				break;
			syslog(1, listenlog, "%s: can't announce as %s: %r", net, aname);
		}
		if(ctl < 0) {
			syslog(1, listenlog, "%s: giving up", net);
			exits("ctl");
		}
		if(!quiet)
			syslog(1, listenlog, "%s: announced as %s on %s", net, aname, dir);

		dolisten(net, dir, ctl);
		syslog(1, listenlog, "%s: hung up on %s", net, aname);
		close(ctl);
		syslog(1, listenlog, "%s: restarting %s", net, aname);
	}
}

void
becomenone(void)
{
	int fd;

	fd = open("#c/user", OWRITE);
	if(fd < 0 || write(fd, "none", strlen("none")) < 0)
		error("can't become none");
	close(fd);
	fd = open("#c/key", OWRITE);
	if(fd >= 0){
		write(fd, "1234567", 7);
		close(fd);
	}
	if(newns("none", 0))
		error("can't build namespace");
}

void
dolisten(char *net, char *dir, int ctl)
{
	Service s;
	char ndir[40];
	int nctl, data;

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
		 *  start a process for the service
		 */
		switch(rfork(RFFDG|RFPROC|RFNOWAIT|RFENVG|RFNAMEG|RFNOTEG)){
		case -1:
			reject(ctl, ndir, "host overloaded");
			close(nctl);
			continue;
		case 0:
			/*
			 *  see if we know the service requested
			 */
			memset(&s, 0, sizeof s);
			if(!findserv(net, ndir, &s)){
				if(!quiet)
					syslog(1, listenlog, "%s: unknown service '%s' from '%s': %r",
						net, s.serv, s.remote);
				reject(ctl, ndir, "connection refused");
				exits(0);
			}
			data = accept(ctl, ndir);
			if(data < 0){
				syslog(1, listenlog, "can't open %s/data: %r", ndir);
				exits(0);
			}
			close(ctl);
			close(nctl);
			newcall(data, net, ndir, &s);
			exits(0);
		default:
			close(nctl);
			break;
		}
	}
}

/*
 * look in the two service dirctories for the service
 */
int 
findserv(char *net, char *dir, Service *s)
{
	char dbuf[DIRLEN];

	if(!getserv(net, dir, s))
		return 0;
	if(servdir){
		s->trusted = 0;
		sprint(s->prog, "%s/%s", servdir, s->serv);
		if(stat(s->prog, dbuf) >= 0)
			return 1;
	}
	if(trustdir){
		s->trusted = 1;
		sprint(s->prog, "%s/%s", trustdir, s->serv);
		if(stat(s->prog, dbuf) >= 0)
			return 1;
	}
	return 0;
}

/*
 *  get the service name out of the local address
 */
int
getserv(char *net, char *dir, Service *s)
{
	char addr[128], *serv, *p;
	long n;

	readstr(dir, "remote", s->remote, sizeof(s->remote)-1);
	if(p = utfrune(s->remote, L'\n'))
		*p = '\0';

	n = readstr(dir, "local", addr, sizeof(addr)-1);
	if(n <= 0)
		return 0;
	if(p = utfrune(addr, L'\n'))
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
	if(strlen(serv) +strlen(net) >= NAMELEN || utfrune(serv, L'/') || *serv == '.')
		return 0;
	sprint(s->serv, "%s%s", net, serv);

	return 1;
}

void
newcall(int fd, char *net, char *dir, Service *s)
{
	char data[4*NAMELEN];

	if(!quiet)
		syslog(0, listenlog, "%s call for %s on chan %s", net, s->serv, dir);

	sprint(data, "%s/data", dir);
	bind(data, "/dev/cons", MREPL);
	dup(fd, 0);
	dup(fd, 1);
	dup(fd, 2);
	close(fd);

	/*
	 * close the stupid syslog fds
	 */
	for(fd=3; fd<20; fd++)
		close(fd);
	if(!s->trusted)
		becomenone();
	execl(s->prog, s->prog, s->serv, net, dir, 0);
	error(s->prog);
}

void
error(char *s)
{
	syslog(1, listenlog, "%s: %s: %r", net, s);
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

	sprint(buf, "%.*s/%.*s", 2*NAMELEN, dir, NAMELEN, info);
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

void
netchown(int fd, char *user)
{
	Dir d;

	if(dirfstat(fd, &d) < 0)
		return;
	strncpy(d.uid, user, NAMELEN);
	strncpy(d.gid, user, NAMELEN);
	d.mode = 0600;
	dirfwstat(fd, &d);
}
