#include "all.h"

static RWLock fidhashlk;
static Fid *fidshd, *fidstl;
static Fid *fidhash[Fidhashsz];
static uint fidgen;

int noauth;

Alloc fidalloc =
{
	.elsz = sizeof(Fid),
	.zeroing = 1,
};
Alloc rpcalloc =
{
	.elsz = sizeof(Largerpc),
	.zeroing = 0,
};

Alloc clialloc =
{
	.elsz = sizeof(Cli),
	.zeroing = 1,
};

static QLock clientslk;
static Cli *clients;

static void
fidlink(Fid *fid)
{
	fid->next = fidshd;
	fid->prev = nil;
	if(fidshd != nil)
		fidshd->prev = fid;
	else
		fidstl = fid;
	fidshd = fid;
}

static void
fidunlink(Fid *fid)
{
	if(fid->prev != nil)
		fid->prev->next = fid->next;
	else
		fidshd = fid->next;
	if(fid->next != nil)
		fid->next->prev = fid->prev;
	else
		fidstl = fid->prev;
	fid->next = nil;
	fid->prev = nil;
}

static int
fidfmt(Fmt *fmt)
{
	Fid *fid;
	Path *p;
	int i;

	fid = va_arg(fmt->args, Fid*);
	if(fid == nil)
		return fmtprint(fmt, "<nil>");
	fmtprint(fmt, "fid %#p no %d r%d, omode %d arch %d",
		fid, fid->no, fid->ref, fid->omode, fid->archived);
	p = fid->p;
	if(p == nil)
		return 0;
	fmtprint(fmt, " path");
	for(i = 0; i < p->nf; i++)
		fmtprint(fmt, " d%#ullx", p->f[i]->addr);
	return fmtprint(fmt, "\n=>%H", p->f[p->nf-1]);
}

void
dumpfids(void)
{
	Fid *fid;
	int n;

	xrwlock(&fidhashlk, Rd);
	fprint(2, "fids:\n");
	n = 0;
	for(fid = fidshd; fid != nil; fid = fid->next)
		fprint(2, "[%d] = %X\n", n++, fid);
	xrwunlock(&fidhashlk, Rd);
}

static int
meltpath(Path *p)
{
	int i, n;
	Memblk *f;

	n = 0;
	for(i = 0; i < p->nf; i++)
		while((f = p->f[i]->mf->melted) != nil){
			n++;
			incref(f);
			mbput(p->f[i]);
			p->f[i] = f;
		}
	return n;
}

void
meltfids(void)
{
	Fid *fid;
	int n;

	xrwlock(&fidhashlk, Rd);
	n = 0;
	for(fid = fidshd; fid != nil; fid = fid->next)
		if(canqlock(fid)){
			if(!fid->archived && fid->p != nil)
				n += meltpath(fid->p);
			qunlock(fid);
		}
	xrwunlock(&fidhashlk, Rd);
	dprint("meltfids: %d fids advanced\n", n);
}

void
countfidrefs(void)
{
	Fid *fid;
	Path *p;
	int i;

	xrwlock(&fidhashlk, Rd);
	for(fid = fidshd; fid != nil; fid = fid->next){
		p = fid->p;
		for(i = 0; i < p->nf; i++)
			mbcountref(p->f[i]);
	}
	xrwunlock(&fidhashlk, Rd);
}

Rpc*
newrpc(void)
{
	Rpc *rpc;

	rpc = anew(&rpcalloc);
	rpc->next = nil;
	rpc->cli = nil;
	rpc->fid = nil;
	rpc->flushed = 0;
	rpc->closed = 0;
	rpc->chan = ~0;
	rpc->rpc0 = nil;
	/* ouch! union. */
	if(sizeof(Fcall) > sizeof(IXcall)){
		memset(&rpc->t, 0, sizeof rpc->t);
		memset(&rpc->r, 0, sizeof rpc->r);
	}else{
		memset(&rpc->xt, 0, sizeof rpc->xt);
		memset(&rpc->xr, 0, sizeof rpc->xr);
	}
	return rpc;	
}

