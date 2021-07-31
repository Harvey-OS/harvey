#define	Image	IMAGE
#include "vnc.h"
#include "vncs.h"
#include "compat.h"
#include <cursor.h>
#include "screen.h"
#include "kbd.h"

enum
{
	MaxCorreDim	= 48,
	MaxRreDim	= 64,
};

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

int            shared;
int		verbose = 0;
static int	worker = -1;			/* pid of command we ran */
static int	srvfd;
static int	exportfd;
static Vncs	**vncpriv;

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

static void shutdown_clients(Vncs *);

/* 
 * list head. used to hold the list, the lock, dim, and pixelfmt
 */
Vnc clients;

static void	vnccsexiting(void);

static int
nbits(ulong mask)
{
	int bit;

	if (mask == 0)
		sysfatal("nbits: invalid mask 0x%lx", mask);

	mask = ~mask;
	for (bit = 0; !(mask & 1); bit++)
		mask >>= 1;

	return bit;
}

struct _chan {
	uchar type;
	uchar nbits;
	Colorfmt * fmt;
};
#define SORT(x, y) if(x.fmt->shift < y.fmt->shift) do { chan = x; x = y; y = chan; } while(0)

static ulong
fmt2chan(Pixfmt * fmt)
{
	struct _chan chans[4];
	struct _chan chan;
	int i, depth, n;
	ulong mask = 0xffffffff;
	Colorfmt xfmt;

	chans[0] = (struct _chan){CRed,0,&fmt->red};
	chans[1] = (struct _chan){CGreen,0,&fmt->green};
	chans[2] = (struct _chan){CBlue,0,&fmt->blue};

	SORT(chans[0], chans[1]);
	SORT(chans[0], chans[2]);
	SORT(chans[1], chans[2]);

	depth = 0;
	for(i = 0; i < 3; i++) {
		n = chans[i].nbits = nbits(chans[i].fmt->max);
		depth += n;
		mask &= ~(chans[i].fmt->max<<chans[i].fmt->shift);
	}

	if(fmt->bpp != depth && mask) {
		xfmt.shift = 0;
		while(!(mask & 1)) {
			xfmt.shift++;
			mask >>= 1;
		}
		xfmt.max = mask;
		chans[3] = (struct _chan){CIgnore, fmt->bpp-depth, &xfmt};

		SORT(chans[2], chans[3]);
		SORT(chans[1], chans[2]);
		SORT(chans[0], chans[1]);

		return (CHAN4(chans[0].type, chans[0].nbits,
		    chans[1].type, chans[1].nbits,
		    chans[2].type, chans[2].nbits,
		    chans[3].type, chans[3].nbits));
	} else
		return (CHAN3(chans[0].type, chans[0].nbits,
		    chans[1].type, chans[1].nbits,
		    chans[2].type, chans[2].nbits));
}
#undef SORT

