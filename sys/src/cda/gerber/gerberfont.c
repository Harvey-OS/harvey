#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	"gfont.h"

extern void gerbermoveto(int x, int y);
extern void gerberlineto(int x, int y);

void
gerberstring(int x, int y, char *s, int ht, int goo)
{
	uchar *p;
	int *v, w, centx, centy;
	double scale;
	enum { Font_ht = 22 };		/* for our font */
	int xm;

	xm = (goo&2)? -1:1;
	scale = ht/(double)Font_ht;
	/* figure out total width */
	w = 0;
	for(p = (uchar *)s; *p; p++){
		v = vecs[*p];
		w += *v * scale;
	}
	/* center and draw! */
	if(goo&1){
		y += xm*w/2;
		for(p = (uchar *)s; *p; p++){
			v = vecs[*p];
			w = *v++;
			centy = -xm*w*scale/2 + y;
			while(*v != ENDCHAR){
				gerbermoveto(v[1]*scale+x, -v[0]*xm*scale+centy);
				while(*(v += 2) != ENDVEC)
					gerberlineto(v[1]*scale+x, -v[0]*xm*scale+centy);
				v++;
			}
			y -= xm*w*scale;
		}
	} else {
		x -= xm*w/2;
		for(p = (uchar *)s; *p; p++){
			v = vecs[*p];
			w = *v++;
			centx = xm*w*scale/2 + x;
			while(*v != ENDCHAR){
				gerbermoveto(v[0]*xm*scale+centx, -v[1]*scale+y);
				while(*(v += 2) != ENDVEC)
					gerberlineto(v[0]*xm*scale+centx, -v[1]*scale+y);
				v++;
			}
			x += w*scale*xm;
		}
	}
}
