#include "agent.h"

/*
 * This would be simple except for dealing with flushes.
 */

typedef struct Confirm Confirm;
struct Confirm {
	char *what;
	Channel *c;	/* chan(int) */
	int flushed;
	Confirm *next;
};

enum {
	Xnone,
	Xconfirm,
	Xconfirmread,
	Xflushed,
};
typedef struct Xthread Xthread;
struct Xthread {
	Req *req;
	int state;
	Confirm *confirm;
};

Channel *creq;		/* chan(Req*) */
Channel *cconfirm;	/* chan(int) */
Channel *clog;		/* chan(malloc'ed char*) */

static void reqthread(void*);
static void doflush(Req*, Req*);
static Key logkey;
static void logread(Req*);

static void
reqproc(void*)
{
	Req *r;

	threadsetname("reqproc");
	while(r = recvp(creq))
		threadcreate(reqthread, r, STACK);
}

static void
responderrstr(Req *r)
{
	char err[ERRLEN];

	err[0] = '\0';
	errstr(err);
	if(err[0] == '\0')
		strcpy(err, "unknown os error");
	respond(r, err);
}

typedef struct Xclunk Xclunk;
struct Xclunk {
	Key *k;
	void *fidaux;
};

static void
reqthread(void *v)
{
	int n;
	Fid *fid;
	Req *r;
	Key *k;
	char name[20];
	Xclunk *x;
	Xthread *t;

	r = v;
	t = emalloc(sizeof *t);
	t->req = r;
	t->state = Xnone;
	snprint(name, sizeof name, "req %lud", r->tag);
	threadsetname(name);
	*threaddata() = (ulong)t;
	r->aux = t;
	switch(r->type){
	case Topen:
		fid = r->fid;
		k = fid->file->aux;
		if(k && k->proto->open
		&& (fid->aux = k->proto->open(k, fid->file->name, 1)) == nil)
			responderrstr(r);
		else
			respond(r, nil);
		free(t);
		return;

	case Tread:
		fid = r->fid;
		k = fid->file->aux;
		if(k == &logkey){
			logread(r);
			return;
		}
		if(k->proto->read == nil)
			abort();
		else if((n=k->proto->read(fid->aux, r->ofcall.data, r->fcall.count)) < 0)
			responderrstr(r);
		else{
			r->ofcall.count = n;
			respond(r, nil);
		}
		free(t);
		return;

	case Twrite:
		fid = r->fid;
		k = fid->file->aux;
		if(k->proto->write == nil)
			abort();
		if((n=k->proto->write(fid->aux, r->fcall.data, r->fcall.count)) < 0)
			responderrstr(r);
		else{
			r->ofcall.count = n;
			respond(r, nil);
		}
		free(t);
		return;

	case Tclunk:	/* fake request, no response */
		x = (Xclunk*)r->fid;
		if(x->k->proto->close)
			x->k->proto->close(x->fidaux);
		free(x);
		free(r);
		free(t);
		return;

	case Tflush:
		doflush(r, r->oldreq);
		return;
	}
	respond(r, "whoops");
}

static File*
fcreatewalk(File *f, char *name, char *u, char *g, ulong m)
{
	char elem[NAMELEN];
	char *p;
	File *nf;

	if(verbose)
		fprint(2, "fcreatewalk %s\n", name);
	incref(&f->ref);
	while(f && (p = strchr(name, '/'))) {
		memmove(elem, name, p-name);
		elem[p-name] = '\0';
		name = p+1;
		if(strcmp(elem, "") == 0)
			continue;
		nf = fwalk(f, elem);
		decref(&f->ref);
		if(nf == nil) {
			nf = fcreate(f, elem, u, g, CHDIR|0555);
			decref(&f->ref);
		}
		f = nf;
	}
	if(f == nil)
		return nil;

	if(nf = fwalk(f, name)) {
		decref(&f->ref);
		return nf;
	}
	nf = fcreate(f, name, u, g, m);
	decref(&f->ref);
	return nf;
}

