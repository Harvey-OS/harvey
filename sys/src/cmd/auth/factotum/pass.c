/*
 * This is just a repository for a password.
 * We don't want to encourage this, there's
 * no server side.
 */

#include "dat.h"

typedef struct State State;
struct State 
{
	Key *key;
};

enum
{
	HavePass,
	Maxphase,
};

static char *phasenames[Maxphase] =
{
[HavePass]	"HavePass",
};

static int
passinit(Proto *p, Fsstate *fss)
{
	int ret;
	Key *k;
	Keyinfo ki;
	State *s;

	ret = findkey(&k, mkkeyinfo(&ki, fss, nil), "%s", p->keyprompt);
	if(ret != RpcOk)
		return ret;
	setattrs(fss->attr, k->attr);
	s = emalloc(sizeof(*s));
	s->key = k;
	fss->ps = s;
	fss->phase = HavePass;
	return RpcOk;
}

static void
passclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->key)
		closekey(s->key);
	free(s);
}

static int
passread(Fsstate *fss, void *va, uint *n)
{
	int m;
	char buf[500];
	char *pass, *user;
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case HavePass:
		user = _strfindattr(s->key->attr, "user");
		pass = _strfindattr(s->key->privattr, "!password");
		if(user==nil || pass==nil)
			return failure(fss, "passread cannot happen");
		snprint(buf, sizeof buf, "%q %q", user, pass);
		m = strlen(buf);
		if(m > *n)
			return toosmall(fss, m);
		*n = m;
		memmove(va, buf, m);
		return RpcOk;
	}
}

static int
passwrite(Fsstate *fss, void*, uint)
{
	return phaseerror(fss, "write");
}

Proto pass =
{
.name=		"pass",
.init=		passinit,
.write=		passwrite,
.read=		passread,
.close=		passclose,
.addkey=		replacekey,
.keyprompt=	"user? !password?",
};
