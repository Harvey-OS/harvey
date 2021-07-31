#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

#define	FIDMOD	127	/* prime */

static Xfs	*xhead;
static Xfile	*xfiles[FIDMOD], *freelist;
static Lock	xlock, xlocks[FIDMOD], freelock;

int	client;

Xfs *
getxfs(char *name)
{
	int fd;
	Dir dir;
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
	if(dirfstat(fd, &dir) < 0){
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
		if(xf->qid.path != dir.qid.path || xf->qid.vers != dir.qid.vers)
			continue;
		if(strcmp(xf->name, name) != 0 || xf->dev < 0)
			continue;
		chat("incref \"%s\", dev=%d...", xf->name, xf->dev);
		++xf->ref;
		unlock(&xlock);
		close(fd);
		return xf;
	}
	if(fxf==0){
		fxf = malloc(sizeof(Xfs));
		if(fxf==0){
			unlock(&xlock);
			close(fd);
			errno = Enomem;
			return 0;
		}
		fxf->next = xhead;
		xhead = fxf;
	}
	chat("alloc \"%s\", dev=%d...", name, fd);
	fxf->name = strdup(name);
	fxf->ref = 1;
	fxf->qid = dir.qid;
	fxf->dev = fd;
	fxf->fmt = 0;
	fxf->ptr = 0;
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
xfile(int fid, int flag)
{
	int k = ((ulong)fid^(ulong)client)%FIDMOD;
	Xfile **hp=&xfiles[k], *f, *pf;

	lock(&xlocks[k]);
	for(f=*hp,pf=0; f; pf=f, f=f->next)
		if(f->fid == fid && f->client == client)
			break;
	if(f && pf){
		pf->next = f->next;
		f->next = *hp;
		*hp = f;
	}
	switch(flag){
	default:
		panic("xfile");
	case Asis:
		unlock(&xlocks[k]);
		return (f && f->xf && f->xf->dev < 0) ? 0 : f;
	case Clean:
		break;
	case Clunk:
		if(f){
			*hp = f->next;
			unlock(&xlocks[k]);
			clean(f);
			lock(&freelock);
			f->next = freelist;
			freelist = f;
			unlock(&freelock);
		} else
			unlock(&xlocks[k]);
		return 0;
	}
	unlock(&xlocks[k]);
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
	lock(&xlocks[k]);
	f->next = *hp;
	*hp = f;
	unlock(&xlocks[k]);
	f->fid = fid;
	f->client = client;
	f->flags = 0;
	f->qid = (Qid){0,0};
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
	f->flags = 0;
	f->qid = (Qid){0,0};
	f->root = 0;
	return f;
}
int
xfspurge(void)
{
	Xfile **hp, *f, *pf, *nf;
	int k, count=0;

	for(k=0; k<FIDMOD; k++){
		lock(&xlocks[k]);
		hp=&xfiles[k];
		for(f=*hp,pf=0; f; f=nf){
			nf = f->next;
			if(f->client != client){
				pf = f;
				continue;
			}
			if (pf)
				pf->next = f->next;
			else
				*hp = f->next;
			clean(f);
			lock(&freelock);
			f->next = freelist;
			freelist = f;
			unlock(&freelock);
			++count;
		}
		unlock(&xlocks[k]);
	}
	return count;
}
