typedef	long	Type;

/*
 * Little-endian byte and bit order of bbc.h.
 * See comment there for general explanation.
 * Differences: 'left' and 'right' shifts in the abstract
 * machine are in screen direction, so turn into right and left shifts her;
 * any 'offset' arguments are here taken to mean offset from the low order
 * end of the word.
 */
enum
{
	field,		/* arg = field mask */
	lsha_RsRt,
	lshb_RsRt,
	lsh_RsRd,	/* arg = shift amount */
	lsh_RtRt,	/* arg = shift amount */
	lsha_RtRt,
	lsha_RtRu,
	lshb_RtRu,
	rsha_RsRt,
	rshb_RsRt,
	rsha_RtRu,
	rshb_RtRu,
	orlsha_RsRt,
	orlshb_RsRt,
	orlsh_RsRd,	/* arg = shift amount */
	orrsha_RsRt,
	orrshb_RsRt,
	orrsha_RtRu,
	orrshb_RtRu,
	or_RsRd,
	add_As,		/* arg = add amount */
	add_Ad,		/* arg = add amount */
	initsd,		/* arg1 = value for As; arg2 = value for Ad */
	ilabel,		/* arg = inner loop count value for Ri */
	olabel,		/* arg = outer loop count value for Ro */
	iloop,		/* arg = pointer to beginning of inner loop */
	oloop,		/* arg = pointer to beginning of outer loop */
	rts,
	load_Rd_P,
	load_Rs_P,
	load_Rt_P,
	load_Ru_P,
	load_Rd_D,
	load_Rs_D,
	load_Rt_D,
	load_Rd,
	load_Rs,
	load_Rt,
	fetch_Rd_P,
	fetch_Rd_D,
	fetch_Rd,
	store_Rs_P,
	store_Rs_D,
	store_Rs,
	inittab,	/* arg1 = table addr; arg2 = entry size (bytes) */
	initsh,		/* arg1 = shift amount a, arg2 = shift amount b */
	table_RdRt,	/* arg1 = offset, arg2 = nbits */
	table_RsRt,	/* arg1 = offset, arg2 = nbits */
	assemble,	/* arg1 = offset, arg2 = nbits */
	Ozero,
	ODnorS,
	ODandnotS,
	OnotS,
	OnotDandS,
	OnotD,
	ODxorS,
	ODnandS,
	ODandS,
	ODxnorS,
	OD,
	ODornotS,
	OnotDorS,
	ODorS,
	OF,
};

