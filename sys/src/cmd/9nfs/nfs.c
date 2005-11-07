#include "all.h"

extern uchar buf[];

Xfid *
rpc2xfid(Rpccall *cmd, Dir *dp)
{
	char *argptr = cmd->args;
	Xfile *xp;
	Xfid *xf;
	Session *s;
	char *service;
	Authunix au;
	Qid qid;
	char client[256], *user;
	Unixidmap *m;
	int i;
	uvlong x1, x2;

	chat("rpc2xfid %.8lux %.8lux %p %p\n", *((ulong*)argptr), *((ulong*)argptr+1), buf, argptr);
	if(argptr[0] == 0 && argptr[1] == 0){	/* root */
		chat("root...");
		xp = xfroot(&argptr[2], 0);
		s = xp ? xp->s : 0;
	}else{
		ulong ul;
		chat("noroot %.8lux...", *((ulong*)argptr));
		if((ul=GLONG()) != starttime){
			chat("bad tag %lux %lux...", ul, starttime);
			return 0;
		}
		s = (Session *)GLONG();
		x1 = GLONG();
		x2 = GLONG();
		qid.path = x1 | (x2<<32);
		qid.vers = 0;
		qid.type = GBYTE();
		xp = xfile(&qid, s, 0);
	}
	if(xp == 0){
		chat("no xfile...");
		return 0;
	}
	if(auth2unix(&cmd->cred, &au) != 0){
		chat("auth flavor=%ld, count=%ld\n",
			cmd->cred.flavor, cmd->cred.count);
		for(i=0; i<cmd->cred.count; i++)
			chat(" %.2ux", ((uchar *)cmd->cred.data)[i]);
		chat("...");
		return 0;
	}else{
/*		chat("auth: %d %.*s u=%d g=%d",
 *			au.stamp, utfnlen(au.mach.s, au.mach.n), au.mach.s, au.uid, au.gid);
 *		for(i=0; i<au.gidlen; i++)
 *			chat(", %d", au.gids[i]);
 *		chat("...");
 */
		char *p = memchr(au.mach.s, '.', au.mach.n);
		chat("%ld@%.*s...", au.uid, utfnlen(au.mach.s, (p ? p-au.mach.s : au.mach.n)), au.mach.s);
	}
	if(au.mach.n >= sizeof client){
		chat("client name too long...");
		return 0;
	}
	memcpy(client, au.mach.s, au.mach.n);
	client[au.mach.n] = 0;
	service = xp->parent->s->service;
	cmd->up = m = pair2idmap(service, cmd->host);
	if(m == 0){
		chat("no map for pair (%s,%s)...", service, client);
		/*chat("getdom %d.%d.%d.%d", cmd->host&0xFF, (cmd->host>>8)&0xFF,
			(cmd->host>>16)&0xFF, (cmd->host>>24)&0xFF);/**/
		/*if(getdom(cmd->host, client, sizeof(client))<0)
			return 0;/**/
		return 0;
	}
	/*chat("map=(%s,%s)...", m->server, m->client);/**/
	cmd->user = user = id2name(&m->u.ids, au.uid);
	if(user == 0){
		chat("no user for id %ld...", au.uid);
		return 0;
	}
	chat("user=%s...", user);/**/
	xf = 0;
	if(s == xp->parent->s){
		if(!s->noauth)
			xf = setuser(xp, user);
		if(xf == 0)
			xf = setuser(xp, "none");
		if(xf == 0)
			chat("can't set user none...");
	}else
		xf = xp->users;
	if(xf)
		chat("uid=%s...", xf->uid);
	if(xf && dp && xfstat(xf, dp) < 0){
		chat("can't stat %s...", xp->name);
		return 0;
	}
	return xf;
}

Xfid *
setuser(Xfile *xp, char *user)
{
	Xfid *xf, *xpf;
	Session *s;

	xf = xfid(user, xp, 1);
	if(xf->urfid)
		return xf;
	if(xp->parent==xp || !(xpf = setuser(xp->parent, user))) /* assign = */
		return xfid(user, xp, -1);
	s = xp->s;
	xf->urfid = newfid(s);
	xf->urfid->owner = &xf->urfid;
	setfid(s, xpf->urfid);
	s->f.newfid = xf->urfid - s->fids;
	s->f.nwname = 1;
	s->f.wname[0] = xp->name;
	if(xmesg(s, Twalk) || s->f.nwqid != 1)
		return xfid(user, xp, -1);
	return xf;
}

