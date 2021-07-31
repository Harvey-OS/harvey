#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>

#define NS(x)		((vlong)x)
#define US(x)		(NS(x) * 1000LL)
#define MS(x)		(US(x) * 1000LL)
#define S(x)		(MS(x) * 1000LL)

#define LOGNAME	"aan"

enum {
	Synctime = S(8),
	Nbuf = 10,
	K = 1024,
	Bufsize = 8 * K,
	Stacksize = 8 * K,
	Timer = 0,					// Alt channels.
	Unsent = 1,
	Maxto = 24 * 3600,			// A full day to reconnect.
};

typedef struct Endpoints Endpoints;
struct Endpoints {
	char	*lsys;
	char	*lserv;
	char	*rsys;
	char	*rserv;
};

typedef struct {
	ulong		nb;		// Number of data bytes in this message
	ulong		msg;		// Message number
	ulong		acked;	// Number of messages acked
} Hdr;

typedef struct t_Buf {
	Hdr			hdr;
	uchar		buf[Bufsize];
} Buf;

static char 	*progname;
static Channel	*unsent;
static Channel	*unacked;
static Channel	*empty;
static int		netfd;
static int		inmsg;
static char	*devdir;
static int		debug;
static int		done;
static char	*dialstring;
static int		maxto = Maxto;
static char	*Logname = LOGNAME;
static int		client;

static Alt a[] = {
	/*	c	v	 op   */
	{ 	nil,	nil,	CHANRCV			},	// timer
	{	nil,	nil,	CHANRCV			},	// unsent
	{ 	nil,	nil,	CHANEND		},
};

static void		fromnet(void*);
static void		fromclient(void*);
static void		reconnect(void);
static void		synchronize(void);
static int 		sendcommand(ulong, ulong);
static void		showmsg(int, char *, Buf *);
static int		writen(int, uchar *, int);
static int		getport(char *);
static void		dmessage(int, char *, ...);
static void		timerproc(void *);
static Endpoints *getendpoints(char *);
static void		freeendpoints(Endpoints *);

static void
usage(void)
{
	fprint(2, "Usage: %s [-cd] [-m maxto] dialstring|netdir\n", progname);
	exits("usage");
}

static int
catch(void *, char *s)
{
	if (!strcmp(s, "alarm")) {
		syslog(0, Logname, "Timed out while waiting for client on %s, exiting...",
			   devdir);
		threadexitsall(nil);
	}
	return 0;
}

void
threadmain(int argc, char **argv)
{
	int i, failed;
	Buf *b;
	Channel *timer;
	vlong synctime;

	progname = argv[0];
	ARGBEGIN {
	case 'c':
		client++;
		break;
	case 'd':
		debug++;
		break;
	case 'm':
		maxto = (int)strtol(EARGF(usage()), (char **)nil, 0);
		break;
	default:
		usage();
	} ARGEND;

	if (argc != 1)
		usage();

	if (!client) {
		char *p;

		devdir = argv[0];
		if ((p = strstr(devdir, "/local")) != nil)
			*p = '\0';
	}
	else
		dialstring = argv[0];

	if (debug > 0) {
		int fd = open("#c/cons", OWRITE|OCEXEC);	
		dup(fd, 2);
	}

	fmtinstall('F', fcallfmt);

	atnotify(catch, 1);

	unsent = chancreate(sizeof(Buf *), Nbuf);
	unacked = chancreate(sizeof(Buf *), Nbuf);
	empty = chancreate(sizeof(Buf *), Nbuf);
	timer = chancreate(sizeof(uchar *), 1);

	for (i = 0; i != Nbuf; i++) {
		Buf *b = malloc(sizeof(Buf));
		sendp(empty, b);
	}

	netfd = -1;

	if (proccreate(fromnet, nil, Stacksize) < 0)
		sysfatal("%s; Cannot start fromnet; %r", progname);

	reconnect();		// Set up the initial connection.
	synchronize();

	if (proccreate(fromclient, nil, Stacksize) < 0)
		sysfatal("%s; Cannot start fromclient; %r", progname);

	if (proccreate(timerproc, timer, Stacksize) < 0)
		sysfatal("%s; Cannot start timerproc; %r", progname);

	a[Timer].c = timer;
	a[Unsent].c = unsent;
	a[Unsent].v = &b;

	synctime = nsec() + Synctime;
	failed = 0;
	while (!done) {
		vlong now;
		int delta;

		if (failed) {
			// Wait for the netreader to die.
			while (netfd >= 0) {
				dmessage(1, "main; waiting for netreader to die\n");
				sleep(1000);
			}

			// the reader died; reestablish the world.
			reconnect();
			synchronize();
			failed = 0;
		}

		now = nsec();
		delta = (synctime - nsec()) / MS(1);

		if (delta <= 0) {
			Hdr hdr;

			hdr.nb = 0;
			hdr.acked = inmsg;
			hdr.msg = -1;

			if (writen(netfd, (uchar *)&hdr, sizeof(Hdr)) < 0) {
				dmessage(2, "main; writen failed; %r\n");
				failed = 1;
				continue;
			}
			synctime = nsec() + Synctime;
			assert(synctime > now);
		}

		switch (alt(a)) {
		case Timer:
			break;

		case Unsent:
			sendp(unacked, b);

			b->hdr.acked = inmsg;

			if (writen(netfd, (uchar *)&b->hdr, sizeof(Hdr)) < 0) {
				dmessage(2, "main; writen failed; %r\n");
				failed = 1;
			}

			if (writen(netfd, b->buf, b->hdr.nb) < 0) {
				dmessage(2, "main; writen failed; %r\n");
				failed = 1;
			}

			if (b->hdr.nb == 0)
				done = 1;
			break;
		}
	}
	syslog(0, Logname, "exiting...");
	threadexitsall(nil);
}


