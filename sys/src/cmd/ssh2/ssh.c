/*
 * ssh - remote login via SSH v2
 *	/net/ssh does most of the work; we copy bytes back and forth
 */
#include <u.h>
#include <libc.h>
#include <auth.h>
#include "ssh2.h"

int doauth(int, char *);
int isatty(int);

char *user, *remote;
char *netdir = "/net";
int debug = 0;

static int stripcr = 0;
static int mflag = 0;
static int iflag = -1;
static int nopw = 0, nopka = 0;
static int chpid;
static int reqfd, dfd1, cfd1, dfd2, cfd2, consfd, kconsfd, cctlfd, notefd, keyfd;

void
usage(void)
{
	fprint(2, "usage: %s [-dkKmr] [-l user] [-n dir] [-z attr=val] addr "
		"[cmd [args]]\n", argv0);
	exits("usage");
}

/*
 * this is probably overkill except writing "kill" to notefd;
 * file descriptors are closed by the kernel upon exit.
 */
static void
shutdown(void)
{
	if (cctlfd > 0) {
		fprint(cctlfd, "rawoff");
		close(cctlfd);
	}
	if (consfd > 0)
		close(consfd);
	if (reqfd > 0) {
		fprint(reqfd, "close");
		close(reqfd);
	}
	close(dfd2);
	close(dfd1);
	close(cfd2);
	close(cfd1);

	fprint(notefd, "kill");
	close(notefd);
}

static void
bail(char *sts)
{
	shutdown();
	exits(sts);
}

int
handler(void *, char *s)
{
	char *nf;
	int fd;

	if (strstr(s, "alarm") != nil)
		return 0;
	if (chpid) {
		nf = esmprint("/proc/%d/note", chpid);
		fd = open(nf, OWRITE);
		fprint(fd, "interrupt");
		close(fd);
		free(nf);
	}
	shutdown();
	return 1;
}

static void
parseargs(void)
{
	int n;
	char *p, *q;

	q = strchr(remote, '@');
	if (q != nil) {
		user = remote;
		*q++ = 0;
		remote = q;
	}

	q = strchr(remote, '!');
	if (q) {
		n = q - remote;
		netdir = malloc(n+1);
		if (netdir == nil)
			sysfatal("out of memory");
		strncpy(netdir, remote, n+1);
		netdir[n] = '\0';

		p = strrchr(netdir, '/');
		if (p == nil) {
			free(netdir);
			netdir = "/net";
		} else if (strcmp(p+1, "ssh") == 0)
			*p = '\0';
		else
			remote = esmprint("%s/ssh", netdir);
	}

}

static int
catcher(void *, char *s)
{
	return strstr(s, "alarm") != nil;
}

static int
timedmount(int fd, int afd, char *mntpt, int flag, char *aname)
{
	int oalarm, ret;

	atnotify(catcher, 1);
	oalarm = alarm(5*1000);		/* don't get stuck here */
	ret = mount(fd, afd, mntpt, flag, aname);
	alarm(oalarm);
	atnotify(catcher, 0);
	return ret;
}

static void
mounttunnel(char *srv)
{
	int fd;

	if (debug)
		fprint(2, "%s: mounting %s on /net\n", argv0, srv);
	fd = open(srv, OREAD);
	if (fd < 0) {
		if (debug)
			fprint(2, "%s: can't open %s: %r\n", argv0, srv);
	} else if (timedmount(fd, -1, netdir, MBEFORE, "") < 0) {
		fprint(2, "can't mount %s on %s: %r\n", srv, netdir);
		close(fd);
	}
}

static void
newtunnel(char *myname)
{
	int kid, pid;

	if(debug)
		fprint(2, "%s: starting new netssh for key access\n", argv0);
	kid = rfork(RFPROC|RFNOTEG|RFENVG /* |RFFDG */);
	if (kid == 0) {
//		for (fd = 3; fd < 40; fd++)
//			close(fd);
		execl("/bin/netssh", "netssh", "-m", netdir, "-s", myname, nil);
		sysfatal("no /bin/netssh: %r");
	} else if (kid < 0)
		sysfatal("fork failed: %r");
	while ((pid = waitpid()) != kid && pid >= 0)
		;
}

