#include	"l.h"

void
span(void)
{
	Prog *p, *q;
	long c, idat;
	int m, n, again;

	xdefine("etext", STEXT, 0L);
	idat = INITDAT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		n = 0;
		q = p->cond;
		if(q != P)
			if(q->back != 2)
				n = 1;
		p->back = n;
	}
	n = 0;

start:
	if(debug['v'])
		Bprint(&bso, "%5.2f span\n", cputime());
	Bflush(&bso);
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + 4;
			if(p->from.sym != S)
				p->from.sym->value = c;
		}
		p->pc = c;
		if(p->to.type == D_BRANCH) {
			if(p->cond == P)
				p->cond = p;
		}
		asmins(p);
		m = (char*)andptr - (char*)and;
		p->mark = m;
		c += m;
	}

loop:
	n++;
	if(debug['v'])
		Bprint(&bso, "%5.2f span %d\n", cputime(), n);
	Bflush(&bso);
	if(n > 60) {
		print("span must be looping\n");
		errorexit();
	}
	again = 0;
	c = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + 4;
		}
		if(p->to.type == D_BRANCH) {
			asmins(p);
			if(!p->back)
				p->pc = c;
			m = (char*)andptr - (char*)and;
			if(m != p->mark) {
				p->mark = m;
				again++;
			}
		}
		p->pc = c;
		c += p->mark;
	}
	if(again) {
		textsize = c;
		goto loop;
	}
	INITDAT = c;
	if(INITRND) {
		INITDAT = rnd(c, INITRND);
		if(INITDAT != idat) {
			idat = INITDAT;
			goto start;
		}
	}
	xdefine("etext", STEXT, c);
	if(debug['v'])
		Bprint(&bso, "etext = %lux\n", c);
	Bflush(&bso);
	for(p = textp; p != P; p = p->cond)
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
	if(s->type == STEXT || s->type == SLEAF)
		if(s->value == 0)
			s->value = v;
}

void
putsymb(char *s, int t, long v, int ver)
{
	int i, f;

	if(t == 'f')
		s++;
	lput(v);
	if(ver)
		t += 'a' - 'A';
	CPUT(t+0x80);			/* 0x80 is variable length */

	if(t == 'Z' || t == 'z') {
		CPUT(s[0]);
		for(i=1; s[i] != 0 || s[i+1] != 0; i += 2) {
			CPUT(s[i]);
			CPUT(s[i+1]);
		}
		CPUT(0);
		CPUT(0);
		i++;
	}
	else {
		for(i=0; s[i]; i++)
			CPUT(s[i]);
		CPUT(0);
	}
	symsize += 4 + 1 + i + 1;

	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			Bprint(&bso, "%c %.8lux ", t, v);
			for(i=1; s[i] != 0 || s[i+1] != 0; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				Bprint(&bso, "/%x", f);
			}
			Bprint(&bso, "\n");
			return;
		}
		if(ver)
			Bprint(&bso, "%c %.8lux %s<%d>\n", t, v, s, ver);
		else
			Bprint(&bso, "%c %.8lux %s\n", t, v, s);
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
		putsymb(s->name, 'T', s->value, s->version);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SDATA:
				putsymb(s->name, 'D', s->value+INITDAT, s->version);
				continue;

			case SBSS:
				putsymb(s->name, 'B', s->value+INITDAT, s->version);
				continue;

			case SFILE:
				putsymb(s->name, 'f', s->value, s->version);
				continue;
			}

	for(p=textp; p!=P; p=p->cond) {
		s = p->from.sym;
		if(s->type != STEXT && s->type != SLEAF)
			continue;

		/* filenames first */
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_FILE)
				putsymb(a->sym->name, 'z', a->offset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->sym->name, 'Z', a->offset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+4, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->sym->name, 'a', -a->offset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->sym->name, 'p', a->offset, 0);
	}
	if(debug['v'] || debug['n'])
		Bprint(&bso, "symsize = %lud\n", symsize);
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