static void
chan2fmt(Pixfmt *fmt, ulong chan)
{
	ulong c, rc, shift;

	shift = 0;
	for(rc = chan; rc; rc >>=8) {
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

void
srvchoosecolor(Vnc *v)
{
	int bpp, depth;
	ulong chan;

	bpp = gscreen->depth;
	depth = gscreen->depth;
	chan = gscreen->chan;

	v->bpp = bpp;
	v->depth = depth;
	v->truecolor = 1;
	v->bigendian = 0;
	chan2fmt(v, chan);
	if(verbose)
		fprint(2, "%d bpp, %d depth, 0x%lx chan, %d truecolor, %d bigendian\n",
		    v->bpp, v->depth, chan, v->truecolor, v->bigendian);

	vncwrpixfmt(v, &v->Pixfmt);
	vncflush(v);
}

void
settranslation(Vncs * v)
{
	ulong chan;

	chan = fmt2chan(&v->Pixfmt);
	if(verbose)
		fprint(2, "display chan 0x%lx, client chan 0x%lx\n", gscreen->chan, chan);

	v->clientimage = allocmemimage(Rpt(ZP, v->dim), chan);
	if(v->clientimage == nil)
		sysfatal("allocmemimage %r");
	if(verbose)
		fprint(2, "\ttranslating image format\n");
}

void
senddim(Vnc *v)
{
	v->dim = (Point){Dx(gscreen->r), Dy(gscreen->r)};
	vncwrpoint(v, v->dim);

	if(verbose)
		fprint(2, "screen is %d x %d %R\n", v->dim.x, v->dim.y, gscreen->r);
}

void flushmemscreen(Rectangle r)
{
	Vncs *v;

	vnclock(&clients);
	for(v = (Vncs*)clients.next; v; v = (Vncs*)v->next)
		region_union(&v->updateregion, r, Rpt(ZP, v->dim));
	vncunlock(&clients);
}

// do lazy mouse warp. without it, we would cause
// a warp message in each update.
void mousewarpnote()
{
	Vncs *v;

	vnclock(&clients);
	for(v = (Vncs*)clients.next; v; v = (Vncs*)v->next) {
		if( v->canmousewarp ) {
			vnclock(v);
			v->mousewarpneeded = 1;
			vncunlock(v);
		}
	}
	vncunlock(&clients);
}

static void
sendcopyrect(Vncs *v, Rectangle r)
{
	sendraw(v, r);
}

static int
sendupdate(Vncs *v)
{
	int nrects, i, snrects, docursor, dowarp;
	Rectangle nr, cr;
	Region updateregion;
	vlong t1;
	Point mpt;

	drawlock();

	dowarp = v->mousewarpneeded; // cleared in vnc_update with lock

	/* we make a snapshot so that new screen updates can overlap with us */
	region_init(&updateregion);
	region_copy(&updateregion, &v->updateregion);
	region_reset(&v->updateregion);

	docursor = 0;
	lock(&cursor);
	if(v->cursorver != cursorver || !eqpt(v->cursorpos, cursorpos)){
		docursor = 1;
		v->cursorver = cursorver;
		v->cursorpos = cursorpos;
		cr = cursorrect();
	}else
		cr = v->cursorr;
	unlock(&cursor);
	if(docursor)
		region_union(&updateregion, v->cursorr, Rpt(ZP, v->dim));
	for(i = 0; i < updateregion.nrects; i++){
		if(!docursor)
			docursor = rectXrect(v->cursorr, updateregion.rects[i]);
		memimagedraw(v->clientimage, updateregion.rects[i], gscreen, updateregion.rects[i].min, memopaque, ZP);
	}

	if(docursor){
		cursordraw(v->clientimage, cr);
		region_union(&updateregion, cr, Rpt(ZP, v->dim));
		v->cursorr = cr;
	}
	drawunlock();
	if( REGION_EMPTY(&updateregion) && dowarp == 0 ) {
		region_reset(&updateregion);
		return 1;
	}

	if(v->preferredencoding == EncCorre) {
		nrects = 0;
		for(i = 0; i < updateregion.nrects; i ++ ) {
			nrects += rrerects(updateregion.rects[i], MaxCorreDim);
		}
	} else if(v->preferredencoding == EncRre) {
		nrects = 0;
		for(i = 0; i < updateregion.nrects; i ++ ) {
			nrects += rrerects(updateregion.rects[i], MaxRreDim);
		}
	} else
		nrects = updateregion.nrects;

	if( verbose > 1 )
		fprint(2, "sendupdate: %d rects, %d region rects..", nrects, updateregion.nrects);
	t1 = nsec();
	vncwrchar(v, MFrameUpdate);
	vncwrchar(v, 0);
	if( v->canmousewarp )
		vncwrshort(v, nrects+dowarp);
	else
		vncwrshort(v, nrects);

	snrects = 0;
	for(i = 0; i < updateregion.nrects; i++) {
		nr = updateregion.rects[i];

		switch(v->preferredencoding) {
		default:
			sysfatal("don't know about encoding %d\n", v->preferredencoding);
			break;
		case EncRaw:
			sendraw(v, nr);
			snrects++;
			break;
		case EncRre:
			snrects += sendrre(v, nr, MaxRreDim, 0);
			break;
		case EncCorre:
			snrects += sendrre(v, nr, MaxCorreDim, 1);
			break;
		case EncHextile:
			sendhextile(v, nr);
			snrects++;
			break;
		}
	}
	if(snrects != nrects)
		sysfatal("bad number of rectangles sent; %d expexted %d\n", snrects, nrects);
	region_reset(&updateregion);
	if( dowarp) {
		// four shorts and an int (x, y, w, h, enctype)
		mpt = mousexy();
		vncwrrect(v, Rpt(mpt, Pt(mpt.x+1, mpt.y+1)));
		vncwrlong(v, EncMouseWarp);
	}
	if( verbose > 1 ) 
		fprint(2, " in %lld msecs\n", (nsec()-t1)/(1000*1000LL));

	return 0;
}

static void
vnc_update(Vncs *v)
{
	char *buf;
	int n, notsent;

	qlock(&snarf);
	v->snarfvers = snarf.vers;
	qunlock(&snarf);
	while(1) {
		vnclock(v);
		if( v->ndeadprocs )
			break;
		notsent = 1;
		if( v->updaterequested ) {
			v->updaterequested = 0;
			vncunlock(v);
			qlock(&snarf);
			if(v->snarfvers != snarf.vers) {
				n = snarf.n;
				buf = malloc(n);
				if(buf != nil)
					memmove(buf, snarf.buf, n);
				v->snarfvers = snarf.vers;
				qunlock(&snarf);

				if(buf != nil){
					vncwrchar(v, MSCut);
					vncwrbytes(v, "pad", 3);
					vncwrlong(v, n);
					vncwrbytes(v, buf, n);
					free(buf);
				}
			}else
				qunlock(&snarf);
			notsent = sendupdate(v);
			vnclock(v);
			v->updaterequested |= notsent;
			v->mousewarpneeded = 0;
		}
		vncunlock(v);
		vncflush(v);
		if( notsent ) sleep(40);
	}
	vncunlock(v);
	vnchungup(v);
}

static int
srvtlscrypt(Vnc *v)
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
		execl("/bin/ntlssrv", "tlssrv", "-Dlvnc", "-k/tmp", nil);
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
srvnocrypt(Vnc*)
{
	return 0;
}

static int
srvvncauth(Vnc *v)
{
	return vncauth_srv(v);
}

static int
srvnoauth(Vnc*)
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
	int required;
	Choice *c;
	int nc;
};

static Choice srvcryptchoice[] = {
	"tlsv1",	srvtlscrypt,
//	"none",	srvnocrypt,
};

static Choice srvauthchoice[] = {
	"vnc",	srvvncauth,
//	"none",	srvnoauth,
};

static Option options[] = {
	"crypt",	1,	srvcryptchoice,	nelem(srvcryptchoice),
	"auth",	1,	srvauthchoice,	nelem(srvauthchoice),
};

static int
vncnegotiate_srv(Vnc *v)
{
	char opts[256], *eopts, *p, *resp;
	int i, j;

	for(i=0; i<nelem(options); i++){
		if(verbose)
			fprint(2, "%s...", options[i].s);
		eopts = opts+sizeof opts;
		p = seprint(opts, eopts, "%s", options[i].s);
		for(j=0; j<options[i].nc; j++)
			p = seprint(p, eopts, " %s", options[i].c[j].s);
		vncwrstring(v, opts);
		vncflush(v);
		resp = vncrdstringx(v);
		p = strchr(resp, ' ');
		if(p == nil){
			fprint(2, "malformed response in negotiation\n");
			free(resp);
			return -1;
		}
		*p++ = '\0';
		if(strcmp(resp, "error")==0){
			fprint(2, "negotiating %s: %s\n", options[i].s, p);
			if(options[i].required){
				free(resp);
				return -1;
			}
		}else if(strcmp(resp, options[i].s)==0){
			for(j=0; j<options[i].nc; i++)
				if(strcmp(options[i].c[j].s, p)==0)
					break;
			if(j==options[i].nc){
				fprint(2, "negotiating %s: unknown choice: %s\n",
					options[i].s, p);
				free(resp);
				return -1;
			}
			if(verbose)
				fprint(2, "(%s)...", p);
			if(options[i].c[j].fn(v) < 0){
				fprint(2, "%s %s failed: %r\n", resp, p);
				free(resp);
				return -1;
			}
		}else{
			p[-1] = ' ';
			fprint(2, "negotiating %s: bad response: %s\n", options[i].s, resp);
			free(resp);
			return -1;
		}
		free(resp);
	}
	vncwrstring(v, "start");
	if(verbose)
		fprint(2, "start...");
	vncflush(v);
	return 0;
}

static void
vnc_srv(Vncs *v, int dfd, int cfd)
{
	Rectangle r;
	char msg[12+1], *sn;
	uchar type, keydown;
	ushort n;
	ulong incremental, x, key;
	int y, buttons;

	if(verbose)
		fprint(2, "init...");

	vncinit(dfd, cfd, v);
	v->preferredencoding = -1;
	v->usecopyrect = 0;
	v->canmousewarp = 0;
	v->ndeadprocs = 0;
	v->nprocs = 1;

	if(verbose)
		fprint(2, "handshake...");
	if(vnchandshake_srv(v) < 0){
		if(verbose)
			fprint(2, "vnc handshake failed\n");
		vnchungup(v);
	}

	if(v->extended){
		if(verbose)
			fprint(2, "negotiate...");
		if(vncnegotiate_srv(v) < 0){
			if(verbose)
				fprint(2, "vnc negotiation failed\n");
			vnchungup(v);
		}
	}else{
		if(verbose)
			fprint(2, "auth...");
		if(vncauth_srv(v) < 0){
			if(verbose)
				fprint(2, "vnc authentication failed\n");
			vnchungup(v);
		}
	}

	shared = vncrdchar(v);
	if(verbose)
		fprint(2, "client is %sshared\n", shared?"":"not ");
	if(!shared){
		shutdown_clients(v);
	}

	senddim(v);
	srvchoosecolor(v);
	vncwrlong(v, 14);
	vncwrbytes(v, "Plan9 Desktop", 14);
	vncflush(v);

	if( verbose )
		fprint(2, "handshaking done\n");

	/* start the update process */
	switch(rfork(RFPROC|RFMEM)){
	case -1:
		sysfatal("can't make update process: %r");
	default:
		break;
	case 0:
		v->nprocs++;
		*vncpriv = v;
		if(!atexit(vnccsexiting)){
			vnccsexiting();
			sysfatal("too many clients");
		}
		vnc_update(v);
		exits(nil);
	}

	region_init(&v->updateregion);

	while(1) {
		type = vncrdchar(v);
		switch(type) {
		default:
			sysfatal("unknown vnc message type=%d\n", type);
			break;

		case MPixFmt:
			vncrdbytes(v, msg, 3);
			v->Pixfmt = vncrdpixfmt(v);
			settranslation(v);
			break;

		case MFixCmap:
			vncrdbytes(v, msg, 3);
			n = vncrdshort(v);
			vncgobble(v, n*6);
			break;

		case MSetEnc:
			vncrdchar(v);
			n = vncrdshort(v);
			while( n-- ) {
				x = vncrdlong(v);
				if( x == EncCopyRect )
					v->usecopyrect = 1;
				else if ( x == EncMouseWarp )
					v->canmousewarp = 1;
				else if( v->preferredencoding == -1 )
					v->preferredencoding = x;
			}
			if( v->preferredencoding == -1 )
				v->preferredencoding = EncRaw;
			if( verbose )
				fprint(2, "encoding is %s %s copyrect\n", encname[v->preferredencoding], v->usecopyrect?"with":"without");
			break;

		case MFrameReq:
			incremental = vncrdchar(v);
			r = vncrdrect(v);
			drawlock();  // must hold drawlock first
			vnclock(v);
			v->updaterequested = 1;
			if( !incremental ) 
				region_union(&v->updateregion, r, Rpt(ZP, v->dim));
			vncunlock(v);
			drawunlock();
			break;

		case MKey:
			keydown = vncrdchar(v);
			vncgobble(v, 2);
			key = vncrdlong(v);
			vncputc(!keydown, key);
			break;

		case MMouse:
			buttons = vncrdchar(v);
			x = vncrdshort(v);
			y = vncrdshort(v);
			mousetrack(x, y, buttons, nsec()/(1000*1000LL));
			break;

		case MCCut:
			vncrdbytes(v, msg, 3);
			x = vncrdlong(v);
			sn = malloc(x + 1);
			if(sn == nil)
				vncgobble(v, x);
			else{
				vncrdbytes(v, sn, x);
				sn[x] = '\0';
				setsnarf(sn, x);
			}
			break;
		}
	}
}

void
vnc_newclient(int dfd, int cfd, int extended)
{
	Vncs * v;

	/* caller returns to listen */
	switch(rfork(RFPROC|RFMEM)){
	case -1:
		close(dfd);
		close(cfd);
		return;
	default:
		return;
	case 0:
		break;
	}

	v = mallocz(sizeof(Vncs), 1);
	vnclock(&clients);
	v->next = clients.next;
	clients.next = v;
	vncunlock(&clients);
	v->extended = extended;
	*vncpriv = v;
	if(!atexit(vnccsexiting)){
		vnccsexiting();
		sysfatal("too many clients");
	}
	vnc_srv(v, dfd, cfd);
}

/* here is where client procs exit.
 * vnclock(v) must not be held.
 */
void
vnchungup(Vnc *)
{
	exits(nil);
}

static void
vnccsexiting(void)
{
	Vncs *v;
	Vnc **vp;

	v = *vncpriv;
	vnclock(&clients);
	for(vp = &clients.next; *vp != nil; vp = &(*vp)->next)
		if(*vp == v)
			break;

	/* check for shut down */
	if(*vp == nil){
		vncunlock(&clients);
		return;
	}

	/* wait for all of this client's procs to die */
	vnclock(v);
	if( ++v->ndeadprocs < v->nprocs ) {
		vncunlock(v);
		vncunlock(&clients);
		return;
	}

	*vp = v->next;
	vncunlock(&clients);

	region_reset(&v->updateregion);
	vncterm(v);
	close(v->ctlfd);
	close(v->datafd);
	freememimage(v->clientimage);

	free(v);
}

/*
 * kill all clients except nv.
 *
 * called by:
 *	the listener before a non-shared client starts
 *	shutdown
 */
void static
shutdown_clients(Vncs * nv)
{
	Vncs *v;

	vnclock(&clients);
	for(v = (Vncs*)clients.next; v != nil; v = (Vncs*)v->next){
		if(v == nv)
			continue;

		if(v->ctlfd >= 0){
			hangup(v->ctlfd);
			close(v->ctlfd);
			v->ctlfd = -1;
			close(v->datafd);
			v->datafd = -1;
		}
	}
	vncunlock(&clients);
}

static char killkin[] = "die vnc kin";

void
killall(void)
{
	int pid;

	postnote(PNGROUP, worker, "kill");
	close(srvfd);
	srvfd = -1;
	close(exportfd);
	exportfd = -1;
	pid = getpid();
	postnote(PNGROUP, pid, killkin);
}

/*
 * called by the exporter when unmounted
 * and the main listener when hung up.
 */
void
shutdown(void)
{
	if(verbose)
		fprint(2, "vnc server shutdown\n");
	killall();
}

static void
noteshut(void*, char *msg)
{
	if(strcmp(msg, killkin) != 0)
		killall();
	noted(NDFLT);
}

/* try once if the user passed us the address, or hunt for one 
 * explicit:	tcp!*!5900
 * auto:	/net | nil
 */
int
vncannounce(char *netmt, char * darg, char * adir)
{
	int port, eport, fd;
	char portstr[NETPATHLEN];

	port = 5900;
	eport = port + 50;
	if( darg != nil && darg[0] == ':' ) {
		port = 5900 + strtol(&darg[1], nil, 10);
		eport = port;
	}

	for(; port <= eport; port++) {
		snprint(portstr, NETPATHLEN, "%s/tcp!*!%d", netmt, port);
		fd = announce(portstr, adir);
		if( fd >= 0 ) {
			fprint(2, "server started on display :%d\n", port-5900);
			return fd;
		}
	}

	return -1;
}

/* the user wants to kill the sever on a specified port 
*/
static void
vnckillsrv(char * netroot, char * portstr)
{
	int port, fd, lport;
	char * p;
	int fdir, i, tot, n;
	Dir *dir, * db;
	char buf[NETPATHLEN];

	port = atoi(portstr);
	
	port += 5900;
	/* find the listener in /net/tcp/ */
	snprint(buf, sizeof buf, "%s/tcp", netroot);
	fdir = open(buf, OREAD);
	if(fdir < 0)
		return;

	tot = dirreadall(fdir, &dir);
	for(i = 0; i < tot; i++) {
		db = & dir[i];

		/* read in the local port number */
		sprint(buf, "%s/tcp/%s/local", netroot, db->name);
		fd = open(buf, OREAD);
		if(fd < 0) 
			sysfatal("can't open file %s", buf);
		n = read(fd, buf, sizeof(buf));
		if(n < 0) 
			sysfatal("can't read file %s", buf);
		buf[n-1] = 0;
		close(fd);
		p = strchr(buf, '!');
		if(p == 0) 
			sysfatal("can't parse string %s", buf);
		lport = atoi(p+1);
		if(lport != port)
			continue;

		sprint(buf, "%s/tcp/%s/ctl", netroot, db->name);
		fd = open(buf, OWRITE);
		if( fd < 0 )
			sysfatal("can't open file %s", buf);
		if( write(fd, "hangup", 6) != 6 )
			sysfatal("can't write to file %s", buf);
		close(fd);
		break;
	}
	free(dir);
	close(fdir);
}

void
usage(void)
{
	fprint(2, "usage:\tvncs [-v] [-g widthXheight] [-d :display] [command [args ...]]\n");
	fprint(2, "\tvncs [-v] [-k :display]\n");
	exits("usage");
}

void
main(int argc, char * argv[])
{
	static char *defargv[] = { "/bin/rc", "-i", nil };
	char *addr, netmt[NETPATHLEN], adir[NETPATHLEN], ldir[NETPATHLEN], *p, *darg, *karg;
	int cfd, s, n, fd, w, h, extended;

	w = 1024;
	h = 768;
	addr = nil;
	darg = nil;
	karg = nil;
	extended = 0;
	setnetmtpt(netmt, NETPATHLEN, nil);
	ARGBEGIN{
	case 'd':
		darg = ARGF();
		if(darg == nil || *darg != ':')
			usage();
		break;
	case 'e':
		extended = 1;
		break;
	case 'g':
		p = ARGF();
		if(p == nil)
			usage();
		w = strtol(p, &p, 10);
		h = atoi(++p);
		break;
	case 'k':
		karg = ARGF();
		if(karg == nil || *karg != ':')
			usage();
		break;
	case 'v':
		verbose++;
		break;
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		if(!extended)
			usage();
		setnetmtpt(netmt, NETPATHLEN, p);
		break;
	default:
		usage();
		break;
	}ARGEND

	
	if( karg != nil ) {
		if(argc ||  darg != nil )
			usage();
		vnckillsrv(netmt, &karg[1]);
		exits(nil);
	}

	if(argc == 0)
		argv = defargv;

	if(access(argv[0], OEXEC) < 0)
		sysfatal("can't exec %s: %r", argv[0]);

	/*
	 * background the server
	 */
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
		sysfatal("can't allocate private storage for vnc threads");

	initcompat();
	if(waserror())
		sysfatal("can't initialize screen: %s", up->error);

	fprint(2, "geometry is %dx%d\n", w, h);
	screeninit(w, h, "r5g6b5");
	poperror();

	/*
	 * fork the plan 9 file system device slaves
	 */
	n = exporter(devtab, &fd, &exportfd);

	/*
	 * scummy: we have to rebuild all of /dev because the
	 * underlying connection may go away, ripping out the old /dev/cons, etc.
	 */
	unmount(nil, "/dev");
	bind("#c", "/dev", MREPL);

	/*
	 * run the command
	 */
	switch(worker = rfork(RFPROC|RFFDG|RFNOTEG|RFNAMEG|RFREND)){
	case -1:
		sysfatal("can't fork: %r");
		break;
	case 0:
		if(!mounter("/dev", MBEFORE, fd, n))
			exits("errors");
		close(exportfd);
		close(0);
		close(1);
		close(2);
		open("/dev/cons", OREAD);
		open("/dev/cons", OWRITE);
		open("/dev/cons", OWRITE);
		fprint(2, "execing %s...\n", argv[0]);
		exec(argv[0], argv);
		fprint(2, "can't exec %s: %r\n", argv[0]);
		_exits(0);
	default:
		close(fd);
		break;
	}

	/*
	 * run the service
	 */
	srvfd = vncannounce(netmt, darg, adir);
	if(srvfd < 0)
		sysfatal("announce %s: %r", addr);

	if(verbose)
		fprint(2, "announced %s\n", adir);

	atexit(shutdown);
	notify(noteshut);
	while(1){
		cfd = listen(adir, ldir);
		if(cfd < 0)
			break;

		if(verbose)
			fprint(2, "received call at %s\n", ldir);

		s = accept(cfd, ldir);
		if(s < 0){
			close(cfd);
			continue;
		}

		vnc_newclient(s, cfd, extended);
	}
	exits(0);
}
