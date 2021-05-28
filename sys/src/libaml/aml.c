#include <u.h>
#include <libc.h>
#include <aml.h>

typedef struct Interp Interp;
typedef struct Frame Frame;
typedef struct Heap Heap;

typedef struct Method Method;
typedef struct Region Region;
typedef struct Field Field;

typedef struct Name Name;
typedef struct Ref Ref;
typedef struct Env Env;
typedef struct Op Op;

struct Heap {
	Heap	*link;
	short	size;
	uchar	mark;
	char	tag;
};

#define H2D(h)	(((Heap*)(h))+1)
#define D2H(d)	(((Heap*)(d))-1)
#define TAG(d)	D2H(d)->tag
#define SIZE(d)	D2H(d)->size

static char *spacename[] = {
	"Mem", 
	"Io",
	"Pcicfg",
	"Ebctl",
	"Smbus",
	"Cmos",
	"Pcibar",
	"Ipmi",
};

/* field flags */
enum {
	AnyAcc		= 0x00,
	ByteAcc		= 0x01,
	WordAcc		= 0x02,
	DWordAcc	= 0x03,
	QWordAcc	= 0x04,
	BufferAcc	= 0x05,
	AccMask		= 0x07,

	NoLock		= 0x10,

	Preserve	= 0x00,
	WriteAsOnes	= 0x20,
	WriteAsZeros	= 0x40,
	UpdateMask	= 0x60,
};

struct Method {
	Name	*name;
	int	narg;
	void*	(*eval)(void);
	uchar	*start;
	uchar	*end;
};

struct Region {
	Amlio;
	char	mapped;
};

struct Field {
	void	*reg;	/* Buffer or Region */
	Field	*index;
	void	*indexv;
	int	flags;
	int	bitoff;
	int	bitlen;
};

struct Name {
	void	*v;

	Name	*up;
	Name	*next;
	Name	*fork;
	Name	*down;

	char	seg[4];
};

struct Ref {
	void	*ref;
	void	**ptr;
};

struct Env {
	void	*loc[8];
	void	*arg[8];
};

struct Op {
	char	*name;
	char	*sequence;
	void*	(*eval)(void);
};

struct Frame {
	int	tag;
	int	cond;
	char	*phase;
	uchar	*start;
	uchar	*end;
	Op	*op;
	Env	*env;
	Name	*dot;
	void	*ref;
	void	*aux;
	int	narg;
	void	*arg[8];
};

struct Interp {
	uchar	*pc;
	Frame	*fp;
};

static Interp	interp;
static Frame	stack[32];

#define PC	interp.pc
#define	FP	interp.fp
#define FB	stack
#define FT	&stack[nelem(stack)]

static Heap *hp;

enum {
	Obad, Onop, Odebug,
	Ostr, Obyte, Oword, Odword, Oqword, Oconst,
	Onamec, Oname, Oscope, Oalias,
	Oreg, Ofld, Oxfld, Opkg, Ovpkg, Oenv, Obuf, Omet, 
	Odev, Ocpu, Othz, Oprc,
	Oadd, Osub, Omod, Omul, Odiv, Oshl, Oshr, Oand, Onand, Oor,
	Onor, Oxor, Onot, Olbit, Orbit, Oinc, Odec,
	Oland, Olor, Olnot, Oleq, Olgt, Ollt,
	Oindex, Omutex, Oevent,
	Ocfld, Ocfld0, Ocfld1, Ocfld2, Ocfld4, Ocfld8,
	Oif, Oelse, Owhile, Obreak, Oret, Ocall, 
	Ostore, Oderef, Osize, Oref, Ocref, Ocat,
	Oacq, Orel, Ostall, Osleep, Oload, Ounload,
};

static Op optab[];
static uchar octab1[];
static uchar octab2[];

static Name*
rootname(Name *dot)
{
	while(dot != dot->up)
		dot = dot->up;
	return dot;
}

static void
gcmark(void *p)
{
	int i;
	Env *e;
	Field *f;
	Heap *h;
	Name *n, *d;

	if(p == nil)
		return;
	h = D2H(p);
	if(h->mark & 1)
		return;
	h->mark |= 1;
	switch(h->tag){
	case 'E':
		e = p;
		for(i=0; i<nelem(e->loc); i++)
			gcmark(e->loc[i]);
		for(i=0; i<nelem(e->arg); i++)
			gcmark(e->arg[i]);
		break;
	case 'R':
	case 'A':
	case 'L':
		gcmark(((Ref*)p)->ref);
		break;
	case 'N':
		n = p;
		gcmark(n->v);
		for(d = n->down; d; d = d->next)
			gcmark(d);
		gcmark(n->fork);
		gcmark(n->up);
		break;
	case 'p':
		for(i=0; i<(SIZE(p)/sizeof(void*)); i++)
			gcmark(((void**)p)[i]);
		break;
	case 'r':
		gcmark(((Region*)p)->name);
		break;
	case 'm':
		gcmark(((Method*)p)->name);
		break;
	case 'f':
	case 'u':
		f = p;
		gcmark(f->reg);
		gcmark(f->index);
		gcmark(f->indexv);
		break;
	}
}

static int
gc(void)
{
	int i;
	Heap *h, **hh;
	Frame *f;

	for(h = hp; h; h = h->link)
		h->mark &= ~1;

	for(h = hp; h; h = h->link)
		if(h->mark & 2)
			gcmark(H2D(h));

	for(f = FP; f >= FB; f--){
		for(i=0; i<f->narg; i++)
			gcmark(f->arg[i]);
		gcmark(f->env);
		gcmark(f->dot);
		gcmark(f->ref);
	}

	gcmark(amlroot);

	i = 0;
	hh = &hp;
	while(h = *hh){
		if(h->mark){
			hh = &h->link;
			continue;
		}
		*hh = h->link;
		if(h->tag == 'r'){
			Region *r = (void*)H2D(h);
			if(r->mapped > 0){
				if(amldebug)
					print("\namlunmapio(%N): %-8s %llux - %llux\n", 
						(Name*)r->name, spacename[r->space],
						r->off, r->off + r->len);
				amlunmapio(r);
			}
			r->mapped = 0;
			r->write = nil;
			r->read = nil;
			r->aux = nil;
			r->va = nil;
		}
		memset(h, ~0, sizeof(Heap)+h->size);
		amlfree(h);
		i++;
	}

	return i;
}

static void*
mk(int tag, int size)
{
	Heap *h;
	int a;

	a = sizeof(Heap) + size;
	h = amlalloc(a);
	h->size = size;
	h->tag = tag;
	h->link = hp;
	hp = h;
	return h+1;
}

static uvlong*
mki(uvlong i)
{
	uvlong *v;

	v = mk('i', sizeof(uvlong));
	*v = i;
	return v;
}

static char*
mks(char *s)
{
	char *r = mk('s', strlen(s)+1);
	strcpy(r, s);
	return r;
}

