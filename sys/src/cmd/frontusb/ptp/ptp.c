#include <u.h>
#include <libc.h>
#include <thread.h>
#include <auth.h>
#include <fcall.h>
#include <9p.h>

#include "usb.h"

enum
{
	Qroot,
	Qstore,
	Qobj,
	Qthumb,
};

enum {
	/* flags */
	DataSend			=	0x00010000,
	DataRecv			=	0x00020000,
	OutParam			=	0x00040000,

	/* rpc codes */
	OpenSession		=	0x1002,
	CloseSession		=	0x1003,
	GetStorageIds		=	0x1004,
	GetStorageInfo		=	0x1005,
	GetObjectHandles	=	0x1007,
	GetObjectInfo		=	0x1008,
	GetObject			=	0x1009,
	GetThumb		=	0x100A,
	DeleteObject		=	0x100B,
	GetPartialObject	=	0x101B,

	Maxio			=	0x2000,
};

typedef struct Ptprpc Ptprpc;
typedef struct Node Node;

struct Ptprpc
{
	uchar	length[4];
	uchar	type[2];
	uchar	code[2];
	uchar	transid[4];
	uchar	d[500];
};

struct Node
{
	Dir		d;

	Node	*parent;
	Node	*next;
	Node	*child;

	int		store;
	int		handle;
	int		format;

	void		*data;
	int		ndata;
};

enum {
	In,
	Out,
	Int,
	Setup,
};

static Dev *usbep[Setup+1];

static int debug = 0;
static ulong time0;
static int maxpacket = 64;
static int sessionId = 1;
static int transId = 1;

static Node **nodes;
static int nnodes;

static Channel *iochan;

char Eperm[] = "permission denied";
char Einterrupt[] = "interrupted";

#define PATH(type, n)		((uvlong)(type)|((uvlong)(n)<<4))
#define TYPE(path)			((int)((path)&0xF))
#define NUM(path)			((int)((path)>>4))

static void
hexdump(char *prefix, uchar *p, int n)
{
	char *s;
	int i;
	int m;

	m = 12;
	s = emalloc9p(1+((n+1)*((m*6)+7)));
	s[0] = '\0';
	for(i=0; i<n; i++){
		int printable;
		char x[8];
		if((i % m)==0){
			sprint(x, "\n%.4x: ", i);
			strcat(s, x);
		}
		printable = (p[i] >= 32 && p[i]<=127);
		sprint(x, "%.2x %c  ", (int)p[i],  printable ? p[i] : '.');
		strcat(s, x);
	}
	fprint(2, "%20-s: %6d bytes %s\n", prefix, n, s);
	free(s);
}

static int
wasinterrupt(void)
{
	char err[ERRMAX];

	rerrstr(err, sizeof(err));
	if(strstr(err, Einterrupt) || strstr(err, "timed out")){
		werrstr(Einterrupt);
		return 1;
	}
	return 0;
}

static char *
ptperrstr(int code)
{
	static char *a[] = {
		"undefined",
		nil ,
		"general error" ,
		"session not open" ,
		 "invalid transaction id" ,
		 "operation not supported" ,
		 "parameter not supported" ,
		 "incomplete transfer" ,
		 "invalid storage id" ,
		 "invalid object handle" ,
		 "device prop not supported" ,
		 "invalid object format code" ,
		 "storage full" ,
		 "object write protected" ,
		 "store read only" ,
		 "access denied" ,
		 "no thumbnail present" ,
		 "self test failed" ,
		 "partial deletion" ,
		 "store not available" ,
		 "specification by format unsupported" ,
		 "no valid object info" ,
		 "invalid code format" ,
		 "unknown vendor code",
		"capture already terminated",
		"device busy",
		"invalid parent object",
		"invalid device prop format",
		"invalid device prop value",
		"invalid parameter",
		"session already opend",
		"transaction canceld",
		"specification of destination unsupported"
	};

	code -= 0x2000;
	if(code < 0)
		return nil;
	if(code >= nelem(a))
		return "invalid error number";
	return a[code];
}

