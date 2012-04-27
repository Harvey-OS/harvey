#include <u.h>
#include <libc.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>

enum{ BufSize = 8192 };

char *remotesys, *logfile;
int debug, p[2];

void
death(void *, char *)
{
	int pid;

	close(0);
	close(1);
	close(p[1]);
	pid = getpid();
	postnote(PNGROUP, pid, "die");
	postnote(PNGROUP, pid, "die");
	postnote(PNGROUP, pid, "die");
	_exits(0);
}

static void
dump(int fd, uchar *buf, int n, char *label)
{
	Biobuf bout;
	int i;

	Binit(&bout, fd, OWRITE);
	Bprint(&bout, "%s<%d>: ", label, n);
	if(n > 64)
		n = 64;
	for(i = 0; i < n; i++)
		Bprint(&bout, "%2.2x ", buf[i]);
	Bprint(&bout, "\n");
	Bterm(&bout);
}

static void
xfer(int from, int to, int cfd, char *label)
{
	uchar buf[BufSize];
	int n;

	if(fork() == 0)
		return;

	close(cfd);
	for(;;){
		n = read(from, buf, sizeof(buf));
		if(n <= 0){
			fprint(2, "%s EOF\n", label);
			close(to);
			close(from);
			death(nil, nil);
		}
		dump(2, buf, n, label);
		n = write(to, buf, n);
		if(n < 0){
			fprint(2, "%s write err\n", label);
			close(to);
			close(from);
			death(nil, nil);
		}
	}
}

static int
dumper(int fd)
{
	int p[2];

	if(pipe(p) < 0)
		sysfatal("can't make pipe: %r");

	xfer(fd, p[0], p[1], "read");
	xfer(p[0], fd, p[1], "write");
	close(p[0]);
	return p[1];
}

static int
reporter(char *fmt, ...)
{
	va_list ap;
	char buf[2000];

	va_start(ap, fmt);
	if(logfile){
		vsnprint(buf, sizeof buf, fmt, ap);
		syslog(0, logfile, "%s tls reports %s", remotesys, buf);
	}else{
		fprint(2, "%s: %s tls reports ", argv0, remotesys);
		vfprint(2, fmt, ap);
		fprint(2, "\n");
	}
	va_end(ap);
	return 0;
}

void
usage(void)
{
	fprint(2, "usage: tlssrv -c cert [-D] [-l logfile] [-r remotesys] [cmd args...]\n");
	fprint(2, "  after  auth/secretpem key.pem > /mnt/factotum/ctl\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	TLSconn *conn;
	uchar buf[BufSize];
	char *cert;
	int n, fd, clearfd;

	debug = 0;
	remotesys = nil;
	cert = nil;
	logfile = nil;
	ARGBEGIN{
	case 'D':
		debug++;
		break;
	case 'c':
		cert = EARGF(usage());
		break;
	case 'l':
		logfile = EARGF(usage());
		break;
	case 'r':
		remotesys = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(cert == nil)
		sysfatal("no certificate specified");
	if(remotesys == nil)
		remotesys = "";
	conn = (TLSconn*)mallocz(sizeof *conn, 1);
	if(conn == nil)
		sysfatal("out of memory");
	conn->chain = readcertchain(cert);
	if (conn->chain == nil)
		sysfatal("can't read certificate");
	conn->cert = conn->chain->pem;
	conn->certlen = conn->chain->pemlen;
	conn->chain = conn->chain->next;
	if(debug)
		conn->trace = reporter;

	clearfd = 0;
	fd = 1;
	if(debug > 1)
		fd = dumper(fd);
	fd = tlsServer(fd, conn);
	if(fd < 0){
		reporter("failed: %r");
		exits(0);
	}
	reporter("open");

	if(argc > 0){
		if(pipe(p) < 0)
			exits("pipe");
		switch(fork()){
		case 0:
			close(fd);
			dup(p[0], 0);
			dup(p[0], 1);
			close(p[1]);
			close(p[0]);
			exec(argv[0], argv);
			reporter("can't exec %s: %r", argv[0]);
			_exits("exec");
		case -1:
			exits("fork");
		default:
			close(p[0]);
			clearfd = p[1];
			break;
		}
	}

	rfork(RFNOTEG);
	notify(death);
	switch(rfork(RFPROC)){
	case -1:
		sysfatal("can't fork");
	case 0:
		for(;;){
			n = read(clearfd, buf, BufSize);
			if(n <= 0)
				break;
			if(write(fd, buf, n) != n)
				break;
		}
		break;
	default:
		for(;;){
			n = read(fd, buf, BufSize);
			if(n <= 0)
				break;
			if(write(clearfd, buf, n) != n)
				break;
		}
		break;
	}
	death(nil, nil);
}
