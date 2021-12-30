#include	"l.h"

#define JMPSZ	sizeof(u32int)		/* size of bootstrap jump section */

#define	LPUT(c)\
	{\
		cbp[0] = (c)>>24;\
		cbp[1] = (c)>>16;\
		cbp[2] = (c)>>8;\
		cbp[3] = (c);\
		cbp += 4;\
		cbc -= 4;\
		if(cbc <= 0)\
			cflush();\
	}

#define	CPUT(c)\
	{\
		cbp[0] = (c);\
		cbp++;\
		cbc--;\
		if(cbc <= 0)\
			cflush();\
	}

void	strnput(char*, int);

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
	if(dlm && s->type == SDATA)
		return s->value+INITDAT;
	if(s->type != STEXT && s->type != SLEAF)
		diag("entry not text: %s", s->name);
	return s->value;
}

static void
elf32jmp(Putl putl)
{
	/* describe a tiny text section at end with jmp to start; see below */
	elf32phdr(putl, PT_LOAD, HEADR+textsize-JMPSZ, 0xFFFFFFFC, 0xFFFFFFFC,
		JMPSZ, JMPSZ, R|X, 0);	/* text */
}

void
asmb(void)
{
	Prog *p;
	long t;
	Optab *o;
	long prevpc;

	if(debug['v'])
		Bprint(&bso, "%5.2f asm\n", cputime());
	Bflush(&bso);

	/* emit text segment */
	seek(cout, HEADR, 0);
	prevpc = pc = INITTEXT;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT) {
			curtext = p;
			autosize = p->to.offset + 4;
			if(p->from3.type == D_CONST) {
				for(; pc < p->pc; pc++)
					CPUT(0);
			}
		}
		if(p->pc != pc) {
			diag("phase error %lux sb %lux",
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
		if (prevpc & (1<<31) && (pc & (1<<31)) == 0) {
			char *tn;

			tn = "??none??";
			if(curtext != P && curtext->from.sym != S)
				tn = curtext->from.sym->name;
			Bprint(&bso, "%s: warning: text segment wrapped past 0\n", tn);
		}
		prevpc = pc;
	}

	if(debug['a'])
		Bprint(&bso, "\n");
	Bflush(&bso);
	cflush();

	/* emit data segment */
	curtext = P;
	switch(HEADTYPE) {
	case 6:
		/*
		 * but first, for virtex 4, inject a jmp instruction after
		 * other text: branch to absolute entry address (0xfffe2100).
		 */
		lput((18 << 26) | (0x03FFFFFC & entryvalue()) | 2);
		textsize += JMPSZ;
		cflush();
		/* fall through */
	case 0:
	case 1:
	case 2:
	case 5:
		seek(cout, HEADR+textsize, 0);
		break;
	case 3:
		seek(cout, rnd(HEADR+textsize, 4), 0);
		break;
	case 4:
		seek(cout, rnd(HEADR+textsize, 4096), 0);
		break;
	}

	if(dlm){
		char buf[8];

		write(cout, buf, INITDAT-textsize);
		textsize = INITDAT;
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
		case 1:
		case 2:
		case 5:
		case 6:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		case 3:
			seek(cout, rnd(HEADR+textsize, 4)+datsize, 0);
			break;
		case 4:
			seek(cout, rnd(HEADR+textsize, 4096)+datsize, 0);
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f sp\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		if(dlm)
			asmdyn();
		if(HEADTYPE == 0 || HEADTYPE == 1)	/* round up file length for boot image */
			if((symsize+lcsize) & 1)
				CPUT(0);
		cflush();
	}
	else if(dlm){
		asmdyn();
		cflush();
	}

	/* back up and write the header */
	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	case 0:
		lput(0x1030107);		/* magic and sections */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 1:
		lput(0x4a6f7921);		/* Joy! */
		lput(0x70656666);		/* peff */
		lput(0x70777063);		/* pwpc */
		lput(1);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0x30002);			/*YY*/
		lput(0);
		lput(~0);
		lput(0);
		lput(textsize+datsize);
		lput(textsize+datsize);
		lput(textsize+datsize);
		lput(0xd0);			/* header size */
		lput(0x10400);
		lput(~0);
		lput(0);
		lput(0xc);
		lput(0xc);
		lput(0xc);
		lput(0xc0);
		lput(0x01010400);
		lput(~0);
		lput(0);
		lput(0x38);
		lput(0x38);
		lput(0x38);
		lput(0x80);
		lput(0x04040400);
		lput(0);
		lput(1);
		lput(0);
		lput(~0);
		lput(0);
		lput(~0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0);
		lput(0x3100);			/* load address */
		lput(0);
		lput(0);
		lput(0);			/* whew! */
		break;
	case 2:
		if(dlm)
			lput(0x80000000 | (4*21*21+7));		/* magic */
		else
			lput(4*21*21+7);	/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(0L);
		lput(lcsize);
		break;
	case 3:
		break;
	case 4:
		lput((0x1DFL<<16)|3L);		/* magic and sections */
		lput(time(0));			/* time and date */
		lput(rnd(HEADR+textsize, 4096)+datsize);
		lput(symsize);			/* nsyms */
		lput((0x48L<<16)|15L);		/* size of optional hdr and flags */

		lput((0413<<16)|01L);		/* magic and version */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(entryvalue());		/* va of entry */
		lput(INITTEXT);			/* va of base of text */
		lput(INITDAT);			/* va of base of data */
		lput(INITDAT);			/* address of TOC */
		lput((1L<<16)|1);		/* sn(entry) | sn(text) */
		lput((2L<<16)|1);		/* sn(data) | sn(toc) */
		lput((0L<<16)|3);		/* sn(loader) | sn(bss) */
		lput((3L<<16)|3);		/* maxalign(text) | maxalign(data) */
		lput(('1'<<24)|('L'<<16)|0);	/* type field, and reserved */
		lput(0);			/* max stack allowed */
		lput(0);			/* max data allowed */
		lput(0); lput(0); lput(0);	/* reserved */

		strnput(".text", 8);		/* text segment */
		lput(INITTEXT);			/* address */
		lput(INITTEXT);
		lput(textsize);
		lput(HEADR);
		lput(0L);
		lput(HEADR+textsize+datsize+symsize);
		lput(lcsize);			/* line number size */
		lput(0x20L);			/* flags */

		strnput(".data", 8);		/* data segment */
		lput(INITDAT);			/* address */
		lput(INITDAT);
		lput(datsize);
		lput(rnd(HEADR+textsize, 4096));/* sizes */
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0x40L);			/* flags */

		strnput(".bss", 8);		/* bss segment */
		lput(INITDAT+datsize);		/* address */
		lput(INITDAT+datsize);
		lput(bsssize);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0x80L);			/* flags */
		break;
	case 5:
		/*
		 * intended for blue/gene
		 */
		elf32(POWER, ELFDATA2MSB, 0, nil);
		break;
	case 6:
		/*
		 * intended for virtex 4 boot
		 */
		debug['S'] = 1;			/* symbol table */
		elf32(POWER, ELFDATA2MSB, 1, elf32jmp);
		break;
	}
	cflush();
}

