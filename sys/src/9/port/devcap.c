/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include <mp.h>
#include	<libsec.h>

enum
{
	Hashlen=	SHA1dlen,
	Maxhash=	256,
};

/*
 *  if a process knows cap->cap, it can change user
 *  to capabilty->user.
 */
typedef struct Caphash	Caphash;
struct Caphash
{
	Caphash	*next;
	char		hash[Hashlen];
};

struct
{
	QLock QLock;
	Caphash	*first;
	int	nhash;
} capalloc;

enum
{
	Qdir,
	Qhash,
	Quse,
};

/* caphash must be last */
Dirtab capdir[] =
{
	{".",		{Qdir,0,QTDIR},	0,		DMDIR|0500},
	{"capuse",	{Quse},		0,		0222},
	{"caphash",	{Qhash},	0,		0200},
};
int ncapdir = nelem(capdir);

static Chan*
capattach(char *spec)
{
	return devattach(L'¤', spec);
}

static Walkqid*
capwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, capdir, ncapdir, devgen);
}

static void
capremove(Chan *c)
{
	if(iseve() && c->qid.path == Qhash)
		ncapdir = nelem(capdir)-1;
	else
		error(Eperm);
}


static int32_t
capstat(Chan *c, uint8_t *db, int32_t n)
{
	return devstat(c, db, n, capdir, ncapdir, devgen);
}

/*
 *  if the stream doesn't exist, create it
 */
static Chan*
capopen(Chan *c, int omode)
{
	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Ebadarg);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	}

	switch((uint32_t)c->qid.path){
	case Qhash:
		if(!iseve())
			error(Eperm);
		break;
	}

	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

/*
static char*
hashstr(uchar *hash)
{
	static char buf[2*Hashlen+1];
	int i;

	for(i = 0; i < Hashlen; i++)
		sprint(buf+2*i, "%2.2x", hash[i]);
	buf[2*Hashlen] = 0;
	return buf;
}
 */

static Caphash*
remcap(uint8_t *hash)
{
	Caphash *t, **l;

	qlock(&capalloc.QLock);

	/* find the matching capability */
	for(l = &capalloc.first; *l != nil;){
		t = *l;
		if(memcmp(hash, t->hash, Hashlen) == 0)
			break;
		l = &t->next;
	}
	t = *l;
	if(t != nil){
		capalloc.nhash--;
		*l = t->next;
	}
	qunlock(&capalloc.QLock);

	return t;
}

/* add a capability, throwing out any old ones */
static void
addcap(uint8_t *hash)
{
	Caphash *p, *t, **l;

	p = smalloc(sizeof *p);
	memmove(p->hash, hash, Hashlen);
	p->next = nil;

	qlock(&capalloc.QLock);

	/* trim extras */
	while(capalloc.nhash >= Maxhash){
		t = capalloc.first;
		if(t == nil)
			panic("addcap");
		capalloc.first = t->next;
		free(t);
		capalloc.nhash--;
	}

	/* add new one */
	for(l = &capalloc.first; *l != nil; l = &(*l)->next)
		;
	*l = p;
	capalloc.nhash++;

	qunlock(&capalloc.QLock);
}

static void
capclose(Chan* c)
{
}

static int32_t
capread(Chan *c, void *va, int32_t n, int64_t m)
{
	switch((uint32_t)c->qid.path){
	case Qdir:
		return devdirread(c, va, n, capdir, ncapdir, devgen);

	default:
		error(Eperm);
		break;
	}
	return n;
}

static int32_t
capwrite(Chan *c, void *va, int32_t n, int64_t m)
{
	Caphash *p;
	char *cp;
	uint8_t hash[Hashlen];
	char *key, *from, *to;
	char err[256];
	Proc *up = externup();

	switch((uint32_t)c->qid.path){
	case Qhash:
		if(!iseve())
			error(Eperm);
		if(n < Hashlen)
			error(Eshort);
		memmove(hash, va, Hashlen);
		addcap(hash);
		break;

	case Quse:
		/* copy key to avoid a fault in hmac_xx */
		cp = nil;
		if(waserror()){
			free(cp);
			nexterror();
		}
		cp = smalloc(n+1);
		memmove(cp, va, n);
		cp[n] = 0;

		from = cp;
		key = strrchr(cp, '@');
		if(key == nil)
			error(Eshort);
		*key++ = 0;
		hmac_sha1((uint8_t*)from, strlen(from), (uint8_t*)key, strlen(key), hash, nil);

		p = remcap(hash);
		if(p == nil){
			snprint(err, sizeof err, "invalid capability %s@%s", from, key);
			error(err);
		}

		/* if a from user is supplied, make sure it matches */
		to = strchr(from, '@');
		if(to == nil){
			to = from;
		} else {
			*to++ = 0;
			if(strcmp(from, up->user) != 0)
				error("capability must match user");
		}

		/* set user id */
		kstrdup(&up->user, to);
		up->basepri = PriNormal;

		free(p);
		free(cp);
		poperror();
		break;

	default:
		error(Eperm);
		break;
	}

	return n;
}

Dev capdevtab = {
	.dc = L'¤',
	.name = "cap",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = capattach,
	.walk = capwalk,
	.stat = capstat,
	.open = capopen,
	.create = devcreate,
	.close = capclose,
	.read = capread,
	.bread = devbread,
	.write = capwrite,
	.bwrite = devbwrite,
	.remove = capremove,
	.wstat = devwstat
};
