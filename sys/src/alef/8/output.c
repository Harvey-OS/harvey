#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"
#include "y.tab.h"

static int slot;

char	*itab[] =
{
	"XXX",
	"AAA",
	"AAD",
	"AAM",
	"AAS",
	"ADCB",
	"ADCL",
	"ADCW",
	"ADDB",
	"ADDL",
	"ADDW",
	"ADJSP",
	"ANDB",
	"ANDL",
	"ANDW",
	"ARPL",
	"BOUNDL",
	"BOUNDW",
	"BSFL",
	"BSFW",
	"BSRL",
	"BSRW",
	"BTL",
	"BTW",
	"BTCL",
	"BTCW",
	"BTRL",
	"BTRW",
	"BTSL",
	"BTSW",
	"BYTE",
	"CALL",
	"CLC",
	"CLD",
	"CLI",
	"CLTS",
	"CMC",
	"CMPB",
	"CMPL",
	"CMPW",
	"CMPSB",
	"CMPSL",
	"CMPSW",
	"DAA",
	"DAS",
	"DATA",
	"DECB",
	"DECL",
	"DECW",
	"DIVB",
	"DIVL",
	"DIVW",
	"ENTER",
	"GLOBL",
	"GOK",
	"HISTORY",
	"HLT",
	"IDIVB",
	"IDIVL",
	"IDIVW",
	"IMULB",
	"IMULL",
	"IMULW",
	"INB",
	"INL",
	"INW",
	"INCB",
	"INCL",
	"INCW",
	"INSB",
	"INSL",
	"INSW",
	"INT",
	"INTO",
	"IRETL",
	"IRETW",
	"JCC",
	"JCS",
	"JCXZ",
	"JEQ",
	"JGE",
	"JGT",
	"JHI",
	"JLE",
	"JLS",
	"JLT",
	"JMI",
	"JMP",
	"JNE",
	"JOC",
	"JOS",
	"JPC",
	"JPL",
	"JPS",
	"LAHF",
	"LARL",
	"LARW",
	"LEAL",
	"LEAW",
	"LEAVEL",
	"LEAVEW",
	"LOCK",
	"LODSB",
	"LODSL",
	"LODSW",
	"LONG",
	"LOOP",
	"LOOPEQ",
	"LOOPNE",
	"LSLL",
	"LSLW",
	"MOVB",
	"MOVL",
	"MOVW",
	"MOVBLSX",
	"MOVBLZX",
	"MOVBWSX",
	"MOVBWZX",
	"MOVWLSX",
	"MOVWLZX",
	"MOVSB",
	"MOVSL",
	"MOVSW",
	"MULB",
	"MULL",
	"MULW",
	"NAME",
	"NEGB",
	"NEGL",
	"NEGW",
	"NOP",
	"NOTB",
	"NOTL",
	"NOTW",
	"ORB",
	"ORL",
	"ORW",
	"OUTB",
	"OUTL",
	"OUTW",
	"OUTSB",
	"OUTSL",
	"OUTSW",
	"POPAL",
	"POPAW",
	"POPFL",
	"POPFW",
	"POPL",
	"POPW",
	"PUSHAL",
	"PUSHAW",
	"PUSHFL",
	"PUSHFW",
	"PUSHL",
	"PUSHW",
	"RCLB",
	"RCLL",
	"RCLW",
	"RCRB",
	"RCRL",
	"RCRW",
	"REP",
	"REPN",
	"RET",
	"ROLB",
	"ROLL",
	"ROLW",
	"RORB",
	"RORL",
	"RORW",
	"SAHF",
	"SALB",
	"SALL",
	"SALW",
	"SARB",
	"SARL",
	"SARW",
	"SBBB",
	"SBBL",
	"SBBW",
	"SCASB",
	"SCASL",
	"SCASW",
	"SETCC",
	"SETCS",
	"SETEQ",
	"SETGE",
	"SETGT",
	"SETHI",
	"SETLE",
	"SETLS",
	"SETLT",
	"SETMI",
	"SETNE",
	"SETOC",
	"SETOS",
	"SETPC",
	"SETPL",
	"SETPS",
	"CDQ",
	"CWD",
	"SHLB",
	"SHLL",
	"SHLW",
	"SHRB",
	"SHRL",
	"SHRW",
	"STC",
	"STD",
	"STI",
	"STOSB",
	"STOSL",
	"STOSW",
	"SUBB",
	"SUBL",
	"SUBW",
	"SYSCALL",
	"TESTB",
	"TESTL",
	"TESTW",
	"TEXT",
	"VERR",
	"VERW",
	"WAIT",
	"WORD",
	"XCHGB",
	"XCHGL",
	"XCHGW",
	"XLAT",
	"XORB",
	"XORL",
	"XORW",
	"FMOVB",
	"FMOVBP",
	"FMOVD",
	"FMOVDP",
	"FMOVF",
	"FMOVFP",
	"FMOVL",
	"FMOVLP",
	"FMOVV",
	"FMOVVP",
	"FMOVW",
	"FMOVWP",
	"FMOVX",
	"FMOVXP",
	"FCOMB",
	"FCOMBP",
	"FCOMD",
	"FCOMDP",
	"FCOMDPP",
	"FCOMF",
	"FCOMFP",
	"FCOML",
	"FCOMLP",
	"FCOMW",
	"FCOMWP",
	"FUCOM",
	"FUCOMP",
	"FUCOMPP",
	"FADDDP",
	"FADDW",
	"FADDL",
	"FADDF",
	"FADDD",
	"FMULDP",
	"FMULW",
	"FMULL",
	"FMULF",
	"FMULD",
	"FSUBDP",
	"FSUBW",
	"FSUBL",
	"FSUBF",
	"FSUBD",
	"FSUBRDP",
	"FSUBRW",
	"FSUBRL",
	"FSUBRF",
	"FSUBRD",
	"FDIVDP",
	"FDIVW",
	"FDIVL",
	"FDIVF",
	"FDIVD",
	"FDIVRDP",
	"FDIVRW",
	"FDIVRL",
	"FDIVRF",
	"FDIVRD",
	"FXCHD",
	"FFREE",
	"FLDCW",
	"FLDENV",
	"FRSTOR",
	"FSAVE",
	"FSTCW",
	"FSTENV",
	"FSTSW",
	"F2XM1",
	"FABS",
	"FCHS",
	"FCLEX",
	"FCOS",
	"FDECSTP",
	"FINCSTP",
	"FINIT",
	"FLD1",
	"FLDL2E",
	"FLDL2T",
	"FLDLG2",
	"FLDLN2",
	"FLDPI",
	"FLDZ",
	"FNOP",
	"FPATAN",
	"FPREM",
	"FPREM1",
	"FPTAN",
	"FRNDINT",
	"FSCALE",
	"FSIN",
	"FSINCOS",
	"FSQRT",
	"FTST",
	"FXAM",
	"FXTRACT",
	"FYL2X",
	"FYL2XP1",
	"END",
	"DYNT",
	"INIT",
	"LAST",
};

