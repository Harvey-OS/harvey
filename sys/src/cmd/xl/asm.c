#include	"l.h"

#define	CPUT(c)\
	{\
		cbp[0] = (c);\
		cbp++;\
		cbc--;\
		if(cbc <= 0)\
			cflush();\
	}

long
entryvalue(void)
{
	char *a;
	Sym *s;

	a = entry;
	if(*a >= '0' && *a <= '9')
		return atolwhex(a);
	s = lookup(a, 0);
	if(s->type == 0)
		return textstart;
	if(s->type != STEXT && s->type != SLEAF)
		diag("entry not text: %s", s->name);
	return s->value;
}

void
asmb(void)
{
	long ramsize, ramtsize, t;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);
	seek(cout, headlen, 0);
	asmtext(0);

	curtext = P;
	seek(cout, headlen+textsize, 0);
	if(debug['v'])
		Bprint(&bso, "%5.2f asm data\n", cputime());
	for(t = 0; t < datsize; t += sizeof(buf)-100) {
		if(datsize-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100, 0);
		else
			datblk(t, datsize-t, 0);
	}

	ramsize = ramend - ramstart;
	ramtsize = ramdstart - ramstart;
	if(ramend > maxram)
		diag("text too big for alloted ram: %lux > %lux\n",
			ramend, maxram);

	seek(cout, headlen+textsize+datsize, 0);
	if(debug['v'])
		Bprint(&bso, "%5.2f asm ram\n", cputime());
	asmtext(1);

	curtext = P;
	seek(cout, headlen+textsize+datsize+ramtsize, 0);
	if(debug['v'])
		Bprint(&bso, "%5.2f asm ram data\n", cputime());
	for(t = 0; t < datsize; t += sizeof(buf)-100) {
		if(datsize-t > sizeof(buf)-100)
			datblk(t, sizeof(buf)-100, 1);
		else
			datblk(t, datsize-t, 1);
	}

	symsize = 0;
	lcsize = 0;
	spsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		seek(cout, headlen+textsize+datsize+ramsize, 0);
		asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f sp\n", cputime());
		Bflush(&bso);
		asmsp();
		asmlc();
		cflush();
	}

	seek(cout, 0L, 0);
	switch(header) {
	/*
	 * coff
	 */
	case 0:
		sput(0x0166);			/* magic */
		sput(3);			/* sections */
		lput(0);			/* time stamp */
		lput(headlen+textsize+datsize);
		lput(symsize);
		sput(1);
		sput(0x0017);			/* flags */
		if(debug['e'])
			cput(0);		/* big endian */
		else
			cput(1);		/* little endian */

		strnput(".text", 8);		/* text segment */
		lput(textstart);		/* address */
		lput(textstart);
		lput(textsize);
		lput(headlen);
		lput(0);			/* relocation offset in file */
		lput(headlen+textsize+datsize+symsize);	/* line number table offset in file */
		sput(0);			/* relocation len */
		sput(lcsize);			/* line number size */
		lput(0x20);			/* flags */

		strnput(".data", 8);		/* data segment */
		lput(datstart);			/* address */
		lput(datstart);
		lput(datsize);
		lput(headlen+textsize);		/* offset in file */
		lput(0);
		lput(0);
		lput(0);
		lput(0x40);			/* flags */

		strnput(".bss", 8);		/* bss segment */
		lput(datstart+datsize);		/* address */
		lput(datstart+datsize);
		lput(bsssize);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0x80);			/* flags */
		break;
	/*
	 * start at 0 text
	 */
	case 1:
		break;
	/*
	 * normal plan 9 format
	 */
	case 2:
		lput(4*17*17+7);		/* magic */
		lput(textsize);			/* sizes */
		lput(datsize+ramsize);
		if(ramsize > bsssize)
			lput(0);
		else
			lput(bsssize-ramsize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(spsize);			/* size of sp offset table */
		lput(lcsize);
		break;
	}
	cflush();
}