int
oclass1(Prog *p)
{
	int t;

	t = p->type;
	if(t == D_NONE)
		return Ynone;
	if(t >= D_R0 && t < D_R0+32)
		return Yr;
	if(t == D_CONST)
		return Yi5;
	return Yxxx;
}

int
oclass(Adr *a)
{
	long v;

	if(a->type >= D_INDIR || a->index != D_NONE) {
		if(a->index != D_NONE && a->scale == 0) {
			if(a->type >= D_ADDR)
				return Yi32;
			return Ycol;
		}
		return Ym;
	}
	switch(a->type)
	{
	default:
		if(a->type >= D_R0 && a->type < D_R0+32)
			return Yr;
	case D_NONE:
		return Ynone;

	case D_EXTERN:
	case D_STATIC:
	case D_AUTO:
	case D_PARAM:
		return Ym;

	case D_CONST:
	case D_ADDR:
		if(a->sym == S) {
			v = a->offset;
			if(v >= 0 && v < 32)
				return Yi5;
		}
		return Yi32;

	case D_BRANCH:
		return Ybr;
	}
	return Yxxx;
}

void
dob(int o, long dpc)
{
	long i;

	i = (o & 0xff) << 24;
	if(dpc >= -(1<<23) || dpc < (1<<23)) {
		i |= dpc & 0xfffffc;	/* tandi seems to keep low bits on */
		*andptr++ = i;
		return;
	}
	diag("branch 23 too far");
}

void
dorrb(int o, int r, int m, long dpc)
{
	long i;

	i = (o & 0xff) << 24;
	i |= (r & 0x1f) << 19;
	if(r & 32)
		i |= 1<<13;		/* m1 */
	i |= (m & 0x1f) << 14;
	if(r & 32)
		i |= 1<<13;		/* m1 */
/* bug?? */
	if(dpc >= -((1<<12)) || dpc < ((1<<12))) {
		i |= dpc & 0xffc;
		*andptr++ = i;
		return;
	}
	diag("branch 12 too far");
}

