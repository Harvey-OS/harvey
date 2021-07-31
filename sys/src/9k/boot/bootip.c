#include <u.h>
#include <libc.h>
#include <ip.h>

#include "boot.h"

static	uchar	fsip[IPaddrlen];
static	uchar	auip[IPaddrlen];
static	char	mpoint[32];

static int isvalidip(uchar*);
static void getndbvar(char *name, uchar *var, char *prompt);

void
configip(int bargc, char **bargv, int needfs)
{
	Waitmsg *w;
	int argc, pid;
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
	strcpy(mpoint, "/net");
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
	if(access("#l0", 0) == 0 && bind("#l0", mpoint, MAFTER) < 0)
		warning("bind #l0");
	dprint("bind #l1...");
	if(access("#l1", 0) == 0 && bind("#l1", mpoint, MAFTER) < 0)
		warning("bind #l1");
	dprint("bind #l2...");
	if(access("#l2", 0) == 0 && bind("#l2", mpoint, MAFTER) < 0)
		warning("bind #l2");
	dprint("bind #l3...");
	if(access("#l3", 0) == 0 && bind("#l3", mpoint, MAFTER) < 0)
		warning("bind #l3");
	werrstr("");

	/* let ipconfig configure the first ip interface */
	switch(pid = fork()){
	case -1:
		fatal("fork configuring ip: %r");
	case 0:
		dprint("starting ipconfig...");
		exec("/boot/ipconfig", arg);
		fatal("execing /boot/ipconfig: %r");
	default:
		break;
	}

	/* wait for ipconfig to finish */
	dprint("waiting for dhcp...");
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
