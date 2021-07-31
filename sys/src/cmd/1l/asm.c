#include	"l.h"

short	opa[20];
short	*op;

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
	if(s->type != STEXT)
		diag("entry not text: %s", s->name);
	return s->value;
}

void
asmb(void)
{
	Prog *p;
	long v;
	int a;
	short *op1;

	if(debug['v'])
		Bprint(&bso, "%5.2f asmb\n", cputime());
	Bflush(&bso);

	seek(cout, HEADR, 0);
	pc = INITTEXT;
	curp = firstp;
	for(p = firstp; p != P; p = p->link) {
		if(p->as == ATEXT)
			curtext = p;
		if(p->pc != pc) {
			if(!debug['a'])
				print("%P\n", curp);
			diag("phase error %.4lux sb %.4lux in %s", p->pc, pc, TNAME);
			pc = p->pc;
		}
		curp = p;
		if(debug['a'])
			Bprint(&bso, "%lux:%P\n", pc, curp);
		asmins(p);
		if(cbc < sizeof(opa))
			cflush();
		for(op1 = opa; op1 < op; op1++) {
			a = *op1;
			*cbp++ = a >> 8;
			*cbp++ = a;
		}
		a = 2*(op - opa);
		pc += a;
		cbc -= a;
		if(debug['a']) {
			for(op1 = opa; op1 < op; op1++)
				if(op1 == opa)
					Bprint(&bso, "\t\t%4ux", *op1 & 0xffff);
				else
					Bprint(&bso, " %4ux", *op1 & 0xffff);
			if(op != opa)
				Bprint(&bso, "\n");
		}
	}
	cflush();
	switch(HEADTYPE) {
	case 0:	/* this is garbage */
		seek(cout, rnd(HEADR+textsize, 8192), 0);
		break;
	case 1:	/* plan9 boot data goes into text */
		seek(cout, rnd(HEADR+textsize, INITRND), 0);
		break;
	case 2:	/* plan 9 */
		seek(cout, HEADR+textsize, 0);
		break;
	case 3:	/* next boot */
		seek(cout, HEADR+rnd(textsize, INITRND), 0);
		break;
	case 4:	/* preprocess pilot */
		seek(cout, HEADR+textsize, 0);
		break;
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f datblk\n", cputime());
	Bflush(&bso);

	for(v = 0; v < datsize; v += sizeof(buf)-100) {
		if(datsize-v > sizeof(buf)-100)
			datblk(v, sizeof(buf)-100);
		else
			datblk(v, datsize-v);
	}

	symsize = 0;
	spsize = 0;
	lcsize = 0;
	relocsize = 0;

	Bflush(&bso);

	switch(HEADTYPE) {
	default:
		seek(cout, rnd(HEADR+textsize, 8192)+datsize, 0);
		break;
	case 1:	/* plan9 boot data goes into text */
		seek(cout, rnd(HEADR+textsize, INITRND)+datsize, 0);
		break;
	case 2:	/* plan 9 */
		seek(cout, HEADR+textsize+datsize, 0);
		break;
	case 3:	/* next boot */
		seek(cout, HEADR+rnd(textsize, INITRND)+datsize, 0);
		break;
	case 4:	/* preprocess pilot */
		seek(cout, HEADR+textsize+datsize, 0);
		break;
	}
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		asmsym();
	}
	Bflush(&bso);
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sp\n", cputime());
		asmsp();
	}
	Bflush(&bso);
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f pc\n", cputime());
		asmlc();
	}
	Bflush(&bso);
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f reloc\n", cputime());
		asmreloc();
	}
	cflush();

	if(debug['v'])
		Bprint(&bso, "%5.2f headr\n", cputime());
	Bflush(&bso);
	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	default:
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
		lput(0L);
		lput(0L);
		lput(0L);
		lput(0L);
		lput(~0L);			/* gp value ?? */
		break;
	case 1:	/* plan9 boot data goes into text */
		lput(0407);			/* magic */
		lput(rnd(HEADR+textsize, INITRND)-HEADR+datsize);		/* sizes */
		lput(0);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(spsize);			/* sp offsets */
		lput(lcsize);			/* line offsets */
		break;
	case 2:	/* plan 9 */
		lput(0407);			/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(spsize);			/* sp offsets */
		lput(lcsize);			/* line offsets */
		break;
	case 3:	/* next boot */
		/* header */
		lput(0xfeedfaceL);			/* magic */
		lput(6);				/* 68040 */
		lput(1);				/* more 68040 */
		lput(5);				/* file type 'boot' */
		lput(HEADTYPE);				/* number commands */
		lput(HEADR-7*4);			/* sizeof commands */
		lput(1);				/* no undefineds */
		/* command 1 text */
		lput(1);				/* command = 'segment' */
		lput(124);				/* command size */
		s16put("__TEXT");
			/* botch?? entryvalue() */
		lput(INITTEXT);				/* va of start */
		lput(rnd(textsize, 8192));		/* va size */
		lput(HEADR);				/* file offset */
		lput(rnd(textsize, 8192));		/* file size */
		lput(7);				/* max prot */
		lput(7);				/* init prot */
		lput(1);				/* number of sections */
		lput(0);				/* flags */
		/* text section */
		s16put("__text");
		s16put("__TEXT");
			/* botch?? entryvalue() */
		lput(INITTEXT);				/* va of start */
		lput(textsize);				/* va size */
		lput(HEADR);				/* file offset */
		lput(2);				/* align */
		lput(0);				/* reloff */
		lput(0);				/* nreloc */
		lput(0);				/* flags */
		lput(0);				/* reserved1 */
		lput(0);				/* reserved2 */
		/* command 1 data */
		lput(1);				/* command = 'segment' */
		lput(192);				/* command size */
		s16put("__DATA");
		lput(INITDAT);				/* va of start */
		lput(rnd(datsize, 8192));		/* va size */
		lput(HEADR+rnd(textsize, 8192));	/* file offset */
		lput(rnd(datsize, 8192));		/* file size */
		lput(7);				/* max prot */
		lput(7);				/* init prot */
		lput(2);				/* number of sections */
		lput(0);				/* flags */
		/* data section */
		s16put("__data");
		s16put("__DATA");
		lput(INITDAT);				/* va of start */
		lput(datsize);				/* va size */
		lput(HEADR+rnd(textsize, 8192));	/* file offset */
		lput(2);				/* align */
		lput(0);				/* reloff */
		lput(0);				/* nreloc */
		lput(0);				/* flags */
		lput(0);				/* reserved1 */
		lput(0);				/* reserved2 */
		/* bss section */
		s16put("__bss");
		s16put("__DATA");
		lput(INITDAT+datsize);			/* va of start */
		lput(bsssize);				/* va size */
		lput(0);				/* file offset */
		lput(2);				/* align */
		lput(0);				/* reloff */
		lput(0);				/* nreloc */
		lput(1);				/* flags = zero fill */
		lput(0);				/* reserved1 */
		lput(0);				/* reserved2 */
		/* command 2 symbol */
		lput(2);				/* command = 'symbol' */
		lput(24);				/* command size */
		lput(HEADR+rnd(textsize, INITRND)
			+datsize);			/* symoff */
		lput(symsize);				/* nsyms */
		lput(spsize);				/* sp offsets */
		lput(lcsize);				/* line offsets */
		break;
	case 4:	/* preprocess pilot */
		lput(0407);			/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(spsize);			/* sp offsets */
		lput(lcsize);			/* line offsets */
		lput(relocsize);		/* relocation */
		break;
	}
	cflush();
}

