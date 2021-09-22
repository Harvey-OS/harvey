#include "audio.h"
#include <bio.h>

Biobuf*	bin;
Biobuf*	bou;

void
openw(char *ofile)
{
	bou = Bopen(ofile, OWRITE);
	if(bou == 0) {
		fprint(2, "cant create %s: %r\n", ofile);
		exits("create");
	}
}

void
closew(void)
{
	Bterm(bou);
	bou = 0;
}

void
openr(char *ifile)
{
	print("%s:\n", ifile);
	bin = Bopen(ifile, OREAD);
	if(bin == 0) {
		fprint(2, "cant open %s: %r\n", ifile);
		exits("open");
	}
}

void
closer(void)
{
	Bterm(bin);
	bin = 0;
}

char*
getline(void)
{
	char *p;

loop:
	if(bin == 0) {
		if(gargi >= gargc)
			return 0;
		openr(gargv[gargi]);
		gargi++;
		lineno = 0;
		goto loop;
	}
	p = Brdline(bin, '\n');
	if(p == 0) {
		closer();
		goto loop;
	}
	lineno++;
	p[BLINELEN(bin)-1] = 0;
	if(*p == '#')
		goto loop;
	return p;
}

void
putint1(int c)
{
	offset++;
	if(pass != 1)
		Bputc(bou, c);
}

void
putint2(long n)
{
	putint1(n>>8);
	putint1(n);
}

void
putint4(long n)
{
	putint2(n>>16);
	putint2(n);
}
