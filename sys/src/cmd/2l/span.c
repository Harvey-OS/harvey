#include	"l.h"

void
span(void)
{
	Prog *p, *q;
	long v, c, idat;
	Optab *o;
	int m, n;

	xdefine("etext", STEXT, 0L);
	xdefine("a6base", STEXT, 0L);
	idat = INITDAT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		n = 0;
		if((q = p->pcond) != P)
			if(q->back != 2)
				n = 1;
		p->back = n;
		if(p->as == AADJSP) {
			p->to.type = D_A0+7;
			v = -p->from.offset;
			p->from.offset = v;
			if((v < -8 && v >= -32768L) || (v > 8 && v < 32768L)) {
				p->as = ALEA;
				p->from.type = I_INDIR | (D_A0+7);
				continue;
			}
			p->as = AADDL;
			if(v < 0) {
				p->as = ASUBL;
				v = -v;
				p->from.offset = v;
			}
			if(v >= 0 && v <= 8)
				p->from.type = D_QUICK;
			if(v == 0)
				p->as = ANOP;
		}
	}
	n = 0;

start:
	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		o = &optab[p->as];
		p->pc = c;
		m = mmsize[o->optype];
		if(m == 0) {
			if(p->as == AWORD)
				m = 2;
			if(p->as == ALONG)
				m = 4;
			p->mark = m;
			c += m;
			continue;
		}
		if(p->from.type != D_NONE)
			m += andsize(p, &p->from);
		if(p->to.type == D_BRANCH) {
			if(p->pcond == P)
				p->pcond = p;
			c += m;
			if(m == 2)
				m |= 0100;
			p->mark = m;
			continue;
		}
		if(p->to.type != D_NONE)
			m += andsize(p, &p->to);
		p->mark = m;
		c += m;
	}

loop:
	n++;
	if(debug['v'])
		Bprint(&bso, "%5.2f span %d\n", cputime(), n);
	Bflush(&bso);
	if(n > 60) {
		diag("span must be looping");
		errorexit();
	}
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		if((m = p->mark) & 0100) {
			q = p->pcond;
			v = q->pc - 2;
			if(p->back)
				v -= c;
			else
				v -= p->pc;
			p->pc = c;
			if(v < -32768L || v >= 32768L) {
				if(p->as == ABSR && q->pc < 32768L && q->pc >= 0) 
					c += 4;
				else
					c += 6;
			} else
			if(v < -128 || v >= 128)
				c += 4;
			else
			if(v == 0) {
				c += 4;
				p->mark = 4;
			} else
				c += 2;
			continue;
		}
		p->pc = c;
		c += m;
	}
	if(c != textsize) {
		textsize = c;
		goto loop;
	}
	if(INITRND)
		INITDAT = rnd(c, INITRND);
	if(INITDAT != idat) {
		idat = INITDAT;
		goto start;
	}
	xdefine("etext", STEXT, c);
	xdefine("a6base", STEXT, INITDAT+A6OFFSET);
	if(debug['v'])
		Bprint(&bso, "etext = %lux\n", c);
	Bflush(&bso);
	for(p = textp; p != P; p = p->pcond)
		p->from.sym->value = p->pc;
	textsize = c - INITTEXT;
}

void
xdefine(char *p, int t, long v)
{
	Sym *s;

	s = lookup(p, 0);
	if(s->type == 0 || s->type == SXREF) {
		s->type = t;
		s->value = v;
	}
	if(s->type == STEXT && s->value == 0)
		s->value = v;
}

