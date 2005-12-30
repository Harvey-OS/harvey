#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"error.h"


void
cinit(void)
{
}

void
copen(Chan *c)
{
	USED(c);
}

int
cread(Chan *c, uchar *buf, int len, vlong off)
{
	USED(c);
	USED(buf);
	USED(len);
	USED(off);

	return 0;
}

void
cupdate(Chan *c, uchar *buf, int len, vlong off)
{
	USED(c);
	USED(buf);
	USED(len);
	USED(off);
}

void
cwrite(Chan* c, uchar *buf, int len, vlong off)
{
	USED(c);
	USED(buf);
	USED(len);
	USED(off);
}
