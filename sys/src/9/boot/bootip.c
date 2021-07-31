#include <u.h>
#include <libc.h>
#include <ip.h>

#include "boot.h"

static uchar	fsip[IPaddrlen];
static uchar	auip[IPaddrlen];
static char	mpoint[32];

static int isvalidip(uchar*);
static void netndb(char*, uchar*);

void
configip(void)
{
	int argc, pid, wpid;
	char **argv, *p;
	Waitmsg w;
	char **arg;
	char buf[32];
	
	fmtinstall('I', eipconv);
	fmtinstall('M', eipconv);
	fmtinstall('E', eipconv);

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
		wpid = wait(&w);
		if(wpid == pid){
			if(w.msg[0] != 0)
				fatal(w.msg);
			break;
		} else if(wpid < 0)
			fatal("configuring ip");
	}

	/* if we didn't get a file and auth server, query user */
	netndb("fs", fsip);
	netndb("auth", auip);
	if(!isvalidip(fsip)){
		buf[0] = 0;
		outin("filesystem IP address", buf, sizeof(buf));
		parseip(fsip, buf);
	}
	if(!isvalidip(auip)){
		buf[0] = 0;
		outin("authentication server IP address", buf, sizeof(buf));
		parseip(auip, buf);
	}
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
}

int
authtcp(void)
{
	return -1;
}

int
connecttcp(void)
{
	char buf[2*NAMELEN];

	sprint(buf, "tcp!%I!564", fsip);
	return dial(buf, 0, 0, 0);
}

void
configil(Method*)
{
	configip();
}

int
authil(void)
{
	char buf[2*NAMELEN];

	sprint(buf, "il!%I!566", auip);
	return dial(buf, 0, 0, 0);
}

int
connectil(void)
{
	char buf[2*NAMELEN];

	sprint(buf, "il!%I!17008", fsip);
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
			break;
		}
		p++;
	}
}