void
dormem(int o, int r, Adr *a)
{
	long i, v;
	int t, abase, index, scale;


	abase = D_NONE;
	index = a->index;
	scale = a->scale;
	v = a->offset;

	t = a->type;
	if(t >= D_INDIR) {
		t -= D_INDIR;
		if(t < D_R0 || t >= D_R0+32)
			if(t != D_NONE)
				diag("dormem 1 %D %d", a, a->type);
		abase = t;
		goto asm;
	}
	switch(t) {
	default:
		diag("dormem 2 %D", a);
		break;

	case D_STATIC:
	case D_EXTERN:
		t = a->sym->type;
		if(t == 0 || t == SXREF) {
			diag("undefined external: %s in %s\n",
				a->sym->name, TNAME);
			a->sym->type = SDATA;
		}
		v += a->sym->value;
		abase = REGSB;
		if(a->sym == symSB) {
			v = INITDAT;
			abase = D_NONE;
		}
		break;

	case D_PARAM:
		v += 4L;

	case D_AUTO:
		v += autosize;
		abase = REGSP;
		break;
	}

asm:
	t = 0;
	if(v == 0)
		t |= Anooffset;
	else
	if(v < 0 || v >= 4096)
		t |= Abigoffset;
	if(index != D_NONE) {
		t |= Aindex;
		switch(scale) {
		default:
			diag("bad scale %A", a);
		case 1:
			scale = 0;
			break;
		case 2:
			scale = 1;
			break;
		case 4:
			scale = 2;
			break;
		case 8:
			scale = 3;
			break;
		case 16:
			scale = 4;
			break;
		}
	}
	if(abase != D_NONE)
		t |= Abase;

	i = (o & 0xff) << 24;
	i |= (r & 0x1f) << 19;
	switch(t) {
	default:
		diag("dormem mode %lux %D\n", t, a);
		break;

	case 0:
	case Anooffset:
		/*
		 * mema mode 0
		 */
		i |= v;
		*andptr++ = i;
		break;

	case Abase:
		/*
		 * mema mode 1
		 */
		i |= 1 << 13;
		i |= ((abase-D_R0) & 0x1f) << 14;
		i |= v;
		*andptr++ = i;
		break;

	case Abase|Anooffset:
		/*
		 * memb mode 4
		 */
		i |= 0x4 << 10;
		i |= ((abase-D_R0) & 0x1f) << 14;
		*andptr++ = i;
		break;

	case Abase|Aindex|Anooffset:
		/*
		 * memb mode 7
		 */
		i |= 0x7 << 10;
		i |= ((abase-D_R0) & 0x1f) << 14;
		i |= ((index-D_R0) & 0x1f) << 0;
		i |= (scale & 0x7) << 7;
		*andptr++ = i;
		break;

	case Abigoffset:
		/*
		 * memb mode c
		 */
		i |= 0xc << 10;
		*andptr++ = i;
		*andptr++ = v;
		break;

	case Abase|Abigoffset:
		/*
		 * memb mode d
		 */
		i |= 0xd << 10;
		i |= ((abase-D_R0) & 0x1f) << 14;
		*andptr++ = i;
		*andptr++ = v;
		break;

	case Aindex:
	case Aindex|Abigoffset:
	case Aindex|Anooffset:
		/*
		 * memb mode e
		 */
		i |= 0xe << 10;
		i |= ((index-D_R0) & 0x1f) << 0;
		i |= (scale & 0x7) << 7;
		*andptr++ = i;
		*andptr++ = v;
		break;

	case Abase|Aindex:
	case Abase|Aindex|Abigoffset:
		/*
		 * memb mode f
		 */
		i |= 0xf << 10;
		i |= ((abase-D_R0) & 0x1f) << 14;
		i |= ((index-D_R0) & 0x1f) << 0;
		i |= (scale & 0x7) << 7;
		*andptr++ = i;
		*andptr++ = v;
		break;
	}

}

void
dorrr(int op, int src1, int src2, int dst)
{
	ulong i;

	i = (op & 0xfL) << 7;
	i |= (op & 0xff0L) << 20;
	i |= (src1 & 31) << 0;		/* src1 */
	if(src1 & 32)
		i |= 1<<11;		/* m1 */
	i |= (src2 & 31) << 14;		/* src2 */
	if(src2 & 32)
		i |= 1<<12;		/* m2 */
	i |= (dst & 31) << 19;		/* dst */
	*andptr++ = i;
}

void
doir(Adr *a, int dst)
{
	long c;
	int t;

	c = a->offset;
	if(a->type == D_ADDR) {
		switch(a->index) {
		default:
			diag("type in doir/D_ADDR");
			break;
		case D_EXTERN:
		case D_STATIC:
			if(!a->sym)
				break;
			t = a->sym->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s in %s\n",
					a->sym->name, TNAME);
				a->sym->type = SDATA;
			}
			c += a->sym->value;
			if(a->sym == symSB)
				c = INITDAT;
		}
	} else
	if(a->type != D_CONST)
		diag("type in doir");

	if(c >= 0 && c < 4096) {
		*andptr++ = (0x8c<<24) | (dst<<19) | (c<<0);	/* mova $c,dst */
		return;
	}
	if(c >= -31 && c < 0) {
		dorrr(0x592, 32-c, 32+0, dst);			/* subo $c,$0,dst */
		return;
	}
	for(t=10; t<32-5; t++)
		if((c & ~(31<<t)) == 0) {			/* shlo $c1,$c2,dst */
			dorrr(0x59c, 32+t, 32+((c>>t)&31), dst);
			return;
		}
	*andptr++ = (0x8c<<24) | (dst<<19) | (0xc<<10);	/* mova $bigc,dst */
	*andptr++ = c;
}