static int
ptpcheckerr(Ptprpc *rpc, int type, int transid, int length)
{
	char *s;

	if(length < 4+2+2+4){
		werrstr("short response: %d < %d", length, 4+2+2+4);
		return 1;
	}
	if(GET4(rpc->length) < length){
		werrstr("unexpected response length 0x%x < 0x%x", GET4(rpc->length), length);
		return 1;
	}
	if(GET4(rpc->transid) != transid){
		werrstr("unexpected transaction id 0x%x != 0x%x", GET4(rpc->transid), transid);
		return 1;
	}
	if(GET2(rpc->type) != type){
		werrstr("unexpected response type 0x%x != 0x%x", GET2(rpc->type), type);
		return 1;
	}
	if(s = ptperrstr(GET2(rpc->code))){
		werrstr("%s", s);
		return -GET2(rpc->code);
	}
	return 0;
}

static int
vptprpc(Ioproc *io, int code, int flags, va_list a)
{
	Ptprpc rpc;
	int np, n, t, i, l;
	uchar *b, *p, *e;

	np = flags & 0xF;
	n = 4+2+2+4+4*np;
	t = transId++;

	PUT4(rpc.length, n);
	PUT2(rpc.type, 1);
	PUT2(rpc.code, code);
	PUT4(rpc.transid, t);

	for(i=0; i<np; i++){
		int x = va_arg(a, int);
		PUT4(rpc.d + i*4, x);
	}
	if(debug)
		hexdump("req>", (uchar*)&rpc, n);
	werrstr("");
	if(iowrite(io, usbep[Out]->dfd, &rpc, n) != n){
		wasinterrupt();
		return -1;
	}

	if(flags & DataSend){
		void *sdata;
		int sdatalen;

		sdata = va_arg(a, void*);
		sdatalen = va_arg(a, int);

		b = (uchar*)sdata;
		p = b;
		e = b + sdatalen;

		l = 4+2+2+4+sdatalen;
		PUT4(rpc.length, l);
		PUT2(rpc.type, 2);

		if((n = sdatalen) > sizeof(rpc.d))
			n = sizeof(rpc.d);
		memmove(rpc.d, p, n);
		p += n;
		n += (4+2+2+4);

		if(debug)
			hexdump("data>", (uchar*)&rpc, n);
		if(iowrite(io, usbep[Out]->dfd, &rpc, n) != n){
			wasinterrupt();
			return -1;
		}
		while(p < e){
			n = e - p;
			if(n > Maxio)
				n = Maxio;
			if((n = iowrite(io, usbep[Out]->dfd, p, n)) < 0){
				wasinterrupt();
				return -1;
			}
			p += n;
		}
	}

	if(flags & DataRecv){
		void **prdata;
		int *prdatalen;

		prdata = va_arg(a, void**);
		prdatalen = va_arg(a, int*);

		*prdata = nil;
		*prdatalen = 0;

		while((n = ioread(io, usbep[In]->dfd, &rpc, usbep[In]->maxpkt)) <= 0){
			if(n < 0){
				wasinterrupt();
				return -1;
			}
		}
		if((l = ptpcheckerr(&rpc, 2, t, n)) < 0)
			return -1;
		if(l && GET2(rpc.type) == 3)
			goto Resp1;

		l = GET4(rpc.length);
		l -= (4+2+2+4);
		n -= (4+2+2+4);
	
		b = emalloc9p(l);
		p = b;
		e = b+l;
		if(n <= l){
			if(debug)
				hexdump("data<", rpc.d, n);
			memmove(p, rpc.d, n);
			p += n;
			while(p < e){
				n = e-p;
				if(n > Maxio)
					n = Maxio;
				while((n = ioread(io, usbep[In]->dfd, p, n)) <= 0){
					if(n < 0){
						wasinterrupt();
						free(b);
						return -1;
					}
				}
				if(debug)
					hexdump("data<", p, n);
				p += n;
			}
			*prdata = b;
			*prdatalen = e-b;
		} else {
			if(debug)
				hexdump("data<", rpc.d, l);
			memmove(p, rpc.d, l);
			*prdata = b;
			*prdatalen = e-b;

			n -= l;
			memmove(&rpc, rpc.d+l, n);
			goto Resp1;
		}
	}

	while((n = ioread(io, usbep[In]->dfd, &rpc, usbep[In]->maxpkt)) <= 0){
		if(n < 0){
			wasinterrupt();
			return -1;
		}
	}
Resp1:
	if(debug)
		hexdump("resp<", (uchar*)&rpc, n);
	if(ptpcheckerr(&rpc, 3, t, n))
		return -1;
	if(flags & OutParam){
		int *pp;

		for(i=0; i<5; i++){
			if((pp = va_arg(a, int*)) == nil)
				break;
			*pp = GET4(rpc.d + i*4);
		}
	}
	return 0;
}