char*	rnam[] =
{
[D_AL]		"AL",	"CL",	"DL",	"BL",	"AH",	"CH",	"DH",	"BH",
[D_AX]		"AX",	"CX",	"DX",	"BX",	"SP",	"BP",	"SI",	"DI",
[D_F0]		"F0",	"F1",	"F2",	"F3",	"F4",	"F5",	"F6",	"F7",
[D_CS]		"CS",	"SS",	"DS",	"ES",	"FS",	"GS",
[D_GDTR]	"GDTR",
[D_IDTR]	"IDTR",
[D_LDTR]	"LDTR",
[D_MSW]		"MSW",
[D_TASK]	"TASK",
[D_CR]		"CR0",	"CR1",	"CR2",	"CR3",	"CR4",	"CR5",	"CR6",	"CR7",
[D_DR]		"DR0",	"DR1",	"DR2",	"DR3",	"DR4",	"DR5",	"DR6",	"DR7",
[D_TR]		"TR0",	"TR1",	"TR2",	"TR3",	"TR4",	"TR5",	"TR6",	"TR7",
[D_NONE]	"NONE",
};

char rcmap[256] =
{
	['\0'] 'z',
	['\n'] 'n',
	['\r'] 'r',
	['\t'] 't',
	['\b'] 'b',
	['\f'] 'f',
	['\a'] 'a',
	['\v'] 'v',
	['\\'] '\\',
	['"']  '"',
};

