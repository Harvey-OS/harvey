#include	"l.h"

void
dodata(void)
{
	int i, t;
	Sym *s;
	Prog *p, *p1;
	long orig, orig1, v;

	if(debug['v'])
		Bprint(&bso, "%5.2f dodata\n", cputime());
	Bflush(&bso);
	for(p = datap; p != P; p = p->link) {
		s = p->from.sym;
		if(p->as == ADYNT || p->as == AINIT)
			s->value = dtype;
		if(s->type == SBSS)
			s->type = SDATA;
		if(s->type != SDATA)
			diag("initialize non-data (%d): %s\n%P",
				s->type, s->name, p);
		v = p->from.offset + p->reg;
		if(v > s->value)
			diag("initialize bounds (%ld): %s\n%P",
				s->value, s->name, p);
	}

	/*
	 * pass 1
	 *	assign 'small' variables to data segment
	 *	(rational is that data segment is more easily
	 *	 addressed through offset on REGSB)
	 */
	orig = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA && t != SBSS)
			continue;
		v = s->value;
		if(v == 0) {
			diag("%s: no size", s->name);
			v = 1;
		}
		while(v & 3)
			v++;
		s->value = v;
		if(v > MINSIZ)
			continue;
		if(v >= 8)
			while(orig & 7)
				orig++;
		s->value = orig;
		orig += v;
		s->type = SDATA1;
	}
	orig1 = orig;

	/*
	 * pass 2
	 *	assign 'data' variables to data segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA) {
			if(t == SDATA1)
				s->type = SDATA;
			continue;
		}
		v = s->value;
		if(v >= 8)
			while(orig & 7)
				orig++;
		s->value = orig;
		orig += v;
		s->type = SDATA1;
	}

	while(orig & 7)
		orig++;
	datsize = orig;

	/*
	 * pass 3
	 *	everything else to bss segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SBSS)
			continue;
		v = s->value;
		if(v >= 8)
			while(orig & 7)
				orig++;
		s->value = orig;
		orig += v;
	}
	while(orig & 7)
		orig++;
	bsssize = orig-datsize;

	/*
	 * pass 4
	 *	add literals to all large values.
	 *	at this time:
	 *		small data is allocated DATA
	 *		large data is allocated DATA1
	 *		large bss is allocated BSS
	 *	the new literals are loaded between
	 *	small data and large data.
	 */
	orig = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->as != AMOVW)
			continue;
		if(p->from.type != D_CONST)
			continue;
		if(s = p->from.sym) {
			t = s->type;
			if(t != SDATA && t != SDATA1 && t != SBSS)
				continue;
			t = p->from.name;
			if(t != D_EXTERN && t != D_STATIC)
				continue;
			v = s->value + p->from.offset;
			if(v >= 0 && v <= 0xffff)
				continue;
			if(!strcmp(s->name, "setSB"))
				continue;
			/* size should be 19 max */
			if(strlen(s->name) >= 10)	/* has loader address */ 
				sprint(literal, "$%p.%lux", s, p->from.offset);
			else
				sprint(literal, "$%s.%d.%lux", s->name, s->version, p->from.offset);
		} else {
			if(p->from.name != D_NONE)
				continue;
			if(p->from.reg != NREG)
				continue;
			v = p->from.offset;
			if(v >= -0x7fff-1 && v <= 0x7fff)
				continue;
			if(!(v & 0xffff))
				continue;
			if(v)
				continue;	/* quicker to build it than load it */
			/* size should be 9 max */
			sprint(literal, "$%lux", v);
		}
		s = lookup(literal, 0);
		if(s->type == 0) {
			s->type = SDATA;
			s->value = orig1+orig;
			orig += 4;
			p1 = prg();
			p1->as = ADATA;
			p1->line = p->line;
			p1->from.type = D_OREG;
			p1->from.sym = s;
			p1->from.name = D_EXTERN;
			p1->reg = 4;
			p1->to = p->from;
			p1->link = datap;
			datap = p1;
		}
		if(s->type != SDATA)
			diag("literal not data: %s", s->name);
		p->from.type = D_OREG;
		p->from.sym = s;
		p->from.name = D_EXTERN;
		p->from.offset = 0;
		continue;
	}
	while(orig & 7)
		orig++;
	/*
	 * pass 5
	 *	re-adjust offsets
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t == SBSS) {
			s->value += orig;
			continue;
		}
		if(t == SDATA1) {
			s->type = SDATA;
			s->value += orig;
			continue;
		}
	}
	datsize += orig;
	xdefine("setSB", SDATA, 0L+BIG);
	xdefine("bdata", SDATA, 0L);
	xdefine("edata", SDATA, datsize);
	xdefine("end", SBSS, datsize+bsssize);
	xdefine("etext", STEXT, 0L);
}

void
undef(void)
{
	int i;
	Sym *s;

	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link)
		if(s->type == SXREF)
			diag("%s: not defined", s->name);
}

int
relinv(int a)
{

	switch(a) {
	case ABEQ:	return ABNE;
	case ABNE:	return ABEQ;

	case ABGE:	return ABLT;
	case ABLT:	return ABGE;

	case ABGT:	return ABLE;
	case ABLE:	return ABGT;

	case ABVC:	return ABVS;
	case ABVS:	return ABVC;
	}
	return 0;
}

void
follow(void)
{

	if(debug['v'])
		Bprint(&bso, "%5.2f follow\n", cputime());
	Bflush(&bso);

	firstp = prg();
	lastp = firstp;

	xfol(textp);

	firstp = firstp->link;
	lastp->link = P;
}

void
xfol(Prog *p)
{
	Prog *q, *r;
	int a, b, i;

loop:
	if(p == P)
		return;
	a = p->as;
	if(a == ATEXT)
		curtext = p;
	if(a == ABR) {
		q = p->cond;
		if((p->mark&NOSCHED) || q && (q->mark&NOSCHED)){
			p->mark |= FOLL;
			lastp->link = p;
			lastp = p;
			p = p->link;
			xfol(p);
			p = q;
			if(p && !(p->mark & FOLL))
				goto loop;
			return;
		}
		if(q != P) {
			p->mark |= FOLL;
			p = q;
			if(!(p->mark & FOLL))
				goto loop;
		}
	}
	if(p->mark & FOLL) {
		for(i=0,q=p; i<4; i++,q=q->link) {
			if(q == lastp || (q->mark&NOSCHED))
				break;
			b = 0;		/* set */
			a = q->as;
			if(a == ANOP) {
				i--;
				continue;
			}
			if(a == ABR || a == ARETURN || a == ARFI || a == ARFCI)
				goto copy;
			if(!q->cond || (q->cond->mark&FOLL))
				continue;
			b = relinv(a);
			if(!b)
				continue;
		copy:
			for(;;) {
				r = prg();
				*r = *p;
				if(!(r->mark&FOLL))
					print("cant happen 1\n");
				r->mark |= FOLL;
				if(p != q) {
					p = p->link;
					lastp->link = r;
					lastp = r;
					continue;
				}
				lastp->link = r;
				lastp = r;
				if(a == ABR || a == ARETURN || a == ARFI || a == ARFCI)
					return;
				r->as = b;
				r->cond = p->link;
				r->link = p->cond;
				if(!(r->link->mark&FOLL))
					xfol(r->link);
				if(!(r->cond->mark&FOLL))
					print("cant happen 2\n");
				return;
			}
		}

		a = ABR;
		q = prg();
		q->as = a;
		q->line = p->line;
		q->to.type = D_BRANCH;
		q->to.offset = p->pc;
		q->cond = p;
		p = q;
	}
	p->mark |= FOLL;
	lastp->link = p;
	lastp = p;
	if(a == ABR || a == ARETURN || a == ARFI || a == ARFCI){
		if(p->mark & NOSCHED){
			p = p->link;
			goto loop;
		}
		return;
	}
	if(p->cond != P)
	if(a != ABL && p->link != P) {
		xfol(p->link);
		p = p->cond;
		if(p == P || (p->mark&FOLL))
			return;
		goto loop;
	}
	p = p->link;
	goto loop;
}

