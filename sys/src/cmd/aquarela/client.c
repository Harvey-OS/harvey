#include <u.h>
#include <libc.h>
#include <ip.h>
#include <thread.h>
#include "netbios.h"

static char *hmsg = "headers";
int chatty = 1;

void
warning(char *fmt, ...)
{
	char err[128];
	va_list arg;

	va_start(arg, fmt);
	vseprint(err, err+sizeof(err), fmt, arg);
	va_end(arg);
	syslog(1, "netbios-ns", err);
	if (chatty)
		print("%s\n", err);
}

static int
udpannounce(void)
{
	int data, ctl;
	char dir[64];
	char datafile[64+6];

	/* get a udp port */
	ctl = announce("udp!*!netbios-ns", dir);
	if(ctl < 0){
		warning("can't announce on netbios-ns udp port");
		return -1;
	}
	snprint(datafile, sizeof(datafile), "%s/data", dir);

	/* turn on header style interface */
	if(write(ctl, hmsg, strlen(hmsg)) , 0)
		abort(); /* hmsg */;
	data = open(datafile, ORDWR);
	if(data < 0){
		close(ctl);
		warning("can't announce on dns udp port");
		return -1;
	}

	close(ctl);
	return data;
}

#define BROADCAST 1


void
listen137(void *)
{	
	for (;;) {
		uchar msg[Udphdrsize + 576];
		int len = read(fd137, msg, sizeof(msg));
		if (len < 0)
			break;
		if (len >= Udphdrsize) {
			NbnsMessage *s;
			Udphdr *uh;
			uchar *p;

			uh = (Udphdr*)msg;
			p = msg + Udphdrsize;
			len -= Udphdrsize;
			s = nbnsconvM2S(p, len);
			if (s) {
				print("%I:%d -> %I:%d\n", uh->raddr, nhgets(uh->rport), uh->laddr, nhgets(uh->lport));
				nbnsdumpmessage(s);
				if (s->response) {
					NbnsTransaction *t;
					qlock(&transactionlist);
					for (t = transactionlist.head; t; t = t->next)
						if (t->id == s->id)
							break;
					if (t) {
						sendp(t->c, s);
					}
					else
						nbnsmessagefree(&s);
					qunlock(&transactionlist);
				}
				else
					nbnsmessagefree(&s);
			}
		}
	}
}

void
usage(void)
{
	fprint(2, "usage: %s [-u ipaddr] name\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char **argv)
{
	int broadcast = 1, i, listen137thread, rv;
	char *ip;
	uchar ipaddr[IPaddrlen], serveripaddr[IPaddrlen];
	NbName nbname;

	ARGBEGIN {
	case 'u':
		broadcast = 0;
		ip = EARGF(usage());
		if (parseip(serveripaddr, ip) == -1)
			sysfatal("bad ip address %s", ip);
		break;
	default:
		usage();
	} ARGEND;

	if (argc == 0)
		usage();

	nbmknamefromstring(nbname, argv[0]);

	ipifc = readipifc("/net", nil, 0);
	if (ipifc == nil || ipifc->lifc == nil)
		sysfatal("no network interface");
	fmtinstall('I', eipfmt);
	ipmove(bcastaddr, ipifc->lifc->ip);
	for (i = 0; i < IPaddrlen; i++)
		bcastaddr[i] |= ~ipifc->lifc->mask[i];
	print("broadcasting to %I\n", bcastaddr);
//	setnetmtpt("/net");
	fd137 = udpannounce();
	listen137thread = proccreate(listen137, nil, 16384);
	rv = nbnsaddname(broadcast ? nil : serveripaddr, nbname, 3000, ipifc->lifc->ip);
	if (rv != 0)
		print("error code %d\n", rv);
	else
		print("%I\n", ipaddr);
	nbnsalarmend();
	threadint(listen137thread);
}
