#include	"l.h"

void
dodata(void)
{
	int i;
	Sym *s;
	Prog *p;
	long t, u;

	if(debug['v'])
		Bprint(&bso, "%5.2f dodata\n", cputime());
	Bflush(&bso);
	for(p = datap; p != P; p = p->link) {
		s = p->from.sym;
		if(s->type == SBSS)
			s->type = SDATA;
		if(s->type != SDATA)
			diag("initialize non-data (%d): %s\n%P",
				s->type, s->name, p);
		t = p->from.offset + p->from.displace;
		if(t > s->value)
			diag("initialize bounds (%ld): %s\n%P",
				s->value, s->name, p);
	}

	/* allocate small guys */
	datsize = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SDATA)
		if(s->type != SBSS)
			continue;
		t = s->value;
		if(t == 0) {
			diag("%s: no size", s->name);
			t = 1;
		}
		t = rnd(t, 4);;
		s->value = t;
		if(t > MINSIZ)
			continue;
		s->value = datsize;
		datsize += t;
		s->type = SDATA1;
	}

	/* allocate the rest of the data */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SDATA) {
			if(s->type == SDATA1)
				s->type = SDATA;
			continue;
		}
		t = s->value;
		s->value = datsize;
		datsize += t;
	}

	if(debug['j']) {
		/*
		 * pad data with bss that fits up to next
		 * 8k boundary, then push data to 8k
		 */
		u = rnd(datsize, 8192);
		u -= datsize;
		for(i=0; i<NHASH; i++)
		for(s = hash[i]; s != S; s = s->link) {
			if(s->type != SBSS)
				continue;
			t = s->value;
			if(t > u)
				continue;
			u -= t;
			s->value = datsize;
			s->type = SDATA;
			datsize += t;
		}
		datsize += u;
	}

	/* now the bss */
	bsssize = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SBSS)
			continue;
		t = s->value;
		s->value = bsssize + datsize;
		bsssize += t;
	}
	xdefine("bdata", SDATA, 0L);
	xdefine("edata", SDATA, datsize);
	xdefine("end", SBSS, datsize+bsssize);
}

Prog*
brchain(Prog *p)
{
	int i;

	for(i=0; i<20; i++) {
		if(p == P || p->as != ABRA)
			return p;
		p = p->pcond;
	}
	return P;
}

void
follow(void)
{
	Prog *p;
	long o;

	if(debug['v'])
		Bprint(&bso, "%5.2f follow\n", cputime());
	Bflush(&bso);
	firstp = prg();
	lastp = firstp;
	xfol(textp);
	lastp->link = P;
	firstp = firstp->link;
	o = 0; /* set */
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;

		p->stkoff = -1;	/* initialization for stkoff */
		if(p->as == ATEXT) {
			p->stkoff = 0;
			o = p->to.offset;
			continue;
		}
		if(p->as == AADJSP && p->from.offset == 0) {
			p->stkoff = o;
			continue;
		}
	}
}

void
xfol(Prog *p)
{
	Prog *q;
	int i;
	enum as a;

loop:
	if(p == P)
		return;
	if(p->as == ATEXT)
		curtext = p;
	if(p->as == ABRA)
	if((q = p->pcond) != P) {
		p->mark = 1;
		p = q;
		if(p->mark == 0)
			goto loop;
	}
	if(p->mark) {
		/* copy up to 4 instructions to avoid branch */
		for(i=0,q=p; i<4; i++,q=q->link) {
			if(q == P)
				break;
			if(q == lastp)
				break;
			a = q->as;
			if(a == ANOP) {
				i--;
				continue;
			}
			if(a == ABRA || a == ARTS || a == ARTE)
				break;
			if(q->pcond == P || q->pcond->mark)
				continue;
			if(a == ABSR || a == ADBF)
				continue;
			for(;;) {
				if(p->as == ANOP) {
					p = p->link;
					continue;
				}
				q = copyp(p);
				p = p->link;
				q->mark = 1;
				lastp->link = q;
				lastp = q;
				if(q->as != a || q->pcond == P || q->pcond->mark)
					continue;
				q->as = relinv(q->as);
				p = q->pcond;
				q->pcond = q->link;
				q->link = p;
				xfol(q->link);
				p = q->link;
				if(p->mark)
					return;
				goto loop;
			}
		} /* */
		q = prg();
		q->as = ABRA;
		q->line = p->line;
		q->to.type = D_BRANCH;
		q->to.offset = p->pc;
		q->pcond = p;
		p = q;
	}
	p->mark = 1;
	lastp->link = p;
	lastp = p;
	a = p->as;
	if(a == ARTS || a == ABRA || a == ARTE)
		return;
	if(p->pcond != P)
	if(a != ABSR) {
		q = brchain(p->link);
		if(q != P && q->mark)
		if(a != ADBF) {
			p->as = relinv(a);
			p->link = p->pcond;
			p->pcond = q;
		}
		xfol(p->link);
		q = brchain(p->pcond);
		if(q->mark) {
			p->pcond = q;
			return;
		}
		p = q;
		goto loop;
	}
	p = p->link;
	goto loop;
}