int
andsize(Prog *p, Adr *ap)
{
	int t, n;
	long v;
	Optab *o;

	t = ap->type;
	if(ap->index != D_NONE) {
		n = 2;
		v = ap->displace;
		if(v != 0) {
			n += 2;
			if(v < -32768L || v >= 32768L)
				n += 2;
		}
		switch(t) {
		default:
			v = ap->offset;
			break;

		case D_STATIC:
		case D_EXTERN:
			if(ap->sym->type == STEXT)
				return n+4;	/* see below */
			v = ap->sym->value + ap->offset - A6OFFSET;
			if(debug['6'])
				v += INITDAT + A6OFFSET;
		}
		if(v != 0) {
			n += 2;
			if(v < -32768L || v >= 32768L)
				n += 2;
		}
		return n;
	}
	n = simple[t];
	if(n != 0177) {
		v = ap->offset;
		if(v == 0)
			return 0;
		if((n&070) != 020)	/* D_INDIR */
			return 0;
		if(v == 0)
			return 0;
		if(v < -32768L || v >= 32768L)
			return 6;	/* switch to index1 mode */
		return 2;
	}
	if((t&I_MASK) == I_ADDR)
		t = D_CONST;
	switch(t) {

	default:
		return 0;

	case D_STACK:
	case D_AUTO:
	case D_PARAM:
		v = ap->offset;
		if(v == 0)
			return 0;
		if(v < -32768L || v >= 32768L)
			return 6;	/* switch to index1 mode */
		return 2;

	case I_INDIR|D_CONST:
		v = ap->offset;
		goto adr;

	case D_STATIC:
	case D_EXTERN:
		if(ap->sym->type == STEXT)
			return 4;	/* too slow to get back into namelist */
		v = ap->sym->value + ap->offset - A6OFFSET;
		if(debug['6']) {
			v += INITDAT + A6OFFSET;
			goto adr;
		}
		if(v == 0)
			return 0;

	adr:
		if(v < -32768L || v >= 32768L)
			return 4;
		return 2;

	case D_CONST:
	case D_FCONST:
		o = &optab[p->as];
		if(ap == &(p->from))
			return o->srcsp;
		return o->dstsp;

	case D_CCR:
	case D_SR:
		if(p->as == AMOVW)
			return 0;
		return 2;

	case D_USP:
		t = p->from.type;
		if(t >= D_A0 && t <= D_A0+8)
			return 0;
		t = p->to.type;
		if(t >= D_A0 && t <= D_A0+8)
			return 0;

	case D_SFC:
	case D_DFC:
	case D_CACR:
	case D_VBR:
	case D_CAAR:
	case D_MSP:
	case D_ISP:
	case D_FPCR:
	case D_FPSR:
	case D_FPIAR:
 	case D_TC:
	case D_ITT0:
	case D_ITT1:
	case D_DTT0:
	case D_DTT1:
	case D_MMUSR:
	case D_URP:
	case D_SRP:
		return 2;
	}
}

void
putsymb(Sym *s, int t, long v)
{
	int i, f;
	char *n;

	n = s->name;
	if(t == 'f')
		n++;
	lput(v);
	if(s->version)
		t += 'a' - 'A';
	CPUT(t+0x80);			/* 0x80 is variable length */

	if(t == 'Z' || t == 'z') {
		CPUT(n[0]);
		for(i=1; n[i] != 0 || n[i+1] != 0; i += 2) {
			CPUT(n[i]);
			CPUT(n[i+1]);
		}
		CPUT(0);
		CPUT(0);
		i++;
	}
	else {
		for(i=0; n[i]; i++)
			CPUT(n[i]);
		CPUT(0);
	}
	symsize += 4 + 1 + i + 1;

	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			Bprint(&bso, "%c %.8lux ", t, v);
			for(i=1; n[i] != 0 || n[i+1] != 0; i+=2) {
				f = ((n[i]&0xff) << 8) | (n[i+1]&0xff);
				Bprint(&bso, "/%x", f);
			}
			Bprint(&bso, "\n");
			return;
		}
		if(s->version)
			Bprint(&bso, "%c %.8lux %s<%d>\n", t, v, n, s->version);
		else
			Bprint(&bso, "%c %.8lux %s\n", t, v, n);
	}
}

void
asmsym(void)
{
	Prog *p;
	Auto *a;
	Sym *s;
	int h;

	s = lookup("etext", 0);
	if(s->type == STEXT)
		putsymb(s, 'T', s->value);
	s = lookup("a6base", 0);
	if(s->type == STEXT)
		putsymb(s, 'D', s->value);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SDATA:
				putsymb(s, 'D', s->value+INITDAT);
				continue;

			case SBSS:
				putsymb(s, 'B', s->value+INITDAT);
				continue;

			case SFILE:
				putsymb(s, 'f', s->value);
				continue;
			}

	for(p=textp; p!=P; p=p->pcond) {
		s = p->from.sym;
		if(s->type != STEXT)
			continue;

		/* filenames first */
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_FILE)
				putsymb(a->asym, 'z', a->aoffset);
			else
			if(a->type == D_FILE1)
				putsymb(a->asym, 'Z', a->aoffset);

		putsymb(s, 'T', s->value);

		/* auto and param after */
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->asym, 'a', -a->aoffset);
			else
			if(a->type == D_PARAM)
				putsymb(a->asym, 'p', a->aoffset);
	}
	if(debug['v'] || debug['n'])
		Bprint(&bso, "symsize = %lud\n", symsize);
	Bflush(&bso);
}

