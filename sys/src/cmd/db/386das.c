#include "defs.h"
#include "fns.h"

typedef struct Instr	Instr;
typedef struct Operand	Operand;
typedef struct Opcode	Opcode;

static int myisp;
static Map *mymap;
static int mydot;

struct Opcode
{
	char	name[10];
	uchar	opsize;		/* 0 means overidable by prefix */
	uchar	and[3];		/* type of operand 1 */
};

struct Operand
{
	uchar	type;
	uchar	index;
	ushort	seg;
	ulong	val;
};

/*
 *  an instruction
 */
struct Instr
{
	Opcode	op;		/* opcode line */
	uchar	mem[20];	/* memory for this instruction */
	int	n;		/* # bytes for this instruction */
	uchar	flags;		/* see #define's below */
	uchar	seg;		/* segment override (if non-zero) */
	char	*pre;		/* instruction prefix (if non-zero) */
	Operand	and[3];		/* operands */

	/*
	 *  stuff decoded from the modRM and sib bytes
	 */
	uchar	rf;		/* register field */
	uchar	reg;		/* general reg */
	uchar	otherreg;	/* other general reg */
	uchar	index;		/* index reg */
	uchar	base;		/* base reg */
	uchar	mult;		/* index multiplier */
	long	disp;		/* displacement */
};
#define OSIZE 1		/* alternate operand size */
#define ASIZE 2		/* alternate address size */
#define	MODREAD 4	/* mod R/M byte read */

/*
 *  operand types
 */
enum {
	None,
	/*
	 *  general (hah!) registers
	 */
	EAX,	ECX,	EDX,	EBX,	ESP,	EBP,	ESI,	EDI,
	AX,	CX,	DX,	BX,	SP,	BP,	SI,	DI,
	AL,	CL,	DL,	BL,	AH,	CH,	DH,	BH,
	/*
	 *  segment registers
	 */
	ES,	CS,	SS,	DS,	FS,	GS,	S6,	S7,
	/*
	 *  control registers
	 */
	CR0,	CR1,	CR2,	CR3,	CR4,	CR5,	CR6,	CR7,
	/*
	 *  debug registers
	 */
	DR0,	DR1,	DR2,	DR3,	DR4,	DR5,	DR6,	DR7,
	/*
	 *  test registers
	 */
	TR0,	TR1,	TR2,	TR3,	TR4,	TR5,	TR6,	TR7,
	/*
	 *  floating point registers
	 */
	F0,	F1,	F2,	F3,	F4,	F5,	F6,	F7,
	/*
	 *  special registers
	 */
	GDTR,
	IDTR,
	EFLAGS,
	/*
	 *  immediate data, offsets, etc.
	 */
	F,	/* far pointer */
	I,	/* immediate of conforming size */
	Ib,	/* immediate that has to be a byte */
	O,
	J,
	/*
	 *  strange indexing
	 */
	X,
	Y,
	/*
	 *  mod reg R/M operands
	 */
	T,	/* reg == test register */
	C,	/* reg == control register */
	D,	/* reg == debug register */
	G,	/* reg == general register */
	S,	/* reg == segment register */
	E,	/* the mod R/M part */
	M,	/* the mod R/M part == memory only */
	R,	/* the mod R/M part == general register only */
	/*
	 * some implicit operand
	 */
	N,
};

/*
 *  register names
 */
static char *regname[] = {
	"none",
	"AX",	"CX",	"DX",	"BX",	"SP",	"BP",	"SI",	"DI",
	"AX",	"CX",	"DX",	"BX",	"SP",	"BP",	"SI",	"DI",
	"AL",	"CL",	"DL",	"BL",	"AH",	"CH",	"DH",	"BH",
	"ES",	"CS",	"SS",	"DS",	"FS",	"GS",	"6?S",	"7?S",
	"CR0",	"CR1",	"CR2",	"?CR3",	"?CR4",	"?CR5",	"?CR6",	"?CR7",
	"DR0",	"DR1",	"DR2",	"DR3",	"?DR4",	"?DR5",	"DR6",	"DR7",
	"?TR0",	"?TR1",	"?TR2",	"?TR3",	"?TR4",	"?TR5",	"TR6",	"TR7",
	"F0",	"F1",	"F2",	"F3",	"F4",	"F5",	"F6",	"F7",
	"GDTR", "IDTR", "EFLAGS", 
};

/*
 *  one byte instructions
 */