static int
pkglen(uchar *p, uchar *e, uchar **np)
{
	ulong n;
	uchar b;

	if(p >= e)
		return -1;
	b = *p++;
	if(b <= 0x3F)
		n = b;
	else {
		n = b & 0xF;
		if(p >= e)
			return -1;
		n += *p++ << 4;
		if(b >= 0x80){
			if(p >= e)
				return -1;
			n += *p++ << 12;
		}
		if(b >= 0xC0){
			if(p >= e)
				return -1;
			n += *p++ << 20;
		}
	}
	if(np)
		*np = p;
	return n;
}

static Name*
forkname(Name *dot)
{
	Name *n;

	n = mk('N', sizeof(Name));
	*n = *dot;
	n->fork = dot;
	n->next = n->down = nil;
	if(dot->v == dot)
		n->v = n;
	if(dot->up == dot)
		n->up = n;
	else {
		if(n->up = forkname(dot->up))
			n->up->down = n;
	}
	return n;
}

static Name*
getseg(Name *dot, void *seg, int new)
{
	Name *n, *l;

	for(n = l = nil; dot; dot = dot->fork){
		for(n = dot->down; n; n = n->next){
			if(memcmp(seg, n->seg, 4) == 0)
				return n;
			l = n;
		}
		if(new){
			n = mk('N', sizeof(Name));
			memmove(n->seg, seg, sizeof(n->seg));
			n->up = dot;
			if(l == nil)
				dot->down = n;
			else
				l->next = n;
			n->v = n;
			break;
		}
	}
	return n;
}

Name*
getname(Name *dot, char *path, int new)
{
	char seg[4];
	int i, s;
	Name *x;

	s = !new;
	if(*path == '\\'){
		path++;
		dot = rootname(dot);
		s = 0;
	}
	while(*path == '^'){
		path++;
		dot = dot->up;
		s = 0;
	}
	do {
		for(i=0; i<4; i++){
			if(*path == 0 || *path == '.')
				break;
			seg[i] = *path++;
		}
		if(i == 0)
			break;
		while(i < 4)
			seg[i++] = '_';
		if(s && *path == 0){
			for(;;){
				if(x = getseg(dot, seg, 0))
					break;
				if(dot == dot->up)
					break;
				dot = dot->up;
			}
			return x;
		}
		s = 0;
		dot = getseg(dot, seg, new);
	} while(*path++ == '.');

	return dot;
}

static int
fixnames(void *dot, void *arg)
{
	void **r, *v;
	int i;

	if(arg == nil)
		r = &((Name*)dot)->v;
	else
		r = arg;
	v = *r;
	if(v == nil || v == dot)
		return 0;
	if(TAG(v) == 'p'){
		r = (void**)v;
		for(i=0; i<(SIZE(r)/sizeof(void*)); i++)
			fixnames(dot, r+i);
		return 0;
	}
	if(TAG(v) == 'n' && (v = getname(dot, v, 0)) != nil)
		*r = v;
	return 0;
}

static uvlong
getle(uchar *p, int len)
{
	uvlong v;
	int i;

	v = 0ULL;
	for(i=0; i<len; i++)
		v |= ((uvlong)p[i]) << i*8;
	return v;
}

static void
putle(uchar *p, int len, uvlong v)
{
	int i;

	for(i=0; i<len; i++){
		p[i] = v;
		v >>= 8;
	}
}

static uvlong
rwreg(void *reg, int off, int len, uvlong v, int write)
{
	uchar buf[8], *p;
	Region *r;

	switch(TAG(reg)){
	case 'b':
		p = reg;
		if((off+len) > SIZE(p))
			break;
		p += off;
		if(write)
			putle(p, len, v);
		else
			v = getle(p, len);
		return v;

	case 'r':
		r = reg;
		if((off+len) > r->len)
			break;
		if(r->mapped == 0){
			if(amldebug)
				print("\namlmapio(%N): %-8s %llux - %llux\n", 
					(Name*)r->name, spacename[r->space],
					r->off, r->off + r->len);
			r->mapped = 1;
			if(amlmapio(r) < 0)
				r->mapped = -1;
		}
		if(r->mapped <= 0)
			break;

		if(r->va != nil)
			p = r->va + off;
		else {
			if(len > sizeof(buf))
				break;
			p = buf;
		}

		if(write){
			if(amldebug)
				print("\nrwreg(%N): %-8s [%llux+%x]/%d <- %llux\n", 
					(Name*)r->name, spacename[r->space],
					r->off, off, len, v);
			putle(p, len, v);
			if(r->write != nil){
				if((*r->write)(r, p, len, off) != len)
					break;
			} else if(p == buf)
				break;
		} else {
			if(r->read != nil){
				if((*r->read)(r, p, len, off) != len)
					break;
			} else if(p == buf)
				break;
			v = getle(p, len);
			if(amldebug)
				print("\nrwreg(%N): %-8s [%llux+%x]/%d -> %llux\n", 
					(Name*)r->name, spacename[r->space],
					r->off, off, len, v);
		}
		return v;
	}

	return ~0;
}

static uvlong
ival(void *p)
{
	int n;

	if(p != nil){
		switch(TAG(p)){
		case 'i':
			return *((uvlong*)p);
		case 's':
			if(*((char*)p) == 0)
				break;
			return strtoull((char*)p, 0, 16);
		case 'b':
			n = SIZE(p);
			if(n > 0){
				if(n > 8) n = 8;
				return rwreg(p, 0, n, 0, 0);
			}
		}
	}
	return 0;
}

static void *deref(void *p);
static void *store(void *s, void *d);

static int
fieldalign(int flags)
{
	switch(flags & AccMask){
	default:
	case AnyAcc:
	case ByteAcc:
	case BufferAcc:
		return 1;
	case WordAcc:
		return 2;
	case DWordAcc:
		return 4;
	case QWordAcc:
		return 8;
	}
}

static void*
rwfield(Field *f, void *v, int write)
{
	int boff, blen, wo, ws, wl, wa, wd, i;
	uvlong w, m;
	void *reg;
	uchar *b;

	if(f == nil || (reg = deref(f->reg)) == nil)
		return nil;
	if(f->index)
		store(f->indexv, f->index);
	blen = f->bitlen;
	if(write){
		if(v && TAG(v) == 'b'){
			b = v;
			if(SIZE(b)*8 < blen)
				blen = SIZE(b)*8;
		} else {
			w = ival(v);
			b = mk('b', (blen+7)/8);
			putle(b, SIZE(b), w);
		}
	} else
		b = mk('b', (blen+7)/8);
	wa = fieldalign(f->flags);
	wd = wa*8;
	boff = 0;
	while((wl = (blen-boff)) > 0){
		wo = (f->bitoff+boff) / wd;
		ws = (f->bitoff+boff) % wd;
		if(wl > (wd - ws))
			wl = wd - ws;
		if(write){
			w = 0;
			for(i = 0; i < wl; i++, boff++)
				if(b[boff/8] & (1<<(boff%8)))
					w |= 1ULL<<i;
			w <<= ws;
			if(wl != wd){
				m = ((1ULL<<wl)-1) << ws;
				w |= rwreg(reg, wo*wa, wa, 0, 0) & ~m;
			}
			rwreg(reg, wo*wa, wa, w, 1);
		} else {
			w = rwreg(reg, wo*wa, wa, 0, 0) >> ws;
			for(i = 0; i < wl; i++, boff++){
				b[boff/8] |= (w&1)<<(boff%8);
				w >>= 1;
			}
		}
	}
	if(write)
		return nil;
	if(blen > 64)
		return b;
	w = getle(b, SIZE(b));
	return mki(w);
}

