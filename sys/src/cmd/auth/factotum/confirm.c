/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "dat.h"

Logbuf confbuf;

Req *cusewait;		/* requests waiting for confirmation */
Req **cuselast = &cusewait;

void
confirmread(Req *r)
{
	logbufread(&confbuf, r);
}

void
confirmflush(Req *r)
{
	Req **l;

	for(l=(Req **)cusewait; *l; l=(Req **)(*l)->aux){
		if(*l == r){
			*l = r->aux;
			if(r->aux == nil)
				cuselast = l;
			closereq(r);
			break;
		}
	}
	logbufflush(&confbuf, r);
}

static int
hastag(Fsstate *fss, int tag, int *tagoff)
{
	int i;

	for(i=0; i<fss->nconf; i++)
		if(fss->conf[i].tag == tag){
			*tagoff = i;
			return 1;
		}
	return 0;
}

int
confirmwrite(char *s)
{
	char *t, *ans;
	int allow, tagoff;
	uint32_t tag;
	Attr *a;
	Fsstate *fss;
	Req *r, **l;

	a = _parseattr(s);
	if(a == nil){
		werrstr("empty write");
		return -1;
	}
	if((t = _strfindattr(a, "tag")) == nil){
		werrstr("no tag");
		return -1;
	}
	tag = strtoul(t, 0, 0);
	if((ans = _strfindattr(a, "answer")) == nil){
		werrstr("no answer");
		return -1;
	}
	if(strcmp(ans, "yes") == 0)
		allow = 1;
	else if(strcmp(ans, "no") == 0)
		allow = 0;
	else{
		werrstr("bad answer");
		return -1;
	}
	r = nil;
	tagoff = -1;
	for(l=(Req **)cusewait; *l; l=(Req **)(*l)->aux){
		r = *l;
		if(hastag(r->fid->aux, tag, &tagoff)){
			*l = r->aux;
			if(r->aux == nil)
				cuselast = l;
			break;
		}
	}
	if(r == nil || tagoff == -1){
		werrstr("tag not found");
		return -1;
	}
	fss = r->fid->aux;
	fss->conf[tagoff].canuse = allow;
	rpcread(r);
	return 0;
}

void
confirmqueue(Req *r, Fsstate *fss)
{
	int i, n;
	char msg[1024];

	if(*confirminuse == 0){
		respond(r, "confirm is closed");
		return;
	}

	n = 0;
	for(i=0; i<fss->nconf; i++)
		if(fss->conf[i].canuse == -1){
			n++;
			snprint(msg, sizeof msg, "confirm tag=%lu %A", fss->conf[i].tag, fss->conf[i].key->attr);
			logbufappend(&confbuf, msg);
		}
	if(n == 0){
		respond(r, "no confirmations to wait for (bug)");
		return;
	}
	*cuselast = r;
	r->aux = nil;
	cuselast = (Req **)r->aux;
}

/* Yes, I am unhappy that the code below is a copy of the code above. */

Logbuf needkeybuf;
Req *needwait;		/* requests that need keys */
Req **needlast = &needwait;

void
needkeyread(Req *r)
{
	logbufread(&needkeybuf, r);
}

void
needkeyflush(Req *r)
{
	Req **l;

	for(l=(Req **)needwait; *l; l=(Req **)(*l)->aux){
		if(*l == r){
			*l = r->aux;
			if(r->aux == nil)
				needlast = l;
			closereq(r);
			break;
		}
	}
	logbufflush(&needkeybuf, r);
}

int
needkeywrite(char *s)
{
	char *t;
	uint32_t tag;
	Attr *a;
	Req *r, **l;

	a = _parseattr(s);
	if(a == nil){
		werrstr("empty write");
		return -1;
	}
	if((t = _strfindattr(a, "tag")) == nil){
		werrstr("no tag");
		return -1;
	}
	tag = strtoul(t, 0, 0);
	r = nil;
	for(l=(Req **)needwait; *l; l=(Req **)(*l)->aux){
		r = *l;
		if(r->tag == tag){
			*l = r->aux;
			if(r->aux == nil)
				needlast = l;
			break;
		}
	}
	if(r == nil){
		werrstr("tag not found");
		return -1;
	}
	rpcread(r);
	return 0;
}

int
needkeyqueue(Req *r, Fsstate *fss)
{
	char msg[1024];

	if(*needkeyinuse == 0)
		return -1;

	snprint(msg, sizeof msg, "needkey tag=%lu %s", r->tag, fss->keyinfo);
	logbufappend(&needkeybuf, msg);
	*needlast = r;
	r->aux = nil;
	needlast = (Req **)r->aux;
	return 0;
}

