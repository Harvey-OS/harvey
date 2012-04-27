#include <u.h>
#include <libc.h>
#include <thread.h>
#include <sunrpc.h>

/*
 * Sun RPC server; for now, no reply cache
 */

static void sunRpcProc(void*);
static void sunRpcRequestThread(void*);
static void sunRpcReplyThread(void*);
static void sunRpcForkThread(void*);
static SunProg *sunFindProg(SunSrv*, SunMsg*, SunRpc*, Channel**);

typedef struct Targ Targ;
struct Targ
{
	void (*fn)(void*);
	void *arg;
};

SunSrv*
sunSrv(void)
{
	SunSrv *srv;

	srv = emalloc(sizeof(SunSrv));
	srv->chatty = 0;
	srv->crequest = chancreate(sizeof(SunMsg*), 16);
	srv->creply = chancreate(sizeof(SunMsg*), 16);
	srv->cthread = chancreate(sizeof(Targ), 4);

	proccreate(sunRpcProc, srv, SunStackSize);
	return srv;
}

void
sunSrvProg(SunSrv *srv, SunProg *prog, Channel *c)
{
	if(srv->nprog%16 == 0){
		srv->prog = erealloc(srv->prog, (srv->nprog+16)*sizeof(srv->prog[0]));
		srv->cdispatch = erealloc(srv->cdispatch, (srv->nprog+16)*sizeof(srv->cdispatch[0]));
	}
	srv->prog[srv->nprog] = prog;
	srv->cdispatch[srv->nprog] = c;
	srv->nprog++;
}

static void
sunRpcProc(void *v)
{
	threadcreate(sunRpcReplyThread, v, SunStackSize);
	threadcreate(sunRpcRequestThread, v, SunStackSize);
	threadcreate(sunRpcForkThread, v, SunStackSize);

}

static void
sunRpcForkThread(void *v)
{
	SunSrv *srv = v;
	Targ t;

	while(recv(srv->cthread, &t) == 1)
		threadcreate(t.fn, t.arg, SunStackSize);
}

void
sunSrvThreadCreate(SunSrv *srv, void (*fn)(void*), void *arg)
{
	Targ t;

	t.fn = fn;
	t.arg = arg;
	send(srv->cthread, &t);
}

static void
sunRpcRequestThread(void *v)
{
	uchar *p, *ep;
	Channel *c;
	SunSrv *srv = v;
	SunMsg *m;
	SunProg *pg;
	SunStatus ok;

	while((m = recvp(srv->crequest)) != nil){
		/* could look up in cache here? */

if(srv->chatty) fprint(2, "sun msg %p count %d\n", m, m->count);
		m->srv = srv;
		p = m->data;
		ep = p+m->count;
		if(sunRpcUnpack(p, ep, &p, &m->rpc) != SunSuccess){
			fprint(2, "in: %.*H unpack failed\n", m->count, m->data);
			sunMsgDrop(m);
			continue;
		}
		if(srv->chatty)
			fprint(2, "in: %B\n", &m->rpc);

		if(srv->alwaysReject){
			if(srv->chatty)
				fprint(2, "\trejecting\n");
			sunMsgReplyError(m, SunAuthTooWeak);
			continue;
		}

		if(!m->rpc.iscall){
			sunMsgReplyError(m, SunGarbageArgs);
			continue;
		}

		if((pg = sunFindProg(srv, m, &m->rpc, &c)) == nil){
			/* sunFindProg sent error */
			continue;
		}

		p = m->rpc.data;
		ep = p+m->rpc.ndata;
		m->call = nil;
		if((ok = sunCallUnpackAlloc(pg, m->rpc.proc<<1, p, ep, &p, &m->call)) != SunSuccess){
			sunMsgReplyError(m, ok);
			continue;
		}
		m->call->rpc = m->rpc;

		if(srv->chatty)
			fprint(2, "\t%C\n", m->call);

		m->pg = pg;
		sendp(c, m);
	}
}

