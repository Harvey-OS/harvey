#include <u.h>
#include	<libc.h>
#include	<cda/fizz.h>

int cmppal(Type ** , Type ** );
int cmptype(Type ** , Type ** );
void cnttype(Type *c); 
void gettype(Type *c); 
void prchip(Chip *c); 
void prtype(Type *);
void prcbchip(Chip *); 
Board b;
Type * curtype;
int burn = 0;
int shortflg = 0;
int tflg = 0;
int typecnt;
Type ** types;
void
main(int argc, char **argv)
{
	int n;
	extern long nph;
	char *pkname = 0;
	extern int optind;
	extern char *optarg;
	extern Signal *maxsig;

	while((n = getopt(argc, argv, "stbp:")) != -1)
		switch(n)
		{
		case 'b':	burn = 1; break;
		case 't':	tflg = 1; break; /*for terry wallis cb*/
		case 's':	shortflg = 1; break;
		case 'p':	pkname = optarg; break;
		case '?':	break;
		}
	fizzinit();
	f_init(&b);
	if(optind == argc)
		argv[--optind] = "/dev/stdin";
	for(; optind < argc; optind++)
		if(n = f_crack(argv[optind], &b)){
			fprint(2, "%s: %d errors\n", argv[optind], n);
			exit(1);
		}
	fizzplace();
	cutout(&b);
	fizzplane(&b);
	if(burn) {
		fprint(1, "case $PALPATH in\n");
		fprint(1, "\t\"\") PALPATH=\"./\"\n");
		fprint(1, "\tesac\n");
		fprint(1, "case $PALCNT in\n");
		fprint(1, "\t\"\") PALCNT=1\n");
		fprint(1, "\tesac\n");
	}
	else if(tflg) {
		symtraverse(S_CHIP, prcbchip);
		exit(0);
	}
	else if(b.name)
		fprint(1, "Board %s:\n", b.name);
		symtraverse(S_TYPE, cnttype);
	typecnt = 0;
	symtraverse(S_TYPE, cnttype);
	n = typecnt;
	types = (Type **) f_malloc(typecnt * sizeof(Type));
	symtraverse(S_TYPE, gettype);
	typecnt = n;
	qsort(types, typecnt, sizeof(Type *), burn ? cmppal : cmptype);
	for(n = 0; n < typecnt; ++n)
		prtype(types[n]);
	exit(0);
}

int
cmppal(Type ** t1, Type ** t2)
{
	return(strcmp((*t1)->comment, (*t2)->comment));
}

int
cmptype(Type ** t1, Type ** t2)
{
	return(strcmp((*t1)->name, (*t2)->name));
}


int colcnt, chipcnt;

void
cnttype(Type *c)
{
	if(burn) {
		if((c->comment[0] == 'P')
		 && (c->comment[1] == 'A')
		 && (c->comment[2] == 'L')) ++typecnt;
	} 
	else ++typecnt;
}


void
gettype(Type *c)
{
	if(burn) {
		if((c->comment[0] == 'P')
		 && (c->comment[1] == 'A')
		 && (c->comment[2] == 'L'))
			types[--typecnt] = c;
	}
	else types[--typecnt] = c;
}

void
prtype(Type *c)
{
	colcnt = 0;
	curtype = c;
	chipcnt = 0;
	if(burn) {
			symtraverse(S_CHIP, prchip);
			fprint(1, "echo \"%s %s `echo %d*$PALCNT|hoc`\"; ", c->name, c->comment, chipcnt);
			fprint(1, "palwait; ");
			fprint(1, "cda/urom -w -J -N `echo %d*$PALCNT|hoc` -t $%s <$PALPATH/%s.jed;\n",
				chipcnt, c->comment, c->name);
	} 
	else {
		fprint(1, "%s	%s	%s\n", c->name, c->comment, c->pkgname);
		symtraverse(S_CHIP, prchip);
		if(colcnt) fprint(1, "\n");
		fprint(1, "	quantity %d\n", chipcnt);
	}
}

#define COLCNT  2
void
prchip(Chip *c) 
{
	if(c->type == curtype) {
		++chipcnt;
		if(burn | shortflg) return;
		fprint(1, "	%s %s %s (%d/%d)",
			c->name,
			((c->rotation == 3) ? "270" : 
				((c->rotation == 2) ? "180" :
					((c->rotation == 1) ? "90" : ""))),
			(c->flags & WSIDE) ? "W" : "",
			c->pt.x,
			c->pt.y
			);
		if(++colcnt == COLCNT) {
			fprint(1, "\n");
			colcnt = 0;
		}
	}
}

void
prcbchip(Chip *c) 
{
		fprint(1, "%s:%s:%s%s%s%s::%d.%3.3d:%d.%3.3d\n",
			c->name,
			c->typename,
			c->type->pkgname,
			(c->rotation&1) ? "-V" : "-H",
			(c->rotation&2) ? "-I" : "",
			(c->flags & WSIDE) ? "-W" : "",
			c->pt.x/1000,
			c->pt.x%1000,
			c->pt.y/1000,
			c->pt.y%1000);
}
