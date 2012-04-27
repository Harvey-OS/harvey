/*
 * I/O for a Wiki document set.
 * 
 * The files are kept in one flat directory.
 * There are three files for each document:
 *	nnn	- current version of the document
 *	nnn.hist	- history (all old versions) of the document
 *		append-only
 *	L.nnn	- write lock file for the document
 * 
 * At the moment, since we don't have read/write locks
 * in the file system, we use the L.nnn file as a read lock too.
 * It's a hack but there aren't supposed to be many readers
 * anyway.
 *
 * The nnn.hist file is in the format read by Brdwhist.
 * The nnn file is in that format too, but only contains the
 * last entry of the nnn.hist file.
 *
 * In addition to this set of files, there is an append-only
 * map file that provides a mapping between numbers and titles.
 * The map file is a sequence of lines of the form
 *	nnn Title Here
 * The lock file L.map must be held to add to the end, to
 * make sure that the numbers are allocated sequentially.
 * 
 * We assume that writes to the map file will fit in one message,
 * so that we don't have to read-lock the file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

enum {
	Nhash = 64,
	Mcache = 128,
};

typedef struct Wcache Wcache;
struct Wcache {
	int n;
	ulong use;
	RWLock;
	ulong tcurrent;
	ulong thist;
	Whist *hist;
	Whist *current;
	Qid qid;
	Qid qidhist;
	Wcache *hash;
};

static RWLock cachelock;
static Wcache *tab[Nhash];
int ncache;

void
closewhist(Whist *wh)
{
	int i;

	if(wh && decref(wh) == 0){
		free(wh->title);
		for(i=0; i<wh->ndoc; i++){
			free(wh->doc[i].author);
			free(wh->doc[i].comment);
			freepage(wh->doc[i].wtxt);
		}
		free(wh->doc);
		free(wh);
	}
}

void
freepage(Wpage *p)
{
	Wpage *next;

	for(; p; p=next){
		next = p->next;
		free(p->text);
		free(p->url);
		free(p);
	}
}

static Wcache*
findcache(int n)
{
	Wcache *w;

	for(w=tab[n%Nhash]; w; w=w->hash)
		if(w->n == n){
			w->use = time(0);
			return w;
		}
	return nil;
}

static int
getlock(char *lock)
{
	char buf[ERRMAX];
	int i, fd;
	enum { SECS = 200 };

	for(i=0; i<SECS*10; i++){
		fd = wcreate(lock, ORDWR, DMEXCL|0666);
		if(fd >= 0)
			return fd;
		buf[0] = '\0';
		rerrstr(buf, sizeof buf);
		if(strstr(buf, "locked") == nil)
			break;
		sleep(1000/10);
	}
	werrstr("couldn't acquire lock %s: %r", lock);
	return -1;
}

static Whist*
readwhist(char *file, char *lock, Qid *qid)
{
	int lfd;
	Biobuf *b;
	Dir *d;
	Whist *wh;

	if((lfd=getlock(lock)) < 0)	// LOG?
		return nil;

	if(qid){
		if((d = wdirstat(file)) == nil){
			close(lfd);
			return nil;
		}
		*qid = d->qid;
		free(d);
	}

	if((b = wBopen(file, OREAD)) == nil){	//LOG?
		close(lfd);
		return nil;
	}

	wh = Brdwhist(b);

	Bterm(b);
	close(lfd);
	return wh;
}

static void
gencurrent(Wcache *w, Qid *q, char *file, char *lock, ulong *t, Whist **wp, int n)
{
	Dir *d;
	Whist *wh;

	if(*wp && *t+Tcache >= time(0))
		return;

	wlock(w);
	if(*wp && *t+Tcache >= time(0)){
		wunlock(w);
		return;
	}

	if(((d = wdirstat(file)) == nil) || (d->qid.path==q->path && d->qid.vers==q->vers)){
		*t = time(0);
		wunlock(w);
		free(d);
		return;
	}

	free(d);
	if(wh = readwhist(file, lock, q)){
		wh->n = n;
		*t = time(0);
		closewhist(*wp);
		*wp = wh;
	}
else fprint(2, "error file=%s lock=%s %r\n", file, lock);
	wunlock(w);
}

static void
current(Wcache *w)
{
	char tmp[40];
	char tmplock[40];

	sprint(tmplock, "d/L.%d", w->n);
	sprint(tmp, "d/%d", w->n);
	gencurrent(w, &w->qid, tmp, tmplock, &w->tcurrent, &w->current, w->n);
}

static void
currenthist(Wcache *w)
{
	char hist[40], lock[40];

	sprint(hist, "d/%d.hist", w->n);
	sprint(lock, "d/L.%d", w->n);

	gencurrent(w, &w->qidhist, hist, lock, &w->thist, &w->hist, w->n);
}

void
voidcache(int n)
{
	Wcache *c;

	rlock(&cachelock);
	if(c = findcache(n)){
		wlock(c);
		c->tcurrent = 0;
		c->thist = 0;
		/* aggressively free memory */
		closewhist(c->hist);
		c->hist = nil;
		closewhist(c->current);
		c->current = nil;
		wunlock(c);
	}
	runlock(&cachelock);
}