void
asmtext(int isram)
{
	Prog *p;
	Optab *o;
	int inram;

	inram = 0;
	curtext = P;
	pc = textstart;
	if(isram)
		pc = ramstart;
	for(p = firstp; p != P; p = p->link){
		if(p->as == ATEXT){
			curtext = p;
			inram = p->from.sym->isram;
		}
		if(inram != isram)
			continue;
		if(p->pc != pc) {
			diag("phase error %lux sb %lux\n",
				p->pc, pc);
			if(!debug['a'])
				prasm(curp);
			pc = p->pc;
		}
		curp = p;
		o = oplook(p);	/* could probably avoid this call */
		if(asmout(p, o, 0)) {
			p = p->link;
			pc += 4;
		}
		pc += o->size;
	}
	if(debug['a'])
		Bprint(&bso, "\n");
	Bflush(&bso);
	cflush();
}

void
lput(long l)
{
	cbp[0] = l>>24;
	cbp[1] = l>>16;
	cbp[2] = l>>8;
	cbp[3] = l;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
cput(int c)
{
	CPUT(c);
}

void
sput(int s)
{
	cbp[0] = s>>8;
	cbp[1] = s;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
codeput(long c)
{
	char *cast;
	int i;

	cast = (char*)&c;
	for(i=0; i<4; i++)
		cbp[i] = cast[inuxi4[i]];
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
strnput(char *s, int n)
{
	for(; *s; s++){
		cput(*s);
		n--;
	}
	for(; n > 0; n--)
		cput(0);
}

void
cflush(void)
{
	int n;

	n = sizeof(buf.cbuf) - cbc;
	if(n)
		write(cout, buf.cbuf, n);
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

	s = lookup("etext", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);
	s = lookup("_ramtext", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);
	s = lookup("_ramsize", 0);
	if(s->type == STEXT)
		putsymb(s->name, 'T', s->value, s->version);

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SSDATA:
				putsymb(s->name, 'D', s->value, s->version);
				continue;
			case SDATA:
				putsymb(s->name, 'D', s->value+datstart, s->version);
				continue;
			case SBSS:
				putsymb(s->name, 'B', s->value+datstart, s->version);
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

		/* auto and param after */
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
putsymb(char *s, int t, long v, int ver)
{
	int i, f;

	if(t == 'f')
		s++;
	lput(v);
	if(ver)
		t += 'a' - 'A';
	CPUT(t + 0x80);			/* flag the new symbol format */
	if(t == 'z' || t == 'Z'){
		CPUT(s[0]);
		for(i=1; s[i] != 0 || s[i+1] != 0; i += 2){
			CPUT(s[i]);
			CPUT(s[i+1]);
		}
		CPUT(0);
		CPUT(0);
		i++;
	}else{
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

#define	MINLC	4
void
asmlc(void)
{
	if(debug['v'])
		Bprint(&bso, "%5.2f pc\n", cputime());
	Bflush(&bso);
	asmlc1(0);
	if(debug['v'] || debug['L'])
		Bprint(&bso, "lcsize = %ld\n", lcsize);
	if(debug['v'])
		Bprint(&bso, "%5.2f ram pc\n", cputime());
	Bflush(&bso);
/*	asmlc1(1);/**/
	while(lcsize & 1) {
		CPUT(129);
		lcsize++;
	}
	if(debug['v'] || debug['L'])
		Bprint(&bso, "lcsize = %ld\n", lcsize);
	Bflush(&bso);
}

void
asmlc1(int isram)
{
	static long oldpc, oldlc;
	Prog *p;
	long v,  s;
	int inram;

	if(!isram){
		oldpc = textstart;
		oldlc = 0;
	}
	inram = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->line == oldlc || p->as == ATEXT || p->as == ANOP || p->as == ADOEND) {
			if(p->as == ATEXT){
				curtext = p;
				inram = p->from.sym->isram;
			}
			if(debug['L'])
				Bprint(&bso, "%6lux %P\n", p->pc, p);
			continue;
		}
		if(inram != isram)
			continue;
		if(debug['L'])
			Bprint(&bso, "\t\t%6ld", lcsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			if(v < 0 || v > 4*126){
				CPUT(255);
				v = p->pc;
				CPUT(v>>24);
				CPUT(v>>16);
				CPUT(v>>8);
				CPUT(v);
				lcsize += 5;
				break;
			}
			s = 126;
			if(v < 126)
				s = v;
			CPUT(s+128);	/* 129-254 +pc */
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
}

void
asmsp(void)
{
	asmsp1(0);
/*	asmsp1(1);/**/
	while(spsize & 1) {
		CPUT(129);
		spsize++;
	}
	if(debug['v'] || debug['G'])
		Bprint(&bso, "stsize = %ld\n", spsize);
	Bflush(&bso);
}

void
asmsp1(int isram)
{
	static long oldpc, oldsp;
	Prog *p;
	long v;
	int inram, s;

	if(!isram){
		oldpc = textstart;
		oldsp = 0;
	}
	inram = 0;
	for(p = firstp; p != P; p = p->link) {
		if(p->stkoff == oldsp || p->as == ATEXT || p->as == ANOP || p->as == ADOEND) {
			if(p->as == ATEXT){
				curtext = p;
				inram = p->from.sym->isram;
			}
			if(debug['G'])
				Bprint(&bso, "%6lux %4ld%P\n", p->pc, p->stkoff, p);
			continue;
		}
		if(inram != isram)
			continue;
		if((p->as == ACALL || p->as == AJMP || p->as == ABRA)
		&& p->link
		&& p->link->stkoff == oldsp){
			p->stkoff = oldsp;
			if(debug['G'])
				Bprint(&bso, "%6lux %4ld%P\n",
					p->pc, p->stkoff, p);
			continue;
		}
		if(debug['G'])
			Bprint(&bso, "\t\t%6ld", spsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			if(v < 0 || v > 4*126){
				CPUT(255);
				v = p->pc - MINLC;
				CPUT(v>>24);
				CPUT(v>>16);
				CPUT(v>>8);
				CPUT(v);
				spsize += 5;
				break;
			}
			s = 126;
			if(v < 126)
				s = v;
			CPUT(s+128);	/* 129-254 +pc */
			if(debug['G'])
				Bprint(&bso, " pc+%d*%d(%d)", s, MINLC, s+128);
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
}

void
datblk(long s, long n, int small)
{
	Prog *p;
	char *cast;
	long l, j, d, idat;
	int i, c, t;

	idat = datstart;
	if(small)
		idat = ramdstart;
	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		t = p->from.sym->type;
		if(small && t != SSDATA || !small && t != SDATA)
			continue;
		l = p->from.sym->value + p->from.offset - s;
		if(small)
			l -= idat;
		c = p->reg;
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
				print("%P\n", p);
				diag("multiple initialization\n");
				break;
			}

		switch(p->to.type) {
		default:
			diag("unknown mode in initialization\n%P\n", p);
			break;

		case D_FCONST:
			if(debug['d'])
				Bprint(&bso, "%.8lux: %.8lux\t%P\n",
					p->from.sym->value+idat, p->to.dsp, p);
			cast = (char*)&p->to.dsp;
			for(; i<c; i++) {
				buf.dbuf[l] = cast[fnuxi4[i]];
				l++;
			}
			break;

		case D_SCONST:
			if(debug['d'])
				Bprint(&bso, "%.8lux: %S\t%P\n",
					p->from.sym->value+idat, p->to.sval, p);
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.sval[i];
				l++;
			}
			break;

		case D_CONST:
			d = p->to.offset;
			if(p->to.sym) {
				switch(p->to.sym->type){
				case STEXT:
				case SLEAF:
					d += p->to.sym->value;
					break;
				case SDATA:
				case SBSS:
					d += p->to.sym->value + datstart;
					break;
				case SSDATA:
					d += p->to.sym->value;
					break;
				}
			}
			if(debug['d'])
				Bprint(&bso, "%.8lux: %.8lux\t%P\n",
					p->from.sym->value+idat, d, p);
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P\n", c, i, curp);
				break;
			case 1:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
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
