/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>

#include "boot.h"

static	uint8_t	fsip[IPaddrlen];
	uint8_t	auip[IPaddrlen];
static	char	mpoint[32];

static int isvalidip(uint8_t*);
static void netndb(char*, uint8_t*);
static void netenv(char*, uint8_t*);


void
configip(int bargc, char **bargv, int needfs)
{
	Waitmsg *w;
	int argc, pid;
	char **arg, **argv, buf[32], *p;

	fmtinstall('I', eipfmt);
	fmtinstall('M', eipfmt);
	fmtinstall('E', eipfmt);

	arg = malloc((bargc+1) * sizeof(char*));
	if(arg == nil)
		fatal("malloc");
	memmove(arg, bargv, bargc * sizeof(char*));
	arg[bargc] = 0;

	print("ipconfig...\n");
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
	if(bind("#I", mpoint, MAFTER) < 0)
		fatal("bind #I\n");
	if(access("#l0", 0) == 0 && bind("#l0", mpoint, MAFTER) < 0)
		print("bind #l0: %r\n");
	if(access("#l1", 0) == 0 && bind("#l1", mpoint, MAFTER) < 0)
		print("bind #l1: %r\n");
	if(access("#l2", 0) == 0 && bind("#l2", mpoint, MAFTER) < 0)
		print("bind #l2: %r\n");
	if(access("#l3", 0) == 0 && bind("#l3", mpoint, MAFTER) < 0)
		print("bind #l3: %r\n");
	werrstr("");

	/* let ipconfig configure the ip interface */
	switch(pid = fork()){
	case -1:
		fatal("fork configuring ip");
	case 0:
		exec("/boot/ipconfig", arg);
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

	if(!needfs)
		return;

	/* if we didn't get a file and auth server, query user */
	netndb("fs", fsip);
	if(!isvalidip(fsip))
		netenv("fs", fsip);
	while(!isvalidip(fsip)){
		outin("filesystem IP address", buf, sizeof(buf));
		if (parseip(fsip, buf) == -1)
			fprint(2, "configip: can't parse fs ip %s\n", buf);
	}

	netndb("auth", auip);
	if(!isvalidip(auip))
		netenv("auth", auip);
	while(!isvalidip(auip)){
		outin("authentication server IP address", buf, sizeof(buf));
		if (parseip(auip, buf) == -1)
			fprint(2, "configip: can't parse auth ip %s\n", buf);
	}
	free(arg);
}

static void
setauthaddr(char *proto, int port)
{
	char buf[128];

	snprint(buf, sizeof buf, "%s!%I!%d", proto, auip, port);
	authaddr = strdup(buf);
}

void
configtcp(Method* m)
{
	configip(bargc, bargv, 1);
	setauthaddr("tcp", 567);
}

int
connecttcp(void)
{
	int fd;
	char buf[64];

	snprint(buf, sizeof buf, "tcp!%I!5640", fsip);
	fd = dial(buf, 0, 0, 0);
	if (fd < 0)
		werrstr("dial %s: %r", buf);
	return fd;
}

static int
isvalidip(uint8_t *ip)
{
	if(ipcmp(ip, IPnoaddr) == 0)
		return 0;
	if(ipcmp(ip, v4prefix) == 0)
		return 0;
	return 1;
}

static void
netenv(char *attr, uint8_t *ip)
{
	int fd, n;
	char buf[128];

	ipmove(ip, IPnoaddr);
	snprint(buf, sizeof(buf), "#e/%s", attr);
	fd = open(buf, OREAD);
	if(fd < 0)
		return;

	n = read(fd, buf, sizeof(buf)-1);
	if(n > 0){
		buf[n] = 0;
		if (parseip(ip, buf) == -1)
			fprint(2, "netenv: can't parse ip %s\n", buf);
	}
	close(fd);
}

static void
netndb(char *attr, uint8_t *ip)
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
	return;
}
