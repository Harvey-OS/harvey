/*
 * SRS - sender rewriting scheme
 *
 * Encoding:
 *
 *	user@domain => SRS0=hash=time=dom=user
 *	SRS0=hash=...@domain => SRS1=hash'=domain==hash=...
 * 	SRS1=hash=...@domain => SRS1=hash'=...
 *
 * This is just a crutch so that factotum can keep the secret for us.
 */

#include "dat.h"

static int
srslevel(char *s)
{
	if(cistrncmp(s, "SRS", 3) != 0)
		return -1;
	if(s[3] != '0' && s[3] != '1')
		return -1;
	if(s[4] != '-' && s[4] != '+' && s[4] != '=')
		return -1;
	return s[3]-'0';
}

static char t32e[] = "23456789abcdefghijkmnpqrstuvwxyz";
static char t32E[] = "23456789ABCDEFGHIJKMNPQRSTUVWXYZ";

static char*
srsencode(char *addr, char *key)
{
	DigestState ds;
	uchar digest[SHA1dlen];
	char *user, *dom, *new, *p;
	int day, len;

	len = strlen(addr);
	dom = strchr(addr, '@');
	if(dom == nil){
		werrstr("no domain");
		return nil;
	}
	*dom++ = 0;
	user = addr;
	new = emalloc(len+40);

	switch(srslevel(user)){
	case -1:
		strcpy(new, "SRS0=HHH=TT=");
		strcat(new, dom);
		strcat(new, "=");
		strcat(new, user);
		day = time(0)/86400;
		new[9] = t32e[(day/32)%32];
		new[10] = t32e[day%32];
		break;
	case 0:
		strcpy(new, "SRS1=HHH=");
		strcat(new, dom);
		strcat(new, "=");
		strcat(new, user+4);
		break;
	case 1:
		strcpy(new, "SRS1=HHH=");
		p = strchr(user+5, '=');
		if(p == nil){
			free(new);
			werrstr("bad SRS1 address");
			return nil;
		}
		strcat(new, p+1);
		break;
	}

	memset(&ds, 0, sizeof ds);
	hmac_sha1((uchar*)new+9, strlen(new+9), (uchar*)key, strlen(key), digest, &ds);
	new[5] = t32e[digest[0]%32];
	new[6] = t32e[digest[1]%32];
	new[7] = t32e[digest[2]%32];
	return new;
}

static int
u32d(char c)
{
	char *p;

	p = strchr(t32e, c);
	if(p)
		return p - t32e;
	p = strchr(t32E, c);
	if(p)
		return p - t32E;
	return 0;
}

static char*
srsdecode(char *addr, char *key)
{
	DigestState ds;
	uchar digest[SHA1dlen];
	char *new, *p;
	int day, len, t;

	if(p = strchr(addr, '@'))
		*p = 0;

	len = strlen(addr);
	if(srslevel(addr) < 0 || len < 9 || addr[8] != '='){
	bad:
		werrstr("bad SRS address");
		return nil;
	}

	memset(&ds, 0, sizeof ds);
	hmac_sha1((uchar*)addr+9, strlen(addr+9), (uchar*)key, strlen(key), digest, &ds);
	if((addr[5] != t32e[digest[0]%32] && addr[5] != t32E[digest[0]%32])
	|| (addr[6] != t32e[digest[1]%32] && addr[6] != t32E[digest[0]%32])
	|| (addr[7] != t32e[digest[2]%32] && addr[7] != t32E[digest[0]%32])){
	corrupt:
		werrstr("corrupted SRS address");
		return nil;
	}
	
	switch(srslevel(addr)){
	default:
	case -1:
		goto bad;
	case 0:
		if(len < 12 || addr[11] != '=')
			goto corrupt;
		t = u32d(addr[9])*32 + u32d(addr[10]);
		day = (time(0)/86400) % (32*32);
		t = day - t;
		if(t < 0)
			t += 32*32;
		if(t >= 7){	/* one week */
			werrstr("expired SRS address");
			return nil;
		}
		if((p = strchr(addr+12, '=')) == nil)
			goto corrupt;
		*p++ = 0;
		new = emalloc(len);
		strcpy(new, p);
		strcat(new, "@");
		strcat(new, addr+12);
		break;
	case 1:
		if((p = strchr(addr+9, '=')) == nil)
			goto corrupt;
		*p++ = 0;
		if(*p != '=' && *p != '-' && *p != '+')
			goto corrupt;
		new = emalloc(len);
		strcpy(new, "SRS0");
		strcat(new, p);
		strcat(new, "@");
		strcat(new, addr+9);
		break;
	}
	return new;
}

static char *srsencode(char *addr, char *key);
static char *srsdecode(char *addr, char *key);

struct State
{
	char *addr;
	int decode;
};

enum
{
	CNeedChal,
	CHaveResp,

	Maxphase,
};

static char *phasenames[Maxphase] = {
[CNeedChal]	"CNeedChal",
[CHaveResp]	"CHaveResp",
};

static int
srsinit(Proto *p, Fsstate *fss)
{
	State *s;

	USED(p);
	s = emalloc(sizeof *s);
	s->decode = _strfindattr(fss->attr, "decode") != nil;
	fss->phasename = phasenames;
	fss->maxphase = Maxphase;
	fss->phase = CNeedChal;
	fss->ps = s;
	return RpcOk;
}

static int
srswrite(Fsstate *fss, void *va, uint n)
{
	char *a, *v;
	int ret;
	Key *k;
	Keyinfo ki;
	State *s;

	s = fss->ps;
	a = va;
	USED(n);
	switch(fss->phase){
	default:
		return phaseerror(fss, "write");

	case CNeedChal:
		ret = findkey(&k, mkkeyinfo(&ki, fss, nil), "%s", fss->proto->keyprompt);
		if(ret != RpcOk)
			return ret;
		v = _strfindattr(k->privattr, "!password");
		if(v == nil)
			return failure(fss, "key has no password");
		setattrs(fss->attr, k->attr);
		if(s->decode)
			s->addr = srsdecode(a, v);
		else
			s->addr = srsencode(a, v);
		if(s->addr == nil)
			return failure(fss, "%r");
		closekey(k);
		fss->phase = CHaveResp;
		return RpcOk;
	}
}

static int
srsread(Fsstate *fss, void *va, uint *n)
{
	State *s;

	s = fss->ps;
	switch(fss->phase){
	default:
		return phaseerror(fss, "read");

	case CHaveResp:
		if(*n > strlen(s->addr))
			*n = strlen(s->addr);
		memmove(va, s->addr, *n);
		fss->phase = Established;
		fss->haveai = 0;
		return RpcOk;
	}
}

static void
srsclose(Fsstate *fss)
{
	State *s;

	s = fss->ps;
	free(s->addr);
	free(s);
}

Proto srs = {
.name=	"srs",
.init=		srsinit,
.write=	srswrite,
.read=	srsread,
.close=	srsclose,
.addkey=	replacekey,
.keyprompt=	"!password?"
};
