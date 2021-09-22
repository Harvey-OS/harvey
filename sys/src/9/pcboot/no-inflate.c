#include	"u.h"
#include	"../port/lib.h"

int
gunzip(uchar *, int, uchar *, int)
{
	print("booting compressed kernels is not supported by this bootstrap.\n");
	return -1;
}

int
lunzip(uchar *, int, uchar *, int)
{
	return gunzip(nil, 0, nil, 0);
}
