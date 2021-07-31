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
	int code;	/* last response code */
	char response[128];	/* last response */
	Group *currentgroup;
	char *addr;
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

int
nntpcmd(Netbuf *n, char *cmd, int e)
{
	int tries;

	for(tries=0;; tries++){
		Bprint(&n->bw, "%s\r\n", cmd);
		if(nntpresponse(n, e, cmd)>=0 && n->code/100 != 5)
			return 0;

		/* redial */
		if(tries==1)
			return -1;

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
	}
}

/*
 * Return the new Group structure for the group name.
 * Destroys name.
 */
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
	return nil;	/* shut up 8c */
}

/*
 * Return the output of a HEAD, BODY, or ARTICLE command.
 */
char*
nntpget(Netbuf *n, Group *g, int msg, char *retr)
{
	char cmd[1024];

	if(g->isgroup == 0){
		werrstr("not a group");
		return nil;
	}

	if(n->currentgroup != g){
		strcpy(cmd, "GROUP ");
		printgroup(cmd, g);
		if(nntpcmd(n, cmd, 21) < 0)
			return nil;
		n->currentgroup = g;
	}

	sprint(cmd, "%s %d", retr, msg);
	nntpcmd(n, cmd, 0);
	if(n->code/10 != 22)
		return nil;

	return Nreaddata(n);
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
		nf = tokenize(p, f, nelem(f));
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
		Bputc(&n->bw, '\n');
	}
	Bprint(&n->bw, ".\n");

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
 * The newsgroup is encoded in the top 14 bits (after CHDIR)
 * of the path.  The message number is the bottom 17 bits.
 * The file within the message directory is in the version [sic].
 */

enum {	/* file qids */
	Qhead,
	Qbody,
	Qarticle,
	Nfile,
};
char *filename[] = {
	"header",
	"body",
	"article",
};
char *nntpname[] = {
	"HEAD",
	"BODY",
	"ARTICLE",
};

#define GROUP(p)	(((p)>>17)&0x3FFF)
#define MESSAGE(p)	((p)&0x1FFFF)
#define FILE(v)		((v)&0x3)

#define PATH(d,g,m)	((d)|(((g)&0x3FFF)<<17)|((m)&0x1FFFF))
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
};

static void
fsattach(Req *r, Fid *fid, char *spec, Qid *qid)
{
	Aux *a;

	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}

	a = emalloc(sizeof *a);
	a->g = root;
	a->n = -1;
	fid->aux = a;
	
	*qid = (Qid){CHDIR, 0};
	respond(r, nil);
}

static void
fswalk(Req *r, Fid *fid, char *name, Qid *qid)
{
	char *p;
	int i, isdotdot, n;
	Aux *a;
	Group *ng;

	isdotdot = strcmp(name, "..")==0;
	a = fid->aux;

	if(a->s){	/* file */
		respond(r, "protocol botch");
		return;
	}

	if(a->n != -1){
		if(isdotdot){
			*qid = (Qid){PATH(CHDIR, a->g->num, 0), 0};
			a->n = -1;
			respond(r, nil);
			return;
		}
		for(i=0; i<Nfile; i++){ 
			if(strcmp(name, filename[i])==0){
				if(a->s = nntpget(net, a->g, a->n, nntpname[i])){
					*qid = (Qid){PATH(0, a->g->num, a->n), Qbody};
					a->file = i;
					respond(r, nil);
				}else
					respond(r, "file does not exist");
				return;
			}
		}
		respond(r, "file does not exist");
		return;
	}

	if(isdotdot){
		a->g = a->g->parent;
		*qid = (Qid){PATH(CHDIR, a->g->num, 0), 0};
		respond(r, nil);
		return;
	}

	if(a->g->isgroup && !readonly && a->g->canpost
	&& strcmp(name, "post")==0){
		a->ispost = 1;
		*qid = (Qid){PATH(0, a->g->num, 0), 0};
		respond(r, nil);
		return;
	}

	if(ng = findgroup(a->g, name, 0)){
		a->g = ng;
		*qid = (Qid){PATH(CHDIR, a->g->num, 0), 0};
		respond(r, nil);
		return;
	}

	n = strtoul(name, &p, 0);
	if('0'<=name[0] && name[0]<='9' && *p=='\0' && a->g->lo<=n && n<a->g->hi){
		a->n = n;
		*qid = (Qid){PATH(CHDIR, a->g->num, n+1-a->g->lo), 0};
		respond(r, nil);
		return;
	}

	respond(r, "file does not exist");
	return;
}

static void
fsopen(Req *r, Fid *fid, int omode, Qid*)
{
	Aux *a;

	a = fid->aux;
	if((a->ispost && (omode&~OTRUNC) != OWRITE)
	|| (!a->ispost && omode != OREAD))
		respond(r, "permission denied");
	else
		respond(r, nil);
}

static void
fsclone(Req *r, Fid *old, Fid *new)
{
	Aux *a;

	a = emalloc(sizeof(*a));
	*a = *(Aux*)old->aux;
	new->aux = a;
	respond(r, nil);
}

