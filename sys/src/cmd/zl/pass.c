#include	"l.h"

void
dodata(void)
{
	int i;
	Sym *s;
	Prog *p;
	long cneg, cpos, t;

	if(debug['v'])
		Bprint(&bso, "%5.2f dodata\n", cputime());
	Bflush(&bso);
	for(p = datap; p != P; p = p->link) {
		s = p->from.sym;
		if(s->type == SBSS)
			s->type = SDATA;
		if(s->type != SDATA)
			diag("initialize non-data (%d): %s\n",
				s->type, s->name);
		if(p->to.type == D_SCONST)
			i = p->to.width;
		else
			i = wsize[p->from.width];
		t = p->from.offset + i;
		if(t > s->value)
			diag("initialize bounds (%ld): %s\n%P\n",
				s->value, s->name, p);
	}
	cneg = 100000000L;
	cpos = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SDATA)
		if(s->type != SBSS)
			continue;
		t = s->value;
		if(t == 0) {
			Bprint(&bso, "%s: no size\n", s->name);
			t = 1;
		}
		while(t & 3)
			t++;
		s->value = t;
		if(t > MINSIZ)
			continue;
		if(cneg+t < cpos) {
			cneg += t;
			s->value = -cneg;
		} else {
			s->value = cpos;
			cpos += t;
		}
		s->type = SDATA1;
	}
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SDATA) {
			if(s->type == SDATA1)
				s->type = SDATA;
			continue;
		}
		t = s->value;
		if(cneg+t < cpos) {
			cneg += t;
			s->value = -cneg;
		} else {
			s->value = cpos;
			cpos += t;
		}
	}
	cneg = 0;
	datsize = cpos+cneg;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SBSS)
			continue;
		t = s->value;
		s->value = cpos;
		cpos += t;
	}
	bsssize = cpos+cneg-datsize;

	xdefine("bdata", SBSS, 0L);
	xdefine("edata", SBSS, datsize);
	xdefine("end", SBSS, bsssize + datsize);
}

void
follow(void)
{
	Prog *p, *q;
	Sym *s;
	long c, v;

	if(debug['v'])
		Bprint(&bso, "%5.2f follow\n", cputime());
	Bflush(&bso);
	if(firstp == P)
		return;
	firstp = P;
	lastp = P;
	xfol(textp);
	if(firstp == P)
		return;
	lastp->link = P;
	for(p = firstp; p != P; p = p->link) {
		if(p->from.type == D_CPU || p->to.type == D_CPU) {
			q = prg();
			*q = *p;
			p->as = ACPU;
			p->from.type = D_NONE;
			p->to.type = D_NONE;
			p->link = q;
			p = q;
		}
		switch(p->as) {
		case ACALL:
		case ACALL1:
			c = (maxref(p->link) + 15) & ~15;
			if(c >= SBSIZE)
				c = SBSIZE;
			s = p->to.sym;
			if(p->to.type == D_BRANCH && s != S) {
				v = s->lstack + s->rstack;
				if(v + c <= SBSIZE)
					c = 0;
			}
			if(c == 0 || debug['C'])
				break;
			if((q = p->link) && q->as == ACATCH)
				break;
			q = prg();
			q->link = p->link;
			p->link = q;
			q->as = ACATCH;
			q->line = p->line;
			q->to.type = D_REG;
			q->to.offset = c;
			q->to.width = W_NONE;
			q->mark = p->mark;
			break;

		case ATEXT:
			autosize = p->to.offset;
			if(autosize > 0) {
				q = prg();
				q->link = p->link;
				p->link = q;
				q->as = AENTER;
				q->line = p->line;
				q->to.type = D_REG;
				q->to.offset = -autosize;
				q->to.width = W_NONE;
				q->mark = p->mark;
			}
			break;

		case ARETURN:
			if(p->to.type == D_NONE) {
				p->to.type = D_REG;
				p->to.offset = autosize;
				p->cond = P;
			}
			break;

		case AJMP:
			q = p->cond;
			if(q != P) {
				if(q->as == ARETURN) {
					p->as = ARETURN;
					p->to.type = D_REG;
					p->to.offset = autosize;
					p->cond = P;
					break;
				}
				if(q->as == AKRET) {
					p->as = AKRET;
					p->to.type = D_NONE;
					p->cond = P;
					break;
				}
			}
			break;
		}
		if(p->to.type != D_BRANCH)
			continue;
		if(p->as == ACALL)
			continue;
		q = p->link;
		if(q == P)
			continue;
		if(q->as != AJMP)
			continue;
		if(p->cond == P)
			continue;
		if(q->link == p->cond) {
			/* jmp[tf] .+2; jmp x ==> jmp[ft] x */
			p->as = relinv(p->as);
			p->to = q->to;
			p->cond = q->cond;
			p->link = q->link;
			continue;
		}
		if(p->cond->as == ARETURN) {
			/* jmp[tf] (return); jmp x ==> jmp[ft] x; return */
			p->as = relinv(p->as);
			p->to = q->to;
			p->cond = q->cond;

			q->as = ARETURN;
			q->to.type = D_REG;
			q->to.offset = autosize;
			q->cond = P;
			continue;
		}
	}
}

