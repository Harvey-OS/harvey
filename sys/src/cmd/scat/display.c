#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libg.h>
#include "sky.h"

void
display(Picture *pic)
{
	int p[2];
	int i, n;
	uchar *a;
	

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
		dup(p[0], 10);
		close(p[0]);
		execl("/bin/fb/9v", "9v", "/fd/10", 0);
		fprint(2, "exec failed: %r\n");
		exits("exec");

	default:
		close(p[0]);
		fprint(p[1], "%11d %11d %11d %11d %11d ",
			3, pic->minx, pic->miny, pic->maxx, pic->maxy);
		n = (pic->maxx-pic->minx)*(pic->maxy-pic->miny);
		/* release the memory as we hand it off; this could be a big piece of data */
		a = pic->data;
		while(n > 0){
			i = 8192 - (((int)a)&8191);
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
