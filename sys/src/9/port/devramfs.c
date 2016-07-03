/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define DIR_EXTEND_SIZE 10

int memcpy(void* dst, const void* src, int len);

struct Directory {
	int inuse;
	int allocated;
	Dirtab ramdir[];
};

struct RamFile {
	int length;
	char data[];
};

static struct Directory *ramroot;

static char* readme = "This is the kernel ramdisk.\n";

static void
raminit(void)
{
	ramroot = (struct Directory *)smalloc(sizeof(struct Directory) + DIR_EXTEND_SIZE * sizeof(Dirtab));
	ramroot->inuse = 3;
	ramroot->allocated = DIR_EXTEND_SIZE;
	strcpy(ramroot->ramdir[0].name, ".");
	ramroot->ramdir[0].qid = (Qid){(int64_t)ramroot, 0,QTDIR};
	ramroot->ramdir[0].length = 0;
	ramroot->ramdir[0].perm = DMDIR|0777;

	struct RamFile* file = (struct RamFile*)smalloc(sizeof(struct RamFile*) + strlen(readme));
	file->length = strlen(readme);
	memcpy(file->data, readme, file->length);
	strcpy(ramroot->ramdir[1].name, "README");
	ramroot->ramdir[1].qid = (Qid){(int64_t)file};
	ramroot->ramdir[1].length = file->length;
	ramroot->ramdir[1].perm = 0444;

	struct Directory *subdir = (struct Directory *)smalloc(sizeof(struct Directory) + DIR_EXTEND_SIZE * sizeof(Dirtab));
	strcpy(ramroot->ramdir[2].name, "subdir");
	ramroot->ramdir[2].qid = (Qid){(int64_t)subdir, 0, QTDIR};
	ramroot->ramdir[2].length = 0;
        ramroot->ramdir[2].perm = DMDIR|0777;

	subdir->inuse = 2;
        subdir->allocated = DIR_EXTEND_SIZE;
        strcpy(subdir->ramdir[0].name, ".");
        subdir->ramdir[0].qid = (Qid){(int64_t)subdir, 0,QTDIR};
        subdir->ramdir[0].length = 0;
        subdir->ramdir[0].perm = DMDIR|0777;


        strcpy(subdir->ramdir[1].name, "README2");
        subdir->ramdir[1].qid = (Qid){(int64_t)file};
        subdir->ramdir[1].length = file->length;
        subdir->ramdir[1].perm = 0444;

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

static Walkqid*
ramwalk(Chan *c, Chan *nc, char **name, int nname)
{
        Proc *up = externup();
	struct Directory *directory = (struct Directory *)c->qid.path;
	Dirtab *tab = directory->ramdir;
	int ntab = directory->inuse;
        int i, j, alloc;
        Walkqid *wq;
        char *n;
        Dir dir;

        if(nname > 0)
                isdir(c);

        alloc = 0;
        wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
        if(waserror()){
                if(alloc && wq->clone!=nil)
                        cclose(wq->clone);
                free(wq);
                return nil;
        }
        if(nc == nil){
                nc = devclone(c);
                alloc = 1;
        }
        wq->clone = nc;

        for(j=0; j<nname; j++){
                if(!(nc->qid.type & QTDIR)){
                        if(j==0)
                                error(Enotdir);
                        goto Done;
                }
                n = name[j];
                if(strcmp(n, ".") == 0){
    Accept:
                        wq->qid[wq->nqid++] = nc->qid;
                        continue;
                }
                if(strcmp(n, "..") == 0){
                        /*
                         * Use c->dev->name in the error because
                         * nc->dev should be nil here.
                         */
                        if(devgen(nc, nil, tab, ntab, DEVDOTDOT, &dir) != 1){
                                print("devgen walk .. in dev%s %#llux broken\n",
                                        c->dev->name, nc->qid.path);
                                error("broken devgen");
                        }
                        nc->qid = dir.qid;
                        goto Accept;
                }
                /*
                 * Ugly problem: If we're using devgen, make sure we're
                 * walking the directory itself, represented by the first
                 * entry in the table, and not trying to step into a sub-
                 * directory of the table, e.g. /net/net. Devgen itself
                 * should take care of the problem, but it doesn't have
                 * the necessary information (that we're doing a walk).
                 */
                if(nc->qid.path!=tab[0].qid.path)
                        goto Notfound;
                for(i=0;; i++) {
                        switch(devgen(nc, n, tab, ntab, i, &dir)){
                        case -1:
                        Notfound:
                                if(j == 0)
                                        error(Enonexist);
                                kstrcpy(up->errstr, Enonexist, ERRMAX);
                                goto Done;
                        case 0:
                                continue;
                        case 1:
                                if(strcmp(n, dir.name) == 0){
                                        nc->qid = dir.qid;
					directory = (struct Directory *)nc->qid.path;
					tab = directory->ramdir;
					ntab = directory->inuse;
                                        goto Accept;
                                }
                                continue;
                        }
                }
        }
        /*
         * We processed at least one name, so will return some data.
         * If we didn't process all nname entries succesfully, we drop
         * the cloned channel and return just the Qids of the walks.
         */
Done:
        poperror();
        if(wq->nqid < nname){
                if(alloc)
                        cclose(wq->clone);
                wq->clone = nil;
        }else if(wq->clone){
                /* attach cloned channel to same device */
                wq->clone->dev = c->dev;
                //if(wq->clone->dev)                    //XDYNX
                //      devtabincr(wq->clone->dev);
        }
        return wq;
}

static int32_t
ramstat(Chan *c, uint8_t *dp, int32_t n)
{
	struct Directory *dir = (struct Directory *)c->qid.path;
	return devstat(c, dp, n, dir->ramdir, dir->inuse, devgen);
}

static Chan*
ramopen(Chan *c, int omode)
{
	struct Directory *dir = (struct Directory *)c->qid.path;
	return devopen(c, omode, dir->ramdir, dir->inuse, devgen);
}

/*
 * sysremove() knows this is a nop
 */
static void
ramclose(Chan* c)
{
}

static int32_t
ramread(Chan *c, void *buf, int32_t n, int64_t off)
{
	if (c->qid.type == QTDIR){
		struct Directory *dir = (struct Directory*)c->qid.path;
		return devdirread(c, buf, n, dir->ramdir, dir->inuse, devgen);
	}
	// Read file
	struct RamFile *file = (void*)c->qid.path;
	int filelen = file->length;
	if (off > filelen){
		return 0;
	}
	if (off + n > filelen){
		n = filelen - off;
	}
	memcpy(buf, file->data, n);
	return n;
}

static int32_t
ramwrite(Chan* c, void* v, int32_t n, int64_t m)
{
	error(Egreg);		// TODO
	return 0;
}

Dev ramdevtab = {
	.dc = '@',
	.name = "ram",

	.reset = ramreset,
	.init = raminit,
	.shutdown = devshutdown,
	.attach = ramattach,
	.walk = ramwalk,
	.stat = ramstat,
	.open = ramopen,
	.create = devcreate,
	.close = ramclose,
	.read = ramread,
	.bread = devbread,
	.write = ramwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
