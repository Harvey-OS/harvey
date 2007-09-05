/* NCR53c8xx assembler */
%{
#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>

#include "na.h"

#define COND_WAIT (1L << 16)
#define COND_TRUE (1L << 19)
#define COND_INTFLY (1L << 20)
#define COND_CARRY (1L << 21)
#define COND_REL (1L << 23)
#define COND_PHASE (1L << 17)
#define COND_DATA (1L << 18)

#define IO_REL (1L << 26)

#define MOVE_MODE (1L << 27)

int yylex(void);
int yyparse(void);
void assemble(void);
void yyerror(char *, ...);
void yywarn(char *, ...);
void p2error(int line, char *);

struct addr {
	int type; /* 0 - direct, 1 - indirect 2 - table indirect */
	unsigned long offset;
};

typedef enum Type { Const, Addr, Table, Extern, Reg, Unknown, Error } Type;

struct sym {
	char *name;
	int set;
	Type t;
	long value;
	struct sym *next;
};

struct sym *findsym(char *name);
struct sym *symlist;

void newsym(struct sym *s, Type t, long v);

struct binary {
	char len;
	unsigned long data[3];
	unsigned char patch[3];
};

#define MAXCPPOPTS 30
#define MAX_PATCHES 1000
struct na_patch patch[MAX_PATCHES];
int patches;

struct binary out;

struct expval {
	Type t;
	long value;
};

struct expval eval(struct expval a, struct expval b, char op);

int patchtype(Type t);
void fixup(void);

unsigned dot;
unsigned externs;
int errors, warnings;
struct sym *externp[100];

void regmove(unsigned char src_reg, unsigned char op,
    unsigned char dst_reg, struct expval *imm);

void preprocess(char *in, FILE *out);

int mk24bitssigned(long *l);
long mkreladdr(long value, int len);
long chkreladdr(int d, struct expval *e, int len, long relrv);
int pass2;
FILE *in_f;

int yyline = 0;
char yyfilename[200];
char line[500];
char *cppopts[MAXCPPOPTS];
int ncppopts;
int wflag;
%}

%union {
	long n;
	struct sym *s;
	struct expval e;
}

%token NUM MOVE WHEN SYMBOL SELECT WAIT DISCONNECT RESELECT SET CLEAR
%token DATA_OUT DATA_IN COMMAND STATUS RESERVED_OUT RESERVED_IN MESSAGE_OUT
%token MESSAGE_IN WITH ATN FAIL CARRY TARGET ACK COMMENT TO
%token SCNTL0 SCNTL1 SCNTL2 SCNTL3 SCID SXFER SDID GPREG
%token SFBR SOCL SSID SBCL DSTAT SSTAT0 SSTAT1 SSTAT2
%token ISTAT CTEST0 CTEST1 CTEST2 CTEST3 TEMP DFIFO CTEST4 CTEST5 CTEST6
%token DBC DCMD DNAD DSP DSPS DMODE DIEN DWT DCNTL ADDER
%token SIEN0 SIEN1 SIST0 SIST1 SLPAR MACNTL GPCNTL STIME0 STIME1 RESPID
%token STEST0 STEST1 STEST2 STEST3 SIDL SODL SBDL
%token SHL SHR AND OR XOR ADD ADDC
%token JUMP CALL RETURN INT INTFLY NOT ABSOLUTE MASK IF REL PTR
%token TABLE FROM MEMORY NOP EXTERN
%token SCRATCHA0 SCRATCHA1 SCRATCHA2 SCRATCHA3
%token SCRATCHB0 SCRATCHB1 SCRATCHB2 SCRATCHB3
%token SCRATCHC0 SCRATCHC1 SCRATCHC2 SCRATCHC3
%token DSA0 DSA1 DSA2 DSA3
%token DEFW

%left '-' '+'
%left '*' '/'
%left NEG     /* negation--unary minus */
%right '^'    /* exponentiation        */
%type <n> NUM phase .atn set_list set_bit regA reg
%type <n> set_cmd .cond condsfbr condphase
%type <n> jump_or_call .ptr
%type <s> SYMBOL
%type <e> exp byteexp regexp

/* Grammar follows */
%%
input:    /* empty string */
        | input line
;

