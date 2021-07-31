#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ip.h>
#include <plumb.h>
#include <thread.h>
#include <fcall.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

int nclient;
Client **client;

static void clientthread(void*);
int
newclient(int plumbed)
{
	int i;
	Client *c;

	for(i=0; i<nclient; i++)
		if(client[i]->ref==0)
			return i;

	c = emalloc(sizeof(Client));
	c->plumbed = plumbed;
	c->creq = chancreate(sizeof(Req*), 8);
	threadcreate(clientthread, c, STACK);

	c->io = ioproc();
	c->num = nclient;
	c->ctl = globalctl;
	clonectl(&c->ctl);
	if(nclient%16 == 0)
		client = erealloc(client, (nclient+16)*sizeof(client[0]));
	client[nclient++] = c;
	return nclient-1;
}

void
closeclient(Client *c)
{
	if(--c->ref == 0){
		if(c->bodyopened){
			if(c->url && c->url->close)
				(*c->url->close)(c);
			c->bodyopened = 0;
		}
		free(c->contenttype);
		c->contenttype = nil;
		free(c->postbody);
		c->postbody = nil;
		freeurl(c->url);
		c->url = nil;
		free(c->redirect);
		c->redirect = nil;
		free(c->authenticate);
		c->authenticate = nil;
		c->npostbody = 0;
		c->havepostbody = 0;
		c->bodyopened = 0;
	}
}

void
clonectl(Ctl *c)
{
	if(c->useragent)
		c->useragent = estrdup(c->useragent);
}

void
clientbodyopen(Client *c, Req *r)
{
	char e[ERRMAX], *next;
	int i;
	Url *u;

	next = nil;
	for(i=0; i<=c->ctl.redirectlimit; i++){
		if(c->url == nil){
			werrstr("nil url");
			goto Error;
		}
		if(c->url->open == nil){
			werrstr("unsupported url type");
			goto Error;
		}
		if(fsdebug)
			fprint(2, "try %s\n", c->url->url);
		if(c->url->open(c, c->url) < 0){
		Error:
			if(next)
				fprint(2, "next %s (but for error)\n", next);
			free(next);
			rerrstr(e, sizeof e);
			c->iobusy = 0;
			if(r != nil)
				r->fid->omode = -1;
			closeclient(c);	/* not opening */
			if(r != nil)
				respond(r, e);
			return;
		}
		if (c->authenticate)
			continue;
		if(!c->redirect)
			break;
		next = c->redirect;
		c->redirect = nil;
		if(i==c->ctl.redirectlimit){
			werrstr("redirect limit reached");
			goto Error;
		}
		if((u = parseurl(next, c->url)) == nil)
			goto Error;
		if(urldebug)
			fprint(2, "parseurl %s got scheme %d\n", next, u->ischeme);
		if(u->ischeme == USunknown){
			werrstr("redirect with unknown URL scheme");
			goto Error;
		}
		if(u->ischeme == UScurrent){
			werrstr("redirect to URL relative to current document");
			goto Error;
		}
		freeurl(c->url);
		c->url = u;
	}
	free(next);
	c->iobusy = 0;
	if(r != nil)
		respond(r, nil);
}

void
plumburl(char *url, char *base)
{
	int i;
	Client *c;
	Url *ubase, *uurl;

	ubase = nil;
	if(base){
		ubase = parseurl(base, nil);
		if(ubase == nil)
			return;
	}
	uurl = parseurl(url, ubase);
	if(uurl == nil){
		freeurl(ubase);
		return;
	}
	i = newclient(1);
	c = client[i];
	c->ref++;
	c->baseurl = ubase;
	c->url = uurl;
	sendp(c->creq, nil);
}

void
clientbodyread(Client *c, Req *r)
{
	char e[ERRMAX];

	if(c->url->read == nil){
		respond(r, "unsupported url type");
		return;
	}
	if(c->url->read(c, r) < 0){
		rerrstr(e, sizeof e);
		c->iobusy = 0;
		respond(r, e);
		return;
	}
	c->iobusy = 0;
	respond(r, nil);
}

static void
clientthread(void *a)
{
	Client *c;
	Req *r;

	c = a;
	if(c->plumbed) {
		recvp(c->creq);
		if(c->url == nil){
			fprint(2, "bad url got plumbed\n");
			return;
		}
		clientbodyopen(c, nil);
		replumb(c);
	}
	while((r = recvp(c->creq)) != nil){
		if(fsdebug)
			fprint(2, "clientthread %F\n", &r->ifcall);
		switch(r->ifcall.type){
		case Topen:
			if(c->plumbed) {
				c->plumbed = 0;
				c->ref--;			/* from plumburl() */
				respond(r, nil);
			}
			else
				clientbodyopen(c, r);
			break;
		case Tread:
			clientbodyread(c, r);
			break;
		case Tflush:
			respond(r, nil);
		}
		if(fsdebug)
			fprint(2, "clientthread finished req\n");
	}
}
	
