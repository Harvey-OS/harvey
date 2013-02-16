/*
 * Factotum RPC
 *
 * Must be paired write/read cycles on /mnt/factotum/rpc.
 * The format of a request is verb, single space, data.
 * Data format is verb-dependent; in particular, it can be binary.
 * The format of a response is the same.  The write only sets up
 * the RPC.  The read tries to execute it.  If the /mnt/factotum/key
 * file is open, we ask for new keys using that instead of returning
 * an error in the RPC.  This means the read blocks.
 * Textual arguments are parsed with tokenize, so rc-style quoting
 * rules apply.
 *
 * Only authentication protocol messages go here.  Configuration
 * is still via ctl (below).
 *
 * Return values are:
 *	error message - an error happened.
 *	ok [data] - success, possible data is request dependent.
 *	needkey proto domain user - request aborted, get me this key and try again
 *	badkey proto domain user - request aborted, this key might be bad
 *	done [haveai] - authentication is done [haveai: you can get an ai with authinfo]
 * Request RPCs are:
 *	start attrs - initializes protocol for authentication, can fail.
 *		returns "ok read" or "ok write" on success.
 *	read - execute protocol read
 *	write - execute protocol write
 *	authinfo - if the protocol is finished, return the AI if any
 *	attr - return protocol information
 */

#include "dat.h"

Req *rpcwait;

typedef struct Verb Verb;
struct Verb {
	char *verb;
	int iverb;
};

enum {
	Vunknown = -1,
	Vauthinfo = 0,
	Vread,
	Vstart,
	Vwrite,
	Vattr,
};

Verb rpctab[] = {
	"authinfo",	Vauthinfo,
	"read",		Vread,
	"start",		Vstart,
	"write",		Vwrite,
	"attr",		Vattr,
};

static int
classify(char *s, Verb *verbtab, int nverbtab)
{
	int i;

	for(i=0; i<nverbtab; i++)
		if(strcmp(s, verbtab[i].verb) == 0)
			return verbtab[i].iverb;
	return Vunknown;
}

void
rpcwrite(Req *r)
{
	Fsstate *fss;

	if(r->ifcall.count >= Maxrpc){
		respond(r, Etoolarge);
		return;
	}
	fss = r->fid->aux;
	if(fss->pending){
		respond(r, "rpc already pending; read to clear");
		return;
	}
	memmove(fss->rpc.buf, r->ifcall.data, r->ifcall.count);
	fss->rpc.buf[r->ifcall.count] = '\0';
	fss->rpc.verb = fss->rpc.buf;
	if(fss->rpc.arg = strchr(fss->rpc.buf, ' ')){
		*fss->rpc.arg++ = '\0';
		fss->rpc.narg = r->ifcall.count - (fss->rpc.arg - fss->rpc.buf);
	}else{
		fss->rpc.arg = "";
		fss->rpc.narg = 0;
	}
	fss->rpc.iverb = classify(fss->rpc.verb, rpctab, nelem(rpctab));
	r->ofcall.count = r->ifcall.count;
	fss->pending = 1;
	respond(r, nil);
}

static void
retstring(Req *r, Fsstate *fss, char *s)
{
	int n;

	n = strlen(s);
	if(n > r->ifcall.count)
		n = r->ifcall.count;
	memmove(r->ofcall.data, s, n);
	r->ofcall.count = n;
	fss->pending = 0;
	respond(r, nil);
	return;
}

static void
retrpc(Req *r, int ret, Fsstate *fss)
{
	switch(ret){
	default:
		snprint(fss->rpc.buf, Maxrpc, "internal error %d", ret);
		retstring(r, fss, fss->rpc.buf);
		return;
	case RpcErrstr:
		snprint(fss->rpc.buf, Maxrpc, "error %r");
		retstring(r, fss, fss->rpc.buf);
		return;
	case RpcFailure:
		snprint(fss->rpc.buf, Maxrpc, "error %s", fss->err);
		retstring(r, fss, fss->rpc.buf);
		return;
	case RpcNeedkey:
		if(needkeyqueue(r, fss) < 0){
			snprint(fss->rpc.buf, Maxrpc, "needkey %s", fss->keyinfo);
			retstring(r, fss, fss->rpc.buf);
		}
		return;
	case RpcOk:
		retstring(r, fss, "ok");
		return;
	case RpcToosmall:
		snprint(fss->rpc.buf, Maxrpc, "toosmall %d", fss->rpc.nwant);
		retstring(r, fss, fss->rpc.buf);
		return;
	case RpcPhase:
		snprint(fss->rpc.buf, Maxrpc, "phase %r");
		retstring(r, fss, fss->rpc.buf);
		return;
	case RpcConfirm:
		confirmqueue(r, fss);
		return;
	}
}