line:	.label .opcode .comment '\n'
	{
		if (pass2) {
			int x;
			for (x = 0; x < out.len; x++) {
				printf("/* %.4x */ 0x%.8lxL,",
				    dot, out.data[x]);
				if (x == 0) {
					printf(" /*\t");
					fwrite(line,
					    strlen(line) - 1, 1, stdout);
					printf(" */");
				}
				printf("\n");
				if (out.patch[x]) {
					patch[patches].lwoff = dot / 4;
					patch[patches].type = out.patch[x];
					patches++;
				}
				dot += 4;
			}
		}
		else
			dot += 4 * out.len;
	}
	| ABSOLUTE SYMBOL '=' exp .comment '\n'
	{
		setsym($2, $4.t, $4.value);
		if (pass2) {
			printf("\t\t\t/*\t");
			fwrite(line, strlen(line) - 1, 1, stdout);
			printf(" */\n");
		}
	}
	| SYMBOL '=' exp .comment '\n'
	{
		setsym($1, $3.t, $3.value);
		if (pass2) {
			printf("\t\t\t/*\t");
			fwrite(line, strlen(line) - 1, 1, stdout);
			printf(" */\n");
		}
	}
	| EXTERN SYMBOL {
		if (pass2) {
			printf("\t\t\t/*\t");
			fwrite(line, strlen(line) - 1, 1, stdout);
			printf(" */\n");
		}
		else {
			if (!pass2)
				externp[externs] = $2;
			setsym($2, Extern, externs++);
		}
	}
	;

.comment: COMMENT
	| /* nothing */
	;

.label:	SYMBOL ':' {
		if ($1->t != Unknown)
		{
			if (!pass2)
				yyerror("multiply defined symbol");
		}
		else {
			$1->t = Addr;
			$1->value = dot;
		}
	}
	| /* nothing */
	;

set_cmd: SET { $$ = 3; }
	| CLEAR { $$ = 4; }
	;

set_bit: CARRY { $$ = 0x400; }
	| TARGET { $$ = 0x200; }
	| ACK { $$ = 0x40; }
	| ATN { $$ = 0x8; }
	;
	
set_list: set_list ',' set_bit { $$ = $1 | $3; }
	| set_list AND set_bit { $$ = $1 | $3; }
	| set_bit { $$ = $1; }
	;