static void*
deref(void *p)
{
	if(p) switch(TAG(p)){
	case 'N':
		return ((Name*)p)->v;
	case 'R': case 'A': case 'L':
		return *((Ref*)p)->ptr;
	case 'f': case 'u':
		return rwfield(p, nil, 0);
	}
	return p;
}

static void*
copy(int tag, void *s)
{
	static char hex[] = "0123456789ABCDEF";
	uvlong v;
	void *d;
	int i, n;

	if(tag == 0){
		if(s == nil)
			return nil;
		tag = TAG(s);
	}
	if(s == nil || TAG(s) == 'i'){
		n = 4;
		v = ival(s);
		if(v > 0xFFFFFFFFULL)
			n <<= 1;
		switch(tag){
		case 'b':
			d = mk(tag, n);
			rwreg(d, 0, n, v, 1);
			return d;
		case 's':
			n <<= 1;
			d = mk(tag, n+1);
			((char*)d)[n] = 0;
			while(n > 0){
				((char*)d)[--n] = hex[v & 0xF];
				v >>= 4;
			}
			return d;
		case 'i':
			if(v == 0ULL)
				return nil;
			return mki(v);
		}
	} else {
		n = SIZE(s);
		switch(tag){
		case 's':
			if(TAG(s) == 'b'){
				d = mk(tag, n*3 + 1);
				for(i=0; i<n; i++){
					((char*)d)[i*3 + 0] = hex[((uchar*)s)[i] >> 4];
					((char*)d)[i*3 + 1] = hex[((uchar*)s)[i] & 0xF];
					((char*)d)[i*3 + 2] = ' ';
				}
				((char*)d)[n*3] = 0;
				return d;
			}
			/* no break */
		case 'b':
			if(TAG(s) == 's'){
				n = strlen(s);
				/* zero length string is converted to zero length buffer */
				if(n > 0) n++;
			}
			d = mk(tag, n);
			memmove(d, s, n);
			return d;
		}
	}
	return s;
}

static void*
store(void *s, void *d)
{
	void *p, **pp;

	if(d == nil)
		return nil;
	switch(TAG(d)){
	default:
		return nil;
	case 'A':
		s = deref(s);
		/* no break */
	case 'R': case 'L':
		pp = ((Ref*)d)->ptr;
		while((p = *pp) != nil){
			switch(TAG(p)){
			case 'R': case 'A': case 'L':
				pp = ((Ref*)p)->ptr;
				break;
			case 'N':
				pp = &((Name*)p)->v;
				break;
			}
			if(*pp == p)
				break;
		}
		break;
	case 'N':
		pp = &((Name*)d)->v;
		break;
	}
	p = *pp;
	if(p != nil && TAG(p) != 'N'){
		switch(TAG(p)){
		case 'f':
		case 'u':
			rwfield(p, s, 1);
			return d;
		}
		if(TAG(d) != 'A' && TAG(d) != 'L'){
			*pp = copy(TAG(p), s);
			return d;
		}
	}
	*pp = copy(0, s);
	return d;
}

static int
Nfmt(Fmt *f)
{
	char buf[5];
	int i;
	Name *n;

	n = va_arg(f->args, Name*);
	if(n == nil)
		return fmtprint(f, "?NIL");
	if(n == n->up)
		return fmtprint(f, "\\");
	strncpy(buf, n->seg, 4);
	buf[4] = 0;
	for(i=3; i>0; i--){
		if(buf[i] != '_')
			break;
		buf[i] = 0;
	}
	if(n->up == n->up->up)
		return fmtprint(f, "\\%s", buf);
	return fmtprint(f, "%N.%s", n->up, buf);
}

static int
Vfmt(Fmt *f)
{
	void *p;
	int i, n, c;
	Env *e;
	Field *l;
	Name *nm;
	Method *m;
	Region *g;
	Ref *r;

	p = va_arg(f->args, void*);
	if(p == nil)
		return fmtprint(f, "nil");
	c = TAG(p);
	switch(c){
	case 'N':
		nm = p;
		if(nm->v != nm)
			return fmtprint(f, "%N=%V", nm, nm->v);
		return fmtprint(f, "%N=*", nm);
	case 'A':
	case 'L':
		r = p;
		e = r->ref;
		if(c == 'A')
			return fmtprint(f, "Arg%ld=%V", r->ptr - e->arg, *r->ptr);
		if(c == 'L')
			return fmtprint(f, "Local%ld=%V", r->ptr - e->loc, *r->ptr);
	case 'n':
		return fmtprint(f, "%s", (char*)p);
	case 's':
		return fmtprint(f, "\"%s\"", (char*)p);
	case 'i':
		return fmtprint(f, "%#llux", *((uvlong*)p));
	case 'p':
		n = SIZE(p)/sizeof(void*);
		fmtprint(f, "Package(%d){", n);
		for(i=0; i<n; i++){
			if(i > 0)
				fmtprint(f, ", ");
			fmtprint(f, "%V", ((void**)p)[i]);
		}
		fmtprint(f, "}");
		return 0;
	case 'b':
		n = SIZE(p);
		fmtprint(f, "Buffer(%d){", n);
		for(i=0; i<n; i++){
			if(i > 0)
				fmtprint(f, ", ");
			fmtprint(f, "%.2uX", ((uchar*)p)[i]);
		}
		fmtprint(f, "}");
		return 0;
	case 'r':
		g = p;
		return fmtprint(f, "Region(%s, %#llux, %#llux)",
			spacename[g->space & 7], g->off, g->len);
	case 'm':
		m = p;
		fmtprint(f, "%N(", m->name);
		for(i=0; i < m->narg; i++){
			if(i > 0)
				fmtprint(f, ", ");
			fmtprint(f, "Arg%d", i);
		}
		fmtprint(f, ")");
		return 0;
	case 'u':
		fmtprint(f, "Buffer");
		/* no break */
	case 'f':
		l = p;
		if(l->index)
			return fmtprint(f, "IndexField(%x, %x, %x) @ %V[%V]",
				l->flags, l->bitoff, l->bitlen, l->index, l->indexv);
		return fmtprint(f, "Field(%x, %x, %x) @ %V",
			l->flags, l->bitoff, l->bitlen, l->reg);
	default:
		return fmtprint(f, "%c:%p", c, p);
	}
}

