#include <u.h>
#include <libc.h>
#include <libg.h>
#include <fb.h>
int wrpicfile(PICFILE *f, Bitmap *b){
	int width=PIC_WIDTH(f);
	int nchan=PIC_NCHAN(f);
	int depth=1<<b->ldepth;
	int mask=(1<<depth)-1;
	int y, shift;
	char *fbuf, *efbuf, *fp;
	uchar *bbuf, *bp;
	if(width!=b->r.max.x-b->r.min.x);
	fbuf=malloc(width*nchan);
	if(fbuf==0) return -1;
	if(depth==nchan*8){	/* simple case, why fool around? */
		for(y=b->r.min.x;y!=b->r.max.x;y++){
			rdbitmap(b, y, y+1, (uchar *)fbuf);
			picwrite(f, fbuf);
		}
		free(fbuf);
		return 0;
	}
	memset(fbuf, 0, width*nchan);
	efbuf=fbuf+width*nchan;
	/*
	 * The code below fails to work if ldepth>3
	 */
	bbuf=malloc(b->r.max.x-b->r.min.x);	/* overestimate */
	if(bbuf==0){
		free(fbuf);
		return -1;
	}
	for(y=b->r.min.y;y!=b->r.max.y;y++){
		rdbitmap(b, y, y+1, bbuf);
		bp=bbuf;
		shift=8-depth*(b->r.min.x%(8/depth)+1);
		for(fp=fbuf;fp!=efbuf;fp+=nchan){
			*fp=((~*bp>>shift)&mask)*255/mask;
			shift-=depth;
			if(shift<0){
				shift+=8;
				bp++;
			}
		}
		picwrite(f, fbuf);
	}
	free(bbuf);
	free(fbuf);
	return 0;
}
