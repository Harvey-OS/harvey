#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

void
cinit(void)
{
}

void
copen(Chan* chan)
{
	chan->flag &= ~CCACHE;
}

int
cread(Chan*, uchar*, int, vlong)
{
	return 0;
}

void
cupdate(Chan*, uchar*, int, vlong)
{
}

void
cwrite(Chan*, uchar*, int, vlong)
{
}
