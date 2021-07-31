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
	if(xf->fmt == 0 && dosfs(xf) < 0){
		errno = Eformat;
		goto error;
	}
	root->qid.path = CHDIR;
	root->qid.vers = 0;
	root->xf->rootqid = root->qid;
	dp = malloc(sizeof(Dosptr));
	if(dp == nil){
		errno = Enomem;
		goto error;
	}
	root->ptr = dp;
	rootfile(root);
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
	Xfile *of, *nf, *next;
	Dosptr *dp;

	chat("clone(fid=%d,newfid=%d)...", thdr.fid, thdr.newfid);
	of = xfile(thdr.fid, Asis);
	if(!of){
		errno = Eio;
		return;
	}
	nf = xfile(thdr.newfid, Clean);
	if(!nf){
		errno = Enomem;
		return;
	}
	dp = malloc(sizeof(Dosptr));
	if(dp == nil){
		errno = Enomem;
		return;
	}
	next = nf->next;
	*nf = *of;
	nf->next = next;
	nf->fid = thdr.newfid;
	nf->ptr = dp;
	refxfs(nf->xf, 1);
	memmove(dp, of->ptr, sizeof(Dosptr));
	dp->p = nil;
	dp->d = nil;
}

void
rwalk(void)
{
	Xfile *f;
	Dosptr dp[1];
	int r, islong;

	chat("walk(fid=%d,name=\"%s\")...", thdr.fid, thdr.name);
	f = xfile(thdr.fid, Asis);
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
		if(isroot(dp->addr))
			f->qid.path = f->xf->rootqid.path;
		else
			f->qid.path = CHDIR | QIDPATH(dp);
	}else{
		islong = fixname(thdr.name);
		if(islong < 0)
			goto error;
		if(getfile(f) < 0)
			goto error;

		/*
		 * always do a search for the long name,
		 * because it could be filed as such
		 */
		r = searchdir(f, thdr.name, dp, 0, islong);
		putfile(f);
		if(r < 0)
			goto error;
		memmove(f->ptr, dp, sizeof(Dosptr));
		f->qid.path = QIDPATH(dp);
		if(isroot(dp->addr))
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
	int attr, omode;

	chat("open(fid=%d,mode=%d)...", thdr.fid, thdr.mode);
	f = xfile(thdr.fid, Asis);
	if(!f || (f->flags&Omodes)){
		errno = Eio;
		return;
	}
	dp = f->ptr;
	omode = 0;
	if(!isroot(dp->paddr) && (thdr.mode & ORCLOSE)){
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
	if(!isroot(dp->addr))
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
		if(truncfile(f) < 0){
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

static int
uniquename(Xfile *f, Dosptr *ndp, char *name, char *sname)
{
	Dosptr tmpdp;
	int i, islong;

	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return -1;

	/*
	 * always do a search for the long name,
	 * because it could be filed as such
	 */
	islong = fixname(name);
	if(islong < 0 || searchdir(f, name, ndp, 1, islong) < 0)
		return -1;

	if(!islong)
		return 0;

	/*
	 * find alias for the long name
	 */
	for(i = 1; ; i++){
		mkalias(name, sname, i);
		if(searchdir(f, sname, &tmpdp, 0, 0) < 0)
			break;
		putsect(tmpdp.p);
	}

	return 1;
}

/*
 * fill in a directory entry for a new file
 */
static int
mkdentry(Xfs *xf, Dosptr *ndp, char *name, char *sname, int islong, int nattr, long start, long length)
{
	Dosdir *nd;

	/*
	 * fill in the entry
	 */
	ndp->p = getsect(xf, ndp->addr);
	if(ndp->p == nil
	|| islong && putlongname(xf, ndp, name, sname) < 0){
		errno = Eio;
		return -1;
	}

	ndp->d = (Dosdir *)&ndp->p->iobuf[ndp->offset];
	nd = ndp->d;
	memset(nd, 0, DOSDIRSIZE);

	if(islong)
		name = sname;
	putname(name, nd);

	nd->attr = nattr;
	puttime(nd, 0);
	putstart(xf, nd, start);
	nd->length[0] = length;
	nd->length[1] = length>>8;
	nd->length[2] = length>>16;
	nd->length[3] = length>>24;

	ndp->p->flags |= BMOD;

	return 0;
}

void
rcreate(void)
{
	Dosbpb *bp;
	Xfile *f;
	Dosptr *pdp, *ndp;
	Iosect *xp;
	Dosdir *pd, *xd;
	char sname[13];
	long start;
	int islong, attr, omode, nattr;

	chat("create(fid=%d,name=\"%s\",perm=%uo,mode=%d)...",
		thdr.fid, thdr.name, thdr.perm, thdr.mode);
	f = xfile(thdr.fid, Asis);
	if(!f || (f->flags&Omodes) || getfile(f)<0){
		errno = Eio;
		return;
	}
	pdp = f->ptr;
	pd = pdp->d;
	/*
	 * perm check
	 */
	if(isroot(pdp->addr) && pd != nil)
		panic("root pd != nil");
	attr = pd ? pd->attr : DDIR;
	if(!(attr & DDIR) || (attr & DRONLY)){
badperm:
		putfile(f);
		errno = Eperm;
		return;
	}
	omode = 0;
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

	/*
	 * check the name, find the slot for the dentry,
	 * and find a good alias for a long name
	 */
	ndp = malloc(sizeof(Dosptr));
	if(ndp == nil){
		putfile(f);
		errno = Enomem;
		return;
	}
	islong = uniquename(f, ndp, thdr.name, sname);
	if(islong < 0){
		free(ndp);
		goto badperm;
	}

	/*
	 * allocate first cluster, if making directory
	 */
	start = 0;
	bp = nil;
	if(thdr.perm & CHDIR){
		bp = f->xf->ptr;
		mlock(bp);
		start = falloc(f->xf);
		unmlock(bp);
		if(start <= 0){
			free(ndp);
			putfile(f);
			errno = Eio;
			return;
		}
	}

	/*
	 * make the entry
	 */
	nattr = 0;
	if((thdr.perm & 0222) == 0)
		nattr |= DRONLY;
	if(thdr.perm & CHDIR)
		nattr |= DDIR;

	if(mkdentry(f->xf, ndp, thdr.name, sname, islong, nattr, start, 0) < 0){
		if(ndp->p != nil)
			putsect(ndp->p);
		free(ndp);
		if(start > 0)
			ffree(f->xf, start);
		putfile(f);
		return;
	}

	if(pd != nil){
		puttime(pd, 0);
		pdp->p->flags |= BMOD;
	}

	/*
	 * fix up the fid
	 */
	f->ptr = ndp;
	f->qid.path = QIDPATH(ndp);

	if(thdr.perm & CHDIR){
		f->qid.path |= CHDIR;
		xp = getsect(f->xf, clust2sect(bp, start));
		if(xp == nil){
			errno = Eio;
			goto badio;
		}
		xd = (Dosdir *)&xp->iobuf[0];
		memmove(xd, ndp->d, DOSDIRSIZE);
		memset(xd->name, ' ', sizeof xd->name+sizeof xd->ext);
		xd->name[0] = '.';
		xd = (Dosdir *)&xp->iobuf[DOSDIRSIZE];
		if(pd)
			memmove(xd, pd, DOSDIRSIZE);
		else{
			memset(xd, 0, DOSDIRSIZE);
			puttime(xd, 0);
			xd->attr = DDIR;
		}
		memset(xd->name, ' ', sizeof xd->name+sizeof xd->ext);
		xd->name[0] = '.';
		xd->name[1] = '.';
		xp->flags |= BMOD;
		putsect(xp);
	}

	f->flags |= omode;
	chat("f->qid=0x%8.8lux...", f->qid.path);
	rhdr.qid = f->qid;

badio:
	putfile(f);
	putsect(pdp->p);
	free(pdp);
}

void
rread(void)
{
	Xfile *f;
	int r;
	long offset;

	chat("read(fid=%d,offset=%lld,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	if (!(f=xfile(thdr.fid, Asis)) || !(f->flags&Oread))
		goto error;
	if(f->qid.path & CHDIR){
		thdr.count = (thdr.count/DIRLEN)*DIRLEN;
		offset = thdr.offset;	/* cast vlong to long */
		if(thdr.count<DIRLEN || offset%DIRLEN){
			chat("count=%d,offset=%lld,DIRLEN=%d...",
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
	Xfile *f;
	int r;

	chat("write(fid=%d,offset=%lld,count=%d)...",
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

/*
 * wipe out a dos directory entry
 */
static void
doremove(Xfs *xf, Dosptr *dp)
{
	Iosect *p;
	int prevdo;

	dp->p->iobuf[dp->offset] = DOSEMPTY;
	dp->p->flags |= BMOD;
	for(prevdo = dp->offset-DOSDIRSIZE; prevdo >= 0; prevdo -= DOSDIRSIZE){
		if(dp->p->iobuf[prevdo+11] != 0xf)
			break;
		dp->p->iobuf[prevdo] = DOSEMPTY;
	}
	if(prevdo < 0 && dp->prevaddr != -1){
		p = getsect(xf, dp->prevaddr);
		for(prevdo = ((Dosbpb*)xf->ptr)->sectsize-DOSDIRSIZE; prevdo >= 0; prevdo -= DOSDIRSIZE){
			if(p->iobuf[prevdo+11] != 0xf)
				break;
			p->iobuf[prevdo] = DOSEMPTY;
			p->flags |= BMOD;
		}
		putsect(p);
	}		
}

void
rremove(void)
{
	Xfile *f;
	Dosptr *dp;
	Iosect *parp;
	Dosdir *pard;

	chat("remove(fid=%d,name=\"%s\")...", thdr.fid, thdr.name);
	f = xfile(thdr.fid, Asis);
	if(!f){
		errno = Eio;
		goto out;
	}
	dp = f->ptr;
	if(isroot(dp->addr)){
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
	if(!isroot(dp->paddr) && (pard->attr & DRONLY)){
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
	if(isroot(dp->paddr) && (dp->d->attr&DRONLY)){
		chat("read-only file in root directory...");
		putfile(f);
		putsect(parp);
		errno = Eperm;
		goto out;
	}
	doremove(f->xf, f->ptr);
	if(!isroot(dp->paddr)){
		puttime(pard, 0);
		parp->flags |= BMOD;
	}
	putsect(parp);
	if(truncfile(f) < 0)
		errno = Eio;
	putfile(f);
out:
	xfile(thdr.fid, Clunk);
	sync();
}

static void
dostat(Xfile *f, Dir *d)
{
	Dosptr *dp;
	Iosect *p;
	char *name, namebuf[DOSNAMELEN];
	int islong, sum, prevdo;

	dp = f->ptr;
	if(isroot(dp->addr)){
		memset(d, 0, sizeof(Dir));
		d->name[0] = '/';
		d->qid.path = CHDIR;
		d->mode = CHDIR|0777;
		strcpy(d->uid, "bill");
		strcpy(d->gid, "trog");
	}else{
		/*
		 * assemble any long file name
		 */
		sum = aliassum(dp->d);
		islong = 0;
		name = namebuf;
		for(prevdo = dp->offset-DOSDIRSIZE; prevdo >= 0; prevdo -= DOSDIRSIZE){
			if(dp->p->iobuf[prevdo+11] != 0xf)
				break;
			name = getnamesect(namebuf, name, &dp->p->iobuf[prevdo], &islong, &sum, -1);
		}
		if(prevdo < 0 && dp->prevaddr != -1){
			p = getsect(f->xf, dp->prevaddr);
			for(prevdo = ((Dosbpb*)f->xf->ptr)->sectsize-DOSDIRSIZE; prevdo >= 0; prevdo -= DOSDIRSIZE){
				if(p->iobuf[prevdo+11] != 0xf)
					break;
				name = getnamesect(namebuf, name, &p->iobuf[prevdo], &islong, &sum, -1);
			}
			putsect(p);
		}
		getdir(f->xf, d, dp->d, dp->addr, dp->offset);
		if(islong && sum == -1 && nameok(namebuf))
			strncpy(d->name, namebuf, NAMELEN);
	}
}

void
rstat(void)
{
	Dir dir;
	Xfile *f;

	chat("stat(fid=%d)...", thdr.fid);

	f = xfile(thdr.fid, Asis);
	if(!f || getfile(f) < 0){
		errno = Eio;
		return;
	}

	dostat(f, &dir);

	convD2M(&dir, rhdr.stat);
	putfile(f);
}

void
rwstat(void)
{
	Dir dir, wdir;
	Xfile *f, pf;
	Dosptr *dp, ndp, pdp;
	Iosect *parp;
	Dosdir *pard, *d, od;
	char sname[13];
	ulong oaddr, ooffset;
	long start, length;
	int i, islong, changes, attr;

	chat("wstat(fid=%d)...", thdr.fid);

	f = xfile(thdr.fid, Asis);
	if(!f || getfile(f) < 0){
		errno = Eio;
		return;
	}
	dp = f->ptr;

	if(isroot(dp->addr)){
		errno = Eperm;
		goto out;
	}

	changes = 0;
	dostat(f, &dir);
	convM2D(thdr.stat, &wdir);

	/*
	 * no chown or chgrp
	 */
	if(strncmp(dir.uid, wdir.uid, NAMELEN) != 0
	|| strncmp(dir.gid, wdir.gid, NAMELEN) != 0){
		errno = Eperm;
		goto out;
	}

	/*
	 * mode/mtime allowed
	 */
	if(dir.mtime != wdir.mtime || ((dir.mode^wdir.mode) & (CHEXCL|0777)))
		changes = 1;

	/*
	 * Setting CHAPPEND (make system file contiguous)
	 * requires setting CHEXCL (system file).
	 */
	if((dir.mode^wdir.mode) & CHAPPEND) {
		if((wdir.mode & (CHEXCL|CHAPPEND)) == CHAPPEND) {
			errno = Eperm;
			goto out;
		}
		if((wdir.mode & CHAPPEND) && makecontig(f, 0) < 0) {
			errno = Econtig;
			goto out;
		}
	}

	if((wdir.mode & 7) != ((wdir.mode >> 3) & 7)
	|| (wdir.mode & 7) != ((wdir.mode >> 6) & 7)){
		errno = Eperm;
		goto out;
	}

	/*
	 * to rename:
	 *	1) make up a fake clone
	 *	2) walk to parent
	 *	3) remove the old entry
	 *	4) create entry with new name
	 *	5) write correct mode/mtime info
	 * we need to remove the old entry before creating the new one
	 * to avoid a lock loop.
	 */
	if(strncmp(dir.name, wdir.name, NAMELEN) != 0){
		/*
		 * grab parent directory of file to be changed and check for write perm
		 * rename also disallowed for read-only files in root directory
		 */
		parp = getsect(f->xf, dp->paddr);
		if(parp == nil){
			errno = Eio;
			goto out;
		}
		pard = (Dosdir *)&parp->iobuf[dp->poffset];
		if(!isroot(dp->paddr) && (pard->attr & DRONLY)
		|| isroot(dp->paddr) && (dp->d->attr&DRONLY)){
			putsect(parp);
			errno = Eperm;
			goto out;
		}

		/*
		 * retrieve info from old entry
		 */
		oaddr = dp->addr;
		ooffset = dp->offset;
		d = dp->d;
		od = *d;
		start = getstart(f->xf, d);
		length = GLONG(d->length);
		attr = d->attr;

		/*
		 * temporarily release file to allow other directory ops:
		 * walk to parent, validate new name
		 * then remove old entry
		 */
		putfile(f);
		pf = *f;
		memset(&pdp, 0, sizeof(Dosptr));
		pdp.prevaddr = -1;
		pdp.naddr = -1;
		pdp.addr = dp->paddr;
		pdp.offset = dp->poffset;
		pdp.p = parp;
		if(!isroot(pdp.addr))
			pdp.d = (Dosdir *)&parp->iobuf[pdp.offset];
		pf.ptr = &pdp;
		islong = uniquename(&pf, &ndp, wdir.name, sname);
		if(islong < 0){
			putsect(parp);
			errno = Eperm;
			return;
		}
		if(getfile(f) < 0){
			putsect(parp);
			errno = Eio;
			return;
		}
		doremove(f->xf, dp);
		putfile(f);

		/*
		 * search for dir entry again, since we may be able to use the old slot,
		 * and we need to set up the naddr field if a long name spans the block.
		 * create new entry.
		 */
		if(searchdir(&pf, wdir.name, dp, 1, islong) < 0
		|| mkdentry(pf.xf, dp, wdir.name, sname, islong, attr, start, length) < 0){
			putsect(parp);
			errno = Eio;
			goto out;
		}

		/*
		 * copy invisible fields
		 */
		d = dp->d;
		for(i = 0; i < 2; i++)
			d->ctime[i] = od.ctime[i];
		for(i = 0; i < nelem(od.cdate); i++)
			d->cdate[i] = od.cdate[i];
		for(i = 0; i < nelem(od.adate); i++)
			d->adate[i] = od.adate[i];

		putsect(parp);

		/*
		 * relocate up other fids to the same file, if it moved
		 */
		f->qid.path = (f->qid.path & CHDIR) | QIDPATH(dp);
		if(oaddr != dp->addr || ooffset != dp->offset)
			dosptrreloc(f, dp, oaddr, ooffset);
		changes = 1;
	}

	if(changes){
		putdir(dp->d, &wdir);
		dp->p->flags |= BMOD;
	}

out:
	putfile(f);
	sync();
}