static void
fillstat(Dir *d, Aux *a)
{
	Group *g;

	memset(d, 0, sizeof *d);
	strcpy(d->uid, "nntp");
	strcpy(d->gid, "nntp");
	g = a->g;
	d->atime = d->mtime = g->mtime;

	if(a->ispost){
		strcpy(d->name, "post");
		d->mode = 0222;
		d->qid = (Qid){PATH(0, g->num, 0), 0};
		d->length = a->ns;
		return;
	}

	if(a->s){	/* article file */
		strcpy(d->name, filename[a->file]);
		d->mode = 0444;
		d->qid = (Qid){PATH(0, g->num, a->n+1-g->lo), a->file};
		return;
	}

	if(a->n != -1){	/* article directory */
		sprint(d->name, "%d", a->n);
		d->mode = CHDIR|0555;
		d->qid = (Qid){PATH(CHDIR, g->num, a->n+1-g->lo), 0};
		return;
	}

	/* group directory */
	if(g->name[0])
		strecpy(d->name, d->name+NAMELEN, g->name);
	else
		strcpy(d->name, "/");
	d->mode = CHDIR|0555;
	d->qid = (Qid){PATH(CHDIR, g->num, 0), g->hi-1};
}

static int
dirfillstat(Dir *d, Aux *a, int i)
{
	int ndir;
	Group *g;

	memset(d, 0, sizeof *d);
	strcpy(d->uid, "nntp");
	strcpy(d->gid, "nntp");
	g = a->g;
	d->atime = d->mtime = g->mtime;

	if(a->n != -1){	/* article directory */
		if(i >= Nfile)
			return -1;

		strcpy(d->name, filename[i]);
		d->mode = 0444;
		d->qid = (Qid){PATH(0, g->num, a->n), i};
		return 0;
	}

	/* hierarchy directory: child groups */
	if(i < g->nkid){
		strecpy(d->name, d->name+NAMELEN, g->kid[i]->name);
		d->mode = CHDIR|0555;
		d->qid = (Qid){PATH(CHDIR, g->kid[i]->num, 0), g->kid[i]->hi-1};
		return 0;
	}
	i -= g->nkid;

	/* group directory: post file */
	if(g->isgroup && !readonly && g->canpost){
		if(i < 1){
			strcpy(d->name, "post");
			d->mode = 0222;
			d->qid = (Qid){PATH(0, g->num, 0), 0};
			return 0;
		}
		i--;
	}

	/* group directory: child articles */
	ndir = g->hi - g->lo;
	if(i < ndir){
		sprint(d->name, "%d", g->lo+i);
		d->mode = CHDIR|0555;
		d->qid = (Qid){PATH(CHDIR, g->num, i+1), 0};
		return 0;
	}

	return -1;
}

static void
fsstat(Req *r, Fid *fid, Dir *d)
{
	Aux *a;

	a = fid->aux;
	if(fid->qid.path == CHDIR)
		nntprefreshall(net);
	else if(a->g->isgroup)
		nntprefresh(net, a->g);
	fillstat(d, a);
	respond(r, nil);
}

static void
fsread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	int i, j, n;
	Aux *a;
	Dir d;

	a = fid->aux;
	if(a->s){
		readstr(offset, buf, count, a->s);
		respond(r, nil);
		return;
	}

	n = *count/DIRLEN;
	i = offset/DIRLEN;
	for(j=0; j<n; j++){
		if(dirfillstat(&d, a, i+j) < 0)
			break;
		convD2M(&d, (char*)buf+j*DIRLEN);
	}
	*count = j*DIRLEN;
	respond(r, nil);
}

static void
fswrite(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	Aux *a;

	a = fid->aux;

	if(*count == 0){	/* commit */
		respond(r, nntppost(net, a->s));
		free(a->s);
		a->ns = 0;
		a->s = nil;
		return;
	}

	if(a->ns < *count+offset+1){
		a->s = erealloc(a->s, *count+offset+1);
		a->ns = *count+offset;
		a->s[a->ns] = '\0';
	}
	memmove(a->s+offset, buf, *count);
	respond(r, nil);

}

static void
fsclunkaux(Fid *fid)
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
.attach=	fsattach,
.clunkaux=	fsclunkaux,
.clone=	fsclone,
.walk=	fswalk,
.open=	fsopen,
.read=	fsread,
.write=	fswrite,
.stat=	fsstat,
};

void
usage(void)
{
	fprint(2, "usage: nntpsrv [-s service] [-m mtpt] [nntp.server]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char *mtpt, *service, *where;
	int x;
	Netbuf n;

	mtpt = "/mnt/news";
	service = nil;

	ARGBEGIN{
	case 'D':
		lib9p_chatty++;
		break;
	case 'N':
		netdebug = 1;
		break;
	case 's':
		service = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	}ARGEND

	if(argc > 1)
		usage();
	if(argc==0)
		where = "$nntp";
	else
		where = argv[0]; 

	now = time(0);

	memset(&n, 0, sizeof n);
	n.addr = netmkaddr(where, "tcp", "nntp");
	n.fd = dial(n.addr, nil, nil, nil);
	if(n.fd < 0)
		sysfatal("dial: %r\n");

	Binit(&n.br, n.fd, OREAD);
	Binit(&n.bw, n.fd, OWRITE);
	net = &n;

	if(nntpresponse(&n, 20, "greeting") < 0)
		sysfatal("failed greeting: %r");
	if(n.code/10 != 20)
		sysfatal("greeting: %r");

	if(n.code == 201)
		readonly = 1;

	x=netdebug;
	netdebug=0;
	root = emalloc(sizeof *root);
	root->name = estrdup("");
	root->parent = root;
	nntprefreshall(&n);
	netdebug=x;

	postmountsrv(&nntpsrv, service, mtpt, MREPL);
	exits(nil);
}

