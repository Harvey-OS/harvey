#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "iotrack.h"
#include "dat.h"
#include "dosfs.h"
#include "fns.h"

void
rversion(void)
{
	if(req->msize > Maxiosize)
		rep->msize = Maxiosize;
	else
		rep->msize = req->msize;
	rep->version = "9P2000";
}

void
rauth(void)
{
	errno = Enoauth;
}

void
rflush(void)
{
}

void
rattach(void)
{
	Xfs *xf;
	Xfile *root;
	Dosptr *dp;

	root = xfile(req->fid, Clean);
	if(!root){
		errno = Enomem;
		goto error;
	}
	root->xf = xf = getxfs(req->uname, req->aname);
	if(!xf)
		goto error;
	if(xf->fmt == 0 && dosfs(xf) < 0){
		errno = Eformat;
		goto error;
	}
	root->qid.type = QTDIR;
	root->qid.path = 0;
	root->qid.vers = 0;
	root->xf->rootqid = root->qid;
	dp = malloc(sizeof(Dosptr));
	if(dp == nil){
		errno = Enomem;
		goto error;
	}
	root->ptr = dp;
	rootfile(root);
	rep->qid = root->qid;
	return;
error:
	if(root)
		xfile(req->fid, Clunk);
}

Xfile*
doclone(Xfile *of, int newfid)
{
	Xfile *nf, *next;
	Dosptr *dp;

	nf = xfile(newfid, Clean);
	if(!nf){
		errno = Enomem;
		return nil;
	}
	dp = malloc(sizeof(Dosptr));
	if(dp == nil){
		errno = Enomem;
		return nil;
	}
	next = nf->next;
	*nf = *of;
	nf->next = next;
	nf->fid = req->newfid;
	nf->ptr = dp;
	refxfs(nf->xf, 1);
	memmove(dp, of->ptr, sizeof(Dosptr));
	dp->p = nil;
	dp->d = nil;
	return nf;
}

void
rwalk(void)
{
	Xfile *f, *nf;
	Dosptr dp[1], savedp[1];
	int r, longtype;
	Qid saveqid;

	rep->nwqid = 0;
	nf = nil;
	f = xfile(req->fid, Asis);
	if(f == nil){
		chat("\tno xfile\n");
		goto error2;
	}
	if(req->fid != req->newfid){
		nf = doclone(f, req->newfid);
		if(nf == nil){
			chat("\tclone failed\n");
			goto error2;
		}
		f = nf;
	}

	saveqid = f->qid;
	memmove(savedp, f->ptr, sizeof(Dosptr));
	for(; rep->nwqid < req->nwname && rep->nwqid < MAXWELEM; rep->nwqid++){
		chat("\twalking %s\n", req->wname[rep->nwqid]);
		if(!(f->qid.type & QTDIR)){
			chat("\tnot dir: type=%#x\n", f->qid.type);
			goto error;
		}
		if(strcmp(req->wname[rep->nwqid], ".") == 0){
			;
		}else if(strcmp(req->wname[rep->nwqid], "..") == 0){
			if(f->qid.path != f->xf->rootqid.path){
				r = walkup(f, dp);
				if(r < 0)
					goto error;
				memmove(f->ptr, dp, sizeof(Dosptr));
				if(isroot(dp->addr))
					f->qid.path = f->xf->rootqid.path;
				else
					f->qid.path = QIDPATH(dp);
			}
		}else{
			fixname(req->wname[rep->nwqid]);
			longtype = classifyname(req->wname[rep->nwqid]);
			if(longtype==Invalid || getfile(f) < 0)
				goto error;

			/*
			 * always do a search for the long name,
			 * because it could be filed as such
			 */
			r = searchdir(f, req->wname[rep->nwqid], dp, 0, longtype);
			putfile(f);
			if(r < 0)
				goto error;
			memmove(f->ptr, dp, sizeof(Dosptr));
			f->qid.path = QIDPATH(dp);
			f->qid.type = QTFILE;
			if(isroot(dp->addr))
				f->qid.path = f->xf->rootqid.path;
			else if(dp->d->attr & DDIR)
				f->qid.type = QTDIR;
			else if(dp->d->attr & DSYSTEM){
				f->qid.type |= QTEXCL;
				if(iscontig(f->xf, dp->d))
					f->qid.type |= QTAPPEND;
			}
//ZZZ maybe use other bits than qtexcl & qtapppend
			putfile(f);
		}
		rep->wqid[rep->nwqid] = f->qid;
	}
	return;
error:
	f->qid = saveqid;
	memmove(f->ptr, savedp, sizeof(Dosptr));
	if(nf != nil)
		xfile(req->newfid, Clunk);
error2:
	if(!errno && !rep->nwqid)
		errno = Enonexist;
}

