#include "../lib9.h"

#include "../libdraw/draw.h"

static
int
unitsperline(Rectangle r, int d, int bitsperunit)
{
	ulong l, t;

	if(r.min.x >= 0){
		l = (r.max.x*d+bitsperunit-1)/bitsperunit;
		l -= (r.min.x*d)/bitsperunit;
	}else{			/* make positive before divide */
		t = (-r.min.x*d+bitsperunit-1)/bitsperunit;
		l = t+(r.max.x*d+bitsperunit-1)/bitsperunit;
	}
	return l;
}

int
wordsperline(Rectangle r, int d)
{
	return unitsperline(r, d, 8*sizeof(ulong));
}

int
bytesperline(Rectangle r, int d)
{
	return unitsperline(r, d, 8);
}
