#include <u.h>
#include <libc.h>
#include <libg.h>
#include <auth.h>
#include <fcall.h>
#include <bio.h>

enum {
	Qbitblt = 1,
	Qroot = CHDIR
};

typedef struct Fid Fid;
struct Fid
{
	int	busy;
	int	fid;
	Fid	*next;
	long	qidpath;
};

int		detail;
char		*bmdumpfile;
char		*outfile = "btrace.out";
Biobuf		*out;
Bitmap		*bhead;
Fid		*fidhead;
Rectangle	rrect;
int		rldepth;
int		rid;
int		rtype;
int		srvfd, bitfd;
int		srvpid, rcpid;
long		rminy, rmaxy;
Dir		rootdir = { ".", "btrace", "btrace", {Qroot, 0}, CHDIR|0777 };
Dir		bitbltdir = { "bitblt", "btrace", "btrace", {Qbitblt, 0}, 0666 };
Fcall		rhdr, thdr;
char		data[MAXMSG+MAXFDATA];
char		repdata[EMAXMSG];
char		errbuf[ERRLEN];

int		req(uchar *, int);
void		rep(uchar *, int);
Point		bgpt(uchar *);
Rectangle	bgrect(uchar *);
int		Aconv(void *, Fconv*);
int		Pconv(void *, Fconv*);
int		Rconv(void *, Fconv*);
void		dumpbmrows(uchar *, int, int, Rectangle);
void		printfontchars(uchar *, int);
void		printcmap(uchar *, int);
void		printoffsets(uchar *, Point pt, int);
Bitmap		*bmlook(int);
Fid		*fidlook(int);
void		srv(void);
char		*bitwrite(void);
char		*bitread(void);
void		catcher(void *, char *);

void
main(int argc, char **argv)
{
	int p[2], wpid;
	Waitmsg w;

	ARGBEGIN{
	case 'd':
		detail++;
		break;
	case 'b':
		bmdumpfile = ARGF();
		break;
	case 'o':
		outfile = ARGF();
		break;
	default:
	usage:
		fprint(2, "usage: btrace [-d[d]] [-o file] [-b bitmapfile]\n");
		exits("usage");
	}ARGEND

	notify(catcher);
	out = Bopen(outfile, OWRITE);
	if(!out) {
		fprint(2, "%s: can't open trace file %s\n", argv0, outfile);
		exits("can't open trace file");
	}
	fmtinstall('R', Rconv);
	fmtinstall('P', Pconv);
	fmtinstall('A', Aconv);
	/* fmtinstall('F', fcallconv); */

	bitfd = open("/dev/bitblt", ORDWR);
	if(bitfd < 0) {
		fprint(2, "%s: error opening /dev/bitblt: %r\n", argv0);
		exits("can't open /dev/bitblt");
	}
	if(pipe(p) < 0) {
		fprint(2, "%s: error making pipe: %r\n", argv0);
		exits("can't make pipe");
	}
	srvfd = p[1];
	switch((srvpid = rfork(RFPROC|RFNOTEG|RFFDG|RFNAMEG))) {
	case -1:
		fprint(2, "%s: error forking server: %r\n", argv0);
		exits("can't fork");
	case 0:
		close(p[0]);
		srv();
		exits(0);
	default:
		close(srvfd);
		close(bitfd);
	}

	if(mount(p[0], "/dev", MBEFORE, "") < 0) {
		fprint(2, "%s: error mounting on /dev: %r\n", argv0);
		exits("can't mount\n");
	}
	switch((rcpid = rfork(RFPROC|RFFDG))) {
	case -1:
		fprint(2, "%s: error forking shell: %r\n", argv0);
		exits("can't fork");
	case 0:
		execl("/bin/rc", "rc", 0);
	}
	wpid = wait(&w);
	if(wpid > 0) {
		if(wpid == rcpid)
			postnote(PNPROC, srvpid, "kill");
		else
			postnote(PNPROC, rcpid, "kill");
	}
	exits(0);
}

