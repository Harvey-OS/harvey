#include	"l.h"

enum{
	Align	= 4,		/* strictest data alignment */
	Small	= 3 * 4,	/* largest item in small data */
};

/*
 * layout data
 */
void
dodata(void)
{
	int i, t;
	Sym *s;
	Prog *p;
	long orig, v;

	if(debug['v'])
		Bprint(&bso, "%5.2f dodata\n", cputime());
	Bflush(&bso);
	curtext = P;
	for(p = datap; p != P; p = p->link) {
		s = p->from.sym;
		if(s->type == SBSS)
			s->type = SDATA;
		if(s->type != SDATA)
			diag("initialize non-data (%d): %s\n%P\n",
				s->type, s->name, p);
		v = p->from.offset + p->reg;
		if(v > s->value)
			diag("initialize bounds (%ld): %s\n%P\n",
				s->value, s->name, p);
	}

	/*
	 * pass 1
	 *	assign 'small' variables to small data segment
	 *	rational is that this data will be put in internal ram
	 *	and addressed through constant addressing
	 */
	orig = 0;
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA && t != SBSS)
			continue;
		v = s->value;
		if(v == 0) {
			diag("%s: no size\n", s->name);
			v = 1;
		}
		while(v & (Align-1))
			v++;
		s->value = v;
		s->size = v;

		/*
		 * don't waste space for floating constants
		 */
		if(v > Small || s->name[0] == '$')
			continue;
		s->value = orig;
		orig += v;
		s->type = SSDATA;
		s->ram = ram;
		s->isram = 1;
		ram = s;
	}
	ramover = orig;
	orig = 0;

	/*
	 * pass 2
	 *	assign 'data' variables to data segment
	 */
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		t = s->type;
		if(t != SDATA) {
			if(t == SSDATA)
				s->type = SDATA;
			continue;
		}
		v = s->value;
		s->value = orig;
		orig += v;
	}
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
		s->value = orig;
		orig += v;
	}
	bsssize = orig-datsize;
	xdefine("bdata", SDATA, 0);
	xdefine("edata", SDATA, 0+datsize);
	xdefine("end", SBSS, 0+datsize+bsssize);
}

/*
 * adjust offset in the data segment
 * to reflect ram overflow
 */
void
adjdata(void)
{
	int i;
	Sym *s;

	if(debug['v'])
		Bprint(&bso, "%5.2f adjdata\n", cputime());
	Bflush(&bso);
	for(i=0; i<NHASH; i++)
	for(s = hash[i]; s != S; s = s->link) {
		if(s->type != SDATA && s->type != SBSS)
			continue;
		if(s->isram)
			s->isram = 0;
		else
			s->value += ramover;
	}
	datsize += ramover;
	ramover = 0;
}

/*
 * check for any undefined symbols
 */
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

int
relinv(int a)
{

	switch(a) {
	case CCTRUE:	return CCFALSE;
	case CCFALSE:	return CCTRUE;

	case CCEQ:	return CCNE;
	case CCNE:	return CCEQ;

	case CCLE:	return CCGT;
	case CCGT:	return CCLE;

	case CCLT:	return CCGE;
	case CCGE:	return CCLT;

	case CCCC:	return CCCS;
	case CCCS:	return CCCC;

	case CCOC:	return CCCS;
	case CCOS:	return CCOC;

	case CCHI:	return CCLS;
	case CCLS:	return CCHI;

	case CCPL:	return CCMI;
	case CCMI:	return CCPL;

	case CCFEQ:	return CCFNE;
	case CCFNE:	return CCFEQ;

	case CCFLE:	return CCFGT;
	case CCFGT:	return CCFLE;

	case CCFLT:	return CCFGE;
	case CCFGE:	return CCFLT;

	case CCFUC:	return CCFUS;
	case CCFUS:	return CCFUC;

	case CCFOC:	return CCFOS;
	case CCFOS:	return CCFOC;

	case CCIBE:	return CCIBF;
	case CCIBF:	return CCIBE;

	case CCOBE:	return CCOBF;
	case CCOBF:	return CCOBE;

	case CCSYC:	return CCSYS;
	case CCSYS:	return CCSYC;

	case CCFBC:	return CCFBS;
	case CCFBS:	return CCFBC;

	case CCIR0C:	return CCIR0S;
	case CCIR0S:	return CCIR0C;

	case CCIR1C:	return CCIR1S;
	case CCIR1S:	return CCIR1C;
	}
	return 0;
}

/*
 * make firstp point to only the reachable code
 */
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

