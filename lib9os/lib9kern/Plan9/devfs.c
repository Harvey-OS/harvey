/*
 * Plan 9 file system interface
 */
#include	"dat.h"
#include	"fns.h"
#include	"error.h"

typedef struct Fsinfo Fsinfo;
struct Fsinfo
{
	int	fd;
	QLock;		/* serialise access to offset */
	ulong	offset;	/* offset used only for directory reads */
	Cname*	name;	/* Plan 9's name for file */
	Qid	rootqid;		/* Plan 9's qid for Inferno's root */
	char*	root;		/* prefix to strip from all names in diagnostics */
};
#define	FS(c)	((Fsinfo*)((c)->aux))

char	rootdir[MAXROOT] = ROOT;

static void
fserr(Fsinfo *f)
{
	int n;
	char *p;

	oserrstr(up->env->errstr, ERRMAX);
	if(f != nil && *up->env->errstr == '\'' && (n = strlen(f->root)) > 1){
		/* don't reveal full names */
		if(strncmp(up->env->errstr+1, f->root, n-1) == 0){
			p = up->env->errstr+1+n;
			memmove(up->env->errstr+1, p, strlen(p)+1);
		}
	}
	p9error(up->env->errstr);
}

static void
fsfree(Chan *c)
{
	cnameclose(FS(c)->name);
	free(c->aux);
}

Chan*
fsattach(char *spec)
{
	Chan *c;
	Dir *d;
	char *root;
	Qid rootqid;
	static int devno;
	static Lock l;

	if(!emptystr(spec)){
		if(strcmp(spec, "*") != 0)
			p9error(Ebadspec);
		root = "/";
	}else
		root = rootdir;

	d = dirstat(root);
	if(d == nil)
		fserr(nil);
	rootqid = d->qid;
	free(d);

	c = devattach('U', spec);
	lock(&l);
	c->dev = devno++;
	c->qid = rootqid;
	unlock(&l);
	c->aux = smalloc(sizeof(Fsinfo));
	FS(c)->name = newcname(root);
	FS(c)->rootqid = rootqid;
	FS(c)->fd = -1;
	FS(c)->root = root;

	return c;
}

Walkqid*
fswalk(Chan *c, Chan *nc, char **name, int nname)
{
	int j, alloc;
	Walkqid *wq;
	Dir *dir;
	char *n;
	Cname *current, *next;
	Qid rootqid;

	if(nname > 0)
		isdir(c);	/* do we need this? */

	alloc = 0;
	current = nil;
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	if(waserror()){
		if(alloc && wq->clone!=nil)
			cclose(wq->clone);
		cnameclose(current);
		free(wq);
		return nil;
	}
	if(nc == nil){
		nc = devclone(c);
		nc->type = 0;
		alloc = 1;
	}
	wq->clone = nc;

	rootqid = FS(c)->rootqid;
	current = FS(c)->name;
	if(current != nil)
		incref(&current->r);
	for(j=0; j<nname; j++){
		if(!(nc->qid.type&QTDIR)){
			if(j==0)
				p9error(Enotdir);
			break;
		}
		n = name[j];
		if(strcmp(n, ".") != 0 && !(isdotdot(n) && nc->qid.path == rootqid.path)){	/* TO DO: underlying qids aliased */
			//print("** ufs walk '%s' -> %s\n", current->s, n);
			next = current;
			incref(&next->r);
			next = addelem(current, n);
			dir = dirstat(next->s);
			if(dir == nil){
				cnameclose(next);
				if(j == 0)
					p9error(Enonexist);
				strcpy(up->env->errstr, Enonexist);
				break;
			}
			nc->qid = dir->qid;
			free(dir);
			cnameclose(current);
			current = next;
		}
		wq->qid[wq->nqid++] = nc->qid;
	}
//	print("** ufs walk '%s'\n", current->s);

	poperror();
	if(wq->nqid < nname){
		cnameclose(current);
		if(alloc)
			cclose(wq->clone);
		wq->clone = nil;
	}else if(wq->clone){
		/* now attach to our device */
		nc->aux = smalloc(sizeof(Fsinfo));
		nc->type = c->type;
		FS(nc)->rootqid = FS(c)->rootqid;
		FS(nc)->name = current;
		FS(nc)->fd = -1;
		FS(nc)->root = FS(c)->root;
	}else
		panic("fswalk: can't happen");
	return wq;
}

