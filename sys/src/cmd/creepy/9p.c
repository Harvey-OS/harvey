#include "all.h"

/*
 * 9p server for creepy
 */

static void	rflush(Rpc*), rversion(Rpc*), rauth(Rpc*),
		rattach(Rpc*), rwalk(Rpc*),
		ropen(Rpc*), rcreate(Rpc*),
		rread(Rpc*), rwrite(Rpc*), rclunk(Rpc*),
		rremove(Rpc*), rstat(Rpc*), rwstat(Rpc*);

static void (*fcalls[])(Rpc*) =
{
	[Tversion]	rversion,
	[Tflush]	rflush,
	[Tauth]		rauth,
	[Tattach]	rattach,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]		rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

char*
ninestats(char *s, char *e, int clr)
{
	int i;

	s = seprint(s, e, "fids:\t%4uld alloc %4uld free (%4uld bytes)\n",
		fidalloc.nalloc, fidalloc.nfree, fidalloc.elsz);
	s = seprint(s, e, "rpcs:\t%4uld alloc %4uld free (%4uld bytes)\n",
		rpcalloc.nalloc, rpcalloc.nfree, rpcalloc.elsz);
	s = seprint(s, e, "clis:\t%4uld alloc %4uld free (%4uld bytes)\n",
		clialloc.nalloc, clialloc.nfree, clialloc.elsz);
	for(i = 0; i < nelem(fcalls); i++)
		if(fcalls[i] != nil && ncalls[i] > 0){
			s = seprint(s, e, "%-8s\t%5uld calls\t%11ulld µs per call\n",
				callname[i], ncalls[i],
				(calltime[i]/ncalls[i])/1000);
			if(clr){
				ncalls[i] = 0;
				calltime[i] = 0;
			}
		}
	return s;
}

/*
 * Ok if f is nil, for auth files.
 */
static Qid
mkqid(Memblk *f)
{
	Qid q;
	static uvlong authgen;

	if(f == nil){
		authgen++;
		q.type = QTAUTH;
		q.path = authgen;
		q.vers = 0;
		return q;
	}

	q.path = f->d.id;
	q.vers = f->d.mtime;
	q.type = 0;
	if(f->d.mode&DMDIR)
		q.type |= QTDIR;
	if(f->d.mode&DMTMP)
		q.type |= QTTMP;
	if(f->d.mode&DMAPPEND)
		q.type |= QTAPPEND;
	if(f->d.mode&DMEXCL)
		q.type |= QTEXCL;
	if((q.type&QTEXCL) == 0)
		q.type |= QTCACHE;
	return q;
}

static void
rversion(Rpc *rpc)
{
	rpc->r.msize = rpc->t.msize;
	if(rpc->r.msize > Maxmdata)
		rpc->r.msize = Maxmdata;
	rpc->cli->msize = rpc->r.msize;
	if(strncmp(rpc->t.version, "9P2000", 6) != 0)
		error("unknown protocol version");
	rpc->r.version = "9P2000";
}

/*
 * Served in the main client process.
 */
static void
rflush(Rpc *rpc)
{
	Cli *cli;
	Rpc *r;

	cli = rpc->cli;
	xqlock(&cli->wlk);	/* nobody replies now */
	xqlock(&rpc->cli->rpclk);
	for(r = rpc->cli->rpcs; r != nil; r = r->next)
		if(r->t.tag == rpc->t.oldtag)
			break;
	if(r != nil){
		r->flushed = 1;
		if(r->t.type == Tread && r->fid->consopen)
			consprint("");	/* in case it's waiting... */
	}
	xqunlock(&rpc->cli->rpclk);
	xqunlock(&cli->wlk);
}

static void
rauth(Rpc *rpc)
{
	Fid *fid;
	static char spec[] = "proto=p9any role=server";

	if(noauth)
		error("no auth required");

	fid = newfid(rpc->cli, rpc->t.afid);
	rpc->fid = fid;

	setfiduid(fid, rpc->t.uname);

	fid->omode = ORDWR;
	fid->afd = open("/mnt/factotum/rpc", ORDWR);
	if(fid->afd < 0)
		error("factotum: %r");
	fid->rpc = auth_allocrpc(fid->afd);
	if(fid->rpc == nil){
		close(fid->afd);
		error("auth rpc: %r");
	}
	if(auth_rpc(fid->rpc, "start", spec, strlen(spec)) != ARok){
		auth_freerpc(fid->rpc);
		close(fid->afd);
		error("auth_rpc start failed");
	}
	rpc->r.qid = mkqid(nil);
	d9print("factotum rpc started\n");
}

