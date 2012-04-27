#include "all.h"

/*
 *	Cf. /lib/rfc/rfc1094
 */

static int	nfsnull(int, Rpccall*, Rpccall*);
static int	nfsgetattr(int, Rpccall*, Rpccall*);
static int	nfssetattr(int, Rpccall*, Rpccall*);
static int	nfsroot(int, Rpccall*, Rpccall*);
static int	nfslookup(int, Rpccall*, Rpccall*);
static int	nfsreadlink(int, Rpccall*, Rpccall*);
static int	nfsread(int, Rpccall*, Rpccall*);
static int	nfswritecache(int, Rpccall*, Rpccall*);
static int	nfswrite(int, Rpccall*, Rpccall*);
static int	nfscreate(int, Rpccall*, Rpccall*);
static int	nfsremove(int, Rpccall*, Rpccall*);
static int	nfsrename(int, Rpccall*, Rpccall*);
static int	nfslink(int, Rpccall*, Rpccall*);
static int	nfssymlink(int, Rpccall*, Rpccall*);
static int	nfsmkdir(int, Rpccall*, Rpccall*);
static int	nfsrmdir(int, Rpccall*, Rpccall*);
static int	nfsreaddir(int, Rpccall*, Rpccall*);
static int	nfsstatfs(int, Rpccall*, Rpccall*);

Procmap nfsproc[] = {
	0, nfsnull,	/* void */
	1, nfsgetattr,	/* Fhandle */
	2, nfssetattr,	/* Fhandle, Sattr */
	3, nfsroot,	/* void */
	4, nfslookup,	/* Fhandle, String */
	5, nfsreadlink,	/* Fhandle */
	6, nfsread,	/* Fhandle, long, long, long */
	7, nfswritecache,/* void */
	8, nfswrite,	/* Fhandle, long, long, long, String */
	9, nfscreate,	/* Fhandle, String, Sattr */
	10, nfsremove,	/* Fhandle, String */
	11, nfsrename,	/* Fhandle, String, Fhandle, String */
	12, nfslink,	/* Fhandle, Fhandle, String */
	13, nfssymlink,	/* Fhandle, String, String, Sattr */
	14, nfsmkdir,	/* Fhandle, String, Sattr */
	15, nfsrmdir,	/* Fhandle, String */
	16, nfsreaddir,	/* Fhandle, long, long */
	17, nfsstatfs,	/* Fhandle */
	0, 0
};

void	nfsinit(int, char**);
extern void	mntinit(int, char**);
extern Procmap	mntproc[];

Progmap progmap[] = {
	100005, 1, mntinit, mntproc,
	100003, 2, nfsinit, nfsproc,
	0, 0, 0,
};

int	myport = 2049;
long	nfstime;
int	conftime;

void
main(int argc, char *argv[])
{
	server(argc, argv, myport, progmap);
}

static void
doalarm(void)
{
	nfstime = time(0);
	mnttimer(nfstime);
	if(conftime+5*60 < nfstime){
		conftime = nfstime;
		readunixidmaps(config);
	}
}

void
nfsinit(int argc, char **argv)
{
	/*
	 * mntinit will have already parsed our options.
	 */
	USED(argc, argv);
	clog("nfs file server init\n");
	rpcalarm = doalarm;
	nfstime = time(0);
}

static int
nfsnull(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, reply);
	chat("nfsnull...");
	showauth(&cmd->cred);
	chat("OK\n");
	return 0;
}

