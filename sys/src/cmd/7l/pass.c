#include	"l.h"

void
dodata(void)
{
	int i, t, size;
	Sym *s;
	Prog *p, *p1;
	long orig, orig1, v;
	vlong vv;

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
		if (v >= 8)
			orig = rnd(orig, 8);
		s->value = orig;
		orig += v;
		s->type = SDATA1;
	}
	orig = rnd(orig, 8);
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
		if (v >= 8)
			orig = rnd(orig, 8);
		s->value = orig;
		orig += v;
		s->type = SDATA1;
	}

	orig = rnd(orig, 8);
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
		if (v >= 8)
			orig = rnd(orig, 8);
		s->value = orig;
		orig += v;
	}
	orig = rnd(orig, 8);
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
		if(p->as != AMOVQ)
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
			size = 4;
			if(v >= 0 && v <= 0xffff)
				continue;
			if(!strcmp(s->name, "setSB"))
				continue;
			/* size should be 19 max */
			if(strlen(s->name) >= 10)	/* has loader address */ 
				sprint(literal, "$%p.%llux", s, p->from.offset);
			else
				sprint(literal, "$%s.%d.%llux", s->name, s->version, p->from.offset);
		} else {
			if(p->from.name != D_NONE)
				continue;
			if(p->from.reg != NREG)
				continue;
			vv = p->from.offset;
			if(vv >= -0x8000LL && vv <= 0x7fff)
				continue;
			if(!(vv & 0xffff))
				continue;
			size = 8;
			if (vv <= 0x7FFFFFFFLL && vv >= -0x80000000LL)
				size = 4;
			/* size should be 17 max */
			sprint(literal, "$%llux", vv);
		}
		s = lookup(literal, 0);
		if(s->type == 0) {
			s->type = SDATA;
			orig = rnd(orig, size);
			s->value = orig1+orig;
			orig += size;
			p1 = prg();
			p1->line = p->line;
			p1->as = ADATA;
			p1->from.type = D_OREG;
			p1->from.sym = s;
			p1->from.name = D_EXTERN;
			p1->reg = size;
			p1->to = p->from;
			p1->link = datap;
			datap = p1;
			if(debug['C'])
				Bprint(&bso, "literal %P for %P\n", p1, p);
		}
		if(s->type != SDATA)
			diag("literal not data: %s", s->name);
		if (size == 4)
			p->as = AMOVL;
		p->from.type = D_OREG;
		p->from.sym = s;
		p->from.name = D_EXTERN;
		p->from.offset = 0;
		continue;
	}
	orig = rnd(orig, 8);

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
	if (debug['v'] || debug['z'])
		Bprint(&bso, "datsize = %lux, bsssize = %lux\n", datsize, bsssize);
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
	int a, i;

loop:
	if(p == P)
		return;
	a = p->as;
	if(a == ATEXT)
		curtext = p;
	if(a == AJMP) {
		q = p->cond;
		if(q != P) {
			p->mark = FOLL;
			p = q;
			if(!(p->mark & FOLL))
				goto loop;
		}
	}
	if(p->mark & FOLL) {
		for(i=0,q=p; i<4; i++,q=q->link) {
			if(q == lastp)
				break;
			a = q->as;
			if(a == ANOP) {
				i--;
				continue;
			}
			if(a == AJMP || a == ARET || a == AREI)
				goto copy;
			if(!q->cond || (q->cond->mark&FOLL))
				continue;
			if(a != ABEQ && a != ABNE)
				continue;
		copy:
			for(;;) {
				r = prg();
				*r = *p;
				if(!(r->mark&FOLL))
					print("cant happen 1\n");
				r->mark = FOLL;
				if(p != q) {
					p = p->link;
					lastp->link = r;
					lastp = r;
					continue;
				}
				lastp->link = r;
				lastp = r;
				if(a == AJMP || a == ARET || a == AREI)
					return;
				r->as = ABNE;
				if(a == ABNE)
					r->as = ABEQ;
				r->cond = p->link;
				r->link = p->cond;
				if(!(r->link->mark&FOLL))
					xfol(r->link);
				if(!(r->cond->mark&FOLL))
					print("cant happen 2\n");
				return;
			}
		}
		a = AJMP;
		q = prg();
		q->as = a;
		q->line = p->line;
		q->to.type = D_BRANCH;
		q->to.offset = p->pc;
		q->cond = p;
		p = q;
	}
	p->mark = FOLL;
	lastp->link = p;
	lastp = p;
	if(a == AJMP || a == ARET || a == AREI)
		return;
	if(p->cond != P)
	if(a != AJSR && p->link != P) {
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
		if((a == AJSR || a == AJMP || a == ARET) &&
		   p->to.type != D_BRANCH && p->to.sym != S) {
			s = p->to.sym;
			if(s->type != STEXT) {
				diag("undefined: %s\n%P", s->name, p);
				s->type = STEXT;
				s->value = vexit;
			}
			p->to.offset = s->value;
			p->to.type = D_BRANCH;
		}
		if(p->to.type != D_BRANCH)
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
		if(p->cond != P) {
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
		if(p->as != AJMP)
			return p;
		q = p->cond;
		if(q <= p) {
			c++;
			if(q == p || c > 50)
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
