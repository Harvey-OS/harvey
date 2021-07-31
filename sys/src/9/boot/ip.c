#include <u.h>
#include <libc.h>
#include "../port/bootp.h"
#include "../port/arp.h"
#include "../boot/boot.h"

/*
 *  il 
 */
uchar	fsip[4];
uchar	auip[4];
uchar	gwip[4];
uchar	ipmask[4];
uchar	ipaddr[4];	/* our ip address */
uchar	eaddr[6];	/* our ether address */
uchar 	bcast[6];	/* our ether broadcast address */
uchar	ipnet[4];	/* our ip network number */

static void	arp(uchar*);
static int	arplisten(void);
static ushort	nhgets(uchar*);
static void	hnputs(uchar*, ushort);
static void	parseip(uchar*, char*);
static int	myetheraddr(uchar*, char*);
static int	parseether(uchar*, char*);
static int	mygetfields(char*, char**, int);
static char*	fmtaddr(uchar*);
static void	maskip(uchar*, uchar*, uchar*);
static int	equivip(uchar*, uchar*);
static void	etheripconfig(Method*);
static int	ipdial(int*, char*, uchar*, int);
static void	catchint(void*, char*);

uchar classmask[4][4] = {
	0xff, 0x00, 0x00, 0x00,
	0xff, 0x00, 0x00, 0x00,
	0xff, 0xff, 0x00, 0x00,
	0xff, 0xff, 0xff, 0x00,
};

void
configtcp(Method *mp)
{
	etheripconfig(mp);
}

int
authtcp(void)
{
	return -1;
}

int
connecttcp(void)
{
	int fd[2], rv;

	rv = ipdial(fd, "#Itcp/tcp", fsip, 564);
	if(cpuflag)
		sendmsg(fd[0], "push reboot");
	sendmsg(fd[0], "push fcall");
	if(rv >= 0)
		close(fd[0]);
	return fd[1];
}

void
configil(Method *mp)
{
	etheripconfig(mp);
}

int
authil(void)
{
	int fd[2]; 

	if(auip[0] == 0 || ipdial(fd, "#Iil/il", auip, 566) < 0)
		return -1;
	close(fd[0]);
	return fd[1];
}

int
connectil(void)
{
	int fd[2], rv;

	rv = ipdial(fd, "#Iil/il", fsip, 17008);
	if(cpuflag)
		sendmsg(fd[0], "push reboot");
	if(rv >= 0)
		close(fd[0]);
	return fd[1];
}

static void
etheripconfig(Method *mp)
{
	int efd[2];

	/* configure/open ip */
	myetheraddr(eaddr, "#l/ether");
/*print("my etheraddr is %2.2ux%2.2ux%2.2ux%2.2ux%2.2ux%2.2ux\n", eaddr[0], eaddr[1],
eaddr[2], eaddr[3], eaddr[4], eaddr[5]);/**/
	if(plumb("#l/ether", "0x800", efd, 0) < 0)
		fatal("opening ip ether");

	sendmsg(efd[0], "push arp");
	sendmsg(efd[0], "push internet");
	sendmsg(efd[0], "push permanent");

	/* do a bootp to find fs, auth server, & gateway */
	bootp(mp, efd[0], 0);

	/* done with the mux */
	close(efd[0]);
	close(efd[1]);
}

/*
 *  configure ip. use bootp to get ip address, net mask, file server ip address,
 *  authentication server ip address and gateway ip address.
 */