static long
xauthread(Fid *fid, long count, void *data)
{
	AuthInfo *ai;

	switch(auth_rpc(fid->rpc, "read", nil, 0)){
	case ARdone:
		ai = auth_getinfo(fid->rpc);
		if(ai == nil)
			error("authread: info: %r");
		auth_freeAI(ai);
		d9print("auth: %s: ok\n", usrname(fid->uid));
		fid->authok = 1;
		return 0;
	case ARok:
		if(count < fid->rpc->narg)
			error("authread: count too small");
		count = fid->rpc->narg;
		memmove(data, fid->rpc->arg, count);
		return count;
	}
	error("authread: phase error");
	return -1;
}

static void
rattach(Rpc *rpc)
{
	Fid *fid, *afid;
	Path *p;
	Memblk *f;
	char buf[ERRMAX];

	fid = newfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	afid = nil;
	if(!noauth){
		afid = getfid(rpc->cli, rpc->t.afid);
		if(catcherror()){
			putfid(afid);
			error(nil);
		}
		if(afid->rpc == nil)
			error("afid is not an auth fid");
		if(afid->authok == 0)
			xauthread(afid, 0, buf);
	}
	fidattach(fid, rpc->t.aname, rpc->t.uname);
	if(!noauth){
		if(fid->uid != afid->uid)
			error("auth uid mismatch");
		noerror();
		putfid(afid);
	}
	p = fid->p;
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	rpc->r.qid = mkqid(f);
	rwunlock(f, Rd);

	if(rpc->cli->uid == -1)
		rpc->cli->uid = rpc->fid->uid;
}

static void
rwalk(Rpc *rpc)
{
	Fid *fid, *nfid;
	Path *p;
	Memblk *nf;
	int i;

	rpc->fid = getfid(rpc->cli, rpc->t.fid);
	fid = rpc->fid;
	if(rpc->t.fid == rpc->t.newfid && rpc->t.nwname > 1)
		error("can't walk like a clone without one");
	nfid = nil;
	if(rpc->t.fid != rpc->t.newfid)
		nfid = fidclone(rpc->cli, rpc->fid, rpc->t.newfid);
	if(catcherror()){
		putfid(nfid);
		putfid(nfid);		/* clunk */
		error(nil);
	}
	rpc->r.nwqid = 0;
	for(i=0; i < rpc->t.nwname; i++){
		if(catcherror()){
			if(rpc->r.nwqid == 0)
				error(nil);
			break;
		}
		fidwalk(nfid, rpc->t.wname[i]);
		noerror();
		p = nfid->p;
		nf = p->f[p->nf-1];
		rwlock(nf, Rd);
		rpc->r.wqid[i] = mkqid(nf);
		rwunlock(nf, Rd);
		rpc->r.nwqid++;
		USED(rpc->r.nwqid);	/* damn error()s */
	}
	if(i < rpc->t.nwname){
		putfid(nfid);
		putfid(nfid);		/* clunk */
	}else{
		putfid(fid);
		rpc->fid = nfid;
	}
	noerror();
}

static void
ropen(Rpc *rpc)
{
	Fid *fid;
	Memblk *f;

	rpc->fid = getfid(rpc->cli, rpc->t.fid);
	fid = rpc->fid;

	if(fid->rpc != nil)	/* auth fids are always open */
		return;

	rpc->r.iounit = rpc->cli->msize - IOHDRSZ;
	fidopen(rpc->fid, rpc->t.mode);
	f = fid->p->f[fid->p->nf-1];
	rwlock(f, Rd);
	rpc->r.qid = mkqid(f);
	rwunlock(f, Rd);
}

static void
rcreate(Rpc *rpc)
{
	Fid *fid;
	Path *p;
	Memblk *f;

	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil)
		error("create on auth fid");

	fidcreate(fid, rpc->t.name, rpc->t.mode, rpc->t.perm);
	p = fid->p;
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	rpc->r.qid = mkqid(f);
	rwunlock(f, Rd);
	rpc->r.iounit = rpc->cli->msize-IOHDRSZ;
}