#define Ofield(c)	*p++ = field; *p++ = (c)
#define Olsha_RsRt	*p++ = lsha_RsRt
#define Olshb_RsRt	*p++ = lshb_RsRt
#define Olsh_RsRd(c)	*p++ = lsh_RsRd; *p++ = (c)
#define Olsh_RtRt(c)	*p++ = lsh_RtRt; *p++ = (c)
#define Olsha_RtRt	*p++ = lsha_RtRt
#define Olsha_RtRu	*p++ = lsha_RtRu
#define Olshb_RtRu	*p++ = lshb_RtRu
#define Orsha_RsRt	*p++ = rsha_RsRt
#define Orshb_RsRt	*p++ = rshb_RsRt
#define Orsha_RtRu	*p++ = rsha_RtRu
#define Orshb_RtRu	*p++ = rshb_RtRu
#define Oorlsha_RsRt	*p++ = orlsha_RsRt
#define Oorlshb_RsRt	*p++ = orlshb_RsRt
#define Oorlsh_RsRd(c)	*p++ = orlsh_RsRd; *p++ = (c)
#define Oorrsha_RsRt	*p++ = orrsha_RsRt
#define Oorrshb_RsRt	*p++ = orrshb_RsRt
#define Oorrsha_RtRu	*p++ = orrsha_RtRu
#define Oorrshb_RtRu	*p++ = orrshb_RtRu
#define Oor_RsRd	*p++ = or_RsRd
#define Add_As(c)	*p++ = add_As; *p++ = (c)
#define Add_Ad(c)	*p++ = add_Ad; *p++ = (c)
#define Initsd(s,d)	*p++ = initsd; *p++ = ((ulong)(s)); *p++ = ((ulong)(d))
#define Initsh(a,b)	*p++ = initsh; *p++ = (a); *p++ = (b)
#define Extrainit
#define Ilabel(c)	*p++ = ilabel; *p++ = (c)
#define Olabel(c)	*p++ = olabel; *p++ = (c)
#define Iloop(lp)	*p++ = iloop; *p++ = ((ulong)(lp))
#define Oloop(lp)	*p++ = oloop; *p++ = ((ulong)(lp))
#define Orts		*p++ = rts
#define Load_Rs_P	*p++ = load_Rs_P
#define Load_Rt_P	*p++ = load_Rt_P
#define Loadzx_Rt_P	*p++ = load_Rt_P
#define Loador_Rt_P	*p++ = load_Rt_P
#define Load_Ru_P	*p++ = load_Ru_P
#define Load_Rd_D(f)	*p++ = load_Rd_D
#define Load_Rs_D(f)	*p++ = load_Rs_D
#define Load_Rt_D(f)	*p++ = load_Rt_D
#define Loadzx_Rt_D(f)	*p++ = load_Rt_D
#define Load_Rd(f)	*p++ = load_Rd
#define Load_Rs(f)	*p++ = load_Rs
#define Load_Rt(f)	*p++ = load_Rt
#define Loadzx_Rt(f)	*p++ = load_Rt
#define Fetch_Rd_P(f)	*p++ = fetch_Rd_P
#define Fetch_Rd_D(f)	*p++ = fetch_Rd_D
#define Fetch_Rd(f)	*p++ = fetch_Rd
#define Store_Rs_P	*p++ = store_Rs_P
#define Store_Rs_D	*p++ = store_Rs_D
#define Store_Rs	*p++ = store_Rs
#define Nop
#define Inittab(t,s)	*p++ = inittab; *p++ = ((ulong)(t)); *p++ = (s)
#define Table_RdRt(o,n,l) *p++ = table_RdRt; *p++ = (o); *p++ = (n)
#define Table_RsRt(o,n,l) *p++ = table_RsRt; *p++ = (o); *p++ = (n)
#define Assemble(o,n)	*p++ = assemble; *p++ = (o); *p++ = (n)
#define Assemblex(o,n)	*p++ = assemble; *p++ = (o); *p++ = (n)

#define Execandfree(memstart,onstack)				\
	interpret(memstart);					\
	if(!onstack)						\
		bbfree(memstart, (p-memstart) * sizeof(Type));


/*
 * Emitop can assume that fi points at &fstr[op].instr, and
 * that fin contains fstr[op].n, where op is the desired
 * bitblt opcode as declared in gnot.h
 */

#define Emitop	if(fin) *p++ = *fi;

typedef struct	Fstr
{
	char	fetchs;
	char	fetchd;
	short	n;
	Type	instr[1];
} Fstr;

Fstr	fstr[16] =
{
[0]	0,0,1,		/* Zero */
	{Ozero},

[1]	1,1,1,		/* DnorS */
	{ODnorS},

[2]	1,1,1,		/* DandnotS */
	{ODandnotS},

[3]	1,0,1,		/* notS */
	{OnotS},

[4]	1,1,1,		/* notDandS */
	{OnotDandS},

[5]	0,1,1,		/* notD */
	{OnotD},

[6]	1,1,1,		/* DxorS */
	{ODxorS},

[7]	1,1,1,		/* DnandS */
	{ODnandS},

[8]	1,1,1,		/* DandS */
	{ODandS},

[9]	1,1,1,		/* DxnorS */
	{ODxnorS},

[10]	0,1,1,		/* D */
	{OD},

[11]	1,1,1,		/* DornotS */
	{ODornotS},

[12]	1,0,0,		/* S */
	{0},

[13]	1,1,1,		/* notDorS */
	{OnotDorS},

[14]	1,1,1,		/* DorS */
	{ODorS},

[15]	0,0,1,		/* F */
	{OF},
};

