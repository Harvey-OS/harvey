/*
 * C version for byte-at-a-time architectures
 * (those with big-endian bit order, little endian byte order).
 * See comments at beginning of gbitblt.c and bbc.h.
 * The converting bitblt changes considerably, so that Ru is
 * no longer needed in the abstract machine.
 */

#define WBITS	8
#define LWBITS	3
#define W2L	4
#define WMASK	0xFF
#define BYTEREV
typedef uchar	*WType;

typedef	long	Type;

enum
{
	field,		/* arg = field mask */
	lsha_RsRt,
	lshb_RsRt,
	lsh_RsRd,	/* arg = shift amount */
	lsh_RtRt,	/* arg = shift amount */
	lsha_RtRt,
	rsha_RsRt,
	rshb_RsRt,
	orlsha_RsRt,
	orlshb_RsRt,
	orlsh_RsRd,	/* arg = shift amount */
	orrsha_RsRt,
	orrshb_RsRt,
	or_RsRd,
	add_As,		/* arg = add amount */
	add_Ad,		/* arg = add amount */
	initsd,		/* arg1 = value for As; arg2 = value for Ad */
	ilabel,		/* arg = inner loop count value for Ri */
	olabel,		/* arg = outer loop count value for Ro */
	iloop,		/* arg = pointer to beginning of inner loop */
	oloop,		/* arg = pointer to beginning of outer loop */
	rts,
	load_Rs_P,
	load_Rt_P,
	loador_Rt_P,
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
	assemblex,	/* arg1 = offset, arg2 = nbits */
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

/*
 * Macros for assembling the operations of the abstract machine.
 * Each assumes that Type *p points to the next location where
 * an instruction should be assembled.  The macros may also make
 * use of these other variable values:
 *
 *	tab	value of AT register in abstract machine
 *	osiz	value of osiz register in abstract machine
 *	sha	value of sha register in abstract machine
 *	shb	value of shb register in abstract machine
 *
 * Furthermore, there is a long called tmp which can be used for
 * the duration of any macro.
 *
 * For the most part, the macros required are the same as the
 * operations in the abstract machine (without a leading O,
 * or with a lowercase initial letter).  However, instead of
 * macros for the bitblt opcodes Ozero, ..., OF, there is a
 * macro called Emitop, which can use implicit arguments
 * fi and fin (see Emitop, below).
 *
 * Some of the load macros take an argument f, which will be 1
 * if the result of the load will be used by the next instruction.
 * (Some architectures will require a Noop to be emitted when
 * f is 1.)
 *
 * Finally, two other required macros are Extrainit, which can
 * be used to do any implementation-specific initialization,
 * and Execandfree(start,onstack), which must actually execute
 * the compiled program and free the memory.  If onstack is
 * true, the program was compiled into a local array, so no
 * memory need be freed.  Otherwise, the space was obtained with
 * bbmalloc, and must be freed by bbfree.
 */

#define Ofield(c)	*p++ = field; *p++ = (c)
#define Olsha_RsRt	*p++ = lsha_RsRt
#define Olshb_RsRt	*p++ = lshb_RsRt
#define Olsh_RsRd(c)	*p++ = lsh_RsRd; *p++ = (c)
#define Olsh_RtRt(c)	*p++ = lsh_RtRt; *p++ = (c)
#define Olsha_RtRt	*p++ = lsha_RtRt
#define Olsha_RtRu	/* Ru not needed */
#define Olshb_RtRu	/* Ru not needed */
#define Orsha_RsRt	*p++ = rsha_RsRt
#define Orshb_RsRt	*p++ = rshb_RsRt
#define Orsha_RtRu	/* Ru not needed */
#define Orshb_RtRu	/* Ru not needed */
#define Oorlsha_RsRt	*p++ = orlsha_RsRt
#define Oorlshb_RsRt	*p++ = orlshb_RsRt
#define Oorlsh_RsRd(c)	*p++ = orlsh_RsRd; *p++ = (c)
#define Oorrsha_RsRt	*p++ = orrsha_RsRt
#define Oorrshb_RsRt	*p++ = orrshb_RsRt
#define Oorrsha_RtRu	/* Ru not needed */
#define Oorrshb_RtRu	/* Ru not needed */
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
#define Loador_Rt_P	*p++ = loador_Rt_P
#define Load_Ru_P	/* Ru not needed */
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
	{	     0, (uchar*)tab01b, 0, (uchar*)tab03b},
	{(uchar*)tab10b,	    0,  0,	       0},
	{	     0,		    0,  0,	       0},
	{(uchar*)tab30b,	    0,  0,	       0},
};

