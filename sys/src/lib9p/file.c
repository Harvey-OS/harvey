#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"
#include "impl.h"

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
	int i;
	File *f;
	enum { N = 16 };

	qlock(&filelk);
	if(freefilelist == nil){
		f = mallocz(N*sizeof(*f), 1);
		if(f == nil){
			qunlock(&filelk);
			return nil;
		}
		for(i=0; i<N-1; i++)
			f[i].aux = &f[i+1];
		f[N-1].aux = nil;
		freefilelist = f;
	}

	f = freefilelist;
	freefilelist = f->aux;
	qunlock(&filelk);

	memset(f, 0, sizeof *f);
	return f;
}

static void
freefile(File *f)
{
	qlock(&filelk);
	assert(f->ref.ref == 0);
	f->aux = freefilelist;
	freefilelist = f;
	qunlock(&filelk);
}

int
fclose(File *f)
{
	int n;
	char name[NAMELEN];

	strcpy(name, f->name);
	if((n = decref(&f->ref)) == 0){
		f->tree->rmaux(f);
		freefile(f);
	}
if(lib9p_chatty)
	fprint(2, "closefile \"%s\" %p: %ld refs\n", name, f, n ? f->ref.ref : n);

	return n;
}

static void
destroynop(File*)
{
}

Tree*
mktree(char *uid, char *gid, ulong mode)
{
	Tree *t;
	File *f;

	t = emalloc(sizeof *t);
	f = allocfile();
	if(f == nil)
		return nil;

	strcpy(f->name, "/");

	if(uid == nil)
		uid = getuser();
	if(uid == nil)
		uid = "none";
	if(gid == nil)
		gid = uid;

	strcpy(f->uid, uid);
	strcpy(f->gid, gid);
	f->qid = (Qid){CHDIR, 0};
	f->length = 0;
	f->atime = f->mtime = time(0);
	f->mode = CHDIR|mode;
	f->tree = t;
	f->parent = f;

	incref(&f->ref);
	t->root = f;
	t->qidgen = 0;
	t->dirqidgen = CHDIR|1;
	t->rmaux = destroynop;
	return t;
}

File*
fcreate(File *dir, char *name, char *uid, char *gid, ulong mode)
{
	Filelist *fl;
	Filelist *free;
	Tree *t;
	File *f;

	if((dir->qid.path & CHDIR) == 0){
		werrstr("create in non-directory");
		return nil;
	}

	free = nil;
	qlock(dir);
	for(fl = dir->filelist; fl; fl = fl->link){
		if(fl->f == nil)
			free = fl;
		else if(strcmp(fl->f->name, name) == 0){
			qunlock(dir);
			werrstr("file already exists");
			return nil;
		}
	}

	if(free == nil){
		free = emalloc(sizeof *free);
		free->link = dir->filelist;
		dir->filelist = free;
	}

	f = allocfile();
	if(f == nil)
		return nil;

	strcpy(f->name, name);
	strcpy(f->uid, uid ? uid : dir->uid);
	strcpy(f->gid, gid ? gid : dir->gid);

	t = dir->tree;
	if(mode & CHDIR)
		f->qid.path = t->dirqidgen++;
	else
		f->qid.path = t->qidgen++;

	f->mode = mode;
	f->atime = f->mtime = time(0);
	f->length = 0;

	f->parent = dir;
	incref(&dir->ref);		/* because another child has a ref */
	f->tree = dir->tree;

	incref(&f->ref);		/* because we'll return it */
	incref(&f->ref);		/* because it's going in the tree */
	free->f = f;
	qunlock(dir);

	return f;
}

void
fremove(File *f)
{
	Filelist *fl;

/* BUG: check for children */
//print("fremove %p\n", f);
	qlock(f);
	if(f->parent == nil){ /* already removed; shouldn't happen? */
		abort();
		qunlock(f);
		return;
	}

	qlock(f->parent);
	for(fl = f->parent->filelist; fl; fl = fl->link)
		if(fl->f == f)
			break;

	assert(fl != nil && fl->f == f);
	fl->f = nil;
	qunlock(f->parent);
	qunlock(f);
	if(fclose(f) == 0)	/* we removed it from the tree; still have own reference */
		abort();
	fclose(f);
}

void
fdump(File *f, int ntab)
{
	Filelist *fl;

	if(ntab) print("%.*s", ntab, "                                       ");
	print("%s %s %s\n", f->name, f->uid, f->gid);

	for(fl = f->filelist; fl; fl=fl->link)
		if(fl->f)
			fdump(fl->f, ntab+1);
}

File*
fwalk(File *f, char *name)
{
	Filelist *fl;

	if(strcmp(name, "..") == 0) {
		f = f->parent;
		incref(&f->ref);
		return f;
	}

	qlock(f);
	for(fl = f->filelist; fl; fl=fl->link)
		if(fl->f && strcmp(fl->f->name, name)==0){
			incref(&fl->f->ref);
			qunlock(f);
			return fl->f;
		}
	qunlock(f);
	return nil;
}

char*
fdirread(File *f, char *buf, long *count, vlong offset)
{
	Filelist *fl;
	long c, o;

	c = 0;
	o = offset/DIRLEN;
	*count /= DIRLEN;
	qlock(f);
	for(fl = f->filelist; fl && c < *count; fl=fl->link){
		if(fl->f){
			if(o)
				o--;
			else{
				convD2M(&fl->f->Dir, buf);
				buf += DIRLEN;
				c++;
			}
		}
	}
	qunlock(f);
	*count = c*DIRLEN;
	return nil;
}