void
docreate(Key *k, int must)
{
	File *f;
	ulong p;

	p = k->proto->perm;
	if(p == 0)
		p = 0666;
	f = fcreatewalk(tree->root, k->file, nil, nil, p);
	assert(f->ref.ref == 2);
	if(f == nil) {
		if(must)
			sysfatal("cannot create %s", k->file);
		else
			print("warning: cannot create %s", k->file);
	}
	f->aux = k;
	fclose(f);
}

/*
 * these aren't protocols,
 * but we reuse the interface to
 * implement /config, /confirm, and /log.
 *
 * since some need the read/write offsets but the protocols
 * don't get offsets, we have to handle them
 * specially in fsread and fswrite.
 */

typedef struct Logreq Logreq;
struct Logreq {
	Req *req;
	Logreq *next;
};
typedef struct Logentry Logentry;
struct Logentry {
	char *msg;
	Logentry *next;
};

Config *config;
int unsafe;

Logreq *lrlist;
Logreq **lrtailp = &lrlist;

Logentry *lelist;
Logentry **letailp = &lelist;

static int
logflush(Req *or)
{
	Logreq **rp;
	Logreq *r;

	for(rp=&lrlist; *rp; rp=&(*rp)->next){
		if(or==(*rp)->req){
//threadprint(2, "logflush flushed %p %lud\n", or, or->tag);
			r = *rp;
			*rp = r->next;
			if(lrlist == nil)
				lrtailp = &lrlist;
			free(r);
			return 0;
		}
	}
//threadprint(2, "logflush did not find %p %lud\n", or, or->tag);
	return -1;
}

static void
answerlog(void)
{
	int l;
	Logreq *r;
	Logentry *le;

	while(lrlist && lelist){
		r = lrlist;
		lrlist = r->next;
		if(lrlist == nil)
			lrtailp = &lrlist;

		l = 0;
		while(lelist){
			if(l+strlen(lelist->msg) > r->req->fcall.count)
				break;
			strcpy(r->req->ofcall.data+l, lelist->msg);
			l += strlen(lelist->msg);
			le = lelist;
			lelist = lelist->next;
			if(lelist == nil)
				letailp = &lelist;
			free(le->msg);
			free(le);
		}
//threadprint(2, "respond %p req %p lrlist %p lrtailp %p\n", 
//	r, r->req, lrlist, lrtailp);
		r->req->ofcall.count = l;
		respond(r->req, nil);
		free(r);
	}
}

static void
logread(Req *r)
{
	Logreq *lr;

	lr = emalloc(sizeof *lr);
//threadprint(2, "logread queue lr %p r %p lrtailp %p\n", lr, r, lrtailp);
	lr->req = r;
	*lrtailp = lr;
	lrtailp = &lr->next;

	answerlog();
}

void
agentlog(char *fmt, ...)
{
	va_list va;
	static char buf[128];
	Logentry *le;

	va_start(va, fmt);
	doprint(buf, buf+sizeof buf, fmt, va);
	le = emalloc(sizeof *le);
	le->msg = emalloc(strlen(buf)+40);
	strcpy(le->msg, ctime(time(0)));
	le->msg[strlen(le->msg)-1] = ' ';	/* remove newline */
	strcat(le->msg, buf);
	strcat(le->msg, "\n");
	*letailp = le;
	letailp = &le->next;
	va_end(va);

	answerlog();
}

int confirminuse;

static void*
confirmopen(Key*, char*, int)
{
	if(confirminuse){
		werrstr("file in use");
		return nil;
	}
	confirminuse = 1;
	return emalloc(sizeof(Confirm*));
}

Confirm *clist;
Confirm **ctailp = &clist;

static void
confirmflush(Xthread *t, Confirm *c)
{
	Confirm **cp;

	if(c == nil || c->flushed == 1)
		return;

	for(cp=&clist; *cp; cp=&(*cp)->next)
		if(*cp == c){
			*cp = c->next;
			if(clist == nil)
				ctailp = &clist;
			t->confirm = nil;
//threadprint(2, "flush queued r %lud\n", t->req->tag);
			sendp(c->c, "flushed");
			free(c->c);
			free(c);
			return;
		}

	/* didn't find it, must be pending */
	c->flushed = 1;
//threadprint(2, "flush pending r %lud\n", t->req->tag);
	sendp(c->c, "flushed");
	/* don't free: confirmwrite will do it */
}

