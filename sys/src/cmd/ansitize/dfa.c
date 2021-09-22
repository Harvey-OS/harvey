/*
 * Regular expression search.  Reimplementation of Plan 9 grep.
 * Same basic ideas, completely new code.
 *
 * Build NFA and run cached subset construction on the fly
 * during execution.  Also translate UTF-8 expressions into
 * corresponding byte sequences so that machine can run
 * byte-at-a-time.
 *
 * Not as pretty as I would like.
 *
 * Russ Cox, February 2006
 */
#include "a.h"

/*
 * Manual exception stack.
 */
typedef struct Error Error;
struct Error
{
	jmp_buf jb;
};
#define waserror(e) setjmp((e)->jb)
#define nexterror(e) longjmp((e)->jb, 1)

static void
kaboom(Error *e, char *fmt, ...)
{
	va_list arg;
	
	va_start(arg, fmt);
	fprint(2, "regparse: ");
	vfprint(2, fmt, arg);
	fprint(2, "\n");
	va_end(arg);
	nexterror(e);
}

/*
 * NFA machinery
 */
typedef struct Arrow Arrow;
typedef struct NFA NFA;
typedef struct NState NState;
typedef struct Parse Parse;

enum
{
	ANone,
	ABol,
	AEol,
	AEmpty,
	ARune,
	AByte,
	ANever
};
struct Arrow
{
	int op;
	NState *z;
	Rune lo, hi;
};

struct NState
{
	int id;
	int gen;
	int muststop;
	Arrow *arrow;
	int narrow;
	Arrow *rarrow;
	int nrarrow;
	NState *in1;
	void *val;
	uint valprec;
};

struct NFA
{
	NState *start;
	NState *end;
	NFA *next;
	uint gen;
	uint nnstate;
};

struct Parse
{
	Error *e;
	NFA *stacktop;
	NFA *freeme;
	NState *freemez;
};

static NFA *parseregexp2(char**, Parse*, int);
static int parseregexp1(char**, Parse*, int);
static void parseclass(char**, Parse*);
static void pusharrow(int, Rune, Rune, Parse*);
static NFA *pushnew(Parse*);
static NFA *pushnew0(Parse*);
static NFA *peek(Parse*);
static void arrow(NState*, NState*, int, Rune, Rune, Parse*);
static void freenfa(NFA*);
static void freenstate(NState*);
static void kaboom(Error*, char*, ...);
static void concatstack(NFA*, Parse*);
static void orstack(NFA*, Parse*);
static NState *mkstate(Parse*);
static int optl(NState*, uint*, Parse*);
static int optr(NState*, uint*, Parse*);
static void or(Parse*);
static void concat(Parse*);
static void pushrange(Rune, Rune, Parse*);
static int optor(Parse*);

enum
{
	Flags = RegexpShortest
};

NFA*
parseregexp(char *s, int mode)
{
	Error e;
	Parse p;
	NFA *m;
	int flag;

	flag = mode&Flags;
	mode &= ~Flags;
	memset(&p, 0, sizeof p);
	p.e = &e;
	if(waserror(&e)){
fprint(2, "bad regexp/%d: %s\n", mode, s);
if (0) {
		for(m=p.stacktop; m; m=m->next)
			freenfa(m);
		freenfa(p.freeme);
		freenstate(p.freemez);
}
		return nil;
	}
	
	m = parseregexp2(&s, &p, mode);
	if(*s != 0)
		kaboom(&e, "internal error - parseregexp");
	if(flag&RegexpShortest)
		m->end->muststop = 1;
	return m;
}

static NFA*
parseregexp2(char **t, Parse *p, int mode)
{
	char *s;
	NFA *top, *top2;
	
	s = *t;
	if((mode != RegexpLiteral && *s == ')') || *s == 0)
		return pushnew0(p);

	top = p->stacktop;
	goto parse;

	while(mode != RegexpLiteral && *s == '|'){
		s++;
	parse:
		top2 = p->stacktop;
		while(parseregexp1(&s, p, mode))
			;
		if(p->stacktop == top2)
			pushnew0(p);
		concatstack(top2, p);
	}	
	orstack(top, p);
	*t = s;
	return p->stacktop;
}

static Rune
classchar(char **t)
{
	char *s;
	int n;
	Rune r;

	s = *t;
	if(*s == '\\'){
		s++;
		if(*s == 'n'){
			*t = ++s;
			return '\n';
		}
		if(*s == 't'){
			*t = ++s;
			return '\t';
		}
		if(*s == 'r'){
			*t = ++s;
			return '\r';
		}
		if(*s == 'v'){
			*t = ++s;
			return '\v';
		}
		if(*s == 'b'){
			*t = ++s;
			return '\b';
		}
		if(*s == 'a'){
			*t = ++s;
			return '\a';
		}
	}
	if(*s == 0){
		*t = s;
		return 0;
	}
	s += (n=chartorune(&r, s));
	if(n==1 && r==Runeerror)
		return 0;
	*t = s;
	return r;
}

