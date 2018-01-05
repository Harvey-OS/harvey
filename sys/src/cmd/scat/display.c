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
#include <bio.h>
#include <draw.h>
#include "sky.h"

void
displaypic(Picture *pic)
{
	int p[2];
	int i, n;
	uint8_t *a;


	if(pipe(p) < 0){
		fprint(2, "pipe failed: %r\n");
		return;
	}
	switch(rfork(RFPROC|RFFDG|RFNOTEG|RFNOWAIT)){
	case -1:
		fprint(2, "fork failed: %r\n");
		return;

	case 0:
		close(p[1]);
		dup(p[0], 0);
		close(p[0]);
		execl("/bin/page", "page", "-w", nil);
		fprint(2, "exec failed: %r\n");
		exits("exec");

	default:
		close(p[0]);
		fprint(p[1], "%11s %11d %11d %11d %11d ",
			"k8", pic->minx, pic->miny, pic->maxx, pic->maxy);
		n = (pic->maxx-pic->minx)*(pic->maxy-pic->miny);
		/* release the memory as we hand it off; this could be a big piece of data */
		a = pic->data;
		while(n > 0){
			i = 8192 - (((uintptr)a)&8191);
			if(i > n)
				i = n;
			if(write(p[1], a, i)!=i)
				fprint(2, "write error: %r\n");
			if(i == 8192)	/* page aligned */
				segfree(a, i);
			n -= i;
			a += i;
		}
		free(pic->data);
		free(pic);
		close(p[1]);
		break;
	}
}

void
displayimage(Image *im)
{
	int p[2];

	if(pipe(p) < 0){
		fprint(2, "pipe failed: %r\n");
		return;
	}
	switch(rfork(RFPROC|RFFDG|RFNOTEG|RFNOWAIT)){
	case -1:
		fprint(2, "fork failed: %r\n");
		return;

	case 0:
		close(p[1]);
		dup(p[0], 0);
		close(p[0]);
		execl("/bin/page", "page", "-w", nil);
		fprint(2, "exec failed: %r\n");
		exits("exec");

	default:
		close(p[0]);
		writeimage(p[1], im, 0);
		freeimage(im);
		close(p[1]);
		break;
	}
}