static ulong
pack9dir(Memblk *f, uchar *buf, int nbuf)
{
	Dir d;

	nulldir(&d);
	d.name = f->mf->name;
	d.qid = mkqid(f);
	d.mode = f->d.mode;
	d.length = f->d.length;
	if(d.mode&DMDIR)
		d.length = 0;
	d.uid = f->mf->uid;
	d.gid = f->mf->gid;
	d.muid = f->mf->muid;
	d.atime = f->d.atime;
	d.mtime = f->d.mtime / NSPERSEC;
	return convD2M(&d, buf, nbuf);
}

static void
authread(Rpc *rpc)
{
	Fid *fid;

	fid = rpc->fid;
	if(fid->rpc == nil)
		error("authread: not an auth fid");
	rpc->r.data = (char*)rpc->data;
	rpc->r.count = xauthread(fid, rpc->t.count, rpc->r.data);
	putfid(fid);
	rpc->fid = nil;
}

static void
rread(Rpc *rpc)
{
	Fid *fid;
	vlong off;

	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil){
		authread(rpc);
		return;
	}
	if(rpc->t.count > rpc->cli->msize-IOHDRSZ)
		rpc->r.count = rpc->cli->msize-IOHDRSZ;
	rpc->r.data = (char*)rpc->data;
	off = rpc->t.offset;
	rpc->r.count = fidread(fid, rpc->r.data, rpc->t.count, off, pack9dir);

}

static void
authwrite(Rpc *rpc)
{
	Fid *fid;

	fid = rpc->fid;
	if(fid->rpc == nil)
		error("authwrite: not an auth fid");
	if(auth_rpc(fid->rpc, "write", rpc->t.data, rpc->t.count) != ARok)
		error("authwrite: %r");
	rpc->r.count = rpc->t.count;
	putfid(fid);
	rpc->fid = nil;
}

static void
rwrite(Rpc *rpc)
{
	Fid *fid;
	uvlong off;

	if(rpc->t.offset < 0)
		error("negative offset");
	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil){
		authwrite(rpc);
		return;
	}
	off = rpc->t.offset;
	rpc->r.count = fidwrite(fid, rpc->t.data, rpc->t.count, &off);
}

static void
rclunk(Rpc *rpc)
{
	Fid *fid;

	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil){
		fid->omode = -1;
		if(fid->rpc != nil)
			auth_freerpc(fid->rpc);
		fid->rpc = nil;
		close(fid->afd);
		fid->afd = -1;
	}else if(fid->omode != -1)
		fidclose(fid);
	putfid(fid);
	putfid(fid);
	rpc->fid = nil;
}

static void
rremove(Rpc *rpc)
{
	Fid *fid;

	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil)
		error("remove on auth fid");
	if(catcherror()){
		dEprint("clunking %X:\n\t%r\n", fid);
		putfid(fid);
		putfid(fid);
		rpc->fid = nil;
		error(nil);
	}

	fidremove(fid);
	noerror();
	dEprint("clunking %X\n\n", fid);
	putfid(fid);
	putfid(fid);
	rpc->fid = nil;
}

static void
rstat(Rpc *rpc)
{
	Fid *fid;
	Memblk *f;
	Path *p;

	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil)
		error("stat on auth fid");
	xqlock(fid);
	if(catcherror()){
		xqunlock(fid);
		error(nil);
	}
	p = dflast(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	noerror();
	xqunlock(fid);
	if(catcherror()){
		rwunlock(f, Rd);
		error(nil);
	}
	rpc->r.stat = rpc->data;
	rpc->r.nstat = pack9dir(f, rpc->data, rpc->cli->msize-IOHDRSZ);
	if(rpc->r.nstat <= 2)
		fatal("rstat: convD2M");
	noerror();
	rwunlock(f, Rd);
}

static void
wstatint(Memblk *f, char *name, u64int v)
{
	dfwattr(f, name, &v, sizeof v);
}

static void
wstatstr(Memblk *f, char *name, char *s)
{
	dfwattr(f, name, s, strlen(s)+1);
}

