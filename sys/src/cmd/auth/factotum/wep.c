/*
 *  The caller supplies the device, we do the flavoring.  There
 *  are no phases, everything happens in the init routine.
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
};

static int
wepinit(Proto*, Fsstate *fss)
{
	int ret;
	Key *k;
	Keyinfo ki;
	State *s;

	/* find a key with at least one password */
	mkkeyinfo(&ki, fss, nil);
	ret = findkey(&k, &ki, "!key1?");
	if(ret != RpcOk)
		ret = findkey(&k, &ki, "!key2?");
	if(ret != RpcOk)
		ret = findkey(&k, &ki, "!key3?");
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
wepclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	if(s->key)
		closekey(s->key);
	free(s);
}

static int
wepread(Fsstate *fss, void*, uint*)
{
	return phaseerror(fss, "read");
}

static int
wepwrite(Fsstate *fss, void *va, uint n)
{
	char *data = va;
	State *s;
	char dev[64];
	int fd, cfd;
	int rv;
	char *p;

	/* get the device */
	if(n > sizeof(dev)-5){
		werrstr("device too long");
		return RpcErrstr;
	}
	memmove(dev, data, n);
	dev[n] = 0;
	s = fss->ps;

	/* legal? */
	if(dev[0] != '#' || dev[1] != 'l'){
		werrstr("%s not an ether device", dev);
		return RpcErrstr;
	}
	strcat(dev, "!0");
	fd = dial(dev, 0, 0, &cfd);
	if(fd < 0)
		return RpcErrstr;

	/* flavor it with passwords, essid, and turn on wep */
	rv = RpcErrstr;
	p = _strfindattr(s->key->privattr, "!key1");
	if(p != nil)
		if(fprint(cfd, "key1 %s", p) < 0)
			goto out;
	p = _strfindattr(s->key->privattr, "!key2");
	if(p != nil)
		if(fprint(cfd, "key2 %s", p) < 0)
			goto out;
	p = _strfindattr(s->key->privattr, "!key3");
	if(p != nil)
		if(fprint(cfd, "key3 %s", p) < 0)
			goto out;
	p = _strfindattr(fss->attr, "essid");
	if(p != nil)
		if(fprint(cfd, "essid %s", p) < 0)
			goto out;
	if(fprint(cfd, "crypt on") < 0)
		goto out;
	rv = RpcOk;
out:
	close(fd);
	close(cfd);
	return rv;
}

Proto wep =
{
.name=		"wep",
.init=		wepinit,
.write=		wepwrite,
.read=		wepread,
.close=		wepclose,
.addkey=	replacekey,
.keyprompt=	"!key1? !key2? !key3? essid?",
};