void
bootp(Method *mp, int muxctlfd, uchar *useipaddr)
{
	int fd;
	int ufd[2];
	int n;
	int tries;
	Bootp req, *rp;
	char *field[4];
	char buf[1600];
	char buf2[64];
	uchar ipbcast[4];	/* ip broadcast address for bootp */

	/* determine bootp broadcast address from specified file system address */
	if(*sys){
		parseip(fsip, sys);
		parseip(gwip, sys);
		memmove(ipmask, classmask[fsip[0]>>6], sizeof(ipmask));
		for(n = 0; n < sizeof(ipbcast); n++)
			ipbcast[n] = (ipmask[n] & fsip[n]) | ((~ipmask[n])&0xff);
		sprint(buf2, "%s!67", fmtaddr(ipbcast));
	} else {
		memmove(ipmask, classmask[3], sizeof(ipmask));
		strcpy(buf2, "255.255.255.255!67");
	}

	memset(bcast, 0xff, sizeof(bcast));	/* ether broadcast address */

	/* open a udp connection for bootp and fill in a packet */
	if(plumb("#Iudp/udp", buf2, ufd, "68") < 0)
		fatal("opening bootp udp");
	close(ufd[0]);
	memset(&req, 0, sizeof(req));
	req.op = Bootrequest;
	req.htype = 1;		/* ethernet */
	req.hlen = 6;		/* ethernet */
	memmove(req.chaddr, eaddr, sizeof(req.chaddr));
	if(useipaddr != 0)
		memmove(req.ciaddr, useipaddr, sizeof(req.ciaddr));
	memset(req.file, 0, sizeof(req.file));
	strcpy(req.vend, "p9  ");

	/* broadcast bootp's till we get a reply, or 3 times around the loop */
	notify(catchint);
	tries = 0;
	field[0] = 0;
	for(rp = 0; rp == 0 && tries++ < 10;){
		alarm(1000);
		if(write(ufd[1], &req, sizeof(req)) < 0)
			fatal("sending bootp");
		for(;;){
			rp = 0;
			memset(buf, 0, sizeof(buf));
			n = read(ufd[1], buf, sizeof(buf));
			if(n <= 0)
				break;
			rp = (Bootp*)buf;
			memset(field, 0, sizeof field);
			if(memcmp(req.chaddr, rp->chaddr, 6) == 0
			&& rp->htype == 1
			&& rp->hlen == 6
			&& mygetfields(rp->vend+4, field, 4) == 4){
				if(strncmp(rp->vend, "p9  ", 4) == 0){
					memmove(ipaddr, rp->yiaddr, sizeof(ipaddr));
					parseip(ipmask, field[0]);
					if(*sys == 0){
						strcpy(sys, field[1]);
						parseip(fsip, field[1]);
					}
					parseip(auip, field[2]);
					parseip(gwip, field[3]);
					maskip(ipaddr, ipmask, ipnet);
					if(bootfile[0] == 0){
						strncpy(bootfile, rp->file,
							 3*NAMELEN);
						bootfile[3*NAMELEN-1] = 0;
					}
					break;
				}
			}
		}
		alarm(0);
	}
	close(ufd[1]);

	if(field[0])
		/*print("I am %s sub %s fs %s au %s gw %s\n",
		fmtaddr(ipaddr), field[0], sys, field[2], field[3])/**/;
	else {
		errstr(buf);	/* Clear timeout error from alarm */

		if(readfile("#e/ipaddr", buf2, sizeof(buf2)) < 0)
			strcpy(buf2, "");
		outin(0, "My IP address", buf2, sizeof(buf2));
		parseip(ipaddr, buf2);

		if(readfile("#e/ipmask", buf2, sizeof(buf2)) < 0)
			strcpy(buf2, "");
		outin(0, "My IP mask", buf2, sizeof(buf2));
		parseip(ipmask, buf2);
		maskip(ipaddr, ipmask, ipnet);

		if(readfile("#e/ipgw", buf2, sizeof(buf2)) < 0)
			strcpy(buf2, "");
		outin(0, "My IP gateway", buf2, sizeof(buf2));
		parseip(gwip, buf2);

		if(*sys)
			strcpy(buf2, sys);
		else {
			if(readfile("#e/fs", buf2, sizeof(buf2)) < 0)
				strcpy(buf2, "");
			outin(0, "filesystem IP address", buf2, sizeof(buf2));
		}
		parseip(fsip, buf2);

		if(readfile("#e/auth", buf2, sizeof(buf2)) < 0)
			strcpy(buf2, "0.0.0.0");
		outin(0, "authentication server IP address", buf2, sizeof(buf2));
		parseip(auip, buf2);
	}

	if(auip[0] == 0 && auip[1] == 0)
		mp->auth = 0;

	/* set our ip address and mask */
	n = sprint(buf2, "setip %s ", fmtaddr(ipaddr));
	sprint(buf2+n, "%s", fmtaddr(ipmask));
	sendmsg(muxctlfd, buf2);

	/* specify a routing gateway */
	if(*gwip){
		sprint(buf2, "add 0.0.0.0 0.0.0.0 %s", fmtaddr(gwip));
		fd = open("#P/iproute", OWRITE);
		if(fd < 0)
			fatal("opening iproute");
		if(sendmsg(fd, buf2) < 0)
			print("%s failed\n", buf2);
		close(fd);
	}
}