static int
ptprpc(Req *r, int code, int flags, ...)
{
	va_list va;
	Channel *c;
	Ioproc *io;
	Alt a[3];
	char *m;
	int i;

	i = -1;
	c = nil;
	io = nil;
	m = Einterrupt;
	a[0].op = CHANRCV;
	a[0].c = iochan;
	a[0].v = &io;
	if(r){
		c = chancreate(sizeof(char*), 0);
		a[1].op = CHANRCV;
		a[1].c = c;
		a[1].v = &m;
		a[2].op = CHANEND;
		r->aux = c;
		srvrelease(r->srv);
	} else
		a[1].op = CHANEND;
	if(alt(a) == 0){
		va_start(va, flags);
		i = vptprpc(io, code, flags, va);
		va_end(va);
		if(i < 0 && debug)
			fprint(2, "rpc: %r\n");
	} else
		werrstr("%s", m);
	if(r){
		srvacquire(r->srv);
		r->aux = nil;
	}
	if(io)
		sendp(iochan, io);
	if(c)
		chanfree(c);
	return i;
}

static int*
ptparray4(uchar *d, uchar *e)
{
	int *a, i, n;

	if(d + 4 > e)
		return nil;
	n = GET4(d);
	d += 4;
	if(d + n*4 > e)
		return nil;
	a = emalloc9p((1+n) * sizeof(int));
	a[0] = n;
	for(i=0; i<n; i++){
		a[i+1] = GET4(d);
		d += 4;
	}
	return a;
}

static char*
ptpstring2(uchar *d, uchar *e)
{
	int n, i;
	char *s, *p;

	if(d+1 > e)
		return nil;
	n = *d;
	d++;
	if(d + n*2 > e)
		return nil;
	p = s = emalloc9p((n+1)*UTFmax);
	for(i=0; i<n; i++){
		Rune r;

		r = GET2(d);
		d += 2;
		if(r == 0)
			break;
		p += runetochar(p, &r);
	}
	*p = 0;
	return s;
}

static void
cleardir(Dir *d)
{
	free(d->name);
	free(d->uid);
	free(d->gid);
	free(d->muid);
	memset(d, 0, sizeof(*d));
}

static void
copydir(Dir *d, Dir *s)
{
	memmove(d, s, sizeof(*d));
	if(d->name)
		d->name = estrdup9p(d->name);
	if(d->uid)
		d->uid = estrdup9p(d->uid);
	if(d->gid)
		d->gid = estrdup9p(d->gid);
	if(d->muid)
		d->muid = estrdup9p(d->muid);
}

static Node*
cachednode(uvlong path, Node ***pf)
{
	Node *x;
	int i;

	if(pf)
		*pf = nil;
	for(i=0; i<nnodes; i++){
		if((x = nodes[i]) == nil){
			if(pf)
				*pf = &nodes[i];
			continue;
		}
		if(x->d.qid.path == path)
			return x;
	}
	return nil;
}

