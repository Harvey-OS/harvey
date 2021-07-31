/*
 * Network news transport protocol (NNTP) file server.
 *
 * Unfortunately, the file system differs from that expected
 * by Charles Forsyth's rin news reader.  This is partially out
 * of my own laziness, but it makes the bookkeeping here
 * a lot easier.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Netbuf Netbuf;
typedef struct Group Group;

struct Netbuf {
	Biobuf br;
	Biobuf bw;
	int lineno;
	int fd;
	int code;			/* last response code */
	int auth;			/* Authorization required? */
	char response[128];	/* last response */
	Group *currentgroup;
	char *addr;
	char *user;
	char *pass;
	ulong extended;	/* supported extensions */
};

struct Group {
	char *name;
	Group *parent;
	Group **kid;
	int num;
	int nkid;
	int lo, hi;
	int canpost;
	int isgroup;	/* might just be piece of hierarchy */
	ulong mtime;
	ulong atime;
};

/*
 * First eight fields are, in order: 
 *	article number, subject, author, date, message-ID, 
 *	references, byte count, line count 
 * We don't support OVERVIEW.FMT; when I see a server with more
 * interesting fields, I'll implement support then.  In the meantime,
 * the standard defines the first eight fields.
 */

/* Extensions */
enum {
	Nxover   = (1<<0),
	Nxhdr    = (1<<1),
	Nxpat    = (1<<2),
	Nxlistgp = (1<<3),
};

Group *root;
Netbuf *net;
ulong now;
int netdebug;
int readonly;

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil)
		sysfatal("out of memory reallocating %lud", n);
	setmalloctag(v, getcallerpc(&v));
	return v;
}

void*
emalloc(ulong n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		sysfatal("out of memory allocating %lud", n);
	memset(v, 0, n);
	setmalloctag(v, getcallerpc(&n));
	return v;
}

char*
estrdup(char *s)
{
	int l;
	char *t;

	if (s == nil)
		return nil;
	l = strlen(s)+1;
	t = emalloc(l);
	memcpy(t, s, l);
	setmalloctag(t, getcallerpc(&s));
	return t;
}

char*
estrdupn(char *s, int n)
{
	int l;
	char *t;

	l = strlen(s);
	if(l > n)
		l = n;
	t = emalloc(l+1);
	memmove(t, s, l);
	t[l] = '\0';
	setmalloctag(t, getcallerpc(&s));
	return t;
}

char*
Nrdline(Netbuf *n)
{
	char *p;
	int l;

	n->lineno++;
	Bflush(&n->bw);
	if((p = Brdline(&n->br, '\n')) == nil){
		werrstr("nntp eof");
		return nil;
	}
	p[l=Blinelen(&n->br)-1] = '\0';
	if(l > 0 && p[l-1] == '\r')
		p[l-1] = '\0';
if(netdebug)
	fprint(2, "-> %s\n", p);
	return p;
}

int
nntpresponse(Netbuf *n, int e, char *cmd)
{
	int r;
	char *p;

	for(;;){
		p = Nrdline(n);
		if(p==nil){
			strcpy(n->response, "early nntp eof");
			return -1;
		}
		r = atoi(p);
		if(r/100 == 1){	/* BUG? */
			fprint(2, "%s\n", p);
			continue;
		}
		break;
	}

	strecpy(n->response, n->response+sizeof(n->response), p);

	if((r=atoi(p)) == 0){
		close(n->fd);
		n->fd = -1;
		fprint(2, "bad nntp response: %s\n", p);
		werrstr("bad nntp response");
		return -1;
	}

	n->code = r;
	if(0 < e && e<10 && r/100 != e){
		fprint(2, "%s: expected %dxx: got %s\n", cmd, e, n->response);
		return -1;
	}
	if(10 <= e && e<100 && r/10 != e){
		fprint(2, "%s: expected %dx: got %s\n", cmd, e, n->response);
		return -1;
	}
	if(100 <= e && r != e){
		fprint(2, "%s: expected %d: got %s\n", cmd, e, n->response);
		return -1;
	}
	return r;
}

int nntpauth(Netbuf*);
int nntpxcmdprobe(Netbuf*);
int nntpcurrentgroup(Netbuf*, Group*);