static uchar tabosiz[4][4] = /* size in bytes of entries */
{
	{	     0, 	    1,  0,	       1},
	{	     1,		    0,  0, 	       0},
	{	     0,		    0,  0,	       0},
	{	     1,		    0,  0,	       0},
};

enum {
	Progmax = 2000,		/* max number of words in a bitblt prog */
	Progmaxnoconv = 200,	/* max number of words when no conversion */
};

static void
interpret(Type *pc)
{
	WType As, Ad;
	ulong Rs, Rd, Rt;
	long Ri, Ro;
	uchar *AT;
	int osiz, sha, shb, tmp;

#ifdef TEST
	WType Aslow, Ashigh, Adlow, Adhigh;
	void prprog(void);

	Rs = lrand();
	Rd = lrand();
	Rt = lrand();
	Ri = lrand();
	Ro = lrand();
	sha = lrand();
	shb = lrand();
	As = 0;
	Ad = 0;
	AT = 0;
	Aslow = (WType)gaddr(cursm, curr.min);
	Ashigh = (WType)gaddr(cursm, sub(curr.max, Pt(1,1)));
	Adlow = (WType)gaddr(curdm, curpt);
	Adhigh = (WType)gaddr(curdm, sub(add(curpt, sub(curr.max,curr.min)),Pt(1,1)));
#endif

loop:
#ifdef TEST
	switch(*pc) {
	case load_Rs_P:
	case load_Rt_P:
	case load_Rd:
	case load_Rs:
	case load_Rt:
			if(As < Aslow || As > Ashigh+3){
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
			if(As-1 < Aslow || As-1 > Ashigh+3){
				print("load from bad As-1 %ux\n", As-1);
				goto errplace;
			}
			break;
	case fetch_Rd_P:
	case fetch_Rd:
			if(Ad < Adlow || Ad > Adhigh+3){
				print("fetch from bad Ad %ux\n", Ad);
				goto errplace;
			}
			break;
	case store_Rs_P:
	case store_Rs:
			if(Ad < Adlow || Ad > Adhigh+3){
				print("store to bad Ad %ux\n", Ad);
				goto errplace;
			}
			break;
	case fetch_Rd_D:
	case store_Rs_D:
			if(Ad-1 < Adlow || Ad-1 > Adhigh+3){
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
		Rs = Rt << sha;
		break;

	case lshb_RsRt:
		Rs = Rt << shb;
		break;

	case lsh_RsRd:
		Rs = Rd << *pc++;
		break;

	case lsh_RtRt:	/* arg = shift amount */
		Rt <<= *pc++;
		break;

	case lsha_RtRt:
		Rt <<= sha;
		break;

	case rsha_RsRt:
		Rs = Rt >> sha;
		break;

	case rshb_RsRt:
		Rs = Rt >> shb;
		break;

	case orlsha_RsRt:
		Rs |= Rt << sha;
		break;

	case orlshb_RsRt:
		Rs |= Rt << shb;
		break;

	case orlsh_RsRd:	/* arg = shift amount */
		Rs |= Rd << *pc++;
		break;

	case orrsha_RsRt:
		Rs |= Rt >> sha;
		break;

	case orrshb_RsRt:
		Rs |= Rt >> shb;
		break;

	case or_RsRd:
		Rs |= Rd;
		break;

	case add_As:
		As = (WType)((char*)As + (long)*pc++);
		break;

	case add_Ad:
		Ad = (WType)((char*)Ad + (long)*pc++);
		break;

	case initsd:
		As = (WType)pc[0];
		Ad = (WType)pc[1];
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

	case load_Rs_P:
		Rs = *As++;
		break;

	case load_Rt_P:
		Rt = *As++;
		break;

	case loador_Rt_P:
		Rt |= *As++;
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
		AT = (uchar*)pc[0];
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
		 * and use it to look up in table AT, putting answer in Rd.
		 * osiz is always 1 for this bitblt
		 */
		tmp = (long)pc[1];
		Rd = (Rt >> (32-((long)pc[0]+tmp))) & ((1<<tmp)-1);
		Rd = AT[Rd];
		pc += 2;
		break;

	case table_RsRt:
		/* like table_RdRt, but answer goes in Rs */
		tmp = (long)pc[1];
		Rs = (Rt >> (32-((long)pc[0]+tmp))) & ((1<<tmp)-1);
		Rs = AT[Rs];
		pc += 2;
		break;

	case assemble:
		/*
		 * Move low arg2 bits of Rd into offset arg1 in Rs.
		 * Can assume that high bits of Rd are zero,
		 * and target field of Rs is zero if offset != 0
		 */
		tmp = (long)pc[0];
		if(tmp == 0)
			Rs = Rd << (8-pc[1]);
		else
			Rs |= Rd << (8-(tmp+pc[1]));
		pc += 2;
		break;
		
	case assemblex:
		/*
		 * Like assemble, but fields will be moved into
		 * proper position by later Assemblex's
		 */
		tmp = (long)pc[0];
		if(tmp == 0)
			Rs = Rd;
		else
			Rs = (Rs << pc[1]) | Rd;
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
		print("Rs = Rt << sha\n");
		break;

	case lshb_RsRt:
		print("Rs = Rt << shb\n");
		break;

	case lsh_RsRd:
		print("Rs = Rd << %d\n", *pc++);
		break;

	case lsh_RtRt:
		print("Rt <<= %d\n", *pc++);
		break;

	case lsha_RtRt:
		print("Rt <<= sha\n");
		break;

	case rsha_RsRt:
		print("Rs = Rt >> sha\n");
		break;

	case rshb_RsRt:
		print("Rs = Rt >> shb\n");
		break;

	case orlsha_RsRt:
		print("Rs |= Rt << sha\n");
		break;

	case orlshb_RsRt:
		print("Rs |= Rt << shb\n");
		break;

	case orlsh_RsRd:
		print("Rs |= Rd << %d\n", *pc++);
		break;

	case orrsha_RsRt:
		print("Rs |= Rt >> sha\n");
		break;

	case orrshb_RsRt:
		print("Rs |= Rt >> shb\n");
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

	case load_Rs_P:
		print("Rs = *As++\n");
		break;

	case load_Rt_P:
		print("Rt = *As++\n");
		break;

	case loador_Rt_P:
		print("Rt |= *As++\n");
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
		print("Rd = ((char*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
		pc += 2;
		break;

	case table_RsRt:
		print("Rs = ((char*)AT)[Rt{%d..%d}]\n", pc[0], pc[0]+pc[1]-1);
		pc += 2;
		break;

	case assemble:
		/*
		 * Move low arg2 bits of Rd into offset arg1 in Rs.
		 * Can assume that high bits of Rd are zero,
		 * and target field of Rs is zero if offset != 0
		 */
		if(pc[0] == 0)
			print("Rs = Rd << %d\n", (32-(long)pc[1]));
		else
			print("Rs |= Rd << %d\n", (32-((long)pc[0]+(long)pc[1])));
		pc += 2;
		break;
		
	case assemblex:
		/*
		 * Like assemble, but fields will be moved into
		 * proper position by later Assemblex's
		 */
		if(pc[0] == 0)
			print("Rs = Rd\n");
		else
			print("Rs = (Rs << %d) | Rd\n", pc[1]);
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