static int
ipdial(int *ifd, char *dev, uchar *ip, int service)
{
	uchar tmp[4];
	char buf[64];
	int arpnotefd;

	/* start a process to answer arps */
	arpnotefd = arplisten();

	/* arp for first hop */
	maskip(ip, ipmask, tmp);
	if(equivip(tmp, ipnet))
		arp(ip);
	else
		arp(gwip);

	/* make the call */
	sprint(buf, "%s!%d", fmtaddr(ip), service);
	if(plumb(dev, buf, ifd, 0) < 0){
		fprint(2, "error dialing %s\n", buf);
		ifd[1] = -1;
	}

	fprint(arpnotefd, "kill");

	return ifd[1];
}

/* send an arprequest, wait for a reply */
static void
arp(uchar *addr)
{
	int afd[2];
	int arpdev;
	int n;
	Arpentry entry;
	Arppkt req, *rp;
	char buf[1600];

	if(plumb("#l/ether", "0x806", afd, 0) < 0)
		fatal("opening ip ether");
	close(afd[0]);
	arpdev = open("#a/arp/data", OWRITE);
	if(arpdev < 0)
		fatal("opening arp/data");

	/* arp for the file server or the gateway */
	memset(&req, 0, sizeof(req));
	memmove(req.tpa, addr, sizeof(req.tpa));
	memset(req.d, 0xff, sizeof(req.d));
	memmove(req.spa, ipaddr, sizeof(ipaddr));
	memmove(req.sha, eaddr, sizeof(eaddr));
	hnputs(req.type, ET_ARP);
	hnputs(req.hrd, 1);
	hnputs(req.pro, 0x800);
	req.hln = sizeof(req.sha);
	req.pln = sizeof(req.spa);
	hnputs(req.op, ARP_REQUEST);
	for(rp = 0; rp == 0;){
		if(write(afd[1], &req, sizeof(req)) < 0)
			fatal("sending arpreq");
		alarm(1000);
		for(;;){
			rp = 0;
			memset(buf, 0, sizeof(buf));
			n = read(afd[1], buf, sizeof(buf));
			if(n <= 0)
				break;
			rp = (Arppkt*)buf;
			if(nhgets(rp->op) != ARP_REPLY)
				continue;
			memcpy(entry.etaddr, rp->sha, sizeof(entry.etaddr));
			memcpy(entry.ipaddr, rp->spa, sizeof(entry.ipaddr));
			if(write(arpdev, &entry, sizeof(entry)) < 0)
				warning("write arp entry");
print("arp: rcvd for %s\n", fmtaddr(rp->spa));/**/
			if(equivip(rp->spa, addr))
				break;
		}
		alarm(0);
	}

	close(arpdev);
	close(afd[1]);
}

/*
 *  fork a process to answer arp requests for us.  since
 *  we will kill it after changing user id, we need to open the
 *  note process here.  arplisten returns the fd of the open note
 *  process.
 */
