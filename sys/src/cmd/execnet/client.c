#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"

int nclient;
Client **client;
#define Zmsg ((Msg*)~0)
char nocmd[] = "";

static void readthread(void*);
static void writethread(void*);
static void kickwriter(Client*);

int
newclient(void)
{
	int i;
	Client *c;

	for(i=0; i<nclient; i++)
		if(client[i]->ref==0 && !client[i]->moribund)
			return i;

	c = emalloc(sizeof(Client));
	c->writerkick = chancreate(sizeof(void*), 1);
	c->execpid = chancreate(sizeof(ulong), 0);
	c->cmd = nocmd;

	c->readerproc = ioproc();
	c->writerproc = ioproc();
	c->num = nclient;
	if(nclient%16 == 0)
		client = erealloc(client, (nclient+16)*sizeof(client[0]));
	client[nclient++] = c;
	return nclient-1;
}

void
die(Client *c)
{
	Msg *m, *next;
	Req *r, *rnext;

	c->moribund = 1;
	kickwriter(c);
	iointerrupt(c->readerproc);
	iointerrupt(c->writerproc);
	if(--c->activethread == 0){
		if(c->cmd != nocmd){
			free(c->cmd);
			c->cmd = nocmd;
		}
		c->pid = 0;
		c->moribund = 0;
		c->status = Closed;
		for(m=c->mq; m && m != Zmsg; m=next){
			next = m->link;
			free(m);
		}
		c->mq = nil;
		if(c->rq != nil){
			for(r=c->rq; r; r=rnext){
				rnext = r->aux;
				respond(r, "hangup");
			}
			c->rq = nil;
		}
		if(c->wq != nil){
			for(r=c->wq; r; r=rnext){
				rnext = r->aux;
				respond(r, "hangup");
			}
			c->wq = nil;
		}
		c->rq = nil;
		c->wq = nil;
		c->emq = nil;
		c->erq = nil;
		c->ewq = nil;
	}
}

void
closeclient(Client *c)
{
	if(--c->ref == 0){
		if(c->pid > 0)
			postnote(PNPROC, c->pid, "kill");
		c->status = Hangup;
		close(c->fd[0]);
		c->fd[0] = c->fd[1] = -1;
		c->moribund = 1;
		kickwriter(c);
		iointerrupt(c->readerproc);
		iointerrupt(c->writerproc);		
		c->activethread++;
		die(c);
	}
}

void
queuerdreq(Client *c, Req *r)
{
	if(c->rq==nil)
		c->erq = &c->rq;
	*c->erq = r;
	r->aux = nil;
	c->erq = (Req**)&r->aux;
}

void
queuewrreq(Client *c, Req *r)
{
	if(c->wq==nil)
		c->ewq = &c->wq;
	*c->ewq = r;
	r->aux = nil;
	c->ewq = (Req**)&r->aux;
}

void
queuemsg(Client *c, Msg *m)
{
	if(c->mq==nil)
		c->emq = &c->mq;
	*c->emq = m;
	if(m != Zmsg){
		m->link = nil;
		c->emq = (Msg**)&m->link;
	}else
		c->emq = nil;
}

void
matchmsgs(Client *c)
{
	Req *r;
	Msg *m;
	int n, rm;

	while(c->rq && c->mq){
		r = c->rq;
		c->rq = r->aux;

		rm = 0;
		m = c->mq;
		if(m == Zmsg){
			respond(r, "execnet: no more data");
			break;
		}
		n = r->ifcall.count;
		if(n >= m->ep - m->rp){
			n = m->ep - m->rp;
			c->mq = m->link;
			rm = 1;
		}
		if(n)
			memmove(r->ofcall.data, m->rp, n);
		if(rm)
			free(m);
		else
			m->rp += n;
		r->ofcall.count = n;
		respond(r, nil);
	}
}

void
findrdreq(Client *c, Req *r)
{
	Req **l;

	for(l=&c->rq; *l; l=(Req**)&(*l)->aux){
		if(*l == r){
			*l = r->aux;
			if(*l == nil)
				c->erq = l;
			respond(r, "flushed");
			break;
		}
	}
}

void
findwrreq(Client *c, Req *r)
{
	Req **l;

	for(l=&c->wq; *l; l=(Req**)&(*l)->aux){
		if(*l == r){
			*l = r->aux;
			if(*l == nil)
				c->ewq = l;
			respond(r, "flushed");
			return;
		}
	}
}

void
dataread(Req *r, Client *c)
{
	queuerdreq(c, r);
	matchmsgs(c);
}

static void
readthread(void *a)
{
	uchar *buf;
	int n;
	Client *c;
	Ioproc *io;
	Msg *m;
	char tmp[32];

	c = a;
	snprint(tmp, sizeof tmp, "read%d", c->num);
	threadsetname(tmp);

	buf = emalloc(8192);
	io = c->readerproc;
	while((n = ioread(io, c->fd[0], buf, 8192)) >= 0){
		m = emalloc(sizeof(Msg)+n);
		m->rp = (uchar*)&m[1];
		m->ep = m->rp + n;
		if(n)
			memmove(m->rp, buf, n);
		queuemsg(c, m);
		matchmsgs(c);
	}
	queuemsg(c, Zmsg);
	free(buf);
	die(c);
}