void
strnput(char *s, int n)
{
	for(; *s; s++){
		CPUT(*s);
		n--;
	}
	for(; n > 0; n--)
		CPUT(0);
}

void
cput(long l)
{
	CPUT(l);
}

void
wput(long l)
{
	cbp[0] = l>>8;
	cbp[1] = l;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
wputl(long l)
{
	cbp[0] = l;
	cbp[1] = l>>8;
	cbp += 2;
	cbc -= 2;
	if(cbc <= 0)
		cflush();
}

void
lput(long l)
{
	LPUT(l);
}

void
lputl(long c)
{
	cbp[0] = (c);
	cbp[1] = (c)>>8;
	cbp[2] = (c)>>16;
	cbp[3] = (c)>>24;
	cbp += 4;
	cbc -= 4;
	if(cbc <= 0)
		cflush();
}

void
llput(vlong v)
{
	lput(v>>32);
	lput(v);
}

void
llputl(vlong v)
{
	lputl(v);
	lputl(v>>32);
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

	for(h=0; h<NHASH; h++)
		for(s=hash[h]; s!=S; s=s->link)
			switch(s->type) {
			case SCONST:
				putsymb(s->name, 'D', s->value, s->version);
				continue;

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
				putsymb(a->sym->name, 'z', a->aoffset, 0);
			else
			if(a->type == D_FILE1)
				putsymb(a->sym->name, 'Z', a->aoffset, 0);

		if(s->type == STEXT)
			putsymb(s->name, 'T', s->value, s->version);
		else
			putsymb(s->name, 'L', s->value, s->version);

		/* frame, auto and param after */
		putsymb(".frame", 'm', p->to.offset+4, 0);
		for(a=p->to.autom; a; a=a->link)
			if(a->type == D_AUTO)
				putsymb(a->sym->name, 'a', -a->aoffset, 0);
			else
			if(a->type == D_PARAM)
				putsymb(a->sym->name, 'p', a->aoffset, 0);
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
	LPUT(v);
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

#define	MINLC	4
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
			if(debug['V'])
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			continue;
		}
		if(debug['V'])
			Bprint(&bso, "\t\t%6ld", lcsize);
		v = (p->pc - oldpc) / MINLC;
		while(v) {
			s = 127;
			if(v < 127)
				s = v;
			CPUT(s+128);	/* 129-255 +pc */
			if(debug['V'])
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
			if(debug['V']) {
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
			if(debug['V']) {
				Bprint(&bso, " lc+%ld(%ld)\n", s, 0+s);
				Bprint(&bso, "%6lux %P\n",
					p->pc, p);
			}
		} else {
			CPUT(64-s);	/* 65-128 -lc */
			if(debug['V']) {
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
	if(debug['v'] || debug['V'])
		Bprint(&bso, "lcsize = %ld\n", lcsize);
	Bflush(&bso);
}

void
datblk(long s, long n)
{
	Prog *p;
	char *cast;
	long l, fl, j, d;
	int i, c;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		l = p->from.sym->value + p->from.offset - s;
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
		if(p->as != AINIT && p->as != ADYNT) {
			for(j=l+(c-i)-1; j>=l; j--)
				if(buf.dbuf[j]) {
					print("%P\n", p);
					diag("multiple initialization");
					break;
				}
		}
		switch(p->to.type) {
		default:
			diag("unknown mode in initialization\n%P", p);
			break;

		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(&p->to.ieee);
				cast = (char*)&fl;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i+4]];
					l++;
				}
				break;
			case 8:
				cast = (char*)&p->to.ieee;
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i]];
					l++;
				}
				break;
			}
			break;

		case D_SCONST:
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.sval[i];
				l++;
			}
			break;

		case D_CONST:
			d = p->to.offset;
			if(p->to.sym) {
				if(p->to.sym->type == SUNDEF){
					ckoff(p->to.sym, d);
					d += p->to.sym->value;
				}
				if(p->to.sym->type == STEXT ||
				   p->to.sym->type == SLEAF)
					d += p->to.sym->value;
				if(p->to.sym->type == SDATA)
					d += p->to.sym->value + INITDAT;
				if(p->to.sym->type == SBSS)
					d += p->to.sym->value + INITDAT;
				if(dlm)
					dynreloc(p->to.sym, l+s+INITDAT, 1, 0, 0);
			}
			cast = (char*)&d;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P", c, i, curp);
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
