#include "all.h"

/*
 * ix server for creepy
 */

enum
{
	Tfirst =	0x8000U,
	Tlast =	0x4000U,
	Tmask = 0x3FFFU
};

static void	rversion(Rpc*), rfid(Rpc*), rclone(Rpc*),
		rattach(Rpc*), rwalk(Rpc*),
		ropen(Rpc*), rcreate(Rpc*),
		rread(Rpc*), rwrite(Rpc*), rclunk(Rpc*),
		rremove(Rpc*), rattr(Rpc*), rwattr(Rpc*),
		rcond(Rpc*), rmove(Rpc*);

static int	reply(Rpc*);

static void (*ixcalls[])(Rpc*) =
{
	[IXTversion]		rversion,
	[IXTattach]		rattach,
	[IXTfid]			rfid,
	[IXTclone]		rclone,
	[IXTwalk]		rwalk,
	[IXTopen]		ropen,
	[IXTcreate]		rcreate,
	[IXTread]		rread,
	[IXTwrite]		rwrite,
	[IXTclunk]		rclunk,
	[IXTremove]		rremove,
	[IXTattr]		rattr,
	[IXTwattr]		rwattr,
	[IXTcond]		rcond,
	[IXTmove]		rmove,
};

/*
 * we consider T/R msgs that include uids, errors, and attributes
 * as short. That places a limit on things like user names, element
 * names, and error messages. The limit is lower than Minmdata.
 * Declaring them here as large requests will remove those limits.
 * For large messages, the request buffer is used instead of the
 * per-client write buffer, and data is copied by the rpc code,
 * to save an extra copy.
 */
static int largeix[IXTmax] =
{
	[IXTread]	1,	/* uses its buf for the reply */
	[IXTwrite]	1,
};

Alloc srpcalloc =
{
	.elsz = sizeof(Shortrpc),
	.zeroing = 0,
};

static int ixrreadhdrsz;

char*
ixstats(char *s, char *e, int clr)
{
	int i;

	s = seprint(s, e, "srpcs:\t%4uld alloc %4uld free (%4uld bytes)\n",
		srpcalloc.nalloc, srpcalloc.nfree, srpcalloc.elsz);
	for(i = 0; i < nelem(ixcalls); i++)
		if(ixcalls[i] != nil && ncalls[i] > 0){
			s = seprint(s, e, "%-8s\t%5uld calls\t%11ulld µs\n",
				callname[i], ncalls[i],
				(calltime[i]/ncalls[i])/1000);
			if(clr){
				ncalls[i] = 0;
				calltime[i] = 0;
			}
		}
	return s;
}


static Rpc*
newsrpc(void)
{
	Rpc *rpc;

	rpc = anew(&srpcalloc);
	rpc->next = nil;
	rpc->cli = nil;
	rpc->fid = nil;
	rpc->flushed = 0;
	rpc->closed = 0;
	rpc->chan = ~0;
	rpc->rpc0 = nil;
	memset(&rpc->xt, 0, sizeof rpc->xt);
	memset(&rpc->xr, 0, sizeof rpc->xr);
	return rpc;	
}

static void
freesrpc(Rpc *rpc)
{
	afree(&srpcalloc, rpc);
}

static void
freeixrpc(Rpc *rpc)
{
	rpc->closed = 0;
	rpc->flushed = 0;
	if(largeix[rpc->xt.type])
		freerpc(rpc);
	else
		freesrpc(rpc);
}

static void
rversion(Rpc *rpc)
{
	rpc->xr.msize = rpc->xt.msize;
	if(rpc->xr.msize > Maxmdata)
		rpc->xr.msize = Maxmdata;
	rpc->cli->msize = rpc->xr.msize;
	if(strncmp(rpc->xt.version, "IX", 2) != 0)
		error("unknown protocol version");
	rpc->xr.version = "IX";
}

static void
rattach(Rpc *rpc)
{
	putfid(rpc->fid);
	rpc->rpc0->fid = newfid(rpc->cli, -1);
	fidattach(rpc->rpc0->fid, rpc->xt.aname,  rpc->xt.uname);
}

static void
rfid(Rpc *rpc)
{

	putfid(rpc->rpc0->fid);
	rpc->rpc0->fid = getfid(rpc->cli, rpc->xt.fid);
}

