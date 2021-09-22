/*
 * sshswitch service protocol netdir - read first line & decide which
 *	ssh server to invoke
 */
#include <u.h>
#include <libc.h>
#include "ssh2.h"

enum {
	Maxsynth = 1024,
};

static int debug;

void
usage(void)
{
	fprint(2, "usage: %s svc proto netdir\n", argv0);
	exits("usage");
}

char *
readfile(char *file)
{
	int fd, n;
	char *data;

	fd = open(file, OREAD);
	if (fd < 0)
		return nil;
	data = malloc(Maxsynth);
	if (data) {
		n = read(fd, data, Maxsynth - 1);
		if (n == Maxsynth - 1)
			fprint(2, "%s: losing data from tail of %s\n",
				argv0, file);
		data[n >= 0? n: 0] = '\0';
	}
	close(fd);
	return data;
}

static void
fdck(void)
{
	int i;

	if (debug)
		for (i = 0; i < 2; i++)
			if (dirfstat(i) == nil)
				fprint(2, "sshswitch fd %d closed!\n", i);
}

static void
dump(void *p, int size)
{
	int i;
	uchar *s;

	s = p;
	for (i = size; i > 0; i--)
		fprint(2, "%2.2x ", *s);
	fprint(2, "\n");

	s = p;
	for (i = size; i > 0; i--)
		fprint(2, "%c", *s);
	fprint(2, "\n");
}

static void
readline(int fd, char *id, int size)
{
	int n;
	char *p;

	fdck();
	alarm(60*1000);
	for (p = id; p < id + size - 1; p++) {
		werrstr("");
		*p = '\0';
		n = read(fd, p, 1);
		if (n == 0)
			break;
		if (n < 0) {
			if (debug)
				fprint(2, "sshswitch read(%d) returned %d: %r\n",
					fd, n);
			dump(id, p - id);
			exits("no id line");
		}
		if (debug)
			fprint(2, "sshswitch read %c\n", (uchar)*p);
		if (*p == '\n') {
			*p = '\0';
			break;
		}
	}
	alarm(0);
	dump(id, p - id);
}

void
main(int argc, char *argv[])
{
	int i, fd, net;
	char *netdir, *tcpconn, *tcpdata;
	char *nargv[8];
	char id[256];

	fd = open("/sys/log/ssh", OWRITE);
	if (fd >= 0) {
		if (fd != 2)
			dup(fd, 2);
		close(fd);
	}

	debug = 0;
	ARGBEGIN {
	case 'd':
		debug++;
		break;
	default:
		usage();
	} ARGEND;
	if (argc != 3)
		usage();
	netdir = argv[2];

	/*
	 * remaining args from listen will be roughly
	 * "ssh22", "ssh", "/net/ssh/1".
	 * network (/net/ssh/1/data) should be on fds 0, 1 and 2 at entry
	 * (we've since reopened 2).
	 */
	if (debug) {
		for (i = 0; i < argc; i++)
			fprint(2, "sshswitch argv[%d] %s\n", i, argv[i]);
		for (i = 0; i < 2; i++)
			if (dirfstat(i) == nil)
				fprint(2, "sshswitch fd %d closed at entry!\n", i);
			else {
				fd2path(i, id, sizeof id);
				fprint(2, "sshswitch fd %d (%s) open at entry\n",
					i, id);
			}
	}

	/* dig out underlying tcp connection and use it for id strings */
	tcpconn = readfile(esmprint("%s/tcp", netdir));
	if (debug)
		fprint(2, "sshswitch %s/tcp contains %s\n", netdir, tcpconn);
	tcpdata = esmprint("%s/../../tcp/%s/data", netdir, tcpconn);
	cleanname(tcpdata);
	if (debug)
		fprint(2, "sshswitch opening %s\n", tcpdata);

	net = open(tcpdata, ORDWR);
	if (net < 0)
		sysfatal("can't open tcp conn %s data: %r", tcpdata);

	/* send server's id string first */
	/* sshtun used to send this before we get invoked */
	fprint(net, "SSH-1.99-Plan9\r\n");		/* v2 with v1 compat. */

	/* read client's id string from network */
	if (debug)
		fprint(2, "sshswitch reading client id from network fd %d\n",
			net);
	fdck();
	readline(net, id, sizeof id);
	close(net);

	if (debug)
		fprint(2, "sshswitch read id `%s'\n", id);
	if (id[0] == '\0')
		sysfatal("nil client id string");

	for (i = 3; i < 40; i++)
		close(i);

	/* make servers take `id' as id string and not read the first line */
	i = 1;
	nargv[i++] = "-i";
	nargv[i++] = id;

	/* exec the ssh server appropriate to the ssh version */
	if (cistrncmp(id, "ssh-1", 5) == 0) {
		nargv[0] = "sshserve";
		nargv[i++] = "-A";
		nargv[i++] = "tis password";
		nargv[i++] = readfile(esmprint("%s/remote", netdir));
		nargv[i] = nil;
		if (debug)
			fprint(2, "sshswitch starting sshserve (v1)\n");
		exec("/bin/aux/sshserve", nargv);
	} else {
		nargv[0] = "sshsession";
		nargv[i++] = netdir;
		nargv[i] = nil;
		if (debug)
			fprint(2, "sshswitch starting sshsession (v2)\n");
		exec("/bin/aux/sshsession", nargv);
	}
	sysfatal("can't exec %s: %r", nargv[0]);
}