static Whist*
getcache(int n, int hist)
{
	int i, isw;
	ulong t;
	Wcache *c, **cp, **evict;
	Whist *wh;

	isw = 0;
	rlock(&cachelock);
	if(c = findcache(n)){
	Found:
		current(c);
		if(hist)
			currenthist(c);
		rlock(c);
		if(hist)
			wh = c->hist;
		else
			wh = c->current;
		if(wh)
			incref(wh);
		runlock(c);
		if(isw)
			wunlock(&cachelock);
		else
			runlock(&cachelock);
		return wh;
	}
	runlock(&cachelock);

	wlock(&cachelock);
	if(c = findcache(n)){
		isw = 1;	/* better to downgrade lock but can't */
		goto Found;
	}

	if(ncache < Mcache){
	Alloc:
		c = emalloc(sizeof *c);
		ncache++;
	}else{
		/* find something to evict. */
		t = ~0;
		evict = nil;
		for(i=0; i<Nhash; i++){
			for(cp=&tab[i], c=*cp; c; cp=&c->hash, c=*cp){
				if(c->use < t
				&& (!c->hist || c->hist->ref==1)
				&& (!c->current || c->current->ref==1)){
					evict = cp;
					t = c->use;
				}
			}
		}

		if(evict == nil){
			fprint(2, "wikifs: nothing to evict\n");
			goto Alloc;
		}

		c = *evict;
		*evict = c->hash;

		closewhist(c->current);
		closewhist(c->hist);
		memset(c, 0, sizeof *c);
	}

	c->n = n;
	c->hash = tab[n%Nhash];
	tab[n%Nhash] = c;
	isw = 1;
	goto Found;
}

Whist*
getcurrent(int n)
{
	return getcache(n, 0);
}

Whist*
gethistory(int n)
{
	return getcache(n, 1);
}

RWLock maplock;
Map *map;

static int
mapcmp(const void *va, const void *vb)
{
	Mapel *a, *b;

	a = (Mapel*)va;
	b = (Mapel*)vb;

	return strcmp(a->s, b->s);
}

void
closemap(Map *m)
{
	if(decref(m)==0){
		free(m->buf);
		free(m->el);
		free(m);
	}
}

