#include "vnc.h"
#include "vncv.h"

char*		encodings = "copyrect hextile corre rre raw mousewarp";
int		bpp12;
int		shared;
int		verbose;
Vnc*		vnc;
int		mousefd;

static int	vncstart(Vnc*, int);
static int	vncnegotiate(Vnc*);
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
	if(p = strchr(server, ':')) {
		*p++ = '\0';
		port += atoi(p);
	}

	snprint(portstr, sizeof portstr, "%d", port);
	p = netmkaddr(server, nil, portstr);
	free(server);
	return p;
}

void
vnchungup(Vnc *)
{
	sysfatal("connection closed");
}

void
usage(void)
{
	fprint(2, "usage: vncv [-e encodings] [-csv] host[:n]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int p, dfd, cfd, shared;
	char *addr;
	Point d;

	shared = 0;
	ARGBEGIN{
	case 'c':
		bpp12 = 1;
		break;
	case 'e':
		encodings = ARGF();
		break;
	case 's':
		shared = 1;
		break;
	case 'v':
		verbose = 1;
		break;
	default:
		usage();
	}ARGEND;

	if(argc != 1)
		usage();

	addr = netmkvncaddr(argv[0]);
	dfd = dial(addr, nil, nil, &cfd);
	if(dfd < 0)
		sysfatal("cannot dial %s: %r", addr);

	vnc = vncinit(dfd, cfd, nil);

	if(vnchandshake(vnc) < 0)
		sysfatal("handshake failure: %r");
	if(vnc->extended){
		if(vncnegotiate(vnc) < 0)
			sysfatal("negotiation failure: %r");
	}else{
		if(vncauth(vnc) < 0)
			sysfatal("authentication failure: %r");
	}
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

	if( access("/dev/snarf", AEXIST) == 0 ) {
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
clienttlscrypt(Vnc *v)
{
	int p[2], netfd;

	if(pipe(p) < 0){
		fprint(2, "pipe: %r\n");
		return -1;
	}

	netfd = v->datafd;
	switch(rfork(RFMEM|RFPROC|RFFDG|RFNOWAIT)){
	case -1:
		fprint(2, "fork: %r\n");
		return -1;
	case 0:
		dup(p[0], 0);
		dup(netfd, 1);
		close(v->ctlfd);
		close(netfd);
		close(p[0]);
		close(p[1]);
		execl("/bin/tlsrelay", "tlsrelay", nil);
		fprint(2, "exec: %r\n");
		_exits(nil);
	default:
		close(netfd);
		v->datafd = p[1];
		close(p[0]);
		Binit(&v->in, v->datafd, OREAD);
		Binit(&v->out, v->datafd, OWRITE);
		return 0;
	}
}

static int
clientnocrypt(Vnc*)
{
	return 0;
}

static int
clientvncauth(Vnc *v)
{
	return vncauth(v);
}

static int
clientnoauth(Vnc*)
{
	return 0;
}

typedef struct Option Option;
typedef struct Choice Choice;

struct Choice {
	char *s;
	int (*fn)(Vnc*);
};

struct Option {
	char *s;
	Choice *c;
	int nc;
};

static Choice clientcryptchoice[] = {
	"tlsv1",	clienttlscrypt,
	"none",	clientnocrypt,
};

static Choice clientauthchoice[] = {
	"vnc",	clientvncauth,
	"none",	clientnoauth,
};

static Option options[] = {
	"crypt",	clientcryptchoice,	nelem(clientcryptchoice),
	"auth",	clientauthchoice,	nelem(clientauthchoice),
};

static int
vncnegotiate(Vnc *v)
{
	char *optstr, reply[256], *f[16];
	int i, j, nf;
	Option *o;

	for(;;){
		optstr = vncrdstringx(v);
		if(strcmp(optstr, "start") == 0)
			return 0;
		nf = getfields(optstr, f, nelem(f), 0, " ");
		if(nf == nelem(f)){
			fprint(2, "too many choices for %s\n", f[0]);
			return -1;
		}
		for(i=0; i<nelem(options); i++)
			if(strcmp(f[0], options[i].s) == 0)
				break;
		if(i==nelem(options)){
			fprint(2, "unknown option %s\n", f[0]);
			vncwrstring(v, "error unknown option");
			vncflush(v);
			free(optstr);
			continue;
		}
		o = &options[i];
		for(i=0; i<o->nc; i++)
			for(j=1; j<nf; j++)
				if(strcmp(o->c[i].s, f[j]) == 0)
					goto Break2;
		fprint(2, "no common choice for %s\n", f[0]);
		vncwrstring(v, "error no common choice");
		vncflush(v);
		free(optstr);
		continue;
Break2:
		snprint(reply, sizeof reply, "%s %s", f[0], f[j]);
		vncwrstring(v, reply);
		vncflush(v);
		if((*o->c[i].fn)(v) < 0)
			fprint(2, "%s %s failed: %r\n", f[0], f[j]);
		free(optstr);
	}
	abort();
	return -1;
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
