#include <u.h>
#include <libc.h>
#include <auth.h>

int	eof;		/* send an eof if true */
int	crtonl;		/* convert all received \r to \n */
int	returns;	/* strip \r on reception */
char	*note = "die: yankee dog";
char	*ruser;
char *key;

void	rex(int, char*, char*);
void	tcpexec(int, char*, char*);
int	call(char *, char*, char*, char**);
char	*buildargs(char*[]);
int	send(int);
void	error(char*, char*);
void	sshexec(char*, char*);

void
usage(void)
{
	fprint(2, "usage: %s [-e] [-T] [-r] [-k keypattern] [-l user] net!host command...\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *host, *addr, *args;
	int fd;

	key = "";
	eof = 1;
	crtonl = 0;
	returns = 1;
	ARGBEGIN{
	case 'T':
		crtonl = 1;
		break;
	case 'r':
		returns = 0;
		break;
	case 'e':
		eof = 0;
		break;
	case 'k':
		key = EARGF(usage());
		break;
	case 'l':
		ruser = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc < 2)
		usage();
	host = argv[0];
	args = buildargs(&argv[1]);

	/* try erexexec p9any then dial again with p9sk2 */
	fd = call(0, host, "rexexec", &addr);
	if(fd >= 0)
		rex(fd, args, "p9any");
	close(fd);
	fd = call(0, host, "rexexec", &addr);
	if(fd >= 0)
		rex(fd, args, "p9sk2");
	close(fd);

	/* if there's an ssh port, try that */
	fd = call("tcp", host, "ssh", &addr);
	if(fd >= 0){
		close(fd);
		sshexec(host, args);
		/* falls through if no ssh */
	}

	/* specific attempts */
	fd = call("tcp", host, "shell", &addr);
	if(fd >= 0)
		tcpexec(fd, addr, args);

	error("can't dial", host);
	exits(0);
}

int
call(char *net, char *host, char *service, char **na)
{
	*na = netmkaddr(host, net, service);
	return dial(*na, 0, 0, 0);
}

void
rex(int fd, char *cmd, char *proto)
{
	char buf[4096];
	int kid, n;
	AuthInfo *ai;

	ai = auth_proxy(fd, auth_getkey, "proto=%s role=client %s", proto, key);
	if(ai == nil){
		if(strcmp(proto, "p9any") == 0)
			return;
		error("auth_proxy", nil);
	}
	write(fd, cmd, strlen(cmd)+1);

	kid = send(fd);
	while((n=read(fd, buf, sizeof buf))>0)
		if(write(1, buf, n)!=n)
			error("write error", 0);
	sleep(250);
	postnote(PNPROC, kid, note);/**/
	exits(0);
}

void
tcpexec(int fd, char *addr, char *cmd)
{
	char *cp, *ep, *u, *ru, buf[4096];
	int kid, n;

	/*
	 *  do the ucb authentication and send command
	 */
	u = getuser();
	ru = ruser;
	if(ru == nil)
		ru = u;
	if(write(fd, "", 1)<0 || write(fd, u, strlen(u)+1)<0
	|| write(fd, ru, strlen(ru)+1)<0 || write(fd, cmd, strlen(cmd)+1)<0){
		close(fd);
		error("can't authenticate to", addr);
	}

	/*
	 *  get authentication reply
	 */
	if(read(fd, buf, 1) != 1){
		close(fd);
		error("can't authenticate to", addr);
	}
	if(buf[0] != 0){
		while(read(fd, buf, 1) == 1){
			write(2, buf, 1);
			if(buf[0] == '\n')
				break;
		}
		close(fd);
		error("rejected by", addr);
	}

	kid = send(fd);
	while((n=read(fd, buf, sizeof buf))>0){
		if(crtonl) {
			/* convert cr's to nl's */
			for (cp = buf; cp < buf + n; cp++)
				if (*cp == '\r')
					*cp = '\n';
		}
		else if(!returns){
			/* convert cr's to null's */
			cp = buf;
			ep = buf + n;
			while(cp < ep && (cp = memchr(cp, '\r', ep-cp))){
				memmove(cp, cp+1, ep-cp-1);
				ep--;
				n--;
			}
		}
		if(write(1, buf, n)!=n)
			error("write error", 0);
	}
	sleep(250);
	postnote(PNPROC, kid, note);/**/
	exits(0);
}

void
sshexec(char *host, char *cmd)
{
	char *argv[10];
	int n;

	n = 0;
	argv[n++] = "ssh";
	argv[n++] = "-iCm";
	if(!returns)
		argv[n++] = "-r";
	if(ruser){
		argv[n++] = "-l";
		argv[n++] = ruser;
	}
	argv[n++] = host;
	argv[n++] = cmd;
	argv[n] = 0;
	exec("/bin/ssh", argv);
}

int
send(int fd)
{
	char buf[4096];
	int n;
	int kid;
	switch(kid = fork()){
	case -1:
		error("fork error", 0);
	case 0:
		break;
	default:
		return kid;
	}
	while((n=read(0, buf, sizeof buf))>0)
		if(write(fd, buf, n)!=n)
			exits("write error");
	if(eof)
		write(fd, buf, 0);

	exits(0);
	return 0;			/* to keep compiler happy */
}

void
error(char *s, char *z)
{
	if(z == 0)
		fprint(2, "%s: %s: %r\n", argv0, s);
	else
		fprint(2, "%s: %s %s: %r\n", argv0, s, z);
	exits(s);
}

char *
buildargs(char *argv[])
{
	char *args;
	int m, n;

	args = malloc(1);
	args[0] = '\0';
	n = 0;
	while(*argv){
		m = strlen(*argv) + 1;
		args = realloc(args, n+m +1);
		if(args == 0)
			error("malloc fail", 0);
		args[n] = ' ';	/* smashes old null */
		strcpy(args+n+1, *argv);
		n += m;
		argv++;
	}
	return args;
}