static int
parseregexp1(char **t, Parse *p, int mode)
{
	char *s, *ss;
	NFA *m;
	NState *z;
	Rune r;

	s = *t;
	if(*s == 0)
		return 0;
	if(mode == RegexpLiteral)
		goto literal;

	switch(*s){
	case '|':
	case ')':
	case 0:
		return 0;
	case '^':
		s++;
		pusharrow(ABol, 0, 0, p);
		break;
	case '$':
		s++;
		pusharrow(AEol, 0, 0, p);
		break;
	case '[':
		parseclass(&s, p);
		break;
	case '.':
		s++;
		ss = "[^\\n]";
		parseclass(&ss, p);
		break;
	case '*':
		s++;
		m = peek(p);
		z = mkstate(p);
		m->nnstate++;
		p->freemez = z;
		arrow(z, m->start, AEmpty, 0, 0, p);
		arrow(m->end, z, AEmpty, 0, 0, p);
		if(!optr(m->start, &m->nnstate, p) || m->start != m->end)
			optl(m->end, &m->nnstate, p);
		m->start = z;
		m->end = z;
		p->freemez = nil;
		break;
	case '?':
		s++;
		m = peek(p);
		if(m->start == m->end)
			break;
		z = mkstate(p);
		m->nnstate++;
		p->freemez = z;
		arrow(z, m->start, AEmpty, 0, 0, p);
		arrow(z, m->end, AEmpty, 0, 0, p);
		optr(m->start, &m->nnstate, p);
		if(optr(m->end, &m->nnstate, p))
			m->end = z;
		m->start = z;
		p->freemez = nil;
		break;
	case '+':
		s++;
		m = peek(p);
		if(m->start == m->end)
			break;
		arrow(m->end, m->start, AEmpty, 0, 0, p);
		break;
	case '(':
		s++;
		parseregexp2(&s, p, mode);
		if(*s != ')')
			kaboom(p->e, "syntax - missing ')'");
		s++;
		break;
	default:
	literal:
		r = classchar(&s);
		if(r == 0)
			kaboom(p->e, "bad UTF");
		pushrange(r, r, p);
		break;
	}
	*t = s;
	return 1;
}

typedef struct Class Class;
struct Class
{
	Rune lo;
	Rune hi;
};

static void pushbytes(char*, char*, int, Parse*);
static int simpleclass(Class*, int);

static void
parseclass(char **t, Parse *p)
{
	char *s;
	int i, nc, invert;
	Class *c, *c2;
	NFA *top;
	Error *oe, e;
	int lo, hi;
	
	/* XXX inversion */

	s = *t;
	if(*s != '[')
		kaboom(p->e, "internal error - parseclass");
	s++;
	invert = 0;
	if(*s == '^'){
		invert = 1;
		s++;
	}
	c = nil;
	nc = 0;
	while(*s != ']'){
		if((lo=classchar(&s)) == 0){
		badclasschar:
			free(c);
			if(*s == 0)
				kaboom(p->e, "syntax - missing ']'");
			else
				kaboom(p->e, "bad UTF");
		}
		if(*s == '-'){
			s++;
			if((hi=classchar(&s)) == 0)
				goto badclasschar;
			if(hi < lo){
				free(c);
				kaboom(p->e, "bad range %C-%C", lo, hi);
			}
		}else
			hi = lo;
		if(nc%8 == 0){
			c2 = realloc(c, (nc+8)*sizeof c[0]);
			if(c2 == nil){
				free(c);
				kaboom(p->e, "out of memory");
			}
			c = c2;
		}
		c[nc].lo = lo;
		c[nc].hi = hi;
		nc++;
	}
	*t = s+1;
	nc = simpleclass(c, nc);
	if(invert){
		if(nc%8 == 0){
			c2 = realloc(c, (nc+8)*sizeof c[0]);
			if(c2 == nil){
				free(c);
				kaboom(p->e, "out of memory");
			}
			c = c2;
		}
		lo = 0;
		for(i=0; i<nc; i++){
			hi = c[i].hi;
			c[i].hi = c[i].lo-1;
			c[i].lo = lo;
			lo = hi+1;
		}
		if(lo <= 0xFFFF){
			c[nc].lo = lo;
			c[nc].hi = 0xFFFF;
			nc++;
		}
		nc = simpleclass(c, nc);
	}
	if(nc == 0){
		pusharrow(ANever, 0, 0, p);
		free(c);
		return;
	}
	oe = p->e;
	p->e = &e;
	if(waserror(&e)){
		free(c);
		p->e = oe;
		nexterror(p->e);
	}
	top = p->stacktop;
	for(i=0; i<nc; i++)
		pushrange(c[i].lo, c[i].hi, p);
	free(c);
	p->e = oe;
	orstack(top, p);
	optor(p);
}

static int
classcmp(const void *va, const void *vb)
{
	return ((Class*)va)->lo - ((Class*)vb)->lo;
}

static int
simpleclass(Class *c, int nc)
{
	Class *r, *w, *e;
	
	if(nc == 0)
		return 0;
	qsort(c, nc, sizeof c[0], classcmp);
	for(r=w=c, e=c+nc; r<e; r++){
		if(w>c && r->lo <= (w-1)->hi+1){
			if((w-1)->hi < r->hi)
				(w-1)->hi = r->hi;
		}else
			*w++ = *r;
	}
	return w-c;
}

enum
{
	Rune2 = (1<<7),	/* first 2-byte rune */
	Rune3 = (1<<(5+6)),	/* first 3-byte rune */
	
	RuneX0 = 0x80,	/* minimum extension byte */
	RuneX1 = 0xBF,	/* maximum extension byte */
};