void
vwrite(Biobuf *b, Inst *i, int sf, int st)
{
	char io[100], *bp;

	io[0] = i->op;
	io[1] = i->op>>8;
	io[2] = i->lineno;
	io[3] = i->lineno>>8;
	io[4] = i->lineno>>16;
	io[5] = i->lineno>>24;

	bp = vaddr(io+6, &i->src1, sf);
	bp = vaddr(bp, &i->dst, st);

	Bwrite(b, io, bp-io);
}

void
outhist(Biobuf *b)
{
	Hist *h;
	char *p, *q;
	Inst pg;
	int n;

	pg = zprog;
	pg.op = AHISTORY;
	for(h = hist; h != H; h = h->link) {
		p = h->name;
		while(p) {
			q = strchr(p, '/');
			if(q) {
				n = q-p;
				if(n == 0)
					n = 1;	/* leading "/" */
				q++;
			} else {
				n = strlen(p);
				q = 0;
			}
			if(n) {
				Bputc(b, ANAME);
				Bputc(b, ANAME>>8);
				Bputc(b, D_FILE);
				Bputc(b, 1);
				Bputc(b, '<');
				Bwrite(b, p, n);
				Bputc(b, 0);
			}
			p = q;
		}
		pg.lineno = h->line;
		pg.dst.ival = h->offset;
		pg.dst.type = D_NONE;
		if(h->offset)
			pg.dst.type = D_CONST;

		vwrite(b, &pg, 0, 0);
	}
}

void
sfile(char *name)
{
	int f;
	Inst *i;
	Biobuf b;

	f = create(name, OWRITE|OTRUNC, 0666);
	if(f < 0) {
		diag(ZeroN, "cannot open %s: %r", name);
		return;
	}
	Binit(&b, f, OWRITE);

	for(i = proghead; i; i = i->next) {
		if(i->dst.type == D_BRANCH)
			i->dst.ival -= i->pc;

		Bprint(&b, "%i\n", i);
	}

	Bflush(&b);
	close(f);
}

void
objfile(char *name)
{
	Inst *i;
	Biobuf b;
	Inst end;
	int f, sfrom, sto;
	
	f = create(name, OWRITE|OTRUNC, 0666);
	if(f < 0) {
		diag(ZeroN, "cannot open %s: %r", name);
		return;
	}
	Binit(&b, f, OWRITE);
	Bseek(&b, 0L, 2);

	outhist(&b);

	memset(scache, 0, sizeof(scache));

	for(i = proghead; i; i = i->next) {
		sto = 0;
		sfrom = 0;
		do{
			if(i->src1.sym)
				sfrom = vcache(&b, &i->src1);
			if(i->dst.sym)
				sto = vcache(&b, &i->dst);
		}while(sfrom == sto && sfrom != 0);

		vwrite(&b, i, sfrom, sto);
	}

	end = zprog;
	end.op = AEND;
	vwrite(&b, &end, 0, 0);
	Bflush(&b);
	close(f);
}

int
vcache(Biobuf *b, Adres *a)
{
	char t;
	Sym *s;
	Scache *c;

	s = a->sym;
	t = a->type;
	if(t == D_ADDR)
		t = a->index;

	if(s->slot) {
		c = &scache[s->slot];
		if(c->s == s && c->class == t)
			return s->slot;
	}

	slot++;
	if(slot >= NSYM)
		slot = 1;

	c = &scache[slot];
	c->s = s;
	c->class = t;

	vname(b, t, s->name, slot);

	return slot;
}

void
vname(Biobuf *b, char class, char *name, int slot)
{
	char io[5];

	io[0] = ANAME;
	io[1] = ANAME>>8;
	io[2] = class;
	io[3] = slot;

	Bwrite(b, io, 4);
	Bwrite(b, name, strlen(name)+1);
}