void
freerpc(Rpc *rpc)
{
	afree(&rpcalloc, rpc);
}

Fid*
newfid(void* clino, int no)
{
	Fid *fid, **fidp;

	xrwlock(&fidhashlk, Wr);
	if(catcherror()){
		xrwunlock(&fidhashlk, Wr);
		error(nil);
	}
	if(no < 0)
		no = fidgen++;
	for(fidp = &fidhash[no%Fidhashsz]; *fidp != nil; fidp = &(*fidp)->hnext)
		if((*fidp)->clino == clino && (*fidp)->no == no)
			error("fid in use");
	fid = anew(&fidalloc);
	*fidp = fid;
	fid->hnext = nil;
	fid->omode = -1;
	fid->no = no;
	fid->clino = clino;
	fid->ref = 2;	/* one for the caller; another because it's kept */
	fidlink(fid);
	noerror();
	xrwunlock(&fidhashlk, Wr);
	dEprint("new fid %X\n", fid);
	return fid;
}

Fid*
getfid(void* clino, int no)
{
	Fid *fid;

	xrwlock(&fidhashlk, Rd);
	if(catcherror()){
		xrwunlock(&fidhashlk, Rd);
		error(nil);
	}
	for(fid = fidhash[no%Fidhashsz]; fid != nil; fid = fid->hnext)
		if(fid->clino == clino && fid->no == no){
			incref(fid);
			noerror();
			dEprint("getfid %d -> %X\n", no, fid);
			xrwunlock(&fidhashlk, Rd);
			return fid;
		}
	error("fid not found");
	return fid;
}

void
putfid(Fid *fid)
{
	Fid **fidp;

	if(fid == nil || decref(fid) > 0)
		return;
	d9print("clunk %X\n", fid);
	putpath(fid->p);
	fid->p = nil;
	xrwlock(&fidhashlk, Wr);
	if(catcherror()){
		xrwunlock(&fidhashlk, Wr);
		fprint(2, "putfid: %r");
		error(nil);
	}
	for(fidp = &fidhash[fid->no%Fidhashsz]; *fidp != nil; fidp = &(*fidp)->hnext)
		if(*fidp == fid){
			*fidp = fid->hnext;
			fidunlink(fid);
			noerror();
			xrwunlock(&fidhashlk, Wr);
			afree(&fidalloc, fid);
			return;
		}
	fatal("putfid: fid not found");
}

/* keeps addr, does not copy it */
static Cli*
newcli(char *addr, int fd, int cfd)
{
	Cli *cli;

	cli = anew(&clialloc);
	cli->fd = fd;
	cli->cfd = cfd;
	cli->addr = addr;
	cli->ref = 1;
	cli->uid = -1;

	xqlock(&clientslk);
	cli->next = clients;
	clients = cli;
	xqunlock(&clientslk);
	return cli;
}

void
putcli(Cli *cli)
{
	Cli **cp;

	if(decref(cli) == 0){
		xqlock(&clientslk);
		for(cp = &clients; *cp != nil; cp = &(*cp)->next)
			if(*cp == cli)
				break;
		if(*cp == nil)
			fatal("client not found");
		*cp = cli->next;
		xqunlock(&clientslk);
		close(cli->fd);
		close(cli->cfd);
		free(cli->addr);
		afree(&clialloc, cli);
	}
}

void
consprintclients(void)
{
	Cli *c;

	xqlock(&clientslk);
	for(c = clients; c != nil; c = c->next)
		consprint("%s!%s\n", c->addr, usrname(c->uid));
	xqunlock(&clientslk);
}