static void
starttunnel(void)
{
	char *keys, *mysrv, *myname;

	keys = esmprint("%s/ssh/keys", netdir);
	myname = esmprint("ssh.%s", getuser());
	mysrv = esmprint("/srv/%s", myname);

	if (access(keys, ORDWR) < 0)
		mounttunnel("/srv/netssh");		/* old name */
	if (access(keys, ORDWR) < 0)
		mounttunnel("/srv/ssh");
	if (access(keys, ORDWR) < 0)
		mounttunnel(mysrv);
	if (access(keys, ORDWR) < 0)
		newtunnel(myname);
	if (access(keys, ORDWR) < 0)
		mounttunnel(mysrv);

	/* if we *still* can't see our own tunnel, throw a tantrum. */
	if (access(keys, ORDWR) < 0)
		sysfatal("%s inaccessible: %r", keys);		/* WTF? */

	free(myname);
	free(mysrv);
	free(keys);
}

int
cmdmode(void)
{
	int n, m;
	char buf[Arbbufsz];

	for(;;) {
reprompt:
		print("\n>>> ");
		n = 0;
		do {
			m = read(0, buf + n, sizeof buf - n - 1);
			if (m <= 0)
				return 1;
			write(1, buf + n, m);
			n += m;
			buf[n] = '\0';
			if (buf[n-1] == ('u' & 037))
				goto reprompt;
		} while (buf[n-1] != '\n' && buf[n-1] != '\r');
		switch (buf[0]) {
		case '\n':
		case '\r':
			break;
		case 'q':
			return 1;
		case 'c':
			return 0;
		case 'r':
			stripcr = !stripcr;
			return 0;
		case 'h':
			print("c - continue\n");
			print("h - help\n");
			print("q - quit\n");
			print("r - toggle carriage return stripping\n");
			break;
		default:
			print("unknown command\n");
			break;
		}
	}
}

static void
keyprompt(char *buf, int size, int n)
{
	if (*buf == 'c') {
		fprint(kconsfd, "The following key has been offered by the server:\n");
		write(kconsfd, buf+5, n);
		fprint(kconsfd, "\n\n");
		fprint(kconsfd, "Add this key? (yes, no, session) ");
	} else {
		fprint(kconsfd, "The following key does NOT match the known "
			"key(s) for the server:\n");
		write(kconsfd, buf+5, n);
		fprint(kconsfd, "\n\n");
		fprint(kconsfd, "Add this key? (yes, no, session, replace) ");
	}
	n = read(kconsfd, buf, size - 1);
	if (n <= 0)
		return;
	write(keyfd, buf, n);		/* user's response -> /net/ssh/keys */
	seek(keyfd, 0, 2);
	if (readn(keyfd, buf, 5) <= 0)
		return;
	buf[5] = 0;
	n = strtol(buf+1, nil, 10);
	n = readn(keyfd, buf+5, n);
	if (n <= 0)
		return;
	buf[n+5] = 0;

	switch (*buf) {
	case 'b':
	case 'f':
		fprint(kconsfd, "%s\n", buf+5);
	case 'o':
		close(keyfd);
		close(kconsfd);
	}
}

/* talk the undocumented /net/ssh/keys protocol */
static void
keyproc(char *buf, int size)
{
	int n;
	char *p;

	if (size < 6)
		exits("keyproc buffer too small");
	p = esmprint("%s/ssh/keys", netdir);
	keyfd = open(p, ORDWR);
	if (keyfd < 0) {
		chpid = 0;
		sysfatal("failed to open ssh keys in %s: %r", p);
	}

	kconsfd = open("/dev/cons", ORDWR);
	if (kconsfd < 0)
		nopw = 1;

	buf[0] = 0;
	n = read(keyfd, buf, 5);		/* reading /net/ssh/keys */
	if (n < 0)
		sysfatal("%s read: %r", p);
	buf[5] = 0;
	n = strtol(buf+1, nil, 10);
	n = readn(keyfd, buf+5, n);
	buf[n < 0? 5: n+5] = 0;
	free(p);

	switch (*buf) {
	case 'f':
		if (kconsfd >= 0)
			fprint(kconsfd, "%s\n", buf+5);
		/* fall through */
	case 'o':
		close(keyfd);
		if (kconsfd >= 0)
			close(kconsfd);
		break;
	default:
		if (kconsfd >= 0)
			keyprompt(buf, size, n);
		else {
			fprint(keyfd, "n");
			close(keyfd);
		}
		break;
	}
	chpid = 0;
	exits(nil);
}