opcode: set_cmd set_list {
		out.len = 2;
		out.data[0] = (1L << 30) | ((long)$1 << 27) | $2;
		out.data[1] = 0;
		out.patch[0] = out.patch[1] = 0;
	}
	| DISCONNECT
	{
		out.len = 2;
		out.data[0] = 0x48020000L;
		out.data[1] = 0;
		out.patch[0] = out.patch[1] = 0;
	}
	| INT exp .cond {
		out.len = 2;
		out.data[0] = $3 | 0x98000000L;
		out.data[1] = $2.value;
		out.patch[0] = out.patch[1] = 0;
	}
	| INTFLY exp .cond {
		out.len = 2;
		out.data[0] = $3 | 0x98000000L | COND_INTFLY;
		out.data[1] = $2.value;
		out.patch[0] = out.patch[1] = 0;
	}
	| jump_or_call exp .cond {
		out.len = 2;
		out.data[0] = $1 | $3 | chkreladdr(1, &$2, 2, COND_REL);
		out.patch[0] = 0;
	}
	| jump_or_call REL '(' exp ')' .cond {
		out.len = 2;
		out.data[0] = $1 | $6 | COND_REL;
		out.data[1] = mkreladdr($4.value, 2);
		out.patch[0] = out.patch[1] = 0;
	}
	| MOVE exp ',' .ptr regexp ',' with_or_when phase {
		out.len = 2;
		out.data[0] = ($8 << 24) | $2.value | ($4 << 29) | MOVE_MODE;
		out.data[1] = $5.value;
		out.patch[0] = 0;
		out.patch[1] = patchtype($5.t);
	}
	| MOVE FROM exp ',' with_or_when phase {
		out.len = 2;
		out.data[0] = ($6 << 24) | (1L << 28) | MOVE_MODE;
		out.data[1] = $3.value;
		out.patch[0] = 0;
		out.patch[1] = patchtype($3.t);
	}
	| MOVE MEMORY exp ',' regexp ',' regexp {
		out.len = 3;
		out.data[0] = 0xc0000000L | $3.value;
		out.data[1] = $5.value;
		out.data[2] = $7.value;
		out.patch[0] = 0;
		out.patch[1] = patchtype($5.t);
		out.patch[2] = patchtype($7.t);
	}
	| MOVE regA TO regA		{ regmove($2, 2, $4, 0); }	/* do reg to sfbr moves using or 0 */
	| MOVE exp TO regA		{ regmove($4, 0, $4, &$2); }
	| MOVE regA '|' exp TO regA	{ regmove($2, 2, $6, &$4); }
	| MOVE regA '&' exp TO regA	{ regmove($2, 4, $6, &$4); }
	| MOVE regA '+' exp TO regA	{ regmove($2, 6, $6, &$4); }
	| MOVE regA '-' exp TO regA	{ regmove($2, 6, $6, &$4); }
	| MOVE regA '+' exp TO regA WITH CARRY	{
		regmove($2, 7, $6, &$4);
	}
	| MOVE regA '-' exp TO regA WITH CARRY	{
		$4.value = -$4.value;
		regmove($2, 7, $6, &$4);
	}
	| MOVE regA SHL TO regA		{ regmove($2, 1, $5, 0); }
	| MOVE regA SHR TO regA		{ regmove($2, 5, $5, 0); }
	| MOVE regA XOR exp TO regA	{ regmove($2, 3, $6, &$4); }
	| NOP {
		out.len = 2;
		out.data[0] = 0x80000000L;
		out.data[1] = 0;
		out.patch[0] = out.patch[1] = 0;
	}
	| RESELECT exp ',' exp {
		out.len = 2;
		out.data[0] = 0x40000000L | ((long)$2.value << 16) | (1L << 9) | chkreladdr(1, &$4, 2, IO_REL);
		out.patch[0] = 0;
	}
	| RESELECT exp ',' REL '(' exp ')' {
		out.len = 2;
		out.data[0] = 0x40000000L | IO_REL
		    | ((long)$2.value << 16) | (1L << 9);
		out.data[1] = mkreladdr($6.value, 2);
		out.patch[0] = out.patch[1] = 0;
	}
	| RESELECT FROM exp ',' exp {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 25) | $3.value | chkreladdr(1, &$5, 2, IO_REL);
		out.patch[0] = 5;
	}
	| RESELECT FROM exp ',' REL '(' exp ')' {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 25) | IO_REL | $3.value;
		out.patch[0] = 5;
		out.data[1] = mkreladdr($7.value, 2);
		out.patch[1] = 0;
	}
	| RETURN .cond {
		
		out.len = 2;
		out.data[0] = 0x90000000L | $2;
		out.data[1] = 0;
		out.patch[0] = out.patch[1] = 0;
	}
	| SELECT .atn exp ',' exp {
		out.len = 2;
		out.data[0] =
		    0x40000000L | ((long)$3.value << 16) | (1L << 9) | $2 | chkreladdr(1, &$5, 2, IO_REL);
		out.patch[0] = 0;
	}
	| SELECT .atn exp ',' REL '(' exp ')' {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 26)
		    | ((long)$3.value << 16) | (1L << 9) | $2;
		out.data[1] = mkreladdr($7.value, 2);
		out.patch[0] = out.patch[1] = 0;
	}
	| SELECT .atn FROM exp ',' exp {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 25) | $4.value | $2 | chkreladdr(1, &$6, 2, IO_REL);
		out.patch[0] = 5;
	}
	| SELECT .atn FROM exp ',' REL '(' exp ')' {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 25) | IO_REL | $4.value | $2;
		out.patch[0] = 5;
		out.data[1] = mkreladdr($8.value, 2);
		out.patch[1] = 0;
	}
	| WAIT DISCONNECT {
		out.len = 2;
		out.data[0] = 0x48000000L;
		out.data[1] = 0;
		out.patch[0] = out.patch[1] = 0;
	}
	| WAIT RESELECT exp {
		out.len = 2;
		out.data[0] = 0x50000000L | chkreladdr(1, &$3, 2, IO_REL);
		out.patch[0] = 0;
	}
	| WAIT RESELECT REL '(' exp ')' {
		out.len = 2;
		out.data[0] = 0x50000000L | (1L << 26);
		out.data[1] = mkreladdr($5.value, 2);
		out.patch[0] = out.patch[1] = 0;
	}
	| WAIT SELECT exp {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 9) | chkreladdr(1, &$3, 2, IO_REL);
		out.patch[0] = 0;
	}
	| WAIT SELECT REL '(' exp ')' {
		out.len = 2;
		out.data[0] = 0x40000000L | (1L << 26) | (1L << 9);
		out.data[1] = mkreladdr($5.value, 2);
		out.patch[0] = out.patch[1] = 0;
	}
	| DEFW exp {
		out.len = 1;
		out.data[0] = $2.value;
		out.patch[0] = patchtype($2.t);
	}
	;

.ptr:	PTR { $$ = 1; }
	| { $$ = 0; }
	;

with_or_when: WITH
	| WHEN
	;
	
jump_or_call: JUMP	 { $$ = 0x80000000L; }
	| CALL		 { $$ = 0x88000000L; }
	;

condsfbr: byteexp { $$ = $1.value | COND_DATA; }
	| byteexp AND MASK byteexp { $$ = ($4.value << 8) | $1.value | COND_DATA; }
	;

condphase: phase { $$ = ($1 << 24) | COND_PHASE; }

