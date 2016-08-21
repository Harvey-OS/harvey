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

#define RAM_BLOCK_LEN 32768
#define RAM_MAGIC 0xbedabb1e
#define INVALID_FILE "Invalid ram file"

struct RamFile {
	unsigned int	magic;
	char    name[KNAMELEN];
	struct RamFile *parent;
	struct RamFile *sibling;
	uint64_t length;
	uint64_t alloclength;
	int busy;
	int perm;
	int opencount;
	int deleteonclose;
	union {
		uint8_t	*data;		// List of children if directory
		struct RamFile* firstchild;
	};
};

static struct RamFile *ramroot;
static QLock ramlock;


static void
printramfile(int offset, struct RamFile* file)
{
	int i;
	for(i = 0; i < offset; i++) {
		print(" ");
	}
	print("ramfile: %x, magic:%x, name: %s, parent: %x, sibling:%x, length:%d, alloclength: %d, perm: %o\n", 
			file, file->magic, file->name, file->parent, file->sibling, file->length, file->alloclength, file->perm);
}

static void
debugwalkinternal(int offset, struct RamFile* current)
{
	printramfile(offset, current);
	for (current = current->firstchild; current != nil; current = current->sibling) {
		if(current->perm & DMDIR){
			debugwalkinternal(offset+1, current);
		}else{
			printramfile(offset,current);
		}
	}
}

static void
debugwalk()
{
	print("***********************\n");
	debugwalkinternal(0, ramroot);
}

static void
raminit(void)
{
	ramroot = (struct RamFile *)malloc(sizeof(struct RamFile));
	if (ramroot == nil) {
                error(Eperm);
	}
	strcpy(ramroot->name, ".");
	ramroot->magic = RAM_MAGIC;
	ramroot->length = 0;
	ramroot->alloclength = 0;
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
        buf = malloc(1+UTFmax+strlen(spec)+1);
	if (buf == nil) {
                error(Eperm);
	}
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
	if (current->magic != RAM_MAGIC)
		error(INVALID_FILE);

	qlock(&ramlock);
	mkqid(&qid, c->qid.path, 0, current->perm & DMDIR ? QTDIR : 0);
	devdir(c, qid, current->name, current->length, "harvey", 0555, &dir);
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
	struct RamFile* file = (struct RamFile*)c->qid.path;
	if (file->magic != RAM_MAGIC)
		panic("Invalid ram file");
	file->busy++;
	qunlock(&ramlock);
	poperror();
	return ret;
}

static void
delete(struct RamFile* file)
{
	qlock(&ramlock);
	//printramfile(0, file);
	//debugwalk();
	struct RamFile* prev = file->parent->firstchild;
	if(prev == file) {
		// This is the first file - make any sibling the first child
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
	if(!file->perm & DMDIR){
		free(file->data);
	}
	file->magic = 0;
	free(file);
	if (0) debugwalk();
	qunlock(&ramlock);
}

static void
ramclose(Chan* c)
{
        struct RamFile* file = (struct RamFile *)c->qid.path;
	if (file->magic != RAM_MAGIC)
		error(INVALID_FILE);
	qlock(&ramlock);
	file->busy--;
	qunlock(&ramlock);
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
	if (file->magic != RAM_MAGIC)
		error(INVALID_FILE);
	int filelen = file->length;
	if (off > filelen){
		qunlock(&ramlock);
		return 0;
	}
	if (off + n > filelen){
		n = filelen - off;
	}
	memmove(buf, file->data, n);
	qunlock(&ramlock);
	return n;
}

typedef double Align;
typedef union Header Header;

union Header {
        struct {
                Header* next;
                uint    size;
        } s;
        Align   al;
};


static int32_t
ramwrite(Chan* c, void* v, int32_t n, int64_t off)
{
//	Header *p;

	qlock(&ramlock);
	struct RamFile *file = (void*)c->qid.path;
	if (file->magic != RAM_MAGIC)
		error(INVALID_FILE);
	if(n+off >= file->length){
		uint64_t alloclength = file->alloclength;
		while(alloclength < n + off)
			alloclength += RAM_BLOCK_LEN;
		if(alloclength > file->alloclength){
			void *newfile = realloc(file->data, alloclength);
			if(newfile == nil){
				qunlock(&ramlock);
				return 0;
			}
			file->data = newfile;
			file->alloclength = alloclength;
		}
		file->length = n+off;
	}
//	p = (Header*)file->data - 1;
//	print("length of buffer=%d, header size=%d, header next=%x, start of write=%d, end of write=%d\n", file->alloclength, p->s.size, p->s.next, off, off + n);
	memmove(file->data + off, v, n);
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
	if (parent->magic != RAM_MAGIC)
		error(INVALID_FILE);

        omode = openmode(omode);
        struct RamFile* file = (struct RamFile*)malloc(sizeof(struct RamFile));
	if (file == nil) {
                error(Eperm);
	}
        file->length = 0;
	file->magic = RAM_MAGIC;
        strcpy(file->name, name);
        file->perm = perm;
        file->parent = parent;
	file->busy = 1;

	qlock(&ramlock);
        if(waserror()) {
		free(file->data);
		free(file);
		qunlock(&ramlock);
                nexterror();
        }

	file->sibling = parent->firstchild;
	parent->firstchild = file;
	mkqid(&c->qid, (uintptr_t)file, 0, file->perm & DMDIR ? QTDIR : 0);

        c->offset = 0;
        c->mode = omode;
        c->flag |= COPEN;
	qunlock(&ramlock);

        poperror();
}

void
ramshutdown(void)
{
}

void
ramremove(Chan* c)
{
	struct RamFile* doomed = (struct RamFile *)c->qid.path;
	if (doomed->magic != RAM_MAGIC)
		error(INVALID_FILE);
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
