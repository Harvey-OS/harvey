#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

typedef struct Afid Afid;

struct Afid
{
	AuthRpc *rpc;
	char *uname;
	char *aname;
	int authok;
	int afd;
};

static uvlong authgen = 1ULL<<63;

void
auth9p(Req *r)
{
	char *spec;
	Afid *afid;
	
	afid = emalloc9p(sizeof(Afid));
	afid->afd = open("/mnt/factotum/rpc", ORDWR);
	if(afid->afd < 0)
		goto error;

	if((afid->rpc = auth_allocrpc(afid->afd)) == nil)
		goto error;

	if(r->ifcall.uname[0] == 0)
		goto error;
	afid->uname = estrdup9p(r->ifcall.uname);
	afid->aname = estrdup9p(r->ifcall.aname);

	spec = r->srv->keyspec;
	if(spec == nil)
		spec = "proto=p9any role=server";

	if(auth_rpc(afid->rpc, "start", spec, strlen(spec)) != ARok)
		goto error;

	r->afid->qid.type = QTAUTH;
	r->afid->qid.path = ++authgen;
	r->afid->qid.vers = 0;
	r->afid->omode = ORDWR;
	r->ofcall.qid = r->afid->qid;
	r->afid->aux = afid;
	respond(r, nil);
	return;

error:
	if(afid->rpc)
		auth_freerpc(afid->rpc);
	if(afid->uname)
		free(afid->uname);
	if(afid->aname)
		free(afid->aname);
	if(afid->afd >= 0)
		close(afid->afd);
	free(afid);
	responderror(r);
}

static int
_authread(Afid *afid, void *data, int count)
{
	AuthInfo *ai;
	
	switch(auth_rpc(afid->rpc, "read", nil, 0)){
	case ARdone:
		ai = auth_getinfo(afid->rpc);
		if(ai == nil)
			return -1;
		auth_freeAI(ai);
		if(chatty9p)
			fprint(2, "authenticate %s/%s: ok\n", afid->uname, afid->aname);
		afid->authok = 1;
		return 0;

	case ARok:
		if(count < afid->rpc->narg){
			werrstr("authread count too small");
			return -1;
		}
		count = afid->rpc->narg;
		memmove(data, afid->rpc->arg, count);
		return count;
	
	case ARphase:
	default:
		werrstr("authrpc botch");
		return -1;
	}
}

void
authread(Req *r)
{
	int n;
	Afid *afid;
	Fid *fid;
	
	fid = r->fid;
	afid = fid->aux;
	if(afid == nil || r->fid->qid.type != QTAUTH){
		respond(r, "not an auth fid");
		return;
	}
	n = _authread(afid, r->ofcall.data, r->ifcall.count);
	if(n < 0){
		responderror(r);
		return;
	}
	r->ofcall.count = n;
	respond(r, nil);
}

void
authwrite(Req *r)
{
	Afid *afid;
	Fid *fid;
	
	fid = r->fid;
	afid = fid->aux;
	if(afid == nil || r->fid->qid.type != QTAUTH){
		respond(r, "not an auth fid");
		return;
	}
	if(auth_rpc(afid->rpc, "write", r->ifcall.data, r->ifcall.count) != ARok){
		responderror(r);
		return;
	}
	r->ofcall.count = r->ifcall.count;
	respond(r, nil);
}

void
authdestroy(Fid *fid)
{
	Afid *afid;
	
	if((fid->qid.type & QTAUTH) && (afid = fid->aux) != nil){
		if(afid->rpc)
			auth_freerpc(afid->rpc);
		close(afid->afd);
		free(afid->uname);
		free(afid->aname);
		free(afid);
		fid->aux = nil;
	}
}

int
authattach(Req *r)
{
	Afid *afid;
	char buf[ERRMAX];
	
	if(r->afid == nil){
		respond(r, "not authenticated");
		return -1;
	}

	afid = r->afid->aux;
	if((r->afid->qid.type&QTAUTH) == 0 || afid == nil){
		respond(r, "not an auth fid");
		return -1;
	}

	if(!afid->authok){
		if(_authread(afid, buf, 0) < 0){
			responderror(r);
			return -1;
		}
	}
	
	if(strcmp(afid->uname, r->ifcall.uname) != 0){
		snprint(buf, sizeof buf, "auth uname mismatch: %s vs %s", 
			afid->uname, r->ifcall.uname);
		respond(r, buf);
		return -1;
	}

	if(strcmp(afid->aname, r->ifcall.aname) != 0){
		snprint(buf, sizeof buf, "auth aname mismatch: %s vs %s", 
			afid->aname, r->ifcall.aname);
		respond(r, buf);
		return -1;
	}
	return 0;
}

