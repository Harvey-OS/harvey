#include "common.h"
#define MAXINTONLINE 10
static int intcnt, intonline, firstint;

void
oreset(void)
{
	intcnt = 0;
	intonline = 0;
	firstint = 1;
}

int
ointcnt(void) {
	return(intcnt);
}

void
oputint(int i)
{
	if(!firstint) putc(',', outfile);
	else firstint = 0;
	fprintf(outfile, "%d", i);
	intonline++;
	intcnt++;
	if(intonline >= MAXINTONLINE) {
		putc('\n', outfile); intonline = 0;
	}
}

void
oputoct(int i)
{
	if(!firstint) putc(',', outfile);
	else firstint = 0;
	fprintf(outfile, "0%o", i);
	intonline++;
	intcnt++;
	if(intonline >= MAXINTONLINE) { putc('\n', outfile); intonline = 0; }
}
