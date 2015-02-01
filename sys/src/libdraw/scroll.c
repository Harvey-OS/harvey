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
#include <draw.h>

int
mousescrollsize(int maxlines)
{
	static int lines, pcnt;
	char *mss;

	if(lines == 0 && pcnt == 0){
		mss = getenv("mousescrollsize");
		if(mss){
			if(strchr(mss, '%') != nil)
				pcnt = atof(mss);
			else
				lines = atoi(mss);
			free(mss);
		}
		if(lines == 0 && pcnt == 0)
			lines = 1;
		if(pcnt>=100)
			pcnt = 100;
	}

	if(lines)
		return lines;
	return pcnt * maxlines/100.0;	
}