void
asmins(Prog *p)
{
	Optab *o;
	int t, a, b;
	long v;

	op = opa + 1;
	if(special[p->from.type])
	switch(p->from.type) {

	case D_CCR:
		if(p->as != AMOVW)
			goto bad;
		a = asmea(p, &p->to);
		if((a & 0170) == 010)
			goto bad;
		opa[0] = 0x42c0 | a;		/* mov from ccr */
		return;

	case D_SR:
		if(p->as != AMOVW)
			goto bad;
		a = asmea(p, &p->to);
		if((a & 0170) == 010)
			goto bad;
		opa[0] = 0x40c0 | a;		/* mov from sr */
		return;

	case D_USP:
		if(p->as != AMOVL)
			goto bad;
		a = asmea(p, &p->to);
		if((a & 0170) == 010) {
			opa[0] = 0x4e68|(a&7);	/* mov usp An */
			return;
		}
		t = 0x800;
		goto movec1;

	case D_SFC:
		t = 0x000;
		goto movec1;

	case D_DFC:
		t = 0x001;
		goto movec1;

	case D_CACR:
		t = 0x002;
		goto movec1;

	case D_TC:
		t = 0x003;
		goto movec1;

	case D_ITT0:
		t = 0x004;
		goto movec1;

	case D_ITT1:
		t = 0x005;
		goto movec1;

	case D_DTT0:
		t = 0x006;
		goto movec1;

	case D_DTT1:
		t = 0x007;
		goto movec1;

	case D_VBR:
		t = 0x801;
		goto movec1;

	case D_CAAR:
		t = 0x802;
		goto movec1;

	case D_MSP:
		t = 0x803;
		goto movec1;

	case D_ISP:
		t = 0x804;
		goto movec1;

	case D_MMUSR:
		t = 0x805;
		goto movec1;

	case D_URP:
		t = 0x806;
		goto movec1;

	case D_SRP:
		t = 0x807;
		goto movec1;

	movec1:
		if(p->as != AMOVL)
			goto bad;
		opa[0] = 0x4e7a;			/* mov spc Dn */
		a = asmea(p, &p->to);
		b = a & 0170;
		if(b == 0 || b == 010) {
			*op++ = (a<<12) | t;
			return;
		}
		goto bad;

	case D_FPCR:
		t = 0xb000;
		goto movec3;

	case D_FPSR:
		t = 0xa800;
		goto movec3;

	case D_FPIAR:
		t = 0xa400;

	movec3:
		if(p->as != AMOVL)
			goto bad;
		op++;
		a = asmea(p, &p->to);
		opa[0] = optab[AFMOVEL].opcode0 | a;
		opa[1] = t;
		return;
	}
	if(special[p->to.type])
	switch(p->to.type) {

	case D_CCR:
		if(p->as != AMOVW)		/* botch, needs and, eor etc. */
			goto bad;
		a = asmea(p, &p->from);
		if((a & 0170) == 010)
			goto bad;
		opa[0] = 0x44c0 | a;		/* mov to ccr */
		return;

	case D_SR:
		if(p->as != AMOVW)		/* botch, needs and, eor etc. */
			goto bad;
		a = asmea(p, &p->from);
		if((a & 0170) == 010)
			goto bad;
		opa[0] = 0x46c0 | a;		/* mov to sr */
		return;

	case D_USP:
		if(p->as != AMOVL)
			goto bad;
		a = asmea(p, &p->from);
		if((a & 0170) == 010) {
			opa[0] = 0x4e60|(a&7);	/* mov An usp */
			return;
		}
		t = 0x800;
		goto movec2;

	case D_SFC:
		t = 0x000;
		goto movec2;

	case D_DFC:
		t = 0x001;
		goto movec2;

	case D_CACR:
		t = 0x002;
		goto movec2;

	case D_TC:
		t = 0x003;
		goto movec2;

	case D_ITT0:
		t = 0x004;
		goto movec2;

	case D_ITT1:
		t = 0x005;
		goto movec2;

	case D_DTT0:
		t = 0x006;
		goto movec2;

	case D_DTT1:
		t = 0x007;
		goto movec2;

	case D_VBR:
		t = 0x801;
		goto movec2;

	case D_CAAR:
		t = 0x802;
		goto movec2;

	case D_MSP:
		t = 0x803;
		goto movec2;

	case D_ISP:
		t = 0x804;
		goto movec2;

	case D_MMUSR:
		t = 0x805;
		goto movec2;

	case D_URP:
		t = 0x806;
		goto movec2;

	case D_SRP:
		t = 0x807;
		goto movec2;

	movec2:
		if(p->as != AMOVL)
			goto bad;
		opa[0] = 0x4e7b;			/* mov Dn spc */
		a = asmea(p, &p->from);
		b = a & 0170;
		if(b == 0 || b == 010) {
			*op++ = (a<<12) | t;
			return;
		}
		goto bad;

	case D_FPCR:
		t = 0x9000;
		goto movec4;

	case D_FPSR:
		t = 0x8800;
		goto movec4;

	case D_FPIAR:
		t = 0x8400;

	movec4:
		if(p->as != AMOVL)
			goto bad;
		op++;
		a = asmea(p, &p->from);
		opa[0] = optab[AFMOVEL].opcode0 | a;
		opa[1] = t;
		return;
	}

	o = &optab[p->as];
	t = o->opcode0;
	switch(o->optype) {
	case 0:		/* pseudo ops */
		if(p->as != ATEXT && p->as != ANOP) {
			if(!debug['a'])
				print("%P\n", p);
			diag("unimplemented instruction in %s", TNAME);
			return;
		}
		op = opa;
		return;

	case 1:		/* branches */
		if(p->to.type != D_BRANCH)
			goto bad;
		a = asmea(p, &p->to);
		if(a == 071)
			t = o->opcode1;
		t |= a;
		break;

	case 2:		/* move */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((a & 0170) == 0110) { /* src quick */
			t = o->opcode1;
			if((b & 0170) != 0)
				goto bad;
			t |= a >> 7;
			t |= b << 9;
			break;
		}
		t |= a;
		t |= (b&7) << 9;
		t |= (b&070) << 3;
		break;

	case 3:		/* add */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((a & 0170) == 0110) { /* src quick */
			t = o->opcode1;
			t |= (a&01600) << 2;
			t |= b;
			break;
		}
		if((b & 0170) == 0) { /* dst Dn */
			t |= a;
			t |= (b & 7) << 9;
			break;
		}
		if((b & 0170) == 010) { /* dst An */
			if((t & 0xc0) == 0)
				goto bad;
			t = o->opcode2;
			t |= a;						
			t |= (b & 7) << 9;
			break;
		}
		if((a & 0170) == 0) { /* src Dn */
			t |= 0x100;
			t |= (a & 7) << 9;
			t |= b;
			break;
		}
		if((a & 0177) == 074) { /* src immed */
			t = o->opcode3;
			t |= b;
			break;
		}
		goto bad;

	case 4:		/* no operands */
		break;

	case 5:		/* tst */
		t |= asmea(p, &p->to);
		if((t&0170) == 010)
			goto bad;
		break;

	case 6:		/* lea */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((b & 0170) != 010)
			goto bad;
		t |= a;
		t |= (b & 7) << 9;
		break;

	case 7:		/* cmp */
		b = asmea(p, &p->to);
		a = asmea(p, &p->from);
		if((a & 0170) == 010) {	/* dst An */
			t = o->opcode1;
			if(t == 0)	/* cmpb illegal */
				goto bad;
			t |= 0xc0;
			t |= b;						
			t |= (a & 7) << 9;
			break;
		}
		if((b & 0177) == 074) { /* src immed */
			t = o->opcode2;
			t |= a;
			break;
		}
		if((a & 0170) == 0) {	/* dst Dn */
			t |= b;
			t |= (a&7) << 9;
			break;
		}
		if((b&0170) == 030 && (a&0170) == 030) { /* (A)+,(A)+ */
			t = o->opcode3;
			t |= b & 7;
			t |= (a & 7) << 9;
			break;
		}
		goto bad;

	case 8:		/* svc */
		*op++ = optab[ARTS].opcode0;
		break;

	case 9:		/* and */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((a & 0170) == 010)
			goto bad;
		if((b & 0170) == 0) { /* dst Dn */
			t |= a;
			t |= (b&7) << 9;
			break;
		}
		if((a & 0170) == 0) { /* src Dn */
			t = o->opcode1;
			t |= b;
			t |= (a&7) << 9;
			break;
		}
		if((a & 0177) == 074) { /* src immed */
			t = o->opcode2;
			t |= b;
			break;
		}
		goto bad;

	case 10:	/* eor */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((a & 0170) == 010)
			goto bad;
		if((a & 0170) == 0) { /* src Dn */
			t |= b;
			t |= (a&7) << 9;
			break;
		}
		if((a & 0177) == 074) { /* src immed */
			t = o->opcode1;
			t |= b;
			break;
		}
		goto bad;

	case 11:	/* ext */
		b = asmea(p, &p->to);
		if((b & 0170) == 0) { /* dst Dn */
			t |= b;
			break;
		}
		goto bad;

	case 12:	/* shift */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((b & 0170) == 0) { /* dst Dn */
			if((a & 0177) == 0110) { /* src quick */
				t |= (a & 01600) << 2;
				t |= b;
				break;
			}
			if((a & 0170) == 0) { /* src Dn */
				t |= 0x20;
				t |= a << 9;
				t |= b;
				break;
			}
			goto bad;
		}
		goto bad;

	case 13:	/* mul, div short */
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((b & 0170) == 0) { /* dst Dn */
			if((a & 0170) == 010)
				goto bad;	
			t |= a;
			t |= b << 9;
			break;
		}
		goto bad;

	case 14:	/* mul, div long */
		*op++ = o->opcode1;
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((b & 0170) == 0) { /* dst Dn */
			if((a & 0170) == 010)
				goto bad;	
			t |= a;
			opa[1] |= b << 12;
			opa[1] |= b+1;
			break;
		}
		goto bad;

	case 15:	/* dec and branch */
		if(p->to.type != D_BRANCH)
			goto bad;
		v = p->pcond->pc - p->pc - 2;
		if(v < -32768L || v >= 32768L)
			goto bad;
		*op++ = v;
		a = asmea(p, &p->from);
		if((a & 0170) != 0)
			goto bad;
		t |= a;
		break;

	case 16:	/* fmove */
		*op++ = o->opcode1;
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((a & 0170) == 0100) { /* src Fn */
			if((b & 0170) == 0100) { /* both Fn */
				opa[1] |= (a&7) << 10;
				opa[1] |= (b&7) << 7;
				break;
			}
			t |= b;
			opa[1] = o->opcode2;
			opa[1] |= (a&7) << 7;
			break;
		}
		if((b & 0170) != 0100) /* dst Fn */
			goto bad;
		t |= a;
		opa[1] = o->opcode3;
		opa[1] |= (b&7) << 7;
		break;

	case 17:	/* floating ea,Fn */
		*op++ = o->opcode1;
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((b & 0170) != 0100) /* dst Fn */
			goto bad;
		if((a & 0170) == 0100) { /* both Fn */
			opa[1] |= (a&7) << 10;
			opa[1] |= (b&7) << 7;
			break;
		}
		t |= a;
		opa[1] = o->opcode2;
		opa[1] |= (b&7) << 7;
		break;

	case 18:	/* floating branchs */
		if(p->to.type != D_BRANCH)
			goto bad;
		v = p->pcond->pc - p->pc - 2;
		if(v < -32768L || v >= 32768L)
			goto bad;
		*op++ = v;
		break;

	case 19:	/* floating dec and branch */
		if(p->to.type != D_BRANCH)
			goto bad;
		*op++ = o->opcode1;
		v = p->pcond->pc - p->pc - 2;
		if(v < -32768L || v >= 32768L)
			goto bad;
		*op++ = v;
		a = asmea(p, &p->from);
		if((a & 0170) != 0)
			goto bad;
		t |= a;
		break;

	case 20:	/* ftst ea */
		*op++ = o->opcode1;
		if(p->from.type != D_NONE)
			goto bad;
		a = asmea(p, &p->to);
		if((a & 0170) == 0100) { /* Fn */
			opa[1] |= (a&7) << 10;
			break;
		}
		t |= a;
		opa[1] = o->opcode2;
		break;

	case 21:	/* fneg */
		*op++ = o->opcode1;
		if(p->from.type == D_NONE) {
			b = asmea(p, &p->to);
			a = b;
		} else {
			a = asmea(p, &p->from);
			b = asmea(p, &p->to);
		}
		if((b & 0170) != 0100) /* dst Fn */
			goto bad;
		if((a & 0170) == 0100) { /* both Fn */
			opa[1] |= (a&7) << 10;
			opa[1] |= (b&7) << 7;
			break;
		}
		t |= a;
		opa[1] = o->opcode2;
		opa[1] |= (b&7) << 7;
		break;

	case 22:	/* floating cmp Fn,ea */
		*op++ = o->opcode1;
		a = asmea(p, &p->from);
		b = asmea(p, &p->to);
		if((a & 0170) != 0100) /* dst Fn */
			goto bad;
		if((b & 0170) == 0100) { /* both Fn */
			opa[1] |= (b&7) << 10;
			opa[1] |= (a&7) << 7;
			break;
		}
		t |= b;
		opa[1] = o->opcode2;
		opa[1] |= (a&7) << 7;
		break;

	case 23:	/* word, long */
		op = opa;
		a = asmea(p, &p->to);
		if(a == ((7<<3)|4))
			return;
		if(a == ((7<<3)|1)) {
			if(p->as == AWORD) {
				op = opa;
				*op++ = opa[1];
			}
			return;
		}
		if(a == ((7<<3)|0)) {
			if(p->as == ALONG) {
				*op++ = opa[0];
				opa[0] = 0;
			}
			return;
		}
		goto bad;

	case 24:	/* bit field */
		a = ((p->to.field&31)<<6) | (p->from.field&31);
		if(p->as == ABFINS) {
			b = asmea(p, &p->from);
			if((b&0170) != 0)
				goto bad;
			a |= b<<12;
			*op++ = a;
			a = asmea(p, &p->to);
		} else {
			if(p->to.type != D_NONE) {
				b = asmea(p, &p->to);
				if((b&0170) != 0)
					goto bad;
				a |= b<<12;
			}
			*op++ = a;
			a = asmea(p, &p->from);
		}
		t |= a;
		a &= 0170;
		if(a == 010 || a == 030 || a == 040 || a == 074)
			goto bad;
		break;

	case 25:	/* movem */
		if(p->from.type == D_CONST) { /* registers -> memory */
			asmea(p, &p->from);
			a = asmea(p, &p->to);
			if(a == 074)
				goto bad;
			b = a & 0170;
			if(b == 000 || b == 010 || b == 030)
				goto bad;
			t |= a;
			break;
		}
		if(p->to.type == D_CONST) { /* memory -> registers */
			t |= 0x400;
			asmea(p, &p->to);
			a = asmea(p, &p->from);
			if(a == 074)
				goto bad;
			b = a & 0170;
			if(b == 000 || b == 010 || b == 040)
				goto bad;
			t |= a;
			break;
		}
		goto bad;

	case 26:	/* chk */
		a = asmea(p, &p->from);
		if((a&0170) == 010)
			goto bad;
		b = asmea(p, &p->to);
		if((b&0170) != 0)
			goto bad;
		t |= a;
		t |= b<<9;
		break;

	case 27:	/* btst */
		a = asmea(p, &p->from);
		if(a == 074) {
			t = o->opcode1;
		} else
		if((a&0170) != 0)
			goto bad;
		b = asmea(p, &p->to);
		if(b == 074 || (b&0170) == 010)
			goto bad;
		t |= b;
		break;

	case 28:	/* fmovem */
		if(p->from.type == D_CONST) { /* registers -> memory */
			b = p->from.offset & 0xff;
			b |= 0xf000;		/* control or postinc */
			*op++ = b;
			a = asmea(p, &p->to);
			if(a == 074)
				goto bad;
			b = a & 0170;
			if(b == 000 || b == 010 || b == 030)
				goto bad;
			if(b == 040)
				op[-1] &= ~0x1000;	/* predec */
			t |= a;
			break;
		}
		if(p->to.type == D_CONST) { /* memory -> registers */
			b = p->to.offset & 0xff;
			b |= 0xd000;		/* control or postinc */
			*op++ = b;
			a = asmea(p, &p->from);
			if(a == 074)
				goto bad;
			b = a & 0170;
			if(b == 000 || b == 010 || b == 040)
				goto bad;
			t |= a;
			break;
		}
		goto bad;

	case 29:	/* fmovemc */
		if(p->from.type == D_CONST) { /* registers -> memory */
			b = (p->from.offset & 0x7) << 10;
			b |= 0xa000;
			*op++ = b;
			a = asmea(p, &p->to);
			if(a == 074)
				goto bad;
			b = a & 0170;
			if(b == 000 || b == 010 || b == 030)
				goto bad;
			t |= a;
			break;
		}
		if(p->to.type == D_CONST) { /* memory -> registers */
			b = (p->to.offset & 0x7) << 10;
			b |= 0x8000;
			*op++ = b;
			a = asmea(p, &p->from);
			if(a == 074)
				goto bad;
			b = a & 0170;
			if(b == 000 || b == 010 || b == 040)
				goto bad;
			t |= a;
			break;
		}
		goto bad;

	case 30:	/* trap */
		if(p->to.type == D_CONST) {
			t |= p->to.offset & 0xf;
			break;
		}
		goto bad;

	case 31:	/* chk2, cmp2 */
		b = asmea(p, &p->to);
		a = b & 0170;
		if(a == 000 || a == 010) {
			*op++ = o->opcode1 | (b << 12);
			t |= asmea(p, &p->from);
			break;
		}
		goto bad;

	case 32:	/* casew */
		/* jmp (0,pc,r0.w*1) */
		casepc = p->pc;
		*op++ = o->opcode1;
		break;

	case 34:	/* moves */
		op++;
		a = asmea(p, &p->from);
		b = a & 0170;
		if(b == 0 || b == 010) {
			opa[1] = (a << 12) | 0x800;
			b = asmea(p, &p->to);
			a = b & 0170;
			if(a == 0 || a == 010)
				goto bad;
			t |= b;
			break;
		}
		t |= a;
		b = asmea(p, &p->to);
		a = b & 0170;
		if(a != 0 && a != 010)
			goto bad;
		opa[1] = (b << 12);
		break;

	case 35:	/* swap */
		a = asmea(p, &p->to);
		if((a & 0170) == 0) {
			t |= a;
			break;
		}
		goto bad;
	}
	opa[0] = t;
	return;

bad:
	if(!debug['a'])
		print("%P\n", p);
	diag("bad combination of addressing in %s", TNAME);
	opa[0] = 0;
}

