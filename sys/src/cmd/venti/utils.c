#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int
nameEq(char *s, char *t)
{
	return strncmp(s, t, ANameSize) == 0;
}

void
nameCp(char *dst, char *src)
{
	strncpy(dst, src, ANameSize - 1);
	dst[ANameSize - 1] = '\0';
}

int
nameOk(char *name)
{
	char *t;
	int c;

	if(name == nil)
		return 0;
	for(t = name; c = *t; t++)
		if(t - name >= ANameSize
		|| c < ' ' || c >= 0x7f)
			return 0;
	return 1;
}

int
strU32Int(char *s, u32int *r)
{
	char *t;
	u32int n, nn, m;
	int c;

	m = TWID32 / 10;
	n = 0;
	for(t = s; ; t++){
		c = *t;
		if(c < '0' || c > '9')
			break;
		if(n > m)
			return 0;
		nn = n * 10 + c - '0';
		if(nn < n)
			return 0;
		n = nn;
	}
	*r = n;
	return s != t && *t == '\0';
}

int
strU64Int(char *s, u64int *r)
{
	char *t;
	u64int n, nn, m;
	int c;

	m = TWID64 / 10;
	n = 0;
	for(t = s; ; t++){
		c = *t;
		if(c < '0' || c > '9')
			break;
		if(n > m)
			return 0;
		nn = n * 10 + c - '0';
		if(nn < n)
			return 0;
		n = nn;
	}
	*r = n;
	return s != t && *t == '\0';
}

int
vtTypeValid(int type)
{
	return type > VtErrType && type < VtMaxType;
}

void
fmtZBInit(Fmt *f, ZBlock *b)
{
	f->runes = 0;
	f->start = b->data;
	f->to = f->start;
	f->stop = (char*)f->start + b->len;
	f->flush = nil;
	f->farg = nil;
	f->nfmt = 0;
	f->args = nil;
}

static int
sFlush(Fmt *f)
{
	char *s;
	int n;

	n = (int)f->farg;
	n += 256;
	f->farg = (void*)n;
	s = f->start;
	f->start = realloc(s, n);
	if(f->start == nil){
		f->start = s;
		return 0;
	}
	f->to = (char*)f->start + ((char*)f->to - s);
	f->stop = (char*)f->start + n - 1;
	return 1;
}

static char*
logit(int severity, char *fmt, va_list args)
{
	Fmt f;
	int n;

	f.runes = 0;
	n = 32;
	f.start = malloc(n);
	if(f.start == nil)
		return nil;
	f.to = f.start;
	f.stop = (char*)f.start + n - 1;
	f.flush = sFlush;
	f.farg = (void*)n;
	f.nfmt = 0;
	f.args = args;
	n = dofmt(&f, fmt);
	if(n < 0)
		return nil;
	*(char*)f.to = '\0';

	if(argv0 == nil)
		fprint(2, "%s: err %d: %s\n", argv0, severity, f.start);
	else
		fprint(2, "err %d: %s\n", severity, f.start);
	return f.start;
}

void
setErr(int severity, char *fmt, ...)
{
	char *s;
	va_list args;

	va_start(args, fmt);
	s = logit(severity, fmt, args);
	va_end(args);
	if(s == nil)
		vtSetError("error setting error");
	else{
		vtSetError(s);
		free(s);
	}
}

void
logErr(int severity, char *fmt, ...)
{
	char *s;
	va_list args;

	va_start(args, fmt);
	s = logit(severity, fmt, args);
	va_end(args);
	free(s);
}

u32int
now(void)
{
	return time(nil);
}

void
fatal(char *fmt, ...)
{
	Fmt f;
	char buf[256];

	f.runes = 0;
	f.start = buf;
	f.to = buf;
	f.stop = buf + sizeof(buf);
	f.flush = fmtfdflush;
	f.farg = (void*)2;
	f.nfmt = 0;
	fmtprint(&f, "fatal %s error:", argv0);
	va_start(f.args, fmt);
	dofmt(&f, fmt);
	va_end(f.args);
	fmtprint(&f, "\n");
	fmtfdflush(&f);
	if(0)
		abort();
	exits(buf);
}

ZBlock *
allocZBlock(u32int size, int zeroed)
{
	ZBlock *b;
	static ZBlock z;

	b = malloc(sizeof(ZBlock) + size);
	if(b == nil){
		setErr(EOk, "out of memory");
		return nil;
	}

	*b = z;
	b->data = (u8int*)&b[1];
	b->len = size;
	if(zeroed)
		memset(b->data, 0, size);
	return b;
}

void
freeZBlock(ZBlock *b)
{
	free(b);
}

ZBlock*
packet2ZBlock(Packet *p, u32int size)
{
	ZBlock *b;

	if(p == nil)
		return nil;
	b = allocZBlock(size, 0);
	if(b == nil)
		return nil;
	b->len = size;
	if(!packetCopy(p, b->data, 0, size)){
		freeZBlock(b);
		return nil;
	}
	return b;
}

Packet*
zblock2Packet(ZBlock *zb, u32int size)
{
	Packet *p;

	if(zb == nil)
		return nil;
	p = packetAlloc();
	packetAppend(p, zb->data, size);
	return p;
}

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		fatal("out of memory");
	memset(p, 0xa5, n);
	return p;
}

void *
ezmalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		fatal("out of memory");
	memset(p, 0, n);
	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(p == nil)
		fatal("out of memory");
	return p;
}

char *
estrdup(char *s)
{
	char *t;
	int n;

	n = strlen(s) + 1;
	t = emalloc(n);
	memmove(t, s, n);
	return t;
}

ZBlock*
readFile(char *name)
{
	Part *p;
	ZBlock *b;

	p = initPart(name, 1);
	if(p == nil)
		return nil;
	b = allocZBlock(p->size, 0);
	if(b == nil){
		setErr(EOk, "can't alloc %s: %R", name);
		freePart(p);
		return nil;
	}
	if(!readPart(p, 0, b->data, p->size)){
		setErr(EOk, "can't read %s: %R", name);
		freePart(p);
		freeZBlock(b);
		return nil;
	}
	freePart(p);
	return b;
}

/*
 * return floor(log2(v))
 */
int
u64log2(u64int v)
{
	int i;

	for(i = 0; i < 64; i++)
		if((v >> i) <= 1)
			break;
	return i;
}