static int
confirmread(void *v, void *buf, int n)
{
	char *w;
	Confirm **cp;
	Xthread *t;

	cp = v;
	if(*cp != nil && (*cp)->c != nil){
		werrstr("protocol botch");
		return -1;
	}

	t = *(Xthread**)threaddata();
	t->state = Xconfirmread;
	while(clist == nil && t->state != Xflushed)
		recvul(cconfirm);
	if(t->state == Xflushed){
		werrstr("flushed");
		return -1;
	}
	t->state = Xnone;

	*cp = clist;
	clist = (*cp)->next;
	if(clist == nil)
		ctailp = &clist;

	w = (*cp)->what;
	if(n > strlen(w))
		n = strlen(w);
	memmove(buf, w, n);

	return n;
}

static int
confirmwrite(void *v, void *buf, int n)
{
	char a[10];
	Confirm **cp;

	cp = v;
	if(*cp == nil){
		werrstr("protocol botch");
		return -1;
	}

	if((*cp)->flushed){
		werrstr("request flushed");
		return -1;
	}

	if(n >= sizeof(a))
		n = sizeof(a)-1;
	memmove(a, buf, n);
	a[n] = '\0';
	if(strcmp(a, "yes")==0 || strcmp(a, "yes\n")==0)
		sendp((*cp)->c, nil);
	else
		sendp((*cp)->c, "permission denied");

	chanfree((*cp)->c);
	free(*cp);
	*cp = nil;

	return n;
}

static void
confirmclose(void *v)
{
	confirminuse = 0;
	free(v);
}

int
confirm(char *what)
{
	char *r;
	Confirm *c;
	Xthread *t;

	if(confirminuse == 0)	/* no one is watching, all is permitted */
		return 0;

	t = (Xthread*)*threaddata();
	if(t->state == Xflushed){
		werrstr("flushed");
		return -1;
	}

	t->state = Xconfirm;
//threadprint(2, "confirm r %lud queue\n", t->req->tag);
	c = emalloc(sizeof *c);
	t->confirm = c;
	c->what = what;
	c->c = chancreate(sizeof(ulong), 0);
//threadprint(2, "confirm r %lud queue c %p *ctailp %p\n", t->req->tag, c, *ctailp);
	*ctailp = c;
	ctailp = &c->next;
	nbsendul(cconfirm, 0);
	/* confirmwrite will free c when it gets answered or flushed */
	r = recvp(c->c);
//threadprint(2, "confirm r %lud recv %s%s\n", t->req->tag, r,
//	t->state == Xflushed ? " (flushed)" : "");
	if(t->state != Xflushed)
		t->state = Xnone;
	agentlog("confirm '%s' '%s'\n", what, r ? r : "yes");
	t->confirm = nil;
	if(r != nil){
		werrstr(r);
		return -1;
	}
	return 0;
}

typedef struct Configstate Configstate;
struct Configstate {
	String *s;
	int dirty;
};

static void*
configopen(Key*, char*, int)
{
	Configstate *s;

	s = emalloc(sizeof *s);
	s->s = configtostr(config, unsafe);
	return s;
}

static char*
configread(void *a, void *buf, long *count, vlong offset)
{
	Configstate *s;

	s = a;
	readstr(offset, buf, count, s_to_c(s->s));
	return nil;
}

static char*
configwrite(void *a, void *buf, long *count, vlong offset)
{
	Configstate *s;

	s = a;
	if(offset == 0)
		s_reset(s->s);

	if(memchr(buf, '\0', *count))
		return "NUL in write block";

	s_nappend(s->s, buf, *count);
	s->dirty = 1;
	return nil;
}