#include "tabs.h"
static uchar *tabs[4][4] =
{
	{	      0, (uchar*)tab01l, (uchar*)tab02l, (uchar*)tab03l},
	{(uchar*)tab10l,	      0, (uchar*)tab12l, (uchar*)tab13l},
	{(uchar*)tab20l, (uchar*)tab21l,	      0, (uchar*)tab23l},
	{(uchar*)tab30l, (uchar*)tab31l, (uchar*)tab32l,	      0},
};

static uchar tabosiz[4][4] = /* size in bytes of entries */
{
	{ 0, 2, 4, 4},
	{ 1, 0, 2, 4},
	{ 1, 1, 0, 2},
	{ 1, 1, 1, 0},
};

enum {
	Progmax = 800,		/* max number of words in a bitblt prog */
	Progmaxnoconv = 50,	/* max number of words when no conversion */
};

static void
interpret(Type *pc)
{
	ulong *As, *Ad;
	ulong Rs, Rd, Rt, Ru;
	long Ri, Ro;
	ulong *AT;
	int osiz, sha, shb, tmp;

#ifdef TEST
	ulong *Aslow, *Ashigh, *Adlow, *Adhigh;
	void prprog(void);

	Rs = lrand();
	Rd = lrand();
	Rt = lrand();
	Ru = lrand();
	Ri = lrand();
	Ro = lrand();
	sha = lrand();
	shb = lrand();
	As = 0;
	Ad = 0;
	AT = 0;
	Aslow = gaddr(cursm, curr.min);
	Ashigh = gaddr(cursm, sub(curr.max, Pt(1,1)));
	Adlow = gaddr(curdm, curpt);
	Adhigh = gaddr(curdm, sub(add(curpt, sub(curr.max,curr.min)),Pt(1,1)));
#endif

loop:
#ifdef TEST
	switch(*pc) {
	case load_Rd_P:
	case load_Rs_P:
	case load_Rt_P:
	case load_Ru_P:
	case load_Rd:
	case load_Rs:
	case load_Rt:
			if(As < Aslow || As > Ashigh){
				print("load from bad As %ux\n", As);
		errplace:
				print("src bitmap base %ux zero %d width %d r %d %d %d %d\n",
					cursm->base, cursm->zero, cursm->width,
					cursm->r.min.x, cursm->r.min.y,
					cursm->r.max.x, cursm->r.max.y);
				print("dst bitmap base %ux zero %d width %d r %d %d %d %d\n",
					curdm->base, curdm->zero, curdm->width,
					curdm->r.min.x, curdm->r.min.y,
					curdm->r.max.x, curdm->r.max.y);
				print("p %d %d r %d %d %d %d f %d\n",
					curpt.x, curpt.y, curr.min.x, curr.min.y,
					curr.max.x, curr.max.y, curf);
				prprog();
				exits("fail");
			}
			break;
	case load_Rd_D:
	case load_Rs_D:
	case load_Rt_D:
			if(As-1 < Aslow || As-1 > Ashigh){
				print("load from bad As-1 %ux\n", As-1);
				goto errplace;
			}
			break;
	case fetch_Rd_P:
	case fetch_Rd:
			if(Ad < Adlow || Ad > Adhigh){
				print("fetch from bad Ad %ux\n", Ad);
				goto errplace;
			}
			break;
	case store_Rs_P:
	case store_Rs:
			if(Ad < Adlow || Ad > Adhigh){
				print("store to bad Ad %ux\n", Ad);
				goto errplace;
			}
			break;
	case fetch_Rd_D:
	case store_Rs_D:
			if(Ad-1 < Adlow || Ad-1 > Adhigh){
				print("fetch from bad Ad-1 %ux\n", Ad-1);
				prprog();
			}
			break;
	}
#endif
	switch(*pc++) {
	default:
#ifdef TEST
		print("unknown opcode %d\n", pc[-1]);
		goto errplace;
#else
		return;
#endif
	case field:
		/* Rs gets Rd where mask bits are 0s, Rs where mask bits are 1s */
		Rs = ((Rs ^ Rd) & *pc++) ^ Rd;
		break;

	case lsha_RsRt:
		Rs = Rt >> sha;
		break;

	case lshb_RsRt:
		Rs = Rt >> shb;
		break;

	case lsh_RsRd:
		Rs = Rd >> *pc++;
		break;

	case lsh_RtRt:	/* arg = shift amount */
		Rt >>= *pc++;
		break;

	case lsha_RtRt:
		Rt >>= sha;
		break;

	case lsha_RtRu:
		Rt = Ru >> sha;
		break;

	case lshb_RtRu:
		Rt = Ru >> shb;
		break;

	case rsha_RsRt:
		Rs = Rt << sha;
		break;

	case rshb_RsRt:
		Rs = Rt << shb;
		break;

	case rsha_RtRu:
		Rt = Ru << sha;
		break;

	case rshb_RtRu:
		Rt = Ru << shb;
		break;

	case orlsha_RsRt:
		Rs |= Rt >> sha;
		break;

	case orlshb_RsRt:
		Rs |= Rt >> shb;
		break;

	case orlsh_RsRd:	/* arg = shift amount */
		Rs |= Rd >> *pc++;
		break;

	case orrsha_RsRt:
		Rs |= Rt << sha;
		break;

	case orrshb_RsRt:
		Rs |= Rt << shb;
		break;

	case orrsha_RtRu:
		Rt |= Ru << sha;
		break;

	case orrshb_RtRu:
		Rt |= Ru << shb;
		break;

	case or_RsRd:
		Rs |= Rd;
		break;

	case add_As:
		As = (ulong*)((char*)As + (long)*pc++);
		break;

	case add_Ad:
		Ad = (ulong*)((char*)Ad + (long)*pc++);
		break;

	case initsd:
		As = (ulong*)pc[0];
		Ad = (ulong*)pc[1];
		pc += 2;
		break;

	case ilabel:
		/* initialize inner loop count */
		Ri = *pc++;
		break;

	case olabel:
		/* initialize outer loop count */
		Ro = *pc++;
		break;

	case iloop:
		/* decrement inner loop count, loop back if still positive */
		Ri--;
		if(Ri > 0) {
			pc = (Type*)pc[0];
			break;
		}
		pc++;
		break;

	case oloop:
		/* decrement outer loop count, loop back if still positive */
		Ro--;
		if(Ro > 0) {
			pc = (Type*)pc[0];
			break;
		}
		pc++;
		break;

	case rts:
		return;

	case load_Rd_P:
		Rd = *As++;
		break;

	case load_Rs_P:
		Rs = *As++;
		break;

	case load_Rt_P:
		Rt = *As++;
		break;

	case load_Ru_P:
		Ru = *As++;
		break;

	case load_Rd_D:
		Rd = *--As;
		break;

	case load_Rs_D:
		Rs = *--As;
		break;

	case load_Rt_D:
		Rt = *--As;
		break;

	case load_Rd:
		Rd = *As;
		break;

	case load_Rs:
		Rs = *As;
		break;

	case load_Rt:
		Rt = *As;
		break;

	case fetch_Rd_P:
		Rd = *Ad++;
		break;

	case fetch_Rd_D:
		Rd = *--Ad;
		break;

	case fetch_Rd:
		Rd = *Ad;
		break;

	case store_Rs_P:
		*Ad++ = Rs;
		break;

	case store_Rs_D:
		*--Ad = Rs;
		break;

	case store_Rs:
		*Ad = Rs;
		break;

	case inittab:
		AT = (ulong*)pc[0];
		osiz = (long)pc[1];
		pc += 2;
		break;

	case initsh:
		sha = (long)pc[0];
		shb = (long)pc[1];
		pc += 2;
		break;

	case table_RdRt:
		/*
		 * Starting at offset arg1 in Rt, take arg2 bits,
		 * and use it to look up in table AT, putting answer in Rd
		 */
		Rd = (Rt >> ((long)pc[0])) & ((1<<((long)pc[1]))-1);
		Rd = AT[Rd];
		pc += 2;
		break;

	case table_RsRt:
		/* like table_RdRt, but answer goes in Rs */
		tmp = (long)pc[1];
		Rs = (Rt >> ((long)pc[0])) & ((1<<((long)pc[1]))-1);
		Rs = AT[Rs];
		pc += 2;
		break;


	case assemble:
		/*
		 * Move high arg2 bits of Rd into offset arg1 in Rs.
		 * Can assume that low bits of Rd are zero,
		 * and target field of Rs is zero if offset != 0
		 */
		tmp = (long)pc[0];
		if(tmp == 0)
			Rs = Rd >> (32-pc[1]);
		else
			Rs |= Rd >> (32-(tmp+pc[1]));
		pc += 2;
		break;
		
	case Ozero:
		Rs = 0;
		break;

	case ODnorS:
		Rs = ~(Rd|Rs);
		break;

	case ODandnotS:
		Rs = Rd & ~Rs;
		break;

	case OnotS:
		Rs = ~Rs;
		break;

	case OnotDandS:
		Rs = ~Rd & Rs;
		break;

	case OnotD:
		Rs = ~Rd;
		break;

	case ODxorS:
		Rs ^= Rd;
		break;

	case ODnandS:
		Rs = ~(Rd & Rs);
		break;

	case ODandS:
		Rs &= Rd;
		break;

	case ODxnorS:
		Rs = ~(Rd ^ Rs);
		break;

	case OD:
		Rs = Rd;
		break;

	case ODornotS:
		Rs = Rd | ~Rs;
		break;

	case OnotDorS:
		Rs |= ~Rd;
		break;

	case ODorS:
		Rs |= Rd;
		break;

	case OF:
		Rs = ~0L;
		break;
	}
	goto loop;
}