void
setfiduid(Fid *fid, char *uname)
{
	if(uname[0] == 0)
		error("null uid");

	fid->uid = usrid(uname);

	/*
	 * The owner of the process must be able to attach, if only to
	 * define users and administer the file system.
	 */
	if(fid->uid < 0 && strcmp(uname, getuser()) == 0){
		fprint(2, "%s: owner uid '%s' unknown. attaching as 'elf'\n",
			argv0, uname);
		fid->uid = usrid("elf");
	}

	if(fid->uid < 0){
		fprint(2, "%s: unknown user '%s'. using 'none'\n", argv0, uname);
		fid->uid = usrid("none");
	}
}

void
fidattach(Fid *fid, char *aname, char *uname)
{
	Path *p;

	setfiduid(fid, uname);
	p = newpath(fs->root);
	fid->p = p;
	if(strcmp(aname, "active") == 0 || strcmp(aname, "main/active") == 0){
		addelem(&p, fs->active);
		return;
	}
	fid->archived = 1;
	if(strcmp(aname, "archive") == 0 || strcmp(aname, "main/archive") == 0)
		addelem(&p, fs->archive);
	else if(strcmp(aname, "main") != 0 && strcmp(aname, "") != 0)
		error("unknown tree");
}

Fid*
fidclone(Cli *cli, Fid *fid, int no)
{
	Fid *nfid;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	nfid = newfid(cli, no);
	nfid->p = clonepath(fid->p);
	nfid->uid = fid->uid;
	nfid->archived = fid->archived;
	nfid->consopen = fid->consopen;
	nfid->buf = fid->buf;
	noerror();
	xqunlock(fid);
	return nfid;
}

static Memblk*
pickarch(Memblk *d, uvlong t)
{
	Blksl sl;
	daddrt *de;
	uvlong off;
	int i;
	uvlong cmtime;
	daddrt cdaddr;
	Memblk *f;

	off = 0;
	cmtime = 0;
	cdaddr = 0;
	for(;;){
		sl = dfslice(d, Dblkdatasz, off, Rd);
		if(sl.len == 0){
			assert(sl.b == nil);
			break;
		}
		if(sl.b == nil)
			continue;
		if(catcherror()){
			mbput(sl.b);
			error(nil);
		}
		for(i = 0; i < sl.len/Daddrsz; i++){
			de = sl.data;
			de += i;
			if(*de == 0)
				continue;
			f = dbget(DBfile, *de);
			if(f->d.mtime > cmtime && f->d.mtime < t){
				cmtime = f->d.mtime;
				cdaddr = *de;
			}
			mbput(f);
		}
		noerror();
		mbput(sl.b);
		off += sl.len;
	}
	if(cdaddr == 0)
		error("file not found");
	return dbget(DBfile, cdaddr);
}

static int
digs(char **sp, int n)
{
	char b[8];
	char *s;

	s = *sp;
	if(strlen(s) < n)
		return 0;
	assert(n < sizeof b - 1);
	strecpy(b, b+n+1, *sp);
	*sp += n;
	return strtoul(b, nil, 10);
}

/*
 * convert symbolic time into a valid /archive wname.
 * yyyymmddhhmm, yyyymmdd, mmdd, or hh:mm
 */
static Memblk*
archwalk(Memblk *f, char *wname)
{
	char *s;
	Tm *tm;
	uvlong t;
	static QLock tmlk;
	int wl;

	s = wname;
	qlock(&tmlk);				/* localtime is not reentrant! */
	tm = localtime(time(nil));
	wl = strlen(wname);
	switch(wl){
	case 12:				/* yyyymmddhhmm */
	case 8:					/* yyyymmdd */
		tm->year = digs(&s, 4) - 1900;
	case 4:					/* mmdd */
		tm->mon = digs(&s, 2) - 1;
		tm->mday = digs(&s, 2);
		if(wl == 8)
			break;
		/* else fall */
	case 5:					/* hh:mm */
		tm->hour = digs(&s, 2);
		if(wl == 5 && s[0] != 0)
			s++;
		tm->min = digs(&s, 2);
		break;
	default:
		qunlock(&tmlk);
		error("file not found");
	}
	dprint("archwalk to %d/%d/%d %d:%d:%d\n",
		tm->year, tm->mday, tm->mon, tm->hour, tm->min, tm->sec);
	t = tm2sec(tm);
	t *= NSPERSEC;
	qunlock(&tmlk);
	return pickarch(f, t);
}