int
rdwrcheck(Req *r, Fsstate *fss)
{
	if(fss->ps == nil){
		retstring(r, fss, "error no current protocol");
		return -1;
	}
	if(fss->phase == Notstarted){
		retstring(r, fss, "protocol not started");
		return -1;
	}
	if(fss->phase == Broken){
		snprint(fss->rpc.buf, Maxrpc, "error %s", fss->err);
		retstring(r, fss, fss->rpc.buf);
		return -1;
	}
	if(fss->phase == Established){
		if(fss->haveai)
			retstring(r, fss, "done haveai");
		else
			retstring(r, fss, "done");
		return -1;
	}
	return 0;
}

static void
logret(char *pre, Fsstate *fss, int ret)
{
	switch(ret){
	default:
		flog("%s: code %d", pre, ret);
		break;
	case RpcErrstr:
		flog("%s: error %r", pre);
		break;
	case RpcFailure:
		flog("%s: failure %s", pre, fss->err);
		break;
	case RpcNeedkey:
		flog("%s: needkey %s", pre, fss->keyinfo);
		break;
	case RpcOk:
		flog("%s: ok", pre);
		break;
	case RpcToosmall:
		flog("%s: toosmall %d", pre, fss->rpc.nwant);
		break;
	case RpcPhase:
		flog("%s: phase: %r", pre);
		break;
	case RpcConfirm:
		flog("%s: waiting for confirm", pre);
		break;
	}
}

void
rpcrdwrlog(Fsstate *fss, char *rdwr, uint n, int ophase, int ret)
{
	char buf0[40], buf1[40], pre[300];

	if(!debug)
		return;
	snprint(pre, sizeof pre, "%d: %s %ud in phase %s yields phase %s",
		fss->seqnum, rdwr, n, phasename(fss, ophase, buf0), phasename(fss, fss->phase, buf1));
	logret(pre, fss, ret);
}

void
rpcstartlog(Attr *attr, Fsstate *fss, int ret)
{
	char pre[300], tmp[40];

	if(!debug)
		return;
	snprint(pre, sizeof pre, "%d: start %A yields phase %s", fss->seqnum,
		attr, phasename(fss, fss->phase, tmp));
	logret(pre, fss, ret);		
}

int seqnum;

void
rpcread(Req *r)
{
	Attr *attr;
	char *p;
	int ophase, ret;
	uchar *e;
	uint count;
	Fsstate *fss;
	Proto *proto;

	if(r->ifcall.count < 64){
		respond(r, "rpc read too small");
		return;
	}
	fss = r->fid->aux;
	if(!fss->pending){
		respond(r, "no rpc pending");
		return;
	}
	switch(fss->rpc.iverb){
	default:
	case Vunknown:
		retstring(r, fss, "error unknown verb");
		break;

	case Vstart:
		if(fss->phase != Notstarted){
			flog("%d: implicit close due to second start; old attr '%A'", fss->seqnum, fss->attr);
			if(fss->proto && fss->ps)
				(*fss->proto->close)(fss);
			fss->ps = nil;
			fss->proto = nil;
			_freeattr(fss->attr);
			fss->attr = nil;
			fss->phase = Notstarted;
		}	
		attr = _parseattr(fss->rpc.arg);
		if((p = _strfindattr(attr, "proto")) == nil){
			retstring(r, fss, "error did not specify proto");
			_freeattr(attr);
			break;
		}
		if((proto = findproto(p)) == nil){
			snprint(fss->rpc.buf, Maxrpc, "error unknown protocol %q", p);
			retstring(r, fss, fss->rpc.buf);
			_freeattr(attr);
			break;
		}
		fss->attr = attr;
		fss->proto = proto;
		fss->seqnum = ++seqnum;
		ret = (*proto->init)(proto, fss);
		rpcstartlog(attr, fss, ret);
		if(ret != RpcOk){
			_freeattr(fss->attr);
			fss->attr = nil;
			fss->phase = Notstarted;
		}
		retrpc(r, ret, fss);
		break;

	case Vread:
		if(fss->rpc.arg && fss->rpc.arg[0]){
			retstring(r, fss, "error read needs no parameters");
			break;
		}
		if(rdwrcheck(r, fss) < 0)
			break;
		count = r->ifcall.count - 3;
		ophase = fss->phase;
		ret = fss->proto->read(fss, (uchar*)r->ofcall.data+3, &count);
		rpcrdwrlog(fss, "read", count, ophase, ret);
		if(ret == RpcOk){
			memmove(r->ofcall.data, "ok ", 3);
			if(count == 0)
				r->ofcall.count = 2;
			else
				r->ofcall.count = 3+count;
			fss->pending = 0;
			respond(r, nil);
		}else
			retrpc(r, ret, fss);
		break;

	case Vwrite:
		if(rdwrcheck(r, fss) < 0)
			break;
		ophase = fss->phase;
		ret = fss->proto->write(fss, fss->rpc.arg, fss->rpc.narg);
		rpcrdwrlog(fss, "write", fss->rpc.narg, ophase, ret);
		retrpc(r, ret, fss);
		break;

	case Vauthinfo:
		if(fss->phase != Established){
			retstring(r, fss, "error authentication unfinished");
			break;
		}
		if(!fss->haveai){
			retstring(r, fss, "error no authinfo available");
			break;
		}
		memmove(r->ofcall.data, "ok ", 3);
		fss->ai.cap = mkcap(r->fid->uid, fss->ai.suid);
		e = convAI2M(&fss->ai, (uchar*)r->ofcall.data+3, r->ifcall.count-3);
		free(fss->ai.cap);
		fss->ai.cap = nil;
		if(e == nil){
			retstring(r, fss, "error read too small");
			break;
		}
		r->ofcall.count = e - (uchar*)r->ofcall.data;
		fss->pending = 0;
		respond(r, nil);
		break;

	case Vattr:
		snprint(fss->rpc.buf, Maxrpc, "ok %A", fss->attr);
		retstring(r, fss, fss->rpc.buf);
		break;
	}
}