#ifdef TEST
void
prprog(void)
{
	int osiz;
	Type *pc;
	pc = (Type *)mem;

loop:
	switch(*pc++) {
	default:
		print("unknown opcode %d\n", pc[-1]);
		exits("unknown opcode");
	case field:
		print("Rs = ((Rs ^ Rd) & 0x%lux) ^ Rd\n", *pc++);
		break;

	case lsha_RsRt:
		print("Rs = Rt >> sha\n");
		break;

	case lshb_RsRt:
		print("Rs = Rt >> shb\n");
		break;

	case lsh_RsRd:
		print("Rs = Rd >> %d\n", *pc++);
		break;

	case lsh_RtRt:
		print("Rt >>= %d\n", *pc++);
		break;

	case lsha_RtRt:
		print("Rt >>= sha\n");
		break;

	case lsha_RtRu:
		print("Rt = Ru >> sha\n");
		break;

	case lshb_RtRu:
		print("Rt = Ru >> shb\n");
		break;

	case rsha_RsRt:
		print("Rs = Rt << sha\n");
		break;

	case rshb_RsRt:
		print("Rs = Rt << shb\n");
		break;

	case rsha_RtRu:
		print("Rt = Ru << sha\n");
		break;

	case rshb_RtRu:
		print("Rt = Ru << shb\n");
		break;

	case orlsha_RsRt:
		print("Rs |= Rt >> sha\n");
		break;

	case orlshb_RsRt:
		print("Rs |= Rt >> shb\n");
		break;

	case orlsh_RsRd:
		print("Rs |= Rd >> %d\n", *pc++);
		break;

	case orrsha_RsRt:
		print("Rs |= Rt << sha\n");
		break;

	case orrshb_RsRt:
		print("Rs |= Rt << shb\n");
		break;

	case orrsha_RtRu:
		print("Rt |= Ru << sha\n");
		break;

	case orrshb_RtRu:
		print("Rt |= Ru << shb\n");
		break;

	case or_RsRd:
		print("Rs |= Rd\n");
		break;

	case add_As:
		print("As += %d\n", (long)*pc++);
		break;

	case add_Ad:
		print("Ad += %d\n", (long)*pc++);
		break;

	case initsd:
		print("As = 0x%lux\n", (ulong*)pc[0]);
		print("Ad = 0x%lux\n", (ulong*)pc[1]);
		pc += 2;
		break;

	case ilabel:
		print("Ri = %d\n", *pc++);
		break;

	case olabel:
		print("Ro = %d\n", *pc++);
		break;

	case iloop:
		print("if(--Ri > 0) goto 0x%lux\n", *pc++);
		break;

	case oloop:
		print("if(--Ro > 0) goto 0x%lux\n", *pc++);
		break;

	case rts:
		print("return\n");
		return;

	case load_Rd_P:
		print("Rd = *As++\n");
		break;

	case load_Rs_P:
		print("Rs = *As++\n");
		break;

	case load_Rt_P:
		print("Rt = *As++\n");
		break;

	case load_Ru_P:
		print("Ru = *As++\n");
		break;

	case load_Rd_D:
		print("Rd = *--As\n");
		break;

	case load_Rs_D:
		print("Rs = *--As\n");
		break;

	case load_Rt_D:
		print("Rt = *--As\n");
		break;

	case load_Rd:
		print("Rd = *As\n");
		break;

	case load_Rs:
		print("Rs = *As\n");
		break;

	case load_Rt:
		print("Rt = *As\n");
		break;

	case fetch_Rd_P:
		print("Rd = *Ad++\n");
		break;

	case fetch_Rd_D:
		print("Rd = *--Ad\n");
		break;

	case fetch_Rd:
		print("Rd = *Ad\n");
		break;

	case store_Rs_P:
		print("*Ad++ = Rs\n");
		break;

	case store_Rs_D:
		print("*--Ad = Rs\n");
		break;

	case store_Rs:
		print("*Ad = Rs\n");
		break;

	case inittab:
		print("AT = 0x%lux (%d byte entries)\n", pc[0],pc[1]);
		osiz = pc[1];
		pc += 2;
		break;

	case initsh:
		print("sha = %d\n", (long)pc[0]);
		print("shb = %d\n", (long)pc[1]);
		pc += 2;
		break;

	case table_RdRt:
		switch(osiz){
		case 1:
			print("Rd = ((char*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
			break;
		case 2:
			print("Rd = ((short*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
			break;
		case 4:
			print("Rd = ((long*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
			break;
		default:
			print("bad osiz for table_RdRt\n");
		}
		pc += 2;
		break;

	case table_RsRt:
		switch(osiz){
		case 1:
			print("Rs = ((char*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
			break;
		case 2:
			print("Rs = ((short*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
			break;
		case 4:
			print("Rs = ((long*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
			break;
		default:
			print("bad osiz for table_RdRt\n");
		}
		pc += 2;
		break;

	case assemble:
		/*
		 * Move low arg2 bits of Rd into offset arg1 in Rs.
		 * Can assume that high bits of Rd are zero,
		 * and target field of Rs is zero if offset != 0
		 */
		if(pc[0] == 0)
			print("Rs = Rd >> %d\n", (32-(long)pc[1]));
		else
			print("Rs |= Rd >> %d\n", (32-((long)pc[0]+(long)pc[1])));
		pc += 2;
		break;
		
	case Ozero:
		print("Rs = 0\n");
		break;

	case ODnorS:
		print("Rs = ~(Rd|Rs)\n");
		break;

	case ODandnotS:
		print("Rs = Rd & ~Rs\n");
		break;

	case OnotS:
		print("Rs = ~Rs\n");
		break;

	case OnotDandS:
		print("Rs = ~Rd & Rs\n");
		break;

	case OnotD:
		print("Rs = ~Rd\n");
		break;

	case ODxorS:
		print("Rs ^= Rd\n");
		break;

	case ODnandS:
		print("Rs = ~(Rd & Rs)\n");
		break;

	case ODandS:
		print("Rs &= Rd\n");
		break;

	case ODxnorS:
		print("Rs = ~(Rd ^ Rs)\n");
		break;

	case OD:
		print("Rs = Rd\n");
		break;

	case ODornotS:
		print("Rs = Rd | ~Rs\n");
		break;

	case OnotDorS:
		print("Rs |= ~Rd\n");
		break;

	case ODorS:
		print("Rs |= Rd\n");
		break;

	case OF:
		print("Rs = ~0L\n");
		break;
	}
	goto loop;
}
#endif