Opcode optab1[0x100] =
{
[0x0]	{ "ADDB", 1, E, G, 0 },
	{ "ADD", 0, E, G, 0 },
	{ "ADDB", 1, G, E, 0 },
	{ "ADD", 0, G, E, 0 },
	{ "ADDB", 1, AL, I, 0 },
	{ "ADD", 0, EAX, I, 0 },
	{ "PUSH", 0, ES, 0, 0 },
	{ "POP", 0, ES, 0, 0 },
	{ "ORB", 1, E, G, 0 }, 
	{ "OR", 0, E, G, 0 }, 
	{ "ORB", 1, G, E, 0 }, 
	{ "OR", 0, G, E, 0 }, 
	{ "ORB", 1, AL, I, 0 },
	{ "OR", 0, EAX, I, 0 },
	{ "PUSH", 0, CS, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* 2 byte escape */
[0x10]	{ "ADCB", 1, E, G, 0 },
	{ "ADC", 0, E, G, 0 },
	{ "ADCB", 1, G, E, 0 },
	{ "ADC", 0, G, E, 0 },
	{ "ADCB", 1, AL, I, 0 },
	{ "ADC", 0, EAX, I, 0 },
	{ "PUSH", 0, SS, 0, 0 },
	{ "POP", 0, SS, 0, 0 },
	{ "SBBB", 1, E, G, 0 }, 
	{ "SBB", 0, E, G, 0 }, 
	{ "SBBB", 1, G, E, 0 }, 
	{ "SBB", 0, G, E, 0 }, 
	{ "SBBB", 1, AL, I, 0 },
	{ "SBB", 0, EAX, I, 0 },
	{ "PUSH", 0, DS, 0, 0 },
	{ "POP", 0, DS, 0, 0 },
[0x20]	{ "ANDB", 1, E, G, 0 },
	{ "AND", 0, E, G, 0 },
	{ "ANDB", 1, G, E, 0 },
	{ "AND", 0, G, E, 0 },
	{ "ANDB", 1, AL, I, 0 },
	{ "AND", 0, EAX, I, 0 },
	{ "", 0, 0, 0, 0 },
	{ "DAA", 0, 0, 0, 0 },
	{ "SUBB", 1, E, G, 0 }, 
	{ "SUB", 0, E, G, 0 }, 
	{ "SUBB", 1, G, E, 0 }, 
	{ "SUB", 0, G, E, 0 }, 
	{ "SUBB", 1, AL, I, 0 },
	{ "SUB", 0, EAX, I, 0 },
	{ "", 0, 0, 0, 0 },
	{ "DAS", 0, 0, 0, 0 },
[0x30]	{ "XORB", 1, E, G, 0 },
	{ "XOR", 0, E, G, 0 },
	{ "XORB", 1, G, E, 0 },
	{ "XOR", 0, G, E, 0 },
	{ "XORB", 1, AL, I, 0 },
	{ "XOR", 0, EAX, I, 0 },
	{ "", 0, 0, 0, 0 },
	{ "AAA", 0, 0, 0, 0 },
	{ "CMPB", 1, E, G, 0 }, 
	{ "CMP", 0, E, G, 0 }, 
	{ "CMPB", 1, G, E, 0 }, 
	{ "CMP", 0, G, E, 0 }, 
	{ "CMPB", 1, AL, I, 0 },
	{ "CMP", 0, EAX, I, 0 },
	{ "", 0, 0, 0, 0 },
	{ "AAS", 0, 0, 0, 0 },
[0x40]	{ "INC", 0, EAX, 0, 0 },
	{ "INC", 0, ECX, 0, 0 },
	{ "INC", 0, EDX, 0, 0 },
	{ "INC", 0, EBX, 0, 0 },
	{ "INC", 0, ESP, 0, 0 },
	{ "INC", 0, EBP, 0, 0 },
	{ "INC", 0, ESI, 0, 0 },
	{ "INC", 0, EDI, 0, 0 },
	{ "DEC", 0, EAX, 0, 0 },
	{ "DEC", 0, ECX, 0, 0 },
	{ "DEC", 0, EDX, 0, 0 },
	{ "DEC", 0, EBX, 0, 0 },
	{ "DEC", 0, ESP, 0, 0 },
	{ "DEC", 0, EBP, 0, 0 },
	{ "DEC", 0, ESI, 0, 0 },
	{ "DEC", 0, EDI, 0, 0 },
[0x50]	{ "PUSH", 0, EAX, 0, 0 },
	{ "PUSH", 0, ECX, 0, 0 },
	{ "PUSH", 0, EDX, 0, 0 },
	{ "PUSH", 0, EBX, 0, 0 },
	{ "PUSH", 0, ESP, 0, 0 },
	{ "PUSH", 0, EBP, 0, 0 },
	{ "PUSH", 0, ESI, 0, 0 },
	{ "PUSH", 0, EDI, 0, 0 },
	{ "POP", 0, EAX, 0, 0 },
	{ "POP", 0, ECX, 0, 0 },
	{ "POP", 0, EDX, 0, 0 },
	{ "POP", 0, EBX, 0, 0 },
	{ "POP", 0, ESP, 0, 0 },
	{ "POP", 0, EBP, 0, 0 },
	{ "POP", 0, ESI, 0, 0 },
	{ "POP", 0, EDI, 0, 0 },
[0x60]	{ "PUSHA", 0, N, 0, 0 },
	{ "POPA", 0, N, 0, 0 },
	{ "BOUND", 0, EDX, I, I },
	{ "ARPL", 2, E, R, 0 },
	{ "", 0, 0, 0, 0 },
	{ "", 0, 0, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* operand size prefix */
	{ "", 0, 0, 0, 0 },		/* address size prefix */
	{ "PUSH", 0, I, 0, 0 },
	{ "IMUL", 0, G, E, I },
	{ "PUSH", 0, Ib, 0, 0 },
	{ "IMUL", 0, G, E, Ib },
	{ "INS", 1, Y, EDX, 0 },
	{ "INS", 4, Y, EDX, 0 },
	{ "OUTSB", 1, DX, X, 0 },
	{ "OUTS", 0, DX, X, 0 },
[0x70]	{ "JOS", 1, J, 0, 0 },
	{ "JOC", 1, J, 0, 0 },
	{ "JCS", 1, J, 0, 0 },
	{ "JCC", 1, J, 0, 0 },
	{ "JEQ", 1, J, 0, 0 },
	{ "JNE", 1, J, 0, 0 },
	{ "JLS", 1, J, 0, 0 },
	{ "JHI", 1, J, 0, 0 },
	{ "JMI", 1, J, 0, 0 },
	{ "JPL", 1, J, 0, 0 },
	{ "JPS", 1, J, 0, 0 },
	{ "JPC", 1, J, 0, 0 },
	{ "JLT", 1, J, 0, 0 },
	{ "JGE", 1, J, 0, 0 },
	{ "JLE", 1, J, 0, 0 },
	{ "JGT", 1, J, 0, 0 },
[0x80]	{ "1", 1, E, I, 0 },		/* immediate group 1 */
	{ "1", 0, E, I, 0 },		/* immediate group 1 */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "1", 0, E, Ib, 0 },		/* immediate group 1 */
	{ "TEST", 1, E, G, 0 },
	{ "TEST", 0, E, G, 0 },
	{ "XCHGB", 1, E, G, 0 },
	{ "XCHG", 0, E, G, 0 },
	{ "MOVB", 1, E, G, 0 },
	{ "MOV", 0, E, G, 0 },
	{ "MOVB", 1, G, E, 0 },
	{ "MOV", 0, G, E, 0 },
	{ "MOVW", 2, E, S, 0 },
	{ "LEA", 0, G, M, 0 },
	{ "MOVW", 2, S, E, 0 },
	{ "POP", 0, E, 0, 0 },
[0x90]	{ "NOP", 0, 0, 0, 0 },
	{ "XCHG", 0, EAX, ECX, 0 },
	{ "XCHG", 0, EAX, EDX, 0 },
	{ "XCHG", 0, EAX, EBX, 0 },
	{ "XCHG", 0, EAX, ESP, 0 },
	{ "XCHG", 0, EAX, EBP, 0 },
	{ "XCHG", 0, EAX, ESI, 0 },
	{ "XCHG", 0, EAX, EDI, 0 },
	{ "CWDE", 0, 0, 0, 0 },
	{ "CDQ", 0, 0, 0, 0 },
	{ "CALL", 0, F, 0, 0 },
	{ "WAIT", 0, 0, 0, 0 },
	{ "PUSHF", 0, 0, 0, 0 },
	{ "POPF", 0, 0, 0, 0 },
	{ "SAHF", 0, 0, 0, 0 },
	{ "LAHF", 0, 0, 0, 0 },
[0xA0]	{ "MOVB", 1, AL, O, 0 },
	{ "MOV", 0, EAX, O, 0 },
	{ "MOVB", 1, O, AL, 0 },
	{ "MOV", 0, O, EAX, 0 },
	{ "MOVSB", 1, Y, X, 0 },
	{ "MOVS", 0, Y, X, 0 },
	{ "CMPSB", 1, X, Y, 0 },
	{ "CMPS", 0, X, Y, 0 },
	{ "TESTB", 1, AL, I, 0 },
	{ "TEST", 0, EAX, I, 0 },
	{ "STOSB", 1, Y, AL, 0 },
	{ "STOS", 0, Y, EAX, 0 },
	{ "LODSB", 1, AL, X, 0 },
	{ "LODS", 0, EAX, X, 0 },
	{ "SCASB", 1, AL, Y, 0 },
	{ "SCAS", 0, EAX, Y, 0 },
[0xB0]	{ "MOVB", 1, AL, Ib, 0 },
	{ "MOVB", 1, CL, Ib, 0 },
	{ "MOVB", 1, DL, Ib, 0 },
	{ "MOVB", 1, BL, Ib, 0 },
	{ "MOVB", 1, AH, Ib, 0 },
	{ "MOVB", 1, CH, Ib, 0 },
	{ "MOVB", 1, DH, Ib, 0 },
	{ "MOVB", 1, BH, Ib, 0 },
	{ "MOV", 0, EAX, I, 0 },
	{ "MOV", 0, ECX, I, 0 },
	{ "MOV", 0, EDX, I, 0 },
	{ "MOV", 0, EBX, I, 0 },
	{ "MOV", 0, ESP, I, 0 },
	{ "MOV", 0, EBP, I, 0 },
	{ "MOV", 0, ESI, I, 0 },
	{ "MOV", 0, EDI, I, 0 },
[0xC0]	{ "2", 1, E, Ib, 0 },		/* shift group 2 */
	{ "2", 0, E, Ib, 0 },		/* shift group 2 */
	{ "RET", 2, I, 0, 0 },
	{ "RET", 0, 0, 0, 0 },
	{ "LES", 6, G, M, 0 },
	{ "LDS", 6, G, M, 0 },
	{ "MOV", 1, E, I, 0 },
	{ "MOV", 0, E, I, 0 },
	{ "ENTER", 2, I, Ib, 0 },
	{ "LEAVE", 0, 0, 0, 0 },
	{ "RETfar", 2, I, 0, 0 },
	{ "RETfar", 0, 0, 0, 0 },
	{ "INT 3", 0, 0, 0, 0 },
	{ "INTB", 1, I, 0, 0 },
	{ "INTO", 0, 0, 0, 0 },
	{ "IRET", 0, 0, 0, 0 },
[0xD0]	{ "2", 1, E, 0, 0 },		/* shift group 2 */
	{ "2", 0, E, 0, 0 },		/* shift group 2 */
	{ "2", 1, E, CL, 0 },		/* shift group 2 */
	{ "2", 0, E, CL, 0 },		/* shift group 2 */
	{ "AAM", 0, 0, 0, 0 },
	{ "AAD", 0, 0, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "XLAT", 0, 0, 0, 0 },
	{ "f0", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f1", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f2", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f3", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f4", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f5", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f6", 0, 0, 0, 0, },		/* coprocessor escape */
	{ "f7", 0, 0, 0, 0, },		/* coprocessor escape */
[0xE0]	{ "LOOPNE", 1, J, 0, 0 },
	{ "LOOPE", 1, J, 0, 0 },
	{ "LOOP", 1, J, 0, 0 },
	{ "LCXZ", 1, J, 0, 0 },
	{ "INB", 1, Ib, 0, 0 },
	{ "IN", 0, Ib, 0, 0 },
	{ "OUTB", 1, Ib, 0, 0 },
	{ "OUT", 0, Ib, 0, 0 },
	{ "CALL", 4, J, 0, 0 },
	{ "JMP", 0, J, 0, 0 },
	{ "JMPFAR", 6, F, 0, 0 },
	{ "JMP", 1, J, 0, 0 },
	{ "INB", 1, N, 0, 0 },
	{ "IN", 0, N, 0, 0 },
	{ "OUTB", 1, N, 0, 0 },
	{ "OUT", 0, N, 0, 0 },
[0xF0]	{ "LOCK", 0, 0, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "REPNE", 0, 0, 0, 0 },
	{ "REP", 0, 0, 0, 0 },
	{ "HLT", 0, 0, 0, 0 },
	{ "CMC", 0, 0, 0, 0 },
	{ "3", 1, E, 0, 0 },		/* unary group 3 */
	{ "3", 0, E, 0, 0 },		/* unary group 3 */
	{ "CLC", 0, 0, 0, 0 },
	{ "STC", 0, 0, 0, 0 },
	{ "CLI", 0, 0, 0, 0 },
	{ "STI", 0, 0, 0, 0 },
	{ "CLD", 0, 0, 0, 0 },
	{ "STD", 0, 0, 0, 0 },
	{ "4", 0, 0, 0, 0 },		/* INC/DEC group 4 */
	{ "5", 0, 0, 0, 0 },		/* indirect group 5 */
};

Opcode optab2[0x100] = {
[0x0]	{ "6", 0, 0, 0, 0 },		/* group 6 */
	{ "7", 0, 0, 0, 0 },		/* group 7 */
	{ "LAR", 2, G, E, 0 },
	{ "LSL", 2, G, E, 0 },
[0x06]	{ "CLTS", 0, 0, 0, 0 },
[0x20]	{ "MOV", 4, R, C, 0, },
	{ "MOV", 4, R, D, 0, },
	{ "MOV", 4, C, R, 0, },
	{ "MOV", 4, D, R, 0 },
	{ "MOV", 4, R, T, 0 },
[0x26]	{ "MOV", 4, T, R, 0 },
[0x80]	{ "JOS", 0, J, 0, 0 },
	{ "JOC", 0, J, 0, 0 },
	{ "JCS", 0, J, 0, 0 },
	{ "JCC", 0, J, 0, 0 },
	{ "JEQ", 0, J, 0, 0 },
	{ "JNE", 0, J, 0, 0 },
	{ "JLS", 0, J, 0, 0 },
	{ "JHI", 0, J, 0, 0 },
	{ "JMI", 0, J, 0, 0 },
	{ "JPL", 0, J, 0, 0 },
	{ "JPS", 0, J, 0, 0 },
	{ "JPC", 0, J, 0, 0 },
	{ "JLT", 0, J, 0, 0 },
	{ "JGE", 0, J, 0, 0 },
	{ "JLE", 0, J, 0, 0 },
	{ "JGT", 0, J, 0, 0 },
[0x90]	{ "SETOS", 1, E, 0, 0 },
	{ "SETOC", 1, E, 0, 0 },
	{ "SETCS", 1, E, 0, 0 },
	{ "SETCC", 1, E, 0, 0 },
	{ "SETEQ", 1, E, 0, 0 },
	{ "SETNE", 1, E, 0, 0 },
	{ "SETLS", 1, E, 0, 0 },
	{ "SETHI", 1, E, 0, 0 },
	{ "SETMI", 0, 0, 0, 0 },
	{ "SETPL", 0, 0, 0, 0 },
	{ "SETPS", 0, 0, 0, 0 },
	{ "SETPC", 0, 0, 0, 0 },
	{ "SETLT", 0, 0, 0, 0 },
	{ "SETGE", 0, 0, 0, 0 },
	{ "SETLE", 0, 0, 0, 0 },
	{ "SETGT", 0, 0, 0, 0 },
[0xA0]	{ "PUSH", 0, FS, 0, 0 },
	{ "POP", 0, FS, 0, 0 },
	{ "", 0, 0, 0, 0 },
	{ "BT", 0, E, G, 0 },
	{ "SHLD", 0, E, G, Ib },
	{ "SHLD", 0, E, G, CL },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "PUSH", 0, GS, 0, 0 },
	{ "POP", 0, GS, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "BTS", 0, E, G, 0 },
	{ "SHRD", 0, E, G, Ib },
	{ "SHRD", 0, E, G, CL },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "IMUL", 0, G, E, 0 },
