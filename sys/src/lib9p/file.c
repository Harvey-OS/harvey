#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

/*
 * To avoid deadlock, the following rules must be followed.
 * Always lock child then parent, never parent then child.
 * If holding the free file lock, do not lock any Files.
 */
struct Filelist {
	File *f;
	Filelist *link;
};

static QLock filelk;
static File *freefilelist;

static File*
allocfile(void)
{
	int i, a;
	File *f;
	enum { N = 16 };

	qlock(&filelk);
	if(freefilelist == nil){
		f = emalloc9p(N*sizeof(*f));
		for(i=0; i<N-1; i++)
			f[i].aux = &f[i+1];
		f[N-1].aux = nil;
		f[0].allocd = 1;
		freefilelist = f;
	}

	f = freefilelist;
	freefilelist = f->aux;
	qunlock(&filelk);

	a = f->allocd;
	memset(f, 0, sizeof *f);
	f->allocd = a;
	return f;
}

static void
freefile(File *f)
{
	Filelist *fl, *flnext;

	for(fl=f->filelist; fl; fl=flnext){
		flnext = fl->link;
		assert(fl->f == nil);
		free(fl);
	}

	free(f->name);
	free(f->uid);
	free(f->gid);
	free(f->muid);
	qlock(&filelk);
	assert(f->ref == 0);
	f->aux = freefilelist;
	freefilelist = f;
	qunlock(&filelk);
}

void
closefile(File *f)
{
	if(decref(f) == 0){
		f->tree->destroy(f);
		freefile(f);
	}
}

static void
nop(File*)
{
}

int
removefile(File *f)
{
	File *fp;
	Filelist *fl;
	
	fp = f->parent;
	if(fp == nil){
		werrstr("no parent");
		closefile(f);
		return -1;
	}

	if(fp == f){
		werrstr("cannot remove root");
		closefile(f);
		return -1;
	}

	wlock(fp);
	wlock(f);
	if(f->nchild != 0){
		werrstr("has children");
		wunlock(f);
		wunlock(fp);
		closefile(f);
		return -1;
	}

	if(f->parent != fp){
		werrstr("parent changed underfoot");
		wunlock(f);
		wunlock(fp);
		closefile(f);
		return -1;
	}

	for(fl=fp->filelist; fl; fl=fl->link)
		if(fl->f == f)
			break;
	assert(fl != nil && fl->f == f);

	fl->f = nil;
	fp->nchild--;
	f->parent = nil;
	wunlock(fp);
	wunlock(f);

	closefile(fp);	/* reference from child */
	closefile(f);	/* reference from tree */
	closefile(f);
	return 0;
}

File*
createfile(File *fp, char *name, char *uid, ulong perm, void *aux)
{
	File *f;
	Filelist *fl, *freel;
	Tree *t;

	if((fp->qid.type&QTDIR) == 0){
		werrstr("create in non-directory");
		return nil;
	}

	freel = nil;
	wlock(fp);
	for(fl=fp->filelist; fl; fl=fl->link){
		if(fl->f == nil)
			freel = fl;
		else if(strcmp(fl->f->name, name) == 0){
			wunlock(fp);
			werrstr("file already exists");
			return nil;
		}
	}

	if(freel == nil){
		freel = emalloc9p(sizeof *freel);
		freel->link = fp->filelist;
		fp->filelist = freel;
	}

	f = allocfile();
	f->name = estrdup9p(name);
	f->uid = estrdup9p(uid ? uid : fp->uid);
	f->gid = estrdup9p(fp->gid);
	f->muid = estrdup9p(uid ? uid : "unknown");
	f->aux = aux;
	f->mode = perm;

	t = fp->tree;
	lock(&t->genlock);
	f->qid.path = t->qidgen++;
	unlock(&t->genlock);
	if(perm & DMDIR)
		f->qid.type |= QTDIR;
	if(perm & DMAPPEND)
		f->qid.type |= QTAPPEND;
	if(perm & DMEXCL)
		f->qid.type |= QTEXCL;

	f->mode = perm;
	f->atime = f->mtime = time(0);
	f->length = 0;
	f->parent = fp;
	incref(fp);
	f->tree = fp->tree;

	incref(f);	/* being returned */
	incref(f);	/* for the tree */
	freel->f = f;
	fp->nchild++;
	wunlock(fp);

	return f;
}