static void
rwstat(Rpc *rpc)
{
	Fid *fid;
	Memblk *f;
	Path *p;
	Dir sd;
	u64int n;

	n = convM2D(rpc->t.stat, rpc->t.nstat, &sd, (char*)rpc->t.stat);
	if(n != rpc->t.nstat)
		error("convM2D: bad stat");
	fid = getfid(rpc->cli, rpc->t.fid);
	rpc->fid = fid;
	if(fid->rpc != nil)
		error("wstat on auth fid");
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
		error("can't wstat archived or built-in files");
	p = dfmelt(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	noerror();
	xqunlock(fid);
	if(catcherror()){
		rwunlock(f, Wr);
		error(nil);
	}

	if(f->d.mode&DMUSERS)
		error("can't wstat the users file");
	if(sd.length != ~0 && sd.length != f->d.length){
		if(f->d.mode&DMDIR)
			error("can't resize a directory");
		if(sd.length != 0)
			error("can't truncate to non-zero length");
		dfaccessok(f, fid->uid, AWRITE);
	}else
		sd.length = ~0;

	if(sd.name[0] && strcmp(f->mf->name, sd.name) != 0){
		if(isro(f) || f == fs->active)
			error("can't rename built-in files");
		dfaccessok(p->f[p->nf-2], fid->uid, AWRITE);
		if(!catcherror()){
			mbput(dfwalk(f, sd.name, Rd));
			error("file already exists");
		}
	}else
		sd.name[0] = 0;

	if(sd.uid[0] != 0 && strcmp(sd.uid, f->mf->uid) != 0){
		if(!allowed(f->d.uid)){
			if(fid->uid != f->d.uid && !leader(f->d.gid, fid->uid))
				error("not the owner or group leader");
			if(!member(usrid(sd.uid), fid->uid) != 0)
				error("you are not a member");
		}
	}else
		sd.uid[0] = 0;

	if(sd.gid[0] != 0 && strcmp(sd.gid, f->mf->gid) != 0){
		/*
		 * Not std. in 9: leader must be member of the new gid, not
		 * leader of the new gid.
		 */
		if(!allowed(f->d.uid)){
			if(fid->uid != f->d.uid && !leader(f->d.gid, fid->uid))
				error("not the owner or group leader");
			if(!member(usrid(sd.gid), fid->uid) != 0)
				error("you are not a member");
		}
	}else
		sd.gid[0] = 0;

	/*
	 * Not std. in 9: muid can be updated if uid is allowed, it's
	 * ignored otherwise.
	 */
	if(sd.muid[0] != 0 && strcmp(sd.muid, f->mf->muid) != 0){
		if(!allowed(f->d.uid))
			sd.muid[0] = 0;
	}else
		sd.muid[0] = 0;

	if(sd.mode != ~0 && f->d.mode != sd.mode){
		if((sd.mode&DMBITS) != sd.mode)
			error("unknown bit set in mode");
		if(!allowed(f->d.uid))
			if(fid->uid != f->d.uid && !leader(f->d.gid, fid->uid))
				error("not the owner or group leader");
		if((sd.mode&DMDIR) ^ (f->d.mode&DMDIR))
			error("attempt to change DMDIR");
	}else
		sd.mode = ~0;

	/*
	 * Not std. in 9: allowed users can also set atime.
	 */
	if(sd.atime != ~0 && f->d.atime != sd.atime){
		if(!allowed(f->d.uid))
			sd.atime = ~0;		/* ignore it */
	}else
		sd.atime = ~0;

	if(sd.mtime != ~0 && f->d.mtime != sd.mtime){
		if(!allowed(f->d.uid))
			if(fid->uid != f->d.uid && !leader(f->d.gid, fid->uid))
				error("not the owner or group leader");
	}else
		sd.mtime = ~0;

	/*
	 * Not std. in 9: other non-null fields, if any, are ignored.
	 */
	if(sd.length != ~0)
		wstatint(f, "length", sd.length);
	if(sd.name[0])
		wstatstr(f, "name", sd.name);
	if(sd.uid[0])
		wstatstr(f, "uid", sd.uid);
	if(sd.gid[0])
		wstatstr(f, "gid", sd.gid);
	if(sd.muid[0])
		wstatstr(f, "muid", sd.muid);
	if(sd.mode != ~0)
		wstatint(f, "mode", sd.mode);
	if(sd.atime != ~0)
		wstatint(f, "atime", sd.atime);
	if(sd.mtime != ~0)
		wstatint(f, "mtime", sd.mtime);

	noerror();
	rwunlock(f, Wr);
}

static char*
rpcworker9p(void *v, void**aux)
{
	Rpc *rpc;
	Cli *cli;
	char err[128];
	long n;
	int nerr;
	Memblk *fahead;

	rpc = v;
	cli = rpc->cli;
	threadsetname("rpcworker9p %s %R", cli->addr, rpc);
	dPprint("%s starting\n", threadgetname());

	if(*aux == nil){
		errinit(Errstack);
		*aux = v;		/* make it not nil */
	}
	nerr = nerrors();


	if(fsmemfree() < Mzerofree || fsdiskfree() < Dzerofree)
		fspolicy();


	rpc->r.tag = rpc->t.tag;
	rpc->r.type = rpc->t.type + 1;

	quiescent(No);
	if(catcherror()){
		quiescent(Yes);
		rpc->r.type = Rerror;
		rpc->r.ename = err;
		rerrstr(err, sizeof err);
	}else{
		fcalls[rpc->t.type](rpc);
		quiescent(Yes);	
		noerror();
	}

	xqlock(&cli->wlk);
	fahead = nil;
	if(rpc->fid != nil && rpc->fid->p != nil)
		if(rpc->r.type == Rread || rpc->r.type == Rwalk){
			fahead = rpc->fid->p->f[rpc->fid->p->nf - 1];
			incref(fahead);
		}
	if(catcherror()){
		mbput(fahead);
		error(nil);
	}

	putfid(rpc->fid);	/* release rpc fid before replying */
	rpc->fid = nil;		/* or we might get "fid in use" errors */

	if(rpc->flushed == 0){
		d9print("-> %F\n", &rpc->r);
		n = convS2M(&rpc->r, cli->wdata, sizeof cli->wdata);
		if(n == 0)
			fatal("rpcworker: convS2M");
		if(write(cli->fd, cli->wdata, n) != n)
			d9print("%s: %r\n", cli->addr);
	}else
		dprint("flushed: %F\n", &rpc->r);
	calltime[rpc->t.type] += nsec() - rpc->t0;
	ncalls[rpc->t.type]++;
	xqunlock(&cli->wlk);

	if(fahead != nil){
		if(rpc->r.type == Rread)
			rahead(fahead, rpc->t.offset + rpc->r.count);
		else if(rpc->r.type == Rwalk)
			wahead(fahead);
		mbput(fahead);
	}
	noerror();

	replied(rpc);
	freerpc(rpc);

	fspolicy();

	dPprint("%s exiting\n", threadgetname());

	if(nerrors() != nerr)
		fatal("%s: unbalanced error stack", threadgetname());
	return nil;
}

char*
cliworker9p(void *v, void**aux)
{
	Cli *cli;
	long n;
	Rpc *rpc;

	cli = v;
	threadsetname("cliworker9p %s", cli->addr);
	dPprint("%s started\n", threadgetname());
	if(*aux == nil){
		errinit(Errstack);
		*aux = v;		/* make it not nil */
	}

	if(catcherror())
		fatal("worker: uncatched: %r");

	rpc = nil;
	for(;;){
		if(dbg['E'])
			dumpfids();
		if(rpc == nil)
			rpc = newrpc();
		n = read9pmsg(cli->fd, rpc->data, Maxmdata+IOHDRSZ);
		if(n < 0){
			d9print("%s: read: %r\n", cli->addr);
			break;
		}
		if(n == 0)
			continue;
		rpc->t0 = nsec();
		if(convM2S(rpc->data, n, &rpc->t) == 0){
			d9print("%s: convM2S failed\n", cli->addr);
			continue;
		}
		if(rpc->t.type >= nelem(fcalls) || fcalls[rpc->t.type] == nil){
			d9print("%s: bad fcall type %d\n", cli->addr, rpc->t.type);
			continue;
		}
		d9print("<-%F\n", &rpc->t);
		rpc->cli = cli;
		incref(cli);

		xqlock(&cli->rpclk);
		rpc->next = cli->rpcs;
		cli->rpcs = rpc;
		cli->nrpcs++;
		xqunlock(&cli->rpclk);

		if(rpc->t.type == Tflush ||
		   (Rpcspercli != 0 && cli->nrpcs >= Rpcspercli))
			rpcworker9p(rpc, aux);
		else
			getworker(rpcworker9p, rpc, nil);
		rpc = nil;
	}
	putcli(cli);
	noerror();
	dPprint("%s exiting\n", threadgetname());
	return nil;
};
