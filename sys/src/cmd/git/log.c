#include <u.h>
#include <libc.h>
#include "git.h"

typedef struct Pfilt Pfilt;
struct Pfilt {
	char	*elt;
	int	show;
	Pfilt	*sub;
	int	nsub;
};

Biobuf	*out;
char	*queryexpr;
char	*commitid;
int	shortlog;

Object	**heap;
int	nheap;
int	heapsz;
Objset	done;
Pfilt	*pathfilt;

void
filteradd(Pfilt *pf, char *path)
{
	char *p, *e;
	int i;

	if((e = strchr(path, '/')) != nil)
		p = smprint("%.*s", (int)(e - path), path);
	else
		p = strdup(path);

	while(e != nil && *e == '/')
		e++;
	for(i = 0; i < pf->nsub; i++){
		if(strcmp(pf->sub[i].elt, p) == 0){
			pf->sub[i].show = pf->sub[i].show || (e == nil);
			if(e != nil)
				filteradd(&pf->sub[i], e);
			free(p);
			return;
		}
	}
	pf->sub = earealloc(pf->sub, pf->nsub+1, sizeof(Pfilt));
	pf->sub[pf->nsub].elt = p;
	pf->sub[pf->nsub].show = (e == nil);
	pf->sub[pf->nsub].nsub = 0;
	pf->sub[pf->nsub].sub = nil;
	if(e != nil)
		filteradd(&pf->sub[pf->nsub], e);
	pf->nsub++;
}

Hash
lookup(Pfilt *pf, Object *o)
{
	int i;

	for(i = 0; i < o->tree->nent; i++)
		if(strcmp(o->tree->ent[i].name, pf->elt) == 0)
			return o->tree->ent[i].h;
	return Zhash;
}

int
filtermatch1(Pfilt *pf, Object *t, Object *pt)
{
	Object *a, *b;
	Hash ha, hb;
	int i, r;

	if(pf->show)
		return 1;
	if(t->type != pt->type)
		return 1;
	if(t->type != GTree)
		return 0;

	for(i = 0; i < pf->nsub; i++){
		ha = lookup(&pf->sub[i], t);
		hb = lookup(&pf->sub[i], pt);
		if(hasheq(&ha, &hb))
			continue;
		if(hasheq(&ha, &Zhash) || hasheq(&hb, &Zhash))
			return 1;
		if((a = readobject(ha)) == nil)
			sysfatal("read %H: %r", ha);
		if((b = readobject(hb)) == nil)
			sysfatal("read %H: %r", hb);
		r = filtermatch1(&pf->sub[i], a, b);
		unref(a);
		unref(b);
		if(r)
			return 1;
	}
	return 0;
}

int
filtermatch(Object *o)
{
	Object *t, *p, *pt;
	int i, r;

	if(pathfilt == nil)
		return 1;
	if((t = readobject(o->commit->tree)) == nil)
		sysfatal("read %H: %r", o->commit->tree);
	for(i = 0; i < o->commit->nparent; i++){
		if((p = readobject(o->commit->parent[i])) == nil)
			sysfatal("read %H: %r", o->commit->parent[i]);
		if((pt = readobject(p->commit->tree)) == nil)
			sysfatal("read %H: %r", o->commit->tree);
		r = filtermatch1(pathfilt, t, pt);
		unref(p);
		unref(pt);
		if(r)
			return 1;
	}
	return o->commit->nparent == 0;
}


static char*
nextline(char *p, char *e)
{
	for(; p != e; p++)
		if(*p == '\n')
			break;
	return p;
}

static void
show(Object *o)
{
	Tm tm;
	char *p, *q, *e;

	assert(o->type == GCommit);
	if(!filtermatch(o))
		return;

	if(shortlog){
		p = o->commit->msg;
		e = p + o->commit->nmsg;
		q = nextline(p, e);
		Bprint(out, "%H ", o->hash);
		Bwrite(out, p, q - p);
		Bputc(out, '\n');
	}else{
		tmtime(&tm, o->commit->mtime, tzload("local"));
		Bprint(out, "Hash:\t%H\n", o->hash);
		Bprint(out, "Author:\t%s\n", o->commit->author);
		Bprint(out, "Date:\t%τ\n", tmfmt(&tm, "WW MMM D hh:mm:ss z YYYY"));
		Bprint(out, "\n");
		p = o->commit->msg;
		e = p + o->commit->nmsg;
		for(; p != e; p = q){
			q = nextline(p, e);
			Bputc(out, '\t');
			Bwrite(out, p, q - p);
			Bputc(out, '\n');
			if(q != e)
				q++;
		}
		Bprint(out, "\n");
	}
	Bflush(out);
}