/*
 * start a subproc to copy from network to stdout
 * while we copy from stdin to network.
 */
static void
bidircopy(char *buf, int size)
{
	int i, n, lstart;
	char *path, *p, *q;

	rfork(RFNOTEG);
	path = esmprint("/proc/%d/notepg", getpid());
	notefd = open(path, OWRITE);

	switch (rfork(RFPROC|RFMEM|RFNOWAIT)) {
	case 0:
		while ((n = read(dfd2, buf, size - 1)) > 0) {
			if (!stripcr)
				p = buf + n;
			else
				for (i = 0, p = buf, q = buf; i < n; ++i, ++q)
					if (*q != '\r')
						*p++ = *q;
			if (p != buf)
				write(1, buf, p-buf);
		}
		/*
		 * don't bother; it will be obvious when the user's prompt
		 * changes.
		 *
		 * fprint(2, "%s: Connection closed by server\n", argv0);
		 */
		break;
	default:
		lstart = 1;
		while ((n = read(0, buf, size - 1)) > 0) {
			if (!mflag && lstart && buf[0] == 0x1c)
				if (cmdmode())
					break;
				else
					continue;
			lstart = (buf[n-1] == '\n' || buf[n-1] == '\r');
			write(dfd2, buf, n);
		}
		/*
		 * don't bother; it will be obvious when the user's prompt
		 * changes.
		 *
		 * fprint(2, "%s: EOF on client side\n", argv0);
		 */
		break;
	case -1:
		fprint(2, "%s: fork error: %r\n", argv0);
		break;
	}

	bail(nil);
}

static int
connect(char *buf, int size)
{
	int nfd, n;
	char *dir, *ds, *nf;

	dir = esmprint("%s/ssh", netdir);
	ds = netmkaddr(remote, dir, "22");		/* tcp port 22 is ssh */
	free(dir);

	dfd1 = dial(ds, nil, nil, &cfd1);
	if (dfd1 < 0) {
		fprint(2, "%s: dial conn %s: %r\n", argv0, ds);
		if (chpid) {
			nf = esmprint("/proc/%d/note", chpid);
			nfd = open(nf, OWRITE);
			fprint(nfd, "interrupt");
			close(nfd);
		}
		exits("can't dial");
	}

	seek(cfd1, 0, 0);
	n = read(cfd1, buf, size - 1);
	buf[n >= 0? n: 0] = 0;
	return atoi(buf);
}

static int
chanconnect(int conn, char *buf, int size)
{
	int n;
	char *path;

	path = esmprint("%s/ssh/%d!session", netdir, conn);
	dfd2 = dial(path, nil, nil, &cfd2);
	if (dfd2 < 0) {
		fprint(2, "%s: dial chan %s: %r\n", argv0, path);
		bail("dial");
	}
	free(path);

	n = read(cfd2, buf, size - 1);
	buf[n >= 0? n: 0] = 0;
	return atoi(buf);
}

static void
remotecmd(int argc, char *argv[], int conn, int chan, char *buf, int size)
{
	int i;
	char *path, *q, *ep;

	path = esmprint("%s/ssh/%d/%d/request", netdir, conn, chan);
	reqfd = open(path, OWRITE);
	if (reqfd < 0)
		bail("can't open request chan");
	if (argc == 0)
		if (readfile("/env/TERM", buf, size) < 0)
			fprint(reqfd, "shell");
		else
			fprint(reqfd, "shell %s", buf);
	else {
		assert(size >= Bigbufsz);
		ep = buf + Bigbufsz;
		q = seprint(buf, ep, "exec");
		for (i = 0; i < argc; ++i)
			q = seprint(q, ep, " %q", argv[i]);
		if (q >= ep) {
			fprint(2, "%s: command too long\n", argv0);
			fprint(reqfd, "close");
			bail("cmd too long");
		}
		write(reqfd, buf, q - buf);
	}
}

