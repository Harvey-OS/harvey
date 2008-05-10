#define	Image	IMAGE
#include "vnc.h"
#include "vncs.h"
#include "compat.h"
#include <cursor.h>
#include "screen.h"
#include "kbd.h"

#include <mp.h>
#include <libsec.h>

extern	Dev	drawdevtab;
extern	Dev	mousedevtab;
extern	Dev	consdevtab;

Dev	*devtab[] =
{
	&drawdevtab,
	&mousedevtab,
	&consdevtab,
	nil
};

static char *msgname[] = {
	[MPixFmt] = "MPixFmt",
	[MFixCmap] = "MFixCmap",
	[MSetEnc] = "MSetEnc",
	[MFrameReq] = "MFrameReq",
	[MKey] = "MKey",
	[MMouse] = "MMouse",
	[MCCut] = "MCCut",
};

static char *encname[] = {
	[EncRaw] = "raw",
	[EncCopyRect] = "copy rect",
	[EncRre] = "rre",
	[EncCorre] = "corre",
	[EncHextile] = "hextile",
	[EncZlib]	= "zlib",
	[EncTight]	= "zlibtight",
	[EncZHextile]	= "zhextile",
	[EncMouseWarp]	= "mousewarp",

};

/* 
 * list head. used to hold the list, the lock, dim, and pixelfmt
 */
struct {
	QLock;
	Vncs *head;
} clients;

int	shared;
int	sleeptime = 5;
int	verbose = 0;
char *cert;
char *pixchan = "r5g6b5";
static int	cmdpid;
static int	srvfd;
static int	exportfd;
static Vncs	**vncpriv;

static int parsedisplay(char*);
static void vnckill(char*, int, int);
static int vncannounce(char *net, int display, char *adir, int base);
static void noteshutdown(void*, char*);
static void vncaccept(Vncs*);
static int vncsfmt(Fmt*);
static void getremote(char*, char*);
static void vncname(char*, ...);
#pragma varargck argpos vncname 1

#pragma varargck type "V" Vncs*

