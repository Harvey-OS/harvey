#include <u.h>
#include <libc.h>
#include <stdio.h>
#include "dat.h"
#include "fns.h"
#include "gram.h"

int	argc;
char	**argv;
int	nlook;

void
main(int mrgc, char **mrgv)
{

	argv = mrgv;
	argc = 0;
	xargc = 0;

	while (mrgc>1 && mrgv[1][0]=='-') {
		switch(mrgv[1][1]) {
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
				fprint(2, "too many numeric mrgs\n");
#ifdef PLAN9
				exits("too many numeric mrgs");
#else
				exit(1);
#endif
			}
			xargv[xargc] = atoi(&mrgv[1][1]);
			xargc++;
			break;
		case 'q':
			nlook =  atoi(&mrgv[1][2]);
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
		case 'T':
			romflag++;
			treeflag++;
			break;
		case 'x':
			hexflag++;
			break;
		case 'o':
			octflag++;
			break;
		case 'p':
			promflag++;
			break;
		case 'w':
			drawflag++;
			break;
		case 'D':
			yydebug++;
			break;
		default:
			fprint(2,"unknown option %c\n",mrgv[1][1]);
			errcount++;
		}
		mrgc--;
		mrgv++;
	}
	if(!romflag && !plaflag && !drawflag)
		plaflag++;
	yyfile = "<null>";
	while (mrgc-- > 0)
		argv[++argc] = *++mrgv;
	yyparse();
#ifdef PLAN9
	if(errcount==0)
		exits((char *) 0);
	else
		exits("errcount!=0");
#else
	exit(errcount);
#endif
}
	
FILE
*setinput(void)
{
	static firstarg = 0;
	FILE *F;

	promflag = 1;
	if(firstarg == 0)
		firstarg = argc;
	if(argc > 1) {
		argc--;
		argv++;
		yyfile = argv[0];
		yylineno = 1;
		F = fopen(argv[0], "r");
		if(F == 0) {
			fprint(2, "can't open %s\n", argv[0]);
#ifdef PLAN9
			exits("can't open input file");
#else
			exit(1);
#endif
		}
		return F;
	}
	if(firstarg == 1) {
		yyfile = "<stdin>";
		yylineno = 1;
		firstarg = -1;
		return stdin;
	}
	return 0;
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
