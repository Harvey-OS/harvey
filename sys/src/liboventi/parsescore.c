/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <oventi.h>

int
vtParseScore(char *buf, uint n, uchar score[VtScoreSize])
{
	int i, c;

	memset(score, 0, VtScoreSize);

	if(n != VtScoreSize*2)
		return 0;
	for(i=0; i<VtScoreSize*2; i++){
		if(buf[i] >= '0' && buf[i] <= '9')
			c = buf[i] - '0';
		else if(buf[i] >= 'a' && buf[i] <= 'f')
			c = buf[i] - 'a' + 10;
		else if(buf[i] >= 'A' && buf[i] <= 'F')
			c = buf[i] - 'A' + 10;
		else
			return 0;

		if((i & 1) == 0)
			c <<= 4;
	
		score[i>>1] |= c;
	}
	return 1;
}