/* XXX: bug OVER/XOVER et al. */
static struct {
	ulong n;
	char *s;
} extensions [] = {
	{ Nxover, "OVER" },
	{ Nxhdr, "HDR" },
	{ Nxpat, "PAT" },
	{ Nxlistgp, "LISTGROUP" },
	{ 0, nil }
};

static int indial;

int
nntpconnect(Netbuf *n)
{
	n->currentgroup = nil;
	close(n->fd);
	if((n->fd = dial(n->addr, nil, nil, nil)) < 0){	
		snprint(n->response, sizeof n->response, "dial: %r");
		return -1;
	}
	Binit(&n->br, n->fd, OREAD);
	Binit(&n->bw, n->fd, OWRITE);
	if(nntpresponse(n, 20, "greeting") < 0)
		return -1;
	readonly = (n->code == 201);

	indial = 1;
	if(n->auth != 0)
		nntpauth(n);
//	nntpxcmdprobe(n);
	indial = 0;
	return 0;
}

int
nntpcmd(Netbuf *n, char *cmd, int e)
{
	int tried;

	tried = 0;
	for(;;){
		if(netdebug)
			fprint(2, "<- %s\n", cmd);
		Bprint(&n->bw, "%s\r\n", cmd);
		if(nntpresponse(n, e, cmd)>=0 && (e < 0 || n->code/100 != 5))
			return 0;

		/* redial */
		if(indial || tried++ || nntpconnect(n) < 0)
			return -1;
	}
}

int
nntpauth(Netbuf *n)
{
	char cmd[256];

	snprint(cmd, sizeof cmd, "AUTHINFO USER %s", n->user);
	if (nntpcmd(n, cmd, -1) < 0 || n->code != 381) {
		fprint(2, "Authentication failed: %s\n", n->response);
		return -1;
	}

	snprint(cmd, sizeof cmd, "AUTHINFO PASS %s", n->pass);
	if (nntpcmd(n, cmd, -1) < 0 || n->code != 281) {
		fprint(2, "Authentication failed: %s\n", n->response);
		return -1;
	}

	return 0;
}

int
nntpxcmdprobe(Netbuf *n)
{
	int i;
	char *p;

	n->extended = 0;
	if (nntpcmd(n, "LIST EXTENSIONS", 0) < 0 || n->code != 202)
		return 0;

	while((p = Nrdline(n)) != nil) {
		if (strcmp(p, ".") == 0)
			break;

		for(i=0; extensions[i].s != nil; i++)
			if (cistrcmp(extensions[i].s, p) == 0) {
				n->extended |= extensions[i].n;
				break;
			}
	}
	return 0;
}

/* XXX: searching, lazy evaluation */
static int
overcmp(void *v1, void *v2)
{
	int a, b;

	a = atoi(*(char**)v1);
	b = atoi(*(char**)v2);

	if(a < b)
		return -1;
	else if(a > b)
		return 1;
	return 0;
}

enum {
	XoverChunk = 100,
};

char *xover[XoverChunk];
int xoverlo;
int xoverhi;
int xovercount;
Group *xovergroup;

char*
nntpover(Netbuf *n, Group *g, int m)
{
	int i, lo, hi, mid, msg;
	char *p;
	char cmd[64];

	if (g->isgroup == 0)	/* BUG: should check extension capabilities */
		return nil;

	if(g != xovergroup || m < xoverlo || m >= xoverhi){
		lo = (m/XoverChunk)*XoverChunk;
		hi = lo+XoverChunk;
	
		if(lo < g->lo)
			lo = g->lo;
		else if (lo > g->hi)
			lo = g->hi;
		if(hi < lo || hi > g->hi)
			hi = g->hi;
	
		if(nntpcurrentgroup(n, g) < 0)
			return nil;
	
		if(lo == hi)
			snprint(cmd, sizeof cmd, "XOVER %d", hi);
		else
			snprint(cmd, sizeof cmd, "XOVER %d-%d", lo, hi-1);
		if(nntpcmd(n, cmd, 224) < 0)
			return nil;

		for(i=0; (p = Nrdline(n)) != nil; i++) {
			if(strcmp(p, ".") == 0)
				break;
			if(i >= XoverChunk)
				sysfatal("news server doesn't play by the rules");
			free(xover[i]);
			xover[i] = emalloc(strlen(p)+2);
			strcpy(xover[i], p);
			strcat(xover[i], "\n");
		}
		qsort(xover, i, sizeof(xover[0]), overcmp);

		xovercount = i;

		xovergroup = g;
		xoverlo = lo;
		xoverhi = hi;
	}

	lo = 0;
	hi = xovercount;
	/* search for message */
	while(lo < hi){
		mid = (lo+hi)/2;
		msg = atoi(xover[mid]);
		if(m == msg)
			return xover[mid];
		else if(m < msg)
			hi = mid;
		else
			lo = mid+1;
	}
	return nil;
}