static Node*
getnode(Req *r, uvlong path)
{
	Node *x, *y, **f;
	uchar *p;
	int np;
	char *s;

	if(x = cachednode(path, &f))
		return x;

	y = nil;
	x = emalloc9p(sizeof(*x));
	memset(x, 0, sizeof(*x));
	x->d.qid.path = path;
	x->d.uid = estrdup9p("ptp");
	x->d.gid = estrdup9p("usb");
	x->d.atime = x->d.mtime = time0;

	p = nil;
	np = 0;
	switch(TYPE(path)){
	case Qroot:
		x->d.qid.type = QTDIR;
		x->d.mode = DMDIR|0777;
		x->d.name = estrdup9p("/");
		goto Addnode;

	case Qstore:
		x->store = NUM(path);
		x->handle = 0xffffffff;
		x->d.qid.type = QTDIR;
		x->d.mode = DMDIR|0777;

		if(ptprpc(r, GetStorageInfo, 1|DataRecv, NUM(path), &p, &np) < 0)
			break;
		if(debug)
			hexdump("storageinfo", p, np);
		if(np < 26){
			werrstr("bad storageinfo");
			break;
		}

		if((x->d.name = ptpstring2(p+26, p+np)) == nil){
			werrstr("bad storageinfo");
			break;
		}

		free(p);
		goto Addnode;

	case Qobj:
	case Qthumb:
		if(ptprpc(r, GetObjectInfo, 1|DataRecv, NUM(path), &p, &np) < 0)
			break;
		if(debug)
			hexdump("objectinfo", p, np);
		if(np < 52){
			werrstr("bad objectinfo");
			break;
		}

		/*
		 * another proc migh'v come in and done it for us,
		 * so check the cache again.
		 */
		if(y = cachednode(path, &f))
			break;

		if((x->d.name = ptpstring2(p+52, p+np)) == nil){
			werrstr("bad objectinfo");
			break;
		}
		x->handle = NUM(path);
		x->store = GET4(p);
		x->format = GET2(p+4);
		if(x->format == 0x3001){
			x->d.qid.type = QTDIR;
			x->d.mode = DMDIR|0777;
		} else {
			x->d.mode = 0666;
			if(TYPE(path) == Qthumb){
				char *t;

				t = emalloc9p(8 + strlen(x->d.name));
				sprint(t, "thumb_%s", x->d.name);
				free(x->d.name);
				x->d.name = t;

				x->d.length = GET4(p+14);
			} else {
				x->d.length = GET4(p+8);
			}
		}
		if(s = ptpstring2(p+(53+p[52]*2), p+np)){
			if(strlen(s) >= 15){
				Tm t;

				// 0123 45 67 8 9A BC DF
				// 2008 12 26 T 00 21 18
				memset(&t, 0, sizeof(t));

				s[0x10] = 0;
				t.sec = atoi(s+0xD);
				s[0xD] = 0;
				t.min = atoi(s+0xB);
				s[0xB] = 0;
				t.hour = atoi(s+0x9);
				s[0x8] = 0;
				t.mday = atoi(s+0x6);
				s[0x6] = 0;
				t.mon = atoi(s+0x4) - 1;
				s[0x4] = 0;
				t.year = atoi(s) - 1900;

				x->d.atime = x->d.mtime = tm2sec(&t);
			}
			free(s);
		}
		free(p);
	Addnode:
		if(f == nil){
			if(nnodes % 64 == 0)
				nodes = erealloc9p(nodes, sizeof(nodes[0]) * (nnodes + 64));
			f = &nodes[nnodes++];
		}
		return *f = x;
	}

	cleardir(&x->d);
	free(x);
	free(p);
	return y;
}

static void
freenode(Node *nod)
{
	int i;

	/* remove the node from the tree */
	for(i=0; i<nnodes; i++){
		if(nod == nodes[i]){
			nodes[i] = nil;
			break;
		}
	}
	cleardir(&nod->d);
	free(nod->data);
	free(nod);
}

static int
readchilds(Req *r, Node *nod)
{
	int e, i;
	int *a;
	uchar *p;
	int np;
	Node *x, **xx;

	e = 0;
	switch(TYPE(nod->d.qid.path)){
	case Qroot:
		if(ptprpc(r, GetStorageIds, 0|DataRecv, &p, &np) < 0)
			return -1;
		a = ptparray4(p, p+np);
		free(p);
		xx = &nod->child;
		*xx = nil;
		for(i=0; a && i<a[0]; i++){
			if((x = getnode(r, PATH(Qstore, a[i+1]))) == nil){
				e = -1;
				break;
			}
			x->parent = nod;
			*xx = x;
			xx = &x->next;
			*xx = nil;
		}		
		free(a);
		break;

	case Qstore:
	case Qobj:
		if(ptprpc(r, GetObjectHandles, 3|DataRecv, nod->store, 0, nod->handle, &p, &np) < 0)
			return -1;
		a = ptparray4(p, p+np);
		free(p);
		xx = &nod->child;
		*xx = nil;
		for(i=0; a && i<a[0]; i++){
			if((x = getnode(r, PATH(Qobj, a[i+1]))) == nil){
				e = -1;
				break;
			}
			x->parent = nod;
			*xx = x;
			xx = &x->next;
			*xx = nil;

			/* skip thumb when not image format */
			if((x->format & 0xFF00) != 0x3800)
				continue;
			if((x = getnode(r, PATH(Qthumb, a[i+1]))) == nil){
				e = -1;
				break;
			}
			x->parent = nod;
			*xx = x;
			xx = &x->next;
			*xx = nil;
		}
		free(a);
		break;
	}

	return e;
}