static void
showquery(char *q)
{
	Object *o;
	Hash *h;
	int n, i;

	if((n = resolverefs(&h, q)) == -1)
		sysfatal("resolve: %r");
	for(i = 0; i < n; i++){
		if((o = readobject(h[i])) == nil)
			sysfatal("read %H: %r", h[i]);
		show(o);
		unref(o);
	}
	exits(nil);
}

static void
qput(Object *o)
{
	Object *p;
	int i;

	if(oshas(&done, o->hash))
		return;
	osadd(&done, o);
	if(nheap == heapsz){
		heapsz *= 2;
		heap = earealloc(heap, heapsz, sizeof(Object*));
	}
	heap[nheap++] = o;
	for(i = nheap - 1; i > 0; i = (i-1)/2){
		o = heap[i];
		p = heap[(i-1)/2];
		if(o->commit->mtime < p->commit->mtime)
			break;
		heap[i] = p;
		heap[(i-1)/2] = o;
	}
}

static Object*
qpop(void)
{
	Object *o, *t;
	int i, l, r, m;

	if(nheap == 0)
		return nil;

	i = 0;
	o = heap[0];
	t = heap[--nheap];
	heap[0] = t;
	while(1){
		m = i;
		l = 2*i+1;
		r = 2*i+2;
		if(l < nheap && heap[m]->commit->mtime < heap[l]->commit->mtime)
			m = l;
		if(r < nheap && heap[m]->commit->mtime < heap[r]->commit->mtime)
			m = r;
		else
			break;
		t = heap[m];
		heap[m] = heap[i];
		heap[i] = t;
		i = m;
	}
	return o;
}

static void
showcommits(char *c)
{
	Object *o, *p;
	int i;
	Hash h;

	if(c == nil)
		c = "HEAD";
	if(resolveref(&h, c) == -1)
		sysfatal("resolve %s: %r", c);
	if((o = readobject(h)) == nil)
		sysfatal("load %H: %r", h);
	heapsz = 8;
	heap = eamalloc(heapsz, sizeof(Object*));
	osinit(&done);
	qput(o);
	while((o = qpop()) != nil){
		show(o);
		for(i = 0; i < o->commit->nparent; i++){
			if((p = readobject(o->commit->parent[i])) == nil)
				sysfatal("load %H: %r", o->commit->parent[i]);
			qput(p);
		}
		unref(o);
	}
}

static void
usage(void)
{
	fprint(2, "usage: %s [-s] [-e expr | -c commit] files..\n", argv0);
	exits("usage");
}
	
void
main(int argc, char **argv)
{
	char path[1024], repo[1024], *p, *r;
	int i, nrepo;

	ARGBEGIN{
	case 'e':
		queryexpr = EARGF(usage());
		break;
	case 'c':
		commitid = EARGF(usage());
		break;
	case 's':
		shortlog++;
		break;
	default:
		usage();
		break;
	}ARGEND;

	if(findrepo(repo, sizeof(repo)) == -1)
		sysfatal("find root: %r");
	nrepo = strlen(repo);
	if(argc != 0){
		if(getwd(path, sizeof(path)) == nil)
			sysfatal("getwd: %r");
		if(strncmp(path, repo, nrepo) != 0)
			sysfatal("path shifted??");
		p = path + nrepo;
		pathfilt = emalloc(sizeof(Pfilt));
		for(i = 0; i < argc; i++){
			if(*argv[i] == '/'){
				if(strncmp(argv[i], repo, nrepo) != 0)
					continue;
				r = smprint("./%s", argv[i]+nrepo);
			}else
				r = smprint("./%s/%s", p, argv[i]);
			cleanname(r);
			filteradd(pathfilt, r);
			free(r);
		}
	}
	if(chdir(repo) == -1)
		sysfatal("chdir: %r");

	gitinit();
	tmfmtinstall();
	out = Bfdopen(1, OWRITE);
	if(queryexpr != nil)
		showquery(queryexpr);
	else
		showcommits(commitid);
	exits(nil);
}