void
patch(void)
{
	long c, vexit;
	Prog *p, *q;
	Sym *s;
	int a;

	if(debug['v'])
		Bprint(&bso, "%5.2f patch\n", cputime());
	Bflush(&bso);
	mkfwd();
	s = lookup("exit", 0);
	vexit = s->value;
	for(p = firstp; p != P; p = p->link) {
		a = p->as;
		if(a == ATEXT)
			curtext = p;
		if((a == ABL || a == ARETURN) && p->to.sym != S) {
			s = p->to.sym;
			if(s->type != STEXT && s->type != SUNDEF) {
				diag("undefined: %s\n%P", s->name, p);
				s->type = STEXT;
				s->value = vexit;
			}
			if(s->type == SUNDEF){
				p->to.offset = 0;
				p->cond = UP;
			}
			else
				p->to.offset = s->value;
			p->to.type = D_BRANCH;
		}
		if(p->to.type != D_BRANCH || p->cond == UP)
			continue;
		c = p->to.offset;
		for(q = firstp; q != P;) {
			if(q->forwd != P)
			if(c >= q->forwd->pc) {
				q = q->forwd;
				continue;
			}
			if(c == q->pc)
				break;
			q = q->link;
		}
		if(q == P) {
			diag("branch out of range %ld\n%P", c, p);
			p->to.type = D_NONE;
		}
		p->cond = q;
	}

	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		p->mark = 0;	/* initialization for follow */
		if(p->cond != P && p->cond != UP) {
			p->cond = brloop(p->cond);
			if(p->cond != P)
			if(p->to.type == D_BRANCH)
				p->to.offset = p->cond->pc;
		}
	}
}

