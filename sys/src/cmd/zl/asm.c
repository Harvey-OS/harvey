#include	"l.h"

#define	SPUT(c)\
	{\
		cbp[0] = (c)>>8;\
		cbp[1] = (c);\
		cbp += 2;\
		cbc -= 2;\
		if(cbc <= 0)\
			cflush();\
	}

#define	CPUT(c)\
	{\
		cbp[0] = (c);\
		cbp ++;\
		cbc --;\
		if(cbc <= 0)\
			cflush();\
	}

long
entryvalue(void)
{
	char *a;
	Sym *s;

	a = INITENTRY;
	if(*a >= '0' && *a <= '9')
		return atolwhex(a);
	s = lookup(a, 0);
	if(s->type == 0)
		return INITTEXT;
	if(s->type != STEXT && s->type != SLEAF)
		diag("entry not text: %s", s->name);
	return s->value;
}

void
asmb(void)
{
	Prog *p;
	Optab *o;
	long t;
	int s;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);

	seek(cout, HEADR, 0);
	swizflag = 1;
	pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->pc != pc) {
			diag("phase error %lux sb %lux\n%P\n", p->pc, pc, p);
			pc = p->pc;
		}
		curp = p;
		if(p->as == ATEXT)
			curtext = p;
		o = &optab[p->as];

		switch(o->type) {
		default:
			diag("unknown op type\n%P\n", p);
			if(p->link)
				pc = p->link->pc;
			break;

		case TWORD:
			s = p->to.offset & 0xffff;
			SPUT(s)
			if(debug['a'])
				Bprint(&bso, "%4lux: %.4lux\t\t%P\n",
					pc-INITTEXT, s, p);
			pc += 2;
			break;

		case TLONG:
			t = p->to.offset;
			SPUT(t>>16)
			SPUT(t)
			if(debug['a'])
				Bprint(&bso, "%4lux: %.8lux\t\t%P\n",
					pc-INITTEXT, t, p);
			pc += 4;
			break;

		case TTEXT:
			autosize = p->to.offset;
			if(debug['a'])
				Bprint(&bso, "%4lux:\t\t\t%P\n", pc-INITTEXT, p);
			break;

		case TFLOT:
			s = andasm(p, o, 1);
			pc += s;
			break;

		case TDYAD:
			s = andasm(p, o, 0);
			pc += s;
			break;

		case TPCTO:
			s = andpcasm(p, o);
			pc += s;
			if(p->as == AJMP)
				while(pc & AMOD) {
					SPUT(0)
					pc += 2;
				}
			break;

		case TNNAD:
			s = (0xb << 10) | o->genop;
			SPUT(s)
			if(debug['a'])
				Bprint(&bso, "%4lux: %.4ux\t\t%P\n", pc-INITTEXT, s, p);
			pc += 2;
			break;

		case TDMON:
			s = and10asm(p, o);
			pc += s;
			if(p->as == ARETURN)
				while(pc & AMOD) {
					SPUT(0)
					pc += 2;
				}
			break;
		}
		continue;
	}
	while(pc & 3) {
		CPUT(0);
		pc++;
	}
	cflush();
	swizflag = 0;

	switch(HEADTYPE) {
	case 0:
		seek(cout, rnd(HEADR+textsize, 4096), 0);
		break;
	case 1:
	case 2:
		seek(cout, HEADR+textsize, 0);
		break;
	}
	for(t = 0; t < datsize; t += sizeof(buf)-100) {
		if(datsize-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100);
		else
			datblk(t, datsize-t);
	}

	symsize = 0;
	lcsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);

		switch(HEADTYPE) {
		case 0:
			seek(cout, rnd(HEADR+textsize, 4096)+datsize, 0);
			break;
		case 1:
		case 2:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f pc\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		cflush();
	}

	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	case 0:
		lput(0x160L<<16);		/* magic and sections */
		lput(0L);			/* time and date */
		lput(rnd(HEADR+textsize, 4096)+datsize);
		lput(symsize);			/* nsyms */
		lput((0x38L<<16)|7L);		/* size of optional hdr and flags */
		lput((0413<<16)|0437L);		/* magic and version */
		lput(rnd(HEADR+textsize, 4096));	/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT-HEADR);		/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		lput(lcsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
		break;
	case 1:
		lput(0x161L<<16);		/* magic and sections */
		lput(0L);			/* time and date */
		lput(HEADR+textsize+datsize);
		lput(symsize);			/* nsyms */
		lput((0x38L<<16)|7L);		/* size of optional hdr and flags */
		lput((407<<16)|0437L);	/* magic and version */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT);			/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT+datsize);		/* va of base of bss */
		lput(~0L);			/* gp reg mask */
		lput(lcsize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
		lput(0L);			/* complete mystery */
		break;
	case 2:
		lput(407);			/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	}


	cflush();
}