void
doasm(Prog *p)
{
	Optab *o;
	uchar *t;
	int z, ft, tt, mt;

	o = &optab[p->as];
	ft = oclass(&p->from) * Ymax;
	mt = oclass1(p) * Ymax;
	tt = oclass(&p->to) * Ymax;
	t = o->ytab;
	if(t == 0) {
		diag("asmins: noproto %P\n", p);
		return;
	}
	for(z=0; *t; z+=t[4], t+=5)
		if(ycover[ft+t[0]])
		if(ycover[mt+t[1]])
		if(ycover[tt+t[2]])
			goto found;

	diag("asmins: notfound <%d,%d,%d> %P\n",
		ft/Ymax, mt/Ymax, tt/Ymax, p);
	return;

found:
	switch(t[3]) {
	default:
		diag("asmins: unknown z %d %P\n", t[3], p);
		return;

	case Zpseudo:
		break;

	case Zbr:
		dob(o->op[z], p->cond->pc - p->pc);
		break;

	case Zrxr:	/* Yri5, Ynone, Yr5 */
		if(ft == Yr*Ymax)
			ft = p->from.type - D_R0;
		else
			ft = p->from.offset | 32;

		dorrr(o->op[z], ft, 0, p->to.type-D_R0);
		break;

	case Zrrx:	/* Yri5, Ynone, Yri5 */
		if(ft == Yr*Ymax)
			ft = p->from.type - D_R0;
		else
			ft = p->from.offset | 32;
		if(tt == Yr*Ymax)
			tt = p->to.type - D_R0;
		else
			tt = p->to.offset | 32;

		dorrr(o->op[z], ft, tt, 0);
		break;

	case Zirx:	/* Yi32, Ynone, Yri5 */
		if(tt == Yr*Ymax)
			tt = p->to.type - D_R0;
		else
			tt = p->to.offset | 32;

		doir(&p->from, REGTMP-D_R0);
		dorrr(o->op[z], REGTMP-D_R0, tt, 0);
		break;

	case Zrrr:	/* Yri5, Ynri5, Yr */
		if(ft == Yr*Ymax)
			ft = p->from.type - D_R0;
		else
			ft = p->from.offset | 32;

		if(mt == Ynone*Ymax)
			mt = p->to.type - D_R0;
		else
		if(mt == Yr*Ymax)
			mt = p->type - D_R0;
		else
			mt = p->offset | 32;

		dorrr(o->op[z], ft, mt, p->to.type-D_R0);
		break;

	case Zir:	/* Yi32, Yr */
		doir(&p->from, p->to.type-D_R0);
		break;

	case Zirr:	/* Yi32, Ynri5, Yr */
		if(mt == Ynone*Ymax)
			mt = p->to.type - D_R0;
		else
		if(mt == Yr*Ymax)
			mt = p->type - D_R0;
		else
			mt = p->offset | 32;

		doir(&p->from, REGTMP-D_R0);
		dorrr(o->op[z], REGTMP-D_R0, mt, p->to.type-D_R0);
		break;

	case Zmbr:
		dormem(o->op[z], REGLINK-D_R0, &p->to);
		break;

	case Zmr:
		dormem(o->op[z], p->to.type-D_R0, &p->from);
		break;

	case Zrm:
		dormem(o->op[z], p->from.type-D_R0, &p->to);
		break;

	case Zim:
		if(p->from.offset != 0) {
			doir(&p->from, REGTMP-D_R0);
			dormem(o->op[z], REGTMP-D_R0, &p->to);
		} else
			dormem(o->op[z], REGZERO-D_R0, &p->to);
		break;

	case Zlong:
		*andptr++ = p->from.offset;
		break;

	case Znone:
		dorrr(o->op[z], 0, 0, 0);
		break;
	}
}

void
asmins(Prog *p)
{

	andptr = and;
	doasm(p);
}
