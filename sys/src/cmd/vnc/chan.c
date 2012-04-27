#include	<u.h>
#include	<libc.h>
#include	"compat.h"
#include	"error.h"

Chan*
newchan(void)
{
	Chan *c;

	c = smalloc(sizeof(Chan));

	/* if you get an error before associating with a dev,
	   close calls rootclose, a nop */
	c->type = 0;
	c->flag = 0;
	c->ref = 1;
	c->dev = 0;
	c->offset = 0;
	c->iounit = 0;
	c->aux = 0;
	c->name = 0;
	return c;
}

void
chanfree(Chan *c)
{
	c->flag = CFREE;

	cnameclose(c->name);
	free(c);
}

void
cclose(Chan *c)
{
	if(c->flag&CFREE)
		panic("cclose %#p", getcallerpc(&c));
	if(decref(c))
		return;

	if(!waserror()){
		devtab[c->type]->close(c);
		poperror();
	}

	chanfree(c);
}

Chan*
cclone(Chan *c)
{
	Chan *nc;
	Walkqid *wq;

	wq = devtab[c->type]->walk(c, nil, nil, 0);
	if(wq == nil)
		error("clone failed");
	nc = wq->clone;
	free(wq);
	nc->name = c->name;
	if(c->name)
		incref(c->name);
	return nc;
}

enum
{
	CNAMESLOP	= 20
};

static Ref ncname;

void cleancname(Cname*);

int
isdotdot(char *p)
{
	return p[0]=='.' && p[1]=='.' && p[2]=='\0';
}

int
incref(Ref *r)
{
	int x;

	lock(r);
	x = ++r->ref;
	unlock(r);
	return x;
}

int
decref(Ref *r)
{
	int x;

	lock(r);
	x = --r->ref;
	unlock(r);
	if(x < 0)
		panic("decref");

	return x;
}

Cname*
newcname(char *s)
{
	Cname *n;
	int i;

	n = smalloc(sizeof(Cname));
	i = strlen(s);
	n->len = i;
	n->alen = i+CNAMESLOP;
	n->s = smalloc(n->alen);
	memmove(n->s, s, i+1);
	n->ref = 1;
	incref(&ncname);
	return n;
}

void
cnameclose(Cname *n)
{
	if(n == nil)
		return;
	if(decref(n))
		return;
	decref(&ncname);
	free(n->s);
	free(n);
}

Cname*
addelem(Cname *n, char *s)
{
	int i, a;
	char *t;
	Cname *new;

	if(s[0]=='.' && s[1]=='\0')
		return n;

	if(n->ref > 1){
		/* copy on write */
		new = newcname(n->s);
		cnameclose(n);
		n = new;
	}

	i = strlen(s);
	if(n->len+1+i+1 > n->alen){
		a = n->len+1+i+1 + CNAMESLOP;
		t = smalloc(a);
		memmove(t, n->s, n->len+1);
		free(n->s);
		n->s = t;
		n->alen = a;
	}
	if(n->len>0 && n->s[n->len-1]!='/' && s[0]!='/')	/* don't insert extra slash if one is present */
		n->s[n->len++] = '/';
	memmove(n->s+n->len, s, i+1);
	n->len += i;
	if(isdotdot(s))
		cleancname(n);
	return n;
}

/*
 * In place, rewrite name to compress multiple /, eliminate ., and process ..
 */
void
cleancname(Cname *n)
{
	char *p;

	if(n->s[0] == '#'){
		p = strchr(n->s, '/');
		if(p == nil)
			return;
		cleanname(p);

		/*
		 * The correct name is #i rather than #i/,
		 * but the correct name of #/ is #/.
		 */
		if(strcmp(p, "/")==0 && n->s[1] != '/')
			*p = '\0';
	}else
		cleanname(n->s);
	n->len = strlen(n->s);
}

void
isdir(Chan *c)
{
	if(c->qid.type & QTDIR)
		return;
	error(Enotdir);
}