void
srv(void)
{
	long n, q;
	char *err;
	Fid *f, *nf;

	while((n = read(srvfd, data, sizeof data)) >= 0) {
		if(convM2S(data, &rhdr, n) == 0)
			break;
		/* fprint(2, "%s: fcall %F\n", argv0, &rhdr); */
		f = fidlook(rhdr.fid);
		q = f->qidpath;
		err = 0;
		nf = 0;
		switch(rhdr.type) {
		case Tflush:
		case Tnop:
			break;
		case Tsession:
			memset(thdr.authid, 0, sizeof(thdr.authid));
			memset(thdr.authdom, 0, sizeof(thdr.authdom));
			memset(thdr.chal, 0, sizeof(thdr.chal));
			break;
		case Tcreate:
		case Tremove:
		case Twstat:
			err = "permission denied";
			break;
		case Tattach:
			f->qidpath = Qroot;
			f->busy = 1;
			thdr.qid.path = Qroot;
			thdr.qid.vers = 0;
			break;
		case Tclone:
			nf = fidlook(rhdr.newfid);
			nf->busy = 1;
			nf->qidpath = q;
			thdr.qid.path = q;
			thdr.qid.vers = 0;
			break;
		case Tclwalk:
			nf = fidlook(rhdr.newfid);
			nf->busy = 1;
			f = nf;
			/* fall through */
		case Twalk:
			if(strcmp(rhdr.name, "bitblt") == 0) {
				f->qidpath = Qbitblt;
				thdr.qid.path = Qbitblt;
				thdr.qid.vers = 0;
			} else {
				err = "file does not exist";
				if(nf)
					nf->busy = 0;
			}
			break;
		case Topen:
			thdr.qid.path = q;
			thdr.qid.vers = 0;
			break;
		case Tclunk:
			f->busy = 0;
			f->qidpath = 0;
			break;
		case Tstat:
			convD2M((q==Qroot)? &rootdir : &bitbltdir, thdr.stat);
			break;
		case Tread:
			if(q == Qroot) {
				if(rhdr.offset%DIRLEN || rhdr.count%DIRLEN)
					err = "i/o error";
				else {
					thdr.data = data+MAXMSG;
					convD2M(&bitbltdir, thdr.data);
					if(rhdr.offset > 0 || rhdr.count < DIRLEN)
						thdr.count = 0;
					else
						thdr.count = DIRLEN;
				}
			} else
				err = bitread();
			break;
		case Twrite:
			if(q == Qroot)
				err = "write to directory";
			else
				err = bitwrite();
			break;
		default:
			fprint(2, "%s: fcall unknown message type: %d\n", argv0, rhdr.type);
			exits("unknown message type");
		}
		if(err) {
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		} else
			thdr.type = rhdr.type+1;
		thdr.tag = rhdr.tag;
		thdr.fid = rhdr.fid;
		n = convS2M(&thdr, data);
		if(write(srvfd, data, n) < 0) {
			fprint(2, "%s: error writing server reply: %r\n", argv0);
			exits("mount write");
		}
	}
	fprint(2, "%s: mount read error: %r\n", argv0);
	exits("mount read");
}

/*
 * rhdr has a write message to the bitblt file.
 * It may hold any number of bitblt protocol requests,
 * but only the last, at most, should call for a reply.
 * Format the requests to the out file.
 * Set thdr.count to amount consumed.
 * Return 0 if ok, else an error message
 */
char *
bitwrite(void)
{
	long n, m;
	uchar *p;

	n = rhdr.count;
	p = (uchar *)rhdr.data;
	if(detail)
		Bprint(out, "*** Request length %d\n", rhdr.count);
	while(n > 0) {
		rtype = *p;
		m = req(p, n);
		if(m == 0)
			exits("bad request format");
		if(write(bitfd, p, m) != m) {
			errstr(errbuf);
			Bprint(out, "error writing /dev/bitblt: %s\n", errbuf);
			return errbuf;
		}
		n -= m;
		p += m;
	}
	thdr.count = rhdr.count - n;
	Bflush(out);
	return 0;
}

/*
 * rhdr has a read message to the bitblt file.
 * Read from the real bitblt fd,
 * format the reply to the out file.
 * Set thdr.data to point at the reply,
 * and thdr.count to amount returned.
 * Return 0 if ok, else an error message
 */
char *
bitread(void)
{
	long n;

	if((n = read(bitfd, repdata, rhdr.count)) < 0) {
		errstr(errbuf);
		Bprint(out, "error reading /dev/bitblt: %s\n", errbuf);
		return errbuf;
	}
	if(detail)
		Bprint(out, "*** Reply length %d\n", n);
	rep((uchar *)repdata, n);
	thdr.data = repdata;
	thdr.count = n;
	Bflush(out);
	return 0;
}