static void
dumpregs(void)
{
	Frame *f;
	Env *e;
	int i;

	print("\n*** dumpregs: PC=%p FP=%p\n", PC, FP);
	e = nil;
	for(f = FP; f >= FB; f--){
		print("%.8p.%.2lx: %-8s %N\t", f->start, f-FB, f->phase, f->dot);
		if(f->op)
			print("%s", f->op->name);
		print("(");
		for(i=0; i<f->narg; i++){
			if(i > 0)
				print(", ");
			print("%V", f->arg[i]);
		}
		print(")\n");
		if(e == f->env)
			continue;
		if(e = f->env){
			for(i=0; i<nelem(e->arg); i++)
				print("Arg%d=%V ", i, e->arg[i]);
			print("\n");
			for(i=0; i<nelem(e->loc); i++)
				print("Local%d=%V ", i, e->loc[i]);
			print("\n");
		}
	}
}

static int
xec(uchar *pc, uchar *end, Name *dot, Env *env, void **pret)
{
	static int loop;
	int i, c;
	void *r;

	PC = pc;
	FP = FB;

	FP->tag = 0;
	FP->cond = 0;
	FP->narg = 0;
	FP->phase = "}";
	FP->start = PC;
	FP->end = end;
	FP->aux = end;
	FB->ref = nil;
	FP->dot = dot;
	FP->env = env;
	FP->op = nil;

	for(;;){
		if((++loop & 127) == 0)
			gc();
		if(amldebug)
			print("\n%.8p.%.2lx %-8s\t%N\t", PC, FP - FB, FP->phase, FP->dot);
		r = nil;
		c = *FP->phase++;
		switch(c){
		default:
			if(PC >= FP->end){
			Overrun:
				print("aml: PC overrun frame end");
				goto Out;
			}
			FP++;
			if(FP >= FT){
				print("aml: frame stack overflow");
				goto Out;
			}
			*FP = FP[-1];
			FP->aux = nil;
			FP->ref = nil;
			FP->tag = c;
			FP->start = PC;
			c = *PC++;
			if(amldebug) print("%.2X", c);
			if(c == '['){
				if(PC >= FP->end)
					goto Overrun;
				c = *PC++;
				if(amldebug) print("%.2X", c);
				c = octab2[c];
			}else
				c = octab1[c];
			FP->op = &optab[c];
			FP->narg = 0;
			FP->phase = FP->op->sequence;
			if(amldebug) print("\t%s %s", FP->op->name, FP->phase);
			continue;
		case '{':
			end = PC;
			c = pkglen(PC, FP->end, &PC);
			end += c;
			if(c < 0 || end > FP->end)
				goto Overrun;
			FP->end = end;
			continue;
		case ',':
			FP->start = PC;
			continue;
		case 's':
			if(end = memchr(PC, 0, FP->end - PC))
				end++;
			else
				end = FP->end;
			c = end - PC;
			r = mk('s', c+1);
			memmove(r, PC, c);
			((uchar*)r)[c] = 0;
			PC = end;
			break;
		case '1':
		case '2':
		case '4':
		case '8':
			end = PC+(c-'0');
			if(end > FP->end)
				goto Overrun;
			else {
				r = mki(*PC++);
				for(i = 8; PC < end; i += 8)
					*((uvlong*)r) |= ((uvlong)*PC++) << i;
			}
			break;
		case '}':
		case 0:
			if(FP->op){
				if(amldebug){
					print("*%p,%V\t%s(", FP->aux, FP->ref, FP->op->name);
					for(i = 0; i < FP->narg; i++){
						if(i > 0)
							print(", ");
						print("%V", FP->arg[i]);
					}
					print(")");
				}
				for(i = FP->narg; i < nelem(FP->arg); i++)
					FP->arg[i] = nil;
				r = FP->op->eval();
				if(amldebug)
					print(" -> %V", r);
			}

			c = FP->phase[-1];
			if(c == '}' && PC < FP->end){
				FP->narg = 0;
				FP->phase = "*}";
				continue;
			}

			if(r) switch(FP->tag){
			case '@':
				break;
			case 'n':
			case 'N':
				if(TAG(r) != 'N')
					r = nil;
				break;
			default:
				if((r = deref(r)) == nil)
					break;
				switch(TAG(r)){
				case 'f': case 'u':
					r = rwfield(r, nil, 0);
					break;
				case 'm': {
					Method *m = r;
					FP->ref = m;
					FP->narg = 0;
					FP->phase = "********}" + (8 - m->narg);
					FP->op = &optab[Ocall];
					continue;
					}
				}
			}
			FP--;
			break;
		}
		if(FP < FB){
			if(pret){
				if(amldebug) print(" -> %V\n", r);
				*pret = r;
			}
			return 0;
		}
		FP->arg[FP->narg++] = r;
	}
Out:
	if(amldebug)
		dumpregs();
	return -1;
}

static void*
evalnamec(void)
{
	static char name[1024];
	char *d;
	void *r;
	int c;

	c = 1;
	d = name;
	PC = FP->start;
	if(*PC == '\\')
		*d++ = *PC++;
	while(*PC == '^'){
		if(d >= &name[sizeof(name)-1]){
		Toolong:
			*d = 0;
			print("aml: name too long: %s\n", name);
			PC = FP->end;
			return nil;
		}
		*d++ = *PC++;
	}
	if(*PC == '.'){
		PC++;
		c = 2;
	} else if(*PC == '/'){
		PC++;
		c = *PC++;
	} else if(*PC == 0){
		PC++;
		c = 0;
	}
	while(c > 0){
		if(d >= &name[sizeof(name)-5])
			goto Toolong;
		*d++ = *PC++;
		*d++ = *PC++;
		*d++ = *PC++;
		*d++ = *PC++;
		while((d > &name[0]) && (d[-1] == '_' || d[-1] == 0))
			d--;
		if(--c > 0)
			*d++ = '.';
	}
	*d = 0;
	if((r = getname(FP->dot, name, FP->tag == 'N')) == nil){
		r = mks(name);
		D2H(r)->tag = 'n';	/* unresolved name */
	}
	return r;
}

static void*
evaliarg0(void)
{
	return FP->arg[0];
}

static void*
evalconst(void)
{
	switch(FP->start[0]){
	case 0x01:
		return mki(1);
	case 0xFF:
		return mki(-1);
	}
	return nil;
}

static void*
evalbuf(void)
{
	int n, m;
	uchar *p;

	n = ival(FP->arg[0]);
	p = mk('b', n);
	m = FP->end - PC;
	if(m > n)
		m = n;
	memmove(p, PC, m);
	PC = FP->end;
	return p;
}

