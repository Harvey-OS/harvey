#include "vnc.h"
#include "vncv.h"
#include <libsec.h>

char*	encodings = "copyrect hextile corre rre raw mousewarp";
int		bpp12;
int		shared;
int		verbose;
Vnc*		vnc;
int		mousefd;
int		tls;

static int	vncstart(Vnc*, int);

enum
{
	NProcs	= 4
};

static int pids[NProcs];
static char killkin[] = "die vnc kin";

/*
 * called by any proc when exiting to tear everything down.
 */
static void
shutdown(void)
{
	int i, pid;

	hangup(vnc->ctlfd);
	close(vnc->ctlfd);
	vnc->ctlfd = -1;
	close(vnc->datafd);
	vnc->datafd = -1;

	pid = getpid();
	for(i = 0; i < NProcs; i++)
		if(pids[i] != pid)
			postnote(PNPROC, pids[i], killkin);
}

char*
netmkvncaddr(char *inserver)
{
	char *p, portstr[NETPATHLEN], *server;
	int port;

	server = strdup(inserver);
	assert(server != nil);

	port = 5900;
	if(tls)
		port = 35729;
	if(p = strchr(server, ':')) {
		*p++ = '\0';
		port += atoi(p);
	}

	snprint(portstr, sizeof portstr, "%d", port);
	p = netmkaddr(server, "tcp", portstr);
	free(server);
	return p;
}

void
vnchungup(Vnc*)
{
	sysfatal("connection closed");
}

void
usage(void)
{
	fprint(2, "usage: vncv [-e encodings] [-k keypattern] [-csv] host[:n]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int p, fd, dfd, cfd, shared;
	char *keypattern, *addr;
	Point d;
	TLSconn conn;

	keypattern = nil;
	shared = 0;
	ARGBEGIN{
	case 'c':
		bpp12 = 1;
		break;
	case 'e':
		encodings = EARGF(usage());
		break;
	case 's':
		shared = 1;
		break;
	case 't':
		tls = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	case 'k':
		keypattern = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();

	addr = netmkvncaddr(argv[0]);
	serveraddr = argv[0];
	dfd = dial(addr, nil, nil, &cfd);
	if(dfd < 0)
		sysfatal("cannot dial %s: %r", addr);
	if(tls){
		dfd = tlsClient(dfd, &conn);
		if(dfd < 0)
			sysfatal("tlsClient: %r");
		/* XXX check thumbprint */
	}
	vnc = vncinit(dfd, cfd, nil);

	if(vnchandshake(vnc) < 0)
		sysfatal("handshake failure: %r");
	if(vncauth(vnc, keypattern) < 0)
		sysfatal("authentication failure: %r");
	if(vncstart(vnc, shared) < 0)
		sysfatal("init failure: %r");

	initdraw(0, 0, "vncv");
	display->locking = 1;
	unlockdisplay(display);

	d = addpt(vnc->dim, Pt(2*Borderwidth, 2*Borderwidth));
	if(verbose)
		fprint(2, "screen size %P, desktop size %P\n", display->image->r.max, d);

	choosecolor(vnc);
	sendencodings(vnc);
	initmouse();

	rfork(RFREND);
	atexit(shutdown);
	pids[0] = getpid();

	switch(p = rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("rfork: %r");
	default:
		break;
	case 0:
		atexit(shutdown);
		readfromserver(vnc);
		exits(nil);
	}
	pids[1] = p;

	switch(p = rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("rfork: %r");
	default:
		break;
	case 0:
		atexit(shutdown);
		checksnarf(vnc);
		exits(nil);
	}
	pids[2] = p;

	fd = open("/dev/label", OWRITE);
	if(fd >= 0){
		fprint(fd, "vnc %s", serveraddr);
		close(fd);
	}
	if(access("/dev/snarf", AEXIST) >= 0){
		switch(p = rfork(RFPROC|RFMEM)){
		case -1:
			sysfatal("rfork: %r");
		default:
			break;
		case 0:
			atexit(shutdown);
			readkbd(vnc);
			exits(nil);
		}
	}
	pids[3] = p;

	readmouse(vnc);
	exits(nil);
}

static int
vncstart(Vnc *v, int shared)
{
	vncwrchar(v, shared);
	vncflush(v);
	v->dim = vncrdpoint(v);
	v->Pixfmt = vncrdpixfmt(v);
	v->name = vncrdstring(v);
	return 0;
}