/* Format one request, return number of bytes consumed */
int
req(uchar *p, int m)
{
	int len;
	int v, i;
	ulong ws, l;
	long miny, maxy, t;
	Bitmap *bm;
	Point pt;

	switch(*p) {

	default:
		Bprint(out, "bad bitblt request 0x%x\n", *p);
		len = 0;
		break;

	case 'a':
		len = 18;
		rldepth = p[1];
		rrect = bgrect(p+2);
		Bprint(out, "[a] bitmap allocate, ldepth=%d, rect=%R\n",
			rldepth, rrect);
		break;

	case 'b':
		len = 31;
		Bprint(out, "[b] bitblt, dst=%d, dst pt=%P, src=%d, src rect=%R, code=%A\n",
			BGSHORT(p+1), bgpt(p+3), BGSHORT(p+11), bgrect(p+13), BGSHORT(p+29));
		break;

	case 'c':
		if(m == 1){
			len = 1;
			Bprint(out, "[c] cursorswitch, to default cursor\n");
			break;
		}
		len = 73;
		Bprint(out, "[c] cursorswitch, offset=%P\n", bgpt(p+1));
		if(detail) {
			Bprint(out, "\t     clr       set\n");
			for(i=0; i<32; i += 2)
				Bprint(out, "\t%.4x%.4x  %.4x%.4x\n",
					p[9+i], p[10+i], p[41+i], p[42+i]);
		}
		break;

	case 'e':
		v = BGSHORT(p+14);
		len = 16 + 2*v;
		pt = bgpt(p+3);
		Bprint(out, "[e] polysegment, bitmap=%d, pt=%P, value=%d, code=%A\n    offsets: ",
			BGSHORT(p+1), pt, p[11], BGSHORT(p+12));
		printoffsets(p+16, pt, v);
		break;

	case 'f':
		len = 3;
		Bprint(out, "[f] bitmap free, id=%d\n", BGSHORT(p+1));
		break;

	case 'g':
		len = 3;
		Bprint(out, "[g] subfont free, id=%d\n", BGSHORT(p+1));
		break;

	case 'h':
		len = 3;
		Bprint(out, "[h] font free, id=%d\n", BGSHORT(p+1));
		break;

	case 'i':
		len = 1;
		Bprint(out, "[i] init\n");
		break;

	case 'j':
		len = 9;
		Bprint(out, "[j] subfont cache check, qid=%lux,%lux\n",
			BGLONG(p+1), BGLONG(p+5));
		break;

	case 'k':
		v = BGSHORT(p+1);
		len = 15+6*(v+1);
		Bprint(out, "[k] subfont allocate, n=%d, height=%d, ascent=%d, bitmap=%d, qid=%lux, %lux\n",
			v, p[3], p[4], BGSHORT(p+5), BGLONG(p+7), BGLONG(p+11));
		printfontchars(p+15, v);
		break;

	case 'l':
		len = 22;
		Bprint(out, "[l] line segment, bitmap=%d, pt1=%P, pt2=%P, value=%d, code=%A\n",
			BGSHORT(p+1), bgpt(p+3), bgpt(p+11), p[19], BGSHORT(p+20));
		break;

	case 'm':
		len = 3;
		rid = BGSHORT(p+1);
		Bprint(out, "[m] read colormap, bitmap id=%d\n", rid);
		break;

	case 'n':
		len = 7;
		Bprint(out, "[n] font allocate, height=%d, ascent=%d, ldepth=%d, ncache=%d\n",
			p[1], p[2], BGSHORT(p+3), BGSHORT(p+5));
		break;

	case 'p':
		len = 14;
		Bprint(out, "[p] point, bitmap=%d, pt=%P, value=%d, code=%A\n",
			BGSHORT(p+1), bgpt(p+3), p[11], BGSHORT(p+12));
		break;

	case 'q':
		len = 19;
		Bprint(out, "[q] set clip rectangle, bitmap=%d, rect=%R\n",
			BGSHORT(p+1), bgrect(p+3));
		break;

	case 'r':
		len = 11;
		rid = BGSHORT(p+1);
		rminy = BGLONG(p+3);
		rmaxy = BGLONG(p+7);
		Bprint(out, "[r] bitmap read, bitmap=%d, miny=%d, maxy=%d\n",
			rid, rminy, rmaxy);
		break;

	case 's':
		v = BGSHORT(p+15);
		len = 17 + 2*v;
		Bprint(out, "[s] string, bitmap=%d, pt=%P, font=%d, code=%A, n=%d\n",
			BGSHORT(p+1), bgpt(p+3), BGSHORT(p+11), BGSHORT(p+13), v);
		if(detail) {
			Bprint(out, "\t");
			for(i = 0; i<v; i++)
				Bprint(out, "%d ", BGSHORT(p+17+2*i));
		}
		break;

	case 't':
		len = 23;
		Bprint(out, "[t] texture, dst=%d, rect=%R, src=%d, code=%A\n",
			BGSHORT(p+1), bgrect(p+3), BGSHORT(p+19), BGSHORT(p+21));
		break;

	case 'v':
		len = 7;
		Bprint(out, "[v] font cache clear, font=%d, ncache=%d, width=%d\n",
			BGSHORT(p+1), BGSHORT(p+3), BGSHORT(p+5));
		break;

	case 'w':
		v = BGSHORT(p+1);
		bm = bmlook(v);
		miny = BGLONG(p+3);
		maxy = BGLONG(p+7);
		ws = 1<<(3-bm->ldepth);	/* pixels per byte */
		/* set l to number of bytes of incoming data per scan line */
		if(bm->r.min.x >= 0)
			l = (bm->r.max.x+ws-1)/ws - bm->r.min.x/ws;
		else{	/* make positive before divide */
			t = (-bm->r.min.x)+ws-1;
			t = (t/ws)*ws;
			l = (t+bm->r.max.x+ws-1)/ws;
		}
		len = 11 + l*(maxy-miny);
		Bprint(out, "[w] bitmap write, bitmap=%d, miny=%d, maxy=%d\n",
			v, miny, maxy);
		dumpbmrows(p+11, l, bm->ldepth,
			Rect(bm->r.min.x, miny, bm->r.max.x, maxy));
		break;

	case 'x':
		len = 9;
		Bprint(out, "[x] cursorset, pt=%P\n", bgpt(p+1));
		break;

	case 'y':
		len = 9;
		Bprint(out, "[y] load font char, font=%d, cache index=%d, subfont=%d, subfont index=%d\n",
			BGSHORT(p+1), BGSHORT(p+3), BGSHORT(p+5), BGSHORT(p+7));
		break;

	case 'z':
		v = BGSHORT(p+1);
		bm = bmlook(v);
		l = 1 << (1 << bm->ldepth);
		len = 3 + 12*l;
		Bprint(out, "[z] colormap write, bitmap=%d\n", v);
		printcmap(p+3, l);
		break;
	}
	if(m < len) {
		Bprint(out, "\tEbadblt: request length %d, should be %d\n", m, len);
		len = 0;
	}
	return len;
}