static void
rclone(Rpc *rpc)
{
	Fid *nfid;

	if(rpc->rpc0->fid == nil)
		error("fid not set");
	nfid = fidclone(rpc->cli, rpc->rpc0->fid, -1);
	putfid(rpc->rpc0->fid);
	rpc->rpc0->fid = nfid;
	nfid->cflags = rpc->xt.cflags;
}

static void
rwalk(Rpc *rpc)
{
	if(rpc->rpc0->fid == nil)
		error("fid not set");
	fidwalk(rpc->rpc0->fid, rpc->xt.wname);
}

static void
ropen(Rpc *rpc)
{
	int cflags;

	if(rpc->rpc0->fid == nil)
		error("fid not set");
	cflags = rpc->xt.mode&(OCERR|OCEND);
	fidopen(rpc->rpc0->fid, rpc->xt.mode &~cflags);
	rpc->rpc0->fid->cflags = cflags;
}

static void
rcreate(Rpc *rpc)
{
	int cflags;

	if(rpc->rpc0->fid == nil)
		error("fid not set");
	cflags = rpc->xt.mode&(OCERR|OCEND);
	fidcreate(rpc->rpc0->fid, rpc->xt.name, rpc->xt.mode, rpc->xt.perm);
	rpc->rpc0->fid->cflags = cflags;
}

static ulong
pixd(Memblk *f, uchar *buf, int nbuf)
{
	ulong n;

	if(nbuf < BIT32SZ)
		return 0;
	if(catcherror())
		return 0;
	n = pmeta(buf+BIT32SZ, nbuf-BIT32SZ, f);
	noerror();
	PBIT32(buf, n);
	return n+BIT32SZ;
}

static void
rread(Rpc *rpc)
{
	vlong off;
	Fid *fid;
	int nmsg;

	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	if(rpc->xt.count > rpc->cli->msize-ixrreadhdrsz)
		rpc->xt.count = rpc->cli->msize-ixrreadhdrsz;
	rpc->xr.data = rpc->data + ixrreadhdrsz;

	/*
	 * send all but the last reply, if we are given permissiong to
	 * send multiple replies back.
	 * Errors, eof, and flush terminate the sequence.
	 * As usual, the caller sends the last reply when we return.
	 */
	off = rpc->xt.offset;
	nmsg = rpc->xt.nmsg;
	for(;;){
		rpc->xr.count = fidread(fid, rpc->xr.data, rpc->xt.count, off, pixd);
		if(rpc->xr.count == 0)
			break;
		if(nmsg-- <= 0)
			break;
		if(reply(rpc) < 0)
			break;
		if(rpc != rpc->rpc0)
			freeixrpc(rpc);
		off += rpc->xr.count;
	}
}

static void
rwrite(Rpc *rpc)
{
	Fid *fid;

	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	rpc->xr.offset = rpc->xt.offset;
	rpc->xr.count = fidwrite(fid, rpc->xt.data, rpc->xt.count, &rpc->xr.offset);
}

static void
rclunk(Rpc *rpc)
{
	Fid *fid;

	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	if(fid->omode != -1)
		fidclose(fid);
	fid->cflags = 0;
	putfid(fid);
	putfid(fid);
	rpc->rpc0->fid = nil;
}

static void
rremove(Rpc *rpc)
{
	Fid *fid;

	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	fidremove(fid);
	fid->cflags = 0;
	putfid(fid);
	putfid(fid);
	rpc->rpc0->fid = nil;
}

