#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int
Bfildes(Biobufhdr *bp)
{

	return bp->fid;
}