void
rep(uchar *p, int n)
{
	int len, v;
	Bitmap *bm;
	long t;
	ulong l, ws;

	len = -n;
	switch(rtype) {
	case 'i':
		len = 34;
		if(p[0] != 'I') {
			Bprint(out, "Expected reply 'I', got '%c'\n", p[0]);
			break;
		}
		bm = bmlook(0);
		bm->ldepth = p[1];
		bm->r = bgrect(p+2);
		Bprint(out, "[I] ldepth=%d, rect=%R, clip rect=%R\n",
			p[1], bm->r, bgrect(p+18));
		if(n >= 34 + 3*12) {
			v = atoi((char *)(p+34));
			Bprint(out, "\tdefont, n=%d, height=%d, ascent=%d\n",
				v, atoi((char *)(p+34+12)), atoi((char *)(p+34+24)));
			printfontchars(p+34+3*12, v);
			len = 34+3*12+6*(v+1);
		}
		break;
	case 'a':
		len = 3;
		if(p[0] != 'A') {
			Bprint(out, "Expected reply 'A', got '%c'\n", p[0]);
			break;
		}
		v = BGSHORT(p+1);
		Bprint(out, "[A] bitmap=%d\n", v);
		bm = bmlook(v);
		bm->r = rrect;
		bm->ldepth = rldepth;
		break;
	case 'k':
		len = 3;
		if(p[0] != 'K') {
			Bprint(out, "Expected reply 'K', got '%c'\n", p[0]);
			break;
		}
		Bprint(out, "[K] subfont=%d\n", BGSHORT(p+1));
		break;
	case 'j':
		if(p[0] != 'J') {
			Bprint(out, "Expected reply 'J', got '%c'\n", p[0]);
			break;
		}
		v = atoi((char *)(p+3));
		Bprint(out, "[J] subfont=%d, n=%d, height=%d, ascent=%d\n",
			BGSHORT(p+1), v, atoi((char *)(p+3+12)),
			atoi((char *)(p+3+24)));
		printfontchars(p+3+3*12, v);
		len = 3+3*12+6*(v+1);
		break;
	case 'n':
		len = 3;
		if(p[0] != 'N') {
			Bprint(out, "Expected reply 'N', got '%c'\n", p[0]);
			break;
		}
		Bprint(out, "[N] font=%d\n", BGSHORT(p+1));
		break;
	case 'm':
		bm = bmlook(rid);
		v = 1 << (1 << bm->ldepth);
		len = 12*v;
		printcmap(p, v);
		break;
	case 'r':
		bm = bmlook(rid);
		ws = 1<<(3-bm->ldepth);	/* pixels per byte */
		/* set l to number of bytes of incoming data per scan line */
		if(bm->r.min.x >= 0)
			l = (bm->r.max.x+ws-1)/ws - bm->r.min.x/ws;
		else{	/* make positive before divide */
			t = (-bm->r.min.x)+ws-1;
			t = (t/ws)*ws;
			l = (t+bm->r.max.x+ws-1)/ws;
		}
		dumpbmrows(p, l, bm->ldepth,
			Rect(bm->r.min.x, rminy, bm->r.max.x, rmaxy));
		len = l*(rmaxy-rminy);
		break;
	}
	if(n != len)
		Bprint(out, "\tBad reply length %d, should be %d\n", n, len);
}