[0xB0]	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "LSS", 6, G, M, 0 },
	{ "BTR", 0, E, G, 0 },
	{ "LFS", 6, G, M, 0 },
	{ "LGS", 6, G, M, 0 },
	{ "MOVZX", 0, G, E, 0 },
	{ "MOVZX", 0, G, E, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "8", 0, E, Ib, 0 },
	{ "BTC", 0, G, E, 0 },
	{ "BSF", 0, G, E, 0 },
	{ "BSR", 0, G, E, 0 },
	{ "MOVBLSX", 0, G, E, 0 },
	{ "MOVWLSX", 0, G, E, 0 },
};

/*
 *  instructions encoded in the mod R/M field
 */
Opcode optabg[] = {
[0x0]	{ "ADD", 0, 0, 0, 0 },
	{ "OR", 0, 0, 0, 0 },
	{ "ADC", 0, 0, 0, 0 },
	{ "SBB", 0, 0, 0, 0 },
	{ "AND", 0, 0, 0, 0 },
	{ "SUB", 0, 0, 0, 0 },
	{ "XOR", 0, 0, 0, 0 },
	{ "CMP", 0, 0, 0, 0 },
[0x8]	{ "ROL", 0, 0, 0, 0 },
	{ "ROR", 0, 0, 0, 0 },
	{ "RCL", 0, 0, 0, 0 },
	{ "RCR", 0, 0, 0, 0 },
	{ "SHL", 0, 0, 0, 0 },
	{ "SHR", 0, 0, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "SAR", 0, 0, 0, 0 },
[0x10]	{ "TEST", 0, 0, I, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "NOT", 0, 0, 0, 0 },
	{ "NEG", 0, 0, 0, 0 },
	{ "MUL", 0, 0, EAX, 0 },
	{ "IMUL", 0, 0, EAX, 0 },
	{ "DIV", 0, 0, EAX, 0 },
	{ "IDIV", 0, 0, EAX, 0 },
[0x18]	{ "INC", 1, E, 0, 0 },
	{ "DEC", 1, E, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
[0x20]	{ "INC", 0, E, 0, 0 },
	{ "DEC", 0, E, 0, 0 },
	{ "CALL*", 0, E, 0, 0 },
	{ "CALL*", 0, F, 0, 0 },
	{ "JMP*", 0, E, 0, 0 },
	{ "JMPFAR*", 6, E, 0, 0 },
	{ "PUSH", 0, E, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
[0x28]	{ "SLDT", 2, E, 0, 0 },
	{ "STR", 2, E, 0, 0 },
	{ "LLDT", 2, E, 0, 0 },
	{ "LTR", 2, E, 0, 0 },
	{ "VERR", 2, E, 0, 0 },
	{ "VERW", 2, E, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
[0x30]	{ "MOVL", 4, E, GDTR, 0 },
	{ "MOVL", 4, E, IDTR, 0 },
	{ "MOVL", 4, GDTR, E, 0 },
	{ "MOVL", 4, IDTR, E, 0 },
	{ "SMSW", 2, E, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "LMSW", 2, E, 0, 0 },
	{ "", 0, 0, 0, 0 },		/* no instruction */
[0x38]	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "", 0, 0, 0, 0 },		/* no instruction */
	{ "BT", 0, 0, 0, 0 },
	{ "BTS", 0, 0, 0, 0 },
	{ "BTR", 0, 0, 0, 0 },
	{ "BTC", 0, 0, 0, 0 },
};

/*
 *  floating point sizes
 */
enum
{
	FSZ=	2,
	DSZ=	4,
	DPSZ=	8,
};

/*
 *  instructions encoded in the mod R/M field
 */
Opcode optabf[] = {
[0x0]	{ "FADDF", FSZ, F0, E, 0 },
	{ "FMULF", FSZ, F0, E, 0 },
	{ "FCOMF", FSZ, F0, E, 0 },
	{ "FCOMFP", FSZ, F0, E, 0 },
	{ "FSUBF", FSZ, F0, E, 0 },
	{ "FSUBRF", FSZ, F0, E, 0 },
	{ "FDIVF", FSZ, F0, E, 0 },
	{ "FDIVRF", FSZ, F0, E, 0 },
[0x8]	{ "FMOVF", FSZ, F0, E, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FMOVF", FSZ, E, F0, 0 },
	{ "FMOVFP", FSZ, E, F0, 0 },
	{ "FLDENV", 0, E, 0, 0 },
	{ "FLDCW", 2, E, 0, 0 },
	{ "FSTENV", 0, E, 0, 0 },
	{ "FSTCW", 2, E, 0, 0 },
[0x10]	{ "FADDL", 1, F0, E, 0 },
	{ "FMULL", 1, F0, E, 0 },
	{ "FCOML", 1, F0, E, 0 },
	{ "FSTCWPL", 1, F0, E, 0 },
	{ "FSULL", 1, F0, E, 0 },
	{ "FSULRL", 1, F0, E, 0 },
	{ "FDIVL", 1, F0, E, 0 },
	{ "FDIVRL", 1, F0, E, 0 },
[0x18]	{ "FMOVL", 1, F0, E, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FMOVL", 1, E, F0, 0 },
	{ "FMOVLP", 1, E, F0, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FMOVX", DPSZ, F0, E, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FMOVXP", DPSZ, E, F0, 0 },
[0x20]	{ "FADDD", DSZ, F0, E, 0 },
	{ "FMULD", DSZ, F0, E, 0 },
	{ "FCOMD", DSZ, F0, E, 0 },
	{ "FCOMDP", DSZ, F0, E, 0 },
	{ "FSUBD", DSZ, F0, E, 0 },
	{ "FSUBRD", DSZ, F0, E, 0 },
	{ "FDIVD", DSZ, F0, E, 0 },
	{ "FDIVRD", DSZ, F0, E, 0 },
[0x28]	{ "FMOVD", DPSZ, F0, E, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FMOVD", DPSZ, E, F0, 0 },
	{ "FMOVDP", DPSZ, E, F0, 0 },
	{ "FRSTOR", 0, E, 0, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FSAVE", 0, E, 0, 0 },
	{ "FSTSW", 2, E, 0, 0 },
[0x30]	{ "FADDW", 2, F0, E, 0 },
	{ "FMULW", 2, F0, E, 0 },
	{ "FCOMW", 2, F0, E, 0 },
	{ "FSTCWPW", 2, F0, E, 0 },
	{ "FSUBW", 2, F0, E, 0 },
	{ "FSUBRW", 2, F0, E, 0 },
	{ "FDIVW", 2, F0, E, 0 },
	{ "FDIVRW", 2, F0, E, 0 },
[0x38]	{ "FMOVW", 2, F0, E, 0 },
	{ "", 0, 0, 0, 0 },
	{ "FMOVW", 2, E, F0, 0 },
	{ "FMOVTPW", 2, E, F0, 0 },
	{ "FBLD", 2, E, 0, 0 },
	{ "FMOVL", 4, F0, E, 0 },
	{ "FBSTP", 2, E, 0, 0 },
	{ "FMOVLP", 4, E, F0, 0 },
};

/*
 *  instructions encoded second byte of a floating
 *  point instruction
 */
Opcode optabff[] = {
/*
 *  first byte D8
 */
[0x00]	{ "FADDD", DSZ, F0, F0, 0 },
	{ "FADDD", DSZ, F0, F1, 0 },
	{ "FADDD", DSZ, F0, F2, 0 },
	{ "FADDD", DSZ, F0, F3, 0 },
	{ "FADDD", DSZ, F0, F4, 0 },
	{ "FADDD", DSZ, F0, F5, 0 },
	{ "FADDD", DSZ, F0, F6, 0 },
	{ "FADDD", DSZ, F0, F7, 0 },
[0x08]	{ "FMULD", DSZ, F0, F0, 0 },
	{ "FMULD", DSZ, F0, F1, 0 },
	{ "FMULD", DSZ, F0, F2, 0 },
	{ "FMULD", DSZ, F0, F3, 0 },
	{ "FMULD", DSZ, F0, F4, 0 },
	{ "FMULD", DSZ, F0, F5, 0 },
	{ "FMULD", DSZ, F0, F6, 0 },
	{ "FMULD", DSZ, F0, F7, 0 },
[0x10]	{ "FCOMD", DSZ, F0, F0, 0 },
	{ "FCOMD", DSZ, F0, F1, 0 },
	{ "FCOMD", DSZ, F0, F2, 0 },
	{ "FCOMD", DSZ, F0, F3, 0 },
	{ "FCOMD", DSZ, F0, F4, 0 },
	{ "FCOMD", DSZ, F0, F5, 0 },
	{ "FCOMD", DSZ, F0, F6, 0 },
	{ "FCOMD", DSZ, F0, F7, 0 },
[0x18]	{ "FCOMDP", DSZ, F0, F0, 0 },
	{ "FCOMDP", DSZ, F0, F1, 0 },
	{ "FCOMDP", DSZ, F0, F2, 0 },
	{ "FCOMDP", DSZ, F0, F3, 0 },
	{ "FCOMDP", DSZ, F0, F4, 0 },
	{ "FCOMDP", DSZ, F0, F5, 0 },
	{ "FCOMDP", DSZ, F0, F6, 0 },
	{ "FCOMDP", DSZ, F0, F7, 0 },
[0x20]	{ "FSUBD", DSZ, F0, F0, 0 },
	{ "FSUBD", DSZ, F0, F1, 0 },
	{ "FSUBD", DSZ, F0, F2, 0 },
	{ "FSUBD", DSZ, F0, F3, 0 },
	{ "FSUBD", DSZ, F0, F4, 0 },
	{ "FSUBD", DSZ, F0, F5, 0 },
	{ "FSUBD", DSZ, F0, F6, 0 },
	{ "FSUBD", DSZ, F0, F7, 0 },
[0x28]	{ "FSUBRD", DSZ, F0, F0, 0 },
	{ "FSUBRD", DSZ, F0, F1, 0 },
	{ "FSUBRD", DSZ, F0, F2, 0 },
	{ "FSUBRD", DSZ, F0, F3, 0 },
	{ "FSUBRD", DSZ, F0, F4, 0 },
	{ "FSUBRD", DSZ, F0, F5, 0 },
	{ "FSUBRD", DSZ, F0, F6, 0 },
	{ "FSUBRD", DSZ, F0, F7, 0 },
[0x30]	{ "FDIVD", DSZ, F0, F0, 0 },
	{ "FDIVD", DSZ, F0, F1, 0 },
	{ "FDIVD", DSZ, F0, F2, 0 },
	{ "FDIVD", DSZ, F0, F3, 0 },
	{ "FDIVD", DSZ, F0, F4, 0 },
	{ "FDIVD", DSZ, F0, F5, 0 },
	{ "FDIVD", DSZ, F0, F6, 0 },
	{ "FDIVD", DSZ, F0, F7, 0 },
[0x38]	{ "FDIVRD", DSZ, F0, F0, 0 },
	{ "FDIVRD", DSZ, F0, F1, 0 },
	{ "FDIVRD", DSZ, F0, F2, 0 },
	{ "FDIVRD", DSZ, F0, F3, 0 },
	{ "FDIVRD", DSZ, F0, F4, 0 },
	{ "FDIVRD", DSZ, F0, F5, 0 },
	{ "FDIVRD", DSZ, F0, F6, 0 },
	{ "FDIVRD", DSZ, F0, F7, 0 },
/*
 *  first byte D9
 */
[0x40]	{ "FMOVD", DSZ, F0, F0, 0 },
	{ "FMOVD", DSZ, F0, F1, 0 },
	{ "FMOVD", DSZ, F0, F2, 0 },
	{ "FMOVD", DSZ, F0, F3, 0 },
	{ "FMOVD", DSZ, F0, F4, 0 },
	{ "FMOVD", DSZ, F0, F5, 0 },
	{ "FMOVD", DSZ, F0, F6, 0 },
	{ "FMOVD", DSZ, F0, F7, 0 },
[0x48]	{ "FXCHD", DSZ, F0, F0, 0 },
	{ "FXCHD", DSZ, F0, F1, 0 },
	{ "FXCHD", DSZ, F0, F2, 0 },
	{ "FXCHD", DSZ, F0, F3, 0 },
	{ "FXCHD", DSZ, F0, F4, 0 },
	{ "FXCHD", DSZ, F0, F5, 0 },
	{ "FXCHD", DSZ, F0, F6, 0 },
	{ "FXCHD", DSZ, F0, F7, 0 },
[0x50]	{ "FNOP", DSZ, 0, 0, 0 },
[0x60]	{ "FCHS", DSZ, 0, 0, 0 },
	{ "FABS", DSZ, 0, 0, 0 },
[0x64]	{ "FTST", DSZ, 0, 0, 0 },
	{ "FXAM", DSZ, 0, 0, 0 },
[0x68]	{ "FLD1", DSZ, 0, 0, 0 },
	{ "FLDL2T", DSZ, 0, 0, 0 },
	{ "FLDL2E", DSZ, 0, 0, 0 },
	{ "FLDPI", DSZ, 0, 0, 0 },
	{ "FLDLG2", DSZ, 0, 0, 0 },
	{ "FLDLN2", DSZ, 0, 0, 0 },
	{ "FLDZ", DSZ, 0, 0, 0 },
	{ "", 0, 0, 0, 0 },
[0x70]	{ "F2XM1", DSZ, 0, 0, 0 },
	{ "FYL2X", DSZ, 0, 0, 0 },
	{ "FPTAN", DSZ, 0, 0, 0 },
	{ "FPATAN", DSZ, 0, 0, 0 },
	{ "FXTRACT", DSZ, 0, 0, 0 },
	{ "FPREM1", DSZ, 0, 0, 0 },
	{ "FDECSTP", DSZ, 0, 0, 0 },
	{ "FNCSTP", DSZ, 0, 0, 0 },
[0x78]	{ "FPREM", DSZ, 0, 0, 0 },
	{ "FYL2XP1", DSZ, 0, 0, 0 },
	{ "FSQRT", DSZ, 0, 0, 0 },
	{ "FSINCOS", DSZ, 0, 0, 0 },
	{ "FRNDINT", DSZ, 0, 0, 0 },
	{ "FSCALE", DSZ, 0, 0, 0 },
	{ "FSIN", DSZ, 0, 0, 0 },
	{ "FCOS", DSZ, 0, 0, 0 },
/*
 *  first byte DA
 */
[0xA9]	{ "FUCOMPP", DSZ, 0, 0, 0 },
/*
 *  first byte DB
 */
[0xE2]	{ "FCLEX", DSZ, 0, 0, 0 },
	{ "FNIT", DSZ, 0, 0, 0 },
/*
 *  first byte DC
 */
[0x100]	{ "FADDD", DSZ, F0, F0, 0 },
	{ "FADDD", DSZ, F1, F0, 0 },
	{ "FADDD", DSZ, F2, F0, 0 },
	{ "FADDD", DSZ, F3, F0, 0 },
	{ "FADDD", DSZ, F4, F0, 0 },
	{ "FADDD", DSZ, F5, F0, 0 },
	{ "FADDD", DSZ, F6, F0, 0 },
	{ "FADDD", DSZ, F7, F0, 0 },
[0x108]	{ "FMULD", DSZ, F0, F0, 0 },
	{ "FMULD", DSZ, F1, F0, 0 },
	{ "FMULD", DSZ, F2, F0, 0 },
	{ "FMULD", DSZ, F3, F0, 0 },
	{ "FMULD", DSZ, F4, F0, 0 },
	{ "FMULD", DSZ, F5, F0, 0 },
	{ "FMULD", DSZ, F6, F0, 0 },
	{ "FMULD", DSZ, F7, F0, 0 },
[0x120]	{ "FSUBRD", DSZ, F0, F0, 0 },
	{ "FSUBRD", DSZ, F1, F0, 0 },
	{ "FSUBRD", DSZ, F2, F0, 0 },
	{ "FSUBRD", DSZ, F3, F0, 0 },
	{ "FSUBRD", DSZ, F4, F0, 0 },
	{ "FSUBRD", DSZ, F5, F0, 0 },
	{ "FSUBRD", DSZ, F6, F0, 0 },
	{ "FSUBRD", DSZ, F7, F0, 0 },
[0x128]	{ "FSUBD", DSZ, F0, F0, 0 },
	{ "FSUBD", DSZ, F1, F0, 0 },
	{ "FSUBD", DSZ, F2, F0, 0 },
	{ "FSUBD", DSZ, F3, F0, 0 },
	{ "FSUBD", DSZ, F4, F0, 0 },
	{ "FSUBD", DSZ, F5, F0, 0 },
	{ "FSUBD", DSZ, F6, F0, 0 },
	{ "FSUBD", DSZ, F7, F0, 0 },
[0x130]	{ "FDIVRD", DSZ, F0, F0, 0 },
	{ "FDIVRD", DSZ, F1, F0, 0 },
	{ "FDIVRD", DSZ, F2, F0, 0 },
	{ "FDIVRD", DSZ, F3, F0, 0 },
	{ "FDIVRD", DSZ, F4, F0, 0 },
	{ "FDIVRD", DSZ, F5, F0, 0 },
	{ "FDIVRD", DSZ, F6, F0, 0 },
	{ "FDIVRD", DSZ, F7, F0, 0 },
[0x138]	{ "FDIVD", DSZ, F0, F0, 0 },
	{ "FDIVD", DSZ, F1, F0, 0 },
	{ "FDIVD", DSZ, F2, F0, 0 },
	{ "FDIVD", DSZ, F3, F0, 0 },
	{ "FDIVD", DSZ, F4, F0, 0 },
	{ "FDIVD", DSZ, F5, F0, 0 },
	{ "FDIVD", DSZ, F6, F0, 0 },
	{ "FDIVD", DSZ, F7, F0, 0 },
/*
 *  first byte DD
 */
[0x140]	{ "FFREED", DSZ, F0, 0, 0 },
	{ "FFREED", DSZ, F1, 0, 0 },
	{ "FFREED", DSZ, F2, 0, 0 },
	{ "FFREED", DSZ, F3, 0, 0 },
	{ "FFREED", DSZ, F4, 0, 0 },
	{ "FFREED", DSZ, F5, 0, 0 },
	{ "FFREED", DSZ, F6, 0, 0 },
	{ "FFREED", DSZ, F7, 0, 0 },
[0x150]	{ "FMOVD", DSZ, F0, F0, 0 },
	{ "FMOVD", DSZ, F0, F1, 0 },
	{ "FMOVD", DSZ, F0, F2, 0 },
	{ "FMOVD", DSZ, F0, F3, 0 },
	{ "FMOVD", DSZ, F0, F4, 0 },
	{ "FMOVD", DSZ, F0, F5, 0 },
	{ "FMOVD", DSZ, F0, F6, 0 },
	{ "FMOVD", DSZ, F0, F7, 0 },
[0x158]	{ "FMOVDP", DSZ, F0, F0, 0 },
	{ "FMOVDP", DSZ, F0, F1, 0 },
	{ "FMOVDP", DSZ, F0, F2, 0 },
	{ "FMOVDP", DSZ, F0, F3, 0 },
	{ "FMOVDP", DSZ, F0, F4, 0 },
	{ "FMOVDP", DSZ, F0, F5, 0 },
	{ "FMOVDP", DSZ, F0, F6, 0 },
	{ "FMOVDP", DSZ, F0, F7, 0 },
[0x160]	{ "FUCOMD", F0, DSZ, F0, 0 },
	{ "FUCOMD", F0, DSZ, F1, 0 },
	{ "FUCOMD", F0, DSZ, F2, 0 },
	{ "FUCOMD", F0, DSZ, F3, 0 },
	{ "FUCOMD", F0, DSZ, F4, 0 },
	{ "FUCOMD", F0, DSZ, F5, 0 },
	{ "FUCOMD", F0, DSZ, F6, 0 },
	{ "FUCOMD", F0, DSZ, F7, 0 },
[0x168]	{ "FUCOMDP", F0, DSZ, F0, 0 },
	{ "FUCOMDP", F0, DSZ, F1, 0 },
	{ "FUCOMDP", F0, DSZ, F2, 0 },
	{ "FUCOMDP", F0, DSZ, F3, 0 },
	{ "FUCOMDP", F0, DSZ, F4, 0 },
	{ "FUCOMDP", F0, DSZ, F5, 0 },
	{ "FUCOMDP", F0, DSZ, F6, 0 },
	{ "FUCOMDP", F0, DSZ, F7, 0 },
/*
 *  first byte DE
 */
[0x180]	{ "FADDDP", DPSZ, F0, F0, 0 },
	{ "FADDDP", DPSZ, F1, F0, 0 },
	{ "FADDDP", DPSZ, F2, F0, 0 },
	{ "FADDDP", DPSZ, F3, F0, 0 },
	{ "FADDDP", DPSZ, F4, F0, 0 },
	{ "FADDDP", DPSZ, F5, F0, 0 },
	{ "FADDDP", DPSZ, F6, F0, 0 },
	{ "FADDDP", DPSZ, F7, F0, 0 },
[0x188]	{ "FMULDP", DPSZ, F0, F0, 0 },
	{ "FMULDP", DPSZ, F1, F0, 0 },
	{ "FMULDP", DPSZ, F2, F0, 0 },
	{ "FMULDP", DPSZ, F3, F0, 0 },
	{ "FMULDP", DPSZ, F4, F0, 0 },
	{ "FMULDP", DPSZ, F5, F0, 0 },
	{ "FMULDP", DPSZ, F6, F0, 0 },
	{ "FMULDP", DPSZ, F7, F0, 0 },
[0x199]	{ "FCOMPDP",  DPSZ, 0, 0, 0 },
[0x1A0]	{ "FSUBRDP", DPSZ, F0, F0, 0 },
	{ "FSUBRDP", DPSZ, F1, F0, 0 },
	{ "FSUBRDP", DPSZ, F2, F0, 0 },
	{ "FSUBRDP", DPSZ, F3, F0, 0 },
	{ "FSUBRDP", DPSZ, F4, F0, 0 },
	{ "FSUBRDP", DPSZ, F5, F0, 0 },
	{ "FSUBRDP", DPSZ, F6, F0, 0 },
	{ "FSUBRDP", DPSZ, F7, F0, 0 },
[0x1A8]	{ "FSUBDP", DPSZ, F0, F0, 0 },
	{ "FSUBDP", DPSZ, F1, F0, 0 },
	{ "FSUBDP", DPSZ, F2, F0, 0 },
	{ "FSUBDP", DPSZ, F3, F0, 0 },
	{ "FSUBDP", DPSZ, F4, F0, 0 },
	{ "FSUBDP", DPSZ, F5, F0, 0 },
	{ "FSUBDP", DPSZ, F6, F0, 0 },
	{ "FSUBDP", DPSZ, F7, F0, 0 },
[0x1B0]	{ "FDIVRDP", DPSZ, F0, F0, 0 },
	{ "FDIVRDP", DPSZ, F1, F0, 0 },
	{ "FDIVRDP", DPSZ, F2, F0, 0 },
	{ "FDIVRDP", DPSZ, F3, F0, 0 },
	{ "FDIVRDP", DPSZ, F4, F0, 0 },
	{ "FDIVRDP", DPSZ, F5, F0, 0 },
	{ "FDIVRDP", DPSZ, F6, F0, 0 },
	{ "FDIVRDP", DPSZ, F7, F0, 0 },
[0x1B8]	{ "FDIVDP", DPSZ, F0, F0, 0 },
	{ "FDIVDP", DPSZ, F1, F0, 0 },
	{ "FDIVDP", DPSZ, F2, F0, 0 },
	{ "FDIVDP", DPSZ, F3, F0, 0 },
	{ "FDIVDP", DPSZ, F4, F0, 0 },
	{ "FDIVDP", DPSZ, F5, F0, 0 },
	{ "FDIVDP", DPSZ, F6, F0, 0 },
	{ "FDIVDP", DPSZ, F7, F0, 0 },
/*
 *  first byte DF
 */
[0x1E0]	{ "FSTSW", DPSZ, EAX, 0, 0 },

};

/*
 *  get another byte of the instruction
 */
static uchar
igetc(Instr *i)
{
	uchar	c;

	if(i->n+1 > sizeof(i->mem)){
		error("instruction too long");
		return 0;
	}
	get1(mymap, mydot+i->n, myisp, &c, 1);
	i->mem[i->n++] = c;
	return c;
}

/*
 *  get two bytes of the instruction
 */
static ushort
igets(Instr *i)
{
	ushort	s;

	s = igetc(i);
	s |= (igetc(i)<<8);
	return s;
}

/*
 *  get 4 bytes of the instruction
 */
static long
igetl(Instr *i)
{
	long	l;

	l = igets(i);
	l |= (igets(i)<<16);
	return l;
}

/*
 *  get 4 bytes of the instruction
 */

/*
 *  get instruction, address, and opcode prefixs
 */
static void
prefix(Instr *i)
{
	uchar	c;
	int	done;

	for(done = 0; !done;){
		c = igetc(i);
		switch(c){
		case 0x26:
			i->seg = ES;
			break;
		case 0x2E:
			i->seg = CS;
			break;
		case 0x36:
			i->seg = SS;
			break;
		case 0x3E:
			i->seg = CS;
			break;
		case 0x64:
			i->seg = FS;
			break;
		case 0x65:
			i->seg = GS;
			break;
		case 0x66:
			i->flags |= OSIZE;
			break;
		case 0x67:
			i->flags |= ASIZE;
			break;
		case 0xF0:
			i->pre = "LOCK";
			break;
		case 0xF2:
			i->pre = "REPNE";
			break;
		case 0xF3:
			i->pre = "REP";
			break;
		default:
			done = 1;
			break;
		}
	}
	if(asstype == AI8086)
		i->flags |= OSIZE|ASIZE;
}

/*
 *  convert a register index into an actual register dependng
 *  on operand size
 */
static uchar
regindex(Instr *i, uchar r)
{
	switch(i->op.opsize){
	case 1:
		return AL + r;
	case 2:
		return AX + r;
	case 4:
		return EAX + r;
	default:
		if(i->flags & OSIZE)
			return AX + r;
		else
			return EAX + r;
	}
}

/*
 *  get and crack mod R/M byte (and subsequent sib if it is needed).
 *  we understand both 16 ad 32 bit address modes.
 */
static void
modRM2(Instr *i, uchar x)
{
	uchar	mod;
	uchar	rm;
	short	s;
	char	c;

	if(i->flags & MODREAD)
		return;
	i->flags |= MODREAD;

	mod = (x>>6)&3;
	rm = x&7;
	i->rf = (x>>3)&7;

	/*
	 *  default values
	 */
	i->mult = 1;
	i->otherreg = 0;
	i->reg = regindex(i, i->rf);
	i->index = 0;
	i->base = 0;
	i->disp = 0;

	/*
	 *  register effective address
	 */
	if(mod == 3){
		i->otherreg = regindex(i, rm);
		return;
	}

	/*
	 *  memory effective address
	 */
	if(i->flags & ASIZE){
		/*
		 *  16 bit addressing
		 */
		switch(rm){
		case 0:
			i->base = BX; i->index = SI;
			break;
		case 1:
			i->base = BX; i->index = DI;
			break;
		case 2:
			i->base = BP; i->index = SI;
			break;
		case 3:
			i->base = BP; i->index = DI;
			break;
		case 4:
			i->base = SI;
			break;
		case 5:
			i->base = DI;
			break;
		case 6:
			i->base = BP;
			break;
		case 7:
			i->base = BX;
			break;
		}
		if(mod==0 && rm==6){
			i->base = 0;
			mod = 2;	/* 16 bit displacement */
		}
	} else {
		/*
		 *  32 bit addressing
		 */
		i->base = rm + EAX;
		i->index = 0;
		if(i->base == ESP){
			/*
			 *  sib byte
			 */
			x = igetc(i);
			i->mult = 1<<((x>>6)&3);
			i->index = ((x>>3)&7) + EAX;
			i->base = (x&7) + EAX;
			if(i->index == ESP)
				i->index = 0;
			if(i->base==EBP && mod==00){
				i->base = 0;
				mod = 2;	/* a 32 bit displacement */
			}
		}
		if(mod==0 && rm==5){
			i->base = 0;
			mod = 2;	/* a 32 bit displacement */
		}
	}

	/*
	 *  get the displacement (sign extend)
	 */
	switch(mod){
	case 1:
		c = igetc(i);
		i->disp = c;
		break;
	case 2:
		if(i->flags & ASIZE){
			s = igets(i);
			i->disp = s;
		} else
			i->disp = igetl(i);
		break;
	}
}

static void
modRM(Instr *i)
{
	uchar x;

	if(i->flags & MODREAD)
		return;
	x = igetc(i);
	modRM2(i, x);
}

/*
 *  crack an operand
 */
static void
operand(Instr *i, int and)
{
	Operand *o;
	char c;
	short s;
	int size;

	o = &i->and[and];
	o->type = i->op.and[and];

	switch(o->type){
	/*
 	 *  general register
	 */
	case EAX:
	case ECX:
	case EDX:
	case EBX:
	case ESP:
	case EBP:
	case ESI:
	case EDI:
		o->type = regindex(i, o->type - EAX);
		break;
	case AX:
	case CX:
	case DX:
	case BX:
	case SP:
	case BP:
	case SI:
	case DI:
	case AL:
	case CL:
	case DL:
	case BL:
	case AH:
	case CH:
	case DH:
	case BH:
	case ES:
	case CS:
	case SS:
	case DS:
	case FS:
	case GS:
	case EFLAGS:
	case GDTR:
	case IDTR:
	case F0:
	case F1:
	case F2:
	case F3:
	case F4:
	case F5:
	case F6:
	case F7:
		break;
	/*
	 *  an immediate far pointer
	 */
	case F:
		if(i->flags & OSIZE)
			o->val = igets(i);
		else
			o->val = igetl(i);
		o->seg = igets(i);
		break;
	/*
	 *  an immediate whose size depends on the operand size
	 */
	case I:
		switch(i->op.opsize){
		case 1:
			o->val = igetc(i);
			break;
		case 2:
			o->val = igets(i);
			break;
		case 4:
			o->val = igetl(i);
			break;
		default:
			if(i->flags & OSIZE)
				o->val = igets(i);
			else
				o->val = igetl(i);
			break;
		}
		break;
	/*
	 *  a single byte immediate
	 */
	case Ib:
		o->type = I;
		o->val = igetc(i);

		/*
 		 *  mov's don't sign extend
		 */
		if(strncmp(i->op.name, "MOV", 3)==0)
			break;

		/*
		 *  sign extend
		 */
		size = i->op.opsize;
		if(size == 0)
			size = (i->flags&OSIZE) ? 2 : 4;
		switch(size){
		case 2:
			c = o->val;
			o->val = c;
			o->val &= 0xFFFF;
			break;
		case 4:
			c = o->val;
			o->val = c;
			o->val &= 0xFFFF;
			break;
		}
		break;
	/*
	 *  an absolute offset
	 */
	case O:
		if(i->flags & ASIZE)
			o->val = igets(i);
		else
			o->val = igetl(i);
		break;
	/*
	 *  offset relative to the next instruction
	 */
	case J:
		o->type = O;
		switch(i->op.opsize){
		case 1:
			c = igetc(i);	/* sign extend */
			o->val = c;
			break;
		default: 
			if(i->flags & OSIZE){
				s = igets(i);	/* sign extend */
				o->val = s;
			} else
				o->val = igetl(i);
			break;
		}
		if(i->flags & OSIZE){
			s = o->val;	/* throw out the top 16 bits and */
			o->val = s;	/*  sign extend */
		}
		o->val += mydot + i->n;
		break;
	/*
	 *  strange indexing
	 */
	case X:
		o->type = X;
		o->index = (i->flags&ASIZE) ? SI : ESI;
		break;
	case Y:
		o->type = X;
		o->index = (i->flags&ASIZE) ? DI : EDI;
		break;
	/*
	 *  mod R/M register operands
	 */
	case T:
		modRM(i);
		o->type = ((i->reg-EAX)&7) + TR0;
		break;
	case C:
		modRM(i);
		o->type = ((i->reg-EAX)&7) + CR0;
		break;
	case D:
		modRM(i);
		o->type = ((i->reg-EAX)&7) + DR0;
		break;
	case G:
		modRM(i);
		o->type = i->reg;
		break;
	case S:
		modRM(i);
		o->type = ((i->reg-EAX)&7) + ES;
		break;
	/*
	 *  mod R/M complicated part
	 */
	case E:
		modRM(i);
		if(i->otherreg)
			o->type = i->otherreg;
		else
			o->type = E;
		break;
	case M:
		modRM(i);
		o->type = E;
		break;
	case R:
		modRM(i);
		o->type = i->otherreg;
		break;
	}
}

/*
 *  get the opcode.  The opcode can be be made up of one or two bytes
 *  plus four bits from the mod R/M field.
 */
static void
opcode(Instr *i)
{
	uchar c;
	int j;
	uchar x;

	c = i->mem[i->n-1];
	if(c == 0x0f){
		/* 
		 *  two byte opcode
		 */
		c = igetc(i);
		i->op = optab2[c];
	} else {
		/* 
		 *  one byte opcode
		 */
		i->op = optab1[c];
	}

	if(i->op.name[0] == 'f'){
		/*
		 *  math coprocessor opcode, second byte is
		 *  used to decode opcode
		 */
		x = igetc(i);
		if(((x>>6)&3) != 3){
			/*
			 *  opcode depends on mod R/M field bits.
			 *  operands come both from optab[12] and optabg.
			 */
			modRM2(i, x);
			c = (atoi(i->op.name+1)<<3) | i->rf;
			i->op = optabf[c];
		} else {
			/*
			 *  opcode depends on part of both bytes
			 */
			j = (atoi(i->op.name+1)<<6) | (x & 0x3F);
			i->op = optabff[j];
		}
	} else if(i->op.name[0]>='0' && i->op.name[0]<='9'){
		/*
		 *  opcode depends on mod R/M field bits.
		 *  operands come both from optab[12] and optabg.
		 */
		modRM(i);
		c = ((atoi(i->op.name)-1)<<3) | i->rf;
		strcpy(i->op.name, optabg[c].name);
		i->op.opsize |= optabg[c].opsize;
		for(j = 0; j < 3; j++)
			i->op.and[j] |= optabg[c].and[j];
		if(i->op.opsize==1 && i->op.and[0])
			strcat(i->op.name, "B");
	}

	if(i->op.opsize==0 && i->op.and[0])
		strcat(i->op.name, (i->flags&OSIZE)?"W":"L");
	else if(strcmp(i->op.name, "CWDE")==0 && (i->flags&OSIZE))
		strcpy(i->op.name, "CBW");
	else if(strcmp(i->op.name, "CDQ")==0 && (i->flags&OSIZE))
		strcpy(i->op.name, "CWD");
}

/*
 *  print an operand
 */
static void
properand(Instr *i, int and)
{
	Operand *o;
	Symbol s;

	o = &i->and[and];

	if(o->type < EFLAGS){
		dprint("%s", regname[o->type]);
		return;
	}

	switch(o->type){
	case I:
	case O:
		if (findsym(o->val, CANY, &s) && o->val - s.value < maxoff)
			psymoff(o->val, SEGANY, "(SB)");
		else
			dprint("$%lux", o->val);
		break;
	case F:
		dprint("%lux:%lux", o->seg, o->val);
		break;
	case E:
		if(i->seg)
			dprint("%s:", regname[i->seg]);
		dprint("%lux", i->disp);
		if(i->base)
			dprint("(%s)", regname[i->base]);
		if(i->index)
			dprint("(%s*%d)", regname[i->index], i->mult);
		break;
	case X:
		if(i->seg)
			dprint("%s:", regname[i->seg]);
		dprint("(%s)", regname[o->index]);
		break;
	}
}

/*
 *  get an instruction
 */
static void
mkinstr(Instr *i)
{
	int j;

	memset(i, 0, sizeof(Instr));
	prefix(i);
	opcode(i);
	for(j = 0; j < 3; j++)
		operand(i, j);
};

static char
hex(int x)
{
	x &= 0xf;
	return "0123456789abcdef"[x];
}

void
i386printins(Map *map, char modifier, int isp)
{
	Instr	instr;

	USED(modifier);
	mymap = map;
	myisp = isp;
	mydot = dot;
	mkinstr(&instr);
	if(instr.pre)
		dprint("%s ", instr.pre);
	dprint("%s ", instr.op.name[0] ? instr.op.name : "???");

	/*
	 *  print operands in plan 9 standard order
	 */
	if(strncmp(instr.op.name, "CMP", 3)!=0){
		if(instr.op.and[1]){
			properand(&instr, 1);
			dprint(",");
		}
		if(instr.op.and[2]){
			properand(&instr, 2);
			dprint(",");
		}
		if(instr.op.and[0] && instr.op.and[0]!=N)
			properand(&instr, 0);
	} else {
		if(instr.op.and[0] && instr.op.and[0]!=N){
			properand(&instr, 1);
			dprint(",");
		}
		if(instr.op.and[1])
			properand(&instr, 0);
	}
	dotinc = instr.n;
}

void
i386printdas(Map * map, int isp)
{
	Instr	instr;
	int i;

	mymap = map;
	myisp = isp;
	mydot = dot;
	mkinstr(&instr);
	for(i = 0; i < instr.n; i++)
		dprint("%c%c", hex(instr.mem[i]>>4), hex(instr.mem[i]));
	dprint("%38t");
	dotinc = 0;
}

long
i386regval(int reg)
{
	switch(reg){
	case EAX:
	case ECX:
	case EDX:
	case EBX:
	case ESP:
	case EBP:
	case ESI:
	case EDI:
		return rget(rname(regname[reg]));
	case AX:
	case CX:
	case DX:
	case BX:
	case SP:
	case BP:
	case SI:
	case DI:
		return rget(rname(regname[reg-AX+EAX])) & 0xffff;
	case AL:
	case CL:
	case DL:
	case BL:
		return rget(rname(regname[reg-AL+EAX])) & 0xff;
	case AH:
	case CH:
	case DH:
	case BH:
		return (rget(rname(regname[reg-AH+EAX])) & 0xff00)>>8;
	}
}

int
i386foll(ulong pc, ulong *foll)
{
	Instr in;
	Operand	*o;
	ulong l;

	mydot = pc;
	mkinstr(&in);

	if(strcmp(in.op.name, "RET") == 0){
		if (get4(cormap, rget(rname("SP")), SEGDATA, (long *) &l) == 0)
			chkerr();
		foll[0] = l;
		return 1;
	}
	foll[0] = mydot+in.n;
	if(in.op.name[0] != 'J' && strncmp(in.op.name, "CALL", 4) != 0)
		return 1;

	o = &in.and[0];
	switch(o->type){

	/* a far pointer (ala 8086) */
	case F:
		l = (o->seg<<4) + o->val;
		break;

	/* PC relative */
	case O:
		l = o->val;
		break;

	/* indexing + base + offset */
	case E:
		l = 0;
		if(in.index)
			l += in.mult * i386regval(in.index);
		if(in.base)
			l += i386regval(in.base);
		l += in.disp;
		break;
	
	/* anything else must be a register */
	default:
		l = i386regval(o->type);
		break;
	}

	/* indirect jmp or call */
	if(strchr(in.op.name, '*')){
		if (get4(cormap, l, SEGDATA, (long *) &l) == 0)
			chkerr();
	}

	if(strncmp(in.op.name, "CALL", 4) == 0
	|| strncmp(in.op.name, "JMP", 3) == 0){
		foll[0] = l;
		return 1;
	}

	foll[1] = l;
	return 2;
}