static void
configclose(void *a)
{
	Config *c;
	Configstate *s;
	File *f;
	int i, j;

	s = a;
	if(s->dirty == 0) {
		s_free(s->s);
		return;
	}

	c = strtoconfig(config, s->s);
	s_free(s->s);
	free(s);
	if(c == nil) {
		agentlog("error in config file: %r");
		return;
	}

	// update config from c
	agentlog("rewrote config file");

	for(j=0; j<c->nkey; j++)
		c->key[j]->flags |= Fdirty;

	// update any changed keys, removing others
	for(i=0; i<config->nkey; i++) {
		for(j=0; j<c->nkey; j++)
			if(strcmp(config->key[i]->file, c->key[j]->file) == 0) {
				/* reuse config->key[i] because there's a File* pointing at it */
				free(config->key[i]->file);
				if(--config->key[i]->data->ref == 0)
					s_free(config->key[i]->data);
				*config->key[i] = *c->key[j];
				free(c->key[j]);
				c->key[j] = config->key[i];
				config->key[i] = nil;
				c->key[j]->flags &= ~Fdirty;
				break;
			}
		if(j==c->nkey) {
			f = fcreatewalk(tree->root, config->key[i]->file, nil, nil, 0);
			if(f) {
				fremove(f);	
				fclose(f);
			}
			free(config->key[i]->file);
			if(--config->key[i]->data->ref == 0)
				s_free(config->key[i]->data);
			free(config->key[i]);
			config->key[i] = nil;
		}
	}
			
	// add extra keys
	for(j=0; j<c->nkey; j++)
		if(c->key[j]->flags & Fdirty) {
			docreate(c->key[j], 0);
			c->key[j]->flags &= ~Fdirty;
		}

	freeconfig(config);
	config = c;
}

static Proto logproto = {
.name=	"log",
.perm=	0444,
};
static Key logkey = {
.file=		"/log",
.proto=	&logproto
};

static Proto configproto = {
.name=	"config",
.perm=	0666,
.open=	configopen,
.close=	configclose,
};
static Key configkey = {
.file=		"/config",
.proto=	&configproto
};

static Proto confirmproto = {
.name=	"confirm",
.perm=	0666,
.open=	confirmopen,
.read=	confirmread,
.write=	confirmwrite,
.close=	confirmclose,
};
static Key confirmkey = {
.file=		"/confirm",
.proto=	&confirmproto,
};

void
fsopen(Req *r, Fid*, int, Qid*)
{
	sendp(creq, r);
}

void
fsread(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	Key *k;

	k = fid->file->aux;
	if(k == &configkey)
		respond(r, configread(fid->aux, buf, count, offset));
	else
		sendp(creq, r);
}

void
fswrite(Req *r, Fid *fid, void *buf, long *count, vlong offset)
{
	Key *k;

	k = fid->file->aux;
	if(k == &configkey)
		respond(r, configwrite(fid->aux, buf, count, offset));
	else
		sendp(creq, r);
}

void
fsattach(Req *r, Fid *fid, char *spec, Qid *qid)
{
	static int first = 1;

	if(first){
		first = 0;
		creq = chancreate(sizeof(Req*), 0);
		cconfirm = chancreate(sizeof(int), 0);
		proccreate(reqproc, nil, STACK);
	}

	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}

	fid->file = r->pool->srv->tree->root;
	*qid = fid->file->qid;
	respond(r, nil);
}

void
fsclunkaux(Fid *fid)
{
	Req *r;
	Xclunk *x;

	if(fid->omode != -1 && (fid->file->aux || fid->aux)){
		/* fake request */
		r = emalloc(sizeof *r);
		r->type = Tclunk;
		x = emalloc(sizeof *x);
		r->fid = (Fid*)x;
		x->k = fid->file->aux;
		x->fidaux = fid->aux;
		sendp(creq, r);
	}
}

static void
doflush(Req *r, Req *or)
{
	Xthread *t;

	if(logflush(or) == 0){
		respond(r, nil);
		return;
	}

	t = or->aux;
	/* break out of any blocking things */
	switch(t->state){
	case Xconfirm:
		t->state = Xflushed;
		confirmflush(t, t->confirm);
		break;
	case Xconfirmread:
		t->state = Xflushed;
		nbsendul(cconfirm, 0);
		break;
	default:
		t->state = Xflushed;
		break;
	}
	while(or->responded==0)
		yield();
	respond(r, nil);
}

void
fsflush(Req *r, Req*)
{
	sendp(creq, r);
}

void
setconfig(Config *c)
{
	int i;

	config = c;

	docreate(&configkey, 1);
	docreate(&logkey, 1);
	docreate(&confirmkey, 1);

	for(i=0; i<c->nkey; i++)
		docreate(c->key[i], 0);
}