/*
 * follow all possible instruction sequences
 * and link them onto the list of instructions
 */
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
	if(a == AJMP) {
		q = p->cond;
		if(p->nosched || q && q->nosched){
			p->mark = FOLL;
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
			p->mark = FOLL;
			p = q;
			if(!(p->mark & FOLL))
				goto loop;
		}
	}

	/*
	 * if we enter a section that has been followed and it is small,
	 * just copy it instead of jumping there
	 */
	if(p->mark & FOLL) {
		for(i=0,q=p; i<4; i++,q=q->link) {
			if(q == lastp || q->nosched)
				break;
			b = 0;		/* set */
			a = q->as;
			if(a == ANOP) {
				i--;
				continue;
			}
			if(a == ADO || a == ADOLOCK || a == ADOEND)
				break;
			if(a == AJMP || a == ARETURN || a == AIRET)
				goto copy;
			if(!q->cond || (q->cond->mark&FOLL))
				continue;
			b = relinv(q->cc);
			if(!b)
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
				if(a == AJMP || a == ARETURN || a == AIRET)
					return;
				r->cc = b;
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
	if(a == AJMP || a == ARETURN || a == AIRET){
		if(p->nosched){
			p = p->link;
			goto loop;
		}
		return;
	}
	if(p->cond != P)
	if(a != ACALL && p->link != P) {
		xfol(p->link);
		p = p->cond;
		if(p == P || (p->mark&FOLL))
			return;
		goto loop;
	}
	p = p->link;
	goto loop;
}

/*
 * set destination of all branches
 */
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
		if((a == ACALL || a == AJMP || a == ABRA || a == ARETURN) && p->to.sym != S) {
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
			diag("branch out of range %ld\n%P\n", c, p);
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

/*
 * augment progs with forward pointers
 * that skip 1,5,25,125,625 elements
 */
#define	LOG	5
void
mkfwd(void)
{
	Prog *p;
	long dwn[LOG], cnt[LOG], i;
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

/*
 * find the ultimate destination for jmp p
 */
Prog*
brloop(Prog *p)
{
	Prog *q;
	int c;

	for(c=0; p!=P;) {
		if(p->as != AJMP || p->nosched)
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

int
adjsp(Adr *a)
{
	if(a->type == D_INC && a->reg == REGSP)
		return -4;
	if(a->type == D_DEC && a->reg == REGSP)
		return 4;
	return 0;
}

void
dostkoff(void)
{
	Prog *p, *q;
	long s, t;
	int a, suppress;

	if(debug['v'])
		Bprint(&bso, "%5.2f stkoff\n", cputime());
	Bflush(&bso);
	s = 0;
	suppress = 1;
	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			s = p->to.offset + 4;
			suppress = s == 0;
			if((p->mark & LEAF) && s > 0)
				s -= 4;
			p->to.offset = s - 4;
			p->stkoff = 0;
			continue;
		}
		t = 0;
		for(q = p; q != P; q = q->cond) {
			if(q->as == ATEXT)
				break;
			if((q->as == ADOLOCK || q->as == ADO)
			&& q->from.type == D_CONST && q->from.offset == 0){
				q->stkoff = s;
				break;
			}
			if(q->stkoff >= 0 && q->stkoff != s)
				diag("stack offset is %ld sb %ld\n%P %P\n", q->stkoff, s, q, p);
			q->stkoff = s;
			t++;
			if(t > 10)
				break;
			if(q->as == ADOEND){
				if(q->cond && q->cond->stkoff < 0)
					diag("bad DOEND: %P %P\n", q, q->cond);
				t = 10;
			}
		}
		if(suppress)
			continue;

		/*
		 * only pushes and pops by 4 and add/sub adjustments are tracked
		 */
		switch(p->as) {
		case AMOVW:
		case AFMOVF:
		case AFMOVFN:
		case AFRND:
		case AFSEED:
		case AFDSP:
		case AFIFEQ:
		case AFIFGT:
		case AFIEEE:
		case AFIFLT:
		case AFMOVFW:
		case AFMOVFH:
		case AFMOVBF:
		case AFMOVWF:
		case AFMOVHF:
			s += adjsp(&p->from);
			s += adjsp(&p->to);
			break;
		case AFADD:
		case AFADDN:
		case AFADDT:
		case AFADDTN:
		case AFMUL:
		case AFMULN:
		case AFMULT:
		case AFMULTN:
		case AFSUB:
		case AFSUBN:
		case AFSUBT:
		case AFSUBTN:
			s += adjsp(&p->from);
			s += adjsp(&p->from1);
			s += adjsp(&p->to);
			break;
		case AFMADD:
		case AFMADDN:
		case AFMADDT:
		case AFMADDTN:
		case AFMSUB:
		case AFMSUBN:
		case AFMSUBT:
		case AFMSUBTN:
			s += adjsp(&p->from);
			s += adjsp(&p->from1);
			s += adjsp(&p->from2);
			s += adjsp(&p->to);
			break;
		case ASUB:
			if(p->from.type == D_CONST
			&& p->to.type == D_REG && p->to.reg == REGSP)
				s += p->from.offset;
			break;
		case AADD:
			if(p->from.type == D_CONST
			&& (p->reg == NREG || p->reg == REGSP)
			&& p->to.type == D_REG && p->to.reg == REGSP)
				s -= p->from.offset;
			break;
		}

		/*
		 * if the next instruction is a jump
		 * we need to adjust the stack to be the same
		 */
		t = 0;
		for(q = p->link; q != P; q = q->cond) {
			if(q->as == ATEXT || q->as == ADOEND) {
				q = P;
				break;
			}
			if(q->stkoff >= 0)
				break;
			t++;
			if(t > 20)
				break;
		}
		if(q == P || q->stkoff < 0 || q->stkoff == s)
			continue;
		if(p->as == AJMP || p->as == ARETURN || p->as == AIRET)
			s = q->stkoff;
	}

	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			continue;
		}
		if(0 && p->stkoff < 0)
			diag("stkoff %d in %s: %P\n", p->stkoff, p);
		a = p->from.name;
		if(a == D_AUTO)
			p->from.offset += p->stkoff;
		if(a == D_PARAM)
			p->from.offset += p->stkoff + 4;
		a = p->to.name;
		if(a == D_AUTO)
			p->to.offset += p->stkoff;
		if(a == D_PARAM)
			p->to.offset += p->stkoff + 4;
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
