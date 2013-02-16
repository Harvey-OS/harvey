/*
 * p9any - protocol negotiator.
 *
 * Protocol:
 *	Server->Client: list of proto@domain, tokenize separated, nul terminated
 *	Client->Server: proto domain, tokenize separated (not proto@domain), nul terminated
 *
 * Server protocol:
 * 	read list of protocols.
 *	write null-terminated 
 */

#include "dat.h"

static Proto *negotiable[] = {
	&p9sk1,
};

struct State
{
	Fsstate subfss;
	State *substate;	/* be very careful; this is not one of our States */
	Proto *subproto;
	int keyasked;
	String *subdom;
	int version;
};

enum
{
	CNeedProtos,
	CHaveProto,
	CNeedOK,
	CRelay,

	SHaveProtos,
	SNeedProto,
	SHaveOK,
	SRelay,

	Maxphase,
};

static char *phasenames[Maxphase] =
{
[CNeedProtos]	"CNeedProtos",
[CHaveProto]	"CHaveProto",
[CNeedOK]	"CNeedOK",
[CRelay]	"CRelay",
[SHaveProtos]	"SHaveProtos",
[SNeedProto]	"SNeedProto",
[SHaveOK]	"SHaveOK",
[SRelay]	"SRelay",
};

static int
p9anyinit(Proto*, Fsstate *fss)
{
	int iscli;
	State *s;

	if((iscli = isclient(_strfindattr(fss->attr, "role"))) < 0)
		return failure(fss, nil);

	s = emalloc(sizeof *s);
	fss = fss;
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	if(iscli)
		fss->phase = CNeedProtos;
	else
		fss->phase = SHaveProtos;
	s->version = 1;
	fss->ps = s;
	return RpcOk;
}

static void
p9anyclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->subproto && s->subfss.ps && s->subproto->close)
		(*s->subproto->close)(&s->subfss);
	s->subproto = nil;
	s->substate = nil;
	s_free(s->subdom);
	s->subdom = nil;
	s->keyasked = 0;
	memset(&s->subfss, 0, sizeof s->subfss);
	free(s);
}

static void
setupfss(Fsstate *fss, State *s, Key *k)
{
	fss->attr = setattr(fss->attr, "proto=%q", s->subproto->name);
	fss->attr = setattr(fss->attr, "dom=%q", _strfindattr(k->attr, "dom"));
	s->subfss.attr = fss->attr;
	s->subfss.phase = Notstarted;
	s->subfss.sysuser = fss->sysuser;
	s->subfss.seqnum = fss->seqnum;
	s->subfss.conf = fss->conf;
	s->subfss.nconf = fss->nconf;
}

static int
passret(Fsstate *fss, State *s, int ret)
{
	switch(ret){
	default:
		return ret;
	case RpcFailure:
		if(s->subfss.phase == Broken)
			fss->phase = Broken;
		memmove(fss->err, s->subfss.err, sizeof fss->err);
		return ret;
	case RpcNeedkey:
		memmove(fss->keyinfo, s->subfss.keyinfo, sizeof fss->keyinfo);
		return ret;
	case RpcOk:
		if(s->subfss.haveai){
			fss->haveai = 1;
			fss->ai = s->subfss.ai;
			s->subfss.haveai = 0;
		}
		if(s->subfss.phase == Established)
			fss->phase = Established;
		return ret;
	case RpcToosmall:
		fss->rpc.nwant = s->subfss.rpc.nwant;
		return ret;
	case RpcConfirm:
		fss->conf = s->subfss.conf;
		fss->nconf = s->subfss.nconf;
		return ret;
	}
}

static int
p9anyread(Fsstate *fss, void *a, uint *n)
{
	int i, m, ophase, ret;
	Attr *anew;
	Key *k;
	Keyinfo ki;
	String *negstr;
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case SHaveProtos:
		m = 0;
		negstr = s_new();
		mkkeyinfo(&ki, fss, nil);
		ki.attr = nil;
		ki.noconf = 1;
		ki.user = nil;
		for(i=0; i<nelem(negotiable); i++){
			anew = setattr(_copyattr(fss->attr), "proto=%q dom?", negotiable[i]->name);
			ki.attr = anew;
			for(ki.skip=0; findkey(&k, &ki, nil)==RpcOk; ki.skip++){
				if(m++)
					s_append(negstr, " ");
				s_append(negstr, negotiable[i]->name);
				s_append(negstr, "@");
				s_append(negstr, _strfindattr(k->attr, "dom"));
				closekey(k);
			}
			_freeattr(anew);
		}
		if(m == 0){
			s_free(negstr);
			return failure(fss, Enegotiation);
		}
		i = s_len(negstr)+1;
		if(*n < i){
			s_free(negstr);
			return toosmall(fss, i);
		}
		*n = i;
		memmove(a, s_to_c(negstr), i+1);
		fss->phase = SNeedProto;
		s_free(negstr);
		return RpcOk;

	case CHaveProto:
		i = strlen(s->subproto->name)+1+s_len(s->subdom)+1;
		if(*n < i)
			return toosmall(fss, i);
		*n = i;
		strcpy(a, s->subproto->name);
		strcat(a, " ");
		strcat(a, s_to_c(s->subdom));
		if(s->version == 1)
			fss->phase = CRelay;
		else
			fss->phase = CNeedOK;
		return RpcOk;

	case SHaveOK:
		i = 3;
		if(*n < i)
			return toosmall(fss, i);
		*n = i;
		strcpy(a, "OK");
		fss->phase = SRelay;
		return RpcOk;

	case CRelay:
	case SRelay:
		ophase = s->subfss.phase;
		ret = (*s->subproto->read)(&s->subfss, a, n);
		rpcrdwrlog(&s->subfss, "read", *n, ophase, ret);
		return passret(fss, s, ret);
	}
}

