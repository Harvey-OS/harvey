/*
 * ssh server - serve SSH protocol v2
 *	/net/ssh does most of the work; we copy bytes back and forth
 */
#include <u.h>
#include <libc.h>
#include <ip.h>
#include <auth.h>
#include "ssh2.h"

char *confine(char *, char *);
char *get_string(char *, char *);
void newchannel(int, char *, int);
void runcmd(int, int, char *, char *, char *, char *);

int errfd, toppid, sflag, tflag, prevent;
int debug;
char *idstring;
char *netdir;				/* /net/ssh/<conn> */
char *nsfile = nil;
char *restdir;
char *shell;
char *srvpt;
char *uname;

void
usage(void)
{
	fprint(2, "usage: %s [-i id] [-s shell] [-r dir] [-R dir] [-S srvpt] "
		"[-n ns] [-t] [netdir]\n", argv0);
	exits("usage");
}

static int
getctlfd(void)
{
	int ctlfd;
	char *name;

	name = smprint("%s/clone", netdir);
	ctlfd = -1;
	if (name)
		ctlfd = open(name, ORDWR);
	if (ctlfd < 0) {
		syslog(0, "ssh", "server can't clone: %s: %r", name);
		exits("open clone");
	}
	free(name);
	return ctlfd;
}

static int
getdatafd(int ctlfd)
{
	int fd;
	char *name;

	name = smprint("%s/data", netdir);
	fd = -1;
	if (name)
		fd = open(name, OREAD);
	if (fd < 0) {
		syslog(0, "ssh", "can't open %s: %r", name);
		hangup(ctlfd);
		exits("open data");
	}
	free(name);
	return fd;
}

static void
auth(char *buf, int n, int ctlfd)
{
	int fd;

	fd = open("#¤/capuse", OWRITE);
	if (fd < 0) {
		syslog(0, "ssh", "server can't open capuse: %r");
		hangup(ctlfd);
		exits("capuse");
	}
	if (write(fd, buf, n) != n) {
		syslog(0, "ssh", "server write `%.*s' to capuse failed: %r",
			n, buf);
		hangup(ctlfd);
		exits("capuse");
	}
	close(fd);
}

/*
 * mount tunnel if there isn't one visible.
 */
static void
mounttunnel(int ctlfd)
{
	int fd;
	char *p, *np, *q;

	if (access(netdir, AEXIST) >= 0)
		return;

	p = smprint("/srv/%s", srvpt? srvpt: "ssh");
	np = strdup(netdir);
	if (p == nil || np == nil)
		sysfatal("out of memory");
	q = strstr(np, "/ssh");
	if (q != nil)
		*q = '\0';
	fd = open(p, ORDWR);
	if (fd < 0) {
		syslog(0, "ssh", "can't open %s: %r", p);
		hangup(ctlfd);
		exits("open");
	}
	if (mount(fd, -1, np, MBEFORE, "") < 0) {
		syslog(0, "ssh", "can't mount %s in %s: %r", p, np);
		hangup(ctlfd);
		exits("can't mount");
	}
	free(p);
	free(np);
}

static int
authnewns(int ctlfd, char *buf, int size, int n)
{
	char *p, *q;

	USED(size);
	if (n <= 0)
		return 0;
	buf[n] = '\0';
	if (strcmp(buf, "n/a") == 0)
		return 0;

	auth(buf, n, ctlfd);

	p = strchr(buf, '@');
	if (p == nil)
		return 0;
	++p;
	q = strchr(p, '@');
	if (q) {
		*q = '\0';
		uname = strdup(p);
	}
	if (!tflag && newns(p, nsfile) < 0) {
		syslog(0, "ssh", "server: newns(%s,%s) failed: %r", p, nsfile);
		return -1;
	}
	return 0;
}

static void
listenloop(char *listfile, int ctlfd, char *buf, int size)
{
	int fd, n;

	while ((fd = open(listfile, ORDWR)) >= 0) {
		n = read(fd, buf, size - 1);
		fprint(errfd, "read from listen file returned %d\n", n);
		if (n <= 0) {
			syslog(0, "ssh", "read on listen failed: %r");
			break;
		}
		buf[n >= 0? n: 0] = '\0';
		fprint(errfd, "read %s\n", buf);

		switch (fork()) {
		case 0:					/* child */
			close(ctlfd);
			newchannel(fd, netdir, atoi(buf));  /* never returns */
		case -1:
			syslog(0, "ssh", "fork failed: %r");
			hangup(ctlfd);
			exits("fork");
		}
		close(fd);
	}
	if (fd < 0)
		syslog(0, "ssh", "listen failed: %r");
}