static SunProg*
sunFindProg(SunSrv *srv, SunMsg *m, SunRpc *rpc, Channel **pc)
{
	int i, vlo, vhi;
	SunProg *pg;

	vlo = 0x7fffffff;
	vhi = -1;

	for(i=0; i<srv->nprog; i++){
		pg = srv->prog[i];
		if(pg->prog != rpc->prog)
			continue;
		if(pg->vers == rpc->vers){
			*pc = srv->cdispatch[i];
			return pg;
		}
		/* right program, wrong version: record range */
		if(pg->vers < vlo)
			vlo = pg->vers;
		if(pg->vers > vhi)
			vhi = pg->vers;
	}
	if(vhi == -1){
		if(srv->chatty)
			fprint(2, "\tprogram %ud unavailable\n", rpc->prog);
		sunMsgReplyError(m, SunProgUnavail);
	}else{
		/* putting these in rpc is a botch */
		rpc->low = vlo;
		rpc->high = vhi;
		if(srv->chatty)
			fprint(2, "\tversion %ud unavailable; have %d-%d\n", rpc->vers, vlo, vhi);
		sunMsgReplyError(m, SunProgMismatch);
	}
	return nil;
}

static void
sunRpcReplyThread(void *v)
{
	SunMsg *m;
	SunSrv *srv = v;

	while((m = recvp(srv->creply)) != nil){
		/* could record in cache here? */
		sendp(m->creply, m);
	}	
}

int
sunMsgReplyError(SunMsg *m, SunStatus error)
{
	uchar *p, *bp, *ep;
	int n;

	m->rpc.status = error;
	m->rpc.iscall = 0;
	m->rpc.verf.flavor = SunAuthNone;
	m->rpc.data = nil;
	m->rpc.ndata = 0;

	if(m->srv->chatty)
		fprint(2, "out: %B\n", &m->rpc);

	n = sunRpcSize(&m->rpc);
	bp = emalloc(n);
	ep = bp+n;
	p = bp;
	if(sunRpcPack(p, ep, &p, &m->rpc) < 0){
		fprint(2, "sunRpcPack failed\n");
		sunMsgDrop(m);
		return 0;
	}
	if(p != ep){
		fprint(2, "sunMsgReplyError: rpc sizes didn't work out\n");
		sunMsgDrop(m);
		return 0;
	}
	free(m->data);
	m->data = bp;
	m->count = n;
	sendp(m->srv->creply, m);
	return 0;
}

int
sunMsgReply(SunMsg *m, SunCall *c)
{
	int n1, n2;
	uchar *bp, *p, *ep;

	c->type = m->call->type+1;
	c->rpc.iscall = 0;
	c->rpc.prog = m->rpc.prog;
	c->rpc.vers = m->rpc.vers;
	c->rpc.proc = m->rpc.proc;
	c->rpc.xid = m->rpc.xid;

	if(m->srv->chatty){
		fprint(2, "out: %B\n", &c->rpc);
		fprint(2, "\t%C\n", c);
	}

	n1 = sunRpcSize(&c->rpc);
	n2 = sunCallSize(m->pg, c);

	bp = emalloc(n1+n2);
	ep = bp+n1+n2;
	p = bp;
	if(sunRpcPack(p, ep, &p, &c->rpc) != SunSuccess){
		fprint(2, "sunRpcPack failed\n");
		return sunMsgDrop(m);
	}
	if(sunCallPack(m->pg, p, ep, &p, c) != SunSuccess){
		fprint(2, "pg->pack failed\n");
		return sunMsgDrop(m);
	}
	if(p != ep){
		fprint(2, "sunMsgReply: sizes didn't work out\n");
		return sunMsgDrop(m);
	}
	free(m->data);
	m->data = bp;
	m->count = n1+n2;

	sendp(m->srv->creply, m);
	return 0;
}

int
sunMsgDrop(SunMsg *m)
{
	free(m->data);
	free(m->call);
	memset(m, 0xFB, sizeof *m);
	free(m);
	return 0;
}