int
relinv(int a)
{

	switch(a) {
	case ABEQ:	return ABNE;
	case ABNE:	return ABEQ;
	case ABLE:	return ABGT;
	case ABLS:	return ABHI;
	case ABLT:	return ABGE;
	case ABMI:	return ABPL;
	case ABGE:	return ABLT;
	case ABPL:	return ABMI;
	case ABGT:	return ABLE;
	case ABHI:	return ABLS;
	case ABCS:	return ABCC;
	case ABCC:	return ABCS;
	case AFBEQ:	return AFBNE;
	case AFBF:	return AFBT;
	case AFBGE:	return AFBLT;
	case AFBGT:	return AFBLE;
	case AFBLE:	return AFBGT;
	case AFBLT:	return AFBGE;
	case AFBNE:	return AFBEQ;
	case AFBT:	return AFBF;
	}
	diag("unknown relation: %s in %s", anames[a], TNAME);
	return a;
}

void
patch(void)
{
	long c;
	Prog *p, *q;
	Sym *s;
	long vexit;

	if(debug['v'])
		Bprint(&bso, "%5.2f mkfwd\n", cputime());
	Bflush(&bso);
	mkfwd();
	if(debug['v'])
		Bprint(&bso, "%5.2f patch\n", cputime());
	Bflush(&bso);
	s = lookup("exit", 0);
	vexit = s->value;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		if((p->as == ABSR || p->as == ARTS) && p->to.sym != S) {
			s = p->to.sym;
			if(s->type != STEXT) {
				diag("undefined: %s in %s", s->name, TNAME);
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
			diag("branch out of range in %s\n%P", TNAME, p);
			p->to.type = D_NONE;
		}
		p->pcond = q;
	}

	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		p->mark = 0;	/* initialization for follow */
		if(p->pcond != P) {
			p->pcond = brloop(p->pcond);
			if(p->pcond != P)
			if(p->to.type == D_BRANCH)
				p->to.offset = p->pcond->pc;
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
	int c;
	Prog *q;

	c = 0;
	for(q = p; q != P; q = q->pcond) {
		if(q->as != ABRA)
			break;
		c++;
		if(c >= 5000)
			return P;
	}
	return q;
}

void
dostkoff(void)
{
	Prog *p, *q, *qq;
	long s, t;
	int a;
	Optab *o;

	if(debug['v'])
		Bprint(&bso, "%5.2f stkoff\n", cputime());
	Bflush(&bso);
	s = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			s = p->to.offset;
			if(s == 0)
				continue;
			p = nprg(p);
			p->as = AADJSP;
			p->from.type = D_CONST;
			p->from.offset = s;
			p->stkoff = 0;
			continue;
		}
		t = 0;
		for(q = p; q != P; q = q->pcond) {
			if(q->as == ATEXT)
				break;
			if(q->stkoff >= 0)
				if(q->stkoff != s)
					diag("stack offset %ld is %ld sb %ld in %s\n%P",
						q->pc, q->stkoff, s, q, TNAME, p);
			q->stkoff = s;
			if(t++ > 100) {
				diag("loop in stack offset 1: %P", p);
				break;
			}
		}
		o = &optab[p->as];
		if(p->to.type == D_TOS)
			s -= o->dstsp;
		if(p->from.type == D_TOS)
			s -= o->srcsp;
		if(p->as == AADJSP)
			s += p->from.offset;
		if(p->as == APEA)
			s += 4;
		t = 0;
		for(q = p->link; q != P; q = q->pcond) {
			if(q->as == ATEXT) {
				q = P;
				break;
			}
			if(q->stkoff >= 0)
				break;
			if(t++ > 100) {
				diag("loop in stack offset 2: %P", p);
				break;
			}
		}
		if(q == P || q->stkoff == s)
			continue;
		if(p->as == ABRA || p->as == ARTS || p->as == ARTE) {
			s = q->stkoff;
			continue;
		}
		t = q->stkoff - s;
		s = q->stkoff;
		p = nprg(p);
		p->as = AADJSP;
		p->stkoff = s - t;
		p->from.type = D_CONST;
		p->from.offset = t;
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f rewrite\n", cputime());
	Bflush(&bso);
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		a = p->from.type & D_MASK;
		if(a == D_AUTO)
			p->from.offset += p->stkoff;
		if(a == D_PARAM)
			p->from.offset += p->stkoff + 4;
		a = p->to.type & D_MASK;
		if(a == D_AUTO)
			p->to.offset += p->stkoff;
		if(a == D_PARAM)
			p->to.offset += p->stkoff + 4;
		switch(p->as) {
		default:
			continue;

		case AMOVW:
			if(p->from.type != D_CCR)
				continue;
			a = p->to.type;
			if((a < D_R0 || a > D_R0+7) && a != D_TOS)
				diag("bad dest for MOVCC %P", p);
			p->as = ALEA;
			p->from.type = I_INDIR|(D_A0+7);
			p->from.offset = -2;
			p->to.type = D_A0+7;

			p = nprg(p);
			p->as = ABSR;
			p->to.type = D_BRANCH;
			p->pcond = prog_ccr;
			p->to.sym = prog_ccr->from.sym;

			if(a != D_TOS) {
				p = nprg(p);
				p->as = AMOVW;
				p->from.type = D_TOS;
				p->to.type = a;
			}
			continue;

		case AEXTBL:
			a = p->to.type;
			if(a < D_R0 || a > D_R0+7)
				diag("bad dest for EXTB");
			p->as = AEXTBW;

			p = nprg(p);
			p->as = AEXTWL;
			p->to.type = a;
			continue;

		case AMULSL:
		case AMULUL:
			qq = prog_mull;
			goto mdcom;
		case ADIVSL:
			qq = prog_divsl;
			goto mdcom;
		case ADIVUL:
			qq = prog_divul;
		mdcom:
			if(debug['m'])
				continue;
			a = p->to.type;
			if(a < D_R0 || a > D_R0+7)
				diag("bad dest for mul/div");
			p->as = AMOVL;
			p->to.type = D_TOS;

			p = nprg(p);
			p->as = AMOVL;
			p->from.type = a;
			p->to.type = D_TOS;

			p = nprg(p);
			p->as = ABSR;
			p->to.type = D_BRANCH;
			p->pcond = qq;
			p->to.sym = qq->from.sym;

			p = nprg(p);
			p->as = AMOVL;
			p->from.type = D_TOS;
			p->to.type = a;

			p = nprg(p);
			p->as = AMOVL;
			p->from.type = D_TOS;
			p->to.type = a+1;
			if(qq == prog_mull)
				p->to.type = a;
			continue;

		case ARTS:
			break;
		}
		if(p->stkoff == 0)
			continue;

		p->as = AADJSP;
		p->from.type = D_CONST;
		p->from.offset = -p->stkoff;

		p = nprg(p);
		p->as = ARTS;
		p->stkoff = 0;
	}
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
initmuldiv1(void)
{
	lookup("_mull", 0)->type = SXREF;
	lookup("_divsl", 0)->type = SXREF;
	lookup("_divul", 0)->type = SXREF;
	lookup("_ccr", 0)->type = SXREF;
}

void
initmuldiv2(void)
{
	Sym *s1, *s2, *s3, *s4;
	Prog *p;

	if(prog_mull != P)
		return;
	s1 = lookup("_mull", 0);
	s2 = lookup("_divsl", 0);
	s3 = lookup("_divul", 0);
	s4 = lookup("_ccr", 0);
	for(p = firstp; p != P; p = p->link)
		if(p->as == ATEXT) {
			if(p->from.sym == s1)
				prog_mull = p;
			if(p->from.sym == s2)
				prog_divsl = p;
			if(p->from.sym == s3)
				prog_divul = p;
			if(p->from.sym == s4)
				prog_ccr = p;
		}
	if(prog_mull == P) {
		diag("undefined: %s", s1->name);
		prog_mull = curtext;
	}
	if(prog_divsl == P) {
		diag("undefined: %s", s2->name);
		prog_divsl = curtext;
	}
	if(prog_divul == P) {
		diag("undefined: %s", s3->name);
		prog_divul = curtext;
	}
	if(prog_ccr == P) {
		diag("undefined: %s", s4->name);
		prog_ccr = curtext;
	}
}
