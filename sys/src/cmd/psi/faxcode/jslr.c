#include <u.h>
#include <libc.h>
#include "../system.h"
#ifdef FAX
#include "CPU.h"
/* rlbr - run-length encode a binary raster line
  `rasp' is a raster line of bits.  rasp[0] is the leftmost byte in the line and
   the 001 bit is the leftmost bit in each byte ("little-endian").
   Analyzes the line into a series of runs of `1's, placing them into array
   `runs'.  Observes margins:  starts at `lm' bit and continues through `rm'; bits
   outside margins are assumed 0.   A `run' is two shorts (the starting &
   ending indices of the run of `1's, inclusive).  The indices are shifted
   so that the index of `lm' becomes `olm'.  Returns the number of runs.
   NOTES:
	The contents of the raster-line are restored to their orginal
   state on return. The user must ensure that:
	(1) margins fall within `rasp';
   	(2) `rasp' has one extra byte at the end for use by this routine; and
   	(3) `runs' is big enough to hold the worst-case no. of runs that could
	     result, which is (rm-lm)/2+1.
   */

int rlbr(unsigned char rasp[],int rm,short runp[])	/*l 0 wid-1 0*/
{	register char unsigned *bp,*ep;	/* byte pointers into raster line */
	register short cb;	/* x_coordinate of 1st bit of current byte */
	register short *xp;	/* ptr to x-coordinate in run-length array */
	register short prior;	/* prior bit, coded 0400 or 0000 */

	char unsigned *lbp,*rbp;	/* ptrs to margin bytes */
	int ro;			/* `offset' of margin bits in bytes */
	char unsigned svlb,svrb,svrrb;	/* unmodified copies of margin bytes */

	/* compute margins' bytes, save copies, set bits outside margins to 0 */
	lbp=rasp;  svlb = *lbp;
					/* force bits left of lm to 0 */
	rbp=rasp+(rm/8); svrb = *rbp;
					/* force bits right of rm to 0 */
	ro=rm%8;  if(ro<7) *rbp &= (0377>>(7-ro));
				/* place a 0000 byte to right of margin byte, to force good termination */
	svrrb = *(rbp+1);  *(rbp+1)='\0';

	/* MAIN LOOP */
	prior=0000;	/* act as if there's a 0 byte to left of margin byte */
	xp=runp;
	bp=lbp;
	ep=rbp+2;
	cb=0;
	do {
		switch( prior | *(bp++) ) {
			/* contents of rlbr.c1 come below here */
#ifdef AHMDAL
#include "bigend.h"
#else
#include "littleend.h"
#endif
			/* contents of jslr.c1 come above here */
			};
		cb += 8;
		}
	while(bp<ep);
	/* restore margin bytes */
	*lbp = svlb;
	*rbp = svrb;
	*(rbp+1) = svrrb;
	return((xp-runp)/2);
	};
#endif
