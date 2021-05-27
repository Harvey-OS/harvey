#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

static char *opstr[] = {	/* Edit s/O(.*),/[O\1]=	"\1",/g */
	[OBAD]=	"BAD",
	[O0F]=	"0F",
	[OAAA]=	"AAA",
	[OAAD]=	"AAD",
	[OAAM]=	"AAM",
	[OAAS]=	"AAS",
	[OADC]=	"ADC",
	[OADD]=	"ADD",
	[OAND]=	"AND",
	[OARPL]=	"ARPL",
	[OASIZE]=	"ASIZE",
	[OBOUND]=	"BOUND",
	[OBT]=	"BT",
	[OBTC]= "BTC",
	[OBTR]= "BTR",
	[OBTS]=	"BTS",
	[OBSF]= "BSF",
	[OBSR]= "BSR",
	[OCALL]=	"CALL",
	[OCBW]=	"CBW",
	[OCLC]=	"CLC",
	[OCLD]=	"CLD",
	[OCLI]=	"CLI",
	[OCMC]=	"CMC",
	[OCMOV]=	"CMOV",
	[OCMP]=	"CMP",
	[OCMPS]=	"CMPS",
	[OCWD]=	"CWD",
	[ODAA]=	"DAA",
	[ODAS]=	"DAS",
	[ODEC]=	"DEC",
	[ODIV]=	"DIV",
	[OENTER]=	"ENTER",
	[OGP1]=	"GP1",
	[OGP2]=	"GP2",
	[OGP3b]=	"GP3b",
	[OGP3v]=	"GP3v",
	[OGP4]=	"GP4",
	[OGP5]=	"GP5",
	[OHLT]=	"HLT",
	[OIDIV]=	"IDIV",
	[OIMUL]=	"IMUL",
	[OIN]=	"IN",
	[OINC]=	"INC",
	[OINS]=	"INS",
	[OINT]=	"INT",
	[OIRET]=	"IRET",
	[OJUMP]=	"JUMP",
	[OLAHF]=	"LAHF",
	[OLFP]=	"LFP",
	[OLEA]=	"LEA",
	[OLEAVE]=	"LEAVE",
	[OLOCK]=	"LOCK",
	[OLODS]=	"LODS",
	[OLOOP]=	"LOOP",
	[OLOOPNZ]=	"LOOPNZ",
	[OLOOPZ]=	"LOOPZ",
	[OMOV]=	"MOV",
	[OMOVS]=	"MOVS",
	[OMOVZX]=	"MOVZX",
	[OMOVSX]=	"MOVSX",
	[OMUL]=	"MUL",
	[ONEG]=	"NEG",
	[ONOP]=	"NOP",
	[ONOT]=	"NOT",
	[OOR]=	"OR",
	[OOSIZE]=	"OSIZE",
	[OOUT]=	"OUT",
	[OOUTS]=	"OUTS",
	[OPOP]=	"POP",
	[OPOPA]=	"POPA",
	[OPOPF]=	"POPF",
	[OPUSH]=	"PUSH",
	[OPUSHA]=	"PUSHA",
	[OPUSHF]=	"PUSHF",
	[ORCL]=	"RCL",
	[ORCR]=	"RCR",
	[OREPE]=	"REPE",
	[OREPNE]=	"REPNE",
	[ORET]=	"RET",
	[ORETF]=	"RETF",
	[OROL]=	"ROL",
	[OROR]=	"ROR",
	[OSAHF]=	"SAHF",
	[OSAR]=	"SAR",
	[OSBB]=	"SBB",
	[OSCAS]=	"SCAS",
	[OSEG]=	"SEG",
	[OSET]=	"SET",
	[OSHL]=	"SHL",
	[OSHLD]=	"SHLD",
	[OSHR]=	"SHR",
	[OSHRD]=	"SHRD",
	[OSTC]=	"STC",
	[OSTD]=	"STD",
	[OSTI]=	"STI",
	[OSTOS]=	"STOS",
	[OSUB]=	"SUB",
	[OTEST]=	"TEST",
	[OWAIT]=	"WAIT",
	[OXCHG]=	"XCHG",
	[OXLAT]=	"XLAT",
	[OXOR]=	"XOR",
};