.cond:    ',' IF ATN { $$ = COND_TRUE; }
	| ',' IF condphase { $$ = $3 | COND_TRUE; }
	| ',' IF CARRY { $$ = COND_CARRY | COND_TRUE; }
	| ',' IF condsfbr { $$ = $3 | COND_TRUE; }
	| ',' IF ATN AND condsfbr { $$ = $5 | COND_TRUE; }
	| ',' IF condphase AND condsfbr { $$ = $3 | $5 | COND_TRUE; }
	| ',' WHEN condphase { $$ = $3 | COND_WAIT | COND_TRUE; }
	| ',' WHEN CARRY { $$ = COND_CARRY | COND_WAIT | COND_TRUE; }
	| ',' WHEN condsfbr { $$ = $3 | COND_WAIT | COND_TRUE; }
	| ',' WHEN condphase AND condsfbr { $$ = $3 | $5 | COND_WAIT | COND_TRUE; }
	| ',' IF NOT ATN { $$ = 0; }
	| ',' IF NOT condphase { $$ = $4; }
	| ',' IF NOT CARRY { $$ = COND_CARRY; }
	| ',' IF NOT condsfbr { $$ = $4; }
	| ',' IF NOT ATN OR condsfbr { $$ = $6; }
	| ',' IF NOT condphase OR condsfbr { $$ = $4 | $6; }
	| ',' WHEN NOT condphase { $$ = $4 | COND_WAIT; }
	| ',' WHEN NOT CARRY { $$ = COND_CARRY | COND_WAIT; }
	| ',' WHEN NOT condsfbr { $$ = $4 | COND_WAIT; }
	| ',' WHEN NOT condphase OR condsfbr { $$ = $4 | $6 | COND_WAIT; }
	| { $$ = COND_TRUE; }
	;

.opcode: opcode
	| { out.len = 0; }
	;

regA:	reg
	| SFBR { $$ = 8; }
	;

reg:	  SCNTL0	{ $$ = 0; }
	| SCNTL1	{ $$ = 1; }
	| SCNTL2	{ $$ = 2; }
	| SCNTL3	{ $$ = 3; }
	| SCID		{ $$ = 4; }
	| SXFER		{ $$ = 5; }
	| SDID		{ $$ = 6; }
	| GPREG		{ $$ = 7; }
	| SOCL		{ $$ = 9; }
	| SSID		{ $$ = 0xa; }
	| SBCL		{ $$ = 0xb; }
	| DSTAT		{ $$ = 0xc; }
	| SSTAT0	{ $$ = 0xd; }
	| SSTAT1	{ $$ = 0xe; }
	| SSTAT2	{ $$ = 0xf; }
	| DSA0		{ $$ = 0x10; }
	| DSA1		{ $$ = 0x11; }
	| DSA2		{ $$ = 0x12; }
	| DSA3		{ $$ = 0x13; }
	| ISTAT		{ $$ = 0x14; }
	| CTEST0	{ $$ = 0x18; }
	| CTEST1	{ $$ = 0x19; }
	| CTEST2	{ $$ = 0x1a; }
	| CTEST3	{ $$ = 0x1b; }
	| TEMP		{ $$ = 0x1c; }
	| DFIFO		{ $$ = 0x20; }
	| CTEST4	{ $$ = 0x21; }
	| CTEST5	{ $$ = 0x22; }
	| CTEST6	{ $$ = 0x23; }
	| DBC		{ $$ = 0x24; }
	| DCMD		{ $$ = 0x27; }
	| DNAD		{ $$ = 0x28; }
	| DSP		{ $$ = 0x2c; }
	| DSPS		{ $$ = 0x30; }
	| SCRATCHA0	{ $$ = 0x34; }
	| SCRATCHA1	{ $$ = 0x35; }
	| SCRATCHA2	{ $$ = 0x36; }
	| SCRATCHA3	{ $$ = 0x37; }
	| DMODE		{ $$ = 0x38; }
	| DIEN		{ $$ = 0x39; }
	| DWT		{ $$ = 0x3a; }
	| DCNTL		{ $$ = 0x3b; }
	| ADDER		{ $$ = 0x3c; }
	| SIEN0		{ $$ = 0x40; }
	| SIEN1		{ $$ = 0x41; }
	| SIST0		{ $$ = 0x42; }
	| SIST1		{ $$ = 0x43; }
	| SLPAR		{ $$ = 0x44; }
	| MACNTL	{ $$ = 0x46; }
	| GPCNTL	{ $$ = 0x47; }
	| STIME0	{ $$ = 0x48; }
	| STIME1	{ $$ = 0x49; }
	| RESPID	{ $$ = 0x4a; }
	| STEST0	{ $$ = 0x4c; }
	| STEST1	{ $$ = 0x4d; }
	| STEST2	{ $$ = 0x4e; }
	| STEST3	{ $$ = 0x4f; }
	| SIDL		{ $$ = 0x50; }
	| SODL		{ $$ = 0x54; }
	| SBDL		{ $$ = 0x58; }
	| SCRATCHB0	{ $$ = 0x5c; }
	| SCRATCHB1	{ $$ = 0x5d; }
	| SCRATCHB2	{ $$ = 0x5e; }
	| SCRATCHB3	{ $$ = 0x5f; }
	| SCRATCHC0	{ $$ = 0x60; }
	| SCRATCHC1	{ $$ = 0x61; }
	| SCRATCHC2	{ $$ = 0x62; }
	| SCRATCHC3	{ $$ = 0x63; }
	;