void
usage(void)
{
	fprint(2, "usage: vncs [-v] [-c cert] [-d :display] [-g widthXheight] [-p pixelfmt] [cmd [args]...]\n");
	fprint(2, "\tto kill a server: vncs [-v] -k :display\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int altnet, baseport, cfd, display, exnum, fd, h, killing, w;
	char adir[NETPATHLEN], ldir[NETPATHLEN];
	char net[NETPATHLEN], *p;
	char *rc[] = { "/bin/rc", "-i", nil };
	Vncs *v;

	fmtinstall('V', vncsfmt);
	display = -1;
	killing = 0;
	altnet = 0;
	w = 1024;
	h = 768;
	baseport = 5900;
	setnetmtpt(net, sizeof net, nil);
	ARGBEGIN{
	default:
		usage();
	case 'c':
		cert = EARGF(usage());
		baseport = 35729;
		break;
	case 'd':
		if(display != -1)
			usage();
		display = parsedisplay(EARGF(usage()));
		break;
	case 'g':
		p = EARGF(usage());
		w = strtol(p, &p, 10);
		if(*p != 'x' && *p != 'X' && *p != ' ' && *p != '	')
			usage();
		h = strtol(p+1, &p, 10);
		if(*p != 0)
			usage();
		break;
	case 'k':
		if(display != -1)
			usage();
		display = parsedisplay(EARGF(usage()));
		killing = 1;
		break;
	case 'p':
		pixchan = EARGF(usage());
		break;
/* DEBUGGING
	case 's':
		sleeptime = atoi(EARGF(usage()));
		break;
*/
	case 'v':
		verbose++;
		break;
	case 'x':
		p = EARGF(usage());
		setnetmtpt(net, sizeof net, p);
		altnet = 1;
		break;
	}ARGEND

	if(killing){
		vnckill(net, display, baseport);
		exits(nil);
	}

	if(altnet && !cert)
		sysfatal("announcing on alternate network requires TLS (-c)");

	if(argc == 0)
		argv = rc;

	/* easy exit */
	if(access(argv[0], AEXEC) < 0)
		sysfatal("access %s for exec: %r", argv[0]);

	/* background ourselves */
	switch(rfork(RFPROC|RFNAMEG|RFFDG|RFNOTEG)){
	case -1:
		sysfatal("rfork: %r");
	default:
		exits(nil);
	case 0:
		break;
	}

	vncpriv = privalloc();
	if(vncpriv == nil)
		sysfatal("privalloc: %r");

	/* start screen */
	initcompat();
	if(waserror())
		sysfatal("screeninit %dx%d %s: %s", w, h, pixchan, up->error);
	if(verbose)
		fprint(2, "geometry is %dx%d\n", w, h);
	screeninit(w, h, pixchan);
	poperror();

	/* start file system device slaves */
	exnum = exporter(devtab, &fd, &exportfd);

	/* rebuild /dev because the underlying connection might go away (ick) */
	unmount(nil, "/dev");
	bind("#c", "/dev", MREPL);

	/* run the command */
	switch(cmdpid = rfork(RFPROC|RFFDG|RFNOTEG|RFNAMEG|RFREND)){
	case -1:
		sysfatal("rfork: %r");
		break;
	case 0:
		if(mounter("/dev", MBEFORE, fd, exnum) < 0)
			sysfatal("mounter: %r");
		close(exportfd);
		close(0);
		close(1);
		close(2);
		open("/dev/cons", OREAD);
		open("/dev/cons", OWRITE);
		open("/dev/cons", OWRITE);
		exec(argv[0], argv);
		fprint(2, "exec %s: %r\n", argv[0]);
		_exits(nil);
	default:
		close(fd);
		break;
	}

	/* run the service */
	srvfd = vncannounce(net, display, adir, baseport);
	if(srvfd < 0)
		sysfatal("announce failed");
	if(verbose)
		fprint(2, "announced in %s\n", adir);

	atexit(shutdown);
	notify(noteshutdown);
	for(;;){
		vncname("listener");
		cfd = listen(adir, ldir);
		if(cfd < 0)
			break;
		if(verbose)
			fprint(2, "call in %s\n", ldir);
		fd = accept(cfd, ldir);
		if(fd < 0){
			close(cfd);
			continue;
		}
		v = mallocz(sizeof(Vncs), 1);
		if(v == nil){
			close(cfd);
			close(fd);
			continue;
		}
		v->ctlfd = cfd;
		v->datafd = fd;
		v->nproc = 1;
		v->ndead = 0;
		getremote(ldir, v->remote);
		strcpy(v->netpath, ldir);
		qlock(&clients);
		v->next = clients.head;
		clients.head = v;
		qunlock(&clients);
		vncaccept(v);
	}
	exits(0);
}

static int
parsedisplay(char *p)
{
	int n;

	if(*p != ':')
		usage();
	if(*p == 0)
		usage();
	n = strtol(p+1, &p, 10);
	if(*p != 0)
		usage();
	return n;
}

static void
getremote(char *ldir, char *remote)
{
	char buf[NETPATHLEN];
	int fd, n;

	snprint(buf, sizeof buf, "%s/remote", ldir);
	strcpy(remote, "<none>");
	if((fd = open(buf, OREAD)) < 0)
		return;
	n = readn(fd, remote, NETPATHLEN-1);
	close(fd);
	if(n < 0)
		return;
	remote[n] = 0;
	if(n>0 && remote[n-1] == '\n')
		remote[n-1] = 0;
}

static int
vncsfmt(Fmt *fmt)
{
	Vncs *v;

	v = va_arg(fmt->args, Vncs*);
	return fmtprint(fmt, "[%d] %s %s", getpid(), v->remote, v->netpath);
}

/*
 * We register exiting as an atexit handler in each proc, so that 
 * client procs need merely exit when something goes wrong.
 */
static void
vncclose(Vncs *v)
{
	Vncs **l;

	/* remove self from client list if there */
	qlock(&clients);
	for(l=&clients.head; *l; l=&(*l)->next)
		if(*l == v){
			*l = v->next;
			break;
		}
	qunlock(&clients);

	/* if last proc, free v */
	vnclock(v);
	if(++v->ndead < v->nproc){
		vncunlock(v);
		return;
	}

	freerlist(&v->rlist);
	vncterm(v);
	if(v->ctlfd >= 0)
		close(v->ctlfd);
	if(v->datafd >= 0)
		close(v->datafd);
	if(v->image)
		freememimage(v->image);
	free(v);
}

static void
exiting(void)
{
	vncclose(*vncpriv);
}

void
vnchungup(Vnc *v)
{
	if(verbose)
		fprint(2, "%V: hangup\n", (Vncs*)v);
	exits(0);	/* atexit and exiting() will take care of everything */
}

/*
 * Kill all clients except safe.
 * Used to start a non-shared client and at shutdown. 
 */
static void
killclients(Vncs *safe)
{
	Vncs *v;

	qlock(&clients);
	for(v=clients.head; v; v=v->next){
		if(v == safe)
			continue;
		if(v->ctlfd >= 0){
			hangup(v->ctlfd);
			close(v->ctlfd);
			v->ctlfd = -1;
			close(v->datafd);
			v->datafd = -1;
		}
	}
	qunlock(&clients);
}

/*
 * Kill the executing command and then kill everyone else.
 * Called to close up shop at the end of the day
 * and also if we get an unexpected note.
 */
static char killkin[] = "die vnc kin";
static void
killall(void)
{
	postnote(PNGROUP, cmdpid, "hangup");
	close(srvfd);
	srvfd = -1;
	close(exportfd);
	exportfd = -1;
	postnote(PNGROUP, getpid(), killkin);
}

void
shutdown(void)
{
	if(verbose)
		fprint(2, "vnc server shutdown\n");
	killall();
}

static void
noteshutdown(void*, char *msg)
{
	if(strcmp(msg, killkin) == 0)	/* already shutting down */
		noted(NDFLT);
	killall();
	noted(NDFLT);
}

/*
 * Kill a specific instance of a server.
 */
static void
vnckill(char *net, int display, int baseport)
{
	int fd, i, n, port;
	char buf[NETPATHLEN], *p;

	for(i=0;; i++){
		snprint(buf, sizeof buf, "%s/tcp/%d/local", net, i);
		if((fd = open(buf, OREAD)) < 0)
			sysfatal("did not find display");
		n = read(fd, buf, sizeof buf-1);
		close(fd);
		if(n <= 0)
			continue;
		buf[n] = 0;
		p = strchr(buf, '!');
		if(p == 0)
			continue;
		port = atoi(p+1);
		if(port != display+baseport)
			continue;
		snprint(buf, sizeof buf, "%s/tcp/%d/ctl", net, i);
		fd = open(buf, OWRITE);
		if(fd < 0)
			sysfatal("cannot open %s: %r", buf);
		if(write(fd, "hangup", 6) != 6)
			sysfatal("cannot hangup %s: %r", buf);
		close(fd);
		break;
	}
}

/*
 * Look for a port on which to announce.
 * If display != -1, we only try that one.
 * Otherwise we hunt.
 *
 * Returns the announce fd.
 */
static int
vncannounce(char *net, int display, char *adir, int base)
{
	int port, eport, fd;
	char addr[NETPATHLEN];

	if(display == -1){
		port = base;
		eport = base+50;
	}else{
		port = base+display;
		eport = port;
	}

	for(; port<=eport; port++){
		snprint(addr, sizeof addr, "%s/tcp!*!%d", net, port);
		if((fd = announce(addr, adir)) >= 0){
			fprint(2, "server started on display :%d\n", port-base);
			return fd;
		}
	}
	if(display == -1)
		fprint(2, "could not find any ports to announce\n");
	else
		fprint(2, "announce: %r\n");
	return -1;
}

/*
 * Handle a new connection.
 */
static void clientreadproc(Vncs*);
static void clientwriteproc(Vncs*);
static void chan2fmt(Pixfmt*, ulong);
static ulong fmt2chan(Pixfmt*);

static void
vncaccept(Vncs *v)
{
	char buf[32];
	int fd;
	TLSconn conn;

	/* caller returns to listen */
	switch(rfork(RFPROC|RFMEM|RFNAMEG)){
	case -1:
		fprint(2, "%V: fork failed: %r\n", v);
		vncclose(v);
		return;
	default:
		return;
	case 0:
		break;
	}
	*vncpriv = v;

	if(!atexit(exiting)){
		fprint(2, "%V: could not register atexit handler: %r; hanging up\n", v);
		exiting();
		exits(nil);
	}

	if(cert != nil){
		memset(&conn, 0, sizeof conn);
		conn.cert = readcert(cert, &conn.certlen);
		if(conn.cert == nil){
			fprint(2, "%V: could not read cert %s: %r; hanging up\n", v, cert);
			exits(nil);
		}
		fd = tlsServer(v->datafd, &conn);
		if(fd < 0){
			fprint(2, "%V: tlsServer: %r; hanging up\n", v);
			free(conn.cert);
			if(conn.sessionID)
				free(conn.sessionID);
			exits(nil);
		}
		close(v->datafd);
		v->datafd = fd;
		free(conn.cert);
		free(conn.sessionID);
	}
	vncinit(v->datafd, v->ctlfd, v);

	if(verbose)
		fprint(2, "%V: handshake\n", v);
	if(vncsrvhandshake(v) < 0){
		fprint(2, "%V: handshake failed; hanging up\n", v);
		exits(0);
	}
	if(verbose)
		fprint(2, "%V: auth\n", v);
	if(vncsrvauth(v) < 0){
		fprint(2, "%V: auth failed; hanging up\n", v);
		exits(0);
	}

	shared = vncrdchar(v);

	if(verbose)
		fprint(2, "%V: %sshared\n", v, shared ? "" : "not ");
	if(!shared)
		killclients(v);

	v->dim = (Point){Dx(gscreen->r), Dy(gscreen->r)};
	vncwrpoint(v, v->dim);
	if(verbose)
		fprint(2, "%V: send screen size %P (rect %R)\n", v, v->dim, gscreen->r);

	v->bpp = gscreen->depth;
	v->depth = gscreen->depth;
	v->truecolor = 1;
	v->bigendian = 0;
	chan2fmt(v, gscreen->chan);
	if(verbose)
		fprint(2, "%V: bpp=%d, depth=%d, chan=%s\n", v,
			v->bpp, v->depth, chantostr(buf, gscreen->chan));
	vncwrpixfmt(v, v);
	vncwrlong(v, 14);
	vncwrbytes(v, "Plan9 Desktop", 14);
	vncflush(v);

	if(verbose)
		fprint(2, "%V: handshaking done\n", v);

	switch(rfork(RFPROC|RFMEM)){
	case -1:
		fprint(2, "%V: cannot fork: %r; hanging up\n", v);
		vnchungup(v);
	default:
		clientreadproc(v);
		exits(nil);
	case 0:
		*vncpriv = v;
		v->nproc++;
		if(atexit(exiting) == 0){
			exiting();
			fprint(2, "%V: could not register atexit handler: %r; hanging up\n", v);
			exits(nil);
		}
		clientwriteproc(v);
		exits(nil);
	}
}

static void
vncname(char *fmt, ...)
{
	int fd;
	char name[64], buf[32];
	va_list arg;

	va_start(arg, fmt);
	vsnprint(name, sizeof name, fmt, arg);
	va_end(arg);

	sprint(buf, "/proc/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		write(fd, name, strlen(name));
		close(fd);
	}
}

/*
 * Set the pixel format being sent.  Can only happen once.
 * (Maybe a client would send this again if the screen changed
 * underneath it?  If we want to support this we need a way to
 * make sure the current image is no longer in use, so we can free it. 
 */
static void
setpixelfmt(Vncs *v)
{
	ulong chan;

	vncgobble(v, 3);
	v->Pixfmt = vncrdpixfmt(v);
	chan = fmt2chan(v);
	if(chan == 0){
		fprint(2, "%V: bad pixel format; hanging up\n", v);
		vnchungup(v);
	}
	v->imagechan = chan;
}

/*
 * Set the preferred encoding list.  Can only happen once.
 * If we want to support changing this more than once then
 * we need to put a lock around the encoding functions
 * so as not to conflict with updateimage.
 */
static void
setencoding(Vncs *v)
{
	int n, x;

	vncrdchar(v);
	n = vncrdshort(v);
	while(n-- > 0){
		x = vncrdlong(v);
		switch(x){
		case EncCopyRect:
			v->copyrect = 1;
			continue;
		case EncMouseWarp:
			v->canwarp = 1;
			continue;
		}
		if(v->countrect != nil)
			continue;
		switch(x){
		case EncRaw:
			v->encname = "raw";
			v->countrect = countraw;
			v->sendrect = sendraw;
			break;
		case EncRre:
			v->encname = "rre";
			v->countrect = countrre;
			v->sendrect = sendrre;
			break;
		case EncCorre:
			v->encname = "corre";
			v->countrect = countcorre;
			v->sendrect = sendcorre;
			break;
		case EncHextile:
			v->encname = "hextile";
			v->countrect = counthextile;
			v->sendrect = sendhextile;
			break;
		}
	}

	if(v->countrect == nil){
		v->encname = "raw";
		v->countrect = countraw;
		v->sendrect = sendraw;
	}

	if(verbose)
		fprint(2, "Encoding with %s%s%s\n", v->encname,
			v->copyrect ? ", copyrect" : "",
			v->canwarp ? ", canwarp" : "");
}

/*
 * Continually read updates from one client.
 */
static void
clientreadproc(Vncs *v)
{
	int incremental, key, keydown, buttons, type, x, y, n;
	char *buf;
	Rectangle r;

	vncname("read %V", v);

	for(;;){
		type = vncrdchar(v);
		switch(type){
		default:
			fprint(2, "%V: unknown vnc message type %d; hanging up\n", v, type);
			vnchungup(v);

		/* set pixel format */
		case MPixFmt:
			setpixelfmt(v);
			break;

		/* ignore color map changes */
		case MFixCmap:
			vncgobble(v, 3);
			n = vncrdshort(v);
			vncgobble(v, n*6);
			break;

		/* set encoding list */
		case MSetEnc:
			setencoding(v);
			break;

		/* request image update in rectangle */
		case MFrameReq:
			incremental = vncrdchar(v);
			r = vncrdrect(v);
			if(incremental){
				vnclock(v);
				v->updaterequest = 1;
				vncunlock(v);
			}else{
				drawlock();	/* protects rlist */
				vnclock(v);	/* protects updaterequest */
				v->updaterequest = 1;
				addtorlist(&v->rlist, r);
				vncunlock(v);
				drawunlock();
			}
			break;

		/* send keystroke */
		case MKey:
			keydown = vncrdchar(v);
			vncgobble(v, 2);
			key = vncrdlong(v);
			vncputc(!keydown, key);
			break;

		/* send mouse event */
		case MMouse:
			buttons = vncrdchar(v);
			x = vncrdshort(v);
			y = vncrdshort(v);
			mousetrack(x, y, buttons, nsec()/(1000*1000LL));
			break;

		/* send cut text */
		case MCCut:
			vncgobble(v, 3);
			n = vncrdlong(v);
			buf = malloc(n+1);
			if(buf){
				vncrdbytes(v, buf, n);
				buf[n] = 0;
				vnclock(v);	/* for snarfvers */
				setsnarf(buf, n, &v->snarfvers);
				vncunlock(v);
			}else
				vncgobble(v, n);
			break;
		}
	}
}

static int
nbits(ulong mask)
{
	int n;

	n = 0;
	for(; mask; mask>>=1)
		n += mask&1;
	return n;
}

typedef struct Col Col;
struct Col {
	int type;
	int nbits;
	int shift;
};

static ulong
fmt2chan(Pixfmt *fmt)
{
	Col c[4], t;
	int i, j, depth, n, nc;
	ulong mask, u;

	/* unpack the Pixfmt channels */
	c[0] = (Col){CRed, nbits(fmt->red.max), fmt->red.shift};
	c[1] = (Col){CGreen, nbits(fmt->green.max), fmt->green.shift};
	c[2] = (Col){CBlue, nbits(fmt->blue.max), fmt->blue.shift};
	nc = 3;

	/* add an ignore channel if necessary */
	depth = c[0].nbits+c[1].nbits+c[2].nbits;
	if(fmt->bpp != depth){
		/* BUG: assumes only one run of ignored bits */
		if(fmt->bpp == 32)
			mask = ~0;
		else
			mask = (1<<fmt->bpp)-1;
		mask ^= fmt->red.max << fmt->red.shift;
		mask ^= fmt->green.max << fmt->green.shift;
		mask ^= fmt->blue.max << fmt->blue.shift;
		if(mask == 0)
			abort();
		n = 0;
		for(; !(mask&1); mask>>=1)
			n++;
		c[3] = (Col){CIgnore, nbits(mask), n};
		nc++;
	}

	/* sort the channels, largest shift (leftmost bits) first */
	for(i=1; i<nc; i++)
		for(j=i; j>0; j--)
			if(c[j].shift > c[j-1].shift){
				t = c[j];
				c[j] = c[j-1];
				c[j-1] = t;
			}

	/* build the channel descriptor */
	u = 0;
	for(i=0; i<nc; i++){
		u <<= 8;
		u |= CHAN1(c[i].type, c[i].nbits);
	}

	return u;
}

static void
chan2fmt(Pixfmt *fmt, ulong chan)
{
	ulong c, rc, shift;

	shift = 0;
	for(rc = chan; rc; rc >>=8){
		c = rc & 0xFF;
		switch(TYPE(c)){
		case CRed:
			fmt->red = (Colorfmt){(1<<NBITS(c))-1, shift};
			break;
		case CBlue:
			fmt->blue = (Colorfmt){(1<<NBITS(c))-1, shift};
			break;
		case CGreen:
			fmt->green = (Colorfmt){(1<<NBITS(c))-1, shift};
			break;
		}
		shift += NBITS(c);
	}
}

/*
 * Note that r has changed on the screen.
 * Updating the rlists is okay because they are protected by drawlock.
 */
void
flushmemscreen(Rectangle r)
{
	Vncs *v;

	if(!rectclip(&r, gscreen->r))
		return;
	qlock(&clients);
	for(v=clients.head; v; v=v->next)
		addtorlist(&v->rlist, r);
	qunlock(&clients);
}

/*
 * Queue a mouse warp note for the next update to each client.
 */
void
mousewarpnote(Point p)
{
	Vncs *v;

	qlock(&clients);
	for(v=clients.head; v; v=v->next){
		if(v->canwarp){
			vnclock(v);
			v->needwarp = 1;
			v->warppt = p;
			vncunlock(v);
		}
	}
	qunlock(&clients);
}

/*
 * Send a client his changed screen image.
 * v is locked on entrance, locked on exit, but released during.
 */
static int
updateimage(Vncs *v)
{
	int i, ncount, nsend, docursor, needwarp;
	vlong ooffset;
	Point warppt;
	Rectangle cr;
	Rlist rlist;
	vlong t1;
	int (*count)(Vncs*, Rectangle);
	int (*send)(Vncs*, Rectangle);

	if(v->image == nil)
		return 0;

	/* warping info and unlock v so that updates can proceed */
	needwarp = v->canwarp && v->needwarp;
	warppt = v->warppt;
	v->needwarp = 0;
	vncunlock(v);

	/* copy the screen bits and then unlock the screen so updates can proceed */
	drawlock();
	rlist = v->rlist;
	memset(&v->rlist, 0, sizeof v->rlist);

	/* if the cursor has moved or changed shape, we need to redraw its square */
	lock(&cursor);
	if(v->cursorver != cursorver || !eqpt(v->cursorpos, cursorpos)){
		docursor = 1;
		v->cursorver = cursorver;
		v->cursorpos = cursorpos;
		cr = cursorrect();
	}else{
		docursor = 0;
		cr = v->cursorr;
	}
	unlock(&cursor);

	if(docursor){
		addtorlist(&rlist, v->cursorr);
		if(!rectclip(&cr, gscreen->r))
			cr.max = cr.min;
		addtorlist(&rlist, cr);
	}

	/* copy changed screen parts, also check for parts overlapping cursor location */
	for(i=0; i<rlist.nrect; i++){
		if(!docursor)
			docursor = rectXrect(v->cursorr, rlist.rect[i]);
		memimagedraw(v->image, rlist.rect[i], gscreen, rlist.rect[i].min, memopaque, ZP, S);
	}

	if(docursor){
		cursordraw(v->image, cr);
		addtorlist(&rlist, v->cursorr);
		v->cursorr = cr;
	}

	drawunlock();

	ooffset = Boffset(&v->out);
	/* no more locks are held; talk to the client */

	if(rlist.nrect == 0 && needwarp == 0){
		vnclock(v);
		return 0;
	}

	count = v->countrect;
	send = v->sendrect;
	if(count == nil || send == nil){
		count = countraw;
		send = sendraw;
	}

	ncount = 0;
	for(i=0; i<rlist.nrect; i++)
		ncount += (*count)(v, rlist.rect[i]);

	if(verbose > 1)
		fprint(2, "sendupdate: rlist.nrect=%d, ncount=%d", rlist.nrect, ncount);

	t1 = nsec();
	vncwrchar(v, MFrameUpdate);
	vncwrchar(v, 0);
	vncwrshort(v, ncount+needwarp);

	nsend = 0;
	for(i=0; i<rlist.nrect; i++)
		nsend += (*send)(v, rlist.rect[i]);

	if(ncount != nsend){
		fprint(2, "%V: ncount=%d, nsend=%d; hanging up\n", v, ncount, nsend);
		vnchungup(v);
	}

	if(needwarp){
		vncwrrect(v, Rect(warppt.x, warppt.y, warppt.x+1, warppt.y+1));
		vncwrlong(v, EncMouseWarp);
	}

	t1 = nsec() - t1;
	if(verbose > 1)
		fprint(2, " in %lldms, %lld bytes\n", t1/1000000, Boffset(&v->out) - ooffset);

	freerlist(&rlist);
	vnclock(v);
	return 1;
}

/*
 * Update the snarf buffer if it has changed.
 */
static void
updatesnarf(Vncs *v)
{
	char *buf;
	int len;

	if(v->snarfvers == snarf.vers)
		return;
	vncunlock(v);
	qlock(&snarf);
	len = snarf.n;
	buf = malloc(len);
	if(buf == nil){
		qunlock(&snarf);
		vnclock(v);
		return;
	}
	memmove(buf, snarf.buf, len);
	v->snarfvers = snarf.vers;
	qunlock(&snarf);

	vncwrchar(v, MSCut);
	vncwrbytes(v, "pad", 3);
	vncwrlong(v, len);
	vncwrbytes(v, buf, len);
	free(buf);
	vnclock(v);
}

/*
 * Continually update one client.
 */
static void
clientwriteproc(Vncs *v)
{
	char buf[32], buf2[32];
	int sent;

	vncname("write %V", v);
	for(;;){
		vnclock(v);
		if(v->ndead)
			break;
		if((v->image == nil && v->imagechan!=0)
		|| (v->image && v->image->chan != v->imagechan)){
			if(v->image)
				freememimage(v->image);
			v->image = allocmemimage(Rpt(ZP, v->dim), v->imagechan);
			if(v->image == nil){
				fprint(2, "%V: allocmemimage: %r; hanging up\n", v);
				vnchungup(v);
			}
			if(verbose)
				fprint(2, "%V: translating image from chan=%s to chan=%s\n",
					v, chantostr(buf, gscreen->chan), chantostr(buf2, v->imagechan));
		}
		sent = 0;
		if(v->updaterequest){
			v->updaterequest = 0;
			updatesnarf(v);
			sent = updateimage(v);
			if(!sent)
				v->updaterequest = 1;
		}
		vncunlock(v);
		vncflush(v);
		if(!sent)
			sleep(sleeptime);
	}
	vncunlock(v);
	vnchungup(v);
}

