#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include "arp.h"
#include "bootp.h"

int 	dbg;
int	bootreq;
int	bootctl;
char	*sysname;
Ipinfo	myiip;
Ndb	*db;
char	blog[] = "ipboot";

typedef struct Boottab Boottab;
struct Boottab
{
	Ipinfo;
	uchar	reply[4];		/* reply address */
};

void		openlisten(void);
void		fatal(int, char*, ...);
void		warning(int, char*, ...);
void		doreply(Bootp*, Boottab*);
Boottab*	lookup(Bootp*, Udphdr*);

#define TFTP "/lib/tftpd"

void
main(int argc, char **argv)
{
	Bootp *boot;
	Udphdr *up;
	Boottab *f;
	char *file;
	char buf[2*1024];
	uchar myip[4];

	ARGBEGIN{
	case 'd':
		dbg++;
		break;
	default:
		fprint(2, "usage: bootp [-d]\n");
		exits("usage");
	}ARGEND
	USED(argc); USED(argv);

	fmtinstall('E', eipconv);
	fmtinstall('I', eipconv);

	/* who am i? */
	sysname = getenv("sysname");
	if(sysname == 0)
		fatal(0, "I must know my name!");
	if(myipaddr(myip, "/net/udp") < 0)
		fatal(0, "I must know my ip addr!");

	/* get local info from the database */
	db = ndbopen(0);
	if(db == 0)
		fatal(0, "can't open database\n");
	sprint(buf, "%I", myip);
	if(ipinfo(db, 0, buf, 0, &myiip) < 0)
		fatal(0, "I'm not in network database!");

	syslog(dbg, blog, "started");

	switch(rfork(RFNOTEG|RFPROC|RFFDG)) {
	case -1:
		fatal(1, "fork");
	case 0:
		break;
	default:
		exits(0);
	}

	openlisten();
	chdir(TFTP);

	for(;;) {
		read(bootreq, buf, sizeof(buf));
		up = (Udphdr*)buf;
		boot = (Bootp*)(buf+Udphdrsize);
		if(boot->op != Bootrequest)
			continue;
		if(boot->htype != 1){
			syslog(dbg, blog, "not ethernet %E\n", boot->chaddr);
			continue;
		}

		/*
		 *  Answer the request if the client didn't specify another
		 *  server and either we know the bootfile or the client
		 *  specified one.
		 */
		if(boot->sname[0]==0 || strcmp(boot->sname, sysname)==0){
			f = lookup(boot, up);
			file = boot->file;
			if(*file == 0 && f)
				file = f->bootf;

			/*
			 *  unless we're specificly targeted or have the
			 *  boot file, don't answer
			 */
			if(boot->sname[0]==0 && (*file == 0 || access(file, 4) < 0))
				continue;

			if(f && file[0]){
				doreply(boot, f);
			} else {
				if(f)
					syslog(dbg, blog, "no bootfile for %I %E",
						f->ipaddr, f->etheraddr);
				else
					syslog(dbg, blog, "no entry for %I %E",
						boot->ciaddr, boot->chaddr); 
			}
		}
	}
}

/*
 *  lookup info about a client in the database.  The client may have supplied
 *  an hardware address or an ip adress and a hardware address.
 */
Boottab*
lookup(Bootp *b, Udphdr *up)
{
	static Boottab bt;
	char ether[32];
	char ip[32];
	uchar x[4], y[4], z[4];
	Ndbtuple *t, *nt;
	Ndbs s;
	int i, isether;

	memset(&bt, 0, sizeof(Boottab));

	isether = 0;
	for(i = 0; i < sizeof(b->chaddr); i++)
		isether |= b->chaddr[i];
	sprint(ether, "%E", b->chaddr);
	sprint(ip, "%I", b->ciaddr);
	if(ipinfo(db, isether ? ether : 0, *b->ciaddr ? ip : 0, 0, &bt) < 0)
		return 0;

	maskip(up->ipaddr, bt.ipmask, x);
	if(*up->ipaddr != 0 && memcmp(x, bt.ipnet, sizeof(bt.ipnet)) != 0){
		/*
		 *  the request comes from a different net or subnet than
		 *  the ip address we found, look for a different answer
		 *  that matches both ether address and network.
		 */
		t = ndbsearch(db, &s, "ether", ether);
		while(t){
			for(nt = t; nt; nt = nt->entry){
				if(strcmp(nt->attr, "ip") == 0){
					parseip(y, nt->val);
					maskip(y, bt.ipmask, z);
					if(memcmp(x, z, sizeof(x)) == 0){
						ipinfo(db, 0, nt->val, 0, &bt);
						break;
					}
				}
			}
			ndbfree(t);
			if(nt)
				break;
			t = ndbsnext(&s, "ether", ether);
		}
	}
		

	/* it knows who it it, reply directly */
	memmove(bt.reply, bt.ipaddr, sizeof(bt.reply));

	return &bt;
}