/*
 * Return the new Group structure for the group name.
 * Destroys name.
 */
static int printgroup(char*,Group*);
Group*
findgroup(Group *g, char *name, int mk)
{
	int lo, hi, m;
	char *p, *q;
	static int ngroup;

	for(p=name; *p; p=q){
		if(q = strchr(p, '.'))
			*q++ = '\0';
		else
			q = p+strlen(p);

		lo = 0;
		hi = g->nkid;
		while(hi-lo > 1){
			m = (lo+hi)/2;
			if(strcmp(p, g->kid[m]->name) < 0)
				hi = m;
			else
				lo = m;
		}
		assert(lo==hi || lo==hi-1);
		if(lo==hi || strcmp(p, g->kid[lo]->name) != 0){
			if(mk==0)
				return nil;
			if(g->nkid%16 == 0)
				g->kid = erealloc(g->kid, (g->nkid+16)*sizeof(g->kid[0]));

			/* 
			 * if we're down to a single place 'twixt lo and hi, the insertion might need
			 * to go at lo or at hi.  strcmp to find out.  the list needs to stay sorted.
		 	 */
			if(lo==hi-1 && strcmp(p, g->kid[lo]->name) < 0)
				hi = lo;

			if(hi < g->nkid)
				memmove(g->kid+hi+1, g->kid+hi, sizeof(g->kid[0])*(g->nkid-hi));
			g->nkid++;
			g->kid[hi] = emalloc(sizeof(*g));
			g->kid[hi]->parent = g;
			g = g->kid[hi];
			g->name = estrdup(p);
			g->num = ++ngroup;
			g->mtime = time(0);
		}else
			g = g->kid[lo];
	}
	if(mk)
		g->isgroup = 1;
	return g;
}

static int
printgroup(char *s, Group *g)
{
	if(g->parent == g)
		return 0;

	if(printgroup(s, g->parent))
		strcat(s, ".");
	strcat(s, g->name);
	return 1;
}

static char*
Nreaddata(Netbuf *n)
{
	char *p, *q;
	int l;

	p = nil;
	l = 0;
	for(;;){
		q = Nrdline(n);
		if(q==nil){
			free(p);
			return nil;
		}
		if(strcmp(q, ".")==0)
			return p;
		if(q[0]=='.')
			q++;
		p = erealloc(p, l+strlen(q)+1+1);
		strcpy(p+l, q);
		strcat(p+l, "\n");
		l += strlen(p+l);
	}
}

/*
 * Return the output of a HEAD, BODY, or ARTICLE command.
 */
char*
nntpget(Netbuf *n, Group *g, int msg, char *retr)
{
	char *s;
	char cmd[1024];

	if(g->isgroup == 0){
		werrstr("not a group");
		return nil;
	}

	if(strcmp(retr, "XOVER") == 0){
		s = nntpover(n, g, msg);
		if(s == nil)
			s = "";
		return estrdup(s);
	}

	if(nntpcurrentgroup(n, g) < 0)
		return nil;
	sprint(cmd, "%s %d", retr, msg);
	nntpcmd(n, cmd, 0);
	if(n->code/10 != 22)
		return nil;

	return Nreaddata(n);
}

int
nntpcurrentgroup(Netbuf *n, Group *g)
{
	char cmd[1024];

	if(n->currentgroup != g){
		strcpy(cmd, "GROUP ");
		printgroup(cmd, g);
		if(nntpcmd(n, cmd, 21) < 0)
			return -1;
		n->currentgroup = g;
	}
	return 0;
}

