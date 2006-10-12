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
struct Filelist
{
	File *f;
	Filelist *link;
};

struct Readdir
{
	File *dir;
	Filelist *fl;
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

static void
cleanfilelist(File *f)
{
	Filelist **l;
	Filelist *fl;
	
	/*
	 * can't delete filelist structures while there
	 * are open readers of this directory, because
	 * they might have references to the structures.
	 * instead, just leave the empty refs in the list
	 * until there is no activity and then clean up.
	 */
	if(f->readers.ref != 0)
		return;
	if(f->nxchild == 0)
		return;

	/*
	 * no dir readers, file is locked, and
	 * there are empty entries in the file list.
	 * clean them out.
	 */
	for(l=&f->filelist; fl=*l; ){
		if(fl->f == nil){
			*l = (*l)->link;
			free(fl);
		}else
			l = &(*l)->link;
	}
	f->nxchild = 0;
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

	wlock(f);
	wlock(fp);
	if(f->nchild != 0){
		werrstr("has children");
		wunlock(fp);
		wunlock(f);
		closefile(f);
		return -1;
	}

	if(f->parent != fp){
		werrstr("parent changed underfoot");
		wunlock(fp);
		wunlock(f);
		closefile(f);
		return -1;
	}

	for(fl=fp->filelist; fl; fl=fl->link)
		if(fl->f == f)
			break;
	assert(fl != nil && fl->f == f);

	fl->f = nil;
	fp->nchild--;
	fp->nxchild++;
	f->parent = nil;
	wunlock(f);

	cleanfilelist(fp);
	wunlock(fp);

	closefile(fp);	/* reference from child */
	closefile(f);	/* reference from tree */
	closefile(f);
	return 0;
}

File*
createfile(File *fp, char *name, char *uid, ulong perm, void *aux)
{
	File *f;
	Filelist **l, *fl;
	Tree *t;

	if((fp->qid.type&QTDIR) == 0){
		werrstr("create in non-directory");
		return nil;
	}

	wlock(fp);
	/*
	 * We might encounter blank spots along the
	 * way due to deleted files that have not yet
	 * been flushed from the file list.  Don't reuse
	 * those - some apps (e.g., omero) depend on
	 * the file order reflecting creation order. 
	 * Always create at the end of the list.
	 */
	for(l=&fp->filelist; fl=*l; l=&fl->link){
		if(fl->f && strcmp(fl->f->name, name) == 0){
			wunlock(fp);
			werrstr("file already exists");
			return nil;
		}
	}
	
	fl = emalloc9p(sizeof *fl);
	*l = fl;

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
	fl->f = f;
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

	if(strchr(path, '/') == nil)
		return walkfile1(f, path);	/* avoid malloc */

	os = s = estrdup9p(path);
	for(; *s; s=nexts){
		if(nexts = strchr(s, '/'))
			*nexts++ = '\0';
		else
			nexts = s+strlen(s);
		f = walkfile1(f, s);
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
		uid = getuser();
		if(uid == nil)
			uid = "none";
	}
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
	 * This reference won't go away while we're 
	 * using it because file list entries are not freed
	 * until the directory is removed and all refs to
	 * it (our fid is one!) have gone away.
	 */
	r->fl = dir->filelist;
	r->dir = dir;
	incref(&dir->readers);
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
	if(decref(&r->dir->readers) == 0){
		wlock(r->dir);
		cleanfilelist(r->dir);
		wunlock(r->dir);
	}
	free(r);
}