#define	MINLC	2
void
asmsp(void)
{
	long oldpc, oldsp;
	Prog *p;
	int s;
	long v;

	oldpc = INITTEXT;
	oldsp = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->stkoff == oldsp || p->as == ATEXT || p->as == ANOP) {
			if(p->as == ATEXT)
				curtext = p;
			if(debug['G'])
				Bprint(&bso, "%6lux %4ld%P\n",
					p->pc, p->stkoff, p);
			continue;
		}
		if(debug['G'])
			Bprint(&bso, "\t\t%6ld", spsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			s = 127;
			if(v < 127)
				s = v;
			CPUT(s+128);	/* 129-255 +pc */
			if(debug['G'])
				Bprint(&bso, " pc+%d*2(%d)", s, s+128);
			v -= s;
			spsize++;
		}
		v = p->stkoff - oldsp;
		oldsp = p->stkoff;
		oldpc = p->pc + MINLC;
		if(v & 3 || v > 64L*4L || v < -64L*4L) {
			CPUT(0);	/* 0 vvvv +sp */
			lput(v);
			if(debug['G']) {
				if(v > 0)
					Bprint(&bso, " sp+%ld*1(%d,%ld)\n",
						v, 0, v);
				else
					Bprint(&bso, " sp%ld*1(%d,%ld)\n",
						v, 0, v);
				Bprint(&bso, "%6lux %4ld%P\n",
					p->pc, p->stkoff, p);
			}
			spsize += 5;
			continue;
		}
		s = v/4;
		if(s > 0) {
			CPUT(0+s);	/* 1-64 +sp */
			if(debug['G']) {
				Bprint(&bso, " sp+%d*4(%d)\n", s, 0+s);
				Bprint(&bso, "%6lux %4ld%P\n",
					p->pc, p->stkoff, p);
			}
		} else {
			CPUT(64-s);	/* 65-128 -sp */
			if(debug['G']) {
				Bprint(&bso, " sp%d*4(%d)\n", s, 64-s);
				Bprint(&bso, "%6lux %4ld%P\n",
					p->pc, p->stkoff, p);
			}
		}
		spsize++;
	}
	while(spsize & 1) {
		s = 129;
		CPUT(s);
		spsize++;
	}
	if(debug['v'] || debug['G'])
		Bprint(&bso, "stsize = %ld\n", spsize);
	Bflush(&bso);
}

void
asmlc(void)
{
	long oldpc, oldlc;
	Prog *p;
	long v, s;

	oldpc = INITTEXT;
	oldlc = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->line == oldlc || p->as == ATEXT || p->as == ANOP) {
			if(p->as == ATEXT)
				curtext = p;
			if(debug['L'])
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			continue;
		}
		if(debug['L'])
			Bprint(&bso, "\t\t%6ld", lcsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			s = 127;
			if(v < 127)
				s = v;
			CPUT(s+128);	/* 129-255 +pc */
			if(debug['L'])
				Bprint(&bso, " pc+%ld*%d(%ld)", s, MINLC, s+128);
			v -= s;
			lcsize++;
		}
		s = p->line - oldlc;
		oldlc = p->line;
		oldpc = p->pc + MINLC;
		if(s > 64 || s < -64) {
			CPUT(0);	/* 0 vv +lc */
			CPUT(s>>24);
			CPUT(s>>16);
			CPUT(s>>8);
			CPUT(s);
			if(debug['L']) {
				if(s > 0)
					Bprint(&bso, " lc+%ld(%d,%ld)\n",
						s, 0, s);
				else
					Bprint(&bso, " lc%ld(%d,%ld)\n",
						s, 0, s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
			lcsize += 5;
			continue;
		}
		if(s > 0) {
			CPUT(0+s);	/* 1-64 +lc */
			if(debug['L']) {
				Bprint(&bso, " lc+%ld(%ld)\n", s, 0+s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		} else {
			CPUT(64-s);	/* 65-128 -lc */
			if(debug['L']) {
				Bprint(&bso, " lc%ld(%ld)\n", s, 64-s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		}
		lcsize++;
	}
	while(lcsize & 1) {
		s = 129;
		CPUT(s);
		lcsize++;
	}
	if(debug['v'] || debug['L'])
		Bprint(&bso, "lcsize = %ld\n", lcsize);
	Bflush(&bso);
}