void
nntprefreshall(Netbuf *n)
{
	char *f[10], *p;
	int hi, lo, nf;
	Group *g;

	if(nntpcmd(n, "LIST", 21) < 0)
		return;

	while(p = Nrdline(n)){
		if(strcmp(p, ".")==0)
			break;

		nf = getfields(p, f, nelem(f), 1, "\t\r\n ");
		if(nf != 4){
			int i;
			for(i=0; i<nf; i++)
				fprint(2, "%s%s", i?" ":"", f[i]);
			fprint(2, "\n");
			fprint(2, "syntax error in group list, line %d", n->lineno);
			return;
		}
		g = findgroup(root, f[0], 1);
		hi = strtol(f[1], 0, 10)+1;
		lo = strtol(f[2], 0, 10);
		if(g->hi != hi){
			g->hi = hi;
			if(g->lo==0)
				g->lo = lo;
			g->canpost = f[3][0] == 'y';
			g->mtime = time(0);
		}
	}
}

void
nntprefresh(Netbuf *n, Group *g)
{
	char cmd[1024];
	char *f[5];
	int lo, hi;

	if(g->isgroup==0)
		return;

	if(time(0) - g->atime < 30)
		return;

	strcpy(cmd, "GROUP ");
	printgroup(cmd, g);
	if(nntpcmd(n, cmd, 21) < 0){
		n->currentgroup = nil;
		return;
	}
	n->currentgroup = g;

	if(tokenize(n->response, f, nelem(f)) < 4){
		fprint(2, "error reading GROUP response");
		return;
	}

	/* backwards from LIST! */
	hi = strtol(f[3], 0, 10)+1;
	lo = strtol(f[2], 0, 10);
	if(g->hi != hi){
		g->mtime = time(0);
		if(g->lo==0)
			g->lo = lo;
		g->hi = hi;
	}
	g->atime = time(0);
}

char*
nntppost(Netbuf *n, char *msg)
{
	char *p, *q;

	if(nntpcmd(n, "POST", 34) < 0)
		return n->response;

	for(p=msg; *p; p=q){
		if(q = strchr(p, '\n'))
			*q++ = '\0';
		else
			q = p+strlen(p);

		if(p[0]=='.')
			Bputc(&n->bw, '.');
		Bwrite(&n->bw, p, strlen(p));
		Bputc(&n->bw, '\r');
		Bputc(&n->bw, '\n');
	}
	Bprint(&n->bw, ".\r\n");

	if(nntpresponse(n, 0, nil) < 0)
		return n->response;

	if(n->code/100 != 2)
		return n->response;
	return nil;
}

/*
 * Because an expanded QID space makes thngs much easier,
 * we sleazily use the version part of the QID as more path bits. 
 * Since we make sure not to mount ourselves cached, this
 * doesn't break anything (unless you want to bind on top of 
 * things in this file system).  In the next version of 9P, we'll
 * have more QID bits to play with.
 * 
 * The newsgroup is encoded in the top 15 bits
 * of the path.  The message number is the bottom 17 bits.
 * The file within the message directory is in the version [sic].
 */

enum {	/* file qids */
	Qhead,
	Qbody,
	Qarticle,
	Qxover,
	Nfile,
};
char *filename[] = {
	"header",
	"body",
	"article",
	"xover",
};
char *nntpname[] = {
	"HEAD",
	"BODY",
	"ARTICLE",
	"XOVER",
};

#define GROUP(p)	(((p)>>17)&0x3FFF)
#define MESSAGE(p)	((p)&0x1FFFF)
#define FILE(v)		((v)&0x3)

#define PATH(g,m)	((((g)&0x3FFF)<<17)|((m)&0x1FFFF))
#define POST(g)	PATH(0,g,0)
#define VERS(f)		((f)&0x3)

typedef struct Aux Aux;
struct Aux {
	Group *g;
	int n;
	int ispost;
	int file;
	char *s;
	int ns;
	int offset;
};