static void
pushrange(Rune lo, Rune hi, Parse *p)
{
	int l1, l2;
	char u1[UTFmax], u2[UTFmax];
	
	if(0){
		pusharrow(ARune, lo, hi, p);
		return;
	}
	if(lo < Rune2 && hi >= Rune2){
		pushrange(lo, Rune2-1, p);
		pushrange(Rune2, hi, p);
		or(p);
		return;
	}
	if(lo < Rune3 && hi >= Rune3){
		pushrange(lo, Rune3-1, p);
		pushrange(Rune3, hi, p);
		or(p);
		return;
	}
	l1 = runetochar(u1, &lo);
	l2 = runetochar(u2, &hi);
	if(l1 != l2)
		kaboom(p->e, "internal error - utf");
	pushbytes(u1, u2, l1, p);
}

static void
pushbyte2(char a, char b, char c, char d, Parse *p)
{
	pusharrow(AByte, a, b, p);
	pusharrow(AByte, c, d, p);
	concat(p);
}

static void
pushbyte3(char a, char b, char c, char d, char e, char f, Parse *p)
{
	pusharrow(AByte, a, b, p);
	pusharrow(AByte, c, d, p);
	pusharrow(AByte, e, f, p);
	concat(p);
	concat(p);
}

static void
pushbytes(char *a, char *b, int l, Parse *p)
{
	NFA *top;
	
	switch(l){
	default:
		kaboom(p->e, "internal error - utf len");
		
	case 1:
		pusharrow(AByte, a[0], b[0], p);
		return;
	
	case 2:	/* a1 a2 - b1 b2 */
		top = p->stacktop;
		if(a[0] != b[0] && (uchar)a[1] != RuneX0){
			/* a1 a2 - a1 BF */
			pushbyte2(a[0], a[0], a[1], RuneX1, p);
			a[0]++;
			a[1] = RuneX0;
		}
		if(a[0] != b[0] && (uchar)b[1] != RuneX1){
			/* now a2 == 80: a1 a2 - b1-1 BF */
			pushbyte2(a[0], b[0]-1, RuneX0, RuneX1, p);
			a[0] = b[0];
		}
		/* now a1 a2 - b1 b2 */
		pushbyte2(a[0], b[0], a[1], b[1], p);
		orstack(top, p);
		return;

	case 3:
		top = p->stacktop;
		if((a[0] != b[0] || a[1] != b[1]) && (uchar)a[2] != RuneX0){
			pushbyte3(a[0], a[0], a[1], a[1], a[2], RuneX1, p);
			a[2] = RuneX0;
			if((uchar)a[1]++ == RuneX1){
				a[1] = RuneX0;
				a[0]++;
			}
		}
		if((a[0] != b[0]) && (uchar)a[1] != RuneX0){
			pushbyte3(a[0], a[0], a[1], RuneX1, RuneX0, RuneX1, p);
			a[0]++;
			a[1] = RuneX0;
		}
		if(a[0] != b[0] && ((uchar)b[1] != RuneX1 || (uchar)b[2] != RuneX1)){
			pushbyte3(a[0], b[0]-1, RuneX0, RuneX1, RuneX0, RuneX1, p);
			a[0] = b[0];
		}
		if(a[1] != b[1] && (uchar)b[1] != RuneX1){
			pushbyte3(a[0], b[0], a[1], b[1]-1, RuneX0, RuneX1, p);
			a[1] = b[1];
		}
		pushbyte3(a[0], b[0], a[1], b[1], a[2], b[2], p);
		orstack(top, p);
		return;
	}		
}


static NFA*
pushnew(Parse *p)
{
	NFA *m;
	
	m = malloc(sizeof *m);
	if(m == nil)
		kaboom(p->e, "out of memory");
	m->gen = 0;
	m->next = p->stacktop;
	p->stacktop = m;
	m->start = malloc(sizeof *m->start);
	if(m->start == nil)
		kaboom(p->e, "out of memory");
	memset(m->start, 0, sizeof *m->start);
	m->end = malloc(sizeof *m->end);
	if(m->end == nil)
		kaboom(p->e, "out of memory");
	memset(m->end, 0, sizeof *m->end);
	m->nnstate = 2;
	return m;
}

static NFA*
pushnew0(Parse *p)
{
	NFA *m;
	
	m = malloc(sizeof *m);
	if(m == nil)
		kaboom(p->e, "out of memory");
	m->gen = 0;
	m->next = p->stacktop;
	p->stacktop = m;
	m->start = malloc(sizeof *m->start);
	if(m->start == nil)
		kaboom(p->e, "out of memory");
	memset(m->start, 0, sizeof *m->start);
	m->end = m->start;
	m->nnstate = 1;
	return m;
}

static NState*
mkstate(Parse *p)
{
	NState *z;
	
	z = malloc(sizeof *z);
	if(z == nil)
		kaboom(p->e, "out of memory");
	memset(z, 0, sizeof *z);
	return z;
}

static NFA*
peek(Parse *p)
{
	NFA *m;
	
	if((m=p->stacktop) == nil)
		kaboom(p->e, "internal error - stack underflow");
	return m;
}

static NFA*
pop(Parse *p)
{
	NFA *m;
	
	if((m=p->stacktop) == nil)
		kaboom(p->e, "internal error - stack underflow");
	p->stacktop = m->next;
	return m;
}

static void
push(NFA *m, Parse *p)
{
	m->next = p->stacktop;
	p->stacktop = m;
}