static File*
walkfile1(File *dir, char *elem)
{
	File *fp;
	Filelist *fl;

	rlock(dir);
	if(strcmp(elem, "..") == 0){
		fp = dir->parent;
		incref(fp);
		runlock(dir);
		closefile(dir);
		return fp;
	}

	fp = nil;
	for(fl=dir->filelist; fl; fl=fl->link)
		if(fl->f && strcmp(fl->f->name, elem)==0){
			fp = fl->f;
			incref(fp);
			break;
		}

	runlock(dir);
	closefile(dir);
	return fp;
}

File*
walkfile(File *f, char *path)
{
	char *os, *s, *nexts;
	File *nf;

	if(strchr(path, '/') == nil)
		return walkfile1(f, path);	/* avoid malloc */

	os = s = estrdup9p(path);
	incref(f);
	for(; *s; s=nexts){
		if(nexts = strchr(s, '/'))
			*nexts++ = '\0';
		else
			nexts = s+strlen(s);
		nf = walkfile1(f, s);
		decref(f);
		f = nf;
		if(f == nil)
			break;
	}
	free(os);
	return f;
}
			
Tree*
alloctree(char *uid, char *gid, ulong mode, void (*destroy)(File*))
{
	char *muid;
	Tree *t;
	File *f;

	t = emalloc9p(sizeof *t);
	f = allocfile();
	f->name = estrdup9p("/");
	if(uid == nil){
		if(uid = getuser())
			uid = estrdup9p(uid);
	}
	if(uid == nil)
		uid = estrdup9p("none");
	else
		uid = estrdup9p(uid);

	if(gid == nil)
		gid = estrdup9p(uid);
	else
		gid = estrdup9p(gid);

	muid = estrdup9p(uid);

	f->qid = (Qid){0, 0, QTDIR};
	f->length = 0;
	f->atime = f->mtime = time(0);
	f->mode = DMDIR | mode;
	f->tree = t;
	f->parent = f;
	f->uid = uid;
	f->gid = gid;
	f->muid = muid;

	incref(f);
	t->root = f;
	t->qidgen = 0;
	t->dirqidgen = 1;
	if(destroy == nil)
		destroy = nop;
	t->destroy = destroy;

	return t;
}

static void
_freefiles(File *f)
{
	Filelist *fl, *flnext;

	for(fl=f->filelist; fl; fl=flnext){
		flnext = fl->link;
		_freefiles(fl->f);
		free(fl);
	}

	f->tree->destroy(f);
	freefile(f);
}

void
freetree(Tree *t)
{
	_freefiles(t->root);
	free(t);
}

struct Readdir {
	Filelist *fl;
};

Readdir*
opendirfile(File *dir)
{
	Readdir *r;

	rlock(dir);
	if((dir->mode & DMDIR)==0){
		runlock(dir);
		return nil;
	}
	r = emalloc9p(sizeof(*r));

	/*
	 * This reference won't go away while we're using it
	 * since we are dir->rdir.
	 */
	r->fl = dir->filelist;
	runlock(dir);
	return r;
}

long
readdirfile(Readdir *r, uchar *buf, long n)
{
	long x, m;
	Filelist *fl;

	for(fl=r->fl, m=0; fl && m+2<=n; fl=fl->link, m+=x){
		if(fl->f == nil)
			x = 0;
		else if((x=convD2M(fl->f, buf+m, n-m)) <= BIT16SZ)
			break;
	}
	r->fl = fl;
	return m;
}

void
closedirfile(Readdir *r)
{
	free(r);
}