static void
fsattach(Req *r)
{
	Aux *a;
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}

	a = emalloc(sizeof *a);
	a->g = root;
	a->n = -1;
	r->fid->aux = a;
	
	r->ofcall.qid = (Qid){0, 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

static char*
fsclone(Fid *ofid, Fid *fid)
{
	Aux *a;

	a = emalloc(sizeof(*a));
	*a = *(Aux*)ofid->aux;
	fid->aux = a;
	return nil;
}

static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	char *p;
	int i, isdotdot, n;
	Aux *a;
	Group *ng;

	isdotdot = strcmp(name, "..")==0;

	a = fid->aux;
	if(a->s)	/* file */
		return "protocol botch";
	if(a->n != -1){
		if(isdotdot){
			*qid = (Qid){PATH(a->g->num, 0), 0, QTDIR};
			fid->qid = *qid;
			a->n = -1;
			return nil;
		}
		for(i=0; i<Nfile; i++){ 
			if(strcmp(name, filename[i])==0){
				if(a->s = nntpget(net, a->g, a->n, nntpname[i])){
					*qid = (Qid){PATH(a->g->num, a->n), Qbody, 0};
					fid->qid = *qid;
					a->file = i;
					return nil;
				}else
					return "file does not exist";
			}
		}
		return "file does not exist";
	}

	if(isdotdot){
		a->g = a->g->parent;
		*qid = (Qid){PATH(a->g->num, 0), 0, QTDIR};
		fid->qid = *qid;
		return nil;
	}

	if(a->g->isgroup && !readonly && a->g->canpost
	&& strcmp(name, "post")==0){
		a->ispost = 1;
		*qid = (Qid){PATH(a->g->num, 0), 0, 0};
		fid->qid = *qid;
		return nil;
	}

	if(ng = findgroup(a->g, name, 0)){
		a->g = ng;
		*qid = (Qid){PATH(a->g->num, 0), 0, QTDIR};
		fid->qid = *qid;
		return nil;
	}

	n = strtoul(name, &p, 0);
	if('0'<=name[0] && name[0]<='9' && *p=='\0' && a->g->lo<=n && n<a->g->hi){
		a->n = n;
		*qid = (Qid){PATH(a->g->num, n+1-a->g->lo), 0, QTDIR};
		fid->qid = *qid;
		return nil;
	}

	return "file does not exist";
}

static void
fsopen(Req *r)
{
	Aux *a;

	a = r->fid->aux;
	if((a->ispost && (r->ifcall.mode&~OTRUNC) != OWRITE)
	|| (!a->ispost && r->ifcall.mode != OREAD))
		respond(r, "permission denied");
	else
		respond(r, nil);
}

static void
fillstat(Dir *d, Aux *a)
{
	char buf[32];
	Group *g;

	memset(d, 0, sizeof *d);
	d->uid = estrdup("nntp");
	d->gid = estrdup("nntp");
	g = a->g;
	d->atime = d->mtime = g->mtime;

	if(a->ispost){
		d->name = estrdup("post");
		d->mode = 0222;
		d->qid = (Qid){PATH(g->num, 0), 0, 0};
		d->length = a->ns;
		return;
	}

	if(a->s){	/* article file */
		d->name = estrdup(filename[a->file]);
		d->mode = 0444;
		d->qid = (Qid){PATH(g->num, a->n+1-g->lo), a->file, 0};
		return;
	}

	if(a->n != -1){	/* article directory */
		sprint(buf, "%d", a->n);
		d->name = estrdup(buf);
		d->mode = DMDIR|0555;
		d->qid = (Qid){PATH(g->num, a->n+1-g->lo), 0, QTDIR};
		return;
	}

	/* group directory */
	if(g->name[0])
		d->name = estrdup(g->name);
	else
		d->name = estrdup("/");
	d->mode = DMDIR|0555;
	d->qid = (Qid){PATH(g->num, 0), g->hi-1, QTDIR};
}

static int
dirfillstat(Dir *d, Aux *a, int i)
{
	int ndir;
	Group *g;
	char buf[32];

	memset(d, 0, sizeof *d);
	d->uid = estrdup("nntp");
	d->gid = estrdup("nntp");

	g = a->g;
	d->atime = d->mtime = g->mtime;

	if(a->n != -1){	/* article directory */
		if(i >= Nfile)
			return -1;

		d->name = estrdup(filename[i]);
		d->mode = 0444;
		d->qid = (Qid){PATH(g->num, a->n), i, 0};
		return 0;
	}

	/* hierarchy directory: child groups */
	if(i < g->nkid){
		d->name = estrdup(g->kid[i]->name);
		d->mode = DMDIR|0555;
		d->qid = (Qid){PATH(g->kid[i]->num, 0), g->kid[i]->hi-1, QTDIR};
		return 0;
	}
	i -= g->nkid;

	/* group directory: post file */
	if(g->isgroup && !readonly && g->canpost){
		if(i < 1){
			d->name = estrdup("post");
			d->mode = 0222;
			d->qid = (Qid){PATH(g->num, 0), 0, 0};
			return 0;
		}
		i--;
	}

	/* group directory: child articles */
	ndir = g->hi - g->lo;
	if(i < ndir){
		sprint(buf, "%d", g->lo+i);
		d->name = estrdup(buf);
		d->mode = DMDIR|0555;
		d->qid = (Qid){PATH(g->num, i+1), 0, QTDIR};
		return 0;
	}

	return -1;
}