static char *memstr16[] = {
	"BX+SI",
	"BX+DI",
	"BP+SI",
	"BP+DI",
	"SI",
	"DI",
	"BP",
	"BX",
};

static char *memstr32[] = {
	"EAX",
	"ECX",
	"EDX",
	"EBX",
	"0",
	"EBP",
	"ESI",
	"EDI",
};

static int
argconv(char *p, Inst *i, Iarg *a)
{
	jmp_buf jmp;
	char *s;

	s = p;
	switch(a->tag){
	default:
		abort();

	case TCON:
		return sprint(p, "%lud", a->val);
	case TREG:
	case TREG|TH:
		switch(a->len){
		case 1:
			return sprint(p, "%c%c", "ACDB"[a->reg], "LH"[(a->tag & TH) != 0]);
		case 4:
			*p++ = 'E';
		case 2:
			p += sprint(p, "%c%c", 
				"ACDBSBSDECSDFGIF"[a->reg],
				"XXXXPPIISSSSSSPL"[a->reg]);
			return p - s;
		}
	case TMEM:
		break;
	}

	/* setup trap jump in case we dereference bad memory */
	memmove(jmp, a->cpu->jmp, sizeof jmp);
	if(setjmp(a->cpu->jmp)){
		p += sprint(p, "<%.4lux:%.4lux>", a->seg, a->off);
		goto out;
	}

	switch(a->atype){
	default:
		abort();

	case AAp:
		p += sprint(p, "[%.4lux:%.4lux]", a->seg, a->off);
		break;

	case AJb:
	case AJv:
		p += sprint(p, "[%.4lux]", a->off);
		break;

	case AIc:
		p += sprint(p, "$%.2lx", ars(a));
		break;
	case AIb:
	case AIw:
	case AIv:
		p += sprint(p, "$%.*lux", (int)a->len*2, ar(a));
		break;

	case AMp:
		*p++ = '*';
	case AEb:
	case AEw:
	case AEv:
	case AM:
	case AMa:
	case AMa2:
	case AOb:
	case AOv:
		if(i->sreg != RDS)
			p += sprint(p, "%cS:", "ECSDFG"[i->sreg - RES]);
		if(a->atype == AOb || a->atype == AOv || (i->mod == 0 &&
			(i->alen == 2 && i->rm == 6) ||
			(i->alen == 4 && ((i->rm == 5) ||
			(i->rm == 4 && i->index == 4 && i->base == 5))))){
			p += sprint(p, "[%.*lux]", (int)i->alen*2, a->off);
			break;
		}
		*p++ = '[';
		if(i->alen == 2)
			p += sprint(p, "%s", memstr16[i->rm]);
		else{
			if(i->rm == 4){
				if(i->index != 4)
					p += sprint(p, "%c*%s+", "1248"[i->scale], memstr32[i->index]);
				if(i->base != 5)
					p += sprint(p, "%s", memstr32[i->base]);
				else{
					if(i->mod == 0)
						p += sprint(p, "%.4lux", i->off);
					else
						p += sprint(p, "EBP");
				}
			} else
				p += sprint(p, "%s", memstr32[i->rm]);
		}			
		if(i->mod != 0)
			p += sprint(p, "%+lx", i->disp);
		*p++ = ']';
		break;

	case AXb:
	case AXv:
		if(a->sreg != RDS)
			p += sprint(p, "%cS:", "ECSDFG"[a->sreg - RES]);
		p += sprint(p, "[SI]");
		break;
	case AYb:
	case AYv:
		if(a->sreg != RDS)
			p += sprint(p, "%cS:", "ECSDFG"[a->sreg - RES]);
		p += sprint(p, "[DI]");
		break;
	}

out:
	memmove(a->cpu->jmp, jmp, sizeof jmp);
	*p = 0;
	return p - s;
}