/*
 * We are at /active/... or /archive/x/...
 * Walk to /archive/T/... and return it so it's added to the
 * path by the caller.
 */
static Memblk*
timewalk(Path *p, char *wname, int uid)
{
	Memblk *f, *nf, *pf, *arch, *af;
	int i, isarch;

	if(p->nf < 2 || (p->f[1] == fs->archive && p->nf < 3))
		error("file not found");
	assert(p->f[0] == fs->root);
	isarch = p->f[1] == fs->archive;
	assert(p->f[1] == fs->active || isarch);

	arch = fs->archive;
	rwlock(arch, Rd);
	if(catcherror()){
		rwunlock(arch, Rd);
		error(nil);
	}
	af = archwalk(arch, wname);
	noerror();
	rwunlock(arch, Rd);
	if(catcherror()){
		mbput(af);
		error(nil);
	}
	i = 2;
	if(isarch)
		i++;
	f = af;
	incref(f);
	nf = nil;
	for(; i < p->nf; i++){
		pf = p->f[i];
		rwlock(f, Rd);
		rwlock(pf, Rd);
		if(catcherror()){
			rwunlock(pf, Rd);
			rwunlock(f, Rd);
			mbput(f);
			error(nil);
		}
		dfaccessok(f, uid, AEXEC);
		nf = dfwalk(f, pf->mf->name, Rd);
		noerror();
		rwunlock(pf, Rd);
		rwunlock(f, Rd);
		mbput(f);
		f = nf;
	}
	noerror();
	mbput(af);
	USED(f);
	USED(nf);

	if((f->d.mode&DMDIR) == 0){	/* it was not a dir at that time! */
		mbput(f);
		error("file not found");
	}
	return f;
}

/*
 * For walks into /archive, wname may be any time value (ns) or
 * yyyymmddhhmm, yyyymmdd, mmdd, or hh:mm,
 * and the walk proceeds to the archived file
 * with the bigger mtime not greater than the specified time.
 * If there's no such time, walk reports a file not found error.
 *
 * walks using @<symbolic time> lead to the corresponding dir in the archive.
 */
void
fidwalk(Fid *fid, char *wname)
{
	Path *p;
	Memblk *f, *nf;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	p = dflast(&fid->p, fid->p->nf);
	if(strcmp(wname, ".") == 0)
		goto done;
	if(strcmp(wname, "..") == 0){
		if(p->nf > 1)
			p = dropelem(&fid->p);
		goto done;
	}
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	if(catcherror()){
		rwunlock(f, Rd);
		error(nil);
	}
	dfaccessok(f, fid->uid, AEXEC);

	nf = nil;
	if(catcherror()){
		if(f == fs->archive)
			nf = archwalk(f, wname);
		else if(wname[0] == '@' && f != fs->cons && f != fs->stats)
			nf = timewalk(p, wname+1, fid->uid);
		else
			error(nil);
		fid->archived = 1;
	}else{
		nf = dfwalk(f, wname, Rd);
		noerror();
	}

	rwunlock(f, Rd);
	noerror();
	p = addelem(&fid->p, nf);
	decref(nf);
done:
	f = p->f[p->nf-1];
	if(isro(f))
		fid->archived = f != fs->cons && f != fs->stats;
	else if(f == fs->active)
		fid->archived = 0;
	dfused(p);
	noerror();
	xqunlock(fid);
}