.atn:	ATN		{ $$ = (1 << 24); }
	| /* nothing */ { $$ = 0; }
;

phase:	DATA_OUT 	{ $$ = 0; }
	| DATA_IN	{ $$ = 1; }
	| COMMAND 	{ $$ = 2; }
	| STATUS	{ $$ = 3; }
	| RESERVED_OUT 	{ $$ = 4; }
	| RESERVED_IN	{ $$ = 5; }
	| MESSAGE_OUT   { $$ = 6; }
	| MESSAGE_IN	{ $$ = 7; }
;

byteexp: exp
	{
		if (pass2 && ($1.value < 0 || $1.value > 255)) {
			if (wflag)
				yywarn("conversion causes truncation");
			$$.value = $1.value & 0xff;
		}
		else
			$$.value = $1.value;
	}
	;

regexp:	exp
	| regA { $$.t = Reg; $$.value = $1; }
	;

exp:	NUM { $$.t = Const; $$.value = $1; }
	| SYMBOL {
		$$.t = $1->t; $$.value = $1->value;
		if (pass2 && $1->t == Unknown)
		{
			yyerror("Undefined symbol %s", $1->name);
			$1->t = Error;
			$1->value = 0;
			$$.t = Error;
			$$.value = 0;
		}
	}
        | exp '+' exp { $$ = eval($1, $3, '+'); }
        | exp '-' exp { $$ = eval($1, $3, '-'); }
        | exp '*' exp { $$ = eval($1, $3, '*'); }
        | exp '/' exp { $$ = eval($1, $3, '/'); }
        | '-' exp  %prec NEG { $$ = eval($2, $2, '_'); }
        | '(' exp ')'        { $$ = $2; }
	| '~' exp %prec NEG { $$ = eval($2, $2, '~'); }
	;
%%

struct {
	char *name;
	int tok;
} toktab[] =
{
	{ "when", WHEN },
	{ "data_out", DATA_OUT },
	{ "data_in", DATA_IN },
	{ "msg_out", MESSAGE_OUT },
	{ "msg_in", MESSAGE_IN },
	{ "cmd", COMMAND },
	{ "command", COMMAND },
	{ "status", STATUS },
	{ "move", MOVE },
	{ "select", SELECT },
	{ "reselect", RESELECT },
	{ "disconnect", DISCONNECT },
	{ "wait", WAIT },
	{ "set", SET },
	{ "clear", CLEAR },
	{ "with", WITH },
	{ "atn", ATN },
	{ "fail", FAIL },
	{ "carry", CARRY },
	{ "target", TARGET },
	{ "ack", ACK },
	{ "scntl0", SCNTL0 },
	{ "scntl1", SCNTL1 },
	{ "scntl2", SCNTL2 },
	{ "scntl3", SCNTL3 },
	{ "scid", SCID },
	{ "sxfer", SXFER },
	{ "sdid", SDID },
	{ "gpreg", GPREG },
	{ "sfbr", SFBR },
	{ "socl", SOCL },
	{ "ssid", SSID },
	{ "sbcl", SBCL },
	{ "dstat", DSTAT },
	{ "sstat0", SSTAT0 },
	{ "sstat1", SSTAT1 },
	{ "sstat2", SSTAT2 },
	{ "dsa", DSA0 },
	{ "dsa0", DSA0 },
	{ "dsa1", DSA1 },
	{ "dsa2", DSA2 },
	{ "dsa3", DSA3 },
	{ "istat", ISTAT },
	{ "ctest0", CTEST0 },
	{ "ctest1", CTEST1 },
	{ "ctest2", CTEST2 },
	{ "ctest3", CTEST3 },
	{ "temp", TEMP },
	{ "dfifo", DFIFO },
	{ "ctest4", CTEST4 },
	{ "ctest5", CTEST5 },
	{ "ctest6", CTEST6 },
	{ "dbc", DBC },
	{ "dcmd", DCMD },
	{ "dnad", DNAD },
	{ "dsp", DSP },
	{ "dsps", DSPS },
	{ "scratcha", SCRATCHA0 },
	{ "scratcha0", SCRATCHA0 },
	{ "scratcha1", SCRATCHA1 },
	{ "scratcha2", SCRATCHA2 },
	{ "scratcha3", SCRATCHA3 },
	{ "dmode", DMODE },
	{ "dien", DIEN },
	{ "dwt", DWT },
	{ "dcntl", DCNTL },
	{ "adder", ADDER },
	{ "sien0", SIEN0 },
	{ "sien1", SIEN1 },
	{ "sist0", SIST0 },
	{ "sist1", SIST1 },
	{ "slpar", SLPAR },
	{ "macntl", MACNTL },
	{ "gpcntl", GPCNTL },
	{ "stime0", STIME0 },
	{ "stime1", STIME1 },
	{ "respid", RESPID },
	{ "stest0", STEST0 },
	{ "stest1", STEST1 },
	{ "stest2", STEST2 },
	{ "stest3", STEST3 },
	{ "sidl", SIDL },
	{ "sodl", SODL },
	{ "sbdl", SBDL },
	{ "scratchb", SCRATCHB0 },
	{ "scratchb0", SCRATCHB0 },
	{ "scratchb1", SCRATCHB1 },
	{ "scratchb2", SCRATCHB2 },
	{ "scratchb3", SCRATCHB3 },
	{ "scratchc", SCRATCHC0 },
	{ "scratchc0", SCRATCHC0 },
	{ "scratchc1", SCRATCHC1 },
	{ "scratchc2", SCRATCHC2 },
	{ "scratchc3", SCRATCHC3 },
	{ "add", ADD },
	{ "addc", ADDC },
	{ "and", AND },
	{ "or", OR },
	{ "xor", XOR },
	{ "shl", SHL },
	{ "shr", SHR },
	{ "jump", JUMP },
	{ "call", CALL },
	{ "return", RETURN },
	{ "int", INT },
	{ "intfly", INTFLY },
	{ "not", NOT },
	{ "absolute", ABSOLUTE },
	{ "mask", MASK },
	{ "if", IF },
	{ "rel", REL },
	{ "ptr", PTR },
	{ "table", TABLE },
	{ "from", FROM },
	{ "memory", MEMORY },
	{ "to", TO },
	{ "nop", NOP },
	{ "extern", EXTERN },
	{ "defw", DEFW },
};