static void
fromclient(void*)
{
	static int outmsg;

	for (;;) {
		Buf *b;

		b = recvp(empty);	
		if ((int)(b->hdr.nb = read(0, b->buf, Bufsize)) <= 0) {
			if ((int)b->hdr.nb < 0)
				dmessage(2, "fromclient; Cannot read 9P message; %r\n");
			else
				dmessage(2, "fromclient; Client terminated\n");
			b->hdr.nb = 0;
		}
		b->hdr.msg = outmsg++;

		showmsg(1, "fromclient", b);
		sendp(unsent, b);
		
		if (b->hdr.nb == 0)
			break;
	}
}

static void
fromnet(void*)
{
	static int lastacked;
	Buf *b;

	b = (Buf *)malloc(sizeof(Buf));
	assert(b);

	while (!done) {
		int len, acked, i;

		while (netfd < 0) {
			dmessage(1, "fromnet; waiting for connection... (inmsg %d)\n", 
					  inmsg);
			sleep(1000);
		}

		// Read the header.
		if ((len = readn(netfd, &b->hdr, sizeof(Hdr))) <= 0) {
			if (len < 0)
				dmessage(1, "fromnet; (hdr) network failure; %r\n");
			else
				dmessage(1, "fromnet; (hdr) network closed\n");
			close(netfd);
			netfd = -1;
			continue;
		}
		dmessage(2, "fromnet: Got message, size %d, nb %d, msg %d\n", len,
				b->hdr.nb, b->hdr.msg);

		if (b->hdr.nb == 0) {
			if  ((long)b->hdr.msg >= 0) {
				dmessage(1, "fromnet; network closed\n");
				break;
			}
			continue;
		}
	
		if ((len = readn(netfd, b->buf, b->hdr.nb)) <= 0 || len != b->hdr.nb) {
			if (len == 0)
				dmessage(1, "fromnet; network closed\n");
			else
				dmessage(1, "fromnet; network failure; %r\n");
			close(netfd);
			netfd = -1;
			continue;
		}

		if (b->hdr.msg < inmsg) {
			dmessage(1, "fromnet; skipping message %d, currently at %d\n",
					 b->hdr.msg, inmsg);
			continue;
		}			

		// Process the acked list.
		acked = b->hdr.acked - lastacked;
		for (i = 0; i != acked; i++) {
			Buf *rb;

			rb = recvp(unacked);
			if (rb->hdr.msg != lastacked + i) {
				dmessage(1, "rb %p, msg %d, lastacked %d, i %d\n",
						rb, rb? rb->hdr.msg: -2, lastacked, i);
				assert(0);
			}
			rb->hdr.msg = -1;
			sendp(empty, rb);
		} 
		lastacked = b->hdr.acked;

		inmsg++;

		showmsg(1, "fromnet", b);

		if (writen(1, b->buf, len) < 0) 
			sysfatal("fromnet; cannot write to client; %r");
	}
	done = 1;
}

