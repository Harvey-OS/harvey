/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

	free(f->dir.name);
	free(f->dir.uid);
	free(f->dir.gid);
	free(f->dir.muid);
	qlock(&filelk);
	assert(f->ref.ref == 0);
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
	if(decref(&f->ref) == 0){
		f->tree->destroy(f);
		freefile(f);
	}
}

static void
nop(File* f)
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

	wlock(&f->lock);
	wlock(&fp->lock);
	if(f->nchild != 0){
		werrstr("has children");
		wunlock(&fp->lock);
		wunlock(&f->lock);
		closefile(f);
		return -1;
	}

	if(f->parent != fp){
		werrstr("parent changed underfoot");
		wunlock(&fp->lock);
		wunlock(&f->lock);
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
	wunlock(&f->lock);

	cleanfilelist(fp);
	wunlock(&fp->lock);

	closefile(fp);	/* reference from child */
	closefile(f);	/* reference from tree */
	closefile(f);
	return 0;
}

File*
createfile(File *fp, char *name, char *uid, uint32_t perm, void *aux)
{
	File *f;
	Filelist **l, *fl;
	Tree *t;

	if((fp->dir.qid.type&QTDIR) == 0){
		werrstr("create in non-directory");
		return nil;
	}

	wlock(&fp->lock);
	/*
	 * We might encounter blank spots along the
	 * way due to deleted files that have not yet
	 * been flushed from the file list.  Don't reuse
	 * those - some apps (e.g., omero) depend on
	 * the file order reflecting creation order. 
	 * Always create at the end of the list.
	 */
	for(l=&fp->filelist; fl=*l; l=&fl->link){
		if(fl->f && strcmp(fl->f->dir.name, name) == 0){
			wunlock(&fp->lock);
			werrstr("file already exists");
			return nil;
		}
	}
	
	fl = emalloc9p(sizeof *fl);
	*l = fl;

	f = allocfile();
	f->dir.name = estrdup9p(name);
	f->dir.uid = estrdup9p(uid ? uid : fp->dir.uid);
	f->dir.gid = estrdup9p(fp->dir.gid);
	f->dir.muid = estrdup9p(uid ? uid : "unknown");
	f->aux = aux;
	f->dir.mode = perm;

	t = fp->tree;
	lock(&t->genlock);
	f->dir.qid.path = t->qidgen++;
	unlock(&t->genlock);
	if(perm & DMDIR)
		f->dir.qid.type |= QTDIR;
	if(perm & DMAPPEND)
		f->dir.qid.type |= QTAPPEND;
	if(perm & DMEXCL)
		f->dir.qid.type |= QTEXCL;

	f->dir.mode = perm;
	f->dir.atime = f->dir.mtime = time(0);
	f->dir.length = 0;
	f->parent = fp;
	incref(&fp->ref);
	f->tree = fp->tree;

	incref(&f->ref);	/* being returned */
	incref(&f->ref);	/* for the tree */
	fl->f = f;
	fp->nchild++;
	wunlock(&fp->lock);

	return f;
}

static File*
walkfile1(File *dir, char *elem)
{
	File *fp;
	Filelist *fl;

	rlock(&dir->lock);
	if(strcmp(elem, "..") == 0){
		fp = dir->parent;
		incref(&fp->ref);
		runlock(&dir->lock);
		closefile(dir);
		return fp;
	}

	fp = nil;
	for(fl=dir->filelist; fl; fl=fl->link)
		if(fl->f && strcmp(fl->f->dir.name, elem)==0){
			fp = fl->f;
			incref(&fp->ref);
			break;
		}

	runlock(&dir->lock);
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
alloctree(char *uid, char *gid, uint32_t mode, void (*destroy)(File*))
{
	char *muid;
	Tree *t;
	File *f;

	t = emalloc9p(sizeof *t);
	f = allocfile();
	f->dir.name = estrdup9p("/");
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

	f->dir.qid = (Qid){0, 0, QTDIR};
	f->dir.length = 0;
	f->dir.atime = f->dir.mtime = time(0);
	f->dir.mode = DMDIR | mode;
	f->tree = t;
	f->parent = f;
	f->dir.uid = uid;
	f->dir.gid = gid;
	f->dir.muid = muid;

	incref(&f->ref);
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

	rlock(&dir->lock);
	if((dir->dir.mode & DMDIR)==0){
		runlock(&dir->lock);
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
	runlock(&dir->lock);
	return r;
}

int32_t
readdirfile(Readdir *r, uint8_t *buf, int32_t n)
{
	int32_t x, m;
	Filelist *fl;

	for(fl=r->fl, m=0; fl && m+2<=n; fl=fl->link, m+=x){
		if(fl->f == nil)
			x = 0;
		else if((x=convD2M(&fl->f->dir, buf+m, n-m)) <= BIT16SZ)
			break;
	}
	r->fl = fl;
	return m;
}

void
closedirfile(Readdir *r)
{
	if(decref(&r->dir->readers) == 0){
		wlock(&r->dir->lock);
		cleanfilelist(r->dir);
		wunlock(&r->dir->lock);
	}
	free(r);
}