static char *jmpstr[] = {
	"JO", "JNO", "JC", "JNC", "JZ", "JNZ", "JBE", "JA",
	"JS", "JNS", "JP", "JNP", "JL", "JGE", "JLE", "JG",
};

int
instfmt(Fmt *fmt)
{
	Inst *i;
	char *p, buf[256];

	i = va_arg(fmt->args, Inst*);
	p = buf;

	if(i->olen == 4)
		p += sprint(p, "O32: ");
	if(i->alen == 4)
		p += sprint(p, "A32: ");
	if(i->rep)
		p += sprint(p, "%s: ", opstr[i->rep]);
	
	if(i->op == OXLAT && i->sreg != RDS)
		p += sprint(p, "%cS:", "ECSDFG"[i->sreg - RES]);

	if(i->op == OJUMP){
		switch(i->code){
		case 0xE3:
			p += sprint(p, "%s ", "JCXZ");
			break;
		case 0xEB:
		case 0xE9:
		case 0xEA:
		case 0xFF:
			p += sprint(p, "%s ", "JMP");
			break;
		default:
			p += sprint(p, "%s ", jmpstr[i->code&0xF]);
			break;
		}
	} else
		p += sprint(p, "%s ", opstr[i->op]);


	for(;;){
		if(i->a1 == nil)
			break;
		p += argconv(p, i, i->a1);
		if(i->a2 == nil)
			break;
		*p++ = ',';
		*p++ = ' ';
		p += argconv(p, i, i->a2);
		if(i->a3 == nil)
			break;
		*p++ = ',';
		*p++ = ' ';
		p += argconv(p, i, i->a3);
		break;
	}
	*p = 0;
	fmtstrcpy(fmt, buf);
	return 0;
}

int
flagfmt(Fmt *fmt)
{
	char buf[16];
	ulong f;

	f = va_arg(fmt->args, ulong);
	sprint(buf, "%c%c%c%c%c%c%c",
		(f & CF) ? 'C' : 'c',
		(f & SF) ? 'S' : 's',
		(f & ZF) ? 'Z' : 'z',
		(f & OF) ? 'O' : 'o',
		(f & PF) ? 'P' : 'p',
		(f & DF) ? 'D' : 'd',
		(f & IF) ? 'I' : 'i');
	fmtstrcpy(fmt, buf);
	return 0;
}

int
cpufmt(Fmt *fmt)
{
	char buf[512];
	jmp_buf jmp;
	Cpu *cpu;
	Inst i;

	cpu = va_arg(fmt->args, Cpu*);

	memmove(jmp, cpu->jmp, sizeof jmp);
	if(setjmp(cpu->jmp) == 0)
		decode(amem(cpu, 1, RCS, cpu->reg[RIP]), &i);
	memmove(cpu->jmp, jmp, sizeof jmp);

	snprint(buf, sizeof(buf), 
		"%.6lux "
		"%.8lux %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux %.8lux "
		"%.4lux %.4lux %.4lux %.4lux "
		"%J %.4lux %.2ux %I",

		cpu->ic,

		cpu->reg[RAX],
		cpu->reg[RBX],
		cpu->reg[RCX],
		cpu->reg[RDX],

		cpu->reg[RDI],
		cpu->reg[RSI],

		cpu->reg[RBP],
		cpu->reg[RSP],

		cpu->reg[RDS],
		cpu->reg[RES],
		cpu->reg[RSS],
		cpu->reg[RCS],

		cpu->reg[RFL],
		cpu->reg[RIP],

		i.code,
		&i);

	fmtstrcpy(fmt, buf);
	return 0;
}