void
fidopen(Fid *fid, int mode)
{
	int fmode, amode;
	Memblk *f;
	Path *p;
	uvlong z;

	if(fid->omode != -1)
		error("fid already open");

	/* check this before we try to melt it */
	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	p = fid->p;
	f = p->f[p->nf-1];
	if(mode != OREAD){
		if(f == fs->root || f == fs->archive || fid->archived)
			error("can't write archived or built-in files");
		if(writedenied(fid->uid))
			error("user can't write");
	}
	amode = 0;
	if((mode&3) != OREAD || (mode&OTRUNC) != 0)
		amode |= AWRITE;
	if((mode&3) != OWRITE)
		amode |= AREAD;
	if(amode != AREAD)
		if(f == fs->cons)
			rwlock(f, Wr);
		else{
			p = dfmelt(&fid->p, fid->p->nf);
			f = p->f[p->nf-1];
		}
	else{
		p = dflast(&fid->p, fid->p->nf);
		rwlock(f, Rd);
	}
	if(catcherror()){
		rwunlock(f, (amode!=AREAD)?Wr:Rd);
		error(nil);
	}
	fmode = f->d.mode;
	if(mode != OREAD){
		if(f != fs->root && p->f[p->nf-2]->d.mode&DMAPPEND)
			error("directory is append only");
		if((fmode&DMDIR) != 0)
			error("wrong open mode for a directory");
	}
	dfaccessok(f, fid->uid, amode);
	if(mode&ORCLOSE){
		if(fid->archived || isro(f))
			error("can't remove an archived or built-in file");
		if(f->d.mode&DMUSERS)
			error("can't remove /users");
		dfaccessok(p->f[p->nf-2], fid->uid, AWRITE);
		fid->rclose++;
	}
	if((fmode&DMEXCL) != 0 && f->mf->open)
		if(f != fs->cons || amode != AWRITE)	/* ok to write cons */
			error("exclusive use file already open");
	if((mode&OTRUNC) != 0&& f != fs->cons && f != fs->stats){
		z = 0;
		dfwattr(f, "length", &z, sizeof z);
		if(f->d.mode&DMUSERS){
			f->d.mode = 0664|DMUSERS;
			f->d.uid = usrid("adm");
			f->d.gid = usrid("adm");
			f->mf->uid = "adm";
			f->mf->gid = "adm";
			changed(f);
		}
	}
	if(f == fs->stats)
		fid->buf = updatestats(mode&OTRUNC);
	f->mf->open++;
	fid->omode = mode&3;
	fid->loff = 0;
	fid->lidx = 0;
	fid->consopen = f == fs->cons;
	noerror();
	rwunlock(f, (amode!=AREAD)?Wr:Rd);
	if(mode&OTRUNC)
		dfchanged(p, fid->uid);
	else
		dfused(p);
	noerror();
	xqunlock(fid);
}

