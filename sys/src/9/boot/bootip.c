#include <u.h>
#include <libc.h>
#include <ip.h>

#include "boot.h"

static	uchar	fsip[IPaddrlen];
	uchar	auip[IPaddrlen];
static	char	mpoint[32];

static int isvalidip(uchar*);
static void netndb(char*, uchar*);
static void netenv(char*, uchar*);

void
configip(void)
{
	int argc, pid;
	char **argv, *p;
	Waitmsg *w;
	char **arg;
	char buf[32];
	
	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);
	fmtinstall('E', eipfmt);

	arg = malloc((bargc+1) * sizeof(char*));
	if(arg == nil)
		fatal("%r");
	memmove(arg, bargv, (bargc+1) * sizeof(char*));

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

	/* bind in an ip interface */
	bind("#I", mpoint, MAFTER);
	bind("#l0", mpoint, MAFTER);
	bind("#l1", mpoint, MAFTER);
	bind("#l2", mpoint, MAFTER);
	bind("#l3", mpoint, MAFTER);

	/* let ipconfig configure the ip interface */
	switch(pid = fork()){
	case -1:
		fatal("configuring ip: %r");
	case 0:
		exec("/ipconfig", arg);
		fatal("execing /ipconfig");
	default:
		break;
	}

	/* wait for ipconfig to finish */
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

	/* if we didn't get a file and auth server, query user */
	netndb("fs", fsip);
	if(!isvalidip(fsip))
		netenv("fs", fsip);
	while(!isvalidip(fsip)){
		buf[0] = 0;
		outin("filesystem IP address", buf, sizeof(buf));
		parseip(fsip, buf);
	}

	netndb("auth", auip);
	if(!isvalidip(auip))
		netenv("auth", auip);
	while(!isvalidip(auip)){
		buf[0] = 0;
		outin("authentication server IP address", buf, sizeof(buf));
		parseip(auip, buf);
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
	sleep(100);
	print("t");
	sleep(100);
	configip();
	sleep(100);
	print(".");
	sleep(100);
	setauthaddr("tcp", 567);
}

int
connecttcp(void)
{
	char buf[64];

	snprint(buf, sizeof buf, "tcp!%I!564", fsip);
	return dial(buf, 0, 0, 0);
}

void
configil(Method*)
{
	configip();
	setauthaddr("il", 566);
}

int
connectil(void)
{
	char buf[64];

	snprint(buf, sizeof buf, "il!%I!17008", fsip);
	return dial(buf, 0, 0, 0);
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
	if(n <= 0)
		return;
	buf[n] = 0;
	parseip(ip, buf);
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
	for(p = buf;;){
		p = strstr(p, attr);
		if(p == nil)
			break;
		c = *(p-1);
		if(*(p + n) == '=' && (p == buf || c == '\n' || c == ' ' || c == '\t')){
			p += n+1;
			parseip(ip, p);
			return;
		}
		p++;
	}
	return;
}