int
asmea(Prog *p, Adr *a)
{
	Optab *o;
	int f, t, r, i;
	long v;

	t = a->type;
	r = simple[t];
	v = a->offset;
	if(r != 0177) {
		if(v == 0)
			return r;
		if((r & 070) != 020)
			return r;
		if(v >= -32768L && v < 32768L) {
			*op++ = v;
			return t-D_A0-I_INDIR+050;	/* d(Ax) */
		}
		goto toobig;
	}
	f = 0;
	if(a == &p->from)
		f++;
	o = &optab[p->as];
	switch(t) {
	case D_TOS:
		if(f) {
			if(o->srcsp)
				return (3<<3) | 7;	/* (A7)+ */
		} else
			if(o->dstsp)
				return (4<<3) | 7;	/* -(A7) */
		return (2<<3) | 7;			/* (A7) */
		
	case D_BRANCH:
		v = p->pcond->pc - p->pc - 2;
		if(v < -32768L || v >= 32768L) {
			if(p->as == ABSR || p->as == ABRA) {
				v = p->pcond->pc;
				*op++ = v>>16;
				*op++ = v;
				return (7<<3) | 1;
			}
			goto toobig;
		}
		if(v < -128 || v >= 128 || p->mark == 4) {
			*op++ = v;
			return 0;
		}
		return v & 0xff;

	case I_ADDR|D_STATIC:
	case I_ADDR|D_EXTERN:
		t = a->sym->type;
		if(t == 0 || t == SXREF) {
			diag("undefined external: %s in %s",
				a->sym->name, TNAME);
			a->sym->type = SDATA;
		}
		v = a->sym->value + a->offset;
		if(t != STEXT)
			v += INITDAT;
		if(strcmp(a->sym->name, "a6base"))
			break;

	case D_CONST:
		switch(f? o->srcsp: o->dstsp) {
		case 4:
			*op++ = v>>16;

		case 2:
			*op++ = v;
			break;

		default:
			diag("unknown srcsp asmea in %s", TNAME);
		}
		return (7<<3) | 4;

	case D_FCONST:
		r = f? o->srcsp: o->dstsp;
		for(i=0; i<r; i++)
			((char*)op)[i] = gnuxi(&a->ieee, i, r);
		op += r/2;
		return (7<<3) | 4;

	case D_QUICK:
		v = a->offset & 0xff;
		return 0110 | (v<<7);
			
	case D_STACK:
	case D_AUTO:
	case D_PARAM:
		if(v == 0)
			return (2<<3) | 7;	/* (A7) */
		if(v >= -32768L && v < 32768L) {
			*op++ = v;
			return (5<<3) | 7;	/* d(A7) */
		}
		goto toobig;

	case I_INDIR|D_CONST:
		if(v >= -32768L && v < 32768L) {
			*op++ = v;
			return (7<<3) | 0;
		}
		*op++ = v>>16;
		*op++ = v;
		return (7<<3) | 1;

	case D_STATIC:
	case D_EXTERN:
		t = a->sym->type;
		if(t == 0 || t == SXREF) {
			diag("undefined external: %s in %s",
				a->sym->name, TNAME);
			a->sym->type = SDATA;
		}
		if(t == STEXT) {
			if(HEADTYPE == 4) {
				v = a->sym->value + a->offset - p->pc - 2;
				if(v >= -32768L && v < 32768L) {
					*op++ = v;
					return (7<<3) | 2;
				}
				goto toobig;
			}
			v = a->sym->value + a->offset;
			*op++ = v>>16;
			*op++ = v;
			return (7<<3) | 1;
		}
		v = a->sym->value + a->offset - A6OFFSET;
		if(v < -32768L || v >= 32768L) {
			v += INITDAT + A6OFFSET;
			if(v >= -32768L && v < 32768L) {
				*op++ = v;
				return (7<<3) | 0;
			}
			*op++ = v>>16;
			*op++ = v;
			return (7<<3) | 1;
		}
		if(v == 0)
			return (2<<3) | 6;
		*op++ = v;
		return (5<<3) | 6;
	}
	if(!debug['a'])
		print("%P\n", p);
	diag("unknown addressing mode: %d in %s", t, TNAME);
	return 0;

toobig:
	if(!debug['a'])
		print("%P\n", p);
	diag("addressing mode >> 2^16: %d in %s", t, TNAME);
	return 0;
}