void
lput(long l)
{

	SPUT(l>>16)
	SPUT(l)
}

void
cflush(void)
{
	int n, i, t;

	n = sizeof(buf.cbuf) - cbc;
	if(n) {
		if(swizflag) {
			if(n & 3)
				diag("not multiple of 4 in swiz\n");
			for(i=0; i<n; i+=4) {
				t = buf.cbuf[i+0];
				buf.cbuf[i+0] = buf.cbuf[i+3];
				buf.cbuf[i+3] = t;
				t = buf.cbuf[i+1];
				buf.cbuf[i+1] = buf.cbuf[i+2];
				buf.cbuf[i+2] = t;
			}
		}
		write(cout, buf.cbuf, n);
	}
	cbp = buf.cbuf;
	cbc = sizeof(buf.cbuf);
}

void
asmsym(void)
{
	Prog *p;
	Auto *a;
	Sym *s;
	int h;
	char name[NNAME];

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
			if(a->type == D_CPU)
				putsymb(a->sym->name, 'Z', a->offset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		strncpy(name, ".frame", NNAME);
		putsymb(name, 'm', p->to.offset+4, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->sym->name, 'a', -a->offset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->sym->name, 'p', a->offset, 0);
	}
	if(debug['v'] || debug['n'])
		Bprint(&bso, "symsize = %lud\n", symsize);
}

void
putsymb(char *s, int t, long v, int ver)
{
	int i, f;
	char str[STRINGSZ];

	if(t == 'f')
		s++;
	lput(v);
	if(ver)
		t += 'a' - 'A';
	CPUT(t);
	for(i=0; i<NNAME; i++)
		CPUT(s[i]);
	CPUT(0);
	CPUT(0);
	CPUT(0);
	symsize += 4 + 1 + NNAME + 3;
	if(debug['n']) {
		if(t == 'z' || t == 'Z') {
			str[0] = 0;
			for(i=1; i<NNAME; i+=2) {
				f = ((s[i]&0xff) << 8) | (s[i+1]&0xff);
				if(f == 0)
					break;
				sprint(strchr(str, 0), "/%x", f);
			}
			Bprint(&bso, "%c %.8lux %s\n", t, v, str);
			return;
		}
		if(ver)
			Bprint(&bso, "%c %.8lux %s<%d>\n", t, v, s, ver);
		else
			Bprint(&bso, "%c %.8lux %s\n", t, v, s);
	}
}

#define	MINLC	2
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
}

void
datblk(long s, long n)
{
	Prog *p;
	char *cast;
	long l, fl, j, d;
	int i, c, t;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		l = p->from.sym->value + p->from.offset - s;
		c = wsize[p->from.width];
		t = p->to.type;
		if(t == D_SCONST)
			c = p->to.width;
		i = 0;
		if(l < 0) {
			if(l+c <= 0)
				continue;
			while(l < 0) {
				l++;
				i++;
			}
		}
		if(l >= n)
			continue;
		for(j=l+(c-i)-1; j>=l; j--)
			if(buf.dbuf[j]) {
				Bprint(&bso, "%P\n", p);
				diag("multiple initialization\n");
				break;
			}
		switch(t) {
		default:
			diag("unknown mode in initialization\n%P\n", p);
			break;

		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(&p->to.ieee);
				cast = (char*)&fl;
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux: ", l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[fnuxi8[j+4]] & 0xff);
					Bprint(&bso, "\t\t%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i+4]];
					l++;
				}
				break;
			case 8:
				cast = (char*)&p->to.ieee;
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux: ", l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[fnuxi8[j]] & 0xff);
					Bprint(&bso, "\t\t%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i]];
					l++;
				}
				break;
			}
			break;

		case D_SCONST:
			if(debug['a'] && i == 0) {
				Bprint(&bso, "%lux: ", l+s+INITDAT);
				for(j=0; j<c; j++)
					Bprint(&bso, "%.2ux", p->to.sval[j] & 0xff);
				Bprint(&bso, "\t\t%P\n", curp);
			}
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.sval[i];
				l++;
			}
			break;

		case D_ADDR:
			t = p->to.sym->type;
			if(t == 0 || t == SXREF) {
				diag("undefined external: %s\n%P\n", p->to.sym->name, p);
				t = SDATA;
				p->to.sym->type = t;
			}
			d = p->to.offset;
			if(t == STEXT)
				d += p->to.sym->value;
			if(t == SDATA)
				d += p->to.sym->value + INITDAT;
			if(t == SBSS)
				d += p->to.sym->value + INITDAT;
			goto nux;

		case D_CONST:
			d = p->to.offset;

		nux:
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P\n", c, i, curp);
				break;
			case 1:
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux: ", l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux",cast[inuxi1[j]] & 0xff);
					Bprint(&bso, "\t\t%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux: ", l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux",cast[inuxi2[j]] & 0xff);
					Bprint(&bso, "\t\t%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux: ", l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux",cast[inuxi4[j]] & 0xff);
					Bprint(&bso, "\t\t%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi4[i]];
					l++;
				}
				break;
			}
			break;
		}
	}
	write(cout, buf.dbuf, n);
}

