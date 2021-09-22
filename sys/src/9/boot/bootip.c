#include <u.h>
#include <libc.h>
#include <ip.h>

#include "boot.h"

enum {
	Ttftp = 17015,
};

static	uchar	fsip[IPaddrlen];
static	uchar	auip[IPaddrlen];
static	char	mpoint[32];
static	int	ipconfiged;

static int isvalidip(uchar*);
static void getndbvar(char *name, uchar *var, char *prompt);

void
configip(int bargc, char **bargv, int needfs)
{
	Waitmsg *w;
	int argc, pid, i;
	char **arg, **argv, *p;

	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);
	fmtinstall('E', eipfmt);

	arg = malloc((bargc+1) * sizeof(char*));
	if(arg == nil)
		fatal("malloc");
	memmove(arg, bargv, bargc * sizeof(char*));
	arg[bargc] = 0;

	argc = bargc;
	argv = arg;
	strncpy(mpoint, "/net", sizeof mpoint);
	ARGBEGIN {
	case 'x':
		p = ARGF();
		if(p != nil)
			snprint(mpoint, sizeof(mpoint), "/net%s", p);
		break;
	case 'g':
	case 'b':
	case 'h':
	case 'm':
		p = ARGF();
		USED(p);
		break;
	} ARGEND;

	/* bind in an ip interface or two */
	dprint("bind #I...");
	if(bind("#I", mpoint, MAFTER) < 0)
		fatal("bind #I");
	dprint("bind #l0...");
	if(access("#l0", AEXIST) == 0 && bind("#l0", mpoint, MAFTER) < 0)
		warning("bind #l0");
	dprint("bind #l1...");
	if(access("#l1", AEXIST) == 0 && bind("#l1", mpoint, MAFTER) < 0)
		warning("bind #l1");
	dprint("bind #l2...");
	if(access("#l2", AEXIST) == 0 && bind("#l2", mpoint, MAFTER) < 0)
		warning("bind #l2");
	dprint("bind #l3...");
	if(access("#l3", AEXIST) == 0 && bind("#l3", mpoint, MAFTER) < 0)
		warning("bind #l3");

	if(access("/boot/snoopy", AEXEC) == 0)
		if(fork() == 0){
			dprint("snoopy...");
			execl("/boot/snoopy", "snoopy", nil);
			fatal("execing /boot/snoopy: %r");
		}

	/* let ipconfig configure the first ip interface */
	werrstr("");
	switch(pid = fork()){
	case -1:
		fatal("fork configuring ip: %r");
	case 0:
		dprint("starting ipconfig args `");
		for (i = 0; arg[i] != nil; i++)
			dprint(" %s", arg[i]);
		dprint("'\n");
		exec("/boot/ipconfig", arg);
		fatal("execing /boot/ipconfig: %r");
	default:
		break;
	}

	/* wait for ipconfig to finish */
write(1, "dhcp...", 7);
	dprint("waiting for ipconfig (dhcp)...");
	for(;;){
		w = wait();
		if(w != nil && w->pid == pid){
			if(w->msg[0] != 0)
				fatal(w->msg);
			free(w);
			break;
		} else if(w == nil)
			fatal("configuring ip");
		free(w);
	}
	dprint("\n");

	if(needfs) {  /* if we didn't get a file and auth server, query user */
		getndbvar("fs", fsip, "filesystem IP address");
		getndbvar("auth", auip, "authentication server IP address");
	}

	ipconfiged = 1;
}

static void
setauthaddr(char *proto, int port)
{
	char buf[128];

	snprint(buf, sizeof buf, "%s!%I!%d", proto, auip, port);
	authaddr = strdup(buf);
}

void
configtcp(Method*)
{
	dprint("configip...");
	configip(bargc, bargv, 1);
	dprint("setauthaddr...");
	setauthaddr("tcp", 567);
}

int
connecttcp(void)
{
	int fd;
	char buf[64];

	snprint(buf, sizeof buf, "tcp!%I!564", fsip);
	dprint("dial %s...", buf);
	fd = dial(buf, 0, 0, 0);
	if (fd < 0)
		werrstr("dial %s: %r", buf);
	return fd;
}

char *
basename(char *file)
{
	char *base;

	base = strrchr(file, '/');
	return base == nil? file: base + 1;
}

/* map /.../prog or prog to /boot/prog; result is malloced. */
char *
bootname(char *file)
{
	return smprint("/boot/%s", basename(file));
}

/* map /.../prog to /$cputype/.../prog; result is malloced. */
char *
archname(char *file)
{
	return smprint("/%s%s", cputype, file);
}