static int
arplisten(void)
{
	int afd[2];
	int n;
	int pid;
	Arppkt reply, *rp;
	char buf[1600];

	alarm(0);
	notify(catchint);

	switch(pid = fork()){
	case -1:
		fatal("forking arplisten");
	case 0:
		break;
	default:
		sprint(buf, "#p/%d/note", pid);
		return open(buf, OWRITE);
	}

	if(plumb("#l/ether", "0x806", afd, 0) < 0)
		fatal("opening ip ether");

	for(;;){
		memset(buf, 0, sizeof(buf));
		n = read(afd[1], buf, sizeof(buf));
		if(n < 0)
			break;
		if(n == 0)
			continue;
		rp = (Arppkt*)buf;
		if(nhgets(rp->op) != ARP_REQUEST)
			continue;
		if(memcmp(rp->tpa, ipaddr, sizeof(ipaddr)) != 0)
			continue;

		memset(&reply, 0, sizeof(reply));
		hnputs(reply.type, ET_ARP);
		hnputs(reply.hrd, 1);
		hnputs(reply.pro, 0x800);
		reply.hln = sizeof(reply.sha);
		reply.pln = sizeof(reply.spa);
		hnputs(reply.op, ARP_REPLY);
		memmove(reply.tha, rp->sha, sizeof(reply.tha));
		memmove(reply.tpa, rp->spa, sizeof(reply.tpa));
		memmove(reply.sha, eaddr, sizeof(eaddr));
		memmove(reply.spa, ipaddr, sizeof(ipaddr));
		memmove(reply.d, rp->s, sizeof(rp->s));
print("arplisten: reply to %s\n", fmtaddr(reply.tpa));/**/
		if(write(afd[1], &reply, ARPSIZE) < 0)
			warning("write arp reply");
	}
	exits(0);
	return 0;	/* not reached */
}

static ushort
nhgets(uchar *val)
{
	return (val[0]<<8) | val[1];
}

static void
hnputs(uchar *ptr, ushort val)
{
	ptr[0] = val>>8;
	ptr[1] = val;
}

int
myipaddr(uchar *to, char *dev)
{
	char buf[256];
	int n, fd, clone;
	char *ptr;

	/* Opening clone ensures the 0 connection exists */
	sprint(buf, "%s/clone", dev);
	clone = open(buf, OREAD);
	if(clone < 0)
		return -1;

	sprint(buf, "%s/0/local", dev);
	fd = open(buf, OREAD);
	close(clone);
	if(fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;

	ptr = strchr(buf, ' ');
	if(ptr)
		*ptr = 0;

	parseip(to, buf);
	return 0;
}

#define CLASS(p) ((*(uchar*)(p))>>6)
static void
parseip(uchar *to, char *from)
{
	int i;
	char *p;

	p = from;
	memset(to, 0, 4);
	for(i = 0; i < 4 && *p; i++){
		to[i] = strtoul(p, &p, 0);
		if(*p == '.')
			p++;
	}

	switch(CLASS(to)){
	case 0:	/* class A - 1 byte net */
	case 1:
		if(i == 3){
			to[3] = to[2];
			to[2] = to[1];
			to[1] = 0;
		} else if (i == 2){
			to[3] = to[1];
			to[1] = 0;
		}
		break;
	case 2:	/* class B - 2 byte net */
		if(i == 3){
			to[3] = to[2];
			to[2] = 0;
		}
		break;
	}
}

static int
myetheraddr(uchar *to, char *dev)
{
	char buf[256];
	int n, fd;
	char *ptr;

	sprint(buf, "%s/1/stats", dev);
	fd = open(buf, OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;

	ptr = strstr(buf, "addr: ");
	if(!ptr)
		return -1;
	ptr += 6;

	parseether(to, ptr);
	return 0;
}

static int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

static int
mygetfields(char *lp, char **fields, int n)
{
	int i;

	for(i=0; lp && *lp && i<n; i++){
		while(*lp == ' ' || *lp == '\t')
			*lp++=0;
		if(*lp == 0)
			break;
		fields[i]=lp;
		while(*lp && *lp != ' ' && *lp != '\t')
			lp++;
	}
	return i;
}

static char*
fmtaddr(uchar *a)
{
	static char buf[32];

	sprint(buf, "%d.%d.%d.%d", a[0], a[1], a[2], a[3]);
	return buf;
}

static void
catchint(void *a, char *note)
{
	USED(a);
	if(strstr(note, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

static void
maskip(uchar *a, uchar *m, uchar *n)
{
	int i;

	for(i = 0; i < 4; i++)
		n[i] = a[i] & m[i];
}

static int
equivip(uchar *a, uchar *b)
{
	int i;

	for(i = 0; i < 4; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}