static void
fsstat(Req *r)
{
	Aux *a;

	a = r->fid->aux;
	if(r->fid->qid.path == 0 && (r->fid->qid.type & QTDIR))
		nntprefreshall(net);
	else if(a->g->isgroup)
		nntprefresh(net, a->g);
	fillstat(&r->d, a);
	respond(r, nil);
}

static void
fsread(Req *r)
{
	int offset, n;
	Aux *a;
	char *p, *ep;
	Dir d;

	a = r->fid->aux;
	if(a->s){
		readstr(r, a->s);
		respond(r, nil);
		return;
	}

	if(r->ifcall.offset == 0)
		offset = 0;
	else
		offset = a->offset;

	p = r->ofcall.data;
	ep = r->ofcall.data+r->ifcall.count;
	for(; p+2 < ep; p += n){
		if(dirfillstat(&d, a, offset) < 0)
			break;
		n=convD2M(&d, (uchar*)p, ep-p);
		free(d.name);
		free(d.uid);
		free(d.gid);
		free(d.muid);
		if(n <= BIT16SZ)
			break;
		offset++;
	}
	a->offset = offset;
	r->ofcall.count = p - r->ofcall.data;
	respond(r, nil);
}

static void
fswrite(Req *r)
{
	Aux *a;
	long count;
	vlong offset;

	a = r->fid->aux;

	if(r->ifcall.count == 0){	/* commit */
		respond(r, nntppost(net, a->s));
		free(a->s);
		a->ns = 0;
		a->s = nil;
		return;
	}

	count = r->ifcall.count;
	offset = r->ifcall.offset;
	if(a->ns < count+offset+1){
		a->s = erealloc(a->s, count+offset+1);
		a->ns = count+offset;
		a->s[a->ns] = '\0';
	}
	memmove(a->s+offset, r->ifcall.data, count);
	r->ofcall.count = count;
	respond(r, nil);
}

static void
fsdestroyfid(Fid *fid)
{
	Aux *a;

	a = fid->aux;
	if(a==nil)
		return;

	if(a->ispost && a->s)
		nntppost(net, a->s);

	free(a->s);
	free(a);
}

Srv nntpsrv = {
.destroyfid=	fsdestroyfid,
.attach=	fsattach,
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
	fprint(2, "usage: nntpsrv [-a] [-s service] [-m mtpt] [nntp.server]\n");
	exits("usage");
}

void
dumpgroups(Group *g, int ind)
{
	int i;

	print("%*s%s\n", ind*4, "", g->name);
	for(i=0; i<g->nkid; i++)
		dumpgroups(g->kid[i], ind+1);
}

void
main(int argc, char **argv)
{
	int auth, x;
	char *mtpt, *service, *where, *user;
	Netbuf n;
	UserPasswd *up;

	mtpt = "/mnt/news";
	service = nil;
	memset(&n, 0, sizeof n);
	user = nil;
	auth = 0;
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 'N':
		netdebug = 1;
		break;
	case 'a':
		auth = 1;
		break;
	case 'u':
		user = EARGF(usage());
		break;
	case 's':
		service = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc==0)
		where = "$nntp";
	else
		where = argv[0]; 

	now = time(0);

	net = &n;
	if(auth) {
		n.auth = 1;
		if(user)
			up = auth_getuserpasswd(auth_getkey, "proto=pass service=nntp server=%q user=%q", where, user);
		else
			up = auth_getuserpasswd(auth_getkey, "proto=pass service=nntp server=%q", where);
		if(up == nil)
			sysfatal("no password: %r");

		n.user = up->user;
		n.pass = up->passwd;
	}

	n.addr = netmkaddr(where, "tcp", "nntp");

	root = emalloc(sizeof *root);
	root->name = estrdup("");
	root->parent = root;

	n.fd = -1;
	if(nntpconnect(&n) < 0)
		sysfatal("nntpconnect: %s", n.response);

	x=netdebug;
	netdebug=0;
	nntprefreshall(&n);
	netdebug=x;
//	dumpgroups(root, 0);

	postmountsrv(&nntpsrv, service, mtpt, MREPL);
	exits(nil);
}