static char*
getdom(char *p)
{
	p = strchr(p, '@');
	if(p == nil)
		return "";
	return p+1;
}

static Proto*
findneg(char *name)
{
	int i, len;
	char *p;

	if(p = strchr(name, '@'))
		len = p-name;
	else
		len = strlen(name);

	for(i=0; i<nelem(negotiable); i++)
		if(strncmp(negotiable[i]->name, name, len) == 0 && negotiable[i]->name[len] == 0)
			return negotiable[i];
	return nil;
}

static int
p9anywrite(Fsstate *fss, void *va, uint n)
{
	char *a, *dom, *user, *token[20];
	int asking, i, m, ophase, ret;
	Attr *anew, *anewsf, *attr;
	Key *k;
	Keyinfo ki;
	Proto *p;
	State *s;

	s = fss->ps;
	a = va;
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case CNeedProtos:
		if(n==0 || a[n-1] != '\0')
			return toosmall(fss, 2048);
		a = estrdup(a);
		m = tokenize(a, token, nelem(token));
		if(m > 0 && strncmp(token[0], "v.", 2) == 0){
			s->version = atoi(token[0]+2);
			if(s->version != 2){
				free(a);
				return failure(fss, "unknown version of p9any");
			}
		}
	
		/*
		 * look for a key
		 */
		anew = _delattr(_delattr(_copyattr(fss->attr), "proto"), "role");
		anewsf = _delattr(_copyattr(anew), "user");
		user = _strfindattr(anew, "user");
		k = nil;
		p = nil;
		dom = nil;
		for(i=(s->version==1?0:1); i<m; i++){
			p = findneg(token[i]);
			if(p == nil)
				continue;
			dom = getdom(token[i]);
			ret = RpcFailure;
			mkkeyinfo(&ki, fss, nil);
			if(user==nil || strcmp(user, fss->sysuser)==0){
				ki.attr = anewsf;
				ki.user = nil;
				ret = findkey(&k, &ki, "proto=%q dom=%q role=speakfor %s",
						p->name, dom, p->keyprompt);
			}
			if(ret == RpcFailure){
				ki.attr = anew;
				ki.user = fss->sysuser;
				ret = findkey(&k, &ki,
					"proto=%q dom=%q role=client %s",
					p->name, dom, p->keyprompt);
			}
			if(ret == RpcConfirm){
				free(a);
				return ret;
			}
			if(ret == RpcOk)
				break;
		}
		_freeattr(anewsf);

		/*
		 * no acceptable key, go through the proto@domains one at a time.
		 */
		asking = 0;
		if(k == nil){
			while(!asking && s->keyasked < m){
				p = findneg(token[s->keyasked]);
				if(p == nil){
					s->keyasked++;
					continue;
				}
				dom = getdom(token[s->keyasked]);
				mkkeyinfo(&ki, fss, nil);
				ki.attr = anew;
				ret = findkey(&k, &ki,
					"proto=%q dom=%q role=client %s",
					p->name, dom, p->keyprompt);
				s->keyasked++;
				if(ret == RpcNeedkey){
					asking = 1;
					break;
				}
			}
		}
		if(k == nil){
			free(a);
			_freeattr(anew);
			if(asking)
				return RpcNeedkey;
			else if(s->keyasked)
				return failure(fss, nil);
			else
				return failure(fss, Enegotiation);
		}
		s->subdom = s_copy(dom);
		s->subproto = p;
		free(a);
		_freeattr(anew);
		setupfss(fss, s, k);
		closekey(k);
		ret = (*s->subproto->init)(p, &s->subfss);
		rpcstartlog(s->subfss.attr, &s->subfss, ret);
		if(ret == RpcOk)
			fss->phase = CHaveProto;
		return passret(fss, s, ret);

	case SNeedProto:
		if(n==0 || a[n-1] != '\0')
			return toosmall(fss, n+1);
		a = estrdup(a);
		m = tokenize(a, token, nelem(token));
		if(m != 2){
			free(a);
			return failure(fss, Ebadarg);
		}
		p = findneg(token[0]);
		if(p == nil){
			free(a);
			return failure(fss, Enegotiation);
		}
		attr = _delattr(_copyattr(fss->attr), "proto");
		mkkeyinfo(&ki, fss, nil);
		ki.attr = attr;
		ki.user = nil;
		ret = findkey(&k, &ki, "proto=%q dom=%q role=server", token[0], token[1]);
		free(a);
		_freeattr(attr);
		if(ret == RpcConfirm)
			return ret;
		if(ret != RpcOk)
			return failure(fss, Enegotiation);
		s->subproto = p;
		setupfss(fss, s, k);
		closekey(k);
		ret = (*s->subproto->init)(p, &s->subfss);
		if(ret == RpcOk){
			if(s->version == 1)
				fss->phase = SRelay;
			else
				fss->phase = SHaveOK;
		}
		return passret(fss, s, ret);

	case CNeedOK:
		if(n < 3)
			return toosmall(fss, 3);
		if(strcmp("OK", a) != 0)
			return failure(fss, "server gave up");
		fss->phase = CRelay;
		return RpcOk;

	case CRelay:
	case SRelay:
		ophase = s->subfss.phase;
		ret = (*s->subproto->write)(&s->subfss, va, n);
		rpcrdwrlog(&s->subfss, "write", n, ophase, ret);
		return passret(fss, s, ret);
	}
}

Proto p9any = 
{
.name=	"p9any",
.init=		p9anyinit,
.write=	p9anywrite,
.read=	p9anyread,
.close=	p9anyclose,
};