void
arpenter(uchar *ip, uchar *ether)
{
	Arpentry a;
	int f;

	/* plan 9 */
	f = open("#a/arp/data", OWRITE);
	if(f >= 0){
		memmove(a.etaddr, ether, sizeof(a.etaddr));
		memmove(a.ipaddr, ip, sizeof(a.ipaddr));
		if(write(f, &a, sizeof(a)) < 0)
			syslog(dbg, blog, "write arp: %r\n");
		close(f);
		return;
	}

	/* brazil */
	f = open("/net/arp", OWRITE);
	if(f >= 0){
		fprint(f, "add %I %E", ip, ether);
		close(f);
		return;
	}

	syslog(dbg, blog, "open arp: %r\n");
}

void
doreply(Bootp *boot, Boottab *tab)
{
	Udphdr *up;
	Bootp *rp;
	char buf[2*1024];
	int i, n;

	up = (Udphdr*)buf;
	rp = (Bootp*)(buf+Udphdrsize);
	boot->sname[sizeof(boot->sname)-1] = '\0';

	memmove(rp, boot, sizeof(Bootp));
	rp->op = Bootreply;

	/* the client always best know his ether address */
	n = 0;
	for(i = 0; i < sizeof(boot->chaddr); i++)
		n |= boot->chaddr[i];
	if(n){
		if(tab)
			arpenter(tab->ipaddr, boot->chaddr);
 		else
			arpenter(boot->ciaddr, boot->chaddr);
	}

	/* but we best know the ip addresses */
	if(tab)
		memmove(rp->yiaddr, tab->ipaddr, sizeof(tab->ipaddr));
	else
		memmove(rp->yiaddr, boot->ciaddr, sizeof(rp->yiaddr));
	memmove(rp->siaddr, myiip.ipaddr, sizeof(myiip.ipaddr));
	if(tab)
		memmove(rp->giaddr, tab->gwip, sizeof(rp->giaddr));
	else
		memset(rp->giaddr, 0, sizeof(rp->giaddr));

	strcpy(rp->sname, sysname);
	if(boot->file[0] != '\0') {
		if(boot->file[0] != '/')
			sprint(rp->file, "%s/%s", TFTP, boot->file);
	} else if(tab)
		strcpy(rp->file, tab->bootf);
	else
		strcpy(rp->file, "no file");

	if(tab && strncmp(rp->vend, "p9  ", 4) == 0)
		sprint(rp->vend, "p9  %I %I %I %I", tab->ipmask, tab->fsip,
			tab->auip, tab->gwip);

	syslog(dbg, blog, "reply to %I %E %s %s", rp->yiaddr, boot->chaddr,
		rp->file, rp->vend);

	/*
	 *  create reply address
	 */
	if(tab)
		memmove(up->ipaddr, tab->reply, sizeof(up->ipaddr));
	else
		memmove(up->ipaddr, rp->yiaddr, sizeof(up->ipaddr));
	up->port[0] = 0;
	up->port[1] = 68;

	/*
	 *  send the reply
	 */
	n = write(bootreq, buf, sizeof(Bootp)+Udphdrsize);
	if(n < 0)
		warning(1, "can't reply to %I", up->ipaddr);
}

void
openlisten(void)
{
	char data[128];
	char devdir[40];

	bootctl = announce("udp!*!bootp", devdir);
	if(bootctl < 0)
		fatal(1, "can't announce");
	if(write(bootctl, "headers", sizeof("headers")) < 0)
		fatal(1, "can't set header mode");

	sprint(data, "%s/data", devdir);

	bootreq = open(data, ORDWR);
	if(bootreq < 0)
		fatal(1, "open udp data");
}

void
fatal(int syserr, char *fmt, ...)
{
	char buf[ERRLEN], sysbuf[ERRLEN];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(syserr) {
		errstr(sysbuf);
		fprint(2, "bootp: %s: %s\n", buf, sysbuf);
	}
	else
		fprint(2, "bootp: %s\n", buf);
	exits(buf);
}

void
warning(int syserr, char *fmt, ...)
{
	char buf[ERRLEN], sysbuf[ERRLEN];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	if(syserr) {
		errstr(sysbuf);
		fprint(2, "bootp: %s: %s\n", buf, sysbuf);
	}
	else
		fprint(2, "bootp: %s\n", buf);
}