#define TOKS (sizeof(toktab)/sizeof(toktab[0]))

int lc;
int ll;

void
yyrewind(void)
{
	rewind(in_f);
	ll = lc = 0;
	yyline = 0;
	dot = 0;
}

int
yygetc(void)
{
	if (lc == ll)
	{
	next:
		if (fgets(line, 500, in_f) == 0)
			return EOF;
		/* do nasty check for #line directives */
		if (strncmp(line, "#line", 5) == 0) {
			/* #line n "filename" */
			sscanf(line, "#line %d \"%[^\"]", &yyline, yyfilename);
			yyline--;
			goto next;
		}
		yyline++;
		ll = strlen(line);
		lc = 0;
	}
	return line[lc++];
}

void
yyungetc(void)
{
	if (lc <= 0)
		exits("ungetc");
	lc--;
}

int
yylex(void)
{
	char token[100];
	int tl = 0;
	int c;
	while ((c = yygetc()) != EOF && (c == ' ' || c == '\t'))
		;
	if (c == EOF)
		return 0;
	if (isalpha(c) || c == '_')
	{
		int x;
		do {
			token[tl++] = c;
		} while ((c = yygetc()) != EOF && (isalnum(c) || c == '_'));
		if (c == EOF)
			return 0;
		yyungetc();
		token[tl] = 0;
		for (x = 0; x < TOKS; x++)
			if (strcmp(toktab[x].name, token) == 0)
				return toktab[x].tok;
		/* must be a symbol */
		yylval.s = findsym(token);
		return SYMBOL;
	}
	else if (isdigit(c))
	{
		/* accept 0x<digits> or 0b<digits> 0<digits> or <digits> */
		int prefix = c == '0';
		unsigned long n = c - '0';
		int base = 10;
		for (;;)
		{
			c = yygetc();
			if (c == EOF)
				return 0;
			if (prefix)
			{
				prefix = 0;
				if (c == 'x') {
					base = 16;
					continue;
				}
				else if (c == 'b')
				{
					base = 2;
					continue;
				}
				else
					base = 8;
			}
			if (isdigit(c))
				c -= '0';
			else if (isalpha(c) && base > 10)
			{
				if (isupper(c))
					c = tolower(c);
				c = c - 'a' + 10;
			}
			else {
				yyungetc();
				yylval.n = n;
				return NUM;
			}
			if (c >= base)
				yyerror("illegal format number");
			n = n * base + c;
		}
	}
	else if (c == ';') {
		/* skip to end of line */
		while ((c = yygetc()) != EOF && c != '\n')
			;
		if (c != EOF)
			yyungetc();
		return COMMENT;
	}
	return c;
}

