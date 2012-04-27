#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

#include <auth.h>
#include <fcall.h>
#include <9p.h>

enum {
	Qindexhtml,
	Qindextxt,
	Qraw,
	Qhistoryhtml,
	Qhistorytxt,
	Qdiffhtml,
	Qedithtml,
	Qwerrorhtml,
	Qwerrortxt,
	Qhttplogin,
	Nfile,
};

static char *filelist[] = {
	"index.html",
	"index.txt",
	"current",
	"history.html",
	"history.txt",
	"diff.html",
	"edit.html",
	"werror.html",
	"werror.txt",
	".httplogin",
};

static int needhist[Nfile] = {
[Qhistoryhtml] 1,
[Qhistorytxt] 1,
[Qdiffhtml] 1,
};

/*
 * The qids are <8-bit type><16-bit page number><16-bit page version><8-bit file index>.
 */
enum {		/* <8-bit type> */
	Droot = 1,
	D1st,
	D2nd,
	Fnew,
	Fmap,
	F1st,
	F2nd,
};

uvlong
mkqid(int type, int num, int vers, int file)
{
	return ((uvlong)type<<40) | ((uvlong)num<<24) | (vers<<8) | file;
}

int
qidtype(uvlong path)
{
	return (path>>40)&0xFF;
}

int
qidnum(uvlong path)
{
	return (path>>24)&0xFFFF;
}

int
qidvers(uvlong path)
{
	return (path>>8)&0xFFFF;
}

int
qidfile(uvlong path)
{
	return path&0xFF;
}

typedef struct Aux Aux;
struct Aux {
	String *name;
	Whist *w;
	int n;
	ulong t;
	String *s;
	Map *map;
};