static void
rattr(Rpc *rpc)
{
	Fid *fid;
	Path *p;
	Memblk *f;
	uchar buf[128];

	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	p = dflast(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	if(catcherror()){
		rwunlock(f, Rd);
		error(nil);
	}
	rpc->xr.value = buf;
	rpc->xr.nvalue = dfrattr(f, rpc->xt.attr, buf, sizeof buf);
	rwunlock(f, Rd);
	noerror();
}

static void
rwattr(Rpc *rpc)
{
	Fid *fid;
	Path *p;
	Memblk *f;

	/*
	 * XXX: add checks like done in wstat().
	 * this code is incomplete.
	 */
	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	p = fid->p;
	f = p->f[p->nf-1];
	if(writedenied(fid->uid))
		error("user can't write");
	if(isro(f) || fid->archived)
		error("can't wattr archived or built-in files");
	p = dfmelt(&fid->p, fid->p->nf);
	f = p->f[p->nf-1];
	if(catcherror()){
		rwunlock(f, Wr);
		error(nil);
	}
	dfwattr(f, rpc->xt.attr, rpc->xt.value, rpc->xt.nvalue);
	noerror();
	rwunlock(f, Wr);
}

static void
rcond(Rpc *rpc)
{
	Fid *fid;
	Path *p;
	Memblk *f;

	fid = rpc->rpc0->fid;
	if(fid == nil)
		error("fid not set");
	p = fid->p;
	f = p->f[p->nf-1];
	rwlock(f, Rd);
	if(catcherror()){
		rwunlock(f, Rd);
		error(nil);
	}
	dfcattr(f, rpc->xt.op, rpc->xt.attr, rpc->xt.value, rpc->xt.nvalue);
	noerror();
	rwunlock(f, Rd);
}

static void
rmove(Rpc *rpc)
{
	if(rpc->rpc0->fid == nil)
		error("fid not set");
	error("move not yet implemented");
}

/*
 * Read a short or large rpc and return it.
 * Shouldn't we use bio, or at least a buffer?
 */
static Rpc*
readix(int fd)
{
	uchar hdr[BIT16SZ+BIT16SZ+BIT8SZ];
	long nhdr, nr;
	ulong sz;
	uint type;
	Rpc *rpc;

	nhdr = readn(fd, hdr, sizeof hdr);
	if(nhdr < 0){
		dXprint("readix: %r\n");
		return nil;
	}
	if(nhdr == 0){
		werrstr("eof");
		return nil;
	}
	sz = GBIT16(hdr);
	if(sz > IOHDRSZ+Maxmdata){
		/* don't read it; the entire stream will fail */
		werrstr("msg too large");
		return nil;
	}
	if(sz < BIT16SZ+BIT8SZ){
		/* don't read it; the entire stream will fail */
		werrstr("msg too small");
		return nil;
	}
	type = GBIT8(hdr+BIT16SZ+BIT16SZ);
	if(type >= IXTmax){
		werrstr("wrong message type");
		rpc = newrpc();
	}else if(largeix[type])
		rpc = newrpc();
	else
		rpc = newsrpc();
	rpc->chan = GBIT16(hdr+BIT16SZ);
	rpc->xt.type = type;
	PBIT8(rpc->data, type);
	nr = readn(fd, rpc->data+BIT8SZ, sz-(BIT16SZ+BIT8SZ));
	if(nr < 0){
		freeixrpc(rpc);
		return nil;
	}
	if(nr != sz){
		werrstr("short msg data");
		freeixrpc(rpc);
		return nil;
	}
	rpc->t0 = nsec();
	if(ixunpack(rpc->data, sz-BIT16SZ, &rpc->xt) != sz-BIT16SZ){
		freeixrpc(rpc);
		return nil;
	}
	return rpc;
}

static int
reply(Rpc *rpc)
{
	ulong sz, max;
	uchar *p, *buf;
	Cli *cli;
	u16int chan;

	cli = rpc->cli;
	chan = rpc->chan&Tmask;
	if(rpc->xr.type == IXRerror || (rpc->chan&Tlast) != 0)
		chan |= Tlast;
	xqlock(&cli->wlk);
	if(largeix[rpc->xt.type])
		buf = rpc->data;
	else
		buf = cli->wdata;
	max = IOHDRSZ+Maxmdata;
	p = buf;
	p += BIT16SZ;
	PBIT16(p, chan);
	p += BIT16SZ;
	sz = ixpack(&rpc->xr, p, max-BIT16SZ-BIT16SZ);
	if(sz == 0)
		fatal("writeix: message too large or ixpack failed");
	PBIT16(buf, sz);
	p += sz;

	if(rpc->rpc0->flushed){
		xqunlock(&cli->wlk);
		werrstr("flushed");
		dXprint("write: flushed");
		return -1;
	}
	if(chan&Tlast){
		putfid(rpc->rpc0->fid);	/* release rpc fid before replying */
		rpc->rpc0->fid = nil;	/* or we might get "fid in use" errors */
	}
	dXprint("-> %G\n", &rpc->xr);
	if(write(cli->fd, buf, p-buf) != p-buf){
		xqunlock(&cli->wlk);
		dXprint("write: %r");
		return -1;
	}
	calltime[rpc->xt.type] += nsec() - rpc->t0;
	ncalls[rpc->xt.type]++;
	xqunlock(&cli->wlk);
	return p-buf;
}

static char*
rpcworkerix(void *v, void**aux)
{
	Rpc *rpc, *rpc0;
	Cli *cli;
	Channel *c;
	char err[128];
	long nw;
	int nerr;
	Memblk *fahead;

	c = v;
	if(*aux == nil){
		errinit(Errstack);
		*aux = v;		/* make it not nil */
	}

	err[0] = 0;
	rpc = recvp(c);
	rpc0 = rpc;
	cli = rpc->cli;
	threadsetname("rpcworkerix %s chan %d", cli->addr, rpc0->chan);
	dPprint("%s started\n", threadgetname());

	do{
		if(fsmemfree() < Mzerofree || fsdiskfree() < Dzerofree)
			fspolicy();

		nerr = nerrors();
		rpc->xr.type = rpc->xt.type + 1;
		rpc->rpc0 = rpc0;
		quiescent(No);
		if(catcherror()){
			quiescent(Yes);
			rpc->xr.type = Rerror;
			rpc->xr.ename = err;
			rerrstr(err, sizeof err);
			if(rpc0->fid != nil && (rpc0->fid->cflags&OCERR) != 0)
				rpc0->fid->cflags |= OCEND;
		}else{
			ixcalls[rpc->xt.type](rpc);
			quiescent(Yes);
			noerror();
		}

		fahead = nil;
		if(rpc0->fid != nil && rpc0->fid->p != nil)
			if(rpc->xr.type == IXRread || rpc->xr.type == IXRwalk){
				fahead = rpc0->fid->p->f[rpc0->fid->p->nf - 1];
				incref(fahead);
			}
		if(catcherror()){
			mbput(fahead);
			error(nil);
		}

		nw = reply(rpc);

		if(fahead != nil){
			if(rpc->xr.type == IXRread && rpc->xt.nmsg <= 1)
				rahead(fahead, rpc->xt.offset + rpc->xr.count);
			else if(rpc->xr.type == IXRwalk)
				wahead(fahead);
			mbput(fahead);
		}
		noerror();

		if(rpc != rpc0)
			freeixrpc(rpc);
		if(nerrors() != nerr)
			fatal("%s: unbalanced error stack", threadgetname());
	}while(!rpc0->closed && nw > 0 && err[0] == 0 && (rpc = recvp(c)) != nil);

	while((rpc = nbrecvp(c)) != nil)
		freeixrpc(rpc);
	replied(rpc0);
	freeixrpc(rpc0);

	fspolicy();

	dPprint("%s exiting\n", threadgetname());
	return nil;
}

static void
ixinit(void)
{
	IXcall xt;
	if(ixrreadhdrsz != 0)
		return;
	xt.type = IXRread;
	ixrreadhdrsz = ixpackedsize(&xt) + BIT16SZ + BIT16SZ;
}

static char*
cliworkerix(void *v, void**aux)
{
	Cli *cli;
	Rpc *rpc, *r;

	cli = v;
	threadsetname("cliworkerix %s", cli->addr);
	dPprint("%s started\n", threadgetname());

	ixinit();
	for(;;){
		if(dbg['E'])
			dumpfids();
	loop:	rpc = readix(cli->fd);
		if(rpc == nil){
			dXprint("%s: read: %r\n", cli->addr);
			break;
		}
		rpc->cli = cli;
		incref(cli);

		xqlock(&cli->rpclk);
		for(r = cli->rpcs; r != nil; r = r->next)
			if((r->chan&Tmask) == (rpc->chan&Tmask)){
				if(rpc->chan&Tlast)
					if(r->closed)
						r->flushed = 1;
					else
						r->closed = 1;
				sendp(r->c, rpc);
				xqunlock(&cli->rpclk);
				goto loop;
			}
		if((rpc->chan&Tfirst) == 0){	/* it's channel is gone */
			freeixrpc(rpc);
			xqunlock(&cli->rpclk);
			goto loop;
		}

		/* new channel */
		rpc->next = cli->rpcs;
		cli->rpcs = rpc;
		if(rpc->c == nil)
			rpc->c = chancreate(sizeof(Rpc*), 64);
		cli->nrpcs++;
		xqunlock(&cli->rpclk);

		if(rpc->chan&Tlast)
			rpc->closed = 1;
		sendp(rpc->c, rpc);
		if(Rpcspercli != 0 && cli->nrpcs >= Rpcspercli)
			rpcworkerix(rpc->c, aux);
		else
			getworker(rpcworkerix, rpc->c, nil);
	}
	putcli(cli);
	dPprint("%s exiting\n", threadgetname());
	return nil;
}
