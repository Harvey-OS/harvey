#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

#define	FIDMOD	127	/* prime */

static Xfs	*xhead;
static Xfile	*xfiles[FIDMOD], *freelist;
static MLock	xlock, xlocks[FIDMOD], freelock;

static int
okmode(int omode, int fmode)
{
	if(omode == OREAD)
		return fmode & 4;
	/* else ORDWR */
	return (fmode & 6) == 6;
}

Xfs *
getxfs(char *user, char *name)
{
	Xfs *xf, *fxf;
	Dir *dir;
	Qid dqid;
	char *p, *q;
	long offset;
	int fd, omode;

	USED(user);
	if(name==nil || name[0]==0)
		name = deffile;
	if(name == nil){
		errno = Enofilsys;
		return 0;
	}

	/*
	 * If the name passed is of the form 'name:offset' then
	 * offset is used to prime xf->offset. This allows accessing
	 * a FAT-based filesystem anywhere within a partition.
	 * Typical use would be to mount a filesystem in the presence
	 * of a boot manager programme at the beginning of the disc.
	 */
	offset = 0;
	if(p = strrchr(name, ':')){
		*p++ = 0;
		offset = strtol(p, &q, 0);
		chat("name %s, offset %ld\n", p, offset);
		if(offset < 0 || p == q){
			errno = Enofilsys;
			return 0;
		}
		offset *= Sectorsize;
	}

	if(readonly)
		omode = OREAD;
	else
		omode = ORDWR;
	fd = open(name, omode);

	if(fd < 0 && omode==ORDWR){
		omode = OREAD;
		fd = open(name, omode);
	}

	if(fd < 0){
		chat("can't open %s: %r\n", name);
		errno = Eerrstr;
		return 0;
	}
	dir = dirfstat(fd);
	if(dir == nil){
		errno = Eio;
		close(fd);
		return 0;
	}

	dqid = dir->qid;
	free(dir);
	mlock(&xlock);
	for(fxf=0,xf=xhead; xf; xf=xf->next){
		if(xf->ref == 0){
			if(fxf == 0)
				fxf = xf;
			continue;
		}
		if(!eqqid(xf->qid, dqid))
			continue;
		if(strcmp(xf->name, name) != 0 || xf->dev < 0)
			continue;
		if(devcheck(xf) < 0) /* look for media change */
			continue;
		if(offset && xf->offset != offset)
			continue;
		chat("incref \"%s\", dev=%d...", xf->name, xf->dev);
		++xf->ref;
		unmlock(&xlock);
		close(fd);
		return xf;
	}
	if(fxf == nil){
		fxf = malloc(sizeof(Xfs));
		if(fxf == nil){
			unmlock(&xlock);
			close(fd);
			errno = Enomem;
			return nil;
		}
		fxf->next = xhead;
		xhead = fxf;
	}
	chat("alloc \"%s\", dev=%d...", name, fd);
	fxf->name = strdup(name);
	fxf->ref = 1;
	fxf->qid = dqid;
	fxf->dev = fd;
	fxf->fmt = 0;
	fxf->offset = offset;
	fxf->ptr = nil;
	fxf->isfat32 = 0;
	fxf->omode = omode;
	unmlock(&xlock);
	return fxf;
}

void
refxfs(Xfs *xf, int delta)
{
	mlock(&xlock);
	xf->ref += delta;
	if(xf->ref == 0){
		chat("free \"%s\", dev=%d...", xf->name, xf->dev);
		free(xf->name);
		free(xf->ptr);
		purgebuf(xf);
		if(xf->dev >= 0){
			close(xf->dev);
			xf->dev = -1;
		}
	}
	unmlock(&xlock);
}

Xfile *
xfile(int fid, int flag)
{
	Xfile **hp, *f, *pf;
	int k;

	k = ((ulong)fid) % FIDMOD;
	hp = &xfiles[k];
	mlock(&xlocks[k]);
	pf = nil;
	for(f=*hp; f; f=f->next){
		if(f->fid == fid)
			break;
		pf = f;
	}
	if(f && pf){
		pf->next = f->next;
		f->next = *hp;
		*hp = f;
	}
	switch(flag){
	default:
		panic("xfile");
	case Asis:
		unmlock(&xlocks[k]);
		return (f && f->xf && f->xf->dev < 0) ? nil : f;
	case Clean:
		break;
	case Clunk:
		if(f){
			*hp = f->next;
			unmlock(&xlocks[k]);
			clean(f);
			mlock(&freelock);
			f->next = freelist;
			freelist = f;
			unmlock(&freelock);
		} else
			unmlock(&xlocks[k]);
		return nil;
	}
	unmlock(&xlocks[k]);
	if(f)
		return clean(f);
	mlock(&freelock);
	if(f = freelist){	/* assign = */
		freelist = f->next;
		unmlock(&freelock);
	} else {
		unmlock(&freelock);
		f = malloc(sizeof(Xfile));
		if(f == nil){
			errno = Enomem;
			return nil;
		}
	}
	mlock(&xlocks[k]);
	f->next = *hp;
	*hp = f;
	unmlock(&xlocks[k]);
	f->fid = fid;
	f->flags = 0;
	f->qid = (Qid){0,0,0};
	f->xf = nil;
	f->ptr = nil;
	return f;
}

Xfile *
clean(Xfile *f)
{
	if(f->ptr){
		free(f->ptr);
		f->ptr = nil;
	}
	if(f->xf){
		refxfs(f->xf, -1);
		f->xf = nil;
	}
	f->flags = 0;
	f->qid = (Qid){0,0,0};
	return f;
}

/*
 * the file at <addr, offset> has moved
 * relocate the dos entries of all fids in the same file
 */
void
dosptrreloc(Xfile *f, Dosptr *dp, ulong addr, ulong offset)
{
	int i;
	Xfile *p;
	Dosptr *xdp;

	for(i=0; i < FIDMOD; i++){
		for(p = xfiles[i]; p != nil; p = p->next){
			xdp = p->ptr;
			if(p != f && p->xf == f->xf
			&& xdp != nil && xdp->addr == addr && xdp->offset == offset){
				memmove(xdp, dp, sizeof(Dosptr));
				xdp->p = nil;
				xdp->d = nil;
				p->qid.path = QIDPATH(xdp);
			}
		}
	}
}