static void
arrow(NState *u, NState *v, int op, Rune lo, Rune hi, Parse *p)
{
	Arrow *a;
	Arrow *ra;
	
	if(u->narrow%8 == 0){
		a = realloc(u->arrow, (u->narrow+8)*sizeof u->arrow[0]);
		if(a == nil)
			kaboom(p->e, "out of memory");
		u->arrow = a;
	}
	if(v->nrarrow%8 == 0){
		a = realloc(v->rarrow, (v->nrarrow+8)*sizeof v->rarrow[0]);
		if(a == nil)
			kaboom(p->e, "out of memory");
		v->rarrow = a;
	}
	a = &u->arrow[u->narrow++];
	ra = &v->rarrow[v->nrarrow++];
	a->op = op;
	a->lo = lo;
	a->hi = hi;
	a->z = v;
	ra->op = op;
	ra->lo = lo;
	ra->hi = hi;
	ra->z = u;
}

static void
pusharrow(int type, Rune lo, Rune hi, Parse *p)
{
	NFA *m;
	
	m = pushnew(p);
	if(type == AByte){
		lo = (uchar)lo;
		hi = (uchar)hi;
	}
	arrow(m->start, m->end, type, lo, hi, p);
}

static void
join(NFA *stopat, Parse *p, void (*f)(Parse*))
{
	NFA *mtop;
	
	if((mtop=p->stacktop) == nil || mtop == stopat)
		kaboom(p->e, "internal error - stack mismatch");
	while(p->stacktop && p->stacktop->next && p->stacktop->next != stopat)
		f(p);
	if(p->stacktop == nil || p->stacktop->next != stopat)
		kaboom(p->e, "internal error - stack mismatch");
}

static void
or(Parse *p)
{
	NFA *m, *m1, *m2;
	
	m2 = p->stacktop;
	m1 = m2->next;

	m = pushnew(p);
	arrow(m->start, m1->start, AEmpty, 0, 0, p);
	arrow(m->start, m2->start, AEmpty, 0, 0, p);
	arrow(m1->end, m->end, AEmpty, 0, 0, p);
	arrow(m2->end, m->end, AEmpty, 0, 0, p);
	m->nnstate += m1->nnstate + m2->nnstate;

	if(!optr(m1->start, &m->nnstate, p) || m1->start != m1->end)
		optl(m1->end, &m->nnstate, p);
	if(!optr(m2->start, &m->nnstate, p) || m2->start != m2->end)
		optl(m2->end, &m->nnstate, p);

	pop(p);
	pop(p);
	pop(p);

	free(m1);
	free(m2);
	push(m, p);
}

static void
or2(Parse *p)
{
	NFA *m, *m1, *m2;

	m2 = p->stacktop;
	m1 = m2->next;

	if(m1->start->nrarrow == 0){
		arrow(m1->start, m2->start, AEmpty, 0, 0, p);
		m1->nnstate += m2->nnstate;
		pop(p);
		free(m2);
		return;
	}
	m = pushnew(p);
	m->nnstate += m1->nnstate + m2->nnstate;
	arrow(m->start, m1->start, AEmpty, 0, 0, p);
	arrow(m->start, m2->start, AEmpty, 0, 0, p);
	pop(p);
	pop(p);
	pop(p);
	
	free(m1);
	free(m2);
	push(m, p);
}

static void
concat(Parse *p)
{
	NFA *m1, *m2;
	NState *l, *r;
	
	m2 = p->stacktop;
	m1 = m2->next;

	arrow(l=m1->end, r=m2->start, AEmpty, 0, 0, p);
	pop(p);
	m1->end = m2->end;
	m1->nnstate += m2->nnstate;
	free(m2);
	if(optr(r, &m1->nnstate, p) && m1->end == r)
		m1->end = l;
	else if(optl(l, &m1->nnstate, p) && m1->start == l)
		m1->start = r;
}

static void
orstack(NFA *stopat, Parse *p)
{
	join(stopat, p, or);
}

static void
concatstack(NFA *stopat, Parse *p)
{
	join(stopat, p, concat);
}

static void
delarrow(NState *z, NState *znext)
{
	int i;
	
	for(i=0; i<z->narrow; i++){
		if(z->arrow[i].z == znext){
			z->narrow--;
			memmove(z->arrow+i, z->arrow+i+1, (z->narrow-i)*sizeof z->arrow[0]);
			i--;
		}
	}
}

static void
delrarrow(NState *z, NState *zprev)
{
	int i;
	
	for(i=0; i<z->nrarrow; i++){
		if(z->rarrow[i].z == zprev){
			z->nrarrow--;
			memmove(z->rarrow+i, z->rarrow+i+1, (z->nrarrow-i)*sizeof z->rarrow[0]);
			i--;
		}
	}
}

/* peephole optimizer - node is left-hand side of AEmpty arrow */
static int
optl(NState *z, uint *nnstate, Parse *p)
{
	int i;
	NState *znext, *zprev;

	if(z->narrow != 1 || z->arrow[0].op != AEmpty)
		return 0;
	
	znext = z->arrow[0].z;
	delrarrow(znext, z);
	for(i=0; i<z->nrarrow; i++){
		zprev = z->rarrow[i].z;
		delarrow(zprev, z);
		arrow(zprev, znext, z->rarrow[i].op,
			z->rarrow[i].lo, z->rarrow[i].hi, p);
	}
	freenstate(z);
	--*nnstate;
	return 1;
}

