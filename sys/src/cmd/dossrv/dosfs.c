#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

extern Fcall	thdr;
extern Fcall	rhdr;
extern char	data[sizeof(Fcall)+MAXFDATA];
extern char	fdata[MAXFDATA];
extern int	errno;

void
rnop(void)
{
	chat("nop...");
}
void
rsession(void)
{
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	chat("session...");
}
void
rflush(void)
{
	chat("flush...");
}
void
rattach(void)
{
	Xfs *xf;
	Xfile *root;
	Dosptr *dp;

	chat("attach(fid=%d,uname=\"%s\",aname=\"%s\",auth=\"%s\")...",
		thdr.fid, thdr.uname, thdr.aname, thdr.auth);

	root = xfile(thdr.fid, Clean);
	if(!root){
		errno = Enomem;
		goto error;
	}
	root->xf = xf = getxfs(thdr.aname);
	if(!xf)
		goto error;
	if(xf->fmt==0 && dosfs(xf)<0){
		errno = Eformat;
		goto error;
	}
	root->qid.path = CHDIR;
	root->qid.vers = 0;
	root->xf->rootqid = root->qid;
	dp = malloc(sizeof(Dosptr));
	memset(dp, 0, sizeof(Dosptr));
	root->ptr = dp;
	rhdr.qid = root->qid;
	return;
error:
	if(root)
		xfile(thdr.fid, Clunk);
	return;
}
void
rclone(void)
{
	Xfile *of = xfile(thdr.fid, Asis);
	Xfile *nf = xfile(thdr.newfid, Clean);

	chat("clone(fid=%d,newfid=%d)...", thdr.fid, thdr.newfid);
	if(!of)
		errno = Eio;
	else if(!nf)
		errno = Enomem;
	else{
		Xfile *next = nf->next;
		Dosptr *dp = malloc(sizeof(Dosptr));
		*nf = *of;
		nf->next = next;
		nf->fid = thdr.newfid;
		nf->ptr = dp;
		refxfs(nf->xf, 1);
		memmove(dp, of->ptr, sizeof(Dosptr));
		dp->p = 0;
		dp->d = 0;
	}
}
void
rwalk(void)
{
	Xfile *f=xfile(thdr.fid, Asis);
	Dosptr dp[1];
	int r;

	chat("walk(fid=%d,name=\"%s\")...", thdr.fid, thdr.name);
	if(!f){
		chat("no xfile...");
		goto error;
	}
	if(!(f->qid.path & CHDIR)){
		chat("qid.path=0x%x...", f->qid.path);
		goto error;
	}
	if(strcmp(thdr.name, ".")==0){
		rhdr.qid = f->qid;
		return;
	}else if(strcmp(thdr.name, "..")==0){
		if(f->qid.path==f->xf->rootqid.path){
			chat("walkup from root...");
			rhdr.qid = f->qid;
			return;
		}
		r = walkup(f, dp);
		if(r < 0)
			goto error;
		memmove(f->ptr, dp, sizeof(Dosptr));
		if(dp->addr == 0)
			f->qid.path = f->xf->rootqid.path;
		else{
			Iosect *p; Dosdir *xd;
			p = getsect(f->xf, dp->addr);
			if(p == 0)
				goto error;
			xd = (Dosdir *)&p->iobuf[dp->offset];
			f->qid.path = CHDIR | GSHORT(xd->start);
			putsect(p);
		}
	}else{
		if(getfile(f) < 0)
			goto error;
		r = searchdir(f, thdr.name, dp, 0);
		putfile(f);
		if(r < 0)
			goto error;
		memmove(f->ptr, dp, sizeof(Dosptr));
		f->qid.path = GSHORT(dp->d->start);
		if(f->qid.path == 0)
			f->qid.path = f->xf->rootqid.path;
		else if(dp->d->attr & DDIR)
			f->qid.path |= CHDIR;
		putfile(f);
	}
	rhdr.qid = f->qid;
	return;
error:
	errno = Enonexist;
	return;
}
void
ropen(void)
{
	Xfile *f;
	Dosptr *dp;
	int attr, omode=0;

	chat("open(fid=%d,mode=%d)...", thdr.fid, thdr.mode);
	f = xfile(thdr.fid, Asis);
	if(!f || (f->flags&Omodes)){
		errno = Eio;
		return;
	}
	dp = f->ptr;
	if(dp->paddr && (thdr.mode & ORCLOSE)){
		/*
		 * check on parent directory of file to be deleted
		 */
		Iosect *p = getsect(f->xf, dp->paddr);
		if(p == 0){
			errno = Eio;
			return;
		}
		attr = ((Dosdir *)&p->iobuf[dp->poffset])->attr;
		putsect(p);
		if(attr & DRONLY){
			errno = Eperm;
			return;
		}
		omode |= Orclose;
	}else if(thdr.mode & ORCLOSE)
		omode |= Orclose;
	if(getfile(f) < 0){
		errno = Enonexist;
		return;
	}
	if(dp->addr)
		attr = dp->d->attr;
	else
		attr = DDIR;
	switch(thdr.mode & 7){
	case OREAD:
	case OEXEC:
		omode |= Oread;
		break;
	case ORDWR:
		omode |= Oread;
		/* fall through */
	case OWRITE:
		omode |= Owrite;
		if(attr & DRONLY){
			errno = Eperm;
			goto out;
		}
		break;
	default:
		errno = Eio;
		goto out;
	}
	if(thdr.mode & OTRUNC){
		if(attr & DDIR || attr & DRONLY){
			errno = Eperm;
			goto out;
		}
		if(truncfile(f, 0) < 0){
			errno = Eio;
			goto out;
		}
	}
	f->flags |= omode;
	chat("f->qid=0x%8.8lux...", f->qid.path);
	rhdr.qid = f->qid;
out:
	putfile(f);
}
void
rcreate(void)
{
	Dosbpb *bp;
	Xfile *f;
	Dosptr *pdp, *ndp;
	Iosect *xp;
	Dosdir *pd, *nd, *xd;
	int attr, omode=0, start;

	chat("create(fid=%d,name=\"%s\",perm=%uo,mode=%d)...",
		thdr.fid, thdr.name, thdr.perm, thdr.mode);
	f = xfile(thdr.fid, Asis);
	if(!f || (f->flags&Omodes) || getfile(f)<0){
		errno = Eio;
		return;
	}
	ndp = malloc(sizeof(Dosptr));
	pdp = f->ptr;
	pd = pdp->addr ? pdp->d : 0;
	attr = pd ? pd->attr : DDIR;
	if(!(attr & DDIR) || (attr & DRONLY)){
badperm:
		if(ndp)
			free(ndp);
		putfile(f);
		errno = Eperm;
		return;
	}
	if(thdr.mode & ORCLOSE)
		omode |= Orclose;
	switch(thdr.mode & 7){
	case OREAD:
	case OEXEC:
		omode |= Oread;
		break;
	case ORDWR:
		omode |= Oread;
		/* fall through */
	case OWRITE:
		omode |= Owrite;
		if(thdr.perm & CHDIR)
			goto badperm;
		break;
	default:
		goto badperm;
	}
	if(strcmp(thdr.name, ".") == 0 || strcmp(thdr.name, "..") == 0)
		goto badperm;
	if(searchdir(f, thdr.name, ndp, 1) < 0)
		goto badperm;
	bp = f->xf->ptr;
	lock(bp);
	start = falloc(f->xf);
	unlock(bp);
	if(start <= 0){
		if(ndp)
			free(ndp);
		putfile(f);
		errno = Eio;
		return;
	}
	/*
	 * now we're committed
	 */
	if(pd){
		puttime(pd, 0);
		pdp->p->flags |= BMOD;
	}
	f->ptr = ndp;
	f->qid.path = start;
	ndp->p = getsect(f->xf, ndp->addr);
	if(ndp->p == 0)
		goto badio;
	ndp->d = (Dosdir *)&ndp->p->iobuf[ndp->offset];
	nd = ndp->d;
	memset(nd, 0, sizeof(Dosdir));
	putname(thdr.name, nd);
	if((thdr.perm & 0222) == 0)
		nd->attr |= DRONLY;
	puttime(nd, 0);
	nd->start[0] = start;
	nd->start[1] = start>>8;
	if(thdr.perm & CHDIR){
		nd->attr |= DDIR;
		f->qid.path |= CHDIR;
		xp = getsect(f->xf, bp->dataaddr+(start-2)*bp->clustsize);
		if(xp == 0)
			goto badio;
		xd = (Dosdir *)&xp->iobuf[0];
		memmove(xd, nd, sizeof(Dosdir));
		memset(xd->name, ' ', sizeof xd->name+sizeof xd->ext);
		xd->name[0] = '.';
		xd = (Dosdir *)&xp->iobuf[sizeof(Dosdir)];
		if(pd)
			memmove(xd, pd, sizeof(Dosdir));
		else{
			memset(xd, 0, sizeof(Dosdir));
			puttime(xd, 0);
			xd->attr = DDIR;
		}
		memset(xd->name, ' ', sizeof xd->name+sizeof xd->ext);
		xd->name[0] = '.';
		xd->name[1] = '.';
		xp->flags |= BMOD;
		putsect(xp);
	}
	ndp->p->flags |= BMOD;
	putfile(f);
	putsect(pdp->p);
	free(pdp);
	f->flags |= omode;
	chat("f->qid=0x%8.8lux...", f->qid.path);
	rhdr.qid = f->qid;
	return;
badio:
	if(ndp->p)
		putfile(f);
	putsect(pdp->p);
	free(pdp);
	errno = Eio;
}
void
rread(void)
{
	Xfile *f; int r;

	chat("read(fid=%d,offset=%d,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	if (!(f=xfile(thdr.fid, Asis)) || !(f->flags&Oread))
		goto error;
	if(f->qid.path & CHDIR){
		thdr.count = (thdr.count/DIRLEN)*DIRLEN;
		if(thdr.count<DIRLEN || thdr.offset%DIRLEN){
			chat("count=%d,offset=%d,DIRLEN=%d...",
				thdr.count, thdr.offset, DIRLEN);
			goto error;
		}
		if(getfile(f) < 0)
			goto error;
		r = readdir(f, fdata, thdr.offset, thdr.count);
	}else{
		if(getfile(f) < 0)
			goto error;
		r = readfile(f, fdata, thdr.offset, thdr.count);
	}
	putfile(f);
	if(r < 0){
error:
		errno = Eio;
	}else{
		rhdr.count = r;
		rhdr.data = fdata;
		chat("rcnt=%d...", r);
	}
}
void
rwrite(void)
{
	Xfile *f; int r;

	chat("write(fid=%d,offset=%d,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	if (!(f=xfile(thdr.fid, Asis)) || !(f->flags&Owrite))
		goto error;
	if(getfile(f) < 0)
		goto error;
	r = writefile(f, thdr.data, thdr.offset, thdr.count);
	putfile(f);
	if(r < 0){
error:
		errno = Eio;
	}else{
		rhdr.count = r;
		chat("rcnt=%d...", r);
	}
}
void
rclunk(void)
{
	chat("clunk(fid=%d)...", thdr.fid);
	xfile(thdr.fid, Clunk);
	sync();
}
void
rremove(void)
{
	Xfile *f=xfile(thdr.fid, Asis);
	Dosptr *dp;
	Iosect *parp; Dosdir *pard;

	chat("remove(fid=%d,name=\"%s\")...", thdr.fid, thdr.name);
	if(!f){
		errno = Eio;
		goto out;
	}
	dp = f->ptr;
	if(!dp->addr){
		chat("root...");
		errno = Eperm;
		goto out;
	}
	/*
	 * check on parent directory of file to be deleted
	 */
	parp = getsect(f->xf, dp->paddr);
	if(parp == 0){
		errno = Eio;
		goto out;
	}
	pard = (Dosdir *)&parp->iobuf[dp->poffset];
	if(dp->paddr && (pard->attr & DRONLY)){
		chat("parent read-only...");
		putsect(parp);
		errno = Eperm;
		goto out;
	}
	if(getfile(f) < 0){
		chat("getfile failed...");
		putsect(parp);
		errno = Eio;
		goto out;
	}
	if((dp->d->attr & DDIR) && emptydir(f) < 0){
		chat("non-empty dir...");
		putfile(f);
		putsect(parp);
		errno = Eperm;
		goto out;
	}
	if(dp->paddr == 0 && (dp->d->attr&DRONLY)){
		chat("read-only file in root directory...");
		putfile(f);
		putsect(parp);
		errno = Eperm;
		goto out;
	}
	if(dp->paddr){
		puttime(pard, 0);
		parp->flags |= BMOD;
	}
	putsect(parp);
	if(truncfile(f, 1) < 0)
		errno = Eio;
	dp->d->name[0] = 0xe5;
	dp->p->flags |= BMOD;
	putfile(f);
out:
	xfile(thdr.fid, Clunk);
	sync();
}
void
rstat(void)
{
	Dir dir;
	Xfile *f=xfile(thdr.fid, Asis);
	Dosptr *dp = f->ptr;

	chat("stat(fid=%d)...", thdr.fid);
	if(!f || getfile(f)< 0)
		errno = Eio;
	else{
		getdir(&dir, dp->addr ? dp->d : 0);
		convD2M(&dir, rhdr.stat);
		putfile(f);
	}
}

static char isfrog[256]={
	/*NUL*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*BKS*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*DLE*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*CAN*/	1, 1, 1, 1, 1, 1, 1, 1,
	[' ']	1,
	['/']	1,
	[0x7f]	1,
};

static int
nameok(char *elem)
{
	char *eelem;

	eelem = elem+NAMELEN;
	while(*elem){
		if(isfrog[*(uchar*)elem])
			return -1;
		elem++;
		if(elem >= eelem)
			return -1;
	}
	return 0;
}

void
rwstat(void)
{
	errno = Eperm;
}
