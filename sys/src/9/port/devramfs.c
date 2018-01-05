/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include       "../port/lib.h"
#include       "mem.h"
#include       "dat.h"
#include       "fns.h"
#include	"../port/error.h"

struct RamFile {
	char    name[KNAMELEN];
	struct RamFile *parent;
	struct RamFile *sibling;
	uint64_t length;
	int perm;
	int opencount;
	int deleteonclose;
	union {
		uint8_t	*data;		// List of children if directory
		struct RamFile* firstchild;
	}
};

static struct RamFile *ramroot;
static QLock ramlock;

static void
raminit(void)
{
	ramroot = (struct RamFile *)smalloc(sizeof(struct RamFile));
	strcpy(ramroot->name, ".");
	ramroot->length = 0;
	ramroot->perm = DMDIR|0777;
	ramroot->firstchild = nil;
}

static void
ramreset(void)
{
}

static Chan*
ramattach(char *spec)
{
        Chan *c;
        char *buf;
         
        c = newchan();
        mkqid(&c->qid, (int64_t)ramroot, 0, QTDIR);
        c->dev = devtabget('@', 0);
        if(spec == nil)
                spec = "";
        buf = smalloc(1+UTFmax+strlen(spec)+1);
        sprint(buf, "#@%s", spec);
        c->path = newpath(buf);
	c->mode = 0777;
        free(buf);
        return c;
}

static int
ramgen(Chan *c, char *name, Dirtab *tab, int ntab, int pos, Dir *dp)
{
	Qid qid;
	int i;

	struct RamFile *current = (struct RamFile *)c->qid.path;
	if(pos == DEVDOTDOT){
		if(current->parent == nil){
			mkqid(&qid, (uintptr_t)current, 0, QTDIR);
			devdir(c, qid, "#@", 0, "harvey", 0555, dp);
			return 1;
		} else {
		        mkqid(&qid, (uintptr_t)current->parent, 0, QTDIR);
		        devdir(c, qid, current->name, 0, "harvey", 0555, dp);
		        return 1;
		}
	}
	if(current->perm & QTDIR){
		current = current->firstchild;
		if(current == nil){
			return -1;
		}
	}
	for(i = 0; i < pos; i++){
		current = current->sibling;
		if (current == nil){
			return -1;
		}

	}
	mkqid(&qid, (uintptr_t)current, 0, current->perm & DMDIR ? QTDIR : 0);
	devdir(c, qid, current->name, current->length, "harvey", current->perm, dp);
	if(name == nil || strcmp(current->name, name) == 0){
		return 1;
	} else {
		return 0;
	}
}

static Walkqid*
ramwalk(Chan *c, Chan *nc, char **name, int nname)
{
	Proc *up = externup();
	qlock(&ramlock);
	if(waserror()){
		qunlock(&ramlock);
		nexterror();
	}
	Walkqid* wqid =  devwalk(c, nc, name, nname, 0, 0, ramgen);
	qunlock(&ramlock);
	poperror();
	return wqid;
}

static int32_t
ramstat(Chan *c, uint8_t *dp, int32_t n)
{
	Dir dir;
	Qid qid;
	struct RamFile* current = (struct RamFile*)c->qid.path;

	qlock(&ramlock);
	mkqid(&qid, c->qid.path, 0, current->perm & DMDIR ? QTDIR : 0);
	devdir(c, qid, current->name, 0, "harvey", 0555, &dir);
	int32_t ret = convD2M(&dir, dp, n);
	qunlock(&ramlock);
	return ret;
}

static Chan*
ramopen(Chan *c, int omode)
{
	Proc *up = externup();
	qlock(&ramlock);
	if(waserror()){
		qunlock(&ramlock);
		nexterror();
	}
	Chan* ret = devopen(c, omode, nil, 0, ramgen);
	qunlock(&ramlock);
	poperror();
	return ret;
}

static void
delete(struct RamFile* file)
{
	qlock(&ramlock);
	struct RamFile* prev = file->parent->firstchild;
	if(prev == file) {
		file->parent->firstchild = file->sibling;
	} else {
		// Find previous file
		for(; prev != nil && prev->sibling != file; prev = prev->sibling)
			;
		if(prev == nil){
			qunlock(&ramlock);
			error(Eperm);
		} else {
			prev->sibling = file->sibling;
		}
	}
	if(file->perm & DMDIR){
		free(file->data);
	}
	free(file);
	qunlock(&ramlock);
}

static void
ramclose(Chan* c)
{
        struct RamFile* file = (struct RamFile *)c->qid.path;
        if(file->deleteonclose){
               delete(file);
        }
}

static int32_t
ramread(Chan *c, void *buf, int32_t n, int64_t off)
{
	qlock(&ramlock);
	if (c->qid.type == QTDIR){
		int32_t len =  devdirread(c, buf, n, nil, 0, ramgen);
		qunlock(&ramlock);
		return len;
	}
	// Read file
	struct RamFile *file = (void*)c->qid.path;
	int filelen = file->length;
	if (off > filelen){
		qunlock(&ramlock);
		return 0;
	}
	if (off + n > filelen){
		n = filelen - off;
	}
	memcpy(buf, file->data, n);
	qunlock(&ramlock);
	return n;
}

static int32_t
ramwrite(Chan* c, void* v, int32_t n, int64_t off)
{
	qlock(&ramlock);
	struct RamFile *file = (void*)c->qid.path;
	if(n+off > file->length){
		void *newfile = realloc(file->data, n+off);
		if(newfile == nil){
			return 0;
		}
		file->data = newfile;
		file->length = n+off;
	}
	memcpy(file->data + off, v, n);
	qunlock(&ramlock);
	return n;
}

void
ramcreate(Chan* c, char *name, int omode, int perm)
{
        Proc *up = externup();

        if(c->qid.type != QTDIR)
                error(Eperm);

	struct RamFile* parent = (struct RamFile *)c->qid.path;

        omode = openmode(omode);
        struct RamFile* file = (struct RamFile*)smalloc(sizeof(struct RamFile));
        file->length = 0;
        strcpy(file->name, name);
        file->perm = perm;
        file->parent = parent;

	qlock(&ramlock);
        if(waserror()) {
		free(file->data);
		free(file);
		qunlock(&ramlock);
                nexterror();
        }

	file->sibling = parent->firstchild;
	parent->firstchild = file;
	qunlock(&ramlock);

	mkqid(&c->qid, (uintptr_t)file, 0, file->perm & DMDIR ? QTDIR : 0);

        poperror();

        c->offset = 0;
        c->mode = omode;
        c->flag |= COPEN;
}

void
ramshutdown(void)
{
}

void
ramremove(Chan* c)
{
	struct RamFile* doomed = (struct RamFile *)c->qid.path;
	if(doomed->opencount == 0){
		delete(doomed);
	}
	doomed->deleteonclose =1;
}

Dev ramdevtab = {
	.dc = '@',
	.name = "ram",

	.reset = ramreset,
	.init = raminit,
	.shutdown = ramshutdown,
	.attach = ramattach,
	.walk = ramwalk,
	.stat = ramstat,
	.open = ramopen,
	.create = ramcreate,
	.close = ramclose,
	.read = ramread,
	.bread = devbread,
	.write = ramwrite,
	.bwrite = devbwrite,
	.remove = ramremove,
	.wstat = devwstat,
};