/* peephole optimizer - node is right-hand side of AEmpty arrow */
static int
optr(NState *z, uint *nnstate, Parse *p)
{
	int i;
	NState *znext, *zprev;

	if(z->nrarrow != 1 || z->rarrow[0].op != AEmpty)
		return 0;

	/* push outgoing arrows onto previous guy */
	zprev = z->rarrow[0].z;
	delarrow(zprev, z);
	for(i=0; i<z->narrow; i++){
		znext = z->arrow[i].z;
		delrarrow(znext, z);
		arrow(zprev, znext, z->arrow[i].op, 
			z->arrow[i].lo, z->arrow[i].hi, p);
	}
	freenstate(z);
	--*nnstate;
	return 1;
}

/* peephole optimizer - common outgoing arrows */
/* assumes NFA is dag - be careful! */
static void optor0(NState*, Parse*, uint*);
static void optor1(NState*, Parse*, uint*);
static int
optor(Parse *p)
{
	/* merge similarly labeled arrows */
	optor0(p->stacktop->start, p, &p->stacktop->nnstate);
	optor1(p->stacktop->end, p, &p->stacktop->nnstate);
	return 0;
}

static int
arrowcmp(const void *va, const void *vb)
{
	Arrow *a, *b;
	
	a = (Arrow*)va;
	b = (Arrow*)vb;
	if(a->op != b->op)
		return a->op - b->op;
	if(a->lo != b->lo)
		return a->lo - b->lo;
	if(a->hi != b->hi)
		return a->hi - b->hi;
	return 0;
}

static void
optormerge(NState *y, NState *z, Parse *p, uint *nnstate)
{
	int i;
	NState *next;
	
	for(i=0; i<z->narrow; i++){
		next = z->arrow[i].z;
		delrarrow(next, z);
		arrow(y, next, z->arrow[i].op, z->arrow[i].lo, z->arrow[i].hi, p);
	}
	freenstate(z);
	--*nnstate;
}

static void
optor0(NState *z, Parse *p, uint *nnstate)
{
	int i;
	Arrow *r, *w, *e;
	NState *zz;

	if(z == nil)
		return;

	if(z->narrow == 0)
		return;

	qsort(z->arrow, z->narrow, sizeof z->arrow[0], arrowcmp);
	for(r=w=z->arrow, e=r+z->narrow; r<e; r++){
		if(w>z->arrow && (w-1)->op==r->op && (w-1)->lo==r->lo && (w-1)->hi == r->hi){
			zz = r->z;
			optormerge((w-1)->z, zz, p, nnstate);
		}else
			*w++ = *r;
	}
	z->narrow = w-z->arrow;
	for(i=0; i<z->narrow; i++)
		optor0(z->arrow[i].z, p, nnstate);
}

static void
optormerge1(NState *y, NState *z, Parse *p, uint *nnstate)
{
	int i;
	NState *prev;
	
	for(i=0; i<z->nrarrow; i++){
		prev = z->rarrow[i].z;
		delarrow(prev, z);
		arrow(prev, y, z->rarrow[i].op, z->rarrow[i].lo, z->rarrow[i].hi, p);
	}
	freenstate(z);
	--*nnstate;
}

static void
optor1(NState *z, Parse *p, uint *nnstate)
{
	int i;
	Arrow *r, *w, *e;
	NState *zz;

	if(z == nil)
		return;

	if(z->nrarrow == 0)
		return;

	qsort(z->rarrow, z->nrarrow, sizeof z->rarrow[0], arrowcmp);
	for(r=w=z->rarrow, e=r+z->nrarrow; r<e; r++){
		if(w>z->rarrow && (w-1)->op==r->op && (w-1)->lo==r->lo && (w-1)->hi == r->hi){
			zz = r->z;
			optormerge1((w-1)->z, zz, p, nnstate);
		}else
			*w++ = *r;
	}
	z->nrarrow = w-z->rarrow;
	for(i=0; i<z->nrarrow; i++)
		optor1(z->rarrow[i].z, p, nnstate);
}

/* cleanup */

static void
freenstates(NState *root)
{
	int i;
	NState *z;
	
if (0) {
	for(i=0; i<root->narrow; i++){
		z = root->arrow[i].z;
		if(z){
			root->arrow[i].z = nil;
			z->nrarrow--;
			freenstates(z);
		}
	}
	if(root->nrarrow == 0)
		freenstate(root);
}
}

static void
freenfa(NFA *m)
{
	if(m->end && m->end->narrow == 0)
		freenstate(m->end);
	if(m->start)
		freenstates(m->start);
}

static void
freenstate(NState *z)
{
	free(z->arrow);
	free(z->rarrow);
	free(z);
}

/* assign id numbers and make array indexed by id */
static void
_number(NState **all, NState *z, int gen, int *id)
{
	int i;
	
	if(z == nil || z->gen == gen)
		return;
	z->gen = gen;
	z->id = (*id)++;
	if(all)
		all[z->id] = z;
	for(i=0; i<z->narrow; i++)
		_number(all, z->arrow[i].z, gen, id);
}