int
fsstat(Chan *c, uchar *dp, int n)
{
	if(FS(c)->fd >= 0)
		n = fstat(FS(c)->fd, dp, n);
	else
		n = stat(FS(c)->name->s, dp, n);
	if(n < 0)
		fserr(FS(c));
	/* TO DO: change name to / if rootqid */
	return n;
}

Chan*
fsopen(Chan *c, int mode)
{
	osenter();
	FS(c)->fd = open(FS(c)->name->s, mode);
	osleave();
	if(FS(c)->fd < 0)
		fserr(FS(c));
	c->mode = openmode(mode);
	c->offset = 0;
	FS(c)->offset = 0;
	c->flag |= COPEN;
	return c;
}

void
fscreate(Chan *c, char *name, int mode, ulong perm)
{
	Dir *d;
	Cname *n;

	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		p9error(Efilename);
	n = addelem(newcname(FS(c)->name->s), name);
	osenter();
	FS(c)->fd = create(n->s, mode, perm);
	osleave();
	if(FS(c)->fd < 0) {
		cnameclose(n);
		fserr(FS(c));
	}
	d = dirfstat(FS(c)->fd);
	if(d == nil) {
		cnameclose(n);
		close(FS(c)->fd);
		FS(c)->fd = -1;
		fserr(FS(c));
	}
	c->qid = d->qid;
	free(d);

	cnameclose(FS(c)->name);
	FS(c)->name = n;

	c->mode = openmode(mode);
	c->offset = 0;
	FS(c)->offset = 0;
	c->flag |= COPEN;
}

void
fsclose(Chan *c)
{
	if(c->flag & COPEN){
		osenter();
		close(FS(c)->fd);
		osleave();
	}
	/* don't need to check for CRCLOSE, because Plan 9 itself implements ORCLOSE */
	fsfree(c);
}

static long
fsdirread(Chan *c, void *va, long count, vlong offset)
{
	long n, r;
	static char slop[16384];

	if(FS(c)->offset != offset){
		seek(FS(c)->fd, 0, 0);
		for(n=0; n<offset;) {
			r = offset - n;
			if(r > sizeof(slop))
				r = sizeof(slop);
			osenter();
			r = read(FS(c)->fd, slop, r);
			osleave();
			if(r <= 0){
				FS(c)->offset = n;
				return 0;
			}
			n += r;
		}
		FS(c)->offset = offset;
	}
	osenter();
	r = read(FS(c)->fd, va, count);
	osleave();
	if(r < 0)
		return r;
	FS(c)->offset = offset+r;
	return r;
}

long
fsread(Chan *c, void *va, long n, vlong offset)
{
	int r;

	if(c->qid.type & QTDIR){	/* need to maintain offset only for directories */
		qlock(FS(c));
		if(waserror()){
			qunlock(FS(c));
			nexterror();
		}
		r = fsdirread(c, va, n, offset);
		poperror();
		qunlock(FS(c));
	}else{
		osenter();
		r = pread(FS(c)->fd, va, n, offset);
		osleave();
	}
	if(r < 0)
		fserr(FS(c));
	return r;
}

long
fswrite(Chan *c, void *va, long n, vlong offset)
{
	int r;

	osenter();
	r = pwrite(FS(c)->fd, va, n, offset);
	osleave();
	if(r < 0)
		fserr(FS(c));
	return r;
}

void
fsremove(Chan *c)
{
	int r;

	if(waserror()){
		fsfree(c);
		nexterror();
	}
	osenter();
	r = remove(FS(c)->name->s);
	osleave();
	if(r < 0)
		fserr(FS(c));
	poperror();
	fsfree(c);
}

int
fswstat(Chan *c, uchar *dp, int n)
{
	osenter();
	if(FS(c)->fd >= 0)
		n = fwstat(FS(c)->fd, dp, n);
	else
		n = wstat(FS(c)->name->s, dp, n);
	osleave();
	if(n < 0)
		fserr(FS(c));
	return n;
}

void
setid(char *name, int owner)
{
	if(!owner || iseve())
		kstrdup(&up->env->user, name);
}

Dev fsdevtab = {
	'U',
	"fs",

	devinit,
	fsattach,
	fswalk,
	fsstat,
	fsopen,
	fscreate,
	fsclose,
	fsread,
	devbread,
	fswrite,
	devbwrite,
	fsremove,
	fswstat
};