char*
vaddr(char *bp, Adres *a, int s)
{
	long l;
	Ieee e;
	char *n;
	int i, t;

	t = 0;
	if(a->index != D_NONE || a->scale != 0)
		t |= T_INDEX;
	if(s != 0)
		t |= T_SYM;

	switch(a->type) {
	default:
		t |= T_TYPE;
	case D_NONE:
		if(a->ival != 0)
			t |= T_OFFSET;
		break;
	case D_FCONST:
		t |= T_FCONST;
		break;
	case D_SCONST:
		t |= T_SCONST;
		break;
	}
	*bp++ = t;

	if(t & T_INDEX) {
		*bp++ = a->index;
		*bp++ = a->scale;
	}
	if(t & T_OFFSET) {
		l = a->ival;
		*bp++ = l;
		*bp++ = l>>8;
		*bp++ = l>>16;
		*bp++ = l>>24;
	}
	if(t & T_SYM)
		*bp++ = s;
	if(t & T_FCONST) {
		ieeedtod(&e, a->fval);
		l = e.l;
		*bp++ = l;
		*bp++ = l>>8;
		*bp++ = l>>16;
		*bp++ = l>>24;
		l = e.h;
		*bp++ = l;
		*bp++ = l>>8;
		*bp++ = l>>16;
		*bp++ = l>>24;
		return bp;
	}
	if(t & T_SCONST) {
		n = a->str;
		for(i=0; i<NSNAME; i++)
			*bp++ = *n++;
		return bp;
	}
	if(t & T_TYPE)
		*bp++ = a->type;

	return bp;
}

/*
 * Quoted string printer
 */
int
qconv(void *o, Fconv *f)
{
	char buf[64], *b;
	char *p;
	int i;

	p = *((char**)o);
	b = buf;
	for(i = 0; i < 8; i++) {
		if(rcmap[*p]) {
			b[0] = '\\';
			b[1] = rcmap[*p++];
			b += 2;
		}
		else
			*b++ = *p++;
	}
	*b = '\0';
	strconv(buf, f);
	return sizeof(p);
}

/*
 * register printer
 */
int
Rconv(void *o, Fconv *f)
{
	int i;

	i = *(int*)o;
	strconv(rnam[i], f);
	return sizeof(int*);
}

/*
 * Instruction printer
 */
int
iconv(void *o, Fconv *f)
{
	Inst *i;
	Adres *s, *d;
	char buf[128], *p;

	i = *((Inst **)o);
	s = &i->src1;
	d = &i->dst;
	p = itab[i->op];
	if(i->op == ADATA || i->op == AINIT || i->op == ADYNT)
		sprint(buf, "\t%s\t%a/%d,%a", p, s, s->scale, d);
	else
	if(i->dst.type == D_NONE)
		sprint(buf, "\t%s\t%a", p, s);
	else
		sprint(buf, "\t%s\t%a,%a", p, s, d);

	strconv(buf, f);
	return sizeof(i);
}

/*
 * Address syllable printer
 */
int
aconv(void *o, Fconv *f)
{
	Adres *a;
	char buf[128], tmp[32];

	a = *((Adres **)o);
	if(a->type >= D_INDIR) {
		if(a->ival != 0)
			sprint(buf, "%d(%R)", a->ival, a->type-D_INDIR);
		else
			sprint(buf, "(%R)", a->type-D_INDIR);
	}
	else
	switch(a->type) {
	default:
		if(a->ival) {
			sprint(buf, "$%d,%R", a->ival, a->type);
			break;
		}
		sprint(buf, "%R", a->type);
		break;

	case D_NONE:
		buf[0] = 0;
		break;

	case D_BRANCH:
		sprint(buf, "%d(PC)", a->ival);
		break;

	case D_SCONST:
		sprint(buf, "$\"%q\"", a->str);
		break;

	case D_STATIC:
		sprint(buf, "%s<>+%d(SB)", a->sym->name, a->ival);
		break;

	case D_EXTERN:
		sprint(buf, "%s+%d(SB)", a->sym->name, a->ival);
		break;

	case D_AUTO:
		sprint(buf, "%s+%d(SP)", a->sym->name, a->ival);
		break;

	case D_PARAM:
		if(a->sym) {
			sprint(buf, "%s+%d(FP)", a->sym->name, a->ival);
			break;
		}
		sprint(buf, "%d(FP)", a->ival);
		break;

	case D_CONST:
		sprint(buf, "$%d", a->ival);
		break;

	case D_FCONST:
		sprint(buf, "$(%g)", a->fval);
		break;

	case D_ADDR:
		a->type = a->index;
		a->index = D_NONE;
		sprint(buf, "$%a", a);
		a->index = a->type;
		a->type = D_ADDR;
		break;
	}

	if(a->type != D_ADDR && a->index != D_NONE) {
		sprint(tmp, "(%R*%d)", a->index, a->scale);
		strcat(buf, tmp);
	}

	strconv(buf, f);
	return sizeof(a);
}

