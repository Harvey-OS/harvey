#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<cda/fizz.h>

void	clrcount(Type*);
void	inccount(Chip*);
void	outcount(Type*);

extern int printcol;

int	debug;
Board	b;
Biobuf	out;

void
main(int argc, char **argv)
{
	char *fname;
	int n;

	Binit(&out, 1, OWRITE);
	ARGBEGIN{
	case 'D':
		++debug; break;
	default:
		fprint(2, "usage: %s [files...]\n", argv0);
		exits("usage");
	}ARGEND
	fizzinit();
	f_init(&b);
	if(*argv)
		fname = *argv++;
	else
		fname = "/fd/0";
	do{
		if(n = f_crack(fname, &b)){
			fprint(2, "%s: %d errors\n", fname, n);
			exits("crack");
		}
	}while(fname = *argv++);	/* assign = */
	fizzplane(&b);
	if(n = fizzplace()){
		fprint(2, "warning: %d chip%s unplaced\n",
			n, n==1?"":"s");
	}
	if(fizzprewrap())
		exits("prewrap");
	symtraverse(S_TYPE, clrcount);
	symtraverse(S_CHIP, inccount);
	symtraverse(S_TYPE, outcount);
	exits(0);
}

void
clrcount(Type *t)
{
	t->nam = 0;
}

void
inccount(Chip *c)
{
	++c->type->nam;
}

void
outcount(Type *t)
{
	if(t->nam == 0){
		fprint(2, "type %s unused\n", t->name);
		return;
	}
	Bprint(&out, "%s\t", t->name);
	while(printcol < 16)
		Bprint(&out, "\t");
	Bprint(&out, "%d\n", t->nam);
}
