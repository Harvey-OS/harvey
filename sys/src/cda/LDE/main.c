#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"

int	argc;
char	**argv;
int	nlook;
int 	REDUCE, espresso, allprimes;
int	logicflag, invflag;
int	qflag;
void
main(int mrgc, char **mrgv)
{

	argc = mrgc;
	argv = mrgv;
	xargc = 0;
	qflag = 0;
	logicflag = 0;

	while (argc>1 && argv[1][0]=='-') {
		switch(argv[1][1]) {
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (xargc>=NXARGS) {
				fprintf(stderr, "too many numeric args\n");
				exits("error");;
			}
			xargv[xargc] = atoi(&argv[1][1]);
			xargc++;
			break;
		case 'q':
			qflag = 1;
			break;
		case 'a':
			plaflag++;
			break;
		case 'r':
			romflag++;
			break;
		case 'd':
		case 'v':
			displflag++;
			break;
		case 'e':
			espresso++;
			break;
		case 'L':
			logicflag++;
			break;
		case 'T':
			romflag++;
			treeflag++;
			break;
		case 'x':
			hexflag++;
			break;
		case 'X':
			xorflag++;
			break;
		case 'I':
			invflag += 2;
			break;
		case 'o':
			octflag++;
			break;
		case 'p':
			promflag++;
			break;
		case 'P':
			allprimes++;
			break;
		case 'w':
			drawflag++;
			break;
		case 'D':
			yydebug++;
			break;
		case 'R':
			++REDUCE;
			break;
		default:
			fprintf(stderr,"unknown option %c\n",argv[1][1]);
			errcount++;
		}
		argc--;
		argv++;
	}
	if(!romflag && !plaflag && !drawflag)
		plaflag++;
	yyfile = "<null>";
	yyparse();
	exits(errcount!=0 ? "error" : 0);
}

FILE *
setinput(void)
{
	static firstarg = 0;
	FILE * F;
	promflag = 1;
	if(firstarg == 0)
		firstarg = argc;
	if(argc > 1) {
		argc--;
		argv++;
		yyfile = argv[0];
		yylineno = 1;
		F = fopen(argv[0], "r");
		if(F == (FILE *) 0) {
			fprintf(stderr, "can't open %s\n", argv[0]);
			exits("error");;
		}
		return F;
	}
	if(firstarg == 1) {
		yyfile = "<stdin>";
		yylineno = 1;
		firstarg = -1;
		return stdin;
	}
	return (FILE *) 0;
}

void
clearcache(void)
{
	memset(cache, 0, sizeof(cache));
}

void
clearmem(void)
{

	lexinit = 0;
	memset(hshtab, 0, sizeof(hshtab));
	noutputs = nleaves = hshused = nextfield = 0;
	startnode = nextnode = 0;
}

/*
 * find leftmost one
 */
Value
flone(Value v)
{
	Value o;

	o = 0;
	if(v & 0xffff0000L) {
		o += 16;
		v >>= 16;
	}
	if(v & 0xff00) {
		o += 8;
		v >>= 8;
	}
	if(v & 0xf0) {
		o += 4;
		v >>= 4;
	}
	return "0122333344444444"[v&0xf] - '0' + o;
}

/*
 * find rightmost one
 */
Value
frone(Value v)
{
	Value o;

	o = 0;
	if(v == 0)
		return 0;
	if((v & 0xffffL) == 0) {
		o += 16;
		v >>= 16;
	}
	if((v & 0xffL) == 0) {
		o += 8;
		v >>= 8;
	}
	if((v & 0xfL) == 0) {
		o += 4;
		v >>= 4;
	}
	return "0121312141213121"[v&0xf] - '0' + o;
}

/*
 * binary to grey
 */
Value
btog(Value v)
{
	Value b;

	if(v < 2)
		return v;
	b = 1L << (flone(v) - 1);
	return b + btog(b+b-v-1);
}

#ifndef PLAN9
void
exits(char *x)	{
	exit( ( (int) x) ? (( *x ) ? 1 : 0) : 0 );
}
#endif