#define	LOG	5
void
mkfwd(void)
{
	Prog *p;
	long dwn[LOG], cnt[LOG], i;
	Prog *lst[LOG];

	for(i=0; i<LOG; i++) {
		if(i == 0)
			cnt[i] = 1; else
			cnt[i] = LOG * cnt[i-1];
		dwn[i] = 1;
		lst[i] = P;
	}
	i = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		i--;
		if(i < 0)
			i = LOG-1;
		p->forwd = P;
		dwn[i]--;
		if(dwn[i] <= 0) {
			dwn[i] = cnt[i];
			if(lst[i] != P)
				lst[i]->forwd = p;
			lst[i] = p;
		}
	}
}

Prog*
brloop(Prog *p)
{
	Prog *q;
	int c;

	for(c=0; p!=P;) {
		if(p->as != ABR || (p->mark&NOSCHED))
			return p;
		q = p->cond;
		if(q <= p) {
			c++;
			if(q == p || c > 5000)
				break;
		}
		p = q;
	}
	return P;
}

long
atolwhex(char *s)
{
	long n;
	int f;

	n = 0;
	f = 0;
	while(*s == ' ' || *s == '\t')
		s++;
	if(*s == '-' || *s == '+') {
		if(*s++ == '-')
			f = 1;
		while(*s == ' ' || *s == '\t')
			s++;
	}
	if(s[0]=='0' && s[1]){
		if(s[1]=='x' || s[1]=='X'){
			s += 2;
			for(;;){
				if(*s >= '0' && *s <= '9')
					n = n*16 + *s++ - '0';
				else if(*s >= 'a' && *s <= 'f')
					n = n*16 + *s++ - 'a' + 10;
				else if(*s >= 'A' && *s <= 'F')
					n = n*16 + *s++ - 'A' + 10;
				else
					break;
			}
		} else
			while(*s >= '0' && *s <= '7')
				n = n*8 + *s++ - '0';
	} else
		while(*s >= '0' && *s <= '9')
			n = n*10 + *s++ - '0';
	if(f)
		n = -n;
	return n;
}