static void
fsattach(Req *r)
{
	if(r->ifcall.aname && r->ifcall.aname[0]){
		respond(r, "invalid attach specifier");
		return;
	}
	r->fid->qid.path = PATH(Qroot, 0);
	r->fid->qid.type = QTDIR;
	r->fid->qid.vers = 0;
	r->ofcall.qid = r->fid->qid;
	respond(r, nil);
}

static void
fsstat(Req *r)
{
	Node *nod;

	if((nod = getnode(r, r->fid->qid.path)) == nil){
		responderror(r);
		return;
	}
	copydir(&r->d, &nod->d);
	respond(r, nil);
}

static int
nodegen(int i, Dir *d, void *aux)
{
	Node *nod = aux;

	for(nod=nod->child; nod && i; nod=nod->next, i--)
		;
	if(i==0 && nod){
		copydir(d, &nod->d);
		return 0;
	}
	return -1;
}

static char*
fswalk1(Req *r, char *name, Qid *qid)
{
	static char buf[ERRMAX];
	uvlong path;
	Node *nod;
	Fid *fid;

	fid = r->newfid;
	path = fid->qid.path;
	if(!(fid->qid.type&QTDIR))
		return "walk in non-directory";
	if(nod = getnode(r, path)){
		if(strcmp(name, "..") == 0){
			if(nod = nod->parent)
				*qid = nod->d.qid;
			return nil;
		}
		if(readchilds(r, nod) == 0){
			for(nod=nod->child; nod; nod=nod->next){
				if(strcmp(nod->d.name, name) == 0){
					*qid = nod->d.qid;
					return nil;
				}
			}
			return "directory entry not found";
		}
	}
	rerrstr(buf, sizeof(buf));
	return buf;
}

static char*
oldwalk1(Fid *fid, char *name, void *arg)
{
	Qid qid;
	char *e;
	Req *r;

	r = arg;
	assert(fid == r->newfid);
	if(e = fswalk1(r, name, &qid))
		return e;
	fid->qid = qid;
	return nil;
}

static char*
oldclone(Fid *, Fid *, void *)
{
	return nil;
}

static void
fswalk(Req *r)
{
	walkandclone(r, oldwalk1, oldclone, r);
}

static void
fsread(Req *r)
{
	uvlong path;
	Node *nod;
	uchar *p;
	int np;

	np = 0;
	p = nil;
	path = r->fid->qid.path;
	if(nod = getnode(r, path)){
		switch(TYPE(path)){
		case Qroot:
		case Qstore:
		case Qobj:
			if(nod->d.qid.type & QTDIR){
				if(readchilds(r, nod) < 0)
					break;
				dirread9p(r, nodegen, nod);
				respond(r, nil);
				return;
			}
			if(nod->data == nil){
				int offset, count, pos;

				offset = r->ifcall.offset;
				if(offset >= nod->d.length){
					r->ofcall.count = 0;
					respond(r, nil);
					return;
				}
				/* are these people stupid? */
				pos = (offset == 0) ? 1 : 2;
				count = r->ifcall.count;
				if((count + offset) > nod->d.length){
					count = nod->d.length - offset;
					pos = 3;
				}
				if(!ptprpc(r, GetPartialObject, 4|DataRecv, 
					nod->handle, offset, count, pos, &p, &np)){
					if(np <= count){
						memmove(r->ofcall.data, p, np);
						r->ofcall.count = np;
						respond(r, nil);
						free(p);
						return;
					}
				}
				free(p);
			}
			/* no break */
		case Qthumb:
			if(nod->data == nil){
				np = 0;
				p = nil;
				if(ptprpc(r, TYPE(path)==Qthumb ? GetThumb : GetObject,
					1|DataRecv, nod->handle, &p, &np) < 0){
					free(p);
					break;
				}
				nod->data = p;
				nod->ndata = np;
			}
			readbuf(r, nod->data, nod->ndata);
			respond(r, nil);
			return;
		}
	}
	responderror(r);
}