static int
number(NFA *m, NState ***pnstate)
{
	int id, nnstate;
	NState **nstate;
	
	id = 0;
	_number(nil, m->start, ++m->gen, &id);
	if(id == 0){
		*pnstate = nil;
		return 0;
	}
	if((nstate = malloc(id*sizeof nstate[0])) == nil)
		return -1;
	memset(nstate, 0, id*sizeof nstate[0]);	/* paranoia */
	nnstate = id;
	id = 0;
	_number(nstate, m->start, ++m->gen, &id);
	*pnstate = nstate;
	return nnstate;
}

/* example from graphviz gallery
digraph finite_state_machine {
	rankdir=LR;
	size="8,5"
	node [shape = doublecircle]; LR_0 LR_3 LR_4 LR_8;
	node [shape = circle];
	LR_0 -> LR_2 [ label = "SS(B)" ];
	LR_0 -> LR_1 [ label = "SS(S)" ];
	LR_1 -> LR_3 [ label = "S($end)" ];
	LR_2 -> LR_6 [ label = "SS(b)" ];
	LR_2 -> LR_5 [ label = "SS(a)" ];
	LR_2 -> LR_4 [ label = "S(A)" ];
	LR_5 -> LR_7 [ label = "S(b)" ];
	LR_5 -> LR_5 [ label = "S(a)" ];
	LR_6 -> LR_6 [ label = "S(b)" ];
	LR_6 -> LR_5 [ label = "S(a)" ];
	LR_7 -> LR_8 [ label = "S(b)" ];
	LR_7 -> LR_5 [ label = "S(a)" ];
	LR_8 -> LR_6 [ label = "S(b)" ];
	LR_8 -> LR_5 [ label = "S(a)" ];
}
*/

void
printnfa(NFA *m)
{
	int i, j;
	Arrow *a;
	NState *z;
	NState **nstate;
	int nnstate;
	
	if((nnstate=number(m, &nstate)) < 0){
		fprint(2, "numbering\n");
		exits("dfa");
	}
	
	print("# supposedly %d states\n", m->nnstate);
	print("digraph finite_state_machine {\n");
	print("\trankdir=LR;\n");
	print("\tsize=\"8,8\"\n");
	print("\tnode [shape = doublecircle]; Q%d;\n", m->end->id);
	print("\tnode [shape = circle];\n");
	
	for(i=0; i<nnstate; i++){
		z = nstate[i];
		for(j=0; j<z->narrow; j++){
			a = &z->arrow[j];
			print("\tQ%d -> Q%d [ label = \"", z->id, a->z->id);
			if(a->op == ABol)
				print("^");
			else if(a->op == AEol)
				print("$");
			else if(a->op == AEmpty)
				print("(e)");
			else if(a->op == ARune || a->op == AByte){
				if(a->lo < 0x7E && a->lo > ' ')
					print("%C", a->lo);
				else
					print("0x%ux", a->lo);
				if(a->lo != a->hi){
					if(a->hi < 0x7E && a->hi > ' ')
						print("-%C", a->hi);
					else
						print("-0x%ux", a->hi);
				}
			}else if(a->op == ANever)
				print("!!!");
			else
				print("???");
			print("\" ];\n");
		}
	}
	print("}\n");
}

/*
 * DFA machinery
 */
typedef struct DState DState;
typedef struct DScratch DScratch;

struct DFA
{
	NFA *nfa;
	DScratch *tmp;
	DState *dead;
	DState *start;
	DState **hash;
	uint nhash;
	uint nstate;
	int dirty;
};

struct DScratch
{
	NState **nstate;
	uint nnstate;
};

enum
{
	Eol = 256,
	Bol = 257,
};
struct DState
{
	DState *next[257];
	uint id;
	DState *hash;
	NState **nstate;
	uint nnstate;
	uint match;
	void *val;
};

static DScratch*
mkscratch(Error *e, NFA *m)
{
	DScratch *s;
	
//fprint(2, "mkscratch %d\n", m->nnstate);
	s = malloc(sizeof *s + m->nnstate*sizeof s->nstate[0]);
	if(s == nil)
		kaboom(e, "out of memory");
	s->nstate = (NState**)(s+1);
	s->nnstate = 0;
	return s;
}

static void
walk0(DScratch *tmp, uint gen, NState *z, int also)
{
	int i;

	if(z->gen == gen)
		return;
	z->gen = gen;
	tmp->nstate[tmp->nnstate++] = z;
	for(i=0; i<z->narrow; i++)
		if(z->arrow[i].op == AEmpty || (also && z->arrow[i].op == also))
			walk0(tmp, gen, z->arrow[i].z, also);
}

static void
walk(DScratch *tmp, NFA *m, NState **z, int nz, int also)
{
	int i;
	
	tmp->nnstate = 0;
	m->gen++;
	for(i=0; i<nz; i++)
		walk0(tmp, m->gen, z[i], also);
}

static void
walkbyte(DScratch *tmp, NFA *m, NState **z, int nz, int x)
{
	int i, j;
	
	x = (uchar)x;
	tmp->nnstate = 0;
	m->gen++;
	for(i=0; i<nz; i++)
		for(j=0; j<z[i]->narrow; j++)
			if(z[i]->arrow[j].op == AByte && z[i]->arrow[j].lo <= x && x <= z[i]->arrow[j].hi)
				walk0(tmp, m->gen, z[i]->arrow[j].z, x=='\n' ? AEol : 0);
}