void
yyerror(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	fprintf(stderr, "%s: %d: ", yyfilename, yyline);
	vfprintf(stderr, s, ap);
	if (putc('\n', stderr) < 0)
		exits("io");
	errors++;
	va_end(ap);
}

void
yywarn(char *s, ...)
{
	va_list ap;

	va_start(ap, s);
	fprintf(stderr, "%s: %d: warning: ", yyfilename, yyline);
	vfprintf(stderr, s, ap);
	if (putc('\n', stderr) < 0)
		exits("io");
	warnings++;
	va_end(ap);
}

void
p2error(int line, char *s)
{
	USED(line);
	printf("/*\t%s */\n", s);
}

void
main(int argc, char *argv[])
{
	int a;
	for (a = 1; a < argc; a++)
	{
		if (argv[a][0] == '-')
			switch (argv[a][1]) {
			case 'D':
				/* #defines for cpp */
				if (ncppopts >= MAXCPPOPTS) {
					fprintf(stderr, "too many cpp options\n");
					exits("options");
				}
				cppopts[ncppopts++] = argv[a];
				break;
			default:
				fprintf(stderr, "unrecognised option %s\n",
				    argv[a]);
				exits("options");
			}
		else
			break;
	}
	if (a != argc - 1)
	{
		fprintf(stderr, "usage: na [options] file\n");
		exits("options");
	}
	if (access(argv[a], 4) < 0) {
		fprintf(stderr, "can't read %s\n", argv[a]);
		exits("");
	}
	in_f = tmpfile();
	preprocess(argv[a], in_f);
	rewind(in_f);
	strcpy(yyfilename, argv[a]);
	yyparse();
	if (errors)
		exits("pass1");
	pass2 = 1;
	printf("unsigned long na_script[] = {\n");
	yyrewind();
	yyparse();
	printf("};\n");
	printf("\n");
	printf("#define NA_SCRIPT_SIZE %d\n", dot / 4);
	printf("\n");
	fixup();
/*
	assemble();
*/
	exits(errors ? "pass2" : "");
}

void
preprocess(char *in, FILE *out)
{
	Waitmsg *w;
	char **argv;

	if (fork() == 0) {
		/* child */
		dup(fileno(out), 1);
		argv = (char **)malloc(sizeof(char *) * (ncppopts + 5));
		argv[0] = "cpp";
		memcpy(&argv[1], cppopts, sizeof(char *) * ncppopts);
		argv[ncppopts + 1] = "-+";
		argv[ncppopts + 2] = "-N";
		argv[ncppopts + 3] = in;
		argv[ncppopts + 4] = 0;
		exec("/bin/cpp", argv);
		fprintf(stderr, "failed to exec cpp (%R)\n");
		exits("exec");
	}
	w = wait();
	free(w);
}

struct sym *
findsym(char *name)
{
	struct sym *s;
	for (s = symlist; s; s = s->next)
		if (strcmp(name, s->name) == 0)
			return s;
	s = (struct sym *)malloc(sizeof(*s));
	s->name = strdup(name);
	s->t = Unknown;
	s->set = 0;
	s->next = symlist;
	symlist = s;
	return s;
}

void
setsym(struct sym *s, Type t, long v)
{
	if (pass2) {
		if (t == Unknown || t == Error)
			yyerror("can't resolve symbol");
		else {
			s->t = t;
			s->value = v;
		}
	}
	else {
		if (s->set)
			yyerror("multiply defined symbol");
		s->set = 1;
		s->t = t;
		s->value = v;
	}
}

int
mk24bitssigned(long *l)
{
	if (*l < 0) {
		if ((*l & 0xff800000L) != 0xff800000L) {
			*l = 0;
			return 0;
		}
		else
			*l = (*l) & 0xffffffL;
	}
	else if (*l > 0xffffffL) {
		*l = 0;
		return 0;
	}
	return 1;
}

static Type addresult[5][5] = {
/*		Const	Addr	Table   Extern	Reg */
/* Const */	Const,	Addr,	Table,	Error,  Reg,
/* Addr */	Addr,	Error,	Error,	Error,  Error,
/* Table */	Table,	Error,	Error,	Error,  Error,
/* Extern */	Error,	Error,	Error,	Error,	Error,
/* Reg */	Reg,	Error,  Error,  Error,	Error,
};

static Type subresult[5][5] = {
/*		Const	Addr	Table   Extern	Reg */
/* Const */	Const,	Error,	Error,	Error,  Error,
/* Addr */	Addr,	Const,	Error,	Error,	Error,
/* Table */	Table,	Error,	Const,	Error,	Error,
/* Extern */	Error,	Error,	Error,	Const,	Error,
/* Reg */	Error,	Error,  Error,  Error,	Error,
};

