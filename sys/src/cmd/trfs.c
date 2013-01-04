#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>


enum {
	Maxmsglen	= IOHDRSZ + 8 * 1024,	// Max message size
	Nnames		= 128,			// Max # of names in a message
};

#define dprint	if(debug)print

Rune	altspc = L'·';
Rune	altlparen = L'«';
Rune	altrparen = L'»';
Rune	altquote = L'´';
Rune	altamp = L'­';

int	debug;
int	verbose;

uchar	rxbuf[Maxmsglen];
uchar	txbuf[Maxmsglen];
char	statbuf[Maxmsglen];
char	dirbuf[2*Maxmsglen];

Fidpool*fidpool;


static void
usage(void)
{
	fprint(2, "usage: %s [-Dv] [-RUNE] [-s srvfile] [-n addr] servename\n", argv0);
	exits("usage");
}


static ulong
getaux(Fid* fp)
{
	assert(sizeof(ulong) <= sizeof(fp->aux));
	return (uintptr)fp->aux;
}

static void
setaux(Fid* fp, ulong aux)
{
	assert(sizeof(ulong) <= sizeof(fp->aux));
	fp->aux = (void*) aux;
}

static char*
emalloc(int l)
{
	char* r;
	r = malloc(l);
	if (r == nil)
		sysfatal("not enough memory");
	return r;
}

int	nnames;
char*	names[Nnames];

void
cleannames(void)
{
	int i;

	for (i = 0; i < nnames; i++)
		free(names[i]);
	nnames = 0;
}

/* From Plan 9 to Unix */
char*
exportname(char* name)
{
	Rune r;
	int   nr;
	char *uxname;
	char *up;

	if (name == 0 || 
		(utfrune(name, altspc) == 0 &&
		 utfrune(name,altlparen) == 0 &&
		 utfrune(name,altrparen) == 0 &&
		 utfrune(name,altamp) == 0 &&
		 utfrune(name,altquote) == 0))
		return name;
	up = uxname = emalloc(strlen(name) + 1);
	names[nnames++] = uxname;
	while(*name != 0){
		nr = chartorune(&r, name);
		if (r == altspc)
			r = ' ';
		if (r == altlparen)
			r = '(';
		if (r == altrparen)
			r = ')';
		if (r == altamp)
			r = '&';
		if (r == altquote)
			r = '\'';
		up += runetochar(up, &r);
		name += nr;
	}
	*up = 0;
	return uxname;
}

/* From Unix to Plan 9 */
char*
importname(char* name)
{
	Rune r;
	int  nr;
	char *up;
	char *p9name;

	if (name == 0 ||
	   (strchr(name, ' ') == 0 && strchr(name, '(') == 0 &&
	    strchr(name, ')') == 0 && strchr(name, '&') == 0 &&
	    strchr(name, '\'')== 0))
		return name;
	p9name = emalloc(strlen(name) * 3 + 1);	// worst case: all blanks + 0
	up = p9name;
	names[nnames++] = p9name;
	while (*name != 0){
		nr = chartorune(&r, name);
		if (r == ' ')
			r = altspc;
		if (r == '(')
			r = altlparen;
		if (r == ')')
			r = altrparen;
		if (r == '&')
			r = altamp;
		if (r == '\'')
			r = altquote;
		up += runetochar(up, &r);
		name += nr;
	}
	*up = 0;
	return p9name;
}

static int
isfdir(Fcall* f, Fid **fpp)
{
	Fid*	fp;
	int	r;
	fp = lookupfid(fidpool, f->fid);
	if (fp == nil)
		return 0;
	r = (fp->qid.type&QTDIR);
	if (r)
		*fpp = fp;
	else
		closefid(fp);
	return r;
}

static int
getfcall(int fd, Fcall* f)
{
	int r;

	r = read9pmsg(fd, rxbuf, sizeof(rxbuf));
	if (r <= 0)
		return 0;
	if (convM2S(rxbuf, sizeof(rxbuf), f) == 0)
		return -1;
	return 1;
}

static int
putfcall(int fd, Fcall* f)
{
	int n;

	n = convS2M(f, txbuf, sizeof(txbuf));
	if (n == 0)
		return -1;
	if (write(fd, txbuf, n) != n)
		return -1;
	return n;
}

static void
twalk(Fcall* f)
{
	int	i;

	cleannames();
	for (i = 0; i < f->nwname; i++)
		f->wname[i] = exportname(f->wname[i]);
}

static void
tcreate(Fcall* f)
{
	cleannames();
	f->name = exportname(f->name);
}


// Dir read is tricky.
// We have to change the user supplied offset to match the sizes
// seen by the server. Sizes seen by client are greater than those
// seen by server since the change from ' ' to '␣' adds 2 bytes.
static void
tread(Fcall* f)
{
	Fid*	fp;

	fp = nil;
	if (!isfdir(f, &fp))
		return;
	f->count /= 3;	// sizes will grow upon return.
	if (fp == nil)
		sysfatal("can't find fid\n");
	if (f->offset == 0)
		setaux(fp, 0);
	f->offset -= getaux(fp);	// cumulative size delta
	closefid(fp);
}