int
andpcasm(Prog *p, Optab *o)
{
	int s1, s2;

	operand = p->to.offset - p->pc;
	s1 = o->genop;
	s2 = 0; /* set */

	switch(p->as) {

	case AJMPT:
	case AJMPF:
		/*
		 * make backward jumps predict yes
		 */
		if(operand < 0)
			s1 |= 1;

	case AJMP:
	case ACALL:
	case AJMPFN:
	case AJMPFY:
	case AJMPTN:
	case AJMPTY:
		if(operand >= JMPLO && operand <= 1022) {
			s1 <<= 10;
			s1 |= (operand >> 1) & 0x3ff;
			SPUT(s1)
			if(debug['a']) {
				Bprint(&bso, "%4lux: %.4ux\t\t%P\n", pc-INITTEXT, s1, p);
			}
			return 2;
		}
/*
		s2 = s1;
		if(p->as != ACALL)
			break;
*/

	case ALDRAA:
		s2 = 0;
	}
	s1 = 0x80e0 | (s2 << 8) | s1;
	SPUT(s1)
	SPUT(operand>>16)
	SPUT(operand)
	if(debug['a'])
		Bprint(&bso, "%4lux: %.4ux %.8lux\t%P\n", pc-INITTEXT,
			s1, operand, p);
	return 6;
}

int
and10asm(Prog *p, Optab *o)
{
	int n, as;

	as = p->as;
	n = codc2[andsize(p, &p->to)];
	switch(n) {

	case '0':
		if(as == AKCALL)
		if(operand >= 0 && operand < 4096 && (operand&3) == 0) {
			/*
			 * monadic encoded 2 byte parcel
			 */
			n = (o->decod[0] - '0') << 10;
			n |= (operand >> 2) & 0x3ff;
			SPUT(n)
			if(debug['a'])
				Bprint(&bso, "%4lux: %.4ux\t\t%P\n", pc-INITTEXT, n, p);
			return 2;
		}
		break;

	case '1':
		if(as == ACATCH || as == APOPN || as == ARETURN)
		if(operand >= 0 && operand < 4096 && (operand&15) == 0)
			goto stk2;
		if(as == AENTER)
		if(operand >= -4096 && operand < 0 && (operand&15) == 0) {
			/*
			 * stack encoded 2 byte parcel
			 */
		stk2:
			n = (o->decod[0] - '0') | (2 << 10);
			n |= ((operand >> 4) & 0xff) << 2;
			SPUT(n)
			if(debug['a'])
				Bprint(&bso, "%4lux: %.4ux\t\t%P\n", pc-INITTEXT, n, p);
			return 2;
		}
		break;
	}
	if(as == ACALL1 || as == AJMP1 || as == AJMPT1 || as == AJMPF1) {
		n = "2201"[n-'0'];
		if(n == '2')
			diag("illegal addressing mode for CALL/JMP\n%P\n", p);
	}
	n = 0x8000 | o->genop | (("fdce"[n-'0']-'a'+10) << 4);
	SPUT(n)
	SPUT(operand>>16)
	SPUT(operand)
	if(debug['a'])
		Bprint(&bso, "%4lux: %.4ux %.8lux\t%P\n", pc-INITTEXT,
			n, operand, p);
	return 6;
}

