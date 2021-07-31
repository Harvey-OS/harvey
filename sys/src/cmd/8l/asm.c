#include	"l.h"

#define	Dbufslop	100

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
wput(ushort w)
{
	CPUT(w);
	CPUT(w>>8);
}

void
asmb(void)
{
	Prog *p;
	long v;
	int a;
	uchar *op1;

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
			diag("phase error %lux sb %lux in %s\n", p->pc, pc, TNAME);
			pc = p->pc;
		}
		curp = p;
		asmins(p);
		if(cbc < sizeof(and))
			cflush();
		a = (andptr - and);
		if(debug['a']) {
			Bprint(&bso, pcstr, pc);
			for(op1 = and; op1 < andptr; op1++)
				Bprint(&bso, "%.2ux", *op1 & 0xff);
			Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
			Bprint(&bso, "%P\n", curp);
		}
		memmove(cbp, and, a);
		cbp += a;
		pc += a;
		cbc -= a;
	}
	cflush();
	switch(HEADTYPE) {
	default:
		diag("unknown header type %d\n", HEADTYPE);
	case 0:
		seek(cout, rnd(HEADR+textsize, 8192), 0);
		break;
	case 1:
		textsize = rnd(HEADR+textsize, 4096)-HEADR;
		seek(cout, textsize+HEADR, 0);
		break;
	case 2:
		seek(cout, HEADR+textsize, 0);
		break;
	case 3:
	case 4:
		seek(cout, HEADR+rnd(textsize, INITRND), 0);
		break;
	}

	if(debug['v'])
		Bprint(&bso, "%5.2f datblk\n", cputime());
	Bflush(&bso);
	for(v = 0; v < datsize; v += sizeof(buf)-Dbufslop) {
		if(datsize-v > sizeof(buf)-Dbufslop)
			datblk(v, sizeof(buf)-Dbufslop);
		else
			datblk(v, datsize-v);
	}

	symsize = 0;
	spsize = 0;
	lcsize = 0;
	if(!debug['s']) {
		if(debug['v'])
			Bprint(&bso, "%5.2f sym\n", cputime());
		Bflush(&bso);
		switch(HEADTYPE) {
		default:
		case 0:
			seek(cout, rnd(HEADR+textsize, 8192)+datsize, 0);
			break;
		case 1:
			seek(cout, rnd(HEADR+textsize, INITRND)+datsize, 0);
			break;
		case 2:
			seek(cout, HEADR+textsize+datsize, 0);
			break;
		case 3:
		case 4:
			debug['s'] = 1;
			break;
		}
		if(!debug['s'])
			asmsym();
		if(debug['v'])
			Bprint(&bso, "%5.2f sp\n", cputime());
		Bflush(&bso);
		if(debug['v'])
			Bprint(&bso, "%5.2f pc\n", cputime());
		Bflush(&bso);
		if(!debug['s'])
			asmlc();
		cflush();
	}
	if(debug['v'])
		Bprint(&bso, "%5.2f headr\n", cputime());
	Bflush(&bso);
	seek(cout, 0L, 0);
	switch(HEADTYPE) {
	default:
	case 0:	/* garbage */
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
		lputl(0);			/* x */
	case 1:	/* unix coff */
		/*
		 * file header
		 */
		lputl(0x0004014c);		/* 4 sections, magic */
		lputl(0);			/* unix time stamp */
		lputl(0);			/* symbol table */
		lputl(0);			/* nsyms */
		lputl(0x0003001c);		/* flags, sizeof a.out header */
		/*
		 * a.out header
		 */
		lputl(0x10b);			/* magic, version stamp */
		lputl(rnd(textsize, INITRND));	/* text sizes */
		lputl(datsize);			/* data sizes */
		lputl(bsssize);			/* bss sizes */
		lput(entryvalue());		/* va of entry */
		lputl(INITTEXT);		/* text start */
		lputl(INITDAT);			/* data start */
		/*
		 * text section header
		 */
		s8put(".text");
		lputl(HEADR);			/* pa */
		lputl(HEADR);			/* va */
		lputl(textsize);		/* text size */
		lputl(HEADR);			/* file offset */
		lputl(0);			/* relocation */
		lputl(0);			/* line numbers */
		lputl(0);			/* relocation, line numbers */
		lputl(0x20);			/* flags text only */
		/*
		 * data section header
		 */
		s8put(".data");
		lputl(INITDAT);			/* pa */
		lputl(INITDAT);			/* va */
		lputl(datsize);			/* data size */
		lputl(HEADR+textsize);		/* file offset */
		lputl(0);			/* relocation */
		lputl(0);			/* line numbers */
		lputl(0);			/* relocation, line numbers */
		lputl(0x40);			/* flags data only */
		/*
		 * bss section header
		 */
		s8put(".bss");
		lputl(INITDAT+datsize);		/* pa */
		lputl(INITDAT+datsize);		/* va */
		lputl(bsssize);			/* bss size */
		lputl(0);			/* file offset */
		lputl(0);			/* relocation */
		lputl(0);			/* line numbers */
		lputl(0);			/* relocation, line numbers */
		lputl(0x80);			/* flags bss only */
		/*
		 * comment section header
		 */
		s8put(".comment");
		lputl(0);			/* pa */
		lputl(0);			/* va */
		lputl(symsize+lcsize);		/* comment size */
		lputl(HEADR+textsize+datsize);	/* file offset */
		lputl(HEADR+textsize+datsize);	/* offset of syms */
		lputl(HEADR+textsize+datsize+symsize);/* offset of line numbers */
		lputl(0);			/* relocation, line numbers */
		lputl(0x200);			/* flags comment only */
		break;
	case 2:	/* plan9 */
		lput(4*11*11+7);		/* magic */
		lput(textsize);			/* sizes */
		lput(datsize);
		lput(bsssize);
		lput(symsize);			/* nsyms */
		lput(entryvalue());		/* va of entry */
		lput(spsize);			/* sp offsets */
		lput(lcsize);			/* line offsets */
		break;
	case 3:
		/* MS-DOS .COM */
		break;
	case 4:
		/* fake MS-DOS .EXE */
		v = rnd(HEADR+textsize, INITRND)+datsize;
		wput(0x5A4D);			/* 'MZ' */
		wput(v % 512);			/* bytes in last page */
		wput(rnd(v, 512)/512);		/* total number of pages */
		wput(0x0000);			/* number of reloc items */
		v = rnd(HEADR-(INITTEXT & 0xFFFF), 16);
		wput(v/16);			/* size of header */
		wput(0x0000);			/* minimum allocation */
		wput(0xFFFF);			/* maximum allocation */
		wput(0x0000);			/* initial ss value */
		wput(0x0100);			/* initial sp value */
		wput(0x0000);			/* complemented checksum */
		v = entryvalue();
		wput(v);			/* initial ip value (!) */
		wput(0x0000);			/* initial cs value */
		wput(0x0000);
		wput(0x0000);
		wput(0x003E);			/* reloc table offset */
		wput(0x0000);			/* overlay number */
		break;
	}
	cflush();
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
lputl(long l)
{

	CPUT(l)
	CPUT(l>>8)
	CPUT(l>>16)
	CPUT(l>>24)
}