typedef struct runtime Runtime;
struct runtime
{
	char	*name;
	Node	**p;
}runtime[] = {
	"ALEF_proc", 		&procnode,
	"ALEF_task", 		&tasknode,
	"ALEF_send", 		&sendnode,
	"ALEF_exit", 		&exitnode,
	"ALEF_selrecv",		&selrecv,
	"ALEF_selsend",		&selsend,
	"ALEF_doselect",	&doselect,
	"ALEF_varselect",	&varselect,
	"ALEF_pfork",		&pforknode,
	"ALEF_pexit",		&pexitnode,
	"ALEF_pdone",		&pdonenode,
	"ALEF_csnd",		&csndnode,
	"ALEF_crcv",		&crcvnode,
	"ALEF_chana",		&challocnode,
	"ALEF_chanu",		&chunallocnode,
	"malloc",    		&allocnode,
	"free",    		&unallocnode,
	"ALEF_gin",		&ginode,
	"ALEF_gou",		&gonode,
	"memmove",		&movenode,
	"ALEFcheck",		&checknode,
	0,			0,
};

void
outinit(void)
{
	Node *n;
	Type *t;
	Runtime *rp;

	fmtinstall('i', iconv);		/* Instructions */
	fmtinstall('a', aconv);		/* Addresses */
	fmtinstall('q', qconv);		/* Data */
	fmtinstall('B', Bconv);		/* Bits */
	fmtinstall('R', Rconv);		/* Registers */

	for(rp = runtime; rp->name; rp++) {
		n = an(ONAME, nil, nil);
		n->sym = enter(rp->name, Tid);

		t = builtype[TVOID];
		if(rp->p == &allocnode)
			t = at(TIND, t);
		t = at(TFUNC, t);
		t->proto = an(ONAME, an(OVARARG, nil, nil), nil);
		t->proto->sym = n->sym;
		if(rp->p == &checknode)
			t = at(TIND, t);

		n->t = t;
		n->ti = ati(t, Global);
		sucalc(n);
		*(rp->p) = n;
	}

	zprog.next = 0;
	zprog.src1.type = D_NONE;
	zprog.src1.index = D_NONE;
	zprog.dst.type = D_NONE;
	zprog.dst.index = D_NONE;
	zprog.op = AGOK;
}

void
ieeedtod(Ieee *ieee, double native)
{
	double fr, ho, f;
	int exp;

	if(native < 0) {
		ieeedtod(ieee, -native);
		ieee->h |= 0x80000000L;
		return;
	}
	if(native == 0) {
		ieee->l = 0;
		ieee->h = 0;
		return;
	}
	fr = frexp(native, &exp);
	f = 2097152L;		/* shouldnt use fp constants here */
	fr = modf(fr*f, &ho);
	ieee->h = ho;
	ieee->h &= 0xfffffL;
	ieee->h |= (exp+1022L) << 20;
	f = 65536L;
	fr = modf(fr*f, &ho);
	ieee->l = ho;
	ieee->l <<= 16;
	ieee->l |= (long)(fr*f);
}

void
init(Node *tab, Node *v)
{
	Inst *i;

	i = ai();
	i->op = AINIT;
	i->src1.scale = builtype[TIND]->size;
	mkaddr(tab, &i->src1, 0);
	mkaddr(v, &i->dst, 0);
	ilink(i);
}

void
dynt(Node *tab, Node *ind)
{
	Inst *i;

	i = ai();
	i->op = ADYNT;
	if(tab)
		mkaddr(tab, &i->src1, 0);

	mkaddr(ind, &i->dst, 0);
	ilink(i);
}

void
dupok(void)
{
	ipc->src1.scale |= DUPOK;
}