int
andasm(Prog *p, Optab *o, int flt)
{
	int sf, st, cf, ct;
	long lf, lt;
	char *c;
	int i, s;

	sf = andsize(p, &p->from);
	cf = codc1[sf];
	lf = operand;
	st = andsize(p, &p->to);
	ct = codc1[st];
	lt = operand;
	if(cf == '2' || ct == '2') {
		s = 0xc000;
		s |= o->genop << 8;
		if(flt) {
			s |= fltadr(p, sf, p->from.width) << 4;
			s |= fltadr(p, st, p->to.width);
		} else {
			s |= genadr(p, sf, p->from.width) << 4;
			s |= genadr(p, st, p->to.width);
		}
		SPUT(s)
		SPUT(lf>>16)
		SPUT(lf)
		SPUT(lt>>16)
		SPUT(lt)
		if(debug['a'])
			Bprint(&bso, "%4lux: %.4ux %.8lux %.8lux %P\n", pc-INITTEXT,
				s, lf, lt, p);
		return 10;
	}
	if(cf != '1' && ct != '1' && (c = o->decod))
	for(; *c; c += 6)
		if(c[0] == cf && c[1] == ct) {
			i = c[2] - '0';
			if(i > 9)
				i += '0' - 'a' + 10;
			s = i << 14;
			i = c[3] - '0';
			if(i > 9)
				i += '0' - 'a' + 10;
			s |= i<<10;
			if(c[4] == '>')
				s |= (lf & 0x7c) << 3;
			else
				s |= (lf & 0x1f) << 5;
			if(c[5] == '>')
				s |= (lt & 0x7c) >> 2;
			else
				s |= (lt & 0x1f);
			SPUT(s)
			if(debug['a'])
				Bprint(&bso, "%4lux: %.4ux\t\t%P\n", pc-INITTEXT, s, p);
			return 2;
		}
	s = 0x8000 | (o->genop << 8);
	if(flt) {
		s |= fltadr(p, sf, p->from.width) << 4;
		s |= fltadr(p, st, p->to.width);
	} else {
		s |= genadr(p, sf, p->from.width) << 4;
		s |= genadr(p, st, p->to.width);
	}
	SPUT(s)
	SPUT(lf)
	SPUT(lt)
	if(debug['a'])
		Bprint(&bso, "%4lux: %.4ux %.4ux %.4ux\t%P\n", pc-INITTEXT,
			s, ((int)lf) & 0xffff, ((int)lt) & 0xffff, p);
	return 6;
}

#define	XXX	0x10
char	codc3[] =
{
/*	X    B    UB   H    UH   W    F    D    E    (W)             	  */
	XXX, 0xf, 0xf, 0xf, 0xf, 0xf, XXX, XXX, XXX, 0xf,	/* immed  */
	XXX, 0x4, 0x5, 0x6, 0x7, 0xd, XXX, XXX, XXX, 0xd,	/* stack  */
	XXX, 0x0, 0x1, 0x2, 0x3, 0xc, XXX, XXX, XXX, 0xc,	/* abs    */
	XXX, 0x8, 0x9, 0xa, 0xb, 0xe, XXX, XXX, XXX, 0xe,	/* istack */
};
char	codc4[] =
{
/*	X    B    UB   H    UH   W    F    D    E    (W)             	  */
	XXX, XXX, XXX, XXX, XXX, 0xf, 0x0, 0x1, 0x2, 0xf,	/* immed  */
	XXX, XXX, XXX, XXX, XXX, 0xd, 0x4, 0x5, 0x6, 0xd,	/* stack  */
	XXX, XXX, XXX, XXX, XXX, 0xc, 0x0, 0x1, 0x2, 0xc,	/* abs    */
	XXX, XXX, XXX, XXX, XXX, 0xe, 0x8, 0x9, 0xa, 0xe,	/* istack */
};

int
genadr(Prog *p, int s, int w)
{
	int a;

	a = codc2[s] - '0';
	a *= 10;
	a += w;
	a = codc3[a];
	if(a >= 0x10)
		diag("illegal int addressing mode\n%P\n", p);
	return a;
}

int
fltadr(Prog *p, int s, int w)
{
	int a;

	a = codc2[s] - '0';
	a *= 10;
	a += w;
	a = codc4[a];
	if(a >= 0x10)
		diag("illegal float addressing mode\n%P\n", p);
	return a;
}