static void
fsremove(Req *r)
{
	Node *nod;
	uvlong path;

	path = r->fid->qid.path;
	if(nod = getnode(r, path)){
		switch(TYPE(path)){
		default:
			werrstr(Eperm);
			break;
		case Qobj:
			if(ptprpc(r, DeleteObject, 2, nod->handle, 0) < 0)
				break;
			/* no break */
		case Qthumb:
			if(nod = cachednode(path, nil))
				freenode(nod);
			respond(r, nil);
			return;
		}
	}
	responderror(r);
}

static void
fsopen(Req *r)
{
	if(r->ifcall.mode != OREAD){
		respond(r, Eperm);
		return;
	}
	respond(r, nil);
}

static void
fsflush(Req *r)
{
	Channel *c;

	if(c = r->oldreq->aux)
		nbsendp(c, Einterrupt);
	respond(r, nil);
}

static void
fsdestroyfid(Fid *fid)
{
	Node *nod;
	uvlong path;

	path = fid->qid.path;
	switch(TYPE(path)){
	case Qobj:
	case Qthumb:
		if(nod = cachednode(path, nil)){
			free(nod->data);
			nod->data = nil;
			nod->ndata = 0;
		}
		break;
	}
}

static void
fsend(Srv *)
{
	ptprpc(nil, CloseSession, 0);
	closeioproc(recvp(iochan));
}

static int
findendpoints(Dev *d, int *epin, int *epout, int *epint)
{
	int i;
	Ep *ep;
	Usbdev *ud;

	ud = d->usb;
	*epin = *epout = *epint = -1;
	for(i=0; i<nelem(ud->ep); i++){
		if((ep = ud->ep[i]) == nil)
			continue;
		if(ep->type == Eintr && *epint == -1)
			*epint = ep->id;
		if(ep->type != Ebulk)
			continue;
		if(ep->dir == Eboth || ep->dir == Ein)
			if(*epin == -1)
				*epin =  ep->id;
		if(ep->dir == Eboth || ep->dir == Eout)
			if(*epout == -1)
				*epout = ep->id;
	}
	if(*epin >= 0 && *epout >= 0)
		return 0;
	return -1;
}

Srv fs = 
{
	.attach = fsattach,
	.destroyfid = fsdestroyfid,
	.walk = fswalk,
	.open = fsopen,
	.read = fsread,
	.remove = fsremove,
	.stat = fsstat,
	.flush = fsflush,
	.end = fsend,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-dD] devid\n", argv0);
	threadexits("usage");
}

void
threadmain(int argc, char **argv)
{
	char name[64], desc[64];
	int epin, epout, epint;
	Dev *d;

	ARGBEGIN {
	case 'd':
		debug++;
		break;
	case 'D':
		chatty9p++;
		break;
	default:
		usage();
	} ARGEND;

	if(argc == 0)
		usage();
	if((d = getdev(*argv)) == nil)
		sysfatal("opendev: %r");
	if(findendpoints(d, &epin, &epout, &epint)  < 0)
		sysfatal("findendpoints: %r");

	usbep[In] = openep(usbep[Setup] = d, epin);
	if(epin == epout){
		incref(usbep[In]);
		usbep[Out] = usbep[In];
		opendevdata(usbep[In], ORDWR);
	} else {
		usbep[Out] = openep(d, epout);
		opendevdata(usbep[In], OREAD);
		opendevdata(usbep[Out], OWRITE);
	}
	if(usbep[In]->dfd < 0 || usbep[Out]->dfd < 0)
		sysfatal("open endpoints: %r");
	if(usbep[In]->maxpkt < 12 || usbep[In]->maxpkt > sizeof(Ptprpc))
		sysfatal("bad packet size: %d\n", usbep[In]->maxpkt);
	iochan = chancreate(sizeof(Ioproc*), 1);
	sendp(iochan, ioproc());

	sessionId = getpid();
	if(ptprpc(nil, OpenSession, 1, sessionId) < 0)
		sysfatal("open session: %r");

	time0 = time(0);

	snprint(name, sizeof name, "sdU%s", d->hname);
	snprint(desc, sizeof desc, "%d.ptp", d->id);
	threadpostsharesrv(&fs, nil, name, desc);

	threadexits(0);
}
