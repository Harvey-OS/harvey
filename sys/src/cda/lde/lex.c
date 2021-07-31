#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"

FILE	*IN = 0;
char	yytext[NCPS+1];

typedef struct
{
	char	*kwname;
	int	kwval;
} Keywd;

#define	KWFLAG	0x80000000

Keywd kwtab[] =
{
	".o",	OUTPUTS,
	".i",	INPUTS,
	".io",	BOTHS,
	".b",	BURIEDS,
	".f",	FIELDS,
	".e",	EQNS,
	".x",	EXTERNS,
	".tt",	TYPES,
	".r",	ITER,
 	".u",	USED,
	0,	0
};

int
yylex(void)
{
	int c, nchars, base, x;
	Hshtab *hp;

	if(lexinit == 0) {
		Keywd *k;

	for(k=kwtab; k->kwval; k++) {
			hp = lookup(k->kwname, 1);
			hp->code = KWFLAG|k->kwval;
		}
		lexinit++;
	}
	if(IN == 0)
		if((IN=setinput()) == 0)
			return -1;
	nchars = 0;
	for(;;) {
		c = fgetc(IN);
		if(c == EOF) {
			if((IN=setinput()) != 0)
				continue;
			return c;
		}
		/*
		 * scan symbols
		 */
		if(isalpha(c) || (nchars && isdigit(c)) || (mode==EXTERN && isdigit(c)) ||
			(nchars && (c=='+' || c=='-')) || (c=='.')) {
				if(nchars >= NCPS-1)
					continue;
				yytext[nchars++] = c;
				continue;
		}

		/* look for trailing - */
		if(nchars && (c=='<')) {
				if((c = subseq('-', '<', '@')) == '@') {
					if(nchars >= NCPS-1)
						continue;
					yytext[nchars++] = c;
					continue;
				}
		}

		/*
		 * scan and convert integers
		 * leading zero implies octal, 0x hexadecimal
		 */
		if(nchars==0 && isdigit(c)){
			base = (c=='0') ? subseq('x', 8, 16) : 10;
			yylval.code = 0;
			while ((x = makdigit(c, base)) >= 0) {
				yylval.code *= base;
				yylval.code += x;
				c = fgetc(IN);
			}
			ungetc(c, IN);
			return NUMBER;
		}
		/*
		 * lookup scanned symbols
		 */
		if(nchars) {
			ungetc(c, IN);
			if((nchars > 2)
				&& (yytext[nchars-2] == '@')
				&& ((yytext[nchars-1] == 'P') 
				|| (yytext[nchars-1] == 'Q'))) {
					yytext[nchars-2] = 0;
					if((hp = lookup(yytext, 0)) == 0) {
						fprint(2, "feedback <-%c for undefined %s\n",
							yytext[nchars-1], yytext);
#ifdef PLAN9
						exits("feedback undefined ");
#else
						exit(1);
#endif
					}
					hp = fbctl(hp, yytext[nchars-1]);
			} 
			else {
				yytext[nchars] = 0;
				hp = lookup(yytext, 1);
			}
			yylval.hp = hp;
			if(hp->code&KWFLAG)
				return hp->code & ~KWFLAG;
			if((mode==EQN||mode==FIELD) && hp->code)
				return hp->code;
			return NAME;
		}
		switch(c){
		case '\n':
			yylineno++;
		case ' ':
		case '\t':
			continue;
		case '/':
			if(subseq('*',1,0))
				return(yylval.code=DIV);
			do{
				while((c=fgetc(IN))!= '*'){
					if(c=='\n')
						yylineno++;
				}
			}while(subseq('/',1,0));
			continue;
		case '=':
			return yylval.code=subseq(c,c,EQ);
		case '|':
			return yylval.code=subseq('|',OR,LOR);
		case '&':
			return yylval.code=subseq('&',AND,LAND);
		case '!':
			return yylval.code=subseq('=',NOT,NE);
		case '<':
			return yylval.code=subseq('=',subseq('<',LT,LS),LE);
		case '>':
			return yylval.code=subseq('=',subseq('>',GT,RS),GE);
		case '+':
			return yylval.code=subseq('=',ADD,ADDEQU);
		case '-':
			return yylval.code=subseq('=',SUB,SUBEQU);
		case '*':
			return yylval.code=subseq('=',MUL,MULEQU);
		case '%':
			return yylval.code=subseq('=',MOD,MODEQU);
		case '^':
			return yylval.code=subseq('=',XOR,XOREQU);
		case '~':
			return yylval.code=subseq('=',COM,COMEQU);
		case '#':
			return yylval.code=subseq('=',PND,PNDEQU);
		case '?':
			return yylval.code=CND;
		case ':':
			return yylval.code=subseq('=',ALT,ALTEQU);
		case '\'':
			return yylval.code=DC;
		case '$':
			c = fgetc(IN);
			if(isdigit(c)){
				x = c - '0';
				while((c=fgetc(IN)) >= '0' && c <= '9')
					x = 10*x + c-'0';
				ungetc(c, IN);
				if(x>=xargc)
					yyerror("bad goobie $%d", x);
				yylval.code = xargv[x];
				return NUMBER;
			}
			yyerror("missing number following $");
			/* fall through */
		default:
			return c;
		}
	}
}