static void*
evalpkg(void)
{
	void **p, **x;
	int n;

	if((p = FP->ref) != nil){
		x = FP->aux;
		if(x >= p && x < (p+(SIZE(p)/sizeof(void*)))){
			*x++ = FP->arg[0];
			FP->aux = x;
		}
	}else {
		n = ival(FP->arg[0]);
		if(n < 0) n = 0;
		p = mk('p', n*sizeof(void*));
		FP->aux = p;
		FP->ref = p;
	}
	return p;
}

static void*
evalname(void)
{
	Name *n;

	if(n = FP->arg[0])
		n->v = FP->arg[1];
	else
		PC = FP->end;
	return nil;
}

static void*
evalscope(void)
{
	Name *n;

	if(n = FP->arg[0])
		FP->dot = n;
	else
		PC = FP->end;
	FP->op = nil;
	return nil;
}

static void*
evalalias(void)
{
	Name *n;

	if(n = FP->arg[1])
		n->v = deref(FP->arg[0]);
	return nil;
}

static void*
evalmet(void)
{
	Name *n;
	Method *m;

	if((n = FP->arg[0]) != nil){
		m = mk('m', sizeof(Method));
		m->narg = ival(FP->arg[1]) & 7;
		m->start = PC;
		m->end = FP->end;
		m->name = n;
		n->v = m;
	}
	PC = FP->end;
	return nil;
}

static void*
evalreg(void)
{
	Name *n;
	Region *r;

	if((n = FP->arg[0]) != nil){
		r = mk('r', sizeof(Region));
		r->space = ival(FP->arg[1]);
		r->off = ival(FP->arg[2]);
		r->len = ival(FP->arg[3]);
		r->name = n;
		r->va = nil;
		n->v = r;
	}
	return nil;
}

static void*
evalcfield(void)
{
	void *r;
	Field *f;
	Name *n;
	int c;

	r = FP->arg[0];
	if(r == nil || (TAG(r) != 'b' && TAG(r) != 'r'))
		return nil;
	c = FP->op - optab;
	if(c == Ocfld)
		n = FP->arg[3];
	else
		n = FP->arg[2];
	if(n == nil || TAG(n) != 'N')
		return nil;
	if(TAG(r) == 'b')
		f = mk('u', sizeof(Field));
	else
		f = mk('f', sizeof(Field));
	switch(c){
	case Ocfld:
		f->bitoff = ival(FP->arg[1]);
		f->bitlen = ival(FP->arg[2]);
		break;
	case Ocfld0:
		f->bitoff = ival(FP->arg[1]);
		f->bitlen = 1;
		break;
	case Ocfld1:
	case Ocfld2:
	case Ocfld4:
	case Ocfld8:
		f->bitoff = 8*ival(FP->arg[1]);
		f->bitlen = 8*(c - Ocfld0);
		break;
	}
	f->reg = r;
	n->v = f;
	return nil;
}

static void*
evalfield(void)
{
	int flags, bitoff, wa, n;
	Field *f, *df;
	Name *d;
	uchar *p;

	df = nil;
	flags = 0;
	bitoff = 0;
	switch(FP->op - optab){
	case Ofld:
		flags = ival(FP->arg[1]);
		break;
	case Oxfld:
		df = deref(FP->arg[1]);
		if(df == nil || TAG(df) != 'f')
			goto Out;
		flags = ival(FP->arg[2]);
		break;
	}
	p = PC;
	if(p >= FP->end)
		return nil;
	while(p < FP->end){
		if(*p == 0x00){
			p++;
			if((n = pkglen(p, FP->end, &p)) < 0)
				break;
			bitoff += n;
			continue;
		}
		if(*p == 0x01){
			p++;
			flags = *p;
			p += 2;
			continue;
		}
		if(p+4 >= FP->end)
			break;
		if((d = getseg(FP->dot, p, 1)) == nil)
			break;
		if((n = pkglen(p+4, FP->end, &p)) < 0)
			break;
		f = mk('f', sizeof(Field));
		f->flags = flags;
		f->bitlen = n;
		switch(FP->op - optab){
		case Ofld:
			f->reg = FP->arg[0];
			f->bitoff = bitoff;
			break;
		case Oxfld:
			wa = fieldalign(df->flags);
			f->reg = df->reg;
			f->bitoff = df->bitoff + (bitoff % (wa*8));
			f->indexv = mki((bitoff/(wa*8))*wa);
			f->index = FP->arg[0];
			break;
		}
		bitoff += n;
		d->v = f;
	}
Out:
	PC = FP->end;
	return nil;
}

static void*
evalnop(void)
{
	return nil;
}

static void*
evalbad(void)
{
	int i;

	print("aml: bad opcode %p: ", PC);
	for(i=0; i < 8 && (FP->start+i) < FP->end; i++){
		if(i > 0)
			print(" ");
		print("%.2X", FP->start[i]);
	}
	if((FP->start+i) < FP->end)
		print("...");
	print("\n");
	PC = FP->end;
	return nil;
}

static void*
evalcond(void)
{
	switch(FP->op - optab){
	case Oif:
		if(FP <= FB)
			break;
		FP[-1].cond = ival(FP->arg[0]) != 0;
		if(!FP[-1].cond)
			PC = FP->end;
		break;
	case Oelse:
		if(FP <= FB)
			break;
		if(FP[-1].cond)
			PC = FP->end;
		break;
	case Owhile:
		if(FP->aux){
			if(PC >= FP->end){
				PC = FP->start;
				FP->aux = nil;
			}
			return nil;
		}
		FP->aux = FP->end;
		if(ival(FP->arg[0]) == 0){
			PC = FP->end;
			break;
		}
		return nil;
	}
	FP->op = nil;
	return nil;
}

static void*
evalcmp(void)
{
	void *a, *b;
	int tag, c;

	a = FP->arg[0];
	b = FP->arg[1];
	if(a == nil || TAG(a) == 'i'){
		c = ival(a) - ival(b);
	} else {
		tag = TAG(a);
		if(b == nil || TAG(b) != tag)
			b = copy(tag, b);
		if(TAG(b) != tag)
			return nil;	/* botch */
		switch(tag){
		default:
			return nil;	/* botch */
		case 's':
			c = strcmp((char*)a, (char*)b);
			break;
		case 'b':
			if((c = SIZE(a) - SIZE(b)) == 0)
				c = memcmp(a, b, SIZE(a));
			break;
		}
	}

	switch(FP->op - optab){
	case Oleq:
		if(c == 0) return mki(1);
		break;
	case Olgt:
		if(c > 0) return mki(1);
		break;
	case Ollt:
		if(c < 0) return mki(1);
		break;
	}
	return nil;
}