void
s8put(char *n)
{
	char name[8];
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

	memset(buf.dbuf, 0, n+Dbufslop);
	for(p = datap; p != P; p = p->link) {
		curp = p;
		l = p->from.sym->value + p->from.offset - s;
		c = p->from.scale;
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
					diag("multiple initialization\n");
					break;
				}
		}
		switch(p->to.type) {
		case D_FCONST:
			switch(c) {
			default:
			case 4:
				fl = ieeedtof(&p->to.ieee);
				cast = (char*)&fl;
				if(debug['a'] && i == 0) {
					Bprint(&bso, pcstr, l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[fnuxi4[j]] & 0xff);
					Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
					Bprint(&bso, "%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[fnuxi4[i]];
					l++;
				}
				break;
			case 8:
				cast = (char*)&p->to.ieee;
				if(debug['a'] && i == 0) {
					Bprint(&bso, pcstr, l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[fnuxi8[j]] & 0xff);
					Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
					Bprint(&bso, "%P\n", curp);
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
				Bprint(&bso, pcstr, l+s+INITDAT);
				for(j=0; j<c; j++)
					Bprint(&bso, "%.2ux", p->to.scon[j] & 0xff);
				Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
				Bprint(&bso, "%P\n", curp);
			}
			for(; i<c; i++) {
				buf.dbuf[l] = p->to.scon[i];
				l++;
			}
			break;
		default:
			fl = p->to.offset;
			if(p->to.type == D_ADDR) {
				if(p->to.index != D_STATIC && p->to.index != D_EXTERN)
					diag("DADDR type%P\n", p);
				if(p->to.sym) {
					fl += p->to.sym->value;
					if(p->to.sym->type != STEXT)
						fl += INITDAT;
				}
			}
			cast = (char*)&fl;
			switch(c) {
			default:
				diag("bad nuxi %d %d\n%P\n", c, i, curp);
				break;
			case 1:
				if(debug['a'] && i == 0) {
					Bprint(&bso, pcstr, l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[inuxi1[j]] & 0xff);
					Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
					Bprint(&bso, "%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi1[i]];
					l++;
				}
				break;
			case 2:
				if(debug['a'] && i == 0) {
					Bprint(&bso, pcstr, l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[inuxi2[j]] & 0xff);
					Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
					Bprint(&bso, "%P\n", curp);
				}
				for(; i<c; i++) {
					buf.dbuf[l] = cast[inuxi2[i]];
					l++;
				}
				break;
			case 4:
				if(debug['a'] && i == 0) {
					Bprint(&bso, pcstr, l+s+INITDAT);
					for(j=0; j<c; j++)
						Bprint(&bso, "%.2ux", cast[inuxi4[j]] & 0xff);
					Bprint(&bso, "%.*s", (24+7-printcol)/8, "\t\t\t");
					Bprint(&bso, "%P\n", curp);
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