static int
nfsgetattr(int n, Rpccall *cmd, Rpccall *reply)
{
	Xfid *xf;
	Dir dir;
	uchar *dataptr = reply->results;

	chat("getattr...");
	if(n != FHSIZE)
		return garbage(reply, "bad count");
	xf = rpc2xfid(cmd, &dir);
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	chat("%s...", xf->xp->name);
	PLONG(NFS_OK);
	dataptr += dir2fattr(cmd->up, &dir, dataptr);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
nfssetattr(int n, Rpccall *cmd, Rpccall *reply)
{
	Xfid *xf;
	Dir dir, nd;
	Sattr sattr;
	int r;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;

	chat("setattr...");
	if(n <= FHSIZE)
		return garbage(reply, "count too small");
	xf = rpc2xfid(cmd, &dir);
	argptr += FHSIZE;
	argptr += convM2sattr(argptr, &sattr);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	chat("mode=0%lo,u=%ld,g=%ld,size=%ld,atime=%ld,mtime=%ld...",
		sattr.mode, sattr.uid, sattr.gid, sattr.size,
		sattr.atime, sattr.mtime);
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	if(sattr.uid != NOATTR || sattr.gid != NOATTR)
		return error(reply, NFSERR_PERM);
	if(sattr.size == 0){
		if(xf->xp->s != xf->xp->parent->s){
			if(xfauthremove(xf, cmd->user) < 0)
				return error(reply, NFSERR_PERM);
		}else if(dir.length && xfopen(xf, Trunc|Oread|Owrite) < 0)
			return error(reply, NFSERR_PERM);
	}else if(sattr.size != NOATTR)
		return error(reply, NFSERR_PERM);
	r = 0;
	nulldir(&nd);
	if(sattr.mode != NOATTR)
		++r, nd.mode = (dir.mode & ~0777) | (sattr.mode & 0777);
	if(sattr.atime != NOATTR)
		++r, nd.atime = sattr.atime;
	if(sattr.mtime != NOATTR)
		++r, nd.mtime = sattr.mtime;
	chat("sattr.mode=%luo dir.mode=%luo nd.mode=%luo...", sattr.mode, dir.mode, nd.mode);
	if(r){
		r = xfwstat(xf, &nd);
		if(r < 0)
			return error(reply, NFSERR_PERM);
	}
	if(xfstat(xf, &dir) < 0)
		return error(reply, NFSERR_STALE);
	PLONG(NFS_OK);
	dataptr += dir2fattr(cmd->up, &dir, dataptr);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
nfsroot(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, reply);
	chat("nfsroot...");
	showauth(&cmd->cred);
	chat("OK\n");
	return 0;
}

static int
nfslookup(int n, Rpccall *cmd, Rpccall *reply)
{
	Xfile *xp;
	Xfid *xf, *newxf;
	String elem;
	Dir dir;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;

	chat("lookup...");
	if(n <= FHSIZE)
		return garbage(reply, "count too small");
	xf = rpc2xfid(cmd, 0);
	argptr += FHSIZE;
	argptr += string2S(argptr, &elem);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	xp = xf->xp;
	if(!(xp->qid.type & QTDIR))
		return error(reply, NFSERR_NOTDIR);
	chat("%s -> \"%.*s\"...", xp->name, utfnlen(elem.s, elem.n), elem.s);
	if(xp->s->noauth == 0 && xp->parent == xp && elem.s[0] == '#')
		newxf = xfauth(xp, &elem);
	else
		newxf = xfwalkcr(Twalk, xf, &elem, 0);
	if(newxf == 0)
		return error(reply, NFSERR_NOENT);
	if(xfstat(newxf, &dir) < 0)
		return error(reply, NFSERR_IO);
	PLONG(NFS_OK);
	dataptr += xp2fhandle(newxf->xp, dataptr);
	dataptr += dir2fattr(cmd->up, &dir, dataptr);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
nfsreadlink(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, reply);
	chat("readlink...");
	showauth(&cmd->cred);
	return error(reply, NFSERR_NOENT);
}

static int
nfsread(int n, Rpccall *cmd, Rpccall *reply)
{
	Session *s;
	Xfid *xf;
	Dir dir;
	int offset, count;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	uchar *readptr = dataptr + 4 + 17*4 + 4;

	chat("read...");
	if(n != FHSIZE+12)
		return garbage(reply, "bad count");
	xf = rpc2xfid(cmd, 0);
	argptr += FHSIZE;
	offset = GLONG();
	count = GLONG();
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	chat("%s %d %d...", xf->xp->name, offset, count);
	if(xf->xp->s != xf->xp->parent->s){
		count = xfauthread(xf, offset, readptr, count);
	}else{
		if(xfopen(xf, Oread) < 0)
			return error(reply, NFSERR_PERM);
		if(count > 8192)
			count = 8192;
		s = xf->xp->s;
		setfid(s, xf->opfid);
		xf->opfid->tstale = nfstime + 60;
		s->f.offset = offset;
		s->f.count = count;
		if(xmesg(s, Tread) < 0)
			return error(reply, NFSERR_IO);
		count = s->f.count;
		memmove(readptr, s->f.data, count);
	}
	if(xfstat(xf, &dir) < 0)
		return error(reply, NFSERR_IO);
	PLONG(NFS_OK);
	dataptr += dir2fattr(cmd->up, &dir, dataptr);
	PLONG(count);
	dataptr += ROUNDUP(count);
	chat("%d OK\n", count);
	return dataptr - (uchar *)reply->results;
}

static int
nfswritecache(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, reply);
	chat("writecache...");
	showauth(&cmd->cred);
	chat("OK\n");
	return 0;
}