int
xfstat(Xfid *xf, Dir *dp)
{
	Xfile *xp;
	Session *s;
	char buf[128];

	xp = xf->xp;
	s = xp->s;
	if(s != xp->parent->s){
		seprint(buf, buf+sizeof buf, "#%s", xf->uid);
		dp->name = strstore(buf);
		dp->uid = xf->uid;
		dp->gid = xf->uid;
		dp->muid = xf->uid;
		dp->qid.path = (uvlong)xf->uid;
		dp->qid.type = QTFILE;
		dp->qid.vers = 0;
		dp->mode = 0666;
		dp->atime = time(0);
		dp->mtime = dp->atime;
		dp->length = NETCHLEN;
		dp->type = 0;
		dp->type = 0;
		return 0;
	}
	setfid(s, xf->urfid);
	if(xmesg(s, Tstat) == 0){
		convM2D(s->f.stat, s->f.nstat, dp, (char*)s->statbuf);
		if(xp->qid.path == dp->qid.path){
			xp->name = strstore(dp->name);
			return 0;
		}
		/* not reached ? */
		chat("xp->qid.path=0x%.16llux, dp->qid.path=0x%.16llux name=%s...",
			xp->qid.path, dp->qid.path, dp->name);
	}
	if(xp != xp->parent)
		xpclear(xp);
	else
		clog("can't stat root: %s",
			s->f.type == Rerror ? s->f.ename : "??");
	return -1;
}

int
xfwstat(Xfid *xf, Dir *dp)
{
	Xfile *xp;
	Session *s;

	xp = xf->xp;
	s = xp->s;

	/*
	 * xf->urfid can be zero because some DOS NFS clients
	 * try to do wstat on the #user authentication files on close.
	 */
	if(s == 0 || xf->urfid == 0)
		return -1;
	setfid(s, xf->urfid);
	s->f.stat = s->statbuf;
	convD2M(dp, s->f.stat, Maxstatdata);
	if(xmesg(s, Twstat))
		return -1;
	xp->name = strstore(dp->name);
	return 0;
}

int
xfopen(Xfid *xf, int flag)
{
	static int modes[] = {
		[Oread] OREAD, [Owrite] OWRITE, [Oread|Owrite] ORDWR,
	};
	Xfile *xp;
	Session *s;
	Fid *opfid;
	int omode;

	if(xf->opfid && (xf->mode & flag & Open) == flag)
		return 0;
	omode = modes[(xf->mode|flag) & Open];
	if(flag & Trunc)
		omode |= OTRUNC;
	xp = xf->xp;
	chat("open(\"%s\", %d)...", xp->name, omode);
	s = xp->s;
	opfid = newfid(s);
	setfid(s, xf->urfid);
	s->f.newfid = opfid - s->fids;
	s->f.nwname = 0;
	if(xmesg(s, Twalk)){
		putfid(s, opfid);
		return -1;
	}
	setfid(s, opfid);
	s->f.mode = omode;
	if(xmesg(s, Topen)){
		clunkfid(s, opfid);
		return -1;
	}
	if(xf->opfid)
		clunkfid(s, xf->opfid);
	xf->mode |= flag & Open;
	xf->opfid = opfid;
	opfid->owner = &xf->opfid;
	xf->offset = 0;
	return 0;
}

void
xfclose(Xfid *xf)
{
	Xfile *xp;

	if(xf->mode & Open){
		xp = xf->xp;
		chat("close(\"%s\")...", xp->name);
		if(xf->opfid)
			clunkfid(xp->s, xf->opfid);
		xf->mode &= ~Open;
		xf->opfid = 0;
	}
}

void
xfclear(Xfid *xf)
{
	Xfile *xp = xf->xp;

	if(xf->opfid){
		clunkfid(xp->s, xf->opfid);
		xf->opfid = 0;
	}
	if(xf->urfid){
		clunkfid(xp->s, xf->urfid);
		xf->urfid = 0;
	}
	xfid(xf->uid, xp, -1);
}

Xfid *
xfwalkcr(int type, Xfid *xf, String *elem, long perm)
{
	Session *s;
	Xfile *xp, *newxp;
	Xfid *newxf;
	Fid *nfid;

	chat("xf%s(\"%s\")...", type==Tcreate ? "create" : "walk", elem->s);
	xp = xf->xp;
	s = xp->s;
	nfid = newfid(s);
	setfid(s, xf->urfid);
	s->f.newfid = nfid - s->fids;
	if(type == Tcreate){
		s->f.nwname = 0;
		if(xmesg(s, Twalk)){
			putfid(s, nfid);
			return 0;
		}
		s->f.fid = nfid - s->fids;
	}
	if(type == Tcreate){
		s->f.name = elem->s;
		s->f.perm = perm;
		s->f.mode = (perm&DMDIR) ? OREAD : ORDWR;
		if(xmesg(s, type)){
			clunkfid(s, nfid);
			return 0;
		}
	}else{	/* Twalk */
		s->f.nwname = 1;
		s->f.wname[0] = elem->s;
		if(xmesg(s, type) || s->f.nwqid!=1){
			putfid(s, nfid);
			return 0;
		}
		s->f.qid = s->f.wqid[0];	/* only one element */
	}
	chat("fid=%d,qid=0x%llux,%ld,%.2ux...", s->f.fid, s->f.qid.path, s->f.qid.vers, s->f.qid.type);
	newxp = xfile(&s->f.qid, s, 1);
	if(newxp->parent == 0){
		chat("new xfile...");
		newxp->parent = xp;
		newxp->sib = xp->child;
		xp->child = newxp;
	}
	newxf = xfid(xf->uid, newxp, 1);
	if(type == Tcreate){
		newxf->mode = (perm&DMDIR) ? Oread : (Oread|Owrite);
		newxf->opfid = nfid;
		nfid->owner = &newxf->opfid;
		nfid = newfid(s);
		setfid(s, xf->urfid);
		s->f.newfid = nfid - s->fids;
		s->f.nwname = 1;
		s->f.wname[0] = elem->s;
		if(xmesg(s, Twalk) || s->f.nwqid!=1){
			putfid(s, nfid);
			xpclear(newxp);
			return 0;
		}
		newxf->urfid = nfid;
		nfid->owner = &newxf->urfid;
	}else if(newxf->urfid){
		chat("old xfid %ld...", newxf->urfid-s->fids);
		clunkfid(s, nfid);
	}else{
		newxf->urfid = nfid;
		nfid->owner = &newxf->urfid;
	}
	newxp->name = strstore(elem->s);
	return newxf;
}