void
main(int argc, char *argv[])
{
	char *listfile;
	int ctlfd, fd, n;
	char buf[Arbpathlen], path[Arbpathlen];

	rfork(RFNOTEG);
	toppid = getpid();
	shell = "/bin/rc -il";
	ARGBEGIN {
	case 'd':
		debug++;
		break;
	case 'i':
		idstring = EARGF(usage());
		break;
	case 'n':
		nsfile = EARGF(usage());
		break;
	case 'R':
		prevent = 1;
		/* fall through */
	case 'r':
		restdir = EARGF(usage());
		break;
	case 's':
		sflag = 1;
		shell = EARGF(usage());
		break;
	case 'S':
		srvpt = EARGF(usage());
		break;
	case 't':
		tflag = 1;
		break;
	default:
		usage();
		break;
	} ARGEND;

	errfd = -1;
	if (debug)
		errfd = 2;

	/* work out network connection's directory */
	if (argc >= 1)
		netdir = argv[0];
	else				/* invoked by listen1 */
		netdir = getenv("net");
	if (netdir == nil) {
		syslog(0, "ssh", "server netdir is nil");
		exits("nil netdir");
	}
	syslog(0, "ssh", "server netdir is %s", netdir);

	uname = getenv("user");
	if (uname == nil)
		uname = "none";

	/* extract dfd and cfd from netdir */
	ctlfd = getctlfd();
	fd = getdatafd(ctlfd);

	n = read(fd, buf, sizeof buf - 1);
	if (n < 0) {
		syslog(0, "ssh", "server read error for data file: %r");
		hangup(ctlfd);
		exits("read cap");
	}
	close(fd);
	authnewns(ctlfd, buf, sizeof buf, n);

	/* get connection number in buf */
	n = read(ctlfd, buf, sizeof buf - 1);
	buf[n >= 0? n: 0] = '\0';

	/* tell netssh our id string */
	fd2path(ctlfd, path, sizeof path);
	if (0 && idstring) {			/* was for coexistence */
		syslog(0, "ssh", "server conn %s, writing \"id %s\" to %s",
			buf, idstring, path);
		fprint(ctlfd, "id %s", idstring);
	}

	/* announce */
	fprint(ctlfd, "announce session");

	/* construct listen file name */
	listfile = smprint("%s/%s/listen", netdir, buf);
	if (listfile == nil) {
		syslog(0, "ssh", "out of memory");
		exits("out of memory");
	}
	syslog(0, "ssh", "server listen is %s", listfile);

	mounttunnel(ctlfd);
	listenloop(listfile, ctlfd, buf, sizeof buf);
	hangup(ctlfd);
	exits(nil);
}

/* an abbreviation.  note the assumed variables. */
#define REPLY(s)	if (want_reply) fprint(reqfd, s);

static void
forkshell(char *cmd, int reqfd, int datafd, int want_reply)
{
	switch (fork()) {
	case 0:
		if (sflag)
			snprint(cmd, sizeof cmd, "-s%s", shell);
		else
			cmd[0] = '\0';
		USED(cmd);
		syslog(0, "ssh", "server starting ssh shell for %s", uname);
		/* runcmd doesn't return */
		runcmd(reqfd, datafd, "con", "/bin/ip/telnetd", "-nt", nil);
	case -1:
		REPLY("failure");
		syslog(0, "ssh", "server can't fork: %r");
		exits("fork");
	}
}

static void
forkcmd(char *cmd, char *p, int reqfd, int datafd, int want_reply)
{
	char *q;

	switch (fork()) {
	case 0:
		if (restdir && chdir(restdir) < 0) {
			syslog(0, "ssh", "can't chdir(%s): %r", restdir);
			exits("can't chdir");
		}
		if (!prevent || (q = getenv("sshsession")) &&
		    strcmp(q, "allow") == 0)
			get_string(p+1, cmd);
		else
			confine(p+1, cmd);
		syslog(0, "ssh", "server running `%s' for %s", cmd, uname);
		/* runcmd doesn't return */
		runcmd(reqfd, datafd, "rx", "/bin/rc", "-lc", cmd);
	case -1:
		REPLY("failure");
		syslog(0, "ssh", "server can't fork: %r");
		exits("fork");
	}
}

