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
ointcnt(void)
{
	return intcnt;
}

void
oputint(int i)
{
	Bprint(bout, "%d,", i);
	intonline++;
	intcnt++;
	if(intonline >= MAXINTONLINE){
		Bputc(bout, '\n');
		intonline = 0;
	}
}

void
oputoct(int i)
{
	Bprint(bout, "0x%ux,", i);
	intonline++;
	intcnt++;
	if(intonline >= MAXINTONLINE){
		Bputc(bout, '\n');
		intonline = 0;
	}
}