static uint
canonhash(DScratch *tmp)
{
	int i;
	uint p;
	
	p = 10007;
	for(i=0; i<tmp->nnstate; i++)
		p = (p*127+tmp->nstate[i]->id)%100000007;
	return p;
}

static DState*
canonical(Error *e, DFA *d, DScratch *tmp)
{
	int i, h, size;
	DState *s;

	if(tmp->nnstate == 0)
		return d->dead;
	h = canonhash(tmp)%d->nhash;
	size = tmp->nnstate*sizeof tmp->nstate[0];
	for(s=d->hash[h]; s; s=s->hash)
		if(s->nnstate == tmp->nnstate && memcmp(s->nstate, tmp->nstate, size) == 0)
			return s;
	s = malloc(size+sizeof *s);
	if(s == nil)
		kaboom(e, "out of memory");
	memset(s->next, 0, sizeof s->next);
	s->id = ++d->nstate;
	s->nstate = (NState**)(s+1);
	memmove(s->nstate, tmp->nstate, size);
	s->nnstate = tmp->nnstate;
	s->match = 0;
	s->val = nil;

	for(i=0; i<s->nnstate; i++){
		if(s->nstate[i] == d->nfa->end || s->nstate[i]->val)
			s->match |= 1;
		if(s->nstate[i]->muststop)
			s->match |= 2;
	}
	s->hash = d->hash[h];
	d->hash[h] = s;
	return s;
}

static void
dfainit(Error *e, DFA *d)
{
	DState *s;
	
	if(d->dead == nil){
		s = malloc(sizeof *s);
		if(s == nil)
			kaboom(e, "out of memory");
		memset(s, 0, sizeof *s);
		s->id = -1;
		d->dead = s;
	}
	free(d->tmp);
	d->tmp = nil;
	d->tmp = mkscratch(e, d->nfa);
	walk(d->tmp, d->nfa, &d->nfa->start, 1, 0);
	assert(d->tmp->nnstate <= d->nfa->nnstate);
	d->start = canonical(e, d, d->tmp);
}

static void
dfareset(Error *e, DFA *d)
{
	int i;
	DState *s, *next;

	d->dirty = 0;
	for(i=0; i<d->nhash; i++){
		for(s=d->hash[i]; s; s=next){
			next = s->hash;
			free(s);
		}
		d->hash[i] = nil;
	}
	d->start = nil;
	d->nstate = 0;
	dfainit(e, d);
}

static DState*
walkstate(Error *e, DFA *d, DState *s, int c)
{
	if(c == Eol)
		walk(d->tmp, d->nfa, s->nstate, s->nnstate, AEol);
	else if(c == Bol)
		walk(d->tmp, d->nfa, s->nstate, s->nnstate, ABol);
	else
		walkbyte(d->tmp, d->nfa, s->nstate, s->nnstate, c);
	assert(d->tmp->nnstate <= d->nfa->nnstate);
	return canonical(e, d, d->tmp);
}


/*
 * Public interface for lexer.
 */
DFA*
mkdfa(void)
{
	DFA *d;

	d = malloc(sizeof *d+256*sizeof d->hash[0]);
	if(d == nil)
		return nil;
	memset(d, 0, sizeof *d+256*sizeof d->hash[0]);
	d->hash = (DState**)(d+1);
	d->nhash = 256;
	d->dirty = 1;
	return d;
}

int
addtodfa(DFA *d, char *re, int mode, int prec, void *val)
{
	NFA *m;
	Parse p;
	Error e;
	
	d->dirty = 1;
	if((m = parseregexp(re, mode)) == nil)
		return -1;
	m->end->val = val;
	m->end->valprec = prec;
	if(d->nfa == nil)
		d->nfa = m;
	else{
		memset(&p, 0, sizeof p);
		memset(&e, 0, sizeof e);
		p.e = &e;
		if(waserror(&e))	/* leak m XXX */
			return -1;
		push(d->nfa, &p);
		push(m, &p);
		or2(&p);
		d->nfa = pop(&p);
	}
	return 0;
}

void*
rundfa(DFA *d, char *p, char **pp)
{
	Error e;
	DState *q, *nq, *qmatch;
	NState *z;
	uchar *t, *match;
	int i, c, prec;
	void *val;
	
	if(waserror(&e))
		return nil;
	if(d->dirty)
		dfareset(&e, d);
	
	match = nil;
	qmatch = nil;
	q = d->start;
	for(t=(uchar*)p; c=*t; t++){
		if((nq = q->next[c]) == nil){
			if(q->id == -1)
				break;
			nq = walkstate(&e, d, q, c);
			q->next[c] = nq;
		}
		q = nq;
		if(q->match & 1){
			qmatch = q;
			match = t+1;
			if(q->match & 2)
				break;
		}
	}
	if(match == nil)
		return nil;
	*pp = (char*)match;
	if(qmatch->val == nil){
		val = nil;
		prec = 0;
		for(i=0; i<qmatch->nnstate; i++){
			z = qmatch->nstate[i];
			if(z->val)
			if(val == nil || prec < z->valprec){
				val = z->val;
				prec = z->valprec;
			}
		}
		qmatch->val = val;
	}
	return qmatch->val;
}

/*
 * Test harnesses.
 */