static Type muldivresult[5][5] = {
/*		Const	Addr	Table   Extern */
/* Const */	Const,	Error,	Error,	Error,	Error,
/* Addr */	Error,	Error,	Error,	Error,	Error,
/* Table */	Error,	Error,	Error,	Error,	Error,
/* Extern */	Error,	Error,	Error,	Error,	Error,
/* Reg */	Error,	Error,	Error,	Error,	Error,
};

static Type negresult[] = {
/* Const */	Const,
/* Addr */	Error,
/* Table */	Error,
/* Extern */	Error,
/* Reg */	Error,
};

int
patchtype(Type t)
{
	switch (t) {
	case Addr:
		return 1;
	case Reg:
		return 2;
	case Extern:
		return 4;
	default:
		return 0;
	}
}

struct expval
eval(struct expval a, struct expval b, char op)
{
	struct expval c;
	
	if (a.t == Unknown || b.t == Unknown) {
		c.t = Unknown;
		c.value = 0;
	}
	else if (a.t == Error || b.t == Error) {
		c.t = Error;
		c.value = 0;
	}
	else {
		switch (op) {
		case '+':
			c.t = addresult[a.t][b.t];
			break;
		case '-':
			c.t = subresult[a.t][b.t];
			break;
		case '*':
		case '/':
			c.t = muldivresult[a.t][b.t];
			break;
		case '_':
		case '~':
			c.t = negresult[a.t];
			break;
		default:
			c.t = Error;
			break;
		}
		if (c.t == Error) {
			if (pass2)
				yyerror("type clash in evaluation");
			c.value = 0;
		}
		else {
			switch (op) {
			case '+':
				c.value = a.value + b.value;
				break;
			case '-':
				c.value = a.value - b.value;
				break;
			case '*':
				c.value = a.value * b.value;
				break;
			case '/':
				c.value = a.value / b.value;
				break;
			case '_':
				c.value = -a.value;
				break;
			case '~':
				c.value = ~a.value;
				break;
			}
		}
	}
	return c;
}

void
regmove(unsigned char src_reg, unsigned char op,
    unsigned char dst_reg, struct expval *imm)
{
	unsigned char func, reg;
	int immdata;
	out.len = 2;
	if (src_reg == 8) {
		func = 5;
		reg = dst_reg;
	}
	else if (dst_reg == 8) {
		func = 6;
		reg = src_reg;
	}
	else {
		if (pass2 && src_reg != dst_reg)
			yyerror("Registers must be the same");
		func = 7;
		reg = src_reg;
	}
	immdata = imm ? (imm->value & 0xff) : 0;
	out.data[0] = 0x40000000L
	    | ((long)func << 27)
	    | ((long)op << 24)
	    | ((long)reg << 16)
	    | ((long)(immdata) << 8);
	out.data[1] = 0;
	out.patch[0] = (imm && imm->t == Extern) ? 3 : 0;
	out.patch[1] = 0;
}

long
mkreladdr(long addr, int len)
{
	long rel;
	rel = addr - (dot + 4 * len);
	mk24bitssigned(&rel);
	return rel;
}

long
chkreladdr(int d, struct expval *e, int len, long relrv)
{
	if (e->t == Addr) {
		out.data[d] = mkreladdr(e->value, len);
		out.patch[d] = 0;
		return relrv;
	} else {
		out.data[d] = e->value;
		out.patch[d] = patchtype(e->t);
		return 0;
	}
}

void
fixup(void)
{
	struct sym *s;
	int p;
	printf("struct na_patch na_patches[] = {\n");
	for (p = 0; p < patches; p++) {
		printf("\t{ 0x%.4x, %d }, /* %.8lx */\n",
		    patch[p].lwoff, patch[p].type, patch[p].lwoff * 4L);
	}
	if (patches == 0) {
		printf("\t{ 0, 0 },\n");
	}
	printf("};\n");
	printf("#define NA_PATCHES %d\n", patches);
	printf("\n");
	if (externs) {
		printf("enum na_external {\n");
		for (p = 0; p < externs; p++) {
			printf("\tX_%s,\n", externp[p]->name);
		}
		printf("};\n");
	}
	/* dump all labels (symbols of type Addr) as E_<Name> */
	for (s = symlist; s; s = s->next)
		if (s->t == Addr)
			break;
	if (s) {
		printf("\nenum {\n");
		while (s) {
			if (s->t == Addr)
				printf("\tE_%s = %ld,\n", s->name, s->value);
			s = s->next;
		}
		printf("};\n");
	}
	/* dump all Consts as #define A_<Name> value */
	for (s = symlist; s; s = s->next)
		if (s->t == Const)
			printf("#define A_%s %ld\n", s->name, s->value);
}