char *fcnames[] = {
	[Zero]		"0",
	[DnorS]		"~(D|S)",
	[DandnotS]	"D&~S",
	[notS]		"~S",
	[notDandS]	"~D&S",
	[notD]		"~D",
	[DxorS]		"D^S",
	[DnandS]	"~(D&S)",
	[DandS]		"D&S",
	[DxnorS]	"~(D^S)",
	[D]		"D",
	[DornotS]	"D|~S",
	[S]		"S",
	[notDorS]	"~D|S",
	[DorS]		"D|S",
	[F]		"F",
};

int
Aconv(void *oa, Fconv *f)
{
	int fc;

	fc = *(int *)oa;
	strconv(fcnames[fc & 0xF], f);
	return sizeof(int);
}

void
dumpbmrows(uchar *p, int bytes2row, int ldepth, Rectangle r)
{
	Biobuf *f;
	int i, j;

	if(detail > 1) {
		for(i=0; i<Dy(r); i++) {
			Bprint(out, "\t");
			for(j=0; j<bytes2row; j++) {
				Bprint(out, "%.4x", p[i*bytes2row+j]);
				if(j%2 == 1) Bprint(out, " ");
			}
			Bprint(out, "\n");
		}
	}
	if(!bmdumpfile)
		return;
	f = Bopen(bmdumpfile, OWRITE);
	Bprint(f, "%11d %11d %11d %11d %11d ",
		ldepth, r.min.x, r.min.y, r.max.x, r.max.y);
	Bwrite(f, p, Dy(r)*bytes2row);
	Bterm(f);
}

void
printcmap(uchar *p, int n)
{
	int i;

	if(!detail)
		return;
	Bprint(out, "\t      red    green     blue\n");
	for(i=0; i<n; i++) {
		Bprint(out, "\t%9lux%9lux%9lux\n",
			BGLONG(p), BGLONG(p+4), BGLONG(p+8));
		p += 12;
	}
}

void
printfontchars(uchar *p, int n)
{
	int j;

	if(!detail)
		return;
	Bprint(out, "\tx\ttop\tbot\tleft\twidth\n");
	for(j=0; j<=n; j++) {
		Bprint(out, "\t%d\t%d\t%d\t%d\t%d\n",
			BGSHORT(p), p[2], p[3], p[4], p[5]);
		p += 6;
	}
}

void
printoffsets(uchar *p, Point pt, int n)
{
	int j;
	Point del;

	for(j = 0; j<=n; j++) {
		del = Pt(((signed char *)p)[0], ((signed char *)p)[1]);
		pt = add(del,pt);
		Bprint(out, "+%P=%P, ", del, pt);
		p += 2;
		if(j && (j%4) == 0)
			Bprint(out, "\n\t");
	}
	if((j-1)%4 != 0)
		Bprint(out, "\n");
}

Point
bgpt(uchar *p)
{
	return Pt(BGLONG(p), BGLONG(p+4));
}

Rectangle
bgrect(uchar *p)
{
	return Rpt(bgpt(p), bgpt(p+8));
}

Bitmap *
bmlook(int id)
{
	Bitmap *b;

	for(b = bhead; b; b = b->cache)
		if(id == b->id)
			return b;

	b = malloc(sizeof(Bitmap));
	memset(b, 0, sizeof(Bitmap));
	b->id = id;
	b->cache = bhead;
	bhead = b;
	return b;
}

Fid *
fidlook(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fidhead; f; f = f->next)
		if(f->fid == fid)
			return f;
		else if(!ff && !f->busy)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = malloc(sizeof *f);
	f->qidpath = 0;
	f->fid = fid;
	f->next = fidhead;
	fidhead = f;
	return f;
}

void
catcher(void *a, char *text)
{
	USED(a);
	if(strcmp(text, "interrupted") != 0)
		if(srvpid)
			postnote(PNPROC, srvpid, text);
	noted(NDFLT);
}
