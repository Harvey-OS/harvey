#include <u.h>
#include <libc.h>
#include "icc.h"
#include "nfc.h"
#include "reader.h"
#include "mifare.h"

/* (1K=0x00..0x39, 4K=0x00..0xff) for classic */
CmdiccR *
mifareop(int fd, int ncard, uchar mc, uchar nblk)
{
	CmdiccR *cr;
	uchar buf[3];

	buf[0] = ncard;
	buf[1] = mc;
	buf[2] = nblk;

	cr = xchangedata(fd, buf, 3);	/* BUG, should go through ccid */
	return cr;
}

/* bug, should process the answer and so... */
int
mifareblkread(int fd, uchar *blk, uchar nbytes, uchar off, uchar nblk)
{
	CmdiccR *cr;
	int nr;
	uchar *d;

	cr = mifareop(fd, 0x01, MCREAD, 4*nblk);
	if( cr == nil || cr->dataf == nil)
		return -1;
	
	nr = cr->sw2;
	free(cr);
	if(nr)
		cr = readtag(fd, nr);
	else
		return -1;		
	if( cr == nil )
		return -1;
	d = cr->dataf;

	nr -= 6; 	/* header 4, trailer 2 */
	if(off > nr)
		return 0;
	if (nbytes+off > nr) 
		nbytes =  nr - off;
	
	memmove(blk, d+4+off, nbytes);	/* take off the header */
	free(cr);
	return nbytes;
}

