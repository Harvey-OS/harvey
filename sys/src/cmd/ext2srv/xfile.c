#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"


static Xfs	*xhead;
static Xfile *freelist;
static Lock	xlock, freelock;

int	client;

Xfs *
getxfs(char *name)
{
	int fd;
	Dir *dir;
	Xfs *xf, *fxf;

	if(name==0 || name[0]==0)
		name = deffile;
	if(name == 0){
		errno = Enofilsys;
		return 0;
	}
	fd = open(name, rdonly ? OREAD : ORDWR);
	if(fd < 0){
		errno = Enonexist;
		return 0;
	}
	if((dir = dirfstat(fd)) == 0){
		errno = Eio;
		close(fd);
		return 0;
	}
	lock(&xlock);
	for(fxf=0, xf=xhead; xf; xf=xf->next){
		if(xf->ref == 0){
			if(fxf == 0)
				fxf = xf;
			continue;
		}
		if(xf->qid.path != dir->qid.path || xf->qid.vers != dir->qid.vers)
			continue;
		if(strcmp(xf->name, name) != 0 || xf->dev < 0)
			continue;
		chat("incref \"%s\", dev=%d...", xf->name, xf->dev);
		++xf->ref;
		unlock(&xlock);
		close(fd);
		free(dir);
		return xf;
	}
	if(fxf==0){
		fxf = malloc(sizeof(Xfs));
		if(fxf==0){
			unlock(&xlock);
			close(fd);
			free(dir);
			errno = Enomem;
			return 0;
		}
		fxf->next = xhead;
		xhead = fxf;
	}
	chat("alloc \"%s\", dev=%d...", name, fd);
	fxf->name = strdup(name);
	fxf->ref = 1;
	fxf->qid = dir->qid;
	fxf->dev = fd;
	fxf->fmt = 0;
	fxf->ptr = 0;
	free(dir);
	if( ext2fs(fxf)<0 ){ 
		xhead = fxf->next;
		free(fxf);
		unlock(&xlock);
		return 0;
	}
	unlock(&xlock);
	return fxf;
}

void
refxfs(Xfs *xf, int delta)
{
	lock(&xlock);
	xf->ref += delta;
	if(xf->ref == 0){
		/*mchat("free \"%s\", dev=%d...", xf->name, xf->dev);
		dumpbuf();*/
		CleanSuper(xf);
		syncbuf();
		free(xf->name);
		purgebuf(xf);
		if(xf->dev >= 0){
			close(xf->dev);
			xf->dev = -1;
		}
	}
	unlock(&xlock);
}

Xfile *
xfile(Fid *fid, int flag)
{
	Xfile *f;

	f = (Xfile*)fid->aux;
	switch(flag){
	default:
		panic("xfile");
	case Asis:
		return (f && f->xf && f->xf->dev < 0) ? 0 : f;
	case Clean:
		if (f) chat("Clean and fid->aux already exists\n");
		break;
	case Clunk:
		if(f){
			clean(f);
			lock(&freelock);
			f->next = freelist;
			freelist = f;
			unlock(&freelock);
			fid->aux = 0;
		}
		return 0;
	}
	if(f)
		return clean(f);
	lock(&freelock);
	if(f = freelist){	/* assign = */
		freelist = f->next;
		unlock(&freelock);
	} else {
		unlock(&freelock);
		f = malloc(sizeof(Xfile));
	}
	fid->aux = f;
	f->fid = fid->fid;
	f->client = client;
	f->xf = 0;
	f->ptr = 0;
	f->root = 0;
	return f;
}
Xfile *
clean(Xfile *f)
{
	if(f->xf && f->root){
		refxfs(f->xf, -1);
		f->xf = 0;
	}
	f->xf = 0;
	f->root = 0;
	f->dirindex = 0;
	return f;
}