void
currentmap(int force)
{
	char *p, *q, *r;
	int lfd, fd, m, n;
	Dir *d;
	Map *nmap;
	char *err = nil;

	lfd = -1;
	fd = -1;
	d = nil;
	nmap = nil;
	if(!force && map && map->t+Tcache >= time(0))
		return;

	wlock(&maplock);
	if(!force && map && map->t+Tcache >= time(0))
		goto Return;

	if((lfd = getlock("d/L.map")) < 0){
		err = "can't lock";
		goto Return;
	}

	if((d = wdirstat("d/map")) == nil)
		goto Return;

	if(map && d->qid.path == map->qid.path && d->qid.vers == map->qid.vers){
		map->t = time(0);
		goto Return;
	}

	if(d->length > Maxmap){
		//LOG
		err = "too long";
		goto Return;
	}

	if((fd = wopen("d/map", OREAD)) < 0)
		goto Return;

	nmap = emalloc(sizeof *nmap);
	nmap->buf = emalloc(d->length+1);
	n = readn(fd, nmap->buf, d->length);
	if(n != d->length){
		err = "bad length";
		goto Return;
	}
	nmap->buf[n] = '\0';

	n = 0;
	for(p=nmap->buf; p; p=strchr(p+1, '\n'))
		n++;
	nmap->el = emalloc(n*sizeof(nmap->el[0]));

	m = 0;
	for(p=nmap->buf; p && *p && m < n; p=q){
		if(q = strchr(p+1, '\n'))
			*q++ = '\0';
		nmap->el[m].n = strtol(p, &r, 10);
		if(*r == ' ')
			r++;
		else
			{}//LOG?
		nmap->el[m].s = strcondense(r, 1);
		m++;
	}
	//LOG if m != n

	nmap->qid = d->qid;
	nmap->t = time(0);
	nmap->nel = m;
	qsort(nmap->el, nmap->nel, sizeof(nmap->el[0]), mapcmp);
	if(map)
		closemap(map);
	map = nmap;
	incref(map);
	nmap = nil;
	
Return:
	free(d);
	if(nmap){
		free(nmap->el);
		free(nmap->buf);
		free(nmap);
	}
	if(map == nil)
		sysfatal("cannot get map: %s: %r", err);

	if(fd >= 0)
		close(fd);
	if(lfd >= 0)
		close(lfd);
	wunlock(&maplock);
}

int
allocnum(char *title, int mustbenew)
{
	char *p, *q;
	int lfd, fd, n;
	Biobuf b;

	if(strcmp(title, "map")==0 || strcmp(title, "new")==0){
		werrstr("reserved title name");
		return -1;
	}

	if(title[0]=='\0' || strpbrk(title, "/<>:?")){
		werrstr("invalid character in name");
		return -1;
	}
	if((n = nametonum(title)) >= 0){
		if(mustbenew){
			werrstr("duplicate title");
			return -1;
		}
		return n;
	}

	title = estrdup(title);
	strcondense(title, 1);
	strlower(title);
	if(strchr(title, '\n') || strlen(title) > 200){
		werrstr("bad title");
		free(title);
		return -1;
	}

	if((lfd = getlock("d/L.map")) < 0){
		free(title);
		return -1;
	}

	if((fd = wopen("d/map", ORDWR)) < 0){	// LOG?
		close(lfd);
		free(title);
		return -1;
	}

	/*
	 * What we really need to do here is make sure the 
	 * map is up-to-date, then make sure the title isn't
	 * taken, and then add it, all without dropping the locks.
	 *
	 * This turns out to be a mess when you start adding
	 * all the necessary dolock flags, so instead we just
	 * read through the file ourselves, and let our 
	 * map catch up on its own.
	 */
	Binit(&b, fd, OREAD);
	n = 0;
	while(p = Brdline(&b, '\n')){
		p[Blinelen(&b)-1] = '\0';
		n = atoi(p)+1;
		q = strchr(p, ' ');
		if(q == nil)
			continue;
		if(strcmp(q+1, title) == 0){
			free(title);
			close(fd);
			close(lfd);
			if(mustbenew){
				werrstr("duplicate title");
				return -1;
			}else
				return n;
		}
	}

	seek(fd, 0, 2);	/* just in case it's not append only */
	fprint(fd, "%d %s\n", n, title);
	close(fd);
	close(lfd);
	free(title);
	/* kick the map */
	currentmap(1);
	return n;
}

int
nametonum(char *s)
{
	char *p;
	int i, lo, hi, m, rv;

	s = estrdup(s);
	strlower(s);
	for(p=s; *p; p++)
		if(*p=='_')
			*p = ' ';

	currentmap(0);
	rlock(&maplock);
	lo = 0;
	hi = map->nel;
	while(hi-lo > 1){
		m = (lo+hi)/2;
		i = strcmp(s, map->el[m].s);
		if(i < 0)
			hi = m;
		else
			lo = m;
	}
	if(hi-lo == 1 && strcmp(s, map->el[lo].s)==0)
		rv = map->el[lo].n;
	else
		rv = -1;
	runlock(&maplock);
	free(s);
	return rv;
}

