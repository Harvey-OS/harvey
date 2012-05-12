#include	"u.h"
#include	"../port/lib.h"

int
gunzip(uchar *, int, uchar *, int)
{
	print("booting gzipped kernels is not supported by this bootstrap.\n");
	return -1;
}