static void
kickwriter(Client *c)
{
	nbsendp(c->writerkick, nil);
}

void
clientflush(Req *or, Client *c)
{
	if(or->ifcall.type == Tread)
		findrdreq(c, or);
	else{
		if(c->execreq == or){
			c->execreq = nil;
			iointerrupt(c->writerproc);
		}
		findwrreq(c, or);
		if(c->curw == or){
			c->curw = nil;
			iointerrupt(c->writerproc);
			kickwriter(c);
		}
	}
}

void
datawrite(Req *r, Client *c)
{
	queuewrreq(c, r);
	kickwriter(c);
}

static void
writethread(void *a)
{
	char e[ERRMAX];
	uchar *buf;
	int n;
	Ioproc *io;
	Req *r;
	Client *c;
	char tmp[32];

	c = a;
	snprint(tmp, sizeof tmp, "write%d", c->num);
	threadsetname(tmp);

	buf = emalloc(8192);
	io = c->writerproc;
	for(;;){
		while(c->wq == nil){
			if(c->moribund)
				goto Out;
			recvp(c->writerkick);
			if(c->moribund)
				goto Out;
		}
		r = c->wq;
		c->wq = r->aux;
		c->curw = r;
		n = iowrite(io, c->fd[1], r->ifcall.data, r->ifcall.count);
		if(chatty9p)
			fprint(2, "io->write returns %d\n", n);
		if(n >= 0){
			r->ofcall.count = n;
			respond(r, nil);
		}else{
			rerrstr(e, sizeof e);
			respond(r, e);
		}
	}
Out:
	free(buf);
	die(c);
}

static void
execproc(void *a)
{
	int i, fd;
	Client *c;
	char tmp[32];

	c = a;
	snprint(tmp, sizeof tmp, "execproc%d", c->num);
	threadsetname(tmp);
	if(pipe(c->fd) < 0){
		rerrstr(c->err, sizeof c->err);
		sendul(c->execpid, -1);
		return;
	}
	rfork(RFFDG);
	fd = c->fd[1];
	close(c->fd[0]);
	dup(fd, 0);
	dup(fd, 1);
	for(i=3; i<100; i++)	/* should do better */
		close(i);
	strcpy(c->err, "exec failed");
	procexecl(c->execpid, "/bin/rc", "rc", "-c", c->cmd, nil);
}

static void
execthread(void *a)
{
	Client *c;
	int p;
	char tmp[32];

	c = a;
	snprint(tmp, sizeof tmp, "exec%d", c->num);
	threadsetname(tmp);
	c->execpid = chancreate(sizeof(ulong), 0);
	proccreate(execproc, c, STACK);
	p = recvul(c->execpid);
	chanfree(c->execpid);
	c->execpid = nil;
	close(c->fd[1]);
	c->fd[1] = c->fd[0];
	if(p != -1){
		c->pid = p;
		c->activethread = 2;
		threadcreate(readthread, c, STACK);
		threadcreate(writethread, c, STACK);
		if(c->execreq)
			respond(c->execreq, nil);
	}else{
		if(c->execreq)
			respond(c->execreq, c->err);
	}
}

void
ctlwrite(Req *r, Client *c)
{
	char *f[3], *s, *p;
	int nf;

	s = emalloc(r->ifcall.count+1);
	memmove(s, r->ifcall.data, r->ifcall.count);
	s[r->ifcall.count] = '\0';

	f[0] = s;
	p = strchr(s, ' ');
	if(p == nil)
		nf = 1;
	else{
		*p++ = '\0';
		f[1] = p;
		nf = 2;
	}

	if(f[0][0] == '\0'){
		free(s);
		respond(r, nil);
		return;
	}

	r->ofcall.count = r->ifcall.count;
	if(strcmp(f[0], "hangup") == 0){
		if(c->pid == 0){
			respond(r, "connection already hung up");
			goto Out;
		}
		postnote(PNPROC, c->pid, "kill");
		respond(r, nil);
		goto Out;
	}

	if(strcmp(f[0], "connect") == 0){
		if(c->cmd != nocmd){
			respond(r, "already have connection");
			goto Out;
		}
		if(nf == 1){
			respond(r, "need argument to connect");
			goto Out;
		}
		c->status = Exec;
		if(p = strrchr(f[1], '!'))
			*p = '\0';
		c->cmd = emalloc(4+1+strlen(f[1])+1);
		strcpy(c->cmd, "exec ");
		strcat(c->cmd, f[1]);
		c->execreq = r;
		threadcreate(execthread, c, STACK);
		goto Out;
	}

	respond(r, "bad or inappropriate control message");
Out:
	free(s);
}