char*
numtoname(int n)
{
	int i;
	char *s;

	currentmap(0);
	rlock(&maplock);
	for(i=0; i<map->nel; i++){
		if(map->el[i].n==n)
			break;
	}
	if(i==map->nel){
		runlock(&maplock);
		return nil;
	}
	s = estrdup(map->el[i].s);
	runlock(&maplock);
	return s;
}

Whist*
getcurrentbyname(char *s)
{
	int n;

	if((n = nametonum(s)) < 0)
		return nil;
	return getcache(n, 0);
}

static String*
Brdstring(Biobuf *b)
{
	long len;
	String *s;
	Dir *d;

	d = dirfstat(Bfildes(b));
	if (d == nil)	/* shouldn't happen, we just opened it */
		len = 0;
	else
		len = d->length;
	free(d);
	s = s_newalloc(len);
	s_read(b, s, len);
	return s;
}

/*
 * Attempt to install a new page.  If t==0 we are creating.
 * Otherwise, we are editing and t must be set to the current
 * version (t is the version we started with) to avoid conflicting
 * writes.
 *
 * If there is a conflicting write, we still write the page to 
 * the history file, but mark it as a failed write.
 */
int
writepage(int num, ulong t, String *s, char *title)
{
	char tmp[40], tmplock[40], err[ERRMAX], hist[40], *p;
	int conflict, lfd, fd;
	Biobuf *b;
	String *os;

	sprint(tmp, "d/%d", num);
	sprint(tmplock, "d/L.%d", num);
	sprint(hist, "d/%d.hist", num);
	if((lfd = getlock(tmplock)) < 0)
		return -1;

	conflict = 0;
	if(b = wBopen(tmp, OREAD)){
		Brdline(b, '\n');	/* title */
		if(p = Brdline(b, '\n'))		/* version */
			p[Blinelen(b)-1] = '\0';
		if(p==nil || p[0] != 'D'){
			snprint(err, sizeof err, "bad format in extant file");
			conflict = 1;
		}else if(strtoul(p+1, 0, 0) != t){
			os = Brdstring(b);	/* why read the whole file? */
			p = strchr(s_to_c(s), '\n');
			if(p!=nil && strcmp(p+1, s_to_c(os))==0){	/* ignore dup write */
				close(lfd);
				s_free(os);
				Bterm(b);
				return 0;
			}
			s_free(os);
			snprint(err, sizeof err, "update conflict %lud != %s", t, p+1);
			conflict = 1;
		}
		Bterm(b);
	}else{
		if(t != 0){
			close(lfd);
			werrstr("did not expect to create");
			return -1;
		}
	}

	if((fd = wopen(hist, OWRITE)) < 0){
		if((fd = wcreate(hist, OWRITE, 0666)) < 0){
			close(lfd);
			return -1;
		}else
			fprint(fd, "%s\n", title);
	}
	if(seek(fd, 0, 2) < 0
	|| (conflict && write(fd, "X\n", 2) != 2)
	|| write(fd, s_to_c(s), s_len(s)) != s_len(s)){
		close(fd);
		close(lfd);
		return -1;
	}
	close(fd);

	if(conflict){
		close(lfd);
		voidcache(num);
		werrstr(err);
		return -1;
	}

	if((fd = wcreate(tmp, OWRITE, 0666)) < 0){
		close(lfd);
		voidcache(num);
		return -1;
	}
	if(write(fd, title, strlen(title)) != strlen(title)
	|| write(fd, "\n", 1) != 1
	|| write(fd, s_to_c(s), s_len(s)) != s_len(s)){
		close(fd);
		close(lfd);
		voidcache(num);
		return -1;
	}
	close(fd);
	close(lfd);
	voidcache(num);
	return 0;
}