long
rnd(long v, long r)
{
	long c;

	if(r <= 0)
		return v;
	v += r - 1;
	c = v % r;
	if(c < 0)
		c += r;
	v -= c;
	return v;
}

void
import(void)
{
	int i;
	Sym *s;

	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->sig != 0 && s->type == SXREF && (nimports == 0 || s->subtype == SIMPORT)){
				undefsym(s);
				Bprint(&bso, "IMPORT: %s sig=%lux v=%ld\n", s->name, s->sig, s->value);
			}
}

void
ckoff(Sym *s, long v)
{
	if(v < 0 || v >= 1<<Roffset)
		diag("relocation offset %ld for %s out of range", v, s->name);
}

static Prog*
newdata(Sym *s, int o, int w, int t)
{
	Prog *p;

	p = prg();
	p->link = datap;
	datap = p;
	p->as = ADATA;
	p->reg = w;
	p->from.type = D_OREG;
	p->from.name = t;
	p->from.sym = s;
	p->from.offset = o;
	p->to.type = D_CONST;
	p->to.name = D_NONE;
	return p;
}

void
export(void)
{
	int i, j, n, off, nb, sv, ne;
	Sym *s, *et, *str, **esyms;
	Prog *p;
	char buf[NSNAME], *t;

	n = 0;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->sig != 0 && s->type != SXREF && s->type != SUNDEF && (nexports == 0 || s->subtype == SEXPORT))
				n++;
	esyms = malloc(n*sizeof(Sym*));
	ne = n;
	n = 0;
	for(i = 0; i < NHASH; i++)
		for(s = hash[i]; s != S; s = s->link)
			if(s->sig != 0 && s->type != SXREF && s->type != SUNDEF && (nexports == 0 || s->subtype == SEXPORT))
				esyms[n++] = s;
	for(i = 0; i < ne-1; i++)
		for(j = i+1; j < ne; j++)
			if(strcmp(esyms[i]->name, esyms[j]->name) > 0){
				s = esyms[i];
				esyms[i] = esyms[j];
				esyms[j] = s;
			}

	nb = 0;
	off = 0;
	et = lookup(EXPTAB, 0);
	if(et->type != 0 && et->type != SXREF)
		diag("%s already defined", EXPTAB);
	et->type = SDATA;
	str = lookup(".string", 0);
	if(str->type == 0)
		str->type = SDATA;
	sv = str->value;
	for(i = 0; i < ne; i++){
		s = esyms[i];
		Bprint(&bso, "EXPORT: %s sig=%lux t=%d\n", s->name, s->sig, s->type);

		/* signature */
		p = newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
		p->to.offset = s->sig;

		/* address */
		p = newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
		p->to.name = D_EXTERN;
		p->to.sym = s;

		/* string */
		t = s->name;
		n = strlen(t)+1;
		for(;;){
			buf[nb++] = *t;
			sv++;
			if(nb >= NSNAME){
				p = newdata(str, sv-NSNAME, NSNAME, D_STATIC);
				p->to.type = D_SCONST;
				memmove(p->to.sval, buf, NSNAME);
				nb = 0;
			}
			if(*t++ == 0)
				break;
		}

		/* name */
		p = newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
		p->to.name = D_STATIC;
		p->to.sym = str;
		p->to.offset = sv-n;
	}

	if(nb > 0){
		p = newdata(str, sv-nb, nb, D_STATIC);
		p->to.type = D_SCONST;
		memmove(p->to.sval, buf, nb);
	}

	for(i = 0; i < 3; i++){
		newdata(et, off, sizeof(long), D_EXTERN);
		off += sizeof(long);
	}
	et->value = off;
	if(sv == 0)
		sv = 1;
	str->value = sv;
	exports = ne;
	free(esyms);
}