#ifdef TEST
static int
dfaexec(Error *e, DFA *d, char *s)
{
	uchar *t;
	char *match;
	DState *q, *nq;
	int c;
	
	if(d->dirty)
		dfareset(e, d);

	fprint(2, "%s\n", s);
	t = (uchar*)s;
	q = d->start;
	match = nil;
	for(; c=*t; t++){
		if(c == '\n'){
			if((nq = q->next[Eol]) == nil){
				if(q->id == -1)
					break;
				nq = walkstate(e, d, q, Eol);
				q->next[Eol] = nq;
			}
			q = nq;
			if(q->match)
				match = (char*)t+1;
		}
		if((nq = q->next[c]) == nil){
			if(q->id == -1)
				break;
			nq = walkstate(e, d, q, c);
			q->next[c] = nq;
		}
		q = nq;
		if(q->match)
			match = (char*)t+1;
	}
	if(match)
		fprint(2, "match: '%.*s'\n", utfnlen(s, match-s), s);
	else{
		if(*t == 0)
			fprint(2, "ran out of string - ");
		fprint(2, "no match\n");
	}
	return 0;
}

int
main(int argc, char **argv)
{
	int i;
	DFA d;
	DState *hash[1];
	NFA *m;
	Error e;

	if(argc < 2){
		fprint(2, "usage\n");
		exits("dfa");
	}
	
	m = parseregexp(argv[1], RegexpNormal);
	if(m == nil){
		fprint(2, "bad regexp\n");
		exits("dfa");
	}
	printnfa(m);
	
	memset(&d, 0, sizeof d);
	d.nfa = m;
	d.hash = hash;
	d.nhash = 1;
	if(waserror(&e))
		exits("dfa");
	dfainit(&e, &d);
	for(i=2; i<argc; i++)
		dfaexec(&e, &d, argv[i]);
	return 0;
}
#endif

#ifdef GREP
#undef uchar
#include "a.h"
Biobuf bout;
int cflag;
int count;
int hflag;

uchar buf[2][8192];
void
grep(DFA *d, int fd)
{
	uchar *match, *bol, *t, *e, *e0;
	DState *q, *nq, *start;
	int c, n, which;
	Error err;
	
	if(waserror(&err))
		return;

	which = 0;
	t = (uchar*)buf[0];
	n = read(fd, buf[0], sizeof buf[0]);
	if(n < 0){
		fprint(2, "read error\n");
		return;
	}
	if(n == 0)
		return;
	e = buf[0]+n;
	e0 = buf[1];
	q = d->start;
	start = walkstate(&err, d, q, Bol);
	q = start;
	match = nil;
	bol = buf[which];
more:
	for(; t<e; t++){
		c = *t;
		if(c == '\n'){
			if((nq = q->next[Eol]) == nil){
				if(q->id == -1)
					nq = q;
				else
					nq = walkstate(&err, d, q, Eol);
				q->next[Eol] = nq;
			}
			q = nq;
			if(q->match)
				match = (char*)t+1;
			if(match){
				if(!cflag){
					if(buf[which] <= bol && bol < buf[which]+sizeof buf[which])
						Bwrite(&bout, bol, (t+1)-bol);
					else{
						Bwrite(&bout, bol, e0-bol);
						Bwrite(&bout, buf[which], (t+1)-buf[which]);
					}
				}
				count++;
				match = nil;
			}
			bol = t+1;
			nq = start;
		}else{
			if((nq = q->next[c]) == nil){
				if(q->id == -1)
					nq = q;
				else
					nq = walkstate(&err, d, q, c);
				q->next[c] = nq;
			}
		}
		q = nq;
	//	if(q->match)
	//		match = (char*)t+1;
	}
	which = 1-which;
	n = read(fd, buf[which], sizeof buf[which]);
	if(n < 0){
		fprint(2, "read error\n");
		return;
	}
	if(n == 0)
		return;
	t = buf[which];
	e0 = e;
	e = buf[which]+n;
	goto more;
}

void
usage(void)
{
	fprint(2, "usage: rgrep [-ch] regexp [file...]\n");
	exits("usage");
}

void
grep1(char *file, DFA *d, int fd)
{
	count = 0;
	grep(d, fd);
	if(cflag){
		if(hflag)
			print("%d\n", count);
		else
			print("%s:%d\n", file, count);
	}
	Bflush(&bout);
}

void
main(int argc, char **argv)
{
	int i, fd;
	DFA d;
	DState *hash[1];
	NFA *m;
	Error e;
	
	ARGBEGIN{
	default:
		usage();
	case 'c':
		cflag++;
		break;
	case 'h':
		hflag++;
		break;
	}ARGEND
	
	if(argc < 1)
		usage();
	if(argc == 1)
		hflag++;

	m = parseregexp(smprint(".*%s.*", argv[0]), RegexpNormal);
	if(m == nil)
		sysfatal("bad regexp");
	
	memset(&d, 0, sizeof d);
	d.nfa = m;
	d.hash = hash;
	memset(hash, 0, sizeof hash);
	d.nhash = 1;
	if(waserror(&e))
		exits("error");
	dfainit(&e, &d);
	Binit(&bout, 1, OWRITE);
	for(i=1; i<argc; i++){
		if((fd = open(argv[i], OREAD)) < 0){
			fprint(2, "open %s: %r\n", argv[i]);
			continue;
		}
		grep1(argv[i], &d, fd);
		close(fd);
	}
	if(argc == 1)
		grep(&d, 0);
	fprint(2, "%d states\n", d.nstate);
	exits(0);
}
		
#endif