void
xfol(Prog *p)
{
	Prog *q;
	int a;

loop:
	if(p == P)
		return;
	if(p->mark) {
		q = prg();
		q->as = AJMP;
		q->line = p->line;
		q->to.type = D_BRANCH;
		q->to.offset = p->pc;
		q->cond = p;
		p = q;
	}
	p->mark = 1;
	a = p->as;
	if(a == AJMP)
	if(p->cond != P)
	if(p->cond->mark == 0) {
		p = p->cond;
		if(p == P || p->mark)
			return;
		goto loop;
	}
	if(firstp == P)
		firstp = p;
	else
		lastp->link = p;
	lastp = p;
	if(a == ARETURN || a == AJMP || a == AKRET)
		return;
	if(p->cond != P)
	if(a != ACALL) {
		xfol(p->link);
		p = p->cond;
		if(p == P || p->mark)
			return;
		goto loop;
	}
	p = p->link;
	goto loop;
}

int
relinv(int a)
{

	switch(a) {
	case AJMPF:	return AJMPT;
	case AJMPT:	return AJMPF;
	}
	Bprint(&bso, "unknown relation: %s", anames[a]);
	return a;
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
	if(firstp == P)
		return;
	mkfwd();
	s = lookup("exit", 0);
	vexit = s->value;
	for(p = firstp; p != P; p = p->link) {
		a = p->as;
		if(a == ATEXT)
			curtext = p;
		if(a == ACALL && p->to.sym != S) {
			s = p->to.sym;
			if(s->type != STEXT) {
				diag("undefined: %s\n%P\n", s->name, p);
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
			Bprint(&bso, "branch out of range\n%P\n", p);
			p->to.type = D_NONE;
		}
		p->cond = q;
	}

	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		p->mark = 0;	/* initialization for follow */
		p->catch = -1;
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
	int i;
	long dwn[LOG], cnt[LOG];
	Prog *lst[LOG];

	for(i=0; i<LOG; i++) {
		if(i == 0)
			cnt[i] = 1;
		else
			cnt[i] = LOG * cnt[i-1];
		dwn[i] = 1;
		lst[i] = P;
	}
	i = 0;
	for(p = firstp; p != P; p = p->link) {
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
	return p; /* vl has P */
}

int
maxref(Prog *q)
{
	int m;
	Prog *p;
	long c, d;

	if(q == P)
		return 0;
	if(q->catch >= 0)
		return q->catch;
	q->catch = 0;
	c = 0;
	p = q;

loop:
	if(p == P)
		goto out;
	switch(p->as) {

	case AJMP:
		d = maxref(p->cond);
		if(d > c)
			c = d;
		goto out;

	case AJMPF:
	case AJMPT:
		d = maxref(p->cond);
		if(d > c)
			c = d;
		d = maxref(p->link);
		if(d > c)
			c = d;
		goto out;

	case ACALL:
	case ACALL1:
	case ARETURN:
		goto out;
	}
	d = 0;
	m = p->from.type & ~D_INDIR;
	if(m == D_REG) {
		d = p->from.offset;
	} else
	if(m == D_AUTO) {
		d = autosize + 0 + p->from.offset;
	} else
	if(m == D_PARAM) {
		d = autosize + 4 + p->from.offset;
	}
	if(p->as == AMOVA)
		d = 0;
	if(d > c) {
		if(d > SBSIZE)
			d = SBSIZE;
		c = d;
	}
	d = 0;
	m = p->to.type & ~D_INDIR;
	if(m == D_REG) {
		d = p->to.offset;
	} else
	if(m == D_AUTO) {
		d = autosize + 0 + p->to.offset;
	} else
	if(m == D_PARAM) {
		d = autosize + 4 + p->to.offset;
	}
	if(d > c) {
		if(d > SBSIZE)
			d = SBSIZE;
		c = d;
	}
	p = p->link;
	goto loop;

out:
	q->catch = c;
	return c;
}

short	wsize[] =
{
	4,	/* default is .L */
	1,	/* B */
	1,	/* UB */
	2,	/* H */
	2,	/* UH */
	4,	/* W */
	4,	/* F */
	8,	/* D */
	12,	/* E */
	8,	/* S */
	4,	/* NONE */
};

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

void
undef(void)
{
	int i;
	Sym *s;

	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link)
		if(s->type == SXREF)
			diag("%s: not defined\n", s->name);
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