/*
 * return digit value of character, or -1 if illegal
 */
int
makdigit(int c, int base)
{
	if('0' <= c && c <= '9')
		return c - '0';
	if(base != 16)
		return -1;
	if('A' <= c && c <= 'F')
		return c - 'A' + 10;
	if('a' <= c && c <= 'f')
		return c - 'a' + 10;
	return -1;
}

/*
 * grab 2-char tokens, borrowed from C
 */
int
scanseq(int c, int a, int b)
{
	int peekc;

	do {
		peekc = fgetc(IN);
	} while (peekc==' ' || peekc=='\t');
	if(peekc == c)
		return(b);
	ungetc(peekc, IN);
	return a;
}

int
subseq(int c, int a, int b)
{
	int peekc;

	peekc = fgetc(IN);
	if (peekc == c)
		return(b);
	ungetc(peekc, IN);
	return(a);
}

int
seqsub(int c, int a, int b)
{
	int peekc;

	peekc = fgetc(IN);
	if (peekc != c)
		return(a);
	ungetc(peekc, IN);
	return(b);
}

/*
 * hash table routine
 */
Hshtab*
lookup(char *string, int ent)
{
	Hshtab *rp;
	char *sp;
	int ihash;

	/*
	 * hash
	 * take one's complement of every other char
	 */
	ihash = 0;
	for(sp=string; *sp; sp++)
		ihash = ihash+ihash+ihash + *sp;
	if(ihash < 0)
		ihash = ~ihash;
	rp = &hshtab[ihash%HSHSIZ];

	/*
	 * lookup
	 */
	while(*(sp = rp->name)) {
		if(strcmp(string, sp) == 0)
			return rp;
		rp++;
		if(rp >= &hshtab[HSHSIZ])
			rp = hshtab;
	}
	if(ent == 0)
		return 0;
	hshused++;
	if(hshused >= HSHSIZ)
		yyerror("Symbol table overflow\n");
	strcpy(sp, string);
	return rp;
}

Hshtab*
comname(Hshtab *hp)
{
	char cptext[NCPS+1];
	char *cp;
	Hshtab *h;

	strcpy(cptext, hp->name);
	cp = strchr(cptext, 0);
	if(cp[-1] != '-') {
		cp[0] = '-';
		cp[1] = 0;
	} else
		cp[-1] = 0;
	h = lookup(cptext, 0);
	if(h == 0)
		h = hp;
	return h;
}

Hshtab*
fbctl(Hshtab *hp, char type)
{
	int lo, hi, i;
	Hshtab *hp1;
	char buf[NCPS];
	setiused(hp,ALLBITS);
	sprint(buf, "%s@%c", hp->name, type);
	hp1 = lookup(buf, 1);
	if(hp1->code) return(hp1);
	if(hp->code == FIELD) {
		SETLO(hp1, nextfield);
		hp1->code = FIELD;
		lo = LO(hp);
		hi = HI(hp);
		for(i = lo; i < hi; ++i) {
			hp = fields[i];
			hp = fbctl(hp, type);
			fields[nextfield++] = hp;
			if(nextfield >= NFIELDS)
				yyerror("too many fields\n");
		}
		SETHI(hp1, nextfield);
		return(hp1);
	}
	hp1->iref = ++nleaves;
	if(nleaves >= NLEAVES)
		yyerror("too many inputs\n");
	leaves[nleaves] = hp1;
	hp1->code = INPUT;
	hp1->pin = 0;
	hp1->iused = 0;
	hp1->assign = (Node *) 0;
	switch(type) {
	case 'P':
		hp1->type |= CTLPIN;
		break;
	case 'Q':
		hp1->type |= CTLQIN;
		break;
	}
	return(hp1);
}