static int
mountramfsafter(char *mntpt)
{
	static int ramfs;
	static char tmpmtpt[] = "/net.alt";	/* temporary, in #/ */

	if (ramfs++ != 0)
		return 0;
	run("/boot/ramfs", "-m", tmpmtpt, nil);
	return bind(tmpmtpt, mntpt, MAFTER|MCREATE);
}

/* totally-trivial file transfer protocol of /n/fs/cpuremote to binfd */
static int
ttftp(int binfd, char *fs, char *cpuremote)
{
	int rv, net, n;
	char buf[4096];

	rv = -1;
	notify(catchint);
	snprint(buf, sizeof buf, "tcp!%s!%d", fs, Ttftp);
	net = dial(buf, 0, nil, nil);
	if (net < 0)
		return rv;
	fprint(net, "%s\n", cpuremote);
	alarm(5*1000);
	while ((n = read(net, buf, sizeof buf)) > 0) {
		write(binfd, buf, n);
		rv = 0;
		alarm(5*1000);
	}
	alarm(0);
	close(net);
	if (n < 0)
		rv = -1;
	return rv;
}

static int
fetch(char *remote, char *binary)
{
	int rv, fd;
	char *base, *cpuremote;
	char fs[50];
	static int depth;

	rv = -1;
	if (depth > 0) {
		fprint(2, "fetchmissing recursion for %s!\n", remote);
		return -1;
	}
	++depth;
	base = basename(remote);
	dprint("fetch %s -> %s...", remote, binary);
	fprint(2, "fetch %s...", base);
	mountramfsafter("/boot");

	fd = create(binary, OWRITE, 0777);
	if (fd < 0)
		fprint(2, "creating %s: %r\n", binary);
	else {
		snprint(fs, sizeof fs, "%I", fsip);
		cpuremote = archname(remote);
		rv = ttftp(fd, fs, cpuremote);
		if (rv < 0)
			fprint(2, "failed.\n");
		close(fd);
		free(cpuremote);
	}
	--depth;
	return rv;
}

/*
 * if /boot/`{basename remote} is missing, start ramfs if needed & bind -ac it
 * to /boot, use ttftp to fetch remote copy.  this lets us trade-off a
 * little start-up time for smaller kernel size, which may permit direct pxe
 * booting.
 *
 * accepts a canonical executable path from /bin and generates the /boot
 * equivalent path.
 *
 * this will only work after configip has run.
 */
int
fetchmissing(char *remote)
{
	int rv;
	char *base, *binary;

	rv = -1;
	base = basename(remote);
	binary = bootname(base);
	if (access(binary, AEXIST) < 0)
		if (!ipconfiged)
			fprint(2, "no ip, no ttftp %s...", base);
		else
			rv = fetch(remote, binary);
	free(binary);
	return rv;
}

static int
isvalidip(uchar *ip)
{
	if(ipcmp(ip, IPnoaddr) == 0)
		return 0;
	if(ipcmp(ip, v4prefix) == 0)
		return 0;
	return 1;
}

static void
netenv(char *attr, uchar *ip)
{
	int fd, n;
	char buf[128];

	ipmove(ip, IPnoaddr);
	snprint(buf, sizeof(buf), "#e/%s", attr);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;

	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return;
	buf[n] = 0;
	if (parseip(ip, buf) == -1)
		fprint(2, "netenv: can't parse ip %s\n", buf);
}

static void
netndb(char *attr, uchar *ip)
{
	int fd, n, c;
	char buf[1024];
	char *p;

	ipmove(ip, IPnoaddr);
	snprint(buf, sizeof(buf), "%s/ndb", mpoint);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return;
	buf[n] = 0;
	n = strlen(attr);
	for(p = buf; ; p++){
		p = strstr(p, attr);
		if(p == nil)
			break;
		c = *(p-1);
		if(*(p + n) == '=' && (p == buf || c == '\n' || c == ' ' || c == '\t')){
			p += n+1;
			if (parseip(ip, p) == -1)
				fprint(2, "netndb: can't parse ip %s\n", p);
			return;
		}
	}
}

static void
getndbvar(char *name, uchar *var, char *prompt)
{
	char buf[64];

	netndb(name, var);
	if(!isvalidip(var))
		netenv(name, var);
	while(!isvalidip(var)){
		buf[0] = 0;
		outin(prompt, buf, sizeof buf);
		if (parseip(var, buf) == -1)
			fprint(2, "configip: can't parse %s ip %s\n", name, buf);
	}
}
