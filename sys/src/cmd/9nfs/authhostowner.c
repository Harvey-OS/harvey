#include "all.h"

enum { 
	ARgiveup = 100,
};

static uchar*
gstring(uchar *p, uchar *ep, char **s)
{
	uint n;

	if(p == nil)
		return nil;
	if(p+BIT16SZ > ep)
		return nil;
	n = GBIT16(p);
	p += BIT16SZ;
	if(p+n > ep)
		return nil;
	*s = malloc(n+1);
	memmove((*s), p, n);
	(*s)[n] = '\0';
	p += n;
	return p;
}

static uchar*
gcarray(uchar *p, uchar *ep, uchar **s, int *np)
{
	uint n;

	if(p == nil)
		return nil;
	if(p+BIT16SZ > ep)
		return nil;
	n = GBIT16(p);
	p += BIT16SZ;
	if(p+n > ep)
		return nil;
	*s = malloc(n);
	if(*s == nil)
		return nil;
	memmove((*s), p, n);
	*np = n;
	p += n;
	return p;
}

static uchar*
convM2AI(uchar *p, int n, AuthInfo **aip)
{
	uchar *e = p+n;
	AuthInfo *ai;

	ai = mallocz(sizeof(*ai), 1);
	if(ai == nil)
		return nil;

	p = gstring(p, e, &ai->cuid);
	p = gstring(p, e, &ai->suid);
	p = gstring(p, e, &ai->cap);
	p = gcarray(p, e, &ai->secret, &ai->nsecret);
	if(p == nil)
		auth_freeAI(ai);
	else
		*aip = ai;
	return p;
}

static int
dorpc(AuthRpc *rpc, char *verb, char *val, int len, AuthGetkey *getkey)
{
	int ret;

	for(;;){
		if((ret = auth_rpc(rpc, verb, val, len)) != ARneedkey && ret != ARbadkey)
			return ret;
		if(getkey == nil)
			return ARgiveup;	/* don't know how */
		if((*getkey)(rpc->arg) < 0)
			return ARgiveup;	/* user punted */
	}
}

static int
doread(Session *s, Fid *f, void *buf, int n)
{
	s->f.fid = f - s->fids;
	s->f.offset = 0;
	s->f.count = n;
	if(xmesg(s, Tread) < 0)
		return -1;
	n = s->f.count;
	memmove(buf, s->f.data, n);
	return n;
}

static int
dowrite(Session *s, Fid *f, void *buf, int n)
{
	s->f.fid = f - s->fids;
	s->f.offset = 0;
	s->f.count = n;
	s->f.data = (char *)buf;
	if(xmesg(s, Twrite) < 0)
		return -1;
	return n;
}

/*
 *  this just proxies what the factotum tells it to.
 */
AuthInfo*
authproto(Session *s, Fid *f, AuthRpc *rpc, AuthGetkey *getkey, char *params)
{
	char *buf;
	int m, n, ret;
	AuthInfo *a;
	char oerr[ERRMAX];

	rerrstr(oerr, sizeof oerr);
	werrstr("UNKNOWN AUTH ERROR");

	if(dorpc(rpc, "start", params, strlen(params), getkey) != ARok){
		werrstr("fauth_proxy start: %r");
		return nil;
	}

	buf = malloc(AuthRpcMax);
	if(buf == nil)
		return nil;
	for(;;){
		switch(dorpc(rpc, "read", nil, 0, getkey)){
		case ARdone:
			free(buf);
			a = auth_getinfo(rpc);
			errstr(oerr, sizeof oerr);	/* no error, restore whatever was there */
			return a;
		case ARok:
			if(dowrite(s, f, rpc->arg, rpc->narg) != rpc->narg){
				werrstr("auth_proxy write fd: %r");
				goto Error;
			}
			break;
		case ARphase:
			n = 0;
			memset(buf, 0, AuthRpcMax);
			while((ret = dorpc(rpc, "write", buf, n, getkey)) == ARtoosmall){
				if(atoi(rpc->arg) > AuthRpcMax)
					break;
				m = doread(s, f, buf+n, atoi(rpc->arg)-n);
				if(m <= 0){
					if(m == 0)
						werrstr("auth_proxy short read: %s", buf);
					goto Error;
				}
				n += m;
			}
			if(ret != ARok){
				werrstr("auth_proxy rpc write: %s: %r", buf);
				goto Error;
			}
			break;
		default:
			werrstr("auth_proxy rpc: %r");
			goto Error;
		}
	}
Error:
	free(buf);
	return nil;
}

/* returns 0 if auth succeeded (or unneeded), -1 otherwise */
int
authhostowner(Session *s)
{
	Fid *af, *f;
	int rv = -1;
	int afd;
	AuthInfo *ai;
	AuthRpc *rpc;

	/* get a fid to authenticate over */
	f = nil;
	af = newfid(s);
	s->f.afid = af - s->fids;
	s->f.uname = getuser();
	s->f.aname = s->spec;
	if(xmesg(s, Tauth)){
		/* not needed */
		rv = 0;
		goto out;
	}

	quotefmtinstall();	/* just in case */

	afd = open("/mnt/factotum/rpc", ORDWR);
	if(afd < 0){
		werrstr("opening /mnt/factotum/rpc: %r");
		goto out;
	}

	rpc = auth_allocrpc(afd);
	if(rpc == nil)
		goto out;

	ai = authproto(s, af, rpc, auth_getkey, "proto=p9any role=client");
	if(ai != nil){
		rv = 0;
		auth_freeAI(ai);
	}
	auth_freerpc(rpc);
	close(afd);

	/* try attaching with the afid */
	chat("attaching as hostowner...");
	f = newfid(s);
	s->f.fid = f - s->fids;
	s->f.afid = af - s->fids;;
	s->f.uname = getuser();
	s->f.aname = s->spec;
	if(xmesg(s, Tattach) == 0)
		rv = 0;
out:
	if(af != nil){
		putfid(s, af);
		s->f.fid = af - s->fids;
		xmesg(s, Tclunk);
	}
	if(f != nil){
		putfid(s, f);
		s->f.fid = f - s->fids;
		xmesg(s, Tclunk);
	}

	return rv;
}