enum
{
	Bool,
	Int,
	String,
	XUrl,
	Fn,
};

typedef struct Ctab Ctab;
struct Ctab {
	char *name;
	int type;
	void *offset;
};

Ctab ctltab[] = {
	"acceptcookies",	Bool,		(void*)offsetof(Ctl, acceptcookies),
	"sendcookies",		Bool,		(void*)offsetof(Ctl, sendcookies),
	"redirectlimit",		Int,		(void*)offsetof(Ctl, redirectlimit),
	"useragent",		String,	(void*)offsetof(Ctl, useragent),
};

Ctab globaltab[] = {
	"chatty9p",		Int,		&chatty9p,
	"fsdebug",		Int,		&fsdebug,
	"cookiedebug",		Int,		&cookiedebug,
	"urldebug",		Int,		&urldebug,
	"httpdebug",		Int,		&httpdebug,
};

Ctab clienttab[] = {
	"baseurl",			XUrl,		(void*)offsetof(Client, baseurl),
	"url",				XUrl,		(void*)offsetof(Client, url),
};

static Ctab*
findcmd(char *cmd, Ctab *tab, int ntab)
{
	int i;

	for(i=0; i<ntab; i++)
		if(strcmp(tab[i].name, cmd) == 0)
			return &tab[i];
	return nil;
}

static void
parseas(Req *r, char *arg, int type, void *a)
{
	Url *u;
	char e[ERRMAX];

	switch(type){
	case Bool:
		if(strcmp(arg, "on")==0 || strcmp(arg, "1")==0)
			*(int*)a = 1;
		else
			*(int*)a = 0;
		break;
	case String:
		free(*(char**)a);
		*(char**)a = estrdup(arg);
		break;
	case XUrl:
		u = parseurl(arg, nil);
		if(u == nil){
			snprint(e, sizeof e, "parseurl: %r");
			respond(r, e);
			return;
		}
		freeurl(*(Url**)a);
		*(Url**)a = u;
		break;
	case Int:
		if(strcmp(arg, "on")==0)
			*(int*)a = 1;
		else
			*(int*)a = atoi(arg);
		break;
	}
	respond(r, nil);
}

int
ctlwrite(Req *r, Ctl *ctl, char *cmd, char *arg)
{
	void *a;
	Ctab *t;

	if((t = findcmd(cmd, ctltab, nelem(ctltab))) == nil)
		return 0;
	a = (void*)((uintptr)ctl+(uintptr)t->offset);
	parseas(r, arg, t->type, a);
	return 1;
}

int
clientctlwrite(Req *r, Client *c, char *cmd, char *arg)
{
	void *a;
	Ctab *t;

	if((t = findcmd(cmd, clienttab, nelem(clienttab))) == nil)
		return 0;
	a = (void*)((uintptr)c+(uintptr)t->offset);
	parseas(r, arg, t->type, a);
	return 1;
}

int
globalctlwrite(Req *r, char *cmd, char *arg)
{
	void *a;
	Ctab *t;

	if((t = findcmd(cmd, globaltab, nelem(globaltab))) == nil)
		return 0;
	a = t->offset;
	parseas(r, arg, t->type, a);
	return 1;
}

static void
ctlfmt(Ctl *c, char *s)
{
	int i;
	void *a;
	char *t;

	for(i=0; i<nelem(ctltab); i++){
		a = (void*)((uintptr)c+(uintptr)ctltab[i].offset);
		switch(ctltab[i].type){
		case Bool:
			s += sprint(s, "%s %s\n", ctltab[i].name, *(int*)a ? "on" : "off");
			break;
		case Int:
			s += sprint(s, "%s %d\n", ctltab[i].name, *(int*)a);
			break;
		case String:
			t = *(char**)a;
			if(t != nil)
				s += sprint(s, "%s %.*s%s\n", ctltab[i].name, utfnlen(t, 100), t, strlen(t)>100 ? "..." : "");
			break;
		}
	}
}

void
ctlread(Req *r, Client *c)
{
	char buf[1024];

	sprint(buf, "%11d \n", c->num);
	ctlfmt(&c->ctl, buf+strlen(buf));
	readstr(r, buf);
	respond(r, nil);
}

void
globalctlread(Req *r)
{
	char buf[1024], *s;
	int i;

	s = buf;
	for(i=0; i<nelem(globaltab); i++)
		s += sprint(s, "%s %d\n", globaltab[i].name, *(int*)globaltab[i].offset);
	ctlfmt(&globalctl, s);
	readstr(r, buf);
	respond(r, nil);
}
