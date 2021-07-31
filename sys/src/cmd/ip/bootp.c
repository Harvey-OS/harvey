#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <ndb.h>
#include "/sys/src/9/port/arp.h"
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
Boottab*	lookup(Bootp*);

void
main(int argc, char **argv)
{
	Bootp *boot;
	Boottab *f;
	char buf[1024];
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

	for(;;) {
		read(bootreq, buf, sizeof(buf));
		boot = (Bootp*)buf;
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
			f = lookup(boot);
			if(f && (f->bootf[0] || boot->file[0]!=0)){
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
lookup(Bootp *b)
{
	static Boottab bt;
	char ether[32];
	char ip[32];

	memset(&bt, 0, sizeof(Boottab));

	sprint(ether, "%E", b->chaddr);
	sprint(ip, "%I", b->ciaddr);
	if(ipinfo(db, ether , *ip ? ip : 0, 0, &bt) < 0)
		return 0; 

	/* it knows who it it, reply directly */
	memmove(bt.reply, bt.ipaddr, sizeof(bt.reply));

	return &bt;
}

void
arpenter(uchar *ip, uchar *ether)
{
	Arpentry a;
	int f;

	memmove(a.etaddr, ether, sizeof(a.etaddr));
	memmove(a.ipaddr, ip, sizeof(a.ipaddr));

	f = open("#a/arp/data", OWRITE);
	if(f < 0) {
		syslog(dbg, blog, "open arp: %r\n");
		return;
	}
	if(write(f, &a, sizeof(a)) < 0)
		syslog(dbg, blog, "write arp: %r\n");
	close(f);
}

void
doreply(Bootp *boot, Boottab *tab)
{
	Bootp reply;
	char buf[128];
	int n;

	boot->sname[sizeof(boot->sname)-1] = '\0';

	memmove(&reply, boot, sizeof(reply));
	reply.op = Bootreply;

	/* the client always best know his ether address */
	if(tab)
		arpenter(tab->ipaddr, boot->chaddr);
 	else
		arpenter(boot->ciaddr, boot->chaddr);

	/* but we best know the ip addresses */
	if(tab)
		memmove(reply.yiaddr, tab->ipaddr, sizeof(tab->ipaddr));
	else
		memmove(reply.yiaddr, boot->ciaddr, sizeof(reply.yiaddr));
	memmove(reply.siaddr, myiip.ipaddr, sizeof(myiip.ipaddr));
	if(tab)
		memmove(reply.giaddr, tab->gwip, sizeof(reply.giaddr));
	else
		memset(reply.giaddr, 0, sizeof(reply.giaddr));

	strcpy(reply.sname, sysname);
	if(boot->file[0] != '\0') {
		if(boot->file[0] != '/')
			sprint(reply.file, "/lib/tftpd/%s", boot->file);
	}
	else if(tab)
		strcpy(reply.file, tab->bootf);
	else
		strcpy(reply.file, "no file");

	if(tab && strncmp(reply.vend, "p9  ", 4) == 0)
		sprint(reply.vend, "p9  %I %I %I %I", tab->ipmask, tab->fsip,
			tab->auip, tab->gwip);

	syslog(dbg, blog, "reply to %I %E %s %s", reply.yiaddr, boot->chaddr,
		reply.file, reply.vend);

	/*
	 *  get a connection (sort of) back to the client
	 */
	if(tab)
		sprint(buf, "connect %I!68", tab->reply);
	else
		sprint(buf, "connect %I!68", reply.yiaddr);
	if(write(bootctl, buf, strlen(buf)) < 0){
		warning(1, "can't %s", buf);
		return;
	}

	/*
	 *  send the reply
	 */
	n = write(bootreq, (void*)&reply, sizeof(reply));
	if(n < 0)
		warning(1, "can't reply to %s", buf);

	/*
	 *  clear
	 */
	sprint(buf, "connect 0.0.0.0!0");
	if(write(bootctl, buf, strlen(buf)) < 0){
		warning(1, "can't %s", buf);
		return;
	}
}

void
openlisten(void)
{
	char data[128];
	char devdir[40];

	bootctl = announce("udp!*!bootp", devdir);
	if(bootctl < 0)
		fatal(1, "can't announce");

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