void
newchannel(int fd, char *conndir, int channum)
{
	char *p, *reqfile, *datafile;
	int n, reqfd, datafd, want_reply, already_done;
	char buf[Maxpayload], cmd[Bigbufsz];

	close(fd);

	already_done = 0;
	reqfile = smprint("%s/%d/request", conndir, channum);
	if (reqfile == nil)
		sysfatal("out of memory");
	reqfd = open(reqfile, ORDWR);
	if (reqfd < 0) {
		syslog(0, "ssh", "can't open request file %s: %r", reqfile);
		exits("net");
	}
	datafile = smprint("%s/%d/data", conndir, channum);
	if (datafile == nil)
		sysfatal("out of memory");
	datafd = open(datafile, ORDWR);
	if (datafd < 0) {
		syslog(0, "ssh", "can't open data file %s: %r", datafile);
		exits("net");
	}
	while ((n = read(reqfd, buf, sizeof buf - 1)) > 0) {
		fprint(errfd, "read from request file returned %d\n", n);
		for (p = buf; p < buf + n && *p != ' '; ++p)
			;
		*p++ = '\0';
		want_reply = (*p == 't');
		/* shell, exec, and various flavours of failure */
		if (strcmp(buf, "shell") == 0) {
			if (already_done) {
				REPLY("failure");
				continue;
			}
			forkshell(cmd, reqfd, datafd, want_reply);
			already_done = 1;
			REPLY("success");
		} else if (strcmp(buf, "exec") == 0) {
			if (already_done) {
				REPLY("failure");
				continue;
			}
			forkcmd(cmd, p, reqfd, datafd, want_reply);
			already_done = 1;
			REPLY("success");
		} else if (strcmp(buf, "pty-req") == 0 ||
		    strcmp(buf, "window-change") == 0) {
			REPLY("success");
		} else if (strcmp(buf, "x11-req") == 0 ||
		    strcmp(buf, "env") == 0 || strcmp(buf, "subsystem") == 0) {
			REPLY("failure");
		} else if (strcmp(buf, "xon-xoff") == 0 ||
		    strcmp(buf, "signal") == 0 ||
		    strcmp(buf, "exit-status") == 0 ||
		    strcmp(buf, "exit-signal") == 0) {
			;
		} else
			syslog(0, "ssh", "server unknown channel request: %s",
				buf);
	}
	if (n < 0)
		syslog(0, "ssh", "server read failed: %r");
	exits(nil);
}

char *
get_string(char *q, char *s)
{
	int n;

	n = nhgetl(q);
	q += 4;
	memmove(s, q, n);
	s[n] = '\0';
	q += n;
	return q;
}

char *
confine(char *q, char *s)
{
	int i, n, m;
	char *p, *e, *r, *buf, *toks[Maxtoks];

	n = nhgetl(q);
	q += 4;
	buf = malloc(n+1);
	if (buf == nil)
		return nil;
	memmove(buf, q, n);
	buf[n]  = 0;
	m = tokenize(buf, toks, nelem(toks));
	e = s + n + 1;
	for (i = 0, r = s; i < m; ++i) {
		p = strrchr(toks[i], '/');
		if (p == nil)
			r = seprint(r, e, "%s ", toks[i]);
		else if (p[0] != '\0' && p[1] != '\0')
			r = seprint(r, e, "%s ", p+1);
		else
			r = seprint(r, e, ". ");
	}
	free(buf);
	q += n;
	return q;
}

void
runcmd(int reqfd, int datafd, char *svc, char *cmd, char *arg1, char *arg2)
{
	char *p;
	int fd, cmdpid, child;

	cmdpid = rfork(RFPROC|RFMEM|RFNOTEG|RFFDG|RFENVG);
	switch (cmdpid) {
	case -1:
		syslog(0, "ssh", "fork failed: %r");
		exits("fork");
	case 0:
		if (restdir == nil) {
			p = smprint("/usr/%s", uname);
			if (p && access(p, AREAD) == 0 && chdir(p) < 0) {
				syslog(0, "ssh", "can't chdir(%s): %r", p);
				exits("can't chdir");
			}
			free(p);
		}
		p = strrchr(cmd, '/');
		if (p)
			++p;
		else
			p = cmd;

		dup(datafd, 0);
		dup(datafd, 1);
		dup(datafd, 2);
		close(datafd);
		putenv("service", svc);
		fprint(errfd, "starting %s\n", cmd);
		execl(cmd, p, arg1, arg2, nil);

		syslog(0, "ssh", "cannot exec %s: %r", cmd);
		exits("exec");
	default:
		close(datafd);
		fprint(errfd, "waiting for child %d\n", cmdpid);
		while ((child = waitpid()) != cmdpid && child != -1)
			fprint(errfd, "child %d passed\n", child);
		if (child == -1)
			fprint(errfd, "wait failed: %r\n");

		syslog(0, "ssh", "server closing ssh session for %s", uname);
		fprint(errfd, "closing connection\n");
		fprint(reqfd, "close");
		p = smprint("/proc/%d/notepg", toppid);
		if (p) {
			fd = open(p, OWRITE);
			fprint(fd, "interrupt");
			close(fd);
		}
		exits(nil);
	}
}