static void*
evalcall(void)
{
	Method *m;
	Env *e;
	int i;

	if(FP->aux){
		if(PC >= FP->end){
			PC = FP->aux;
			FP->end = PC;
		}
		return nil;
	}
	m = FP->ref;
	e = mk('E', sizeof(Env));
	for(i=0; i<FP->narg; i++)
		e->arg[i] = deref(FP->arg[i]);
	FP->env = e;
	FP->narg = 0;
	FP->dot = m->name;
	if(m->eval != nil){
		FP->op = nil;
		FP->end = PC;
		return (*m->eval)();
	}
	FP->dot = forkname(FP->dot);
	FP->aux = PC;
	FP->start = m->start;
	FP->end = m->end;
	PC = FP->start;
	return nil;
}

static void*
evalret(void)
{
	void *r = FP->arg[0];
	int brk = (FP->op - optab) != Oret;
	while(--FP >= FB){
		switch(FP->op - optab){
		case Owhile:
			if(!brk)
				continue;
			PC = FP->end;
			return nil;
		case Ocall:
			PC = FP->aux;
			return r;
		}
	}
	FP = FB;
	PC = FB->end;
	return r;
}

static void*
evalenv(void)
{
	Ref *r;
	Env *e;
	int c;

	if((e = FP->env) == nil)
		return nil;
	c = FP->start[0];
	if(c >= 0x60 && c <= 0x67){
		r = mk('L', sizeof(Ref));
		r->ptr = &e->loc[c - 0x60];
	} else if(c >= 0x68 && c <= 0x6E){
		r = mk('A', sizeof(Ref));
		r->ptr = &e->arg[c - 0x68];
	} else
		return nil;
	r->ref = e;
	return r;
}

static void*
evalstore(void)
{
	return store(FP->arg[0], FP->arg[1]);
}

static void*
evalcat(void)
{
	void *r, *a, *b;
	int tag, n, m;

	a = FP->arg[0];
	b = FP->arg[1];
	if(a == nil || TAG(a) == 'i')
		a = copy('b', a);	/* Concat(Int, ???) -> Buf */
	tag = TAG(a);
	if(b == nil || TAG(b) != tag)
		b = copy(tag, b);
	if(TAG(b) != tag)
		return nil;	/* botch */
	switch(tag){
	default:
		return nil;	/* botch */
	case 'b':
		n = SIZE(a);
		m = SIZE(b);
		r = mk('b', n + m);
		memmove(r, a, n);
		memmove((uchar*)r + n, b, m);
		break;
	case 's':
		n = strlen((char*)a);
		m = strlen((char*)b);
		r = mk('s', n + m + 1);
		memmove(r, a, n);
		memmove((char*)r + n, b, m);
		((char*)r)[n+m] = 0;
		break;
	}
	store(r, FP->arg[2]);
	return r;
}

static void*
evalindex(void)
{
	Field *f;
	void *p;
	Ref *r;
	int x;

	x = ival(FP->arg[1]);
	if(p = deref(FP->arg[0])) switch(TAG(p)){
	case 's':
		if(x >= strlen((char*)p))
			break;
		/* no break */
	case 'b':
		if(x < 0 || x >= SIZE(p))
			break;
		f = mk('u', sizeof(Field));
		f->reg = p;
		f->bitlen = 8;
		f->bitoff = 8*x;
		store(f, FP->arg[2]);
		return f;
	case 'p':
		if(x < 0 || x >= (SIZE(p)/sizeof(void*)))
			break;
		if(TAG(FP->arg[0]) == 'A' || TAG(FP->arg[0]) == 'L')
			r = mk(TAG(FP->arg[0]), sizeof(Ref));
		else
			r = mk('R', sizeof(Ref));
		r->ref = p;
		r->ptr = ((void**)p) + x;
		store(r, FP->arg[2]);
		return r;
	}
	return nil;
}

static void*
evalcondref(void)
{
	void *s;
	if((s = FP->arg[0]) == nil)
		return nil;
	store(s, FP->arg[1]);
	return mki(1);
}

static void*
evalsize(void)
{
	return mki(amllen(FP->arg[0]));
}

static void*
evalderef(void)
{
	void *p;

	if(p = FP->arg[0]){
		if(TAG(p) == 's' || TAG(p) == 'n')
			p = getname(FP->dot, (char*)p, 0);
		p = deref(p);
	}
	return p;
}

static void*
evalarith(void)
{
	uvlong v, d;
	void *r;
	int i;

	r = nil;
	switch(FP->op - optab){
	case Oadd:
		r = mki(ival(FP->arg[0]) + ival(FP->arg[1]));
		break;
	case Osub:
		r = mki(ival(FP->arg[0]) - ival(FP->arg[1]));
		break;
	case Omul:
		r = mki(ival(FP->arg[0]) * ival(FP->arg[1]));
		break;
	case Omod:
	case Odiv:
		v = ival(FP->arg[0]);
		d = ival(FP->arg[1]);
		if(d == 0){
			print("aml: division by zero: PC=%#p\n", PC);
			return nil;
		}
		r = mki(v % d);
		store(r, FP->arg[2]);
		if((FP->op - optab) != Odiv)
			return r;
		r = mki(v / d);
		store(r, FP->arg[3]); 
		return r;
	case Oshl:
		r = mki(ival(FP->arg[0]) << ival(FP->arg[1]));
		break;
	case Oshr:
		r = mki(ival(FP->arg[0]) >> ival(FP->arg[1]));
		break;
	case Oand:
		r = mki(ival(FP->arg[0]) & ival(FP->arg[1]));
		break;
	case Onand:
		r = mki(~(ival(FP->arg[0]) & ival(FP->arg[1])));
		break;
	case Oor:
		r = mki(ival(FP->arg[0]) | ival(FP->arg[1]));
		break;
	case Onor:
		r = mki(~(ival(FP->arg[0]) | ival(FP->arg[1])));
		break;
	case Oxor:
		r = mki(ival(FP->arg[0]) ^ ival(FP->arg[1]));
		break;
	case Onot:
		r = mki(~ival(FP->arg[0]));
		store(r, FP->arg[1]);
		return r;
	case Olbit:
		v = ival(FP->arg[0]);
		if(v == 0)
			break;
		for(i=0; (v & 1) == 0; i++)
			v >>= 1;
		r = mki(i+1);
		break;
	case Orbit:
		v = ival(FP->arg[0]);
		if(v == 0)
			break;
		for(i=0; v != 0; i++)
			v >>= 1;
		r = mki(i);
		break;
	case Oland:
		return mki(ival(FP->arg[0]) && ival(FP->arg[1]));
	case Olor:
		return mki(ival(FP->arg[0]) || ival(FP->arg[1]));
	case Olnot:
		return mki(ival(FP->arg[0]) == 0);

	case Oinc:
		r = mki(ival(deref(FP->arg[0]))+1);
		store(r, FP->arg[0]);
		return r;
	case Odec:
		r = mki(ival(deref(FP->arg[0]))-1);
		store(r, FP->arg[0]);
		return r;
	}

	store(r, FP->arg[2]);
	return r;
}