void
xpclear(Xfile *xp)
{
	Session *s;
	Xfid *xf;
	Xfile *xnp;

	s = xp->s;
	while(xf = xp->users)	/* assign = */
		xfclear(xf);
	while(xnp = xp->child){	/* assign = */
		xp->child = xnp->sib;
		xnp->parent = 0;
		xpclear(xnp);
		xfile(&xnp->qid, s, -1);
	}
	if(xnp = xp->parent){	/* assign = */
		if(xnp->child == xp)
			xnp->child = xp->sib;
		else{
			xnp = xnp->child;
			while(xnp->sib != xp)
				xnp = xnp->sib;
			xnp->sib = xp->sib;
		}
		xfile(&xp->qid, s, -1);
	}
}

int
xp2fhandle(Xfile *xp, Fhandle fh)
{
	uchar *dataptr = fh;
	ulong x;
	int n;

	memset(fh, 0, FHSIZE);
	if(xp == xp->parent){	/* root */
		dataptr[0] = 0;
		dataptr[1] = 0;
		n = strlen(xp->s->service);
		if(n > FHSIZE-3)
			n = FHSIZE-3;
		memmove(&dataptr[2], xp->s->service, n);
		dataptr[2+n] = 0;
	}else{
		PLONG(starttime);
		PLONG((u32int)(uintptr)xp->s);
		x = xp->qid.path;
		PLONG(x);
		x = xp->qid.path>>32;
		PLONG(x);
		PBYTE(xp->qid.type);
		USED(dataptr);
	}
	return FHSIZE;
}

int
dir2fattr(Unixidmap *up, Dir *dp, void *mp)
{
	uchar *dataptr = mp;
	long length;
	int r;

	r = dp->mode & 0777;
	if (dp->mode & DMDIR)
		length = 1024;
	else
		length = dp->length;
	if((dp->mode & DMDIR) && dp->type == '/' && dp->dev == 0)
		r |= 0555;
	if(dp->mode & DMDIR){
		PLONG(NFDIR);	/* type */
		r |= S_IFDIR;
		PLONG(r);	/* mode */
		PLONG(3);	/* nlink */
	}else{
		PLONG(NFREG);	/* type */
		r |= S_IFREG;
		PLONG(r);	/* mode */
		PLONG(1);	/* nlink */
	}
	r = name2id(&up->u.ids, dp->uid);
	if(r < 0){
		r = name2id(&up->u.ids, "daemon");
		if(r < 0)
			r = 1;
	}
	PLONG(r);		/* uid */
	r = name2id(&up->g.ids, dp->gid);
	if(r < 0){
		r = name2id(&up->g.ids, "user");
		if(r < 0)
			r = 1;
	}
	PLONG(r);		/* gid */
	PLONG(length);		/* size */
	PLONG(2048);		/* blocksize */
	PLONG(0);		/* rdev */
	r = (length+2047)/2048;
	PLONG(r);		/* blocks */
	r = (dp->type<<16) | dp->dev;
	PLONG(r);		/* fsid */
	PLONG(dp->qid.path);	/* fileid */
	PLONG(dp->atime);	/* atime */
	PLONG(0);
	PLONG(dp->mtime);	/* mtime */
	PLONG(0);
	PLONG(dp->mtime);	/* ctime */
	PLONG(0);
	return dataptr - (uchar *)mp;
}

int
convM2sattr(void *mp, Sattr *sp)
{
	uchar *argptr = mp;

	sp->mode = GLONG();
	sp->uid = GLONG();
	sp->gid = GLONG();
	sp->size = GLONG();
	sp->atime = GLONG();
	sp->ausec = GLONG();
	sp->mtime = GLONG();
	sp->musec = GLONG();
	return argptr - (uchar *)mp;
}