void
lput(long l)
{

	CPUT(l>>24)
	CPUT(l>>16)
	CPUT(l>>8)
	CPUT(l)
}

void
s16put(char *n)
{
	char name[16];
	int i;

	strncpy(name, n, sizeof(name));
	for(i=0; i<sizeof(name); i++)
		CPUT(name[i])
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
datblk(long s, long n)
{
	Prog *p;
	char *cast;
	long l, fl, j;
	int i, c;

	memset(buf.dbuf, 0, n+100);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		l = p->from.sym->value + p->from.offset - s;
		c = p->from.displace;
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
				diag("multiple initialization");
				break;
			}
		switch(p->to.type) {
		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(&p->to.ieee);
				cast = (char*)&fl;
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux:%P\n\t\t", l+s+INITDAT, curp);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[fnuxi8[j+4]] & 0xff);
					Bprint(&bso, "\n");
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi8[i+4]];
					l++;
				}
				break;
			case 8:
				cast = (char*)&p->to.ieee;
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%lux:%P\n\t\t", l+s+INITDAT, curp);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[fnuxi8[j]] & 0xff);
					Bprint(&bso, "\n");
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
				Bprint(&bso, "%.4lux:%P\n\t\t", l+s+INITDAT, curp);
				for(j=0; j<c; j++)
					Bprint(&bso, "%.2ux", p->to.scon[j] & 0xff);
				Bprint(&bso, "\n");
			}
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.scon[i];
				l++;
			}
			break;
		default:
			fl = p->to.offset;
			if(p->to.sym) {
				if(p->to.sym->type == STEXT)
					fl += p->to.sym->value;
				if(p->to.sym->type == SDATA)
					fl += p->to.sym->value + INITDAT;
				if(p->to.sym->type == SBSS)
					fl += p->to.sym->value + INITDAT;
			}

			cast = (char*)&fl;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P", c, i, curp);
				break;
			case 1:
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%.4lux:%P\n\t\t", l+s+INITDAT, curp);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux",cast[inuxi1[j]] & 0xff);
					Bprint(&bso, "\n");
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%.4lux:%P\n\t\t", l+s+INITDAT, curp);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux",cast[inuxi2[j]] & 0xff);
					Bprint(&bso, "\n");
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
				if(debug['a'] && i == 0) {
					Bprint(&bso, "%.4lux:%P\n\t\t", l+s+INITDAT, curp);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux",cast[inuxi4[j]] & 0xff);
					Bprint(&bso, "\n");
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

void
asmreloc(void)
{
	Prog *p;
	Sym *s1, *s2;
	int c1, c2, c3;
	long v;

	if(HEADTYPE != 4)
		return;
	for(p = datap; p != P; p = p->link) {
		curp = p;
		s1 = p->to.sym;
		if(s1 == S)
			continue;
		c1 = 'D';
		c3 = p->from.displace;
		s2 = p->from.sym;
		v = s2->value + INITDAT;
		switch(s1->type) {
		default:
			diag("unknown reloc %d", s1->type);
			continue;
		case STEXT:
			c2 = 'T';
			break;

		case SDATA:
			c2 = 'D';
			break;

		case SBSS:
			c2 = 'B';
			break;
		}
		CPUT(c1);
		CPUT(c2);
		CPUT(c3);
		lput(v);
		if(debug['a'])
			Bprint(&bso, "r %c%c%d %.8lux %s $%s\n",
				c1, c2, c3, v, s2->name, s1->name);
		relocsize += 7;
	}
}

int
gnuxi(Ieee *d, int i, int c)
{
	char *p;
	long l;

	switch(c) {
	default:
		diag("bad nuxi %d %d\n%P", c, i, curp);
		return 0;
	
		/*
		 * 2301 vax
		 * 0123 68k
		 */
	case 4:
		l = ieeedtof(d);
		p = (char*)&l;
		i = gnuxi8[i+4];
		break;

		/*
		 * 67452301 vax
		 * 45670123 68k
		 */
	case 8:
		p = (char*)d;
		i = gnuxi8[i];
		break;
	}
	return p[i];
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