void
fidcreate(Fid *fid, char *name, int mode, ulong perm)
{
	Path *p;
	Memblk *f, *nf;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	if(fid->omode != -1)
		error("fid already open");
	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		error("that file name scares me");
	if(utfrune(name, '/') != nil)
		error("that file name is too creepy");
	if((perm&DMDIR) != 0 && mode != OREAD)
		error("wrong open mode for a directory");
	if(writedenied(fid->uid))
		error("user can't write");
	if(fid->archived)
		error("%P is archived or builtin", fid->p);
	if((perm&DMBITS) != perm)
		error("unknown bit set in perm %M %#ulx", perm, perm);
	p = fid->p;
	f = p->f[p->nf-1];
	if(mode&ORCLOSE)
		if(f->d.mode&DMUSERS)
			error("can't remove the users file");
	if((f->d.mode&DMDIR) == 0)
		error("not a directory");
	p = dfmelt(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	if(catcherror()){
		rwunlock(f, Wr);
		error(nil);
	}
	dfaccessok(f, fid->uid, AWRITE);
	if(!catcherror()){
		mbput(dfwalk(f, name, Rd));
		error("file already exists");
	}


	nf = dfcreate(f, name, fid->uid, perm);
	p = addelem(&fid->p, nf);
	if(f == fs->active && strcmp(name, "users") == 0){
		nf->d.mode = 0664|DMUSERS;
		nf->d.uid = usrid("adm");
		nf->d.gid = usrid("adm");
		nf->mf->uid = "adm";
		nf->mf->gid = "adm";
		changed(nf);
	}
	decref(nf);
	nf->mf->open++;
	noerror();
	rwunlock(f, Wr);
	fid->omode = mode&3;
	fid->loff = 0;
	fid->lidx = 0;
	if(mode&ORCLOSE)
		fid->rclose++;
	dfchanged(p, fid->uid);
	noerror();
	xqunlock(fid);
}

static ulong
readdir(Fid *fid, uchar *data, ulong ndata, uvlong, Packmeta pack)
{
	Memblk *d, *f;
	ulong tot, nr;

	d = fid->p->f[fid->p->nf-1];
	for(tot = 0; tot+2 < ndata && fid->lidx < d->d.ndents; tot += nr){
		f = dfchild(d, fid->lidx);
		nr = pack(f, data+tot, ndata-tot);
		mbput(f);
		if(nr <= 2)
			break;
		fid->lidx++;
	}
	return tot;
}

static long
strread(char *s, void *data, ulong count, vlong offset)
{
	long n;

	n = strlen(s);
	if(offset >= n)
		return 0;
	s += offset;
	n -= offset;
	if(count > n)
		count = n;
	if(count > 0)
		memmove(data, s, count);
	return count;
}

long
fidread(Fid *fid, void *data, ulong count, vlong offset, Packmeta pack)
{
	Memblk *f;
	Path *p;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	if(fid->omode == -1)
		error("fid not open");
	if(fid->omode == OWRITE)
		error("fid not open for reading");
	if(offset < 0)
		error("negative offset");
	p = dflast(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	if(f == fs->cons){
		noerror();
		xqunlock(fid);
		return consread(data, count);
	}
	if(f == fs->stats){
		noerror();
		assert(fid->buf != nil);
		count = strread(fid->buf, data, count, offset);
		xqunlock(fid);
		return count;
	}
	rwlock(f, Rd);
	noerror();
	xqunlock(fid);
	if(catcherror()){
		rwunlock(f, Rd);
		error(nil);
	}
	if(f->d.mode&DMDIR){
		if(fid->loff != offset)
			error("non-sequential dir read not supported");
		count = readdir(fid, data, count, offset, pack);
		fid->loff += count;
	}else
		count = dfpread(f, data, count, offset);
	noerror();
	rwunlock(f, Rd);
	dfused(p);
	return count;
}

long
fidwrite(Fid *fid, void *data, ulong count, uvlong *offset)
{
	Memblk *f;
	Path *p;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	if(writedenied(fid->uid))
		error("user can't write");
	if(fid->omode == -1)
		error("fid not open");
	if(fid->omode == OREAD)
		error("fid not open for writing");
	p = fid->p;
	f = p->f[p->nf-1];
	if(f == fs->cons){
		xqunlock(fid);
		noerror();
		return conswrite(data, count);
	}
	if(f == fs->stats){
		xqunlock(fid);
		noerror();
		return count;
	}
		
	p = dfmelt(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	if(catcherror()){
		rwunlock(f, Wr);
		error(nil);
	}
	count = dfpwrite(f, data, count, offset);
	noerror();
	rwunlock(f, Wr);

	dfchanged(p, fid->uid);
	noerror();
	xqunlock(fid);
	return count;
}

void
fidclose(Fid *fid)
{
	Memblk *f, *fp;
	Path *p;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	p = fid->p;
	f = p->f[p->nf-1];
	rwlock(f, Wr);
	f->mf->open--;
	if((f->d.mode&DMUSERS) && (fid->omode&3) != OREAD)
		rwusers(f);
	rwunlock(f, Wr);
	fid->omode = -1;
	if(fid->rclose){
		dflast(&fid->p, fid->p->nf);
		p = dfmelt(&fid->p, fid->p->nf-1);
		fp = p->f[p->nf-2];
		rwlock(f, Wr);
		if(catcherror()){
			rwunlock(f, Wr);
			mbput(f);
		}else{
			dfremove(fp, f);
			fid->p->nf--;
			noerror();
		}
		rwunlock(fp, Wr);
		dfchanged(p, fid->uid);
	}
	putpath(fid->p);
	fid->p = nil;
	fid->consopen = 0;
	noerror();
	xqunlock(fid);
}

void
fidremove(Fid *fid)
{
	Memblk *f, *fp;
	Path *p;

	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	if(writedenied(fid->uid))
		error("user can't write");
	p = fid->p;
	f = p->f[p->nf-1];
	if(fid->archived || isro(f))
		error("can't remove archived or built-in files");
	dflast(&fid->p, fid->p->nf);
	p = dfmelt(&fid->p, fid->p->nf-1);
	fp = p->f[p->nf-2];
	f = p->f[p->nf-1];
	rwlock(f, Wr);
	if(catcherror()){
		rwunlock(f, Wr);
		rwunlock(fp, Wr);
		error(nil);
	}
	if(fp->d.mode&DMAPPEND)
		error("directory is append only");
	if(f->d.mode&DMUSERS)
		error("can't remove the users file");
	dfaccessok(fp, fid->uid, AWRITE);
	fid->omode = -1;
	dfremove(fp, f);
	fid->p->nf--;
	noerror();
	rwunlock(fp, Wr);
	dfchanged(fid->p, fid->uid);
	putpath(fid->p);
	fid->p = nil;
	noerror();
	xqunlock(fid);
}

void
replied(Rpc *rpc)
{
	Rpc **rl;

	xqlock(&rpc->cli->rpclk);
	for(rl = &rpc->cli->rpcs; (*rl != nil); rl = &(*rl)->next)
		if(*rl == rpc){
			*rl = rpc->next;
			break;
		}
	rpc->cli->nrpcs--;
	xqunlock(&rpc->cli->rpclk);
	rpc->next = nil;
	assert(rpc->fid == nil);
	putcli(rpc->cli);
	rpc->cli = nil;

}

/*
 * Read ahead policy: to be called after replying to an ok. read RPC.
 *
 * We try to keep at least Nahead more bytes in the file if it seems
 * that's ok.
 */
void
rahead(Memblk *f, uvlong offset)
{
	Mfile *m;

	rwlock(f, Rd);
	m = f->mf;
	if(m->sequential == 0 || m->raoffset > offset + Nahead){
		rwunlock(f, Rd);
		return;
	}
	if(catcherror()){
		rwunlock(f, Rd);
		fprint(2, "%s: rahead: %r\n", argv0);
		return;
	}
	m->raoffset = offset + Nahead;
	d9print("rahead d%#ullx off %#ullx\n", f->addr, m->raoffset);
	for(; offset < m->raoffset; offset += Maxmdata)
		if(dfpread(f, nil, Maxmdata, offset) != Maxmdata)
			break;
	noerror();
	rwunlock(f, Rd);
}

/*
 * Walk ahead policy: to be called after replying to an ok. walk RPC.
 *
 * We try to keep the children of a directory we have walked to
 * loaded in memory before further walks/reads.
 */
void
wahead(Memblk *f)
{
	Mfile *m;
	int i;

	rwlock(f, Rd);
	m = f->mf;
	if((f->d.mode&DMDIR) == 0 || m->wadone){
		rwunlock(f, Rd);
		return;
	}
	m->wadone = 1;
	if(catcherror()){
		rwunlock(f, Rd);
		error(nil);
	}
	for(i = 0; i < f->d.ndents; i++)
		mbput(dfchild(f, i));
	noerror();
	rwunlock(f, Rd);
}

static void
postfd(char *name, int pfd)
{
	int fd;

	remove(name);
	fd = create(name, OWRITE|ORCLOSE|OCEXEC, 0600);
	if(fd < 0)
		fatal("postfd: %r\n");
	if(fprint(fd, "%d", pfd) < 0){
		close(fd);
		fatal("postfd: %r\n");
	}
	close(pfd);
}

static char*
getremotesys(char *ndir)
{
	char buf[128], *serv, *sys;
	int fd, n;

	snprint(buf, sizeof buf, "%s/remote", ndir);
	sys = nil;
	fd = open(buf, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n>0){
			buf[n-1] = 0;
			serv = strchr(buf, '!');
			if(serv)
				*serv = 0;
			sys = strdup(buf);
		}
		close(fd);
	}
	if(sys == nil)
		sys = strdup("unknown");
	return sys;
}

static void
srv9pix(char *srv, char* (*cliworker)(void *arg, void **aux))
{
	Cli *cli;
	int fd[2];
	char *name;

	name = smprint("/srv/%s", srv);
	if(pipe(fd) < 0)
		fatal("pipe: %r");
	postfd(name, fd[0]);
	consprint("listen %s\n", srv);
	cli = newcli(name, fd[1], -1);
	getworker(cliworker, cli, nil);
}

static void
listen9pix(char *addr,  char* (*cliworker)(void *arg, void **aux))
{
	Cli *cli;
	char ndir[NETPATHLEN], dir[NETPATHLEN];
	int ctl, data, nctl;

	ctl = announce(addr, dir);
	if(ctl < 0)
		fatal("announce %s: %r", addr);
	consprint("listen %s\n", addr);
	for(;;){
		nctl = listen(dir, ndir);
		if(nctl < 0)
			fatal("listen %s: %r", addr);
		data = accept(nctl, ndir);
		if(data < 0){
			fprint(2, "%s: accept %s: %r\n", argv0, ndir);
			continue;
		}
		cli = newcli(getremotesys(ndir), data, nctl);
		getworker(cliworker, cli, nil);
	}
}

static void
usage(void)
{
	fprint(2, "usage: %s [-DFLAGS] [-n addr] disk\n", argv0);
	exits("usage");
}

int mainstacksize = Stack;

void
threadmain(int argc, char *argv[])
{
	char *addr, *dev, *srv;

	addr = nil;
	srv = "9pix";
	ARGBEGIN{
	case 'n':
		addr = EARGF(usage());
		break;
	case 'a':
		noauth = 1;
		break;
	default:
		if(ARGC() >= 'A' && ARGC() <= 'Z' || ARGC() == '9'){
			dbg['d'] = 1;
			dbg[ARGC()] = 1;
			fatalaborts = 1;
		}else
			usage();
	}ARGEND;
	if(argc != 1)
		usage();
	dev = argv[0];

	workerthreadcreate = proccreate;
	fmtinstall('H', mbfmt);
	fmtinstall('M', dirmodefmt);
	fmtinstall('F', fcallfmt);
	fmtinstall('G', ixcallfmt);
	fmtinstall('X', fidfmt);
	fmtinstall('R', rpcfmt);
	fmtinstall('A', usrfmt);
	fmtinstall('P', pathfmt);

	errinit(Errstack);
	if(catcherror())
		fatal("uncatched error: %r");
	rfork(RFNAMEG);
	rwusers(nil);
	fsopen(dev);
	if(srv != nil)
		srv9pix(srv, cliworker9p);
	if(addr != nil)
		listen9pix(addr, cliworker9p);

	consinit();
	proccreate(fssyncproc, nil, Stack);
	noerror();
	threadexits(nil);
}