static int
nfswrite(int n, Rpccall *cmd, Rpccall *reply)
{
	Session *s;
	Xfid *xf;
	Dir dir;
	int offset, count;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;

	chat("write...");
	if(n < FHSIZE+16)
		return garbage(reply, "count too small");
	xf = rpc2xfid(cmd, 0);
	argptr += FHSIZE + 4;
	offset = GLONG();
	argptr += 4;
	count = GLONG();
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	chat("%s %d %d...", xf->xp->name, offset, count);
	if(xf->xp->s != xf->xp->parent->s){
		if(xfauthwrite(xf, offset, argptr, count) < 0)
			return error(reply, NFSERR_IO);
	}else{
		if(xfopen(xf, Owrite) < 0)
			return error(reply, NFSERR_PERM);
		s = xf->xp->s;
		setfid(s, xf->opfid);
		xf->opfid->tstale = nfstime + 60;
		s->f.offset = offset;
		s->f.count = count;
		s->f.data = (char *)argptr;
		if(xmesg(s, Twrite) < 0)
			return error(reply, NFSERR_IO);
	}
	if(xfstat(xf, &dir) < 0)
		return error(reply, NFSERR_IO);
	PLONG(NFS_OK);
	dataptr += dir2fattr(cmd->up, &dir, dataptr);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
creat(int n, Rpccall *cmd, Rpccall *reply, int chdir)
{
	Xfid *xf, *newxf;
	Xfile *xp;
	String elem;
	Dir dir; Sattr sattr;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;
	int trunced;

	if(n <= FHSIZE)
		return garbage(reply, "count too small");
	xf = rpc2xfid(cmd, 0);
	argptr += FHSIZE;
	argptr += string2S(argptr, &elem);
	argptr += convM2sattr(argptr, &sattr);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	xp = xf->xp;
	if(!(xp->qid.type & QTDIR))
		return error(reply, NFSERR_NOTDIR);
	chat("%s/%.*s...", xp->name, utfnlen(elem.s, elem.n), elem.s);
	trunced = 0;
	if(xp->parent == xp && elem.s[0] == '#'){
		newxf = xfauth(xp, &elem);
		if(newxf == 0)
			return error(reply, NFSERR_PERM);
		if(xfauthremove(newxf, cmd->user) < 0)
			return error(reply, NFSERR_PERM);
		trunced = 1;
	}else
		newxf = xfwalkcr(Twalk, xf, &elem, 0);
	if(newxf == 0){
		newxf = xfwalkcr(Tcreate, xf, &elem, chdir|(sattr.mode&0777));
		if(newxf)
			trunced = 1;
		else
			newxf = xfwalkcr(Twalk, xf, &elem, 0);
	}
	if(newxf == 0)
		return error(reply, NFSERR_PERM);
	if(!trunced && chdir)
		return error(reply, NFSERR_EXIST);
	if(!trunced && xfopen(newxf, Trunc|Oread|Owrite) < 0)
		return error(reply, NFSERR_PERM);
	if(xfstat(newxf, &dir) < 0)
		return error(reply, NFSERR_IO);

	PLONG(NFS_OK);
	dataptr += xp2fhandle(newxf->xp, dataptr);
	dataptr += dir2fattr(cmd->up, &dir, dataptr);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
nfscreate(int n, Rpccall *cmd, Rpccall *reply)
{
	chat("create...");
	return creat(n, cmd, reply, 0);
}

static int
remov(int n, Rpccall *cmd, Rpccall *reply)
{
	Session *s;
	Xfile *xp;
	Xfid *xf, *newxf;
	String elem;
	Fid *nfid;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;

	if(n <= FHSIZE)
		return garbage(reply, "count too small");
	xf = rpc2xfid(cmd, 0);
	argptr += FHSIZE;
	argptr += string2S(argptr, &elem);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	xp = xf->xp;
	if(!(xp->qid.type & QTDIR))
		return error(reply, NFSERR_NOTDIR);
	chat("%s/%.*s...", xp->name, utfnlen(elem.s, elem.n), elem.s);
	if(xp->s->noauth == 0 && xp->parent == xp && elem.s[0] == '#')
		return error(reply, NFSERR_PERM);
	newxf = xfwalkcr(Twalk, xf, &elem, 0);
	if(newxf == 0)
		return error(reply, NFSERR_NOENT);
	s = xp->s;
	nfid = newfid(s);
	setfid(s, newxf->urfid);
	s->f.newfid = nfid - s->fids;
	s->f.nwname = 0;
	if(xmesg(s, Twalk) < 0){
		putfid(s, nfid);
		return error(reply, NFSERR_IO);
	}
	s->f.fid = nfid - s->fids;
	if(xmesg(s, Tremove) < 0){
		putfid(s, nfid);
		return error(reply, NFSERR_PERM);
	}
	putfid(s, nfid);
	xpclear(newxf->xp);
	PLONG(NFS_OK);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
nfsremove(int n, Rpccall *cmd, Rpccall *reply)
{
	chat("remove...");
	return remov(n, cmd, reply);
}

static int
nfsrename(int n, Rpccall *cmd, Rpccall *reply)
{
	Xfid *xf, *newxf;
	Xfile *xp;
	uchar *fromdir, *todir;
	String fromelem, toelem;
	Dir dir;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;

	chat("rename...");
	if(n <= FHSIZE)
		return garbage(reply, "count too small");
	xf = rpc2xfid(cmd, 0);
	fromdir = argptr;
	argptr += FHSIZE;
	argptr += string2S(argptr, &fromelem);
	todir = argptr;
	argptr += FHSIZE;
	argptr += string2S(argptr, &toelem);
	if(argptr != &((uchar *)cmd->args)[n])
		return garbage(reply, "bad count");
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	xp = xf->xp;
	if(!(xp->qid.type & QTDIR))
		return error(reply, NFSERR_NOTDIR);
	if(memcmp(fromdir, todir, FHSIZE) != 0)
		return error(reply, NFSERR_NXIO);
	newxf = xfwalkcr(Twalk, xf, &fromelem, 0);
	if(newxf == 0)
		return error(reply, NFSERR_NOENT);
	if(xfstat(newxf, &dir) < 0)
		return error(reply, NFSERR_IO);

	if(xp->parent == xp && toelem.s[0] == '#')
		return error(reply, NFSERR_PERM);
	nulldir(&dir);
	dir.name = toelem.s;
	if(xfwstat(newxf, &dir) < 0)
		return error(reply, NFSERR_PERM);
	PLONG(NFS_OK);
	chat("OK\n");
	return dataptr - (uchar *)reply->results;
}

static int
nfslink(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, reply);
	chat("link...");
	showauth(&cmd->cred);
	return error(reply, NFSERR_NOENT);
}

static int
nfssymlink(int n, Rpccall *cmd, Rpccall *reply)
{
	USED(n, reply);
	chat("symlink...");
	showauth(&cmd->cred);
	return error(reply, NFSERR_NOENT);
}

static int
nfsmkdir(int n, Rpccall *cmd, Rpccall *reply)
{
	chat("mkdir...");
	return creat(n, cmd, reply, DMDIR);
}

static int
nfsrmdir(int n, Rpccall *cmd, Rpccall *reply)
{
	chat("rmdir...");
	return remov(n, cmd, reply);
}

static int
nfsreaddir(int n, Rpccall *cmd, Rpccall *reply)
{
	Session *s;
	Xfid *xf;
	Dir dir;
	char *rdata;
	int k, offset, count, sfcount, entries, dsize;
	uchar *argptr = cmd->args;
	uchar *dataptr = reply->results;

	chat("readdir...");
	if(n != FHSIZE+8)
		return garbage(reply, "bad count");
	xf = rpc2xfid(cmd, 0);
	argptr += FHSIZE;
	offset = GLONG();
	count = GLONG();
	if(xf == 0)
		return error(reply, NFSERR_STALE);
	chat("%s (%ld) %d %d...", xf->xp->name, xf->offset, offset, count);
	s = xf->xp->s;
	if((xf->mode & Open) && xf->offset > offset)
		xfclose(xf);
	if(xfopen(xf, Oread) < 0)
		return error(reply, NFSERR_PERM);
	while(xf->offset < offset){	/* if we reopened, xf->offset will be zero */
		sfcount = offset - xf->offset;
		if(sfcount > messagesize-IOHDRSZ)
			sfcount = messagesize-IOHDRSZ;
		setfid(s, xf->opfid);
		s->f.offset = xf->offset;
		s->f.count = sfcount;
		if(xmesg(s, Tread) < 0){
			xfclose(xf);
			return error(reply, NFSERR_IO);
		}
		if(s->f.count <= BIT16SZ)
			break;
		xf->offset += s->f.count;
	}
	if(count > messagesize-IOHDRSZ)
		count = messagesize-IOHDRSZ;
	PLONG(NFS_OK);
	entries = 0;
	while(count > 16){	/* at least 16 bytes required; we don't know size of name */
chat("top of loop\n");
		setfid(s, xf->opfid);
		s->f.offset = xf->offset;
		s->f.count = count;	/* as good a guess as any */
		if(xmesg(s, Tread) < 0){
			xfclose(xf);
			return error(reply, NFSERR_IO);
		}
		sfcount = s->f.count;
		if(sfcount <= BIT16SZ)
			break;
		xf->offset += sfcount;
chat("count %d data 0x%p\n", s->f.count, s->f.data);
		rdata = s->f.data;
		/* now have a buffer of Plan 9 directories; unpack into NFS thingies */
		while(sfcount >= 0){
			dsize = convM2D((uchar*)rdata, sfcount, &dir, (char*)s->statbuf);
			if(dsize <= BIT16SZ){
				count = 0;	/* force break from outer loop */
				break;
			}
			offset += dsize;
			k = strlen(dir.name);
			if(count < 16+ROUNDUP(k)){
				count = 0;	/* force break from outer loop */
				break;
			}
			PLONG(TRUE);
			PLONG(dir.qid.path);
			PLONG(k);
			PPTR(dir.name, k);
			PLONG(offset);
			count -= 16+ROUNDUP(k);
			rdata += dsize;
			sfcount -= dsize;
		}
	}
	PLONG(FALSE);
	if(s->f.count <= 0){
		xfclose(xf);
		chat("eof...");
		PLONG(TRUE);
	}else
		PLONG(FALSE);
	chat("%d OK\n", entries);
	return dataptr - (uchar *)reply->results;
}

static int
nfsstatfs(int n, Rpccall *cmd, Rpccall *reply)
{
	uchar *dataptr = reply->results;
	enum {
		Xfersize = 2048,
		Maxlong = (long)((1ULL<<31) - 1),
		Maxfreeblks = Maxlong / Xfersize,
	};

	chat("statfs...");
	showauth(&cmd->cred);
	if(n != FHSIZE)
		return garbage(reply, "bad count");
	PLONG(NFS_OK);
	PLONG(4096);		/* tsize (fs block size) */
	PLONG(Xfersize);	/* bsize (optimal transfer size) */
	PLONG(Maxfreeblks);	/* blocks in fs */
	PLONG(Maxfreeblks);	/* bfree to root*/
	PLONG(Maxfreeblks);	/* bavail (free to mortals) */
	chat("OK\n");
	/*conftime = 0;
	readunixidmaps(config);*/
	return dataptr - (uchar *)reply->results;
}