static void*
evalload(void)
{
	enum { LenOffset = 4, HdrLen = 36 };
	uvlong *tid;
	Region *r;
	int l;

	tid = nil;
	if(FP->aux){
		if(PC >= FP->end){
			amlenum(amlroot, nil, fixnames, nil);
			FP->aux = nil;
			FP->end = PC;
			tid = mki(1);	/* fake */
		}
	} else {
		store(nil, FP->arg[1]);
		if(FP->arg[0] == nil)
			return nil;

		l = rwreg(FP->arg[0], LenOffset, 4, 0, 0);
		if(l <= HdrLen)
			return nil;

		FP->aux = PC;	/* save */
		FP->ref = FP->arg[0];
		switch(TAG(FP->ref)){
		case 'b':
			if(SIZE(FP->ref) < l)
				return nil;
			PC = (uchar*)FP->ref + HdrLen;
			break;
		case 'r':
			r = FP->ref;
			if(r->len < l || r->va == nil || r->mapped <= 0)
				return nil;
			PC = (uchar*)r->va + HdrLen;
			break;
		default:
			return nil;
		}
		FP->end = PC + (l - HdrLen);
		FP->dot = amlroot;
		FP->env = nil;

		tid = mki(1); /* fake */
		store(tid, FP->arg[1]);
	}
	return tid;
}

static void*
evalstall(void)
{
	amldelay(ival(FP->arg[0]));
	return nil;
}

static void*
evalsleep(void)
{
	amldelay(ival(FP->arg[0])*1000);
	return nil;
}

static Op optab[] = {
	[Obad]		"",			"",		evalbad,
	[Onop]		"Noop",			"",		evalnop,
	[Odebug]	"Debug",		"",		evalnop,

	[Ostr]		".str",			"s",		evaliarg0,
	[Obyte]		".byte",		"1",		evaliarg0,
	[Oword]		".word",		"2",		evaliarg0,
	[Odword]	".dword",		"4",		evaliarg0,
	[Oqword]	".qword",		"8",		evaliarg0,
	[Oconst]	".const",		"",		evalconst,
	[Onamec]	".name",		"",		evalnamec,
	[Oenv]		".env",			"",		evalenv,

	[Oname]		"Name",			"N*",		evalname,
	[Oscope]	"Scope",		"{n}",		evalscope,
	[Oalias]	"Alias",		"nN",		evalalias,

	[Odev]		"Device",		"{N}",		evalscope,
	[Ocpu]		"Processor",		"{N141}",	evalscope,
	[Othz]		"ThermalZone",		"{N}",		evalscope,
	[Oprc]		"PowerResource",	"{N12}",	evalscope,

	[Oreg]		"OperationRegion",	"N1ii",		evalreg,
	[Ofld]		"Field",		"{n1",		evalfield,
	[Oxfld]		"IndexField",		"{nn1",		evalfield,

	[Ocfld]		"CreateField",		"*iiN",		evalcfield,
	[Ocfld0]	"CreateBitField",	"*iN",		evalcfield,
	[Ocfld1]	"CreateByteField",	"*iN",		evalcfield,
	[Ocfld2]	"CreateWordField",	"*iN",		evalcfield,
	[Ocfld4]	"CreateDWordField",	"*iN",		evalcfield,
	[Ocfld8]	"CreateQWordField",	"*iN",		evalcfield,

	[Opkg]		"Package",		"{1}",		evalpkg,
	[Ovpkg]		"VarPackage",		"{i}",		evalpkg,
	[Obuf]		"Buffer",		"{i",		evalbuf,
	[Omet]		"Method",		"{N1",		evalmet,

	[Oadd]		"Add",			"ii@",		evalarith,
	[Osub]		"Subtract",		"ii@",		evalarith,
	[Omod]		"Mod",			"ii@",		evalarith,
	[Omul]		"Multiply",		"ii@",		evalarith,
	[Odiv]		"Divide",		"ii@@",		evalarith,
	[Oshl]		"ShiftLef",		"ii@",		evalarith,
	[Oshr]		"ShiftRight",		"ii@",		evalarith,
	[Oand]		"And",			"ii@",		evalarith,
	[Onand]		"Nand",			"ii@",		evalarith,
	[Oor]		"Or",			"ii@",		evalarith,
	[Onor]		"Nor",			"ii@",		evalarith,
	[Oxor]		"Xor",			"ii@",		evalarith,
	[Onot]		"Not",			"i@",		evalarith,

	[Olbit]		"FindSetLeftBit",	"i@",		evalarith,
	[Orbit]		"FindSetRightBit",	"i@",		evalarith,

	[Oinc]		"Increment",		"@",		evalarith,
	[Odec]		"Decrement",		"@",		evalarith,

	[Oland]		"LAnd",			"ii",		evalarith,
	[Olor]		"LOr",			"ii",		evalarith,
	[Olnot]		"LNot",			"i",		evalarith,

	[Oleq]		"LEqual",		"**",		evalcmp,
	[Olgt]		"LGreater",		"**",		evalcmp,
	[Ollt]		"LLess",		"**",		evalcmp,

	[Omutex]	"Mutex",		"N1",		evalnop,
	[Oevent]	"Event",		"N",		evalnop,

	[Oif]		"If",			"{i}",		evalcond,
	[Oelse]		"Else",			"{}",		evalcond,
	[Owhile]	"While",		"{,i}",		evalcond,
	[Obreak]	"Break",		"",		evalret,
	[Oret]		"Return",		"*",		evalret,
	[Ocall]		"Call",			"",		evalcall,

	[Ostore]	"Store",		"*@",		evalstore,
	[Oindex]	"Index",		"@i@",		evalindex,
	[Osize]		"SizeOf",		"*",		evalsize,
	[Oref]		"RefOf",		"@",		evaliarg0,
	[Ocref]		"CondRefOf",		"@@",		evalcondref,
	[Oderef]	"DerefOf",		"@",		evalderef,
	[Ocat]		"Concatenate",		"**@",		evalcat,

	[Oacq]		"Acquire",		"@2",		evalnop,
	[Orel]		"Release",		"@",		evalnop,
	[Ostall]	"Stall",		"i",		evalstall,
	[Osleep]	"Sleep",		"i",		evalsleep,
	[Oload] 	"Load", 		"*@}", 		evalload,
	[Ounload]	"Unload",		"@",		evalnop,
};

