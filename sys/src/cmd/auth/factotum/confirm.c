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

	for(l=&cusewait; *l; l=&(*l)->aux){
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
	ulong tag;
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
	for(l=&cusewait; *l; l=&(*l)->aux){
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
			snprint(msg, sizeof msg, "confirm tag=%lud %A", fss->conf[i].tag, fss->conf[i].key->attr);
			logbufappend(&confbuf, msg);
		}
	if(n == 0){
		respond(r, "no confirmations to wait for (bug)");
		return;
	}
	*cuselast = r;
	r->aux = nil;
	cuselast = &r->aux;
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

	for(l=&needwait; *l; l=&(*l)->aux){
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
	ulong tag;
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
	for(l=&needwait; *l; l=&(*l)->aux){
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

	snprint(msg, sizeof msg, "needkey tag=%lud %s", r->tag, fss->keyinfo);
	logbufappend(&needkeybuf, msg);
	*needlast = r;
	r->aux = nil;
	needlast = &r->aux;
	return 0;
}