void
main(int argc, char *argv[])
{
	char *whichkey;
	int conn, chan, n;
	char buf[Copybufsz];

	quotefmtinstall();
	reqfd = dfd1 = cfd1 = dfd2 = cfd2 = consfd = kconsfd = cctlfd =
		notefd = keyfd = -1;
	whichkey = nil;
	ARGBEGIN {
	case 'A':			/* auth protos */
	case 'c':			/* ciphers */
		fprint(2, "%s: sorry, -%c is not supported\n", argv0, ARGC());
		break;
	case 'a':			/* compat? */
	case 'C':			/* cooked mode */
	case 'f':			/* agent forwarding */
	case 'p':			/* force pty */
	case 'P':			/* force no pty */
	case 'R':			/* force raw mode on pty */
	case 'v':			/* scp compat */
	case 'w':			/* send window-size changes */
	case 'x':			/* unix compat: no x11 forwarding */
		break;
	case 'd':
		debug++;
		break;
	case 'I':			/* non-interactive */
		iflag = 0;
		break;
	case 'i':			/* interactive: scp & rx do it */
		iflag = 1;
		break;
	case 'l':
	case 'u':
		user = EARGF(usage());
		break;
	case 'k':
		nopka = 1;
		break;
	case 'K':
		nopw = 1;
		break;
	case 'm':
		mflag = 1;
		break;
	case 'n':
		netdir = EARGF(usage());
		break;
	case 'r':
		stripcr = 1;
		break;
	case 'z':
		whichkey = EARGF(usage());
		break;
	default:
		usage();
	} ARGEND;
	if (argc == 0)
		usage();

	if (iflag == -1)
		iflag = isatty(0);
	remote = *argv++;
	--argc;

	parseargs();

	if (!user)
		user = getuser();
	if (user == nil || remote == nil)
		sysfatal("out of memory");

	starttunnel();

	/* fork subproc to handle keys; don't wait for it */
	if ((n = rfork(RFPROC|RFMEM|RFFDG|RFNOWAIT)) == 0)
		keyproc(buf, sizeof buf);
	chpid = n;
	atnotify(handler, 1);

	/* connect and learn connection number */
	conn = connect(buf, sizeof buf);

	consfd = open("/dev/cons", ORDWR);
	cctlfd = open("/dev/consctl", OWRITE);
	fprint(cctlfd, "rawon");
	if (doauth(cfd1, whichkey) < 0)
		bail("doauth");

	/* connect a channel of conn and learn channel number */
	chan = chanconnect(conn, buf, sizeof buf);

	/* open request channel, request shell or command execution */
	remotecmd(argc, argv, conn, chan, buf, sizeof buf);

	bidircopy(buf, sizeof buf);
}

int
isatty(int fd)
{
	char buf[64];

	buf[0] = '\0';
	fd2path(fd, buf, sizeof buf);
	return strlen(buf) >= 9 && strcmp(buf+strlen(buf)-9, "/dev/cons") == 0;
}

int
doauth(int cfd1, char *whichkey)
{
	UserPasswd *up;
	int n;
	char path[Arbpathlen];

 	if (!nopka) {
		if (whichkey)
			n = fprint(cfd1, "ssh-userauth K %q %q", user, whichkey);
		else
			n = fprint(cfd1, "ssh-userauth K %q", user);
		if (n >= 0)
			return 0;
	}
	if (nopw)
		return -1;
	up = auth_getuserpasswd(iflag? auth_getkey: nil,
		"proto=pass service=ssh server=%q user=%q", remote, user);
	if (up == nil) {
		fprint(2, "%s: didn't get password: %r\n", argv0);
		return -1;
	}
	n = fprint(cfd1, "ssh-userauth k %q %q", user, up->passwd);
	if (n >= 0)
		return 0;

	path[0] = '\0';
	fd2path(cfd1, path, sizeof path);
	fprint(2, "%s: auth ctl msg `ssh-userauth k %q <password>' for %q: %r\n",
		argv0, user, path);
	return -1;
}