static void
fsattach(Req *r)
{
	Aux *a;

	if(r->ifcall.aname && r->ifcall.aname[0]){
		respond(r, "invalid attach specifier");
		return;
	}

	a = emalloc(sizeof(Aux));
	r->fid->aux = a;
	a->name = s_copy(r->ifcall.uname);

	r->ofcall.qid = (Qid){mkqid(Droot, 0, 0, 0), 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

static String *
httplogin(void)
{
	String *s=s_new();
	Biobuf *b;

	if((b = wBopen(".httplogin", OREAD)) == nil)
		goto Return;

	while(s_read(b, s, Bsize) > 0)
		;
	Bterm(b);

Return:
	return s;
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	char *q;
	int i, isdotdot, n, t;
	uvlong path;
	Aux *a;
	Whist *wh;
	String *s;

	isdotdot = strcmp(name, "..")==0;
	n = strtoul(name, &q, 10);
	path = fid->qid.path;
	a = fid->aux;

	switch(qidtype(path)){
	case 0:
		return "wikifs: bad path in server (bug)";

	case Droot:
		if(isdotdot){
			*qid = fid->qid;
			return nil;
		}
		if(strcmp(name, "new")==0){
			*qid = (Qid){mkqid(Fnew, 0, 0, 0), 0, 0};
			return nil;
		}
		if(strcmp(name, "map")==0){
			*qid = (Qid){mkqid(Fmap, 0, 0, 0), 0, 0};
			return nil;
		}
		if((*q!='\0' || (wh=getcurrent(n))==nil)
		&& (wh=getcurrentbyname(name))==nil)
			return "file does not exist";
		*qid = (Qid){mkqid(D1st, wh->n, 0, 0), wh->doc->time, QTDIR};
		a->w = wh;
		return nil;

	case D1st:
		if(isdotdot){
			*qid = (Qid){mkqid(Droot, 0, 0, 0), 0, QTDIR};
			return nil;
		}

		/* handle history directories */
		if(*q == '\0'){
			if((wh = gethistory(qidnum(path))) == nil)
				return "file does not exist";
			for(i=0; i<wh->ndoc; i++)
				if(wh->doc[i].time == n)
					break;
			if(i==wh->ndoc){
				closewhist(wh);
				return "file does not exist";
			}
			closewhist(a->w);
			a->w = wh;
			a->n = i;
			*qid = (Qid){mkqid(D2nd, qidnum(path), i, 0), wh->doc[i].time, QTDIR};
			return nil;
		}

		/* handle files other than index */
		for(i=0; i<nelem(filelist); i++){
			if(strcmp(name, filelist[i])==0){
				if(needhist[i]){
					if((wh = gethistory(qidnum(path))) == nil)
						return "file does not exist";
					closewhist(a->w);
					a->w = wh;
				}
				*qid = (Qid){mkqid(F1st, qidnum(path), 0, i), a->w->doc->time, 0};
				goto Gotfile;
			}
		}
		return "file does not exist";

	case D2nd:
		if(isdotdot){
			/*
			 * Can't use a->w[a->ndoc-1] because that
			 * might be a failed write rather than the real one.
			 */
			*qid = (Qid){mkqid(D1st, qidnum(path), 0, 0), 0, QTDIR};
			if((wh = getcurrent(qidnum(path))) == nil)
				return "file does not exist";
			closewhist(a->w);
			a->w = wh;
			a->n = 0;
			return nil;
		}
		for(i=0; i<=Qraw; i++){
			if(strcmp(name, filelist[i])==0){
				*qid = (Qid){mkqid(F2nd, qidnum(path), qidvers(path), i), a->w->doc->time, 0};
				goto Gotfile;
			}
		}
		return "file does not exist";

	default:
		return "bad programming";
	}
	/* not reached */

Gotfile:
	t = qidtype(qid->path);
	switch(qidfile(qid->path)){
	case Qindexhtml:
		s = tohtml(a->w, a->w->doc+a->n,
			t==F1st? Tpage : Toldpage);
		break;
	case Qindextxt:
		s = totext(a->w, a->w->doc+a->n,
			t==F1st? Tpage : Toldpage);
		break;
	case Qraw:
		s = s_copy(a->w->title);
		s = s_append(s, "\n");
		s = doctext(s, &a->w->doc[a->n]);
		break;
	case Qhistoryhtml:
		s = tohtml(a->w, a->w->doc+a->n, Thistory);
		break;
	case Qhistorytxt:
		s = totext(a->w, a->w->doc+a->n, Thistory);
		break;
	case Qdiffhtml:
		s = tohtml(a->w, a->w->doc+a->n, Tdiff);
		break;
	case Qedithtml:
		s = tohtml(a->w, a->w->doc+a->n, Tedit);
		break;
	case Qwerrorhtml:
		s = tohtml(a->w, a->w->doc+a->n, Twerror);
		break;
	case Qwerrortxt:
		s = totext(a->w, a->w->doc+a->n, Twerror);
		break;
	case Qhttplogin:
		s = httplogin();
		break;
	default:
		return "internal error";
	}
	a->s = s;
	return nil;
}

static void
fsopen(Req *r)
{
	int t;
	uvlong path;
	Aux *a;
	Fid *fid;
	Whist *wh;

	fid = r->fid;
	path = fid->qid.path;
	t = qidtype(fid->qid.path);
	if((r->ifcall.mode != OREAD && t != Fnew && t != Fmap)
	|| (r->ifcall.mode&ORCLOSE)){
		respond(r, "permission denied");
		return;
	}

	a = fid->aux;
	switch(t){
	case Droot:
		currentmap(0);
		rlock(&maplock);
		a->map = map;
		incref(map);
		runlock(&maplock);
		respond(r, nil);
		break;
		
	case D1st:
		if((wh = gethistory(qidnum(path))) == nil){
			respond(r, "file does not exist");
			return;
		}
		closewhist(a->w);
		a->w = wh;
		a->n = a->w->ndoc-1;
		r->ofcall.qid.vers = wh->doc[a->n].time;
		r->fid->qid = r->ofcall.qid;
		respond(r, nil);
		break;
		
	case D2nd:
		respond(r, nil);
		break;

	case Fnew:
		a->s = s_copy("");
		respond(r, nil);
		break;

	case Fmap:
	case F1st:
	case F2nd:
		respond(r, nil);
		break;

	default:
		respond(r, "programmer error");
		break;
	}
}

static char*
fsclone(Fid *old, Fid *new)
{
	Aux *a;

	a = emalloc(sizeof(*a));
	*a = *(Aux*)old->aux;
	if(a->s)
		s_incref(a->s);
	if(a->w)
		incref(a->w);
	if(a->map)
		incref(a->map);
	if(a->name)
		s_incref(a->name);
	new->aux = a;
	new->qid = old->qid;

	return nil;
}

static void
fsdestroyfid(Fid *fid)
{
	Aux *a;

	a = fid->aux;
	if(a==nil)
		return;

	if(a->name)
		s_free(a->name);
	if(a->map)
		closemap(a->map);
	if(a->s)
		s_free(a->s);
	if(a->w)
		closewhist(a->w);
	free(a);
	fid->aux = nil;
}

static void
fillstat(Dir *d, uvlong path, ulong tm, ulong length)
{
	char tmp[32], *p;
	int type;

	memset(d, 0, sizeof(Dir));
	d->uid = estrdup9p("wiki");
	d->gid = estrdup9p("wiki");

	switch(qidtype(path)){
	case Droot:
	case D1st:
	case D2nd:
		type = QTDIR;
		break;
	default:
		type = 0;
		break;
	}
	d->qid = (Qid){path, tm, type};

	d->atime = d->mtime = tm;
	d->length = length;
	if(qidfile(path) == Qedithtml)
		d->atime = d->mtime = time(0);

	switch(qidtype(path)){
	case Droot:
		d->name = estrdup("/");
		d->mode = DMDIR|0555;
		break;

	case D1st:
		d->name = numtoname(qidnum(path));
		if(d->name == nil)
			d->name = estrdup("<dead>");
		for(p=d->name; *p; p++)
			if(*p==' ')
				*p = '_';
		d->mode = DMDIR|0555;
		break;

	case D2nd:
		snprint(tmp, sizeof tmp, "%lud", tm);
		d->name = estrdup(tmp);
		d->mode = DMDIR|0555;
		break;

	case Fmap:
		d->name = estrdup("map");
		d->mode = 0666;
		break;

	case Fnew:
		d->name = estrdup("new");
		d->mode = 0666;
		break;

	case F1st:
		d->name = estrdup(filelist[qidfile(path)]);
		d->mode = 0444;
		break;

	case F2nd:
		d->name = estrdup(filelist[qidfile(path)]);
		d->mode = 0444;
		break;

	default:
		print("bad qid path 0x%.8llux\n", path);
		break;
	}
}

static void
fsstat(Req *r)
{
	Aux *a;
	Fid *fid;
	ulong t;

	t = 0;
	fid = r->fid;
	if((a = fid->aux) && a->w)
		t = a->w->doc[a->n].time;

	fillstat(&r->d, fid->qid.path, t, a->s ? s_len(a->s) : 0);
	respond(r, nil);
}

typedef struct Bogus Bogus;
struct Bogus {
	uvlong path;
	Aux *a;
};

static int
rootgen(int i, Dir *d, void *aux)
{
	Aux *a;
	Bogus *b;

	b = aux;
	a = b->a;
	switch(i){
	case 0:	/* new */
		fillstat(d, mkqid(Fnew, 0, 0, 0), a->map->t, 0);
		return 0;
	case 1:	/* map */
		fillstat(d, mkqid(Fmap, 0, 0, 0), a->map->t, 0);
		return 0;
	default:	/* first-level directory */
		i -= 2;
		if(i >= a->map->nel)
			return -1;
		fillstat(d, mkqid(D1st, a->map->el[i].n, 0, 0), a->map->t, 0);
		return 0;
	}
}

static int
firstgen(int i, Dir *d, void *aux)
{
	ulong t;
	Bogus *b;
	int num;
	Aux *a;

	b = aux;
	num = qidnum(b->path);
	a = b->a;
	t = a->w->doc[a->n].time;

	if(i < Nfile){	/* file in first-level directory */
		fillstat(d, mkqid(F1st, num, 0, i), t, 0);
		return 0;
	}
	i -= Nfile;

	if(i < a->w->ndoc){	/* second-level (history) directory */
		fillstat(d, mkqid(D2nd, num, i, 0), a->w->doc[i].time, 0);
		return 0;
	}
	//i -= a->w->ndoc;

	return -1;
}

static int
secondgen(int i, Dir *d, void *aux)
{
	Bogus *b;
	uvlong path;
	Aux *a;

	b = aux;
	path = b->path;
	a = b->a;

	if(i <= Qraw){	/* index.html, index.txt, raw */
		fillstat(d, mkqid(F2nd, qidnum(path), qidvers(path), i), a->w->doc[a->n].time, 0);
		return 0;
	}
	//i -= Qraw;

	return -1;
}

static void
fsread(Req *r)
{
	char *t, *s;
	uvlong path;
	Aux *a;
	Bogus b;

	a = r->fid->aux;
	path = r->fid->qid.path;
	b.a = a;
	b.path = path;
	switch(qidtype(path)){
	default:
		respond(r, "cannot happen (bad qid)");
		return;

	case Droot:
		if(a == nil || a->map == nil){
			respond(r, "cannot happen (no map)");
			return;
		}
		dirread9p(r, rootgen, &b);
		respond(r, nil);
		return;
		
	case D1st:
		if(a == nil || a->w == nil){
			respond(r, "cannot happen (no wh)");
			return;
		}
		dirread9p(r, firstgen, &b);
		respond(r, nil);
		return;
		
	case D2nd:
		dirread9p(r, secondgen, &b);
		respond(r, nil);
		return;

	case Fnew:
		if(a->s){
			respond(r, "protocol botch");
			return;
		}
		/* fall through */
	case Fmap:
		t = numtoname(a->n);
		if(t == nil){
			respond(r, "unknown name");
			return;
		}
		for(s=t; *s; s++)
			if(*s == ' ')
				*s = '_';
		readstr(r, t);
		free(t);
		respond(r, nil);
		return;

	case F1st:
	case F2nd:
		if(a == nil || a->s == nil){
			respond(r, "cannot happen (no s)");
			return;
		}
		readbuf(r, s_to_c(a->s), s_len(a->s));
		respond(r, nil);
		return;
	}
}

typedef struct Sread Sread;
struct Sread {
	char *rp;
};

static char*
Srdline(void *v, int c)
{
	char *p, *rv;
	Sread *s;

	s = v;
	if(s->rp == nil)
		rv = nil;
	else if(p = strchr(s->rp, c)){
		*p = '\0';
		rv = s->rp;
		s->rp = p+1;
	}else{
		rv = s->rp;
		s->rp = nil;
	}
	return rv;
}

static void
responderrstr(Req *r)
{
	char buf[ERRMAX];

	rerrstr(buf, sizeof buf);
	if(buf[0] == '\0')
		strcpy(buf, "unknown error");
	respond(r, buf);
}

static void
fswrite(Req *r)
{
	char *author, *comment, *net, *err, *p, *title, tmp[40];
	int rv, n;
	ulong t;
	Aux *a;
	Fid *fid;
	Sread s;
	String *stmp;
	Whist *w;

	fid = r->fid;
	a = fid->aux;
	switch(qidtype(fid->qid.path)){
	case Fmap:
		stmp = s_nappend(s_reset(nil), r->ifcall.data, r->ifcall.count);
		a->n = nametonum(s_to_c(stmp));
		s_free(stmp);
		if(a->n < 0)
			respond(r, "name not found");
		else
			respond(r, nil);
		return;
	case Fnew:
		break;
	default:
		respond(r, "cannot happen");
		return;
	}

	if(a->s == nil){
		respond(r, "protocol botch");
		return;
	}
	if(r->ifcall.count==0){	/* do final processing */
		s.rp = s_to_c(a->s);
		w = nil;
		err = "bad format";
		if((title = Srdline(&s, '\n')) == nil){
		Error:
			if(w)
				closewhist(w);
			s_free(a->s);
			a->s = nil;
			respond(r, err);
			return;
		}

		w = emalloc(sizeof(*w));
		incref(w);
		w->title = estrdup(title);

		t = 0;
		author = estrdup(s_to_c(a->name));

		comment = nil;
		while(s.rp && *s.rp && *s.rp != '\n'){
			p = Srdline(&s, '\n');
			assert(p != nil);
			switch(p[0]){
			case 'A':
				free(author);
				author = estrdup(p+1);
				break;
			case 'D':
				t = strtoul(p+1, &p, 10);
				if(*p != '\0')
					goto Error;
				break;
			case 'C':
				free(comment);
				comment = estrdup(p+1);
				break;
			}
		}

		w->doc = emalloc(sizeof(w->doc[0]));
		w->doc->time = time(0);
		w->doc->comment = comment;

		if(net = r->pool->srv->aux){
			p = emalloc(strlen(author)+10+strlen(net));
			strcpy(p, author);
			strcat(p, " (");
			strcat(p, net);
			strcat(p, ")");
			free(author);
			author = p;
		}
		w->doc->author = author;

		if((w->doc->wtxt = Brdpage(Srdline, &s)) == nil){
			err = "empty document";
			goto Error;
		}

		w->ndoc = 1;
		if((n = allocnum(w->title, 0)) < 0)
			goto Error;
		sprint(tmp, "D%lud\n", w->doc->time);
		a->s = s_reset(a->s);
		a->s = doctext(a->s, w->doc);
		rv = writepage(n, t, a->s, w->title);
		s_free(a->s);
		a->s = nil;
		a->n = n;
		closewhist(w);
		if(rv < 0)
			responderrstr(r);
		else
			respond(r, nil);
		return;
	}

	if(s_len(a->s)+r->ifcall.count > Maxfile){
		respond(r, "file too large");
		s_free(a->s);
		a->s = nil;
		return;
	}
	a->s = s_nappend(a->s, r->ifcall.data, r->ifcall.count);
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

Srv wikisrv = {
.attach=	fsattach,
.destroyfid=	fsdestroyfid,
.clone=	fsclone,
.walk1=	fswalk1,
.open=	fsopen,
.read=	fsread,
.write=	fswrite,
.stat=	fsstat,
};

void
usage(void)
{
	fprint(2, "usage: wikifs [-D] [-a addr]... [-m mtpt] [-p perm] [-s service] dir\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char **addr;
	int i, naddr;
	char *buf;
	char *service, *mtpt;
	ulong perm;
	Dir d, *dp;
	Srv *s;

	naddr = 0;
	addr = nil;
	perm = 0;
	service = nil;
	mtpt = "/mnt/wiki";
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'a':
		if(naddr%8 == 0)
			addr = erealloc(addr, (naddr+8)*sizeof(addr[0]));
		addr[naddr++] = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'M':
		mtpt = nil;
		break;
	case 'p':
		perm = strtoul(EARGF(usage()), nil, 8);
		break;
	case 's':
		service = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1)
		usage();

	if((dp = dirstat(argv[0])) == nil)
		sysfatal("dirstat %s: %r", argv[0]);
	if((dp->mode&DMDIR) == 0)
		sysfatal("%s: not a directory", argv[0]);
	free(dp);
	wikidir = argv[0];

	currentmap(0);

	for(i=0; i<naddr; i++)
		listensrv(&wikisrv, addr[i]);

	s = emalloc(sizeof *s);
	*s = wikisrv;
	postmountsrv(s, service, mtpt, MREPL|MCREATE);
	if(perm){
		buf = emalloc9p(5+strlen(service)+1);
		strcpy(buf, "/srv/");
		strcat(buf, service);
		nulldir(&d);
		d.mode = perm;
		if(dirwstat(buf, &d) < 0)
			fprint(2, "wstat: %r\n");
		free(buf);
	}
	exits(nil);
}