static void
reconnect(void)
{
	char ldir[40];
	int lcfd, fd;

	if (dialstring) {
		syslog(0, Logname, "dialing %s", dialstring);
  		while ((fd = dial(dialstring, nil, nil, nil)) < 0) {
			char err[32];

			err[0] = '\0';
			errstr(err, sizeof err);
			if (strstr(err, "connection refused")) {
				dmessage(1, "reconnect; server died...\n");
				threadexitsall("server died...");
			}
			dmessage(1, "reconnect: dialed %s; %s\n", dialstring, err);
			sleep(1000);
		}
		syslog(0, Logname, "reconnected to %s", dialstring);
	} 
	else {
		Endpoints *ep;

		syslog(0, Logname, "waiting for connection on %s", devdir);
		alarm(maxto * 1000);
 		if ((lcfd = listen(devdir, ldir)) < 0) 
			sysfatal("reconnect; cannot listen; %r");
	
		if ((fd = accept(lcfd, ldir)) < 0)
			sysfatal("reconnect; cannot accept; %r");
		alarm(0);
		close(lcfd);
		
		ep = getendpoints(ldir);
		dmessage(1, "rsys '%s'\n", ep->rsys);
		syslog(0, Logname, "connected from %s", ep->rsys);
		freeendpoints(ep);
	}
	
	netfd = fd;		// Wakes up the netreader.
}

static void
synchronize(void)
{
	Channel *tmp;
	Buf *b;

	// Ignore network errors here.  If we fail during 
	// synchronization, the next alarm will pick up 
	// the error.

	tmp = chancreate(sizeof(Buf *), Nbuf);
	while ((b = nbrecvp(unacked)) != nil) {
		writen(netfd, (uchar *)b, sizeof(Hdr) + b->hdr.nb);
		sendp(tmp, b);
	}
	chanfree(unacked);
	unacked = tmp;
}

static void
showmsg(int level, char *s, Buf *b)
{
	if (b == nil) {
		dmessage(level, "%s; b == nil\n", s);
		return;
	}

	dmessage(level, 
			"%s;  (len %d) %X %X %X %X %X %X %X %X %X (%p)\n", s, 
		  	b->hdr.nb, 
		  	b->buf[0], b->buf[1], b->buf[2],
		  	b->buf[3], b->buf[4], b->buf[5],
		  	b->buf[6], b->buf[7], b->buf[8], b);
}

static int
writen(int fd, uchar *buf, int nb)
{
	int len = nb;

	while (nb > 0) {
		int n;

		if (fd < 0) 
			return -1;

		if ((n = write(fd, buf, nb)) < 0) {
			dmessage(1, "writen; Write failed; %r\n");
			return -1;
		}
		dmessage(2, "writen: wrote %d bytes\n", n);

		buf += n;
		nb -= n;
	}
	return len;
}

static void
timerproc(void *x)
{
	Channel *timer = x;
	while (!done) {
		sleep((Synctime / MS(1)) >> 1);
		sendp(timer, "timer");
	}
}

static void
dmessage(int level, char *fmt, ...)
{
	va_list arg; 

	if (level > debug) 
		return;

	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
}

static void
getendpoint(char *dir, char *file, char **sysp, char **servp)
{
	int fd, n;
	char buf[128];
	char *sys, *serv;

	sys = serv = 0;

	snprint(buf, sizeof buf, "%s/%s", dir, file);
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv){
				*serv++ = 0;
				serv = strdup(serv);
			}
			sys = strdup(buf);
		}
		close(fd);
	}
	if(serv == 0)
		serv = strdup("unknown");
	if(sys == 0)
		sys = strdup("unknown");
	*servp = serv;
	*sysp = sys;
}

static Endpoints *
getendpoints(char *dir)
{
	Endpoints *ep;

	ep = malloc(sizeof(*ep));
	getendpoint(dir, "local", &ep->lsys, &ep->lserv);
	getendpoint(dir, "remote", &ep->rsys, &ep->rserv);
	return ep;
}

static void
freeendpoints(Endpoints *ep)
{
	free(ep->lsys);
	free(ep->rsys);
	free(ep->lserv);
	free(ep->rserv);
	free(ep);
}