static void
rread(Fcall* f)
{
	ulong	n, rn, nn, delta;
	Dir	d;
	Fid*	fp;

	if (!isfdir(f, &fp))
		return;
	if (f->count == 0)
		goto done;
	cleannames();
	for (n = nn = 0; n < f->count; n += rn){
		rn = convM2D((uchar*)f->data + n, f->count - n, &d, statbuf);
		if (rn <= BIT16SZ)
			break;
		d.name = importname(d.name);
		//dprint("⇒ %D\n", &d);
		nn += convD2M(&d, (uchar*)dirbuf + nn, sizeof(dirbuf) - nn);
	}
	delta = nn - n;
	setaux(fp, getaux(fp) + delta);
	f->count = nn;
	f->data = dirbuf;
done:
	closefid(fp);
}

static void
twstat(Fcall* f)
{
	Dir	d;

	cleannames();
	if (convM2D(f->stat, f->nstat, &d, statbuf) <= BIT16SZ)
		return;
	d.name = exportname(d.name);
	f->nstat = convD2M(&d, (uchar*)dirbuf, sizeof(dirbuf));
	f->stat = (uchar*)dirbuf;
	if (statcheck(f->stat, f->nstat) < 0)
		dprint("stat fails\n");
}

static void
rstat(Fcall* f)
{
	Dir	d;

	cleannames();
	convM2D(f->stat, f->nstat, &d, statbuf);
	d.name = importname(d.name);
	f->nstat = convD2M(&d, (uchar*)dirbuf, sizeof(dirbuf));
	f->stat = (uchar*)dirbuf;
	if (statcheck(f->stat, f->nstat) < 0)
		dprint("stat fails\n");
}

void
nop(Fid*)
{
}


static void
service(int cfd, int sfd, int dfd)
{
	Fcall	f;
	int	r;
	Fid*	fp;

	fidpool = allocfidpool(nop);
	for(;;){
		fp = nil;
		r = getfcall(cfd, &f);
		if (r <= 0){
			fprint(dfd, "trfs: getfcall %r\n");
			break;
		}
		if(verbose)
			fprint(dfd , "c→s %F\n", &f);
		switch(f.type){
		case Tclunk:
		case Tremove:
			// BUG in lib9p? removefid leaks fid.
			// is that what it should do?
			fp = lookupfid(fidpool, f.fid);
			if (fp != nil){
				removefid(fidpool, f.fid);
				closefid(fp);
				closefid(fp);
				fp = nil;
			}
			break;
		case Tcreate:
			tcreate(&f);
			// and also...
		case Topen:
			fp = allocfid(fidpool, f.fid);
			fp->aux = 0;
			break;
		case Tread:
			tread(&f);
			break;
		case Twalk:
			twalk(&f);
			break;
		case Twstat:
			twstat(&f);
			break;
		}
		if(verbose && debug)
			fprint(dfd , "c→s %F\n", &f);
		if (putfcall(sfd, &f) < 0)
			fprint(dfd , "can't putfcall: %r\n");
		

		r = getfcall(sfd, &f);
		if (r <= 0){
			fprint(dfd, "trfs: 2nd getfcall %r\n");
			break;
		}
		if (verbose)
			fprint(dfd, "c←s %F\n", &f);
		switch(f.type){
		case Ropen:
		case Rcreate:
			fp->qid = f.qid;
			break;
		case Rread:
			rread(&f);
			break;
		case Rstat:
			rstat(&f);
			break;
		}
		if(verbose && debug)
			fprint(dfd , "c←s %F\n", &f);
		if (putfcall(cfd, &f) < 0)
			fprint(dfd , "can't 2n dputfcall: %r\n");
		if (fp != nil)
			closefid(fp);
	}
}

void
main(int argc, char* argv[])
{
	char*	srv = nil;
	char*	sname = nil;
	char*	addr = nil;
	int	fd;
	int	p[2];

	ARGBEGIN{
	case 'D':
		debug = 1;
		break;
	case 'n':
		addr = EARGF(usage());
		break;
	case 'v':
		verbose = 1;
		break;
	case 's':
		sname = EARGF(usage());
		break;
	default:
		altspc = ARGC();
	}ARGEND;
	if (addr == nil){
		if (argc < 1)
			usage();
		srv = *argv;
		argc--;
	}
	if (argc > 0)
		usage();
	if (sname == nil)
		sname = (addr != nil) ? addr : "trfs";
	fmtinstall('D', dirfmt);
	fmtinstall('M', dirmodefmt);
	fmtinstall('F', fcallfmt);

	if (addr == nil)
		fd = open(srv, ORDWR);
	else
		fd = dial(netmkaddr(addr, "net", "9fs"), 0, 0, 0);
	if (fd < 0 || pipe(p) < 0)
		sysfatal("can't connect to  server %s: %r\n", (addr?addr:srv));
	if (postfd(sname, p[0]) < 0)
		sysfatal("can't post srv: %r\n");
	rfork(RFNOTEG);
	switch(rfork(RFPROC|RFNOTEG)){
	case 0:
		service(p[1], fd, 2);
		break;
	case -1:
		sysfatal("can't fork server: %r\n");
		break;
	}
	exits(nil);	
}