void
ropen(void)
{
	Xfile *f;
	Iosect *p;
	Dosptr *dp;
	int attr, omode;

	f = xfile(req->fid, Asis);
	if(!f || (f->flags&Omodes)){
		errno = Eio;
		return;
	}
	dp = f->ptr;
	omode = 0;
	if(!isroot(dp->paddr) && (req->mode & ORCLOSE)){
		/*
		 * check on parent directory of file to be deleted
		 */
		p = getsect(f->xf, dp->paddr);
		if(p == nil){
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
	}else if(req->mode & ORCLOSE)
		omode |= Orclose;
	if(getfile(f) < 0){
		errno = Enonexist;
		return;
	}
	if(!isroot(dp->addr))
		attr = dp->d->attr;
	else
		attr = DDIR;
	switch(req->mode & 7){
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
	if(req->mode & OTRUNC){
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
	rep->qid = f->qid;
	rep->iounit = 0;
out:
	putfile(f);
}

static int
mk8dot3name(Xfile *f, Dosptr *ndp, char *name, char *sname)
{
	Dosptr tmpdp;
	int i, longtype;

	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return Invalid;

	/*
	 * always do a search for the long name,
	 * because it could be filed as such
	 */
	fixname(name);
	longtype = classifyname(name);
	if(longtype==Invalid || searchdir(f, name, ndp, 1, longtype) < 0)
		return Invalid;

	if(longtype==Short)
		return Short;

	if(longtype==ShortLower){
		/*
		 * alias is the upper-case version, which we
		 * already know does not exist.
		 */
		strcpy(sname, name);
		for(i=0; sname[i]; i++)
			if('a' <= sname[i] && sname[i] <= 'z')
				sname[i] += 'A'-'a';
		return ShortLower;
	}

	/*
	 * find alias for the long name
	 */
	for(i=1;; i++){
		mkalias(name, sname, i);
		if(searchdir(f, sname, &tmpdp, 0, 0) < 0)
			return Long;
		putsect(tmpdp.p);
	}
}

/*
 * fill in a directory entry for a new file
 */
static int
mkdentry(Xfs *xf, Dosptr *ndp, char *name, char *sname, int longtype, int nattr, long start, long length)
{
	Dosdir *nd;

	/*
	 * fill in the entry
	 */
	ndp->p = getsect(xf, ndp->addr);
	if(ndp->p == nil
	|| longtype!=Short && putlongname(xf, ndp, name, sname) < 0){
		errno = Eio;
		return -1;
	}

	ndp->d = (Dosdir *)&ndp->p->iobuf[ndp->offset];
	nd = ndp->d;
	memset(nd, 0, DOSDIRSIZE);

	if(longtype!=Short)
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
	int longtype, attr, omode, nattr;

	f = xfile(req->fid, Asis);
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
	if(req->mode & ORCLOSE)
		omode |= Orclose;
	switch(req->mode & 7){
	case OREAD:
	case OEXEC:
		omode |= Oread;
		break;
	case ORDWR:
		omode |= Oread;
		/* fall through */
	case OWRITE:
		omode |= Owrite;
		if(req->perm & DMDIR)
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
	longtype = mk8dot3name(f, ndp, req->name, sname);
	chat("rcreate %s longtype %d...\n", req->name, longtype);
	if(longtype == Invalid){
		free(ndp);
		goto badperm;
	}

	/*
	 * allocate first cluster, if making directory
	 */
	start = 0;
	bp = nil;
	if(req->perm & DMDIR){
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
	if((req->perm & 0222) == 0)
		nattr |= DRONLY;
	if(req->perm & DMDIR)
		nattr |= DDIR;

	if(mkdentry(f->xf, ndp, req->name, sname, longtype, nattr, start, 0) < 0){
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
	f->qid.type = QTFILE;
	f->qid.path = QIDPATH(ndp);

//ZZZ set type for excl, append?
	if(req->perm & DMDIR){
		f->qid.type = QTDIR;
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
	rep->qid = f->qid;
	rep->iounit = 0;

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

	if (!(f=xfile(req->fid, Asis)) || !(f->flags&Oread))
		goto error;
	if(req->count > sizeof repdata)
		req->count = sizeof repdata;
	if(f->qid.type & QTDIR){
		if(getfile(f) < 0)
			goto error;
		r = readdir(f, repdata, req->offset, req->count);
	}else{
		if(getfile(f) < 0)
			goto error;
		r = readfile(f, repdata, req->offset, req->count);
	}
	putfile(f);
	if(r < 0){
error:
		errno = Eio;
	}else{
		rep->count = r;
		rep->data = (char*)repdata;
	}
}

void
rwrite(void)
{
	Xfile *f;
	int r;

	if (!(f=xfile(req->fid, Asis)) || !(f->flags&Owrite))
		goto error;
	if(getfile(f) < 0)
		goto error;
	r = writefile(f, req->data, req->offset, req->count);
	putfile(f);
	if(r < 0){
error:
		errno = Eio;
	}else{
		rep->count = r;
	}
}

void
rclunk(void)
{
	xfile(req->fid, Clunk);
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

	f = xfile(req->fid, Asis);
	parp = nil;
	if(f == nil){
		errno = Eio;
		goto out;
	}
	dp = f->ptr;
	if(isroot(dp->addr)){
		errno = Eperm;
		goto out;
	}

	/*
	 * can't remove if parent is read only,
	 * it's a non-empty directory,
	 * or it's a read only file in the root directory
	 */
	parp = getsect(f->xf, dp->paddr);
	if(parp == nil
	|| getfile(f) < 0){
		errno = Eio;
		goto out;
	}
	pard = (Dosdir *)&parp->iobuf[dp->poffset];
	if(!isroot(dp->paddr) && (pard->attr & DRONLY)
	|| (dp->d->attr & DDIR) && emptydir(f) < 0
	|| isroot(dp->paddr) && (dp->d->attr&DRONLY)){
		errno = Eperm;
		goto out;
	}
	if(truncfile(f, 0) < 0){
		errno = Eio;
		goto out;
	}
	doremove(f->xf, f->ptr);
	if(!isroot(dp->paddr)){
		puttime(pard, 0);
		parp->flags |= BMOD;
	}
out:
	if(parp != nil)
		putsect(parp);
	if(f != nil)
		putfile(f);
	xfile(req->fid, Clunk);
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
		d->name = "/";
		d->qid.type = QTDIR;
		d->qid.path = f->xf->rootqid.path;
		d->mode = DMDIR|0777;
		d->uid = "bill";
		d->muid = "bill";
		d->gid = "trog";
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
			strcpy(d->name, namebuf);
	}
}

void
rstat(void)
{
	Dir dir;
	Xfile *f;

	f = xfile(req->fid, Asis);
	if(!f || getfile(f) < 0){
		errno = Eio;
		return;
	}

	dir.name = repdata;
	dostat(f, &dir);

	rep->nstat = convD2M(&dir, statbuf, sizeof statbuf);
	rep->stat = statbuf;
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
	int i, longtype, changes, attr;

	f = xfile(req->fid, Asis);
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
	dir.name = repdata;
	dostat(f, &dir);
	if(convM2D(req->stat, req->nstat, &wdir, (char*)statbuf) != req->nstat){
		errno = Ebadstat;
		goto out;
	}

	/*
	 * To change length, must have write permission on file.
	 * we only allow truncates for now.
	 */
	if(wdir.length!=~0 && wdir.length!=dir.length){
		if(wdir.length > dir.length || !dir.mode & 0222){
			errno = Eperm;
			goto out;
		}
	}

	/*
	 * no chown or chgrp
	 */
	if(wdir.uid[0] != '\0' && strcmp(dir.uid, wdir.uid) != 0
	|| wdir.gid[0] != '\0' && strcmp(dir.gid, wdir.gid) != 0){
		errno = Eperm;
		goto out;
	}

	/*
	 * mode/mtime allowed
	 */
	if(wdir.mtime != ~0 && dir.mtime != wdir.mtime)
		changes = 1;

	/*
	 * Setting DMAPPEND (make system file contiguous)
	 * requires setting DMEXCL (system file).
	 */
	if(wdir.mode != ~0){
		if((wdir.mode & 7) != ((wdir.mode >> 3) & 7)
		|| (wdir.mode & 7) != ((wdir.mode >> 6) & 7)){
			errno = Eperm;
			goto out;
		}
		if((dir.mode^wdir.mode) & (DMEXCL|DMAPPEND|0777))
			changes = 1;
		if((dir.mode^wdir.mode) & DMAPPEND) {
			if((wdir.mode & (DMEXCL|DMAPPEND)) == DMAPPEND) {
				errno = Eperm;
				goto out;
			}
			if((wdir.mode & DMAPPEND) && makecontig(f, 0) < 0) {
				errno = Econtig;
				goto out;
			}
		}
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
	if(wdir.name[0] != '\0' && strcmp(dir.name, wdir.name) != 0){
		if(utflen(wdir.name) >= DOSNAMELEN){
			errno = Etoolong;
			goto out;
		}

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
		longtype = mk8dot3name(&pf, &ndp, wdir.name, sname);
		if(longtype==Invalid){
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
		if(searchdir(&pf, wdir.name, dp, 1, longtype) < 0
		|| mkdentry(pf.xf, dp, wdir.name, sname, longtype, attr, start, length) < 0){
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
		f->qid.path = QIDPATH(dp);
		if(oaddr != dp->addr || ooffset != dp->offset)
			dosptrreloc(f, dp, oaddr, ooffset);

		/*
		 * copy fields that are not supposed to change
		 */
		if(wdir.mtime == ~0)
			wdir.mtime = dir.mtime;
		if(wdir.mode == ~0)
			wdir.mode = dir.mode;
		changes = 1;
	}

	/*
	 * do the actual truncate
	 */
	if(wdir.length != ~0 && wdir.length != dir.length && truncfile(f, wdir.length) < 0)
		errno = Eio;

	if(changes){
		putdir(dp->d, &wdir);
		dp->p->flags |= BMOD;
	}

out:
	putfile(f);
	sync();
}