enum {
	Vdelkey,
	Vaddkey,
	Vdebug,
};

Verb ctltab[] = {
	"delkey",		Vdelkey,
	"key",		Vaddkey,
	"debug",	Vdebug,
};

/*
 *	key attr=val... - add a key
 *		the attr=val pairs are protocol-specific.
 *		for example, both of these are valid:
 *			key p9sk1 gre cs.bell-labs.com mysecret
 *			key p9sk1 gre cs.bell-labs.com 11223344556677 fmt=des7hex
 *	delkey ... - delete a key
 *		if given, the attr=val pairs are used to narrow the search
 *		[maybe should require a password?]
 */

int
ctlwrite(char *a, int atzero)
{
	char *p;
	int i, nmatch, ret;
	Attr *attr, **l, **lpriv, **lprotos, *pa, *priv, *protos;
	Key *k;
	Proto *proto;

	if(a[0] == '#' || a[0] == '\0')
		return 0;

	/*
	 * it would be nice to emit a warning of some sort here.
	 * we ignore all but the first line of the write.  this helps
	 * both with things like "echo delkey >/mnt/factotum/ctl"
	 * and writes that (incorrectly) contain multiple key lines.
	 */
	if(p = strchr(a, '\n')){
		if(p[1] != '\0'){
			werrstr("multiline write not allowed");
			return -1;
		}
		*p = '\0';
	}

	if((p = strchr(a, ' ')) == nil)
		p = "";
	else
		*p++ = '\0';
	switch(classify(a, ctltab, nelem(ctltab))){
	default:
	case Vunknown:
		werrstr("unknown verb");
		return -1;
	case Vdebug:
		debug ^= 1;
		return 0;
	case Vdelkey:
		nmatch = 0;
		attr = _parseattr(p);
		for(pa=attr; pa; pa=pa->next){
			if(pa->type != AttrQuery && pa->name[0]=='!'){
				werrstr("only !private? patterns are allowed for private fields");
				_freeattr(attr);
				return -1;
			}
		}
		for(i=0; i<ring->nkey; ){
			if(matchattr(attr, ring->key[i]->attr, ring->key[i]->privattr)){
				nmatch++;
				closekey(ring->key[i]);
				ring->nkey--;
				memmove(&ring->key[i], &ring->key[i+1], (ring->nkey-i)*sizeof(ring->key[0]));
			}else
				i++;
		}
		_freeattr(attr);
		if(nmatch == 0){
			werrstr("found no keys to delete");
			return -1;
		}
		return 0;
	case Vaddkey:
		attr = _parseattr(p);
		/* separate out proto= attributes */
		lprotos = &protos;
		for(l=&attr; (*l); ){
			if(strcmp((*l)->name, "proto") == 0){
				*lprotos = *l;
				lprotos = &(*l)->next;
				*l = (*l)->next;
			}else
				l = &(*l)->next;
		}
		*lprotos = nil;
		if(protos == nil){
			werrstr("key without protos");
			_freeattr(attr);
			return -1;
		}

		/* separate out private attributes */
		lpriv = &priv;
		for(l=&attr; (*l); ){
			if((*l)->name[0] == '!'){
				*lpriv = *l;
				lpriv = &(*l)->next;
				*l = (*l)->next;
			}else
				l = &(*l)->next;
		}
		*lpriv = nil;

		/* add keys */
		ret = 0;
		for(pa=protos; pa; pa=pa->next){
			if((proto = findproto(pa->val)) == nil){
				werrstr("unknown proto %s", pa->val);
				ret = -1;
				continue;
			}
			if(proto->addkey == nil){
				werrstr("proto %s doesn't take keys", proto->name);
				ret = -1;
				continue;
			}
			k = emalloc(sizeof(Key));
			k->attr = _mkattr(AttrNameval, "proto", proto->name, _copyattr(attr));
			k->privattr = _copyattr(priv);
			k->ref = 1;
			k->proto = proto;
			if(proto->addkey(k, atzero) < 0){
				ret = -1;
				closekey(k);
				continue;
			}
			closekey(k);
		}
		_freeattr(attr);
		_freeattr(priv);
		_freeattr(protos);
		return ret;
	}
}