static uchar octab1[] = {
/* 00 */	Oconst,	Oconst,	Obad,	Obad,	Obad,	Obad,	Oalias,	Obad,
/* 08 */	Oname,	Obad,	Obyte,	Oword,	Odword,	Ostr,	Oqword,	Obad,
/* 10 */	Oscope,	Obuf,	Opkg,	Ovpkg,	Omet,	Obad,	Obad,	Obad,
/* 18 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 20 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 28 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Onamec,	Onamec,
/* 30 */	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,
/* 38 */	Onamec,	Onamec,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 40 */	Obad,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,
/* 48 */	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,
/* 50 */	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,	Onamec,
/* 58 */	Onamec,	Onamec,	Onamec,	Obad,	Onamec,	Obad,	Onamec,	Onamec,
/* 60 */	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,
/* 68 */	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,	Oenv,	Obad,
/* 70 */	Ostore,	Oref,	Oadd,	Ocat,	Osub,	Oinc,	Odec,	Omul,
/* 78 */	Odiv,	Oshl,	Oshr,	Oand,	Onand,	Oor,	Onor,	Oxor,
/* 80 */	Onot,	Olbit,	Orbit,	Oderef,	Obad,	Omod,	Obad,	Osize,
/* 88 */	Oindex,	Obad,	Ocfld4,	Ocfld2,	Ocfld1,	Ocfld0,	Obad,	Ocfld8,
/* 90 */	Oland,	Olor,	Olnot,	Oleq,	Olgt,	Ollt,	Obad,	Obad,
/* 98 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* A0 */	Oif,	Oelse,	Owhile,	Onop,	Oret,	Obreak,	Obad,	Obad,
/* A8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* B0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* B8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* C0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* C8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* D0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* D8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* E0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* E8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* F0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* F8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Oconst,
};

static uchar octab2[] = {
/* 00 */	Obad,	Omutex,	Oevent,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 08 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 10 */	Obad,	Obad,	Ocref,	Ocfld,	Obad,	Obad,	Obad,	Obad,
/* 18 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 20 */	Oload,	Ostall,	Osleep,	Oacq,	Obad,	Obad,	Obad,	Orel,
/* 28 */	Obad,	Obad,	Ounload,Obad,	Obad,	Obad,	Obad,	Obad,
/* 30 */	Obad,	Odebug,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 38 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 40 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 48 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 50 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 58 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 60 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 68 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 70 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 78 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 80 */	Oreg,	Ofld,	Odev,	Ocpu,	Oprc,	Othz,	Oxfld,	Obad,
/* 88 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 90 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* 98 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* A0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* A8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* B0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* B8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* C0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* C8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* D0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* D8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* E0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* E8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* F0 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
/* F8 */	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,	Obad,
};

int
amltag(void *p)
{
	return p ? TAG(p) : 0;
}

void*
amlval(void *p)
{
	return deref(p);
}

uvlong
amlint(void *p)
{
	return ival(p);
}

int
amllen(void *p)
{
	while(p){
		switch(TAG(p)){
		case 'R':
			p = *((Ref*)p)->ptr;
			continue;
		case 'n':
		case 's':
			return strlen((char*)p);
		case 'p':
			return SIZE(p)/sizeof(void*);
		default:
			return SIZE(p);
		}
	}
	return 0;
}

void*
amlnew(char tag, int len)
{
	switch(tag){
	case 'i':
		return mki(0);
	case 'n':
	case 's':
		return mk(tag, len + 1);
	case 'p':
		return mk(tag, len * sizeof(void*));
	default:
		return mk(tag, len);
	}
}

static void*
evalosi(void)
{
	static char *w[] = { 
		"Windows 2001",
		"Windows 2001 SP1",
		"Windows 2001 SP2",
		"Windows 2006",
	};
	char *s;
	int i;

	s = FP->env->arg[0];
	if(s == nil || TAG(s) != 's')
		return nil;
	for(i = 0; i < nelem(w); i++)
		if(strcmp(s, w[i]) == 0)
			return mki(0xFFFFFFFF);
	return nil;
}

void
amlinit(void)
{
	Name *n;

	fmtinstall('V', Vfmt);
	fmtinstall('N', Nfmt);

	n = mk('N', sizeof(Name));
	n->up = n;

	amlroot = n;

	getname(amlroot, "_GPE", 1);
	getname(amlroot, "_PR", 1);
	getname(amlroot, "_SB", 1);
	getname(amlroot, "_TZ", 1);
	getname(amlroot, "_SI", 1);
	getname(amlroot, "_GL", 1);

	if(n = getname(amlroot, "_REV", 1))
		n->v = mki(2);
	if(n = getname(amlroot, "_OS", 1))
		n->v = mks("Microsoft Windows");
	if(n = getname(amlroot, "_OSI", 1)){
		Method *m;

		m = mk('m', sizeof(Method));
		m->narg = 1;
		m->eval = evalosi;
		m->name = n;
		n->v = m;
	}
}

void
amlexit(void)
{
	amlroot = nil;
	FP = FB-1;
	gc();
}

int
amlload(uchar *data, int len)
{
	int ret;

	ret = xec(data, data+len, amlroot, nil, nil);
	amlenum(amlroot, nil, fixnames, nil);
	return ret;
}

void*
amlwalk(void *dot, char *name)
{
	return getname(dot, name, 0);
}

void
amlenum(void *dot, char *seg, int (*proc)(void *, void *), void *arg)
{
	Name *n, *d;
	int rec;

	d = dot;
	if(d == nil || TAG(d) != 'N')
		return;
	do {
		rec = 1;
		if(seg == nil || memcmp(seg, d->seg, sizeof(d->seg)) == 0)
			rec = (*proc)(d, arg) == 0;
		for(n = d->down; n && rec; n = n->next)
			amlenum(n, seg, proc, arg);
		d = d->fork;
	} while(d);
}

int
amleval(void *dot, char *fmt, ...)
{
	va_list a;
	Method *m;
	void **r;
	Env *e;
	int i;

	va_start(a, fmt);
	e = mk('E', sizeof(Env));
	for(i=0;*fmt;fmt++){
		switch(*fmt){
		default:
			return -1;
		case 's':
			e->arg[i++] = mks(va_arg(a, char*));
			break;
		case 'i':
			e->arg[i++] = mki(va_arg(a, int));
			break;
		case 'I':
			e->arg[i++] = mki(va_arg(a, uvlong));
			break;
		case 'b':
		case 'p':
		case '*':
			e->arg[i++] = va_arg(a, void*);
			break;
		}
	}
	r = va_arg(a, void**);
	va_end(a);
	if(dot = deref(dot))
	if(TAG(dot) == 'm'){
		m = dot;
		if(i != m->narg)
			return -1;
		if(m->eval == nil)
			return xec(m->start, m->end, forkname(m->name), e, r);
		FP = FB;
		FP->op = nil;
		FP->env = e;
		FP->narg = 0;
		FP->dot = m->name;
		FP->ref = FP->aux = nil;
		dot = (*m->eval)();
	}
	if(r != nil)
		*r = dot;
	return 0;
}

void
amltake(void *p)
{
	if(p != nil)
		D2H(p)->mark |= 2;
}

void
amldrop(void *p)
{
	if(p != nil)
		D2H(p)->mark &= ~2;
}
